/*
 * Copyright(c) 2013-2014 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fcntl.h>
#include <linux/genhd.h>
#include <linux/ndctl.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/nd.h>
#include "nd-private.h"
#include "nfit.h"
#include "nd.h"

static int nd_major;
static struct class *nd_class;

static int to_nd_device_type(struct device *dev)
{
	if (is_nd_dimm(dev))
		return ND_DEVICE_DIMM;
	else if (is_nd_pmem(dev))
		return ND_DEVICE_REGION_PMEM;
	else if (is_nd_blk(dev))
		return ND_DEVICE_REGION_BLOCK;
	else if (is_nd_pmem(dev->parent) || is_nd_blk(dev->parent))
		return nd_region_to_namespace_type(to_nd_region(dev->parent));
	else if (is_nd_btt(dev))
		return ND_DEVICE_BTT;

	return 0;
}

static int nd_bus_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "MODALIAS=" ND_DEVICE_MODALIAS_FMT,
			to_nd_device_type(dev));
	return 0;
}

static int nd_bus_match(struct device *dev, struct device_driver *drv)
{
	struct nd_device_driver *nd_drv = to_nd_device_driver(drv);

	return test_bit(to_nd_device_type(dev), &nd_drv->type);
}

struct nd_defer_tracker {
	struct list_head list;
	struct device *dev;
};

static struct nd_defer_tracker *__find_deferred_tracker(struct nd_bus *nd_bus,
		struct device *dev)
{
	struct nd_defer_tracker *nd_defer;

	list_for_each_entry(nd_defer, &nd_bus->deferred, list)
		if (dev == nd_defer->dev)
			return nd_defer;
	return NULL;
}

static int add_deferred_tracker(struct nd_bus *nd_bus, struct device *dev)
{
	struct nd_defer_tracker *nd_defer;

	spin_lock(&nd_bus->deferred_lock);
	nd_defer = __find_deferred_tracker(nd_bus, dev);
	while (!nd_defer) {
		nd_defer = kmalloc(sizeof(*nd_defer), GFP_KERNEL);
		if (!nd_defer)
			break;
		INIT_LIST_HEAD(&nd_defer->list);
		nd_defer->dev = dev;
		list_add_tail(&nd_defer->list, &nd_bus->deferred);
		dev_dbg(dev, "add to defer queue\n");
	}
	spin_unlock(&nd_bus->deferred_lock);

	return nd_defer ? 0 : -ENOMEM;
}

static void del_deferred_tracker(struct nd_bus *nd_bus, struct device *dev)
{
	struct nd_defer_tracker *nd_defer;

	spin_lock(&nd_bus->deferred_lock);
	nd_defer = __find_deferred_tracker(nd_bus, dev);
	if (nd_defer) {
		list_del_init(&nd_defer->list);
		kfree(nd_defer);
		wake_up_all(&nd_bus->deferq);
		dev_dbg(dev, "del from defer queue\n");
	}
	spin_unlock(&nd_bus->deferred_lock);
}

static struct module *to_bus_provider(struct device *dev)
{
	/* pin bus providers while regions are enabled */
	if (is_nd_pmem(dev) || is_nd_blk(dev)) {
		struct nd_bus *nd_bus = walk_to_nd_bus(dev);

		return nd_bus->module;
	}
	return NULL;
}

static int nd_bus_probe(struct device *dev)
{
	struct nd_device_driver *nd_drv = to_nd_device_driver(dev->driver);
	struct module *provider = to_bus_provider(dev);
	struct nd_bus *nd_bus = walk_to_nd_bus(dev);
	int rc;

	if (!try_module_get(provider))
		return -ENXIO;

	rc = nd_drv->probe(dev);
	if (rc == -EPROBE_DEFER)
		rc = add_deferred_tracker(nd_bus, dev);
	else if (rc == 0)
		del_deferred_tracker(nd_bus, dev);
	dev_dbg(dev, "%pf returned: %d\n", nd_drv->probe, rc);

	/* check if our btt-seed has sprouted, and plant another */
	if (rc == 0 && is_nd_btt(dev) && dev == &nd_bus->nd_btt->dev) {
		const char *sep = "", *name = "", *status = "failed";

		nd_bus->nd_btt = nd_btt_create(nd_bus, NULL, NULL, 0, NULL);
		if (nd_bus->nd_btt) {
			status = "succeeded";
			sep = ": ";
			name = dev_name(&nd_bus->nd_btt->dev);
		}
		dev_dbg(&nd_bus->dev, "btt seed creation %s%s%s\n",
				status, sep, name);
	}

	if (rc != 0)
		module_put(provider);
	return rc;
}

