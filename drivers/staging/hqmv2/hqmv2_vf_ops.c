// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2018 Intel Corporation */

#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>

#include "base/hqmv2_resource.h"
#include "base/hqmv2_mbox.h"
#include "base/hqmv2_osdep.h"
#include "hqmv2_intr.h"
#include "hqmv2_dp_ops.h"

int hqmv2_send_sync_mbox_cmd(struct hqmv2_dev *dev,
			     void *data,
			     int len,
			     int tmo);

/***********************************/
/****** Runtime PM management ******/
/***********************************/

void hqmv2_vf_pm_inc_refcnt(struct pci_dev *pdev, bool resume)
{
}

void hqmv2_vf_pm_dec_refcnt(struct pci_dev *pdev)
{
}

bool hqmv2_vf_pm_status_suspended(struct pci_dev *pdev)
{
	return false;
}

/**************************/
/****** Device reset ******/
/**************************/

int hqmv2_vf_reset(struct pci_dev *pdev)
{
	int ret;

	/* TODO: When upstreaming this, replace this function with
	 * pci_reset_function_locked() (available in newer kernels).
	 */

	ret = pci_save_state(pdev);
	if (ret)
		return ret;

	ret = __pci_reset_function_locked(pdev);
	if (ret)
		return ret;

	pci_restore_state(pdev);

	return 0;
}

/********************************/
/****** PCI BAR management ******/
/********************************/

void hqmv2_vf_unmap_pci_bar_space(struct hqmv2_dev *hqmv2_dev,
				  struct pci_dev *pdev)
{
	pci_iounmap(pdev, hqmv2_dev->hw.func_kva);
}

int hqmv2_vf_map_pci_bar_space(struct hqmv2_dev *hqmv2_dev,
			       struct pci_dev *pdev)
{
	enum hqmv2_virt_mode mode;

	/* BAR 0: VF FUNC BAR space */
	hqmv2_dev->hw.func_kva = pci_iomap(pdev, 0, 0);
	hqmv2_dev->hw.func_phys_addr = pci_resource_start(pdev, 0);

	if (!hqmv2_dev->hw.func_kva) {
		HQMV2_ERR(&pdev->dev, "Cannot iomap BAR 0 (size %llu)\n",
			  pci_resource_len(pdev, 0));

		return -EIO;
	}

	HQMV2_INFO(&pdev->dev, "BAR 0 iomem base: %p\n",
		   hqmv2_dev->hw.func_kva);
	HQMV2_INFO(&pdev->dev, "BAR 0 start: 0x%llx\n",
		   pci_resource_start(pdev, 0));
	HQMV2_INFO(&pdev->dev, "BAR 0 len: %llu\n",
		   pci_resource_len(pdev, 0));

	/* Before the driver can use its mailbox, it needs to identify whether
	 * its device is a VF (SR-IOV) or VDEV (S-IOV), because the mailbox
	 * interface differs slightly among the two. Detect by looking for an
	 * MSI-X capability (S-IOV only).
	 */
	mode = (pdev->msix_cap) ? HQMV2_VIRT_SIOV : HQMV2_VIRT_SRIOV;

	hqmv2_hw_set_virt_mode(&hqmv2_dev->hw, mode);

	return 0;
}

#define HQMV2_LDB_CQ_BOUND (HQMV2_LDB_CQ_BASE + PAGE_SIZE * \
			    HQMV2_MAX_NUM_LDB_PORTS)
#define HQMV2_DIR_CQ_BOUND (HQMV2_DIR_CQ_BASE + PAGE_SIZE * \
			    HQMV2_MAX_NUM_DIR_PORTS)
#define HQMV2_LDB_PC_BOUND (HQMV2_LDB_PC_BASE + PAGE_SIZE * \
			    HQMV2_MAX_NUM_LDB_PORTS)
#define HQMV2_DIR_PC_BOUND (HQMV2_DIR_PC_BASE + PAGE_SIZE * \
			    HQMV2_MAX_NUM_DIR_PORTS)

int hqmv2_vf_mmap(struct file *f, struct vm_area_struct *vma, u32 domain_id)
{
	struct hqmv2_dev *dev;
	unsigned long offset;
	unsigned long pgoff;
	u32 port_id;
	pgprot_t pgprot;

	dev = container_of(f->f_inode->i_cdev, struct hqmv2_dev, cdev);

	offset = vma->vm_pgoff << PAGE_SHIFT;

	/* The VF's ops callbacks go through the mailbox, so synchronization is
	 * required.
	 */
	mutex_lock(&dev->resource_mutex);

	if (offset >= HQMV2_LDB_CQ_BASE && offset < HQMV2_LDB_CQ_BOUND) {
		struct page *page;

		pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - HQMV2_LDB_CQ_BASE) / PAGE_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1) {
			mutex_unlock(&dev->resource_mutex);
			return -EINVAL;
		}

		page = virt_to_page(dev->ldb_port_pages[port_id].cq_base);

		mutex_unlock(&dev->resource_mutex);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= HQMV2_DIR_CQ_BASE && offset < HQMV2_DIR_CQ_BOUND) {
		struct page *page;

		pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - HQMV2_DIR_CQ_BASE) / PAGE_SIZE;

		if (dev->ops->dir_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1) {
			mutex_unlock(&dev->resource_mutex);
			return -EINVAL;
		}

		mutex_unlock(&dev->resource_mutex);

		page = virt_to_page(dev->dir_port_pages[port_id].cq_base);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= HQMV2_LDB_PC_BASE && offset < HQMV2_LDB_PC_BOUND) {
		struct page *page;

		pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - HQMV2_LDB_PC_BASE) / PAGE_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1) {
			mutex_unlock(&dev->resource_mutex);
			return -EINVAL;
		}

		mutex_unlock(&dev->resource_mutex);

		page = virt_to_page(dev->ldb_port_pages[port_id].pc_base);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= HQMV2_DIR_PC_BASE && offset < HQMV2_DIR_PC_BOUND) {
		struct page *page;

		pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - HQMV2_DIR_PC_BASE) / PAGE_SIZE;

		if (dev->ops->dir_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1) {
			mutex_unlock(&dev->resource_mutex);
			return -EINVAL;
		}

		mutex_unlock(&dev->resource_mutex);

		page = virt_to_page(dev->dir_port_pages[port_id].pc_base);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= HQMV2_LDB_PP_BASE && offset < HQMV2_LDB_PP_BOUND) {
		pgoff = (dev->hw.func_phys_addr >> PAGE_SHIFT) + vma->vm_pgoff;

		port_id = (offset - HQMV2_LDB_PP_BASE) / PAGE_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1) {
			mutex_unlock(&dev->resource_mutex);
			return -EINVAL;
		}

		pgprot = pgprot_noncached(vma->vm_page_prot);

	} else if (offset >= HQMV2_DIR_PP_BASE && offset < HQMV2_DIR_PP_BOUND) {
		pgoff = (dev->hw.func_phys_addr >> PAGE_SHIFT) + vma->vm_pgoff;

		port_id = (offset - HQMV2_DIR_PP_BASE) / PAGE_SIZE;

		if (dev->ops->dir_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1) {
			mutex_unlock(&dev->resource_mutex);
			return -EINVAL;
		}

		pgprot = pgprot_noncached(vma->vm_page_prot);

	} else {
		mutex_unlock(&dev->resource_mutex);
		return -EINVAL;
	}

	mutex_unlock(&dev->resource_mutex);

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
static void hqmv2_vf_claim_cq_interrupts(struct hqmv2_dev *hqmv2_dev)
{
	struct hqmv2_dev *primary_vf;
	int i, cnt;

	hqmv2_dev->intr.num_cq_intrs = 0;
	primary_vf = hqmv2_dev->vf_id_state.primary_vf;

	if (!primary_vf)
		return;

	cnt = 0;

	for (i = 0; i < primary_vf->intr.num_ldb_ports; i++) {
		if (primary_vf->intr.ldb_cq_intr_owner[i])
			continue;

		primary_vf->intr.ldb_cq_intr_owner[i] = hqmv2_dev;

		hqmv2_dev->intr.msi_map[cnt].port_id = i;
		hqmv2_dev->intr.msi_map[cnt].is_ldb = true;
		cnt++;

		hqmv2_dev->intr.num_cq_intrs++;

		if (hqmv2_dev->intr.num_cq_intrs ==
		    HQMV2_VF_NUM_CQ_INTERRUPT_VECTORS)
			return;
	}

	for (i = 0; i <  primary_vf->intr.num_dir_ports; i++) {
		if (primary_vf->intr.dir_cq_intr_owner[i])
			continue;

		primary_vf->intr.dir_cq_intr_owner[i] = hqmv2_dev;

		hqmv2_dev->intr.msi_map[cnt].port_id = i;
		hqmv2_dev->intr.msi_map[cnt].is_ldb = false;
		cnt++;

		hqmv2_dev->intr.num_cq_intrs++;

		if (hqmv2_dev->intr.num_cq_intrs ==
		    HQMV2_VF_NUM_CQ_INTERRUPT_VECTORS)
			return;
	}
}

