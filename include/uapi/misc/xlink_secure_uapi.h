/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
 * Xlink Secure Linux Kernel API
 *
 * Copyright (C) 2020 Intel Corporation
 *
 */
#ifndef __XLINK_SECURE_UAPI_H
#define __XLINK_SECURE_UAPI_H

#include <linux/types.h>

#define XLINK_MAGIC 'x'
#define XL_SECURE_OPEN_CHANNEL				_IOW(XLINK_MAGIC, 1, void*)
#define XL_SECURE_READ_DATA				_IOW(XLINK_MAGIC, 2, void*)
#define XL_SECURE_WRITE_DATA				_IOW(XLINK_MAGIC, 3, void*)
#define XL_SECURE_CLOSE_CHANNEL			_IOW(XLINK_MAGIC, 4, void*)
#define XL_SECURE_WRITE_VOLATILE			_IOW(XLINK_MAGIC, 5, void*)
#define XL_SECURE_READ_TO_BUFFER			_IOW(XLINK_MAGIC, 6, void*)
#define XL_SECURE_CONNECT					_IOW(XLINK_MAGIC, 7, void*)
#define XL_SECURE_RELEASE_DATA				_IOW(XLINK_MAGIC, 8, void*)
#define XL_SECURE_DISCONNECT				_IOW(XLINK_MAGIC, 9, void*)

struct xlinksecureopenchannel {
	void *handle;
	__u16 chan;
	int mode;
	__u32 data_size;
	__u32 timeout;
	__u32 *return_code;
};

struct xlinksecurewritedata {
	void *handle;
	__u16 chan;
	void const *pmessage;
	__u32 size;
	__u32 *return_code;
};

struct xlinksecurereaddata {
	void *handle;
	__u16 chan;
	void *pmessage;
	__u32 *size;
	__u32 *return_code;
};

struct xlinksecurereadtobuffer {
	void *handle;
	__u16 chan;
	void *pmessage;
	__u32 *size;
	__u32 *return_code;
};

struct xlinksecureconnect {
	void *handle;
	__u32 *return_code;
};

struct xlinksecurerelease {
	void *handle;
	__u16 chan;
	void *addr;
	__u32 *return_code;
};

#endif /* __XLINK_SECURE_UAPI_H */
