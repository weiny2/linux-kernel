// SPDX-License-Identifier: GPL-2.0
/*
 * Thunderbolt/USB4 retimer support. Currently only supports USB4
 * compliant retimers.
 *
 * Copyright (C) 2020, Intel Corporation
 * Authors: Kranthi Kuntala <kranthi.kuntala@intel.com>
 *	    Mika Westerberg <mika.westerberg@linux.intel.com>
 */

#include "tb.h"

#define TB_MAX_RETIMER_INDEX	6

static int tb_retimer_nvm_add(struct tb_retimer *rt)
{
	int ret;
	u32 val;

	if (rt->vendor != PCI_VENDOR_ID_INTEL) {
		dev_info(&rt->dev,
			 "NVM format of vendor %#x is not known, disabling NVM upgrade\n",
			 rt->vendor);
		return 0;
	}

	ret = usb4_port_retimer_nvm_sector_size(rt->port, rt->index);
	if (ret < 0)
		return ret;

	dev_dbg(&rt->dev, "NVM sector size %d bytes\n", ret);

	ret = usb4_port_retimer_nvm_read(rt->port, rt->index, 0x08, &val,
					 sizeof(val));
	if (ret)
		return ret;

	rt->nvm_major = val >> 16;
	rt->nvm_minor = val >> 8;

	dev_dbg(&rt->dev, "NVM version %x.%x\n", rt->nvm_major, rt->nvm_minor);

	return 0;
}

static ssize_t device_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct tb_retimer *rt = tb_to_retimer(dev);

	return sprintf(buf, "%#x\n", rt->device);
}
static DEVICE_ATTR_RO(device);

static ssize_t nvm_version_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct tb_retimer *rt = tb_to_retimer(dev);

	/*
	 * TODO: This is temporary. We will be reusing the tb_switch NVM
	 *	 operations here once they all work as expected.
	 */
	return sprintf(buf, "%x.%x\n", rt->nvm_major, rt->nvm_minor);
}
static DEVICE_ATTR_RO(nvm_version);

static ssize_t vendor_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct tb_retimer *rt = tb_to_retimer(dev);

	return sprintf(buf, "%#x\n", rt->vendor);
}
static DEVICE_ATTR_RO(vendor);

static struct attribute *retimer_attrs[] = {
	&dev_attr_device.attr,
	&dev_attr_nvm_version.attr,
	&dev_attr_vendor.attr,
	NULL
};

static const struct attribute_group retimer_group = {
	.attrs = retimer_attrs,
};

static const struct attribute_group *retimer_groups[] = {
	&retimer_group,
	NULL
};

static void tb_retimer_release(struct device *dev)
{
	struct tb_retimer *rt = tb_to_retimer(dev);

	kfree(rt);
}

struct device_type tb_retimer_type = {
	.name = "thunderbolt_retimer",
	.groups = retimer_groups,
	.release = tb_retimer_release,
};

static struct tb_retimer *tb_retimer_add(struct tb_port *port, u8 index)
{
	struct usb4_port *usb4 = port->usb4;
	struct tb_retimer *rt;
	u32 vendor, device;
	int ret;

	if (!usb4)
		return ERR_PTR(-EINVAL);

	ret = usb4_port_retimer_read(port, index, USB4_SB_VENDOR_ID, &vendor,
				     sizeof(vendor));
	if (ret) {
		if (ret != -ENODEV)
			tb_port_warn(port, "failed read retimer VendorId: %d\n", ret);
		return ERR_PTR(ret);
	}

	ret = usb4_port_retimer_read(port, index, USB4_SB_PRODUCT_ID, &device,
				     sizeof(device));
	if (ret) {
		if (ret != -ENODEV)
			tb_port_warn(port, "failed read retimer ProductId: %d\n", ret);
		return ERR_PTR(ret);
	}

	rt = kzalloc(sizeof(*rt), GFP_KERNEL);
	if (!rt)
		return ERR_PTR(-ENOMEM);

	rt->index = index;
	rt->vendor = vendor;
	rt->device = device;
	rt->port = port;

	rt->dev.parent = &usb4->dev;
	rt->dev.bus = &tb_bus_type;
	rt->dev.type = &tb_retimer_type;
	dev_set_name(&rt->dev, "%s:%u.%u", dev_name(&port->sw->dev),
		     port->port, index);

	ret = device_register(&rt->dev);
	if (ret) {
		dev_err(&rt->dev, "failed to register retimer: %d\n", ret);
		put_device(&rt->dev);
		return ERR_PTR(ret);
	}

	ret = tb_retimer_nvm_add(rt);
	if (ret) {
		dev_err(&rt->dev, "failed to add NVM devices: %d\n", ret);
		device_del(&rt->dev);
		return ERR_PTR(ret);
	}

	dev_info(&rt->dev, "new retimer found, vendor=%#x device=%#x\n",
		rt->vendor, rt->device);

	return rt;
}

static void tb_retimer_remove(struct tb_retimer *rt)
{
	dev_info(&rt->dev, "retimer disconnected\n");
	device_unregister(&rt->dev);
}

static int retimer_match(struct device *dev, void *data)
{
	struct tb_retimer *rt = tb_to_retimer(dev);
	const u8 *index = data;

	return rt && rt->index == *index;
}

static struct tb_retimer *tb_port_find_retimer(struct tb_port *port, u8 index)
{
	struct device *dev;

	dev = device_find_child(&port->usb4->dev, &index, retimer_match);
	if (dev)
		return tb_to_retimer(dev);

	return NULL;
}

/**
 * tb_retimer_scan() - Scan for on-board retimers under port
 * @port: USB4 port to scan
 *
 * Tries to enumerate on-board retimers connected to @port. Found
 * retimers are registered as children of @port. Does not scan for cable
 * retimers for now.
 */
int tb_retimer_scan(struct tb_port *port)
{
	int i, last_idx = 0;

	if (!port->usb4)
		return 0;

	for (i = 1; i <= TB_MAX_RETIMER_INDEX; i++) {
		int ret;

		/*
		 * Last retimer is true only for the last on-board
		 * retimer (the one connected directly to the Type-C
		 * port).
		 */
		ret = usb4_port_retimer_is_last(port, i);
		if (ret > 0)
			last_idx = i;
		else if (ret < 0)
			break;
	}

	if (!last_idx)
		return 0;

	/* Add on-board retimers if they do not exist already */
	for (i = 1; i <= last_idx; i++) {
		struct tb_retimer *rt;

		rt = tb_port_find_retimer(port, i);
		if (rt) {
			put_device(&rt->dev);
		} else {
			rt = tb_retimer_add(port, i);
			if (IS_ERR(rt))
				return PTR_ERR(rt);
		}
	}

	return 0;
}

static int remove_retimer(struct device *dev, void *data)
{
	if (tb_is_retimer(dev))
		tb_retimer_remove(tb_to_retimer(dev));
	return 0;
}

/**
 * tb_retimer_remove_all() - Remove all retimers under port
 * @port: USB4 port whose retimers to remove
 *
 * This removes all previously added retimers under @port.
 */
void tb_retimer_remove_all(struct tb_port *port)
{
	if (port->usb4)
		device_for_each_child_reverse(&port->usb4->dev, NULL,
					      remove_retimer);
}
