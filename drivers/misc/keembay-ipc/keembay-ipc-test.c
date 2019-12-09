// SPDX-License-Identifier: GPL-2.0
/*
 * kmb-ipc-test.c - Keem Bay IPC Test module.
 *
 * Copyright (c) 2019 Intel Corporation.
 */

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/keembay-ipc.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Daniele Alessandrelli");
MODULE_DESCRIPTION("A simple Linux module for testing Keem Bay IPC.");
MODULE_VERSION("0.2");

/* Timeout out in ms. */
#define RECV_TIMEOUT	(15 * 1000)

#define LOCAL_IPC_MEM_VADDR ((void *)0xffff000009200000)
#define LOCAL_IPC_MEM_SIZE  0x200000

/* IPC buffer. */
struct kmb_ipc_buf {
	u32 data_paddr; /* Physical address where payload is located. */
	u32 data_size;  /* Size of payload. */
	u16 channel;    /* The channel used. */
	u8 src_node;    /* The Node ID of the sender. */
	u8 dst_node;    /* The Node ID of the intended receiver. */
	u8 status;	/* Either free or allocated. */
} __packed __aligned(64);

struct test_params {
	struct device *ipc_dev;
	unsigned long chan_id;
};

struct dut {
	struct device *dev;
	struct list_head list;
};

static void remove_ipc_test_dbgfs_tree(void);
static ssize_t recv_wr(struct file *, const char __user *, size_t, loff_t *);
static ssize_t send_wr(struct file *, const char __user *, size_t, loff_t *);
static ssize_t open_wr(struct file *, const char __user *, size_t, loff_t *);
static ssize_t close_wr(struct file *, const char __user *, size_t, loff_t *);
static ssize_t test_loop_wr(struct file *filp, const char __user *buf,
			    size_t count, loff_t *fpos);
static ssize_t test_long_send_wr(struct file *filp, const char __user *buf,
				 size_t count, loff_t *fpos);

/* The Keem Bay IPC device under test. */
static LIST_HEAD(dut_list);

/*
 * The dentry of the base debugfs directory for thie module
 * /<dbgfs>/kmb_ipc_test
 */
static struct dentry *test_dir;

/* The file operations for the 'recv' file. */
static const struct file_operations recv_fops = {
	.owner	= THIS_MODULE,
	.write	= recv_wr,
};

/* The file operations for the 'send' file. */
static const struct file_operations send_fops = {
	.owner	= THIS_MODULE,
	.write	= send_wr,
};

/* The file operations for the 'open' file. */
static const struct file_operations open_fops = {
	.owner	= THIS_MODULE,
	.write	= open_wr,
};

/* The file operations for the 'close' file. */
static const struct file_operations close_fops = {
	.owner	= THIS_MODULE,
	.write	= close_wr,
};

/* The file operations for the 'close' file. */
static const struct file_operations test_loop_fops = {
	.owner	= THIS_MODULE,
	.write	= test_loop_wr,
};

/* The file operations for the 'close' file. */
static const struct file_operations test_long_send_fops = {
	.owner	= THIS_MODULE,
	.write	= test_long_send_wr,
};

static int ipc_buf_used_cnt(struct device *dev)
{
	struct device_node *node;
	struct resource res;
	struct kmb_ipc_buf *buffers;
	size_t buf_cnt;
	int rc, i, used = 0;

	/* Find local IPC memory resource. */
	node = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (!node) {
		pr_err("Couldn't find local IPC memory region for %s.\n",
		       dev_name(dev));
		return -EINVAL;
	}
	rc = of_address_to_resource(node, 0, &res);
	/* Release node first as we will not use it anymore */
	of_node_put(node);
	if (rc) {
		pr_err("Couldn't resolve IPC memory region for %s\n",
		       dev_name(dev));
		return rc;
	}
	/* Map IPC physical memory to virtual address space. */
	buffers = memremap(res.start, resource_size(&res), MEMREMAP_WB);
	if (!buffers) {
		pr_err("Couldn't map IPC memory region for %s\n",
		       dev_name(dev));
		return -ENOMEM;
	}
	buf_cnt = resource_size(&res) / sizeof(struct kmb_ipc_buf);
	for (i = 0; i < buf_cnt; i++) {
		if (buffers[i].status)
			used++;
	}
	WARN_ON(i != 32768);
	memunmap(buffers);

	return used;
}

