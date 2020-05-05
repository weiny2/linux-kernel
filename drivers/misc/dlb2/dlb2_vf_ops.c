// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/uaccess.h>

#include "base/dlb2_mbox.h"
#include "base/dlb2_osdep.h"
#include "base/dlb2_resource.h"
#include "dlb2_dp_ops.h"
#include "dlb2_intr.h"
#include "dlb2_main.h"

/***********************************/
/****** Mailbox communication ******/
/***********************************/

static int
dlb2_send_sync_mbox_cmd(struct dlb2_dev *dlb2_dev,
			void *data,
			int len,
			int timeout_s)
{
	int ret, retry_cnt;

	if (len > DLB2_FUNC_VF_VF2PF_MAILBOX_BYTES) {
		dev_err(dlb2_dev->dlb2_device,
			"Internal error: VF mbox message too large\n");
		return -1;
	}

	ret = dlb2_vf_write_pf_mbox_req(&dlb2_dev->hw, data, len);
	if (ret)
		return ret;

	dlb2_send_async_vdev_to_pf_msg(&dlb2_dev->hw);

	/* Timeout after timeout_s seconds of inactivity */
	retry_cnt = 1000 * timeout_s;
	do {
		if (dlb2_vdev_to_pf_complete(&dlb2_dev->hw))
			break;
		usleep_range(1000, 1001);
	} while (--retry_cnt);

	if (!retry_cnt) {
		dev_err(dlb2_dev->dlb2_device,
			"VF driver timed out waiting for mbox response\n");
		return -1;
	}

	return 0;
}

/***********************************/
/****** Runtime PM management ******/
/***********************************/

static void
dlb2_vf_pm_inc_refcnt(struct pci_dev *pdev, bool resume)
{
}

static void
dlb2_vf_pm_dec_refcnt(struct pci_dev *pdev)
{
}

static bool
dlb2_vf_pm_status_suspended(struct pci_dev *pdev)
{
	return false;
}

/********************************/
/****** PCI BAR management ******/
/********************************/

static void
dlb2_vf_unmap_pci_bar_space(struct dlb2_dev *dlb2_dev,
			    struct pci_dev *pdev)
{
	pci_iounmap(pdev, dlb2_dev->hw.func_kva);
}

static int
dlb2_vf_map_pci_bar_space(struct dlb2_dev *dlb2_dev,
			  struct pci_dev *pdev)
{
	enum dlb2_virt_mode mode;

	/* BAR 0: VF FUNC BAR space */
	dlb2_dev->hw.func_kva = pci_iomap(pdev, 0, 0);
	dlb2_dev->hw.func_phys_addr = pci_resource_start(pdev, 0);

	if (!dlb2_dev->hw.func_kva) {
		dev_err(&pdev->dev, "Cannot iomap BAR 0 (size %llu)\n",
			pci_resource_len(pdev, 0));

		return -EIO;
	}

	dev_dbg(&pdev->dev, "BAR 0 iomem base: %p\n",
		dlb2_dev->hw.func_kva);
	dev_dbg(&pdev->dev, "BAR 0 start: 0x%llx\n",
		pci_resource_start(pdev, 0));
	dev_dbg(&pdev->dev, "BAR 0 len: %llu\n",
		pci_resource_len(pdev, 0));

	/* Before the driver can use its mailbox, it needs to identify whether
	 * its device is a VF (SR-IOV) or VDEV (S-IOV), because the mailbox
	 * interface differs slightly among the two. Detect by looking for an
	 * MSI-X capability (S-IOV only).
	 */
	mode = (pdev->msix_cap) ? DLB2_VIRT_SIOV : DLB2_VIRT_SRIOV;

	dlb2_hw_set_virt_mode(&dlb2_dev->hw, mode);

	return 0;
}

#define DLB2_LDB_CQ_BOUND DLB2_LDB_CQ_OFFS(DLB2_MAX_NUM_LDB_PORTS)
#define DLB2_DIR_CQ_BOUND DLB2_DIR_CQ_OFFS(DLB2_MAX_NUM_DIR_PORTS)
#define DLB2_LDB_PC_BOUND DLB2_LDB_PC_OFFS(DLB2_MAX_NUM_LDB_PORTS)
#define DLB2_DIR_PC_BOUND DLB2_DIR_PC_OFFS(DLB2_MAX_NUM_DIR_PORTS)

static int
dlb2_vf_mmap(struct file *f,
	     struct vm_area_struct *vma,
	     u32 domain_id)
{
	struct dlb2_dev *dev;
	unsigned long offset;
	unsigned long pgoff;
	struct page *page;
	pgprot_t pgprot;
	u32 port_id;

	dev = container_of(f->f_inode->i_cdev, struct dlb2_dev, cdev);

	offset = vma->vm_pgoff << PAGE_SHIFT;

	/* The VF's ops callbacks go through the mailbox, so the caller must
	 * hold the resource_mutex.
	 */

	if (offset >= DLB2_LDB_CQ_BASE && offset < DLB2_LDB_CQ_BOUND) {
		if ((vma->vm_end - vma->vm_start) != DLB2_LDB_CQ_MAX_SIZE)
			return -EINVAL;

		pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - DLB2_LDB_CQ_BASE) / DLB2_LDB_CQ_MAX_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		page = virt_to_page(dev->ldb_port_mem[port_id].cq_base);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= DLB2_DIR_CQ_BASE && offset < DLB2_DIR_CQ_BOUND) {
		if ((vma->vm_end - vma->vm_start) != DLB2_DIR_CQ_MAX_SIZE)
			return -EINVAL;

		pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - DLB2_DIR_CQ_BASE) / DLB2_DIR_CQ_MAX_SIZE;

		if (dev->ops->dir_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		page = virt_to_page(dev->dir_port_mem[port_id].cq_base);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= DLB2_LDB_PP_BASE && offset < DLB2_LDB_PP_BOUND) {
		if ((vma->vm_end - vma->vm_start) != PAGE_SIZE)
			return -EINVAL;

		pgoff = (dev->hw.func_phys_addr >> PAGE_SHIFT) + vma->vm_pgoff;

		port_id = (offset - DLB2_LDB_PP_BASE) / PAGE_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		pgprot = pgprot_noncached(vma->vm_page_prot);

	} else if (offset >= DLB2_DIR_PP_BASE && offset < DLB2_DIR_PP_BOUND) {
		if ((vma->vm_end - vma->vm_start) != PAGE_SIZE)
			return -EINVAL;

		pgoff = (dev->hw.func_phys_addr >> PAGE_SHIFT) + vma->vm_pgoff;

		port_id = (offset - DLB2_DIR_PP_BASE) / PAGE_SIZE;

		if (dev->ops->dir_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		pgprot = pgprot_noncached(vma->vm_page_prot);

	} else {
		return -EINVAL;
	}

	return io_remap_pfn_range(vma,
				  vma->vm_start,
				  pgoff,
				  vma->vm_end - vma->vm_start,
				  pgprot);
}

/**********************************/
/****** Interrupt management ******/
/**********************************/

/* Claim any unclaimed CQ interrupts from the primary VF. We use the primary's
 * *_cq_intr[] structure, vs. the auxiliary's copy of that structure, because
 * if the aux VFs are unbound, their memory will be lost and any blocked
 * threads in the primary's waitqueues could access their freed memory.
 */
static void
dlb2_vf_claim_cq_interrupts(struct dlb2_dev *dlb2_dev)
{
	struct dlb2_dev *primary_vf;
	int i, cnt;
	u8 nvecs;

	dlb2_dev->intr.num_cq_intrs = 0;
	primary_vf = dlb2_dev->vf_id_state.primary_vf;

	if (!primary_vf)
		return;

	nvecs = DLB2_VF_NUM_CQ_INTERRUPT_VECTORS;
	cnt = 0;

	for (i = 0; i < primary_vf->intr.num_ldb_ports; i++) {
		if (primary_vf->intr.ldb_cq_intr_owner[i])
			continue;

		primary_vf->intr.ldb_cq_intr_owner[i] = dlb2_dev;

		dlb2_dev->intr.msi_map[cnt].port_id = i;
		dlb2_dev->intr.msi_map[cnt].is_ldb = true;
		cnt++;

		dlb2_dev->intr.num_cq_intrs++;

		if (dlb2_dev->intr.num_cq_intrs == nvecs)
			return;
	}

	for (i = 0; i <  primary_vf->intr.num_dir_ports; i++) {
		if (primary_vf->intr.dir_cq_intr_owner[i])
			continue;

		primary_vf->intr.dir_cq_intr_owner[i] = dlb2_dev;

		dlb2_dev->intr.msi_map[cnt].port_id = i;
		dlb2_dev->intr.msi_map[cnt].is_ldb = false;
		cnt++;

		dlb2_dev->intr.num_cq_intrs++;

		if (dlb2_dev->intr.num_cq_intrs == nvecs)
			return;
	}
}

