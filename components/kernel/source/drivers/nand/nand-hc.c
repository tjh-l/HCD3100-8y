// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * HiChip HC16xx NAND Flash controller driver
 *
 * Copyright © 2021 HiChip Semiconductor Co., Ltd.
 *              http://www.hichiptech.com
 */
#define LOG_TAG "nand"
#define ELOG_OUTPUT_LVL ELOG_LVL_DEBUG

#include <linux/of.h>
#include <linux/mtd/mtd.h>
#include <linux/sizes.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
// #include <linux/mtd/partitions.h>
// #include <linux/mtd/mtd.h>
// #include <linux/pinctrl/pinctrl.h>
// #include <linux/version.h>
#include <linux/mtd/nand.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/jiffies.h>

#include <kernel/lib/fdt_api.h>
#include <hcuapi/pinmux.h>
#include <kernel/ld.h>


/* ****** porting for hcrtos ******** */
#ifdef READ
#undef READ
#endif
#define READ 0

#ifdef WRITE
#undef WRITE
#endif
#define WRITE 1


#define is_vmalloc_addr(DUMMY) 0
/* **************************** */

#define HCNFC_CHIP_DELAY			(0)
#define HCNFC_MAX_CHIP				(1)
#define HCNFC_DMA_TIMEOUT			msecs_to_jiffies(50)
#define HCNFC_DMA_ADDR_ALIGNMENT		(32)
#define HCNFC_DMA_BOUNCE_BUF_SIZE		(0x4000)

struct hcnfc_host {
	int			irq;
	struct nand_chip	chip;
	struct mtd_info		mtd;
	struct device		*dev;
	void __iomem		*iobase;
	struct pinctrl		*pctl;
	struct pinctrl_state	*active;
	struct completion       cmd_complete;
	int			chipselect;

	char			*buffer;
	dma_addr_t		dma_buffer;

	u32			need_scrambling;
	u32			warn_bitflips;
	u8			ecc_bch_mode;
	u8			all_ff;
};

/*
 * Define some generic bad / good block scan pattern which are used
 * while scanning a device for factory marked good / bad blocks.
 */
static uint8_t scan_ff_pattern[] = { 0xff, 0xff };

/* Generic flash bbt descriptors */
static uint8_t bbt_pattern[] = {'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = {'1', 't', 'b', 'B' };

__attribute__((unused))  static struct nand_ecclayout hc_nand_oob = {
	.oobavail = 8,
	.eccbytes = 28,
	.eccpos = { 4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
		    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 },
	.oobfree = { { .offset = 0, .length = 4 } }
};

__attribute__((unused))  static struct nand_bbt_descr largepage_flashbased = {
	.options = NAND_BBT_SCAN2NDPAGE,
	.offs = 0,
	.len = 2,
	.pattern = scan_ff_pattern
};

__attribute__((unused))  static struct nand_bbt_descr hc_bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE |
		   NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
	.pattern = bbt_pattern
};

__attribute__((unused))  static struct nand_bbt_descr hc_bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE |
		   NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs = 0,
	.len = 4,
	.veroffs = 4,
	.maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
	.pattern = mirror_pattern
};

#define NFC_REG_RD_REDUNDANT0				(0x0000)
#define NFC_REG_RD_REDUNDANT4				(0x0004)
#define NFC_REG_WR_REDUNDANT0				(0x0008)
#define NFC_REG_WR_REDUNDANT4				(0x000c)
#define NFC_REG_ERR_ECC_OCCUR				(0x0010)
#define NFC_REG_ERR_ECC_ST				(0x0014)
#define NFC_REG_CTL					(0x001c)
#define NFC_REG_IO_DATA					(0x0018)
#define NFC_REG_MODE					(0x0020)
#define NFC_REG_RD_TIMING_CTL				(0x0024)
#define NFC_REG_WR_TIMING_CTL				(0x0028)
#define NFC_REG_EDO_RB_CTL				(0x002c)
#define NFC_REG_DMA_CTL					(0x0030)
#define NFC_REG_DMA_LEN					(0x0034)
#define NFC_REG_ECC_CTL					(0x0038)
#define NFC_REG_ECC_ST					(0x003c)
#define NFC_REG_INTFLAG					(0x0040)
#define NFC_REG_DMA_ADDR				(0x004c)
#define NFC_REG_DMA_CONF				(0x0050)
#define NFC_REG_ERR_DETECT_SECT				(0x0058)

