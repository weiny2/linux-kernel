// SPDX-License-Identifier: GPL-2.0 only
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#include "../common/mxlk_core.h"
#include "../common/mxlk_util.h"
#include "../common/mxlk_print.h"
#include "../common/mxlk_capabilities.h"
#include "mxlk_epf.h"
/* TODO remove this when host->device interrupt WA is not needed */
#include "mxlk_struct.h"

static struct mxlk *global_mxlk;

#define MXLK_CIRCULAR_INC(val, max) (((val) + 1) % (max))

static int rx_pool_size = 8 * 1024 * 1024;
module_param(rx_pool_size, int, 0664);
MODULE_PARM_DESC(rx_pool_size, "receive pool size (default 32 MiB)");

static int tx_pool_size = 8 * 1024 * 1024;
module_param(tx_pool_size, int, 0664);
MODULE_PARM_DESC(tx_pool_size, "transmit pool size (default 32 MiB)");

static int fragment_size = MXLK_FRAGMENT_SIZE;
module_param(fragment_size, int, 0664);
MODULE_PARM_DESC(fragment_size, "transfer descriptor size (default 128 KiB)");

static void mxlk_set_cap_txrx(struct mxlk *mxlk)
{
	struct mxlk_cap_txrx cap;
	uint32_t start = sizeof(struct mxlk_mmio);
	size_t hdr_len = sizeof(struct mxlk_cap_txrx);
	size_t tx_len = sizeof(struct mxlk_transfer_desc) * MXLK_NUM_TX_DESCS;
	size_t rx_len = sizeof(struct mxlk_transfer_desc) * MXLK_NUM_RX_DESCS;
	uint16_t next = (uint16_t)(start + hdr_len + tx_len + rx_len);

	memset(&cap, 0, sizeof(struct mxlk_cap_txrx));
	cap.hdr.id = MXLK_CAP_TXRX;
	cap.hdr.next = next;
	cap.fragment_size = fragment_size;
	cap.tx.ring = start + hdr_len;
	cap.tx.ndesc = MXLK_NUM_TX_DESCS;
	cap.rx.ring = start + hdr_len + tx_len;
	cap.rx.ndesc = MXLK_NUM_RX_DESCS;

	iowrite32(start, mxlk->mmio + MXLK_MMIO_CAPABILITIES);
	memcpy_toio(mxlk->mmio + start, &cap, sizeof(struct mxlk_cap_txrx));
	iowrite16(MXLK_CAP_NULL, mxlk->mmio + next);
}

static int mxlk_set_version(struct mxlk *mxlk)
{
	struct mxlk_version version;

	version.major = MXLK_VERSION_MAJOR;
	version.minor = MXLK_VERSION_MINOR;
	version.build = MXLK_VERSION_BUILD;

	memcpy_toio(mxlk->mmio + MXLK_MMIO_VERSION, &version, sizeof(version));

	mxlk_info("versions, device : %u.%u.%u\n", version.major,
		  version.minor, version.build);

	return 0;
}

static void mxlk_txrx_cleanup(struct mxlk *mxlk)
{
	int index;
	struct mxlk_transfer_desc *td;
	struct mxlk_stream *tx = &mxlk->tx;
	struct mxlk_stream *rx = &mxlk->rx;

	for (index = 0; index < rx->pipe.ndesc; index++) {
		td = rx->pipe.tdr + index;
		mxlk_set_td_address(td, 0);
		mxlk_set_td_length(td, 0);
	}
	for (index = 0; index < tx->pipe.ndesc; index++) {
		td = tx->pipe.tdr + index;
		mxlk_set_td_address(td, 0);
		mxlk_set_td_length(td, 0);
	}

	mxlk_list_cleanup(&mxlk->tx_pool);
	mxlk_list_cleanup(&mxlk->rx_pool);
}

/*
 * The RX/TX are named for Remote Host, in Local Host RX/TX is reversed.
 */
