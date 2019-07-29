/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2018 Intel Corporation
 */

#ifndef __HQMV2_MAIN_H
#define __HQMV2_MAIN_H

#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/iova.h>
#include <linux/pci.h>
#include <linux/time.h>
#include <linux/wait.h>

static const char hqmv2_driver_name[] = KBUILD_MODNAME;

#include "base/hqmv2_hw_types.h"
#include <uapi/linux/hqmv2_user.h>

#define HQMV2_NUM_FUNCS_PER_DEVICE (1 + HQMV2_MAX_NUM_VDEVS)
#define HQMV2_MAX_NUM_DEVICES (HQMV2_NUM_FUNCS_PER_DEVICE)
#define HQMV2_NUM_DEV_FILES_PER_DEVICE (HQMV2_MAX_NUM_DOMAINS + 1)
#define HQMV2_MAX_NUM_DEVICE_FILES (HQMV2_MAX_NUM_DEVICES * \
				  HQMV2_NUM_DEV_FILES_PER_DEVICE)
/* This macro returns an ID from 0 to HQMV2_NUM_DEV_FILES_PER_DEVICE,
 * inclusive.
 */
#define HQMV2_FILE_ID_FROM_DEV_T(base, file) ((MINOR(file) - MINOR(base)) % \
					    HQMV2_NUM_DEV_FILES_PER_DEVICE)
/* This macro returns an ID from 0 to HQMV2_MAX_NUM_DEVICES, inclusive */
#define HQMV2_DEV_ID_FROM_DEV_T(base, file) ((MINOR(file) - MINOR(base)) / \
					   HQMV2_NUM_DEV_FILES_PER_DEVICE)
#define IS_HQMV2_DEV_FILE(base, file) (HQMV2_FILE_ID_FROM_DEV_T(base, file) == \
				     HQMV2_MAX_NUM_DOMAINS)

#define HQMV2_DEFAULT_RESET_TIMEOUT_S 5
#define HQMV2_VF_FLR_DONE_POLL_TIMEOUT_MS 1000
#define HQMV2_VF_FLR_DONE_SLEEP_PERIOD_MS 1

extern struct list_head hqmv2_dev_list;
extern struct mutex driver_lock;

enum hqmv2_device_type {
	HQMV2_PF,
	HQMV2_VF,
};

struct hqmv2_dev;

