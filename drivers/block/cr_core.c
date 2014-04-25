/*
 * Copyright 2013 Intel Corporation All Rights Reserved.
 *
 * INTEL CONFIDENTIAL
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material may contain trade secrets and
 * proprietary and confidential information of Intel Corporation and its
 * suppliers and licensors, and is protected by worldwide copyright and trade
 * secret laws and treaty provisions. No part of the Material may be used,
 * copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express written
 * permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or alter
 * this notice or any other notice embedded in Materials by Intel or Intel's
 * suppliers or licensors in any way.
 */

#include <linux/crbd_dimm.h>
#include <linux/cr_ioctl.h>
#include <linux/gen_nvdimm.h>
#include <linux/nvdimm_core.h>
#include <linux/module.h>

static DEFINE_SPINLOCK(cr_spa_list_lock);
static struct list_head cr_spa_list = LIST_HEAD_INIT(cr_spa_list);

struct cr_spa_rng_tbl {
	struct spa_rng_tbl *spa_tbl;
	void __iomem *mapped_va;
	struct list_head node;
};

static struct cr_spa_rng_tbl *get_cr_spa_rng_tbl(__u16 spa_index)
{
	struct cr_spa_rng_tbl *cur_tbl = NULL;
	struct cr_spa_rng_tbl *target_tbl = NULL;

	spin_lock(&cr_spa_list_lock);
	list_for_each_entry(cur_tbl, &cr_spa_list, node) {
		if (spa_index == cur_tbl->spa_tbl->spa_index) {
			target_tbl = cur_tbl;
			break;
		}
	}
	spin_unlock(&cr_spa_list_lock);

	return target_tbl;
}

void __iomem *cr_get_virt(phys_addr_t spa_addr,
	struct memdev_spa_rng_tbl *ctrl_tbl)
{
	struct cr_spa_rng_tbl *s_tbl;
	phys_addr_t spa_offset;

	s_tbl = get_cr_spa_rng_tbl(ctrl_tbl->spa_index);

	if (!s_tbl)
		return NULL;

	if (spa_addr < (phys_addr_t)ctrl_tbl->start_spa)
		return NULL;

	spa_offset = spa_addr - (phys_addr_t)ctrl_tbl->start_spa;

	return s_tbl->mapped_va + spa_offset;
}

static void cr_free_fw_cmd(struct fv_fw_cmd *fw_cmd)
{
	kfree(fw_cmd->input_payload);
	kfree(fw_cmd->large_input_payload);
	kfree(fw_cmd->output_payload);
	kfree(fw_cmd->large_output_payload);
}

static int cr_put_fw_cmd(struct fv_fw_cmd *u_fw_cmd, struct fv_fw_cmd *k_fw_cmd)
{
	if (k_fw_cmd->output_payload_size > 0
			&& copy_to_user(
				(void __user *) u_fw_cmd->output_payload,
				k_fw_cmd->output_payload,
				u_fw_cmd->output_payload_size))
			return -EFAULT;

	if (k_fw_cmd->large_output_payload_size > 0
			&& copy_to_user(
				(void __user *) u_fw_cmd->large_output_payload,
				k_fw_cmd->large_output_payload,
				u_fw_cmd->large_output_payload_size))
			return -EFAULT;

	return 0;
}

static int cr_get_fw_cmd_large_input_buff(struct fv_fw_cmd *u_fw_cmd,
		struct fv_fw_cmd *k_fw_cmd)
{
	k_fw_cmd->large_input_payload = kzalloc(
			k_fw_cmd->large_input_payload_size,
			GFP_KERNEL);

	if (!k_fw_cmd->large_input_payload)
		return -ENOMEM;

	if (copy_from_user(k_fw_cmd->large_input_payload,
			(void __user *)u_fw_cmd->large_input_payload,
			k_fw_cmd->large_input_payload_size)) {
		kfree(k_fw_cmd->large_input_payload);
		return -EFAULT;
	}

	return 0;
}

static int cr_get_fw_cmd_input_buff(struct fv_fw_cmd *u_fw_cmd,
		struct fv_fw_cmd *k_fw_cmd)
{
	k_fw_cmd->input_payload = kzalloc(k_fw_cmd->input_payload_size,
			GFP_KERNEL);

	if (!k_fw_cmd->input_payload)
		return -ENOMEM;

	if (copy_from_user(k_fw_cmd->input_payload,
			(void __user *)u_fw_cmd->input_payload,
			k_fw_cmd->input_payload_size)) {
		kfree(k_fw_cmd->input_payload);
		return -EFAULT;
	}

	return 0;
}

static int cr_get_fw_cmd(struct fv_fw_cmd *u_fw_cmd, void __user *useraddr,
		struct fv_fw_cmd *k_fw_cmd)
{
	int ret = 0;

	if (copy_from_user(u_fw_cmd, useraddr, sizeof(*u_fw_cmd)))
		return -EFAULT;

	ret = cr_verify_fw_cmd(u_fw_cmd);

	if (ret)
		return ret;

	memcpy(k_fw_cmd, u_fw_cmd, sizeof(*k_fw_cmd));

