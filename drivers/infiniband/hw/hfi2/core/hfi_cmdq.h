/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _HFI_CMDQ_H
#define _HFI_CMDQ_H

#define HFI_TX_MAX_BUFFERED	64
#define HFI_TX_MAX_PIO		4096

/* TODO - cleanup usage of this */
#define _HFI_DBG_PRINTF(FMT, args...)

/* Using copy64 for Command Queues require 8-byte aligned writes (see below) */
#define _hfi_copy64_aligned(dst, src)	hfi_memcpy_align((dst), (src), 64, 8)
#define _hfi_copy64(dst, src)		hfi_memcpy_align((dst), (src), 64, 8)
#define _hfi_copy32_aligned(dst, src)	_hfi_memcpy((dst), (src), 32)
#define _hfi_copy32(dst, src)		_hfi_memcpy((dst), (src), 32)
#define _hfi_memcpy(dst, src, len)	hfi_memcpy_align((dst), (src), (len), 0)

static inline
void hfi_memcpy_align(void *dst, const void *src, size_t len,
		      uintptr_t min_align)
{
	uintptr_t align = ((uintptr_t)dst | (uintptr_t)src | len);
	int i;

	if ((align & 7L) == 0) {
		for (i = 0; i < len / sizeof(u64); i++)
			((u64 *)dst)[i] = ((const u64 *)src)[i];
	/*
	 * If src is not 8-byte aligned, but 8-byte stores are required (per
	 * min_align), then force 8-byte aligned writes.
	 * As Command Queues require 8-byte aligned writes, this is required
	 * for usage of PIO in kernel IB driver.  This is seen as we may
	 * be given a 4-byte aligned src pointer due to IB headers being
	 * specified in multiples of 4-bytes and causing the payload to thus
	 * become offset by 4 bytes.
	 */
	} else if (min_align && (((min_align |
				  (uintptr_t)dst | len) & 7L) == 0)) {
		u64 tmp;

		for (i = 0; i < len / sizeof(u64); i++) {
			tmp  = ((const u64 *)src)[i];
			((u64 *)dst)[i] = tmp;
		}
	} else if ((align & 3L) == 0) {
		for (i = 0; i < len / sizeof(u32); i++)
			((u32 *)dst)[i] = ((const u32 *)src)[i];
	} else {
		memcpy(dst, src, len);
	}
}

/*
 * Macros for CMDQ manipulation
 */

#define _HFI_SLOT_INDEX(IDX, TSLOTS)		  ((IDX) & (TSLOTS - 1))
#define _HFI_INCR_SLOT_INDEX(IDXP, INCR, TSLOTS)  (*(IDXP) = \
						   (*(IDXP) + (INCR)) & \
						   (TSLOTS - 1))
#define _HFI_FIFO_SPACE(T, H, TSLOTS)		  ((TSLOTS - 1) - \
						   (((T) - (H)) & (TSLOTS - 1)))

/* Calculate the total command length (+ payload) in 8-byte flits */
#define _HFI_CMD_LENGTH(LEN, OFFSET)		  (((LEN) + (OFFSET) + 7) >> 3)
/* Calculate the total command length (+ payload) in 64-byte slots */
#define _HFI_CMD_SLOTS(CMD_LEN)			  (((CMD_LEN) + 7) >> 3)

static inline
int hfi_cmd_slots_avail(struct hfi_cmdq *cq, unsigned int req)
{
	if (unlikely(cq->slots_avail < req)) {
		_HFI_DBG_PRINTF("%s: idx %x sw_idx %x hw_idx %x, slots %d\n",
				__func__, cq->slot_idx, cq->sw_head_idx,
				*cq->head_addr, cq->slots_avail);

		/* refresh SW head ptr once */
		cq->sw_head_idx = *cq->head_addr;
		cq->slots_avail = _HFI_FIFO_SPACE(cq->slot_idx,
						  cq->sw_head_idx,
						  cq->slots_total);
	}
	return (cq->slots_avail >= req);
}

/*
 * Now copy the user data into the TX CMDQ
 * This assumes that the payload begins immediately
 * after the TX header command
 * For PIO commands this means it must be a hfi_tx_cq_pio_put_match_t
 */
