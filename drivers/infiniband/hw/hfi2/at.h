/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a GPLv2 license.  When using or
 * redistributing this file, you must do so under such license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/*
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

/* AT: Address Translation */

#ifndef _AT_H
#define _AT_H

/*
 * AT register specification per version 3.0
 */
#define	AT_VER_REG	0x0	/* Arch version supported by this AT */
#define	AT_CAP_REG	0x8	/* Hardware supported capabilities */
#define	AT_ECAP_REG	0x10	/* Extended capabilities supported */
#define	AT_GCMD_REG	0x18	/* Global command register */
#define	AT_GSTS_REG	0x1c	/* Global status register */
#define	AT_RTADDR_REG	0x20	/* Root entry table */
#define	AT_CCMD_REG	0x28	/* Context command reg */
#define	AT_FSTS_REG	0x34	/* Fault Status register */
#define	AT_FECTL_REG	0x38	/* Fault control register */
#define	AT_FEDATA_REG	0x3c	/* Fault event interrupt data register */
#define	AT_FEADDR_REG	0x40	/* Fault event interrupt addr register */
#define	AT_FEUADDR_REG	0x44	/* Upper address register */
#define	AT_AFLOG_REG	0x58	/* Advanced Fault control */
#define	AT_PMEN_REG	0x64	/* Enable Protected Memory Region */
#define	AT_PLMBASE_REG	0x68	/* PMRR Low addr */
#define	AT_PLMLIMIT_REG 0x6c	/* PMRR low limit */
#define	AT_PHMBASE_REG	0x70	/* pmrr high base addr */
#define	AT_PHMLIMIT_REG 0x78	/* pmrr high limit */
#define AT_IQH_REG	0x80	/* Invalidation queue head register */
#define AT_IQT_REG	0x88	/* Invalidation queue tail register */
#define AT_IQ_SHIFT	4	/* Invalidation queue head/tail shift */
#define AT_IQA_REG	0x90	/* Invalidation queue addr register */
#define AT_ICS_REG	0x9c	/* Invalidation complete status register */
#define AT_IRTA_REG	0xb8    /* Interrupt remapping table addr register */
#define AT_PQH_REG	0xc0	/* Page request queue head register */
#define AT_PQT_REG	0xc8	/* Page request queue tail register */
#define AT_PQA_REG	0xd0	/* Page request queue address register */
#define AT_PRS_REG	0xdc	/* Page request status register */
#define AT_PECTL_REG	0xe0	/* Page request event control register */
#define	AT_PEDATA_REG	0xe4	/* Page request event interrupt data register */
#define	AT_PEADDR_REG	0xe8	/* Page request event interrupt addr register */
#define	AT_PEUADDR_REG	0xec	/* Page request event Upper address register */

#define OFFSET_STRIDE		(9)

#define at_readq(a)		readq(a)
#define at_writeq(a, v)		writeq(v, a)

#define AT_VER_MAJOR(v)		(((v) & 0xf0) >> 4)
#define AT_VER_MINOR(v)		((v) & 0x0f)

/*
 * Decoding Capability Register
 */
#define cap_pi_support(c)	(((c) >> 59) & 1)
#define cap_read_drain(c)	(((c) >> 55) & 1)
#define cap_write_drain(c)	(((c) >> 54) & 1)
#define cap_max_amask_val(c)	(((c) >> 48) & 0x3f)
#define cap_num_fault_regs(c)	((((c) >> 40) & 0xff) + 1)
#define cap_pgsel_inv(c)	(((c) >> 39) & 1)

#define cap_super_page_val(c)	(((c) >> 34) & 0xf)
#define cap_super_offset(c)	(((find_first_bit(&cap_super_page_val(c), 4)) \
					* OFFSET_STRIDE) + 21)

#define cap_fault_reg_offset(c)	((((c) >> 24) & 0x3ff) * 16)
#define cap_max_fault_reg_offset(c) \
				(cap_fault_reg_offset(c) \
					+ cap_num_fault_regs(c) * 16)

