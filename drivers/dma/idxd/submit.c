// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2019 Intel Corporation. All rights rsvd. */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <uapi/linux/idxd.h>
#include "idxd.h"
#include "registers.h"

struct idxd_desc *idxd_alloc_desc(struct idxd_wq *wq,
				  enum idxd_op_type optype)
{
	struct idxd_device *idxd = wq->idxd;
	struct idxd_desc *desc;
	int idx;

	if (idxd->state != IDXD_DEV_ENABLED)
		return ERR_PTR(-EIO);

	if (wq_dedicated(wq)) {
		if (optype == IDXD_OP_BLOCK)
			percpu_down_read(&wq->submit_lock);
		else if (!percpu_down_read_trylock(&wq->submit_lock))
			return ERR_PTR(-EBUSY);

		if (!atomic_add_unless(&wq->dq_count, 1, wq->size)) {
			int rc;

			if (optype == IDXD_OP_NONBLOCK) {
				percpu_up_read(&wq->submit_lock);
				return ERR_PTR(-EAGAIN);
			}

			percpu_up_read(&wq->submit_lock);
			percpu_down_write(&wq->submit_lock);
			rc = wait_event_interruptible(wq->submit_waitq,
					atomic_add_unless(&wq->dq_count, 1,
							  wq->size) ||
					idxd->state != IDXD_DEV_ENABLED);
			percpu_up_write(&wq->submit_lock);
			if (rc < 0)
				return ERR_PTR(-EINTR);
			if (idxd->state != IDXD_DEV_ENABLED)
				return ERR_PTR(-EIO);
		} else {
			percpu_up_read(&wq->submit_lock);
		}
	}

	idx = sbitmap_get(&wq->sbmap, 0, false);
	if (idx < 0) {
		atomic_dec(&wq->dq_count);
		return ERR_PTR(-EAGAIN);
	}

	desc = wq->descs[idx];
	memset(desc->hw, 0, sizeof(struct dsa_hw_desc));
	memset(desc->completion, 0, sizeof(struct dsa_completion_record));
	desc->done = NULL;
	if (idxd->pasid_enabled)
		desc->hw->pasid = idxd->pasid;

	/*
	 * Descriptor completion vectors are 1-8 for MSIX. We will round
	 * robin through the 8 vectors.
	 */
	if (!idxd->int_handles) {
		wq->vec_ptr = (wq->vec_ptr % idxd->num_wq_irqs) + 1;
		desc->hw->int_handle =  wq->vec_ptr;
	} else
		desc->hw->int_handle = idxd->int_handles[wq->id];	

	return desc;
}

void idxd_free_desc(struct idxd_wq *wq, struct idxd_desc *desc)
{
	if (wq_dedicated(wq))
		atomic_dec(&wq->dq_count);

	sbitmap_clear_bit(&wq->sbmap, desc->id);
	wake_up(&wq->submit_waitq);
}

static int idxd_iosubmit_cmd_sync(struct idxd_wq *wq, void __iomem *portal,
				  struct dsa_hw_desc *hw,
				  enum idxd_op_type optype)
{
	struct idxd_device *idxd = wq->idxd;
	int rc;

	if (optype == IDXD_OP_BLOCK)
		percpu_down_read(&wq->submit_lock);
	else if (!percpu_down_read_trylock(&wq->submit_lock))
		return -EBUSY;

	/*
	 * The wmb() flushes writes to coherent DMA data before possibly
	 * triggering a DMA read. The wmb() is necessary even on UP because
	 * the recipient is a device.
	 */
	wmb();
	rc = iosubmit_cmds512_sync(portal, hw, 1);
	if (rc) {
		if (optype == IDXD_OP_NONBLOCK)
			return -EBUSY;
		if (idxd->state != IDXD_DEV_ENABLED)
			return -EIO;
		percpu_up_read(&wq->submit_lock);
		percpu_down_write(&wq->submit_lock);
		rc = wait_event_interruptible(wq->submit_waitq,
					      !iosubmit_cmds512_sync(portal,
								     hw, 1) ||
					      idxd->state != IDXD_DEV_ENABLED);
		percpu_up_write(&wq->submit_lock);
		if (rc < 0)
			return -EINTR;
		if (idxd->state != IDXD_DEV_ENABLED)
			return -EIO;
	} else {
		percpu_up_read(&wq->submit_lock);
	}

	return 0;
}

int idxd_submit_desc(struct idxd_wq *wq, struct idxd_desc *desc,
		     enum idxd_op_type optype)
{
	struct idxd_device *idxd = wq->idxd;
	int rc;
	void __iomem *portal;

	if (idxd->state != IDXD_DEV_ENABLED)
		return -EIO;

	portal = wq->portal +
		 idxd_get_wq_portal_offset(IDXD_PORTAL_UNLIMITED,
		 			   IDXD_IRQ_MSIX);
	if (wq_dedicated(wq)) {
		/*
		 * The wmb() flushes writes to coherent DMA data before
		 * possibly triggering a DMA read. The wmb() is necessary
		 * even on UP because the recipient is a device.
		 */
		wmb();
		iosubmit_cmds512(portal, desc->hw, 1);
	} else {
		rc = idxd_iosubmit_cmd_sync(wq, portal, desc->hw, optype);
		if (rc < 0)
			return rc;
	}

	/*
	 * Pending the descriptor to the lockless list for the irq_entry
	 * that we designated the descriptor to.
	 */
	if (desc->hw->flags & IDXD_OP_FLAG_RCI) {
		int vec;

		/*
		 * If the driver is on host kernel, it would be the value
		 * assigned to interrupt handle, which is index for MSIX
		 * vector. If it's guest then we'll set it to 1 for now
		 * since only 1 workqueue is exported.
		 */
		vec = !idxd->int_handles ? desc->hw->int_handle : 1;
		llist_add(&desc->llnode,
			  &idxd->irq_entries[vec].pending_llist);
	}

	return 0;
}
