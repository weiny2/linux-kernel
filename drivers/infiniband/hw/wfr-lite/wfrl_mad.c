/*
 * Copyright (c) 2012 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2005, 2006 PathScale, Inc. All rights reserved.
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

#include <linux/module.h>
#include <linux/printk.h>
#include <rdma/ib_smi.h>
#include <rdma/stl_smi.h>

#include "wfrl.h"
#include "wfrl_mad.h"
#include "wfrl_pma.h"

static ushort wfr_allow_ib_mads = 1;
module_param_named(allow_ib_mads, wfr_allow_ib_mads, ushort, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(allow_ib_mads, "If 1 driver will allow IB MAD's to be processed (default == 1)");

static ushort wfr_warn_ib_mads = 0;
module_param_named(warn_ib_mads, wfr_warn_ib_mads, ushort, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(warn_ib_mads, "If 1 driver will print a warning for all IB MAD's processed (default == 0)");

static unsigned wfr_dump_sma_mads = 0;
module_param_named(dump_sma_mads, wfr_dump_sma_mads, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(dump_sma_mads, "Dump all SMA MAD's to the console");

static unsigned wfr_sma_debug = 0;
module_param_named(sma_debug, wfr_sma_debug, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(sma_debug, "Print SMA debug to the console");

static unsigned wfr_fake_mtu = 0;
module_param_named(fake_mtu, wfr_fake_mtu, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(fake_mtu, "Set a fake MTU value to show in STLPortInfo");

static unsigned wfr_initial_vlflow_disable_mask = 0x00008000; // Default to VL15 disabled (bit 15)
module_param_named(initial_vlflow_disable_mask, wfr_initial_vlflow_disable_mask, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(initial_vlflow_disable_mask, "Set initial wfr_initial_vlflow_disable_mask for port initialization");

static unsigned wfr_strict_am_processing = 1;
module_param_named(strict_am_processing, wfr_strict_am_processing, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(strict_am_processing, "Default == on ; enables Attribute Modifier checking "
					"according to the STL spec; "
					"off == less restrictive 'IB' like processing");

static unsigned wfr_replay_depth_buffer = 128;
module_param_named(replay_depth_buffer, wfr_replay_depth_buffer, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(replay_depth_buffer, "Set a fake ReplayDepth.BufferDepth (256 max) value to show in STLPortInfo");

static unsigned wfr_replay_depth_wire = 64;
module_param_named(replay_depth_wire, wfr_replay_depth_wire, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(replay_depth_wire, "Set a fake ReplayDepth.WireDepth (256 max) value to show in STLPortInfo");

static unsigned wfr_buffer_unit_buf_alloc = 3;
module_param_named(buffer_unit_buf_alloc, wfr_buffer_unit_buf_alloc, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(buffer_unit_buf_alloc, "Set a fake BufferUnit.BufferAlloc (8 max) value to show in STLPortInfo");

static unsigned wfr_buffer_unit_credit_ack = 0;
module_param_named(buffer_unit_credit_ack, wfr_buffer_unit_credit_ack, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(buffer_unit_credit_ack, "Set a fake BufferUnit.CreditAck (8 max) value to show in STLPortInfo");

static unsigned wfr_overall_buffer_space = 144 * 1024; /* 144K */
module_param_named(overall_buffer_space, wfr_overall_buffer_space, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(overall_buffer_space, "Set a fake PortInfo.OverallBufferSpace (IN BYTES); actual value in PortInfo _will_ be adjusted based on buffer_unit_buf_alloc");

static unsigned wfr_shared_space_sup = 1;
module_param_named(shared_space_sup, wfr_shared_space_sup, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(shared_space_sup, "Set if shared space is supported");

static unsigned wfr_mgmt_allowed = 0;
module_param_named(mgmt_allowed, wfr_mgmt_allowed, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(mgmt_allowed, "Set MgmtAllowed bit in simulated LNI negotiations (default 1): "
			"NOTE this results in _REMOTE_ nodes PortInfo.MgmtAllowed bit being set");

static unsigned wfr_enforce_mgmt_pkey = 0;
module_param_named(enforce_mgmt_pkey, wfr_enforce_mgmt_pkey, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(enforce_mgmt_pkey, "Enforce Management Pkey checks on inbound UD QP processing"
		"(default 0); if 1 then P_Key violation traps are also sent");


/** =========================================================================
 * For STL simulation environment we fake some STL values
 */

/**
 * FAKE attribute ID's to comunicate various things across the link
 */
#define IB_SMP_ATTR_WFR_LITE_VIRT_LINK_INFO	cpu_to_be16(0xFFFF)


#define MAX_SIM_STL_PKEY_BLOCKS ((u16)40)
static unsigned wfr_sim_pkey_tbl_block_size = 0;
module_param_named(sim_pkey_tbl_block_size, wfr_sim_pkey_tbl_block_size, uint, S_IRUGO | S_IWUSR | S_IWGRP);
MODULE_PARM_DESC(sim_pkey_tbl_block_size, "When != 0 simulate a large pkey table up to 40 blocks big\n"
			"WARNING: it is possible but unadvisable to change this while running management software");

#define NUM_VIRT_PORTS 2
#define IB_SMP_ATTR_PORT_INFO_STL_RESET		cpu_to_be16(0xFF15)
#define STL_NUM_PKEYS (MAX_SIM_STL_PKEY_BLOCKS * STL_PARTITION_TABLE_BLK_SIZE)
#define STL_NUM_PKEY_BLOCKS_PER_SMP (STL_SMP_DR_DATA_SIZE \
					/ (STL_PARTITION_TABLE_BLK_SIZE * sizeof(u16)))

struct stl_vlarb_entry
{
	u8 vl; /* 3bits reserved, 5 bits VL */
	u8 weight;
};

#define STL_MAX_PREEMPT_CAP         32
#define STL_VLARB_LOW_ELEMENTS       0
#define STL_VLARB_HIGH_ELEMENTS      1
#define STL_VLARB_PREEMPT_ELEMENTS   2
#define STL_VLARB_PREEMPT_MATRIX     3
struct stl_vlarb_data
{
	struct stl_vlarb_entry low_pri_table[128];
	struct stl_vlarb_entry high_pri_table[128];
	struct stl_vlarb_entry preempting_table[STL_MAX_PREEMPT_CAP];
	__be32 preemption_matrix[STL_MAX_VLS];
};

/* stored in network order to be able to pass right back to the user. */
struct stl_buffer_control_table
{
	__be16 tx_overall_shared_limit;
	struct {
		__be16 tx_dedicated_limit;
		__be16 tx_shared_limit;
	} vl[STL_MAX_VLS];
};

/* simulate link init data exchange */
struct wfr_lite_link_init_data
{
	__be64 sender_guid;
	u8 sender_port_mode;
};
static int reply(struct ib_smp *smp); /* fwd declare */

static struct {
	struct stl_port_info port_info;
	u16 pkeys[STL_NUM_PKEYS];
	u8 sl_to_sc[STL_MAX_SLS];
	u8 sc_to_sl[STL_MAX_SCS];
	u8 sc_to_vlt[STL_MAX_SCS];
	u8 sc_to_vlnt[STL_MAX_SCS];
	struct stl_vlarb_data vlarb_data;
	struct stl_buffer_control_table buffer_control_table;
	int member_full_mgmt; /* flag if this port is a member of the full management partition */
} virtual_stl[NUM_VIRT_PORTS];

int virtual_stl_init = 0;

static void linkup_default_mtu(u8 port)
{
	unsigned i;
	//uint8_t mtu_set;
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;

	vpi->mtucap = IB_MTU_2048;
	if (wfr_fake_mtu > 2048) {
		vpi->mtucap = IB_MTU_4096;
	}
	if (wfr_fake_mtu > 4096) {
		vpi->mtucap = STL_MTU_8192;
	}
	if (wfr_fake_mtu > 8192) {
		vpi->mtucap = STL_MTU_10240;
	}

	for (i=0; i< ARRAY_SIZE(vpi->neigh_mtu.pvlx_to_mtu); i++) {
		vpi->neigh_mtu.pvlx_to_mtu[i] = (IB_MTU_2048 << 4) | IB_MTU_2048;
#if 0
		if (i == 7) { /* VL15 is always 2048 */
			vpi->neigh_mtu.pvlx_to_mtu[i] = (mtu_set << 4) | IB_MTU_2048;
		} else {
			vpi->neigh_mtu.pvlx_to_mtu[i] = (mtu_set << 4) | mtu_set;
		}
#endif
	}
}

static u16 get_bytes_per_AU(void)
{
	return (8 * (1 << wfr_buffer_unit_buf_alloc));
}

/**
 * We give VL15 2 * 2048 bufferspace
 * @return vl15_init value in "AU" units
 */
static u16 get_vl15_init(void)
{
	return ((2048 * 2) / get_bytes_per_AU());
}

static void linkup_default(u8 port)
{
	u32 buffer_units;
	u16 vl15_init = get_vl15_init();
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;

	linkup_default_mtu(port);

	vpi->replay_depth.buffer = (u8)(wfr_replay_depth_buffer);
	vpi->replay_depth.wire = (u8)(wfr_replay_depth_wire);

	buffer_units  = (wfr_buffer_unit_buf_alloc) & STL_PI_MASK_BUF_UNIT_BUF_ALLOC;
	buffer_units |= (wfr_buffer_unit_credit_ack << 3) & STL_PI_MASK_BUF_UNIT_CREDIT_ACK;
	buffer_units |= (vl15_init << 11) & STL_PI_MASK_BUF_UNIT_VL15_INIT;
	vpi->buffer_units = cpu_to_be32(buffer_units);

	vpi->flow_control_mask = cpu_to_be32(wfr_initial_vlflow_disable_mask);

	vpi->overall_buffer_space = cpu_to_be16(wfr_overall_buffer_space / get_bytes_per_AU());
	vpi->port_phys_conf = STL_PORT_PHYS_CONF_STANDARD;

	if (wfr_shared_space_sup) {
		vpi->stl_cap_mask |= cpu_to_be16(STL_CAP_MASK3_IsSharedSpaceSupported);
	} else {
		vpi->stl_cap_mask &= ~cpu_to_be16(STL_CAP_MASK3_IsSharedSpaceSupported);
	}
}

/*
 * This function is called when the virtual port is going into Init/LinkUp
 * Link Up Defaults should be set here.
 */
void virtual_port_linkup_init(u8 port)
{
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;
	u16 speed_enabled = be16_to_cpu(vpi->link_speed.enabled);
	u16 width_enabled = be16_to_cpu(vpi->link_width.enabled);
	struct stl_buffer_control_table *vbct = &virtual_stl[port-1].buffer_control_table;

	vpi->port_states.portphysstate_portstate = 5 << 4;
	vpi->port_states.portphysstate_portstate |= 2;

	/* go to the fastest speed enabled. */
	if (STL_LINK_SPEED_25G & speed_enabled)
		vpi->link_speed.active = cpu_to_be16(STL_LINK_SPEED_25G);
	else
		vpi->link_speed.active = cpu_to_be16(STL_LINK_SPEED_12_5G);

	/* go to the widest width enabled. */
	if (IB_WIDTH_4X & width_enabled)
		vpi->link_width.active = cpu_to_be16(IB_WIDTH_4X);
	else {
		if (!(IB_WIDTH_1X & vpi->link_width_downgrade.enabled)) {
			printk(KERN_WARNING PFX
				"STL Port link width going to 1X but "
				"LinkWidthDowgrade.Enable != 1X "
				"port should drop link but SusieQ does "
				"not support this; letting link go to 1X\n");
		}
		vpi->link_width.active = cpu_to_be16(IB_WIDTH_1X);
	}

	vpi->link_width_downgrade.active = cpu_to_be16(IB_WIDTH_4X);

	vbct->vl[15].tx_dedicated_limit = cpu_to_be16(get_vl15_init());

	printk(KERN_WARNING PFX
		"STL Port '%d' Virtual Link Up state : 0x%x\n"
		"              speed en 0x%x; sup 0x%x; act 0x%x\n"
		"              width en 0x%x; sup 0x%x; act 0x%x\n",
		port,
		vpi->port_states.portphysstate_portstate,
		speed_enabled, be16_to_cpu(vpi->link_speed.supported),
		be16_to_cpu(vpi->link_speed.active),
		width_enabled, be16_to_cpu(vpi->link_width.supported),
		be16_to_cpu(vpi->link_width.active));

	linkup_default(port);
}

#define WFR_LITE_LINK_SPEED_ALL_SUP (cpu_to_be16(STL_LINK_SPEED_12_5G | \
						STL_LINK_SPEED_25G))
#define WFR_LITE_LINK_WIDTH_ALL_SUP (cpu_to_be16(IB_WIDTH_1X | IB_WIDTH_4X))

static void init_virtual_port_info(u8 port)
{
	unsigned i;
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;
	struct stl_vlarb_data *vlarb_data = &virtual_stl[port-1].vlarb_data;

	virtual_port_linkup_init(port);
	vpi->link_speed.supported = WFR_LITE_LINK_SPEED_ALL_SUP;
	vpi->link_speed.active = cpu_to_be16(STL_LINK_SPEED_25G);
	vpi->link_speed.enabled = cpu_to_be16(STL_LINK_SPEED_ALL_SUPPORTED);

	vpi->link_width.supported = WFR_LITE_LINK_WIDTH_ALL_SUP;
	vpi->link_width.active = cpu_to_be16(IB_WIDTH_4X);
	vpi->link_width.enabled = cpu_to_be16(STL_LINK_WIDTH_ALL_SUPPORTED);

	vpi->link_width_downgrade.supported = WFR_LITE_LINK_WIDTH_ALL_SUP;
	vpi->link_width_downgrade.active = cpu_to_be16(IB_WIDTH_4X);
	vpi->link_width_downgrade.enabled = cpu_to_be16(STL_LINK_WIDTH_ALL_SUPPORTED);

	linkup_default(port);

	/* VLARB */
	for (i = 0; i < qib_num_cfg_vls; i++) {
		vlarb_data->low_pri_table[i].vl = i & 0x1f;
		vlarb_data->low_pri_table[i].weight = 1;
	}
	for (i = 0; i < ARRAY_SIZE(vlarb_data->high_pri_table); i++) {
		vlarb_data->high_pri_table[i].vl = 15;
		vlarb_data->high_pri_table[i].weight = 0;
	}
	for (i = 0; i < ARRAY_SIZE(vlarb_data->preempting_table); i++) {
		vlarb_data->preempting_table[i].vl = 15;
		vlarb_data->preempting_table[i].weight = 0;
	}
	memset(vlarb_data->preemption_matrix, 0,
	       sizeof(vlarb_data->preemption_matrix));
}

static void reset_virtual_port(u8 port)
{
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;

	virtual_port_linkup_init(port);
	vpi->link_speed.active = cpu_to_be16(STL_LINK_SPEED_25G);

	printk(KERN_WARNING PFX
		"STL Port '%d' Virtual State reset : 0x%x\n",
		port,
		virtual_stl[port-1].port_info.port_states.portphysstate_portstate);
}

static inline void set_virtual_port_state(u8 port, u8 state)
{
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;
	uint8_t tmp = vpi->port_states.portphysstate_portstate;

	vpi->port_states.portphysstate_portstate =
			(tmp & STL_PI_MASK_PORT_PHYSICAL_STATE) |
			(state & STL_PI_MASK_PORT_STATE);
}

static void arm_virtual_port(u8 port)
{
	set_virtual_port_state(port, IB_PORT_ARMED);
	printk(KERN_WARNING PFX "Virtual Port %d Armed\n", port);
}

static void activate_virtual_port(u8 port)
{
	set_virtual_port_state(port, IB_PORT_ACTIVE);
	printk(KERN_WARNING PFX "Virtual Port %d Activated\n", port);
}

static void set_virtual_mtu(u8 port, struct stl_port_info *pi)
{
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;

	memcpy(vpi->neigh_mtu.pvlx_to_mtu, pi->neigh_mtu.pvlx_to_mtu,
		ARRAY_SIZE(vpi->neigh_mtu.pvlx_to_mtu));
}

static int get_pkeys(struct qib_devdata *dd, u8 port, u16 *pkeys);
static void init_virtual_stl(struct ib_device *ibdev, u8 port)
{
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	u8 i;
	/* We have not been called yet
	 * Fake Initial STL data
	 */
	for (i = 0; i < NUM_VIRT_PORTS; i++) {
		memset(&virtual_stl[i], 0, sizeof(virtual_stl[i]));
		get_pkeys(dd, port, virtual_stl[port-1].pkeys);
		init_virtual_port_info(i+1);
	}
	virtual_stl[0].pkeys[STL_NUM_PKEYS-1] = 0xDEAD;
	virtual_stl[1].pkeys[STL_NUM_PKEYS-1] = 0xBEEF;
	for (i = 0; i < STL_MAX_SLS; i++) {
		virtual_stl[0].sl_to_sc[i] = i;
		virtual_stl[1].sl_to_sc[i] = i;
	}
	for (i = 0; i < STL_MAX_SCS; i++) {
		virtual_stl[0].sc_to_sl[i] = i;
		virtual_stl[1].sc_to_sl[i] = i;
	}
	for (i = 0; i < ARRAY_SIZE(virtual_stl[0].sc_to_vlt); i++) {
		virtual_stl[0].sc_to_vlt[i] = i;
		virtual_stl[1].sc_to_vlt[i] = i;
	}
	for (i = 0; i < ARRAY_SIZE(virtual_stl[0].sc_to_vlnt); i++) {
		virtual_stl[0].sc_to_vlnt[i] = i;
		virtual_stl[1].sc_to_vlnt[i] = i;
	}
	virtual_stl_init = 1;
}

/* allocate an IB DR send mad for control messages */
static struct ib_mad_send_buf *wfr_create_ib_dr_send_mad(struct ib_device *ibdev, u8 port)
{
	struct ib_mad_send_buf *send_buf;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct ib_mad_agent *agent;
	struct ib_ah *ah;
	unsigned long flags;
	int pkey_idx;

	agent = ibp->send_agent;
	if (!agent)
		return ERR_PTR(-EINVAL);

	/* o14-3.2.1 */
	if (!(ppd_from_ibp(ibp)->lflags & QIBL_LINKACTIVE))
		return ERR_PTR(-EINVAL);

	spin_lock_irqsave(&ibp->lock, flags);
	if (!ibp->dr_ah) {
		ibp->dr_ah = qib_create_qp0_ah(ibp, IB_LID_PERMISSIVE);
		if (IS_ERR(ibp->dr_ah)) {
			ibp->dr_ah = NULL;
			return ERR_PTR(-ENOMEM);
		}
	}
	ah = ibp->dr_ah;
	spin_unlock_irqrestore(&ibp->lock, flags);

	pkey_idx = wfr_lookup_pkey_idx(ibp, WFR_LIM_MGMT_P_KEY);
	if (pkey_idx < 0) {
		printk(KERN_ERR PFX
			"ERROR: WFR create IB ctrl msg failed to find 0x%x "
			"pkey index\n",
			WFR_LIM_MGMT_P_KEY);
		pkey_idx = 1;
	}
	send_buf = ib_create_send_mad(agent, 0, 0, 0, IB_MGMT_MAD_HDR,
				      IB_MGMT_MAD_DATA, GFP_ATOMIC);
	if (IS_ERR(send_buf))
		goto err;

	send_buf->ah = ah;

err:
	return send_buf;
}


/* simulate link changes/negotiation */
static int subn_set_stl_virt_link_info(struct ib_smp *smp, struct ib_device *ibdev,
			   u8 port, int send_GetResp)
{
	u8 rem_phys_state, rem_link_state;
	u8 loc_phys_state, loc_link_state, new_loc_phys_state;
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	struct wfr_link_info *link_info =
					(struct wfr_link_info *)smp->data;
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;
	struct stl_buffer_control_table *vbct = &virtual_stl[port-1].buffer_control_table;

	rem_phys_state = (link_info->port_state >> 4) & 0xF;
	rem_link_state = link_info->port_state & 0xF;

	loc_phys_state = (vpi->port_states.portphysstate_portstate >> 4) & 0xF;
	loc_link_state =  vpi->port_states.portphysstate_portstate & 0xF;

	printk(KERN_WARNING PFX "STL Link State p%d: remote state 0x%02x; local state 0x%02x\n",
		port,
		link_info->port_state,
		vpi->port_states.portphysstate_portstate);

	if (loc_phys_state == 0x02 /* Polling */
	    && (rem_phys_state == 0x02 /* Polling */
	       || rem_phys_state == 0x05 /* LinkUp */)) {
		virtual_port_linkup_init(port);
	}

	if (rem_link_state == IB_PORT_DOWN) {
		/* Set local port to Down... */
		vpi->port_states.portphysstate_portstate = 1;
		if (loc_phys_state == 0x3) {
			/* were disabled stay there */
			vpi->port_states.portphysstate_portstate |= 0x3 << 4;
		} else {
			if (rem_phys_state == 0x03 /* remote Down/Disabled */
			    || rem_phys_state == 0x01) { /* remote Down/Sleep */
				/* ... Polling */
				vpi->port_states.portphysstate_portstate |= 2 << 4;
				printk(KERN_WARNING PFX
					"STL local Virt Port '%d' : down/polling\n", port);
			} else {
				virtual_port_linkup_init(port);
			}
		}
		vpi->neigh_node_guid = 0;
		vpi->port_neigh_mode = 0;
	} else {
		vpi->neigh_node_guid = link_info->node_guid;
		vpi->port_neigh_mode = link_info->port_mode;
		if (vpi->port_neigh_mode & STL_PI_MASK_NEIGH_MGMT_ALLOWED) {
			struct qib_pportdata *ppd = dd->pport + port - 1;
			struct qib_ctxtdata *rcd = dd->rcd[ppd->hw_pidx];
			rcd->pkeys[2] = 0xffff;
		}
	}

// FIXME at some point we should tell users about the state change
//int qib_set_uevent_bits(struct qib_pportdata *ppd, const int evtbit)
//qib_set_uevent_bits(ppd, _QIB_EVENT_LINKDOWN_BIT);
//ev = IB_EVENT_PORT_ACTIVE;
//if (ev)
//signal_ib_event(ppd, ev);

	new_loc_phys_state = (vpi->port_states.portphysstate_portstate >> 4) & 0xF;
	if (new_loc_phys_state == 0x5) { /* LinkUp */
		vpi->link_width.active = link_info->link_width_active;
		vpi->link_speed.active = link_info->link_speed_active;
		if (loc_phys_state < 0x5)
			vbct->vl[15].tx_dedicated_limit = link_info->vl15_init;
	}

	if (send_GetResp) {
		/* send our response back */
		link_info->node_guid = dd->pport->guid;

		if (wfr_mgmt_allowed)
			link_info->port_mode = STL_PI_MASK_NEIGH_MGMT_ALLOWED;
		else
			link_info->port_mode = 0;

		link_info->port_state = virtual_stl[port-1].port_info.port_states.portphysstate_portstate;
		link_info->vl15_init = cpu_to_be16(get_vl15_init());

		return reply(smp);
	}

	return IB_MAD_RESULT_CONSUMED;
}

static void wfr_send_stl_virt_link_info(struct ib_device *ibdev, u8 port, uint8_t state,
					__be64 local_node_guid, u8 local_port_mode,
					__be16 link_speed_active,
					__be16 link_width_active)
{
	struct ib_mad_send_buf *send_buf;
	struct ib_smp *smp;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct wfr_link_info *link_info;

	send_buf = wfr_create_ib_dr_send_mad(ibdev, port);
	if (IS_ERR(send_buf)) {
		printk(KERN_WARNING PFX "STL link state send FAILED: DR AH\n");
		return;
	}

	smp = send_buf->mad;
	smp->base_version = IB_MGMT_BASE_VERSION;
	smp->mgmt_class = IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE;
	smp->class_version = 1;
	smp->method = IB_MGMT_METHOD_SET;
	smp->hop_ptr = 0;
	smp->hop_cnt = 1;
	ibp->tid++;
	smp->tid = cpu_to_be64(ibp->tid);
	smp->attr_id = IB_SMP_ATTR_WFR_LITE_VIRT_LINK_INFO;
	smp->attr_mod = 0;
	smp->mkey = 0;
	smp->dr_slid = IB_LID_PERMISSIVE;
	smp->dr_dlid = IB_LID_PERMISSIVE;
	smp->initial_path[0] = 0;
	smp->initial_path[1] = port;

	link_info = (struct wfr_link_info *)smp->data;
	link_info->node_guid = local_node_guid;
	link_info->port_mode = local_port_mode;
	link_info->port_state = state;
	link_info->link_speed_active = link_speed_active;
	link_info->link_width_active = link_width_active;
	link_info->vl15_init = cpu_to_be16(get_vl15_init());

	if (ib_post_send_mad(send_buf, NULL)) {
		printk(KERN_WARNING PFX "STL link state send FAILED: ib_post_send_mad\n");
		ib_free_send_mad(send_buf);
	}

	printk(KERN_WARNING PFX "STL port '%d' state send: state 0x%02x\n",
		port,
		link_info->port_state);
}

/* This is called when the IB port goes active to send the initial link_info
 *
 * Effectively the virtual STL link is "negotiated" when the IB port is made
 * active.  This allows the STL Link to "pass traffic"
 */
static void wfr_send_stl_link_info(struct ib_device *ibdev, u8 port)
{
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;
	u8 state = vpi->port_states.portphysstate_portstate;
	__be64 local_node_guid = dd->pport->guid;

	wfr_send_stl_virt_link_info(ibdev, port, state, local_node_guid,
					wfr_mgmt_allowed ?
						STL_PI_MASK_NEIGH_MGMT_ALLOWED : 0,
					vpi->link_speed.active,
					vpi->link_width.active);
}

static u8 wfrl_get_stl_virtual_port_state(u8 port)
{
	return (virtual_stl[port-1].port_info.port_states.portphysstate_portstate);
}

/* These communicate to/from the snoop port (AKA simulator) */
void wfrl_get_stl_virtual_link_info(u8 port, struct wfr_link_info *link_info)
{
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;

	link_info->port_state = vpi->port_states.portphysstate_portstate;
	link_info->node_guid = be64_to_cpu(vpi->neigh_node_guid);
	link_info->port_mode = vpi->port_neigh_mode;
	link_info->link_speed_active = be16_to_cpu(vpi->link_speed.active);
	link_info->link_width_active = be16_to_cpu(vpi->link_width.active);
}
void wfrl_set_stl_virtual_link_info(struct ib_device *ibdev, u8 port, uint8_t state,
					__be64 local_node_guid, u8 local_port_mode)
{
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;
	vpi->port_states.portphysstate_portstate = state;
	wfr_send_stl_virt_link_info(ibdev, port, state, local_node_guid, local_port_mode,
					vpi->link_speed.active,
					vpi->link_width.active);
}

u16 wfrl_get_num_sim_pkey_blocks(void)
{
	return min_t(u16, wfr_sim_pkey_tbl_block_size, MAX_SIM_STL_PKEY_BLOCKS);
}

/**
 * End STL simulation values
 * ========================================================================= */

static int reply(struct ib_smp *smp)
{
	/*
	 * The verbs framework will handle the directed/LID route
	 * packet changes.
	 */
	smp->method = IB_MGMT_METHOD_GET_RESP;
	if (smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
		smp->status |= IB_SMP_DIRECTION;
	return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_REPLY;
}
static int reply_stl(struct stl_smp *smp)
{
	/*
	 * The verbs framework will handle the directed/LID route
	 * packet changes.
	 */
	smp->method = IB_MGMT_METHOD_GET_RESP;
	if (smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
		smp->status |= IB_SMP_DIRECTION;
	return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_REPLY;
}

static int reply_failure(struct ib_smp *smp)
{
	/*
	 * The verbs framework will handle the directed/LID route
	 * packet changes.
	 */
	smp->method = IB_MGMT_METHOD_GET_RESP;
	if (smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
		smp->status |= IB_SMP_DIRECTION;
	return IB_MAD_RESULT_FAILURE | IB_MAD_RESULT_REPLY;
}

static void qib_send_trap(struct qib_ibport *ibp, void *data, unsigned len)
{
	struct ib_mad_send_buf *send_buf;
	struct ib_mad_agent *agent;
	struct ib_smp *smp;
	int ret;
	unsigned long flags;
	unsigned long timeout;
	int pkey_idx;

	agent = ibp->send_agent;
	if (!agent)
		return;

	/* o14-3.2.1 */
	if (!(ppd_from_ibp(ibp)->lflags & QIBL_LINKACTIVE))
		return;

	/* o14-2 */
	if (ibp->trap_timeout && time_before(jiffies, ibp->trap_timeout))
		return;

	pkey_idx = wfr_lookup_pkey_idx(ibp, WFR_LIM_MGMT_P_KEY);
	if (pkey_idx < 0) {
		printk(KERN_ERR PFX
			"ERROR: Trap failed to find 0x%x pkey index\n",
			WFR_LIM_MGMT_P_KEY);
		pkey_idx = 1;
	}
	send_buf = ib_create_send_mad(agent, 0, pkey_idx, 0, IB_MGMT_MAD_HDR,
				      IB_MGMT_MAD_DATA, GFP_ATOMIC);
	if (IS_ERR(send_buf))
		return;

	smp = send_buf->mad;
	smp->base_version = IB_MGMT_BASE_VERSION;
	smp->mgmt_class = IB_MGMT_CLASS_SUBN_LID_ROUTED;
	smp->class_version = 1;
	smp->method = IB_MGMT_METHOD_TRAP;
	ibp->tid++;
	smp->tid = cpu_to_be64(ibp->tid);
	smp->attr_id = IB_SMP_ATTR_NOTICE;
	/* o14-1: smp->mkey = 0; */
	memcpy(smp->data, data, len);

	spin_lock_irqsave(&ibp->lock, flags);
	if (!ibp->sm_ah) {
		if (ibp->sm_lid != be16_to_cpu(IB_LID_PERMISSIVE)) {
			struct ib_ah *ah;

			ah = qib_create_qp0_ah(ibp, ibp->sm_lid);
			if (IS_ERR(ah))
				ret = PTR_ERR(ah);
			else {
				send_buf->ah = ah;
				ibp->sm_ah = to_iah(ah);
				ret = 0;
			}
		} else
			ret = -EINVAL;
	} else {
		send_buf->ah = &ibp->sm_ah->ibah;
		ret = 0;
	}
	spin_unlock_irqrestore(&ibp->lock, flags);

	if (!ret)
		ret = ib_post_send_mad(send_buf, NULL);
	if (!ret) {
		/* 4.096 usec. */
		timeout = (4096 * (1UL << ibp->subnet_timeout)) / 1000;
		ibp->trap_timeout = jiffies + usecs_to_jiffies(timeout);
	} else {
		ib_free_send_mad(send_buf);
		ibp->trap_timeout = 0;
	}
}

/*
 * Send a bad [PQ]_Key trap (ch. 14.3.8).
 */
void qib_bad_pqkey(struct qib_ibport *ibp, __be16 trap_num, u32 key, u32 sl,
		   u32 qp1, u32 qp2, __be16 lid1, __be16 lid2)
{
	struct ib_mad_notice_attr data;

	if (trap_num == IB_NOTICE_TRAP_BAD_PKEY)
		ibp->pkey_violations++;
	else
		ibp->qkey_violations++;
	ibp->n_pkt_drops++;

	/* Send violation trap */
	data.generic_type = IB_NOTICE_TYPE_SECURITY;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = trap_num;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof data.details);
	data.details.ntc_257_258.lid1 = lid1;
	data.details.ntc_257_258.lid2 = lid2;
	data.details.ntc_257_258.key = cpu_to_be32(key);
	data.details.ntc_257_258.sl_qp1 = cpu_to_be32((sl << 28) | qp1);
	data.details.ntc_257_258.qp2 = cpu_to_be32(qp2);

	qib_send_trap(ibp, &data, sizeof data);
}

/*
 * Send a bad M_Key trap (ch. 14.3.9).
 */
static void qib_bad_mkey(struct qib_ibport *ibp, struct ib_smp *smp)
{
	struct ib_mad_notice_attr data;

	/* Send violation trap */
	data.generic_type = IB_NOTICE_TYPE_SECURITY;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_BAD_MKEY;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof data.details);
	data.details.ntc_256.lid = data.issuer_lid;
	data.details.ntc_256.method = smp->method;
	data.details.ntc_256.attr_id = smp->attr_id;
	data.details.ntc_256.attr_mod = smp->attr_mod;
	data.details.ntc_256.mkey = smp->mkey;
	if (smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE) {
		u8 hop_cnt;

		data.details.ntc_256.dr_slid = smp->dr_slid;
		data.details.ntc_256.dr_trunc_hop = IB_NOTICE_TRAP_DR_NOTICE;
		hop_cnt = smp->hop_cnt;
		if (hop_cnt > ARRAY_SIZE(data.details.ntc_256.dr_rtn_path)) {
			data.details.ntc_256.dr_trunc_hop |=
				IB_NOTICE_TRAP_DR_TRUNC;
			hop_cnt = ARRAY_SIZE(data.details.ntc_256.dr_rtn_path);
		}
		data.details.ntc_256.dr_trunc_hop |= hop_cnt;
		memcpy(data.details.ntc_256.dr_rtn_path, smp->return_path,
		       hop_cnt);
	}

	qib_send_trap(ibp, &data, sizeof data);
}

/*
 * Send a bad M_Key trap (ch. 14.3.9).
 */
static void wfr_bad_mkey(struct qib_ibport *ibp, struct stl_smp *smp)
{
	struct ib_mad_notice_attr data;

	/* Send violation trap */
	data.generic_type = IB_NOTICE_TYPE_SECURITY;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_BAD_MKEY;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof data.details);
	data.details.ntc_256.lid = data.issuer_lid;
	data.details.ntc_256.method = smp->method;
	data.details.ntc_256.attr_id = smp->attr_id;
	data.details.ntc_256.attr_mod = smp->attr_mod;
	data.details.ntc_256.mkey = smp->mkey;
	if (smp->mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE) {
		u8 hop_cnt;

		data.details.ntc_256.dr_slid = smp->route.dr.dr_slid;
		data.details.ntc_256.dr_trunc_hop = IB_NOTICE_TRAP_DR_NOTICE;
		hop_cnt = smp->hop_cnt;
		if (hop_cnt > ARRAY_SIZE(data.details.ntc_256.dr_rtn_path)) {
			data.details.ntc_256.dr_trunc_hop |=
				IB_NOTICE_TRAP_DR_TRUNC;
			hop_cnt = ARRAY_SIZE(data.details.ntc_256.dr_rtn_path);
		}
		data.details.ntc_256.dr_trunc_hop |= hop_cnt;
		memcpy(data.details.ntc_256.dr_rtn_path, smp->route.dr.return_path,
		       hop_cnt);
	}

	qib_send_trap(ibp, &data, sizeof data);
}

/*
 * Send a Port Capability Mask Changed trap (ch. 14.3.11).
 */
void qib_cap_mask_chg(struct qib_ibport *ibp)
{
	struct ib_mad_notice_attr data;

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_CAP_MASK_CHG;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof data.details);
	data.details.ntc_144.lid = data.issuer_lid;
	data.details.ntc_144.new_cap_mask = cpu_to_be32(ibp->port_cap_flags);

	qib_send_trap(ibp, &data, sizeof data);
}

