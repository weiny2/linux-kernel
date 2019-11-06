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
#include "../common/mxlk_mmio.h"
#include "../common/mxlk_print.h"
#include "../common/mxlk_capabilities.h"
#include "mxlk_epf.h"
/* TODO remove this when host->device interrupt WA is not needed */
#include "mxlk_struct.h"

static struct mxlk_interface *mxlk_inf;

#define MXLK_CIRCULAR_INC(val, max) (((val) + 1) % (max))

static int rx_pool_size = 8 * 1024 * 1024;
module_param(rx_pool_size, int, 0664);
MODULE_PARM_DESC(rx_pool_size, "receive pool size (default 8MB)");

static int tx_pool_size = 8 * 1024 * 1024;
module_param(tx_pool_size, int, 0664);
MODULE_PARM_DESC(tx_pool_size, "transmit pool size (default 8MB)");

static int mxlk_set_version(struct mxlk *mxlk)
{
	struct mxlk_version version;

	version.major = MXLK_VERSION_MAJOR;
	version.minor = MXLK_VERSION_MINOR;
	version.build = MXLK_VERSION_BUILD;

	mxlk_wr_buffer(mxlk->mmio, MXLK_MMIO_VERSION, &version,
		       sizeof(version));

	mxlk_info("versions, dev : %d.%d.%d, host : %d.%d.%d\n", version.major,
		  version.minor, version.build, MXLK_VERSION_MAJOR,
		  MXLK_VERSION_MINOR, MXLK_VERSION_BUILD);

	return 0;
}

static void mxlk_set_device_status(struct mxlk *mxlk, int status)
{
	mxlk->status = status;
	mxlk_wr32(mxlk->mmio, MXLK_MMIO_DEV_STATUS, status);
}

static void mxlk_set_doorbell_status(struct mxlk *mxlk, int status)
{
	mxlk_wr32(mxlk->mmio, MXLK_MMIO_DOORBELL_STATUS, status);
}

static u32 mxlk_get_doorbell_status(struct mxlk *mxlk)
{
	return mxlk_rd32(mxlk->mmio, MXLK_MMIO_DOORBELL_STATUS);
}

static u32 mxlk_get_host_status(struct mxlk *mxlk)
{
	return mxlk_rd32(mxlk->mmio, MXLK_MMIO_HOST_STATUS);
}

static struct mxlk_buf_desc *mxlk_alloc_bd(size_t length)
{
	struct mxlk_buf_desc *bd;

	bd = kzalloc(sizeof(*bd), GFP_KERNEL);
	if (!bd)
		return NULL;

	bd->head = kzalloc(roundup(length, cache_line_size()), GFP_KERNEL);
	if (!bd->head) {
		kfree(bd);
		return NULL;
	}

	bd->data = bd->head;
	bd->length = bd->true_len = length;
	bd->next = NULL;

	return bd;
}

static void mxlk_free_bd(struct mxlk_buf_desc *bd)
{
	if (bd) {
		kfree(bd->head);
		kfree(bd);
	}
}

static int mxlk_list_init(struct mxlk_list *list)
{
	spin_lock_init(&list->lock);
	list->bytes = 0;
	list->buffers = 0;
	list->head = NULL;
	list->tail = NULL;

	return 0;
}

static void mxlk_list_cleanup(struct mxlk_list *list)
{
	struct mxlk_buf_desc *bd;

	spin_lock(&list->lock);
	while (list->head) {
		bd = list->head;
		list->head = bd->next;
		mxlk_free_bd(bd);
	}

	list->head = list->tail = NULL;
	spin_unlock(&list->lock);
}

static int mxlk_list_put(struct mxlk_list *list, struct mxlk_buf_desc *bd)
{
	if (!bd) {
		mxlk_error("attempt to add null buf desc to list!\n");
		return -EINVAL;
	}

	spin_lock(&list->lock);
	if (list->head)
		list->tail->next = bd;
	else
		list->head = bd;
	while (bd) {
		list->tail = bd;
		list->bytes += bd->length;
		list->buffers++;
		bd = bd->next;
	}
	spin_unlock(&list->lock);

	return 0;
}

static struct mxlk_buf_desc *mxlk_list_get(struct mxlk_list *list)
{
	struct mxlk_buf_desc *bd;

	spin_lock(&list->lock);
	bd = list->head;
	if (list->head) {
		list->head = list->head->next;
		if (!list->head)
			list->tail = NULL;
		bd->next = NULL;
		list->bytes -= bd->length;
		list->buffers--;
	}
	spin_unlock(&list->lock);