static int mxlk_txrx_init(struct mxlk *mxlk, struct mxlk_cap_txrx *cap)
{
	int index;
	int ndesc;
	struct mxlk_buf_desc *bd;
	struct mxlk_stream *tx = &mxlk->tx;
	struct mxlk_stream *rx = &mxlk->rx;

	mxlk->txrx = cap;
	mxlk->fragment_size = ioread32(&cap->fragment_size);

	rx->pipe.ndesc = ioread32(&cap->tx.ndesc);
	rx->pipe.head = &cap->tx.head;
	rx->pipe.tail = &cap->tx.tail;
	rx->pipe.tdr = mxlk->mmio + ioread32(&cap->tx.ring);

	tx->pipe.ndesc = ioread32(&cap->rx.ndesc);
	tx->pipe.head = &cap->rx.head;
	tx->pipe.tail = &cap->rx.tail;
	tx->pipe.tdr = mxlk->mmio + ioread32(&cap->rx.ring);

	mxlk_list_init(&mxlk->rx_pool);
	rx_pool_size = roundup(rx_pool_size, mxlk->fragment_size);
	ndesc = rx_pool_size / mxlk->fragment_size;

	for (index = 0; index < ndesc; index++) {
		bd = mxlk_alloc_bd(mxlk->fragment_size);
		if (bd) {
			mxlk_list_put(&mxlk->rx_pool, bd);
		} else {
			mxlk_error("failed to alloc all rx pool descriptors\n");
			goto error;
		}
	}

	mxlk_list_init(&mxlk->tx_pool);
	tx_pool_size = roundup(tx_pool_size, mxlk->fragment_size);
	ndesc = tx_pool_size / mxlk->fragment_size;

	for (index = 0; index < ndesc; index++) {
		bd = mxlk_alloc_bd(mxlk->fragment_size);
		if (bd) {
			mxlk_list_put(&mxlk->tx_pool, bd);
		} else {
			mxlk_error("failed to alloc all tx pool descriptors\n");
			goto error;
		}
	}

	return 0;

error:
	mxlk_txrx_cleanup(mxlk);

	return -ENOMEM;
}

static int mxlk_discover_txrx(struct mxlk *mxlk)
{
	int error;
	struct mxlk_cap_txrx *cap;

	cap = mxlk_cap_find(mxlk, 0, MXLK_CAP_TXRX);
	if (cap) {
		error = mxlk_txrx_init(mxlk, cap);
	} else {
		mxlk_error("mxlk txrx info not found\n");
		error = -EIO;
	}

	return error;
}

static void mxlk_start_tx(struct mxlk *mxlk, unsigned long delay)
{
	queue_delayed_work(mxlk->wq, &mxlk->tx_event, delay);
}

static void mxlk_start_rx(struct mxlk *mxlk, unsigned long delay)
{
	queue_delayed_work(mxlk->wq, &mxlk->rx_event, delay);
}

