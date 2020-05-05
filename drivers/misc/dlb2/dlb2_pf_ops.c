// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/uaccess.h>

#include "base/dlb2_mbox.h"
#include "base/dlb2_osdep.h"
#include "base/dlb2_resource.h"
#include "dlb2_dp_ops.h"
#include "dlb2_intr.h"
#include "dlb2_ioctl.h"
#include "dlb2_sriov.h"
#include "dlb2_vdcm.h"

/***********************************/
/****** Runtime PM management ******/
/***********************************/

static void
dlb2_pf_pm_inc_refcnt(struct pci_dev *pdev, bool resume)
{
	if (resume)
		/* Increment the device's usage count and immediately wake it
		 * if it was suspended.
		 */
		pm_runtime_get_sync(&pdev->dev);
	else
		pm_runtime_get_noresume(&pdev->dev);
}

static void
dlb2_pf_pm_dec_refcnt(struct pci_dev *pdev)
{
	/* Decrement the device's usage count and suspend it if the
	 * count reaches zero.
	 */
	pm_runtime_put_sync_suspend(&pdev->dev);
}

static bool
dlb2_pf_pm_status_suspended(struct pci_dev *pdev)
{
	return pm_runtime_status_suspended(&pdev->dev);
}

/********************************/
/****** PCI BAR management ******/
/********************************/

static void
dlb2_pf_unmap_pci_bar_space(struct dlb2_dev *dlb2_dev,
			    struct pci_dev *pdev)
{
	pci_iounmap(pdev, dlb2_dev->hw.csr_kva);
	pci_iounmap(pdev, dlb2_dev->hw.func_kva);
}

static int
dlb2_pf_map_pci_bar_space(struct dlb2_dev *dlb2_dev,
			  struct pci_dev *pdev)
{
	int ret;

	/* BAR 0: PF FUNC BAR space */
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

	/* BAR 2: PF CSR BAR space */
	dlb2_dev->hw.csr_kva = pci_iomap(pdev, 2, 0);
	dlb2_dev->hw.csr_phys_addr = pci_resource_start(pdev, 2);

	if (!dlb2_dev->hw.csr_kva) {
		dev_err(&pdev->dev, "Cannot iomap BAR 2 (size %llu)\n",
			pci_resource_len(pdev, 2));

		ret = -EIO;
		goto pci_iomap_bar2_fail;
	}

	dev_dbg(&pdev->dev, "BAR 2 iomem base: %p\n",
		dlb2_dev->hw.csr_kva);
	dev_dbg(&pdev->dev, "BAR 2 start: 0x%llx\n",
		pci_resource_start(pdev, 2));
	dev_dbg(&pdev->dev, "BAR 2 len: %llu\n",
		pci_resource_len(pdev, 2));

	return 0;

pci_iomap_bar2_fail:
	pci_iounmap(pdev, dlb2_dev->hw.func_kva);

	return ret;
}

#define DLB2_LDB_CQ_BOUND DLB2_LDB_CQ_OFFS(DLB2_MAX_NUM_LDB_PORTS)
#define DLB2_DIR_CQ_BOUND DLB2_DIR_CQ_OFFS(DLB2_MAX_NUM_DIR_PORTS)
#define DLB2_LDB_PC_BOUND DLB2_LDB_PC_OFFS(DLB2_MAX_NUM_LDB_PORTS)
#define DLB2_DIR_PC_BOUND DLB2_DIR_PC_OFFS(DLB2_MAX_NUM_DIR_PORTS)

