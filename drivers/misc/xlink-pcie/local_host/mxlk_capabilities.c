// SPDX-License-Identifier: GPL-2.0-only
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#include "../common/mxlk_mmio.h"
#include "../common/mxlk_common.h"
#include "../common/mxlk_capabilities.h"

static int fragment_size = MXLK_FRAGMENT_SIZE;
module_param(fragment_size, int, 0664);
MODULE_PARM_DESC(fragment_size, "transfer descriptor size (default 32KB)");

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

void mxlk_set_cap(struct mxlk *mxlk)
{
	struct mxlk_cap_txrx cap;
	uint32_t start = roundup(sizeof(struct mxlk_mmio), MXLK_DMA_ALIGNMENT);
	size_t hdr_len = sizeof(struct mxlk_cap_txrx);
	size_t tx_len = sizeof(struct mxlk_transfer_desc) * MXLK_NUM_TX_DESCS;
	size_t rx_len = sizeof(struct mxlk_transfer_desc) * MXLK_NUM_RX_DESCS;
	uint16_t next = (uint16_t)(start + hdr_len + tx_len + rx_len);

	mxlk_wr32(mxlk->mmio, MXLK_MMIO_CAPABILITIES, start);

	cap.hdr.id = MXLK_CAP_TXRX;
	cap.hdr.next = start + sizeof(struct mxlk_cap_txrx);
	cap.fragment_size = fragment_size;
	cap.tx.ring = start + hdr_len;
	cap.tx.ndesc = MXLK_NUM_TX_DESCS;
	cap.tx.head = 0;
	cap.tx.tail = 0;
	cap.rx.ring = start + hdr_len + tx_len;
	cap.rx.ndesc = MXLK_NUM_RX_DESCS;
	cap.rx.head = 0;
	cap.rx.tail = 0;
	mxlk_wr_buffer(mxlk->mmio, start, &cap, sizeof(struct mxlk_cap_txrx));

	mxlk_wr16(mxlk->mmio, next, MXLK_CAP_NULL);
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
