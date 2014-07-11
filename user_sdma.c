/*
 * Copyright (c) 2014 Intel Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/dmapool.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/uio.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/mmu_context.h>

#include "hfi.h"
#include "sdma.h"
#include "user_sdma.h"
#include "sdma.h"
#include "verbs.h"  /* for the headers */
#include "common.h" /* for struct hfi_tid_info */
#include "trace.h"

/*
 * Debugging aid - allows for quick switching between
 * inline function and non-inline fuctions (which
 * improve debuggability by making the stack traces
 * a bit easier to decode).
 */
#define _hfi_inline inline

/* The maximum number of Data io vectors per message/request */
#define MAX_VECTORS_PER_REQ 8
/*
 * Maximum number of packet to send from each message/request
 * before moving to the next one.
 */
#define MAX_PKTS_PER_QUEUE 16

#define num_pages(x) (1 + ((((x) - 1) & PAGE_MASK) >> PAGE_SHIFT))

#define REQ_VERSION_MASK 0xF
#define REQ_VERSION_SHIFT 0x0
#define REQ_OPCODE_MASK 0xF
#define REQ_OPCODE_SHIFT 0x4
#define REQ_IOVCNT_MASK 0xFF
#define REQ_IOVCNT_SHIFT 0x8
#define req_opcode(x) (((x) >> REQ_OPCODE_SHIFT) & REQ_OPCODE_MASK)
#define req_version(x) (((x) >> REQ_VERSION_SHIFT) & REQ_OPCODE_MASK)
#define req_iovcnt(x) (((x) >> REQ_IOVCNT_SHIFT) & REQ_IOVCNT_MASK)

/*
 * Define fields in the KDETH header so we can update the header
 * template.
 */
#define KDETH_OFFSET_SHIFT        0
#define KDETH_OFFSET_MASK         0x7fff
#define KDETH_OM_SHIFT            15
#define KDETH_OM_MASK             0x1
#define KDETH_TID_SHIFT           16
#define KDETH_TID_MASK            0x3ff
#define KDETH_TIDCTRL_SHIFT       26
#define KDETH_TIDCTRL_MASK        0x3
#define KDETH_HCRC_UPPER_SHIFT    16
#define KDETH_HCRC_UPPER_MASK     0xff
#define KDETH_HCRC_LOWER_SHIFT    24
#define KDETH_HCRC_LOWER_MASK     0xff

