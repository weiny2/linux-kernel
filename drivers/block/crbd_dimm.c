/*
 * Copyright 2013 Intel Corporation All Rights Reserved.
 *
 * INTEL CONFIDENTIAL
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material may contain trade secrets and
 * proprietary and confidential information of Intel Corporation and its
 * suppliers and licensors, and is protected by worldwide copyright and trade
 * secret laws and treaty provisions. No part of the Material may be used,
 * copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express written
 * permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or alter
 * this notice or any other notice embedded in Materials by Intel or Intel's
 * suppliers or licensors in any way.
 */

#include <linux/nvdimm_core.h>
#include <linux/crbd_dimm.h>
#include <linux/cr_ioctl.h>

static void print_fw_cmd(struct fv_fw_cmd *fw_cmd)
{
	NVDIMM_DBG("***FW_CMD***\n"
		"Handle: %#hx\n"
		"OpCode: %#hhx\n"
		"SubOPcode: %#hhx\n"
		"Input Payload Size: %u\n"
		"Large Input Payload Size: %u\n"
		"Output Payload Size: %u\n"
		"Large Output Payload Size: %u",
		fw_cmd->id,
		fw_cmd->opcode,
		fw_cmd->sub_opcode,
		fw_cmd->input_payload_size,
		fw_cmd->large_input_payload_size,
		fw_cmd->output_payload_size,
		fw_cmd->large_output_payload_size);
}

/**
 * TODO: Shockingly the MB flow is changing a bit due to the inclusion
 * of a sequence bit. FIS > 0.65 should include this change.
 */

/**
 * cr_poll_fw_cmd_completion() - Poll Firmware Command Completion
 * @mb - The mailbox the fw cmd was submitted on
 *
 * Poll the status register of the mailbox waiting for the
 * mailbox complete bit to be set
 */
static void cr_poll_fw_cmd_completion(struct cr_mailbox *mb)
{
	__u64 status;
	do {
		status = readq(mb->status);
	} while (!(status & MB_COMPLETE));
}

/**
 * cr_set_mb_door_bell() - Set Mailbox Door Bell
 * @mb: The mailbox to initiate a fw command
 *
 * Set the door bell bit of the command register to inform
 * FW to begin processing the FW command
 */
static void cr_set_mb_door_bell(struct cr_mailbox *mb)
{
	__u64 command = readq(mb->command) | (1 << DB_SHIFT);

	__raw_writeq(command, mb->command);
	__builtin_ia32_pcommit();/*TODO: May not be needed*/
	cr_write_barrier();
}

/**
 * cr_write_cmd_op() - Write Command Op Codes
 * @mb: mailbox to write the command op codes to
 * @fw_cmd: Firmware command to execute
 *
 * Write the opcode and the subop code to the command register
 * of the mailbox
 */
void cr_write_cmd_op(struct cr_mailbox *mb, struct fv_fw_cmd *fw_cmd)
{
	__u64 command = 0;

	command = fw_cmd->opcode | (fw_cmd->sub_opcode << SUB_OP_SHIFT);

	__raw_writeq(command, mb->command);
}

/**
 * cr_memcopy_large_inpayload() - MemCopy Large InPayload
 * @mb: the mailbox to write the large payload to
 * @fw_cmd: The firmware command with the largepayload to place into
 * the mailbox
 *
 * Move the large input payload from a fw command structure to
 * a mailbox that has been interleaved across the physical address
 * space.
 */
void cr_memcopy_large_inpayload(struct cr_mailbox *mb,
	struct fv_fw_cmd *fw_cmd)
{
	unsigned char *from;
	size_t remain = fw_cmd->large_input_payload_size
		& (mb->mb_in_line_size - 1);
	int segments;
	int i;

	for (i = 0; i < mb->num_mb_in_segments; i++)
		memset_io(mb->mb_in[i], 0, mb->mb_in_line_size);

	cr_write_barrier();

	from = fw_cmd->large_input_payload;

	segments = fw_cmd->large_input_payload_size / mb->mb_in_line_size;

	for (i = 0; i < segments; i++) {
		memcpy_toio(mb->mb_in[i], from, mb->mb_in_line_size);
		from += mb->mb_in_line_size;
	}

	if (remain)
		memcpy_toio(mb->mb_in[i], from, remain);
}

/**
 * cr_memcopy_large_outpayload() - MemCopy Large Outpayload
 * @mb: The mailbox to retrieve the large payload from
 * @fw_cmd: The fw command to place the large payload in
 *
 * Move the completed large payload from the interleaved system memory
 * mailbox to the fw command structure
 */
void cr_memcopy_large_outpayload(struct cr_mailbox *mb,
	struct fv_fw_cmd *fw_cmd)
{
	unsigned char *to;
	size_t remain = fw_cmd->large_output_payload_size
		& (mb->mb_out_line_size - 1);
	int segments;
	int i;

	to = fw_cmd->large_output_payload;

	segments = fw_cmd->large_output_payload_size / mb->mb_out_line_size;

