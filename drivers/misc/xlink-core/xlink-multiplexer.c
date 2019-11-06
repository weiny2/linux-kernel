// SPDX-License-Identifier: GPL-2.0-only
/*
 * xlink Multiplexer.
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/completion.h>

#ifdef CONFIG_XLINK_LOCAL_HOST
#include <linux/keembay-vpu-ipc.h>
#endif

#include "xlink-multiplexer.h"
#include "xlink-dispatcher.h"
#include "xlink-platform.h"

// Maximum number of channels per link.
#define XLINK_IPC_MAX_CHANNELS	1024

// Timeout used for open channel
#define OPEN_CHANNEL_TIMEOUT_MSEC 5000

// This is used as index for retrieving reserved memory from the device tree.
#define LOCAL_XLINK_IPC_BUFFER_IDX	0
#define REMOTE_XLINK_IPC_BUFFER_IDX	1

/* Channel mapping table. */
struct xlink_channel_type {
	enum xlink_dev_type remote_to_local;
	enum xlink_dev_type local_to_ip;
};

struct xlink_channel_table_entry {
	uint16_t start_range;
	uint16_t stop_range;
	struct xlink_channel_type type;
};

const struct xlink_channel_table_entry default_channel_table[] = {
	{0x0,			0x1,			{PCIE_DEVICE,	IPC_DEVICE}},
	{0x2,			0x9,			{USB_DEVICE,	IPC_DEVICE}},
	{0xA,			0x3FD,			{PCIE_DEVICE,	IPC_DEVICE}},
	{0x3FE,			0x3FF,			{ETH_DEVICE,	IPC_DEVICE}},
	{0x400,			0xFFE,			{PCIE_DEVICE,	NULL_DEVICE}},
	{0xFFF,			0xFFF,			{ETH_DEVICE,	NULL_DEVICE}},
	{NMB_CHANNELS,	NMB_CHANNELS,	{NULL_DEVICE,	NULL_DEVICE}},
};

/* struct xlink_buf_mem - xlink Buffer Memory Region. */
struct xlink_buf_mem {
	struct device *dev;	/* Child device managing the memory region. */
	void *vaddr;		/* The virtual address of the memory region. */
	dma_addr_t dma_handle;  /* The physical address of the memory region. */
	size_t size;		/* The size of the memory region. */
};

/* xlink buffer pool. */
struct xlink_buf_pool {
	void *buf;			/* Pointer to the start of pool area */
	size_t buf_cnt;		/* Pool size (i.e., number of buffers). */
	size_t idx;			/* Current index. */
	spinlock_t lock;	/* The lock protecting this pool. */
};

struct channel {
	struct xlink_handle	*handle;
	enum xlink_opmode mode;
	enum xlink_channel_status status;
	uint32_t size;
	uint32_t timeout;
};

struct packet {
	uint8_t* data;
	uint32_t length;
	dma_addr_t paddr;
	struct list_head list;
};

struct packet_queue {
	uint32_t count;
	uint32_t capacity;
	struct list_head head;
	struct mutex lock;
};

struct open_channel {
	uint16_t id;
	struct channel *chan;
	struct xlink_channel_type type;
	struct packet_queue rx_queue;
	struct packet_queue tx_queue;
	int32_t rx_fill_level;
	int32_t tx_fill_level;
	int32_t tx_packet_level;
	uint8_t is_opened;
	struct list_head list;
	struct completion opened;
	struct completion pkt_available;
	struct completion pkt_consumed;
	struct completion pkt_released;
	struct mutex lock;
};

struct xlink_multiplexer {
	struct device *dev;
	struct xlink_buf_mem local_xlink_mem;
	struct xlink_buf_mem remote_xlink_mem;
	struct xlink_buf_pool xlink_buf_pool;
	struct channel channels[NMB_CHANNELS];
	struct list_head open_channels;
	struct mutex lock;
};

struct xlink_multiplexer *mux = 0;

#ifdef CONFIG_XLINK_LOCAL_HOST
/*
 * Functions related to reserved-memory sub-devices.
 */

/* Remove the xlink memory sub-devices. */
static void xlink_reserved_memory_remove(struct xlink_multiplexer *xmux)
{
	device_unregister(xmux->local_xlink_mem.dev);
	device_unregister(xmux->remote_xlink_mem.dev);
}

/* Release function for the reserved memory sub-devices. */
static void xlink_reserved_mem_release(struct device *dev)
{
	of_reserved_mem_device_release(dev);
}

/* Get the size of the specified reserved memory region. */
static resource_size_t get_xlink_reserved_mem_size(struct device *dev,
						 unsigned int idx)
{
	struct resource mem;
	struct device_node *np;
	int rc;

	np = of_parse_phandle(dev->of_node, "memory-region", idx);
	if (!np) {
		dev_err(dev, "Couldn't find memory-region %d\n", idx);
		return 0;
	}

	rc = of_address_to_resource(np, 0, &mem);
	if (rc) {
		dev_err(dev, "Couldn't map address to resource %d\n", idx);
		return 0;
	}

	return resource_size(&mem);
}

/* Init a reserved memory sub-devices. */
static struct device *init_xlink_reserved_mem_dev(struct device *dev,
						const char *name,
						unsigned int idx)
{
	struct device *child;
	int rc;

	child = devm_kzalloc(dev, sizeof(*child), GFP_KERNEL);
	if (!child)
		return NULL;

	device_initialize(child);
	dev_set_name(child, "%s:%s", dev_name(dev), name);
	dev_err(dev, " dev_name %s, name %s\n", dev_name(dev), name);
	child->parent = dev;
	child->coherent_dma_mask = dev->coherent_dma_mask;
	child->dma_mask = dev->dma_mask;
	child->release = xlink_reserved_mem_release;

	rc = device_add(child);
	if (rc)
		goto err;
	rc = of_reserved_mem_device_init_by_idx(child, dev->of_node, idx);
	if (rc) {
		dev_err(dev, "Couldn't get reserved memory with idx = %d, %d\n",
			idx, rc);
		device_del(child);
		goto err;
	}
	return child;

err:
	put_device(child);
	return NULL;
}

