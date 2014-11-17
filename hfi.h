#ifndef _QIB_KERNEL_H
#define _QIB_KERNEL_H
/*
 * Copyright (c) 2012 - 2014 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
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
 * This header file is the base header file for qlogic_ib kernel code
 * qib_user.h serves a similar purpose for user code.
 */

#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/completion.h>
#include <linux/kref.h>
#include <linux/sched.h>
#include <linux/cdev.h>

#include "common.h"
#include "verbs.h"
#include "pio.h"
#include "wfr.h"
#include "mad.h"

/* only s/w major version of QLogic_IB we can handle */
#define QIB_CHIP_VERS_MAJ 2U

/* don't care about this except printing */
#define QIB_CHIP_VERS_MIN 0U

/* The Organization Unique Identifier (Mfg code), and its position in GUID */
#define QIB_OUI 0x001175
#define QIB_OUI_LSB 40

/*
 * per driver stats, either not device nor port-specific, or
 * summed over all of the devices and ports.
 * They are described by name via ipathfs filesystem, so layout
 * and number of elements can change without breaking compatibility.
 * If members are added or deleted qib_statnames[] in qib_fs.c must
 * change to match.
 */
struct qlogic_ib_stats {
	__u64 sps_ints; /* number of interrupts handled */
	__u64 sps_errints; /* number of error interrupts */
	__u64 sps_txerrs; /* tx-related packet errors */
	__u64 sps_rcverrs; /* non-crc rcv packet errors */
	__u64 sps_hwerrs; /* hardware errors reported (parity, etc.) */
	__u64 sps_nopiobufs; /* no pio bufs avail from kernel */
	__u64 sps_ctxts; /* number of contexts currently open */
	__u64 sps_lenerrs; /* number of kernel packets where RHF != LRH len */
	__u64 sps_buffull;
	__u64 sps_hdrfull;
};

extern struct qlogic_ib_stats qib_stats;
extern const struct pci_error_handlers qib_pci_err_handler;
extern struct pci_driver qib_driver;

#define QIB_CHIP_SWVERSION QIB_CHIP_VERS_MAJ
/*
 * First-cut critierion for "device is active" is
 * two thousand dwords combined Tx, Rx traffic per
 * 5-second interval. SMA packets are 64 dwords,
 * and occur "a few per second", presumably each way.
 */
#define QIB_TRAFFIC_ACTIVE_THRESHOLD (2000)

/*
 * Below contains all data related to a single context (formerly called port).
 */

#ifdef CONFIG_DEBUG_FS
struct hfi_opcode_stats_perctx;
#endif

struct qib_ctxtdata {
	/* shadow the ctxt's RcvCtrl register */
	u64 rcvctrl;
	void **rcvegrbuf;
	dma_addr_t *rcvegrbuf_phys;
	/* rcvhdrq base, needs mmap before useful */
	void *rcvhdrq;
	/* kernel virtual address where hdrqtail is updated */
	void *rcvhdrtail_kvaddr;
	/*
	 * Shared page for kernel to signal user processes that send buffers
	 * need disarming.  The process should call QIB_CMD_DISARM_BUFS
	 * or QIB_CMD_ACK_EVENT with IPATH_EVENT_DISARM_BUFS set.
	 */
	unsigned long *user_event_mask;
	/* when waiting for rcv or pioavail */
	wait_queue_head_t wait;
	/*
	 * rcvegr bufs base, physical, must fit
	 * in 44 bits so 32 bit programs mmap64 44 bit works)
	 */
	dma_addr_t rcvegr_phys;
	/* mmap of hdrq, must fit in 44 bits */
	dma_addr_t rcvhdrq_phys;
	dma_addr_t rcvhdrqtailaddr_phys;
	/* this receive context's assigned PIO ACK send context */
	struct send_context *sc;

	/* dynamic receive available interrupt timeout */
	u32 rcvavail_timeout;
	/*
	 * number of opens (including slave sub-contexts) on this instance
	 * (ignoring forks, dup, etc. for now)
	 */
	int cnt;
	/*
	 * how much space to leave at start of eager TID entries for
	 * protocol use, on each TID
	 */
	/* instead of calculating it */
	unsigned ctxt;
	/* non-zero if ctxt is being shared. */
	u16 subctxt_cnt;
	/* non-zero if ctxt is being shared. */
	u16 subctxt_id;
	u8 uuid[16];
	/* job key */
	u16 jkey;
	/* number of RcvArray groups for this context. */
	u32 rcv_array_groups;
	/* number of eager TID entries. */
	u32 eager_count;
	/* index of first eager TID entry. */
	u32 eager_base;
	/* number of expected TID entries */
	u32 expected_count;
	/* index of first expected TID entry. */
	u32 expected_base;
	/* cursor into the exp group sets */
	u16 tidcursor;
	/* number of exp TID groups assigned to the ctxt */
	u16 numtidgroups;
	/* size of exp TID group fields in tidusemap */
	u16 tidmapcnt;
	/* exp TID group usage bitfield array */
	unsigned long *tidusemap;
	/* pinned pages for exp sends, allocated at open */
	struct page **tid_pg_list;
	/* dma handles for exp tid pages */
	dma_addr_t *physshadow;
	/* lock protecting all Expected TID data */
	spinlock_t exp_lock;
	/* number of pio bufs for this ctxt (all procs, if shared) */
	u32 piocnt;
	/* first pio buffer for this ctxt */
	u32 pio_base;
	/* chip offset of PIO buffers for this ctxt */
	u32 piobufs;
	/* how many alloc_pages() chunks in rcvegrbuf_pages */
	u32 rcvegrbuf_chunks;
	u32 rcvegrbuf_chunksize;
	u32 rcvegrbufs_idx_mask;
	/* how many egrbufs per chunk */
	u16 rcvegrbufs_perchunk;
	/* ilog2 of above */
	u16 rcvegrbufs_perchunk_shift;
	/* order for rcvegrbuf_pages */
	size_t rcvegrbuf_size;
	/* number of rcvhdrq entries */
	u16 rcvhdrq_cnt;
	/* size of each of the rcvhdrq entries */
	u16 rcvhdrqentsize;
	/* rcvhdrq size (for freeing) */
	size_t rcvhdrq_size;
	/* per-context configuration flags */
	u16 flags;
	/* per-context event flags for fileops/intr communication */
	unsigned long event_flags;
	/* WAIT_RCV that timed out, no interrupt */
	u32 rcvwait_to;
	/* WAIT_PIO that timed out, no interrupt */
	u32 piowait_to;
	/* WAIT_RCV already happened, no wait */
	u32 rcvnowait;
	/* WAIT_PIO already happened, no wait */
	u32 pionowait;
	/* total number of polled urgent packets */
	u32 urgent;
	/* saved total number of polled urgent packets for poll edge trigger */
	u32 urgent_poll;
	/* pid of process using this ctxt */
	pid_t pid;
	pid_t subpid[QLOGIC_IB_MAX_SUBCTXT];
	/* same size as task_struct .comm[], command that opened context */
	char comm[16];
	/* so file ops can get at unit */
	struct hfi_devdata *dd;
	/* so funcs that need physical port can get it easily */
	struct qib_pportdata *ppd;
	/* A page of memory for rcvhdrhead, rcvegrhead, rcvegrtail * N */
	void *subctxt_uregbase;
	/* An array of pages for the eager receive buffers * N */
	void *subctxt_rcvegrbuf;
	/* An array of pages for the eager header queue entries * N */
	void *subctxt_rcvhdr_base;
	/* The version of the library which opened this ctxt */
	u32 userversion;
	/* Bitmask of active slaves */
	u32 active_slaves;
	/* Type of packets or conditions we want to poll for */
	u16 poll_type;
	/* receive packet sequence counter */
	u8 seq_cnt;
	u8 redirect_seq_cnt;
	/* ctxt rcvhdrq head offset */
	u32 head;
	u32 pkt_count;
	/* lookaside fields */
	struct qib_qp *lookaside_qp;
	u32 lookaside_qpn;
	/* QPs waiting for context processing */
	struct list_head qp_wait_list;
	/* interrupt handling */
	u64 imask;	/* clear interupt mask */
	int ireg;	/* clear interrupt register */
	unsigned numa_id; /* numa node of this context */
#ifdef CONFIG_DEBUG_FS
	/* verbs stats per CTX */
	struct hfi_opcode_stats_perctx *opstats;
#endif
	/*
	 * This is the kernel thread that will keep making
	 * progress on the user sdma requests behind the scenes.
	 * There is one per context (shared contexts use the master's).
	 */
	struct task_struct *progress;
	struct list_head sdma_queues;
	spinlock_t sdma_qlock;
};

