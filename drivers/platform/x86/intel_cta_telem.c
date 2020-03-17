// SPDX-License-Identifier: GPL-2.0
/*
 * Intel Converged Telemetry Aggregator Telemetry driver
 *
 * Copyright (c) 2019, Intel Corporation.
 * All Rights Reserved.
 *
 * Author: "David E. Box" <david.e.box@linux.intel.com>
 */

#include <linux/cdev.h>
#include <linux/intel-dvsec.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/xarray.h>

#include "intel_cta_telem.h"

/* platform device name to bind to driver */
#define TELEM_DRV_NAME		"cta_telemetry"

/* Telemetry access types */
#define TELEM_ACCESS_FUTURE	1
#define TELEM_ACCESS_BARID	2
#define TELEM_ACCESS_LOCAL	3

#define TELEM_GUID_OFFSET	0x4
#define TELEM_BASE_OFFSET	0x8
#define TELEM_TBIR_MASK		0x7
#define TELEM_ACCESS(v)		((v) & GENMASK(3, 0))
#define TELEM_TYPE(v)		(((v) & GENMASK(7, 4)) >> 4)
/* size is in bytes */
#define TELEM_SIZE(v)		(((v) & GENMASK(27, 12)) >> 10)

#define TELEM_XA_START		1
#define TELEM_XA_MAX		INT_MAX
#define TELEM_XA_LIMIT		XA_LIMIT(TELEM_XA_START, TELEM_XA_MAX)

static DEFINE_XARRAY_ALLOC(telem_array);

struct cta_telem_priv {
	struct device			*dev;
	struct intel_dvsec_header	*dvsec;
	struct telem_header		header;
	unsigned long			base_addr;
	void __iomem			*disc_table;
	struct cdev			cdev;
	dev_t				devt;
	int				devid;
};

/*
 * devfs
 */
static int cta_telem_open(struct inode *inode, struct file *filp)
{
	struct cta_telem_priv *priv;
	struct pci_driver *pci_drv;
	struct pci_dev *pci_dev;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	priv = container_of(inode->i_cdev, struct cta_telem_priv, cdev);
	pci_dev = to_pci_dev(priv->dev->parent);

	pci_drv = pci_dev_driver(pci_dev);
	if (!pci_drv)
		return -ENODEV;

	filp->private_data = priv;
	get_device(&pci_dev->dev);

	if (!try_module_get(pci_drv->driver.owner)) {
		put_device(&pci_dev->dev);
		return -ENODEV;
	}

	return 0;
}

static int cta_telem_release(struct inode *inode, struct file *filp)
{
	struct cta_telem_priv *priv = filp->private_data;
	struct pci_dev *pci_dev = to_pci_dev(priv->dev->parent);
	struct pci_driver *pci_drv = pci_dev_driver(pci_dev);

	put_device(&pci_dev->dev);
	module_put(pci_drv->driver.owner);

	return 0;
}

static int cta_telem_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct cta_telem_priv *priv = filp->private_data;
	unsigned long vsize = vma->vm_end - vma->vm_start;
	unsigned long phys = priv->base_addr;
	unsigned long pfn = PFN_DOWN(phys);
	unsigned long psize;

	psize = (PFN_UP(priv->base_addr + priv->header.size) - pfn) * PAGE_SIZE;
	if (vsize > psize) {
		dev_err(priv->dev, "Requested mmap size is too large\n");
		return -EINVAL;
	}

	if ((vma->vm_flags & VM_WRITE) || (vma->vm_flags & VM_MAYWRITE))
		return -EPERM;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (io_remap_pfn_range(vma, vma->vm_start, pfn, vsize,
			       vma->vm_page_prot))
		return -EINVAL;

	return 0;
}

static const struct file_operations cta_telem_fops = {
	.owner =	THIS_MODULE,
	.open =		cta_telem_open,
	.mmap =		cta_telem_mmap,
	.release =	cta_telem_release,
};

/*
 * sysfs
 */
static ssize_t guid_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct cta_telem_priv *priv = dev_get_drvdata(dev);

	return sprintf(buf, "0x%x\n", priv->header.guid);
}
static DEVICE_ATTR_RO(guid);

static ssize_t size_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct cta_telem_priv *priv = dev_get_drvdata(dev);

	/* Display buffer size in bytes */
	return sprintf(buf, "%u\n", priv->header.size);
}
static DEVICE_ATTR_RO(size);

static ssize_t offset_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct cta_telem_priv *priv = dev_get_drvdata(dev);

	/* Display buffer offset in bytes */
	return sprintf(buf, "%lu\n", offset_in_page(priv->base_addr));
}
static DEVICE_ATTR_RO(offset);

static struct attribute *cta_telem_attrs[] = {
	&dev_attr_guid.attr,
	&dev_attr_size.attr,
	&dev_attr_offset.attr,
	NULL
};
ATTRIBUTE_GROUPS(cta_telem);

struct class cta_telem_class = {
	.owner	= THIS_MODULE,
	.name	= "cta_telem",
	.dev_groups = cta_telem_groups,
};