static void mxlk_rx_event_handler(struct work_struct *work)
{
	struct mxlk *mxlk = container_of(work, struct mxlk, rx_event.work);

	u16 interface;
	u32 head, tail, ndesc, length;
	u64 address;
	struct mxlk_stream *rx = &mxlk->rx;
	struct mxlk_buf_desc *bd;
	struct mxlk_transfer_desc *td;
	unsigned long delay = msecs_to_jiffies(1);
	bool reset_work = false;
	// TODO Remove this when host->device interrupt is available
	reset_work = true;
	if (mxlk_get_host_status(mxlk) != MXLK_STATUS_RUN)
		goto task_exit;

	ndesc = rx->pipe.ndesc;
	tail = mxlk_get_tdr_tail(&rx->pipe);
	head = mxlk_get_tdr_head(&rx->pipe);

	while (head != tail) {
		td = rx->pipe.tdr + head;

		bd = mxlk_alloc_rx_bd(mxlk);
		if (!bd) {
			delay = msecs_to_jiffies(20);
			reset_work = true;
			break;
		}

		interface = mxlk_get_td_interface(td);
		length = mxlk_get_td_length(td);
		address = mxlk_get_td_address(td);

		if (mxlk_copy_from_host(mxlk, bd->data, address, length) < 0) {
			mxlk_error("failed to copy from host\n");
			mxlk_free_rx_bd(mxlk, bd);
			delay = 0;
			reset_work = true;
			break;
		}

		bd->interface = interface;
		bd->length = length;
		bd->next = NULL;

		if (likely(interface < MXLK_NUM_INTERFACES)) {
			mxlk_set_td_status(td, MXLK_DESC_STATUS_SUCCESS);
			mxlk_add_bd_to_interface(mxlk, bd);
		} else {
			mxlk_error("detected rx desc interface failure (%u)\n", interface);
			mxlk_set_td_status(td, MXLK_DESC_STATUS_ERROR);
			mxlk_free_rx_bd(mxlk, bd);
		}

		head = MXLK_CIRCULAR_INC(head, ndesc);
	}

	if (mxlk_get_tdr_head(&rx->pipe) != head) {
		mxlk_set_tdr_head(&rx->pipe, head);
		mxlk_set_doorbell(mxlk, FROM_DEVICE, DATA_RECEIVED);
		mxlk_raise_irq(mxlk, MXLK_DMA_TODEVICE_DONE);
	}

task_exit:
	if (reset_work)
		mxlk_start_rx(mxlk, delay);
}

static void mxlk_tx_event_handler(struct work_struct *work)
{
	struct mxlk *mxlk = container_of(work, struct mxlk, tx_event.work);

	int error;
	u32 head, tail, ndesc;
	u64 address;
	struct mxlk_stream *tx = &mxlk->tx;
	struct mxlk_buf_desc *bd;
	struct mxlk_transfer_desc *td;
	size_t bytes, buffers;

	if (mxlk_get_host_status(mxlk) != MXLK_STATUS_RUN)
		return;

	ndesc = tx->pipe.ndesc;
	tail = mxlk_get_tdr_tail(&tx->pipe);
	head = mxlk_get_tdr_head(&tx->pipe);

	// add new entries
	while (MXLK_CIRCULAR_INC(tail, ndesc) != head) {
		bd = mxlk_list_get(&mxlk->write);
		if (!bd)
			break;

		td = tx->pipe.tdr + tail;
		address = mxlk_get_td_address(td);

		error = mxlk_copy_to_host(mxlk, address, bd->data, bd->length);
		if (error < 0) {
			mxlk_error("failed to copy to host\n");
			mxlk_set_td_status(td, MXLK_DESC_STATUS_ERROR);
		} else {
			mxlk_set_td_status(td, MXLK_DESC_STATUS_SUCCESS);
		}

		mxlk_set_td_length(td, bd->length);
		mxlk_set_td_interface(td, bd->interface);
		mxlk_free_tx_bd(mxlk, bd);

		tail = MXLK_CIRCULAR_INC(tail, ndesc);
	}

	if (mxlk_get_tdr_tail(&tx->pipe) != tail) {
		mxlk_set_tdr_tail(&tx->pipe, tail);
		mxlk_set_doorbell(mxlk, FROM_DEVICE, DATA_SENT);
		mxlk_raise_irq(mxlk, MXLK_DMA_FROMDEVICE_DONE);
	}

	mxlk_list_info(&mxlk->write, &bytes, &buffers);
	if (buffers)
		mxlk->tx_pending = true;
	else
		mxlk->tx_pending = false;
}

/*
 * TODO
 * This implementation is workaround. Change when HW is ready
 */
