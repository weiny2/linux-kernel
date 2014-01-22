/*!
 * @file
 * @brief
 * The interface between the platform independent device adapter and the device
 *  driver, that defines the shared structures shared across that interface.
 *
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
 * Unless otherwise agreed by Intel in writing, you may not remove or alter this
 * notice or any other notice embedded in Materials by Intel or Intel's
 * suppliers or licensors in any way.
 * @endinternal
 */

#ifndef	_CR_IOCTL_H_
#define	_CR_IOCTL_H_

#ifdef __cplusplus
extern "C"
{
#endif


/*
 * ****************************************************************************
 * DEFINES
 * ****************************************************************************
 */

#define	CR_SN_LEN	19 /*!< DIMM Serial Number buffer length*/
#define	CR_PASSPHRASE_LEN	32 /*!< Length of a passphrase buffer*/
#define	CR_SECURITY_NONCE_LEN	8 /*!< Length of a security nonce*/
#define	CR_BCD_DATE_LEN	4 /*!< Length of a BDC Formatted Date*/
#define	CR_BCD_TIME_LEN	3 /*!< Length of a BDC Formatted Time*/
#define CR_BCD_PCT_COMPLETE 3/*!<Length of a BDC Formated Percent Complete*/
#define	CR_FW_REV_LEN 5 /*!< Length of the formatted Firmware Revision string*/
#define	CR_MFR_LEN	19 /*!< Length of manufacturer name buffer*/
#define	CR_MODELNUM_LEN	19 /*!< Length of DIMM Model Number buffer*/

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
 *				struct ioctl_dimm_topology *p_dimm_topo);
 *
 * IOCTL_GET_DIMM_DETAILS
 *		int ioctl_get_dimm_details(
 *				const struct ioctl_dimm_topology,
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
 *				struct ioctl_pool_details *p_details);
 *
 * IOCTL_GET_VOLUME_COUNT
 *		int ioctl_get_volume_count();
 *
 * IOCTL_GET_VOLUMES
 *		int ioctl_get_volumes(
 *				const NVM_UINT32 count,
 *				struct ioctl_volume_discovery *p_volumes);
 *
 * IOCTL_GET_VOLUME_DETAILS
 *		int ioctl_get_volume(
 *				const NVM_GUID uuid,
 *				struct ioctl_volume_details *p_details);
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

/*!
 * Defines the Firmware Command Table opcodes accessed via the
 * CR_IOCTL_PASSTHROUGH_CMD
 */
enum cr_passthrough_opcode {
	/*! NULL command, sub-op 00h always generates err*/
	CR_PT_NULL_COMMAND = 0x00,
	/*! Retrieve security information from an AEP DIMM*/
	CR_PT_GET_SEC_INFO = 0x01,
	/*! Send a security related command to an AEP DIMM*/
	CR_PT_SET_SEC_INFO = 0x02,
	/*! Allows BIOS to lock down the SMM mailbox*/
	CR_PT_SET_SMM_NONC = 0x03,
	/*! Retrieve modifiable settings for AEP DIMM*/
	CR_PT_GET_FEATURES = 0x04,
	CR_PT_SET_FEATURES = 0x05, /*!< Modify settings for AEP DIMM*/
	CR_PT_GET_ADMIN_FEATURES = 0x06, /*! TODO */
	CR_PT_SET_ADMIN_FEATURES = 0x07, /*! TODO */
	/*!< Retrieve administrative data, error info, other FW data*/
	CR_PT_GET_LOG = 0x08,
	CR_PT_UPDATE_FW = 0x09, /*!< Move an image to the AEP DIMM*/
	/*! Retrieve physical inventory data for an AEP DIMM*/
	CR_PT_IDENTIFY_DIMM = 0x0A,
	/*! Validation only CMD to trigger error conditions*/
	CR_PT_INJECT_ERROR = 0x0B,
	CR_PT_RESERVED_1 = 0x0C,
	CR_PT_RESERVED_2 = 0x0D,
	CR_PT_GET_DBG_FEATURES = 0x0E, /*!< TODO*/
	CR_PT_SET_DBG_FEATURES = 0x0F, /*!< TODO*/
};

/*!
 * Defines the Sub-Opcodes for CR_PT_GET_SEC_INFO
 */
