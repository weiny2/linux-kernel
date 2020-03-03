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
#include <linux/of_reserved_mem.h>
#include <linux/of_device.h>
#include "../common/xlink_drv_inf.h"
#include "../common/mxlk.h"
#include "../common/mxlk_core.h"
#include "../common/mxlk_util.h"
#include "../common/mxlk_print.h"
#include "../common/mxlk_boot.h"
#include "mxlk_struct.h"
#include "mxlk_dma.h"

static bool dma_enable = true;
module_param(dma_enable, bool, 0664);
MODULE_PARM_DESC(dma_enable, "DMA transfer enable");

static int func_no = 0;
module_param(func_no, int, 0664);
MODULE_PARM_DESC(func_no, "function number enable");

static LIST_HEAD(dev_list);
static DEFINE_MUTEX(dev_list_mutex);

#define BAR0_SIZE (4 * 1024)
#define BAR2_SIZE (16 * 1024 * 1024)
#define BAR4_SIZE (8 * 1024 * 1024)

static struct pci_epf_header mxlk_pcie_header = {
	.vendorid = PCI_VENDOR_ID_INTEL,
	.deviceid = 0x4fc0,
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
	return request_irq(mxlk_epf->irq_doorbell, func, flags, MXLK_DRIVER_NAME, mxlk);
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

u32 mxlk_get_device_num(u32 *id_list)
{
	u32 num = 0;
	struct mxlk_epf *p;
	mutex_lock(&dev_list_mutex);
	list_for_each_entry(p, &dev_list, list) {
		if (true == p->sw_dev_id_updated)
		{
	            *id_list++ = p->sw_devid;
	             num++;
		}
	}
	mutex_unlock(&dev_list_mutex);
	return num;
}

static struct mxlk_epf *mxlk_get_device_by_id(u32 id)
{
	struct mxlk_epf *xdev;
        mutex_lock(&dev_list_mutex);
	list_for_each_entry(xdev, &dev_list, list) {
	if (xdev->sw_devid == id)
		break;
	}
	mutex_unlock(&dev_list_mutex);
	return xdev;
}


int mxlk_get_device_name_by_id(u32 id, char *device_name, size_t name_size)
{
	struct mxlk_epf *xdev;
	size_t size;
        xdev = mxlk_get_device_by_id(id);
        if (!xdev)
		return -ENODEV;
	size = (name_size > MXLK_MAX_NAME_LEN) ? MXLK_MAX_NAME_LEN : name_size;
	strncpy(device_name, xdev->name, size);
	return 0;
}

struct mxlk_epf *mxlk_get_device_by_name(const char *name)
{
	struct mxlk_epf *p;
	mutex_lock(&dev_list_mutex);
	list_for_each_entry(p, &dev_list, list) {
	if (!strncmp(p->name, name, MXLK_MAX_NAME_LEN))
		break;
	}
	mutex_unlock(&dev_list_mutex);
	return p;
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
	dma_addr_t dma_dst = (dma_addr_t)dst_addr;
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
        dev_dbg(dev, "pcie:mxlk_copy_from_host() device=%s: phys_addr=%llx,pci_addr=%llx,function_no=%d\n",
			mxlk_epf->name,phys_addr,pci_addr,epf->func_no);
	if (dma_enable) {
		tx = mxlk_ep_dma_read(epf, dma_dst, phys_addr, len);
		if (tx) {
			dev_err(dev, "DMA transfer failed, using memcpy..\n");
			memcpy_fromio(dst_addr, src_addr, len);
			goto skip_dma;
		}
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
	dma_addr_t dma_src = (dma_addr_t)src_addr;
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
        dev_dbg(dev,"pcie:mxlk_to_host() device=%s: phys_addr=%llu,pci_addr=%llu,function_no=%d\n",
			mxlk_epf->name,phys_addr,pci_addr,epf->func_no);
	if (dma_enable) {
		tx = mxlk_ep_dma_write(epf, phys_addr, dma_src, len);
		if (tx) {
			dev_err(dev, "DMA transfer failed, using memcpy..\n");
			memcpy_toio(dst_addr, src_addr, len);
			goto skip_dma;
		}
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
	void __iomem *vaddr = NULL;
	struct pci_epc *epc = epf->epc;
	struct mxlk_epf *mxlk_epf = epf_get_drvdata(epf);
	struct pci_epf_bar *bar = &epf->bar[barno];
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
	struct thunderbay_pcie *thunderbay = to_thunderbay_pcie(pci);

	bar->flags |= PCI_BASE_ADDRESS_MEM_TYPE_64;
	bar->size = size;

	if (barno == 0) {
		bar->phys_addr = thunderbay->doorbell->start + (epf->func_no*0x1000);
		vaddr = ioremap(bar->phys_addr, size);
	}

	if (barno == 2) {
		bar->phys_addr = thunderbay->mmr2[epf->func_no]->start;
		vaddr = ioremap(bar->phys_addr, size);
	}
	if (barno == 4) {
		bar->phys_addr = thunderbay->mmr4[epf->func_no]->start;
		vaddr = ioremap_cache(bar->phys_addr, size);
	}

	if (!vaddr) {
		dev_err(&epf->dev, "Failed to map BAR%d\n", barno);
		return -ENOMEM;
	}

	epf->bar[barno].phys_addr = bar->phys_addr;
	epf->bar[barno].size = size;
	epf->bar[barno].barno = barno;
	epf->bar[barno].flags |= upper_32_bits(size) ?
				PCI_BASE_ADDRESS_MEM_TYPE_64 :
				PCI_BASE_ADDRESS_MEM_TYPE_32;

	thunderbay->setup_bar[epf->func_no] = true;
	ret = pci_epc_set_bar(epc, epf->func_no, bar);
	thunderbay->setup_bar[epf->func_no] = false;

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

	ret = mxlk_setup_bar(epf, 0, BAR0_SIZE, align);
	if (ret)
		return ret;

	ret = mxlk_setup_bar(epf, 2, BAR2_SIZE, align);
	if (ret)
		return ret;

	ret = mxlk_setup_bar(epf, 4, BAR4_SIZE, align);
	if (ret) {
		mxlk_cleanup_bar(epf, BAR_2);
		return ret;
	}
	mxlk_epf->doorbell_bar = BAR_0;
	mxlk_epf->mxlk.doorbell_base = mxlk_epf->vaddr[BAR_0];

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
	struct thunderbay_pcie *thunderbay = to_thunderbay_pcie(pci);
	const struct pci_epc_features *features;
	bool msi_capable = true;
	size_t align = 0;
	size_t doorbell_clr_size = 0;
	unsigned long doorbell_clr_addr = 0;
	int ret;

	if (WARN_ON_ONCE(!epc))
		return -EINVAL;
#if 1
	if (!(epf->func_no & 0x1))
	{
		return 0;
	}
#endif
	features = pci_epc_get_features(epc, epf->func_no);
	mxlk_epf->epc_features = features;
	if (features) {
		msi_capable = features->msi_capable;
		align = features->align;
		ret = mxlk_configure_bar(epf, features);
		if (ret)
			goto bind_error;
	}

	mxlk_epf->irq_doorbell = thunderbay->irq_doorbell[epf->func_no];
	mxlk_epf->irq_rdma = thunderbay->irq_rdma[epf->func_no];
	mxlk_epf->irq_wdma = thunderbay->irq_wdma[epf->func_no];
	mxlk_epf->apb_base = thunderbay->slv_apb_base;

	ret = mxlk_setup_bars(epf, align);
	if (ret) {
		dev_err(&epf->dev, "BAR initialize failed\n");
		goto bind_error;
	}

	switch(epf->func_no)
	{
		case 0:
                        doorbell_clr_addr = thunderbay->doorbell_clear->start;
			doorbell_clr_size = 4;
			break;
		case 1:
			ret = of_reserved_mem_device_init_by_idx(&epf->dev,pci->dev->of_node, 16);
			if (ret) {
				dev_dbg(&epc->dev,"pcie:of_reserved_mem_device_init_by_idx():16:failed.ret=%d\n",ret);
			}
			epf->dev.dma_mask = pci->dev->dma_mask;
			epf->dev.coherent_dma_mask = pci->dev->coherent_dma_mask;
			ret = of_dma_configure(&epf->dev, pci->dev->of_node, true);
			if (ret) {
				dev_dbg(&epc->dev,"pcie:failed of_dma_configure().ret=%d\n",ret);
			}
			ret = dma_set_mask_and_coherent(&epf->dev, DMA_BIT_MASK(64));
			if (ret) {
				dev_dbg(&epc->dev,"pcie:failed to set dma mask.ret=%d\n",ret);
			}
                        doorbell_clr_addr = thunderbay->doorbell_clear->start+0x8;
			doorbell_clr_size = 4;
			break;
		case 2:
                        doorbell_clr_addr = thunderbay->doorbell_clear->start+0x14;
			doorbell_clr_size = 4;
			break;
		case 3:
			ret = of_reserved_mem_device_init_by_idx(&epf->dev,pci->dev->of_node, 17);
			if (ret) {
				dev_dbg(&epc->dev,"pcie:of_reserved_mem_device_init_by_idx():17:failed.ret=%d\n",ret);
			}
			epf->dev.dma_mask = pci->dev->dma_mask;
			epf->dev.coherent_dma_mask = pci->dev->coherent_dma_mask;
			ret = of_dma_configure(&epf->dev, pci->dev->of_node, true);
			if (ret) {
				dev_dbg(&epc->dev,"pcie:failed of_dma_configure().ret=%d\n",ret);
			}
			ret = dma_set_mask_and_coherent(&epf->dev, DMA_BIT_MASK(64));
			if (ret) {
				dev_dbg(&epc->dev,"pcie:failed to set dma mask.ret=%d\n",ret);
			}
                        doorbell_clr_addr = thunderbay->doorbell_clear->start+0x1C;
			doorbell_clr_size = 4;
			break;
		case 4:
                        doorbell_clr_addr = thunderbay->doorbell_clear->start+0x28;
			doorbell_clr_size = 4;
			break;
		case 5:
			ret = of_reserved_mem_device_init_by_idx(&epf->dev,pci->dev->of_node, 18);
			if (ret) {
				dev_dbg(&epc->dev,"pcie:of_reserved_mem_device_init_by_idx():18:failed.ret=%d\n",ret);
			}
			epf->dev.dma_mask = pci->dev->dma_mask;
			epf->dev.coherent_dma_mask = pci->dev->coherent_dma_mask;
			ret = of_dma_configure(&epf->dev, pci->dev->of_node, true);
			if (ret) {
				dev_dbg(&epc->dev,"pcie:failed of_dma_configure().ret=%d\n",ret);
			}
			ret = dma_set_mask_and_coherent(&epf->dev, DMA_BIT_MASK(64));
			if (ret) {
				dev_dbg(&epc->dev,"pcie:failed to set dma mask.ret=%d\n",ret);
			}
                        doorbell_clr_addr = thunderbay->doorbell_clear->start+0x30;
			doorbell_clr_size = 4;
			break;
		case 6:
                        doorbell_clr_addr = thunderbay->doorbell_clear->start+0x3C;
			doorbell_clr_size = 4;
			break;
		case 7:
			ret = of_reserved_mem_device_init_by_idx(&epf->dev,pci->dev->of_node, 19);
			if (ret) {
				dev_dbg(&epc->dev,"pcie:of_reserved_mem_device_init_by_idx():19:failed.ret=%d\n",ret);
			}
			epf->dev.dma_mask = pci->dev->dma_mask;
			epf->dev.coherent_dma_mask = pci->dev->coherent_dma_mask;
			ret = of_dma_configure(&epf->dev, pci->dev->of_node, true);
			if (ret) {
				dev_dbg(&epc->dev,"pcie:failed of_dma_configure().ret=%d\n",ret);
			}
			ret = dma_set_mask_and_coherent(&epf->dev, DMA_BIT_MASK(64));
			if (ret) {
				dev_dbg(&epc->dev,"pcie:failed to set dma mask.ret=%d\n",ret);
			}
                        doorbell_clr_addr = thunderbay->doorbell_clear->start+0x44;
			doorbell_clr_size = 4;
			break;
	}

	mxlk_epf->mxlk.doorbell_clear = ioremap(doorbell_clr_addr,doorbell_clr_size);
	dev_dbg(&epc->dev,"virtual doorbell_clear=%p,physical doorbell_clr_addr=%lx\n",mxlk_epf->mxlk.doorbell_clear,doorbell_clr_addr);

	ret = mxlk_ep_dma_init(epf);
	if (ret) {
		dev_err(&epf->dev, "DMA initialization failed\n");
		goto bind_error;
	}

	mxlk_set_max_functions(&mxlk_epf->mxlk,epc->max_functions);
	mxlk_set_device_status(&mxlk_epf->mxlk, MXLK_STATUS_READY);

	snprintf(mxlk_epf->name, MXLK_MAX_NAME_LEN, "%s_func%x",epf->name, epf->func_no);
	list_add_tail(&mxlk_epf->list, &dev_list);
       	dev_dbg(&epc->dev,"pcie:mxlk_epf->name of device = %s, sw_devid=%d\n",mxlk_epf->name, mxlk_epf->sw_devid);

	ret = mxlk_core_init(&mxlk_epf->mxlk);
	if (ret) {
		dev_err(&epf->dev, "Core component configuration failed\n");
		goto bind_error;
	}

	memcpy_toio(mxlk_epf->mxlk.io_comm + MXLK_BOOT_OFFSET_MAGIC,
		    MXLK_BOOT_MAGIC_YOCTO, strlen(MXLK_BOOT_MAGIC_YOCTO));

	return 0;

bind_error:
	mxlk_set_device_status(&mxlk_epf->mxlk, MXLK_STATUS_ERROR);
	return ret;
}

static void epf_unbind(struct pci_epf *epf)
{
	struct pci_epc *epc = epf->epc;
	struct mxlk_epf *mxlk_epf = epf_get_drvdata(epf);

	mxlk_core_cleanup(&mxlk_epf->mxlk);

	mxlk_ep_dma_uninit(epf);

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
	dev_dbg(dev,"pcie:epf_probe() enter fn=%d\n",epf->func_no);

	mxlk_epf = devm_kzalloc(dev, sizeof(*mxlk_epf), GFP_KERNEL);
	if (!mxlk_epf)
		return -ENOMEM;

	epf->header = &mxlk_pcie_header;
	mxlk_epf->epf = epf;

	epf_set_drvdata(epf, mxlk_epf);
	dev_dbg(dev, "pcie:epf_probe() exit fn=%d\n",epf->func_no);

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
MODULE_VERSION(MXLK_DRIVER_VERSION);