static void
dlb2_vf_unclaim_cq_interrupts(struct dlb2_dev *dlb2_dev)
{
	struct dlb2_dev *primary_vf;
	int i;

	primary_vf = dlb2_dev->vf_id_state.primary_vf;

	if (!primary_vf)
		return;

	for (i = 0; i < DLB2_MAX_NUM_LDB_PORTS; i++) {
		if (primary_vf->intr.ldb_cq_intr_owner[i] != dlb2_dev)
			continue;

		primary_vf->intr.ldb_cq_intr_owner[i] = NULL;
	}

	for (i = 0; i < DLB2_MAX_NUM_DIR_PORTS; i++) {
		if (primary_vf->intr.dir_cq_intr_owner[i] != dlb2_dev)
			continue;

		primary_vf->intr.dir_cq_intr_owner[i] = NULL;
	}
}

static void
dlb2_vf_mbox_cmd_alarm_fn(struct dlb2_dev *dev, void *data)
{
	struct dlb2_mbox_vf_alert_cmd_req *req = data;

	os_notify_user_space(&dev->hw,
			     req->domain_id,
			     req->alert_id,
			     req->aux_alert_data);

	/* No response needed beyond ACKing the interrupt. */
}

static void
dlb2_vf_mbox_cmd_notification_fn(struct dlb2_dev *dev, void *data)
{
	struct dlb2_mbox_vf_notification_cmd_req *req = data;

	/* If the VF is auxiliary, it has no resources affected by PF reset */
	if (dev->vf_id_state.is_auxiliary_vf)
		return;

	/* When the PF is reset, it notifies every registered VF driver twice:
	 * once before the reset, and once after.
	 *
	 * The pre-reset notification gives the VF an opportunity to notify its
	 * users to shutdown. The PF driver will not proceed with the reset
	 * until either all VF-owned domains are reset (and all the PF's users
	 * quiesce), or the PF driver's reset wait timeout expires.
	 *
	 * The post-reset notification gives the VF driver the all-clear to
	 * allow applications to begin running again.
	 */
	if (req->notification == DLB2_MBOX_VF_NOTIFICATION_PRE_RESET) {
		/* Before the reset occurs, wake up all active users and block
		 * them from continuing to access the device. Hold the
		 * resource_mutex during reset to prevent new users.
		 */
		mutex_lock(&dev->resource_mutex);

		/* Block any new device files from being opened */
		dev->reset_active = true;

		/* Stop existing applications from continuing to use the device
		 * by blocking kernel driver interfaces and waking any threads
		 * on wait queues.
		 */
		dlb2_stop_users(dev);

		/* Zap any MMIO mappings that could be used to access the
		 * device during the FLR
		 */
		dlb2_zap_vma_entries(dev);

	} else if (req->notification == DLB2_MBOX_VF_NOTIFICATION_POST_RESET) {
		int i;

		/* Clear all status pointers, to be filled in by post-FLR
		 * applications using the device driver.
		 *
		 * Note that status memory isn't leaked -- it is either freed
		 * during dlb2_stop_users() or in the file close callback.
		 */
		for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++)
			dev->sched_domains[i].status = NULL;

		dev->status = NULL;

		/* Free allocated CQ and PC memory. These are no longer
		 * accessible to user-space: either the applications closed, or
		 * their mappings were zapped.
		 */
		dlb2_release_device_memory(dev);

		/* Reset resource allocation state */
		dlb2_resource_reset(&dev->hw);

		dev->reset_active = false;

		/* Release resource_mutex, opening driver to new users. */
		mutex_unlock(&dev->resource_mutex);
	}

	/* No response needed beyond ACKing the interrupt. */
}

static void
dlb2_vf_mbox_cmd_in_use_fn(struct dlb2_dev *dev, void *data)
{
	struct dlb2_mbox_vf_in_use_cmd_resp resp;

	/* If the VF is auxiliary, the PF shouldn't send it an in-use request */
	if (dev->vf_id_state.is_auxiliary_vf) {
		dev_err(dev->dlb2_device,
			"Internal error: VF in-use request sent to auxiliary vf %d\n",
			dev->vf_id_state.vf_id);
		return;
	}

	resp.in_use = dlb2_in_use(dev);
	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;

	dlb2_vf_write_pf_mbox_resp(&dev->hw, &resp, sizeof(resp));
}

static void (*vf_mbox_fn_table[])(struct dlb2_dev *dev, void *data) = {
	dlb2_vf_mbox_cmd_alarm_fn,
	dlb2_vf_mbox_cmd_notification_fn,
	dlb2_vf_mbox_cmd_in_use_fn,
};

/* If an mbox request handler acquires the resource mutex, deadlock can occur.
 * For example:
 * - The PF driver grabs its resource mutex and issues a mailbox request to VF
 *   N, then waits for a response.
 * - At the same time, VF N grabs its resource mutex and issues a mailbox
 *   request, then waits for a response.
 *
 * In this scenario, both the PF and VF's mailbox handlers will block
 * attempting to grab their respective resource mutex.
 *
 * We avoid this deadlock by deferring the execution of VF handlers that
 * acquire the resource mutex until after ACKing the interrupt, which allows
 * the PF to release its resource mutex. This is possible because those VF
 * handlers don't send any response data to the PF, which must be sent prior to
 * ACKing the interrupt.
 *
 * In fact, we defer any handler that doesn't send a response, including those
 * that don't acquire the resource mutex. Handlers that respond to the PF
 * cannot be deferred.
 */
static const bool deferred_mbox_hdlrs[] = {
	[DLB2_MBOX_VF_CMD_DOMAIN_ALERT] = true,
	[DLB2_MBOX_VF_CMD_NOTIFICATION] = true,
	[DLB2_MBOX_VF_CMD_IN_USE] = false,
};

static void
dlb2_vf_handle_pf_req(struct dlb2_dev *dev)
{
	u8 data[DLB2_PF2VF_REQ_BYTES];
	bool deferred;

	dlb2_vf_read_pf_mbox_req(&dev->hw, &data, sizeof(data));

	deferred = deferred_mbox_hdlrs[DLB2_MBOX_CMD_TYPE(data)];

	if (!deferred)
		vf_mbox_fn_table[DLB2_MBOX_CMD_TYPE(data)](dev, data);

	dlb2_ack_pf_mbox_int(&dev->hw);

	if (deferred)
		vf_mbox_fn_table[DLB2_MBOX_CMD_TYPE(data)](dev, data);
}

static irqreturn_t
dlb2_vf_intr_handler(int irq, void *hdlr_ptr)
{
	struct dlb2_dev *dev = hdlr_ptr;
	struct dlb2_dev *primary_vf;
	u32 interrupts, mask, ack;
	int vector;
	int i;

	dev_dbg(dev->dlb2_device, "Entered ISR\n");

	primary_vf = dev->vf_id_state.primary_vf;

	vector = irq - dev->intr.base_vector;

	mask = dev->intr.num_vectors - 1;

	interrupts = dlb2_read_vf_intr_status(&dev->hw);

	ack = 0;

	for (i = 0; i < DLB2_VF_TOTAL_NUM_INTERRUPT_VECTORS; i++) {
		if ((i & mask) == vector && (interrupts & (1 << i)))
			ack |= (1 << i);
	}

	dlb2_ack_vf_intr_status(&dev->hw, ack);

	for (i = 0; i < DLB2_VF_TOTAL_NUM_INTERRUPT_VECTORS; i++) {
		struct dlb2_cq_intr *intr;
		int port_id;

		if ((i & mask) != vector || !(interrupts & (1 << i)))
			continue;

		dev_dbg(dev->dlb2_device,
			"[%s()] VF intr vector %d fired\n",
			__func__, i);

		if (i == DLB2_VF_MBOX_VECTOR_ID) {
			dlb2_vf_handle_pf_req(dev);
			continue;
		}

		port_id = dev->intr.msi_map[i].port_id;

		if (dev->intr.msi_map[i].is_ldb)
			intr = &primary_vf->intr.ldb_cq_intr[port_id];
		else
			intr = &primary_vf->intr.dir_cq_intr[port_id];

		dlb2_wake_thread(dev, intr, WAKE_CQ_INTR);
	}

	dlb2_ack_vf_msi_intr(&dev->hw, 1 << vector);

	return IRQ_HANDLED;
}

