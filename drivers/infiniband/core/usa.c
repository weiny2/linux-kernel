/*
 * Copyright (c) 2006-2007 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *	copyright notice, this list of conditions and the following
 *	disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *	copyright notice, this list of conditions and the following
 *	disclaimer in the documentation and/or other materials
 *	provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/idr.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <rdma/ib_usa.h>
#include "sa.h"

MODULE_AUTHOR("Sean Hefty");
MODULE_DESCRIPTION("IB userspace SA");
MODULE_LICENSE("Dual BSD/GPL");

static void usa_add_one(struct ib_device *device);
static void usa_remove_one(struct ib_device *device);

static struct ib_client usa_client = {
	.name   = "ib_usa",
	.add    = usa_add_one,
	.remove = usa_remove_one
};

struct usa_device {
	struct list_head list;
	struct ib_device *device;
	struct completion comp;
	atomic_t refcount;
	int start_port;
	int end_port;
};

struct usa_file {
	struct mutex		file_mutex;
	struct file		*filp;
	struct ib_sa_client	sa_client;
	struct list_head	event_list;
	struct list_head	id_list;
	wait_queue_head_t	poll_wait;
	int			event_id;
};

struct usa_id {
	struct usa_file *file;
	struct usa_device *dev;
	struct list_head list;
	u64 uid;
	int num;
	int events_reported;
	u16 attr_id;
};

struct usa_event {
	struct usa_id *id;
	struct list_head list;
	struct ib_usa_event_resp resp;
};

struct usa_multicast {
	struct usa_id id;
	struct usa_event event;
	struct ib_sa_multicast *multicast;
};

struct usa_inform_info {
	struct usa_id id;
	struct ib_inform_info *inform_info;
};

static DEFINE_MUTEX(usa_mutex);
static LIST_HEAD(dev_list);
static DEFINE_IDR(usa_idr);

static struct usa_device *get_dev(__be64 guid, __u8 port_num)
{
	struct usa_device *dev;

	mutex_lock(&usa_mutex);
	list_for_each_entry(dev, &dev_list, list) {
		if (dev->device->node_guid == guid) {
			if (port_num < dev->start_port ||
			    port_num > dev->end_port)
				break;
			atomic_inc(&dev->refcount);
			mutex_unlock(&usa_mutex);
			return dev;
		}
	}
	mutex_unlock(&usa_mutex);
	return NULL;
}

static struct usa_device *get_dev_by_name(__u8 *name, __u8 port_num)
{
	struct usa_device *dev;

	mutex_lock(&usa_mutex);
	list_for_each_entry(dev, &dev_list, list) {
		if (strcmp(dev->device->name, name) == 0) {
			if (port_num < dev->start_port ||
			    port_num > dev->end_port)
				break;
			atomic_inc(&dev->refcount);
			mutex_unlock(&usa_mutex);
			return dev;
		}
	}
	mutex_unlock(&usa_mutex);
	return NULL;
}

static void put_dev(struct usa_device *dev)
{
	if (atomic_dec_and_test(&dev->refcount))
		complete(&dev->comp);
}

static int insert_id(struct usa_id *id)
{
	int ret;

	do {
		ret = idr_pre_get(&usa_idr, GFP_KERNEL);
		if (!ret)
			break;

		mutex_lock(&usa_mutex);
		ret = idr_get_new(&usa_idr, id, &id->num);
		mutex_unlock(&usa_mutex);
	} while (ret == -EAGAIN);

	return ret;
}

static void remove_id(struct usa_id *id)
{
	mutex_lock(&usa_mutex);
	idr_remove(&usa_idr, id->num);
	mutex_unlock(&usa_mutex);
}

static struct usa_id *get_id(int num, struct usa_file *file, u16 attr_id)
{
	struct usa_id *id;

	id = idr_find(&usa_idr, num);
	if (!id)
		return ERR_PTR(-ENOENT);

	if ((id->file != file) || (id->attr_id != attr_id))
		return ERR_PTR(-EINVAL);

	return id;
}

static void insert_file_id(struct usa_file *file, struct usa_id *id)
{
	mutex_lock(&file->file_mutex);
	list_add_tail(&id->list, &file->id_list);
	mutex_unlock(&file->file_mutex);
}

static void remove_file_id(struct usa_file *file, struct usa_id *id)
{
	mutex_lock(&file->file_mutex);
	list_del(&id->list);
	mutex_unlock(&file->file_mutex);
}

static void finish_event(struct usa_event *event)
{
	switch (be16_to_cpu(event->resp.attr_id)) {
	case IB_SA_ATTR_MC_MEMBER_REC:
		list_del_init(&event->list);
		event->id->events_reported++;
		break;
	default:
		list_del(&event->list);
		if (event->id)
			event->id->events_reported++;
		kfree(event);
		break;
	}
}

static ssize_t usa_get_event(struct usa_file *file, const char __user *inbuf,
			      int in_len, int out_len)
{
	struct ib_usa_get_event cmd;
	struct usa_event *event;
	int ret = 0;
	DEFINE_WAIT(wait);

	if (out_len < sizeof(event->resp))
		return -ENOSPC;

	if (copy_from_user(&cmd, inbuf, sizeof(cmd)))
		return -EFAULT;

	mutex_lock(&file->file_mutex);
	while (list_empty(&file->event_list)) {
		mutex_unlock(&file->file_mutex);

		if (file->filp->f_flags & O_NONBLOCK)
			return -EAGAIN;

		if (wait_event_interruptible(file->poll_wait,
					     !list_empty(&file->event_list)))
			return -ERESTARTSYS;

		mutex_lock(&file->file_mutex);
	}

	event = list_entry(file->event_list.next, struct usa_event, list);

	if (copy_to_user((void __user *)(unsigned long)cmd.response,
			 &event->resp, sizeof(event->resp))) {
		ret = -EFAULT;
		goto done;
	}

	finish_event(event);
done:
	mutex_unlock(&file->file_mutex);
	return ret;
}

static void queue_event(struct usa_file *file, struct usa_event *event)
{
	mutex_lock(&file->file_mutex);
	list_add(&event->list, &file->event_list);
	wake_up_interruptible(&file->poll_wait);
	mutex_unlock(&file->file_mutex);
}

/*
 * We can get up to two events for a single multicast member.  A second event
 * only occurs if there's an error on an existing multicast membership.
 * Report only the last event.
 */
