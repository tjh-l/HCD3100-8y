// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2005, Intec Automation Inc.
 * Copyright (C) 2014, Freescale Semiconductor, Inc.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

static const struct flash_info xmc_parts[] = {
	/* XMC (Wuhan Xinxin Semiconductor Manufacturing Corp.) */
	{ "XM25QH40B", INFO(0x204013, 0, 64 * 1024, 8,
			    SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "XM25QH16A", INFO(0x207015, 0, 64 * 1024, 32,
			    SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "XM25QH32A", INFO(0x207016, 0, 64 * 1024, 64,
			    SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "XM25QH64A", INFO(0x207017, 0, 64 * 1024, 128,
			    SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "XM25QH128A", INFO(0x207018, 0, 64 * 1024, 256,
			     SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "XM25QH64D", INFO(0x204017, 0, 64 * 1024, 128,
			    SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "XM25QH128D", INFO(0x204018, 0, 64 * 1024, 256,
			    SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
};

const struct spi_nor_manufacturer spi_nor_xmc = {
	.name = "xmc",
	.parts = xmc_parts,
	.nparts = ARRAY_SIZE(xmc_parts),
};
