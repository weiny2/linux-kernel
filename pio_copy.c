/*
 * Copyright (c) 2013 Intel Corporation. All rights reserved.
 * Copyright (c) 2009 QLogic Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "hfi.h"

/* additive distance between non-SOP and SOP space */
#define WFR_SOP_DISTANCE (WFR_TXE_PIO_SIZE / 2)
#define WFR_PIO_BLOCK_MASK (WFR_PIO_BLOCK_SIZE-1)

union mix {
	u64 val64;
	u32 val32[2];
};

/**
 * pio_copy - copy data block to MMIO space
 * @pbuf: a number of blocks allocated within a PIO send context
 * @pbc: PBC to send
 * @from: source, must be 8 byte aligned
 * @count: number of DWORD (32-bit) quantities to copy from source
 *
 * Copy data from source to PIO Send Buffer memory, 8 bytes at a time.
 * Must always write full BLOCK_SIZE bytes blocks.  The first block must
 * be written to the corresponding SOP=1 address.
 *
 * Known:
 * o pbuf->start always starts on a block boundary
 * o pbuf can wrap only at a block boundary
 */ 
void pio_copy(struct pio_buf *pbuf, u64 pbc, const void *from, size_t count)
{
	void __iomem *dest = pbuf->start + WFR_SOP_DISTANCE;
	void __iomem *send = dest + WFR_PIO_BLOCK_SIZE;
	void __iomem *dend;			/* 8-byte data end */

	/* write thte PBC */
	writeq(pbc, dest);
	dest += sizeof(u64);

	/* calculate where the QWORD data ends - in SOP=1 space */
	dend = dest + ((count>>1) * sizeof(u64));

	if (dend < send) {
		/* all QWORD data is within the SOP block, does *not*
		   reach the end of the SOP block */

		while (dest < dend) {
			writeq(*(u64 *)from, dest);
			from += sizeof(u64);
			dest += sizeof(u64);
		}
		/*
		 * No boundary checks are needed here:
		 * 0. We're not on the SOP block boundary
		 * 1. The possible DWORD dangle will still be within
		 *    the SOP block
		 * 2. We cannot wrap except on a block bondary.
		 */
	} else {
		/* QWORD data extends _to_ or beyond the SOP block */

		/* write 8-byte SOP chunk data */
		while (dest < send) {
			writeq(*(u64 *)from, dest);
			from += sizeof(u64);
			dest += sizeof(u64);
		}
		/* drop out of the SOP range */
		dest -= WFR_SOP_DISTANCE;
		dend -= WFR_SOP_DISTANCE;

		/*
		 * If the wrap comes before or matches the data end,
		 * copy until until the wrap, then wrap.
		 *
		 * If the data ends at the end of the SOP above and
		 * the buffer wraps, then pbuf->end == dend == dest
		 * and nothing will get written, but we will wrap in
		 * case there is a dangling DWORD.
		 */
		if (pbuf->end <= dend) {
			while (dest < pbuf->end) {
				writeq(*(u64 *)from, dest);
				from += sizeof(u64);
				dest += sizeof(u64);
			}

			dest -= pbuf->size;
			dend -= pbuf->size;
		}

		/* write 8-byte non-SOP, non-wrap chunk data */
		while (dest < dend) {
			writeq(*(u64 *)from, dest);
			from += sizeof(u64);
			dest += sizeof(u64);
		}
	}
	/* at this point we have wrapped if we are going to wrap */

	/* write dangling u32, if any */
	if (count & 1) {
		union mix val;

		val.val64 = 0;
		val.val32[0] = *(u32 *)from;
		writeq(val.val64, dest);
		dest += sizeof(u64);
	}
	/* fill in rest of block, no need to check pbuf->end
	   as we only wrap on a block boundary */
	while (((unsigned long)dest & WFR_PIO_BLOCK_MASK) != 0) {
		writeq(0, dest);
		dest += sizeof(u64);
	}
	/* TODO: update qw_written? */
}

/*
 * Segmented PIO Copy - start
 *
 * Start a PIO copy.  
 *
 * @pbuf: destination buffer
 * @pbc: the PBC for the PIO buffer
 * @from: data source, QWORD aligned
 * @count: DWORDs to copy
 */