/* define bit use in NFC_REG_CTL */
#define NFC_CLE						BIT(4)
#define NFC_ALE						BIT(5)
#define NFC_CEJ						BIT(7)

/* define bit use in NFC_REG_MODE */
#define NFC_CRC_MODE_EN					BIT(0)
#define NFC_EF_MODE					BIT(1)
#define NFC_EF_MODE_EN					BIT(2)
#define NFC_EDO_EN					BIT(3)
#define NFC_WP						BIT(6)
#define NFC_EN						BIT(7)

/* define bit use in NFC_REG_EDO_RB_CTL */
#define NFC_TRB						GENMASK(3, 0)
	#define NFC_TRB_CONFIG				(0x08)
#define NFC_CRYPTO_ENGINE_EN				BIT(7)
#define NFC_REJ_DELAY_SEL1				GENMASK(12, 8)
#define NFC_REJ_DELAY_SEL2				GENMASK(20, 16)

/* define bit use in NFC_REG_DMA_CTL */
#define NFC_DMA_EN					BIT(0)
#define NFC_DMA_DIR					BIT(1)
	#define NFC_DMA_DIR_IN				(0 << 1)
	#define NFC_DMA_DIR_OUT				(1 << 1)
#define NFC_DMA_IMB_EN					BIT(2)

/* define bit use in NFC_REG_DMA_LEN */
#define NFC_FW_REDUNDANT_0B_PER_SECT			(0 << 5)
#define NFC_FW_REDUNDANT_1B_PER_SECT			(1 << 5)
#define NFC_FW_REDUNDANT_2B_PER_SECT			(2 << 5)
#define NFC_FW_REDUNDANT_3B_PER_SECT			(3 << 5)
#define NFC_FW_REDUNDANT_4B_PER_SECT			(4 << 5)
#define NFC_DMA_SECT_MASK				GENMASK(4, 0)
#define NFC_DMA_1_SECT					(1)
#define NFC_DMA_2_SECT					(2)
#define NFC_DMA_4_SECT					(4)
#define NFC_DMA_8_SECT					(8)
#define NFC_DMA_16_SECT					(16)

/* define bit use in NFC_REG_ECC_CTL */
#define NFC_ECC_RST					BIT(0)
#define NFC_ECC_FF_FLAG					BIT(1)
#define NFC_ECC_BCH_MODE				(GENMASK(3, 2) | 1 << 6)
	#define NFC_ECC_BCH_MODE_16B			(0 << 2 | 0 << 6)
	#define NFC_ECC_BCH_MODE_24B			(0 << 2 | 1 << 6)
	#define NFC_ECC_BCH_MODE_40B			(1 << 2 | 0 << 6)
	#define NFC_ECC_BCH_MODE_48B			(1 << 2 | 1 << 6)
	#define NFC_ECC_BCH_MODE_60B			(2 << 2 | 0 << 6)
	#define NFC_ECC_BCH_MODE_80B			(3 << 2 | 1 << 6)
#define NFC_ECC_BYPASS_MODE				GENMASK(5, 4)
	#define NFC_ECC_BYPASS_AUTO_STOP		(0 << 4)
	#define NFC_ECC_BYPASS_AUTO_STOP_FF		(1 << 4)
	#define NFC_ECC_BYPASS_NO_STOP			(2 << 4)
	#define NFC_ECC_BYPASS				(3 << 4)
#define NFC_ECC_MODE					BIT(7)
	#define NFC_ECC_MODE_ODDEVEN			(0 << 7)
	#define NFC_ECC_MODE_BCH			(1 << 7)
#define NFC_ECC_TOGGLE_MODE				BIT(14)
#define NFC_ECC_SYNC_MODE				BIT(15)
#define NFC_ECC_DQS_DELAY_SEL1				GENMASK(23, 16)
#define NFC_ECC_DQS_DELAY_SEL2				GENMASK(31, 24)