/* Init reserved memory for our diver. */
static int xlink_reserved_memory_init(struct xlink_multiplexer *xmux,
		struct platform_device *plat_dev)
{
	struct device *dev = &plat_dev->dev;

	xmux->local_xlink_mem.dev = init_xlink_reserved_mem_dev(
		dev, "xlink_local_reserved", LOCAL_XLINK_IPC_BUFFER_IDX);
	if (!xmux->local_xlink_mem.dev)
		return -ENOMEM;

	xmux->local_xlink_mem.size =
		get_xlink_reserved_mem_size(dev, LOCAL_XLINK_IPC_BUFFER_IDX);

	xmux->remote_xlink_mem.dev = init_xlink_reserved_mem_dev(
		dev, "xlink_remote_reserved", REMOTE_XLINK_IPC_BUFFER_IDX);
	if (!xmux->remote_xlink_mem.dev) {
		device_unregister(xmux->local_xlink_mem.dev);
		return -ENOMEM;
	}

	xmux->remote_xlink_mem.size =
		get_xlink_reserved_mem_size(dev, REMOTE_XLINK_IPC_BUFFER_IDX);

	return 0;
}

/*
 * Init the xlink Buffer Pool.
 *
 * Set up the xlink Buffer Pool to be used for allocating TX buffers.
 *
 * The pool uses the local xlink Buffer memory previously allocated.
 */
static int init_xlink_buf_pool(struct xlink_multiplexer *xmux,
		struct platform_device *plat_dev)
{
	struct xlink_buf_mem *mem = &xmux->local_xlink_mem;
	/* Initialize xlink_buf_poll global variable. */
	/*
	 * Start by setting everything to 0 to initialize the xlink Buffer array
	 */
	memset(mem->vaddr, 0, mem->size);
	xmux->xlink_buf_pool.buf = mem->vaddr;
	xmux->xlink_buf_pool.buf_cnt =
			mem->size / XLINK_MAX_BUF_SIZE;
	xmux->xlink_buf_pool.idx = 0;
	dev_info(&plat_dev->dev, "xlink Buffer Pool size: %zX\n",
			xmux->xlink_buf_pool.buf_cnt);
	spin_lock_init(&xmux->xlink_buf_pool.lock);
	return 0;
}

/*
 * xlink_phys_to_virt() - Convert xlink physical addresses to virtual addresses.
 *
 * @xlink_mem: xlink mem region where the physical address is expected to be.
 * @paddr:	 The physical address to be converted to a virtual one.
 *
 * Return: The corresponding virtual address, or NULL if the physical address
 *	   is not in the expected memory range.
 */
static void *xlink_phys_to_virt(const struct xlink_buf_mem *xlink_mem,
		uint32_t paddr)
{
	if (unlikely(paddr < xlink_mem->dma_handle) ||
			 paddr >= (xlink_mem->dma_handle + xlink_mem->size))
		return NULL;
	return xlink_mem->vaddr + (paddr - xlink_mem->dma_handle);
}

/*
 * xlink_virt_to_phys() - Convert xlink virtual addresses to physical addresses.
 *
 * @xlink_mem: [in] xlink mem region where the physical address is expected
 *		   to be.
 * @vaddr:   [in]  The virtual address to be converted to a physical one.
 * @paddr:   [out] Where to store the computed physical address.
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int xlink_virt_to_phys(struct xlink_buf_mem *xlink_mem, void *vaddr,
		uint32_t *paddr)
{
	if (unlikely((xlink_mem->dma_handle + xlink_mem->size) > 0xFFFFFFFF))
		return -EINVAL;
	if (unlikely(vaddr < xlink_mem->vaddr ||
			vaddr >= (xlink_mem->vaddr + xlink_mem->size)))
		return -EINVAL;
	*paddr = xlink_mem->dma_handle + (vaddr - xlink_mem->vaddr);

	return 0;
}

/* Get next xlink buffer from pool */
static int get_next_xlink_buf(void **buf, int size)
{
	struct xlink_buf_pool *pool = &mux->xlink_buf_pool;

	if (size > XLINK_MAX_BUF_SIZE)
		return -1;
	mutex_lock(&mux->lock);
	if (pool->idx == pool->buf_cnt) {
		/* reached end of buffers - wrap around */
		pool->idx = 0;
	}
	*buf = pool->buf + (pool->idx * XLINK_MAX_BUF_SIZE);
	pool->idx++;
	mutex_unlock(&mux->lock);

	return 0;
}
#endif // CONFIG_XLINK_LOCAL_HOST

/*
 * Multiplexer Internal Functions
 *
 */

static struct xlink_channel_type const *get_channel_type(uint16_t chan)
{
	int i = 0;
	struct xlink_channel_type const *type = NULL;
	
	while(default_channel_table[i].start_range < NMB_CHANNELS) {
		if ((chan >= default_channel_table[i].start_range) &&
				(chan <= default_channel_table[i].stop_range)) {
			type = &default_channel_table[i].type;
			break;
		}
		i++;
	}
	return type;
}

static int is_channel_for_device_type(uint16_t chan,
		enum xlink_dev_type dev_type)
{
	struct xlink_channel_type const *chan_type = get_channel_type(chan);
	if (dev_type == IPC_DEVICE) {
		if (chan_type->local_to_ip == dev_type)
			return 1;
	} else {
		if (chan_type->remote_to_local == dev_type)
			return 1;
	}
	return 0; 
}

static int is_enough_space_in_channel(struct open_channel *opchan,
		uint32_t size)
{
	if (opchan->tx_packet_level >= XLINK_PACKET_QUEUE_CAPACITY ||
		opchan->tx_fill_level + size > opchan->chan->size) {
		printk(KERN_DEBUG "Not enough space in channel 0x%x for %u: PKT %u, \
				FILL %u SIZE %u\n", opchan->id, size,
				opchan->tx_packet_level, opchan->tx_fill_level,
				opchan->chan->size);
		return 0;
	}
	return 1;
}

