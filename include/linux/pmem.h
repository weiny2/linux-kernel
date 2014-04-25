/*
 * Definitions for the Persistent Memory interface
 * Copyright (c) 2013, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _LINUX_PMEM_H
#define _LINUX_PMEM_H

#include <linux/types.h>

struct persistent_memory_extent {
	phys_addr_t	pme_spa;
	u64		pme_len;
	int		pme_numa_node;
	phys_addr_t	pme_errs[];	/* null or list of errors in range */
};

struct pmem_layout {
	u64		pml_flags;
	u64		pml_total_size;
	u32		pml_extent_count;
	u32		pml_interleave;	/* interleave bytes */
	struct persistent_memory_extent pml_extents[];
};

/*
 * Flags values
 */
#define PMEM_ENABLED 0x0000000000000001 /* can be used for Persistent Mem */
#define PMEM_ERRORED 0x0000000000000002 /* in an error state */
#define PMEM_COMMIT  0x0000000000000004 /* commit function available */
#define PMEM_CLEAR_ERROR  0x0000000000000008 /* clear error function provided */

#endif /* _LINUX_PMEM_H */