static int
dlb2_pf_mmap(struct file *f,
	     struct vm_area_struct *vma,
	     u32 domain_id)
{
	unsigned long bar_pgoff;
	struct dlb2_dev *dev;
	unsigned long offset;
	struct page *page;
	pgprot_t pgprot;
	u32 port_id;

	dev = container_of(f->f_inode->i_cdev, struct dlb2_dev, cdev);

	offset = vma->vm_pgoff << PAGE_SHIFT;

	if (offset >= DLB2_LDB_CQ_BASE && offset < DLB2_LDB_CQ_BOUND) {
		if ((vma->vm_end - vma->vm_start) != DLB2_LDB_CQ_MAX_SIZE)
			return -EINVAL;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

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

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

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

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - DLB2_LDB_PP_BASE) / PAGE_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		pgprot = pgprot_noncached(vma->vm_page_prot);

	} else if (offset >= DLB2_DIR_PP_BASE && offset < DLB2_DIR_PP_BOUND) {
		if ((vma->vm_end - vma->vm_start) != PAGE_SIZE)
			return -EINVAL;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

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
static int
dlb2_mbox_version_supported(u16 min, u16 max)
{
	/* Only version 1 exists at this time */
	if (min > DLB2_MBOX_INTERFACE_VERSION)
		return -1;

	return DLB2_MBOX_INTERFACE_VERSION;
}

static void
dlb2_mbox_cmd_register_fn(struct dlb2_dev *dev,
			  int vf_id,
			  void *data)
{
	struct dlb2_mbox_register_cmd_resp resp = { {0} };
	struct dlb2_mbox_register_cmd_req *req = data;
	int ret;

	/* Given an interface version range ('min' to 'max', inclusive)
	 * requested by the VF driver:
	 * - If PF supports any versions in that range, it returns the newest
	 *   supported version.
	 * - Else PF responds with MBOX_ST_VERSION_MISMATCH
	 */
	ret = dlb2_mbox_version_supported(req->min_interface_version,
					  req->max_interface_version);

	if (ret == -1) {
		resp.hdr.status = DLB2_MBOX_ST_VERSION_MISMATCH;
		resp.interface_version = DLB2_MBOX_INTERFACE_VERSION;

		goto done;
	}

	resp.interface_version = ret;

	/* S-IOV vdev locking is handled in the VDCM */
	if (dlb2_hw_get_virt_mode(&dev->hw) == DLB2_VIRT_SRIOV)
		dlb2_lock_vdev(&dev->hw, vf_id);

	/* The VF can re-register without sending an unregister mailbox
	 * command (for example if the guest OS crashes). To protect against
	 * this, reset any in-use resources assigned to the driver now.
	 */
	dlb2_reset_vdev(&dev->hw, vf_id);

	dev->vf_registered[vf_id] = true;

	resp.pf_id = dev->id;
	resp.vf_id = vf_id;
	resp.is_auxiliary_vf = dev->child_id_state[vf_id].is_auxiliary_vf;
	resp.primary_vf_id = dev->child_id_state[vf_id].primary_vf_id;
	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;

done:
	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_unregister_fn(struct dlb2_dev *dev,
			    int vf_id,
			    void *data)
{
	struct dlb2_mbox_unregister_cmd_resp resp = { {0} };

	dev->vf_registered[vf_id] = false;

	dlb2_reset_vdev(&dev->hw, vf_id);

	/* S-IOV vdev locking is handled in the VDCM */
	if (dlb2_hw_get_virt_mode(&dev->hw) == DLB2_VIRT_SRIOV)
		dlb2_unlock_vdev(&dev->hw, vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_get_num_resources_fn(struct dlb2_dev *dev,
				   int vf_id,
				   void *data)
{
	struct dlb2_mbox_get_num_resources_cmd_resp resp = { {0} };
	struct dlb2_get_num_resources_args arg;
	int ret;

	ret = dlb2_hw_get_num_resources(&dev->hw, &arg, true, vf_id);

	resp.num_sched_domains = arg.num_sched_domains;
	resp.num_ldb_queues = arg.num_ldb_queues;
	resp.num_ldb_ports = arg.num_ldb_ports;
	resp.num_cos0_ldb_ports = arg.num_cos0_ldb_ports;
	resp.num_cos1_ldb_ports = arg.num_cos1_ldb_ports;
	resp.num_cos2_ldb_ports = arg.num_cos2_ldb_ports;
	resp.num_cos3_ldb_ports = arg.num_cos3_ldb_ports;
	resp.num_dir_ports = arg.num_dir_ports;
	resp.num_atomic_inflights = arg.num_atomic_inflights;
	resp.num_hist_list_entries = arg.num_hist_list_entries;
	resp.max_contiguous_hist_list_entries =
		arg.max_contiguous_hist_list_entries;
	resp.num_ldb_credits = arg.num_ldb_credits;
	resp.num_dir_credits = arg.num_dir_credits;

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_create_sched_domain_fn(struct dlb2_dev *dev,
				     int vf_id,
				     void *data)
{
	struct dlb2_mbox_create_sched_domain_cmd_resp resp = { {0} };
	struct dlb2_mbox_create_sched_domain_cmd_req *req = data;
	struct dlb2_create_sched_domain_args hw_arg;
	struct dlb2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.num_ldb_queues = req->num_ldb_queues;
	hw_arg.num_ldb_ports = req->num_ldb_ports;
	hw_arg.num_cos0_ldb_ports = req->num_cos0_ldb_ports;
	hw_arg.num_cos1_ldb_ports = req->num_cos1_ldb_ports;
	hw_arg.num_cos2_ldb_ports = req->num_cos2_ldb_ports;
	hw_arg.num_cos3_ldb_ports = req->num_cos3_ldb_ports;
	hw_arg.num_dir_ports = req->num_dir_ports;
	hw_arg.num_hist_list_entries = req->num_hist_list_entries;
	hw_arg.num_atomic_inflights = req->num_atomic_inflights;
	hw_arg.num_ldb_credits = req->num_ldb_credits;
	hw_arg.num_dir_credits = req->num_dir_credits;
	hw_arg.cos_strict = req->cos_strict;

	ret = dlb2_hw_create_sched_domain(&dev->hw,
					  &hw_arg,
					  &hw_response,
					  true,
					  vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_reset_sched_domain_fn(struct dlb2_dev *dev,
				    int vf_id,
				    void *data)
{
	struct dlb2_mbox_reset_sched_domain_cmd_resp resp = { {0} };
	struct dlb2_mbox_reset_sched_domain_cmd_req *req = data;
	int ret;

	ret = dlb2_reset_domain(&dev->hw, req->id, true, vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_create_ldb_queue_fn(struct dlb2_dev *dev,
				  int vf_id,
				  void *data)
{
	struct dlb2_mbox_create_ldb_queue_cmd_resp resp = { {0} };
	struct dlb2_mbox_create_ldb_queue_cmd_req *req = data;
	struct dlb2_cmd_response hw_response = {0};
	struct dlb2_create_ldb_queue_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.num_sequence_numbers = req->num_sequence_numbers;
	hw_arg.num_qid_inflights = req->num_qid_inflights;
	hw_arg.num_atomic_inflights = req->num_atomic_inflights;
	hw_arg.lock_id_comp_level = req->lock_id_comp_level;
	hw_arg.depth_threshold = req->depth_threshold;

	ret = dlb2_hw_create_ldb_queue(&dev->hw,
				       req->domain_id,
				       &hw_arg,
				       &hw_response,
				       true,
				       vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_create_dir_queue_fn(struct dlb2_dev *dev,
				  int vf_id,
				  void *data)
{
	struct dlb2_mbox_create_dir_queue_cmd_resp resp = { {0} };
	struct dlb2_mbox_create_dir_queue_cmd_req *req;
	struct dlb2_cmd_response hw_response = {0};
	struct dlb2_create_dir_queue_args hw_arg;
	int ret;

	req = (struct dlb2_mbox_create_dir_queue_cmd_req *)data;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;
	hw_arg.depth_threshold = req->depth_threshold;

	ret = dlb2_hw_create_dir_queue(&dev->hw,
				       req->domain_id,
				       &hw_arg,
				       &hw_response,
				       true,
				       vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_create_ldb_port_fn(struct dlb2_dev *dev,
				 int vf_id,
				 void *data)
{
	struct dlb2_mbox_create_ldb_port_cmd_resp resp = { {0} };
	struct dlb2_mbox_create_ldb_port_cmd_req *req = data;
	struct dlb2_cmd_response hw_response = {0};
	struct dlb2_create_ldb_port_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.cq_depth = req->cq_depth;
	hw_arg.cq_history_list_size = req->cq_history_list_size;
	hw_arg.cos_id = req->cos_id;
	hw_arg.cos_strict = req->cos_strict;

	ret = dlb2_hw_create_ldb_port(&dev->hw,
				      req->domain_id,
				      &hw_arg,
				      req->cq_base_address,
				      &hw_response,
				      true,
				      vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_create_dir_port_fn(struct dlb2_dev *dev,
				 int vf_id,
				 void *data)
{
	struct dlb2_mbox_create_dir_port_cmd_resp resp = { {0} };
	struct dlb2_mbox_create_dir_port_cmd_req *req;
	struct dlb2_cmd_response hw_response = {0};
	struct dlb2_create_dir_port_args hw_arg;
	int ret;

	req = (struct dlb2_mbox_create_dir_port_cmd_req *)data;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.cq_depth = req->cq_depth;
	hw_arg.queue_id = req->queue_id;

	ret = dlb2_hw_create_dir_port(&dev->hw,
				      req->domain_id,
				      &hw_arg,
				      req->cq_base_address,
				      &hw_response,
				      true,
				      vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_enable_ldb_port_fn(struct dlb2_dev *dev,
				 int vf_id,
				 void *data)
{
	struct dlb2_mbox_enable_ldb_port_cmd_resp resp = { {0} };
	struct dlb2_mbox_enable_ldb_port_cmd_req *req = data;
	struct dlb2_cmd_response hw_response = {0};
	struct dlb2_enable_ldb_port_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	ret = dlb2_hw_enable_ldb_port(&dev->hw,
				      req->domain_id,
				      &hw_arg,
				      &hw_response,
				      true,
				      vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;

	resp.error_code = ret;
	resp.status = hw_response.status;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_disable_ldb_port_fn(struct dlb2_dev *dev,
				  int vf_id,
				  void *data)
{
	struct dlb2_mbox_disable_ldb_port_cmd_resp resp = { {0} };
	struct dlb2_mbox_disable_ldb_port_cmd_req *req = data;
	struct dlb2_cmd_response hw_response = {0};
	struct dlb2_disable_ldb_port_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	ret = dlb2_hw_disable_ldb_port(&dev->hw,
				       req->domain_id,
				       &hw_arg,
				       &hw_response,
				       true,
				       vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_enable_dir_port_fn(struct dlb2_dev *dev,
				 int vf_id,
				 void *data)
{
	struct dlb2_mbox_enable_dir_port_cmd_resp resp = { {0} };
	struct dlb2_mbox_enable_dir_port_cmd_req *req = data;
	struct dlb2_cmd_response hw_response = {0};
	struct dlb2_enable_dir_port_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	ret = dlb2_hw_enable_dir_port(&dev->hw,
				      req->domain_id,
				      &hw_arg,
				      &hw_response,
				      true,
				      vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_disable_dir_port_fn(struct dlb2_dev *dev,
				  int vf_id,
				  void *data)
{
	struct dlb2_mbox_disable_dir_port_cmd_resp resp = { {0} };
	struct dlb2_mbox_disable_dir_port_cmd_req *req = data;
	struct dlb2_cmd_response hw_response = {0};
	struct dlb2_disable_dir_port_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	ret = dlb2_hw_disable_dir_port(&dev->hw,
				       req->domain_id,
				       &hw_arg,
				       &hw_response,
				       true,
				       vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_ldb_port_owned_by_domain_fn(struct dlb2_dev *dev,
					  int vf_id,
					  void *data)
{
	struct dlb2_mbox_ldb_port_owned_by_domain_cmd_resp resp = { {0} };
	struct dlb2_mbox_ldb_port_owned_by_domain_cmd_req *req = data;
	int ret;

	ret = dlb2_ldb_port_owned_by_domain(&dev->hw,
					    req->domain_id,
					    req->port_id,
					    true,
					    vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.owned = ret;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_dir_port_owned_by_domain_fn(struct dlb2_dev *dev,
					  int vf_id,
					  void *data)
{
	struct dlb2_mbox_dir_port_owned_by_domain_cmd_resp resp = { {0} };
	struct dlb2_mbox_dir_port_owned_by_domain_cmd_req *req = data;
	int ret;

	ret = dlb2_dir_port_owned_by_domain(&dev->hw,
					    req->domain_id,
					    req->port_id,
					    true,
					    vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.owned = ret;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_map_qid_fn(struct dlb2_dev *dev,
			 int vf_id,
			 void *data)
{
	struct dlb2_mbox_map_qid_cmd_resp resp = { {0} };
	struct dlb2_mbox_map_qid_cmd_req *req = data;
	struct dlb2_cmd_response hw_response = {0};
	struct dlb2_map_qid_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;
	hw_arg.qid = req->qid;
	hw_arg.priority = req->priority;

	ret = dlb2_hw_map_qid(&dev->hw,
			      req->domain_id,
			      &hw_arg,
			      &hw_response,
			      true,
			      vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_unmap_qid_fn(struct dlb2_dev *dev,
			   int vf_id,
			   void *data)
{
	struct dlb2_mbox_unmap_qid_cmd_resp resp = { {0} };
	struct dlb2_mbox_unmap_qid_cmd_req *req = data;
	struct dlb2_cmd_response hw_response = {0};
	struct dlb2_unmap_qid_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;
	hw_arg.qid = req->qid;

	ret = dlb2_hw_unmap_qid(&dev->hw,
				req->domain_id,
				&hw_arg,
				&hw_response,
				true,
				vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_start_domain_fn(struct dlb2_dev *dev,
			      int vf_id,
			      void *data)
{
	struct dlb2_mbox_start_domain_cmd_resp resp = { {0} };
	struct dlb2_mbox_start_domain_cmd_req *req = data;
	struct dlb2_cmd_response hw_response = {0};
	struct dlb2_start_domain_args hw_arg;
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;

	ret = dlb2_hw_start_domain(&dev->hw,
				   req->domain_id,
				   &hw_arg,
				   &hw_response,
				   true,
				   vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_enable_ldb_port_intr_fn(struct dlb2_dev *dev,
				      int vf_id,
				      void *data)
{
	struct dlb2_mbox_enable_ldb_port_intr_cmd_resp resp = { {0} };
	struct dlb2_mbox_enable_ldb_port_intr_cmd_req *req = data;
	int ret, mode;

	if (req->owner_vf >= DLB2_MAX_NUM_VDEVS ||
	    (dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     dev->child_id_state[req->owner_vf].primary_vf_id != vf_id) ||
	    (!dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     req->owner_vf != vf_id)) {
		resp.hdr.status = DLB2_MBOX_ST_INVALID_OWNER_VF;
		goto finish;
	}

	if (dlb2_hw_get_virt_mode(&dev->hw) == DLB2_VIRT_SRIOV)
		mode = DLB2_CQ_ISR_MODE_MSI;
	else
		mode = DLB2_CQ_ISR_MODE_ADI;

	ret = dlb2_configure_ldb_cq_interrupt(&dev->hw,
					      req->port_id,
					      req->vector,
					      mode,
					      vf_id,
					      req->owner_vf,
					      req->thresh);

	if (ret == 0 && !dlb2_wdto_disable)
		ret = dlb2_hw_enable_ldb_cq_wd_int(&dev->hw,
						   req->port_id,
						   true,
						   vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

finish:
	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_enable_dir_port_intr_fn(struct dlb2_dev *dev,
				      int vf_id,
				      void *data)
{
	struct dlb2_mbox_enable_dir_port_intr_cmd_resp resp = { {0} };
	struct dlb2_mbox_enable_dir_port_intr_cmd_req *req = data;
	int ret, mode;

	if (req->owner_vf >= DLB2_MAX_NUM_VDEVS ||
	    (dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     dev->child_id_state[req->owner_vf].primary_vf_id != vf_id) ||
	    (!dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     req->owner_vf != vf_id)) {
		resp.hdr.status = DLB2_MBOX_ST_INVALID_OWNER_VF;
		goto finish;
	}

	if (dlb2_hw_get_virt_mode(&dev->hw) == DLB2_VIRT_SRIOV)
		mode = DLB2_CQ_ISR_MODE_MSI;
	else
		mode = DLB2_CQ_ISR_MODE_ADI;

	ret = dlb2_configure_dir_cq_interrupt(&dev->hw,
					      req->port_id,
					      req->vector,
					      mode,
					      vf_id,
					      req->owner_vf,
					      req->thresh);

	if (ret == 0 && !dlb2_wdto_disable)
		ret = dlb2_hw_enable_dir_cq_wd_int(&dev->hw,
						   req->port_id,
						   true,
						   vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

finish:
	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_arm_cq_intr_fn(struct dlb2_dev *dev,
			     int vf_id,
			     void *data)
{
	struct dlb2_mbox_arm_cq_intr_cmd_resp resp = { {0} };
	struct dlb2_mbox_arm_cq_intr_cmd_req *req = data;
	int ret;

	if (req->is_ldb)
		ret = dlb2_ldb_port_owned_by_domain(&dev->hw,
						    req->domain_id,
						    req->port_id,
						    true,
						    vf_id);
	else
		ret = dlb2_dir_port_owned_by_domain(&dev->hw,
						    req->domain_id,
						    req->port_id,
						    true,
						    vf_id);

	if (ret != 1) {
		resp.error_code = -EINVAL;
		goto finish;
	}

	resp.error_code = dlb2_arm_cq_interrupt(&dev->hw,
						req->port_id,
						req->is_ldb,
						true,
						vf_id);

finish:
	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_get_num_used_resources_fn(struct dlb2_dev *dev,
					int vf_id,
					void *data)
{
	struct dlb2_mbox_get_num_resources_cmd_resp resp = { {0} };
	struct dlb2_get_num_resources_args arg;
	int ret;

	ret = dlb2_hw_get_num_used_resources(&dev->hw, &arg, true, vf_id);

	resp.num_sched_domains = arg.num_sched_domains;
	resp.num_ldb_queues = arg.num_ldb_queues;
	resp.num_ldb_ports = arg.num_ldb_ports;
	resp.num_cos0_ldb_ports = arg.num_cos0_ldb_ports;
	resp.num_cos1_ldb_ports = arg.num_cos1_ldb_ports;
	resp.num_cos2_ldb_ports = arg.num_cos2_ldb_ports;
	resp.num_cos3_ldb_ports = arg.num_cos3_ldb_ports;
	resp.num_dir_ports = arg.num_dir_ports;
	resp.num_atomic_inflights = arg.num_atomic_inflights;
	resp.num_hist_list_entries = arg.num_hist_list_entries;
	resp.max_contiguous_hist_list_entries =
		arg.max_contiguous_hist_list_entries;
	resp.num_ldb_credits = arg.num_ldb_credits;
	resp.num_dir_credits = arg.num_dir_credits;

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_get_sn_allocation_fn(struct dlb2_dev *dev,
				   int vf_id,
				   void *data)
{
	struct dlb2_mbox_get_sn_allocation_cmd_resp resp = { {0} };
	struct dlb2_mbox_get_sn_allocation_cmd_req *req = data;

	resp.num = dlb2_get_group_sequence_numbers(&dev->hw, req->group_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_get_sn_occupancy_fn(struct dlb2_dev *dev,
				  int vf_id,
				  void *data)
{
	struct dlb2_mbox_get_sn_occupancy_cmd_resp resp = { {0} };
	struct dlb2_mbox_get_sn_occupancy_cmd_req *req = data;

	resp.num = dlb2_get_group_sequence_number_occupancy(&dev->hw,
							    req->group_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_get_ldb_queue_depth_fn(struct dlb2_dev *dev,
				     int vf_id,
				     void *data)
{
	struct dlb2_mbox_get_ldb_queue_depth_cmd_resp resp = { {0} };
	struct dlb2_mbox_get_ldb_queue_depth_cmd_req *req = data;
	struct dlb2_get_ldb_queue_depth_args hw_arg;
	struct dlb2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.queue_id = req->queue_id;

	ret = dlb2_hw_get_ldb_queue_depth(&dev->hw,
					  req->domain_id,
					  &hw_arg,
					  &hw_response,
					  true,
					  vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.depth = hw_response.id;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_get_dir_queue_depth_fn(struct dlb2_dev *dev,
				     int vf_id,
				     void *data)
{
	struct dlb2_mbox_get_dir_queue_depth_cmd_resp resp = { {0} };
	struct dlb2_mbox_get_dir_queue_depth_cmd_req *req = data;
	struct dlb2_get_dir_queue_depth_args hw_arg;
	struct dlb2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.queue_id = req->queue_id;

	ret = dlb2_hw_get_dir_queue_depth(&dev->hw,
					  req->domain_id,
					  &hw_arg,
					  &hw_response,
					  true,
					  vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.depth = hw_response.id;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_pending_port_unmaps_fn(struct dlb2_dev *dev,
				     int vf_id,
				     void *data)
{
	struct dlb2_mbox_pending_port_unmaps_cmd_resp resp = { {0} };
	struct dlb2_mbox_pending_port_unmaps_cmd_req *req = data;
	struct dlb2_pending_port_unmaps_args hw_arg;
	struct dlb2_cmd_response hw_response = {0};
	int ret;

	hw_arg.port_id = req->port_id;

	ret = dlb2_hw_pending_port_unmaps(&dev->hw,
					  req->domain_id,
					  &hw_arg,
					  &hw_response,
					  true,
					  vf_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.num = hw_response.id;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void
dlb2_mbox_cmd_get_cos_bw_fn(struct dlb2_dev *dev,
			    int vf_id,
			    void *data)
{
	struct dlb2_mbox_get_cos_bw_cmd_resp resp = { {0} };
	struct dlb2_mbox_get_cos_bw_cmd_req *req = data;

	resp.num = dlb2_hw_get_cos_bandwidth(&dev->hw, req->cos_id);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static bool
dlb2_sparse_cq_enabled;

static int
dlb2_pf_query_cq_poll_mode(struct dlb2_dev *dlb2_dev,
			   struct dlb2_cmd_response *user_resp)
{
	user_resp->status = 0;
	user_resp->id = DLB2_CQ_POLL_MODE_SPARSE;

	dlb2_sparse_cq_enabled = true;

	/* TEMPORARY: Only enable sparse CQ mode if the poll mode is queried,
	 * rather than enabling by default. This will be removed when the PMD
	 * adds support for sparse CQ mode.
	 */
	dlb2_hw_enable_sparse_ldb_cq_mode(&dlb2_dev->hw);

	dlb2_hw_enable_sparse_dir_cq_mode(&dlb2_dev->hw);

	return 0;
}

static void
dlb2_mbox_cmd_query_cq_poll_mode_fn(struct dlb2_dev *dev,
				    int vf_id,
				    void *data)
{
	struct dlb2_mbox_query_cq_poll_mode_cmd_resp resp = { {0} };
	struct dlb2_cmd_response response = {0};
	int ret;

	ret = dlb2_pf_query_cq_poll_mode(dev, &response);

	resp.hdr.status = DLB2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = response.status;
	resp.mode = response.id;

	dlb2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void (*mbox_fn_table[])(struct dlb2_dev *dev, int vf_id, void *data) = {
	dlb2_mbox_cmd_register_fn,
	dlb2_mbox_cmd_unregister_fn,
	dlb2_mbox_cmd_get_num_resources_fn,
	dlb2_mbox_cmd_create_sched_domain_fn,
	dlb2_mbox_cmd_reset_sched_domain_fn,
	dlb2_mbox_cmd_create_ldb_queue_fn,
	dlb2_mbox_cmd_create_dir_queue_fn,
	dlb2_mbox_cmd_create_ldb_port_fn,
	dlb2_mbox_cmd_create_dir_port_fn,
	dlb2_mbox_cmd_enable_ldb_port_fn,
	dlb2_mbox_cmd_disable_ldb_port_fn,
	dlb2_mbox_cmd_enable_dir_port_fn,
	dlb2_mbox_cmd_disable_dir_port_fn,
	dlb2_mbox_cmd_ldb_port_owned_by_domain_fn,
	dlb2_mbox_cmd_dir_port_owned_by_domain_fn,
	dlb2_mbox_cmd_map_qid_fn,
	dlb2_mbox_cmd_unmap_qid_fn,
	dlb2_mbox_cmd_start_domain_fn,
	dlb2_mbox_cmd_enable_ldb_port_intr_fn,
	dlb2_mbox_cmd_enable_dir_port_intr_fn,
	dlb2_mbox_cmd_arm_cq_intr_fn,
	dlb2_mbox_cmd_get_num_used_resources_fn,
	dlb2_mbox_cmd_get_sn_allocation_fn,
	dlb2_mbox_cmd_get_ldb_queue_depth_fn,
	dlb2_mbox_cmd_get_dir_queue_depth_fn,
	dlb2_mbox_cmd_pending_port_unmaps_fn,
	dlb2_mbox_cmd_get_cos_bw_fn,
	dlb2_mbox_cmd_get_sn_occupancy_fn,
	dlb2_mbox_cmd_query_cq_poll_mode_fn,
};

static u32
dlb2_handle_vf_flr_interrupt(struct dlb2_dev *dev)
{
	u32 bitvec;
	int i;

	bitvec = dlb2_read_vf_flr_int_bitvec(&dev->hw);

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		if (!(bitvec & (1 << i)))
			continue;

		dev_dbg(dev->dlb2_device,
			"Received VF FLR ISR from VF %d\n",
			i);

		dlb2_reset_vdev(&dev->hw, i);
	}

	dlb2_ack_vf_flr_int(&dev->hw, bitvec);

	return bitvec;
}

/**********************************/
/****** Interrupt management ******/
/**********************************/

void dlb2_handle_mbox_interrupt(struct dlb2_dev *dev, int id)
{
	u8 data[DLB2_VF2PF_REQ_BYTES];

	dev_dbg(dev->dlb2_device, "Received VF->PF ISR from VF %d\n", id);

	dlb2_pf_read_vf_mbox_req(&dev->hw, id, data, sizeof(data));

	/* Unrecognized request command, send an error response */
	if (DLB2_MBOX_CMD_TYPE(data) >= NUM_DLB2_MBOX_CMD_TYPES) {
		struct dlb2_mbox_resp_hdr resp = {0};

		resp.status = DLB2_MBOX_ST_INVALID_CMD_TYPE;

		dlb2_pf_write_vf_mbox_resp(&dev->hw,
					   id,
					   &resp,
					   sizeof(resp));
	} else {
		dev_dbg(dev->dlb2_device,
			"Received mbox command %s\n",
			DLB2_MBOX_CMD_STRING(data));

		mbox_fn_table[DLB2_MBOX_CMD_TYPE(data)](dev, id, data);
	}

	dlb2_ack_vdev_mbox_int(&dev->hw, 1 << id);
}

static u32
dlb2_handle_vf_to_pf_interrupt(struct dlb2_dev *dev)
{
	u32 bitvec;
	int i;

	bitvec = dlb2_read_vdev_to_pf_int_bitvec(&dev->hw);

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		if (!(bitvec & (1 << i)))
			continue;

		dlb2_handle_mbox_interrupt(dev, i);
	}

	return bitvec;
}

static u32
dlb2_handle_vf_requests(struct dlb2_hw *hw)
{
	struct dlb2_dev *dev;
	u32 mbox_bitvec;
	u32 flr_bitvec;

	dev = container_of(hw, struct dlb2_dev, hw);

	flr_bitvec = dlb2_handle_vf_flr_interrupt(dev);

	mbox_bitvec = dlb2_handle_vf_to_pf_interrupt(dev);

	dlb2_ack_vdev_to_pf_int(hw, mbox_bitvec, flr_bitvec);

	return mbox_bitvec | flr_bitvec;
}

static void dlb2_detect_ingress_err_overload(struct dlb2_dev *dev)
{
	s64 delta_us;

	if (dev->ingress_err.count == 0)
		dev->ingress_err.ts = ktime_get();

	dev->ingress_err.count++;

	/* Don't check for overload until OVERLOAD_THRESH ISRs have run */
	if (dev->ingress_err.count < DLB2_ISR_OVERLOAD_THRESH)
		return;

	delta_us = ktime_us_delta(ktime_get(), dev->ingress_err.ts);

	/* Reset stats for next measurement period */
	dev->ingress_err.count = 0;
	dev->ingress_err.ts = ktime_get();

	/* Check for overload during this measurement period */
	if (delta_us > DLB2_ISR_OVERLOAD_PERIOD_S * USEC_PER_SEC)
		return;

	/* Alarm interrupt overload: disable software-generated alarms,
	 * so only hardware problems (e.g. ECC errors) interrupt the PF.
	 */
	dlb2_disable_ingress_error_alarms(&dev->hw);

	dev->ingress_err.enabled = false;

	dev_err(dev->dlb2_device,
		"[%s()] Overloaded detected: disabling ingress error interrupts",
		__func__);
}

static void dlb2_detect_mbox_overload(struct dlb2_dev *dev, int id)
{
	union dlb2_iosf_func_vf_bar_dsbl r0 = { {0} };
	s64 delta_us;

	if (dev->mbox[id].count == 0)
		dev->mbox[id].ts = ktime_get();

	dev->mbox[id].count++;

	/* Don't check for overload until OVERLOAD_THRESH ISRs have run */
	if (dev->mbox[id].count < DLB2_ISR_OVERLOAD_THRESH)
		return;

	delta_us = ktime_us_delta(ktime_get(), dev->mbox[id].ts);

	/* Reset stats for next measurement period */
	dev->mbox[id].count = 0;
	dev->mbox[id].ts = ktime_get();

	/* Check for overload during this measurement period */
	if (delta_us > DLB2_ISR_OVERLOAD_PERIOD_S * USEC_PER_SEC)
		return;

	/* Mailbox interrupt overload: disable the VF FUNC BAR to prevent
	 * further abuse. The FUNC BAR is re-enabled when the device is reset
	 * or the driver is reloaded.
	 */
	r0.field.func_vf_bar_dis = 1;

	DLB2_CSR_WR(&dev->hw, DLB2_IOSF_FUNC_VF_BAR_DSBL(id), r0.val);

	dev->mbox[id].enabled = false;

	dev_err(dev->dlb2_device,
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
static irqreturn_t
dlb2_service_intr_handler(int irq, void *hdlr_ptr)
{
	struct dlb2_dev *dev = (struct dlb2_dev *)hdlr_ptr;
	union dlb2_sys_alarm_hw_synd r0;
	u32 bitvec;
	int i;

	dev_dbg(dev->dlb2_device, "DLB service interrupt fired\n");

	mutex_lock(&dev->resource_mutex);

	r0.val = DLB2_CSR_RD(&dev->hw, DLB2_SYS_ALARM_HW_SYND);

	/* Clear the MSI-X ack bit before processing the VF->PF or watchdog
	 * timer interrupts. This order is necessary so that if an interrupt
	 * event arrives after reading the corresponding bit vector, the event
	 * won't be lost.
	 */
	dlb2_ack_msix_interrupt(&dev->hw, DLB2_INT_NON_CQ);

	if (r0.field.alarm & r0.field.valid)
		dlb2_process_alarm_interrupt(&dev->hw);

	if (dlb2_process_ingress_error_interrupt(&dev->hw))
		dlb2_detect_ingress_err_overload(dev);

	if (r0.field.cwd & r0.field.valid)
		dlb2_process_wdt_interrupt(&dev->hw);

	if (r0.field.vf_pf_mb & r0.field.valid) {
		bitvec = dlb2_handle_vf_requests(&dev->hw);
		for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
			if (bitvec & (1 << i))
				dlb2_detect_mbox_overload(dev, i);
		}
	}

	mutex_unlock(&dev->resource_mutex);

	return IRQ_HANDLED;
}

static int
dlb2_init_alarm_interrupts(struct dlb2_dev *dev,
			   struct pci_dev *pdev)
{
	int i, ret;

	for (i = 0; i < DLB2_PF_NUM_NON_CQ_INTERRUPT_VECTORS; i++) {
		ret = devm_request_threaded_irq(&pdev->dev,
						pci_irq_vector(pdev, i),
						NULL,
						dlb2_service_intr_handler,
						IRQF_ONESHOT,
						"dlb2_alarm",
						dev);
		if (ret)
			return ret;

		dev->intr.isr_registered[i] = true;
	}

	dlb2_enable_ingress_error_alarms(&dev->hw);

	return 0;
}

static irqreturn_t
dlb2_compressed_cq_intr_handler(int irq, void *hdlr_ptr)
{
	struct dlb2_dev *dev = (struct dlb2_dev *)hdlr_ptr;
	u32 ldb_cq_interrupts[DLB2_MAX_NUM_LDB_PORTS / 32];
	u32 dir_cq_interrupts[DLB2_MAX_NUM_DIR_PORTS / 32];
	int i;

	dev_dbg(dev->dlb2_device, "Entered ISR\n");

	dlb2_read_compressed_cq_intr_status(&dev->hw,
					    ldb_cq_interrupts,
					    dir_cq_interrupts);

	dlb2_ack_compressed_cq_intr(&dev->hw,
				    ldb_cq_interrupts,
				    dir_cq_interrupts);

	for (i = 0; i < DLB2_MAX_NUM_LDB_PORTS; i++) {
		if (!(ldb_cq_interrupts[i / 32] & (1 << (i % 32))))
			continue;

		dev_dbg(dev->dlb2_device, "[%s()] Waking LDB port %d\n",
			__func__, i);

		dlb2_wake_thread(dev, &dev->intr.ldb_cq_intr[i], WAKE_CQ_INTR);
	}

	for (i = 0; i < DLB2_MAX_NUM_DIR_PORTS; i++) {
		if (!(dir_cq_interrupts[i / 32] & (1 << (i % 32))))
			continue;

		dev_dbg(dev->dlb2_device, "[%s()] Waking DIR port %d\n",
			__func__, i);

		dlb2_wake_thread(dev, &dev->intr.dir_cq_intr[i], WAKE_CQ_INTR);
	}

	return IRQ_HANDLED;
}

static int
dlb2_init_compressed_mode_interrupts(struct dlb2_dev *dev,
				     struct pci_dev *pdev)
{
	int ret, irq;

	irq = pci_irq_vector(pdev, DLB2_PF_COMPRESSED_MODE_CQ_VECTOR_ID);

	ret = devm_request_irq(&pdev->dev,
			       irq,
			       dlb2_compressed_cq_intr_handler,
			       0,
			       "dlb2_compressed_cq",
			       dev);
	if (ret)
		return ret;

	dev->intr.isr_registered[DLB2_PF_COMPRESSED_MODE_CQ_VECTOR_ID] = true;

	dev->intr.mode = DLB2_MSIX_MODE_COMPRESSED;

	dlb2_set_msix_mode(&dev->hw, DLB2_MSIX_MODE_COMPRESSED);

	return 0;
}

static void
dlb2_pf_free_interrupts(struct dlb2_dev *dev,
			struct pci_dev *pdev)
{
	int i;

	for (i = 0; i < dev->intr.num_vectors; i++) {
		if (dev->intr.isr_registered[i])
			devm_free_irq(&pdev->dev, pci_irq_vector(pdev, i), dev);
	}

	pci_free_irq_vectors(pdev);
}

static int
dlb2_pf_init_interrupts(struct dlb2_dev *dev, struct pci_dev *pdev)
{
	int ret, i;

	/* DLB supports two modes for CQ interrupts:
	 * - "compressed mode": all CQ interrupts are packed into a single
	 *	vector. The ISR reads six interrupt status registers to
	 *	determine the source(s).
	 * - "packed mode" (unused): the hardware supports up to 64 vectors.
	 */

	ret = pci_alloc_irq_vectors(pdev,
				    DLB2_PF_NUM_COMPRESSED_MODE_VECTORS,
				    DLB2_PF_NUM_COMPRESSED_MODE_VECTORS,
				    PCI_IRQ_MSIX);
	if (ret < 0)
		return ret;

	dev->intr.num_vectors = ret;
	dev->intr.base_vector = pci_irq_vector(pdev, 0);

	ret = dlb2_init_alarm_interrupts(dev, pdev);
	if (ret) {
		dlb2_pf_free_interrupts(dev, pdev);
		return ret;
	}

	ret = dlb2_init_compressed_mode_interrupts(dev, pdev);
	if (ret) {
		dlb2_pf_free_interrupts(dev, pdev);
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

/* If the device is reset during use, its interrupt registers need to be
 * reinitialized.
 */
static void
dlb2_pf_reinit_interrupts(struct dlb2_dev *dev)
{
	int i;

	/* Re-enable alarms after device reset */
	dlb2_enable_ingress_error_alarms(&dev->hw);

	if (!dev->ingress_err.enabled)
		dev_err(dev->dlb2_device,
			"[%s()] Re-enabling ingress error interrupts",
			__func__);

	dev->ingress_err.enabled = true;

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		if (!dev->mbox[i].enabled)
			dev_err(dev->dlb2_device,
				"[%s()] Re-enabling VF %d's FUNC BAR",
				__func__, i);

		dev->mbox[i].enabled = true;
	}

	dlb2_set_msix_mode(&dev->hw, DLB2_MSIX_MODE_COMPRESSED);
}

static int
dlb2_pf_enable_ldb_cq_interrupts(struct dlb2_dev *dev,
				 int id,
				 u16 thresh)
{
	int mode, vec, ret;

	if (dev->intr.mode == DLB2_MSIX_MODE_COMPRESSED) {
		mode = DLB2_CQ_ISR_MODE_MSIX;
		vec = 0;
	} else {
		mode = DLB2_CQ_ISR_MODE_MSIX;
		vec = id % 64;
	}

	dev->intr.ldb_cq_intr[id].disabled = false;
	dev->intr.ldb_cq_intr[id].configured = true;

	ret = dlb2_configure_ldb_cq_interrupt(&dev->hw, id, vec,
					      mode, 0, 0, thresh);

	if (ret || dlb2_wdto_disable)
		return ret;

	return dlb2_hw_enable_ldb_cq_wd_int(&dev->hw, id, false, 0);
}

static int
dlb2_pf_enable_dir_cq_interrupts(struct dlb2_dev *dev,
				 int id,
				 u16 thresh)
{
	int mode, vec, ret;

	if (dev->intr.mode == DLB2_MSIX_MODE_COMPRESSED) {
		mode = DLB2_CQ_ISR_MODE_MSIX;
		vec = 0;
	} else {
		mode = DLB2_CQ_ISR_MODE_MSIX;
		vec = id % 64;
	}

	dev->intr.dir_cq_intr[id].disabled = false;
	dev->intr.dir_cq_intr[id].configured = true;

	ret = dlb2_configure_dir_cq_interrupt(&dev->hw, id, vec,
					      mode, 0, 0, thresh);

	if (ret || dlb2_wdto_disable)
		return ret;

	return dlb2_hw_enable_dir_cq_wd_int(&dev->hw, id, false, 0);
}

static int
dlb2_pf_arm_cq_interrupt(struct dlb2_dev *dev,
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

	return dlb2_arm_cq_interrupt(&dev->hw, port_id, is_ldb, false, 0);
}

/*******************************/
/****** Driver management ******/
/*******************************/

static int
dlb2_pf_init_driver_state(struct dlb2_dev *dlb2_dev)
{
	int i, ret;

	dlb2_dev->wq = create_singlethread_workqueue("DLB queue remapper");
	if (!dlb2_dev->wq)
		return -EINVAL;

	if (movdir64b_supported()) {
		dlb2_dev->enqueue_four = dlb2_movdir64b;
	} else {
#ifdef CONFIG_AS_SSE2
		dlb2_dev->enqueue_four = dlb2_movntdq;
#else
		dev_err(dlb2_dev->dlb2_device,
			"%s: Platforms without movdir64 must support SSE2\n",
			dlb2_driver_name);
		ret = -EINVAL;
		goto enqueue_four_fail;
#endif
	}

#ifdef CONFIG_INTEL_DLB2_SIOV
	ret = dlb2_vdcm_init(dlb2_dev);
	if (ret)
		goto vdcm_init_fail;
#endif

	/* Initialize software state */
	for (i = 0; i < DLB2_MAX_NUM_DOMAINS; i++) {
		struct dlb2_domain_dev *domain = &dlb2_dev->sched_domains[i];

		mutex_init(&domain->alert_mutex);
		init_waitqueue_head(&domain->wq_head);
	}

	dlb2_dev->ingress_err.count = 0;
	dlb2_dev->ingress_err.enabled = true;

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		dlb2_dev->mbox[i].count = 0;
		dlb2_dev->mbox[i].enabled = true;
	}

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++)
		dlb2_dev->child_id_state[i].is_auxiliary_vf = false;

	dlb2_hw_set_virt_mode(&dlb2_dev->hw, DLB2_VIRT_NONE);

	mutex_init(&dlb2_dev->resource_mutex);
	mutex_init(&dlb2_dev->svc_isr_mutex);

	return 0;

#ifdef CONFIG_INTEL_DLB2_SIOV
vdcm_init_fail:
#endif
#ifndef CONFIG_AS_SSE2
enqueue_four_fail:
	destroy_workqueue(dlb2_dev->wq);
#endif
	return ret;
}

static void
dlb2_pf_free_driver_state(struct dlb2_dev *dlb2_dev)
{
#ifdef CONFIG_INTEL_DLB2_SIOV
	dlb2_vdcm_exit(dlb2_dev->pdev);
#endif

	destroy_workqueue(dlb2_dev->wq);
}

static int
dlb2_pf_cdev_add(struct dlb2_dev *dlb2_dev,
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
dlb2_pf_cdev_del(struct dlb2_dev *dlb2_dev)
{
	cdev_del(&dlb2_dev->cdev);
}

static int
dlb2_pf_device_create(struct dlb2_dev *dlb2_dev,
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
dlb2_pf_device_destroy(struct dlb2_dev *dlb2_dev,
		       struct class *dlb2_class)
{
	device_destroy(dlb2_class,
		       MKDEV(MAJOR(dlb2_dev->dev_number),
			     MINOR(dlb2_dev->dev_number) +
			     DLB2_MAX_NUM_DOMAINS));
}

static int
dlb2_pf_register_driver(struct dlb2_dev *dlb2_dev)
{
	/* Function intentionally left blank */
	return 0;
}

static void
dlb2_pf_unregister_driver(struct dlb2_dev *dlb2_dev)
{
	/* Function intentionally left blank */
}

static void
dlb2_pf_enable_pm(struct dlb2_dev *dlb2_dev)
{
	dlb2_clr_pmcsr_disable(&dlb2_dev->hw);
}

#define DLB2_READY_RETRY_LIMIT 1000
static int
dlb2_pf_wait_for_device_ready(struct dlb2_dev *dlb2_dev,
			      struct pci_dev *pdev)
{
	u32 retries = DLB2_READY_RETRY_LIMIT;

	/* Allow at least 1s for the device to become active after power-on */
	do {
		union dlb2_cfg_mstr_cfg_diagnostic_idle_status idle;
		union dlb2_cfg_mstr_cfg_pm_status pm_st;
		u32 addr;

		addr = DLB2_CFG_MSTR_CFG_PM_STATUS;

		pm_st.val = DLB2_CSR_RD(&dlb2_dev->hw, addr);

		addr = DLB2_CFG_MSTR_CFG_DIAGNOSTIC_IDLE_STATUS;

		idle.val = DLB2_CSR_RD(&dlb2_dev->hw, addr);

		if (pm_st.field.pmsm == 1 && idle.field.dlb_func_idle == 1)
			break;

		dev_dbg(&pdev->dev, "[%s()] pm_st: 0x%x\n",
			__func__, pm_st.val);
		dev_dbg(&pdev->dev, "[%s()] idle: 0x%x\n",
			__func__, idle.val);

		usleep_range(1000, 2000);
	} while (--retries);

	if (!retries) {
		dev_err(&pdev->dev, "Device idle test failed\n");
		return -EIO;
	}

	return 0;
}

static void dlb2_pf_calc_arbiter_weights(struct dlb2_hw *hw,
					 u8 *weight,
					 unsigned int pct)
{
	int val, i;

	/* Largest possible weight (100% SA case): 32 */
	val = (DLB2_MAX_WEIGHT + 1) / DLB2_NUM_ARB_WEIGHTS;

	/* Scale val according to the starvation avoidance percentage */
	val = (val * pct) / 100;
	if (val == 0 && pct != 0)
		val = 1;

	/* Prio 7 always has weight 0xff */
	weight[DLB2_NUM_ARB_WEIGHTS - 1] = DLB2_MAX_WEIGHT;

	for (i = DLB2_NUM_ARB_WEIGHTS - 2; i >= 0; i--)
		weight[i] = weight[i + 1] - val;
}

static void
dlb2_pf_init_hardware(struct dlb2_dev *dlb2_dev)
{
	if (!dlb2_wdto_disable)
		dlb2_hw_enable_wd_timer(&dlb2_dev->hw, DLB2_WD_TMO_10S);

	if (dlb2_sparse_cq_enabled) {
		dlb2_hw_enable_sparse_ldb_cq_mode(&dlb2_dev->hw);

		dlb2_hw_enable_sparse_dir_cq_mode(&dlb2_dev->hw);
	}

	/* Configure arbitration weights for QE selection */
	if (dlb2_qe_sa_pct <= 100) {
		u8 weight[DLB2_NUM_ARB_WEIGHTS];

		dlb2_pf_calc_arbiter_weights(&dlb2_dev->hw,
					     weight,
					     dlb2_qe_sa_pct);

		dlb2_hw_set_qe_arbiter_weights(&dlb2_dev->hw, weight);
	}

	/* Configure arbitration weights for QID selection */
	if (dlb2_qid_sa_pct <= 100) {
		u8 weight[DLB2_NUM_ARB_WEIGHTS];

		dlb2_pf_calc_arbiter_weights(&dlb2_dev->hw,
					     weight,
					     dlb2_qid_sa_pct);

		dlb2_hw_set_qid_arbiter_weights(&dlb2_dev->hw, weight);
	}
}

/*****************************/
/****** Sysfs callbacks ******/
/*****************************/

static ssize_t
dlb2_sysfs_aux_vf_ids_read(struct device *dev,
			   struct device_attribute *attr,
			   char *buf,
			   int vf_id)
{
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);
	int i, size;

	mutex_lock(&dlb2_dev->resource_mutex);

	size = 0;

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++) {
		if (!dlb2_dev->child_id_state[i].is_auxiliary_vf)
			continue;

		if (dlb2_dev->child_id_state[i].primary_vf_id != vf_id)
			continue;

		size += scnprintf(&buf[size], PAGE_SIZE - size, "%d,", i);
	}

	if (size == 0)
		size = 1;

	/* Replace the last comma with a newline */
	size += scnprintf(&buf[size - 1], PAGE_SIZE - size, "\n");

	mutex_unlock(&dlb2_dev->resource_mutex);

	return size;
}

static ssize_t
dlb2_sysfs_aux_vf_ids_write(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf,
			    size_t count,
			    int primary_vf_id)
{
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);
	char *user_buf = (char *)buf;
	unsigned int vf_id;
	char *vf_id_str;

	mutex_lock(&dlb2_dev->resource_mutex);

	/* If the primary VF is locked, no auxiliary VFs can be added to or
	 * removed from it.
	 */
	if (dlb2_vdev_is_locked(&dlb2_dev->hw, primary_vf_id)) {
		mutex_unlock(&dlb2_dev->resource_mutex);
		return -EINVAL;
	}

	vf_id_str = strsep(&user_buf, ",");
	while (vf_id_str) {
		struct vf_id_state *child_id_state;
		int ret;

		ret = kstrtouint(vf_id_str, 0, &vf_id);
		if (ret) {
			mutex_unlock(&dlb2_dev->resource_mutex);
			return -EINVAL;
		}

		if (vf_id >= DLB2_MAX_NUM_VDEVS) {
			mutex_unlock(&dlb2_dev->resource_mutex);
			return -EINVAL;
		}

		child_id_state = &dlb2_dev->child_id_state[vf_id];

		if (vf_id == primary_vf_id) {
			mutex_unlock(&dlb2_dev->resource_mutex);
			return -EINVAL;
		}

		/* Check if the aux-primary VF relationship already exists */
		if (child_id_state->is_auxiliary_vf &&
		    child_id_state->primary_vf_id == primary_vf_id) {
			vf_id_str = strsep(&user_buf, ",");
			continue;
		}

		/* If the desired VF is locked, it can't be made auxiliary */
		if (dlb2_vdev_is_locked(&dlb2_dev->hw, vf_id)) {
			mutex_unlock(&dlb2_dev->resource_mutex);
			return -EINVAL;
		}

		/* Attempt to reassign the VF */
		child_id_state->is_auxiliary_vf = true;
		child_id_state->primary_vf_id = primary_vf_id;

		/* Reassign any of the desired VF's resources back to the PF */
		dlb2_reset_vdev_resources(&dlb2_dev->hw, vf_id);

		vf_id_str = strsep(&user_buf, ",");
	}

	mutex_unlock(&dlb2_dev->resource_mutex);

	return count;
}

static ssize_t
dlb2_sysfs_vf_read(struct device *dev,
		   struct device_attribute *attr,
		   char *buf,
		   int vf_id)
{
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);
	struct dlb2_get_num_resources_args num_avail_rsrcs;
	struct dlb2_get_num_resources_args num_used_rsrcs;
	struct dlb2_get_num_resources_args num_rsrcs;
	struct dlb2_hw *hw = &dlb2_dev->hw;
	int val;

	mutex_lock(&dlb2_dev->resource_mutex);

	val = dlb2_hw_get_num_resources(hw, &num_avail_rsrcs, true, vf_id);
	if (val) {
		mutex_unlock(&dlb2_dev->resource_mutex);
		return -1;
	}

	val = dlb2_hw_get_num_used_resources(hw, &num_used_rsrcs, true, vf_id);
	if (val) {
		mutex_unlock(&dlb2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&dlb2_dev->resource_mutex);

	num_rsrcs.num_sched_domains = num_avail_rsrcs.num_sched_domains +
		num_used_rsrcs.num_sched_domains;
	num_rsrcs.num_ldb_queues = num_avail_rsrcs.num_ldb_queues +
		num_used_rsrcs.num_ldb_queues;
	num_rsrcs.num_ldb_ports = num_avail_rsrcs.num_ldb_ports +
		num_used_rsrcs.num_ldb_ports;
	num_rsrcs.num_cos0_ldb_ports = num_avail_rsrcs.num_cos0_ldb_ports +
		num_used_rsrcs.num_cos0_ldb_ports;
	num_rsrcs.num_cos1_ldb_ports = num_avail_rsrcs.num_cos1_ldb_ports +
		num_used_rsrcs.num_cos1_ldb_ports;
	num_rsrcs.num_cos2_ldb_ports = num_avail_rsrcs.num_cos2_ldb_ports +
		num_used_rsrcs.num_cos2_ldb_ports;
	num_rsrcs.num_cos3_ldb_ports = num_avail_rsrcs.num_cos3_ldb_ports +
		num_used_rsrcs.num_cos3_ldb_ports;
	num_rsrcs.num_dir_ports = num_avail_rsrcs.num_dir_ports +
		num_used_rsrcs.num_dir_ports;
	num_rsrcs.num_ldb_credits = num_avail_rsrcs.num_ldb_credits +
		num_used_rsrcs.num_ldb_credits;
	num_rsrcs.num_dir_credits = num_avail_rsrcs.num_dir_credits +
		num_used_rsrcs.num_dir_credits;
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
	else if (strncmp(attr->attr.name, "num_cos0_ldb_ports",
			 sizeof("num_cos0_ldb_ports")) == 0)
		val = num_rsrcs.num_cos0_ldb_ports;
	else if (strncmp(attr->attr.name, "num_cos1_ldb_ports",
			 sizeof("num_cos1_ldb_ports")) == 0)
		val = num_rsrcs.num_cos1_ldb_ports;
	else if (strncmp(attr->attr.name, "num_cos2_ldb_ports",
			 sizeof("num_cos2_ldb_ports")) == 0)
		val = num_rsrcs.num_cos2_ldb_ports;
	else if (strncmp(attr->attr.name, "num_cos3_ldb_ports",
			 sizeof("num_cos3_ldb_ports")) == 0)
		val = num_rsrcs.num_cos3_ldb_ports;
	else if (strncmp(attr->attr.name, "num_dir_ports",
			 sizeof("num_dir_ports")) == 0)
		val = num_rsrcs.num_dir_ports;
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
		val = (int)dlb2_vdev_is_locked(hw, vf_id);
	else if (strncmp(attr->attr.name, "func_bar_en",
			 sizeof("func_bar_en")) == 0)
		val = (int)dlb2_dev->mbox[vf_id].enabled;
	else
		return -1;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t
dlb2_sysfs_vf_write(struct device *dev,
		    struct device_attribute *attr,
		    const char *buf,
		    size_t count,
		    int vf_id)
{
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);
	struct dlb2_hw *hw = &dlb2_dev->hw;
	unsigned long num;
	const char *name;
	int ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	name = attr->attr.name;

	mutex_lock(&dlb2_dev->resource_mutex);

	if (strncmp(name, "num_sched_domains",
		    sizeof("num_sched_domains")) == 0) {
		ret = dlb2_update_vdev_sched_domains(hw, vf_id, num);
	} else if (strncmp(name, "num_ldb_queues",
			   sizeof("num_ldb_queues")) == 0) {
		ret = dlb2_update_vdev_ldb_queues(hw, vf_id, num);
	} else if (strncmp(name, "num_ldb_ports",
			   sizeof("num_ldb_ports")) == 0) {
		ret = dlb2_update_vdev_ldb_ports(hw, vf_id, num);
	} else if (strncmp(name, "num_cos0_ldb_ports",
			   sizeof("num_cos0_ldb_ports")) == 0) {
		ret = dlb2_update_vdev_ldb_cos_ports(hw, vf_id, 0, num);
	} else if (strncmp(name, "num_cos1_ldb_ports",
			   sizeof("num_cos1_ldb_ports")) == 0) {
		ret = dlb2_update_vdev_ldb_cos_ports(hw, vf_id, 1, num);
	} else if (strncmp(name, "num_cos2_ldb_ports",
			   sizeof("num_cos2_ldb_ports")) == 0) {
		ret = dlb2_update_vdev_ldb_cos_ports(hw, vf_id, 2, num);
	} else if (strncmp(name, "num_cos3_ldb_ports",
			   sizeof("num_cos3_ldb_ports")) == 0) {
		ret = dlb2_update_vdev_ldb_cos_ports(hw, vf_id, 3, num);
	} else if (strncmp(name, "num_dir_ports",
			   sizeof("num_dir_ports")) == 0) {
		ret = dlb2_update_vdev_dir_ports(hw, vf_id, num);
	} else if (strncmp(name, "num_ldb_credits",
			   sizeof("num_ldb_credits")) == 0) {
		ret = dlb2_update_vdev_ldb_credits(hw, vf_id, num);
	} else if (strncmp(name, "num_dir_credits",
			   sizeof("num_dir_credits")) == 0) {
		ret = dlb2_update_vdev_dir_credits(hw, vf_id, num);
	} else if (strncmp(attr->attr.name, "num_hist_list_entries",
			   sizeof("num_hist_list_entries")) == 0) {
		ret = dlb2_update_vdev_hist_list_entries(hw, vf_id, num);
	} else if (strncmp(attr->attr.name, "num_atomic_inflights",
			   sizeof("num_atomic_inflights")) == 0) {
		ret = dlb2_update_vdev_atomic_inflights(hw, vf_id, num);
	} else if (strncmp(attr->attr.name, "func_bar_en",
			   sizeof("func_bar_en")) == 0) {
		if (!dlb2_dev->mbox[vf_id].enabled && num) {
			union dlb2_iosf_func_vf_bar_dsbl r0 = { {0} };

			r0.field.func_vf_bar_dis = 0;

			DLB2_CSR_WR(&dlb2_dev->hw,
				    DLB2_IOSF_FUNC_VF_BAR_DSBL(vf_id),
				    r0.val);

			dev_err(dlb2_dev->dlb2_device,
				"[%s()] Re-enabling VDEV %d's FUNC BAR",
				__func__, vf_id);

			dlb2_dev->mbox[vf_id].enabled = true;
		}
	} else {
		mutex_unlock(&dlb2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&dlb2_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

#define DLB2_VF_SYSFS_RD_FUNC(id) \
static ssize_t dlb2_sysfs_vf ## id ## _read(		      \
	struct device *dev,				      \
	struct device_attribute *attr,			      \
	char *buf)					      \
{							      \
	return dlb2_sysfs_vf_read(dev, attr, buf, id);	      \
}							      \

#define DLB2_VF_SYSFS_WR_FUNC(id) \
static ssize_t dlb2_sysfs_vf ## id ## _write(		       \
	struct device *dev,				       \
	struct device_attribute *attr,			       \
	const char *buf,				       \
	size_t count)					       \
{							       \
	return dlb2_sysfs_vf_write(dev, attr, buf, count, id); \
}

#define DLB2_AUX_VF_ID_RD_FUNC(id) \
static ssize_t dlb2_sysfs_vf ## id ## _vf_ids_read(		      \
	struct device *dev,					      \
	struct device_attribute *attr,				      \
	char *buf)						      \
{								      \
	return dlb2_sysfs_aux_vf_ids_read(dev, attr, buf, id);	      \
}								      \

#define DLB2_AUX_VF_ID_WR_FUNC(id) \
static ssize_t dlb2_sysfs_vf ## id ## _vf_ids_write(		       \
	struct device *dev,					       \
	struct device_attribute *attr,				       \
	const char *buf,					       \
	size_t count)						       \
{								       \
	return dlb2_sysfs_aux_vf_ids_write(dev, attr, buf, count, id); \
}

/* Read-write per-resource-group sysfs files */
#define DLB2_VF_DEVICE_ATTRS(id) \
static struct device_attribute			    \
dev_attr_vf ## id ## _sched_domains =		    \
	__ATTR(num_sched_domains,		    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _ldb_queues =		    \
	__ATTR(num_ldb_queues,			    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _ldb_ports =		    \
	__ATTR(num_ldb_ports,			    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _cos0_ldb_ports =		    \
	__ATTR(num_cos0_ldb_ports,		    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _cos1_ldb_ports =		    \
	__ATTR(num_cos1_ldb_ports,		    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _cos2_ldb_ports =		    \
	__ATTR(num_cos2_ldb_ports,		    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _cos3_ldb_ports =		    \
	__ATTR(num_cos3_ldb_ports,		    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _dir_ports =		    \
	__ATTR(num_dir_ports,			    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _ldb_credits =		    \
	__ATTR(num_ldb_credits,			    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _dir_credits =		    \
	__ATTR(num_dir_credits,			    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _hist_list_entries =	    \
	__ATTR(num_hist_list_entries,		    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _atomic_inflights =	    \
	__ATTR(num_atomic_inflights,		    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _locked =			    \
	__ATTR(locked,				    \
	       0444,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       NULL);				    \
static struct device_attribute			    \
dev_attr_vf ## id ## _func_bar_en =		    \
	__ATTR(func_bar_en,			    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _read,	    \
	       dlb2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _aux_vf_ids =		    \
	__ATTR(aux_vf_ids,			    \
	       0644,				    \
	       dlb2_sysfs_vf ## id ## _vf_ids_read,  \
	       dlb2_sysfs_vf ## id ## _vf_ids_write) \

#define DLB2_VF_SYSFS_ATTRS(id) \
DLB2_VF_DEVICE_ATTRS(id);				\
static struct attribute *dlb2_vf ## id ## _attrs[] = {	\
	&dev_attr_vf ## id ## _sched_domains.attr,	\
	&dev_attr_vf ## id ## _ldb_queues.attr,		\
	&dev_attr_vf ## id ## _ldb_ports.attr,		\
	&dev_attr_vf ## id ## _cos0_ldb_ports.attr,	\
	&dev_attr_vf ## id ## _cos1_ldb_ports.attr,	\
	&dev_attr_vf ## id ## _cos2_ldb_ports.attr,	\
	&dev_attr_vf ## id ## _cos3_ldb_ports.attr,	\
	&dev_attr_vf ## id ## _dir_ports.attr,		\
	&dev_attr_vf ## id ## _ldb_credits.attr,	\
	&dev_attr_vf ## id ## _dir_credits.attr,	\
	&dev_attr_vf ## id ## _hist_list_entries.attr,	\
	&dev_attr_vf ## id ## _atomic_inflights.attr,	\
	&dev_attr_vf ## id ## _locked.attr,		\
	&dev_attr_vf ## id ## _func_bar_en.attr,	\
	&dev_attr_vf ## id ## _aux_vf_ids.attr,		\
	NULL						\
}

#define DLB2_VF_SYSFS_ATTR_GROUP(id) \
DLB2_VF_SYSFS_ATTRS(id);					\
static struct attribute_group dlb2_vf ## id ## _attr_group = {	\
	.attrs = dlb2_vf ## id ## _attrs,			\
	.name = "vf" #id "_resources"				\
}

DLB2_VF_SYSFS_RD_FUNC(0);
DLB2_VF_SYSFS_RD_FUNC(1);
DLB2_VF_SYSFS_RD_FUNC(2);
DLB2_VF_SYSFS_RD_FUNC(3);
DLB2_VF_SYSFS_RD_FUNC(4);
DLB2_VF_SYSFS_RD_FUNC(5);
DLB2_VF_SYSFS_RD_FUNC(6);
DLB2_VF_SYSFS_RD_FUNC(7);
DLB2_VF_SYSFS_RD_FUNC(8);
DLB2_VF_SYSFS_RD_FUNC(9);
DLB2_VF_SYSFS_RD_FUNC(10);
DLB2_VF_SYSFS_RD_FUNC(11);
DLB2_VF_SYSFS_RD_FUNC(12);
DLB2_VF_SYSFS_RD_FUNC(13);
DLB2_VF_SYSFS_RD_FUNC(14);
DLB2_VF_SYSFS_RD_FUNC(15);

DLB2_VF_SYSFS_WR_FUNC(0);
DLB2_VF_SYSFS_WR_FUNC(1);
DLB2_VF_SYSFS_WR_FUNC(2);
DLB2_VF_SYSFS_WR_FUNC(3);
DLB2_VF_SYSFS_WR_FUNC(4);
DLB2_VF_SYSFS_WR_FUNC(5);
DLB2_VF_SYSFS_WR_FUNC(6);
DLB2_VF_SYSFS_WR_FUNC(7);
DLB2_VF_SYSFS_WR_FUNC(8);
DLB2_VF_SYSFS_WR_FUNC(9);
DLB2_VF_SYSFS_WR_FUNC(10);
DLB2_VF_SYSFS_WR_FUNC(11);
DLB2_VF_SYSFS_WR_FUNC(12);
DLB2_VF_SYSFS_WR_FUNC(13);
DLB2_VF_SYSFS_WR_FUNC(14);
DLB2_VF_SYSFS_WR_FUNC(15);

DLB2_AUX_VF_ID_RD_FUNC(0);
DLB2_AUX_VF_ID_RD_FUNC(1);
DLB2_AUX_VF_ID_RD_FUNC(2);
DLB2_AUX_VF_ID_RD_FUNC(3);
DLB2_AUX_VF_ID_RD_FUNC(4);
DLB2_AUX_VF_ID_RD_FUNC(5);
DLB2_AUX_VF_ID_RD_FUNC(6);
DLB2_AUX_VF_ID_RD_FUNC(7);
DLB2_AUX_VF_ID_RD_FUNC(8);
DLB2_AUX_VF_ID_RD_FUNC(9);
DLB2_AUX_VF_ID_RD_FUNC(10);
DLB2_AUX_VF_ID_RD_FUNC(11);
DLB2_AUX_VF_ID_RD_FUNC(12);
DLB2_AUX_VF_ID_RD_FUNC(13);
DLB2_AUX_VF_ID_RD_FUNC(14);
DLB2_AUX_VF_ID_RD_FUNC(15);

DLB2_AUX_VF_ID_WR_FUNC(0);
DLB2_AUX_VF_ID_WR_FUNC(1);
DLB2_AUX_VF_ID_WR_FUNC(2);
DLB2_AUX_VF_ID_WR_FUNC(3);
DLB2_AUX_VF_ID_WR_FUNC(4);
DLB2_AUX_VF_ID_WR_FUNC(5);
DLB2_AUX_VF_ID_WR_FUNC(6);
DLB2_AUX_VF_ID_WR_FUNC(7);
DLB2_AUX_VF_ID_WR_FUNC(8);
DLB2_AUX_VF_ID_WR_FUNC(9);
DLB2_AUX_VF_ID_WR_FUNC(10);
DLB2_AUX_VF_ID_WR_FUNC(11);
DLB2_AUX_VF_ID_WR_FUNC(12);
DLB2_AUX_VF_ID_WR_FUNC(13);
DLB2_AUX_VF_ID_WR_FUNC(14);
DLB2_AUX_VF_ID_WR_FUNC(15);

DLB2_VF_SYSFS_ATTR_GROUP(0);
DLB2_VF_SYSFS_ATTR_GROUP(1);
DLB2_VF_SYSFS_ATTR_GROUP(2);
DLB2_VF_SYSFS_ATTR_GROUP(3);
DLB2_VF_SYSFS_ATTR_GROUP(4);
DLB2_VF_SYSFS_ATTR_GROUP(5);
DLB2_VF_SYSFS_ATTR_GROUP(6);
DLB2_VF_SYSFS_ATTR_GROUP(7);
DLB2_VF_SYSFS_ATTR_GROUP(8);
DLB2_VF_SYSFS_ATTR_GROUP(9);
DLB2_VF_SYSFS_ATTR_GROUP(10);
DLB2_VF_SYSFS_ATTR_GROUP(11);
DLB2_VF_SYSFS_ATTR_GROUP(12);
DLB2_VF_SYSFS_ATTR_GROUP(13);
DLB2_VF_SYSFS_ATTR_GROUP(14);
DLB2_VF_SYSFS_ATTR_GROUP(15);

const struct attribute_group *dlb2_vf_attrs[] = {
	&dlb2_vf0_attr_group,
	&dlb2_vf1_attr_group,
	&dlb2_vf2_attr_group,
	&dlb2_vf3_attr_group,
	&dlb2_vf4_attr_group,
	&dlb2_vf5_attr_group,
	&dlb2_vf6_attr_group,
	&dlb2_vf7_attr_group,
	&dlb2_vf8_attr_group,
	&dlb2_vf9_attr_group,
	&dlb2_vf10_attr_group,
	&dlb2_vf11_attr_group,
	&dlb2_vf12_attr_group,
	&dlb2_vf13_attr_group,
	&dlb2_vf14_attr_group,
	&dlb2_vf15_attr_group,
};

#define DLB2_TOTAL_SYSFS_SHOW(name, macro)		\
static ssize_t total_##name##_show(			\
	struct device *dev,				\
	struct device_attribute *attr,			\
	char *buf)					\
{							\
	int val = DLB2_MAX_NUM_##macro;			\
							\
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);	\
}

DLB2_TOTAL_SYSFS_SHOW(num_sched_domains, DOMAINS)
DLB2_TOTAL_SYSFS_SHOW(num_ldb_queues, LDB_QUEUES)
DLB2_TOTAL_SYSFS_SHOW(num_ldb_ports, LDB_PORTS)
DLB2_TOTAL_SYSFS_SHOW(num_cos0_ldb_ports, LDB_PORTS / DLB2_NUM_COS_DOMAINS)
DLB2_TOTAL_SYSFS_SHOW(num_cos1_ldb_ports, LDB_PORTS / DLB2_NUM_COS_DOMAINS)
DLB2_TOTAL_SYSFS_SHOW(num_cos2_ldb_ports, LDB_PORTS / DLB2_NUM_COS_DOMAINS)
DLB2_TOTAL_SYSFS_SHOW(num_cos3_ldb_ports, LDB_PORTS / DLB2_NUM_COS_DOMAINS)
DLB2_TOTAL_SYSFS_SHOW(num_dir_ports, DIR_PORTS)
DLB2_TOTAL_SYSFS_SHOW(num_ldb_credits, LDB_CREDITS)
DLB2_TOTAL_SYSFS_SHOW(num_dir_credits, DIR_CREDITS)
DLB2_TOTAL_SYSFS_SHOW(num_atomic_inflights, AQED_ENTRIES)
DLB2_TOTAL_SYSFS_SHOW(num_hist_list_entries, HIST_LIST_ENTRIES)

#define DLB2_AVAIL_SYSFS_SHOW(name)			     \
static ssize_t avail_##name##_show(			     \
	struct device *dev,				     \
	struct device_attribute *attr,			     \
	char *buf)					     \
{							     \
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);    \
	struct dlb2_get_num_resources_args arg;		     \
	struct dlb2_hw *hw = &dlb2_dev->hw;		     \
	int val;					     \
							     \
	mutex_lock(&dlb2_dev->resource_mutex);		     \
							     \
	val = dlb2_hw_get_num_resources(hw, &arg, false, 0); \
							     \
	mutex_unlock(&dlb2_dev->resource_mutex);	     \
							     \
	if (val)					     \
		return -1;				     \
							     \
	val = arg.name;					     \
							     \
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);	     \
}

DLB2_AVAIL_SYSFS_SHOW(num_sched_domains)
DLB2_AVAIL_SYSFS_SHOW(num_ldb_queues)
DLB2_AVAIL_SYSFS_SHOW(num_ldb_ports)
DLB2_AVAIL_SYSFS_SHOW(num_cos0_ldb_ports)
DLB2_AVAIL_SYSFS_SHOW(num_cos1_ldb_ports)
DLB2_AVAIL_SYSFS_SHOW(num_cos2_ldb_ports)
DLB2_AVAIL_SYSFS_SHOW(num_cos3_ldb_ports)
DLB2_AVAIL_SYSFS_SHOW(num_dir_ports)
DLB2_AVAIL_SYSFS_SHOW(num_ldb_credits)
DLB2_AVAIL_SYSFS_SHOW(num_dir_credits)
DLB2_AVAIL_SYSFS_SHOW(num_atomic_inflights)
DLB2_AVAIL_SYSFS_SHOW(num_hist_list_entries)

static ssize_t max_ctg_hl_entries_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);
	struct dlb2_get_num_resources_args arg;
	struct dlb2_hw *hw = &dlb2_dev->hw;
	int val;

	mutex_lock(&dlb2_dev->resource_mutex);

	val = dlb2_hw_get_num_resources(hw, &arg, false, 0);

	mutex_unlock(&dlb2_dev->resource_mutex);

	if (val)
		return -1;

	val = arg.max_contiguous_hist_list_entries;

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

static const struct attribute_group dlb2_total_attr_group = {
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

static const struct attribute_group dlb2_avail_attr_group = {
	.attrs = dlb2_avail_attrs,
	.name = "avail_resources",
};

#define DLB2_GROUP_SNS_PER_QUEUE_SHOW(id)	       \
static ssize_t group##id##_sns_per_queue_show(	       \
	struct device *dev,			       \
	struct device_attribute *attr,		       \
	char *buf)				       \
{						       \
	struct dlb2_dev *dlb2_dev =		       \
		dev_get_drvdata(dev);		       \
	struct dlb2_hw *hw = &dlb2_dev->hw;	       \
	int val;				       \
						       \
	mutex_lock(&dlb2_dev->resource_mutex);	       \
						       \
	val = dlb2_get_group_sequence_numbers(hw, id); \
						       \
	mutex_unlock(&dlb2_dev->resource_mutex);       \
						       \
	return scnprintf(buf, PAGE_SIZE, "%d\n", val); \
}

DLB2_GROUP_SNS_PER_QUEUE_SHOW(0)
DLB2_GROUP_SNS_PER_QUEUE_SHOW(1)

#define DLB2_GROUP_SNS_PER_QUEUE_STORE(id)		    \
static ssize_t group##id##_sns_per_queue_store(		    \
	struct device *dev,				    \
	struct device_attribute *attr,			    \
	const char *buf,				    \
	size_t count)					    \
{							    \
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);   \
	struct dlb2_hw *hw = &dlb2_dev->hw;		    \
	unsigned long val;				    \
	int err;					    \
							    \
	err = kstrtoul(buf, 0, &val);			    \
	if (err)					    \
		return -1;				    \
							    \
	mutex_lock(&dlb2_dev->resource_mutex);		    \
							    \
	err = dlb2_set_group_sequence_numbers(hw, id, val); \
							    \
	mutex_unlock(&dlb2_dev->resource_mutex);	    \
							    \
	return err ? err : count;			    \
}

DLB2_GROUP_SNS_PER_QUEUE_STORE(0)
DLB2_GROUP_SNS_PER_QUEUE_STORE(1)

/* RW sysfs files in the sequence_numbers/ subdirectory */
static DEVICE_ATTR_RW(group0_sns_per_queue);
static DEVICE_ATTR_RW(group1_sns_per_queue);

static struct attribute *dlb2_sequence_number_attrs[] = {
	&dev_attr_group0_sns_per_queue.attr,
	&dev_attr_group1_sns_per_queue.attr,
	NULL
};

static const struct attribute_group dlb2_sequence_number_attr_group = {
	.attrs = dlb2_sequence_number_attrs,
	.name = "sequence_numbers"
};

#define DLB2_COS_BW_PERCENT_SHOW(id)		       \
static ssize_t cos##id##_bw_percent_show(	       \
	struct device *dev,			       \
	struct device_attribute *attr,		       \
	char *buf)				       \
{						       \
	struct dlb2_dev *dlb2_dev =		       \
		dev_get_drvdata(dev);		       \
	struct dlb2_hw *hw = &dlb2_dev->hw;	       \
	int val;				       \
						       \
	mutex_lock(&dlb2_dev->resource_mutex);	       \
						       \
	val = dlb2_hw_get_cos_bandwidth(hw, id);       \
						       \
	mutex_unlock(&dlb2_dev->resource_mutex);       \
						       \
	return scnprintf(buf, PAGE_SIZE, "%d\n", val); \
}

DLB2_COS_BW_PERCENT_SHOW(0)
DLB2_COS_BW_PERCENT_SHOW(1)
DLB2_COS_BW_PERCENT_SHOW(2)
DLB2_COS_BW_PERCENT_SHOW(3)

#define DLB2_COS_BW_PERCENT_STORE(id)		      \
static ssize_t cos##id##_bw_percent_store(	      \
	struct device *dev,			      \
	struct device_attribute *attr,		      \
	const char *buf,			      \
	size_t count)				      \
{						      \
	struct dlb2_dev *dlb2_dev =		      \
		dev_get_drvdata(dev);		      \
	struct dlb2_hw *hw = &dlb2_dev->hw;	      \
	unsigned long val;			      \
	int err;				      \
						      \
	err = kstrtoul(buf, 0, &val);		      \
	if (err)				      \
		return -1;			      \
						      \
	mutex_lock(&dlb2_dev->resource_mutex);	      \
						      \
	err = dlb2_hw_set_cos_bandwidth(hw, id, val); \
						      \
	mutex_unlock(&dlb2_dev->resource_mutex);      \
						      \
	return err ? err : count;		      \
}

DLB2_COS_BW_PERCENT_STORE(0)
DLB2_COS_BW_PERCENT_STORE(1)
DLB2_COS_BW_PERCENT_STORE(2)
DLB2_COS_BW_PERCENT_STORE(3)

/* RW sysfs files in the sequence_numbers/ subdirectory */
static DEVICE_ATTR_RW(cos0_bw_percent);
static DEVICE_ATTR_RW(cos1_bw_percent);
static DEVICE_ATTR_RW(cos2_bw_percent);
static DEVICE_ATTR_RW(cos3_bw_percent);

static struct attribute *dlb2_cos_bw_percent_attrs[] = {
	&dev_attr_cos0_bw_percent.attr,
	&dev_attr_cos1_bw_percent.attr,
	&dev_attr_cos2_bw_percent.attr,
	&dev_attr_cos3_bw_percent.attr,
	NULL
};

static const struct attribute_group dlb2_cos_bw_percent_attr_group = {
	.attrs = dlb2_cos_bw_percent_attrs,
	.name = "cos_bw"
};

static ssize_t dev_id_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n", dlb2_dev->id);
}

static DEVICE_ATTR_RO(dev_id);

static ssize_t ingress_err_en_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);
	ssize_t ret;

	mutex_lock(&dlb2_dev->resource_mutex);

	ret = scnprintf(buf, PAGE_SIZE, "%d\n", dlb2_dev->ingress_err.enabled);

	mutex_unlock(&dlb2_dev->resource_mutex);

	return ret;
}

static ssize_t ingress_err_en_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf,
				    size_t count)
{
	struct dlb2_dev *dlb2_dev = dev_get_drvdata(dev);
	unsigned long num;
	ssize_t ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	mutex_lock(&dlb2_dev->resource_mutex);

	if (!dlb2_dev->ingress_err.enabled && num) {
		dlb2_enable_ingress_error_alarms(&dlb2_dev->hw);

		dev_err(dlb2_dev->dlb2_device,
			"[%s()] Re-enabling ingress error interrupts",
			__func__);

		dlb2_dev->ingress_err.enabled = true;
	}

	mutex_unlock(&dlb2_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

static DEVICE_ATTR_RW(ingress_err_en);

static int
dlb2_pf_sysfs_create(struct dlb2_dev *dlb2_dev)
{
	struct kobject *kobj;
	int ret;
	int i;

	kobj = &dlb2_dev->pdev->dev.kobj;

	ret = sysfs_create_file(kobj, &dev_attr_ingress_err_en.attr);
	if (ret)
		goto dlb2_ingress_err_en_attr_group_fail;

	ret = sysfs_create_file(kobj, &dev_attr_dev_id.attr);
	if (ret)
		goto dlb2_dev_id_attr_group_fail;

	ret = sysfs_create_group(kobj, &dlb2_total_attr_group);
	if (ret)
		goto dlb2_total_attr_group_fail;

	ret = sysfs_create_group(kobj, &dlb2_avail_attr_group);
	if (ret)
		goto dlb2_avail_attr_group_fail;

	ret = sysfs_create_group(kobj, &dlb2_sequence_number_attr_group);
	if (ret)
		goto dlb2_sn_attr_group_fail;

	ret = sysfs_create_group(kobj, &dlb2_cos_bw_percent_attr_group);
	if (ret)
		goto dlb2_cos_bw_percent_attr_group_fail;

	for (i = 0; i < pci_num_vf(dlb2_dev->pdev); i++) {
		ret = sysfs_create_group(kobj, dlb2_vf_attrs[i]);
		if (ret)
			goto dlb2_vf_attr_group_fail;
	}

	return 0;

dlb2_vf_attr_group_fail:
	for (i--; i >= 0; i--)
		sysfs_remove_group(kobj, dlb2_vf_attrs[i]);

	sysfs_remove_group(kobj, &dlb2_cos_bw_percent_attr_group);
dlb2_cos_bw_percent_attr_group_fail:
	sysfs_remove_group(kobj, &dlb2_sequence_number_attr_group);
dlb2_sn_attr_group_fail:
	sysfs_remove_group(kobj, &dlb2_avail_attr_group);
dlb2_avail_attr_group_fail:
	sysfs_remove_group(kobj, &dlb2_total_attr_group);
dlb2_total_attr_group_fail:
	sysfs_remove_file(kobj, &dev_attr_dev_id.attr);
dlb2_dev_id_attr_group_fail:
	sysfs_remove_file(kobj, &dev_attr_ingress_err_en.attr);
dlb2_ingress_err_en_attr_group_fail:
	return ret;
}

static void
dlb2_pf_sysfs_destroy(struct dlb2_dev *dlb2_dev)
{
	struct kobject *kobj;
	int i;

	kobj = &dlb2_dev->pdev->dev.kobj;

	for (i = 0; i < pci_num_vf(dlb2_dev->pdev); i++)
		sysfs_remove_group(kobj, dlb2_vf_attrs[i]);

	sysfs_remove_group(kobj, &dlb2_cos_bw_percent_attr_group);
	sysfs_remove_group(kobj, &dlb2_sequence_number_attr_group);
	sysfs_remove_group(kobj, &dlb2_avail_attr_group);
	sysfs_remove_group(kobj, &dlb2_total_attr_group);
	sysfs_remove_file(kobj, &dev_attr_dev_id.attr);
	sysfs_remove_file(kobj, &dev_attr_ingress_err_en.attr);
}

static void
dlb2_pf_sysfs_reapply_configuration(struct dlb2_dev *dev)
{
	int i;

	for (i = 0; i < DLB2_MAX_NUM_SEQUENCE_NUMBER_GROUPS; i++) {
		int num_sns = dlb2_get_group_sequence_numbers(&dev->hw, i);

		dlb2_set_group_sequence_numbers(&dev->hw, i, num_sns);
	}

	for (i = 0; i < DLB2_NUM_COS_DOMAINS; i++) {
		int bw = dlb2_hw_get_cos_bandwidth(&dev->hw, i);

		dlb2_hw_set_cos_bandwidth(&dev->hw, i, bw);
	}
}

/*****************************/
/****** IOCTL callbacks ******/
/*****************************/

static int
dlb2_pf_create_sched_domain(struct dlb2_hw *hw,
			    struct dlb2_create_sched_domain_args *args,
			    struct dlb2_cmd_response *resp)
{
	return dlb2_hw_create_sched_domain(hw, args, resp, false, 0);
}

static int
dlb2_pf_create_ldb_queue(struct dlb2_hw *hw,
			 u32 id,
			 struct dlb2_create_ldb_queue_args *args,
			 struct dlb2_cmd_response *resp)
{
	return dlb2_hw_create_ldb_queue(hw, id, args, resp, false, 0);
}

static int
dlb2_pf_create_dir_queue(struct dlb2_hw *hw,
			 u32 id,
			 struct dlb2_create_dir_queue_args *args,
			 struct dlb2_cmd_response *resp)
{
	return dlb2_hw_create_dir_queue(hw, id, args, resp, false, 0);
}

static int
dlb2_pf_create_ldb_port(struct dlb2_hw *hw,
			u32 id,
			struct dlb2_create_ldb_port_args *args,
			uintptr_t cq_dma_base,
			struct dlb2_cmd_response *resp)
{
	return dlb2_hw_create_ldb_port(hw, id, args, cq_dma_base,
				       resp, false, 0);
}

static int
dlb2_pf_create_dir_port(struct dlb2_hw *hw,
			u32 id,
			struct dlb2_create_dir_port_args *args,
			uintptr_t cq_dma_base,
			struct dlb2_cmd_response *resp)
{
	return dlb2_hw_create_dir_port(hw, id, args, cq_dma_base,
				       resp, false, 0);
}

static int
dlb2_pf_start_domain(struct dlb2_hw *hw,
		     u32 id,
		     struct dlb2_start_domain_args *args,
		     struct dlb2_cmd_response *resp)
{
	return dlb2_hw_start_domain(hw, id, args, resp, false, 0);
}

static int
dlb2_pf_map_qid(struct dlb2_hw *hw,
		u32 id,
		struct dlb2_map_qid_args *args,
		struct dlb2_cmd_response *resp)
{
	return dlb2_hw_map_qid(hw, id, args, resp, false, 0);
}

static int
dlb2_pf_unmap_qid(struct dlb2_hw *hw,
		  u32 id,
		  struct dlb2_unmap_qid_args *args,
		  struct dlb2_cmd_response *resp)
{
	return dlb2_hw_unmap_qid(hw, id, args, resp, false, 0);
}

static int
dlb2_pf_enable_ldb_port(struct dlb2_hw *hw,
			u32 id,
			struct dlb2_enable_ldb_port_args *args,
			struct dlb2_cmd_response *resp)
{
	return dlb2_hw_enable_ldb_port(hw, id, args, resp, false, 0);
}

static int
dlb2_pf_disable_ldb_port(struct dlb2_hw *hw,
			 u32 id,
			 struct dlb2_disable_ldb_port_args *args,
			 struct dlb2_cmd_response *resp)
{
	return dlb2_hw_disable_ldb_port(hw, id, args, resp, false, 0);
}

static int
dlb2_pf_enable_dir_port(struct dlb2_hw *hw,
			u32 id,
			struct dlb2_enable_dir_port_args *args,
			struct dlb2_cmd_response *resp)
{
	return dlb2_hw_enable_dir_port(hw, id, args, resp, false, 0);
}

static int
dlb2_pf_disable_dir_port(struct dlb2_hw *hw,
			 u32 id,
			 struct dlb2_disable_dir_port_args *args,
			 struct dlb2_cmd_response *resp)
{
	return dlb2_hw_disable_dir_port(hw, id, args, resp, false, 0);
}

static int
dlb2_pf_get_num_resources(struct dlb2_hw *hw,
			  struct dlb2_get_num_resources_args *args)
{
	return dlb2_hw_get_num_resources(hw, args, false, 0);
}

static int
dlb2_pf_reset_domain(struct dlb2_hw *hw, u32 id)
{
	return dlb2_reset_domain(hw, id, false, 0);
}

static int
dlb2_pf_write_smon(struct dlb2_dev *dev,
		   struct dlb2_write_smon_args *args,
		   struct dlb2_cmd_response *resp)
{
	int ret;

	mutex_lock(&dev->resource_mutex);

	ret = dlb2_write_smon(&dev->hw, args, resp);

	mutex_unlock(&dev->resource_mutex);

	return ret;
}

static int
dlb2_pf_read_smon(struct dlb2_dev *dev,
		  struct dlb2_read_smon_args *args,
		  struct dlb2_cmd_response *resp)
{
	int ret;

	mutex_lock(&dev->resource_mutex);

	ret = dlb2_read_smon(&dev->hw, args, resp);

	mutex_unlock(&dev->resource_mutex);

	return ret;
}

static int
dlb2_pf_get_ldb_queue_depth(struct dlb2_hw *hw,
			    u32 id,
			    struct dlb2_get_ldb_queue_depth_args *args,
			    struct dlb2_cmd_response *resp)
{
	return dlb2_hw_get_ldb_queue_depth(hw, id, args, resp, false, 0);
}

static int
dlb2_pf_get_dir_queue_depth(struct dlb2_hw *hw,
			    u32 id,
			    struct dlb2_get_dir_queue_depth_args *args,
			    struct dlb2_cmd_response *resp)
{
	return dlb2_hw_get_dir_queue_depth(hw, id, args, resp, false, 0);
}

static int
dlb2_pf_pending_port_unmaps(struct dlb2_hw *hw,
			    u32 id,
			    struct dlb2_pending_port_unmaps_args *args,
			    struct dlb2_cmd_response *resp)
{
	return dlb2_hw_pending_port_unmaps(hw, id, args, resp, false, 0);
}

/**************************************/
/****** Resource query callbacks ******/
/**************************************/

static int
dlb2_pf_ldb_port_owned_by_domain(struct dlb2_hw *hw,
				 u32 domain_id,
				 u32 port_id)
{
	return dlb2_ldb_port_owned_by_domain(hw, domain_id, port_id, false, 0);
}

static int
dlb2_pf_dir_port_owned_by_domain(struct dlb2_hw *hw,
				 u32 domain_id,
				 u32 port_id)
{
	return dlb2_dir_port_owned_by_domain(hw, domain_id, port_id, false, 0);
}

static int
dlb2_pf_get_sn_allocation(struct dlb2_hw *hw, u32 group_id)
{
	return dlb2_get_group_sequence_numbers(hw, group_id);
}

static int
dlb2_pf_get_cos_bw(struct dlb2_hw *hw, u32 cos_id)
{
	return dlb2_hw_get_cos_bandwidth(hw, cos_id);
}

static int
dlb2_pf_get_sn_occupancy(struct dlb2_hw *hw, u32 group_id)
{
	return dlb2_get_group_sequence_number_occupancy(hw, group_id);
}

/********************************/
/****** DLB2 PF Device Ops ******/
/********************************/

struct dlb2_device_ops dlb2_pf_ops = {
	.map_pci_bar_space = dlb2_pf_map_pci_bar_space,
	.unmap_pci_bar_space = dlb2_pf_unmap_pci_bar_space,
	.mmap = dlb2_pf_mmap,
	.inc_pm_refcnt = dlb2_pf_pm_inc_refcnt,
	.dec_pm_refcnt = dlb2_pf_pm_dec_refcnt,
	.pm_refcnt_status_suspended = dlb2_pf_pm_status_suspended,
	.init_driver_state = dlb2_pf_init_driver_state,
	.free_driver_state = dlb2_pf_free_driver_state,
	.device_create = dlb2_pf_device_create,
	.device_destroy = dlb2_pf_device_destroy,
	.cdev_add = dlb2_pf_cdev_add,
	.cdev_del = dlb2_pf_cdev_del,
	.sysfs_create = dlb2_pf_sysfs_create,
	.sysfs_destroy = dlb2_pf_sysfs_destroy,
	.sysfs_reapply = dlb2_pf_sysfs_reapply_configuration,
	.init_interrupts = dlb2_pf_init_interrupts,
	.enable_ldb_cq_interrupts = dlb2_pf_enable_ldb_cq_interrupts,
	.enable_dir_cq_interrupts = dlb2_pf_enable_dir_cq_interrupts,
	.arm_cq_interrupt = dlb2_pf_arm_cq_interrupt,
	.reinit_interrupts = dlb2_pf_reinit_interrupts,
	.free_interrupts = dlb2_pf_free_interrupts,
	.enable_pm = dlb2_pf_enable_pm,
	.wait_for_device_ready = dlb2_pf_wait_for_device_ready,
	.register_driver = dlb2_pf_register_driver,
	.unregister_driver = dlb2_pf_unregister_driver,
	.create_sched_domain = dlb2_pf_create_sched_domain,
	.create_ldb_queue = dlb2_pf_create_ldb_queue,
	.create_dir_queue = dlb2_pf_create_dir_queue,
	.create_ldb_port = dlb2_pf_create_ldb_port,
	.create_dir_port = dlb2_pf_create_dir_port,
	.start_domain = dlb2_pf_start_domain,
	.map_qid = dlb2_pf_map_qid,
	.unmap_qid = dlb2_pf_unmap_qid,
	.enable_ldb_port = dlb2_pf_enable_ldb_port,
	.enable_dir_port = dlb2_pf_enable_dir_port,
	.disable_ldb_port = dlb2_pf_disable_ldb_port,
	.disable_dir_port = dlb2_pf_disable_dir_port,
	.get_num_resources = dlb2_pf_get_num_resources,
	.reset_domain = dlb2_pf_reset_domain,
	.write_smon = dlb2_pf_write_smon,
	.read_smon = dlb2_pf_read_smon,
	.ldb_port_owned_by_domain = dlb2_pf_ldb_port_owned_by_domain,
	.dir_port_owned_by_domain = dlb2_pf_dir_port_owned_by_domain,
	.get_sn_allocation = dlb2_pf_get_sn_allocation,
	.get_sn_occupancy = dlb2_pf_get_sn_occupancy,
	.get_ldb_queue_depth = dlb2_pf_get_ldb_queue_depth,
	.get_dir_queue_depth = dlb2_pf_get_dir_queue_depth,
	.pending_port_unmaps = dlb2_pf_pending_port_unmaps,
	.get_cos_bw = dlb2_pf_get_cos_bw,
	.init_hardware = dlb2_pf_init_hardware,
	.query_cq_poll_mode = dlb2_pf_query_cq_poll_mode,
};
