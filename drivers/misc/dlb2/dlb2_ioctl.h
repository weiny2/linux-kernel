/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2020 Intel Corporation
 */

#ifndef __DLB2_IOCTL_H
#define __DLB2_IOCTL_H

#include "dlb2_main.h"

int dlb2_ioctl_dispatcher(struct dlb2_dev *dev,
			  unsigned int cmd,
			  unsigned long arg);

int dlb2_domain_ioctl_dispatcher(struct dlb2_dev *dev,
				 struct dlb2_status *status,
				 unsigned int cmd,
				 unsigned long arg,
				 uint32_t domain_id);

#endif /* __DLB2_IOCTL_H */
