/*
 * Copyright (c) 2013 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 */

#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/dmi.h>
#include <linux/genhd.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pmem.h>
#include <linux/slab.h>
#include <linux/kref.h>
#include <linux/miscdevice.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include <linux/nvdimm_core.h>
#include <linux/nvdimm_ioctl.h>
#include <linux/nvdimm_acpi.h>
#include <linux/gen_nvdimm.h>
#include <linux/gen_nvm_volumes.h>

static int pmem_major;
static struct pmem_dev *dev;

static DEFINE_SPINLOCK(nvdimm_driver_lock);
static struct list_head nvdimm_driver_list =
		LIST_HEAD_INIT(nvdimm_driver_list);

int match_nvdimm_ids(const struct nvdimm_ids *first_ids,
		const struct nvdimm_ids *second_ids)
{
	return first_ids->device_id == second_ids->device_id &&
		first_ids->vendor_id == second_ids->vendor_id &&
		first_ids->rid == second_ids->rid &&
		first_ids->fmt_interface_code
					== second_ids->fmt_interface_code;
}

/**
 * find_nvdimm_driver() - Find NVDIMM Driver
 * @ids: Key identifiers of the driver
 *
 * Find the NVDIMM driver for a specific nvdimm format
 *
 * Context: Cannot Sleep
 *
 * Returns:
 * driver - success
 * null - operations not found, NVDIMM type unsupported
 */
struct nvdimm_driver *find_nvdimm_driver(const struct nvdimm_ids *ids)
{
	struct nvdimm_driver *driver;

	list_for_each_entry(driver, &nvdimm_driver_list, nvdimm_driver_node)
		if (match_nvdimm_ids(ids, driver->ids))
			return driver;

	return NULL;
}

static int dimm_ids_exist(struct pmem_dev *dev,
		const struct nvdimm_ids *ids)
{
	struct nvdimm *cur_dimm;
	list_for_each_entry(cur_dimm, &dev->nvdimms, dimm_node) {
		if (match_nvdimm_ids(&cur_dimm->ids, ids))
			return 1;
	}

	return 0;
}

static void nvm_init_vendor_dimm(struct fit_header *fit_head,
		struct nvdimm *dimm)
{
	int err;
	if (dimm->dimm_ops && dimm->dimm_ops->initialize_dimm) {
		err = dimm->dimm_ops->initialize_dimm(dimm, fit_head);

		if (err)
			NVDIMM_DBG("Dimm %#x failed to initialize, Error: %d",
					dimm->physical_id, err);
		else
			dimm->health = NVDIMM_INITIALIZED;
	}
}

static void nvm_attach_ops(struct pmem_dev *dev, struct nvdimm_driver *driver)
{
	struct nvdimm *dimm;

	list_for_each_entry(dimm, &dev->nvdimms, dimm_node) {
		if (dimm->health == NVDIMM_UNINITIALIZED
				&& match_nvdimm_ids(&dimm->ids, driver->ids)) {
			dimm->dimm_ops = driver->dimm_ops;
			dimm->ioctl_ops = driver->ioctl_ops;

			nvm_init_vendor_dimm(dev->fit_head, dimm);
		}
	}
}

static void nvm_remove_vendor_dimm(struct nvdimm *dimm)
{
	int err;
	if (dimm->dimm_ops && dimm->dimm_ops->remove_dimm) {
		err = dimm->dimm_ops->remove_dimm(dimm);
		if (err) {
			NVDIMM_DBG(
		"Dimm %#x failed to Remove, Error: %d",
					dimm->physical_id, err);
		/* TODO: What happens if vendor says removal failed? */
		} else {
			dimm->health = NVDIMM_UNINITIALIZED;
			NVDIMM_INFO("DIMM:%#hx uninitialized",
					dimm->physical_id);
		}
	}
}

static void nvm_remove_ops(struct pmem_dev *dev, struct nvdimm_driver *driver)
{
	struct nvdimm *dimm;

	list_for_each_entry(dimm, &dev->nvdimms, dimm_node) {
		if (dimm->health == NVDIMM_INITIALIZED
				&& match_nvdimm_ids(&dimm->ids, driver->ids)) {
			nvm_remove_vendor_dimm(dimm);
			dimm->dimm_ops = NULL;
			dimm->ioctl_ops = NULL;
		}
	}
}

