/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2020 Intel Corporation
 */

#ifndef __DLB2_INTR_H
#define __DLB2_INTR_H

#include <linux/pci.h>

#include "dlb2_main.h"

int dlb2_block_on_cq_interrupt(struct dlb2_dev *dev,
			       struct dlb2_status *status,
			       int domain_id,
			       int port_id,
			       bool is_ldb,
			       u64 cq_va,
			       u8 cq_gen,
			       bool arm);

enum dlb2_wake_reason {
	WAKE_CQ_INTR,
	WAKE_DEV_RESET,
	WAKE_PORT_DISABLED
};

void dlb2_wake_thread(struct dlb2_dev *dev,
		      struct dlb2_cq_intr *intr,
		      enum dlb2_wake_reason reason);

#endif /* __DLB2_INTR_H */
