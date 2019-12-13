// SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause
/*
 * Keem Bay VPU IPC test driver
 *
 * Copyright (c) 2019 Intel Corporation.
 */

#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/keembay-vpu-ipc.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define TEST_READY_WAIT_TIME_MS (31000)

#define TEST_NODE (KMB_VPU_IPC_NODE_LEON_MSS)
#define TEST_CHANNEL (10)
#define TEST_DATA (0x53544f50)
#define TEST_SIZE (0)

/* Device Under Test (DUT) struct. */
struct dut {
	struct device *dev;
	struct list_head list;
};

/* The struct grouping all the parameters to be passed to a test kthread. */
struct test_params {
	struct device *ipc_dev;
	unsigned long chan_id;
};

/* Forward declaration of the write function for the 'test_vpu' file. */
static ssize_t test_vpu_wr(struct file *, const char __user *, size_t,
			   loff_t *);

/* List of VPU/IPC devices under test. */
static LIST_HEAD(dut_list);

/*
 * The dentry of the base debugfs directory for this module
 * /<dbgfs>/keembay_vpu_ipc_test
 */
static struct dentry *test_dir;

/* The file operations for the 'test_vpu' file. */
static const struct file_operations test_vpu_fops = {
	.owner	= THIS_MODULE,
	.write	= test_vpu_wr,
};

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

static int run_vpu_test(struct device *dev)
{
	int ret;
	enum intel_keembay_vpu_state state;

	pr_info("Starting VPU/IPC test for device: %s\n", dev_name(dev));

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

	return 0;
}

static ssize_t test_vpu_wr(struct file *filp, const char __user *buf,
			   size_t count, loff_t *fpos)
{
	int rc;
	struct device *vpu_dev = filp->f_inode->i_private;

	pr_info("TEST_VPU write: %s\n", dev_name(vpu_dev));

	rc = run_vpu_test(vpu_dev);
	if (rc)
		return rc;

	return count;
}

/*
 * Add an IPC device to be tested.
 */
static int add_dev_to_dbgfs_tree(struct device *dev, struct dentry *base_dir)
{
	struct dentry *dev_dir;
	struct dentry *file;

	dev_dir = debugfs_create_dir(dev_name(dev), base_dir);
	if (!dev_dir)
		return -ENOMEM;
	pr_info("TEST_MOD: device dir created: %s\n", dev_name(dev));

	/* Create test_vpu file */
	file = debugfs_create_file("test_vpu", 0220, dev_dir, dev,
				   &test_vpu_fops);
	if (!file)
		return -ENOMEM;
	pr_info("TEST_MOD: send file created\n");

	return 0;
}

/* Remove the DebugFS tree. */
static void remove_vpu_test_dbgfs_tree(void)
{
	debugfs_remove_recursive(test_dir);
}

/* Create the DebugFS tree. */
static int create_vpu_test_dbgfs_tree(struct list_head *list)
{
	struct dut *dut;
	struct list_head *pos;
	int rc = 0;

	test_dir = debugfs_create_dir("keembay_vpu_ipc_test", NULL);
	/*
	 * The only error can be -ENODEV; i.e., debugfs is not enabled.
	 * Therefore, the IS_ERR(test_dir) check is done only here.
	 */
	if (IS_ERR(test_dir))
		return PTR_ERR(test_dir);
	if (!test_dir)
		return -ENOMEM;
	pr_info("TEST_MOD: kmb_vpu_ipc_test dir created\n");

	list_for_each(pos, list) {
		dut = list_entry(pos, struct dut, list);
		rc = add_dev_to_dbgfs_tree(dut->dev, test_dir);
		if (rc) {
			remove_vpu_test_dbgfs_tree();
			return rc;
		}
	}
	return 0;
}

/* Release the list of DUTs. */
static void free_dut_list(struct list_head *list)
{
	struct dut *dut;
	struct list_head *pos, *q;

	list_for_each_safe(pos, q, list) {
		list_del(pos);
		dut = list_entry(pos, struct dut, list);
		pr_info("Removing DUT %s\n", dev_name(dut->dev));
		put_device(dut->dev);
		kfree(dut);
	}
}

/* Initialize the list of DUTs. */
static int init_dut_list(struct list_head *list)
{
	struct device_driver *vpu_drv;
	struct device *dev = NULL;
	struct dut *dut;

	vpu_drv = driver_find("keembay-vpu-ipc", &platform_bus_type);
	if (!vpu_drv) {
		pr_err("Cannot find VPU/IPC driver\n");
		return -ENODEV;
	}

	while ((dev = driver_find_next_device(vpu_drv, dev))) {
		dut = kmalloc(sizeof(*dut), GFP_KERNEL);
		if (!dut) {
			free_dut_list(list);
			return -ENOMEM;
		}
		pr_info("Adding DUT %s\n", dev_name(dev));
		dut->dev = dev;
		list_add_tail(&dut->list, &dut_list);
	}

	return 0;
}

static int __init keem_bay_vpu_ipc_test_init(void)
{
	int rc;

	pr_info("Loading VPU IPC test module.\n");

	rc = init_dut_list(&dut_list);
	if (rc) {
		pr_err("Failed to init DUT list\n");
		return rc;
	}
	rc = create_vpu_test_dbgfs_tree(&dut_list);
	if (rc) {
		pr_err("Failed to create debugfs tree\n");
		free_dut_list(&dut_list);
		return rc;
	}
	return 0;
}

static void __exit keem_bay_vpu_ipc_test_exit(void)
{
	pr_info("Removing VPU IPC test module.\n");
	remove_vpu_test_dbgfs_tree();
	free_dut_list(&dut_list);
}

module_init(keem_bay_vpu_ipc_test_init);
module_exit(keem_bay_vpu_ipc_test_exit);

MODULE_DESCRIPTION("Keem Bay VPU IPC test driver");
MODULE_AUTHOR("Paul Murphy <paul.j.murphy@intel.com>");
MODULE_AUTHOR("Daniele Alessandrelli <daniele.alessandrelli@intel.com>");
MODULE_LICENSE("Dual BSD/GPL");
