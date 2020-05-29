/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2020 Intel Corporation
 */

#ifndef __DLB2_MAIN_H
#define __DLB2_MAIN_H

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/wait.h>

#include <uapi/linux/dlb2_user.h>

#include "base/dlb2_hw_types.h"

static const char dlb2_driver_name[] = KBUILD_MODNAME;

#define DLB2_NUM_FUNCS_PER_DEVICE (1 + DLB2_MAX_NUM_VDEVS)
#define DLB2_MAX_NUM_DEVICES (DLB2_NUM_FUNCS_PER_DEVICE)
#define DLB2_NUM_DEV_FILES_PER_DEVICE (DLB2_MAX_NUM_DOMAINS + 1)
#define DLB2_MAX_NUM_DEVICE_FILES (DLB2_MAX_NUM_DEVICES * \
				   DLB2_NUM_DEV_FILES_PER_DEVICE)
/* This macro returns an ID from 0 to DLB2_NUM_DEV_FILES_PER_DEVICE,
 * inclusive.
 */
#define DLB2_FILE_ID_FROM_DEV_T(base, file) ((MINOR(file) - MINOR(base)) % \
					     DLB2_NUM_DEV_FILES_PER_DEVICE)
/* This macro returns an ID from 0 to DLB2_MAX_NUM_DEVICES, inclusive */
#define DLB2_DEV_ID_FROM_DEV_T(base, file) ((MINOR(file) - MINOR(base)) / \
					    DLB2_NUM_DEV_FILES_PER_DEVICE)
#define IS_DLB2_DEV_FILE(base, file) (DLB2_FILE_ID_FROM_DEV_T(base, file) == \
				      DLB2_MAX_NUM_DOMAINS)

#define DLB2_DEFAULT_RESET_TIMEOUT_S 5
#define DLB2_VF_FLR_DONE_POLL_TIMEOUT_MS 1000
#define DLB2_VF_FLR_DONE_SLEEP_PERIOD_MS 1

extern struct list_head dlb2_dev_list;
extern struct mutex dlb2_driver_lock;
extern unsigned int dlb2_qe_sa_pct;
extern unsigned int dlb2_qid_sa_pct;

enum dlb2_device_type {
	DLB2_PF,
	DLB2_VF,
};

struct dlb2_dev;

