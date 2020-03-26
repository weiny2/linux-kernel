/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * KeemBay OCS AES Crypto Driver.
 *
 * Copyright (C) 2018-2019 Intel Corporation
 */

#ifndef _KEEMBAY_OCS_AES_CORE_H
#define _KEEMBAY_OCS_AES_CORE_H

struct keembay_ocs_aes_dev {
	struct platform_device *plat_dev;
	int irq;
	int irq_enabled;
	spinlock_t irq_lock;
	void __iomem *base_reg;
	void __iomem *base_wrapper_reg;
	struct clk *ocs_clk;
};

/*
 * Global variable pointing to our KeemBay OCS AES Device.
 *
 * This is meant to be used when platform_get_drvdata() cannot be used
 * because we lack a reference to our platform_device.
 */
extern struct keembay_ocs_aes_dev *kmb_ocs_aes_dev;

#endif
