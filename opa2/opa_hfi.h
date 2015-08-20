/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 * Copyright (c) 2014 - 2015 Intel Corporation.
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
 *
 * Intel(R) Omni-Path Gen2 HFI PCIe Driver
 */

#ifndef _OPA_HFI_H
#define _OPA_HFI_H

#include <linux/pci.h>
#include <linux/slab.h>
#include <rdma/hfi_cmd.h>
#include <rdma/opa_core.h>
#include <rdma/fxr/fxr_linkmux_defs.h>

extern unsigned int hfi_max_mtu;
#define DRIVER_NAME		KBUILD_MODNAME
#define DRIVER_CLASS_NAME	DRIVER_NAME

enum {
	/* Port type is undefined */
	HFI_PORT_TYPE_UNKNOWN = 0,
	/* port is not currently usable, CableInfo not available */
	HFI_PORT_TYPE_DISCONNECTED = 1,
	/* A fixed backplane port in a director class switch. All HFI ASICS */
	HFI_PORT_TYPE_FIXED = 2,
	/* A backplane port in a blade system, possibly mixed configuration */
	HFI_PORT_TYPE_VARIABLE = 3,
	/* Implies a SFF-8636 defined format for CableInfo (QSFP) */
	HFI_PORT_TYPE_STANDARD = 4,
	/*
	 * Intel Silicon Photonics x16 mid-board module connector is
	 * integrated onto system board
	 */
	HFI_PORT_TYPE_SI_PHOTONICS = 5,
	/* 6 - 15 are reserved */
};

#define HFI_NUM_BARS		2
#define HFI_NUM_PPORTS		2
#define HFI_MAX_VLS		32
#define HFI_PID_SYSTEM		0
#define HFI_PID_BYPASS_BASE	0xF00

/* FXRTODO: based on 16bit (9B) LID */
#define HFI_MULTICAST_LID_BASE	0xC000
/* Maximum number of traffic classes supported */
#define HFI_MAX_TC		4

/* Maximum number of unicast LIDs supported */
#define HFI_MAX_LID_SUPP	(0xBFFF)

/*
 * Size of packet sequence number state assuming LMC = 0
 * The same PSN buffer is used for both TX and RX and hence
 * the multiplication by 2
 */
#define HFI_PSN_SIZE		(2 * HFI_MAX_LID_SUPP * 8)

/* TX timeout for E2E control messages */
#define HFI_TX_TIMEOUT_MS	100

/* use this MTU size if none other is given */
#define HFI_DEFAULT_ACTIVE_MTU 10240
/* use this MTU size as the default maximum */
#define HFI_DEFAULT_MAX_MTU	10240
/* The largest MAD packet size. */
#define HFI_MIN_VL_15_MTU		2048
/* Number of Data VLs supported */
#define HFI_NUM_DATA_VLS	8
/* Number of PKey entries in the HW */
#define HFI_MAX_PKEYS		LM_PKEY_ARRAY_SIZE

/* In accordance with stl vol 1 section 4.1 */
#define PGUID_MASK		(~(0x3UL << 32))
#define PORT_GUID(ng, pn)	(((be64_to_cpu(ng)) & PGUID_MASK) |\
				 (((u64)pn) << 32))

/* FXRTODO: Harcoded for now. Fix this once MNH reg is available */
#define NODE_GUID		0x11750101000000UL

#define pidx_to_pnum(id)	((id) + 1)
#define pnum_to_pidx(pn)	((pn) - 1)

#define HFI_PKEY_CAM_SHIFT		0
#define HFI_PKEY_CAM_MASK		0x7fff
#define HFI_PKEY_CAM(pkey)		(((pkey) >> HFI_PKEY_CAM_SHIFT) & \
					HFI_PKEY_CAM_MASK)

#define HFI_PKEY_MEMBER_SHIFT		15
#define HFI_PKEY_MEMBER_MASK		0x1
#define HFI_PKEY_MEMBER_TYPE(pkey)	(((pkey) >> HFI_PKEY_MEMBER_SHIFT) & \
					HFI_PKEY_MEMBER_MASK)