struct dlb2_device_ops {
	/* Device create? (maybe) */
	int (*map_pci_bar_space)(struct dlb2_dev *dev, struct pci_dev *pdev);
	void (*unmap_pci_bar_space)(struct dlb2_dev *dev,
				    struct pci_dev *pdev);
	int (*mmap)(struct file *f, struct vm_area_struct *vma, u32 id);
	void (*inc_pm_refcnt)(struct pci_dev *pdev, bool resume);
	void (*dec_pm_refcnt)(struct pci_dev *pdev);
	bool (*pm_refcnt_status_suspended)(struct pci_dev *pdev);
	int (*init_driver_state)(struct dlb2_dev *dev);
	void (*free_driver_state)(struct dlb2_dev *dev);
	int (*device_create)(struct dlb2_dev *dlb2_dev,
			     struct pci_dev *pdev,
			     struct class *dlb2_class);
	void (*device_destroy)(struct dlb2_dev *dlb2_dev,
			       struct class *dlb2_class);
	int (*cdev_add)(struct dlb2_dev *dlb2_dev,
			dev_t base,
			const struct file_operations *fops);
	void (*cdev_del)(struct dlb2_dev *dlb2_dev);
	int (*sysfs_create)(struct dlb2_dev *dlb2_dev);
	void (*sysfs_destroy)(struct dlb2_dev *dlb2_dev);
	void (*sysfs_reapply)(struct dlb2_dev *dev);
	int (*init_interrupts)(struct dlb2_dev *dev, struct pci_dev *pdev);
	int (*enable_ldb_cq_interrupts)(struct dlb2_dev *dev,
					int port_id,
					u16 thresh);
	int (*enable_dir_cq_interrupts)(struct dlb2_dev *dev,
					int port_id,
					u16 thresh);
	int (*arm_cq_interrupt)(struct dlb2_dev *dev,
				int domain_id,
				int port_id,
				bool is_ldb);
	void (*reinit_interrupts)(struct dlb2_dev *dev);
	void (*free_interrupts)(struct dlb2_dev *dev, struct pci_dev *pdev);
	void (*enable_pm)(struct dlb2_dev *dev);
	int (*wait_for_device_ready)(struct dlb2_dev *dev,
				     struct pci_dev *pdev);
	int (*register_driver)(struct dlb2_dev *dev);
	void (*unregister_driver)(struct dlb2_dev *dev);
	int (*create_sched_domain)(struct dlb2_hw *hw,
				   struct dlb2_create_sched_domain_args *args,
				   struct dlb2_cmd_response *resp);
	int (*create_ldb_queue)(struct dlb2_hw *hw,
				u32 domain_id,
				struct dlb2_create_ldb_queue_args *args,
				struct dlb2_cmd_response *resp);
	int (*create_dir_queue)(struct dlb2_hw *hw,
				u32 domain_id,
				struct dlb2_create_dir_queue_args *args,
				struct dlb2_cmd_response *resp);
	int (*create_ldb_port)(struct dlb2_hw *hw,
			       u32 domain_id,
			       struct dlb2_create_ldb_port_args *args,
			       uintptr_t cq_dma_base,
			       struct dlb2_cmd_response *resp);
	int (*create_dir_port)(struct dlb2_hw *hw,
			       u32 domain_id,
			       struct dlb2_create_dir_port_args *args,
			       uintptr_t cq_dma_base,
			       struct dlb2_cmd_response *resp);
	int (*start_domain)(struct dlb2_hw *hw,
			    u32 domain_id,
			    struct dlb2_start_domain_args *args,
			    struct dlb2_cmd_response *resp);
	int (*map_qid)(struct dlb2_hw *hw,
		       u32 domain_id,
		       struct dlb2_map_qid_args *args,
		       struct dlb2_cmd_response *resp);
	int (*unmap_qid)(struct dlb2_hw *hw,
			 u32 domain_id,
			 struct dlb2_unmap_qid_args *args,
			 struct dlb2_cmd_response *resp);
	int (*enable_ldb_port)(struct dlb2_hw *hw,
			       u32 domain_id,
			       struct dlb2_enable_ldb_port_args *args,
			       struct dlb2_cmd_response *resp);
	int (*disable_ldb_port)(struct dlb2_hw *hw,
				u32 domain_id,
				struct dlb2_disable_ldb_port_args *args,
				struct dlb2_cmd_response *resp);
	int (*enable_dir_port)(struct dlb2_hw *hw,
			       u32 domain_id,
			       struct dlb2_enable_dir_port_args *args,
			       struct dlb2_cmd_response *resp);
	int (*disable_dir_port)(struct dlb2_hw *hw,
				u32 domain_id,
				struct dlb2_disable_dir_port_args *args,
				struct dlb2_cmd_response *resp);
	int (*get_num_resources)(struct dlb2_hw *hw,
				 struct dlb2_get_num_resources_args *args);
	int (*reset_domain)(struct dlb2_hw *hw, u32 domain_id);
	int (*write_smon)(struct dlb2_dev *dev,
			  struct dlb2_write_smon_args *args,
			  struct dlb2_cmd_response *response);
	int (*read_smon)(struct dlb2_dev *dev,
			 struct dlb2_read_smon_args *args,
			 struct dlb2_cmd_response *response);
	int (*ldb_port_owned_by_domain)(struct dlb2_hw *hw,
					u32 domain_id,
					u32 port_id);
	int (*dir_port_owned_by_domain)(struct dlb2_hw *hw,
					u32 domain_id,
					u32 port_id);
	int (*get_sn_allocation)(struct dlb2_hw *hw, u32 group_id);
	int (*get_sn_occupancy)(struct dlb2_hw *hw, u32 group_id);
	int (*get_ldb_queue_depth)(struct dlb2_hw *hw,
				   u32 domain_id,
				   struct dlb2_get_ldb_queue_depth_args *args,
				   struct dlb2_cmd_response *resp);
	int (*get_dir_queue_depth)(struct dlb2_hw *hw,
				   u32 domain_id,
				   struct dlb2_get_dir_queue_depth_args *args,
				   struct dlb2_cmd_response *resp);
	int (*pending_port_unmaps)(struct dlb2_hw *hw,
				   u32 domain_id,
				   struct dlb2_pending_port_unmaps_args *args,
				   struct dlb2_cmd_response *resp);
	int (*get_cos_bw)(struct dlb2_hw *hw, u32 cos_id);
	void (*init_hardware)(struct dlb2_dev *dev);
	int (*query_cq_poll_mode)(struct dlb2_dev *dev,
				  struct dlb2_cmd_response *user_resp);
};

