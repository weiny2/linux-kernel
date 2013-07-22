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
 * @to: destination in TXE PIO space (SOP=0), must be 64-byte aligned
 * @pbc: The PIO buffer's PBC
 * @from: source, must be 8 byte aligned
 * @count: number of DWORD (32-bit) quantities to copy
 *
 * Copy data from kernel space to PIO Send Buffer memory, 8 bytes at a time.
 * Must always write full BLOCK_SIZE bytes blocks.  The first block will
 * be written to the corresponding SOP=1 address.
 *
 * NOTE: count does _not_ include the size of the PBC.
 */ 
void pio_copy(void __iomem *to, u64 pbc, const void *from, size_t count)
{
	unsigned long dest = (unsigned long)to + WFR_SOP_DISTANCE;
	unsigned long send;			/* SOP end */
	unsigned long dend;			/* 8-byte data end */

	send = dest + WFR_PIO_BLOCK_SIZE;
	dend = dest + sizeof(u64) + ((count&~(size_t)0x1) * 4);

	writeq(pbc, (void  __iomem *)dest);
	dest += sizeof(u64);

	/* write 8-byte chunk data */
	while (dest < dend) {
		writeq(*(u64 *)from, (void  __iomem *)dest);
		from += sizeof(u64);
		dest += sizeof(u64);
		if (dest == send) {
			dest -= WFR_SOP_DISTANCE;
			dend -= WFR_SOP_DISTANCE;
			goto main_no_sop;
		}
	}
	/* write dangling u32, if any */
	if (count & 1) {
		union mix val;

		val.val64 = 0;
		val.val32[0] = *(u32 *)from;
		writeq(val.val64, (void  __iomem *)dest);
		dest += sizeof(u64);
		if (dest == send) {
			dest -= WFR_SOP_DISTANCE;
			goto fill_no_sop;
		}
	}
	/* fill in rest of block, no need to check send as we're
	   on the final block and sop will stay the same */
	while ((dest & WFR_PIO_BLOCK_MASK) != 0) {
		writeq(0, (void  __iomem *)dest);
		dest += sizeof(u64);
	}

	return;

	/*
	 * This is the same as above, but without the SOP address boost
	 * and SOP end check.
	 */
main_no_sop:
	/* write 8-byte chunk data */
	while (dest < dend) {
		writeq(*(u64 *)from, (void  __iomem *)dest);
		from += sizeof(u64);
		dest += sizeof(u64);
	}
	/* write dangling u32, if any */
	if (count & 1) {
		writeq(*(u32 *)from, (void  __iomem *)dest);
		dest += sizeof(u64);
	}
fill_no_sop:
	/* fill in rest of block */
	while ((dest & WFR_PIO_BLOCK_MASK) != 0) {
		writeq(0, (void  __iomem *)dest);
		dest += sizeof(u64);
	}
}



/*
 * Segmented PIO Copy - start
 *
 * Start a PIO copy.  
 *
 * @to: destination buffer
 * @carry: OUT - possible unwritten data
 * @pbc: the PBC for the PIO buffer
 * @from: data source, QWORD aligned
 * @count: DWORDs to copy
 *
 * Return value: number of DWORDS accepted so far. If even, all DWORDS have
 * been written, if odd the final DWORD is in carry, not written.
 */
u32 seg_pio_copy_start(void __iomem *to, u32 *carry, u64 pbc,
				const void *from, size_t count)
{
	unsigned long dest = (unsigned long)to + WFR_SOP_DISTANCE;
	unsigned long send;			/* SOP end */
	unsigned long dend;			/* 8-byte data end */

	send = dest + WFR_PIO_BLOCK_SIZE;
	dend = dest + sizeof(u64) + ((count>>1) * sizeof(u64));

	writeq(pbc, (void  __iomem *)dest);
	dest += sizeof(u64);

	/* write 8-byte chunk data */
	while (dest < dend) {
		writeq(*(u64 *)from, (void  __iomem *)dest);
		from += sizeof(u64);
		dest += sizeof(u64);
		if (dest == send) {
			dest -= WFR_SOP_DISTANCE;
			dend -= WFR_SOP_DISTANCE;
			goto main_no_sop;
		}
	}
	goto done;

	/*
	 * This is the same as above, but without the SOP address boost
	 * and SOP end check.
	 */
main_no_sop:
	/* write 8-byte chunk data */
	while (dest < dend) {
		writeq(*(u64 *)from, (void  __iomem *)dest);
		from += sizeof(u64);
		dest += sizeof(u64);
	}

done:
	/* save dangling u32, if any */
	*carry = (count & 1) ? *(u32 *)from : 0;

	return 2 + count;	/* PBC + data */
}