static int is_control_channel(uint16_t chan)
{
	if ((chan == IP_CONTROL_CHANNEL) || (chan == VPU_CONTROL_CHANNEL)) {
		return 1;
	}
	return 0;
}

static struct open_channel *get_channel(uint16_t chan)
{
	struct open_channel *opchan = NULL;
	mutex_lock(&mux->lock);
	list_for_each_entry(opchan, &mux->open_channels, list) {
		if (opchan->id == chan) {
			mutex_lock(&opchan->lock);
			mutex_unlock(&mux->lock);
			return opchan;
		}
	}
	mutex_unlock(&mux->lock);
	return NULL;
}

static void release_channel(struct open_channel *opchan)
{
	if (opchan) {
		mutex_unlock(&opchan->lock);
	}
}

static int add_packet_to_channel(struct open_channel *opchan,
		struct packet_queue *queue, void* buffer, uint32_t size,
		dma_addr_t paddr)
{
	struct packet *pkt = NULL;

	mutex_lock(&queue->lock);
	if (queue->count < queue->capacity) {
		pkt = kzalloc(sizeof(*pkt), GFP_KERNEL);
		if (pkt == NULL) {
			printk(KERN_DEBUG "Failed to allocate memory for packet\n");
			return X_LINK_ERROR;
		}
		pkt->data = buffer;
		pkt->length = size;
		pkt->paddr = paddr;
		list_add_tail(&pkt->list, &queue->head);
		queue->count++;
		opchan->rx_fill_level += pkt->length;
	}
	mutex_unlock(&queue->lock);
	return X_LINK_SUCCESS;
}

static struct packet *get_packet_from_channel(struct packet_queue *queue)
{
	struct packet *pkt = NULL;

	mutex_lock(&queue->lock);
	// get first packet in queue
	if (!list_empty(&queue->head)) {
		pkt = list_first_entry(&queue->head, struct packet, list);
	}
	mutex_unlock(&queue->lock);
	return pkt;
}

static int release_packet_from_channel(struct open_channel *opchan,
		struct packet_queue *queue, uint8_t * const addr, uint32_t* size)
{
	uint8_t packet_found = 0;
	struct packet *pkt = NULL;

	mutex_lock(&queue->lock);
	if (addr == NULL) {
		// address is null, release first packet in queue
		if (!list_empty(&queue->head)) {
			pkt = list_first_entry(&queue->head, struct packet, list);
			packet_found = 1;
		}
	} else {
		// find packet in channel rx queue
		list_for_each_entry(pkt, &queue->head, list) {
			if (pkt->data == addr) {
				packet_found = 1;
				break;
			}
		}
	}
	if (!pkt || !packet_found) {
		mutex_unlock(&queue->lock);
		return X_LINK_ERROR;
	}
	// packet found, deallocate and remove from queue
	xlink_platform_deallocate(mux->dev, pkt->data, pkt->paddr, pkt->length,
			XLINK_PACKET_ALIGNMENT);
	list_del(&pkt->list);
	queue->count--;
	opchan->rx_fill_level -= pkt->length;
	if(opchan->rx_fill_level < 0)
		opchan->rx_fill_level = 0;
	if (size) {
		*size = pkt->length;
	}
	kfree(pkt);
	// printk(KERN_DEBUG "Release of %u on channel 0x%x: rx fill level = %u out of %u\n",
	// 		pkt->length, opchan->id, opchan->rx_fill_level,
	// 		opchan->chan->size);
	mutex_unlock(&queue->lock);
	return X_LINK_SUCCESS;
}

static int multiplexer_open_channel(uint16_t chan)
{
	struct open_channel* opchan = NULL;

	// allocate open channel
	opchan = kzalloc(sizeof(*opchan), GFP_KERNEL);
	if (opchan == NULL) {
		printk(KERN_DEBUG "Failed to allocate memory for open channel\n");
		return X_LINK_ERROR;
	}
	// initialize open channel
	opchan->id = chan;
	opchan->chan = &mux->channels[chan];
	INIT_LIST_HEAD(&opchan->rx_queue.head);
	opchan->rx_queue.count = 0;
	opchan->rx_queue.capacity = XLINK_PACKET_QUEUE_CAPACITY;
	INIT_LIST_HEAD(&opchan->tx_queue.head);
	opchan->tx_queue.count = 0;
	opchan->tx_queue.capacity = XLINK_PACKET_QUEUE_CAPACITY;
	opchan->rx_fill_level = 0;
	opchan->tx_fill_level = 0;
	opchan->tx_packet_level = 0;
	opchan->is_opened = 0;
	init_completion(&opchan->opened);
	init_completion(&opchan->pkt_available);
	init_completion(&opchan->pkt_consumed);
	init_completion(&opchan->pkt_released);
	mutex_init(&opchan->lock);
	// add to open channels list
	mutex_lock(&mux->lock);
	list_add_tail(&opchan->list, &mux->open_channels);
	mutex_unlock(&mux->lock);
	return X_LINK_SUCCESS;
}

static int multiplexer_close_channel(struct open_channel *opchan)
{
	if (opchan == NULL) {
		return X_LINK_ERROR;
	}

	// free remaining packets
	while (!list_empty(&opchan->rx_queue.head)) {
		release_packet_from_channel(opchan, &opchan->rx_queue, NULL, NULL);
	}
	while (!list_empty(&opchan->tx_queue.head)) {
		release_packet_from_channel(opchan, &opchan->tx_queue, NULL, NULL);
	}
	// remove channel from list
	mutex_lock(&mux->lock);
	list_del(&opchan->list);
	mutex_unlock(&mux->lock);
	// deallocate data structure and destroy
	mutex_destroy(&opchan->rx_queue.lock);
	mutex_destroy(&opchan->tx_queue.lock);
	mutex_unlock(&opchan->lock);
	mutex_destroy(&opchan->lock);
	kfree(opchan);
	return X_LINK_SUCCESS;
}

/*
 * Multiplexer External Functions
 *
 */

enum xlink_error xlink_multiplexer_init(void *dev)
{
	int rc = 0;
	struct xlink_multiplexer *mux_init; 
	struct platform_device *plat_dev = (struct platform_device *) dev;

