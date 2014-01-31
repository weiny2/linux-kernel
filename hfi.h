#ifndef _QIB_KERNEL_H
#define _QIB_KERNEL_H
/*
 * Copyright (c) 2012 Intel Corporation.  All rights reserved.
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
struct qib_ctxtdata {
	void **rcvegrbuf;
	dma_addr_t *rcvegrbuf_phys;
	/* rcvhdrq base, needs mmap before useful */
	void *rcvhdrq;
	/* kernel virtual address where hdrqtail is updated */
	void *rcvhdrtail_kvaddr;
	/*
	 * temp buffer for expected send setup, allocated at open, instead
	 * of each setup call
	 */
	void *tid_pg_list;
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
	/* this receive context's assigned PIO send context */
	struct send_context *sc;

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
	/* number of eager TID entries. */
	u32 eager_count;
	/* index of first eager TID entry. */
	u32 eager_base;
	/* number of expected TID entries */
	u32 expected_count;
	/* index of first expected TID entry. */
	u32 expected_base;
	/* number of pio bufs for this ctxt (all procs, if shared) */
	u32 piocnt;
	/* first pio buffer for this ctxt */
	u32 pio_base;
	/* chip offset of PIO buffers for this ctxt */
	u32 piobufs;
	/* how many alloc_pages() chunks in rcvegrbuf_pages */
	u32 rcvegrbuf_chunks;
	/* how many egrbufs per chunk */
	u16 rcvegrbufs_perchunk;
	/* ilog2 of above */
	u16 rcvegrbufs_perchunk_shift;
	/* order for rcvegrbuf_pages */
	size_t rcvegrbuf_size;
	/* rcvhdrq size (for freeing) */
	size_t rcvhdrq_size;
	/* per-context flags for fileops/intr communication */
	unsigned long flag;
	/* next expected TID to check when looking for free */
	u32 tidcursor;
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
	/* pkeys set by this use of this ctxt */
	u16 pkeys[4];
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
};

struct qib_sge_state;

struct qib_sdma_txreq {
	int                 flags;
	int                 sg_count;
	dma_addr_t          addr;
	void              (*callback)(struct qib_sdma_txreq *, int);
	u16                 start_idx;  /* sdma private */
	u16                 next_descq_idx;  /* sdma private */
	struct list_head    list;       /* sdma private */
};

struct qib_sdma_desc {
	__le64 qw[2];
};

struct qib_verbs_txreq {
	struct qib_sdma_txreq   txreq;
	struct qib_qp           *qp;
	struct qib_swqe         *wqe;
	u32                     dwords;
	u16                     hdr_dwords;
	u16                     hdr_inx;
	struct qib_pio_header	*align_buf;
	struct qib_mregion	*mr;
	struct qib_sge_state    *ss;
};

#define QIB_SDMA_TXREQ_F_USELARGEBUF  0x1
#define QIB_SDMA_TXREQ_F_HEADTOHOST   0x2
#define QIB_SDMA_TXREQ_F_INTREQ       0x4
#define QIB_SDMA_TXREQ_F_FREEBUF      0x8
#define QIB_SDMA_TXREQ_F_FREEDESC     0x10

#define QIB_SDMA_TXREQ_S_OK        0
#define QIB_SDMA_TXREQ_S_SENDERROR 1
#define QIB_SDMA_TXREQ_S_ABORTED   2
#define QIB_SDMA_TXREQ_S_SHUTDOWN  3

/*
 * Get/Set IB link-level config parameters for f_get/set_ib_cfg()
 * Mostly for MADs that set or query link parameters, also ipath
 * config interfaces
 */
#define QIB_IB_CFG_LIDLMC 0 /* LID (LS16b) and Mask (MS16b) */
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
#define QIB_IB_CFG_LSTATE 18 /* update linkcmd and linkinitcmd in IBC */
#define QIB_IB_CFG_VL_HIGH_LIMIT 19
#define QIB_IB_CFG_PMA_TICKS 20 /* PMA sample tick resolution */
#define QIB_IB_CFG_PORT 21 /* switch port we are connected to */