#define cap_zlr(c)		(((c) >> 22) & 1)
#define cap_isoch(c)		(((c) >> 23) & 1)
#define cap_mgaw(c)		((((c) >> 16) & 0x3f) + 1)
#define cap_sagaw(c)		(((c) >> 8) & 0x1f)
#define cap_caching_mode(c)	(((c) >> 7) & 1)
#define cap_phmr(c)		(((c) >> 6) & 1)
#define cap_plmr(c)		(((c) >> 5) & 1)
#define cap_rwbf(c)		(((c) >> 4) & 1)
#define cap_afl(c)		(((c) >> 3) & 1)
#define cap_ndoms(c)		(1 << (4 + 2 * ((c) & 0x7)))

/*
 * Extended Capability Register
 */
#define ecap_dit(e)		((e >> 41) & 0x1)
#define ecap_pasid(e)		((e >> 40) & 0x1)
#define ecap_pss(e)		((e >> 35) & 0x1f)
#define ecap_eafs(e)		((e >> 34) & 0x1)
#define ecap_nwfs(e)		((e >> 33) & 0x1)
#define ecap_srs(e)		((e >> 31) & 0x1)
#define ecap_ers(e)		((e >> 30) & 0x1)
#define ecap_prs(e)		((e >> 29) & 0x1)
#define ecap_broken_pasid(e)	((e >> 28) & 0x1)
#define ecap_dis(e)		((e >> 27) & 0x1)
#define ecap_nest(e)		((e >> 26) & 0x1)
#define ecap_mts(e)		((e >> 25) & 0x1)
#define ecap_ecs(e)		((e >> 24) & 0x1)
#define ecap_iotlb_offset(e)	((((e) >> 8) & 0x3ff) * 16)
#define ecap_max_iotlb_offset(e) (ecap_iotlb_offset(e) + 16)
#define ecap_coherent(e)	((e) & 0x1)
#define ecap_qis(e)		((e) & 0x2)
#define ecap_pass_through(e)	((e >> 6) & 0x1)
#define ecap_eim_support(e)	((e >> 4) & 0x1)
#define ecap_ir_support(e)	((e >> 3) & 0x1)
#define ecap_dev_iotlb_support(e)	(((e) >> 2) & 0x1)
#define ecap_max_handle_mask(e) ((e >> 20) & 0xf)
#define ecap_sc_support(e)	((e >> 7) & 0x1) /* Snooping Control */

/* IOTLB_REG */
#define AT_TLB_FLUSH_GRANU_OFFSET  60
#define AT_TLB_GLOBAL_FLUSH	(((u64)1) << 60)
#define AT_TLB_DSI_FLUSH	(((u64)2) << 60)
#define AT_TLB_PSI_FLUSH	(((u64)3) << 60)
#define AT_TLB_IIRG(type)	((type >> 60) & 3)
#define AT_TLB_IAIG(val)	(((val) >> 57) & 3)
#define AT_TLB_READ_DRAIN	(((u64)1) << 49)
#define AT_TLB_WRITE_DRAIN	(((u64)1) << 48)
#define AT_TLB_DID(id)		(((u64)((id) & 0xffff)) << 32)
#define AT_TLB_IVT		(((u64)1) << 63)
#define AT_TLB_IH_NONLEAF	(((u64)1) << 6)
#define AT_TLB_MAX_SIZE	(0x3f)

/* INVALID_DESC */
#define AT_CCMD_INVL_GRANU_OFFSET  61
#define AT_ID_TLB_GLOBAL_FLUSH	(((u64)1) << 4)
#define AT_ID_TLB_DSI_FLUSH	(((u64)2) << 4)
#define AT_ID_TLB_PSI_FLUSH	(((u64)3) << 4)
#define AT_ID_TLB_READ_DRAIN	(((u64)1) << 7)
#define AT_ID_TLB_WRITE_DRAIN	(((u64)1) << 6)
#define AT_ID_TLB_DID(id)	(((u64)((id & 0xffff) << 16)))
#define AT_ID_TLB_IH_NONLEAF	(((u64)1) << 6)
#define AT_ID_TLB_ADDR(addr)	(addr)
#define AT_ID_TLB_ADDR_MASK(mask)  (mask)

/* PMEN_REG */
#define AT_PMEN_EPM		(((u32)1) << 31)
#define AT_PMEN_PRS		(((u32)1) << 0)