/*
 * Represents a single packet at a high level. Put commonly computed things in
 * here so we do not have to keep doing them over and over. The rule of thumb is
 * if something is used one time to derive some value, store that soemthing in
 * here. If it is used mutliple times, then store the result of that derivation
 * in here.
 */
struct hfi_packet {
	void *ebuf;
	void *hdr;
	struct qib_ctxtdata *rcd;
	u64 rhf;
	u16 tlen;
	u16 hlen;
	u32 updegr;
};

/*
 * Private data for snoop/capture support.
 */
struct hfi_snoop_data {
	int mode_flag;
	struct cdev cdev;
	struct device *class_dev;
	spinlock_t snoop_lock;
	struct list_head queue;
	wait_queue_head_t waitq;
	void *filter_value;
	int (*filter_callback)(void *hdr, void *data, void *value);
};

struct qib_sge_state;

/*
 * Get/Set IB link-level config parameters for f_get/set_ib_cfg()
 * Mostly for MADs that set or query link parameters, also ipath
 * config interfaces
 */
#define QIB_IB_CFG_LIDLMC 0 /* LID (LS16b) and Mask (MS16b) */
#define QIB_IB_CFG_LWID_DG_ENB 1 /* allowed Link-width downgrade */
#define QIB_IB_CFG_LWID_ENB 2 /* allowed Link-width */
#define QIB_IB_CFG_LWID 3 /* currently active Link-width */
#define QIB_IB_CFG_SPD_ENB 4 /* allowed Link speeds */
#define QIB_IB_CFG_SPD 5 /* current Link spd */
#define QIB_IB_CFG_RXPOL_ENB 6 /* Auto-RX-polarity enable */
#define QIB_IB_CFG_LREV_ENB 7 /* Auto-Lane-reversal enable */
#define QIB_IB_CFG_LINKLATENCY 8 /* Link Latency (IB1.2 only) */
#define QIB_IB_CFG_HRTBT 9 /* IB heartbeat off/enable/auto; DDR/QDR only */
#define QIB_IB_CFG_OP_VLS 10 /* operational VLs */
#define QIB_IB_CFG_VL_HIGH_CAP 11 /* num of VL high priority weights */
#define QIB_IB_CFG_VL_LOW_CAP 12 /* num of VL low priority weights */
#define QIB_IB_CFG_OVERRUN_THRESH 13 /* IB overrun threshold */
#define QIB_IB_CFG_PHYERR_THRESH 14 /* IB PHY error threshold */
#define QIB_IB_CFG_LINKDEFAULT 15 /* IB link default (sleep/poll) */
#define QIB_IB_CFG_PKEYS 16 /* update partition keys */
#define QIB_IB_CFG_MTU 17 /* update MTU in IBC */
#define QIB_IB_CFG_VL_HIGH_LIMIT 19
#define QIB_IB_CFG_PMA_TICKS 20 /* PMA sample tick resolution */
#define QIB_IB_CFG_PORT 21 /* switch port we are connected to */

/*
 * HFI or Host Link States
 *
 * These describe the states the driver thinks the logical and physicial
 * states are in.  Used as an argument to set_link_state().
 */
#define HLS_UP_INIT	 0
#define HLS_UP_ARMED	 1
#define HLS_UP_ACTIVE	 2
#define HLS_DN_DOWNDEF	 3	/* link down default */
#define HLS_DN_POLL	 4
#define HLS_DN_SLEEP	 5
#define HLS_DN_DISABLE	 6
#define HLS_DN_OFFLINE	 7
#define HLS_VERIFY_CAP	 8
#define HLS_GOING_UP	 9
#define HLS_GOING_DOWN	10

/* use this MTU size if none other is given */
#define HFI_DEFAULT_ACTIVE_MTU 4096
/* use this MTU size as the default maximum */
/* TODO: bad things may happen if 8K and 10K STL sizes used with an IB FM,
   e.g. opensm */
#define HFI_DEFAULT_MAX_MTU 8192
/* default parition key */
#define DEFAULT_PKEY 0xffff

/*
 * Possible fabric manager config parameters for fm_{get,set}_table()
 */
#define FM_TBL_VL_HIGH_ARB		1 /* Get/set VL high prio weights */
#define FM_TBL_VL_LOW_ARB		2 /* Get/set VL low prio weights */
#define FM_TBL_BUFFER_CONTROL		3 /* Get/set Buffer Control */
#define FM_TBL_SC2VLNT			4 /* Get/set SC->VLnt */
#define FM_TBL_VL_PREEMPT_ELEMS		5 /* Get (no set) VL preempt elems */
#define FM_TBL_VL_PREEMPT_MATRIX	6 /* Get (no set) VL preempt matrix */

/*
 * Possible "operations" for f_rcvctrl(ppd, op, ctxt)
 * these are bits so they can be combined, e.g.
 * QIB_RCVCTRL_INTRAVAIL_ENB | QIB_RCVCTRL_CTXT_ENB
 */
#define QIB_RCVCTRL_TAILUPD_ENB 0x01
#define QIB_RCVCTRL_TAILUPD_DIS 0x02
#define QIB_RCVCTRL_CTXT_ENB 0x04
#define QIB_RCVCTRL_CTXT_DIS 0x08
#define QIB_RCVCTRL_INTRAVAIL_ENB 0x10
#define QIB_RCVCTRL_INTRAVAIL_DIS 0x20
#define QIB_RCVCTRL_PKEY_ENB 0x40  /* Note, default is enabled */
#define QIB_RCVCTRL_PKEY_DIS 0x80
#define QIB_RCVCTRL_BP_ENB 0x0100
#define QIB_RCVCTRL_BP_DIS 0x0200
#define QIB_RCVCTRL_TIDFLOW_ENB 0x0400
#define QIB_RCVCTRL_TIDFLOW_DIS 0x0800
#define QIB_RCVCTRL_ONE_PKT_EGR_ENB 0x1000
#define QIB_RCVCTRL_ONE_PKT_EGR_DIS 0x2000
#define QIB_RCVCTRL_NO_RHQ_DROP_ENB 0x4000
#define QIB_RCVCTRL_NO_RHQ_DROP_DIS 0x8000
#define QIB_RCVCTRL_NO_EGR_DROP_ENB 0x10000
#define QIB_RCVCTRL_NO_EGR_DROP_DIS 0x20000

/*
 * These are the generic indices for requesting per-port
 * counter values via the f_portcntr function.  They
 * are always returned as 64 bit values, although most
 * are 32 bit counters.
 */
