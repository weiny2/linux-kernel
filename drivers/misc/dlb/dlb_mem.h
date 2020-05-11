/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2020 Intel Corporation
 */

#ifndef __DLB_MEM_H
#define __DLB_MEM_H

#include "dlb_main.h"

void dlb_release_domain_memory(struct dlb_dev *dev, u32 domain_id);

void dlb_release_device_memory(struct dlb_dev *dev);

#endif /* __DLB_MEM_H */
