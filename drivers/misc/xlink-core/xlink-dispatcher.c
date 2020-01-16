// SPDX-License-Identifier: GPL-2.0-only
/*
 * xlink Dispatcher.
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/sched/signal.h>
#include <linux/platform_device.h>

#include "xlink-dispatcher.h"
#include "xlink-multiplexer.h"
#include "xlink-platform.h"

#define DISPATCHER_RX_TIMEOUT_MSEC 0

enum dispatcher_state {
	XLINK_DISPATCHER_NOT_INIT,
	XLINK_DISPATCHER_INIT,
	XLINK_DISPATCHER_RUNNING,
	XLINK_DISPATCHER_STOPPED,
};

struct event_queue {
	uint32_t count;
	uint32_t capacity;
	struct list_head head;
	struct mutex lock;
};

struct xlink_dispatcher {
	enum dispatcher_state state;
	struct task_struct *rxthread;
	struct task_struct *txthread;
	struct event_queue queue;
	struct semaphore event_sem;
	struct completion rx_done;
	struct completion tx_done;
	struct mutex lock;
};

static struct xlink_dispatcher disp[XLINK_MAX_CONNECTIONS];
static struct device *disp_dev;

/*
 * Dispatcher Internal Functions
 *
 */

static uint32_t event_generate_id(void)
{
	static uint32_t id = 0xa; // TODO: temporary solution
	return id++;
}

static struct xlink_event *event_dequeue(struct event_queue *queue)
{
	struct xlink_event *event = NULL;
	mutex_lock(&queue->lock);
	if (!list_empty(&queue->head)) {
		event = list_first_entry(&queue->head, struct xlink_event, list);
		list_del(&event->list);
		queue->count--;
	}
	mutex_unlock(&queue->lock);
	return event;
}

static int event_enqueue(struct event_queue *queue, struct xlink_event *event)
{
	int rc = -1;
	mutex_lock(&queue->lock);
	if(queue->count < queue->capacity) {
		list_add_tail(&event->list, &queue->head);
		queue->count++;
		rc = 0;
	}
	mutex_unlock(&queue->lock);
	return rc;
}

static struct xlink_event *dispatcher_event_get(int id)
{
	int rc = 0;
	struct xlink_event *event = NULL;

	// wait until an event is available
	rc = down_interruptible(&disp[id].event_sem);
	if (!rc) {
		// dequeue and return next event to process
		event = event_dequeue(&disp[id].queue);
	}
	return event;
}

static int is_valid_event_header(struct xlink_event *event)
{
	if (event->header.magic != XLINK_EVENT_HEADER_MAGIC)
		return 0;
	else
		return 1;
}

static int dispatcher_event_send(struct xlink_event *event)
{
	int rc = 0;
	size_t event_header_size = sizeof(event->header);

	// write event header
	// printk(KERN_DEBUG "Sending event: type = 0x%x, id = 0x%x\n",
	// 		event->header.type, event->header.id);
	rc = xlink_platform_write(event->handle->dev_type,
			event->handle->fd, &event->header, &event_header_size,
			event->header.timeout);
	if (rc || (event_header_size != sizeof(event->header))) {
		printk(KERN_DEBUG "Write header failed %d\n", rc);
	} else {
		if ((event->header.type == XLINK_WRITE_REQ) ||
				(event->header.type == XLINK_WRITE_VOLATILE_REQ)) {
			// write event data
			rc = xlink_platform_write(event->handle->dev_type,
					event->handle->fd, event->data, &event->header.size,
					event->header.timeout);
			if (rc) {
				printk(KERN_DEBUG "Write data failed %d\n", rc);
			}
			if(event->paddr != 0){
				xlink_platform_deallocate(disp_dev, event->data, event->paddr,
						event->header.size, XLINK_PACKET_ALIGNMENT);
			}
		}
	}
	return rc;
}