static int nd_bus_remove(struct device *dev)
{
	struct nd_device_driver *nd_drv = to_nd_device_driver(dev->driver);
	struct module *provider = to_bus_provider(dev);
	int rc;

	rc = nd_drv->remove(dev);
	dev_dbg(dev, "%pf returned: %d\n", nd_drv->remove, rc);
	module_put(provider);
	return rc;
}

static struct bus_type nd_bus_type = {
	.name = "nd",
	.uevent = nd_bus_uevent,
	.match = nd_bus_match,
	.probe = nd_bus_probe,
	.remove = nd_bus_remove,
};

static ASYNC_DOMAIN_EXCLUSIVE(nd_async_register);

static void nd_async_device_register(void *d, async_cookie_t cookie)
{
	struct device *dev = d;

	if (device_add(dev) != 0)
		put_device(dev);
	put_device(dev);
}

static void nd_async_device_unregister(void *d, async_cookie_t cookie)
{
	struct device *dev = d;

	device_unregister(dev);
	put_device(dev);
}

int nd_device_register(struct device *dev, enum nd_async_mode mode)
{
	int rc;

	dev->bus = &nd_bus_type;
	switch (mode) {
	case ND_ASYNC:
		device_initialize(dev);
		get_device(dev);
		async_schedule_domain(nd_async_device_register, dev,
				&nd_async_register);
		return 0;
	case ND_SYNC:
		rc = device_register(dev);
		if (rc != 0)
			put_device(dev);
		return rc;
	default:
		return 0;
	}
}
EXPORT_SYMBOL(nd_device_register);

static bool is_deferred_queue_empty(struct nd_bus *nd_bus)
{
	/* flush the wake up path */
	spin_lock(&nd_bus->deferred_lock);
	spin_unlock(&nd_bus->deferred_lock);
	return list_empty(&nd_bus->deferred);
}

void nd_bus_wait_probe(struct nd_bus *nd_bus)
{
	wait_for_completion(&nd_bus->registration);
	async_synchronize_full_domain(&nd_async_register);
	wait_event(nd_bus->deferq, is_deferred_queue_empty(nd_bus));
}

void nd_device_unregister(struct device *dev, enum nd_async_mode mode)
{
	struct nd_bus *nd_bus = walk_to_nd_bus(dev);

	switch (mode) {
	case ND_ASYNC:
		get_device(dev);
		async_schedule_domain(nd_async_device_unregister, dev,
				&nd_async_register);
		break;
	case ND_SYNC:
		nd_bus_wait_probe(nd_bus);
		device_unregister(dev);
		break;
	}
}
EXPORT_SYMBOL(nd_device_unregister);

/**
 * __nd_driver_register() - register a region or a namespace driver
 * @nd_drv: driver to register
 * @owner: automatically set by nd_driver_register() macro
 * @mod_name: automatically set by nd_driver_register() macro
 */
int __nd_driver_register(struct nd_device_driver *nd_drv, struct module *owner,
		const char *mod_name)
{
	struct device_driver *drv = &nd_drv->drv;

	if (!nd_drv->type) {
		pr_debug("driver type bitmask not set (%pf)\n",
				__builtin_return_address(0));
		return -EINVAL;
	}

	if (!nd_drv->probe || !nd_drv->remove) {
		pr_debug("->probe() and ->remove() must be specified\n");
		return -EINVAL;
	}

	drv->bus = &nd_bus_type;
	drv->owner = owner;
	drv->mod_name = mod_name;

	return driver_register(drv);
}
EXPORT_SYMBOL(__nd_driver_register);

/**
 * nd_register_ndio() - register byte-aligned access capability for an nd-bdev
 * @disk: child gendisk of the ndio namepace device
 * @ndio: initialized ndio instance to register
 *
 * LOCKING: hold nd_bus_lock() over the creation of ndio->disk and the
 * subsequent nd_region_ndio event
 */
int nd_register_ndio(struct nd_io *ndio)
{
	struct nd_bus *nd_bus;
	struct device *dev;

	if (!ndio || !ndio->dev || !ndio->disk || !list_empty(&ndio->list)
			|| !ndio->rw_bytes || !list_empty(&ndio->claims)) {
		pr_debug("%s bad parameters from %pf\n", __func__,
				__builtin_return_address(0));
		return -EINVAL;
	}

	dev = ndio->dev;
	nd_bus = walk_to_nd_bus(dev);
	if (!nd_bus)
		return -EINVAL;

	mutex_lock(&nd_bus_list_mutex);
	list_add(&ndio->list, &nd_bus->ndios);
	mutex_unlock(&nd_bus_list_mutex);

	/* TODO: Autodetect BTT */

	return 0;
}
EXPORT_SYMBOL(nd_register_ndio);

