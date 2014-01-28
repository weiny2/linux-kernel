#ifndef _PIO_H
#define _PIO_H
/*
 * Copyright (c) 2013 Intel Corporation.  All rights reserved.
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


/* send context types */
#define SC_KERNEL 0
#define SC_ACK    1
#define SC_USER   2
#define SC_MAX    3

/* PIO buffer release callback function */
typedef void (*pio_release_cb)(void *arg);

/* byte helper */
union mix {
	u64 val64;
	u32 val32[2];
	u8  val8[8];
};

/* an allocated PIO buffer */
struct pio_buf {
	pio_release_cb cb;	/* called when the buffer is released */
	void *arg;		/* argument for cb */
	void __iomem *start;	/* buffer start address */
	void __iomem *end;	/* context end address */
	unsigned long size;	/* context size, in bytes */
	unsigned long sent_at;	/* buffer is sent when <= free */
	u32 block_count;	/* size of buffer, in blocks */
	u32 qw_written;		/* QW written so far */
	u32 carry_bytes;	/* number of valid bytes in carry */
	union mix carry;	/* pending unwritten bytes */
};

/* cache line aligned pio buffer array */
union pio_shadow_ring {
	struct pio_buf pbuf;
	u64 unused[8];		/* cache line spacer */
} ____cacheline_aligned;

/* per-NUMA send context */
struct send_context {
	/* read-only after init */
	struct hfi_devdata *dd;		/* device */
	void __iomem *base_addr;	/* start of PIO memory */
	union pio_shadow_ring *sr;	/* shadow ring */
	volatile __le64 *hw_free;	/* HW free counter */
	u32 context;			/* context number */
	u32 credits;			/* number of blocks in context */
	u32 sr_size;			/* size of the shadow ring */
	u32 group;			/* credit return group */
	u16 enabled;                    /* is the send ctxt enabled */
	/* allocator fields */
	spinlock_t alloc_lock ____cacheline_aligned_in_smp;
	unsigned long fill;		/* official alloc count */
	unsigned long alloc_free;	/* copy of free (less cache thrash) */
	u32 sr_head;			/* shadow ring head */
	/* releaser fields */
	spinlock_t release_lock ____cacheline_aligned_in_smp;
	unsigned long free;		/* official free count */
	u32 sr_tail;			/* shadow ring tail */
	spinlock_t wait_lock  ____cacheline_aligned_in_smp;
	struct list_head piowait;       /* list for PIO waiters */
	u64 credit_ctrl;		/* cache for credit control */
};

struct send_context_info {
	struct send_context *sc;	/* allocated working context */
	int allocated;			/* compare and exchange interlock */
	u16 type;			/* context type */
	u16 base;			/* base in PIO array */
	u16 credits;			/* size in PIO array */
};

/* DMA credit return, index is always (context & 0x7) */
struct credit_return {
	volatile __le64 cr[8];
};

/* NUMA indexed credit return array */
struct credit_return_base {
	struct credit_return *va;
	dma_addr_t pa;
};

/* send context configuration sizes (one per type) */
struct sc_config_sizes {
	short int size;
	short int count;
};

/* send context functions */
int init_credit_return(struct hfi_devdata *dd);
void free_credit_return(struct hfi_devdata *dd);
int init_sc_pools_and_sizes(struct hfi_devdata *dd);
int init_send_contexts(struct hfi_devdata *dd);
int init_credit_return(struct hfi_devdata *dd);
struct send_context *sc_alloc(struct hfi_devdata *dd, int type, int numa);
void sc_free(struct send_context *sc);
int sc_enable(struct send_context *sc);
void sc_disable(struct send_context *sc);
void sc_return_credits(struct send_context *sc);
void sc_flush(struct send_context *sc);
void sc_drop(struct send_context *sc);
u64 create_pbc(struct send_context *, u64, u32, u32, u32);
struct pio_buf *sc_buffer_alloc(struct send_context *sc, u32 dw_len,
			pio_release_cb cb, void *arg);
void sc_release_update(struct send_context *sc);
void sc_group_release_update(struct send_context *sc);
void sc_wantpiobuf_intr(struct send_context *sc, u32 needint);

/* global PIO send control operations */
#define PSC_GLOBAL_ENABLE 0
#define PSC_GLOBAL_DISABLE 1
#define PSC_GLOBAL_VLARB_ENABLE 2
#define PSC_GLOBAL_VLARB_DISABLE 3

void pio_send_control(struct hfi_devdata *dd, int op);


/* PIO copy routines */
void pio_copy(struct pio_buf *pbuf, u64 pbc, const void *from, size_t count);
void seg_pio_copy_start(struct pio_buf *pbuf, u64 pbc,
					const void *from, size_t nbytes);
void seg_pio_copy_mid(struct pio_buf *pbuf, const void *from, size_t nbytes);
void seg_pio_copy_end(struct pio_buf *pbuf);

#endif /* _PIO_H */