#define KDETH_GET(val, field)						\
	(((le32_to_cpu((val))) >> KDETH_##field##_SHIFT) & KDETH_##field##_MASK)
#define KDETH_SET(dw, field, val) do {					\
		u32 dwval = le32_to_cpu(dw);				\
		dwval &= ~(KDETH_##field##_MASK << KDETH_##field##_SHIFT); \
		dwval |= (((val) & KDETH_##field##_MASK) << \
			  KDETH_##field##_SHIFT);			\
		dw = cpu_to_le32(dwval);				\
	} while (0)

/* KDETH OM multipliers and switch over point */
#define KDETH_OM_SMALL     4
#define KDETH_OM_LARGE     64
#define KDETH_OM_MAX_SIZE  ((1 << (KDETH_OM_LARGE / KDETH_OM_SMALL)) - 1)

/* Last fragment/packet of a io vector */
#define USER_SDMA_TXREQ_FLAGS_LAST_FRAG  (1 << 0)
/* Last fragment/packet in the request */
#define USER_SDMA_TXREQ_FLAGS_LAST_PKT   (1 << 1)
/* First packet in the request */
#define USER_SDMA_TXREQ_FLAGS_FIRST_PKT  (1 << 2)
/* Secont packet in the request. This is useful for AHG */
#define USER_SDMA_TXREQ_FLAGS_SECOND_PKT (1 << 3)
/* Third packet in the request. This is the first packet
 * in the sequence that has a "static" size that can be used
 * for the rest of the packets (besides the last one). */
#define USER_SDMA_TXREQ_FLAGS_THIRD_PKT  (1 << 4)
/* The TX descriptor uses AHG */
#define USER_SDMA_TXREQ_FLAGS_USE_AHG    (1 << 5)

#define SDMA_REQ_IN_USE     0
#define SDMA_REQ_FOR_THREAD 1
#define SDMA_REQ_SEND_DONE  2
#define SDMA_REQ_HAVE_AHG   3
#define SDMA_REQ_CB_DONE    4

#define SDMA_PKT_Q_INACTIVE (1 << 0)
#define SDMA_PKT_Q_ACTIVE   (1 << 1)
#define SDMA_PKT_Q_DEFERRED (1 << 2)

enum sdma_req_opcode {
	EAGER = 0,
	EXPECTED
};

unsigned user_sdma_use_ahg = 0;
unsigned initial_pkt_count = 8;

#define SDMA_IOWAIT_TIMEOUT 1000 /* in milliseconds */

struct sdma_req_info {
	/*
	 * bits 0-3 - version (currently unused)
	 * bits 4-7 - opcode (enum sdma_req_opcode)
	 * bits 8-15 - io vector count
	 */
	u16 ctrl;
	/*
	 * Number of fragments contained in this request.
	 * User-space has already computed how many
	 * fragment-sized packet the user buffer will be
	 * split into.
	 */
	u16 npkts;
	/*
	 * Size of each fragment the user buffer will be
	 * split into.
	 */
	u16 fragsize;
	/*
	 * Index of the slot in the SDMA completion ring
	 * this request should be using. User-space is
	 * in charge of managing its own ring.
	 */
	u16 comp_idx;
} __packed;

struct user_sdma_iovec {
	struct iovec iov;
	/* number of pages in this vector */
	unsigned npages;
	/* array of pinned pages for this vector */
	struct page **pages;
	/* number of fragments (packet) in this vector */
	unsigned num_frags;
	/* number of fragments sent (submitted to the SDMA queue */
	unsigned frags_sent;
	/* offset into the virtual address space of the vector at
	 * which we last left off. */
	u64 offset;
};

struct user_sdma_request {
	struct sdma_req_info info;
	struct hfi_user_sdma_pkt_q *pq;
	struct hfi_user_sdma_comp_q *cq;
	/* This is the original header from user space */
	struct hfi_pio_hdr hdr;
	/*
	 * Pointer to the SDMA engine for this request.
	 * Since different request could be on different VLs,
	 * each request will need it's own engine pointer.
	 */
	struct sdma_engine *sde;
	u8 ahg_idx;
	u32 ahg[10];
	/*
	 * KDETH.Offset (Eager) field
	 * We need to remember the initial value so the headers
	 * can be updated properly.
	 */
	u32 koffset;
	/*
	 * KDETH.OFFSET (TID) field
	 * The offset can cover multiple packets, depending on the
	 * size of the TID entry.
	 */
	u32 tidoffset;
	/*
	 * KDETH.OM
	 * Remember this because the header template always sets it
	 * to 0.
	 */
	u8 omfactor;
	/*
	 * pointer to the user's task_struct. We are going to
	 * get a reference to it so we can process io vectors
	 * at a later time.
	 */
	struct task_struct *user_proc;
	/*
	 * pointer to the user's mm_struct. We are going to
	 * get a reference to it so it doesn't get freed
	 * since we might not be in process context when we
	 * are processing the iov's.
	 * Using this mm_struct, we can get vma based on the
	 * iov's address (find_vma()).
	 */
	struct mm_struct *user_mm;
	/*
	 * We copy the iovs for this request (based on
	 * info.iovcnt). These are only the data vectors
	 */
	unsigned data_iovs;
	/* total length of the data in the request */
	u32 data_len;
	/* progress index moving along the iovs array */
	unsigned long iov_idx;
	struct user_sdma_iovec iovs[MAX_VECTORS_PER_REQ];
	/* number of elements copied to the tids array */
	u8 n_tids;
	/* TID array values copied from the tid_iov vector */
	u32 tids[1024];
	u8 tididx;
	u32 sent;
	/* number of txreqs sent to the queue */
	u16 txreqs_sent;
	/* number of txreqs completed (callback was called) */
	u16 txreqs_completed;
	spinlock_t list_lock;
	struct list_head txps;
	unsigned long flags;
};

struct user_sdma_txreq {
	/* Packet header for the txreq */
	struct hfi_pio_hdr hdr ____cacheline_aligned_in_smp;
	struct list_head list;
	struct sdma_txreq txreq;
	struct user_sdma_request *req;
	struct user_sdma_iovec *iovec;
	spinlock_t lock;
	u16 flags;
};

#define SDMA_DBG(req, fmt, ...)				     \
	hfi_cdbg(SDMA, "[%u:%u:%u:%u] " fmt, (req)->pq->dd->unit, \
		 (req)->pq->ctxt, (req)->pq->subctxt, (req)->info.comp_idx, \
		 ##__VA_ARGS__)
#define SDMA_Q_DBG(pq, fmt, ...)			 \
	hfi_cdbg(SDMA, "[%u:%u:%u] " fmt, (pq)->dd->unit, (pq)->ctxt, \
		 (pq)->subctxt, ##__VA_ARGS__)

static int user_sdma_send_pkts(struct user_sdma_request *, unsigned);
static int num_user_pages(const struct iovec *);
static void user_sdma_txreq_cb(struct sdma_txreq *, int, int);
static void user_sdma_free_request(struct user_sdma_request *);
static int pin_vector_pages(struct user_sdma_request *,
			    struct user_sdma_iovec *);
static _hfi_inline void unpin_vector_pages(struct user_sdma_iovec *);
static int check_header_template(struct user_sdma_request *,
				 struct hfi_pio_hdr *, u32, u32);
static int set_txreq_header(struct user_sdma_request *,
			    struct user_sdma_txreq *, u32);
static int set_txreq_header_ahg(struct user_sdma_request *,
			       struct user_sdma_txreq *, u32);
static _hfi_inline void set_comp_state(struct user_sdma_request *,
				       enum hfi_sdma_comp_state, int);
#if 0
static int user_sdma_progress(void *);
static int defer_packet_queue(struct iowait *, struct sdma_txreq *);
#endif
static void activate_packet_queue(struct iowait *, int);

#if 0
static int defer_packet_queue(struct iowait *wait, struct sdma_txreq *txreq)
{
	struct hfi_user_sdma_pkt_q *pq =
		container_of(wait, struct hfi_user_sdma_pkt_q, busy);

	SDMA_Q_DBG(pq, "Putting queue to sleep");
	if (cmpxchg(&pq->state, SDMA_PKT_Q_ACTIVE, SDMA_PKT_Q_DEFERRED) ==
	    SDMA_PKT_Q_ACTIVE)
		return -EBUSY;
	return -EINVAL;
}
#endif

static void activate_packet_queue(struct iowait *wait, int reason)
{
	struct hfi_user_sdma_pkt_q *pq =
		container_of(wait, struct hfi_user_sdma_pkt_q, busy);
#if 0
	struct qib_ctxtdata *uctxt = pq->dd->rcd[pq->ctxt];

	if (cmpxchg(&pq->state, SDMA_PKT_Q_DEFERRED, SDMA_PKT_Q_ACTIVE) ==
	    SDMA_PKT_Q_DEFERRED)
		wake_up_process(uctxt->progress);
	else
		SDMA_Q_DBG(pq, "Wrong queue state %u, expected %u",
			   pq->state, SDMA_PKT_Q_DEFERRED);
#endif
	xchg(&pq->state, SDMA_PKT_Q_ACTIVE);
	wake_up(&wait->wait_dma);
};

int hfi_user_sdma_alloc_queues(struct qib_ctxtdata *uctxt, struct file *fp,
			       u16 size)
{
	int ret = 0;
	unsigned memsize;
	char buf[64];
	struct hfi_devdata *dd;
	struct hfi_user_sdma_comp_q *cq;
	struct hfi_user_sdma_pkt_q *pq;
	unsigned long flags;

	if (!uctxt || !fp) {
		ret = -EBADF;
		goto done;
	}

	if (!size) {
		ret = -EINVAL;
		goto done;
	}

	dd = uctxt->dd;

	pq = kzalloc(sizeof(*pq), GFP_KERNEL);
	if (!pq) {
		dd_dev_err(dd,
			   "[%u:%u] Failed to allocate SDMA request struct\n",
			   uctxt->ctxt, subctxt_fp(fp));
		ret = -ENOMEM;
		goto done;
	}
	memsize = sizeof(*pq->reqs) * size;
	pq->reqs = kmalloc(memsize, GFP_KERNEL);
	if (!pq->reqs) {
		dd_dev_err(dd,
			   "[%u:%u] Failed to allocate SDMA request queue\n",
			   uctxt->ctxt, subctxt_fp(fp));
		ret = -ENOMEM;
		goto done;
	}
	INIT_LIST_HEAD(&pq->list);
	pq->dd = dd;
	pq->ctxt = uctxt->ctxt;
	pq->subctxt = subctxt_fp(fp);
	pq->n_max_reqs = size;
	pq->state = SDMA_PKT_Q_INACTIVE;
	atomic_set(&pq->n_reqs, 0);
	atomic64_set(&pq->npkts, 0);
#if 0
	iowait_init(&pq->busy, 0, NULL,
		    defer_packet_queue,
		    activate_packet_queue);
#endif
	iowait_init(&pq->busy, 0, NULL, NULL, activate_packet_queue);
	pq->reqidx = 0;
	snprintf(buf, 64, "txreq-kmem-cache-%u-%u-%u", dd->unit, uctxt->ctxt,
		 subctxt_fp(fp));
	pq->txreq_cache = kmem_cache_create(buf,
			       sizeof(struct user_sdma_txreq),
					    L1_CACHE_BYTES,
					    SLAB_HWCACHE_ALIGN,
					    NULL);
	if (!pq->txreq_cache) {
		dd_dev_err(dd, "[%u] Failed to allocate TxReq cache\n",
			   uctxt->ctxt);
		ret = -ENOMEM;
		goto done;
	}
	user_sdma_pkt_fp(fp) = pq;
	cq = kzalloc(sizeof(*cq), GFP_KERNEL);
	if (!cq) {
		dd_dev_err(dd,
			   "[%u:%u] Failed to allocate SDMA completion queue\n",
			   uctxt->ctxt, subctxt_fp(fp));
		ret = -ENOMEM;
		goto done;
	}

	memsize = ALIGN(sizeof(*cq->comps) * size, PAGE_SIZE);
	cq->comps = vmalloc_user(memsize);
	if (!cq->comps) {
		dd_dev_err(dd,
		      "[%u:%u] Failed to allocate SDMA completion queue entries\n",
		      uctxt->ctxt, subctxt_fp(fp));
		ret = -ENOMEM;
		goto done;
	}
	cq->nentries = size;
	user_sdma_comp_fp(fp) = cq;

#if 0
	/* Allocate the task (only if master context). However, we don't need
	 * to start it until the first request arrives. */
	if (!subctxt_fp(fp)) {
		uctxt->progress =
			kthread_create_on_node(user_sdma_progress,
					       (void *)uctxt,
					       uctxt->numa_id, "hfiusdma%u:%u",
					       dd->unit, uctxt->ctxt);
		if (IS_ERR(uctxt->progress)) {
			dd_dev_err(dd,
				   "[%u:%u] Failed to create progress thread\n",
				   dd->unit, uctxt->ctxt);
			ret = PTR_ERR(uctxt->progress);
			goto done;
		}
	}
	/*
	 * This should start the thread, which should immediately put itself
	 * to sleep since there is nothing to do yet.
	 */
	wake_up_process(uctxt->progress);
#endif
	spin_lock_irqsave(&uctxt->sdma_qlock, flags);
	list_add(&pq->list, &uctxt->sdma_queues);
	spin_unlock_irqrestore(&uctxt->sdma_qlock, flags);
done:
	return ret;
}

int hfi_user_sdma_free_queues(struct hfi_filedata *fd)
{
	struct qib_ctxtdata *uctxt = fd->uctxt;
	struct hfi_user_sdma_pkt_q *pq;
	unsigned long flags;

	hfi_cdbg(SDMA, "[%u:%u:%u] Freeing user SDMA queues", uctxt->dd->unit,
		 uctxt->ctxt, fd->subctxt);
#if 0
	if (!fd->subctxt && uctxt->progress)
		kthread_stop(uctxt->progress);
#endif
	pq = fd->pq;
	if (pq) {
		u16 i, j;
		spin_lock_irqsave(&uctxt->sdma_qlock, flags);
		if (!list_empty(&pq->list))
			list_del_init(&pq->list);
		spin_unlock_irqrestore(&uctxt->sdma_qlock, flags);
		iowait_sdma_drain(&pq->busy);
		if (pq->reqs) {
			for (i = 0, j = 0; i < atomic_read(&pq->n_reqs) &&
				     j < pq->n_max_reqs; j++) {
				struct user_sdma_request *req = &pq->reqs[j];
				if (test_bit(SDMA_REQ_IN_USE, &req->flags)) {
					set_comp_state(req, ERROR, -ECOMM);
					user_sdma_free_request(req);
					i++;
				}
			}
			kfree(pq->reqs);
		}
		if (pq->txreq_cache)
			kmem_cache_destroy(pq->txreq_cache);
		kfree(pq);
		fd->pq = NULL;
	}
	if (fd->cq) {
		if (fd->cq->comps)
			vfree(fd->cq->comps);
		kfree(fd->cq);
		fd->cq = NULL;
	}
	return 0;
}

int hfi_user_sdma_process_request(struct file *fp, struct iovec *iovec,
				  unsigned long dim, unsigned long *count)
{
	int ret = 0, i = 0, sent;
	struct qib_ctxtdata *uctxt = ctxt_fp(fp);
	struct hfi_user_sdma_pkt_q *pq = user_sdma_pkt_fp(fp);
	struct hfi_user_sdma_comp_q *cq = user_sdma_comp_fp(fp);
	struct hfi_devdata *dd = pq->dd;
	unsigned long idx = 0;
	u8 pcount = initial_pkt_count;
	struct sdma_req_info info;
	struct user_sdma_request *req;

	if (iovec[idx].iov_len < sizeof(info) + sizeof(req->hdr)) {
		hfi_cdbg(SDMA,
			 "[%u:%u:%u] First vector not big enough for header %lu/%lu",
			 dd->unit, uctxt->ctxt, subctxt_fp(fp),
			 iovec[idx].iov_len, sizeof(info) + sizeof(req->hdr));
		ret = -EINVAL;
		goto done;
	}
	ret = copy_from_user(&info, iovec[idx].iov_base, sizeof(info));
	if (ret) {
		hfi_cdbg(SDMA, "[%u:%u:%u] Failed to copy info QW (%d)",
			 dd->unit, uctxt->ctxt, subctxt_fp(fp), ret);
		ret = -EFAULT;
		goto done;
	}
	trace_hfi_sdma_user_reqinfo(dd, uctxt->ctxt, subctxt_fp(fp),
				    (u16 *)&info);
	if (cq->comps[info.comp_idx].status == QUEUED) {
		hfi_cdbg(SDMA, "[%u:%u:%u] Entry %u is in QUEUED state",
			 dd->unit, uctxt->ctxt, subctxt_fp(fp),
			 info.comp_idx);
		ret = -EBADSLT;
		goto done;
	}
	/*
	 * We've done all the safety checks that we can up to this point,
	 * "allocate" the request entry.
	 */
	hfi_cdbg(SDMA, "[%u:%u:%u] Using req/comp entry %u\n", dd->unit,
		 uctxt->ctxt, subctxt_fp(fp), info.comp_idx);
	req = pq->reqs + info.comp_idx;
	memset(req, 0, sizeof(*req));
	/* Mark the request as IN_USE before we start filling it in. */
	set_bit(SDMA_REQ_IN_USE, &req->flags);
	req->data_iovs = req_iovcnt(info.ctrl) - 1;
	req->pq = pq;
	req->cq = cq;
	INIT_LIST_HEAD(&req->txps);
	spin_lock_init(&req->list_lock);
	memcpy(&req->info, &info, sizeof(info));

	if (req_opcode(info.ctrl) == EXPECTED)
		req->data_iovs--;

	if (!info.npkts || req->data_iovs > MAX_VECTORS_PER_REQ) {
		SDMA_DBG(req, "Too many vectors (%u/%u)", req->data_iovs,
			 MAX_VECTORS_PER_REQ);
		ret = -EINVAL;
		goto done;
	}

	/* Copy the header from the user buffer */
	ret = copy_from_user(&req->hdr, iovec[idx].iov_base + sizeof(info),
			     sizeof(req->hdr));
	if (ret) {
		SDMA_DBG(req, "Failed to copy header template (%d)", ret);
		ret = -EFAULT;
		goto free_req;
	}
	req->koffset = le32_to_cpu(req->hdr.kdeth.offset);
	/* The KDETH.OM flag in the first packed (and header template) is
	 * always 0. */
	req->tidoffset = KDETH_GET(req->hdr.kdeth.ver_tid_offset, OFFSET) *
		KDETH_OM_SMALL;
	SDMA_DBG(req, "Initial TID offset %u", req->tidoffset);
	idx++;

	/* Save all the IO vector structures */
	SDMA_DBG(req, "copying %u data iovs", req->data_iovs);
	while (i < req->data_iovs) {
		memcpy(&req->iovs[i].iov, iovec + idx++, sizeof(struct iovec));
		req->iovs[i].offset = 0;
		req->data_len += req->iovs[i++].iov.iov_len;
	}
	SDMA_DBG(req, "total data length %u", req->data_len);

	if (pcount > req->info.npkts)
		pcount = req->info.npkts;
	/*
	 * Copy any TID info
	 * User space will provide the TID info only when the
	 * request type is EXPECTED. This is true even if there is
	 * only one packet in the request and the header is already
	 * setup. The reason for the singular TID case is that the
	 * driver needs to perform safety checks.
	 */
	if (req_opcode(req->info.ctrl) == EXPECTED) {
		/*
		 * We have to copy all of the tids because they may vary
		 * in size and, therefore, the TID count might not be
		 * equal to the pkt count. However, there is no way to
		 * tell at this point.
		 */
		ret = copy_from_user(req->tids, iovec[idx].iov_base,
				     iovec[idx].iov_len);
		if (ret) {
			SDMA_DBG(req, "Failed to copy %d TIDs (%d)",
				 pcount, ret);
			ret = -EFAULT;
			goto free_req;
		}
		req->n_tids = iovec[idx].iov_len / sizeof(req->tids[0]);
		idx++;
	}

	/* Have to select the engine */
	req->sde = sdma_select_engine_vl(dd,
					 (u32)(uctxt->ctxt + subctxt_fp(fp)),
					 (be16_to_cpu(req->hdr.lrh[0]) & 0xF));
	if (!sdma_running(req->sde)) {
		ret = -ECOMM;
		goto free_req;
	}

	if (user_sdma_use_ahg) {
		req->ahg_idx = sdma_ahg_alloc(req->sde);
		if (req->ahg_idx < 0)
			SDMA_DBG(req, "Failed to allocate AHG entry");
		else
			set_bit(SDMA_REQ_HAVE_AHG, &req->flags);
	}

	set_comp_state(req, QUEUED, 0);
	atomic64_add(req->info.npkts, &pq->npkts);
	/* Send the first N packets in the request to buy us some time */
	sent = user_sdma_send_pkts(req, pcount);
	if (unlikely(sent < 0)) {
		if (sent != -EBUSY) {
			ret = sent;
			goto send_err;
		} else
			sent = 0;
	}

	/*
	 * If the descriptor callback for the last packet of the message
	 * has already run by the time we get here, we are done.
	 */
	if (test_bit(SDMA_REQ_CB_DONE, &req->flags)) {
		*count += idx;
		ret = 0;
		goto done;
	}
	/* Take the references to the user's task and mm_struct */
	get_task_struct(current);
	req->user_proc = current;

	if (sent < req->info.npkts) {
#if 0
		/*
		 * It is *VERY* important that this increment does not happen
		 * until all request processing that should happen in this
		 * function has already completed. This is due to the fact that
		 * the background packet processing thread uses this count as an
		 * indicator to start processing the request.
		 * If the increment happens before this function is done with
		 * the request, then the thread could start processing it before
		 * this function is done.
		 */
		set_bit(SDMA_REQ_FOR_THREAD, &req->flags);
		atomic_inc(&pq->n_reqs);
		xchg(&pq->state, SDMA_PKT_Q_ACTIVE);
		/*
		 * We are done with everything here and we've indicated to the
		 * thread that we've added this request. Now, it's time to wake
		 * it up.
		 */
		if (uctxt->progress)
			wake_up_process(uctxt->progress);
#endif
		/*
		 * This is a somewhat blocking send implementation.
		 * The driver will block the caller until all packets of the
		 * request have been submitted to the SDMA engine. However, it
		 * will not wait for send completions.
		 */
		while (!test_bit(SDMA_REQ_SEND_DONE, &req->flags)) {
			ret = user_sdma_send_pkts(req, pcount);
			if (ret < 0) {
				if (ret != -EBUSY)
					goto send_err;
				else {
					/*
					 * Work around the fact that the
					 * SDMA API does not add us to the
					 * dmawait queue.
					 */
					struct sdma_engine *sde = req->sde;
					struct qib_ibdev *dev =
						&sde->dd->verbs_dev;
					xchg(&pq->state, SDMA_PKT_Q_DEFERRED);
					spin_lock(&dev->pending_lock);
					list_add_tail(&pq->busy.list,
						      &sde->dmawait);
					spin_unlock(&dev->pending_lock);
					/*
					 * There seems to be an issue with
					 * getting woken up in that we are not
					 * woken up every time and the process
					 * could "hang". Work around this by
					 * using a timeout.
					 */
					wait_event_interruptible_timeout(
						pq->busy.wait_dma,
						(pq->state ==
						 SDMA_PKT_Q_ACTIVE),
						msecs_to_jiffies(
							SDMA_IOWAIT_TIMEOUT));
				}
			}
		}

	}
	ret = 0;
	*count += idx;
	goto done;
send_err:
	set_comp_state(req, ERROR, ret);
free_req:
	user_sdma_free_request(req);
done:
	return ret;
}

static _hfi_inline u32 compute_data_length(struct user_sdma_request *req,
					   struct user_sdma_txreq *tx)
{
	/*
	 * Determine the proper size of the packet data.
	 * The size of the data of the first packet is in the header
	 * template. However, it includes the header and ICRC, which need
	 * to be subtracted.
	 * The size of the remaining packets is the minumum of the frag
	 * size (MTU) or remaining data in the request.
	 */
	u32 len;
	if (tx->flags & USER_SDMA_TXREQ_FLAGS_FIRST_PKT) {
		len = ((be16_to_cpu(req->hdr.lrh[2]) << 2) -
		       (sizeof(tx->hdr) - 4));
	} else if (tx->flags & USER_SDMA_TXREQ_FLAGS_SECOND_PKT &&
		   req_opcode(req->info.ctrl) == EXPECTED) {
		len = (EXP_TID_GET(req->tids[req->tididx], LEN) * PAGE_SIZE) -
			req->tidoffset;
		if (unlikely(!len) && ++req->tididx < req->n_tids &&
		    req->tids[req->tididx]) {
			req->tidoffset = 0;
			len = min((u32)(EXP_TID_GET(req->tids[req->tididx],
						    LEN) * PAGE_SIZE),
				  (u32)req->info.fragsize);
		}
		len = min(len, req->data_len - req->sent);
	} else
		len = min(req->data_len - req->sent, (u32)req->info.fragsize);
	SDMA_DBG(req, "Data Length = %u", len);
	return len;
}

static int user_sdma_send_pkts(struct user_sdma_request *req, unsigned maxpkts)
{
	int ret = 0;
	unsigned npkts = 0;
	struct user_sdma_txreq *tx;
	struct hfi_user_sdma_pkt_q *pq = NULL;
	struct user_sdma_iovec *iovec = NULL;
	unsigned long flags;

	if (!req->pq) {
		ret = -EINVAL;
		goto bail;
	}

	/*
	 * Check if we might have sent the entire request already
	 */
	if (unlikely(req->txreqs_sent == req->info.npkts))
		goto bail;

	pq = req->pq;

	if (!maxpkts || maxpkts > req->info.npkts - req->txreqs_sent)
		maxpkts = req->info.npkts - req->txreqs_sent;

	SDMA_DBG(req, "sending maxpkts=%u %u", maxpkts, req->txreqs_sent);
	while (npkts < maxpkts) {
		u32 datalen = 0, queued = 0, data_sent = 0;
		u64 iov_offset = 0;

		/* XXX (Mitko): check if the SDMA engine has any free
		   resources */

		tx = kmem_cache_alloc(pq->txreq_cache, GFP_KERNEL);
		if (!tx) {
			ret = -ENOMEM;
			goto free_tx;
		}
		INIT_LIST_HEAD(&tx->list);
		tx->flags = 0;
		tx->req = req;
		tx->iovec = NULL;
		spin_lock_init(&tx->lock);
		switch (req->txreqs_sent) {
		case 0:
			tx->flags |= USER_SDMA_TXREQ_FLAGS_FIRST_PKT;
			break;
		case 1:
			tx->flags |= USER_SDMA_TXREQ_FLAGS_SECOND_PKT;
			break;
		case 2:
			tx->flags |= USER_SDMA_TXREQ_FLAGS_THIRD_PKT;
			break;
		}
		if (req->txreqs_sent == req->info.npkts - 1)
			tx->flags |= USER_SDMA_TXREQ_FLAGS_LAST_PKT;
		SDMA_DBG(req, "tx->flags=%#x", tx->flags);
		/*
		 * Calculate the payload size - this is min of the fragment
		 * (MTU) size or the remaing bytes in the request but only
		 * if we have payload data.
		 */
		if (req->data_len) {
			iovec = &req->iovs[req->iov_idx];

			if (iovec->offset == iovec->iov.iov_len) {
				SDMA_DBG(req, "Vector done %llu %lu\n",
					 iovec->offset, iovec->iov.iov_len);
				if (++req->iov_idx == req->data_iovs)
					break;
				continue;
			}

			/*
			 * This request might include only a header and no user
			 * data, so pin pages only if there is data and it the
			 * pages have not been pinned already.
			 */
			if (unlikely(!iovec->pages && iovec->iov.iov_len)) {
				ret = pin_vector_pages(req, iovec);
				if (ret)
					goto free_tx;
			}

			tx->iovec = iovec;
			datalen = compute_data_length(req, tx);
			if (!datalen) {
				SDMA_DBG(req,
					 "Request has data but pkt len is 0");
				ret = -EFAULT;
				goto free_tx;
			}
			/*
			 * Mark the last fragment in this io vector so we can
			 * unpin the pages when we get the callback.
			 */
			if (unlikely(iovec->frags_sent == iovec->num_frags - 1))
				tx->flags |= USER_SDMA_TXREQ_FLAGS_LAST_FRAG;
		}

		if (test_bit(SDMA_REQ_HAVE_AHG, &req->flags)) {
			tx->flags |= USER_SDMA_TXREQ_FLAGS_USE_AHG;
			if (tx->flags & USER_SDMA_TXREQ_FLAGS_FIRST_PKT)
				sdma_txinit_ahg(&tx->txreq,
						SDMA_TXREQ_F_AHG_COPY,
						sizeof(req->hdr) + datalen,
						req->ahg_idx, 0, NULL,
						user_sdma_txreq_cb);
			else {
				int changes = 0;
				changes = set_txreq_header_ahg(req, tx,
							       datalen);
				if (changes < 0)
					goto free_tx;
				sdma_txinit_ahg(&tx->txreq,
						SDMA_TXREQ_F_USE_AHG,
						sizeof(req->hdr) + datalen,
						req->ahg_idx, changes,
						req->ahg,
						user_sdma_txreq_cb);
			}
		} else {
			sdma_txinit(&tx->txreq, 0, sizeof(req->hdr) + datalen,
				    user_sdma_txreq_cb);
			/*
			 * Modify the header for this packet. This only needs
			 * to be done if we are not goint to use AHG. Otherwise,
			 * the HW will do it based on the changes we gave it
			 * during sdma_txinit_ahg().
			 */
			ret = set_txreq_header(req, tx, datalen);
			if (ret)
				goto free_tx;
		}

		/*
		 * If the request contains any data vectors, add up to
		 * fragsize bytes to the descriptor.
		 */
		while (queued < datalen &&
		       (req->sent + data_sent) < req->data_len) {
			unsigned long base = (unsigned long)iovec->iov.iov_base,
				offset = ((base + iovec->offset + iov_offset) &
					  ~PAGE_MASK);
			unsigned pageidx = (((iovec->offset + iov_offset +
					     base) - (base & PAGE_MASK)) >>
					    PAGE_SHIFT),
				len = offset + req->info.fragsize > PAGE_SIZE ?
				PAGE_SIZE - offset : req->info.fragsize;
			len = min((datalen - queued), len);
			SDMA_DBG(req,
				 "iovidx=%lu offset=%#llx pageidx=%u, %p, %#lx, len=%u, offset=%lu",
				 req->iov_idx, iovec->offset + iov_offset,
				 pageidx, iovec->pages[pageidx],
				 (unsigned long)iovec->iov.iov_base, len,
				 offset);
			BUG_ON(!iovec->pages[pageidx]);
			ret = sdma_txadd_page(pq->dd, &tx->txreq,
					      iovec->pages[pageidx],
					      offset, len);
			if (ret) {
				unpin_vector_pages(iovec);
				goto free_tx;
			}
			iov_offset += len;
			queued += len;
			data_sent += len;
			if (unlikely(queued < datalen &&
				     pageidx == iovec->npages &&
				     req->iov_idx < req->data_iovs - 1)) {
				iovec->offset += iov_offset;
				iovec = &req->iovs[++req->iov_idx];
				if (!iovec->pages) {
					ret = pin_vector_pages(req, iovec);
					if (ret)
						goto free_tx;
				}
				iov_offset = 0;

			}
		}
		/*
		 * Lock the descriptor until all the book-keeping has been done
		 * This guards agains the callback running prior to that and
		 * freeing needed data structures.
		 */
		spin_lock_irqsave(&tx->lock, flags);
		/*
		 * We should have added all the pages to this request by now.
		 * Send it off.
		 */
		ret = sdma_send_txreq(req->sde, &pq->busy, NULL, &tx->txreq);
		if (unlikely(ret)) {
			if (iovec)
				unpin_vector_pages(iovec);
			spin_unlock_irqrestore(&tx->lock, flags);
			goto free_tx;
		}
		/*
		 * The txreq was submitted successfully so we can update
		 * the counters.
		 */
		req->koffset += datalen;
		if (req_opcode(req->info.ctrl) == EXPECTED)
			req->tidoffset += datalen;
		req->sent += data_sent;
		if (req->data_len && iovec) {
			iovec->frags_sent++;
			iovec->offset += iov_offset;
		}
		/*
		 * It is important to increment this here as it is used to
		 * generate the BTH.PSN and, therefore, can't be bulk-updated
		 * outside of the loop.
		 */
		req->txreqs_sent++;
		if (req->txreqs_sent == req->info.npkts)
			set_bit(SDMA_REQ_SEND_DONE, &req->flags);
		list_add_tail(&tx->list, &req->txps);
		spin_unlock_irqrestore(&tx->lock, flags);
		npkts++;
	}

	ret = npkts;
	goto done;
free_tx:
	sdma_txclean(pq->dd, &tx->txreq);
	kmem_cache_free(pq->txreq_cache, tx);
done:
	atomic64_sub(npkts, &pq->npkts);
bail:
	return ret;
}

/*
 * How many pages in this iovec element?
 */
static _hfi_inline int num_user_pages(const struct iovec *iov)
{
	const unsigned long addr  = (unsigned long) iov->iov_base;
	const unsigned long len   = iov->iov_len;
	const unsigned long spage = addr & PAGE_MASK;
	const unsigned long epage = (addr + len - 1) & PAGE_MASK;

	return 1 + ((epage - spage) >> PAGE_SHIFT);
}

static int pin_vector_pages(struct user_sdma_request *req,
			    struct user_sdma_iovec *iovec) {
	int ret = 0;
	unsigned pinned;

	iovec->npages = num_user_pages(&iovec->iov);
	iovec->num_frags = (iovec->iov.iov_len / req->info.fragsize) +
		!!(iovec->iov.iov_len % req->info.fragsize) +
		(req_opcode(req->info.ctrl) == EXPECTED && req->tidoffset);
	iovec->pages = kzalloc(sizeof(*iovec->pages) *
			       iovec->npages, GFP_KERNEL);
	if (!iovec->pages) {
		SDMA_DBG(req, "Failed page array alloc");
		ret = -ENOMEM;
		goto done;
	}
	/* If called by the kernel thread, use the user's mm */
	if (current->flags & PF_KTHREAD)
		use_mm(req->user_proc->mm);
	pinned = get_user_pages_fast(
		(unsigned long)iovec->iov.iov_base,
		iovec->npages, 0, iovec->pages);
	/* If called by the kernel thread, unuse the user's mm */
	if (current->flags & PF_KTHREAD)
		unuse_mm(req->user_proc->mm);
	if (pinned != iovec->npages) {
		SDMA_DBG(req, "Failed to pin pages (%u/%u)", pinned,
			 iovec->npages);
		ret = -EFAULT;
		goto pfree;
	}
	goto done;
pfree:
	unpin_vector_pages(iovec);
done:
	return ret;
}

static _hfi_inline void unpin_vector_pages(struct user_sdma_iovec *iovec)
{
	unsigned i;
	for (i = 0; i < iovec->npages; i++)
		if (iovec->pages[i])
			put_page(iovec->pages[i]);
	kfree(iovec->pages);
	iovec->pages = NULL;
	iovec->npages = 0;
	iovec->num_frags = 0;
}

static int check_header_template(struct user_sdma_request *req,
				 struct hfi_pio_hdr *hdr, u32 lrhlen,
				 u32 datalen)
{
	/*
	 * Perform safety checks for any type of packet:
	 *    - trasnfer size is multiple of 64bytes
	 *    - packet length is multiple of 4bytes
	 *    - entire request length is multiple of 4bytes
	 *    - packet length is not larger than MTU size
	 *
	 * These checks are only done for the first packet of the
	 * transfer since the header is "given" to us by user space.
	 * For the remainder of the packets we compute the values.
	 */
	if (req->info.fragsize % WFR_PIO_BLOCK_SIZE ||
	    lrhlen & 0x3 || req->data_len & 0x3) /* ||
		// XXX (MITKO): lrhlen will be larger than the fragsize due
		// to the header. How do we handle that?
		//lrhlen > req->info.fragsize) */
		return -EINVAL;

	if (req_opcode(req->info.ctrl) == EXPECTED) {
		u32 tidval = req->tids[req->tididx],
			tidlen = EXP_TID_GET(tidval, LEN) * PAGE_SIZE,
			tididx = EXP_TID_GET(tidval, IDX),
			tidctrl = EXP_TID_GET(tidval, CTRL);
		__le32 kval = hdr->kdeth.ver_tid_offset;
		/*
		 * Expected receive packets have the following
		 * additional checks:
		 *     - KDETH.OM is not set
		 *     - offset is not larger than the TID size
		 *     - TIDCtrl values match between header and TID array
		 *     - TID indexes match between header and TID array
		 */
		if (KDETH_GET(req->hdr.kdeth.ver_tid_offset, OM) ||
		    ((KDETH_GET(kval, OFFSET) * KDETH_OM_SMALL) +
		     datalen > tidlen) ||
		    KDETH_GET(kval, TIDCTRL) != tidctrl ||
		    KDETH_GET(kval, TID) != tididx)
			return -EINVAL;
	}
	return 0;
}

static int set_txreq_header(struct user_sdma_request *req,
			    struct user_sdma_txreq *tx, u32 datalen)
{
	struct hfi_user_sdma_pkt_q *pq = req->pq;
	struct hfi_pio_hdr *hdr = &tx->hdr;
	u16 pbclen;
	int ret;
	/* (Size of complete header - size of PBC) + 4B ICRC + data length */
	u32 val, lrhlen = ((sizeof(*hdr) - sizeof(hdr->pbc)) + 4 + datalen);

	/* Copy the header template to the request before modification */
	memcpy(hdr, &req->hdr, sizeof(*hdr));

	/*
	 * We only have to modify the header if this is not the
	 * first packet in the request. Otherwise, we use the
	 * header given to us.
	 */
	if (unlikely(tx->flags & USER_SDMA_TXREQ_FLAGS_FIRST_PKT)) {
		ret = check_header_template(req, hdr, lrhlen, datalen);
		if (ret)
			return ret;
		if (req_opcode(req->info.ctrl) == EXPECTED) {
			u32 tidlen = EXP_TID_GET(req->tids[req->tididx], LEN) *
						 PAGE_SIZE;
			/*
			 * If the length of the tid is larger than 128K,
			 * use the large offset multipliers. This needs to be
			 * checked for every TIDPair.
			 */
			req->omfactor = tidlen >= KDETH_OM_MAX_SIZE ?
				KDETH_OM_LARGE : KDETH_OM_SMALL;
		}
		goto done;

	}
	pbclen = le16_to_cpu(hdr->pbc[0]);
	/*
	 * Set the BTH PSN sequence number. Do it here, before we
	 * set the ACK bit so we don't have to clear the PSN bit field.
	 */
	val = (be32_to_cpu(hdr->bth[2]) + req->txreqs_sent) &
		(extended_psn ? 0x7fffffff : 0xffffff);
	hdr->bth[2] = cpu_to_be32(val);

	if (unlikely(tx->flags &
		     (USER_SDMA_TXREQ_FLAGS_SECOND_PKT |
		      USER_SDMA_TXREQ_FLAGS_THIRD_PKT |
		      USER_SDMA_TXREQ_FLAGS_LAST_PKT))) {
		/*
		 * Check if the PBC and LRH length are mismatched. If so
		 * adjust both in the header.
		 */
		if (((pbclen  & 0xfff) << 2) - 4 != lrhlen) {
			/* convert to DWs */
			lrhlen >>= 2;
			pbclen &= ~(0xfff);
			pbclen |= (lrhlen + 1) & 0xfff;
			hdr->pbc[0] = cpu_to_le16(pbclen);
			hdr->lrh[2] = cpu_to_be16(lrhlen);
			if (tx->flags & USER_SDMA_TXREQ_FLAGS_THIRD_PKT) {
				/*
				 * From this point on the lengths in both the
				 * PBC and LRH are the same until the last
				 * packet.
				 * Ajust the template so we don't have to update
				 * every packet
				 */
				req->hdr.pbc[0] = hdr->pbc[0];
				req->hdr.lrh[2] = hdr->lrh[2];
			}
		}
		if (unlikely(tx->flags & USER_SDMA_TXREQ_FLAGS_LAST_PKT)) {
			/* Set ACK request on last packet */
			hdr->bth[2] |= cpu_to_be32(1UL<<31);
		}
	}

	/* Set the new offset */
	hdr->kdeth.offset = cpu_to_le32(req->koffset);
	/* Expected packets have to fill in the new TID information */
	if (req_opcode(req->info.ctrl) == EXPECTED) {
		u32 tidval;

		/* Check whether we have a valid tid */
		if (req->tididx > req->n_tids - 1 ||
		    !req->tids[req->tididx]) {
			SDMA_DBG(req, "tididx=%u, n_tids=%u, %#x", req->tididx,
				 req->n_tids, req->tids[req->tididx]);
			return -EINVAL;
		}
		tidval = req->tids[req->tididx];
		/*
		 * If the offset puts us at the end of the current TID,
		 * advance everything.
		 */
		if ((req->tidoffset) == (EXP_TID_GET(tidval, LEN) *
					 PAGE_SIZE)) {
			u32 tidlen;
			req->tidoffset = 0;
			/* Since we don't copy all the TIDs, all at once,
			 * we have to check again. */
			if (++req->tididx > req->n_tids - 1 ||
			    !req->tids[req->tididx]) {
				SDMA_DBG(req, "new tididx=%u, n_tids=%u, %#x",
					 req->tididx, req->n_tids,
					 req->tids[req->tididx]);
				return -EINVAL;
			}
			tidval = req->tids[req->tididx];
			tidlen = EXP_TID_GET(tidval, LEN) * PAGE_SIZE;
			req->omfactor = tidlen >= KDETH_OM_MAX_SIZE ?
				KDETH_OM_LARGE : KDETH_OM_SMALL;
		}
		SDMA_DBG(req, "tids[%d]=%#x", req->tididx, tidval);
		/* Set KDETH.TIDCtrl based on value for this TID. */
		KDETH_SET(hdr->kdeth.ver_tid_offset, TIDCTRL,
			  EXP_TID_GET(tidval, CTRL));
		/* Set KDETH.TID based on value for this TID */
		KDETH_SET(hdr->kdeth.ver_tid_offset, TID,
			  EXP_TID_GET(tidval, IDX));
		SDMA_DBG(req, "Setting TIDOff %u", req->tidoffset);
		/*
		 * Set the KDETH.OFFSET and KDETH.OM based on size of
		 * transfer.
		 */
		KDETH_SET(hdr->kdeth.ver_tid_offset, OFFSET,
			  req->tidoffset / req->omfactor);
		KDETH_SET(hdr->kdeth.ver_tid_offset, OM,
			  !!(req->omfactor - KDETH_OM_SMALL));
	}
done:
	trace_hfi_sdma_user_header(pq->dd, pq->ctxt, pq->subctxt,
				   req->info.comp_idx, hdr);
	return sdma_txadd_kvaddr(pq->dd, &tx->txreq, hdr, sizeof(*hdr));
}

static int set_txreq_header_ahg(struct user_sdma_request *req,
			       struct user_sdma_txreq *tx, u32 len)
{
	int diff = 0;
	struct hfi_user_sdma_pkt_q *pq = req->pq;
	u16 flags = tx->flags & ~(USER_SDMA_TXREQ_FLAGS_LAST_FRAG |
				  USER_SDMA_TXREQ_FLAGS_USE_AHG);
	struct hfi_pio_hdr *hdr = &tx->hdr;
	u32 val32, lrhlen = ((sizeof(*hdr) - sizeof(hdr->pbc)) + 4 + len) >> 2;

	memcpy(hdr, &req->hdr, sizeof(*hdr));
	if (unlikely(flags)) {
		if (flags & USER_SDMA_TXREQ_FLAGS_FIRST_PKT) {
			int ret;
			ret = check_header_template(req, hdr, lrhlen, len);
			if (ret)
				return ret;
		}
		if (flags & (USER_SDMA_TXREQ_FLAGS_SECOND_PKT |
			     USER_SDMA_TXREQ_FLAGS_THIRD_PKT |
			     USER_SDMA_TXREQ_FLAGS_LAST_PKT)) {
			u16 pbclen = le16_to_cpu(hdr->pbc[0]);
			if (((pbclen & 0xfff) - 1) != lrhlen) {
				/* PBC.PbcLengthDWs */
				req->ahg[diff++] = sdma_build_ahg_descriptor(
					cpu_to_le16(lrhlen + 1) & 0xfff, 0, 0,
					12);
				/* LRH.PktLen */
				req->ahg[diff++] = sdma_build_ahg_descriptor(
					cpu_to_le16(lrhlen), 3, 4, 12);
			}
			/* BTH.A */
			req->ahg[diff++] = sdma_build_ahg_descriptor(6, 0, 1,
								     1);
			/* KDETH.SH */
			if (req_opcode(req->info.ctrl) == EXPECTED)
				req->ahg[diff++] =
					sdma_build_ahg_descriptor(7, 29, 1, 1);
		}
	}
	/*
	 * Do the common updates
	 */
	/* BTH.PSN */
	val32 = (be32_to_cpu(hdr->bth[2]) + req->txreqs_sent) &
		(extended_psn ? 0x7fffffff : 0xffffff);
	req->ahg[diff++] = sdma_build_ahg_descriptor(cpu_to_be16(val32 &
								 0xffff),
						     6, 8, 16);
	req->ahg[diff++] = sdma_build_ahg_descriptor(
		cpu_to_be16((val32 >> 16) & (extended_psn ? 0x7fff : 0xff)),
		6, 24, 8);
	/* KDETH.Offset */
	req->ahg[diff++] = sdma_build_ahg_descriptor(
		cpu_to_le16(req->koffset & 0xffff),
		15, 0, 16);
	req->ahg[diff++] = sdma_build_ahg_descriptor(
		cpu_to_le16((req->koffset >> 16) & 0xffff), 15, 16, 16);

	if (req_opcode(req->info.ctrl) == EXPECTED) {
		u32 tidval;
		if (req->tididx > req->n_tids - 1 ||
		    !req->tids[req->tididx])
			return -EINVAL;
		tidval = req->tids[req->tididx];
		/* TIDCtrl */
		req->ahg[diff++] = sdma_build_ahg_descriptor(
			cpu_to_le16(EXP_TID_GET(tidval, CTRL)), 7, 26, 2);
		/* TIDIdx */
		req->ahg[diff++] = sdma_build_ahg_descriptor(
			cpu_to_le16(EXP_TID_GET(tidval, IDX)), 7, 16, 10);
		/* Offset */
		/* XXX (MITKO): Set the TID offset here */
	}

	trace_hfi_sdma_user_header_ahg(pq->dd, pq->ctxt, pq->subctxt,
				       req->info.comp_idx, req->ahg);
	return diff;
}

static void user_sdma_txreq_cb(struct sdma_txreq *txreq, int status,
			       int drain)
{
	struct user_sdma_txreq *tx =
		container_of(txreq, struct user_sdma_txreq, txreq);
	struct user_sdma_iovec *iovec = tx->iovec;
	struct user_sdma_request *req = tx->req;
	struct hfi_user_sdma_pkt_q *pq = req->pq;
	u16 txflags = tx->flags;
	unsigned long flags;

	/* XXX (MITKO): Once we start using the SDMA drain mechanism, this
	 * should never happen. For now, guard against it so we don't crash
	 * the kernel. */
	if (!req || !pq) {
		hfi_cdbg(SDMA,
			 "Attempting to free txreq after queue was destroyed");
		return;
	}

	spin_lock_irqsave(&tx->lock, flags);
	/* This should not happen. */
	if (list_empty(&tx->list)) {
		dd_dev_err(pq->dd, "Attempting to free an empty tx descriptor");
		spin_unlock_irqrestore(&tx->lock, flags);
		return;
	}
	list_del_init(&tx->list);
	spin_unlock_irqrestore(&tx->lock, flags);

	if (txflags & USER_SDMA_TXREQ_FLAGS_LAST_FRAG) {
		if (tx->iovec)
			unpin_vector_pages(iovec);
		else
			SDMA_DBG(req, "TxReq has data but no vector");
	}

	sdma_txclean(pq->dd, &tx->txreq);
	kmem_cache_free(pq->txreq_cache, tx);
	req->txreqs_completed++;

	if (status != SDMA_TXREQ_S_OK) {
		set_comp_state(req, ERROR, status);
		goto free;
	}

	if (unlikely(txflags & USER_SDMA_TXREQ_FLAGS_LAST_PKT)) {
		/* We've sent and completed all packets in this
		 * request. Signal completion to the user */
		if (test_and_clear_bit(SDMA_REQ_FOR_THREAD, &req->flags))
			atomic_dec(&pq->n_reqs);
		set_bit(SDMA_REQ_CB_DONE, &req->flags);
		set_comp_state(req, COMPLETE, 0);
	} else
		goto done;
free:
	if (txflags & USER_SDMA_TXREQ_FLAGS_USE_AHG)
		sdma_ahg_free(req->sde, req->ahg_idx);
	user_sdma_free_request(req);
done:
	if (!atomic_read(&pq->n_reqs))
		xchg(&pq->state, SDMA_PKT_Q_INACTIVE);
	return;
}

static void user_sdma_free_request(struct user_sdma_request *req)
{
	unsigned long flags;
	spin_lock_irqsave(&req->list_lock, flags);
	if (!list_empty(&req->txps)) {
		struct user_sdma_txreq *t, *p;
		SDMA_DBG(req, "Request still has tx requests");
		list_for_each_entry_safe(t, p, &req->txps, list) {
			list_del_init(&t->list);
			sdma_txclean(req->pq->dd, &t->txreq);
			kmem_cache_free(req->pq->txreq_cache, t);
		}
	}
	spin_unlock_irqrestore(&req->list_lock, flags);
	/* Remove number of outstanding packets from packet queue count */
	atomic64_sub((req->info.npkts - req->txreqs_sent), &req->pq->npkts);
	if (req->user_proc)
		put_task_struct(req->user_proc);
	return;
}

static _hfi_inline void set_comp_state(struct user_sdma_request *req,
				       enum hfi_sdma_comp_state state, int ret)
{
	SDMA_DBG(req, "Setting completion status %u %d", state, ret);
	req->cq->comps[req->info.comp_idx].status = state;
	if (state == ERROR)
		req->cq->comps[req->info.comp_idx].errno = -ret;
}

#if 0
static int user_sdma_progress(void *data)
{
	int ret = 0, sleep;
	struct qib_ctxtdata *uctxt = data;
	struct hfi_user_sdma_pkt_q *pq, *ptr;

	while (!kthread_should_stop()) {
		sleep = 1;
		list_for_each_entry_safe(pq, ptr, &uctxt->sdma_queues, list) {
			struct user_sdma_request *req = NULL;
			unsigned sent = 0;

			if (pq->state != SDMA_PKT_Q_ACTIVE)
				continue;

			hfi_cdbg(SDMA, "processing queue %u:%u %u", pq->ctxt,
				 pq->subctxt, pq->reqidx);
			/* Does this queue contain any requests? */
			if (unlikely(!atomic_read(&pq->n_reqs)))
				continue;

			while (atomic64_read(&pq->npkts) &&
			      sent < MAX_PKTS_PER_QUEUE) {
				unsigned to_send = 0, next = 0;

				req = &pq->reqs[pq->reqidx];
				/* Is this request current active */
				if (!test_bit(SDMA_REQ_IN_USE, &req->flags) ||
				    !test_bit(SDMA_REQ_FOR_THREAD,
					      &req->flags) ||
				    test_bit(SDMA_REQ_SEND_DONE, &req->flags)) {
					next = 1;
					goto next;
				}
				/*
				 * Check whether the request contains any
				 * data vectors. We won't be processing any
				 * requests that have only a header since that
				 * would have been sent as the first (and only)
				 * packet.
				 */
				if (!req->data_iovs) {
					next = 1;
					goto next;
				}
				to_send = min(req->info.npkts -
					      req->txreqs_sent,
					      MAX_PKTS_PER_QUEUE);
				SDMA_DBG(req, "attempting to send %u pkts %u",
					 to_send, sleep);
				if (!to_send)
					goto next;
				ret = user_sdma_send_pkts(req, to_send);
				if (unlikely(ret < 0)) {
					if (ret != -EBUSY) {
						set_comp_state(req, ERROR, ret);
						user_sdma_free_request(req);
						next = 1;
						goto next;
					} else
						break;
				} else
					sent += ret;
				if (req->txreqs_sent == req->info.npkts)
					next = 1;
next:
				if (next)
					if (++pq->reqidx == pq->n_max_reqs)
						pq->reqidx = 0;
			}
			if (atomic64_read(&pq->npkts))
				sleep = 0;
		}
		if (sleep) {
			hfi_cdbg(SDMA, "Going to sleep");
			/* Put ourselves to sleep if there is nothing to do */
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
		}
	}
	return 0;
}
#endif