/**
 * __nd_unregister_ndio() - try to remove an ndio interface
 * @ndio: interface to remove
 */
static int __nd_unregister_ndio(struct nd_io *ndio)
{
	struct nd_io_claim *ndio_claim, *_n;
	struct nd_bus *nd_bus;
	LIST_HEAD(claims);

	nd_bus = walk_to_nd_bus(ndio->dev);
	if (!nd_bus || list_empty(&ndio->list))
		return -ENXIO;

	spin_lock(&ndio->lock);
	list_splice_init(&ndio->claims, &claims);
	spin_unlock(&ndio->lock);

	list_for_each_entry_safe(ndio_claim, _n, &claims, list)
		ndio_claim->notify_remove(ndio_claim);

	list_del_init(&ndio->list);

	return 0;
}

int nd_unregister_ndio(struct nd_io *ndio)
{
	struct nd_bus *nd_bus = walk_to_nd_bus(ndio->dev);
	struct device *dev = ndio->dev;
	int rc;

	nd_bus_lock(dev);
	mutex_lock(&nd_bus_list_mutex);
	rc = __nd_unregister_ndio(ndio);
	mutex_unlock(&nd_bus_list_mutex);
	nd_bus_unlock(dev);

	/*
	 * Flush in case ->notify_remove() kicked off asynchronous device
	 * unregistration
	 */
	nd_bus_wait_probe(nd_bus);

	return rc;
}
EXPORT_SYMBOL(nd_unregister_ndio);

static struct nd_io *__ndio_lookup(struct nd_bus *nd_bus, const char *diskname)
{
	struct nd_io *ndio;

	list_for_each_entry(ndio, &nd_bus->ndios, list)
		if (strcmp(diskname, ndio->disk->disk_name) == 0)
			return ndio;

	return NULL;
}

struct nd_io *ndio_lookup(struct nd_bus *nd_bus, const char *diskname)
{
	struct nd_io *ndio;

	mutex_lock(&nd_bus_list_mutex);
	ndio = __ndio_lookup(nd_bus, diskname);
	mutex_unlock(&nd_bus_list_mutex);

	return ndio;
}

static const char *nfit_desc_provider(struct device *parent,
		struct nfit_bus_descriptor *nfit_desc)
{
	if (nfit_desc->provider_name)
		return nfit_desc->provider_name;
	else if (parent)
		return dev_name(parent);
	else
		return "unknown";
}

static ssize_t commands_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int cmd, len = 0;
	struct nd_bus *nd_bus = to_nd_bus(dev->parent);
	struct nfit_bus_descriptor *nfit_desc = nd_bus->nfit_desc;

	for_each_set_bit(cmd, &nfit_desc->dsm_mask, BITS_PER_LONG)
		len += sprintf(buf + len, "%s ", nfit_cmd_name(cmd));
	len += sprintf(buf + len, "\n");
	return len;
}
static DEVICE_ATTR_RO(commands);

static ssize_t provider_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_bus *nd_bus = to_nd_bus(dev->parent);

	return sprintf(buf, "%s\n", nfit_desc_provider(nd_bus->dev.parent,
				nd_bus->nfit_desc));
}
static DEVICE_ATTR_RO(provider);

static ssize_t revision_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct nd_bus *nd_bus = to_nd_bus(dev->parent);
	struct nfit __iomem *nfit = nd_bus->nfit_desc->nfit_base;

	return sprintf(buf, "%d\n", readb(&nfit->revision));
}
static DEVICE_ATTR_RO(revision);

static ssize_t wait_probe_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	nd_bus_wait_probe(to_nd_bus(dev->parent));
	return sprintf(buf, "1\n");
}
static DEVICE_ATTR_RO(wait_probe);

static struct attribute *nd_bus_attributes[] = {
	&dev_attr_commands.attr,
	&dev_attr_wait_probe.attr,
	&dev_attr_provider.attr,
	&dev_attr_revision.attr,
	NULL,
};

static struct attribute_group nd_bus_attribute_group = {
	.attrs = nd_bus_attributes,
};

static const struct attribute_group *nd_bus_attribute_groups[] = {
	&nd_bus_attribute_group,
	NULL,
};