/* GCMD_REG */
#define AT_GCMD_TE		(((u32)1) << 31)
#define AT_GCMD_SRTP		(((u32)1) << 30)
#define AT_GCMD_SFL		(((u32)1) << 29)
#define AT_GCMD_EAFL		(((u32)1) << 28)
#define AT_GCMD_WBF		(((u32)1) << 27)
#define AT_GCMD_QIE		(((u32)1) << 26)
#define AT_GCMD_SIRTP		(((u32)1) << 24)
#define AT_GCMD_IRE		(((u32)1) << 25)
#define AT_GCMD_CFI		(((u32)1) << 23)

/* GSTS_REG */
#define AT_GSTS_TES		(((u32)1) << 31)
#define AT_GSTS_RTPS		(((u32)1) << 30)
#define AT_GSTS_FLS		(((u32)1) << 29)
#define AT_GSTS_AFLS		(((u32)1) << 28)
#define AT_GSTS_WBFS		(((u32)1) << 27)
#define AT_GSTS_QIES		(((u32)1) << 26)
#define AT_GSTS_IRTPS		(((u32)1) << 24)
#define AT_GSTS_IRES		(((u32)1) << 25)
#define AT_GSTS_CFIS		(((u32)1) << 23)

/* RTADDR_REG */
#define AT_RTADDR_RTT		(((u64)1) << 11)

/* CCMD_REG */
#define AT_CCMD_ICC		(((u64)1) << 63)
#define AT_CCMD_GLOBAL_INVL	(((u64)1) << 61)
#define AT_CCMD_DOMAIN_INVL	(((u64)2) << 61)
#define AT_CCMD_DEVICE_INVL	(((u64)3) << 61)
#define AT_CCMD_FM(m)		(((u64)((m) & 0x3)) << 32)
#define AT_CCMD_MASK_NOBIT	0
#define AT_CCMD_MASK_1BIT	1
#define AT_CCMD_MASK_2BIT	2
#define AT_CCMD_MASK_3BIT	3
#define AT_CCMD_SID(s)		(((u64)((s) & 0xffff)) << 16)
#define AT_CCMD_DID(d)		((u64)((d) & 0xffff))

/* FECTL_REG */
#define AT_FECTL_IM		(((u32)1) << 31)

/* FSTS_REG */
#define AT_FSTS_PPF		((u32)2)
#define AT_FSTS_PFO		((u32)1)
#define AT_FSTS_IQE		BIT(4)
#define AT_FSTS_ICE		BIT(5)
#define AT_FSTS_ITE		BIT(6)
#define at_fsts_fault_record_index(s) (((s) >> 8) & 0xff)

/* FRCD_REG, 32 bits access */
#define AT_FRCD_F		(((u32)1) << 31)
#define at_frcd_type(d)		((d >> 30) & 1)
#define at_frcd_fault_reason(c)	(c & 0xff)
#define at_frcd_source_id(c)	(c & 0xffff)
/* low 64 bit */
#define at_frcd_page_addr(d)	(d & (((u64)-1) << PAGE_SHIFT))

/* PRS_REG */
#define AT_PRS_PPR		((u32)1)

/* 10 seconds */
#define AT_OPERATION_TIMEOUT	((cycles_t)tsc_khz * 10 * 1000)

#define AT_WAIT_OP(at, offset, op, cond, sts)			\
do {									\
	cycles_t start_time = get_cycles();				\
	while (1) {							\
		sts = op(at->reg + offset);				\
		if (cond)						\
			break;						\
		if (AT_OPERATION_TIMEOUT < (get_cycles() - start_time))\
			panic("AT hardware is malfunctioning\n");	\
		cpu_relax();						\
	}								\
} while (0)

/*
 * AT hardware uses 4KiB page size regardless of host page size.
 */
#define AT_PAGE_SHIFT		(12)
#define AT_PAGE_SIZE		BIT(AT_PAGE_SHIFT)
#define AT_PAGE_MASK		(((u64)-1) << AT_PAGE_SHIFT)
#define AT_PAGE_ALIGN(addr)	(((addr) + AT_PAGE_SIZE - 1) & AT_PAGE_MASK)

