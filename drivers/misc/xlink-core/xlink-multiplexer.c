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
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dma-direct.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/completion.h>
#include <linux/sched/signal.h>

#ifdef CONFIG_XLINK_LOCAL_HOST
#include <linux/xlink-ipc.h>
#endif

#include "xlink-multiplexer.h"
#include "xlink-dispatcher.h"
#include "xlink-platform.h"

// timeout used for open channel
#define OPEN_CHANNEL_TIMEOUT_MSEC 5000

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

struct channel {
	struct xlink_handle	*handle;
	struct open_channel *opchan;
	enum xlink_opmode mode;
	enum xlink_channel_status status;
	struct task_struct *ready_calling_pid;
	void *ready_callback;
	struct task_struct *consumed_calling_pid;
	void *consumed_callback;
	char callback_origin;
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
	struct packet_queue rx_queue;
	struct packet_queue tx_queue;
	int32_t rx_fill_level;
	int32_t tx_fill_level;
	int32_t tx_packet_level;
	struct completion opened;
	struct completion pkt_available;
	struct completion pkt_consumed;
	struct completion pkt_released;
	struct mutex lock;
};

struct xlink_multiplexer {
	struct device *dev;
	struct channel channels[NMB_CHANNELS];
};

struct xlink_multiplexer *mux = 0;

/*
 * Multiplexer Internal Functions
 *
 */

static enum xlink_error run_callback(const uint16_t chan, void *callback,
		struct task_struct *pid)
{
	enum xlink_error rc = X_LINK_SUCCESS;
	int ret;
	void(*func)(int);
#if KERNEL_VERSION(4, 20, 0) > LINUX_VERSION_CODE
	struct siginfo info;
	memset(&info, 0, sizeof(struct siginfo));
#else
	struct kernel_siginfo info;
	memset(&info, 0, sizeof(struct kernel_siginfo));
#endif

	if (mux->channels[chan].callback_origin == 'U') { // user-space origin
		if (pid != NULL) {
			info.si_signo = SIGXLNK;
			info.si_code = SI_QUEUE;
			info.si_errno = chan;
			info.si_ptr = callback;
			if ((ret = send_sig_info(SIGXLNK, &info, pid)) < 0) {
				printk(KERN_INFO "Unable to send signal %d\n", ret);
				rc = X_LINK_ERROR;
			}
		}
		else {
			printk(KERN_DEBUG "CHAN %x -- calling_pid == NULL\n", chan);
			rc = X_LINK_ERROR;
		}
	} else { // kernel origin
		func = callback;
		func(chan);
	}
	return rc;
}

static inline int chan_is_non_blocking_read(uint16_t chan)
{
	if ((mux->channels[chan].mode == RXN_TXN) ||
			(mux->channels[chan].mode == RXN_TXB)) {
		return 1;
	}
	return 0;
}
static inline int chan_is_non_blocking_write(uint16_t chan)
{
	if ((mux->channels[chan].mode == RXN_TXN) ||
			(mux->channels[chan].mode == RXB_TXN)) {
		return 1;
	}
	return 0;
}


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
	if (chan_type) {
		if (dev_type == IPC_DEVICE) {
			if (chan_type->local_to_ip == dev_type)
				return 1;
		} else {
			if (chan_type->remote_to_local == dev_type)
				return 1;
		}
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
	if (!mux->channels[chan].opchan)
		return NULL;
	mutex_lock(&mux->channels[chan].opchan->lock);
	return mux->channels[chan].opchan;
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

	if (queue->count < queue->capacity) {
		pkt = kzalloc(sizeof(*pkt), GFP_KERNEL);
		if (!pkt)
			return X_LINK_ERROR;

		pkt->data = buffer;
		pkt->length = size;
		pkt->paddr = paddr;
		list_add_tail(&pkt->list, &queue->head);
		queue->count++;
		opchan->rx_fill_level += pkt->length;
	}
	return X_LINK_SUCCESS;
}

static struct packet *get_packet_from_channel(struct packet_queue *queue)
{
	struct packet *pkt = NULL;

	// get first packet in queue
	if (!list_empty(&queue->head)) {
		pkt = list_first_entry(&queue->head, struct packet, list);
	}
	return pkt;
}

