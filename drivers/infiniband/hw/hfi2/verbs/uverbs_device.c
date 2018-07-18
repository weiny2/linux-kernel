/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2017-2018 Intel Corporation.
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
 * Copyright (c) 2017-2018 Intel Corporation.
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
#include "verbs/verbs.h"
#include "timesync.h"
#include <rdma/uverbs_ioctl.h>

static int hfi2_abi_info_handler(struct ib_device *ib_dev,
				 struct ib_uverbs_file *file,
				 struct uverbs_attr_bundle *attrs)
{
	u32 object_id;
	u64 ver;
	int ret;

	ret = uverbs_copy_from(&object_id, attrs, HFI2_ABI_INFO_OBJECT_ID);
	if (ret)
		return ret;

	switch (object_id) {
	case HFI2_OBJECT_DEVICE:
		ver = HFI2_OBJECT_DEVICE_ABI_VERSION;
		break;
	case HFI2_OBJECT_CMDQ:
		ver = HFI2_OBJECT_CMDQ_ABI_VERSION;
		break;
	case HFI2_OBJECT_CTX:
		ver = HFI2_OBJECT_CTX_ABI_VERSION;
		break;
	case HFI2_OBJECT_JOB:
		ver = HFI2_OBJECT_JOB_ABI_VERSION;
		break;
	default:
		return -EINVAL;
	}

	return uverbs_copy_to(attrs, HFI2_ABI_INFO_VERSION, &ver, sizeof(ver));
}

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

static int hfi2_e2e_connect_handler(struct ib_device *ib_dev,
				    struct ib_uverbs_file *file,
				    struct uverbs_attr_bundle *attrs)
{
	const struct uverbs_attr *uattr = uverbs_attr_get(attrs,
							  HFI2_E2E_CONN_ATTR);

	if (unlikely(IS_ERR(uattr)))
		return PTR_ERR(uattr);

	/* copy_from_user handled in hfi_e2e_conn */
	return hfi_e2e_conn((struct hfi_ibcontext *)file->ucontext,
			    u64_to_user_ptr(uattr->ptr_attr.data));
}

static int hfi2_check_sl_pair_handler(struct ib_device *ib_dev,
				      struct ib_uverbs_file *file,
				      struct uverbs_attr_bundle *attrs)
{
	struct hfi_sl_pair slp;
	int ret;

	ret = uverbs_copy_from(&slp, attrs, HFI2_SL_PAIR_ATTR);
	if (ret)
		return ret;
	return hfi_check_sl_pair((struct hfi_ibcontext *)file->ucontext, &slp);
}

static int hfi2_get_sl_info_handler(struct ib_device *ib_dev,
				    struct ib_uverbs_file *file,
				    struct uverbs_attr_bundle *attrs)
{
	struct hfi_ibcontext *hfi_context = container_of(file->ucontext,
							 struct hfi_ibcontext,
							 ibuc);
	struct hfi_sl_pair slp;
	u16 mtu;
	bool is_pair = false;
	int ret;

	ret = uverbs_copy_from(&slp.sl1, attrs, HFI2_SL_INFO_REQ_SL);
	if (ret)
		return ret;
	ret = uverbs_copy_from(&slp.sl2, attrs, HFI2_SL_INFO_RESP_SL);
	if (ret)
		return ret;

	if (slp.sl1 >= OPA_MAX_SLS || slp.sl2 >= OPA_MAX_SLS)
		return -EINVAL;

	slp.port = 1;
	mtu = mtu_from_sl(to_hfi_ibp(ib_dev, slp.port), slp.sl1);
	if (slp.sl1 != slp.sl2)
		is_pair = hfi_check_sl_pair(hfi_context, &slp) == 0;

	ret = uverbs_copy_to(attrs, HFI2_SL_INFO_MTU, &mtu, sizeof(mtu));
	if (!ret)
		ret = uverbs_copy_to(attrs, HFI2_SL_INFO_IS_PAIR, &is_pair,
				     sizeof(is_pair));
	return ret;
}

static int hfi2_get_hw_limits_handler(struct ib_device *ib_dev,
				      struct ib_uverbs_file *file,
				      struct uverbs_attr_bundle *attrs)
{
	struct hfi_hw_limit hwl;

	hfi_get_hw_limits((struct hfi_ibcontext *)file->ucontext, &hwl);

	return uverbs_copy_to(attrs, HFI2_GET_HW_LIMITS_RESP,
			      &hwl, sizeof(hwl));
}

static int hfi2_ts_get_master_handler(struct ib_device *ib_dev,
				      struct ib_uverbs_file *file,
				      struct uverbs_attr_bundle *attrs)
{
#ifdef CONFIG_HFI2_STLNP
	struct hfi_ts_master_regs master_ts;
	int ret;

	ret = uverbs_copy_from(&master_ts.clkid, attrs,
			       HFI2_GET_TS_MASTER_CLK_ID);
	ret += uverbs_copy_from(&master_ts.port, attrs,
				HFI2_GET_TS_MASTER_PORT);
	if (ret)
		return ret;

	ret = hfi_get_ts_master_regs((struct hfi_ibcontext *)file->ucontext,
				     &master_ts);
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, HFI2_GET_TS_MASTER_TIME,
			     &master_ts.master, sizeof(uint64_t));
	ret += uverbs_copy_to(attrs, HFI2_GET_TS_MASTER_TIMESTAMP,
			      &master_ts.timestamp, sizeof(uint64_t));
	return ret;