/* send-related counters */
#define QIBPORTCNTR_PKTSEND         0U
#define QIBPORTCNTR_WORDSEND        1U
#define QIBPORTCNTR_PSXMITDATA      2U
#define QIBPORTCNTR_PSXMITPKTS      3U
#define QIBPORTCNTR_PSXMITWAIT      4U
#define QIBPORTCNTR_SENDSTALL       5U
/* receive-related counters */
#define QIBPORTCNTR_PKTRCV          6U
#define QIBPORTCNTR_PSRCVDATA       7U
#define QIBPORTCNTR_PSRCVPKTS       8U
#define QIBPORTCNTR_RCVEBP          9U
#define QIBPORTCNTR_RCVOVFL         10U
#define QIBPORTCNTR_WORDRCV         11U
/* IB link related error counters */
#define QIBPORTCNTR_RXLOCALPHYERR   12U
#define QIBPORTCNTR_RXVLERR         13U
#define QIBPORTCNTR_ERRICRC         14U
#define QIBPORTCNTR_ERRVCRC         15U
#define QIBPORTCNTR_ERRLPCRC        16U
#define QIBPORTCNTR_BADFORMAT       17U
#define QIBPORTCNTR_ERR_RLEN        18U
#define QIBPORTCNTR_IBSYMBOLERR     19U
#define QIBPORTCNTR_INVALIDRLEN     20U
#define QIBPORTCNTR_UNSUPVL         21U
#define QIBPORTCNTR_EXCESSBUFOVFL   22U
#define QIBPORTCNTR_ERRLINK         23U
#define QIBPORTCNTR_IBLINKDOWN      24U
#define QIBPORTCNTR_IBLINKERRRECOV  25U
#define QIBPORTCNTR_LLI             26U
/* other error counters */
#define QIBPORTCNTR_RXDROPPKT       27U
#define QIBPORTCNTR_VL15PKTDROP     28U
#define QIBPORTCNTR_ERRPKEY         29U
#define QIBPORTCNTR_KHDROVFL        30U
/* sampling counters (these are actually control registers) */
#define QIBPORTCNTR_PSINTERVAL      31U
#define QIBPORTCNTR_PSSTART         32U
#define QIBPORTCNTR_PSSTAT          33U

/* how often we check for packet activity for "power on hours (in seconds) */
#define ACTIVITY_TIMER 5

#define MAX_NAME_SIZE 64
struct qib_msix_entry {
	struct msix_entry msix;
	void *arg;
	char name[MAX_NAME_SIZE];
	cpumask_var_t mask;
};

/* per-SL CCA information */
struct cca_timer {
	struct hrtimer hrtimer;
	struct qib_pportdata *ppd; /* read-only */
	int sl; /* read-only */
	u16 ccti; /* read/write - current value of CCTI */
};

/*
 * The structure below encapsulates data relevant to a physical IB Port.
 * Current chips support only one such port, but the separation
 * clarifies things a bit. Note that to conform to IB conventions,
 * port-numbers are one-based. The first or only port is port1.
 */
struct qib_pportdata {
	struct qib_ibport ibport_data;

	struct hfi_devdata *dd;
	struct kobject pport_kobj;
	struct kobject pport_cc_kobj;
	struct kobject sc2vl_kobj;
	struct kobject sl2sc_kobj;
	struct kobject vl2mtu_kobj;
	struct kobject diagc_kobj;

	/* GUID for this interface, in network order */
	__be64 guid;
	/* GUID for peer interface, in network order */
	__be64 neighbor_guid;

	/* up or down physical link state */
	u32 linkup;

	/*
	 * this address is mapped readonly into user processes so they can
	 * get status cheaply, whenever they want.  One qword of status per port
	 */
	u64 *statusp;

	/* SendDMA related entries */

	struct workqueue_struct *qib_wq;

	/* move out of interrupt context */
	struct work_struct link_vc_work;
	struct work_struct link_up_work;
	struct work_struct link_down_work;
	struct work_struct sma_message_work;
	struct delayed_work link_restart_work;
	/* host link state variables */
	struct mutex hls_lock;
	u32 host_link_state;

	spinlock_t            sdma_alllock ____cacheline_aligned_in_smp;

	u32 lstate;	/* logical link state */

	/* these are the "32 bit" regs */

	u32 ibmtu; /* The MTU programmed for this unit */
	/*
	 * Current max size IB packet (in bytes) including IB headers, that
	 * we can send. Changes when ibmtu changes.
	 */
	u32 ibmaxlen;
	/* LID programmed for this instance */
	u16 lid;
	/* list of pkeys programmed; 0 if not set */
	u16 pkeys[WFR_MAX_PKEY_VALUES];
	u16 link_width_supported;
	u16 link_width_downgrade_supported;
	u16 link_speed_supported;
	u16 link_width_enabled;
	u16 link_width_downgrade_enabled;
	u16 link_speed_enabled;
	u16 link_width_active;
	u16 link_width_downgrade_active;
	u16 link_speed_active;
	u8 vls_supported;
	u8 vls_operational;
	/* LID mask control */
	u8 lmc;
	/* Rx Polarity inversion (compensate for ~tx on partner) */
	u8 rx_pol_inv;

	u8 hw_pidx;     /* physical port index */
	u8 port;        /* IB port number and index into dd->pports - 1 */
	/* type of neighbor node */
	u8 neighbor_type;
	u8 neighbor_normal;
	u8 is_sm_config_started;
	u8 is_active_optimize_enabled;
	u8 driver_link_ready;	/* driver ready for active link */
	u8 link_enabled;	/* link enabled? */

	u8 delay_mult;
	/* placeholders for IB MAD packet settings */
	u8 overrun_threshold;
	u8 phy_error_threshold;

	/* used to override LED behavior */
	u8 led_override;  /* Substituted for normal value, if non-zero */
	u16 led_override_timeoff; /* delta to next timer event */
	u8 led_override_vals[2]; /* Alternates per blink-frame */
	u8 led_override_phase; /* Just counts, LSB picks from vals[] */
	atomic_t led_override_timer_active;
	/* Used to flash LEDs in override mode */
	struct timer_list led_override_timer;
	struct timer_list symerr_clear_timer;

	/*
	 * cca_timer_lock protects access to the per-SL cca_timer
	 * structures (specifically the ccti member).
	 */
	spinlock_t cca_timer_lock ____cacheline_aligned_in_smp;
	struct cca_timer cca_timer[STL_MAX_SLS];

	/* List of congestion control table entries */
	struct ib_cc_table_entry_shadow ccti_entries[CC_TABLE_SHADOW_MAX];

	/* congestion entries, each entry corresponding to a SL */
	struct stl_congestion_setting_entry_shadow
		congestion_entries[STL_MAX_SLS];

	/*
	 * cc_state_lock protects (write) access to the per-port
	 * struct cc_state.
	 */
	spinlock_t cc_state_lock ____cacheline_aligned_in_smp;

	struct cc_state __rcu *cc_state;

	/* Total number of congestion control table entries */
	u16 total_cct_entry;

	/* Bit map identifying service level */
	u32 cc_sl_control_map;

	/* CA's max number of 64 entry units in the congestion control table */
	u8 cc_max_table_entries;

	/* begin congestion log related entries
	 * cc_log_lock protects all congestion log related data */
	spinlock_t cc_log_lock ____cacheline_aligned_in_smp;
	u8 threshold_cong_event_map[STL_MAX_SLS/8];
	u16 threshold_event_counter;
	struct stl_hfi_cong_log_event_internal cc_events[STL_CONG_LOG_ELEMS];
	int cc_log_idx; /* index for logging events */
	int cc_mad_idx; /* index for reporting events */
	/* end congestion log related entries */