static void hqmv2_vf_unclaim_cq_interrupts(struct hqmv2_dev *hqmv2_dev)
{
	struct hqmv2_dev *primary_vf;
	int i;

	primary_vf = hqmv2_dev->vf_id_state.primary_vf;

	if (!primary_vf)
		return;

	for (i = 0; i < HQMV2_MAX_NUM_LDB_PORTS; i++) {
		if (primary_vf->intr.ldb_cq_intr_owner[i] != hqmv2_dev)
			continue;

		primary_vf->intr.ldb_cq_intr_owner[i] = NULL;
	}

	for (i = 0; i < HQMV2_MAX_NUM_DIR_PORTS; i++) {
		if (primary_vf->intr.dir_cq_intr_owner[i] != hqmv2_dev)
			continue;

		primary_vf->intr.dir_cq_intr_owner[i] = NULL;
	}
}

static void hqmv2_vf_mbox_cmd_alarm_fn(struct hqmv2_dev *dev, void *data)
{
	struct hqmv2_mbox_vf_alert_cmd_req *req = data;

	os_notify_user_space(&dev->hw,
			     req->domain_id,
			     req->alert_id,
			     req->aux_alert_data);

	/* No response needed beyond ACKing the interrupt. */
}

static void hqmv2_vf_mbox_cmd_notification_fn(struct hqmv2_dev *dev, void *data)
{
	struct hqmv2_mbox_vf_notification_cmd_req *req = data;

	/* If the VF is auxiliary, it has no resources affected by PF reset */
	if (dev->vf_id_state.is_auxiliary_vf)
		return;

	/* When the PF is reset, it notifies every registered VF driver twice:
	 * once before the reset, and once after.
	 *
	 * The pre-reset notification gives the VF an opportunity to notify its
	 * users to shutdown. The PF will not proceed with the reset until
	 * either all VF-owned domains are reset (and all the PF's users
	 * quiesce), or the PF waits for its reset timeout.
	 *
	 * The post-reset notification gives the VF driver a chance to zap
	 * any uncooperative/non-responsive users before allowing the VF to be
	 * used again.
	 */
	if (req->notification == HQMV2_MBOX_VF_NOTIFICATION_PRE_RESET) {
		int i;

		/* Before the reset occurs, wake up all active users and give
		 * them a chance to stop using the device. The PF won't wait
		 * indefinitely, however.
		 */
		mutex_lock(&dev->resource_mutex);

		WRITE_ONCE(dev->unexpected_reset, true);

		/* Wake any blocked device file readers. These threads will
		 * return the HQMV2_DOMAIN_ALERT_DEVICE_RESET alert, and
		 * well-behaved applications will close their fds and unmap HQM
		 * memory as a result.
		 */
		for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++)
			wake_up_interruptible(&dev->sched_domains[i].wq_head);

		/* Wake threads blocked on a CQ interrupt */
		for (i = 0; i < HQMV2_MAX_NUM_LDB_PORTS; i++)
			hqmv2_wake_thread(dev,
					  &dev->intr.ldb_cq_intr[i],
					  WAKE_DEV_RESET);

		for (i = 0; i < HQMV2_MAX_NUM_DIR_PORTS; i++)
			hqmv2_wake_thread(dev,
					  &dev->intr.dir_cq_intr[i],
					  WAKE_DEV_RESET);

		mutex_unlock(&dev->resource_mutex);

	} else if (req->notification == HQMV2_MBOX_VF_NOTIFICATION_POST_RESET) {
		mutex_lock(&dev->resource_mutex);

		/* Shutdown any extant user threads before clearing
		 * unexpected_reset, which allows ioctls to succeed. We can't
		 * allow rogue software to continue to use the device when
		 * another process configures and begins to use the device.
		 */
		hqmv2_shutdown_user_threads(dev);

		/* hqmv2_shutdown_user_threads() causes all remaining user
		 * threads to close their device files. These files must be
		 * closed before the handler clears unexpected_reset, or else
		 * these processes will attempt to reset the domain (see
		 * hqmv2_close_domain_device_file() for more details). Rather
		 * than spinning in this handler waiting for user processes to
		 * idle, we set a flag (vf_waiting_for_idle) that causes the
		 * file closers to check if the hqmv2 is in use, and if not
		 * then clear unexpected_reset.
		 */
		dev->vf_waiting_for_idle = hqmv2_in_use(dev);
		if (!dev->vf_waiting_for_idle)
			WRITE_ONCE(dev->unexpected_reset, false);

		mutex_unlock(&dev->resource_mutex);
	}

	/* No response needed beyond ACKing the interrupt. */
}

static void hqmv2_vf_mbox_cmd_in_use_fn(struct hqmv2_dev *dev, void *data)
{
	struct hqmv2_mbox_vf_in_use_cmd_resp resp;

	/* If the VF is auxiliary, the PF shouldn't send it an in-use request */
	if (dev->vf_id_state.is_auxiliary_vf) {
		HQMV2_ERR(dev->hqmv2_device,
			  "Internal error: VF in-use request sent to auxiliary vf %d\n",
			  dev->vf_id_state.vf_id);
		return;
	}

	resp.in_use = hqmv2_in_use(dev);
	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;

	hqmv2_vf_write_pf_mbox_resp(&dev->hw, &resp, sizeof(resp));
}

static void (*vf_mbox_fn_table[])(struct hqmv2_dev *dev, void *data) = {
	hqmv2_vf_mbox_cmd_alarm_fn,
	hqmv2_vf_mbox_cmd_notification_fn,
	hqmv2_vf_mbox_cmd_in_use_fn,
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
	[HQMV2_MBOX_VF_CMD_DOMAIN_ALERT] = true,
	[HQMV2_MBOX_VF_CMD_NOTIFICATION] = true,
	[HQMV2_MBOX_VF_CMD_IN_USE] = false,
};

static void hqmv2_vf_handle_pf_req(struct hqmv2_dev *dev)
{
	u8 data[HQMV2_PF2VF_REQ_BYTES];
	bool deferred;

	hqmv2_vf_read_pf_mbox_req(&dev->hw, &data, sizeof(data));

	deferred = deferred_mbox_hdlrs[HQMV2_MBOX_CMD_TYPE(data)];

	if (!deferred)
		vf_mbox_fn_table[HQMV2_MBOX_CMD_TYPE(data)](dev, data);

	hqmv2_ack_pf_mbox_int(&dev->hw);

	if (deferred)
		vf_mbox_fn_table[HQMV2_MBOX_CMD_TYPE(data)](dev, data);
}

static irqreturn_t hqmv2_vf_intr_handler(int irq, void *hdlr_ptr)
{
	struct hqmv2_dev *dev = hdlr_ptr;
	struct hqmv2_dev *primary_vf;
	u32 interrupts, mask;
	int vector;
	int i;

	HQMV2_INFO(dev->hqmv2_device, "Entered ISR\n");

	primary_vf = dev->vf_id_state.primary_vf;

	vector = irq - dev->intr.base_vector;

	mask = dev->intr.num_vectors - 1;

	interrupts = hqmv2_read_vf_intr_status(&dev->hw);

	hqmv2_ack_vf_intr_status(&dev->hw, interrupts);

	for (i = 0; i < HQMV2_VF_TOTAL_NUM_INTERRUPT_VECTORS; i++) {
		struct hqmv2_cq_intr *intr;
		int port_id;

		if ((i & mask) != vector || !(interrupts & (1 << i)))
			continue;

		HQMV2_INFO(dev->hqmv2_device,
			   "[%s()] VF intr vector %d fired\n",
			   __func__, i);

		if (i == HQMV2_VF_MBOX_VECTOR_ID) {
			hqmv2_vf_handle_pf_req(dev);
			continue;
		}

		port_id = dev->intr.msi_map[i].port_id;

		if (dev->intr.msi_map[i].is_ldb)
			intr = &primary_vf->intr.ldb_cq_intr[port_id];
		else
			intr = &primary_vf->intr.dir_cq_intr[port_id];

		hqmv2_wake_thread(dev, intr, WAKE_CQ_INTR);
	}

	hqmv2_ack_vf_msi_intr(&dev->hw, 1 << vector);

	return IRQ_HANDLED;
}