#define HFI_SL_TO_MC_SHIFT	FXR_TXCID_CFG_SL0_TO_TC_SL0_P0_MC_SHIFT
#define HFI_SL_TO_MC_MASK	FXR_TXCID_CFG_SL0_TO_TC_SL0_P0_MC_MASK
#define HFI_SL_TO_TC_SHIFT	FXR_TXCID_CFG_SL0_TO_TC_SL0_P0_TC_SHIFT
#define HFI_SL_TO_TC_MASK	FXR_TXCID_CFG_SL0_TO_TC_SL0_P0_TC_MASK

/*
 * HFI or Host Link States
 *
 * These describe the states the driver thinks the logical and physical
 * states are in.  Used as an argument to set_link_state().  Implemented
 * as bits for easy multi-state checking.  The actual state can only be
 * one.
 *
 * FXRTODO: STL2 related states need to be added
 * Ganged, ShallowSleep and DeepSleep.
 */
#define __HLS_UP_INIT_BP	0
#define __HLS_UP_ARMED_BP	1
#define __HLS_UP_ACTIVE_BP	2
#define __HLS_DN_DOWNDEF_BP	3	/* link down default */
#define __HLS_DN_POLL_BP	4
#define __HLS_DN_DISABLE_BP	5
#define __HLS_DN_OFFLINE_BP	6
#define __HLS_VERIFY_CAP_BP	7
#define __HLS_GOING_UP_BP	8
#define __HLS_GOING_OFFLINE_BP  9
#define __HLS_LINK_COOLDOWN_BP 10

#define HLS_UP_INIT	  (1 << __HLS_UP_INIT_BP)
#define HLS_UP_ARMED	  (1 << __HLS_UP_ARMED_BP)
#define HLS_UP_ACTIVE	  (1 << __HLS_UP_ACTIVE_BP)
#define HLS_DN_DOWNDEF	  (1 << __HLS_DN_DOWNDEF_BP) /* link down default */
#define HLS_DN_POLL	  (1 << __HLS_DN_POLL_BP)
#define HLS_DN_DISABLE	  (1 << __HLS_DN_DISABLE_BP)
#define HLS_DN_OFFLINE	  (1 << __HLS_DN_OFFLINE_BP)
#define HLS_VERIFY_CAP	  (1 << __HLS_VERIFY_CAP_BP)
#define HLS_GOING_UP	  (1 << __HLS_GOING_UP_BP)
#define HLS_GOING_OFFLINE (1 << __HLS_GOING_OFFLINE_BP)
#define HLS_LINK_COOLDOWN (1 << __HLS_LINK_COOLDOWN_BP)

#define HLS_UP (HLS_UP_INIT | HLS_UP_ARMED | HLS_UP_ACTIVE)

/* SMA idle flit payload commands */
#define SMA_IDLE_ARM	1
#define SMA_IDLE_ACTIVE 2

enum {
	HFI_IB_CFG_LIDLMC = 0,		/* LID and Mask*/
	HFI_IB_CFG_LWID_DG_ENB,		/* allowed Link-width downgrade */
	HFI_IB_CFG_LWID_ENB,		/* allowed Link-width */
	HFI_IB_CFG_LWID,		/* currently active Link-width */
	HFI_IB_CFG_SPD_ENB,		/* allowed Link speeds */
	HFI_IB_CFG_SPD,			/* current Link spd */
	HFI_IB_CFG_OP_VLS,		/* operational VLs */
	HFI_IB_CFG_PKEYS,		/* update partition keys */
	HFI_IB_CFG_MTU,			/* update MTU in IBC */
	HFI_IB_CFG_VL_HIGH_LIMIT,	/* Change VL high limit */
	HFI_IB_CFG_SL_TO_SC,		/* Change SLtoSC mapping */
	HFI_IB_CFG_SC_TO_SL,		/* Change SCtoSL mapping */
};

/* verify capability fabric CRC size bits */
enum {
	HFI_CAP_CRC_14B = (1 << 0), /* 14b CRC */
	HFI_CAP_CRC_48B = (1 << 1), /* 48b CRC */
	HFI_CAP_CRC_12B_16B_PER_LANE = (1 << 2) /* 12b-16b per lane CRC */
};

#define HFI_SUPPORTED_CRCS (HFI_CAP_CRC_14B | HFI_CAP_CRC_48B)

