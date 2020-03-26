// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2018 Intel Corporation */

#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#include "base/hqmv2_resource.h"
#include "hqmv2_sriov.h"
#include "hqmv2_ioctl.h"
#include "base/hqmv2_mbox.h"
#include "hqmv2_intr.h"
#include "hqmv2_dp_ops.h"
#include "hqmv2_vdcm.h"
#include "base/hqmv2_osdep.h"

/***********************************/
/****** Runtime PM management ******/
/***********************************/

void hqmv2_pf_pm_inc_refcnt(struct pci_dev *pdev, bool resume)
{
	if (resume)
		/* Increment the device's usage count and immediately wake it
		 * if it was suspended.
		 */
		pm_runtime_get_sync(&pdev->dev);
	else
		pm_runtime_get_noresume(&pdev->dev);
}

void hqmv2_pf_pm_dec_refcnt(struct pci_dev *pdev)
{
	/* Decrement the device's usage count and suspend it if the
	 * count reaches zero.
	 */
	pm_runtime_put_sync_suspend(&pdev->dev);
}

bool hqmv2_pf_pm_status_suspended(struct pci_dev *pdev)
{
	return pm_runtime_status_suspended(&pdev->dev);
}

/**************************/
/****** Device reset ******/
/**************************/

int hqmv2_pf_reset(struct pci_dev *pdev)
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

void hqmv2_pf_unmap_pci_bar_space(struct hqmv2_dev *hqmv2_dev,
				  struct pci_dev *pdev)
{
	pci_iounmap(pdev, hqmv2_dev->hw.csr_kva);
	pci_iounmap(pdev, hqmv2_dev->hw.func_kva);
}

int hqmv2_pf_map_pci_bar_space(struct hqmv2_dev *hqmv2_dev,
			       struct pci_dev *pdev)
{
	int ret;

	/* BAR 0: PF FUNC BAR space */
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

	/* BAR 2: PF CSR BAR space */
	hqmv2_dev->hw.csr_kva = pci_iomap(pdev, 2, 0);
	hqmv2_dev->hw.csr_phys_addr = pci_resource_start(pdev, 2);

	if (!hqmv2_dev->hw.csr_kva) {
		HQMV2_ERR(&pdev->dev, "Cannot iomap BAR 2 (size %llu)\n",
			  pci_resource_len(pdev, 2));

		ret = -EIO;
		goto pci_iomap_bar2_fail;
	}

	HQMV2_INFO(&pdev->dev, "BAR 2 iomem base: %p\n",
		   hqmv2_dev->hw.csr_kva);
	HQMV2_INFO(&pdev->dev, "BAR 2 start: 0x%llx\n",
		   pci_resource_start(pdev, 2));
	HQMV2_INFO(&pdev->dev, "BAR 2 len: %llu\n",
		   pci_resource_len(pdev, 2));

	return 0;

pci_iomap_bar2_fail:
	pci_iounmap(pdev, hqmv2_dev->hw.func_kva);

	return ret;
}

#define HQMV2_LDB_CQ_BOUND (HQMV2_LDB_CQ_BASE + PAGE_SIZE * \
			    HQMV2_MAX_NUM_LDB_PORTS)
#define HQMV2_DIR_CQ_BOUND (HQMV2_DIR_CQ_BASE + PAGE_SIZE * \
			    HQMV2_MAX_NUM_DIR_PORTS)
#define HQMV2_LDB_PC_BOUND (HQMV2_LDB_PC_BASE + PAGE_SIZE * \
			    HQMV2_MAX_NUM_LDB_PORTS)
#define HQMV2_DIR_PC_BOUND (HQMV2_DIR_PC_BASE + PAGE_SIZE * \
			    HQMV2_MAX_NUM_DIR_PORTS)

