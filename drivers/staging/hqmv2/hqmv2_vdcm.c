// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2018-2019 Intel Corporation */

#ifdef CONFIG_INTEL_HQMV2_SIOV

#include <linux/vfio.h>
#include <linux/mdev.h>
#include <linux/pci-ats.h>
#include <linux/pm_runtime.h>
#include <linux/eventfd.h>
#include <linux/msi.h>
#include <linux/version.h>

#include "base/hqmv2_resource.h"
#include "base/hqmv2_regs.h"
#include "hqmv2_vdcm.h"
#include "hqmv2_main.h"

struct list_head hqmv2_vdevs_list;

/* Helper macros copied from pci/vfio_pci_private.h */
#define VFIO_PCI_OFFSET_SHIFT 40
#define VFIO_PCI_OFFSET_TO_INDEX(off) ((off) >> VFIO_PCI_OFFSET_SHIFT)
#define VFIO_PCI_INDEX_TO_OFFSET(index) ((u64)(index) << VFIO_PCI_OFFSET_SHIFT)
#define VFIO_PCI_OFFSET_MASK (((u64)(1) << VFIO_PCI_OFFSET_SHIFT) - 1)

#define VDCM_MSIX_MSG_CTRL_OFFSET (0x60 + PCI_MSIX_FLAGS)
#define VDCM_MSIX_MAX_ENTRIES 256
/* Note: VDCM_MSIX_TBL_OFFSET must be in sync with hqmv2_pci_config. */
#define VDCM_MSIX_TBL_OFFSET 0x01000000
#define VDCM_MSIX_TBL_ENTRY_SZ 16
/* Note: VDCM_MSIX_TBL_SZ_BYTES must be in sync with hqmv2_pci_config. */
#define VDCM_MSIX_TBL_SZ_BYTES (VDCM_MSIX_TBL_ENTRY_SZ * VDCM_MSIX_MAX_ENTRIES)
#define VDCM_MSIX_TBL_END_OFFSET \
	(VDCM_MSIX_TBL_OFFSET + VDCM_MSIX_TBL_SZ_BYTES - 1)
#define VDCM_MSIX_PBA_OFFSET (VDCM_MSIX_TBL_OFFSET + VDCM_MSIX_TBL_SZ_BYTES)
#define VDCM_MSIX_PBA_SZ_QWORD (VDCM_MSIX_MAX_ENTRIES / 64)
#define VDCM_MSIX_PBA_SZ_BYTES (VDCM_MSIX_MAX_ENTRIES / 8)
#define VDCM_MSIX_PBA_END_OFFSET \
	(VDCM_MSIX_PBA_OFFSET + VDCM_MSIX_PBA_SZ_BYTES - 1)

#define VDCM_PCIE_DEV_CTRL_OFFSET (0x6C + PCI_EXP_DEVCTL)

#define VDCM_MBOX_MSIX_VECTOR 0

#define VDCM_MAX_NUM_IMS_ENTRIES \
	(HQMV2_MAX_NUM_LDB_PORTS + HQMV2_MAX_NUM_DIR_PORTS)

struct hqmv2_ims_irq_entry {
	struct hqmv2_vdev *vdev;
	unsigned int int_src;
	u32 cq_id;
	bool is_ldb;
};

struct hqmv2_vdev {
	struct list_head next;
	bool released;
	unsigned int id;
	struct mdev_device *mdev;
	struct notifier_block iommu_notifier;
	struct notifier_block group_notifier;
	struct work_struct release_work;
	struct eventfd_ctx *msix_eventfd[VDCM_MSIX_MAX_ENTRIES];

	/* IOMMU */
	int pasid;

	/* HQM resources */
	u32 num_ldb_ports;
	u32 num_dir_ports;

	/* Config region */
	u32 num_regions;
	u8 cfg[PCI_CFG_SPACE_SIZE];

	/* Software mailbox */
	u8 *pf_to_vdev_mbox;
	u8 *vdev_to_pf_mbox;

	/* BAR 0 */
	u64 bar0_addr;
	u8 msix_table[VDCM_MSIX_TBL_SZ_BYTES];
	u64 msix_pba[VDCM_MSIX_PBA_SZ_QWORD];

	/* IMS IRQs */
	int group_id;
	struct hqmv2_ims_irq_entry irq_entries[VDCM_MAX_NUM_IMS_ENTRIES];
};

static u64 hqmv2_pci_config[] = {
	0x0010000027118086ULL, /* 0x00-0x40: PCI config header */
	0x000000000b400000ULL,
	0x000000000000000cULL,
	0x0000000000000000ULL,
	0x0000000000000000ULL,
	0x0000808600000000ULL,
	0x0000006000000000ULL,
	0x0000000000000000ULL,
	0x0000000000000000ULL, /* 0x40-0x60: unused */
	0x0000000000000000ULL,
	0x0000000000000000ULL,
	0x0000000000000000ULL,
	0x0100000000406c11ULL, /* 0x60-0x6C: MSI-X Capability */
	0x0002001001001000ULL, /* 0x6C-0xB0: PCIe Capability */
	0x0000291010008062ULL,
	0x1011000000400c11ULL,
	0x0000000000000000ULL,
	0x0000000000000000ULL,
	0x0000000000700010ULL,
	0x0000000000000000ULL,
	0x0000000000000000ULL,
	0x0000000000000000ULL,
};

#define KB 1024
#define MB (1024 * KB)
#define HQMV2_VDEV_BAR0_SIZE (64 * MB)

/**********************************/
/****** Supported type attrs ******/
/**********************************/

static struct attribute *hqmv2_mdev_types_attrs[] = {
	NULL,
};

static struct attribute_group hqmv2_mdev_type_group = {
	.name = "hqm",
	.attrs = hqmv2_mdev_types_attrs,
};

static struct attribute_group *hqmv2_mdev_type_groups[] = {
	&hqmv2_mdev_type_group,
	NULL,
};

/************************/
/****** mdev attrs ******/
/************************/

static inline struct pci_dev *hqmv2_mdev_get_pdev(struct mdev_device *mdev)
{
	struct device *dev = mdev_parent_dev(mdev);

	return container_of(dev, struct pci_dev, dev);
}

static inline struct hqmv2_dev *hqmv2_mdev_get_dev(struct mdev_device *mdev)
{
	return pci_get_drvdata(hqmv2_mdev_get_pdev(mdev));
}

static ssize_t num_sched_domains_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct hqmv2_get_num_resources_args avail, used;
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	int val;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	val = hqmv2_hw_get_num_resources(hw, &avail, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	val = hqmv2_hw_get_num_used_resources(hw, &used, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	val = avail.num_sched_domains + used.num_sched_domains;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t num_sched_domains_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf,
				       size_t count)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	unsigned long num;
	int ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	ret = hqmv2_update_vdev_sched_domains(hw, vdev->id, num);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

static ssize_t num_ldb_queues_show(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct hqmv2_get_num_resources_args avail, used;
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	int val;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	val = hqmv2_hw_get_num_resources(hw, &avail, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	val = hqmv2_hw_get_num_used_resources(hw, &used, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	val = avail.num_ldb_queues + used.num_ldb_queues;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t num_ldb_queues_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf,
				    size_t count)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	unsigned long num;
	int ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	ret = hqmv2_update_vdev_ldb_queues(hw, vdev->id, num);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

static ssize_t num_ldb_ports_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct hqmv2_get_num_resources_args avail, used;
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	int val;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	val = hqmv2_hw_get_num_resources(hw, &avail, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	val = hqmv2_hw_get_num_used_resources(hw, &used, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	val = avail.num_ldb_ports + used.num_ldb_ports;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t num_ldb_ports_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf,
				   size_t count)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	unsigned long num;
	int ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	ret = hqmv2_update_vdev_ldb_ports(hw, vdev->id, num);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

#define HQMV2_COS_LDB_PORTS_SHOW(cos_id)				       \
static ssize_t num_cos##cos_id##_ldb_ports_show(struct device *dev,	       \
						struct device_attribute *attr, \
						char *buf)		       \
{									       \
	struct hqmv2_get_num_resources_args avail, used;		       \
	struct hqmv2_dev *hqmv2_dev;					       \
	struct hqmv2_vdev *vdev;					       \
	struct hqmv2_hw *hw;						       \
	int val;							       \
									       \
	vdev = mdev_get_drvdata(mdev_from_dev(dev));			       \
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));		       \
	hw = &hqmv2_dev->hw;						       \
									       \
	mutex_lock(&hqmv2_dev->resource_mutex);				       \
									       \
	val = hqmv2_hw_get_num_resources(hw, &avail, true, vdev->id);	       \
	if (val) {							       \
		mutex_unlock(&hqmv2_dev->resource_mutex);		       \
		return -1;						       \
	}								       \
									       \
	val = hqmv2_hw_get_num_used_resources(hw, &used, true, vdev->id);      \
	if (val) {							       \
		mutex_unlock(&hqmv2_dev->resource_mutex);		       \
		return -1;						       \
	}								       \
									       \
	mutex_unlock(&hqmv2_dev->resource_mutex);			       \
									       \
	val = avail.num_cos##cos_id##_ldb_ports +			       \
	      used.num_cos##cos_id##_ldb_ports;				       \
									       \
	return scnprintf(buf, PAGE_SIZE, "%d\n", val);			       \
}

HQMV2_COS_LDB_PORTS_SHOW(0)
HQMV2_COS_LDB_PORTS_SHOW(1)
HQMV2_COS_LDB_PORTS_SHOW(2)
HQMV2_COS_LDB_PORTS_SHOW(3)

#define HQMV2_COS_LDB_PORTS_STORE(cos_id)				  \
static ssize_t num_cos##cos_id##_ldb_ports_store(struct device *dev,	  \
					struct device_attribute *attr,	  \
					const char *buf,		  \
					size_t count)			  \
{									  \
	struct hqmv2_dev *hqmv2_dev;					  \
	struct hqmv2_vdev *vdev;					  \
	struct hqmv2_hw *hw;						  \
	unsigned long num;						  \
	int ret;							  \
									  \
	ret = kstrtoul(buf, 0, &num);					  \
	if (ret)							  \
		return -1;						  \
									  \
	vdev = mdev_get_drvdata(mdev_from_dev(dev));			  \
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));		  \
	hw = &hqmv2_dev->hw;						  \
									  \
	mutex_lock(&hqmv2_dev->resource_mutex);				  \
									  \
	ret = hqmv2_update_vdev_ldb_cos_ports(hw, vdev->id, cos_id, num); \
									  \
	mutex_unlock(&hqmv2_dev->resource_mutex);			  \
									  \
	return (ret == 0) ? count : ret;				  \
}

