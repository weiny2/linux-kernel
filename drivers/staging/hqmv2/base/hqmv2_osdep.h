/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2018 Intel Corporation
 */

#ifndef __HQMV2_OSDEP_H
#define __HQMV2_OSDEP_H

#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <asm/byteorder.h>
#include "../hqmv2_main.h"
#include "hqmv2_resource.h"
#include <uapi/linux/hqmv2_user.h>

#define HQMV2_PCI_REG_READ(addr)        ioread32(addr)
#define HQMV2_PCI_REG_WRITE(reg, value) iowrite32(value, reg)

#define HQMV2_CSR_WRITE_LOG(hw, reg, value, str) \
	HQMV2_BASE_INFO(hw, \
			"[%s()] *(0x%x) = 0x%x, read back 0x%x (reg: %s)\n",\
			__func__, reg, value, HQMV2_CSR_RD(hw, reg), str)

/* Read/write register 'reg' in the CSR BAR space */
#define HQMV2_CSR_REG_ADDR(a, reg) ((a)->csr_kva + (reg))
#define HQMV2_CSR_RD(hw, reg) \
	HQMV2_PCI_REG_READ(HQMV2_CSR_REG_ADDR((hw), (reg)))
#define HQMV2_CSR_WR(hw, reg, value) \
	do { \
		HQMV2_PCI_REG_WRITE(HQMV2_CSR_REG_ADDR((hw), (reg)), (value)); \
		HQMV2_CSR_WRITE_LOG(hw, reg, value, #reg); \
	} while (0)

/* Read/write register 'reg' in the func BAR space */
#define HQMV2_FUNC_REG_ADDR(a, reg) ((a)->func_kva + (reg))
#define HQMV2_FUNC_RD(hw, reg) \
	HQMV2_PCI_REG_READ(HQMV2_FUNC_REG_ADDR((hw), (reg)))
#define HQMV2_FUNC_WR(hw, reg, value) \
	HQMV2_PCI_REG_WRITE(HQMV2_FUNC_REG_ADDR((hw), (reg)), (value))

/* Macros that prevent the compiler from optimizing away memory accesses */
#define OS_READ_ONCE(x) READ_ONCE(x)
#define OS_WRITE_ONCE(x, y) WRITE_ONCE(x, y)

/**
 * os_udelay() - busy-wait for a number of microseconds
 * @usecs: delay duration.
 */
static inline void os_udelay(int usecs)
{
	udelay(usecs);
}

/**
 * os_msleep() - sleep for a number of milliseconds
 * @usecs: delay duration.
 */
static inline void os_msleep(int msecs)
{
	msleep(msecs);
}

/**
 * os_curtime_s() - get the current time (in seconds)
 * @usecs: delay duration.
 */
static inline unsigned long os_curtime_s(void)
{
	return jiffies_to_msecs(jiffies) / 1000;
}

/**
 * os_map_producer_port() - map a producer port into the caller's address space
 * @hw: hqmv2_hw handle for a particular device.
 * @port_id: port ID
 * @is_ldb: true for load-balanced port, false for a directed port
 *
 * This function maps the requested producer port memory into the caller's
 * address space.
 *
 * Return:
 * Returns the base address at which the PP memory was mapped, else NULL.
 */
static inline void *os_map_producer_port(struct hqmv2_hw *hw,
					 u8 port_id,
					 bool is_ldb)
{
	struct hqmv2_dev *hqmv2_dev;
	unsigned long size;
	uintptr_t address;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	address = hqmv2_dev->hw.func_phys_addr;

	if (is_ldb) {
		size = HQMV2_LDB_PP_STRIDE;
		address += HQMV2_DRV_LDB_PP_BASE + size * port_id;
	} else {
		size = HQMV2_DIR_PP_STRIDE;
		address += HQMV2_DRV_DIR_PP_BASE + size * port_id;
	}

	return devm_ioremap(&hqmv2_dev->pdev->dev, address, size);
}

/**
 * os_unmap_producer_port() - unmap a producer port
 * @addr: mapped producer port address
 *
 * This function undoes os_map_producer_port() by unmapping the producer port
 * memory from the caller's address space.
 *
 * Return:
 * Returns the base address at which the PP memory was mapped, else NULL.
 */
static inline void os_unmap_producer_port(struct hqmv2_hw *hw, void *addr)
{
	struct hqmv2_dev *hqmv2_dev;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	devm_iounmap(&hqmv2_dev->pdev->dev, addr);
}

/**
 * os_fence_hcw() - fence an HCW to ensure it arrives at the device
 * @hcw: pointer to the 64B-aligned contiguous HCW memory
 * @addr: producer port address
 */
static inline void os_fence_hcw(struct hqmv2_hw *hw, u64 *pp_addr)
{
	/* To ensure outstanding HCWs reach the device, read the PP address. IA
	 * memory ordering prevents reads from passing older writes, and the
	 * mfence also ensures this.
	 */
	mb();

	READ_ONCE(pp_addr);
}

/**
 * os_enqueue_four_hcws() - enqueue four HCWs to HQM
 * @hw: hqm_hw handle for a particular device.
 * @hcw: pointer to the 64B-aligned contiguous HCW memory
 * @addr: producer port address
 */
static inline void os_enqueue_four_hcws(struct hqmv2_hw *hw,
					struct hqmv2_hcw *hcw,
					void *addr)
{
	struct hqmv2_dev *hqmv2_dev;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	hqmv2_dev->enqueue_four(hcw, addr);
}

/**
 * os_notify_user_space() - notify user space
 * @hw: hqmv2_hw handle for a particular device.
 * @domain_id: ID of domain to notify.
 * @alert_id: alert ID.
 * @aux_alert_data: additional alert data.
 *
 * This function notifies user space of an alert (such as a hardware alarm).
 *
 * Return:
 * Returns 0 upon success, <0 otherwise.
 */
static inline int os_notify_user_space(struct hqmv2_hw *hw,
				       u32 domain_id,
				       u64 alert_id,
				       u64 aux_alert_data)
{
	struct hqmv2_domain_dev *domain_dev;
	struct hqmv2_domain_alert alert;
	struct hqmv2_dev *hqmv2_dev;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	domain_dev = &hqmv2_dev->sched_domains[domain_id];

	/* TODO: Add support for notifying VF-owned domains through the PF->VF
	 * mailbox.
	 */
	if (hw->domains[domain_id].id.vdev_owned) {
		WARN_ON(hw->domains[domain_id].id.vdev_owned);
		return -EINVAL;
	}

	/* Grab the alert mutex to access the read and write indexes */
	if (mutex_lock_interruptible(&domain_dev->alert_mutex))
		return -ERESTARTSYS;

	/* If there's no space for this notification, return */
	if ((domain_dev->alert_wr_idx - domain_dev->alert_rd_idx) ==
	    (HQMV2_DOMAIN_ALERT_RING_SIZE - 1)) {
		mutex_unlock(&domain_dev->alert_mutex);
		return 0;
	}

	alert.alert_id = alert_id;
	alert.aux_alert_data = aux_alert_data;

	domain_dev->alerts[domain_dev->alert_wr_idx++] = alert;

	mutex_unlock(&domain_dev->alert_mutex);

	/* Wake any blocked readers */
	wake_up_interruptible(&domain_dev->wq_head);

	return 0;
}

static inline size_t os_strlcpy(char *dst, const char *src, size_t sz)
{
	return strlcpy(dst, src, sz);
}

/**
 * HQMV2_BASE_ERR() - log an error message
 * @hqmv2: hqmv2_hw handle for a particular device.
 * @...: variable string args.
 */
#define HQMV2_BASE_ERR(hqmv2, ...) do {		     \
	struct hqmv2_dev *dev;			     \
	dev = container_of(hqmv2, struct hqmv2_dev, hw); \
	HQMV2_ERR(dev->hqmv2_device, __VA_ARGS__);	     \
} while (0)

