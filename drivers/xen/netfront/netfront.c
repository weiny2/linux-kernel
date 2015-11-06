/******************************************************************************
 * Virtual network driver for conversing with remote driver backends.
 *
 * Copyright (c) 2002-2005, K A Fraser
 * Copyright (c) 2005, XenSource Ltd
 * Copyright (C) 2007 Solarflare Communications, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation; or, when distributed
 * separately from the Linux kernel or incorporated into other
 * software packages, subject to the following license:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/pfn.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/stringify.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/ethtool.h>
#include <linux/in.h>
#include <linux/if_ether.h>
#include <linux/io.h>
#include <linux/moduleparam.h>
#include <net/sock.h>
#include <net/pkt_sched.h>
#include <net/route.h>
#include <net/tcp.h>
#include <asm/uaccess.h>
#include <xen/evtchn.h>
#include <xen/xenbus.h>
#include <xen/interface/io/netif.h>
#include <xen/interface/memory.h>
#include <xen/balloon.h>
#include <asm/page.h>
#include <asm/maddr.h>
#include <asm/uaccess.h>
#include <xen/interface/grant_table.h>
#include <xen/gnttab.h>
#include <xen/net-util.h>
#include <xen/xen_pvonhvm.h>

struct netfront_cb {
	unsigned int pull_to;
};

#define NETFRONT_SKB_CB(skb)	((struct netfront_cb *)((skb)->cb))

#include "netfront.h"

/*
 * Mutually-exclusive module options to select receive data path:
 *  rx_copy : Packets are copied by network backend into local memory
 *  rx_flip : Page containing packet data is transferred to our ownership
 * For fully-virtualised guests there is no option - copying must be used.
 * For paravirtualised guests, flipping is the default.
 */
#ifdef CONFIG_XEN
static bool MODPARM_rx_copy;
module_param_named(rx_copy, MODPARM_rx_copy, bool, 0);
MODULE_PARM_DESC(rx_copy, "Copy packets from network card (rather than flip)");
static bool MODPARM_rx_flip;
module_param_named(rx_flip, MODPARM_rx_flip, bool, 0);
MODULE_PARM_DESC(rx_flip, "Flip packets from network card (rather than copy)");
#else
# define MODPARM_rx_copy true
# define MODPARM_rx_flip false
#endif

#define RX_COPY_THRESHOLD 256

/* If we don't have GSO, fake things up so that we never try to use it. */
#if defined(NETIF_F_GSO)
#define HAVE_GSO			1
#define HAVE_TSO			1 /* TSO is a subset of GSO */
#define NO_CSUM_OFFLOAD			0
static inline void dev_disable_gso_features(struct net_device *dev)
{
	/* Turn off all GSO bits except ROBUST. */
	dev->features &= ~NETIF_F_GSO_MASK;
	dev->features |= NETIF_F_GSO_ROBUST;
}
#elif defined(NETIF_F_TSO)
#define HAVE_GSO		       0
#define HAVE_TSO                       1

/* Some older kernels cannot cope with incorrect checksums,
 * particularly in netfilter. I'm not sure there is 100% correlation
 * with the presence of NETIF_F_TSO but it appears to be a good first
 * approximiation.
 */
#define NO_CSUM_OFFLOAD			1

#define gso_size tso_size
#define gso_segs tso_segs
static inline void dev_disable_gso_features(struct net_device *dev)
{
       /* Turn off all TSO bits. */
       dev->features &= ~NETIF_F_TSO;
}
static inline int skb_is_gso(const struct sk_buff *skb)
{
        return skb_shinfo(skb)->tso_size;
}
static inline int skb_gso_ok(struct sk_buff *skb, int features)
{
        return (features & NETIF_F_TSO);
}

#define netif_skb_features(skb) ((skb)->dev->features)
static inline int netif_needs_gso(struct sk_buff *skb, int features)
{
        return skb_is_gso(skb) &&
               (!skb_gso_ok(skb, features) ||
                unlikely(skb->ip_summed != CHECKSUM_PARTIAL));
}
#else
#define HAVE_GSO			0
#define HAVE_TSO			0
#define NO_CSUM_OFFLOAD			1
#define netif_needs_gso(skb, feat)	0
#define dev_disable_gso_features(dev)	((void)0)
#define ethtool_op_set_tso(dev, data)	(-ENOSYS)
#endif

struct netfront_rx_info {
	struct netif_rx_response rx;
	struct netif_extra_info extras[XEN_NETIF_EXTRA_TYPE_MAX - 1];
};

/*
 * Implement our own carrier flag: the network stack's version causes delays
 * when the carrier is re-enabled (in particular, dev_activate() may not
 * immediately be called, which can cause packet loss).
 */
#define netfront_carrier_on(netif)	((netif)->carrier = 1)
#define netfront_carrier_off(netif)	((netif)->carrier = 0)
#define netfront_carrier_ok(netif)	((netif)->carrier)

/*
 * Access macros for acquiring freeing slots in tx_skbs[].
 */

static inline void add_id_to_freelist(struct sk_buff **list, unsigned short id)
{
	list[id] = list[0];
	list[0]  = (void *)(unsigned long)id;
}

static inline unsigned short get_id_from_freelist(struct sk_buff **list)
{
	unsigned int id = (unsigned int)(unsigned long)list[0];
	list[0] = list[id];
	return id;
}

static inline int xennet_rxidx(RING_IDX idx)
{
	return idx & (NET_RX_RING_SIZE - 1);
}

static inline struct sk_buff *xennet_get_rx_skb(struct netfront_info *np,
						RING_IDX ri)
{
	int i = xennet_rxidx(ri);
	struct sk_buff *skb = np->rx_skbs[i];
	np->rx_skbs[i] = NULL;
	return skb;
}

static inline grant_ref_t xennet_get_rx_ref(struct netfront_info *np,
					    RING_IDX ri)
{
	int i = xennet_rxidx(ri);
	grant_ref_t ref = np->grant_rx_ref[i];
	np->grant_rx_ref[i] = GRANT_INVALID_REF;
	return ref;
}