	for (i = 0; i < segments; i++) {
		memcpy_fromio(to, mb->mb_out[i], mb->mb_out_line_size);
		to += mb->mb_out_line_size;
	}

	if (remain)
		memcpy_fromio(to, mb->mb_out[i], remain);
}

/**
 * cr_memcopy_inpayload() - MemCopy Inpayload
 * @mb: The mailbox to write the payload to
 * @fw_cmd: The firmware command structure to read the payload from
 *
 * Move the input payload from the fw cmd to the in_payload mb registers
 */
void cr_memcopy_inpayload(struct cr_mailbox *mb,
	struct fv_fw_cmd *fw_cmd)
{
	unsigned char *from;
	size_t remain = fw_cmd->input_payload_size & (CR_REG_SIZE - 1);
	int segments;
	int i;

	for (i = 0; i < ARRAY_SIZE(mb->in_payload); i++)
		memset_io(mb->in_payload[i], 0, CR_REG_SIZE);

	cr_write_barrier();

	from = fw_cmd->input_payload;

	segments = fw_cmd->input_payload_size / CR_REG_SIZE;

	for (i = 0; i < segments; i++) {
		memcpy_toio(mb->in_payload[i], from, CR_REG_SIZE);
		from += CR_REG_SIZE;
	}

	if (remain)
		memcpy_toio(mb->in_payload[i], from, remain);
}

/**
 * cr_memcopy_outpayload() - MemCopy Outpayload
 * @mb: Mailbox to read the outpayload from
 * @fw_cmd: FW command to write the outputpayload to
 *
 * Read the outpayload from the mailboxes and place it
 * into the fw command structure
 */
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

/**
 * cr_verify_fw_cmd() - Verify FW Command
 * @fw_cmd: The FW command to verify
 *
 * Perform boundary checks to ensure that the fw command can be executed
 * on FW
 *
 * Returns:
 * 0		- Success
 * -EINVAL	- Invalid FW Command Parameter
 */
int cr_verify_fw_cmd(struct fv_fw_cmd *fw_cmd)
{
	if (fw_cmd->input_payload_size > CR_IN_PAYLOAD_SIZE ||
		fw_cmd->output_payload_size > CR_OUT_PAYLOAD_SIZE ||
		fw_cmd->large_input_payload_size > CR_IN_MB_SIZE ||
		fw_cmd->large_output_payload_size > CR_OUT_MB_SIZE)
		return -EINVAL;

	return 0;
}

/**
 * cr_get_mb() - Get Mailbox
 * @c_dimm: CR DIMM to claim the mailbox for
 *
 * Enforce mutual exclusion on a mailbox for a CR DIMM
 * Only one FW Command can be sent to a CR DIMM at a time
 *
 * Return:
 * Mailbox for the CR DIMM
 */
static struct cr_mailbox *cr_get_mb(struct cr_dimm *c_dimm)
{
	struct cr_mailbox *mb = c_dimm->host_mailbox;

	mutex_lock(&mb->mb_lock);

	return mb;
}

/**
 * cr_put_mb() - Put Mailbox
 * @mb: The mailbox to release ownership of
 *
 * Release ownership of a mailbox and allow others to claim
 */
static void cr_put_mb(struct cr_mailbox *mb)
{
	mutex_unlock(&mb->mb_lock);
}

/**
 * cr_send_command() - Pass thru command to FW
 * @fw_cmd: A firmware command structure
 *
 * Sends a command to FW and waits for response from firmware
 *
 * Returns:
 * 0	- Success
 * -EIO	- FW error received
 *
 */
int cr_send_command(struct fv_fw_cmd *fw_cmd, struct cr_mailbox *mb)
{
	__u8 status;

	if (cr_verify_fw_cmd(fw_cmd))
		return -EINVAL;

	if (fw_cmd->input_payload_size > 0)
		cr_memcopy_inpayload(mb, fw_cmd);
	if (fw_cmd->large_input_payload_size > 0)
		cr_memcopy_large_inpayload(mb, fw_cmd);

	cr_write_cmd_op(mb, fw_cmd);

	cr_write_barrier();

	/* BUG: Simics MB bug never resets status code */
	memset_io(mb->status, 0, CR_REG_SIZE);

	cr_set_mb_door_bell(mb);

	cr_poll_fw_cmd_completion(mb);

	status = (readq(mb->status) & STATUS_MASK) >> STATUS_SHIFT;

	/* TODO: MB Error handling logic needs to be defined */
	if (status)
		return status;

	if (fw_cmd->output_payload_size > 0)
		cr_memcopy_outpayload(mb, fw_cmd);
	if (fw_cmd->large_output_payload_size > 0)
		cr_memcopy_large_outpayload(mb, fw_cmd);

	return 0;
}

int cr_fw_null_cmd(struct cr_dimm *c_dimm, struct fv_fw_cmd *fw_cmd,
		struct cr_mailbox *mb)
{
	return cr_send_command(fw_cmd, mb);
}

