// SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause
/*
 * Keem Bay VPU IPC test driver
 *
 * Copyright (c) 2019 Intel Corporation.
 */

#include <linux/io.h>
#include <linux/keembay-vpu-ipc.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define KEEMBAY_VPU_IPC_TEST_READY_WAIT_TIME_MS (31000)

static int __init keem_bay_vpu_ipc_test_init(void)
{
	int ret;
	enum intel_keembay_vpu_state state;

	pr_info("Entering VPU IPC test.\n");

	/* Check state */
	state = intel_keembay_vpu_status();
	if (state != KEEMBAY_VPU_OFF) {
		pr_warn("VPU was not OFF, test may fail (it was %d)\n", state);

		/* Stop the VPU */
		ret = intel_keembay_vpu_stop();
		if (ret) {
			pr_err("Failed to stop VPU: %d\n", ret);
			return ret;
		}
	}

	/* Boot VPU */
	ret = intel_keembay_vpu_startup("vpu.bin");
	if (ret) {
		pr_err("Failed to start VPU: %d\n", ret);
		return ret;
	}
	pr_info("Successfully started VPU!\n");

	/* Wait for VPU to be READY */
	ret = intel_keembay_vpu_wait_for_ready(
		KEEMBAY_VPU_IPC_TEST_READY_WAIT_TIME_MS);
	if (ret) {
		pr_err("Tried to start VPU but never got READY.\n");
		return ret;
	}
	pr_info("Successfully synchronised state with VPU!\n");

	/* Check state */
	state = intel_keembay_vpu_status();
	if (state != KEEMBAY_VPU_READY) {
		pr_err("VPU was not ready, it was %d\n", state);
		return -EIO;
	}
	pr_info("VPU was ready.\n");

	/* Check WDT API */
	ret = intel_keembay_vpu_get_wdt_count(KEEMBAY_VPU_NCE);
	if (ret < 0) {
		pr_err("Error getting NCE WDT count.\n");
		return ret;
	}
	pr_info("NCE WDT count = %d\n", ret);

	ret = intel_keembay_vpu_get_wdt_count(KEEMBAY_VPU_MSS);
	if (ret < 0) {
		pr_err("Error getting MSS WDT count.\n");
		return ret;
	}
	pr_info("MSS WDT count = %d\n", ret);

	/* Stop the VPU */
	ret = intel_keembay_vpu_stop();
	if (ret) {
		pr_err("Failed to stop VPU: %d\n", ret);
		return ret;
	}
	pr_info("Successfully stopped VPU!\n");

	/* Check state */
	state = intel_keembay_vpu_status();
	if (state != KEEMBAY_VPU_OFF) {
		pr_err("VPU was not OFF after stop request, it was %d\n",
		       state);
		return -EIO;
	}

	return 0;
}

static void __exit keem_bay_vpu_ipc_test_exit(void)
{
	pr_info("Leaving VPU IPC test.\n");
}

module_init(keem_bay_vpu_ipc_test_init);
module_exit(keem_bay_vpu_ipc_test_exit);

MODULE_DESCRIPTION("Keem Bay VPU IPC test driver");
MODULE_AUTHOR("Paul Murphy <paul.j.murphy@intel.com>");
MODULE_LICENSE("Dual BSD/GPL");