static irqreturn_t hqmv2_vdev_intr_handler(int irq, void *hdlr_ptr)
{
	struct hqmv2_dev *dev = hdlr_ptr;
	int vector;

	vector = irq - dev->intr.base_vector;

	HQMV2_INFO(dev->hqmv2_device,
		   "[%s()] Intr vector %d fired\n",
		   __func__, vector);

	if (vector == HQMV2_INT_NON_CQ) {
		hqmv2_vf_handle_pf_req(dev);
	} else {
		struct hqmv2_cq_intr *intr;
		int port_id, idx;

		idx = vector - 1;
		port_id = dev->intr.msi_map[idx].port_id;

		if (dev->intr.msi_map[idx].is_ldb)
			intr = &dev->intr.ldb_cq_intr[port_id];
		else
			intr = &dev->intr.dir_cq_intr[port_id];

		hqmv2_wake_thread(dev, intr, WAKE_CQ_INTR);
	}

	return IRQ_HANDLED;
}

static void hqmv2_vf_get_cq_interrupt_name(struct hqmv2_dev *dev, int vector)
{
	int port_id;
	bool is_ldb;

	port_id = dev->intr.msi_map[vector].port_id;
	is_ldb = dev->intr.msi_map[vector].is_ldb;

	snprintf(dev->intr.msi_map[vector].name,
		 sizeof(dev->intr.msi_map[vector].name) - 1,
		 "hqmv2_%s_cq_%d", is_ldb ? "ldb" : "dir", port_id);
}

static int hqmv2_vf_init_interrupt_handlers(struct hqmv2_dev *dev,
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

		hqmv2_vf_get_cq_interrupt_name(dev, i);

		ret = devm_request_threaded_irq(&pdev->dev,
						pci_irq_vector(pdev, i),
						NULL,
						hqmv2_vf_intr_handler,
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
					hqmv2_vf_intr_handler,
					IRQF_ONESHOT,
					"hqmv2_pf_to_vf_mbox",
					dev);
	if (ret)
		return ret;

	dev->intr.isr_registered[i] = true;

	return 0;
}

static int hqmv2_vdev_init_interrupt_handlers(struct hqmv2_dev *dev,
					      struct pci_dev *pdev)
{
	int i, ret;

	/* Request the mailbox interrupt vector */
	i = HQMV2_INT_NON_CQ;

	ret = devm_request_threaded_irq(&pdev->dev,
					pci_irq_vector(pdev, i),
					NULL,
					hqmv2_vdev_intr_handler,
					IRQF_ONESHOT,
					"hqmv2_pf_to_vf_mbox",
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

		hqmv2_vf_get_cq_interrupt_name(dev, cq_idx);

		ret = devm_request_threaded_irq(&pdev->dev,
						pci_irq_vector(pdev, i),
						NULL,
						hqmv2_vdev_intr_handler,
						IRQF_ONESHOT,
						name,
						dev);
		if (ret)
			return ret;

		dev->intr.isr_registered[i] = true;
	}

	return 0;
}

int hqmv2_vf_init_sriov_interrupts(struct hqmv2_dev *dev, struct pci_dev *pdev)
{
	int ret, req_size;
	int num_cq_intrs;

	/* Claim a batch of CQs from the primary VF for assigning to MSI
	 * vectors (if the primary VF has been probed).
	 */
	hqmv2_vf_claim_cq_interrupts(dev);

	/* Request IRQ vectors. The request size depends on the number of CQs
	 * this VF claimed -- it will attempt to take enough for a 1:1 mapping,
	 * else it falls back to a single vector.
	 */
	num_cq_intrs = dev->intr.num_cq_intrs;

	if ((num_cq_intrs + HQMV2_VF_NUM_NON_CQ_INTERRUPT_VECTORS) > 16)
		req_size = 32;
	else if ((num_cq_intrs + HQMV2_VF_NUM_NON_CQ_INTERRUPT_VECTORS) > 8)
		req_size = 16;
	else if ((num_cq_intrs + HQMV2_VF_NUM_NON_CQ_INTERRUPT_VECTORS) > 4)
		req_size = 8;
	else if ((num_cq_intrs + HQMV2_VF_NUM_NON_CQ_INTERRUPT_VECTORS) > 2)
		req_size = 4;
	else if ((num_cq_intrs + HQMV2_VF_NUM_NON_CQ_INTERRUPT_VECTORS) > 1)
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

int hqmv2_vf_init_siov_interrupts(struct hqmv2_dev *dev, struct pci_dev *pdev)
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
		HQMV2_ERR(dev->hqmv2_device,
			  "Error: unable to allocate %d MSI-X vectors.\n",
			  nvec);
		return ret;
	}

	dev->intr.num_vectors = ret;
	dev->intr.base_vector = pci_irq_vector(pdev, 0);

	dev->intr.num_cq_intrs = ret - 1;

	return 0;
}

int hqmv2_vf_init_interrupts(struct hqmv2_dev *dev, struct pci_dev *pdev)
{
	int ret, i;

	if (hqmv2_hw_get_virt_mode(&dev->hw) == HQMV2_VIRT_SRIOV)
		ret = hqmv2_vf_init_sriov_interrupts(dev, pdev);
	else
		ret = hqmv2_vf_init_siov_interrupts(dev, pdev);

	if (ret)
		return ret;

	if (hqmv2_hw_get_virt_mode(&dev->hw) == HQMV2_VIRT_SRIOV)
		ret = hqmv2_vf_init_interrupt_handlers(dev, pdev);
	else
		ret = hqmv2_vdev_init_interrupt_handlers(dev, pdev);

	if (ret) {
		pci_free_irq_vectors(pdev);
		return ret;
	}

	/* Initialize per-CQ interrupt structures, such as wait queues that
	 * threads will wait on until the CQ's interrupt fires.
	 */
	for (i = 0; i < HQMV2_MAX_NUM_LDB_PORTS; i++) {
		init_waitqueue_head(&dev->intr.ldb_cq_intr[i].wq_head);
		mutex_init(&dev->intr.ldb_cq_intr[i].mutex);
	}

	for (i = 0; i < HQMV2_MAX_NUM_DIR_PORTS; i++) {
		init_waitqueue_head(&dev->intr.dir_cq_intr[i].wq_head);
		mutex_init(&dev->intr.dir_cq_intr[i].mutex);
	}

	return 0;
}

void hqmv2_vf_reinit_interrupts(struct hqmv2_dev *dev)
{
}

void hqmv2_vf_free_interrupts(struct hqmv2_dev *dev, struct pci_dev *pdev)
{
	int i;

	if (hqmv2_hw_get_virt_mode(&dev->hw) == HQMV2_VIRT_SRIOV)
		hqmv2_vf_unclaim_cq_interrupts(dev);

	for (i = 0; i < dev->intr.num_vectors; i++) {
		if (dev->intr.isr_registered[i])
			devm_free_irq(&pdev->dev, pci_irq_vector(pdev, i), dev);
	}

	pci_free_irq_vectors(pdev);
}