/*
 * Send a System Image GUID Changed trap (ch. 14.3.12).
 */
void qib_sys_guid_chg(struct qib_ibport *ibp)
{
	struct ib_mad_notice_attr data;

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_SYS_GUID_CHG;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof data.details);
	data.details.ntc_145.lid = data.issuer_lid;
	data.details.ntc_145.new_sys_guid = ib_qib_sys_image_guid;

	qib_send_trap(ibp, &data, sizeof data);
}

/*
 * Send a Node Description Changed trap (ch. 14.3.13).
 */
void qib_node_desc_chg(struct qib_ibport *ibp)
{
	struct ib_mad_notice_attr data;

	data.generic_type = IB_NOTICE_TYPE_INFO;
	data.prod_type_msb = 0;
	data.prod_type_lsb = IB_NOTICE_PROD_CA;
	data.trap_num = IB_NOTICE_TRAP_CAP_MASK_CHG;
	data.issuer_lid = cpu_to_be16(ppd_from_ibp(ibp)->lid);
	data.toggle_count = 0;
	memset(&data.details, 0, sizeof data.details);
	data.details.ntc_144.lid = data.issuer_lid;
	data.details.ntc_144.local_changes = 1;
	data.details.ntc_144.change_flags = IB_NOTICE_TRAP_NODE_DESC_CHG;

	qib_send_trap(ibp, &data, sizeof data);
}

static int subn_get_nodedescription(struct ib_smp *smp,
				    struct ib_device *ibdev)
{
	if (smp->attr_mod)
		smp->status |= IB_SMP_INVALID_FIELD;

	memcpy(smp->data, ibdev->node_desc, sizeof(smp->data));

	return reply(smp);
}

static int subn_get_stl_nodedescription(struct stl_smp *smp, struct ib_device *ibdev)
{
	struct stl_node_description *nd;

	if (smp->attr_mod)
		smp->status |= IB_SMP_INVALID_FIELD;

	nd = (struct stl_node_description *)stl_get_smp_data(smp);

	memcpy(nd->data, ibdev->node_desc, sizeof(nd->data));

	return reply_stl(smp);
}

static int subn_get_nodeinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct ib_node_info *nip = (struct ib_node_info *)&smp->data;
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	u32 vendor, majrev, minrev;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	/* GUID 0 is illegal */
	if (smp->attr_mod || pidx >= dd->num_pports ||
	    dd->pport[pidx].guid == 0)
		smp->status |= IB_SMP_INVALID_FIELD;
	else
		nip->port_guid = dd->pport[pidx].guid;

	if (wfr_vl15_ovl0) {
		nip->base_version = JUMBO_MGMT_BASE_VERSION;
		nip->class_version = STL_SMI_CLASS_VERSION;
	} else {
		nip->base_version = 1;
		nip->class_version = 1;
	}
	nip->node_type = 1;     /* channel adapter */
	nip->num_ports = ibdev->phys_port_cnt;
	/* This is already in network order */
	nip->sys_guid = ib_qib_sys_image_guid;
	nip->node_guid = dd->pport->guid; /* Use first-port GUID as node */
	nip->partition_cap = cpu_to_be16(qib_get_npkeys(dd));
	nip->device_id = cpu_to_be16(dd->deviceid);
	majrev = dd->majrev;
	minrev = dd->minrev;
	nip->revision = cpu_to_be32((majrev << 16) | minrev);
	nip->local_port_num = port;
	vendor = dd->vendorid;
	nip->vendor_id[0] = WFR_SRC_OUI_1;
	nip->vendor_id[1] = WFR_SRC_OUI_2;
	nip->vendor_id[2] = WFR_SRC_OUI_3;

	return reply(smp);
}

static int subn_get_guidinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	u32 startgx = 8 * be32_to_cpu(smp->attr_mod);
	__be64 *p = (__be64 *) smp->data;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	/* 32 blocks of 8 64-bit GUIDs per block */

	memset(smp->data, 0, sizeof(smp->data));

	if (startgx == 0 && pidx < dd->num_pports) {
		struct qib_pportdata *ppd = dd->pport + pidx;
		struct qib_ibport *ibp = &ppd->ibport_data;
		__be64 g = ppd->guid;
		unsigned i;

		/* GUID 0 is illegal */
		if (g == 0)
			smp->status |= IB_SMP_INVALID_FIELD;
		else {
			/* The first is a copy of the read-only HW GUID. */
			p[0] = g;
			for (i = 1; i < QIB_GUIDS_PER_PORT; i++)
				p[i] = ibp->guids[i - 1];
		}
	} else
		smp->status |= IB_SMP_INVALID_FIELD;

	return reply(smp);
}

static void set_link_width_enabled(struct qib_pportdata *ppd, u32 w)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LWID_ENB, w);
}

static void set_link_speed_enabled(struct qib_pportdata *ppd, u32 s)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_SPD_ENB, s);
}

static int get_overrunthreshold(struct qib_pportdata *ppd)
{
	return ppd->dd->f_get_ib_cfg(ppd, QIB_IB_CFG_OVERRUN_THRESH);
}

/**
 * set_overrunthreshold - set the overrun threshold
 * @ppd: the physical port data
 * @n: the new threshold
 *
 * Note that this will only take effect when the link state changes.
 */
static int set_overrunthreshold(struct qib_pportdata *ppd, unsigned n)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_OVERRUN_THRESH,
					 (u32)n);
	return 0;
}

static int get_phyerrthreshold(struct qib_pportdata *ppd)
{
	return ppd->dd->f_get_ib_cfg(ppd, QIB_IB_CFG_PHYERR_THRESH);
}

/**
 * set_phyerrthreshold - set the physical error threshold
 * @ppd: the physical port data
 * @n: the new threshold
 *
 * Note that this will only take effect when the link state changes.
 */
static int set_phyerrthreshold(struct qib_pportdata *ppd, unsigned n)
{
	(void) ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PHYERR_THRESH,
					 (u32)n);
	return 0;
}

/**
 * get_linkdowndefaultstate - get the default linkdown state
 * @ppd: the physical port data
 *
 * Returns zero if the default is POLL, 1 if the default is SLEEP.
 */
static int get_linkdowndefaultstate(struct qib_pportdata *ppd)
{
	return ppd->dd->f_get_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT) ==
		IB_LINKINITCMD_SLEEP;
}

static int check_mkey(struct qib_ibport *ibp, struct ib_smp *smp, int mad_flags)
{
	int valid_mkey = 0;
	int ret = 0;

	/* Is the mkey in the process of expiring? */
	if (ibp->mkey_lease_timeout &&
	    time_after_eq(jiffies, ibp->mkey_lease_timeout)) {
		/* Clear timeout and mkey protection field. */
		ibp->mkey_lease_timeout = 0;
		ibp->mkeyprot = 0;
	}

	if ((mad_flags & IB_MAD_IGNORE_MKEY) ||  ibp->mkey == 0 ||
	    ibp->mkey == smp->mkey)
		valid_mkey = 1;

	/* Unset lease timeout on any valid Get/Set/TrapRepress */
	if (valid_mkey && ibp->mkey_lease_timeout &&
	    (smp->method == IB_MGMT_METHOD_GET ||
	     smp->method == IB_MGMT_METHOD_SET ||
	     smp->method == IB_MGMT_METHOD_TRAP_REPRESS))
		ibp->mkey_lease_timeout = 0;

	if (!valid_mkey) {
		switch (smp->method) {
		case IB_MGMT_METHOD_GET:
			/* Bad mkey not a violation below level 2 */
			if (ibp->mkeyprot < 2)
				break;
		case IB_MGMT_METHOD_SET:
		case IB_MGMT_METHOD_TRAP_REPRESS:
			if (ibp->mkey_violations != 0xFFFF)
				++ibp->mkey_violations;
			if (!ibp->mkey_lease_timeout && ibp->mkey_lease_period)
				ibp->mkey_lease_timeout = jiffies +
					ibp->mkey_lease_period * HZ;
			/* Generate a trap notice. */
			qib_bad_mkey(ibp, smp);
			ret = 1;
		}
	}

	return ret;
}

static int check_stl_mkey(struct qib_ibport *ibp, struct stl_smp *smp, int mad_flags)
{
	int valid_mkey = 0;
	int ret = 0;

	/* Is the mkey in the process of expiring? */
	if (ibp->mkey_lease_timeout &&
	    time_after_eq(jiffies, ibp->mkey_lease_timeout)) {
		/* Clear timeout and mkey protection field. */
		ibp->mkey_lease_timeout = 0;
		ibp->mkeyprot = 0;
	}

	if ((mad_flags & IB_MAD_IGNORE_MKEY) ||  ibp->mkey == 0 ||
	    ibp->mkey == smp->mkey)
		valid_mkey = 1;

	/* Unset lease timeout on any valid Get/Set/TrapRepress */
	if (valid_mkey && ibp->mkey_lease_timeout &&
	    (smp->method == IB_MGMT_METHOD_GET ||
	     smp->method == IB_MGMT_METHOD_SET ||
	     smp->method == IB_MGMT_METHOD_TRAP_REPRESS))
		ibp->mkey_lease_timeout = 0;

	if (!valid_mkey) {
		switch (smp->method) {
		case IB_MGMT_METHOD_GET:
			/* Bad mkey not a violation below level 2 */
			if (ibp->mkeyprot < 2)
				break;
		case IB_MGMT_METHOD_SET:
		case IB_MGMT_METHOD_TRAP_REPRESS:
			if (ibp->mkey_violations != 0xFFFF)
				++ibp->mkey_violations;
			if (!ibp->mkey_lease_timeout && ibp->mkey_lease_period)
				ibp->mkey_lease_timeout = jiffies +
					ibp->mkey_lease_period * HZ;
			/* Generate a trap notice. */
			wfr_bad_mkey(ibp, smp);
			ret = 1;
		}
	}

	return ret;
}