static irqreturn_t mxlk_host_interrupt(int irq, void *args)
{
	struct mxlk *mxlk = args;
	struct mxlk_epf *mxlk_epf = container_of(mxlk, struct mxlk_epf, mxlk);
	//int int_en;

	/* Disable interrupt */
	//int_en = readl(mxlk_epf->apb_base + 0x18);
	//writel(int_en & ~BIT(18), mxlk_epf->apb_base + 0x18);

	/* Handle interrupt if flag is set */
	if (readl(mxlk_epf->apb_base + 0x1c) & BIT(18)) {
		writel(BIT(18), mxlk_epf->apb_base + 0x1c);
		if (mxlk_get_doorbell(mxlk, TO_DEVICE, DATA_SENT)) {
			mxlk_clear_doorbell(mxlk, TO_DEVICE, DATA_SENT);
			mxlk_start_rx(mxlk, 0);
		}
		if (mxlk_get_doorbell(mxlk, TO_DEVICE, DATA_RECEIVED)) {
			mxlk_clear_doorbell(mxlk, TO_DEVICE, DATA_RECEIVED);
			if (mxlk->tx_pending)
				mxlk_start_tx(mxlk, 0);
		}
	}

	/* Enable interrupt */
	//writel(int_en & BIT(18), mxlk_epf->apb_base + 0x18);

	return IRQ_HANDLED;
}

static int mxlk_events_init(struct mxlk *mxlk)
{
	mxlk->wq = alloc_ordered_workqueue(MXLK_DRIVER_NAME,
					   WQ_MEM_RECLAIM | WQ_HIGHPRI);
	if (!mxlk->wq) {
		mxlk_error("failed to allocate workqueue\n");
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&mxlk->rx_event, mxlk_rx_event_handler);
	INIT_DELAYED_WORK(&mxlk->tx_event, mxlk_tx_event_handler);

	return 0;
}

static void mxlk_events_cleanup(struct mxlk *mxlk)
{
	cancel_delayed_work_sync(&mxlk->rx_event);
	cancel_delayed_work_sync(&mxlk->tx_event);

	destroy_workqueue(mxlk->wq);
}

int mxlk_core_init(struct mxlk *mxlk)
{
	int error;

	global_mxlk = mxlk;

	mxlk_set_version(mxlk);
	mxlk_set_cap_txrx(mxlk);

	error = mxlk_events_init(mxlk);
	if (error)
		return error;

	error = mxlk_discover_txrx(mxlk);
	if (error)
		goto error_stream;

	error = mxlk_interfaces_init(mxlk);
	if (error)
		goto error_interfaces;
#if 0
	error = mxlk_register_host_irq(mxlk, &mxlk_host_interrupt, 0);
	if (error) {
		mxlk_error("failed to request irq\n");
		goto error_interfaces;
	}
#endif
	mxlk_set_device_status(mxlk, MXLK_STATUS_RUN);
        /*TBD*/
        mxlk_start_rx(mxlk, 0);
	return 0;

error_interfaces:
	mxlk_txrx_cleanup(mxlk);

error_stream:
	mxlk_events_cleanup(mxlk);

	mxlk_set_device_status(mxlk, MXLK_STATUS_ERROR);

	return error;
}

void mxlk_core_cleanup(struct mxlk *mxlk)
{
	if (mxlk->status == MXLK_STATUS_RUN) {
		mxlk_set_device_status(mxlk, MXLK_STATUS_READY);
		mxlk_events_cleanup(mxlk);
		mxlk_interfaces_cleanup(mxlk);
		mxlk_txrx_cleanup(mxlk);
	}
}