extern const struct attribute_group *dlb2_vf_attrs[];
extern struct dlb2_device_ops dlb2_pf_ops;
extern struct dlb2_device_ops dlb2_vf_ops;

struct dlb2_port_memory {
	void *cq_base;
	dma_addr_t cq_dma_base;
	int domain_id;
	u8 valid;
};

struct dlb2_status {
	u8 valid;
	u32 refcnt;
};

#define DLB2_DOMAIN_ALERT_RING_SIZE 256

struct dlb2_domain_dev {
	u8 user_mode;
#ifdef CONFIG_INTEL_DLB2_DATAPATH
	struct dlb2_dp_domain *dp;
#endif
	struct dlb2_status *status;
	struct dlb2_domain_alert alerts[DLB2_DOMAIN_ALERT_RING_SIZE];
	u8 alert_rd_idx;
	u8 alert_wr_idx;
	/* The alert mutex protects access to the alert ring and its read and
	 * write indexes.
	 */
	struct mutex alert_mutex;
	wait_queue_head_t wq_head;
};

struct dlb2_cq_intr {
	u8 wake;
	u8 configured;
	/* disabled is true if the port is disabled. In that
	 * case, the driver doesn't allow applications to block on the
	 * port's interrupt.
	 */
	u8 disabled;
	wait_queue_head_t wq_head;
	/* The CQ interrupt mutex guarantees one thread is blocking on a CQ's
	 * interrupt at a time.
	 */
	struct mutex mutex;
} __aligned(64);

struct dlb2_vdev_cq_int_info {
	char name[32];
	u8 is_ldb;
	s16 port_id;
};

struct dlb2_intr {
	int num_vectors;
	int base_vector;
	int mode;
	/* The VDEV has more possible interrupt vectors than the PF or VF, so
	 * we simply over-allocate in those cases
	 */
	u8 isr_registered[DLB2_VDEV_MAX_NUM_INTERRUPT_VECTORS];
	struct dlb2_cq_intr ldb_cq_intr[DLB2_MAX_NUM_LDB_PORTS];
	struct dlb2_cq_intr dir_cq_intr[DLB2_MAX_NUM_DIR_PORTS];
	int num_ldb_ports;
	int num_dir_ports;

	/* The following state is VF/VDEV only */
	u8 num_cq_intrs;
	struct dlb2_vdev_cq_int_info
		msi_map[DLB2_VDEV_MAX_NUM_INTERRUPT_VECTORS];
	struct dlb2_dev *ldb_cq_intr_owner[DLB2_MAX_NUM_LDB_PORTS];
	struct dlb2_dev *dir_cq_intr_owner[DLB2_MAX_NUM_DIR_PORTS];
};

struct dlb2_datapath {
	/* Linked list of datapath handles */
	struct list_head hdl_list;
};

struct vf_id_state {
	/* pf_id and vf_id contain unique identifiers given by the PF at driver
	 * register time. These IDs can be used to identify VFs from the same
	 * physical device.
	 */
	u8 pf_id;
	u8 vf_id;
	/* An auxiliary VF has no resources of its own. It exists to provide
	 * its primary VF sibling with MSI vectors, so a VF user can exceed the
	 * 31 vector per VF limit.
	 */
	u8 is_auxiliary_vf;
	/* If this is an auxiliary VF, primary_vf_id is the 'vf_id' of its
	 * primary sibling.
	 */
	u8 primary_vf_id;
	/* If this is an auxiliary VF, primary_vf points to the dlb2_dev
	 * structure of its primary sibling.
	 */
	struct dlb2_dev *primary_vf;
};

struct dlb2_vf_perf_metric_data {
	u64 ldb_cq[DLB2_MAX_NUM_LDB_PORTS];
	u64 dir_cq[DLB2_MAX_NUM_DIR_PORTS];
	u32 elapsed;
	u8 valid;
};

struct dlb2_vma_node {
	struct list_head list;
	struct vm_area_struct *vma;
};

/* ISR overload is defined as more than DLB2_ISR_OVERLOAD_THRESH interrupts
 * (of a particular type) occurring in a 1s period. If overload is detected,
 * the driver blocks that interrupt (exact mechanism depending on the
 * interrupt) from overloading the PF driver.
 */
