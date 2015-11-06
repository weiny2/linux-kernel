/*  Xenbus code for blkif backend
    Copyright (C) 2005 Rusty Russell <rusty@rustcorp.com.au>
    Copyright (C) 2005 XenSource Ltd

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdarg.h>
#include <linux/kthread.h>
#include "common.h"
#include "../core/domctl.h"

#undef DPRINTK
#define DPRINTK(fmt, args...)				\
	pr_debug("blkback/xenbus (%s:%d) " fmt ".\n",	\
		 __FUNCTION__, __LINE__, ##args)

static void connect(struct backend_info *);
static int connect_ring(struct backend_info *);
static void backend_changed(struct xenbus_watch *, const char **,
			    unsigned int);

static char *blkback_name(const struct xenbus_device *dev)
{
	char *devpath, *devname;

	devpath = xenbus_read(XBT_NIL, dev->nodename, "dev", NULL);
	if (!IS_ERR(devpath) && (devname = strstr(devpath, "/dev/")) != NULL)
		return devname + strlen("/dev/");

	return devpath;
}

static void update_blkif_status(blkif_t *blkif)
{ 
	int err;
	char *devname;

	/* Not ready to connect? */
	if (!blkif->irq)
		return;

	/* Already connected? */
	if (blkif->be->dev->state == XenbusStateConnected)
		return;

	/* Attempt to connect: exit if we fail to. */
	connect(blkif->be);
	if (blkif->be->dev->state != XenbusStateConnected)
		return;

	if (blkif->vbd.bdev) {
		struct address_space *mapping
			= blkif->vbd.bdev->bd_inode->i_mapping;

		err = filemap_write_and_wait(mapping);
		if (err) {
			xenbus_dev_error(blkif->be->dev, err, "block flush");
			return;
		}
		invalidate_inode_pages2(mapping);
	}

	devname = blkback_name(blkif->be->dev);
	if (IS_ERR(devname)) {
		xenbus_dev_error(blkif->be->dev, PTR_ERR(devname),
				 "get blkback dev name");
		return;
	}

	blkif->xenblkd = kthread_run(blkif_schedule, blkif,
				     "blkbk.%d.%s", blkif->domid, devname);
	kfree(devname);
	if (IS_ERR(blkif->xenblkd)) {
		err = PTR_ERR(blkif->xenblkd);
		blkif->xenblkd = NULL;
		xenbus_dev_error(blkif->be->dev, err, "start xenblkd");
	}
}


/****************************************************************
 *  sysfs interface for VBD I/O requests
 */

