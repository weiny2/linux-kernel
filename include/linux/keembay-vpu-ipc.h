/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * keembay-vpu-ipc.h - KeemBay VPU IPC Linux Kernel API
 *
 * Copyright (c) 2018 Intel Corporation.
 */

#ifndef __KEEMBAY_VPU_IPC_H
#define __KEEMBAY_VPU_IPC_H

/* The possible node IDs. */
enum {
	KMB_VPU_IPC_NODE_ARM_CSS = 0,
	KMB_VPU_IPC_NODE_LEON_MSS,
};

/* Possible states of VPU. */
enum intel_keembay_vpu_state {
	KEEMBAY_VPU_OFF = 0,
	KEEMBAY_VPU_BUSY,
	KEEMBAY_VPU_READY,
	KEEMBAY_VPU_ERROR,
	KEEMBAY_VPU_STOPPING
};

/* Possible CPU IDs for which we receive WDT timeout events. */
enum intel_keembay_wdt_cpu_id {
	KEEMBAY_VPU_MSS = 0,
	KEEMBAY_VPU_NCE
};

int intel_keembay_vpu_ipc_open_channel(u8 node_id, u16 chan_id);
int intel_keembay_vpu_ipc_close_channel(u8 node_id, u16 chan_id);
int intel_keembay_vpu_ipc_send(u8 node_id, u16 chan_id, uint32_t paddr,
			       size_t size);
int intel_keembay_vpu_ipc_recv(u8 node_id, u16 chan_id, uint32_t *paddr,
			       size_t *size, u32 timeout);
int intel_keembay_vpu_startup(const char *firmware_name);
int intel_keembay_vpu_reset(void);
int intel_keembay_vpu_stop(void);
enum intel_keembay_vpu_state intel_keembay_vpu_status(void);
int intel_keembay_vpu_get_wdt_count(enum intel_keembay_wdt_cpu_id id);
int intel_keembay_vpu_wait_for_ready(u32 timeout);

#endif /* __KEEMBAY_VPU_IPC_H */
