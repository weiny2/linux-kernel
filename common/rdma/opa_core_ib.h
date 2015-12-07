/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
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
 * Copyright (c) 2015 Intel Corporation.
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
 * Intel(R) Omni-Path Core Driver Interface
 */
#ifndef _OPA_CORE_IB_H_
#define _OPA_CORE_IB_H_
#include <rdma/opa_smi.h>
#include <rdma/opa_port_info.h>

/*
 * QP numbering defines
 * Unique QPN[23..16] value (KDETH_BASE) determines start of the KDETH
 * QPNs (lower 16 bits = 64K QPNs).
 * TODO - For now just limit the Verbs QPNs to the 23-bits below the
 * KDETH base.  Support to allocate from KDETH range (for TID RDMA) or
 * the ones above the KDETH range is not yet provided.
 */
#define OPA_QPN_MAX		BIT(24)
#define OPA_QPN_KDETH_BASE	BIT(23) /* Gen1 default */
#define OPA_QPN_VERBS_MAX	OPA_QPN_KDETH_BASE
#define OPA_QPN_MAP_MAX		BIT(8)	/* 8-bits to map to Recv Context */

#define OPA_NUM_PKEY_BLOCKS_PER_SMP 	(OPA_SMP_DR_DATA_SIZE \
			/ (OPA_PKEY_TABLE_BLK_COUNT * sizeof(u16)))

#define OPA_LIM_MGMT_PKEY		0x7FFF
#define OPA_FULL_MGMT_PKEY		0xFFFF
#define OPA_LIM_MGMT_PKEY_IDX		1
#define OPA_PKEY_TABLE_BLK_COUNT	OPA_PARTITION_TABLE_BLK_SIZE

/* OPA SMA aggregate */
#define OPA_MAX_AGGR_ATTR		117
#define OPA_MIN_AGGR_ATTR		1
#define OPA_AGGR_REQ_LEN_SHIFT		0
#define OPA_AGGR_REQ_LEN_MASK		0x7f
#define OPA_AGGR_REQ_LEN(req)		(((req) >> OPA_AGGR_REQ_LEN_SHIFT) & \
					OPA_AGGR_REQ_LEN_MASK)
#define OPA_AGGR_REQ_BYTES_PER_UNIT	8
#define OPA_AGGR_ERROR			cpu_to_be16(0x8000)

/* OPA SMA attribute IDs */
#define OPA_ATTRIB_ID_CONGESTION_INFO		cpu_to_be16(0x008b)
#define OPA_ATTRIB_ID_HFI_CONGESTION_LOG	cpu_to_be16(0x008f)
#define OPA_ATTRIB_ID_HFI_CONGESTION_SETTING	cpu_to_be16(0x0090)
#define OPA_ATTRIB_ID_CONGESTION_CONTROL_TABLE	cpu_to_be16(0x0091)

/* OPA PMA attribute IDs */
#define OPA_PM_ATTRIB_ID_PORT_STATUS		cpu_to_be16(0x0040)
#define OPA_PM_ATTRIB_ID_CLEAR_PORT_STATUS	cpu_to_be16(0x0041)
#define OPA_PM_ATTRIB_ID_DATA_PORT_COUNTERS	cpu_to_be16(0x0042)
#define OPA_PM_ATTRIB_ID_ERROR_PORT_COUNTERS	cpu_to_be16(0x0043)
#define OPA_PM_ATTRIB_ID_ERROR_INFO		cpu_to_be16(0x0044)

/* attribute modifier macros */
#define OPA_AM_NPORT_SHIFT	24
#define OPA_AM_NPORT_MASK	0xff
#define OPA_AM_NPORT_SMASK	(OPA_AM_NPORT_MASK << OPA_AM_NPORT_SHIFT)
#define OPA_AM_NPORT(am)	(((am) >> OPA_AM_NPORT_SHIFT) & \
					OPA_AM_NPORT_MASK)

#define OPA_AM_NBLK_SHIFT	24
#define OPA_AM_NBLK_MASK	0xff
#define OPA_AM_NBLK_SMASK	(OPA_AM_NBLK_MASK << OPA_AM_NBLK_SHIFT)
#define OPA_AM_NBLK(am)		(((am) >> OPA_AM_NBLK_SHIFT) & \
					OPA_AM_NBLK_MASK)

#define OPA_AM_START_BLK_SHIFT	0
#define OPA_AM_START_BLK_MASK	0x7ff
#define OPA_AM_START_BLK_SMASK	(OPA_AM_START_BLK_MASK << \
					OPA_AM_START_BLK_SHIFT)
#define OPA_AM_START_BLK(am)	(((am) >> OPA_AM_START_BLK_SHIFT) & \
					OPA_AM_START_BLK_MASK)

#define OPA_AM_START_SM_CFG_SHIFT	9
#define OPA_AM_START_SM_CFG_MASK	0x1
#define OPA_AM_START_SM_CFG_SMASK	(OPA_AM_START_SM_CFG_MASK << \
						OPA_AM_START_SM_CFG_SHIFT)
#define OPA_AM_START_SM_CFG(am)		(((am) >> OPA_AM_START_SM_CFG_SHIFT) \
						& OPA_AM_START_SM_CFG_MASK)

#define OPA_AM_ASYNC_SHIFT	12
#define OPA_AM_ASYNC_MASK	0x1
#define OPA_AM_ASYNC_SMASK	(OPA_AM_ASYNC_MASK << OPA_AM_ASYNC_SHIFT)
#define OPA_AM_ASYNC(am)	(((am) >> OPA_AM_ASYNC_SHIFT) & \
					OPA_AM_ASYNC_MASK)

