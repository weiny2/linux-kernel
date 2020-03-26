/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2018 Intel Corporation
 */

#ifndef __HQMV2_IOCTL_H
#define __HQMV2_IOCTL_H

#include "hqmv2_main.h"

int hqmv2_ioctl_dispatcher(struct hqmv2_dev *dev,
			   unsigned int cmd,
			   unsigned long arg);

int hqmv2_domain_ioctl_dispatcher(struct hqmv2_dev *dev,
				  unsigned int cmd,
				  unsigned long arg,
				  uint32_t domain_id);

#endif /* __HQMV2_IOCTL_H */