static irqreturn_t
dlb2_vdev_intr_handler(int irq, void *hdlr_ptr)
{
	struct dlb2_dev *dev = hdlr_ptr;
	int vector;

	vector = irq - dev->intr.base_vector;

	dev_dbg(dev->dlb2_device,
		"[%s()] Intr vector %d fired\n",
		__func__, vector);

	if (vector == DLB2_INT_NON_CQ) {
		dlb2_vf_handle_pf_req(dev);
	} else {
		struct dlb2_cq_intr *intr;
		int port_id, idx;

		idx = vector - 1;
		port_id = dev->intr.msi_map[idx].port_id;

		if (dev->intr.msi_map[idx].is_ldb)
			intr = &dev->intr.ldb_cq_intr[port_id];
		else
			intr = &dev->intr.dir_cq_intr[port_id];

		dlb2_wake_thread(dev, intr, WAKE_CQ_INTR);
	}

	return IRQ_HANDLED;
}

static void
dlb2_vf_get_cq_interrupt_name(struct dlb2_dev *dev, int vector)
{
	int port_id;
	bool is_ldb;

	port_id = dev->intr.msi_map[vector].port_id;
	is_ldb = dev->intr.msi_map[vector].is_ldb;

	snprintf(dev->intr.msi_map[vector].name,
		 sizeof(dev->intr.msi_map[vector].name) - 1,
		 "dlb2_%s_cq_%d", is_ldb ? "ldb" : "dir", port_id);
}

static int
dlb2_vf_init_interrupt_handlers(struct dlb2_dev *dev,
				struct pci_dev *pdev)
{
	int i, ret;

	/* Request CQ interrupt vectors */
	for (i = 0; i < dev->intr.num_vectors - 1; i++) {
		/* We allocate IRQ vectors in power-of-2 units but may have
		 * non-power-of-2 CQs to service. Don't register more handlers
		 * than are needed.
		 */
		if (i == dev->intr.num_cq_intrs)
			break;

		dlb2_vf_get_cq_interrupt_name(dev, i);

		ret = devm_request_threaded_irq(&pdev->dev,
						pci_irq_vector(pdev, i),
						NULL,
						dlb2_vf_intr_handler,
						IRQF_ONESHOT,
						dev->intr.msi_map[i].name,
						dev);
		if (ret)
			return ret;

		dev->intr.isr_registered[i] = true;
	}

	/* Request the mailbox interrupt vector */
	i = dev->intr.num_vectors - 1;

	ret = devm_request_threaded_irq(&pdev->dev,
					pci_irq_vector(pdev, i),
					NULL,
					dlb2_vf_intr_handler,
					IRQF_ONESHOT,
					"dlb2_pf_to_vf_mbox",
					dev);
	if (ret)
		return ret;

	dev->intr.isr_registered[i] = true;

	return 0;
}

static int
dlb2_vdev_init_interrupt_handlers(struct dlb2_dev *dev,
				  struct pci_dev *pdev)
{
	int i, ret;

	/* Request the mailbox interrupt vector */
	i = DLB2_INT_NON_CQ;

	ret = devm_request_threaded_irq(&pdev->dev,
					pci_irq_vector(pdev, i),
					NULL,
					dlb2_vdev_intr_handler,
					IRQF_ONESHOT,
					"dlb2_pf_to_vf_mbox",
					dev);
	if (ret)
		return ret;

	dev->intr.isr_registered[i] = true;

	i++;

	/* Request CQ interrupt vectors */
	for (; i < dev->intr.num_vectors; i++) {
		int cq_idx = i - 1;
		char *name;

		name = dev->intr.msi_map[cq_idx].name;

		dlb2_vf_get_cq_interrupt_name(dev, cq_idx);

		ret = devm_request_threaded_irq(&pdev->dev,
						pci_irq_vector(pdev, i),
						NULL,
						dlb2_vdev_intr_handler,
						IRQF_ONESHOT,
						name,
						dev);
		if (ret)
			return ret;

		dev->intr.isr_registered[i] = true;
	}

	return 0;
}

static int
dlb2_vf_init_sriov_interrupts(struct dlb2_dev *dev,
			      struct pci_dev *pdev)
{
	int ret, req_size;
	int num_cq_intrs;

	/* Claim a batch of CQs from the primary VF for assigning to MSI
	 * vectors (if the primary VF has been probed).
	 */
	dlb2_vf_claim_cq_interrupts(dev);

	/* Request IRQ vectors. The request size depends on the number of CQs
	 * this VF claimed -- it will attempt to take enough for a 1:1 mapping,
	 * else it falls back to a single vector.
	 */
	num_cq_intrs = dev->intr.num_cq_intrs;

	if ((num_cq_intrs + DLB2_VF_NUM_NON_CQ_INTERRUPT_VECTORS) > 16)
		req_size = 32;
	else if ((num_cq_intrs + DLB2_VF_NUM_NON_CQ_INTERRUPT_VECTORS) > 8)
		req_size = 16;
	else if ((num_cq_intrs + DLB2_VF_NUM_NON_CQ_INTERRUPT_VECTORS) > 4)
		req_size = 8;
	else if ((num_cq_intrs + DLB2_VF_NUM_NON_CQ_INTERRUPT_VECTORS) > 2)
		req_size = 4;
	else if ((num_cq_intrs + DLB2_VF_NUM_NON_CQ_INTERRUPT_VECTORS) > 1)
		req_size = 2;
	else
		req_size = 1;

	ret = pci_alloc_irq_vectors(pdev, req_size, req_size, PCI_IRQ_MSI);
	if (ret < 0) {
		ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI);
		if (ret < 0)
			return ret;
	}

	dev->intr.num_vectors = ret;
	dev->intr.base_vector = pci_irq_vector(pdev, 0);

	return 0;
}

static int
dlb2_vf_init_siov_interrupts(struct dlb2_dev *dev,
			     struct pci_dev *pdev)
{
	int ret, nvec, i;

	dev->intr.num_cq_intrs = 0;

	for (i = 0; i < dev->intr.num_ldb_ports; i++) {
		dev->intr.ldb_cq_intr_owner[i] = dev;
		dev->intr.msi_map[i].port_id = i;
		dev->intr.msi_map[i].is_ldb = true;
	}

	for (i = 0; i < dev->intr.num_dir_ports; i++) {
		int idx = dev->intr.num_ldb_ports + i;

		dev->intr.dir_cq_intr_owner[i] = dev;
		dev->intr.msi_map[idx].port_id = i;
		dev->intr.msi_map[idx].is_ldb = false;
	}

	nvec = pci_msix_vec_count(pdev);
	if (nvec < 0)
		return nvec;

	ret = pci_alloc_irq_vectors(pdev, nvec, nvec, PCI_IRQ_MSIX);
	if (ret < 0) {
		dev_err(dev->dlb2_device,
			"Error: unable to allocate %d MSI-X vectors.\n",
			nvec);
		return ret;
	}

	dev->intr.num_vectors = ret;
	dev->intr.base_vector = pci_irq_vector(pdev, 0);

	dev->intr.num_cq_intrs = ret - 1;

	return 0;
}

static void
dlb2_vf_free_interrupts(struct dlb2_dev *dev,
			struct pci_dev *pdev)
{
	int i;

	if (dlb2_hw_get_virt_mode(&dev->hw) == DLB2_VIRT_SRIOV)
		dlb2_vf_unclaim_cq_interrupts(dev);

	for (i = 0; i < dev->intr.num_vectors; i++) {
		if (dev->intr.isr_registered[i])
			devm_free_irq(&pdev->dev, pci_irq_vector(pdev, i), dev);
	}

	pci_free_irq_vectors(pdev);
}

static int
dlb2_vf_init_interrupts(struct dlb2_dev *dev, struct pci_dev *pdev)
{
	int ret, i;

	if (dlb2_hw_get_virt_mode(&dev->hw) == DLB2_VIRT_SRIOV)
		ret = dlb2_vf_init_sriov_interrupts(dev, pdev);
	else
		ret = dlb2_vf_init_siov_interrupts(dev, pdev);

	if (ret)
		return ret;

	if (dlb2_hw_get_virt_mode(&dev->hw) == DLB2_VIRT_SRIOV)
		ret = dlb2_vf_init_interrupt_handlers(dev, pdev);
	else
		ret = dlb2_vdev_init_interrupt_handlers(dev, pdev);

	if (ret) {
		dlb2_vf_free_interrupts(dev, pdev);
		return ret;
	}

	/* Initialize per-CQ interrupt structures, such as wait queues that
	 * threads will wait on until the CQ's interrupt fires.
	 */
	for (i = 0; i < DLB2_MAX_NUM_LDB_PORTS; i++) {
		init_waitqueue_head(&dev->intr.ldb_cq_intr[i].wq_head);
		mutex_init(&dev->intr.ldb_cq_intr[i].mutex);
	}

	for (i = 0; i < DLB2_MAX_NUM_DIR_PORTS; i++) {
		init_waitqueue_head(&dev->intr.dir_cq_intr[i].wq_head);
		mutex_init(&dev->intr.dir_cq_intr[i].mutex);
	}

	return 0;
}

