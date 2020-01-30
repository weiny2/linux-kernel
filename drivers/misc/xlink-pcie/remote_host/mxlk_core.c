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

#include "mxlk_pci.h"
#include "../common/mxlk_core.h"
#include "../common/mxlk_util.h"
#include "../common/mxlk_print.h"
#include "../common/mxlk_capabilities.h"

#define MXLK_CIRCULAR_INC(val, max) (((val) + 1) % (max))

static int rx_pool_size = 8 * 1024 * 1024;
module_param(rx_pool_size, int, 0664);
MODULE_PARM_DESC(rx_pool_size, "receive pool size (default 8MB)");

static int tx_pool_size = 8 * 1024 * 1024;
module_param(tx_pool_size, int, 0664);
MODULE_PARM_DESC(tx_pool_size, "transmit pool size (default 8MB)");

static int mxlk_version_check(struct mxlk *mxlk)
{
	struct mxlk_version version;

	memcpy_fromio(&version, mxlk->mmio + MXLK_MMIO_VERSION,
		      sizeof(version));

	mxlk_info("versions, device : %u.%u.%u, host : %u.%u.%u\n",
		  version.major, version.minor, version.build,
		  MXLK_VERSION_MAJOR, MXLK_VERSION_MINOR, MXLK_VERSION_BUILD);

	return 0;
}

static int mxlk_map_dma(struct mxlk *mxlk, struct mxlk_dma_desc *dd,
			int direction)
{
	struct mxlk_pcie *xdev = container_of(mxlk, struct mxlk_pcie, mxlk);
	struct device *dev = &xdev->pci->dev;

	dd->phys = dma_map_single(dev, dd->bd->data, dd->bd->length, direction);
	dd->length = dd->bd->length;

	return dma_mapping_error(dev, dd->phys);
}

static void mxlk_unmap_dma(struct mxlk *mxlk, struct mxlk_dma_desc *dd,
			   int direction)
{
	struct mxlk_pcie *xdev = container_of(mxlk, struct mxlk_pcie, mxlk);
	struct device *dev = &xdev->pci->dev;
	dma_unmap_single(dev, dd->phys, dd->length, direction);
}

static void mxlk_txrx_cleanup(struct mxlk *mxlk)
{
	int index;
	struct mxlk_dma_desc *dd;
	struct mxlk_stream *tx = &mxlk->tx;
	struct mxlk_stream *rx = &mxlk->rx;

	if (tx->ddr) {
		for (index = 0; index < tx->pipe.ndesc; index++) {
			dd = tx->ddr + index;
			if (dd->bd) {
				mxlk_unmap_dma(mxlk, dd, DMA_TO_DEVICE);
				mxlk_free_tx_bd(mxlk, dd->bd);
			}
		}
		kfree(tx->ddr);
	}

	if (rx->ddr) {
		for (index = 0; index < rx->pipe.ndesc; index++) {
			struct mxlk_transfer_desc *td = rx->pipe.tdr + index;

			dd = rx->ddr + index;
			if (dd->bd) {
				mxlk_unmap_dma(mxlk, dd, DMA_FROM_DEVICE);
				mxlk_free_rx_bd(mxlk, dd->bd);
				mxlk_set_td_address(td, 0);
				mxlk_set_td_length(td, 0);
			}
		}
		kfree(rx->ddr);
	}

	mxlk_list_cleanup(&mxlk->tx_pool);
	mxlk_list_cleanup(&mxlk->rx_pool);
}

