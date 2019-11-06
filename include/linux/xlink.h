// SPDX-License-Identifier: GPL-2.0-only
/*
 * xlink Linux Kernel API
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 */
#ifndef __XLINK_H
#define __XLINK_H

typedef uint32_t xlink_channel_id_t;

enum xlink_prof_cfg {
	PROFILE_DISABLE = 0,
	PROFILE_ENABLE
};

struct xlink_global_handle {
	int log_level;
	enum xlink_prof_cfg prof_cfg;
};

enum xlink_dev_type {
	VPU_DEVICE = 0,
	PCIE_DEVICE,
	USB_DEVICE,
	ETH_DEVICE,
	IPC_DEVICE,
	NMB_OF_DEVICE_TYPES
};

struct xlink_handle {
	char *dev_path;
	char *dev_path2;
	int link_id;
	uint8_t node;
	enum xlink_dev_type dev_type;
	void *fd;
};

enum xlink_opmode {
	RXB_TXB = 0,
	RXN_TXN,
	RXB_TXN,
	RXN_TXB
};

enum xlink_sys_freq {
	DEFAULT_NOMINAL_MAX = 0,
	POWER_SAVING_MEDIUM,
	POWER_SAVING_HIGH,
};

enum xlink_error {
	X_LINK_SUCCESS = 0,
	X_LINK_ALREADY_INIT,
	X_LINK_ALREADY_OPEN,
	X_LINK_COMMUNICATION_NOT_OPEN,
	X_LINK_COMMUNICATION_FAIL,
	X_LINK_COMMUNICATION_UNKNOWN_ERROR,
	X_LINK_DEVICE_NOT_FOUND,
	X_LINK_TIMEOUT,
	X_LINK_ERROR
};

enum xlink_channel_status {
	CHAN_CLOSED			= 0x0000,
	CHAN_OPEN			= 0x0001,
	CHAN_BLOCKED_READ	= 0x0010,
	CHAN_BLOCKED_WRITE	= 0x0100,
	CHAN_OPEN_PEER		= 0x1000,
};

enum xlink_error xlink_initialize(struct xlink_global_handle *gl_handle);

enum xlink_error xlink_connect(struct xlink_handle *handle);

enum xlink_error xlink_open_channel(struct xlink_handle *handle,
		uint16_t chan, enum xlink_opmode mode, uint32_t data_size,
		uint32_t timeout);

enum xlink_error xlink_close_channel(struct xlink_handle *handle,
		uint16_t chan);

enum xlink_error xlink_write_data(struct xlink_handle *handle,
		uint16_t chan, uint8_t const *message, uint32_t size);

enum xlink_error xlink_read_data(struct xlink_handle *handle,
		uint16_t chan, uint8_t **message, uint32_t *size);

enum xlink_error xlink_write_control_data(struct xlink_handle *handle,
		uint16_t chan, uint8_t const *message, uint32_t size);

enum xlink_error xlink_write_volatile(struct xlink_handle *handle,
		uint16_t chan, uint8_t const *message, uint32_t size);

enum xlink_error xlink_read_data_to_buffer(struct xlink_handle *handle,
		uint16_t chan, uint8_t * const message, uint32_t *size);

enum xlink_error xlink_release_data(struct xlink_handle *handle,
		uint16_t chan, uint8_t * const data_addr);

enum xlink_error xlink_disconnect(struct xlink_handle *handle);

enum xlink_error xlink_start_vpu(char *filename);

enum xlink_error xlink_stop_vpu(void);

/* API functions to be implemented

enum xlink_error xlink_write_data_crc(struct xlink_handle *handle,
		uint16_t chan, uint8_t const *message, uint32_t size);

enum xlink_error xlink_read_data_crc(struct xlink_handle *handle,
		uint16_t channelId, uint8_t * const message, uint32_t size);

enum xlink_error xlink_read_data_to_buffer_crc(struct xlink_handle *handle,
		uint16_t chan, uint8_t * const message, uint32_t *size);

enum xlink_error xlink_boot_remote(struct xlink_handle *handle,
		enum xlink_sys_freq sys_freq);

enum xlink_error xlink_reset_remote(xlink_handle *handle,
		enum xlink_sys_freq sys_freq);

enum xlink_error xlink_reset_all(enum xlink_sys_freq sys_freq);

enum xlink_error xlink_prof_start(enum xlink_prof_cfg prof_cfg);

enum xlink_error xlink_prof_stop(enum xlink_prof_cfg prof_cfg);

uint32_t xlink_get_number_of_devices(void);

 */

#endif /* __XLINK_H */
