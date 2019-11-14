// SPDX-License-Identifier: GPL-2.0-only
/*
 * Thunderbolt character device
 *
 * Copyright (C) 2019, Intel Corporation
 * Author: Raanan Avargil <raanan.avargil@intel.com>
 */

#define pr_fmt(fmt) "thunderbolt: tb_dbg: " fmt

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include <uapi/linux/thunderbolt.h>

#include "tb.h"

#define TB_DBG_MAX_DEVICE	8
#define TB_DBG_MAJOR		MAJOR(tb_dbg_t)
#define TB_DBG_MKDEV(tb)	MKDEV(MAJOR(tb_dbg_t), tb->index)
#define TB_DBG_NHI_MAX_OFFSET	0x39884

/* Holds major */
static dev_t tb_dbg_t;
static struct class *tb_dbg_class;

/**
 * struct tb_dbg - Thunderbolt debug struct to support character device
 * @tb: Pointer to the Thunderbolt domain
 * @dev: Pointer to the device (per domain) at /dev/
 * @cdev: Character device per domain
 * @is_open: Boolean to indicate whether the character device file is
 *	     already open. Multiple opens are not allowed.
 * @list: All character devices are kept via this list head element.
 */
struct tb_dbg {
	struct tb *tb;
	struct device *dev;
	struct cdev cdev;
	bool is_open;
	struct list_head list;
};

/* A linked list to hold all character devices */
static LIST_HEAD(tb_dbg_list);

/*
 * A lock object to sync operations. It is used whenever we perform an
 * operation on the character devices list, or update/check the correspond
 * is_open variable.
 */
static DEFINE_MUTEX(tb_dbg_lock);

static struct tb_dbg *__tb_dbg_get_by_minor(unsigned int index)
{
	struct tb_dbg *tb_dbg;

	list_for_each_entry(tb_dbg, &tb_dbg_list, list) {
		if (tb_dbg->tb->index == index)
			return tb_dbg;
	}

	return NULL;
}

static struct tb_dbg *tb_dbg_get_by_minor(unsigned int index)
{
	struct tb_dbg *tb_dbg;

	mutex_lock(&tb_dbg_lock);
	tb_dbg = __tb_dbg_get_by_minor(index);
	mutex_unlock(&tb_dbg_lock);

	return tb_dbg;
}

static struct tb_dbg *tb_dbg_get_free(struct tb *tb)
{
	struct tb_dbg *tb_dbg;

	if (tb->index >= TB_DBG_MAX_DEVICE) {
		tb_err(tb, "tb_dbg: out of device minors (%d)\n", tb->index);
		return ERR_PTR(-ENODEV);
	}

	tb_dbg = kzalloc(sizeof(*tb_dbg), GFP_KERNEL);
	if (!tb_dbg)
		return ERR_PTR(-ENOMEM);

	tb_dbg->tb = tb;
	tb_dbg->is_open = false;

	mutex_lock(&tb_dbg_lock);
	list_add_tail(&tb_dbg->list, &tb_dbg_list);
	mutex_unlock(&tb_dbg_lock);

	return tb_dbg;
}

static void put_tb_dbg(struct tb_dbg *tb_dbg)
{
	mutex_lock(&tb_dbg_lock);
	list_del(&tb_dbg->list);
	mutex_unlock(&tb_dbg_lock);

	kfree(tb_dbg);
}

static int tb_dbg_open(struct inode *inode, struct file *file)
{
	struct tb_dbg *tb_dbg;
	int ret;

	mutex_lock(&tb_dbg_lock);
	tb_dbg = __tb_dbg_get_by_minor(iminor(inode));
	if (!tb_dbg) {
		ret = -ENODEV;
		goto err_unlock;
	}

	/* Allow only one open at a time */
	if (tb_dbg->is_open) {
		ret = -EBUSY;
		goto err_unlock;
	}
	tb_dbg->is_open = true;
	mutex_unlock(&tb_dbg_lock);

	file->private_data = tb_dbg;

	//TODO: stop CM event handling
	tb_dbg(tb_dbg->tb, "tb_dbg: device with major %u, minor %u was opened\n",
		imajor(inode), iminor(inode));

	return 0;

err_unlock:
	mutex_unlock(&tb_dbg_lock);
	return ret;
}

