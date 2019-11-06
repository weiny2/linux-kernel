/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_COMMON_HEADER_
#define MXLK_COMMON_HEADER_

#include <linux/types.h>

/*
 * Number of interfaces to statically allocate resources for
 */
#define MXLK_NUM_INTERFACES (1)

/*
 * Alignment restriction on buffers passed between device and host
 */
#define MXLK_DMA_ALIGNMENT (16)

#define MXLK_FRAGMENT_SIZE (32 * 1024)
#define MXLK_NUM_TX_DESCS (64)
#define MXLK_NUM_RX_DESCS (64)

////////////////////////////////////////////////////////////////////////////////

/*
 * Status encoding of the transfer descriptors
 */
#define MXLK_DESC_STATUS_SUCCESS (0)
#define MXLK_DESC_STATUS_ERROR (-1)

/*
 * Layout transfer descriptors used by device and host
 */
struct mxlk_transfer_desc {
	uint64_t address;
	uint32_t length;
	uint16_t status;
	uint16_t interface;
} __packed;

////////////////////////////////////////////////////////////////////////////////

/*
 * Version to be exposed by both device and host
 */
#define MXLK_VERSION_MAJOR (0)
#define MXLK_VERSION_MINOR (2)
#define MXLK_VERSION_BUILD (0)
#define MXLK_IO_COMM_SIZE (16 * 1024)
#define MXLK_MMIO_OFFSET (4 * 1024)


struct mxlk_version {
	uint8_t major;
	uint8_t minor;
	uint16_t build;
} __packed;

/*
 * Status encoding of both device and host
 */
#define MXLK_STATUS_ERROR (-1)
#define MXLK_STATUS_UNINIT (0)
#define MXLK_STATUS_BOOT (1)
#define MXLK_STATUS_RUN (2)

#define MXLK_DOORBELL_NONE (0)
#define MXLK_DOORBELL_SEND (1)

/*
 * MMIO layout and offsets shared between device and host
 */
struct mxlk_mmio {
	struct mxlk_version version;
	uint32_t device_status;
	uint32_t host_status;
	uint32_t doorbell_status;
	uint32_t cap_offset;
} __packed;

#define MXLK_MMIO_VERSION (offsetof(struct mxlk_mmio, version))
#define MXLK_MMIO_DEV_STATUS (offsetof(struct mxlk_mmio, device_status))
#define MXLK_MMIO_HOST_STATUS (offsetof(struct mxlk_mmio, host_status))
#define MXLK_MMIO_DOORBELL_STATUS (offsetof(struct mxlk_mmio, doorbell_status))
#define MXLK_MMIO_CAPABILITIES (offsetof(struct mxlk_mmio, cap_offset))

////////////////////////////////////////////////////////////////////////////////

/*
 * Defined capabilities located in mmio space
 */
#define MXLK_CAP_NULL (0)
#define MXLK_CAP_BOOT (1)
#define MXLK_CAP_STATS (2)
#define MXLK_CAP_TXRX (3)

/*
 * Header at the beginning of each capability to define and link to next
 */
struct mxlk_cap_hdr {
	uint16_t id;
	uint16_t next;
} __packed;

#define MXLK_CAP_HDR_ID (offsetof(struct mxlk_cap_hdr, id))
#define MXLK_CAP_HDR_NEXT (offsetof(struct mxlk_cap_hdr, next))

/*
 * Bootloader capability - to be defined later
 */
struct mxlk_cap_boot {
	struct mxlk_cap_hdr hdr;
} __packed;

/*
 * Stat collection - to be defined later
 */
struct mxlk_cap_stats {
	struct mxlk_cap_hdr hdr;
} __packed;

/*
 * Simplex used by txrx cap
 */
struct mxlk_cap_pipe {
	uint32_t ring;
	uint32_t ndesc;
	uint32_t head;
	uint32_t tail;
} __packed;

/*
 * Transmit and Receive capability
 */
struct mxlk_cap_txrx {
	struct mxlk_cap_hdr hdr;
	u32 fragment_size;
	struct mxlk_cap_pipe tx;
	struct mxlk_cap_pipe rx;
} __packed;

#endif
