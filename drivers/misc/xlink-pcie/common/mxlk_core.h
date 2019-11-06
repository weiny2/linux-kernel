/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_CORE_HEADER_
#define MXLK_CORE_HEADER_

#include "mxlk.h"

/*
 * @brief Initializes mxlk core component
 * NOTES:
 *  1) To be called at PCI probe event
 *
 * @param[in] mxlk - pointer to mxlk instance
 *
 * @return:
 *       0 - success
 *      <0 - linux error code
 */
int mxlk_core_init(struct mxlk *mxlk);

/*
 * @brief cleans up mxlk core component
 * NOTES:
 *  1) To be called at PCI remove event
 *
 * @param[in] mxlk - pointer to mxlk instance
 *
 */
void mxlk_core_cleanup(struct mxlk *mxlk);

/*
 * @brief opens mxlk interface
 *
 * @param[in] inf - pointer to interface instance
 *
 * @return:
 *       0 - success
 *      <0 - linux error code
 */
int mxlk_core_open(struct mxlk_interface *inf);

/*
 * @brief closes mxlk interface
 *
 * @param[in] inf - pointer to interface instance
 *
 * @return:
 *       0 - success
 *      <0 - linux error code
 */
int mxlk_core_close(struct mxlk_interface *inf);

/*
 * @brief Read buffer from mxlk interface. Function will block when no data.
 *
 * @param[in] inf           - pointer to interface instance
 * @param[in] buffer        - pointer to userspace buffer
 * @param[in] length        - max bytes to copy into buffer
 * @param[in] timeout_ms    - timeout in ms for blocking when no data
 *
 * @return:
 *      >=0 - number of bytes read
 *      <0  - linux error code
 *              -ETIME - timeout
 *              -EINTR - interrupted
 */
ssize_t mxlk_core_read(struct mxlk_interface *inf, void *buffer, size_t *length,
		       unsigned int timeout_ms);

/*
 * @brief Writes buffer to mxlk interface. Function will block when no buffer.
 *
 * @param[in] inf           - pointer to interface instance
 * @param[in] buffer        - pointer to userspace buffer
 * @param[in] length        - length of buffer to copy from
 * @param[in] timeout_ms    - timeout in ms for blocking when no buffer
 *
 * @return:
 *      >=0 - number of bytes write
 *      <0  - linux error code
 *              -ETIME - timeout
 *              -EINTR - interrupted
 */
ssize_t mxlk_core_write(struct mxlk_interface *inf, void *buffer, size_t length,
			unsigned int timeout_ms);

struct mxlk_interface *mxlk_core_default_interface(void);

#endif