HQMV2_COS_LDB_PORTS_STORE(0)
HQMV2_COS_LDB_PORTS_STORE(1)
HQMV2_COS_LDB_PORTS_STORE(2)
HQMV2_COS_LDB_PORTS_STORE(3)

static ssize_t num_dir_ports_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct hqmv2_get_num_resources_args avail, used;
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	int val;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	val = hqmv2_hw_get_num_resources(hw, &avail, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	val = hqmv2_hw_get_num_used_resources(hw, &used, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	val = avail.num_dir_ports + used.num_dir_ports;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t num_dir_ports_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf,
				   size_t count)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	unsigned long num;
	int ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	ret = hqmv2_update_vdev_dir_ports(hw, vdev->id, num);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

static ssize_t num_ldb_credits_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct hqmv2_get_num_resources_args avail, used;
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	int val;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	val = hqmv2_hw_get_num_resources(hw, &avail, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	val = hqmv2_hw_get_num_used_resources(hw, &used, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	val = avail.num_ldb_credits + used.num_ldb_credits;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t num_ldb_credits_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf,
				     size_t count)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	unsigned long num;
	int ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	ret = hqmv2_update_vdev_ldb_credits(hw, vdev->id, num);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

static ssize_t num_dir_credits_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct hqmv2_get_num_resources_args avail, used;
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	int val;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	val = hqmv2_hw_get_num_resources(hw, &avail, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	val = hqmv2_hw_get_num_used_resources(hw, &used, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	val = avail.num_dir_credits + used.num_dir_credits;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t num_dir_credits_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf,
				     size_t count)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	unsigned long num;
	int ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	ret = hqmv2_update_vdev_dir_credits(hw, vdev->id, num);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

static ssize_t num_hist_list_entries_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	struct hqmv2_get_num_resources_args avail, used;
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	int val;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	val = hqmv2_hw_get_num_resources(hw, &avail, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	val = hqmv2_hw_get_num_used_resources(hw, &used, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	val = avail.num_hist_list_entries + used.num_hist_list_entries;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t num_hist_list_entries_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf,
					   size_t count)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	unsigned long num;
	int ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	ret = hqmv2_update_vdev_hist_list_entries(hw, vdev->id, num);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

static ssize_t num_atomic_inflights_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct hqmv2_get_num_resources_args avail, used;
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	int val;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	val = hqmv2_hw_get_num_resources(hw, &avail, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	val = hqmv2_hw_get_num_used_resources(hw, &used, true, vdev->id);
	if (val) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return -1;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	val = avail.num_atomic_inflights + used.num_atomic_inflights;

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t num_atomic_inflights_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf,
					  size_t count)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct hqmv2_hw *hw;
	unsigned long num;
	int ret;

	ret = kstrtoul(buf, 0, &num);
	if (ret)
		return -1;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));
	hw = &hqmv2_dev->hw;

	mutex_lock(&hqmv2_dev->resource_mutex);

	ret = hqmv2_update_vdev_atomic_inflights(hw, vdev->id, num);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return (ret == 0) ? count : ret;
}

static ssize_t locked_show(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	int val;

	vdev = mdev_get_drvdata(mdev_from_dev(dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(dev));

	val = (int)hqmv2_vdev_is_locked(&hqmv2_dev->hw, vdev->id);

	return scnprintf(buf, PAGE_SIZE, "%d\n", val);
}

static DEVICE_ATTR_RW(num_sched_domains);
static DEVICE_ATTR_RW(num_ldb_queues);
static DEVICE_ATTR_RW(num_ldb_ports);
static DEVICE_ATTR_RW(num_cos0_ldb_ports);
static DEVICE_ATTR_RW(num_cos1_ldb_ports);
static DEVICE_ATTR_RW(num_cos2_ldb_ports);
static DEVICE_ATTR_RW(num_cos3_ldb_ports);
static DEVICE_ATTR_RW(num_dir_ports);
static DEVICE_ATTR_RW(num_ldb_credits);
static DEVICE_ATTR_RW(num_dir_credits);
static DEVICE_ATTR_RW(num_hist_list_entries);
static DEVICE_ATTR_RW(num_atomic_inflights);
static DEVICE_ATTR_RO(locked);

static struct attribute *hqmv2_mdev_attrs[] = {
	&dev_attr_num_sched_domains.attr,
	&dev_attr_num_ldb_queues.attr,
	&dev_attr_num_ldb_ports.attr,
	&dev_attr_num_cos0_ldb_ports.attr,
	&dev_attr_num_cos1_ldb_ports.attr,
	&dev_attr_num_cos2_ldb_ports.attr,
	&dev_attr_num_cos3_ldb_ports.attr,
	&dev_attr_num_dir_ports.attr,
	&dev_attr_num_ldb_credits.attr,
	&dev_attr_num_dir_credits.attr,
	&dev_attr_num_hist_list_entries.attr,
	&dev_attr_num_atomic_inflights.attr,
	&dev_attr_locked.attr,
	NULL,
};

static const struct attribute_group hqmv2_mdev_attr_group = {
	.name = "hqmv2_mdev",
	.attrs = hqmv2_mdev_attrs,
};

const struct attribute_group *hqmv2_mdev_attr_groups[] = {
	&hqmv2_mdev_attr_group,
	NULL,
};

/****************************/
/****** mdev callbacks ******/
/****************************/

static bool id_in_use[HQMV2_MAX_NUM_VDEVS];

static int hqmv2_next_available_vdev_id(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(id_in_use); i++)
		if (!id_in_use[i])
			return i;

	return -1;
}

static void hqmv2_set_vdev_id_in_use(int id, bool in_use)
{
	id_in_use[id] = in_use;
}

static int hqmv2_vdcm_num_cq_irqs(struct hqmv2_vdev *vdev)
{
	/* 1 interrupt per CQ */
	return vdev->num_ldb_ports + vdev->num_dir_ports;
}

static int hqmv2_vdcm_num_irqs(struct hqmv2_vdev *vdev)
{
	/* 1 mailbox interrupt in addition to the CQ interrupts */
	return 1 + hqmv2_vdcm_num_cq_irqs(vdev);
}

static void hqmv2_ims_write_msg(struct msi_desc *desc, struct msi_msg *msg)
{
	struct hqmv2_ims_irq_entry *irq_entry;
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;

	vdev = mdev_get_drvdata(mdev_from_dev(desc->dev));
	hqmv2_dev = hqmv2_mdev_get_dev(mdev_from_dev(desc->dev));

	irq_entry = &vdev->irq_entries[desc->platform.msi_index];

	hqmv2_hw_setup_cq_ims_entry(&hqmv2_dev->hw,
				    vdev->id,
				    irq_entry->cq_id,
				    irq_entry->is_ldb,
				    msg->address_lo,
				    msg->data);
}

static int hqmv2_ims_size(struct device *dev)
{
	return VDCM_MAX_NUM_IMS_ENTRIES;
}

/* Return true if either the vector or the function is masked */
static bool vdcm_msix_is_masked(struct hqmv2_vdev *vdev, int vector)
{
	u8 *entry = &vdev->msix_table[vector * VDCM_MSIX_TBL_ENTRY_SZ];
	u16 msg_ctrl = vdev->cfg[VDCM_MSIX_MSG_CTRL_OFFSET];

	if ((entry[PCI_MSIX_ENTRY_VECTOR_CTRL] & PCI_MSIX_ENTRY_CTRL_MASKBIT) ||
	    (msg_ctrl & PCI_MSIX_FLAGS_MASKALL))
		return true;

	return false;
}

static int vdcm_send_interrupt(struct hqmv2_vdev *vdev, int vector)
{
	struct device *dev = mdev_parent_dev(vdev->mdev);
	int ret = -1;

	if (!vdev->msix_eventfd[vector]) {
		HQMV2_ERR(dev, "[%s()] vector %d's eventfd not found\n",
			  __func__, vector);
		return -EINVAL;
	}

	ret = eventfd_signal(vdev->msix_eventfd[vector], 1);

	HQMV2_INFO(dev, "[%s()] vector %d interrupt triggered\n",
		   __func__, vector);

	if (ret != 1)
		HQMV2_ERR(dev, "[%s()] vector %d eventfd signal failed\n",
			  __func__, vector);

	return ret;
}

irqreturn_t hqmv2_vdcm_cq_isr(int irq, void *data)
{
	struct hqmv2_ims_irq_entry *irq_entry = data;
	struct hqmv2_vdev *vdev = irq_entry->vdev;
	int msix_idx = irq_entry->int_src + 1; /* +1 due to mailbox vector */
	struct hqmv2_dev *hqmv2_dev;

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);

	mutex_lock(&hqmv2_dev->resource_mutex);

	if (vdcm_msix_is_masked(vdev, msix_idx))
		set_bit(msix_idx, (unsigned long *)vdev->msix_pba);
	else
		vdcm_send_interrupt(vdev, msix_idx);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return IRQ_HANDLED;
}

