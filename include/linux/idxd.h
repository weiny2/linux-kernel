// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 Intel Corporation. All rights rsvd. */
#ifndef _KERNEL_IDXD_H_
#define _KERNEL_IDXD_H_

#include <linux/dmaengine.h>

enum idxd_type {
	IDXD_TYPE_UNKNOWN = -1,
	IDXD_TYPE_ANY = -1,
	IDXD_TYPE_DSA = 0,
	IDXD_TYPE_IAX,
	IDXD_TYPE_MAX,
};

enum idxd_wq_mode {
	IDXD_WQ_UNKNOWN = -1,
	IDXD_WQ_ANY = -1,
	IDXD_WQ_DEDICATED = 0,
	IDXD_WQ_SHARED,
	IDXD_WQ_MAX,
};

enum idxd_op_type {
	IDXD_OP_BLOCK = 0,
	IDXD_OP_NONBLOCK = 1,
};

enum dma_chan_idxd_info_op {
	DMA_CHAN_INFO_IDXD_START = DMA_CHAN_INFO_MAX,
	DMA_CHAN_INFO_IDXD_PORTAL,
	DMA_CHAN_INFO_IDXD_PASID,
};


struct idxd_wq_request {
	char name[128];
	int node;
	enum idxd_type type;
	enum idxd_wq_mode mode;
};

struct idxd_dev_err {
	int err;
	u64 addr;
};

/* IDXD software descriptor */
struct idxd_desc {
	union {
		struct dsa_hw_desc *hw;
		struct iax_hw_desc *iax_hw;
	};
	dma_addr_t desc_dma;
	union {
		struct dsa_completion_record *completion;
		struct iax_completion_record *iax_completion;
	};
	dma_addr_t compl_dma;
	struct dma_async_tx_descriptor txd;
	struct llist_node llnode;
	struct list_head list;
	int id;
        int cpu;
	struct idxd_wq *wq;
	struct completion *done;
};

extern bool idxd_filter_kdirect(struct dma_chan *chan, void *filter_param);

/* submission */
extern int idxd_submit_desc(struct idxd_wq *wq, struct idxd_desc *desc);
extern struct idxd_desc *idxd_alloc_desc(struct idxd_wq *wq,
					 enum idxd_op_type optype);
extern void idxd_free_desc(struct idxd_wq *wq, struct idxd_desc *desc);

#endif