/**
 * HQMV2_BASE_INFO() - log an info message
 * @hqmv2: hqmv2_hw handle for a particular device.
 * @...: variable string args.
 */
#define HQMV2_BASE_INFO(hqmv2, ...) do {		     \
	struct hqmv2_dev *dev;			     \
	dev = container_of(hqmv2, struct hqmv2_dev, hw); \
	HQMV2_INFO(dev->hqmv2_device, __VA_ARGS__);	     \
} while (0)

/*** Workqueue scheduling functions ***/

/* The workqueue callback runs until it completes all outstanding QID->CQ
 * map and unmap requests. To prevent deadlock, this function gives other
 * threads a chance to grab the resource mutex and configure hardware.
 */
static void hqmv2_complete_queue_map_unmap(struct work_struct *work)
{
	int delay_multiplier = 1;
	struct hqmv2_dev *hqmv2_dev;
	int ret;

	hqmv2_dev = container_of(work, struct hqmv2_dev, work);

	while (1) {
		mutex_lock(&hqmv2_dev->resource_mutex);

		ret = hqmv2_finish_unmap_qid_procedures(&hqmv2_dev->hw);
		ret += hqmv2_finish_map_qid_procedures(&hqmv2_dev->hw);

		if (ret != 0) {
			/* Release the mutex */
			mutex_unlock(&hqmv2_dev->resource_mutex);
			/* Relinquish the CPU so the application can process
			 * its CQs, so this function doesn't deadlock. Vary
			 * the delay from 10-100 us.
			 */
			udelay(10 * delay_multiplier);

			delay_multiplier = (delay_multiplier + 1) % 10;
		} else {
			break;
		}
	}

	hqmv2_dev->worker_launched = false;

	mutex_unlock(&hqmv2_dev->resource_mutex);
}

/**
 * os_schedule_work() - launch a thread to process pending map and unmap work
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function launches a kernel thread that will run until all pending
 * map and unmap procedures are complete.
 */
static inline void os_schedule_work(struct hqmv2_hw *hw)
{
	struct hqmv2_dev *hqmv2_dev;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	INIT_WORK(&hqmv2_dev->work, hqmv2_complete_queue_map_unmap);

	queue_work(hqmv2_dev->wq, &hqmv2_dev->work);

	hqmv2_dev->worker_launched = true;
}

/**
 * os_worker_active() - query whether the map/unmap worker thread is active
 * @hw: hqmv2_hw handle for a particular device.
 *
 * This function returns a boolean indicating whether a thread (launched by
 * os_schedule_work()) is active. This function is used to determine
 * whether or not to launch a worker thread.
 */
static inline bool os_worker_active(struct hqmv2_hw *hw)
{
	struct hqmv2_dev *hqmv2_dev;

	hqmv2_dev = container_of(hw, struct hqmv2_dev, hw);

	return hqmv2_dev->worker_launched;
}

#endif /*  __HQMV2_OSDEP_H */