	// only initialize multiplexer once
	if (mux != NULL) {
		printk(KERN_DEBUG "Multiplexer already initialized\n");
		return X_LINK_ERROR;
	}

	// allocate multiplexer data structure
	mux_init = kzalloc(sizeof(*mux_init), GFP_KERNEL);
	if (mux_init == NULL) {
		printk(KERN_DEBUG "Failed to allocate memory for Multiplexer\n");
		return X_LINK_ERROR;
	}
	mux_init->dev = &plat_dev->dev;

#ifdef CONFIG_XLINK_LOCAL_HOST
	/* Grab reserved memory regions and assign to child devices */
	rc = xlink_reserved_memory_init(mux_init, plat_dev);
	if (rc < 0) {
		dev_err(&plat_dev->dev,
			"Failed to set up reserved memory regions.\n");
		return rc;
	}

	/* Allocate memory from the reserved memory regions */
	mux_init->local_xlink_mem.vaddr =
		dmam_alloc_coherent(mux_init->local_xlink_mem.dev,
				mux_init->local_xlink_mem.size,
					&mux_init->local_xlink_mem.dma_handle,
					GFP_KERNEL);
	if (!mux_init->local_xlink_mem.vaddr) {
		dev_err(&plat_dev->dev,
			"Failed to allocate from local reserved memory.\n");
		xlink_reserved_memory_remove(mux_init);
		return -ENOMEM;
	}
	mux_init->remote_xlink_mem.vaddr = dmam_alloc_coherent(
			mux_init->remote_xlink_mem.dev,
			mux_init->remote_xlink_mem.size,
			&mux_init->remote_xlink_mem.dma_handle,
			GFP_KERNEL);
	if (!mux_init->remote_xlink_mem.vaddr) {
		dev_err(&plat_dev->dev,
			"Failed to allocate from remote reserved memory.\n");
		xlink_reserved_memory_remove(mux_init);
		return -ENOMEM;
	}

	dev_info(&plat_dev->dev, "Local vaddr 0x%p paddr 0x%pad size 0x%zX\n",
			mux_init->local_xlink_mem.vaddr,
			&mux_init->local_xlink_mem.dma_handle,
			mux_init->local_xlink_mem.size);
	dev_info(&plat_dev->dev, "Remote vaddr 0x%p paddr 0x%pad size 0x%zX\n",
			mux_init->remote_xlink_mem.vaddr,
			&mux_init->remote_xlink_mem.dma_handle,
			mux_init->remote_xlink_mem.size);

	/* Init the pool of xlink Buffer to be used to TX. */
	init_xlink_buf_pool(mux_init, plat_dev);
	/* Init the only link we have (ARM CSS -> LEON MSS). */
#endif // CONFIG_XLINK_LOCAL_HOST

	mutex_init(&mux_init->lock);
	INIT_LIST_HEAD(&mux_init->open_channels);

	// set the global reference to multiplexer
	mux = mux_init;

	// open ip control channel
	mux->channels[IP_CONTROL_CHANNEL].handle = NULL;
	mux->channels[IP_CONTROL_CHANNEL].size = CONTROL_CHANNEL_DATASIZE;
	mux->channels[IP_CONTROL_CHANNEL].timeout = CONTROL_CHANNEL_TIMEOUT_MS;
	mux->channels[IP_CONTROL_CHANNEL].mode = CONTROL_CHANNEL_OPMODE;
	rc = multiplexer_open_channel(IP_CONTROL_CHANNEL);
	if (rc) {
		return X_LINK_ERROR;
	} else {
		mux->channels[IP_CONTROL_CHANNEL].status = CHAN_OPEN;
	}
	// open vpu control channel
	mux->channels[VPU_CONTROL_CHANNEL].handle = NULL;
	mux->channels[VPU_CONTROL_CHANNEL].size = CONTROL_CHANNEL_DATASIZE;
	mux->channels[VPU_CONTROL_CHANNEL].timeout = CONTROL_CHANNEL_TIMEOUT_MS;
	mux->channels[VPU_CONTROL_CHANNEL].mode = CONTROL_CHANNEL_OPMODE;
	rc = multiplexer_open_channel(VPU_CONTROL_CHANNEL);
	if (rc) {
		return X_LINK_ERROR;
	} else {
		mux->channels[VPU_CONTROL_CHANNEL].status = CHAN_OPEN;
	}
	return X_LINK_SUCCESS;
}

enum xlink_error xlink_multiplexer_tx(struct xlink_event *event, int *event_queued)
{
	enum xlink_error rc = X_LINK_SUCCESS;
	struct open_channel *opchan = NULL;
	struct packet *pkt = NULL;
	uint32_t size = 0;
	uint16_t chan;

	if ((mux == NULL) || (event == NULL)) {
		return X_LINK_ERROR;
	}
	chan = event->header.chan;

	// verify channel ID is in range
	if (chan >= NMB_CHANNELS) {
		return X_LINK_ERROR;
	}

	// verify device type can communicate on channel ID
	if (!is_channel_for_device_type(chan, event->handle->dev_type)) {
		return X_LINK_ERROR;
	}

	// verify this is not a control channel
	if (is_control_channel(chan)) {
		return X_LINK_ERROR;
	}

