// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include "dlb_main.h"
#include "dlb_resource.h"

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
};
