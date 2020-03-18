/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2020 Intel Corporation
 */

#ifndef __DLB_IOCTL_H
#define __DLB_IOCTL_H

#include "dlb_main.h"

int dlb_ioctl_dispatcher(struct dlb_dev *dev,
			 unsigned int cmd,
			 unsigned long arg);

int dlb_domain_ioctl_dispatcher(struct dlb_dev *dev,
				struct dlb_status *status,
				unsigned int cmd,
				unsigned long arg,
				uint32_t domain_id);

#endif /* __DLB_IOCTL_H */