#define DLB2_ISR_OVERLOAD_THRESH 1000
#define DLB2_ISR_OVERLOAD_PERIOD_S 1

struct dlb2_alarm {
	u32 count;
	ktime_t ts;
	unsigned int enabled;
};

struct dlb2_dev {
	int id;
	/* (VF only) */
	struct vf_id_state vf_id_state;
	/* (PF only) */
	struct vf_id_state child_id_state[DLB2_MAX_NUM_VDEVS];
	struct pci_dev *pdev;
	struct dlb2_hw hw;
	struct cdev cdev;
	enum dlb2_device_type type;
	struct dlb2_device_ops *ops;
	dev_t dev_number;
	struct list_head list;
	struct device *dlb2_device;
	struct dlb2_status *status;
	struct dlb2_domain_dev sched_domains[DLB2_MAX_NUM_DOMAINS];
	struct dlb2_port_memory ldb_port_mem[DLB2_MAX_NUM_LDB_PORTS];
	struct dlb2_port_memory dir_port_mem[DLB2_MAX_NUM_DIR_PORTS];
	struct list_head vma_list;
	/* Number of threads currently executing a file read, mmap, or ioctl.
	 * Protected by the resource_mutex.
	 */
	unsigned long active_users;
#ifdef CONFIG_INTEL_DLB2_DATAPATH
	struct dlb2_datapath dp;
#endif
	/* The enqueue_four function enqueues four HCWs (one cache-line worth)
	 * to the DLB, using whichever mechanism is supported by the platform
	 * on which this driver is running.
	 */
	void (*enqueue_four)(void *qe4, void __iomem *pp_addr);
	struct dlb2_intr intr;
	u8 domain_reset_failed;
	u8 reset_active;
	/* The resource mutex serializes access to driver data structures and
	 * hardware registers.
	 */
	struct mutex resource_mutex;
	/* There are two entry points to the service ISR: an interrupt, and an
	 * S-IOV software mailbox request. This mutex ensures only one thread
	 * executes the ISR at any time.
	 */
	struct mutex svc_isr_mutex;
	/* This workqueue thread is responsible for processing all CQ->QID unmap
	 * requests.
	 */
	struct workqueue_struct *wq;
	struct work_struct work;
	u8 worker_launched;
	/* (PF only) */
	u8 vf_registered[DLB2_MAX_NUM_VDEVS];
	struct dlb2_alarm ingress_err;
	struct dlb2_alarm mbox[DLB2_MAX_NUM_VDEVS];
	struct list_head vdev_list;
};

int dlb2_add_domain_device_file(struct dlb2_dev *dlb2_dev, u32 domain_id);
int dlb2_close_domain_device_file(struct dlb2_dev *dlb2_dev,
				  struct dlb2_status *status,
				  u32 domain_id,
				  bool skip_reset);
int dlb2_read_domain_alert(struct dlb2_dev *dev,
			   struct dlb2_status *status,
			   int domain_id,
			   struct dlb2_domain_alert *alert,
			   bool block);

struct dlb2_dp;
void dlb2_register_dp_handle(struct dlb2_dp *dp);
void dlb2_unregister_dp_handle(struct dlb2_dp *dp);

extern bool dlb2_pasid_override;
extern bool dlb2_wdto_disable;

/* Return the number of registered VFs */
static inline bool dlb2_is_registered_vf(struct dlb2_dev *dlb2_dev,
					 int vf_id)
{
	return dlb2_dev->vf_registered[vf_id];
}

/* Return the number of registered VFs */
static inline int dlb2_num_vfs_registered(struct dlb2_dev *dlb2_dev)
{
	int i, cnt = 0;

	for (i = 0; i < DLB2_MAX_NUM_VDEVS; i++)
		cnt += !!dlb2_is_registered_vf(dlb2_dev, i);

	return cnt;
}

int dlb2_host_vdevs_in_use(struct dlb2_dev *dev);

bool dlb2_in_use(struct dlb2_dev *dev);

void dlb2_stop_users(struct dlb2_dev *dlb2_dev);

void dlb2_zap_vma_entries(struct dlb2_dev *dlb2_dev);

void dlb2_release_device_memory(struct dlb2_dev *dev);

void dlb2_handle_mbox_interrupt(struct dlb2_dev *dev, int id);

#endif /* __DLB2_MAIN_H */
