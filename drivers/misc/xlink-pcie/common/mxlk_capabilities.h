/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_CAPABILITIES_HEADER_
#define MXLK_CAPABILITIES_HEADER_

#include "mxlk.h"
#include "mxlk_common.h"

void *mxlk_cap_find(struct mxlk *mxlk, u16 start, u16 id);
#ifdef XLINK_PCIE_LOCAL
void mxlk_set_cap(struct mxlk *mxlk);
#endif

void mxlk_set_td_address(struct mxlk_transfer_desc *td, u64 address);
u64 mxlk_get_td_address(struct mxlk_transfer_desc *td);
void mxlk_set_td_length(struct mxlk_transfer_desc *td, u32 length);
u32 mxlk_get_td_length(struct mxlk_transfer_desc *td);
void mxlk_set_td_interface(struct mxlk_transfer_desc *td, u16 interface);
u16 mxlk_get_td_interface(struct mxlk_transfer_desc *td);
void mxlk_set_td_status(struct mxlk_transfer_desc *td, u16 status);
u16 mxlk_get_td_status(struct mxlk_transfer_desc *td);

void mxlk_set_tdr_head(struct mxlk_pipe *p, u32 head);
u32 mxlk_get_tdr_head(struct mxlk_pipe *p);
void mxlk_set_tdr_tail(struct mxlk_pipe *p, u32 tail);
u32 mxlk_get_tdr_tail(struct mxlk_pipe *p);

#endif