#define VBD_SHOW(name, format, args...)					\
	static ssize_t show_##name(struct device *_dev,			\
				   struct device_attribute *attr,	\
				   char *buf)				\
	{								\
		ssize_t ret = -ENODEV;					\
		struct xenbus_device *dev;				\
		struct backend_info *be;				\
									\
		if (!get_device(_dev))					\
			return ret;					\
		dev = to_xenbus_device(_dev);				\
		if ((be = dev_get_drvdata(&dev->dev)) != NULL)		\
			ret = sprintf(buf, format, ##args);		\
		put_device(_dev);					\
		return ret;						\
	}								\
	static DEVICE_ATTR(name, S_IRUGO, show_##name, NULL)

VBD_SHOW(oo_req,  "%lu\n", be->blkif->st_oo_req);
VBD_SHOW(rd_req,  "%lu\n", be->blkif->st_rd_req);
VBD_SHOW(wr_req,  "%lu\n", be->blkif->st_wr_req);
VBD_SHOW(br_req,  "%lu\n", be->blkif->st_br_req);
VBD_SHOW(fl_req,  "%lu\n", be->blkif->st_fl_req);
VBD_SHOW(ds_req,  "%lu\n", be->blkif->st_ds_req);
VBD_SHOW(pk_req,  "%lu\n", be->blkif->st_pk_req);
VBD_SHOW(rd_sect, "%lu\n", be->blkif->st_rd_sect);
VBD_SHOW(wr_sect, "%lu\n", be->blkif->st_wr_sect);

static struct attribute *vbdstat_attrs[] = {
	&dev_attr_oo_req.attr,
	&dev_attr_rd_req.attr,
	&dev_attr_wr_req.attr,
	&dev_attr_br_req.attr,
	&dev_attr_fl_req.attr,
	&dev_attr_ds_req.attr,
	&dev_attr_pk_req.attr,
	&dev_attr_rd_sect.attr,
	&dev_attr_wr_sect.attr,
	NULL
};

static const struct attribute_group vbdstat_group = {
	.name = "statistics",
	.attrs = vbdstat_attrs,
};

VBD_SHOW(physical_device, "%x:%x\n", be->major, be->minor);
VBD_SHOW(mode, "%s\n", be->mode);

int xenvbd_sysfs_addif(struct xenbus_device *dev)
{
	int error;
	
	error = device_create_file(&dev->dev, &dev_attr_physical_device);
 	if (error)
		goto fail1;

	error = device_create_file(&dev->dev, &dev_attr_mode);
	if (error)
		goto fail2;

	error = sysfs_create_group(&dev->dev.kobj, &vbdstat_group);
	if (error)
		goto fail3;

	return 0;

fail3:	sysfs_remove_group(&dev->dev.kobj, &vbdstat_group);
fail2:	device_remove_file(&dev->dev, &dev_attr_mode);
fail1:	device_remove_file(&dev->dev, &dev_attr_physical_device);
	return error;
}

void xenvbd_sysfs_delif(struct xenbus_device *dev)
{
	sysfs_remove_group(&dev->dev.kobj, &vbdstat_group);
	device_remove_file(&dev->dev, &dev_attr_mode);
	device_remove_file(&dev->dev, &dev_attr_physical_device);
}

static int blkback_remove(struct xenbus_device *dev)
{
	struct backend_info *be = dev_get_drvdata(&dev->dev);

	DPRINTK("");

	if (be->major || be->minor)
		xenvbd_sysfs_delif(dev);

	if (be->backend_watch.node) {
		unregister_xenbus_watch(&be->backend_watch);
		kfree(be->backend_watch.node);
		be->backend_watch.node = NULL;
	}

	if (be->cdrom_watch.node) {
		unregister_xenbus_watch(&be->cdrom_watch);
		kfree(be->cdrom_watch.node);
		be->cdrom_watch.node = NULL;
	}

	if (be->blkif) {
		blkif_disconnect(be->blkif);
		vbd_free(&be->blkif->vbd);
		blkif_free(be->blkif);
		be->blkif = NULL;
	}

	kfree(be->mode);
	kfree(be);
	dev_set_drvdata(&dev->dev, NULL);
	return 0;
}

void blkback_barrier(struct xenbus_transaction xbt,
		     struct backend_info *be, int state)
{
	struct xenbus_device *dev = be->dev;
	int err = xenbus_printf(xbt, dev->nodename, "feature-barrier",
				"%d", state);

	if (err)
		xenbus_dev_error(dev, err, "writing feature-barrier");
}

void blkback_flush_diskcache(struct xenbus_transaction xbt,
			     struct backend_info *be, int state)
{
	struct xenbus_device *dev = be->dev;
	int err = xenbus_printf(xbt, dev->nodename, "feature-flush-cache",
				"%d", state);

	if (err)
		xenbus_dev_error(dev, err, "writing feature-flush-cache");
}

static void blkback_discard(struct xenbus_transaction xbt,
			    struct backend_info *be)
{
	struct xenbus_device *dev = be->dev;
	struct vbd *vbd = &be->blkif->vbd;
	struct request_queue *q = bdev_get_queue(vbd->bdev);
	int err, state = 0;

	if (blk_queue_discard(q)) {
		err = xenbus_printf(xbt, dev->nodename, "discard-granularity",
				    "%u", q->limits.discard_granularity);
		if (!err)
			state = 1;
		else
			xenbus_dev_error(dev, err,
					 "writing discard-granularity");
		err = xenbus_printf(xbt, dev->nodename, "discard-alignment",
				    "%u", q->limits.discard_alignment);
		if (err) {
			xenbus_dev_error(dev, err,
					 "writing discard-alignment");
			state = 0;
		}
	}

	/* Optional. */
	if (state) {
		err = xenbus_printf(xbt, dev->nodename, "discard-secure",
				    "%d", vbd->discard_secure);
		if (err)
			xenbus_dev_error(dev, err, "writing discard-secure");
	}

	err = xenbus_printf(xbt, dev->nodename, "feature-discard",
			    "%d", state);
	if (err)
		xenbus_dev_error(dev, err, "writing feature-discard");
}

/**
 * Entry point to this code when a new device is created.  Allocate the basic
 * structures, and watch the store waiting for the hotplug scripts to tell us
 * the device's physical major and minor numbers.  Switch to InitWait.
 */
static int blkback_probe(struct xenbus_device *dev,
			 const struct xenbus_device_id *id)
{
	int err;
	struct backend_info *be = kzalloc(sizeof(struct backend_info),
					  GFP_KERNEL);
	if (!be) {
		xenbus_dev_fatal(dev, -ENOMEM,
				 "allocating backend structure");
		return -ENOMEM;
	}
	be->dev = dev;
	dev_set_drvdata(&dev->dev, be);

	be->blkif = blkif_alloc(dev->otherend_id);
	if (IS_ERR(be->blkif)) {
		err = PTR_ERR(be->blkif);
		be->blkif = NULL;
		xenbus_dev_fatal(dev, err, "creating block interface");
		goto fail;
	}

	/* setup back pointer */
	be->blkif->be = be;

	err = xenbus_watch_path2(dev, dev->nodename, "physical-device",
				 &be->backend_watch, backend_changed);
	if (err)
		goto fail;

	err = xenbus_printf(XBT_NIL, dev->nodename, "max-ring-page-order",
			    "%u", blkif_max_ring_page_order);
	if (err)
		xenbus_dev_error(dev, err, "writing max-ring-page-order");

	if (blkif_max_segs_per_req > BLKIF_MAX_SEGMENTS_PER_REQUEST) {
		err = xenbus_printf(XBT_NIL, dev->nodename,
				    "feature-max-indirect-segments", "%u",
				    blkif_max_segs_per_req);
		if (err)
			xenbus_dev_error(dev, err,
					 "writing feature-max-indirect-segments");
	}

	err = xenbus_switch_state(dev, XenbusStateInitWait);
	if (err)
		goto fail;

	return 0;

fail:
	DPRINTK("failed");
	blkback_remove(dev);
	return err;
}


/**
 * Callback received when the hotplug scripts have placed the physical-device
 * node.  Read it and the mode node, and create a vbd.  If the frontend is
 * ready, connect.
 */
static void backend_changed(struct xenbus_watch *watch,
			    const char **vec, unsigned int len)
{
	int err;
	unsigned major;
	unsigned minor;
	struct backend_info *be
		= container_of(watch, struct backend_info, backend_watch);
	struct xenbus_device *dev = be->dev;
	int cdrom = 0;
	unsigned long handle;
	char *device_type;

	DPRINTK("");

	err = xenbus_scanf(XBT_NIL, dev->nodename, "physical-device", "%x:%x",
			   &major, &minor);
	if (XENBUS_EXIST_ERR(err)) {
		/* Since this watch will fire once immediately after it is
		   registered, we expect this.  Ignore it, and wait for the
		   hotplug scripts. */
		return;
	}
	if (err != 2) {
		xenbus_dev_fatal(dev, err, "reading physical-device");
		return;
	}

	if (be->major | be->minor) {
		if (be->major != major || be->minor != minor)
			pr_warning("blkback: changing physical device"
				   " (from %x:%x to %x:%x) not supported\n",
				   be->major, be->minor, major, minor);
		return;
	}

	be->mode = xenbus_read(XBT_NIL, dev->nodename, "mode", NULL);
	if (IS_ERR(be->mode)) {
		err = PTR_ERR(be->mode);
		be->mode = NULL;
		xenbus_dev_fatal(dev, err, "reading mode");
		return;
	}

	device_type = xenbus_read(XBT_NIL, dev->otherend, "device-type", NULL);
	if (!IS_ERR(device_type)) {
		cdrom = strcmp(device_type, "cdrom") == 0;
		kfree(device_type);
	}

	/* Front end dir is a number, which is used as the handle. */
	if (kstrtoul(strrchr(dev->otherend, '/') + 1, 0, &handle))
		return;

	be->major = major;
	be->minor = minor;

	err = vbd_create(be->blkif, handle, major, minor,
			 FMODE_READ
			 | (strchr(be->mode, 'w') ? FMODE_WRITE : 0)
			 | (strchr(be->mode, '!') ? 0 : FMODE_EXCL),
			 cdrom);

	switch (err) {
	case -ENOMEDIUM:
		if (!(be->blkif->vbd.type & (VDISK_CDROM | VDISK_REMOVABLE))) {
	default:
			xenbus_dev_fatal(dev, err, "creating vbd structure");
			break;
		}
		/* fall through */
	case 0:
		err = xenvbd_sysfs_addif(dev);
		if (err) {
			vbd_free(&be->blkif->vbd);
			xenbus_dev_fatal(dev, err, "creating sysfs entries");
		}
		break;
	}

	if (err) {
		kfree(be->mode);
		be->mode = NULL;
		be->major = be->minor = 0;
	} else {
		/* We're potentially connected now */
		update_blkif_status(be->blkif);

		/* Add watch for cdrom media status if necessay */
		cdrom_add_media_watch(be);
	}
}


/**
 * Callback received when the frontend's state changes.
 */
static void frontend_changed(struct xenbus_device *dev,
			     enum xenbus_state frontend_state)
{
	struct backend_info *be = dev_get_drvdata(&dev->dev);
	int err;

	DPRINTK("%s", xenbus_strstate(frontend_state));

	switch (frontend_state) {
	case XenbusStateInitialising:
		if (dev->state == XenbusStateClosed) {
			pr_info("%s: %s: prepare for reconnect\n",
				__FUNCTION__, dev->nodename);
			xenbus_switch_state(dev, XenbusStateInitWait);
		}
		break;

	case XenbusStateInitialised:
	case XenbusStateConnected:
		/* Ensure we connect even when two watches fire in 
		   close successsion and we miss the intermediate value 
		   of frontend_state. */
		if (dev->state == XenbusStateConnected)
			break;

		/* Enforce precondition before potential leak point.
		 * blkif_disconnect() is idempotent.
		 */
		blkif_disconnect(be->blkif);

		err = connect_ring(be);
		if (err)
			break;
		if (be->blkif->vbd.bdev)
			update_blkif_status(be->blkif);
		break;

	case XenbusStateClosing:
	case XenbusStateClosed:
		blkif_disconnect(be->blkif);
		xenbus_switch_state(dev, frontend_state);
		if (frontend_state != XenbusStateClosed ||
		    xenbus_dev_is_online(dev))
			break;
		/* fall through if not online */
	case XenbusStateUnknown:
		/* implies blkif_disconnect() via blkback_remove() */
		device_unregister(&dev->dev);
		break;

	default:
		xenbus_dev_fatal(dev, -EINVAL, "saw state %d at frontend",
				 frontend_state);
		break;
	}
}


/* ** Connection ** */


/**
 * Write the physical details regarding the block device to the store, and
 * switch to Connected state.
 */
static void connect(struct backend_info *be)
{
	struct xenbus_transaction xbt;
	int err;
	struct xenbus_device *dev = be->dev;

	DPRINTK("%s", dev->otherend);

	/* Supply the information about the device the frontend needs */
again:
	err = xenbus_transaction_start(&xbt);
	if (err) {
		xenbus_dev_fatal(dev, err, "starting transaction");
		return;
	}

	blkback_flush_diskcache(xbt, be, be->blkif->vbd.flush_support);
	blkback_barrier(xbt, be, be->blkif->vbd.flush_support);
	blkback_discard(xbt, be);

	err = xenbus_printf(xbt, dev->nodename, "sectors", "%llu",
			    vbd_size(&be->blkif->vbd));
	if (err) {
		xenbus_dev_fatal(dev, err, "writing %s/sectors",
				 dev->nodename);
		goto abort;
	}

	/* FIXME: use a typename instead */
	err = xenbus_printf(xbt, dev->nodename, "info", "%u",
			    be->blkif->vbd.type);
	if (err) {
		xenbus_dev_fatal(dev, err, "writing %s/info",
				 dev->nodename);
		goto abort;
	}
	err = xenbus_printf(xbt, dev->nodename, "sector-size", "%lu",
			    vbd_secsize(&be->blkif->vbd));
	if (err) {
		xenbus_dev_fatal(dev, err, "writing %s/sector-size",
				 dev->nodename);
		goto abort;
	}

	err = xenbus_printf(xbt, dev->nodename, "physical-sector-size", "%u",
			    bdev_physical_block_size(be->blkif->vbd.bdev));
	if (err)
		xenbus_dev_error(dev, err, "writing %s/physical-sector-size",
				 dev->nodename);

	err = xenbus_transaction_end(xbt, 0);
	if (err == -EAGAIN)
		goto again;
	if (err)
		xenbus_dev_fatal(dev, err, "ending transaction");

	err = xenbus_switch_state(dev, XenbusStateConnected);
	if (err)
		xenbus_dev_fatal(dev, err, "%s: switching to Connected state",
				 dev->nodename);

	return;
 abort:
	xenbus_transaction_end(xbt, 1);
}


static int connect_ring(struct backend_info *be)
{
	struct xenbus_device *dev = be->dev;
	unsigned int ring_ref[BLKIF_MAX_RING_PAGES], evtchn, ring_size;
	char *protocol;
	int err;

	DPRINTK("%s", dev->otherend);

	err = xenbus_scanf(XBT_NIL, dev->otherend, "event-channel", "%u",
			   &evtchn);
	if (err != 1) {
		xenbus_dev_fatal(dev, -EINVAL, "reading %s/event-channel",
				 dev->otherend);
		return -EINVAL;
	}

	pr_info("blkback: event-channel %u\n", evtchn);

	be->blkif->blk_protocol = BLKIF_PROTOCOL_NATIVE;
	protocol = xenbus_read(XBT_NIL, dev->otherend, "protocol", NULL);
	if (IS_ERR(protocol)) {
		protocol = NULL;
		be->blkif->blk_protocol = xen_guest_blkif_protocol(be->blkif->domid);
	}
#ifndef CONFIG_X86_32
	else if (0 == strcmp(protocol, XEN_IO_PROTO_ABI_X86_32))
		be->blkif->blk_protocol = BLKIF_PROTOCOL_X86_32;
#endif
#ifndef CONFIG_X86_64
	else if (0 == strcmp(protocol, XEN_IO_PROTO_ABI_X86_64))
		be->blkif->blk_protocol = BLKIF_PROTOCOL_X86_64;
#endif
	else if (0 != strcmp(protocol, XEN_IO_PROTO_ABI_NATIVE)) {
		xenbus_dev_fatal(dev, -EINVAL, "unknown fe protocol %s",
				 protocol);
		kfree(protocol);
		return -EINVAL;
	}

	pr_info("blkback: protocol %d (%s)\n",
		be->blkif->blk_protocol, protocol ?: "unspecified");
	kfree(protocol);

	err = xenbus_scanf(XBT_NIL, dev->otherend, "ring-page-order", "%u",
			   &ring_size);
	if (err != 1) {
		err = xenbus_scanf(XBT_NIL, dev->otherend, "ring-ref",
				   "%u", ring_ref);
		if (err != 1) {
			xenbus_dev_fatal(dev, -EINVAL, "reading %s/ring-ref",
					 dev->otherend);
			return -EINVAL;
		}
		pr_info("blkback: ring-ref %u\n", *ring_ref);
		ring_size = 1;
	} else if (ring_size > blkif_max_ring_page_order) {
		xenbus_dev_fatal(dev, -EINVAL,
				 "%s/ring-page-order (%u) too big",
				 dev->otherend, ring_size);
		return -EINVAL;
	} else {
		unsigned int i;

		ring_size = blkif_ring_size(be->blkif->blk_protocol,
					    1U << ring_size);

		for (i = 0; i < ring_size; i++) {
			char ring_ref_name[16];

			snprintf(ring_ref_name, sizeof (ring_ref_name),
				 "ring-ref%u", i);
			err = xenbus_scanf(XBT_NIL, dev->otherend,
					   ring_ref_name, "%u",
					   ring_ref + i);
			if (err != 1) {
				xenbus_dev_fatal(dev, -EINVAL, "reading %s/%s",
						 dev->otherend, ring_ref_name);
				return -EINVAL;
			}

			pr_info("blkback: ring-ref%u %u\n", i, ring_ref[i]);
		}
	}

	/* Map the shared frame, irq etc. */
	err = blkif_map(be->blkif, ring_ref, ring_size, evtchn);
	if (err) {
		xenbus_dev_fatal(dev, err, "mapping ring-refs and evtchn");
		return err;
	}

	return 0;
}


/* ** Driver Registration ** */


static const struct xenbus_device_id blkback_ids[] = {
	{ "vbd" },
	{ "" }
};

static DEFINE_XENBUS_DRIVER(blkback, ,
	.probe = blkback_probe,
	.remove = blkback_remove,
	.otherend_changed = frontend_changed
);


void blkif_xenbus_init(void)
{
	WARN_ON(xenbus_register_backend(&blkback_driver));
}
