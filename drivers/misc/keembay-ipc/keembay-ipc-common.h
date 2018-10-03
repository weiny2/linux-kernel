/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * keembay-ipc-common.h - KeemBay IPC common definitions.
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
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

/* IPC buffer. */
struct kmb_ipc_buf {
	uint32_t data_paddr; /* Physical address where payload is located. */
	uint32_t data_size;  /* Size of payload. */
	uint16_t channel;    /* The channel used. */
	uint8_t src_node;    /* The Node ID of the sender. */
	uint8_t dst_node;    /* The Node ID of the intended receiver. */
	uint8_t status;	     /* Either free or allocated. */
} __packed __aligned(KMB_IPC_ALIGNMENT);

#endif /* __KEEMBAY_IPC_COMMON_H */
