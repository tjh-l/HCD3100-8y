// SPDX-License-Identifier: GPL-2.0
/*
 * This file is used to hold the spi-nor-manufacturer parts that datasheet unknown or to be verified.
 *
 * IMPORTANT: You'd better to ask help from spi-nor-manufacturer to confirm this.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

static const struct flash_info general_parts[] = {
	{ "25q32B",  INFO(0x544016, 0, 64 * 1024,	64, SECT_4K ) },
	{ "25q128B",  INFO(0x544018, 0, 64 * 1024,     256, SECT_4K|SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ ) },

	// Zbit
	{ "25VQ16B",  INFO(0x5E4015, 0, 64 * 1024,	32, SECT_4K | SPI_NOR_QUAD_READ | SPI_NOR_DUAL_READ | SPI_NOR_SKIP_SFDP) },
	{ "25VQ32B",  INFO(0x5E4016, 0, 64 * 1024,	64, SECT_4K | SPI_NOR_QUAD_READ | SPI_NOR_DUAL_READ | SPI_NOR_SKIP_SFDP) },
	{ "25VQ64B",  INFO(0x5E4017, 0, 64 * 1024,	128, SECT_4K | SPI_NOR_QUAD_READ | SPI_NOR_DUAL_READ | SPI_NOR_SKIP_SFDP) },
	{ "25VQ128B",  INFO(0x5E4018, 0, 64 * 1024,	256, SECT_4K | SPI_NOR_QUAD_READ | SPI_NOR_DUAL_READ | SPI_NOR_SKIP_SFDP) },

	/* UNKONW FLASH MANUFACTURER */
	{ "25q128csig",  INFO(0xd84018, 0, 64 * 1024,	256, SECT_4K | SPI_NOR_QUAD_READ | SPI_NOR_SKIP_SFDP) },
	{ "25qh64XMC",  INFO(0x204017, 0, 64 * 1024,    128, SECT_4K | SPI_NOR_QUAD_READ | SPI_NOR_SKIP_SFDP) },
	{ "25qh64XMC",  INFO(0xd84017, 0, 64 * 1024,    128, SECT_4K | SPI_NOR_QUAD_READ | SPI_NOR_SKIP_SFDP) },
	{ "P25Q128HA",  INFO(0x852018, 0, 64 * 1024,  256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ ) },
};

const struct spi_nor_manufacturer spi_nor_general= {
	.name = "General",
	.parts = general_parts,
	.nparts = ARRAY_SIZE(general_parts),
};

#if defined(CONFIG_SUPPORT_UNKNOW_FLASH)
#define SPI_NOR_GENERIC_FLAGS                                                            \
	(SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |                     \
	 SPI_NOR_4B_OPCODES | SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB |              \
	 SPI_NOR_QUAD_WRITE | SPI_NOR_SKIP_SFDP)
static const struct flash_info unknown_parts[] = {
	{ "Unknow128KB", 	INFO(0x5E4011, 0, 64 * 1024, 2, SPI_NOR_GENERIC_FLAGS & (~SPI_NOR_QUAD_WRITE)) },
	{ "Unknow256KB", 	INFO(0x5E4012, 0, 64 * 1024, 4, SPI_NOR_GENERIC_FLAGS & (~SPI_NOR_QUAD_WRITE)) },
	{ "Unknow512KB", 	INFO(0x5E4013, 0, 64 * 1024, 8, SPI_NOR_GENERIC_FLAGS & (~SPI_NOR_QUAD_WRITE)) },
	{ "Unknow1M", 		INFO(0x5E4014, 0, 64 * 1024, 16, SPI_NOR_GENERIC_FLAGS) },
	{ "Unknow2M", 		INFO(0x5E4015, 0, 64 * 1024, 32, SPI_NOR_GENERIC_FLAGS) },
	{ "Unknow4M", 		INFO(0x5E4016, 0, 64 * 1024, 64, SPI_NOR_GENERIC_FLAGS) },
	{ "Unknow8M", 		INFO(0x5E4017, 0, 64 * 1024, 128, SPI_NOR_GENERIC_FLAGS) },
	{ "Unknow16M",		INFO(0x5E4018, 0, 64 * 1024, 256, SPI_NOR_GENERIC_FLAGS) },
	{ "Unknow32M",		INFO(0x5E4019, 0, 64 * 1024, 512, SPI_NOR_GENERIC_FLAGS) },
};

const struct spi_nor_manufacturer spi_nor_unknown = {
	.name = "Unknow",
	.parts = unknown_parts,
	.nparts = ARRAY_SIZE(unknown_parts),
};
#endif