#define OPA_MTU_0     0
#define OPA_INVALID_LID_MASK	0xFFFF0000
#define IS_VALID_LID_SIZE(lid) (!((lid) & (OPA_INVALID_LID_MASK)))
#define INVALID_MTU	0xffff
#define INVALID_MTU_ENC	0xff

#define OPA_AM_NATTR_SHIFT	0
#define OPA_AM_NATTR_MASK	0xff
#define OPA_AM_NATTR(am)	(((am) >> OPA_AM_NATTR_SHIFT) & \
					OPA_AM_NATTR_MASK)

#define OPA_AM_CI_ADDR_SHIFT	19
#define OPA_AM_CI_ADDR_MASK	0xfff
#define OPA_AM_CI_ADDR_SMASK	(OPA_AM_CI_ADDR_MASK << OPA_CI_ADDR_SHIFT)
#define OPA_AM_CI_ADDR(am)	(((am) >> OPA_AM_CI_ADDR_SHIFT) & \
					OPA_AM_CI_ADDR_MASK)

#define OPA_AM_CI_LEN_SHIFT	13
#define OPA_AM_CI_LEN_MASK	0x3f
#define OPA_AM_CI_LEN_SMASK	(OPA_AM_CI_LEN_MASK << OPA_CI_LEN_SHIFT)
#define OPA_AM_CI_LEN(am)	(((am) >> OPA_AM_CI_LEN_SHIFT) & \
					OPA_AM_CI_LEN_MASK)

/*
 * Convert 4bit number format
 * described in IB 1.3 (14.2.5.6) and OPA spec
 * to MTU sizes in bytes
 */
static inline u16 opa_enum_to_mtu(u8 emtu)
{
	switch (emtu) {
	case OPA_MTU_0:     return 0;
	case IB_MTU_256:   return 256;
	case IB_MTU_512:   return 512;
	case IB_MTU_1024:  return 1024;
	case IB_MTU_2048:  return 2048;
	case IB_MTU_4096:  return 4096;
	case OPA_MTU_8192:  return 8192;
	case OPA_MTU_10240: return 10240;
	default: return INVALID_MTU;
	}
}

/*
 * Convert MTU sizes to 4bit number format
 * described in IB 1.3 (14.2.5.6) and OPA spec
 */
static inline u8 opa_mtu_to_enum(u16 mtu)
{
	switch (mtu) {
	case   	 0: return OPA_MTU_0;
	case   256: return IB_MTU_256;
	case   512: return IB_MTU_512;
	case  1024: return IB_MTU_1024;
	case  2048: return IB_MTU_2048;
	case  4096: return IB_MTU_4096;
	case  8192: return OPA_MTU_8192;
	case 10240: return OPA_MTU_10240;
	default: return INVALID_MTU_ENC;
	}
}

static inline u8 opa_mtu_to_enum_safe(u16 mtu, u8 if_bad_mtu)
{
	u8 ibmtu = opa_mtu_to_enum(mtu);

	return (ibmtu == INVALID_MTU_ENC) ? if_bad_mtu : ibmtu;
}

/*
 * Convert MTU sizes to compressed 3-bit encoding (0..7)
 * TODO: this is FXR specific, can move to opa2 directory
 *  when VPD is merged with PCIe driver
 */
static inline u8 opa_mtu_to_id(u16 mtu)
{
	switch (mtu) {
	case   256: return 1;
	case   512: return 2;
	case  1024: return 3;
	case  2048: return 4;
	case  4096: return 5;
	case  8192: return 6;
	case 10240: return 7;
	default: return INVALID_MTU_ENC;
	}
}

/*
 * Helper function to read mtu field for a given VL from
 * opa_port_info
 */
static inline u16 opa_pi_to_mtu(struct opa_port_info *pi, int vl_num)
{
	u8 mtu_enc;
	u16 mtu;

	mtu = pi->neigh_mtu.pvlx_to_mtu[vl_num / 2];
	if (!(vl_num % 2))
		mtu_enc = (mtu >> 4) & 0xF;
	else
		mtu_enc = mtu & 0xF;

	mtu = opa_enum_to_mtu(mtu_enc);

	WARN(mtu == INVALID_MTU,
		"SubnSet(OPA_PortInfo) mtu(enc) %u invalid for VL %d\n",
				mtu_enc, vl_num);

	return mtu;
}

/*
 * OPA port physical states
 * Returned by the ibphys_portstate() routine.
 */
enum opa_port_phys_state {
	IB_PORTPHYSSTATE_NOP = 0,
	/* 1 is reserved */
	IB_PORTPHYSSTATE_POLLING = 2,
	IB_PORTPHYSSTATE_DISABLED = 3,
	IB_PORTPHYSSTATE_TRAINING = 4,
	IB_PORTPHYSSTATE_LINKUP = 5,
	IB_PORTPHYSSTATE_LINK_ERROR_RECOVERY = 6,
	IB_PORTPHYSSTATE_PHY_TEST = 7,
	/* 8 is reserved */
	OPA_PORTPHYSSTATE_OFFLINE = 9,
	OPA_PORTPHYSSTATE_GANGED = 10,
	OPA_PORTPHYSSTATE_TEST = 11,
	OPA_PORTPHYSSTATE_MAX = 11,
	/* values 12-15 are reserved/ignored */
};

/*
 * States reported by SMA-HFI
 * to SMA-IB
 */
enum opa_sma_status {
	OPA_SMA_SUCCESS = 0,
	OPA_SMA_FAIL_WITH_DATA,
	OPA_SMA_FAIL_WITH_NO_DATA,
};
#endif /* _OPA_CORE_IB_H_ */