/*
 * for CFG_LSTATE: LINKCMD in upper 16 bits, LINKINITCMD in lower 16
 * IB_LINKINITCMD_POLL and SLEEP are also used as set/get values for
 * QIB_IB_CFG_LINKDEFAULT cmd
 */
#define   IB_LINKCMD_DOWN   (0 << 16)
#define   IB_LINKCMD_ARMED  (1 << 16)
#define   IB_LINKCMD_ACTIVE (2 << 16)
#define   IB_LINKINITCMD_NOP     0
#define   IB_LINKINITCMD_POLL    1
#define   IB_LINKINITCMD_SLEEP   2
#define   IB_LINKINITCMD_DISABLE 3

/*
 * valid states passed to qib_set_linkstate() user call
 */
#define QIB_IB_LINKDOWN         0
#define QIB_IB_LINKARM          1
#define QIB_IB_LINKACTIVE       2
#define QIB_IB_LINKDOWN_ONLY    3
#define QIB_IB_LINKDOWN_SLEEP   4
#define QIB_IB_LINKDOWN_DISABLE 5

/* use this MTU size if none other is given */
#define HFI_DEFAULT_ACTIVE_MTU 4096
/* use this MTU size as the default maximum */
/* TODO: more work is needed for the 8K and 10K STL sizes */
/* TODO: bad things may happen if 8K and 10K STL sizes used with an IB FM,
   e.g. opensm */
#define HFI_DEFAULT_MAX_MTU 4096
/* default parition key */
#define DEFAULT_PKEY 0xffff

/*
 * Possible IB config parameters for f_get/set_ib_table()
 */
#define QIB_IB_TBL_VL_HIGH_ARB 1 /* Get/set VL high priority weights */
#define QIB_IB_TBL_VL_LOW_ARB 2 /* Get/set VL low priority weights */

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

enum qib_sdma_states {
	qib_sdma_state_s00_hw_down,
	qib_sdma_state_s10_hw_start_up_wait,
	qib_sdma_state_s20_idle,
	qib_sdma_state_s30_sw_clean_up_wait,
	qib_sdma_state_s40_hw_clean_up_wait,
	qib_sdma_state_s50_hw_halt_wait,
	qib_sdma_state_s99_running,
};

enum qib_sdma_events {
	qib_sdma_event_e00_go_hw_down,
	qib_sdma_event_e10_go_hw_start,
	qib_sdma_event_e20_hw_started,
	qib_sdma_event_e30_go_running,
	qib_sdma_event_e40_sw_cleaned,
	qib_sdma_event_e50_hw_cleaned,
	qib_sdma_event_e60_hw_halted,
	qib_sdma_event_e70_go_idle,
	qib_sdma_event_e7220_err_halted,
	qib_sdma_event_e7322_err_halted,
	qib_sdma_event_e90_timer_tick,
};

extern char *qib_sdma_state_names[];
extern char *qib_sdma_event_names[];

struct sdma_set_state_action {
	unsigned op_enable:1;
	unsigned op_intenable:1;
	unsigned op_halt:1;
	unsigned op_drain:1;
	unsigned go_s99_running_tofalse:1;
	unsigned go_s99_running_totrue:1;
};

struct qib_sdma_state {
	struct kref          kref;
	struct completion    comp;
	enum qib_sdma_states current_state;
	struct sdma_set_state_action *set_state_action;
	unsigned             current_op;
	unsigned             go_s99_running;
	unsigned             first_sendbuf;
	unsigned             last_sendbuf; /* really last +1 */
	/* debugging/devel */
	enum qib_sdma_states previous_state;
	unsigned             previous_op;
	enum qib_sdma_events last_event;
};

struct xmit_wait {
	struct timer_list timer;
	u64 counter;
	u8 flags;
	struct cache {
		u64 psxmitdata;
		u64 psrcvdata;
		u64 psxmitpkts;
		u64 psrcvpkts;
		u64 psxmitwait;
	} counter_cache;
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
	struct kobject sl2vl_kobj;
	struct kobject diagc_kobj;

	/* GUID for this interface, in network order */
	__be64 guid;

	/* up or down physical link state */
	u32 linkup;

	/* ref count for each pkey */
	atomic_t pkeyrefs[4];

	/*
	 * this address is mapped readonly into user processes so they can
	 * get status cheaply, whenever they want.  One qword of status per port
	 */
	u64 *statusp;

