/*  Xenbus code for netif backend
    Copyright (C) 2005 Rusty Russell <rusty@rustcorp.com.au>
    Copyright (C) 2005 XenSource Ltd

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdarg.h>
#include <linux/rwsem.h>
#include <xen/xenbus.h>
#include "common.h"

static DECLARE_RWSEM(teardown_sem);

static int connect_rings(struct backend_info *);
static void connect(struct backend_info *);
static int backend_create_netif(struct backend_info *be);
static void unregister_hotplug_status_watch(struct backend_info *be);

static int netback_remove(struct xenbus_device *dev)
{
	struct backend_info *be = dev_get_drvdata(&dev->dev);

	netback_remove_accelerators(be, dev);

	xenbus_switch_state(dev, XenbusStateClosed);
	unregister_hotplug_status_watch(be);
	kobject_uevent(&dev->dev.kobj, KOBJ_OFFLINE);
	down_write(&teardown_sem);
	if (be->netif) {
		xenbus_rm(XBT_NIL, be->dev->nodename, "hotplug-status");
		netif_disconnect(be);
		netif_free(be);
		be->netif = NULL;
	}
	dev_set_drvdata(&dev->dev, NULL);
	up_write(&teardown_sem);
	kfree(be->hotplug_script);
	kfree(be);
	return 0;
}

static void netback_disconnect(struct device *xbdev_dev)
{
	struct backend_info *be;

	down_read(&teardown_sem);
	be = dev_get_drvdata(xbdev_dev);
	if (be && be->netif)
		netif_disconnect(be);
	up_read(&teardown_sem);
}

/**
 * Entry point to this code when a new device is created.  Allocate the basic
 * structures and switch to InitWait.
 */
static int netback_probe(struct xenbus_device *dev,
			 const struct xenbus_device_id *id)
{
	const char *message;
	struct xenbus_transaction xbt;
	int err;
	int sg;
	const char *script;
	struct backend_info *be = kzalloc(sizeof(struct backend_info),
					  GFP_KERNEL);
	if (!be) {
		xenbus_dev_fatal(dev, -ENOMEM,
				 "allocating backend structure");
		return -ENOMEM;
	}

	be->dev = dev;
	dev_set_drvdata(&dev->dev, be);

	sg = 1;
	if (netbk_copy_skb_mode == NETBK_ALWAYS_COPY_SKB)
		sg = 0;

	do {
		err = xenbus_transaction_start(&xbt);
		if (err) {
			xenbus_dev_fatal(dev, err, "starting transaction");
			goto fail;
		}

		err = xenbus_printf(xbt, dev->nodename, "feature-sg", "%d", sg);
		if (err) {
			message = "writing feature-sg";
			goto abort_transaction;
		}

		err = xenbus_printf(xbt, dev->nodename, "feature-gso-tcpv4",
				    "%d", sg);
		if (err) {
			message = "writing feature-gso-tcpv4";
			goto abort_transaction;
		}

		/* We support rx-copy path. */
		err = xenbus_write(xbt, dev->nodename, "feature-rx-copy", "1");
		if (err) {
			message = "writing feature-rx-copy";
			goto abort_transaction;
		}

		/*
		 * We don't support rx-flip path (except old guests who don't
		 * grok this feature flag).
		 */
		err = xenbus_write(xbt, dev->nodename, "feature-rx-flip", "0");
		if (err) {
			message = "writing feature-rx-flip";
			goto abort_transaction;
		}

		err = xenbus_transaction_end(xbt, 0);
	} while (err == -EAGAIN);

	if (err) {
		xenbus_dev_fatal(dev, err, "completing transaction");
		goto fail;
	}

	netback_probe_accelerators(be, dev);

	script = xenbus_read(XBT_NIL, dev->nodename, "script", NULL);
	if (IS_ERR(script)) {
		err = PTR_ERR(script);
		xenbus_dev_fatal(dev, err, "reading script");
		goto fail;
	}

	be->hotplug_script = script;

	err = xenbus_switch_state(dev, XenbusStateInitWait);
	if (err)
		goto fail;

	/* This kicks hotplug scripts, so do it immediately. */
	err = backend_create_netif(be);
	if (err)
		goto fail;

	return 0;

abort_transaction:
	xenbus_transaction_end(xbt, 1);
	xenbus_dev_fatal(dev, err, "%s", message);
fail:
	DPRINTK("failed");
	netback_remove(dev);
	return err;
}