/* Test loop thread func. */
static int test_loop_thread_fn(void *data)
{
	struct test_params *params = data;
	u16 chan_id = params->chan_id;
	struct device *ipc_dev = params->ipc_dev;
	int rc;
	u32 paddr1, paddr2;
	size_t size1, size2;
	char name[TASK_COMM_LEN];

	get_task_comm(name, current);
	pr_info("%s: started\n", name);

	rc = intel_keembay_ipc_open_channel(ipc_dev, KMB_IPC_NODE_LEON_MSS,
					    chan_id);
	if (rc) {
		pr_info("Failed to open channel: %d\n", rc);
		goto err;
	}

	while (!kthread_should_stop()) {
		rc = intel_keembay_ipc_recv(ipc_dev, KMB_IPC_NODE_LEON_MSS,
					    chan_id, &paddr1, &size1, U32_MAX);
		if (rc) {
			pr_info("Failed 1st recv(): %d\n", rc);
			goto err;
		}
		rc = intel_keembay_ipc_recv(ipc_dev, KMB_IPC_NODE_LEON_MSS,
					    chan_id, &paddr2, &size2, U32_MAX);
		if (rc) {
			pr_info("Failed 2nd recv(): %d\n", rc);
			goto err;
		}
		rc = intel_keembay_ipc_send(ipc_dev, KMB_IPC_NODE_LEON_MSS,
					    chan_id, paddr1, size1);
		if (rc) {
			pr_info("Failed 1st send(): %d\n", rc);
			goto err;
		}
		rc = intel_keembay_ipc_send(ipc_dev, KMB_IPC_NODE_LEON_MSS,
					    chan_id, paddr2, size2);
		if (rc) {
			pr_info("Failed 2nd send(): %d\n", rc);
			goto err;
		}
	}

	rc = intel_keembay_ipc_close_channel(ipc_dev, KMB_IPC_NODE_LEON_MSS,
					     chan_id);
	if (rc) {
		pr_info("Failed to close channel: %d\n", rc);
		/* Do not jump to error, as we are exiting already. */
	}
	pr_info("Test loop thread stopping; channel: %d\n", chan_id);
	kfree(data);

	return rc;
err:
	pr_info("Test loop thread exiting; channel: %d\n", chan_id);
	kfree(data);
	/* Leave channel open. */
	/* TODO: do we get here even when we get a signal by kthread_stop()? */
	do_exit(rc);
}

/* Test long send thread func. */
static int test_long_send_thread_fn(void *data)
{
	struct test_params *params = data;
	u16 chan_id = params->chan_id;
	struct device *ipc_dev = params->ipc_dev;
	int rc;
	char name[TASK_COMM_LEN];
	u32 send_cnt = 0;
	const u32 send_limit = 1000000;

	get_task_comm(name, current);
	pr_info("%s: started\n", name);

	rc = intel_keembay_ipc_open_channel(ipc_dev, KMB_IPC_NODE_LEON_MSS,
					    chan_id);
	if (rc) {
		pr_info("Failed to open channel: %d\n", rc);
		goto err_open;
	}

	while (!kthread_should_stop() && send_cnt < send_limit) {
		rc = intel_keembay_ipc_send(ipc_dev, KMB_IPC_NODE_LEON_MSS,
					    chan_id, 0xDEADBEEF, 42);
		/* If FIFO full sleep and retry. */
		if (rc == -EBUSY) {
			usleep_range(1000, 2000);
			continue;
		}
		if (rc) {
			pr_info("Failed send(): %d\n", rc);
			break;
		}
		send_cnt++;
		if (!(send_cnt % 1000))
			pr_info("send_cnt: %u\n", send_cnt);
	}

	rc = intel_keembay_ipc_close_channel(ipc_dev, KMB_IPC_NODE_LEON_MSS,
					     chan_id);
	if (rc) {
		pr_info("Failed to close channel: %d\n", rc);
		/* Do not jump to error, as we are exiting already. */
	}
	msleep(1000);
	pr_info("%s: IPC buf used: %d\n", name, ipc_buf_used_cnt(ipc_dev));
err_open:
	pr_info("%s: stopping\n", name);

	/* Exit properly depending on what caused the thread to stop. */
	if (kthread_should_stop()) {
		kfree(data);
		return rc;
	}
	kfree(data);
	do_exit(rc);
}

/* Parse channel from user buffer. */
int parse_chan(const char __user *buf, size_t count, unsigned long *chan_id)
{
	char *args;
	int rc;

	/* Consume the data */
	args = memdup_user_nul(buf, count);
	if (IS_ERR(args))
		return PTR_ERR(args);
	rc = kstrtoul(args, 0, chan_id);
	kfree(args);

	return rc;
}

