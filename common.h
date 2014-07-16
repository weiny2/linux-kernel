/*
 * Copyright (c) 2006, 2007, 2008, 2009, 2010 QLogic Corporation.
 * All rights reserved.
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

#ifndef _COMMON_H
#define _COMMON_H

/*
 * This file contains defines, structures, etc. that are used
 * to communicate between kernel and user code.
 */

/* This is the IEEE-assigned OUI for QLogic Inc. QLogic_IB */
#define QIB_SRC_OUI_1 0x00
#define QIB_SRC_OUI_2 0x11
#define QIB_SRC_OUI_3 0x75

/* This is the IEEE-assigned OUI for Intel...  At least one of them. */
#define WFR_SRC_OUI_1 0x00
#define WFR_SRC_OUI_2 0x02
#define WFR_SRC_OUI_3 0xB3

/* version of protocol header (known to chip also). In the long run,
 * we should be able to generate and accept a range of version numbers;
 * for now we only accept one, and it's compiled in.
 */
#define IPS_PROTO_VERSION 2

/*
 * These are compile time constants that you may want to enable or disable
 * if you are trying to debug problems with code or performance.
 * QIB_VERBOSE_TRACING define as 1 if you want additional tracing in
 * fastpath code
 * QIB_TRACE_REGWRITES define as 1 if you want register writes to be
 * traced in faspath code
 * _QIB_TRACING define as 0 if you want to remove all tracing in a
 * compilation unit
 */

/*
 * If a packet's QP[23:16] bits match this value, then it is
 * a PSM packet and the hardware will expect a KDETH header
 * following the BTH.
 */
#define DEFAULT_KDETH_QP 0x80

/*
 * These are the status bits readable (in ascii form, 64bit value)
 * from the "status" sysfs file.  For binary compatibility, values
 * must remain as is; removed states can be reused for different
 * purposes.
 */
#define QIB_STATUS_INITTED       0x1    /* basic initialization done */
/* Chip has been found and initted */
#define QIB_STATUS_CHIP_PRESENT 0x20
/* IB link is at ACTIVE, usable for data traffic */
#define QIB_STATUS_IB_READY     0x40
/* link is configured, LID, MTU, etc. have been set */
#define QIB_STATUS_IB_CONF      0x80
/* A Fatal hardware error has occurred. */
#define QIB_STATUS_HWERROR     0x200

/*
 * The list of usermode accessible registers.
 */
enum hfi_ureg {
	/* (RO)  DMA RcvHdr to be used next. */
	ur_rcvhdrtail,
	/* (RW)  RcvHdr entry to be processed next by host. */
	ur_rcvhdrhead,
	/* (RO)  Index of next Eager index to use. */
	ur_rcvegrindextail,
	/* (RW)  Eager TID to be processed next */
	ur_rcvegrindexhead,
	/* (RW)  Receive TID flow table */
	ur_rcvtidflowtable,
	/* For internal use only; max register number. */
	ur_maxreg
};

/* bit values for spi_runtime_flags */
#define HFI_RUNTIME_NODMA_RTAIL         0x01
#define HFI_RUNTIME_SDMA                0x02
#define HFI_RUNTIME_FORCE_PIOAVAIL      0x04
#define HFI_RUNTIME_HDRSUPP             0x08
#define HFI_RUNTIME_EXTENDED_PSN        0x10
#define HFI_RUNTIME_TID_UNMAP           0x20

/*
 * This structure is returned by qib_userinit() immediately after
 * open to get implementation-specific info, and info specific to this
 * instance.
 *
 * This struct must have explict pad fields where type sizes
 * may result in different alignments between 32 and 64 bit
 * programs, since the 64 bit * bit kernel requires the user code
 * to have matching offsets
 */