int hqmv2_vf_enable_ldb_cq_interrupts(struct hqmv2_dev *dev,
				      int id,
				      u16 thresh)
{
	struct hqmv2_mbox_enable_ldb_port_intr_cmd_resp resp;
	struct hqmv2_mbox_enable_ldb_port_intr_cmd_req req;
	struct hqmv2_dev *owner;
	int ret;
	int i;

	/* If no owner was registered, dev->intr...configured remains false,
	 * and any attempts to block on the CQ interrupt will fail. This will
	 * only happen if the VF doesn't have enough auxiliary VFs to service
	 * its CQ interrupts.
	 */
	owner = dev->intr.ldb_cq_intr_owner[id];
	if (!owner) {
		HQMV2_INFO(dev->hqmv2_device,
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

	req.hdr.type = HQMV2_MBOX_CMD_ENABLE_LDB_PORT_INTR;
	req.port_id = id;
	req.vector = i;
	req.owner_vf = owner->vf_id_state.vf_id;
	req.thresh = thresh;

	ret = hqmv2_send_sync_mbox_cmd(dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(dev->hqmv2_device,
			  "LDB CQ interrupt enable failed with mailbox error: %s\n",
			  HQMV2_MBOX_ST_STRING(&resp));
	}

	return resp.error_code;
}

int hqmv2_vf_enable_dir_cq_interrupts(struct hqmv2_dev *dev,
				      int id,
				      u16 thresh)
{
	struct hqmv2_mbox_enable_dir_port_intr_cmd_resp resp;
	struct hqmv2_mbox_enable_dir_port_intr_cmd_req req;
	struct hqmv2_dev *owner;
	int ret;
	int i;

	/* If no owner was registered, dev->intr...configured remains false,
	 * and any attempts to block on the CQ interrupt will fail. This will
	 * only happen if the VF doesn't have enough auxiliary VFs to service
	 * its CQ interrupts.
	 */
	owner = dev->intr.dir_cq_intr_owner[id];
	if (!owner) {
		HQMV2_INFO(dev->hqmv2_device,
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

	req.hdr.type = HQMV2_MBOX_CMD_ENABLE_DIR_PORT_INTR;
	req.port_id = id;
	req.vector = i;
	req.owner_vf = owner->vf_id_state.vf_id;
	req.thresh = thresh;

	ret = hqmv2_send_sync_mbox_cmd(dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(dev->hqmv2_device,
			  "DIR CQ interrupt enable failed with mailbox error: %s\n",
			  HQMV2_MBOX_ST_STRING(&resp));
	}

	return resp.error_code;
}

int hqmv2_vf_arm_cq_interrupt(struct hqmv2_dev *dev,
			      int domain_id,
			      int port_id,
			      bool is_ldb)
{
	struct hqmv2_mbox_arm_cq_intr_cmd_resp resp;
	struct hqmv2_mbox_arm_cq_intr_cmd_req req;
	int ret;

	req.hdr.type = HQMV2_MBOX_CMD_ARM_CQ_INTR;
	req.domain_id = domain_id;
	req.port_id = port_id;
	req.is_ldb = is_ldb;

	/* Unlike other VF ioctl callbacks, this one isn't called while holding
	 * the resource mutex. However, we must serialize access to the mailbox
	 * to prevent data corruption.
	 */
	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_send_sync_mbox_cmd(dev, &req, sizeof(req), 1);
	if (ret) {
		mutex_unlock(&dev->resource_mutex);
		return ret;
	}

	hqmv2_vf_read_pf_mbox_resp(&dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(dev->hqmv2_device,
			  "LDB CQ interrupt enable failed with mailbox error: %s\n",
			  HQMV2_MBOX_ST_STRING(&resp));
	}

	mutex_unlock(&dev->resource_mutex);

	return resp.error_code;
}

/*******************************/
/****** Driver management ******/
/*******************************/

int hqmv2_vf_init_driver_state(struct hqmv2_dev *hqmv2_dev)
{
	int i;

	if (movdir64b_supported()) {
		hqmv2_dev->enqueue_four = hqmv2_movdir64b;
	} else {
#ifdef CONFIG_AS_SSE2
		hqmv2_dev->enqueue_four = hqmv2_movntdq;
#else
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "%s: Platforms without movdir64 must support SSE2\n",
			  hqmv2_driver_name);
		return -EINVAL;
#endif
	}

	/* Initialize software state */
	for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++) {
		struct hqmv2_domain_dev *domain = &hqmv2_dev->sched_domains[i];

		domain->file_refcnt = -1;

		memset(&domain->alerts, 0, sizeof(domain->alerts));
		domain->alert_rd_idx = 0;
		domain->alert_wr_idx = 0;
		mutex_init(&domain->alert_mutex);
		init_waitqueue_head(&domain->wq_head);
	}

	mutex_init(&hqmv2_dev->resource_mutex);
	mutex_init(&hqmv2_dev->perf.measurement_mutex);

	return 0;
}

void hqmv2_vf_free_driver_state(struct hqmv2_dev *hqmv2_dev)
{
}

int hqmv2_vf_cdev_add(struct hqmv2_dev *hqmv2_dev,
		      dev_t base,
		      const struct file_operations *fops)
{
	int ret;

	hqmv2_dev->dev_number = MKDEV(MAJOR(base),
				      MINOR(base) +
				      (hqmv2_dev->id *
				       HQMV2_NUM_DEV_FILES_PER_DEVICE));

	cdev_init(&hqmv2_dev->cdev, fops);

	hqmv2_dev->cdev.dev   = hqmv2_dev->dev_number;
	hqmv2_dev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&hqmv2_dev->cdev,
		       hqmv2_dev->cdev.dev,
		       HQMV2_NUM_DEV_FILES_PER_DEVICE);

	if (ret < 0)
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "%s: cdev_add() returned %d\n",
			  hqmv2_driver_name, ret);

	return ret;
}

void hqmv2_vf_cdev_del(struct hqmv2_dev *hqmv2_dev)
{
	cdev_del(&hqmv2_dev->cdev);
}

int hqmv2_vf_device_create(struct hqmv2_dev *hqmv2_dev,
			   struct pci_dev *pdev,
			   struct class *ws_class)
{
	dev_t dev;

	dev = MKDEV(MAJOR(hqmv2_dev->dev_number),
		    MINOR(hqmv2_dev->dev_number) + HQMV2_MAX_NUM_DOMAINS);

	/* Create a new device in order to create a /dev/hqm node. This device
	 * is a child of the HQM PCI device.
	 */
	hqmv2_dev->hqmv2_device = device_create(ws_class,
						&pdev->dev,
						dev,
						hqmv2_dev,
						"hqm%d/hqm",
						hqmv2_dev->id);

	if (IS_ERR_VALUE(PTR_ERR(hqmv2_dev->hqmv2_device))) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "%s: device_create() returned %ld\n",
			  hqmv2_driver_name, PTR_ERR(hqmv2_dev->hqmv2_device));

		return PTR_ERR(hqmv2_dev->hqmv2_device);
	}

	return 0;
}

void hqmv2_vf_device_destroy(struct hqmv2_dev *hqmv2_dev,
			     struct class *ws_class)
{
	device_destroy(ws_class,
		       MKDEV(MAJOR(hqmv2_dev->dev_number),
			     MINOR(hqmv2_dev->dev_number) +
				HQMV2_MAX_NUM_DOMAINS));
}

/*****************************/
/****** Sysfs callbacks ******/
/*****************************/

static int
hqmv2_vf_get_num_used_rsrcs(struct hqmv2_hw *hw,
			    struct hqmv2_get_num_resources_args *args)
{
	struct hqmv2_mbox_get_num_resources_cmd_resp resp;
	struct hqmv2_mbox_get_num_resources_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_GET_NUM_USED_RESOURCES;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

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

#define HQMV2_VF_TOTAL_SYSFS_SHOW(name)				\
static ssize_t total_##name##_show(				\
	struct device *dev,					\
	struct device_attribute *attr,				\
	char *buf)						\
{								\
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);	\
	struct hqmv2_get_num_resources_args rsrcs[2];		\
	struct hqmv2_hw *hw = &hqmv2_dev->hw;			\
	int val, i;						\
								\
	mutex_lock(&hqmv2_dev->resource_mutex);			\
								\
	val = hqmv2_dev->ops->get_num_resources(hw, &rsrcs[0]);	\
	if (val) {						\
		mutex_unlock(&hqmv2_dev->resource_mutex);	\
		return -1;					\
	}							\
								\
	val = hqmv2_vf_get_num_used_rsrcs(hw, &rsrcs[1]);	\
	if (val) {						\
		mutex_unlock(&hqmv2_dev->resource_mutex);	\
		return -1;					\
	}							\
								\
	mutex_unlock(&hqmv2_dev->resource_mutex);		\
								\
	val = 0;						\
	for (i = 0; i < ARRAY_SIZE(rsrcs); i++)			\
		val += rsrcs[i].name;				\
								\
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);		\
}

HQMV2_VF_TOTAL_SYSFS_SHOW(num_sched_domains)
HQMV2_VF_TOTAL_SYSFS_SHOW(num_ldb_queues)
HQMV2_VF_TOTAL_SYSFS_SHOW(num_ldb_ports)
HQMV2_VF_TOTAL_SYSFS_SHOW(num_cos0_ldb_ports)
HQMV2_VF_TOTAL_SYSFS_SHOW(num_cos1_ldb_ports)
HQMV2_VF_TOTAL_SYSFS_SHOW(num_cos2_ldb_ports)
HQMV2_VF_TOTAL_SYSFS_SHOW(num_cos3_ldb_ports)
HQMV2_VF_TOTAL_SYSFS_SHOW(num_dir_ports)
HQMV2_VF_TOTAL_SYSFS_SHOW(num_ldb_credits)
HQMV2_VF_TOTAL_SYSFS_SHOW(num_dir_credits)
HQMV2_VF_TOTAL_SYSFS_SHOW(num_atomic_inflights)
HQMV2_VF_TOTAL_SYSFS_SHOW(num_hist_list_entries)

