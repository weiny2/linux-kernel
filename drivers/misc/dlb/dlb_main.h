/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright(c) 2017-2020 Intel Corporation
 */

#ifndef __DLB_MAIN_H
#define __DLB_MAIN_H

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/types.h>

#include <uapi/linux/dlb_user.h>

#include "dlb_hw_types.h"

static const char dlb_driver_name[] = KBUILD_MODNAME;

#define DLB_NUM_FUNCS_PER_DEVICE (1 + DLB_MAX_NUM_VFS)
#define DLB_MAX_NUM_DEVICES (DLB_NUM_FUNCS_PER_DEVICE)
#define DLB_NUM_DEV_FILES_PER_DEVICE (DLB_MAX_NUM_DOMAINS + 1)
#define DLB_MAX_NUM_DEVICE_FILES (DLB_MAX_NUM_DEVICES * \
				  DLB_NUM_DEV_FILES_PER_DEVICE)
/* This macro returns an ID from 0 to DLB_NUM_DEV_FILES_PER_DEVICE, inclusive */
#define DLB_FILE_ID_FROM_DEV_T(base, file) ((MINOR(file) - MINOR(base)) % \
					    DLB_NUM_DEV_FILES_PER_DEVICE)
/* This macro returns an ID from 0 to DLB_MAX_NUM_DEVICES, inclusive */
#define DLB_DEV_ID_FROM_DEV_T(base, file) ((MINOR(file) - MINOR(base)) / \
					   DLB_NUM_DEV_FILES_PER_DEVICE)
#define IS_DLB_DEV_FILE(base, file) (DLB_FILE_ID_FROM_DEV_T(base, file) == \
				     DLB_MAX_NUM_DOMAINS)

#define DLB_DEFAULT_RESET_TIMEOUT_S 5
#define DLB_VF_FLR_DONE_POLL_TIMEOUT_MS 1000
#define DLB_VF_FLR_DONE_SLEEP_PERIOD_MS 1

extern struct list_head dlb_dev_list;
extern struct mutex dlb_driver_lock;
extern unsigned int dlb_qe_sa_pct;
extern unsigned int dlb_qid_sa_pct;

enum dlb_device_type {
	DLB_PF,
	DLB_VF,
};

struct dlb_dev;

