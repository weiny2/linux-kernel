// SPDX-License-Identifier: GPL-2.0
/*
 * Compute eXpress Link Support
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/irqdomain.h>

#define PCI_EXT_CAP_ID_DVSEC		0x23
#define PCI_EXT_DVSEC_VENDOR_OFFSET	0x04
#define PCI_EXT_DVSEC_ID_OFFSET		0x08
#define PCI_CXL_DVSEC_ID		0
#define PCI_CXL_DVSEC_PORT_ID		0x07

#define PCI_CXL_CAP			0x0a
#define PCI_CXL_CTRL			0x0c
#define PCI_CXL_STS			0x0e
#define PCI_CXL_CTRL2			0x10
#define PCI_CXL_STS2			0x12
#define PCI_CXL_LOCK			0x14

#define PCI_CXL_CACHE			BIT(0)
#define PCI_CXL_IO			BIT(1)
#define PCI_CXL_MEM			BIT(2)
#define PCI_CXL_HDM_COUNT(reg)		(((reg) & (3 << 4)) >> 4)
#define PCI_CXL_VIRAL			BIT(14)

#define PCI_CXL_CONFIG_LOCK		BIT(0)

static void pci_cxl_unlock(struct pci_dev *dev)
{
	int pos = dev->cxl_cap;
	u16 lock;

	/* Only for Root Complex Endpoints */
	if (pci_pcie_type(dev) != PCI_EXP_TYPE_RC_END)
		return;

	pci_read_config_word(dev, pos + PCI_CXL_LOCK, &lock);
	lock &= ~PCI_CXL_CONFIG_LOCK;
	pci_write_config_word(dev, pos + PCI_CXL_LOCK, lock);
}

static void pci_cxl_lock(struct pci_dev *dev)
{
	int pos = dev->cxl_cap;
	u16 lock;

	/* Only for Root Complex Endpoints */
	if (pci_pcie_type(dev) != PCI_EXP_TYPE_RC_END)
		return;

	pci_read_config_word(dev, pos + PCI_CXL_LOCK, &lock);
	lock |= PCI_CXL_CONFIG_LOCK;
	pci_write_config_word(dev, pos + PCI_CXL_LOCK, lock);
}

static int pci_cxl_enable_disable_feature(struct pci_dev *dev, int enable,
		u16 feature)
{
	int pos = dev->cxl_cap;
	int ret;
	u16 reg;

	if (!dev->cxl_cap)
		return -EINVAL;

	/* Only for PCIe */
	if (!pci_is_pcie(dev))
		return -EINVAL;

	/* Only for Device 0 Function 0 */
	if (dev->devfn)
		return -EINVAL;

	pci_cxl_unlock(dev);
	ret = pci_read_config_word(dev, pos + PCI_CXL_CTRL, &reg);
	if (ret)
		goto lock;

	if (enable)
		reg |= feature;
	else
		reg &= ~feature;

	ret = pci_write_config_word(dev, pos + PCI_CXL_CTRL, reg);

lock:
	pci_cxl_lock(dev);

	return ret;
}

int pci_cxl_mem_enable(struct pci_dev *dev)
{
	return pci_cxl_enable_disable_feature(dev, true, PCI_CXL_MEM);
}
EXPORT_SYMBOL_GPL(pci_cxl_mem_enable);

void pci_cxl_mem_disable(struct pci_dev *dev)
{
	pci_cxl_enable_disable_feature(dev, false, PCI_CXL_MEM);
}
EXPORT_SYMBOL_GPL(pci_cxl_mem_disable);

int pci_cxl_cache_enable(struct pci_dev *dev)
{
	return pci_cxl_enable_disable_feature(dev, true, PCI_CXL_CACHE);
}
EXPORT_SYMBOL_GPL(pci_cxl_cache_enable);

void pci_cxl_cache_disable(struct pci_dev *dev)
{
	pci_cxl_enable_disable_feature(dev, false, PCI_CXL_CACHE);
}
EXPORT_SYMBOL_GPL(pci_cxl_cache_disable);