static int mxlk_txrx_init(struct mxlk *mxlk, struct mxlk_cap_txrx *cap)
{
	int index;
	int ndesc;
	struct mxlk_buf_desc *bd;
	struct mxlk_stream *tx = &mxlk->tx;
	struct mxlk_stream *rx = &mxlk->rx;

	mxlk->txrx = cap;
	mxlk->fragment_size = ioread32(&cap->fragment_size);

	tx->pipe.ndesc = ioread32(&cap->tx.ndesc);
	tx->pipe.head = &cap->tx.head;
	tx->pipe.tail = &cap->tx.tail;
	tx->pipe.old = ioread32(&cap->tx.tail);
	tx->pipe.tdr = mxlk->mmio + ioread32(&cap->tx.ring);

	tx->ddr = kcalloc(tx->pipe.ndesc, sizeof(struct mxlk_dma_desc),
			  GFP_KERNEL);
	if (!tx->ddr) {
		mxlk_error("failed to alloc tx dma desc ring\n");
		goto error;
	}

	rx->pipe.ndesc = ioread32(&cap->rx.ndesc);
	rx->pipe.head = &cap->rx.head;
	rx->pipe.tail = &cap->rx.tail;
	rx->pipe.old = ioread32(&cap->rx.head);
	rx->pipe.tdr = mxlk->mmio + ioread32(&cap->rx.ring);

	rx->ddr = kcalloc(rx->pipe.ndesc, sizeof(struct mxlk_dma_desc),
			  GFP_KERNEL);
	if (!rx->ddr) {
		mxlk_error("failed to alloc rx dma desc ring\n");
		goto error;
	}

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

	for (index = 0; index < rx->pipe.ndesc; index++) {
		struct mxlk_dma_desc *dd = rx->ddr + index;
		struct mxlk_transfer_desc *td = rx->pipe.tdr + index;

		bd = mxlk_alloc_rx_bd(mxlk);
		if (!bd) {
			mxlk_error("failed to alloc rx ring buf desc [%d]\n",
				   index);
			goto error;
		}

		dd->bd = bd;
		if (mxlk_map_dma(mxlk, dd, DMA_FROM_DEVICE)) {
			mxlk_error("failed to map rx bd\n");
			goto error;
		}

		mxlk_set_td_address(td, dd->phys);
		mxlk_set_td_length(td, dd->length);
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
	if (cap)
		error = mxlk_txrx_init(mxlk, cap);
	else
		error = -EIO;

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

	int error;
	/* TODO Remove when no need to workaround for host->device interrupt */
	u16 vendor_id;
	struct mxlk_pcie *xdev = container_of(mxlk, struct mxlk_pcie, mxlk);
	u16 status, interface;
	u32 head, tail, ndesc, length;
	struct mxlk_stream *rx = &mxlk->rx;
	struct mxlk_buf_desc *bd, *replacement = NULL;
	struct mxlk_dma_desc *dd;
	struct mxlk_dma_desc temp_dd;
	struct mxlk_transfer_desc *td;
	unsigned long delay = msecs_to_jiffies(1);

	ndesc = rx->pipe.ndesc;
	tail = mxlk_get_tdr_tail(&rx->pipe);
	head = mxlk_get_tdr_head(&rx->pipe);
	while (head != tail) {
		td = rx->pipe.tdr + head;
		dd = rx->ddr + head;
		temp_dd = *dd;

		replacement = mxlk_alloc_rx_bd(mxlk);
		if (!replacement) {
			delay = msecs_to_jiffies(20);
			break;
		}

		temp_dd.bd = replacement;
		error = mxlk_map_dma(mxlk, &temp_dd, DMA_FROM_DEVICE);
		if (error) {
			mxlk_error("failed to map rx bd (%d)\n", error);
			mxlk_free_rx_bd(mxlk, replacement);
			continue;
		}

		status = mxlk_get_td_status(td);
		interface = mxlk_get_td_interface(td);
		length = mxlk_get_td_length(td);
		mxlk_unmap_dma(mxlk, dd, DMA_FROM_DEVICE);

		if (unlikely(status != MXLK_DESC_STATUS_SUCCESS) ||
		    unlikely(interface >= MXLK_NUM_INTERFACES)) {
			mxlk_error("detected rx desc failure, status(%u), interface(%u)\n",
				   status, interface);
			mxlk_free_rx_bd(mxlk, dd->bd);
		} else {
			bd = dd->bd;
			bd->interface = interface;
			bd->length = length;
			bd->next = NULL;

			mxlk_add_bd_to_interface(mxlk, bd);
		}

		*dd = temp_dd;

		mxlk_set_td_address(td, dd->phys);
		mxlk_set_td_length(td, dd->length);
		head = MXLK_CIRCULAR_INC(head, ndesc);
	}

	if (mxlk_get_tdr_head(&rx->pipe) != head) {
		mxlk_set_tdr_head(&rx->pipe, head);
		mxlk_set_doorbell(mxlk, TO_DEVICE, DATA_RECEIVED);
		/*
		 * TODO
		 * change when no need to workaround for host->device interrupt
		 */
		pci_read_config_word(xdev->pci, PCI_VENDOR_ID, &vendor_id);
	}

	if (!replacement)
		mxlk_start_rx(mxlk, delay);
}

static void mxlk_tx_event_handler(struct work_struct *work)
{
	struct mxlk *mxlk = container_of(work, struct mxlk, tx_event.work);

	u16 status;
	/* TODO Remove when no need to workaround for host->device interrupt */
	u16 vendor_id;
	struct mxlk_pcie *xdev = container_of(mxlk, struct mxlk_pcie, mxlk);
	u32 head, tail, old, ndesc;
	struct mxlk_stream *tx = &mxlk->tx;
	struct mxlk_buf_desc *bd;
	struct mxlk_dma_desc *dd;
	struct mxlk_transfer_desc *td;
	size_t bytes, buffers;

	ndesc = tx->pipe.ndesc;
	old = tx->pipe.old;
	tail = mxlk_get_tdr_tail(&tx->pipe);
	head = mxlk_get_tdr_head(&tx->pipe);

	// clean old entries first
	while (old != head) {
		dd = tx->ddr + old;
		td = tx->pipe.tdr + old;
		bd = dd->bd;
		status = mxlk_get_td_status(td);
		if (status != MXLK_DESC_STATUS_SUCCESS)
			mxlk_error("detected tx desc failure (%u)\n", status);

		mxlk_unmap_dma(mxlk, dd, DMA_TO_DEVICE);
		mxlk_free_tx_bd(mxlk, dd->bd);
		dd->bd = NULL;
		old = MXLK_CIRCULAR_INC(old, ndesc);
	}
	tx->pipe.old = old;

	// add new entries
	while (MXLK_CIRCULAR_INC(tail, ndesc) != head) {
		bd = mxlk_list_get(&mxlk->write);
		if (!bd)
			break;

		dd = tx->ddr + tail;
		td = tx->pipe.tdr + tail;

		dd->bd = bd;
		if (mxlk_map_dma(mxlk, dd, DMA_TO_DEVICE)) {
			mxlk_error("dma mapping error bd addr %p, size %zu\n",
				   bd->data, bd->length);
			break;
		}

		mxlk_set_td_address(td, dd->phys);
		mxlk_set_td_length(td, dd->length);
		mxlk_set_td_interface(td, bd->interface);
		mxlk_set_td_status(td, MXLK_DESC_STATUS_ERROR);

		tail = MXLK_CIRCULAR_INC(tail, ndesc);
	}

	if (mxlk_get_tdr_tail(&tx->pipe) != tail) {
		mxlk_set_tdr_tail(&tx->pipe, tail);
		mxlk_set_doorbell(mxlk, TO_DEVICE, DATA_SENT);
		/*
		 * TODO
		 * change when no need to workaround for host->device interrupt
		 */
		pci_read_config_word(xdev->pci, PCI_VENDOR_ID, &vendor_id);
	}

	mxlk_list_info(&mxlk->write, &bytes, &buffers);
	if (buffers)
		mxlk->tx_pending = true;
	else
		mxlk->tx_pending = false;
}

static irqreturn_t mxlk_interrupt(int irq, void *args)
{
	struct mxlk_pcie *xdev = args;
	struct mxlk *mxlk = &xdev->mxlk;

	if (mxlk_get_doorbell(mxlk, FROM_DEVICE, DATA_SENT)) {
		mxlk_clear_doorbell(mxlk, FROM_DEVICE, DATA_SENT);
		mxlk_start_rx(mxlk, 0);
	}
	if (mxlk_get_doorbell(mxlk, FROM_DEVICE, DATA_RECEIVED)) {
		mxlk_clear_doorbell(mxlk, FROM_DEVICE, DATA_RECEIVED);
		if (mxlk->tx_pending)
			mxlk_start_tx(mxlk, 0);
	}

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
	int status;
	struct mxlk_pcie *xdev = container_of(mxlk, struct mxlk_pcie, mxlk);

	status = mxlk_get_device_status(mxlk);
	if (status != MXLK_STATUS_RUN) {
		mxlk_error("device status not RUNNING (%d)\n", status);
		error = -EBUSY;
		return error;
	}

	error = mxlk_version_check(mxlk);
	if (error)
		return error;

	error = mxlk_pci_register_irq(xdev, &mxlk_interrupt);
	if (error)
		return error;

	error = mxlk_events_init(mxlk);
	if (error)
		return error;

	error = mxlk_discover_txrx(mxlk);
	if (error)
		goto error_stream;

	error = mxlk_interfaces_init(mxlk);
	if (error)
		goto error_interfaces;

	mxlk_set_host_status(mxlk, MXLK_STATUS_RUN);
	mxlk_clear_doorbell(mxlk, TO_DEVICE, DATA_SENT);
	mxlk_clear_doorbell(mxlk, TO_DEVICE, DATA_RECEIVED);

	return 0;

error_interfaces:
	mxlk_txrx_cleanup(mxlk);

error_stream:
	mxlk_events_cleanup(mxlk);

	mxlk_set_host_status(mxlk, MXLK_STATUS_ERROR);

	return error;
}

void mxlk_core_cleanup(struct mxlk *mxlk)
{
	if (mxlk->status == MXLK_STATUS_RUN) {
		mxlk_set_host_status(mxlk, MXLK_STATUS_READY);
		mxlk_events_cleanup(mxlk);
		mxlk_interfaces_cleanup(mxlk);
		mxlk_txrx_cleanup(mxlk);
	}
}

int mxlk_core_read(struct mxlk *mxlk, void *buffer, size_t *length,
		   uint32_t timeout_ms)
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
		    uint32_t timeout_ms)
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

		mxlk_list_put(&mxlk->write, head);
		mxlk_start_tx(mxlk, 0);

		*length = len - remaining;

		jiffies_passed = (long)jiffies - (long)jiffies_start;
	} while (remaining > 0 && (jiffies_passed < jiffies_timeout ||
				   timeout_ms == 0));

	mutex_unlock(&mxlk->wlock);

	return 0;
}
