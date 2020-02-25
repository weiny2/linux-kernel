// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for Intel Integrated Error Handler (IEH)
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * IEH is meant to centralize and standardize how I/O device errors are
 * reported. They are PCIe devices which aggregate and report error events
 * of different severities (correctable, non-fatal uncorrectable, and fatal
 * uncorrectable) from various I/O devices, e.g., PCIe devices, legacy PCI
 * devices.
 *
 * There is a global IEH and optional north/south satellite IEH(s) logically
 * connected to global IEH. The global IEH is the root to process all incoming
 * error messages from satellite IEH(s) and local devices (if some devices
 * are connected directly to the global IEH) and generate interrupts(SMI/NMI/MCE
 * configured by BIOS/platform firmware). The first IEH-supported platform is
 * Tiger Lake-LP/H. This driver reads/prints the error severity and error source
 * (bus/device/function) logged in the IEH(s).
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/slab.h>
#include <linux/bitops.h>
#include <linux/nmi.h>
#include <linux/irq_work.h>
#include <linux/edac.h>
#include <linux/processor.h>
#include <asm/intel-family.h>
#include <asm/cpu_device_id.h>
#include <asm/mce.h>

#include "edac_mc.h"

#define IEH_REVISION	"v1.0"

#define EDAC_MOD_STR	"ieh_edac"
#define IEH_NMI_NAME	"ieh"

#define GET_BITFIELD(v, lo, hi) (((v) & GENMASK_ULL(hi, lo)) >> (lo))

#define IEH_G_SYS_EVT_STS_OFF	0x260
#define IEH_G_CE_STS_OFF	0x200
#define IEH_G_NF_UE_STS_OFF	0x210
#define IEH_G_FA_UE_STS_OFF	0x220

#define IEH_G_SYS_EVT_MAP_OFF	0x268
#define IEH_G_SYS_EVT_MAP_CO(m)	GET_BITFIELD(m, 0, 1)
#define IEH_G_SYS_EVT_MAP_NF(m)	GET_BITFIELD(m, 2, 3)
#define IEH_G_SYS_EVT_MAP_FA(m)	GET_BITFIELD(m, 4, 5)

#define IEH_TYPEVER_OFF		0x26c
#define IEH_TYPEVER_TYP(t)	GET_BITFIELD(t, 0, 3)
#define IEH_TYPEVER_VER(t)	GET_BITFIELD(t, 4, 7)
#define IEH_TYPEVER_BUS(t)	GET_BITFIELD(t, 8, 15)

#define IEH_BITMAP_OFF		0x27c
#define IEH_BITMAP(m)		GET_BITFIELD(m, 0, 4)

#define IEH_DEVFUN_OFF		0x300
#define IEH_DEVFUN_FUN(d)	GET_BITFIELD(d, 0, 2)
#define IEH_DEVFUN_DEV(d)	GET_BITFIELD(d, 7, 3)

#define G_SYS_EVT_STS_MASK	0x7
#define G_SYS_EVT_STS_CO	BIT(0)
#define G_SYS_EVT_STS_NF	BIT(1)
#define G_SYS_EVT_STS_FA	BIT(2)

