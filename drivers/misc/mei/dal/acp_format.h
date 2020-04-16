/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(c) 2016-2019, Intel Corporation.
 */

#ifndef _ACP_FORMAT_H
#define _ACP_FORMAT_H

#include <linux/uuid.h>

#define AC_MAX_INS_REASONS_LENGTH 1024
#define AC_MAX_USED_SERVICES 20
#define AC_MAX_PROPS_LENGTH 2048
#define AC_MAX_PACK_HASH_LEN 32

/**
 * enum ac_cmd_id - acp file command (acp type)
 *
 * @AC_CMD_INVALID: invalid command
 * @AC_INSTALL_SD: install new sub security domain
 * @AC_UNINSTALL_SD: uninstall sub security domain
 * @AC_INSTALL_JTA: install java ta
 * @AC_UNINSTALL_JTA: uninstall java ta
 * @AC_INSTALL_NTA: install native ta (currently NOT SUPPORTED)
 * @AC_UNINSTALL_NTA: uninstall native ta (currently NOT SUPPORTED)
 * @AC_UPDATE_SVL: update the security version list
 * @AC_INSTALL_JTA_PROP: ta properties for installation
 * @AC_CMD_NUM: number of acp commands
 */
enum ac_cmd_id {
	AC_CMD_INVALID,
	AC_INSTALL_SD,
	AC_UNINSTALL_SD,
	AC_INSTALL_JTA,
	AC_UNINSTALL_JTA,
	AC_INSTALL_NTA,
	AC_UNINSTALL_NTA,
	AC_UPDATE_SVL,
	AC_INSTALL_JTA_PROP,
	AC_CMD_NUM
};

/**
 * struct ac_pack_hash - ta pack hash
 *
 * @data: ta hash
 */
struct ac_pack_hash {
	u8 data[AC_MAX_PACK_HASH_LEN];
} __packed;

/**
 * struct ac_pack_header - admin comman pack header
 *
 * @magic: magic string which represents an ACP
 * @version: package format version
 * @byte_order: byte order of package (0 big endian, 1 little endian)
 * @reserved: reserved bytes
 * @size: total package size
 * @cmd_id: acp command (acp file type)
 * @svn: security version number
 *
 * @idx_num: the number of the indexed sections
 * @idx_condition: condition section offset
 * @idx_data: data section offset
 */
struct ac_pack_header {
	/*ACP Header*/
	u8 magic[4];
	u8 version;
	u8 byte_order;
	u16 reserved;
	u32 size;
	u32 cmd_id;
	u32 svn;

	/* Index Section */
	u32 idx_num;
	u32 idx_condition;
	u32 idx_data;
} __packed;

/**
 * struct ac_ta_id_list - A list of ta ids which the ta
 *    is allowed to communicate with.
 *
 * @num: ta ids count
 * @list: ta ids list
 */
struct ac_ta_id_list {
	u32 num;
	uuid_t list[];
} __packed;

/**
 * struct ac_prop_list - TLV list of acp properties
 *
 * @num: number of properties
 * @len: size of all properties
 * @data: acp properties. TLV format is "type\0key\0value\0"
 *        (e.g. string\0name\0Tom\0int\0Age\013\0)
 */
struct ac_prop_list {
	u32 num;
	u32 len;
	s8 data[];
} __packed;

/**
 * struct ac_ins_reasons - list of event codes that can be
 *     received or posted by ta
 *
 * @len: event codes count
 * @data: event codes list
 */
struct ac_ins_reasons {
	u32 len;
	u32 data[];
} __packed;

/**
 * struct ac_pack - general struct to hold parsed acp content
 *
 * @head: acp pack header
 * @data: acp parsed content
 */
struct ac_pack {
	struct ac_pack_header *head;
	char data[];
} __packed;

/**
 * struct ac_ins_ta_header - ta installation header
 *
 * @ta_id: ta id
 * @ta_svn: ta security version number
 * @hash_alg_type: ta hash algorithm type
 * @ta_reserved: reserved bytes
 * @hash: ta pack hash
 */
struct ac_ins_ta_header {
	uuid_t ta_id;
	u32 ta_svn;
	u8 hash_alg_type;
	u8 ta_reserved[3];
	struct ac_pack_hash hash;
} __packed;

/**
 * struct ac_ins_jta_pack - ta installation information
 *
 * @ins_cond: ta install conditions (contains some of the manifest data,
 *            including security.version, applet.version, applet.platform,
 *            applet.api.level)
 * @head: ta installation header
 */
struct ac_ins_jta_pack {
	struct ac_prop_list *ins_cond;
	struct ac_ins_ta_header *head;
} __packed;

/**
 * struct ac_ins_jta_prop_header - ta manifest header
 *
 * @mem_quota: ta heap size
 * @ta_encrypted: ta encrypted by provider flag
 * @padding: padding
 * @allowed_inter_session_num: allowed internal session count
 * @ac_groups: ta permission groups
 * @timeout: ta timeout in milliseconds
 */
struct ac_ins_jta_prop_header {
	u32 mem_quota;
	u8 ta_encrypted;
	u8 padding;
	u16 allowed_inter_session_num;
	u64 ac_groups;
	u32 timeout;
} __packed;

/**
 * struct ac_ins_jta_prop - ta manifest
 *
 * @head: manifest header
 * @post_reasons: list of event codes that can be posted by ta
 * @reg_reasons: list of event codes that can be received by ta
 * @prop: all other manifest fields (acp properties)
 * @used_service_list: list of ta ids which ta is allowed to communicate with
 */
struct ac_ins_jta_prop {
	struct ac_ins_jta_prop_header *head;
	struct ac_ins_reasons *post_reasons;
	struct ac_ins_reasons *reg_reasons;
	struct ac_prop_list *prop;
	struct ac_ta_id_list *used_service_list;
} __packed;

#endif /* _ACP_FORMAT_H */
