// SPDX-License-Identifier: GPL-2.0
/*
 * Simple zone file system for zoned block devices.
 *
 * Copyright (C) 2019 Western Digital Corporation or its affiliates.
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/magic.h>
#include <linux/iomap.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/statfs.h>
#include <linux/writeback.h>
#include <linux/quotaops.h>
#include <linux/seq_file.h>
#include <linux/parser.h>
#include <linux/uio.h>
#include <linux/mman.h>
#include <linux/sched/mm.h>
#include <linux/crc32.h>

#include "zonefs.h"

static int zonefs_iomap_begin(struct inode *inode, loff_t offset, loff_t length,
			      unsigned int flags, struct iomap *iomap,
			      struct iomap *srcmap)
{
	struct zonefs_sb_info *sbi = ZONEFS_SB(inode->i_sb);
	struct zonefs_inode_info *zi = ZONEFS_I(inode);
	loff_t max_isize = zi->i_max_size;
	loff_t isize;

	/*
	 * For sequential zones, enforce direct IO writes. This is already
	 * checked when writes are issued, so warn about this here if we
	 * get buffered write to a sequential file inode.
	 */
	if (WARN_ON_ONCE(zi->i_ztype == ZONEFS_ZTYPE_SEQ &&
			 (flags & IOMAP_WRITE) && !(flags & IOMAP_DIRECT)))
		return -EIO;

	/*
	 * For all zones, all blocks are always mapped. For sequential zones,
	 * all blocks after the write pointer (inode size) are always unwritten.
	 */
	mutex_lock(&zi->i_truncate_mutex);
	isize = i_size_read(inode);
	if (offset >= isize) {
		length = min(length, max_isize - offset);
		if (zi->i_ztype == ZONEFS_ZTYPE_CNV)
			iomap->type = IOMAP_MAPPED;
		else
			iomap->type = IOMAP_UNWRITTEN;
	} else {
		length = min(length, isize - offset);
		iomap->type = IOMAP_MAPPED;
	}
	mutex_unlock(&zi->i_truncate_mutex);

	iomap->offset = offset & (~sbi->s_blocksize_mask);
	iomap->length = ((offset + length + sbi->s_blocksize_mask) &
			 (~sbi->s_blocksize_mask)) - iomap->offset;
	iomap->bdev = inode->i_sb->s_bdev;
	iomap->addr = (zi->i_zsector << SECTOR_SHIFT) + iomap->offset;

	return 0;
}

static const struct iomap_ops zonefs_iomap_ops = {
	.iomap_begin	= zonefs_iomap_begin,
};

static int zonefs_readpage(struct file *unused, struct page *page)
{
	return iomap_readpage(page, &zonefs_iomap_ops);
}

static int zonefs_readpages(struct file *unused, struct address_space *mapping,
			    struct list_head *pages, unsigned int nr_pages)
{
	return iomap_readpages(mapping, pages, nr_pages, &zonefs_iomap_ops);
}

static int zonefs_map_blocks(struct iomap_writepage_ctx *wpc,
			     struct inode *inode, loff_t offset)
{
	if (offset >= wpc->iomap.offset &&
	    offset < wpc->iomap.offset + wpc->iomap.length)
		return 0;

	memset(&wpc->iomap, 0, sizeof(wpc->iomap));
	return zonefs_iomap_begin(inode, offset, ZONEFS_I(inode)->i_max_size,
				  0, &wpc->iomap, NULL);
}

static const struct iomap_writeback_ops zonefs_writeback_ops = {
	.map_blocks		= zonefs_map_blocks,
};

static int zonefs_writepage(struct page *page, struct writeback_control *wbc)
{
	struct iomap_writepage_ctx wpc = { };

	return iomap_writepage(page, wbc, &wpc, &zonefs_writeback_ops);
}

static int zonefs_writepages(struct address_space *mapping,
			     struct writeback_control *wbc)
{
	struct iomap_writepage_ctx wpc = { };

	return iomap_writepages(mapping, wbc, &wpc, &zonefs_writeback_ops);
}

static const struct address_space_operations zonefs_file_aops = {
	.readpage		= zonefs_readpage,
	.readpages		= zonefs_readpages,
	.writepage		= zonefs_writepage,
	.writepages		= zonefs_writepages,
	.set_page_dirty		= iomap_set_page_dirty,
	.releasepage		= iomap_releasepage,
	.invalidatepage		= iomap_invalidatepage,
	.migratepage		= iomap_migrate_page,
	.is_partially_uptodate  = iomap_is_partially_uptodate,
	.error_remove_page	= generic_error_remove_page,
	.direct_IO		= noop_direct_IO,
};