/* LCB_CFG_CRC_MODE TX_VAL and RX_VAL CRC mode values */
enum {
	HFI_LCB_CRC_16B = 0,	/* 16b CRC */
	HFI_LCB_CRC_14B = 1,	/* 14b CRC */
	HFI_LCB_CRC_48B = 2,	/* 48b CRC */
	HFI_LCB_CRC_12B_16B_PER_LANE = 3	/* 12b-16b per lane CRC */
};

/* the following enum is (almost) a copy/paste of the definition
 * in the OPA spec, section 20.2.2.6.8 (PortInfo) */
enum {
	OPA_PORT_LTP_CRC_MODE_NONE = 0,
	OPA_PORT_LTP_CRC_MODE_14 = 1, /* 14-bit LTP CRC mode (optional) */
	OPA_PORT_LTP_CRC_MODE_16 = 2, /* 16-bit LTP CRC mode */
	/* 48-bit overlapping LTP CRC mode (optional) */
	OPA_PORT_LTP_CRC_MODE_48 = 4,
	/* 12 to 16 bit per lane LTP CRC mode (optional) */
	OPA_PORT_LTP_CRC_MODE_PER_LANE = 8
};

#define HFI_LTP_CRC_SUPPORTED_SHIFT	8
#define HFI_LTP_CRC_SUPPORTED_MASK	0xf
#define HFI_LTP_CRC_SUPPORTED(val)	(((val) >> \
					HFI_LTP_CRC_SUPPORTED_SHIFT) & \
					HFI_LTP_CRC_SUPPORTED_MASK)

#define HFI_LTP_CRC_ENABLED_SHIFT	4
#define HFI_LTP_CRC_ENABLED_MASK	0xf
#define HFI_LTP_CRC_ENABLED(val)	(((val) >> \
					HFI_LTP_CRC_ENABLED_SHIFT) & \
					HFI_LTP_CRC_ENABLED_MASK)

#define HFI_LTP_CRC_ACTIVE_SHIFT	0
#define HFI_LTP_CRC_ACTIVE_MASK		0xf
#define HFI_LTP_CRC_ACTIVE(val)		(((val) >> \
					HFI_LTP_CRC_ENABLED_SHIFT) & \
					HFI_LTP_CRC_ACTIVE_MASK)

/*
 * FXRTODO: VL ARB is absent for FXR. Remove these
 * once we have STL2 specific opafm
 */
#define HFI_VL_ARB_TABLE_SIZE 		16
#define OPA_VLARB_LOW_ELEMENTS		0
#define OPA_VLARB_HIGH_ELEMENTS		1
#define OPA_VLARB_PREEMPT_ELEMENTS	2
#define OPA_VLARB_PREEMPT_MATRIX	3

struct hfi_link_down_reason {
	/*
	 * SMA-facing value.  Should be set from .latest when
	 * HLS_UP_* -> HLS_DN_* transition actually occurs.
	 */
	u8 sma;
	u8 latest;
};

struct hfi_msix_entry {
	struct msix_entry msix;
	void *arg;
	cpumask_var_t mask;
	struct list_head irq_wait_head;
	rwlock_t irq_wait_lock;
	struct hfi_devdata *dd;
	int intr_src;
};

struct hfi_event_queue {
	struct list_head irq_wait_chain;
	wait_queue_head_t wq;
	u32 irq_vector;
};

/*
 * hfi_ptcdata - HFI traffic class specific information per port
 * @psn_base: packet sequence number buffer used for TX/RX
 * @e2e_state_cache: E2E connection state cache
 * @max_e2e_dlid: Maximum DLID to which an E2E connection has been
 *	established which is used to detect the DLID till which
 *	destroy messages have to be sent during driver unload
 */
struct hfi_ptcdata {
	void *psn_base;
	struct ida e2e_state_cache;
	u32 max_e2e_dlid;
};

/*
 * FXRTODO: VL ARB is absent for FXR. Remove these
 * once we have STL2 specific opafm
 */
struct ib_vl_weight_elem {
	u8      vl;     /* Only low 4 bits, upper 4 bits reserved */
	u8      weight;
};