#define ieh_printk(level, fmt, arg...)	\
	edac_printk(level, "ieh", fmt, ##arg)

#define PCI_RD_U32(pdev, addr, val)					     \
	do {								     \
		if (pci_read_config_dword(pdev, addr, val)) {		     \
			ieh_printk(KERN_ERR, "Failed to read 0x%x\n", addr); \
			goto fail;					     \
		}							     \
	} while (0)

#define PCI_WR_U32(pdev, addr, val)					      \
	do {								      \
		if (pci_write_config_dword(pdev, addr, val)) {		      \
			ieh_printk(KERN_ERR, "Failed to write 0x%x\n", addr); \
			goto fail;					      \
		}							      \
	} while (0)

/* Error notification methods */
enum evt_map {
	IEH_IGN,
	IEH_SMI,
	IEH_NMI,
	IEH_MCE,
};

enum severity_level {
	IEH_CO_ERR,
	IEH_NF_ERR,
	IEH_FA_ERR,
};

enum ieh_type {
	/* Global IEH */
	IEH_GB,
	/* North satellite IEH logically connected to global IEH */
	IEH_NS,
	/* South satellite IEH logically connected to north IEH */
	IEH_SS,
	/*
	 * Superset south satellite IEH with physical ERR[2:0] signals output.
	 * It's used as a global IEH (when it present, system has only one IEH).
	 */
	IEH_SU,
};

struct pci_sbdf {
	u32 seg : 16;
	u32 bus : 8;
	u32 dev : 5;
	u32 fun : 3;
};

struct ieh_dev {
	struct list_head list;
	struct pci_dev *pdev;
	struct pci_sbdf sbdf;
	enum ieh_type type;
	u8 ver;
	/* Global IEH fields */
	enum evt_map co_map;
	enum evt_map nf_map;
	enum evt_map fa_map;
};

struct ieh_config {
	const u16 *ieh_dids;
	int ieh_did_num;
};

struct decoded_res {
	enum severity_level sev;
	struct pci_sbdf sbdf;
};

static struct ieh_dev *gb_ieh;
static LIST_HEAD(ns_ieh_list);
static LIST_HEAD(ss_ieh_list);

/* Tiger Lake-LP SoC IEH device ID */
#define IEH_DID_TGL_LP	0xa0af
/* Tiger Lake-H SoC IEH device ID */
#define IEH_DID_TGL_H	0x43af

static const u16 tgl_ieh_dids[] = {
	IEH_DID_TGL_LP,
	IEH_DID_TGL_H,
};

static struct ieh_config tgl_cfg = {
	.ieh_did_num = ARRAY_SIZE(tgl_ieh_dids),
	.ieh_dids    = tgl_ieh_dids,
};

static const char * const severities[] = {
	[IEH_CO_ERR] = "correctable",
	[IEH_NF_ERR] = "non-fatal uncorrectable",
	[IEH_FA_ERR] = "fatal uncorrectable",
};

static struct irq_work ieh_irq_work;

static int dev_idx(u32 sts, int start)
{
	int i;

	for (i = start; i < 32; i++) {
		if (sts & (1 << i))
			return i;
	}

	return -1;
}

static inline bool notify_by_nmi(void)
{
	return (gb_ieh->co_map == IEH_NMI) || (gb_ieh->nf_map == IEH_NMI) ||
	       (gb_ieh->fa_map == IEH_NMI);
}

static inline bool notify_by_mce(void)
{
	return (gb_ieh->co_map == IEH_MCE) || (gb_ieh->nf_map == IEH_MCE) ||
	       (gb_ieh->fa_map == IEH_MCE);
}

static void ieh_output_error(struct decoded_res *res)
{
	struct pci_sbdf *p = &res->sbdf;

	ieh_printk(KERN_ERR, "Device %04x:%02x:%02x.%x - %s error\n",
		   p->seg, p->bus, p->dev,
		   p->fun, severities[res->sev]);

	/* TODO: Further report error information from the error source */
}

static bool is_same_pdev(struct pci_sbdf *p, struct pci_sbdf *q)
{
	return (p->seg == q->seg && p->bus == q->bus &&
		p->dev == q->dev && p->fun == q->fun);
}

static struct ieh_dev *__get_sat_ieh(struct list_head *ieh_list,
				     struct pci_sbdf *p)
{
	struct ieh_dev *d;

	list_for_each_entry(d, ieh_list, list) {
		if (is_same_pdev(p, &d->sbdf))
			return d;
	}

	return NULL;
}

static struct ieh_dev *get_gb_ieh(struct pci_sbdf *p)
{
	if (is_same_pdev(p, &gb_ieh->sbdf))
		return gb_ieh;

	return NULL;
}

static inline struct ieh_dev *get_ns_sat_ieh(struct pci_sbdf *p)
{
	return __get_sat_ieh(&ns_ieh_list, p);
}

static inline struct ieh_dev *get_ss_sat_ieh(struct pci_sbdf *p)
{
	return __get_sat_ieh(&ss_ieh_list, p);
}

static int ieh_handle_error(struct ieh_dev *d, enum severity_level sev)
{
	struct decoded_res res;
	struct pci_sbdf *p = &res.sbdf;
	struct ieh_dev *sat;
	int i, start = 0;
	u32 sts, reg;

	switch (sev) {
	case IEH_CO_ERR:
		PCI_RD_U32(d->pdev, IEH_G_CE_STS_OFF, &sts);
		/* Write 1s to clear status */
		PCI_WR_U32(d->pdev, IEH_G_CE_STS_OFF, sts);
		break;
	case IEH_NF_ERR:
		PCI_RD_U32(d->pdev, IEH_G_NF_UE_STS_OFF, &sts);
		PCI_WR_U32(d->pdev, IEH_G_NF_UE_STS_OFF, sts);
		break;
	case IEH_FA_ERR:
		PCI_RD_U32(d->pdev, IEH_G_FA_UE_STS_OFF, &sts);
		PCI_WR_U32(d->pdev, IEH_G_FA_UE_STS_OFF, sts);
		break;
	}

	while ((i = dev_idx(sts, start)) != -1) {
		memset(&res, 0, sizeof(res));
		PCI_RD_U32(d->pdev, IEH_DEVFUN_OFF + i * 4, &reg);
		res.sev = sev;
		p->seg	= d->sbdf.seg;
		p->bus	= d->sbdf.bus;
		p->dev	= IEH_DEVFUN_DEV(reg);
		p->fun	= IEH_DEVFUN_FUN(reg);

		switch (d->type) {
		case IEH_GB:
			sat = get_ns_sat_ieh(p);
			if (!sat)
				ieh_output_error(&res);
			else if (sat->type == IEH_NS)
				ieh_handle_error(sat, sev);
			else
				ieh_printk(KERN_ERR, "Invalid IEH1\n");
			break;
		case IEH_NS:
			sat = get_ss_sat_ieh(p);
			if (!sat)
				ieh_output_error(&res);
			else if (sat->type == IEH_SS)
				ieh_handle_error(sat, sev);
			else
				ieh_printk(KERN_ERR, "Invalid IEH2\n");
			break;
		case IEH_SS:
		case IEH_SU:
			ieh_output_error(&res);
			break;
		}

		start = i + 1;
	}

	return 0;
fail:
	return -ENODEV;
}

static void ieh_check_error(void)
{
	struct pci_dev *pdev = gb_ieh->pdev;
	u32 sts;

	PCI_RD_U32(pdev, IEH_G_SYS_EVT_STS_OFF, &sts);

	if (sts & (1 << IEH_FA_ERR))
		ieh_handle_error(gb_ieh, IEH_FA_ERR);

	if (sts & (1 << IEH_NF_ERR))
		ieh_handle_error(gb_ieh, IEH_NF_ERR);

	if (sts & (1 << IEH_CO_ERR))
		ieh_handle_error(gb_ieh, IEH_CO_ERR);

fail:
	return;
}

static void ieh_irq_work_cb(struct irq_work *irq_work)
{
	ieh_check_error();
}

static int ieh_nmi_handler(unsigned int cmd, struct pt_regs *regs)
{
	irq_work_queue(&ieh_irq_work);
	return 0;
}

static int mce_check_error(struct notifier_block *nb, unsigned long val,
			   void *data)
{
	struct mce *mce = (struct mce *)data;
	struct decoded_res res;
	struct pci_sbdf *p = &res.sbdf;
	struct ieh_dev *d;
	u64 rid;

	if (edac_get_report_status() == EDAC_REPORTING_DISABLED)
		return NOTIFY_DONE;

	if ((mce->status & MCACOD) != MCACOD_IOERR)
		return NOTIFY_DONE;

	memset(&res, 0, sizeof(res));
	rid    = MCI_MISC_PCIRID(mce->misc);
	p->seg = MCI_MISC_PCISEG(mce->misc);
	p->bus = GET_BITFIELD(rid, 15, 8);
	p->dev = GET_BITFIELD(rid, 7, 3);
	p->fun = GET_BITFIELD(rid, 2, 0);

	if (mce->status & MCI_STATUS_PCC)
		res.sev = IEH_FA_ERR;
	else if (mce->status & MCI_STATUS_UC)
		res.sev = IEH_NF_ERR;
	else
		res.sev = IEH_CO_ERR;

	if ((d = get_gb_ieh(p)) || (d = get_ns_sat_ieh(p)) ||
	    (d = get_ss_sat_ieh(p)))
		ieh_handle_error(d, res.sev);
	else
		ieh_output_error(&res);

	return NOTIFY_DONE;
}

static struct notifier_block ieh_mce_dec = {
	.notifier_call	= mce_check_error,
	.priority	= MCE_PRIO_EDAC,
};

static const struct x86_cpu_id ieh_cpuids[] = {
	INTEL_CPU_FAM6(TIGERLAKE_L, tgl_cfg),
	{}
};
MODULE_DEVICE_TABLE(x86cpu, ieh_cpuids);

static void __put_ieh(struct ieh_dev *d)
{
	if (!d)
		return;
	if (d->pdev) {
		pci_disable_device(d->pdev);
		pci_dev_put(d->pdev);
	}
	kfree(d);
}

static void __put_iehs(struct list_head *ieh_list)
{
	struct ieh_dev *d, *tmp;

	edac_dbg(0, "\n");

	list_for_each_entry_safe(d, tmp, ieh_list, list) {
		list_del(&d->list);
		__put_ieh(d);
	}
}

static void put_all_iehs(void)
{
	__put_ieh(gb_ieh);
	__put_iehs(&ns_ieh_list);
	__put_iehs(&ss_ieh_list);
}

static int __get_all_iehs(u16 did)
{
	struct pci_dev *pdev, *prev = NULL;
	int rc = -ENODEV, n = 0;
	struct pci_sbdf *p;
	struct ieh_dev *d;
	u32 reg;

	edac_dbg(0, "\n");

	for (;;) {
		pdev = pci_get_device(PCI_VENDOR_ID_INTEL, did, prev);
		if (!pdev)
			break;

		if (pci_enable_device(pdev)) {
			ieh_printk(KERN_ERR, "Failed to enable %04x:%04x\n",
				   pdev->vendor, pdev->device);
			goto fail3;
		}

		d = kzalloc(sizeof(*d), GFP_KERNEL);
		if (!d) {
			rc = -ENOMEM;
			goto fail2;
		}

		PCI_RD_U32(pdev, IEH_TYPEVER_OFF, &reg);
		d->pdev = pdev;
		d->ver  = IEH_TYPEVER_VER(reg);
		d->type = IEH_TYPEVER_TYP(reg);
		p	= &d->sbdf;
		p->seg  = pci_domain_nr(pdev->bus);
		p->bus  = IEH_TYPEVER_BUS(reg);
		p->dev  = PCI_SLOT(pdev->devfn);
		p->fun  = PCI_FUNC(pdev->devfn);

		if (p->bus != pdev->bus->number) {
			ieh_printk(KERN_ERR, "Mismatched IEH bus\n");
			rc = -EINVAL;
			goto fail;
		}

		switch (d->type) {
		case IEH_GB:
		case IEH_SU:
			if (gb_ieh) {
				ieh_printk(KERN_ERR, "Double global IEHs\n");
				rc = -EINVAL;
				goto fail;
			}
			gb_ieh = d;

			PCI_RD_U32(pdev, IEH_G_SYS_EVT_MAP_OFF, &reg);
			d->co_map = IEH_G_SYS_EVT_MAP_CO(reg);
			d->nf_map = IEH_G_SYS_EVT_MAP_NF(reg);
			d->fa_map = IEH_G_SYS_EVT_MAP_FA(reg);
			ieh_printk(KERN_DEBUG, "GB IEH %02x:%02x.%x\n",
				   p->bus, p->dev, p->fun);
			break;
		case IEH_NS:
			list_add_tail(&d->list, &ns_ieh_list);
			ieh_printk(KERN_DEBUG, "NS IEH %02x:%02x.%x\n",
				   p->bus, p->dev, p->fun);
			break;
		case IEH_SS:
			list_add_tail(&d->list, &ss_ieh_list);
			ieh_printk(KERN_DEBUG, "SS IEH %02x:%02x.%x\n",
				   p->bus, p->dev, p->fun);
			break;
		}

		prev = pdev;
		n++;
	}

	return n;
fail:
	kfree(d);
fail2:
	pci_disable_device(pdev);
fail3:
	pci_dev_put(pdev);
	put_all_iehs();
	return rc;
}

static int get_all_iehs(struct ieh_config *cfg)
{
	int i, rc, n = 0;

	for (i = 0; i < cfg->ieh_did_num; i++) {
		rc = __get_all_iehs(cfg->ieh_dids[i]);
		if (rc < 0)
			return rc;
		n += rc;
	}

	if (n == 0) {
		ieh_printk(KERN_DEBUG, "No IEHs found\n");
		return -ENODEV;
	}

	if (!gb_ieh) {
		ieh_printk(KERN_ERR, "No global IEH found\n");
		put_all_iehs();
		return -ENODEV;
	}

	return 0;
}

static int register_err_handler(void)
{
	int rc;

	if (notify_by_nmi()) {
		init_irq_work(&ieh_irq_work, ieh_irq_work_cb);
		rc = register_nmi_handler(NMI_SERR, ieh_nmi_handler,
					  0, IEH_NMI_NAME);
		if (rc) {
			ieh_printk(KERN_ERR, "Can't register NMI handler\n");
			return rc;
		}
	} else if (notify_by_mce()) {
		mce_register_decode_chain(&ieh_mce_dec);
	} else {
		ieh_printk(KERN_INFO, "No OS-visible IEH events\n");
		return -ENODEV;
	}

	return 0;
}

static void unregister_err_handler(void)
{
	if (notify_by_nmi()) {
		unregister_nmi_handler(NMI_SERR, IEH_NMI_NAME);
		irq_work_sync(&ieh_irq_work);
	} else if (notify_by_mce()) {
		mce_unregister_decode_chain(&ieh_mce_dec);
	}
}

static int __init ieh_init(void)
{
	const struct x86_cpu_id *id;
	struct ieh_config *cfg;
	int rc;

	edac_dbg(2, "\n");

	id = x86_match_cpu(ieh_cpuids);
	if (!id)
		return -ENODEV;
	cfg = (struct ieh_config *)id->driver_data;

	rc = get_all_iehs(cfg);
	if (rc)
		return rc;

	rc = register_err_handler();
	if (rc) {
		put_all_iehs();
		return rc;
	}

	ieh_printk(KERN_INFO, "hw v%d, drv %s\n", gb_ieh->ver, IEH_REVISION);
	return 0;
}

static void __exit ieh_exit(void)
{
	edac_dbg(2, "\n");

	unregister_err_handler();
	put_all_iehs();
}

module_init(ieh_init);
module_exit(ieh_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qiuxu Zhuo");
MODULE_DESCRIPTION("IEH Driver for Intel CPU using I/O IEH");
