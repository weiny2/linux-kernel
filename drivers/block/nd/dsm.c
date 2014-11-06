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

#include <linux/module.h>
#include <linux/ndctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "dsm.h"
#include "nfit.h"

/* the mailbox is at 0xf000100000, the large input payload is at 0xf00020000
 * (1MB later), and the large output payload is at 0xf00030000 (another 1MB
 * later).  For simplicity we just ioremap the whole 3MB at once, and set the
 * pointers up manually.
 */
#define CR_MB_TOTAL_SIZE (3 * CR_IN_MB_SIZE)
phys_addr_t	mb_phys_addr = 0xf000100000; /* single simics dimm config */
void		*mb_addr;
struct cr_mailbox cr_mb;

static void print_fw_cmd(struct fv_fw_cmd *fw_cmd)
{
	pr_debug("***FW_CMD***\n"
		"OpCode: %#hhx\n"
		"SubOPcode: %#hhx\n"
		"flags: %#x\n"
		"Large Payload Offset:%u\n"
		"Input Payload Size: %u\n"
		"Large Input Payload Size: %u\n"
		"Output Payload Size: %u\n"
		"Large Output Payload Size: %u\n",
		fw_cmd->opcode,
		fw_cmd->sub_opcode,
		fw_cmd->flags,
		fw_cmd->large_payload_offset,
		fw_cmd->input_payload_size,
		fw_cmd->large_input_payload_size,
		fw_cmd->output_payload_size,
		fw_cmd->large_output_payload_size);
}

/* Offset from the start of the CTRL region to the start of the OS mailbox */
int cr_verify_fw_cmd(struct fv_fw_cmd *fw_cmd)
{
	if (fw_cmd->input_payload_size > CR_IN_PAYLOAD_SIZE ||
		fw_cmd->output_payload_size > CR_OUT_PAYLOAD_SIZE ||
		fw_cmd->large_input_payload_size > CR_IN_MB_SIZE ||
		fw_cmd->large_output_payload_size > CR_OUT_MB_SIZE)
		return -EINVAL;

	return 0;
}


void cr_write_cmd_op(struct cr_mailbox *mb, struct fv_fw_cmd *fw_cmd)
{
	__u64 command = 0;

	command = ((__u64) 1 << DB_SHIFT
		| (__u64) fw_cmd->opcode << OP_SHIFT)
		| ((__u64) fw_cmd->sub_opcode << SUB_OP_SHIFT);

	__raw_writeq(command, mb->command);
}

void cr_memcopy_outpayload(struct cr_mailbox *mb,
	struct fv_fw_cmd *fw_cmd)
{
	unsigned char *to;
	size_t remain = fw_cmd->output_payload_size & (CR_REG_SIZE - 1);
	int segments;
	int i;

	to = fw_cmd->output_payload;

	segments = fw_cmd->output_payload_size / CR_REG_SIZE;

	for (i = 0; i < segments; i++) {
		memcpy_fromio(to, mb->out_payload[i], CR_REG_SIZE);
		to += CR_REG_SIZE;
	}

	if (remain)
		memcpy_fromio(to, mb->out_payload[i], remain);
}

#define MB_COMPLETE 0x1

static void cr_poll_fw_cmd_completion(struct cr_mailbox *mb)
{
	__u64 status;

	do {
		status = readq(mb->status);
	} while (!(status & MB_COMPLETE));
}

void cr_read_large_outpayload(struct cr_mailbox *mb,
	struct fv_fw_cmd *fw_cmd)
{
	memcpy_fromio(fw_cmd->large_output_payload,
			mb->mb_out[0] + fw_cmd->large_payload_offset,
			fw_cmd->large_output_payload_size);
}

void cr_read_large_inpayload(struct cr_mailbox *mb,
	struct fv_fw_cmd *fw_cmd)
{
	memcpy_fromio(fw_cmd->large_output_payload,
			mb->mb_in[0] + fw_cmd->large_payload_offset,
			fw_cmd->large_output_payload_size);
}

void cr_memcopy_inpayload(struct cr_mailbox *mb,
	struct fv_fw_cmd *fw_cmd)
{
	unsigned char *from;
	size_t remain = fw_cmd->input_payload_size & (CR_REG_SIZE - 1);
	int segments;
	int i;