static void
dlb2_vf_reinit_interrupts(struct dlb2_dev *dev)
{
}

static int
dlb2_vf_enable_ldb_cq_interrupts(struct dlb2_dev *dev,
				 int id,
				 u16 thresh)
{
	struct dlb2_mbox_enable_ldb_port_intr_cmd_resp resp;
	struct dlb2_mbox_enable_ldb_port_intr_cmd_req req;
	struct dlb2_dev *owner;
	int ret;
	int i;

	/* If no owner was registered, dev->intr...configured remains false,
	 * and any attempts to block on the CQ interrupt will fail. This will
	 * only happen if the VF doesn't have enough auxiliary VFs to service
	 * its CQ interrupts.
	 */
	owner = dev->intr.ldb_cq_intr_owner[id];
	if (!owner) {
		dev_dbg(dev->dlb2_device,
			"[%s()] LDB port %d has no interrupt owner\n",
			__func__, id);
		return 0;
	}

	for (i = 0; i <= owner->intr.num_cq_intrs; i++) {
		if (owner->intr.msi_map[i].port_id == id &&
		    owner->intr.msi_map[i].is_ldb)
			break;
	}

	dev->intr.ldb_cq_intr[id].disabled = false;
	dev->intr.ldb_cq_intr[id].configured = true;

	req.hdr.type = DLB2_MBOX_CMD_ENABLE_LDB_PORT_INTR;
	req.port_id = id;
	req.vector = i;
	req.owner_vf = owner->vf_id_state.vf_id;
	req.thresh = thresh;

	ret = dlb2_send_sync_mbox_cmd(dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dev->dlb2_device,
			"LDB CQ interrupt enable failed with mailbox error: %s\n",
			DLB2_MBOX_ST_STRING(&resp));
	}

	return resp.error_code;
}

static int
dlb2_vf_enable_dir_cq_interrupts(struct dlb2_dev *dev,
				 int id,
				 u16 thresh)
{
	struct dlb2_mbox_enable_dir_port_intr_cmd_resp resp;
	struct dlb2_mbox_enable_dir_port_intr_cmd_req req;
	struct dlb2_dev *owner;
	int ret;
	int i;

	/* If no owner was registered, dev->intr...configured remains false,
	 * and any attempts to block on the CQ interrupt will fail. This will
	 * only happen if the VF doesn't have enough auxiliary VFs to service
	 * its CQ interrupts.
	 */
	owner = dev->intr.dir_cq_intr_owner[id];
	if (!owner) {
		dev_dbg(dev->dlb2_device,
			"[%s()] DIR port %d has no interrupt owner\n",
			__func__, id);
		return 0;
	}

	for (i = 0; i <= owner->intr.num_cq_intrs; i++) {
		if (owner->intr.msi_map[i].port_id == id &&
		    !owner->intr.msi_map[i].is_ldb)
			break;
	}

	dev->intr.dir_cq_intr[id].disabled = false;
	dev->intr.dir_cq_intr[id].configured = true;

	req.hdr.type = DLB2_MBOX_CMD_ENABLE_DIR_PORT_INTR;
	req.port_id = id;
	req.vector = i;
	req.owner_vf = owner->vf_id_state.vf_id;
	req.thresh = thresh;

	ret = dlb2_send_sync_mbox_cmd(dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dev->dlb2_device,
			"DIR CQ interrupt enable failed with mailbox error: %s\n",
			DLB2_MBOX_ST_STRING(&resp));
	}

	return resp.error_code;
}

static int
dlb2_vf_arm_cq_interrupt(struct dlb2_dev *dev,
			 int domain_id,
			 int port_id,
			 bool is_ldb)
{
	struct dlb2_mbox_arm_cq_intr_cmd_resp resp;
	struct dlb2_mbox_arm_cq_intr_cmd_req req;
	int ret;

	req.hdr.type = DLB2_MBOX_CMD_ARM_CQ_INTR;
	req.domain_id = domain_id;
	req.port_id = port_id;
	req.is_ldb = is_ldb;

	/* Unlike other VF ioctl callbacks, this one isn't called while holding
	 * the resource mutex. However, we must serialize access to the mailbox
	 * to prevent data corruption.
	 */
	mutex_lock(&dev->resource_mutex);

	ret = dlb2_send_sync_mbox_cmd(dev, &req, sizeof(req), 1);
	if (ret) {
		mutex_unlock(&dev->resource_mutex);
		return ret;
	}

	dlb2_vf_read_pf_mbox_resp(&dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dev->dlb2_device,
			"LDB CQ interrupt enable failed with mailbox error: %s\n",
			DLB2_MBOX_ST_STRING(&resp));
	}

	mutex_unlock(&dev->resource_mutex);

	return resp.error_code;
}

/*******************************/
/****** Driver management ******/
/*******************************/

static int
dlb2_vf_init_driver_state(struct dlb2_dev *dlb2_dev)
{
	int i;

	if (movdir64b_supported()) {
		dlb2_dev->enqueue_four = dlb2_movdir64b;
	} else {
#ifdef CONFIG_AS_SSE2
		dlb2_dev->enqueue_four = dlb2_movntdq;
#else
		dev_err(dlb2_dev->dlb2_device,
			"%s: Platforms without movdir64 must support SSE2\n",
			dlb2_driver_name);
		return -EINVAL;
#endif
	}

	/* Initialize software state */
	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++) {
		struct dlb2_domain_dev *domain = &dlb2_dev->sched_domains[i];

		mutex_init(&domain->alert_mutex);
		init_waitqueue_head(&domain->wq_head);
	}

	mutex_init(&dlb2_dev->resource_mutex);

	return 0;
}

static void
dlb2_vf_free_driver_state(struct dlb2_dev *dlb2_dev)
{
}

static int
dlb2_vf_cdev_add(struct dlb2_dev *dlb2_dev,
		 dev_t base,
		 const struct file_operations *fops)
{
	int ret;

	dlb2_dev->dev_number = MKDEV(MAJOR(base),
				     MINOR(base) +
				     (dlb2_dev->id *
				      DLB2_NUM_DEV_FILES_PER_DEVICE));

	cdev_init(&dlb2_dev->cdev, fops);

	dlb2_dev->cdev.dev   = dlb2_dev->dev_number;
	dlb2_dev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&dlb2_dev->cdev,
		       dlb2_dev->cdev.dev,
		       DLB2_NUM_DEV_FILES_PER_DEVICE);

	if (ret < 0)
		dev_err(dlb2_dev->dlb2_device,
			"%s: cdev_add() returned %d\n",
			dlb2_driver_name, ret);

	return ret;
}

static void
dlb2_vf_cdev_del(struct dlb2_dev *dlb2_dev)
{
	cdev_del(&dlb2_dev->cdev);
}

static int
dlb2_vf_device_create(struct dlb2_dev *dlb2_dev,
		      struct pci_dev *pdev,
		      struct class *dlb2_class)
{
	dev_t dev;

	dev = MKDEV(MAJOR(dlb2_dev->dev_number),
		    MINOR(dlb2_dev->dev_number) + DLB2_MAX_NUM_DOMAINS);

	/* Create a new device in order to create a /dev/dlb node. This device
	 * is a child of the DLB PCI device.
	 */
	dlb2_dev->dlb2_device = device_create(dlb2_class,
					      &pdev->dev,
					      dev,
					      dlb2_dev,
					      "dlb%d/dlb",
					      dlb2_dev->id);

	if (IS_ERR_VALUE(PTR_ERR(dlb2_dev->dlb2_device))) {
		dev_err(dlb2_dev->dlb2_device,
			"%s: device_create() returned %ld\n",
			dlb2_driver_name, PTR_ERR(dlb2_dev->dlb2_device));

		return PTR_ERR(dlb2_dev->dlb2_device);
	}

	return 0;
}

static void
dlb2_vf_device_destroy(struct dlb2_dev *dlb2_dev,
		       struct class *dlb2_class)
{
	device_destroy(dlb2_class,
		       MKDEV(MAJOR(dlb2_dev->dev_number),
			     MINOR(dlb2_dev->dev_number) +
				DLB2_MAX_NUM_DOMAINS));
}

static void
dlb2_vf_init_hardware(struct dlb2_dev *dlb2_dev)
{
	/* Function intentionally left blank */
}

/*****************************/
/****** Sysfs callbacks ******/
/*****************************/

