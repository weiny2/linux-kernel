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

static int pci_find_cxl_capability(struct pci_dev *dev)
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
		if (vendor == PCI_VENDOR_ID_INTEL && id == PCI_CXL_DVSEC_ID)
			return pos;
	} while (pos);

	return 0;
}

void pci_cxl_init(struct pci_dev *dev)
{
	int pos;
	u16 cap;
	u16 ctrl;
	u16 status;
	u16 ctrl2;
	u16 status2;
	u16 lock;

	/* Only for PCIe */
	if (!pci_is_pcie(dev))
		return;

	/* Only for Device 0 Function 0 */
	if (dev->devfn)
		return;

	pos = pci_find_cxl_capability(dev);
	if (!pos)
		return;

	dev->cxl_cap = pos;

	pci_read_config_word(dev, pos + PCI_CXL_CAP, &cap);
	pci_read_config_word(dev, pos + PCI_CXL_CTRL, &ctrl);
	pci_read_config_word(dev, pos + PCI_CXL_STS, &status);
	pci_read_config_word(dev, pos + PCI_CXL_CTRL2, &ctrl2);
	pci_read_config_word(dev, pos + PCI_CXL_STS2, &status2);
	pci_read_config_word(dev, pos + PCI_CXL_LOCK, &lock);

	dev_info(&dev->dev, "CXL: Cache%c IO%c Mem%c Viral%d HDMCount %d\n",
			(cap & PCI_CXL_CACHE) ? '+' : '-',
			(cap & PCI_CXL_IO) ? '+' : '-',
			(cap & PCI_CXL_MEM) ? '+' : '-',
			(cap & PCI_CXL_VIRAL) ? '+' : '-',
			PCI_CXL_HDM_COUNT(cap));

	dev_info(&dev->dev, "CXL: cap ctrl status ctrl2 status2 lock\n");
	dev_info(&dev->dev, "CXL: %04x %04x %04x %04x %04x %04x\n",
			cap, ctrl, status, ctrl2, status2, lock);
}
