/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Intel(R) HFI2 specific support for VNIC functionality
 */
#include <rdma/opa_vnic.h>
#include "hfi2.h"
#include "hfi_kclient.h"
#include "hfi_tx_bypass.h"
#include "verbs/verbs.h"
#include "verbs/packet.h"

#define HFI2_MAX_NUM_VNIC_CTXT 4
#define HFI2_NUM_VNIC_CTXT 4
#define HFI2_MAX_NUM_PORTS 1
#define HFI2_NET_RX_POLL_MS 1
#define HFI2_MAX_NET_NUM_RX_BUFS 2048
#define HFI2_NET_NUM_RX_BUFS 2048
#define HFI2_NET_EAGER_SIZE (PAGE_SIZE * 4)

#define HFI2_VNIC_ICRC_LEN   4
#define HFI2_VNIC_TAIL_LEN   1
#define HFI2_VNIC_ICRC_TAIL_LEN  (HFI2_VNIC_ICRC_LEN + HFI2_VNIC_TAIL_LEN)
#define HFI2_VNIC_L4_OFFSET  18
#define HFI2_VNIC_L4_HDR_LEN 2
#define HFI2_VNIC_16B_L2_HDR_LEN 16
#define HFI2_VNIC_16B_HDR_LEN    (HFI2_VNIC_16B_L2_HDR_LEN + \
				  HFI2_VNIC_L4_HDR_LEN + 2)

#define HFI2_VNIC_16B_CNP_FLITS  4
#define HFI2_VNIC_CNP_PKT_LEN    20
#define HFI2_VNIC_L4_TYPE_OFFSET 8
#define HFI2_VNIC_L4_TYPE_MASK   0xff
#define HFI2_VNIC_L4_HDR_OFFSET  18
#define HFI2_VNIC_SC_OFFSET      6
#define HFI2_VNIC_SC_HIGH_OFFSET 7
#define HFI2_VNIC_SC_SHIFT 4
#define HFI2_VNIC_FECN_OFFSET    7
#define HFI2_VNIC_FECN_MASK      0x10
#define HFI2_VNIC_BECN_OFFSET	 3
#define HFI2_VNIC_BECN_MASK      0x80

#define HFI2_VNIC_L4_ETHR 0x78
#define HFI2_VNIC_L4_EEPH 0x79

#define HFI2_VNIC_GET_L4_TYPE(data) \
	(*((u8 *)(data) + HFI2_VNIC_L4_TYPE_OFFSET) & HFI2_VNIC_L4_TYPE_MASK)

#define HFI2_VNIC_GET_L4_HDR(data) \
	(*((u16 *)((u8 *)(data) + HFI2_VNIC_L4_HDR_OFFSET)))

#define HFI2_VNIC_IS_FECN_SET(data) \
	(*((u8 *)(data) + HFI2_VNIC_FECN_OFFSET) & HFI2_VNIC_FECN_MASK)
#define HFI2_VNIC_IS_BECN_SET(data) \
	(*((u8 *)(data) + HFI2_VNIC_BECN_OFFSET) & HFI2_VNIC_BECN_MASK)
#define HFI2_VNIC_L4_GNI	BIT(15)

#define HFI2_VNIC_RCV_Q_SIZE	8192
#define HFI2_VNIC_UP		0

static inline u8 hfi2_vnic_get_sc(u8 *hdr)
{
	u8 sc5;

	/* sc5 = bit hdr7[0] + bits hdr6[7..4] */
	sc5 = ((*(hdr + HFI2_VNIC_SC_HIGH_OFFSET) & 0x1) << HFI2_VNIC_SC_SHIFT);
	sc5 |= (*(hdr + HFI2_VNIC_SC_OFFSET) >> HFI2_VNIC_SC_SHIFT);
	return sc5;
}

static inline void hfi2_vnic_set_sc(u8 *hdr, u8 sc)
{
	/* sc5 = bit hdr7[0] + bits hdr6[7..4] */
	*(hdr + HFI2_VNIC_SC_HIGH_OFFSET) &= ~0x1;
	*(hdr + HFI2_VNIC_SC_OFFSET) &= ~0xf0;
	*(hdr + HFI2_VNIC_SC_HIGH_OFFSET) |= (sc >> HFI2_VNIC_SC_SHIFT) & 0x1;
	*(hdr + HFI2_VNIC_SC_OFFSET) |= sc << HFI2_VNIC_SC_SHIFT;
}

/* TODO: Share duplicated macros with PCIe driver in common headers */
#define HFI2_BYPASS_L2_MASK	0x3ull
#define HFI2_BYPASS_HDR_16B	0x2
#define QW_SHIFT		6ull
/* VESWID[0..1] QW 2, OFFSET 16 - for select */
#define L4_HDR_VESWID_OFFSET	((2 << QW_SHIFT) | (16ull))
/* ENTROPY[0..1] QW 1, OFFSET 32 - for select */
#define L2_ENTROPY_OFFSET	((1 << QW_SHIFT) | (32ull))
#define SUM_GRP_COUNTERS(stats, qstats, x_grp) do {            \
		u64 *src64, *dst64;                            \
		for (src64 = &qstats->x_grp.unicast,           \
			dst64 = &stats->x_grp.unicast;         \
			dst64 <= &stats->x_grp.s_1519_max;) {  \
			*dst64++ += *src64++;                  \
		}                                              \
	} while (0)

/**
 * struct hfi2_vnic_rx_queue - HFI2 VNIC receive queue
 * @idx: queue index
 * @vinfo: pointer to vport information
 * @netdev: network device
 * @napi: netdev napi structure
 * @skbq: queue of received socket buffers
 */
struct hfi2_vnic_rx_queue {
	u8                           idx;
	struct hfi2_vnic_vport_info *vinfo;
	struct net_device           *netdev;
	struct napi_struct           napi;
	struct sk_buff_head          skbq;
};

/*
 * struct hfi2_ctx_info - HFI context specific fields
 *
 * @ctx: HFI context
 * @cmdq: TX and RX command queues
 * @buf: Array of receive buffers
 * @eq_tx: TX event handle
 * @eq_rx: RX event handle
 * @ndev: Back pointer to HFI2 netdev
 * @q_idx: Queue index number
 * @tx_notify_list: List of notifications to send do upper layer
 * @tx_notify_count: Number of outstanding notifications
 */
struct hfi2_ctx_info {
	struct hfi_ctx			ctx;
	struct hfi_cmdq_pair		cmdq;
	void				*buf[HFI2_MAX_NET_NUM_RX_BUFS];
	struct hfi_eq			eq_tx;
	struct hfi_eq			eq_rx;
	struct hfi2_netdev		*ndev;
	u8				q_idx;
	struct list_head		tx_notify_list;
	atomic_t			tx_notify_count;
};

/**
 * struct hfi2_vnic_vport_info - HFI2 VNIC virtual port information
 * @dd: device data pointer
 * @netdev: net device pointer
 * @flags: state flags
 * @lock: vport lock
 * @num_tx_q: number of transmit queues
 * @num_rx_q: number of receive queues
 * @vesw_id: virtual switch id
 * @port_num: port_num
 * @rxq: Array of receive queues
 * @stats: per queue stats
 * @sdma: VNIC SDMA structure per TXQ
 */
struct hfi2_vnic_vport_info {
	struct hfi_devdata	*dd;
	struct net_device	*netdev;
	unsigned long		flags;
	/* Lock used around state updates */
	struct mutex		lock;
	u8			num_tx_q;
	u8			num_rx_q;
	u16			vesw_id;
	u8			port_num;
	struct hfi2_vnic_rx_queue rxq[HFI2_MAX_NUM_VNIC_CTXT];
	struct opa_vnic_stats	stats[HFI2_MAX_NUM_VNIC_CTXT];
};