/* Write operation for the 'recv' file. */
static ssize_t recv_wr(struct file *filp, const char __user *buf, size_t count,
		       loff_t *fpos)
{
	u32 paddr;
	size_t size;
	int rc;
	unsigned long chan_id;
	struct device *ipc_dev = filp->f_inode->i_private;

	pr_info("RECV write: %s\n", dev_name(ipc_dev));

	rc = parse_chan(buf, count, &chan_id);
	if (rc)
		return rc;

	rc = intel_keembay_ipc_recv(ipc_dev, KMB_IPC_NODE_LEON_MSS, chan_id,
				    &paddr, &size, RECV_TIMEOUT);
	if (rc)
		pr_info("recv(chan_id=%lu) failed: %d\n", chan_id, rc);
	else
		pr_info("recv(chan_id=%lu) paddr: %u\tsize: %ld\n", chan_id,
			paddr, size);

	return count;
}

/* Write operation for the 'send' file. */
static ssize_t send_wr(struct file *filp, const char __user *buf, size_t count,
		       loff_t *fpos)
{
	int rc;
	unsigned long chan_id;
	struct device *ipc_dev = filp->f_inode->i_private;

	pr_info("SEND write: %s\n", dev_name(ipc_dev));
	if (*fpos) {
		pr_err("TEST_MOD: write offset must always be zero\n");
		return -EINVAL;
	}

	rc = parse_chan(buf, count, &chan_id);
	if (rc)
		return rc;

	rc = intel_keembay_ipc_send(ipc_dev, KMB_IPC_NODE_LEON_MSS, chan_id,
				    0xDEADBEEF, 48);
	pr_info("send(chan_id=%lu): %d\n", chan_id, rc);

	return count;
}

/* Write operation for the 'close' file. */
static ssize_t open_wr(struct file *filp, const char __user *buf, size_t count,
		       loff_t *fpos)
{
	int rc;
	unsigned long chan_id;
	struct device *ipc_dev = filp->f_inode->i_private;

	pr_info("OPEN write: %s\n", dev_name(ipc_dev));
	rc = parse_chan(buf, count, &chan_id);
	if (rc)
		return rc;

	rc = intel_keembay_ipc_open_channel(ipc_dev, KMB_IPC_NODE_LEON_MSS,
					    chan_id);
	pr_info("open(chan_id=%lu): %d\n", chan_id, rc);

	return count;
}

/* Write operation for the 'open' file. */
static ssize_t close_wr(struct file *filp, const char __user *buf, size_t count,
			loff_t *fpos)
{
	int rc;
	unsigned long chan_id;
	struct device *ipc_dev = filp->f_inode->i_private;

	pr_info("CLOSE write: %s\n", dev_name(ipc_dev));

	rc = parse_chan(buf, count, &chan_id);
	if (rc)
		return rc;

	rc = intel_keembay_ipc_close_channel(ipc_dev, KMB_IPC_NODE_LEON_MSS,
					     chan_id);
	pr_info("close(chan_id=%lu): %d\n", chan_id, rc);

	return count;
}

/* Write operation for the 'test-loop' file. */
static ssize_t test_loop_wr(struct file *filp, const char __user *buf,
			    size_t count, loff_t *fpos)
{
	int rc;
	struct task_struct *test_loop_thread;
	struct test_params *params = NULL;
	unsigned long chan_id;
	struct device *ipc_dev = filp->f_inode->i_private;

	pr_info("TEST_LOOP write: %s\n", dev_name(ipc_dev));

	rc = parse_chan(buf, count, &chan_id);
	if (rc)
		return rc;

	params = kmalloc(sizeof(*params), GFP_KERNEL);
	if (!params)
		return -ENOMEM;
	params->chan_id = chan_id;
	params->ipc_dev = ipc_dev;
	pr_info("Starting test loop for channel %lu\n", chan_id);
	test_loop_thread = kthread_run(&test_loop_thread_fn, (void *)params,
				       "test-loop-%lu", chan_id);
	if (IS_ERR(test_loop_thread)) {
		kfree(params);
		return PTR_ERR(test_loop_thread);
	}

	return count;
}

