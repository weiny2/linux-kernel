/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_CAPABILITIES_HEADER_
#define MXLK_CAPABILITIES_HEADER_

#include "mxlk.h"
#include "mxlk_common.h"

#define MXLK_CAP_TTL (32)

static inline
void *mxlk_cap_find(struct mxlk *mxlk, u16 start, u16 id)
{
	int ttl = MXLK_CAP_TTL;
	struct mxlk_cap_hdr *hdr;

	// If user didn't specify start, assume start of mmio
	if (!start) {
		start = ioread32(mxlk->mmio +
				 offsetof(struct mxlk_mmio, cap_offset));
	}

	// Read header info
	hdr = (struct mxlk_cap_hdr *)(mxlk->mmio + start);
	// Check if we still have time to live
	while (ttl--) {
		// If cap matches, return header
		if (hdr->id == id)
			return hdr;
		// If cap is NULL, we are at the end of the list
		else if (hdr->id == MXLK_CAP_NULL)
			return NULL;
		// If no match and no end of list, traverse the linked list
		else
			hdr = (struct mxlk_cap_hdr *)(mxlk->mmio + hdr->next);
	}

	// If we reached here, the capability list is corrupted
	return NULL;
}

static inline
void mxlk_set_td_address(struct mxlk_transfer_desc *td, u64 address)
{
	_iowrite64(address, &td->address);
}

static inline
u64 mxlk_get_td_address(struct mxlk_transfer_desc *td)
{
	return _ioread64(&td->address);
}

static inline
void mxlk_set_td_length(struct mxlk_transfer_desc *td, u32 length)
{
	iowrite32(length, &td->length);
}

static inline
u32 mxlk_get_td_length(struct mxlk_transfer_desc *td)
{
	return ioread32(&td->length);
}

static inline
void mxlk_set_td_interface(struct mxlk_transfer_desc *td, u16 interface)
{
	iowrite16(interface, &td->interface);
}

static inline
u16 mxlk_get_td_interface(struct mxlk_transfer_desc *td)
{
	return ioread16(&td->interface);
}

static inline
void mxlk_set_td_status(struct mxlk_transfer_desc *td, u16 status)
{
	iowrite16(status, &td->status);
}

static inline
u16 mxlk_get_td_status(struct mxlk_transfer_desc *td)
{
	return ioread16(&td->status);
}

static inline
void mxlk_set_tdr_head(struct mxlk_pipe *p, u32 head)
{
	iowrite32(head, p->head);
}

static inline
u32 mxlk_get_tdr_head(struct mxlk_pipe *p)
{
	return ioread32(p->head);
}

static inline
void mxlk_set_tdr_tail(struct mxlk_pipe *p, u32 tail)
{
	iowrite32(tail, p->tail);
}

static inline
u32 mxlk_get_tdr_tail(struct mxlk_pipe *p)
{
	return ioread32(p->tail);
}
#endif // MXLK_CAPABILITIES_HEADER_