	/* SendDMA related entries */

	/* read mostly */
	struct qib_sdma_desc *sdma_descq;
	struct workqueue_struct *qib_wq;
	struct qib_sdma_state sdma_state;
	dma_addr_t       sdma_descq_phys;
	volatile __le64 *sdma_head_dma; /* DMA'ed by chip */
	dma_addr_t       sdma_head_phys;
	u16                   sdma_descq_cnt;

	/* read/write using lock */
	spinlock_t            sdma_lock ____cacheline_aligned_in_smp;
	struct list_head      sdma_activelist;
	u64                   sdma_descq_added;
	u64                   sdma_descq_removed;
	u16                   sdma_descq_tail;
	u16                   sdma_descq_head;
	u8                    sdma_generation;

	struct tasklet_struct sdma_sw_clean_up_task
		____cacheline_aligned_in_smp;

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
	u16 pkeys[4];
	/* LID mask control */
	u8 lmc;
	u8 link_width_supported;
	u8 link_speed_supported;
	u8 link_width_enabled;
	u8 link_speed_enabled;
	u8 link_width_active;
	u8 link_speed_active;
	u8 vls_supported;
	u8 vls_operational;
	/* Rx Polarity inversion (compensate for ~tx on partner) */
	u8 rx_pol_inv;

	u8 hw_pidx;     /* physical port index */
	u8 port;        /* IB port number and index into dd->pports - 1 */

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
	struct xmit_wait cong_stats;
	struct timer_list symerr_clear_timer;

	/* Synchronize access between driver writes and sysfs reads */
	spinlock_t cc_shadow_lock
		____cacheline_aligned_in_smp;

	/* Shadow copy of the congestion control table */
	struct cc_table_shadow *ccti_entries_shadow;

	/* Shadow copy of the congestion control entries */
	struct ib_cc_congestion_setting_attr_shadow *congestion_entries_shadow;

	/* List of congestion control table entries */
	struct ib_cc_table_entry_shadow *ccti_entries;

	/* 16 congestion entries with each entry corresponding to a SL */
	struct ib_cc_congestion_entry_shadow *congestion_entries;

	/* Maximum number of congestion control entries that the agent expects
	 * the manager to send.
	 */
	u16 cc_supported_table_entries;

	/* Total number of congestion control table entries */
	u16 total_cct_entry;

	/* Bit map identifying service level */
	u16 cc_sl_control_map;

	/* maximum congestion control table index */
	u16 ccti_limit;

	/* CA's max number of 64 entry units in the congestion control table */
	u8 cc_max_table_entries;
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

/*
 * Data pertaining to each SDMA engine.
 */
struct sdma_engine {
	struct hfi_devdata *dd;
	int which;			/* which engine */
	u64 imask;			/* clear interrupt mask */
	/* add sdma fields here... */
};

/* device data struct now contains only "general per-device" info.
 * fields related to a physical IB port are in a qib_pportdata struct,
 * described above) while fields only used by a particular chip-type are in
 * a qib_chipdata struct, whose contents are opaque to this file.
 */
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
	/* Send Context initialization lock. */
	spinlock_t sc_init_lock;

	/* qib_pportdata, points to array of (physical) port-specific
	 * data structs, indexed by pidx (0..n-1)
	 */
	struct qib_pportdata *pport;

	/* mem-mapped pointer to base of PIO buffers */
	void __iomem *piobase;
	/*
	 * credit return base - a per-NUMA range of DMA address that
	 * the chip will use to update the per-context free counter
	 */
	struct credit_return_base *cr_base;