#define HQMV2_VF_AVAIL_SYSFS_SHOW(name)				       \
static ssize_t avail_##name##_show(				       \
	struct device *dev,					       \
	struct device_attribute *attr,				       \
	char *buf)						       \
{								       \
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);	       \
	struct hqmv2_get_num_resources_args num_avail_rsrcs;	       \
	struct hqmv2_hw *hw = &hqmv2_dev->hw;			       \
	int val;						       \
								       \
	mutex_lock(&hqmv2_dev->resource_mutex);			       \
								       \
	val = hqmv2_dev->ops->get_num_resources(hw, &num_avail_rsrcs); \
								       \
	mutex_unlock(&hqmv2_dev->resource_mutex);		       \
								       \
	if (val)						       \
		return -1;					       \
								       \
	val = num_avail_rsrcs.name;				       \
								       \
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);		       \
}

HQMV2_VF_AVAIL_SYSFS_SHOW(num_sched_domains)
HQMV2_VF_AVAIL_SYSFS_SHOW(num_ldb_queues)
HQMV2_VF_AVAIL_SYSFS_SHOW(num_ldb_ports)
HQMV2_VF_AVAIL_SYSFS_SHOW(num_cos0_ldb_ports)
HQMV2_VF_AVAIL_SYSFS_SHOW(num_cos1_ldb_ports)
HQMV2_VF_AVAIL_SYSFS_SHOW(num_cos2_ldb_ports)
HQMV2_VF_AVAIL_SYSFS_SHOW(num_cos3_ldb_ports)
HQMV2_VF_AVAIL_SYSFS_SHOW(num_dir_ports)
HQMV2_VF_AVAIL_SYSFS_SHOW(num_ldb_credits)
HQMV2_VF_AVAIL_SYSFS_SHOW(num_dir_credits)
HQMV2_VF_AVAIL_SYSFS_SHOW(num_atomic_inflights)
HQMV2_VF_AVAIL_SYSFS_SHOW(num_hist_list_entries)

static ssize_t max_ctg_hl_entries_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);
	struct hqmv2_get_num_resources_args num_avail_rsrcs;
	struct hqmv2_hw *hw = &hqmv2_dev->hw;
	int val;

	mutex_lock(&hqmv2_dev->resource_mutex);

	val = hqmv2_dev->ops->get_num_resources(hw, &num_avail_rsrcs);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	if (val)
		return -1;

	val = num_avail_rsrcs.max_contiguous_hist_list_entries;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

/* Device attribute name doesn't match the show function name, so we define our
 * own DEVICE_ATTR macro.
 */
#define HQMV2_DEVICE_ATTR_RO(_prefix, _name) \
struct device_attribute dev_attr_##_prefix##_##_name = {\
	.attr = { .name = __stringify(_name), .mode = 0444 },\
	.show = _prefix##_##_name##_show,\
}

static HQMV2_DEVICE_ATTR_RO(total, num_sched_domains);
static HQMV2_DEVICE_ATTR_RO(total, num_ldb_queues);
static HQMV2_DEVICE_ATTR_RO(total, num_ldb_ports);
static HQMV2_DEVICE_ATTR_RO(total, num_cos0_ldb_ports);
static HQMV2_DEVICE_ATTR_RO(total, num_cos1_ldb_ports);
static HQMV2_DEVICE_ATTR_RO(total, num_cos2_ldb_ports);
static HQMV2_DEVICE_ATTR_RO(total, num_cos3_ldb_ports);
static HQMV2_DEVICE_ATTR_RO(total, num_dir_ports);
static HQMV2_DEVICE_ATTR_RO(total, num_ldb_credits);
static HQMV2_DEVICE_ATTR_RO(total, num_dir_credits);
static HQMV2_DEVICE_ATTR_RO(total, num_atomic_inflights);
static HQMV2_DEVICE_ATTR_RO(total, num_hist_list_entries);

static struct attribute *hqmv2_total_attrs[] = {
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

const static struct attribute_group hqmv2_vf_total_attr_group = {
	.attrs = hqmv2_total_attrs,
	.name = "total_resources",
};

static HQMV2_DEVICE_ATTR_RO(avail, num_sched_domains);
static HQMV2_DEVICE_ATTR_RO(avail, num_ldb_queues);
static HQMV2_DEVICE_ATTR_RO(avail, num_ldb_ports);
static HQMV2_DEVICE_ATTR_RO(avail, num_cos0_ldb_ports);
static HQMV2_DEVICE_ATTR_RO(avail, num_cos1_ldb_ports);
static HQMV2_DEVICE_ATTR_RO(avail, num_cos2_ldb_ports);
static HQMV2_DEVICE_ATTR_RO(avail, num_cos3_ldb_ports);
static HQMV2_DEVICE_ATTR_RO(avail, num_dir_ports);
static HQMV2_DEVICE_ATTR_RO(avail, num_ldb_credits);
static HQMV2_DEVICE_ATTR_RO(avail, num_dir_credits);
static HQMV2_DEVICE_ATTR_RO(avail, num_atomic_inflights);
static HQMV2_DEVICE_ATTR_RO(avail, num_hist_list_entries);
static DEVICE_ATTR_RO(max_ctg_hl_entries);

static struct attribute *hqmv2_avail_attrs[] = {
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

const static struct attribute_group hqmv2_vf_avail_attr_group = {
	.attrs = hqmv2_avail_attrs,
	.name = "avail_resources",
};

static ssize_t dev_id_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n", hqmv2_dev->id);
}

static DEVICE_ATTR_RO(dev_id);

int hqmv2_vf_sysfs_create(struct hqmv2_dev *hqmv2_dev)
{
	struct kobject *kobj;
	int ret;

	kobj = &hqmv2_dev->pdev->dev.kobj;

	ret = sysfs_create_file(kobj, &dev_attr_dev_id.attr);
	if (ret)
		goto hqm_dev_id_attr_group_fail;

	ret = sysfs_create_group(kobj, &hqmv2_vf_total_attr_group);
	if (ret)
		goto hqmv2_total_attr_group_fail;

	ret = sysfs_create_group(kobj, &hqmv2_vf_avail_attr_group);
	if (ret)
		goto hqmv2_avail_attr_group_fail;

	return 0;

hqmv2_avail_attr_group_fail:
	sysfs_remove_group(kobj, &hqmv2_vf_total_attr_group);
hqmv2_total_attr_group_fail:
	sysfs_remove_file(kobj, &dev_attr_dev_id.attr);
hqm_dev_id_attr_group_fail:
	return ret;
}

void hqmv2_vf_sysfs_destroy(struct hqmv2_dev *hqmv2_dev)
{
	struct kobject *kobj;

	kobj = &hqmv2_dev->pdev->dev.kobj;

	sysfs_remove_group(kobj, &hqmv2_vf_avail_attr_group);
	sysfs_remove_group(kobj, &hqmv2_vf_total_attr_group);
	sysfs_remove_file(kobj, &dev_attr_dev_id.attr);
}

void hqmv2_vf_sysfs_reapply_configuration(struct hqmv2_dev *hqmv2_dev)
{
}

/**********************************/
/****** Interrupt management ******/
/**********************************/

int hqmv2_send_sync_mbox_cmd(struct hqmv2_dev *hqmv2_dev,
			     void *data,
			     int len,
			     int timeout_s)
{
	int retry_cnt;

	if (len > HQMV2_FUNC_VF_VF2PF_MAILBOX_BYTES) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "Internal error: VF mbox message too large\n");
		return -1;
	}

	hqmv2_vf_write_pf_mbox_req(&hqmv2_dev->hw, data, len);

	hqmv2_send_async_vdev_to_pf_msg(&hqmv2_dev->hw);

	/* Timeout after timeout_s seconds of inactivity */
	retry_cnt = 0;
	while (!hqmv2_vdev_to_pf_complete(&hqmv2_dev->hw)) {
		usleep_range(1000, 1001);
		if (++retry_cnt >= 1000 * timeout_s) {
			HQMV2_ERR(hqmv2_dev->hqmv2_device,
				  "VF driver timed out waiting for mbox response\n");
			return -1;
		}
	}

	return 0;
}

static struct hqmv2_dev *hqmv2_vf_get_primary(struct hqmv2_dev *hqmv2_dev)
{
	struct vf_id_state *vf_id_state;
	struct hqmv2_dev *dev;
	bool found = false;

	if (!hqmv2_dev->vf_id_state.is_auxiliary_vf)
		return hqmv2_dev;

	mutex_lock(&driver_lock);