enum cr_get_sec_info_subop {
	SUBOP_GET_SEC_STATE = 0x00 /*!< Returns the DIMM security state*/
};

/*!
 * Defines the Sub-Opcodes for CR_PT_SET_SEC_INFO
 */
enum cr_set_sec_info_subop {
	/*! Changes the security administrator passphrase*/
	SUBOP_SET_PASS = 0xF1,
	/*! Disables the current password on a drive*/
	SUBOP_DISABLE_PASS = 0xF2,
	SUBOP_UNLOCK_UNIT = 0xF3, /*!< Unlocks the persistent region*/
	SUBOP_SEC_ERASE_PREP = 0xF4, /*!< First cmd in erase sequence*/
	SUBOP_SEC_ERASE_UNIT = 0xF5, /*!< Second cmd in erase sequence*/
	/*! Prevents changes to all security states until reset*/
	SUBOP_SEC_FREEZE_LOCK = 0xF6
};

/*!
 * Defines the Sub-Opcodes for CR_PT_GET_FEATURES & CR_PT_SET_FEATURES
 */
enum cr_get_set_feat_subop {
	SUBOP_RESET_THRESHOLDS = 0x00, /*!< TODO Reset all thresholds.*/
	SUBOP_ALARM_THRESHOLDS = 0x01, /*!< TODO Get Alarm Thresholds.*/
	SUBOP_POLICY_POW_MGMT = 0x02, /*!< TODO Power management & throttling.*/
	SUBOP_POLICY_MEM_TEST = 0x03, /*!< TODO*/
	/*! TODO Action policy to extend DIMM life.*/
	SUBOP_POLICY_DIE_SPARRING = 0x04,
	SUBOP_POLICY_PATROL_SCRUB = 0x05  /*!< TODO*/
};

/*!
 * Defines the Sub-Opcodes for CR_PT_GET_ADMIN_FEATURES &
 * CR_PT_SET_ADMIN_FEATURES
 */
enum cr_get_set_admin_feat_subop {
	SUBOP_SYSTEM_TIME = 0x00, /*!< TODO Get/Set System Time*/
	/*!TODO Get/Set Platform Config Data (PCD)*/
	SUBOP_PLATFORM_DATA_INFO = 0x01,
	/*! TODO Set/Get DIMM Partition Config*/
	SUBOP_DIMM_PARTITION_INFO = 0x02,
	/*!< TODO Get/Set log level of FNV FW*/
	SUBOP_FNV_FW_DBG_LOG_LEVEL = 0x03
};

/*!
 * Defines the Sub-Opcodes for CR_PT_GET_DBG_FEATURES & CR_PT_SET_DBG_FEATURES
 */
enum cr_get_set_dbg_feat_subop {
	SUBOP_CSR = 0x00, /*!< TODO*/
	SUBOP_ERR_POLICY = 0x01,
	SUBOP_THERMAL_POLICY = 0x02,
	SUBOP_MEDIA_TRAIN_DATA = 0x03
};

/*!
 * Defines the Sub-Opcodes for CR_PT_INJECT_ERROR
 */
enum cr_inject_error_subop {
	SUBOP_CLEAR_ALL_ERRORS = 0x00, /*!< TODO*/
	SUBOP_ERROR_POISON = 0x01, /*!< TODO*/
	SUBOP_ERROR_TEMP = 0x02, /*!< TODO*/
};

/*!
 * Defines the Sub-Opcodes for CR_PT_GET_LOG
 */
enum cr_get_log_subop {
	/*! Retrieves various SMART & Health information.*/
	SUBOP_SMART_HEALTH = 0x00,
	SUBOP_FW_IMAGE_INFO = 0x01, /*!< Retrieves firmware image information.*/
	SUBOP_FW_DBG_LOG = 0x02, /*!< TODO Retrieves the binary firmware log.*/
	SUBOP_MEM_INFO = 0x03, /*!< TODO*/
	/*!TODO Status of any pending long operations.*/
	SUBOP_LONG_OPERATION_STAT = 0x04,
	SUBOP_ERROR_LOG = 0x05, /*!< TODO*/
};

/*!
 * Defines the Sub-Opcodes for CR_PT_UPDATE_FW
 */
enum cr_update_fw_subop {
	SUBOP_UPDATE_FNV_FW = 0x00, /*!< TODO*/
	SUBOP_UPDATE_MEDIA_TRAINING_CODE = 0x01 /*!< TODO*/
};