struct platform_msi_ops hqmv2_ims_ops  = {
	.write_msg = hqmv2_ims_write_msg,
};

static int hqmv2_vdcm_alloc_ims_irq_vectors(struct hqmv2_vdev *vdev)
{
	struct hqmv2_ims_irq_entry *irq_entry;
	struct device *dev, *parent_dev;
	struct hqmv2_dev *hqmv2_dev;
	struct msi_desc *desc;
	int ret, nvec, i;

	dev = mdev_dev(vdev->mdev);
	parent_dev = mdev_parent_dev(vdev->mdev);

	nvec = hqmv2_vdcm_num_cq_irqs(vdev);
	if (nvec < 0) {
		HQMV2_ERR(parent_dev,
			  "[%s()] failed to lookup num irqs (%d)\n",
			  __func__, nvec);
		return nvec;
	}

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);

	for (i = 0; i < nvec; i++) {
		bool is_ldb = i < vdev->num_ldb_ports;

		irq_entry = &vdev->irq_entries[i];

		irq_entry->vdev = vdev;
		irq_entry->int_src = i;
		irq_entry->is_ldb = is_ldb;
		irq_entry->cq_id = is_ldb ? i : i - vdev->num_ldb_ports;
	}

	ret = platform_msi_domain_alloc_irqs_group(dev,
						   nvec,
						   &hqmv2_ims_ops,
						   &vdev->group_id);
	if (ret < 0) {
		HQMV2_ERR(parent_dev,
			  "[%s()] failed to allocate %d ims irqs (%d)\n",
			  __func__, nvec, ret);
		goto alloc_ims_fail;
	}

	i = 0;
	for_each_msi_entry(desc, dev) {
		irq_entry = &vdev->irq_entries[i];

		ret = devm_request_threaded_irq(dev, desc->irq,
						NULL, hqmv2_vdcm_cq_isr,
						IRQF_ONESHOT,
						"hqmv2-ims", irq_entry);
		if (ret)
			goto request_irq_fail;

		i++;
	}

	return 0;

request_irq_fail:
	irq_entry = &vdev->irq_entries[0];

	for_each_msi_entry(desc, dev) {
		if (irq_entry->int_src == i)
			break;

		hqmv2_hw_clear_cq_ims_entry(&hqmv2_dev->hw,
					    vdev->id,
					    irq_entry->cq_id,
					    irq_entry->is_ldb);

		devm_free_irq(dev, desc->irq, irq_entry);

		irq_entry++;
	}

	dev_ims_free_irqs_grp(dev, vdev->group_id);
alloc_ims_fail:
	return ret;
}

void hqmv2_vdcm_free_ims_irq_vectors(struct hqmv2_vdev *vdev)
{
	struct device *dev = mdev_dev(vdev->mdev);
	struct hqmv2_ims_irq_entry *irq_entry;
	struct hqmv2_dev *hqmv2_dev;
	struct msi_desc *desc;

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);

	irq_entry = &vdev->irq_entries[0];

	for_each_msi_entry(desc, dev) {
		hqmv2_hw_clear_cq_ims_entry(&hqmv2_dev->hw,
					    vdev->id,
					    irq_entry->cq_id,
					    irq_entry->is_ldb);

		devm_free_irq(dev, desc->irq, irq_entry);

		irq_entry++;
	}

	platform_msi_domain_free_irqs_group(dev, vdev->group_id);
}

static struct hqmv2_vdev *hqmv2_vdev_create(struct hqmv2_dev *dev,
					    struct mdev_device *mdev)
{
	struct hqmv2_vdev *vdev;
	int id;

	vdev = kzalloc(sizeof(*vdev), GFP_KERNEL);
	if (!vdev)
		return NULL;

	id = hqmv2_next_available_vdev_id();

	if (id < 0) {
		kfree(vdev);
		return NULL;
	}

	memcpy(vdev->cfg, hqmv2_pci_config, sizeof(hqmv2_pci_config));

	vdev->id = id;

	hqmv2_set_vdev_id_in_use(id, true);

	return vdev;
}

static void __hqmv2_vdcm_release(struct hqmv2_vdev *vdev)
{
	struct hqmv2_dev *hqmv2_dev;
	struct device *dev;
	int ret;

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);
	dev = mdev_parent_dev(vdev->mdev);

	mutex_lock(&hqmv2_dev->resource_mutex);

	if (vdev->released) {
		mutex_unlock(&hqmv2_dev->resource_mutex);
		return;
	}

	vdev->released = true;

	/* There's no guarantee the VM exited cleanly, so reset the VDEV before
	 * releasing it. If the VDEV was reset, this function will detect that
	 * and return early.
	 */
	hqmv2_reset_vdev(&hqmv2_dev->hw, vdev->id);

	hqmv2_vdcm_free_ims_irq_vectors(vdev);

	hqmv2_unlock_vdev(&hqmv2_dev->hw, vdev->id);

	hqmv2_hw_unregister_sw_mbox(&hqmv2_dev->hw, vdev->id);

	free_page((unsigned long)vdev->pf_to_vdev_mbox);
	free_page((unsigned long)vdev->vdev_to_pf_mbox);

	ret = vfio_unregister_notifier(mdev_dev(vdev->mdev), VFIO_GROUP_NOTIFY,
				       &vdev->group_notifier);
	WARN(ret, "vfio_unregister_notifier group failed: %d\n", ret);

	ret = vfio_unregister_notifier(mdev_dev(vdev->mdev), VFIO_IOMMU_NOTIFY,
				       &vdev->iommu_notifier);
	WARN(ret, "vfio_unregister_notifier iommu failed: %d\n", ret);

	/* Decrement the device's usage count and suspend it if the
	 * count reaches zero.
	 */
	pm_runtime_put_sync_suspend(dev);

	mutex_unlock(&hqmv2_dev->resource_mutex);
}

static void hqmv2_vdcm_release_work(struct work_struct *work)
{
	struct hqmv2_vdev *vdev;

	vdev = container_of(work, struct hqmv2_vdev, release_work);

	__hqmv2_vdcm_release(vdev);
}

static void hqmv2_vdcm_send_unmasked_interrupts(struct hqmv2_vdev *vdev)
{
	int i;

	for (i = 0; i < VDCM_MSIX_MAX_ENTRIES; i++) {
		if (!vdcm_msix_is_masked(vdev, i) &&
		    test_and_clear_bit(i, (unsigned long *)vdev->msix_pba))
			vdcm_send_interrupt(vdev, i);
	}
}

static void hqmv2_trigger_mbox_interrupt(void *arg)
{
	struct hqmv2_vdev *vdev = arg;

	/* Caller is expected to hold the resource_mutex */

	if (vdcm_msix_is_masked(vdev, VDCM_MBOX_MSIX_VECTOR))
		set_bit(VDCM_MBOX_MSIX_VECTOR, (unsigned long *)vdev->msix_pba);
	else
		vdcm_send_interrupt(vdev, VDCM_MBOX_MSIX_VECTOR);
}

/* Temporary workaround. Derived from pci_enable_pasid(), minus the end-to-end
 * TLP prefix capability error checking.
 */