/*
 * Number of IOVs cached in hfi2_vnic_txreq.
 * iov_cache starts at offset 48, iov size 16 bytes, with 5 cache entries
 * the hfi2_vnic_txreq size is 128 bytes - 2 cache lines.
 */
#define	VNIC_TXREQ_IOV_CACHE_SIZE 5

/*
 * struct hfi2_vnic_txreq - Transmit request descriptor.
 * Keeps data referenced by HW while processing asynchronous transmit request.
 * Also used to hold notification info in case transmit request cannot be
 * send due to HW busy.
 *
 * skb: Buffer that holds packet header and payload
 * iov: Pointer to iov array used in transmit request
 * tail_byte: Holds pad count that is accessed by TX engine to generate padding
 * iov_cache: IOV cache used if skb contains small number of fragments
 *	otherwise iov array will be allocated dynamically
 * head: Links tx notifications into list
 * q_idx: Vnic network device queue to notify
 */
struct hfi2_vnic_txreq {
	struct sk_buff			*skb;
	union base_iovec		*iov;
	struct hfi2_vnic_vport_info	*vinfo;
	u8				tail_byte;
	u8				q_idx;
	struct list_head		head;
	union base_iovec iov_cache[VNIC_TXREQ_IOV_CACHE_SIZE] __aligned(16);
};

/*
 * struct hfi2_netdev - HFI2 net device specific fields
 * @dd: hfi device specic data
 * @num_ports: number of physical ports
 * @num_vports: number of virtual ethernet switch ports
 * @ctrl_lock: Lock to synchronize setup and teardown of the ctrl device
 * @ctx_lock: Lock to synchronize setup and teardown of the bypass context
 * @stats_lock: Lock to synchronize access to common stats
 * @eth_ctx: array of ethernet contexts
 * @vesw_idr: IDR for looking up vesw information
 * @txreq_cache: Cache of transmit request descriptors
 */
struct hfi2_netdev {
	struct hfi_devdata		*dd;
	int				num_ports;
	int				num_vports;
	struct mutex			ctrl_lock;
	struct mutex			ctx_lock;
	spinlock_t			stats_lock;
	struct idr			vesw_idr[HFI2_MAX_NUM_PORTS];
	struct hfi2_ctx_info		eth_ctx[HFI2_MAX_NUM_VNIC_CTXT];
	struct kmem_cache		*txreq_cache;
};

/* Append a single socket buffer */
static int hfi2_vnic_append_skb(struct hfi2_ctx_info *ctx_i, int idx)
{
	struct hfi2_netdev *ndev = ctx_i->ndev;
	struct hfi_ctx *ctx = &ctx_i->ctx;
	struct hfi_cmdq *rx = &ctx_i->cmdq.rx;
	union hfi_rx_cq_command rx_cmd;
	int n_slots, rc;
	u64 done = 0;
	unsigned long sflags;

	n_slots = hfi_format_rx_bypass(ctx, HFI_NI_BYPASS,
				       ctx_i->buf[idx],
				       HFI2_NET_EAGER_SIZE,
				       HFI_PT_BYPASS_EAGER,
				       ctx->ptl_uid,
				       PTL_USE_ONCE | PTL_OP_PUT,
				       HFI_CT_NONE, 0,
				       (u64)&done,
				       idx, HFI_GEN_CC, &rx_cmd);

	spin_lock_irqsave(&rx->lock, sflags);
	rc = hfi_rx_command(rx, (uint64_t *)&rx_cmd, n_slots);
	spin_unlock_irqrestore(&rx->lock, sflags);
	if (rc < 0) {
		dd_dev_err(ndev->dd, "%s rc %d\n", __func__, rc);
		return rc;
	}

	/* Check on EQ 0 NI 0 for a PTL_CMD_COMPLETE event */
	rc = hfi_eq_poll_cmd_complete_timeout(ctx, &done);

	return rc;
}

/* Select vnic TX queue */
static
u16 hfi2_vnic_select_queue(struct net_device *netdev,
			   struct sk_buff *skb,
			   void *accel_priv,
			   select_queue_fallback_t fallback)
{
	struct opa_vnic_skb_mdata *mdata;

	mdata = (struct opa_vnic_skb_mdata *)skb->data;
	/* FXRTODO: Need to support multiple queues per vl */
	return mdata->vl % HFI2_NUM_VNIC_CTXT;
}

/* Called when new cache item is allocated from memory */
static void txreq_cache_ctor(void *obj)
{
	struct hfi2_vnic_txreq *txreq = (struct hfi2_vnic_txreq *)obj;

	memset(txreq, 0, sizeof(*txreq));
}

/* Initialize transmit request cache */
static int txreq_cache_create(struct hfi2_netdev *ndev)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "hfi2_%u_vnic_txreq_cache",
		 ndev->dd->unit);

	ndev->txreq_cache = kmem_cache_create(buf,
					      sizeof(struct hfi2_vnic_txreq),
					      0,
					      SLAB_HWCACHE_ALIGN,
					      txreq_cache_ctor);
	if (!ndev->txreq_cache)
		return -ENOMEM;
	return 0;
}

/* Destroy transmit request cache */
static void txreq_cache_destroy(struct hfi2_netdev *ndev)
{
	kmem_cache_destroy(ndev->txreq_cache);
	ndev->txreq_cache = NULL;
}

/* Allocate tx request descriptor from cache, init iov pointer */
static struct hfi2_vnic_txreq *txreq_cache_get(struct hfi2_netdev *ndev,
					       int num_iov)
{
	struct hfi2_vnic_txreq *txreq;

	txreq = kmem_cache_alloc_node(ndev->txreq_cache, GFP_ATOMIC,
				      ndev->dd->node);
	if (!txreq)
		return NULL;

	if (num_iov <= VNIC_TXREQ_IOV_CACHE_SIZE) {
		txreq->iov = &txreq->iov_cache[0];
	} else {
		/* FXRTODO: replace kcalloc with kcalloc_node */
		/* More IOVs needed that we have in cache, alloc dynamically */
		txreq->iov = kcalloc(num_iov, sizeof(*txreq->iov), GFP_KERNEL);
		if (!txreq->iov) {
			kmem_cache_free(ndev->txreq_cache, txreq);
			return NULL;
		}
	}
	return txreq;
}

/* Release transmit request descriptor to cache */
static void txreq_cache_put(struct hfi2_netdev *ndev,
			    struct hfi2_vnic_txreq *txreq)
{
	if (txreq->iov != &txreq->iov_cache[0])
		kfree(txreq->iov);
	txreq->iov = NULL;
	if (txreq->skb) {
		dev_kfree_skb_any(txreq->skb);
		txreq->skb = NULL;
	}
	kmem_cache_free(ndev->txreq_cache, txreq);
}

/* hfi2_vnic_update_stats - update statistics */
static void hfi2_vnic_update_stats(struct hfi2_vnic_vport_info *vinfo,
				   struct opa_vnic_stats *stats)
{
	struct net_device *netdev = vinfo->netdev;
	u8 i;

	/* add tx counters on different queues */
	for (i = 0; i < vinfo->num_tx_q; i++) {
		struct opa_vnic_stats *qstats = &vinfo->stats[i];
		struct rtnl_link_stats64 *qnstats = &vinfo->stats[i].netstats;

		stats->netstats.tx_fifo_errors += qnstats->tx_fifo_errors;
		stats->netstats.tx_carrier_errors += qnstats->tx_carrier_errors;
		stats->tx_drop_state += qstats->tx_drop_state;
		stats->tx_dlid_zero += qstats->tx_dlid_zero;

		SUM_GRP_COUNTERS(stats, qstats, tx_grp);
		stats->netstats.tx_packets += qnstats->tx_packets;
		stats->netstats.tx_bytes += qnstats->tx_bytes;
	}