	if (chan < XLINK_IPC_MAX_CHANNELS &&
			event->handle->dev_type == IPC_DEVICE) {
		// event should be handled by passthrough
		rc = xlink_passthrough(event);
		// kfree(event);
	} else {
		// event should be handled by dispatcher
		switch (event->header.type) {
		case XLINK_WRITE_REQ:
		case XLINK_WRITE_VOLATILE_REQ:
		case XLINK_WRITE_CONTROL_REQ:
			opchan = get_channel(chan);
			if ((opchan == NULL) || (opchan->chan->status != CHAN_OPEN)) {
				rc = X_LINK_COMMUNICATION_FAIL;
			} else {
				event->header.timeout = opchan->chan->timeout;
				while(!is_enough_space_in_channel(opchan,
						event->header.size)) {
					if ((opchan->chan->mode == RXN_TXB) ||
							(opchan->chan->mode == RXB_TXB)) {
						// channel is blocking, wait for packet to be released
						// TODO: calculate timeout remainder after each loop
						if (opchan->chan->timeout == 0) {
							mutex_unlock(&opchan->lock);
							rc = wait_for_completion_interruptible(
									&opchan->pkt_released);
							mutex_lock(&opchan->lock);
							if (rc < 0) {
								// wait interrupted
								rc = X_LINK_ERROR;
								break;
							}
						} else {
							mutex_unlock(&opchan->lock);
							rc = wait_for_completion_interruptible_timeout(
									&opchan->pkt_released,
									msecs_to_jiffies(opchan->chan->timeout));
							mutex_lock(&opchan->lock);
							if (rc == 0) {
								rc = X_LINK_TIMEOUT;
								break;
							} else if (rc < 0) {
								// wait interrupted
								rc = X_LINK_ERROR;
								break;
							} else if (rc > 0) {
								rc = X_LINK_SUCCESS;
							}
						}
					} else {
						rc = X_LINK_ERROR;
						break;
					}
				}
				if (rc == X_LINK_SUCCESS) {
					xlink_dispatcher_event_add(EVENT_TX, event);
					*event_queued = 1;
					if ((opchan->chan->mode == RXN_TXB) ||
							(opchan->chan->mode == RXB_TXB)) {
						// channel is blocking, wait for packet to be consumed
						// TODO: calculate timeout remainder since last wait
						mutex_unlock(&opchan->lock);
						if (opchan->chan->timeout == 0) {
							rc = wait_for_completion_interruptible(
									&opchan->pkt_consumed);
							// reinit_completion(&opchan->pkt_consumed);
							if (rc < 0) {
								// wait interrupted
								rc = X_LINK_ERROR;
							}
						} else {
							rc = wait_for_completion_interruptible_timeout(
									&opchan->pkt_consumed,
									msecs_to_jiffies(opchan->chan->timeout));
							// reinit_completion(&opchan->pkt_consumed);
							if (rc == 0) {
								rc = X_LINK_TIMEOUT;
							} else if (rc < 0) {
								// wait interrupted
								rc = X_LINK_ERROR;
							} else if (rc > 0) {
								rc = X_LINK_SUCCESS;
							}
						}
						mutex_lock(&opchan->lock);
					}
				}
			}
			release_channel(opchan);
			break;
		case XLINK_READ_REQ:
			opchan = get_channel(chan);
			if ((opchan == NULL) || (opchan->chan->status != CHAN_OPEN)) {
				rc = X_LINK_COMMUNICATION_FAIL;
			} else {
				event->header.timeout = opchan->chan->timeout;
				if ((opchan->chan->mode == RXB_TXN) ||
						(opchan->chan->mode == RXB_TXB)) {
					// channel is blocking, wait for packet to become available
					mutex_unlock(&opchan->lock);
					if (opchan->chan->timeout == 0) {
						rc = wait_for_completion_interruptible(
								&opchan->pkt_available);
					} else {
						rc = wait_for_completion_interruptible_timeout(
								&opchan->pkt_available,
								msecs_to_jiffies(opchan->chan->timeout));
						if (rc == 0) {
							rc = X_LINK_TIMEOUT;
						} else if (rc < 0) {
							// wait interrupted
							rc = X_LINK_ERROR;
						} else if (rc > 0) {
							rc = X_LINK_SUCCESS;
						}
					}
					mutex_lock(&opchan->lock);
				}
				if (rc == X_LINK_SUCCESS) {
					pkt = get_packet_from_channel(&opchan->rx_queue);
					if (pkt) {
						*(uint32_t **)event->pdata = (uint32_t *)pkt->data;
						*event->length = pkt->length;
						xlink_dispatcher_event_add(EVENT_TX, event);
						*event_queued = 1;
					} else {
						rc = X_LINK_ERROR;
					}
				}
			}
			release_channel(opchan);
			break;
		case XLINK_READ_TO_BUFFER_REQ:
			opchan = get_channel(chan);
			if ((opchan == NULL) || (opchan->chan->status != CHAN_OPEN)) {
				rc = X_LINK_COMMUNICATION_FAIL;
			} else {
				event->header.timeout = opchan->chan->timeout;
				if ((opchan->chan->mode == RXB_TXN) ||
						(opchan->chan->mode == RXB_TXB)) {
					// channel is blocking, wait for packet to become available
					mutex_unlock(&opchan->lock);
					if (opchan->chan->timeout == 0) {
						rc = wait_for_completion_interruptible(
								&opchan->pkt_available);
					} else {
						rc = wait_for_completion_interruptible_timeout(
								&opchan->pkt_available,
								msecs_to_jiffies(opchan->chan->timeout));
						if (rc == 0) {
							rc = X_LINK_TIMEOUT;
						} else if (rc > 0) {
							rc = X_LINK_SUCCESS;
						} else if ( rc < 0 ) {
							// wait interrupted
							rc = X_LINK_ERROR;
						}
					}
					mutex_lock(&opchan->lock);
				}
				if (rc == X_LINK_SUCCESS) {
					pkt = get_packet_from_channel(&opchan->rx_queue);
					if (pkt) {
						memcpy(event->data, pkt->data, pkt->length);
						*event->length = pkt->length;
						xlink_dispatcher_event_add(EVENT_TX, event);
						*event_queued = 1;
					} else {
						rc = X_LINK_ERROR;
					}
				}
			}
			release_channel(opchan);
			break;
		case XLINK_RELEASE_REQ:
			opchan = get_channel(chan);
			if (opchan == NULL) {
				rc = X_LINK_COMMUNICATION_FAIL;
			} else {
				rc = release_packet_from_channel(opchan, &opchan->rx_queue,
						event->data, &size);
				if (rc) {
					rc = X_LINK_ERROR;
				} else {
					event->header.size = size;
					xlink_dispatcher_event_add(EVENT_TX, event);
					*event_queued = 1;
				}
			}
			release_channel(opchan);
			break;
		case XLINK_OPEN_CHANNEL_REQ:
			if (mux->channels[chan].status == CHAN_CLOSED) {
				mux->channels[chan].handle = event->handle;
				mux->channels[chan].size = event->header.size;
				mux->channels[chan].timeout = event->header.timeout;
				mux->channels[chan].mode = *(enum xlink_opmode *)event->data;
				rc = multiplexer_open_channel(chan);
				if (rc) {
					rc = X_LINK_ERROR;
				} else {
					opchan = get_channel(chan);
					if (opchan == NULL) {
						rc = X_LINK_COMMUNICATION_FAIL;
					} else {
						xlink_dispatcher_event_add(EVENT_TX, event);
						*event_queued = 1;
						mutex_unlock(&opchan->lock);
						rc = wait_for_completion_interruptible_timeout(
								&opchan->opened,
								msecs_to_jiffies(OPEN_CHANNEL_TIMEOUT_MSEC));
						mutex_lock(&opchan->lock);
						if (rc == 0) {
							rc = X_LINK_TIMEOUT;
						} else if (rc > 0) {
							rc = X_LINK_SUCCESS;
						} else if (rc < 0) {
							// wait interrupted
							rc = X_LINK_ERROR;
						}
						if (rc == 0) {
							mux->channels[chan].status = CHAN_OPEN;
							release_channel(opchan);
						} else {
							multiplexer_close_channel(opchan);
						}
					}
				}
			} else if (mux->channels[chan].status == CHAN_OPEN_PEER) {
				/* channel already open */
				mux->channels[chan].status = CHAN_OPEN; // opened locally
				mux->channels[chan].handle = event->handle;
				mux->channels[chan].size = event->header.size;
				mux->channels[chan].timeout = event->header.timeout;
				mux->channels[chan].mode = *(enum xlink_opmode *)event->data;
				rc = multiplexer_open_channel(chan);
				rc = X_LINK_SUCCESS;
			} else {
				/* channel already open */
				rc = X_LINK_ALREADY_OPEN;
			}
			break;
		case XLINK_CLOSE_CHANNEL_REQ:
			if (mux->channels[chan].status == CHAN_OPEN) {
				opchan = get_channel(chan);
				rc = multiplexer_close_channel(opchan);
				if (rc) {
					rc = X_LINK_ERROR;
				} else {
					mux->channels[chan].status = CHAN_CLOSED;
					xlink_dispatcher_event_add(EVENT_TX, event);
					*event_queued = 1;
				}
			} else {
				/* can't close channel not open */
				rc = X_LINK_ERROR;
			}
			break;
		case XLINK_PING_REQ:
			break;
		case XLINK_WRITE_RESP:
		case XLINK_WRITE_VOLATILE_RESP:
		case XLINK_WRITE_CONTROL_RESP:
		case XLINK_READ_RESP:
		case XLINK_READ_TO_BUFFER_RESP:
		case XLINK_RELEASE_RESP:
		case XLINK_OPEN_CHANNEL_RESP:
		case XLINK_CLOSE_CHANNEL_RESP:
		case XLINK_PING_RESP:
		default:
			rc = X_LINK_ERROR;
		}
	}
	return rc;
}