struct hqmv2_device_ops {
	/* Device create? (maybe) */
	int (*map_pci_bar_space)(struct hqmv2_dev *dev, struct pci_dev *pdev);
	void (*unmap_pci_bar_space)(struct hqmv2_dev *dev,
				    struct pci_dev *pdev);
	int (*mmap)(struct file *f, struct vm_area_struct *vma, u32 id);
	void (*inc_pm_refcnt)(struct pci_dev *pdev, bool resume);
	void (*dec_pm_refcnt)(struct pci_dev *pdev);
	bool (*pm_refcnt_status_suspended)(struct pci_dev *pdev);
	int (*init_driver_state)(struct hqmv2_dev *dev);
	void (*free_driver_state)(struct hqmv2_dev *dev);
	int (*device_create)(struct hqmv2_dev *hqmv2_dev,
			     struct pci_dev *pdev,
			     struct class *ws_class);
	void (*device_destroy)(struct hqmv2_dev *hqmv2_dev,
			       struct class *ws_class);
	int (*cdev_add)(struct hqmv2_dev *hqmv2_dev,
			dev_t base,
			const struct file_operations *fops);
	void (*cdev_del)(struct hqmv2_dev *hqmv2_dev);
	int (*sysfs_create)(struct hqmv2_dev *hqmv2_dev);
	void (*sysfs_destroy)(struct hqmv2_dev *hqmv2_dev);
	void (*sysfs_reapply)(struct hqmv2_dev *dev);
	int (*init_interrupts)(struct hqmv2_dev *dev, struct pci_dev *pdev);
	int (*enable_ldb_cq_interrupts)(struct hqmv2_dev *dev,
					int port_id,
					u16 thresh);
	int (*enable_dir_cq_interrupts)(struct hqmv2_dev *dev,
					int port_id,
					u16 thresh);
	int (*arm_cq_interrupt)(struct hqmv2_dev *dev,
				int domain_id,
				int port_id,
				bool is_ldb);
	void (*reinit_interrupts)(struct hqmv2_dev *dev);
	void (*free_interrupts)(struct hqmv2_dev *dev, struct pci_dev *pdev);
	void (*enable_pm)(struct hqmv2_dev *dev);
	int (*wait_for_device_ready)(struct hqmv2_dev *dev,
				     struct pci_dev *pdev);
	int (*reset_device)(struct pci_dev *pdev);
	int (*register_driver)(struct hqmv2_dev *dev);
	void (*unregister_driver)(struct hqmv2_dev *dev);
	int (*create_sched_domain)(struct hqmv2_hw *hw,
				   struct hqmv2_create_sched_domain_args *args,
				   struct hqmv2_cmd_response *resp);
	int (*create_ldb_queue)(struct hqmv2_hw *hw,
				u32 domain_id,
				struct hqmv2_create_ldb_queue_args *args,
				struct hqmv2_cmd_response *resp);
	int (*create_dir_queue)(struct hqmv2_hw *hw,
				u32 domain_id,
				struct hqmv2_create_dir_queue_args *args,
				struct hqmv2_cmd_response *resp);
	int (*create_ldb_port)(struct hqmv2_hw *hw,
			       u32 domain_id,
			       struct hqmv2_create_ldb_port_args *args,
			       uintptr_t pop_count_dma_base,
			       uintptr_t cq_dma_base,
			       struct hqmv2_cmd_response *resp);
	int (*create_dir_port)(struct hqmv2_hw *hw,
			       u32 domain_id,
			       struct hqmv2_create_dir_port_args *args,
			       uintptr_t pop_count_dma_base,
			       uintptr_t cq_dma_base,
			       struct hqmv2_cmd_response *resp);
	int (*start_domain)(struct hqmv2_hw *hw,
			    u32 domain_id,
			    struct hqmv2_start_domain_args *args,
			    struct hqmv2_cmd_response *resp);
	int (*map_qid)(struct hqmv2_hw *hw,
		       u32 domain_id,
		       struct hqmv2_map_qid_args *args,
		       struct hqmv2_cmd_response *resp);
	int (*unmap_qid)(struct hqmv2_hw *hw,
			 u32 domain_id,
			 struct hqmv2_unmap_qid_args *args,
			 struct hqmv2_cmd_response *resp);
	int (*enable_ldb_port)(struct hqmv2_hw *hw,
			       u32 domain_id,
			       struct hqmv2_enable_ldb_port_args *args,
			       struct hqmv2_cmd_response *resp);
	int (*disable_ldb_port)(struct hqmv2_hw *hw,
				u32 domain_id,
				struct hqmv2_disable_ldb_port_args *args,
				struct hqmv2_cmd_response *resp);
	int (*enable_dir_port)(struct hqmv2_hw *hw,
			       u32 domain_id,
			       struct hqmv2_enable_dir_port_args *args,
			       struct hqmv2_cmd_response *resp);
	int (*disable_dir_port)(struct hqmv2_hw *hw,
				u32 domain_id,
				struct hqmv2_disable_dir_port_args *args,
				struct hqmv2_cmd_response *resp);
	int (*get_num_resources)(struct hqmv2_hw *hw,
				 struct hqmv2_get_num_resources_args *args);
	int (*reset_domain)(struct hqmv2_hw *hw, u32 domain_id);
	int (*measure_perf)(struct hqmv2_dev *dev,
			    struct hqmv2_sample_perf_counters_args *args,
			    struct hqmv2_cmd_response *response);
	int (*ldb_port_owned_by_domain)(struct hqmv2_hw *hw,
					u32 domain_id,
					u32 port_id);
	int (*dir_port_owned_by_domain)(struct hqmv2_hw *hw,
					u32 domain_id,
					u32 port_id);
	int (*get_sn_allocation)(struct hqmv2_hw *hw, u32 group_id);
	int (*get_ldb_queue_depth)(struct hqmv2_hw *hw,
				   u32 domain_id,
				   struct hqmv2_get_ldb_queue_depth_args *args,
				   struct hqmv2_cmd_response *resp);
	int (*get_dir_queue_depth)(struct hqmv2_hw *hw,
				   u32 domain_id,
				   struct hqmv2_get_dir_queue_depth_args *args,
				   struct hqmv2_cmd_response *resp);
	int (*pending_port_unmaps)(struct hqmv2_hw *hw,
				   u32 domain_id,
				   struct hqmv2_pending_port_unmaps_args *args,
				   struct hqmv2_cmd_response *resp);
	int (*get_cos_bw)(struct hqmv2_hw *hw, u32 cos_id);
};