	/* add rx counters on different queues */
	for (i = 0; i < vinfo->num_rx_q; i++) {
		struct opa_vnic_stats *qstats = &vinfo->stats[i];
		struct rtnl_link_stats64 *qnstats = &vinfo->stats[i].netstats;

		stats->netstats.rx_fifo_errors += qnstats->rx_fifo_errors;
		stats->netstats.rx_nohandler += qnstats->rx_nohandler;
		stats->rx_drop_state += qstats->rx_drop_state;
		stats->rx_oversize += qstats->rx_oversize;
		stats->rx_runt += qstats->rx_runt;

		SUM_GRP_COUNTERS(stats, qstats, rx_grp);
		stats->netstats.rx_packets += qnstats->rx_packets;
		stats->netstats.rx_bytes += qnstats->rx_bytes;
	}

	stats->netstats.tx_errors = stats->netstats.tx_fifo_errors +
				    stats->netstats.tx_carrier_errors +
				    stats->tx_drop_state + stats->tx_dlid_zero;
	stats->netstats.tx_dropped = stats->netstats.tx_errors;

	stats->netstats.rx_errors = stats->netstats.rx_fifo_errors +
				    stats->netstats.rx_nohandler +
				    stats->rx_drop_state + stats->rx_oversize +
				    stats->rx_runt;
	stats->netstats.rx_dropped = stats->netstats.rx_errors;

	netdev->stats.tx_packets = stats->netstats.tx_packets;
	netdev->stats.tx_bytes = stats->netstats.tx_bytes;
	netdev->stats.tx_fifo_errors = stats->netstats.tx_fifo_errors;
	netdev->stats.tx_carrier_errors = stats->netstats.tx_carrier_errors;
	netdev->stats.tx_errors = stats->netstats.tx_errors;
	netdev->stats.tx_dropped = stats->netstats.tx_dropped;

	netdev->stats.rx_packets = stats->netstats.rx_packets;
	netdev->stats.rx_bytes = stats->netstats.rx_bytes;
	netdev->stats.rx_fifo_errors = stats->netstats.rx_fifo_errors;
	netdev->stats.multicast = stats->rx_grp.mcastbcast;
	netdev->stats.rx_length_errors = stats->rx_oversize + stats->rx_runt;
	netdev->stats.rx_errors = stats->netstats.rx_errors;
	netdev->stats.rx_dropped = stats->netstats.rx_dropped;
}

/* update_len_counters - update pkt's len histogram counters */
static inline void update_len_counters(struct opa_vnic_grp_stats *grp,
				       int len)
{
	/* account for 4 byte FCS */
	if (len >= 1515)
		grp->s_1519_max++;
	else if (len >= 1020)
		grp->s_1024_1518++;
	else if (len >= 508)
		grp->s_512_1023++;
	else if (len >= 252)
		grp->s_256_511++;
	else if (len >= 124)
		grp->s_128_255++;
	else if (len >= 61)
		grp->s_65_127++;
	else
		grp->s_64++;
}

/* hfi2_vnic_update_tx_counters - update transmit counters */
static void hfi2_vnic_update_tx_counters(struct hfi2_vnic_vport_info *vinfo,
					 u8 q_idx, struct sk_buff *skb, int err)
{
	struct ethhdr *mac_hdr = (struct ethhdr *)skb_mac_header(skb);
	struct opa_vnic_stats *stats = &vinfo->stats[q_idx];
	struct opa_vnic_grp_stats *tx_grp = &stats->tx_grp;
	u16 vlan_tci;

	stats->netstats.tx_packets++;
	stats->netstats.tx_bytes += skb->len + ETH_FCS_LEN;

	update_len_counters(tx_grp, skb->len);

	/* rest of the counts are for good packets only */
	if (unlikely(err))
		return;

	if (is_multicast_ether_addr(mac_hdr->h_dest))
		tx_grp->mcastbcast++;
	else
		tx_grp->unicast++;

	if (!__vlan_get_tag(skb, &vlan_tci))
		tx_grp->vlan++;
	else
		tx_grp->untagged++;
}

/* hfi2_vnic_update_rx_counters - update receive counters */
static void hfi2_vnic_update_rx_counters(struct hfi2_vnic_vport_info *vinfo,
					 u8 q_idx, struct sk_buff *skb, int err)
{
	struct ethhdr *mac_hdr = (struct ethhdr *)skb->data;
	struct opa_vnic_stats *stats = &vinfo->stats[q_idx];
	struct opa_vnic_grp_stats *rx_grp = &stats->rx_grp;
	u16 vlan_tci;

	stats->netstats.rx_packets++;
	stats->netstats.rx_bytes += skb->len + ETH_FCS_LEN;

	update_len_counters(rx_grp, skb->len);

	/* rest of the counts are for good packets only */
	if (unlikely(err))
		return;

	if (is_multicast_ether_addr(mac_hdr->h_dest))
		rx_grp->mcastbcast++;
	else
		rx_grp->unicast++;

	if (!__vlan_get_tag(skb, &vlan_tci))
		rx_grp->vlan++;
	else
		rx_grp->untagged++;
}

/* This function is overloaded for opa_vnic specific implementation */
static void hfi2_vnic_get_stats64(struct net_device *netdev,
				  struct rtnl_link_stats64 *stats)
{
	struct opa_vnic_stats *vstats = (struct opa_vnic_stats *)stats;
	struct hfi2_vnic_vport_info *vinfo = opa_vnic_dev_priv(netdev);

	hfi2_vnic_update_stats(vinfo, vstats);
}

/* Sent tx notification to upper layer */
static void tx_notify(struct hfi2_vnic_txreq *txreq)
{
	struct hfi2_vnic_vport_info *vinfo = txreq->vinfo;
	u8 q_idx = txreq->q_idx;

	if (__netif_subqueue_stopped(vinfo->netdev, q_idx)) {
		netif_wake_subqueue(vinfo->netdev, q_idx);
	}
}

/* Process list of outstanding notifications */
static void tx_notify_list_process(struct hfi2_ctx_info *ctx_i, bool deinit)
{
	unsigned long sflags;

	spin_lock_irqsave(&ctx_i->cmdq.tx.lock, sflags);
	/*
	 * Wait until there are at least 32 available slots in CMDQ before
	 * sending notifications. Note that due to HW cq head update limiting
	 * we might end up waiting for up to 64 available slots.
	 * TODO: Optimize for best performance on real HW
	 */
	if (deinit || hfi_queue_ready(&ctx_i->cmdq.tx, 32, &ctx_i->eq_tx)) {
		while (!list_empty(&ctx_i->tx_notify_list)) {
			struct hfi2_vnic_txreq *txreq;

			txreq = list_entry(ctx_i->tx_notify_list.next,
					   struct hfi2_vnic_txreq,
					   head);
			list_del(&txreq->head);
			if (!deinit)
				tx_notify(txreq);
			txreq_cache_put(ctx_i->ndev, txreq);
			atomic_dec(&ctx_i->tx_notify_count);
		}
	}
	spin_unlock_irqrestore(&ctx_i->cmdq.tx.lock, sflags);
}