	/* send context numbers and sizes for each type */
	struct sc_config_sizes sc_sizes[SC_MAX];

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
	void (*f_put_tid)(struct hfi_devdata *, u32, u32, unsigned long);
	void (*f_cleanup)(struct hfi_devdata *);
	void (*f_setextled)(struct qib_pportdata *, u32);
	/* fill out chip-specific fields */
	int (*f_get_base_info)(struct qib_ctxtdata *, struct qib_base_info *);
	/* free irq */
	void (*f_free_irq)(struct hfi_devdata *);
	struct qib_message_header *(*f_get_msgheader)
					(struct hfi_devdata *, __le32 *);
	void (*f_config_ctxts)(struct hfi_devdata *);
	int (*f_get_ib_cfg)(struct qib_pportdata *, int);
	int (*f_set_ib_cfg)(struct qib_pportdata *, int, u32);
	int (*f_set_ib_loopback)(struct qib_pportdata *, const char *);
	int (*f_get_ib_table)(struct qib_pportdata *, int, void *);
	int (*f_set_ib_table)(struct qib_pportdata *, int, void *);
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
	int (*f_init_sdma_regs)(struct qib_pportdata *);
	u16 (*f_sdma_gethead)(struct qib_pportdata *);
	int (*f_sdma_busy)(struct qib_pportdata *);
	void (*f_sdma_update_tail)(struct qib_pportdata *, u16);
	void (*f_sdma_set_desc_cnt)(struct qib_pportdata *, unsigned);
	void (*f_sdma_sendctrl)(struct qib_pportdata *, unsigned);
	void (*f_sdma_hw_clean_up)(struct qib_pportdata *);
	void (*f_sdma_hw_start_up)(struct qib_pportdata *);
	void (*f_sdma_init_early)(struct qib_pportdata *);
	void (*f_set_cntr_sample)(struct qib_pportdata *, u32, u32);
	void (*f_update_usrhead)(struct qib_ctxtdata *, u64, u32, u32, u32);
	u32 (*f_hdrqempty)(struct qib_ctxtdata *);
	u64 (*f_portcntr)(struct qib_pportdata *, u32);
	u32 (*f_read_cntrs)(struct hfi_devdata *, loff_t, char **,
		u64 **);
	u32 (*f_read_portcntrs)(struct hfi_devdata *, loff_t, u32,
		char **, u64 **);
	void (*f_init_ctxt)(struct qib_ctxtdata *);
	int (*f_tempsense_rd)(struct hfi_devdata *, int regnum);

	char *boardname; /* human readable board info */

	/* number of registers used for pioavail */
	u32 pioavregs;
	/* device (not port) flags, basically device capabilities */
	u32 flags;

	/* saturating counter of (non-port-specific) device interrupts */
	u32 int_counter;

	/* number of receive contexts in use by the driver */
	u32 num_rcv_contexts;
	/* number of pio send contexts in use by the driver */
	u32 num_send_contexts;
	/*
	 * number of ctxts available for PSM open
	 */
	u32 freectxts;

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

	/* shadow copy of struct page *'s for exp tid pages */
	struct page **pageshadow;
	/* shadow copy of dma handles for exp tid pages */
	dma_addr_t *physshadow;
	u64 __iomem *egrtidbase;
	spinlock_t sendctrl_lock; /* protect changes to sendctrl shadow */
	/* around rcd and (user ctxts) ctxt_cnt use (intr vs free) */
	spinlock_t uctxt_lock; /* rcd and user context changes */
	/* exclusive access to 8051 */
	spinlock_t dc8051_lock;
	/*
	 * per unit status, see also portdata statusp
	 * mapped readonly into user processes so they can get unit and
	 * IB link status cheaply
	 */
	u64 *devstatusp;
	char *freezemsg; /* freeze msg if hw error put chip in freeze */
	u32 freezelen; /* max length of freezemsg */
	/* timer used to prevent stats overflow, error throttling, etc. */
	struct timer_list stats_timer;

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
	/* localbus width (1, 2,4,8,16,32) from config space  */
	u32 lbus_width;
	/* localbus speed in MHz */
	u32 lbus_speed;
	int unit; /* unit # of this chip */

	/* so we can rewrite it after a chip reset */
	u32 pcibar0;
	/* so we can rewrite it after a chip reset */
	u32 pcibar1;
	u64 rhdrhead_intr_off;

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

	/* Misc small ints */
	/* Number of physical ports available */
	u8 num_pports;
	/* Lowest context number which can be used by user processes */
	u8 first_user_ctxt;
	u8 n_krcv_queues;
	u8 qpn_mask;

	u16 rhf_offset; /* offset of RHF within receive header entry */

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