	for (i = 0; i < ARRAY_SIZE(mb->in_payload); i++)
		memset_io(mb->in_payload[i], 0, CR_REG_SIZE);

	from = fw_cmd->input_payload;

	segments = fw_cmd->input_payload_size / CR_REG_SIZE;

	for (i = 0; i < segments; i++) {
		memcpy_toio(mb->in_payload[i], from, CR_REG_SIZE);
		from += CR_REG_SIZE;
	}

	if (remain)
		memcpy_toio(mb->in_payload[i], from, remain);
}

void cr_write_large_inpayload(struct cr_mailbox *mb,
	struct fv_fw_cmd *fw_cmd)
{
	memcpy_toio(mb->mb_in[0] + fw_cmd->large_payload_offset,
			fw_cmd->large_input_payload,
			fw_cmd->large_input_payload_size);
}

int cr_send_command(struct fv_fw_cmd *fw_cmd, struct cr_mailbox *mb)
{
	u8 status = 0;

	if (cr_verify_fw_cmd(fw_cmd))
		return -EINVAL;

	if (fw_cmd->input_payload_size > 0)
		cr_memcopy_inpayload(mb, fw_cmd);
	if (fw_cmd->large_input_payload_size > 0)
		cr_write_large_inpayload(mb, fw_cmd);

	if (fw_cmd->flags != FNV_BIOS_FLAG) {
		/* BUG: Simics MB bug never resets status code */
		memset_io(mb->status, 0, CR_REG_SIZE);

		cr_write_cmd_op(mb, fw_cmd);

		cr_poll_fw_cmd_completion(mb);

		status = (readq(mb->status) & STATUS_MASK) >> STATUS_SHIFT;
	}

	/*
	 * even if we have bad mailbox status, copy the out payloads so that
	 * the user can look at the embedded firmware status for more
	 * information
	 */
	if (fw_cmd->output_payload_size > 0)
		cr_memcopy_outpayload(mb, fw_cmd);
	if (fw_cmd->large_output_payload_size > 0) {
		if (fw_cmd->sub_opcode == FNV_BIOS_SUBOP_READ_OUTPUT)
			cr_read_large_outpayload(mb, fw_cmd);
		else /* sub_opcode == FNV_BIOS_SUBOP_READ_INPUT */
			cr_read_large_inpayload(mb, fw_cmd);
	}

	if (status)
		return -EINVAL;

	return 0;
}

unsigned long nd_manual_dsm = (!!IS_ENABLED(CONFIG_ND_MANUAL_DSM) << NFIT_CMD_VENDOR);
EXPORT_SYMBOL(nd_manual_dsm);
module_param(nd_manual_dsm, ulong, S_IRUGO);
MODULE_PARM_DESC(nd_manual_dsm,
		"Manually override _DSM commands instead of bus provided routines");

