#ifdef CONFIG_DEBUG_FS
/*
 * Copyright (c) 2014 Intel Corporation.  All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <linux/export.h>

#include "hfi.h"
#include "verbs.h"
#include "debugfs.h"
#include "device.h"
#include "qp.h"

static struct dentry *hfi_dbg_root;

#define private2dd(file) (file_inode(file)->i_private)
#define private2ppd(file) (file_inode(file)->i_private)

#define DEBUGFS_SEQ_FILE(name) \
static const struct seq_operations _##name##_seq_ops = { \
	.start = _##name##_seq_start, \
	.next  = _##name##_seq_next, \
	.stop  = _##name##_seq_stop, \
	.show  = _##name##_seq_show \
}; \
static int _##name##_open(struct inode *inode, struct file *s) \
{ \
	struct seq_file *seq; \
	int ret; \
	ret =  seq_open(s, &_##name##_seq_ops); \
	if (ret) \
		return ret; \
	seq = s->private_data; \
	seq->private = inode->i_private; \
	return 0; \
} \
static const struct file_operations _##name##_file_ops = { \
	.owner   = THIS_MODULE, \
	.open    = _##name##_open, \
	.read    = seq_read, \
	.llseek  = seq_lseek, \
	.release = seq_release \
};

#define DEBUGFS_FILE_CREATE(name, parent, data, ops) \
do { \
	struct dentry *ent; \
	ent = debugfs_create_file(name , 0400, parent, \
		data, ops); \
	if (!ent) \
		pr_warn("create of %s failed\n", name); \
} while (0)


#define DEBUGFS_SEQ_FILE_CREATE(name, parent, data) \
	DEBUGFS_FILE_CREATE(#name, parent, data, &_##name##_file_ops)

static void *_opcode_stats_seq_start(struct seq_file *s, loff_t *pos)
{
	struct hfi_opcode_stats_perctx *opstats;

	rcu_read_lock();
	if (*pos >= ARRAY_SIZE(opstats->stats))
		return NULL;
	return pos;
}

static void *_opcode_stats_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct hfi_opcode_stats_perctx *opstats;

	++*pos;
	if (*pos >= ARRAY_SIZE(opstats->stats))
		return NULL;
	return pos;
}


static void _opcode_stats_seq_stop(struct seq_file *s, void *v)
{
	rcu_read_unlock();
}

static int _opcode_stats_seq_show(struct seq_file *s, void *v)
{
	loff_t *spos = v;
	loff_t i = *spos, j;
	u64 n_packets = 0, n_bytes = 0;
	struct qib_ibdev *ibd = (struct qib_ibdev *)s->private;
	struct hfi_devdata *dd = dd_from_dev(ibd);

	for (j = 0; j < dd->first_user_ctxt; j++) {
		if (!dd->rcd[j])
			continue;
		n_packets += dd->rcd[j]->opstats->stats[i].n_packets;
		n_bytes += dd->rcd[j]->opstats->stats[i].n_bytes;
	}
	if (!n_packets && !n_bytes)
		return SEQ_SKIP;
	seq_printf(s, "%02llx %llu/%llu\n", i,
		(unsigned long long) n_packets,
		(unsigned long long) n_bytes);

	return 0;
}

DEBUGFS_SEQ_FILE(opcode_stats)

static void *_ctx_stats_seq_start(struct seq_file *s, loff_t *pos)
{
	struct qib_ibdev *ibd = (struct qib_ibdev *)s->private;
	struct hfi_devdata *dd = dd_from_dev(ibd);

	if (!*pos)
		return SEQ_START_TOKEN;
	if (*pos >= dd->first_user_ctxt)
		return NULL;
	return pos;
}

static void *_ctx_stats_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct qib_ibdev *ibd = (struct qib_ibdev *)s->private;
	struct hfi_devdata *dd = dd_from_dev(ibd);

	if (v == SEQ_START_TOKEN)
		return pos;

	++*pos;
	if (*pos >= dd->first_user_ctxt)
		return NULL;
	return pos;
}

static void _ctx_stats_seq_stop(struct seq_file *s, void *v)
{
	/* nothing allocated */
}

