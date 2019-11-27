// SPDX-License-Identifier: GPL-2.0-only
/*
 * xlink Core Driver.
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#ifdef CONFIG_XLINK_LOCAL_HOST
#include <linux/keembay-vpu-ipc.h>
#endif

#include "xlink-defs.h"
#include "xlink-multiplexer.h"
#include "xlink-dispatcher.h"
#include "xlink-platform.h"

#define XLINK_VERSION_MAJOR	0
#define XLINK_VERSION_MINOR	93

#define XLINK_VPU_WAIT_FOR_READY (3000000)

#define DEVICE_NAME "xlnk"
#define CLASS_NAME 	"xlkcore"

#define DRV_NAME	"xlink-driver"

#define XLINK_IPC_MAX_CHANNELS	1024

static dev_t xdev;
static struct class *dev_class;
static struct cdev xlink_cdev;


static long xlink_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static const struct file_operations fops = {
		.owner 			= THIS_MODULE,
		.unlocked_ioctl = xlink_ioctl,
};

struct keembay_xlink_dev {
	struct platform_device *pdev;
	struct xlink_global_handle gl_handle;
	struct xlink_handle links[XLINK_MAX_CONNECTIONS];
	int nmb_connected_links;
	struct mutex lock;
};

/*
 * Global variable pointing to our Xlink Device.
 *
 * This is meant to be used only when platform_get_drvdata() cannot be used
 * because we lack a reference to our platform_device.
 */
static struct keembay_xlink_dev *xlink;

struct xlink_event *xlink_create_event(enum xlink_error type,
		struct xlink_handle *handle, uint16_t chan,
		uint32_t size, uint32_t timeout)
{
	struct xlink_event *new_event = NULL;
	new_event = kzalloc(sizeof(*new_event), GFP_KERNEL);
	if (new_event == NULL)
	{
		printk(KERN_INFO "Failed to allocate memory for new event\n");
		return NULL;
	}
	new_event->handle = handle;
	new_event->header.magic = XLINK_EVENT_HEADER_MAGIC;
	new_event->header.id = XLINK_INVALID_EVENT_ID;
	new_event->header.type = type;
	new_event->header.chan = chan;
	new_event->header.size = size;
	new_event->header.timeout = timeout;

	return new_event;
}

void xlink_destroy_event(struct xlink_event *event)
{
	kfree(event);
}

static int get_next_link_id(void)
{
	int i = 0;
	if (xlink->nmb_connected_links == XLINK_MAX_CONNECTIONS)
		return -1;
	for (i = 0; i < XLINK_MAX_CONNECTIONS; i++)
		if (xlink->links[i].link_id == -1)
			break;
	return i;
}

static struct xlink_handle *get_link(int link_id)
{
	struct xlink_handle *link = NULL;
	mutex_lock(&xlink->lock);
	if (xlink->links[link_id].link_id == link_id) {
		link = &xlink->links[link_id];
	}
	mutex_unlock(&xlink->lock);
	return link;
}
static struct xlink_handle *get_link_by_device(enum xlink_dev_type dev_type)
{
	int i = 0;
	struct xlink_handle *link = NULL;
	mutex_lock(&xlink->lock);
	for(i = 0; i < xlink->nmb_connected_links; i++){
		if(xlink->links[i].dev_type == dev_type){
			link = &xlink->links[i];
			break;
		}
	}
	mutex_unlock(&xlink->lock);
	return link;
}