static int
dlb2_vf_get_num_used_rsrcs(struct dlb2_hw *hw,
			   struct dlb2_get_num_resources_args *args)
{
	struct dlb2_mbox_get_num_resources_cmd_resp resp;
	struct dlb2_mbox_get_num_resources_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_GET_NUM_USED_RESOURCES;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		return -1;
	}

	args->num_sched_domains = resp.num_sched_domains;
	args->num_ldb_queues = resp.num_ldb_queues;
	args->num_ldb_ports = resp.num_ldb_ports;
	args->num_cos0_ldb_ports = resp.num_cos0_ldb_ports;
	args->num_cos1_ldb_ports = resp.num_cos1_ldb_ports;
	args->num_cos2_ldb_ports = resp.num_cos2_ldb_ports;
	args->num_cos3_ldb_ports = resp.num_cos3_ldb_ports;
	args->num_dir_ports = resp.num_dir_ports;
	args->num_atomic_inflights = resp.num_atomic_inflights;
	args->num_hist_list_entries = resp.num_hist_list_entries;
	args->max_contiguous_hist_list_entries =
		resp.max_contiguous_hist_list_entries;
	args->num_ldb_credits = resp.num_ldb_credits;
	args->num_dir_credits = resp.num_dir_credits;

	return resp.error_code;
}

#define DLB2_VF_TOTAL_SYSFS_SHOW(name)				\
static ssize_t total_##name##_show(				\
	struct device *dev,					\
	struct device_attribute *attr,				\
	char *buf)						\
{								\
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);	\
	struct dlb2_get_num_resources_args rsrcs[2];		\
	struct dlb2_hw *hw = &dlb2_dev->hw;			\
	int val, i;						\
								\
	mutex_lock(&dlb2_dev->resource_mutex);			\
								\
	val = dlb2_dev->ops->get_num_resources(hw, &rsrcs[0]);	\
	if (val) {						\
		mutex_unlock(&dlb2_dev->resource_mutex);	\
		return -1;					\
	}							\
								\
	val = dlb2_vf_get_num_used_rsrcs(hw, &rsrcs[1]);	\
	if (val) {						\
		mutex_unlock(&dlb2_dev->resource_mutex);	\
		return -1;					\
	}							\
								\
	mutex_unlock(&dlb2_dev->resource_mutex);		\
								\
	val = 0;						\
	for (i = 0; i < ARRAY_SIZE(rsrcs); i++)			\
		val += rsrcs[i].name;				\
								\
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);		\
}

DLB2_VF_TOTAL_SYSFS_SHOW(num_sched_domains)
DLB2_VF_TOTAL_SYSFS_SHOW(num_ldb_queues)
DLB2_VF_TOTAL_SYSFS_SHOW(num_ldb_ports)
DLB2_VF_TOTAL_SYSFS_SHOW(num_cos0_ldb_ports)
DLB2_VF_TOTAL_SYSFS_SHOW(num_cos1_ldb_ports)
DLB2_VF_TOTAL_SYSFS_SHOW(num_cos2_ldb_ports)
DLB2_VF_TOTAL_SYSFS_SHOW(num_cos3_ldb_ports)
DLB2_VF_TOTAL_SYSFS_SHOW(num_dir_ports)
DLB2_VF_TOTAL_SYSFS_SHOW(num_ldb_credits)
DLB2_VF_TOTAL_SYSFS_SHOW(num_dir_credits)
DLB2_VF_TOTAL_SYSFS_SHOW(num_atomic_inflights)
DLB2_VF_TOTAL_SYSFS_SHOW(num_hist_list_entries)

#define DLB2_VF_AVAIL_SYSFS_SHOW(name)				      \
static ssize_t avail_##name##_show(				      \
	struct device *dev,					      \
	struct device_attribute *attr,				      \
	char *buf)						      \
{								      \
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);	      \
	struct dlb2_get_num_resources_args num_avail_rsrcs;	      \
	struct dlb2_hw *hw = &dlb2_dev->hw;			      \
	int val;						      \
								      \
	mutex_lock(&dlb2_dev->resource_mutex);			      \
								      \
	val = dlb2_dev->ops->get_num_resources(hw, &num_avail_rsrcs); \
								      \
	mutex_unlock(&dlb2_dev->resource_mutex);		      \
								      \
	if (val)						      \
		return -1;					      \
								      \
	val = num_avail_rsrcs.name;				      \
								      \
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);		      \
}

DLB2_VF_AVAIL_SYSFS_SHOW(num_sched_domains)
DLB2_VF_AVAIL_SYSFS_SHOW(num_ldb_queues)
DLB2_VF_AVAIL_SYSFS_SHOW(num_ldb_ports)
DLB2_VF_AVAIL_SYSFS_SHOW(num_cos0_ldb_ports)
DLB2_VF_AVAIL_SYSFS_SHOW(num_cos1_ldb_ports)
DLB2_VF_AVAIL_SYSFS_SHOW(num_cos2_ldb_ports)
DLB2_VF_AVAIL_SYSFS_SHOW(num_cos3_ldb_ports)
DLB2_VF_AVAIL_SYSFS_SHOW(num_dir_ports)
DLB2_VF_AVAIL_SYSFS_SHOW(num_ldb_credits)
DLB2_VF_AVAIL_SYSFS_SHOW(num_dir_credits)
DLB2_VF_AVAIL_SYSFS_SHOW(num_atomic_inflights)
DLB2_VF_AVAIL_SYSFS_SHOW(num_hist_list_entries)

static ssize_t max_ctg_hl_entries_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);
	struct dlb2_get_num_resources_args num_avail_rsrcs;
	struct dlb2_hw *hw = &dlb2_dev->hw;
	int val;

	mutex_lock(&dlb2_dev->resource_mutex);

	val = dlb2_dev->ops->get_num_resources(hw, &num_avail_rsrcs);

	mutex_unlock(&dlb2_dev->resource_mutex);

	if (val)
		return -1;

	val = num_avail_rsrcs.max_contiguous_hist_list_entries;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

/* Device attribute name doesn't match the show function name, so we define our
 * own DEVICE_ATTR macro.
 */
#define DLB2_DEVICE_ATTR_RO(_prefix, _name) \
struct device_attribute dev_attr_##_prefix##_##_name = {\
	.attr = { .name = __stringify(_name), .mode = 0444 },\
	.show = _prefix##_##_name##_show,\
}

static DLB2_DEVICE_ATTR_RO(total, num_sched_domains);
static DLB2_DEVICE_ATTR_RO(total, num_ldb_queues);
static DLB2_DEVICE_ATTR_RO(total, num_ldb_ports);
static DLB2_DEVICE_ATTR_RO(total, num_cos0_ldb_ports);
static DLB2_DEVICE_ATTR_RO(total, num_cos1_ldb_ports);
static DLB2_DEVICE_ATTR_RO(total, num_cos2_ldb_ports);
static DLB2_DEVICE_ATTR_RO(total, num_cos3_ldb_ports);
static DLB2_DEVICE_ATTR_RO(total, num_dir_ports);
static DLB2_DEVICE_ATTR_RO(total, num_ldb_credits);
static DLB2_DEVICE_ATTR_RO(total, num_dir_credits);
static DLB2_DEVICE_ATTR_RO(total, num_atomic_inflights);
static DLB2_DEVICE_ATTR_RO(total, num_hist_list_entries);

static struct attribute *dlb2_total_attrs[] = {
	&dev_attr_total_num_sched_domains.attr,
	&dev_attr_total_num_ldb_queues.attr,
	&dev_attr_total_num_ldb_ports.attr,
	&dev_attr_total_num_cos0_ldb_ports.attr,
	&dev_attr_total_num_cos1_ldb_ports.attr,
	&dev_attr_total_num_cos2_ldb_ports.attr,
	&dev_attr_total_num_cos3_ldb_ports.attr,
	&dev_attr_total_num_dir_ports.attr,
	&dev_attr_total_num_ldb_credits.attr,
	&dev_attr_total_num_dir_credits.attr,
	&dev_attr_total_num_atomic_inflights.attr,
	&dev_attr_total_num_hist_list_entries.attr,
	NULL
};

static const struct attribute_group dlb2_vf_total_attr_group = {
	.attrs = dlb2_total_attrs,
	.name = "total_resources",
};

static DLB2_DEVICE_ATTR_RO(avail, num_sched_domains);
static DLB2_DEVICE_ATTR_RO(avail, num_ldb_queues);
static DLB2_DEVICE_ATTR_RO(avail, num_ldb_ports);
static DLB2_DEVICE_ATTR_RO(avail, num_cos0_ldb_ports);
static DLB2_DEVICE_ATTR_RO(avail, num_cos1_ldb_ports);
static DLB2_DEVICE_ATTR_RO(avail, num_cos2_ldb_ports);
static DLB2_DEVICE_ATTR_RO(avail, num_cos3_ldb_ports);
static DLB2_DEVICE_ATTR_RO(avail, num_dir_ports);
static DLB2_DEVICE_ATTR_RO(avail, num_ldb_credits);
static DLB2_DEVICE_ATTR_RO(avail, num_dir_credits);
static DLB2_DEVICE_ATTR_RO(avail, num_atomic_inflights);
static DLB2_DEVICE_ATTR_RO(avail, num_hist_list_entries);
static DEVICE_ATTR_RO(max_ctg_hl_entries);