/* Add (vdev,queue) to list of outstanding notifications */
static void tx_notify_list_add(struct hfi2_ctx_info *ctx_i,
			       struct hfi2_vnic_txreq *txreq,
			       struct hfi2_vnic_vport_info *vinfo,
			       u8 q_idx)
	__must_hold(&ctx_i->tx_lock)
{
	txreq->vinfo = vinfo;
	txreq->q_idx = q_idx;
	list_add_tail(&txreq->head, &ctx_i->tx_notify_list);
	atomic_inc(&ctx_i->tx_notify_count);
}

/*
 * Transmit ISR callback, called after HW finished sending packet.
 * Returns tx request decriptor to cache
 */
static void hfi2_vnic_tx_isr_cb(struct hfi_eq *eq_tx, void *data)
{
	struct hfi2_ctx_info *ctx_i = data;
	struct hfi_eq *eq = &ctx_i->eq_tx;
	struct hfi2_netdev *ndev = ctx_i->ndev;
	struct hfi2_vnic_txreq *txreq;
	union initiator_EQEntry *eq_entry;
	bool dropped;
	int ret;

next_event:
	ret = hfi_eq_peek(eq, (uint64_t **)&eq_entry, &dropped);
	if (ret <= 0)
		return;

	if (unlikely(eq_entry->event_kind != NON_PTL_EVENT_TX_COMPLETE)) {
		dd_dev_warn(ndev->dd, "Ctx %d: unexpected event kind %x\n",
			    ctx_i->ctx.pid, eq_entry->event_kind);
		goto eq_advance;
	}
	txreq = (struct hfi2_vnic_txreq *)eq_entry->user_ptr;
	if (unlikely(!txreq)) {
		dd_dev_warn(ndev->dd, "TX event, ctx %d: invalid txreq\n",
			    ctx_i->ctx.pid);
		goto eq_advance;
	}

	txreq_cache_put(ndev, txreq);

eq_advance:
	hfi_eq_advance(eq, (uint64_t *)eq_entry);
	hfi_eq_pending_dec(eq);
	if (atomic_read(&ctx_i->tx_notify_count))
		tx_notify_list_process(ctx_i, false);

	goto next_event;
}

static void hfi2_vnic_maybe_stop_tx(struct hfi2_vnic_vport_info *vinfo,
				    u8 q_idx)
{	struct hfi2_netdev *ndev = vinfo->dd->vnic;
	struct hfi2_ctx_info *ctx_i;
	unsigned long sflags;
	bool write_avail;

	ctx_i = &ndev->eth_ctx[q_idx];
	netif_stop_subqueue(vinfo->netdev, q_idx);
	spin_lock_irqsave(&ctx_i->cmdq.tx.lock, sflags);
	write_avail = hfi_queue_ready(&ctx_i->cmdq.tx, 1, &ctx_i->eq_tx);
	spin_unlock_irqrestore(&ctx_i->cmdq.tx.lock, sflags);

	if (!write_avail)
		return;

	netif_start_subqueue(vinfo->netdev, q_idx);
}

/*
 * Transmit an SKB. Packet transmit completions are handled asynchronously
 * in hfi2_vnic_tx_isr_cb()
 */
static netdev_tx_t hfi2_netdev_start_xmit(struct sk_buff *skb,
					  struct net_device *netdev)
{
	struct hfi2_vnic_vport_info *vinfo = opa_vnic_dev_priv(netdev);
	struct hfi_devdata *dd = vinfo->dd;
	struct hfi2_netdev *ndev = dd->vnic;
	struct hfi_pportdata *ppd = to_hfi_ppd(dd, vinfo->port_num);
	struct hfi2_ctx_info *ctx_i;
	struct hfi_ctx *ctx;
	struct hfi_cmdq *tx;
	struct hfi_eq *eq;
	unsigned long sflags;
	struct opa_vnic_skb_mdata *mdata =
		(struct opa_vnic_skb_mdata *)skb->data;
	u8 nr_frags = skb_shinfo(skb)->nr_frags;
	u16 headlen;
	u8 num_iov = nr_frags;
	u8 sc5, sl, q_idx = skb->queue_mapping, j, i = 0;
	int err = 0;
	unsigned int pad_len;
	struct hfi2_vnic_txreq *txreq;
	union hfi_tx_general_dma cmd;
	int slots;
	bool is_cnp_pkt = (skb->len == HFI2_VNIC_CNP_PKT_LEN);

	if (unlikely(!netif_oper_up(netdev))) {
		vinfo->stats[q_idx].tx_drop_state++;
		goto err;
	}
	/* take out meta data */
	skb_pull(skb, sizeof(*mdata));
	if (unlikely(mdata->flags & OPA_VNIC_SKB_MDATA_ENCAP_ERR)) {
		vinfo->stats[q_idx].tx_dlid_zero++;
		goto err;
	}

	headlen = skb_headlen(skb);
	if (headlen)
		num_iov++;

	/* Increase the number of iovecs for the tail byte */
	num_iov++;
	ctx_i = &ndev->eth_ctx[q_idx];
	/* padding for 8 bytes size alignment */
	pad_len = (skb->len + HFI2_VNIC_ICRC_TAIL_LEN) & 0x7;
	if (pad_len)
		pad_len = 8 - pad_len;
	ctx = &ctx_i->ctx;
	tx = &ctx_i->cmdq.tx;
	eq = &ctx_i->eq_tx;

	skb_get(skb);
	txreq = txreq_cache_get(ndev, num_iov);
	if (!txreq) {
		err = -ENOMEM;
		goto err;
	}
	if (headlen) {
		txreq->iov[0].val[1] = 0;
		txreq->iov[0].start = (u64)skb->data;
		txreq->iov[0].length = headlen;
		txreq->iov[0].v = 1;
		i++;
	}
	for (j = 0; j < nr_frags; j++) {
		struct skb_frag_struct *frag = &skb_shinfo(skb)->frags[j];

		/* FXRTODO: combine virtually continuous fragments */
		txreq->iov[i].val[1] = 0;
		txreq->iov[i].start = (u64)skb_frag_address(frag);
		txreq->iov[i].length = skb_frag_size(frag);
		txreq->iov[i].v = 1;
		i++;
	}
	txreq->iov[0].sp = 1;
	txreq->iov[0].ptl_or_stleep = 1;
	txreq->tail_byte = pad_len;
	txreq->iov[num_iov - 1].val[1] = 0;
	txreq->iov[num_iov - 1].start = (u64)&txreq->tail_byte;
	txreq->iov[num_iov - 1].length = 1;
	txreq->iov[num_iov - 1].v = 1;
	txreq->iov[num_iov - 1].ep = 1;

	/* FXRTODO: Not an effient way to read/update sc5; revisit later */
	sc5 = hfi2_vnic_get_sc(skb->data);
	sl = ppd->sc_to_sl[sc5];
	hfi2_vnic_set_sc(skb->data, sl);