/* define bit use in NFC_REG_ECC_ST */
#define NFC_ECC_DQS_DELAY_SEL3				GENMASK(15, 8)
#define NFC_ECC_DQS_DELAY_SEL4				GENMASK(23, 16)
#define NFC_ECC_DQS_DELAY_SEL5				GENMASK(31, 24)
#define NFC_ECC_MAX_ERR_CNT				GENMASK(5, 0)

/* define bit use in NFC_REG_INTFLAG */
#define NFC_ECC_FLAG					BIT(5)
#define NFC_DMA_FLAG					BIT(7)
#define NFC_SRAM_DATA_VALID_FLAG			BIT(8)
#define NFC_SRAM_FULL_FLAG				BIT(9)
#define NFC_SRAM_EMPTY_FLAG				BIT(10)
#define NFC_IMB_BUF_FULL_FLAG				BIT(11)
#define NFC_IMB_BUF_EMPTY_FLAG				BIT(12)
#define NFC_IMB_WR_FINISH_FLAG				BIT(16)
#define NFC_IMB_RD_FINISH_FLAG				BIT(17)
#define NFC_ECC_INT_EN					BIT(24)
#define NFC_DMA_INT_EN					BIT(25)
#define NFC_IMB_WR_FINISH_INT_EN			BIT(26)
#define NFC_IMB_RD_FINISH_INT_EN			BIT(27)

/* define bit use in NFC_REG_DMA_CONF */
#define NFC_LAT						GENMASK(7, 0)
#define NFC_HI_PRIO					BIT(16)
#define NFC_INIT_DMA_DM					BIT(17)
#define NFC_INIT_DMA_IMB				BIT(18)

/* define bit use in NFC_REG_ERR_DETECT_SECT */
#define NFC_ERR_DETECT_16B_ECC				(0x0f << 16)
#define NFC_ERR_DETECT_24B_ECC				(0x17 << 16)
#define NFC_ERR_DETECT_40B_ECC				(0x27 << 16)
#define NFC_ERR_DETECT_48B_ECC				(0x2f << 16)
#define NFC_ERR_DETECT_60B_ECC				(0x37 << 16)
#define NFC_ERR_DETECT_80B_ECC				(0x4f << 16)

extern unsigned int noinline write_sync(void);

void hcnfc_irq_handle(uint32_t priv)
{
	struct hcnfc_host *host = (struct hcnfc_host *)priv;
	writel(0, host->iobase + NFC_REG_INTFLAG);
	complete(&host->cmd_complete);
}

static void hc_nfc_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *chip = mtd->priv;
	struct hcnfc_host *host = chip->priv;

	if ((ctrl & NAND_NCE) != NAND_NCE) {
		writeb(NFC_CEJ, host->iobase + NFC_REG_CTL);
		return;
	}

	if (ctrl & NAND_CTRL_CHANGE) {
		if ((ctrl & NAND_CTRL_CLE) == NAND_CTRL_CLE) {
			writeb(NFC_CLE, host->iobase + NFC_REG_CTL);
		} else if ((ctrl & NAND_CTRL_ALE) == NAND_CTRL_ALE) {
			writeb(NFC_ALE, host->iobase + NFC_REG_CTL);
		} else {
			writeb(0, host->iobase + NFC_REG_CTL);
		}
	}

	if (cmd != NAND_CMD_NONE) {
		writeb((u8)cmd, host->iobase + NFC_REG_IO_DATA);
	}

	return;
}

static void hc_nfc_init_dma(struct hcnfc_host *host, u8 dir)
{
	u32 val;


	writeb(0, host->iobase + NFC_REG_DMA_CTL);

	val = readl(host->iobase + NFC_REG_DMA_CONF);

	val |= NFC_INIT_DMA_IMB | NFC_INIT_DMA_DM;
	writel(val, host->iobase + NFC_REG_DMA_CONF);
	val &= ~(NFC_INIT_DMA_IMB | NFC_INIT_DMA_DM);
	writel(val, host->iobase + NFC_REG_DMA_CONF);

	writeb(0, host->iobase + NFC_REG_ECC_ST);
	writel(0, host->iobase + NFC_REG_INTFLAG);
	writel(0xffff, host->iobase + NFC_REG_ERR_ECC_ST);

	writeb(NFC_ECC_MODE_BCH | NFC_ECC_BYPASS_NO_STOP | host->ecc_bch_mode,
	       host->iobase + NFC_REG_ECC_CTL);
	
	if (dir == READ)
		writel(NFC_IMB_WR_FINISH_INT_EN, host->iobase + NFC_REG_INTFLAG);
	else
		writel(NFC_DMA_INT_EN, host->iobase + NFC_REG_INTFLAG);
}