int nd_dsm_passthru(void *buf, unsigned int buf_len)
{
	struct nfit_cmd_vendor_hdr *in = buf;
	struct nfit_cmd_vendor_tail *out = buf + sizeof(*in) + in->in_length;
	struct fnv_passthru_cmd	*cmd = (struct fnv_passthru_cmd *)in->in_buf;
	struct fv_fw_cmd fw_cmd;
	int ret, input_payload_size;

	if ((cmd->flags == FNV_BIOS_FLAG) && cmd->opcode == FNV_BIOS_OPCODE &&
	    cmd->sub_opcode == FNV_BIOS_SUBOP_GET_SIZE) {
		struct fnv_bios_get_size *fnv_size =
			(struct fnv_bios_get_size *)out->out_buf;

		fnv_size->input_size = CR_IN_MB_SIZE;
		fnv_size->output_size = CR_OUT_MB_SIZE;
		return 0;
	}

	memset(&fw_cmd, 0, sizeof(fw_cmd));

	fw_cmd.opcode     = cmd->opcode;
	fw_cmd.sub_opcode = cmd->sub_opcode;
	fw_cmd.flags	  = cmd->flags;

	if (fw_cmd.flags == FNV_BIOS_FLAG) {
		struct fnv_bios_input *bios_input;

		bios_input = (struct fnv_bios_input *)cmd->in_buf;
		input_payload_size = in->in_length - sizeof(*cmd) -
					sizeof(*bios_input);

		if (fw_cmd.opcode == FNV_BIOS_OPCODE &&
		    fw_cmd.sub_opcode == FNV_BIOS_SUBOP_WRITE_INPUT &&
		    input_payload_size + bios_input->offset <= CR_IN_MB_SIZE) {
			fw_cmd.large_input_payload_size	= input_payload_size;
			fw_cmd.large_input_payload	= bios_input->buffer;
			fw_cmd.large_payload_offset	= bios_input->offset;
		} else if (input_payload_size) {
			pr_err("%s invalid input payload size %d\n",
					__func__, input_payload_size);
			return -EINVAL;
		}

		if (fw_cmd.opcode == FNV_BIOS_OPCODE &&
		    (fw_cmd.sub_opcode == FNV_BIOS_SUBOP_READ_INPUT ||
		     fw_cmd.sub_opcode == FNV_BIOS_SUBOP_READ_OUTPUT) &&
		    out->out_length + bios_input->offset <= CR_OUT_MB_SIZE) {
			fw_cmd.large_output_payload_size = out->out_length;
			fw_cmd.large_output_payload	 = out->out_buf;
			fw_cmd.large_payload_offset	 = bios_input->offset;
		} else if (out->out_length) {
			pr_err("%s invalid output payload size %d\n",
					__func__, out->out_length);
			return -EINVAL;
		}
	} else {
		input_payload_size = in->in_length - sizeof(*cmd);

		if (input_payload_size <= CR_IN_PAYLOAD_SIZE) {
			fw_cmd.input_payload_size = input_payload_size;
			fw_cmd.input_payload	  = cmd->in_buf;
		} else {
			pr_err("%s invalid input payload size %d\n",
					__func__, input_payload_size);
			return -EINVAL;
		}

		if (out->out_length <= CR_OUT_PAYLOAD_SIZE) {
			fw_cmd.output_payload_size = out->out_length;
			fw_cmd.output_payload	   = out->out_buf;
		} else {
			pr_err("%s invalid output payload size %d\n",
					__func__, out->out_length);
			return -EINVAL;
		}
	}

	print_fw_cmd(&fw_cmd);

	mutex_lock(&cr_mb.mb_lock);
	ret = cr_send_command(&fw_cmd, &cr_mb);
	mutex_unlock(&cr_mb.mb_lock);

	return ret;
}