	/* port relative counter buffer */
	u64 *cntrs;
	/* we synthesize port_xmit_discards from several egress errors */
	u64 port_xmit_discards;
	/* count of 'link_err' interrupts from DC */
	u32 link_downed;
	/* port_ltp_crc_mode is returned in 'portinfo' MADs */
	u16 port_ltp_crc_mode;
	/* mgmt_allowed is also returned in 'portinfo' MADs */
	u8 mgmt_allowed;
	u8 link_quality; /* part of portstatus, datacounters PMA queries */
};

/* Observers. Not to be taken lightly, possibly not to ship. */
/*
 * If a diag read or write is to (bottom <= offset <= top),
 * the "hoook" is called, allowing, e.g. shadows to be
 * updated in sync with the driver. struct diag_observer
 * is the "visible" part.
 */
struct diag_observer;

typedef int (*diag_hook) (struct hfi_devdata *dd,
	const struct diag_observer *op,
	u32 offs, u64 *data, u64 mask, int only_32);

struct diag_observer {
	diag_hook hook;
	u32 bottom;
	u32 top;
};

extern int qib_register_observer(struct hfi_devdata *dd,
	const struct diag_observer *op);

/* Only declared here, not defined. Private to diags */
struct diag_observer_list_elt;

struct rcv_array_data {
	u8 group_size;
	u16 ngroups;
	u16 nctxt_extra;
};

struct per_vl_data {
	u16 mtu;
	struct send_context *sc;
};

/* 16 to directly index */
#define PER_VL_SEND_CONTEXTS 16

struct err_info_rcvport {
	u8 status_and_code;
	u64 packet_flit1;
	u64 packet_flit2;
};

/* device data struct now contains only "general per-device" info.
 * fields related to a physical IB port are in a qib_pportdata struct,
 * described above) while fields only used by a particular chip-type are in
 * a qib_chipdata struct, whose contents are opaque to this file.
 */
struct sdma_engine;
struct hfi_devdata {
	struct qib_ibdev verbs_dev;     /* must be first */
	struct list_head list;
	/* pointers to related structs for this device */
	/* pci access data structure */
	struct pci_dev *pcidev;
	struct cdev user_cdev;
	struct cdev diag_cdev;
	struct cdev ui_cdev;
	struct device *user_device;
	struct device *diag_device;
	struct device *ui_device;

	/* mem-mapped pointer to base of chip regs */
	u8 __iomem *kregbase;
	/* end of mem-mapped chip space excluding sendbuf and user regs */
	u8 __iomem *kregend;
	/* physical address of chip for io_remap, etc. */
	resource_size_t physaddr;
	/* recevie context data */
	struct qib_ctxtdata **rcd;
	/* send context data */
	struct send_context_info *send_contexts;
	/* Per VL data. Enough for all VLs but not all elements are set/used. */
	struct per_vl_data vld[PER_VL_SEND_CONTEXTS];
	/* seqlock for sc2vl */
	seqlock_t sc2vl_lock;
	u64 sc2vl[4];
	/* Send Context initialization lock. */
	spinlock_t sc_init_lock;

	/* fields common to all SDMA engines */

	/* default flags to last descriptor */
	__le64 default_desc1;
	volatile __le64                    *sdma_heads_dma; /* DMA'ed by chip */
	dma_addr_t                          sdma_heads_phys;
	void                               *sdma_pad_dma; /* DMA'ed by chip */
	dma_addr_t                          sdma_pad_phys;
	/* for deallocation */
	size_t                              sdma_heads_size;
	/* number from the chip */
	u32                                 chip_sdma_engines;
	/* num used */
	u32                                 num_sdma;
	/* lock for selector_sdma_mask and sdma_map */
	seqlock_t                           sde_map_lock;
	u32                                 selector_sdma_mask;
	u32                                 selector_sdma_shift;
	/* array of engines dimensioned by num_sdma */
	struct sdma_engine                 *per_sdma;
	/* array of engines to select based on vl,selector */
	struct sdma_engine                **sdma_map;

	/* qib_pportdata, points to array of (physical) port-specific
	 * data structs, indexed by pidx (0..n-1)
	 */
	struct qib_pportdata *pport;

	/* mem-mapped pointer to base of PIO buffers */
	void __iomem *piobase;
	/*
	 * write-combining mem-mapped pointer to base of RcvArray
	 * memory.
	 */
	void __iomem *rcvarray_wc;
	/*
	 * credit return base - a per-NUMA range of DMA address that
	 * the chip will use to update the per-context free counter
	 */
	struct credit_return_base *cr_base;

	/* send context numbers and sizes for each type */
	struct sc_config_sizes sc_sizes[SC_MAX];

	u32 lcb_access_count;		/* count of LCB users */

	/* device-specific implementations of functions needed by
	 * common code. Contrary to previous consensus, we can't
	 * really just point to a device-specific table, because we
	 * may need to "bend", e.g. *_f_put_tid
	 */
	/* fallback to alternate interrupt type if possible */
	int (*f_intr_fallback)(struct hfi_devdata *);
	/* hard reset chip */
	int (*f_reset)(struct hfi_devdata *);
	void (*f_quiet_serdes)(struct qib_pportdata *);
	int (*f_bringup_serdes)(struct qib_pportdata *);
	int (*f_early_init)(struct hfi_devdata *);
	void (*f_clear_tids)(struct qib_ctxtdata *);
	void (*f_put_tid)(struct hfi_devdata *, u32, u32, unsigned long, u16);
	void (*f_cleanup)(struct hfi_devdata *);
	void (*f_setextled)(struct qib_pportdata *, u32);
	/* fill out chip-specific fields */
	int (*f_get_base_info)(struct qib_ctxtdata *, struct hfi_base_info *);
	/* free irq */
	void (*f_free_irq)(struct hfi_devdata *);
	struct qib_message_header *(*f_get_msgheader)
					(struct hfi_devdata *, __le32 *);
	void (*f_config_ctxts)(struct hfi_devdata *);
	int (*f_get_ib_cfg)(struct qib_pportdata *, int);
	int (*f_set_ib_cfg)(struct qib_pportdata *, int, u32);
	int (*f_set_ib_loopback)(struct qib_pportdata *, const char *);
	u32 (*f_iblink_state)(struct qib_pportdata *);
	u8 (*f_ibphys_portstate)(struct qib_pportdata *);
	void (*f_xgxs_reset)(struct qib_pportdata *);
	/* Read/modify/write of GPIO pins (potentially chip-specific */
	int (*f_gpio_mod)(struct hfi_devdata *dd, u32 out, u32 dir,
		u32 mask);
	/* modify receive context registers, see RCVCTRL_* for operations */
	void (*f_rcvctrl)(struct hfi_devdata *, unsigned int op, int context);
	void (*f_set_intr_state)(struct hfi_devdata *, u32);
	void (*f_set_armlaunch)(struct hfi_devdata *, u32);
	void (*f_wantpiobuf_intr)(struct send_context *, u32);
	/* FIXME - get rid of this */
	void (*f_sdma_update_tail)(struct sdma_engine *, u16);
	u64 (*f_portcntr)(struct qib_pportdata *, u32);
	u32 (*f_read_cntrs)(struct hfi_devdata *, loff_t, char **,
		u64 **);
	u32 (*f_read_portcntrs)(struct hfi_devdata *, loff_t, u32,
		char **, u64 **);
	int (*f_init_ctxt)(struct qib_ctxtdata *);
	int (*f_tempsense_rd)(struct hfi_devdata *, int regnum);
	int (*f_set_ctxt_jkey)(struct hfi_devdata *, unsigned, u16);
	int (*f_clear_ctxt_jkey)(struct hfi_devdata *, unsigned);
	int (*f_set_ctxt_pkey)(struct hfi_devdata *, unsigned, u16);
	int (*f_clear_ctxt_pkey)(struct hfi_devdata *, unsigned);

