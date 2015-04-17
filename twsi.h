#ifndef _TWSI_H
#define _TWSI_H
/*
 * Copyright (c) 2013 Intel Corporation.  All rights reserved.
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

#define QIB_TWSI_NO_DEV 0xFF

struct hfi_devdata;

/* Bit position of SDA pin in ASIC_QSFP* registers  */
#define  WFR_GPIO_SDA_NUM 1

/* these functions must be called with qsfp_lock held */
int hfi1_twsi_reset(struct hfi_devdata *dd, u32 target);
int hfi1_twsi_blk_rd(struct hfi_devdata *dd, u32 target, int dev, int addr,
		    void *buffer, int len);
int hfi1_twsi_blk_wr(struct hfi_devdata *dd, u32 target, int dev, int addr,
		    const void *buffer, int len);


#endif /* _TWSI_H */