/**
 * TODO:Determine what pool ops are required for a driver that
 * wishes to support block ops
 */
static int validate_pool_ops(const struct nvm_pool_ops *pool_ops)
{
	return 0;
}

static int validate_dimm_ops(const struct nvdimm_ops *dimm_ops)
{
	if (!dimm_ops->initialize_dimm ||
		!dimm_ops->remove_dimm ||
		!dimm_ops->read_labels ||
		!dimm_ops->write_label ||
		!dimm_ops->read	||
		!dimm_ops->write)
		return -EINVAL;

	return 0;
}

static int validate_driver(struct nvdimm_driver *driver)
{
	int ret;

	if (!driver->ids || !driver->probe || !driver->remove)
		return -EINVAL;

	if (driver->dimm_ops) {

		/* TODO: Implement when pool ops have been created */
/*		if (!driver->pool_ops)
			return -EINVAL;*/

		ret = validate_dimm_ops(driver->dimm_ops);

		if (ret)
			return ret;

		ret = validate_pool_ops(driver->pool_ops);

		if (ret)
			return ret;
	}

	return 0;
}

/**
 * nvm_register_nvdimm_driver() - Register NVDIMM driver
 * @driver: NVDIMM driver to add to list
 *
 * Add a given nvdimm driver to the driver list
 * if it doesn't already exist in the list
 *
 * Only one driver table per nvdimm key identifiers can exist
 *
 * Returns:
 * 0 - success
 * -EEXIST - operations already exist in list
 */
int nvm_register_nvdimm_driver(struct nvdimm_driver *driver)
{
	int ret = 0;

	ret = validate_driver(driver);

	if (ret)
		return ret;

	spin_lock(&nvdimm_driver_lock);
	if (find_nvdimm_driver(driver->ids)) {
		spin_unlock(&nvdimm_driver_lock);
		return -EEXIST;
	}
	list_add(&driver->nvdimm_driver_node, &nvdimm_driver_list);
	spin_unlock(&nvdimm_driver_lock);

	if (dimm_ids_exist(dev, driver->ids)) {
		ret = driver->probe(dev);
		if (ret) {
			NVDIMM_DBG("Module probe failed");
			goto after_list_add;
		}

		nvm_attach_ops(dev, driver);

		/* TODO: Call init_pools */
		/* TODO: Call volume init due to new pools */
	}

	return 0;

after_list_add:
	spin_lock(&nvdimm_driver_lock);
	list_del(&driver->nvdimm_driver_node);
	spin_unlock(&nvdimm_driver_lock);

	return ret;
}
EXPORT_SYMBOL(nvm_register_nvdimm_driver);


/**
 * nvm_unregister_nvdimm_driver - Unregister NVDIMM Driver
 * @driver: The type specific nvdimm driver to remove
 *
 * Remove the nvdimm driver from the driver list
 *
 * Returns:
 * None
 */
void nvm_unregister_nvdimm_driver(struct nvdimm_driver *driver)
{
	spin_lock(&nvdimm_driver_lock);
	if (!find_nvdimm_driver(driver->ids)) {
		NVDIMM_DBG("Trying to remove unregistered NVDIMM driver");
		/* TODO: Bug out? */
	}
	list_del(&driver->nvdimm_driver_node);
	spin_unlock(&nvdimm_driver_lock);

	driver->remove(dev);

	/* Remove volumes that contain extents from this driver */
	/* Destory pools made from this driver */
	/* Remove the ops from nvdimms that match this driver */
	nvm_remove_ops(dev, driver);
}
EXPORT_SYMBOL(nvm_unregister_nvdimm_driver);

static void nvdimm_dmidecode(const struct dmi_header *dh, void *arg)
{
	struct nvdimm *dimm = arg;

	if (dh->type == DMI_ENTRY_MEM_DEVICE && dh->handle == dimm->physical_id)
		dimm->dmi_physical_dev = (struct memdev_dmi_entry *)dh;

	if (dh->type == DMI_ENTRY_MEM_DEV_MAPPED_ADDR) {
		struct memdev_mapped_addr_entry *entry =
			(struct memdev_mapped_addr_entry *) dh;
		if (entry->device_handle == dimm->physical_id)
			dimm->dmi_device_mapped_addr = entry;
	}
}