static int multicast_handler(int status, struct ib_sa_multicast *multicast)
{
	struct usa_multicast *mcast = multicast->context;
	struct usa_file *file = mcast->id.file;

	mcast->event.resp.status = status;
	if (!status) {
		mcast->event.resp.data_len = IB_SA_ATTR_MC_MEMBER_REC_LEN;
		ib_sa_pack_attr(mcast->event.resp.data, &multicast->rec,
				IB_SA_ATTR_MC_MEMBER_REC);
	}

	mutex_lock(&file->file_mutex);
	list_move_tail(&mcast->event.list, &file->event_list);
	wake_up_interruptible(&file->poll_wait);
	mutex_unlock(&file->file_mutex);

	return 0;
}

static int join_mcast(struct usa_file *file, struct ib_usa_request *req,
		      int out_len)
{
	struct usa_multicast *mcast;
	struct ib_sa_mcmember_rec rec;
	int ret;

	if (out_len < sizeof(u32))
		return -ENOSPC;

	mcast = kzalloc(sizeof *mcast, GFP_KERNEL);
	if (!mcast)
		return -ENOMEM;

	mcast->id.dev = get_dev(req->node_guid, req->port_num);
	if (!mcast->id.dev) {
		ret = -ENODEV;
		goto err1;
	}

	if (copy_from_user(mcast->event.resp.data,
			   (void __user *) (unsigned long) req->attr,
			   IB_SA_ATTR_MC_MEMBER_REC_LEN)) {
		ret = -EFAULT;
		goto err2;
	}

	INIT_LIST_HEAD(&mcast->event.list);
	mcast->event.id = &mcast->id;
	mcast->event.resp.attr_id = cpu_to_be16(IB_SA_ATTR_MC_MEMBER_REC);
	mcast->event.resp.uid = req->uid;
	mcast->id.attr_id = IB_SA_ATTR_MC_MEMBER_REC;
	mcast->id.uid = req->uid;

	ret = insert_id(&mcast->id);
	if (ret)
		goto err2;

	mcast->event.resp.id = mcast->id.num;
	if (copy_to_user((void __user *) (unsigned long) req->response,
			 &mcast->id.num, sizeof(u32))) {
		ret = EFAULT;
		goto err3;
	}

	mcast->id.file = file;
	insert_file_id(file, &mcast->id);

	ib_sa_unpack_attr(&rec, mcast->event.resp.data,
			  IB_SA_ATTR_MC_MEMBER_REC);
	mcast->multicast = ib_sa_join_multicast(
					&file->sa_client,
					mcast->id.dev->device,
					req->port_num, &rec,
					(ib_sa_comp_mask) req->comp_mask,
					GFP_KERNEL, multicast_handler,
					mcast);
	if (IS_ERR(mcast->multicast)) {
		ret = PTR_ERR(mcast->multicast);
		goto err4;
	}

	return 0;

err4:
	remove_file_id(file, &mcast->id);
err3:
	remove_id(&mcast->id);
err2:
	put_dev(mcast->id.dev);
err1:
	kfree(mcast);
	return ret;
}

