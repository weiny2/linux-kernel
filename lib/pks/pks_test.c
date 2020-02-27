// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright(c) 2020 Intel Corporation. All rights reserved.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <asm/pgtable.h>

#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pkeys.h>
#include <linux/vmalloc.h>

#define PKS_TEST_MEM_SIZE (4096)

bool pks_cpu_enabled = false;
char *data = "deadbeef";
bool test_armed = false;
bool pass_fail = false;
unsigned long prev_cnt = 0;
unsigned long fault_cnt = 0;
bool run_on_boot = false;

static pte_t *walk_table(void *ptr)
{
	struct page *page = NULL;
	pgd_t *pgdp;
	p4d_t *p4dp;
	pud_t *pudp;
	pmd_t *pmdp;
	pte_t *ret = NULL;

	pgdp = pgd_offset_k((unsigned long)ptr);
	if (pgd_none(*pgdp) || pgd_bad(*pgdp))
		goto error;

	p4dp = p4d_offset(pgdp, (unsigned long)ptr);
	if (p4d_none(*p4dp) || p4d_bad(*p4dp))
		goto error;

	pudp = pud_offset(p4dp, (unsigned long)ptr);
	if (pud_none(*pudp) || pud_bad(*pudp))
		goto error;

	pmdp = pmd_offset(pudp, (unsigned long)ptr);
	if (pmd_none(*pmdp) || pmd_bad(*pmdp))
		goto error;

	ret = pte_offset_map(pmdp, (unsigned long)ptr);
	if (pte_present(*ret)) {
		page = pte_page(*ret);
		if (!page) {
			pte_unmap(ret);
			goto error;
		}
		printk(KERN_INFO "PKS page 0x%lx; flags 0x%lx\n",
		       (unsigned long)page, page->flags);
	}

error:
	return ret;
}

/**
 * pks_test_armed() is exported so that the fault handler can detect and report
 * back status of intentional faults.
 */
bool pks_test_armed(void)
{
	bool armed = test_armed;

	if (armed) {
		update_local_sup_key(PKS_KEY_IDX_PMEM, 0, 0);
		fault_cnt++;
	}

	return armed;
}
EXPORT_SYMBOL(pks_test_armed);

static void arm_pks_test(int ad, int wd)
{
	update_pmem_key(ad, wd);
	test_armed = true;
}

static void disarm_pks_test(void)
{
	test_armed = false;
	update_pmem_key(0, 0);
}

static bool exception_caught(void)
{
	bool ret = (fault_cnt > prev_cnt);

	prev_cnt = fault_cnt;

	return ret;
}

static void cleanup(void)
{
	test_armed = false;
	fault_cnt = 0;
	prev_cnt = 0;
	update_pmem_key(1, 1);
}

static void report_pkey_settings(void *unused)
{
	u8 pkey;
	unsigned long long msr = 0;
	unsigned int cpu = smp_processor_id();

	rdmsrl(MSR_IA32_PKRS, msr);

	printk(KERN_INFO "PKS for CPU %d : %llu\n", cpu, msr);
	for (pkey = 0; pkey < PKS_KEY_IDX_MAX; pkey++) {
		int ad, wd;

		ad = ((msr >> (pkey * PKS_BITS_PER_KEY)) & (1 << PKS_AD_OFFSET));
		wd = ((msr >> (pkey * PKS_BITS_PER_KEY)) & (1 << PKS_WD_OFFSET));
		printk(KERN_INFO "   %u: A:%d W:%d\n", pkey, ad, wd);
	}
}