static void zonefs_update_stats(struct inode *inode, loff_t new_isize)
{
	struct super_block *sb = inode->i_sb;
	struct zonefs_sb_info *sbi = ZONEFS_SB(sb);
	loff_t old_isize = i_size_read(inode);

	if (new_isize == old_isize)
		return;

	spin_lock(&sbi->s_lock);

	if (!new_isize) {
		/* File truncated to 0 */
		sbi->s_used_blocks -= old_isize >> sb->s_blocksize_bits;
	} else if (new_isize > old_isize) {
		/* File written or truncated to max size */
		sbi->s_used_blocks +=
			(new_isize - old_isize) >> sb->s_blocksize_bits;
	} else {
		/* Sequential zone files can only grow or be truncated to 0 */
		WARN_ON(new_isize < old_isize);
	}

	spin_unlock(&sbi->s_lock);
}

static int zonefs_seq_file_truncate(struct inode *inode, loff_t isize)
{
	struct zonefs_inode_info *zi = ZONEFS_I(inode);
	loff_t old_isize;
	enum req_opf op;
	int ret = 0;

	/*
	 * For sequential zone files, we can only allow truncating to 0 size,
	 * which is equivalent to a zone reset, or to the maximum file size,
	 * which is equivalent to a zone finish.
	 */
	if (!isize)
		op = REQ_OP_ZONE_RESET;
	else if (isize == zi->i_max_size)
		op = REQ_OP_ZONE_FINISH;
	else
		return -EPERM;

	inode_dio_wait(inode);

	/* Serialize against page faults */
	down_write(&zi->i_mmap_sem);

	/* Serialize against zonefs_iomap_begin() */
	mutex_lock(&zi->i_truncate_mutex);

	old_isize = i_size_read(inode);
	if (isize == old_isize)
		goto unlock;

	ret = blkdev_zone_mgmt(inode->i_sb->s_bdev, op, zi->i_zsector,
			       zi->i_max_size >> SECTOR_SHIFT, GFP_NOFS);
	if (ret) {
		zonefs_err(inode->i_sb,
			   "Zone management operation at %llu failed %d",
			   zi->i_zsector, ret);
		goto unlock;
	}

	zonefs_update_stats(inode, isize);
	truncate_setsize(inode, isize);
	zi->i_wpoffset = isize;

unlock:
	mutex_unlock(&zi->i_truncate_mutex);
	up_write(&zi->i_mmap_sem);

	return ret;
}

static int zonefs_inode_setattr(struct dentry *dentry, struct iattr *iattr)
{
	struct inode *inode = d_inode(dentry);
	int ret;

	ret = setattr_prepare(dentry, iattr);
	if (ret)
		return ret;

	/* Files and sub-directories cannot be created or deleted */
	if ((iattr->ia_valid & ATTR_MODE) && (inode->i_mode & S_IFDIR) &&
	    (iattr->ia_mode & 0222))
		return -EPERM;

	if (((iattr->ia_valid & ATTR_UID) &&
	     !uid_eq(iattr->ia_uid, inode->i_uid)) ||
	    ((iattr->ia_valid & ATTR_GID) &&
	     !gid_eq(iattr->ia_gid, inode->i_gid))) {
		ret = dquot_transfer(inode, iattr);
		if (ret)
			return ret;
	}

	if (iattr->ia_valid & ATTR_SIZE) {
		/* The size of conventional zone files cannot be changed */
		if (ZONEFS_I(inode)->i_ztype == ZONEFS_ZTYPE_CNV)
			return -EPERM;

		ret = zonefs_seq_file_truncate(inode, iattr->ia_size);
		if (ret)
			return ret;
	}

	setattr_copy(inode, iattr);

	return 0;
}

static const struct inode_operations zonefs_file_inode_operations = {
	.setattr	= zonefs_inode_setattr,
};

static int zonefs_file_fsync(struct file *file, loff_t start, loff_t end,
			     int datasync)
{
	struct inode *inode = file_inode(file);
	int ret = 0;

	/*
	 * Since only direct writes are allowed in sequential files, page cache
	 * flush is needed only for conventional zone files.
	 */
	if (ZONEFS_I(inode)->i_ztype == ZONEFS_ZTYPE_CNV) {
		ret = file_write_and_wait_range(file, start, end);
		if (ret)
			return ret;
		ret = file_check_and_advance_wb_err(file);
	}

	if (ret == 0)
		ret = blkdev_issue_flush(inode->i_sb->s_bdev, GFP_KERNEL, NULL);

	return ret;
}

static vm_fault_t zonefs_filemap_fault(struct vm_fault *vmf)
{
	struct zonefs_inode_info *zi = ZONEFS_I(file_inode(vmf->vma->vm_file));
	vm_fault_t ret;

	down_read(&zi->i_mmap_sem);
	ret = filemap_fault(vmf);
	up_read(&zi->i_mmap_sem);

	return ret;
}