static inline
void _hfi_copy_payload(struct hfi_cmdq *cq, const void *start, size_t length,
		       u8 slot_offset, unsigned int slots_total)
{
	u8   start_slot;
	u64  *cq_addr;
	size_t remain;
	int       i;
	const u8 *payload = (const u8 *)start;

	start_slot = cq->slot_idx;

	/*
	 * Write the CMDQ slots in forward order to avoid
	 * store-and-forward for large payloads
	 */
	for (i = slot_offset, remain = length; remain >= 64;
	     i++, remain -= 64) {
		cq_addr = (u64 *)((uintptr_t)cq->base +
				       (_HFI_SLOT_INDEX(start_slot + i,
							slots_total) * 64));

		_HFI_DBG_PRINTF("write CMDQ_SLOT[%03d] %p payload %p (%p)\n",
				_HFI_SLOT_INDEX(start_slot + i, slots_total),
				&cq_addr[0], &payload[(i - 1) << 6], start);
		/* Burst write a cacheline of data to the CMDQ */
		_hfi_copy64(&cq_addr[0], &payload[(i - 1) << 6]);
	}

	if (remain) {
		u64 buffer[8] __aligned(64);

		memset((char *)buffer + remain, 0, 64 - remain);
		/* We have a runty bit of data left to copy
		 * and we must not read beyond the end of the user buffer
		 * So carefully copy it to an aligned buffer and then issue it
		 */
		_hfi_memcpy(buffer, &payload[length - remain], remain);

		cq_addr = (u64 *)((uintptr_t)cq->base +
				       (_HFI_SLOT_INDEX(start_slot + i,
							slots_total) * 64));

		_HFI_DBG_PRINTF("write CMDQ_SLOT[%03d] %p payload %p (%p)\n",
				_HFI_SLOT_INDEX(start_slot + i, slots_total),
				&cq_addr[0], buffer, &payload[length - remain]);
		/* Burst write a cacheline of data to the CMDQ */
		_hfi_copy64_aligned(&cq_addr[0], buffer);
	}
}

static inline
void _hfi_command_pio(struct hfi_cmdq *cq, u64 *command, const void *start,
		      size_t length, unsigned int nslots,
		      unsigned int slots_total)
{
	u8   start_slot = cq->slot_idx;
	u64  *cq_addr;

	_HFI_DBG_PRINTF("PIO command of %d slots start %d avail %d\n",
			nslots, start_slot, cq->slots_avail);

	/*
	 * TODO Should write the first slot here,
	 * but it causes issues in simics model currently
	 */

	/* Copy the user data into the TX CMDQ */
	_hfi_copy_payload(cq, start, length, 1, slots_total);

	/* Burst write the command cacheline (1st slot) to the CMDQ */
	cq_addr = (u64 *)((uintptr_t)cq->base +
			       (_HFI_SLOT_INDEX(start_slot, slots_total) * 64));
	_HFI_DBG_PRINTF("write CMDQ_SLOT[%03d] %p payload %p\n",
			_HFI_SLOT_INDEX(start_slot, slots_total),
			&cq_addr[0], &command[0]);
	_hfi_copy64_aligned(&cq_addr[0], &command[0]);

	_HFI_INCR_SLOT_INDEX(&cq->slot_idx, nslots, slots_total);
	cq->slots_avail -= nslots;
}

/* Issue a 2 slot TX command */
static inline
void _hfi_command2(struct hfi_cmdq *cq, u64 *command,
		   unsigned int slots_total)
{
	u8   start_slot = cq->slot_idx;
	u64  *cq_addr;

	/*
	 * We write the CMDQ slots in reverse order
	 * (in case we get de-scheduled)
	 */

	/* Burst write the cachelines of command+data to the CMDQ */
	cq_addr = (u64 *)((uintptr_t)cq->base +
			       (_HFI_SLOT_INDEX(start_slot + 1,
						slots_total) * 64));
	_hfi_copy64_aligned(&cq_addr[0], &command[8]);

	/* NB: We have to cope with CMDQ wrap */
	cq_addr = (u64 *)((uintptr_t)cq->base +
			       (_HFI_SLOT_INDEX(start_slot, slots_total) * 64));
	_hfi_copy64_aligned(&cq_addr[0], &command[0]);

	_HFI_INCR_SLOT_INDEX(&cq->slot_idx, 2, slots_total);
	cq->slots_avail -= 2;
}

/* Issue a single slot command using a cacheline write */
static inline
void _hfi_command(struct hfi_cmdq *cq, u64 *command,
		  unsigned int slots_total)
{
	u8   start_slot = cq->slot_idx;
	u64  *cq_addr;

	cq_addr = (u64 *)((uintptr_t)cq->base + (start_slot * 64));

	/* Burst write a command to the CMDQ */
	_hfi_copy64_aligned(&cq_addr[0], &command[0]);

	_HFI_INCR_SLOT_INDEX(&cq->slot_idx, 1, slots_total);
	cq->slots_avail--;
}

/* Issue a TX command sequence (no PIO data) */
static inline
int hfi_tx_command(struct hfi_cmdq *cq, u64 *command,
		   unsigned int nslots)
{
	if (unlikely(!hfi_cmd_slots_avail(cq, nslots)))
		return -EAGAIN;

	if (nslots == 1)
		_hfi_command(cq, command, HFI_CMDQ_TX_ENTRIES);
	else /*if (nslots == 2)*/
		_hfi_command2(cq, command, HFI_CMDQ_TX_ENTRIES);

	return 0;
}

/* Issue an RX command sequence */
static inline
int hfi_rx_command(struct hfi_cmdq *cq, u64 *command,
		   unsigned int nslots)
{
	if (unlikely(!hfi_cmd_slots_avail(cq, nslots)))
		return -EAGAIN;

	if (nslots == 1)
		_hfi_command(cq, command, HFI_CMDQ_RX_ENTRIES);
	else if (nslots == 2)
		_hfi_command2(cq, command, HFI_CMDQ_RX_ENTRIES);

	return 0;
}

