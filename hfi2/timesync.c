/*
 * Intel Omni-Path Architecture (OPA) Host Fabric Software
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2015 Intel Corporation.
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
 * Copyright (c) 2015 Intel Corporation.
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
 */

#include <linux/ptp_clock_kernel.h>
#include <linux/ptp_classify.h>
#include <linux/mutex.h>
#include "opa_hfi.h"
#include "link.h"
#include "timesync.h"

/**
 * hfi_ptp_read - Read the PHC time from the device
 * @ppd: Board private structure
 * @ts: timespec structure to hold the current time value
 *
 * This function reads the local time register and stores it in a
 * timespec. However, since the registers are 64 bits of nanoseconds, we must
 * convert the result to a timespec before we can return.
 **/
static void hfi_ptp_read(struct hfi_pportdata *ppd, struct timespec64 *ts)
{
	u64 ns;

	ns = read_fzc_csr(ppd, FZC_LCB_STS_UPPER_LOCAL_TIME);
	*ts = ns_to_timespec64(ns);
}

/**
 * hfi_ptp_write - Write the PHC time to the device
 * @ppd: Board private structure
 * @ts: timespec structure that holds the new time value
 *
 * This function writes the local time register with the user value. Since
 * we receive a timespec from the stack, we must convert that timespec into
 * nanoseconds before programming the register.
 **/
static void hfi_ptp_write(struct hfi_pportdata *ppd,
			  const struct timespec64 *ts)
{
	u64 ns = timespec64_to_ns(ts);

	write_fzc_csr(ppd, FZC_LCB_STS_UPPER_LOCAL_TIME, ns);
}

/**
 * hfi_ptp_adjfreq - Adjust the PHC frequency
 * @ptp: The PTP clock structure
 * @ppb: Parts per billion adjustment from the base
 *
 * Adjust the frequency of the PHC by the indicated parts per billion from the
 * base frequency.
 **/
static int hfi_ptp_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
	struct hfi_pportdata *ppd = container_of(ptp, struct hfi_pportdata,
						 ptp_caps);
	u64 adj, freq, diff;
	int neg_adj = 0;

	/* FXRTODO: This logic may be wrong for STL2 */

	if (ppb < 0) {
		neg_adj = 1;
		ppb = -ppb;
	}

	freq = read_fzc_csr(ppd, FZC_LCB_CFG_TIME_SYNC_1);
	freq &= FZC_LCB_CFG_TIME_SYNC_1_TSYN_INC_SMASK;
	adj = freq;

	freq *= ppb;
	diff = div_u64(freq, 1000000000ULL);

	if (neg_adj)
		adj -= diff;
	else
		adj += diff;

	write_fzc_csr(ppd, FZC_LCB_CFG_TIME_SYNC_1, adj &
		      FZC_LCB_CFG_TIME_SYNC_1_TSYN_INC_MASK); /* 38 bits */

	return 0;
}

/**
 * hfi_ptp_adjtime - Adjust the PHC time
 * @ptp: The PTP clock structure
 * @delta: Offset in nanoseconds to adjust the PHC time by
 *
 * Adjust the time of the PHC by the indicated delta using read/modify/write.
 **/
static int hfi_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	struct hfi_pportdata *ppd = container_of(ptp, struct hfi_pportdata,
						 ptp_caps);
	struct timespec64 now, then = ns_to_timespec64(delta);
	unsigned long flags;

	spin_lock_irqsave(&ppd->tmreg_lock, flags);

	hfi_ptp_read(ppd, &now);
	now = timespec64_add(now, then);
	hfi_ptp_write(ppd, (struct timespec64 *)&now);

	spin_unlock_irqrestore(&ppd->tmreg_lock, flags);
	return 0;
}

/**
 * hfi_ptp_gettime - Get the time of the PHC
 * @ptp: The PTP clock structure
 * @ts: timespec structure to hold the current time value
 *
 * Read the device clock and return the correct value on ns, after converting it
 * into a timespec struct.
 **/
static int hfi_ptp_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	struct hfi_pportdata *ppd = container_of(ptp, struct hfi_pportdata,
						 ptp_caps);
	unsigned long flags;

	spin_lock_irqsave(&ppd->tmreg_lock, flags);
	hfi_ptp_read(ppd, ts);
	spin_unlock_irqrestore(&ppd->tmreg_lock, flags);

	return 0;
}

/**
 * hfi_ptp_settime - Set the time of the PHC
 * @ptp: The PTP clock structure
 * @ts: timespec structure that holds the new time value
 *
 * Set the device clock to the user input value. The conversion from timespec
 * to ns happens in the write function.
 **/
static int hfi_ptp_settime(struct ptp_clock_info *ptp,
			   const struct timespec64 *ts)
{
	struct hfi_pportdata *ppd = container_of(ptp, struct hfi_pportdata,
						 ptp_caps);
	unsigned long flags;

	spin_lock_irqsave(&ppd->tmreg_lock, flags);
	hfi_ptp_write(ppd, ts);
	spin_unlock_irqrestore(&ppd->tmreg_lock, flags);

	return 0;
}