#define DPRINTK(fmt, args...)				\
	pr_debug("netfront (%s:%d) " fmt,		\
		 __FUNCTION__, __LINE__, ##args)

static int setup_device(struct xenbus_device *, struct netfront_info *);
static struct net_device *create_netdev(struct xenbus_device *);

static void end_access(int, void *);
static void netif_release_rings(struct netfront_info *);
static void netif_disconnect_backend(struct netfront_info *);

static int network_connect(struct net_device *);
static void network_tx_buf_gc(struct net_device *);
static void network_alloc_rx_buffers(struct net_device *);

static irqreturn_t netif_int(int irq, void *dev_id);

#ifdef CONFIG_SYSFS
static int xennet_sysfs_addif(struct net_device *netdev);
static void xennet_sysfs_delif(struct net_device *netdev);
#else /* !CONFIG_SYSFS */
#define xennet_sysfs_addif(dev) (0)
#define xennet_sysfs_delif(dev) do { } while(0)
#endif

static inline bool xennet_can_sg(struct net_device *dev)
{
	return dev->features & NETIF_F_SG;
}

static void netfront_free_netdev(struct net_device *netdev)
{
	struct netfront_info *np = netdev_priv(netdev);

	free_percpu(np->rx_stats);
	free_percpu(np->tx_stats);
	free_netdev(netdev);
}

/*
 * Work around net.ipv4.conf.*.arp_notify not being enabled by default.
 */
static void netfront_enable_arp_notify(struct netfront_info *info)
{
#ifdef CONFIG_INET
	struct in_device *in_dev;

	rtnl_lock();
	in_dev = __in_dev_get_rtnl(info->netdev);
	if (in_dev && !IN_DEV_CONF_GET(in_dev, ARP_NOTIFY))
		IN_DEV_CONF_SET(in_dev, ARP_NOTIFY, 1);
	rtnl_unlock();
	if (!in_dev)
		pr_warn("Cannot enable ARP notification on %s\n",
			info->xbdev->nodename);
#endif
}

static bool netfront_nic_unplugged(struct xenbus_device *dev)
{
	bool ret = true;
#ifndef CONFIG_XEN
	char *typestr;
	/*
	 * For HVM guests:
	 * - pv driver required if booted with xen_emul_unplug=nics|all
	 * - native driver required with xen_emul_unplug=ide-disks|never
	 */

	/* Use pv driver if emulated hardware was unplugged */
	if (xen_pvonhvm_unplugged_nics)
		return true;

	typestr  = xenbus_read(XBT_NIL, dev->otherend, "type", NULL);

	/* Assume emulated+pv interface (ioemu+vif) when type property is missing. */
	if (IS_ERR(typestr))
		return false;

	/* If the interface is emulated and not unplugged, skip it. */
	if (strcmp(typestr, "vif_ioemu") == 0 || strcmp(typestr, "ioemu") == 0)
		ret = false;
	kfree(typestr);
#endif
	return ret;
}

/**
 * Entry point to this code when a new device is created.  Allocate the basic
 * structures and the ring buffers for communication with the backend, and
 * inform the backend of the appropriate details for those.
 */
static int netfront_probe(struct xenbus_device *dev,
			  const struct xenbus_device_id *id)
{
	int err;
	struct net_device *netdev;
	struct netfront_info *info;

	if (!netfront_nic_unplugged(dev)) {
		pr_warning("netfront: skipping emulated interface, native driver required\n");
		return -ENODEV;
	}

	netdev = create_netdev(dev);
	if (IS_ERR(netdev)) {
		err = PTR_ERR(netdev);
		xenbus_dev_fatal(dev, err, "creating netdev");
		return err;
	}

	info = netdev_priv(netdev);
	dev_set_drvdata(&dev->dev, info);

	err = register_netdev(info->netdev);
	if (err) {
		pr_warning("%s: register_netdev err=%d\n",
			   __FUNCTION__, err);
		goto fail;
	}

	netfront_enable_arp_notify(info);

	err = xennet_sysfs_addif(info->netdev);
	if (err) {
		unregister_netdev(info->netdev);
		pr_warning("%s: add sysfs failed err=%d\n",
			   __FUNCTION__, err);
		goto fail;
	}

	return 0;

 fail:
	netfront_free_netdev(netdev);
	dev_set_drvdata(&dev->dev, NULL);
	return err;
}

static int netfront_remove(struct xenbus_device *dev)
{
	struct netfront_info *info = dev_get_drvdata(&dev->dev);

	DPRINTK("%s\n", dev->nodename);

	netfront_accelerator_call_remove(info, dev);

	netif_disconnect_backend(info);

	del_timer_sync(&info->rx_refill_timer);

	xennet_sysfs_delif(info->netdev);

	unregister_netdev(info->netdev);

	netfront_free_netdev(info->netdev);

	return 0;
}


static int netfront_suspend(struct xenbus_device *dev)
{
	struct netfront_info *info = dev_get_drvdata(&dev->dev);
	return netfront_accelerator_suspend(info, dev);
}


static int netfront_suspend_cancel(struct xenbus_device *dev)
{
	struct netfront_info *info = dev_get_drvdata(&dev->dev);
	return netfront_accelerator_suspend_cancel(info, dev);
}


/**
 * We are reconnecting to the backend, due to a suspend/resume, or a backend
 * driver restart.  We tear down our netif structure and recreate it, but
 * leave the device-layer structures intact so that this is transparent to the
 * rest of the kernel.
 */
static int netfront_resume(struct xenbus_device *dev)
{
	struct netfront_info *info = dev_get_drvdata(&dev->dev);

	DPRINTK("%s\n", dev->nodename);

	netfront_accelerator_resume(info, dev);

	netif_disconnect_backend(info);
	return 0;
}

static int xen_net_read_mac(struct xenbus_device *dev, u8 mac[])
{
	char *s, *e, *macstr;
	int i;

	macstr = s = xenbus_read(XBT_NIL, dev->nodename, "mac", NULL);
	if (IS_ERR(macstr))
		return PTR_ERR(macstr);

	for (i = 0; i < ETH_ALEN; i++) {
		mac[i] = simple_strtoul(s, &e, 16);
		if ((s == e) || (*e != ((i == ETH_ALEN-1) ? '\0' : ':'))) {
			kfree(macstr);
			return -ENOENT;
		}
		s = e+1;
	}

	kfree(macstr);
	return 0;
}

/* Common code used when first setting up, and when resuming. */
static int talk_to_backend(struct xenbus_device *dev,
			   struct netfront_info *info)
{
	const char *message;
	struct xenbus_transaction xbt;
	int err;

	/* Read mac only in the first setup. */
	if (!is_valid_ether_addr(info->mac)) {
		err = xen_net_read_mac(dev, info->mac);
		if (err) {
			xenbus_dev_fatal(dev, err, "parsing %s/mac",
					 dev->nodename);
			goto out;
		}
	}

	/* Create shared ring, alloc event channel. */
	err = setup_device(dev, info);
	if (err)
		goto out;

	/* This will load an accelerator if one is configured when the
	 * watch fires */
	netfront_accelerator_add_watch(info);

again:
	err = xenbus_transaction_start(&xbt);
	if (err) {
		xenbus_dev_fatal(dev, err, "starting transaction");
		goto destroy_ring;
	}

	err = xenbus_printf(xbt, dev->nodename, "tx-ring-ref","%u",
			    info->tx_ring_ref);
	if (err) {
		message = "writing tx ring-ref";
		goto abort_transaction;
	}
	err = xenbus_printf(xbt, dev->nodename, "rx-ring-ref","%u",
			    info->rx_ring_ref);
	if (err) {
		message = "writing rx ring-ref";
		goto abort_transaction;
	}
	err = xenbus_printf(xbt, dev->nodename,
			    "event-channel", "%u",
			    irq_to_evtchn_port(info->irq));
	if (err) {
		message = "writing event-channel";
		goto abort_transaction;
	}

	err = xenbus_printf(xbt, dev->nodename, "request-rx-copy", "%u",
			    info->copying_receiver);
	if (err) {
		message = "writing request-rx-copy";
		goto abort_transaction;
	}

	err = xenbus_write(xbt, dev->nodename, "feature-rx-notify", "1");
	if (err) {
		message = "writing feature-rx-notify";
		goto abort_transaction;
	}

	err = xenbus_write(xbt, dev->nodename, "feature-no-csum-offload",
			   __stringify(NO_CSUM_OFFLOAD));
	if (err) {
		message = "writing feature-no-csum-offload";
		goto abort_transaction;
	}

	err = xenbus_write(xbt, dev->nodename, "feature-sg", "1");
	if (err) {
		message = "writing feature-sg";
		goto abort_transaction;
	}

	err = xenbus_write(xbt, dev->nodename, "feature-gso-tcpv4",
			   __stringify(HAVE_TSO));
	if (err) {
		message = "writing feature-gso-tcpv4";
		goto abort_transaction;
	}

	err = xenbus_transaction_end(xbt, 0);
	if (err) {
		if (err == -EAGAIN)
			goto again;
		xenbus_dev_fatal(dev, err, "completing transaction");
		goto destroy_ring;
	}

	return 0;

 abort_transaction:
	xenbus_transaction_end(xbt, 1);
	xenbus_dev_fatal(dev, err, "%s", message);
 destroy_ring:
	netfront_accelerator_call_remove(info, dev);
	netif_disconnect_backend(info);
 out:
	return err;
}

static int setup_device(struct xenbus_device *dev, struct netfront_info *info)
{
	struct netif_tx_sring *txs;
	struct netif_rx_sring *rxs;
	int err;
	struct net_device *netdev = info->netdev;

	info->tx_ring_ref = GRANT_INVALID_REF;
	info->rx_ring_ref = GRANT_INVALID_REF;
	info->rx.sring = NULL;
	info->tx.sring = NULL;
	info->irq = 0;

	txs = (struct netif_tx_sring *)get_zeroed_page(GFP_NOIO | __GFP_HIGH);
	if (!txs) {
		err = -ENOMEM;
		xenbus_dev_fatal(dev, err, "allocating tx ring page");
		goto fail;
	}
	SHARED_RING_INIT(txs);
	FRONT_RING_INIT(&info->tx, txs, PAGE_SIZE);

	err = xenbus_grant_ring(dev, virt_to_mfn(txs));
	if (err < 0) {
		free_page((unsigned long)txs);
		goto fail;
	}
	info->tx_ring_ref = err;

	rxs = (struct netif_rx_sring *)get_zeroed_page(GFP_NOIO | __GFP_HIGH);
	if (!rxs) {
		err = -ENOMEM;
		xenbus_dev_fatal(dev, err, "allocating rx ring page");
		goto fail;
	}
	SHARED_RING_INIT(rxs);
	FRONT_RING_INIT(&info->rx, rxs, PAGE_SIZE);

	err = xenbus_grant_ring(dev, virt_to_mfn(rxs));
	if (err < 0) {
		free_page((unsigned long)rxs);
		goto fail;
	}
	info->rx_ring_ref = err;

	memcpy(netdev->dev_addr, info->mac, ETH_ALEN);

	err = bind_listening_port_to_irqhandler(
		dev->otherend_id, netif_int, 0, netdev->name, netdev);
	if (err < 0)
		goto fail;
	info->irq = err;

	return 0;

 fail:
	netif_release_rings(info);
	return err;
}

/**
 * Callback received when the backend's state changes.
 */
static void backend_changed(struct xenbus_device *dev,
			    enum xenbus_state backend_state)
{
	struct netfront_info *np = dev_get_drvdata(&dev->dev);
	struct net_device *netdev = np->netdev;

	DPRINTK("%s\n", xenbus_strstate(backend_state));

	switch (backend_state) {
	case XenbusStateInitialising:
	case XenbusStateInitialised:
	case XenbusStateReconfiguring:
	case XenbusStateReconfigured:
	case XenbusStateUnknown:
		break;

	case XenbusStateInitWait:
		if (dev->state != XenbusStateInitialising)
			break;
		if (network_connect(netdev) != 0)
			break;
		xenbus_switch_state(dev, XenbusStateConnected);
		break;

	case XenbusStateConnected:
		netdev_notify_peers(netdev);
		break;

	case XenbusStateClosed:
		if (dev->state == XenbusStateClosed)
			break;
		/* Missed the backend's CLOSING state -- fallthrough */
	case XenbusStateClosing:
		xenbus_frontend_closed(dev);
		break;
	}
}

static inline int netfront_tx_slot_available(struct netfront_info *np)
{
	return ((np->tx.req_prod_pvt - np->tx.rsp_cons) <
		(TX_MAX_TARGET - MAX_SKB_FRAGS - 2));
}


static inline void network_maybe_wake_tx(struct net_device *dev)
{
	struct netfront_info *np = netdev_priv(dev);

	if (unlikely(netif_queue_stopped(dev)) &&
	    netfront_tx_slot_available(np) &&
	    likely(netif_running(dev)) &&
	    netfront_check_accelerator_queue_ready(dev, np))
		netif_wake_queue(dev);
}


int netfront_check_queue_ready(struct net_device *dev)
{
	struct netfront_info *np = netdev_priv(dev);

	return unlikely(netif_queue_stopped(dev)) &&
		netfront_tx_slot_available(np) &&
		likely(netif_running(dev));
}
EXPORT_SYMBOL(netfront_check_queue_ready);

static int network_open(struct net_device *dev)
{
	struct netfront_info *np = netdev_priv(dev);

	napi_enable(&np->napi);

	spin_lock_bh(&np->rx_lock);
	if (netfront_carrier_ok(np)) {
		network_alloc_rx_buffers(dev);
		np->rx.sring->rsp_event = np->rx.rsp_cons + 1;
		if (RING_HAS_UNCONSUMED_RESPONSES(&np->rx)){
			netfront_accelerator_call_stop_napi_irq(np, dev);

			napi_schedule(&np->napi);
		}
	}
	spin_unlock_bh(&np->rx_lock);

	netif_start_queue(dev);

	return 0;
}

static void network_tx_buf_gc(struct net_device *dev)
{
	RING_IDX cons, prod;
	unsigned short id;
	struct netfront_info *np = netdev_priv(dev);
	struct sk_buff *skb;

	BUG_ON(!netfront_carrier_ok(np));

	do {
		prod = np->tx.sring->rsp_prod;
		rmb(); /* Ensure we see responses up to 'rp'. */

		for (cons = np->tx.rsp_cons; cons != prod; cons++) {
			struct netif_tx_response *txrsp;

			txrsp = RING_GET_RESPONSE(&np->tx, cons);
			if (txrsp->status == XEN_NETIF_RSP_NULL)
				continue;

			id  = txrsp->id;
			skb = np->tx_skbs[id];
			if (unlikely(gnttab_query_foreign_access(
				np->grant_tx_ref[id]) != 0)) {
				pr_alert("network_tx_buf_gc: grant still"
					 " in use by backend domain\n");
				BUG();
			}
			gnttab_end_foreign_access_ref(np->grant_tx_ref[id]);
			gnttab_release_grant_reference(
				&np->gref_tx_head, np->grant_tx_ref[id]);
			np->grant_tx_ref[id] = GRANT_INVALID_REF;
			add_id_to_freelist(np->tx_skbs, id);
			dev_kfree_skb_irq(skb);
		}

		np->tx.rsp_cons = prod;

		/*
		 * Set a new event, then check for race with update of tx_cons.
		 * Note that it is essential to schedule a callback, no matter
		 * how few buffers are pending. Even if there is space in the
		 * transmit ring, higher layers may be blocked because too much
		 * data is outstanding: in such cases notification from Xen is
		 * likely to be the only kick that we'll get.
		 */
		np->tx.sring->rsp_event =
			prod + ((np->tx.sring->req_prod - prod) >> 1) + 1;
		mb();
	} while ((cons == prod) && (prod != np->tx.sring->rsp_prod));

	network_maybe_wake_tx(dev);
}

static void rx_refill_timeout(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct netfront_info *np = netdev_priv(dev);

	netfront_accelerator_call_stop_napi_irq(np, dev);

	napi_schedule(&np->napi);
}

static void network_alloc_rx_buffers(struct net_device *dev)
{
	unsigned short id;
	struct netfront_info *np = netdev_priv(dev);
	struct sk_buff *skb;
	struct page *page;
	int i, batch_target, notify;
	RING_IDX req_prod = np->rx.req_prod_pvt;
	grant_ref_t ref;
 	unsigned long pfn;
 	void *vaddr;
	int nr_flips;
	netif_rx_request_t *req;

	if (unlikely(!netfront_carrier_ok(np)))
		return;

	/*
	 * Allocate skbuffs greedily, even though we batch updates to the
	 * receive ring. This creates a less bursty demand on the memory
	 * allocator, so should reduce the chance of failed allocation requests
	 * both for ourself and for other kernel subsystems.
	 */
	batch_target = np->rx_target - (req_prod - np->rx.rsp_cons);
	for (i = skb_queue_len(&np->rx_batch); i < batch_target; i++) {
		/*
		 * Allocate an skb and a page. Do not use __dev_alloc_skb as
		 * that will allocate page-sized buffers which is not
		 * necessary here.
		 * 16 bytes added as necessary headroom for netif_receive_skb.
		 */
		skb = alloc_skb(RX_COPY_THRESHOLD + 16 + NET_IP_ALIGN,
				GFP_ATOMIC | __GFP_NOWARN);
		if (unlikely(!skb))
			goto no_skb;

		page = alloc_page(GFP_ATOMIC | __GFP_NOWARN);
		if (!page) {
			kfree_skb(skb);
no_skb:
			/* Could not allocate enough skbuffs. Try again later. */
			mod_timer(&np->rx_refill_timer,
				  jiffies + (HZ/10));

			/* Any skbuffs queued for refill? Force them out. */
			if (i != 0)
				goto refill;
			break;
		}

		skb_reserve(skb, 16 + NET_IP_ALIGN); /* mimic dev_alloc_skb() */
		skb_add_rx_frag(skb, 0, page, 0, 0, PAGE_SIZE);
		__skb_queue_tail(&np->rx_batch, skb);
	}

	/* Is the batch large enough to be worthwhile? */
	if (i < (np->rx_target/2)) {
		if (req_prod > np->rx.sring->req_prod)
			goto push;
		return;
	}

	/* Adjust our fill target if we risked running out of buffers. */
	if (((req_prod - np->rx.sring->rsp_prod) < (np->rx_target / 4)) &&
	    ((np->rx_target *= 2) > np->rx_max_target))
		np->rx_target = np->rx_max_target;

 refill:
	for (nr_flips = i = 0; ; i++) {
		if ((skb = __skb_dequeue(&np->rx_batch)) == NULL)
			break;

		skb->dev = dev;

		id = xennet_rxidx(req_prod + i);

		BUG_ON(np->rx_skbs[id]);
		np->rx_skbs[id] = skb;

		ref = gnttab_claim_grant_reference(&np->gref_rx_head);
		BUG_ON((signed short)ref < 0);
		np->grant_rx_ref[id] = ref;

		page = skb_frag_page(skb_shinfo(skb)->frags);
		pfn = page_to_pfn(page);
		vaddr = page_address(page);

		req = RING_GET_REQUEST(&np->rx, req_prod + i);
		if (!np->copying_receiver) {
			gnttab_grant_foreign_transfer_ref(ref,
							  np->xbdev->otherend_id,
							  pfn);
			np->rx_pfn_array[nr_flips] = pfn_to_mfn(pfn);
			if (!xen_feature(XENFEAT_auto_translated_physmap)) {
				/* Remove this page before passing
				 * back to Xen. */
				set_phys_to_machine(pfn, INVALID_P2M_ENTRY);
				MULTI_update_va_mapping(np->rx_mcl+i,
							(unsigned long)vaddr,
							__pte(0), 0);
			}
			nr_flips++;
		} else {
			gnttab_grant_foreign_access_ref(ref,
							np->xbdev->otherend_id,
							pfn_to_mfn(pfn),
							0);
		}

		req->id = id;
		req->gref = ref;
	}

	if (nr_flips) {
		struct xen_memory_reservation reservation = {
			.nr_extents = nr_flips,
			.domid      = DOMID_SELF,
		};

		/* Tell the ballon driver what is going on. */
		balloon_update_driver_allowance(i);

		set_xen_guest_handle(reservation.extent_start,
				     np->rx_pfn_array);

		if (!xen_feature(XENFEAT_auto_translated_physmap)) {
			/* After all PTEs have been zapped, flush the TLB. */
			np->rx_mcl[i-1].args[MULTI_UVMFLAGS_INDEX] =
				UVMF_TLB_FLUSH|UVMF_ALL;

			/* Give away a batch of pages. */
			MULTI_memory_op(np->rx_mcl + i,
					XENMEM_decrease_reservation,
					&reservation);

			/* Zap PTEs and give away pages in one big
			 * multicall. */
			if (unlikely(HYPERVISOR_multicall(np->rx_mcl, i+1)))
				BUG();

			/* Check return status of HYPERVISOR_memory_op(). */
			if (unlikely(np->rx_mcl[i].result != i))
				panic("Unable to reduce memory reservation\n");
			while (nr_flips--)
				BUG_ON(np->rx_mcl[nr_flips].result);
		} else {
			if (HYPERVISOR_memory_op(XENMEM_decrease_reservation,
						 &reservation) != i)
				panic("Unable to reduce memory reservation\n");
		}
	} else {
		wmb();
	}

	/* Above is a suitable barrier to ensure backend will see requests. */
	np->rx.req_prod_pvt = req_prod + i;
 push:
	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&np->rx, notify);
	if (notify)
		notify_remote_via_irq(np->irq);
}