	slots = hfi_format_bypass_dma_cmd(ctx, (u64)txreq->iov, num_iov,
					  (uint64_t)txreq, PTL_MD_RESERVED_IOV,
					  eq, HFI_CT_NONE,
					  vinfo->port_num - 1,
					  sl, ppd->lid, HDR_16B,
					  GENERAL_DMA, &cmd);
	WARN_ON_ONCE(slots != 1);
	spin_lock_irqsave(&tx->lock, sflags);
	if (!hfi_queue_ready(tx, slots, eq)) {
		dd_dev_dbg(dd, "TX send queue %d full\n", q_idx);
		tx_notify_list_add(ctx_i, txreq, vinfo, q_idx);
		vinfo->stats[q_idx].netstats.tx_fifo_errors++;
		spin_unlock_irqrestore(&tx->lock, sflags);
		err = -EBUSY;
	}
	txreq->skb = skb;
	hfi_eq_pending_inc(eq);
	_hfi_command(tx, (u64 *)&cmd, HFI_CMDQ_TX_ENTRIES);
	spin_unlock_irqrestore(&tx->lock, sflags);
	if (unlikely(err && !is_cnp_pkt)) {
		if (err == -ENOMEM)
			vinfo->stats[q_idx].netstats.tx_fifo_errors++;
	}
	skb_pull(skb, OPA_VNIC_HDR_LEN);
	if (unlikely(err == -EBUSY)) {
		hfi2_vnic_maybe_stop_tx(vinfo, q_idx);
		dev_kfree_skb_any(skb);
		return NETDEV_TX_BUSY;
	}

err:
	/* update tx counters */
	if (!is_cnp_pkt)
		hfi2_vnic_update_tx_counters(vinfo, q_idx, skb, err);
	dev_kfree_skb_any(skb);
	return NETDEV_TX_OK;
}

static inline u16 hfi2_vnic_get_vesw_id(void *hdr)
{
	u16 l4_hdr;
	u16 vesw_id;

	l4_hdr = HFI2_VNIC_GET_L4_HDR(hdr);

	if (l4_hdr & HFI2_VNIC_L4_GNI)
		vesw_id = l4_hdr & 0xFF;
	else
		vesw_id = l4_hdr & 0xFFF;
	return vesw_id;
}

/* hfi2_vnic_decap_skb - strip OPA header from the skb (ethernet) packet */
static inline int hfi2_vnic_decap_skb(struct hfi2_vnic_rx_queue *rxq,
				      struct sk_buff *skb)
{
	struct hfi2_vnic_vport_info *vinfo = rxq->vinfo;
	int max_len = vinfo->netdev->mtu + VLAN_ETH_HLEN;
	int rc = -EFAULT;

	skb_pull(skb, OPA_VNIC_HDR_LEN);

	/* Validate Packet length */
	if (unlikely(skb->len > max_len))
		vinfo->stats[rxq->idx].rx_oversize++;
	else if (unlikely(skb->len < ETH_ZLEN))
		vinfo->stats[rxq->idx].rx_runt++;
	else
		rc = 0;
	return rc;
}

static void hfi2_vnic_handle_rx(struct hfi2_vnic_rx_queue *rxq,
				int *work_done, int work_to_do)
{
	struct hfi2_vnic_vport_info *vinfo = rxq->vinfo;
	struct sk_buff *skb;
	int rc;

	while (1) {
		if (*work_done >= work_to_do)
			break;

		skb = skb_dequeue(&rxq->skbq);
		if (unlikely(!skb))
			break;

		rc = hfi2_vnic_decap_skb(rxq, skb);
		/* update rx counters */
		hfi2_vnic_update_rx_counters(vinfo, rxq->idx, skb, rc);
		if (unlikely(rc)) {
			dev_kfree_skb_any(skb);
			continue;
		}

		skb_checksum_none_assert(skb);
		skb->protocol = eth_type_trans(skb, rxq->netdev);

		napi_gro_receive(&rxq->napi, skb);
		(*work_done)++;
	}
}

/* hfi2_vnic_napi - napi receive polling callback function */
static int hfi2_vnic_napi(struct napi_struct *napi, int budget)
{
	struct hfi2_vnic_rx_queue *rxq = container_of(napi,
					      struct hfi2_vnic_rx_queue, napi);
	struct hfi2_vnic_vport_info *vinfo = rxq->vinfo;
	int work_done = 0;

	dd_dev_info(vinfo->dd, "napi %d budget %d\n", rxq->idx, budget);
	hfi2_vnic_handle_rx(rxq, &work_done, budget);

	dd_dev_info(vinfo->dd, "napi %d work_done %d\n", rxq->idx, work_done);
	if (work_done < budget)
		napi_complete(napi);

	return work_done;
}

/*
 * hfi2_vnic_return_cnp constructs a CNP with zero payload by allocating
 * a skb, constructs the 16B L2 header by filling in the header fields
 * from the received pecket with FECN, sets the BECN and queues the packet
 * into the TX queue.
 */
static void hfi2_vnic_return_cnp(void *buf, struct hfi2_vnic_vport_info *vinfo)
{
	struct sk_buff *skb;
	struct opa_vnic_skb_mdata *mdata;
	struct hfi_devdata *dd;
	struct hfi_pportdata *ppd;
	u32 *hdr_cnp;
	struct hfi2_16b_header *hdr;
	u32 slid, dlid;
	u16 len, pkey;
	u8 sc, l4, vl;
	bool fecn, becn;

	hdr = buf;

	slid = hfi2_16B_get_slid(hdr);
	dlid = hfi2_16B_get_dlid(hdr);
	len  = hfi2_16B_get_pkt_len(hdr);
	pkey = hfi2_16B_get_pkey(hdr);
	sc   = hfi2_16B_get_sc(hdr);
	fecn = hfi2_16B_get_fecn(hdr);
	becn = hfi2_16B_get_becn(hdr);
	l4   = hfi2_16B_get_l4(hdr);

	/* FECNs for multicast packets should be ignored */
	if (dlid < HFI1_16B_MULTICAST_LID_BASE) {
		/*
		 * allocate a buffer to construct the CNP
		 * CNP consists of the 16B header and padding bytes
		 */
		dd = vinfo->dd;
		skb = netdev_alloc_skb(vinfo->netdev, HFI2_VNIC_CNP_PKT_LEN);
		hdr_cnp = (u32 *)(skb->data);
		memset(hdr_cnp, 0, HFI2_VNIC_CNP_PKT_LEN);
		memcpy(((u8 *)hdr_cnp + HFI2_VNIC_L4_OFFSET),
		       ((u8 *)hdr + HFI2_VNIC_L4_OFFSET), HFI2_VNIC_L4_HDR_LEN);
		len = HFI2_VNIC_16B_CNP_FLITS;
		fecn = false;
		becn = true;
		ppd = to_hfi_ppd(dd, vinfo->port_num);
		hfi2_make_16b_hdr((struct hfi2_16b_header *)hdr_cnp, ppd->lid,
				  slid, len, pkey, becn, fecn, l4, sc);
		skb_put(skb, HFI2_VNIC_CNP_PKT_LEN);
		vl = ppd->sc_to_vlt[sc];
		mdata = (struct opa_vnic_skb_mdata *)skb_push(skb,
							      sizeof(*mdata));
		mdata->vl = vl;
		mdata->entropy = 0;
		mdata->flags = 0;
		skb->queue_mapping = hfi2_vnic_select_queue(vinfo->netdev, skb,
							    NULL, NULL);
		hfi2_netdev_start_xmit(skb, vinfo->netdev);
	}
}

/*
 * This RX ISR callback does the following:
 * a) queries the EQ for received packets
 * b) figures out the veswport the packet is destined for
 * c) allocates an SKB
 * d) copies the data from the eager buffer into the SKB
 * e) appends the skb to the appropriate RX queue
 * f) notifies the upper layer about a received packet
 * FXRTODO:
 * a) Need an efficient way to enable/disable interrupt notifications
 * b) There is a lot being done in the ISR. do we need to defer the
 * handling to a bottom half instead?
 */
