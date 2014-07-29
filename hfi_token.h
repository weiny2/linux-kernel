/*
 * Copyright (c) 2012 - 2014 Intel Corporation.  All rights reserved.
 * Copyright (c) 2006 - 2012 QLogic Corporation. All rights reserved.
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

#ifndef _HFI_MMAP_H
#define _HFI_MMAP_H

/*
 * Masks and offsets defining the mmap tokens
 */
#define HFI_MMAP_OFFSET_MASK   0xfffULL
#define HFI_MMAP_OFFSET_SHIFT  0
#define HFI_MMAP_SUBCTXT_MASK  0xfULL
#define HFI_MMAP_SUBCTXT_SHIFT 12
#define HFI_MMAP_CTXT_MASK     0xffULL
#define HFI_MMAP_CTXT_SHIFT    16
#define HFI_MMAP_TYPE_MASK     0xfULL
#define HFI_MMAP_TYPE_SHIFT    24
#define HFI_MMAP_MAGIC_MASK    0xffffffffULL
#define HFI_MMAP_MAGIC_SHIFT   32

#define HFI_MMAP_MAGIC         0xdabbad00

#define HFI_MMAP_TOKEN_SET(field, val)	\
	(((val) & HFI_MMAP_##field##_MASK) << HFI_MMAP_##field##_SHIFT)
#define HFI_MMAP_TOKEN_GET(field, token) \
	(((token) >> HFI_MMAP_##field##_SHIFT) & HFI_MMAP_##field##_MASK)
#define HFI_MMAP_TOKEN(type, ctxt, subctxt, addr)   \
	(HFI_MMAP_TOKEN_SET(MAGIC, HFI_MMAP_MAGIC) | \
	 HFI_MMAP_TOKEN_SET(TYPE, type) | \
	 HFI_MMAP_TOKEN_SET(CTXT, ctxt) | \
	 HFI_MMAP_TOKEN_SET(SUBCTXT, subctxt) | \
	 HFI_MMAP_TOKEN_SET(OFFSET, ((unsigned long)addr & ~PAGE_MASK)))

#endif
