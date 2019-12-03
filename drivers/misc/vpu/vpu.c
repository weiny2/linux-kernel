// SPDX-License-Identifier: GPL-2.0-only
/*
 * Kernel module for:
 *    1. Fixed memory mapping for IOMMU supported devices
 *
 * Copyright (C) 2019 Intel Corporation
 *
 */
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/iommu.h>

static int vpu_platform_driver_probe(struct platform_device *pdev)
{
    int rc = 0;
    struct device *dev = &pdev->dev;
    struct resource phy_res;
    int i;
    const __be32 *prop_reg;
    int reg_len;

    struct iommu_domain *vpu_domain = NULL;

    prop_reg = of_get_property(dev->of_node,"reg", &reg_len);
    if (prop_reg == NULL)
    {
        pr_info("vpu-mm: no entries found for mem mapping\n");
        return -1;
    }

    reg_len /= sizeof(*prop_reg);
    pr_info("vpu-mm: found %d entries for mapping\n", reg_len/2);

    if (iommu_present(dev->bus))
    {
        if (!vpu_domain)
            vpu_domain = iommu_domain_alloc(dev->bus);

        iommu_attach_device(vpu_domain, dev);

        for (i = 0; i < reg_len/2 ;i++)
        {
            rc = of_address_to_resource(dev->of_node, i, &phy_res);
            if (rc) {
              dev_err(dev, "No memory region to map\n");
              goto error_exit;
            }

            iommu_map(vpu_domain, (unsigned int)be32_to_cpu(prop_reg[i*2]), (unsigned long long)phy_res.start,
                      resource_size(&phy_res), IOMMU_READ | IOMMU_WRITE);
    
            pr_info("vpu-mm: iommu mapping done for vaddr: 0x%0X, paddr: 0x%0llX\n", be32_to_cpu(prop_reg[i*2]), phy_res.start);
        }
    }
    else
    {
          pr_info("vpu-mm: iommu not present");
    }

    return 0;

error_exit:
    return -1;
}

static void vpu_platform_driver_shutdown(struct platform_device *pdev)
{
}

static struct of_device_id vpu_of_match[] = {
    { .compatible = "intel,vpu", },
    { /* end of table */}
};

static struct platform_driver vpu_platform_driver = {
    .probe  = vpu_platform_driver_probe,
    .shutdown = vpu_platform_driver_shutdown,
    .driver = {
        .owner = THIS_MODULE,
        .name  = "intel,vpu",
        .of_match_table = vpu_of_match,
    },
};
builtin_platform_driver(vpu_platform_driver);

MODULE_DESCRIPTION("VPU Driver");
MODULE_AUTHOR("Raja Subramanian, Lakshmi Bai <lakshmi.bai.raja.subramanian@intel.com>");
MODULE_AUTHOR("Demakkanavar, Kenchappa <kenchappa.demakkanavar@intel.com>");
MODULE_LICENSE("GPL v2");
