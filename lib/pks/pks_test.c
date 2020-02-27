// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright(c) 2020 Intel Corporation. All rights reserved.
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <asm/pgtable.h>

#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/pkeys.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#define PKS_TEST_MEM_SIZE (4096)

static bool run_on_boot = false;

/**
 * We must lock the following globals for brief periods while the fault handler
 * checks/updates them.
 */
static DEFINE_SPINLOCK(test_lock);
static int test_armed_key = 0;
static unsigned long prev_cnt = 0;
static unsigned long fault_cnt = 0;

struct pks_test_ctx {
	bool pass;
	bool pks_cpu_enabled;
	int pkey;
	char data[64];
};

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
		pr_info("page 0x%lx; flags 0x%lx\n",
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
	bool armed = (test_armed_key != 0);

	if (armed) {
		/* Enable read and write to stop faults */
		update_local_sup_key(test_armed_key, 0);
		fault_cnt++;
	}

	return armed;
}
EXPORT_SYMBOL(pks_test_armed);

static bool exception_caught(void)
{
	bool ret = (fault_cnt != prev_cnt);

	prev_cnt = fault_cnt;
	return ret;
}

static void report_pkey_settings(void *unused)
{
	u8 pkey;
	unsigned long long msr = 0;
	unsigned int cpu = smp_processor_id();

	rdmsrl(MSR_IA32_PKRS, msr);

	pr_info("for CPU %d : 0x%llx\n", cpu, msr);
	for (pkey = 0; pkey < PKS_NUM_KEYS; pkey++) {
		int ad, wd;

		ad = (msr >> (pkey * PKRU_BITS_PER_PKEY)) & PKEY_DISABLE_ACCESS;
		wd = (msr >> (pkey * PKRU_BITS_PER_PKEY)) & PKEY_DISABLE_WRITE;
		pr_info("   %u: A:%d W:%d\n", pkey, ad, wd);
	}
}

struct pks_access_test {
	int ad;
	int wd;
	bool write;
	bool exception;
};

static struct pks_access_test pkey_test_ary[] = {
	/*  disable both */
	{ PKEY_DISABLE_ACCESS, PKEY_DISABLE_WRITE,   true,  true },
	{ PKEY_DISABLE_ACCESS, PKEY_DISABLE_WRITE,   false, true },

	/*  enable both */
	{ 0, 0, true,  false },
	{ 0, 0, false, false },

	/*  enable read only */
	{ 0, PKEY_DISABLE_WRITE,  true,  true },
	{ 0, PKEY_DISABLE_WRITE,  false, false },
};

static int run_access_test(struct pks_test_ctx *ctx,
			   struct pks_access_test *test,
			   void *ptr)
{
	int ret = 0;
	bool exception;

	update_local_sup_key(ctx->pkey, test->ad | test->wd);

	spin_lock(&test_lock);
	test_armed_key = ctx->pkey;

	if (test->write)
		memcpy(ptr, ctx->data, 8);
	else
		memcpy(ctx->data, ptr, 8);

	exception = exception_caught();

	test_armed_key = 0;
	spin_unlock(&test_lock);

	if (test->exception != exception) {
		pr_err("pkey test FAILED: ad %d; wd %d; write %s; exception %s != %s\n",
			test->ad, test->wd,
			test->write ? "TRUE" : "FALSE",
			test->exception ? "TRUE" : "FALSE",
			exception ? "TRUE" : "FALSE");
		ret = -EFAULT;
	}

	return ret;
}

static void test_mem_access(struct pks_test_ctx *ctx)
{
	int i, rc;
	u8 pkey;
	void *ptr = NULL;
	pte_t *ptep;

	ptr = __vmalloc_node_range(PKS_TEST_MEM_SIZE, 1, VMALLOC_START, VMALLOC_END,
				   GFP_KERNEL, PAGE_KERNEL_PKEY(ctx->pkey),
				   0, NUMA_NO_NODE, __builtin_return_address(0));
	if (!ptr) {
		pr_err("Failed to vmalloc page???\n");
		ctx->pass = false;
		return;
	}

	ptep = walk_table(ptr);
	if (!ptep) {
		pr_err("Failed to walk table???\n");
		ctx->pass = false;
		goto done;
	}

	pkey = pte_flags_pkey(ptep->pte);
	pr_info("ptep flags 0x%lx pkey %u\n",
	       (unsigned long)ptep->pte, pkey);

	if (pkey != ctx->pkey) {
		pr_err("invalid pkey found: %u, test_pkey: %u\n",
			pkey, ctx->pkey);
		ctx->pass = false;
		goto unmap;
	}

	if (!ctx->pks_cpu_enabled) {
		pr_err("not CPU enabled; skipping access tests...\n");
		ctx->pass = true;
		goto unmap;
	}

	for (i = 0; i < ARRAY_SIZE(pkey_test_ary); i++) {
		rc = run_access_test(ctx, &pkey_test_ary[i], ptr);

		/*  only save last error is fine */
		if (rc)
			ctx->pass = false;
	}

unmap:
	pte_unmap(ptep);
done:
	vfree(ptr);
}