int cr_fw_id_dimm(struct cr_dimm *c_dimm, struct fv_fw_cmd *fw_cmd,
		struct cr_mailbox *mb)
{
	return cr_send_command(fw_cmd, mb);
}

int cr_fw_get_security(struct cr_dimm *c_dimm, struct fv_fw_cmd *fw_cmd,
		struct cr_mailbox *mb)
{
	return cr_send_command(fw_cmd, mb);
}

/*
 * TODO: CR_DIMM security state needs to be updated by some
 * of these commands
 */
int cr_fw_set_security(struct cr_dimm *c_dimm, struct fv_fw_cmd *fw_cmd,
		struct cr_mailbox *mb)
{
	switch (fw_cmd->sub_opcode) {
	case SUBOP_SET_NONCE:
		return cr_send_command(fw_cmd, mb);
		break;
	case SUBOP_SET_PASS:
		return cr_send_command(fw_cmd, mb);
		break;
	case SUBOP_DISABLE_PASS:
		return cr_send_command(fw_cmd, mb);
		break;
	case SUBOP_UNLOCK_UNIT:
		return cr_send_command(fw_cmd, mb);
		break;
	case SUBOP_SEC_ERASE_PREP:
		return cr_send_command(fw_cmd, mb);
		break;
	case SUBOP_SEC_ERASE_UNIT:
		return cr_send_command(fw_cmd, mb);
		break;
	case SUBOP_SEC_FREEZE_LOCK:
		return cr_send_command(fw_cmd, mb);
		break;
	default:
		NVDIMM_DBG("Unknown Get Security Sub-Op received");
		return cr_send_command(fw_cmd, mb);
	}
}

int cr_fw_get_admin_features(struct cr_dimm *c_dimm, struct fv_fw_cmd *fw_cmd,
		struct cr_mailbox *mb)
{
	switch (fw_cmd->sub_opcode) {
	case SUBOP_SYSTEM_TIME:
		return cr_send_command(fw_cmd, mb);
		break;
	case SUBOP_DIMM_PARTITION_INFO:
		return cr_send_command(fw_cmd, mb);
		break;
	default:
		NVDIMM_DBG("Unknown Get Admin Feature Sub-Op received");
		return cr_send_command(fw_cmd, mb);
	}
}

/*
 * TODO: Internal information may need to be updated on some of these FW
 * commands
 */
int cr_fw_set_admin_features(struct cr_dimm *c_dimm, struct fv_fw_cmd *fw_cmd,
		struct cr_mailbox *mb)
{
	switch (fw_cmd->sub_opcode) {
	case SUBOP_SYSTEM_TIME:
		NVDIMM_WARN("System Time cannot be set from OS");
		return cr_send_command(fw_cmd, mb);
		break;
	case SUBOP_DIMM_PARTITION_INFO:
		return cr_send_command(fw_cmd, mb);
		break;
	default:
		NVDIMM_DBG("Unknown Get Admin Feature Sub-Op received");
		return cr_send_command(fw_cmd, mb);
	}
}

/*
 * TODO: In the future only sniff out the commands we care about
 * for now though sniff all commands and leave stubs incase we need to take
 * action on any command to stay in sync with simics
 */
int cr_sniff_fw_command(struct cr_dimm *c_dimm, struct fv_fw_cmd *fw_cmd,
		struct cr_mailbox *mb)
{
	if (cr_verify_fw_cmd(fw_cmd))
		return -EINVAL;

	switch (fw_cmd->opcode) {
	case CR_PT_NULL_COMMAND:
		return cr_fw_null_cmd(c_dimm, fw_cmd, mb);
		break;
	case CR_PT_IDENTIFY_DIMM:
		return cr_fw_id_dimm(c_dimm, fw_cmd, mb);
		break;
	case CR_PT_GET_SEC_INFO:
		return cr_fw_get_security(c_dimm, fw_cmd, mb);
		break;
	case CR_PT_SET_SEC_INFO:
		return cr_fw_set_security(c_dimm, fw_cmd, mb);
		break;
	case CR_PT_GET_ADMIN_FEATURES:
		return cr_fw_get_admin_features(c_dimm, fw_cmd, mb);
		break;
	case CR_PT_SET_ADMIN_FEATURES:
		return cr_fw_set_admin_features(c_dimm, fw_cmd, mb);
		break;
	default:
		NVDIMM_DBG("Opcode: %#hhx Not Recognized",
				fw_cmd->opcode);
		return cr_send_command(fw_cmd, mb);
	}

	return 0;
}

/**
 * fw_cmd_pass_thru() - Firmware Command Pass Thru
 * @c_dimm: CR_DIMM to execute a FW command on
 * @cmd: The FW Command to execute
 *
 * Execute a FW command on a given CR DIMM
 *
 * Return:
 * Any errors from the send_command() function
 */
int fw_cmd_pass_thru(struct cr_dimm *c_dimm, struct fv_fw_cmd *cmd)
{
	struct cr_mailbox *mb;
	int ret = 0;

	print_fw_cmd(cmd);

	mb = cr_get_mb(c_dimm);

	ret = cr_sniff_fw_command(c_dimm, cmd, mb);

	cr_put_mb(mb);

	return ret;
}