static void hc_nfc_set_dma_addr(struct hcnfc_host *host, u32 addr)
{
	writel(addr & ~0xa0000000, host->iobase + NFC_REG_DMA_ADDR);
}

static void hc_nfc_set_dma_len(struct hcnfc_host *host, u8 sectors)
{
	u8 val;

	val = readb(host->iobase + NFC_REG_DMA_LEN);
	val &= ~NFC_DMA_SECT_MASK;
	val |= sectors;
	writeb(val, host->iobase + NFC_REG_DMA_LEN);
}

static void hc_nfc_dma_start(struct hcnfc_host *host, u8 dir)
{
	writeb(0, host->iobase + NFC_REG_INTFLAG);
	if (dir == READ) {
		writeb(NFC_DMA_EN | NFC_DMA_IMB_EN | NFC_DMA_DIR_IN,
		       host->iobase + NFC_REG_DMA_CTL);
	} else {
		writeb(NFC_DMA_EN | NFC_DMA_IMB_EN | NFC_DMA_DIR_OUT,
		       host->iobase + NFC_REG_DMA_CTL);
	}
}

static void hc_nfc_read0_cmd(struct nand_chip *chip)
{
	struct hcnfc_host *host = chip->priv;
	struct mtd_info *mtd = &host->mtd;

	chip->waitfunc(mtd, chip);
	chip->cmd_ctrl(mtd, NAND_CMD_READ0,
			      NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
}

static void hc_nfc_get_oob(struct hcnfc_host *host, u8 *oob_poi)
{
	*((u32 *)oob_poi + 0) = readl(host->iobase + NFC_REG_RD_REDUNDANT0);
	*((u32 *)oob_poi + 1) = readl(host->iobase + NFC_REG_RD_REDUNDANT4);
}

static void hc_nfc_fill_oob(struct hcnfc_host *host, u8 *oob_poi)
{
	writel(*((u32 *)oob_poi + 0), host->iobase + NFC_REG_WR_REDUNDANT0);
	writel(*((u32 *)oob_poi + 1), host->iobase + NFC_REG_WR_REDUNDANT4);
}

static int hc_nfc_check_ecc_status(struct hcnfc_host *host, int sectors)
{
	struct mtd_info *mtd = &host->mtd;
	u32 tmp, err;
	u32 mask = GENMASK(sectors - 1, 0);
	u8 val, ecc_fail = 0;
    int i;

	host->all_ff = 0;
	val = readb(host->iobase + NFC_REG_ECC_CTL);
	if (val & NFC_ECC_FF_FLAG) {
		host->all_ff = 1;
		return 0;
	}

	tmp = readl(host->iobase + NFC_REG_ERR_ECC_ST) & mask;
	if (!tmp) {
		tmp = readl(host->iobase + NFC_REG_ERR_DETECT_SECT) & mask;
		if (mask == tmp) {
			host->all_ff = 1;
			return 0;
		}
	}

	err = readl(host->iobase + NFC_REG_ERR_ECC_ST);
	tmp = err & mask;
	for (i = 0; i < sectors; i++) {
		if (!(tmp & 0x01)) {
			pr_err("ECC Error: %x\n", err);
			mtd->ecc_stats.failed++;
			ecc_fail = 1;
		}
		tmp >>= 1;
	}
	if (ecc_fail)
		return -1;

	if (!(readl(host->iobase + NFC_REG_ERR_ECC_ST) & mask))
		return 0;

	val = readb(host->iobase + NFC_REG_ECC_ST) & NFC_ECC_MAX_ERR_CNT;
	if ((val & NFC_ECC_MAX_ERR_CNT) > host->warn_bitflips)
		pr_warn("ECC corrected bits=%d\n", val);

	return 0;
}

static void hc_nfc_host_init(struct hcnfc_host *host)
{
	writeb(NFC_EN, host->iobase + NFC_REG_MODE);
	writeb(0x33, host->iobase + NFC_REG_RD_TIMING_CTL);
	writeb(0x33, host->iobase + NFC_REG_WR_TIMING_CTL);
	writeb(0, host->iobase + NFC_REG_DMA_CTL);
	writel(0, host->iobase + NFC_REG_INTFLAG);
}

/**
 * nand_read_page_op - Do a READ PAGE operation
 * @chip: The NAND chip
 * @page: page to read
 * @offset_in_page: offset within the page
 * @buf: buffer used to store the data
 * @len: length of the buffer
 *
 * This function issues a READ PAGE operation.
 * This function does not select/unselect the CS line.
 *
 * Returns 0 on success, a negative error code otherwise.
 */
static int nand_read_page_op(struct nand_chip *chip, unsigned int page,
		      unsigned int offset_in_page, void *buf, unsigned int len)
{
	struct hcnfc_host *host = chip->priv;
	struct mtd_info *mtd = &host->mtd;

	if (len && !buf)
		return -EINVAL;

	if (offset_in_page + len > mtd->writesize + mtd->oobsize)
		return -EINVAL;

	chip->cmdfunc(mtd, NAND_CMD_READ0, offset_in_page, page);
	if (len)
		chip->read_buf(mtd, buf, len);

	return 0;
}

static int hc_nfc_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
				  uint8_t *buf, int oob_required, int page)
{
	struct hcnfc_host *host = chip->priv;
	struct device *dev = host->dev;
	dma_addr_t phys_addr;
	u32 max_bitflips = 0;
	u8 dma_mapped = 0;
	u8 err_retry = 3;

	//nand_read_page_op(chip, page, 0, NULL, 0);

	if (buf && IS_ALIGNED((u32)buf, HCNFC_DMA_ADDR_ALIGNMENT) &&
	    !(is_vmalloc_addr((void *)buf))) {
		phys_addr = dma_map_single(dev, buf, mtd->writesize,
					   DMA_FROM_DEVICE);
		if (!dma_mapping_error(dev, phys_addr))
			dma_mapped = 1;
		else
			phys_addr = host->dma_buffer;
	} else {
		phys_addr = host->dma_buffer;
	}

	hc_nfc_init_dma(host, READ);
	hc_nfc_set_dma_addr(host, phys_addr);
	hc_nfc_set_dma_len(host, mtd->writesize >> 10);

	max_bitflips = mtd->ecc_stats.failed;
	hc_nfc_read0_cmd(chip);

	while (1) {
		init_completion(&host->cmd_complete);
		hc_nfc_dma_start(host, READ);

		if (wait_for_completion_timeout(&host->cmd_complete,
						HCNFC_DMA_TIMEOUT) <= 0) {
			pr_err("Error read page(%d) timeout!\n", page);
			hc_nfc_host_init(host);
			goto dma_retry;
		}

		if (dma_mapped) {
			dma_unmap_single(dev, phys_addr, mtd->writesize,
					 DMA_FROM_DEVICE);
		}

		if (hc_nfc_check_ecc_status(host, mtd->writesize >> 10))
			pr_err("ecc error page #0x%08x\n", page);

		if (host->all_ff)
			memset(buf, 0xff, mtd->writesize);
		else if (!dma_mapped)
			memcpy(buf, host->buffer, mtd->writesize);

		hc_nfc_get_oob(host, chip->oob_poi);

		if (mtd->ecc_stats.failed <= max_bitflips)
			break;

dma_retry:
		if (err_retry-- == 0) {
			pr_err("read fail!\n");
			break;
		}

		chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
		mtd->ecc_stats.failed = max_bitflips;
		nand_read_page_op(chip, page, 0, NULL, 0);
		hc_nfc_read0_cmd(chip);
	}

	return 0;
}