static int get_mcast(struct usa_file *file, struct ib_usa_request *req,
		     int out_len)
{
	struct usa_device *dev;
	struct ib_sa_mcmember_rec rec;
	u8 mcmember_rec[IB_SA_ATTR_MC_MEMBER_REC_LEN];
	int ret;

	if (out_len < sizeof(IB_SA_ATTR_MC_MEMBER_REC_LEN))
		return -ENOSPC;

	if (req->comp_mask != IB_SA_MCMEMBER_REC_MGID)
		return -ENOSYS;

	if (copy_from_user(mcmember_rec,
			   (void __user *) (unsigned long) req->attr,
			   IB_SA_ATTR_MC_MEMBER_REC_LEN))
		return -EFAULT;

	dev = get_dev(req->node_guid, req->port_num);
	if (!dev)
		return -ENODEV;

	ib_sa_unpack_attr(&rec, mcmember_rec, IB_SA_ATTR_MC_MEMBER_REC);
	ret = ib_sa_get_mcmember_rec(dev->device, req->port_num,
				     &rec.mgid, &rec);
	if (!ret) {
		ib_sa_pack_attr(mcmember_rec, &rec, IB_SA_ATTR_MC_MEMBER_REC);
		if (copy_to_user((void __user *) (unsigned long) req->response,
				 mcmember_rec, IB_SA_ATTR_MC_MEMBER_REC_LEN))
			ret = -EFAULT;
	}

	put_dev(dev);
	return ret;
}

static int process_mcast(struct usa_file *file, struct ib_usa_request *req,
			 int out_len)
{
	/* Only indirect requests are currently supported. */
	if (!req->local)
		return -ENOSYS;

	switch (req->method) {
	case IB_MGMT_METHOD_GET:
		return get_mcast(file, req, out_len);
	case IB_MGMT_METHOD_SET:
		return join_mcast(file, req, out_len);
	default:
		return -EINVAL;
	}
}

static int notice_handler(int status, struct ib_inform_info *info,
			  struct ib_sa_notice *notice)
{
	struct usa_inform_info *inform = info->context;
	struct usa_file *file = inform->id.file;
	struct usa_event *event;

	event = kzalloc(sizeof *event, GFP_KERNEL);
	if (!event)
		return 0;

	event->resp.uid = inform->id.uid;
	event->id = &inform->id;
	event->resp.status = status;

	if (notice) {
		event->resp.attr_id = cpu_to_be16(IB_SA_ATTR_NOTICE);
		event->resp.data_len = IB_SA_ATTR_NOTICE_LEN;
		ib_sa_pack_attr(event->resp.data, notice, IB_SA_ATTR_NOTICE);
	} else
		event->resp.attr_id = cpu_to_be16(IB_SA_ATTR_INFORM_INFO);

	queue_event(file, event);
	return 0;
}