static int test_mem_access(pgprot_t prot)
{
	int ret = 0;
	u8 pkey;
	void *ptr = NULL;
	pte_t *ptep;

	ptr = __vmalloc(PKS_TEST_MEM_SIZE, GFP_KERNEL, prot);
	if (!ptr) {
		printk(KERN_INFO "PKS Failed to vmalloc page???\n");
		return -ENOMEM;
	}

	printk(KERN_INFO "PKS prot 0x%lx; vaddr 0x%lx\n",
	       pgprot_val(prot), (unsigned long)ptr);

	ptep = walk_table(ptr);
	if (!ptep) {
		printk(KERN_INFO "PKS Failed to walk table???\n");
		ret = -ENOMEM;
		goto done;
	}

	pkey = pte_flags_pkey(ptep->pte);
	printk(KERN_INFO "PKS ptep 0x%lx pkey 0x%u\n",
	       (unsigned long)ptep, pkey);

	if (pgprot_val(prot) == pgprot_val(PAGE_KERNEL_PKEY_PMEM)) {
		if (pkey != PKS_KEY_IDX_PMEM) {
			printk(KERN_INFO "PKS invalid pkey with PAGE_KERNEL_PKEY_PMEM pkey 0x%u\n",
				pkey);
			ret = -EINVAL;
			goto unmap;
		}
	} else {
		if (pkey != PKS_KEY_IDX_DEFAULT) {
			printk(KERN_INFO "PKS invalid pkey with PAGE_KERNEL pkey 0x%u\n",
				pkey);
			ret = -EINVAL;
			goto unmap;
		}
	}

	if (!pks_cpu_enabled) {
		printk(KERN_INFO "PKS not CPU enabled; skipping access tests...\n");
		ret = 0;
		goto unmap;
	}

	if (pgprot_val(prot) == pgprot_val(PAGE_KERNEL_PKEY_PMEM)) {
		/* disable both read and write */
		arm_pks_test(1, 1);
		memcpy(ptr, data, strlen(data));
		if (!exception_caught()) {
			printk(KERN_INFO "PKS pkey protection failed to fault: write\n");
			ret = -EFAULT;
			goto unmap;
		}
		disarm_pks_test();

		/* disable both read and write */
		arm_pks_test(1, 1);
		memcpy(data, ptr, strlen(data));
		if (!exception_caught()) {
			printk(KERN_INFO "PKS pkey protection failed to fault: read\n");
			ret = -EFAULT;
			goto unmap;
		}
		disarm_pks_test();

		/* enable both read/write */
		arm_pks_test(0, 0);
		memcpy(ptr, data, strlen(data));
		if (exception_caught()) {
			printk(KERN_INFO "PKS pkey invalid fault.\n");
			ret = -EFAULT;
			goto unmap;
		}
		disarm_pks_test();

		/* enable both read/write */
		arm_pks_test(0, 0);
		memcpy(data, ptr, strlen(data));
		if (exception_caught()) {
			printk(KERN_INFO "PKS pkey invalid fault.\n");
			ret = -EFAULT;
			goto unmap;
		}
		disarm_pks_test();

		/* enable read only */
		arm_pks_test(0, 1);
		memcpy(ptr, data, strlen(data));
		if (!exception_caught()) {
			printk(KERN_INFO "PKS pkey failed to fault: write\n");
			ret = -EFAULT;
			goto unmap;
		}
		disarm_pks_test();

		/* enable read only */
		arm_pks_test(0, 1);
		memcpy(data, ptr, strlen(data));
		if (exception_caught()) {
			printk(KERN_INFO "PKS pkey invalid fault: read\n");
			ret = -EFAULT;
			goto unmap;
		}
		disarm_pks_test();
	} else {
		arm_pks_test(0, 0);
		memcpy(ptr, data, strlen(data));
		if (exception_caught()) {
			printk(KERN_INFO "PKS pkey protection triggered with PAGE_KERNEL\n");
			ret = -EFAULT;
			goto unmap;
		}
		disarm_pks_test();

		arm_pks_test(0, 0);
		memcpy(data, ptr, strlen(data));
		if (exception_caught()) {
			printk(KERN_INFO "PKS pkey protection triggered with PAGE_KERNEL\n");
			ret = -EFAULT;
			goto unmap;
		}
		disarm_pks_test();
	}

	cleanup();
unmap:
	pte_unmap(ptep);
done:
	vfree(ptr);
	return ret;
}

static void pks_run_test(void)
{
	int ret;

	pks_cpu_enabled = cpu_feature_enabled(X86_FEATURE_PKS);

	pass_fail = true;

	printk(KERN_INFO "\n");
	printk(KERN_INFO "\n");
	printk(KERN_INFO "     ***** BEGIN: PKS Testing (CPU enabled : %s) *****\n",
		pks_cpu_enabled ? "TRUE" : "FALSE");

	on_each_cpu(report_pkey_settings, NULL, 1);

	printk(KERN_INFO "           BEGIN: PKS PAGE_KERNEL Testing\n");
	ret = test_mem_access(PAGE_KERNEL);
	if (ret) {
		printk(KERN_INFO "PKS FAILED: Normal page testing...\n");
		pass_fail = false;
	}
	printk(KERN_INFO "           END: PKS PAGE_KERNEL Testing: %s\n",
		pass_fail ? "PASS" : "FAIL");

	printk(KERN_INFO "           BEGIN: PKS PAGE_KERNEL_PKEY_PMEM Testing\n");
	ret = test_mem_access(PAGE_KERNEL_PKEY_PMEM);
	if (ret) {
		printk(KERN_INFO "PKS FAILED: PMEM page testing...\n");
		pass_fail = false;
	}
	printk(KERN_INFO "           END: PKS PAGE_KERNEL_PKEY_PMEM Testing : %s\n",
		pass_fail ? "PASS" : "FAIL");

	printk(KERN_INFO "     ***** END: PKS Testing *****\n");
	printk(KERN_INFO "\n");
	printk(KERN_INFO "\n");
}

static ssize_t pks_read_file(struct file *file, char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "%s\n", pass_fail ? "PASS" : "FAIL");
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t pks_write_file(struct file *file, const char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	/* Any write just triggers the test. */
	pks_run_test();
	return count;
}

static const struct file_operations fops_init_pks = {
	.read = pks_read_file,
	.write = pks_write_file,
	.llseek = default_llseek,
};

static int __init parse_pks_test_options(char *str)
{
	if (parse_option_str(str, "pks-test-on-boot"))
		run_on_boot = true;

	return 0;
}
early_param("pks-test-on-boot", parse_pks_test_options);

static int __init pks_test_init(void)
{
	if (run_on_boot)
		pks_run_test();		

	debugfs_create_file("run_pks", S_IRUSR | S_IWUSR,
			arch_debugfs_dir, NULL, &fops_init_pks);
	return 0;
}
late_initcall(pks_test_init);

static void __exit pks_test_exit(void)
{
	printk(KERN_INFO "PKS test exit\n");
}
module_exit(pks_test_exit);

MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("GPL v2");