/**
 * fw_cmd_get_security_info() - Firmware command get security info
 * @c_dimm: The CR DIMM to retrieve security info on
 * @payload: Area to place the security info returned from FW
 * @dimm_id: The SMBIOS table type 17 handle of the NVDIMM
 *
 * Execute a FW command to check the security status of a CR DIMM
 *
 * Return:
 * 0		- Success
 * -ENOMEM	- kalloc failure
 * Various errors from FW are still TBD
 */

/*
 * TODO: It may be possible to roll up all internal fw commands
 * into one function
 */
static
int fw_cmd_get_security_info(struct cr_dimm *c_dimm,
		struct cr_pt_payload_get_security_state *payload,
		__u16 dimm_id)
{
	struct fv_fw_cmd *fw_cmd;
	struct cr_mailbox *mb;
	int ret = 0;

	fw_cmd = kzalloc(sizeof(*fw_cmd), GFP_KERNEL);

	if (!fw_cmd)
		return -ENOMEM;

	fw_cmd->id = dimm_id;
	fw_cmd->opcode = CR_PT_GET_SEC_INFO;
	fw_cmd->output_payload_size = 8;

	fw_cmd->output_payload = kzalloc(fw_cmd->output_payload_size,
			GFP_KERNEL);

	if (!fw_cmd->output_payload) {
		ret = -ENOMEM;
		goto after_fw_alloc;
	}

	mb = cr_get_mb(c_dimm);

	ret = cr_fw_get_security(c_dimm, fw_cmd, mb);

	if (!ret)
		memcpy(payload, fw_cmd->output_payload, sizeof(*payload));

	cr_put_mb(mb);
	kfree(fw_cmd->output_payload);
after_fw_alloc:
	kfree(fw_cmd);

	return ret;
}

/**
 * fw_cmd_id_dimm() - Firmware command Identify DIMM
 * @c_dimm: The CR DIMM to retrieve identity info on
 * @payload: Area to place the identity info returned from FW
 * @dimm_id: The SMBIOS table type 17 handle of the NVDIMM
 *
 * Execute a FW command to check the security status of a CR DIMM
 *
 * Return:
 * 0		- Success
 * -ENOMEM	- kalloc failure
 * Various errors from FW are still TBD
 */

int fw_cmd_id_dimm(struct cr_dimm *c_dimm,
	struct cr_pt_payload_identify_dimm *payload,
	__u16 dimm_id)
{
	struct fv_fw_cmd *fw_cmd;
	struct cr_mailbox *mb;
	int ret = 0;

	fw_cmd = kzalloc(sizeof(*fw_cmd), GFP_KERNEL);

	if (!fw_cmd)
		return -ENOMEM;

	fw_cmd->id = dimm_id;
	fw_cmd->opcode = CR_PT_IDENTIFY_DIMM;
	fw_cmd->output_payload_size = 128;

	fw_cmd->output_payload = kzalloc(fw_cmd->output_payload_size,
			GFP_KERNEL);

	if (!fw_cmd->output_payload) {
		ret = -ENOMEM;
		goto after_fw_alloc;
	}

	mb = cr_get_mb(c_dimm);

	ret = cr_fw_id_dimm(c_dimm, fw_cmd, mb);

	if (!ret)
		memcpy(payload, fw_cmd->output_payload, sizeof(*payload));

	cr_put_mb(mb);
	kfree(fw_cmd->output_payload);
after_fw_alloc:
	kfree(fw_cmd);

	return ret;
}

/**
 * cr_free_mailbox() - Free a mailbox structure
 * @mb: Mailbox structure to free
 *
 * Frees the resources held by a mailbox
 */
void cr_free_mailbox(struct cr_mailbox *mb)
{
	kfree(mb->mb_in);
	kfree(mb->mb_out);
	kfree(mb);
}

/**
 * cr_free_block_window() - Free memory for a single block window
 * @bw: The block window to free
 *
 * Frees the resources held by a block window
 */
void cr_free_block_window(struct block_window *bw)
{
	kfree(bw->bw_apt);
	kfree(bw);
}

/**
 * cr_free_block_windows() - Free all the block windows for a given CR DIMM
 * @c_dimm: The CR DIMM to free the block windows for
 *
 * For each block window in a DIMM remove it from the bw list and free the
 * memory allocated for the block window.
 */

void cr_free_block_windows(struct cr_dimm *c_dimm)
{
	struct block_window *bw, *next;
	list_for_each_entry_safe(bw, next, &c_dimm->bw_list, bw_node) {
		list_del(&bw->bw_node);
		cr_free_block_window(bw);
	}
}

/**
 * cr_create_os_mailbox() - Create and Configure the OS Mailbox
 * @c_dimm: The CR DIMM to create the OS mailbox for
 * @i_tbl: the interleave table referenced by the mdsarmt_tbl
 *
 * Using the memdev to spa range table, determine the location of the OS mailbox
 * in the system physical address space. For each piece of the mailbox in SPA
 * map them into the virtual address space and record the location.
 *
 * Return:
 * Success - The pointer to the completed mailbox structure
 * Error - ERR_PTR on error
 */