#define AT_STRIDE_SHIFT		(9)
#define AT_STRIDE_MASK		(((u64)-1) << AT_STRIDE_SHIFT)

#define AT_PTE_PRESENT    BIT(0)
#define AT_PTE_WRITE      BIT(1)
#define AT_PTE_USER       BIT(2)
#define AT_PTE_ACCESSED   BIT(5)
#define AT_PTE_DIRTY      BIT(6)
#define AT_PTE_LARGE_PAGE BIT(7)
#define AT_PTE_GLOBAL     BIT(8)
#define AT_PTE_EXEC_DIS   BIT(63)

#define CONTEXT_TT_MULTI_LEVEL	0
#define CONTEXT_TT_DEV_IOTLB	1
#define CONTEXT_TT_PASS_THROUGH 2
/* Extended context entry types */
#define CONTEXT_TT_PT_PASID	4
#define CONTEXT_TT_PT_PASID_DEV_IOTLB 5
#define CONTEXT_TT_MASK		(7ULL << 2)

#define CONTEXT_DINVE		BIT(8)
#define CONTEXT_PRS		BIT(9)
#define CONTEXT_NESTE		BIT(10)
#define CONTEXT_PASIDE		BIT(11)

/* Queued Invalidation */
#define QI_LENGTH		256	/* queue length */

enum {
	QI_FREE,
	QI_IN_USE,
	QI_DONE,
	QI_ABORT
};

#define QI_CC_TYPE		0x1
#define QI_IOTLB_TYPE		0x2
#define QI_DIOTLB_TYPE		0x3
#define QI_IEC_TYPE		0x4
#define QI_IWD_TYPE		0x5
#define QI_EIOTLB_TYPE		0x6
#define QI_PC_TYPE		0x7
#define QI_DEIOTLB_TYPE		0x8
#define QI_PGRP_RESP_TYPE	0x9
#define QI_PSTRM_RESP_TYPE	0xa

#define QI_DID(did)		(((u64)did & 0xffff) << 16)
#define QI_DID_MASK		GENMASK(31, 16)
#define QI_TYPE_MASK		GENMASK(3, 0)

#define QI_IEC_SELECTIVE	(((u64)1) << 4)
#define QI_IEC_IIDEX(idx)	(((u64)(idx & 0xffff) << 32))
#define QI_IEC_IM(m)		(((u64)(m & 0x1f) << 27))

#define QI_IWD_STATUS_DATA(d)	(((u64)d) << 32)
#define QI_IWD_STATUS_WRITE	(((u64)1) << 5)

#define QI_IOTLB_DID(did)	(((u64)did) << 16)
#define QI_IOTLB_DR(dr)		(((u64)dr) << 7)
#define QI_IOTLB_DW(dw)		(((u64)dw) << 6)
#define QI_IOTLB_GRAN(gran)	(((u64)gran) >> (AT_TLB_FLUSH_GRANU_OFFSET - 4))
#define QI_IOTLB_ADDR(addr)	(((u64)addr) & AT_PAGE_MASK)
#define QI_IOTLB_IH(ih)		(((u64)ih) << 6)
#define QI_IOTLB_AM(am)		(((u8)am))

#define QI_CC_FM(fm)		(((u64)fm) << 48)
#define QI_CC_SID(sid)		(((u64)sid) << 32)
#define QI_CC_DID(did)		(((u64)did) << 16)
#define QI_CC_GRAN(gran)	(((u64)gran) >> (AT_CCMD_INVL_GRANU_OFFSET - 4))

#define QI_DEV_IOTLB_SID(sid)	((u64)((sid) & 0xffff) << 32)
#define QI_DEV_IOTLB_QDEP(qdep)	(((qdep) & 0x1f) << 16)
#define QI_DEV_IOTLB_ADDR(addr)	((u64)(addr) & AT_PAGE_MASK)
#define QI_DEV_IOTLB_PFSID(pfsid) (((u64)(pfsid & 0xf) << 12) | \
				  ((u64)(pfsid & 0xff0) << 48))
#define QI_DEV_IOTLB_SIZE	1
#define QI_DEV_IOTLB_MAX_INVS	32