void seg_pio_copy_start(struct pio_buf *pbuf, u64 pbc,
				const void *from, size_t count)
{
	void __iomem *dest = pbuf->start + WFR_SOP_DISTANCE;
	void __iomem *send = dest + WFR_PIO_BLOCK_SIZE;
	void __iomem *dend;			/* 8-byte data end */

	writeq(pbc, dest);
	dest += sizeof(u64);

	/* calculate where the QWORD data ends - in SOP=1 space */
	dend = dest + ((count>>1) * sizeof(u64));

	if (dend < send) {
		/* all QWORD data is within the SOP block, does *not*
		   reach the end of the SOP block */

		while (dest < dend) {
			writeq(*(u64 *)from, dest);
			from += sizeof(u64);
			dest += sizeof(u64);
		}
		/*
		 * No boundary checks are needed here:
		 * 0. We're not on the SOP block boundary
		 * 1. The possible DWORD dangle will still be within
		 *    the SOP block
		 * 2. We cannot wrap except on a block bondary.
		 */
	} else {
		/* QWORD data extends _to_ or beyond the SOP block */

		/* write 8-byte SOP chunk data */
		while (dest < send) {
			writeq(*(u64 *)from, dest);
			from += sizeof(u64);
			dest += sizeof(u64);
		}
		/* drop out of the SOP range */
		dest -= WFR_SOP_DISTANCE;
		dend -= WFR_SOP_DISTANCE;

		/*
		 * If the wrap comes before or matches the data end,
		 * copy until until the wrap, then wrap.
		 *
		 * If the data ends at the end of the SOP above and
		 * the buffer wraps, then pbuf->end == dend == dest
		 * and nothing will get written, but we will wrap in
		 * case there is a dangling DWORD.
		 */
		if (pbuf->end <= dend) {
			while (dest < pbuf->end) {
				writeq(*(u64 *)from, dest);
				from += sizeof(u64);
				dest += sizeof(u64);
			}

			dest -= pbuf->size;
			dend -= pbuf->size;
		}

		/* write 8-byte non-SOP, non-wrap chunk data */
		while (dest < dend) {
			writeq(*(u64 *)from, dest);
			from += sizeof(u64);
			dest += sizeof(u64);
		}
	}
	/* at this point we have wrapped if we are going to wrap */

	/* ...but it doesn't matter as we're done writing */

	/* save dangling u32, if any */
	if (count & 1) {
		pbuf->carry_valid = 1;
		pbuf->carry = *(u32 *)from;
	}
	/* no need for else here: the initialized buffer will already
	   have carry_valid and carry set to 0 */

	pbuf->qw_written = 1 /*PBC*/ + (count >> 1);
}

/*
 * Mid copy helper, "mixed case" - source is u64 aligned, dest is u32 aligned.
 * In particular, pbuf->carry_valid == 1.
 *
 * We must write whole u64s to the chip, so we must combine u32s.
 *
 * @pbuf: destination buffer
 * @from: data source, is QWORD aligned.
 * @count: DWORDs to copy
 *
 * Return value: number of DWORDS accepted so far. If even, all DWORDS have
 * been written, if odd the final DWORD is in carry, not written.
 *
 * Must handle count == 0.
 *
 * NOTE: If count is odd, this routine will read 4 bytes *beyond*
 * from+(count*sizeof(u32)).  It will not use that value, nor will
 * that value extend to possible illegal page as it will be part of
 * an aligned u64 read.
 */
static void mid_copy_mix(struct pio_buf *pbuf, const void *from, size_t count)
{
	void __iomem *dest = pbuf->start + (pbuf->qw_written * sizeof(u64));
	void __iomem *dend;			/* 8-byte data end */
	union mix val, temp;

	/*
	 * Calculate 8-byte data end - if count is odd, the last
	 * QWORD will contain only a DWORD of valid data.  See NOTE
	 * above.
	 */
	dend = dest + (((count+1)>>1) * sizeof(u64));

	/* stage overlap */
	val.val32[0] = pbuf->carry;

	if (pbuf->qw_written < WFR_PIO_BLOCK_SIZE/sizeof(u64)) {
		/*
		 * Still within SOP block.  We don't need to check for
		 * wrap because we are still in the first block and
		 * can only wrap on block boundaries.
		 */
		void __iomem *send;		/* SOP end */
		void __iomem *xend;

		/* calculate the end of data or end of block, whichever
		   comes first */
		send = pbuf->start + WFR_PIO_BLOCK_SIZE;
		xend = send < dend ? send : dend;

		/* shift up to SOP=1 space */
		dest += WFR_SOP_DISTANCE;
		xend += WFR_SOP_DISTANCE;

		/* write 8-byte chunk data */
		while (dest < xend) {
			temp.val64 = *(u64 *)from;
			val.val32[1] = temp.val32[0];
			writeq(val.val64, dest);
			val.val32[0] = temp.val32[1];
			from += sizeof(u64);
			dest += sizeof(u64);
		}

		/* shift down to SOP=0 space */
		dest -= WFR_SOP_DISTANCE;
	}
	/*
	 * At this point dest could be (either, both, or neither):
	 * - at dend
	 * - at the wrap
	 */

	/*
	 * If the wrap comes before or matches the data end,
	 * copy until until the wrap, then wrap.
	 *
	 * If dest is at the wrap, we will fall into the if,
	 * not do the loop, when wrap.
	 *
	 * If the data ends at the end of the SOP above and
	 * the buffer wraps, then pbuf->end == dend == dest
	 * and nothing will get written, but we will wrap in
	 * case there is a dangling DWORD.
	 */
	if (pbuf->end <= dend) {
		while (dest < pbuf->end) {
			temp.val64 = *(u64 *)from;
			val.val32[1] = temp.val32[0];
			writeq(val.val64, dest);
			val.val32[0] = temp.val32[1];
			from += sizeof(u64);
			dest += sizeof(u64);
		}

		dest -= pbuf->size;
		dend -= pbuf->size;
	}

	/* write 8-byte non-SOP, non-wrap chunk data */
	while (dest < dend) {
		temp.val64 = *(u64 *)from;
		val.val32[1] = temp.val32[0];
		writeq(val.val64, dest);
		val.val32[0] = temp.val32[1];
		from += sizeof(u64);
		dest += sizeof(u64);
	}

	if (count & 1) {
		/* count is odd - we had a carry in, so nothing will
		   carry out */
		pbuf->carry = 0;
		pbuf->carry_valid = 0;
	} else {
		/* count is even - we had a carry in, so we will have a
		   carry out */
		pbuf->carry = val.val32[0];
		/* pbuf->carry_valid was 1 */
	}
	pbuf->qw_written += count>>1;
}