/*
 * hfi_pportdata - HFI port specific information
 *
 * @dd: pointer to the per node hfi_devdata
 * @pguid: port_guid identifying port
 * @neighbor_guid: Node guid of the neighboring port
 * @lid: LID for this port
 * @ptc: per traffic class specific fields
 * @sm_lid: LID of the SM
 * @lstate: Logical link state
 * @ibmtu: The MTU programmed for this port
 * @port_error_action: contains bit mask for various errors. The HFI
 *	should initiate link-bounce when the corresponding error occurs.
 * @link_width_supported: Supported link width
 * @link_width_downgrade_supported: Supported link width downgrading
 * @link_speed_supported: Supported link speed
 * @link_width_enabled: Enabled link width
 * @link_width_downgrade_enabled: Enabled link width downgrading
 * @link_speed_enabled: Enabled link speed
 * @link_width_active: Currently active link width
 * @link_width_downgrade_tx_active: Current TX side link width downgrading
 * @link_width_downgrade_rx_active: Current RX side link width downgrading
 * @link_speed_active: Current active link speed
 * @port_ltp_crc_mode: Supported, enabled and active LTP CRC modes
 * @port_crc_mode_enabled: CRC mode to be enabled
 * @smsl: Service level to use for SM
 * @lmc: LID mask control
 * @pnum: port number of this port
 * @vls_supported: Virtual lane supported
 * @vls_operational: Virtual lane operational
 * @vl_high_limit: Limit of high priority compenent of
 *	VL Arbitration table
 * @neighbor_type: Node type of neighboring port
 *			0 - HFI
 *			1 - Switch
 * @neighbor_normal: State of neighbor's port's Logical link state
 *	0 - Neighbor Down/Init
 *	1 - Neighbhor LinkArmed/LinkActive
 * @neighbor_fm_security: Configuration of a switch pin which can be used to
 *	disable firmware authentication during switch ASIC boot.
 *	0 - Authenticated
 *	1 - Authentication bypassed
 * @neighbor_port_number: Port number of the neighboring port
 * @is_sm_config_started: indicates that FM has started configuration for this
 *	port.
 * @offline_disabled_reason: offline reason
 * @is_active_optimize_enabled: if enabled, then LinkArmed -> LinkActive state
 *	change propogates this event to neighboring port via SMA Idle Flit
 *	messages
 * @mgmt_allowed: Indicates if neighbor is allowing this node to be a mgmt node
 *	(information received via LNI)
 * @sl_to_sc: service level to service class mapping table
 * @sc_to_sl: service class to service level mapping table
 *@local_link_down_reason: Reason why this port transitioned to link down
 *@local_link_down_reason: Reason why the neighboring port transitioned to
 *	link down
 *@remote_link_down_reason: Value to be sent to link peer on LinkDown
 */
struct hfi_pportdata {
	struct hfi_devdata *dd;
	__be64 pguid;
	__be64 neighbor_guid;
	u32 lid;
	u32 sm_lid;
	struct hfi_ptcdata ptc[HFI_MAX_TC];
	/* host link state which keeps both Physical Port and Logical Link
	   state by having HLS_* */
	struct mutex hls_lock;
	u32 host_link_state;

	u32 lstate;
	u32 ibmtu;
	u32 port_error_action;
	u16 pkeys[HFI_MAX_PKEYS];
	u16 link_width_supported;
	u16 link_width_downgrade_supported;
	u16 link_speed_supported;
	u16 link_width_enabled;
	u16 link_width_downgrade_enabled;
	u16 link_speed_enabled;
	u16 link_width_active;
	u16 link_width_downgrade_tx_active;
	u16 link_width_downgrade_rx_active;
	u16 link_speed_active;
	u16 port_ltp_crc_mode;
	u8 port_crc_mode_enabled;
	u8 smsl;
	u8 lmc;
	u8 pnum;
	u8 vls_supported;
	u8 vls_operational;
	u8 vl_high_limit;
	u8 neighbor_type;
	u8 neighbor_normal;
	u8 neighbor_fm_security;
	u8 neighbor_port_number;
	u8 is_sm_config_started;
	u8 offline_disabled_reason;
	u8 is_active_optimize_enabled;
	u8 mgmt_allowed;
	u8 sl_to_sc[OPA_MAX_SLS];
	u8 sc_to_sl[OPA_MAX_SCS];
	struct hfi_link_down_reason local_link_down_reason;
	struct hfi_link_down_reason neigh_link_down_reason;
	u8 remote_link_down_reason;

