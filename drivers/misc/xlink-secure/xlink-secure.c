// SPDX-License-Identifier: GPL-2.0-only
/*
 * Xlink Secure Driver.
 *
 * Copyright (C) 2020 Intel Corporation
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include "xlink-secure-defs.h"

// xlink secure version number
#define XLINK_VERSION_MAJOR	0
#define XLINK_VERSION_MINOR	1

// device, class, and driver names
#define DEVICE_NAME "xlnksecure"
#define CLASS_NAME  "xlksecure"
#define DRV_NAME    "xlink-secure-driver"

static dev_t xdev;
static struct class *dev_class;
static struct cdev xlink_cdev;

static long xlink_secure_ioctl(struct file *file, unsigned int cmd,
                               unsigned long arg);
static enum xlink_error xlink_secure_write_data_user(struct xlink_handle *handle,
		               uint16_t chan, uint8_t const *pmessage, uint32_t size);

static const struct file_operations fops = {
		.owner			= THIS_MODULE,
		.unlocked_ioctl = xlink_secure_ioctl,
};

struct xlink_secure_dev {
	struct platform_device *pdev;
};

/*
 * Global variable pointing to our Xlink Secure Device.
 *
 * This is meant to be used only when platform_get_drvdata() cannot be used
 * because we lack a reference to our platform_device.
 */
static struct xlink_secure_dev *xlink;

/* Driver probing. */
static int xlink_secure_probe(struct platform_device *pdev)
{
	struct xlink_secure_dev *xlink_dev;
	struct device *dev_ret;

	dev_info(&pdev->dev, "xlink-secure v%d.%d\n", XLINK_VERSION_MAJOR,
		 XLINK_VERSION_MINOR);
	xlink_dev = devm_kzalloc(&pdev->dev, sizeof(*xlink), GFP_KERNEL);
	if (!xlink_dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, xlink_dev);

	/* Set the global reference to our device. */
	xlink = xlink_dev;

	/*Allocating Major number*/
	if ((alloc_chrdev_region(&xdev, 0, 1, "xlinksecuredev")) < 0) {
		dev_info(&pdev->dev, "Cannot allocate major number\n");
		return -1;
	}
	dev_info(&pdev->dev, "Major = %d Minor = %d\n", MAJOR(xdev), MINOR(xdev));

	/*Creating struct class*/
	dev_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(dev_class)) {
		dev_info(&pdev->dev, "Cannot create the struct class - Err %ld\n",
				PTR_ERR(dev_class));
		goto r_class;
	}

	/*Creating device*/
	dev_ret = device_create(dev_class, NULL, xdev, NULL, DEVICE_NAME);
	if (IS_ERR(dev_ret)) {
		dev_info(&pdev->dev, "Cannot create the Device 1 - Err %ld\n",
				PTR_ERR(dev_ret));
		goto r_device;
	}
	dev_info(&pdev->dev, "Xlink-Secure Device Driver Insert...Done!!!\n");

	/*Creating cdev structure*/
	cdev_init(&xlink_cdev, &fops);

	/*Adding character device to the system*/
	if ((cdev_add(&xlink_cdev, xdev, 1)) < 0) {
		dev_info(&pdev->dev, "Cannot add the device to the system\n");
		goto r_class;
	}
	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(xdev, 1);
	return -1;
}

/* Driver removal. */
static int xlink_secure_remove(struct platform_device *pdev)
{
	// unregister and destroy device
	unregister_chrdev_region(xdev, 1);
	device_destroy(dev_class, xdev);
	cdev_del(&xlink_cdev);
	class_destroy(dev_class);
	printk(KERN_DEBUG "XLink Secure Driver removed\n");
	return 0;
}

/*
 * IOCTL function for User Space access to xlink secure kernel functions
 *
 */

