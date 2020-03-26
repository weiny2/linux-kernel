// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2018 Intel Corporation */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/bug.h>

#include "hqmv2_main.h"
#include "hqmv2_dp.h"
#include "hqmv2_dp_ioctl.h"
#include "hqmv2_dp_priv.h"
#include "hqmv2_dp_ops.h"
#include "base/hqmv2_osdep.h"

static inline void hqmv2_log_ioctl_error(struct device *dev,
					 int ret,
					 int status)
{
	if (ret && status) {
		HQMV2_ERR(dev, "[%s()] Error: %s\n", __func__,
			  hqmv2_error_strings[status]);
	} else if (ret) {
		HQMV2_ERR(dev, "%s: ioctl failed before handler, ret = %d\n",
			  __func__, ret);
	}
}

/*****************/
/* HQM Functions */
/*****************/

/* Pointers to HQM devices. These are set in hqmv2_datapath_init() and cleared
 * in hqmv2_datapath_free().
 */
static struct hqmv2_dev *hqmv2_devices[HQMV2_MAX_NUM_DEVICES];

void hqmv2_datapath_init(struct hqmv2_dev *dev, int id)
{
	hqmv2_devices[id] = dev;

	INIT_LIST_HEAD(&dev->dp.hdl_list);
}

static void hqmv2_domain_free(struct hqmv2_dp_domain *domain)
{
	int i;

	for (i = 0; i < HQMV2_MAX_NUM_LDB_PORTS; i++) {
		struct hqmv2_port_hdl *port_hdl, *next;
		struct list_head *head;
		struct hqmv2_port *port;

		port = &domain->ldb_ports[i];

		if (!port->configured)
			continue;

		head = &port->hdl_list_head;

		list_for_each_entry_safe(port_hdl, next, head, list)
			hqmv2_detach_port(port_hdl);
	}

	for (i = 0; i < HQMV2_MAX_NUM_DIR_PORTS; i++) {
		struct hqmv2_port_hdl *port_hdl, *next;
		struct list_head *head;
		struct hqmv2_port *port;

		port = &domain->dir_ports[i];

		if (!port->configured)
			continue;

		head = &port->hdl_list_head;

		list_for_each_entry_safe(port_hdl, next, head, list)
			hqmv2_detach_port(port_hdl);
	}
}

/* hqmv2_datapath_free - Clean up all datapath-related state
 *
 * This function is called as part of the driver's remove callback, thus no
 * other kernel modules are actively using the datapath. This function follows
 * the standard clean-up procedure (detach handles, reset domains, close HQM
 * handle) for any resources that other kernel software neglected to clean up.
 */
void hqmv2_datapath_free(int id)
{
	struct hqmv2_dp *dp, *next;
	struct hqmv2_dev *dev;

	dev = hqmv2_devices[id];

	if (!dev)
		return;

	list_for_each_entry_safe(dp, next, &dev->dp.hdl_list, next) {
		int i;

		for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++) {
			struct hqmv2_domain_hdl *domain_hdl, *next;
			struct hqmv2_dp_domain *domain;
			struct list_head *head;

			domain = &dp->domains[i];

			if (!domain->configured)
				continue;

			hqmv2_domain_free(domain);

			head = &domain->hdl_list_head;

			list_for_each_entry_safe(domain_hdl, next, head, list)
				hqmv2_detach_sched_domain(domain_hdl);

			hqmv2_reset_sched_domain(dp, i);
		}

		hqmv2_close(dp);
	}

	hqmv2_devices[id] = NULL;
}

int hqmv2_open(int device_id,
	       struct hqmv2_dp **hdl)
{
	int ret = -1;
	struct hqmv2_dev *dev;
	struct hqmv2_dp *hqmv2 = NULL;

	BUILD_BUG_ON(sizeof(struct hqmv2_enqueue_qe) != 16);
	BUILD_BUG_ON(sizeof(struct hqmv2_dequeue_qe) != 16);
	BUILD_BUG_ON(sizeof(struct hqmv2_enqueue_qe) !=
		     sizeof(struct hqmv2_send));
	BUILD_BUG_ON(sizeof(struct hqmv2_enqueue_qe) !=
		     sizeof(struct hqmv2_adv_send));
	BUILD_BUG_ON(sizeof(struct hqmv2_dequeue_qe) !=
		     sizeof(struct hqmv2_recv));

	if (!(device_id >= 0 && device_id < HQMV2_MAX_NUM_DEVICES)) {
		ret = -EINVAL;
		goto cleanup;
	}

	dev = hqmv2_devices[device_id];

	if (!dev) {
		ret = -EINVAL;
		goto cleanup;
	}

	hqmv2 = devm_kcalloc(&dev->pdev->dev,
			     1,
			     sizeof(struct hqmv2_dp),
			     GFP_KERNEL);
	if (!hqmv2) {
		ret = -ENOMEM;
		goto cleanup;
	}

	hqmv2->dev = dev;
	hqmv2->magic_num = HQMV2_MAGIC_NUM;
	hqmv2->id = device_id;

	mutex_init(&hqmv2->resource_mutex);

	hqmv2_register_dp_handle(hqmv2);

	*hdl = hqmv2;

	ret = 0;

cleanup:

	if (ret && hqmv2)
		devm_kfree(&dev->pdev->dev, hqmv2);

	return ret;
}
EXPORT_SYMBOL(hqmv2_open);

int hqmv2_close(struct hqmv2_dp *hqmv2)
{
	int i, ret = -1;

/* DISABLE_CHECK wraps checks that are helpful to catch errors during
 * development, but not strictly required. Typically used for datapath
 * functions to improve performance.
 */
#ifndef DISABLE_CHECK
	if (hqmv2->magic_num != HQMV2_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	/* Check if there are any remaining attached domain handles */
	for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++)
		if (hqmv2->domains[i].configured &&
		    !list_empty(&hqmv2->domains[i].hdl_list_head)) {
			ret = -EEXIST;
			goto cleanup;
		}

	for (i = 0; i < HQMV2_MAX_NUM_DOMAINS; i++)
		if (hqmv2->domains[i].configured)
			hqmv2_reset_sched_domain(hqmv2, i);

	hqmv2_unregister_dp_handle(hqmv2);

	devm_kfree(&hqmv2->dev->pdev->dev, hqmv2);

	ret = 0;

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_close);

static int hqmv2_dp_ioctl_get_num_resources(struct hqmv2_dp *hqmv2,
					    struct hqmv2_resources *rsrcs)
{
	struct hqmv2_get_num_resources_args ioctl_args = {0};
	int ret;

	ret = hqmv2_ioctl_get_num_resources(hqmv2->dev,
					    (unsigned long)&ioctl_args);

	rsrcs->num_sched_domains = ioctl_args.num_sched_domains;
	rsrcs->num_ldb_queues = ioctl_args.num_ldb_queues;
	rsrcs->num_ldb_ports = ioctl_args.num_ldb_ports;
	rsrcs->num_dir_ports = ioctl_args.num_dir_ports;
	rsrcs->num_ldb_event_state_entries =
		ioctl_args.num_hist_list_entries;
	rsrcs->max_contiguous_ldb_event_state_entries =
		ioctl_args.max_contiguous_hist_list_entries;
	rsrcs->num_ldb_credits =
		ioctl_args.num_ldb_credits;
	rsrcs->num_dir_credits =
		ioctl_args.num_dir_credits;
	rsrcs->num_ldb_credit_pools = NUM_LDB_CREDIT_POOLS;
	rsrcs->num_dir_credit_pools = NUM_DIR_CREDIT_POOLS;

	return ret;
}

int hqmv2_get_num_resources(struct hqmv2_dp *hqmv2,
			    struct hqmv2_resources *rsrcs)
{
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hqmv2->magic_num != HQMV2_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	ret = hqmv2_dp_ioctl_get_num_resources(hqmv2, rsrcs);

cleanup:
	return ret;
}
EXPORT_SYMBOL(hqmv2_get_num_resources);

/*********************************************/
/* Scheduling domain configuration Functions */
/*********************************************/

static int hqmv2_dp_ioctl_create_sch_dom(struct hqmv2_dp *hqmv2,
					 struct hqmv2_create_sched_domain *args)
{
	struct hqmv2_create_sched_domain_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;
	ioctl_args.num_ldb_queues = args->num_ldb_queues;
	ioctl_args.num_ldb_ports = args->num_ldb_ports;
	ioctl_args.num_dir_ports = args->num_dir_ports;
	ioctl_args.num_atomic_inflights =
		args->num_ldb_queues * NUM_ATM_INFLIGHTS_PER_LDB_QUEUE;
	ioctl_args.num_hist_list_entries = args->num_ldb_event_state_entries;
	ioctl_args.num_ldb_credits = args->num_ldb_credits;
	ioctl_args.num_dir_credits = args->num_dir_credits;