static int reg_inform(struct usa_file *file, struct ib_usa_request *req,
		      int out_len)
{
	struct usa_inform_info *inform;
	struct ib_sa_inform sa_inform_info;
	u8 net_inform_info[IB_SA_ATTR_INFORM_INFO_LEN];
	u16 trap_number;
	int ret;

	if (out_len < sizeof(u32))
		return -ENOSPC;

	if (copy_from_user(&net_inform_info,
			   (void __user *) (unsigned long) req->attr,
			   IB_SA_ATTR_INFORM_INFO_LEN))
		return -EFAULT;

	inform = kzalloc(sizeof *inform, GFP_KERNEL);
	if (!inform)
		return -ENOMEM;

	inform->id.dev = get_dev(req->node_guid, req->port_num);
	if (!inform->id.dev) {
		ret = -ENODEV;
		goto err1;
	}

	inform->id.attr_id = IB_SA_ATTR_INFORM_INFO;
	inform->id.uid = req->uid;

	ret = insert_id(&inform->id);
	if (ret)
		goto err2;

	if (copy_to_user((void __user *) (unsigned long) req->response,
			 &inform->id.num, sizeof(u32))) {
		ret = EFAULT;
		goto err3;
	}

	inform->id.file = file;
	insert_file_id(file, &inform->id);

	ib_sa_unpack_attr(&sa_inform_info, &net_inform_info,
			  IB_SA_ATTR_INFORM_INFO);
	trap_number = be16_to_cpu(sa_inform_info.trap.generic.trap_num);
	inform->inform_info =
		ib_sa_register_inform_info(&file->sa_client,
					   inform->id.dev->device,
					   req->port_num, trap_number,
					   GFP_KERNEL, notice_handler,
					   inform);
	if (IS_ERR(inform->inform_info)) {
		ret = PTR_ERR(inform->inform_info);
		goto err4;
	}

	return 0;

err4:
	remove_file_id(file, &inform->id);
err3:
	remove_id(&inform->id);
err2:
	put_dev(inform->id.dev);
err1:
	kfree(inform);
	return ret;
}

static int process_inform(struct usa_file *file, struct ib_usa_request *req,
			  int out_len)
{
	/* Only indirect requests are currently supported. */
	if (!req->local)
		return -ENOSYS;

	if (req->method != IB_MGMT_METHOD_SET)
		return -EINVAL;

	return reg_inform(file, req, out_len);
}

static ssize_t usa_request(struct usa_file *file, const char __user *inbuf,
			   int in_len, int out_len)
{
	struct ib_usa_request req;

	if (copy_from_user(&req, inbuf, sizeof(req)))
		return -EFAULT;

	switch (be16_to_cpu(req.attr_id)) {
	case IB_SA_ATTR_MC_MEMBER_REC:
		return process_mcast(file, &req, out_len);
	case IB_SA_ATTR_INFORM_INFO:
		return process_inform(file, &req, out_len);
	default:
		return -EINVAL;
	}
}

static void cleanup_events(struct usa_id *id)
{
	struct usa_event *event, *next_event;

	list_for_each_entry_safe(event, next_event,
				 &id->file->event_list, list) {
		if (event->id == id) {
			list_del(&event->list);
			kfree(event);
		}
	}
}

static void *cleanup_mcast(struct usa_id *id)
{
	struct usa_multicast *mcast;

	mcast = container_of(id, struct usa_multicast, id);
	ib_sa_free_multicast(mcast->multicast);

	mutex_lock(&id->file->file_mutex);
	list_del(&id->list);
	list_del(&mcast->event.list);
	mutex_unlock(&id->file->file_mutex);

	return mcast;
}

static void *cleanup_inform(struct usa_id *id)
{
	struct usa_inform_info *inform;

	inform = container_of(id, struct usa_inform_info, id);
	ib_sa_unregister_inform_info(inform->inform_info);

	mutex_lock(&id->file->file_mutex);
	list_del(&id->list);
	cleanup_events(id);
	mutex_unlock(&id->file->file_mutex);

	return inform;
}

static int free_id(struct usa_id *id)
{
	void *free_obj;
	int events_reported;

	switch (id->attr_id) {
	case IB_SA_ATTR_MC_MEMBER_REC:
		free_obj = cleanup_mcast(id);
		break;
	case IB_SA_ATTR_INFORM_INFO:
		free_obj = cleanup_inform(id);
		break;
	default:
		free_obj = NULL;
		break;
	}

	events_reported = id->events_reported;
	put_dev(id->dev);
	kfree(free_obj);

	return events_reported;
}

