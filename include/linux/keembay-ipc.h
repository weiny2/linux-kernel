/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * keembay-ipc.h - KeemBay IPC Linux Kernel API
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

#ifndef __KEEMBAY_IPC_H
#define __KEEMBAY_IPC_H

#include <linux/types.h>

/* The possible node IDs. */
enum {
	KMB_IPC_NODE_ARM_CSS = 0,
	KMB_IPC_NODE_LEON_MSS,
};

int intel_keembay_ipc_open_channel(u8 node_id, u16 chan_id);
int intel_keembay_ipc_close_channel(u8 node_id, u16 chan_id);
int intel_keembay_ipc_send(u8 node_id, u16 chan_id, uint32_t paddr,
			   size_t size);
int intel_keembay_ipc_recv(u8 node_id, u16 chan_id, uint32_t *paddr,
			   size_t *size, u32 timeout);

#endif /* __KEEMBAY_IPC_H */
