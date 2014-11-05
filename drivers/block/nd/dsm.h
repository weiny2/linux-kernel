/*
 * Copyright(c) 2014 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef __DSM_H__
#define __DSM_H__

#include "nfit.h"

#if IS_ENABLED(CONFIG_ND_MANUAL_DSM)
int nd_dsm_ctl(struct nfit_bus_descriptor *nfit_desc, unsigned int cmd,
		void *buf, unsigned int buf_len);

extern unsigned long nd_manual_dsm;
#else
static inline int nd_dsm_ctl(struct nfit_bus_descriptor *nfit_desc,
		unsigned int cmd, void *buf, unsigned int buf_len)
{
	return -ENOTTY;
}

static unsigned long nd_manual_dsm;
#endif

struct cr_mailbox {
	struct mutex mb_lock;
	__u64 __iomem *command; /* va of the command register */
	__u64 __iomem *nonce0; /* va of the nonce0 register */
	__u64 __iomem *nonce1; /* va of the nonce1 register */
	/* va of the 16 in payload registers write only*/
	__u64 __iomem *in_payload[16];
	__u64 __iomem *status; /*va of the status register*/
	/* va of the 8 out payload registers read only */
	__u64 __iomem *out_payload[16];
	__u32 mb_in_line_size;
	__u32 mb_out_line_size;
	__u32 num_mb_in_segments; /* number of segments of the IN mailbox  */
	__u32 num_mb_out_segments; /* number of segments of the OUT mailbox  */
	void __iomem **mb_in; /* va of the IN mailbox segments */
	void __iomem **mb_out; /* va of the OUT mailbox segments */
};

/* Real HW Offsets */
/* Offset from the start of the OS mailbox */
enum {
	CR_MB_COMMAND_OFFSET		= 0x000,
	CR_MB_NONCE0_OFFSET		= 0x040,
	CR_MB_NONCE1_OFFSET		= 0x080,
	CR_MB_IN_PAYLOAD0_OFFSET	= 0x0C0,
	CR_MB_IN_PAYLOAD1_OFFSET	= 0x100,
	CR_MB_IN_PAYLOAD2_OFFSET	= 0x140,
	CR_MB_IN_PAYLOAD3_OFFSET	= 0x180,
	CR_MB_IN_PAYLOAD4_OFFSET	= 0x1C0,
	CR_MB_IN_PAYLOAD5_OFFSET	= 0x200,
	CR_MB_IN_PAYLOAD6_OFFSET	= 0x240,
	CR_MB_IN_PAYLOAD7_OFFSET	= 0x280,
	CR_MB_IN_PAYLOAD8_OFFSET	= 0x2C0,
	CR_MB_IN_PAYLOAD9_OFFSET	= 0x300,
	CR_MB_IN_PAYLOAD10_OFFSET	= 0x340,
	CR_MB_IN_PAYLOAD11_OFFSET	= 0x380,
	CR_MB_IN_PAYLOAD12_OFFSET	= 0x3C0,
	CR_MB_IN_PAYLOAD13_OFFSET	= 0x400,
	CR_MB_IN_PAYLOAD14_OFFSET	= 0x440,
	CR_MB_IN_PAYLOAD15_OFFSET	= 0x480,
	CR_MB_STATUS_OFFSET		= 0x4C0,
	CR_MB_OUT_PAYLOAD0_OFFSET	= 0x500,
	CR_MB_OUT_PAYLOAD1_OFFSET	= 0x540,
	CR_MB_OUT_PAYLOAD2_OFFSET	= 0x580,
	CR_MB_OUT_PAYLOAD3_OFFSET	= 0x5C0,
	CR_MB_OUT_PAYLOAD4_OFFSET	= 0x600,
	CR_MB_OUT_PAYLOAD5_OFFSET	= 0x640,
	CR_MB_OUT_PAYLOAD6_OFFSET	= 0x680,
	CR_MB_OUT_PAYLOAD7_OFFSET	= 0x6C0,
	CR_MB_OUT_PAYLOAD8_OFFSET	= 0x700,
	CR_MB_OUT_PAYLOAD9_OFFSET	= 0x740,
	CR_MB_OUT_PAYLOAD10_OFFSET	= 0x780,
	CR_MB_OUT_PAYLOAD11_OFFSET	= 0x7C0,
	CR_MB_OUT_PAYLOAD12_OFFSET	= 0x800,
	CR_MB_OUT_PAYLOAD13_OFFSET	= 0x840,
	CR_MB_OUT_PAYLOAD14_OFFSET	= 0x880,
	CR_MB_OUT_PAYLOAD15_OFFSET	= 0x8C0,
};

/*
 * The struct defining the passthrough command and payloads to be operated
 * upon by the FV firmware.
 */
struct fv_fw_cmd {
	unsigned int id; /* The physical ID of the memory module */
	unsigned char opcode; /* The command opcode. */
	unsigned char sub_opcode; /* The command sub-opcode. */
	unsigned int input_payload_size; /* The size of the input payload */
	void *input_payload; /* A pointer to the input payload buffer */
	unsigned int output_payload_size; /* The size of the output payload */
	void *output_payload; /* A pointer to the output payload buffer */
	unsigned int large_input_payload_size; /* Size large input payload */
	void *large_input_payload; /* A pointer to the large input buffer */
	unsigned int large_output_payload_size;/* Size large output payload */
	void *large_output_payload; /* A pointer to the large output buffer */
};

#define CR_IN_PAYLOAD_SIZE (128)
#define CR_OUT_PAYLOAD_SIZE (128)
#define CR_IN_MB_SIZE (1 << 20)
#define CR_OUT_MB_SIZE (1 << 20)
#define CR_REG_SIZE 8
#define DB_SHIFT 48
#define SUB_OP_SHIFT 40
#define OP_SHIFT 32
#define STATUS_MASK 0xFF00
#define STATUS_SHIFT 8

#endif /* __DSM_H__ */