static vm_fault_t zonefs_filemap_page_mkwrite(struct vm_fault *vmf)
{
	struct inode *inode = file_inode(vmf->vma->vm_file);
	struct zonefs_inode_info *zi = ZONEFS_I(inode);
	vm_fault_t ret;

	sb_start_pagefault(inode->i_sb);
	file_update_time(vmf->vma->vm_file);

	/* Serialize against truncates */
	down_read(&zi->i_mmap_sem);
	ret = iomap_page_mkwrite(vmf, &zonefs_iomap_ops);
	up_read(&zi->i_mmap_sem);

	sb_end_pagefault(inode->i_sb);
	return ret;
}

static const struct vm_operations_struct zonefs_file_vm_ops = {
	.fault		= zonefs_filemap_fault,
	.map_pages	= filemap_map_pages,
	.page_mkwrite	= zonefs_filemap_page_mkwrite,
};

static int zonefs_file_mmap(struct file *file, struct vm_area_struct *vma)
{
	/*
	 * Since conventional zones accept random writes, conventioanl zone
	 * files can support shared writeable mappings. For sequential zone
	 * files, only readonly mappings are possible since there no gurantees
	 * for write ordering due to msync() and page cache writeback.
	 */
	if (ZONEFS_I(file_inode(file))->i_ztype == ZONEFS_ZTYPE_SEQ &&
	    (vma->vm_flags & VM_SHARED) && (vma->vm_flags & VM_MAYWRITE))
		return -EINVAL;

	file_accessed(file);
	vma->vm_ops = &zonefs_file_vm_ops;

	return 0;
}

static loff_t zonefs_file_llseek(struct file *file, loff_t offset, int whence)
{
	loff_t isize = i_size_read(file_inode(file));

	/*
	 * Seeks are limited to below the zone size for conventional zones
	 * and below the zone write pointer for sequential zones. In both
	 * cases, this limit is the inode size.
	 */
	return generic_file_llseek_size(file, offset, whence, isize, isize);
}

static ssize_t zonefs_file_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
	struct inode *inode = file_inode(iocb->ki_filp);
	struct zonefs_sb_info *sbi = ZONEFS_SB(inode->i_sb);
	struct zonefs_inode_info *zi = ZONEFS_I(inode);
	loff_t max_pos;
	size_t count;
	ssize_t ret;

	if (iocb->ki_pos >= zi->i_max_size)
		return 0;

	if (iocb->ki_flags & IOCB_NOWAIT) {
		if (!inode_trylock_shared(inode))
			return -EAGAIN;
	} else {
		inode_lock_shared(inode);
	}

	mutex_lock(&zi->i_truncate_mutex);

	/*
	 * Limit read operations to written data.
	 */
	max_pos = i_size_read(inode);
	if (iocb->ki_pos >= max_pos) {
		mutex_unlock(&zi->i_truncate_mutex);
		ret = 0;
		goto out;
	}

	iov_iter_truncate(to, max_pos - iocb->ki_pos);

	mutex_unlock(&zi->i_truncate_mutex);

	count = iov_iter_count(to);

	if (iocb->ki_flags & IOCB_DIRECT) {
		if ((iocb->ki_pos | count) & sbi->s_blocksize_mask) {
			ret = -EINVAL;
			goto out;
		}
		file_accessed(iocb->ki_filp);
		ret = iomap_dio_rw(iocb, to, &zonefs_iomap_ops, NULL,
				   is_sync_kiocb(iocb));
	} else {
		ret = generic_file_read_iter(iocb, to);
	}

out:
	inode_unlock_shared(inode);

	return ret;
}

static int zonefs_report_zones_err_cb(struct blk_zone *zone, unsigned int idx,
				      void *data)
{
	struct inode *inode = data;
	struct zonefs_inode_info *zi = ZONEFS_I(inode);
	loff_t pos;

	/*
	 * The condition of the zone may have change. Check it and adjust the
	 * inode information as needed, similarly to zonefs_init_file_inode().
	 */
	if (zone->cond == BLK_ZONE_COND_OFFLINE) {
		inode->i_flags |= S_IMMUTABLE;
		inode->i_mode &= ~0777;
		zone->wp = zone->start;
	} else if (zone->cond == BLK_ZONE_COND_READONLY) {
		inode->i_flags |= S_IMMUTABLE;
		inode->i_mode &= ~0222;
	}

	pos = (zone->wp - zone->start) << SECTOR_SHIFT;
	zi->i_wpoffset = pos;
	if (i_size_read(inode) != pos) {
		zonefs_update_stats(inode, pos);
		i_size_write(inode, pos);
	}

	return 0;
}

/*
 * When a write error occurs in a sequential zone, the zone write pointer
 * position must be refreshed to correct the file size and zonefs inode
 * write pointer offset.
 */
