// SPDX-License-Identifier: GPL-2.0 only
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#include "../common/mxlk_mmio.h"
#include "../common/mxlk_capabilities.h"

#define MXLK_CAP_TTL (32)

void *mxlk_cap_find(struct mxlk *mxlk, u16 start, u16 id)
{
	int ttl = MXLK_CAP_TTL;
	struct mxlk_cap_hdr *hdr;

	// If user didn't specify start, assume start of mmio
	if (!start) {
		start = mxlk_rd32(mxlk->mmio,
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

void mxlk_set_td_address(struct mxlk_transfer_desc *td, u64 address)
{
	mxlk_wr64(td, offsetof(struct mxlk_transfer_desc, address), address);
}

u64 mxlk_get_td_address(struct mxlk_transfer_desc *td)
{
	return mxlk_rd64(td, offsetof(struct mxlk_transfer_desc, address));
}

void mxlk_set_td_length(struct mxlk_transfer_desc *td, u32 length)
{
	mxlk_wr32(td, offsetof(struct mxlk_transfer_desc, length), length);
}

u32 mxlk_get_td_length(struct mxlk_transfer_desc *td)
{
	return mxlk_rd32(td, offsetof(struct mxlk_transfer_desc, length));
}

void mxlk_set_td_interface(struct mxlk_transfer_desc *td, u16 interface)
{
	mxlk_wr16(td, offsetof(struct mxlk_transfer_desc, interface),
		  interface);
}

u16 mxlk_get_td_interface(struct mxlk_transfer_desc *td)
{
	return mxlk_rd16(td, offsetof(struct mxlk_transfer_desc, interface));
}

void mxlk_set_td_status(struct mxlk_transfer_desc *td, u16 status)
{
	mxlk_wr16(td, offsetof(struct mxlk_transfer_desc, status), status);
}

u16 mxlk_get_td_status(struct mxlk_transfer_desc *td)
{
	return mxlk_rd16(td, offsetof(struct mxlk_transfer_desc, status));
}

void mxlk_set_tdr_head(struct mxlk_pipe *p, u32 head)
{
	mxlk_wr32(p->head, 0, head);
}

u32 mxlk_get_tdr_head(struct mxlk_pipe *p)
{
	return mxlk_rd32(p->head, 0);
}

void mxlk_set_tdr_tail(struct mxlk_pipe *p, u32 tail)
{
	mxlk_wr32(p->tail, 0, tail);
}

u32 mxlk_get_tdr_tail(struct mxlk_pipe *p)
{
	return mxlk_rd32(p->tail, 0);
}