static __init int nd_dsm_init(void)
{
	if (!nd_manual_dsm)
		return 0;

	mb_addr = ioremap_nocache(mb_phys_addr, CR_MB_TOTAL_SIZE);
	if (unlikely(!mb_addr))
		return -ENXIO;

	cr_mb.command		= mb_addr + CR_MB_COMMAND_OFFSET;
	cr_mb.nonce0		= mb_addr + CR_MB_NONCE0_OFFSET;
	cr_mb.nonce1		= mb_addr + CR_MB_NONCE1_OFFSET;
	cr_mb.in_payload[0]	= mb_addr + CR_MB_IN_PAYLOAD0_OFFSET;
	cr_mb.in_payload[1]	= mb_addr + CR_MB_IN_PAYLOAD1_OFFSET;
	cr_mb.in_payload[2]	= mb_addr + CR_MB_IN_PAYLOAD2_OFFSET;
	cr_mb.in_payload[3]	= mb_addr + CR_MB_IN_PAYLOAD3_OFFSET;
	cr_mb.in_payload[4]	= mb_addr + CR_MB_IN_PAYLOAD4_OFFSET;
	cr_mb.in_payload[5]	= mb_addr + CR_MB_IN_PAYLOAD5_OFFSET;
	cr_mb.in_payload[6]	= mb_addr + CR_MB_IN_PAYLOAD6_OFFSET;
	cr_mb.in_payload[7]	= mb_addr + CR_MB_IN_PAYLOAD7_OFFSET;
	cr_mb.in_payload[8]	= mb_addr + CR_MB_IN_PAYLOAD8_OFFSET;
	cr_mb.in_payload[9]	= mb_addr + CR_MB_IN_PAYLOAD9_OFFSET;
	cr_mb.in_payload[10]	= mb_addr + CR_MB_IN_PAYLOAD10_OFFSET;
	cr_mb.in_payload[11]	= mb_addr + CR_MB_IN_PAYLOAD11_OFFSET;
	cr_mb.in_payload[12]	= mb_addr + CR_MB_IN_PAYLOAD12_OFFSET;
	cr_mb.in_payload[13]	= mb_addr + CR_MB_IN_PAYLOAD13_OFFSET;
	cr_mb.in_payload[14]	= mb_addr + CR_MB_IN_PAYLOAD14_OFFSET;
	cr_mb.in_payload[15]	= mb_addr + CR_MB_IN_PAYLOAD15_OFFSET;
	cr_mb.status		= mb_addr + CR_MB_STATUS_OFFSET;
	cr_mb.out_payload[0]	= mb_addr + CR_MB_OUT_PAYLOAD0_OFFSET;
	cr_mb.out_payload[1]	= mb_addr + CR_MB_OUT_PAYLOAD1_OFFSET;
	cr_mb.out_payload[2]	= mb_addr + CR_MB_OUT_PAYLOAD2_OFFSET;
	cr_mb.out_payload[3]	= mb_addr + CR_MB_OUT_PAYLOAD3_OFFSET;
	cr_mb.out_payload[4]	= mb_addr + CR_MB_OUT_PAYLOAD4_OFFSET;
	cr_mb.out_payload[5]	= mb_addr + CR_MB_OUT_PAYLOAD5_OFFSET;
	cr_mb.out_payload[6]	= mb_addr + CR_MB_OUT_PAYLOAD6_OFFSET;
	cr_mb.out_payload[7]	= mb_addr + CR_MB_OUT_PAYLOAD7_OFFSET;
	cr_mb.out_payload[8]	= mb_addr + CR_MB_OUT_PAYLOAD8_OFFSET;
	cr_mb.out_payload[9]	= mb_addr + CR_MB_OUT_PAYLOAD9_OFFSET;
	cr_mb.out_payload[10]	= mb_addr + CR_MB_OUT_PAYLOAD10_OFFSET;
	cr_mb.out_payload[11]	= mb_addr + CR_MB_OUT_PAYLOAD11_OFFSET;
	cr_mb.out_payload[12]	= mb_addr + CR_MB_OUT_PAYLOAD12_OFFSET;
	cr_mb.out_payload[13]	= mb_addr + CR_MB_OUT_PAYLOAD13_OFFSET;
	cr_mb.out_payload[14]	= mb_addr + CR_MB_OUT_PAYLOAD14_OFFSET;
	cr_mb.out_payload[15]	= mb_addr + CR_MB_OUT_PAYLOAD15_OFFSET;

	cr_mb.mb_in_line_size		= CR_IN_MB_SIZE;
	cr_mb.mb_out_line_size		= CR_OUT_MB_SIZE;
	cr_mb.num_mb_in_segments	= 1;
	cr_mb.num_mb_out_segments	= 1;

	cr_mb.mb_in  = kmalloc(sizeof(void *), GFP_KERNEL);
	cr_mb.mb_out = kmalloc(sizeof(void *), GFP_KERNEL);

	cr_mb.mb_in[0]  = mb_addr + CR_IN_MB_SIZE;
	cr_mb.mb_out[0] = mb_addr + CR_OUT_MB_SIZE * 2;

	mutex_init(&cr_mb.mb_lock);

	return 0;
}

static __exit void nd_dsm_exit(void)
{
	if (!nd_manual_dsm)
		return;

	kfree(cr_mb.mb_in);
	kfree(cr_mb.mb_out);

	iounmap(mb_addr);
}

int nd_dsm_ctl(struct nfit_bus_descriptor *nfit_desc,
		unsigned int cmd, void *buf, unsigned int buf_len)
{
	int rc;

	switch (cmd) {
	case NFIT_CMD_VENDOR:
		rc = nd_dsm_passthru(buf, buf_len);
		break;
	default:
		rc = -EOPNOTSUPP;
		break;
	}
	pr_debug("%s: %s cmd: %d buf_len: %d rc: %d\n",
			nfit_desc->provider_name ? nfit_desc->provider_name
			: "nd_dsm", __func__, cmd, buf_len, rc);
	return rc;

}
EXPORT_SYMBOL(nd_dsm_ctl);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");
module_init(nd_dsm_init);
module_exit(nd_dsm_exit);