enum xlink_error xlink_multiplexer_rx(struct xlink_event *event)
{
	enum xlink_error rc = X_LINK_SUCCESS;
	struct open_channel *opchan = NULL;
	void *buffer = NULL;
	size_t size = 0;
	uint16_t chan;
	int interface;
	dma_addr_t paddr;

	if ((mux == NULL) || (event == NULL)) {
		return X_LINK_ERROR;
	}
	chan = event->header.chan;

	switch (event->header.type) {
	case XLINK_WRITE_REQ:
	case XLINK_WRITE_VOLATILE_REQ:
		opchan = get_channel(chan);
		if (opchan == NULL) {
			rc = X_LINK_COMMUNICATION_FAIL;
		} else {
			event->header.timeout = opchan->chan->timeout;
			interface = opchan->chan->handle->dev_type;
			// printk(KERN_DEBUG "Write of size %u on channel 0x%x, rx fill level = %u out of %u\n",
			// 		event->header.size, chan, opchan->rx_fill_level,
			// 		opchan->chan->size);
			buffer = xlink_platform_allocate(mux->dev, &paddr,
					event->header.size, XLINK_PACKET_ALIGNMENT);
			if (buffer != NULL) {
				size = event->header.size;
				rc = xlink_platform_read(interface, event->handle->fd, buffer, &size,
						opchan->chan->timeout);
				if (rc || event->header.size != size) {
					xlink_platform_deallocate(mux->dev, buffer, paddr,
							event->header.size, XLINK_PACKET_ALIGNMENT);
					rc = X_LINK_ERROR;
					release_channel(opchan);
					break;
				}
				event->data = buffer;
				if (add_packet_to_channel(opchan, &opchan->rx_queue,
						event->data, event->header.size, paddr)) {
					xlink_platform_deallocate(mux->dev, buffer, paddr,
							event->header.size, XLINK_PACKET_ALIGNMENT);
					rc = X_LINK_ERROR;
					release_channel(opchan);
					break;
				}
				event->header.type = XLINK_WRITE_VOLATILE_RESP;
				xlink_dispatcher_event_add(EVENT_RX, event);
				//complete regardless of mode/timeout
				complete(&opchan->pkt_available);
			} else {
				// failed to allocate buffer
				rc = X_LINK_ERROR;
			}
		}
		release_channel(opchan);
		break;
	case XLINK_WRITE_CONTROL_REQ:
		opchan = get_channel(chan);
		if (opchan == NULL) {
			rc = X_LINK_COMMUNICATION_FAIL;
		} else {
			event->header.timeout = opchan->chan->timeout;
			interface = opchan->chan->handle->dev_type;
			// printk(KERN_DEBUG "Write of size %u on channel 0x%x, rx fill level = %u out of %u\n",
			// 		event->header.size, chan, opchan->rx_fill_level,
			// 		opchan->chan->size);
			buffer = xlink_platform_allocate(mux->dev, &paddr,
					event->header.size, XLINK_PACKET_ALIGNMENT);
			if (buffer != NULL) {
				size = event->header.size;
				memcpy(buffer, event->header.control_data, size);
				event->data = buffer;
				if (add_packet_to_channel(opchan, &opchan->rx_queue,
						event->data, event->header.size, paddr)) {
					xlink_platform_deallocate(mux->dev, buffer, paddr,
							event->header.size, XLINK_PACKET_ALIGNMENT);
					rc = X_LINK_ERROR;
					release_channel(opchan);
					break;
				}
				event->header.type = XLINK_WRITE_CONTROL_RESP;
				xlink_dispatcher_event_add(EVENT_RX, event);
				// channel blocking, notify waiting threads of available packet
				complete(&opchan->pkt_available);
			} else {
				// failed to allocate buffer
				rc = X_LINK_ERROR;
			}
		}
		release_channel(opchan);
		break;
	case XLINK_READ_REQ:
	case XLINK_READ_TO_BUFFER_REQ:
		opchan = get_channel(chan);
		if (opchan == NULL) {
			rc = X_LINK_COMMUNICATION_FAIL;
		} else {
			event->header.timeout = opchan->chan->timeout;
			event->header.type = XLINK_READ_TO_BUFFER_RESP;
			xlink_dispatcher_event_add(EVENT_RX, event);
			//complete regardless of mode/timeout
			complete(&opchan->pkt_consumed);
		}
		release_channel(opchan);
		break;
	case XLINK_RELEASE_REQ:
		opchan = get_channel(chan);
		if (opchan == NULL) {
			rc = X_LINK_COMMUNICATION_FAIL;
		} else {
			event->header.timeout = opchan->chan->timeout;
			opchan->tx_fill_level -= event->header.size;
			if(opchan->tx_fill_level < 0)
				opchan->tx_fill_level = 0;
			opchan->tx_packet_level--;
			if(opchan->tx_packet_level < 0)
				opchan->tx_packet_level = 0;;
			event->header.type = XLINK_RELEASE_RESP;
			xlink_dispatcher_event_add(EVENT_RX, event);
			//complete regardless of mode/timeout
			complete(&opchan->pkt_released);
		}
		release_channel(opchan);
		break;
	case XLINK_OPEN_CHANNEL_REQ:
		if (mux->channels[chan].status == CHAN_CLOSED) {
			mux->channels[chan].handle = event->handle;
			mux->channels[chan].size = event->header.size;
			mux->channels[chan].timeout = event->header.timeout;
			//mux->channels[chan].mode = *(enum xlink_opmode *)event->data;
			rc = multiplexer_open_channel(chan);
			if (rc) {
				rc = X_LINK_ERROR;
			} else {
				opchan = get_channel(chan);
				if (opchan == NULL) {
					rc = X_LINK_COMMUNICATION_FAIL;
				} else {
					mux->channels[chan].status = CHAN_OPEN_PEER;
					complete(&opchan->opened);
					opchan->is_opened = 1;
					event->header.timeout = opchan->chan->timeout;
					event->header.type = XLINK_OPEN_CHANNEL_RESP;
					xlink_dispatcher_event_add(EVENT_RX, event);
					release_channel(opchan);
				}
			}
		} else {
			/* channel already open */
			rc = X_LINK_ALREADY_OPEN;
		}
		break;
	case XLINK_CLOSE_CHANNEL_REQ:
		opchan = get_channel(chan);
		if (opchan == NULL) {
			rc = X_LINK_COMMUNICATION_FAIL;
		} else {
			opchan->is_opened = 0;
			event->header.timeout = opchan->chan->timeout;
			event->header.type = XLINK_CLOSE_CHANNEL_RESP;
			xlink_dispatcher_event_add(EVENT_RX, event);
		}
		release_channel(opchan);
		break;
	case XLINK_PING_REQ:
		break;
	case XLINK_WRITE_RESP:
	case XLINK_WRITE_VOLATILE_RESP:
	case XLINK_WRITE_CONTROL_RESP:
		opchan = get_channel(chan);
		if (opchan == NULL) {
			rc = X_LINK_COMMUNICATION_FAIL;
		} else {
			opchan->tx_fill_level += event->header.size;
			opchan->tx_packet_level++;
			// printk(KERN_DEBUG "Write of size %u on channel 0x%x, tx fill level = %u out of %u\n",
			// 		event->header.size, chan, opchan->tx_fill_level,
			// 		opchan->chan->size);
		}
		release_channel(opchan);
		break;
	case XLINK_READ_RESP:
	case XLINK_READ_TO_BUFFER_RESP:
	case XLINK_RELEASE_RESP:
		break;
	case XLINK_OPEN_CHANNEL_RESP:
		opchan = get_channel(chan);
		if (opchan == NULL) {
			rc = X_LINK_COMMUNICATION_FAIL;
		} else {
			complete(&opchan->opened);
			opchan->is_opened = 1;
		}
		release_channel(opchan);
		break;
	case XLINK_CLOSE_CHANNEL_RESP:
	case XLINK_PING_RESP:
	default:
		rc = X_LINK_ERROR;
	}

#ifdef CONFIG_XLINK_LOCAL_HOST
	if (chan < XLINK_IPC_MAX_CHANNELS) {
		// event should be handled by passthrough
		// TODO: implement passthrough from remote to ip
		printk(KERN_DEBUG "Passthrough: forwarding event to ipc...\n");
	}
#endif