static long xlink_secure_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	struct xlink_handle devH = {0};
	struct xlinksecureopenchannel op = {0};
	struct xlinksecurewritedata wr = {0};
	struct xlinksecurereaddata rd = {0};
	struct xlinksecurereadtobuffer rdtobuf = {0};
	struct xlinksecureconnect con = {0};
	struct xlinksecurerelease rel = {0};
	uint8_t *rdaddr = NULL;
	uint32_t size;
	uint8_t reladdr;
	uint8_t volbuf[XLINK_MAX_BUF_SIZE];

	switch (cmd) {
	case XL_SECURE_CONNECT:
		if (copy_from_user(&con, (int32_t *)arg, sizeof(struct xlinksecureconnect)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)con.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_secure_connect(&devH);
		if (!rc) {
			if (copy_to_user((struct xlink_handle *)con.handle, &devH,
					sizeof(struct xlink_handle)))
				return -EFAULT;
		}
		if (copy_to_user(con.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_SECURE_OPEN_CHANNEL:
		if (copy_from_user(&op, (int32_t *)arg,
				sizeof(struct xlinksecureopenchannel)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)op.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_secure_open_channel(&devH, op.chan, op.mode, op.data_size,
				op.timeout);
		if (copy_to_user(op.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_SECURE_READ_DATA:
		if (copy_from_user(&rd, (int32_t *)arg, sizeof(struct xlinksecurereaddata)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)rd.handle,
					sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_secure_read_data(&devH, rd.chan, &rdaddr, &size);
		if (!rc) {
			if (copy_to_user(rd.pmessage, (void *)rdaddr, size))
			        return -EFAULT;
			if (copy_to_user(rd.size, (void *)&size, sizeof(size)))
				return -EFAULT;
		}
		if (copy_to_user(rd.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_SECURE_READ_TO_BUFFER:
		if (copy_from_user(&rdtobuf, (int32_t *)arg,
				sizeof(struct xlinksecurereadtobuffer)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)rdtobuf.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_secure_read_data_to_buffer(&devH, rdtobuf.chan, (uint8_t *)volbuf,
				&size);
		if (!rc) {
			if (copy_to_user(rdtobuf.pmessage, (void *)volbuf, size))
				return -EFAULT;
			if (copy_to_user(rdtobuf.size, (void *)&size, sizeof(size)))
				return -EFAULT;
		}
		if (copy_to_user(rdtobuf.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_SECURE_WRITE_DATA:
		if (copy_from_user(&wr, (int32_t *)arg, sizeof(struct xlinksecurewritedata)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)wr.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		if (wr.size <= XLINK_MAX_DATA_SIZE) {
			rc = xlink_secure_write_data_user(&devH, wr.chan, wr.pmessage, wr.size);
			if (copy_to_user(wr.return_code, (void *)&rc, sizeof(rc)))
				return -EFAULT;
		} else {
			return -EFAULT;
		}
		break;
	case XL_SECURE_WRITE_VOLATILE:
		if (copy_from_user(&wr, (int32_t *)arg, sizeof(struct xlinksecurewritedata)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)wr.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		if (wr.size <= XLINK_MAX_BUF_SIZE) {
			if (copy_from_user(volbuf, (char *)wr.pmessage, wr.size))
				return -EFAULT;
			rc = xlink_secure_write_volatile(&devH, wr.chan, volbuf, wr.size);
			if (copy_to_user(wr.return_code, (void *)&rc, sizeof(rc)))
				return -EFAULT;
		} else {
			return -EFAULT;
		}
		break;
	case XL_SECURE_RELEASE_DATA:
		if (copy_from_user(&rel, (int32_t *)arg, sizeof(struct xlinksecurerelease)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)rel.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		if (rel.addr) {
			if (get_user(reladdr, (uint32_t *const)rel.addr))
				return -EFAULT;
			rc = xlink_secure_release_data(&devH, rel.chan, (uint8_t *)&reladdr);
		} else {
			rc = xlink_secure_release_data(&devH, rel.chan, NULL);
		}
		if (copy_to_user(rel.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_SECURE_CLOSE_CHANNEL:
		if (copy_from_user(&op, (int32_t *)arg,
				sizeof(struct xlinksecureopenchannel)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)op.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_secure_close_channel(&devH, op.chan);
		if (copy_to_user(op.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_SECURE_DISCONNECT:
		if (copy_from_user(&con, (int32_t *)arg, sizeof(struct xlinksecureconnect)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)con.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_secure_disconnect(&devH);
		if (copy_to_user(con.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	}
	if (rc)
		return -EIO;
	else
		return 0;
}

/*
 * Xlink Secure Kernel API.
 */

enum xlink_error xlink_secure_initialize(void)
{
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_secure_initialize);

enum xlink_error xlink_secure_connect(struct xlink_handle *handle)
{
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_secure_connect);

enum xlink_error xlink_secure_open_channel(struct xlink_handle *handle,
		uint16_t chan, enum xlink_opmode mode, uint32_t data_size,
		uint32_t timeout)
{
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_secure_open_channel);

enum xlink_error xlink_secure_close_channel(struct xlink_handle *handle,
		uint16_t chan)
{
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_secure_close_channel);

enum xlink_error xlink_secure_write_data(struct xlink_handle *handle,
		uint16_t chan, uint8_t const *pmessage, uint32_t size)
{
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_secure_write_data);

static enum xlink_error xlink_secure_write_data_user(struct xlink_handle *handle,
		uint16_t chan, uint8_t const *pmessage, uint32_t size)
{
	return X_LINK_SUCCESS;
}

enum xlink_error xlink_secure_write_volatile(struct xlink_handle *handle,
		uint16_t chan, uint8_t const *message, uint32_t size)
{
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_secure_write_volatile);

enum xlink_error xlink_secure_read_data(struct xlink_handle *handle,
		uint16_t chan, uint8_t **pmessage, uint32_t *size)
{
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_secure_read_data);

enum xlink_error xlink_secure_read_data_to_buffer(struct xlink_handle *handle,
		uint16_t chan, uint8_t * const message, uint32_t *size)
{
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_secure_read_data_to_buffer);

enum xlink_error xlink_secure_release_data(struct xlink_handle *handle,
		uint16_t chan, uint8_t * const data_addr)
{
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_secure_release_data);

enum xlink_error xlink_secure_disconnect(struct xlink_handle *handle)
{
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_secure_disconnect);

/* Device tree driver match. */
static const struct of_device_id xlink_secure_of_match[] = {
	{
		.compatible = "intel,xlink-secure",
	},
	{}
};

/* The xlink secure driver is a platform device. */
static struct platform_driver xlink_secure_driver = {
	.probe = xlink_secure_probe,
	.remove = xlink_secure_remove,
	.driver = {
			.name = DRV_NAME,
			.of_match_table = xlink_secure_of_match,
		},
};

/*
 * The remote host system will need to create an xlink secure platform
 * device for the platform driver to match with
 */
#ifndef CONFIG_XLINK_SECURE_LOCAL_HOST
static struct platform_device pdev;
void xlink_secure_release(struct device *dev) { return; }
#endif

static int xlink_secure_init(void)
{
	int rc = 0;

	rc = platform_driver_register(&xlink_secure_driver);
#ifndef CONFIG_XLINK_SECURE_LOCAL_HOST
	pdev.dev.release = xlink_secure_release;
	pdev.name = DRV_NAME;
	pdev.id = -1;
	if (!rc) {
		rc = platform_device_register(&pdev);
		if (rc) {
			platform_driver_unregister(&xlink_secure_driver);
		}
	}
#endif
	return rc;
}
module_init(xlink_secure_init);

static void xlink_secure_exit(void)
{
#ifndef CONFIG_XLINK_SECURE_LOCAL_HOST
	platform_device_unregister(&pdev);
#endif
	platform_driver_unregister(&xlink_secure_driver);
}
module_exit(xlink_secure_exit);

MODULE_DESCRIPTION("Xlink-Secure Kernel Driver");
MODULE_AUTHOR("Sagar Patil <sagarp.patil@intel.com>");
MODULE_LICENSE("GPL v2");
