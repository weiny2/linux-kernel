// SPDX-License-Identifier: Apache-2.0
/*
 *  Copyright 2016-2019, Intel Corporation
 */

#ifndef KDI_CMD_DEFS_H
#define KDI_CMD_DEFS_H

enum kdi_command_id {
	KDI_SESSION_CREATE,
	KDI_SESSION_CLOSE,
	KDI_SEND_AND_RCV,
	KDI_VERSION_GET_INFO,
	KDI_EXCLUSIVE_ACCESS_SET,
	KDI_EXCLUSIVE_ACCESS_REMOVE
};

/**
 * struct kdi_test_command - contains the command data to be sent
 * to the kdi_test module.
 * @cmd_id: the command id (kdi_command_id type)
 * @data: the actual command data.
 */
struct kdi_test_command {
	uint8_t cmd_id;
	unsigned char data[0];
} __attribute__ ((__packed__));

struct session_create_cmd {
	uint32_t app_id_len;           /* length app_id arg (valid len is 33) */
	uint32_t acp_pkg_len;          /* length of the acp_pkg arg */
	uint32_t init_param_len;       /* length of init param arg */
	uint8_t is_session_handle_ptr; /* either send kdi session handle or NULL */
	unsigned char data[0];
} __attribute__ ((__packed__));

struct session_create_resp {
	uint64_t session_handle;
	int32_t test_mod_status;
	int32_t status;
} __attribute__ ((__packed__));

struct session_close_cmd {
	uint64_t session_handle;
} __attribute__ ((__packed__));

struct session_close_resp {
	int32_t test_mod_status;
	int32_t status;
} __attribute__ ((__packed__));

struct send_and_rcv_cmd {
	uint64_t session_handle;
	uint32_t command_id;
	uint32_t output_buf_len;     /* the size of output buffer */
	uint8_t is_output_buf;       /* either send kdi output buffer or NULL */
	uint8_t is_output_len_ptr;   /* either send kdi output len ptr or NULL */
	uint8_t is_response_code_ptr;/* either send kdi res code ptr or NULL */
	unsigned char input[0];
} __attribute__ ((__packed__));

struct send_and_rcv_resp {
	int32_t test_mod_status;
	int32_t status;
	int32_t response_code;
	uint32_t output_len;
	unsigned char output[0];
} __attribute__ ((__packed__));

struct version_get_info_cmd {
	uint8_t is_version_ptr; /* either send kdi version info ptr or NULL */
} __attribute__ ((__packed__));

struct version_get_info_resp {
	char kdi_version[32];
	uint32_t reserved[4];
	int32_t test_mod_status;
	int32_t status;
} __attribute__ ((__packed__));

struct ta_access_set_remove_cmd {
	uint32_t app_id_len;
	unsigned char data[0];
} __attribute__ ((__packed__));

struct ta_access_set_remove_resp {
	int32_t test_mod_status;
	int32_t status;
} __attribute__ ((__packed__));

#endif /* KDI_CMD_DEFS_H */