static ssize_t modalias_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, ND_DEVICE_MODALIAS_FMT "\n",
			to_nd_device_type(dev));
}
static DEVICE_ATTR_RO(modalias);

static ssize_t devtype_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf, "%s\n", dev->type->name);
}
DEVICE_ATTR_RO(devtype);

static struct attribute *nd_device_attributes[] = {
	&dev_attr_modalias.attr,
	&dev_attr_devtype.attr,
	NULL,
};

/**
 * nd_device_attribute_group - generic attributes for all devices on an nd bus
 */
struct attribute_group nd_device_attribute_group = {
	.attrs = nd_device_attributes,
};

int nd_bus_create_ndctl(struct nd_bus *nd_bus)
{
	dev_t devt = MKDEV(nd_major, nd_bus->id);
	struct device *dev;

	dev = device_create_with_groups(nd_class, &nd_bus->dev, devt, nd_bus,
			nd_bus_attribute_groups, "ndctl%d", nd_bus->id);

	if (IS_ERR(dev)) {
		dev_dbg(&nd_bus->dev, "failed to register ndctl%d: %ld\n",
				nd_bus->id, PTR_ERR(dev));
		return PTR_ERR(dev);
	}
	return 0;
}

void nd_bus_destroy_ndctl(struct nd_bus *nd_bus)
{
	device_destroy(nd_class, MKDEV(nd_major, nd_bus->id));
}

static int __nd_ioctl(struct nd_bus *nd_bus, int read_only, unsigned int cmd,
		unsigned long arg)
{
	struct nfit_bus_descriptor *nfit_desc = nd_bus->nfit_desc;
	void __user *p = (void __user *) arg;
	size_t buf_len = 0;
	void *buf = NULL;
	int rc;

	/* check if the command is supported */
	if (!test_bit(_IOC_NR(cmd), &nfit_desc->dsm_mask))
		return -ENXIO;

	/* fail write commands (when read-only), or unknown commands */
	switch (cmd) {
	case NFIT_IOCTL_SMART:
	case NFIT_IOCTL_VENDOR:
	case NFIT_IOCTL_SET_CONFIG_DATA:
	case NFIT_IOCTL_ARS_START:
	case NFIT_IOCTL_ARM:
		if (read_only)
			return -EPERM;
		/* fallthrough */
	case NFIT_IOCTL_GET_CONFIG_SIZE:
	case NFIT_IOCTL_GET_CONFIG_DATA:
	case NFIT_IOCTL_ARS_CAP:
	case NFIT_IOCTL_ARS_QUERY:
	case NFIT_IOCTL_SMART_THRESHOLD:
		break;
	default:
		pr_debug("%s: unknown cmd: %d\n", __func__, _IOC_NR(cmd));
		return -ENOTTY;
	}

	/* validate input buffer / determine size */
	switch (cmd) {
	case NFIT_IOCTL_SMART:
		buf_len = sizeof(struct nfit_cmd_smart);
		break;
	case NFIT_IOCTL_VENDOR: {
		struct nfit_cmd_vendor_hdr nfit_cmd_v;
		struct nfit_cmd_vendor_tail nfit_cmd_vt;

		if (!access_ok(VERIFY_WRITE, p, sizeof(nfit_cmd_v)))
			return -EFAULT;
		if (copy_from_user(&nfit_cmd_v, p, sizeof(nfit_cmd_v)))
			return -EFAULT;
		buf_len = sizeof(nfit_cmd_v) + nfit_cmd_v.in_length;
		if (!access_ok(VERIFY_WRITE, p + buf_len, sizeof(nfit_cmd_vt)))
			return -EFAULT;
		if (copy_from_user(&nfit_cmd_vt, p + buf_len,
					sizeof(nfit_cmd_vt)))
			return -EFAULT;
		buf_len += sizeof(nfit_cmd_vt) + nfit_cmd_vt.out_length;
		break;
	}
	case NFIT_IOCTL_SET_CONFIG_DATA: {
		struct nfit_cmd_set_config_hdr nfit_cmd_set;

		if (!access_ok(VERIFY_WRITE, p, sizeof(nfit_cmd_set)))
			return -EFAULT;
		if (copy_from_user(&nfit_cmd_set, p, sizeof(nfit_cmd_set)))
			return -EFAULT;
		/* include input buffer size and trailing status */
		buf_len = sizeof(nfit_cmd_set) + nfit_cmd_set.in_length + 4;
		break;
	}
	case NFIT_IOCTL_ARS_START:
		buf_len = sizeof(struct nfit_cmd_ars_start);
		break;
	case NFIT_IOCTL_ARM:
		buf_len = sizeof(struct nfit_cmd_arm);
		break;
	case NFIT_IOCTL_GET_CONFIG_SIZE:
		buf_len = sizeof(struct nfit_cmd_get_config_size);
		break;
	case NFIT_IOCTL_GET_CONFIG_DATA: {
		struct nfit_cmd_get_config_data_hdr nfit_cmd_get;

		if (!access_ok(VERIFY_WRITE, p, sizeof(nfit_cmd_get)))
			return -EFAULT;
		if (copy_from_user(&nfit_cmd_get, p, sizeof(nfit_cmd_get)))
			return -EFAULT;
		buf_len = sizeof(nfit_cmd_get) + nfit_cmd_get.in_length;
		break;
	}
	case NFIT_IOCTL_ARS_CAP:
		buf_len = sizeof(struct nfit_cmd_ars_cap);
		break;
	case NFIT_IOCTL_ARS_QUERY: {
		struct nfit_cmd_ars_query nfit_cmd_query;

		if (!access_ok(VERIFY_WRITE, p, sizeof(nfit_cmd_query)))
			return -EFAULT;
		if (copy_from_user(&nfit_cmd_query, p, sizeof(nfit_cmd_query)))
			return -EFAULT;
		buf_len = sizeof(nfit_cmd_query) + nfit_cmd_query.out_length
			- offsetof(struct nfit_cmd_ars_query, out_length);
		break;
	}
	case NFIT_IOCTL_SMART_THRESHOLD:
		buf_len = sizeof(struct nfit_cmd_smart_threshold);
		break;
	default:
		pr_debug("%s: unknown cmd: %d\n", __func__, _IOC_NR(cmd));
		return -ENOTTY;
	}

	if (!access_ok(VERIFY_WRITE, p, sizeof(buf_len)))
		return -EFAULT;

	if (buf_len > ND_IOCTL_MAX_BUFLEN) {
		pr_debug("%s: buf_len: %zd > %d\n",
				__func__, buf_len, ND_IOCTL_MAX_BUFLEN);
		return -EINVAL;
	}

	if (buf_len < KMALLOC_MAX_SIZE)
		buf = kmalloc(buf_len, GFP_KERNEL);

	if (!buf)
		buf = vmalloc(buf_len);

	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, p, buf_len)) {
		rc = -EFAULT;
		goto out;
	}

	rc = nfit_desc->nfit_ctl(nfit_desc, _IOC_NR(cmd), buf, buf_len);
	if (rc)
		goto out;
	if (copy_to_user(p, buf, buf_len))
		rc = -EFAULT;
 out:
	if (is_vmalloc_addr(buf))
		vfree(buf);
	else
		kfree(buf);
	return rc;
}

