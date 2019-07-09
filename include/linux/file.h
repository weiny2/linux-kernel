/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Wrapper functions for accessing the file_struct fd array.
 */

#ifndef __LINUX_FILE_H
#define __LINUX_FILE_H

#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/posix_types.h>
#include <linux/kref.h>

struct file;

extern void fput(struct file *);
extern void fput_many(struct file *, unsigned int);

struct file_operations;
struct vfsmount;
struct dentry;
struct inode;
struct path;
extern struct file *alloc_file_pseudo(struct inode *, struct vfsmount *,
	const char *, int flags, const struct file_operations *);
extern struct file *alloc_file_clone(struct file *, int flags,
	const struct file_operations *);

static inline void fput_light(struct file *file, int fput_needed)
{
	if (fput_needed)
		fput(file);
}

struct fd {
	struct file *file;
	unsigned int flags;
};
#define FDPUT_FPUT       1
#define FDPUT_POS_UNLOCK 2

static inline void fdput(struct fd fd)
{
	if (fd.flags & FDPUT_FPUT)
		fput(fd.file);
}

extern struct file *fget(unsigned int fd);
extern struct file *fget_many(unsigned int fd, unsigned int refs);
extern struct file *fget_raw(unsigned int fd);
extern unsigned long __fdget(unsigned int fd);
extern unsigned long __fdget_raw(unsigned int fd);
extern unsigned long __fdget_pos(unsigned int fd);
extern void __f_unlock_pos(struct file *);

static inline struct fd __to_fd(unsigned long v)
{
	return (struct fd){(struct file *)(v & ~3),v & 3};
}

static inline struct fd fdget(unsigned int fd)
{
	return __to_fd(__fdget(fd));
}

static inline struct fd fdget_raw(unsigned int fd)
{
	return __to_fd(__fdget_raw(fd));
}

static inline struct fd fdget_pos(int fd)
{
	return __to_fd(__fdget_pos(fd));
}

static inline void fdput_pos(struct fd f)
{
	if (f.flags & FDPUT_POS_UNLOCK)
		__f_unlock_pos(f.file);
	fdput(f);
}

extern int f_dupfd(unsigned int from, struct file *file, unsigned flags);
extern int replace_fd(unsigned fd, struct file *file, unsigned flags);
extern void set_close_on_exec(unsigned int fd, int flag);
extern bool get_close_on_exec(unsigned int fd);
extern int get_unused_fd_flags(unsigned flags);
extern void put_unused_fd(unsigned int fd);

extern void fd_install(unsigned int fd, struct file *file);

extern void flush_delayed_fput(void);
extern void __fput_sync(struct file *);

/**
 * struct file_file_pin
 *
 * Associate a pin'ed file with another file owner.
 *
 * Subsystems such as RDMA have the ability to pin memory which is associated
 * with a file descriptor which can be passed to other processes without
 * necessarily having that memory accessed in the remote processes address
 * space.
 *
 * @file file backing memory which was pined by a GUP caller
 * @f_owner the file representing the GUP owner
 * @list of all file pins this owner has
 *       (struct file *)->file_pins
 * @ref number of times this pin was taken (roughly the number of pages pinned
 *      in the file)
 */
struct file_file_pin {
	struct file *file;
	struct file *f_owner;
	struct list_head list;
	struct kref ref;
};

/*
 * struct mm_file_pin
 *
 * Some GUP callers do not have an "owning" file.  Those pins are accounted for
 * in the mm of the process that called GUP.
 *
 * The tuple {file, inode} is used to track this as a unique file pin and to
 * track when this pin has been removed.
 *
 * @file file backing memory which was pined by a GUP caller
 * @mm back point to owning mm
 * @inode backing the file
 * @list of all file pins this owner has
 *       (struct mm_struct *)->file_pins
 * @ref number of times this pin was taken
 */
struct mm_file_pin {
	struct file *file;
	struct mm_struct *mm;
	struct inode *inode;
	struct list_head list;
	struct kref ref;
};

#endif /* __LINUX_FILE_H */
