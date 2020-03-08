// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
/* Copyright(c) 2017-2020 Intel Corporation */

#include "dlb_main.h"
#include "dlb_regs.h"
#include "dlb_resource.h"

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

/*******************************/
/****** Driver management ******/
/*******************************/

static int dlb_pf_init_driver_state(struct dlb_dev *dlb_dev)
{
	mutex_init(&dlb_dev->resource_mutex);

	return 0;
}

static void dlb_pf_free_driver_state(struct dlb_dev *dlb_dev)
{
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

static void dlb_pf_init_hardware(struct dlb_dev *dlb_dev)
{
	dlb_disable_dp_vasr_feature(&dlb_dev->hw);
}

/*******************************/
/****** DLB PF Device Ops ******/
/*******************************/

struct dlb_device_ops dlb_pf_ops = {
	.map_pci_bar_space = dlb_pf_map_pci_bar_space,
	.unmap_pci_bar_space = dlb_pf_unmap_pci_bar_space,
	.init_driver_state = dlb_pf_init_driver_state,
	.free_driver_state = dlb_pf_free_driver_state,
	.device_create = dlb_pf_device_create,
	.device_destroy = dlb_pf_device_destroy,
	.cdev_add = dlb_pf_cdev_add,
	.cdev_del = dlb_pf_cdev_del,
	.init_hardware = dlb_pf_init_hardware,
};
