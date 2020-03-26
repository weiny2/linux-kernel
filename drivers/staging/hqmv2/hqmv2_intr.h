/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2018 Intel Corporation
 */

#ifndef __HQMV2_INTR_H
#define __HQMV2_INTR_H

#include <linux/pci.h>
#include "hqmv2_main.h"

int hqmv2_block_on_cq_interrupt(struct hqmv2_dev *dev,
				int domain_id,
				int port_id,
				bool is_ldb,
				u64 cq_va,
				u8 cq_gen,
				bool arm);

enum hqmv2_wake_reason {
	WAKE_CQ_INTR,
	WAKE_DEV_RESET,
	WAKE_PORT_DISABLED
};

void hqmv2_wake_thread(struct hqmv2_dev *dev,
		       struct hqmv2_cq_intr *intr,
		       enum hqmv2_wake_reason reason);

#endif /* __HQMV2_INTR_H */
