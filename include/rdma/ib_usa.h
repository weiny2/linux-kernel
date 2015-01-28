/*
 * Copyright (c) 2006-2007 Intel Corporation.  All rights reserved.
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

#ifndef IB_USA_H
#define IB_USA_H

#include <linux/types.h>
#include <rdma/ib_sa.h>

#define IB_USA_ABI_VERSION	2

#define IB_USA_EVENT_DATA	256

enum {
	IB_USA_CMD_REQUEST,
	IB_USA_CMD_GET_EVENT,
	IB_USA_CMD_FREE,
	IB_USA_CMD_GET_PATH,
	IB_USA_CMD_GET_HCA
};

enum {
	IB_SA_ATTR_NOTICE_LEN = 80,
	IB_SA_ATTR_INFORM_INFO_LEN = 36,
	IB_SA_ATTR_MC_MEMBER_REC_LEN = 52,
	IB_SA_ATTR_PATH_REC_LEN = 64 /* matches size of ib_path_rec_t */
};

struct ib_usa_cmd_hdr {
	__u32 cmd;
	__u16 in;
	__u16 out;
};

struct ib_usa_request {
	__u64 response;
	__u64 uid;
	__u64 node_guid;
	__u64 comp_mask;
	__u64 attr;
	__be16 attr_id;
	__u8  method;
	__u8  port_num;
	__u8  local;
};

struct ib_usa_free {
	__u64 response;
	__u32 id;
	__be16 attr_id;
};

struct ib_usa_free_resp {
	__u32 events_reported;
};

struct ib_usa_get_event {
	__u64 response;
};

struct ib_usa_event_resp {
	__u64 uid;
	__u32 id;
	__u32 status;
	__u32 data_len;
	__be16 attr_id;
	__u16 reserved;
	__u8  data[IB_USA_EVENT_DATA];
};

struct ib_usa_path {
	__u64 hca_context;
	__u8  port_num;
	__u64 timeout;
	__u8 *query;
	__u8 *response;
} __attribute__((__packed__));

struct ib_usa_hca {
	__u8 hca_name[IB_DEVICE_NAME_MAX];
	__u8 port_num;
	__u64 hca_context;
} __attribute__((__packed__));

#endif /* IB_USA_H */
