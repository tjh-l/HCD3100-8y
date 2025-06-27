// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2005, Intec Automation Inc.
 * Copyright (C) 2014, Freescale Semiconductor, Inc.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

static void gd25q256_default_init(struct spi_nor *nor)
{
	/*
	 * Some manufacturer like GigaDevice may use different
	 * bit to set QE on different memories, so the MFR can't
	 * indicate the quad_enable method for this case, we need
	 * to set it in the default_init fixup hook.
	 */
	nor->params->quad_enable = spi_nor_sr2_bit1_quad_enable;
}

static struct spi_nor_fixups gd25q256_fixups = {
	.default_init = gd25q256_default_init,
};

static const struct flash_info gigadevice_parts[] = {
	{ "gd25q16", INFO(0xc84015, 0, 64 * 1024,  32,
			  SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			  SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25q32", INFO(0xc84016, 0, 64 * 1024,  64,
			  SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			  SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25lq32", INFO(0xc86016, 0, 64 * 1024, 64,
			   SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			   SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25q64", INFO(0xc84017, 0, 64 * 1024, 128,
			  SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			  SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25lq64c", INFO(0xc86017, 0, 64 * 1024, 128,
			    SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			    SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25lq128d", INFO(0xc86018, 0, 64 * 1024, 256,
			     SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			     SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25q128", INFO(0xc84018, 0, 64 * 1024, 256,
			   SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			   SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "by25q32", INFO(0x684016, 0, 64 * 1024, 64,
			   SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			   SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },

	{ "zb25vq128", INFO(0x5e4018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			   SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB | SPI_NOR_SKIP_SFDP | SPI_NOR_QUAD_WRITE)
		.fixups = &gd25q256_fixups},

	{ "zb25vq256", INFO(0x5e4019, 0, 64 * 1024, 512, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			   SPI_NOR_4B_OPCODES | SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB | SPI_NOR_SKIP_SFDP)
		.fixups = &gd25q256_fixups},

	{ "xt25f64", INFO(0x0b4017, 0, 64 * 1024, 128, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "xt25f128b", INFO(0x0b4018, 0, 64 * 1024, 256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },

	{ "by25q64", INFO(0x684017, 0, 64 * 1024, 128,
			   SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			   SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },

	{ "by25q128", INFO(0x684018, 0, 64 * 1024, 256,
			   SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			   SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "by25q256", INFO(0x684919, 0, 64 * 1024, 512,
			   SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			   SPI_NOR_4B_OPCODES | SPI_NOR_HAS_LOCK |
			   SPI_NOR_HAS_TB | SPI_NOR_TB_SR_BIT6 | SPI_NOR_SKIP_SFDP)
		.fixups = &gd25q256_fixups },
	{ "gd25q256", INFO(0xc84019, 0, 64 * 1024, 512,
			   SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			   SPI_NOR_4B_OPCODES | SPI_NOR_HAS_LOCK |
			   SPI_NOR_HAS_TB | SPI_NOR_TB_SR_BIT6)
		.fixups = &gd25q256_fixups },
	{ "fm25m4aa", INFO(0xf84218, 0, 64 * 1024, 256, SECT_4K) },
	{ "gm25q128", INFO(0x1c4018, 0, 64 * 1024, 256, SECT_4K| SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "gm25q64", INFO(0x1c4017, 0, 64 * 1024, 128, SECT_4K| SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },

	//ucuntech
	{ "25HD40",  INFO(0xB36013, 0, 64 * 1024,      8, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ ) },
	{ "25HQ32I",  INFO(0xB36016, 0, 64 * 1024,      64, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ ) },
	{ "25HQ64I",  INFO(0xB36017, 0, 64 * 1024,	128, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "25IQ128A",  INFO(0xB34018, 0, 64 * 1024,	256, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | SPI_NOR_SKIP_SFDP) },

	//Puya
	{ "P25D80SH",  INFO(0x856014, 0, 64 * 1024,      16, SECT_4K ) },
	{ "P25D16SH",  INFO(0x856015, 0, 64 * 1024,      32, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "P25D16SH",  INFO(0x852015, 0, 64 * 1024,      32, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "P25D32H",  INFO(0x856016, 0, 64 * 1024, 64, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "P25Q32H",  INFO(0x852016, 0, 64 * 1024, 64, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB ) },
	{ "P25Q64H",  INFO(0x852017, 0, 64 * 1024, 128, SECT_4K| SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ ) },
	{ "P25Q64SH",  INFO(0x856017, 0, 64 * 1024, 128, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "P25Q64HA",  INFO(0x852017, 0, 64 * 1024,  128, SECT_4K
			| SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			   SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "P25Q128HA",  INFO(0x852018, 0, 64 * 1024,  256,
			 SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |SECT_4K|
			 SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB | SPI_NOR_SKIP_SFDP | SPI_NOR_QUAD_WRITE)
		.fixups = &gd25q256_fixups
	},
	{ "P25Q256HA",  INFO(0x852019, 0, 64 * 1024,  512, SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ | \
			SPI_NOR_4B_OPCODES | SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB | \
			SPI_NOR_QUAD_WRITE | SPI_NOR_SKIP_SFDP) },
	{ "GT25Q64A-S",  INFO(0xC46017, 0, 64 * 1024, 128, SECT_4K| SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ ) },
};

const struct spi_nor_manufacturer spi_nor_gigadevice = {
	.name = "gigadevice",
	.parts = gigadevice_parts,
	.nparts = ARRAY_SIZE(gigadevice_parts),
};