#endif
	return 0;
}

static int hfi2_ts_get_fm_handler(struct ib_device *ib_dev,
				  struct ib_uverbs_file *file,
				  struct uverbs_attr_bundle *attrs)
{
#ifdef CONFIG_HFI2_STLNP
	struct hfi_ts_fm_data fm_data;
	int ret;

	ret = uverbs_copy_from(&fm_data.hack_timesync, attrs,
			       HFI2_GET_TS_FM_HACK);
	ret += uverbs_copy_from(&fm_data.port, attrs,
				HFI2_GET_TS_FM_PORT);
	if (ret)
		return ret;

	ret = hfi_get_ts_fm_data((struct hfi_ibcontext *)file->ucontext,
				 &fm_data);
	if (ret)
		return ret;

	ret = uverbs_copy_to(attrs, HFI2_GET_TS_FM_CLK_OFFSET,
			     &fm_data.clock_offset, sizeof(uint64_t));
	ret += uverbs_copy_to(attrs, HFI2_GET_TS_FM_CLK_DELAY,
			      &fm_data.clock_delay, sizeof(uint16_t));
	ret += uverbs_copy_to(attrs, HFI2_GET_TS_FM_PERIODICITY,
			      &fm_data.periodicity, sizeof(uint16_t));
	ret += uverbs_copy_to(attrs, HFI2_GET_TS_FM_CURRENT_CLK_ID,
			      &fm_data.current_clock_id, sizeof(uint8_t));
	ret += uverbs_copy_to(attrs, HFI2_GET_TS_FM_PTP_IDX,
			      &fm_data.ptp_index, sizeof(uint8_t));
	ret += uverbs_copy_to(attrs, HFI2_GET_TS_FM_IS_ACTIVE_MASTER,
			      &fm_data.is_active_master, sizeof(uint8_t));
	return ret;
#endif
	return 0;
}

DECLARE_UVERBS_METHOD(hfi2_abi_info, HFI2_DEV_GET_ABI_INFO,
		      hfi2_abi_info_handler,
		      &UVERBS_ATTR_PTR_IN(
				HFI2_ABI_INFO_OBJECT_ID, u32,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_ABI_INFO_VERSION, u64,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_e2e_connect, HFI2_DEV_E2E_CONNECT,
		      hfi2_e2e_connect_handler,
		      &UVERBS_ATTR_PTR_IN(
				HFI2_E2E_CONN_ATTR,
				struct hfi_e2e_conn_args,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_check_sl_pair, HFI2_DEV_CHK_SL_PAIR,
		      hfi2_check_sl_pair_handler,
		      &UVERBS_ATTR_PTR_IN(
				HFI2_SL_PAIR_ATTR,
				struct hfi_sl_pair,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_get_sl_info, HFI2_DEV_GET_SL_INFO,
		      hfi2_get_sl_info_handler,
		      &UVERBS_ATTR_PTR_IN(
				HFI2_SL_INFO_REQ_SL, u8,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_IN(
				HFI2_SL_INFO_RESP_SL, u8,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_SL_INFO_IS_PAIR, bool,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_SL_INFO_MTU, u16,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_get_hw_limits, HFI2_DEV_GET_HW_LIMITS,
		      hfi2_get_hw_limits_handler,
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_GET_HW_LIMITS_RESP,
				struct hfi_hw_limit,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_ts_get_master, HFI2_DEV_TS_GET_MASTER,
		      hfi2_ts_get_master_handler,
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_GET_TS_MASTER_TIME, uint64_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_GET_TS_MASTER_TIMESTAMP, uint64_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_IN(
				HFI2_GET_TS_MASTER_CLK_ID, uint8_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_IN(
				HFI2_GET_TS_MASTER_PORT, uint8_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_METHOD(hfi2_ts_get_fm, HFI2_DEV_TS_GET_FM,
		      hfi2_ts_get_fm_handler,
		      &UVERBS_ATTR_PTR_IN(
				HFI2_GET_TS_FM_PORT, uint8_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_IN(
				HFI2_GET_TS_FM_HACK, uint64_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_GET_TS_FM_CLK_OFFSET, uint64_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_GET_TS_FM_CLK_DELAY, uint16_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_GET_TS_FM_PERIODICITY, uint16_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_GET_TS_FM_CURRENT_CLK_ID, uint8_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_GET_TS_FM_PTP_IDX, uint8_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)),
		      &UVERBS_ATTR_PTR_OUT(
				HFI2_GET_TS_FM_IS_ACTIVE_MASTER, uint8_t,
				UA_FLAGS(UVERBS_ATTR_SPEC_F_MANDATORY)));

DECLARE_UVERBS_OBJECT(hfi2_object_device,
		      UVERBS_CREATE_NS_INDEX(HFI2_OBJECT_DEVICE,
					     HFI2_OBJECTS),
		      NULL,
		      &hfi2_abi_info,
		      &hfi2_e2e_connect,
		      &hfi2_check_sl_pair,
		      &hfi2_get_sl_info,
		      &hfi2_get_hw_limits,
		      &hfi2_ts_get_master,
		      &hfi2_ts_get_fm);