struct dlb_device_ops {
	int (*map_pci_bar_space)(struct dlb_dev *dev, struct pci_dev *pdev);
	void (*unmap_pci_bar_space)(struct dlb_dev *dev, struct pci_dev *pdev);
	int (*mmap)(struct file *f, struct vm_area_struct *vma, u32 id);
	void (*inc_pm_refcnt)(struct pci_dev *pdev, bool resume);
	void (*dec_pm_refcnt)(struct pci_dev *pdev);
	int (*init_driver_state)(struct dlb_dev *dev);
	void (*free_driver_state)(struct dlb_dev *dev);
	int (*device_create)(struct dlb_dev *dlb_dev,
			     struct pci_dev *pdev,
			     struct class *dlb_class);
	void (*device_destroy)(struct dlb_dev *dlb_dev,
			       struct class *dlb_class);
	int (*cdev_add)(struct dlb_dev *dlb_dev,
			dev_t base,
			const struct file_operations *fops);
	void (*cdev_del)(struct dlb_dev *dlb_dev);
	int (*sysfs_create)(struct dlb_dev *dlb_dev);
	void (*sysfs_destroy)(struct dlb_dev *dlb_dev);
	void (*sysfs_reapply)(struct dlb_dev *dev);
	int (*init_interrupts)(struct dlb_dev *dev, struct pci_dev *pdev);
	int (*enable_ldb_cq_interrupts)(struct dlb_dev *dev,
					int port_id,
					u16 thresh);
	int (*enable_dir_cq_interrupts)(struct dlb_dev *dev,
					int port_id,
					u16 thresh);
	int (*arm_cq_interrupt)(struct dlb_dev *dev,
				int domain_id,
				int port_id,
				bool is_ldb);
	void (*reinit_interrupts)(struct dlb_dev *dev);
	void (*free_interrupts)(struct dlb_dev *dev, struct pci_dev *pdev);
	void (*init_hardware)(struct dlb_dev *dev);
	int (*register_driver)(struct dlb_dev *dev);
	void (*unregister_driver)(struct dlb_dev *dev);
	int (*create_sched_domain)(struct dlb_hw *hw,
				   struct dlb_create_sched_domain_args *args,
				   struct dlb_cmd_response *resp);
	int (*create_ldb_pool)(struct dlb_hw *hw,
			       u32 domain_id,
			       struct dlb_create_ldb_pool_args *args,
			       struct dlb_cmd_response *resp);
	int (*create_dir_pool)(struct dlb_hw *hw,
			       u32 domain_id,
			       struct dlb_create_dir_pool_args *args,
			       struct dlb_cmd_response *resp);
	int (*create_ldb_queue)(struct dlb_hw *hw,
				u32 domain_id,
				struct dlb_create_ldb_queue_args *args,
				struct dlb_cmd_response *resp);
	int (*create_dir_queue)(struct dlb_hw *hw,
				u32 domain_id,
				struct dlb_create_dir_queue_args *args,
				struct dlb_cmd_response *resp);
	int (*create_ldb_port)(struct dlb_hw *hw,
			       u32 domain_id,
			       struct dlb_create_ldb_port_args *args,
			       uintptr_t pop_count_dma_base,
			       uintptr_t cq_dma_base,
			       struct dlb_cmd_response *resp);
	int (*create_dir_port)(struct dlb_hw *hw,
			       u32 domain_id,
			       struct dlb_create_dir_port_args *args,
			       uintptr_t pop_count_dma_base,
			       uintptr_t cq_dma_base,
			       struct dlb_cmd_response *resp);
	int (*start_domain)(struct dlb_hw *hw,
			    u32 domain_id,
			    struct dlb_start_domain_args *args,
			    struct dlb_cmd_response *resp);
	int (*map_qid)(struct dlb_hw *hw,
		       u32 domain_id,
		       struct dlb_map_qid_args *args,
		       struct dlb_cmd_response *resp);
	int (*unmap_qid)(struct dlb_hw *hw,
			 u32 domain_id,
			 struct dlb_unmap_qid_args *args,
			 struct dlb_cmd_response *resp);
	int (*pending_port_unmaps)(struct dlb_hw *hw,
				   u32 domain_id,
				   struct dlb_pending_port_unmaps_args *args,
				   struct dlb_cmd_response *resp);
	int (*enable_ldb_port)(struct dlb_hw *hw,
			       u32 domain_id,
			       struct dlb_enable_ldb_port_args *args,
			       struct dlb_cmd_response *resp);
	int (*disable_ldb_port)(struct dlb_hw *hw,
				u32 domain_id,
				struct dlb_disable_ldb_port_args *args,
				struct dlb_cmd_response *resp);
	int (*enable_dir_port)(struct dlb_hw *hw,
			       u32 domain_id,
			       struct dlb_enable_dir_port_args *args,
			       struct dlb_cmd_response *resp);
	int (*disable_dir_port)(struct dlb_hw *hw,
				u32 domain_id,
				struct dlb_disable_dir_port_args *args,
				struct dlb_cmd_response *resp);
	int (*get_num_resources)(struct dlb_hw *hw,
				 struct dlb_get_num_resources_args *args);
	int (*reset_domain)(struct dlb_dev *dev, u32 domain_id);
	int (*ldb_port_owned_by_domain)(struct dlb_hw *hw,
					u32 domain_id,
					u32 port_id);
	int (*dir_port_owned_by_domain)(struct dlb_hw *hw,
					u32 domain_id,
					u32 port_id);
	int (*get_sn_allocation)(struct dlb_hw *hw, u32 group_id);
	int (*set_sn_allocation)(struct dlb_hw *hw, u32 group_id, u32 num);
	int (*get_sn_occupancy)(struct dlb_hw *hw, u32 group_id);
	int (*get_ldb_queue_depth)(struct dlb_hw *hw,
				   u32 domain_id,
				   struct dlb_get_ldb_queue_depth_args *args,
				   struct dlb_cmd_response *resp);
	int (*get_dir_queue_depth)(struct dlb_hw *hw,
				   u32 domain_id,
				   struct dlb_get_dir_queue_depth_args *args,
				   struct dlb_cmd_response *resp);
	int (*query_cq_poll_mode)(struct dlb_dev *dev,
				  struct dlb_cmd_response *user_resp);
};

extern const struct attribute_group *dlb_vf_attrs[];
extern struct dlb_device_ops dlb_pf_ops;
extern struct dlb_device_ops dlb_vf_ops;

void dlb_vf_ack_vf_flr_done(struct dlb_hw *hw);

struct dlb_port_memory {
	void *cq_base;
	dma_addr_t cq_dma_base;
	void *pc_base;
	dma_addr_t pc_dma_base;
	int domain_id;
	u8 valid;
};

struct dlb_status {
	u8 valid;
	u32 refcnt;
};

struct dlb_cq_intr {
	u8 wake;
	u8 configured;
	/* vector is in the range [0,63] */
	u8 vector;
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

struct dlb_vf_cq_int_info {
	char name[32];
	u8 is_ldb;
	s16 port_id;
};

struct dlb_intr {
	int num_vectors;
	int base_vector;
	int mode;
	u64 packed_vector_bitmap;
	/* The PF has more interrupt vectors than the VF, so we
	 * simply over-allocate in the case of the VF driver
	 */
	u8 isr_registered[DLB_PF_TOTAL_NUM_INTERRUPT_VECTORS];
	struct dlb_cq_intr ldb_cq_intr[DLB_MAX_NUM_LDB_PORTS];
	struct dlb_cq_intr dir_cq_intr[DLB_MAX_NUM_DIR_PORTS];
	int num_ldb_ports;
	int num_dir_ports;