int get_dmi_memdev(struct nvdimm *dimm)
{
	dmi_walk(nvdimm_dmidecode, dimm);

	if (!dimm->dmi_device_mapped_addr || !dimm->dmi_physical_dev) {
		NVDIMM_DBG("SMBIOS Tables not found");
		return -ENODEV;
	}

	return 0;
}

static int nvdimm_user_dimm_init(struct pmem_dev *dev)
{
	int ret;

	if (dev->fit_head == NULL)
		return -EINVAL;

	if (!nvm_dimm_list_empty(dev)) {
		NVDIMM_DBG("NVDIMM Inventory already created");
		NVDIMM_DBG("reload module to create new inventory");
		return -EEXIST;
	}

	ret = gen_nvdimm_init(dev);

	return ret;
}

static int nvdimm_get_dimm_topology(struct pmem_dev *dev,
		struct nvdimm_req *nvdr)
{
	struct nvdimm_topology *topo_array;
	size_t size_needed;
	int num_nvdimms;
	int count;
	int ret = 0;

	num_nvdimms = nvm_dimm_list_size(dev);

	if (!num_nvdimms) {
		NVDIMM_DBG("No NVDIMMs present in inventory");
		return -ENODEV;
	}

	size_needed = (num_nvdimms * sizeof(*topo_array));

	if (nvdr->nvdr_data_size != size_needed) {
		NVDIMM_DBG("Incorrect data size");
		return -EINVAL;
	}

	topo_array = kzalloc(size_needed, GFP_KERNEL);

	if (!topo_array)
		return -ENOMEM;

	count = nvm_get_dimm_topology(num_nvdimms, topo_array, dev);

	if (count != num_nvdimms) {
		NVDIMM_DBG("Unable to retrieve topology for all NVDIMMs");
		ret = -EINVAL;
		goto after_topo_array;
	}

	if (copy_to_user((void __user *) nvdr->nvdr_data, topo_array,
			nvdr->nvdr_data_size))
		ret = -EFAULT;

after_topo_array:
	kfree(topo_array);

	return ret;
}

static int nvdimm_user_load_fit(struct pmem_dev *dev,
	void *user_fit_ptr)
{
	if (dev->fit_head) {
		NVDIMM_DBG("FIT already exists");
		NVDIMM_DBG("reload module to load a new FIT");
		return -EEXIST;
	}

	if (!user_fit_ptr) {
		NVDIMM_DBG("NVDIMM FIT PTR is NULL");
		return -EINVAL;
	}

	dev->fit_head = create_fit_table(user_fit_ptr);

	if (IS_ERR(dev->fit_head)) {
		NVDIMM_DBG("Unable to Create FIT Table");
		return PTR_ERR(dev->fit_head);
	}
	return 0;
}

static int nvdimm_pass_through(struct nvdimm_req *nvdr,
		struct nvdimm *dimm)
{
	void __user *useraddr = (void __user *)nvdr->nvdr_data;
	const struct nvdimm_ioctl_ops *ops = dimm->ioctl_ops;

	if (!ops->passthrough_cmd) {
		NVDIMM_DBG("Passthrough not supported");
		return -EOPNOTSUPP;
	}
	return ops->passthrough_cmd(dimm, useraddr);
}

static long pmem_dev_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct pmem_dev *dev = f->private_data;
	struct nvdimm_req nvdr;
	struct nvdimm *dimm;

	if (_IOC_TYPE(cmd) != NVDIMM_MAGIC) {
		NVDIMM_DBG("IOCTL Magic does not match");
		return -ENOTTY;
	}

	if (!capable(CAP_SYS_ADMIN)) {
		NVDIMM_DBG("IOCTL Permission not SYS ADMIN");
		return -EPERM;
	}

	switch (cmd) {
	/* Debug IOCTLs */
	case NVDIMM_LOAD_ACPI_FIT:
		return nvdimm_user_load_fit(dev, (void *)arg);
		break;
	case NVDIMM_INIT:
		return nvdimm_user_dimm_init(dev);
		break;
	/* IOCTLS that do not require data from userspace */
	case NVDIMM_GET_TOPOLOGY_COUNT:
		return nvm_dimm_list_size(dev);
		break;
	}

	if (copy_from_user(&nvdr, (void __user *)arg, sizeof(nvdr)))
		return -EFAULT;

	/* IOCTLs that apply to all vendors */
	switch (cmd) {
	case NVDIMM_GET_DIMM_TOPOLOGY:
		return nvdimm_get_dimm_topology(dev, &nvdr);
		break;
	}

	/* IOCTLS for specific NVDIMM types */
	dimm = get_nvdimm_by_pid(nvdr.nvdr_dimm_id, dev);

	if (!dimm)
		return -ENODEV;

	if (!dimm->ioctl_ops) {
		NVDIMM_DBG("IOCTL OPS not registered for DIMM");
		return -EOPNOTSUPP;
	}
	switch (cmd) {
	case NVDIMM_PASSTHROUGH_CMD:
		return nvdimm_pass_through(&nvdr, dimm);
	default:
		return -ENOTTY;
	}
}