static long nd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long id = (long) file->private_data;
	int rc = -ENXIO, read_only;
	struct nd_bus *nd_bus;

	read_only = (O_RDWR != (file->f_flags & O_ACCMODE));
	mutex_lock(&nd_bus_list_mutex);
	list_for_each_entry(nd_bus, &nd_bus_list, list) {
		if (nd_bus->id == id) {
			rc = __nd_ioctl(nd_bus, read_only, cmd, arg);
			break;
		}
	}
	mutex_unlock(&nd_bus_list_mutex);

	return rc;
}

static int nd_open(struct inode *inode, struct file *file)
{
	long minor = iminor(inode);

	file->private_data = (void *) minor;
	return 0;
}

static const struct file_operations nd_bus_fops = {
	.owner = THIS_MODULE,
	.open = nd_open,
	.unlocked_ioctl = nd_ioctl,
	.compat_ioctl = nd_ioctl,
	.llseek = noop_llseek,
};

int __init nd_bus_init(void)
{
	int rc;

	rc = bus_register(&nd_bus_type);
	if (rc)
		return rc;

	rc = register_chrdev(0, "ndctl", &nd_bus_fops);
	if (rc < 0)
		goto err_chrdev;
	nd_major = rc;

	nd_class = class_create(THIS_MODULE, "nd_bus");
	if (IS_ERR(nd_class))
		goto err_class;

	return 0;

 err_class:
	unregister_chrdev(nd_major, "ndctl");
 err_chrdev:
	bus_unregister(&nd_bus_type);

	return rc;
}

void nd_bus_exit(void)
{
	class_destroy(nd_class);
	unregister_chrdev(nd_major, "ndctl");
	bus_unregister(&nd_bus_type);
}