	/*
	 * FXRTODO: VL ARB is absent for FXR. Remove these
	 * once we have STL2 specific opafm
	 */
	struct ib_vl_weight_elem vl_arb_low[HFI_VL_ARB_TABLE_SIZE];
	struct ib_vl_weight_elem vl_arb_high[HFI_VL_ARB_TABLE_SIZE];
	struct ib_vl_weight_elem vl_arb_prempt_ele[HFI_VL_ARB_TABLE_SIZE];
	struct ib_vl_weight_elem vl_arb_prempt_mat[HFI_VL_ARB_TABLE_SIZE];
#ifdef CONFIG_DEBUG_FS
	struct dentry *hfi_port_dbg;
#endif
};

/* device data struct contains only per-HFI info. */
struct hfi_devdata {
	/* pci access data structure */
	struct pci_dev *pcidev;
	struct opa_core_device *bus_dev;

	/* localbus width (1, 2,4,8,16,32) from config space  */
	u32 lbus_width;
	/* localbus speed in MHz */
	u32 lbus_speed;
	int unit; /* unit # of this chip */
	int node; /* home node of this chip */
	u32 pcibar[HFI_NUM_BARS];
	/* mem-mapped pointer to base of chip regs */
	u8 __iomem *kregbase[HFI_NUM_BARS];
	u8 __iomem *kregend[HFI_NUM_BARS];
	/* physical address of chip for io_remap, etc. */
	resource_size_t physaddr;
	/* Per VL data. Enough for all VLs but not all elements are set/used.*/
	u16 vl_mtu[OPA_MAX_VLS];

	/* MSI-X information */
	struct hfi_msix_entry *msix_entries;
	u32 num_msix_entries;
	u32 num_eq_irqs;
	atomic_t msix_eq_next;

	/* Device Portals State */
	struct idr ptl_user;
	unsigned long ptl_map[HFI_NUM_PIDS / BITS_PER_LONG];
	spinlock_t ptl_lock;
	struct hfi_ctx priv_ctx;
	struct hfi_cq priv_tx_cq;
	struct hfi_cq priv_rx_cq;

	/* Command Queue State */
	struct idr cq_pair;
	spinlock_t cq_lock;
	void *cq_tx_base;
	void *cq_rx_base;
	void *cq_head_base;
	size_t cq_head_size;

	/* IOMMU */
	void *iommu_excontext_tbl;
	void *iommu_pasid_tbl;

	/* node_guid identifying node */
	__be64 nguid;

	/* Number of physical ports available */
	u8 num_pports;

	/*
	 * hfi_pportdata, points to array of port
	 * port-specifix data structs
	 */
	struct hfi_pportdata *pport;

	/* OUI comes from the HW. Used everywhere as 3 separate bytes. */
	u8 oui[3];

	/* Lock to synchronize access to tx_priv_cq */
	spinlock_t priv_tx_cq_lock;

	/* Lock to synchronize access to rx_priv_cq */
	spinlock_t priv_rx_cq_lock;

	/* Mutex lock synchronizing E2E operations */
	struct mutex e2e_lock;
#ifdef CONFIG_DEBUG_FS
	/* per HFI debugfs */
	struct dentry *hfi_dev_dbg;
	/* per HFI symlinks to above */
	struct dentry *hfi_dev_link;
#endif
};

/* return the driver's idea of the logical OPA port state */
static inline u32 hfi_driver_lstate(struct hfi_pportdata *ppd)
{
	return ppd->lstate; /* use the cached value */
}

int hfi_pci_init(struct pci_dev *pdev, const struct pci_device_id *ent);
void hfi_pci_cleanup(struct pci_dev *pdev);
struct hfi_devdata *hfi_pci_dd_init(struct pci_dev *pdev,
				    const struct pci_device_id *ent);
void hfi_pci_dd_free(struct hfi_devdata *dd);
int hfi_pcie_params(struct hfi_devdata *dd, u32 minw, u32 *nent,
		    struct hfi_msix_entry *entry);
