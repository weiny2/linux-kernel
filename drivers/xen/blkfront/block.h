/******************************************************************************
 * block.h
 * 
 * Shared definitions between all levels of XenLinux Virtual block devices.
 * 
 * Copyright (c) 2003-2004, Keir Fraser & Steve Hand
 * Modifications by Mark A. Williamson are (c) Intel Research Cambridge
 * Copyright (c) 2004-2005, Christian Limpach
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation; or, when distributed
 * separately from the Linux kernel or incorporated into other
 * software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __XEN_DRIVERS_BLOCK_H__
#define __XEN_DRIVERS_BLOCK_H__

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/atomic.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <linux/blkdev.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <asm/hypervisor.h>
#include <xen/barrier.h>
#include <xen/xenbus.h>
#include <xen/gnttab.h>
#include <xen/interface/xen.h>
#include <xen/interface/io/blkif.h>
#include <xen/interface/io/ring.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define DPRINTK(_f, _a...) pr_debug(_f, ## _a)

#if 0
#define DPRINTK_IOCTL(_f, _a...) pr_alert(_f, ## _a)
#else
#define DPRINTK_IOCTL(_f, _a...) ((void)0)
#endif

struct xlbd_major_info
{
	int major;
	int index;
	int usage;
	const struct xlbd_type_info *type;
	struct xlbd_minor_state *minors;
};

struct blk_shadow {
	union {
		blkif_request_t req;
		blkif_request_indirect_t ind;
	};
	struct request *request;
	unsigned long *frame;
};

#define BLK_MAX_RING_PAGE_ORDER 4U
#define BLK_MAX_RING_PAGES (1U << BLK_MAX_RING_PAGE_ORDER)
#define BLK_MAX_RING_SIZE __CONST_RING_SIZE(blkif, \
					    BLK_MAX_RING_PAGES * PAGE_SIZE)

/*
 * We have one of these per vbd, whether ide, scsi or 'other'.  They
 * hang in private_data off the gendisk structure. We may end up
 * putting all kinds of interesting stuff here :-)
 */
struct blkfront_info
{
	struct xenbus_device *xbdev;
 	struct gendisk *gd;
	struct mutex mutex;
	int vdevice;
	blkif_vdev_t handle;
	int connected;
	unsigned int ring_size;
	blkif_front_ring_t ring;
	spinlock_t io_lock;
	struct scatterlist *sg;
	struct blkif_request_segment **indirect_segs;
	unsigned int irq;
	unsigned int max_segs_per_req;
	struct xlbd_major_info *mi;
	struct request_queue *rq;
	struct work_struct work;
	struct gnttab_free_callback callback;
	struct blk_shadow shadow[BLK_MAX_RING_SIZE];
	struct list_head resume_list, resume_split;
	grant_ref_t ring_refs[BLK_MAX_RING_PAGES];
	struct page *ring_pages[BLK_MAX_RING_PAGES];
	unsigned long shadow_free;
	unsigned int feature_flush;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	unsigned int flush_op;
#endif
	bool feature_discard;
	bool feature_secdiscard;
	unsigned int discard_granularity;
	unsigned int discard_alignment;
	int is_ready;
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,28)
extern int blkif_open(struct inode *inode, struct file *filep);
extern int blkif_release(struct inode *inode, struct file *filep);
extern int blkif_ioctl(struct inode *inode, struct file *filep,
		       unsigned command, unsigned long argument);
#else
extern int blkif_open(struct block_device *bdev, fmode_t mode);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
extern int blkif_release(struct gendisk *disk, fmode_t mode);
#else
extern void blkif_release(struct gendisk *disk, fmode_t mode);
#endif
extern int blkif_ioctl(struct block_device *bdev, fmode_t mode,
		       unsigned command, unsigned long argument);
#endif
extern int blkif_getgeo(struct block_device *, struct hd_geometry *);
extern int blkif_check(dev_t dev);
extern int blkif_revalidate(dev_t dev);
extern void do_blkif_request (struct request_queue *rq);

/* Virtual block-device subsystem. */
/* Note that xlvbd_add doesn't call add_disk for you: you're expected
   to call add_disk on info->gd once the disk is properly connected
   up. */
int xlvbd_add(blkif_sector_t capacity, int device, unsigned int vdisk_info,
	      unsigned int sector_size, unsigned int physical_sector_size,
	      struct blkfront_info *);
void xlvbd_del(struct blkfront_info *info);
void xlvbd_flush(struct blkfront_info *info);

#ifdef CONFIG_SYSFS
int xlvbd_sysfs_addif(struct blkfront_info *info);
void xlvbd_sysfs_delif(struct blkfront_info *info);
#else
static inline int xlvbd_sysfs_addif(struct blkfront_info *info)
{
	return 0;
}

static inline void xlvbd_sysfs_delif(struct blkfront_info *info)
{
	;
}
#endif

void xlbd_release_major_info(void);

/* Virtual cdrom block-device */
#ifdef CONFIG_XEN
extern void register_vcd(struct blkfront_info *info);
extern void unregister_vcd(struct blkfront_info *info);
#else
static inline void register_vcd(struct blkfront_info *info) {}
static inline void unregister_vcd(struct blkfront_info *info) {}
#endif

#endif /* __XEN_DRIVERS_BLOCK_H__ */