	char *boardname; /* human readable board info */

	/* number of registers used for pioavail */
	u32 pioavregs;
	/* device (not port) flags, basically device capabilities */
	u32 flags;

	/* reset value */
	u64 z_int_counter;
	/* percpu int_counter */
	u64 __percpu *int_counter;

	/* number of receive contexts in use by the driver */
	u32 num_rcv_contexts;
	/* number of pio send contexts in use by the driver */
	u32 num_send_contexts;
	/*
	 * number of ctxts available for PSM open
	 */
	u32 freectxts;
	/* base receive interrupt timeout, in CSR units */
	u32 rcv_intr_timeout_csr;

	/*
	 * hint that we should update pioavailshadow before
	 * looking for a PIO buffer
	 */
	u32 upd_pio_shadow;

	/* internal debugging stats */
	u32 maxpkts_call;
	u32 avgpkts_call;
	u64 nopiobufs;

	/* PCI Vendor ID (here for NodeInfo) */
	u16 vendorid;
	/* PCI Device ID (here for NodeInfo) */
	u16 deviceid;
	/* for write combining settings */
	unsigned long wc_cookie;
	unsigned long wc_base;
	unsigned long wc_len;

	u64 __iomem *egrtidbase;
	spinlock_t sendctrl_lock; /* protect changes to sendctrl shadow */
	/* around rcd and (user ctxts) ctxt_cnt use (intr vs free) */
	spinlock_t uctxt_lock; /* rcd and user context changes */
	/* exclusive access to 8051 */
	spinlock_t dc8051_lock;
	int dc8051_timed_out;	/* remember if the 8051 timed out */
	/*
	 * A page that will hold event notification bitmaps for all
	 * contexts. This page will be mapped into all processes.
	 */
	u64 *events;
	/*
	 * per unit status, see also portdata statusp
	 * mapped readonly into user processes so they can get unit and
	 * IB link status cheaply
	 */
	struct hfi_status *status;
	u32 freezelen; /* max length of freezemsg */

	/* timer to verify interrupts work, and fall back if possible */
	struct delayed_work interrupt_check_worker;
	unsigned long ureg_align; /* user register alignment */

	/* revision register shadow */
	u64 revision;
	/* Base GUID for device (network order) */
	__be64 base_guid;

	/* these are the "32 bit" regs */

	/* value we put in kr_rcvhdrcnt */
	u32 rcvhdrcnt;
	/* value we put in kr_rcvhdrsize */
	u32 rcvhdrsize;
	/* value we put in kr_rcvhdrentsize */
	u32 rcvhdrentsize;
	/* number of receive contexts the chip supports */
	u32 chip_rcv_contexts;
	/* number of receive array entries */
	u32 chip_rcv_array_count;
	/* number of PIO send contexts the chip supports */
	u32 chip_send_contexts;
	/* number of bytes in the PIO memory buffer */
	u32 chip_pio_mem_size;
	/* number of bytes in the SDMA memory buffer */
	u32 chip_sdma_mem_size;
	/* kr_pagealign value */
	u32 palign;
	/* max usable size in dwords of a "2KB" PIO buffer before going "4KB" */
	u32 piosize2kmax_dwords;
	/* kr_userregbase */
	u32 uregbase;
	/* shadow the control register contents */
	u32 control;

	/* size of each rcvegrbuffer */
	u32 rcvegrbufsize;
	/* log2 of above */
	u16 rcvegrbufsize_shift;
	/* both sides of the PCIe link are gen3 capable */
	u8 link_gen3_capable;
	/* localbus width (1, 2,4,8,16,32) from config space  */
	u32 lbus_width;
	/* localbus speed in MHz */
	u32 lbus_speed;
	int unit; /* unit # of this chip */
	int node; /* home node of this chip */

	/* save these BARs and command to rewrite after a reset */
	u32 pcibar0;
	u32 pcibar1;
	u32 pci_rom;
	u16 pci_command;

	/*
	 * ASCII serial number, from flash, large enough for original
	 * all digit strings, and longer QLogic serial number format
	 */
	u8 serial[16];
	/* human readable board version */
	u8 boardversion[96];
	u8 lbus_info[32]; /* human readable localbus info */
	/* chip major rev, from qib_revision */
	u8 majrev;
	/* chip minor rev, from qib_revision */
	u8 minrev;
	/* hardware ID */
	u8 hfi_id;
	/* implementation code */
	u8 icode;
	/* default link down value (poll/sleep) */
	u8 link_default;
	/* vAU of this device */
	u8 vau;
	/* vCU of this device */
	u8 vcu;
	/* link credits of this device */
	u16 link_credits;
	/* initial vl15 credits to use */
	u16 vl15_init;

	/* Misc small ints */
	/* Number of physical ports available */
	u8 num_pports;
	/* Lowest context number which can be used by user processes */
	u8 first_user_ctxt;
	u8 n_krcv_queues;
	u8 qpn_mask;

	u16 rhf_offset; /* offset of RHF within receive header entry */
	u16 irev;	/* implementation revision */

	/*
	 * GPIO pins for twsi-connected devices
	 */
	u8 gpio_sda_num;
	u8 gpio_scl_num;
	u8 board_atten;

	/* control high-level access to qsfp */
	struct mutex qsfp_lock;

	struct diag_client *diag_client;
	spinlock_t qib_diag_trans_lock; /* protect diag observer ops */
	struct diag_observer_list_elt *diag_observer_list;

	u8 psxmitwait_supported;
	/* cycle length of PS* counters in HW (in picoseconds) */
	u16 psxmitwait_check_rate;
	/* high volume overflow errors defered to tasklet */
	struct tasklet_struct error_tasklet;

	/* MSI-X information */
	struct qib_msix_entry *msix_entries;
	u32 num_msix_entries;

	/* INTx information */
	u32 requested_intx_irq;		/* did we request one? */
	char intx_name[MAX_NAME_SIZE];	/* INTx name */

	/* general interrupt: mask of handled interrupts */
	u64 gi_mask[WFR_CCE_NUM_INT_CSRS];

	struct rcv_array_data rcv_entries;

	/* timer used for 64 bit counter emulation  */
	/* See HAS section 13.2 */
	struct timer_list stats_timer;
	/* TODO: temporary code for missing interrupts, HSD 291041 */
	struct timer_list fifo_timer;	/* interval timer for FIFO check */
	u32 *last_krcv_fifo_head;	/* last read FIFO head value */

	/*
	 * per device 64 bit counters
	 */
	u64 *cntrs;
	/*
	 * per device counters names
	 */
	char *cntrnames;
	size_t cntrnameslen;
	/*
	 * number of port counters
	 */
	size_t nportcntrs;
	/*
	 * port cntr names
	 */
	char *portcntrnames;
	size_t portcntrnameslen;
	struct hfi_snoop_data hfi_snoop;

	struct err_info_rcvport err_info_rcvport;
	u8 err_info_uncorrectable;
	u8 err_info_fmconfig;
	/*
	 * Handlers for outgoing data so that snoop/capture does not
	 * have to have its hooks in the send path
	 */
	int (*process_pio_send)(struct qib_qp *qp, struct ahg_ib_header *ibhdr,
				u32 hdrwords, struct qib_sge_state *ss, u32 len,
				u32 plen, u32 dwords, u64 pbc);
	int (*process_dma_send)(struct qib_qp *qp, struct ahg_ib_header *ibhdr,
				u32 hdrwords, struct qib_sge_state *ss, u32 len,
				u32 plen, u32 dwords, u64 pbc);
	void (*pio_inline_send)(struct hfi_devdata *dd, struct pio_buf *pbuf,
				u64 pbc, const void *from, size_t count);
};