int hqmv2_pf_mmap(struct file *f, struct vm_area_struct *vma, u32 domain_id)
{
	struct hqmv2_dev *dev;
	unsigned long bar_pgoff;
	unsigned long offset;
	u32 port_id;
	pgprot_t pgprot;

	dev = container_of(f->f_inode->i_cdev, struct hqmv2_dev, cdev);

	offset = vma->vm_pgoff << PAGE_SHIFT;

	if (offset >= HQMV2_LDB_CQ_BASE && offset < HQMV2_LDB_CQ_BOUND) {
		struct page *page;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - HQMV2_LDB_CQ_BASE) / PAGE_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		page = virt_to_page(dev->ldb_port_pages[port_id].cq_base);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= HQMV2_DIR_CQ_BASE && offset < HQMV2_DIR_CQ_BOUND) {
		struct page *page;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - HQMV2_DIR_CQ_BASE) / PAGE_SIZE;

		if (dev->ops->dir_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		page = virt_to_page(dev->dir_port_pages[port_id].cq_base);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= HQMV2_LDB_PC_BASE && offset < HQMV2_LDB_PC_BOUND) {
		struct page *page;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - HQMV2_LDB_PC_BASE) / PAGE_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		page = virt_to_page(dev->ldb_port_pages[port_id].pc_base);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= HQMV2_DIR_PC_BASE && offset < HQMV2_DIR_PC_BOUND) {
		struct page *page;

		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - HQMV2_DIR_PC_BASE) / PAGE_SIZE;

		if (dev->ops->dir_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		page = virt_to_page(dev->dir_port_pages[port_id].pc_base);

		return remap_pfn_range(vma,
				       vma->vm_start,
				       page_to_pfn(page),
				       vma->vm_end - vma->vm_start,
				       vma->vm_page_prot);

	} else if (offset >= HQMV2_LDB_PP_BASE && offset < HQMV2_LDB_PP_BOUND) {
		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - HQMV2_LDB_PP_BASE) / PAGE_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		pgprot = pgprot_noncached(vma->vm_page_prot);

	} else if (offset >= HQMV2_DIR_PP_BASE && offset < HQMV2_DIR_PP_BOUND) {
		bar_pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

		port_id = (offset - HQMV2_DIR_PP_BASE) / PAGE_SIZE;

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

static void hqmv2_mbox_cmd_register_fn(struct hqmv2_dev *dev,
				       int vf_id,
				       void *data)
{
	struct hqmv2_mbox_register_cmd_resp resp;
	struct hqmv2_mbox_register_cmd_req *req = data;

	/* The PF driver is backwards-compatible with older mailbox versions */
	if (req->interface_version > HQMV2_MBOX_INTERFACE_VERSION) {
		resp.hdr.status = HQMV2_MBOX_ST_VERSION_MISMATCH;
		resp.interface_version = HQMV2_MBOX_INTERFACE_VERSION;

		goto done;
	}

	/* S-IOV vdev locking is handled in the VDCM */
	if (hqmv2_hw_get_virt_mode(&dev->hw) == HQMV2_VIRT_SRIOV)
		hqmv2_lock_vdev(&dev->hw, vf_id);

	/* The VF can re-register without sending an unregister mailbox
	 * command (for example if the guest OS crashes). To protect against
	 * this, reset any in-use resources assigned to the driver now.
	 */
	hqmv2_reset_vdev(&dev->hw, vf_id);

	dev->vf_registered[vf_id] = true;

	resp.pf_id = dev->id;
	resp.vf_id = vf_id;
	resp.is_auxiliary_vf = dev->child_id_state[vf_id].is_auxiliary_vf;
	resp.primary_vf_id = dev->child_id_state[vf_id].primary_vf_id;
	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;

done:
	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_unregister_fn(struct hqmv2_dev *dev,
					 int vf_id,
				       void *data)
{
	struct hqmv2_mbox_unregister_cmd_resp resp;

	dev->vf_registered[vf_id] = false;

	hqmv2_reset_vdev(&dev->hw, vf_id);

	/* S-IOV vdev locking is handled in the VDCM */
	if (hqmv2_hw_get_virt_mode(&dev->hw) == HQMV2_VIRT_SRIOV)
		hqmv2_unlock_vdev(&dev->hw, vf_id);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_get_num_resources_fn(struct hqmv2_dev *dev,
						int vf_id,
					      void *data)
{
	struct hqmv2_mbox_get_num_resources_cmd_resp resp;
	struct hqmv2_get_num_resources_args arg;
	int ret;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_get_num_resources(&dev->hw, &arg, true, vf_id);

	mutex_unlock(&dev->resource_mutex);

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

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_create_sched_domain_fn(struct hqmv2_dev *dev,
						  int vf_id,
						void *data)
{
	struct hqmv2_mbox_create_sched_domain_cmd_req *req = data;
	struct hqmv2_mbox_create_sched_domain_cmd_resp resp;
	struct hqmv2_create_sched_domain_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
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

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_create_sched_domain(&dev->hw,
					   &hw_arg,
					   &hw_response,
					   true,
					   vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_reset_sched_domain_fn(struct hqmv2_dev *dev,
						 int vf_id,
						 void *data)
{
	struct hqmv2_mbox_reset_sched_domain_cmd_req *req = data;
	struct hqmv2_mbox_reset_sched_domain_cmd_resp resp;
	int ret;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_reset_domain(&dev->hw, req->id, true, vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_create_ldb_queue_fn(struct hqmv2_dev *dev,
					       int vf_id,
					       void *data)
{
	struct hqmv2_mbox_create_ldb_queue_cmd_req *req = data;
	struct hqmv2_mbox_create_ldb_queue_cmd_resp resp;
	struct hqmv2_create_ldb_queue_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.num_sequence_numbers = req->num_sequence_numbers;
	hw_arg.num_qid_inflights = req->num_qid_inflights;
	hw_arg.num_atomic_inflights = req->num_atomic_inflights;
	hw_arg.lock_id_comp_level = req->lock_id_comp_level;
	hw_arg.depth_threshold = req->depth_threshold;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_create_ldb_queue(&dev->hw,
					req->domain_id,
					&hw_arg,
					&hw_response,
					true,
					vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_create_dir_queue_fn(struct hqmv2_dev *dev,
					       int vf_id,
					       void *data)
{
	struct hqmv2_mbox_create_dir_queue_cmd_resp resp;
	struct hqmv2_mbox_create_dir_queue_cmd_req *req;
	struct hqmv2_create_dir_queue_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	req = (struct hqmv2_mbox_create_dir_queue_cmd_req *)data;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;
	hw_arg.depth_threshold = req->depth_threshold;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_create_dir_queue(&dev->hw,
					req->domain_id,
					&hw_arg,
					&hw_response,
					true,
					vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_create_ldb_port_fn(struct hqmv2_dev *dev,
					      int vf_id,
					      void *data)
{
	struct hqmv2_mbox_create_ldb_port_cmd_req *req = data;
	struct hqmv2_mbox_create_ldb_port_cmd_resp resp;
	struct hqmv2_create_ldb_port_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.cq_depth = req->cq_depth;
	hw_arg.cq_history_list_size = req->cq_history_list_size;
	hw_arg.cos_id = req->cos_id;
	hw_arg.cos_strict = req->cos_strict;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_create_ldb_port(&dev->hw,
				       req->domain_id,
				       &hw_arg,
				       req->pop_count_address,
				       req->cq_base_address,
				       &hw_response,
				       true,
				       vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_create_dir_port_fn(struct hqmv2_dev *dev,
					      int vf_id,
					      void *data)
{
	struct hqmv2_mbox_create_dir_port_cmd_resp resp;
	struct hqmv2_mbox_create_dir_port_cmd_req *req;
	struct hqmv2_create_dir_port_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	req = (struct hqmv2_mbox_create_dir_port_cmd_req *)data;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.cq_depth = req->cq_depth;
	hw_arg.queue_id = req->queue_id;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_create_dir_port(&dev->hw,
				       req->domain_id,
				       &hw_arg,
				       req->pop_count_address,
				       req->cq_base_address,
				       &hw_response,
				       true,
				       vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.id = hw_response.id;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_enable_ldb_port_fn(struct hqmv2_dev *dev,
					      int vf_id,
					      void *data)
{
	struct hqmv2_mbox_enable_ldb_port_cmd_req *req = data;
	struct hqmv2_mbox_enable_ldb_port_cmd_resp resp;
	struct hqmv2_enable_ldb_port_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_enable_ldb_port(&dev->hw,
				       req->domain_id,
				       &hw_arg,
				       &hw_response,
				       true,
				       vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;

	resp.error_code = ret;
	resp.status = hw_response.status;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_disable_ldb_port_fn(struct hqmv2_dev *dev,
					       int vf_id,
					       void *data)
{
	struct hqmv2_mbox_disable_ldb_port_cmd_req *req = data;
	struct hqmv2_mbox_disable_ldb_port_cmd_resp resp;
	struct hqmv2_disable_ldb_port_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_disable_ldb_port(&dev->hw,
					req->domain_id,
					&hw_arg,
					&hw_response,
					true,
					vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_enable_dir_port_fn(struct hqmv2_dev *dev,
					      int vf_id,
					      void *data)
{
	struct hqmv2_mbox_enable_dir_port_cmd_req *req = data;
	struct hqmv2_mbox_enable_dir_port_cmd_resp resp;
	struct hqmv2_enable_dir_port_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_enable_dir_port(&dev->hw,
				       req->domain_id,
				       &hw_arg,
				       &hw_response,
				       true,
				       vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_disable_dir_port_fn(struct hqmv2_dev *dev,
					       int vf_id,
					       void *data)
{
	struct hqmv2_mbox_disable_dir_port_cmd_req *req = data;
	struct hqmv2_mbox_disable_dir_port_cmd_resp resp;
	struct hqmv2_disable_dir_port_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_disable_dir_port(&dev->hw,
					req->domain_id,
					&hw_arg,
					&hw_response,
					true,
					vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_ldb_port_owned_by_domain_fn(struct hqmv2_dev *dev,
						       int vf_id,
						       void *data)
{
	struct hqmv2_mbox_ldb_port_owned_by_domain_cmd_req *req = data;
	struct hqmv2_mbox_ldb_port_owned_by_domain_cmd_resp resp;
	int ret;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_ldb_port_owned_by_domain(&dev->hw,
					     req->domain_id,
					     req->port_id,
					     true,
					     vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.owned = ret;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_dir_port_owned_by_domain_fn(struct hqmv2_dev *dev,
						       int vf_id,
						       void *data)
{
	struct hqmv2_mbox_dir_port_owned_by_domain_cmd_req *req = data;
	struct hqmv2_mbox_dir_port_owned_by_domain_cmd_resp resp;
	int ret;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_dir_port_owned_by_domain(&dev->hw,
					     req->domain_id,
					     req->port_id,
					     true,
					     vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.owned = ret;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_map_qid_fn(struct hqmv2_dev *dev,
				      int vf_id,
				      void *data)
{
	struct hqmv2_mbox_map_qid_cmd_req *req = data;
	struct hqmv2_mbox_map_qid_cmd_resp resp;
	struct hqmv2_map_qid_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;
	hw_arg.qid = req->qid;
	hw_arg.priority = req->priority;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_map_qid(&dev->hw,
			       req->domain_id,
			       &hw_arg,
			       &hw_response,
			       true,
			       vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_unmap_qid_fn(struct hqmv2_dev *dev,
					int vf_id,
					void *data)
{
	struct hqmv2_mbox_unmap_qid_cmd_req *req = data;
	struct hqmv2_mbox_unmap_qid_cmd_resp resp;
	struct hqmv2_unmap_qid_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.port_id = req->port_id;
	hw_arg.qid = req->qid;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_unmap_qid(&dev->hw,
				 req->domain_id,
				 &hw_arg,
				 &hw_response,
				 true,
				 vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_start_domain_fn(struct hqmv2_dev *dev,
					   int vf_id,
					   void *data)
{
	struct hqmv2_mbox_start_domain_cmd_req *req = data;
	struct hqmv2_mbox_start_domain_cmd_resp resp;
	struct hqmv2_start_domain_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_start_domain(&dev->hw,
				    req->domain_id,
				    &hw_arg,
				    &hw_response,
				    true,
				    vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_enable_ldb_port_intr_fn(struct hqmv2_dev *dev,
						   int vf_id,
						   void *data)
{
	struct hqmv2_mbox_enable_ldb_port_intr_cmd_req *req = data;
	struct hqmv2_mbox_enable_ldb_port_intr_cmd_resp resp;
	int ret, mode;

	mutex_lock(&dev->resource_mutex);

	if ((dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     dev->child_id_state[req->owner_vf].primary_vf_id != vf_id) ||
	    (!dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     req->owner_vf != vf_id)) {
		mutex_unlock(&dev->resource_mutex);
		resp.hdr.status = HQMV2_MBOX_ST_INVALID_OWNER_VF;
		goto finish;
	}

	if (hqmv2_hw_get_virt_mode(&dev->hw) == HQMV2_VIRT_SRIOV)
		mode = HQMV2_CQ_ISR_MODE_MSI;
	else
		mode = HQMV2_CQ_ISR_MODE_ADI;

	ret = hqmv2_configure_ldb_cq_interrupt(&dev->hw,
					       req->port_id,
					       req->vector,
					       mode,
					       vf_id,
					       req->owner_vf,
					       req->thresh);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

finish:
	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_enable_dir_port_intr_fn(struct hqmv2_dev *dev,
						   int vf_id,
						   void *data)
{
	struct hqmv2_mbox_enable_dir_port_intr_cmd_req *req = data;
	struct hqmv2_mbox_enable_dir_port_intr_cmd_resp resp;
	int ret, mode;

	mutex_lock(&dev->resource_mutex);

	if ((dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     dev->child_id_state[req->owner_vf].primary_vf_id != vf_id) ||
	    (!dev->child_id_state[req->owner_vf].is_auxiliary_vf &&
	     req->owner_vf != vf_id)) {
		mutex_unlock(&dev->resource_mutex);
		resp.hdr.status = HQMV2_MBOX_ST_INVALID_OWNER_VF;
		goto finish;
	}

	if (hqmv2_hw_get_virt_mode(&dev->hw) == HQMV2_VIRT_SRIOV)
		mode = HQMV2_CQ_ISR_MODE_MSI;
	else
		mode = HQMV2_CQ_ISR_MODE_ADI;

	ret = hqmv2_configure_dir_cq_interrupt(&dev->hw,
					       req->port_id,
					       req->vector,
					       mode,
					       vf_id,
					       req->owner_vf,
					       req->thresh);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

finish:
	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_arm_cq_intr_fn(struct hqmv2_dev *dev,
					  int vf_id,
					  void *data)
{
	struct hqmv2_mbox_arm_cq_intr_cmd_req *req = data;
	struct hqmv2_mbox_arm_cq_intr_cmd_resp resp;
	int ret;

	if (req->is_ldb)
		ret = hqmv2_ldb_port_owned_by_domain(&dev->hw,
						     req->domain_id,
						     req->port_id,
						     true,
						     vf_id);
	else
		ret = hqmv2_dir_port_owned_by_domain(&dev->hw,
						     req->domain_id,
						     req->port_id,
						     true,
						     vf_id);

	if (ret != 1) {
		resp.error_code = -EINVAL;
		goto finish;
	}

	resp.error_code = hqmv2_arm_cq_interrupt(&dev->hw,
						 req->port_id,
						 req->is_ldb,
						 true,
						 vf_id);

finish:
	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_get_num_used_resources_fn(struct hqmv2_dev *dev,
						     int vf_id,
						     void *data)
{
	struct hqmv2_mbox_get_num_resources_cmd_resp resp;
	struct hqmv2_get_num_resources_args arg;
	int ret;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_get_num_used_resources(&dev->hw, &arg, true, vf_id);

	mutex_unlock(&dev->resource_mutex);

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

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_init_cq_sched_count_fn(struct hqmv2_dev *dev,
						  int vf_id,
						  void *data)
{
	struct hqmv2_mbox_init_cq_sched_count_measure_cmd_req *req = data;
	struct hqmv2_mbox_init_cq_sched_count_measure_cmd_resp resp;
	struct hqmv2_perf_metric_pre_data *pre = NULL;
	struct timespec64 start;
	int ret = 0;

	/* Check if the measurement hardware is already in use. */
	if (!mutex_trylock(&dev->perf.measurement_mutex)) {
		ret = -EBUSY;
		goto finish;
	}

	pre = devm_kmalloc(&dev->pdev->dev, sizeof(*pre), GFP_KERNEL);
	if (!pre) {
		ret = -ENOMEM;
		mutex_unlock(&dev->perf.measurement_mutex);
		goto finish;
	}

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		ret = -EINVAL;
		mutex_unlock(&dev->perf.measurement_mutex);
		goto finish;
	}

	ktime_get_real_ts64(&start);

	hqmv2_init_perf_metric_measurement(&dev->hw,
					   0,
					   req->duration_us,
					   pre,
					   true,
					   vf_id);

	dev->perf.start = start;
	dev->perf.measurement_duration_us = req->duration_us;
	dev->perf.pre = pre;
	dev->perf.vf_id = vf_id;
	dev->perf.data[vf_id].valid = false;

	/* Don't unlock the measurement mutex until the data is collected */

finish:
	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_collect_cq_sched_count_fn(struct hqmv2_dev *dev,
						     int vf_id,
						     void *data)
{
	struct hqmv2_mbox_collect_cq_sched_count_cmd_req *req = data;
	struct hqmv2_mbox_collect_cq_sched_count_cmd_resp resp;
	int i, ret = 0;

	if ((req->is_ldb && req->cq_id >= HQMV2_MAX_NUM_LDB_PORTS) ||
	    (!req->is_ldb && req->cq_id >= HQMV2_MAX_NUM_DIR_PORTS)) {
		ret = -EINVAL;
		goto finish;
	}

	/* If this is the first time collecting data for a measurement, we
	 * extract the CQ sched count data from hardware, store it in memory,
	 * and release the measurement mutex.
	 */
	if (!dev->perf.data[vf_id].valid) {
		union hqmv2_perf_metric_group_data perf_data;
		struct timespec64 start, end;
		u32 elapsed, duration;

		duration = dev->perf.measurement_duration_us;

		/* Collect the CQ sched count data */
		hqmv2_collect_perf_metric_data(&dev->hw,
					       0,
					       duration,
					       dev->perf.pre,
					       &perf_data,
					       true,
					       vf_id);

		ktime_get_real_ts64(&end);

		start = dev->perf.start;

		elapsed = (end.tv_sec - start.tv_sec) * 1000000 +
			(end.tv_nsec - start.tv_nsec) / 1000;

		/* Store the data for the VF to read later */
		for (i = 0; i < HQMV2_MAX_NUM_LDB_PORTS; i++) {
			dev->perf.data[vf_id].ldb_cq[i] =
				perf_data.group_0.hqmv2_ldb_cq_sched_count[i];
		}

		for (i = 0; i < HQMV2_MAX_NUM_DIR_PORTS; i++)
			dev->perf.data[vf_id].dir_cq[i] =
				perf_data.group_0.hqmv2_dir_cq_sched_count[i];

		devm_kfree(&dev->pdev->dev, dev->perf.pre);

		dev->perf.data[vf_id].elapsed = elapsed;
		dev->perf.data[vf_id].valid = true;

		mutex_unlock(&dev->perf.measurement_mutex);
	}

	if (req->is_ldb)
		resp.cq_sched_count = dev->perf.data[vf_id].ldb_cq[req->cq_id];
	else
		resp.cq_sched_count = dev->perf.data[vf_id].dir_cq[req->cq_id];

	resp.elapsed = dev->perf.data[vf_id].elapsed;

finish:
	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_get_sn_allocation_fn(struct hqmv2_dev *dev,
						int vf_id,
						void *data)
{
	struct hqmv2_mbox_get_sn_allocation_cmd_req *req = data;
	struct hqmv2_mbox_get_sn_allocation_cmd_resp resp;

	mutex_lock(&dev->resource_mutex);

	resp.num = hqmv2_get_group_sequence_numbers(&dev->hw, req->group_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_get_ldb_queue_depth_fn(struct hqmv2_dev *dev,
						  int vf_id,
						  void *data)
{
	struct hqmv2_mbox_get_ldb_queue_depth_cmd_req *req = data;
	struct hqmv2_mbox_get_ldb_queue_depth_cmd_resp resp;
	struct hqmv2_get_ldb_queue_depth_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.queue_id = req->queue_id;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_get_ldb_queue_depth(&dev->hw,
					   req->domain_id,
					   &hw_arg,
					   &hw_response,
					   true,
					   vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.depth = hw_response.id;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_get_dir_queue_depth_fn(struct hqmv2_dev *dev,
						  int vf_id,
						  void *data)
{
	struct hqmv2_mbox_get_dir_queue_depth_cmd_req *req = data;
	struct hqmv2_mbox_get_dir_queue_depth_cmd_resp resp;
	struct hqmv2_get_dir_queue_depth_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.response = (uintptr_t)&hw_response;
	hw_arg.queue_id = req->queue_id;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_get_dir_queue_depth(&dev->hw,
					   req->domain_id,
					   &hw_arg,
					   &hw_response,
					   true,
					   vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.depth = hw_response.id;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_pending_port_unmaps_fn(struct hqmv2_dev *dev,
						  int vf_id,
						  void *data)
{
	struct hqmv2_mbox_pending_port_unmaps_cmd_req *req = data;
	struct hqmv2_mbox_pending_port_unmaps_cmd_resp resp;
	struct hqmv2_pending_port_unmaps_args hw_arg;
	struct hqmv2_cmd_response hw_response = {0};
	int ret;

	hw_arg.port_id = req->port_id;

	mutex_lock(&dev->resource_mutex);

	ret = hqmv2_hw_pending_port_unmaps(&dev->hw,
					   req->domain_id,
					   &hw_arg,
					   &hw_response,
					   true,
					   vf_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;
	resp.error_code = ret;
	resp.status = hw_response.status;
	resp.num = hw_response.id;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

static void hqmv2_mbox_cmd_get_cos_bw_fn(struct hqmv2_dev *dev,
					 int vf_id,
					 void *data)
{
	struct hqmv2_mbox_get_cos_bw_cmd_req *req = data;
	struct hqmv2_mbox_get_cos_bw_cmd_resp resp;

	mutex_lock(&dev->resource_mutex);

	resp.num = hqmv2_hw_get_cos_bandwidth(&dev->hw, req->cos_id);

	mutex_unlock(&dev->resource_mutex);

	resp.hdr.status = HQMV2_MBOX_ST_SUCCESS;

	hqmv2_pf_write_vf_mbox_resp(&dev->hw, vf_id, &resp, sizeof(resp));
}

void (*mbox_fn_table[])(struct hqmv2_dev *dev, int vf_id, void *data) = {
	hqmv2_mbox_cmd_register_fn,
	hqmv2_mbox_cmd_unregister_fn,
	hqmv2_mbox_cmd_get_num_resources_fn,
	hqmv2_mbox_cmd_create_sched_domain_fn,
	hqmv2_mbox_cmd_reset_sched_domain_fn,
	hqmv2_mbox_cmd_create_ldb_queue_fn,
	hqmv2_mbox_cmd_create_dir_queue_fn,
	hqmv2_mbox_cmd_create_ldb_port_fn,
	hqmv2_mbox_cmd_create_dir_port_fn,
	hqmv2_mbox_cmd_enable_ldb_port_fn,
	hqmv2_mbox_cmd_disable_ldb_port_fn,
	hqmv2_mbox_cmd_enable_dir_port_fn,
	hqmv2_mbox_cmd_disable_dir_port_fn,
	hqmv2_mbox_cmd_ldb_port_owned_by_domain_fn,
	hqmv2_mbox_cmd_dir_port_owned_by_domain_fn,
	hqmv2_mbox_cmd_map_qid_fn,
	hqmv2_mbox_cmd_unmap_qid_fn,
	hqmv2_mbox_cmd_start_domain_fn,
	hqmv2_mbox_cmd_enable_ldb_port_intr_fn,
	hqmv2_mbox_cmd_enable_dir_port_intr_fn,
	hqmv2_mbox_cmd_arm_cq_intr_fn,
	hqmv2_mbox_cmd_get_num_used_resources_fn,
	hqmv2_mbox_cmd_init_cq_sched_count_fn,
	hqmv2_mbox_cmd_collect_cq_sched_count_fn,
	hqmv2_mbox_cmd_get_sn_allocation_fn,
	hqmv2_mbox_cmd_get_ldb_queue_depth_fn,
	hqmv2_mbox_cmd_get_dir_queue_depth_fn,
	hqmv2_mbox_cmd_pending_port_unmaps_fn,
	hqmv2_mbox_cmd_get_cos_bw_fn,
};

static u32 hqmv2_handle_vf_flr_interrupt(struct hqmv2_dev *dev)
{
	u32 bitvec;
	int i;

	bitvec = hqmv2_read_vf_flr_int_bitvec(&dev->hw);

	for (i = 0; i < HQMV2_MAX_NUM_VDEVS; i++) {
		if (!(bitvec & (1 << i)))
			continue;

		HQMV2_INFO(dev->hqmv2_device,
			   "Received VF FLR ISR from VF %d\n",
			   i);

		hqmv2_reset_vdev(&dev->hw, i);
	}

	hqmv2_ack_vf_flr_int(&dev->hw, bitvec);

	return bitvec;
}

/**********************************/
/****** Interrupt management ******/
/**********************************/

void hqmv2_handle_mbox_interrupt(struct hqmv2_dev *dev, int id)
{
	u8 data[HQMV2_VF2PF_REQ_BYTES];

	HQMV2_INFO(dev->hqmv2_device, "Received VF->PF ISR from VF %d\n", id);

	hqmv2_pf_read_vf_mbox_req(&dev->hw, id, data, sizeof(data));

	/* Unrecognized request command, send an error response */
	if (HQMV2_MBOX_CMD_TYPE(data) >= NUM_HQMV2_MBOX_CMD_TYPES) {
		struct hqmv2_mbox_resp_hdr resp;

		resp.status = HQMV2_MBOX_ST_INVALID_CMD_TYPE;

		hqmv2_pf_write_vf_mbox_resp(&dev->hw,
					    id,
					    &resp,
					    sizeof(resp));
	} else {
		HQMV2_INFO(dev->hqmv2_device,
			   "Received mbox command %s\n",
			   HQMV2_MBOX_CMD_STRING(data));

		mbox_fn_table[HQMV2_MBOX_CMD_TYPE(data)](dev, id, data);
	}

	hqmv2_ack_vdev_mbox_int(&dev->hw, 1 << id);
}

static u32 hqmv2_handle_vf_to_pf_interrupt(struct hqmv2_dev *dev)
{
	u32 bitvec;
	int i;

	bitvec = hqmv2_read_vdev_to_pf_int_bitvec(&dev->hw);

	for (i = 0; i < HQMV2_MAX_NUM_VDEVS; i++) {
		if (!(bitvec & (1 << i)))
			continue;

		hqmv2_handle_mbox_interrupt(dev, i);
	}

	return bitvec;
}

static void hqmv2_handle_vf_requests(struct hqmv2_hw *hw)
{
	struct hqmv2_dev *dev;
	u32 mbox_bitvec;
	u32 flr_bitvec;

	dev = container_of(hw, struct hqmv2_dev, hw);

	flr_bitvec = hqmv2_handle_vf_flr_interrupt(dev);

	mbox_bitvec = hqmv2_handle_vf_to_pf_interrupt(dev);

	hqmv2_ack_vdev_to_pf_int(hw, mbox_bitvec, flr_bitvec);
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
static irqreturn_t hqmv2_service_intr_handler(int irq, void *hdlr_ptr)
{
	struct hqmv2_dev *dev = (struct hqmv2_dev *)hdlr_ptr;

	HQMV2_INFO(dev->hqmv2_device, "HQM service interrupt fired\n");

	/* Only one thread should execute the service interrupt handler at a
	 * time.
	 */
	mutex_lock(&dev->svc_isr_mutex);

	hqmv2_handle_service_intr(&dev->hw, hqmv2_handle_vf_requests);

	mutex_unlock(&dev->svc_isr_mutex);

	return IRQ_HANDLED;
}

static int hqmv2_init_alarm_interrupts(struct hqmv2_dev *dev,
				       struct pci_dev *pdev)
{
	int i, ret;

	for (i = 0; i < HQMV2_PF_NUM_NON_CQ_INTERRUPT_VECTORS; i++) {
		ret = devm_request_threaded_irq(&pdev->dev,
						pci_irq_vector(pdev, i),
						NULL,
						hqmv2_service_intr_handler,
						IRQF_ONESHOT,
						"hqmv2_alarm",
						dev);
		if (ret)
			return ret;

		dev->intr.isr_registered[i] = true;
	}

	hqmv2_enable_alarm_interrupts(&dev->hw);

	return 0;
}

static irqreturn_t hqmv2_compressed_cq_intr_handler(int irq, void *hdlr_ptr)
{
	struct hqmv2_dev *dev = (struct hqmv2_dev *)hdlr_ptr;
	u32 ldb_cq_interrupts[HQMV2_MAX_NUM_LDB_PORTS / 32];
	u32 dir_cq_interrupts[HQMV2_MAX_NUM_DIR_PORTS / 32];
	int i;

	HQMV2_INFO(dev->hqmv2_device, "Entered ISR\n");

	hqmv2_read_compressed_cq_intr_status(&dev->hw,
					     ldb_cq_interrupts,
					     dir_cq_interrupts);

	hqmv2_ack_compressed_cq_intr(&dev->hw,
				     ldb_cq_interrupts,
				     dir_cq_interrupts);

	for (i = 0; i < HQMV2_MAX_NUM_LDB_PORTS; i++) {
		if (!(ldb_cq_interrupts[i / 32] & (1 << (i % 32))))
			continue;

		HQMV2_INFO(dev->hqmv2_device, "[%s()] Waking LDB port %d\n",
			   __func__, i);

		hqmv2_wake_thread(dev, &dev->intr.ldb_cq_intr[i], WAKE_CQ_INTR);
	}

	for (i = 0; i < HQMV2_MAX_NUM_DIR_PORTS; i++) {
		if (!(dir_cq_interrupts[i / 32] & (1 << (i % 32))))
			continue;

		HQMV2_INFO(dev->hqmv2_device, "[%s()] Waking DIR port %d\n",
			   __func__, i);

		hqmv2_wake_thread(dev, &dev->intr.dir_cq_intr[i], WAKE_CQ_INTR);
	}

	return IRQ_HANDLED;
}

static int hqmv2_init_compressed_mode_interrupts(struct hqmv2_dev *dev,
						 struct pci_dev *pdev)
{
	int ret, irq;

	irq = pci_irq_vector(pdev, HQMV2_PF_COMPRESSED_MODE_CQ_VECTOR_ID);

	ret = devm_request_irq(&pdev->dev,
			       irq,
			       hqmv2_compressed_cq_intr_handler,
			       0,
			       "hqmv2_compressed_cq",
			       dev);
	if (ret)
		return ret;

	dev->intr.isr_registered[HQMV2_PF_COMPRESSED_MODE_CQ_VECTOR_ID] = true;

	dev->intr.mode = HQMV2_MSIX_MODE_COMPRESSED;

	hqmv2_set_msix_mode(&dev->hw, HQMV2_MSIX_MODE_COMPRESSED);

	return 0;
}

int hqmv2_pf_init_interrupts(struct hqmv2_dev *dev, struct pci_dev *pdev)
{
	int ret, i;

	/* HQM supports two modes for CQ interrupts:
	 * - "compressed mode": all CQ interrupts are packed into a single
	 *	vector. The ISR reads six interrupt status registers to
	 *	determine the source(s).
	 * - "packed mode" (unused): the hardware supports up to 64 vectors.
	 */

	ret = pci_alloc_irq_vectors(pdev,
				    HQMV2_PF_NUM_COMPRESSED_MODE_VECTORS,
				    HQMV2_PF_NUM_COMPRESSED_MODE_VECTORS,
				    PCI_IRQ_MSIX);
	if (ret < 0)
		return ret;

	dev->intr.num_vectors = ret;
	dev->intr.base_vector = pci_irq_vector(pdev, 0);

	ret = hqmv2_init_alarm_interrupts(dev, pdev);
	if (ret)
		return ret;

	ret = hqmv2_init_compressed_mode_interrupts(dev, pdev);
	if (ret)
		return ret;

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

/* If the device is reset during use, its interrupt registers need to be
 * reinitialized.
 */
void hqmv2_pf_reinit_interrupts(struct hqmv2_dev *dev)
{
	hqmv2_enable_alarm_interrupts(&dev->hw);

	hqmv2_set_msix_mode(&dev->hw, HQMV2_MSIX_MODE_COMPRESSED);
}

void hqmv2_pf_free_interrupts(struct hqmv2_dev *dev, struct pci_dev *pdev)
{
	int i;

	for (i = 0; i < dev->intr.num_vectors; i++) {
		if (dev->intr.isr_registered[i])
			devm_free_irq(&pdev->dev, pci_irq_vector(pdev, i), dev);
	}

	pci_free_irq_vectors(pdev);
}

int hqmv2_pf_enable_ldb_cq_interrupts(struct hqmv2_dev *dev,
				      int id,
				      u16 thresh)
{
	int mode, vec;

	if (dev->intr.mode == HQMV2_MSIX_MODE_COMPRESSED) {
		mode = HQMV2_CQ_ISR_MODE_MSIX;
		vec = 0;
	} else {
		mode = HQMV2_CQ_ISR_MODE_MSIX;
		vec = id % 64;
	}

	dev->intr.ldb_cq_intr[id].disabled = false;
	dev->intr.ldb_cq_intr[id].configured = true;

	return hqmv2_configure_ldb_cq_interrupt(&dev->hw, id, vec,
						mode, 0, 0, thresh);
}

int hqmv2_pf_enable_dir_cq_interrupts(struct hqmv2_dev *dev,
				      int id,
				      u16 thresh)
{
	int mode, vec;

	if (dev->intr.mode == HQMV2_MSIX_MODE_COMPRESSED) {
		mode = HQMV2_CQ_ISR_MODE_MSIX;
		vec = 0;
	} else {
		mode = HQMV2_CQ_ISR_MODE_MSIX;
		vec = id % 64;
	}

	dev->intr.dir_cq_intr[id].disabled = false;
	dev->intr.dir_cq_intr[id].configured = true;

	return hqmv2_configure_dir_cq_interrupt(&dev->hw, id, vec,
						mode, 0, 0, thresh);
}

int hqmv2_pf_arm_cq_interrupt(struct hqmv2_dev *dev,
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

	return hqmv2_arm_cq_interrupt(&dev->hw, port_id, is_ldb, false, 0);
}

/*******************************/
/****** Driver management ******/
/*******************************/

int hqmv2_pf_init_driver_state(struct hqmv2_dev *hqmv2_dev)
{
	int i, ret;

	hqmv2_dev->wq = create_singlethread_workqueue("HQM queue remapper");
	if (!hqmv2_dev->wq)
		return -EINVAL;

	if (movdir64b_supported()) {
		hqmv2_dev->enqueue_four = hqmv2_movdir64b;
	} else {
#ifdef CONFIG_AS_SSE2
		hqmv2_dev->enqueue_four = hqmv2_movntdq;
#else
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "%s: Platforms without movdir64 must support SSE2\n",
			  hqmv2_driver_name);
		ret = -EINVAL;
		goto enqueue_four_fail;
#endif
	}

#if 0
#ifdef CONFIG_INTEL_HQMV2_SIOV
	ret = hqmv2_vdcm_init(hqmv2_dev->pdev);
	if (ret)
		goto vdcm_init_fail;
#endif
#endif

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

	for (i = 0; i < HQMV2_MAX_NUM_VDEVS; i++)
		hqmv2_dev->child_id_state[i].is_auxiliary_vf = false;

	hqmv2_hw_set_virt_mode(&hqmv2_dev->hw, HQMV2_VIRT_NONE);

	mutex_init(&hqmv2_dev->resource_mutex);
	mutex_init(&hqmv2_dev->perf.measurement_mutex);
	mutex_init(&hqmv2_dev->svc_isr_mutex);

	return 0;

#if 0
#ifdef CONFIG_INTEL_HQMV2_SIOV
vdcm_init_fail:
#endif
#endif
#ifndef CONFIG_AS_SSE2
enqueue_four_fail:
	destroy_workqueue(hqmv2_dev->wq);
#endif
	return ret;
}

void hqmv2_pf_free_driver_state(struct hqmv2_dev *hqmv2_dev)
{
#if 0
#ifdef CONFIG_INTEL_HQMV2_SIOV
	hqmv2_vdcm_exit(hqmv2_dev->pdev);
#endif
#endif

	destroy_workqueue(hqmv2_dev->wq);
}

int hqmv2_pf_cdev_add(struct hqmv2_dev *hqmv2_dev,
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

void hqmv2_pf_cdev_del(struct hqmv2_dev *hqmv2_dev)
{
	cdev_del(&hqmv2_dev->cdev);
}

int hqmv2_pf_device_create(struct hqmv2_dev *hqmv2_dev,
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

void hqmv2_pf_device_destroy(struct hqmv2_dev *hqmv2_dev,
			     struct class *ws_class)
{
	device_destroy(ws_class,
		       MKDEV(MAJOR(hqmv2_dev->dev_number),
			     MINOR(hqmv2_dev->dev_number) +
				HQMV2_MAX_NUM_DOMAINS));
}

int hqmv2_pf_register_driver(struct hqmv2_dev *hqmv2_dev)
{
	/* Function intentionally left blank */
	return 0;
}

void hqmv2_pf_unregister_driver(struct hqmv2_dev *hqmv2_dev)
{
	/* Function intentionally left blank */
}

void hqmv2_pf_enable_pm(struct hqmv2_dev *hqmv2_dev)
{
	hqmv2_clr_pmcsr_disable(&hqmv2_dev->hw);
}

#define HQM_READY_RETRY_LIMIT 1000
int hqmv2_pf_wait_for_device_ready(struct hqmv2_dev *hqmv2_dev,
				   struct pci_dev *pdev)
{
	u32 retries = 0;

	/* Allow at least 1s for the device to become active after power-on */
	for (retries = 0; retries < HQM_READY_RETRY_LIMIT; retries++) {
		union hqmv2_cfg_mstr_cfg_diagnostic_idle_status idle;
		union hqmv2_cfg_mstr_cfg_pm_status pm_st;
		u32 addr;

		addr = HQMV2_CFG_MSTR_CFG_PM_STATUS;

		pm_st.val = HQMV2_CSR_RD(&hqmv2_dev->hw, addr);

		addr = HQMV2_CFG_MSTR_CFG_DIAGNOSTIC_IDLE_STATUS;

		idle.val = HQMV2_CSR_RD(&hqmv2_dev->hw, addr);

		if (pm_st.field.pmsm == 1 && idle.field.hqm_func_idle == 1)
			break;

		HQMV2_INFO(&pdev->dev, "[%s()] pm_st: 0x%x\n",
			   __func__, pm_st.val);
		HQMV2_INFO(&pdev->dev, "[%s()] idle: 0x%x\n",
			   __func__, idle.val);

		usleep_range(1000, 2000);
	};

	if (retries == HQM_READY_RETRY_LIMIT) {
		HQMV2_ERR(&pdev->dev, "Device idle test failed\n");
		return -EIO;
	}

	return 0;
}

/*****************************/
/****** Sysfs callbacks ******/
/*****************************/

static ssize_t hqmv2_sysfs_aux_vf_ids_read(struct device *dev,
					   struct device_attribute *attr,
					 char *buf,
					 int vf_id)
{
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);
	int i, size;

	mutex_lock(&hqmv2_dev->resource_mutex);

	size = 0;

	for (i = 0; i < HQMV2_MAX_NUM_VDEVS; i++) {
		if (!hqmv2_dev->child_id_state[i].is_auxiliary_vf)
			continue;

		if (hqmv2_dev->child_id_state[i].primary_vf_id != vf_id)
			continue;

		size += scnprintf(&buf[size], PAGE_SIZE - size, "%d,", i);
	}

	if (size == 0)
		size = 1;

	/* Replace the last comma with a newline */
	size += scnprintf(&buf[size - 1], PAGE_SIZE - size, "\n");

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return size;
}

static ssize_t hqmv2_sysfs_aux_vf_ids_write(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf,
					    size_t count,
					    int primary_vf_id)
{
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);
	char *user_buf = (char *)buf;
	unsigned int vf_id;
	char *vf_id_str;

	mutex_lock(&hqmv2_dev->resource_mutex);

	/* If the primary VF is locked, no auxiliary VFs can be added to or
	 * removed from it.
	 */
	if (hqmv2_vdev_is_locked(&hqmv2_dev->hw, primary_vf_id)) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -EINVAL;
	}

	vf_id_str = strsep(&user_buf, ",");
	while (vf_id_str) {
		struct vf_id_state *child_id_state;
		int ret;

		ret = kstrtouint(vf_id_str, 0, &vf_id);
		if (ret) {
			mutex_unlock(&hqmv2_dev->resource_mutex);
			return -EINVAL;
		}

		if (vf_id >= HQMV2_MAX_NUM_VDEVS) {
			mutex_unlock(&hqmv2_dev->resource_mutex);
			return -EINVAL;
		}

		child_id_state = &hqmv2_dev->child_id_state[vf_id];

		if (vf_id == primary_vf_id) {
			mutex_unlock(&hqmv2_dev->resource_mutex);
			return -EINVAL;
		}

		/* Check if the aux-primary VF relationship already exists */
		if (child_id_state->is_auxiliary_vf &&
		    child_id_state->primary_vf_id == primary_vf_id) {
			vf_id_str = strsep(&user_buf, ",");
			continue;
		}

		/* If the desired VF is locked, it can't be made auxiliary */
		if (hqmv2_vdev_is_locked(&hqmv2_dev->hw, vf_id)) {
			mutex_unlock(&hqmv2_dev->resource_mutex);
			return -EINVAL;
		}

		/* Attempt to reassign the VF */
		child_id_state->is_auxiliary_vf = true;
		child_id_state->primary_vf_id = primary_vf_id;

		/* Reassign any of the desired VF's resources back to the PF */
		hqmv2_reset_vdev_resources(&hqmv2_dev->hw, vf_id);

		vf_id_str = strsep(&user_buf, ",");
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return count;
}

static ssize_t hqmv2_sysfs_vf_read(struct device *dev,
				   struct device_attribute *attr,
				   char *buf,
				   int vf_id)
{
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);
	struct hqmv2_get_num_resources_args num_avail_rsrcs;
	struct hqmv2_get_num_resources_args num_used_rsrcs;
	struct hqmv2_get_num_resources_args num_rsrcs;
	struct hqmv2_hw *hw = &hqmv2_dev->hw;
	int val;

	mutex_lock(&hqmv2_dev->resource_mutex);

	val = hqmv2_hw_get_num_resources(hw, &num_avail_rsrcs, true, vf_id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	val = hqmv2_hw_get_num_used_resources(hw, &num_used_rsrcs, true, vf_id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

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
		val = (int)hqmv2_vdev_is_locked(hw, vf_id);
	else
		return -1;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t hqmv2_sysfs_vf_write(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf,
				    size_t count,
				    int vf_id)
{
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);
	struct hqmv2_hw *hw = &hqmv2_dev->hw;
	unsigned long num;
	const char *name;
	int ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	name = attr->attr.name;

	mutex_lock(&hqmv2_dev->resource_mutex);

	if (strncmp(name, "num_sched_domains",
		    sizeof("num_sched_domains")) == 0) {
		ret = hqmv2_update_vdev_sched_domains(hw, vf_id, num);
	} else if (strncmp(name, "num_ldb_queues",
			   sizeof("num_ldb_queues")) == 0) {
		ret = hqmv2_update_vdev_ldb_queues(hw, vf_id, num);
	} else if (strncmp(name, "num_ldb_ports",
			   sizeof("num_ldb_ports")) == 0) {
		ret = hqmv2_update_vdev_ldb_ports(hw, vf_id, num);
	} else if (strncmp(name, "num_cos0_ldb_ports",
			   sizeof("num_cos0_ldb_ports")) == 0) {
		ret = hqmv2_update_vdev_ldb_cos_ports(hw, vf_id, 0, num);
	} else if (strncmp(name, "num_cos1_ldb_ports",
			   sizeof("num_cos1_ldb_ports")) == 0) {
		ret = hqmv2_update_vdev_ldb_cos_ports(hw, vf_id, 1, num);
	} else if (strncmp(name, "num_cos2_ldb_ports",
			   sizeof("num_cos2_ldb_ports")) == 0) {
		ret = hqmv2_update_vdev_ldb_cos_ports(hw, vf_id, 2, num);
	} else if (strncmp(name, "num_cos3_ldb_ports",
			   sizeof("num_cos3_ldb_ports")) == 0) {
		ret = hqmv2_update_vdev_ldb_cos_ports(hw, vf_id, 3, num);
	} else if (strncmp(name, "num_dir_ports",
			   sizeof("num_dir_ports")) == 0) {
		ret = hqmv2_update_vdev_dir_ports(hw, vf_id, num);
	} else if (strncmp(name, "num_ldb_credits",
			   sizeof("num_ldb_credits")) == 0) {
		ret = hqmv2_update_vdev_ldb_credits(hw, vf_id, num);
	} else if (strncmp(name, "num_dir_credits",
			   sizeof("num_dir_credits")) == 0) {
		ret = hqmv2_update_vdev_dir_credits(hw, vf_id, num);
	} else if (strncmp(attr->attr.name, "num_hist_list_entries",
			   sizeof("num_hist_list_entries")) == 0) {
		ret = hqmv2_update_vdev_hist_list_entries(hw, vf_id, num);
	} else if (strncmp(attr->attr.name, "num_atomic_inflights",
			   sizeof("num_atomic_inflights")) == 0) {
		ret = hqmv2_update_vdev_atomic_inflights(hw, vf_id, num);
	} else {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

#define HQMV2_VF_SYSFS_RD_FUNC(id) \
static ssize_t hqmv2_sysfs_vf ## id ## _read(		      \
	struct device *dev,				      \
	struct device_attribute *attr,			      \
	char *buf)					      \
{							      \
	return hqmv2_sysfs_vf_read(dev, attr, buf, id);	      \
}							      \

#define HQMV2_VF_SYSFS_WR_FUNC(id) \
static ssize_t hqmv2_sysfs_vf ## id ## _write(			\
	struct device *dev,					\
	struct device_attribute *attr,				\
	const char *buf,					\
	size_t count)						\
{								\
	return hqmv2_sysfs_vf_write(dev, attr, buf, count, id); \
}

#define HQMV2_AUX_VF_ID_RD_FUNC(id) \
static ssize_t hqmv2_sysfs_vf ## id ## _vf_ids_read(		      \
	struct device *dev,					      \
	struct device_attribute *attr,				      \
	char *buf)						      \
{								      \
	return hqmv2_sysfs_aux_vf_ids_read(dev, attr, buf, id);	      \
}								      \

#define HQMV2_AUX_VF_ID_WR_FUNC(id) \
static ssize_t hqmv2_sysfs_vf ## id ## _vf_ids_write(			\
	struct device *dev,						\
	struct device_attribute *attr,					\
	const char *buf,						\
	size_t count)							\
{									\
	return hqmv2_sysfs_aux_vf_ids_write(dev, attr, buf, count, id); \
}

/* Read-write per-resource-group sysfs files */
#define HQMV2_VF_DEVICE_ATTRS(id) \
static struct device_attribute			    \
dev_attr_vf ## id ## _sched_domains =		    \
	__ATTR(num_sched_domains,		    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _ldb_queues =		    \
	__ATTR(num_ldb_queues,			    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _ldb_ports =		    \
	__ATTR(num_ldb_ports,			    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _cos0_ldb_ports =		    \
	__ATTR(num_cos0_ldb_ports,		    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _cos1_ldb_ports =		    \
	__ATTR(num_cos1_ldb_ports,		    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _cos2_ldb_ports =		    \
	__ATTR(num_cos2_ldb_ports,		    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _cos3_ldb_ports =		    \
	__ATTR(num_cos3_ldb_ports,		    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _dir_ports =		    \
	__ATTR(num_dir_ports,			    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _ldb_credits =		    \
	__ATTR(num_ldb_credits,			    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _dir_credits =		    \
	__ATTR(num_dir_credits,			    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _hist_list_entries =	    \
	__ATTR(num_hist_list_entries,		    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _atomic_inflights =	    \
	__ATTR(num_atomic_inflights,		    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       hqmv2_sysfs_vf ## id ## _write);	    \
static struct device_attribute			    \
dev_attr_vf ## id ## _locked =			    \
	__ATTR(locked,				    \
	       0444,				    \
	       hqmv2_sysfs_vf ## id ## _read,	    \
	       NULL);				    \
static struct device_attribute			    \
dev_attr_vf ## id ## _aux_vf_ids =		    \
	__ATTR(aux_vf_ids,			    \
	       0644,				    \
	       hqmv2_sysfs_vf ## id ## _vf_ids_read,  \
	       hqmv2_sysfs_vf ## id ## _vf_ids_write) \

#define HQMV2_VF_SYSFS_ATTRS(id) \
HQMV2_VF_DEVICE_ATTRS(id);				\
static struct attribute *hqmv2_vf ## id ## _attrs[] = {	\
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
	&dev_attr_vf ## id ## _aux_vf_ids.attr,		\
	NULL						\
}

#define HQMV2_VF_SYSFS_ATTR_GROUP(id) \
HQMV2_VF_SYSFS_ATTRS(id);					\
static struct attribute_group hqmv2_vf ## id ## _attr_group = {	\
	.attrs = hqmv2_vf ## id ## _attrs,			\
	.name = "vf" #id "_resources"				\
}

HQMV2_VF_SYSFS_RD_FUNC(0);
HQMV2_VF_SYSFS_RD_FUNC(1);
HQMV2_VF_SYSFS_RD_FUNC(2);
HQMV2_VF_SYSFS_RD_FUNC(3);
HQMV2_VF_SYSFS_RD_FUNC(4);
HQMV2_VF_SYSFS_RD_FUNC(5);
HQMV2_VF_SYSFS_RD_FUNC(6);
HQMV2_VF_SYSFS_RD_FUNC(7);
HQMV2_VF_SYSFS_RD_FUNC(8);
HQMV2_VF_SYSFS_RD_FUNC(9);
HQMV2_VF_SYSFS_RD_FUNC(10);
HQMV2_VF_SYSFS_RD_FUNC(11);
HQMV2_VF_SYSFS_RD_FUNC(12);
HQMV2_VF_SYSFS_RD_FUNC(13);
HQMV2_VF_SYSFS_RD_FUNC(14);
HQMV2_VF_SYSFS_RD_FUNC(15);

HQMV2_VF_SYSFS_WR_FUNC(0);
HQMV2_VF_SYSFS_WR_FUNC(1);
HQMV2_VF_SYSFS_WR_FUNC(2);
HQMV2_VF_SYSFS_WR_FUNC(3);
HQMV2_VF_SYSFS_WR_FUNC(4);
HQMV2_VF_SYSFS_WR_FUNC(5);
HQMV2_VF_SYSFS_WR_FUNC(6);
HQMV2_VF_SYSFS_WR_FUNC(7);
HQMV2_VF_SYSFS_WR_FUNC(8);
HQMV2_VF_SYSFS_WR_FUNC(9);
HQMV2_VF_SYSFS_WR_FUNC(10);
HQMV2_VF_SYSFS_WR_FUNC(11);
HQMV2_VF_SYSFS_WR_FUNC(12);
HQMV2_VF_SYSFS_WR_FUNC(13);
HQMV2_VF_SYSFS_WR_FUNC(14);
HQMV2_VF_SYSFS_WR_FUNC(15);

HQMV2_AUX_VF_ID_RD_FUNC(0);
HQMV2_AUX_VF_ID_RD_FUNC(1);
HQMV2_AUX_VF_ID_RD_FUNC(2);
HQMV2_AUX_VF_ID_RD_FUNC(3);
HQMV2_AUX_VF_ID_RD_FUNC(4);
HQMV2_AUX_VF_ID_RD_FUNC(5);
HQMV2_AUX_VF_ID_RD_FUNC(6);
HQMV2_AUX_VF_ID_RD_FUNC(7);
HQMV2_AUX_VF_ID_RD_FUNC(8);
HQMV2_AUX_VF_ID_RD_FUNC(9);
HQMV2_AUX_VF_ID_RD_FUNC(10);
HQMV2_AUX_VF_ID_RD_FUNC(11);
HQMV2_AUX_VF_ID_RD_FUNC(12);
HQMV2_AUX_VF_ID_RD_FUNC(13);
HQMV2_AUX_VF_ID_RD_FUNC(14);
HQMV2_AUX_VF_ID_RD_FUNC(15);

HQMV2_AUX_VF_ID_WR_FUNC(0);
HQMV2_AUX_VF_ID_WR_FUNC(1);
HQMV2_AUX_VF_ID_WR_FUNC(2);
HQMV2_AUX_VF_ID_WR_FUNC(3);
HQMV2_AUX_VF_ID_WR_FUNC(4);
HQMV2_AUX_VF_ID_WR_FUNC(5);
HQMV2_AUX_VF_ID_WR_FUNC(6);
HQMV2_AUX_VF_ID_WR_FUNC(7);
HQMV2_AUX_VF_ID_WR_FUNC(8);
HQMV2_AUX_VF_ID_WR_FUNC(9);
HQMV2_AUX_VF_ID_WR_FUNC(10);
HQMV2_AUX_VF_ID_WR_FUNC(11);
HQMV2_AUX_VF_ID_WR_FUNC(12);
HQMV2_AUX_VF_ID_WR_FUNC(13);
HQMV2_AUX_VF_ID_WR_FUNC(14);
HQMV2_AUX_VF_ID_WR_FUNC(15);

HQMV2_VF_SYSFS_ATTR_GROUP(0);
HQMV2_VF_SYSFS_ATTR_GROUP(1);
HQMV2_VF_SYSFS_ATTR_GROUP(2);
HQMV2_VF_SYSFS_ATTR_GROUP(3);
HQMV2_VF_SYSFS_ATTR_GROUP(4);
HQMV2_VF_SYSFS_ATTR_GROUP(5);
HQMV2_VF_SYSFS_ATTR_GROUP(6);
HQMV2_VF_SYSFS_ATTR_GROUP(7);
HQMV2_VF_SYSFS_ATTR_GROUP(8);
HQMV2_VF_SYSFS_ATTR_GROUP(9);
HQMV2_VF_SYSFS_ATTR_GROUP(10);
HQMV2_VF_SYSFS_ATTR_GROUP(11);
HQMV2_VF_SYSFS_ATTR_GROUP(12);
HQMV2_VF_SYSFS_ATTR_GROUP(13);
HQMV2_VF_SYSFS_ATTR_GROUP(14);
HQMV2_VF_SYSFS_ATTR_GROUP(15);

const struct attribute_group *hqmv2_vf_attrs[] = {
	&hqmv2_vf0_attr_group,
	&hqmv2_vf1_attr_group,
	&hqmv2_vf2_attr_group,
	&hqmv2_vf3_attr_group,
	&hqmv2_vf4_attr_group,
	&hqmv2_vf5_attr_group,
	&hqmv2_vf6_attr_group,
	&hqmv2_vf7_attr_group,
	&hqmv2_vf8_attr_group,
	&hqmv2_vf9_attr_group,
	&hqmv2_vf10_attr_group,
	&hqmv2_vf11_attr_group,
	&hqmv2_vf12_attr_group,
	&hqmv2_vf13_attr_group,
	&hqmv2_vf14_attr_group,
	&hqmv2_vf15_attr_group,
};

#define HQMV2_TOTAL_SYSFS_SHOW(name, macro)		\
static ssize_t total_##name##_show(			\
	struct device *dev,				\
	struct device_attribute *attr,			\
	char *buf)					\
{							\
	int val = HQMV2_MAX_NUM_##macro;		\
							\
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);	\
}

HQMV2_TOTAL_SYSFS_SHOW(num_sched_domains, DOMAINS)
HQMV2_TOTAL_SYSFS_SHOW(num_ldb_queues, LDB_QUEUES)
HQMV2_TOTAL_SYSFS_SHOW(num_ldb_ports, LDB_PORTS)
HQMV2_TOTAL_SYSFS_SHOW(num_cos0_ldb_ports, LDB_PORTS / HQMV2_NUM_COS_DOMAINS)
HQMV2_TOTAL_SYSFS_SHOW(num_cos1_ldb_ports, LDB_PORTS / HQMV2_NUM_COS_DOMAINS)
HQMV2_TOTAL_SYSFS_SHOW(num_cos2_ldb_ports, LDB_PORTS / HQMV2_NUM_COS_DOMAINS)
HQMV2_TOTAL_SYSFS_SHOW(num_cos3_ldb_ports, LDB_PORTS / HQMV2_NUM_COS_DOMAINS)
HQMV2_TOTAL_SYSFS_SHOW(num_dir_ports, DIR_PORTS)
HQMV2_TOTAL_SYSFS_SHOW(num_ldb_credits, LDB_CREDITS)
HQMV2_TOTAL_SYSFS_SHOW(num_dir_credits, DIR_CREDITS)
HQMV2_TOTAL_SYSFS_SHOW(num_atomic_inflights, AQED_ENTRIES)
HQMV2_TOTAL_SYSFS_SHOW(num_hist_list_entries, HIST_LIST_ENTRIES)

#define HQMV2_AVAIL_SYSFS_SHOW(name)			      \
static ssize_t avail_##name##_show(			      \
	struct device *dev,				      \
	struct device_attribute *attr,			      \
	char *buf)					      \
{							      \
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);   \
	struct hqmv2_get_num_resources_args arg;	      \
	struct hqmv2_hw *hw = &hqmv2_dev->hw;		      \
	int val;					      \
							      \
	mutex_lock(&hqmv2_dev->resource_mutex);		      \
							      \
	val = hqmv2_hw_get_num_resources(hw, &arg, false, 0); \
							      \
	mutex_unlock(&hqmv2_dev->resource_mutex);	      \
							      \
	if (val)					      \
		return -1;				      \
							      \
	val = arg.name;					      \
							      \
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);	      \
}

HQMV2_AVAIL_SYSFS_SHOW(num_sched_domains)
HQMV2_AVAIL_SYSFS_SHOW(num_ldb_queues)
HQMV2_AVAIL_SYSFS_SHOW(num_ldb_ports)
HQMV2_AVAIL_SYSFS_SHOW(num_cos0_ldb_ports)
HQMV2_AVAIL_SYSFS_SHOW(num_cos1_ldb_ports)
HQMV2_AVAIL_SYSFS_SHOW(num_cos2_ldb_ports)
HQMV2_AVAIL_SYSFS_SHOW(num_cos3_ldb_ports)
HQMV2_AVAIL_SYSFS_SHOW(num_dir_ports)
HQMV2_AVAIL_SYSFS_SHOW(num_ldb_credits)
HQMV2_AVAIL_SYSFS_SHOW(num_dir_credits)
HQMV2_AVAIL_SYSFS_SHOW(num_atomic_inflights)
HQMV2_AVAIL_SYSFS_SHOW(num_hist_list_entries)

static ssize_t max_ctg_hl_entries_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf)
{
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);
	struct hqmv2_get_num_resources_args arg;
	struct hqmv2_hw *hw = &hqmv2_dev->hw;
	int val;

	mutex_lock(&hqmv2_dev->resource_mutex);

	val = hqmv2_hw_get_num_resources(hw, &arg, false, 0);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	if (val)
		return -1;

	val = arg.max_contiguous_hist_list_entries;

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

const struct attribute_group hqmv2_total_attr_group = {
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

const struct attribute_group hqmv2_avail_attr_group = {
	.attrs = hqmv2_avail_attrs,
	.name = "avail_resources",
};

#define HQMV2_GROUP_SNS_PER_QUEUE_SHOW(id)		\
static ssize_t group##id##_sns_per_queue_show(		\
	struct device *dev,				\
	struct device_attribute *attr,			\
	char *buf)					\
{							\
	struct hqmv2_dev *hqmv2_dev =			\
		dev_get_drvdata(dev);			\
	struct hqmv2_hw *hw = &hqmv2_dev->hw;		\
	int val;					\
							\
	mutex_lock(&hqmv2_dev->resource_mutex);		\
							\
	val = hqmv2_get_group_sequence_numbers(hw, id);	\
							\
	mutex_unlock(&hqmv2_dev->resource_mutex);	\
							\
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);	\
}

HQMV2_GROUP_SNS_PER_QUEUE_SHOW(0)
HQMV2_GROUP_SNS_PER_QUEUE_SHOW(1)

#define HQMV2_GROUP_SNS_PER_QUEUE_STORE(id)			\
static ssize_t group##id##_sns_per_queue_store(			\
	struct device *dev,					\
	struct device_attribute *attr,				\
	const char *buf,					\
	size_t count)						\
{								\
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);	\
	struct hqmv2_hw *hw = &hqmv2_dev->hw;			\
	unsigned long val;					\
	int err;						\
								\
	err = kstrtoul(buf, 0, &val);				\
	if (err)						\
		return -1;					\
								\
	mutex_lock(&hqmv2_dev->resource_mutex);			\
								\
	err = hqmv2_set_group_sequence_numbers(hw, id, val);	\
								\
	mutex_unlock(&hqmv2_dev->resource_mutex);		\
								\
	return err ? err : count;				\
}

HQMV2_GROUP_SNS_PER_QUEUE_STORE(0)
HQMV2_GROUP_SNS_PER_QUEUE_STORE(1)

/* RW sysfs files in the sequence_numbers/ subdirectory */
static DEVICE_ATTR_RW(group0_sns_per_queue);
static DEVICE_ATTR_RW(group1_sns_per_queue);

static struct attribute *hqmv2_sequence_number_attrs[] = {
	&dev_attr_group0_sns_per_queue.attr,
	&dev_attr_group1_sns_per_queue.attr,
	NULL
};

const struct attribute_group hqmv2_sequence_number_attr_group = {
	.attrs = hqmv2_sequence_number_attrs,
	.name = "sequence_numbers"
};

#define HQMV2_COS_BW_PERCENT_SHOW(id)			\
static ssize_t cos##id##_bw_percent_show(		\
	struct device *dev,				\
	struct device_attribute *attr,			\
	char *buf)					\
{							\
	struct hqmv2_dev *hqmv2_dev =			\
		dev_get_drvdata(dev);			\
	struct hqmv2_hw *hw = &hqmv2_dev->hw;		\
	int val;					\
							\
	mutex_lock(&hqmv2_dev->resource_mutex);		\
							\
	val = hqmv2_hw_get_cos_bandwidth(hw, id);	\
							\
	mutex_unlock(&hqmv2_dev->resource_mutex);	\
							\
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);	\
}

HQMV2_COS_BW_PERCENT_SHOW(0)
HQMV2_COS_BW_PERCENT_SHOW(1)
HQMV2_COS_BW_PERCENT_SHOW(2)
HQMV2_COS_BW_PERCENT_SHOW(3)

#define HQMV2_COS_BW_PERCENT_STORE(id)			\
static ssize_t cos##id##_bw_percent_store(		\
	struct device *dev,				\
	struct device_attribute *attr,			\
	const char *buf,				\
	size_t count)					\
{							\
	struct hqmv2_dev *hqmv2_dev =			\
		dev_get_drvdata(dev);			\
	struct hqmv2_hw *hw = &hqmv2_dev->hw;		\
	unsigned long val;				\
	int err;					\
							\
	err = kstrtoul(buf, 0, &val);			\
	if (err)					\
		return -1;				\
							\
	mutex_lock(&hqmv2_dev->resource_mutex);		\
							\
	err = hqmv2_hw_set_cos_bandwidth(hw, id, val);	\
							\
	mutex_unlock(&hqmv2_dev->resource_mutex);	\
							\
	return err ? err : count;			\
}

HQMV2_COS_BW_PERCENT_STORE(0)
HQMV2_COS_BW_PERCENT_STORE(1)
HQMV2_COS_BW_PERCENT_STORE(2)
HQMV2_COS_BW_PERCENT_STORE(3)

/* RW sysfs files in the sequence_numbers/ subdirectory */
static DEVICE_ATTR_RW(cos0_bw_percent);
static DEVICE_ATTR_RW(cos1_bw_percent);
static DEVICE_ATTR_RW(cos2_bw_percent);
static DEVICE_ATTR_RW(cos3_bw_percent);

static struct attribute *hqmv2_cos_bw_percent_attrs[] = {
	&dev_attr_cos0_bw_percent.attr,
	&dev_attr_cos1_bw_percent.attr,
	&dev_attr_cos2_bw_percent.attr,
	&dev_attr_cos3_bw_percent.attr,
	NULL
};

const struct attribute_group hqmv2_cos_bw_percent_attr_group = {
	.attrs = hqmv2_cos_bw_percent_attrs,
	.name = "cos_bw"
};

static ssize_t dev_id_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct hqmv2_dev *hqmv2_dev = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%d\n", hqmv2_dev->id);
}

static DEVICE_ATTR_RO(dev_id);

int hqmv2_pf_sysfs_create(struct hqmv2_dev *hqmv2_dev)
{
	struct kobject *kobj;
	int ret;
	int i;

	kobj = &hqmv2_dev->pdev->dev.kobj;

	ret = sysfs_create_file(kobj, &dev_attr_dev_id.attr);
	if (ret)
		goto hqm_dev_id_attr_group_fail;

	ret = sysfs_create_group(kobj, &hqmv2_total_attr_group);
	if (ret)
		goto hqmv2_total_attr_group_fail;

	ret = sysfs_create_group(kobj, &hqmv2_avail_attr_group);
	if (ret)
		goto hqmv2_avail_attr_group_fail;

	ret = sysfs_create_group(kobj, &hqmv2_sequence_number_attr_group);
	if (ret)
		goto hqmv2_sn_attr_group_fail;

	ret = sysfs_create_group(kobj, &hqmv2_cos_bw_percent_attr_group);
	if (ret)
		goto hqmv2_cos_bw_percent_attr_group_fail;

	for (i = 0; i < pci_num_vf(hqmv2_dev->pdev); i++) {
		ret = sysfs_create_group(kobj, hqmv2_vf_attrs[i]);
		if (ret)
			goto hqmv2_vf_attr_group_fail;
	}

	return 0;

hqmv2_vf_attr_group_fail:
	for (i--; i >= 0; i--)
		sysfs_remove_group(kobj, hqmv2_vf_attrs[i]);

	sysfs_remove_group(kobj, &hqmv2_cos_bw_percent_attr_group);
hqmv2_cos_bw_percent_attr_group_fail:
	sysfs_remove_group(kobj, &hqmv2_sequence_number_attr_group);
hqmv2_sn_attr_group_fail:
	sysfs_remove_group(kobj, &hqmv2_avail_attr_group);
hqmv2_avail_attr_group_fail:
	sysfs_remove_group(kobj, &hqmv2_total_attr_group);
hqmv2_total_attr_group_fail:
	sysfs_remove_file(kobj, &dev_attr_dev_id.attr);
hqm_dev_id_attr_group_fail:
	return ret;
}

void hqmv2_pf_sysfs_destroy(struct hqmv2_dev *hqmv2_dev)
{
	struct kobject *kobj;
	int i;

	kobj = &hqmv2_dev->pdev->dev.kobj;

	for (i = 0; i < pci_num_vf(hqmv2_dev->pdev); i++)
		sysfs_remove_group(kobj, hqmv2_vf_attrs[i]);

	sysfs_remove_group(kobj, &hqmv2_cos_bw_percent_attr_group);
	sysfs_remove_group(kobj, &hqmv2_sequence_number_attr_group);
	sysfs_remove_group(kobj, &hqmv2_avail_attr_group);
	sysfs_remove_group(kobj, &hqmv2_total_attr_group);
	sysfs_remove_file(kobj, &dev_attr_dev_id.attr);
}

void hqmv2_pf_sysfs_reapply_configuration(struct hqmv2_dev *dev)
{
	int i;

	for (i = 0; i < HQMV2_MAX_NUM_SEQUENCE_NUMBER_GROUPS; i++) {
		int num_sns = hqmv2_get_group_sequence_numbers(&dev->hw, i);

		hqmv2_set_group_sequence_numbers(&dev->hw, i, num_sns);
	}

	for (i = 0; i < HQMV2_NUM_COS_DOMAINS; i++) {
		int bw = hqmv2_hw_get_cos_bandwidth(&dev->hw, i);

		hqmv2_hw_set_cos_bandwidth(&dev->hw, i, bw);
	}
}

/*****************************/
/****** IOCTL callbacks ******/
/*****************************/

int hqmv2_pf_create_sched_domain(struct hqmv2_hw *hw,
				 struct hqmv2_create_sched_domain_args *args,
				 struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_create_sched_domain(hw, args, resp, false, 0);
}

int hqmv2_pf_create_ldb_queue(struct hqmv2_hw *hw,
			      u32 id,
			      struct hqmv2_create_ldb_queue_args *args,
			      struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_create_ldb_queue(hw, id, args, resp, false, 0);
}

int hqmv2_pf_create_dir_queue(struct hqmv2_hw *hw,
			      u32 id,
			      struct hqmv2_create_dir_queue_args *args,
			      struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_create_dir_queue(hw, id, args, resp, false, 0);
}

int hqmv2_pf_create_ldb_port(struct hqmv2_hw *hw,
			     u32 id,
			     struct hqmv2_create_ldb_port_args *args,
			     uintptr_t pop_count_dma_base,
			     uintptr_t cq_dma_base,
			     struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_create_ldb_port(hw, id, args,
				      pop_count_dma_base,
				      cq_dma_base,
				      resp, false, 0);
}

int hqmv2_pf_create_dir_port(struct hqmv2_hw *hw,
			     u32 id,
			     struct hqmv2_create_dir_port_args *args,
			     uintptr_t pop_count_dma_base,
			     uintptr_t cq_dma_base,
			     struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_create_dir_port(hw, id, args,
				      pop_count_dma_base,
				      cq_dma_base,
				      resp, false, 0);
}

int hqmv2_pf_start_domain(struct hqmv2_hw *hw,
			  u32 id,
			  struct hqmv2_start_domain_args *args,
			  struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_start_domain(hw, id, args, resp, false, 0);
}

int hqmv2_pf_map_qid(struct hqmv2_hw *hw,
		     u32 id,
		     struct hqmv2_map_qid_args *args,
		     struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_map_qid(hw, id, args, resp, false, 0);
}

int hqmv2_pf_unmap_qid(struct hqmv2_hw *hw,
		       u32 id,
		       struct hqmv2_unmap_qid_args *args,
		       struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_unmap_qid(hw, id, args, resp, false, 0);
}

int hqmv2_pf_enable_ldb_port(struct hqmv2_hw *hw,
			     u32 id,
			     struct hqmv2_enable_ldb_port_args *args,
			     struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_enable_ldb_port(hw, id, args, resp, false, 0);
}

int hqmv2_pf_disable_ldb_port(struct hqmv2_hw *hw,
			      u32 id,
			      struct hqmv2_disable_ldb_port_args *args,
			      struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_disable_ldb_port(hw, id, args, resp, false, 0);
}

int hqmv2_pf_enable_dir_port(struct hqmv2_hw *hw,
			     u32 id,
			     struct hqmv2_enable_dir_port_args *args,
			     struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_enable_dir_port(hw, id, args, resp, false, 0);
}

int hqmv2_pf_disable_dir_port(struct hqmv2_hw *hw,
			      u32 id,
			      struct hqmv2_disable_dir_port_args *args,
			      struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_disable_dir_port(hw, id, args, resp, false, 0);
}

int hqmv2_pf_get_num_resources(struct hqmv2_hw *hw,
			       struct hqmv2_get_num_resources_args *args)
{
	return hqmv2_hw_get_num_resources(hw, args, false, 0);
}

int hqmv2_pf_reset_domain(struct hqmv2_hw *hw, u32 id)
{
	return hqmv2_reset_domain(hw, id, false, 0);
}

int hqmv2_pf_measure_perf(struct hqmv2_dev *dev,
			  struct hqmv2_sample_perf_counters_args *args,
			  struct hqmv2_cmd_response *response)
{
	struct hqmv2_perf_metric_pre_data *pre = NULL;
	union hqmv2_perf_metric_group_data data;
	struct timespec64 start, end;
	u32 elapsed;
	int ret = 0;

	memset(&data, 0, sizeof(data));

	pre = devm_kmalloc(&dev->pdev->dev, sizeof(*pre), GFP_KERNEL);
	if (!pre) {
		ret = -ENOMEM;
		goto done;
	}

	/* Only one active measurement is allowed at a time. */
	if (!mutex_trylock(&dev->perf.measurement_mutex)) {
		ret = -EBUSY;
		devm_kfree(&dev->pdev->dev, pre);
		goto done;
	}

	/* See hqmv2_reset_notify() for more details */
	if (READ_ONCE(dev->unexpected_reset)) {
		HQMV2_ERR(dev->hqmv2_device,
			  "[%s()] The HQM was unexpectedly reset. To recover, all HQM applications must close.\n",
			  __func__);
		ret = -EINVAL;
		devm_kfree(&dev->pdev->dev, pre);
		mutex_unlock(&dev->perf.measurement_mutex);
		goto done;
	}

	ktime_get_real_ts64(&start);

	hqmv2_init_perf_metric_measurement(&dev->hw,
					   args->perf_metric_group_id,
					   args->measurement_duration_us,
					   pre,
					   false,
					   0);

	set_current_state(TASK_INTERRUPTIBLE);

	schedule_timeout(usecs_to_jiffies(args->measurement_duration_us));

	ktime_get_real_ts64(&end);

	hqmv2_collect_perf_metric_data(&dev->hw,
				       args->perf_metric_group_id,
				       args->measurement_duration_us,
				       pre,
				       &data,
				       false,
				       0);

	mutex_unlock(&dev->perf.measurement_mutex);

	/* Calculate the elapsed time in microseconds */
	elapsed = (end.tv_sec - start.tv_sec) * 1000000 +
		  (end.tv_nsec - start.tv_nsec) / 1000;

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

int hqmv2_pf_get_ldb_queue_depth(struct hqmv2_hw *hw,
				 u32 id,
				 struct hqmv2_get_ldb_queue_depth_args *args,
				 struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_get_ldb_queue_depth(hw, id, args, resp, false, 0);
}

int hqmv2_pf_get_dir_queue_depth(struct hqmv2_hw *hw,
				 u32 id,
				 struct hqmv2_get_dir_queue_depth_args *args,
				 struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_get_dir_queue_depth(hw, id, args, resp, false, 0);
}

int hqmv2_pf_pending_port_unmaps(struct hqmv2_hw *hw,
				 u32 id,
				 struct hqmv2_pending_port_unmaps_args *args,
				 struct hqmv2_cmd_response *resp)
{
	return hqmv2_hw_pending_port_unmaps(hw, id, args, resp, false, 0);
}

/**************************************/
/****** Resource query callbacks ******/
/**************************************/

int hqmv2_pf_ldb_port_owned_by_domain(struct hqmv2_hw *hw,
				      u32 domain_id,
				      u32 port_id)
{
	return hqmv2_ldb_port_owned_by_domain(hw, domain_id, port_id, false, 0);
}

int hqmv2_pf_dir_port_owned_by_domain(struct hqmv2_hw *hw,
				      u32 domain_id,
				      u32 port_id)
{
	return hqmv2_dir_port_owned_by_domain(hw, domain_id, port_id, false, 0);
}

int hqmv2_pf_get_sn_allocation(struct hqmv2_hw *hw, u32 group_id)
{
	return hqmv2_get_group_sequence_numbers(hw, group_id);
}

int hqmv2_pf_get_cos_bw(struct hqmv2_hw *hw, u32 cos_id)
{
	return hqmv2_hw_get_cos_bandwidth(hw, cos_id);
}

/*********************************/
/****** HQMv2 PF Device Ops ******/
/*********************************/

struct hqmv2_device_ops hqmv2_pf_ops = {
	.map_pci_bar_space = hqmv2_pf_map_pci_bar_space,
	.unmap_pci_bar_space = hqmv2_pf_unmap_pci_bar_space,
	.mmap = hqmv2_pf_mmap,
	.inc_pm_refcnt = hqmv2_pf_pm_inc_refcnt,
	.dec_pm_refcnt = hqmv2_pf_pm_dec_refcnt,
	.pm_refcnt_status_suspended = hqmv2_pf_pm_status_suspended,
	.init_driver_state = hqmv2_pf_init_driver_state,
	.free_driver_state = hqmv2_pf_free_driver_state,
	.device_create = hqmv2_pf_device_create,
	.device_destroy = hqmv2_pf_device_destroy,
	.cdev_add = hqmv2_pf_cdev_add,
	.cdev_del = hqmv2_pf_cdev_del,
	.sysfs_create = hqmv2_pf_sysfs_create,
	.sysfs_destroy = hqmv2_pf_sysfs_destroy,
	.sysfs_reapply = hqmv2_pf_sysfs_reapply_configuration,
	.init_interrupts = hqmv2_pf_init_interrupts,
	.enable_ldb_cq_interrupts = hqmv2_pf_enable_ldb_cq_interrupts,
	.enable_dir_cq_interrupts = hqmv2_pf_enable_dir_cq_interrupts,
	.arm_cq_interrupt = hqmv2_pf_arm_cq_interrupt,
	.reinit_interrupts = hqmv2_pf_reinit_interrupts,
	.free_interrupts = hqmv2_pf_free_interrupts,
	.enable_pm = hqmv2_pf_enable_pm,
	.wait_for_device_ready = hqmv2_pf_wait_for_device_ready,
	.reset_device = hqmv2_pf_reset,
	.register_driver = hqmv2_pf_register_driver,
	.unregister_driver = hqmv2_pf_unregister_driver,
	.create_sched_domain = hqmv2_pf_create_sched_domain,
	.create_ldb_queue = hqmv2_pf_create_ldb_queue,
	.create_dir_queue = hqmv2_pf_create_dir_queue,
	.create_ldb_port = hqmv2_pf_create_ldb_port,
	.create_dir_port = hqmv2_pf_create_dir_port,
	.start_domain = hqmv2_pf_start_domain,
	.map_qid = hqmv2_pf_map_qid,
	.unmap_qid = hqmv2_pf_unmap_qid,
	.enable_ldb_port = hqmv2_pf_enable_ldb_port,
	.enable_dir_port = hqmv2_pf_enable_dir_port,
	.disable_ldb_port = hqmv2_pf_disable_ldb_port,
	.disable_dir_port = hqmv2_pf_disable_dir_port,
	.get_num_resources = hqmv2_pf_get_num_resources,
	.reset_domain = hqmv2_pf_reset_domain,
	.measure_perf = hqmv2_pf_measure_perf,
	.ldb_port_owned_by_domain = hqmv2_pf_ldb_port_owned_by_domain,
	.dir_port_owned_by_domain = hqmv2_pf_dir_port_owned_by_domain,
	.get_sn_allocation = hqmv2_pf_get_sn_allocation,
	.get_ldb_queue_depth = hqmv2_pf_get_ldb_queue_depth,
	.get_dir_queue_depth = hqmv2_pf_get_dir_queue_depth,
	.pending_port_unmaps = hqmv2_pf_pending_port_unmaps,
	.get_cos_bw = hqmv2_pf_get_cos_bw,
};