static ssize_t usa_free(struct usa_file *file, const char __user *inbuf,
			int in_len, int out_len)
{
	struct ib_usa_free cmd;
	struct ib_usa_free_resp resp;
	struct usa_id *id;
	int ret = 0;

	if (out_len < sizeof(resp))
		return -ENOSPC;

	if (copy_from_user(&cmd, inbuf, sizeof(cmd)))
		return -EFAULT;

	mutex_lock(&usa_mutex);
	id = get_id(cmd.id, file, be16_to_cpu(cmd.attr_id));
	if (!IS_ERR(id))
		idr_remove(&usa_idr, id->num);
	mutex_unlock(&usa_mutex);

	resp.events_reported = free_id(id);

	if (copy_to_user((void __user *) (unsigned long) cmd.response,
			 &resp, sizeof resp))
		ret = -EFAULT;

	return ret;
}

struct usa_path_response {
	struct ib_sa_path_rec rec;
	int result;
	struct semaphore waiting;
};

void usa_path_rec_completion(int status, struct ib_sa_path_rec *resp,
				void *context)
{
	struct usa_path_response *response =
					(struct usa_path_response *)context;

	response->result = status;

	if (status == 0) {
		/* Note that we are copying by value, not by reference! */
		response->rec = *resp;
	}

	up(&response->waiting);
}

static ib_sa_comp_mask usa_build_comp_mask(struct ib_sa_path_rec path)
{
	ib_sa_comp_mask mask = 0;

	if (path.service_id)
		mask |= IB_SA_PATH_REC_SERVICE_ID;
	if (path.dgid.global.interface_id | path.dgid.global.subnet_prefix)
		mask |= IB_SA_PATH_REC_DGID;
	if (path.sgid.global.interface_id | path.sgid.global.subnet_prefix)
		mask |= IB_SA_PATH_REC_SGID;
	if (path.dlid)
		mask |= IB_SA_PATH_REC_DLID;
	if (path.slid)
		mask |= IB_SA_PATH_REC_SLID;
	if (path.raw_traffic)
		mask |= IB_SA_PATH_REC_RAW_TRAFFIC;
	if (path.flow_label)
		mask |= IB_SA_PATH_REC_FLOW_LABEL;
	if (path.hop_limit)
		mask |= IB_SA_PATH_REC_HOP_LIMIT;
	if (path.traffic_class)
		mask |= IB_SA_PATH_REC_TRAFFIC_CLASS;
	if (path.reversible)
		mask |= IB_SA_PATH_REC_REVERSIBLE;
	if (path.numb_path)
		mask |= IB_SA_PATH_REC_NUMB_PATH;
	if (path.pkey)
		mask |= IB_SA_PATH_REC_PKEY;
	if (path.qos_class)
		mask |= IB_SA_PATH_REC_QOS_CLASS;
	if (path.sl)
		mask |= IB_SA_PATH_REC_SL;
	if (path.mtu_selector)
		mask |= IB_SA_PATH_REC_MTU_SELECTOR;
	if (path.mtu)
		mask |= IB_SA_PATH_REC_MTU;
	if (path.rate_selector)
		mask |= IB_SA_PATH_REC_RATE_SELECTOR;
	if (path.rate)
		mask |= IB_SA_PATH_REC_RATE;
	if (path.packet_life_time_selector)
		mask |= IB_SA_PATH_REC_PACKET_LIFE_TIME_SELECTOR;
	if (path.packet_life_time)
		mask |= IB_SA_PATH_REC_PACKET_LIFE_TIME;
	if (path.preference)
		mask |= IB_SA_PATH_REC_PREFERENCE;

	return mask;
};

