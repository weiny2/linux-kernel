/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2020 Intel Corporation
 */

#ifndef __DLB2_IOCTL_DP_H
#define __DLB2_IOCTL_DP_H

#include "dlb2_dp_priv.h"
#include "dlb2_main.h"

int dlb2_ioctl_get_num_resources(struct dlb2_dev *dev,
				 unsigned long user_arg);

int __dlb2_ioctl_create_sched_domain(struct dlb2_dev *dev,
				     unsigned long user_arg,
				     bool user,
				     struct dlb2_dp *dp);

int dlb2_domain_ioctl_create_ldb_queue(struct dlb2_dev *dev,
				       unsigned long user_arg,
				       uint32_t domain_id);

int dlb2_domain_ioctl_create_dir_queue(struct dlb2_dev *dev,
				       unsigned long user_arg,
				       uint32_t domain_id);

int dlb2_domain_ioctl_create_ldb_port(struct dlb2_dev *dev,
				      unsigned long user_arg,
				      uint32_t domain_id);

int dlb2_domain_ioctl_create_dir_port(struct dlb2_dev *dev,
				      unsigned long user_arg,
				      uint32_t domain_id);

int dlb2_domain_ioctl_map_qid(struct dlb2_dev *dev,
			      unsigned long user_arg,
			      uint32_t domain_id);

int dlb2_domain_ioctl_unmap_qid(struct dlb2_dev *dev,
				unsigned long user_arg,
				uint32_t domain_id);

int dlb2_domain_ioctl_enable_ldb_port(struct dlb2_dev *dev,
				      unsigned long user_arg,
				      uint32_t domain_id);

int dlb2_domain_ioctl_enable_dir_port(struct dlb2_dev *dev,
				      unsigned long user_arg,
				      uint32_t domain_id);

int dlb2_domain_ioctl_disable_ldb_port(struct dlb2_dev *dev,
				       unsigned long user_arg,
				       uint32_t domain_id);

int dlb2_domain_ioctl_disable_dir_port(struct dlb2_dev *dev,
				       unsigned long user_arg,
				       uint32_t domain_id);

int dlb2_domain_ioctl_start_domain(struct dlb2_dev *dev,
				   unsigned long user_arg,
				   uint32_t domain_id);

int dlb2_domain_ioctl_block_on_cq_interrupt(struct dlb2_dev *dev,
					    unsigned long user_arg,
					    uint32_t domain_id);

int dlb2_domain_ioctl_enqueue_domain_alert(struct dlb2_dev *dev,
					   unsigned long user_arg,
					   uint32_t domain_id);

#endif /* __DLB2_IOCTL_DP_H */