static int zonefs_seq_file_write_failed(struct inode *inode, int error)
{
	struct zonefs_inode_info *zi = ZONEFS_I(inode);
	struct super_block *sb = inode->i_sb;
	sector_t sector = zi->i_zsector;
	unsigned int nofs_flag;
	int ret;

	zonefs_warn(sb, "Updating inode zone %llu info\n", sector);

	/*
	 * blkdev_report_zones() uses GFP_KERNEL by default. Force execution as
	 * if GFP_NOFS was specified so that it will not end up recursing into
	 * the FS on memory allocation.
	 */
	nofs_flag = memalloc_nofs_save();
	ret = blkdev_report_zones(sb->s_bdev, sector, 1,
				  zonefs_report_zones_err_cb, inode);
	memalloc_nofs_restore(nofs_flag);

	if (ret != 1) {
		if (!ret)
			ret = -EIO;
		zonefs_err(sb, "Get zone %llu report failed %d\n",
			   sector, ret);
		return ret;
	}

	return 0;
}

static int zonefs_file_dio_write_end(struct kiocb *iocb, ssize_t size, int ret,
				     unsigned int flags)
{
	struct inode *inode = file_inode(iocb->ki_filp);
	struct zonefs_inode_info *zi = ZONEFS_I(inode);

	if (ret)
		return ret;

	/*
	 * Conventional zone file size is fixed to the zone size so there
	 * is no need to do anything.
	 */
	if (zi->i_ztype == ZONEFS_ZTYPE_CNV)
		return 0;

	mutex_lock(&zi->i_truncate_mutex);

	if (size < 0) {
		ret = zonefs_seq_file_write_failed(inode, size);
	} else if (i_size_read(inode) < iocb->ki_pos + size) {
		zonefs_update_stats(inode, iocb->ki_pos + size);
		i_size_write(inode, iocb->ki_pos + size);
	}

	mutex_unlock(&zi->i_truncate_mutex);

	return ret;
}

static const struct iomap_dio_ops zonefs_dio_ops = {
	.end_io			= zonefs_file_dio_write_end,
};

static ssize_t zonefs_file_dio_write(struct kiocb *iocb, struct iov_iter *from)
{
	struct inode *inode = file_inode(iocb->ki_filp);
	struct zonefs_sb_info *sbi = ZONEFS_SB(inode->i_sb);
	struct zonefs_inode_info *zi = ZONEFS_I(inode);
	size_t count;
	ssize_t ret;

	if (iocb->ki_flags & IOCB_NOWAIT) {
		if (!inode_trylock(inode))
			return -EAGAIN;
	} else {
		inode_lock(inode);
	}

	ret = generic_write_checks(iocb, from);
	if (ret <= 0)
		goto out;

	iov_iter_truncate(from, zi->i_max_size - iocb->ki_pos);
	count = iov_iter_count(from);

	/*
	 * Direct writes must be aligned to the block size, that is, the device
	 * physical sector size, to avoid errors when writing sequential zones
	 * on 512e devices (512B logical sector, 4KB physical sectors).
	 */
	if ((iocb->ki_pos | count) & sbi->s_blocksize_mask) {
		ret = -EINVAL;
		goto out;
	}

	/*
	 * Enforce sequential writes (append only) in sequential zones.
	 */
	mutex_lock(&zi->i_truncate_mutex);
	if (zi->i_ztype == ZONEFS_ZTYPE_SEQ &&
	    iocb->ki_pos != zi->i_wpoffset) {
		zonefs_err(inode->i_sb,
			   "Unaligned write at %llu + %zu (wp %llu)\n",
			   iocb->ki_pos, count,
			   zi->i_wpoffset);
		mutex_unlock(&zi->i_truncate_mutex);
		ret = -EINVAL;
		goto out;
	}
	mutex_unlock(&zi->i_truncate_mutex);

	ret = iomap_dio_rw(iocb, from, &zonefs_iomap_ops, &zonefs_dio_ops,
			   is_sync_kiocb(iocb));
	if (zi->i_ztype == ZONEFS_ZTYPE_SEQ &&
	    (ret > 0 || ret == -EIOCBQUEUED)) {
		if (ret > 0)
			count = ret;
		mutex_lock(&zi->i_truncate_mutex);
		zi->i_wpoffset += count;
		mutex_unlock(&zi->i_truncate_mutex);
	}

out:
	inode_unlock(inode);

	return ret;
}

static ssize_t zonefs_file_buffered_write(struct kiocb *iocb,
					  struct iov_iter *from)
{
	struct inode *inode = file_inode(iocb->ki_filp);
	struct zonefs_inode_info *zi = ZONEFS_I(inode);
	size_t count;
	ssize_t ret;

	/*
	 * Direct IO writes are mandatory for sequential zones so that the
	 * write IO order is preserved.
	 */
	if (zi->i_ztype == ZONEFS_ZTYPE_SEQ)
		return -EIO;