int hc_nfc_read_oob_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
			  int page)
{
	struct hcnfc_host *host = chip->priv;

	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
	return hc_nfc_read_page_hwecc(mtd, chip, (uint8_t *)host->buffer, 1,
				      page);
}

static int hc_nfc_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
				   const uint8_t *buf, int oob_required,
				   int page)
{
	struct hcnfc_host *host = chip->priv;
	struct device *dev = host->dev;
	dma_addr_t phys_addr;
	u8 dma_mapped = 0;

	if (buf && IS_ALIGNED((u32)buf, HCNFC_DMA_ADDR_ALIGNMENT) &&
	    !(is_vmalloc_addr((void *)buf))) {
		phys_addr = dma_map_single(dev, (u8 *)(buf), mtd->writesize,
				DMA_TO_DEVICE);
		if (!dma_mapping_error(dev, phys_addr))
			dma_mapped = 1;
		else
			phys_addr = host->dma_buffer;
	} else {
		phys_addr = host->dma_buffer;
	}

	hc_nfc_init_dma(host, WRITE);
	hc_nfc_set_dma_addr(host, phys_addr);
	hc_nfc_set_dma_len(host, mtd->writesize >> 10);

	hc_nfc_fill_oob(host, chip->oob_poi);

	if (!dma_mapped) {
		memcpy(host->buffer, buf, mtd->writesize);
	}

	write_sync();

	init_completion(&host->cmd_complete);

	hc_nfc_dma_start(host, WRITE);
	if (wait_for_completion_timeout(&host->cmd_complete,
					HCNFC_DMA_TIMEOUT) <= 0) {
		pr_err("Error write page timeout %d!\n", page);
	}

	if (dma_mapped)
		dma_unmap_single(dev, phys_addr, mtd->writesize, DMA_TO_DEVICE);

	return 0;
}

