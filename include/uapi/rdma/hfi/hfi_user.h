/*
 * Copyright (c) 2014, 2015 Intel Corporation. All rights reserved.
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
 * This file contains defines, structures, etc. that are used
 * to communicate between kernel and user code.
 */

#ifndef _LINUX__HFI_USER_H
#define _LINUX__HFI_USER_H

#include <linux/types.h>

/*
 * This version number is given to the driver by the user code during
 * initialization in the spu_userversion field of hfi_user_info, so
 * the driver can check for compatibility with user code.
 *
 * The major version changes when data structures change in an incompatible
 * way. The driver must be the same for initialization to succeed.
 */
#define HFI_USER_SWMAJOR 4

/*
 * Minor version differences are always compatible
 * a within a major version, however if user software is larger
 * than driver software, some new features and/or structure fields
 * may not be implemented; the user code must deal with this if it
 * cares, or it must abort after initialization reports the difference.
 */
#define HFI_USER_SWMINOR 0

/*
 * Set of HW and driver capability/feature bits.
 * These bit values are used to configure enabled/disabled HW and
 * driver features. The same set of bits are communicated to user
 * space.
 */
#define HFI_CAP_DMA_RTAIL        (1UL <<  0) /* Use DMA'ed RTail value */
#define HFI_CAP_SDMA             (1UL <<  1) /* Enable SDMA support */
#define HFI_CAP_SDMA_AHG         (1UL <<  2) /* Enable SDMA AHG support */
#define HFI_CAP_EXTENDED_PSN     (1UL <<  3) /* Enable Extended PSN support */
#define HFI_CAP_HDRSUPP          (1UL <<  4) /* Enable Header Suppression */
#define HFI_CAP_ENABLE_SMA       (1UL <<  5) /* Enable driver SM Agent */
#define HFI_CAP_USE_DMA_HEAD     (1UL <<  6) /* DMA Hdr Q tail vs. use CSR */
#define HFI_CAP_MULTI_PKT_EGR    (1UL <<  7) /* Enable multipacket Egr buffs */
#define HFI_CAP_NODROP_RHQ_FULL  (1UL <<  8) /* Don't drop on Hdr Q full */
#define HFI_CAP_NODROP_EGR_FULL  (1UL <<  9) /* Don't drop on EGR buffs full */
#define HFI_CAP_TID_UNMAP        (1UL << 10) /* Enable Expected TID caching */
#define HFI_CAP_PRINT_UNIMPL     (1UL << 11) /* Show for unimplemented feats */
#define HFI_CAP_ALLOW_PERM_JKEY  (1UL << 12) /* Allow use of permissive JKEY */
#define HFI_CAP_NO_INTEGRITY     (1UL << 13) /* Enable ctxt integrity checks */
#define HFI_CAP_PKEY_CHECK       (1UL << 14) /* Enable ctxt PKey checking */
#define HFI_CAP_STATIC_RATE_CTRL (1UL << 15) /* Allow PBC.StaticRateControl */
#define HFI_CAP_QSFP_ENABLED     (1UL << 16) /* Enable QSFP check during LNI */

#define HFI_RCVHDR_ENTSIZE_2    (1UL << 0)
#define HFI_RCVHDR_ENTSIZE_16   (1UL << 1)
#define HFI_RCVDHR_ENTSIZE_32   (1UL << 2)

/*
 * If the unit is specified via open, HCA choice is fixed.  If port is
 * specified, it's also fixed.  Otherwise we try to spread contexts
 * across ports and HCAs, using different algorithims.  WITHIN is
 * the old default, prior to this mechanism.
 */
#define HFI_ALG_ACROSS 0 /* round robin contexts across HCAs, then
			  * ports; this is the default */
#define HFI_ALG_WITHIN 1 /* use all contexts on an HCA (round robin
			  * active ports within), then next HCA */
#define HFI_ALG_COUNT  2 /* number of algorithm choices */