	if (iocb->ki_flags & IOCB_NOWAIT) {
		if (!inode_trylock(inode))
			return -EAGAIN;
	} else {
		inode_lock(inode);
	}

	ret = generic_write_checks(iocb, from);
	if (ret <= 0)
		goto out;

	iov_iter_truncate(from, zi->i_max_size - iocb->ki_pos);
	count = iov_iter_count(from);

	ret = iomap_file_buffered_write(iocb, from, &zonefs_iomap_ops);
	if (ret > 0)
		iocb->ki_pos += ret;

out:
	inode_unlock(inode);
	if (ret > 0)
		ret = generic_write_sync(iocb, ret);

	return ret;
}

static ssize_t zonefs_file_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
	struct inode *inode = file_inode(iocb->ki_filp);

	/*
	 * Check that the write operation does not go beyond the zone size.
	 */
	if (iocb->ki_pos >= ZONEFS_I(inode)->i_max_size)
		return -EFBIG;

	if (iocb->ki_flags & IOCB_DIRECT)
		return zonefs_file_dio_write(iocb, from);

	return zonefs_file_buffered_write(iocb, from);
}

static const struct file_operations zonefs_file_operations = {
	.open		= generic_file_open,
	.fsync		= zonefs_file_fsync,
	.mmap		= zonefs_file_mmap,
	.llseek		= zonefs_file_llseek,
	.read_iter	= zonefs_file_read_iter,
	.write_iter	= zonefs_file_write_iter,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.iopoll		= iomap_dio_iopoll,
};

static struct kmem_cache *zonefs_inode_cachep;

static struct inode *zonefs_alloc_inode(struct super_block *sb)
{
	struct zonefs_inode_info *zi;

	zi = kmem_cache_alloc(zonefs_inode_cachep, GFP_KERNEL);
	if (!zi)
		return NULL;

	inode_init_once(&zi->i_vnode);
	mutex_init(&zi->i_truncate_mutex);
	init_rwsem(&zi->i_mmap_sem);

	return &zi->i_vnode;
}

static void zonefs_free_inode(struct inode *inode)
{
	kmem_cache_free(zonefs_inode_cachep, ZONEFS_I(inode));
}

/*
 * File system stat.
 */
static int zonefs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	struct super_block *sb = dentry->d_sb;
	struct zonefs_sb_info *sbi = ZONEFS_SB(sb);
	enum zonefs_ztype t;
	u64 fsid;

	buf->f_type = ZONEFS_MAGIC;
	buf->f_bsize = sb->s_blocksize;
	buf->f_namelen = ZONEFS_NAME_MAX;

	spin_lock(&sbi->s_lock);

	buf->f_blocks = sbi->s_blocks;
	if (WARN_ON(sbi->s_used_blocks > sbi->s_blocks))
		buf->f_bfree = 0;
	else
		buf->f_bfree = buf->f_blocks - sbi->s_used_blocks;
	buf->f_bavail = buf->f_bfree;

	for (t = 0; t < ZONEFS_ZTYPE_MAX; t++) {
		if (sbi->s_nr_files[t])
			buf->f_files += sbi->s_nr_files[t] + 1;
	}
	buf->f_ffree = 0;

	spin_unlock(&sbi->s_lock);

	fsid = le64_to_cpup((void *)sbi->s_uuid.b) ^
		le64_to_cpup((void *)sbi->s_uuid.b + sizeof(u64));
	buf->f_fsid.val[0] = (u32)fsid;
	buf->f_fsid.val[1] = (u32)(fsid >> 32);

	return 0;
}

static const struct super_operations zonefs_sops = {
	.alloc_inode	= zonefs_alloc_inode,
	.free_inode	= zonefs_free_inode,
	.statfs		= zonefs_statfs,
};

static const struct inode_operations zonefs_dir_inode_operations = {
	.lookup		= simple_lookup,
	.setattr	= zonefs_inode_setattr,
};

static void zonefs_init_dir_inode(struct inode *parent, struct inode *inode)
{
	inode_init_owner(inode, parent, S_IFDIR | 0555);
	inode->i_op = &zonefs_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;
	set_nlink(inode, 2);
	inc_nlink(parent);
}

