// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include <linux/interrupt.h>
#include <linux/uaccess.h>

#include "dlb_intr.h"
#include "dlb_main.h"
#include "dlb_resource.h"

void dlb_wake_thread(struct dlb_dev *dev,
		     struct dlb_cq_intr *intr,
		     enum dlb_wake_reason reason)
{
	switch (reason) {
	case WAKE_CQ_INTR:
		WRITE_ONCE(intr->wake, true);
		break;
	case WAKE_PORT_DISABLED:
		WRITE_ONCE(intr->disabled, true);
		break;
	default:
		break;
	}

	wake_up_interruptible(&intr->wq_head);
}

static inline bool wake_condition(struct dlb_cq_intr *intr,
				  struct dlb_dev *dev,
				  struct dlb_status *status)
{
	return (READ_ONCE(intr->wake) ||
		READ_ONCE(dev->reset_active) ||
		!READ_ONCE(status->valid) ||
		READ_ONCE(intr->disabled));
}

struct dlb_dequeue_qe {
	u8 rsvd0[15];
	u8 cq_gen:1;
	u8 rsvd1:7;
} __packed;

/**
 * dlb_cq_empty() - determine whether a CQ is empty
 * @dev: struct dlb_dev pointer.
 * @user_cq_va: User VA pointing to next CQ entry.
 * @cq_gen: Current CQ generation bit.
 *
 * Return:
 * Returns 1 if empty, 0 if non-empty, or < 0 if an error occurs.
 */
static int dlb_cq_empty(struct dlb_dev *dev, u64 user_cq_va, u8 cq_gen)
{
	struct dlb_dequeue_qe qe;

	if (copy_from_user(&qe, (void __user *)user_cq_va, sizeof(qe))) {
		dev_err(dev->dlb_device,
			"[%s()] Invalid cq_va pointer\n", __func__);
		return -EFAULT;
	}

	return qe.cq_gen != cq_gen;
}

int dlb_block_on_cq_interrupt(struct dlb_dev *dev,
			      struct dlb_status *status,
			      int domain_id,
			      int port_id,
			      bool is_ldb,
			      u64 cq_va,
			      u8 cq_gen,
			      bool arm)
{
	struct dlb_cq_intr *intr;
	int ret = 0;

	if (is_ldb && port_id >= DLB_MAX_NUM_LDB_PORTS)
		return -EINVAL;
	if (!is_ldb && port_id >= DLB_MAX_NUM_DIR_PORTS)
		return -EINVAL;

	if (is_ldb)
		intr = &dev->intr.ldb_cq_intr[port_id];
	else
		intr = &dev->intr.dir_cq_intr[port_id];

	/* If the user assigns more CQs to a VF resource group than there are
	 * interrupt vectors (31 per VF), then some of its CQs won't be
	 * configured for interrupts. Also on A-stepping hardware, the software
	 * workaround for a write-buffer lockup issue requires CQs to not be
	 * configured for interrupts.
	 */
	if (unlikely(!intr->configured))
		return -EINVAL;

	/* This function requires that only one thread process the CQ at a time.
	 * Otherwise, the wake condition could become false in the time between
	 * the ISR calling wake_up_interruptible() and the thread checking its
	 * wake condition.
	 */
	mutex_lock(&intr->mutex);

	/* Return early if the port's interrupt is disabled */
	if (READ_ONCE(intr->disabled)) {
		mutex_unlock(&intr->mutex);
		return -EACCES;
	}

	dev_dbg(dev->dlb_device,
		"Thread is blocking on %s port %d's interrupt\n",
		(is_ldb) ? "LDB" : "DIR", port_id);

	/* Don't block if the CQ is non-empty */
	ret = dlb_cq_empty(dev, cq_va, cq_gen);
	if (ret != 1)
		goto error;

	if (arm) {
		ret = dev->ops->arm_cq_interrupt(dev,
						 domain_id,
						 port_id,
						 is_ldb);
		if (ret)
			goto error;
	}

	ret = wait_event_interruptible(intr->wq_head,
				       wake_condition(intr, dev, status));

	if (ret == 0) {
		if (READ_ONCE(dev->reset_active) ||
		    !READ_ONCE(status->valid))
			ret = -EINTR;
		else if (READ_ONCE(intr->disabled))
			ret = -EACCES;
	}

	dev_dbg(dev->dlb_device,
		"Thread is unblocked from %s port %d's interrupt\n",
		(is_ldb) ? "LDB" : "DIR", port_id);

	WRITE_ONCE(intr->wake, false);

error:
	mutex_unlock(&intr->mutex);

	return ret;
}