static int hqmv2_enable_pasid(struct pci_dev *pdev)
{
	int pos;

	pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_PASID);
	if (!pos)
		return -EINVAL;

	pdev->pasid_features = 0;

	pci_write_config_word(pdev, pos + PCI_PASID_CTRL,
			      PCI_PASID_CTRL_ENABLE);

	pdev->pasid_enabled = 1;
#if KERNEL_VERSION(5, 3, 0) <= LINUX_VERSION_CODE
	pdev->pasid_cap = pos;
#endif

	return 0;
}

static int hqmv2_vdcm_create(struct kobject *kobj, struct mdev_device *mdev)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;
	struct pci_dev *pdev;
	struct device *dev;
	int ret;

	dev = mdev_parent_dev(mdev);
	hqmv2_dev = hqmv2_mdev_get_dev(mdev);
	pdev = hqmv2_mdev_get_pdev(mdev);

	mutex_lock(&hqmv2_dev->resource_mutex);

	if (hqmv2_hw_get_virt_mode(&hqmv2_dev->hw) == HQMV2_VIRT_SRIOV) {
		ret = -EINVAL;
		HQMV2_ERR(dev,
			  "HQM driver supports either SR-IOV or S-IOV, not both.\n");
		goto virt_mode_err;
	}

	hqmv2_hw_set_virt_mode(&hqmv2_dev->hw, HQMV2_VIRT_SIOV);

	/* Indicate to the mdev layer that this device uses IOMMU-provided
	 * isolation and protection through the HQM PCI device. Each HQM mdev
	 * receives its own IOMMU domain, identified by a unique PASID.
	 */
	mdev_set_iommu_device(mdev_dev(mdev), mdev_parent_dev(mdev));

	if (list_empty(&hqmv2_vdevs_list)) {
		ret = pci_enable_pasid(pdev, 0);
		if (ret) {
			/* TEMPORARY: HQM 2.0 uses the PASID enabled status to
			 * enable certain producer port functionality in
			 * scalable-IOV mode. On development platforms that
			 * lack end-to-end TLP prefix support, force PASID
			 * enable.
			 */
			ret = hqmv2_enable_pasid(pdev);
			if (ret) {
				HQMV2_ERR(&pdev->dev,
					  "[%s()] Failed to enable PASID: %d\n",
					  __func__, ret);
				goto enable_pasid_fail;
			}
		}
	}

	vdev = hqmv2_vdev_create(hqmv2_dev, mdev);
	if (IS_ERR_OR_NULL(vdev)) {
		ret = (!vdev) ? -EFAULT : PTR_ERR(vdev);
		HQMV2_ERR(dev,
			  "[%s()] Failed to create hqmv2 vdev: %d\n",
			  __func__, ret);
		goto create_fail;
	}

	INIT_WORK(&vdev->release_work, hqmv2_vdcm_release_work);

	vdev->mdev = mdev;
	mdev_set_drvdata(mdev, vdev);

	list_add(&vdev->next, &hqmv2_vdevs_list);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	HQMV2_INFO(dev, "[%s()] Created mdev %s\n",
		   __func__, dev_name(mdev_dev(mdev)));

	return 0;

create_fail:
	if (list_empty(&hqmv2_vdevs_list))
		pci_disable_pasid(pdev);
enable_pasid_fail:
	if (list_empty(&hqmv2_vdevs_list))
		hqmv2_hw_set_virt_mode(&hqmv2_dev->hw, HQMV2_VIRT_NONE);
virt_mode_err:
	mutex_unlock(&hqmv2_dev->resource_mutex);

	return ret;
}

static int hqmv2_vdcm_remove(struct mdev_device *mdev)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;

	vdev = mdev_get_drvdata(mdev);
	hqmv2_dev = hqmv2_mdev_get_dev(mdev);

	/* Ensure this vdev's release operation completes before acquiring the
	 * resource_mutex.
	 */
	flush_scheduled_work();

	mutex_lock(&hqmv2_dev->resource_mutex);

	list_del(&vdev->next);

	if (list_empty(&hqmv2_vdevs_list)) {
		pci_disable_pasid(hqmv2_mdev_get_pdev(mdev));
		hqmv2_hw_set_virt_mode(&hqmv2_dev->hw, HQMV2_VIRT_NONE);
	}

	hqmv2_reset_vdev_resources(&hqmv2_dev->hw, vdev->id);

	hqmv2_set_vdev_id_in_use(vdev->id, false);

	kfree(vdev);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return 0;
}

static int hqmv2_vdcm_iommu_notifier(struct notifier_block *nb,
				     unsigned long action, void *data)
{
	struct hqmv2_dev *hqmv2_dev;
	struct hqmv2_vdev *vdev;

	vdev = container_of(nb, struct hqmv2_vdev, iommu_notifier);

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);

	/* The user is unmapping the IOMMU space before releasing the vdev, so
	 * we must reset the VDEV now (while its IOVAs are still valid).
	 */
	if (action == VFIO_IOMMU_NOTIFY_DMA_UNMAP) {
		mutex_lock(&hqmv2_dev->resource_mutex);

		hqmv2_reset_vdev(&hqmv2_dev->hw, vdev->id);

		mutex_unlock(&hqmv2_dev->resource_mutex);
	}

	return NOTIFY_OK;
}

static int hqmv2_vdcm_group_notifier(struct notifier_block *nb,
				     unsigned long action, void *data)
{
	struct hqmv2_vdev *vdev;

	vdev = container_of(nb, struct hqmv2_vdev, group_notifier);

	if (action == VFIO_GROUP_NOTIFY_SET_KVM) {
		/* If the VFIO group is being deleted, schedule the release
		 * workqueue. (Run in a separate context because a notifier
		 * callout routine "must not try to register or unregister
		 * entries on its own chain.")
		 */
		if (!data)
			schedule_work(&vdev->release_work);
	}

	return NOTIFY_OK;
}

static int hqmv2_get_mdev_pasid(struct mdev_device *mdev)
{
	struct iommu_domain *domain;
	struct device *dev = mdev_dev(mdev);

	/* PASID override is intended for development systems without PASID
	 * support.
	 */
	if (hqmv2_pasid_override)
		return 0;

	/* Doesn't exist in Intel-Next v5.6 yet, so SIOV is turned off */
	domain = mdev_get_iommu_domain(dev);
	if (!domain)
		return -EINVAL;

	return iommu_aux_get_pasid(domain, dev->parent);
}

static int hqmv2_vdcm_open(struct mdev_device *mdev)
{
	struct hqmv2_get_num_resources_args rsrcs;
	struct hqmv2_dev *hqmv2_dev;
	struct device *parent_dev;
	struct hqmv2_vdev *vdev;
	unsigned long events;
	int ret;

	vdev = mdev_get_drvdata(mdev);
	hqmv2_dev = hqmv2_mdev_get_dev(mdev);
	parent_dev = mdev_parent_dev(mdev);

	mutex_lock(&hqmv2_dev->resource_mutex);

	/* Increment the device's usage count and immediately wake it if it was
	 * suspended.
	 */
	pm_runtime_get_sync(parent_dev);

	ret = hqmv2_get_mdev_pasid(mdev);
	if (ret < 0) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()] PASID get failed with error %d\n",
			  __func__, ret);
		goto pasid_get_fail;
	}

	vdev->pasid = ret;

	ret = hqmv2_hw_register_pasid(&hqmv2_dev->hw, vdev->id, vdev->pasid);
	if (ret)
		goto pasid_register_fail;

	/* Register a notifier for when VFIO is about to unmap IOVAs, in order
	 * to reset the mdev if it is active, to prevent the device from
	 * attempting to write to invalid an IOVA.
	 */
	vdev->iommu_notifier.notifier_call = hqmv2_vdcm_iommu_notifier;
	events = VFIO_IOMMU_NOTIFY_DMA_UNMAP;

	ret = vfio_register_notifier(mdev_dev(mdev), VFIO_IOMMU_NOTIFY, &events,
				     &vdev->iommu_notifier);
	if (ret != 0) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()] Failed to register iommu notifier: %d\n",
			  __func__, ret);
		goto iommu_notif_register_fail;
	}

	/* Register a KVM notifier for when a VFIO group is registered or
	 * unregistered with KVM.
	 */
	vdev->group_notifier.notifier_call = hqmv2_vdcm_group_notifier;
	events = VFIO_GROUP_NOTIFY_SET_KVM;

	ret = vfio_register_notifier(mdev_dev(mdev), VFIO_GROUP_NOTIFY, &events,
				     &vdev->group_notifier);
	if (ret != 0) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()] Failed to register group notifier: %d\n",
			  __func__, ret);
		goto group_notif_register_fail;
	}

	/* Mailbox mapping is at a page granularity, so round up size to 4KB */
	vdev->pf_to_vdev_mbox = (u8 *)get_zeroed_page(GFP_KERNEL);
	if (!vdev->pf_to_vdev_mbox) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()] Failed to alloc PF2VF mailbox\n", __func__);
		goto pf_to_vdev_alloc_fail;
	}

	vdev->vdev_to_pf_mbox = (u8 *)get_zeroed_page(GFP_KERNEL);
	if (!vdev->vdev_to_pf_mbox) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()] Failed to alloc VF2PF mailbox\n", __func__);
		goto vdev_to_pf_alloc_fail;
	}

	hqmv2_hw_register_sw_mbox(&hqmv2_dev->hw,
				  vdev->id,
				  (u32 *)vdev->vdev_to_pf_mbox,
				  (u32 *)vdev->pf_to_vdev_mbox,
				  hqmv2_trigger_mbox_interrupt,
				  vdev);

	/* Cache the assigned number of ldb and dir ports, used for IMS */
	ret = hqmv2_hw_get_num_resources(&hqmv2_dev->hw, &rsrcs,
					 true, vdev->id);
	if (ret)
		goto get_num_resources_fail;

	vdev->num_ldb_ports = rsrcs.num_ldb_ports;
	vdev->num_dir_ports = rsrcs.num_dir_ports;

	/* Set MSI-X table size using N-1 encoding */
	vdev->cfg[VDCM_MSIX_MSG_CTRL_OFFSET] = hqmv2_vdcm_num_irqs(vdev) - 1;

	hqmv2_lock_vdev(&hqmv2_dev->hw, vdev->id);

	/* IMS configuration must be done after locking the vdev, which sets
	 * its virtual->physical port ID mapping.
	 */
	ret = hqmv2_vdcm_alloc_ims_irq_vectors(vdev);
	if (ret != 0) {
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()] failed to allocate ims irq vectors: %d\n",
			  __func__, ret);
		goto ims_irq_vector_alloc_fail;
	}

	vdev->released = false;

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return 0;

