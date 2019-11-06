// SPDX-License-Identifier: GPL-2.0 only
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/pci_ids.h>
#include "../common/mxlk.h"
#include "../common/mxlk_core.h"
#include "../common/mxlk_print.h"
#include "../common/mxlk_mmio.h"
#include "../common/mxlk_boot.h"
#include "mxlk_struct.h"
#include "mxlk_dma.h"

static bool dma_enable = true;
module_param(dma_enable, bool, 0664);
MODULE_PARM_DESC(dma_enable, "DMA transfer enable");

#define BAR2_SIZE (16 * 1024 * 1024)
#define BAR4_SIZE (512 * 1024 * 1024)

static struct pci_epf_header mxlk_pcie_header = {
	.vendorid = PCI_VENDOR_ID_INTEL,
	.deviceid = 0x6240,
	.baseclass_code = PCI_BASE_CLASS_MULTIMEDIA,
	.subclass_code = 0x0,
	.subsys_vendor_id = 0x0,
	.subsys_id = 0x0,
};

static const struct pci_epf_device_id mxlk_pcie_epf_ids[] = {
	{
		.name = "mxlk_pcie_epf",
	},
	{},
};

int mxlk_register_host_irq(struct mxlk *mxlk, irq_handler_t func,
			   unsigned int flags)
{
	struct mxlk_epf *mxlk_epf = container_of(mxlk, struct mxlk_epf, mxlk);

	/* Enable interrupt */
	writel(BIT(18), mxlk_epf->apb_base + 0x18);

	return request_irq(mxlk_epf->irq, func, flags, MXLK_DRIVER_NAME, mxlk);
}

int mxlk_raise_irq(struct mxlk *mxlk, unsigned int flags)
{
	struct mxlk_epf *mxlk_epf = container_of(mxlk, struct mxlk_epf, mxlk);
	struct pci_epf *epf = mxlk_epf->epf;

	return pci_epc_raise_irq(epf->epc, epf->func_no, PCI_EPC_IRQ_MSI, 1);
}

static void __iomem *mxlk_epc_alloc_addr(struct pci_epc *epc,
					 phys_addr_t *phys_addr, size_t size)
{
	void __iomem *virt_addr;
	unsigned long flags;

	spin_lock_irqsave(&epc->lock, flags);
	virt_addr = pci_epc_mem_alloc_addr(epc, phys_addr, size);
	spin_unlock_irqrestore(&epc->lock, flags);

	return virt_addr;
}

static void mxlk_epc_free_addr(struct pci_epc *epc, phys_addr_t phys_addr,
			       void __iomem *virt_addr, size_t size)
{
	unsigned long flags;

	spin_lock_irqsave(&epc->lock, flags);
	pci_epc_mem_free_addr(epc, phys_addr, virt_addr, size);
	spin_unlock_irqrestore(&epc->lock, flags);
}

