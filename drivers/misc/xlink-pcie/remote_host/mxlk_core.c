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
#include "../common/mxlk_mmio.h"
#include "../common/mxlk_print.h"
#include "../common/mxlk_capabilities.h"

static struct mxlk_interface *mxlk_inf;

#define MXLK_CIRCULAR_INC(val, max) (((val) + 1) % (max))

static int rx_pool_size = 8 * 1024 * 1024;
module_param(rx_pool_size, int, 0664);
MODULE_PARM_DESC(rx_pool_size, "receive pool size (default 8MB)");

static int tx_pool_size = 8 * 1024 * 1024;
module_param(tx_pool_size, int, 0664);
MODULE_PARM_DESC(tx_pool_size, "transmit pool size (default 8MB)");

static ssize_t debug_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct pci_dev *pdev = container_of(dev, struct pci_dev, dev);
	struct mxlk *mxlk = pci_get_drvdata(pdev);
	struct mxlk_stats new = mxlk->stats;

	snprintf(buf, 4096,
		 "tx_krn, pkts %zu (%zu) bytes %zu (%zu)\n"
		 "tx_usr, pkts %zu (%zu) bytes %zu (%zu)\n"
		 "rx_krn, pkts %zu (%zu) bytes %zu (%zu)\n"
		 "rx_usr, pkts %zu (%zu) bytes %zu (%zu)\n"
		 "interrupts %zu (%zu)\n"
		 "rx runs %zu (%zu) tx runs %zu (%zu)\n",
		 new.tx_krn.pkts,
		 (new.tx_krn.pkts - mxlk->stats_old.tx_krn.pkts),
		 new.tx_krn.bytes,
		 (new.tx_krn.bytes - mxlk->stats_old.tx_krn.bytes),
		 new.tx_usr.pkts,
		 (new.tx_usr.pkts - mxlk->stats_old.tx_usr.pkts),
		 new.tx_usr.bytes,
		 (new.tx_usr.bytes - mxlk->stats_old.tx_usr.bytes),
		 new.rx_krn.pkts,
		 (new.rx_krn.pkts - mxlk->stats_old.rx_krn.pkts),
		 new.rx_krn.bytes,
		 (new.rx_krn.bytes - mxlk->stats_old.rx_krn.bytes),
		 new.rx_usr.pkts,
		 (new.rx_usr.pkts - mxlk->stats_old.rx_usr.pkts),
		 new.rx_usr.bytes,
		 (new.rx_usr.bytes - mxlk->stats_old.rx_usr.bytes),
		 new.interrupts, (new.interrupts - mxlk->stats_old.interrupts),
		 new.rx_event_runs,
		 (new.rx_event_runs - mxlk->stats_old.rx_event_runs),
		 new.tx_event_runs,
		 (new.tx_event_runs - mxlk->stats_old.tx_event_runs));

	mxlk->stats_old = new;

	return strlen(buf);
}

static ssize_t debug_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	struct pci_dev *pdev = container_of(dev, struct pci_dev, dev);
	struct mxlk *mxlk = pci_get_drvdata(pdev);

	memset(&mxlk->stats_old, 0, sizeof(struct mxlk_stats));
	memset(&mxlk->stats, 0, sizeof(struct mxlk_stats));

	return count;
}

static int mxlk_version_check(struct mxlk *mxlk)
{
	struct mxlk_version version;

	mxlk_rd_buffer(mxlk->mmio, MXLK_MMIO_VERSION, &version,
		       sizeof(version));

	if ((version.major != MXLK_VERSION_MAJOR) ||
	    (version.minor != MXLK_VERSION_MINOR)) {
		mxlk_error(
			"version mismatch, dev : %d.%d.%d, host : %d.%d.%d\n",
			version.major, version.minor, version.build,
			MXLK_VERSION_MAJOR, MXLK_VERSION_MINOR,
			MXLK_VERSION_BUILD);

		return -EIO;
	}

	mxlk_info("versions, dev : %d.%d.%d, host : %d.%d.%d\n", version.major,
		  version.minor, version.build, MXLK_VERSION_MAJOR,
		  MXLK_VERSION_MINOR, MXLK_VERSION_BUILD);

	return 0;
}

