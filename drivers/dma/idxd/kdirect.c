// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 Intel Corporation. All rights rsvd. */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <uapi/linux/idxd.h>
#include <linux/idxd.h>
#include "registers.h"
#include "idxd.h"

static inline bool name_match(const char *wq_name, const char *req_name)
{
	if (strcmp(wq_name, req_name) == 0)
		return true;
	return false;
}

static inline bool type_match(enum idxd_type dev_type, enum idxd_type req_type)
{
	if (dev_type == req_type || req_type == IDXD_TYPE_ANY)
		return true;
	return false;
}

static inline bool mode_match(enum idxd_wq_mode wq_mode,
			      enum idxd_wq_mode req_mode)
{
	if (wq_mode == req_mode || req_mode == IDXD_WQ_ANY)
		return true;
	return false;
}

static inline bool node_match(int dev_node, int req_node)
{
	if (dev_node == req_node || req_node == NUMA_NO_NODE ||
	    dev_node == NUMA_NO_NODE)
		return true;
	return false;
}

bool idxd_filter_kdirect(struct dma_chan *chan, void *filter_param)
{
	struct idxd_wq *wq = to_idxd_wq(chan);
	struct idxd_device *idxd = wq->idxd;
	struct idxd_wq_request *wreq = (struct idxd_wq_request *)filter_param;
	bool found = false;
	int node = dev_to_node(&idxd->pdev->dev);

	if (name_match(wq->name, wreq->name) &&
	    type_match(idxd->type, wreq->type) &&
	    mode_match(!wq_dedicated(wq), wreq->mode) &&
	    node_match(node, wreq->node))
		found = true;

	/* Only assign to 1 client for dedicated wq */
	if (found && wq_dedicated(wq) && chan->client_count)
		found = false;

	return found;
}
EXPORT_SYMBOL_GPL(idxd_filter_kdirect);

static int idxd_dma_get_device_info(struct dma_chan *chan, unsigned int op, u64 *data)
{
	struct idxd_wq *wq = to_idxd_wq(chan);
	struct idxd_device *idxd = wq->idxd;

	switch (op) {
	case DMA_CHAN_INFO_DEPTH:
		*data = wq->size;
		break;
	case DMA_CHAN_INFO_NUMA:
		*data = dev_to_node(&wq->idxd->pdev->dev);
		break;
	case DMA_CHAN_INFO_IDXD_PORTAL:
		*data = (u64)(u64 *)(wq->portal);
		*data += idxd_get_wq_portal_offset(IDXD_PORTAL_UNLIMITED,
						   IDXD_IRQ_MSIX);
		break;
	case DMA_CHAN_INFO_IDXD_PASID:
		*data = idxd->pasid;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int idxd_dma_get_device_errors(struct dma_chan *chan, void *data)
{
	struct idxd_wq *wq = to_idxd_wq(chan);
	struct idxd_device *idxd = wq->idxd;
	struct idxd_dev_err *err = (struct idxd_dev_err *)data;
	unsigned long flags;

	memset(err, 0, sizeof(*err));
	spin_lock_irqsave(&idxd->dev_lock, flags);
	if (idxd->sw_err.valid) {
		err->err = idxd->sw_err.error;
		err->addr = idxd->sw_err.fault_addr;
	} else {
		spin_unlock_irqrestore(&idxd->dev_lock, flags);
		return -ENOENT;
	}
	spin_unlock_irqrestore(&idxd->dev_lock, flags);

	return 0;
}

static void *idxd_dma_get_desc(struct dma_chan *chan, unsigned long flags)
{
	struct idxd_wq *wq = to_idxd_wq(chan);

	return (void *)idxd_alloc_desc(wq, flags);
}

static void idxd_dma_free_desc(struct dma_chan *chan, void *desc)
{
	struct idxd_wq *wq = to_idxd_wq(chan);

	idxd_free_desc(wq, (struct idxd_desc *)desc);
}

static int idxd_dma_submit_and_wait(struct dma_chan *chan, void *desc,
			     unsigned long flags, int timeout)
{
	int rc;
	struct idxd_wq *wq = to_idxd_wq(chan);
	struct idxd_desc *d = desc;
	DECLARE_COMPLETION_ONSTACK(done);

	d->done = &done;
	rc = idxd_submit_desc(wq, d);
	if (rc < 0)
		return rc;

	if (timeout)
		return wait_for_completion_timeout(&done, timeout);

	wait_for_completion(&done);
	return 0;
}

void idxd_setup_dma_kdirect(struct idxd_device *idxd)
{
	struct dma_device *dma = &idxd->dma_dev;

	dma_cap_zero(dma->cap_mask);
	dma_cap_set(DMA_DIRECT, dma->cap_mask);
	dma_cap_set(DMA_PRIVATE, dma->cap_mask);
	dma->kdops.device_get_info = idxd_dma_get_device_info;
	dma->kdops.device_get_errors = idxd_dma_get_device_errors;
	dma->kdops.device_get_desc = idxd_dma_get_desc;
	dma->kdops.device_free_desc = idxd_dma_free_desc;
	dma->kdops.device_submit_and_wait = idxd_dma_submit_and_wait;
}
