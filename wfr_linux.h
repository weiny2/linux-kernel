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
 * This header file is for STL-specific definitions which are
 * required by the WFR driver, and which aren't yet in the linux
 * IB core. We'll collect these all here, then merge them into
 * the kernel when that's convenient.
 */

/* STL SMA attribute IDs */
#define STL_ATTRIB_ID_CONGESTION_INFO		cpu_to_be16(0x008b)
#define STL_ATTRIB_ID_HFI_CONGESTION_SETTING	cpu_to_be16(0x0090)
#define STL_ATTRIB_ID_CONGESTION_CONTROL_TABLE	cpu_to_be16(0x0091)

/* STL PMA attribute IDs */
#define STL_PM_ATTRIB_ID_PORT_STATUS		cpu_to_be16(0x0040)
#define STL_PM_ATTRIB_ID_CLEAR_PORT_STATUS	cpu_to_be16(0x0041)
#define STL_PM_ATTRIB_ID_DATA_PORT_COUNTERS	cpu_to_be16(0x0042)
#define STL_PM_ATTRIB_ID_ERROR_PORT_COUNTERS	cpu_to_be16(0x0043)
#define STL_PM_ATTRIB_ID_ERROR_INFO		cpu_to_be16(0x0044)

/* STL status codes */
#define STL_PM_STATUS_REQUEST_TOO_LARGE		cpu_to_be16(0x100)

/*
 * 'struct stl_port_states' is a part of SMA query results for
 * IB_SMP_ATTR_PORT_INFO, and STL_ATTRIB_ID_PORT_STATE_INFO. For now
 * the structure is defined within 'struct stl_port_info', but it
 * should be defined independently so that it can be reused for these
 * two queries.
 */
struct stl_port_states {
	u8 reserved;
	u8 offline_reason;            /* 4 res,  4 bits */
	u8 unsleepstate_downdefstate; /* 4 bits, 4 bits */
	u8 portphysstate_portstate;   /* 4 bits, 4 bits */
};

static inline u8 port_states_to_logical_state(struct stl_port_states *ps)
{
	return ps->portphysstate_portstate & STL_PI_MASK_PORT_STATE;
}

static inline u8 port_states_to_phys_state(struct stl_port_states *ps)
{
	return ((ps->portphysstate_portstate &
		  STL_PI_MASK_PORT_PHYSICAL_STATE) >> 4) & 0xf;
}

/* missing from stl_port_info.h:enum port_info_field_masks */
enum missing_port_info_field_masks {
	/* rename field to port_states.neighbor_normal_offline_reason? */
	/* port_states.offline_reason */
	STL_PI_MASK_NEIGHBOR_NORMAL = 0x10,
	STL_PI_MASK_IS_SM_CONFIG_STARTED = 0x20
};

/* where best to put this opcode? */
#define CNP_OPCODE 0x80

#endif /* _WFR_LINUX_H */
