/*
 * Copyright 2013 Intel Corporation All Rights Reserved.
 *
 * INTEL CONFIDENTIAL
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material may contain trade secrets and
 * proprietary and confidential information of Intel Corporation and its
 * suppliers and licensors, and is protected by worldwide copyright and trade
 * secret laws and treaty provisions. No part of the Material may be used,
 * copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express written
 * permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or alter
 * this notice or any other notice embedded in Materials by Intel or Intel's
 * suppliers or licensors in any way.
 */

#ifndef _NVDIMM_ACPI_H
#define _NVDIMM_ACPI_H

#include <linux/types.h>

#define NVDIMM_PM_RNG_TYPE 1
#define NVDIMM_CRTL_RNG_TYPE 2
#define NVDIMM_DATA_RNG_TYPE 3

#define NVDIMM_SIG 609043789

/* Generic NVDIMM FIT R0_7 */
struct spa_rng_tbl {
	/*
	 * Address Range Type
	 * 1: NVDIMM(Persistent Memory)
	 * 2: NVDIMM Control Region (Contains Mailboxes)
	 * 3: NVDIMM Data Region (Contains Aperture?)
	 */
	__u16 addr_rng_type;
	/* used by memdev_spa_rng_tbl to uniquely refer to this table */
	__u16 spa_index;
	__u32 reserved;
	/* start address of the system physical address range */
	__u64 start_addr;
	__u64 length; /* Size of the region in bytes */
} __packed;

/* Generic NVDIMM FIT R0_7 */
struct memdev_spa_rng_tbl {
	/*
	 * refers to the socket to which this DIMM is attached via memory
	 * controller. Socket ID is a logic id that corresponds to a TBD table
	 */
	__u16 socket_id;
	/*
	 * refers to a memory controller that this DIMM belongs to for the
	 * given socket. The ID here is a logical ID corresponding to a TBD
	 * table
	 */
	__u16 mem_ctrl_id;
	/* SMBIOS Type 17 handle corresponding to this memory device */
	__u16 mem_dev_pid;
	/* Unique ID to refer to regions in a memory device. */
	__u16 mem_dev_lid;
	/* Refers to corresponding SPA range description table entry */
	__u16 spa_index;
	__u16 vendor_id; /* To allow loading of vendor specific driver */
	__u16 device_id; /* To allow vendor to comprehend multiple devices */
	__u16 rid; /* Revision ID */
	/*
	 * Format interface code: Allows vendor hardware to be handled by a
	 * generic driver (behaves similar to class code in PCI)
	 */
	__u16 fmt_interface_code;
	__u8 reserved[6];
	/* Size in bytes of the address range for this logical ID */
	__u64 length;
	/*
	 * In case of multiple regions of the same type within the device,
	 * provides offset of this region in bytes
	 */
	__u64 region_offset;
	/*
	 * This start spa will be contained within the range of the
	 * corresponding spa range description table entry
	 */
	__u64 start_spa;
	/*
	 * Refers to the interleave description that should be used for the
	 * range described by this structure. Value of 0 indicates that this
	 * range is not interleaved.
	 */
	__u16 interleave_idx;
	__u8 interleave_ways;
} __packed;

/* Generic NVDIMM FIT R0_7 */
struct interleave_tbl {
	/*
	 * Index number uniquely identifies the interleave description - this
	 * allows the reuse of interleave description across multiple memory
	 * devices.  Index must be non-zero.
	 */
	__u16 interleave_idx;
	__u16 reserved;
	/*
	 * table size in bytes. This includes all the bytes of the table
	 * including the header
	 */
	__u32 tbl_size;
	/*
	 * Only need to describe the number of lines needed before the
	 * interleave pattern repeats
	 */
	__u32 lines_described;
	__u32 line_size; /* Bytes, 64,128,256,4K */
	__u32 *offsets; /* line offsets */
} __packed;

/* Generic NVDIMM FIT R0_7 */
struct flush_hint_tbl {
	/*
	 * refers to the socket to which this DIMM is attached via memory
	 * controller. Socket ID is a logic id that corresponds to a TBD table
	 */
	__u16 socket_id;
	/*
	 * refers to a memory controller that this DIMM belongs to for the
	 * given socket. The ID here is a logical ID corresponding to a TBD
	 * table
	 */
	__u16 mem_ctrl_id;
	/*
	 * max number of flush hint addresses in this table entry. Max number
	 * of flush hint addresses has to be the same for all memory
	 * controllers for a given system.
	 */
	__u16 max_flush_hint_addrs;
	/*
	 * Indicates consecutive flush hint addresses that are valid in this
	 * table entry
	 */
	__u16 num_valid_flush_hint_addrs;
	/*
	 * 64 bit address that needs to be written in order to cause a
	 * durability flush. A value of all F indicates that the entry is not
	 * valid.
	 */
	__u64 *flush_hint_addr;
} __packed;

/* Generic NVDIMM FIT R0_7 */
struct nvdimm_fit {
	__u32 signature; /* '$MEM' is signature for this table */
	__u32 length; /* Length in bytes of the entire table */
	__u8 revision; /* 1 */
	__u8 checksum; /* entire table sums to 0 */
	__u8 oemid[6];	/* OEM ID */
	__u64 oem_tbl_id; /* Manufacturer model ID */
	__u32 oem_revision; /* OEM revision */
	/* VendorId of the utility that created the table */
	__u32 creator_id;
	/* revision of the utility that created the table */
	__u32 creator_revision;
	/* number of system physical address description tables */
	__u16 num_spa_range_tbl;
	/* number of memory device to system address range mapping tables */
	__u16 num_memdev_spa_range_tbl;
	__u16 num_flush_hint_tbl; /* number of flush hint address tables */
	__u16 num_interleave_tbl; /* number of interleave description tables */
	/*
	 * size in bytes of flush hint address table entry for a given
	 * platform. All entries in the table are the same size
	 */
	__u16 size_flush_hint_tbl;
	__u16 reserved;
} __packed;

/* Generic */
struct fit_header {
	struct nvdimm_fit *fit;
	struct spa_rng_tbl *spa_rng_tbls;
	struct interleave_tbl *interleave_tbls;
	struct memdev_spa_rng_tbl *memdev_spa_rng_tbls;
	struct flush_hint_tbl *flush_tbls;
	__u8 num_dimms; /* Number of DIMMs described by the FIT */
	/* Number of DIMMs described by the FIT that have a PM region */
	__u8 num_pm_dimms;
};

/******************************************************************************
 * ACPI Related Functions
 *****************************************************************************/

struct fit_header *create_fit_table(void *nvdimm_fit_ptr);
void free_fit_table(struct fit_header *fit_head);
struct memdev_spa_rng_tbl *__get_memdev_spa_tbl(struct fit_header *fit_head,
	__u16 pid);
struct memdev_spa_rng_tbl *get_memdev_spa_tbl(struct fit_header *fit_head,
	__u16 pid, __u16 addr_rng_type);
__u8 get_num_pm_dimms(struct fit_header *fit_head);
__u8 get_num_dimms(struct memdev_spa_rng_tbl *memdev_tbls, __u16 num_tbls);

/******************************************************************************
 * Conversion Functions
 *****************************************************************************/
__u64 spa_to_rdpa(phys_addr_t spa, struct memdev_spa_rng_tbl *mdsarmt_tbl,
	struct interleave_tbl *i_tbl, int *err);

phys_addr_t rdpa_to_spa(__u64 rdpa, struct memdev_spa_rng_tbl *mdsarmt_tbl,
	struct interleave_tbl *i_tbl, int *err);

#endif /* _NVDIMM_ACPI_H */