ims_irq_vector_alloc_fail:
	hqmv2_unlock_vdev(&hqmv2_dev->hw, vdev->id);
get_num_resources_fail:
	hqmv2_hw_unregister_sw_mbox(&hqmv2_dev->hw, vdev->id);
vdev_to_pf_alloc_fail:
	free_page((unsigned long)vdev->pf_to_vdev_mbox);
pf_to_vdev_alloc_fail:
	vfio_unregister_notifier(mdev_dev(mdev), VFIO_GROUP_NOTIFY,
				 &vdev->group_notifier);
group_notif_register_fail:
	vfio_unregister_notifier(mdev_dev(mdev), VFIO_IOMMU_NOTIFY,
				 &vdev->iommu_notifier);
iommu_notif_register_fail:
pasid_register_fail:
pasid_get_fail:
	pm_runtime_put_sync_suspend(parent_dev);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return ret;
}

static void hqmv2_vdcm_release(struct mdev_device *mdev)
{
	struct hqmv2_vdev *vdev = mdev_get_drvdata(mdev);

	__hqmv2_vdcm_release(vdev);
}

static u64 get_reg_val(void *buf, int size)
{
	u64 val = 0;

	if (size == 8)
		val = *(u64 *)buf;
	else if (size == 4)
		val = *(u32 *)buf;
	else if (size == 2)
		val = *(u16 *)buf;
	else if (size == 1)
		val = *(u8 *)buf;

	return val;
}

static int hqmv2_vdcm_cfg_read(struct hqmv2_vdev *vdev, unsigned int pos,
			       unsigned char *buf, unsigned int count)
{
	unsigned int offset = pos & (PCI_CFG_SPACE_SIZE - 1);
	struct device *dev = mdev_parent_dev(vdev->mdev);
	struct hqmv2_dev *hqmv2_dev;

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);

	mutex_lock(&hqmv2_dev->resource_mutex);

	memcpy(buf, &vdev->cfg[offset], count);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	HQMV2_INFO(dev, "[%s()] config[%d:%d] = 0x%llx\n",
		   __func__, offset, offset + count, get_reg_val(buf, count));

	return 0;
}

static int hqmv2_vdcm_cfg_write(struct hqmv2_vdev *vdev, unsigned int pos,
				unsigned char *buf, unsigned int count)
{
	unsigned int offset = pos & (PCI_CFG_SPACE_SIZE - 1);
	struct device *dev = mdev_parent_dev(vdev->mdev);
	struct hqmv2_dev *hqmv2_dev;
	u8 *cfg = vdev->cfg;
	u64 val, bar;

	/* Only look for writable config registers. Ignore BARs 2-5;
	 * unimplemented BARs are hard-wired to zero.
	 */

	HQMV2_INFO(dev, "[%s()] config[%d:%d] = 0x%llx\n",
		   __func__, offset, offset + count, get_reg_val(buf, count));

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);

	mutex_lock(&hqmv2_dev->resource_mutex);

	switch (offset) {
	case PCI_COMMAND:
		memcpy(&cfg[offset], buf, count);
		if (count < 4)
			break;
		offset += 2;
		buf = buf + 2;
		count -= 2;
		/* Intentional Fall through */
	case PCI_STATUS:
	{
		/* Bits 8 and 11-15 are WOCLR, the rest are RO */
		u16 mask;

		mask = get_reg_val(buf, count) << (offset & 1) * 8;
		mask &= 0xf900;

		*(u16 *)&cfg[offset] = *((u16 *)&cfg[offset]) & ~mask;
		break;
	}

	case PCI_CACHE_LINE_SIZE:
	case PCI_INTERRUPT_LINE:
		memcpy(&cfg[offset], buf, count);
		break;

	case PCI_BASE_ADDRESS_0:
	case PCI_BASE_ADDRESS_1:
		/* Allow software to write all 1s to query the BAR size. Save
		 * the overwritten BAR address in case it is needed before the
		 * BAR is restored.
		 */
		val = get_reg_val(buf, count);
		bar = *(u64 *)&cfg[PCI_BASE_ADDRESS_0];

		/* Copy data either BAR0 or BAR1, depending on offset */
		memcpy((u8 *)&bar + (offset & 0x7), buf, count);

		/* Unused address bits are hardwired to zero. */
		bar &= ~(HQMV2_VDEV_BAR0_SIZE - 1);

		*(u64 *)&cfg[PCI_BASE_ADDRESS_0] = bar |
			PCI_BASE_ADDRESS_MEM_TYPE_64 |
			PCI_BASE_ADDRESS_MEM_PREFETCH;

		/* Don't overwrite BAR addr if the user is querying the size */
		if (val == -1U || val == -1ULL)
			break;

		vdev->bar0_addr = bar;

		HQMV2_INFO(dev, "[%s()] BAR0 addr: 0x%llx\n",
			   __func__, vdev->bar0_addr);

		break;

	case VDCM_PCIE_DEV_CTRL_OFFSET:
		val = get_reg_val(buf, count);

		if (val & PCI_EXP_DEVCTL_BCR_FLR)
			hqmv2_reset_vdev(&hqmv2_dev->hw, vdev->id);

		/* Per spec, software always reads 0 for the initiate FLR bit,
		 * and for the vdev Aux PME is hard-wired to 0.
		 */
		val &= ~(PCI_EXP_DEVCTL_BCR_FLR | PCI_EXP_DEVCTL_AUX_PME);

		memcpy(&cfg[offset], &val, count);

		break;

	case VDCM_MSIX_MSG_CTRL_OFFSET:
		/* Allow software to write all 1s to query the BAR size. Save
		 * the overwritten BAR address in case it is needed before the
		 * BAR is restored.
		 */
		val = get_reg_val(buf, count);

		/* Bits [15:14] are writeable, the rest are RO */
		val &= 0xC000;

		memcpy(&cfg[offset], &val, count);

		/* If the function is unmasked and any pending bits are set,
		 * fire the interrupt(s) and clear the pending bit.
		 */
		hqmv2_vdcm_send_unmasked_interrupts(vdev);

		break;

	default:
		break;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return 0;
}

static int hqmv2_vdcm_mmio_read(struct hqmv2_vdev *vdev, u64 pos, void *buf,
				unsigned int size)
{
	struct device *dev = mdev_parent_dev(vdev->mdev);
	u32 offs = pos & (HQMV2_VDEV_BAR0_SIZE - 1);
	struct hqmv2_dev *hqmv2_dev;
	u8 *addr;

	/* The function expects reads of either 8, 4, or 2 bytes, and the
	 * location to be aligned to the read size.
	 */
	if (((size & (size - 1)) != 0) || size > 8 ||
	    ((offs & (size - 1)) != 0))
		return -EINVAL;

	HQMV2_INFO(dev, "[%s()] mmio[%d:%d] = 0x%llx\n",
		   __func__, offs, offs + size, get_reg_val(buf, size));

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);

	mutex_lock(&hqmv2_dev->resource_mutex);

	switch (offs) {
	case VDCM_MSIX_TBL_OFFSET ... VDCM_MSIX_TBL_END_OFFSET:
		addr = &vdev->msix_table[offs - VDCM_MSIX_TBL_OFFSET];
		break;

	case VDCM_MSIX_PBA_OFFSET ... VDCM_MSIX_PBA_END_OFFSET:
		addr = (u8 *)vdev->msix_pba;
		addr = &addr[offs - VDCM_MSIX_PBA_OFFSET];
		break;

	default:
		addr = NULL;
		break;
	}

	if (addr)
		memcpy(buf, addr, size);
	else
		memset(buf, 0, size);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return 0;
}

