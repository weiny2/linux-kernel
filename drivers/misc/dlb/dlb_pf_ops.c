// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include <linux/pm_runtime.h>

#include "dlb_intr.h"
#include "dlb_main.h"
#include "dlb_mbox.h"
#include "dlb_regs.h"
#include "dlb_resource.h"

/***********************************/
/****** Runtime PM management ******/
/***********************************/

static void dlb_pf_pm_inc_refcnt(struct pci_dev *pdev, bool resume)
{
	if (resume)
		/* Increment the device's usage count and immediately wake it
		 * if it was suspended.
		 */
		pm_runtime_get_sync(&pdev->dev);
	else
		pm_runtime_get_noresume(&pdev->dev);
}

static void dlb_pf_pm_dec_refcnt(struct pci_dev *pdev)
{
	/* Decrement the device's usage count and suspend it if the
	 * count reaches zero.
	 */
	pm_runtime_put_sync_suspend(&pdev->dev);
}

/********************************/
/****** PCI BAR management ******/
/********************************/

static void dlb_pf_unmap_pci_bar_space(struct dlb_dev *dlb_dev,
				       struct pci_dev *pdev)
{
	pci_iounmap(pdev, dlb_dev->hw.csr_kva);
	pci_iounmap(pdev, dlb_dev->hw.func_kva);
}

static int dlb_pf_map_pci_bar_space(struct dlb_dev *dlb_dev,
				    struct pci_dev *pdev)
{
	int ret;
	u32 reg;

	/* BAR 0: PF FUNC BAR space */
	dlb_dev->hw.func_kva = pci_iomap(pdev, 0, 0);
	dlb_dev->hw.func_phys_addr = pci_resource_start(pdev, 0);

	if (!dlb_dev->hw.func_kva) {
		dev_err(&pdev->dev, "Cannot iomap BAR 0 (size %llu)\n",
			pci_resource_len(pdev, 0));

		return -EIO;
	}

	dev_dbg(&pdev->dev, "BAR 0 iomem base: %p\n", dlb_dev->hw.func_kva);
	dev_dbg(&pdev->dev, "BAR 0 start: 0x%llx\n",
		pci_resource_start(pdev, 0));
	dev_dbg(&pdev->dev, "BAR 0 len: %llu\n", pci_resource_len(pdev, 0));

	/* BAR 2: PF CSR BAR space */
	dlb_dev->hw.csr_kva = pci_iomap(pdev, 2, 0);
	dlb_dev->hw.csr_phys_addr = pci_resource_start(pdev, 2);

	if (!dlb_dev->hw.csr_kva) {
		dev_err(&pdev->dev, "Cannot iomap BAR 2 (size %llu)\n",
			pci_resource_len(pdev, 2));

		ret = -EIO;
		goto pci_iomap_bar2_fail;
	}

	dev_dbg(&pdev->dev, "BAR 2 iomem base: %p\n", dlb_dev->hw.csr_kva);
	dev_dbg(&pdev->dev, "BAR 2 start: 0x%llx\n",
		pci_resource_start(pdev, 2));
	dev_dbg(&pdev->dev, "BAR 2 len: %llu\n", pci_resource_len(pdev, 2));

	/* Do a test register read. A failure here could be due to a BIOS or
	 * device enumeration issue.
	 */
	reg = DLB_CSR_RD(&dlb_dev->hw, DLB_SYS_TOTAL_VAS);
	if (reg != DLB_MAX_NUM_DOMAINS) {
		dev_err(&pdev->dev,
			"Test MMIO access error (read 0x%x, expected 0x%x)\n",
			reg, DLB_MAX_NUM_DOMAINS);
		ret = -EIO;
		goto mmio_read_fail;
	}

	return 0;

mmio_read_fail:
	pci_iounmap(pdev, dlb_dev->hw.csr_kva);
pci_iomap_bar2_fail:
	pci_iounmap(pdev, dlb_dev->hw.func_kva);

	return ret;
}

#define DLB_LDB_CQ_BOUND DLB_LDB_CQ_OFFS(DLB_MAX_NUM_LDB_PORTS)
#define DLB_DIR_CQ_BOUND DLB_DIR_CQ_OFFS(DLB_MAX_NUM_DIR_PORTS)
#define DLB_LDB_PC_BOUND DLB_LDB_PC_OFFS(DLB_MAX_NUM_LDB_PORTS)
#define DLB_DIR_PC_BOUND DLB_DIR_PC_OFFS(DLB_MAX_NUM_DIR_PORTS)

static int dlb_pf_mmap(struct file *f,
		       struct vm_area_struct *vma,
		       u32 domain_id)
{
	unsigned long bar_pgoff;
	unsigned long offset;
	struct dlb_dev *dev;
	struct page *page;
	pgprot_t pgprot;
	u32 port_id;

	dev = container_of(f->f_inode->i_cdev, struct dlb_dev, cdev);

	offset = vma->vm_pgoff << PAGE_SHIFT;

	if (offset >= DLB_LDB_CQ_BASE && offset < DLB_LDB_CQ_BOUND) {
		if ((vma->vm_end - vma->vm_start) != DLB_LDB_CQ_MAX_SIZE)
			return -EINVAL;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - DLB_LDB_CQ_BASE) / DLB_LDB_CQ_MAX_SIZE;

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

	} else if (offset >= DLB_DIR_CQ_BASE && offset < DLB_DIR_CQ_BOUND) {
		if ((vma->vm_end - vma->vm_start) != DLB_DIR_CQ_MAX_SIZE)
			return -EINVAL;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - DLB_DIR_CQ_BASE) / DLB_DIR_CQ_MAX_SIZE;

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

	} else if (offset >= DLB_LDB_PC_BASE && offset < DLB_LDB_PC_BOUND) {
		if ((vma->vm_end - vma->vm_start) != DLB_LDB_PC_MAX_SIZE)
			return -EINVAL;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - DLB_LDB_PC_BASE) / DLB_LDB_PC_MAX_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		page = virt_to_page(dev->ldb_port_mem[port_id].pc_base);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= DLB_DIR_PC_BASE && offset < DLB_DIR_PC_BOUND) {
		if ((vma->vm_end - vma->vm_start) != DLB_DIR_PC_MAX_SIZE)
			return -EINVAL;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - DLB_DIR_PC_BASE) / DLB_DIR_PC_MAX_SIZE;

		if (dev->ops->dir_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		page = virt_to_page(dev->dir_port_mem[port_id].pc_base);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= DLB_LDB_PP_BASE && offset < DLB_LDB_PP_BOUND) {
		if ((vma->vm_end - vma->vm_start) != DLB_LDB_PP_MAX_SIZE)
			return -EINVAL;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - DLB_LDB_PP_BASE) / DLB_LDB_PP_MAX_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		pgprot = pgprot_noncached(vma->vm_page_prot);

	} else if (offset >= DLB_DIR_PP_BASE && offset < DLB_DIR_PP_BOUND) {
		if ((vma->vm_end - vma->vm_start) != DLB_DIR_PP_MAX_SIZE)
			return -EINVAL;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - DLB_DIR_PP_BASE) / DLB_DIR_PP_MAX_SIZE;

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
				  bar_pgoff + vma->vm_pgoff,
				  vma->vm_end - vma->vm_start,
				  pgprot);
}

/*******************************/
/****** Mailbox callbacks ******/
/*******************************/

/* Return -1 if no interfaces in the range are supported, else return the
 * newest version.
 */
static int dlb_mbox_version_supported(u16 min, u16 max)
{
	/* Only version 1 exists at this time */
	if (min > DLB_MBOX_INTERFACE_VERSION)
		return -1;

	return DLB_MBOX_INTERFACE_VERSION;
}