void hfi_disable_msix(struct hfi_devdata * dd);
int setup_interrupts(struct hfi_devdata *dd, int total, int minw);
void cleanup_interrupts(struct hfi_devdata *dd);

struct hfi_devdata *hfi_alloc_devdata(struct pci_dev *pdev);
int hfi_user_cleanup(struct hfi_ctx *ud);

/* HFI specific functions */
int hfi_cq_assign_privileged(struct hfi_ctx *ctx, u16 *cq_idx);
int hfi_eq_assign_privileged(struct hfi_ctx *ctx);
void __hfi_eq_release(struct hfi_ctx *ctx);
void hfi_cq_cleanup(struct hfi_ctx *ctx);
void hfi_cq_config(struct hfi_ctx *ctx, u16 cq_idx,
		   struct hfi_auth_tuple *auth_table, bool user_priv);
void hfi_cq_config_tuples(struct hfi_ctx *ctx, u16 cq_idx,
			  struct hfi_auth_tuple *auth_table);
int hfi_update_dlid_relocation_table(struct hfi_ctx *ctx,
			       struct hfi_dlid_assign_args *dlid_assign);
int hfi_reset_dlid_relocation_table(struct hfi_ctx *ctx, u32 dlid_base,
				    u32 count);
void hfi_cq_disable(struct hfi_devdata *dd, u16 cq_idx);
void hfi_pcb_write(struct hfi_ctx *ctx, u16 ptl_pid);
void hfi_pcb_reset(struct hfi_devdata *dd, u16 ptl_pid);

/* OPA core functions */
int hfi_cq_assign(struct hfi_ctx *ctx, struct hfi_auth_tuple *auth_table, u16 *cq_idx);
int hfi_cq_update(struct hfi_ctx *ctx, u16 cq_idx, struct hfi_auth_tuple *auth_table);
int hfi_cq_release(struct hfi_ctx *ctx, u16 cq_idx);
int hfi_cq_map(struct hfi_ctx *ctx, u16 cq_idx,
	       struct hfi_cq *tx, struct hfi_cq *rx);
void hfi_cq_unmap(struct hfi_cq *tx, struct hfi_cq *rx);
int hfi_dlid_assign(struct hfi_ctx *ctx,
		    struct hfi_dlid_assign_args *dlid_assign);
int hfi_dlid_release(struct hfi_ctx *ctx, u32 dlid_base, u32 count);
int hfi_cteq_assign(struct hfi_ctx *ctx, struct opa_ev_assign *ev_assign);
int hfi_cteq_release(struct hfi_ctx *ctx, u16 eq_mode, u16 eq_idx,
		     u64 user_data);
int hfi_cteq_wait_single(struct hfi_ctx *ctx, u16 eq_mode, u16 ev_idx,
			 long timeout);
int hfi_ctxt_attach(struct hfi_ctx *ctx, struct opa_ctx_assign *ctx_assign);
void hfi_ctxt_cleanup(struct hfi_ctx *ctx);
int hfi_ctxt_reserve(struct hfi_ctx *ctx, u16 *base, u16 count);
void hfi_ctxt_unreserve(struct hfi_ctx *ctx);
int hfi_ctxt_hw_addr(struct hfi_ctx *ctx, int token, u16 ctxt, void **addr,
		     ssize_t *len);
int hfi_e2e_ctrl(struct hfi_ctx *ctx, struct opa_e2e_ctrl *e2e_ctrl);
void hfi_e2e_destroy(struct hfi_devdata *dd);

int hfi_iommu_root_alloc(void);
void hfi_iommu_root_free(void);
int hfi_iommu_root_set_context(struct hfi_devdata *dd);
void hfi_iommu_root_clear_context(struct hfi_devdata *dd);
void hfi_iommu_set_pasid(struct hfi_devdata *dd, struct mm_struct *user_mm,
			 u16 pasid);
void hfi_iommu_clear_pasid(struct hfi_devdata *dd, u16 pasid);

static inline struct hfi_pportdata *to_hfi_ppd(struct hfi_devdata *dd,
							u8 port)
{
	u8 pidx = port - 1; /* IB number port from 1, hdw from 0 */

	if (pidx >= dd->num_pports) {
		WARN_ON(pidx >= dd->num_pports);
		return NULL;
	}
	return &(dd->pport[pidx]);
}