static int xlink_dispatcher_rxthread(void* context)
{
	int rc = 0;
	size_t size = 0;
	struct xlink_event *event = NULL;
	struct xlink_handle *handle = (struct xlink_handle *)context;

	// printk(KERN_DEBUG "dispatcher rxthread started\n");
	event = xlink_create_event(0, handle, 0, 0, 0);
	if (!event)
		return -1;

	allow_signal(SIGTERM); // allow thread termination while waiting on sem
	complete(&disp[handle->link_id].rx_done);
	while (!kthread_should_stop()) {
		size = sizeof(event->header);
		rc = xlink_platform_read(handle->dev_type, handle->fd, &event->header,
				&size, DISPATCHER_RX_TIMEOUT_MSEC);
		if (rc || (size != (int)sizeof(event->header))) {
			continue;
		}
		if (is_valid_event_header(event)) {
			// printk(KERN_DEBUG "Incoming event: type = 0x%x, id = 0x%x\n",
			// 		event->header.type, event->header.id);
			rc = xlink_multiplexer_rx(event);
			if (!rc) {
				event = xlink_create_event(0, handle, 0, 0, 0);
				if (!event)
					return -1;
			}
		}
	}
	// printk(KERN_INFO "dispatcher rxthread stopped\n");
	complete(&disp[handle->link_id].rx_done);
	do_exit(0);
	return 0;
}

static int xlink_dispatcher_txthread(void* context)
{
	struct xlink_event *event = NULL;
	struct xlink_handle *handle = (struct xlink_handle *)context;

	// printk(KERN_DEBUG "dispatcher txthread started\n");
	allow_signal(SIGTERM); // allow thread termination while waiting on sem
	complete(&disp[handle->link_id].tx_done);
	while (!kthread_should_stop()) {
		event = dispatcher_event_get(handle->link_id);
		if (!event)
			continue;

		dispatcher_event_send(event);
		xlink_destroy_event(event); // event is handled and can now be freed
	}
	// printk(KERN_INFO "dispatcher txthread stopped\n");
	complete(&disp[handle->link_id].tx_done);
	do_exit(0);
	return 0;
}

/*
 * Dispatcher External Functions
 *
 */

enum xlink_error xlink_dispatcher_init(void *dev)
{
	int i = 0;
	struct platform_device *plat_dev = (struct platform_device *) dev;

	disp_dev = &plat_dev->dev;
	for (i = 0; i < XLINK_MAX_CONNECTIONS; i++) {
		sema_init(&disp[i].event_sem, 0);
		init_completion(&disp[i].rx_done);
		init_completion(&disp[i].tx_done);
		mutex_init(&disp[i].lock);
		INIT_LIST_HEAD(&disp[i].queue.head);
		mutex_init(&disp[i].queue.lock);
		disp[i].queue.count = 0;
		disp[i].queue.capacity = XLINK_EVENT_QUEUE_CAPACITY;
		disp[i].state = XLINK_DISPATCHER_INIT;
	}
	return X_LINK_SUCCESS;
}

enum xlink_error xlink_dispatcher_start(struct xlink_handle *handle)
{
	int id = handle->link_id;

	// don't start dispatcher without stopping first
	if (disp[id].state == XLINK_DISPATCHER_RUNNING) {
		printk(KERN_DEBUG "Dispatcher already running\n");
		return X_LINK_ERROR;
	}
	// create dispatcher thread to handle and write outgoing packets
	disp[id].txthread = kthread_run(xlink_dispatcher_txthread,
			(void *)handle, "txthread");
	if (!disp[id].txthread) {
		printk(KERN_ERR "txthread creation failed\n");
		goto r_txthread;
	}
	wait_for_completion(&disp[id].tx_done);
	disp[id].state = XLINK_DISPATCHER_RUNNING;
	// create dispatcher thread to read and handle incoming packets
	disp[id].rxthread = kthread_run(xlink_dispatcher_rxthread,
			(void *)handle, "rxthread");
	if (!disp[id].rxthread) {
		printk(KERN_ERR "rxthread creation failed\n");
		goto r_rxthread;
	}
	wait_for_completion(&disp[id].rx_done);
	return X_LINK_SUCCESS;

r_rxthread:
	kthread_stop(disp[id].txthread);
r_txthread:
	disp[id].state = XLINK_DISPATCHER_STOPPED;
	return X_LINK_ERROR;
}