	return bd;
}

static void mxlk_list_info(struct mxlk_list *list, size_t *bytes,
			   size_t *buffers)
{
	spin_lock(&list->lock);
	*bytes = list->bytes;
	*buffers = list->buffers;
	spin_unlock(&list->lock);
}

static struct mxlk_buf_desc *mxlk_alloc_rx_bd(struct mxlk *mxlk)
{
	struct mxlk_buf_desc *bd;

	bd = mxlk_list_get(&mxlk->rx_pool);
	if (bd) {
		bd->data = bd->head;
		bd->length = bd->true_len;
		bd->next = NULL;
		bd->interface = 0;
	}

	return bd;
}

static void mxlk_free_rx_bd(struct mxlk *mxlk, struct mxlk_buf_desc *bd)
{
	if (bd)
		mxlk_list_put(&mxlk->rx_pool, bd);
}

static struct mxlk_buf_desc *mxlk_alloc_tx_bd(struct mxlk *mxlk)
{
	struct mxlk_buf_desc *bd;

	bd = mxlk_list_get(&mxlk->tx_pool);
	if (bd) {
		bd->data = bd->head;
		bd->length = bd->true_len;
		bd->next = NULL;
		bd->interface = 0;
	} else {
		mxlk->no_tx_buffer = true;
	}

	return bd;
}

static void mxlk_free_tx_bd(struct mxlk *mxlk, struct mxlk_buf_desc *bd)
{
	if (!bd)
		return;

	mxlk_list_put(&mxlk->tx_pool, bd);

	mutex_lock(&mxlk->wlock);
	mxlk->no_tx_buffer = false;
	mutex_unlock(&mxlk->wlock);

	wake_up_interruptible(&mxlk->tx_waitqueue);
}

static int mxlk_interface_init(struct mxlk *mxlk, int id)
{
	struct mxlk_interface *inf = mxlk->interfaces + id;

	inf->id = id;
	inf->mxlk = mxlk;
	inf->opened = 0;

	inf->partial_read = NULL;
	mxlk_list_init(&inf->read);
	mutex_init(&inf->rlock);
	inf->data_available = false;
	init_waitqueue_head(&inf->rx_waitqueue);

	mxlk_inf = inf;

	return 0;
}

static void mxlk_interface_cleanup(struct mxlk_interface *inf)
{
	struct mxlk_buf_desc *bd;

	inf->opened = 0;

	mutex_destroy(&inf->rlock);

	mxlk_free_rx_bd(inf->mxlk, inf->partial_read);
	while ((bd = mxlk_list_get(&inf->read)))
		mxlk_free_rx_bd(inf->mxlk, bd);
}

static void mxlk_interfaces_cleanup(struct mxlk *mxlk)
{
	int index;

	for (index = 0; index < MXLK_NUM_INTERFACES; index++)
		mxlk_interface_cleanup(mxlk->interfaces + index);

	mxlk_list_cleanup(&mxlk->write);
	mutex_destroy(&mxlk->wlock);
}

static int mxlk_interfaces_init(struct mxlk *mxlk)
{
	int index;
	int error;

	mutex_init(&mxlk->wlock);
	mxlk_list_init(&mxlk->write);
	init_waitqueue_head(&mxlk->tx_waitqueue);
	mxlk->no_tx_buffer = false;

	for (index = 0; index < MXLK_NUM_INTERFACES; index++) {
		error = mxlk_interface_init(mxlk, index);
		if (error < 0)
			goto cleanup;
	}

	return 0;

cleanup:
	mxlk_interfaces_cleanup(mxlk);
	return error;
}

static void mxlk_add_bd_to_interface(struct mxlk *mxlk,
				     struct mxlk_buf_desc *bd)
{
	struct mxlk_interface *inf;

	inf = mxlk->interfaces + bd->interface;

	mxlk_list_put(&inf->read, bd);

	mutex_lock(&inf->rlock);
	inf->data_available = true;
	mutex_unlock(&inf->rlock);
	wake_up_interruptible(&inf->rx_waitqueue);
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
	mxlk->fragment_size = mxlk_rd32(&cap->fragment_size, 0);

	rx->busy = 0;
	rx->pipe.ndesc = mxlk_rd32(&cap->tx.ndesc, 0);
	rx->pipe.head = &cap->tx.head;
	rx->pipe.tail = &cap->tx.tail;
	rx->pipe.old = mxlk_rd32(&cap->tx.tail, 0);
	rx->pipe.tdr = mxlk->mmio + mxlk_rd32(&cap->tx.ring, 0);

