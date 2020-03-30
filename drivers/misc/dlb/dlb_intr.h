/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2020 Intel Corporation
 */

#ifndef __DLB_INTR_H
#define __DLB_INTR_H

#include <linux/pci.h>

#include "dlb_main.h"

int dlb_block_on_cq_interrupt(struct dlb_dev *dev,
			      struct dlb_status *status,
			      int domain_id,
			      int port_id,
			      bool is_ldb,
			      u64 cq_va,
			      u8 cq_gen,
			      bool arm);

enum dlb_wake_reason {
	WAKE_CQ_INTR,
	WAKE_PORT_DISABLED,
	WAKE_DEV_RESET
};

void dlb_wake_thread(struct dlb_dev *dev,
		     struct dlb_cq_intr *intr,
		     enum dlb_wake_reason reason);

#endif /* __DLB_INTR_H */