/*
 * driver initialization
 */
static int cta_telem_create_dev(struct cta_telem_priv *priv)
{
	struct device *dev;
	int ret;

	cdev_init(&priv->cdev, &cta_telem_fops);
	ret = cdev_add(&priv->cdev, priv->devt, 1);
	if (ret) {
		dev_err(priv->dev, "Could not add char dev\n");
		return ret;
	}

	dev = device_create(&cta_telem_class, priv->dev, priv->devt,
			    priv, "telem%d", priv->devid);
	if (IS_ERR(dev)) {
		dev_err(priv->dev, "Could not create device node\n");
		cdev_del(&priv->cdev);
	}

	return PTR_ERR_OR_ZERO(dev);
}

static int cta_telem_add_endpoint(struct cta_telem_priv *priv)
{
	header->access_type = TELEM_ACCESS(readb(disc_offset));
	header->telem_type = TELEM_TYPE(readb(disc_offset));
	header->size = TELEM_SIZE(readl(disc_offset));
	header->guid = readl(disc_offset + TELEM_GUID_OFFSET);
	header->base_offset = readl(disc_offset + TELEM_BASE_OFFSET);

	/*
	 * For non-local access types the lower 3 bits of base offset
	 * contains the index of the base address register where the
	 * telemetry can be found.
	 */
	header->tbir = header->base_offset & TELEM_TBIR_MASK;
	header->base_offset ^= header->tbir;
}

static int cta_telem_probe(struct platform_device *pdev)
{
	struct cta_telem_priv *priv;
	struct pci_dev *parent;
	int err;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);
	priv->dev = &pdev->dev;
	parent = to_pci_dev(priv->dev->parent);

	/* TODO: replace with device properties??? */
	priv->dvsec = dev_get_platdata(&pdev->dev);
	if (!priv->dvsec) {
		dev_err(&pdev->dev, "Platform data not found\n");
		return -ENODEV;
	}

	/* Remap and access the discovery table header */
	priv->disc_table = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->disc_table))
		return PTR_ERR(priv->disc_table);

	cta_telem_populate_header(priv->disc_table, &priv->header);

	/* Local access and BARID only for now */
	switch (priv->header.access_type) {
	case TELEM_ACCESS_LOCAL:
		if (priv->header.tbir) {
			dev_err(&pdev->dev,
				"Unsupported BAR index %d for access type %d\n",
				priv->header.tbir, priv->header.access_type);
			return -EINVAL;
		}
		/* Fall Through */
	case TELEM_ACCESS_BARID:
		break;
	default:
		dev_err(&pdev->dev, "Unsupported access type %d\n",
			priv->header.access_type);
		return -EINVAL;
	}

	priv->base_addr = pci_resource_start(parent, priv->header.tbir) +
			  priv->header.base_offset;

	err = alloc_chrdev_region(&priv->devt, 0, 1, TELEM_DRV_NAME);
	if (err < 0) {
		dev_err(&pdev->dev,
			"CTA telemetry chrdev_region err: %d\n", err);
		return err;
	}

	err = xa_alloc(&telem_array, &priv->devid, priv, TELEM_XA_LIMIT,
		       GFP_KERNEL);
	if (err < 0)
		goto fail_xa_alloc;

	err = cta_telem_create_dev(priv);
	if (err < 0)
		goto fail_create_dev;

	return 0;

fail_create_dev:
	xa_erase(&telem_array, priv->devid);
fail_xa_alloc:
	unregister_chrdev_region(priv->devt, 1);

	return err;
}

static int cta_telem_remove(struct platform_device *pdev)
{
	struct cta_telem_priv *priv = platform_get_drvdata(pdev);

	device_destroy(&cta_telem_class, priv->devt);
	cdev_del(&priv->cdev);

	xa_erase(&telem_array, priv->devid);
	unregister_chrdev_region(priv->devt, 1);

	return 0;
}

static const struct platform_device_id cta_telem_table[] = {
	{
		.name = "cta_telemetry",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(platform, cta_telem_table);

static struct platform_driver cta_telem_driver = {
	.driver = {
		.name   = TELEM_DRV_NAME,
	},
	.probe  = cta_telem_probe,
	.remove = cta_telem_remove,
	.id_table = cta_telem_table,
};

static int __init cta_telem_init(void)
{
	int ret = class_register(&cta_telem_class);

	if (ret)
		return ret;

	ret = platform_driver_register(&cta_telem_driver);
	if (ret)
		class_unregister(&cta_telem_class);

	return ret;
}

static void __exit cta_telem_exit(void)
{
	platform_driver_unregister(&cta_telem_driver);
	class_unregister(&cta_telem_class);
	xa_destroy(&telem_array);
}

module_init(cta_telem_init);
module_exit(cta_telem_exit);

MODULE_AUTHOR("David E. Box <david.e.box@linux.intel.com>");
MODULE_DESCRIPTION("Intel CTA Telemetry driver");
MODULE_LICENSE("GPL v2");