static int hqmv2_vdcm_mmio_write(struct hqmv2_vdev *vdev, u64 pos, void *buf,
				 unsigned int size)
{
	struct device *dev = mdev_parent_dev(vdev->mdev);
	u32 offs = pos & (HQMV2_VDEV_BAR0_SIZE - 1);
	struct hqmv2_dev *hqmv2_dev;
	u8 *entry, *reg;
	int idx;

	/* The function expects writes of either 8, 4, or 2 bytes, and the
	 * location to be aligned to the write size.
	 */
	if (((size & (size - 1)) != 0) || size > 8 ||
	    ((offs & (size - 1)) != 0))
		return -EINVAL;

	HQMV2_INFO(dev, "[%s()] mmio[%d:%d] = 0x%llx\n",
		   __func__, offs, offs + size, get_reg_val(buf, size));

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);

	switch (offs) {
	case VDCM_MSIX_TBL_OFFSET ... VDCM_MSIX_TBL_END_OFFSET:
		mutex_lock(&hqmv2_dev->resource_mutex);

		/* Calculate the MSI-X vector */
		idx = (offs - VDCM_MSIX_TBL_OFFSET) / VDCM_MSIX_TBL_ENTRY_SZ;

		/* Find the corresponding table entry */
		entry = &vdev->msix_table[idx * VDCM_MSIX_TBL_ENTRY_SZ];
		entry += offs & (VDCM_MSIX_TBL_ENTRY_SZ - 1);

		memcpy(entry, buf, size);

		/* If the vector is unmasked and its pending bit is
		 * set, fire the interrupt and clear the pending bit.
		 */
		if (!vdcm_msix_is_masked(vdev, idx) &&
		    test_and_clear_bit(idx, (unsigned long *)vdev->msix_pba))
			vdcm_send_interrupt(vdev, idx);

		mutex_unlock(&hqmv2_dev->resource_mutex);

		break;

	case HQMV2_FUNC_VF_SIOV_VF2PF_MAILBOX_ISR_TRIGGER:
		/* Set the vdev->PF ISR in progress bit. The PF driver clears
		 * this when it's done processing the mailbox request, while
		 * the vdev driver polls it.
		 */
		reg = vdev->vdev_to_pf_mbox;
		reg += (HQMV2_FUNC_VF_VF2PF_MAILBOX_ISR % 0x1000);

		*(u32 *)reg = 1;

		/* Ensure only one thread executes the service interrupt
		 * handler at a time. Don't lock the resource_mutex here
		 * because hqmv2_handle_mbox_interrupt() will.
		 */
		mutex_lock(&hqmv2_dev->svc_isr_mutex);

		hqmv2_handle_mbox_interrupt(hqmv2_dev, vdev->id);

		mutex_unlock(&hqmv2_dev->svc_isr_mutex);

		break;
	default:
		return -ENOTSUPP;
	}

	return 0;
}

static ssize_t hqmv2_vdcm_rw(struct mdev_device *mdev, char *buf,
			     size_t count, const loff_t *ppos, bool is_write)
{
	unsigned int index = VFIO_PCI_OFFSET_TO_INDEX(*ppos);
	struct hqmv2_vdev *vdev = mdev_get_drvdata(mdev);
	struct device *dev = mdev_parent_dev(mdev);
	u64 pos = *ppos & VFIO_PCI_OFFSET_MASK;
	int ret = -EINVAL;

	if (index >= VFIO_PCI_NUM_REGIONS) {
		HQMV2_ERR(dev, "[%s()] invalid index: %u\n", __func__, index);
		return -EINVAL;
	}

	switch (index) {
	case VFIO_PCI_CONFIG_REGION_INDEX:
		if (is_write)
			ret = hqmv2_vdcm_cfg_write(vdev, pos, buf, count);
		else
			ret = hqmv2_vdcm_cfg_read(vdev, pos, buf, count);
		break;
	case VFIO_PCI_BAR0_REGION_INDEX:
		if (is_write)
			ret = hqmv2_vdcm_mmio_write(vdev, pos, buf, count);
		else
			ret = hqmv2_vdcm_mmio_read(vdev, pos, buf, count);
		break;
	case VFIO_PCI_BAR1_REGION_INDEX ... VFIO_PCI_BAR5_REGION_INDEX:
	case VFIO_PCI_ROM_REGION_INDEX:
	case VFIO_PCI_VGA_REGION_INDEX:
	default:
		HQMV2_ERR(dev, "[%s()] unsupported region: %u\n",
			  __func__, index);
	}

	return ret == 0 ? count : ret;
}

static ssize_t hqmv2_vdcm_read(struct mdev_device *mdev, char __user *buf,
			       size_t count, loff_t *ppos)
{
	unsigned int done = 0;
	int ret;

	while (count) {
		size_t filled;

		if (count >= 8 && !(*ppos % 8)) {
			u64 val;

			ret = hqmv2_vdcm_rw(mdev, (char *)&val, sizeof(val),
					    ppos, false);
			if (ret <= 0)
				goto read_err;

			if (copy_to_user(buf, &val, sizeof(val)))
				goto read_err;

			filled = 8;
		} else if (count >= 4 && !(*ppos % 4)) {
			u32 val;

			ret = hqmv2_vdcm_rw(mdev, (char *)&val, sizeof(val),
					    ppos, false);
			if (ret <= 0)
				goto read_err;

			if (copy_to_user(buf, &val, sizeof(val)))
				goto read_err;

			filled = 4;
		} else if (count >= 2 && !(*ppos % 2)) {
			u16 val;

			ret = hqmv2_vdcm_rw(mdev, (char *)&val, sizeof(val),
					    ppos, false);
			if (ret <= 0)
				goto read_err;

			if (copy_to_user(buf, &val, sizeof(val)))
				goto read_err;

			filled = 2;
		} else {
			u8 val;

			ret = hqmv2_vdcm_rw(mdev, &val, sizeof(val), ppos,
					    false);
			if (ret <= 0)
				goto read_err;

			if (copy_to_user(buf, &val, sizeof(val)))
				goto read_err;

			filled = 1;
		}

		count -= filled;
		done += filled;
		*ppos += filled;
		buf += filled;
	}

	return done;

read_err:
	return -EFAULT;
}

static ssize_t hqmv2_vdcm_write(struct mdev_device *mdev,
				const char __user *buf,
				size_t count, loff_t *ppos)
{
	unsigned int done = 0;
	int ret;

	while (count) {
		size_t filled;

		if (count >= 8 && !(*ppos % 8)) {
			u64 val;

			if (copy_from_user(&val, buf, sizeof(val)))
				goto write_err;

			ret = hqmv2_vdcm_rw(mdev, (char *)&val, sizeof(val),
					    ppos, true);
			if (ret <= 0)
				goto write_err;

			filled = 8;
		} else if (count >= 4 && !(*ppos % 4)) {
			u32 val;

			if (copy_from_user(&val, buf, sizeof(val)))
				goto write_err;

			ret = hqmv2_vdcm_rw(mdev, (char *)&val, sizeof(val),
					    ppos, true);
			if (ret <= 0)
				goto write_err;

			filled = 4;
		} else if (count >= 2 && !(*ppos % 2)) {
			u16 val;

			if (copy_from_user(&val, buf, sizeof(val)))
				goto write_err;

			ret = hqmv2_vdcm_rw(mdev, (char *)&val,
					    sizeof(val), ppos, true);
			if (ret <= 0)
				goto write_err;

			filled = 2;
		} else {
			u8 val;

			if (copy_from_user(&val, buf, sizeof(val)))
				goto write_err;

			ret = hqmv2_vdcm_rw(mdev, &val, sizeof(val),
					    ppos, true);
			if (ret <= 0)
				goto write_err;

			filled = 1;
		}

		count -= filled;
		done += filled;
		*ppos += filled;
		buf += filled;
	}

	return done;
write_err:
	return -EFAULT;
}