static int release_packet_from_channel(struct open_channel *opchan,
		struct packet_queue *queue, uint8_t * const addr, uint32_t* size)
{
	uint8_t packet_found = 0;
	struct packet *pkt = NULL;

	if (!addr) {
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
		return X_LINK_ERROR;
	}
	// packet found, deallocate and remove from queue
	xlink_platform_deallocate(mux->dev, pkt->data, pkt->paddr, pkt->length,
			XLINK_PACKET_ALIGNMENT);
	list_del(&pkt->list);
	queue->count--;
	opchan->rx_fill_level -= pkt->length;
	if (opchan->rx_fill_level < 0)
		opchan->rx_fill_level = 0;
	if (size) {
		*size = pkt->length;
	}
	kfree(pkt);
	// printk(KERN_DEBUG "Release of %u on channel 0x%x: rx fill level = %u out of %u\n",
	// 		pkt->length, opchan->id, opchan->rx_fill_level,
	// 		opchan->chan->size);
	return X_LINK_SUCCESS;
}

static int multiplexer_open_channel(uint16_t chan)
{
	struct open_channel* opchan = NULL;

	// allocate open channel
	opchan = kzalloc(sizeof(*opchan), GFP_KERNEL);
	if (!opchan)
		return X_LINK_ERROR;

	// initialize open channel
	opchan->id = chan;
	opchan->chan = &mux->channels[chan];
	mux->channels[chan].opchan = opchan; // TODO: remove circular dependency
	INIT_LIST_HEAD(&opchan->rx_queue.head);
	opchan->rx_queue.count = 0;
	opchan->rx_queue.capacity = XLINK_PACKET_QUEUE_CAPACITY;
	INIT_LIST_HEAD(&opchan->tx_queue.head);
	opchan->tx_queue.count = 0;
	opchan->tx_queue.capacity = XLINK_PACKET_QUEUE_CAPACITY;
	opchan->rx_fill_level = 0;
	opchan->tx_fill_level = 0;
	opchan->tx_packet_level = 0;
	init_completion(&opchan->opened);
	init_completion(&opchan->pkt_available);
	init_completion(&opchan->pkt_consumed);
	init_completion(&opchan->pkt_released);
	mutex_init(&opchan->lock);
	return X_LINK_SUCCESS;
}

static int multiplexer_close_channel(struct open_channel *opchan)
{
	if (!opchan)
		return X_LINK_ERROR;

	// free remaining packets
	while (!list_empty(&opchan->rx_queue.head)) {
		release_packet_from_channel(opchan, &opchan->rx_queue, NULL, NULL);
	}
	while (!list_empty(&opchan->tx_queue.head)) {
		release_packet_from_channel(opchan, &opchan->tx_queue, NULL, NULL);
	}
	// deallocate data structure and destroy
	opchan->chan->opchan = NULL; // TODO: remove circular dependency
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
	if (mux) {
		printk(KERN_DEBUG "Multiplexer already initialized\n");
		return X_LINK_ERROR;
	}

	// allocate multiplexer data structure
	mux_init = kzalloc(sizeof(*mux_init), GFP_KERNEL);
	if (!mux_init)
		return X_LINK_ERROR;

	mux_init->dev = &plat_dev->dev;

	// set the global reference to multiplexer
	mux = mux_init;

	// open ip control channel
	mux->channels[IP_CONTROL_CHANNEL].handle = NULL;
	mux->channels[IP_CONTROL_CHANNEL].size = CONTROL_CHANNEL_DATASIZE;
	mux->channels[IP_CONTROL_CHANNEL].timeout = CONTROL_CHANNEL_TIMEOUT_MS;
	mux->channels[IP_CONTROL_CHANNEL].mode = CONTROL_CHANNEL_OPMODE;
	rc = multiplexer_open_channel(IP_CONTROL_CHANNEL);
	if (rc) {
		rc = X_LINK_ERROR;
		goto r_cleanup;
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
		goto r_cleanup;
	} else {
		mux->channels[VPU_CONTROL_CHANNEL].status = CHAN_OPEN;
	}
	return X_LINK_SUCCESS;

r_cleanup:
	mux = mux_init;
	xlink_multiplexer_destroy();
	return rc;
}

enum xlink_error xlink_multiplexer_tx(struct xlink_event *event, int *event_queued)
{
	int rc = X_LINK_SUCCESS;
	struct open_channel *opchan = NULL;
	struct packet *pkt = NULL;
	uint32_t size = 0;
	uint16_t chan;