/**
 * nand_command_lp - [DEFAULT] Send command to NAND large page device
 * @chip: NAND chip object
 * @command: the command to be sent
 * @column: the column address for this command, -1 if none
 * @page_addr: the page address for this command, -1 if none
 *
 * Send command to NAND device. This is the version for the new large page
 * devices. We don't have the separate regions as we have in the small page
 * devices. We must emulate NAND_CMD_READOOB to keep the code compatible.
 */
static int nand_command_lp(struct mtd_info *mtd, unsigned command,
			    int column, int page_addr)
{
	struct nand_chip *chip = mtd->priv;

	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += mtd->writesize;
		command = NAND_CMD_READ0;
	}

	/* Command latch cycle */
	if (command != (unsigned)NAND_CMD_NONE)
		chip->cmd_ctrl(mtd, command,
				      NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

	if (column != -1 || page_addr != -1) {
		int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;

		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (chip->options & NAND_BUSWIDTH_16 &&
					!nand_opcode_8bits(command))
				column >>= 1;
			chip->cmd_ctrl(mtd, column, ctrl);
			ctrl &= ~NAND_CTRL_CHANGE;

			/* Only output a single addr cycle for 8bits opcodes. */
			if (!nand_opcode_8bits(command))
				chip->cmd_ctrl(mtd, column >> 8, ctrl);
		}
		if (page_addr != -1) {
			chip->cmd_ctrl(mtd, page_addr, ctrl);
			chip->cmd_ctrl(mtd, page_addr >> 8,
					     NAND_NCE | NAND_ALE);
			if (chip->chipsize > (128 << 20))
				chip->cmd_ctrl(mtd, page_addr >> 16,
						      NAND_NCE | NAND_ALE);
		}
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			      NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * Program and erase have their own busy handlers status, sequential
	 * in and status need no delay.
	 */
	switch (command) {

	case NAND_CMD_NONE:
	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_STATUS:
	case NAND_CMD_READID:
	case NAND_CMD_SET_FEATURES:
		return 0;

	case NAND_CMD_RNDIN:
		return 0;

	case NAND_CMD_RESET:
		if (chip->dev_ready)
			break;
		udelay(chip->chip_delay);
		chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
				      NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
				      NAND_NCE | NAND_CTRL_CHANGE);
		/* EZ-NAND can take upto 250ms as per ONFi v4.0 */
		nand_wait_ready(mtd);
		return 0;

	case NAND_CMD_RNDOUT:
		/* No ready / busy check necessary */
		chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART,
				      NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
				      NAND_NCE | NAND_CTRL_CHANGE);

		return 0;

	case NAND_CMD_READ0:
		/*
		 * READ0 is sometimes used to exit GET STATUS mode. When this
		 * is the case no address cycles are requested, and we can use
		 * this information to detect that READSTART should not be
		 * issued.
		 */
		if (column == -1 && page_addr == -1)
			return 0;

		chip->cmd_ctrl(mtd, NAND_CMD_READSTART,
				      NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
				      NAND_NCE | NAND_CTRL_CHANGE);
		if (!chip->dev_ready) {
			udelay(chip->chip_delay);
			return 0;
		}
		break;

	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay.
		 */
		if (!chip->dev_ready) {
			udelay(chip->chip_delay);
			return 0;
		}
	}

	/*
	 * Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine.
	 */
	udelay(1); //ndelay(100);

	nand_wait_ready(mtd);
}