/* User commands. */
#define HFI_CMD_ASSIGN_CTXT     1	/* allocate HCA and context */
#define HFI_CMD_CTXT_INFO       2	/* find out what resources we got */
#define HFI_CMD_USER_INFO       3	/* set up userspace */
#define HFI_CMD_TID_UPDATE      4	/* update expected TID entries */
#define HFI_CMD_TID_FREE        5	/* free expected TID entries */
#define HFI_CMD_CREDIT_UPD      6	/* force an update of PIO credit */
#define HFI_CMD_SDMA_STATUS_UPD 7       /* force update of SDMA status ring */

#define HFI_CMD_RECV_CTRL       8	/* control receipt of packets */
#define HFI_CMD_POLL_TYPE       9	/* set the kind of polling we want */
#define HFI_CMD_ACK_EVENT       10	/* ack & clear user status bits */
#define HFI_CMD_SET_PKEY        11      /* set context's pkey */
#define HFI_CMD_CTXT_RESET      12      /* reset context's HW send context */
/* separate EPROM commands from normal PSM commands */
#define HFI_CMD_EP_INFO         64      /* read EPROM device ID */
#define HFI_CMD_EP_ERASE_CHIP   65      /* erase whole EPROM */
#define HFI_CMD_EP_ERASE_P0     66      /* erase EPROM partition 0 */
#define HFI_CMD_EP_ERASE_P1     67      /* erase EPROM partition 1 */
#define HFI_CMD_EP_READ_P0      68      /* read EPROM partition 0 */
#define HFI_CMD_EP_READ_P1      69      /* read EPROM partition 1 */
#define HFI_CMD_EP_WRITE_P0     70      /* write EPROM partition 0 */
#define HFI_CMD_EP_WRITE_P1     71      /* write EPROM partition 1 */

/*
 * HFI_CMD_ACK_EVENT obsoletes HFI_CMD_DISARM_BUFS, but we keep it for
 * compatibility with libraries from previous release.   The ACK_EVENT
 * will take appropriate driver action (if any, just DISARM for now),
 * then clear the bits passed in as part of the mask.  These bits are
 * in the first 64bit word at spi_sendbuf_status, and are passed to
 * the driver in the event_mask union as well.
 */
#define _HFI_EVENT_DISARM_BUFS_BIT  0
#define _HFI_EVENT_LINKDOWN_BIT     1
#define _HFI_EVENT_LID_CHANGE_BIT   2
#define _HFI_EVENT_LMC_CHANGE_BIT   3
#define _HFI_EVENT_SL2VL_CHANGE_BIT 4
#define _HFI_MAX_EVENT_BIT _HFI_EVENT_SL2VL_CHANGE_BIT

#define HFI_EVENT_DISARM_BUFS_BIT	(1UL << _HFI_EVENT_DISARM_BUFS_BIT)
#define HFI_EVENT_LINKDOWN_BIT		(1UL << _HFI_EVENT_LINKDOWN_BIT)
#define HFI_EVENT_LID_CHANGE_BIT	(1UL << _HFI_EVENT_LID_CHANGE_BIT)
#define HFI_EVENT_LMC_CHANGE_BIT	(1UL << _HFI_EVENT_LMC_CHANGE_BIT)
#define HFI_EVENT_SL2VL_CHANGE_BIT	(1UL << _HFI_EVENT_SL2VL_CHANGE_BIT)

/*
 * These are the status bits readable (in ascii form, 64bit value)
 * from the "status" sysfs file.  For binary compatibility, values
 * must remain as is; removed states can be reused for different
 * purposes.
 */
#define HFI_STATUS_INITTED       0x1    /* basic initialization done */
/* Chip has been found and initted */
#define HFI_STATUS_CHIP_PRESENT 0x20
/* IB link is at ACTIVE, usable for data traffic */
#define HFI_STATUS_IB_READY     0x40
/* link is configured, LID, MTU, etc. have been set */
#define HFI_STATUS_IB_CONF      0x80
/* A Fatal hardware error has occurred. */
#define HFI_STATUS_HWERROR     0x200

/*
 * Number of supported shared contexts.
 * This is the maximum number of software contexts that can share
 * a hardware send/receive context.
 */
#define HFI_MAX_SHARED_CTXTS 8