static ssize_t usa_get_path(struct usa_file *file, const char __user *inbuf,
			int in_len, int out_len)
{
	int ret = 0;
	ib_sa_comp_mask sa_comp_mask = 0;
	__u8 packed_data[IB_SA_ATTR_PATH_REC_LEN];
	struct ib_usa_path cmd;
	struct ib_sa_path_rec query;
	struct usa_path_response response;
	struct usa_device *dev;
	void *sa_query;

	if (copy_from_user(&cmd, inbuf, sizeof(cmd))) {
		pr_err("usa_get_path copy_from_user failed to copy command.\n");
		return -EFAULT;
	}

	if (copy_from_user(packed_data, (void __user *)cmd.query,
			   sizeof(cmd))) {
		pr_err("usa_get_path copy_from_user failed to copy command.\n");
		return -EFAULT;
	}

	dev = get_dev(cmd.hca_context, cmd.port_num);
	if (!dev) {
		pr_err("usa_get_path get_dev failed.\n");
		return -EFAULT;
	}

	ib_sa_unpack_attr(&query, cmd.query, IB_SA_ATTR_PATH_REC);

	/* query.reversible = 1; */
	query.numb_path = 1;

	sa_comp_mask = usa_build_comp_mask(query);

	sema_init(&response.waiting, 1);
	down(&response.waiting);
	ret = ib_sa_path_rec_get(&file->sa_client,
			dev->device,
			cmd.port_num,
			&query,
			sa_comp_mask,
			cmd.timeout,
			GFP_KERNEL,
			usa_path_rec_completion,
			&response,
			(struct ib_sa_query **)&sa_query);

	if (ret < 0) {
		pr_err("usa_get_path ib_sa_path_get_rec failed: %d\n", ret);
		ret = -EINVAL;
		goto out;
	}

	/* Wait for callback. */
	down(&response.waiting);

	if (response.result != 0) {
		pr_err("usa_get_path - query failed. result = %d\n",
			response.result);
		ret = -EFAULT;
		goto out;
	}

	ret = 0;

	ib_sa_pack_attr(packed_data, &response.rec, IB_SA_ATTR_PATH_REC);
	if (copy_to_user((void __user *)cmd.response, packed_data,
			IB_SA_ATTR_PATH_REC_LEN)) {
		pr_err(KERN_ERR"usa_get_path copy_to_user failed.\n");
		ret = -EFAULT;
	}

out:
	if (dev)
		put_dev(dev);

	return ret;
}

static ssize_t usa_get_hca(struct usa_file *file, const char __user *inbuf,
			int in_len, int out_len)
{
	int ret = 0;
	struct ib_usa_hca cmd;
	struct usa_device *dev;

	if (copy_from_user(&cmd, inbuf, sizeof(cmd))) {
		pr_err("usa_get_hca copy_from_user failed to copy command.\n");
		return -EFAULT;
	}

	dev = get_dev_by_name(cmd.hca_name, cmd.port_num);
	if (!dev) {
		pr_err("usa_get_hca get_dev_by_name(%s,%d) failed.\n",
			cmd.hca_name, cmd.port_num);
		return -ENXIO;
	}

	cmd.hca_context = dev->device->node_guid;

	put_dev(dev);

	if (copy_to_user((void __user *)inbuf, &cmd, sizeof(cmd))) {
		pr_err("usa_get_hca copy_to_user failed.\n");
		ret = -EFAULT;
	}

	return 0;
}

static ssize_t (*usa_cmd_table[])(struct usa_file *file,
				   const char __user *inbuf,
				   int in_len, int out_len) = {
	[IB_USA_CMD_REQUEST]	= usa_request,
	[IB_USA_CMD_GET_EVENT]	= usa_get_event,
	[IB_USA_CMD_FREE]	= usa_free,
	[IB_USA_CMD_GET_PATH]	= usa_get_path,
	[IB_USA_CMD_GET_HCA]	= usa_get_hca
};

static ssize_t usa_write(struct file *filp, const char __user *buf,
			 size_t len, loff_t *pos)
{
	struct usa_file *file = filp->private_data;
	struct ib_usa_cmd_hdr hdr;
	ssize_t ret;

	if (len < sizeof(hdr))
		return -EINVAL;

	if (copy_from_user(&hdr, buf, sizeof(hdr)))
		return -EFAULT;

	if (hdr.cmd < 0 || hdr.cmd >= ARRAY_SIZE(usa_cmd_table))
		return -EINVAL;

	if (hdr.in + sizeof(hdr) > len)
		return -EINVAL;

	ret = usa_cmd_table[hdr.cmd](file, buf + sizeof(hdr), hdr.in, hdr.out);
	if (!ret)
		ret = len;

	return ret;
}