static int hc_nfc_dev_ready(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
			      NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	if (chip->read_byte(mtd) & NAND_STATUS_READY) {
		return 1;
	} else {
		return 0;
	}
}

static int hc_nfc_ecc_probe(struct hcnfc_host *host)
{
	struct nand_chip *chip = &host->chip;
	struct mtd_info *mtd = &host->mtd;
	int size, oobsize;
	u8 val;
	u32 err_detect;

	size = chip->ecc.size;
	oobsize = mtd->oobsize / (mtd->writesize / size);

	chip->ecc.strength = 16;
	val = NFC_ECC_MODE_BCH | NFC_ECC_BYPASS_NO_STOP;
	if (nand_is_slc(chip) && oobsize <= 32) {
		host->ecc_bch_mode = NFC_ECC_BCH_MODE_16B;
		host->warn_bitflips = 13;
		val |= NFC_ECC_BCH_MODE_16B;
		err_detect = NFC_ERR_DETECT_16B_ECC;
	} else if (oobsize <= 46) {
		host->ecc_bch_mode = NFC_ECC_BCH_MODE_16B;
		host->warn_bitflips = 13;
		val |= NFC_ECC_BCH_MODE_16B;
		err_detect = NFC_ERR_DETECT_16B_ECC;
	} else if (oobsize <= 74) {
		host->ecc_bch_mode = NFC_ECC_BCH_MODE_24B;
		chip->ecc.strength = 24;
		host->warn_bitflips = 20;
		val |= NFC_ECC_BCH_MODE_24B;
		err_detect = NFC_ERR_DETECT_24B_ECC;
	} else if (oobsize <= 88) {
		host->ecc_bch_mode = NFC_ECC_BCH_MODE_40B;
		chip->ecc.strength = 40;
		host->warn_bitflips = 32;
		val |= NFC_ECC_BCH_MODE_40B;
		err_detect = NFC_ERR_DETECT_40B_ECC;
	} else if (oobsize <= 110) {
		host->ecc_bch_mode = NFC_ECC_BCH_MODE_48B;
		chip->ecc.strength = 48;
		host->warn_bitflips = 40;
		val |= NFC_ECC_BCH_MODE_48B;
		err_detect = NFC_ERR_DETECT_48B_ECC;
	} else if (oobsize <= 144) {
		host->ecc_bch_mode = NFC_ECC_BCH_MODE_60B;
		chip->ecc.strength = 60;
		host->warn_bitflips = 52;
		val |= NFC_ECC_BCH_MODE_60B;
		err_detect = NFC_ERR_DETECT_60B_ECC;
	} else {
		host->ecc_bch_mode = NFC_ECC_BCH_MODE_60B;
		chip->ecc.strength = 60;
		host->warn_bitflips = 52;
		val |= NFC_ECC_BCH_MODE_60B;
		err_detect = NFC_ERR_DETECT_60B_ECC;
	}

	writeb(val, host->iobase + NFC_REG_ECC_CTL);
	writeb(NFC_FW_REDUNDANT_4B_PER_SECT, host->iobase + NFC_REG_DMA_LEN);
	writel(err_detect, host->iobase + NFC_REG_ERR_DETECT_SECT);

	writeb(0, host->iobase + NFC_REG_DMA_CTL);
	writel(0, host->iobase + NFC_REG_INTFLAG);
	val = readb(host->iobase + NFC_REG_EDO_RB_CTL);
	if (host->need_scrambling) {
		val |= NFC_CRYPTO_ENGINE_EN | NFC_TRB_CONFIG;
		writeb(val, host->iobase + NFC_REG_EDO_RB_CTL);
	} else {
		val &= ~NFC_CRYPTO_ENGINE_EN;
		val |= NFC_TRB_CONFIG;
		writeb(val, host->iobase + NFC_REG_EDO_RB_CTL);
	}

	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	return 0;
}