/*
 * TODO: This is a very long function can anything be done to make it cleaner?
 */
struct cr_mailbox *cr_create_mailbox(struct cr_dimm *c_dimm,
	struct interleave_tbl *i_tbl)
{
	int err = 0;
	struct cr_mailbox *mb;
	int i;

	mb = kzalloc(sizeof(*mb), GFP_KERNEL);

	if (!mb) {
		NVDIMM_DBG("Unable to allocate mailbox");
		err = -ENOMEM;
		goto out;
	}

	mb->command = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_COMMAND_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->command)
		goto after_mb;

	mb->nonce0 = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_NONCE0_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->nonce0)
		goto after_mb;

	mb->nonce1 = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_NONCE1_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->nonce1)
		goto after_mb;

	mb->in_payload[0] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD0_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[0])
		goto after_mb;

	mb->in_payload[1] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD1_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[1])
		goto after_mb;

	mb->in_payload[2] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD2_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[2])
		goto after_mb;

	mb->in_payload[3] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD3_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[3])
		goto after_mb;

	mb->in_payload[4] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD4_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[4])
		goto after_mb;

	mb->in_payload[5] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD5_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if  (err || !mb->in_payload[5])
		goto after_mb;

	mb->in_payload[6] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD6_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[6])
		goto after_mb;

	mb->in_payload[7] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD7_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[7])
		goto after_mb;

	mb->in_payload[8] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD8_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[8])
		goto after_mb;

	mb->in_payload[9] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD9_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[9])
		goto after_mb;

	mb->in_payload[10] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD10_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[10])
		goto after_mb;

	mb->in_payload[11] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD11_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[11])
		goto after_mb;

	mb->in_payload[12] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD12_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[12])
		goto after_mb;

	mb->in_payload[13] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD13_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[13])
		goto after_mb;

	mb->in_payload[14] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD14_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[14])
		goto after_mb;

	mb->in_payload[15] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_IN_PAYLOAD15_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->in_payload[15])
		goto after_mb;

	mb->status = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_STATUS_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->status)
		goto after_mb;

	mb->out_payload[0] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD0_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[0])
		goto after_mb;

	mb->out_payload[1] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD1_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[1])
		goto after_mb;

	mb->out_payload[2] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD2_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[2])
		goto after_mb;

	mb->out_payload[3] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD3_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[3])
		goto after_mb;

	mb->out_payload[4] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD4_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[4])
		goto after_mb;

	mb->out_payload[5] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD5_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[5])
		goto after_mb;

	mb->out_payload[6] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD6_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[6])
		goto after_mb;

	mb->out_payload[7] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD7_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[7])
		goto after_mb;

	mb->out_payload[8] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD8_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[8])
		goto after_mb;

	mb->out_payload[9] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD9_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[9])
		goto after_mb;

	mb->out_payload[10] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD10_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[10])
		goto after_mb;

	mb->out_payload[11] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD11_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[11])
		goto after_mb;

	mb->out_payload[12] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD12_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[12])
		goto after_mb;

	mb->out_payload[13] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD13_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[13])
		goto after_mb;

	mb->out_payload[14] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD14_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[14])
		goto after_mb;

	mb->out_payload[15] = (__u64 __iomem *)cr_get_virt(rdpa_to_spa(
		CR_OS_MB_OFFSET + CR_MB_OUT_PAYLOAD15_OFFSET,
		c_dimm->ctrl_tbl,
		i_tbl,
		&err), c_dimm->ctrl_tbl);

	if (err || !mb->out_payload[15])
		goto after_mb;

	if (i_tbl) {
		mb->mb_in_line_size = i_tbl->line_size;
		mb->mb_out_line_size = i_tbl->line_size;
		mb->num_mb_in_segments = CR_IN_MB_SIZE / i_tbl->line_size;
		mb->num_mb_out_segments = CR_OUT_MB_SIZE / i_tbl->line_size;
	} else {
		mb->mb_in_line_size = CR_IN_MB_SIZE;
		mb->mb_out_line_size = CR_OUT_MB_SIZE;
		mb->num_mb_in_segments = 1;
		mb->num_mb_out_segments = 1;
	}
	mb->mb_in = kcalloc(mb->num_mb_in_segments,
		sizeof(*mb->mb_in), GFP_KERNEL);

	if (!mb->mb_in) {
		NVDIMM_DBG("Unable to allocate OS In Mailbox");
		err = -ENOMEM;
		goto after_mb;
	}

	for (i = 0; i < mb->num_mb_in_segments; i++) {
		mb->mb_in[i] = (void __iomem *)cr_get_virt(rdpa_to_spa(
			CR_OS_MB_IN_OFFSET + (i * mb->mb_in_line_size),
			c_dimm->ctrl_tbl, i_tbl, &err), c_dimm->ctrl_tbl);
		if (err || !mb->mb_in[i])
			goto after_os_in;
	}

	mb->mb_out = kcalloc(mb->num_mb_out_segments,
		sizeof(*mb->mb_out), GFP_KERNEL);

	if (!mb->mb_out) {
		NVDIMM_DBG("Unable to allocate OS Out Mailbox");
		err = -ENOMEM;
		goto after_os_in;
	}

	for (i = 0; i < mb->num_mb_out_segments; i++) {
		mb->mb_out[i] = (void __iomem *)cr_get_virt(rdpa_to_spa(
			CR_OS_MB_OUT_OFFSET + (i * mb->mb_out_line_size),
			c_dimm->ctrl_tbl, i_tbl, &err), c_dimm->ctrl_tbl);
		if (err || !mb->mb_out[i])
			goto after_os_out;
	}

	mutex_init(&mb->mb_lock);

	return mb;