static int _ctx_stats_seq_show(struct seq_file *s, void *v)
{
	loff_t *spos;
	loff_t i, j;
	u64 n_packets = 0;
	struct qib_ibdev *ibd = (struct qib_ibdev *)s->private;
	struct hfi_devdata *dd = dd_from_dev(ibd);

	if (v == SEQ_START_TOKEN) {
		seq_puts(s, "Ctx:npkts\n");
		return 0;
	}

	spos = v;
	i = *spos;

	if (!dd->rcd[i])
		return SEQ_SKIP;

	for (j = 0; j < ARRAY_SIZE(dd->rcd[i]->opstats->stats); j++)
		n_packets += dd->rcd[i]->opstats->stats[j].n_packets;

	if (!n_packets)
		return SEQ_SKIP;

	seq_printf(s, "  %llu:%llu\n", i, n_packets);
	return 0;
}

DEBUGFS_SEQ_FILE(ctx_stats)

static void *_qp_stats_seq_start(struct seq_file *s, loff_t *pos)
{
	struct qib_qp_iter *iter;
	loff_t n = *pos;

	iter = qib_qp_iter_init(s->private);
	if (!iter)
		return NULL;

	while (n--) {
		if (qib_qp_iter_next(iter)) {
			kfree(iter);
			return NULL;
		}
	}

	return iter;
}

static void *_qp_stats_seq_next(struct seq_file *s, void *iter_ptr,
				   loff_t *pos)
{
	struct qib_qp_iter *iter = iter_ptr;

	(*pos)++;

	if (qib_qp_iter_next(iter)) {
		kfree(iter);
		return NULL;
	}

	return iter;
}

static void _qp_stats_seq_stop(struct seq_file *s, void *iter_ptr)
{
	/* nothing for now */
}

static int _qp_stats_seq_show(struct seq_file *s, void *iter_ptr)
{
	struct qib_qp_iter *iter = iter_ptr;

	if (!iter)
		return 0;

	qib_qp_iter_print(s, iter);

	return 0;
}

DEBUGFS_SEQ_FILE(qp_stats)

/* read the per-device counters */
static ssize_t dev_counters_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	u64 *counters;
	size_t avail;
	struct hfi_devdata *dd;
	ssize_t rval;

	rcu_read_lock();
	dd = private2dd(file);
	avail = dd->f_read_cntrs(dd, *ppos, NULL, &counters);
	rval =  simple_read_from_buffer(buf, count, ppos, counters, avail);
	rcu_read_unlock();
	return rval;
}

/* read the per-device counters */
static ssize_t dev_names_read(struct file *file, char __user *buf,
			      size_t count, loff_t *ppos)
{
	char *names;
	size_t avail;
	struct hfi_devdata *dd;
	ssize_t rval;

	rcu_read_lock();
	dd = private2dd(file);
	avail = dd->f_read_cntrs(dd, *ppos, &names, NULL);
	rval =  simple_read_from_buffer(buf, count, ppos, names, avail);
	rcu_read_unlock();
	return rval;
}

struct counter_info {
	char *name;
	const struct file_operations ops;
};

/*
 * Could use file_inode(file)->i_ino to figure out which file,
 * instead of separate routine for each, but for now, this works...
 */

/* read the per-port names (same for each port) */
static ssize_t portnames_read(struct file *file, char __user *buf,
			      size_t count, loff_t *ppos)
{
	char *names;
	size_t avail;
	struct hfi_devdata *dd;
	ssize_t rval;

	rcu_read_lock();
	dd = private2dd(file);
	/* port num n/a here since names are constant */
	avail = dd->f_read_portcntrs(dd, *ppos, 0, &names, NULL);
	rval = simple_read_from_buffer(buf, count, ppos, names, avail);
	rcu_read_unlock();
	return rval;
}