/*
 * Poll types
 */
#define HFI_POLL_TYPE_ANYRCV     0x0
#define HFI_POLL_TYPE_URGENT     0x1

/*
 * This structure is passed to qib_userinit() to tell the driver where
 * user code buffers are, sizes, etc.   The offsets and sizes of the
 * fields must remain unchanged, for binary compatibility.  It can
 * be extended, if userversion is changed so user code can tell, if needed
 */
struct hfi_user_info {
	/*
	 * version of user software, to detect compatibility issues.
	 * Should be set to HFI_USER_SWVERSION.
	 */
	__u32 userversion;
	__u16 pad;
	/* HFI selection algorithm, if unit has not selected */
	__u16 hfi_alg;
	/*
	 * If two or more processes wish to share a context, each process
	 * must set the subcontext_cnt and subcontext_id to the same
	 * values.  The only restriction on the subcontext_id is that
	 * it be unique for a given node.
	 */
	__u16 subctxt_cnt;
	__u16 subctxt_id;
	/* 128bit UUID passed in by PSM. */
	__u8 uuid[16];
};

struct hfi_ctxt_info {
	__u64 runtime_flags;    /* chip/drv runtime flags (HFI_CAP_*) */
	__u32 rcvegr_size;      /* size of each eager buffer */
	__u16 num_active;       /* number of active units */
	__u16 unit;             /* unit (chip) assigned to caller */
	__u16 ctxt;             /* ctxt on unit assigned to caller */
	__u16 subctxt;          /* subctxt on unit assigned to caller */
	__u16 rcvtids;          /* number of Rcv TIDs for this context */
	__u16 credits;          /* number of PIO credits for this context */
	__u16 numa_node;        /* NUMA node of the assigned device */
	__u16 rec_cpu;          /* cpu # for affinity (ffff if none) */
	__u16 send_ctxt;        /* send context in use by this user context */
	__u16 egrtids;          /* number of RcvArray entries for Eager Rcvs */
	__u16 rcvhdrq_cnt;      /* number of RcvHdrQ entries */
	__u16 rcvhdrq_entsize;  /* size (in bytes) for each RcvHdrQ entry */
	__u16 sdma_ring_size;   /* number of entries in SDMA request ring */
};

struct hfi_tid_info {
	/* virtual address of first page in transfer */
	__u64 vaddr;
	/* pointer to tid array. this array is big enough */
	__u64 tidlist;
	/* number of tids programmed by this request */
	__u32 tidcnt;
	/* length of transfer buffer programmed by this request */
	__u32 length;
	/*
	 * pointer to bitmap of TIDs used for this call;
	 * checked for being large enough at open
	 */
	__u64 tidmap;
};

struct hfi_cmd {
	__u32 type;        /* command type */
	__u32 len;         /* length of struct pointed to by add */
	__u64 addr;        /* pointer to user structure */
};

enum hfi_sdma_comp_state {
	FREE = 0,
	QUEUED,
	COMPLETE,
	ERROR
};

/*
 * SDMA completion ring entry
 */
struct hfi_sdma_comp_entry {
	__u32 status;
	__u32 errcode;
};

/*
 * Device status and notifications from driver to user-space.
 */
struct hfi_status {
	__u64 dev;      /* device/hw status bits */
	__u64 port;     /* port state and status bits */
	char freezemsg[0];
};

/*
 * This structure is returned by qib_userinit() immediately after
 * open to get implementation-specific info, and info specific to this
 * instance.
 *
 * This struct must have explicit pad fields where type sizes
 * may result in different alignments between 32 and 64 bit
 * programs, since the 64 bit * bit kernel requires the user code
 * to have matching offsets
 */