#define QI_PC_PASID(pasid)	(((u64)pasid) << 32)
#define QI_PC_DID(did)		(((u64)did) << 16)
#define QI_PC_GRAN(gran)	(((u64)gran) << 4)

/* PC inv granu */
#define QI_PC_ALL_PASIDS	0
#define QI_PC_PASID_SEL		1

#define QI_EIOTLB_ADDR(addr)	((u64)(addr) & AT_PAGE_MASK)
#define QI_EIOTLB_GL(gl)	(((u64)gl) << 7)
#define QI_EIOTLB_IH(ih)	(((u64)ih) << 6)
#define QI_EIOTLB_AM(am)	(((u64)am))
#define QI_EIOTLB_PASID(pasid)	(((u64)pasid) << 32)
#define QI_EIOTLB_DID(did)	(((u64)did) << 16)
#define QI_EIOTLB_GRAN(gran)	(((u64)gran) << 4)

/* QI Dev-IOTLB inv granu */
#define QI_DEV_IOTLB_GRAN_ALL	0
#define QI_DEV_IOTLB_GRAN_PASID_SEL  1

#define QI_DEV_EIOTLB_ADDR(a)	((u64)(a) & AT_PAGE_MASK)
#define QI_DEV_EIOTLB_SIZE	(((u64)1) << 11)
#define QI_DEV_EIOTLB_GLOB(g)	((u64)g)
#define QI_DEV_EIOTLB_PASID(p)	(((u64)p) << 32)
#define QI_DEV_EIOTLB_SID(sid)	((u64)((sid) & 0xffff) << 16)
#define QI_DEV_EIOTLB_QDEP(qd)	((u64)((qd) & 0x1f) << 4)
#define QI_DEV_EIOTLB_PFSID(pfsid) (((u64)(pfsid & 0xf) << 12) | \
				   ((u64)(pfsid & 0xff0) << 48))
#define QI_DEV_EIOTLB_MAX_INVS	32

#define QI_PGRP_IDX(idx)	(((u64)(idx)) << 55)
#define QI_PGRP_PRIV(priv)	(((u64)(priv)) << 32)
#define QI_PGRP_RESP_CODE(res)	((u64)(res))
#define QI_PGRP_PASID(pasid)	(((u64)(pasid)) << 32)
#define QI_PGRP_DID(did)	(((u64)(did)) << 16)
#define QI_PGRP_PASID_P(p)	(((u64)(p)) << 4)

#define QI_PSTRM_ADDR(addr)	(((u64)(addr)) & AT_PAGE_MASK)
#define QI_PSTRM_DEVFN(devfn)	(((u64)(devfn)) << 4)
#define QI_PSTRM_RESP_CODE(res)	((u64)(res))
#define QI_PSTRM_IDX(idx)	(((u64)(idx)) << 55)
#define QI_PSTRM_PRIV(priv)	(((u64)(priv)) << 32)
#define QI_PSTRM_BUS(bus)	(((u64)(bus)) << 24)
#define QI_PSTRM_PASID(pasid)	(((u64)(pasid)) << 4)

#define QI_RESP_SUCCESS		0x0
#define QI_RESP_INVALID		0x1
#define QI_RESP_FAILURE		0xf

/* QI EIOTLB inv granu */
#define QI_GRAN_ALL_ALL		0
#define QI_GRAN_NONG_ALL	1
#define QI_GRAN_NONG_PASID	2
#define QI_GRAN_PSI_PASID	3

struct qi_desc {
	u64 low, high;
};

struct q_inval {
	raw_spinlock_t   q_lock;
	struct qi_desc  *desc;            /* invalidation queue */
	dma_addr_t       desc_dma;        /* invalidation queue dma address */
	int             *desc_status;     /* desc status */
	dma_addr_t       desc_status_dma; /* desc status dma address */
	int              free_head;       /* first free entry */
	int              free_tail;       /* last free entry */
	int              free_cnt;
};

enum {
	SR_AT_FECTL_REG,
	SR_AT_FEDATA_REG,
	SR_AT_FEADDR_REG,
	SR_AT_FEUADDR_REG,
	MAX_SR_AT_REGS
};

