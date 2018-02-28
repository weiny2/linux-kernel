/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015-2017 Intel Corporation.
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
 * Copyright (c) 2015-2017 Intel Corporation.
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

#ifndef _HFI_EQ_COMMON_H
#define _HFI_EQ_COMMON_H

/* event index to hardware ID and NI conversion */
#define HFI_EIDX2ID(idx)	((idx) & 0x7FF)
#define HFI_EIDX2NI(idx)	(((idx) >> 11) & 0x3)

#ifdef __KERNEL__
#define HFI_EQ_ZERO(ctx, ni)  (&((ctx)->eq_zero[(ni)]))
#else
#define HFI_EQ_ZERO(ctx, ni)  ((ctx)->eq_zero[(ni)])
#endif

#define HFI_EQ_EMPTY (0)

static inline
int hfi_eq_full(struct hfi_eq *eq)
{
	unsigned int pending;

#ifdef __KERNEL__
	pending = atomic_read(&eq->events_pending);
#else
	pending = eq->events_pending.counter;
#endif

	return pending >= (eq->count - 1);
}

static inline
void hfi_eq_pending_inc(struct hfi_eq *eq)
{
#ifdef __KERNEL__
	atomic_inc(&eq->events_pending);
#else
	__sync_add_and_fetch(&eq->events_pending.counter, 1);
#endif
}

static inline
void hfi_eq_pending_dec_multiple(struct hfi_eq *eq, ssize_t dec)
{
#ifdef __KERNEL__
	if (atomic_read(&eq->events_pending) >= dec)
		atomic_sub(dec, &eq->events_pending);
#else
	if (eq->events_pending.counter >= dec)
		__sync_add_and_fetch(&eq->events_pending.counter, -dec);
#endif
}

static inline
void hfi_eq_pending_dec(struct hfi_eq *eq)
{
	hfi_eq_pending_dec_multiple(eq, 1);
}

static inline
int hfi_eq_advance(struct hfi_eq *eq_handle, u64 *eq_entry)
{
	u32 eq_count;
	u32 eq_head_idx, new_head_idx;

	eq_count = eq_handle->count;

	/* Read the current head index */
	eq_head_idx = *eq_handle->head_addr;

	/* Clear the valid bit for reuse */
	*eq_entry &= ~TARGET_EQENTRY_V_MASK;

	/* Move on SW head index and wrap */
	new_head_idx = ++eq_head_idx & (eq_count - 1);

	/* Update the SW head index in host memory (not used by FXR) */
	*eq_handle->head_addr = new_head_idx;

	hfi_eq_pending_dec(eq_handle);

	return 0;
}

static inline
int hfi_eq_advance_multiple(struct hfi_eq *eq_handle,
			    u64 **eq_entry, size_t count)
{
	u32 eq_head_idx, new_head_idx;
	size_t entry_count;

	/* Clear the valid bit of each entry for reuse */
	for (entry_count = 0; entry_count < count; entry_count++)
		*eq_entry[entry_count] &= ~TARGET_EQENTRY_V_MASK;

	/* Read the current head index and move it on*/
	eq_head_idx = *eq_handle->head_addr + count;

	/* Wrap */
	new_head_idx = eq_head_idx & (eq_handle->count - 1);

	/* Update the SW head index in host memory (not used by FXR) */
	*eq_handle->head_addr = new_head_idx;

	hfi_eq_pending_dec_multiple(eq_handle, count);

	return 0;
}

/*
 * Before reading nth entry, caller is responsible for ensuring that
 * entries 0 through n-1 are valid.
 */
static inline
u64 *_hfi_eq_nth_entry(struct hfi_eq *eq_handle, int nth)
{
	u32 eq_head_idx;

	/*
	 * Determine head index.
	 *
	 * The first case avoids an integer modulus operation.
	 * Nth will almost always be known to be 0 at compile time, so this
	 * branch should be optimized away.
	 */
	if (nth == 0)
		eq_head_idx = *eq_handle->head_addr;
	else
		eq_head_idx = ((*eq_handle->head_addr) + nth) %
			eq_handle->count;

	/* Calculate the current EQ FIFO head slot, return pointer */
	return (u64 *)((u64)eq_handle->base +
			(eq_head_idx << eq_handle->width));
}

static inline
int hfi_eq_peek_nth(struct hfi_eq *eq_handle,
		    u64 **entry, int nth, bool *dropped)
{
	volatile u64 *eq_entry;

	eq_entry = _hfi_eq_nth_entry(eq_handle, nth);
	if (!eq_entry)
		return -EINVAL;

	/*
	 * Test the valid bit of the EQ FIFO entry
	 * to see if it has been written by the HW
	 */
	if (likely(*eq_entry & TARGET_EQENTRY_V_MASK)) {
		/* Return EQ slot address to the caller */
		*entry = (u64 *)eq_entry;

		*dropped = !!(*eq_entry & TARGET_EQENTRY_D_MASK);
		return 1;
	}

	return HFI_EQ_EMPTY;
}