/*
 * ****************************************************************************
 * STRUCTURES
 * ****************************************************************************
 */

/*
 * ****************************************************************************
 * Passthrough CR Struct: fv_fw_cmd
 * ****************************************************************************
 */

/*!
 * The struct defining the passthrough command and payloads to be operated
 * upon by the FV firmware.
 */
struct fv_fw_cmd {
	unsigned short id; /*!< The physical ID of the memory module*/
	unsigned char opcode; /*!< The command opcode.*/
	unsigned char sub_opcode; /*!< The command sub-opcode.*/
	unsigned int input_payload_size; /*!< The size of the input payload*/
	void *input_payload; /*!< A pointer to the input payload buffer*/
	unsigned int output_payload_size; /*!< The size of the output payload*/
	void *output_payload; /*!< A pointer to the output payload buffer*/
	unsigned int large_input_payload_size;/*!< Size large input payload*/
	void *large_input_payload; /*!< A pointer to the large input buffer*/
	unsigned int large_output_payload_size;/*!< Size large output payload*/
	void *large_output_payload;/*!< A pointer to the large output buffer*/
};

/*
 * ****************************************************************************
 * Payloads for passthrough fw commands
 * ****************************************************************************
 */

/*!
 * Passthrough CR Payload:
 *		Opcode: 0x0Ah (Identify DIMM)
 *	Small Output Payload
 */
struct cr_pt_payload_identify_dimm {
	__le16 vendor_id;
	__le16 device_id;
	__le16 revision_id;
	__le16 ifc; /*!< Interface format code*/
	__u8 fwr[CR_FW_REV_LEN]; /*!< BCD formated firmware revision*/
	__u8 api_ver; /*!< BCD formated api version*/
	__u8 fswr; /*!< Feature SW Required Mask*/
	__u8 reserved;
	__le16 nbw; /*!< Number of block windows*/
	__le16 nwfa; /*!< Number write flush addresses*/
	__le32 obmcr; /*!< Offset of block mode control region*/
	__le64 rc; /*!< raw capacity*/
	char mf[CR_MFR_LEN]; /*!< ASCII Manufacturer*/
	char sn[CR_SN_LEN]; /*!< ASCII Serial Number*/
	char mn[CR_MODELNUM_LEN]; /*!< ASCII Model Number*/
	__u8 resrvd[39]; /*!< Reserved*/
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x01h (Get Security Info)
 *		Sub-Opcode:	0x00h (Get Security State)
 *	Small Output Payload
 */
struct cr_pt_payload_get_security_state {
	/*!
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

	unsigned char reserved[7]; /*!< TODO*/
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x02h (Set Security Info)
 *		Sub-Opcode:	0xF1h (Set Passphrase)
 *	Small Input Payload
 */
struct cr_pt_payload_set_passphrase {
	/*! The current security passphrase*/
	char passphrase_current[CR_PASSPHRASE_LEN];
	/*! The new passphrase to be set/changed to*/
	char passphrase_new[CR_PASSPHRASE_LEN];
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x02h (Set Security Info)
 *		Sub-Opcode:	0xF2h (Disable Passphrase)
 *		Sub-Opcode:	0xF3h (Unlock Unit)
 *	Small Input Payload
 */
struct cr_pt_payload_passphrase {
	/*! The end user passphrase*/
	char passphrase_current[CR_PASSPHRASE_LEN];
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x02h (Set Security Info)
 *		Sub-Opcode:	0xF5h (Secure Erase Unit)
 *	Small Input Payload
 */
struct cr_pt_payload_secure_erase_unit {
	 /*!< the end user passphrase*/
	char passphrase_current[CR_PASSPHRASE_LEN];
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x03h (Set SMM Security Nonce)
 *	Small Input Payload
 */
struct cr_pt_payload_smm_security_nonce {
	/*!< Security nonce to lock down SMM mailbox*/
	unsigned char sec_nonce[CR_SECURITY_NONCE_LEN];
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x04h (Get/Set Features)
 *		Sub-Opcode:	0x01h (Alarm Thresholds)
 *	Get - Small Output Payload
 *	Set - Small Input Payload
 */
struct cr_pt_payload_alarm_thresholds {
	/*!
	 * Temperatures (in Kelvin) above this threshold trigger asynchronous
	 * events and may cause a transition in the overall health state
	 */
	__le16 temperature;