static void xennet_make_frags(struct sk_buff *skb, struct net_device *dev,
			      struct netif_tx_request *tx)
{
	struct netfront_info *np = netdev_priv(dev);
	char *data = skb->data;
	unsigned long mfn;
	RING_IDX prod = np->tx.req_prod_pvt;
	int frags = skb_shinfo(skb)->nr_frags;
	unsigned int offset = offset_in_page(data);
	unsigned int len = skb_headlen(skb);
	unsigned int id;
	grant_ref_t ref;
	int i;

	while (len > PAGE_SIZE - offset) {
		tx->size = PAGE_SIZE - offset;
		tx->flags |= XEN_NETTXF_more_data;
		len -= tx->size;
		data += tx->size;
		offset = 0;

		id = get_id_from_freelist(np->tx_skbs);
		np->tx_skbs[id] = skb_get(skb);
		tx = RING_GET_REQUEST(&np->tx, prod++);
		tx->id = id;
		ref = gnttab_claim_grant_reference(&np->gref_tx_head);
		BUG_ON((signed short)ref < 0);

		mfn = virt_to_mfn(data);
		gnttab_grant_foreign_access_ref(ref, np->xbdev->otherend_id,
						mfn, GTF_readonly);

		tx->gref = np->grant_tx_ref[id] = ref;
		tx->offset = offset;
		tx->size = len;
		tx->flags = 0;
	}