static int hqmv2_vdcm_mmap(struct mdev_device *mdev, struct vm_area_struct *vma)
{
	unsigned long virt_port_id, bar_pgoff, offset, index;
	struct hqmv2_vdev *vdev = mdev_get_drvdata(mdev);
	struct hqmv2_dev *hqmv2_dev;
	pgprot_t pgprot;
	s32 port_id;

	if (vma->vm_end < vma->vm_start)
		return -EINVAL;
	if (vma->vm_end - vma->vm_start != PAGE_SIZE)
		return -EINVAL;
	if ((vma->vm_flags & VM_SHARED) == 0)
		return -EINVAL;

	index = vma->vm_pgoff >> (VFIO_PCI_OFFSET_SHIFT - PAGE_SHIFT);

	if (index != VFIO_PCI_BAR0_REGION_INDEX)
		return -EINVAL;

	hqmv2_dev = hqmv2_mdev_get_dev(mdev);

	offset = (vma->vm_pgoff << PAGE_SHIFT) & VFIO_PCI_OFFSET_MASK;

	switch (offset) {
	case HQMV2_LDB_PP_BASE ... HQMV2_LDB_PP_BOUND - 1:
		bar_pgoff = hqmv2_dev->hw.func_phys_addr >> PAGE_SHIFT;

		/* The VDEV has a 0-based port ID space, but those ports can
		 * map to any physical port. Convert the virt ID to a physical
		 * ID, and in doing so check if the virt ID is valid.
		 */
		virt_port_id = (offset - HQMV2_LDB_PP_BASE) / PAGE_SIZE;

		port_id = hqmv2_hw_get_ldb_port_phys_id(&hqmv2_dev->hw,
							virt_port_id,
							vdev->id);
		if (port_id == -1)
			return -EINVAL;

		offset = HQMV2_LDB_PP_BASE + port_id * HQMV2_LDB_PP_STRIDE;
		offset >>= PAGE_SHIFT;
		offset += bar_pgoff;

		pgprot = pgprot_noncached(vma->vm_page_prot);

		return io_remap_pfn_range(vma,
					  vma->vm_start,
					  offset,
					  vma->vm_end - vma->vm_start,
					  pgprot);
	case HQMV2_DIR_PP_BASE ... HQMV2_DIR_PP_BOUND - 1:
		bar_pgoff = hqmv2_dev->hw.func_phys_addr >> PAGE_SHIFT;

		virt_port_id = (offset - HQMV2_DIR_PP_BASE) / PAGE_SIZE;

		/* The VDEV has a 0-based port ID space, but those ports can
		 * map to any physical port. Convert the virt ID to a physical
		 * ID, and in doing so check if the virt ID is valid.
		 */
		port_id = hqmv2_hw_get_dir_port_phys_id(&hqmv2_dev->hw,
							virt_port_id,
							vdev->id);
		if (port_id == -1)
			return -EINVAL;

		offset = HQMV2_DIR_PP_BASE + port_id * HQMV2_DIR_PP_STRIDE;
		offset >>= PAGE_SHIFT;
		offset += bar_pgoff;

		pgprot = pgprot_noncached(vma->vm_page_prot);

		return io_remap_pfn_range(vma,
					  vma->vm_start,
					  offset,
					  vma->vm_end - vma->vm_start,
					  pgprot);
	case HQMV2_FUNC_VF_PF2VF_MAILBOX(0):
		return vm_insert_page(vma,
				      vma->vm_start,
				      virt_to_page(vdev->pf_to_vdev_mbox));
	case HQMV2_FUNC_VF_VF2PF_MAILBOX(0):
		return vm_insert_page(vma,
				      vma->vm_start,
				      virt_to_page(vdev->vdev_to_pf_mbox));
	default:
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()] Invalid offset %lu\n", __func__, offset);
		break;
	}

	return -EINVAL;
}

static long hqmv2_vfio_device_get_info(struct hqmv2_vdev *vdev,
				       unsigned long arg)
{
	struct vfio_device_info info;
	unsigned long minsz;

	minsz = offsetofend(struct vfio_device_info, num_irqs);

	if (copy_from_user(&info, (void __user *)arg, minsz))
		return -EFAULT;

	if (info.argsz < minsz)
		return -EINVAL;

	info.flags = VFIO_DEVICE_FLAGS_PCI | VFIO_DEVICE_FLAGS_RESET;
	info.num_regions = VFIO_PCI_NUM_REGIONS + vdev->num_regions;
	info.num_irqs = VFIO_PCI_NUM_IRQS;

	return copy_to_user((void __user *)arg, &info, minsz) ? -EFAULT : 0;
}

static long hqmv2_vfio_device_get_region_info(struct hqmv2_vdev *vdev,
					      unsigned long arg)
{
	struct vfio_region_info_cap_sparse_mmap *sparse = NULL;
	struct vfio_region_info info;
	struct hqmv2_dev *hqmv2_dev;
	struct vfio_info_cap caps;
	int num_areas, i, ret;
	unsigned long minsz;
	struct hqmv2_hw *hw;
	size_t sz;

	minsz = offsetofend(struct vfio_region_info, offset);

	if (copy_from_user(&info, (void __user *)arg, minsz))
		return -EFAULT;

	if (info.argsz < minsz)
		return -EINVAL;

	info.cap_offset = 0;

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);

	switch (info.index) {
	case VFIO_PCI_CONFIG_REGION_INDEX:
		info.offset = VFIO_PCI_INDEX_TO_OFFSET(info.index);
		info.size = PCI_CFG_SPACE_SIZE;
		info.flags = VFIO_REGION_INFO_FLAG_READ |
			     VFIO_REGION_INFO_FLAG_WRITE;
		break;
	case VFIO_PCI_BAR0_REGION_INDEX:
		info.offset = VFIO_PCI_INDEX_TO_OFFSET(info.index);
		info.size = HQMV2_VDEV_BAR0_SIZE;
		info.flags = VFIO_REGION_INFO_FLAG_CAPS |
			     VFIO_REGION_INFO_FLAG_MMAP |
			     VFIO_REGION_INFO_FLAG_READ |
			     VFIO_REGION_INFO_FLAG_WRITE;

		hw = &hqmv2_dev->hw;

		/* 1 mmap'able region per LDB and DIR PP, plus one each for
		 * PF->VF and VF->PF mailbox memory.
		 */
		num_areas = vdev->num_ldb_ports + vdev->num_dir_ports + 2;
		sz = sizeof(*sparse) + num_areas * sizeof(*sparse->areas);

		sparse = kzalloc(sz, GFP_KERNEL);
		if (!sparse)
			return -ENOMEM;

		sparse->header.id = VFIO_REGION_INFO_CAP_SPARSE_MMAP;
		sparse->header.version = 1;
		sparse->nr_areas = num_areas;

		/* LDB PP mapping info */
		for (i = 0; i < vdev->num_ldb_ports; i++) {
			sparse->areas[i].offset = HQMV2_LDB_PP_BASE;
			sparse->areas[i].offset += i * PAGE_SIZE;
			sparse->areas[i].size = PAGE_SIZE;
		}

		/* DIR PP mapping info */
		for (; i < vdev->num_ldb_ports + vdev->num_dir_ports; i++) {
			u32 idx = i - vdev->num_ldb_ports;

			sparse->areas[i].offset = HQMV2_DIR_PP_BASE;
			sparse->areas[i].offset += idx * PAGE_SIZE;
			sparse->areas[i].size = PAGE_SIZE;
		}

		/* PF->VF mbox memory */
		sparse->areas[i].offset = HQMV2_FUNC_VF_PF2VF_MAILBOX(0);
		sparse->areas[i].size = PAGE_SIZE;

		/* VF->PF mbox memory */
		sparse->areas[++i].offset = HQMV2_FUNC_VF_VF2PF_MAILBOX(0);
		sparse->areas[i].size = PAGE_SIZE;

		caps.buf = NULL;
		caps.size = 0;

		/* Create a capability header chain and copy the sparse mmap
		 * info into it. This is later copied into the user buffer.
		 */
		ret = vfio_info_add_capability(&caps, &sparse->header, sz);
		if (ret) {
			kfree(sparse);
			return ret;
		}

		/* Getting this region's info is a two step operation:
		 * 1. User calls with argsz == sizeof(info), and the driver
		 *    notifies the user of the buffer size required to store
		 *    the additional sparse mmap info.
		 * 2. User retries with a sufficiently large buffer and the
		 *    driver copies the region and sparse mmap info into it.
		 */
		info.flags |= VFIO_REGION_INFO_FLAG_CAPS;
		if (info.argsz < sizeof(info) + caps.size) {
			info.argsz = sizeof(info) + caps.size;
			info.cap_offset = 0;
		} else {
			vfio_info_cap_shift(&caps, sizeof(info));
			if (copy_to_user((void __user *)arg +
					  sizeof(info), caps.buf,
					  caps.size)) {
				kfree(caps.buf);
				kfree(sparse);
				return -EFAULT;
			}
			info.cap_offset = sizeof(info);
		}

		kfree(caps.buf);
		kfree(sparse);

		break;
	case VFIO_PCI_BAR1_REGION_INDEX ... VFIO_PCI_BAR5_REGION_INDEX:
	case VFIO_PCI_ROM_REGION_INDEX:
	case VFIO_PCI_VGA_REGION_INDEX:
		info.offset = VFIO_PCI_INDEX_TO_OFFSET(info.index);
		info.size = 0;
		info.flags = 0;
		break;
	default:
		HQMV2_ERR(hqmv2_dev->hqmv2_device,
			  "[%s()] Invalid index %u\n", __func__, info.index);
		return -EINVAL;
	}

	return copy_to_user((void __user *)arg, &info, minsz) ? -EFAULT : 0;
}