	/*!
	 * When spare levels fall below this percentage based value,
	 * asynchronous events may be triggered and may cause a
	 * transition in the overall health state
	 */
	unsigned char spare;

	unsigned char reserved[5]; /*!< TODO*/
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x04h (Get Features)
 *		Sub-Opcode:	0x04h (Die Sparing Policy)
 *	Small Output Payload
 */
struct cr_pt_get_die_spare_policy {
	/*!
	 * Reflects whether the die sparing policy is enabled or disabled
	 * 0x00 - Disabled
	 * 0x01 - Enabled
	 */
	unsigned char enable;
	/*! How aggressive to be on die sparing (0...255), Default 128*/
	unsigned char aggressiveness;

	/*!
	 * Designates whether or not the DIMM still supports die sparing
	 * 0x00 - No longer available - die sparing has most likely occurred
	 * 0x01 - Still available - the dimm has not had to take action yet
	 */
	unsigned char supported;

};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x05h (Set Features)
 *		Sub-Opcode:	0x04h (Die Sparing Policy)
 *	Small Input Payload
 */
struct cr_pt_set_die_spare_policy {
	/*!
	 * Reflects whether the die sparing policy is enabled or disabled
	 * 0x00 - Disabled
	 * 0x01 - Enabled
	 */
	unsigned char enable;
	/*! How aggressive to be on die sparing (0...255), Default 128*/
	unsigned char aggressiveness;
};

/*!
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
	/*!
	 * The date returned in BCD format. (slashes are not encoded) @n
	 * MM/DD/YYYY @n
	 * MM = 2 digit month (01-12) @n
	 * DD = 2 digit day (01-31) @n
	 * YYYY = 4 digit year (0000 - 9999)
	 */
	unsigned char date[CR_BCD_DATE_LEN];

	/*!
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

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Get Admin Features)
 *		Sub-Opcode:	0x01h (Platform Config Data)
 *	Get - Small Input Payload - Data placed in Large Output Payload
 *	Set - Small Input Payload - Data read from Large Input Payload
 */
struct cr_pt_payload_platform_cfg_data {
	/*!
	 * Which Partition to access
	 * 0x01 - The first partition
	 * 0x02 - The second partition
	 * All other values are reserved
	 */
	unsigned char partition_id;

	/*!
	 * Dictates what information you want to read
	 * 0x01 - Retrieve Size
	 * 0x02 - Retrieve/Set Data
	 * All other values are resvered
	 */
	unsigned char command_parameter;

	/*! Offset in bytes of the partition to start reading from*/
	__le32 offset;

	/*! Length in bytes to read from designated offset*/
	__le32 length;
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Get Admin Features)
 *		Sub-Opcode:	0x01h (Platform Config Data)
 *	Small Output Payload
 */
struct cr_pt_payload_platform_get_size {
	__le32 size; /*! In bytes of the selected partition*/
	__le32 total_size; /*! In bytes of the platform config area*/
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Set Admin Features)
 *		Sub-Opcode:	0x01h (Platform Config Data)
 */
struct cr_pt_payload_platform_set_size {
	/*!
	 * Which Partition to access
	 * 0x01 - The first partition
	 * 0x02 - The second partition
	 * All other values are reserved
	 */
	unsigned char partition_id;

	/*!
	 * Dictates what information you want to read
	 * 0x01 - Retrieve Size
	 * 0x02 - Retrieve/Set Data
	 * All other values are resvered
	 */
	unsigned char command_parameter;

	/*! Size to set for the partition*/
	__le32 size;
};
/*!
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Get Admin Features)
 *		Sub-Opcode:	0x02h (DIMM Partition Info)
 *	Small Output Payload
 */
struct cr_pt_payload_get_dimm_partion_info {
	/*!
	 * AEP DIMM volatile memory capacity (bytes). Special Values:
	 * 0x0000000000000000 - No volatile capacity
	 * 0xFFFFFFFFFFFFFFFF - All capacity as volatile
	 */
	__le64 volatile_capacity;

	__le64 start_2lm; /*!The DPA start address of the 2LM region*/