static int hc_nfc_attach_chip(struct nand_chip *chip)
{
	struct hcnfc_host *host = chip->priv;

	chip->options |= NAND_NO_SUBPAGE_WRITE | NAND_USE_BOUNCE_BUFFER;

	chip->bbt_options |= NAND_BBT_USE_FLASH | NAND_BBT_NO_OOB_BBM | NAND_BBT_NO_OOB;
	chip->bbt_td = &hc_bbt_main_descr;
	chip->bbt_md = &hc_bbt_mirror_descr;
	chip->badblock_pattern = &largepage_flashbased;

	chip->ecc.read_page = hc_nfc_read_page_hwecc;
	chip->ecc.write_page = hc_nfc_write_page_hwecc;
	chip->ecc.read_oob = hc_nfc_read_oob_hwecc;
	chip->ecc.layout = &hc_nand_oob;
	chip->ecc.mode = NAND_ECC_HW;
	chip->ecc.size = 1024;
	chip->ecc.bytes = 28;

	hc_nfc_ecc_probe(host);

	return 0;
}

static int hc_nfc_probe(char *node)
{
	int ret, max_chips = HCNFC_MAX_CHIP;
	struct hcnfc_host *host = NULL;
	struct nand_chip *chip = NULL;
	struct mtd_info *mtd = NULL;
	u32 iobase;

	int np;
	struct pinmux_setting *active_state;

	
	hc_clk_enable(NAND_FLASH_CLK);

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	host = kzalloc(sizeof(*host), GFP_KERNEL);
	if (!host)
		return -ENOMEM;

	if (fdt_get_property_u_32_index(np, "reg", 0, &iobase))
		return 0;
	host->iobase = (void *)((u32)iobase | (0xa0000000));

	host->irq = (int)&NF_INTR;

	host->buffer = kzalloc(HCNFC_DMA_BOUNCE_BUF_SIZE, GFP_KERNEL);
	if (!host->buffer) {
		kfree(host);
		return -ENOMEM;
	}
	host->dma_buffer = (dma_addr_t)host->buffer | 0xA0000000;

	init_completion(&host->cmd_complete);

	chip = &host->chip;
	chip->priv = host;
	mtd = &host->mtd;
	mtd->priv = chip;

	if (host->irq < 0)
		return -ENXIO;

	if (!fdt_get_property_u_32_index(np, "scrambling", 0, &host->need_scrambling))
		host->need_scrambling = !!host->need_scrambling;

	active_state = fdt_get_property_pinmux(np, "active");
	if (active_state) {
		pinmux_select_setting(active_state);
		free(active_state);
	}

	mtd->name               = "hichip_nand";

	chip->IO_ADDR_R = (void __iomem *) (host->iobase + NFC_REG_IO_DATA);
	chip->IO_ADDR_W = (void __iomem *) (host->iobase + NFC_REG_IO_DATA);
	chip->cmd_ctrl = hc_nfc_cmd_ctrl;
	chip->chip_delay = HCNFC_CHIP_DELAY;
	chip->dev_ready = hc_nfc_dev_ready;
	chip->cmdfunc = nand_command_lp;

	xPortInterruptInstallISR(host->irq, hcnfc_irq_handle, (uint32_t)host);

	hc_nfc_host_init(host);

	ret = nand_scan_ident(mtd, max_chips, NULL, NULL);
	if (ret)
		return ret;

	ret = hc_nfc_attach_chip(chip);
	if (ret)
		return ret;

	ret = nand_scan_tail(mtd);
	if (ret) {
		nand_release(mtd);
		return ret;
	}

	ret = mtd_device_register(mtd, node);
	if (ret) {
		dev_err(dev, "Err MTD partition=%d\n", ret);
		nand_release(mtd);
		return ret;
	}

	return 0;
}

static int hc_nand_init(void)
{
	elog_set_filter_tag_lvl("nand", ELOG_LVL_ERROR);
	return hc_nfc_probe("/hcrtos/nand");
}

module_system(hc_nand, hc_nand_init, NULL, 0)
