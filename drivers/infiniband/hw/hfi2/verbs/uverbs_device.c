/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2017 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Intel(R) OPA Gen2 IB Driver
 */
#include "uverbs_obj.h"
#include "hfi2.h"
#include "timesync.h"

static int hfi_e2e_conn(struct hfi_ibcontext *uc,
			struct hfi_e2e_conn_args __user *e2e_args)
{
	int num_conn = 0;
#ifdef CONFIG_HFI2_STLNP
	struct hfi_e2e_conn e2e_conn;
	struct hfi_e2e_conn_args e2e;
	int i;

	/* Copy the details for this array of connection requests */
	if (copy_from_user(&e2e, e2e_args, sizeof(*e2e_args)))
		return -EFAULT;
	for (i = 0; i < e2e.num_req; i++) {
		/* Copy the details for this connection */
		if (copy_from_user(&e2e_conn, &e2e.e2e_req[i],
				   sizeof(e2e_conn)))
			return -EFAULT;
		/* Initiate the E2E connection */
		e2e_conn.conn_status = hfi_e2e_ctrl(uc, &e2e_conn);
		/* Inform user space about the connection status */
		if (copy_to_user(&e2e.e2e_req[i].conn_status,
				 &e2e_conn.conn_status,
				 sizeof(e2e_conn.conn_status)))
			return -EFAULT;
		if (!e2e_conn.conn_status)
			num_conn++;
	}
#endif

	/* Inform user space about the number of connections established */
	if (copy_to_user(&e2e_args->num_conn, &num_conn, sizeof(num_conn)))
		return -EFAULT;
	return 0;
}

static int hfi2_e2e_connect(struct ib_device *ib_dev,
			    struct ib_uverbs_file *file,
			    struct uverbs_attr_array *attrs,
			    size_t num)
{
	struct uverbs_attr *attr = &attrs->attrs[0];

	/* copy_from_user handled in hfi_e2e_conn */
	return hfi_e2e_conn((struct hfi_ibcontext *)file->ucontext,
			    attr->ptr_attr.ptr);
}

static int hfi2_check_sl_pair(struct ib_device *ib_dev,
			      struct ib_uverbs_file *file,
			      struct uverbs_attr_array *attrs,
			      size_t num)
{
	struct hfi_sl_pair slp;
	int ret;

	ret = uverbs_copy_from(&slp, attrs, HFI2_SL_PAIR_ATTR);
	if (ret)
		return ret;
	return hfi_check_sl_pair((struct hfi_ibcontext *)file->ucontext, &slp);
}

static int hfi2_get_hw_limits(struct ib_device *ib_dev,
			      struct ib_uverbs_file *file,
			      struct uverbs_attr_array *attrs,
			      size_t num)
{
	struct uverbs_attr_array *common = &attrs[0];
	struct hfi_hw_limit hwl;

	hfi_get_hw_limits((struct hfi_ibcontext *)file->ucontext, &hwl);

	return uverbs_copy_to(common, HFI2_GET_HW_LIMITS_RESP, &hwl);
}

static int hfi2_ts_get_master(struct ib_device *ib_dev,
			     struct ib_uverbs_file *file,
			     struct uverbs_attr_array *attrs,
			     size_t num)
{
#ifdef CONFIG_HFI2_STLNP
	struct uverbs_attr_array *common = &attrs[0];
	struct hfi_ts_master_regs master_ts;
	struct hfi_ts_master_cmd cmd;
	struct hfi_ts_master_resp resp;
	int ret;

	ret = uverbs_copy_from(&cmd, common, HFI2_GET_TS_MASTER_CMD);
	if (ret)
		return ret;

	master_ts.clkid = cmd.clkid;
	master_ts.port = cmd.port;

	ret = hfi_get_ts_master_regs((struct hfi_ibcontext *)file->ucontext,
				     &master_ts);
	if (ret)
		return ret;

	resp.master = master_ts.master;
	resp.timestamp = master_ts.timestamp;

	return uverbs_copy_to(common, HFI2_GET_TS_MASTER_RESP, &resp);
#endif
	return 0;
}