	/*!
	 * AEP DIMM capacity (bytes) to reserve for use as PM.
	 *
	 * Special Values:
	 * 0x0000000000000000 - No persistent capacity
	 * 0xFFFFFFFFFFFFFFFF - All capacity as persistent
	 */
	__le64 pm_capacity;

	__le64 start_pm; /*! The DPA start address of the PM region*/

	/*! The raw usable size of the DIMM(Volatile + Persistent)(In Bytes)*/
	__le64 raw_capacity;

	/*!
	 * AEP DIMM volatile memory capacity(bytes), which will take effect
	 * on the next boot.
	 * Special Values:
	 * 0x0000000000000000 - No volatile capacity
	 * 0xFFFFFFFFFFFFFFFF - All capacity as volatile
	 */
	__le64 pending_volatile_capacity;

	/*!
	 * AEP DIMM persistent memory capacity(bytes), which will take effect
	 * on the next boot.
	 * Special Values:
	 * 0x0000000000000000 - No persistent capacity
	 * 0xFFFFFFFFFFFFFFFF - All capacity as persistent
	 */
	__le64 pending_pm_capacity;

	unsigned char rsvd[87];
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x07h (Set Admin Features)
 *		Sub-Opcode:	0x02h (DIMM Partition Info)
 *	Small Input Payload
 */
struct cr_pt_payload_set_dimm_partion_info {
	/*!
	 * AEP DIMM volatile memory capacity (bytes). Special Values:
	 * 0x0000000000000000 - No volatile capacity
	 * 0xFFFFFFFFFFFFFFFF - All capacity as volatile
	 */
	__le64 volatile_capacity;

	/*!
	 * AEP DIMM capacity (bytes) to reserve for use as PM.
	 *
	 * Special Values:
	 * 0x0000000000000000 - No persistent capacity
	 * 0xFFFFFFFFFFFFFFFF - All capacity as persistent
	 */
	__le64 pm_capacity;

	unsigned char rsvd[112];
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Get Admin Features)
 *		Sub-Opcode:	0x03h (FNV FW Debug Log Level)
 *	Small Input Payload
 */
struct cr_pt_input_payload_get_fnv_fw_dbg_log_level {
	/**
	 * This is the ID(0 to 255) of the log from which to retrieve the log
	 * level.
	 * Default - 0
	 */
	unsigned char log_id;
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x06h (Get Admin Features)
 *		Sub-Opcode:	0x03h (FNV FW Debug Log Level)
 *	Small Payload Output
 */
struct cr_pt_output_payload_get_fnv_fw_dbg_log_level {
	/*!
	 * The current logging level of the FNV FW (0-255).
	 *
	 * 0 = Disabled @n
	 * 1 = Error @n
	 * 2 = Warning @n
	 * 3 = Info @n
	 * 4 = Debug
	 */
	unsigned char log_level;

	/**
	 * The number of logs available for which to change the level(0 to 255)
	 */
	unsigned char logs;
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x07h (Set Admin Features)
 *		Sub-Opcode:	0x03h (FNV FW Debug Log Level)
 *	Small Payload Input
 */
struct cr_pt_payload_set_fnv_fw_dbg_log_level {
	/*!
	 * The current logging level of the FNV FW (0-255).
	 *
	 * 0 = Disabled @n
	 * 1 = Error @n
	 * 2 = Warning @n
	 * 3 = Info @n
	 * 4 = Debug
	 */
	unsigned char log_level;