static void mxlk_set_host_status(struct mxlk *mxlk, int status)
{
	mxlk->status = status;
	mxlk_wr32(mxlk->mmio, MXLK_MMIO_HOST_STATUS, status);
}

static void mxlk_set_doorbell_status(struct mxlk *mxlk, int status)
{
	mxlk_wr32(mxlk->mmio, MXLK_MMIO_DOORBELL_STATUS, status);
}

static int mxlk_get_device_status(struct mxlk *mxlk)
{
	return mxlk_rd32(mxlk->mmio, MXLK_MMIO_DEV_STATUS);
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

static int mxlk_map_dma(struct mxlk *mxlk, struct mxlk_dma_desc *dd,
			int direction)
{
	struct device *dev = MXLK_TO_DEV(mxlk);

	dd->phys = dma_map_single(dev, dd->bd->data, dd->bd->length, direction);
	dd->length = dd->bd->length;

	return dma_mapping_error(dev, dd->phys);
}

static void mxlk_unmap_dma(struct mxlk *mxlk, struct mxlk_dma_desc *dd,
			   int direction)
{
	struct device *dev = MXLK_TO_DEV(mxlk);
	dma_unmap_single(dev, dd->phys, dd->length, direction);
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

	mxlk_inf = NULL;
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
	mxlk->fragment_size = mxlk_rd32(&cap->fragment_size, 0);

	tx->busy = 0;
	tx->pipe.ndesc = mxlk_rd32(&cap->tx.ndesc, 0);
	tx->pipe.head = &cap->tx.head;
	tx->pipe.tail = &cap->tx.tail;
	tx->pipe.old = mxlk_rd32(&cap->tx.tail, 0);
	tx->pipe.tdr = mxlk->mmio + mxlk_rd32(&cap->tx.ring, 0);

	tx->ddr = kcalloc(tx->pipe.ndesc, sizeof(struct mxlk_dma_desc),
			  GFP_KERNEL);
	if (!tx->ddr) {
		mxlk_error("failed to alloc tx dma desc ring\n");
		goto error;
	}

	rx->busy = 0;
	rx->pipe.ndesc = mxlk_rd32(&cap->rx.ndesc, 0);
	rx->pipe.head = &cap->rx.head;
	rx->pipe.tail = &cap->rx.tail;
	rx->pipe.old = mxlk_rd32(&cap->tx.head, 0);
	rx->pipe.tdr = mxlk->mmio + mxlk_rd32(&cap->rx.ring, 0);

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
	u16 status, interface;
	u32 head, tail, ndesc, length;
	struct mxlk_stream *rx = &mxlk->rx;
	struct mxlk_buf_desc *bd, *replacement = NULL;
	struct mxlk_dma_desc *dd;
	struct mxlk_transfer_desc *td;
	unsigned long delay = msecs_to_jiffies(1);

	mxlk->stats.rx_event_runs++;

	ndesc = rx->pipe.ndesc;
	tail = mxlk_get_tdr_tail(&rx->pipe);
	head = mxlk_get_tdr_head(&rx->pipe);
	// clean old entries first
	while (head != tail) {
		td = rx->pipe.tdr + head;
		dd = rx->ddr + head;

		replacement = mxlk_alloc_rx_bd(mxlk);
		if (!replacement) {
			delay = msecs_to_jiffies(20);
			break;
		}

		status = mxlk_get_td_status(td);
		interface = mxlk_get_td_interface(td);
		length = mxlk_get_td_length(td);
		mxlk_unmap_dma(mxlk, dd, DMA_FROM_DEVICE);

		if (unlikely(status != MXLK_DESC_STATUS_SUCCESS)) {
			mxlk_free_rx_bd(mxlk, dd->bd);
		} else {
			bd = dd->bd;
			bd->interface = interface;
			bd->length = length;
			bd->next = NULL;

			if (likely(interface < MXLK_NUM_INTERFACES)) {
				mxlk->stats.rx_krn.pkts++;
				mxlk->stats.rx_krn.bytes += bd->length;
				mxlk_add_bd_to_interface(mxlk, bd);
			} else {
				mxlk_free_rx_bd(mxlk, bd);
			}
		}

		dd->bd = replacement;
		error = mxlk_map_dma(mxlk, dd, DMA_FROM_DEVICE);
		if (error)
			mxlk_error("failed to map rx bd (%d)\n", error);

		mxlk_set_td_address(td, dd->phys);
		mxlk_set_td_length(td, dd->length);
		head = MXLK_CIRCULAR_INC(head, ndesc);
	}

	if (mxlk_get_tdr_head(&rx->pipe) != head)
		mxlk_set_tdr_head(&rx->pipe, head);

	if (!replacement)
		mxlk_start_rx(mxlk, delay);
}

static void mxlk_tx_event_handler(struct work_struct *work)
{
	struct mxlk *mxlk = container_of(work, struct mxlk, tx_event.work);

	u16 status;
	/* TODO Remove when no need to workaround for host->device interrupt */
	u16 vendor_id;
	u32 head, tail, old, ndesc;
	struct mxlk_stream *tx = &mxlk->tx;
	struct mxlk_buf_desc *bd;
	struct mxlk_dma_desc *dd;
	struct mxlk_transfer_desc *td;

	mxlk->stats.tx_event_runs++;

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
		mxlk->stats.tx_krn.pkts++;
		mxlk->stats.tx_krn.bytes += bd->length;

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
		mxlk_set_doorbell_status(mxlk, MXLK_DOORBELL_SEND);
		/*
		 * TODO
		 * change when no need to workaround for host->device interrupt
		 */
		pci_read_config_word(mxlk->pci, PCI_VENDOR_ID, &vendor_id);
	}

	if (tail != head)
		mxlk_start_tx(mxlk, 0);
}

