/*
 * @internal
 * @copyright
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
 * @endinternal
 */

#ifndef CRBD_CRDIMM_H_
#define CRBD_CRDIMM_H_

#include <linux/nvdimm_acpi.h>
#include <linux/nvdimm_core.h>
#include <linux/cr_ioctl.h>

#define CR_OS_MB_OFFSET 0 /*Offset from the start of the CTRL
				region to the start of the OS mailbox*/
#define CR_OS_MB_IN_OFFSET (2 << 20)
/*Offset from the start of the CTRL region
			to the start of the OS mailbox large input payload*/
#define CR_OS_MB_OUT_OFFSET (3 << 20)
/*Offset from the start of the CTRL region
			to the start of the OS mailbox large output payload*/
#define CR_IN_MB_SIZE (1 << 20) /*Size of the OS mailbox large input payload*/
#define CR_OUT_MB_SIZE (1 << 20)/*Size of the OS mailbox large output payload*/
#define CR_REG_SIZE (8) /*Size of a CR Mailbox Register Bytes*/
#define CR_IN_PAYLOAD_SIZE (128) /*Total size of the input payload registers*/
#define CR_OUT_PAYLOAD_SIZE (128) /*Total size of the output payload registers*/

#define STATUS_MASK 0x00FF0000

#define DB_SHIFT 16
#define SUB_OP_SHIFT 8

/* Offset from the start of the OS mailbox*/
enum {
	CR_MB_COMMAND_OFFSET = 0,
	CR_MB_NONCE0_OFFSET = 0x40,
	CR_MB_NONCE1_OFFSET = 0x80,
	CR_MB_IN_PAYLOAD0_OFFSET = 0xC0,
	CR_MB_IN_PAYLOAD1_OFFSET = 0x100,
	CR_MB_IN_PAYLOAD2_OFFSET = 0x140,
	CR_MB_IN_PAYLOAD3_OFFSET = 0x180,
	CR_MB_IN_PAYLOAD4_OFFSET = 0x1C0,
	CR_MB_IN_PAYLOAD5_OFFSET = 0x200,
	CR_MB_IN_PAYLOAD6_OFFSET = 0x240,
	CR_MB_IN_PAYLOAD7_OFFSET = 0x280,
	CR_MB_IN_PAYLOAD8_OFFSET = 0x2C0,
	CR_MB_IN_PAYLOAD9_OFFSET = 0x300,
	CR_MB_IN_PAYLOAD10_OFFSET = 0x340,
	CR_MB_IN_PAYLOAD11_OFFSET = 0x380,
	CR_MB_IN_PAYLOAD12_OFFSET = 0x3C0,
	CR_MB_IN_PAYLOAD13_OFFSET = 0x400,
	CR_MB_IN_PAYLOAD14_OFFSET = 0x440,
	CR_MB_IN_PAYLOAD15_OFFSET = 0x480,
	CR_MB_STATUS_OFFSET = 0x4C0,
	CR_MB_OUT_PAYLOAD0_OFFSET = 0x500,
	CR_MB_OUT_PAYLOAD1_OFFSET = 0x540,
	CR_MB_OUT_PAYLOAD2_OFFSET = 0x580,
	CR_MB_OUT_PAYLOAD3_OFFSET = 0x5C0,
	CR_MB_OUT_PAYLOAD4_OFFSET = 0x600,
	CR_MB_OUT_PAYLOAD5_OFFSET = 0x640,
	CR_MB_OUT_PAYLOAD6_OFFSET = 0x680,
	CR_MB_OUT_PAYLOAD7_OFFSET = 0x6C0,
	CR_MB_OUT_PAYLOAD8_OFFSET = 0x700,
	CR_MB_OUT_PAYLOAD9_OFFSET = 0x740,
	CR_MB_OUT_PAYLOAD10_OFFSET = 0x780,
	CR_MB_OUT_PAYLOAD11_OFFSET = 0x7C0,
	CR_MB_OUT_PAYLOAD12_OFFSET = 0x800,
	CR_MB_OUT_PAYLOAD13_OFFSET = 0x840,
	CR_MB_OUT_PAYLOAD14_OFFSET = 0x880,
	CR_MB_OUT_PAYLOAD15_OFFSET = 0x8C0,
};

