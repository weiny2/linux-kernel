// SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause
/*
 * Keem Bay VPU IPC test driver
 *
 * Copyright (c) 2019 Intel Corporation.
 */

#include <linux/device.h>
#include <linux/io.h>
#include <linux/keembay-vpu-ipc.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define TEST_READY_WAIT_TIME_MS (31000)

#define TEST_NODE (KMB_VPU_IPC_NODE_LEON_MSS)
#define TEST_CHANNEL (10)
#define TEST_DATA (0x53544f50)
#define TEST_SIZE (0)

static struct device *get_vpu_dev(void)
{
	struct device_driver *vpu_drv;

	vpu_drv = driver_find("keembay-vpu-ipc", &platform_bus_type);
	if (!vpu_drv) {
		pr_err("Cannot find IPC driver\n");
		return NULL;
	}
	return driver_find_next_device(vpu_drv, NULL);
}

static int __init intel_keembay_vpu_ipc_test_send_data(struct device *dev)
{
	int ret;
	int close_ret;

	ret = intel_keembay_vpu_ipc_open_channel(dev, TEST_NODE, TEST_CHANNEL);
	if (ret) {
		pr_info("Failed to open channel %d:%d\n", TEST_NODE,
			TEST_CHANNEL);
		return ret;
	}

	ret = intel_keembay_vpu_ipc_send(dev, TEST_NODE, TEST_CHANNEL,
					 TEST_DATA, TEST_SIZE);
	if (ret) {
		pr_info("Failed to send test data 0x%x size 0x%x\n", TEST_DATA,
			TEST_SIZE);
	}

	close_ret = intel_keembay_vpu_ipc_close_channel(dev, TEST_NODE,
							TEST_CHANNEL);
	if (close_ret) {
		pr_info("Failed to close channel %d:%d\n", TEST_NODE,
			TEST_CHANNEL);
	}

	if (close_ret)
		return close_ret;

	return ret;
}

static int __init keem_bay_vpu_ipc_test_init(void)
{
	int ret;
	enum intel_keembay_vpu_state state;
	struct device *dev;

	pr_info("Entering VPU IPC test.\n");

	dev = get_vpu_dev();
	if (!dev)
		return -1;
	pr_info("Running test for device: %s\n", dev_name(dev));

	/* Check state */
	state = intel_keembay_vpu_status(dev);
	if (state != KEEMBAY_VPU_OFF) {
		pr_warn("VPU was not OFF, test may fail (it was %d)\n", state);

		/* Stop the VPU */
		ret = intel_keembay_vpu_stop(dev);
		if (ret) {
			pr_err("Failed to stop VPU: %d\n", ret);
			return ret;
		}
	}

	/* Boot VPU */
	ret = intel_keembay_vpu_startup(dev, "vpu.bin");
	if (ret) {
		pr_err("Failed to start VPU: %d\n", ret);
		return ret;
	}
	pr_info("Successfully started VPU!\n");

	/* Wait for VPU to be READY */
	ret = intel_keembay_vpu_wait_for_ready(dev, TEST_READY_WAIT_TIME_MS);
	if (ret) {
		pr_err("Tried to start VPU but never got READY.\n");
		return ret;
	}
	pr_info("Successfully synchronised state with VPU - after start!\n");

	/* Check state */
	state = intel_keembay_vpu_status(dev);
	if (state != KEEMBAY_VPU_READY) {
		pr_err("VPU was not ready, it was %d\n", state);
		return -EIO;
	}
	pr_info("VPU was ready.\n");

	/* Check WDT API */
	ret = intel_keembay_vpu_get_wdt_count(dev, KEEMBAY_VPU_NCE);
	if (ret < 0) {
		pr_err("Error getting NCE WDT count.\n");
		return ret;
	}
	pr_info("NCE WDT count = %d\n", ret);

	ret = intel_keembay_vpu_get_wdt_count(dev, KEEMBAY_VPU_MSS);
	if (ret < 0) {
		pr_err("Error getting MSS WDT count.\n");
		return ret;
	}
	pr_info("MSS WDT count = %d\n", ret);

	/* Reset the VPU */
	ret = intel_keembay_vpu_reset(dev);
	if (ret) {
		pr_err("Failed to reset VPU: %d\n", ret);
		return ret;
	}
	pr_info("Successfully reset VPU!\n");

	/* Wait for VPU to be READY */
	ret = intel_keembay_vpu_wait_for_ready(dev, TEST_READY_WAIT_TIME_MS);
	if (ret) {
		pr_err("Tried to reset VPU but never got READY.\n");
		return ret;
	}
	pr_info("Successfully synchronised state with VPU - after reset!\n");

	/* Send test data on test channel */
	ret = intel_keembay_vpu_ipc_test_send_data(dev);
	if (ret) {
		pr_err("Tried to send data but failed.\n");
		return ret;
	}
	pr_info("Successfully sent test data!\n");

	/* Stop the VPU */
	ret = intel_keembay_vpu_stop(dev);
	if (ret) {
		pr_err("Failed to stop VPU: %d\n", ret);
		return ret;
	}
	pr_info("Successfully stopped VPU!\n");

	/* Check state */
	state = intel_keembay_vpu_status(dev);
	if (state != KEEMBAY_VPU_OFF) {
		pr_err("VPU was not OFF after stop request, it was %d\n",
		       state);
		return -EIO;
	}

	put_device(dev);

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
