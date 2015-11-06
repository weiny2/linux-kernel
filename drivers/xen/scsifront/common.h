/*
 * Xen SCSI frontend driver
 *
 * Copyright (c) 2008, FUJITSU Limited
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

#ifndef __XEN_DRIVERS_SCSIFRONT_H__
#define __XEN_DRIVERS_SCSIFRONT_H__

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/blkdev.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi.h>
#include <scsi/scsi_host.h>
#include <xen/barrier.h>
#include <xen/xenbus.h>
#include <xen/gnttab.h>
#include <xen/evtchn.h>
#include <xen/interface/xen.h>
#include <xen/interface/io/ring.h>
#include <xen/interface/io/vscsiif.h>
#include <xen/interface/grant_table.h>
#include <xen/interface/io/protocols.h>
#include <asm/delay.h>
#include <asm/hypervisor.h>
#include <asm/maddr.h>

#ifdef HAVE_XEN_PLATFORM_COMPAT_H
#include <xen/platform-compat.h>
#endif

#define VSCSI_IN_ABORT		1
#define VSCSI_IN_RESET		2

/* tuning point*/
#define VSCSIIF_DEFAULT_CMD_PER_LUN 10
#define VSCSIIF_MAX_TARGET          64
#define VSCSIIF_MAX_LUN             255

#define VSCSIIF_RING_SIZE	__CONST_RING_SIZE(vscsiif, PAGE_SIZE)
#define VSCSIIF_MAX_REQS	VSCSIIF_RING_SIZE

struct vscsifrnt_shadow {
	uint16_t next_free;
#define VSCSIIF_IN_USE		0x0fff
#define VSCSIIF_NONE		0x0ffe

	/* command between backend and frontend
	 * VSCSIIF_ACT_SCSI_CDB or VSCSIIF_ACT_SCSI_RESET */
	unsigned char act;
	
	/* Number of pieces of scatter-gather */
	unsigned int nr_segments;

	/* do reset function */
	wait_queue_head_t wq_reset;	/* reset work queue           */
	int wait_reset;			/* reset work queue condition */
	int32_t rslt_reset;		/* reset response status      */
					/* (SUCESS or FAILED)         */

	/* requested struct scsi_cmnd is stored from kernel */
	struct scsi_cmnd *sc;
	int gref[SG_ALL];
};

struct vscsifrnt_info {
	struct xenbus_device *dev;

	struct Scsi_Host *host;

	spinlock_t shadow_lock;
	unsigned int evtchn;
	unsigned int irq;

	grant_ref_t ring_ref;
	struct vscsiif_front_ring ring;

	struct vscsifrnt_shadow shadow[VSCSIIF_MAX_REQS];
	uint32_t shadow_free;

	struct task_struct *kthread;
	wait_queue_head_t wq;
	wait_queue_head_t wq_sync;
	wait_queue_head_t wq_pause;
	unsigned char waiting_resp;
	unsigned char waiting_sync;
	unsigned char waiting_pause;
	unsigned char pause;
	unsigned callers;

	struct {
		struct scsi_cmnd *sc;
		unsigned int rqid;
		unsigned int done;
		struct scsiif_request_segment segs[];
	} active;
};

#define DPRINTK(_f, _a...)				\
	pr_debug("(file=%s, line=%d) " _f,	\
		 __FILE__ , __LINE__ , ## _a )

int scsifront_xenbus_init(void);
void scsifront_xenbus_unregister(void);
int scsifront_schedule(void *data);
void scsifront_finish_all(struct vscsifrnt_info *info);
irqreturn_t scsifront_intr(int irq, void *dev_id);

#endif /* __XEN_DRIVERS_SCSIFRONT_H__  */