/*Mailbox Status Codes*/
enum {
	MB_SUCCESS = 0x00, /*Command Complete*/
	MB_INVALID_PARAM = 0x01, /*Input parameter invalid*/
	MB_DATA_TRANS_ERR = 0x02, /*Error in the data transfer*/
	MB_INTERNAL_ERR = 0x03, /*Internal device error*/
	MB_UNSUPPORTED_CMD = 0x04, /*Opcode or Sub opcode not supported*/
	MB_BUSY = 0x05, /*Device is busy processing a long operation*/
	MB_PASSPHRASE_ERR = 0x06, /*Incorrect Passphrase*/
	MB_SECURITY_ERR = 0x07, /*Security check on the image has failed*/
	MB_INVALID_STATE = 0x08, /*Op not permitted in current security state*/
	MB_SYS_TIME_ERR = 0x09, /*System time has not been set yet*/
	MB_DATA_NOT_SET = 0x0A /*Get data called without ever calling set data*/
};

#define MB_COMPLETE 0x1

#define CR_OS_BW_CONTROL_REG_BASE	(0x00000000)
#define CR_OS_BW_BLOCK_APERTURE_BASE	(0x07800000)
#define CR_OS_BW_CTRL_REG_OFFSET	(0x00000000)
#define CR_OS_BW_STATUS_REG_OFFSET	(0x00000008)
#define CR_OS_BW_REG_SKIP	(0x1000)
#define CR_OS_BW_APT_SKIP	(0x2000)

struct block_window {
	struct list_head bw_node;
	__u16 bw_id;
	__u64 __iomem *bw_ctrl; /*va of the control register*/
	__u64 __iomem *bw_status; /*va of the status register*/
	__u32 num_apt_segments; /*number of aperture segments*/
	void __iomem **bw_apt; /*v. addresses of the aperture segments*/
};

struct cr_mailbox {
	struct mutex mb_lock;
	__u64 __iomem *command; /*va of the command register*/
	__u64 __iomem *nonce0; /*va of the nonce0 register*/
	__u64 __iomem *nonce1; /*va of the nonce1 register*/
	__u64 __iomem *in_payload[16]; /*va of the 16 in payload
						registers write only*/
	__u64 __iomem *status; /*va of the status register*/
	__u64 __iomem *out_payload[16]; /*va of the 8 out payload
						registers read only*/
	__u32 mb_in_line_size;
	__u32 mb_out_line_size;
	__u32 num_mb_in_segments; /*number of segments of the IN mailbox */
	__u32 num_mb_out_segments; /*number of segments of the OUT mailbox */
	void __iomem **mb_in; /*va of the IN mailbox segments*/
	void __iomem **mb_out; /*va of the OUT mailbox segments*/
};

struct cr_dimm {
	__u8 fw_major; /*FW Version Major*/
	__u8 fw_minor; /*FW Version Minor*/
	__u8 fw_hot_fix; /*FW Version HotFix*/
	__u16 fw_build; /*FW Version Build Number*/
	__u8 fw_api_major; /*FW API Version Major*/
	__u8 fw_api_minor; /*FW API Version Minor*/
	__u8 security_flag; /*Identifies the security status of the DIMM
	 collected from FW*/
	__u64 pm_start; /*The DPA start address of the PM region*/
	__u64 pm_capacity; /*DIMM Capacity (Bytes) to reserve for PM*/
	__u16 num_block_windows; /*Number of Block Windows Supported*/
	__u16 num_write_flush_addrs;
	__u64 raw_capacity; /*PM + volatile*/
	struct list_head bw_list;
	struct cr_mailbox *host_mailbox;
	struct memdev_spa_rng_tbl *ctrl_tbl; /*ptr to the table used to
	 configure the mailbox*/
	struct memdev_spa_rng_tbl *data_tbl; /*ptr to the table used to
	 configure the block windows*/
	/*XXX: Flush Hint Tables need to be accounted for somehow*/
};

#define CR_BCD_TO_TWO_DEC(BUFF) (((BUFF>>4) * 10) + (BUFF & 0xF))