	return rc;
}

enum xlink_error xlink_passthrough(struct xlink_event *event)
{
	enum xlink_error rc = 0;
#ifdef CONFIG_XLINK_LOCAL_HOST
	void *vaddr = 0;
	uint32_t paddr;
	uint32_t timeout = 0;
	uint32_t thismessage;
	struct xlink_ipc_fd ipc = {0};

	ipc.node = event->handle->node;
	ipc.chan = event->header.chan;;

	switch (event->header.type) {
	case XLINK_WRITE_REQ:
		if (mux->channels[ipc.chan].status == CHAN_OPEN) {
			rc = xlink_platform_write(IPC_DEVICE, &ipc, event->data,
					event->header.size, 0);
		} else {
			/* channel not open */
			rc = X_LINK_ERROR;
		}
		break;
	case XLINK_WRITE_VOLATILE_REQ:
		if (mux->channels[ipc.chan].status == CHAN_OPEN) {
			if (get_next_xlink_buf(&vaddr, 128)) {
				rc = X_LINK_ERROR;
			} else {
				memcpy(vaddr, event->data, event->header.size);
			}
			if (xlink_virt_to_phys(&mux->local_xlink_mem, vaddr, &paddr)) {
				rc = X_LINK_ERROR;
			} else {
				rc = xlink_platform_write(IPC_DEVICE, &ipc, &paddr,
						event->header.size, 0);
			}
		} else {
			/* channel not open */
			rc = X_LINK_ERROR;
		}
		break;
	case XLINK_WRITE_CONTROL_REQ:
		if (mux->channels[ipc.chan].status == CHAN_OPEN) {
			if (get_next_xlink_buf(&vaddr, 128)) {
				rc = X_LINK_ERROR;
			} else {
				memcpy(vaddr, event->header.control_data, event->header.size);
			}
			if (xlink_virt_to_phys(&mux->local_xlink_mem, vaddr, &paddr)) {
				rc = X_LINK_ERROR;
			} else {
				rc = xlink_platform_write(IPC_DEVICE, &ipc, &paddr,
						event->header.size, 0);
			}
		} else {
			/* channel not open */
			rc = X_LINK_ERROR;
		}
		break;
	case XLINK_READ_REQ:
		if (mux->channels[ipc.chan].status == CHAN_OPEN) {
			/* if channel has receive blocking set,
			 * then set timeout to U32_MAX
			 */
			if (mux->channels[ipc.chan].mode == RXB_TXN ||
					mux->channels[ipc.chan].mode == RXB_TXB) {
				timeout = U32_MAX;
			} else {
				timeout = mux->channels[ipc.chan].timeout;
			}
			rc = xlink_platform_read(IPC_DEVICE, &ipc, event->pdata, //????
					(size_t *)event->length, timeout);
		} else {
			/* channel not open */
			rc = X_LINK_ERROR;
		}
		break;

	case XLINK_READ_TO_BUFFER_REQ:
		if (mux->channels[ipc.chan].status == CHAN_OPEN) {
			/* if channel has receive blocking set,
			 * then set timeout to U32_MAX
			 */
			if (mux->channels[ipc.chan].mode == RXB_TXN ||
					mux->channels[ipc.chan].mode == RXB_TXB) {
				timeout = U32_MAX;
			} else {
				timeout = mux->channels[ipc.chan].timeout;
			}
			rc = xlink_platform_read(IPC_DEVICE, &ipc, &thismessage,
					(size_t *)event->length, timeout);
			if (rc || *event->length > XLINK_MAX_BUF_SIZE) {
				rc = X_LINK_ERROR;
			} else {
				vaddr = xlink_phys_to_virt(&mux->remote_xlink_mem,
						thismessage);
				if (vaddr) {
					memcpy(event->data, vaddr, *event->length);
				} else {
					rc = X_LINK_ERROR;
				}
			}
		} else {
			/* channel not open */
			rc = X_LINK_ERROR;
		}
		break;
	case XLINK_RELEASE_REQ:
		break;
	case XLINK_OPEN_CHANNEL_REQ:
		if (mux->channels[ipc.chan].status == CHAN_CLOSED) {
			mux->channels[ipc.chan].handle = event->handle;
			mux->channels[ipc.chan].size = event->header.size;
			mux->channels[ipc.chan].timeout = event->header.timeout;
			mux->channels[ipc.chan].mode = *(enum xlink_opmode *)event->data;
			rc = intel_keembay_vpu_ipc_open_channel(ipc.node, ipc.chan);
			if (rc) {
				rc = X_LINK_ERROR;
			} else {
				mux->channels[ipc.chan].status = CHAN_OPEN;
			}
		} else {
			/* channel already open */
			rc = X_LINK_ALREADY_OPEN;
		}
		break;
	case XLINK_CLOSE_CHANNEL_REQ:
		if (mux->channels[ipc.chan].status == CHAN_OPEN) {
			rc = intel_keembay_vpu_ipc_close_channel(ipc.node, ipc.chan);
			if (rc)
				rc = X_LINK_ERROR;
			else
				mux->channels[ipc.chan].status = CHAN_CLOSED;
		} else {
			/* can't close channel not open */
			rc = X_LINK_ERROR;
		}
		break;
	case XLINK_PING_REQ:
	case XLINK_WRITE_RESP:
	case XLINK_WRITE_VOLATILE_RESP:
	case XLINK_WRITE_CONTROL_RESP:
	case XLINK_READ_RESP:
	case XLINK_READ_TO_BUFFER_RESP:
	case XLINK_RELEASE_RESP:
	case XLINK_OPEN_CHANNEL_RESP:
	case XLINK_CLOSE_CHANNEL_RESP:
	case XLINK_PING_RESP:
		break;
	default:
		rc = X_LINK_ERROR;
	}
#else 
	rc = X_LINK_ERROR;
#endif // CONFIG_XLINK_LOCAL_HOST
	return rc;
}

enum xlink_error xlink_multiplexer_destroy(void)
{
	struct open_channel *opchan, *tmp;

	if (mux == NULL) {
		return X_LINK_ERROR;
	}

#ifdef CONFIG_XLINK_LOCAL_HOST
	/*
	 * No need to de-alloc xlink mem (local_xlink_mem and remote_xlink_mem)
	 * since it was allocated with dmam_alloc.
	 */
	xlink_reserved_memory_remove(mux);
#endif
	// close all open channels and deallocate remaining packets
	list_for_each_entry_safe(opchan, tmp, &mux->open_channels, list) {
		multiplexer_close_channel(opchan);
	}
	// destroy multiplexer
	mutex_destroy(&mux->lock);
	kfree(mux);

	return X_LINK_SUCCESS;
}