struct hfi_base_info {
	/* version of hardware, for feature checking. */
	__u32 hw_version;
	/* version of software, for feature checking. */
	__u32 sw_version;
	/* Job key */
	__u16 jkey;
	__u16 padding1;
	/*
	 * The special QP (queue pair) value that identifies PSM
	 * protocol packet from standard IB packets.
	 */
	__u32 bthqp;
	/* PIO credit return address, */
	__u64 sc_credits_addr;
	/*
	 * Base address of writeonly pio buffers for this process.
	 * Each buffer has sendpio_credits*64 bytes.
	 */
	__u64 pio_bufbase_sop;
	/*
	 * Base address of writeonly pio buffers for this process.
	 * Each buffer has sendpio_credits*64 bytes.
	 */
	__u64 pio_bufbase;
	/* address where receive buffer queue is mapped into */
	__u64 rcvhdr_bufbase;
	/* base address of Eager receive buffers. */
	__u64 rcvegr_bufbase;
	/* base address of SDMA completion ring */
	__u64 sdma_comp_bufbase;
	/*
	 * User register base for init code, not to be used directly by
	 * protocol or applications.  Always maps real chip register space.
	 * the register addresses are:
	 * ur_rcvhdrhead, ur_rcvhdrtail, ur_rcvegrhead, ur_rcvegrtail,
	 * ur_rcvtidflow
	 */
	__u64 user_regbase;
	/* notification events */
	__u64 events_bufbase;
	/* status page */
	__u64 status_bufbase;
	/* rcvhdrtail updata */
	__u64 rcvhdrtail_base;
	/*
	 * shared memory pages for subctxts if ctxt is shared; these cover
	 * all the processes in the group sharing a single context.
	 * all have enough space for the num_subcontexts value on this job.
	 */
	__u64 subctxt_uregbase;
	__u64 subctxt_rcvegrbuf;
	__u64 subctxt_rcvhdrbuf;
};

enum sdma_req_opcode {
	EXPECTED = 0,
	EAGER
};

#define HFI_SDMA_REQ_VERSION_MASK 0xF
#define HFI_SDMA_REQ_VERSION_SHIFT 0x0
#define HFI_SDMA_REQ_OPCODE_MASK 0xF
#define HFI_SDMA_REQ_OPCODE_SHIFT 0x4
#define HFI_SDMA_REQ_IOVCNT_MASK 0xFF
#define HFI_SDMA_REQ_IOVCNT_SHIFT 0x8

struct sdma_req_info {
	/*
	 * bits 0-3 - version (currently unused)
	 * bits 4-7 - opcode (enum sdma_req_opcode)
	 * bits 8-15 - io vector count
	 */
	__u16 ctrl;
	/*
	 * Number of fragments contained in this request.
	 * User-space has already computed how many
	 * fragment-sized packet the user buffer will be
	 * split into.
	 */
	__u16 npkts;
	/*
	 * Size of each fragment the user buffer will be
	 * split into.
	 */
	__u16 fragsize;
	/*
	 * Index of the slot in the SDMA completion ring
	 * this request should be using. User-space is
	 * in charge of managing its own ring.
	 */
	__u16 comp_idx;
} __packed;

/*
 * SW KDETH header.
 * swdata is SW defined portion.
 */
struct hfi_kdeth_header {
	__le32 ver_tid_offset;
	__le16 jkey;
	__le16 hcrc;
	__le32 swdata[7];
} __packed;

/*
 * Structure describing the headers that User space uses. The
 * structure above is a subset of this one.
 */
struct hfi_pkt_header {
	__le16 pbc[4];
	__be16 lrh[4];
	__be32 bth[3];
	struct hfi_kdeth_header kdeth;
} __packed;


/*
 * The list of usermode accessible registers.
 */
enum hfi_ureg {
	/* (RO)  DMA RcvHdr to be used next. */
	ur_rcvhdrtail = 0,
	/* (RW)  RcvHdr entry to be processed next by host. */
	ur_rcvhdrhead = 1,
	/* (RO)  Index of next Eager index to use. */
	ur_rcvegrindextail = 2,
	/* (RW)  Eager TID to be processed next */
	ur_rcvegrindexhead = 3,
	/* (RO)  Receive Eager Offset Tail */
	ur_rcvegroffsettail = 4,
	/* For internal use only; max register number. */
	ur_maxreg,
	/* (RW)  Receive TID flow table */
	ur_rcvtidflowtable = 256
};

#endif /* _LINIUX__HFI_USER_H */