enum xlink_error xlink_dispatcher_event_add(enum xlink_event_origin origin,
		struct xlink_event *event)
{
	int rc = 0;
	int id = event->handle->link_id;

	mutex_lock(&disp[id].lock);
	// only add events if the dispatcher is running
	if (disp[id].state != XLINK_DISPATCHER_RUNNING) {
		mutex_unlock(&disp[id].lock);
		return X_LINK_ERROR;
	}
	// configure event and add to queue
	if (origin == EVENT_TX) {
		event->header.id = event_generate_id();
	}
	event->origin = origin;
	rc = event_enqueue(&disp[id].queue, event);
	if (rc) {
		mutex_unlock(&disp[id].lock);
		return X_LINK_ERROR;
	}
	// notify dispatcher tx thread of new event
	up(&disp[id].event_sem);
	mutex_unlock(&disp[id].lock);
	return X_LINK_SUCCESS;
}

enum xlink_error xlink_dispatcher_stop(int id)
{
	int rc = 0;

	// don't stop dispatcher if not started
	if (disp[id].state != XLINK_DISPATCHER_RUNNING) {
		printk(KERN_DEBUG "Dispatcher not started\n");
		return X_LINK_ERROR;
	}
	if (disp[id].rxthread) {
		// stop dispatcher rx thread reading and handling incoming packets
		send_sig(SIGTERM, disp[id].rxthread, 0);
		rc = kthread_stop(disp[id].rxthread);
		if (rc) {
			printk(KERN_DEBUG "Error stopping rxthread");
			return X_LINK_ERROR;
		}
	}
	wait_for_completion(&disp[id].rx_done);
	if (disp[id].txthread) {
		// stop dispatcher tx thread handling and writing outgoing packets
		send_sig(SIGTERM, disp[id].txthread, 0);
		rc = kthread_stop(disp[id].txthread);
		if (rc) {
			printk(KERN_DEBUG "Error stopping txthread");
			return X_LINK_ERROR;
		}
	}
	wait_for_completion(&disp[id].tx_done);
	disp[id].state = XLINK_DISPATCHER_STOPPED;
	return X_LINK_SUCCESS;
}

enum xlink_error xlink_dispatcher_destroy(void)
{
	int rc = 0;
	int i = 0;
	struct xlink_event *event = NULL;

	for (i = 0; i < XLINK_MAX_CONNECTIONS; i++) {
		mutex_lock(&disp[i].lock);
		if (disp[i].state == XLINK_DISPATCHER_RUNNING) {
			rc = xlink_dispatcher_stop(i);
			if (rc != X_LINK_SUCCESS) {
				printk(KERN_DEBUG "Dispatcher stop failed\n");
				return X_LINK_ERROR;
			}
		}
		if (disp[i].state != XLINK_DISPATCHER_INIT) {
			// deallocate remaining events in queue
			while (!list_empty(&disp[i].queue.head)) {
				event = event_dequeue(&disp[i].queue);
				if (event) {
					if ((event->header.type == XLINK_WRITE_REQ) ||
							(event->header.type == XLINK_WRITE_VOLATILE_REQ)) {
						// free buffer allocated for event data
						xlink_platform_deallocate(disp_dev, event->data,
								event->paddr, event->header.size,
								XLINK_PACKET_ALIGNMENT);
					}
					xlink_destroy_event(event);
				}
			}
			// destroy dispatcher
			mutex_unlock(&disp[i].lock);
			mutex_destroy(&disp[i].lock);
			mutex_destroy(&disp[i].queue.lock);
		}
	}
	return X_LINK_SUCCESS;
}