static int tb_dbg_release(struct inode *inode, struct file *file)
{
	struct tb_dbg *tb_dbg = file->private_data;

	mutex_lock(&tb_dbg_lock);
	tb_dbg->is_open = false;
	mutex_unlock(&tb_dbg_lock);

	//TODO: enable CM event handling
	tb_dbg(tb_dbg->tb, "tb_dbg: device with major %u, minor %u was released\n",
		imajor(inode), iminor(inode));

	return 0;
}

static int tb_dbg_read_config_space(struct tb *tb, unsigned long arg)
{
	struct tb_dbg_query_config_space qry;
	size_t buffer_size;
	u32 *buffer;
	int ret;

	ret = copy_from_user(&qry, (void __user *)arg, sizeof(qry));
	if (ret)
		return -EFAULT;

	/* Validate input params */
	if (!qry.length || qry.length > TB_MAX_CONFIG_RW_LENGTH)
		return -EINVAL;

	// TODO: validate rest of them:
	// - route
	// - adapter_number
	// - space
	// - address
	buffer_size = sizeof(u32) * qry.length;
	buffer = kzalloc(buffer_size, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	ret = tb_cfg_read(tb->ctl, buffer, qry.route, qry.adapter_number,
			  (enum tb_cfg_space)qry.space, qry.address,
			  qry.length);
	if (ret)
		goto out;

	if (copy_to_user((void __user *)arg, &qry, sizeof(qry))) {
		ret = -EFAULT;
		goto out;
	}

	if (copy_to_user((void __user *)qry.buffer, buffer, buffer_size))
		ret = -EFAULT;

out:
	kfree(buffer);
	return ret;
}

static int tb_dbg_write_config_space(struct tb *tb, unsigned long arg)
{
	struct tb_dbg_query_config_space qry;
	u32 *buffer;
	int ret;

	if (copy_from_user(&qry, (void __user *)arg, sizeof(qry)))
		return -EFAULT;

	/* Validate input params */
	if (!qry.length || qry.length > TB_MAX_CONFIG_RW_LENGTH)
		return -EINVAL;

	// TODO: validate rest of them:
	// - route
	// - adapter_number
	// - space
	// - address
	buffer = memdup_user((void __user *)qry.buffer, sizeof(u32) * qry.length);
	if (IS_ERR(buffer))
		return PTR_ERR(buffer);

	ret = tb_cfg_write(tb->ctl, buffer, qry.route, qry.adapter_number,
			   (enum tb_cfg_space)qry.space, qry.address,
			   qry.length);

	kfree(buffer);
	return ret;
}

// TODO: This most likely will be debugfs
static int tb_dbg_read_nhi(struct tb *tb, unsigned long arg)
{
	struct tb_dbg_query_nhi qry;

	if (copy_from_user(&qry, (void __user *)arg, sizeof(qry)))
		return -EFAULT;

	/* Validate input params */
	if (qry.offset > TB_DBG_NHI_MAX_OFFSET)
		return -EINVAL;

	qry.value = ioread32(tb->nhi->iobase + qry.offset);

	if (copy_to_user((void __user *)arg, &qry, sizeof(qry)))
		return -EFAULT;

	return 0;
}

// TODO: Do we need write side at all?
static int tb_dbg_write_nhi(struct tb *tb, unsigned long arg)
{
	struct tb_dbg_query_nhi qry;

	if (copy_from_user(&qry, (void __user *)arg, sizeof(qry)))
		return -EFAULT;

	/* Validate input params */
	if (qry.offset > TB_DBG_NHI_MAX_OFFSET)
		return -EINVAL;

	iowrite32(qry.value, tb->nhi->iobase + qry.offset);
	return 0;
}

static long tb_dbg_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct tb_dbg *tb_dbg = file->private_data;
	struct tb *tb = tb_dbg->tb;

	switch (cmd) {
	case TB_DBG_IOC_READ_TBT:
		tb_dbg(tb, "tb_dbg_ioctl TB_DBG_IOC_READ_TBT cmd\n");
		return tb_dbg_read_config_space(tb, arg);

	case TB_DBG_IOC_WRITE_TBT:
		tb_dbg(tb, "tb_dbg_ioctl TB_DBG_IOC_WRITE_TBT cmd\n");
		return tb_dbg_write_config_space(tb, arg);

	case TB_DBG_IOC_READ_NHI:
		tb_dbg(tb, "tb_dbg_ioctl TB_DBG_IOC_READ_NHI cmd\n");
		return tb_dbg_read_nhi(tb, arg);

	case TB_DBG_IOC_WRITE_NHI:
		tb_dbg(tb, "tb_dbg_ioctl TB_DBG_IOC_WRITE_NHI cmd\n");
		return tb_dbg_write_nhi(tb, arg);
	}

	tb_warn(tb, "tb_dbg_ioctl unkown cmd\n");
	return -ENOTTY;
}