/* f_put_tid types */
#define PT_EXPECTED 0
#define PT_EAGER    1
#define PT_INVALID  2

#define QIB_SDMA_SENDCTRL_OP_ENABLE    (1U << 0)
#define QIB_SDMA_SENDCTRL_OP_INTENABLE (1U << 1)
#define QIB_SDMA_SENDCTRL_OP_HALT      (1U << 2)
#define QIB_SDMA_SENDCTRL_OP_CLEANUP   (1U << 3)
#define QIB_SDMA_SENDCTRL_OP_DRAIN     (1U << 4)

/* Private data for file operations */
struct hfi_filedata {
	struct qib_ctxtdata *uctxt;
	unsigned subctxt;
	struct hfi_user_sdma_comp_q *cq;
	struct hfi_user_sdma_pkt_q *pq;
	int rec_cpu_num; // for cpu affinity; -1 if none
};

extern struct list_head qib_dev_list;
extern spinlock_t qib_devs_lock;
extern struct hfi_devdata *qib_lookup(int unit);
extern u32 qib_cpulist_count;
extern unsigned long *qib_cpulist;

extern unsigned int snoop_drop_send;
extern unsigned int snoop_force_capture;
int qib_init(struct hfi_devdata *, int);
int qib_count_units(int *npresentp, int *nupp);
int qib_count_active_units(void);

int qib_diag_add(struct hfi_devdata *);
void qib_diag_remove(struct hfi_devdata *);
void handle_linkup_change(struct hfi_devdata *dd, u32 linkup);
void qib_sdma_update_tail(struct qib_pportdata *, u16); /* hold sdma_lock */

int qib_decode_err(struct hfi_devdata *dd, char *buf, size_t blen, u64 err);
void qib_bad_intrstatus(struct hfi_devdata *);
void handle_user_interrupt(struct qib_ctxtdata *rcd);

/* clean up any per-chip chip-specific stuff */
void qib_chip_cleanup(struct hfi_devdata *);
/* clean up any chip type-specific stuff */
void qib_chip_done(void);

int qib_create_rcvhdrq(struct hfi_devdata *, struct qib_ctxtdata *);
int qib_setup_eagerbufs(struct qib_ctxtdata *);
int qib_create_ctxts(struct hfi_devdata *dd);
struct qib_ctxtdata *qib_create_ctxtdata(struct qib_pportdata *, u32);
void qib_init_pportdata(struct pci_dev *, struct qib_pportdata *,
			struct hfi_devdata *, u8, u8);
void qib_free_ctxtdata(struct hfi_devdata *, struct qib_ctxtdata *);
int hfi_setup_ctxt(struct qib_ctxtdata *, u16, u16, u16, u16);

void handle_receive_interrupt(struct qib_ctxtdata *);
int qib_reset_device(int);
int qib_wait_linkstate(struct qib_pportdata *, u32, int);
inline u16 generate_jkey(unsigned int);
void set_link_ipg(struct qib_pportdata *ppd);
void process_becn(struct qib_pportdata *ppd, u8 sl,  u16 rlid, u32 lqpn,
		  u32 rqpn, u8 svc_type);
void return_cnp(struct qib_ibport *ibp, struct qib_qp *qp, u32 remote_qpn,
		u32 pkey, u32 slid, u32 dlid, u8 sc5,
		const struct ib_grh *old_grh);

/**
 * sc_to_vlt() reverse lookup sc to vl
 * @dd - devdata
 * @sc5 - 5 bit sc
 */
static inline u8 sc_to_vlt(struct hfi_devdata *dd, u8 sc5)
{
	unsigned seq;
	u8 rval;

	if (sc5 > STL_MAX_SCS)
		return (u8)(0xff);

	do {
		seq = read_seqbegin(&dd->sc2vl_lock);
		rval = *(((u8 *)dd->sc2vl) + sc5);
	} while (read_seqretry(&dd->sc2vl_lock, seq));

	return rval;
}

/* MTU handling */

/* MTU enumeration, 256-4k match IB */
#define STL_MTU_256   1
#define STL_MTU_512   2
#define STL_MTU_1024  3
#define STL_MTU_2048  4
#define STL_MTU_4096  5
#include <rdma/stl_smi.h>
#include <rdma/stl_port_info.h>

u32 lrh_max_header_bytes(struct hfi_devdata *dd);
int mtu_to_enum(u32 mtu, int default_if_bad);
u32 enum_to_mtu(int);
static inline int valid_ib_mtu(unsigned int mtu)
{
	return mtu == 256 || mtu == 512 ||
		mtu == 1024 || mtu == 2048 ||
		mtu == 4096;
}
static inline int valid_stl_mtu(unsigned int mtu)
{
	return valid_ib_mtu(mtu) || mtu == 8192	|| mtu == 10240;
}
int set_mtu(struct qib_pportdata *);

int qib_set_lid(struct qib_pportdata *, u32, u8);
void qib_disable_after_error(struct hfi_devdata *);
int qib_set_uevent_bits(struct qib_pportdata *, const int);
int hfi_rcvbuf_validate(u32, u8, u16 *);

int fm_get_table(struct qib_pportdata *, int, void *);
int fm_set_table(struct qib_pportdata *, int, void *);

void set_up_vl15(struct hfi_devdata *dd, u8 vau, u16 vl15buf);
void reset_link_credits(struct hfi_devdata *dd);
void assign_remote_cm_au_table(struct hfi_devdata *dd, u8 vcu);
void assign_link_credits(struct hfi_devdata *dd);

void snoop_recv_handler(struct hfi_packet *packet);
int snoop_send_dma_handler(struct qib_qp *qp, struct ahg_ib_header *ibhdr,
			   u32 hdrwords, struct qib_sge_state *ss, u32 len,
			   u32 plen, u32 dwords, u64 pbc);
int snoop_send_pio_handler(struct qib_qp *qp, struct ahg_ib_header *ibhdr,
			   u32 hdrwords, struct qib_sge_state *ss, u32 len,
			   u32 plen, u32 dwords, u64 pbc);
void snoop_inline_pio_send(struct hfi_devdata *dd, struct pio_buf *pbuf,
			   u64 pbc, const void *from, size_t count);

/* for use in system calls, where we want to know device type, etc. */
#define ctxt_fp(fp) \
	(((struct hfi_filedata *)(fp)->private_data)->uctxt)
#define subctxt_fp(fp) \
	(((struct hfi_filedata *)(fp)->private_data)->subctxt)
#define tidcursor_fp(fp) \
	(((struct hfi_filedata *)(fp)->private_data)->tidcursor)
#define user_sdma_pkt_fp(fp) \
	(((struct hfi_filedata *)(fp)->private_data)->pq)
#define user_sdma_comp_fp(fp) \
	(((struct hfi_filedata *)(fp)->private_data)->cq)

static inline struct hfi_devdata *dd_from_ppd(struct qib_pportdata *ppd)
{
	return ppd->dd;
}

static inline struct hfi_devdata *dd_from_dev(struct qib_ibdev *dev)
{
	return container_of(dev, struct hfi_devdata, verbs_dev);
}

static inline struct hfi_devdata *dd_from_ibdev(struct ib_device *ibdev)
{
	return dd_from_dev(to_idev(ibdev));
}

