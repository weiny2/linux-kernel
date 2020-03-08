/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
 * Copyright(c) 2016-2020 Intel Corporation
 */

#ifndef __DLB_HW_TYPES_H
#define __DLB_HW_TYPES_H

#include <linux/io.h>

#define DLB_PCI_REG_READ(addr)        ioread32(addr)
#define DLB_PCI_REG_WRITE(reg, value) iowrite32(value, reg)

/* Read/write register 'reg' in the CSR BAR space */
#define DLB_CSR_REG_ADDR(a, reg) ((a)->csr_kva + (reg))
#define DLB_CSR_RD(hw, reg) \
	DLB_PCI_REG_READ(DLB_CSR_REG_ADDR((hw), (reg)))
#define DLB_CSR_WR(hw, reg, value) \
	DLB_PCI_REG_WRITE(DLB_CSR_REG_ADDR((hw), (reg)), (value))

/* Read/write register 'reg' in the func BAR space */
#define DLB_FUNC_REG_ADDR(a, reg) ((a)->func_kva + (reg))
#define DLB_FUNC_RD(hw, reg) \
	DLB_PCI_REG_READ(DLB_FUNC_REG_ADDR((hw), (reg)))
#define DLB_FUNC_WR(hw, reg, value) \
	DLB_PCI_REG_WRITE(DLB_FUNC_REG_ADDR((hw), (reg)), (value))

#define DLB_MAX_NUM_VFS 16
#define DLB_MAX_NUM_DOMAINS 32
#define DLB_MAX_NUM_LDB_QUEUES 128
#define DLB_MAX_NUM_LDB_PORTS 64
#define DLB_MAX_NUM_DIR_PORTS 128
#define DLB_MAX_NUM_LDB_CREDITS 16384
#define DLB_MAX_NUM_DIR_CREDITS 4096
#define DLB_MAX_NUM_LDB_CREDIT_POOLS 64
#define DLB_MAX_NUM_DIR_CREDIT_POOLS 64
#define DLB_MAX_NUM_HIST_LIST_ENTRIES 5120
#define DLB_MAX_NUM_AQOS_ENTRIES 2048
#define DLB_MAX_NUM_TOTAL_OUTSTANDING_COMPLETIONS 4096
#define DLB_MAX_NUM_QIDS_PER_LDB_CQ 8
#define DLB_MAX_NUM_SEQUENCE_NUMBER_GROUPS 4
#define DLB_MAX_NUM_SEQUENCE_NUMBER_MODES 6
#define DLB_QID_PRIORITIES 8
#define DLB_NUM_ARB_WEIGHTS 8
#define DLB_MAX_WEIGHT 255
#define DLB_MAX_PORT_CREDIT_QUANTUM 1023
#define DLB_MAX_CQ_COMP_CHECK_LOOPS 409600
#define DLB_MAX_QID_EMPTY_CHECK_LOOPS (32 * 64 * 1024 * (800 / 30))
#define DLB_HZ 800000000

struct dlb_hw {
	/* BAR 0 address */
	void __iomem *csr_kva;
	unsigned long csr_phys_addr;
	/* BAR 2 address */
	void __iomem *func_kva;
	unsigned long func_phys_addr;
};

#endif /* __DLB_HW_TYPES_H */
