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

	size = snprintf(buf, PAGE_SIZE, "%-20s   Weak  State\n", "Function");


	kgr_for_each_patch_fun(p, pf) {
		size += snprintf(buf + size, PAGE_SIZE - size,
				"%-20s  %5d  %5d\n", pf->name,
				 !(pf->abort_if_missing), pf->state);
	}
	return size;
}

static ssize_t refs_show(struct kobject *kobj, struct kobj_attribute *attr,
		char *buf)
{
	struct kgr_patch *p = kobj_to_patch(kobj);

	return snprintf(buf, PAGE_SIZE, "%d\n", p->refs);
}

static ssize_t replace_all_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct kgr_patch *p = kobj_to_patch(kobj);

	return snprintf(buf, PAGE_SIZE, "%d\n", p->replace_all);
}

static void kgr_taint_kernel(const struct kgr_patch *p)
{
#ifdef CONFIG_SUSE_KERNEL_SUPPORTED
	const char *modname;

#ifdef CONFIG_MODULES
	modname = p->owner ? p->owner->name : "n/a";
#else
	modname = "n/a";
#endif
	pr_warning("attempt to revert kgr patch %s (%s), setting NO_SUPPORT taint flag\n",
			p->name, modname);
	add_taint(TAINT_NO_SUPPORT, LOCKDEP_STILL_OK);
#endif
}

static ssize_t revert_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct kgr_patch *p = kobj_to_patch(kobj);
	int ret;

	kgr_taint_kernel(p);

	ret = kgr_modify_kernel(p, true);

	return ret < 0 ? ret : count;
}

static struct kobj_attribute kgr_attr_state = __ATTR_RO(state);
static struct kobj_attribute kgr_attr_refs = __ATTR_RO(refs);
static struct kobj_attribute kgr_attr_replace_all = __ATTR_RO(replace_all);
static struct kobj_attribute kgr_attr_revert = __ATTR_WO(revert);

static struct attribute *kgr_patch_sysfs_entries[] = {
	&kgr_attr_state.attr,
	&kgr_attr_refs.attr,
	&kgr_attr_replace_all.attr,
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

#ifdef CONFIG_MODULES
	if (patch->owner) {
		ret = sysfs_create_link(&patch->kobj, &patch->owner->mkobj.kobj,
				"owner");
		if (ret)
			goto err_put;
	}
#endif

	ret = sysfs_create_group(&patch->kobj, &kgr_patch_sysfs_group);
	if (ret)
		goto err_del_link;

	return 0;
err_del_link:
#ifdef CONFIG_MODULES
	if (patch->owner)
		sysfs_delete_link(&patch->kobj, &patch->owner->mkobj.kobj,
				"owner");
err_put:
#endif
	kobject_put(&patch->kobj);
	return ret;
}

void kgr_patch_dir_del(struct kgr_patch *patch)
{
	sysfs_remove_group(&patch->kobj, &kgr_patch_sysfs_group);
#ifdef CONFIG_MODULES
	if (patch->owner)
		sysfs_delete_link(&patch->kobj, &patch->owner->mkobj.kobj,
				"owner");
#endif
	kobject_put(&patch->kobj);
	wait_for_completion(&patch->finish);
}

static ssize_t in_progress_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", kgr_in_progress);
}

static ssize_t in_progress_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	if (count == 0 || buf[0] != '0')
		return -EINVAL;

	kgr_unmark_processes();
	WARN(1, "kgr: all processes marked as migrated on admin's request\n");

	return count;
}

static ssize_t force_load_module_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", kgr_force_load_module);
}

static ssize_t force_load_module_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (val != 1 && val != 0)
		return -EINVAL;

	kgr_force_load_module = val;

	return count;
}

static struct kobj_attribute kgr_attr_in_progress = __ATTR_RW(in_progress);
static struct kobj_attribute kgr_attr_force_load_module =
		__ATTR_RW(force_load_module);

static struct attribute *kgr_sysfs_entries[] = {
	&kgr_attr_in_progress.attr,
	&kgr_attr_force_load_module.attr,
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
		pr_err("kgr: cannot create kgraft directory in sysfs!\n");
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