static inline struct qib_pportdata *ppd_from_ibp(struct qib_ibport *ibp)
{
	return container_of(ibp, struct qib_pportdata, ibport_data);
}

static inline struct qib_ibport *to_iport(struct ib_device *ibdev, u8 port)
{
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	unsigned pidx = port - 1; /* IB number port from 1, hdw from 0 */

	WARN_ON(pidx >= dd->num_pports);
	return &dd->pport[pidx].ibport_data;
}

/*
 * Readers of cc_state must call get_cc_state() under rcu_read_lock().
 * Writers of cc_state must call get_cc_state() under cc_state_lock.
 */
static inline struct cc_state *get_cc_state(struct qib_pportdata *ppd)
{
	return rcu_dereference(ppd->cc_state);
}

/*
 * values for dd->flags (_device_ related flags) and
 */
#define QIB_HAS_LINK_LATENCY  0x1 /* supports link latency (IB 1.2) */
#define QIB_INITTED           0x2 /* chip and driver up and initted */
#define QIB_DOING_RESET       0x4  /* in the middle of doing chip reset */
#define QIB_PRESENT           0x8  /* chip accesses can be done */
/* unused		      0x10 */
#define QIB_HAS_THRESH_UPDATE 0x40
#define QIB_HAS_SDMA_TIMEOUT  0x80
/* unused		      0x100 */
#define QIB_NODMA_RTAIL       0x200 /* rcvhdrtail register DMA enabled */
#define QIB_HAS_INTX          0x800 /* Supports INTx interrupts */
#define QIB_HAS_SEND_DMA      0x1000 /* Supports Send DMA */
#define QIB_HAS_VLSUPP        0x2000 /* Supports multiple VLs; PBC different */
#define QIB_BADINTR           0x8000 /* severe interrupt problems */
#define QIB_DCA_ENABLED       0x10000 /* Direct Cache Access enabled */
#define QIB_HAS_QSFP          0x20000 /* device (card instance) has QSFP */
#define ICHECK_WORKER_INITED  0x40000 /* initialized interrupt_check_worker */

/* IB dword length mask in PBC (lower 11 bits); same for all chips */
#define QIB_PBC_LENGTH_MASK                     ((1 << 11) - 1)


/* ctxt_flag bit offsets */
                /* context has been setup */
#define QIB_CTXT_SETUP_DONE 1
		/* waiting for a packet to arrive */
#define QIB_CTXT_WAITING_RCV   2
		/* master has not finished initializing */
#define QIB_CTXT_MASTER_UNINIT 4
		/* waiting for an urgent packet to arrive */
#define QIB_CTXT_WAITING_URG 5

/* free up any allocated data at closes */
void qib_free_data(struct qib_ctxtdata *dd);
struct hfi_devdata *qib_init_wfr_funcs(struct pci_dev *, const struct pci_device_id *);
void qib_free_devdata(struct hfi_devdata *);
struct hfi_devdata *qib_alloc_devdata(struct pci_dev *pdev, size_t extra);

void qib_dump_lookup_output_queue(struct hfi_devdata *);
void qib_clear_symerror_on_linkup(unsigned long opaque);

/*
 * Set LED override, only the two LSBs have "public" meaning, but
 * any non-zero value substitutes them for the Link and LinkTrain
 * LED states.
 */
#define QIB_LED_PHYS 1 /* Physical (linktraining) GREEN LED */
#define QIB_LED_LOG 2  /* Logical (link) YELLOW LED */
void qib_set_led_override(struct qib_pportdata *ppd, unsigned int val);

/* send dma routines */
void __qib_sdma_intr(struct qib_pportdata *);
void qib_sdma_intr(struct qib_pportdata *);
struct verbs_txreq;
int qib_sdma_verbs_send(struct sdma_engine *, struct qib_sge_state *,
			u32, struct verbs_txreq *);
/*
 * The number of words for the KDETH protocol field.  If this is
 * larger then the actual field used, then part of the payload
 * will be in the header.
 *
 * Optimally, we want this sized so that a typical case will
 * use full cache lines.  The typical local KDETH header would
 * be:
 *
 *	Bytes	Field
 *	  8	LRH
 *	 12	BHT
 *	 ??	KDETH
 *	  8	RHF
 *	---
 *	 28 + KDETH
 *
 * For a 64-byte cache line, KDETH would need to be 36 bytes or 9 DWORDS
 */
#define DEFAULT_RCVHDRSIZE 9

/*
 * Maximal header byte count:
 *
 *	Bytes	Field
 *	  8	LRH
 *	 40	GRH (optional)
 *	 12	BTH
 *	 ??	KDETH
 *	  8	RHF
 *	---
 *	 68 + KDETH
 *
 * We also want to maintain a cache line alignment to assist DMA'ing
 * of the header bytes.  Round up to a good size.
 */
#define DEFAULT_RCVHDR_ENTSIZE 32

int qib_get_user_pages(unsigned long, size_t, struct page **);
void qib_release_user_pages(struct page **, size_t);

static inline void qib_clear_rcvhdrtail(const struct qib_ctxtdata *rcd)
{
	*((u64 *) rcd->rcvhdrtail_kvaddr) = 0ULL;
}

static inline u32 qib_get_rcvhdrtail(const struct qib_ctxtdata *rcd)
{
	/*
	 * volatile because it's a DMA target from the chip, routine is
	 * inlined, and don't want register caching or reordering.
	 */
	return (u32) le64_to_cpu(
		*((volatile __le64 *)rcd->rcvhdrtail_kvaddr)); /* DMA'ed */
}

static inline u32 qib_get_hdrqtail(const struct qib_ctxtdata *rcd)
{
	const struct hfi_devdata *dd = rcd->dd;
	u32 hdrqtail;

	if (dd->flags & QIB_NODMA_RTAIL) {
		__le32 *rhf_addr;
		u32 seq;

		rhf_addr = (__le32 *) rcd->rcvhdrq +
			rcd->head + dd->rhf_offset;
		seq = rhf_rcv_seq(rhf_to_cpu(rhf_addr));
		hdrqtail = rcd->head;
		if (seq == rcd->seq_cnt)
			hdrqtail++;
	} else
		hdrqtail = qib_get_rcvhdrtail(rcd);

	return hdrqtail;
}

/*
 * sysfs interface.
 */

extern const char ib_qib_version[];

int hfi_device_create(struct hfi_devdata *);
void hfi_device_remove(struct hfi_devdata *);

int qib_create_port_files(struct ib_device *ibdev, u8 port_num,
			  struct kobject *kobj);
int qib_verbs_register_sysfs(struct hfi_devdata *);
void qib_verbs_unregister_sysfs(struct hfi_devdata *);
/* Hook for sysfs read of QSFP */
extern int qib_qsfp_dump(struct qib_pportdata *ppd, char *buf, int len);

int qib_pcie_init(struct pci_dev *, const struct pci_device_id *);
void hfi_pcie_cleanup(struct pci_dev *);
int qib_pcie_ddinit(struct hfi_devdata *, struct pci_dev *,
		    const struct pci_device_id *);
void qib_pcie_ddcleanup(struct hfi_devdata *);
void hfi_pcie_flr(struct hfi_devdata *);
int pcie_speeds(struct hfi_devdata *);
void request_msix(struct hfi_devdata *, u32 *, struct qib_msix_entry *);
void qib_enable_intx(struct pci_dev *);
void qib_nomsix(struct hfi_devdata *);
void restore_pci_variables(struct hfi_devdata *dd);
int do_pcie_gen3_transition(struct hfi_devdata *dd);
/* interrupts for device */
u64 hfi_int_counter(struct hfi_devdata *);

