/* SPDX-License-Identifier: GPL-2.0 only */
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#ifndef MXLK_MMIO_HEADER_
#define MXLK_MMIO_HEADER_

#include <linux/module.h>
#include <linux/io.h>

/*
 * @brief Performs platform independent uint8_t read from iomem
 *
 * @param[in] base - pointer to base of remapped iomem
 * @param[in] offset - offset within iomem to read from
 *
 * @return:
 *	 Value read from location as uint8_t
 */
static u8 mxlk_rd8(void __iomem *base, int offset)
{
	return ioread8(base + offset);
}

/*
 * @brief Performs platform independent uint16_t read from iomem
 *
 * @param[in] base - pointer to base of remapped iomem
 * @param[in] offset - offset within iomem to read from
 *
 * @return:
 *	 Value read from location as uint16_t
 */
static u16 mxlk_rd16(void __iomem *base, int offset)
{
	return ioread16(base + offset);
}

/*
 * @brief Performs platform independent uint32_t read from iomem
 *
 * @param[in] base - pointer to base of remapped iomem
 * @param[in] offset - offset within iomem to read from
 *
 * @return:
 *	 Value read from location as uint32_t
 */
static u32 mxlk_rd32(void __iomem *base, int offset)
{
	return ioread32(base + offset);
}

/*
 * @brief Performs platform independent uint64_t read from iomem
 *
 * @param[in] base - pointer to base of remapped iomem
 * @param[in] offset - offset within iomem to read from
 *
 * @return:
 *	 Value read from location as uint64_t
 */
static u64 mxlk_rd64(void __iomem *base, int offset)
{
	u64 low;
	u64 high;

	low = mxlk_rd32(base, offset);
	high = mxlk_rd32(base, offset + sizeof(u32));

	return low | (high << 32);
}

/*
 * @brief Performs platform independent read from iomem
 *
 * @param[in]  base - pointer to base of remapped iomem
 * @param[in]  offset - offset within iomem to read from
 * @param[out] buffer - pointer to location to store values
 * @param[in]  len - length in bytes to read into buffer
 */
static void mxlk_rd_buffer(void __iomem *base, int offset, void *buffer,
			   size_t len)
{
	memcpy_fromio(buffer, base + offset, len);
}

/*
 * @brief Performs platform independent uint8_t write to iomem
 *
 * @param[in] base - pointer to base of remapped iomem
 * @param[in] offset - offset within iomem to read from
 * @param[in] value - uint8_t value to write
 */
static void mxlk_wr8(void __iomem *base, int offset, u8 value)
{
	iowrite8(value, base + offset);
}

/*
 * @brief Performs platform independent uint16_t write to iomem
 *
 * @param[in] base - pointer to base of remapped iomem
 * @param[in] offset - offset within iomem to read from
 * @param[in] value - uint16_t value to write
 */
static void mxlk_wr16(void __iomem *base, int offset, u16 value)
{
	iowrite16(value, base + offset);
}

/*
 * @brief Performs platform independent uint32_t write to iomem
 *
 * @param[in] base - pointer to base of remapped iomem
 * @param[in] offset - offset within iomem to read from
 * @param[in] value - uint32_t value to write
 */
static void mxlk_wr32(void __iomem *base, int offset, u32 value)
{
	iowrite32(value, base + offset);
}

/*
 * @brief Performs platform independent uint64_t write to iomem
 *
 * @param[in] base - pointer to base of remapped iomem
 * @param[in] offset - offset within iomem to read from
 * @param[in] value - uint64_t value to write
 */
static void mxlk_wr64(void __iomem *base, int offset, u64 value)
{
	mxlk_wr32(base, offset, value);
	mxlk_wr32(base, offset + sizeof(u32), value >> 32);
}

/*
 * @brief Performs platform independent write to iomem
 *
 * @param[in] base - pointer to base of remapped iomem
 * @param[in] offset - offset within iomem to read from
 * @param[in] buffer - pointer to location to write
 * @param[in] len - length in bytes to write into iomem
 */
static void mxlk_wr_buffer(void __iomem *base, int offset, void *buffer,
			   size_t len)
{
	memcpy_toio(base + offset, buffer, len);
}

#endif