static struct attribute *dlb2_avail_attrs[] = {
	&dev_attr_avail_num_sched_domains.attr,
	&dev_attr_avail_num_ldb_queues.attr,
	&dev_attr_avail_num_ldb_ports.attr,
	&dev_attr_avail_num_cos0_ldb_ports.attr,
	&dev_attr_avail_num_cos1_ldb_ports.attr,
	&dev_attr_avail_num_cos2_ldb_ports.attr,
	&dev_attr_avail_num_cos3_ldb_ports.attr,
	&dev_attr_avail_num_dir_ports.attr,
	&dev_attr_avail_num_ldb_credits.attr,
	&dev_attr_avail_num_dir_credits.attr,
	&dev_attr_avail_num_atomic_inflights.attr,
	&dev_attr_avail_num_hist_list_entries.attr,
	&dev_attr_max_ctg_hl_entries.attr,
	NULL
};

static const struct attribute_group dlb2_vf_avail_attr_group = {
	.attrs = dlb2_avail_attrs,
	.name = "avail_resources",
};

static ssize_t dev_id_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n", dlb2_dev->id);
}

static DEVICE_ATTR_RO(dev_id);

static int
dlb2_vf_sysfs_create(struct dlb2_dev *dlb2_dev)
{
	struct kobject *kobj;
	int ret;

	kobj = &dlb2_dev->pdev->dev.kobj;

	ret = sysfs_create_file(kobj, &dev_attr_dev_id.attr);
	if (ret)
		goto dlb2_dev_id_attr_group_fail;

	ret = sysfs_create_group(kobj, &dlb2_vf_total_attr_group);
	if (ret)
		goto dlb2_total_attr_group_fail;

	ret = sysfs_create_group(kobj, &dlb2_vf_avail_attr_group);
	if (ret)
		goto dlb2_avail_attr_group_fail;

	return 0;

dlb2_avail_attr_group_fail:
	sysfs_remove_group(kobj, &dlb2_vf_total_attr_group);
dlb2_total_attr_group_fail:
	sysfs_remove_file(kobj, &dev_attr_dev_id.attr);
dlb2_dev_id_attr_group_fail:
	return ret;
}

static void
dlb2_vf_sysfs_destroy(struct dlb2_dev *dlb2_dev)
{
	struct kobject *kobj;

	kobj = &dlb2_dev->pdev->dev.kobj;

	sysfs_remove_group(kobj, &dlb2_vf_avail_attr_group);
	sysfs_remove_group(kobj, &dlb2_vf_total_attr_group);
	sysfs_remove_file(kobj, &dev_attr_dev_id.attr);
}

static void
dlb2_vf_sysfs_reapply_configuration(struct dlb2_dev *dlb2_dev)
{
}

/**********************************/
/****** Interrupt management ******/
/**********************************/

static struct
dlb2_dev *dlb2_vf_get_primary(struct dlb2_dev *dlb2_dev)
{
	struct vf_id_state *vf_id_state;
	struct dlb2_dev *dev;
	bool found = false;

	if (!dlb2_dev->vf_id_state.is_auxiliary_vf)
		return dlb2_dev;

	mutex_lock(&dlb2_driver_lock);

	vf_id_state = &dlb2_dev->vf_id_state;

	list_for_each_entry(dev, &dlb2_dev_list, list) {
		if (dev->type == DLB2_VF &&
		    dev->vf_id_state.pf_id == vf_id_state->pf_id &&
		    dev->vf_id_state.vf_id == vf_id_state->primary_vf_id) {
			found = true;
			break;
		}
	}

	mutex_unlock(&dlb2_driver_lock);

	return (found) ? dev : NULL;
}

static int
dlb2_init_siov_vdev_interrupt_state(struct dlb2_dev *dlb2_dev)
{
	struct dlb2_get_num_resources_args num_rsrcs;

	dlb2_dev->ops->get_num_resources(&dlb2_dev->hw, &num_rsrcs);

	dlb2_dev->intr.num_ldb_ports = num_rsrcs.num_ldb_ports;
	dlb2_dev->intr.num_dir_ports = num_rsrcs.num_dir_ports;

	return 0;
}

static int
dlb2_init_auxiliary_vf_interrupts(struct dlb2_dev *dlb2_dev)
{
	struct dlb2_get_num_resources_args num_rsrcs;
	struct dlb2_dev *aux_vf;
	int ret;

	/* If the primary hasn't been probed yet, we can't init the auxiliary's
	 * interrupts.
	 */
	if (dlb2_dev->vf_id_state.is_auxiliary_vf &&
	    !dlb2_dev->vf_id_state.primary_vf)
		return 0;

	if (dlb2_dev->vf_id_state.is_auxiliary_vf)
		return dlb2_dev->ops->init_interrupts(dlb2_dev,
						      dlb2_dev->pdev);

	/* This is a primary VF, so iniitalize all of its auxiliary siblings
	 * that were already probed.
	 */
	dlb2_dev->ops->get_num_resources(&dlb2_dev->hw, &num_rsrcs);

	dlb2_dev->intr.num_ldb_ports = num_rsrcs.num_ldb_ports;
	dlb2_dev->intr.num_dir_ports = num_rsrcs.num_dir_ports;

	mutex_lock(&dlb2_driver_lock);

	list_for_each_entry(aux_vf, &dlb2_dev_list, list) {
		if (aux_vf->type != DLB2_VF)
			continue;

		if (!aux_vf->vf_id_state.is_auxiliary_vf)
			continue;

		if (aux_vf->vf_id_state.pf_id != dlb2_dev->vf_id_state.pf_id)
			continue;

		if (aux_vf->vf_id_state.primary_vf_id !=
		    dlb2_dev->vf_id_state.vf_id)
			continue;

		aux_vf->vf_id_state.primary_vf = dlb2_dev;

		ret = aux_vf->ops->init_interrupts(aux_vf, aux_vf->pdev);
		if (ret)
			goto interrupt_cleanup;
	}

	mutex_unlock(&dlb2_driver_lock);

	return 0;

interrupt_cleanup:

	list_for_each_entry(aux_vf, &dlb2_dev_list, list) {
		if (aux_vf->vf_id_state.primary_vf == dlb2_dev)
			aux_vf->ops->free_interrupts(aux_vf, aux_vf->pdev);
	}

	mutex_unlock(&dlb2_driver_lock);

	return ret;
}

static int
dlb2_vf_register_driver(struct dlb2_dev *dlb2_dev)
{
	struct dlb2_mbox_register_cmd_resp resp;
	struct dlb2_mbox_register_cmd_req req;
	int ret;

	/* Once the VF driver's BAR space is mapped in, it must inititate a
	 * handshake with the PF driver. The purpose is twofold:
	 * 1. Confirm that the drivers are using compatible mailbox interface
	 *	versions.
	 * 2. Alert the PF driver that the VF driver is in use. This causes the
	 *	PF driver to lock the VF's assigned resources, and causes the PF
	 *	driver to notify this driver whenever device-wide activities
	 *	occur (e.g. PF FLR).
	 */

	req.hdr.type = DLB2_MBOX_CMD_REGISTER;
	/* The VF driver only supports interface version 1 */
	req.min_interface_version = DLB2_MBOX_INTERFACE_VERSION;
	req.max_interface_version = DLB2_MBOX_INTERFACE_VERSION;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"VF driver registration failed with mailbox error: %s\n",
			DLB2_MBOX_ST_STRING(&resp));

		if (resp.hdr.status == DLB2_MBOX_ST_VERSION_MISMATCH) {
			dev_err(dlb2_dev->dlb2_device,
				"VF driver mailbox interface version: %d\n",
				DLB2_MBOX_INTERFACE_VERSION);
			dev_err(dlb2_dev->dlb2_device,
				"PF driver mailbox interface version: %d\n",
				resp.interface_version);
		}

		return -1;
	}

	dlb2_dev->vf_id_state.pf_id = resp.pf_id;
	dlb2_dev->vf_id_state.vf_id = resp.vf_id;
	dlb2_dev->vf_id_state.is_auxiliary_vf = resp.is_auxiliary_vf;
	dlb2_dev->vf_id_state.primary_vf_id = resp.primary_vf_id;

	/* Auxiliary VF interrupts are initialized in the register_driver
	 * callback and freed in the unregister_driver callback. There are
	 * two possible cases.
	 * 1. The auxiliary VF is probed after its primary: during the aux VF's
	 *    probe, it initializes its interrupts.
	 * 2. The auxiliary VF is probed before its primary: during the primary
	 *    VF's driver registration, it initializes the interrupts of all its
	 *    aux siblings that have already been probed.
	 */

	/* If the VF is not auxiliary, dlb2_vf_get_primary() returns
	 * dlb2_dev.
	 */
	dlb2_dev->vf_id_state.primary_vf = dlb2_vf_get_primary(dlb2_dev);

	/* If this is a primary VF, initialize the interrupts of any auxiliary
	 * VFs that were already probed. If this is an auxiliary VF and its
	 * primary has been probed, initialize the auxiliary's interrupts.
	 *
	 * If this is an S-IOV vdev, initialize the state needed to configure
	 * and service its CQ interrupts.
	 */
	if (dlb2_hw_get_virt_mode(&dlb2_dev->hw) == DLB2_VIRT_SRIOV)
		return dlb2_init_auxiliary_vf_interrupts(dlb2_dev);
	else
		return dlb2_init_siov_vdev_interrupt_state(dlb2_dev);
}