	for (i = 0; i < frags; i++) {
		skb_frag_t *frag = skb_shinfo(skb)->frags + i;
		struct page *page = skb_frag_page(frag);

		len = skb_frag_size(frag);
		offset = frag->page_offset;

		/* Data must not cross a page boundary. */
		BUG_ON(offset + len > (PAGE_SIZE << compound_order(page)));

		/* Skip unused frames from start of page */
		page += PFN_DOWN(offset);
		for (offset &= ~PAGE_MASK; len; page++, offset = 0) {
			unsigned int bytes = PAGE_SIZE - offset;

			if (bytes > len)
				bytes = len;

			tx->flags |= XEN_NETTXF_more_data;

			id = get_id_from_freelist(np->tx_skbs);
			np->tx_skbs[id] = skb_get(skb);
			tx = RING_GET_REQUEST(&np->tx, prod++);
			tx->id = id;
			ref = gnttab_claim_grant_reference(&np->gref_tx_head);
			BUG_ON((signed short)ref < 0);

			mfn = pfn_to_mfn(page_to_pfn(page));
			gnttab_grant_foreign_access_ref(ref,
							np->xbdev->otherend_id,
							mfn, GTF_readonly);

			tx->gref = np->grant_tx_ref[id] = ref;
			tx->offset = offset;
			tx->size = bytes;
			tx->flags = 0;

			len -= bytes;
		}
	}

	np->tx.req_prod_pvt = prod;
}

/*
 * Count how many ring slots are required to send the frags of this
 * skb. Each frag might be a compound page.
 */
static unsigned int xennet_count_skb_frag_slots(const struct sk_buff *skb)
{
	unsigned int i, pages, frags = skb_shinfo(skb)->nr_frags;

	for (pages = i = 0; i < frags; i++) {
		const skb_frag_t *frag = skb_shinfo(skb)->frags + i;
		unsigned int len = skb_frag_size(frag);

		if (!len)
			continue;
		pages += PFN_UP((frag->page_offset & ~PAGE_MASK) + len);
	}

	return pages;
}

static int network_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	unsigned short id;
	struct netfront_info *np = netdev_priv(dev);
	struct netfront_stats *stats = this_cpu_ptr(np->tx_stats);
	struct netif_tx_request *tx;
	struct netif_extra_info *extra;
	char *data = skb->data;
	RING_IDX i;
	grant_ref_t ref;
	unsigned long mfn, flags;
	int notify;
	unsigned int offset = offset_in_page(data);
	unsigned int slots, len = skb_headlen(skb);

	/* Check the fast path, if hooks are available */
 	if (np->accel_vif_state.hooks && 
 	    np->accel_vif_state.hooks->start_xmit(skb, dev)) { 
 		/* Fast path has sent this packet */ 
		return NETDEV_TX_OK;
 	} 

	/*
	 * If skb->len is too big for wire format, drop skb and alert
	 * user about misconfiguration.
	 */
	if (unlikely(skb->len > XEN_NETIF_MAX_TX_SIZE)) {
		net_alert_ratelimited("xennet: length %u too big for interface\n",
				      skb->len);
		goto drop;
	}

	slots = PFN_UP(offset + len) + xennet_count_skb_frag_slots(skb);
	if (unlikely(slots > MAX_SKB_FRAGS + 1)) {
		net_alert_ratelimited("xennet: skb rides the rocket: %u slots\n",
				      slots);
		goto drop;
	}

	spin_lock_irqsave(&np->tx_lock, flags);

	if (unlikely(!netfront_carrier_ok(np) ||
		     (slots > 1 && !xennet_can_sg(dev)) ||
		     netif_needs_gso(skb, netif_skb_features(skb)))) {
		spin_unlock_irqrestore(&np->tx_lock, flags);
		goto drop;
	}

	i = np->tx.req_prod_pvt;

	id = get_id_from_freelist(np->tx_skbs);
	np->tx_skbs[id] = skb;

	tx = RING_GET_REQUEST(&np->tx, i);

	tx->id   = id;
	ref = gnttab_claim_grant_reference(&np->gref_tx_head);
	BUG_ON((signed short)ref < 0);
	mfn = virt_to_mfn(data);
	gnttab_grant_foreign_access_ref(
		ref, np->xbdev->otherend_id, mfn, GTF_readonly);
	tx->gref = np->grant_tx_ref[id] = ref;
	tx->offset = offset;
	tx->size = len;

	tx->flags = 0;
	extra = NULL;

	if (skb->ip_summed == CHECKSUM_PARTIAL) /* local packet? */
		tx->flags |= XEN_NETTXF_csum_blank | XEN_NETTXF_data_validated;
	else if (skb->ip_summed == CHECKSUM_UNNECESSARY)
		tx->flags |= XEN_NETTXF_data_validated;

#if HAVE_TSO
	if (skb_is_gso(skb)) {
		struct netif_extra_info *gso = (struct netif_extra_info *)
			RING_GET_REQUEST(&np->tx, ++i);

		if (extra)
			extra->flags |= XEN_NETIF_EXTRA_FLAG_MORE;
		else
			tx->flags |= XEN_NETTXF_extra_info;

		gso->u.gso.size = skb_shinfo(skb)->gso_size;
		gso->u.gso.type = XEN_NETIF_GSO_TYPE_TCPV4;
		gso->u.gso.pad = 0;
		gso->u.gso.features = 0;

		gso->type = XEN_NETIF_EXTRA_TYPE_GSO;
		gso->flags = 0;
		extra = gso;
	}
#endif

	np->tx.req_prod_pvt = i + 1;

	xennet_make_frags(skb, dev, tx);
	tx->size = skb->len;

	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&np->tx, notify);
	if (notify)
		notify_remote_via_irq(np->irq);

	u64_stats_update_begin(&stats->syncp);
	stats->bytes += skb->len;
	stats->packets++;
	u64_stats_update_end(&stats->syncp);
	dev->trans_start = jiffies;

	/* Note: It is not safe to access skb after network_tx_buf_gc()! */
	network_tx_buf_gc(dev);

	if (!netfront_tx_slot_available(np))
		netif_stop_queue(dev);

	spin_unlock_irqrestore(&np->tx_lock, flags);

	return NETDEV_TX_OK;

 drop:
	dev->stats.tx_dropped++;
	dev_kfree_skb(skb);
	return NETDEV_TX_OK;
}

static irqreturn_t netif_int(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct netfront_info *np = netdev_priv(dev);
	unsigned long flags;

	spin_lock_irqsave(&np->tx_lock, flags);

	if (likely(netfront_carrier_ok(np))) {
		network_tx_buf_gc(dev);
		/* Under tx_lock: protects access to rx shared-ring indexes. */
		if (RING_HAS_UNCONSUMED_RESPONSES(&np->rx)) {
			netfront_accelerator_call_stop_napi_irq(np, dev);

			napi_schedule(&np->napi);
		}
	}

	spin_unlock_irqrestore(&np->tx_lock, flags);

	return IRQ_HANDLED;
}

static void xennet_move_rx_slot(struct netfront_info *np, struct sk_buff *skb,
				grant_ref_t ref)
{
	int new = xennet_rxidx(np->rx.req_prod_pvt);

	BUG_ON(np->rx_skbs[new]);
	np->rx_skbs[new] = skb;
	np->grant_rx_ref[new] = ref;
	RING_GET_REQUEST(&np->rx, np->rx.req_prod_pvt)->id = new;
	RING_GET_REQUEST(&np->rx, np->rx.req_prod_pvt)->gref = ref;
	np->rx.req_prod_pvt++;
}

static int xennet_get_extras(struct netfront_info *np,
			     struct netif_extra_info *extras, RING_IDX rp)
{
	struct netif_extra_info *extra;
	RING_IDX cons = np->rx.rsp_cons;
	int err = 0;