/* Driver probing. */
static int kmb_xlink_probe(struct platform_device *pdev)
{
	int rc, i;
	struct keembay_xlink_dev *xlink_dev;
	struct device *dev_ret;

	dev_info(&pdev->dev, "KeemBay xlink v%d.%d\n", XLINK_VERSION_MAJOR,
		 XLINK_VERSION_MINOR);

	xlink_dev = devm_kzalloc(&pdev->dev, sizeof(*xlink), GFP_KERNEL);
	if (!xlink_dev)
		return -ENOMEM;

	xlink_dev->pdev = pdev;

	rc = xlink_multiplexer_init(xlink_dev->pdev);
	if (rc != X_LINK_SUCCESS) {
		printk(KERN_DEBUG "Multiplexer initialization failed\n");
		goto r_multiplexer;
	}

	// initialize dispatcher
	rc = xlink_dispatcher_init(xlink_dev->pdev);
	if (rc != X_LINK_SUCCESS) {
		printk(KERN_DEBUG "Dispatcher initialization failed\n");
		goto r_dispatcher;
	}

	// initialize xlink data structure
	xlink_dev->nmb_connected_links = 0;
	mutex_init(&xlink_dev->lock);
	for (i = 0; i < XLINK_MAX_CONNECTIONS; i++) {
		xlink_dev->links[i].link_id = -1;
	}

	platform_set_drvdata(pdev, xlink_dev);

	/* Set the global reference to our device. */
	xlink = xlink_dev;

	/*Allocating Major number*/
	if ((alloc_chrdev_region(&xdev, 0, 1, "xlinkdev")) < 0) {
		dev_info(&pdev->dev, "Cannot allocate major number\n");
		goto r_dispatcher;
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
	dev_info(&pdev->dev, "Device Driver Insert...Done!!!\n");

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
r_dispatcher:
	xlink_dispatcher_destroy();
r_multiplexer:
	xlink_multiplexer_destroy();
	return -1;
}

/* Driver removal. */
static int kmb_xlink_remove(struct platform_device *pdev)
{
	int rc = 0;

	mutex_lock(&xlink->lock);
	// destroy multiplexer
	rc = xlink_multiplexer_destroy();
	if (rc != X_LINK_SUCCESS) {
		printk(KERN_DEBUG "Multiplexer destroy failed\n");
	}

	// stop dispatchers and destroy
	rc = xlink_dispatcher_destroy();
	if (rc != X_LINK_SUCCESS) {
		printk(KERN_DEBUG "Dispatcher destroy failed\n");
	}
	mutex_unlock(&xlink->lock);
	mutex_destroy(&xlink->lock);
	// unregister and destroy device
	unregister_chrdev_region(xdev, 1);
	device_destroy(dev_class, xdev);
	cdev_del(&xlink_cdev);
	class_destroy(dev_class);
	return 0;
}

/*
 * IOCTL function for User Space access to xlink kernel functions
 *
 */

static long xlink_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	struct xlink_handle devH = {0};
	struct xlinkopenchannel op = {0};
	struct xlinkwritedata wr = {0};
	struct xlinkreaddata rd = {0};
	struct xlinkreadtobuffer rdtobuf = {0};
	struct xlinkconnect con = {0};
	struct xlinkrelease rel = {0};
	struct xlinkstartvpu startvpu = {0};
	uint8_t *rdaddr;
	uint32_t size;
	uint8_t reladdr;
	uint8_t volbuf[XLINK_MAX_BUF_SIZE];
	char filename[64];

	switch (cmd) {
	case XL_CONNECT:
		if (copy_from_user(&con, (int32_t *)arg, sizeof(struct xlinkconnect)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)con.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_connect(&devH);
		if (!rc) {
			if (copy_to_user((struct xlink_handle *)con.handle, &devH,
					sizeof(struct xlink_handle)))
				return -EFAULT;
		}
		if (copy_to_user(con.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_OPEN_CHANNEL:
		if (copy_from_user(&op, (int32_t *)arg,
				sizeof(struct xlinkopenchannel)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)op.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_open_channel(&devH, op.chan, op.mode, op.data_size,
				op.timeout);
		if (copy_to_user(op.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_READ_DATA:
		if (copy_from_user(&rd, (int32_t *)arg, sizeof(struct xlinkreaddata)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)rd.handle,
					sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_read_data(&devH, rd.chan, &rdaddr, &size);
		if (!rc) {
			if(devH.dev_type == IPC_DEVICE) {
				if (copy_to_user(rd.pmessage, (void *)&rdaddr, sizeof(uint32_t)))
				return -EFAULT;
			} else {
				if (copy_to_user(rd.pmessage, (void *)rdaddr, size))
				return -EFAULT;
			}
			if (copy_to_user(rd.size, (void *)&size, sizeof(size)))
				return -EFAULT;
		}
		if (copy_to_user(rd.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_READ_TO_BUFFER:

		if (copy_from_user(&rdtobuf, (int32_t *)arg,
				sizeof(struct xlinkreadtobuffer)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)rdtobuf.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_read_data_to_buffer(&devH, rdtobuf.chan, (uint8_t *)volbuf,
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
	case XL_WRITE_DATA:
		if (copy_from_user(&wr, (int32_t *)arg, sizeof(struct xlinkwritedata)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)wr.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_write_data(&devH, wr.chan, wr.pmessage, wr.size);
		if (copy_to_user(wr.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_WRITE_VOLATILE:
		if (copy_from_user(&wr, (int32_t *)arg, sizeof(struct xlinkwritedata)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)wr.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		if (wr.size <= XLINK_MAX_BUF_SIZE) {
			if (copy_from_user(volbuf, (char *)wr.pmessage, wr.size))
				return -EFAULT;
			rc = xlink_write_volatile(&devH, wr.chan, volbuf, wr.size);
			if (copy_to_user(wr.return_code, (void *)&rc, sizeof(rc)))
				return -EFAULT;
		} else {
			return -EFAULT;
		}
		break;
	case XL_WRITE_CONTROL_DATA:
		if (copy_from_user(&wr, (int32_t *)arg, sizeof(struct xlinkwritedata)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)wr.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		if (wr.size <= MAX_CONTROL_DATA_SIZE) {
			if (copy_from_user(volbuf, (char *)wr.pmessage, wr.size))
				return -EFAULT;
			rc = xlink_write_control_data(&devH, wr.chan, volbuf, wr.size);
			if (copy_to_user(wr.return_code, (void *)&rc, sizeof(rc)))
				return -EFAULT;
		} else {
			return -EFAULT;
		}
		break;
	case XL_RELEASE_DATA:
		if (copy_from_user(&rel, (int32_t *)arg, sizeof(struct xlinkrelease)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)rel.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		if (rel.addr) {
			if (get_user(reladdr, (uint32_t *const)rel.addr))
				return -EFAULT;
			rc = xlink_release_data(&devH, rel.chan, (uint8_t *)&reladdr);
		} else {
			rc = xlink_release_data(&devH, rel.chan, NULL);
		}
		if (copy_to_user(rel.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_CLOSE_CHANNEL:
		if (copy_from_user(&op, (int32_t *)arg,
				sizeof(struct xlinkopenchannel)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)op.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_close_channel(&devH, op.chan);
		if (copy_to_user(op.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_START_VPU:
		if (copy_from_user(&startvpu, (int32_t *)arg,
				sizeof(struct xlinkstartvpu)))
			return -EFAULT;
		if (startvpu.namesize > sizeof(filename))
			return -EINVAL;
		memset(filename, 0, sizeof(filename));
		if (copy_from_user(filename, startvpu.filename, startvpu.namesize))
			return -EFAULT;
		rc = xlink_start_vpu(filename);
		if (copy_to_user(startvpu.return_code, (void *)&rc, sizeof(rc)))
			return -EFAULT;
		break;
	case XL_STOP_VPU:
		rc = xlink_stop_vpu();
		break;
	case XL_RESET_VPU:
		rc = xlink_stop_vpu();
		break;
	case XL_DISCONNECT:
		if (copy_from_user(&con, (int32_t *)arg, sizeof(struct xlinkconnect)))
			return -EFAULT;
		if (copy_from_user(&devH, (struct xlink_handle *)con.handle,
				sizeof(struct xlink_handle)))
			return -EFAULT;
		rc = xlink_disconnect(&devH);
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
 * xlink Kernel API.
 */

enum xlink_error xlink_stop_vpu(void)
{
#ifdef CONFIG_XLINK_LOCAL_HOST
	int rc = 0;
	enum intel_keembay_vpu_state state;

	/* Stop the VPU */
	rc = intel_keembay_vpu_stop();
	if (rc) {
		pr_err("Failed to stop VPU: %d\n", rc);
		return X_LINK_ERROR;
	}
	pr_info("Successfully stopped VPU!\n");

	/* Check state */
	state = intel_keembay_vpu_status();
	if (state != KEEMBAY_VPU_OFF) {
		pr_err("VPU was not OFF after stop request, it was %d\n", state);
		return X_LINK_ERROR;
	}
#endif
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_stop_vpu);

enum xlink_error xlink_start_vpu(char *filename)
{
#ifdef CONFIG_XLINK_LOCAL_HOST
	int rc = 0;
	enum intel_keembay_vpu_state state;

	pr_info("\nStart VPU - %s\n", filename);
	rc = intel_keembay_vpu_startup(filename);
	if (rc) {
		pr_err("Failed to start VPU: %d\n", rc);
		return X_LINK_ERROR;
	}
	pr_info("Successfully started VPU!\n");

	/* Wait for VPU to be READY */
	rc = intel_keembay_vpu_wait_for_ready(
			XLINK_VPU_WAIT_FOR_READY);
	if (rc) {
		pr_err("Tried to start VPU but never got READY.\n");
		return X_LINK_ERROR;
	}
	pr_info("Successfully synchronised state with VPU!\n");

	/* Check state */
	state = intel_keembay_vpu_status();
	if (state != KEEMBAY_VPU_READY) {
		pr_err("VPU was not ready, it was %d\n", state);
		return X_LINK_ERROR;
	}
	pr_info("VPU was ready.\n");
#endif
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_start_vpu);

enum xlink_error xlink_initialize(struct xlink_global_handle *gl_handle)
{
	return X_LINK_SUCCESS;
}
EXPORT_SYMBOL(xlink_initialize);

enum xlink_error xlink_connect(struct xlink_handle *handle)
{
	enum xlink_error rc = 0;
	struct xlink_handle *link = NULL;
	int link_id = -1;

	if ((xlink == NULL) || (handle == NULL)) {
		return X_LINK_ERROR;
	}
	link = get_link_by_device(handle->dev_type);
	mutex_lock(&xlink->lock);
	if (link == NULL) {
		link_id = get_next_link_id();
		if (link_id < 0) {
			printk(KERN_DEBUG "Maximum number of connections reached\n");
			mutex_unlock(&xlink->lock);
			return X_LINK_ERROR;
		}
		// platform specific connect
		rc = xlink_platform_connect(handle->dev_type,
				handle->dev_path, handle->dev_path2,
				&handle->fd);
		if (rc) {
			printk(KERN_DEBUG "Platform connect failed\n");
			mutex_unlock(&xlink->lock);
			return X_LINK_ERROR;
		}
		// set link handler reference and link id
		handle->link_id = link_id;
		xlink->links[link_id] = *handle;
		xlink->nmb_connected_links++;
		if (handle->dev_type != IPC_DEVICE) {
			// start dispatcher
			rc = xlink_dispatcher_start(&xlink->links[link_id]);
			if (rc) {
				printk(KERN_DEBUG "Dispatcher start failed\n");
				rc = X_LINK_ERROR;
			}
		}
	} else {
		// already connected
		*handle = *link;
	}
	mutex_unlock(&xlink->lock);
	// TODO: implement ping
	return rc;
}
EXPORT_SYMBOL(xlink_connect);

enum xlink_error xlink_open_channel(struct xlink_handle *handle,
		uint16_t chan, enum xlink_opmode mode, uint32_t data_size,
		uint32_t timeout)
{
	enum xlink_error rc = 0;
	struct xlink_handle *link = NULL;
	struct xlink_event *event = NULL;
	int event_queued = 0;

	if ((xlink == NULL) || (handle == NULL)) {
		return X_LINK_ERROR;
	}
	link = get_link(handle->link_id);
	if (link == NULL) {
		return X_LINK_ERROR;
	}
	event = xlink_create_event(XLINK_OPEN_CHANNEL_REQ, link, chan,
			data_size, timeout);
	event->data = &mode;
	rc = xlink_multiplexer_tx(event, &event_queued);
	if (!event_queued) {
		xlink_destroy_event(event);
	}
	return rc;
}
EXPORT_SYMBOL(xlink_open_channel);

enum xlink_error xlink_close_channel(struct xlink_handle *handle,
		uint16_t chan)
{
	enum xlink_error rc = 0;
	struct xlink_handle *link = NULL;
	struct xlink_event *event = NULL;
	int event_queued = 0;

	if ((xlink == NULL) || (handle == NULL)) {
		return X_LINK_ERROR;
	}
	link = get_link(handle->link_id);
	if (link == NULL) {
		return X_LINK_ERROR;
	}
	event = xlink_create_event(XLINK_CLOSE_CHANNEL_REQ, link,
			chan, 0, 0);
	rc = xlink_multiplexer_tx(event, &event_queued);
	if (!event_queued) {
		xlink_destroy_event(event);
	}
	return rc;
}
EXPORT_SYMBOL(xlink_close_channel);

enum xlink_error xlink_write_data(struct xlink_handle *handle,
		uint16_t chan, uint8_t const *pmessage, uint32_t size)
{
	enum xlink_error rc = 0;
	struct xlink_handle *link = NULL;
	struct xlink_event *event = NULL;
	int event_queued = 0;
	dma_addr_t paddr;
	uint32_t addr;

	if ((xlink == NULL) || (handle == NULL)) {
		return X_LINK_ERROR;
	}
	link = get_link(handle->link_id);
	if (link == NULL) {
		return X_LINK_ERROR;
	}
	event = xlink_create_event(XLINK_WRITE_REQ, link, chan, size, 0);
	if(chan < XLINK_IPC_MAX_CHANNELS) {
		/* only passing message address across IPC interface */
		if(get_user(addr, (uint32_t *)pmessage)){
			return X_LINK_ERROR;
		}
		event->data = &addr;
		rc = xlink_multiplexer_tx(event, &event_queued);
		xlink_destroy_event(event);
	} else {
		event->data = xlink_platform_allocate(&xlink->pdev->dev, &paddr, size,
				XLINK_PACKET_ALIGNMENT);
		if (!event->data) {
			xlink_destroy_event(event);
			return X_LINK_ERROR;
		}
		if (copy_from_user(event->data, (char *)pmessage, size)) {
			xlink_platform_deallocate(&xlink->pdev->dev, event->data, paddr, size,
				XLINK_PACKET_ALIGNMENT);
			return X_LINK_ERROR;
		}
		event->paddr = paddr;
		rc = xlink_multiplexer_tx(event, &event_queued);
		if (!event_queued) {
			xlink_platform_deallocate(&xlink->pdev->dev, event->data, paddr, size,
				XLINK_PACKET_ALIGNMENT);
			xlink_destroy_event(event);
		}
	}
	return rc;
}
EXPORT_SYMBOL(xlink_write_data);


enum xlink_error xlink_write_control_data(struct xlink_handle *handle,
		uint16_t chan, uint8_t const *pmessage, uint32_t size)
{
	enum xlink_error rc = 0;
	struct xlink_handle *link = NULL;
	struct xlink_event *event = NULL;
	int event_queued = 0;

	if(size > MAX_CONTROL_DATA_SIZE)
		return X_LINK_ERROR;   // XLink Paramater Error ??

	if ((xlink == NULL) || (handle == NULL)) {
		return X_LINK_ERROR;
	}
	link = get_link(handle->link_id);
	if (link == NULL) {
		return X_LINK_ERROR;
	}
	event = xlink_create_event(XLINK_WRITE_CONTROL_REQ, link, chan,
			size, 0);

	memcpy(event->header.control_data, pmessage, size);
	rc = xlink_multiplexer_tx(event, &event_queued);
	if (!event_queued) {
		xlink_destroy_event(event);
	}
	return rc;
}
EXPORT_SYMBOL(xlink_write_control_data);


enum xlink_error xlink_write_volatile(struct xlink_handle *handle,
		uint16_t chan, uint8_t const *message, uint32_t size)
{
	enum xlink_error rc = 0;
	struct xlink_handle *link = NULL;
	struct xlink_event *event = NULL;
	int event_queued = 0;
	dma_addr_t paddr;

	if ((xlink == NULL) || (handle == NULL)) {
		return X_LINK_ERROR;
	}
	link = get_link(handle->link_id);
	if (link == NULL) {
		return X_LINK_ERROR;
	}
	event = xlink_create_event(XLINK_WRITE_VOLATILE_REQ, link, chan, size, 0);
	event->data = xlink_platform_allocate(&xlink->pdev->dev, &paddr, size,
			XLINK_PACKET_ALIGNMENT);
	if (!event->data) {
		xlink_destroy_event(event);
		return X_LINK_ERROR;
	} else {
		memcpy(event->data, message, size);
	}
	event->paddr = paddr;
	rc = xlink_multiplexer_tx(event, &event_queued);
	if (!event_queued) {
		xlink_platform_deallocate(&xlink->pdev->dev, event->data, paddr, size,
			XLINK_PACKET_ALIGNMENT);
		xlink_destroy_event(event);
	}
	return rc;
}
EXPORT_SYMBOL(xlink_write_volatile);

enum xlink_error xlink_write_data_crc(struct xlink_handle *handle,
		uint16_t chan, const uint8_t *message,
		uint32_t size)
{
	enum xlink_error rc = 0;
	/* To be implemented */
	return rc;
}
EXPORT_SYMBOL(xlink_write_data_crc);

enum xlink_error xlink_read_data(struct xlink_handle *handle,
		uint16_t chan, uint8_t **pmessage, uint32_t *size)
{
	enum xlink_error rc = 0;
	struct xlink_handle *link = NULL;
	struct xlink_event *event = NULL;
	int event_queued = 0;

	if ((xlink == NULL) || (handle == NULL)) {
		return X_LINK_ERROR;
	}
	link = get_link(handle->link_id);
	if (link == NULL) {
		return X_LINK_ERROR;
	}
	event = xlink_create_event(XLINK_READ_REQ, link, chan, *size, 0);
	event->pdata = (void **)pmessage;
	event->length = size;
	rc = xlink_multiplexer_tx(event, &event_queued);
	if (!event_queued) {
		xlink_destroy_event(event);
	}
	return rc;
}
EXPORT_SYMBOL(xlink_read_data);

enum xlink_error xlink_read_data_to_buffer(struct xlink_handle *handle,
		uint16_t chan, uint8_t * const message, uint32_t *size)
{
	enum xlink_error rc = 0;
	struct xlink_handle *link = NULL;
	struct xlink_event *event = NULL;
	int event_queued = 0;

	if ((xlink == NULL) || (handle == NULL)) {
		return X_LINK_ERROR;
	}
	link = get_link(handle->link_id);
	if (link == NULL) {
		return X_LINK_ERROR;
	}
	event = xlink_create_event(XLINK_READ_TO_BUFFER_REQ, link, chan,
			*size, 0);
	event->data = message;
	event->length = size;
	rc = xlink_multiplexer_tx(event, &event_queued);
	if (!event_queued) {
		xlink_destroy_event(event);
	}
	return rc;
}
EXPORT_SYMBOL(xlink_read_data_to_buffer);

enum xlink_error xlink_read_data_to_buffer_crc(struct xlink_handle *handle,
		uint16_t chan, uint8_t * const message, uint32_t *size)
{
	enum xlink_error rc = 0;
	/* To be implemented */
	return rc;
}
EXPORT_SYMBOL(xlink_read_data_to_buffer_crc);

enum xlink_error xlink_release_data(struct xlink_handle *handle,
		uint16_t chan, uint8_t * const data_addr)
{
	enum xlink_error rc = 0;
	struct xlink_handle *link = NULL;
	struct xlink_event *event = NULL;
	int event_queued = 0;

	if ((xlink == NULL) || (handle == NULL)) {
		return X_LINK_ERROR;
	}
	link = get_link(handle->link_id);
	if (link == NULL) {
		return X_LINK_ERROR;
	}
	event = xlink_create_event(XLINK_RELEASE_REQ, link, chan, 0, 0);
	event->data = data_addr;
	rc = xlink_multiplexer_tx(event, &event_queued);
	if (!event_queued) {
		xlink_destroy_event(event);
	}
	return rc;
}
EXPORT_SYMBOL(xlink_release_data);

uint32_t xlink_get_number_of_devices(void)
{
	/* To be implemented - needed ??*/
	return 0;
}

enum xlink_error xlink_boot_remote(struct xlink_handle *handle)
{
	enum xlink_error rc = 0;
	/* To be implemented */
	return rc;
}

enum xlink_error xlink_reset_remote(struct xlink_handle *handle)
{
	enum xlink_error rc = 0;
	struct xlink_handle *link = NULL;

	if ((xlink == NULL) || (handle == NULL)) {
		return X_LINK_ERROR;
	}
	link = get_link(handle->link_id);
	if (link == NULL) {
		return X_LINK_ERROR;
	}
	rc = xlink_platform_reset_remote(link->dev_type, link->fd, NULL);
	return rc;
}

enum xlink_error xlink_disconnect(struct xlink_handle *handle)
{
	enum xlink_error rc = 0;
	struct xlink_handle *link = NULL;

	if ((xlink == NULL) || (handle == NULL)) {
		return X_LINK_ERROR;
	}
	link = get_link(handle->link_id);
	if (link == NULL) {
		return X_LINK_ERROR;
	}
	// stop dispatcher
	if (link->dev_type != IPC_DEVICE) {
		rc = xlink_dispatcher_stop(link->link_id);
		if (rc != X_LINK_SUCCESS) {
			printk(KERN_DEBUG "Dispatcher stop failed\n");
			return X_LINK_ERROR;
		}
	}
	// reset remote
	xlink_reset_remote(link);
	// set link handler reference
	mutex_lock(&xlink->lock);
	xlink->links[link->link_id].link_id = -1;
	xlink->nmb_connected_links--;
	mutex_unlock(&xlink->lock);
	return rc;
}
EXPORT_SYMBOL(xlink_disconnect);

enum xlink_error xlink_reset_all(void)
{
	enum xlink_error rc = 0;
	/* To be implemented */
	return rc;
}

enum xlink_error xlink_prof_start(void)
{
	enum xlink_error rc = 0;
	/* To be implemented */
	return rc;
}

enum xlink_error xlink_prof_stop(void)
{
	enum xlink_error rc = 0;
	/* To be implemented */
	return rc;
}

/* Device tree driver match. */
static const struct of_device_id kmb_xlink_of_match[] = {
	{
		.compatible = "intel,keembay-xlink",
	},
	{}
};

/* The xlink driver is a platform device. */
static struct platform_driver kmb_xlink_driver = {
	.probe = kmb_xlink_probe,
	.remove = kmb_xlink_remove,
	.driver = {
			.name = DRV_NAME,
			.of_match_table = kmb_xlink_of_match,
		},
};

/*
 * The remote host system will need to create an xlink platform
 * device for the platform driver to match with
 */
#ifndef CONFIG_XLINK_LOCAL_HOST
static struct platform_device pdev;
void kmb_xlink_release(struct device *dev) { return; }
#endif

static int kmb_xlink_init(void)
{
	int rc = 0;
	rc = platform_driver_register(&kmb_xlink_driver);
#ifndef CONFIG_XLINK_LOCAL_HOST
	pdev.dev.release = kmb_xlink_release;
	pdev.name = DRV_NAME;
	pdev.id = -1;
	if (!rc) {
		rc = platform_device_register(&pdev);
		if (rc) {
			platform_driver_unregister(&kmb_xlink_driver);
		}
	}
#endif
	return rc;
}
module_init(kmb_xlink_init);

static void kmb_xlink_exit(void)
{
#ifndef CONFIG_XLINK_LOCAL_HOST
	platform_device_unregister(&pdev);
#endif
	platform_driver_unregister(&kmb_xlink_driver);
}
module_exit(kmb_xlink_exit);

MODULE_DESCRIPTION("KeemBay xlink Kernel Driver");
MODULE_AUTHOR("Seamus Kelly <seamus.kelly@intel.com>");
MODULE_LICENSE("GPL v2");