/* Write operation for the 'test-long' file. */
static ssize_t test_long_send_wr(struct file *filp, const char __user *buf,
				 size_t count, loff_t *fpos)
{
	int rc;
	struct task_struct *thread;
	struct test_params *params = NULL;
	unsigned long chan_id;
	struct device *ipc_dev = filp->f_inode->i_private;

	pr_info("TEST_LONG write: %s\n", dev_name(ipc_dev));

	rc = parse_chan(buf, count, &chan_id);
	if (rc)
		return rc;

	params = kmalloc(sizeof(*params), GFP_KERNEL);
	if (!params)
		return -ENOMEM;
	params->chan_id = chan_id;
	params->ipc_dev = ipc_dev;
	pr_info("Starting test long for channel %lu\n", chan_id);
	thread = kthread_run(&test_long_send_thread_fn, (void *)params,
			     "t-long-send-%lu", chan_id);
	if (IS_ERR(thread)) {
		kfree(params);
		return PTR_ERR(thread);
	}

	return count;
}

/*
 * My debugfs tree.
 */

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

	/* create send file */
	file = debugfs_create_file("send", 0220, dev_dir, dev,
				   &send_fops);
	if (!file)
		return -ENOMEM;
	pr_info("TEST_MOD: send file created\n");
	/* Create open file. */
	file = debugfs_create_file("open", 0220, dev_dir, dev,
				   &open_fops);
	if (!file)
		return -ENOMEM;
	pr_info("TEST_MOD: open file created\n");
	/* Create close file. */
	file = debugfs_create_file("close", 0220, dev_dir, dev,
				   &close_fops);
	if (!file)
		return -ENOMEM;
	pr_info("TEST_MOD: close file created\n");

	/* Create close file. */
	file = debugfs_create_file("recv", 0220, dev_dir, dev,
				   &recv_fops);
	if (!file)
		return -ENOMEM;
	pr_info("TEST_MOD: recv file created\n");

	/* Create test-loop file. */
	file = debugfs_create_file("test-loop", 0220, dev_dir, dev,
				   &test_loop_fops);
	if (!file)
		return -ENOMEM;
	pr_info("TEST_MOD: test-loop file created\n");

	/* Create test-long-send file. */
	file = debugfs_create_file("test-long-send", 0220, dev_dir, dev,
				   &test_long_send_fops);
	if (!file)
		return -ENOMEM;
	pr_info("TEST_MOD: test-long-send file created\n");

	return 0;
}

/* Create the DebugFS tree. */
static int create_ipc_test_dbgfs_tree(struct list_head *list)
{
	struct dut *dut;
	struct list_head *pos;
	int rc = 0;

	test_dir = debugfs_create_dir("kmb_ipc_test", NULL);
	/*
	 * The only error can be -ENODEV; i.e., debugfs is not enabled.
	 * Therefore, the IS_ERR(test_dir) check is done only here.
	 */
	if (IS_ERR(test_dir))
		return PTR_ERR(test_dir);
	if (!test_dir)
		return -ENOMEM;
	pr_info("TEST_MOD: kmb_ipc_test dir created\n");

	list_for_each(pos, list) {
		dut = list_entry(pos, struct dut, list);
		rc = add_dev_to_dbgfs_tree(dut->dev, test_dir);
		if (rc) {
			remove_ipc_test_dbgfs_tree();
			return rc;
		}
	}
	return 0;
}

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

static int init_dut_list(struct list_head *list)
{
	struct device_driver *ipc_drv;
	struct device *dev = NULL;
	struct dut *dut;

	ipc_drv = driver_find("kmb-ipc-driver", &platform_bus_type);
	if (!ipc_drv) {
		pr_err("Cannot find IPC driver\n");
		return -ENODEV;
	}

	while ((dev = driver_find_next_device(ipc_drv, dev))) {
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

/* Remove the DebugFS tree. */
static void remove_ipc_test_dbgfs_tree(void)
{
	debugfs_remove_recursive(test_dir);
}

/*
 * Module loading / unloading.
 */

static int __init kmb_ipc_test_init(void)
{
	int rc;

	pr_info("TEST_MOD: Init\n");

	rc = init_dut_list(&dut_list);
	if (rc) {
		pr_err("Failed to init DUT list\n");
		return rc;
	}
	rc = create_ipc_test_dbgfs_tree(&dut_list);
	if (rc) {
		pr_err("Failed to create debugfs tree\n");
		free_dut_list(&dut_list);
		return rc;
	}

	return 0;
}

static void __exit kmb_ipc_test_exit(void)
{
	pr_info("TEST_MOD: Exit\n");
	remove_ipc_test_dbgfs_tree();
	free_dut_list(&dut_list);
}

module_init(kmb_ipc_test_init);
module_exit(kmb_ipc_test_exit);