static irqreturn_t mxlk_interrupt(int irq, void *args)
{
	struct mxlk *mxlk = args;

	mxlk->stats.interrupts++;
	mxlk_start_rx(mxlk, 0);

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

	DEVICE_ATTR_RW(debug);

	mxlk_set_host_status(mxlk, mxlk->status);

	scnprintf(mxlk->name, MXLK_MAX_NAME_LEN, MXLK_DRIVER_NAME);

	error = mxlk_pci_register_irq(mxlk, &mxlk_interrupt);
	if (error)
		return error;

	error = mxlk_events_init(mxlk);
	if (error)
		goto error_events;

	status = mxlk_get_device_status(mxlk);
	if (status != MXLK_STATUS_RUN) {
		mxlk_error("device status not RUNNING (%d)\n", status);
		error = -EBUSY;
		goto error_device_status;
	}

	error = mxlk_version_check(mxlk);
	if (error)
		goto error_version;

	error = mxlk_discover_txrx(mxlk);
	if (error)
		goto error_stream;

	error = mxlk_interfaces_init(mxlk);
	if (error)
		goto error_interfaces;

	mxlk_set_host_status(mxlk, MXLK_STATUS_RUN);
	mxlk_set_doorbell_status(mxlk, MXLK_DOORBELL_NONE);

	mxlk->debug = dev_attr_debug;
	device_create_file(&mxlk->pci->dev, &mxlk->debug);

	memset(&mxlk->stats, 0, sizeof(struct mxlk_stats));
	memset(&mxlk->stats_old, 0, sizeof(struct mxlk_stats));

	return 0;

error_interfaces:
	mxlk_txrx_cleanup(mxlk);

error_stream:
error_version:
error_device_status:
	mxlk_events_cleanup(mxlk);

error_events:
	mxlk_error("core failed to init\n");

	return error;
}

void mxlk_core_cleanup(struct mxlk *mxlk)
{
	mxlk_set_host_status(mxlk, MXLK_STATUS_UNINIT);

	mxlk_events_cleanup(mxlk);

	device_remove_file(MXLK_TO_DEV(mxlk), &mxlk->debug);
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