	tx->busy = 0;
	tx->pipe.ndesc = mxlk_rd32(&cap->rx.ndesc, 0);
	tx->pipe.head = &cap->rx.head;
	tx->pipe.tail = &cap->rx.tail;
	tx->pipe.old = mxlk_rd32(&cap->tx.head, 0);
	tx->pipe.tdr = mxlk->mmio + mxlk_rd32(&cap->rx.ring, 0);

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

	return -1;
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

	if (mxlk_get_host_status(mxlk) != MXLK_STATUS_RUN)
		goto task_exit;

	mxlk->stats.rx_event_runs++;

	ndesc = rx->pipe.ndesc;
	tail = mxlk_get_tdr_tail(&rx->pipe);
	head = mxlk_get_tdr_head(&rx->pipe);
	// clean old entries first
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
			mxlk_free_rx_bd(mxlk, bd);
			delay = 0;
			reset_work = true;
			break;
		}

		bd->interface = interface;
		bd->length = length;
		bd->next = NULL;

		if (likely(interface < MXLK_NUM_INTERFACES)) {
			mxlk->stats.rx_krn.pkts++;
			mxlk->stats.rx_krn.bytes += bd->length;
			mxlk_set_td_status(td, MXLK_DESC_STATUS_SUCCESS);
			mxlk_add_bd_to_interface(mxlk, bd);
		} else {
			mxlk_set_td_status(td, MXLK_DESC_STATUS_ERROR);
			mxlk_free_rx_bd(mxlk, bd);
		}

		head = MXLK_CIRCULAR_INC(head, ndesc);
	}

	if (mxlk_get_tdr_head(&rx->pipe) != head)
		mxlk_set_tdr_head(&rx->pipe, head);

task_exit:
	if (reset_work) {
		mxlk_start_rx(mxlk, delay);
	}
}

static void mxlk_tx_event_handler(struct work_struct *work)
{
	struct mxlk *mxlk = container_of(work, struct mxlk, tx_event.work);

	u32 head, tail, ndesc;
	u64 address;
	struct mxlk_stream *tx = &mxlk->tx;
	struct mxlk_buf_desc *bd;
	struct mxlk_transfer_desc *td;

	if (mxlk_get_host_status(mxlk) != MXLK_STATUS_RUN)
		return;

	mxlk->stats.tx_event_runs++;

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

		if (mxlk_copy_to_host(mxlk, address, bd->data, bd->length) <
		    0) {
			mxlk_free_tx_bd(mxlk, bd);
			mxlk_start_tx(mxlk, 0);
			break;
		}

		mxlk_set_td_length(td, bd->length);
		mxlk_set_td_interface(td, bd->interface);
		mxlk_set_td_status(td, MXLK_DESC_STATUS_SUCCESS);

		mxlk->stats.tx_krn.pkts++;
		mxlk->stats.tx_krn.bytes += bd->length;
		mxlk_free_tx_bd(mxlk, bd);

		tail = MXLK_CIRCULAR_INC(tail, ndesc);
	}

	if (mxlk_get_tdr_tail(&tx->pipe) != tail) {
		mxlk_set_tdr_tail(&tx->pipe, tail);
		mxlk_raise_irq(mxlk, MXLK_DMA_FROMDEVICE_DONE);
	}

	if (tail != head)
		mxlk_start_tx(mxlk, 0);
}

/*
 * TODO
 * This implementation is workaround. Change when HW is ready
 */
