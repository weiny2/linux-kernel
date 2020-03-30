// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
/* Copyright(c) 2017-2020 Intel Corporation */

#include <linux/delay.h>

#include "dlb_main.h"
#include "dlb_mbox.h"
#include "dlb_resource.h"

/***********************************/
/****** Mailbox communication ******/
/***********************************/

static int dlb_send_sync_mbox_cmd(struct dlb_dev *dlb_dev,
				  void *data,
				  int len,
				  int timeout_s)
{
	int retry_cnt, ret;

	if (len > DLB_FUNC_VF_VF2PF_MAILBOX_BYTES) {
		dev_err(dlb_dev->dlb_device,
			"Internal error: VF mbox message too large\n");
			return -1;
	}

	ret = dlb_vf_write_pf_mbox_req(&dlb_dev->hw, data, len);
	if (ret)
		return ret;

	dlb_send_async_vf_to_pf_msg(&dlb_dev->hw);

	/* Timeout after timeout_s seconds of inactivity */
	retry_cnt = 1000 * timeout_s;
	do {
		if (dlb_vf_to_pf_complete(&dlb_dev->hw))
			break;
		usleep_range(1000, 1001);
	} while (--retry_cnt);

	if (!retry_cnt) {
		dev_err(dlb_dev->dlb_device,
			"VF driver timed out waiting for mbox response\n");
		return -1;
	}

	return 0;
}

/***********************************/
/****** Runtime PM management ******/
/***********************************/

static void dlb_vf_pm_inc_refcnt(struct pci_dev *pdev, bool resume)
{
}

static void dlb_vf_pm_dec_refcnt(struct pci_dev *pdev)
{
}

/********************************/
/****** PCI BAR management ******/
/********************************/

static void dlb_vf_unmap_pci_bar_space(struct dlb_dev *dlb_dev,
				       struct pci_dev *pdev)
{
	pci_iounmap(pdev, dlb_dev->hw.func_kva);
}

static int dlb_vf_map_pci_bar_space(struct dlb_dev *dlb_dev,
				    struct pci_dev *pdev)
{
	/* BAR 0: VF FUNC BAR space */
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

	return 0;
}

#define DLB_LDB_CQ_BOUND DLB_LDB_CQ_OFFS(DLB_MAX_NUM_LDB_PORTS)
#define DLB_DIR_CQ_BOUND DLB_DIR_CQ_OFFS(DLB_MAX_NUM_DIR_PORTS)
#define DLB_LDB_PC_BOUND DLB_LDB_PC_OFFS(DLB_MAX_NUM_LDB_PORTS)
#define DLB_DIR_PC_BOUND DLB_DIR_PC_OFFS(DLB_MAX_NUM_DIR_PORTS)

static int dlb_vf_mmap(struct file *f,
		       struct vm_area_struct *vma,
		       u32 domain_id)
{
	unsigned long offset;
	unsigned long pgoff;
	struct dlb_dev *dev;
	struct page *page;
	pgprot_t pgprot;
	u32 port_id;

	dev = container_of(f->f_inode->i_cdev, struct dlb_dev, cdev);

	offset = vma->vm_pgoff << PAGE_SHIFT;

	/* The VF's ops callbacks go through the mailbox, so the caller must
	 * hold the resource_mutex.
	 */

	if (offset >= DLB_LDB_CQ_BASE && offset < DLB_LDB_CQ_BOUND) {
		if ((vma->vm_end - vma->vm_start) != DLB_LDB_CQ_MAX_SIZE)
			return -EINVAL;

		pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

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

		pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

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

		pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

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

		pgoff = dev->hw.func_phys_addr >> PAGE_SHIFT;

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

		pgoff = (dev->hw.func_phys_addr >> PAGE_SHIFT) + vma->vm_pgoff;

		port_id = (offset - DLB_LDB_PP_BASE) / DLB_LDB_PP_MAX_SIZE;

		if (dev->ops->ldb_port_owned_by_domain(&dev->hw,
						       domain_id,
						       port_id) != 1)
			return -EINVAL;

		pgprot = pgprot_noncached(vma->vm_page_prot);

	} else if (offset >= DLB_DIR_PP_BASE && offset < DLB_DIR_PP_BOUND) {
		if ((vma->vm_end - vma->vm_start) != DLB_DIR_PP_MAX_SIZE)
			return -EINVAL;

		pgoff = (dev->hw.func_phys_addr >> PAGE_SHIFT) + vma->vm_pgoff;

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
				  pgoff,
				  vma->vm_end - vma->vm_start,
				  pgprot);
}