/* read the per-port counters */
static ssize_t portcntrs_read(struct file *file, char __user *buf,
				size_t count, loff_t *ppos)
{
	u64 *counters;
	size_t avail;
	struct hfi_devdata *dd;
	struct qib_pportdata *ppd;
	ssize_t rval;

	rcu_read_lock();
	ppd = private2ppd(file);
	dd = ppd->dd;
	avail = dd->f_read_portcntrs(dd, *ppos, ppd->port - 1, NULL, &counters);
	rval = simple_read_from_buffer(buf, count, ppos, counters, avail);
	rcu_read_unlock();
	return rval;
}

/*
 * read the per-port QSFP data for ppd
 */
static ssize_t qsfp_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct qib_pportdata *ppd;
	char *tmp;
	int ret;

	rcu_read_lock();
	ppd = private2ppd(file);
	tmp = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	ret = qib_qsfp_dump(ppd, tmp, PAGE_SIZE);
	if (ret > 0)
		ret = simple_read_from_buffer(buf, count, ppos, tmp, ret);
	rcu_read_unlock();
	kfree(tmp);
	return ret;
}

#define DEBUGFS_OPS(nm, readroutine) \
{ \
	.name = nm, \
	.ops = { \
		.read = readroutine, \
		.llseek = generic_file_llseek, \
	}, \
}

static const struct counter_info cntr_ops[] = {
	DEBUGFS_OPS("counter_names", dev_names_read),
	DEBUGFS_OPS("counters", dev_counters_read),
	DEBUGFS_OPS("portcounter_names", portnames_read),
};

static const struct counter_info port_cntr_ops[] = {
	DEBUGFS_OPS("port%dcounters", portcntrs_read),
	DEBUGFS_OPS("qsfp%d", qsfp_read),
};

void hfi_dbg_ibdev_init(struct qib_ibdev *ibd)
{
	char name[sizeof("port0counters") + 1];
	char link[10];
	struct hfi_devdata *dd = dd_from_dev(ibd);
	struct qib_pportdata *ppd;
	int unit = dd->unit;
	int i, j;

	if (!hfi_dbg_root)
		return;
	snprintf(name, sizeof(name), "%s%d", class_name(), unit);
	snprintf(link, sizeof(link), "%d", unit);
	ibd->hfi_ibdev_dbg = debugfs_create_dir(name, hfi_dbg_root);
	if (!ibd->hfi_ibdev_dbg) {
		pr_warn("create of %s failed\n", name);
		return;
	}
	ibd->hfi_ibdev_link =
		debugfs_create_symlink(link, hfi_dbg_root, name);
	if (!ibd->hfi_ibdev_link) {
		pr_warn("create of %s symlink failed\n", name);
		return;
	}
	DEBUGFS_SEQ_FILE_CREATE(opcode_stats, ibd->hfi_ibdev_dbg, ibd);
	DEBUGFS_SEQ_FILE_CREATE(ctx_stats, ibd->hfi_ibdev_dbg, ibd);
	DEBUGFS_SEQ_FILE_CREATE(qp_stats, ibd->hfi_ibdev_dbg, ibd);
	/* dev counter files */
	for (i = 0; i < ARRAY_SIZE(cntr_ops); i++)
		DEBUGFS_FILE_CREATE(cntr_ops[i].name,
				    ibd->hfi_ibdev_dbg,
				    dd,
				    &cntr_ops[i].ops);
	/* per port files */
	for (ppd = dd->pport, j = 0; j < dd->num_pports; j++, ppd++)
		for (i = 0; i < ARRAY_SIZE(port_cntr_ops); i++) {
			snprintf(name,
				 sizeof(name),
				 port_cntr_ops[i].name,
				 j + 1);
			DEBUGFS_FILE_CREATE(name,
					    ibd->hfi_ibdev_dbg,
					    ppd,
					    &port_cntr_ops[i].ops);
		}
	return;
}

