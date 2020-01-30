// SPDX-License-Identifier: GPL-2.0 only
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#include "mxlk_util.h"

void mxlk_set_device_status(struct mxlk *mxlk, u32 status)
{
	mxlk->status = status;
	iowrite32(status, mxlk->mmio + MXLK_MMIO_DEV_STATUS);
}

u32 mxlk_get_device_status(struct mxlk *mxlk)
{
	return ioread32(mxlk->mmio + MXLK_MMIO_DEV_STATUS);
}

static size_t mxlk_doorbell_offset(enum mxlk_doorbell_direction dirt,
				   enum mxlk_doorbell_type type)
{
	if (dirt == TO_DEVICE && type == DATA_SENT)
		return MXLK_MMIO_HTOD_TX_DOORBELL_STATUS;
	if (dirt == TO_DEVICE && type == DATA_RECEIVED)
		return MXLK_MMIO_HTOD_RX_DOORBELL_STATUS;
	if (dirt == FROM_DEVICE && type == DATA_SENT)
		return MXLK_MMIO_DTOH_TX_DOORBELL_STATUS;
	if (dirt == FROM_DEVICE && type == DATA_RECEIVED)
		return MXLK_MMIO_DTOH_RX_DOORBELL_STATUS;

	return 0;
}

void mxlk_set_doorbell(struct mxlk *mxlk, enum mxlk_doorbell_direction dirt,
		       enum mxlk_doorbell_type type)
{
	size_t offset = mxlk_doorbell_offset(dirt, type);
	iowrite8(0x1, mxlk->mmio + offset);
}

bool mxlk_get_doorbell(struct mxlk *mxlk, enum mxlk_doorbell_direction dirt,
		       enum mxlk_doorbell_type type)
{
	size_t offset = mxlk_doorbell_offset(dirt, type);
	return ioread8(mxlk->mmio + offset);
}

void mxlk_clear_doorbell(struct mxlk *mxlk, enum mxlk_doorbell_direction dirt,
			 enum mxlk_doorbell_type type)
{
	size_t offset = mxlk_doorbell_offset(dirt, type);
	iowrite8(0, mxlk->mmio + offset);
}

u32 mxlk_get_host_status(struct mxlk *mxlk)
{
	return ioread32(mxlk->mmio + MXLK_MMIO_HOST_STATUS);
}

void mxlk_set_host_status(struct mxlk *mxlk, u32 status)
{
	mxlk->status = status;
	iowrite32(status, mxlk->mmio + MXLK_MMIO_HOST_STATUS);
}

struct mxlk_buf_desc *mxlk_alloc_bd(size_t length)
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

void mxlk_free_bd(struct mxlk_buf_desc *bd)
{
	if (bd) {
		kfree(bd->head);
		kfree(bd);
	}
}

int mxlk_list_init(struct mxlk_list *list)
{
	spin_lock_init(&list->lock);
	list->bytes = 0;
	list->buffers = 0;
	list->head = NULL;
	list->tail = NULL;

	return 0;
}

void mxlk_list_cleanup(struct mxlk_list *list)
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

int mxlk_list_put(struct mxlk_list *list, struct mxlk_buf_desc *bd)
{
	if (!bd)
		return -EINVAL;

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

struct mxlk_buf_desc *mxlk_list_get(struct mxlk_list *list)
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

void mxlk_list_info(struct mxlk_list *list, size_t *bytes, size_t *buffers)
{
	spin_lock(&list->lock);
	*bytes = list->bytes;
	*buffers = list->buffers;
	spin_unlock(&list->lock);
}

struct mxlk_buf_desc *mxlk_alloc_rx_bd(struct mxlk *mxlk)
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

void mxlk_free_rx_bd(struct mxlk *mxlk, struct mxlk_buf_desc *bd)
{
	if (bd)
		mxlk_list_put(&mxlk->rx_pool, bd);
}

struct mxlk_buf_desc *mxlk_alloc_tx_bd(struct mxlk *mxlk)
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

void mxlk_free_tx_bd(struct mxlk *mxlk, struct mxlk_buf_desc *bd)
{
	if (!bd)
		return;

	mxlk_list_put(&mxlk->tx_pool, bd);

	mxlk->no_tx_buffer = false;
	wake_up_interruptible(&mxlk->tx_waitqueue);
}

int mxlk_interface_init(struct mxlk *mxlk, int id)
{
	struct mxlk_interface *inf = mxlk->interfaces + id;

	inf->id = id;
	inf->mxlk = mxlk;

	inf->partial_read = NULL;
	mxlk_list_init(&inf->read);
	mutex_init(&inf->rlock);
	inf->data_available = false;
	init_waitqueue_head(&inf->rx_waitqueue);

	return 0;
}

void mxlk_interface_cleanup(struct mxlk_interface *inf)
{
	struct mxlk_buf_desc *bd;

	mutex_destroy(&inf->rlock);

	mxlk_free_rx_bd(inf->mxlk, inf->partial_read);
	while ((bd = mxlk_list_get(&inf->read)))
		mxlk_free_rx_bd(inf->mxlk, bd);
}

void mxlk_interfaces_cleanup(struct mxlk *mxlk)
{
	int index;

	for (index = 0; index < MXLK_NUM_INTERFACES; index++)
		mxlk_interface_cleanup(mxlk->interfaces + index);

	mxlk_list_cleanup(&mxlk->write);
	mutex_destroy(&mxlk->wlock);
}

int mxlk_interfaces_init(struct mxlk *mxlk)
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

void mxlk_add_bd_to_interface(struct mxlk *mxlk, struct mxlk_buf_desc *bd)
{
	struct mxlk_interface *inf;

	inf = mxlk->interfaces + bd->interface;

	mxlk_list_put(&inf->read, bd);

	mutex_lock(&inf->rlock);
	inf->data_available = true;
	mutex_unlock(&inf->rlock);
	wake_up_interruptible(&inf->rx_waitqueue);
}