/*******************************/
/****** Driver management ******/
/*******************************/

static int dlb_vf_init_driver_state(struct dlb_dev *dlb_dev)
{
	int i;

	/* Initialize software state */
	for (i = 0; i < DLB_MAX_NUM_DOMAINS; i++) {
		struct dlb_domain_dev *domain = &dlb_dev->sched_domains[i];

		mutex_init(&domain->alert_mutex);
		init_waitqueue_head(&domain->wq_head);
	}

	init_waitqueue_head(&dlb_dev->measurement_wq);

	mutex_init(&dlb_dev->resource_mutex);
	mutex_init(&dlb_dev->measurement_mutex);

	return 0;
}

static void dlb_vf_free_driver_state(struct dlb_dev *dlb_dev)
{
}

static int dlb_vf_cdev_add(struct dlb_dev *dlb_dev,
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

static void dlb_vf_cdev_del(struct dlb_dev *dlb_dev)
{
	cdev_del(&dlb_dev->cdev);
}

static int dlb_vf_device_create(struct dlb_dev *dlb_dev,
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

static void dlb_vf_device_destroy(struct dlb_dev *dlb_dev,
				  struct class *dlb_class)
{
	device_destroy(dlb_class,
		       MKDEV(MAJOR(dlb_dev->dev_number),
			     MINOR(dlb_dev->dev_number) +
				DLB_MAX_NUM_DOMAINS));
}


static void dlb_vf_init_hardware(struct dlb_dev *dlb_dev)
{
	/* Function intentionally left blank */
}

/*****************************/
/****** IOCTL callbacks ******/
/*****************************/

static int dlb_vf_create_sched_domain(struct dlb_hw *hw,
				      struct dlb_create_sched_domain_args *args,
				      struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_create_sched_domain_cmd_resp resp;
	struct dlb_mbox_create_sched_domain_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_CREATE_SCHED_DOMAIN;
	req.num_ldb_queues = args->num_ldb_queues;
	req.num_ldb_ports = args->num_ldb_ports;
	req.num_dir_ports = args->num_dir_ports;
	req.num_atomic_inflights = args->num_atomic_inflights;
	req.num_hist_list_entries = args->num_hist_list_entries;
	req.num_ldb_credits = args->num_ldb_credits;
	req.num_dir_credits = args->num_dir_credits;
	req.num_ldb_credit_pools = args->num_ldb_credit_pools;
	req.num_dir_credit_pools = args->num_dir_credit_pools;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int dlb_vf_create_ldb_pool(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_create_ldb_pool_args *args,
				  struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_create_credit_pool_cmd_resp resp;
	struct dlb_mbox_create_credit_pool_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_CREATE_LDB_POOL;
	req.domain_id = id;
	req.num_credits = args->num_ldb_credits;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int dlb_vf_create_dir_pool(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_create_dir_pool_args *args,
				  struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_create_credit_pool_cmd_resp resp;
	struct dlb_mbox_create_credit_pool_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_CREATE_DIR_POOL;
	req.domain_id = id;
	req.num_credits = args->num_dir_credits;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int dlb_vf_create_ldb_queue(struct dlb_hw *hw,
				   u32 id,
				   struct dlb_create_ldb_queue_args *args,
				   struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_create_ldb_queue_cmd_resp resp;
	struct dlb_mbox_create_ldb_queue_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_CREATE_LDB_QUEUE;
	req.domain_id = id;
	req.num_sequence_numbers = args->num_sequence_numbers;
	req.num_qid_inflights = args->num_qid_inflights;
	req.num_atomic_inflights = args->num_atomic_inflights;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int dlb_vf_create_dir_queue(struct dlb_hw *hw,
				   u32 id,
				   struct dlb_create_dir_queue_args *args,
				   struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_create_dir_queue_cmd_resp resp;
	struct dlb_mbox_create_dir_queue_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_CREATE_DIR_QUEUE;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int dlb_vf_create_ldb_port(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_create_ldb_port_args *args,
				  uintptr_t pop_count_dma_base,
				  uintptr_t cq_dma_base,
				  struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_create_ldb_port_cmd_resp resp;
	struct dlb_mbox_create_ldb_port_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_CREATE_LDB_PORT;
	req.domain_id = id;
	req.ldb_credit_pool_id = args->ldb_credit_pool_id;
	req.dir_credit_pool_id = args->dir_credit_pool_id;
	req.pop_count_address = pop_count_dma_base;
	req.ldb_credit_high_watermark = args->ldb_credit_high_watermark;
	req.ldb_credit_low_watermark = args->ldb_credit_low_watermark;
	req.ldb_credit_quantum = args->ldb_credit_quantum;
	req.dir_credit_high_watermark = args->dir_credit_high_watermark;
	req.dir_credit_low_watermark = args->dir_credit_low_watermark;
	req.dir_credit_quantum = args->dir_credit_quantum;
	req.cq_depth = args->cq_depth;
	req.cq_history_list_size = args->cq_history_list_size;
	req.cq_base_address = cq_dma_base;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int dlb_vf_create_dir_port(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_create_dir_port_args *args,
				  uintptr_t pop_count_dma_base,
				  uintptr_t cq_dma_base,
				  struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_create_dir_port_cmd_resp resp;
	struct dlb_mbox_create_dir_port_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_CREATE_DIR_PORT;
	req.domain_id = id;
	req.ldb_credit_pool_id = args->ldb_credit_pool_id;
	req.dir_credit_pool_id = args->dir_credit_pool_id;
	req.pop_count_address = pop_count_dma_base;
	req.ldb_credit_high_watermark = args->ldb_credit_high_watermark;
	req.ldb_credit_low_watermark = args->ldb_credit_low_watermark;
	req.ldb_credit_quantum = args->ldb_credit_quantum;
	req.dir_credit_high_watermark = args->dir_credit_high_watermark;
	req.dir_credit_low_watermark = args->dir_credit_low_watermark;
	req.dir_credit_quantum = args->dir_credit_quantum;
	req.cq_depth = args->cq_depth;
	req.cq_base_address = cq_dma_base;
	req.queue_id = args->queue_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.id;

	return resp.error_code;
}

static int dlb_vf_start_domain(struct dlb_hw *hw,
			       u32 id,
			       struct dlb_start_domain_args *args,
			       struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_start_domain_cmd_resp resp;
	struct dlb_mbox_start_domain_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_START_DOMAIN;
	req.domain_id = id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int dlb_vf_map_qid(struct dlb_hw *hw,
			  u32 id,
			  struct dlb_map_qid_args *args,
			  struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_map_qid_cmd_resp resp;
	struct dlb_mbox_map_qid_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_MAP_QID;
	req.domain_id = id;
	req.port_id = args->port_id;
	req.qid = args->qid;
	req.priority = args->priority;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int dlb_vf_unmap_qid(struct dlb_hw *hw,
			    u32 id,
			    struct dlb_unmap_qid_args *args,
			    struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_unmap_qid_cmd_resp resp;
	struct dlb_mbox_unmap_qid_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_UNMAP_QID;
	req.domain_id = id;
	req.port_id = args->port_id;
	req.qid = args->qid;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int dlb_vf_enable_ldb_port(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_enable_ldb_port_args *args,
				  struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_enable_ldb_port_cmd_resp resp;
	struct dlb_mbox_enable_ldb_port_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_ENABLE_LDB_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int dlb_vf_disable_ldb_port(struct dlb_hw *hw,
				   u32 id,
				   struct dlb_disable_ldb_port_args *args,
				   struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_disable_ldb_port_cmd_resp resp;
	struct dlb_mbox_disable_ldb_port_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_DISABLE_LDB_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int dlb_vf_enable_dir_port(struct dlb_hw *hw,
				  u32 id,
				  struct dlb_enable_dir_port_args *args,
				  struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_enable_dir_port_cmd_resp resp;
	struct dlb_mbox_enable_dir_port_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_ENABLE_DIR_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int dlb_vf_disable_dir_port(struct dlb_hw *hw,
				   u32 id,
				   struct dlb_disable_dir_port_args *args,
				   struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_disable_dir_port_cmd_resp resp;
	struct dlb_mbox_disable_dir_port_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_DISABLE_DIR_PORT;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;

	return resp.error_code;
}

static int dlb_vf_get_num_resources(struct dlb_hw *hw,
				    struct dlb_get_num_resources_args *args)
{
	struct dlb_mbox_get_num_resources_cmd_resp resp;
	struct dlb_mbox_get_num_resources_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_GET_NUM_RESOURCES;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		return -1;
	}

	args->num_sched_domains = resp.num_sched_domains;
	args->num_ldb_queues = resp.num_ldb_queues;
	args->num_ldb_ports = resp.num_ldb_ports;
	args->num_dir_ports = resp.num_dir_ports;
	args->num_atomic_inflights = resp.num_atomic_inflights;
	args->max_contiguous_atomic_inflights =
		resp.max_contiguous_atomic_inflights;
	args->num_hist_list_entries = resp.num_hist_list_entries;
	args->max_contiguous_hist_list_entries =
		resp.max_contiguous_hist_list_entries;
	args->num_ldb_credits = resp.num_ldb_credits;
	args->max_contiguous_ldb_credits = resp.max_contiguous_ldb_credits;
	args->num_dir_credits = resp.num_dir_credits;
	args->max_contiguous_dir_credits = resp.max_contiguous_dir_credits;
	args->num_ldb_credit_pools = resp.num_ldb_credit_pools;
	args->num_dir_credit_pools = resp.num_dir_credit_pools;

	return resp.error_code;
}

static int dlb_vf_reset_domain(struct dlb_dev *dlb_dev, u32 id)
{
	struct dlb_mbox_reset_sched_domain_cmd_resp resp;
	struct dlb_mbox_reset_sched_domain_cmd_req req;
	int ret;

	req.hdr.type = DLB_MBOX_CMD_RESET_SCHED_DOMAIN;
	req.id = id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.error_code;
}

#define DLB_VF_PERF_MEAS_TMO_S 60

static int dlb_vf_req_sched_count_measurement(struct dlb_dev *dlb_dev,
					      u32 duration)
{
	struct dlb_mbox_init_cq_sched_count_measure_cmd_resp resp;
	struct dlb_mbox_init_cq_sched_count_measure_cmd_req req;
	int ret;

	req.hdr.type = DLB_MBOX_CMD_INIT_CQ_SCHED_COUNT;
	req.duration_us = duration;

	mutex_lock(&dlb_dev->resource_mutex);

	ret = dlb_send_sync_mbox_cmd(dlb_dev,
				     &req,
				     sizeof(req),
				     DLB_VF_PERF_MEAS_TMO_S);
	if (ret) {
		mutex_unlock(&dlb_dev->resource_mutex);
		return ret;
	}

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	mutex_unlock(&dlb_dev->resource_mutex);

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.error_code;
}

static int dlb_vf_collect_cq_sched_count(struct dlb_dev *dlb_dev,
					 struct dlb_sched_counts *data,
					 int cq_id,
					 bool is_ldb,
					 u32 *elapsed)
{
	struct dlb_mbox_collect_cq_sched_count_cmd_resp resp;
	struct dlb_mbox_collect_cq_sched_count_cmd_req req;
	int ret;

	req.hdr.type = DLB_MBOX_CMD_COLLECT_CQ_SCHED_COUNT;
	req.cq_id = cq_id;
	req.is_ldb = is_ldb;

	mutex_lock(&dlb_dev->resource_mutex);

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret) {
		mutex_unlock(&dlb_dev->resource_mutex);
		return ret;
	}

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	mutex_unlock(&dlb_dev->resource_mutex);

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		return -1;
	}

	if (is_ldb)
		data->ldb_cq_sched_count[cq_id] = resp.cq_sched_count;
	else
		data->dir_cq_sched_count[cq_id] = resp.cq_sched_count;

	*elapsed = resp.elapsed;

	return resp.error_code;
}

static int dlb_vf_get_num_used_rsrcs(struct dlb_hw *hw,
				     struct dlb_get_num_resources_args *args)
{
	struct dlb_mbox_get_num_resources_cmd_resp resp;
	struct dlb_mbox_get_num_resources_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_GET_NUM_USED_RESOURCES;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		return -1;
	}

	args->num_sched_domains = resp.num_sched_domains;
	args->num_ldb_queues = resp.num_ldb_queues;
	args->num_ldb_ports = resp.num_ldb_ports;
	args->num_dir_ports = resp.num_dir_ports;
	args->num_atomic_inflights = resp.num_atomic_inflights;
	args->max_contiguous_atomic_inflights =
		resp.max_contiguous_atomic_inflights;
	args->num_hist_list_entries = resp.num_hist_list_entries;
	args->max_contiguous_hist_list_entries =
		resp.max_contiguous_hist_list_entries;
	args->num_ldb_credits = resp.num_ldb_credits;
	args->max_contiguous_ldb_credits = resp.max_contiguous_ldb_credits;
	args->num_dir_credits = resp.num_dir_credits;
	args->max_contiguous_dir_credits = resp.max_contiguous_dir_credits;
	args->num_ldb_credit_pools = resp.num_ldb_credit_pools;
	args->num_dir_credit_pools = resp.num_dir_credit_pools;

	return resp.error_code;
}

static int dlb_vf_collect_cq_sched_counts(struct dlb_dev *dlb_dev,
					  struct dlb_sched_counts *data,
					  u32 *elapsed)
{
	struct dlb_get_num_resources_args num_used_rsrcs;
	struct dlb_get_num_resources_args num_rsrcs;
	int i, ret, num_ldb_ports, num_dir_ports;

	/* Query how many ldb and dir ports this VF owns */
	mutex_lock(&dlb_dev->resource_mutex);

	ret = dlb_dev->ops->get_num_resources(&dlb_dev->hw, &num_rsrcs);
	if (ret) {
		mutex_unlock(&dlb_dev->resource_mutex);
		return -EINVAL;
	}

	ret = dlb_vf_get_num_used_rsrcs(&dlb_dev->hw, &num_used_rsrcs);

	mutex_unlock(&dlb_dev->resource_mutex);

	if (ret)
		return -EINVAL;

	num_ldb_ports = num_rsrcs.num_ldb_ports + num_used_rsrcs.num_ldb_ports;

	num_dir_ports = num_rsrcs.num_dir_ports + num_used_rsrcs.num_dir_ports;

	for (i = 0; i < num_ldb_ports; i++) {
		ret = dlb_vf_collect_cq_sched_count(dlb_dev,
						    data,
						    i,
						    true,
						    elapsed);
		if (ret)
			return ret;
	}

	for (i = 0; i < num_dir_ports; i++) {
		ret = dlb_vf_collect_cq_sched_count(dlb_dev,
						    data,
						    i,
						    false,
						    elapsed);
		if (ret)
			return ret;
	}

	return 0;
}

static int dlb_vf_measure_sched_count(struct dlb_dev *dev,
				      struct dlb_measure_sched_count_args *args,
				      struct dlb_cmd_response *response)
{
	struct dlb_sched_counts data;
	long timeout;
	u32 elapsed;
	int ret = 0;

	memset(&data, 0, sizeof(data));

	/* Only one active measurement is allowed at a time. */
	mutex_lock(&dev->measurement_mutex);

	/* Send a mailbox request to start the measurement. */
	ret = dlb_vf_req_sched_count_measurement(dev,
						 args->measurement_duration_us);
	if (ret) {
		mutex_unlock(&dev->measurement_mutex);
		goto done;
	}

	timeout = usecs_to_jiffies(args->measurement_duration_us);

	wait_event_interruptible_timeout(dev->measurement_wq,
					 !READ_ONCE(dev->reset_active),
					 timeout);

	/* Send mailbox requests to collect the measurement data */
	ret = dlb_vf_collect_cq_sched_counts(dev, &data, &elapsed);
	if (ret) {
		mutex_unlock(&dev->measurement_mutex);
		goto done;
	}

	mutex_unlock(&dev->measurement_mutex);

	if (copy_to_user((void __user *)args->elapsed_time_us,
			 &elapsed,
			 sizeof(elapsed))) {
		pr_err("Invalid elapsed time pointer\n");
		return -EFAULT;
	}

	if (copy_to_user((void __user *)args->sched_count_data,
			 &data,
			 sizeof(data))) {
		pr_err("Invalid performance metric group data pointer\n");
		return -EFAULT;
	}

done:
	return ret;
}

static int dlb_vf_measure_perf(struct dlb_dev *dev,
			       struct dlb_sample_perf_counters_args *args,
			       struct dlb_cmd_response *response)
{
	pr_err("VF devices not allowed to access device-global perf metrics\n");
	return -EINVAL;
}

void dlb_vf_ack_vf_flr_done(struct dlb_hw *hw)
{
	struct dlb_mbox_ack_vf_flr_done_cmd_resp resp;
	struct dlb_mbox_ack_vf_flr_done_cmd_req req;
	struct dlb_dev *dlb_dev;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_ACK_VF_FLR_DONE;

	dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));
}

static int dlb_vf_get_ldb_queue_depth(struct dlb_hw *hw,
				      u32 id,
				      struct dlb_get_ldb_queue_depth_args *args,
				      struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_get_ldb_queue_depth_cmd_resp resp;
	struct dlb_mbox_get_ldb_queue_depth_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_GET_LDB_QUEUE_DEPTH;
	req.domain_id = id;
	req.queue_id = args->queue_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.depth;

	return resp.error_code;
}

static int dlb_vf_get_dir_queue_depth(struct dlb_hw *hw,
				      u32 id,
				      struct dlb_get_dir_queue_depth_args *args,
				      struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_get_dir_queue_depth_cmd_resp resp;
	struct dlb_mbox_get_dir_queue_depth_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_GET_DIR_QUEUE_DEPTH;
	req.domain_id = id;
	req.queue_id = args->queue_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.depth;

	return resp.error_code;
}

static int dlb_vf_pending_port_unmaps(struct dlb_hw *hw,
				      u32 id,
				      struct dlb_pending_port_unmaps_args *args,
				      struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_pending_port_unmaps_cmd_resp resp;
	struct dlb_mbox_pending_port_unmaps_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_PENDING_PORT_UNMAPS;
	req.domain_id = id;
	req.port_id = args->port_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.num;

	return resp.error_code;
}

static int dlb_vf_query_cq_poll_mode(struct dlb_dev *dlb_dev,
				     struct dlb_cmd_response *user_resp)
{
	struct dlb_mbox_query_cq_poll_mode_cmd_resp resp;
	struct dlb_mbox_query_cq_poll_mode_cmd_req req;
	int ret;

	req.hdr.type = DLB_MBOX_CMD_QUERY_CQ_POLL_MODE;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		user_resp->status = DLB_ST_MBOX_ERROR;

		return -1;
	}

	user_resp->status = resp.status;
	user_resp->id = resp.mode;

	return resp.error_code;
}

/**************************************/
/****** Resource query callbacks ******/
/**************************************/

static int dlb_vf_ldb_port_owned_by_domain(struct dlb_hw *hw,
					   u32 domain_id,
					   u32 port_id)
{
	struct dlb_mbox_ldb_port_owned_by_domain_cmd_resp resp;
	struct dlb_mbox_ldb_port_owned_by_domain_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_LDB_PORT_OWNED_BY_DOMAIN;
	req.domain_id = domain_id;
	req.port_id = port_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.owned;
}

static int dlb_vf_dir_port_owned_by_domain(struct dlb_hw *hw,
					   u32 domain_id,
					   u32 port_id)
{
	struct dlb_mbox_dir_port_owned_by_domain_cmd_resp resp;
	struct dlb_mbox_dir_port_owned_by_domain_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_DIR_PORT_OWNED_BY_DOMAIN;
	req.domain_id = domain_id;
	req.port_id = port_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.owned;
}

static int dlb_vf_get_sn_allocation(struct dlb_hw *hw, u32 group_id)
{
	struct dlb_mbox_get_sn_allocation_cmd_resp resp;
	struct dlb_mbox_get_sn_allocation_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_GET_SN_ALLOCATION;
	req.group_id = group_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.num;
}

static int dlb_vf_set_sn_allocation(struct dlb_hw *hw, u32 group_id, u32 val)
{
	/* Only the PF can modify the SN allocations */
	return -EPERM;
}

static int dlb_vf_get_sn_occupancy(struct dlb_hw *hw, u32 group_id)
{
	struct dlb_mbox_get_sn_occupancy_cmd_resp resp;
	struct dlb_mbox_get_sn_occupancy_cmd_req req;
	struct dlb_dev *dlb_dev;
	int ret;

	dlb_dev = container_of(hw, struct dlb_dev, hw);

	req.hdr.type = DLB_MBOX_CMD_GET_SN_OCCUPANCY;
	req.group_id = group_id;

	ret = dlb_send_sync_mbox_cmd(dlb_dev, &req, sizeof(req), 1);
	if (ret)
		return ret;

	BUILD_BUG_ON(sizeof(resp) > DLB_PF2VF_RESP_BYTES);

	dlb_vf_read_pf_mbox_resp(&dlb_dev->hw, &resp, sizeof(resp));

	if (resp.hdr.status != DLB_MBOX_ST_SUCCESS) {
		dev_err(dlb_dev->dlb_device,
			"[%s()]: failed with mailbox error: %s\n",
			__func__,
			DLB_MBOX_ST_STRING(&resp));

		return -1;
	}

	return resp.num;
}

/*******************************/
/****** DLB VF Device Ops ******/
/*******************************/

struct dlb_device_ops dlb_vf_ops = {
	.map_pci_bar_space = dlb_vf_map_pci_bar_space,
	.unmap_pci_bar_space = dlb_vf_unmap_pci_bar_space,
	.mmap = dlb_vf_mmap,
	.inc_pm_refcnt = dlb_vf_pm_inc_refcnt,
	.dec_pm_refcnt = dlb_vf_pm_dec_refcnt,
	.init_driver_state = dlb_vf_init_driver_state,
	.free_driver_state = dlb_vf_free_driver_state,
	.device_create = dlb_vf_device_create,
	.device_destroy = dlb_vf_device_destroy,
	.cdev_add = dlb_vf_cdev_add,
	.cdev_del = dlb_vf_cdev_del,
	.init_hardware = dlb_vf_init_hardware,
	.create_sched_domain = dlb_vf_create_sched_domain,
	.create_ldb_pool = dlb_vf_create_ldb_pool,
	.create_dir_pool = dlb_vf_create_dir_pool,
	.create_ldb_queue = dlb_vf_create_ldb_queue,
	.create_dir_queue = dlb_vf_create_dir_queue,
	.create_ldb_port = dlb_vf_create_ldb_port,
	.create_dir_port = dlb_vf_create_dir_port,
	.start_domain = dlb_vf_start_domain,
	.map_qid = dlb_vf_map_qid,
	.unmap_qid = dlb_vf_unmap_qid,
	.enable_ldb_port = dlb_vf_enable_ldb_port,
	.enable_dir_port = dlb_vf_enable_dir_port,
	.disable_ldb_port = dlb_vf_disable_ldb_port,
	.disable_dir_port = dlb_vf_disable_dir_port,
	.get_num_resources = dlb_vf_get_num_resources,
	.reset_domain = dlb_vf_reset_domain,
	.measure_perf = dlb_vf_measure_perf,
	.measure_sched_count = dlb_vf_measure_sched_count,
	.ldb_port_owned_by_domain = dlb_vf_ldb_port_owned_by_domain,
	.dir_port_owned_by_domain = dlb_vf_dir_port_owned_by_domain,
	.get_sn_allocation = dlb_vf_get_sn_allocation,
	.set_sn_allocation = dlb_vf_set_sn_allocation,
	.get_sn_occupancy = dlb_vf_get_sn_occupancy,
	.get_ldb_queue_depth = dlb_vf_get_ldb_queue_depth,
	.get_dir_queue_depth = dlb_vf_get_dir_queue_depth,
	.pending_port_unmaps = dlb_vf_pending_port_unmaps,
	.query_cq_poll_mode = dlb_vf_query_cq_poll_mode,
};