	u32 chip_sdma_engines;	/* number from the chip */
	u32 num_sdma;		/* number being used */
	struct sdma_engine per_sdma[WFR_TXE_NUM_SDMA_ENGINES];
	/*
	 * Simple receive array allocation: the number per receive array entries
	 * per context.
	 */
	u32 rcv_entries;
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
struct qib_filedata {
	struct qib_ctxtdata *rcd;
	unsigned subctxt;
	unsigned tidcursor;
	struct qib_user_sdma_queue *pq;
	int rec_cpu_num; /* for cpu affinity; -1 if none */
};

extern struct list_head qib_dev_list;
extern spinlock_t qib_devs_lock;
extern struct hfi_devdata *qib_lookup(int unit);
extern u32 qib_cpulist_count;
extern unsigned long *qib_cpulist;

extern unsigned qib_cc_table_size;
int qib_init(struct hfi_devdata *, int);
int qib_count_units(int *npresentp, int *nupp);
int qib_count_active_units(void);

int qib_diag_add(struct hfi_devdata *);
void qib_diag_remove(struct hfi_devdata *);
void handle_linkup_change(struct hfi_devdata *dd, u32 linkup);
void qib_sdma_update_tail(struct qib_pportdata *, u16); /* hold sdma_lock */

int qib_decode_err(struct hfi_devdata *dd, char *buf, size_t blen, u64 err);
void qib_bad_intrstatus(struct hfi_devdata *);
void qib_handle_urcv(struct hfi_devdata *, u64);

/* clean up any per-chip chip-specific stuff */
void qib_chip_cleanup(struct hfi_devdata *);
/* clean up any chip type-specific stuff */
void qib_chip_done(void);

struct send_context *qp_to_send_context(struct qib_qp *qp);

int qib_create_rcvhdrq(struct hfi_devdata *, struct qib_ctxtdata *);
int qib_setup_eagerbufs(struct qib_ctxtdata *);
int qib_create_ctxts(struct hfi_devdata *dd);
struct qib_ctxtdata *qib_create_ctxtdata(struct qib_pportdata *, u32);
void qib_init_pportdata(struct qib_pportdata *, struct hfi_devdata *, u8, u8);
void qib_free_ctxtdata(struct hfi_devdata *, struct qib_ctxtdata *);

void handle_receive_interrupt(struct qib_ctxtdata *);
int qib_reset_device(int);
int qib_wait_linkstate(struct qib_pportdata *, u32, int);
int qib_set_linkstate(struct qib_pportdata *, u8);

/* MTU handling */

/* MTU enumeration, 256-4k match IB */
#define STL_MTU_256   1
#define STL_MTU_512   2
#define STL_MTU_1024  3
#define STL_MTU_2048  4
#define STL_MTU_4096  5
#define STL_MTU_8192  8
#define STL_MTU_10240 9

u32 lrh_max_header_bytes(struct hfi_devdata *dd);
int mtu_to_enum(u32 mtu, int default_if_bad);
static inline int valid_mtu(unsigned int mtu)
{
	return mtu == 256|| mtu == 512
		|| mtu == 1024 || mtu == 2048
		|| mtu == 4096 || mtu == 8192
		|| mtu == 10240;
}
int set_mtu(struct qib_pportdata *, u16);

int qib_set_lid(struct qib_pportdata *, u32, u8);
void qib_disable_after_error(struct hfi_devdata *);
int qib_set_uevent_bits(struct qib_pportdata *, const int);

/* for use in system calls, where we want to know device type, etc. */
#define ctxt_fp(fp) \
	(((struct qib_filedata *)(fp)->private_data)->rcd)
#define subctxt_fp(fp) \
	(((struct qib_filedata *)(fp)->private_data)->subctxt)
#define tidcursor_fp(fp) \
	(((struct qib_filedata *)(fp)->private_data)->tidcursor)
#define user_sdma_queue_fp(fp) \
	(((struct qib_filedata *)(fp)->private_data)->pq)

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
#define QIB_HAS_HDRSUPP       0x4000 /* Supports header suppression */
#define QIB_BADINTR           0x8000 /* severe interrupt problems */
#define QIB_DCA_ENABLED       0x10000 /* Direct Cache Access enabled */
#define QIB_HAS_QSFP          0x20000 /* device (card instance) has QSFP */
#define ICHECK_WORKER_INITED  0x40000 /* initialized interrupt_check_worker */

/* IB dword length mask in PBC (lower 11 bits); same for all chips */
#define QIB_PBC_LENGTH_MASK                     ((1 << 11) - 1)


/* ctxt_flag bit offsets */
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
int qib_setup_sdma(struct qib_pportdata *);
void qib_teardown_sdma(struct qib_pportdata *);
void __qib_sdma_intr(struct qib_pportdata *);
void qib_sdma_intr(struct qib_pportdata *);
int qib_sdma_verbs_send(struct qib_pportdata *, struct qib_sge_state *,
			u32, struct qib_verbs_txreq *);
/* ppd->sdma_lock should be locked before calling this. */
int qib_sdma_make_progress(struct qib_pportdata *dd);

static inline int qib_sdma_empty(const struct qib_pportdata *ppd)
{
	return ppd->sdma_descq_added == ppd->sdma_descq_removed;
}

/* must be called under qib_sdma_lock */
static inline u16 qib_sdma_descq_freecnt(const struct qib_pportdata *ppd)
{
	return ppd->sdma_descq_cnt -
		(ppd->sdma_descq_added - ppd->sdma_descq_removed) - 1;
}

static inline int __qib_sdma_running(struct qib_pportdata *ppd)
{
	return ppd->sdma_state.current_state == qib_sdma_state_s99_running;
}
int qib_sdma_running(struct qib_pportdata *);

void __qib_sdma_process_event(struct qib_pportdata *, enum qib_sdma_events);
void qib_sdma_process_event(struct qib_pportdata *, enum qib_sdma_events);

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
 * For a 64-byte cache line, KETH would need to be 36 bytes or 9 DWORDS
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
		seq = rhf_rcv_seq(rhf_addr);
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

int qib_device_create(struct hfi_devdata *);
void qib_device_remove(struct hfi_devdata *);

int qib_create_port_files(struct ib_device *ibdev, u8 port_num,
			  struct kobject *kobj);
int qib_verbs_register_sysfs(struct hfi_devdata *);
void qib_verbs_unregister_sysfs(struct hfi_devdata *);
/* Hook for sysfs read of QSFP */
extern int qib_qsfp_dump(struct qib_pportdata *ppd, char *buf, int len);

int __init qib_init_qibfs(void);
int __exit qib_exit_qibfs(void);

int qibfs_add(struct hfi_devdata *);
int qibfs_remove(struct hfi_devdata *);

int qib_pcie_init(struct pci_dev *, const struct pci_device_id *);
int qib_pcie_ddinit(struct hfi_devdata *, struct pci_dev *,
		    const struct pci_device_id *);
void qib_pcie_ddcleanup(struct hfi_devdata *);
void hfi_pcie_flr(struct hfi_devdata *);
int qib_pcie_params(struct hfi_devdata *, u32, u32 *, struct qib_msix_entry *);
void qib_enable_intx(struct pci_dev *);
void qib_nomsix(struct hfi_devdata *);
void qib_pcie_getcmd(struct hfi_devdata *, u16 *, u8 *, u8 *);
void qib_pcie_reenable(struct hfi_devdata *, u16, u8, u8);

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

/* global module parameter variables */
extern unsigned int max_mtu;
extern unsigned int default_mtu;
extern unsigned int hfi_cu;
extern uint num_rcv_contexts;
extern unsigned qib_n_krcv_queues;
extern uint kdeth_qp;

extern struct mutex qib_mutex;

/* Number of seconds before our card status check...  */
#define STATUS_TIMEOUT 60

#define DRIVER_NAME		"hfi"
#define HFI_USER_MINOR_BASE     0
#define HFI_TRACE_MINOR         127
#define HFI_DIAGPKT_MINOR       128
#define HFI_DIAG_MINOR_BASE     129
#define HFI_NMINORS             255

#define PCI_VENDOR_ID_INTEL 0x8086
#define PCI_DEVICE_ID_INTEL_WFR0 0x24f0
#define PCI_DEVICE_ID_INTEL_WFR1 0x24f1
#define PCI_DEVICE_ID_INTEL_WFR2 0x24f2

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
#endif                          /* _QIB_KERNEL_H */