static void dump_mad(uint8_t *mad, size_t size)
{
	int i = 0;
	for (i=0; i<size; i++) {
		if ((i%16) == 0)
			printk("\n%04d:", i);
		else if ((i%8) == 0)
			printk("   %04d:", i);
		printk(" %02x", mad[i]);
	}
}

static int subn_get_stl_nodeinfo(struct stl_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct stl_node_info *ni;
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	u32 vendor, majrev, minrev;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	ni = (struct stl_node_info *)stl_get_smp_data(smp);

	memset(ni, 0, sizeof(*ni));

	/* GUID 0 is illegal */
	if (smp->attr_mod || pidx >= dd->num_pports ||
	    dd->pport[pidx].guid == 0)
		smp->status |= IB_SMP_INVALID_FIELD;
	else
		ni->port_guid = dd->pport[pidx].guid;

	ni->base_version = JUMBO_MGMT_BASE_VERSION;
	ni->class_version = STL_SMI_CLASS_VERSION;
	ni->node_type = 1;     /* channel adapter */
	ni->num_ports = ibdev->phys_port_cnt;
	/* This is already in network order */
	ni->system_image_guid = ib_qib_sys_image_guid;
	ni->node_guid = dd->pport->guid; /* Use first-port GUID as node */
	if (wfr_sim_pkey_tbl_block_size) {
		u16 n_blocks = wfrl_get_num_sim_pkey_blocks();
		ni->partition_cap = cpu_to_be16(n_blocks *
					STL_PARTITION_TABLE_BLK_SIZE);
	} else
		ni->partition_cap = cpu_to_be16(qib_get_npkeys(dd));
	ni->device_id = cpu_to_be16(dd->deviceid);
	majrev = dd->majrev;
	minrev = dd->minrev;
	ni->revision = cpu_to_be32((majrev << 16) | minrev);
	ni->local_port_num = port;
	vendor = dd->vendorid;
	ni->vendor_id[0] = WFR_SRC_OUI_1;
	ni->vendor_id[1] = WFR_SRC_OUI_2;
	ni->vendor_id[2] = WFR_SRC_OUI_3;

	return reply_stl(smp);
}

/* Where the IB hardware has values they are filled in here
 * Other values are simply stored in virtual_stl and echoed
 * back.
 */
static int subn_get_stl_portinfo(struct stl_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	//int i;
	struct qib_devdata *dd;
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;
	struct stl_port_info *pi;
	//u8 mtu;
	int ret;
	//u32 state;
	u32 port_num = be32_to_cpu(smp->attr_mod) & 0xff;
	u32 num_ports = be32_to_cpu(smp->attr_mod) >> 24;
	struct stl_port_info *vpi;

	/* NOTE: the following checks could be optimized.
	 *       but for testing and clarity I wanted to keep them separate. */
	if (wfr_strict_am_processing) {
		if (num_ports != 1) {
			printk(KERN_WARNING PFX "STL Get(STLPortInfo) invalid AM.N %x (%x)\n",
					num_ports, smp->attr_mod);
			smp->status |= IB_SMP_INVALID_FIELD;
			ret = reply_stl(smp);
			goto bail;
		}

		/* PortInfo is only valid on receiving port
		 * or 0 for wildcard
		 */
		if (port_num == 0)
			port_num = port;

		if (port_num != port) {
			printk(KERN_WARNING PFX
				"STL Get(STLPortInfo) invalid AM.P %d; inbound %d\n",
				port_num, port);
			smp->status |= IB_SMP_INVALID_FIELD;
			ret = reply_stl(smp);
			goto bail;
		}
	} else {
		if (num_ports > 1) {
			smp->status |= IB_SMP_INVALID_FIELD;
			ret = reply_stl(smp);
			goto bail;
		}
		if (port_num == 0)
			port_num = port;
		else {
			if (port_num > ibdev->phys_port_cnt) {
				smp->status |= IB_SMP_INVALID_FIELD;
				ret = reply_stl(smp);
				goto bail;
			}
			if (port_num != port) {
				ibp = to_iport(ibdev, port_num);
				ret = check_mkey(ibp, (struct ib_smp*)smp, 0);
				if (ret) {
					ret = IB_MAD_RESULT_FAILURE;
					goto bail;
				}
			}
		}
	}
	vpi = &virtual_stl[port_num-1].port_info;

	dd = dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port_num - 1);
	ibp = &ppd->ibport_data;

	pi = (struct stl_port_info *)stl_get_smp_data(smp);
	/* Clear all fields.  Only set the non-zero fields. */
	memset(pi, 0, sizeof(*pi));

	/* Get virtual values first */
	memcpy(pi, &virtual_stl[port-1].port_info, sizeof(*pi));

	/* Then set some real values to make sure that LID MAD's are processed and
	 * local software has valid data to pull from ie for SA queries and
	 * what not
	 */
	pi->lid = cpu_to_be32(ppd->lid);

	/* Only return the mkey if the protection field allows it. */
	if (!(smp->method == IB_MGMT_METHOD_GET &&
	      ibp->mkey != smp->mkey &&
	      ibp->mkeyprot == 1))
		pi->mkey = ibp->mkey;

	pi->subnet_prefix = ibp->gid_prefix;
	pi->sm_lid = cpu_to_be32(ibp->sm_lid);
	pi->ib_cap_mask = cpu_to_be32(ibp->port_cap_flags);
	/* pi->diag_code; */
	pi->mkey_lease_period = cpu_to_be16(ibp->mkey_lease_period);

	/*
	 * These are picked up from the simulated port info
	pi->link_width.enabled = cpu_to_be16(ppd->link_width_enabled);
	pi->link_width.supported = cpu_to_be16(ppd->link_width_supported);
	pi->link_width.active = cpu_to_be16(ppd->link_width_active);

	pi->link_speed.supported = cpu_to_be16(ppd->link_speed_supported);
	pi->link_speed.active = cpu_to_be16(ppd->link_speed_active);
	pi->link_speed.enabled = cpu_to_be16(ppd->link_speed_enabled);
	*/
	if (wfr_sma_debug)
	{
		printk(KERN_WARNING PFX
			"STL Get Port '%d' speed en 0x%x; sup 0x%x; act 0x%x\n",
			port,
			be16_to_cpu(pi->link_speed.enabled),
			be16_to_cpu(pi->link_speed.supported),
			be16_to_cpu(pi->link_speed.active));
		printk(KERN_WARNING PFX
			"STL Get Port '%d' width en 0x%x; sup 0x%x; act 0x%x\n",
			port,
			be16_to_cpu(pi->link_width.enabled),
			be16_to_cpu(pi->link_width.supported),
			be16_to_cpu(pi->link_width.active));
	}

	/* FIXME make sure that this default state matches */
	pi->port_states.offline_reason = 0;
	pi->port_states.unsleepstate_downdefstate = (get_linkdowndefaultstate(ppd) ? 1 : 2);

	/*
	 * Return virtual Port States in STL mode
	state = dd->f_iblink_state(ppd->lastibcstat);
	pi->port_states.portphysstate_portstate =
		(dd->f_ibphys_portstate(ppd->lastibcstat) << 4) |
		state;
	*/
	pi->port_states.portphysstate_portstate = virtual_stl[port-1].port_info.port_states.portphysstate_portstate;

	//pi->collectivemask_multicastmask = 0;

	pi->mkeyprotect_lmc = (ibp->mkeyprot << 6) | ppd->lmc;

#if 0
	/* return simulated mtu data */
	switch (ppd->ibmtu) {
	default: /* something is wrong; fall through */
	case 4096:
		mtu = IB_MTU_4096;
		break;
	case 2048:
		mtu = IB_MTU_2048;
		break;
	case 1024:
		mtu = IB_MTU_1024;
		break;
	case 512:
		mtu = IB_MTU_512;
		break;
	case 256:
		mtu = IB_MTU_256;
		break;
	}

	memset(pi->neigh_mtu.pvlx_to_mtu, 0, sizeof(pi->neigh_mtu.pvlx_to_mtu));
	for (i = 0; i < ppd->vls_supported; i++)
		if ((i % 2) == 0)
			pi->neigh_mtu.pvlx_to_mtu[i/2] |= (mtu << 4);
		else
			pi->neigh_mtu.pvlx_to_mtu[i/2] |= mtu;
#endif

	pi->smsl = ibp->sm_sl & STL_PI_MASK_SMSL;
	//pi->partenforce_filterraw = 0;
	pi->operational_vls = dd->f_get_ib_cfg(ppd, QIB_IB_CFG_OP_VLS);

	//pi->pkey_8b = cpu_to_be16(0);
	//pi->pkey_10b = cpu_to_be16(0);

	pi->mkey_violations = cpu_to_be16(ibp->mkey_violations);
	/* P_KeyViolations are counted by hardware. */
	pi->pkey_violations = cpu_to_be16(ibp->pkey_violations);
	pi->qkey_violations = cpu_to_be16(ibp->qkey_violations);

	//pi->sm_trap_qp = cpu_to_be32(0);
	//pi->sa_qp = cpu_to_be32(0);

	//pi->vl.inittype = 0;
	pi->vl.cap = ppd->vls_supported;
	pi->vl.high_limit = cpu_to_be16(ibp->vl_high_limit);
	//pi->vl.preempt_limit = cpu_to_be16(0);
	pi->vl.arb_high_cap = (u8)dd->f_get_ib_cfg(ppd, QIB_IB_CFG_VL_HIGH_CAP);
	pi->vl.arb_low_cap = (u8)dd->f_get_ib_cfg(ppd, QIB_IB_CFG_VL_LOW_CAP);

	pi->mtucap = vpi->mtucap;
	/* HCAs ignore VLStallCount and HOQLife */
	/* pip->vlstallcnt_hoqlife; */

	pi->clientrereg_subnettimeout = ibp->subnet_timeout;
	pi->localphy_overrun_errors =
		(get_phyerrthreshold(ppd) << 4) |
		get_overrunthreshold(ppd);
	/* pip->max_credit_hint; */
#if 0
	/* no longer used in STL */
	if (ibp->port_cap_flags & IB_PORT_LINK_LATENCY_SUP) {
		u32 v;

		v = dd->f_get_ib_cfg(ppd, QIB_IB_CFG_LINKLATENCY);
		pi->link_roundtrip_latency = cpu_to_be32(v);
	}
#endif

	//pi->ib_cap_mask2 = 0;
	//pi->stl_cap_mask = 0;

	/* HCAs ignore VLStallCount and HOQLife */
	//pi->xmit_q[x] = 0;

	//pi->neigh_node_guid = cpu_to_be64(0);

	/* FIXME supported, enabled, active all == STL */
	pi->port_link_mode  = STL_PORT_LINK_MODE_STL << 10;
	pi->port_link_mode |= STL_PORT_LINK_MODE_STL << 5;
	pi->port_link_mode |= STL_PORT_LINK_MODE_STL;
	pi->port_link_mode = cpu_to_be16(pi->port_link_mode);

	/* FIXME supported, enabled, active all == 16-bit */
	pi->port_ltp_crc_mode  = STL_PORT_LTP_CRC_MODE_16 << 8;
	pi->port_ltp_crc_mode |= STL_PORT_LTP_CRC_MODE_16 << 4;
	pi->port_ltp_crc_mode |= STL_PORT_LTP_CRC_MODE_16;
	pi->port_ltp_crc_mode = cpu_to_be16(pi->port_ltp_crc_mode);

	//pi->port_mode = cpu_to_be16(0);
	//pi->port_neigh_mode = 0;

	pi->port_packet_format.supported = cpu_to_be16(STL_PORT_PACKET_FORMAT_9B);
	pi->port_packet_format.enabled = cpu_to_be16(STL_PORT_PACKET_FORMAT_9B);

	pi->link_down_reason = STL_LINKDOWN_REASON_NONE;

	// now picked up from the simulated port info
	//pi->link_width_downgrade.supported = 0;
	//pi->link_width_downgrade.active = 0;
	//pi->link_width_downgrade.enabled = 0;

	// these are RO and only set at linkup time
	//pi->buffer_units = cpu_to_be32(0);

	// these are RO and only set at linkup time
	//pi->replay_depth.buffer = 0;
	//pi->replay_depth.wire = 0;

	//pi->flit_control.interleave = cpu_to_be16(0);
	//pi->flit_control.preemption.min_initial = cpu_to_be16(0);
	//pi->flit_control.preemption.min_tail = cpu_to_be16(0);
	//pi->flit_control.preemption.large_pkt_limit = 0;
	//pi->flit_control.preemption.small_pkt_limit = 0;
	//pi->flit_control.preemption.max_small_pkt_limit = 0;
	//pi->flit_control.preemption.preemption_limit = 0;

	//pi->pass_through.egress_port = 0;
	//pi->pass_through.res_drctl = 0;

	/* 32.768 usec. response time (guessing) */
	pi->resptimevalue = 3;

	pi->local_port_num = port;
	//pi->ganged_port_details = 0;
	// No longer used in STL
	//pi->guid_cap = 1;

	ret = reply_stl(smp);

bail:
	return ret;
}

/**
 * subn_set_portinfo - set port information
 * @smp: the incoming SM packet
 * @ibdev: the infiniband device
 * @port: the port on the device
 *
 * Set Portinfo (see ch. 14.2.5.6).
 */