static irqreturn_t mxlk_host_interrupt(int irq, void *args)
{
	struct mxlk *mxlk = args;
	int int_en;
	u32 status;
	struct mxlk_epf *mxlk_epf = container_of(mxlk, struct mxlk_epf, mxlk);

	/* Disable interrupt */
	int_en = readl(mxlk_epf->apb_base + 0x18);
	writel(int_en & ~BIT(18), mxlk_epf->apb_base + 0x18);

	/* Handle interrupt if flag is set */
	if (readl(mxlk_epf->apb_base + 0x1c) & BIT(18)) {
		writel(BIT(18), mxlk_epf->apb_base + 0x1c);
		status = mxlk_get_doorbell_status(mxlk);
		if (status == MXLK_DOORBELL_SEND) {
			mxlk->stats.interrupts++;
			mxlk_set_doorbell_status(mxlk, MXLK_DOORBELL_NONE);
			mxlk_start_rx(mxlk, 0);
		}
	}

	/* Enable interrupt */
	writel(int_en & BIT(18), mxlk_epf->apb_base + 0x18);

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

	scnprintf(mxlk->name, MXLK_MAX_NAME_LEN, MXLK_DRIVER_NAME);

	mxlk_set_version(mxlk);
	mxlk_set_cap(mxlk);

	error = mxlk_events_init(mxlk);
	if (error)
		goto error_events;

	error = mxlk_discover_txrx(mxlk);
	if (error)
		goto error_stream;

	error = mxlk_interfaces_init(mxlk);
	if (error)
		goto error_interfaces;

	mxlk_set_device_status(mxlk, MXLK_STATUS_RUN);

	error = mxlk_register_host_irq(mxlk, &mxlk_host_interrupt, 0);
	if (error) {
		mxlk_error("failed to request irq\n");
		goto error_events;
	}

	return 0;

error_interfaces:
	mxlk_txrx_cleanup(mxlk);

error_stream:
	mxlk_events_cleanup(mxlk);

error_events:
	mxlk_error("core failed to init\n");

	return error;
}

void mxlk_core_cleanup(struct mxlk *mxlk)
{
	mxlk_set_device_status(mxlk, MXLK_STATUS_UNINIT);

	mxlk_events_cleanup(mxlk);

	mxlk_interfaces_cleanup(mxlk);
	mxlk_txrx_cleanup(mxlk);
}

int mxlk_core_open(struct mxlk_interface *inf)
{
	if (inf->opened)
		return -EACCES;

	inf->opened = 1;

	return 0;
}

int mxlk_core_close(struct mxlk_interface *inf)
{
	if (inf->opened)
		inf->opened = 0;

	return 0;
}

ssize_t mxlk_core_read(struct mxlk_interface *inf, void *buffer, size_t *length,
		       unsigned int timeout_ms)
{
	int ret = 0;
	struct mxlk *mxlk = inf->mxlk;
	size_t len = *length;
	size_t remaining = len;
	struct mxlk_buf_desc *bd;
	unsigned long jiffies_start = jiffies;
	unsigned long jiffies_passed = 0;
	unsigned long jiffies_timeout = msecs_to_jiffies(timeout_ms);

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

			mxlk->stats.rx_usr.bytes += bcopy;
			if (bd->length == 0) {
				mxlk->stats.rx_usr.pkts++;
				mxlk_free_rx_bd(mxlk, bd);
				bd = mxlk_list_get(&inf->read);
			}
		}

		// save for next time
		inf->partial_read = bd;

		if (!bd)
			inf->data_available = false;

		*length = len - remaining;

		jiffies_passed = jiffies - jiffies_start;
	} while (remaining > 0 && (jiffies_passed < jiffies_timeout ||
				   timeout_ms == 0));

	mutex_unlock(&inf->rlock);

	return remaining;
}

ssize_t mxlk_core_write(struct mxlk_interface *inf, void *buffer, size_t length,
			unsigned int timeout_ms)
{
	int ret;
	size_t remaining = length;
	struct mxlk *mxlk = inf->mxlk;
	struct mxlk_buf_desc *bd, *head;
	unsigned long jiffies_start = jiffies;
	unsigned long jiffies_passed = 0;
	unsigned long jiffies_timeout = msecs_to_jiffies(timeout_ms);

	if (length == 0)
		return -EINVAL;

	ret = mutex_lock_interruptible(&mxlk->wlock);
	if (ret < 0)
		return -EINTR;

	do {
		bd = head = mxlk_alloc_tx_bd(mxlk);
		while (mxlk->no_tx_buffer) {
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

			mxlk->stats.tx_usr.pkts++;
			mxlk->stats.tx_usr.bytes += bcopy;

			if (remaining) {
				bd->next = mxlk_alloc_tx_bd(mxlk);
				bd = bd->next;
			}
		}

		if (head) {
			mxlk_list_put(&inf->mxlk->write, head);
			mxlk_start_tx(mxlk, 0);
		}

		jiffies_passed = jiffies - jiffies_start;
	} while (remaining > 0 && (jiffies_passed < jiffies_timeout ||
				   timeout_ms == 0));

	mutex_unlock(&mxlk->wlock);

	return remaining;
}

struct mxlk_interface *mxlk_core_default_interface()
{
	return mxlk_inf;
}