/**
 * hfi_ptp_feature_enable - Enable/disable ancillary features of the PHC
 * subsystem
 * @ptp: The PTP clock structure
 * @rq: The requested feature to change
 * @on: Enable/disable flag
 *
 * The XL710 does not support any of the ancillary features of the PHC
 * subsystem, so this function may just return.
 **/
static int hfi_ptp_feature_enable(struct ptp_clock_info *ptp,
				  struct ptp_clock_request *rq, int on)
{
	return -EOPNOTSUPP;
}

/**
 * hfi_ptp_create_clock - Create PTP clock device for userspace
 * @ppd: Board private structure
 *
 * This function creates a new PTP clock device. It only creates one if we
 * don't already have one, so it is safe to call. Will return error if it
 * can't create one, but success if we already have a device. Should be used
 * by hfi_timesync_init to create clock initially, and prevent global resets
 * from creating new clock devices.
 **/
static long hfi_ptp_create_clock(struct hfi_pportdata *ppd)
{
	/* no need to create a clock device if we already have one */
	if (!IS_ERR_OR_NULL(ppd->ptp_clock))
		return 0;

	strncpy(ppd->ptp_caps.name, KBUILD_MODNAME, sizeof(ppd->ptp_caps.name));
	ppd->ptp_caps.owner = THIS_MODULE;
	ppd->ptp_caps.max_adj = 999999999;
	ppd->ptp_caps.n_ext_ts = 0;
	ppd->ptp_caps.pps = 0;
	ppd->ptp_caps.adjfreq = hfi_ptp_adjfreq;
	ppd->ptp_caps.adjtime = hfi_ptp_adjtime;
	ppd->ptp_caps.gettime64 = hfi_ptp_gettime;
	ppd->ptp_caps.settime64 = hfi_ptp_settime;
	ppd->ptp_caps.enable = hfi_ptp_feature_enable;

	/* Attempt to register the clock */
	ppd->ptp_clock = ptp_clock_register(&ppd->ptp_caps,
					    &ppd->dd->pcidev->dev);
	if (IS_ERR(ppd->ptp_clock))
		return PTR_ERR(ppd->ptp_clock);

	return 0;
}

/**
 * hfi_timesync_init - Initialize timesync support after link training
 * @ppd: Port specific structure
 *
 * Initialize support for timesync in the Intel OPA fabic. The first time it
 * is run, it will create a PHC clock device. It does not create a clock
 * device if one already exists. It also reconfigures the device after a reset.
 **/
void hfi_timesync_init(struct hfi_pportdata *ppd)
{
	long err;

	/* Skip if already initialized */
	if (ppd->ptp_clock)
		return;

	/* we have to initialize the lock first, since we can't control
	 * when the user will enter the PHC device entry points
	 */
	spin_lock_init(&ppd->tmreg_lock);

	/* Mutex to separate init and stop */
	mutex_init(&ppd->timesync_mutex);
	mutex_lock(&ppd->timesync_mutex);

	/* ensure we have a clock device */
	err = hfi_ptp_create_clock(ppd);

	if (err) {
		ppd->ptp_clock = NULL;
		dd_dev_err(ppd->dd, "Timesync initialization failed");
	} else {
		ppd->ptp_index = ptp_clock_index(ppd->ptp_clock);
#if 0
		/* FXRTODO: complete this for FXR timesync when registers
		 * are functional
		 */
		struct timespec64 ts;
		u32 regval;
#endif
		ppd_dev_info(ppd, "%s: added PHC on /dev/ptp%d\n", __func__,
			     ppd->ptp_index);
#if 0
		/* DEBUG - Check for flits on each clockid and report */

		/* Set the clock value. */
		ts = ktime_to_timespec64(ktime_get_real());
		hfi_ptp_settime(&pf->ptp_caps, &ts);
#endif
	}
	mutex_unlock(&ppd->timesync_mutex);
}

/**
 * hfi_timesync_stop - Disable the driver/hardware support and unregister
 *                     the PHC
 * @ppd: Port specific structure
 *
 * This function handles the cleanup work required from the initialization by
 * clearing out the important information and unregistering the PHC.
 **/
void hfi_timesync_stop(struct hfi_pportdata *ppd)
{
	if (ppd->ptp_clock) {
		mutex_lock(&ppd->timesync_mutex);
		ptp_clock_unregister(ppd->ptp_clock);
		ppd->ptp_clock = NULL;
		ppd_dev_dbg(ppd, "%s: removed PHC on %s\n", __func__,
			    "device TBD");
		mutex_unlock(&ppd->timesync_mutex);
	}
}

/**
 * hfi_get_ts_master_regs - Capture master low and high, check 8 bit overlap.
 * If mismatch, return EAGAIN.  Fill structure master_ts with the decoded
 * master time and the timestamp.
 * @ctx: Context struct
 * @master_ts: Struct containing master time and timestamp
 **/