static void hfi2_vnic_rx_isr_cb(struct hfi_eq *eq_rx, void *data)
{
	struct hfi2_ctx_info *ctx_i = data;
	struct hfi_ctx *ctx = &ctx_i->ctx;
	struct hfi2_netdev *ndev = ctx_i->ndev;
	u64 *eq_entry;
	bool dropped;
	union rhf *rhf;
	int l4_type, id = 0;
	struct hfi2_vnic_vport_info *vinfo = NULL;
	void *buf;
	unsigned long sflags;
	struct hfi_cmdq *rx = &ctx_i->cmdq.rx;
	int len, err;
	unsigned char *pad_info = NULL;
	struct sk_buff *skb;
	struct hfi2_vnic_rx_queue *rxq;
	u8 q_idx = ctx_i->q_idx;
	unsigned char pad_i;

retry:
	skb = NULL;
	err = hfi_eq_peek(eq_rx, &eq_entry, &dropped);
	if (err <= 0)
		return;
	rhf = (union rhf *)eq_entry;
	dd_dev_dbg(ndev->dd, "kind %d flit_len %4d egridx %d egroffset %d\n",
		rhf->event_kind, rhf->pktlen,
		rhf->egrindex, rhf->egroffset);

	buf = ctx_i->buf[rhf->egrindex];
	l4_type = HFI2_VNIC_GET_L4_TYPE(buf);
	if (l4_type == HFI2_VNIC_L4_ETHR) {
		id = hfi2_vnic_get_vesw_id(buf);
		/* FXRTODO: Check performance impact of idr_find */
		vinfo = idr_find(&ndev->vesw_idr[0], id);

		/*
		 * In case of invalid vesw id, update the rx_bad_veswid
		 * error count of first available vinfo
		 */
		if (unlikely(!vinfo)) {
			struct hfi2_vnic_vport_info *vinfo_tmp;
			int id_tmp = 0;

			spin_lock_irqsave(&ndev->stats_lock, sflags);
			vinfo_tmp = idr_get_next(&ndev->vesw_idr[0],
						 &id_tmp);
			if (vinfo_tmp)
				vinfo_tmp->stats[0].netstats.rx_nohandler++;
			spin_unlock_irqrestore(&ndev->stats_lock, sflags);
		}
	}
	/* Drop the packet if a vdev is not available */
	if (!vinfo)
		goto pkt_drop;
	/* RHF packet length is in dwords */
	len = rhf->pktlen << 2;

	pad_info = buf + len - 1;
	pad_i = *pad_info;
	len -= pad_i & 0x3f;
	len -= HFI2_VNIC_ICRC_TAIL_LEN;
	/* only handle FECN for vnic packets*/
	if (HFI2_VNIC_IS_FECN_SET(buf))
		hfi2_vnic_return_cnp(buf, vinfo);
	/* BECNs are expected only for vnic packets */
	if (HFI2_VNIC_IS_BECN_SET(buf) &&
	    (len == (HFI2_VNIC_16B_HDR_LEN))) {
		dd_dev_dbg(ndev->dd, "Dropping CNP\n");
		goto pkt_drop;
	}

	rxq = &vinfo->rxq[q_idx];
	if (unlikely(!netif_oper_up(vinfo->netdev))) {
		vinfo->stats[q_idx].rx_drop_state++;
		skb_queue_purge(&rxq->skbq);
		return;
	}
	if (unlikely(skb_queue_len(&rxq->skbq) > HFI2_VNIC_RCV_Q_SIZE)) {
		vinfo->stats[q_idx].netstats.rx_fifo_errors++;
		return;
	}
	/*
	 * Allocate the skb and then copy the data from the eager
	 * buffer into the skb
	 */
	skb = netdev_alloc_skb(vinfo->netdev, len);
	if (unlikely(!skb)) {
		vinfo->stats[q_idx].netstats.rx_fifo_errors++;
		return;
	}
	memcpy(skb->data, buf, len);
	skb_put(skb, len);
	skb_queue_tail(&rxq->skbq, skb);
	dd_dev_dbg(ndev->dd, "RX%d kind %d skb->len %4d flit_len %4d (pad & 0x3f) %d egridx %d egroffset %d\n",
		   q_idx, rhf->event_kind, skb->len, rhf->pktlen << 2,
		   pad_i & 0x3f,
		   rhf->egrindex, rhf->egroffset);
	if (napi_schedule_prep(&rxq->napi)) {
		dd_dev_dbg(vinfo->dd, "napi %d scheduling\n", q_idx);
		__napi_schedule(&rxq->napi);
	}

pkt_drop:
	spin_lock_irqsave(&rx->lock, sflags);
	err = hfi_pt_update_eager(ctx, rx, rhf->egrindex);
	spin_unlock_irqrestore(&rx->lock, sflags);
	if (err < 0) {
		/*
		 * only potential error is DROPPED event on EQ0 to
		 * confirm the PT_UPDATE, print error and ignore.
		 */
		dd_dev_err(ndev->dd, "unexpected PT update error %d\n", err);
	}
	hfi_eq_advance(eq_rx, eq_entry);

	/* Check if there are more events */
	goto retry;
}

/* Free any RX buffers previously allocated */
static void hfi2_free_rx_bufs(struct hfi2_ctx_info *ctx_i)
{
	int i = 0;

	for (i = 0; i < HFI2_NET_NUM_RX_BUFS; i++) {
		if (ctx_i->buf[i]) {
			hfi_at_dereg_range(&ctx_i->ctx,
					   ctx_i->buf[i],
					   HFI2_NET_EAGER_SIZE);
			vfree(ctx_i->buf[i]);
			ctx_i->buf[i] = NULL;
		}
	}
}

/* Allocate and append RX buffers */
static int hfi2_alloc_rx_bufs(struct hfi2_ctx_info *ctx_i)
{
	int i, rc = 0;

	for (i = 0; i < HFI2_NET_NUM_RX_BUFS; i++) {
		ctx_i->buf[i] = vzalloc_node(HFI2_NET_EAGER_SIZE,
					     ctx_i->ctx.devdata->node);
		if (!ctx_i->buf[i]) {
			rc = -ENOMEM;
			goto err1;
		}

		hfi_at_reg_range(&ctx_i->ctx, ctx_i->buf[i],
				 HFI2_NET_EAGER_SIZE, NULL, true);
		rc = hfi2_vnic_append_skb(ctx_i, i);
		if (rc)
			goto err1;
	}
	return rc;
err1:
	hfi2_free_rx_bufs(ctx_i);
	return rc;
}

