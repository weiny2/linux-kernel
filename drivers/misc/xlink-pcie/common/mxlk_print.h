/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_PRINT_HEADER_
#define MXLK_PRINT_HEADER_

#include <linux/module.h>
#include <linux/kernel.h>

#define mxlk_debug(fmt, args...)                                               \
	pr_debug("MXLK:DEBUG - %s(%d) -- " fmt, __func__, __LINE__, ##args)
#define mxlk_error(fmt, args...)                                               \
	pr_err("MXLK:ERROR - %s(%d) -- " fmt, __func__, __LINE__, ##args)
#define mxlk_info(fmt, args...)                                                \
	pr_info("MXLK:INFO  - %s(%d) -- " fmt, __func__, __LINE__, ##args)

#endif