/**
 * Return -ENOTTY if PM not supported
 */
static struct pmem_layout *nvdimm_getpmem(struct block_device *bdev)
{
	struct nvm_volume *volume = bdev->bd_disk->private_data;
	struct pmem_layout *layout;
	struct persistent_memory_extent *extent;
	struct extent_set *pos;
	__u16 num_extent_sets = 0;
	int i = 0;
	int ret = 0;

	if (!(volume->volume_attributes & VOLUME_PM_CAPABLE))
		return ERR_PTR(-ENOTTY);

	mutex_lock(&volume->extent_set_lock);

	list_for_each_entry(pos, &volume->extent_set_list, extent_set_node) {
		num_extent_sets++;
	}

	layout = kzalloc(sizeof(*layout) + (num_extent_sets * sizeof(*extent)),
			GFP_KERNEL);

	if (!layout) {
		ret = -ENOMEM;
		goto release_mutex;
	}

	layout->pml_extent_count = num_extent_sets;
	layout->pml_interleave = volume->interleave_size;

	list_for_each_entry(pos, &volume->extent_set_list, extent_set_node) {
		layout->pml_extents[i].pme_len = pos->set_size;
		layout->pml_total_size += pos->set_size;
		layout->pml_extents[i].pme_spa = pos->spa_start;
		layout->pml_extents[i].pme_numa_node = pos->numa_node;
		i++;
	}

	layout->pml_flags = PMEM_ENABLED;
	mutex_unlock(&volume->extent_set_lock);

	return layout;

release_mutex:
	mutex_unlock(&volume->extent_set_lock);
	return ERR_PTR(ret);
}

static const struct block_device_operations vol_dev_ops = {
/*	.getpmem = nvdimm_getpmem, */
	.owner = THIS_MODULE,
};

static void noop_make_request(struct request_queue *q, struct bio *bio)
{
	bio_endio(bio, -EIO);
}

static int pmem_alloc_disk(struct nvm_volume *volume)
{
	int ret = 0;

	if (!(volume->volume_attributes & VOLUME_ENABLED))
		return -EACCES;

	volume->queue = blk_alloc_queue(GFP_KERNEL);

	if (!volume->queue) {
		NVDIMM_ERR("Unable to create request queue.\n");
		ret = -ENOMEM;
		goto out;
	}

	volume->queue->queue_flags = QUEUE_FLAG_DEFAULT;
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, volume->queue);

	blk_queue_make_request(volume->queue, noop_make_request);

	volume->queue->queuedata = volume;

	blk_queue_logical_block_size(volume->queue, volume->block_size);
	blk_queue_physical_block_size(volume->queue, volume->block_size);

	volume->gd = alloc_disk(1);

	if (!volume->gd) {
		NVDIMM_ERR("Unable to allocate gen disk");
		ret = -ENOMEM;
		goto after_queue;
	}

	volume->gd->major = pmem_major;
	volume->gd->first_minor = volume->volume_id;
	volume->gd->minors = 1;
	volume->gd->fops = &vol_dev_ops;
	volume->gd->queue = volume->queue;
	volume->gd->private_data = volume;

	set_capacity(volume->gd,
			(volume->block_size * volume->block_count) >> 9);

	snprintf(volume->gd->disk_name, 32, "pmem%d", volume->volume_id);

	add_disk(volume->gd);

	return ret;

after_queue:
	blk_cleanup_queue(volume->queue);
out:
	return ret;
}

static int pmem_dev_open(struct inode *inode, struct file *f)
{
	struct pmem_dev *dev = container_of(f->private_data, struct pmem_dev,
								miscdev);
	kref_get(&dev->kref);
	f->private_data = dev;
	return 0;
}