static void dlb_mbox_cmd_register_fn(struct dlb_dev *dev,
				     int vf_id,
				     void *data)
{
	struct dlb_mbox_register_cmd_resp resp = { {0} };
	struct dlb_mbox_register_cmd_req *req = data;
	int ret;

	/* Given an interface version range ('min' to 'max', inclusive)
	 * requested by the VF driver:
	 * - If PF supports any versions in that range, it returns the newest
	 *   supported version.
	 * - Else PF responds with MBOX_ST_VERSION_MISMATCH
	 */
	ret = dlb_mbox_version_supported(req->min_interface_version,
					 req->max_interface_version);

	if (ret == -1) {
		resp.hdr.status = DLB_MBOX_ST_VERSION_MISMATCH;
		resp.interface_version = DLB_MBOX_INTERFACE_VERSION;

		goto done;
	}

	resp.interface_version = ret;

	dlb_lock_vf(&dev->hw, vf_id);

	/* The VF can re-register without sending an unregister mailbox
	 * command (for example if the guest OS crashes). To protect against
	 * this, reset any in-use resources assigned to the driver now.
	 */
	if (dlb_reset_vf(&dev->hw, vf_id))
		dev_err(dev->dlb_device, "[%s()] Internal error\n", __func__);

	dev->vf_registered[vf_id] = true;

	resp.pf_id = dev->id;
	resp.vf_id = vf_id;
	resp.is_auxiliary_vf = dev->child_id_state[vf_id].is_auxiliary_vf;
	resp.primary_vf_id = dev->child_id_state[vf_id].primary_vf_id;
	resp.hdr.status = DLB_MBOX_ST_SUCCESS;

done:
	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_unregister_fn(struct dlb_dev *dev,
				       int vf_id,
				       void *data)
{
	struct dlb_mbox_unregister_cmd_resp resp = { {0} };

	dev->vf_registered[vf_id] = false;

	if (dlb_reset_vf(&dev->hw, vf_id))
		dev_err(dev->dlb_device, "[%s()] Internal error\n", __func__);

	dlb_unlock_vf(&dev->hw, vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_get_num_resources_fn(struct dlb_dev *dev,
					      int vf_id,
					      void *data)
{
	struct dlb_mbox_get_num_resources_cmd_resp resp = { {0} };
	struct dlb_get_num_resources_args arg;
	int ret;

	ret = dlb_hw_get_num_resources(&dev->hw, &arg, true, vf_id);

	resp.num_sched_domains = arg.num_sched_domains;
	resp.num_ldb_queues = arg.num_ldb_queues;
	resp.num_ldb_ports = arg.num_ldb_ports;
	resp.num_dir_ports = arg.num_dir_ports;
	resp.num_atomic_inflights = arg.num_atomic_inflights;
	resp.max_contiguous_atomic_inflights =
		arg.max_contiguous_atomic_inflights;
	resp.num_hist_list_entries = arg.num_hist_list_entries;
	resp.max_contiguous_hist_list_entries =
		arg.max_contiguous_hist_list_entries;
	resp.num_ldb_credits = arg.num_ldb_credits;
	resp.max_contiguous_ldb_credits = arg.max_contiguous_ldb_credits;
	resp.num_dir_credits = arg.num_dir_credits;
	resp.max_contiguous_dir_credits = arg.max_contiguous_dir_credits;
	resp.num_ldb_credit_pools = arg.num_ldb_credit_pools;
	resp.num_dir_credit_pools = arg.num_dir_credit_pools;

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_create_sched_domain_fn(struct dlb_dev *dev,
						int vf_id,
						void *data)
{
	struct dlb_mbox_create_sched_domain_cmd_resp resp = { {0} };
	struct dlb_mbox_create_sched_domain_cmd_req *req = data;
	struct dlb_create_sched_domain_args hw_arg;
	struct dlb_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.num_ldb_queues = req->num_ldb_queues;
	hw_arg.num_ldb_ports = req->num_ldb_ports;
	hw_arg.num_dir_ports = req->num_dir_ports;
	hw_arg.num_hist_list_entries = req->num_hist_list_entries;
	hw_arg.num_atomic_inflights = req->num_atomic_inflights;
	hw_arg.num_ldb_credits = req->num_ldb_credits;
	hw_arg.num_dir_credits = req->num_dir_credits;
	hw_arg.num_ldb_credit_pools = req->num_ldb_credit_pools;
	hw_arg.num_dir_credit_pools = req->num_dir_credit_pools;

	ret = dlb_hw_create_sched_domain(&dev->hw,
					 &hw_arg,
					 &hw_response,
					 true,
					 vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_reset_sched_domain_fn(struct dlb_dev *dev,
					       int vf_id,
					       void *data)
{
	struct dlb_mbox_reset_sched_domain_cmd_resp resp = { {0} };
	struct dlb_mbox_reset_sched_domain_cmd_req *req = data;
	int ret;

	ret = dlb_reset_domain(&dev->hw, req->id, true, vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_create_ldb_pool_fn(struct dlb_dev *dev,
					    int vf_id,
					    void *data)
{
	struct dlb_mbox_create_credit_pool_cmd_resp resp = { {0} };
	struct dlb_mbox_create_credit_pool_cmd_req *req = data;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_create_ldb_pool_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.num_ldb_credits = req->num_credits;

	ret = dlb_hw_create_ldb_pool(&dev->hw,
				     req->domain_id,
				     &hw_arg,
				     &hw_response,
				     true,
				     vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_create_dir_pool_fn(struct dlb_dev *dev,
					    int vf_id,
					    void *data)
{
	struct dlb_mbox_create_credit_pool_cmd_resp resp = { {0} };
	struct dlb_mbox_create_credit_pool_cmd_req *req = data;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_create_dir_pool_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.num_dir_credits = req->num_credits;

	ret = dlb_hw_create_dir_pool(&dev->hw,
				     req->domain_id,
				     &hw_arg,
				     &hw_response,
				     true,
				     vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_create_ldb_queue_fn(struct dlb_dev *dev,
					     int vf_id,
					     void *data)
{
	struct dlb_mbox_create_ldb_queue_cmd_resp resp = { {0} };
	struct dlb_mbox_create_ldb_queue_cmd_req *req = data;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_create_ldb_queue_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.num_sequence_numbers = req->num_sequence_numbers;
	hw_arg.num_qid_inflights = req->num_qid_inflights;
	hw_arg.num_atomic_inflights = req->num_atomic_inflights;

	ret = dlb_hw_create_ldb_queue(&dev->hw,
				      req->domain_id,
				      &hw_arg,
				      &hw_response,
				      true,
				      vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_create_dir_queue_fn(struct dlb_dev *dev,
					     int vf_id,
					     void *data)
{
	struct dlb_mbox_create_dir_queue_cmd_resp resp = { {0} };
	struct dlb_mbox_create_dir_queue_cmd_req *req;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_create_dir_queue_args hw_arg;
	int ret;

	req = (struct dlb_mbox_create_dir_queue_cmd_req *)data;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	ret = dlb_hw_create_dir_queue(&dev->hw,
				      req->domain_id,
				      &hw_arg,
				      &hw_response,
				      true,
				      vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_create_ldb_port_fn(struct dlb_dev *dev,
					    int vf_id,
					    void *data)
{
	struct dlb_mbox_create_ldb_port_cmd_resp resp = { {0} };
	struct dlb_mbox_create_ldb_port_cmd_req *req = data;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_create_ldb_port_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.ldb_credit_pool_id = req->ldb_credit_pool_id;
	hw_arg.dir_credit_pool_id = req->dir_credit_pool_id;
	hw_arg.ldb_credit_high_watermark = req->ldb_credit_high_watermark;
	hw_arg.ldb_credit_low_watermark = req->ldb_credit_low_watermark;
	hw_arg.ldb_credit_quantum = req->ldb_credit_quantum;
	hw_arg.dir_credit_high_watermark = req->dir_credit_high_watermark;
	hw_arg.dir_credit_low_watermark = req->dir_credit_low_watermark;
	hw_arg.dir_credit_quantum = req->dir_credit_quantum;
	hw_arg.cq_depth = req->cq_depth;
	hw_arg.cq_history_list_size = req->cq_history_list_size;

	ret = dlb_hw_create_ldb_port(&dev->hw,
				     req->domain_id,
				     &hw_arg,
				     req->pop_count_address,
				     req->cq_base_address,
				     &hw_response,
				     true,
				     vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_create_dir_port_fn(struct dlb_dev *dev,
					    int vf_id,
					    void *data)
{
	struct dlb_mbox_create_dir_port_cmd_resp resp = { {0} };
	struct dlb_mbox_create_dir_port_cmd_req *req;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_create_dir_port_args hw_arg;
	int ret;

	req = (struct dlb_mbox_create_dir_port_cmd_req *)data;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.ldb_credit_pool_id = req->ldb_credit_pool_id;
	hw_arg.dir_credit_pool_id = req->dir_credit_pool_id;
	hw_arg.ldb_credit_high_watermark = req->ldb_credit_high_watermark;
	hw_arg.ldb_credit_low_watermark = req->ldb_credit_low_watermark;
	hw_arg.ldb_credit_quantum = req->ldb_credit_quantum;
	hw_arg.dir_credit_high_watermark = req->dir_credit_high_watermark;
	hw_arg.dir_credit_low_watermark = req->dir_credit_low_watermark;
	hw_arg.dir_credit_quantum = req->dir_credit_quantum;
	hw_arg.cq_depth = req->cq_depth;
	hw_arg.queue_id = req->queue_id;

	ret = dlb_hw_create_dir_port(&dev->hw,
				     req->domain_id,
				     &hw_arg,
				     req->pop_count_address,
				     req->cq_base_address,
				     &hw_response,
				     true,
				     vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_enable_ldb_port_fn(struct dlb_dev *dev,
					    int vf_id,
					    void *data)
{
	struct dlb_mbox_enable_ldb_port_cmd_resp resp = { {0} };
	struct dlb_mbox_enable_ldb_port_cmd_req *req = data;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_enable_ldb_port_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	ret = dlb_hw_enable_ldb_port(&dev->hw,
				     req->domain_id,
				     &hw_arg,
				     &hw_response,
				     true,
				     vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;

	resp.error_code = ret;
	resp.status = hw_response.status;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_disable_ldb_port_fn(struct dlb_dev *dev,
					     int vf_id,
					    void *data)
{
	struct dlb_mbox_disable_ldb_port_cmd_resp resp = { {0} };
	struct dlb_mbox_disable_ldb_port_cmd_req *req = data;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_disable_ldb_port_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	ret = dlb_hw_disable_ldb_port(&dev->hw,
				      req->domain_id,
				      &hw_arg,
				      &hw_response,
				      true,
				      vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_enable_dir_port_fn(struct dlb_dev *dev,
					    int vf_id,
					    void *data)
{
	struct dlb_mbox_enable_dir_port_cmd_resp resp = { {0} };
	struct dlb_mbox_enable_dir_port_cmd_req *req = data;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_enable_dir_port_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	ret = dlb_hw_enable_dir_port(&dev->hw,
				     req->domain_id,
				     &hw_arg,
				     &hw_response,
				     true,
				     vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_disable_dir_port_fn(struct dlb_dev *dev,
					     int vf_id,
					    void *data)
{
	struct dlb_mbox_disable_dir_port_cmd_resp resp = { {0} };
	struct dlb_mbox_disable_dir_port_cmd_req *req = data;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_disable_dir_port_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	ret = dlb_hw_disable_dir_port(&dev->hw,
				      req->domain_id,
				      &hw_arg,
				      &hw_response,
				      true,
				      vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_ldb_port_owned_by_domain_fn(struct dlb_dev *dev,
						     int vf_id,
						     void *data)
{
	struct dlb_mbox_ldb_port_owned_by_domain_cmd_resp resp = { {0} };
	struct dlb_mbox_ldb_port_owned_by_domain_cmd_req *req = data;
	int ret;

	ret = dlb_ldb_port_owned_by_domain(&dev->hw,
					   req->domain_id,
					   req->port_id,
					   true,
					   vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.owned = ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_dir_port_owned_by_domain_fn(struct dlb_dev *dev,
						     int vf_id,
						     void *data)
{
	struct dlb_mbox_dir_port_owned_by_domain_cmd_resp resp = { {0} };
	struct dlb_mbox_dir_port_owned_by_domain_cmd_req *req = data;
	int ret;

	ret = dlb_dir_port_owned_by_domain(&dev->hw,
					   req->domain_id,
					   req->port_id,
					   true,
					   vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.owned = ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_map_qid_fn(struct dlb_dev *dev,
				    int vf_id,
				    void *data)
{
	struct dlb_mbox_map_qid_cmd_resp resp = { {0} };
	struct dlb_mbox_map_qid_cmd_req *req = data;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_map_qid_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;
	hw_arg.qid = req->qid;
	hw_arg.priority = req->priority;

	ret = dlb_hw_map_qid(&dev->hw,
			     req->domain_id,
			     &hw_arg,
			     &hw_response,
			     true,
			     vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_unmap_qid_fn(struct dlb_dev *dev,
				      int vf_id,
				      void *data)
{
	struct dlb_mbox_unmap_qid_cmd_resp resp = { {0} };
	struct dlb_mbox_unmap_qid_cmd_req *req = data;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_unmap_qid_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;
	hw_arg.qid = req->qid;

	ret = dlb_hw_unmap_qid(&dev->hw,
			       req->domain_id,
			       &hw_arg,
			       &hw_response,
			       true,
			       vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_start_domain_fn(struct dlb_dev *dev,
					 int vf_id,
					 void *data)
{
	struct dlb_mbox_start_domain_cmd_resp resp = { {0} };
	struct dlb_mbox_start_domain_cmd_req *req = data;
	struct dlb_cmd_response hw_response = {0};
	struct dlb_start_domain_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;

	ret = dlb_hw_start_domain(&dev->hw,
				  req->domain_id,
				  &hw_arg,
				  &hw_response,
				  true,
				  vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_enable_ldb_port_intr_fn(struct dlb_dev *dev,
						 int vf_id,
						 void *data)
{
	struct dlb_mbox_enable_ldb_port_intr_cmd_resp resp = { {0} };
	struct dlb_mbox_enable_ldb_port_intr_cmd_req *req = data;
	int ret;

	if (req->owner_vf >= DLB_MAX_NUM_VFS ||
	    (dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     dev->child_id_state[req->owner_vf].primary_vf_id != vf_id) ||
	    (!dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     req->owner_vf != vf_id)) {
		resp.hdr.status = DLB_MBOX_ST_INVALID_OWNER_VF;
		goto finish;
	}

	ret = dlb_configure_ldb_cq_interrupt(&dev->hw,
					     req->port_id,
					     req->vector,
					     DLB_CQ_ISR_MODE_MSI,
					     vf_id,
					     req->owner_vf,
					     req->thresh);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;

finish:
	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_enable_dir_port_intr_fn(struct dlb_dev *dev,
						 int vf_id,
						 void *data)
{
	struct dlb_mbox_enable_dir_port_intr_cmd_resp resp = { {0} };
	struct dlb_mbox_enable_dir_port_intr_cmd_req *req = data;
	int ret;

	if (req->owner_vf >= DLB_MAX_NUM_VFS ||
	    (dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     dev->child_id_state[req->owner_vf].primary_vf_id != vf_id) ||
	    (!dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     req->owner_vf != vf_id)) {
		resp.hdr.status = DLB_MBOX_ST_INVALID_OWNER_VF;
		goto finish;
	}

	ret = dlb_configure_dir_cq_interrupt(&dev->hw,
					     req->port_id,
					     req->vector,
					     DLB_CQ_ISR_MODE_MSI,
					     vf_id,
					     req->owner_vf,
					     req->thresh);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;

finish:
	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_arm_cq_intr_fn(struct dlb_dev *dev,
					int vf_id,
					void *data)
{
	struct dlb_mbox_arm_cq_intr_cmd_resp resp = { {0} };
	struct dlb_mbox_arm_cq_intr_cmd_req *req = data;
	int ret;

	if (req->is_ldb)
		ret = dlb_ldb_port_owned_by_domain(&dev->hw,
						   req->domain_id,
						   req->port_id,
						   true,
						   vf_id);
	else
		ret = dlb_dir_port_owned_by_domain(&dev->hw,
						   req->domain_id,
						   req->port_id,
						   true,
						   vf_id);

	if (ret != 1) {
		resp.error_code = -EINVAL;
		goto finish;
	}

	resp.error_code = dlb_arm_cq_interrupt(&dev->hw,
					       req->port_id,
					       req->is_ldb,
					       true,
					       vf_id);

finish:
	resp.hdr.status = DLB_MBOX_ST_SUCCESS;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_get_num_used_resources_fn(struct dlb_dev *dev,
						   int vf_id,
						   void *data)
{
	struct dlb_mbox_get_num_resources_cmd_resp resp = { {0} };
	struct dlb_get_num_resources_args arg;
	int ret;

	ret = dlb_hw_get_num_used_resources(&dev->hw, &arg, true, vf_id);

	resp.num_sched_domains = arg.num_sched_domains;
	resp.num_ldb_queues = arg.num_ldb_queues;
	resp.num_ldb_ports = arg.num_ldb_ports;
	resp.num_dir_ports = arg.num_dir_ports;
	resp.num_atomic_inflights = arg.num_atomic_inflights;
	resp.max_contiguous_atomic_inflights =
		arg.max_contiguous_atomic_inflights;
	resp.num_hist_list_entries = arg.num_hist_list_entries;
	resp.max_contiguous_hist_list_entries =
		arg.max_contiguous_hist_list_entries;
	resp.num_ldb_credits = arg.num_ldb_credits;
	resp.max_contiguous_ldb_credits = arg.max_contiguous_ldb_credits;
	resp.num_dir_credits = arg.num_dir_credits;
	resp.max_contiguous_dir_credits = arg.max_contiguous_dir_credits;
	resp.num_ldb_credit_pools = arg.num_ldb_credit_pools;
	resp.num_dir_credit_pools = arg.num_dir_credit_pools;

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_ack_vf_flr_done(struct dlb_dev *dev,
					 int vf_id,
					 void *data)
{
	struct dlb_mbox_ack_vf_flr_done_cmd_resp resp = { {0} };

	/* Once the VF has observed the cleared reset-in-progress bit, it has
	 * to alert the PF driver to reset it. Workaround for A-stepping
	 * devices only.
	 */
	dlb_set_vf_reset_in_progress(&dev->hw, vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = 0;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_get_sn_allocation_fn(struct dlb_dev *dev,
					      int vf_id,
					      void *data)
{
	struct dlb_mbox_get_sn_allocation_cmd_resp resp = { {0} };
	struct dlb_mbox_get_sn_allocation_cmd_req *req = data;

	resp.num = dlb_get_group_sequence_numbers(&dev->hw, req->group_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_get_sn_occupancy_fn(struct dlb_dev *dev,
					     int vf_id,
					     void *data)
{
	struct dlb_mbox_get_sn_occupancy_cmd_resp resp = { {0} };
	struct dlb_mbox_get_sn_occupancy_cmd_req *req = data;

	resp.num = dlb_get_group_sequence_number_occupancy(&dev->hw,
							   req->group_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_get_ldb_queue_depth_fn(struct dlb_dev *dev,
						int vf_id,
						void *data)
{
	struct dlb_mbox_get_ldb_queue_depth_cmd_resp resp = { {0} };
	struct dlb_mbox_get_ldb_queue_depth_cmd_req *req = data;
	struct dlb_get_ldb_queue_depth_args hw_arg;
	struct dlb_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.queue_id = req->queue_id;

	ret = dlb_hw_get_ldb_queue_depth(&dev->hw,
					 req->domain_id,
					 &hw_arg,
					 &hw_response,
					 true,
					 vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.depth = hw_response.id;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_get_dir_queue_depth_fn(struct dlb_dev *dev,
						int vf_id,
						void *data)
{
	struct dlb_mbox_get_dir_queue_depth_cmd_resp resp = { {0} };
	struct dlb_mbox_get_dir_queue_depth_cmd_req *req = data;
	struct dlb_get_dir_queue_depth_args hw_arg;
	struct dlb_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.queue_id = req->queue_id;

	ret = dlb_hw_get_dir_queue_depth(&dev->hw,
					 req->domain_id,
					 &hw_arg,
					 &hw_response,
					 true,
					 vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.depth = hw_response.id;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void dlb_mbox_cmd_pending_port_unmaps_fn(struct dlb_dev *dev,
						int vf_id,
						void *data)
{
	struct dlb_mbox_pending_port_unmaps_cmd_resp resp = { {0} };
	struct dlb_mbox_pending_port_unmaps_cmd_req *req = data;
	struct dlb_pending_port_unmaps_args hw_arg;
	struct dlb_cmd_response hw_response = {0};
	int ret;

	hw_arg.port_id = req->port_id;

	ret = dlb_hw_pending_port_unmaps(&dev->hw,
					 req->domain_id,
					 &hw_arg,
					 &hw_response,
					 true,
					 vf_id);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.num = hw_response.id;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static int dlb_pf_query_cq_poll_mode(struct dlb_dev *dlb_dev,
				     struct dlb_cmd_response *user_resp);

static void dlb_mbox_cmd_query_cq_poll_mode_fn(struct dlb_dev *dev,
					       int vf_id,
					       void *data)
{
	struct dlb_mbox_query_cq_poll_mode_cmd_resp resp = { {0} };
	struct dlb_cmd_response response = {0};
	int ret;

	ret = dlb_pf_query_cq_poll_mode(dev, &response);

	resp.hdr.status = DLB_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = response.status;
	resp.mode = response.id;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void (*mbox_fn_table[])(struct dlb_dev *dev, int vf_id, void *data) = {
	dlb_mbox_cmd_register_fn,
	dlb_mbox_cmd_unregister_fn,
	dlb_mbox_cmd_get_num_resources_fn,
	dlb_mbox_cmd_create_sched_domain_fn,
	dlb_mbox_cmd_reset_sched_domain_fn,
	dlb_mbox_cmd_create_ldb_pool_fn,
	dlb_mbox_cmd_create_dir_pool_fn,
	dlb_mbox_cmd_create_ldb_queue_fn,
	dlb_mbox_cmd_create_dir_queue_fn,
	dlb_mbox_cmd_create_ldb_port_fn,
	dlb_mbox_cmd_create_dir_port_fn,
	dlb_mbox_cmd_enable_ldb_port_fn,
	dlb_mbox_cmd_disable_ldb_port_fn,
	dlb_mbox_cmd_enable_dir_port_fn,
	dlb_mbox_cmd_disable_dir_port_fn,
	dlb_mbox_cmd_ldb_port_owned_by_domain_fn,
	dlb_mbox_cmd_dir_port_owned_by_domain_fn,
	dlb_mbox_cmd_map_qid_fn,
	dlb_mbox_cmd_unmap_qid_fn,
	dlb_mbox_cmd_start_domain_fn,
	dlb_mbox_cmd_enable_ldb_port_intr_fn,
	dlb_mbox_cmd_enable_dir_port_intr_fn,
	dlb_mbox_cmd_arm_cq_intr_fn,
	dlb_mbox_cmd_get_num_used_resources_fn,
	dlb_mbox_cmd_ack_vf_flr_done,
	dlb_mbox_cmd_get_sn_allocation_fn,
	dlb_mbox_cmd_get_ldb_queue_depth_fn,
	dlb_mbox_cmd_get_dir_queue_depth_fn,
	dlb_mbox_cmd_pending_port_unmaps_fn,
	dlb_mbox_cmd_query_cq_poll_mode_fn,
	dlb_mbox_cmd_get_sn_occupancy_fn,
};

static u32 dlb_handle_vf_flr_interrupt(struct dlb_dev *dev)
{
	u32 bitvec;
	int i;

	bitvec = dlb_read_vf_flr_int_bitvec(&dev->hw);

	for (i = 0; i < DLB_MAX_NUM_VFS; i++) {
		if (!(bitvec & (1 << i)))
			continue;

		dev_dbg(dev->dlb_device,
			"Received VF FLR ISR from VF %d\n",
			i);

		if (dlb_reset_vf(&dev->hw, i))
			dev_err(dev->dlb_device,
				"[%s()] Internal error\n",
				__func__);
	}

	dlb_ack_vf_flr_int(&dev->hw, bitvec, dev->revision < DLB_REV_B0);

	return bitvec;
}

/**********************************/
/****** Interrupt management ******/
/**********************************/

static u32 dlb_handle_vf_to_pf_interrupt(struct dlb_dev *dev)
{
	u8 data[DLB_VF2PF_REQ_BYTES];
	u32 bitvec;
	int i;

	bitvec = dlb_read_vf_to_pf_int_bitvec(&dev->hw);

	for (i = 0; i < DLB_MAX_NUM_VFS; i++) {
		if (!(bitvec & (1 << i)))
			continue;

		dev_dbg(dev->dlb_device,
			"Received VF->PF ISR from VF %d\n",
			i);

		BUILD_BUG_ON(sizeof(data) > DLB_VF2PF_REQ_BYTES);

		dlb_pf_read_vf_mbox_req(&dev->hw, i, data, sizeof(data));

		/* Unrecognized request command, send an error response */
		if (DLB_MBOX_CMD_TYPE(data) >= NUM_DLB_MBOX_CMD_TYPES) {
			struct dlb_mbox_resp_hdr resp = {0};

			resp.status = DLB_MBOX_ST_INVALID_CMD_TYPE;

			BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

			dlb_pf_write_vf_mbox_resp(&dev->hw,
						  i,
						  &resp,
						  sizeof(resp));

			continue;
		}

		dev_dbg(dev->dlb_device,
			"Received mbox command %s\n",
			DLB_MBOX_CMD_STRING(data));

		mbox_fn_table[DLB_MBOX_CMD_TYPE(data)](dev, i, data);
	}

	dlb_ack_vf_mbox_int(&dev->hw, bitvec);

	return bitvec;
}

static u32 dlb_handle_vf_requests(struct dlb_dev *dev)
{
	u32 mbox_bitvec;
	u32 flr_bitvec;

	flr_bitvec = dlb_handle_vf_flr_interrupt(dev);

	mbox_bitvec = dlb_handle_vf_to_pf_interrupt(dev);

	dlb_ack_vf_to_pf_int(&dev->hw, mbox_bitvec, flr_bitvec);

	return mbox_bitvec | flr_bitvec;
}

static void dlb_enable_alarms(struct dlb_dev *dev)
{
	dlb_hw_enable_pp_sw_alarms(&dev->hw);

	dlb_enable_excess_tokens_alarm(&dev->hw);

	dlb_enable_alarm_interrupts(&dev->hw);
}

static void dlb_disable_alarms(struct dlb_dev *dev)
{
	dlb_hw_disable_pp_sw_alarms(&dev->hw);

	dlb_disable_excess_tokens_alarm(&dev->hw);

	dlb_disable_alarm_interrupts(&dev->hw);
}

static void dlb_detect_ingress_err_overload(struct dlb_dev *dev)
{
	s64 delta_us;

	if (dev->ingress_err.count == 0)
		dev->ingress_err.ts = ktime_get();

	dev->ingress_err.count++;

	/* Don't check for overload until OVERLOAD_THRESH ISRs have run */
	if (dev->ingress_err.count < DLB_ISR_OVERLOAD_THRESH)
		return;

	delta_us = ktime_us_delta(ktime_get(), dev->ingress_err.ts);

	/* Reset stats for next measurement period */
	dev->ingress_err.count = 0;
	dev->ingress_err.ts = ktime_get();

	/* Check for overload during this measurement period */
	if (delta_us > DLB_ISR_OVERLOAD_PERIOD_S * USEC_PER_SEC)
		return;

	/* Alarm interrupt overload: disable software-generated alarms,
	 * so only hardware problems (e.g. ECC errors) interrupt the PF.
	 */
	dlb_disable_alarms(dev);

	dev->ingress_err.enabled = false;

	dev_err(dev->dlb_device,
		"[%s()] Overloaded detected: disabling ingress error interrupts",
		__func__);
}

static void dlb_detect_mbox_overload(struct dlb_dev *dev, int id)
{
	union dlb_sys_func_vf_bar_dsbl r0 = { {0} };
	s64 delta_us;

	if (dev->mbox[id].count == 0)
		dev->mbox[id].ts = ktime_get();

	dev->mbox[id].count++;

	/* Don't check for overload until OVERLOAD_THRESH ISRs have run */
	if (dev->mbox[id].count < DLB_ISR_OVERLOAD_THRESH)
		return;

	delta_us = ktime_us_delta(ktime_get(), dev->mbox[id].ts);

	/* Reset stats for next measurement period */
	dev->mbox[id].count = 0;
	dev->mbox[id].ts = ktime_get();

	/* Check for overload during this measurement period */
	if (delta_us > DLB_ISR_OVERLOAD_PERIOD_S * USEC_PER_SEC)
		return;

	/* Mailbox interrupt overload: disable the VF FUNC BAR to prevent
	 * further abuse. The FUNC BAR is re-enabled when the device is reset
	 * or the driver is reloaded.
	 */
	r0.field.func_vf_bar_dis = 1;

	DLB_CSR_WR(&dev->hw, DLB_SYS_FUNC_VF_BAR_DSBL(id), r0.val);

	dev->mbox[id].enabled = false;

	dev_err(dev->dlb_device,
		"[%s()] Overloaded detected: disabling VF %d's FUNC BAR",
		__func__, id);
}

/* The alarm handler logs the alarm syndrome and, for user-caused errors,
 * reports the alarm to user-space through the per-domain device file interface.
 *
 * This function runs as a bottom-half handler because it can call printk
 * and/or acquire a mutex. These alarms don't need to be handled immediately --
 * they represent a serious, unexpected error (either in hardware or software)
 * that can't be recovered without restarting the application or resetting the
 * device. The VF->PF operations are also non-trivial and require running in a
 * bottom-half handler.
 */
static irqreturn_t dlb_alarm_handler(int irq, void *hdlr_ptr)
{
	struct dlb_dev *dev = (struct dlb_dev *)hdlr_ptr;
	u32 bitvec;
	int id, i;

	id = irq - dev->intr.base_vector;

	dev_dbg(dev->dlb_device, "DLB alarm %d fired\n", id);

	mutex_lock(&dev->resource_mutex);

	switch (id) {
	case DLB_INT_ALARM:
		dlb_process_alarm_interrupt(&dev->hw);
		break;
	case DLB_INT_VF_TO_PF_MBOX:
		bitvec = dlb_handle_vf_requests(dev);
		for (i = 0; i < DLB_MAX_NUM_VFS; i++) {
			if (bitvec & (1 << i))
				dlb_detect_mbox_overload(dev, i);
		}
		break;
	case DLB_INT_INGRESS_ERROR:
		dlb_process_ingress_error_interrupt(&dev->hw);
		dlb_detect_ingress_err_overload(dev);
		break;
	default:
		dev_err(dev->dlb_device,
			"[%s()] Internal error: unexpected IRQ", __func__);
	}

	mutex_unlock(&dev->resource_mutex);

	return IRQ_HANDLED;
}

static const char *alarm_hdlr_names[DLB_PF_NUM_ALARM_INTERRUPT_VECTORS] = {
	[DLB_INT_ALARM]		= "dlb_alarm",
	[DLB_INT_VF_TO_PF_MBOX]	= "dlb_vf_to_pf_mbox",
	[DLB_INT_INGRESS_ERROR]	= "dlb_ingress_err",
};

static int dlb_init_alarm_interrupts(struct dlb_dev *dev, struct pci_dev *pdev)
{
	int i, ret;

	for (i = 0; i < DLB_PF_NUM_ALARM_INTERRUPT_VECTORS; i++) {
		ret = devm_request_threaded_irq(&pdev->dev,
						pci_irq_vector(pdev, i),
						NULL,
						dlb_alarm_handler,
						IRQF_ONESHOT,
						alarm_hdlr_names[i],
						dev);
		if (ret)
			return ret;

		dev->intr.isr_registered[i] = true;
	}

	dlb_enable_alarms(dev);

	return 0;
}

static irqreturn_t dlb_compressed_cq_intr_handler(int irq, void *hdlr_ptr)
{
	struct dlb_dev *dev = (struct dlb_dev *)hdlr_ptr;
	u32 ldb_cq_interrupts[DLB_MAX_NUM_LDB_PORTS / 32];
	u32 dir_cq_interrupts[DLB_MAX_NUM_DIR_PORTS / 32];
	int i;

	dev_dbg(dev->dlb_device, "Entered ISR\n");

	dlb_read_compressed_cq_intr_status(&dev->hw,
					   ldb_cq_interrupts,
					   dir_cq_interrupts);

	dlb_ack_compressed_cq_intr(&dev->hw,
				   ldb_cq_interrupts,
				   dir_cq_interrupts);

	for (i = 0; i < DLB_MAX_NUM_LDB_PORTS; i++) {
		if (!(ldb_cq_interrupts[i / 32] & (1 << (i % 32))))
			continue;

		dev_dbg(dev->dlb_device, "[%s()] Waking LDB port %d\n",
			__func__, i);

		dlb_wake_thread(dev, &dev->intr.ldb_cq_intr[i], WAKE_CQ_INTR);
	}

	for (i = 0; i < DLB_MAX_NUM_DIR_PORTS; i++) {
		if (!(dir_cq_interrupts[i / 32] & (1 << (i % 32))))
			continue;

		dev_dbg(dev->dlb_device, "[%s()] Waking DIR port %d\n",
			__func__, i);

		dlb_wake_thread(dev, &dev->intr.dir_cq_intr[i], WAKE_CQ_INTR);
	}

	return IRQ_HANDLED;
}

static int dlb_init_compressed_mode_interrupts(struct dlb_dev *dev,
					       struct pci_dev *pdev)
{
	int ret, irq;

	irq = pci_irq_vector(pdev, DLB_PF_COMPRESSED_MODE_CQ_VECTOR_ID);

	ret = devm_request_irq(&pdev->dev,
			       irq,
			       dlb_compressed_cq_intr_handler,
			       0,
			       "dlb_compressed_cq",
			       dev);
	if (ret)
		return ret;

	dev->intr.isr_registered[DLB_PF_COMPRESSED_MODE_CQ_VECTOR_ID] = true;

	dev->intr.mode = DLB_MSIX_MODE_COMPRESSED;

	dlb_set_msix_mode(&dev->hw, DLB_MSIX_MODE_COMPRESSED);

	return 0;
}

static void dlb_pf_free_interrupts(struct dlb_dev *dev, struct pci_dev *pdev)
{
	int i;

	for (i = 0; i < dev->intr.num_vectors; i++) {
		if (dev->intr.isr_registered[i])
			devm_free_irq(&pdev->dev, pci_irq_vector(pdev, i), dev);
	}

	pci_free_irq_vectors(pdev);
}

static int dlb_pf_init_interrupts(struct dlb_dev *dev, struct pci_dev *pdev)
{
	int ret, i, nvecs;

	/* DLB supports two modes for CQ interrupts:
	 * - "compressed mode": all CQ interrupts are packed into a single
	 *	vector. The ISR reads six interrupt status registers to
	 *	determine the source(s).
	 * - "packed mode" (unused): the hardware supports up to 64 vectors. If
	 *	software requests more than 64 CQs to use interrupts, the
	 *	driver will pack CQs onto the same vector. The application
	 *	thread is responsible for checking the CQ depth when it is
	 *	woken and returns to user-space, in case its CQ shares the
	 *	vector and didn't cause the interrupt.
	 */

	nvecs = DLB_PF_NUM_COMPRESSED_MODE_VECTORS;

	ret = pci_alloc_irq_vectors(pdev, nvecs, nvecs, PCI_IRQ_MSIX);
	if (ret < 0)
		return ret;

	dev->intr.num_vectors = ret;
	dev->intr.base_vector = pci_irq_vector(pdev, 0);

	ret = dlb_init_alarm_interrupts(dev, pdev);
	if (ret) {
		dlb_pf_free_interrupts(dev, pdev);
		return ret;
	}

	ret = dlb_init_compressed_mode_interrupts(dev, pdev);
	if (ret) {
		dlb_pf_free_interrupts(dev, pdev);
		return ret;
	}

	/* Initialize per-CQ interrupt structures, such as wait queues
	 * that threads will wait on until the CQ's interrupt fires.
	 */
	for (i = 0; i < DLB_MAX_NUM_LDB_PORTS; i++) {
		init_waitqueue_head(&dev->intr.ldb_cq_intr[i].wq_head);
		mutex_init(&dev->intr.ldb_cq_intr[i].mutex);
	}

	for (i = 0; i < DLB_MAX_NUM_DIR_PORTS; i++) {
		init_waitqueue_head(&dev->intr.dir_cq_intr[i].wq_head);
		mutex_init(&dev->intr.dir_cq_intr[i].mutex);
	}

	return 0;
}

/* If the device is reset during use, its interrupt registers need to be
 * reinitialized.
 */
static void dlb_pf_reinit_interrupts(struct dlb_dev *dev)
{
	int i;

	/* Re-enable alarms after device reset */
	dlb_enable_alarms(dev);

	if (!dev->ingress_err.enabled)
		dev_err(dev->dlb_device,
			"[%s()] Re-enabling ingress error interrupts",
			__func__);

	dev->ingress_err.enabled = true;

	for (i = 0; i < DLB_MAX_NUM_VFS; i++) {
		if (!dev->mbox[i].enabled)
			dev_err(dev->dlb_device,
				"[%s()] Re-enabling VF %d's FUNC BAR",
				__func__, i);

		dev->mbox[i].enabled = true;
	}

	dlb_set_msix_mode(&dev->hw, dev->intr.mode);
}

static int dlb_pf_enable_ldb_cq_interrupts(struct dlb_dev *dev,
					   int id,
					   u16 thresh)
{
	int mode, vec;

	if (dev->intr.mode == DLB_MSIX_MODE_COMPRESSED) {
		mode = DLB_CQ_ISR_MODE_MSIX;
		vec = 0;
	} else {
		mode = DLB_CQ_ISR_MODE_MSIX;
		vec = fls64(~dev->intr.packed_vector_bitmap) - 1;
		dev->intr.packed_vector_bitmap |= (u64)1 << vec;
	}

	dev->intr.ldb_cq_intr[id].disabled = false;
	dev->intr.ldb_cq_intr[id].configured = true;
	dev->intr.ldb_cq_intr[id].vector = vec;

	return dlb_configure_ldb_cq_interrupt(&dev->hw, id, vec,
					      mode, 0, 0, thresh);
}

static int dlb_pf_enable_dir_cq_interrupts(struct dlb_dev *dev,
					   int id,
					   u16 thresh)
{
	int mode, vec;

	if (dev->intr.mode == DLB_MSIX_MODE_COMPRESSED) {
		mode = DLB_CQ_ISR_MODE_MSIX;
		vec = 0;
	} else {
		mode = DLB_CQ_ISR_MODE_MSIX;
		vec = fls64(~dev->intr.packed_vector_bitmap) - 1;
		dev->intr.packed_vector_bitmap |= (u64)1 << vec;
	}

	dev->intr.dir_cq_intr[id].disabled = false;
	dev->intr.dir_cq_intr[id].configured = true;
	dev->intr.dir_cq_intr[id].vector = vec;

	return dlb_configure_dir_cq_interrupt(&dev->hw, id, vec,
					      mode, 0, 0, thresh);
}

static int dlb_pf_arm_cq_interrupt(struct dlb_dev *dev,
				   int domain_id,
				   int port_id,
				   bool is_ldb)
{
	int ret;

	if (is_ldb)
		ret = dev->ops->ldb_port_owned_by_domain(&dev->hw,
							 domain_id,
							 port_id);
	else
		ret = dev->ops->dir_port_owned_by_domain(&dev->hw,
							 domain_id,
							 port_id);

	if (ret != 1)
		return -EINVAL;

	return dlb_arm_cq_interrupt(&dev->hw, port_id, is_ldb, false, 0);
}

/*******************************/
/****** Driver management ******/
/*******************************/

static int dlb_pf_init_driver_state(struct dlb_dev *dlb_dev)
{
	int i;

	dlb_dev->wq = create_singlethread_workqueue("DLB queue remapper");
	if (!dlb_dev->wq)
		return -EINVAL;

	/* Initialize software state */
	for (i = 0; i < DLB_MAX_NUM_DOMAINS; i++) {
		struct dlb_domain_dev *domain = &dlb_dev->sched_domains[i];

		mutex_init(&domain->alert_mutex);
		init_waitqueue_head(&domain->wq_head);
	}

	for (i = 0; i < DLB_MAX_NUM_VFS; i++)
		dlb_dev->child_id_state[i].is_auxiliary_vf = false;

	mutex_init(&dlb_dev->resource_mutex);

	dlb_dev->ingress_err.count = 0;
	dlb_dev->ingress_err.enabled = true;

	for (i = 0; i < DLB_MAX_NUM_VFS; i++) {
		dlb_dev->mbox[i].count = 0;
		dlb_dev->mbox[i].enabled = true;
	}

	return 0;
}

static void dlb_pf_free_driver_state(struct dlb_dev *dlb_dev)
{
	destroy_workqueue(dlb_dev->wq);
}

static int dlb_pf_cdev_add(struct dlb_dev *dlb_dev,
			   dev_t base,
			   const struct file_operations *fops)
{
	int ret;

	dlb_dev->dev_number = MKDEV(MAJOR(base),
				    MINOR(base) +
				    (dlb_dev->id *
				     DLB_NUM_DEV_FILES_PER_DEVICE));

	cdev_init(&dlb_dev->cdev, fops);

	dlb_dev->cdev.dev   = dlb_dev->dev_number;
	dlb_dev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&dlb_dev->cdev,
		       dlb_dev->cdev.dev,
		       DLB_NUM_DEV_FILES_PER_DEVICE);

	if (ret < 0)
		dev_err(dlb_dev->dlb_device,
			"%s: cdev_add() returned %d\n",
			dlb_driver_name, ret);

	return ret;
}

static void dlb_pf_cdev_del(struct dlb_dev *dlb_dev)
{
	cdev_del(&dlb_dev->cdev);
}

static int dlb_pf_device_create(struct dlb_dev *dlb_dev,
				struct pci_dev *pdev,
				struct class *dlb_class)
{
	dev_t dev;

	dev = MKDEV(MAJOR(dlb_dev->dev_number),
		    MINOR(dlb_dev->dev_number) + DLB_MAX_NUM_DOMAINS);

	/* Create a new device in order to create a /dev/ dlb node. This device
	 * is a child of the DLB PCI device.
	 */
	dlb_dev->dlb_device = device_create(dlb_class,
					    &pdev->dev,
					    dev,
					    dlb_dev,
					    "dlb%d/dlb",
					    dlb_dev->id);

	if (IS_ERR_VALUE(PTR_ERR(dlb_dev->dlb_device))) {
		dev_err(dlb_dev->dlb_device,
			"%s: device_create() returned %ld\n",
			dlb_driver_name, PTR_ERR(dlb_dev->dlb_device));

		return PTR_ERR(dlb_dev->dlb_device);
	}

	return 0;
}

static void dlb_pf_device_destroy(struct dlb_dev *dlb_dev,
				  struct class *dlb_class)
{
	device_destroy(dlb_class,
		       MKDEV(MAJOR(dlb_dev->dev_number),
			     MINOR(dlb_dev->dev_number) +
				DLB_MAX_NUM_DOMAINS));
}

static int dlb_pf_register_driver(struct dlb_dev *dlb_dev)
{
	/* Function intentionally left blank */
	return 0;
}

static void dlb_pf_unregister_driver(struct dlb_dev *dlb_dev)
{
	/* Function intentionally left blank */
}

static void dlb_pf_calc_arbiter_weights(struct dlb_hw *hw,
					u8 *weight,
					unsigned int pct)
{
	int val, i;

	/* Largest possible weight (100% SA case): 32 */
	val = (DLB_MAX_WEIGHT + 1) / DLB_NUM_ARB_WEIGHTS;

	/* Scale val according to the starvation avoidance percentage */
	val = (val * pct) / 100;
	if (val == 0 && pct != 0)
		val = 1;

	/* Prio 7 always has weight 0xff */
	weight[DLB_NUM_ARB_WEIGHTS - 1] = DLB_MAX_WEIGHT;

	for (i = DLB_NUM_ARB_WEIGHTS - 2; i >= 0; i--)
		weight[i] = weight[i + 1] - val;
}

static bool dlb_sparse_cq_mode_enabled;

static void dlb_pf_init_hardware(struct dlb_dev *dlb_dev)
{
	int i;

	dlb_disable_dp_vasr_feature(&dlb_dev->hw);

	if (dlb_dev->revision < DLB_REV_B0) {
		for (i = 0; i < pci_num_vf(dlb_dev->pdev); i++)
			dlb_set_vf_reset_in_progress(&dlb_dev->hw, i);
	}

	dlb_sparse_cq_mode_enabled = dlb_dev->revision >= DLB_REV_B0;

	if (dlb_sparse_cq_mode_enabled) {
		dlb_hw_enable_sparse_ldb_cq_mode(&dlb_dev->hw);
		dlb_hw_enable_sparse_dir_cq_mode(&dlb_dev->hw);
	}

	/* Configure arbitration weights for QE selection */
	if (dlb_qe_sa_pct <= 100) {
		u8 weight[DLB_NUM_ARB_WEIGHTS];

		dlb_pf_calc_arbiter_weights(&dlb_dev->hw,
					    weight,
					    dlb_qe_sa_pct);

		dlb_hw_set_qe_arbiter_weights(&dlb_dev->hw, weight);
	}

	/* Configure arbitration weights for QID selection */
	if (dlb_qid_sa_pct <= 100) {
		u8 weight[DLB_NUM_ARB_WEIGHTS];

		dlb_pf_calc_arbiter_weights(&dlb_dev->hw,
					    weight,
					    dlb_qid_sa_pct);

		dlb_hw_set_qid_arbiter_weights(&dlb_dev->hw, weight);
	}

}

/*****************************/
/****** Sysfs callbacks ******/
/*****************************/

static ssize_t dlb_sysfs_aux_vf_ids_read(struct device *dev,
					 struct device_attribute *attr,
					 char *buf,
					 int vf_id)
{
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);
	int i, size;

	mutex_lock(&dlb_dev->resource_mutex);

	size = 0;

	for (i = 0; i < DLB_MAX_NUM_VFS; i++) {
		if (!dlb_dev->child_id_state[i].is_auxiliary_vf)
			continue;

		if (dlb_dev->child_id_state[i].primary_vf_id != vf_id)
			continue;

		size += scnprintf(&buf[size], PAGE_SIZE - size, "%d,", i);
	}

	if (size == 0)
		size = 1;

	/* Replace the last comma with a newline */
	size += scnprintf(&buf[size - 1], PAGE_SIZE - size, "\n");

	mutex_unlock(&dlb_dev->resource_mutex);

	return size;
}

static ssize_t dlb_sysfs_aux_vf_ids_write(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf,
					  size_t count,
					  int primary_vf_id)
{
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);
	char *user_buf = (char *)buf;
	unsigned int vf_id;
	char *vf_id_str;

	mutex_lock(&dlb_dev->resource_mutex);

	/* If the primary VF is locked, no auxiliary VFs can be added to or
	 * removed from it.
	 */
	if (dlb_vf_is_locked(&dlb_dev->hw, primary_vf_id)) {
		mutex_unlock(&dlb_dev->resource_mutex);
		return -EINVAL;
	}

	vf_id_str = strsep(&user_buf, ",");
	while (vf_id_str) {
		struct vf_id_state *child_id_state;
		int ret;

		ret = kstrtouint(vf_id_str, 0, &vf_id);
		if (ret) {
			mutex_unlock(&dlb_dev->resource_mutex);
			return -EINVAL;
		}

		if (vf_id >= DLB_MAX_NUM_VFS) {
			mutex_unlock(&dlb_dev->resource_mutex);
			return -EINVAL;
		}

		child_id_state = &dlb_dev->child_id_state[vf_id];

		if (vf_id == primary_vf_id) {
			mutex_unlock(&dlb_dev->resource_mutex);
			return -EINVAL;
		}

		/* Check if the aux-primary VF relationship already exists */
		if (child_id_state->is_auxiliary_vf &&
		    child_id_state->primary_vf_id == primary_vf_id) {
			vf_id_str = strsep(&user_buf, ",");
			continue;
		}

		/* If the desired VF is locked, it can't be made auxiliary */
		if (dlb_vf_is_locked(&dlb_dev->hw, vf_id)) {
			mutex_unlock(&dlb_dev->resource_mutex);
			return -EINVAL;
		}

		/* Attempt to reassign the VF */
		child_id_state->is_auxiliary_vf = true;
		child_id_state->primary_vf_id = primary_vf_id;

		/* Reassign any of the desired VF's resources back to the PF */
		if (dlb_reset_vf_resources(&dlb_dev->hw, vf_id)) {
			mutex_unlock(&dlb_dev->resource_mutex);
			return -EINVAL;
		}

		vf_id_str = strsep(&user_buf, ",");
	}

	mutex_unlock(&dlb_dev->resource_mutex);

	return count;
}

static ssize_t dlb_sysfs_vf_read(struct device *dev,
				 struct device_attribute *attr,
				 char *buf,
				 int vf_id)
{
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);
	struct dlb_get_num_resources_args num_avail_rsrcs;
	struct dlb_get_num_resources_args num_used_rsrcs;
	struct dlb_get_num_resources_args num_rsrcs;
	struct dlb_hw *hw = &dlb_dev->hw;
	int val;

	mutex_lock(&dlb_dev->resource_mutex);

	val = dlb_hw_get_num_resources(hw, &num_avail_rsrcs, true, vf_id);
	if (val) {
		mutex_unlock(&dlb_dev->resource_mutex);
		return -1;
	}

	val = dlb_hw_get_num_used_resources(hw, &num_used_rsrcs, true, vf_id);
	if (val) {
		mutex_unlock(&dlb_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&dlb_dev->resource_mutex);

	num_rsrcs.num_sched_domains = num_avail_rsrcs.num_sched_domains +
		num_used_rsrcs.num_sched_domains;
	num_rsrcs.num_ldb_queues = num_avail_rsrcs.num_ldb_queues +
		num_used_rsrcs.num_ldb_queues;
	num_rsrcs.num_ldb_ports = num_avail_rsrcs.num_ldb_ports +
		num_used_rsrcs.num_ldb_ports;
	num_rsrcs.num_dir_ports = num_avail_rsrcs.num_dir_ports +
		num_used_rsrcs.num_dir_ports;
	num_rsrcs.num_ldb_credits = num_avail_rsrcs.num_ldb_credits +
		num_used_rsrcs.num_ldb_credits;
	num_rsrcs.num_dir_credits = num_avail_rsrcs.num_dir_credits +
		num_used_rsrcs.num_dir_credits;
	num_rsrcs.num_ldb_credit_pools = num_avail_rsrcs.num_ldb_credit_pools +
		num_used_rsrcs.num_ldb_credit_pools;
	num_rsrcs.num_dir_credit_pools = num_avail_rsrcs.num_dir_credit_pools +
		num_used_rsrcs.num_dir_credit_pools;
	num_rsrcs.num_hist_list_entries =
		num_avail_rsrcs.num_hist_list_entries +
		num_used_rsrcs.num_hist_list_entries;
	num_rsrcs.num_atomic_inflights = num_avail_rsrcs.num_atomic_inflights +
		num_used_rsrcs.num_atomic_inflights;

	if (strncmp(attr->attr.name, "num_sched_domains",
		    sizeof("num_sched_domains")) == 0)
		val = num_rsrcs.num_sched_domains;
	else if (strncmp(attr->attr.name, "num_ldb_queues",
			 sizeof("num_ldb_queues")) == 0)
		val = num_rsrcs.num_ldb_queues;
	else if (strncmp(attr->attr.name, "num_ldb_ports",
			 sizeof("num_ldb_ports")) == 0)
		val = num_rsrcs.num_ldb_ports;
	else if (strncmp(attr->attr.name, "num_dir_ports",
			 sizeof("num_dir_ports")) == 0)
		val = num_rsrcs.num_dir_ports;
	else if (strncmp(attr->attr.name, "num_ldb_credit_pools",
			 sizeof("num_ldb_credit_pools")) == 0)
		val = num_rsrcs.num_ldb_credit_pools;
	else if (strncmp(attr->attr.name, "num_dir_credit_pools",
			 sizeof("num_dir_credit_pools")) == 0)
		val = num_rsrcs.num_dir_credit_pools;
	else if (strncmp(attr->attr.name, "num_ldb_credits",
			 sizeof("num_ldb_credits")) == 0)
		val = num_rsrcs.num_ldb_credits;
	else if (strncmp(attr->attr.name, "num_dir_credits",
			 sizeof("num_dir_credits")) == 0)
		val = num_rsrcs.num_dir_credits;
	else if (strncmp(attr->attr.name, "num_hist_list_entries",
			 sizeof("num_hist_list_entries")) == 0)
		val = num_rsrcs.num_hist_list_entries;
	else if (strncmp(attr->attr.name, "num_atomic_inflights",
			 sizeof("num_atomic_inflights")) == 0)
		val = num_rsrcs.num_atomic_inflights;
	else if (strncmp(attr->attr.name, "locked",
			 sizeof("locked")) == 0)
		val = (int)dlb_vf_is_locked(hw, vf_id);
	else if (strncmp(attr->attr.name, "func_bar_en",
			 sizeof("func_bar_en")) == 0)
		val = (int)dlb_dev->mbox[vf_id].enabled;
	else
		return -1;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t dlb_sysfs_vf_write(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf,
				  size_t count,
				  int vf_id)
{
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);
	unsigned long num;
	const char *name;
	int ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	name = attr->attr.name;

	mutex_lock(&dlb_dev->resource_mutex);

	if (strncmp(name, "num_sched_domains",
		    sizeof("num_sched_domains")) == 0) {
		ret = dlb_update_vf_sched_domains(&dlb_dev->hw, vf_id, num);
	} else if (strncmp(name, "num_ldb_queues",
			   sizeof("num_ldb_queues")) == 0) {
		ret = dlb_update_vf_ldb_queues(&dlb_dev->hw, vf_id, num);
	} else if (strncmp(name, "num_ldb_ports",
			   sizeof("num_ldb_ports")) == 0) {
		ret = dlb_update_vf_ldb_ports(&dlb_dev->hw, vf_id, num);
	} else if (strncmp(name, "num_dir_ports",
			   sizeof("num_dir_ports")) == 0) {
		ret = dlb_update_vf_dir_ports(&dlb_dev->hw, vf_id, num);
	} else if (strncmp(name, "num_ldb_credit_pools",
			   sizeof("num_ldb_credit_pools")) == 0) {
		ret = dlb_update_vf_ldb_credit_pools(&dlb_dev->hw, vf_id, num);
	} else if (strncmp(name, "num_dir_credit_pools",
			   sizeof("num_dir_credit_pools")) == 0) {
		ret = dlb_update_vf_dir_credit_pools(&dlb_dev->hw, vf_id, num);
	} else if (strncmp(name, "num_ldb_credits",
			   sizeof("num_ldb_credits")) == 0) {
		ret = dlb_update_vf_ldb_credits(&dlb_dev->hw, vf_id, num);
	} else if (strncmp(name, "num_dir_credits",
			   sizeof("num_dir_credits")) == 0) {
		ret = dlb_update_vf_dir_credits(&dlb_dev->hw, vf_id, num);
	} else if (strncmp(attr->attr.name, "num_hist_list_entries",
			   sizeof("num_hist_list_entries")) == 0) {
		ret = dlb_update_vf_hist_list_entries(&dlb_dev->hw, vf_id, num);
	} else if (strncmp(attr->attr.name, "num_atomic_inflights",
			   sizeof("num_atomic_inflights")) == 0) {
		ret = dlb_update_vf_atomic_inflights(&dlb_dev->hw, vf_id, num);
	} else if (strncmp(attr->attr.name, "func_bar_en",
			   sizeof("func_bar_en")) == 0) {
		if (!dlb_dev->mbox[vf_id].enabled && num) {
			union dlb_sys_func_vf_bar_dsbl r0 = { {0} };

			r0.field.func_vf_bar_dis = 0;

			DLB_CSR_WR(&dlb_dev->hw,
				   DLB_SYS_FUNC_VF_BAR_DSBL(vf_id),
				   r0.val);

			dev_err(dlb_dev->dlb_device,
				"[%s()] Re-enabling VF %d's FUNC BAR",
				__func__, vf_id);

			dlb_dev->mbox[vf_id].enabled = true;
		}
	} else {
		mutex_unlock(&dlb_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&dlb_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

#define DLB_VF_SYSFS_RD_FUNC(id) \
static ssize_t dlb_sysfs_vf ## id ## _read(		      \
	struct device *dev,				      \
	struct device_attribute *attr,			      \
	char *buf)					      \
{							      \
	return dlb_sysfs_vf_read(dev, attr, buf, id);	      \
}							      \

#define DLB_VF_SYSFS_WR_FUNC(id) \
static ssize_t dlb_sysfs_vf ## id ## _write(		      \
	struct device *dev,				      \
	struct device_attribute *attr,			      \
	const char *buf,				      \
	size_t count)					      \
{							      \
	return dlb_sysfs_vf_write(dev, attr, buf, count, id); \
}

#define DLB_AUX_VF_ID_RD_FUNC(id) \
static ssize_t dlb_sysfs_vf ## id ## _vf_ids_read(		      \
	struct device *dev,					      \
	struct device_attribute *attr,				      \
	char *buf)						      \
{								      \
	return dlb_sysfs_aux_vf_ids_read(dev, attr, buf, id);	      \
}								      \

#define DLB_AUX_VF_ID_WR_FUNC(id) \
static ssize_t dlb_sysfs_vf ## id ## _vf_ids_write(		      \
	struct device *dev,					      \
	struct device_attribute *attr,				      \
	const char *buf,					      \
	size_t count)						      \
{								      \
	return dlb_sysfs_aux_vf_ids_write(dev, attr, buf, count, id); \
}

/* Read-write per-resource-group sysfs files */
#define DLB_VF_DEVICE_ATTRS(id) \
static struct device_attribute			    \
dev_attr_vf ## id ## _sched_domains =		    \
	__ATTR(num_sched_domains,		    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       dlb_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _ldb_queues =		    \
	__ATTR(num_ldb_queues,			    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       dlb_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _ldb_ports =		    \
	__ATTR(num_ldb_ports,			    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       dlb_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _dir_ports =		    \
	__ATTR(num_dir_ports,			    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       dlb_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _ldb_credit_pools =	    \
	__ATTR(num_ldb_credit_pools,		    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       dlb_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _dir_credit_pools =	    \
	__ATTR(num_dir_credit_pools,		    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       dlb_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _ldb_credits =		    \
	__ATTR(num_ldb_credits,			    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       dlb_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _dir_credits =		    \
	__ATTR(num_dir_credits,			    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       dlb_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _hist_list_entries =	    \
	__ATTR(num_hist_list_entries,		    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       dlb_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _atomic_inflights =	    \
	__ATTR(num_atomic_inflights,		    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       dlb_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _locked =			    \
	__ATTR(locked,				    \
	       0444,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       NULL);				    \
static struct device_attribute			    \
dev_attr_vf ## id ## _func_bar_en =		    \
	__ATTR(func_bar_en,			    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _read,	    \
	       dlb_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _aux_vf_ids =		    \
	__ATTR(aux_vf_ids,			    \
	       0644,				    \
	       dlb_sysfs_vf ## id ## _vf_ids_read,  \
	       dlb_sysfs_vf ## id ## _vf_ids_write) \

#define DLB_VF_SYSFS_ATTRS(id) \
DLB_VF_DEVICE_ATTRS(id);				\
static struct attribute *dlb_vf ## id ## _attrs[] = {	\
	&dev_attr_vf ## id ## _sched_domains.attr,	\
	&dev_attr_vf ## id ## _ldb_queues.attr,		\
	&dev_attr_vf ## id ## _ldb_ports.attr,		\
	&dev_attr_vf ## id ## _dir_ports.attr,		\
	&dev_attr_vf ## id ## _ldb_credit_pools.attr,	\
	&dev_attr_vf ## id ## _dir_credit_pools.attr,	\
	&dev_attr_vf ## id ## _ldb_credits.attr,	\
	&dev_attr_vf ## id ## _dir_credits.attr,	\
	&dev_attr_vf ## id ## _hist_list_entries.attr,	\
	&dev_attr_vf ## id ## _atomic_inflights.attr,	\
	&dev_attr_vf ## id ## _locked.attr,		\
	&dev_attr_vf ## id ## _func_bar_en.attr,	\
	&dev_attr_vf ## id ## _aux_vf_ids.attr,		\
	NULL						\
}

#define DLB_VF_SYSFS_ATTR_GROUP(id) \
DLB_VF_SYSFS_ATTRS(id);						\
static struct attribute_group dlb_vf ## id ## _attr_group = {	\
	.attrs = dlb_vf ## id ## _attrs,			\
	.name = "vf" #id "_resources"				\
}

DLB_VF_SYSFS_RD_FUNC(0);
DLB_VF_SYSFS_RD_FUNC(1);
DLB_VF_SYSFS_RD_FUNC(2);
DLB_VF_SYSFS_RD_FUNC(3);
DLB_VF_SYSFS_RD_FUNC(4);
DLB_VF_SYSFS_RD_FUNC(5);
DLB_VF_SYSFS_RD_FUNC(6);
DLB_VF_SYSFS_RD_FUNC(7);
DLB_VF_SYSFS_RD_FUNC(8);
DLB_VF_SYSFS_RD_FUNC(9);
DLB_VF_SYSFS_RD_FUNC(10);
DLB_VF_SYSFS_RD_FUNC(11);
DLB_VF_SYSFS_RD_FUNC(12);
DLB_VF_SYSFS_RD_FUNC(13);
DLB_VF_SYSFS_RD_FUNC(14);
DLB_VF_SYSFS_RD_FUNC(15);

DLB_VF_SYSFS_WR_FUNC(0);
DLB_VF_SYSFS_WR_FUNC(1);
DLB_VF_SYSFS_WR_FUNC(2);
DLB_VF_SYSFS_WR_FUNC(3);
DLB_VF_SYSFS_WR_FUNC(4);
DLB_VF_SYSFS_WR_FUNC(5);
DLB_VF_SYSFS_WR_FUNC(6);
DLB_VF_SYSFS_WR_FUNC(7);
DLB_VF_SYSFS_WR_FUNC(8);
DLB_VF_SYSFS_WR_FUNC(9);
DLB_VF_SYSFS_WR_FUNC(10);
DLB_VF_SYSFS_WR_FUNC(11);
DLB_VF_SYSFS_WR_FUNC(12);
DLB_VF_SYSFS_WR_FUNC(13);
DLB_VF_SYSFS_WR_FUNC(14);
DLB_VF_SYSFS_WR_FUNC(15);

DLB_AUX_VF_ID_RD_FUNC(0);
DLB_AUX_VF_ID_RD_FUNC(1);
DLB_AUX_VF_ID_RD_FUNC(2);
DLB_AUX_VF_ID_RD_FUNC(3);
DLB_AUX_VF_ID_RD_FUNC(4);
DLB_AUX_VF_ID_RD_FUNC(5);
DLB_AUX_VF_ID_RD_FUNC(6);
DLB_AUX_VF_ID_RD_FUNC(7);
DLB_AUX_VF_ID_RD_FUNC(8);
DLB_AUX_VF_ID_RD_FUNC(9);
DLB_AUX_VF_ID_RD_FUNC(10);
DLB_AUX_VF_ID_RD_FUNC(11);
DLB_AUX_VF_ID_RD_FUNC(12);
DLB_AUX_VF_ID_RD_FUNC(13);
DLB_AUX_VF_ID_RD_FUNC(14);
DLB_AUX_VF_ID_RD_FUNC(15);

DLB_AUX_VF_ID_WR_FUNC(0);
DLB_AUX_VF_ID_WR_FUNC(1);
DLB_AUX_VF_ID_WR_FUNC(2);
DLB_AUX_VF_ID_WR_FUNC(3);
DLB_AUX_VF_ID_WR_FUNC(4);
DLB_AUX_VF_ID_WR_FUNC(5);
DLB_AUX_VF_ID_WR_FUNC(6);
DLB_AUX_VF_ID_WR_FUNC(7);
DLB_AUX_VF_ID_WR_FUNC(8);
DLB_AUX_VF_ID_WR_FUNC(9);
DLB_AUX_VF_ID_WR_FUNC(10);
DLB_AUX_VF_ID_WR_FUNC(11);
DLB_AUX_VF_ID_WR_FUNC(12);
DLB_AUX_VF_ID_WR_FUNC(13);
DLB_AUX_VF_ID_WR_FUNC(14);
DLB_AUX_VF_ID_WR_FUNC(15);

DLB_VF_SYSFS_ATTR_GROUP(0);
DLB_VF_SYSFS_ATTR_GROUP(1);
DLB_VF_SYSFS_ATTR_GROUP(2);
DLB_VF_SYSFS_ATTR_GROUP(3);
DLB_VF_SYSFS_ATTR_GROUP(4);
DLB_VF_SYSFS_ATTR_GROUP(5);
DLB_VF_SYSFS_ATTR_GROUP(6);
DLB_VF_SYSFS_ATTR_GROUP(7);
DLB_VF_SYSFS_ATTR_GROUP(8);
DLB_VF_SYSFS_ATTR_GROUP(9);
DLB_VF_SYSFS_ATTR_GROUP(10);
DLB_VF_SYSFS_ATTR_GROUP(11);
DLB_VF_SYSFS_ATTR_GROUP(12);
DLB_VF_SYSFS_ATTR_GROUP(13);
DLB_VF_SYSFS_ATTR_GROUP(14);
DLB_VF_SYSFS_ATTR_GROUP(15);

const struct attribute_group *dlb_vf_attrs[] = {
	&dlb_vf0_attr_group,
	&dlb_vf1_attr_group,
	&dlb_vf2_attr_group,
	&dlb_vf3_attr_group,
	&dlb_vf4_attr_group,
	&dlb_vf5_attr_group,
	&dlb_vf6_attr_group,
	&dlb_vf7_attr_group,
	&dlb_vf8_attr_group,
	&dlb_vf9_attr_group,
	&dlb_vf10_attr_group,
	&dlb_vf11_attr_group,
	&dlb_vf12_attr_group,
	&dlb_vf13_attr_group,
	&dlb_vf14_attr_group,
	&dlb_vf15_attr_group,
};

#define DLB_TOTAL_SYSFS_SHOW(name, macro)		\
static ssize_t total_##name##_show(			\
	struct device *dev,				\
	struct device_attribute *attr,			\
	char *buf)					\
{							\
	int val = DLB_MAX_NUM_##macro;			\
							\
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);	\
}

DLB_TOTAL_SYSFS_SHOW(num_sched_domains, DOMAINS)
DLB_TOTAL_SYSFS_SHOW(num_ldb_queues, LDB_QUEUES)
DLB_TOTAL_SYSFS_SHOW(num_ldb_ports, LDB_PORTS)
DLB_TOTAL_SYSFS_SHOW(num_ldb_credit_pools, LDB_CREDIT_POOLS)
DLB_TOTAL_SYSFS_SHOW(num_dir_credit_pools, DIR_CREDIT_POOLS)
DLB_TOTAL_SYSFS_SHOW(num_ldb_credits, LDB_CREDITS)
DLB_TOTAL_SYSFS_SHOW(num_dir_credits, DIR_CREDITS)
DLB_TOTAL_SYSFS_SHOW(num_atomic_inflights, AQOS_ENTRIES)
DLB_TOTAL_SYSFS_SHOW(num_hist_list_entries, HIST_LIST_ENTRIES)

static ssize_t total_num_dir_ports_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int val = DLB_MAX_NUM_DIR_PORTS;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

#define DLB_AVAIL_SYSFS_SHOW(name)			    \
static ssize_t avail_##name##_show(			    \
	struct device *dev,				    \
	struct device_attribute *attr,			    \
	char *buf)					    \
{							    \
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);     \
	struct dlb_get_num_resources_args arg;		    \
	struct dlb_hw *hw = &dlb_dev->hw;		    \
	int val;					    \
							    \
	mutex_lock(&dlb_dev->resource_mutex);		    \
							    \
	val = dlb_hw_get_num_resources(hw, &arg, false, 0); \
							    \
	mutex_unlock(&dlb_dev->resource_mutex);		    \
							    \
	if (val)					    \
		return -1;				    \
							    \
	val = arg.name;					    \
							    \
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);	    \
}

DLB_AVAIL_SYSFS_SHOW(num_sched_domains)
DLB_AVAIL_SYSFS_SHOW(num_ldb_queues)
DLB_AVAIL_SYSFS_SHOW(num_ldb_ports)
DLB_AVAIL_SYSFS_SHOW(num_dir_ports)
DLB_AVAIL_SYSFS_SHOW(num_ldb_credit_pools)
DLB_AVAIL_SYSFS_SHOW(num_dir_credit_pools)
DLB_AVAIL_SYSFS_SHOW(num_ldb_credits)
DLB_AVAIL_SYSFS_SHOW(num_dir_credits)
DLB_AVAIL_SYSFS_SHOW(num_atomic_inflights)
DLB_AVAIL_SYSFS_SHOW(num_hist_list_entries)

static ssize_t max_ctg_atm_inflights_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);
	struct dlb_get_num_resources_args arg;
	struct dlb_hw *hw = &dlb_dev->hw;
	int val;

	mutex_lock(&dlb_dev->resource_mutex);

	val = dlb_hw_get_num_resources(hw, &arg, false, 0);

	mutex_unlock(&dlb_dev->resource_mutex);

	if (val)
		return -1;

	val = arg.max_contiguous_atomic_inflights;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t max_ctg_hl_entries_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);
	struct dlb_get_num_resources_args arg;
	struct dlb_hw *hw = &dlb_dev->hw;
	int val;

	mutex_lock(&dlb_dev->resource_mutex);

	val = dlb_hw_get_num_resources(hw, &arg, false, 0);

	mutex_unlock(&dlb_dev->resource_mutex);

	if (val)
		return -1;

	val = arg.max_contiguous_hist_list_entries;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

/* Device attribute name doesn't match the show function name, so we define our
 * own DEVICE_ATTR macro.
 */
#define DLB_DEVICE_ATTR_RO(_prefix, _name) \
struct device_attribute dev_attr_##_prefix##_##_name = {\
	.attr = { .name = __stringify(_name), .mode = 0444 },\
	.show = _prefix##_##_name##_show,\
}

static DLB_DEVICE_ATTR_RO(total, num_sched_domains);
static DLB_DEVICE_ATTR_RO(total, num_ldb_queues);
static DLB_DEVICE_ATTR_RO(total, num_ldb_ports);
static DLB_DEVICE_ATTR_RO(total, num_dir_ports);
static DLB_DEVICE_ATTR_RO(total, num_ldb_credit_pools);
static DLB_DEVICE_ATTR_RO(total, num_dir_credit_pools);
static DLB_DEVICE_ATTR_RO(total, num_ldb_credits);
static DLB_DEVICE_ATTR_RO(total, num_dir_credits);
static DLB_DEVICE_ATTR_RO(total, num_atomic_inflights);
static DLB_DEVICE_ATTR_RO(total, num_hist_list_entries);

static struct attribute *dlb_total_attrs[] = {
	&dev_attr_total_num_sched_domains.attr,
	&dev_attr_total_num_ldb_queues.attr,
	&dev_attr_total_num_ldb_ports.attr,
	&dev_attr_total_num_dir_ports.attr,
	&dev_attr_total_num_ldb_credit_pools.attr,
	&dev_attr_total_num_dir_credit_pools.attr,
	&dev_attr_total_num_ldb_credits.attr,
	&dev_attr_total_num_dir_credits.attr,
	&dev_attr_total_num_atomic_inflights.attr,
	&dev_attr_total_num_hist_list_entries.attr,
	NULL
};

static const struct attribute_group dlb_total_attr_group = {
	.attrs = dlb_total_attrs,
	.name = "total_resources",
};

static DLB_DEVICE_ATTR_RO(avail, num_sched_domains);
static DLB_DEVICE_ATTR_RO(avail, num_ldb_queues);
static DLB_DEVICE_ATTR_RO(avail, num_ldb_ports);
static DLB_DEVICE_ATTR_RO(avail, num_dir_ports);
static DLB_DEVICE_ATTR_RO(avail, num_ldb_credit_pools);
static DLB_DEVICE_ATTR_RO(avail, num_dir_credit_pools);
static DLB_DEVICE_ATTR_RO(avail, num_ldb_credits);
static DLB_DEVICE_ATTR_RO(avail, num_dir_credits);
static DLB_DEVICE_ATTR_RO(avail, num_atomic_inflights);
static DLB_DEVICE_ATTR_RO(avail, num_hist_list_entries);
static DEVICE_ATTR_RO(max_ctg_atm_inflights);
static DEVICE_ATTR_RO(max_ctg_hl_entries);

static struct attribute *dlb_avail_attrs[] = {
	&dev_attr_avail_num_sched_domains.attr,
	&dev_attr_avail_num_ldb_queues.attr,
	&dev_attr_avail_num_ldb_ports.attr,
	&dev_attr_avail_num_dir_ports.attr,
	&dev_attr_avail_num_ldb_credit_pools.attr,
	&dev_attr_avail_num_dir_credit_pools.attr,
	&dev_attr_avail_num_ldb_credits.attr,
	&dev_attr_avail_num_dir_credits.attr,
	&dev_attr_avail_num_atomic_inflights.attr,
	&dev_attr_avail_num_hist_list_entries.attr,
	&dev_attr_max_ctg_atm_inflights.attr,
	&dev_attr_max_ctg_hl_entries.attr,
	NULL
};

static const struct attribute_group dlb_avail_attr_group = {
	.attrs = dlb_avail_attrs,
	.name = "avail_resources",
};

#define DLB_GROUP_SNS_PER_QUEUE_SHOW(id)		\
static ssize_t group##id##_sns_per_queue_show(		\
	struct device *dev,				\
	struct device_attribute *attr,			\
	char *buf)					\
{							\
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);	\
	struct dlb_hw *hw = &dlb_dev->hw;		\
	int val;					\
							\
	mutex_lock(&dlb_dev->resource_mutex);		\
							\
	val = dlb_get_group_sequence_numbers(hw, id);	\
							\
	mutex_unlock(&dlb_dev->resource_mutex);		\
							\
	if (val < 0)					\
		return val;				\
							\
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);	\
}

DLB_GROUP_SNS_PER_QUEUE_SHOW(0)
DLB_GROUP_SNS_PER_QUEUE_SHOW(1)
DLB_GROUP_SNS_PER_QUEUE_SHOW(2)
DLB_GROUP_SNS_PER_QUEUE_SHOW(3)

#define DLB_GROUP_SNS_PER_QUEUE_STORE(id)		\
static ssize_t group##id##_sns_per_queue_store(		\
	struct device *dev,				\
	struct device_attribute *attr,			\
	const char *buf,				\
	size_t count)					\
{							\
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);	\
	struct dlb_hw *hw = &dlb_dev->hw;		\
	unsigned long val;				\
	int err;					\
							\
	err = kstrtoul(buf, 0, &val);			\
	if (err)					\
		return -1;				\
							\
	mutex_lock(&dlb_dev->resource_mutex);		\
							\
	dlb_set_group_sequence_numbers(hw, id, val);	\
							\
	mutex_unlock(&dlb_dev->resource_mutex);		\
							\
	return count;					\
}

DLB_GROUP_SNS_PER_QUEUE_STORE(0)
DLB_GROUP_SNS_PER_QUEUE_STORE(1)
DLB_GROUP_SNS_PER_QUEUE_STORE(2)
DLB_GROUP_SNS_PER_QUEUE_STORE(3)

/* RW sysfs files in the sequence_numbers/ subdirectory */
static DEVICE_ATTR_RW(group0_sns_per_queue);
static DEVICE_ATTR_RW(group1_sns_per_queue);
static DEVICE_ATTR_RW(group2_sns_per_queue);
static DEVICE_ATTR_RW(group3_sns_per_queue);

static struct attribute *dlb_sequence_number_attrs[] = {
	&dev_attr_group0_sns_per_queue.attr,
	&dev_attr_group1_sns_per_queue.attr,
	&dev_attr_group2_sns_per_queue.attr,
	&dev_attr_group3_sns_per_queue.attr,
	NULL
};

static const struct attribute_group dlb_sequence_number_attr_group = {
	.attrs = dlb_sequence_number_attrs,
	.name = "sequence_numbers"
};

static ssize_t dev_id_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n", dlb_dev->id);
}

static DEVICE_ATTR_RO(dev_id);

static ssize_t ingress_err_en_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);
	ssize_t ret;

	mutex_lock(&dlb_dev->resource_mutex);

	ret = scnprintf(buf, PAGE_SIZE, "%d\n", dlb_dev->ingress_err.enabled);

	mutex_unlock(&dlb_dev->resource_mutex);

	return ret;
}

static ssize_t ingress_err_en_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf,
				    size_t count)
{
	struct dlb_dev *dlb_dev = dev_get_drvdata(dev);
	unsigned long num;
	ssize_t ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	mutex_lock(&dlb_dev->resource_mutex);

	if (!dlb_dev->ingress_err.enabled && num) {
		dlb_enable_alarms(dlb_dev);

		dev_err(dlb_dev->dlb_device,
			"[%s()] Re-enabling ingress error interrupts",
			__func__);

		dlb_dev->ingress_err.enabled = true;
	}

	mutex_unlock(&dlb_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

static DEVICE_ATTR_RW(ingress_err_en);

static int dlb_pf_sysfs_create(struct dlb_dev *dlb_dev)
{
	struct kobject *kobj;
	int ret;
	int i;

	kobj = &dlb_dev->pdev->dev.kobj;

	ret = sysfs_create_file(kobj, &dev_attr_ingress_err_en.attr);
	if (ret)
		goto dlb_ingress_err_en_attr_group_fail;

	ret = sysfs_create_file(kobj, &dev_attr_dev_id.attr);
	if (ret)
		goto dlb_dev_id_attr_group_fail;

	ret = sysfs_create_group(kobj, &dlb_total_attr_group);
	if (ret)
		goto dlb_total_attr_group_fail;

	ret = sysfs_create_group(kobj, &dlb_avail_attr_group);
	if (ret)
		goto dlb_avail_attr_group_fail;

	ret = sysfs_create_group(kobj, &dlb_sequence_number_attr_group);
	if (ret)
		goto dlb_sn_attr_group_fail;

	for (i = 0; i < pci_num_vf(dlb_dev->pdev); i++) {
		ret = sysfs_create_group(kobj, dlb_vf_attrs[i]);
		if (ret)
			goto dlb_vf_attr_group_fail;
	}

	return 0;

dlb_vf_attr_group_fail:
	for (i--; i >= 0; i--)
		sysfs_remove_group(kobj, dlb_vf_attrs[i]);

	sysfs_remove_group(kobj, &dlb_sequence_number_attr_group);
dlb_sn_attr_group_fail:
	sysfs_remove_group(kobj, &dlb_avail_attr_group);
dlb_avail_attr_group_fail:
	sysfs_remove_group(kobj, &dlb_total_attr_group);
dlb_total_attr_group_fail:
	sysfs_remove_file(kobj, &dev_attr_dev_id.attr);
dlb_dev_id_attr_group_fail:
	sysfs_remove_file(kobj, &dev_attr_ingress_err_en.attr);
dlb_ingress_err_en_attr_group_fail:
	return ret;
}

static void dlb_pf_sysfs_destroy(struct dlb_dev *dlb_dev)
{
	struct kobject *kobj;
	int i;

	kobj = &dlb_dev->pdev->dev.kobj;

	for (i = 0; i < pci_num_vf(dlb_dev->pdev); i++)
		sysfs_remove_group(kobj, dlb_vf_attrs[i]);

	sysfs_remove_group(kobj, &dlb_sequence_number_attr_group);
	sysfs_remove_group(kobj, &dlb_avail_attr_group);
	sysfs_remove_group(kobj, &dlb_total_attr_group);
	sysfs_remove_file(kobj, &dev_attr_dev_id.attr);
	sysfs_remove_file(kobj, &dev_attr_ingress_err_en.attr);
}

static void dlb_pf_sysfs_reapply_configuration(struct dlb_dev *dev)
{
	int i;

	for (i = 0; i < DLB_MAX_NUM_SEQUENCE_NUMBER_GROUPS; i++) {
		int num_sns = dlb_get_group_sequence_numbers(&dev->hw, i);

		dlb_set_group_sequence_numbers(&dev->hw, i, num_sns);
	}
}

/*****************************/
/****** IOCTL callbacks ******/
/*****************************/

static int dlb_pf_create_sched_domain(struct dlb_hw *hw,
				      struct dlb_create_sched_domain_args *args,
				      struct dlb_cmd_response *resp)
{
	return dlb_hw_create_sched_domain(hw, args, resp, false, 0);
}

static int dlb_pf_create_ldb_pool(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_create_ldb_pool_args *args,
				  struct dlb_cmd_response *resp)
{
	return dlb_hw_create_ldb_pool(hw, id, args, resp, false, 0);
}

static int dlb_pf_create_dir_pool(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_create_dir_pool_args *args,
				  struct dlb_cmd_response *resp)
{
	return dlb_hw_create_dir_pool(hw, id, args, resp, false, 0);
}

static int dlb_pf_create_ldb_queue(struct dlb_hw *hw,
				   u32 id,
				   struct dlb_create_ldb_queue_args *args,
				   struct dlb_cmd_response *resp)
{
	return dlb_hw_create_ldb_queue(hw, id, args, resp, false, 0);
}

static int dlb_pf_create_dir_queue(struct dlb_hw *hw,
				   u32 id,
				   struct dlb_create_dir_queue_args *args,
				   struct dlb_cmd_response *resp)
{
	return dlb_hw_create_dir_queue(hw, id, args, resp, false, 0);
}

static int dlb_pf_create_ldb_port(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_create_ldb_port_args *args,
				  uintptr_t pop_count_dma_base,
				  uintptr_t cq_dma_base,
				  struct dlb_cmd_response *resp)
{
	return dlb_hw_create_ldb_port(hw, id, args,
				      pop_count_dma_base,
				      cq_dma_base,
				      resp, false, 0);
}

static int dlb_pf_create_dir_port(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_create_dir_port_args *args,
				  uintptr_t pop_count_dma_base,
				  uintptr_t cq_dma_base,
				  struct dlb_cmd_response *resp)
{
	return dlb_hw_create_dir_port(hw, id, args,
				      pop_count_dma_base,
				      cq_dma_base,
				      resp, false, 0);
}

static int dlb_pf_start_domain(struct dlb_hw *hw,
			       u32 id,
			       struct dlb_start_domain_args *args,
			       struct dlb_cmd_response *resp)
{
	return dlb_hw_start_domain(hw, id, args, resp, false, 0);
}

static int dlb_pf_map_qid(struct dlb_hw *hw,
			  u32 id,
			  struct dlb_map_qid_args *args,
			  struct dlb_cmd_response *resp)
{
	return dlb_hw_map_qid(hw, id, args, resp, false, 0);
}

static int dlb_pf_unmap_qid(struct dlb_hw *hw,
			    u32 id,
			    struct dlb_unmap_qid_args *args,
			    struct dlb_cmd_response *resp)
{
	return dlb_hw_unmap_qid(hw, id, args, resp, false, 0);
}

static int dlb_pf_pending_port_unmaps(struct dlb_hw *hw,
				      u32 id,
				      struct dlb_pending_port_unmaps_args *args,
				      struct dlb_cmd_response *resp)
{
	return dlb_hw_pending_port_unmaps(hw, id, args, resp, false, 0);
}

static int dlb_pf_enable_ldb_port(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_enable_ldb_port_args *args,
				  struct dlb_cmd_response *resp)
{
	return dlb_hw_enable_ldb_port(hw, id, args, resp, false, 0);
}

static int dlb_pf_disable_ldb_port(struct dlb_hw *hw,
				   u32 id,
				   struct dlb_disable_ldb_port_args *args,
				   struct dlb_cmd_response *resp)
{
	return dlb_hw_disable_ldb_port(hw, id, args, resp, false, 0);
}

static int dlb_pf_enable_dir_port(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_enable_dir_port_args *args,
				  struct dlb_cmd_response *resp)
{
	return dlb_hw_enable_dir_port(hw, id, args, resp, false, 0);
}

static int dlb_pf_disable_dir_port(struct dlb_hw *hw,
				   u32 id,
				   struct dlb_disable_dir_port_args *args,
				   struct dlb_cmd_response *resp)
{
	return dlb_hw_disable_dir_port(hw, id, args, resp, false, 0);
}

static int dlb_pf_get_num_resources(struct dlb_hw *hw,
				    struct dlb_get_num_resources_args *args)
{
	return dlb_hw_get_num_resources(hw, args, false, 0);
}

static void dlb_reset_packed_interrupts(struct dlb_dev *dev, int id)
{
	struct dlb_hw *hw = &dev->hw;
	u32 vec;
	int i;

	for (i = 0; i < DLB_MAX_NUM_LDB_PORTS; i++) {
		if (!dlb_ldb_port_owned_by_domain(hw, id, i, false, 0))
			continue;

		if (!dev->intr.ldb_cq_intr[i].configured)
			continue;

		vec = dev->intr.ldb_cq_intr[i].vector;

		dev->intr.packed_vector_bitmap &= ~((u64)1 << vec);
	}

	for (i = 0; i < DLB_MAX_NUM_DIR_PORTS; i++) {
		if (!dlb_dir_port_owned_by_domain(hw, id, i, false, 0))
			continue;

		if (!dev->intr.dir_cq_intr[i].configured)
			continue;

		vec = dev->intr.dir_cq_intr[i].vector;

		dev->intr.packed_vector_bitmap &= ~((u64)1 << vec);
	}
}

static int dlb_pf_reset_domain(struct dlb_dev *dev, u32 id)
{
	/* Unset the domain's packed vector bitmap entries */
	if (dev->intr.mode == DLB_MSIX_MODE_PACKED)
		dlb_reset_packed_interrupts(dev, id);

	return dlb_reset_domain(&dev->hw, id, false, 0);
}


static int dlb_pf_get_ldb_queue_depth(struct dlb_hw *hw,
				      u32 id,
				      struct dlb_get_ldb_queue_depth_args *args,
				      struct dlb_cmd_response *resp)
{
	return dlb_hw_get_ldb_queue_depth(hw, id, args, resp, false, 0);
}

static int dlb_pf_get_dir_queue_depth(struct dlb_hw *hw,
				      u32 id,
				      struct dlb_get_dir_queue_depth_args *args,
				      struct dlb_cmd_response *resp)
{
	return dlb_hw_get_dir_queue_depth(hw, id, args, resp, false, 0);
}

static int dlb_pf_query_cq_poll_mode(struct dlb_dev *dlb_dev,
				     struct dlb_cmd_response *user_resp)
{
	user_resp->status = 0;

	if (dlb_sparse_cq_mode_enabled) {
		dlb_hw_enable_sparse_ldb_cq_mode(&dlb_dev->hw);
		dlb_hw_enable_sparse_dir_cq_mode(&dlb_dev->hw);
	}

	if (dlb_sparse_cq_mode_enabled)
		user_resp->id = DLB_CQ_POLL_MODE_SPARSE;
	else
		user_resp->id = DLB_CQ_POLL_MODE_STD;

	return 0;

}

/**************************************/
/****** Resource query callbacks ******/
/**************************************/

static int dlb_pf_ldb_port_owned_by_domain(struct dlb_hw *hw,
					   u32 domain_id,
					   u32 port_id)
{
	return dlb_ldb_port_owned_by_domain(hw, domain_id, port_id, false, 0);
}

static int dlb_pf_dir_port_owned_by_domain(struct dlb_hw *hw,
					   u32 domain_id,
					   u32 port_id)
{
	return dlb_dir_port_owned_by_domain(hw, domain_id, port_id, false, 0);
}

static int dlb_pf_get_sn_allocation(struct dlb_hw *hw, u32 group_id)
{
	return dlb_get_group_sequence_numbers(hw, group_id);
}

static int dlb_pf_set_sn_allocation(struct dlb_hw *hw, u32 group_id, u32 num)
{
	return dlb_set_group_sequence_numbers(hw, group_id, num);
}

static int dlb_pf_get_sn_occupancy(struct dlb_hw *hw, u32 group_id)
{
	return dlb_get_group_sequence_number_occupancy(hw, group_id);
}

/*******************************/
/****** DLB PF Device Ops ******/
/*******************************/

struct dlb_device_ops dlb_pf_ops = {
	.map_pci_bar_space = dlb_pf_map_pci_bar_space,
	.unmap_pci_bar_space = dlb_pf_unmap_pci_bar_space,
	.mmap = dlb_pf_mmap,
	.inc_pm_refcnt = dlb_pf_pm_inc_refcnt,
	.dec_pm_refcnt = dlb_pf_pm_dec_refcnt,
	.init_driver_state = dlb_pf_init_driver_state,
	.free_driver_state = dlb_pf_free_driver_state,
	.device_create = dlb_pf_device_create,
	.device_destroy = dlb_pf_device_destroy,
	.cdev_add = dlb_pf_cdev_add,
	.cdev_del = dlb_pf_cdev_del,
	.sysfs_create = dlb_pf_sysfs_create,
	.sysfs_destroy = dlb_pf_sysfs_destroy,
	.sysfs_reapply = dlb_pf_sysfs_reapply_configuration,
	.init_interrupts = dlb_pf_init_interrupts,
	.enable_ldb_cq_interrupts = dlb_pf_enable_ldb_cq_interrupts,
	.enable_dir_cq_interrupts = dlb_pf_enable_dir_cq_interrupts,
	.arm_cq_interrupt = dlb_pf_arm_cq_interrupt,
	.reinit_interrupts = dlb_pf_reinit_interrupts,
	.free_interrupts = dlb_pf_free_interrupts,
	.init_hardware = dlb_pf_init_hardware,
	.register_driver = dlb_pf_register_driver,
	.unregister_driver = dlb_pf_unregister_driver,
	.create_sched_domain = dlb_pf_create_sched_domain,
	.create_ldb_pool = dlb_pf_create_ldb_pool,
	.create_dir_pool = dlb_pf_create_dir_pool,
	.create_ldb_queue = dlb_pf_create_ldb_queue,
	.create_dir_queue = dlb_pf_create_dir_queue,
	.create_ldb_port = dlb_pf_create_ldb_port,
	.create_dir_port = dlb_pf_create_dir_port,
	.start_domain = dlb_pf_start_domain,
	.map_qid = dlb_pf_map_qid,
	.unmap_qid = dlb_pf_unmap_qid,
	.pending_port_unmaps = dlb_pf_pending_port_unmaps,
	.enable_ldb_port = dlb_pf_enable_ldb_port,
	.enable_dir_port = dlb_pf_enable_dir_port,
	.disable_ldb_port = dlb_pf_disable_ldb_port,
	.disable_dir_port = dlb_pf_disable_dir_port,
	.get_num_resources = dlb_pf_get_num_resources,
	.reset_domain = dlb_pf_reset_domain,
	.ldb_port_owned_by_domain = dlb_pf_ldb_port_owned_by_domain,
	.dir_port_owned_by_domain = dlb_pf_dir_port_owned_by_domain,
	.get_sn_allocation = dlb_pf_get_sn_allocation,
	.set_sn_allocation = dlb_pf_set_sn_allocation,
	.get_sn_occupancy = dlb_pf_get_sn_occupancy,
	.get_ldb_queue_depth = dlb_pf_get_ldb_queue_depth,
	.get_dir_queue_depth = dlb_pf_get_dir_queue_depth,
	.query_cq_poll_mode = dlb_pf_query_cq_poll_mode,
};