extern const struct attribute_group *hqmv2_vf_attrs[];
extern struct hqmv2_device_ops hqmv2_pf_ops;
extern struct hqmv2_device_ops hqmv2_vf_ops;

struct hqmv2_port_memory {
	void *cq_base;
	dma_addr_t cq_dma_base;
	void *pc_base;
	dma_addr_t pc_dma_base;
	int domain_id;
	u8 valid;
};

#define HQMV2_DOMAIN_ALERT_RING_SIZE 256

struct hqmv2_domain_dev {
	u32 file_refcnt;
	u8 user_mode;
#ifdef CONFIG_INTEL_HQMV2_DATAPATH
	struct hqmv2_dp_domain *dp;
#endif
	struct hqmv2_domain_alert alerts[HQMV2_DOMAIN_ALERT_RING_SIZE];
	u8 alert_rd_idx;
	u8 alert_wr_idx;
	/* The alert mutex protects access to the alert ring and its read and
	 * write indexes.
	 */
	struct mutex alert_mutex;
	wait_queue_head_t wq_head;
};

struct hqmv2_cq_intr {
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

struct hqmv2_vdev_cq_int_info {
	char name[32];
	u8 is_ldb;
	s16 port_id;
};

struct hqmv2_intr {
	int num_vectors;
	int base_vector;
	int mode;
	/* The VDEV has more possible interrupt vectors than the PF or VF, so
	 * we simply over-allocate in those cases
	 */
	u8 isr_registered[HQMV2_VDEV_MAX_NUM_INTERRUPT_VECTORS];
	struct hqmv2_cq_intr ldb_cq_intr[HQMV2_MAX_NUM_LDB_PORTS];
	struct hqmv2_cq_intr dir_cq_intr[HQMV2_MAX_NUM_DIR_PORTS];
	int num_ldb_ports;
	int num_dir_ports;

	/* The following state is VF/VDEV only */
	u8 num_cq_intrs;
	struct hqmv2_vdev_cq_int_info
		msi_map[HQMV2_VDEV_MAX_NUM_INTERRUPT_VECTORS];
	struct hqmv2_dev *ldb_cq_intr_owner[HQMV2_MAX_NUM_LDB_PORTS];
	struct hqmv2_dev *dir_cq_intr_owner[HQMV2_MAX_NUM_DIR_PORTS];
};

struct hqmv2_datapath {
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
	/* If this is an auxiliary VF, primary_vf points to the hqmv2_dev
	 * structure of its primary sibling.
	 */
	struct hqmv2_dev *primary_vf;
};

struct hqmv2_vf_perf_metric_data {
	u64 ldb_cq[HQMV2_MAX_NUM_LDB_PORTS];
	u64 dir_cq[HQMV2_MAX_NUM_DIR_PORTS];
	u32 elapsed;
	u8 valid;
};

struct hqmv2_perf_counter_state {
	/* The measurement mutex serializes access to performance monitoring
	 * hardware.
	 */
	struct mutex measurement_mutex;

	/* Current operation */
	struct timespec64 start;
	u32 measurement_duration_us;
	struct hqmv2_perf_metric_pre_data *pre;
	int vf_id;