after_os_out:
	kfree(mb->mb_out);
after_os_in:
	kfree(mb->mb_in);
after_mb:
	kfree(mb);
out:
	return ERR_PTR(err);
}

/**
 * cr_create_bw() - Create and configure CR block window
 * @bw_id: ID number of the block window to create
 * @c_dimm: cr_dimm to create the bw for
 * @i_tbl: The interleave table referenced by the mdsarmt_tbl
 *
 * Create the block window structure. This includes locating
 * each part of the block window in the system physical address space, and
 * mapping each part into the virtual address space.
 *
 * Returns: pointer to a completed block window on success
 *          on error returns error pointer
 */
struct block_window *cr_create_bw(__u16 bw_id,
		struct cr_dimm *c_dimm,
		struct interleave_tbl *i_tbl)
{

	struct block_window *bw;
	int err = 0, i, seg_linesize;
	__u64 ctladdr, aptaddr;
	struct memdev_spa_rng_tbl *mdsarmt_tbl = c_dimm->data_tbl;

	bw = kzalloc(sizeof(*bw), GFP_KERNEL);

	if (!bw) {
		NVDIMM_DBG("Unable to allocate block windows memory");
		return ERR_PTR(-ENOMEM);
	}

	ctladdr = (CR_OS_BW_REG_SKIP * bw_id);
	bw->bw_id = bw_id;
	bw->bw_ctrl = (void __iomem *)cr_get_virt(rdpa_to_spa(ctladdr +
			CR_OS_BW_CONTROL_REG_BASE + CR_OS_BW_CTRL_REG_OFFSET,
			mdsarmt_tbl, i_tbl, &err), c_dimm->ctrl_tbl);
	if (err || !bw->bw_ctrl)
		goto free_bw;

	bw->bw_status = (void __iomem *)cr_get_virt(rdpa_to_spa(ctladdr +
			CR_OS_BW_CONTROL_REG_BASE + CR_OS_BW_STATUS_REG_OFFSET,
			mdsarmt_tbl, i_tbl, &err), c_dimm->ctrl_tbl);
	if (err || !bw->bw_status)
		goto free_bw;

	aptaddr = (CR_OS_BW_APT_SKIP * bw_id);
	if (!i_tbl) {
		bw->num_apt_segments = 1;
		seg_linesize = 8192;
	} else {
		bw->num_apt_segments =
			CR_OS_BW_APT_SKIP / i_tbl->line_size;
		seg_linesize = i_tbl->line_size;
	}

	bw->bw_apt = kcalloc(bw->num_apt_segments, sizeof(*bw->bw_apt),
			GFP_KERNEL);

	if (!bw->bw_apt) {
		err = -ENOMEM;
		goto free_bw;
	}

	for (i = 0; i < bw->num_apt_segments; i++) {
		bw->bw_apt[i] = (void __iomem *)cr_get_virt(
				rdpa_to_spa(aptaddr +
					CR_OS_BW_BLOCK_APERTURE_BASE,
					mdsarmt_tbl, i_tbl, &err),
					c_dimm->ctrl_tbl);
		if (err || !bw->bw_apt[i])
			goto free_bw_apt;
		aptaddr += seg_linesize;
	}
	return bw;

free_bw_apt:
	kfree(bw->bw_apt);
free_bw:
	kfree(bw);
	return ERR_PTR(err);
}

/**
 * cr_create_bws() - Create and configure all block windows for a cr dimm
 * @c_dimm: The CR to create the block windows for
 * @i_tbl: The interleave table referenced by mdsarmt_tbl
 *
 * Create a BW for each and add to list of block windows kept in cr_create_bws
 *
 * Return: Error Code?
 */
int cr_create_bws(struct cr_dimm *c_dimm, struct interleave_tbl *i_tbl)
{
	int bw_id;
	struct block_window *bw;

	NVDIMM_DBG("CRBD: number of block windows %d\n",
			c_dimm->num_block_windows);

	for (bw_id = 0; bw_id < c_dimm->num_block_windows; bw_id++) {
		bw = cr_create_bw(bw_id, c_dimm, i_tbl);
		if (!(bw) || IS_ERR(bw))
			goto fail;
		list_add_tail(&bw->bw_node, &c_dimm->bw_list);
	}
	return 0;

fail:
	NVDIMM_WARN("CRBD: bw allocation failed\n");
	cr_free_block_windows(c_dimm);
	return PTR_ERR(bw);
}