	do {
		struct sk_buff *skb;
		grant_ref_t ref;

		if (unlikely(cons + 1 == rp)) {
			if (net_ratelimit())
				netdev_warn(np->netdev,
					    "Missing extra info\n");
			err = -EBADR;
			break;
		}

		extra = (struct netif_extra_info *)
			RING_GET_RESPONSE(&np->rx, ++cons);

		if (unlikely(!extra->type ||
			     extra->type >= XEN_NETIF_EXTRA_TYPE_MAX)) {
			if (net_ratelimit())
				netdev_warn(np->netdev,
					    "Invalid extra type: %d\n",
					    extra->type);
			err = -EINVAL;
		} else {
			memcpy(&extras[extra->type - 1], extra,
			       sizeof(*extra));
		}

		skb = xennet_get_rx_skb(np, cons);
		ref = xennet_get_rx_ref(np, cons);
		xennet_move_rx_slot(np, skb, ref);
	} while (extra->flags & XEN_NETIF_EXTRA_FLAG_MORE);

	np->rx.rsp_cons = cons;
	return err;
}

static int xennet_get_responses(struct netfront_info *np,
				struct netfront_rx_info *rinfo, RING_IDX rp,
				struct sk_buff_head *list,
				int *pages_flipped_p)
{
	int pages_flipped = *pages_flipped_p;
	struct mmu_update *mmu;
	struct multicall_entry *mcl;
	struct netif_rx_response *rx = &rinfo->rx;
	struct netif_extra_info *extras = rinfo->extras;
	RING_IDX cons = np->rx.rsp_cons;
	struct sk_buff *skb = xennet_get_rx_skb(np, cons);
	grant_ref_t ref = xennet_get_rx_ref(np, cons);
	int max = MAX_SKB_FRAGS + (rx->status <= RX_COPY_THRESHOLD);
	int frags = 1;
	int err = 0;
	unsigned long ret;

	if (rx->flags & XEN_NETRXF_extra_info) {
		err = xennet_get_extras(np, extras, rp);
		cons = np->rx.rsp_cons;
	}

	for (;;) {
		unsigned long mfn;

		if (unlikely(rx->status < 0 ||
			     rx->offset + rx->status > PAGE_SIZE)) {
			if (net_ratelimit())
				netdev_warn(np->netdev,
					    "rx->offset: %#x, size: %d\n",
					    rx->offset, rx->status);
			xennet_move_rx_slot(np, skb, ref);
			err = -EINVAL;
			goto next;
		}

		/*
		 * This definitely indicates a bug, either in this driver or in
		 * the backend driver. In future this should flag the bad
		 * situation to the system controller to reboot the backed.
		 */
		if (ref == GRANT_INVALID_REF) {
			if (net_ratelimit())
				netdev_warn(np->netdev,
					    "Bad rx response id %d.\n",
					    rx->id);
			err = -EINVAL;
			goto next;
		}

		if (!np->copying_receiver) {
			/* Memory pressure, insufficient buffer
			 * headroom, ... */
			if (!(mfn = gnttab_end_foreign_transfer_ref(ref))) {
				if (net_ratelimit())
					netdev_warn(np->netdev,
						    "Unfulfilled rx req (id=%d, st=%d).\n",
						    rx->id, rx->status);
				xennet_move_rx_slot(np, skb, ref);
				err = -ENOMEM;
				goto next;
			}

			if (!xen_feature(XENFEAT_auto_translated_physmap)) {
				/* Remap the page. */
				const struct page *page =
					skb_frag_page(skb_shinfo(skb)->frags);
				unsigned long pfn = page_to_pfn(page);
				void *vaddr = page_address(page);

				mcl = np->rx_mcl + pages_flipped;
				mmu = np->rx_mmu + pages_flipped;

				MULTI_update_va_mapping(mcl,
							(unsigned long)vaddr,
							pfn_pte_ma(mfn,
								   PAGE_KERNEL),
							0);
				mmu->ptr = ((maddr_t)mfn << PAGE_SHIFT)
					| MMU_MACHPHYS_UPDATE;
				mmu->val = pfn;

				set_phys_to_machine(pfn, mfn);
			}
			pages_flipped++;
		} else {
			ret = gnttab_end_foreign_access_ref(ref);
			BUG_ON(!ret);
		}

		gnttab_release_grant_reference(&np->gref_rx_head, ref);

		__skb_queue_tail(list, skb);

next:
		if (!(rx->flags & XEN_NETRXF_more_data))
			break;

		if (cons + frags == rp) {
			if (net_ratelimit())
				netdev_warn(np->netdev, "Need more frags\n");
			err = -ENOENT;
			break;
		}

		rx = RING_GET_RESPONSE(&np->rx, cons + frags);
		skb = xennet_get_rx_skb(np, cons + frags);
		ref = xennet_get_rx_ref(np, cons + frags);
		frags++;
	}

	if (unlikely(frags > max)) {
		if (net_ratelimit())
			netdev_warn(np->netdev, "Too many frags\n");
		err = -E2BIG;
	}

	if (unlikely(err))
		np->rx.rsp_cons = cons + frags;

	*pages_flipped_p = pages_flipped;

	return err;
}

static RING_IDX xennet_fill_frags(struct netfront_info *np,
				  struct sk_buff *skb,
				  struct sk_buff_head *list)
{
	struct skb_shared_info *shinfo = skb_shinfo(skb);
	RING_IDX cons = np->rx.rsp_cons;
	struct sk_buff *nskb;

	while ((nskb = __skb_dequeue(list))) {
		struct netif_rx_response *rx =
			RING_GET_RESPONSE(&np->rx, ++cons);

		if (shinfo->nr_frags == MAX_SKB_FRAGS) {
			unsigned int pull_to = NETFRONT_SKB_CB(skb)->pull_to;

			BUG_ON(pull_to <= skb_headlen(skb));
			__pskb_pull_tail(skb, pull_to - skb_headlen(skb));
		}
		BUG_ON(shinfo->nr_frags >= MAX_SKB_FRAGS);

		skb_add_rx_frag(skb, shinfo->nr_frags,
				skb_frag_page(skb_shinfo(nskb)->frags),
				rx->offset, rx->status, PAGE_SIZE);

		skb_shinfo(nskb)->nr_frags = 0;
		kfree_skb(nskb);
	}

	return cons;
}

static int xennet_set_skb_gso(struct sk_buff *skb,
			      struct netif_extra_info *gso)
{
	if (!gso->u.gso.size) {
		if (net_ratelimit())
			netdev_warn(skb->dev,
				    "GSO size must not be zero.\n");
		return -EINVAL;
	}

	/* Currently only TCPv4 S.O. is supported. */
	if (gso->u.gso.type != XEN_NETIF_GSO_TYPE_TCPV4) {
		if (net_ratelimit())
			netdev_warn(skb->dev, "Bad GSO type %d.\n",
				    gso->u.gso.type);
		return -EINVAL;
	}

#if HAVE_TSO
	skb_shinfo(skb)->gso_size = gso->u.gso.size;
#if HAVE_GSO
	skb_shinfo(skb)->gso_type = SKB_GSO_TCPV4;

	/* Header must be checked, and gso_segs computed. */
	skb_shinfo(skb)->gso_type |= SKB_GSO_DODGY;
#endif
	skb_shinfo(skb)->gso_segs = 0;

	return 0;
#else
	if (net_ratelimit())
		netdev_warn(skb->dev, "GSO unsupported by this kernel.\n");
	return -EINVAL;
#endif
}