static int pci_find_cxl_capability(struct pci_dev *dev, int cxl_dvsec_id)
{
	int pos = 0;
	u16 vendor;
	u16 id;

	do {
		pos = pci_find_next_ext_capability(dev, pos,
				PCI_EXT_CAP_ID_DVSEC);
		pci_read_config_word(dev, pos + PCI_EXT_DVSEC_VENDOR_OFFSET,
				&vendor);
		pci_read_config_word(dev, pos + PCI_EXT_DVSEC_ID_OFFSET, &id);
		if (vendor == PCI_VENDOR_ID_INTEL && id == cxl_dvsec_id)
			return pos;
	} while (pos);

	return 0;
}

void pci_cxl_init(struct pci_dev *dev)
{
	int pos;
	int cxl_dvsec_id;
	u16 cap;

	u16 ctrl;
	u16 status;
	u16 ctrl2;
	u16 status2;
	u16 lock;

	/* Only for PCIe */
	if (!pci_is_pcie(dev))
		return;

	/* Only for Device 0 Function 0 Root Complex Endpoints */
	if ((dev->devfn == 0) && (pci_pcie_type(dev) == PCI_EXP_TYPE_RC_END)) {
		cxl_dvsec_id = PCI_CXL_DVSEC_ID;
		dev_info(&dev->dev, "CXL: pci_cxl_init() : dev 0, func 0 found\n");
	} else if ((pci_pcie_type(dev) == PCI_EXP_TYPE_ROOT_PORT ||
		   pci_pcie_type(dev) == PCI_EXP_TYPE_UPSTREAM ||
		   pci_pcie_type(dev) == PCI_EXP_TYPE_DOWNSTREAM)) {
		dev_info(&dev->dev, "CXL: pci_cxl_init() : upstream/downstream port found\n");
		cxl_dvsec_id = PCI_CXL_DVSEC_PORT_ID;
	} else
		return;

	pos = pci_find_cxl_capability(dev, cxl_dvsec_id);
	if (!pos) {
		dev_info(&dev->dev, "CXL: pci_cxl_init() : No CXL capability found\n");
		return;
	}
	dev->cxl_cap = pos;

	dev_info(&dev->dev, "CXL: pci_cxl_init() : CXL capability found\n");
	pci_read_config_word(dev, pos + PCI_CXL_CAP, &cap);

	if (pci_pcie_type(dev) == PCI_EXP_TYPE_RC_END) {
		dev_info(&dev->dev, "CXL: Cache%c IO%c Mem%c Viral%c HDMCount %d\n",
			 (cap & PCI_CXL_CACHE) ? '+' : '-',
			 (cap & PCI_CXL_IO) ? '+' : '-',
			 (cap & PCI_CXL_MEM) ? '+' : '-',
			 (cap & PCI_CXL_VIRAL) ? '+' : '-',
			 PCI_CXL_HDM_COUNT(cap));

		pci_read_config_word(dev, pos + PCI_CXL_CTRL, &ctrl);
		pci_read_config_word(dev, pos + PCI_CXL_STS, &status);
		pci_read_config_word(dev, pos + PCI_CXL_CTRL2, &ctrl2);
		pci_read_config_word(dev, pos + PCI_CXL_STS2, &status2);
		pci_read_config_word(dev, pos + PCI_CXL_LOCK, &lock);

		dev_info(&dev->dev, "CXL: cap ctrl status ctrl2 status2 lock\n");
		dev_info(&dev->dev, "CXL: %04x %04x %04x %04x %04x %04x\n",
			 cap, ctrl, status, ctrl2, status2, lock);
	} else { /* upstream/downstream Port capabilities */
		dev_info(&dev->dev, "CXL: Cache%c IO%c Mem%c\n",
			 (cap & PCI_CXL_CACHE) ? '+' : '-',
			 (cap & PCI_CXL_IO) ? '+' : '-',
			 (cap & PCI_CXL_MEM) ? '+' : '-');

		pci_read_config_word(dev, pos + PCI_CXL_CTRL, &ctrl);
		pci_read_config_word(dev, pos + PCI_CXL_STS, &status);

		dev_info(&dev->dev, "CXL: cap ctrl status\n");
		dev_info(&dev->dev, "CXL: %04x %04x %04x\n",
			 cap, ctrl, status);
	}
}