static int hfi2_ts_get_fm(struct ib_device *ib_dev,
			  struct ib_uverbs_file *file,
			  struct uverbs_attr_array *attrs,
			  size_t num)
{
#ifdef CONFIG_HFI2_STLNP
	struct uverbs_attr_array *common = &attrs[0];
	struct hfi_ts_fm_data fm_data;
	struct hfi_ts_fm_cmd cmd;
	struct hfi_ts_fm_resp resp;
	int ret;

	ret = uverbs_copy_from(&cmd, common, HFI2_GET_TS_FM_CMD);
	if (ret)
		return ret;

	fm_data.hack_timesync = cmd.hack_timesync;
	fm_data.port = cmd.port;

	ret = hfi_get_ts_fm_data((struct hfi_ibcontext *)file->ucontext,
				 &fm_data);
	if (ret)
		return ret;

	resp.clock_offset = fm_data.clock_offset;
	resp.clock_delay = fm_data.clock_delay;
	resp.periodicity = fm_data.periodicity;
	resp.current_clock_id = fm_data.current_clock_id;
	resp.ptp_index = fm_data.ptp_index;
	resp.is_active_master = fm_data.is_active_master;

	return uverbs_copy_to(common, HFI2_GET_TS_FM_RESP, &resp);
#endif
	return 0;
}

static DECLARE_UVERBS_ATTR_SPEC(
			hfi2_e2e_connect_spec,
			UVERBS_ATTR_PTR_IN(
				HFI2_E2E_CONN_ATTR,
				struct hfi_e2e_conn_args,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

static DECLARE_UVERBS_ATTR_SPEC(
			hfi2_check_sl_pair_spec,
			UVERBS_ATTR_PTR_IN(
				HFI2_SL_PAIR_ATTR,
				struct hfi_sl_pair,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

static DECLARE_UVERBS_ATTR_SPEC(
			hfi2_get_hw_limits_spec,
			UVERBS_ATTR_PTR_OUT(
				HFI2_GET_HW_LIMITS_RESP,
				struct hfi_hw_limit,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

static DECLARE_UVERBS_ATTR_SPEC(
			hfi2_ts_get_master_spec,
			UVERBS_ATTR_PTR_IN(
				HFI2_GET_TS_MASTER_CMD,
				struct hfi_ts_master_cmd,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
			UVERBS_ATTR_PTR_OUT(
				HFI2_GET_TS_MASTER_RESP,
				struct hfi_ts_master_resp,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

static DECLARE_UVERBS_ATTR_SPEC(
			hfi2_ts_get_fm_spec,
			UVERBS_ATTR_PTR_IN(
				HFI2_GET_TS_FM_CMD,
				struct hfi_ts_fm_cmd,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
			UVERBS_ATTR_PTR_OUT(
				HFI2_GET_TS_FM_RESP,
				struct hfi_ts_fm_resp,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_TYPE(hfi2_type_device, NULL,
		    &UVERBS_ACTIONS(
			ADD_UVERBS_ACTION(HFI2_DEV_E2E_CONNECT,
					  hfi2_e2e_connect,
					  &hfi2_e2e_connect_spec),
			ADD_UVERBS_ACTION(HFI2_DEV_CHK_SL_PAIR,
					  hfi2_check_sl_pair,
					  &hfi2_check_sl_pair_spec),
			ADD_UVERBS_ACTION(HFI2_DEV_GET_HW_LIMITS,
					  hfi2_get_hw_limits,
					  &hfi2_get_hw_limits_spec),
			ADD_UVERBS_ACTION(HFI2_DEV_TS_GET_MASTER,
					  hfi2_ts_get_master,
					  &hfi2_ts_get_master_spec),
			ADD_UVERBS_ACTION(HFI2_DEV_TS_GET_FM,
					  hfi2_ts_get_fm,
					  &hfi2_ts_get_fm_spec)));