static void
dlb2_vf_unregister_driver(struct dlb2_dev *dlb2_dev)
{
	struct dlb2_mbox_unregister_cmd_resp resp;
	struct dlb2_mbox_unregister_cmd_req req;
	int ret;

	/* Aux VF interrupts are initialized in the register_driver callback
	 * and freed here.
	 */
	if (dlb2_dev->vf_id_state.is_auxiliary_vf &&
	    dlb2_dev->vf_id_state.primary_vf)
		dlb2_dev->ops->free_interrupts(dlb2_dev, dlb2_dev->pdev);

	req.hdr.type = DLB2_MBOX_CMD_UNREGISTER;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"VF driver registration failed with mailbox error: %s\n",
			DLB2_MBOX_ST_STRING(&resp));
	}
}

static void
dlb2_vf_enable_pm(struct dlb2_dev *dlb2_dev)
{
	/* Function intentionally left blank */
}

static int
dlb2_vf_wait_for_device_ready(struct dlb2_dev *dlb2_dev,
			      struct pci_dev *pdev)
{
	/* Device ready check only performed on the PF */
	return 0;
}

/*****************************/
/****** IOCTL callbacks ******/
/*****************************/

static int
dlb2_vf_create_sched_domain(struct dlb2_hw *hw,
			    struct dlb2_create_sched_domain_args *args,
			    struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_create_sched_domain_cmd_resp resp;
	struct dlb2_mbox_create_sched_domain_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_CREATE_SCHED_DOMAIN;
	req.num_ldb_queues = args->num_ldb_queues;
	req.num_ldb_ports = args->num_ldb_ports;
	req.num_cos0_ldb_ports = args->num_cos0_ldb_ports;
	req.num_cos1_ldb_ports = args->num_cos1_ldb_ports;
	req.num_cos2_ldb_ports = args->num_cos2_ldb_ports;
	req.num_cos3_ldb_ports = args->num_cos3_ldb_ports;
	req.num_dir_ports = args->num_dir_ports;
	req.num_atomic_inflights = args->num_atomic_inflights;
	req.num_hist_list_entries = args->num_hist_list_entries;
	req.num_ldb_credits = args->num_ldb_credits;
	req.num_dir_credits = args->num_dir_credits;
	req.cos_strict = args->cos_strict;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int
dlb2_vf_create_ldb_queue(struct dlb2_hw *hw,
			 u32 id,
			 struct dlb2_create_ldb_queue_args *args,
			 struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_create_ldb_queue_cmd_resp resp;
	struct dlb2_mbox_create_ldb_queue_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_CREATE_LDB_QUEUE;
	req.domain_id = id;
	req.num_sequence_numbers = args->num_sequence_numbers;
	req.num_qid_inflights = args->num_qid_inflights;
	req.num_atomic_inflights = args->num_atomic_inflights;
	req.lock_id_comp_level = args->lock_id_comp_level;
	req.depth_threshold = args->depth_threshold;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int
dlb2_vf_create_dir_queue(struct dlb2_hw *hw,
			 u32 id,
			 struct dlb2_create_dir_queue_args *args,
			 struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_create_dir_queue_cmd_resp resp;
	struct dlb2_mbox_create_dir_queue_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_CREATE_DIR_QUEUE;
	req.domain_id = id;
	req.port_id = args->port_id;
	req.depth_threshold = args->depth_threshold;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int
dlb2_vf_create_ldb_port(struct dlb2_hw *hw,
			u32 id,
			struct dlb2_create_ldb_port_args *args,
			uintptr_t cq_dma_base,
			struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_create_ldb_port_cmd_resp resp;
	struct dlb2_mbox_create_ldb_port_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_CREATE_LDB_PORT;
	req.domain_id = id;
	req.cq_depth = args->cq_depth;
	req.cq_history_list_size = args->cq_history_list_size;
	req.cos_id = args->cos_id;
	req.cos_strict = args->cos_strict;
	req.cq_base_address = cq_dma_base;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int
dlb2_vf_create_dir_port(struct dlb2_hw *hw,
			u32 id,
			struct dlb2_create_dir_port_args *args,
			uintptr_t cq_dma_base,
			struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_create_dir_port_cmd_resp resp;
	struct dlb2_mbox_create_dir_port_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_CREATE_DIR_PORT;
	req.domain_id = id;
	req.cq_depth = args->cq_depth;
	req.cq_base_address = cq_dma_base;
	req.queue_id = args->queue_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int
dlb2_vf_start_domain(struct dlb2_hw *hw,
		     u32 id,
		     struct dlb2_start_domain_args *args,
		     struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_start_domain_cmd_resp resp;
	struct dlb2_mbox_start_domain_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_START_DOMAIN;
	req.domain_id = id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int
dlb2_vf_map_qid(struct dlb2_hw *hw,
		u32 id,
		struct dlb2_map_qid_args *args,
		struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_map_qid_cmd_resp resp;
	struct dlb2_mbox_map_qid_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_MAP_QID;
	req.domain_id = id;
	req.port_id = args->port_id;
	req.qid = args->qid;
	req.priority = args->priority;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int
dlb2_vf_unmap_qid(struct dlb2_hw *hw,
		  u32 id,
		  struct dlb2_unmap_qid_args *args,
		  struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_unmap_qid_cmd_resp resp;
	struct dlb2_mbox_unmap_qid_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_UNMAP_QID;
	req.domain_id = id;
	req.port_id = args->port_id;
	req.qid = args->qid;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int
dlb2_vf_enable_ldb_port(struct dlb2_hw *hw,
			u32 id,
			struct dlb2_enable_ldb_port_args *args,
			struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_enable_ldb_port_cmd_resp resp;
	struct dlb2_mbox_enable_ldb_port_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_ENABLE_LDB_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int
dlb2_vf_disable_ldb_port(struct dlb2_hw *hw,
			 u32 id,
			 struct dlb2_disable_ldb_port_args *args,
			 struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_disable_ldb_port_cmd_resp resp;
	struct dlb2_mbox_disable_ldb_port_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_DISABLE_LDB_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int
dlb2_vf_enable_dir_port(struct dlb2_hw *hw,
			u32 id,
			struct dlb2_enable_dir_port_args *args,
			struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_enable_dir_port_cmd_resp resp;
	struct dlb2_mbox_enable_dir_port_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_ENABLE_DIR_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int
dlb2_vf_disable_dir_port(struct dlb2_hw *hw,
			 u32 id,
			 struct dlb2_disable_dir_port_args *args,
			 struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_disable_dir_port_cmd_resp resp;
	struct dlb2_mbox_disable_dir_port_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_DISABLE_DIR_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int
dlb2_vf_get_num_resources(struct dlb2_hw *hw,
			  struct dlb2_get_num_resources_args *args)
{
	struct dlb2_mbox_get_num_resources_cmd_resp resp;
	struct dlb2_mbox_get_num_resources_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_GET_NUM_RESOURCES;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		return -1;
	}

	args->num_sched_domains = resp.num_sched_domains;
	args->num_ldb_queues = resp.num_ldb_queues;
	args->num_ldb_ports = resp.num_ldb_ports;
	args->num_cos0_ldb_ports = resp.num_cos0_ldb_ports;
	args->num_cos1_ldb_ports = resp.num_cos1_ldb_ports;
	args->num_cos2_ldb_ports = resp.num_cos2_ldb_ports;
	args->num_cos3_ldb_ports = resp.num_cos3_ldb_ports;
	args->num_dir_ports = resp.num_dir_ports;
	args->num_atomic_inflights = resp.num_atomic_inflights;
	args->num_hist_list_entries = resp.num_hist_list_entries;
	args->max_contiguous_hist_list_entries =
		resp.max_contiguous_hist_list_entries;
	args->num_ldb_credits = resp.num_ldb_credits;
	args->num_dir_credits = resp.num_dir_credits;

	return resp.error_code;
}

static int
dlb2_vf_reset_domain(struct dlb2_hw *hw, u32 id)
{
	struct dlb2_mbox_reset_sched_domain_cmd_resp resp;
	struct dlb2_mbox_reset_sched_domain_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_RESET_SCHED_DOMAIN;
	req.id = id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.error_code;
}

static int
dlb2_vf_write_smon(struct dlb2_dev *dev,
		   struct dlb2_write_smon_args *args,
		   struct dlb2_cmd_response *response)
{
	pr_err("Virtual devices are not allowed to access SMONs\n");
	return -EINVAL;
}

static int
dlb2_vf_read_smon(struct dlb2_dev *dev,
		  struct dlb2_read_smon_args *args,
		  struct dlb2_cmd_response *response)
{
	pr_err("Virtual devices are not allowed to access SMONs\n");
	return -EINVAL;
}

static int
dlb2_vf_get_ldb_queue_depth(struct dlb2_hw *hw,
			    u32 id,
			    struct dlb2_get_ldb_queue_depth_args *args,
			    struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_get_ldb_queue_depth_cmd_resp resp;
	struct dlb2_mbox_get_ldb_queue_depth_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_GET_LDB_QUEUE_DEPTH;
	req.domain_id = id;
	req.queue_id = args->queue_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.depth;

	return resp.error_code;
}

static int
dlb2_vf_get_dir_queue_depth(struct dlb2_hw *hw,
			    u32 id,
			    struct dlb2_get_dir_queue_depth_args *args,
			    struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_get_dir_queue_depth_cmd_resp resp;
	struct dlb2_mbox_get_dir_queue_depth_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_GET_DIR_QUEUE_DEPTH;
	req.domain_id = id;
	req.queue_id = args->queue_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.depth;

	return resp.error_code;
}

static int
dlb2_vf_pending_port_unmaps(struct dlb2_hw *hw,
			    u32 id,
			    struct dlb2_pending_port_unmaps_args *args,
			    struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_pending_port_unmaps_cmd_resp resp;
	struct dlb2_mbox_pending_port_unmaps_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_PENDING_PORT_UNMAPS;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.num;

	return resp.error_code;
}

static int
dlb2_vf_query_cq_poll_mode(struct dlb2_dev *dlb2_dev,
			   struct dlb2_cmd_response *user_resp)
{
	struct dlb2_mbox_query_cq_poll_mode_cmd_resp resp;
	struct dlb2_mbox_query_cq_poll_mode_cmd_req req;
	int ret;

	req.hdr.type = DLB2_MBOX_CMD_QUERY_CQ_POLL_MODE;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		user_resp->status = DLB2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.mode;

	return resp.error_code;
}

/**************************************/
/****** Resource query callbacks ******/
/**************************************/

static int
dlb2_vf_ldb_port_owned_by_domain(struct dlb2_hw *hw,
				 u32 domain_id,
				 u32 port_id)
{
	struct dlb2_mbox_ldb_port_owned_by_domain_cmd_resp resp;
	struct dlb2_mbox_ldb_port_owned_by_domain_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_LDB_PORT_OWNED_BY_DOMAIN;
	req.domain_id = domain_id;
	req.port_id = port_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.owned;
}

static int
dlb2_vf_dir_port_owned_by_domain(struct dlb2_hw *hw,
				 u32 domain_id,
				 u32 port_id)
{
	struct dlb2_mbox_dir_port_owned_by_domain_cmd_resp resp;
	struct dlb2_mbox_dir_port_owned_by_domain_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_DIR_PORT_OWNED_BY_DOMAIN;
	req.domain_id = domain_id;
	req.port_id = port_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.owned;
}

static int
dlb2_vf_get_sn_allocation(struct dlb2_hw *hw, u32 group_id)
{
	struct dlb2_mbox_get_sn_allocation_cmd_resp resp;
	struct dlb2_mbox_get_sn_allocation_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_GET_SN_ALLOCATION;
	req.group_id = group_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.num;
}

static int
dlb2_vf_get_cos_bw(struct dlb2_hw *hw, u32 cos_id)
{
	struct dlb2_mbox_get_cos_bw_cmd_resp resp;
	struct dlb2_mbox_get_cos_bw_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_GET_COS_BW;
	req.cos_id = cos_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.num;
}

static int
dlb2_vf_get_sn_occupancy(struct dlb2_hw *hw, u32 group_id)
{
	struct dlb2_mbox_get_sn_occupancy_cmd_resp resp;
	struct dlb2_mbox_get_sn_occupancy_cmd_req req;
	struct dlb2_dev *dlb2_dev;
	int ret;

	dlb2_dev = container_of(hw, struct dlb2_dev, hw);

	req.hdr.type = DLB2_MBOX_CMD_GET_SN_OCCUPANCY;
	req.group_id = group_id;

	ret = dlb2_send_sync_mbox_cmd(dlb2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	dlb2_vf_read_pf_mbox_resp(&dlb2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB2_MBOX_ST_SUCCESS) {
		dev_err(dlb2_dev->dlb2_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.num;
}

/*********************************/
/****** DLB2 VF Device Ops ******/
/*********************************/

struct dlb2_device_ops dlb2_vf_ops = {
	.map_pci_bar_space = dlb2_vf_map_pci_bar_space,
	.unmap_pci_bar_space = dlb2_vf_unmap_pci_bar_space,
	.mmap = dlb2_vf_mmap,
	.inc_pm_refcnt = dlb2_vf_pm_inc_refcnt,
	.dec_pm_refcnt = dlb2_vf_pm_dec_refcnt,
	.pm_refcnt_status_suspended = dlb2_vf_pm_status_suspended,
	.init_driver_state = dlb2_vf_init_driver_state,
	.free_driver_state = dlb2_vf_free_driver_state,
	.device_create = dlb2_vf_device_create,
	.device_destroy = dlb2_vf_device_destroy,
	.cdev_add = dlb2_vf_cdev_add,
	.cdev_del = dlb2_vf_cdev_del,
	.sysfs_create = dlb2_vf_sysfs_create,
	.sysfs_destroy = dlb2_vf_sysfs_destroy,
	.sysfs_reapply = dlb2_vf_sysfs_reapply_configuration,
	.init_interrupts = dlb2_vf_init_interrupts,
	.enable_ldb_cq_interrupts = dlb2_vf_enable_ldb_cq_interrupts,
	.enable_dir_cq_interrupts = dlb2_vf_enable_dir_cq_interrupts,
	.arm_cq_interrupt = dlb2_vf_arm_cq_interrupt,
	.reinit_interrupts = dlb2_vf_reinit_interrupts,
	.free_interrupts = dlb2_vf_free_interrupts,
	.enable_pm = dlb2_vf_enable_pm,
	.wait_for_device_ready = dlb2_vf_wait_for_device_ready,
	.register_driver = dlb2_vf_register_driver,
	.unregister_driver = dlb2_vf_unregister_driver,
	.create_sched_domain = dlb2_vf_create_sched_domain,
	.create_ldb_queue = dlb2_vf_create_ldb_queue,
	.create_dir_queue = dlb2_vf_create_dir_queue,
	.create_ldb_port = dlb2_vf_create_ldb_port,
	.create_dir_port = dlb2_vf_create_dir_port,
	.start_domain = dlb2_vf_start_domain,
	.map_qid = dlb2_vf_map_qid,
	.unmap_qid = dlb2_vf_unmap_qid,
	.enable_ldb_port = dlb2_vf_enable_ldb_port,
	.enable_dir_port = dlb2_vf_enable_dir_port,
	.disable_ldb_port = dlb2_vf_disable_ldb_port,
	.disable_dir_port = dlb2_vf_disable_dir_port,
	.get_num_resources = dlb2_vf_get_num_resources,
	.reset_domain = dlb2_vf_reset_domain,
	.write_smon = dlb2_vf_write_smon,
	.read_smon = dlb2_vf_read_smon,
	.ldb_port_owned_by_domain = dlb2_vf_ldb_port_owned_by_domain,
	.dir_port_owned_by_domain = dlb2_vf_dir_port_owned_by_domain,
	.get_sn_allocation = dlb2_vf_get_sn_allocation,
	.get_sn_occupancy = dlb2_vf_get_sn_occupancy,
	.get_ldb_queue_depth = dlb2_vf_get_ldb_queue_depth,
	.get_dir_queue_depth = dlb2_vf_get_dir_queue_depth,
	.pending_port_unmaps = dlb2_vf_pending_port_unmaps,
	.get_cos_bw = dlb2_vf_get_cos_bw,
	.init_hardware = dlb2_vf_init_hardware,
	.query_cq_poll_mode = dlb2_vf_query_cq_poll_mode,
};