static int netif_poll(struct napi_struct *napi, int budget)
{
	struct netfront_info *np = container_of(napi, struct netfront_info, napi);
	struct netfront_stats *stats = this_cpu_ptr(np->rx_stats);
	struct net_device *dev = np->netdev;
	struct sk_buff *skb;
	struct netfront_rx_info rinfo;
	struct netif_rx_response *rx = &rinfo.rx;
	struct netif_extra_info *extras = rinfo.extras;
	RING_IDX i, rp;
	int work_done, more_to_do = 1, accel_more_to_do = 1;
	struct sk_buff_head rxq;
	struct sk_buff_head errq;
	struct sk_buff_head tmpq;
	unsigned long flags;
	int pages_flipped = 0;
	int err;

	spin_lock(&np->rx_lock); /* no need for spin_lock_bh() in ->poll() */

	if (unlikely(!netfront_carrier_ok(np))) {
		spin_unlock(&np->rx_lock);
		return 0;
	}

	skb_queue_head_init(&rxq);
	skb_queue_head_init(&errq);
	skb_queue_head_init(&tmpq);

	rp = np->rx.sring->rsp_prod;
	rmb(); /* Ensure we see queued responses up to 'rp'. */

	i = np->rx.rsp_cons;
	work_done = 0;
	while ((i != rp) && (work_done < budget)) {
		memcpy(rx, RING_GET_RESPONSE(&np->rx, i), sizeof(*rx));
		memset(extras, 0, sizeof(rinfo.extras));

		err = xennet_get_responses(np, &rinfo, rp, &tmpq,
					   &pages_flipped);

		if (unlikely(err)) {
err:	
			while ((skb = __skb_dequeue(&tmpq)))
				__skb_queue_tail(&errq, skb);
			dev->stats.rx_errors++;
			i = np->rx.rsp_cons;
			continue;
		}

		skb = __skb_dequeue(&tmpq);

		if (extras[XEN_NETIF_EXTRA_TYPE_GSO - 1].type) {
			struct netif_extra_info *gso;
			gso = &extras[XEN_NETIF_EXTRA_TYPE_GSO - 1];

			if (unlikely(xennet_set_skb_gso(skb, gso))) {
				__skb_queue_head(&tmpq, skb);
				np->rx.rsp_cons += skb_queue_len(&tmpq);
				goto err;
			}
		}

		NETFRONT_SKB_CB(skb)->pull_to = rx->status;
		if (NETFRONT_SKB_CB(skb)->pull_to > RX_COPY_THRESHOLD)
			NETFRONT_SKB_CB(skb)->pull_to = RX_COPY_THRESHOLD;

		skb_shinfo(skb)->frags[0].page_offset = rx->offset;
		skb_frag_size_set(skb_shinfo(skb)->frags, rx->status);
		skb->len += skb->data_len = rx->status;

		i = xennet_fill_frags(np, skb, &tmpq);

		if (rx->flags & XEN_NETRXF_csum_blank)
			skb->ip_summed = CHECKSUM_PARTIAL;
		else if (rx->flags & XEN_NETRXF_data_validated)
			skb->ip_summed = CHECKSUM_UNNECESSARY;
		else
			skb->ip_summed = CHECKSUM_NONE;

		__skb_queue_tail(&rxq, skb);

		np->rx.rsp_cons = ++i;
		work_done++;
	}

	if (pages_flipped) {
		/* Some pages are no longer absent... */
		balloon_update_driver_allowance(-pages_flipped);

		/* Do all the remapping work and M2P updates. */
		if (!xen_feature(XENFEAT_auto_translated_physmap)) {
			MULTI_mmu_update(np->rx_mcl + pages_flipped,
					 np->rx_mmu, pages_flipped, 0,
					 DOMID_SELF);
			err = HYPERVISOR_multicall_check(np->rx_mcl,
							 pages_flipped + 1,
							 NULL);
			BUG_ON(err);
		}
	}

	__skb_queue_purge(&errq);

	while ((skb = __skb_dequeue(&rxq)) != NULL) {
		unsigned int pull_to = NETFRONT_SKB_CB(skb)->pull_to;

		if (pull_to > skb_headlen(skb))
			__pskb_pull_tail(skb, pull_to - skb_headlen(skb));

		/* Ethernet work: Delayed to here as it peeks the header. */
		skb->protocol = eth_type_trans(skb, dev);

		if (skb_checksum_setup(skb, &np->rx_gso_csum_fixups)) {
			kfree_skb(skb);
			dev->stats.rx_errors++;
			--work_done;
			continue;
		}

		u64_stats_update_begin(&stats->syncp);
		stats->packets++;
		stats->bytes += skb->len;
		u64_stats_update_end(&stats->syncp);

		/* Pass it up. */
		netif_receive_skb(skb);
	}

	/* If we get a callback with very few responses, reduce fill target. */
	/* NB. Note exponential increase, linear decrease. */
	if (((np->rx.req_prod_pvt - np->rx.sring->rsp_prod) >
	     ((3*np->rx_target) / 4)) &&
	    (--np->rx_target < np->rx_min_target))
		np->rx_target = np->rx_min_target;

	network_alloc_rx_buffers(dev);

	if (work_done < budget) {
		/* there's some spare capacity, try the accelerated path */
		int accel_budget = budget - work_done;
		int accel_budget_start = accel_budget;

 		if (np->accel_vif_state.hooks) { 
 			accel_more_to_do =  
 				np->accel_vif_state.hooks->netdev_poll 
 				(dev, &accel_budget); 
 			work_done += (accel_budget_start - accel_budget); 
 		} else
			accel_more_to_do = 0;
	}

	if (work_done < budget) {
		local_irq_save(flags);

		RING_FINAL_CHECK_FOR_RESPONSES(&np->rx, more_to_do);

		if (!more_to_do && !accel_more_to_do && 
		    np->accel_vif_state.hooks) {
			/* 
			 *  Slow path has nothing more to do, see if
			 *  fast path is likewise
			 */
			accel_more_to_do = 
				np->accel_vif_state.hooks->start_napi_irq(dev);
		}

		if (!more_to_do && !accel_more_to_do)
			__napi_complete(napi);

		local_irq_restore(flags);
	}

	spin_unlock(&np->rx_lock);
	
	return work_done;
}

static void netif_release_tx_bufs(struct netfront_info *np)
{
	struct sk_buff *skb;
	int i;

	for (i = 1; i <= NET_TX_RING_SIZE; i++) {
		if ((unsigned long)np->tx_skbs[i] < PAGE_OFFSET)
			continue;

		skb = np->tx_skbs[i];
		gnttab_end_foreign_access_ref(np->grant_tx_ref[i]);
		gnttab_release_grant_reference(
			&np->gref_tx_head, np->grant_tx_ref[i]);
		np->grant_tx_ref[i] = GRANT_INVALID_REF;
		add_id_to_freelist(np->tx_skbs, i);
		dev_kfree_skb_irq(skb);
	}
}

static void netif_release_rx_bufs_flip(struct netfront_info *np)
{
	struct mmu_update      *mmu = np->rx_mmu;
	struct multicall_entry *mcl = np->rx_mcl;
	struct sk_buff_head free_list;
	struct sk_buff *skb;
	unsigned long mfn;
	int xfer = 0, noxfer = 0, unused = 0;
	int id, ref, rc;

	skb_queue_head_init(&free_list);

	spin_lock_bh(&np->rx_lock);

	for (id = 0; id < NET_RX_RING_SIZE; id++) {
		struct page *page;

		if ((ref = np->grant_rx_ref[id]) == GRANT_INVALID_REF) {
			unused++;
			continue;
		}

		skb = np->rx_skbs[id];
		mfn = gnttab_end_foreign_transfer_ref(ref);
		gnttab_release_grant_reference(&np->gref_rx_head, ref);
		np->grant_rx_ref[id] = GRANT_INVALID_REF;
		add_id_to_freelist(np->rx_skbs, id);

		page = skb_frag_page(skb_shinfo(skb)->frags);

		if (0 == mfn) {
			balloon_release_driver_page(page);
			skb_shinfo(skb)->nr_frags = 0;
			dev_kfree_skb(skb);
			noxfer++;
			continue;
		}

		if (!xen_feature(XENFEAT_auto_translated_physmap)) {
			/* Remap the page. */
			unsigned long pfn = page_to_pfn(page);
			void *vaddr = page_address(page);

			MULTI_update_va_mapping(mcl, (unsigned long)vaddr,
						pfn_pte_ma(mfn, PAGE_KERNEL),
						0);
			mcl++;
			mmu->ptr = ((maddr_t)mfn << PAGE_SHIFT)
				| MMU_MACHPHYS_UPDATE;
			mmu->val = pfn;
			mmu++;

			set_phys_to_machine(pfn, mfn);
		}
		__skb_queue_tail(&free_list, skb);
		xfer++;
	}

	DPRINTK("%s: %d xfer, %d noxfer, %d unused\n",
		__FUNCTION__, xfer, noxfer, unused);

	if (xfer) {
		/* Some pages are no longer absent... */
		balloon_update_driver_allowance(-xfer);

		if (!xen_feature(XENFEAT_auto_translated_physmap)) {
			/* Do all the remapping work and M2P updates. */
			MULTI_mmu_update(mcl, np->rx_mmu, mmu - np->rx_mmu,
					 0, DOMID_SELF);
			rc = HYPERVISOR_multicall_check(
				np->rx_mcl, mcl + 1 - np->rx_mcl, NULL);
			BUG_ON(rc);
		}
	}

	__skb_queue_purge(&free_list);

	spin_unlock_bh(&np->rx_lock);
}

static void netif_release_rx_bufs_copy(struct netfront_info *np)
{
	struct sk_buff *skb;
	int i, ref;
	int busy = 0, inuse = 0;

	spin_lock_bh(&np->rx_lock);

	for (i = 0; i < NET_RX_RING_SIZE; i++) {
		ref = np->grant_rx_ref[i];

		if (ref == GRANT_INVALID_REF)
			continue;

		inuse++;

		skb = np->rx_skbs[i];

		if (!gnttab_end_foreign_access_ref(ref))
		{
			busy++;
			continue;
		}

		gnttab_release_grant_reference(&np->gref_rx_head, ref);
		np->grant_rx_ref[i] = GRANT_INVALID_REF;
		add_id_to_freelist(np->rx_skbs, i);

		dev_kfree_skb(skb);
	}

	if (busy)
		DPRINTK("%s: Unable to release %d of %d inuse grant references out of %ld total.\n",
			__FUNCTION__, busy, inuse, NET_RX_RING_SIZE);

	spin_unlock_bh(&np->rx_lock);
}

static int network_close(struct net_device *dev)
{
	struct netfront_info *np = netdev_priv(dev);
	netif_stop_queue(np->netdev);
	napi_disable(&np->napi);
	return 0;
}


static int xennet_set_mac_address(struct net_device *dev, void *p)
{
	struct netfront_info *np = netdev_priv(dev);
	struct sockaddr *addr = p;

	if (netif_running(dev))
		return -EBUSY;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
	memcpy(np->mac, addr->sa_data, ETH_ALEN);

	return 0;
}