	/**
	 * The ID of the log you're changing the level for (0-255)
	 */
	unsigned char log_id;
};

/*!
 * Passthrough CR Input Payload:
 *		Opcode:		0x0Eh (Get Debug Features)
 *		Sub-Opcode:	0x00h (Read CSR)
 *	Small Input Payload
 */
struct cr_pt_input_payload_read_csr {
	/*! The address of the CSR register to read*/
	__le32 csr_register_address;
};

/*!
 * Passthrough CR Output Payload:
 *		Opcode:		0x0Eh (Get Debug Features)
 *		Sub-Opcode:	0x00h (Read CSR)
 *
 *		Opcode:		0x0Fh (Set Debug Features)
 *		Sub-Opcode:	0x00h (Write CSR)
 *	Small Output Payload
 */
struct cr_pt_output_payload_read_write_csr {
	__le32 csr_value; /*!< The value of the CSR register*/
};

/*!
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

/*!
 * Passthrough CR Input Payload:
 *		Opcode:		0x0Fh (Set Debug Features)
 *		Sub-Opcode:	0x00h (Write CSR)
 */
struct cr_pt_input_payload_write_csr {
	__le32 csr_register_address; /*!< The address of the CSR register*/
	__le32 csr_value; /*!< The value to be written to the CSR register*/
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x0Bh (Inject Error)
 *		Sub-Opcode:	0x01h (Poison Error)
 *	Small Input Payload
 */
struct cr_pt_payload_poison_err {
	__le64 dpa_address; /*!<Address to set the poison bit for*/
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x0bh (Inject Error)
 *		Sub-Opcode:	0x0sh (Temperature Error)
 */
struct cr_pt_payload_temp_err {
	/*!
	 * A number representing the temperature (Kelvin) to inject
	 */
	__le16 temperature;
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x00h (SMART & Health Info)
 *	Small payload output
 */
struct cr_pt_payload_smart_health {
	/*!
	 * Overall health summary
	 * 0x00h = Normal @n
	 * 0x01h = Non-Critical (maintenance required) @n
	 * 0x02h = Critical (features or performance degraded due to failure) @n
	 * 0x03h = Fatal (data loss has occurred or is imminent) @n
	 * 0x04h = Read Only(errors have forced media to be no longer writeable
	 * 0xFF - 0x05 Reserved
	 */
	unsigned char health_status;

	__le16 temperature; /*!< Current temperature in Kelvin*/
	/*!
	 * Remaining spare capacity as a percentage of factory configured spare
	 */
	unsigned char spare;

	/*!
	 * Device life span as a percentage.
	 * 100 = warranted life span of device has been reached however values
	 * up to 255 can be used.
	 */
	unsigned char percentage_used;

	unsigned char reserved_a[11];
	unsigned char power_cycles[16]; /*!< Number of AEP DIMM power cycles*/
	/*!
	 *Lifetime hours the AEP DIMM has been powered on
	 */
	unsigned char power_on_hours[16];
	unsigned char unsafe_shutdowns[16];
	unsigned char reserved_b[64];
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x01h (Firmware Image Info)
 *	Small output payload
 */
struct cr_pt_payload_fw_image_info {
	/*!
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

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x03h (Memory Info)
 *	Small Output Payload
 */
struct cr_pt_payload_memory_info {
	/*!
	 * Number of bytes the host has read from the AEP DIMM.
	 * This value is cumulative across the life span of the device.
	 */
	unsigned char bytes_read[16];

	/*!
	 * Number of bytes the host has written to the AEP DIMM.
	 * This value is cumulative across the life span of the device.
	 */
	unsigned char bytes_written[16];

	/*!
	 * Lifetime # of read requests serviced by AEP DIMM
	 */
	unsigned char host_read_reqs[16];
	/*!
	 * Lifetime # of write requests serviced by AEP DIMM
	 */
	unsigned char host_write_cmds[16];

	unsigned char media_errors[16]; /*!< ECC uncorrectable errors.*/

	unsigned char reserved[48]; /*!< TODO*/
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x04h (Long Operations Status)
 *	Small Output Payload
 */
struct cr_pt_payload_long_op_stat {
	/*!
	 * This will coincide with the opcode & sub-opcode
	 * Bytes 7:0 - Opcode
	 * Bytes 15:8 - Sub-Opcode
	 */
	unsigned char command[16];

	/*!
	 * The % complete of the current command
	 * BCD Format
	 */
	unsigned char percent_complete[CR_BCD_PCT_COMPLETE];

	/*!
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

	unsigned char reserved[106]; /*!< TODO*/
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x08h (Get Log Page)
 *		Sub-Opcode:	0x02h (Firmware Debug Log)
 *	Small Input Payload
 *	Log returned in Large Output Payload
 */
struct cr_pt_payload_fw_debug_log {
	unsigned char log_id; /*!< The ID of the log to retrieve*/
};

/*!
 * Passthrough CR Payload:
 *		Opcode:		0x09h (Update Firmware)
 *		Sub-Opcode:	0x00h (Update FNV FW)
 *	For DDRT Large Input Payload holds new firmware image
 */

#ifdef __cplusplus
}
#endif

#endif	/* _CR_IOCTL_H_ */