/**
 * Handle the creation of the hotplug script environment.  We add the script
 * and vif variables to the environment, for the benefit of the vif-* hotplug
 * scripts.
 */
static int netback_uevent(struct xenbus_device *xdev, struct kobj_uevent_env *env)
{
	struct backend_info *be;

	DPRINTK("netback_uevent");

	down_read(&teardown_sem);
	be = dev_get_drvdata(&xdev->dev);
	if (be)
		add_uevent_var(env, "script=%s", be->hotplug_script);
	if (be && be->netif)
		add_uevent_var(env, "vif=%s", be->netif->dev->name);
	up_read(&teardown_sem);

	return 0;
}


static int backend_create_netif(struct backend_info *be)
{
	int err;
	long handle;
	struct xenbus_device *dev = be->dev;
	netif_t *netif;

	if (be->netif != NULL)
		return 0;

	err = xenbus_scanf(XBT_NIL, dev->nodename, "handle", "%li", &handle);
	if (err != 1) {
		xenbus_dev_fatal(dev, err, "reading handle");
		return (err < 0) ? err : -EINVAL;
	}

	netif = netif_alloc(&dev->dev, dev->otherend_id, handle);
	if (IS_ERR(netif)) {
		err = PTR_ERR(netif);
		xenbus_dev_fatal(dev, err, "creating interface");
		return err;
	}
	be->netif = netif;

	kobject_uevent(&dev->dev.kobj, KOBJ_ONLINE);
	return 0;
}


/**
 * Callback received when the frontend's state changes.
 */
static void frontend_changed(struct xenbus_device *dev,
			     enum xenbus_state frontend_state)
{
	struct backend_info *be = dev_get_drvdata(&dev->dev);

	DPRINTK("%s", xenbus_strstate(frontend_state));

	be->frontend_state = frontend_state;

	switch (frontend_state) {
	case XenbusStateInitialising:
		if (dev->state == XenbusStateClosed) {
			pr_info("%s: %s: prepare for reconnect\n",
				__FUNCTION__, dev->nodename);
			xenbus_switch_state(dev, XenbusStateInitWait);
		}
		break;

	case XenbusStateInitialised:
		break;

	case XenbusStateConnected:
		if (dev->state == XenbusStateConnected)
			break;
		if (be->netif)
			connect(be);
		break;

	case XenbusStateClosing:
		netback_disconnect(&dev->dev);
		xenbus_switch_state(dev, XenbusStateClosing);
		break;

	case XenbusStateClosed:
		xenbus_switch_state(dev, XenbusStateClosed);
		if (xenbus_dev_is_online(dev))
			break;
		/* fall through if not online */
	case XenbusStateUnknown:
		/* implies netback_disconnect() via netback_remove() */
		device_unregister(&dev->dev);
		break;

	default:
		xenbus_dev_fatal(dev, -EINVAL, "saw state %d at frontend",
				 frontend_state);
		break;
	}
}