	/* Data from completed VF requests */
	struct hqmv2_vf_perf_metric_data data[HQMV2_MAX_NUM_VDEVS];
};

struct hqmv2_dev {
	int id;
	/* (VF only) */
	struct vf_id_state vf_id_state;
	/* (PF only) */
	struct vf_id_state child_id_state[HQMV2_MAX_NUM_VDEVS];
	struct pci_dev *pdev;
	struct hqmv2_hw hw;
	struct cdev cdev;
	enum hqmv2_device_type type;
	struct hqmv2_device_ops *ops;
	dev_t dev_number;
	struct list_head list;
	struct device *hqmv2_device;
	struct hqmv2_domain_dev sched_domains[HQMV2_MAX_NUM_DOMAINS];
	struct hqmv2_port_memory ldb_port_pages[HQMV2_MAX_NUM_LDB_PORTS];
	struct hqmv2_port_memory dir_port_pages[HQMV2_MAX_NUM_DIR_PORTS];
#ifdef CONFIG_INTEL_HQMV2_DATAPATH
	struct hqmv2_datapath dp;
#endif
	/* The enqueue_four function enqueues four HCWs (one cache-line worth)
	 * to the HQM, using whichever mechanism is supported by the platform
	 * on which this driver is running.
	 */
	void (*enqueue_four)(void *qe4, void *pp_addr);
	struct hqmv2_intr intr;
	u8 domain_reset_failed;
	u8 unexpected_reset;
	u8 vf_waiting_for_idle; /* (VF only) */
	/* The resource mutex serializes access to driver data structures and
	 * hardware registers.
	 */
	struct mutex resource_mutex;
	struct hqmv2_perf_counter_state perf;
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
	u8 vf_registered[HQMV2_MAX_NUM_VDEVS];
};

int hqmv2_add_domain_device_file(struct hqmv2_dev *hqmv2_dev, u32 domain_id);
int hqmv2_close_domain_device_file(struct hqmv2_dev *hqmv2_dev,
				   u32 domain_id,
				   bool skip_reset);
int hqmv2_read_domain_alert(struct hqmv2_dev *dev,
			    int domain_id,
			    struct hqmv2_domain_alert *alert,
			    bool block);

struct hqmv2_dp;
void hqmv2_register_dp_handle(struct hqmv2_dp *dp);
void hqmv2_unregister_dp_handle(struct hqmv2_dp *dp);

extern bool hqmv2_pasid_override;

/* Each subsequent log level expands on the previous level. */
#define HQMV2_LOG_LEVEL_NONE 0
#define	HQMV2_LOG_LEVEL_ERR  1
#define HQMV2_LOG_LEVEL_INFO 2

extern unsigned int hqmv2_log_level;

#define HQMV2_ERR(dev, ...) do {			  \
	if (hqmv2_log_level >= HQMV2_LOG_LEVEL_ERR) { \
		dev_info(dev, __VA_ARGS__);	  \
	}					  \
} while (0)

#define HQMV2_INFO(dev, ...) do {			   \
	if (hqmv2_log_level >= HQMV2_LOG_LEVEL_INFO) { \
		dev_info(dev, __VA_ARGS__);	   \
	}					   \
} while (0)

/* Return the number of registered VFs */
static inline bool hqmv2_is_registered_vf(struct hqmv2_dev *hqmv2_dev,
					  int vf_id)
{
	return hqmv2_dev->vf_registered[vf_id];
}

/* Return the number of registered VFs */
static inline int hqmv2_num_vfs_registered(struct hqmv2_dev *hqmv2_dev)
{
	int i, cnt = 0;

	for (i = 0; i < HQMV2_MAX_NUM_VDEVS; i++)
		cnt += !!hqmv2_is_registered_vf(hqmv2_dev, i);

	return cnt;
}

void hqmv2_shutdown_user_threads(struct hqmv2_dev *dev);

int hqmv2_host_vdevs_in_use(struct hqmv2_dev *dev);

bool hqmv2_in_use(struct hqmv2_dev *dev);

void hqmv2_handle_mbox_interrupt(struct hqmv2_dev *dev, int id);

#endif /* __HQMV2_MAIN_H */
