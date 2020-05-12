// SPDX-License-Identifier: GPL-2.0
/*
 * Compute eXpress Link Support
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/pci_regs.h>

#define PCI_DVSEC_VENDOR_ID_CXL		0x1e98
#define PCI_DVSEC_ID_CXL_DEV		0x0

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

/*
 * pci_find_cxl_capability - Identify and return offset to Vendor-Specific
 * capabilities.
 *
 * CXL makes use of Designated Vendor-Specific Extended Capability (DVSEC)
 * to uniquely identify both DVSEC Vendor ID and DVSEC ID aligning with
 * PCIe r5.0, sec 7.9.6.2
 */
static int pci_find_cxl_capability(struct pci_dev *dev)
{
	u16 vendor, id;
	int pos = 0;

	while ((pos = pci_find_next_ext_capability(dev, pos,
						   PCI_EXT_CAP_ID_DVSEC))) {
		pci_read_config_word(dev, pos + PCI_DVSEC_HEADER1,
				     &vendor);
		pci_read_config_word(dev, pos + PCI_DVSEC_HEADER2, &id);
		if (vendor == PCI_DVSEC_VENDOR_ID_CXL &&
		    id == PCI_DVSEC_ID_CXL_DEV)
			return pos;
	}

	return 0;
}


#define FLAG(x, y)	(((x) & (y)) ? '+' : '-')

void pci_cxl_init(struct pci_dev *dev)
{
	u16 cap, ctrl, status, ctrl2, status2, lock;
	int cxl;

	/* Only for PCIe */
	if (!pci_is_pcie(dev))
		return;

	/* Only for Device 0 Function 0, Root Complex Integrated Endpoints */
	if (dev->devfn != 0 || (pci_pcie_type(dev) != PCI_EXP_TYPE_RC_END))
		return;

	cxl = pci_find_cxl_capability(dev);
	if (!cxl)
		return;

	dev->cxl_cap = cxl;
	pci_read_config_word(dev, cxl + PCI_CXL_CAP, &cap);

	pci_info(dev, "CXL: Cache%c IO%c Mem%c Viral%c HDMCount %d\n",
		FLAG(cap, PCI_CXL_CACHE),
		FLAG(cap, PCI_CXL_IO),
		FLAG(cap, PCI_CXL_MEM),
		FLAG(cap, PCI_CXL_VIRAL),
		PCI_CXL_HDM_COUNT(cap));

	pci_read_config_word(dev, cxl + PCI_CXL_CTRL, &ctrl);
	pci_read_config_word(dev, cxl + PCI_CXL_STS, &status);
	pci_read_config_word(dev, cxl + PCI_CXL_CTRL2, &ctrl2);
	pci_read_config_word(dev, cxl + PCI_CXL_STS2, &status2);
	pci_read_config_word(dev, cxl + PCI_CXL_LOCK, &lock);

	pci_info(dev, "CXL: cap ctrl status ctrl2 status2 lock\n");
	pci_info(dev, "CXL: %04x %04x %04x %04x %04x %04x\n",
		 cap, ctrl, status, ctrl2, status2, lock);
}