static inline int ppd_to_pnum(struct hfi_pportdata *ppd)
{
	u8 pnum = ppd->pnum;
	struct hfi_devdata *dd = ppd->dd;

	if (unlikely(pnum == 0 || pnum > dd->num_pports)) {
		WARN(1, "Invalid port number");
		return -1;
	}

	return (int)pnum;
}

static inline void hfi_set_bits(u64 *target, u64 val, u64 mask, u8 shift)
{
	/* clear */
	*target &=  ~(mask << shift);
	/* set */
	*target |= (val & mask) << shift;
}

static inline u8 hfi_parity(u16 val)
{
	int i, m = 0;

	/* m is 1 for odd parity */
	for (i = 0; i < sizeof(val) * 8; i++, val >>= 1)
		m ^= (val & 0x1);

	return m;
}

int hfi_get_sma(struct opa_core_device *odev, u16 attr_id, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status);
int hfi_set_sma(struct opa_core_device *odev, u16 attr_id, struct opa_smp *smp,
		u32 am, u8 *data, u8 port, u32 *resp_len, u8 *sma_status);

void hfi_set_link_down_reason(struct hfi_pportdata *ppd, u8 lcl_reason,
			  u8 neigh_reason, u8 rem_reason);
void hfi_apply_link_downgrade_policy(struct hfi_pportdata *ppd,
						int refresh_widths);
int hfi_set_link_state(struct hfi_pportdata *ppd, u32 state);
int hfi_send_idle_sma(struct hfi_devdata *dd, u64 message);
u8 hfi_porttype(struct hfi_pportdata *ppd);
u8 hfi_ibphys_portstate(struct hfi_pportdata *ppd);
int hfi_get_ib_cfg(struct hfi_pportdata *ppd, int which);
int hfi_set_ib_cfg(struct hfi_pportdata *ppd, int which, u32 val);
int hfi_set_mtu(struct hfi_pportdata *ppd);
u16 hfi_port_ltp_to_cap(u16 port_ltp);
u16 hfi_port_ltp_to_lcb(struct hfi_devdata *dd, u16 port_ltp);
u16 hfi_cap_to_port_ltp(u16 cap);
void hfi_set_crc_mode(struct hfi_devdata *dd, u8 port, u16 crc_lcb_mode);
/*
 * dev_err can be used (only!) to print early errors before devdata is
 * allocated, or when dd->pcidev may not be valid, and at the tail end of
 * cleanup when devdata may have been freed, etc.
 * Otherwise these macros below are the preferred ones.
 */
#define dd_dev_err(dd, fmt, ...) \
	dev_err(&(dd)->pcidev->dev, DRIVER_NAME"%d: " fmt, \
		(dd)->unit, ##__VA_ARGS__)

#define dd_dev_info(dd, fmt, ...) \
	dev_info(&(dd)->pcidev->dev, DRIVER_NAME"%d: " fmt, \
		 (dd)->unit, ##__VA_ARGS__)

#define dd_dev_dbg(dd, fmt, ...) \
	dev_dbg(&(dd)->pcidev->dev, DRIVER_NAME"%d: " fmt, \
		 (dd)->unit, ##__VA_ARGS__)

#define dd_dev_warn(dd, fmt, ...) \
	dev_warn(&(dd)->pcidev->dev, DRIVER_NAME"%d: " fmt, \
		 (dd)->unit, ##__VA_ARGS__)

#ifndef BIT_ULL
#define BIT_ULL(nr)             (1ULL << (nr))
#endif

/* printk wrappers (pr_warn, etc) can also be used for general debugging. */
#undef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

static inline u64 read_csr(const struct hfi_devdata *dd, u32 offset)
{
	u64 val;

	BUG_ON(dd->kregbase[0] == NULL);
	val = readq(dd->kregbase[0] + offset);
	return le64_to_cpu(val);
}

static inline void write_csr(const struct hfi_devdata *dd, u32 offset, u64 value)
{
	BUG_ON(dd->kregbase[0] == NULL);
	writeq(cpu_to_le64(value), dd->kregbase[0] + offset);
}

#endif