static void zonefs_init_file_inode(struct inode *inode, struct blk_zone *zone)
{
	struct super_block *sb = inode->i_sb;
	struct zonefs_sb_info *sbi = ZONEFS_SB(sb);
	struct zonefs_inode_info *zi = ZONEFS_I(inode);
	umode_t	perm = sbi->s_perm;

	if (zone->cond == BLK_ZONE_COND_OFFLINE) {
		/*
		 * Dead zone: make the inode immutable, disable all accesses
		 * and set the file size to 0.
		 */
		inode->i_flags |= S_IMMUTABLE;
		zone->wp = zone->start;
		perm &= ~0777;
	} else if (zone->cond == BLK_ZONE_COND_READONLY) {
		/* Do not allow writes in read-only zones */
		inode->i_flags |= S_IMMUTABLE;
		perm &= ~0222;
	}

	zi->i_ztype = zonefs_zone_type(zone);
	zi->i_zsector = zone->start;
	zi->i_max_size = min_t(loff_t, MAX_LFS_FILESIZE,
			       zone->len << SECTOR_SHIFT);
	if (zi->i_ztype == ZONEFS_ZTYPE_CNV)
		zi->i_wpoffset = zi->i_max_size;
	else
		zi->i_wpoffset = (zone->wp - zone->start) << SECTOR_SHIFT;

	inode->i_mode = S_IFREG | perm;
	inode->i_uid = sbi->s_uid;
	inode->i_gid = sbi->s_gid;
	inode->i_size = zi->i_wpoffset;
	inode->i_blocks = zone->len;

	inode->i_op = &zonefs_file_inode_operations;
	inode->i_fop = &zonefs_file_operations;
	inode->i_mapping->a_ops = &zonefs_file_aops;

	sb->s_maxbytes = max(zi->i_max_size, sb->s_maxbytes);
	sbi->s_blocks += zi->i_max_size >> sb->s_blocksize_bits;
	sbi->s_used_blocks += zi->i_wpoffset >> sb->s_blocksize_bits;
}

static struct dentry *zonefs_create_inode(struct dentry *parent,
					const char *name, struct blk_zone *zone)
{
	struct inode *dir = d_inode(parent);
	struct dentry *dentry;
	struct inode *inode;

	dentry = d_alloc_name(parent, name);
	if (!dentry)
		return NULL;

	inode = new_inode(parent->d_sb);
	if (!inode)
		goto out;

	inode->i_ino = get_next_ino();
	inode->i_ctime = inode->i_mtime = inode->i_atime = dir->i_ctime;
	if (zone)
		zonefs_init_file_inode(inode, zone);
	else
		zonefs_init_dir_inode(dir, inode);
	d_add(dentry, inode);
	dir->i_size++;

	return dentry;

out:
	dput(dentry);

	return NULL;
}

static char *zgroups_name[ZONEFS_ZTYPE_MAX] = { "cnv", "seq" };

struct zonefs_zone_data {
	struct super_block *sb;
	unsigned int nr_zones[ZONEFS_ZTYPE_MAX];
	struct blk_zone *zones;
};

/*
 * Create a zone group and populate it with zone files.
 */
static int zonefs_create_zgroup(struct zonefs_zone_data *zd,
				enum zonefs_ztype type)
{
	struct super_block *sb = zd->sb;
	struct zonefs_sb_info *sbi = ZONEFS_SB(sb);
	struct blk_zone *zone, *next, *end;
	char name[ZONEFS_NAME_MAX];
	struct dentry *dir;
	unsigned int n = 0;

	/* If the group is empty, there is nothing to do */
	if (!zd->nr_zones[type])
		return 0;

	dir = zonefs_create_inode(sb->s_root, zgroups_name[type], NULL);
	if (!dir)
		return -ENOMEM;

	/*
	 * The first zone contains the super block: skip it.
	 */
	end = zd->zones + blkdev_nr_zones(sb->s_bdev->bd_disk);
	for (zone = &zd->zones[1]; zone < end; zone = next) {

		next = zone + 1;
		if (zonefs_zone_type(zone) != type)
			continue;

		/*
		 * For conventional zones, contiguous zones can be aggregated
		 * together to form larger files.
		 * Note that this overwrites the length of the first zone of
		 * the set of contiguous zones aggregated together.
		 * Only zones with the same condition can be agreggated so that
		 * offline zones are excluded and readonly zones are aggregated
		 * together into a read only file.
		 */
		if (type == ZONEFS_ZTYPE_CNV &&
		    (sbi->s_features & ZONEFS_F_AGGRCNV)) {
			for (; next < end; next++) {
				if (zonefs_zone_type(next) != type ||
				    next->cond != zone->cond)
					break;
				zone->len += next->len;
			}
		}

		/*
		 * Use the file number within its group as file name.
		 */
		snprintf(name, ZONEFS_NAME_MAX - 1, "%u", n);
		if (!zonefs_create_inode(dir, name, zone))
			return -ENOMEM;

		n++;
	}

	zonefs_info(sb, "Zone group \"%s\" has %u file%s\n",
		    zgroups_name[type], n, n > 1 ? "s" : "");

	sbi->s_nr_files[type] = n;

	return 0;
}