static int subn_set_stl_portinfo(struct stl_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct stl_port_info *pi;
	struct ib_event event;
	struct qib_devdata *dd;
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;
	u8 clientrereg;
	unsigned long flags;
	u32 stl_lid; /* Temp to hold STL LID values */
	u16 lid, smlid;
	u8 state;
	u8 vls;
	u8 msl;
	u16 lse, lwe;
	u16 lstate;
	int ret, ore;
	u32 port_num = be32_to_cpu(smp->attr_mod) & 0xff;
	u32 num_ports = be32_to_cpu(smp->attr_mod) >> 24;
	struct stl_port_info *vpi;
	u8 orig_portphysstate_portstate;

	printk(KERN_WARNING PFX "SubnSet(STL_PortInfo) port (attr_mod) %d\n", port_num);

	/* NOTE: the following checks could be optimized.
	 *       but for testing and clarity I wanted to keep them separate. */
	if (wfr_strict_am_processing) {
		if (num_ports != 1) {
			printk(KERN_WARNING PFX "STL Set(STLPortInfo) invalid AM.N\n");
			goto err;
		}

		/* PortInfo is only valid on receiving port
		 * or 0 for wildcard
		 */
		if (port_num == 0)
			port_num = port;

		if (port_num != port) {
			printk(KERN_WARNING PFX
				"STL Set(STLPortInfo) invalid AM.P %d; inbound %d\n",
				port_num, port);
			goto err;
		}
	} else {
		if (num_ports > 1) {
			goto err;
		}
		if (port_num == 0)
			port_num = port;
		else {
			if (port_num > ibdev->phys_port_cnt)
				goto err;
			/* Port attributes can only be set on the receiving port */
			if (port_num != port)
				goto get_only;
		}
	}

	vpi = &virtual_stl[port_num-1].port_info;
	orig_portphysstate_portstate = vpi->port_states.portphysstate_portstate;

	pi = (struct stl_port_info *)stl_get_smp_data(smp);

	stl_lid = be32_to_cpu(pi->lid);
	if (stl_lid & 0xFFFF0000) {
		printk(KERN_WARNING PFX "STL_PortInfo lid out of range: %X\n",
			stl_lid);
		goto err;
	}
	lid = (u16)(stl_lid & 0x0000FFFF);

	stl_lid = be32_to_cpu(pi->sm_lid);
	if (stl_lid & 0xFFFF0000) {
		printk(KERN_WARNING PFX "STL_PortInfo SM lid out of range: %X\n",
			stl_lid);
		goto err;
	}
	smlid = (u16)(stl_lid & 0x0000FFFF);

	clientrereg = (pi->clientrereg_subnettimeout &
			STL_PI_MASK_CLIENT_REREGISTER);

	dd = dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port_num - 1);
	ibp = &ppd->ibport_data;
	event.device = ibdev;
	event.element.port_num = port;

	ibp->mkey = pi->mkey;
	ibp->gid_prefix = pi->subnet_prefix;
	ibp->mkey_lease_period = be16_to_cpu(pi->mkey_lease_period);

	/* Must be a valid unicast LID address. */
	if (lid == 0 || lid >= QIB_MULTICAST_LID_BASE) {
		smp->status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX
			"SubnSet(STL_PortInfo) lid invalid 0x%x\n",
			lid);
	} else if (ppd->lid != lid ||
		 ppd->lmc != (pi->mkeyprotect_lmc & STL_PI_MASK_LMC)) {
		if (ppd->lid != lid)
			qib_set_uevent_bits(ppd, _QIB_EVENT_LID_CHANGE_BIT);
		if (ppd->lmc != (pi->mkeyprotect_lmc & STL_PI_MASK_LMC))
			qib_set_uevent_bits(ppd, _QIB_EVENT_LMC_CHANGE_BIT);
		qib_set_lid(ppd, lid, pi->mkeyprotect_lmc & STL_PI_MASK_LMC);
		event.event = IB_EVENT_LID_CHANGE;
		ib_dispatch_event(&event);
	}

	msl = pi->smsl & STL_PI_MASK_SMSL;
	/* Must be a valid unicast LID address. */
	if (smlid == 0 || smlid >= QIB_MULTICAST_LID_BASE) {
		smp->status |= IB_SMP_INVALID_FIELD;
		printk(KERN_WARNING PFX
			"SubnSet(STL_PortInfo) smlid invalid 0x%x\n",
			smlid);
	} else if (smlid != ibp->sm_lid || msl != ibp->sm_sl) {
		printk(KERN_WARNING PFX
			"SubnSet(STL_PortInfo) smlid 0x%x\n", smlid);
		spin_lock_irqsave(&ibp->lock, flags);
		if (ibp->sm_ah) {
			if (smlid != ibp->sm_lid)
				ibp->sm_ah->attr.dlid = smlid;
			if (msl != ibp->sm_sl)
				ibp->sm_ah->attr.sl = msl;
		}
		spin_unlock_irqrestore(&ibp->lock, flags);
		if (smlid != ibp->sm_lid)
			ibp->sm_lid = smlid;
		if (msl != ibp->sm_sl)
			ibp->sm_sl = msl;
		event.event = IB_EVENT_SM_CHANGE;
		ib_dispatch_event(&event);
	}

	lwe = be16_to_cpu(pi->link_width.enabled);
	if (lwe) {
		if ((lwe & STL_LINK_WIDTH_ALL_SUPPORTED) == STL_LINK_WIDTH_ALL_SUPPORTED) {
			vpi->link_width.enabled = cpu_to_be16(STL_LINK_WIDTH_ALL_SUPPORTED);
		} else if (lwe & be16_to_cpu(vpi->link_width.supported)) {
			vpi->link_width.enabled = pi->link_width.enabled;
		} else
			smp->status |= IB_SMP_INVALID_FIELD;
	}
	lse = be16_to_cpu(pi->link_speed.enabled);
	if (lse) {
		if ((lse & STL_LINK_SPEED_ALL_SUPPORTED) == STL_LINK_SPEED_ALL_SUPPORTED) {
			vpi->link_speed.enabled = cpu_to_be16(STL_LINK_SPEED_ALL_SUPPORTED);
		} else if (lse & be16_to_cpu(vpi->link_speed.supported)) {
			vpi->link_speed.enabled = pi->link_speed.enabled;
		} else
			smp->status |= IB_SMP_INVALID_FIELD;
	}

	/* Set link down default state. */
	/* Again make this virtual.  Only IB PortInfo controls the actual port
	 * states */
	switch (pi->port_states.unsleepstate_downdefstate & STL_PI_MASK_DOWNDEF_STATE) {
	case 0: /* NOP */
		break;
	case 1: /* SLEEP */
	/*
		(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT,
					IB_LINKINITCMD_SLEEP);
					*/
		break;
	case 2: /* POLL */
	/*
		(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT,
					IB_LINKINITCMD_POLL);
					*/
		break;
	default:
		printk(KERN_WARNING PFX
			"SubnSet(STL_PortInfo) Default state invalid 0x%x\n",
			pi->port_states.unsleepstate_downdefstate & STL_PI_MASK_DOWNDEF_STATE);
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	ibp->mkeyprot = (pi->mkeyprotect_lmc & STL_PI_MASK_MKEY_PROT_BIT) >> 6;
	ibp->vl_high_limit = be16_to_cpu(pi->vl.high_limit) & 0xFF;
	(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_VL_HIGH_LIMIT,
				    ibp->vl_high_limit);

#if 0
	mtu = ib_mtu_enum_to_int((pi->neigh_mtu.pvlx_to_mtu[0] >> 4) & 0xF);
	if (mtu == -1) {
		printk(KERN_WARNING PFX
			"SubnSet(STL_PortInfo) mtu invalid %d (0x%x)\n", mtu,
			(pi->neigh_mtu.pvlx_to_mtu[0] >> 4) & 0xF);
		smp->status |= IB_SMP_INVALID_FIELD;
	} else {
		printk(KERN_WARNING PFX "SubnSet(STL_PortInfo) Neigh MTU ignored %d\n", mtu);
		//qib_set_mtu(ppd, mtu);
	}
#else
	set_virtual_mtu(port, pi);
#endif

	/* Set operational VLs */
	vls = pi->operational_vls & STL_PI_MASK_OPERATIONAL_VL;
	if (vls) {
		if (vls > ppd->vls_supported) {
			printk(KERN_WARNING PFX
				"SubnSet(STL_PortInfo) VL's supported invalid %d\n",
				pi->operational_vls);
			smp->status |= IB_SMP_INVALID_FIELD;
		} else
			(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_OP_VLS, vls);
	}

	if (pi->mkey_violations == 0)
		ibp->mkey_violations = 0;

	if (pi->pkey_violations == 0)
		ibp->pkey_violations = 0;

	if (pi->qkey_violations == 0)
		ibp->qkey_violations = 0;

	ore = pi->localphy_overrun_errors;
	if (set_phyerrthreshold(ppd, (ore >> 4) & 0xF)) {
		printk(KERN_WARNING PFX
			"SubnSet(STL_PortInfo) phyerrthreshold invalid 0x%x\n",
			(ore >> 4) & 0xF);
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	if (set_overrunthreshold(ppd, (ore & 0xF))) {
		printk(KERN_WARNING PFX
			"SubnSet(STL_PortInfo) overrunthreshold invalid 0x%x\n",
			ore & 0xF);
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	ibp->subnet_timeout = pi->clientrereg_subnettimeout & STL_PI_MASK_SUBNET_TIMEOUT;

	/*
	 * Do the port state change now that the other link parameters
	 * have been set.
	 * Changing the port physical state only makes sense if the link
	 * is down or is being set to down.
	 */
	state = pi->port_states.portphysstate_portstate & STL_PI_MASK_PORT_STATE;
	lstate = (pi->port_states.portphysstate_portstate & STL_PI_MASK_PORT_PHYSICAL_STATE) >> 4;
	if (lstate && !(state == IB_PORT_DOWN || state == IB_PORT_NOP)) {
		printk(KERN_WARNING PFX
			"SubnSet(STL_PortInfo) port state invalid; state 0x%x link state 0x%x\n",
			state, lstate);
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	/*
	 * Only state changes of DOWN, ARM, and ACTIVE are valid
	 * and must be in the correct state to take effect (see 7.2.6).
	 */
	switch (state) {
	case IB_PORT_NOP:
		if (lstate == 0)
			break;
		/* FALLTHROUGH */
	case IB_PORT_DOWN:
#if 0
		if (lstate == 0)
			//lstate = QIB_IB_LINKDOWN_ONLY;
		else if (lstate == 1)
			//lstate = QIB_IB_LINKDOWN_SLEEP;
		else if (lstate == 2)
			//lstate = QIB_IB_LINKDOWN;
		else if (lstate == 3)
			//lstate = QIB_IB_LINKDOWN_DISABLE;
		else {
#endif
		/* FIXME allow other STL states */
		if (lstate > 3) {
			printk(KERN_WARNING PFX
				"SubnSet(STL_PortInfo) invalid Physical state 0x%x\n",
				lstate);
			smp->status |= IB_SMP_INVALID_FIELD;
			break;
		}
#if 0
		spin_lock_irqsave(&ppd->lflags_lock, flags);
		ppd->lflags &= ~QIBL_LINKV;
		spin_unlock_irqrestore(&ppd->lflags_lock, flags);
		qib_set_linkstate(ppd, lstate);
		/*
		 * Don't send a reply if the response would be sent
		 * through the disabled port.
		 */
		if (lstate == QIB_IB_LINKDOWN_DISABLE && smp->hop_cnt) {
			ret = IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
			goto done;
		}
		qib_wait_linkstate(ppd, QIBL_LINKV, 10);
#endif
		break;
	case IB_PORT_ARMED:
		//qib_set_linkstate(ppd, QIB_IB_LINKARM);
		arm_virtual_port(port);
		break;
	case IB_PORT_ACTIVE:
		//qib_set_linkstate(ppd, QIB_IB_LINKACTIVE);
		activate_virtual_port(port);
		break;
	default:
		printk(KERN_WARNING PFX
			"SubnSet(STL_PortInfo) invalid state 0x%x\n", state);
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	if (lstate) {
		u8 tmp;
		tmp = virtual_stl[port-1].port_info.port_states.portphysstate_portstate;
		virtual_stl[port-1].port_info.port_states.portphysstate_portstate =
			((lstate << 4) & STL_PI_MASK_PORT_PHYSICAL_STATE) |
			(tmp & STL_PI_MASK_PORT_STATE);
		printk(KERN_WARNING PFX
			"SubnSet(STL_PortInfo) Port PhyState 0x%x virtualized\n",
			lstate);
		if (lstate == 3) {
			virtual_stl[port-1].port_info.port_states.portphysstate_portstate = 3 << 4;
			virtual_stl[port-1].port_info.port_states.portphysstate_portstate |= 1;
		} else if (lstate == 2) {
			virtual_port_linkup_init(port);
		}
	}

	virtual_stl[port-1].port_info.flow_control_mask = pi->flow_control_mask;

	if (vpi->port_states.portphysstate_portstate != orig_portphysstate_portstate) {
		wfr_send_stl_virt_link_info(ibdev, port, vpi->port_states.portphysstate_portstate,
					dd->pport->guid,
					wfr_mgmt_allowed ?
						STL_PI_MASK_NEIGH_MGMT_ALLOWED : 0,
					vpi->link_speed.active,
					vpi->link_width.active);
	}

	if (clientrereg) {
		event.event = IB_EVENT_CLIENT_REREGISTER;
		ib_dispatch_event(&event);
	}

	ret = subn_get_stl_portinfo(smp, ibdev, port);

	/* restore re-reg bit per o14-12.2.1 */
	pi->clientrereg_subnettimeout |= clientrereg;

	goto done;

err:
	smp->status |= IB_SMP_INVALID_FIELD;
get_only:
	ret = subn_get_stl_portinfo(smp, ibdev, port);
done:
	return ret;
}


static int subn_get_portinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_devdata *dd;
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;
	struct ib_port_info *pip = (struct ib_port_info *)smp->data;
	u8 mtu;
	int ret;
	u32 state;
	u32 port_num = be32_to_cpu(smp->attr_mod);

	if (port_num == 0)
		port_num = port;
	else {
		if (port_num > ibdev->phys_port_cnt) {
			smp->status |= IB_SMP_INVALID_FIELD;
			ret = reply(smp);
			goto bail;
		}
		if (port_num != port) {
			ibp = to_iport(ibdev, port_num);
			ret = check_mkey(ibp, smp, 0);
			if (ret) {
				ret = IB_MAD_RESULT_FAILURE;
				goto bail;
			}
		}
	}

	dd = dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port_num - 1);
	ibp = &ppd->ibport_data;

	/* Clear all fields.  Only set the non-zero fields. */
	memset(smp->data, 0, sizeof(smp->data));

	/* Only return the mkey if the protection field allows it. */
	if (!(smp->method == IB_MGMT_METHOD_GET &&
	      ibp->mkey != smp->mkey &&
	      ibp->mkeyprot == 1))
		pip->mkey = ibp->mkey;
	pip->gid_prefix = ibp->gid_prefix;
	pip->lid = cpu_to_be16(ppd->lid);
	pip->sm_lid = cpu_to_be16(ibp->sm_lid);
	pip->cap_mask = cpu_to_be32(ibp->port_cap_flags);
	/* pip->diag_code; */
	pip->mkey_lease_period = cpu_to_be16(ibp->mkey_lease_period);
	pip->local_port_num = port;
	pip->link_width_enabled = ppd->link_width_enabled;
	pip->link_width_supported = ppd->link_width_supported;
	pip->link_width_active = ppd->link_width_active;
	state = dd->f_iblink_state(ppd->lastibcstat);
	pip->linkspeed_portstate = ppd->link_speed_supported << 4 | state;

	pip->portphysstate_linkdown =
		(dd->f_ibphys_portstate(ppd->lastibcstat) << 4) |
		(get_linkdowndefaultstate(ppd) ? 1 : 2);
	pip->mkeyprot_resv_lmc = (ibp->mkeyprot << 6) | ppd->lmc;
	pip->linkspeedactive_enabled = (ppd->link_speed_active << 4) |
		ppd->link_speed_enabled;
	switch (ppd->ibmtu) {
	default: /* something is wrong; fall through */
	case 4096:
		mtu = IB_MTU_4096;
		break;
	case 2048:
		mtu = IB_MTU_2048;
		break;
	case 1024:
		mtu = IB_MTU_1024;
		break;
	case 512:
		mtu = IB_MTU_512;
		break;
	case 256:
		mtu = IB_MTU_256;
		break;
	}
	pip->neighbormtu_mastersmsl = (mtu << 4) | ibp->sm_sl;
	pip->vlcap_inittype = ppd->vls_supported << 4;  /* InitType = 0 */
	pip->vl_high_limit = ibp->vl_high_limit;
	pip->vl_arb_high_cap =
		dd->f_get_ib_cfg(ppd, QIB_IB_CFG_VL_HIGH_CAP);
	pip->vl_arb_low_cap =
		dd->f_get_ib_cfg(ppd, QIB_IB_CFG_VL_LOW_CAP);
	pip->inittypereply_mtucap = qib_ibmtu ? qib_ibmtu : IB_MTU_2048;
	/* HCAs ignore VLStallCount and HOQLife */
	/* pip->vlstallcnt_hoqlife; */
	pip->operationalvl_pei_peo_fpi_fpo =
		dd->f_get_ib_cfg(ppd, QIB_IB_CFG_OP_VLS) << 4;
	pip->mkey_violations = cpu_to_be16(ibp->mkey_violations);
	/* P_KeyViolations are counted by hardware. */
	pip->pkey_violations = cpu_to_be16(ibp->pkey_violations);
	pip->qkey_violations = cpu_to_be16(ibp->qkey_violations);
	/* Only the hardware GUID is supported for now */
	pip->guid_cap = QIB_GUIDS_PER_PORT;
	pip->clientrereg_resv_subnetto = ibp->subnet_timeout;
	/* 32.768 usec. response time (guessing) */
	pip->resv_resptimevalue = 3;
	pip->localphyerrors_overrunerrors =
		(get_phyerrthreshold(ppd) << 4) |
		get_overrunthreshold(ppd);
	/* pip->max_credit_hint; */
	if (ibp->port_cap_flags & IB_PORT_LINK_LATENCY_SUP) {
		u32 v;

		v = dd->f_get_ib_cfg(ppd, QIB_IB_CFG_LINKLATENCY);
		pip->link_roundtrip_latency[0] = v >> 16;
		pip->link_roundtrip_latency[1] = v >> 8;
		pip->link_roundtrip_latency[2] = v;
	}

	ret = reply(smp);

bail:
	return ret;
}

/**
 * get_pkeys - return the PKEY table
 * @dd: the qlogic_ib device
 * @port: the IB port number
 * @pkeys: the pkey table is placed here
 */
static int get_pkeys(struct qib_devdata *dd, u8 port, u16 *pkeys)
{
	struct qib_pportdata *ppd = dd->pport + port - 1;
	/*
	 * always a kernel context, no locking needed.
	 * If we get here with ppd setup, no need to check
	 * that pd is valid.
	 */
	struct qib_ctxtdata *rcd = dd->rcd[ppd->hw_pidx];

	memcpy(pkeys, rcd->pkeys, sizeof(rcd->pkeys));

	return 0;
}

static int subn_get_pkeytable(struct ib_smp *smp, struct ib_device *ibdev,
			      u8 port)
{
	u32 startpx = 32 * (be32_to_cpu(smp->attr_mod) & 0xffff);
	u16 *p = (u16 *) smp->data;
	__be16 *q = (__be16 *) smp->data;

	/* 64 blocks of 32 16-bit P_Key entries */

	memset(smp->data, 0, sizeof(smp->data));
	if (startpx == 0) {
		struct qib_devdata *dd = dd_from_ibdev(ibdev);
		unsigned i, n = qib_get_npkeys(dd);

		get_pkeys(dd, port, p);

		for (i = 0; i < n; i++)
			q[i] = cpu_to_be16(p[i]);
	} else
		smp->status |= IB_SMP_INVALID_FIELD;

	return reply(smp);
}

static int subn_get_stl_pkeytable(struct stl_smp *smp, struct ib_device *ibdev,
				  u8 port)
{
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	u32 n_blocks_req = (be32_to_cpu(smp->attr_mod) & 0xff000000) >> 24;
	u32 am_port = (be32_to_cpu(smp->attr_mod) & 0x00ff0000) >> 16;
	u32 start_block = be32_to_cpu(smp->attr_mod) & 0x7ff;
	__be16 *p;
	u8 *from;
	int i;
	size_t size;
	u16 n_blocks_avail;

	if (wfr_strict_am_processing) {
		if (am_port == 0)
			am_port = port;

		if (am_port != port || n_blocks_req == 0) {
			printk(KERN_WARNING PFX
				"STL Get PKey AM Invalid : P = %d; B = 0x%x; N = 0x%x\n",
				am_port, start_block, n_blocks_req);
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply_stl(smp);
		}
	} else {
		/* special case to make this Attribute more compatible with IB queries */
		if (n_blocks_req == 0)
			n_blocks_req = 1;
	}

	if (wfr_sim_pkey_tbl_block_size) {
		n_blocks_avail = wfrl_get_num_sim_pkey_blocks();
	} else {
		n_blocks_avail = (u16)(qib_get_npkeys(dd)/STL_PARTITION_TABLE_BLK_SIZE) +1;
	}

	memset(stl_get_smp_data(smp), 0, stl_get_smp_data_size(smp));

	if (start_block + n_blocks_req > n_blocks_avail ||
	    n_blocks_req > STL_NUM_PKEY_BLOCKS_PER_SMP) {
		printk(KERN_WARNING PFX
			"STL Get PKey AM Invalid : s 0x%x; req 0x%x; avail 0x%x; blk/smp 0x%lx\n",
			start_block, n_blocks_req, n_blocks_avail,
			STL_NUM_PKEY_BLOCKS_PER_SMP);
		smp->status |= IB_SMP_INVALID_FIELD;
		goto error;
	}

	// get the real pkeys if we are requesting the first block
	if (start_block == 0) {
		get_pkeys(dd, port, virtual_stl[port-1].pkeys);
	}

	from = (u8 *)virtual_stl[port-1].pkeys + (start_block * STL_PARTITION_TABLE_BLK_SIZE * sizeof(u16));
	size = n_blocks_req * STL_PARTITION_TABLE_BLK_SIZE * sizeof(u16);
	p = (__be16 *) stl_get_smp_data(smp);
	memcpy((void *)p, from, size);

	for (i = 0; i < n_blocks_req * STL_PARTITION_TABLE_BLK_SIZE; i++)
		p[i] = cpu_to_be16(p[i]);

error:
	return reply_stl(smp);
}

static int subn_get_stl_sl_to_sc(struct stl_smp *smp, struct ib_device *ibdev,
				  u8 port)
{
	u8 *p = stl_get_smp_data(smp);
	unsigned i;
	u32 num_ports = be32_to_cpu(smp->attr_mod) >> 24;

	if (wfr_strict_am_processing && num_ports != 1) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply_stl(smp);
	}

	memset(p, 0, stl_get_smp_data_size(smp));

	for (i = 0; i < ARRAY_SIZE(virtual_stl[port-1].sl_to_sc); i++)
		*p++ = virtual_stl[port-1].sl_to_sc[i];

	return reply_stl(smp);
}

static int subn_set_stl_sl_to_sc(struct stl_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	u8 *p = stl_get_smp_data(smp);
	unsigned i;
	u32 num_ports = be32_to_cpu(smp->attr_mod) >> 24;

	if (wfr_strict_am_processing && num_ports != 1) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply_stl(smp);
	}

	for (i = 0; i < ARRAY_SIZE(virtual_stl[port-1].sl_to_sc); i ++, p++)
		virtual_stl[port-1].sl_to_sc[i] = *p & 0x1f;

	qib_set_uevent_bits(ppd_from_ibp(to_iport(ibdev, port)),
			    _QIB_EVENT_SL2VL_CHANGE_BIT);

	return subn_get_stl_sl_to_sc(smp, ibdev, port);
}

static int subn_get_stl_sc_to_sl(struct stl_smp *smp, struct ib_device *ibdev,
				  u8 port)
{
	u8 *p = stl_get_smp_data(smp);
	unsigned i;

	if (wfr_strict_am_processing && be32_to_cpu(smp->attr_mod) != 0) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply_stl(smp);
	}

	memset(p, 0, stl_get_smp_data_size(smp));

	for (i = 0; i < ARRAY_SIZE(virtual_stl[port-1].sc_to_sl); i++)
		*p++ = virtual_stl[port-1].sc_to_sl[i];

	return reply_stl(smp);
}

static int subn_set_stl_sc_to_sl(struct stl_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	u8 *p = stl_get_smp_data(smp);
	unsigned i;

	if (wfr_strict_am_processing && be32_to_cpu(smp->attr_mod) != 0) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply_stl(smp);
	}

	for (i = 0; i < ARRAY_SIZE(virtual_stl[port-1].sc_to_sl); i ++, p++)
		virtual_stl[port-1].sc_to_sl[i] = *p & 0x1f;

	return subn_get_stl_sc_to_sl(smp, ibdev, port);
}

static int subn_get_stl_sc_to_vlt(struct stl_smp *smp, struct ib_device *ibdev,
				  u8 port)
{
	u8 *p = stl_get_smp_data(smp);
	unsigned i;
	u32 num_ports = (be32_to_cpu(smp->attr_mod) & 0xff000000) >> 24;
	u32 port_num = be32_to_cpu(smp->attr_mod) & 0x000000ff;

	if (wfr_strict_am_processing) {
		if (port_num == 0)
			port_num = port;
		if (num_ports != 1 ||
		    port_num != port) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply_stl(smp);
		}
	}

	memset(p, 0, stl_get_smp_data_size(smp));

	/* FIXME We need to resolve getting the real sl to vl values set here */
	for (i = 0; i < ARRAY_SIZE(virtual_stl[port-1].sc_to_vlt); i++)
		*p++ = virtual_stl[port-1].sc_to_vlt[i] & 0x1f;

	return reply_stl(smp);
}

static int subn_set_stl_sc_to_vlt(struct stl_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	u8 *p = stl_get_smp_data(smp);
	unsigned i;
	u8 port_state = wfrl_get_stl_virtual_port_state(port) & 0x0f;
	u32 num_ports = (be32_to_cpu(smp->attr_mod) & 0xff000000) >> 24;
	u32 async_update = be32_to_cpu(smp->attr_mod) & 0x1000;
	u32 port_num = be32_to_cpu(smp->attr_mod) & 0x000000ff;

	if (wfr_strict_am_processing) {
		if (port_num == 0)
			port_num = port;
		if (num_ports != 1 ||
		    port_num != port) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply_stl(smp);
		}
	}

	/* check the AM and port state for proper settings */
#if 0
	if (async_update && port_state == IB_PORT_INIT) {
#endif
	if (async_update) {
		/* for now we don't support IsAsyncSC2VLSupported */
		smp->status |= IB_SMP_INVALID_FIELD;
		goto error;
	} else if (port_state == IB_PORT_ARMED || port_state == IB_PORT_ACTIVE) {
		smp->status |= IB_SMP_INVALID_FIELD;
		goto error;
	}

	/* FIXME We need to resolve getting the real sl to vl values set here */
	for (i = 0; i < ARRAY_SIZE(virtual_stl[port-1].sc_to_vlt); i ++, p++)
		virtual_stl[port-1].sc_to_vlt[i] = *p & 0x1f;

error:
	return subn_get_stl_sc_to_vlt(smp, ibdev, port);
}

static int subn_get_stl_sc_to_vlnt(struct stl_smp *smp, struct ib_device *ibdev,
				  u8 port)
{
	u8 *p = stl_get_smp_data(smp);
	unsigned i;
	u32 num_ports = (be32_to_cpu(smp->attr_mod) & 0xff000000) >> 24;
	u32 port_num = be32_to_cpu(smp->attr_mod) & 0x000000ff;

	if (wfr_strict_am_processing) {
		if (port_num == 0)
			port_num = port;
		if (num_ports != 1 ||
		    port_num != port) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply_stl(smp);
		}
	}


	memset(p, 0, stl_get_smp_data_size(smp));

	/* FIXME We need to resolve getting the real sl to vl values set here */
	for (i = 0; i < ARRAY_SIZE(virtual_stl[port-1].sc_to_vlnt); i++)
		*p++ = virtual_stl[port-1].sc_to_vlnt[i] & 0x1f;

	return reply_stl(smp);
}

static int subn_set_stl_sc_to_vlnt(struct stl_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	u8 *p = stl_get_smp_data(smp);
	unsigned i;
	u8 port_state = wfrl_get_stl_virtual_port_state(port) & 0x0f;

	u32 num_ports = (be32_to_cpu(smp->attr_mod) & 0xff000000) >> 24;
	u32 port_num = be32_to_cpu(smp->attr_mod) & 0x000000ff;

	if (wfr_strict_am_processing) {
		if (port_num == 0)
			port_num = port;
		if (num_ports != 1 ||
		    port_num != port) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply_stl(smp);
		}
	}


	/* check the AM and port state for proper settings */
	if (port_state == IB_PORT_ARMED || port_state == IB_PORT_ACTIVE) {
		smp->status |= IB_SMP_INVALID_FIELD;
		goto error;
	}

	/* FIXME We need to resolve getting the real sl to vl values set here */
	for (i = 0; i < ARRAY_SIZE(virtual_stl[port-1].sc_to_vlnt); i ++, p++)
		virtual_stl[port-1].sc_to_vlnt[i] = *p & 0x1f;

error:
	return subn_get_stl_sc_to_vlnt(smp, ibdev, port);
}