static void pks_run_test(struct pks_test_ctx *ctx)
{
	ctx->pass = true;

	pr_info("\n");
	pr_info("\n");
	pr_info("     ***** BEGIN: Testing (CPU enabled : %s) *****\n",
		ctx->pks_cpu_enabled ? "TRUE" : "FALSE");

	if (ctx->pks_cpu_enabled)
		on_each_cpu(report_pkey_settings, NULL, 1);

	pr_info("           BEGIN: pkey %d Testing\n", ctx->pkey);
	test_mem_access(ctx);
	pr_info("           END: PAGE_KERNEL_PKEY Testing : %s\n",
		ctx->pass ? "PASS" : "FAIL");

	pr_info("     ***** END: Testing *****\n");
	pr_info("\n");
	pr_info("\n");
}

static ssize_t pks_read_file(struct file *file, char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	struct pks_test_ctx *ctx = file->private_data;
	char buf[32];
	unsigned int len;

	if (!ctx)
		len = sprintf(buf, "not run\n");
	else
		len = sprintf(buf, "%s\n", ctx->pass ? "PASS" : "FAIL");

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static struct pks_test_ctx *alloc_ctx(const char *name)
{
	struct pks_test_ctx *ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);

	if (!ctx) {
		pr_err("Failed to allocate memory for test context\n");
		return ERR_PTR(-ENOMEM);
	}

	ctx->pkey = pks_key_alloc(name);
	if (ctx->pkey <= 0) {
		pr_err("Failed to allocate memory for test context\n");
		kfree(ctx);
		return ERR_PTR(-ENOMEM);
	}

	ctx->pks_cpu_enabled = cpu_feature_enabled(X86_FEATURE_PKS);
	sprintf(ctx->data, "%s", "DEADBEEF");
	return ctx;
}

static void free_ctx(struct pks_test_ctx *ctx)
{
	pks_key_free(ctx->pkey);
	kfree(ctx);
}

static void run_all(void)
{
	struct pks_test_ctx *ctx[PKS_NUM_KEYS];
	static char name[PKS_NUM_KEYS][64];
	int i;

	for (i = 1; i < PKS_NUM_KEYS; i++) {
		sprintf(name[i], "pks ctx %d", i);
		ctx[i] = alloc_ctx((const char *)name[i]);
	}

	for (i = 1; i < PKS_NUM_KEYS; i++) {
		if (!IS_ERR(ctx[i]))
			pks_run_test(ctx[i]);
	}

	for (i = 1; i < PKS_NUM_KEYS; i++) {
		if (!IS_ERR(ctx[i]))
			free_ctx(ctx[i]);
	}
}

static ssize_t pks_write_file(struct file *file, const char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	char buf[2];
	struct pks_test_ctx *ctx = file->private_data;

	if (copy_from_user(buf, user_buf, 1))
	        return -EFAULT;
	buf[1] = '\0';

	/*
	 * Test "3" will test allocating all keys. Do it first without
	 * using "ctx".
	 */
	if (!strcmp(buf, "3"))
		run_all();

	if (!ctx) {
		ctx = alloc_ctx("pks test");
		if (IS_ERR(ctx))
			return -ENOMEM;
		file->private_data = ctx;
	}

	if (!strcmp(buf, "0"))
		pks_run_test(ctx);

	/* start of context switch test */
	if (!strcmp(buf, "1")) {
		/* Ensure a known state to test context switch */
		update_local_sup_key(ctx->pkey,
				     PKEY_DISABLE_ACCESS | PKEY_DISABLE_WRITE);
	}

	/* After context switch msr should be restored */
	if (!strcmp(buf, "2") && ctx->pks_cpu_enabled) {
		unsigned long reg_pkrs;
		int access;

		rdmsrl(MSR_IA32_PKRS, reg_pkrs);

		access = (reg_pkrs >> (ctx->pkey * PKRU_BITS_PER_PKEY)) &
			  PKEY_ACCESS_MASK;
		if (access != (PKEY_DISABLE_ACCESS | PKEY_DISABLE_WRITE)) {
			ctx->pass = false;
			pr_err("Context switch check failed\n");
		}
	}

	return count;
}

static int pks_release_file (struct inode *inode, struct file *file)
{
	struct pks_test_ctx *ctx = file->private_data;

	if (!ctx)
		return 0;

	free_ctx(ctx);
	return 0;
}

static const struct file_operations fops_init_pks = {
	.read = pks_read_file,
	.write = pks_write_file,
	.llseek = default_llseek,
	.release = pks_release_file,
};

static int __init parse_pks_test_options(char *str)
{
	run_on_boot = true;

	return 0;
}
early_param("pks-test-on-boot", parse_pks_test_options);

static int __init pks_test_init(void)
{
	if (run_on_boot)
		run_all();

	debugfs_create_file("run_pks", S_IRUSR | S_IWUSR,
			arch_debugfs_dir, NULL, &fops_init_pks);

	return 0;
}
late_initcall(pks_test_init);

static void __exit pks_test_exit(void)
{
	pr_info("test exit\n");
}
module_exit(pks_test_exit);

MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("GPL v2");