struct hfi_base_info {
	/* per-chip and other runtime features bitmap (HFI_RUNTIME_*) */
	__u32 runtime_flags;
	/* version of hardware, for feature checking. */
	__u32 hw_version;
	/* version of software, for feature checking. */
	__u32 sw_version;
	/*
	 * IB MTU, packets IB data must be less than this.
	 * The MTU is in bytes, and will be a multiple of 4 bytes.
	 */
	__u16 mtu;
	/*
	 * Job key
	 */
	__u16 jkey;
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

/*
 * This version number is given to the driver by the user code during
 * initialization in the spu_userversion field of hfi_user_info, so
 * the driver can check for compatibility with user code.
 *
 * The major version changes when data structures
 * change in an incompatible way.  The driver must be the same or higher
 * for initialization to succeed.  In some cases, a higher version
 * driver will not interoperate with older software, and initialization
 * will return an error.
 */
#define QIB_USER_SWMAJOR 2

/*
 * Minor version differences are always compatible
 * a within a major version, however if user software is larger
 * than driver software, some new features and/or structure fields
 * may not be implemented; the user code must deal with this if it
 * cares, or it must abort after initialization reports the difference.
 */
#define QIB_USER_SWMINOR 11

#define QIB_USER_SWVERSION ((QIB_USER_SWMAJOR << 16) | QIB_USER_SWMINOR)

#ifndef QIB_KERN_TYPE
#define QIB_KERN_TYPE 0
#endif

/*
 * Similarly, this is the kernel version going back to the user.  It's
 * slightly different, in that we want to tell if the driver was built as
 * part of a QLogic release, or from the driver from openfabrics.org,
 * kernel.org, or a standard distribution, for support reasons.
 * The high bit is 0 for non-QLogic and 1 for QLogic-built/supplied.
 *
 * It's returned by the driver to the user code during initialization in the
 * spi_sw_version field of hfi_base_info, so the user code can in turn
 * check for compatibility with the kernel.
*/
#define QIB_KERN_SWVERSION ((QIB_KERN_TYPE << 31) | QIB_USER_SWVERSION)

/*
 * Define the driver version number.  This is something that refers only
 * to the driver itself, not the software interfaces it supports.
 */
#ifndef HFI_DRIVER_VERSION_BASE
#define HFI_DRIVER_VERSION_BASE "1.11"
#endif

/* create the final driver version string */
#ifdef HFI_IDSTR
#define HFI_DRIVER_VERSION HFI_DRIVER_VERSION_BASE " " HFI_IDSTR
#else
#define HFI_DRIVER_VERSION HFI_DRIVER_VERSION_BASE
#endif

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

/*
 * PSM can set any of these bits in the flags field of the
 * hfi_user_info structure to alter the benavior of the HW on
 * a per-context level.
 */
#define HFI_CTXTFLAG_ONEPKTPEREGRBUF  (1<<0)
#define HFI_CTXTFLAG_DONTDROPEGRFULL  (1<<1)
#define HFI_CTXTFLAG_DONTDROPHDRQFULL (1<<2)
#define HFI_CTXTFLAG_TIDFLOWENABLE    (1<<3)
#define HFI_CTXTFLAG_ALLOWPERMJKEY    (1<<4)

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
	/* various context setup flags. See HFI_CTXTFLAG_* */
	__u16 flags;
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

/* User commands. */
#define HFI_CMD_ASSIGN_CTXT     1	/* allocate HCA and context */
#define HFI_CMD_CTXT_INFO	2	/* find out what resources we got */
#define HFI_CMD_CTXT_SETUP      3
#define HFI_CMD_USER_INFO 	4	/* set up userspace */
#define HFI_CMD_TID_UPDATE	5	/* update expected TID entries */
#define HFI_CMD_TID_FREE	6	/* free expected TID entries */
#define HFI_CMD_CREDIT_UPD	7	/* force an update of PIO credit */
#define HFI_CMD_SDMA_STATUS_UPD 8       /* force update of SDMA status ring */

#define HFI_CMD_RECV_CTRL	9	/* control receipt of packets */
#define HFI_CMD_POLL_TYPE	10	/* set the kind of polling we want */
#define HFI_CMD_ACK_EVENT       11	/* ack & clear bits *spi_sendbuf_status */
#define HFI_CMD_SET_PKEY        12      /* set context's pkey */
#define HFI_CMD_CTXT_RESET      13      /* reset context's HW send context */

/*
 * HFI_CMD_ACK_EVENT obsoletes HFI_CMD_DISARM_BUFS, but we keep it for
 * compatibility with libraries from previous release.   The ACK_EVENT
 * will take appropriate driver action (if any, just DISARM for now),
 * then clear the bits passed in as part of the mask.  These bits are
 * in the first 64bit word at spi_sendbuf_status, and are passed to
 * the driver in the event_mask union as well.
 */
#define _QIB_EVENT_DISARM_BUFS_BIT	0
#define _QIB_EVENT_LINKDOWN_BIT		1
#define _QIB_EVENT_LID_CHANGE_BIT	2
#define _QIB_EVENT_LMC_CHANGE_BIT	3
#define _QIB_EVENT_SL2VL_CHANGE_BIT	4
#define _QIB_MAX_EVENT_BIT _QIB_EVENT_SL2VL_CHANGE_BIT

#define QIB_EVENT_DISARM_BUFS_BIT	(1UL << _QIB_EVENT_DISARM_BUFS_BIT)
#define QIB_EVENT_LINKDOWN_BIT		(1UL << _QIB_EVENT_LINKDOWN_BIT)
#define QIB_EVENT_LID_CHANGE_BIT	(1UL << _QIB_EVENT_LID_CHANGE_BIT)
#define QIB_EVENT_LMC_CHANGE_BIT	(1UL << _QIB_EVENT_LMC_CHANGE_BIT)
#define QIB_EVENT_SL2VL_CHANGE_BIT	(1UL << _QIB_EVENT_SL2VL_CHANGE_BIT)


/*
 * Poll types
 */
#define QIB_POLL_TYPE_ANYRCV     0x0
#define QIB_POLL_TYPE_URGENT     0x1

struct hfi_ctxt_info {
	__u16 num_active;       /* number of active units */
	__u16 unit;             /* unit (chip) assigned to caller */
	__u16 ctxt;             /* ctxt on unit assigned to caller */
	__u16 subctxt;          /* subctxt on unit assigned to caller */
	__u16 rcvtids;          /* number of Rcv TIDs for this context */
	__u16 credits;          /* number of PIO credits for this context */
	__u16 numa_node;        /* NUMA node of the assigned device */
	__u16 rec_cpu;          /* cpu # for affinity (ffff if none) */
};

struct hfi_ctxt_setup {
	__u16 egrtids;          /* number of RcvArray entries for Eager Rcvs */
	__u16 rcvhdrq_cnt;      /* number of RcvHdrQ entries */
	__u16 rcvhdrq_entsize;  /* size (in bytes) for each RcvHdrQ entry */
	__u16 sdma_ring_size;   /* number of entries in SDMA request ring */
	__u32 rcvegr_size;      /* size of Eager buffers */
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
	enum hfi_sdma_comp_state status;
	int errno;
};

/*
 * Device status and notifications from driver to user-space.
 */
struct hfi_status {
	__u64 dev;      /* device/hw status bits */
	__u64 port;     /* port state and status bits */
	char freezemsg[0];
};

struct qib_iovec {
	/* Pointer to data, but same size 32 and 64 bit */
	__u64 iov_base;
	/*
	 * Length of data; don't need 64 bits, but want
	 * qib_sendpkt to remain same size as before 32 bit changes, so...
	 */
	__u64 iov_len;
};

/*
 * Describes a single packet for send.  Each packet can have one or more
 * buffers, but the total length (exclusive of IB headers) must be less
 * than the MTU, and if using the PIO method, entire packet length,
 * including IB headers, must be less than the qib_piosize value (words).
 * Use of this necessitates including sys/uio.h
 */
struct __qib_sendpkt {
	__u32 sps_flags;        /* flags for packet (TBD) */
	__u32 sps_cnt;          /* number of entries to use in sps_iov */
	/* array of iov's describing packet. TEMPORARY */
	struct qib_iovec sps_iov[4];
};

/*
 * Diagnostics can send a packet by writing the following
 * struct to the diag packet special file.
 *
 * This allows a custom PBC qword, so that special modes and deliberate
 * changes to CRCs can be used.
 */
#define _DIAG_PKT_VERS 0
struct diag_pkt {
	__u16 version;		/* structure version */
	__u16 unit;		/* which device */
	__u16 context;		/* send context to use */
	__u16 len;		/* data length, in bytes */
	__u64 data;		/* user data pointer */
	__u64 pbc;		/* PBC for the packet */
};

/*
 * Data layout in I2C flash (for GUID, etc.)
 * All fields are little-endian binary unless otherwise stated
 */
#define QIB_FLASH_VERSION 2
struct qib_flash {
	/* flash layout version (QIB_FLASH_VERSION) */
	__u8 if_fversion;
	/* checksum protecting if_length bytes */
	__u8 if_csum;
	/*
	 * valid length (in use, protected by if_csum), including
	 * if_fversion and if_csum themselves)
	 */
	__u8 if_length;
	/* the GUID, in network order */
	__u8 if_guid[8];
	/* number of GUIDs to use, starting from if_guid */
	__u8 if_numguid;
	/* the (last 10 characters of) board serial number, in ASCII */
	char if_serial[12];
	/* board mfg date (YYYYMMDD ASCII) */
	char if_mfgdate[8];
	/* last board rework/test date (YYYYMMDD ASCII) */
	char if_testdate[8];
	/* logging of error counts, TBD */
	__u8 if_errcntp[4];
	/* powered on hours, updated at driver unload */
	__u8 if_powerhour[2];
	/* ASCII free-form comment field */
	char if_comment[32];
	/* Backwards compatible prefix for longer QLogic Serial Numbers */
	char if_sprefix[4];
	/* 82 bytes used, min flash size is 128 bytes */
	__u8 if_future[46];
};

/*
 * The next set of defines are for packet headers, and chip register
 * and memory bits that are visible to and/or used by user-mode software.
 */

/*
 * Receive Header Flags
 * These bits are split into 2 4-byte quantities.
 */
/* first DWORD flags */
#define RHF0_PKT_LEN_SHIFT 0
#define RHF0_PKT_LEN_MASK 0xfff
#define RHF0_PKT_LEN_SMASK (RHF0_PKT_LEN_MASK << RHF0_PKT_LEN_SHIFT)

#define RHF0_RCV_TYPE_SHIFT 12
#define RHF0_RCV_TYPE_MASK 0x7
#define RHF0_RCV_TYPE_SMASK (RHF0_RCV_TYPE_MASK << RHF0_RCV_TYPE_SHIFT)

#define RHF0_USE_EGR_BFR_SHIFT 15
#define RHF0_USE_EGR_BFR_MASK 0x1
#define RHF0_USE_EGR_BFR_SMASK (RHF0_USE_EGR_BFR_MASK << RHF0_USE_EGR_BFR_SHIFT)

#define RHF0_EGR_INDEX_SHIFT 16
#define RHF0_EGR_INDEX_MASK 0x7ff
#define RHF0_EGR_INDEX_SMASK (RHF0_EGR_INDEX_MASK << RHF0_EGR_INDEX_SHIFT)

#define RHF0_DC_INFO_SHIFT 27
#define RHF0_DC_INFO_MASK 0x1
#define RHF0_DC_INFO_SMASK (RHF0_DC_INFO_MASK << RHF0_DC_INFO_SHIFT)

#define RHF0_RCV_SEQ_SHIFT 28
#define RHF0_RCV_SEQ_MASK 0xf
#define RHF0_RCV_SEQ_SMASK (RHF0_RCV_SEQ_MASK << RHF0_RCV_SEQ_SHIFT)

/* second DWORD flags */
#define RHF1_EGR_OFFSET_SHIFT           (32-32)
#define RHF1_EGR_OFFSET_MASK 0xfff
#define RHF1_EGR_OFFSET_SMASK (RHF1_EGR_OFFSET_MASK << RHF1_EGR_OFFSET_SHIFT)
#define RHF1_HDRQ_OFFSET_SHIFT          (44-32)
#define RHF1_HDRQ_OFFSET_MASK 0x1ff
#define RHF1_HDRQ_OFFSET_SMASK (RHF1_HDRQ_OFFSET_MASK << RHF1_HDRQ_OFFSET_SHIFT)
#define RHF1_K_HDR_LEN_ERR	(0x1 << (53-32))
#define RHF1_DC_UNC_ERR		(0x1 << (54-32))
#define RHF1_DC_ERR	        (0x1 << (55-32))
#define RHF1_RCV_TYPE_ERR_SHIFT         (56-32)
#define RHF1_RCV_TYPE_ERR_MASK  0x7
#define RHF1_RCV_TYPE_ERR_SMASK (RHF1_RCV_TYPE_ERR_MASK << RHF1_RCV_TYPE_ERR_SHIFT)
#define RHF1_TID_ERR		(0x1 << (59-32))
#define RHF1_LEN_ERR		(0x1 << (60-32))
#define RHF1_ECC_ERR		(0x1 << (61-32))
#define RHF1_VCRC_ERR		(0x1 << (62-32))
#define RHF1_ICRC_ERR		(0x1 << (63-32))

#define RHF1_ERROR_SMASK 0xffe00000			/* bits 63:53 */

/* RHF receive types */
#define RHF_RCV_TYPE_EXPECTED 0
#define RHF_RCV_TYPE_EAGER    1
#define RHF_RCV_TYPE_IB       2 /* normal IB, IB Raw, or IPv6 */
#define RHF_RCV_TYPE_ERROR    3
#define RHF_RCV_TYPE_BYPASS   4

/* RHF receive type error - expected packet errors */
#define RHF_RTE_EXPECTED_FLOW_SEQ_ERR	0x2
#define RHF_RTE_EXPECTED_FLOW_GEN_ERR	0x4

/* RHF receive type error - eager packet errors */
#define RHF_RTE_EAGER_NO_ERR		0x0

/* RHF receive type error - IB packet errors */
#define RHF_RTE_IB_NO_ERR		0x0

/* RHF receive type error - error packet errors */
#define RHF_RTE_ERROR_NO_ERR		0x0
#define RHF_RTE_ERROR_OP_CODE_ERR	0x1
#define RHF_RTE_ERROR_KHDR_MIN_LEN_ERR	0x2
#define RHF_RTE_ERROR_KHDR_HCRC_ERR	0x3
#define RHF_RTE_ERROR_KHDR_KVER_ERR	0x4
#define RHF_RTE_ERROR_CONTEXT_ERR	0x5
#define RHF_RTE_ERROR_KHDR_TID_ERR	0x6

/* RHF receive type error - bypass packet errors */
#define RHF_RTE_BYPASS_NO_ERR		0x0


#define QLOGIC_IB_MAX_SUBCTXT   4

/* SendPIOAvail bits */
#define QLOGIC_IB_SENDPIOAVAIL_BUSY_SHIFT 1
#define QLOGIC_IB_SENDPIOAVAIL_CHECK_SHIFT 0

/*
 * This structure contains the first field common to all protocols
 * that employ this chip.
 */
struct qib_message_header {
	__be16 lrh[4];
};

/* IB - LRH header consts */
#define QIB_LRH_GRH 0x0003      /* 1. word of IB LRH - next header: GRH */
#define QIB_LRH_BTH 0x0002      /* 1. word of IB LRH - next header: BTH */

/* misc. */
#define SIZE_OF_CRC 1

#define WFR_LIM_MGMT_P_KEY       0x7FFF
#define WFR_FULL_MGMT_P_KEY      0xFFFF

#ifdef CONFIG_STL_MGMT
#define WFR_DEFAULT_P_KEY WFR_LIM_MGMT_P_KEY
#else
#define WFR_DEFAULT_P_KEY WFR_FULL_MGMT_P_KEY
#endif
#define QIB_PERMISSIVE_LID 0xFFFF
#define QIB_AETH_CREDIT_SHIFT 24
#define QIB_AETH_CREDIT_MASK 0x1F
#define QIB_AETH_CREDIT_INVAL 0x1F
#define QIB_PSN_MASK 0xFFFFFF
#define QIB_MSN_MASK 0xFFFFFF
#define QIB_QPN_MASK 0xFFFFFF
#define QIB_FECN_SHIFT 31
#define QIB_FECN_MASK 0x1
#define QIB_BECN_SHIFT 30
#define QIB_BECN_MASK 0x1
#define QIB_MULTICAST_LID_BASE 0xC000
#define QIB_MULTICAST_QPN 0xFFFFFF

static inline __u64 rhf_to_cpu(const __le32 *rbuf)
{
	return __le64_to_cpu(*((__le64 *)rbuf));
}

static inline __u32 rhf_err_flags(const __le32 *rbuf)
{
	return __le32_to_cpu(rbuf[1]) & RHF1_ERROR_SMASK;
}

static inline __u32 rhf_rcv_type(const __le32 *rbuf)
{
	return (__le32_to_cpu(rbuf[0]) >> RHF0_RCV_TYPE_SHIFT) &
		RHF0_RCV_TYPE_MASK;
}

static inline __u32 rhf_rcv_type_err(const __le32 *rbuf)
{
	return (__le32_to_cpu(rbuf[1]) >> RHF1_RCV_TYPE_ERR_SHIFT) &
		RHF1_RCV_TYPE_ERR_MASK;
}

/* return size is in bytes, not DWORDs */
static inline __u32 rhf_pkt_len(const __le32 *rbuf)
{
	/* ordered so the shiftc can combine */
	return ((__le32_to_cpu(rbuf[0]) & RHF0_PKT_LEN_SMASK) >>
		RHF0_PKT_LEN_SHIFT) << 2;
}

static inline __u32 rhf_egr_index(const __le32 *rbuf)
{
	return (__le32_to_cpu(rbuf[0]) >> RHF0_EGR_INDEX_SHIFT) &
		RHF0_EGR_INDEX_MASK;
}

static inline __u32 rhf_rcv_seq(const __le32 *rbuf)
{
	return (__le32_to_cpu(rbuf[0]) >> RHF0_RCV_SEQ_SHIFT) &
		RHF0_RCV_SEQ_MASK;
}

/* returned offset is in DWORDS */
static inline __u32 rhf_hdrq_offset(const __le32 *rbuf)
{
	return (__le32_to_cpu(rbuf[1]) >> RHF1_HDRQ_OFFSET_SHIFT) &
		RHF1_HDRQ_OFFSET_MASK;
}

static inline __u32 rhf_use_egr_bfr(const __le32 *rbuf)
{
	return __le32_to_cpu(rbuf[0]) & RHF0_USE_EGR_BFR_SMASK;
}

#endif /* _COMMON_H */
