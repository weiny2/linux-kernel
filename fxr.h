/*
 * Copyright (c) 2013 - 2014 Intel Corporation.  All rights reserved.
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

#ifndef _FXR_H
#define _FXR_H

/*
 * This file contains all of the defines that is specific to the FXR chip.
 *
 * Note, the use of HFI prefix instead of FXR is intended to make it
 * easier to port this driver to a future HFI (if also similar to FXR).
 */

#include "include/fxr/fxr_top_map_defs.h"
#include "include/fxr/fxr_tx_ci_defs.h"
#include "include/fxr/fxr_rx_ci_defs.h"
#include "include/fxr/fxr_sw_structs.h"

/* TODO - find replacements for these from FXR headers */
#define HFI_NUM_INTERRUPTS	256
#define HFI_PCB_SIZE		32
#define HFI_PSB_PT_SIZE		32768
#define HFI_PSB_CT_SIZE		262144
#define HFI_PSB_EQ_DESC_SIZE	131072
#define HFI_PSB_EQ_HEAD_SIZE	32768
#define HFI_TRIG_OP_SIZE	128
#define HFI_LE_ME_SIZE		128

/* General */
#define HFI_NUM_PTL_PIDS	FXR_NUM_PIDS
#define HFI_NUM_PTL_AUTH_TUPLES	FXR_TX_CI_AUTH_ENTRIES

/* PCB */
#define HFI_PCB_TOTAL_MEM	(FXR_NUM_PIDS * HFI_PCB_SIZE)

/* PSB */
#define HFI_PSB_PT_OFFSET	0
#define HFI_PSB_CT_OFFSET	(HFI_PSB_PT_OFFSET + HFI_PSB_PT_SIZE)
#define HFI_PSB_EQ_DESC_OFFSET	(HFI_PSB_CT_OFFSET  + HFI_PSB_CT_SIZE)
#define HFI_PSB_EQ_HEAD_OFFSET	(HFI_PSB_EQ_DESC_OFFSET + HFI_PSB_EQ_DESC_SIZE)
#define HFI_PSB_FIXED_TOTAL_MEM	(HFI_PSB_PT_SIZE + HFI_PSB_CT_SIZE + \
				HFI_PSB_EQ_DESC_SIZE + HFI_PSB_EQ_HEAD_SIZE)
#define HFI_PSB_TRIG_OFFSET	HFI_PSB_FIXED_TOTAL_MEM
/* minimum triggered ops is defined such that PSB fills 512 KB */
#define HFI_PSB_TRIG_MIN_SIZE	65536
#define HFI_PSB_TRIG_MIN_COUNT  (HFI_PSB_TRIG_MIN_SIZE / HFI_TRIG_OP_SIZE)
#define HFI_PSB_MIN_TOTAL_MEM	(HFI_PSB_FIXED_TOTAL_MEM + \
				HFI_PSB_TRIG_MIN_SIZE)
#endif /* _FXR_H */
