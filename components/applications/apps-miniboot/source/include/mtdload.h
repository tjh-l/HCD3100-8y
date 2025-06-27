#ifndef _MTDLOAD_H
#define _MTDLOAD_H

#include <kernel/types.h>

enum {
	IH_DEVT_SPINOR = 0, /*  spi norflash       */
	IH_DEVT_SPINAND, /* spi nandflash       */
	IH_DEVT_NAND, /* nandflash       */
	IH_DEVT_EMMC, /* emmc or sd-card       */
};

int mtdloadraw(unsigned char dev_type, void *dtb, u32 dtb_size, u32 start, u32 size);
int mtdloaduImage(unsigned char dev_type, u32 start, u32 size);

#endif