static int subn_get_stl_vlarb(struct stl_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct stl_vlarb_data *virt_data = &virtual_stl[port-1].vlarb_data;
	u32 num_ports = (be32_to_cpu(smp->attr_mod) & 0xff000000) >> 24;
	u8 section = (be32_to_cpu(smp->attr_mod) & 0x00ff0000) >> 16;
	u32 port_num = be32_to_cpu(smp->attr_mod) & 0x000000ff;
	u8 *p = stl_get_smp_data(smp);

	if (wfr_strict_am_processing) {
		if (num_ports != 1 ||
		    port_num != port) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply_stl(smp);
		}
	}

	switch (section) {
		case STL_VLARB_LOW_ELEMENTS:
			memcpy(p, virt_data->low_pri_table,
				sizeof(virt_data->low_pri_table));
			break;
		case STL_VLARB_HIGH_ELEMENTS:
			memcpy(p, virt_data->high_pri_table,
				sizeof(virt_data->high_pri_table));
			break;
		case STL_VLARB_PREEMPT_ELEMENTS:
			memcpy(p, virt_data->preempting_table,
				sizeof(virt_data->preempting_table));
			break;
		case STL_VLARB_PREEMPT_MATRIX:
			memcpy(p, virt_data->preemption_matrix,
				sizeof(virt_data->preemption_matrix));
			break;
		default:
			printk(KERN_WARNING PFX
				"STL SubnGet(VL Arb) AM Invalid : 0x%x\n",
				be32_to_cpu(smp->attr_mod));
			smp->status |= IB_SMP_INVALID_FIELD;
			break;
	}

	return reply_stl(smp);
}
static int subn_set_stl_vlarb(struct stl_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct stl_vlarb_data *virt_data = &virtual_stl[port-1].vlarb_data;
	u32 num_ports = (be32_to_cpu(smp->attr_mod) & 0xff000000) >> 24;
	u8 section = (be32_to_cpu(smp->attr_mod) & 0x00ff0000) >> 16;
	u32 port_num = be32_to_cpu(smp->attr_mod) & 0x000000ff;
	u8 *p = stl_get_smp_data(smp);

	if (wfr_strict_am_processing) {
		if (num_ports != 1 ||
		    port_num != port) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply_stl(smp);
		}
	}

	switch (section) {
		case STL_VLARB_LOW_ELEMENTS:
			memcpy(virt_data->low_pri_table, p,
				sizeof(virt_data->low_pri_table));
			break;
		case STL_VLARB_HIGH_ELEMENTS:
			memcpy(virt_data->high_pri_table, p,
				sizeof(virt_data->high_pri_table));
			break;
		case STL_VLARB_PREEMPT_ELEMENTS:
			memcpy(virt_data->preempting_table, p,
				sizeof(virt_data->preempting_table));
			break;
		case STL_VLARB_PREEMPT_MATRIX:
			memcpy(virt_data->preemption_matrix, p,
				sizeof(virt_data->preemption_matrix));
			break;
		default:
			printk(KERN_WARNING PFX
				"STL SubnSet(VL Arb) AM Invalid : 0x%x\n",
				be32_to_cpu(smp->attr_mod));
			smp->status |= IB_SMP_INVALID_FIELD;
			break;
	}

	return subn_get_stl_vlarb(smp, ibdev, port);
}

static int subn_get_stl_psi(struct stl_smp *smp, struct ib_device *ibdev,
			u8 port)
{
	struct stl_port_info *vpi = &virtual_stl[port-1].port_info;
	u32 num_ports = (be32_to_cpu(smp->attr_mod) & 0xff000000) >> 24;
	u32 port_num = be32_to_cpu(smp->attr_mod) & 0x000000ff;
	u8 *p = stl_get_smp_data(smp);

	if (wfr_strict_am_processing) {
		if (num_ports != 1 || port_num != port) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply_stl(smp);
		}
	}

	memcpy(p, &vpi->port_states, sizeof(vpi->port_states));

	return reply_stl(smp);
}

static int subn_get_stl_bct(struct stl_smp *smp, struct ib_device *ibdev,
			u8 port)
{
	struct stl_buffer_control_table *vbct = &virtual_stl[port-1].buffer_control_table;
	u32 num_ports = (be32_to_cpu(smp->attr_mod) & 0xff000000) >> 24;
	u32 port_num = be32_to_cpu(smp->attr_mod) & 0x000000ff;
	u8 *p = stl_get_smp_data(smp);

	if (wfr_strict_am_processing) {
		if (num_ports != 1 || port_num != port) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply_stl(smp);
		}
	}

	memcpy(p, vbct, sizeof(*vbct));
	return reply_stl(smp);
}

static int subn_set_stl_bct(struct stl_smp *smp, struct ib_device *ibdev,
			u8 port)
{
	struct stl_buffer_control_table *vbct = &virtual_stl[port-1].buffer_control_table;
	u32 num_ports = (be32_to_cpu(smp->attr_mod) & 0xff000000) >> 24;
	u32 port_num = be32_to_cpu(smp->attr_mod) & 0x000000ff;
	u8 *p = stl_get_smp_data(smp);

	if (wfr_strict_am_processing) {
		if (num_ports != 1 || port_num != port) {
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply_stl(smp);
		}
	}

	memcpy(vbct, p, sizeof(*vbct));
	return reply_stl(smp);
}


static int subn_set_guidinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	u32 startgx = 8 * be32_to_cpu(smp->attr_mod);
	__be64 *p = (__be64 *) smp->data;
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	/* 32 blocks of 8 64-bit GUIDs per block */

	if (startgx == 0 && pidx < dd->num_pports) {
		struct qib_pportdata *ppd = dd->pport + pidx;
		struct qib_ibport *ibp = &ppd->ibport_data;
		unsigned i;

		/* The first entry is read-only. */
		for (i = 1; i < QIB_GUIDS_PER_PORT; i++)
			ibp->guids[i - 1] = p[i];
	} else
		smp->status |= IB_SMP_INVALID_FIELD;

	/* The only GUID we support is the first read-only entry. */
	return subn_get_guidinfo(smp, ibdev, port);
}

/**
 * subn_set_portinfo - set port information
 * @smp: the incoming SM packet
 * @ibdev: the infiniband device
 * @port: the port on the device
 *
 * Set Portinfo (see ch. 14.2.5.6).
 */
static int subn_set_portinfo(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct ib_port_info *pip = (struct ib_port_info *)smp->data;
	struct ib_event event;
	struct qib_devdata *dd;
	struct qib_pportdata *ppd;
	struct qib_ibport *ibp;
	u8 clientrereg = (pip->clientrereg_resv_subnetto & 0x80);
	unsigned long flags;
	u16 lid, smlid;
	u8 lwe;
	u8 lse;
	u8 state;
	u8 vls;
	u8 msl;
	u16 lstate;
	int ret, ore, mtu;
	u32 port_num = be32_to_cpu(smp->attr_mod);

	if (port_num == 0)
		port_num = port;
	else {
		if (port_num > ibdev->phys_port_cnt)
			goto err;
		/* Port attributes can only be set on the receiving port */
		if (port_num != port)
			goto get_only;
	}

	dd = dd_from_ibdev(ibdev);
	/* IB numbers ports from 1, hdw from 0 */
	ppd = dd->pport + (port_num - 1);
	ibp = &ppd->ibport_data;
	event.device = ibdev;
	event.element.port_num = port;

	ibp->mkey = pip->mkey;
	ibp->gid_prefix = pip->gid_prefix;
	ibp->mkey_lease_period = be16_to_cpu(pip->mkey_lease_period);

	lid = be16_to_cpu(pip->lid);
	/* Must be a valid unicast LID address. */
	if (lid == 0 || lid >= QIB_MULTICAST_LID_BASE)
		smp->status |= IB_SMP_INVALID_FIELD;
	else if (ppd->lid != lid || ppd->lmc != (pip->mkeyprot_resv_lmc & 7)) {
		if (ppd->lid != lid)
			qib_set_uevent_bits(ppd, _QIB_EVENT_LID_CHANGE_BIT);
		if (ppd->lmc != (pip->mkeyprot_resv_lmc & 7))
			qib_set_uevent_bits(ppd, _QIB_EVENT_LMC_CHANGE_BIT);
		qib_set_lid(ppd, lid, pip->mkeyprot_resv_lmc & 7);
		event.event = IB_EVENT_LID_CHANGE;
		ib_dispatch_event(&event);
	}

	smlid = be16_to_cpu(pip->sm_lid);
	msl = pip->neighbormtu_mastersmsl & 0xF;
	/* Must be a valid unicast LID address. */
	if (smlid == 0 || smlid >= QIB_MULTICAST_LID_BASE)
		smp->status |= IB_SMP_INVALID_FIELD;
	else if (smlid != ibp->sm_lid || msl != ibp->sm_sl) {
		spin_lock_irqsave(&ibp->lock, flags);
		if (ibp->sm_ah) {
			if (smlid != ibp->sm_lid)
				ibp->sm_ah->attr.dlid = smlid;
			if (msl != ibp->sm_sl)
				ibp->sm_ah->attr.sl = msl;
		}
		spin_unlock_irqrestore(&ibp->lock, flags);
		if (smlid != ibp->sm_lid)
			ibp->sm_lid = smlid;
		if (msl != ibp->sm_sl)
			ibp->sm_sl = msl;
		event.event = IB_EVENT_SM_CHANGE;
		ib_dispatch_event(&event);
	}

	/* Allow 1x or 4x to be set (see 14.2.6.6). */
	lwe = pip->link_width_enabled;
	if (lwe) {
		if (lwe == 0xFF)
			set_link_width_enabled(ppd, ppd->link_width_supported);
		else if (lwe >= 16 || (lwe & ~ppd->link_width_supported))
			smp->status |= IB_SMP_INVALID_FIELD;
		else if (lwe != ppd->link_width_enabled)
			set_link_width_enabled(ppd, lwe);
	}

	lse = pip->linkspeedactive_enabled & 0xF;
	if (lse) {
		/*
		 * The IB 1.2 spec. only allows link speed values
		 * 1, 3, 5, 7, 15.  1.2.1 extended to allow specific
		 * speeds.
		 */
		if (lse == 15)
			set_link_speed_enabled(ppd,
					       ppd->link_speed_supported);
		else if (lse >= 8 || (lse & ~ppd->link_speed_supported))
			smp->status |= IB_SMP_INVALID_FIELD;
		else if (lse != ppd->link_speed_enabled)
			set_link_speed_enabled(ppd, lse);
	}

	/* Set link down default state. */
	switch (pip->portphysstate_linkdown & 0xF) {
	case 0: /* NOP */
		break;
	case 1: /* SLEEP */
		(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT,
					IB_LINKINITCMD_SLEEP);
		break;
	case 2: /* POLL */
		(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_LINKDEFAULT,
					IB_LINKINITCMD_POLL);
		break;
	default:
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	ibp->mkeyprot = pip->mkeyprot_resv_lmc >> 6;
	ibp->vl_high_limit = pip->vl_high_limit;
	(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_VL_HIGH_LIMIT,
				    ibp->vl_high_limit);

	mtu = ib_mtu_enum_to_int((pip->neighbormtu_mastersmsl >> 4) & 0xF);
	if (mtu == -1)
		smp->status |= IB_SMP_INVALID_FIELD;
	else
		qib_set_mtu(ppd, mtu);

	/* Set operational VLs */
	vls = (pip->operationalvl_pei_peo_fpi_fpo >> 4) & 0xF;
	if (vls) {
		if (vls > ppd->vls_supported)
			smp->status |= IB_SMP_INVALID_FIELD;
		else
			(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_OP_VLS, vls);
	}

	if (pip->mkey_violations == 0)
		ibp->mkey_violations = 0;

	if (pip->pkey_violations == 0)
		ibp->pkey_violations = 0;

	if (pip->qkey_violations == 0)
		ibp->qkey_violations = 0;

	ore = pip->localphyerrors_overrunerrors;
	if (set_phyerrthreshold(ppd, (ore >> 4) & 0xF))
		smp->status |= IB_SMP_INVALID_FIELD;

	if (set_overrunthreshold(ppd, (ore & 0xF)))
		smp->status |= IB_SMP_INVALID_FIELD;

	ibp->subnet_timeout = pip->clientrereg_resv_subnetto & 0x1F;

	/*
	 * Do the port state change now that the other link parameters
	 * have been set.
	 * Changing the port physical state only makes sense if the link
	 * is down or is being set to down.
	 */
	state = pip->linkspeed_portstate & 0xF;
	lstate = (pip->portphysstate_linkdown >> 4) & 0xF;
	if (lstate && !(state == IB_PORT_DOWN || state == IB_PORT_NOP))
		smp->status |= IB_SMP_INVALID_FIELD;

	/*
	 * Only state changes of DOWN, ARM, and ACTIVE are valid
	 * and must be in the correct state to take effect (see 7.2.6).
	 */
	switch (state) {
	case IB_PORT_NOP:
		if (lstate == 0)
			break;
		/* FALLTHROUGH */
	case IB_PORT_DOWN:
		if (lstate == 0)
			lstate = QIB_IB_LINKDOWN_ONLY;
		else if (lstate == 1)
			lstate = QIB_IB_LINKDOWN_SLEEP;
		else if (lstate == 2)
			lstate = QIB_IB_LINKDOWN;
		else if (lstate == 3)
			lstate = QIB_IB_LINKDOWN_DISABLE;
		else {
			smp->status |= IB_SMP_INVALID_FIELD;
			break;
		}
		spin_lock_irqsave(&ppd->lflags_lock, flags);
		ppd->lflags &= ~QIBL_LINKV;
		spin_unlock_irqrestore(&ppd->lflags_lock, flags);
		qib_set_linkstate(ppd, lstate);
		/*
		 * Don't send a reply if the response would be sent
		 * through the disabled port.
		 */
		if (lstate == QIB_IB_LINKDOWN_DISABLE && smp->hop_cnt) {
			ret = IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
			goto done;
		}
		qib_wait_linkstate(ppd, QIBL_LINKV, 10);
		break;
	case IB_PORT_ARMED:
		qib_set_linkstate(ppd, QIB_IB_LINKARM);
		break;
	case IB_PORT_ACTIVE:
		qib_set_linkstate(ppd, QIB_IB_LINKACTIVE);
		wfr_send_stl_link_info(ibdev, port);
		break;
	default:
		smp->status |= IB_SMP_INVALID_FIELD;
	}

	if (clientrereg) {
		event.event = IB_EVENT_CLIENT_REREGISTER;
		ib_dispatch_event(&event);
	}

	ret = subn_get_portinfo(smp, ibdev, port);

	/* restore re-reg bit per o14-12.2.1 */
	pip->clientrereg_resv_subnetto |= clientrereg;

	goto get_only;

err:
	smp->status |= IB_SMP_INVALID_FIELD;
get_only:
	ret = subn_get_portinfo(smp, ibdev, port);
done:
	return ret;
}

/**
 * rm_pkey - decrecment the reference count for the given PKEY
 * @dd: the qlogic_ib device
 * @key: the PKEY index
 *
 * Return true if this was the last reference and the hardware table entry
 * needs to be changed.
 */
static int rm_pkey(struct qib_pportdata *ppd, u16 key)
{
	int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(ppd->pkeys); i++) {
		if (ppd->pkeys[i] != key)
			continue;
		if (atomic_dec_and_test(&ppd->pkeyrefs[i])) {
			ppd->pkeys[i] = 0;
			ret = 1;
			goto bail;
		}
		break;
	}

	ret = 0;

bail:
	return ret;
}

/**
 * add_pkey - add the given PKEY to the hardware table
 * @dd: the qlogic_ib device
 * @key: the PKEY
 *
 * Return an error code if unable to add the entry, zero if no change,
 * or 1 if the hardware PKEY register needs to be updated.
 */
static int add_pkey(struct qib_pportdata *ppd, u16 key)
{
	int i;
	u16 lkey = key & 0x7FFF;
	int any = 0;
	int ret;

	if (lkey == 0x7FFF) {
		ret = 0;
		goto bail;
	}

	/* Look for an empty slot or a matching PKEY. */
	for (i = 0; i < ARRAY_SIZE(ppd->pkeys); i++) {
		if (!ppd->pkeys[i]) {
			any++;
			continue;
		}
		/* If it matches exactly, try to increment the ref count */
		if (ppd->pkeys[i] == key) {
			if (atomic_inc_return(&ppd->pkeyrefs[i]) > 1) {
				ret = 0;
				goto bail;
			}
			/* Lost the race. Look for an empty slot below. */
			atomic_dec(&ppd->pkeyrefs[i]);
			any++;
		}
		/*
		 * It makes no sense to have both the limited and unlimited
		 * PKEY set at the same time since the unlimited one will
		 * disable the limited one.
		 */
		if ((ppd->pkeys[i] & 0x7FFF) == lkey) {
			ret = -EEXIST;
			goto bail;
		}
	}
	if (!any) {
		ret = -EBUSY;
		goto bail;
	}
	for (i = 0; i < ARRAY_SIZE(ppd->pkeys); i++) {
		if (!ppd->pkeys[i] &&
		    atomic_inc_return(&ppd->pkeyrefs[i]) == 1) {
			/* for qibstats, etc. */
			ppd->pkeys[i] = key;
			ret = 1;
			goto bail;
		}
	}
	ret = -EBUSY;

bail:
	return ret;
}

/**
 * set_pkeys - set the PKEY table for ctxt 0
 * @dd: the qlogic_ib device
 * @port: the IB port number
 * @pkeys: the PKEY table
 */
static int set_pkeys(struct qib_devdata *dd, u8 port, u16 *pkeys)
{
	struct qib_pportdata *ppd;
	struct qib_ctxtdata *rcd;
	int i;
	int changed = 0;

	/*
	 * IB port one/two always maps to context zero/one,
	 * always a kernel context, no locking needed
	 * If we get here with ppd setup, no need to check
	 * that rcd is valid.
	 */
	ppd = dd->pport + (port - 1);
	rcd = dd->rcd[ppd->hw_pidx];

	for (i = 0; i < ARRAY_SIZE(rcd->pkeys); i++) {
		u16 key = pkeys[i];
		u16 okey = rcd->pkeys[i];

		if (key == okey)
			continue;
		/*
		 * The value of this PKEY table entry is changing.
		 * Remove the old entry in the hardware's array of PKEYs.
		 */
		if (okey & 0x7FFF)
			changed |= rm_pkey(ppd, okey);
		if (key & 0x7FFF) {
			int ret = add_pkey(ppd, key);

			if (ret < 0)
				key = 0;
			else
				changed |= ret;
		}
		rcd->pkeys[i] = key;
	}
	if (changed) {
		struct ib_event event;

		(void) dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PKEYS, 0);

		event.event = IB_EVENT_PKEY_CHANGE;
		event.device = &dd->verbs_dev.ibdev;
		event.element.port_num = 1;
		ib_dispatch_event(&event);
	}
	return 0;
}

static int subn_set_pkeytable(struct ib_smp *smp, struct ib_device *ibdev,
			      u8 port)
{
	u32 startpx = 32 * (be32_to_cpu(smp->attr_mod) & 0xffff);
	__be16 *p = (__be16 *) smp->data;
	u16 *q = (u16 *) smp->data;
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	unsigned i, n = qib_get_npkeys(dd);

	for (i = 0; i < n; i++)
		q[i] = be16_to_cpu(p[i]);

	if (startpx != 0 || set_pkeys(dd, port, q) != 0)
		smp->status |= IB_SMP_INVALID_FIELD;

	return subn_get_pkeytable(smp, ibdev, port);
}

static int subn_set_stl_pkeytable(struct stl_smp *smp, struct ib_device *ibdev,
			      u8 port)
{
	struct qib_devdata *dd = dd_from_ibdev(ibdev);
	u32 n_blocks_sent = (be32_to_cpu(smp->attr_mod) & 0xff000000) >> 24;
	u32 am_port = (be32_to_cpu(smp->attr_mod) & 0x00ff0000) >> 16;
	u32 start_block = be32_to_cpu(smp->attr_mod) & 0x7ff;
	u16 *p = (u16 *) stl_get_smp_data(smp);
	u16 *cur_keys;
	int i, b;
	int found_lim;
	size_t size;
	u16 n_blocks_avail;

	if (wfr_strict_am_processing) {
		if (am_port == 0)
			am_port = port;

		if (am_port != port || n_blocks_sent == 0) {
			printk(KERN_WARNING PFX
				"STL Get PKey AM Invalid : P = %d; B = 0x%x; N = 0x%x\n",
				am_port, start_block, n_blocks_sent);
			smp->status |= IB_SMP_INVALID_FIELD;
			return reply_stl(smp);
		}
	} else {
		/* special case to make this Attribute more compatible with IB queries */
		if (n_blocks_sent == 0)
			n_blocks_sent = 1;
	}

	if (wfr_sim_pkey_tbl_block_size) {
		n_blocks_avail = wfrl_get_num_sim_pkey_blocks();
	} else {
		n_blocks_avail = (u16)(qib_get_npkeys(dd)/STL_PARTITION_TABLE_BLK_SIZE) +1;
	}

	if (start_block + n_blocks_sent > n_blocks_avail ||
	    n_blocks_sent > STL_NUM_PKEY_BLOCKS_PER_SMP) {
		printk(KERN_WARNING PFX
			"STL Set PKey AM Invalid : s 0x%x; req 0x%x; avail 0x%x; blk/smp 0x%lx\n",
			start_block, n_blocks_sent, n_blocks_avail,
			STL_NUM_PKEY_BLOCKS_PER_SMP);
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply_stl(smp);
	}

	p = (u16 *) stl_get_smp_data(smp);
	for (i = 0; i < n_blocks_sent * STL_PARTITION_TABLE_BLK_SIZE; i++)
		p[i] = be16_to_cpu(p[i]);

	/* before writing this block ensure the limited management pkey is somewhere in the table. */
	virtual_stl[port-1].member_full_mgmt = 0;
	found_lim = 0;
	for (b = 0; b < n_blocks_avail; b++) {
		if (start_block <= b && b < (start_block + n_blocks_sent)) {
			cur_keys = p + ((b - start_block) * STL_PARTITION_TABLE_BLK_SIZE * sizeof(u16));
		} else {
			cur_keys = virtual_stl[port-1].pkeys + (b * STL_PARTITION_TABLE_BLK_SIZE * sizeof(u16));
		}
		for (i = 0; i < STL_PARTITION_TABLE_BLK_SIZE; i++) {
			if (cur_keys[i] == WFR_LIM_MGMT_P_KEY)
				found_lim++;
			if (cur_keys[i] == WFR_FULL_MGMT_P_KEY)
				virtual_stl[port-1].member_full_mgmt = 1;
		}
	}

	if (!found_lim) {
		printk(KERN_ERR PFX
			"STL Set(PKeyTable) would result in the removal of 0x%x; rejecting\n",
			WFR_LIM_MGMT_P_KEY);
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply_stl(smp);
	}

	if (start_block == 0 && set_pkeys(dd, port, p) != 0) {
		smp->status |= IB_SMP_INVALID_FIELD;
		return reply_stl(smp);
	}

	// Set virtual pkeys for this block/port
	cur_keys = virtual_stl[port-1].pkeys + (start_block * STL_PARTITION_TABLE_BLK_SIZE * sizeof(u16));
	size = n_blocks_sent * STL_PARTITION_TABLE_BLK_SIZE * sizeof(u16);
	memcpy(cur_keys, (void *)p, size);

	return subn_get_stl_pkeytable(smp, ibdev, port);
}

static int subn_get_sl_to_vl(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	u8 *p = (u8 *) smp->data;
	unsigned i;

	memset(smp->data, 0, sizeof(smp->data));

	if (!(ibp->port_cap_flags & IB_PORT_SL_MAP_SUP))
		smp->status |= IB_SMP_UNSUP_METHOD;
	else
		for (i = 0; i < ARRAY_SIZE(ibp->sl_to_vl); i += 2)
			*p++ = (ibp->sl_to_vl[i] << 4) | ibp->sl_to_vl[i + 1];

	return reply(smp);
}