/*
 * RX Command formats
 */
struct hfi_rx_cq_list {
	union rx_cq_list_flit0		flit0;
	union rx_cq_list_flit1		flit1;
};

struct hfi_rx_cq_state {
	union rx_cq_state_flit0		flit0;
	union rx_cq_state_flit1		flit1;
};

struct hfi_rx_cq_state_verbs {
	union rx_cq_state_verbs_flit0	flit0;
	union rx_cq_state_flit1		flit1;
};

struct hfi_rx_cq_list_verbs {
	union rx_cq_list_flit0		flit0;
	union rx_cq_list_verbs_flit1	flit1;
};

struct hfi_rx_cq_trig {
	union rx_cq_trig_flit0		flit0;
	union rx_cq_trig_flit1		flit1;
};

struct hfi_rx_cq_verbs {
	union rx_cq_state_verbs_flit0	flit0;
	union rx_cq_state_flit1		flit1;
};

union hfi_rx_cq_command {
	struct hfi_rx_cq_list		list;
	struct hfi_rx_cq_state		state;
	struct hfi_rx_cq_trig		trig;
	struct hfi_rx_cq_state_verbs	state_verbs;
	struct hfi_rx_cq_list_verbs	list_verbs;
} __aligned(64);

union hfi_rx_cq_update_command {
	struct hfi_rx_cq_state_verbs	state_verbs;
	u64				command[16];
} __aligned(64);

/* Format an RX CMDQ command for a 64B STATE UPDATE operation */
static inline
int hfi_format_rx_update64(struct hfi_ctx *ctx, u8 ni,
			   u32 pt_index,
			   enum rx_cq_cmd cq_cmd,
			   u16 ct_handle,
			   u64 p0, u64 p1,
			   u64 m0, u64 m1,
			   u64 user_ptr,
			   u8 ncc,
			   union hfi_rx_cq_command *cmd)
{
	cmd->state.flit0.p0            = p0;
	cmd->state.flit0.p1	       = p1;
	cmd->state.flit0.c.ptl_idx     = pt_index;
	cmd->state.flit0.d.ni          = ni;
	cmd->state.flit0.d.ct_handle   = ct_handle;
	cmd->state.flit0.d.ncc         = ncc;
	cmd->state.flit0.d.command     = cq_cmd;
	cmd->state.flit0.d.cmd_len     = (sizeof(cmd->state) >> 5) - 1;

	cmd->state.flit1.e.cmd_pid     = ctx->pid;
	cmd->state.flit1.p2            = m0;
	cmd->state.flit1.p3            = m1;
	cmd->state.flit1.user_ptr      = user_ptr;

	return sizeof(cmd->state) >> 6;
}

/* Format an RX CMDQ command for a 64B STATE WRITE operation */
static inline
int hfi_format_rx_write64(struct hfi_ctx *ctx, u8 ni,
			  u32 pt_index,
			  enum rx_cq_cmd cq_cmd,
			  u16 ct_handle,
			  u64 *payload,
			  u64 user_ptr,
			  u8 ncc,
			  union hfi_rx_cq_command *cmd)
{
	return hfi_format_rx_update64(ctx, ni, pt_index,
				      cq_cmd, ct_handle,
				      payload[0], payload[1],
				      payload[2], payload[3],
				      user_ptr, ncc, cmd);
}

/* Format an RX CMDQ command for an ENTRY_WRITE operation (bypass packets) */
static inline
int hfi_format_rx_bypass(struct hfi_ctx *ctx, u8 ni,
			 const void *start, size_t length,
			 u32 pt_index, u32 id,
			 u32 me_options,
			 u16 ct_handle,
			 size_t min_free,
			 u64 user_ptr, u16 me_handle,
			 u8 ncc,
			 union hfi_rx_cq_command *cmd)
{
	cmd->list.flit0.a             = 0;
	cmd->list.flit0.b	      = 0;
	/* above unused for bypass */
	cmd->list.flit0.c.user_id     = id;
	cmd->list.flit0.c.me_options  = me_options;
	cmd->list.flit0.c.ptl_idx     = pt_index;
	cmd->list.flit0.d.initiator_id  = 0;
	cmd->list.flit0.d.ct_handle   = ct_handle;
	cmd->list.flit0.d.ncc         = ncc;
	cmd->list.flit0.d.command     = ENTRY_WRITE;
	cmd->list.flit0.d.cmd_len     = (sizeof(cmd->list) >> 5) - 1;

	cmd->list.flit1.e.cmd_pid     = ctx->pid;
	cmd->list.flit1.e.list_handle = me_handle;
	cmd->list.flit1.e.min_free    = min_free;
	cmd->list.flit1.f.length      = length;
	cmd->list.flit1.g             = user_ptr;
	cmd->list.flit1.j.val         = 0;
	cmd->list.flit1.j.start       = (uintptr_t)start;
	cmd->list.flit1.j.ni          = ni;
	cmd->list.flit1.j.v           = 1;

	return sizeof(cmd->list) >> 6;
}

#endif /* _HFI_CMDQ_H */