	if (k_fw_cmd->input_payload_size > 0)
		ret = cr_get_fw_cmd_input_buff(u_fw_cmd, k_fw_cmd);
	else
		k_fw_cmd->input_payload = NULL;

	if (ret)
		return ret;

	if (k_fw_cmd->large_input_payload_size > 0)
		ret = cr_get_fw_cmd_large_input_buff(u_fw_cmd, k_fw_cmd);
	else
		k_fw_cmd->large_input_payload = NULL;

	if (ret)
		goto after_input_payload;

	if (k_fw_cmd->output_payload_size > 0) {
		k_fw_cmd->output_payload = kzalloc(
				k_fw_cmd->output_payload_size,
				GFP_KERNEL);

		if (!k_fw_cmd->output_payload) {
			ret =  -ENOMEM;
			goto after_large_input;
		}
	} else
		k_fw_cmd->output_payload = NULL;

	if (k_fw_cmd->large_output_payload_size > 0) {
		k_fw_cmd->large_output_payload = kzalloc(
				k_fw_cmd->large_output_payload_size,
				GFP_KERNEL);

		if (!k_fw_cmd->large_output_payload) {
			ret =  -ENOMEM;
			goto after_output;
		}
	} else
		k_fw_cmd->large_output_payload = NULL;

	return ret;

after_output:
	kfree(k_fw_cmd->output_payload);
after_large_input:
	kfree(k_fw_cmd->large_input_payload);
after_input_payload:
	kfree(k_fw_cmd->input_payload);

	return ret;
}

static int cr_passthrough_cmd(struct nvdimm *dimm, void __user *useraddr)
{
	struct cr_dimm *c_dimm;
	struct fv_fw_cmd u_fw_cmd;
	struct fv_fw_cmd k_fw_cmd;
	int ret = 0;

	ret = cr_get_fw_cmd(&u_fw_cmd, useraddr, &k_fw_cmd);

	if (ret)
		return ret;

	c_dimm = (struct cr_dimm *)dimm->private;

	if (!c_dimm) {
		ret = -ENODEV;
		goto out;
	}

	ret = fw_cmd_pass_thru(c_dimm, &k_fw_cmd);

	if (ret)
		goto out;

	ret = cr_put_fw_cmd(&u_fw_cmd, &k_fw_cmd);
out:
	cr_free_fw_cmd(&k_fw_cmd);
	return ret;
}

static int cr_register_ctrl_ranges(struct fit_header *fit_head)
{
	int i;
	struct spa_rng_tbl *spa_tbls = fit_head->spa_rng_tbls;
	struct memdev_spa_rng_tbl *memdev_tbls = fit_head->memdev_spa_rng_tbls;

	__u16 *registered_spa_tbls = kcalloc(fit_head->fit->num_spa_range_tbl,
		sizeof(__u16), GFP_KERNEL);

	if (!registered_spa_tbls)
		return -ENOMEM;

	for (i = 0; i < fit_head->fit->num_memdev_spa_range_tbl; i++) {
		if (memdev_tbls[i].fmt_interface_code == AEP_DIMM
			&& spa_tbls[memdev_tbls[i].spa_index].addr_rng_type
				== NVDIMM_CRTL_RNG_TYPE
			&& !registered_spa_tbls[memdev_tbls[i].spa_index]) {

			struct resource *ctrl_mem_region =
			request_mem_region_exclusive((phys_addr_t)
			spa_tbls[memdev_tbls[i].spa_index].start_addr,
			spa_tbls[memdev_tbls[i].spa_index].length, "nvdimm");

			if (!ctrl_mem_region)
				goto fail;

			registered_spa_tbls[memdev_tbls[i].spa_index] = 1;
		}
	}

	kfree(registered_spa_tbls);

	return 0;

fail:
	for (i = i - 1; i >= 0; i--) {
		if (registered_spa_tbls[memdev_tbls[i].spa_index])
			release_mem_region((phys_addr_t) spa_tbls[i].start_addr,
				spa_tbls[i].length);
	}
	kfree(registered_spa_tbls);
	return -EFAULT;
}

static void cr_unmap_ctrl_ranges(void)
{
	struct cr_spa_rng_tbl *cur_tbl, *tmp_tbl;

	spin_lock(&cr_spa_list_lock);
	list_for_each_entry_safe(cur_tbl, tmp_tbl, &cr_spa_list, node) {
		iounmap(cur_tbl->mapped_va);
		list_del(&cur_tbl->node);
		kfree(cur_tbl);
	}
	spin_unlock(&cr_spa_list_lock);
}

