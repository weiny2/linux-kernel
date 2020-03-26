/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_UTIL_HEADER_
#define MXLK_UTIL_HEADER_

#include "mxlk.h"


enum mxlk_doorbell_direction {
	TO_DEVICE,
	FROM_DEVICE
};

enum mxlk_doorbell_type {
	DATA_SENT,
	DATA_RECEIVED,
	PHY_ID_UPDATED
};

u32 mxlk_create_sw_device_id(u8 func_no, u16 phy_id, u8 max_functions);
void mxlk_set_max_functions(struct mxlk *mxlk, u8 max_function);
u8 mxlk_get_max_functions(struct mxlk *mxlk);
void mxlk_set_physical_device_id(struct mxlk *mxlk, u16 phys_id);
u16 mxlk_get_physical_device_id(struct mxlk *mxlk);

void mxlk_set_doorbell(struct mxlk *mxlk, enum mxlk_doorbell_direction dirt,
		       enum mxlk_doorbell_type type);
bool mxlk_get_doorbell(struct mxlk *mxlk,
		       enum mxlk_doorbell_direction dirt,
		       enum mxlk_doorbell_type type);
void mxlk_clear_doorbell(struct mxlk *mxlk,
			 enum mxlk_doorbell_direction dirt,
			 enum mxlk_doorbell_type type);

void mxlk_set_device_status(struct mxlk *mxlk, u32 status);
u32 mxlk_get_device_status(struct mxlk *mxlk);
u32 mxlk_get_host_status(struct mxlk *mxlk);
void mxlk_set_host_status(struct mxlk *mxlk, u32 status);

struct mxlk_buf_desc *mxlk_alloc_bd(size_t length);
struct mxlk_buf_desc *mxlk_alloc_bd_reuse(size_t length, void *virt,
					  dma_addr_t phys);
void mxlk_free_bd(struct mxlk_buf_desc *bd);

int mxlk_list_init(struct mxlk_list *list);
void mxlk_list_cleanup(struct mxlk_list *list);
int mxlk_list_put(struct mxlk_list *list, struct mxlk_buf_desc *bd);
struct mxlk_buf_desc *mxlk_list_get(struct mxlk_list *list);
void mxlk_list_info(struct mxlk_list *list, size_t *bytes, size_t *buffers);

struct mxlk_buf_desc *mxlk_alloc_rx_bd(struct mxlk *mxlk);
void mxlk_free_rx_bd(struct mxlk *mxlk, struct mxlk_buf_desc *bd);
struct mxlk_buf_desc *mxlk_alloc_tx_bd(struct mxlk *mxlk);
void mxlk_free_tx_bd(struct mxlk *mxlk, struct mxlk_buf_desc *bd);

int mxlk_interface_init(struct mxlk *mxlk, int id);
void mxlk_interface_cleanup(struct mxlk_interface *inf);
void mxlk_interfaces_cleanup(struct mxlk *mxlk);
int mxlk_interfaces_init(struct mxlk *mxlk);
void mxlk_add_bd_to_interface(struct mxlk *mxlk, struct mxlk_buf_desc *bd);

#endif // MXLK_UTIL_HEADER_
