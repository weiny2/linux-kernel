// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 Intel Corporation. All rights rsvd. */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/workqueue.h>
#include <linux/aer.h>
#include <linux/fs.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/device.h>
#include <linux/idr.h>
#include <linux/notifier.h>
#include <linux/intel-svm.h>
#include <linux/dmaengine.h>
#include <uapi/linux/idxd.h>
#include <linux/idxd.h>
#include "../dmatest.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");

static int type = IDXD_TYPE_ANY;
module_param(type, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(type,
		 "Device type (-1: any, 0: DSA, 1: IAX)");

static int mode = IDXD_WQ_ANY;
module_param(mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mode,
		 "WQ mode (-1: any, 0: dedicated, 1: shared)");

static int node = NUMA_NO_NODE;
module_param(node, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(node, "NUMA node for device (-1: any)");

static int bof = 1;
module_param(bof, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(bof, "Support block on fault (0: no 1: yes)");

static struct dmatest_kdirect_ops ktest_ops;

static int submit_memmove_operation(struct dma_chan *chan,
				    struct dmatest_data *src,
				    struct dmatest_data *dst,
				    unsigned int len)
{
	struct dma_device *dma = chan->device;
	struct idxd_desc *desc;
	struct dsa_hw_desc *hw;
	int rc;
	u8 status;

	desc = (struct idxd_desc *)dma->kdops.device_get_desc(chan,
							      IDXD_OP_BLOCK);
	if (!desc) {
		pr_warn("desc alloc failed\n");
		return -ENOMEM;
	}

	hw = desc->hw;
	hw->flags = IDXD_OP_FLAG_CRAV | IDXD_OP_FLAG_RCR | IDXD_OP_FLAG_RCI;
	if (bof)
		hw->flags |= IDXD_OP_FLAG_BOF;
	hw->opcode = DSA_OPCODE_MEMMOVE;
	hw->src_addr = (u64)(u64 *)(src->aligned[0] + src->off);
	hw->dst_addr = (u64)(u64 *)(dst->aligned[0] + dst->off);
	hw->xfer_size = len;
	hw->priv = 1;
	hw->completion_addr = (u64)(u64 *)desc->completion;
	/* int_handle and pasid filled by alloc_desc */

	rc = dma->kdops.device_submit_and_wait(chan, desc, IDXD_OP_BLOCK,
					       jiffies_to_msecs(100));
	if (rc < 0) {
		pr_warn("desc submit failed\n");
		rc = -ENXIO;
		goto err;
	}

	status = desc->completion->status & DSA_COMP_STATUS_MASK;
	if (status != DSA_COMP_SUCCESS) {
		rc = -ENXIO;
		goto err;
	}

err:
	dma->kdops.device_free_desc(chan, desc);
	return rc;
}

static int param_check(void)
{
	if (type < IDXD_TYPE_ANY || type >= IDXD_TYPE_MAX) {
		pr_warn("Device type (%d) invalid\n", type);
		return -EINVAL;
	}

	if (mode >= IDXD_WQ_MAX || mode < IDXD_WQ_ANY) {
		pr_warn("WQ mode (%d) invalid\n", mode);
		return -EINVAL;
	}

	return 0;
}

static int __init ktest_init(void)
{
	struct idxd_wq_request *req;
	int rc;

	rc = param_check();
	if (rc)
		return rc;

	req = kmalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	sprintf(req->name, "dmaengine");
	req->node = node;
	req->type = type;
	req->mode = mode;
	ktest_ops.filter_param = req;
	ktest_ops.operation = submit_memmove_operation;
	ktest_ops.dma_filter = idxd_filter_kdirect;

	rc = dmatest_kdirect_register(&ktest_ops);
	if (!rc)
		return rc;

	return 0;
}
module_init(ktest_init);

static void __exit ktest_exit(void)
{
	dmatest_kdirect_unregister();
	kfree(ktest_ops.filter_param);
}
module_exit(ktest_exit);