static int hfi2_vnic_init_ctx(struct hfi2_vnic_vport_info *vinfo, int ctx_num)
{

	struct hfi_devdata *dd = vinfo->dd;
	struct hfi2_netdev *ndev = dd->vnic;
	struct hfi2_ctx_info *ctx_i;
	struct hfi_ctx *ctx;
	struct hfi_cmdq *rx;
	struct opa_ctx_assign ctx_assign = {0};
	struct opa_ev_assign eq_alloc_rx = {0};
	struct opa_ev_assign eq_alloc_tx = {0};
	struct hfi_pt_alloc_eager_args pt_alloc = {0};
	int rc;
	unsigned long flags;

	ctx_i = &ndev->eth_ctx[ctx_num];
	ctx_i->ndev = vinfo->dd->vnic;
	ctx_i->q_idx = ctx_num;
	ctx = &ctx_i->ctx;
	INIT_LIST_HEAD(&ctx_i->tx_notify_list);
	hfi_ctx_init(ctx, vinfo->dd);
	/* EEPH requires default bypass context whereas STLEEP uses RSM */
	ctx->mode |= HFI_CTX_MODE_USE_BYPASS;
	ctx_assign.pid = HFI_PID_ANY;
	ctx_assign.le_me_count = HFI2_NET_NUM_RX_BUFS;
	ctx_assign.flags |= HFI_CTX_FLAG_USE_BYPASS;
	rc = hfi_ctx_attach(ctx, &ctx_assign);
	if (rc)
		return rc;
	/*
	 * FXRTODO: Obtain a pair of command queues and set it up.
	 * The pair of CMDQs is shared across both ports since there are
	 * only 192 CMDQs for the entire chip. It might be better to have
	 * separate CMDQs for each port for scalability reasons as a
	 * potential future performance optimization.
	 */
	rc = hfi_cmdq_assign(&ctx_i->cmdq, ctx, NULL);
	if (rc)
		goto err;
	rc = hfi_cmdq_map(&ctx_i->cmdq);
	if (rc)
		goto err1;
	rx = &ctx_i->cmdq.rx;

	/* TX EQ can be allocated per netdev */
	eq_alloc_tx.ni = HFI_NI_BYPASS;
	eq_alloc_tx.count = HFI_CMDQ_TX_ENTRIES * 2;
	eq_alloc_tx.isr_cb = hfi2_vnic_tx_isr_cb;
	eq_alloc_tx.cookie = ctx_i;
	rc = _hfi_eq_alloc(ctx, &eq_alloc_tx, &ctx_i->eq_tx);
	if (rc)
		goto err2;
	eq_alloc_rx.ni = HFI_NI_BYPASS;
	eq_alloc_rx.count = HFI2_NET_NUM_RX_BUFS;
	eq_alloc_rx.isr_cb = hfi2_vnic_rx_isr_cb;
	eq_alloc_rx.cookie = ctx_i;
	rc = _hfi_eq_alloc(ctx, &eq_alloc_rx, &ctx_i->eq_rx);
	if (rc)
		goto err3;
	pt_alloc.eager_order = ilog2(HFI2_NET_NUM_RX_BUFS) - 2;
	pt_alloc.eq_handle = &ctx_i->eq_rx;

	spin_lock_irqsave(&rx->lock, flags);
	rc = hfi_pt_alloc_eager(ctx, rx, &pt_alloc);
	spin_unlock_irqrestore(&rx->lock, flags);
	if (rc < 0)
		goto err4;

	rc = hfi2_alloc_rx_bufs(ctx_i);
	if (rc < 0)
		goto err5;

	return 0;
err5:
	spin_lock_irqsave(&rx->lock, flags);
	hfi_pt_disable(ctx, rx, HFI_NI_BYPASS, HFI_PT_BYPASS_EAGER);
	spin_unlock_irqrestore(&rx->lock, flags);
err4:
	_hfi_eq_free(&ctx_i->eq_rx);
err3:
	_hfi_eq_free(&ctx_i->eq_tx);
err2:
	hfi_cmdq_unmap(&ctx_i->cmdq);
err1:
	hfi_cmdq_release(&ctx_i->cmdq);
err:
	hfi_ctx_cleanup(ctx);
	dd_dev_err(vinfo->dd, "%s rc %d\n", __func__, rc);
	return rc;
}

static void hfi2_vnic_uninit_ctx(struct hfi2_vnic_vport_info *vinfo,
				 int ctx_num)
{
	struct hfi_devdata *dd = vinfo->dd;
	struct hfi2_netdev *ndev = dd->vnic;
	struct hfi2_ctx_info *ctx_i;
	struct hfi_ctx *ctx;
	struct hfi_cmdq *rx;
	unsigned long flags;

	ctx_i = &ndev->eth_ctx[ctx_num];
	ctx = &ctx_i->ctx;
	rx = &ctx_i->cmdq.rx;
	/* FXRTODO: Release EQ before disabling PT to prevent EQ ISR
	 * firing while PT is being disabled
	 */
	_hfi_eq_free(&ctx_i->eq_rx);
	spin_lock_irqsave(&rx->lock, flags);
	hfi_pt_disable(ctx, rx, HFI_NI_BYPASS, HFI_PT_BYPASS_EAGER);
	spin_unlock_irqrestore(&rx->lock, flags);
	hfi2_free_rx_bufs(ctx_i);
	tx_notify_list_process(ctx_i, true);
	hfi_cmdq_unmap(&ctx_i->cmdq);
	hfi_cmdq_release(&ctx_i->cmdq);
	_hfi_eq_free(&ctx_i->eq_tx);
	hfi_ctx_cleanup(ctx);
}

static int hfi2_vnic_init(struct hfi2_vnic_vport_info *vinfo)
{
	struct hfi_devdata *dd = vinfo->dd;
	struct hfi2_netdev *ndev = dd->vnic;
	struct hfi_ctx *rsm_ctx[HFI2_MAX_NUM_VNIC_CTXT];
	int rc = 0, i, j, num_ctx;
	struct hfi_rsm_rule rule = {0};

	mutex_lock(&ndev->ctx_lock);
	if (ndev->num_vports)
		goto finish;
	num_ctx = HFI2_NUM_VNIC_CTXT;
	for (i = 0; i < ndev->num_ports; i++)
		idr_init(&ndev->vesw_idr[i]);

	for (i = 0; i < num_ctx; i++) {
		rc = hfi2_vnic_init_ctx(vinfo, i);
		if (rc) {
			dd_dev_err(dd, "alloc_ctx fail rc %d\n", rc);
			goto fail;
		}
		rsm_ctx[i] = &ndev->eth_ctx[i].ctx;
	}

	/* set RSM rule for 16B STLEEP using L2 and L4 */
	rule.idx = HFI_VNIC_RSM_RULE_IDX;
	/* bypass */
	rule.pkt_type = 0x4;
	/* match L2 == 16B (0x1)*/
	rule.match_offset[0] = 61;
	rule.match_mask[0] = HFI2_BYPASS_L2_MASK;
	rule.match_value[0] = HFI2_BYPASS_HDR_16B;
	/* match 16B STLEEP L4: 0x78 */
	rule.match_offset[1] = 64;
	rule.match_mask[1] = HFI2_VNIC_L4_TYPE_MASK;
	rule.match_value[1] = HFI2_VNIC_L4_ETHR;

	rule.select_width[0] = ilog2(HFI2_NUM_VNIC_CTXT);
	rule.select_offset[0] = L4_HDR_VESWID_OFFSET;
	rule.select_width[1] = ilog2(HFI2_NUM_VNIC_CTXT);
	rule.select_offset[1] = L2_ENTROPY_OFFSET;
	rc = hfi_rsm_set_rule(dd, &rule, rsm_ctx,
			      HFI2_NUM_VNIC_CTXT);
	if (rc) {
		dd_dev_err(dd, "rsm fail rc %d\n", rc);
		goto fail;
	}

finish:
	ndev->num_vports++;
	mutex_unlock(&ndev->ctx_lock);
	return 0;
fail:
	mutex_unlock(&ndev->ctx_lock);
	for (j = 0; j < i; j++)
		hfi2_vnic_uninit_ctx(vinfo, j);
	dd_dev_err(dd, "%s rc %d\n", __func__, rc);
	return rc;
}

static void hfi2_vnic_deinit(struct hfi2_vnic_vport_info *vinfo)
{
	int i;
	struct hfi2_netdev *ndev = vinfo->dd->vnic;

	mutex_lock(&ndev->ctx_lock);
	if (!--ndev->num_vports) {
		for (i = 0; i < HFI2_NUM_VNIC_CTXT; i++)
			hfi2_vnic_uninit_ctx(vinfo, i);
		hfi_rsm_clear_rule(ndev->dd, 1);
		for (i = 0; i < ndev->num_ports; i++)
			idr_destroy(&ndev->vesw_idr[i]);
	}
	mutex_unlock(&ndev->ctx_lock);
}