static int xennet_change_mtu(struct net_device *dev, int mtu)
{
	int max = xennet_can_sg(dev) ? XEN_NETIF_MAX_TX_SIZE - MAX_TCP_HEADER
				     : ETH_DATA_LEN;

	if (mtu > max)
		return -EINVAL;
	dev->mtu = mtu;
	return 0;
}

static struct rtnl_link_stats64 *xennet_get_stats64(struct net_device *dev,
						    struct rtnl_link_stats64 *tot)
{
	struct netfront_info *np = netdev_priv(dev);
	int cpu;

	netfront_accelerator_call_get_stats(np, dev);

	for_each_possible_cpu(cpu) {
		struct netfront_stats *stats = per_cpu_ptr(np->rx_stats, cpu);
		u64 packets, bytes;
		unsigned int start;

		do {
			start = u64_stats_fetch_begin_bh(&stats->syncp);
			packets = stats->packets;
			bytes = stats->bytes;
		} while (u64_stats_fetch_retry_bh(&stats->syncp, start));

		tot->rx_packets += packets;
		tot->rx_bytes   += bytes;

		stats = per_cpu_ptr(np->tx_stats, cpu);

		do {
			start = u64_stats_fetch_begin_bh(&stats->syncp);
			packets = stats->packets;
			bytes = stats->bytes;
		} while (u64_stats_fetch_retry_bh(&stats->syncp, start));

		tot->tx_packets += packets;
		tot->tx_bytes   += bytes;
	}

	tot->rx_errors  = dev->stats.rx_errors;
	tot->tx_dropped = dev->stats.tx_dropped;

	return tot;
}

static const struct xennet_stat {
	char name[ETH_GSTRING_LEN];
	u16 offset;
} xennet_stats[] = {
	{
		"rx_gso_csum_fixups",
		offsetof(struct netfront_info, rx_gso_csum_fixups) / sizeof(long)
	},
};

static int xennet_get_sset_count(struct net_device *dev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return ARRAY_SIZE(xennet_stats);
	}
	return -EOPNOTSUPP;
}

static void xennet_get_ethtool_stats(struct net_device *dev,
				     struct ethtool_stats *stats, u64 *data)
{
	unsigned long *np = netdev_priv(dev);
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(xennet_stats); i++)
		data[i] = np[xennet_stats[i].offset];
}

static void xennet_get_strings(struct net_device *dev, u32 stringset, u8 *data)
{
	unsigned int i;

	switch (stringset) {
	case ETH_SS_STATS:
		for (i = 0; i < ARRAY_SIZE(xennet_stats); i++)
			memcpy(data + i * ETH_GSTRING_LEN,
			       xennet_stats[i].name, ETH_GSTRING_LEN);
		break;
	}
}

static void netfront_get_drvinfo(struct net_device *dev,
				 struct ethtool_drvinfo *info)
{
	strcpy(info->driver, "netfront");
	strlcpy(info->bus_info, dev_name(dev->dev.parent),
		ARRAY_SIZE(info->bus_info));
}

static int network_connect(struct net_device *dev)
{
	struct netfront_info *np = netdev_priv(dev);
	int i, requeue_idx, err;
	struct sk_buff *skb;
	grant_ref_t ref;
	netif_rx_request_t *req;
	unsigned int feature_rx_copy, feature_rx_flip;

	err = xenbus_scanf(XBT_NIL, np->xbdev->otherend,
			   "feature-rx-copy", "%u", &feature_rx_copy);
	if (err != 1)
		feature_rx_copy = 0;
	err = xenbus_scanf(XBT_NIL, np->xbdev->otherend,
			   "feature-rx-flip", "%u", &feature_rx_flip);
	if (err != 1)
		feature_rx_flip = 1;

	/*
	 * Copy packets on receive path if:
	 *  (a) This was requested by user, and the backend supports it; or
	 *  (b) Flipping was requested, but this is unsupported by the backend.
	 */
	np->copying_receiver = ((MODPARM_rx_copy && feature_rx_copy) ||
				(MODPARM_rx_flip && !feature_rx_flip));

	err = talk_to_backend(np->xbdev, np);
	if (err)
		return err;

	rtnl_lock();
	netdev_update_features(dev);
	rtnl_unlock();

	DPRINTK("device %s has %sing receive path.\n",
		dev->name, np->copying_receiver ? "copy" : "flipp");

	spin_lock_bh(&np->rx_lock);
	spin_lock_irq(&np->tx_lock);

	/*
	 * Recovery procedure:
	 *  NB. Freelist index entries are always going to be less than
	 *  PAGE_OFFSET, whereas pointers to skbs will always be equal or
	 *  greater than PAGE_OFFSET: we use this property to distinguish
	 *  them.
	 */

	/* Step 1: Discard all pending TX packet fragments. */
	netif_release_tx_bufs(np);

	/* Step 2: Rebuild the RX buffer freelist and the RX ring itself. */
	for (requeue_idx = 0, i = 0; i < NET_RX_RING_SIZE; i++) {
		unsigned long pfn;

		if (!np->rx_skbs[i])
			continue;

		skb = np->rx_skbs[requeue_idx] = xennet_get_rx_skb(np, i);
		ref = np->grant_rx_ref[requeue_idx] = xennet_get_rx_ref(np, i);
		req = RING_GET_REQUEST(&np->rx, requeue_idx);
		pfn = page_to_pfn(skb_frag_page(skb_shinfo(skb)->frags));

		if (!np->copying_receiver) {
			gnttab_grant_foreign_transfer_ref(
				ref, np->xbdev->otherend_id, pfn);
		} else {
			gnttab_grant_foreign_access_ref(
				ref, np->xbdev->otherend_id,
				pfn_to_mfn(pfn), 0);
		}
		req->gref = ref;
		req->id   = requeue_idx;

		requeue_idx++;
	}

	np->rx.req_prod_pvt = requeue_idx;

	/*
	 * Step 3: All public and private state should now be sane.  Get
	 * ready to start sending and receiving packets and give the driver
	 * domain a kick because we've probably just requeued some
	 * packets.
	 */
	netfront_carrier_on(np);
	notify_remote_via_irq(np->irq);
	network_tx_buf_gc(dev);
	network_alloc_rx_buffers(dev);

	spin_unlock_irq(&np->tx_lock);
	spin_unlock_bh(&np->rx_lock);

	return 0;
}

static void netif_uninit(struct net_device *dev)
{
	struct netfront_info *np = netdev_priv(dev);
	netif_release_tx_bufs(np);
	if (np->copying_receiver)
		netif_release_rx_bufs_copy(np);
	else
		netif_release_rx_bufs_flip(np);
	gnttab_free_grant_references(np->gref_tx_head);
	gnttab_free_grant_references(np->gref_rx_head);
}

static const struct ethtool_ops network_ethtool_ops =
{
	.get_drvinfo = netfront_get_drvinfo,
	.get_link = ethtool_op_get_link,

	.get_sset_count = xennet_get_sset_count,
	.get_ethtool_stats = xennet_get_ethtool_stats,
	.get_strings = xennet_get_strings,
};

#ifdef CONFIG_SYSFS
static ssize_t show_rxbuf_min(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct netfront_info *info = netdev_priv(to_net_dev(dev));

	return sprintf(buf, "%u\n", info->rx_min_target);
}

static ssize_t store_rxbuf_min(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t len)
{
	struct net_device *netdev = to_net_dev(dev);
	struct netfront_info *np = netdev_priv(netdev);
	char *endp;
	unsigned long target;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	target = simple_strtoul(buf, &endp, 0);
	if (endp == buf)
		return -EBADMSG;

	if (target < RX_MIN_TARGET)
		target = RX_MIN_TARGET;
	if (target > RX_MAX_TARGET)
		target = RX_MAX_TARGET;

	spin_lock_bh(&np->rx_lock);
	if (target > np->rx_max_target)
		np->rx_max_target = target;
	np->rx_min_target = target;
	if (target > np->rx_target)
		np->rx_target = target;

	network_alloc_rx_buffers(netdev);

	spin_unlock_bh(&np->rx_lock);
	return len;
}

static ssize_t show_rxbuf_max(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct netfront_info *info = netdev_priv(to_net_dev(dev));

	return sprintf(buf, "%u\n", info->rx_max_target);
}

static ssize_t store_rxbuf_max(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t len)
{
	struct net_device *netdev = to_net_dev(dev);
	struct netfront_info *np = netdev_priv(netdev);
	char *endp;
	unsigned long target;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	target = simple_strtoul(buf, &endp, 0);
	if (endp == buf)
		return -EBADMSG;

	if (target < RX_MIN_TARGET)
		target = RX_MIN_TARGET;
	if (target > RX_MAX_TARGET)
		target = RX_MAX_TARGET;

	spin_lock_bh(&np->rx_lock);
	if (target < np->rx_min_target)
		np->rx_min_target = target;
	np->rx_max_target = target;
	if (target < np->rx_target)
		np->rx_target = target;

	network_alloc_rx_buffers(netdev);

	spin_unlock_bh(&np->rx_lock);
	return len;
}