int hfi_get_ts_master_regs(struct hfi_ctx *ctx,
			   struct hfi_ts_master_regs *master_ts)
{
	u64 master_low;		/* 40 bits with 8 bits of overlap */
	u64 master_high;
	u64 reg;
	struct hfi_pportdata *ppd = to_hfi_ppd(ctx->devdata, master_ts->port);
	u64 ns;

	if (!ppd)
		return -ENOENT;

	/* Disable_snaps here to ensure atomic reads */
	reg = read_fzc_csr(ppd, FZC_LCB_CFG_TIME_SYNC_0);
	reg |= 1 << (FZC_LCB_CFG_TIME_SYNC_0_DISABLE_SNAPS_SHIFT +
		     3 - master_ts->clkid);
	write_fzc_csr(ppd, FZC_LCB_CFG_TIME_SYNC_0, reg);

	master_low = read_fzc_csr(ppd, FZC_LCB_STS_MASTER_TIME_LOW_0 + 0x18 *
				  master_ts->clkid) &
				  FZC_LCB_STS_MASTER_TIME_LOW_0_VAL_SMASK;
	master_high = read_fzc_csr(ppd, FZC_LCB_STS_MASTER_TIME_HIGH_0 + 0x18 *
				   master_ts->clkid) &
				   FZC_LCB_STS_MASTER_TIME_HIGH_0_VAL_SMASK;
	master_ts->timestamp = read_fzc_csr(ppd,
					    FZC_LCB_STS_SNAPPED_LOCAL_TIME_0 +
					    0x18 * master_ts->clkid);
	ns = read_fzc_csr(ppd, FZC_LCB_STS_UPPER_LOCAL_TIME); /* temp hack */
	/* Enable snaps again */
	reg &= ~FZC_LCB_CFG_TIME_SYNC_0_DISABLE_SNAPS_SMASK;
	write_fzc_csr(ppd, FZC_LCB_CFG_TIME_SYNC_0, reg);
	/* See if low and high are from the same sample */
	if ((master_high & 0xff) == (master_low >> 32)) {
		master_ts->master = (master_high << 32) |
				    (master_low & 0xffffffff);
		return 0;
	}
	return -EAGAIN;
}

static void simics_hack_write_local_time(struct hfi_pportdata *ppd, u64 ns)
{
	write_fzc_csr(ppd, FZC_LCB_STS_UPPER_LOCAL_TIME, ns);
}

/*
 * Temporary hack to configure Simics for timesync.
 */
void simics_hack_timesync(struct hfi_pportdata *ppd, u64 hack_timesync, u8 port)
{
#define HACK_TIMESYNC_1 0x2000040
#define HACK_TIMESYNC_2 0x2004040
/* abbccccdddddddddddddddddddddddd  fields
 * 0987654321098765432109876543210  ruler
 * 3         2         1
 *        111111111111111111111111  SnapFreq
 *    1111   .   .   .   .   .   .  TimeIncShift
 *  11   .   .   .   .   .   .   .  ClockId
 * 1 .   .   .   .   .   .   .   .  Enable
 *   .   .   .   .   .   .   .   .
 *   .   .   .   .   .   .   .   .  mask         shift
 *   .   .   f   f   f   f   f   f    0xffffff    0
 *   .   f   0   0   0   0   0   0   0xf000000   24
 *   3   0   0   0   0   0   0   0  0x30000000   28
 *   4   0   0   0   0   0   0   0  0x40000000   30
 *
 * If bit 63 is 1 then write register otherwise ignore.
 */
	u32 a, b, c, d;

	if (!(hack_timesync & (1UL << 63)))
		return;

	a = (hack_timesync & 0x40000000) >> 30;
	b = (hack_timesync & 0x30000000) >> 28;
	c = (hack_timesync & 0xf000000) >> 24;
	d = hack_timesync & 0xffffff;

	if (port == 1)
		write_csr(ppd->dd, HACK_TIMESYNC_1, hack_timesync);
	else
		write_csr(ppd->dd, HACK_TIMESYNC_2, hack_timesync);
}

/**
 * hfi_get_ts_fm_data - Returns data collected from FM MAD interface for
 * timesync parameters needed to ensure accurate synchronization on the fabric.
 * @ctx: Context struct
 * @fm_data: Struct containing data collected from FM MAD interface
 **/
int hfi_get_ts_fm_data(struct hfi_ctx *ctx, struct hfi_ts_fm_data *fm_data)
{
	struct hfi_pportdata *ppd = to_hfi_ppd(ctx->devdata, fm_data->port);

	if (!ppd)
		return -ENOENT;

	/* Dummy data for testing */
	ppd->propagation_delay = 100;
	ppd->fabric_time_offset = 500;
	ppd->update_interval = 100;
	ppd->current_clock_id = 0;
	/* End dummy data */

	fm_data->propagation_delay = ppd->propagation_delay;
	fm_data->fabric_time_offset = ppd->fabric_time_offset;
	fm_data->update_interval = ppd->update_interval;
	fm_data->current_clock_id = ppd->current_clock_id;
	fm_data->ptp_index = ppd->ptp_index;
	simics_hack_timesync(ppd, fm_data->hack_timesync, fm_data->port);
	return 0;
}