static unsigned int usa_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct usa_file *file = filp->private_data;
	unsigned int mask = 0;

	poll_wait(filp, &file->poll_wait, wait);

	if (!list_empty(&file->event_list))
		mask = POLLIN | POLLRDNORM;

	return mask;
}

static int usa_open(struct inode *inode, struct file *filp)
{
	struct usa_file *file;

	file = kmalloc(sizeof *file, GFP_KERNEL);
	if (!file)
		return -ENOMEM;

	ib_sa_register_client(&file->sa_client);

	INIT_LIST_HEAD(&file->event_list);
	INIT_LIST_HEAD(&file->id_list);
	init_waitqueue_head(&file->poll_wait);
	mutex_init(&file->file_mutex);

	filp->private_data = file;
	file->filp = filp;
	return 0;
}

static int usa_close(struct inode *inode, struct file *filp)
{
	struct usa_file *file = filp->private_data;
	struct usa_id *id;

	while (!list_empty(&file->id_list)) {
		id = list_entry(file->id_list.next, struct usa_id, list);
		remove_id(id);
		free_id(id);
	}
	ib_sa_unregister_client(&file->sa_client);

	kfree(file);
	return 0;
}

static void usa_add_one(struct ib_device *device)
{
	struct usa_device *dev;

	if (rdma_node_get_transport(device->node_type) != RDMA_TRANSPORT_IB)
		return;

	dev = kmalloc(sizeof *dev, GFP_KERNEL);
	if (!dev)
		return;

	dev->device = device;
	if (device->node_type == RDMA_NODE_IB_SWITCH)
		dev->start_port = dev->end_port = 0;
	else {
		dev->start_port = 1;
		dev->end_port = device->phys_port_cnt;
	}

	init_completion(&dev->comp);
	atomic_set(&dev->refcount, 1);
	ib_set_client_data(device, &usa_client, dev);

	mutex_lock(&usa_mutex);
	list_add_tail(&dev->list, &dev_list);
	mutex_unlock(&usa_mutex);
}

static void usa_remove_one(struct ib_device *device)
{
	struct usa_device *dev;

	dev = ib_get_client_data(device, &usa_client);
	if (!dev)
		return;

	mutex_lock(&usa_mutex);
	list_del(&dev->list);
	mutex_unlock(&usa_mutex);

	/* TODO: force immediate device removal */
	put_dev(dev);
	wait_for_completion(&dev->comp);
	kfree(dev);
}

static const struct file_operations usa_fops = {
	.owner		= THIS_MODULE,
	.open		= usa_open,
	.release	= usa_close,
	.write		= usa_write,
	.poll		= usa_poll,
};

static struct miscdevice usa_misc = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "ib_usa",
	.nodename	= "infiniband/ib_usa",
	.mode		= 0666,
	.fops		= &usa_fops,
};

static ssize_t show_abi_version(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%d\n", IB_USA_ABI_VERSION);
}
static DEVICE_ATTR(abi_version, S_IRUGO, show_abi_version, NULL);

static int __init usa_init(void)
{
	int ret;

	ret = misc_register(&usa_misc);
	if (ret)
		return ret;

	ret = device_create_file(usa_misc.this_device, &dev_attr_abi_version);
	if (ret) {
		pr_err("ib_usa: couldn't create abi_version attr\n");
		goto err1;
	}

	ret = ib_register_client(&usa_client);
	if (ret)
		goto err2;

	return 0;

err2:
	device_remove_file(usa_misc.this_device, &dev_attr_abi_version);
err1:
	misc_deregister(&usa_misc);
	return ret;
}

static void __exit usa_cleanup(void)
{
	ib_unregister_client(&usa_client);
	device_remove_file(usa_misc.this_device, &dev_attr_abi_version);
	misc_deregister(&usa_misc);
	idr_destroy(&usa_idr);
}

module_init(usa_init);
module_exit(usa_cleanup);