static void xen_net_read_rate(struct xenbus_device *dev,
			      unsigned long *bytes, unsigned long *usec)
{
	char *s, *e;
	unsigned long b, u;
	char *ratestr;

	/* Default to unlimited bandwidth. */
	*bytes = ~0UL;
	*usec = 0;

	ratestr = xenbus_read(XBT_NIL, dev->nodename, "rate", NULL);
	if (IS_ERR(ratestr))
		return;

	s = ratestr;
	b = simple_strtoul(s, &e, 10);
	if ((s == e) || (*e != ','))
		goto fail;

	s = e + 1;
	u = simple_strtoul(s, &e, 10);
	if ((s == e) || (*e != '\0'))
		goto fail;

	*bytes = b;
	*usec = u;

	kfree(ratestr);
	return;

 fail:
	dev_warn(&dev->dev,
		 "Failed to parse network rate limit. Traffic unlimited.\n");
	kfree(ratestr);
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

static void unregister_hotplug_status_watch(struct backend_info *be)
{
	if (be->have_hotplug_status_watch) {
		unregister_xenbus_watch(&be->hotplug_status_watch);
		kfree(be->hotplug_status_watch.node);
	}
	be->have_hotplug_status_watch = 0;
}

static void hotplug_status_changed(struct xenbus_watch *watch,
				   const char **vec,
				   unsigned int vec_size)
{
	struct backend_info *be = container_of(watch,
					       struct backend_info,
					       hotplug_status_watch);
	char *str;
	unsigned int len;

	str = xenbus_read(XBT_NIL, be->dev->nodename, "hotplug-status", &len);
	if (IS_ERR(str))
		return;
	if (len == sizeof("connected")-1 && !memcmp(str, "connected", len)) {
		xenbus_switch_state(be->dev, XenbusStateConnected);
		/* Not interested in this watch anymore. */
		unregister_hotplug_status_watch(be);
	}
	kfree(str);
}

static void connect(struct backend_info *be)
{
	int err;
	struct xenbus_device *dev = be->dev;

	err = connect_rings(be);
	if (err)
		return;

	err = xen_net_read_mac(dev, be->netif->fe_dev_addr);
	if (err) {
		xenbus_dev_fatal(dev, err, "parsing %s/mac", dev->nodename);
		return;
	}

	xen_net_read_rate(dev, &be->netif->credit_bytes,
			  &be->netif->credit_usec);
	be->netif->remaining_credit = be->netif->credit_bytes;

	unregister_hotplug_status_watch(be);
	err = xenbus_watch_path2(dev, dev->nodename, "hotplug-status",
				 &be->hotplug_status_watch,
				 hotplug_status_changed);
	if (err) {
		/* Switch now, since we can't do a watch. */
		xenbus_switch_state(dev, XenbusStateConnected);
	} else {
		be->have_hotplug_status_watch = 1;
	}

	netif_wake_queue(be->netif->dev);
}


static int connect_rings(struct backend_info *be)
{
	netif_t *netif = be->netif;
	struct xenbus_device *dev = be->dev;
	unsigned int tx_ring_ref, rx_ring_ref;
	unsigned int evtchn, rx_copy;
	int err;
	int val;

	DPRINTK("");

	err = xenbus_gather(XBT_NIL, dev->otherend,
			    "tx-ring-ref", "%u", &tx_ring_ref,
			    "rx-ring-ref", "%u", &rx_ring_ref,
			    "event-channel", "%u", &evtchn, NULL);
	if (err) {
		xenbus_dev_fatal(dev, err,
				 "reading %s/ring-ref and event-channel",
				 dev->otherend);
		return err;
	}

	err = xenbus_scanf(XBT_NIL, dev->otherend, "request-rx-copy", "%u",
			   &rx_copy);
	if (err == -ENOENT) {
		err = 0;
		rx_copy = 0;
	}
	if (err < 0) {
		xenbus_dev_fatal(dev, err, "reading %s/request-rx-copy",
				 dev->otherend);
		return err;
	}
	netif->copying_receiver = !!rx_copy;

	if (netif->dev->tx_queue_len != 0) {
		if (xenbus_scanf(XBT_NIL, dev->otherend,
				 "feature-rx-notify", "%d", &val) < 0)
			val = 0;
		if (val)
			netif->can_queue = 1;
		else
			/* Must be non-zero for pfifo_fast to work. */
			netif->dev->tx_queue_len = 1;
	}

	if (xenbus_scanf(XBT_NIL, dev->otherend, "feature-sg", "%d", &val) < 0)
		val = 0;
	netif->can_sg = !!val;

	if (xenbus_scanf(XBT_NIL, dev->otherend, "feature-gso-tcpv4", "%d",
			 &val) < 0)
		val = 0;
	netif->gso = !!val;

	if (xenbus_scanf(XBT_NIL, dev->otherend, "feature-no-csum-offload",
			 "%d", &val) < 0)
		val = 0;
	netif->csum = !val;

	/* Map the shared frame, irq etc. */
	err = netif_map(be, tx_ring_ref, rx_ring_ref, evtchn);
	if (err) {
		xenbus_dev_fatal(dev, err,
				 "mapping shared-frames %u/%u port %u",
				 tx_ring_ref, rx_ring_ref, evtchn);
		return err;
	}
	return 0;
}


/* ** Driver Registration ** */


static const struct xenbus_device_id netback_ids[] = {
	{ "vif" },
	{ "" }
};

static DEFINE_XENBUS_DRIVER(netback, ,
	.probe = netback_probe,
	.remove = netback_remove,
	.uevent = netback_uevent,
	.otherend_changed = frontend_changed,
);


void netif_xenbus_init(void)
{
	WARN_ON(xenbus_register_backend(&netback_driver));
}