	vf_id_state = &hqmv2_dev->vf_id_state;

	list_for_each_entry(dev, &hqmv2_dev_list, list) {
		if (dev->type == HQMV2_VF &&
		    dev->vf_id_state.pf_id == vf_id_state->pf_id &&
		    dev->vf_id_state.vf_id == vf_id_state->primary_vf_id) {
			found = true;
			break;
		}
	}

	mutex_unlock(&driver_lock);

	return (found) ? dev : NULL;
}

static int hqmv2_init_siov_vdev_interrupt_state(struct hqmv2_dev *hqmv2_dev)
{
	struct hqmv2_get_num_resources_args num_rsrcs;

	hqmv2_dev->ops->get_num_resources(&hqmv2_dev->hw, &num_rsrcs);

	hqmv2_dev->intr.num_ldb_ports = num_rsrcs.num_ldb_ports;
	hqmv2_dev->intr.num_dir_ports = num_rsrcs.num_dir_ports;

	return 0;
}

static int hqmv2_init_auxiliary_vf_interrupts(struct hqmv2_dev *hqmv2_dev)
{
	struct hqmv2_get_num_resources_args num_rsrcs;
	struct hqmv2_dev *aux_vf;
	int ret;

	/* If the primary hasn't been probed yet, we can't init the auxiliary's
	 * interrupts.
	 */
	if (hqmv2_dev->vf_id_state.is_auxiliary_vf &&
	    !hqmv2_dev->vf_id_state.primary_vf)
		return 0;

	if (hqmv2_dev->vf_id_state.is_auxiliary_vf)
		return hqmv2_dev->ops->init_interrupts(hqmv2_dev,
						       hqmv2_dev->pdev);

	/* This is a primary VF, so iniitalize all of its auxiliary siblings
	 * that were already probed.
	 */
	hqmv2_dev->ops->get_num_resources(&hqmv2_dev->hw, &num_rsrcs);

	hqmv2_dev->intr.num_ldb_ports = num_rsrcs.num_ldb_ports;
	hqmv2_dev->intr.num_dir_ports = num_rsrcs.num_dir_ports;

	mutex_lock(&driver_lock);

	list_for_each_entry(aux_vf, &hqmv2_dev_list, list) {
		if (aux_vf->type != HQMV2_VF)
			continue;

		if (!aux_vf->vf_id_state.is_auxiliary_vf)
			continue;

		if (aux_vf->vf_id_state.pf_id != hqmv2_dev->vf_id_state.pf_id)
			continue;

		if (aux_vf->vf_id_state.primary_vf_id !=
		    hqmv2_dev->vf_id_state.vf_id)
			continue;

		aux_vf->vf_id_state.primary_vf = hqmv2_dev;

		ret = aux_vf->ops->init_interrupts(aux_vf, aux_vf->pdev);
		if (ret)
			goto interrupt_cleanup;
	}

	mutex_unlock(&driver_lock);

	return 0;

interrupt_cleanup:

	list_for_each_entry(aux_vf, &hqmv2_dev_list, list) {
		if (aux_vf->vf_id_state.primary_vf == hqmv2_dev)
			aux_vf->ops->free_interrupts(aux_vf, aux_vf->pdev);
	}

	mutex_unlock(&driver_lock);

	return ret;
}

int hqmv2_vf_register_driver(struct hqmv2_dev *hqmv2_dev)
{
	struct hqmv2_mbox_register_cmd_resp resp;
	struct hqmv2_mbox_register_cmd_req req;
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

	req.hdr.type = HQMV2_MBOX_CMD_REGISTER;
	req.interface_version = HQMV2_MBOX_INTERFACE_VERSION;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "VF driver registration failed with mailbox error: %s\n",
			  HQMV2_MBOX_ST_STRING(&resp));

		if (resp.hdr.status == HQMV2_MBOX_ST_VERSION_MISMATCH) {
			HQMV2_ERR(hqmv2_dev->hqmv2_device,
				  "VF driver mailbox interface version: %d\n",
				  HQMV2_MBOX_INTERFACE_VERSION);
			HQMV2_ERR(hqmv2_dev->hqmv2_device,
				  "PF driver mailbox interface version: %d\n",
				  resp.interface_version);
		}

		return -1;
	}

	hqmv2_dev->vf_id_state.pf_id = resp.pf_id;
	hqmv2_dev->vf_id_state.vf_id = resp.vf_id;
	hqmv2_dev->vf_id_state.is_auxiliary_vf = resp.is_auxiliary_vf;
	hqmv2_dev->vf_id_state.primary_vf_id = resp.primary_vf_id;

	/* Auxiliary VF interrupts are initialized in the register_driver
	 * callback and freed in the unregister_driver callback. There are
	 * two possible cases.
	 * 1. The auxiliary VF is probed after its primary: during the aux VF's
	 *    probe, it initializes its interrupts.
	 * 2. The auxiliary VF is probed before its primary: during the primary
	 *    VF's driver registration, it initializes the interrupts of all its
	 *    aux siblings that have already been probed.
	 */

	/* If the VF is not auxiliary, hqmv2_vf_get_primary() returns
	 * hqmv2_dev.
	 */
	hqmv2_dev->vf_id_state.primary_vf = hqmv2_vf_get_primary(hqmv2_dev);

	/* If this is a primary VF, initialize the interrupts of any auxiliary
	 * VFs that were already probed. If this is an auxiliary VF and its
	 * primary has been probed, initialize the auxiliary's interrupts.
	 *
	 * If this is an S-IOV vdev, initialize the state needed to configure
	 * and service its CQ interrupts.
	 */
	if (hqmv2_hw_get_virt_mode(&hqmv2_dev->hw) == HQMV2_VIRT_SRIOV)
		return hqmv2_init_auxiliary_vf_interrupts(hqmv2_dev);
	else
		return hqmv2_init_siov_vdev_interrupt_state(hqmv2_dev);
}

void hqmv2_vf_unregister_driver(struct hqmv2_dev *hqmv2_dev)
{
	struct hqmv2_mbox_unregister_cmd_resp resp;
	struct hqmv2_mbox_unregister_cmd_req req;
	int ret;

	/* Aux VF interrupts are initialized in the register_driver callback
	 * and freed here.
	 */
	if (hqmv2_dev->vf_id_state.is_auxiliary_vf &&
	    hqmv2_dev->vf_id_state.primary_vf)
		hqmv2_dev->ops->free_interrupts(hqmv2_dev, hqmv2_dev->pdev);

	req.hdr.type = HQMV2_MBOX_CMD_UNREGISTER;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "VF driver registration failed with mailbox error: %s\n",
			  HQMV2_MBOX_ST_STRING(&resp));
	}
}

void hqmv2_vf_enable_pm(struct hqmv2_dev *hqmv2_dev)
{
	/* Function intentionally left blank */
}

int hqmv2_vf_wait_for_device_ready(struct hqmv2_dev *hqmv2_dev,
				   struct pci_dev *pdev)
{
	/* Device ready check only performed on the PF */
	return 0;
}

/*****************************/
/****** IOCTL callbacks ******/
/*****************************/