#define AT_FLAG_TRANS_PRE_ENABLED	BIT(0)
#define AT_FLAG_IRQ_REMAP_PRE_ENABLED	BIT(1)

/* address width */
#define DEFAULT_ADDRESS_WIDTH		48
#define MAX_PGTBL_LEVEL			5

/* page table handling */
#define LEVEL_STRIDE			(9)
#define LEVEL_MASK			(((u64)1 << LEVEL_STRIDE) - 1)

#define ROOT_SIZE			AT_PAGE_SIZE
#define CONTEXT_SIZE			AT_PAGE_SIZE

/* page request queue size/order */
#define PRQ_ORDER			0

#define HFI_ATS_MAX_QDEP		0x20

struct pasid_entry {
	u64 val;
};

struct pasid_state_entry {
	u64 val;
};

/* Page request queue descriptor */
struct page_req_dsc {
	u64 srr:1;
	u64 bof:1;
	u64 pasid_present:1;
	u64 lpig:1;
	u64 pasid:20;
	u64 bus:8;
	u64 private:23;
	u64 prg_index:9;
	u64 rd_req:1;
	u64 wr_req:1;
	u64 exe_req:1;
	u64 priv_req:1;
	u64 devfn:8;
	u64 addr:52;
};

/*
 * 0: Present
 * 1-11: Reserved
 * 12-63: Context Ptr (12 - (haw-1))
 * 64-127: Reserved
 */
struct root_entry {
	u64	lo;
	u64	hi;
};

/*
 * low 64 bits:
 * 0: present
 * 1: fault processing disable
 * 2-3: translation type
 * 12-63: address space root
 * high 64 bits:
 * 0-2: address width
 * 3-6: aval
 * 8-23: domain id
 */
struct context_entry {
	u64 lo;
	u64 hi;
};

struct hfi_at_stats {
	int pasid;
	u64 preg;
	u64 preg_fail;
	u64 prq;
	u64 prq_dup;
	u64 prq_fail;
};

struct hfi_at {
	void __iomem	*reg; /* Pointer to hardware regs, virtual addr */
	u64		cap;
	u64		ecap;
	u32		gcmd;
	raw_spinlock_t	register_lock; /* protect register handling */
	int		seq_id;	/* sequence id of the AT */
	int		agaw; /* agaw of this AT */
	unsigned int	irq, pr_irq;
	unsigned char	name[13];    /* Device Name */

	spinlock_t	lock; /* protect context */
	struct root_entry *root_entry; /* virtual address */
	dma_addr_t root_entry_dma;     /* dma address */
	struct context_entry *context;
	dma_addr_t context_dma;
	struct q_inval  *qi;            /* Queued invalidation info */
	struct page_req_dsc *prq;
	dma_addr_t prq_dma;
	unsigned char prq_name[16];    /* Name for PRQ interrupt */

	/* pasid tables */
	struct pasid_entry *pasid_table;
	dma_addr_t pasid_table_dma;
	struct idr pasid_idr;
	struct idr pasid_stats_idr;
	u32 pasid_max;

	/* mm for system pasid */
	struct mm_struct *system_mm;

	/* FXR device info */
	u8 bus;			/* PCI bus number */
	u8 devfn;		/* PCI devfn number */
	u8 ats_qdep;

	struct hfi_devdata *dd; /* for backward reference */

	/* temp threads */
	struct task_struct *prq_thread;
	struct task_struct *fault_thread;
};

#define SVM_FLAG_PRIVATE_PASID		BIT(0)
#define SVM_FLAG_SUPERVISOR_MODE	BIT(1)

struct at_pte {
	u64 val;
};

struct hfi_at_svm {
	struct mmu_notifier notifier;
	struct mm_struct *mm;
	struct hfi_at *at;
	int pasid;
	/*
	 * flags passed in with hfi_svm_bind_mm() call,
	 * currently support above two values.
	 */
	int flags;

	int users;
	u16 did, sid, qdep;

	struct at_pte *pgd;
	struct task_struct *tsk;
	spinlock_t lock; /* protect pgd access */
	struct hfi_at_stats *stats;
};

/*
 * irq vector for at
 */
#define HFI_AT_PRQ_IRQ			256
#define HFI_AT_FAULT_IRQ		273

#endif
