/*
 * The interface between the platform independent device adapter and the device
 * driver, that defines the shared structures shared across that interface.
 *
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
 * Unless otherwise agreed by Intel in writing, you may not remove or alter this
 * notice or any other notice embedded in Materials by Intel or Intel's
 * suppliers or licensors in any way.
 */

#ifndef	_CR_IOCTL_H_
#define	_CR_IOCTL_H_

#include <linux/types.h>

#ifdef __cplusplus
extern "C"
{
#endif


/*
 * ****************************************************************************
 * DEFINES
 * ****************************************************************************
 */

#define	CR_SN_LEN	19 /* DIMM Serial Number buffer length */
#define	CR_PASSPHRASE_LEN	32 /* Length of a passphrase buffer */
#define	CR_SECURITY_NONCE_LEN	8 /* Length of a security nonce */
#define	CR_BCD_DATE_LEN	4 /* Length of a BDC Formatted Date */
#define	CR_BCD_TIME_LEN	3 /* Length of a BDC Formatted Time */
#define	CR_BCD_PCT_COMPLETE 3 /* Length of a BDC Formated Percent Complete */
#define	CR_FW_REV_LEN 5	/* Length of the formatted Firmware Revision string */
#define	CR_MFR_LEN	19 /* Length of manufacturer name buffer */
#define	CR_MODELNUM_LEN	19 /* Length of DIMM Model Number buffer */
#define CR_FW_LOG_ENTRY_COUNT 5 /* max number of log entries returned by FW */
#define	CR_OS_PARTITION	2 /* get platform config OS partition number */
#define	CR_CONFIG_GET_SIZE	1 /* get platform config get size command */
#define	CR_CONFIG_GET_DATA	2 /* get platform config get data command */

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

/*CR Security Status*/
enum {
	CR_SEC_RSVD		= 1 << 0,
	CR_SEC_ENABLED		= 1 << 1,
	CR_SEC_LOCKED		= 1 << 2,
	CR_SEC_FROZEN		= 1 << 3,
	CR_SEC_COUNT_EXP	= 1 << 4,
};

/*
 * ****************************************************************************
 * ENUMS
 * ****************************************************************************
 */

/*
 * TODO: for reference only, delete this comment block when no longer needed.
 * Notes on the implementation of the above IOCTL types.  The following IOCTL
 * enumerations must support the stated function prototypes and their arguments
 *
 * IOCTL_GET_TOPOLOGY_COUNT
 *		int ioctl_get_topology_count();
 *
 * IOCTL_GET_TOPOLOGY
 *		int ioctl_get_topology(
 *				const NVM_UINT8 count,
 *				struct nvdimm_topology *p_dimm_topo);
 *
 * IOCTL_GET_DIMM_DETAILS
 *		int ioctl_get_dimm_details(
 *				const struct nvdimm_topology,
 *				struct ioctl_dimm_details *p_dimm_details);
 *
 * IOCTL_GET_POOL_COUNT
 *		int ioctl_get_pool_count();
 *
 * IOCTL_GET_POOLS
 *		int ioctl_get_pools(
 *				const NVM_UINT32 count,
 *				struct ioctl_pool_discovery *p_pools);
 *
 * IOCTL_GET_POOL_DETAILS
 *		int ioctl_get_pool(
 *				const NVM_GUID uuid,
 *				struct nvdimm_pool_details *p_details);
 *
 * IOCTL_GET_VOLUME_COUNT
 *		int ioctl_get_volume_count();
 *
 * IOCTL_GET_VOLUMES
 *		int ioctl_get_volumes(
 *				const NVM_UINT32 count,
 *				struct nvdimm_volume_discovery *p_volumes);
 *
 * IOCTL_GET_VOLUME_DETAILS
 *		int ioctl_get_volume(
 *				const NVM_GUID uuid,
 *				struct nvdimm_volume_details *p_details);
 *
 * IOCTL_GET_VOLUME_POOLS
 *		int ioctl_get_volume_pools(
 *				const NVM_GUID vol_uuid,
 *				const NVM_UINT32 pool_count,
 *				NVM_GUID *p_pool_uuids);
 *
 * IOCTL_CREATE_VOLUME
 *		(pending)
 *
 * IOCTL_DELETE_VOLUME
 *		(pending)
 *
 * IOCTL_MODIFY_VOLUME
 *		(pending)
 *
 * IOCTL_PASSTHROUGH_CMD
 *		(pending)
 */

/*
 * Defines the Firmware Command Table opcodes accessed via the
 * CR_IOCTL_PASSTHROUGH_CMD
 */
enum cr_passthrough_opcode {
	/* NULL command, sub-op 00h always generates err */
	CR_PT_NULL_COMMAND = 0x00,
	/* Retrieve physical inventory data for an AEP DIMM */
	CR_PT_IDENTIFY_DIMM = 0x01,
	/* Retrieve security information from an AEP DIMM */
	CR_PT_GET_SEC_INFO = 0x02,
	/* Send a security related command to an AEP DIMM */
	CR_PT_SET_SEC_INFO = 0x03,
	/* Retrieve modifiable settings for AEP DIMM */
	CR_PT_GET_FEATURES = 0x04,
	CR_PT_SET_FEATURES = 0x05, /* Modify settings for AEP DIMM */
	CR_PT_GET_ADMIN_FEATURES = 0x06, /* TODO */
	CR_PT_SET_ADMIN_FEATURES = 0x07, /* TODO */
	/* Retrieve administrative data, error info, other FW data */
	CR_PT_GET_LOG = 0x08,
	CR_PT_UPDATE_FW = 0x09, /* Move an image to the AEP DIMM */
	/* Validation only CMD to trigger error conditions */
	CR_PT_INJECT_ERROR = 0x0B,
	CR_PT_RESERVED_1 = 0x0C,
	CR_PT_RESERVED_2 = 0x0D,
	CR_PT_GET_DBG_FEATURES = 0x0E, /* TODO */
	CR_PT_SET_DBG_FEATURES = 0x0F, /* TODO */
};

/*
 * Defines the Sub-Opcodes for CR_PT_GET_SEC_INFO
 */
enum cr_get_sec_info_subop {
	SUBOP_GET_SEC_STATE = 0x00 /* Returns the DIMM security state */
};

/*
 * Defines the Sub-Opcodes for CR_PT_SET_SEC_INFO
 */
enum cr_set_sec_info_subop {
	/* Set BIOS Security Nonce */
	SUBOP_SET_NONCE = 0x00,
	/* Changes the security administrator passphrase */
	SUBOP_SET_PASS = 0xF1,
	/* Disables the current password on a drive*/
	SUBOP_DISABLE_PASS = 0xF2,
	SUBOP_UNLOCK_UNIT = 0xF3, /* Unlocks the persistent region */
	SUBOP_SEC_ERASE_PREP = 0xF4, /* First cmd in erase sequence */
	SUBOP_SEC_ERASE_UNIT = 0xF5, /* Second cmd in erase sequence */
	/* Prevents changes to all security states until reset */
	SUBOP_SEC_FREEZE_LOCK = 0xF6
};

/*
 * Defines the Sub-Opcodes for CR_PT_GET_FEATURES & CR_PT_SET_FEATURES
 */
enum cr_get_set_feat_subop {
	SUBOP_RESET_THRESHOLDS = 0x00, /* TODO Reset all thresholds. */
	SUBOP_ALARM_THRESHOLDS = 0x01, /* TODO Get Alarm Thresholds. */
	SUBOP_POLICY_POW_MGMT = 0x02, /* TODO Power management & throttling. */
	SUBOP_POLICY_MEM_TEST = 0x03, /* TODO*/
	/* TODO Action policy to extend DIMM life. */
	SUBOP_POLICY_DIE_SPARRING = 0x04,
	SUBOP_POLICY_PATROL_SCRUB = 0x05  /* TODO */
};

/*
 * Defines the Sub-Opcodes for CR_PT_GET_ADMIN_FEATURES &
 * CR_PT_SET_ADMIN_FEATURES
 */
enum cr_get_set_admin_feat_subop {
	SUBOP_SYSTEM_TIME = 0x00, /* TODO Get/Set System Time */
	/*TODO Get/Set Platform Config Data (PCD) */
	SUBOP_PLATFORM_DATA_INFO = 0x01,
	/* TODO Set/Get DIMM Partition Config */
	SUBOP_DIMM_PARTITION_INFO = 0x02,
	/* TODO Get/Set log level of FNV FW */
	SUBOP_FNV_FW_DBG_LOG_LEVEL = 0x03
};

/*
 * Defines the Sub-Opcodes for CR_PT_GET_DBG_FEATURES & CR_PT_SET_DBG_FEATURES
 */
enum cr_get_set_dbg_feat_subop {
	SUBOP_CSR = 0x00, /* TODO */
	SUBOP_ERR_POLICY = 0x01,
	SUBOP_THERMAL_POLICY = 0x02,
	SUBOP_MEDIA_TRAIN_DATA = 0x03
};

/*
 * Defines the Sub-Opcodes for CR_PT_INJECT_ERROR
 */
enum cr_inject_error_subop {
	SUBOP_CLEAR_ALL_ERRORS = 0x00, /* TODO */
	SUBOP_ERROR_POISON = 0x01, /* TODO */
	SUBOP_ERROR_TEMP = 0x02, /* TODO */
};

/*
 * Defines the Sub-Opcodes for CR_PT_GET_LOG
 */
enum cr_get_log_subop {
	/* Retrieves various SMART & Health information. */
	SUBOP_SMART_HEALTH = 0x00,
	SUBOP_FW_IMAGE_INFO = 0x01, /* Retrieves firmware image information. */
	SUBOP_FW_DBG_LOG = 0x02, /* TODO Retrieves the binary firmware log. */
	SUBOP_MEM_INFO = 0x03, /* TODO*/
	/* TODO Status of any pending long operations. */
	SUBOP_LONG_OPERATION_STAT = 0x04,
	SUBOP_ERROR_LOG = 0x05, /* Retrieves firmware error log */
};

/*
 * Defines the Sub-Opcodes for CR_PT_UPDATE_FW
 */
enum cr_update_fw_subop {
	SUBOP_UPDATE_FNV_FW = 0x00, /* TODO */
	SUBOP_UPDATE_MEDIA_TRAINING_CODE = 0x01 /* TODO */
};

/*
 * Defines the various memory mode capabilities bits
 *
 */
enum cr_mem_mode_bits {
	CR_MEM_MODE_1LM = 1, /* 1LM mode supported */
	CR_MEM_MODE_2LM = 1 << 1, /* 2LM mode supported */
	CR_MEM_MODE_BLOCK = 1 << 2, /* block mode supported */
	CR_MEM_MODE_PM = 1 << 3, /* PM mode supported */
};

/*
 * Defines the interleave capabilities bits
 *
 */
enum cr_interleave_bits {
	CR_INTERLEAVE_64_CH = 1, /* 64 Bytes channel interleave */
	CR_INTERLEAVE_256_CH = 1 << 1, /* 256 Bytes channel interleave */
	CR_INTERLEAVE_4K_CH = 1 << 2, /* 4K Bytes channel interleave */
	CR_INTERLEAVE_1M_CH = 1 << 3, /* 1M Bytes channel interleave */
	CR_INTERLEAVE_64_IMC = 1 << 4, /* 64 Bytes iMC interleave */
	CR_INTERLEAVE_256_IMC = 1 << 5, /* 256 Bytes iMC interleave */
	CR_INTERLEAVE_4K_IMC = 1 << 6, /* 4K Bytes iMC interleave */
	CR_INTERLEAVE_1M_IMC = 1 << 7, /* 1M Bytes iMC interleave */
};

/*
 * ****************************************************************************
 * STRUCTURES
 * ****************************************************************************
 */

/*
 * Describes the capabilities of the host system's platform
 */
struct cr_platform_capabilites {
	/* header */
	unsigned char signature[4]; /* 'PCAP' is Signature for this table */
	unsigned int length; /* Length in bytes for entire table */
	unsigned char revision;
	unsigned char checksum; /* Must sum to zero */
	unsigned char reserved1[6]; /* reserved */

	/* body */
	/*
	 * Bit0 Clear: BIOS does not allow config change
	 *				request from CR mgmt software @n
	 * Bit0 Set: BIOS allows config change request from CR mgmt software
	 */
	unsigned short cr_mgmt_sw_config_request_support;

	/*
	 * Bit0: 1LM Supported @n
	 * Bit1: 2LM supported @n
	 * Bit2: Block supported @n
	 * Bit3: PM supported @n
	 */
	unsigned short mem_mode_capabilities;

	/*
	 * Interleave size supported by BIOS for 1LM @n
	 * Bit0: 64 Bytes channel interleave @n
	 * Bit1: 256 Bytes channel interleave @n
	 * Bit2: 4KB channel interleave @n
	 * Bit3: 1MB channel interleave @n
	 * Bit4: 64 Bytes iMC interleave @n
	 * Bit5: 256 Bytes iMC interleave @n
	 * Bit6: 4KB iMC interleave @n
	 * Bit7: 1MB iMC interleave
	 */
	unsigned short one_lm_interleave_capabilities;

	/*
	 * Interleave size supported by BIOS for 2LM @n
	 * Bit0: 64 Bytes channel interleave @n
	 * Bit1: 256 Bytes channel interleave @n
	 * Bit2: 4KB channel interleave @n
	 * Bit3: 1MB channel interleave @n
	 * Bit4: 64 Bytes iMC interleave @n
	 * Bit5: 256 Bytes iMC interleave @n
	 * Bit6: 4KB iMC interleave @n
	 * Bit7: 1MB iMC interleave
	 */
	unsigned short two_lm_interleave_capabilities;

	/*
	 * Interleave size supported by BIOS for Persistent Memory @n
	 * Bit0: 64 Bytes channel interleave @n
	 * Bit1: 256 Bytes channel interleave @n
	 * Bit2: 4KB channel interleave @n
	 * Bit3: 1MB channel interleave @n
	 * Bit4: 64 Bytes iMC interleave @n
	 * Bit5: 256 Bytes iMC interleave @n
	 * Bit6: 4KB iMC interleave @n
	 * Bit7: 1MB iMC interleave
	 */
	unsigned short pm_interleave_capabilities;

	/*
	 * Supported if set, not supported if clear
	 * Bit0: Spare control through  management sw @n
	 * Bit1: AEP DIMM spare @n
	 */
	unsigned char mem_spare_capability;

	/*
	 * Supported if set, not supported if clear
	 * Bit0: Memory control through mgmt sw @n
	 * Bit1: intra channel mirror @n
	 * Bit2: intra MC mirror @n
	 * Bit3: intra socket mirror (inter MC) @n
	 * Bit4: inter socket mirror @n
	 * Bit5: intra socket address based mirror @n
	 * Bit6: inter socket address based mirror @n
	 */
	unsigned char memory_mirror_capability;

	unsigned char reserved2[4]; /* reserved */

	/*
	 * AEP DIMM partition size needed to align to this value in bytes
	 */
	unsigned long long mem_alignment;
};

/*
 * ****************************************************************************
 * Passthrough CR Struct: fv_fw_cmd
 * ****************************************************************************
 */

/*
 * The struct defining the passthrough command and payloads to be operated
 * upon by the FV firmware.
 */
struct fv_fw_cmd {
	unsigned short id; /* The physical ID of the memory module */
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

/*
 * ****************************************************************************
 * Payloads for passthrough fw commands
 * ****************************************************************************
 */

/*
 * Passthrough CR Payload:
 *		Opcode: 0x0Ah (Identify DIMM)
 *	Small Output Payload
 */
struct cr_pt_payload_identify_dimm {
	__le16 vendor_id;
	__le16 device_id;
	__le16 revision_id;
	__le16 ifc; /* Interface format code */
	__u8 fwr[CR_FW_REV_LEN]; /* BCD formated firmware revision */
	__u8 api_ver; /* BCD formated api version */
	__u8 fswr; /* Feature SW Required Mask */
	__u8 reserved;
	__le16 nbw; /* Number of block windows */
	__le16 nwfa; /* Number write flush addresses */
	__le32 obmcr; /* Offset of block mode control region */
	__le32 rc; /* raw capacity (volatile+persistent in 4KB multiples*/
	char mf[CR_MFR_LEN]; /* ASCII Manufacturer */
	char sn[CR_SN_LEN]; /* ASCII Serial Number */
	char mn[CR_MODELNUM_LEN]; /* ASCII Model Number */
	__u8 resrvd[43]; /* Reserved */
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x01h (Get Security Info)
 *		Sub-Opcode:	0x00h (Get Security State)
 *	Small Output Payload
 */
struct cr_pt_payload_get_security_state {
	/*
	 * Bit0: reserved @n
	 * Bit1: Security Enabled (E) @n
	 * Bit2: Security Locked (L) @n
	 * Bit3: Security Frozen (F) @n
	 * Bit4: Security Count Expired (C) @n
	 * Bit5: reserved @n
	 * Bit6: reserved @n
	 * Bit7: reserved
	 */
	unsigned char security_status;

	unsigned char reserved[7]; /* TODO */
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x02h (Set Security Info)
 *		Sub-Opcode:	0xF1h (Set Passphrase)
 *	Small Input Payload
 */
struct cr_pt_payload_set_passphrase {
	/* The current security passphrase */
	char passphrase_current[CR_PASSPHRASE_LEN];
	/* The new passphrase to be set/changed to */
	char passphrase_new[CR_PASSPHRASE_LEN];
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x02h (Set Security Info)
 *		Sub-Opcode:	0xF2h (Disable Passphrase)
 *		Sub-Opcode:	0xF3h (Unlock Unit)
 *	Small Input Payload
 */
struct cr_pt_payload_passphrase {
	/* The end user passphrase */
	char passphrase_current[CR_PASSPHRASE_LEN];
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x02h (Set Security Info)
 *		Sub-Opcode:	0xF5h (Secure Erase Unit)
 *	Small Input Payload
 */
struct cr_pt_payload_secure_erase_unit {
	/* the end user passphrase */
	char passphrase_current[CR_PASSPHRASE_LEN];
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x03h (Set SMM Security Nonce)
 *	Small Input Payload
 */
struct cr_pt_payload_smm_security_nonce {
	/* Security nonce to lock down SMM mailbox */
	unsigned char sec_nonce[CR_SECURITY_NONCE_LEN];
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x04h (Get/Set Features)
 *		Sub-Opcode:	0x01h (Alarm Thresholds)
 *	Get - Small Output Payload
 *	Set - Small Input Payload
 */
struct cr_pt_payload_alarm_thresholds {
	/*
	 * Temperatures (in Kelvin) above this threshold trigger asynchronous
	 * events and may cause a transition in the overall health state
	 */
	__le16 temperature;

	/*
	 * When spare levels fall below this percentage based value,
	 * asynchronous events may be triggered and may cause a
	 * transition in the overall health state
	 */
	unsigned char spare;

	unsigned char reserved[5]; /* TODO*/
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x04h (Get Features)
 *		Sub-Opcode:	0x04h (Die Sparing Policy)
 *	Small Output Payload
 */
struct cr_pt_get_die_spare_policy {
	/*
	 * Reflects whether the die sparing policy is enabled or disabled
	 * 0x00 - Disabled
	 * 0x01 - Enabled
	 */
	unsigned char enable;
	/* How aggressive to be on die sparing (0...255), Default 128 */
	unsigned char aggressiveness;

	/*
	 * Designates whether or not the DIMM still supports die sparing
	 * 0x00 - No longer available - die sparing has most likely occurred
	 * 0x01 - Still available - the dimm has not had to take action yet
	 */
	unsigned char supported;

};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x05h (Set Features)
 *		Sub-Opcode:	0x04h (Die Sparing Policy)
 *	Small Input Payload
 */
struct cr_pt_set_die_spare_policy {
	/*
	 * Reflects whether the die sparing policy is enabled or disabled
	 * 0x00 - Disabled
	 * 0x01 - Enabled
	 */
	unsigned char enable;
	/* How aggressive to be on die sparing (0...255), Default 128 */
	unsigned char aggressiveness;
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Get Admin Features)
 *		Sub-Opcode:	0x00h (System Time)
 *
 *		Opcode:		0x07h (Set Admin Features)
 *		Sub-Opcode:	0x00h (System Time)
 *	Get - Small Output Payload
 *	Set - Small Input Payload
 */
struct cr_pt_payload_system_time {
	/*
	 * The date returned in BCD format. (slashes are not encoded) @n
	 * MM/DD/YYYY @n
	 * MM = 2 digit month (01-12) @n
	 * DD = 2 digit day (01-31) @n
	 * YYYY = 4 digit year (0000 - 9999)
	 */
	unsigned char date[CR_BCD_DATE_LEN];

	/*
	 * The time returned in BCD format. (colons are not encoded) @n
	 * HH:MM:SS:mmm @n
	 * HH = 2 digit hour (00-23) @n
	 * MM = 2 digit minute (00-59) @n
	 * SS = 3 digit second (00 - 59) @n
	 * mmm = 3 digit miliseconds (000 - 9990
	 */
	unsigned char time[CR_BCD_TIME_LEN];

	unsigned char rsvd[121];
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Get Admin Features)
 *		Sub-Opcode:	0x01h (Platform Config Data)
 *	Get - Small Input Payload - Data placed in Large Output Payload
 *	Set - Small Input Payload - Data read from Large Input Payload
 */
struct cr_pt_payload_platform_cfg_data {
	/*
	 * Which Partition to access
	 * 0x01 - The first partition
	 * 0x02 - The second partition
	 * All other values are reserved
	 */
	unsigned char partition_id;

	/*
	 * Dictates what information you want to read
	 * 0x01 - Retrieve Size
	 * 0x02 - Retrieve/Set Data
	 * All other values are reserved
	 */
	unsigned char command_parameter;

	/* Offset in bytes of the partition to start reading from */
	__le32 offset;

	/* Length in bytes to read from designated offset */
	__le32 length;
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Get Admin Features)
 *		Sub-Opcode:	0x01h (Platform Config Data)
 *	Small Output Payload
 */
struct cr_pt_payload_platform_get_size {
	__le32 size; /* In bytes of the selected partition */
	__le32 total_size; /* In bytes of the platform config area */
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Set Admin Features)
 *		Sub-Opcode:	0x01h (Platform Config Data)
 */
struct cr_pt_payload_platform_set_size {
	/*
	 * Which Partition to access
	 * 0x01 - The first partition
	 * 0x02 - The second partition
	 * All other values are reserved
	 */
	unsigned char partition_id;

	/*
	 * Dictates what information you want to read
	 * 0x01 - Retrieve Size
	 * 0x02 - Retrieve/Set Data
	 * All other values are resvered
	 */
	unsigned char command_parameter;

	/* Size to set for the partition */
	__le32 size;
};
/*
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Get Admin Features)
 *		Sub-Opcode:	0x02h (DIMM Partition Info)
 *	Small Output Payload
 */
struct cr_pt_payload_get_dimm_partion_info {
	/*
	 * AEP DIMM volatile memory capacity (in 4KB multiples of bytes).
	 * Special Values:
	 * 0x0000000000000000 - No volatile capacity
	 */
	__le32 volatile_capacity;
	unsigned char rsvd1[4];

	/* The DPA start address of the 2LM region */
	__le64 start_2lm;

	/*
	 * AEP DIMM PM capacity (in 4KB multiples of bytes).
	 * Special Values:
	 * 0x0000000000000000 - No persistent capacity
	 */
	__le32 pm_capacity;
	unsigned char rsvd2[4];

	/* The DPA start address of the PM region */
	__le64 start_pm;

	/* The raw usable size of the DIMM (Volatile + Persistent)
	 * (in 4KB multiples of bytes) */
	__le32 raw_capacity;
	unsigned char rsvd3[102];
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x07h (Set Admin Features)
 *		Sub-Opcode:	0x02h (DIMM Partition Info)
 *	Small Input Payload
 */
struct cr_pt_payload_set_dimm_partion_info {
	/*
	 * AEP DIMM volatile memory capacity (in 4KB multiples of bytes).
	 * Special Values:
	 * 0x0000000000000000 - No volatile capacity
	 */
	__le32 volatile_capacity;

	/*
	 * AEP DIMM PM capacity (in 4KB multiples of bytes).
	 *
	 * Special Values:
	 * 0x0000000000000000 - No persistent capacity
	 */
	__le32 pm_capacity;

	unsigned char rsvd[120];
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Get Admin Features)
 *		Sub-Opcode:	0x03h (FNV FW Debug Log Level)
 *	Small Payload Output
 */
struct cr_pt_payload_get_fnv_fw_dbg_log_level {
	/*
	 * The current logging level of the FNV FW (0-255).
	 *
	 * 0 = Disabled @n
	 * 1 = Error @n
	 * 2 = Warning @n
	 * 3 = Info @n
	 * 4 = Debug
	 */
	unsigned char log_level;
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x07h (Set Admin Features)
 *		Sub-Opcode:	0x03h (FNV FW Debug Log Level)
 *	Small Payload Input
 */
struct cr_pt_payload_set_fnv_fw_dbg_log_level {
	/*
	 * The current logging level of the FNV FW (0-255).
	 *
	 * 0 = Disabled @n
	 * 1 = Error @n
	 * 2 = Warning @n
	 * 3 = Info @n
	 * 4 = Debug
	 */
	unsigned char log_level;
};

/*
 * Passthrough CR Input Payload:
 *		Opcode:		0x0Eh (Get Debug Features)
 *		Sub-Opcode:	0x00h (Read CSR)
 *	Small Input Payload
 */
struct cr_pt_input_payload_read_csr {
	/* The address of the CSR register to read */
	__le32 csr_register_address;
};

/*
 * Passthrough CR Output Payload:
 *		Opcode:		0x0Eh (Get Debug Features)
 *		Sub-Opcode:	0x00h (Read CSR)
 *
 *		Opcode:		0x0Fh (Set Debug Features)
 *		Sub-Opcode:	0x00h (Write CSR)
 *	Small Output Payload
 */
struct cr_pt_output_payload_read_write_csr {
	__le32 csr_value; /* The value of the CSR register */
};

/*
 * Passthrough CR Payload
 *		Opcode:		0x0Eh (Get Debug Features)
 *		Sub-Opcode:	0x01h (Error Correction/Erasure Policy)
 *	Small Output Payload
 */
struct cr_pt_payload_err_correct_erasure_policy {
	/**
	 * Which policy to retrieve the policy values for.
	 * Bit 0 - Error Correction Policy
	 * Bit 1 - Erasure Coding Policy
	 */
	unsigned char policy_selection;

	/**
	 * The values for the selected policy
	 *
	 * Bit 0 - Verify ERC Scrub
	 * Bit 1 - Forced Write on Unrefreshed Lines
	 * Bit 2 - Forced Write on Refreshed Lines
	 * Bit 3 - Unrefreshed Enabled
	 * Bit 4 - Refreshed Enabled
	 */
	__le16 policy;
};

/**
 * Passthrough CR Payload:
 *		Opcode:		0x0Eh (Get Debug Features)
 *		Sub-Opcode:	0x02h (Thermal Policy)
 *	Small output payload
 *		Opcode:		0x0Fh (Set Debug Features)
 *		Sub-Opcode:	0x02h (Thermal Policy)
 *	Small input payload
 */
struct cr_pt_payload_thermal_policy {
	/**
	 * Reflects whether the termal policy is enabled or disabled
	 * 0x00 - Disabled
	 * 0x01 - Enabled
	 */
	unsigned char enabled;
};

/*
 * Passthrough CR Input Payload:
 *		Opcode:		0x0Fh (Set Debug Features)
 *		Sub-Opcode:	0x00h (Write CSR)
 */
struct cr_pt_input_payload_write_csr {
	__le32 csr_register_address; /* The address of the CSR register */
	__le32 csr_value; /* The value to be written to the CSR register */
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x0Bh (Inject Error)
 *		Sub-Opcode:	0x01h (Poison Error)
 *	Small Input Payload
 */
struct cr_pt_payload_poison_err {
	__le64 dpa_address; /* Address to set the poison bit for */
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x0bh (Inject Error)
 *		Sub-Opcode:	0x0sh (Temperature Error)
 */
struct cr_pt_payload_temp_err {
	/*
	 * A number representing the temperature (Kelvin) to inject
	 */
	__le16 temperature;
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x00h (SMART & Health Info)
 *	Small payload output
 */
struct cr_pt_payload_smart_health {
	/*
	 * Overall health summary
	 * 0x00h = Normal @n
	 * 0x01h = Non-Critical (maintenance required) @n
	 * 0x02h = Critical (features or performance degraded due to failure) @n
	 * 0x03h = Fatal (data loss has occurred or is imminent) @n
	 * 0x04h = Read Only(errors have forced media to be no longer writeable
	 * 0xFF - 0x05 Reserved
	 */
	unsigned char health_status;

	__le16 temperature; /* Current temperature in Kelvin*/
	/*
	 * Remaining spare capacity as a percentage of factory configured spare
	 */
	unsigned char spare;

	/*
	 * Device life span as a percentage.
	 * 100 = warranted life span of device has been reached however values
	 * up to 255 can be used.
	 */
	unsigned char percentage_used;

	unsigned char reserved_a[11];
	unsigned char power_cycles[16]; /* Number of AEP DIMM power cycles */
	/*
	 * Lifetime hours the AEP DIMM has been powered on
	 */
	unsigned char power_on_hours[16];
	unsigned char unsafe_shutdowns[16];
	unsigned char reserved_b[64];
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x01h (Firmware Image Info)
 *	Small output payload
 */
struct cr_pt_payload_fw_image_info {
	/*
	 * Contains revision of the firmware in ASCII in the following format:
	 * aa.bb.cc.dddd
	 *
	 * aa = 2 digit Major Version
	 * bb = 2 digit Minor Version
	 * cc = 2 digit Hotfix Version
	 * dddd = 4 digit Build Version
	 */
	unsigned char fw_rev[CR_FW_REV_LEN];
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x03h (Memory Info)
 *	Small Output Payload
 */
struct cr_pt_payload_memory_info {
	/*
	 * Number of bytes the host has read from the AEP DIMM.
	 * This value is cumulative across the life span of the device.
	 */
	unsigned char bytes_read[16];

	/*
	 * Number of bytes the host has written to the AEP DIMM.
	 * This value is cumulative across the life span of the device.
	 */
	unsigned char bytes_written[16];

	/*
	 * Lifetime # of read requests serviced by AEP DIMM
	 */
	unsigned char host_read_reqs[16];
	/*
	 * Lifetime # of write requests serviced by AEP DIMM
	 */
	unsigned char host_write_cmds[16];

	unsigned char media_errors[16]; /* ECC uncorrectable errors. */

	unsigned char reserved[48]; /* TODO */
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x04h (Long Operations Status)
 *	Small Output Payload
 */
struct cr_pt_payload_long_op_stat {
	/*
	 * This will coincide with the opcode & sub-opcode
	 * Bytes 7:0 - Opcode
	 * Bytes 15:8 - Sub-Opcode
	 */
	unsigned char command[16];

	/*
	 * The % complete of the current command
	 * BCD Format
	 */
	unsigned char percent_complete[CR_BCD_PCT_COMPLETE];

	/*
	 * Estimated Time to Completion.
	 * The time in BCD format of how much longer the operation will take.
	 * (colons are not encoded) @n
	 * HH:MM:SS:mmm @n
	 * HH = 2 digit hour (00-23) @n
	 * MM = 2 digit minute (00-59) @n
	 * SS = 3 digit second (00 - 59) @n
	 * mmm = 3 digit miliseconds (000 - 9990
	 */
	unsigned char etc[CR_BCD_TIME_LEN];

	unsigned char reserved[106]; /* TODO */
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x02h (Firmware Debug Log)
 *	Small Input Payload
 *	Log returned in Large Output Payload
 */
struct cr_pt_payload_fw_debug_log {
	unsigned char log_id; /* The ID of the log to retrieve*/
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x05h (Get Error Log)
 *	Small Input Payload
 */
struct cr_pt_input_payload_fw_error_log {
	/*
	 * Configure what is returned
	 *
	 * Bit 0 - Access Type: Specifies if request is for new entries vs the
	 *							entire log
	 *			0x0 = Return only new entries in Log FIFI
	 *			0x1 = Return all entries in log FIFO
	 * Bit 1 - Log Level: Specifies which error log to retrieve
	 *						(High vs Low Priority)
	 *						0x00 = Low
	 *						0x01 = High
	 * Bit 2 - Clear on read: Flag to indicate that entries read will
	 *			no longer be reported as new. This causes the
	 *			head of the log FIFO to progress by number of
	 *			entries retrieved. Should only be used
	 *			with Offset = 0
	 * Bit 7:3 - Unrefreshed Enable
	 *		(1 = enables XOR scrub on unrefreshed lines)
	 */
	unsigned char params;
	/*
	 * Offset in into log to retrieve.
	 * Reads log from specified entry offset.
	 * Should use 0 when Clear on Read is enabled.
	 */
	unsigned char offset;
	/*
	 *  Request Count: Max number of log entries requested for this access.
	 *  ‘0’ can be used to request the count fields without reading the
	 *  log entries.
	 */
	unsigned char request_count;
	unsigned char reserved[125];
};


/*
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x05h (Get Error Log)
 * Represents the Log Entries returned in the Small Output Payload
 */
struct cr_pt_fw_log_entry {
	/*
	 * Last System Time set by BIOS using the Set Admin Feature-System Time
	 * (BCD format).
	 */
	unsigned char system_timestamp[7];
	/*
	 * Power on time in seconds since last System Time update
	 */
	unsigned char timestamp_offset[4];
	/*
	 * Specifies the DPA address of the error
	 */
	unsigned char dpa[5];
	/*
	 * Specifies the PDA address of the error
	 */
	unsigned char pda[5];
	/*
	 * Specifies the length in address space of this error. Ranges will be
	 * encoded as a power of 2. Range is in 2^X bytes. Typically X=8 for
	 * one SXP 256B error
	 */
	unsigned char range;
	/*
	 * Indicates what kind of error was logged. Entry includes
	 * error type and Flags.
	 * Bit 2:0 - Error Type Encoded Value
	 *				0h = Uncorrectable
	 *				1h = DPA Mismatch
	 *				2h = AIT Error
	 * Bit 7:3 - Error Flags (Bitfields)
	 *	(7) – VIRAL: Indicates Viral was signaled for this error
	 *	(6) – INJECT: Bit to indicate this was an injected
	 *			 error entry
	 *	(5) – INTERRUPT: Bit to indicate this error generated
	 *			an interrupt packet
	 *	(4) – DPA VALID: Indicates the DPA address is valid.
	 *	(3) – PDA VALID: Indicates the PDA address is valid.
	 */
	unsigned char error_id;
	/*
	 * Indicates what transaction caused the error
	 *		0h=2LM READ
	 *		1h=2LM WRITE (Uncorrectable on a partial write)
	 *		2h=PM READ
	 *		3h=PM WRITE
	 *		4h=BW READ
	 */
	unsigned char transaction_type;
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x05h (Get Error Log)
 *	Small Output Payload
 */
struct cr_pt_output_payload_fw_error_log {
	/*
	 * Number Total Entries: Specifies the total number of valid entries
	 * in the Log (New or Old)
	 */
	unsigned char number_total_entries;
	/*
	 * Number New Entries: Specifies the total number of
	 * New entries in the Log
	 */
	unsigned char number_new_entries;
	/*
	 * Overrun Flag: Flag to indicate that the Log FIFO had an overrun
	 * condition. Occurs if new entries exceed the LOG size
	 */
	unsigned char overrun_flag;
	/*
	 * Return Count: Number of Log entries returned
	 */
	unsigned char return_count;

	/*
	 * See cr_pt_fw_log_entry for log entries
	 */
	struct cr_pt_fw_log_entry log_entries[CR_FW_LOG_ENTRY_COUNT];

	unsigned char reservers[4];
};

/*
 * Passthrough CR Payload:
 *		Opcode:		0x09h (Update Firmware)
 *		Sub-Opcode:	0x00h (Update FNV FW)
 *	For DDRT Large Input Payload holds new firmware image
 */

#ifdef __cplusplus
}
#endif

#endif	/* _CR_IOCTL_H_ */