	ret = __hqmv2_ioctl_create_sched_domain(hqmv2->dev,
						(unsigned long)&ioctl_args,
						false,
						hqmv2);

	hqmv2_log_ioctl_error(hqmv2->dev->hqmv2_device, ret, response.status);

	return (ret == 0) ? response.id : ret;
}

int hqmv2_create_sched_domain(struct hqmv2_dp *hqmv2,
			      struct hqmv2_create_sched_domain *args)
{
	struct hqmv2_dp_domain *domain;
	int id, ret, i;

	ret = -1;

#ifndef DISABLE_CHECK
	if (hqmv2->magic_num != HQMV2_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	mutex_lock(&hqmv2->resource_mutex);

	if (!(args->num_ldb_credit_pools <= NUM_LDB_CREDIT_POOLS)) {
		ret = -EINVAL;
		goto cleanup;
	}

	if (!(args->num_dir_credit_pools <= NUM_DIR_CREDIT_POOLS)) {
		ret = -EINVAL;
		goto cleanup;
	}

	id = hqmv2_dp_ioctl_create_sch_dom(hqmv2, args);
	if (id < 0) {
		ret = id;
		mutex_unlock(&hqmv2->resource_mutex);
		goto cleanup;
	}

	domain = &hqmv2->domains[id];

	memset(domain, 0, sizeof(struct hqmv2_dp_domain));

	domain->id = id;
	domain->dev = hqmv2->dev;
	domain->hqmv2_dp = hqmv2;

	INIT_LIST_HEAD(&domain->hdl_list_head);

	for (i = 0; i < HQMV2_MAX_NUM_LDB_PORTS; i++)
		INIT_LIST_HEAD(&domain->ldb_ports[i].hdl_list_head);
	for (i = 0; i < HQMV2_MAX_NUM_DIR_PORTS; i++)
		INIT_LIST_HEAD(&domain->dir_ports[i].hdl_list_head);

	mutex_init(&domain->resource_mutex);

	domain->sw_credits.avail_credits[LDB] = args->num_ldb_credits;
	domain->sw_credits.avail_credits[DIR] = args->num_dir_credits;

	domain->reads_allowed = true;
	domain->num_readers = 0;
	domain->configured = true;

	ret = id;

	mutex_unlock(&hqmv2->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_create_sched_domain);

struct hqmv2_domain_hdl *
hqmv2_attach_sched_domain(struct hqmv2_dp *hqmv2,
			  int domain_id)
{
	struct hqmv2_domain_hdl *domain_hdl = NULL;
	struct hqmv2_dp_domain *domain;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hqmv2->magic_num != HQMV2_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	if (!(domain_id >= 0 && domain_id < HQMV2_MAX_NUM_DOMAINS)) {
		ret = -EINVAL;
		goto cleanup;
	}

	if (!hqmv2->domains[domain_id].configured) {
		ret = -EINVAL;
		goto cleanup;
	}

	domain = &hqmv2->domains[domain_id];

	mutex_lock(&domain->resource_mutex);

	domain_hdl = devm_kcalloc(DEV_FROM_HQMV2_DP_DOMAIN(domain),
				  1,
				  sizeof(struct hqmv2_domain_hdl),
				  GFP_KERNEL);
	if (!domain_hdl) {
		ret = -ENOMEM;
		mutex_unlock(&domain->resource_mutex);
		goto cleanup;
	}

	domain_hdl->magic_num = DOMAIN_MAGIC_NUM;
	domain_hdl->domain = domain;

	/* Add the new handle to the domain's linked list of handles */
	list_add(&domain_hdl->list, &domain->hdl_list_head);

	ret = 0;

	mutex_unlock(&domain->resource_mutex);

cleanup:

	return (ret == 0) ? domain_hdl : NULL;
}
EXPORT_SYMBOL(hqmv2_attach_sched_domain);

int hqmv2_detach_sched_domain(struct hqmv2_domain_hdl *hdl)
{
	struct hqmv2_dp_domain *domain;
	int i, ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;

	mutex_lock(&domain->resource_mutex);

	/* All port handles must be detached before the domain handle */
	for (i = 0; i < HQMV2_MAX_NUM_LDB_PORTS; i++)
		if (!list_empty(&domain->ldb_ports[i].hdl_list_head)) {
			ret = -EINVAL;
			mutex_unlock(&domain->resource_mutex);
			goto cleanup;
		}
	for (i = 0; i < HQMV2_MAX_NUM_DIR_PORTS; i++)
		if (!list_empty(&domain->dir_ports[i].hdl_list_head)) {
			ret = -EINVAL;
			mutex_unlock(&domain->resource_mutex);
			goto cleanup;
		}

	/* Remove the handle from the domain's handles list */
	list_del(&hdl->list);

	memset(hdl, 0, sizeof(struct hqmv2_domain_hdl));
	devm_kfree(DEV_FROM_HQMV2_DP_DOMAIN(domain), hdl);

	ret = 0;

	mutex_unlock(&domain->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_detach_sched_domain);

int hqmv2_create_ldb_credit_pool(struct hqmv2_domain_hdl *hdl,
				 int num_credits)
{
	struct hqmv2_dp_domain *domain;
	int i, ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;

	if (domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}

	if (!(num_credits <= domain->sw_credits.avail_credits[LDB])) {
		ret = -EINVAL;
		goto cleanup;
	}

	mutex_lock(&domain->resource_mutex);

	for (i = 0; i < NUM_LDB_CREDIT_POOLS; i++) {
		if (!domain->sw_credits.ldb_pools[i].configured)
			break;
	}

	if (!(i < NUM_LDB_CREDIT_POOLS)) {
		ret = -EINVAL;
		goto cleanup;
	}

	atomic_set(&domain->sw_credits.ldb_pools[i].avail_credits, num_credits);
	domain->sw_credits.ldb_pools[i].configured = true;

	domain->sw_credits.avail_credits[LDB] -= num_credits;

	ret = i;

	mutex_unlock(&domain->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_create_ldb_credit_pool);

int hqmv2_create_dir_credit_pool(struct hqmv2_domain_hdl *hdl,
				 int num_credits)
{
	struct hqmv2_dp_domain *domain;
	int i, ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;

	if (domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}

	if (!(num_credits <= domain->sw_credits.avail_credits[DIR])) {
		ret = -EINVAL;
		goto cleanup;
	}

	mutex_lock(&domain->resource_mutex);

	for (i = 0; i < NUM_DIR_CREDIT_POOLS; i++) {
		if (!domain->sw_credits.dir_pools[i].configured)
			break;
	}

	if (!(i < NUM_DIR_CREDIT_POOLS)) {
		ret = -EINVAL;
		goto cleanup;
	}

	atomic_set(&domain->sw_credits.dir_pools[i].avail_credits, num_credits);
	domain->sw_credits.dir_pools[i].configured = true;

	domain->sw_credits.avail_credits[DIR] -= num_credits;

	ret = i;

	mutex_unlock(&domain->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_create_dir_credit_pool);

static int hqmv2_dp_ioctl_create_ldb_queue(struct hqmv2_dp_domain *domain,
					   struct hqmv2_create_ldb_queue *args)
{
	struct hqmv2_create_ldb_queue_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;
	ioctl_args.num_sequence_numbers = args->num_sequence_numbers;
	ioctl_args.num_atomic_inflights = NUM_ATM_INFLIGHTS_PER_LDB_QUEUE;
	ioctl_args.lock_id_comp_level = args->lock_id_comp_level;
	if (args->num_sequence_numbers > 0)
		ioctl_args.num_qid_inflights = args->num_sequence_numbers;
	else
		/* Give each queue half of the QID inflights. Intent is to
		 * support high fan-out queues without allowing one or two
		 * queues to use all the inflights.
		 */
		ioctl_args.num_qid_inflights = NUM_QID_INFLIGHTS / 4;

	ret = hqmv2_domain_ioctl_create_ldb_queue(domain->dev,
						  (unsigned long)&ioctl_args,
						  domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return (ret == 0) ? response.id : ret;
}

int hqmv2_create_ldb_queue(struct hqmv2_domain_hdl *hdl,
			   struct hqmv2_create_ldb_queue *args)
{
	struct hqmv2_dp_domain *domain;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;

	if (domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}

	mutex_lock(&domain->resource_mutex);

	ret = hqmv2_dp_ioctl_create_ldb_queue(domain, args);

	if (ret >= 0)
		domain->queue_valid[LDB][ret] = true;

	mutex_unlock(&domain->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_create_ldb_queue);

static int hqmv2_dp_ioctl_create_dir_queue(struct hqmv2_dp_domain *domain,
					   int port_id)
{
	struct hqmv2_create_dir_queue_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;
	ioctl_args.port_id = port_id;

	ret = hqmv2_domain_ioctl_create_dir_queue(domain->dev,
						  (unsigned long)&ioctl_args,
						  domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return (ret == 0) ? response.id : ret;
}

int hqmv2_create_dir_queue(struct hqmv2_domain_hdl *hdl,
			   int port_id)
{
	struct hqmv2_dp_domain *domain;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;

	if (domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}

	mutex_lock(&domain->resource_mutex);

	ret = hqmv2_dp_ioctl_create_dir_queue(domain, port_id);

	if (ret >= 0)
		domain->queue_valid[DIR][ret] = true;

	mutex_unlock(&domain->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_create_dir_queue);

static int hqmv2_create_ldb_port_adv(struct hqmv2_domain_hdl *hdl,
				     struct hqmv2_create_port *args,
				     struct hqmv2_create_port_adv *adv_args);

int hqmv2_create_ldb_port(struct hqmv2_domain_hdl *hdl,
			  struct hqmv2_create_port *args)
{
	struct hqmv2_create_port_adv adv_args = {0};
	struct hqmv2_create_port __args;
	struct hqmv2_sw_credit_pool *pool;
	struct hqmv2_dp_domain *domain;
	int ret = -1;

	/* Create a local copy to allow for modifications */
	__args = *args;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;

	if (!(args->ldb_credit_pool_id <= NUM_LDB_CREDIT_POOLS)) {
		ret = -EINVAL;
		goto cleanup;
	}

	pool = &domain->sw_credits.ldb_pools[args->ldb_credit_pool_id];

	if (!pool->configured) {
		ret = -EINVAL;
		goto cleanup;
	}

	if (!(args->dir_credit_pool_id <= NUM_DIR_CREDIT_POOLS)) {
		ret = -EINVAL;
		goto cleanup;
	}

	pool = &domain->sw_credits.dir_pools[args->dir_credit_pool_id];

	if (!pool->configured) {
		ret = -EINVAL;
		goto cleanup;
	}

	adv_args.cq_history_list_size = __args.num_ldb_event_state_entries;

	/* Set the low watermark to 1/2 of the credit allocation, and the
	 * quantum to 1/4.
	 */
	adv_args.ldb_credit_low_watermark = __args.num_ldb_credits >> 1;
	adv_args.dir_credit_low_watermark = __args.num_dir_credits >> 1;
	adv_args.ldb_credit_quantum = __args.num_ldb_credits >> 2;
	adv_args.dir_credit_quantum = __args.num_dir_credits >> 2;

	/* Create the load-balanced port */
	ret = hqmv2_create_ldb_port_adv(hdl, &__args, &adv_args);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_create_ldb_port);

static int hqmv2_create_dir_port_adv(struct hqmv2_domain_hdl *hdl,
				     struct hqmv2_create_port *args,
				     struct hqmv2_create_port_adv *adv_args,
				     int queue_id);

int hqmv2_create_dir_port(struct hqmv2_domain_hdl *hdl,
			  struct hqmv2_create_port *args,
			  int queue_id)
{
	struct hqmv2_create_port_adv adv_args = {0};
	struct hqmv2_create_port __args;
	struct hqmv2_sw_credit_pool *pool;
	struct hqmv2_dp_domain *domain;
	int ret = -1;

	/* Create a local copy to allow for modifications */
	__args = *args;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;

	if (!(args->ldb_credit_pool_id <= NUM_LDB_CREDIT_POOLS)) {
		ret = -EINVAL;
		goto cleanup;
	}

	pool = &domain->sw_credits.ldb_pools[args->ldb_credit_pool_id];

	if (!pool->configured) {
		ret = -EINVAL;
		goto cleanup;
	}

	if (!(args->dir_credit_pool_id <= NUM_DIR_CREDIT_POOLS)) {
		ret = -EINVAL;
		goto cleanup;
	}

	pool = &domain->sw_credits.dir_pools[args->dir_credit_pool_id];

	if (!pool->configured) {
		ret = -EINVAL;
		goto cleanup;
	}

	/* Set the low watermark to 1/2 of the credit allocation, and the
	 * quantum to 1/4.
	 */
	adv_args.ldb_credit_low_watermark = __args.num_ldb_credits >> 1;
	adv_args.dir_credit_low_watermark = __args.num_dir_credits >> 1;
	adv_args.ldb_credit_quantum = __args.num_ldb_credits >> 2;
	adv_args.dir_credit_quantum = __args.num_dir_credits >> 2;

	/* Create the directed port */
	ret = hqmv2_create_dir_port_adv(hdl, &__args, &adv_args, queue_id);

cleanup:
	return ret;
}
EXPORT_SYMBOL(hqmv2_create_dir_port);

struct hqmv2_port_hdl *
hqmv2_attach_ldb_port(struct hqmv2_domain_hdl *hdl,
		      int port_id)
{
	struct hqmv2_port_hdl *port_hdl = NULL;
	struct hqmv2_dp_domain *domain;
	struct device *device;
	struct hqmv2_port *port;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;
	device = DEV_FROM_HQMV2_DP_DOMAIN(domain);

	if (!(port_id >= 0 && port_id < HQMV2_MAX_NUM_LDB_PORTS)) {
		ret = -EINVAL;
		goto cleanup;
	}

	if (!domain->ldb_ports[port_id].configured) {
		ret = -EINVAL;
		goto cleanup;
	}

	port = &domain->ldb_ports[port_id];

	mutex_lock(&port->resource_mutex);

	port_hdl = devm_kcalloc(device,
				1,
				sizeof(struct hqmv2_port_hdl),
				GFP_KERNEL);
	if (!port_hdl) {
		ret = -ENOMEM;
		mutex_unlock(&port->resource_mutex);
		goto cleanup;
	}

	/* Allocate cache-line-aligned memory for sending QEs */
	port_hdl->qe = (void *)devm_get_free_pages(device,
						   GFP_KERNEL,
						   0);
	if (!port_hdl->qe) {
		ret = -ENOMEM;
		mutex_unlock(&port->resource_mutex);
		goto cleanup;
	}

	port_hdl->magic_num = PORT_MAGIC_NUM;
	port_hdl->port = port;

	/* Add the newly created handle to the port's linked list of handles */
	list_add(&port_hdl->list, &port->hdl_list_head);

	ret = 0;

	mutex_unlock(&port->resource_mutex);

cleanup:

	if (ret) {
		if (port_hdl && port_hdl->qe)
			devm_free_pages(DEV_FROM_HQMV2_DP_DOMAIN(domain),
					(unsigned long)port_hdl->qe);
		if (port_hdl)
			devm_kfree(DEV_FROM_HQMV2_DP_DOMAIN(domain), port_hdl);
		port_hdl = NULL;
	}

	return port_hdl;
}
EXPORT_SYMBOL(hqmv2_attach_ldb_port);

struct hqmv2_port_hdl *
hqmv2_attach_dir_port(struct hqmv2_domain_hdl *hdl,
		      int port_id)
{
	struct hqmv2_port_hdl *port_hdl = NULL;
	struct hqmv2_dp_domain *domain;
	struct device *device;
	struct hqmv2_port *port;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;
	device = DEV_FROM_HQMV2_DP_DOMAIN(domain);

	if (!(port_id >= 0 && port_id < HQMV2_MAX_NUM_DIR_PORTS)) {
		ret = -EINVAL;
		goto cleanup;
	}

	if (!domain->dir_ports[port_id].configured) {
		ret = -EINVAL;
		goto cleanup;
	}

	port = &domain->dir_ports[port_id];

	mutex_lock(&port->resource_mutex);

	port_hdl = devm_kcalloc(device,
				1,
				sizeof(struct hqmv2_port_hdl),
				GFP_KERNEL);
	if (!port_hdl) {
		ret = -ENOMEM;
		mutex_unlock(&port->resource_mutex);
		goto cleanup;
	}

	/* Allocate cache-line-aligned memory for sending QEs */
	port_hdl->qe = (void *)devm_get_free_pages(device,
						   GFP_KERNEL,
						   0);
	if (!port_hdl->qe) {
		ret = -ENOMEM;
		mutex_unlock(&port->resource_mutex);
		goto cleanup;
	}

	port_hdl->magic_num = PORT_MAGIC_NUM;
	port_hdl->port = port;

	/* Add the new handle to the port's linked list of handles */
	list_add(&port_hdl->list, &port->hdl_list_head);

	ret = 0;

	mutex_unlock(&port->resource_mutex);

cleanup:

	if (ret) {
		if (port_hdl && port_hdl->qe)
			devm_free_pages(DEV_FROM_HQMV2_DP_DOMAIN(domain),
					(unsigned long)port_hdl->qe);
		if (port_hdl)
			devm_kfree(DEV_FROM_HQMV2_DP_DOMAIN(domain), port_hdl);
		port_hdl = NULL;
	}

	return port_hdl;
}
EXPORT_SYMBOL(hqmv2_attach_dir_port);

int hqmv2_detach_port(struct hqmv2_port_hdl *hdl)
{
	struct hqmv2_port *port;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != PORT_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	port = hdl->port;

	mutex_lock(&port->resource_mutex);

	/* Remove the handle from the port's handles list */
	list_del(&hdl->list);

	memset(hdl, 0, sizeof(struct hqmv2_port_hdl));
	devm_kfree(DEV_FROM_HQMV2_DP_DOMAIN(port->domain), hdl);

	ret = 0;

	mutex_unlock(&port->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_detach_port);

static int hqmv2_dp_ioctl_link_qid(struct hqmv2_dp_domain *domain,
				   int port_id,
				   int queue_id,
				   int priority)
{
	struct hqmv2_map_qid_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;
	ioctl_args.port_id = port_id;
	ioctl_args.qid = queue_id;
	ioctl_args.priority = priority;

	ret = hqmv2_domain_ioctl_map_qid(domain->dev,
					 (unsigned long)&ioctl_args,
					 domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return (ret == 0) ? response.id : ret;
}

int hqmv2_link_queue(struct hqmv2_port_hdl *hdl,
		     int queue_id,
		     int priority)
{
	struct hqmv2_port *port;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (!(priority >= 0 && priority <= 7)) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

#ifndef DISABLE_CHECK
	if (hdl->magic_num != PORT_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	port = hdl->port;

	if (port->domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}

	mutex_lock(&port->resource_mutex);

	ret = hqmv2_dp_ioctl_link_qid(port->domain,
				      port->id,
				      queue_id,
				      priority);

	mutex_unlock(&port->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_link_queue);

static int hqmv2_dp_ioctl_unlink_qid(struct hqmv2_dp_domain *domain,
				     int port_id,
				     int queue_id)
{
	struct hqmv2_unmap_qid_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;
	ioctl_args.port_id = port_id;
	ioctl_args.qid = queue_id;

	ret = hqmv2_domain_ioctl_unmap_qid(domain->dev,
					   (unsigned long)&ioctl_args,
					   domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return (ret == 0) ? 0 : ret;
}

int hqmv2_unlink_queue(struct hqmv2_port_hdl *hdl,
		       int queue_id)
{
	struct hqmv2_port *port;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != PORT_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	port = hdl->port;

	if (port->domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}

	mutex_lock(&port->resource_mutex);

	ret = hqmv2_dp_ioctl_unlink_qid(port->domain,
					port->id,
					queue_id);

	mutex_unlock(&port->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_unlink_queue);

static int hqmv2_dp_ioctl_enable_ldb_port(struct hqmv2_dp_domain *domain,
					  int port_id)
{
	struct hqmv2_enable_ldb_port_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;
	ioctl_args.port_id = port_id;

	ret = hqmv2_domain_ioctl_enable_ldb_port(domain->dev,
						 (unsigned long)&ioctl_args,
						 domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return ret;
}

static int hqmv2_dp_ioctl_enable_dir_port(struct hqmv2_dp_domain *domain,
					  int port_id)
{
	struct hqmv2_enable_dir_port_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;
	ioctl_args.port_id = port_id;

	ret = hqmv2_domain_ioctl_enable_dir_port(domain->dev,
						 (unsigned long)&ioctl_args,
						 domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return ret;
}

int hqmv2_enable_port(struct hqmv2_port_hdl *hdl)
{
	struct hqmv2_port *port;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != PORT_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	port = hdl->port;

	if (port->domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}

	mutex_lock(&port->resource_mutex);

	if (port->type == LDB)
		ret = hqmv2_dp_ioctl_enable_ldb_port(port->domain, port->id);
	else
		ret = hqmv2_dp_ioctl_enable_dir_port(port->domain, port->id);

	port->enabled = true;

	mutex_unlock(&port->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_enable_port);

static int hqmv2_dp_ioctl_disable_ldb_port(struct hqmv2_dp_domain *domain,
					   int port_id)
{
	struct hqmv2_disable_ldb_port_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;
	ioctl_args.port_id = port_id;

	ret = hqmv2_domain_ioctl_disable_ldb_port(domain->dev,
						  (unsigned long)&ioctl_args,
						  domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return ret;
}

static int hqmv2_dp_ioctl_disable_dir_port(struct hqmv2_dp_domain *domain,
					   int port_id)
{
	struct hqmv2_disable_dir_port_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;
	ioctl_args.port_id = port_id;

	ret = hqmv2_domain_ioctl_disable_dir_port(domain->dev,
						  (unsigned long)&ioctl_args,
						  domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return ret;
}

int hqmv2_disable_port(struct hqmv2_port_hdl *hdl)
{
	struct hqmv2_port *port;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != PORT_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	port = hdl->port;

	if (port->domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}

	mutex_lock(&port->resource_mutex);

	if (port->type == LDB)
		ret = hqmv2_dp_ioctl_disable_ldb_port(port->domain, port->id);
	else
		ret = hqmv2_dp_ioctl_disable_dir_port(port->domain, port->id);

	port->enabled = false;

	mutex_unlock(&port->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_disable_port);

int hqmv2_dp_ioctl_start_domain(struct hqmv2_dp_domain *domain)
{
	struct hqmv2_start_domain_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;

	ret = hqmv2_domain_ioctl_start_domain(domain->dev,
					      (unsigned long)&ioctl_args,
					      domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return ret;
}

int hqmv2_start_sched_domain(struct hqmv2_domain_hdl *hdl)
{
	struct hqmv2_dp_domain *domain;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;

	if (domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}

	mutex_lock(&domain->resource_mutex);

	if (!domain->thread.started) {
		ret = -ESRCH;
		mutex_unlock(&domain->resource_mutex);
		goto cleanup;
	}

	ret = hqmv2_dp_ioctl_start_domain(domain);
	if (ret) {
		mutex_unlock(&domain->resource_mutex);
		goto cleanup;
	}

	domain->started = true;

	ret = 0;

	mutex_unlock(&domain->resource_mutex);

cleanup:
	return ret;
}
EXPORT_SYMBOL(hqmv2_start_sched_domain);

static int hqmv2_dp_ioctl_enqueue_domain_alert(struct hqmv2_dp_domain *domain,
					       u64 aux_alert_data)
{
	struct hqmv2_enqueue_domain_alert_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	unsigned long args_ptr = (unsigned long)&ioctl_args;
	int ret;

	ioctl_args.response = (uintptr_t)&response;
	ioctl_args.aux_alert_data = aux_alert_data;

	ret = hqmv2_domain_ioctl_enqueue_domain_alert(domain->dev,
						      args_ptr,
						      domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return ret;
}

int hqmv2_reset_sched_domain(struct hqmv2_dp *hqmv2,
			     int domain_id)
{
	struct hqmv2_dp_domain *domain;
	struct device *device;
	int i, ret = -1;

#ifndef DISABLE_CHECK
	if (hqmv2->magic_num != HQMV2_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = &hqmv2->domains[domain_id];
	device = DEV_FROM_HQMV2_DP_DOMAIN(domain);

	if (!domain->configured) {
		ret = -EINVAL;
		goto cleanup;
	}

	/* A domain handle can't be detached if there are any remaining port
	 * handles, so if there are no domain handles then there are no port
	 * handles.
	 */
	if (!list_empty(&domain->hdl_list_head)) {
		ret = -EINVAL;
		goto cleanup;
	}

	/* Free and iounmap memory associated with the reset ports */
	for (i = 0; i < HQMV2_MAX_NUM_LDB_PORTS; i++) {
		struct hqmv2_port *port = &domain->ldb_ports[i];

		if (port->configured)
			devm_iounmap(domain->dev->hqmv2_device,
				     port->pp_addr);
		memset(port, 0, sizeof(struct hqmv2_port));
	}

	for (i = 0; i < HQMV2_MAX_NUM_DIR_PORTS; i++) {
		struct hqmv2_port *port = &domain->dir_ports[i];

		if (port->configured)
			devm_iounmap(domain->dev->hqmv2_device, port->pp_addr);

		memset(port, 0, sizeof(struct hqmv2_port));
	}

	/* Wake this domain's alert thread and prevent further reads. The thread
	 * may have already exited if the device is unexpectedly reset, so check
	 * the started flag first.
	 */
	mutex_lock(&domain->resource_mutex);

	if (domain->thread.started) {
		u64 data = HQMV2_DOMAIN_USER_ALERT_RESET;

		hqmv2_dp_ioctl_enqueue_domain_alert(domain, data);
	}

	mutex_unlock(&domain->resource_mutex);

	while (1) {
		bool started;

		mutex_lock(&domain->resource_mutex);

		started = domain->thread.started;

		mutex_unlock(&domain->resource_mutex);

		if (!started)
			break;

		schedule();
	}

	/* The domain device file is opened in
	 * hqmv2_ioctl_create_sched_domain(), so close it here. This also
	 * resets the domain.
	 */
	mutex_lock(&hqmv2->dev->resource_mutex);

	ret = hqmv2_close_domain_device_file(domain->dev,
					     domain->id,
					     domain->shutdown);

	mutex_unlock(&hqmv2->dev->resource_mutex);

	if (ret)
		goto cleanup;

	memset(domain, 0, sizeof(struct hqmv2_dp_domain));

	ret = 0;

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_reset_sched_domain);

static int hqmv2_read_domain_device_file(struct hqmv2_dp_domain *domain,
					 struct hqmv2_dp_domain_alert *alert)
{
	struct hqmv2_domain_alert kernel_alert;
	u64 alert_id;
	int ret = -1;

	ret = hqmv2_read_domain_alert(domain->dev,
				      domain->id,
				      &kernel_alert,
				      false);
	if (ret)
		goto cleanup;

	alert->data = kernel_alert.aux_alert_data;
	alert_id = kernel_alert.alert_id;

	switch (alert_id) {
	case HQMV2_DOMAIN_ALERT_DEVICE_RESET:
		alert->id = HQMV2_ALERT_DEVICE_RESET;
		break;

	case HQMV2_DOMAIN_ALERT_USER:
		if (alert->data == HQMV2_DOMAIN_USER_ALERT_RESET)
			alert->id = HQMV2_ALERT_DOMAIN_RESET;
		break;

	default:
		if (alert_id < NUM_HQMV2_DOMAIN_ALERTS)
			HQMV2_ERR(domain->dev->hqmv2_device,
				  "[%s()] Internal error: received kernel alert %s\n",
				  __func__,
				  hqmv2_domain_alert_strings[alert_id]);
		else
			HQMV2_ERR(domain->dev->hqmv2_device,
				  "[%s()] Internal error: received invalid alert id %llu\n",
				  __func__, alert_id);
		ret = -EINVAL;
		break;
	}

cleanup:
	return ret;
}

static int __alert_fn(void *__args)
{
	struct hqmv2_dp_domain *domain = __args;

	while (1) {
		struct hqmv2_dp_domain_alert alert;

		if (hqmv2_read_domain_device_file(domain, &alert))
			break;

		if (domain->thread.fn)
			domain->thread.fn(&alert,
					  domain->id,
					  domain->thread.arg);

		if (alert.id == HQMV2_ALERT_DOMAIN_RESET ||
		    alert.id == HQMV2_ALERT_DEVICE_RESET)
			break;
	}

	mutex_lock(&domain->resource_mutex);

	domain->thread.started = false;

	mutex_unlock(&domain->resource_mutex);

	do_exit(0);

	return 0;
}

int hqmv2_launch_domain_alert_thread(struct hqmv2_domain_hdl *hdl,
				     void (*cb)(struct hqmv2_dp_domain_alert *,
						int, void *),
				     void *cb_arg)
{
	struct task_struct *alert_thread;
	struct hqmv2_dp_domain *domain;
	char name[32];
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;

	mutex_lock(&domain->resource_mutex);

	/* Only one thread per domain allowed */
	if (domain->thread.started) {
		ret = -EEXIST;
		mutex_unlock(&domain->resource_mutex);
		goto cleanup;
	}

	domain->thread.fn = (void (*)(void *, int, void *)) cb;
	domain->thread.arg = cb_arg;

	scnprintf(name, sizeof(name), "domain %d alert thread\n", domain->id);

	alert_thread = kthread_create(__alert_fn, (void *)domain, name);

	if (IS_ERR(alert_thread)) {
		ret = PTR_ERR(alert_thread);
		mutex_unlock(&domain->resource_mutex);
		goto cleanup;
	}

	wake_up_process(alert_thread);

	domain->thread.started = true;

	ret = 0;

	mutex_unlock(&domain->resource_mutex);

cleanup:

	return ret;
}
EXPORT_SYMBOL(hqmv2_launch_domain_alert_thread);

/****************************************/
/* Scheduling domain datapath functions */
/****************************************/

static const bool credits_required[NUM_QE_CMD_TYPES] = {
	false, /* NOOP */
	false, /* BAT_T */
	false, /* REL */
	false, /* REL_t */
	false, /* (unused) */
	false, /* (unused) */
	false, /* (unused) */
	false, /* (unused) */
	true,  /* NEW */
	true,  /* NEW_T */
	true,  /* FWD */
	true,  /* FWD_T */
};

static inline int num_token_pops(struct hqmv2_enqueue_qe *enqueue_qe)
{
	enum hqmv2_event_cmd_t cmd = enqueue_qe->cmd_info.qe_cmd;
	int num = 0;

	/* All token return commands set bit 0. BAT_T is a special case. */
	if (cmd & 0x1) {
		num = 1;
		if (cmd == BAT_T)
			num += enqueue_qe->flow_id;
	}

	return num;
}

static inline bool is_release(struct hqmv2_enqueue_qe *enqueue_qe)
{
	enum hqmv2_event_cmd_t cmd = enqueue_qe->cmd_info.qe_cmd;

	return (cmd == REL || cmd == REL_T);
}

static inline bool is_enq_hcw(struct hqmv2_enqueue_qe *enqueue_qe)
{
	enum hqmv2_event_cmd_t cmd = enqueue_qe->cmd_info.qe_cmd;

	return cmd == NEW || cmd == NEW_T || cmd == FWD || cmd == FWD_T;
}

static inline void __attribute__((always_inline))
copy_send_qe(struct hqmv2_enqueue_qe *dest,
	     struct hqmv2_adv_send *src)
{
	((u64 *)dest)[0] = ((u64 *)src)[0];
	((u64 *)dest)[1] = ((u64 *)src)[1];
}

static bool cmd_releases_hist_list_entry(enum hqmv2_event_cmd_t cmd)
{
	return (cmd == REL || cmd == REL_T || cmd == FWD || cmd == FWD_T);
}

static inline void dec_port_owed_releases(struct hqmv2_port *port,
					  struct hqmv2_enqueue_qe *enqueue_qe)
{
	enum hqmv2_event_cmd_t cmd = enqueue_qe->cmd_info.qe_cmd;

	port->owed_releases -= cmd_releases_hist_list_entry(cmd);
}

static inline void inc_port_owed_releases(struct hqmv2_port *port,
					  int cnt)
{
	port->owed_releases += cnt;
}

static inline void dec_port_owed_tokens(struct hqmv2_port *port,
					struct hqmv2_enqueue_qe *enqueue_qe)
{
	enum hqmv2_event_cmd_t cmd = enqueue_qe->cmd_info.qe_cmd;

	/* All token return commands set bit 0. BAT_T is a special case. */
	if (cmd & 0x1) {
		port->owed_tokens--;
		if (cmd == BAT_T)
			port->owed_tokens -= enqueue_qe->flow_id;
	}
}

static inline void inc_port_owed_tokens(struct hqmv2_port *port, int cnt)
{
	port->owed_tokens += cnt;
}

static inline void release_port_credits(struct hqmv2_port *port)
{
	/* When a port's local credit cache reaches a threshold, release them
	 * back to the domain's pool.
	 */

	if (port->num_credits[LDB] >= 2 * HQMV2_SW_CREDIT_BATCH_SZ) {
		atomic_add(HQMV2_SW_CREDIT_BATCH_SZ, port->credit_pool[LDB]);
		port->num_credits[LDB] -= HQMV2_SW_CREDIT_BATCH_SZ;
	}

	if (port->num_credits[DIR] >= 2 * HQMV2_SW_CREDIT_BATCH_SZ) {
		atomic_add(HQMV2_SW_CREDIT_BATCH_SZ, port->credit_pool[DIR]);
		port->num_credits[DIR] -= HQMV2_SW_CREDIT_BATCH_SZ;
	}
}

static inline void refresh_port_credits(struct hqmv2_port *port,
					enum hqmv2_port_type type)
{
	u32 credits = atomic_read(port->credit_pool[type]);
	u32 batch_size = HQMV2_SW_CREDIT_BATCH_SZ;
	u32 new;

	if (!credits)
		return;

	batch_size = (credits < batch_size) ? credits : batch_size;

	new = credits - batch_size;

	if (atomic_cmpxchg(port->credit_pool[type], credits, new) == credits)
		port->num_credits[type] += batch_size;
}

static inline void inc_port_credits(struct hqmv2_port *port,
				    int num)
{
	port->num_credits[port->type] += num;
}

static inline int __attribute__((always_inline))
__hqmv2_adv_send_no_credits(struct hqmv2_port_hdl *hdl,
			    u32 num,
			    union hqmv2_event *evts,
			    bool issue_store_fence,
			    int *error)
{
	struct hqmv2_enqueue_qe *enqueue_qe;
	struct hqmv2_port *port = NULL;
	int i, j, ret, count;

	count = 0;
	ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != PORT_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	enqueue_qe = hdl->qe;
	port = hdl->port;

#ifndef DISABLE_CHECK
	if (!port->domain->started) {
		ret = -EPERM;
		goto cleanup;
	}
#endif

	/* Process the send events. HQM accepts 4 QEs (one cache line's worth)
	 * at a time, so process in chunks of four.
	 */
	for (i = 0; i < num; i += 4) {
		if (issue_store_fence)
			/* Use a store fence to ensure that only one
			 * write-combining operation is present from this core
			 * on the system bus at a time.
			 */
			wmb();

		/* Initialize the four commands to NOOP and zero int_arm and
		 * rsvd
		 */
		enqueue_qe[0].cmd_byte = NOOP;
		enqueue_qe[1].cmd_byte = NOOP;
		enqueue_qe[2].cmd_byte = NOOP;
		enqueue_qe[3].cmd_byte = NOOP;

		for (j = 0; j < 4 && (i + j) < num; j++, count++) {
			struct hqmv2_adv_send *adv_send;

			adv_send = &evts[i + j].adv_send;

			/* Copy the 16B QE */
			copy_send_qe(&enqueue_qe[j], adv_send);

			/* Zero out meas_lat, no_dec, cmp_id, int_arm, error,
			 * and rsvd.
			 */
			((struct hqmv2_adv_send *)&enqueue_qe[j])->rsvd1 = 0;
			((struct hqmv2_adv_send *)&enqueue_qe[j])->rsvd2 = 0;

			dec_port_owed_tokens(port, &enqueue_qe[j]);
			dec_port_owed_releases(port, &enqueue_qe[j]);
		}

		if (j != 0)
			port->enqueue_four(enqueue_qe, port->pp_addr);

		if (j != 4)
			break;
	}

	ret = 0;

cleanup:

	if (port)
		release_port_credits(port);

	if (error)
		*error = ret;

	return count;
}

static inline int __attribute__((always_inline))
__hqmv2_adv_send(struct hqmv2_port_hdl *hdl,
		 u32 num,
		 union hqmv2_event *evts,
		 int *error,
		 bool issue_store_fence,
		 bool credits_required_for_all_cmds)
{
	int used_credits[NUM_PORT_TYPES];
	struct hqmv2_enqueue_qe *enqueue_qe;
	struct hqmv2_dp_domain *domain;
	struct hqmv2_port *port = NULL;
	int i, j, ret, count;

	count = 0;
	ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != PORT_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	enqueue_qe = hdl->qe;
	port = hdl->port;
	domain = port->domain;

#ifndef DISABLE_CHECK
	if (!domain->started) {
		ret = -EPERM;
		goto cleanup;
	}
#endif

	for (i = 0; i < num; i++) {
		enum hqmv2_port_type sched_type;
		bool queue_valid;
		int queue_id;

		sched_type = (evts[i].adv_send.sched_type == SCHED_DIRECTED);
		queue_id = evts[i].adv_send.queue_id;
		queue_valid = domain->queue_valid[sched_type][queue_id];

		if (!is_enq_hcw((struct hqmv2_enqueue_qe *)&evts[i]))
			continue;

#ifndef DISABLE_CHECK
		if (!queue_valid) {
			ret = -EINVAL;
			goto cleanup;
		}
#endif
	}

	used_credits[DIR] = 0;
	used_credits[LDB] = 0;

	/* Process the send events. HQM accepts 4 QEs (one cache line's worth)
	 * at a time, so process in chunks of four.
	 */
	for (i = 0; i < num; i += 4) {
		if (issue_store_fence)
			/* Use a store fence to ensure that writes to the
			 * pointed-to data have completed before enqueueing the
			 * HCW, and that only one HCW from this core is on the
			 * system bus at a time.
			 */
			wmb();

		/* Initialize the four commands to NOOP and zero int_arm and
		 * rsvd.
		 */
		enqueue_qe[0].cmd_byte = NOOP;
		enqueue_qe[1].cmd_byte = NOOP;
		enqueue_qe[2].cmd_byte = NOOP;
		enqueue_qe[3].cmd_byte = NOOP;

		for (j = 0; j < 4 && (i + j) < num; j++, count++) {
			struct hqmv2_adv_send *adv_send;
			enum hqmv2_port_type type;

			adv_send = &evts[i + j].adv_send;

			type = (adv_send->sched_type == SCHED_DIRECTED);

			/* Copy the 16B QE */
			copy_send_qe(&enqueue_qe[j], adv_send);

			/* Zero out meas_lat, no_dec, cmp_id, int_arm, error,
			 * and rsvd.
			 */
			((struct hqmv2_adv_send *)&enqueue_qe[j])->rsvd1 = 0;
			((struct hqmv2_adv_send *)&enqueue_qe[j])->rsvd2 = 0;

			dec_port_owed_tokens(port, &enqueue_qe[j]);
			dec_port_owed_releases(port, &enqueue_qe[j]);

			if (!credits_required_for_all_cmds &&
			    !credits_required[adv_send->cmd])
				continue;

			/* Check credit availability */
			if (port->num_credits[type] == used_credits[type]) {
				/* Check if the device has replenished this
				 * port's credits.
				 */
				refresh_port_credits(port, type);

				if (port->num_credits[type] ==
				    used_credits[type]) {
					/* Undo the 16B QE copy by setting cmd
					 * to NOOP.
					 */
					enqueue_qe[j].cmd_byte = 0;
					break;
				}
			}

			used_credits[type]++;
		}

		if (j != 0)
			port->enqueue_four(enqueue_qe, port->pp_addr);

		if (j != 4)
			break;
	}

	port->num_credits[LDB] -= used_credits[LDB];
	port->num_credits[DIR] -= used_credits[DIR];

	ret = 0;

cleanup:

	if (port)
		release_port_credits(port);

	if (error)
		*error = ret;

	return count;
}

static inline int hqmv2_adv_send_wrapper(struct hqmv2_port_hdl *hdl,
					 u32 num,
					 union hqmv2_event *send,
					 int *err,
					 enum hqmv2_event_cmd_t cmd)
{
	struct hqmv2_port *port;
	int i, ret = -1;
	bool is_bat;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != PORT_MAGIC_NUM) {
		if (err)
			*err = -EINVAL;
		ret = 0;
		goto cleanup;
	}
#endif

	port = hdl->port;

	if (port->domain->shutdown) {
		if (err)
			*err = -EINTR;
		ret = 0;
		goto cleanup;
	}
#ifndef DISABLE_CHECK
	if (!send) {
		if (err)
			*err = -EINVAL;
		ret = 0;
		goto cleanup;
	}

	if (!port->domain->started) {
		if (err)
			*err = -EPERM;
		ret = 0;
		goto cleanup;
	}
#endif

	for (i = 0; i < num; i++)
		send[i].adv_send.cmd = cmd;

	is_bat = (cmd == BAT_T);

	/* Since we're sending the same command for all events, we can use
	 * specialized send functions according to whether or not credits
	 * are required.
	 *
	 * A store fence isn't required if this is a BAT_T command, which is
	 * safe to reorder and doesn't point to any data.
	 */
	if (credits_required[cmd])
		ret = __hqmv2_adv_send(hdl, num, send, err, true, true);
	else
		ret = __hqmv2_adv_send_no_credits(hdl, num, send, !is_bat, err);

cleanup:
	return ret;
}

int hqmv2_send(struct hqmv2_port_hdl *hdl,
	       u32 num,
	       union hqmv2_event *event,
	       int *error)
{
	return hqmv2_adv_send_wrapper(hdl, num, event, error, NEW);
}
EXPORT_SYMBOL(hqmv2_send);

int hqmv2_release(struct hqmv2_port_hdl *hdl,
		  u32 num,
		  int *error)
{
#define REL_BATCH_SZ 4
	/* This variable intentionally left blank */
	union hqmv2_event send[REL_BATCH_SZ];
	struct hqmv2_port *port;
	int i, ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != PORT_MAGIC_NUM) {
		if (error)
			*error = -EINVAL;
		ret = 0;
		goto cleanup;
	}
#endif

	port = hdl->port;

#ifndef DISABLE_CHECK
	if (port->type != LDB) {
		if (error)
			*error = -EINVAL;
		ret = 0;
		goto cleanup;
	}
#endif

	/* Prevent the user from releasing more events than are owed. */
	num = (num < port->owed_releases) ? num : port->owed_releases;

	for (i = 0; i < num; i += REL_BATCH_SZ) {
		int n, num_to_send = min_t(u32, REL_BATCH_SZ, num);

		n = hqmv2_adv_send_wrapper(hdl, num_to_send, send, error, REL);

		ret += n;

		if (n != num_to_send)
			break;
	}

cleanup:
	return ret;
}
EXPORT_SYMBOL(hqmv2_release);

int hqmv2_forward(struct hqmv2_port_hdl *hdl,
		  u32 num,
		  union hqmv2_event *event,
		  int *error)
{
	return hqmv2_adv_send_wrapper(hdl, num, event, error, FWD);
}
EXPORT_SYMBOL(hqmv2_forward);

int hqmv2_pop_cq(struct hqmv2_port_hdl *hdl,
		 u32 num,
		 int *error)
{
	/* Self-initialize send so that GCC doesn't issue a "may be
	 * uninitialized" warning when the udata64 field (which is
	 * intentionally uninitialized) is dereferenced in copy_send_qe().
	 */
	struct hqmv2_adv_send send = send;

	struct hqmv2_port *port;
	int ret;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != PORT_MAGIC_NUM) {
		if (error)
			*error = -EINVAL;
		ret = 0;
		goto cleanup;
	}
#endif

	port = hdl->port;

	/* Prevent the user from popping more tokens than are owed. This is
	 * required when using hqmv2_recv_no_pop() and CQ interrupts (see
	 * __hqmv2_block_on_cq_interrupt() for more details), and prevents user
	 * errors when using hqmv2_recv().
	 */
	send.num_tokens_minus_one = (num < port->owed_tokens) ?
				     num : port->owed_tokens;
	if (send.num_tokens_minus_one == 0)
		return 0;

	/* The BAT_T count is zero-based so decrement num_tokens_minus_one */
	send.num_tokens_minus_one--;

	ret = hqmv2_adv_send_wrapper(hdl,
				     1,
				     (union hqmv2_event *)&send,
				     error,
				     BAT_T);

cleanup:
	return ret;
}
EXPORT_SYMBOL(hqmv2_pop_cq);

static inline void __attribute__((always_inline))
copy_recv_qe(struct hqmv2_recv *dest,
	     struct hqmv2_dequeue_qe *src)
{
	((u64 *)dest)[0] = ((u64 *)src)[0];
	((u64 *)dest)[1] = ((u64 *)src)[1];
}

static inline void __hqmv2_issue_int_arm_hcw(struct hqmv2_port_hdl *hdl,
					     struct hqmv2_port *port)
{
	struct hqmv2_enqueue_qe *enqueue_qe = hdl->qe;

	memset(enqueue_qe, 0, sizeof(struct hqmv2_enqueue_qe) * 4);

	enqueue_qe[0].cmd_byte = CMD_ARM;
	/* Initialize the other commands to NOOP and zero int_arm and rsvd */
	enqueue_qe[1].cmd_byte = NOOP;
	enqueue_qe[2].cmd_byte = NOOP;
	enqueue_qe[3].cmd_byte = NOOP;

	port->enqueue_four(enqueue_qe, port->pp_addr);
}

static int hqmv2_dp_ioctl_block_on_cq_interrupt(struct hqmv2_dp_domain *domain,
						int port_id,
						bool is_ldb,
						struct hqmv2_dequeue_qe *cq_va,
						u8 cq_gen,
						bool arm)
{
	struct hqmv2_block_on_cq_interrupt_args ioctl_args = {0};
	unsigned long args_ptr = (unsigned long)&ioctl_args;
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;
	ioctl_args.port_id = port_id;
	ioctl_args.is_ldb = is_ldb;
	ioctl_args.cq_va = (uintptr_t)cq_va;
	ioctl_args.cq_gen = cq_gen;
	ioctl_args.arm = arm;

	ret = hqmv2_domain_ioctl_block_on_cq_interrupt(domain->dev,
						       args_ptr,
						       domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return ret;
}

static inline int __hqmv2_block_on_cq_interrupt(struct hqmv2_port_hdl *hdl,
						struct hqmv2_port *port)
{
	int ret;

	/* If the interrupt is not armed, either sleep-poll (see comment below)
	 * or arm the interrupt.
	 */
	if (!port->int_armed)
		__hqmv2_issue_int_arm_hcw(hdl, port);

	ret = hqmv2_dp_ioctl_block_on_cq_interrupt(port->domain,
						   port->id,
						   port->type == LDB,
						   &port->cq_base[port->cq_idx],
						   port->cq_gen,
						   false);

	/* If the CQ int ioctl was unsuccessful, the interrupt remains armed */
	port->int_armed = (ret != 0);

	return ret;
}

static inline bool port_cq_is_empty(struct hqmv2_port *port)
{
	return READ_ONCE(port->cq_base[port->cq_idx]).cq_gen != port->cq_gen;
}

static inline int __hqmv2_recv(struct hqmv2_port_hdl *hdl,
			       u32 max,
			       bool wait,
			       bool pop,
			       struct hqmv2_recv *event,
			       int *err)
{
	struct hqmv2_port *port;
	int ret = -1;
	int cnt = 0;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != PORT_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	port = hdl->port;

	if (port->domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}
#ifndef DISABLE_CHECK
	if (!port->domain->started) {
		ret = -EPERM;
		goto cleanup;
	}
#endif

	/* If the port is disabled and its CQ is empty, notify the user */
	if (!port->enabled && port_cq_is_empty(port)) {
		ret = -EACCES;
		goto cleanup;
	}

	/* Wait until at least one QE is available if wait == true */
	/* Future work: wait profile */
	while (wait && port_cq_is_empty(port)) {
		if (__hqmv2_block_on_cq_interrupt(hdl, port)) {
			ret = -EINTR;
			goto cleanup;
		}
		if (READ_ONCE(port->domain->shutdown)) {
			ret = -EINTR;
			goto cleanup;
		}
		/* Return if the port is disabled and its CQ is empty */
		if (!port->enabled && port_cq_is_empty(port)) {
			ret = -EACCES;
			goto cleanup;
		}
	}

	ret = 0;

	for (cnt = 0; cnt < max; cnt++) {
		/* TODO: optimize cq_base and other port-> structures */
		if (port_cq_is_empty(port))
			break;

		/* Copy the 16B QE into the user's event structure */
		copy_recv_qe(&event[cnt], &port->cq_base[port->cq_idx]);

		port->cq_idx++;

		if (unlikely(port->cq_idx == port->cq_depth)) {
			port->cq_gen ^= 1;
			port->cq_idx = 0;
		}
	}

	inc_port_owed_tokens(port, cnt);
	inc_port_owed_releases(port, cnt);

	inc_port_credits(port, cnt);

	if (pop && cnt > 0)
		hqmv2_pop_cq(hdl, cnt, NULL);

cleanup:

	if (err)
		*err = ret;

	return cnt;
}

int hqmv2_recv(struct hqmv2_port_hdl *hdl,
	       u32 max,
	       bool wait,
	       union hqmv2_event *event,
	       int *err)
{
	return __hqmv2_recv(hdl, max, wait, true, &event->recv, err);
}
EXPORT_SYMBOL(hqmv2_recv);

int hqmv2_recv_no_pop(struct hqmv2_port_hdl *hdl,
		      u32 max,
		      bool wait,
		      union hqmv2_event *event,
		      int *err)
{
	return __hqmv2_recv(hdl, max, wait, false, &event->recv, err);
}
EXPORT_SYMBOL(hqmv2_recv_no_pop);

/************************************/
/* Advanced Configuration Functions */
/************************************/

#define CQ_BASE(type) ((type == LDB) ? HQMV2_LDB_CQ_BASE : HQMV2_DIR_CQ_BASE)

static int map_consumer_queue(struct hqmv2_dev *dev, struct hqmv2_port *port)
{
	if (port->type == LDB)
		port->cq_base = dev->ldb_port_pages[port->id].cq_base;
	else
		port->cq_base = dev->dir_port_pages[port->id].cq_base;

	return (!port->cq_base) ? -1 : 0;
}

static int map_producer_port(struct hqmv2_dev *dev, struct hqmv2_port *port)
{
	port->pp_addr = devm_ioremap_wc(dev->hqmv2_device,
					dev->hw.func_phys_addr +
						PP_BASE(port->type) +
						port->id * PAGE_SIZE,
					PAGE_SIZE);

	return (!port->pp_addr) ? -1 : 0;
}

int hqmv2_dp_ioctl_create_ldb_port(struct hqmv2_dp_domain *domain,
				   struct hqmv2_create_port *args,
				   struct hqmv2_create_port_adv *adv_args)
{
	struct hqmv2_create_ldb_port_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;

	ioctl_args.cq_depth = args->cq_depth;
	ioctl_args.cq_depth_threshold = 1;
	ioctl_args.cq_history_list_size = adv_args->cq_history_list_size;

	ret = hqmv2_domain_ioctl_create_ldb_port(domain->dev,
						 (unsigned long)&ioctl_args,
						 domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return (ret == 0) ? response.id : ret;
}

static int hqmv2_create_ldb_port_adv(struct hqmv2_domain_hdl *hdl,
				     struct hqmv2_create_port *args,
				     struct hqmv2_create_port_adv *adv_args)
{
	struct hqmv2_sw_credit_pool *ldb_pool;
	struct hqmv2_sw_credit_pool *dir_pool;
	struct hqmv2_port *port = NULL;
	struct hqmv2_dp_domain *domain;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;

	if (domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}

	mutex_lock(&domain->resource_mutex);

	ret = hqmv2_dp_ioctl_create_ldb_port(domain, args, adv_args);
	if (ret < 0) {
		mutex_unlock(&domain->resource_mutex);
		goto cleanup;
	}

	port = &domain->ldb_ports[ret];

	port->id = ret;
	port->domain = domain;
	port->type = LDB;
	mutex_init(&port->resource_mutex);

	port->pp_addr = NULL;
	port->cq_base = NULL;

	ret = map_producer_port(domain->dev, port);
	if (ret) {
		mutex_unlock(&domain->resource_mutex);
		goto cleanup;
	}

	ret = map_consumer_queue(domain->dev, port);
	if (ret) {
		mutex_unlock(&domain->resource_mutex);
		goto cleanup;
	}

	if (movdir64b_supported())
		port->enqueue_four = hqmv2_movdir64b;
	else
		port->enqueue_four = hqmv2_movntdq;

	ldb_pool = &domain->sw_credits.ldb_pools[args->ldb_credit_pool_id];
	dir_pool = &domain->sw_credits.dir_pools[args->dir_credit_pool_id];

	port->credit_pool[LDB] = &ldb_pool->avail_credits;
	port->credit_pool[DIR] = &dir_pool->avail_credits;
	port->num_credits[LDB] = 0;
	port->num_credits[DIR] = 0;

	port->cq_depth = args->cq_depth;
	port->cq_idx = 0;
	port->cq_gen = 1;

	port->int_armed = false;

	WRITE_ONCE(port->enabled, true);
	port->configured = true;

	ret = port->id;

	mutex_unlock(&domain->resource_mutex);

cleanup:

	if (ret < 0) {
		if (port && port->pp_addr)
			devm_iounmap(domain->dev->hqmv2_device,
				     port->pp_addr);
	}

	return ret;
}

int hqmv2_dp_ioctl_create_dir_port(struct hqmv2_dp_domain *domain,
				   struct hqmv2_create_port *args,
				   struct hqmv2_create_port_adv *adv_args,
				   int queue_id)
{
	struct hqmv2_create_dir_port_args ioctl_args = {0};
	struct hqmv2_cmd_response response = {0};
	int ret;

	ioctl_args.response = (uintptr_t)&response;

	ioctl_args.cq_depth = args->cq_depth;
	ioctl_args.cq_depth_threshold = 1;

	ioctl_args.queue_id = queue_id;

	ret = hqmv2_domain_ioctl_create_dir_port(domain->dev,
						 (unsigned long)&ioctl_args,
						 domain->id);

	hqmv2_log_ioctl_error(domain->dev->hqmv2_device, ret, response.status);

	return (ret == 0) ? response.id : ret;
}

static int hqmv2_create_dir_port_adv(struct hqmv2_domain_hdl *hdl,
				     struct hqmv2_create_port *args,
				     struct hqmv2_create_port_adv *adv_args,
				     int queue_id)
{
	struct hqmv2_sw_credit_pool *ldb_pool;
	struct hqmv2_sw_credit_pool *dir_pool;
	struct hqmv2_port *port = NULL;
	struct hqmv2_dp_domain *domain;
	int ret = -1;

#ifndef DISABLE_CHECK
	if (hdl->magic_num != DOMAIN_MAGIC_NUM) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	domain = hdl->domain;

	if (domain->shutdown) {
		ret = -EINTR;
		goto cleanup;
	}

	mutex_lock(&domain->resource_mutex);

	ret = hqmv2_dp_ioctl_create_dir_port(domain, args, adv_args, queue_id);
	if (ret < 0) {
		mutex_unlock(&domain->resource_mutex);
		goto cleanup;
	}

	port = &domain->dir_ports[ret];

	port->id = ret;
	port->domain = domain;
	port->type = DIR;
	mutex_init(&port->resource_mutex);

	port->pp_addr = NULL;
	port->cq_base = NULL;

	ret = map_producer_port(domain->dev, port);
	if (ret) {
		mutex_unlock(&domain->resource_mutex);
		goto cleanup;
	}

	ret = map_consumer_queue(domain->dev, port);
	if (ret) {
		mutex_unlock(&domain->resource_mutex);
		goto cleanup;
	}

	ldb_pool = &domain->sw_credits.ldb_pools[args->ldb_credit_pool_id];
	dir_pool = &domain->sw_credits.dir_pools[args->dir_credit_pool_id];

	port->credit_pool[LDB] = &ldb_pool->avail_credits;
	port->credit_pool[DIR] = &dir_pool->avail_credits;
	port->num_credits[LDB] = 0;
	port->num_credits[DIR] = 0;

	port->cq_depth = args->cq_depth;
	port->cq_idx = 0;
	port->cq_gen = 1;

	port->int_armed = false;

	if (movdir64b_supported())
		port->enqueue_four = hqmv2_movdir64b;
	else
		port->enqueue_four = hqmv2_movntdq;

	port->enabled = true;
	port->configured = true;

	ret = port->id;

	mutex_unlock(&domain->resource_mutex);

cleanup:

	if (ret < 0) {
		if (port && port->pp_addr)
			devm_iounmap(domain->dev->hqmv2_device, port->pp_addr);
	}

	return ret;
}

/*******************************/
/* Advanced Datapath Functions */
/*******************************/

int hqmv2_adv_send(struct hqmv2_port_hdl *hdl,
		   u32 num,
		   union hqmv2_event *evts,
		   int *error)
{
	int ret;

#ifndef DISABLE_CHECK
	if (!evts) {
		ret = -EINVAL;
		goto cleanup;
	}
#endif

	/* Setting credits_required_for_all_cmds to false means that some
	 * events *may* need credits, so __hqmv2_adv_send() needs to check every
	 * event's cmd.
	 */
	ret = __hqmv2_adv_send(hdl, num, evts, error, true, false);
cleanup:

	return ret;
}