void hfi_dbg_ibdev_exit(struct qib_ibdev *ibd)
{
	if (!hfi_dbg_root)
		goto out;
	debugfs_remove(ibd->hfi_ibdev_link);
	debugfs_remove_recursive(ibd->hfi_ibdev_dbg);
out:
	ibd->hfi_ibdev_dbg = NULL;
}

/*
 * driver stats field names, one line per stat, single string.  Used by
 * programs like hfistats to print the stats in a way which works for
 * different versions of drivers, without changing program source.
 * if hfi_ib_stats changes, this needs to change.  Names need to be
 * 12 chars or less (w/o newline), for proper display by hfistats utility.
 */
static const char * const hfi_statnames[] = {
	/* must be element 0*/
	"KernIntr",
	"ErrorIntr",
	"Tx_Errs",
	"Rcv_Errs",
	"H/W_Errs",
	"NoPIOBufs",
	"CtxtsOpen",
	"RcvLen_Errs",
	"EgrBufFull",
	"EgrHdrFull"
};

static void *_driver_stats_names_seq_start(struct seq_file *s, loff_t *pos)
{
	rcu_read_lock();
	if (*pos >= ARRAY_SIZE(hfi_statnames))
		return NULL;
	return pos;
}

static void *_driver_stats_names_seq_next(
	struct seq_file *s,
	void *v,
	loff_t *pos)
{
	++*pos;
	if (*pos >= ARRAY_SIZE(hfi_statnames))
		return NULL;
	return pos;
}

static void _driver_stats_names_seq_stop(struct seq_file *s, void *v)
{
	rcu_read_unlock();
}

static int _driver_stats_names_seq_show(struct seq_file *s, void *v)
{
	loff_t *spos = v;
	seq_printf(s, "%s\n", hfi_statnames[*spos]);
	return 0;
}

DEBUGFS_SEQ_FILE(driver_stats_names)

static void *_driver_stats_seq_start(struct seq_file *s, loff_t *pos)
{
	rcu_read_lock();
	if (*pos >= ARRAY_SIZE(hfi_statnames))
		return NULL;
	return pos;
}

static void *_driver_stats_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	++*pos;
	if (*pos >= ARRAY_SIZE(hfi_statnames))
		return NULL;
	return pos;
}

static void _driver_stats_seq_stop(struct seq_file *s, void *v)
{
	rcu_read_unlock();
}

static u64 hfi_sps_ints(void)
{
	unsigned long flags;
	struct hfi_devdata *dd;
	u64 sps_ints = 0;

	spin_lock_irqsave(&qib_devs_lock, flags);
	list_for_each_entry(dd, &qib_dev_list, list) {
		sps_ints += hfi_int_counter(dd);
	}
	spin_unlock_irqrestore(&qib_devs_lock, flags);
	return sps_ints;
}

static int _driver_stats_seq_show(struct seq_file *s, void *v)
{
	loff_t *spos = v;
	char *buffer;
	u64 *stats = (u64 *)&qib_stats;
	size_t sz = seq_get_buf(s, &buffer);

	if (sz < sizeof(u64))
		return SEQ_SKIP;
	/* special case for interrupts */
	if (*spos == 0)
		*(u64 *)buffer = hfi_sps_ints();
	else
		*(u64 *)buffer = stats[*spos];
	seq_commit(s,  sizeof(u64));
	return 0;
}

DEBUGFS_SEQ_FILE(driver_stats)

void hfi_dbg_init(void)
{
	hfi_dbg_root  = debugfs_create_dir(DRIVER_NAME, NULL);
	if (!hfi_dbg_root)
		pr_warn("init of debugfs failed\n");
	DEBUGFS_SEQ_FILE_CREATE(driver_stats_names, hfi_dbg_root, NULL);
	DEBUGFS_SEQ_FILE_CREATE(driver_stats, hfi_dbg_root, NULL);
}

void hfi_dbg_exit(void)
{
	debugfs_remove_recursive(hfi_dbg_root);
	hfi_dbg_root = NULL;
}

#endif