static ssize_t show_rxbuf_cur(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	struct netfront_info *info = netdev_priv(to_net_dev(dev));

	return sprintf(buf, "%u\n", info->rx_target);
}

static struct device_attribute xennet_attrs[] = {
	__ATTR(rxbuf_min, S_IRUGO|S_IWUSR, show_rxbuf_min, store_rxbuf_min),
	__ATTR(rxbuf_max, S_IRUGO|S_IWUSR, show_rxbuf_max, store_rxbuf_max),
	__ATTR(rxbuf_cur, S_IRUGO, show_rxbuf_cur, NULL),
};

static int xennet_sysfs_addif(struct net_device *netdev)
{
	int i;
	int error = 0;

	for (i = 0; i < ARRAY_SIZE(xennet_attrs); i++) {
		error = device_create_file(&netdev->dev,
					   &xennet_attrs[i]);
		if (error)
			goto fail;
	}
	return 0;

 fail:
	while (--i >= 0)
		device_remove_file(&netdev->dev, &xennet_attrs[i]);
	return error;
}

static void xennet_sysfs_delif(struct net_device *netdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(xennet_attrs); i++)
		device_remove_file(&netdev->dev, &xennet_attrs[i]);
}

#endif /* CONFIG_SYSFS */


/*
 * Nothing to do here. Virtual interface is point-to-point and the
 * physical interface is probably promiscuous anyway.
 */
static void network_set_multicast_list(struct net_device *dev)
{
}

static netdev_features_t xennet_fix_features(struct net_device *dev,
					     netdev_features_t features)
{
	struct netfront_info *np = netdev_priv(dev);
	int val;

	if (features & NETIF_F_SG) {
		if (xenbus_scanf(XBT_NIL, np->xbdev->otherend, "feature-sg",
				 "%d", &val) < 0)
			val = 0;

		if (!val)
			features &= ~NETIF_F_SG;
	}

	if (features & NETIF_F_TSO) {
		if (xenbus_scanf(XBT_NIL, np->xbdev->otherend,
				 "feature-gso-tcpv4", "%d", &val) < 0)
			val = 0;

		if (!val)
			features &= ~NETIF_F_TSO;
	}

	return features;
}

static int xennet_set_features(struct net_device *dev,
			       netdev_features_t features)
{
	if (!(features & NETIF_F_SG) && dev->mtu > ETH_DATA_LEN) {
		netdev_info(dev, "Reducing MTU because no SG offload");
		dev->mtu = ETH_DATA_LEN;
	}

	return 0;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void xennet_poll_controller(struct net_device *dev)
{
	netif_int(0, dev);
}
#endif

static const struct net_device_ops xennet_netdev_ops = {
	.ndo_uninit             = netif_uninit,
	.ndo_open               = network_open,
	.ndo_stop               = network_close,
	.ndo_start_xmit         = network_start_xmit,
	.ndo_set_rx_mode        = network_set_multicast_list,
	.ndo_set_mac_address    = xennet_set_mac_address,
	.ndo_validate_addr      = eth_validate_addr,
	.ndo_fix_features       = xennet_fix_features,
	.ndo_set_features       = xennet_set_features,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller    = xennet_poll_controller,
#endif
	.ndo_change_mtu	        = xennet_change_mtu,
	.ndo_get_stats64        = xennet_get_stats64,
};

static struct net_device *create_netdev(struct xenbus_device *dev)
{
	int i, err = 0;
	struct net_device *netdev = NULL;
	struct netfront_info *np = NULL;

	netdev = alloc_etherdev(sizeof(struct netfront_info));
	if (!netdev)
		return ERR_PTR(-ENOMEM);

	np                   = netdev_priv(netdev);
	np->xbdev            = dev;

	spin_lock_init(&np->tx_lock);
	spin_lock_init(&np->rx_lock);

	init_accelerator_vif(np, dev);

	skb_queue_head_init(&np->rx_batch);
	np->rx_target     = RX_DFL_MIN_TARGET;
	np->rx_min_target = RX_DFL_MIN_TARGET;
	np->rx_max_target = RX_MAX_TARGET;

	init_timer(&np->rx_refill_timer);
	np->rx_refill_timer.data = (unsigned long)netdev;
	np->rx_refill_timer.function = rx_refill_timeout;

	err = -ENOMEM;
	np->rx_stats = alloc_percpu(struct netfront_stats);
	if (np->rx_stats == NULL)
		goto exit;
	np->tx_stats = alloc_percpu(struct netfront_stats);
	if (np->tx_stats == NULL)
		goto exit;
	for_each_possible_cpu(i) {
		u64_stats_init(&per_cpu_ptr(np->rx_stats, i)->syncp);
		u64_stats_init(&per_cpu_ptr(np->tx_stats, i)->syncp);
	}

	/* Initialise {tx,rx}_skbs as a free chain containing every entry. */
	for (i = 0; i <= NET_TX_RING_SIZE; i++) {
		np->tx_skbs[i] = (void *)((unsigned long) i+1);
		np->grant_tx_ref[i] = GRANT_INVALID_REF;
	}

	for (i = 0; i < NET_RX_RING_SIZE; i++) {
		np->rx_skbs[i] = NULL;
		np->grant_rx_ref[i] = GRANT_INVALID_REF;
	}

	/* A grant for every tx ring slot */
	if (gnttab_alloc_grant_references(TX_MAX_TARGET,
					  &np->gref_tx_head) < 0) {
		pr_alert("#### netfront can't alloc tx grant refs\n");
		err = -ENOMEM;
		goto exit;
	}
	/* A grant for every rx ring slot */
	if (gnttab_alloc_grant_references(RX_MAX_TARGET,
					  &np->gref_rx_head) < 0) {
		pr_alert("#### netfront can't alloc rx grant refs\n");
		err = -ENOMEM;
		goto exit_free_tx;
	}

	netdev->netdev_ops	= &xennet_netdev_ops;
	netif_napi_add(netdev, &np->napi, netif_poll, 64);
	netdev->features        = NETIF_F_RXCSUM | NETIF_F_GSO_ROBUST;
	netdev->hw_features	= NETIF_F_IP_CSUM | NETIF_F_SG | NETIF_F_TSO;

	/*
         * Assume that all hw features are available for now. This set
         * will be adjusted by the call to netdev_update_features() in
         * xennet_connect() which is the earliest point where we can
         * negotiate with the backend regarding supported features.
         */
	netdev->features |= netdev->hw_features;

	SET_ETHTOOL_OPS(netdev, &network_ethtool_ops);
	SET_NETDEV_DEV(netdev, &dev->dev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
	netif_set_gso_max_size(netdev, XEN_NETIF_MAX_TX_SIZE - MAX_TCP_HEADER);
#endif

	np->netdev = netdev;

	netfront_carrier_off(np);

	return netdev;

 exit_free_tx:
	gnttab_free_grant_references(np->gref_tx_head);
 exit:
	netfront_free_netdev(netdev);
	return ERR_PTR(err);
}

static void netif_release_rings(struct netfront_info *info)
{
	end_access(info->tx_ring_ref, info->tx.sring);
	end_access(info->rx_ring_ref, info->rx.sring);
	info->tx_ring_ref = GRANT_INVALID_REF;
	info->rx_ring_ref = GRANT_INVALID_REF;
	info->tx.sring = NULL;
	info->rx.sring = NULL;
}

static void netif_disconnect_backend(struct netfront_info *info)
{
	/* Stop old i/f to prevent errors whilst we rebuild the state. */
	spin_lock_bh(&info->rx_lock);
	spin_lock_irq(&info->tx_lock);
	netfront_carrier_off(info);
	spin_unlock_irq(&info->tx_lock);
	spin_unlock_bh(&info->rx_lock);

	if (info->irq)
		unbind_from_irqhandler(info->irq, info->netdev);
	info->irq = 0;

	netif_release_rings(info);
}


static void end_access(int ref, void *page)
{
	if (ref != GRANT_INVALID_REF)
		gnttab_end_foreign_access(ref, (unsigned long)page);
}


/* ** Driver registration ** */


static const struct xenbus_device_id netfront_ids[] = {
	{ "vif" },
	{ "" }
};
MODULE_ALIAS("xen:vif");

static DEFINE_XENBUS_DRIVER(netfront, ,
	.probe = netfront_probe,
	.remove = netfront_remove,
	.suspend = netfront_suspend,
	.suspend_cancel = netfront_suspend_cancel,
	.resume = netfront_resume,
	.otherend_changed = backend_changed,
);


static int __init netif_init(void)
{
	if (!is_running_on_xen())
		return -ENODEV;

#ifdef CONFIG_XEN
	if (MODPARM_rx_flip && MODPARM_rx_copy) {
		pr_warning("Cannot specify both rx_copy and rx_flip.\n");
		return -EINVAL;
	}

	if (!MODPARM_rx_flip && !MODPARM_rx_copy)
		MODPARM_rx_copy = true; /* Default is to copy. */
#endif

	netif_init_accel();

	pr_info("Initialising virtual ethernet driver.\n");

	return xenbus_register_frontend(&netfront_driver);
}
module_init(netif_init);


static void __exit netif_exit(void)
{
	xenbus_unregister_driver(&netfront_driver);

	netif_exit_accel();
}
module_exit(netif_exit);

MODULE_LICENSE("Dual BSD/GPL");
