/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2013 - 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2013 - 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Intel(R) Omni-Path CSR Access Driver
 */

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <rdma/opa_core.h>
#include "device.h"

#define OPA_UI_SUFFIX			"_ui"

static int ui_open(struct inode *inode, struct file *filp)
{
	struct hfi_info *hi = container_of(filp->private_data,
					   struct hfi_info, ui_miscdev);
	int ret;

	/* hold the device so it cannot be removed until user calls close() */
	/* TODO - may be a race here, see STL-3033, also see ib_uverbs_open. */
	ret = opa_core_device_get(hi->odev);
	if (ret)
		return ret;
	filp->private_data = hi->odev; /* for other methods */
	return 0;
}

static int ui_release(struct inode *inode, struct file *filp)
{
	struct opa_core_device *odev = filp->private_data;

	opa_core_device_put(odev);
	return 0;
}

static loff_t ui_lseek(struct file *filp, loff_t offset, int whence)
{
	struct opa_core_device *odev = filp->private_data;

	/* CSR access is for root user only for security reasons */
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	switch (whence) {
	case SEEK_SET:
		break;
	case SEEK_CUR:
		offset += filp->f_pos;
		break;
	case SEEK_END:
		offset = (odev->kregend - odev->kregbase) - offset;
		break;
	default:
		return -EINVAL;
	}

	if (offset < 0)
		return -EINVAL;

	if (offset >= odev->kregend - odev->kregbase)
		return -EINVAL;

	filp->f_pos = offset;

	return filp->f_pos;
}

/* NOTE: assumes unsigned long is 8 bytes */
static ssize_t ui_read(struct file *filp, char __user *buf, size_t count,
			loff_t *f_pos)
{
	struct opa_core_device *odev = filp->private_data;
	void *base;
	unsigned long total = 0, data;

	/* CSR access is for root user only for security reasons */
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	/* only read 8 byte quantities */
	if ((count % 8) != 0)
		return -EINVAL;
	/* offset must be 8-byte aligned */
	if ((*f_pos % 8) != 0)
		return -EINVAL;
	/* destination buffer must be 8-byte aligned */
	if ((unsigned long)buf % 8 != 0)
		return -EINVAL;

	/* must be in range */
	if (*f_pos + count > odev->kregend - odev->kregbase)
		return -EINVAL;
	base = (void *)odev->kregbase + *f_pos;
	for (total = 0; total < count; total += 8) {
		data = readq(base + total);
		if (put_user(data, (unsigned long *)(buf + total)))
			break;
	}

	*f_pos += total;
	return total;
}


/* NOTE: assumes unsigned long is 8 bytes */
static ssize_t ui_write(struct file *filp, const char __user *buf,
			size_t count, loff_t *f_pos)
{
	struct opa_core_device *odev = filp->private_data;
	void *base;
	unsigned long total, data;

	/* CSR access is for root user only for security reasons */
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	/* only write 8 byte quantities */
	if ((count % 8) != 0)
		return -EINVAL;
	/* offset must be 8-byte aligned */
	if ((*f_pos % 8) != 0)
		return -EINVAL;
	/* source buffer must be 8-byte aligned */
	if ((unsigned long)buf % 8 != 0)
		return -EINVAL;
	/* must be in range */
	if (*f_pos + count > odev->kregend - odev->kregbase)
		return -EINVAL;

	base = (void *)odev->kregbase + *f_pos;
	for (total = 0; total < count; total += 8) {
		if (get_user(data, (unsigned long *)(buf + total)))
			break;
		writeq(data, base + total);
	}
	*f_pos += total;
	return total;
}

static const struct file_operations ui_file_ops = {
	.owner = THIS_MODULE,
	.read = ui_read,
	.llseek = ui_lseek,
	.write = ui_write,
	.open = ui_open,
	.release = ui_release,
};

int hfi_ui_add(struct hfi_info *hi)
{
	int rc;
	struct miscdevice *mdev;
	struct opa_core_device *odev = hi->odev;

	mdev = &hi->ui_miscdev;
	mdev->minor = MISC_DYNAMIC_MINOR;
	snprintf(hi->ui_name, sizeof(hi->ui_name), "%s%d%s",
		 DRIVER_DEVICE_PREFIX, odev->index, OPA_UI_SUFFIX);
	mdev->name = hi->ui_name;
	mdev->fops = &ui_file_ops;
	mdev->parent = &odev->dev;

	rc = misc_register(mdev);
	if (rc)
		dev_err(&odev->dev, "%s failed rc %d\n", __func__, rc);
	return rc;
}

void hfi_ui_remove(struct hfi_info *hi)
{
	misc_deregister(&hi->ui_miscdev);
}