static __u16 cr_parse_fw_build(__u8 mbs, __u8 lsb)
{
	return (CR_BCD_TO_TWO_DEC(mbs) * 100) + CR_BCD_TO_TWO_DEC(lsb);
}

static __u8 cr_parse_fw_hot_fix(__u8 fw_hot_fix)
{
	return CR_BCD_TO_TWO_DEC(fw_hot_fix);
}

static __u8 cr_parse_fw_minor(__u8 fw_minor)
{
	return CR_BCD_TO_TWO_DEC(fw_minor);
}

static __u8 cr_parse_fw_major(__u8 fw_major)
{
	return CR_BCD_TO_TWO_DEC(fw_major);
}

/**
 * cr_parse_fw_version() - Parse Firmware Version
 * @c_dimm - CR DIMM to parse fw_version for
 * @id_payload - FW ID DIMM payload returned from FW previously
 *
 * Parse the FW version returned by the FW into a CPU format
 * FW Payload has the FW version encoded in a binary coded decimal format
  */
static void cr_parse_fw_version(struct cr_dimm *c_dimm,
	struct cr_pt_payload_identify_dimm *id_payload)
{
	c_dimm->fw_major = cr_parse_fw_major(id_payload->fwr[4]);
	c_dimm->fw_minor = cr_parse_fw_minor(id_payload->fwr[3]);
	c_dimm->fw_hot_fix = cr_parse_fw_hot_fix(id_payload->fwr[2]);
	c_dimm->fw_build = cr_parse_fw_build(id_payload->fwr[1],
		id_payload->fwr[0]);
}

/**
 * parse the BCD formatted FW API version into major and minor
 */
static void cr_parse_fw_api_version(struct cr_dimm *c_dimm,
	struct cr_pt_payload_identify_dimm *id_payload)
{
	c_dimm->fw_api_major = (id_payload->api_ver >> 4) & 0xF;
	c_dimm->fw_api_minor = id_payload->api_ver & 0xF;
}

/**
 * cr_initalize_dimm() - Create CR DIMM
 * @dimm: The generic NVM DIMM that the CR DIMM is a part of
 * @fit_head: fully populated NVM Firmware Interface Table
 *
 * Perform all CR specific functions needed for DIMM initialization this
 * includes:
 * setting up mailbox structure
 * retrieving and recording security status
 * retrieving and recording the FW version
 * retrieving and recording partition information
 * setting up block windows
 *
 * Once created the CR DIMM should be placed into the private data of the
 * NVMDIMM
 *
 * Return
 * 0		- Success
 * -ENOMEM	- kalloc failure
 * -ENODEV	- No Control region found in FIT
 *
 */
int cr_initialize_dimm(struct nvdimm *dimm, struct fit_header *fit_head)
{
	struct cr_dimm *c_dimm;
	struct interleave_tbl *mb_i_tbl = NULL;
	struct cr_pt_payload_identify_dimm *id_payload;
	struct cr_pt_payload_get_security_state *security_payload;
	int ret = 0;

	c_dimm = kzalloc(sizeof(*c_dimm), GFP_KERNEL);

	if (!c_dimm)
		return -ENOMEM;

	INIT_LIST_HEAD(&c_dimm->bw_list);
	dimm->private = (void *)c_dimm;

	c_dimm->ctrl_tbl = get_memdev_spa_tbl(fit_head,
		dimm->physical_id, NVDIMM_CRTL_RNG_TYPE);

	if (!c_dimm->ctrl_tbl) {
		NVDIMM_INFO("No CTRL Region found unable to create Mailbox");
		ret = -ENODEV;
		goto after_cr_dimm;
	}

	if (c_dimm->ctrl_tbl->interleave_idx)
		mb_i_tbl =
		&fit_head->interleave_tbls[c_dimm->ctrl_tbl->interleave_idx
				- 1];
	c_dimm->host_mailbox = cr_create_mailbox(c_dimm, mb_i_tbl);

	if (IS_ERR_OR_NULL(c_dimm->host_mailbox)) {
		ret = PTR_ERR(c_dimm->host_mailbox);
		goto after_cr_dimm;
	}

	id_payload = kzalloc(sizeof(*id_payload), GFP_KERNEL);

	if (!id_payload) {
		ret = -ENOMEM;
		goto after_mailbox;
	}

	ret = fw_cmd_id_dimm(c_dimm, id_payload, dimm->physical_id);

	if (ret) {
		NVDIMM_INFO("FW CMD Error: %d", ret);
		goto after_id_dimm;
	}

	if (le16_to_cpu(id_payload->ifc) != dimm->ids.fmt_interface_code) {
		NVDIMM_INFO("FIT and FW Interface Code mismatch");
		ret = -EINVAL;
		goto after_id_dimm;
	}

	c_dimm->num_block_windows = le16_to_cpu(id_payload->nbw);
	c_dimm->num_write_flush_addrs = le16_to_cpu(id_payload->nwfa);
	c_dimm->raw_capacity = (le64_to_cpu(id_payload->rc) << 12);

	cr_parse_fw_version(c_dimm, id_payload);
	cr_parse_fw_api_version(c_dimm, id_payload);

	/* TODO: Call Get DIMM Partition FNV FW Command */

	security_payload = kzalloc(sizeof(*security_payload), GFP_KERNEL);

	if (!security_payload) {
		ret = -ENOMEM;
		goto after_id_dimm;
	}

	ret = fw_cmd_get_security_info(c_dimm, security_payload,
		dimm->physical_id);

	if (ret) {
		NVDIMM_INFO("FW CMD Error: %d", ret);
		ret = -EIO;
		goto after_sec_check;
	}

	c_dimm->security_flag = security_payload->security_status;

	c_dimm->data_tbl = get_memdev_spa_tbl(fit_head,
		dimm->physical_id, NVDIMM_DATA_RNG_TYPE);

/*	if (!c_dimm->data_tbl) {
		NVDIMM_WARN(
			"No DATA Region found unable to create Block Windows");
		ret = -ENODEV;
		goto after_sec_check;
	}*/

	/*TODO: cr_create_bws()*/
	kfree(id_payload);
	kfree(security_payload);
	return 0;

after_sec_check:
	kfree(security_payload);
after_id_dimm:
	kfree(id_payload);
after_mailbox:
	cr_free_mailbox(c_dimm->host_mailbox);
after_cr_dimm:
	kfree(c_dimm);
	return ret;
}