	if (!mux || !event)
		return X_LINK_ERROR;

	chan = event->header.chan;

	// verify channel ID is in range
	if (chan >= NMB_CHANNELS)
		return X_LINK_ERROR;

	// verify device type can communicate on channel ID
	if (!is_channel_for_device_type(chan, event->handle->dev_type))
		return X_LINK_ERROR;

	// verify this is not a control channel
	if (is_control_channel(chan))
		return X_LINK_ERROR;

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
			if (!opchan || (opchan->chan->status != CHAN_OPEN)) {
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
					rc = xlink_dispatcher_event_add(EVENT_TX, event);
					if (rc == X_LINK_SUCCESS) {
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
			}
			release_channel(opchan);
			break;
		case XLINK_READ_REQ:
			opchan = get_channel(chan);
			if (!opchan || (opchan->chan->status != CHAN_OPEN)) {
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
			if (!opchan || (opchan->chan->status != CHAN_OPEN)) {
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
			if (!opchan) {
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
					if (!opchan) {
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
		case XLINK_DATA_READY_CALLBACK_REQ:
				mux->channels[chan].ready_callback = event->data;
				mux->channels[chan].ready_calling_pid = event->calling_pid;
				mux->channels[chan].callback_origin = event->callback_origin;
				printk(KERN_DEBUG "callback process registered - XLINK_DATA_READY_CALLBACK_REQ %lx chan %d\n",(uintptr_t)event->calling_pid, chan);
			break;
		case XLINK_DATA_CONSUMED_CALLBACK_REQ:
				mux->channels[chan].consumed_callback = event->data;
				mux->channels[chan].consumed_calling_pid = event->calling_pid;
				mux->channels[chan].callback_origin = event->callback_origin;
				printk(KERN_DEBUG "callback process registered - XLINK_DATA_CONSUMED_CALLBACK_REQ %lx chan %d\n",(uintptr_t)event->calling_pid, chan);
			break;
		case XLINK_CLOSE_CHANNEL_REQ:
			if (mux->channels[chan].status == CHAN_OPEN) {
				opchan = get_channel(chan);
				rc = multiplexer_close_channel(opchan);
				if (rc) {
					rc = X_LINK_ERROR;
				} else {
					mux->channels[chan].status = CHAN_CLOSED;
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
	int rc = X_LINK_SUCCESS;
	struct open_channel *opchan = NULL;
	void *buffer = NULL;
	size_t size = 0;
	uint16_t chan;
	int interface;
	dma_addr_t paddr;

	if (!mux || !event)
		return X_LINK_ERROR;

	chan = event->header.chan;

	switch (event->header.type) {
	case XLINK_WRITE_REQ:
	case XLINK_WRITE_VOLATILE_REQ:
		opchan = get_channel(chan);
		if (!opchan) {
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
				// run callback
				if (mux->channels[chan].status == CHAN_OPEN &&
						chan_is_non_blocking_read(chan) &&
						mux->channels[chan].ready_callback != NULL) {
					rc = run_callback(chan, mux->channels[chan].ready_callback,
							mux->channels[chan].ready_calling_pid);
					break;
				}
			} else {
				// failed to allocate buffer
				rc = X_LINK_ERROR;
			}
		}
		release_channel(opchan);
		break;
	case XLINK_WRITE_CONTROL_REQ:
		opchan = get_channel(chan);
		if (!opchan) {
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
		if (!opchan) {
			rc = X_LINK_COMMUNICATION_FAIL;
		} else {
			event->header.timeout = opchan->chan->timeout;
			event->header.type = XLINK_READ_TO_BUFFER_RESP;
			xlink_dispatcher_event_add(EVENT_RX, event);
			//complete regardless of mode/timeout
			complete(&opchan->pkt_consumed);
		}
		// run callback
		if (mux->channels[chan].status == CHAN_OPEN &&
				chan_is_non_blocking_write(chan) &&
				mux->channels[chan].consumed_callback != NULL) {
			rc = run_callback(chan, mux->channels[chan].consumed_callback,
					mux->channels[chan].consumed_calling_pid);
		}
		release_channel(opchan);
		break;
	case XLINK_RELEASE_REQ:
		opchan = get_channel(chan);
		if (!opchan) {
			rc = X_LINK_COMMUNICATION_FAIL;
		} else {
			event->header.timeout = opchan->chan->timeout;
			opchan->tx_fill_level -= event->header.size;
			if (opchan->tx_fill_level < 0)
				opchan->tx_fill_level = 0;
			opchan->tx_packet_level--;
			if (opchan->tx_packet_level < 0)
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
				if (!opchan) {
					rc = X_LINK_COMMUNICATION_FAIL;
				} else {
					mux->channels[chan].status = CHAN_OPEN_PEER;
					complete(&opchan->opened);
					event->header.timeout = opchan->chan->timeout;
					event->header.type = XLINK_OPEN_CHANNEL_RESP;
					xlink_dispatcher_event_add(EVENT_RX, event);
				}
				release_channel(opchan);
			}
		} else {
			/* channel already open */
			rc = X_LINK_ALREADY_OPEN;
		}
		break;
	case XLINK_CLOSE_CHANNEL_REQ:
	case XLINK_PING_REQ:
		break;
	case XLINK_WRITE_RESP:
	case XLINK_WRITE_VOLATILE_RESP:
	case XLINK_WRITE_CONTROL_RESP:
		opchan = get_channel(chan);
		if (!opchan) {
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
		if (!opchan) {
			rc = X_LINK_COMMUNICATION_FAIL;
		} else {
			complete(&opchan->opened);
		}
		release_channel(opchan);
		break;
	case XLINK_CLOSE_CHANNEL_RESP:
	case XLINK_PING_RESP:
		break;
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
	int rc = 0;
#ifdef CONFIG_XLINK_LOCAL_HOST
	dma_addr_t vpuaddr = 0;
	phys_addr_t physaddr = 0;
	uint32_t timeout = 0;
	struct xlink_ipc_fd ipc = {0};

	ipc.sw_device_id = event->handle->sw_device_id;
	ipc.node = event->handle->node;
	ipc.chan = event->header.chan;

	switch (event->header.type) {
	case XLINK_WRITE_REQ:
		if (mux->channels[ipc.chan].status == CHAN_OPEN) {
			/* Translate physical address to VPU address */
			vpuaddr = phys_to_dma(mux->dev, *(uint32_t*)event->data);
			event->data = &vpuaddr;
			rc = xlink_platform_write(IPC_DEVICE, &ipc, event->data,
					&event->header.size, 0);
		} else {
			/* channel not open */
			rc = X_LINK_ERROR;
		}
		break;
	case XLINK_WRITE_VOLATILE_REQ:
		if (mux->channels[ipc.chan].status == CHAN_OPEN) {
			ipc.is_volatile = 1;
			rc = xlink_platform_write(IPC_DEVICE, &ipc, event->data,
					&event->header.size, 0);
		} else {
			/* channel not open */
			rc = X_LINK_ERROR;
		}
		break;
	case XLINK_WRITE_CONTROL_REQ:
		if (mux->channels[ipc.chan].status == CHAN_OPEN) {
			ipc.is_volatile = 1;
			rc = xlink_platform_write(IPC_DEVICE, &ipc, event->header.control_data,
					&event->header.size, 0);
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
			rc = xlink_platform_read(IPC_DEVICE, &ipc, &vpuaddr,
					(size_t *)event->length, timeout);
			/* Translate VPU address to physical address */
			physaddr = dma_to_phys(mux->dev, vpuaddr);
			*(phys_addr_t *)event->pdata = physaddr;
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
			ipc.is_volatile = 1;
			rc = xlink_platform_read(IPC_DEVICE, &ipc, event->data,
					(size_t *)event->length, timeout);
			if (rc || *event->length > XLINK_MAX_BUF_SIZE) {
				rc = X_LINK_ERROR;
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
			rc = xlink_platform_open_channel(IPC_DEVICE, &ipc, ipc.chan);
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
			rc = xlink_platform_close_channel(IPC_DEVICE, &ipc, ipc.chan);
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
	int i = 0;

	if (!mux)
		return X_LINK_ERROR;

	// close all open channels and deallocate remaining packets
	for (i = 0; i < NMB_CHANNELS; i++) {
		if (mux->channels[i].opchan)
			multiplexer_close_channel(mux->channels[i].opchan);
	}
	// destroy multiplexer
	kfree(mux);

	return X_LINK_SUCCESS;
}