static int subn_set_sl_to_vl(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	u8 *p = (u8 *) smp->data;
	unsigned i;

	if (!(ibp->port_cap_flags & IB_PORT_SL_MAP_SUP)) {
		smp->status |= IB_SMP_UNSUP_METHOD;
		return reply(smp);
	}

	for (i = 0; i < ARRAY_SIZE(ibp->sl_to_vl); i += 2, p++) {
		ibp->sl_to_vl[i] = *p >> 4;
		ibp->sl_to_vl[i + 1] = *p & 0xF;
	}
	qib_set_uevent_bits(ppd_from_ibp(to_iport(ibdev, port)),
			    _QIB_EVENT_SL2VL_CHANGE_BIT);

	return subn_get_sl_to_vl(smp, ibdev, port);
}

static int subn_get_vl_arb(struct ib_smp *smp, struct ib_device *ibdev,
			   u8 port)
{
	unsigned which = be32_to_cpu(smp->attr_mod) >> 16;
	struct qib_pportdata *ppd = ppd_from_ibp(to_iport(ibdev, port));

	memset(smp->data, 0, sizeof(smp->data));

	if (ppd->vls_supported == IB_VL_VL0)
		smp->status |= IB_SMP_UNSUP_METHOD;
	else if (which == IB_VLARB_LOWPRI_0_31)
		(void) ppd->dd->f_get_ib_table(ppd, QIB_IB_TBL_VL_LOW_ARB,
						   smp->data);
	else if (which == IB_VLARB_HIGHPRI_0_31)
		(void) ppd->dd->f_get_ib_table(ppd, QIB_IB_TBL_VL_HIGH_ARB,
						   smp->data);
	else
		smp->status |= IB_SMP_INVALID_FIELD;

	return reply(smp);
}

static int subn_set_vl_arb(struct ib_smp *smp, struct ib_device *ibdev,
			   u8 port)
{
	unsigned which = be32_to_cpu(smp->attr_mod) >> 16;
	struct qib_pportdata *ppd = ppd_from_ibp(to_iport(ibdev, port));

	if (ppd->vls_supported == IB_VL_VL0)
		smp->status |= IB_SMP_UNSUP_METHOD;
	else if (which == IB_VLARB_LOWPRI_0_31)
		(void) ppd->dd->f_set_ib_table(ppd, QIB_IB_TBL_VL_LOW_ARB,
						   smp->data);
	else if (which == IB_VLARB_HIGHPRI_0_31)
		(void) ppd->dd->f_set_ib_table(ppd, QIB_IB_TBL_VL_HIGH_ARB,
						   smp->data);
	else
		smp->status |= IB_SMP_INVALID_FIELD;

	return subn_get_vl_arb(smp, ibdev, port);
}

static int subn_trap_repress(struct ib_smp *smp, struct ib_device *ibdev,
			     u8 port)
{
	/*
	 * For now, we only send the trap once so no need to process this.
	 * o13-6, o13-7,
	 * o14-3.a4 The SMA shall not send any message in response to a valid
	 * SubnTrapRepress() message.
	 */
	return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
}

static int pma_get_classportinfo(struct ib_pma_mad *pmp,
				 struct ib_device *ibdev)
{
	struct ib_class_port_info *p =
		(struct ib_class_port_info *)pmp->data;
	struct qib_devdata *dd = dd_from_ibdev(ibdev);

	memset(pmp->data, 0, sizeof(pmp->data));

	if (pmp->mad_hdr.attr_mod != 0)
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;

	/* Note that AllPortSelect is not valid */
	p->base_version = 1;
	p->class_version = 1;
	p->capability_mask = IB_PMA_CLASS_CAP_EXT_WIDTH;
	/*
	 * Set the most significant bit of CM2 to indicate support for
	 * congestion statistics
	 */
	p->reserved[0] = dd->psxmitwait_supported << 7;
	/*
	 * Expected response time is 4.096 usec. * 2^18 == 1.073741824 sec.
	 */
	p->resp_time_value = 18;

	return reply((struct ib_smp *) pmp);
}

static int pma_get_portsamplescontrol(struct ib_pma_mad *pmp,
				      struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portsamplescontrol *p =
		(struct ib_pma_portsamplescontrol *)pmp->data;
	struct qib_ibdev *dev = to_idev(ibdev);
	struct qib_devdata *dd = dd_from_dev(dev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	unsigned long flags;
	u8 port_select = p->port_select;

	memset(pmp->data, 0, sizeof(pmp->data));

	p->port_select = port_select;
	if (pmp->mad_hdr.attr_mod != 0 || port_select != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		goto bail;
	}
	spin_lock_irqsave(&ibp->lock, flags);
	p->tick = dd->f_get_ib_cfg(ppd, QIB_IB_CFG_PMA_TICKS);
	p->sample_status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
	p->counter_width = 4;   /* 32 bit counters */
	p->counter_mask0_9 = COUNTER_MASK0_9;
	p->sample_start = cpu_to_be32(ibp->pma_sample_start);
	p->sample_interval = cpu_to_be32(ibp->pma_sample_interval);
	p->tag = cpu_to_be16(ibp->pma_tag);
	p->counter_select[0] = ibp->pma_counter_select[0];
	p->counter_select[1] = ibp->pma_counter_select[1];
	p->counter_select[2] = ibp->pma_counter_select[2];
	p->counter_select[3] = ibp->pma_counter_select[3];
	p->counter_select[4] = ibp->pma_counter_select[4];
	spin_unlock_irqrestore(&ibp->lock, flags);

bail:
	return reply((struct ib_smp *) pmp);
}

static int pma_set_portsamplescontrol(struct ib_pma_mad *pmp,
				      struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portsamplescontrol *p =
		(struct ib_pma_portsamplescontrol *)pmp->data;
	struct qib_ibdev *dev = to_idev(ibdev);
	struct qib_devdata *dd = dd_from_dev(dev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	unsigned long flags;
	u8 status, xmit_flags;
	int ret;

	if (pmp->mad_hdr.attr_mod != 0 || p->port_select != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		ret = reply((struct ib_smp *) pmp);
		goto bail;
	}

	spin_lock_irqsave(&ibp->lock, flags);

	/* Port Sampling code owns the PS* HW counters */
	xmit_flags = ppd->cong_stats.flags;
	ppd->cong_stats.flags = IB_PMA_CONG_HW_CONTROL_SAMPLE;
	status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
	if (status == IB_PMA_SAMPLE_STATUS_DONE ||
	    (status == IB_PMA_SAMPLE_STATUS_RUNNING &&
	     xmit_flags == IB_PMA_CONG_HW_CONTROL_TIMER)) {
		ibp->pma_sample_start = be32_to_cpu(p->sample_start);
		ibp->pma_sample_interval = be32_to_cpu(p->sample_interval);
		ibp->pma_tag = be16_to_cpu(p->tag);
		ibp->pma_counter_select[0] = p->counter_select[0];
		ibp->pma_counter_select[1] = p->counter_select[1];
		ibp->pma_counter_select[2] = p->counter_select[2];
		ibp->pma_counter_select[3] = p->counter_select[3];
		ibp->pma_counter_select[4] = p->counter_select[4];
		dd->f_set_cntr_sample(ppd, ibp->pma_sample_interval,
				      ibp->pma_sample_start);
	}
	spin_unlock_irqrestore(&ibp->lock, flags);

	ret = pma_get_portsamplescontrol(pmp, ibdev, port);

bail:
	return ret;
}

static u64 get_counter(struct qib_ibport *ibp, struct qib_pportdata *ppd,
		       __be16 sel)
{
	u64 ret;

	switch (sel) {
	case IB_PMA_PORT_XMIT_DATA:
		ret = ppd->dd->f_portcntr(ppd, QIBPORTCNTR_PSXMITDATA);
		break;
	case IB_PMA_PORT_RCV_DATA:
		ret = ppd->dd->f_portcntr(ppd, QIBPORTCNTR_PSRCVDATA);
		break;
	case IB_PMA_PORT_XMIT_PKTS:
		ret = ppd->dd->f_portcntr(ppd, QIBPORTCNTR_PSXMITPKTS);
		break;
	case IB_PMA_PORT_RCV_PKTS:
		ret = ppd->dd->f_portcntr(ppd, QIBPORTCNTR_PSRCVPKTS);
		break;
	case IB_PMA_PORT_XMIT_WAIT:
		ret = ppd->dd->f_portcntr(ppd, QIBPORTCNTR_PSXMITWAIT);
		break;
	default:
		ret = 0;
	}

	return ret;
}

/* This function assumes that the xmit_wait lock is already held */
static u64 xmit_wait_get_value_delta(struct qib_pportdata *ppd)
{
	u32 delta;

	delta = get_counter(&ppd->ibport_data, ppd,
			    IB_PMA_PORT_XMIT_WAIT);
	return ppd->cong_stats.counter + delta;
}

static void cache_hw_sample_counters(struct qib_pportdata *ppd)
{
	struct qib_ibport *ibp = &ppd->ibport_data;

	ppd->cong_stats.counter_cache.psxmitdata =
		get_counter(ibp, ppd, IB_PMA_PORT_XMIT_DATA);
	ppd->cong_stats.counter_cache.psrcvdata =
		get_counter(ibp, ppd, IB_PMA_PORT_RCV_DATA);
	ppd->cong_stats.counter_cache.psxmitpkts =
		get_counter(ibp, ppd, IB_PMA_PORT_XMIT_PKTS);
	ppd->cong_stats.counter_cache.psrcvpkts =
		get_counter(ibp, ppd, IB_PMA_PORT_RCV_PKTS);
	ppd->cong_stats.counter_cache.psxmitwait =
		get_counter(ibp, ppd, IB_PMA_PORT_XMIT_WAIT);
}

static u64 get_cache_hw_sample_counters(struct qib_pportdata *ppd,
					__be16 sel)
{
	u64 ret;

	switch (sel) {
	case IB_PMA_PORT_XMIT_DATA:
		ret = ppd->cong_stats.counter_cache.psxmitdata;
		break;
	case IB_PMA_PORT_RCV_DATA:
		ret = ppd->cong_stats.counter_cache.psrcvdata;
		break;
	case IB_PMA_PORT_XMIT_PKTS:
		ret = ppd->cong_stats.counter_cache.psxmitpkts;
		break;
	case IB_PMA_PORT_RCV_PKTS:
		ret = ppd->cong_stats.counter_cache.psrcvpkts;
		break;
	case IB_PMA_PORT_XMIT_WAIT:
		ret = ppd->cong_stats.counter_cache.psxmitwait;
		break;
	default:
		ret = 0;
	}

	return ret;
}

static int pma_get_portsamplesresult(struct ib_pma_mad *pmp,
				     struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portsamplesresult *p =
		(struct ib_pma_portsamplesresult *)pmp->data;
	struct qib_ibdev *dev = to_idev(ibdev);
	struct qib_devdata *dd = dd_from_dev(dev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	unsigned long flags;
	u8 status;
	int i;

	memset(pmp->data, 0, sizeof(pmp->data));
	spin_lock_irqsave(&ibp->lock, flags);
	p->tag = cpu_to_be16(ibp->pma_tag);
	if (ppd->cong_stats.flags == IB_PMA_CONG_HW_CONTROL_TIMER)
		p->sample_status = IB_PMA_SAMPLE_STATUS_DONE;
	else {
		status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
		p->sample_status = cpu_to_be16(status);
		if (status == IB_PMA_SAMPLE_STATUS_DONE) {
			cache_hw_sample_counters(ppd);
			ppd->cong_stats.counter =
				xmit_wait_get_value_delta(ppd);
			dd->f_set_cntr_sample(ppd,
					      QIB_CONG_TIMER_PSINTERVAL, 0);
			ppd->cong_stats.flags = IB_PMA_CONG_HW_CONTROL_TIMER;
		}
	}
	for (i = 0; i < ARRAY_SIZE(ibp->pma_counter_select); i++)
		p->counter[i] = cpu_to_be32(
			get_cache_hw_sample_counters(
				ppd, ibp->pma_counter_select[i]));
	spin_unlock_irqrestore(&ibp->lock, flags);

	return reply((struct ib_smp *) pmp);
}

static int pma_get_portsamplesresult_ext(struct ib_pma_mad *pmp,
					 struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portsamplesresult_ext *p =
		(struct ib_pma_portsamplesresult_ext *)pmp->data;
	struct qib_ibdev *dev = to_idev(ibdev);
	struct qib_devdata *dd = dd_from_dev(dev);
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	unsigned long flags;
	u8 status;
	int i;

	/* Port Sampling code owns the PS* HW counters */
	memset(pmp->data, 0, sizeof(pmp->data));
	spin_lock_irqsave(&ibp->lock, flags);
	p->tag = cpu_to_be16(ibp->pma_tag);
	if (ppd->cong_stats.flags == IB_PMA_CONG_HW_CONTROL_TIMER)
		p->sample_status = IB_PMA_SAMPLE_STATUS_DONE;
	else {
		status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
		p->sample_status = cpu_to_be16(status);
		/* 64 bits */
		p->extended_width = cpu_to_be32(0x80000000);
		if (status == IB_PMA_SAMPLE_STATUS_DONE) {
			cache_hw_sample_counters(ppd);
			ppd->cong_stats.counter =
				xmit_wait_get_value_delta(ppd);
			dd->f_set_cntr_sample(ppd,
					      QIB_CONG_TIMER_PSINTERVAL, 0);
			ppd->cong_stats.flags = IB_PMA_CONG_HW_CONTROL_TIMER;
		}
	}
	for (i = 0; i < ARRAY_SIZE(ibp->pma_counter_select); i++)
		p->counter[i] = cpu_to_be64(
			get_cache_hw_sample_counters(
				ppd, ibp->pma_counter_select[i]));
	spin_unlock_irqrestore(&ibp->lock, flags);

	return reply((struct ib_smp *) pmp);
}

static int pma_get_portcounters(struct ib_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters *p = (struct ib_pma_portcounters *)
		pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_counters cntrs;
	u8 port_select = p->port_select;

	qib_get_counters(ppd, &cntrs);

	/* Adjust counters for any resets done. */
	cntrs.symbol_error_counter -= ibp->z_symbol_error_counter;
	cntrs.link_error_recovery_counter -=
		ibp->z_link_error_recovery_counter;
	cntrs.link_downed_counter -= ibp->z_link_downed_counter;
	cntrs.port_rcv_errors -= ibp->z_port_rcv_errors;
	cntrs.port_rcv_remphys_errors -= ibp->z_port_rcv_remphys_errors;
	cntrs.port_xmit_discards -= ibp->z_port_xmit_discards;
	cntrs.port_xmit_data -= ibp->z_port_xmit_data;
	cntrs.port_rcv_data -= ibp->z_port_rcv_data;
	cntrs.port_xmit_packets -= ibp->z_port_xmit_packets;
	cntrs.port_rcv_packets -= ibp->z_port_rcv_packets;
	cntrs.local_link_integrity_errors -=
		ibp->z_local_link_integrity_errors;
	cntrs.excessive_buffer_overrun_errors -=
		ibp->z_excessive_buffer_overrun_errors;
	cntrs.vl15_dropped -= ibp->z_vl15_dropped;
	cntrs.vl15_dropped += ibp->n_vl15_dropped;

	memset(pmp->data, 0, sizeof(pmp->data));

	p->port_select = port_select;
	if (pmp->mad_hdr.attr_mod != 0 || port_select != port)
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;

	if (cntrs.symbol_error_counter > 0xFFFFUL)
		p->symbol_error_counter = cpu_to_be16(0xFFFF);
	else
		p->symbol_error_counter =
			cpu_to_be16((u16)cntrs.symbol_error_counter);
	if (cntrs.link_error_recovery_counter > 0xFFUL)
		p->link_error_recovery_counter = 0xFF;
	else
		p->link_error_recovery_counter =
			(u8)cntrs.link_error_recovery_counter;
	if (cntrs.link_downed_counter > 0xFFUL)
		p->link_downed_counter = 0xFF;
	else
		p->link_downed_counter = (u8)cntrs.link_downed_counter;
	if (cntrs.port_rcv_errors > 0xFFFFUL)
		p->port_rcv_errors = cpu_to_be16(0xFFFF);
	else
		p->port_rcv_errors =
			cpu_to_be16((u16) cntrs.port_rcv_errors);
	if (cntrs.port_rcv_remphys_errors > 0xFFFFUL)
		p->port_rcv_remphys_errors = cpu_to_be16(0xFFFF);
	else
		p->port_rcv_remphys_errors =
			cpu_to_be16((u16)cntrs.port_rcv_remphys_errors);
	if (cntrs.port_xmit_discards > 0xFFFFUL)
		p->port_xmit_discards = cpu_to_be16(0xFFFF);
	else
		p->port_xmit_discards =
			cpu_to_be16((u16)cntrs.port_xmit_discards);
	if (cntrs.local_link_integrity_errors > 0xFUL)
		cntrs.local_link_integrity_errors = 0xFUL;
	if (cntrs.excessive_buffer_overrun_errors > 0xFUL)
		cntrs.excessive_buffer_overrun_errors = 0xFUL;
	p->link_overrun_errors = (cntrs.local_link_integrity_errors << 4) |
		cntrs.excessive_buffer_overrun_errors;
	if (cntrs.vl15_dropped > 0xFFFFUL)
		p->vl15_dropped = cpu_to_be16(0xFFFF);
	else
		p->vl15_dropped = cpu_to_be16((u16)cntrs.vl15_dropped);
	if (cntrs.port_xmit_data > 0xFFFFFFFFUL)
		p->port_xmit_data = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_xmit_data = cpu_to_be32((u32)cntrs.port_xmit_data);
	if (cntrs.port_rcv_data > 0xFFFFFFFFUL)
		p->port_rcv_data = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_rcv_data = cpu_to_be32((u32)cntrs.port_rcv_data);
	if (cntrs.port_xmit_packets > 0xFFFFFFFFUL)
		p->port_xmit_packets = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_xmit_packets =
			cpu_to_be32((u32)cntrs.port_xmit_packets);
	if (cntrs.port_rcv_packets > 0xFFFFFFFFUL)
		p->port_rcv_packets = cpu_to_be32(0xFFFFFFFF);
	else
		p->port_rcv_packets =
			cpu_to_be32((u32) cntrs.port_rcv_packets);

	return reply((struct ib_smp *) pmp);
}

static int pma_get_portcounters_cong(struct ib_pma_mad *pmp,
				     struct ib_device *ibdev, u8 port)
{
	/* Congestion PMA packets start at offset 24 not 64 */
	struct ib_pma_portcounters_cong *p =
		(struct ib_pma_portcounters_cong *)pmp->reserved;
	struct qib_verbs_counters cntrs;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_devdata *dd = dd_from_ppd(ppd);
	u32 port_select = be32_to_cpu(pmp->mad_hdr.attr_mod) & 0xFF;
	u64 xmit_wait_counter;
	unsigned long flags;

	/*
	 * This check is performed only in the GET method because the
	 * SET method ends up calling this anyway.
	 */
	if (!dd->psxmitwait_supported)
		pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
	if (port_select != port)
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;

	qib_get_counters(ppd, &cntrs);
	spin_lock_irqsave(&ppd->ibport_data.lock, flags);
	xmit_wait_counter = xmit_wait_get_value_delta(ppd);
	spin_unlock_irqrestore(&ppd->ibport_data.lock, flags);

	/* Adjust counters for any resets done. */
	cntrs.symbol_error_counter -= ibp->z_symbol_error_counter;
	cntrs.link_error_recovery_counter -=
		ibp->z_link_error_recovery_counter;
	cntrs.link_downed_counter -= ibp->z_link_downed_counter;
	cntrs.port_rcv_errors -= ibp->z_port_rcv_errors;
	cntrs.port_rcv_remphys_errors -=
		ibp->z_port_rcv_remphys_errors;
	cntrs.port_xmit_discards -= ibp->z_port_xmit_discards;
	cntrs.local_link_integrity_errors -=
		ibp->z_local_link_integrity_errors;
	cntrs.excessive_buffer_overrun_errors -=
		ibp->z_excessive_buffer_overrun_errors;
	cntrs.vl15_dropped -= ibp->z_vl15_dropped;
	cntrs.vl15_dropped += ibp->n_vl15_dropped;
	cntrs.port_xmit_data -= ibp->z_port_xmit_data;
	cntrs.port_rcv_data -= ibp->z_port_rcv_data;
	cntrs.port_xmit_packets -= ibp->z_port_xmit_packets;
	cntrs.port_rcv_packets -= ibp->z_port_rcv_packets;

	memset(pmp->reserved, 0, sizeof(pmp->reserved) +
	       sizeof(pmp->data));

	/*
	 * Set top 3 bits to indicate interval in picoseconds in
	 * remaining bits.
	 */
	p->port_check_rate =
		cpu_to_be16((QIB_XMIT_RATE_PICO << 13) |
			    (dd->psxmitwait_check_rate &
			     ~(QIB_XMIT_RATE_PICO << 13)));
	p->port_adr_events = cpu_to_be64(0);
	p->port_xmit_wait = cpu_to_be64(xmit_wait_counter);
	p->port_xmit_data = cpu_to_be64(cntrs.port_xmit_data);
	p->port_rcv_data = cpu_to_be64(cntrs.port_rcv_data);
	p->port_xmit_packets =
		cpu_to_be64(cntrs.port_xmit_packets);
	p->port_rcv_packets =
		cpu_to_be64(cntrs.port_rcv_packets);
	if (cntrs.symbol_error_counter > 0xFFFFUL)
		p->symbol_error_counter = cpu_to_be16(0xFFFF);
	else
		p->symbol_error_counter =
			cpu_to_be16(
				(u16)cntrs.symbol_error_counter);
	if (cntrs.link_error_recovery_counter > 0xFFUL)
		p->link_error_recovery_counter = 0xFF;
	else
		p->link_error_recovery_counter =
			(u8)cntrs.link_error_recovery_counter;
	if (cntrs.link_downed_counter > 0xFFUL)
		p->link_downed_counter = 0xFF;
	else
		p->link_downed_counter =
			(u8)cntrs.link_downed_counter;
	if (cntrs.port_rcv_errors > 0xFFFFUL)
		p->port_rcv_errors = cpu_to_be16(0xFFFF);
	else
		p->port_rcv_errors =
			cpu_to_be16((u16) cntrs.port_rcv_errors);
	if (cntrs.port_rcv_remphys_errors > 0xFFFFUL)
		p->port_rcv_remphys_errors = cpu_to_be16(0xFFFF);
	else
		p->port_rcv_remphys_errors =
			cpu_to_be16(
				(u16)cntrs.port_rcv_remphys_errors);
	if (cntrs.port_xmit_discards > 0xFFFFUL)
		p->port_xmit_discards = cpu_to_be16(0xFFFF);
	else
		p->port_xmit_discards =
			cpu_to_be16((u16)cntrs.port_xmit_discards);
	if (cntrs.local_link_integrity_errors > 0xFUL)
		cntrs.local_link_integrity_errors = 0xFUL;
	if (cntrs.excessive_buffer_overrun_errors > 0xFUL)
		cntrs.excessive_buffer_overrun_errors = 0xFUL;
	p->link_overrun_errors = (cntrs.local_link_integrity_errors << 4) |
		cntrs.excessive_buffer_overrun_errors;
	if (cntrs.vl15_dropped > 0xFFFFUL)
		p->vl15_dropped = cpu_to_be16(0xFFFF);
	else
		p->vl15_dropped = cpu_to_be16((u16)cntrs.vl15_dropped);

	return reply((struct ib_smp *)pmp);
}

static int pma_get_portcounters_ext(struct ib_pma_mad *pmp,
				    struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters_ext *p =
		(struct ib_pma_portcounters_ext *)pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u64 swords, rwords, spkts, rpkts, xwait;
	u8 port_select = p->port_select;

	memset(pmp->data, 0, sizeof(pmp->data));

	p->port_select = port_select;
	if (pmp->mad_hdr.attr_mod != 0 || port_select != port) {
		pmp->mad_hdr.status |= IB_SMP_INVALID_FIELD;
		goto bail;
	}

	qib_snapshot_counters(ppd, &swords, &rwords, &spkts, &rpkts, &xwait);

	/* Adjust counters for any resets done. */
	swords -= ibp->z_port_xmit_data;
	rwords -= ibp->z_port_rcv_data;
	spkts -= ibp->z_port_xmit_packets;
	rpkts -= ibp->z_port_rcv_packets;

	p->port_xmit_data = cpu_to_be64(swords);
	p->port_rcv_data = cpu_to_be64(rwords);
	p->port_xmit_packets = cpu_to_be64(spkts);
	p->port_rcv_packets = cpu_to_be64(rpkts);
	p->port_unicast_xmit_packets = cpu_to_be64(ibp->n_unicast_xmit);
	p->port_unicast_rcv_packets = cpu_to_be64(ibp->n_unicast_rcv);
	p->port_multicast_xmit_packets = cpu_to_be64(ibp->n_multicast_xmit);
	p->port_multicast_rcv_packets = cpu_to_be64(ibp->n_multicast_rcv);

bail:
	return reply((struct ib_smp *) pmp);
}

static int pma_set_portcounters(struct ib_pma_mad *pmp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters *p = (struct ib_pma_portcounters *)
		pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_verbs_counters cntrs;

	/*
	 * Since the HW doesn't support clearing counters, we save the
	 * current count and subtract it from future responses.
	 */
	qib_get_counters(ppd, &cntrs);

	if (p->counter_select & IB_PMA_SEL_SYMBOL_ERROR)
		ibp->z_symbol_error_counter = cntrs.symbol_error_counter;

	if (p->counter_select & IB_PMA_SEL_LINK_ERROR_RECOVERY)
		ibp->z_link_error_recovery_counter =
			cntrs.link_error_recovery_counter;

	if (p->counter_select & IB_PMA_SEL_LINK_DOWNED)
		ibp->z_link_downed_counter = cntrs.link_downed_counter;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_ERRORS)
		ibp->z_port_rcv_errors = cntrs.port_rcv_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_REMPHYS_ERRORS)
		ibp->z_port_rcv_remphys_errors =
			cntrs.port_rcv_remphys_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_DISCARDS)
		ibp->z_port_xmit_discards = cntrs.port_xmit_discards;

	if (p->counter_select & IB_PMA_SEL_LOCAL_LINK_INTEGRITY_ERRORS)
		ibp->z_local_link_integrity_errors =
			cntrs.local_link_integrity_errors;

	if (p->counter_select & IB_PMA_SEL_EXCESSIVE_BUFFER_OVERRUNS)
		ibp->z_excessive_buffer_overrun_errors =
			cntrs.excessive_buffer_overrun_errors;

	if (p->counter_select & IB_PMA_SEL_PORT_VL15_DROPPED) {
		ibp->n_vl15_dropped = 0;
		ibp->z_vl15_dropped = cntrs.vl15_dropped;
	}

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_DATA)
		ibp->z_port_xmit_data = cntrs.port_xmit_data;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_DATA)
		ibp->z_port_rcv_data = cntrs.port_rcv_data;

	if (p->counter_select & IB_PMA_SEL_PORT_XMIT_PACKETS)
		ibp->z_port_xmit_packets = cntrs.port_xmit_packets;

	if (p->counter_select & IB_PMA_SEL_PORT_RCV_PACKETS)
		ibp->z_port_rcv_packets = cntrs.port_rcv_packets;

	return pma_get_portcounters(pmp, ibdev, port);
}

