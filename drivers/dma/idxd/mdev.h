/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2019 Intel Corporation. All rights rsvd. */

#ifndef _IDXD_MDEV_H_
#define _IDXD_MDEV_H_

/* two 64-bit BARs implemented */
#define VIDXD_MAX_BARS 2
#define VIDXD_MAX_CFG_SPACE_SZ 4096
#define VIDXD_MSIX_TBL_SZ_OFFSET 0x42
#define VIDXD_CAP_CTRL_SZ 0x100
#define VIDXD_GRP_CTRL_SZ 0x100
#define VIDXD_WQ_CTRL_SZ 0x100
#define VIDXD_WQ_OCPY_INT_SZ 0x20
#define VIDXD_MSIX_TBL_SZ 0x90
#define VIDXD_MSIX_PERM_TBL_SZ 0x48

#define VIDXD_MSIX_TABLE_OFFSET 0x600
#define VIDXD_MSIX_PERM_OFFSET 0x300
#define VIDXD_GRPCFG_OFFSET 0x400
#define VIDXD_WQCFG_OFFSET 0x500
#define VIDXD_IMS_OFFSET 0x1000

#define VIDXD_BAR0_SIZE  0x2000
#define VIDXD_BAR2_SIZE  0x20000
#define VIDXD_MAX_MSIX_ENTRIES  (VIDXD_MSIX_TBL_SZ / 0x10)
#define VIDXD_MAX_WQS	1

#define	VIDXD_ATS_OFFSET 0x100
#define	VIDXD_PRS_OFFSET 0x110
#define VIDXD_PASID_OFFSET 0x120
#define VIDXD_MSIX_PBA_OFFSET 0x700

struct vdcm_idxd_pci_bar0 {
	u8 cap_ctrl_regs[VIDXD_CAP_CTRL_SZ];
	u8 grp_ctrl_regs[VIDXD_GRP_CTRL_SZ];
	u8 wq_ctrl_regs[VIDXD_WQ_CTRL_SZ];
	u8 wq_ocpy_int_regs[VIDXD_WQ_OCPY_INT_SZ];
	u8 msix_table[VIDXD_MSIX_TBL_SZ];
	u8 msix_perm_table[VIDXD_MSIX_PERM_TBL_SZ];
	unsigned long msix_pba;
};

struct ims_irq_entry {
	struct vdcm_idxd *vidxd;
	int int_src;
};

struct idxd_vdev {
	struct mdev_device *mdev;
	struct vfio_region *region;
	int num_regions;
	struct eventfd_ctx *msix_trigger[VIDXD_MAX_MSIX_ENTRIES];
	struct notifier_block group_notifier;
	struct kvm *kvm;
	struct work_struct release_work;
	atomic_t released;
};

struct vdcm_idxd {
	struct idxd_device *idxd;
	struct idxd_wq *wq;
	struct idxd_vdev vdev;
	struct vdcm_idxd_type *type;
	int num_wqs;
	unsigned long handle;
	u64 ims_index[VIDXD_MAX_WQS];
	struct msix_entry ims_entry;
	struct ims_irq_entry irq_entries[VIDXD_MAX_WQS];

	/* For VM use case */
	u64 bar_val[VIDXD_MAX_BARS];
	u64 bar_size[VIDXD_MAX_BARS];
	u8 cfg[VIDXD_MAX_CFG_SPACE_SZ];
	struct vdcm_idxd_pci_bar0 bar0;
	struct list_head list;
};

static inline struct vdcm_idxd *to_vidxd(struct idxd_vdev *vdev)
{
	return container_of(vdev, struct vdcm_idxd, vdev);
}

#define IDXD_MDEV_NAME_LEN 16
#define IDXD_MDEV_DESCRIPTION_LEN 64

enum idxd_mdev_type {
	IDXD_MDEV_TYPE_WQ = 0,
};

#define IDXD_MDEV_TYPES 1

struct vdcm_idxd_type {
	char name[IDXD_MDEV_NAME_LEN];
	char description[IDXD_MDEV_DESCRIPTION_LEN];
	enum idxd_mdev_type type;
	unsigned int avail_instance;
};

enum idxd_vdcm_rw {
	IDXD_VDCM_READ = 0,
	IDXD_VDCM_WRITE,
};

#endif
