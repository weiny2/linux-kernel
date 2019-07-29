/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2018 Intel Corporation
 */

#ifndef __HQMV2_IOCTL_DP_H
#define __HQMV2_IOCTL_DP_H

#include "hqmv2_main.h"
#include "hqmv2_dp_priv.h"

int hqmv2_ioctl_get_num_resources(struct hqmv2_dev *dev,
				  unsigned long user_arg);

int __hqmv2_ioctl_create_sched_domain(struct hqmv2_dev *dev,
				      unsigned long user_arg,
				      bool user,
				      struct hqmv2_dp *dp);

int hqmv2_domain_ioctl_create_ldb_queue(struct hqmv2_dev *dev,
					unsigned long user_arg,
					uint32_t domain_id);

int hqmv2_domain_ioctl_create_dir_queue(struct hqmv2_dev *dev,
					unsigned long user_arg,
					uint32_t domain_id);

int hqmv2_domain_ioctl_create_ldb_port(struct hqmv2_dev *dev,
				       unsigned long user_arg,
				       uint32_t domain_id);

int hqmv2_domain_ioctl_create_dir_port(struct hqmv2_dev *dev,
				       unsigned long user_arg,
				       uint32_t domain_id);

int hqmv2_domain_ioctl_map_qid(struct hqmv2_dev *dev,
			       unsigned long user_arg,
			       uint32_t domain_id);

int hqmv2_domain_ioctl_unmap_qid(struct hqmv2_dev *dev,
				 unsigned long user_arg,
				 uint32_t domain_id);

int hqmv2_domain_ioctl_enable_ldb_port(struct hqmv2_dev *dev,
				       unsigned long user_arg,
				       uint32_t domain_id);

int hqmv2_domain_ioctl_enable_dir_port(struct hqmv2_dev *dev,
				       unsigned long user_arg,
				       uint32_t domain_id);

int hqmv2_domain_ioctl_disable_ldb_port(struct hqmv2_dev *dev,
					unsigned long user_arg,
					uint32_t domain_id);

int hqmv2_domain_ioctl_disable_dir_port(struct hqmv2_dev *dev,
					unsigned long user_arg,
					uint32_t domain_id);

int hqmv2_domain_ioctl_start_domain(struct hqmv2_dev *dev,
				    unsigned long user_arg,
				    uint32_t domain_id);

int hqmv2_domain_ioctl_block_on_cq_interrupt(struct hqmv2_dev *dev,
					     unsigned long user_arg,
					     uint32_t domain_id);

int hqmv2_domain_ioctl_enqueue_domain_alert(struct hqmv2_dev *dev,
					    unsigned long user_arg,
					    uint32_t domain_id);

#endif /* __HQMV2_IOCTL_DP_H */