static int pma_set_portcounters_cong(struct ib_pma_mad *pmp,
				     struct ib_device *ibdev, u8 port)
{
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct qib_devdata *dd = dd_from_ppd(ppd);
	struct qib_verbs_counters cntrs;
	u32 counter_select = (be32_to_cpu(pmp->mad_hdr.attr_mod) >> 24) & 0xFF;
	int ret = 0;
	unsigned long flags;

	qib_get_counters(ppd, &cntrs);
	/* Get counter values before we save them */
	ret = pma_get_portcounters_cong(pmp, ibdev, port);

	if (counter_select & IB_PMA_SEL_CONG_XMIT) {
		spin_lock_irqsave(&ppd->ibport_data.lock, flags);
		ppd->cong_stats.counter = 0;
		dd->f_set_cntr_sample(ppd, QIB_CONG_TIMER_PSINTERVAL,
				      0x0);
		spin_unlock_irqrestore(&ppd->ibport_data.lock, flags);
	}
	if (counter_select & IB_PMA_SEL_CONG_PORT_DATA) {
		ibp->z_port_xmit_data = cntrs.port_xmit_data;
		ibp->z_port_rcv_data = cntrs.port_rcv_data;
		ibp->z_port_xmit_packets = cntrs.port_xmit_packets;
		ibp->z_port_rcv_packets = cntrs.port_rcv_packets;
	}
	if (counter_select & IB_PMA_SEL_CONG_ALL) {
		ibp->z_symbol_error_counter =
			cntrs.symbol_error_counter;
		ibp->z_link_error_recovery_counter =
			cntrs.link_error_recovery_counter;
		ibp->z_link_downed_counter =
			cntrs.link_downed_counter;
		ibp->z_port_rcv_errors = cntrs.port_rcv_errors;
		ibp->z_port_rcv_remphys_errors =
			cntrs.port_rcv_remphys_errors;
		ibp->z_port_xmit_discards =
			cntrs.port_xmit_discards;
		ibp->z_local_link_integrity_errors =
			cntrs.local_link_integrity_errors;
		ibp->z_excessive_buffer_overrun_errors =
			cntrs.excessive_buffer_overrun_errors;
		ibp->n_vl15_dropped = 0;
		ibp->z_vl15_dropped = cntrs.vl15_dropped;
	}

	return ret;
}

static int pma_set_portcounters_ext(struct ib_pma_mad *pmp,
				    struct ib_device *ibdev, u8 port)
{
	struct ib_pma_portcounters *p = (struct ib_pma_portcounters *)
		pmp->data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u64 swords, rwords, spkts, rpkts, xwait;

	qib_snapshot_counters(ppd, &swords, &rwords, &spkts, &rpkts, &xwait);

	if (p->counter_select & IB_PMA_SELX_PORT_XMIT_DATA)
		ibp->z_port_xmit_data = swords;

	if (p->counter_select & IB_PMA_SELX_PORT_RCV_DATA)
		ibp->z_port_rcv_data = rwords;

	if (p->counter_select & IB_PMA_SELX_PORT_XMIT_PACKETS)
		ibp->z_port_xmit_packets = spkts;

	if (p->counter_select & IB_PMA_SELX_PORT_RCV_PACKETS)
		ibp->z_port_rcv_packets = rpkts;

	if (p->counter_select & IB_PMA_SELX_PORT_UNI_XMIT_PACKETS)
		ibp->n_unicast_xmit = 0;

	if (p->counter_select & IB_PMA_SELX_PORT_UNI_RCV_PACKETS)
		ibp->n_unicast_rcv = 0;

	if (p->counter_select & IB_PMA_SELX_PORT_MULTI_XMIT_PACKETS)
		ibp->n_multicast_xmit = 0;

	if (p->counter_select & IB_PMA_SELX_PORT_MULTI_RCV_PACKETS)
		ibp->n_multicast_rcv = 0;

	return pma_get_portcounters_ext(pmp, ibdev, port);
}