struct id_dimm_payload {
	__le16 vendor_id;
	__le16 device_id;
	__le16 revision_id;
	__le16 ifc; /*Interface format code*/
	__u8 fwr[5]; /* BCD formated firmware revision*/
	__u8 api_ver; /* BCD formated api version*/
	__u8 fswr; /*Feature SW Required Mask*/
	__u8 reserved;
	__le16 nbw; /*Number of block windows*/
	__le16 nwfa; /*Number write flush addresses*/
	__le32 obmcr; /*Offset of block mode control region*/
	__le64 rc; /*raw capacity*/
	char mf[19]; /*ASCII Manufacturer*/
	char sn[19]; /*ASCII Serial Number*/
	char mn[19]; /*ASCII Model Number*/
	__u8 resrvd[39]; /*Reserved*/
} __packed;

/*CR Security Status*/
enum {
	CR_SEC_RSVD		= 1 << 0,
	CR_SEC_ENABLED		= 1 << 1,
	CR_SEC_LOCKED		= 1 << 2,
	CR_SEC_FROZEN		= 1 << 3,
	CR_SEC_COUNT_EXP	= 1 << 4,
};

struct get_security_payload {
	__u8 security_status;
	__u8 resrvd[39]; /*Reserved*/
} __packed;

/******************************************************************************
 * CR DIMM FUNCTIONS
 *****************************************************************************/

int cr_initialize_dimm(struct nvdimm *dimm, struct fit_header *fit_head);

int cr_remove_dimm(struct nvdimm *dimm);

/**
 * cr_get_dimm_partition_info() - Retrieve DIMM Partition Info from FNV
 *  @dimm: The CR DIMM to gather information from
 *
 *  Send a FNV command to retrieve the partition info of the DIMM
 *  Update the CR DIMM with pm start, pm capacity, and pm locality
 *
 *  CR Specific Function
 *
 *  Returns: Error Code?
 */

int cr_get_dimm_partition_info(struct cr_dimm *dimm);

int cr_write_volume_label(struct nvdimm *dimm, struct label_info *l_info);

int cr_read_labels(struct nvdimm *dimm, struct list_head *list);

int cr_dimm_read(struct nvdimm *dimm, unsigned long offset,
	unsigned long nbytes, char *buffer);

int cr_dimm_write(struct nvdimm *dimm, unsigned long offset,
	unsigned long nbytes, char *buffer);

struct block_window *cr_create_bw(__u16 bw_id,
	struct cr_dimm *c_dimm, struct interleave_tbl *i_tbl);

int cr_create_bws(struct cr_dimm *dimm, struct interleave_tbl *i_tbl);

void cr_free_block_windows(struct cr_dimm *c_dimm);

void cr_free_block_window(struct block_window *bw);

struct cr_mailbox *cr_create_mailbox(struct cr_dimm *c_dimm,
	struct interleave_tbl *i_tbl);

void cr_free_mailbox(struct cr_mailbox *mb);

int cr_send_command(struct fv_fw_cmd *fw_cmd, struct cr_mailbox *mb);

void cr_free_dimm(struct cr_dimm *c_dimm);

void cr_write_cmd_op(struct cr_mailbox *mb, struct fv_fw_cmd *fw_cmd);
void cr_memcopy_inpayload(struct cr_mailbox *mb, struct fv_fw_cmd *fw_cmd);
void cr_memcopy_outpayload(struct cr_mailbox *mb, struct fv_fw_cmd *fw_cmd);
int cr_verify_fw_cmd(struct fv_fw_cmd *fw_cmd);
void cr_memcopy_large_inpayload(struct cr_mailbox *mb,
	struct fv_fw_cmd *fw_cmd);
void cr_memcopy_large_outpayload(struct cr_mailbox *mb,
	struct fv_fw_cmd *fw_cmd);
int fw_cmd_pass_thru(struct cr_dimm *c_dimm, struct fv_fw_cmd *cmd);

int fw_cmd_id_dimm(struct cr_dimm *c_dimm,
	struct id_dimm_payload *payload, __u16 dimm_id);

#endif
