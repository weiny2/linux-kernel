// SPDX-License-Identifier: GPL-2.0-only
/*
 * xlink Defines.
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 */
#ifndef __XLINK_DEFS_H
#define __XLINK_DEFS_H

#include <linux/xlink.h>

#define XLINK_MAX_BUF_SIZE 128U
#define XLINK_MAX_DATA_SIZE (1024U * 1024U * 1024U)
#define XLINK_MAX_CONTROL_DATA_SIZE 100U
#define XLINK_MAX_CONNECTIONS 8
#define XLINK_PACKET_ALIGNMENT 64
#define XLINK_INVALID_EVENT_ID 0xDEADBEEF
#define XLINK_INVALID_CHANNEL_ID 0xDEAD
#define XLINK_PACKET_QUEUE_CAPACITY 10000
#define XLINK_EVENT_QUEUE_CAPACITY 128
#define XLINK_EVENT_HEADER_MAGIC 0x786C6E6B
#define XLINK_PING_TIMEOUT_MS 5000U
#define XLINK_MAX_DEVICE_NAME_SIZE 128
#define XLINK_MAX_DEVICE_LIST_SIZE 8
#define XLINK_INVALID_LINK_ID 0xDEADBEEF

#define NMB_CHANNELS 4096
#define IP_CONTROL_CHANNEL 0x0A
#define VPU_CONTROL_CHANNEL 0x400
#define CONTROL_CHANNEL_OPMODE RXB_TXB	// blocking
#define CONTROL_CHANNEL_DATASIZE 128U	// size of internal rx/tx buffers
#define CONTROL_CHANNEL_TIMEOUT_MS 0U	// wait indefinitely
#define NULL_DEVICE -1 // used for channel table entries with no mapping
#define SIGXLNK 44	// signal XLink uses for callback signalling

// the end of the IPC channel range (starting at zero)
#define XLINK_IPC_MAX_CHANNELS	1024

enum xlink_event_origin {
	EVENT_TX = 0, // outgoing events
	EVENT_RX, // incoming events
};

enum xlink_event_type {
	// request events
	XLINK_WRITE_REQ = 0x00,
	XLINK_WRITE_VOLATILE_REQ,
	XLINK_READ_REQ,
	XLINK_READ_TO_BUFFER_REQ,
	XLINK_RELEASE_REQ,
	XLINK_OPEN_CHANNEL_REQ,
	XLINK_CLOSE_CHANNEL_REQ,
	XLINK_PING_REQ,
	XLINK_WRITE_CONTROL_REQ,
	XLINK_DATA_READY_CALLBACK_REQ,
	XLINK_DATA_CONSUMED_CALLBACK_REQ,
	XLINK_REQ_LAST,
	// response events
	XLINK_WRITE_RESP = 0x10,
	XLINK_WRITE_VOLATILE_RESP,
	XLINK_READ_RESP,
	XLINK_READ_TO_BUFFER_RESP,
	XLINK_RELEASE_RESP,
	XLINK_OPEN_CHANNEL_RESP,
	XLINK_CLOSE_CHANNEL_RESP,
	XLINK_PING_RESP,
	XLINK_WRITE_CONTROL_RESP,
	XLINK_DATA_READY_CALLBACK_RESP,
	XLINK_DATA_CONSUMED_CALLBACK_RESP,
	XLINK_RESP_LAST,
};

struct xlink_event_header {
	uint32_t magic;
	uint32_t id;
	enum xlink_event_type type;
	uint16_t chan;
	size_t size;
	uint32_t timeout;
	uint8_t  control_data[XLINK_MAX_CONTROL_DATA_SIZE];
};

struct xlink_event {
	struct xlink_event_header header;
	enum xlink_event_origin origin;
	struct xlink_handle *handle;
	void *data;
	struct task_struct *calling_pid;
	char callback_origin;
	void **pdata;
	dma_addr_t paddr;
	uint32_t *length;
	struct list_head list;
};

struct xlink_ipc_fd {
	u8 node;
	u16 chan;
};

struct xlink_event *xlink_create_event(enum xlink_event_type type,
		struct xlink_handle *handle, uint16_t chan,
		uint32_t size, uint32_t timeout);

void xlink_destroy_event(struct xlink_event *event);

#endif /* __XLINK_DEFS_H */