/*
 * dma_addr wrappers - all 0's invalid for hw
 */
dma_addr_t qib_map_page(struct pci_dev *, struct page *, unsigned long,
			  size_t, int);
const char *get_unit_name(int unit);

/*
 * Flush write combining store buffers (if present) and perform a write
 * barrier.
 */
#if defined(CONFIG_X86_64)
#define qib_flush_wc() asm volatile("sfence" : : : "memory")
#else
#define qib_flush_wc() wmb() /* no reorder around wc flush */
#endif

extern void handle_eflags(struct hfi_packet *packet);
extern void process_receive_ib(struct hfi_packet *packet);
extern void process_receive_bypass(struct hfi_packet *packet);
extern void process_receive_error(struct hfi_packet *packet);
extern void process_receive_expected(struct hfi_packet *packet);
extern void process_receive_eager(struct hfi_packet *packet);

extern void (*rhf_rcv_function_map[5])(struct hfi_packet *packet);

extern void update_sge(struct qib_sge_state *ss, u32 length);

/* global module parameter variables */
extern unsigned int max_mtu;
extern unsigned int default_mtu;
extern unsigned int hfi_cu;
extern unsigned int set_link_credits;
extern uint num_rcv_contexts;
extern unsigned qib_n_krcv_queues;
extern uint kdeth_qp;
extern uint loopback;
extern uint rcv_intr_timeout;
extern uint rcv_intr_count;
extern uint rcv_intr_dynamic;
extern uint fifo_check;
extern uint fifo_stalled_count;
extern ushort link_crc_mask;

extern struct mutex qib_mutex;

/* Number of seconds before our card status check...  */
#define STATUS_TIMEOUT 60

#define DRIVER_NAME		"hfi"
#define HFI_USER_MINOR_BASE     0
#define HFI_TRACE_MINOR         127
#define HFI_DIAGPKT_MINOR       128
#define HFI_DIAG_MINOR_BASE     129
#define HFI_SNOOP_CAPTURE_BASE  200
#define HFI_NMINORS             255

#define PCI_VENDOR_ID_INTEL 0x8086
#define PCI_DEVICE_ID_INTEL_WFR0 0x24f0
#define PCI_DEVICE_ID_INTEL_WFR1 0x24f1
#define PCI_DEVICE_ID_INTEL_WFR2 0x24f2

#define HFI_PKT_BASE_SC_INTEGRITY                                           \
	(WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_BYPASS_BAD_PKT_LEN_SMASK       \
	| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_PBC_STATIC_RATE_CONTROL_SMASK \
	| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_TOO_LONG_BYPASS_PACKETS_SMASK \
	| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_TOO_LONG_IB_PACKETS_SMASK     \
	| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_BAD_PKT_LEN_SMASK             \
	| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_PBC_TEST_SMASK                \
	| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_TOO_SMALL_BYPASS_PACKETS_SMASK\
	| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_TOO_SMALL_IB_PACKETS_SMASK    \
	| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_RAW_IPV6_SMASK                \
	| WFR_SEND_CTXT_CHECK_ENABLE_DISALLOW_RAW_SMASK                     \
	| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_BYPASS_VL_MAPPING_SMASK          \
	| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_VL_MAPPING_SMASK                 \
	| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_OPCODE_SMASK                     \
	| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_SLID_SMASK                       \
	| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_JOB_KEY_SMASK                    \
	| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_VL_SMASK                         \
	| WFR_SEND_CTXT_CHECK_ENABLE_CHECK_ENABLE_SMASK)

#define HFI_PKT_BASE_SDMA_INTEGRITY                                         \
	(WFR_SEND_DMA_CHECK_ENABLE_DISALLOW_BYPASS_BAD_PKT_LEN_SMASK        \
	| WFR_SEND_DMA_CHECK_ENABLE_DISALLOW_PBC_STATIC_RATE_CONTROL_SMASK  \
	| WFR_SEND_DMA_CHECK_ENABLE_DISALLOW_TOO_LONG_BYPASS_PACKETS_SMASK  \
	| WFR_SEND_DMA_CHECK_ENABLE_DISALLOW_TOO_LONG_IB_PACKETS_SMASK      \
	| WFR_SEND_DMA_CHECK_ENABLE_DISALLOW_BAD_PKT_LEN_SMASK              \
	| WFR_SEND_DMA_CHECK_ENABLE_DISALLOW_TOO_SMALL_BYPASS_PACKETS_SMASK \
	| WFR_SEND_DMA_CHECK_ENABLE_DISALLOW_TOO_SMALL_IB_PACKETS_SMASK     \
	| WFR_SEND_DMA_CHECK_ENABLE_DISALLOW_RAW_IPV6_SMASK                 \
	| WFR_SEND_DMA_CHECK_ENABLE_DISALLOW_RAW_SMASK                      \
	| WFR_SEND_DMA_CHECK_ENABLE_CHECK_BYPASS_VL_MAPPING_SMASK           \
	| WFR_SEND_DMA_CHECK_ENABLE_CHECK_VL_MAPPING_SMASK                  \
	| WFR_SEND_DMA_CHECK_ENABLE_CHECK_OPCODE_SMASK                      \
	| WFR_SEND_DMA_CHECK_ENABLE_CHECK_SLID_SMASK                        \
	| WFR_SEND_DMA_CHECK_ENABLE_CHECK_JOB_KEY_SMASK                     \
	| WFR_SEND_DMA_CHECK_ENABLE_CHECK_VL_SMASK                          \
	| WFR_SEND_DMA_CHECK_ENABLE_CHECK_ENABLE_SMASK)

/*
 * qib_early_err is used (only!) to print early errors before devdata is
 * allocated, or when dd->pcidev may not be valid, and at the tail end of
 * cleanup when devdata may have been freed, etc.  qib_dev_porterr is
 * the same as dd_dev_err, but is used when the message really needs
 * the IB port# to be definitive as to what's happening..
 */
#define qib_early_err(dev, fmt, ...) \
	dev_err(dev, fmt, ##__VA_ARGS__)

#define dd_dev_err(dd, fmt, ...) \
	dev_err(&(dd)->pcidev->dev, "%s: " fmt, \
			get_unit_name((dd)->unit), ##__VA_ARGS__)

#define dd_dev_info(dd, fmt, ...) \
	dev_info(&(dd)->pcidev->dev, "%s: " fmt, \
			get_unit_name((dd)->unit), ##__VA_ARGS__)

#define qib_dev_porterr(dd, port, fmt, ...) \
	dev_err(&(dd)->pcidev->dev, "%s: IB%u:%u " fmt, \
			get_unit_name((dd)->unit), (dd)->unit, (port), \
			##__VA_ARGS__)

/*
 * this is used for formatting hw error messages...
 */
struct qib_hwerror_msgs {
	u64 mask;
	const char *msg;
	size_t sz;
};

#define QLOGIC_IB_HWE_MSG(a, b) { .mask = a, .msg = b }

/* in qib_intr.c... */
void qib_format_hwerrors(u64 hwerrs,
			 const struct qib_hwerror_msgs *hwerrmsgs,
			 size_t nhwerrmsgs, char *msg, size_t lmsg);

#define WFR_USER_OPCODE_CHECK_VAL 0xC0
#define WFR_USER_OPCODE_CHECK_MASK 0xC0
#define WFR_OPCODE_CHECK_VAL_DISABLED 0x0
#define WFR_OPCODE_CHECK_MASK_DISABLED 0x0

#endif                          /* _QIB_KERNEL_H */
