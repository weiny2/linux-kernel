// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include <linux/pm_runtime.h>

#include "dlb_intr.h"
#include "dlb_main.h"
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

/**********************************/
/****** Interrupt management ******/
/**********************************/

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

	init_waitqueue_head(&dlb_dev->measurement_wq);

	mutex_init(&dlb_dev->resource_mutex);
	mutex_init(&dlb_dev->measurement_mutex);

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

static bool dlb_sparse_cq_mode_enabled;

static void dlb_pf_init_hardware(struct dlb_dev *dlb_dev)
{
	dlb_disable_dp_vasr_feature(&dlb_dev->hw);

	dlb_sparse_cq_mode_enabled = dlb_dev->revision >= DLB_REV_B0;

	if (dlb_sparse_cq_mode_enabled) {
		dlb_hw_enable_sparse_ldb_cq_mode(&dlb_dev->hw);
		dlb_hw_enable_sparse_dir_cq_mode(&dlb_dev->hw);
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

static int dlb_pf_measure_perf(struct dlb_dev *dev,
			       struct dlb_sample_perf_counters_args *args,
			       struct dlb_cmd_response *response)
{
	union dlb_perf_metric_group_data data;
	struct timespec64 start, end;
	long timeout;
	u32 elapsed;

	memset(&data, 0, sizeof(data));

	/* Only one active measurement is allowed at a time. */
	if (!mutex_trylock(&dev->measurement_mutex))
		return -EBUSY;

	ktime_get_real_ts64(&start);

	dlb_init_perf_metric_measurement(&dev->hw,
					 args->perf_metric_group_id,
					 args->measurement_duration_us);

	timeout = usecs_to_jiffies(args->measurement_duration_us);

	wait_event_interruptible_timeout(dev->measurement_wq,
					 false,
					 timeout);

	ktime_get_real_ts64(&end);

	dlb_collect_perf_metric_data(&dev->hw,
				     args->perf_metric_group_id,
				     &data);

	mutex_unlock(&dev->measurement_mutex);

	/* Calculate the elapsed time in microseconds */
	elapsed = (end.tv_sec - start.tv_sec) * 1000000 +
		  (end.tv_nsec - start.tv_nsec) / 1000;

	if (copy_to_user((void __user *)args->elapsed_time_us,
			 &elapsed,
			 sizeof(elapsed))) {
		pr_err("Invalid elapsed time pointer\n");
		return -EFAULT;
	}

	if (copy_to_user((void __user *)args->perf_metric_group_data,
			 &data,
			 sizeof(data))) {
		pr_err("Invalid performance metric group data pointer\n");
		return -EFAULT;
	}

	return 0;
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

static int dlb_pf_measure_sched_count(struct dlb_dev *dev,
				      struct dlb_measure_sched_count_args *args,
				      struct dlb_cmd_response *response)
{
	struct dlb_sched_counts *cnt = NULL;
	struct timespec64 start, end;
	int ret = 0, i;
	long timeout;
	u32 elapsed;

	cnt = devm_kzalloc(&dev->pdev->dev, 2 * sizeof(*cnt), GFP_KERNEL);
	if (!cnt) {
		ret = -ENOMEM;
		goto done;
	}

	ktime_get_real_ts64(&start);

	dlb_read_sched_counts(&dev->hw, &cnt[0], false, 0);

	timeout = usecs_to_jiffies(args->measurement_duration_us);

	wait_event_interruptible_timeout(dev->measurement_wq,
					 false,
					 timeout);
	ktime_get_real_ts64(&end);

	dlb_read_sched_counts(&dev->hw, &cnt[1], false, 0);

	/* Calculate the elapsed time in microseconds */
	elapsed = (end.tv_sec - start.tv_sec) * 1000000 +
		  (end.tv_nsec - start.tv_nsec) / 1000;

	if (copy_to_user((void __user *)args->elapsed_time_us,
			 &elapsed,
			 sizeof(elapsed))) {
		pr_err("Invalid elapsed time pointer\n");
		ret = -EFAULT;
		goto done;
	}

	/* Calculate the scheduling count difference */
	cnt[1].ldb_sched_count -= cnt[0].ldb_sched_count;
	cnt[1].dir_sched_count -= cnt[0].dir_sched_count;

	for (i = 0; i < DLB_MAX_NUM_LDB_PORTS; i++)
		cnt[1].ldb_cq_sched_count[i] -= cnt[0].ldb_cq_sched_count[i];

	for (i = 0; i < DLB_MAX_NUM_DIR_PORTS; i++)
		cnt[1].dir_cq_sched_count[i] -= cnt[0].dir_cq_sched_count[i];

	if (copy_to_user((void __user *)args->sched_count_data,
			 &cnt[1],
			 sizeof(cnt[1]))) {
		pr_err("Invalid performance metric group data pointer\n");
		ret = -EFAULT;
		goto done;
	}

done:
	if (cnt)
		devm_kfree(&dev->pdev->dev, cnt);

	return ret;
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
	.init_interrupts = dlb_pf_init_interrupts,
	.enable_ldb_cq_interrupts = dlb_pf_enable_ldb_cq_interrupts,
	.enable_dir_cq_interrupts = dlb_pf_enable_dir_cq_interrupts,
	.arm_cq_interrupt = dlb_pf_arm_cq_interrupt,
	.reinit_interrupts = dlb_pf_reinit_interrupts,
	.free_interrupts = dlb_pf_free_interrupts,
	.init_hardware = dlb_pf_init_hardware,
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
	.measure_perf = dlb_pf_measure_perf,
	.measure_sched_count = dlb_pf_measure_sched_count,
	.ldb_port_owned_by_domain = dlb_pf_ldb_port_owned_by_domain,
	.dir_port_owned_by_domain = dlb_pf_dir_port_owned_by_domain,
	.get_sn_allocation = dlb_pf_get_sn_allocation,
	.set_sn_allocation = dlb_pf_set_sn_allocation,
	.get_sn_occupancy = dlb_pf_get_sn_occupancy,
	.get_ldb_queue_depth = dlb_pf_get_ldb_queue_depth,
	.get_dir_queue_depth = dlb_pf_get_dir_queue_depth,
	.query_cq_poll_mode = dlb_pf_query_cq_poll_mode,
};