/*
 * Mid copy helper, "mixed case" - source is u64 aligned, dest is u32 aligned.
 *
 * We must write whole u64s to the chip, so we must combine u32s.
 *
 * @to: destination buffer
 * @off: DWORD offset from to, value is odd (carry contains data to copy)
 * @carry: IN/OUT - incoming data, possible outgoing data
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
 * that value extend to an illegal page as it will be part of
 * an aligned u64 read.
 */
static u32 mid_copy_mix(void __iomem *to, u32 off, u32 *carry,
			const void *from, size_t count)
{
	unsigned long dest = (unsigned long)to + (off * sizeof(u32));
	unsigned long dend;			/* 8-byte data end */
	union mix val, temp;
	u32 result;

	/* calculate 8-byte data end */
	dend = dest + ((count>>1) * sizeof(u64));

	/* stage overlap */
	val.val32[0] = *carry;

	if (off < WFR_PIO_BLOCK_SIZE/sizeof(u32)) {
		unsigned long send;		/* SOP end */
		unsigned long xend;

		dest += WFR_SOP_DISTANCE;
		dend += WFR_SOP_DISTANCE;

		/* calculate the end of data or end of block, whichever
		   comes first */
		send = dest + WFR_PIO_BLOCK_SIZE - (off * sizeof(u32));
		xend = send < dend ? send : dend;

		/* write 8-byte chunk data */
		while (dest < xend) {
			temp.val64 = *(u64 *)from;
			val.val32[1] = temp.val32[0];
			writeq(val.val64, (void  __iomem *)dest);
			val.val32[0] = temp.val32[1];
			from += sizeof(u64);
			dest += sizeof(u64);
		}

		dest -= WFR_SOP_DISTANCE;
		dend -= WFR_SOP_DISTANCE;
	}

	/* write 8-byte chunk data */
	while (dest < dend) {
		temp.val64 = *(u64 *)from;
		val.val32[1] = temp.val32[0];
		writeq(val.val64, (void  __iomem *)dest);
		val.val32[0] = temp.val32[1];
		from += sizeof(u64);
		dest += sizeof(u64);
	}

	result = count + off;
	*carry = (result & 1) ? val.val32[0] : 0;

	return result;
}

/*
 * Mid copy helper, "straight case"  - source and dest are u64 aligned
 *
 * @to: destination buffer
 * @off: DWORD offset from to, value is eve (carry contains no data to copy)
 * @carry: unused coming in, possible outgoing data
 * @from: data source, is QWORD aligned
 * @count: DWORDs to copy
 *
 * Return value: number of DWORDS accepted so far. If even, all DWORDS have
 * been written, if odd the final DWORD is in carry, not written.
 *
 * Must handle count == 0.
 */
static u32 mid_copy_straight(void __iomem *to, u32 off, u32 *carry,
			const void *from, size_t count)
{
	unsigned long dest = (unsigned long)to + (off * sizeof(u32));
	unsigned long dend;			/* 8-byte data end */
	u32 result;

	/* calculate 8-byte data end */
	dend = dest + ((count>>1) * sizeof(u64));

	if (off < WFR_PIO_BLOCK_SIZE/sizeof(u32)) {
		unsigned long send;		/* SOP end */
		unsigned long xend;

		dest += WFR_SOP_DISTANCE;
		dend += WFR_SOP_DISTANCE;

		/* calculate the end of data or end of block, whichever
		   comes first */
		send = dest + WFR_PIO_BLOCK_SIZE - (off * sizeof(u32));
		xend = send < dend ? send : dend;

		/* write 8-byte chunk data */
		while (dest < xend) {
			writeq(*(u64 *)from, (void  __iomem *)dest);
			from += sizeof(u64);
			dest += sizeof(u64);
		}

		dest -= WFR_SOP_DISTANCE;
		dend -= WFR_SOP_DISTANCE;
	}

	/* write 8-byte chunk data */
	while (dest < dend) {
		writeq(*(u64 *)from, (void  __iomem *)dest);
		from += sizeof(u64);
		dest += sizeof(u64);
	}

	result = count + off;
	*carry = (result & 1) ? *(u32 *)from : 0;

	return result;
}