static long hqmv2_vfio_device_get_irq_info(struct hqmv2_vdev *vdev,
					   unsigned long arg)
{
	struct vfio_irq_info info;
	unsigned long minsz;

	minsz = offsetofend(struct vfio_irq_info, count);

	if (copy_from_user(&info, (void __user *)arg, minsz))
		return -EFAULT;

	if (info.argsz < minsz || info.index >= VFIO_PCI_NUM_IRQS)
		return -EINVAL;

	/* Only (virtual) MSI-X interrupts are supported */
	if (info.index != VFIO_PCI_MSIX_IRQ_INDEX)
		return -EINVAL;

	info.flags = VFIO_IRQ_INFO_EVENTFD;

	info.count = hqmv2_vdcm_num_irqs(vdev);

	info.flags |= VFIO_IRQ_INFO_NORESIZE;

	return copy_to_user((void __user *)arg, &info, minsz) ? -EFAULT : 0;
}

static void hqmv2_vdcm_disable_msix_entry(struct hqmv2_vdev *vdev, int i)
{
	struct device *dev = mdev_parent_dev(vdev->mdev);

	if (!vdev->msix_eventfd[i])
		return;

	HQMV2_INFO(dev, "[%s()] Disabling MSI-X entry %d\n", __func__, i);

	eventfd_ctx_put(vdev->msix_eventfd[i]);

	vdev->msix_eventfd[i] = 0;
}

static int hqmv2_vdcm_disable_msix_entries(struct hqmv2_vdev *vdev)
{
	int i;

	for (i = 0; i < VDCM_MSIX_MAX_ENTRIES; i++)
		hqmv2_vdcm_disable_msix_entry(vdev, i);

	return 0;
}

static int hqmv2_vdcm_set_eventfd(struct hqmv2_vdev *vdev,
				  struct vfio_irq_set *hdr,
				  u32 *fds)
{
	struct device *dev = mdev_parent_dev(vdev->mdev);
	struct eventfd_ctx *ctx;
	int i;

	for (i = hdr->start; i < hdr->start + hdr->count; i++) {
		int fd = fds[i - hdr->start];

		/* fd == -1: deassign the interrupt if cfg'ed or skip it */
		if (fd < 0) {
			if (vdev->msix_eventfd[i])
				hqmv2_vdcm_disable_msix_entry(vdev, i);
			continue;
		}

		HQMV2_INFO(dev, "[%s()] Enabling MSI-X entry %d\n",
			   __func__, i);

		ctx = eventfd_ctx_fdget(fd);
		if (IS_ERR(ctx)) {
			HQMV2_ERR(dev, "[%s()] eventfd_ctx_fdget failed\n",
				  __func__);

			return PTR_ERR(ctx);
		}

		vdev->msix_eventfd[i] = ctx;
	}

	return 0;
}

static int hqmv2_vdcm_trigger_interrupt(struct hqmv2_vdev *vdev,
					struct vfio_irq_set *hdr,
					bool *trigger)
{
	bool data_none = hdr->flags & VFIO_IRQ_SET_DATA_NONE;
	int i;

	for (i = hdr->start; i < hdr->start + hdr->count; i++) {
		if (!vdev->msix_eventfd[i])
			continue;

		if (data_none || trigger[i - hdr->start])
			eventfd_signal(vdev->msix_eventfd[i], 1);
	}

	return 0;
}

static int hqmv2_vdcm_set_msix_trigger(struct hqmv2_vdev *vdev,
				       struct vfio_irq_set *hdr,
				       void *data)
{
	if (hdr->count == 0 && (hdr->flags & VFIO_IRQ_SET_DATA_NONE))
		return hqmv2_vdcm_disable_msix_entries(vdev);

	if (hdr->flags & VFIO_IRQ_SET_DATA_EVENTFD)
		return hqmv2_vdcm_set_eventfd(vdev, hdr, data);

	if (hdr->flags & (VFIO_IRQ_SET_DATA_BOOL | VFIO_IRQ_SET_DATA_NONE))
		return hqmv2_vdcm_trigger_interrupt(vdev, hdr, data);

	return 0;
}

static long hqmv2_vfio_device_set_irqs(struct hqmv2_vdev *vdev,
				       unsigned long arg)
{
	struct device *dev = mdev_parent_dev(vdev->mdev);
	struct hqmv2_dev *hqmv2_dev;
	struct vfio_irq_set hdr;
	unsigned long minsz;
	u8 *data = NULL;
	int ret;

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);

	minsz = offsetofend(struct vfio_irq_set, count);

	if (copy_from_user(&hdr, (void __user *)arg, minsz))
		return -EFAULT;

	if (hdr.argsz < minsz || hdr.index != VFIO_PCI_MSIX_IRQ_INDEX)
		return -EINVAL;

	if (!(hdr.flags & VFIO_IRQ_SET_DATA_NONE)) {
		int max = hqmv2_vdcm_num_irqs(vdev);
		size_t data_size = 0;

		ret = vfio_set_irqs_validate_and_prepare(&hdr, max,
							 VFIO_PCI_NUM_IRQS,
							 &data_size);
		if (ret) {
			HQMV2_ERR(dev, "[%s()] set IRQ validation failed\n",
				  __func__);
			return -EINVAL;
		}

		/* hdr contains data, so copy it */
		if (data_size) {
			data = memdup_user((void __user *)(arg + minsz),
					   data_size);
			if (IS_ERR(data))
				return PTR_ERR(data);
		}
	}

	mutex_lock(&hqmv2_dev->resource_mutex);

	switch (hdr.flags & VFIO_IRQ_SET_ACTION_TYPE_MASK) {
	case VFIO_IRQ_SET_ACTION_MASK:
	case VFIO_IRQ_SET_ACTION_UNMASK:
		ret = -ENOTTY;
		break;
	case VFIO_IRQ_SET_ACTION_TRIGGER:
		ret = hqmv2_vdcm_set_msix_trigger(vdev, &hdr, data);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

	mutex_unlock(&hqmv2_dev->resource_mutex);

	kfree(data);

	return ret;
}

static long hqmv2_vfio_device_reset(struct hqmv2_vdev *vdev)
{
	struct hqmv2_dev *hqmv2_dev;

	hqmv2_dev = hqmv2_mdev_get_dev(vdev->mdev);

	mutex_lock(&hqmv2_dev->resource_mutex);

	hqmv2_reset_vdev(&hqmv2_dev->hw, vdev->id);

	mutex_unlock(&hqmv2_dev->resource_mutex);

	return 0;
}

static long hqmv2_vdcm_ioctl(struct mdev_device *mdev, unsigned int cmd,
			     unsigned long arg)
{
	struct hqmv2_vdev *vdev = mdev_get_drvdata(mdev);
	struct device *dev = mdev_parent_dev(mdev);

	switch (cmd) {
	case VFIO_DEVICE_GET_INFO:
		return hqmv2_vfio_device_get_info(vdev, arg);
	case VFIO_DEVICE_GET_REGION_INFO:
		return hqmv2_vfio_device_get_region_info(vdev, arg);
	case VFIO_DEVICE_GET_IRQ_INFO:
		return hqmv2_vfio_device_get_irq_info(vdev, arg);
	case VFIO_DEVICE_SET_IRQS:
		return hqmv2_vfio_device_set_irqs(vdev, arg);
	case VFIO_DEVICE_RESET:
		return hqmv2_vfio_device_reset(vdev);
	default:
		HQMV2_ERR(dev, "[%s()] Unexpected ioctl cmd %d\n",
			  __func__, cmd);
		break;
	}

	return -ENOTSUPP;
}

static struct mdev_parent_ops hqmv2_vdcm_ops = {
	.mdev_attr_groups       = hqmv2_mdev_attr_groups,
	.supported_type_groups  = hqmv2_mdev_type_groups,
	.create                 = hqmv2_vdcm_create,
	.remove                 = hqmv2_vdcm_remove,

	.open                   = hqmv2_vdcm_open,
	.release                = hqmv2_vdcm_release,

	.read                   = hqmv2_vdcm_read,
	.write                  = hqmv2_vdcm_write,
	.mmap                   = hqmv2_vdcm_mmap,
	.ioctl                  = hqmv2_vdcm_ioctl,
};

int hqmv2_vdcm_init(struct pci_dev *pdev)
{
	INIT_LIST_HEAD(&hqmv2_vdevs_list);

	return mdev_register_device(&pdev->dev, &hqmv2_vdcm_ops);
}

void hqmv2_vdcm_exit(struct pci_dev *pdev)
{
	mdev_unregister_device(&pdev->dev);
}

#endif /* CONFIG_INTEL_HQMV2_SIOV */