static int zonefs_get_zone_info_cb(struct blk_zone *zone, unsigned int idx,
				   void *data)
{
	struct zonefs_zone_data *zd = data;

	/*
	 * Count the number of usable zones: the first zone at index 0 contains
	 * the super block and is ignored.
	 */
	switch (zone->type) {
	case BLK_ZONE_TYPE_CONVENTIONAL:
		zone->wp = zone->start + zone->len;
		if (idx)
			zd->nr_zones[ZONEFS_ZTYPE_CNV]++;
		break;
	case BLK_ZONE_TYPE_SEQWRITE_REQ:
	case BLK_ZONE_TYPE_SEQWRITE_PREF:
		if (idx)
			zd->nr_zones[ZONEFS_ZTYPE_SEQ]++;
		break;
	default:
		zonefs_err(zd->sb, "Unsupported zone type 0x%x\n",
			   zone->type);
		return -EIO;
	}

	memcpy(&zd->zones[idx], zone, sizeof(struct blk_zone));

	return 0;
}

static int zonefs_get_zone_info(struct zonefs_zone_data *zd)
{
	struct block_device *bdev = zd->sb->s_bdev;
	int ret;

	zd->zones = kvcalloc(blkdev_nr_zones(bdev->bd_disk),
			     sizeof(struct blk_zone), GFP_KERNEL);
	if (!zd->zones)
		return -ENOMEM;

	/* Get zones information */
	ret = blkdev_report_zones(bdev, 0, BLK_ALL_ZONES,
				  zonefs_get_zone_info_cb, zd);
	if (ret < 0) {
		zonefs_err(zd->sb, "Zone report failed %d\n", ret);
		return ret;
	}

	if (ret != blkdev_nr_zones(bdev->bd_disk)) {
		zonefs_err(zd->sb, "Invalid zone report (%d/%u zones)\n",
			   ret, blkdev_nr_zones(bdev->bd_disk));
		return -EIO;
	}

	return 0;
}

static inline void zonefs_cleanup_zone_info(struct zonefs_zone_data *zd)
{
	kvfree(zd->zones);
}

/*
 * Read super block information from the device.
 */
static int zonefs_read_super(struct super_block *sb)
{
	struct zonefs_sb_info *sbi = ZONEFS_SB(sb);
	struct zonefs_super *super;
	u32 crc, stored_crc;
	struct page *page;
	struct bio_vec bio_vec;
	struct bio bio;
	int ret;

	page = alloc_page(GFP_KERNEL);
	if (!page)
		return -ENOMEM;

	bio_init(&bio, &bio_vec, 1);
	bio.bi_iter.bi_sector = 0;
	bio_set_dev(&bio, sb->s_bdev);
	bio_set_op_attrs(&bio, REQ_OP_READ, 0);
	bio_add_page(&bio, page, PAGE_SIZE, 0);

	ret = submit_bio_wait(&bio);
	if (ret)
		goto out;

	super = page_address(page);

	stored_crc = le32_to_cpu(super->s_crc);
	super->s_crc = 0;
	crc = crc32(~0U, (unsigned char *)super, sizeof(struct zonefs_super));
	if (crc != stored_crc) {
		zonefs_err(sb, "Invalid checksum (Expected 0x%08x, got 0x%08x)",
			   crc, stored_crc);
		ret = -EIO;
		goto out;
	}

	ret = -EINVAL;
	if (le32_to_cpu(super->s_magic) != ZONEFS_MAGIC)
		goto out;

	sbi->s_features = le64_to_cpu(super->s_features);
	if (sbi->s_features & ~ZONEFS_F_DEFINED_FEATURES) {
		zonefs_err(sb, "Unknown features set 0x%llx\n",
			   sbi->s_features);
		goto out;
	}

	if (sbi->s_features & ZONEFS_F_UID) {
		sbi->s_uid = make_kuid(current_user_ns(),
				       le32_to_cpu(super->s_uid));
		if (!uid_valid(sbi->s_uid)) {
			zonefs_err(sb, "Invalid UID feature\n");
			goto out;
		}
	}

	if (sbi->s_features & ZONEFS_F_GID) {
		sbi->s_gid = make_kgid(current_user_ns(),
				       le32_to_cpu(super->s_gid));
		if (!gid_valid(sbi->s_gid)) {
			zonefs_err(sb, "Invalid GID feature\n");
			goto out;
		}
	}

	if (sbi->s_features & ZONEFS_F_PERM)
		sbi->s_perm = le32_to_cpu(super->s_perm);

	if (memchr_inv(super->s_reserved, 0, sizeof(super->s_reserved))) {
		zonefs_err(sb, "Reserved area is being used\n");
		goto out;
	}

	uuid_copy(&sbi->s_uuid, (uuid_t *)super->s_uuid);
	ret = 0;

out:
	__free_page(page);

	return ret;
}

