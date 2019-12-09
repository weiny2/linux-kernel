/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
 * xlink Linux Kernel API
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 */
#ifndef __XLINK_UAPI_H
#define __XLINK_UAPI_H

#include <linux/types.h>

#define XLINK_MAGIC 'x'
#define XL_OPEN_CHANNEL				_IOW(XLINK_MAGIC, 1, void*)
#define XL_READ_DATA				_IOW(XLINK_MAGIC, 2, void*)
#define XL_WRITE_DATA				_IOW(XLINK_MAGIC, 3, void*)
#define XL_CLOSE_CHANNEL			_IOW(XLINK_MAGIC, 4, void*)
#define XL_WRITE_VOLATILE			_IOW(XLINK_MAGIC, 5, void*)
#define XL_READ_TO_BUFFER			_IOW(XLINK_MAGIC, 6, void*)
#define XL_START_VPU				_IOW(XLINK_MAGIC, 7, void*)
#define XL_STOP_VPU					_IOW(XLINK_MAGIC, 8, void*)
#define XL_RESET_VPU				_IOW(XLINK_MAGIC, 9, void*)
#define XL_CONNECT					_IOW(XLINK_MAGIC, 10, void*)
#define XL_RELEASE_DATA				_IOW(XLINK_MAGIC, 11, void*)
#define XL_DISCONNECT				_IOW(XLINK_MAGIC, 12, void*)
#define XL_WRITE_CONTROL_DATA		_IOW(XLINK_MAGIC, 13, void*)
#define XL_DATA_READY_CALLBACK		_IOW(XLINK_MAGIC, 14, void*)
#define XL_DATA_CONSUMED_CALLBACK	_IOW(XLINK_MAGIC, 15, void*)

struct xlinkopenchannel {
	void *handle;
	__u16 chan;
	int mode;
	__u32 data_size;
	__u32 timeout;
	__u32 *return_code;
};

struct xlinkcallback {
	void *handle;
	__u16 chan;
	void *callback;
	__u32 *return_code;
};

struct xlinkwritedata {
	void *handle;
	__u16 chan;
	void const *pmessage;
	__u32 size;
	__u32 *return_code;
};

struct xlinkreaddata {
	void *handle;
	__u16 chan;
	void *pmessage;
	__u32 *size;
	__u32 *return_code;
};

struct xlinkreadtobuffer {
	void *handle;
	__u16 chan;
	void *pmessage;
	__u32 *size;
	__u32 *return_code;
};

struct xlinkconnect {
	void *handle;
	__u32 *return_code;
};

struct xlinkrelease {
	void *handle;
	__u16 chan;
	void *addr;
	__u32 *return_code;
};

struct xlinkstartvpu {
	char *filename;
	int namesize;
	__u32 *return_code;
};

struct xlinkstopvpu {
	__u32 *return_code;
};

#endif /* __XLINK_UAPI_H */