#define hfi_eq_peek(eq_handle, entry, dropped) \
	hfi_eq_peek_nth(eq_handle, entry, 0, dropped)

/*
 * Peek multiple entries in the queue specified by eq_handle, starting in
 * the nth position. Caller is responsible for allocating the entry
 * vector for the requested entries.
 *
 * count - Maximum number of entries to peek.
 * entry - Caller-allocated vector where entries are returned.
 * dropped - dropped value for the last peeked entry.
 *
 * On success, it returns the number of entries peeked and passed in the
 * entry vector (>= 0).
 * On error, a return code from the driver is returned (< 0) and the
 * entries in the entry vector should be ignored by the caller.
 */
static inline
int hfi_eq_peek_nth_multiple(struct hfi_eq *eq_handle,
			     u64 **entry, int nth, bool *dropped,
			     size_t count)
{
	u64 *peeked_entry;
	size_t entry_count;
	int ret;

	entry_count = 0;
	do {
		ret = hfi_eq_peek_nth(eq_handle, &peeked_entry,
				      nth + entry_count, dropped);

		/*
		 * Return EQ slot address to the caller if
		 * this is not a dropped event
		 * and no error was returned when peeking.
		 */
		if (ret < 0) {
			return ret;
		} else if (ret > 0) {
			*(entry + entry_count++) = peeked_entry;
			if (unlikely(*dropped))
				break;
		} else {
			/* Stop peeking if the queue is empty */
			break;
		}
	} while (entry_count < count);

	return entry_count;
}

#define hfi_eq_peek_multiple(ctx, eq_handle, entry, dropped, count) \
	hfi_eq_peek_nth_multiple(ctx, eq_handle, entry, 0, dropped, count)

/* Poll for PTL_CMD_COMPLETE events on EQD=0,NI=0 */
static inline
int _hfi_eq_poll_cmd_complete(struct hfi_ctx *ctx, u64 *done)
{
	int ret;
	bool dropped = false;

	while (*done == 0) {
		u64 *entry = NULL;
		union initiator_EQEntry *event;

		do {
			ret = hfi_eq_peek(HFI_EQ_ZERO(ctx, 0),
					  &entry, &dropped);
			if (ret < 0)
				return ret;
			cpu_relax();
		} while (ret == HFI_EQ_EMPTY);

		event = (union initiator_EQEntry *)entry;

		_HFI_DBG_PRINTF(
		    "Comp evt %p kind %d ptl_idx %x fail_type %d usr_ptr %lx\n",
		    event, event->event_kind, event->ptl_idx,
		    event->fail_type, event->user_ptr);

		/* Check that the completion event makes sense */
		if (unlikely(event->event_kind != PTL_CMD_COMPLETE) || dropped)
			return -EIO;

		/*
		 * Update the caller's 'done' flag
		 * and return the Event fail_type (NB: must be a non-zero value)
		 */
		if (event->user_ptr)
			*(u64 *)event->user_ptr =
				(event->fail_type | (1ULL << 63));

		ret = hfi_eq_advance(HFI_EQ_ZERO(ctx, 0), entry);
		if (unlikely(ret < 0))
			return ret;
	}

	return 0;
}

/*
 * Poll for PTL_CMD_COMPLETE events on EQD=0,NI=0
 * using mutex lock in user space for protection.
 */
static inline
int hfi_eq_poll_cmd_complete(struct hfi_ctx *ctx, u64 *done)
{
	int ret;

	HFI_TS_MUTEX_LOCK(ctx);
	ret = _hfi_eq_poll_cmd_complete(ctx, done);
	HFI_TS_MUTEX_UNLOCK(ctx);
	return ret;
}

static inline
int hfi_rx_eq_command(struct hfi_ctx *ctx, struct hfi_eq *eq_handle,
		      struct hfi_cmdq *rx_cq, enum rx_cq_cmd eqd_cmd)
{
	union hfi_rx_cq_command cmd;
	int cmd_slots, ret;
	u64 done = 0;

	cmd_slots = hfi_format_rx_update64(ctx,
					   HFI_EIDX2NI(eq_handle->idx),
					   HFI_PT_ANY, /* PT is unused */
					   eqd_cmd,
					   HFI_EIDX2ID(eq_handle->idx),
					   0, 0, 0, 0, /* payload unused */
					   (u64)&done,
					   HFI_GEN_CC, &cmd);

	do {
		ret = hfi_rx_command(rx_cq, (u64 *)&cmd, cmd_slots);
	} while (ret == -EAGAIN);

	if (ret)
		return ret;

	/* Busy poll for the EQ command completion on EQD=0 (NI=0) */
	ret = hfi_eq_poll_cmd_complete(ctx, &done);

	return ret;
}
#endif