int mxlk_core_read(struct mxlk *mxlk, void *buffer, size_t *length,
		   unsigned int timeout_ms)
{
	int ret = 0;
	struct mxlk_interface *inf = &mxlk->interfaces[0];
	size_t len = *length;
	size_t remaining = len;
	struct mxlk_buf_desc *bd;
	unsigned long jiffies_start = jiffies;
	long jiffies_passed = 0;
	long jiffies_timeout = (long)msecs_to_jiffies(timeout_ms);

	*length = 0;
	if (len == 0)
		return -EINVAL;

	ret = mutex_lock_interruptible(&inf->rlock);
	if (ret < 0)
		return -EINTR;

	do {
		while (!inf->data_available) {
			mutex_unlock(&inf->rlock);
			if (timeout_ms == 0) {
				ret = wait_event_interruptible(inf->rx_waitqueue,
							       inf->data_available);
			} else {
				ret = wait_event_interruptible_timeout(
					inf->rx_waitqueue, inf->data_available,
					jiffies_timeout - jiffies_passed);
				if (ret == 0)
					return -ETIME;
			}
			if (ret < 0)
				return -EINTR;

			ret = mutex_lock_interruptible(&inf->rlock);
			if (ret < 0)
				return -EINTR;
		}

		bd = (inf->partial_read) ? inf->partial_read :
					   mxlk_list_get(&inf->read);

		while (remaining && bd) {
			size_t bcopy;

			bcopy = min(remaining, bd->length);
			memcpy(buffer, bd->data, bcopy);

			buffer += bcopy;
			remaining -= bcopy;
			bd->data += bcopy;
			bd->length -= bcopy;

			if (bd->length == 0) {
				mxlk_free_rx_bd(mxlk, bd);
				bd = mxlk_list_get(&inf->read);
			}
		}

		// save for next time
		inf->partial_read = bd;

		if (!bd)
			inf->data_available = false;

		*length = len - remaining;

		jiffies_passed = (long)jiffies - (long)jiffies_start;
	} while (remaining > 0 && (jiffies_passed < jiffies_timeout ||
				   timeout_ms == 0));

	mutex_unlock(&inf->rlock);

	return 0;
}

int mxlk_core_write(struct mxlk *mxlk, void *buffer, size_t *length,
		    unsigned int timeout_ms)
{
	int ret;
	size_t len = *length;
	size_t remaining = len;
	struct mxlk_interface *inf = &mxlk->interfaces[0];
	struct mxlk_buf_desc *bd, *head;
	unsigned long jiffies_start = jiffies;
	long jiffies_passed = 0;
	long jiffies_timeout = (long)msecs_to_jiffies(timeout_ms);

	*length = 0;
	if (len == 0)
		return -EINVAL;

	ret = mutex_lock_interruptible(&mxlk->wlock);
	if (ret < 0)
		return -EINTR;

	do {
		bd = head = mxlk_alloc_tx_bd(mxlk);
		while (!head) {
			mutex_unlock(&mxlk->wlock);
			if (timeout_ms == 0) {
				ret = wait_event_interruptible(mxlk->tx_waitqueue,
							       !mxlk->no_tx_buffer);
			} else {
				ret = wait_event_interruptible_timeout(
					mxlk->tx_waitqueue, !mxlk->no_tx_buffer,
					jiffies_timeout - jiffies_passed);
				if (ret == 0)
					return -ETIME;
			}
			if (ret < 0)
				return -EINTR;

			ret = mutex_lock_interruptible(&mxlk->wlock);
			if (ret < 0)
				return -EINTR;

			bd = head = mxlk_alloc_tx_bd(mxlk);
		}

		while (remaining && bd) {
			size_t bcopy;

			bcopy = min(bd->length, remaining);
			memcpy(bd->data, buffer, bcopy);

			buffer += bcopy;
			remaining -= bcopy;
			bd->length = bcopy;
			bd->interface = inf->id;

			if (remaining) {
				bd->next = mxlk_alloc_tx_bd(mxlk);
				bd = bd->next;
			}
		}

		mxlk_list_put(&inf->mxlk->write, head);
		mxlk_start_tx(mxlk, 0);

		*length = len - remaining;

		jiffies_passed = (long)jiffies - (long)jiffies_start;
	} while (remaining > 0 && (jiffies_passed < jiffies_timeout ||
				   timeout_ms == 0));

	mutex_unlock(&mxlk->wlock);

	return 0;
}

struct mxlk *mxlk_core_get_default(void)
{
	return global_mxlk;
}

void mxlk_core_set_default(struct mxlk *mxlk)
{
	global_mxlk = mxlk;
}