/*
 * Check that the device is zoned. If it is, get the list of zones and create
 * sub-directories and files according to the device zone configuration and
 * format options.
 */
static int zonefs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct zonefs_zone_data zd;
	struct zonefs_sb_info *sbi;
	struct inode *inode;
	enum zonefs_ztype t;
	int ret;

	if (!bdev_is_zoned(sb->s_bdev)) {
		zonefs_err(sb, "Not a zoned block device\n");
		return -EINVAL;
	}

	/*
	 * Initialize super block information: the maximum file size is updated
	 * when the zone files are created so that the format option
	 * ZONEFS_F_AGGRCNV which increases the maximum file size of a file
	 * beyond the zone size is taken into account.
	 */
	sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
	if (!sbi)
		return -ENOMEM;

	spin_lock_init(&sbi->s_lock);
	sb->s_fs_info = sbi;
	sb->s_magic = ZONEFS_MAGIC;
	sb->s_maxbytes = 0;
	sb->s_op = &zonefs_sops;
	sb->s_time_gran	= 1;

	/*
	 * The block size is always equal to the device physical sector size to
	 * ensure that writes on 512e devices (512B logical block and 4KB
	 * physical block) are always aligned to the device physical blocks
	 * (as required for writes to sequential zones on ZBC/ZAC disks).
	 */
	sb_set_blocksize(sb, bdev_physical_block_size(sb->s_bdev));
	sbi->s_blocksize_mask = sb->s_blocksize - 1;
	sbi->s_uid = GLOBAL_ROOT_UID;
	sbi->s_gid = GLOBAL_ROOT_GID;
	sbi->s_perm = 0640;

	ret = zonefs_read_super(sb);
	if (ret)
		return ret;

	memset(&zd, 0, sizeof(struct zonefs_zone_data));
	zd.sb = sb;
	ret = zonefs_get_zone_info(&zd);
	if (ret)
		goto out;

	zonefs_info(sb, "Mounting %u zones",
		    blkdev_nr_zones(sb->s_bdev->bd_disk));

	/* Create root directory inode */
	ret = -ENOMEM;
	inode = new_inode(sb);
	if (!inode)
		goto out;

	inode->i_ino = get_next_ino();
	inode->i_mode = S_IFDIR | 0555;
	inode->i_ctime = inode->i_mtime = inode->i_atime = current_time(inode);
	inode->i_op = &zonefs_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;
	set_nlink(inode, 2);

	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		goto out;

	/* Create and populate files in zone groups directories */
	for (t = 0; t < ZONEFS_ZTYPE_MAX; t++) {
		ret = zonefs_create_zgroup(&zd, t);
		if (ret)
			break;
	}

out:
	zonefs_cleanup_zone_info(&zd);

	return ret;
}

static struct dentry *zonefs_mount(struct file_system_type *fs_type,
				   int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, zonefs_fill_super);
}

static void zonefs_kill_super(struct super_block *sb)
{
	struct zonefs_sb_info *sbi = ZONEFS_SB(sb);

	kfree(sbi);
	if (sb->s_root)
		d_genocide(sb->s_root);
	kill_block_super(sb);
}

/*
 * File system definition and registration.
 */
static struct file_system_type zonefs_type = {
	.owner		= THIS_MODULE,
	.name		= "zonefs",
	.mount		= zonefs_mount,
	.kill_sb	= zonefs_kill_super,
	.fs_flags	= FS_REQUIRES_DEV,
};

static int __init zonefs_init_inodecache(void)
{
	zonefs_inode_cachep = kmem_cache_create("zonefs_inode_cache",
			sizeof(struct zonefs_inode_info), 0,
			(SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD | SLAB_ACCOUNT),
			NULL);
	if (zonefs_inode_cachep == NULL)
		return -ENOMEM;
	return 0;
}

static void zonefs_destroy_inodecache(void)
{
	/*
	 * Make sure all delayed rcu free inodes are flushed before we
	 * destroy the inode cache.
	 */
	rcu_barrier();
	kmem_cache_destroy(zonefs_inode_cachep);
}

static int __init zonefs_init(void)
{
	int ret;

	BUILD_BUG_ON(sizeof(struct zonefs_super) != ZONEFS_SUPER_SIZE);

	ret = zonefs_init_inodecache();
	if (ret)
		return ret;

	ret = register_filesystem(&zonefs_type);
	if (ret) {
		zonefs_destroy_inodecache();
		return ret;
	}

	return 0;
}

static void __exit zonefs_exit(void)
{
	zonefs_destroy_inodecache();
	unregister_filesystem(&zonefs_type);
}

MODULE_AUTHOR("Damien Le Moal");
MODULE_DESCRIPTION("Zone file system for zoned block devices");
MODULE_LICENSE("GPL");
module_init(zonefs_init);
module_exit(zonefs_exit);
