/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * KeemBay IPC common definitions.
 *
 * Copyright (C) 2018-2019 Intel Corporation
 */

#ifndef __KEEMBAY_IPC_COMMON_H
#define __KEEMBAY_IPC_COMMON_H

/* The alignment to be used for IPC Buffers and IPC Data. */
#define KMB_IPC_ALIGNMENT		64

/* Maximum number of channels per link. */
#define KMB_IPC_MAX_CHANNELS		1024

/* The number of high-speed channels per link. */
#define KMB_IPC_NUM_HIGH_SPEED_CHANNELS	10

/* The possible states of an IPC buffer. */
enum {
	/*
	 * KMB_IPC_BUF_FREE must be set to 0 to ensure that buffers can be
	 * initialized with memset(&buf, 0, sizeof(buf)).
	 */
	KMB_IPC_BUF_FREE = 0,
	KMB_IPC_BUF_ALLOCATED,
};

/**
 * struct kmb_ipc_buf - The IPC buffer structure.
 * @data_addr:	The address where the IPC payload is located; NOTE: this is a
 *		VPU address (not a CPU one).
 * @data_size:	The size of the payload.
 * @channel:	The channel used.
 * @src_node:	The Node ID of the sender.
 * @dst_node:	The Node ID of the intended receiver.
 * @status:	Either free or allocated.
 */
struct kmb_ipc_buf {
	uint32_t data_addr;
	uint32_t data_size;
	uint16_t channel;
	uint8_t src_node;
	uint8_t dst_node;
	uint8_t status;
} __packed __aligned(KMB_IPC_ALIGNMENT);

#endif /* __KEEMBAY_IPC_COMMON_H */