/*
 * Mid copy helper, "straight case" - source and dest are u64 aligned.
 * In particular, pbuf->carry_valid == 0.
 *
 * @pbuf: destination buffer
 * @from: data source, is QWORD aligned
 * @count: DWORDs to copy
 *
 * Must handle count == 0.
 */
static void mid_copy_straight(struct pio_buf *pbuf, 
						const void *from, size_t count)
{
	void __iomem *dest = pbuf->start + (pbuf->qw_written * sizeof(u64));
	void __iomem *dend;			/* 8-byte data end */

	/* calculate 8-byte data end */
	dend = dest + ((count>>1) * sizeof(u64));

	if (pbuf->qw_written < WFR_PIO_BLOCK_SIZE/sizeof(u64)) {
		/*
		 * Still within SOP block.  We don't need to check for
		 * wrap because we are still in the first block and
		 * can only wrap on block boundaries.
		 */
		void __iomem *send;		/* SOP end */
		void __iomem *xend;

		/* calculate the end of data or end of block, whichever
		   comes first */
		send = pbuf->start + WFR_PIO_BLOCK_SIZE;
		xend = send < dend ? send : dend;

		/* shift up to SOP=1 space */
		dest += WFR_SOP_DISTANCE;
		xend += WFR_SOP_DISTANCE;

		/* write 8-byte chunk data */
		while (dest < xend) {
			writeq(*(u64 *)from, dest);
			from += sizeof(u64);
			dest += sizeof(u64);
		}

		/* shift down to SOP=0 space */
		dest -= WFR_SOP_DISTANCE;
	}
	/*
	 * At this point dest could be (either, both, or neither):
	 * - at dend
	 * - at the wrap
	 */

	/*
	 * If the wrap comes before or matches the data end,
	 * copy until until the wrap, then wrap.
	 *
	 * If dest is at the wrap, we will fall into the if,
	 * not do the loop, when wrap.
	 *
	 * If the data ends at the end of the SOP above and
	 * the buffer wraps, then pbuf->end == dend == dest
	 * and nothing will get written, but we will wrap in
	 * case there is a dangling DWORD.
	 */
	if (pbuf->end <= dend) {
		while (dest < pbuf->end) {
			writeq(*(u64 *)from, dest);
			from += sizeof(u64);
			dest += sizeof(u64);
		}

		dest -= pbuf->size;
		dend -= pbuf->size;
	}

	/* write 8-byte non-SOP, non-wrap chunk data */
	while (dest < dend) {
		writeq(*(u64 *)from, dest);
		from += sizeof(u64);
		dest += sizeof(u64);
	}

	if (count & 1) {
		pbuf->carry_valid = 1;
		pbuf->carry = *(u32 *)from;
	}
	/* else we know, coming in, that carry_valid is 0 */

	pbuf->qw_written += count>>1;
}

/*
 * Segmented PIO Copy - middle
 *
 * Must handle DWORD aligned tail (pbuf->carry_valid) and DWORD aligned source
 * (from).
 *
 * @pbuf: a number of blocks allocated within a PIO send context
 * @from: data source
 * @count: DWORDs to copy
 */