int mxlk_copy_from_host(struct mxlk *mxlk, void *dst_addr, u64 pci_addr,
			size_t len)
{
	int ret;
	size_t alloc_len;
	void __iomem *src_addr;
	phys_addr_t phys_addr;
	struct mxlk_epf *mxlk_epf = container_of(mxlk, struct mxlk_epf, mxlk);
	struct pci_epf *epf = mxlk_epf->epf;
	struct device *dev = &epf->dev;
	struct pci_epc *epc = epf->epc;
	dma_addr_t dma_dst;
	struct device *dma_dev = epf->epc->dev.parent;
	int tx;

	alloc_len = (((len - 1) / (16 * 1024)) + 1 ) * (16 * 1024);

	src_addr = mxlk_epc_alloc_addr(epc, &phys_addr, alloc_len);
	if (!src_addr) {
		dev_err(dev, "Failed to allocate address\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = pci_epc_map_addr(epc, epf->func_no, phys_addr, pci_addr, alloc_len);
	if (ret) {
		dev_err(dev, "Failed to map address\n");
		goto err_addr;
	}

	if (dma_enable) {
		dma_dst = dma_map_single(dma_dev, dst_addr, len, DMA_FROM_DEVICE);
		if (dma_mapping_error(dma_dev, dma_dst)) {
			dev_err(dev,
				"Failed to map destination buffer address, using memcpy..\n");
			memcpy_fromio(dst_addr, src_addr, len);
			goto skip_dma;
		}

		tx = mxlk_ep_dma_read(epc, dma_dst, phys_addr, len);
		if (tx) {
			dev_err(dev, "DMA transfer failed, using memcpy..\n");
			dma_unmap_single(dma_dev, dma_dst, len, DMA_FROM_DEVICE);
			memcpy_fromio(dst_addr, src_addr, len);
			goto skip_dma;
		}

		dma_unmap_single(dma_dev, dma_dst, len, DMA_FROM_DEVICE);
	} else {
		memcpy_fromio(dst_addr, src_addr, len);
	}

skip_dma:
	pci_epc_unmap_addr(epc, epf->func_no, phys_addr);

err_addr:
	mxlk_epc_free_addr(epc, phys_addr, src_addr, alloc_len);

err:
	return ret;
}

int mxlk_copy_to_host(struct mxlk *mxlk, u64 pci_addr, void *src_addr,
		      size_t len)
{
	int ret;
	size_t alloc_len;
	void __iomem *dst_addr;
	phys_addr_t phys_addr;
	struct mxlk_epf *mxlk_epf = container_of(mxlk, struct mxlk_epf, mxlk);
	struct pci_epf *epf = mxlk_epf->epf;
	struct device *dev = &epf->dev;
	struct pci_epc *epc = epf->epc;
	dma_addr_t dma_src;
	struct device *dma_dev = epf->epc->dev.parent;
	int tx;

	alloc_len = (((len - 1) / (16 * 1024)) + 1 ) * (16 * 1024);

	dst_addr = mxlk_epc_alloc_addr(epc, &phys_addr, alloc_len);
	if (!dst_addr) {
		dev_err(dev, "Failed to allocate address\n");
		ret = -ENOMEM;
		goto err;
	}

	ret = pci_epc_map_addr(epc, epf->func_no, phys_addr, pci_addr, alloc_len);
	if (ret) {
		dev_err(dev, "Failed to map address\n");
		goto err_addr;
	}

	if (dma_enable) {
		dma_src = dma_map_single(dma_dev, src_addr, len, DMA_TO_DEVICE);
		if (dma_mapping_error(dma_dev, dma_src)) {
			dev_err(dev,
				"Failed to map source buffer address, using memcpy..\n");
			memcpy_toio(dst_addr, src_addr, len);
			goto skip_dma;
		}

		tx = mxlk_ep_dma_write(epc, phys_addr, dma_src, len);
		if (tx) {
			dev_err(dev, "DMA transfer failed, using memcpy..\n");
			dma_unmap_single(dma_dev, dma_src, len, DMA_TO_DEVICE);
			memcpy_toio(dst_addr, src_addr, len);
			goto skip_dma;
		}

		dma_unmap_single(dma_dev, dma_src, len, DMA_TO_DEVICE);
	} else {
		memcpy_toio(dst_addr, src_addr, len);
	}

skip_dma:
	pci_epc_unmap_addr(epc, epf->func_no, phys_addr);

err_addr:
	mxlk_epc_free_addr(epc, phys_addr, dst_addr, alloc_len);

err:
	return ret;
}

static int mxlk_check_bar(struct pci_epf *epf, struct pci_epf_bar *epf_bar,
			  enum pci_barno barno, size_t size, u8 reserved_bar)
{
	if (reserved_bar & (1 << barno)) {
		dev_err(&epf->dev, "BAR%d is already reserved\n", barno);
		return -EFAULT;
	}

	if (epf_bar->size != 0 && epf_bar->size != size) {
		dev_err(&epf->dev, "BAR%d fixed size is not enough\n", barno);
		return -ENOMEM;
	}

	return 0;
}

static int mxlk_configure_bar(struct pci_epf *epf,
			      const struct pci_epc_features *epc_features)
{
	struct pci_epf_bar *epf_bar;
	bool bar_fixed_64bit;
	int i;
	int ret;

	for (i = BAR_0; i <= BAR_5; i++) {
		epf_bar = &epf->bar[i];
		bar_fixed_64bit = !!(epc_features->bar_fixed_64bit & (1 << i));
		if (bar_fixed_64bit)
			epf_bar->flags |= PCI_BASE_ADDRESS_MEM_TYPE_64;
		if (epc_features->bar_fixed_size[i])
			epf_bar->size = epc_features->bar_fixed_size[i];

		if (i == BAR_2) {
			ret = mxlk_check_bar(epf, epf_bar, BAR_2,
					     BAR2_SIZE,
					     epc_features->reserved_bar);
			if (ret)
				return ret;
		}

		if (i == BAR_4) {
			ret = mxlk_check_bar(epf, epf_bar, BAR_4,
					     BAR4_SIZE,
					     epc_features->reserved_bar);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static void mxlk_cleanup_bar(struct pci_epf *epf, enum pci_barno barno)
{
	struct pci_epc *epc = epf->epc;
	struct mxlk_epf *mxlk_epf = epf_get_drvdata(epf);

	if (mxlk_epf->vaddr[barno]) {
		pci_epc_clear_bar(epc, epf->func_no, &epf->bar[barno]);
		pci_epf_free_space(epf, mxlk_epf->vaddr[barno], barno);
	}

	mxlk_epf->vaddr[barno] = NULL;
}

static void mxlk_cleanup_bars(struct pci_epf *epf)
{
	struct mxlk_epf *mxlk_epf = epf_get_drvdata(epf);
	mxlk_cleanup_bar(epf, BAR_2);
	mxlk_cleanup_bar(epf, BAR_4);
	mxlk_epf->mxlk.io_comm = NULL;
	mxlk_epf->mxlk.mmio = NULL;
	mxlk_epf->mxlk.bar4 = NULL;
}

static int mxlk_setup_bar(struct pci_epf *epf, enum pci_barno barno,
			  size_t size, size_t align)
{
	int ret;
	void *vaddr;
	struct pci_epc *epc = epf->epc;
	struct mxlk_epf *mxlk_epf = epf_get_drvdata(epf);
	struct pci_epf_bar *bar = &epf->bar[barno];
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
	struct keembay_pcie *keembay = to_keembay_pcie(pci);

	bar->flags |= PCI_BASE_ADDRESS_MEM_TYPE_64;
	bar->size = size;

	if (barno == 2)
		bar->phys_addr = keembay->mmr2->start;
	if (barno == 4)
		bar->phys_addr = keembay->mmr4->start;

	vaddr = ioremap(bar->phys_addr, size);

	epf->bar[barno].phys_addr = bar->phys_addr;
	epf->bar[barno].size = size;
	epf->bar[barno].barno = barno;
	epf->bar[barno].flags |= upper_32_bits(size) ?
				PCI_BASE_ADDRESS_MEM_TYPE_64 :
				PCI_BASE_ADDRESS_MEM_TYPE_32;

	keembay->setup_bar = true;
	ret = pci_epc_set_bar(epc, epf->func_no, bar);
	keembay->setup_bar = false;

	if (ret) {
		pci_epf_free_space(epf, vaddr, barno);
		dev_err(&epf->dev, "Failed to set BAR%d\n", barno);
		return ret;
	}

	mxlk_epf->vaddr[barno] = vaddr;

	return 0;
}

static int mxlk_setup_bars(struct pci_epf *epf, size_t align)
{
	int ret;

	struct mxlk_epf *mxlk_epf = epf_get_drvdata(epf);

	ret = mxlk_setup_bar(epf, 2, BAR2_SIZE, align);
	if (ret)
		return ret;

	ret = mxlk_setup_bar(epf, 4, BAR4_SIZE, align);
	if (ret) {
		mxlk_cleanup_bar(epf, BAR_2);
		return ret;
	}

	mxlk_epf->comm_bar = BAR_2;
	mxlk_epf->mxlk.io_comm = mxlk_epf->vaddr[BAR_2];
	mxlk_epf->mxlk.mmio = mxlk_epf->mxlk.io_comm + MXLK_MMIO_OFFSET;

	mxlk_epf->bar4 = BAR_4;
	mxlk_epf->mxlk.bar4 = mxlk_epf->vaddr[BAR_4];

	return 0;
}

static int epf_bind(struct pci_epf *epf)
{
	struct pci_epc *epc = epf->epc;
	struct mxlk_epf *mxlk_epf = epf_get_drvdata(epf);
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
	struct keembay_pcie *keembay = to_keembay_pcie(pci);
	const struct pci_epc_features *features;
	bool msi_capable = true;
	size_t align = 0;
	int ret;

	if (WARN_ON_ONCE(!epc))
		return -EINVAL;

	features = pci_epc_get_features(epc, epf->func_no);
	mxlk_epf->epc_features = features;
	if (features) {
		msi_capable = features->msi_capable;
		align = features->align;
		ret = mxlk_configure_bar(epf, features);
		if (ret)
			return ret;
	}

	mxlk_epf->irq = keembay->irq;
	mxlk_epf->apb_base = keembay->slv_apb_base;

	ret = mxlk_setup_bars(epf, align);
	if (ret) {
		dev_err(&epf->dev, "BAR initialize failed\n");
		return ret;
	}

	mxlk_ep_dma_init(epc);

	ret = mxlk_core_init(&mxlk_epf->mxlk);
	if (ret) {
		dev_err(&epf->dev, "Core component configuration failed\n");
		goto error;
	}

	mxlk_wr_buffer(mxlk_epf->mxlk.io_comm, MXLK_BOOT_OFFSET_MAGIC,
		       MXLK_BOOT_MAGIC_YOCTO, strlen(MXLK_BOOT_MAGIC_YOCTO));

	return 0;

error:
	mxlk_cleanup_bars(epf);

	return ret;
}

static void epf_unbind(struct pci_epf *epf)
{
	struct pci_epc *epc = epf->epc;
	struct mxlk_epf *mxlk_epf = epf_get_drvdata(epf);

	mxlk_core_cleanup(&mxlk_epf->mxlk);

	mxlk_ep_dma_uninit(epc);

	pci_epc_stop(epc);

	mxlk_cleanup_bars(epf);
}

static void epf_linkup(struct pci_epf *epf)
{
}

static int epf_probe(struct pci_epf *epf)
{
	struct mxlk_epf *mxlk_epf;
	struct device *dev = &epf->dev;

	mxlk_epf = devm_kzalloc(dev, sizeof(*mxlk_epf), GFP_KERNEL);
	if (!mxlk_epf)
		return -ENOMEM;

	epf->header = &mxlk_pcie_header;
	mxlk_epf->epf = epf;

	epf_set_drvdata(epf, mxlk_epf);

	return 0;
}

static struct pci_epf_ops ops = {
	.bind = epf_bind,
	.unbind = epf_unbind,
	.linkup = epf_linkup,
};

static struct pci_epf_driver mxlk_pcie_epf_driver = {
	.driver.name = "mxlk_pcie_epf",
	.probe = epf_probe,
	.id_table = mxlk_pcie_epf_ids,
	.ops = &ops,
	.owner = THIS_MODULE,
};

static int __init mxlk_epf_init(void)
{
	int ret = -EBUSY;

	ret = pci_epf_register_driver(&mxlk_pcie_epf_driver);
	if (ret) {
		pr_err("Failed to register mxlk pcie epf driver: %d\n", ret);
		return ret;
	}

	return 0;
}
module_init(mxlk_epf_init);

static void __exit mxlk_epf_exit(void)
{
	pci_epf_unregister_driver(&mxlk_pcie_epf_driver);
}
module_exit(mxlk_epf_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Intel");
MODULE_DESCRIPTION(MXLK_DRIVER_DESC);