	/* The following state is VF only */
	u8 num_cq_interrupts;
	struct dlb_vf_cq_int_info msi_map[DLB_VF_TOTAL_NUM_INTERRUPT_VECTORS];
	struct dlb_dev *ldb_cq_intr_owner[DLB_MAX_NUM_LDB_PORTS];
	struct dlb_dev *dir_cq_intr_owner[DLB_MAX_NUM_DIR_PORTS];
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
	/* If this is an auxiliary VF, primary_vf points to the dlb_dev
	 * structure of its primary sibling.
	 */
	struct dlb_dev *primary_vf;
};

#define DLB_DOMAIN_ALERT_RING_SIZE 256

struct dlb_domain_dev {
	struct dlb_status *status;
	struct dlb_domain_alert alerts[DLB_DOMAIN_ALERT_RING_SIZE];
	u8 alert_rd_idx;
	u8 alert_wr_idx;
	/* The alert mutex protects access to the alert ring and its read and
	 * write indexes.
	 */
	struct mutex alert_mutex;
	wait_queue_head_t wq_head;
};

struct dlb_vma_node {
	struct list_head list;
	struct vm_area_struct *vma;
};

/* ISR overload is defined as more than DLB_ISR_OVERLOAD_THRESH interrupts (of
 * a particular type) occurring in a 1s period. If overload is detected, the
 * driver blocks that interrupt (exact mechanism depending on the interrupt)
 * from overloading the PF driver.
 */
#define DLB_ISR_OVERLOAD_THRESH 1000
#define DLB_ISR_OVERLOAD_PERIOD_S 1

struct dlb_alarm {
	u32 count;
	ktime_t ts;
	unsigned int enabled;
};

struct dlb_dev {
	int id;
	struct vf_id_state vf_id_state; /* (VF only) */
	struct vf_id_state child_id_state[DLB_MAX_NUM_VFS]; /* (PF only) */
	struct pci_dev *pdev;
	struct dlb_hw hw;
	struct cdev cdev;
	enum dlb_device_type type;
	u8 revision;
	struct dlb_device_ops *ops;
	dev_t dev_number;
	struct list_head list;
	struct device *dlb_device;
	struct dlb_status *status;
	struct dlb_domain_dev sched_domains[DLB_MAX_NUM_DOMAINS];
	struct dlb_port_memory ldb_port_mem[DLB_MAX_NUM_LDB_PORTS];
	struct dlb_port_memory dir_port_mem[DLB_MAX_NUM_DIR_PORTS];
	struct list_head vma_list;
	/* Number of threads currently executing a file read, mmap, or ioctl.
	 * Protected by the resource_mutex.
	 */
	unsigned long active_users;
	struct dlb_intr intr;
	u8 domain_reset_failed;
	u8 reset_active;
	/* The resource mutex serializes access to driver data structures and
	 * hardware registers.
	 */
	struct mutex resource_mutex;
	/* This workqueue thread is responsible for processing all CQ->QID unmap
	 * requests.
	 */
	struct workqueue_struct *wq;
	struct work_struct work;
	u8 worker_launched;
	/* (PF only) */
	u8 vf_registered[DLB_MAX_NUM_VFS];
	struct dlb_alarm ingress_err;
	struct dlb_alarm mbox[DLB_MAX_NUM_VFS];
};

int dlb_add_domain_device_file(struct dlb_dev *dlb_dev, u32 domain_id);

#define DLB_HW_ERR(hw, ...) do {		    \
	struct dlb_dev *dev;			    \
	dev = container_of(hw, struct dlb_dev, hw); \
	dev_err(dev->dlb_device, __VA_ARGS__);	    \
} while (0)

#define DLB_HW_DBG(hw, ...) do {		    \
	struct dlb_dev *dev;			    \
	dev = container_of(hw, struct dlb_dev, hw); \
	dev_dbg(dev->dlb_device, __VA_ARGS__);	    \
} while (0)

/* Return the number of registered VFs */
static inline bool dlb_is_registered_vf(struct dlb_dev *dlb_dev, int vf_id)
{
	return dlb_dev->vf_registered[vf_id];
}

/* Return the number of registered VFs */
static inline int dlb_num_vfs_registered(struct dlb_dev *dlb_dev)
{
	int i, cnt = 0;

	for (i = 0; i < DLB_MAX_NUM_VFS; i++)
		cnt += !!dlb_is_registered_vf(dlb_dev, i);

	return cnt;
}

int dlb_write_domain_alert(struct dlb_dev *dev,
			   u32 domain_id,
			   u64 alert_id,
			   u64 aux_alert_data);

bool dlb_in_use(struct dlb_dev *dev);
void dlb_stop_users(struct dlb_dev *dlb_dev);
void dlb_zap_vma_entries(struct dlb_dev *dlb_dev);

#endif /* __DLB_MAIN_H */