/**
 * cr_free_dimm() - CR free dimm
 * @c_dimm - the cr_dimm to free
 *
 * Free the memory resources associated with a
 * cr_dimm
 */
void cr_free_dimm(struct cr_dimm *c_dimm)
{
	cr_free_mailbox(c_dimm->host_mailbox);
	/* TODO: cr_free_block_windows(c_dimm); */

	kfree(c_dimm);
}

/**
 * cr_remove_dimm() - Remove a CR DIMM
 * @dimm: The generic NVM DIMM that the CR DIMM is a part of
  *
 * Perform all CR specific functions needed for when a DIMM is to be removed
 * from the platform.
 * This may include things like deallocating
 * mailboxes, deallocating block windows, and other memory
 *
 * CR Specific Function
 *
 * Return:
 * 0: Success
 * -ENOTTY: No CR DIMM structure to remove
 * -EBUSY: if unable to remove DIMM from inventory gracefully
 */
int cr_remove_dimm(struct nvdimm *dimm)
{
	struct cr_dimm *c_dimm = (struct cr_dimm *)dimm->private;

	if (!c_dimm)
		return -ENOTTY;

	cr_free_dimm(c_dimm);

	dimm->private = NULL;

	return 0;
}

/**
 * cr_write_volume_label() - Write a NVM Volume label to a CR DIMM
 * @dimm: The CR DIMM to write the volume label to
 * @l_info: The label info structure that contains information of where
 * to write labels to and the label itself
 *
 * Writes an NVM volume label to a CR DIMM. While Labels and the Volume label
 *  space are generic concepts the actual writing of the labels "MAY" be
 *  handled in a type specific way. Should only return when both labels are
 *  durable.
 *
 * Returns: Error Code?
 */
int cr_write_volume_label(struct nvdimm *dimm, struct label_info *l_info)
{
	return 0;
}

/**
 * cr_read_labels() - Read all labels contained on DIMM
 * @dimm: NVM DIMM to read labels from
 * @list: Label Info list to be populated with labels
 *
 * For a given CR DIMM read in all the volume labels.
 * Each volume label read in should be converted into the volume regions and
 * volume label that make up the volume label on disk. Each label should
 * be added to the list when ready.
 *
 *  Returns: Error Code
 */
int cr_read_labels(struct nvdimm *dimm, struct list_head *list)
{
	return 0;
}

/**
 * cr_dimm_read() - Read a number of bytes from a CR DIMM
 * @dimm: NVM DIMM to read from
 * @offset: offset from the start of the region this mem type uses
 * @nbytes: Number of bytes to read
 * @buffer: Buffer to place bytes into
 *
 * Read data in a CR specific way. Including selecting the block window to use.
 *
 * Returns:
 */
int cr_dimm_read(struct nvdimm *dimm, unsigned long offset,
	unsigned long nbytes, char *buffer)
{
	return 0;
}

/**
 * cr_dimm_write() - Write a number of bytes from a CR DIMM
 * @dimm: NVM DIMM to write to
 * @offset: offset from the start of the region this mem type uses
 * @nbytes: Number of bytes to write
 * @buffer: Buffer containing data to write
 *
 * Write data in a CR specific way. Including selecting block window to use.
 *
 * Returns: Error Code?
 */
int cr_dimm_write(struct nvdimm *dimm, unsigned long offset,
	unsigned long nbytes, char *buffer)
{
	return 0;
}
