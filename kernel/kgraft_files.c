/*
 * kGraft Online Kernel Patching
 *
 *  Copyright (c) 2013-2014 SUSE
 *   Authors: Jiri Kosina
 *	      Vojtech Pavlik
 *	      Jiri Slaby
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/kernel.h>
#include <linux/kgraft.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/sysfs.h>

static struct kobject *kgr_sysfs_dir;

static inline struct kgr_patch *kobj_to_patch(struct kobject *kobj)
{
	return container_of(kobj, struct kgr_patch, kobj);
}

static void kgr_patch_kobj_release(struct kobject *kobj)
{
	struct kgr_patch *p = kobj_to_patch(kobj);

	complete(&p->finish);
}

static struct kobj_type kgr_patch_kobj_ktype = {
	.release	= kgr_patch_kobj_release,
	.sysfs_ops	= &kobj_sysfs_ops,
};

static ssize_t state_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	struct kgr_patch *p = kobj_to_patch(kobj);
	const struct kgr_patch_fun *pf;
	ssize_t size;

	size = snprintf(buf, PAGE_SIZE, "%-20s  State  Fatal\n", "Function");

	kgr_for_each_patch(p, pf) {
		size += snprintf(buf + size, PAGE_SIZE - size,
				"%-20s  %5d  %5d\n",
				pf->name, pf->state, pf->abort_if_missing);
	}
	return size;
}

static ssize_t refs_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	struct kgr_patch *p = kobj_to_patch(kobj);

	return snprintf(buf, PAGE_SIZE, "%d\n", p->refs);
}

static ssize_t revert_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct kgr_patch *p = kobj_to_patch(kobj);
	int ret;

	ret = kgr_modify_kernel(p, true);

	return ret < 0 ? ret : count;
}

static struct kobj_attribute kgr_attr_state = __ATTR_RO(state);
static struct kobj_attribute kgr_attr_refs = __ATTR_RO(refs);
static struct kobj_attribute kgr_attr_revert = __ATTR_WO(revert);

static struct attribute *kgr_patch_sysfs_entries[] = {
	&kgr_attr_state.attr,
	&kgr_attr_refs.attr,
	&kgr_attr_revert.attr,
	NULL
};

static struct attribute_group kgr_patch_sysfs_group = {
	.attrs = kgr_patch_sysfs_entries,
};

int kgr_patch_dir_add(struct kgr_patch *patch)
{
	int ret;

	ret = kobject_init_and_add(&patch->kobj, &kgr_patch_kobj_ktype,
			kgr_sysfs_dir, patch->name);
	if (ret)
		return ret;

	ret = sysfs_create_group(&patch->kobj, &kgr_patch_sysfs_group);
	if (ret)
		kobject_put(&patch->kobj);

	return ret;
}

void kgr_patch_dir_del(struct kgr_patch *patch)
{
	sysfs_remove_group(&patch->kobj, &kgr_patch_sysfs_group);
	kobject_put(&patch->kobj);
	wait_for_completion(&patch->finish);
}

static ssize_t in_progress_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", kgr_in_progress);
}

static struct kobj_attribute kgr_attr_in_progress = __ATTR_RO(in_progress);

static struct attribute *kgr_sysfs_entries[] = {
	&kgr_attr_in_progress.attr,
	NULL
};

static struct attribute_group kgr_sysfs_group = {
	.attrs = kgr_sysfs_entries,
};

int kgr_add_files(void)
{
	int ret;

	kgr_sysfs_dir = kobject_create_and_add("kgraft", kernel_kobj);
	if (!kgr_sysfs_dir) {
		pr_err("kgr: cannot create kfraft directory in sysfs!\n");
		return -EIO;
	}

	ret = sysfs_create_group(kgr_sysfs_dir, &kgr_sysfs_group);
	if (ret) {
		pr_err("kgr: cannot create attributes in sysfs\n");
		goto err_put_sysfs;
	}

	return 0;
err_put_sysfs:
	kobject_put(kgr_sysfs_dir);
	return ret;
}

void kgr_remove_files(void)
{
	sysfs_remove_group(kgr_sysfs_dir, &kgr_sysfs_group);
	kobject_put(kgr_sysfs_dir);
}