static int hfi2_vnic_up(struct hfi2_vnic_vport_info *vinfo)
{
	int rc, i;
	struct hfi_devdata *dd = vinfo->dd;

	/* ensure virtual eth switch id is valid */
	if (!vinfo->vesw_id)
		return -EINVAL;

	/* ensure virtual eth switch id is valid */
	rc = idr_alloc(&dd->vnic->vesw_idr[vinfo->port_num - 1], vinfo,
		       vinfo->vesw_id, vinfo->vesw_id + 1, GFP_NOWAIT);

	if (rc < 0) {
		dd_dev_err(vinfo->dd, "%s %d rc %d vesw %d port_num %d\n",
			   __func__, __LINE__, rc, vinfo->vesw_id,
			   vinfo->port_num);
		return rc;
	}
	for (i = 0; i < vinfo->num_rx_q; i++) {
		struct hfi2_vnic_rx_queue *rxq = &vinfo->rxq[i];

		skb_queue_head_init(&rxq->skbq);
		napi_enable(&rxq->napi);
	}

	netif_carrier_on(vinfo->netdev);
	netif_tx_start_all_queues(vinfo->netdev);
	set_bit(HFI2_VNIC_UP, &vinfo->flags);

	return 0;
}

static void hfi2_vnic_down(struct hfi2_vnic_vport_info *vinfo)
{
	int i;
	struct hfi_devdata *dd = vinfo->dd;

	clear_bit(HFI2_VNIC_UP, &vinfo->flags);
	netif_carrier_off(vinfo->netdev);
	netif_tx_disable(vinfo->netdev);
	idr_remove(&dd->vnic->vesw_idr[0], vinfo->vesw_id);

	/* remove unread skbs */
	for (i = 0; i < vinfo->num_rx_q; i++) {
		struct hfi2_vnic_rx_queue *rxq = &vinfo->rxq[i];

		napi_disable(&rxq->napi);
		skb_queue_purge(&rxq->skbq);
	}

}

static int hfi2_netdev_open(struct net_device *netdev)
{
	struct hfi2_vnic_vport_info *vinfo = opa_vnic_dev_priv(netdev);
	int rc;

	mutex_lock(&vinfo->lock);
	rc = hfi2_vnic_up(vinfo);
	mutex_unlock(&vinfo->lock);
	return rc;
}

static int hfi2_netdev_close(struct net_device *netdev)
{
	struct hfi2_vnic_vport_info *vinfo = opa_vnic_dev_priv(netdev);

	mutex_lock(&vinfo->lock);
	if (test_bit(HFI2_VNIC_UP, &vinfo->flags))
		hfi2_vnic_down(vinfo);
	mutex_unlock(&vinfo->lock);
	return 0;
}

static void hfi2_vnic_set_vesw_id(struct net_device *netdev, int id)
{
	struct hfi2_vnic_vport_info *vinfo = opa_vnic_dev_priv(netdev);
	bool reopen = false;

	/*
	 * If vesw_id is being changed, and if the vnic port is up,
	 * reset the vnic port to ensure new vesw_id gets picked up
	 */
	if (id != vinfo->vesw_id) {
		mutex_lock(&vinfo->lock);
		if (test_bit(HFI2_VNIC_UP, &vinfo->flags)) {
			hfi2_vnic_down(vinfo);
			reopen = true;
		}

		vinfo->vesw_id = id;
		if (reopen)
			hfi2_vnic_up(vinfo);

		mutex_unlock(&vinfo->lock);
	}
}

/* netdev ops */
static const struct net_device_ops hfi2_netdev_ops = {
	.ndo_open = hfi2_netdev_open,
	.ndo_stop = hfi2_netdev_close,
	.ndo_start_xmit = hfi2_netdev_start_xmit,
	.ndo_select_queue = hfi2_vnic_select_queue,
	.ndo_get_stats64 = hfi2_vnic_get_stats64,
};

static void hfi2_vnic_free_rn(struct net_device *netdev)
{
	struct hfi2_vnic_vport_info *vinfo = opa_vnic_dev_priv(netdev);

	hfi2_vnic_deinit(vinfo);
	mutex_destroy(&vinfo->lock);
	free_netdev(netdev);
}

struct net_device *hfi2_vnic_alloc_rn(struct ib_device *device,
				      u8 port_num,
				      enum rdma_netdev_t type,
				      const char *name,
				      unsigned char name_assign_type,
				      void (*setup)(struct net_device *))
{
	struct hfi_devdata *dd = hfi_dd_from_ibdev(device);
	struct hfi2_vnic_vport_info *vinfo;
	struct net_device *netdev;
	struct rdma_netdev *rn;
	int i, size, rc;

	if (!port_num || (port_num > dd->num_pports))
		return ERR_PTR(-EINVAL);

	if (type != RDMA_NETDEV_OPA_VNIC)
		return ERR_PTR(-EOPNOTSUPP);

	size = sizeof(struct opa_vnic_rdma_netdev) + sizeof(*vinfo);
	netdev = alloc_netdev_mqs(size, name, name_assign_type, setup,
				  HFI2_NUM_VNIC_CTXT, HFI2_NUM_VNIC_CTXT);
	if (!netdev)
		return ERR_PTR(-ENOMEM);

	rn = netdev_priv(netdev);
	vinfo = opa_vnic_dev_priv(netdev);
	vinfo->dd = dd;
	vinfo->num_tx_q = HFI2_NUM_VNIC_CTXT;
	vinfo->num_rx_q = HFI2_NUM_VNIC_CTXT;
	vinfo->netdev = netdev;
	vinfo->port_num = port_num;
	rn->free_rdma_netdev = hfi2_vnic_free_rn;
	rn->set_id = hfi2_vnic_set_vesw_id;

	netdev->features = NETIF_F_HIGHDMA | NETIF_F_SG;
	netdev->hw_features = netdev->features;
	netdev->vlan_features = netdev->features;
	netdev->watchdog_timeo = msecs_to_jiffies(HFI_TX_TIMEOUT_MS);
	netdev->netdev_ops = &hfi2_netdev_ops;
	mutex_init(&vinfo->lock);

	for (i = 0; i < vinfo->num_rx_q; i++) {
		struct hfi2_vnic_rx_queue *rxq = &vinfo->rxq[i];

		rxq->idx = i;
		rxq->vinfo = vinfo;
		rxq->netdev = netdev;
		netif_napi_add(netdev, &rxq->napi, hfi2_vnic_napi, 64);
	}

	rc = hfi2_vnic_init(vinfo);
	if (rc)
		goto init_fail;

	return netdev;
init_fail:
	mutex_destroy(&vinfo->lock);
	free_netdev(netdev);
	return ERR_PTR(rc);
}

/*
 * Vnic per hfi device initialization.
 * Call hfi_vnic_uninit() to cleanup vnic
 * data if error is returned.
 */
int hfi_vnic_init(struct hfi_devdata *dd)
{
	struct hfi2_netdev *ndev;
	int rc = 0;

	ndev = kzalloc(sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;

	dd->vnic = ndev;
	ndev->dd = dd;
	ndev->num_ports = dd->num_pports;
	mutex_init(&ndev->ctrl_lock);
	mutex_init(&ndev->ctx_lock);
	spin_lock_init(&ndev->stats_lock);

	rc = txreq_cache_create(ndev);
	if (rc)
		dd_dev_err(dd, "Failed to init vnic tx cache, ret %d\n", rc);
	return rc;
}

/* Vnic per hfi device cleanup */
void hfi_vnic_uninit(struct hfi_devdata *dd)
{
	struct hfi2_netdev *ndev = dd->vnic;

	if (!ndev)
		return;

	dd->vnic = NULL;
	txreq_cache_destroy(ndev);
	kfree(ndev);
}
