// SPDX-License-Identifier: GPL-2.0-only
/*
 * Xlink Secure Linux Kernel API
 *
 * Copyright (C) 2020 Intel Corporation
 *
 */
#ifndef __XLINK_SECURE_H
#define __XLINK_SECURE_H

#ifdef __cplusplus
extern "C"{
#endif

#include <uapi/misc/xlink_secure_uapi.h>

/*XLINK SECURE API'S*/
enum xlink_error xlink_secure_initialize(void);

enum xlink_error xlink_secure_connect(struct xlink_handle *handle);

enum xlink_error xlink_secure_open_channel(struct xlink_handle *handle,
        uint16_t chan, enum xlink_opmode mode, uint32_t data_size,
        uint32_t timeout);

enum xlink_error xlink_secure_close_channel(struct xlink_handle *handle,
        uint16_t chan);

enum xlink_error xlink_secure_write_data(struct xlink_handle *handle,
        uint16_t chan, uint8_t const *message, uint32_t size);

enum xlink_error xlink_secure_read_data(struct xlink_handle *handle,
        uint16_t chan, uint8_t **message, uint32_t *size);

enum xlink_error xlink_secure_write_volatile(struct xlink_handle *handle,
        uint16_t chan, uint8_t const *message, uint32_t size);

enum xlink_error xlink_secure_read_data_to_buffer(struct xlink_handle *handle,
        uint16_t chan, uint8_t * const message, uint32_t *size);

enum xlink_error xlink_secure_release_data(struct xlink_handle *handle,
        uint16_t chan, uint8_t * const data_addr);

enum xlink_error xlink_secure_disconnect(struct xlink_handle *handle);

#ifdef __cplusplus
}
#endif

#endif /* __XLINK_SECURE_H */