void seg_pio_copy_mid(struct pio_buf *pbuf, const void *from, size_t count)
{
	if ((unsigned long)from & 0x2) {	/* DWORD aligned source */
		if (pbuf->carry_valid) {	/* DWORD aligned tail */
			/*
			 * Case 0: u32 aligned source, u32 aligned dest
			 *
			 * Do one combined write, so source and dest are
			 * both u64 aligned, then do "straight" copy.
			 */
			void __iomem *dest;
			union mix val;

			dest = pbuf->start + (pbuf->qw_written * sizeof(u64));

			/*
			 * The two checks immediately below cannot both be
			 * true, hence the else.  If we have wrapped, we
			 * cannot still be within the first block.
			 * Conversely, if we are still in the first block, we
			 * cannot have wrapped.  We do the wrap check first
			 * as that is more likely.
			 */
			/* adjust if we've wrapped */
			if (dest >= pbuf->end)
				dest -= pbuf->size;
			/* jump to SOP range if within the first block */
			else if (pbuf->qw_written < WFR_PIO_BLOCK_SIZE/sizeof(u64))
				dest += WFR_SOP_DISTANCE;

			val.val32[0] = pbuf->carry;
			val.val32[1] = *(u32 *)from;
			writeq(val.val64, dest);
			pbuf->carry_valid = 0;
			pbuf->carry = 0;	/* TODO: needed? */
			pbuf->qw_written++;

			mid_copy_straight(pbuf, from+sizeof(u32), count-1);
		} else {			/* QWORD aligned tail */
			/*
			 * Case 1: u32 aligned source, u64 aligned dest
			 *
			 * Read the first u32 of source so it is u64 aligned,
			 * then do a "mixed" copy.
			 */
			pbuf->carry = *(u32 *)from;
			pbuf->carry_valid = 1;
			mid_copy_mix(pbuf, from+sizeof(u32), count-1);
		}
	} else {				/* QWORD aligned source */
		if (pbuf->carry_valid) {	/* DWORD aligned tail */
			/*
			 * Case 2: u64 aligned source, u32 aligned dest
			 *
			 * Do a "mixed" copy.
			 */
			mid_copy_mix(pbuf, from, count);
		} else {			/* QWORD aligned tail */
			/*
			 * Case 3: u64 aligned source, u64 aligned dest
			 *
			 * Everything aligns, do a "straight" copy.
			 */
			mid_copy_straight(pbuf, from, count);
		}
	}
}

/*
 * Segmented PIO Copy - end
 *
 * Write any remainder (in pbuf->carry) and finish writing the whole block.
 *
 * @pbuf: a number of blocks allocated within a PIO send context
 */
void seg_pio_copy_end(struct pio_buf *pbuf)
{
	void __iomem *dest = pbuf->start + (pbuf->qw_written * sizeof(u64));

	/*
	 * The two checks immediately below cannot both be true, hence the
	 * else.  If we have wrapped, we cannot still be within the first
	 * block.  Conversely, if we are still in the first block, we
	 * cannot have wrapped.  We do the wrap check first as that is
	 * more likely.
	 */
	/* adjust if we have wrapped */
	if (dest >= pbuf->end)
		dest -= pbuf->size;
	/* jump to the SOP range if within the first block */
	else if (pbuf->qw_written < (WFR_PIO_BLOCK_SIZE/sizeof(u64)))
		dest += WFR_SOP_DISTANCE;

	/* write the final u32, if present */
	if (pbuf->carry_valid) {
		union mix val;

		val.val64 = 0;
		val.val32[0] = pbuf->carry;

		writeq(val.val64, dest);
		dest += sizeof(u64);
		/*
		 * NOTE: We do not need to recalculate whether dest needs
		 * WFR_SOP_DISTANCE or not.
		 *
		 * If we are in the first block and the dangle write
		 * keeps us in the same block, dest will need
		 * to retain WFR_SOP_DISTANCE in the loop below.
		 *
		 * If we are in the first block and the dangle write pushes
		 * us to the next block, then loop below will not run
		 * and dest is not used.  Hence we do not need to update
		 * it.
		 *
		 * If we are past the first block, then WFR_SOP_DISTANCE
		 * was never added, so there is nothing to do.
		 */
	}

	/* fill in rest of block */
	while (((unsigned long)dest & WFR_PIO_BLOCK_MASK) != 0) {
		writeq(0, dest);
		dest += sizeof(u64);
	}
	/* TODO: update qw_written? set carry_valid to 0?  set carry to 0 */
	/* TODO: sanity check? pbuf->qw_written*sizeof(u64) ==
		pbuf->block_count * WFR_PIO_BLOCK_SIZE */
}