int hqmv2_vf_create_sched_domain(struct hqmv2_hw *hw,
				 struct hqmv2_create_sched_domain_args *args,
				 struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_create_sched_domain_cmd_resp resp;
	struct hqmv2_mbox_create_sched_domain_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_CREATE_SCHED_DOMAIN;
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

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

int hqmv2_vf_create_ldb_queue(struct hqmv2_hw *hw,
			      u32 id,
			      struct hqmv2_create_ldb_queue_args *args,
			      struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_create_ldb_queue_cmd_resp resp;
	struct hqmv2_mbox_create_ldb_queue_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_CREATE_LDB_QUEUE;
	req.domain_id = id;
	req.num_sequence_numbers = args->num_sequence_numbers;
	req.num_qid_inflights = args->num_qid_inflights;
	req.num_atomic_inflights = args->num_atomic_inflights;
	req.lock_id_comp_level = args->lock_id_comp_level;
	req.depth_threshold = args->depth_threshold;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

int hqmv2_vf_create_dir_queue(struct hqmv2_hw *hw,
			      u32 id,
			      struct hqmv2_create_dir_queue_args *args,
			      struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_create_dir_queue_cmd_resp resp;
	struct hqmv2_mbox_create_dir_queue_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_CREATE_DIR_QUEUE;
	req.domain_id = id;
	req.port_id = args->port_id;
	req.depth_threshold = args->depth_threshold;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

int hqmv2_vf_create_ldb_port(struct hqmv2_hw *hw,
			     u32 id,
			     struct hqmv2_create_ldb_port_args *args,
			     uintptr_t pop_count_dma_base,
			     uintptr_t cq_dma_base,
			     struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_create_ldb_port_cmd_resp resp;
	struct hqmv2_mbox_create_ldb_port_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_CREATE_LDB_PORT;
	req.domain_id = id;
	req.pop_count_address = pop_count_dma_base;
	req.cq_depth = args->cq_depth;
	req.cq_history_list_size = args->cq_history_list_size;
	req.cos_id = args->cos_id;
	req.cos_strict = args->cos_strict;
	req.cq_base_address = cq_dma_base;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

int hqmv2_vf_create_dir_port(struct hqmv2_hw *hw,
			     u32 id,
			     struct hqmv2_create_dir_port_args *args,
			     uintptr_t pop_count_dma_base,
			     uintptr_t cq_dma_base,
			     struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_create_dir_port_cmd_resp resp;
	struct hqmv2_mbox_create_dir_port_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_CREATE_DIR_PORT;
	req.domain_id = id;
	req.pop_count_address = pop_count_dma_base;
	req.cq_depth = args->cq_depth;
	req.cq_base_address = cq_dma_base;
	req.queue_id = args->queue_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

int hqmv2_vf_start_domain(struct hqmv2_hw *hw,
			  u32 id,
			  struct hqmv2_start_domain_args *args,
			  struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_start_domain_cmd_resp resp;
	struct hqmv2_mbox_start_domain_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_START_DOMAIN;
	req.domain_id = id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

int hqmv2_vf_map_qid(struct hqmv2_hw *hw,
		     u32 id,
		     struct hqmv2_map_qid_args *args,
		     struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_map_qid_cmd_resp resp;
	struct hqmv2_mbox_map_qid_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_MAP_QID;
	req.domain_id = id;
	req.port_id = args->port_id;
	req.qid = args->qid;
	req.priority = args->priority;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

int hqmv2_vf_unmap_qid(struct hqmv2_hw *hw,
		       u32 id,
		       struct hqmv2_unmap_qid_args *args,
		       struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_unmap_qid_cmd_resp resp;
	struct hqmv2_mbox_unmap_qid_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_UNMAP_QID;
	req.domain_id = id;
	req.port_id = args->port_id;
	req.qid = args->qid;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

int hqmv2_vf_enable_ldb_port(struct hqmv2_hw *hw,
			     u32 id,
			     struct hqmv2_enable_ldb_port_args *args,
			     struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_enable_ldb_port_cmd_resp resp;
	struct hqmv2_mbox_enable_ldb_port_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_ENABLE_LDB_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

int hqmv2_vf_disable_ldb_port(struct hqmv2_hw *hw,
			      u32 id,
			      struct hqmv2_disable_ldb_port_args *args,
			      struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_disable_ldb_port_cmd_resp resp;
	struct hqmv2_mbox_disable_ldb_port_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_DISABLE_LDB_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

int hqmv2_vf_enable_dir_port(struct hqmv2_hw *hw,
			     u32 id,
			     struct hqmv2_enable_dir_port_args *args,
			     struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_enable_dir_port_cmd_resp resp;
	struct hqmv2_mbox_enable_dir_port_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_ENABLE_DIR_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

int hqmv2_vf_disable_dir_port(struct hqmv2_hw *hw,
			      u32 id,
			      struct hqmv2_disable_dir_port_args *args,
			      struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_disable_dir_port_cmd_resp resp;
	struct hqmv2_mbox_disable_dir_port_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_DISABLE_DIR_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

int hqmv2_vf_get_num_resources(struct hqmv2_hw *hw,
			       struct hqmv2_get_num_resources_args *args)
{
	struct hqmv2_mbox_get_num_resources_cmd_resp resp;
	struct hqmv2_mbox_get_num_resources_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_GET_NUM_RESOURCES;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

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

int hqmv2_vf_reset_domain(struct hqmv2_hw *hw, u32 id)
{
	struct hqmv2_mbox_reset_sched_domain_cmd_resp resp;
	struct hqmv2_mbox_reset_sched_domain_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_RESET_SCHED_DOMAIN;
	req.id = id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.error_code;
}

#define HQMV2_VF_PERF_MEAS_TMO_S 60

static int hqmv2_vf_init_perf_metric_measurement(struct hqmv2_dev *hqmv2_dev,
						 u32 duration)
{
	struct hqmv2_mbox_init_cq_sched_count_measure_cmd_resp resp;
	struct hqmv2_mbox_init_cq_sched_count_measure_cmd_req req;
	int ret;

	req.hdr.type = HQMV2_MBOX_CMD_INIT_CQ_SCHED_COUNT;
	req.duration_us = duration;

	mutex_lock(&hqmv2_dev->resource_mutex);

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev,
				       &req,
				       sizeof(req),
				       HQMV2_VF_PERF_MEAS_TMO_S);
	if (ret) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return ret;
	}

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	mutex_unlock(&hqmv2_dev->resource_mutex);

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.error_code;
}

static int
hqmv2_vf_collect_cq_sched_count(struct hqmv2_dev *hqmv2_dev,
				union hqmv2_perf_metric_group_data *data,
				int cq_id,
				bool is_ldb,
				u32 *elapsed)
{
	struct hqmv2_mbox_collect_cq_sched_count_cmd_resp resp;
	struct hqmv2_mbox_collect_cq_sched_count_cmd_req req;
	int ret;

	req.hdr.type = HQMV2_MBOX_CMD_COLLECT_CQ_SCHED_COUNT;
	req.cq_id = cq_id;
	req.is_ldb = is_ldb;

	mutex_lock(&hqmv2_dev->resource_mutex);

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return ret;
	}

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	mutex_unlock(&hqmv2_dev->resource_mutex);

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		return -1;
	}

	if (is_ldb)
		data->group_0.hqmv2_ldb_cq_sched_count[cq_id] =
			resp.cq_sched_count;
	else
		data->group_0.hqmv2_dir_cq_sched_count[cq_id] =
			resp.cq_sched_count;

	*elapsed = resp.elapsed;

	return resp.error_code;
}

int hqmv2_vf_collect_cq_sched_counts(struct hqmv2_dev *hqmv2_dev,
				     union hqmv2_perf_metric_group_data *data,
				     u32 *elapsed)
{
	struct hqmv2_get_num_resources_args num_rsrcs;
	int i, ret;

	/* Query how many ldb and dir ports this VF owns */
	mutex_lock(&hqmv2_dev->resource_mutex);

	ret = hqmv2_dev->ops->get_num_resources(&hqmv2_dev->hw, &num_rsrcs);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	if (ret)
		return -EINVAL;

	for (i = 0; i < num_rsrcs.num_ldb_ports; i++) {
		ret = hqmv2_vf_collect_cq_sched_count(hqmv2_dev,
						      data,
						      i,
						      true,
						      elapsed);
		if (ret)
			return ret;
	}

	for (i = 0; i < num_rsrcs.num_dir_ports; i++) {
		ret = hqmv2_vf_collect_cq_sched_count(hqmv2_dev,
						      data,
						      i,
						      false,
						      elapsed);
		if (ret)
			return ret;
	}

	return 0;
}

int hqmv2_vf_measure_perf(struct hqmv2_dev *dev,
			  struct hqmv2_sample_perf_counters_args *args,
			  struct hqmv2_cmd_response *response)
{
	struct hqmv2_perf_metric_pre_data *pre = NULL;
	union hqmv2_perf_metric_group_data data;
	int retry_counter;
	u32 elapsed;
	int ret = 0;

	memset(&data, 0, sizeof(data));

	/* Since all other metrics are device-global, the only perf metrics that
	 * a VF can access are the scheduling counts of its CQs. All other perf
	 * metrics will return 0.
	 */
	if (args->perf_metric_group_id != 0) {
		if (copy_to_user((void __user *)args->perf_metric_group_data,
				 &data,
				 sizeof(data))) {
			pr_err("Invalid performance metric group data pointer\n");
			return -EFAULT;
		}

		return 0;
	}

	pre = devm_kmalloc(&dev->pdev->dev, sizeof(*pre), GFP_KERNEL);
	if (!pre) {
		ret = -ENOMEM;
		goto done;
	}

	/* Only one active measurement is allowed at a time. */
	mutex_lock(&dev->perf.measurement_mutex);

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset. To recover, all HQM applications must close.\n",
			  __func__);
		ret = -EINVAL;
		mutex_unlock(&dev->perf.measurement_mutex);
		goto done;
	}

	/* Send a mailbox request to start the measurement. If another
	 * measurement is in progress, this will return -EBUSY. Retry for up to
	 * 60s (the maximum measurement duration), and then fail out.
	 */
	retry_counter = 0;
	do {
		u32 duration = args->measurement_duration_us;

		ret = hqmv2_vf_init_perf_metric_measurement(dev, duration);

		if ((ret && ret != -EBUSY) || retry_counter++ == 60) {
			mutex_unlock(&dev->perf.measurement_mutex);
			goto done;
		}
	} while (ret == -EBUSY);

	set_current_state(TASK_INTERRUPTIBLE);

	schedule_timeout(usecs_to_jiffies(args->measurement_duration_us));

	/* Send mailbox requests to collect the measurement data */
	ret = hqmv2_vf_collect_cq_sched_counts(dev, &data, &elapsed);
	if (ret) {
		mutex_unlock(&dev->perf.measurement_mutex);
		goto done;
	}

	mutex_unlock(&dev->perf.measurement_mutex);

	if (copy_to_user((void __user *)args->elapsed_time_us,
			 &elapsed,
			 sizeof(elapsed))) {
		devm_kfree(&dev->pdev->dev, pre);

		pr_err("Invalid elapsed time pointer\n");
		return -EFAULT;
	}

	if (copy_to_user((void __user *)args->perf_metric_group_data,
			 &data,
			 sizeof(data))) {
		devm_kfree(&dev->pdev->dev, pre);

		pr_err("Invalid performance metric group data pointer\n");
		return -EFAULT;
	}

done:
	if (pre)
		devm_kfree(&dev->pdev->dev, pre);

	return ret;
}

int hqmv2_vf_get_ldb_queue_depth(struct hqmv2_hw *hw,
				 u32 id,
				 struct hqmv2_get_ldb_queue_depth_args *args,
				 struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_get_ldb_queue_depth_cmd_resp resp;
	struct hqmv2_mbox_get_ldb_queue_depth_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_GET_LDB_QUEUE_DEPTH;
	req.domain_id = id;
	req.queue_id = args->queue_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.depth;

	return resp.error_code;
}

int hqmv2_vf_get_dir_queue_depth(struct hqmv2_hw *hw,
				 u32 id,
				 struct hqmv2_get_dir_queue_depth_args *args,
				 struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_get_dir_queue_depth_cmd_resp resp;
	struct hqmv2_mbox_get_dir_queue_depth_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_GET_DIR_QUEUE_DEPTH;
	req.domain_id = id;
	req.queue_id = args->queue_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.depth;

	return resp.error_code;
}

int hqmv2_vf_pending_port_unmaps(struct hqmv2_hw *hw,
				 u32 id,
				 struct hqmv2_pending_port_unmaps_args *args,
				 struct hqmv2_cmd_response *user_resp)
{
	struct hqmv2_mbox_pending_port_unmaps_cmd_resp resp;
	struct hqmv2_mbox_pending_port_unmaps_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_PENDING_PORT_UNMAPS;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		user_resp->status = HQMV2_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.num;

	return resp.error_code;
}

/**************************************/
/****** Resource query callbacks ******/
/**************************************/

int hqmv2_vf_ldb_port_owned_by_domain(struct hqmv2_hw *hw,
				      u32 domain_id,
				      u32 port_id)
{
	struct hqmv2_mbox_ldb_port_owned_by_domain_cmd_resp resp;
	struct hqmv2_mbox_ldb_port_owned_by_domain_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_LDB_PORT_OWNED_BY_DOMAIN;
	req.domain_id = domain_id;
	req.port_id = port_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.owned;
}

int hqmv2_vf_dir_port_owned_by_domain(struct hqmv2_hw *hw,
				      u32 domain_id,
				      u32 port_id)
{
	struct hqmv2_mbox_dir_port_owned_by_domain_cmd_resp resp;
	struct hqmv2_mbox_dir_port_owned_by_domain_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_DIR_PORT_OWNED_BY_DOMAIN;
	req.domain_id = domain_id;
	req.port_id = port_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.owned;
}

int hqmv2_vf_get_sn_allocation(struct hqmv2_hw *hw, u32 group_id)
{
	struct hqmv2_mbox_get_sn_allocation_cmd_resp resp;
	struct hqmv2_mbox_get_sn_allocation_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_GET_SN_ALLOCATION;
	req.group_id = group_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.num;
}

int hqmv2_vf_get_cos_bw(struct hqmv2_hw *hw, u32 cos_id)
{
	struct hqmv2_mbox_get_cos_bw_cmd_resp resp;
	struct hqmv2_mbox_get_cos_bw_cmd_req req;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	req.hdr.type = HQMV2_MBOX_CMD_GET_COS_BW;
	req.cos_id = cos_id;

	ret = hqmv2_send_sync_mbox_cmd(hqmv2_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	hqmv2_vf_read_pf_mbox_resp(&hqmv2_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != HQMV2_MBOX_ST_SUCCESS) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()]: failed with mailbox error: %s\n",
			  __func__,
			  HQMV2_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.num;
}

/*********************************/
/****** HQMv2 VF Device Ops ******/
/*********************************/

struct hqmv2_device_ops hqmv2_vf_ops = {
	.map_pci_bar_space = hqmv2_vf_map_pci_bar_space,
	.unmap_pci_bar_space = hqmv2_vf_unmap_pci_bar_space,
	.mmap = hqmv2_vf_mmap,
	.inc_pm_refcnt = hqmv2_vf_pm_inc_refcnt,
	.dec_pm_refcnt = hqmv2_vf_pm_dec_refcnt,
	.pm_refcnt_status_suspended = hqmv2_vf_pm_status_suspended,
	.init_driver_state = hqmv2_vf_init_driver_state,
	.free_driver_state = hqmv2_vf_free_driver_state,
	.device_create = hqmv2_vf_device_create,
	.device_destroy = hqmv2_vf_device_destroy,
	.cdev_add = hqmv2_vf_cdev_add,
	.cdev_del = hqmv2_vf_cdev_del,
	.sysfs_create = hqmv2_vf_sysfs_create,
	.sysfs_destroy = hqmv2_vf_sysfs_destroy,
	.sysfs_reapply = hqmv2_vf_sysfs_reapply_configuration,
	.init_interrupts = hqmv2_vf_init_interrupts,
	.enable_ldb_cq_interrupts = hqmv2_vf_enable_ldb_cq_interrupts,
	.enable_dir_cq_interrupts = hqmv2_vf_enable_dir_cq_interrupts,
	.arm_cq_interrupt = hqmv2_vf_arm_cq_interrupt,
	.reinit_interrupts = hqmv2_vf_reinit_interrupts,
	.free_interrupts = hqmv2_vf_free_interrupts,
	.enable_pm = hqmv2_vf_enable_pm,
	.wait_for_device_ready = hqmv2_vf_wait_for_device_ready,
	.reset_device = hqmv2_vf_reset,
	.register_driver = hqmv2_vf_register_driver,
	.unregister_driver = hqmv2_vf_unregister_driver,
	.create_sched_domain = hqmv2_vf_create_sched_domain,
	.create_ldb_queue = hqmv2_vf_create_ldb_queue,
	.create_dir_queue = hqmv2_vf_create_dir_queue,
	.create_ldb_port = hqmv2_vf_create_ldb_port,
	.create_dir_port = hqmv2_vf_create_dir_port,
	.start_domain = hqmv2_vf_start_domain,
	.map_qid = hqmv2_vf_map_qid,
	.unmap_qid = hqmv2_vf_unmap_qid,
	.enable_ldb_port = hqmv2_vf_enable_ldb_port,
	.enable_dir_port = hqmv2_vf_enable_dir_port,
	.disable_ldb_port = hqmv2_vf_disable_ldb_port,
	.disable_dir_port = hqmv2_vf_disable_dir_port,
	.get_num_resources = hqmv2_vf_get_num_resources,
	.reset_domain = hqmv2_vf_reset_domain,
	.measure_perf = hqmv2_vf_measure_perf,
	.ldb_port_owned_by_domain = hqmv2_vf_ldb_port_owned_by_domain,
	.dir_port_owned_by_domain = hqmv2_vf_dir_port_owned_by_domain,
	.get_sn_allocation = hqmv2_vf_get_sn_allocation,
	.get_ldb_queue_depth = hqmv2_vf_get_ldb_queue_depth,
	.get_dir_queue_depth = hqmv2_vf_get_dir_queue_depth,
	.pending_port_unmaps = hqmv2_vf_pending_port_unmaps,
	.get_cos_bw = hqmv2_vf_get_cos_bw,
};