/*TODO: ioremap_prot or ioremap_nocache?*/
static int cr_map_ctrl_ranges(struct fit_header *fit_head)
{
	int i;
	int ret;
	struct spa_rng_tbl *spa_tbls = fit_head->spa_rng_tbls;
	struct memdev_spa_rng_tbl *memdev_tbls = fit_head->memdev_spa_rng_tbls;

	__u16 *mapped_spa_tbls =
			kcalloc(fit_head->fit->num_spa_range_tbl,
			sizeof(__u16), GFP_KERNEL);

	if (!mapped_spa_tbls)
		return -ENOMEM;

	for (i = 0; i < fit_head->fit->num_memdev_spa_range_tbl; i++) {
		if (memdev_tbls[i].fmt_interface_code == AEP_DIMM
			&& spa_tbls[memdev_tbls[i].spa_index].addr_rng_type
				== NVDIMM_CRTL_RNG_TYPE
			&& !mapped_spa_tbls[memdev_tbls[i].spa_index]) {
			struct cr_spa_rng_tbl *s_tbl =
				kzalloc(sizeof(*s_tbl), GFP_KERNEL);

			if (!s_tbl) {
				ret = -ENOMEM;
				goto fail;
			}

			s_tbl->mapped_va = ioremap_nocache((phys_addr_t)
				spa_tbls[memdev_tbls[i].spa_index].start_addr,
			spa_tbls[memdev_tbls[i].spa_index].length);

			if (!s_tbl->mapped_va) {
				NVDIMM_ERR("Failed to Map CR CTRL Ranges");
				kfree(s_tbl);
				ret = -EFAULT;
				goto fail;
			}

			s_tbl->spa_tbl = &spa_tbls[memdev_tbls[i].spa_index];

			spin_lock(&cr_spa_list_lock);
			list_add_tail(&s_tbl->node, &cr_spa_list);
			spin_unlock(&cr_spa_list_lock);

			mapped_spa_tbls[memdev_tbls[i].spa_index] = 1;
		}
	}

	kfree(mapped_spa_tbls);

	return 0;

fail:
	cr_unmap_ctrl_ranges();
	kfree(mapped_spa_tbls);
	return ret;
}

static void cr_unregister_ctrl_ranges(struct fit_header *fit_head)
{
	int i;
	struct spa_rng_tbl *spa_tbls = fit_head->spa_rng_tbls;
	struct memdev_spa_rng_tbl *memdev_tbls = fit_head->memdev_spa_rng_tbls;

	__u16 *unregistered_spa_tbls = kcalloc(fit_head->fit->num_spa_range_tbl,
		sizeof(__u16), GFP_KERNEL);

	if (!unregistered_spa_tbls)
		return;

	for (i = 0; i < fit_head->fit->num_memdev_spa_range_tbl; i++) {
		if (memdev_tbls[i].fmt_interface_code == AEP_DIMM
			&& spa_tbls[memdev_tbls[i].spa_index].addr_rng_type
				== NVDIMM_CRTL_RNG_TYPE
			&& !unregistered_spa_tbls[memdev_tbls[i].spa_index]) {

			release_mem_region((phys_addr_t) spa_tbls[i].start_addr,
							spa_tbls[i].length);

			unregistered_spa_tbls[memdev_tbls[i].spa_index] = 1;
		}
	}

	kfree(unregistered_spa_tbls);
}

static int crbd_probe(struct pmem_dev *dev)
{
	int ret;

	ret = cr_register_ctrl_ranges(dev->fit_head);

	if (ret)
		goto out;

	ret = cr_map_ctrl_ranges(dev->fit_head);

	if (ret)
		goto after_register_ranges;

	return ret;

after_register_ranges:
	cr_unregister_ctrl_ranges(dev->fit_head);
out:
	return ret;
}

static void crbd_remove(struct pmem_dev *dev)
{
	cr_unmap_ctrl_ranges();
	cr_unregister_ctrl_ranges(dev->fit_head);
}

static struct nvdimm_ops crbd_dimm_ops = {
	.initialize_dimm = cr_initialize_dimm,
	.remove_dimm = cr_remove_dimm,
	.write_label = cr_write_volume_label,
	.read_labels = cr_read_labels,
	.read = cr_dimm_read,
	.write = cr_dimm_write,
};

static struct nvdimm_ioctl_ops crbd_ioctl_ops = {
	.passthrough_cmd = cr_passthrough_cmd,
};

#define CR_VENDOR_ID 32902
#define CR_DEVICE_ID 8215
#define CR_REVISION_ID 0

static const struct nvdimm_ids cr_ids = { CR_VENDOR_ID, CR_DEVICE_ID,
		CR_REVISION_ID, AEP_DIMM };

MODULE_DEVICE_TABLE(nvdimm, cr_ids);

static struct nvdimm_driver cr_driver = {
		.owner = THIS_MODULE,
		.ids = &cr_ids,
		.probe = crbd_probe,
		.remove = crbd_remove,
		.dimm_ops = &crbd_dimm_ops,
		.ioctl_ops = &crbd_ioctl_ops,
};

static int __init crbd_dimm_init(void)
{
	int ret = nvm_register_nvdimm_driver(&cr_driver);

	if (ret)
		NVDIMM_ERR("CRBD DIMM Driver Register Failed %d\n", ret);
	else
		NVDIMM_INFO("CRBD Loaded");

	return ret;
}

module_init(crbd_dimm_init);

static void __exit crbd_dimm_exit(void)
{
	nvm_unregister_nvdimm_driver(&cr_driver);
	NVDIMM_INFO("CRBD Unloaded");
}

module_exit(crbd_dimm_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Crystal Ridge Block Driver");
MODULE_AUTHOR("Intel Corporation");