/*
 * Segmented PIO Copy - middle
 *
 * Must handle DWORD aligned tail (off is odd) and DWORD aligned source
 * (from).
 *
 * @to: destination buffer
 * @off: DWORD offset from to
 * @carry: IN/OUT - possible incoming data, possible outgoing data
 * @from: data source
 * @count: DWORDs to copy
 *
 * Return value: number of DWORDS accepted so far. If even, all DWORDS have
 * been written, if odd the final DWORD is in carry, not written.
 */
u32 seg_pio_copy_mid(void __iomem *to, u32 off, u32 *carry,
			const void *from, size_t count)
{
	if ((unsigned long)from & 0x2) {	/* DWORD aligned source */
		if (off & 1) {			/* DWORD aligned tail */
			/*
			 * Case 0: u32 aligned source, u32 aligned dest
			 *
			 * Do one combined write, so source and dest are
			 * both u64 aligned, then do "straight" copy.
			 */
			unsigned long dest;
			union mix val;

			dest = (unsigned long)to + ((off >> 1) * sizeof(u64));
			if (off < WFR_PIO_BLOCK_SIZE/sizeof(u32))
				dest += WFR_SOP_DISTANCE;

			val.val32[0] = *carry;
			val.val32[1] = *(u32 *)from;

			writeq(val.val64, (void  __iomem *)dest);
			return mid_copy_straight(to, off+1, carry,
					from+sizeof(u32), count-1);
		} else {			/* QWORD aligned tail */
			/*
			 * Case 1: u32 aligned source, u64 aligned dest
			 *
			 * Read the first u32 of source so it is u64 aligned,
			 * then do a "mixed" copy.
			 */
			*carry = *(u32 *)from;
			return mid_copy_mix(to, off+1, carry,
						from + sizeof(u32), count-1);
		}
	} else {				/* QWORD aligned source */
		if (off & 1) {			/* DWORD aligned tail */
			/*
			 * Case 2: u64 aligned source, u32 aligned dest
			 *
			 * Do a "mixed" copy.
			 */
			return mid_copy_mix(to, off, carry, from, count);
		} else {			/* QWORD aligned tail */
			/*
			 * Case 3: u64 aligned source, u64 aligned dest
			 *
			 * Everything aligns, do a "straight" copy.
			 */
			return mid_copy_straight(to, off, carry, from, count);
		}
	}
}

/*
 * Segmented PIO Copy - end
 *
 * Write any remainder (in carry) and finish writing the whole block.
 *
 * @to: destination buffer
 * @off: DWORD offset from to
 * @carry: possible incoming data
 */
void seg_pio_copy_end(void __iomem *to, u32 off, u32 carry)
{
	/* off may be odd, truncate to calculate an aligned u64 address */
	unsigned long dest = (unsigned long)to + ((off>>1) * sizeof(u64));

	if (off < (WFR_PIO_BLOCK_SIZE >> 2))
		dest += WFR_SOP_DISTANCE;

	/*
	 * If off is odd, then we have a dangling u32 in carry that needs
	 * to be written.  
	 */
	if (off & 1) {
		union mix val;

		val.val64 = 0;
		val.val32[0] = carry;

		writeq(val.val64, (void  __iomem *)dest);
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
	while ((dest & WFR_PIO_BLOCK_MASK) != 0) {
		writeq(0, (void  __iomem *)dest);
		dest += sizeof(u64);
	}
}