static int process_subn(struct ib_device *ibdev, int mad_flags,
			u8 port, struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	struct ib_smp *smp = (struct ib_smp *)out_mad;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	int ret;

	*out_mad = *in_mad;
	if (smp->class_version != 1) {
		smp->status |= IB_SMP_UNSUP_VERSION;
		ret = reply(smp);
		goto bail;
	}

	ret = check_mkey(ibp, smp, mad_flags);
	if (ret) {
		u32 port_num = be32_to_cpu(smp->attr_mod);

		/*
		 * If this is a get/set portinfo, we already check the
		 * M_Key if the MAD is for another port and the M_Key
		 * is OK on the receiving port. This check is needed
		 * to increment the error counters when the M_Key
		 * fails to match on *both* ports.
		 */
		if (in_mad->mad_hdr.attr_id == IB_SMP_ATTR_PORT_INFO &&
		    (smp->method == IB_MGMT_METHOD_GET ||
		     smp->method == IB_MGMT_METHOD_SET) &&
		    port_num && port_num <= ibdev->phys_port_cnt &&
		    port != port_num)
			(void) check_mkey(to_iport(ibdev, port_num), smp, 0);
		ret = IB_MAD_RESULT_FAILURE;
		goto bail;
	}

	switch (smp->method) {
	case IB_MGMT_METHOD_GET:
		switch (smp->attr_id) {
		case IB_SMP_ATTR_NODE_DESC:
			ret = subn_get_nodedescription(smp, ibdev);
			goto bail;
		case IB_SMP_ATTR_NODE_INFO:
			ret = subn_get_nodeinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_GUID_INFO:
			ret = subn_get_guidinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PORT_INFO:
			ret = subn_get_portinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PKEY_TABLE:
			ret = subn_get_pkeytable(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_SL_TO_VL_TABLE:
			ret = subn_get_sl_to_vl(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_VL_ARB_TABLE:
			ret = subn_get_vl_arb(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_SM_INFO:
			if (ibp->port_cap_flags & IB_PORT_SM_DISABLED) {
				ret = IB_MAD_RESULT_SUCCESS |
					IB_MAD_RESULT_CONSUMED;
				goto bail;
			}
			if (ibp->port_cap_flags & IB_PORT_SM) {
				ret = IB_MAD_RESULT_SUCCESS;
				goto bail;
			}
			/* FALLTHROUGH */
		default:
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(smp);
			goto bail;
		}

	case IB_MGMT_METHOD_SET:
		switch (smp->attr_id) {
		case IB_SMP_ATTR_GUID_INFO:
			ret = subn_set_guidinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PORT_INFO_STL_RESET:
			/* FAKE STL_PORT_INFO data */
			reset_virtual_port(port);
			/* FALL through */
		case IB_SMP_ATTR_PORT_INFO:
			ret = subn_set_portinfo(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_PKEY_TABLE:
			ret = subn_set_pkeytable(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_SL_TO_VL_TABLE:
			ret = subn_set_sl_to_vl(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_VL_ARB_TABLE:
			ret = subn_set_vl_arb(smp, ibdev, port);
			goto bail;
		case IB_SMP_ATTR_WFR_LITE_VIRT_LINK_INFO:
			ret = subn_set_stl_virt_link_info(smp, ibdev, port, 1);
			goto bail;
		case IB_SMP_ATTR_SM_INFO:
			if (ibp->port_cap_flags & IB_PORT_SM_DISABLED) {
				ret = IB_MAD_RESULT_SUCCESS |
					IB_MAD_RESULT_CONSUMED;
				goto bail;
			}
			if (ibp->port_cap_flags & IB_PORT_SM) {
				ret = IB_MAD_RESULT_SUCCESS;
				goto bail;
			}
			/* FALLTHROUGH */
		default:
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(smp);
			goto bail;
		}

	case IB_MGMT_METHOD_TRAP_REPRESS:
		if (smp->attr_id == IB_SMP_ATTR_NOTICE)
			ret = subn_trap_repress(smp, ibdev, port);
		else {
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply(smp);
		}
		goto bail;

	case IB_MGMT_METHOD_GET_RESP:
		switch (smp->attr_id) {
			case IB_SMP_ATTR_WFR_LITE_VIRT_LINK_INFO:
				ret = subn_set_stl_virt_link_info(smp, ibdev, port, 0);
				goto bail;
		}
		/* FALLTHROUGH */
	case IB_MGMT_METHOD_TRAP:
	case IB_MGMT_METHOD_REPORT:
	case IB_MGMT_METHOD_REPORT_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	case IB_MGMT_METHOD_SEND:
		if (ib_get_smp_direction(smp) &&
		    smp->attr_id == QIB_VENDOR_IPG) {
			ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PORT,
					      smp->data[0]);
			ret = IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		} else
			ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	default:
		smp->status |= IB_SMP_UNSUP_METHOD;
		ret = reply(smp);
	}

bail:
	return ret;
}

static int process_perf(struct ib_device *ibdev, u8 port,
			struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	struct ib_pma_mad *pmp = (struct ib_pma_mad *)out_mad;
	int ret;

	*out_mad = *in_mad;
	if (pmp->mad_hdr.class_version != 1) {
		pmp->mad_hdr.status |= IB_SMP_UNSUP_VERSION;
		ret = reply((struct ib_smp *) pmp);
		goto bail;
	}

	switch (pmp->mad_hdr.method) {
	case IB_MGMT_METHOD_GET:
		switch (pmp->mad_hdr.attr_id) {
		case IB_PMA_CLASS_PORT_INFO:
			ret = pma_get_classportinfo(pmp, ibdev);
			goto bail;
		case IB_PMA_PORT_SAMPLES_CONTROL:
			ret = pma_get_portsamplescontrol(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_SAMPLES_RESULT:
			ret = pma_get_portsamplesresult(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_SAMPLES_RESULT_EXT:
			ret = pma_get_portsamplesresult_ext(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS:
			ret = pma_get_portcounters(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS_EXT:
			ret = pma_get_portcounters_ext(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS_CONG:
			ret = pma_get_portcounters_cong(pmp, ibdev, port);
			goto bail;
		default:
			pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply((struct ib_smp *) pmp);
			goto bail;
		}

	case IB_MGMT_METHOD_SET:
		switch (pmp->mad_hdr.attr_id) {
		case IB_PMA_PORT_SAMPLES_CONTROL:
			ret = pma_set_portsamplescontrol(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS:
			ret = pma_set_portcounters(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS_EXT:
			ret = pma_set_portcounters_ext(pmp, ibdev, port);
			goto bail;
		case IB_PMA_PORT_COUNTERS_CONG:
			ret = pma_set_portcounters_cong(pmp, ibdev, port);
			goto bail;
		default:
			pmp->mad_hdr.status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply((struct ib_smp *) pmp);
			goto bail;
		}

	case IB_MGMT_METHOD_TRAP:
	case IB_MGMT_METHOD_GET_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	default:
		pmp->mad_hdr.status |= IB_SMP_UNSUP_METHOD;
		ret = reply((struct ib_smp *) pmp);
	}

bail:
	return ret;
}

static int cc_get_classportinfo(struct ib_cc_mad *ccp,
				struct ib_device *ibdev)
{
	struct ib_cc_classportinfo_attr *p =
		(struct ib_cc_classportinfo_attr *)ccp->mgmt_data;

	memset(ccp->mgmt_data, 0, sizeof(ccp->mgmt_data));

	p->base_version = 1;
	p->class_version = 1;
	p->cap_mask = 0;

	/*
	 * Expected response time is 4.096 usec. * 2^18 == 1.073741824 sec.
	 */
	p->resp_time_value = 18;

	return reply((struct ib_smp *) ccp);
}

static int cc_get_congestion_info(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_cc_info_attr *p =
		(struct ib_cc_info_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);

	memset(ccp->mgmt_data, 0, sizeof(ccp->mgmt_data));

	p->congestion_info = 0;
	p->control_table_cap = ppd->cc_max_table_entries;

	return reply((struct ib_smp *) ccp);
}

static int cc_get_congestion_setting(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	int i;
	struct ib_cc_congestion_setting_attr *p =
		(struct ib_cc_congestion_setting_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	struct ib_cc_congestion_entry_shadow *entries;

	memset(ccp->mgmt_data, 0, sizeof(ccp->mgmt_data));

	spin_lock(&ppd->cc_shadow_lock);

	entries = ppd->congestion_entries_shadow->entries;
	p->port_control = cpu_to_be16(
		ppd->congestion_entries_shadow->port_control);
	p->control_map = cpu_to_be16(
		ppd->congestion_entries_shadow->control_map);
	for (i = 0; i < IB_CC_CCS_ENTRIES; i++) {
		p->entries[i].ccti_increase = entries[i].ccti_increase;
		p->entries[i].ccti_timer = cpu_to_be16(entries[i].ccti_timer);
		p->entries[i].trigger_threshold = entries[i].trigger_threshold;
		p->entries[i].ccti_min = entries[i].ccti_min;
	}

	spin_unlock(&ppd->cc_shadow_lock);

	return reply((struct ib_smp *) ccp);
}

static int cc_get_congestion_control_table(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_cc_table_attr *p =
		(struct ib_cc_table_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 cct_block_index = be32_to_cpu(ccp->attr_mod);
	u32 max_cct_block;
	u32 cct_entry;
	struct ib_cc_table_entry_shadow *entries;
	int i;

	/* Is the table index more than what is supported? */
	if (cct_block_index > IB_CC_TABLE_CAP_DEFAULT - 1)
		goto bail;

	memset(ccp->mgmt_data, 0, sizeof(ccp->mgmt_data));

	spin_lock(&ppd->cc_shadow_lock);

	max_cct_block =
		(ppd->ccti_entries_shadow->ccti_last_entry + 1)/IB_CCT_ENTRIES;
	max_cct_block = max_cct_block ? max_cct_block - 1 : 0;

	if (cct_block_index > max_cct_block) {
		spin_unlock(&ppd->cc_shadow_lock);
		goto bail;
	}

	ccp->attr_mod = cpu_to_be32(cct_block_index);

	cct_entry = IB_CCT_ENTRIES * (cct_block_index + 1);

	cct_entry--;

	p->ccti_limit = cpu_to_be16(cct_entry);

	entries = &ppd->ccti_entries_shadow->
			entries[IB_CCT_ENTRIES * cct_block_index];
	cct_entry %= IB_CCT_ENTRIES;

	for (i = 0; i <= cct_entry; i++)
		p->ccti_entries[i].entry = cpu_to_be16(entries[i].entry);

	spin_unlock(&ppd->cc_shadow_lock);

	return reply((struct ib_smp *) ccp);

bail:
	return reply_failure((struct ib_smp *) ccp);
}

static int cc_set_congestion_setting(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_cc_congestion_setting_attr *p =
		(struct ib_cc_congestion_setting_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	int i;

	ppd->cc_sl_control_map = be16_to_cpu(p->control_map);

	for (i = 0; i < IB_CC_CCS_ENTRIES; i++) {
		ppd->congestion_entries[i].ccti_increase =
			p->entries[i].ccti_increase;

		ppd->congestion_entries[i].ccti_timer =
			be16_to_cpu(p->entries[i].ccti_timer);

		ppd->congestion_entries[i].trigger_threshold =
			p->entries[i].trigger_threshold;

		ppd->congestion_entries[i].ccti_min =
			p->entries[i].ccti_min;
	}

	return reply((struct ib_smp *) ccp);
}

static int cc_set_congestion_control_table(struct ib_cc_mad *ccp,
				struct ib_device *ibdev, u8 port)
{
	struct ib_cc_table_attr *p =
		(struct ib_cc_table_attr *)ccp->mgmt_data;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	u32 cct_block_index = be32_to_cpu(ccp->attr_mod);
	u32 cct_entry;
	struct ib_cc_table_entry_shadow *entries;
	int i;

	/* Is the table index more than what is supported? */
	if (cct_block_index > IB_CC_TABLE_CAP_DEFAULT - 1)
		goto bail;

	/* If this packet is the first in the sequence then
	 * zero the total table entry count.
	 */
	if (be16_to_cpu(p->ccti_limit) < IB_CCT_ENTRIES)
		ppd->total_cct_entry = 0;

	cct_entry = (be16_to_cpu(p->ccti_limit))%IB_CCT_ENTRIES;

	/* ccti_limit is 0 to 63 */
	ppd->total_cct_entry += (cct_entry + 1);

	if (ppd->total_cct_entry > ppd->cc_supported_table_entries)
		goto bail;

	ppd->ccti_limit = be16_to_cpu(p->ccti_limit);

	entries = ppd->ccti_entries + (IB_CCT_ENTRIES * cct_block_index);

	for (i = 0; i <= cct_entry; i++)
		entries[i].entry = be16_to_cpu(p->ccti_entries[i].entry);

	spin_lock(&ppd->cc_shadow_lock);

	ppd->ccti_entries_shadow->ccti_last_entry = ppd->total_cct_entry - 1;
	memcpy(ppd->ccti_entries_shadow->entries, ppd->ccti_entries,
		(ppd->total_cct_entry * sizeof(struct ib_cc_table_entry)));

	ppd->congestion_entries_shadow->port_control = IB_CC_CCS_PC_SL_BASED;
	ppd->congestion_entries_shadow->control_map = ppd->cc_sl_control_map;
	memcpy(ppd->congestion_entries_shadow->entries, ppd->congestion_entries,
		IB_CC_CCS_ENTRIES * sizeof(struct ib_cc_congestion_entry));

	spin_unlock(&ppd->cc_shadow_lock);

	return reply((struct ib_smp *) ccp);

bail:
	return reply_failure((struct ib_smp *) ccp);
}

static int check_cc_key(struct qib_ibport *ibp,
			struct ib_cc_mad *ccp, int mad_flags)
{
	return 0;
}

static int process_cc(struct ib_device *ibdev, int mad_flags,
			u8 port, struct ib_mad *in_mad,
			struct ib_mad *out_mad)
{
	struct ib_cc_mad *ccp = (struct ib_cc_mad *)out_mad;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	int ret;

	*out_mad = *in_mad;

	if (ccp->class_version != 2) {
		ccp->status |= IB_SMP_UNSUP_VERSION;
		ret = reply((struct ib_smp *)ccp);
		goto bail;
	}

	ret = check_cc_key(ibp, ccp, mad_flags);
	if (ret)
		goto bail;

	switch (ccp->method) {
	case IB_MGMT_METHOD_GET:
		switch (ccp->attr_id) {
		case IB_CC_ATTR_CLASSPORTINFO:
			ret = cc_get_classportinfo(ccp, ibdev);
			goto bail;

		case IB_CC_ATTR_CONGESTION_INFO:
			ret = cc_get_congestion_info(ccp, ibdev, port);
			goto bail;

		case IB_CC_ATTR_CA_CONGESTION_SETTING:
			ret = cc_get_congestion_setting(ccp, ibdev, port);
			goto bail;

		case IB_CC_ATTR_CONGESTION_CONTROL_TABLE:
			ret = cc_get_congestion_control_table(ccp, ibdev, port);
			goto bail;

			/* FALLTHROUGH */
		default:
			ccp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply((struct ib_smp *) ccp);
			goto bail;
		}

	case IB_MGMT_METHOD_SET:
		switch (ccp->attr_id) {
		case IB_CC_ATTR_CA_CONGESTION_SETTING:
			ret = cc_set_congestion_setting(ccp, ibdev, port);
			goto bail;

		case IB_CC_ATTR_CONGESTION_CONTROL_TABLE:
			ret = cc_set_congestion_control_table(ccp, ibdev, port);
			goto bail;

			/* FALLTHROUGH */
		default:
			ccp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply((struct ib_smp *) ccp);
			goto bail;
		}

	case IB_MGMT_METHOD_GET_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	case IB_MGMT_METHOD_TRAP:
	default:
		ccp->status |= IB_SMP_UNSUP_METHOD;
		ret = reply((struct ib_smp *) ccp);
	}

bail:
	return ret;
}

static int process_subn_stl(struct ib_device *ibdev, int mad_flags,
			u8 port, struct jumbo_mad *in_jumbo,
			struct jumbo_mad *out_jumbo)
{
	struct stl_smp *smp = (struct stl_smp *)out_jumbo;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);
	int ret;

	*out_jumbo = *in_jumbo;
	if (smp->class_version != STL_SMI_CLASS_VERSION) {
		smp->status |= IB_SMP_UNSUP_VERSION;
		ret = reply_stl(smp);
		goto bail;
	}

	ret = check_stl_mkey(ibp, smp, mad_flags);
	if (ret) {
#if 0
	/* For WFR we do _not_ support "cross" queries like this */
		u32 port_num = be32_to_cpu(smp->attr_mod);

		/*
		 * If this is a get/set portinfo, we already check the
		 * M_Key if the MAD is for another port and the M_Key
		 * is OK on the receiving port. This check is needed
		 * to increment the error counters when the M_Key
		 * fails to match on *both* ports.
		 */
		if (in_jumbo->mad_hdr.attr_id == IB_SMP_ATTR_PORT_INFO &&
		    (smp->method == IB_MGMT_METHOD_GET ||
		     smp->method == IB_MGMT_METHOD_SET) &&
		    port_num && port_num <= ibdev->phys_port_cnt &&
		    port != port_num)
			(void) check_mkey(to_iport(ibdev, port_num), smp, 0);
#endif
		ret = IB_MAD_RESULT_FAILURE;
		goto bail;
	}

/* FIXME add other STL SMP's as we go */
	switch (smp->method) {
	case IB_MGMT_METHOD_GET:
		switch (smp->attr_id) {
		case STL_ATTRIB_ID_NODE_DESCRIPTION:
			ret = subn_get_stl_nodedescription((struct stl_smp *)smp, ibdev);
			goto bail;
		case STL_ATTRIB_ID_NODE_INFO:
			ret = subn_get_stl_nodeinfo((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_PORT_INFO:
			ret = subn_get_stl_portinfo((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_PARTITION_TABLE:
			ret = subn_get_stl_pkeytable((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_SL_TO_SC_MAP:
			ret = subn_get_stl_sl_to_sc((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_SC_TO_SL_MAP:
			ret = subn_get_stl_sc_to_sl((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_SC_TO_VLT_MAP:
			ret = subn_get_stl_sc_to_vlt((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_SC_TO_VLNT_MAP:
			ret = subn_get_stl_sc_to_vlnt((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_VL_ARBITRATION:
			ret = subn_get_stl_vlarb((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_PORT_STATE_INFO:
			ret = subn_get_stl_psi((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_BUFFER_CONTROL_TABLE:
			ret = subn_get_stl_bct((struct stl_smp *)smp, ibdev, port);
			goto bail;
		default:
			printk(KERN_WARNING PFX
				"WARN: STL SubnGet(%x) not supported yet...\n",
				be16_to_cpu(smp->attr_id));
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply_stl(smp);
			goto bail;
		}
	case IB_MGMT_METHOD_SET:
		switch (smp->attr_id) {
		case STL_ATTRIB_ID_PORT_INFO:
			ret = subn_set_stl_portinfo((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_PARTITION_TABLE:
			ret = subn_set_stl_pkeytable((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_SL_TO_SC_MAP:
			ret = subn_set_stl_sl_to_sc((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_SC_TO_SL_MAP:
			ret = subn_set_stl_sc_to_sl((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_SC_TO_VLT_MAP:
			ret = subn_set_stl_sc_to_vlt((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_SC_TO_VLNT_MAP:
			ret = subn_set_stl_sc_to_vlnt((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_VL_ARBITRATION:
			ret = subn_set_stl_vlarb((struct stl_smp *)smp, ibdev, port);
			goto bail;
		case STL_ATTRIB_ID_BUFFER_CONTROL_TABLE:
			ret = subn_set_stl_bct((struct stl_smp *)smp, ibdev, port);
			goto bail;
		default:
			printk(KERN_WARNING PFX
				"WARN: STL SubnSet(%x) not supported yet...\n",
				be16_to_cpu(smp->attr_id));
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply_stl(smp);
			goto bail;
		}

	case IB_MGMT_METHOD_TRAP_REPRESS:
		if (smp->attr_id == IB_SMP_ATTR_NOTICE)
			ret = subn_trap_repress((struct ib_smp *)smp, ibdev, port);
		else {
			smp->status |= IB_SMP_UNSUP_METH_ATTR;
			ret = reply_stl(smp);
		}
		goto bail;

	case IB_MGMT_METHOD_TRAP:
	case IB_MGMT_METHOD_REPORT:
	case IB_MGMT_METHOD_REPORT_RESP:
	case IB_MGMT_METHOD_GET_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	case IB_MGMT_METHOD_SEND:
		if (stl_get_smp_direction(smp) &&
		    smp->attr_id == QIB_VENDOR_IPG) {
			u32 data;
			if (in_jumbo->mad_hdr.mgmt_class ==
			    IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE)
				data = smp->route.dr.data[0];
			else
				data = smp->route.lid.data[0];
			ppd->dd->f_set_ib_cfg(ppd, QIB_IB_CFG_PORT, data);
			ret = IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		} else
			ret = IB_MAD_RESULT_SUCCESS;
		goto bail;

	default:
		printk(KERN_WARNING PFX
			"WARN: Subn method %x not supported yet...\n",
			smp->method);
		smp->status |= IB_SMP_UNSUP_METHOD;
		ret = reply_stl(smp);
	}
bail:
	return ret;
}

static int is_sma_mad(struct jumbo_mad *mad)
{
	struct stl_smp *smp = (struct stl_smp *)mad;

	switch (mad->mad_hdr.method) {
	case IB_MGMT_METHOD_GET:
	case IB_MGMT_METHOD_SET:
	case IB_MGMT_METHOD_TRAP_REPRESS:
		return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
	case IB_MGMT_METHOD_TRAP:
	case IB_MGMT_METHOD_REPORT:
	case IB_MGMT_METHOD_REPORT_RESP:
	case IB_MGMT_METHOD_GET_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		return IB_MAD_RESULT_SUCCESS;

	case IB_MGMT_METHOD_SEND:
		if (stl_get_smp_direction(smp) &&
		    smp->attr_id == QIB_VENDOR_IPG) {
			return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;
		} else
			return IB_MAD_RESULT_SUCCESS;
	}

	return 0;
}

static int is_pma_mad(struct jumbo_mad *mad)
{
	switch (mad->mad_hdr.method) {
	case IB_MGMT_METHOD_GET:
	case IB_MGMT_METHOD_SET:
		return IB_MAD_RESULT_SUCCESS | IB_MAD_RESULT_CONSUMED;

	case IB_MGMT_METHOD_TRAP:
	case IB_MGMT_METHOD_GET_RESP:
		/*
		 * The ib_mad module will call us to process responses
		 * before checking for other consumers.
		 * Just tell the caller to process it normally.
		 */
		return IB_MAD_RESULT_SUCCESS;
	}

	return 0;
}

static int is_sma_pma_mad(struct jumbo_mad *mad)
{
	switch (mad->mad_hdr.mgmt_class) {
	case IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE:
	case IB_MGMT_CLASS_SUBN_LID_ROUTED:
		return (is_sma_mad(mad));
	case IB_MGMT_CLASS_PERF_MGMT:
		return (is_pma_mad(mad));
	}
	return (0);
}

/*
 * This check is designed to control access to our local SMA.
 *
 * Full member pkey always works.
 *
 * Limited member access is only ok if we we don't have a full member pkey in
 * our table.  Full member pkey availability is cached within the pkey table
 * set function to make this check faster.
 */
static int invalid_mad_pkey(struct qib_ibport *ibp, u8 port, struct jumbo_mad *mad,
			  struct ib_wc *in_wc)
{
	int ret;
	u16 pkey = virtual_stl[port-1].pkeys[in_wc->pkey_index];

	/* WFR-lite needs to pass IB SMP's without harassment */
	if (mad->mad_hdr.base_version != JUMBO_MGMT_BASE_VERSION &&
	    (mad->mad_hdr.mgmt_class == IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE ||
	     mad->mad_hdr.mgmt_class == IB_MGMT_CLASS_SUBN_LID_ROUTED)) {
		printk(KERN_WARNING PFX
			"MAD PKey check IB OK : "
			"pkey == 0x%x; BaseVer 0x%x; MgmtClassVer 0x%x "
			"MgmtClass 0x%x; AttrID 0x%x; Method 0x%x\n",
			pkey,
			mad->mad_hdr.base_version,
			mad->mad_hdr.class_version,
			mad->mad_hdr.mgmt_class,
			be16_to_cpu(mad->mad_hdr.attr_id),
			mad->mad_hdr.method);
		return 0;
	}

	if (pkey == WFR_FULL_MGMT_P_KEY ||
	   (pkey == WFR_LIM_MGMT_P_KEY && !virtual_stl[port-1].member_full_mgmt))
		return 0;

	/* Because the MAD stack passes all MADs through the driver for
	 * processing we have to make sure this MAD is destined for us before
	 * rejecting it.  Otherwise we might reject MAD's destined for user
	 * space.
	 */
	ret = is_sma_pma_mad(mad);
	if (ret & IB_MAD_RESULT_CONSUMED) {
		printk(KERN_ERR PFX
			"ERROR: MAD PKey check failed limited sent to full member! "
			"pkey == 0x%x; slid %d; dlid %d; BaseVer 0x%x; MgmtClassVer 0x%x "
			"MgmtClass 0x%x; AttrID 0x%x; Method 0x%x\n",
			pkey,
			in_wc->slid,
			cpu_to_be16(ppd_from_ibp(ibp)->lid),
			mad->mad_hdr.base_version, mad->mad_hdr.class_version,
			mad->mad_hdr.mgmt_class,
			be16_to_cpu(mad->mad_hdr.attr_id), mad->mad_hdr.method);
		if  (wfr_enforce_mgmt_pkey) {
			printk(KERN_ERR PFX "... dropping MAD pkt\n");
			qib_bad_pqkey(ibp, IB_NOTICE_TRAP_BAD_PKEY,
				      pkey, in_wc->sl,
				      in_wc->src_qp, in_wc->qp->qp_num,
				      in_wc->slid,
				      cpu_to_be16(ppd_from_ibp(ibp)->lid));
			return ret;
		}
	}
	return 0;
}

int wfr_process_jumbo_mad(struct ib_device *ibdev, int mad_flags, u8 port,
			  struct ib_wc *in_wc, struct ib_grh *in_grh,
			  struct jumbo_mad *in_jumbo,
			  struct jumbo_mad *out_jumbo)
{
	int ret;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	int pkey_idx;

	if ((ret = invalid_mad_pkey(ibp, port, in_jumbo, in_wc)) != 0) {
		return ret;
	}

	/* always respond with limited PKey */
	pkey_idx = wfr_lookup_pkey_idx(ibp, WFR_LIM_MGMT_P_KEY);
	if (pkey_idx < 0) {
		printk(KERN_WARNING PFX "failed to find a valid pkey_index "
			"to return defaulting to 1: 0x%x\n",
			qib_get_pkey(ibp, 1));
		pkey_idx = 1;
	}
	in_wc->pkey_index = (u16)pkey_idx;

	if (wfr_dump_sma_mads) {
		printk(KERN_WARNING PFX "Recv: %lu byte mad.\n",
		       sizeof(*in_jumbo));
		dump_mad((uint8_t *)in_jumbo, sizeof(*in_jumbo));
	}

	switch (in_jumbo->mad_hdr.mgmt_class) {
	case IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE:
	case IB_MGMT_CLASS_SUBN_LID_ROUTED:
		ret = process_subn_stl(ibdev, mad_flags, port, in_jumbo, out_jumbo);
		goto bail;

	/* Only process SMP's right now */
	case IB_MGMT_CLASS_PERF_MGMT:
		ret = process_stl_perf(ibdev, port, in_jumbo, out_jumbo);
		goto bail;
#if 0
	case IB_MGMT_CLASS_CONG_MGMT:
		if (!ppd->congestion_entries_shadow ||
			 !qib_cc_table_size) {
			ret = IB_MAD_RESULT_SUCCESS;
			goto bail;
		}
		ret = process_cc(ibdev, mad_flags, port, in_jumbo, out_jumbo);
		goto bail;
#else
	case IB_MGMT_CLASS_CONG_MGMT:
		printk(KERN_WARNING PFX
			"WARN: mgmt_class %x not supported yet...\n",
			in_jumbo->mad_hdr.mgmt_class);
		/* fall through */
#endif

	default:
		ret = IB_MAD_RESULT_SUCCESS;
	}

bail:
	if (wfr_dump_sma_mads && (ret & IB_MAD_RESULT_REPLY)) {
		printk(KERN_WARNING PFX "Reply: %lu byte mad.\n",
		       sizeof(*out_jumbo));
		dump_mad((uint8_t *)out_jumbo, sizeof(*out_jumbo));
	}
	return ret;
}

/**
 * wfr_process_mad - process an incoming MAD packet
 * @ibdev: the infiniband device this packet came in on
 * @mad_flags: MAD flags
 * @port: the port number this packet came in on
 * @in_wc: the work completion entry for this packet
 * @in_grh: the global route header for this packet
 * @in_mad: the incoming MAD
 * @out_mad: any outgoing MAD reply
 *
 * Returns IB_MAD_RESULT_SUCCESS if this is a MAD that we are not
 * interested in processing.
 *
 * Note that the verbs framework has already done the MAD sanity checks,
 * and hop count/pointer updating for IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE
 * MADs.
 *
 * This is called by the ib_mad module.
 */
int wfr_process_mad(struct ib_device *ibdev, int mad_flags, u8 port,
		    struct ib_wc *in_wc, struct ib_grh *in_grh,
		    /* NOTE: for compatiblity with the core layer these are
		     * declared as ib_mad
		     * However, the true type is jumbo_mad for the response
		     * (out mad) and "possibly" jumbo_mad for the in mad (based
		     * off of base_version)
		     */
		    struct ib_mad *in_mad, struct ib_mad *out_mad)
{
	int ret;
	struct qib_ibport *ibp = to_iport(ibdev, port);
	struct qib_pportdata *ppd = ppd_from_ibp(ibp);

	if (!virtual_stl_init) {
		init_virtual_stl(ibdev, port);
	}

	if (in_mad->mad_hdr.base_version == IB_MGMT_BASE_VERSION &&
	    ((in_mad->mad_hdr.mgmt_class != IB_MGMT_CLASS_SUBN_LID_ROUTED &&
	     in_mad->mad_hdr.mgmt_class != IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE) ||
	     in_mad->mad_hdr.attr_id != IB_SMP_ATTR_NODE_INFO)) {
		if (!wfr_allow_ib_mads) {
			printk(KERN_WARNING PFX "Invalid SMP sent : "
				"WFR-Lite is set to only process STL SMP's\n");
			return (IB_MAD_RESULT_FAILURE);
		} else if (wfr_warn_ib_mads) {
			printk(KERN_WARNING PFX "WARNING: "
				"Processing IB MAD: class 0x%x attr 0x%x\n",
				in_mad->mad_hdr.mgmt_class,
				in_mad->mad_hdr.attr_id);
		}
	}

	if (in_mad->mad_hdr.base_version == JUMBO_MGMT_BASE_VERSION)
		return (wfr_process_jumbo_mad(ibdev, mad_flags, port, in_wc, in_grh,
	                   (struct jumbo_mad *)in_mad,
	                   (struct jumbo_mad *)out_mad));

	switch (in_mad->mad_hdr.mgmt_class) {
	case IB_MGMT_CLASS_SUBN_DIRECTED_ROUTE:
	case IB_MGMT_CLASS_SUBN_LID_ROUTED:
		ret = process_subn(ibdev, mad_flags, port, in_mad, out_mad);
		goto bail;

	case IB_MGMT_CLASS_PERF_MGMT:
		ret = process_perf(ibdev, port, in_mad, out_mad);
		goto bail;

	case IB_MGMT_CLASS_CONG_MGMT:
		if (!ppd->congestion_entries_shadow ||
			 !qib_cc_table_size) {
			ret = IB_MAD_RESULT_SUCCESS;
			goto bail;
		}
		ret = process_cc(ibdev, mad_flags, port, in_mad, out_mad);
		goto bail;

	default:
		ret = IB_MAD_RESULT_SUCCESS;
	}

bail:
	return ret;
}

static void send_handler(struct ib_mad_agent *agent,
			 struct ib_mad_send_wc *mad_send_wc)
{
	ib_free_send_mad(mad_send_wc->send_buf);
}

static void xmit_wait_timer_func(unsigned long opaque)
{
	struct qib_pportdata *ppd = (struct qib_pportdata *)opaque;
	struct qib_devdata *dd = dd_from_ppd(ppd);
	unsigned long flags;
	u8 status;

	spin_lock_irqsave(&ppd->ibport_data.lock, flags);
	if (ppd->cong_stats.flags == IB_PMA_CONG_HW_CONTROL_SAMPLE) {
		status = dd->f_portcntr(ppd, QIBPORTCNTR_PSSTAT);
		if (status == IB_PMA_SAMPLE_STATUS_DONE) {
			/* save counter cache */
			cache_hw_sample_counters(ppd);
			ppd->cong_stats.flags = IB_PMA_CONG_HW_CONTROL_TIMER;
		} else
			goto done;
	}
	ppd->cong_stats.counter = xmit_wait_get_value_delta(ppd);
	dd->f_set_cntr_sample(ppd, QIB_CONG_TIMER_PSINTERVAL, 0x0);
done:
	spin_unlock_irqrestore(&ppd->ibport_data.lock, flags);
	mod_timer(&ppd->cong_stats.timer, jiffies + HZ);
}

int qib_create_agents(struct qib_ibdev *dev)
{
	struct qib_devdata *dd = dd_from_dev(dev);
	struct ib_mad_agent *agent;
	struct qib_ibport *ibp;
	int p;
	int ret;

	for (p = 0; p < dd->num_pports; p++) {
		ibp = &dd->pport[p].ibport_data;
		agent = ib_register_mad_agent(&dev->ibdev, p + 1, IB_QPT_SMI,
					      NULL, 0, send_handler,
					      NULL, NULL);
		if (IS_ERR(agent)) {
			ret = PTR_ERR(agent);
			goto err;
		}

		/* Initialize xmit_wait structure */
		dd->pport[p].cong_stats.counter = 0;
		init_timer(&dd->pport[p].cong_stats.timer);
		dd->pport[p].cong_stats.timer.function = xmit_wait_timer_func;
		dd->pport[p].cong_stats.timer.data =
			(unsigned long)(&dd->pport[p]);
		dd->pport[p].cong_stats.timer.expires = 0;
		add_timer(&dd->pport[p].cong_stats.timer);

		ibp->send_agent = agent;
	}

	return 0;

err:
	for (p = 0; p < dd->num_pports; p++) {
		ibp = &dd->pport[p].ibport_data;
		if (ibp->send_agent) {
			agent = ibp->send_agent;
			ibp->send_agent = NULL;
			ib_unregister_mad_agent(agent);
		}
	}

	return ret;
}

void qib_free_agents(struct qib_ibdev *dev)
{
	struct qib_devdata *dd = dd_from_dev(dev);
	struct ib_mad_agent *agent;
	struct qib_ibport *ibp;
	int p;

	for (p = 0; p < dd->num_pports; p++) {
		ibp = &dd->pport[p].ibport_data;
		if (ibp->send_agent) {
			agent = ibp->send_agent;
			ibp->send_agent = NULL;
			ib_unregister_mad_agent(agent);
		}
		if (ibp->sm_ah) {
			ib_destroy_ah(&ibp->sm_ah->ibah);
			ibp->sm_ah = NULL;
		}
		if (ibp->dr_ah) {
			ib_destroy_ah(ibp->dr_ah);
			ibp->dr_ah = NULL;
		}
		if (dd->pport[p].cong_stats.timer.data)
			del_timer_sync(&dd->pport[p].cong_stats.timer);
	}
}