static const struct file_operations tb_dbg_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.open = tb_dbg_open,
	.release = tb_dbg_release,
	.unlocked_ioctl	= tb_dbg_ioctl
};

/**
 * tb_dbg_register_domain() - Register a char device for domain
 * @tb: Domain pointer.
 *
 * Registers debugging char device for domain.
 */
void tb_dbg_register_domain(struct tb *tb)
{
	struct tb_dbg *tb_dbg;
	int ret;

	if (WARN_ON(!tb))
		return;

	/* Ignore registered domains */
	tb_dbg = tb_dbg_get_by_minor(tb->index);
	if (tb_dbg)
		return;

	tb_dbg = tb_dbg_get_free(tb);
	if (IS_ERR(tb_dbg))
		return;

	cdev_init(&tb_dbg->cdev, &tb_dbg_fops);
	tb_dbg->cdev.owner = THIS_MODULE;
	ret = cdev_add(&tb_dbg->cdev, TB_DBG_MKDEV(tb), 1);
	if (ret) {
		tb_err(tb, "tb_dbg: failed to add cdev for minor %d", tb->index);
		goto error_cdev;
	}

	tb_dbg->dev = device_create(tb_dbg_class, &tb->dev, TB_DBG_MKDEV(tb),
				    NULL, "tb_dbg%d", tb->index);
	if (IS_ERR(tb_dbg->dev)) {
		tb_err(tb, "tb_dbg: failed to create device for minor %d",
			tb->index);
		goto error_create;
	}

	tb_dbg(tb, "tb_dbg: successfully registered device for minor %d\n",
	       tb->index);
	return;

error_create:
	cdev_del(&tb_dbg->cdev);
error_cdev:
	put_tb_dbg(tb_dbg);
}

/**
 * tb_dbg_unregister_domain() - Unregister a char device for domain
 * @tb: Domain pointer
 *
 * Unregister char device for domain. Remove the corresponding file and
 * destroy any related structers.
 */
void tb_dbg_unregister_domain(struct tb *tb)
{
	struct tb_dbg *tb_dbg;

	if (WARN_ON(!tb))
		return;

	tb_dbg = tb_dbg_get_by_minor(tb->index);
	if (!tb_dbg)
		return;

	cdev_del(&tb_dbg->cdev);
	put_tb_dbg(tb_dbg);
	device_destroy(tb_dbg_class, TB_DBG_MKDEV(tb));

	tb_dbg(tb, "tb_dbg: successfully unregistered device for minor %d\n",
		tb->index);
}

void __init tb_dbg_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&tb_dbg_t, 0, TB_DBG_MAX_DEVICE, "tb_dbg");
	if (ret) {
		pr_err("unable to allocate major for tb_dbg device\n");
		return;
	}

	tb_dbg_class = class_create(THIS_MODULE, "tb_dbg");
	if (IS_ERR(tb_dbg_class)) {
		pr_err("failed to create class\n");
		unregister_chrdev_region(tb_dbg_t, TB_DBG_MAX_DEVICE);
		return;
	}

	pr_debug("initialize chrdev with major %d", TB_DBG_MAJOR);
}

void tb_dbg_exit(void)
{
	struct tb_dbg *tb_dbg;
	int index;

	/* Iterate all devices (if any) and unregister */
	list_for_each_entry(tb_dbg, &tb_dbg_list, list) {
		index = tb_dbg->tb->index;
		cdev_del(&tb_dbg->cdev);
		put_tb_dbg(tb_dbg);
		device_destroy(tb_dbg_class, TB_DBG_MKDEV(tb_dbg->tb));
	}
	class_destroy(tb_dbg_class);
	unregister_chrdev_region(tb_dbg_t, TB_DBG_MAX_DEVICE);
}
