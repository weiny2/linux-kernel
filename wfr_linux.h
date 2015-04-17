#ifndef _WFR_LINUX_H
#define _WFR_LINUX_H
/*
 * Copyright (c) 2014 Intel Corporation.  All rights reserved.
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

/*
 * This header file is for OPA-specific definitions which are
 * required by the WFR driver, and which aren't yet in the linux
 * IB core. We'll collect these all here, then merge them into
 * the kernel when that's convenient.
 */

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

/* OPA status codes */
#define OPA_PM_STATUS_REQUEST_TOO_LARGE		cpu_to_be16(0x100)

static inline u8 port_states_to_logical_state(struct opa_port_states *ps)
{
	return ps->portphysstate_portstate & OPA_PI_MASK_PORT_STATE;
}

static inline u8 port_states_to_phys_state(struct opa_port_states *ps)
{
	return ((ps->portphysstate_portstate &
		  OPA_PI_MASK_PORT_PHYSICAL_STATE) >> 4) & 0xf;
}

/*
 * OPA port physical states
 * IB Volume 1, Table 146 PortInfo/IB Volume 2 Section 5.4.2(1) PortPhysState
 * values.
 *
 * When writing, only values 0-3 are valid, other values are ignored.
 * When reading, 0 is reserved.
 *
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

/* where best to put this opcode? */
#define CNP_OPCODE 0x80

/* OPA_PORT_TYPE_* definitions - these belong in opa_port_info.h */
#define OPA_PORT_TYPE_UNKNOWN          0
#define OPA_PORT_TYPE_DISCONNECTED     1
/* port is not currently usable, CableInfo not available */
#define OPA_PORT_TYPE_FIXED            2
/* A fixed backplane port in a director class switch. All OPA ASICS */
#define OPA_PORT_TYPE_VARIABLE         3
/* A backplane port in a blade system, possibly mixed configuration */
#define OPA_PORT_TYPE_STANDARD         4
/* implies a SFF-8636 defined format for CableInfo (QSFP) */
#define OPA_PORT_TYPE_SI_PHOTONICS      5
/* A silicon photonics module implies TBD defined format for CableInfo
 * as defined by Intel SFO group */
/* 6 - 15 are reserved */

#endif /* _WFR_LINUX_H */