static void pmem_remove_volume(struct nvm_volume *volume)
{
	del_gendisk(volume->gd);
	blk_cleanup_queue(volume->queue);
	put_disk(volume->gd);
	nvm_delete_volume(volume);
}

static void pmem_free_volumes(struct pmem_dev *dev)
{
	struct nvm_volume *pos;
	struct nvm_volume *n;

	list_for_each_entry_safe(pos, n, &dev->volumes, volume_node) {
		spin_lock(&dev->volume_lock);
		list_del(&pos->volume_node);
		spin_unlock(&dev->volume_lock);
		pmem_remove_volume(pos);
	}

}

static void pmem_free_dev(struct kref *kref)
{
	struct pmem_dev *dev = container_of(kref, struct pmem_dev, kref);

	gen_nvdimm_exit(dev);
	free_fit_table(dev->fit_head);
	kfree(dev);
}

static int pmem_dev_release(struct inode *inode, struct file *f)
{
	struct pmem_dev *dev = f->private_data;
	kref_put(&dev->kref, pmem_free_dev);
	return 0;
}

static const struct file_operations pmem_dev_fops = {
	.owner		= THIS_MODULE,
	.open		= pmem_dev_open,
	.release	= pmem_dev_release,
	.unlocked_ioctl	= pmem_dev_ioctl,
	.compat_ioctl	= pmem_dev_ioctl,
};

static int pmem_dev_init(void)
{
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);

	if (!dev)
		return -ENOMEM;

	INIT_LIST_HEAD(&dev->volumes);
	INIT_LIST_HEAD(&dev->nvdimms);
	INIT_LIST_HEAD(&dev->nvm_pools);

	spin_lock_init(&dev->volume_lock);
	spin_lock_init(&dev->nvdimm_lock);
	spin_lock_init(&dev->nvm_pool_lock);

	dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	dev->miscdev.name = "pmem";
	dev->miscdev.fops = &pmem_dev_fops;

	ret = misc_register(&dev->miscdev);

	if (ret)
		goto out;

	kref_init(&dev->kref);
	return 0;

out:
	kfree(dev);
	return ret;
}

/**
 * gen_nvdimm_init() - Initialize the generic NVDIMM system
 * @fit_head: NVDIMM FIT for the system
 *
 * Initialize all of the type specific NVDIMM systems and
 * initialize the NVDIMM inventory
 *
 * Returns: Vendor Specific Error codes if the initialization fails
 */
int gen_nvdimm_init(struct pmem_dev *dev)
{
	struct nvm_volume *pos;

	nvm_initialize_dimm_inventory(dev);

	/*
	 * TODO: temp holding spot for single volume until initialization
	 * order has been finalized
	 */
	nvm_create_single_volume(dev);

	list_for_each_entry(pos, &dev->volumes, volume_node) {
		spin_lock(&dev->volume_lock);
		pmem_alloc_disk(pos);
		spin_unlock(&dev->volume_lock);
	}

	return 0;
}

/* gen_nvdimm_exit() - Generic NVDIMM exit
 *
 * Removes all NVDIMM and type specific DIMM structures from memory
 * Calls each type specific exit function
 */
void gen_nvdimm_exit(struct pmem_dev *dev)
{
	pmem_free_volumes(dev);
	/* pmem_free_pools(dev); */
	nvm_remove_dimm_inventory(dev);
}

static int __init nvdimm_init(void)
{
	int ret = 0;

	ret = register_blkdev(pmem_major, "pmem");

	if (ret < 0)
		goto out;
	else if (ret > 0)
		pmem_major = ret;

	ret = pmem_dev_init();

	if (ret) {
		NVDIMM_ERR("Failed to create pmem_dev");
		goto unregister_blkdev;
	}

	return ret;

unregister_blkdev:
	unregister_blkdev(pmem_major, "pmem");
out:
	return ret;
}
module_init(nvdimm_init);

static void __exit nvdimm_exit(void)
{
	misc_deregister(&dev->miscdev);
	kref_put(&dev->kref, pmem_free_dev);
	unregister_blkdev(pmem_major, "pmem");
}

module_exit(nvdimm_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Generic NVDIMM Block Driver");
MODULE_AUTHOR("Intel Corporation");
