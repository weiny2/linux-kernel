// SPDX-License-Identifier: GPL-2.0-only
/* Copyright 2024 Intel Corp. */

#include <linux/cxl-event.h>
#include <linux/debugfs.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <acpi/apei.h>

#include "apei-internal.h"

#include "../internal.h"

struct dentry *apei_debugfs_dir;
extern spinlock_t ghes_notify_lock_irq;

static void mock_ghes_poll(enum cxl_event_type event_type,
			   struct cxl_cper_event_rec *rec)
{
	unsigned long flags;

	spin_lock_irqsave(&ghes_notify_lock_irq, flags);
	cxl_cper_post_event(event_type, rec);
	spin_unlock_irqrestore(&ghes_notify_lock_irq, flags);
}

static void ghes_cper_inject(char *bdfstr)
{
	struct cxl_cper_event_rec rec;
	char *segstr, *busstr, *devstr;
	u8 seg, bus, dev, func;

	segstr = strsep(&bdfstr, ":");
	if (!segstr || !bdfstr)
		return;
	seg = strtoul(segstr, NULL, 16);
	busstr = strsep(&bdfstr, ":");
	if (!busstr || !bdfstr)
		return;
	bus = strtoul(busstr, NULL, 16);
	devstr = strsep(&bdfstr, ".");
	if (!devstr || !bdfstr)
		return;
	dev = strtoul(devstr, NULL, 16);
	func = strtoul(bdfstr, NULL, 16);
	if (bus == ULONG_MAX)
		return;

	pr_debug("Record %u:%u:%u.%u\n", seg, bus, dev, func);

	rec = (struct cxl_cper_event_rec) {
		.hdr = {
			.device_id = {
				.func_num = func,
				.device_num = dev,
				.bus_num = bus,
				.segment_num = seg,
			},
			.dev_serial_num = {
				.lower_dw = 0xBEEF,
				.upper_dw = 0xDEAD,
			},
		},
	};

	/* Fail; invalid record length (0) */
	mock_ghes_poll(CXL_CPER_EVENT_GEN_MEDIA, &rec);

	rec.hdr.length = sizeof(rec);
	/* Fail; log valid bit not set */
	mock_ghes_poll(CXL_CPER_EVENT_GEN_MEDIA, &rec);

	rec.hdr.validation_bits |= CPER_CXL_COMP_EVENT_LOG_VALID;
	mock_ghes_poll(CXL_CPER_EVENT_GEN_MEDIA, &rec);
	mock_ghes_poll(CXL_CPER_EVENT_DRAM, &rec);
	mock_ghes_poll(CXL_CPER_EVENT_MEM_MODULE, &rec);
	mock_ghes_poll(CXL_CPER_EVENT_GEN_MEDIA, &rec);
	mock_ghes_poll(CXL_CPER_EVENT_MEM_MODULE, &rec);
}

static ssize_t ghes_cxl_dbg_write(struct file *file, const char __user *buf,
				  size_t count, loff_t *ppos)
{
	char bdfstr[64];

	if (!count)
		return 0;
	if (!access_ok(buf, count))
		return -EFAULT;

	if (copy_from_user(bdfstr, buf, min(sizeof(bdfstr)-1, count)))
		return -EINVAL;

	bdfstr[count - 1] = '\0';
	ghes_cper_inject(bdfstr);

	return count;
}

static const struct file_operations ghes_cxl_cper_fops = {
	.owner	 = THIS_MODULE,
	.write	 = ghes_cxl_dbg_write,
};

void __init ghes_debugfs_init(void)
{
	apei_debugfs_dir = debugfs_create_dir("apei", NULL);
	debugfs_create_file("inject_cxl_cper", 0600, apei_debugfs_dir, NULL,
			    &ghes_cxl_cper_fops);
}
