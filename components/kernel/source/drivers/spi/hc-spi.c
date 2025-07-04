#include <errno.h>
#include <unistd.h>
#include <kernel/module.h>
#include <kernel/completion.h>
#include <kernel/drivers/spi.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <hcuapi/pinmux.h>

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/dma-mapping.h>
#include <linux/jiffies.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <kernel/lib/fdt_api.h>
#include <generated/br2_autoconf.h>
#include <nuttx/fs/fs.h>


extern unsigned long SFSPI0;
extern unsigned long SF_INTR;

#define REG_SF_BASE				(0)
#include "hc_sf_reg.h"

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define MAX_INLINE_NUM_CS			(16)
#define SPI_INVALID_DMA_ADDR			(0xffffffff)

#define SFLASH_DMACTRL_TX_DEFCONFIG                                            \
	(F_SF_0064_DMA_INT_EN | F_SF_0064_ARBIT_MODE | F_SF_0064_ACCESS_REQ |  \
	 F_SF_0064_ACCESS_MODE | F_SF_0064_DMA_DIR | F_SF_0064_DMA_START)

#ifdef CONFIG_SOC_HC15XX
#define SFLASH_DMACTRL_RX_DEFCONFIG                                            \
	(F_SF_0064_DMA_INT_EN | F_SF_0064_ARBIT_MODE | \
	 F_SF_0064_ACCESS_REQ | F_SF_0064_ACCESS_MODE | F_SF_0064_DMA_START)
#else
#define SFLASH_DMACTRL_RX_DEFCONFIG                                            \
	(F_SF_0064_PRE_READ_EN | F_SF_0064_DMA_INT_EN | F_SF_0064_ARBIT_MODE | \
	 F_SF_0064_ACCESS_REQ | F_SF_0064_ACCESS_MODE | F_SF_0064_DMA_START)
#endif

#define SFLASH_CONF0_DEFCONFIG			(0x03)
#define SFLASH_CONF1_DEFCONFIG			(F_SF_0099_DATA_HIT | F_SF_0099_ADDR_HIT | F_SF_0099_CODE_HIT)

/*
 *  mode    cmd    addr+dummy     data
 * MODE0     1          1          1
 * MODE1     1          1          2
 * MODE2     1          2          2
 * MODE3     1          4          4
 * MODE4     4          4          4
 * MODE5     1          1          4
 * MODE6     2          2          2
 */
#define SFLASH_MODE_0 (0)
#define SFLASH_MODE_1 (1)
#define SFLASH_MODE_2 (2)
#define SFLASH_MODE_3 (3)
#define SFLASH_MODE_4 (4)
#define SFLASH_MODE_5 (5)
#define SFLASH_MODE_6 (6)
#define SFLASH_ADDR_3BYTES (0)
#define SFLASH_ADDR_4BYTES (1)
#define SFLASH_ADDR_1BYTES (2)
#define SFLASH_ADDR_2BYTES (3)
#define SFLASH_CONF2_DEFCONFIG			(SFLASH_MODE_0 | SFLASH_ADDR_3BYTES)

#define SFLASH_SIZE_2MB (0)
#define SFLASH_SIZE_4MB (1)
#define SFLASH_SIZE_8MB (2)
#define SFLASH_SIZE_16MB (3)
#define SFLASH_CONF3_DEFCONFIG			(SFLASH_SIZE_16MB << F_SF_009B_SIZE_S)

#define HC_SPI_CLK_108MHZ			(108 * 1000 * 1000)
#define HC_SPI_CLK				(HC_SPI_CLK_108MHZ)

#define DMA_ADDR_ALIGN				(64)
#define DMA_LEN_ALIGN				(64)
#define DMA_LEN_MIN				(0x100)
#define DMA_LEN_MAX				(0x400000)
#define DMA_TRANS_TIMEOUT			msecs_to_jiffies(10000)

static struct mutex            sf_lock;

struct hc_spi {
	int			irq;

	uint32_t		sclk;
	uint32_t		dma_force;

	struct pinmux_setting	*active_state;
	void			*iobase;

	void			*inlinepiobase[MAX_INLINE_NUM_CS];
	void			**piobase;
	uint32_t		num_cs;

	struct spi_device	*spi;

	struct completion	completion;
};

enum spi_dma_dir {
	SPI_DMA_TX,
	SPI_DMA_RX
};

static int hc_spi_set_clk_div(struct hc_spi *hs, u32 rate)
{
	int i = 0;
	uint32_t div = 0;
	uint32_t clk_reg = 0;
	uint32_t tmp1 = HC_SPI_CLK, tmp2 = HC_SPI_CLK;

#ifdef CONFIG_SOC_HC16XX
	if (rate > 54 * 1000 * 1000) {
		taskENTER_CRITICAL();
		clk_reg = REG32_READ((uint32_t)&SYSIO0 + 0x7c);
		clk_reg &= 0xffff3fff;
		clk_reg |= 0x00008000;
		REG32_WRITE((uint32_t)&SYSIO0 + 0x7c, clk_reg);
		taskEXIT_CRITICAL();
		return 0;
	}
#endif

	for (; i < 0x10; i++) {
		tmp1 = HC_SPI_CLK / (2 * i + 2);
		if (abs(tmp1 - rate) < tmp2) {
			tmp2 = abs(tmp1 - rate);
			div = i;
		}
	}

#ifdef CONFIG_SOC_HC16XX
	taskENTER_CRITICAL();
	clk_reg = REG32_READ((uint32_t)&SYSIO0 + 0x7c);
	clk_reg &= 0xffff3fff;
	REG32_WRITE((uint32_t)&SYSIO0 + 0x7c, clk_reg);
	taskEXIT_CRITICAL();
#endif
	return div;
}

static void hc_spi_set_clk_rate(struct hc_spi *hs, u32 rate)
{
	u32 div = hc_spi_set_clk_div(hs, rate);

	writeb(SFLASH_CONF3_DEFCONFIG | (div & F_SF_009B_CLK_DIV_M),
	       hs->iobase + R_SF_CONF3);
	writeb((div >> 4) & F_SF_0004_CLK_DIV_EXT_M,
	       hs->iobase + R_SF_CLK_DIV_EXT);
}

static inline void hc_spi_set_clock_mode(struct hc_spi *hs, u32 mode)
{
	writeb((mode & (SPI_CPHA | SPI_CPOL)) << 1, hs->iobase + R_SF_CPU_DMA_CTRL + 1);
}

static int hc_spi_setup(struct spi_device *spi)
{
	struct hc_spi  *hs = spi_master_get_devdata(spi->master);

	writeb(SFLASH_CONF3_DEFCONFIG, hs->iobase + R_SF_CONF3);
	writeb(SFLASH_CONF2_DEFCONFIG, hs->iobase + R_SF_CONF2);
	writeb(SFLASH_CONF1_DEFCONFIG, hs->iobase + R_SF_CONF1);
	writeb(SFLASH_CONF0_DEFCONFIG, hs->iobase + R_SF_CONF0);

	hc_spi_set_clock_mode(hs, spi->mode);

	return 0;
}

static void hc_spi_dma_transfer_tx(struct hc_spi *hs, void *buf, u32 len)
{
	u32 val;
	dma_addr_t tx_dma;

	tx_dma = dma_map_single(NULL, buf, len, DMA_TO_DEVICE);
	if (dma_mapping_error(dev, tx_dma)) {
		dev_err(dev, "DMA map tx fail!\n");
		return;
	}

	init_completion(&hs->completion);

	writel(tx_dma & F_SF_0058_DMA_MEM_ADDR_M, hs->iobase + R_SF_DMA_MEM_ADDR);

	val = readl(hs->iobase + R_SF_CONF0);
	val |= (1 << 21 | 1 << 15);
	writel(val, hs->iobase + R_SF_CONF0);

	// writel(SFLASH_DMACTRL_TX_DEFCONFIG, hs->iobase + R_SF_DMA_CTRL);
	writel(SFLASH_DMACTRL_TX_DEFCONFIG | hs->spi->chip_select, hs->iobase + R_SF_DMA_CTRL);

	if (wait_for_completion_timeout(&hs->completion, DMA_TRANS_TIMEOUT) == 0) {
		dev_err(dev, "DMA tx timed out!\n");
		return;
	}

	val = readl(hs->iobase + R_SF_CONF0);
	val &= ~(1 << 21 | 1 << 15);
	writel(val, hs->iobase + R_SF_CONF0);

	dma_unmap_single(NULL, tx_dma, len, DMA_TO_DEVICE);

	return;
}

static void hc_spi_dma_transfer_rx(struct hc_spi *hs, void *buf, u32 len)
{
	dma_addr_t rx_dma;
	u32 val;

	rx_dma = dma_map_single(NULL, buf, len, DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, rx_dma)) {
		dev_err(dev, "DMA map rx fail!\n");
		return;
	}

	init_completion(&hs->completion);

	writel(rx_dma & F_SF_0058_DMA_MEM_ADDR_M, hs->iobase + R_SF_DMA_MEM_ADDR);

	val = readl(hs->iobase + R_SF_CONF0);
	val |= (1 << 21 | 1 << 14);
	writel(val, hs->iobase + R_SF_CONF0);

	// writel(SFLASH_DMACTRL_RX_DEFCONFIG, hs->iobase + R_SF_DMA_CTRL);
	writel(SFLASH_DMACTRL_RX_DEFCONFIG | hs->spi->chip_select, hs->iobase + R_SF_DMA_CTRL);

	if (wait_for_completion_timeout(&hs->completion, DMA_TRANS_TIMEOUT) == 0) {
		dev_err(dev, "DMA rx timed out!\n");
		return;
	}

	val = readl(hs->iobase + R_SF_CONF0);
	val &= ~(1 << 21 | 1 << 14);
	writel(val, hs->iobase + R_SF_CONF0);

	dma_unmap_single(NULL, rx_dma, len, DMA_FROM_DEVICE);

	return;
}

static void hc_spi_dma_transfer(struct hc_spi *hs, void *buf, u32 len, int dir)
{
	writel(len, hs->iobase + R_SF_SQI_COUNT);
	writel(len, hs->iobase + R_SF_DMA_LEN);

	if (dir == SPI_DMA_TX) {
		hc_spi_dma_transfer_tx(hs, buf, len);
	} else {
		hc_spi_dma_transfer_rx(hs, buf, len);
	}

	return;
}

static void hc_spi_pio_rx(struct hc_spi *hs, u8 *buf, u32 len)
{
	u8 *dst = buf;
	u32 copy_left = len;
	u32 pioaddr = (u32)hs->piobase[hs->spi->chip_select];

	while (copy_left) {
		if ((copy_left % 4 == 0) && ((u32)dst % 4 == 0)) {
			*(u32 *)dst = *(u32 *)pioaddr;
			dst += 4;
			copy_left -= 4;
		} else {
			*dst = *(u8 *)pioaddr;
			dst++;
			copy_left--;
		}
	}

	return;
}

static void hc_spi_pio_tx(struct hc_spi *hs, u8 *buf, u32 len)
{
	u8 *src = buf;
	u32 copy_left = len;
	u32 pioaddr = (u32)hs->piobase[hs->spi->chip_select];

	while (copy_left) {
		if ((copy_left % 4 == 0) && ((u32)src % 4 == 0)) {
			*(u32 *)(pioaddr) = *(u32 *)src;
			src += 4;
			copy_left -= 4;
		} else {
			*(u8 *)(pioaddr) = *src;
			src++;
			copy_left--;
		}
	}

	return;
}

static void hc_spi_set_multi_io(struct hc_spi *hs, int mode)
{
	u8 val;

	val = readb(hs->iobase + R_SF_CONF2);
	val &= ~(F_SF_009A_MODE_M);
	val |= mode;
	writeb(val, hs->iobase + R_SF_CONF2);
}

static void hc_spi_force_dma_tx(struct hc_spi *hs, void *buf, unsigned len)
{
	u32 val;
	dma_addr_t tx_dma;

	writel(len, hs->iobase + R_SF_SQI_COUNT);
	writel(0, hs->iobase + R_SF_DMA_FLASH_ADDR);
	writel(len, hs->iobase + R_SF_DMA_LEN);

	tx_dma = dma_map_single(NULL, buf, len, DMA_TO_DEVICE);
	if (dma_mapping_error(dev, tx_dma)) {
		dev_err(dev, "DMA map tx fail!\n");
		return;
	}

	init_completion(&hs->completion);

	writel(tx_dma & F_SF_0058_DMA_MEM_ADDR_M, hs->iobase + R_SF_DMA_MEM_ADDR);

	val = readl(hs->iobase + R_SF_CONF0);
	val |= (1 << 21 | 1 << 15);
	writel(val, hs->iobase + R_SF_CONF0);

	writeb(F_SF_0099_CONTINUE_WRITE | F_SF_0099_DATA_HIT, hs->iobase + R_SF_CONF1);

	// writel(0x005e00e0, hs->iobase + R_SF_DMA_CTRL);
	writel(0x005e00e0 | hs->spi->chip_select, hs->iobase + R_SF_DMA_CTRL);

	if (wait_for_completion_timeout(&hs->completion, DMA_TRANS_TIMEOUT) == 0) {
		dev_err(dev, "DMA tx timed out!\n");
		return;
	}

	val = readl(hs->iobase + R_SF_CONF0);
	val &= ~(1 << 21 | 1 << 15);
	writel(val, hs->iobase + R_SF_CONF0);

	dma_unmap_single(NULL, tx_dma, len, DMA_TO_DEVICE);

	return;
}

static void hc_spi_transfer_tx(struct hc_spi *hs, void *buf, unsigned len)
{
	u32 pioaddr;
	u32 copy_left;
	u32 dma_copy_left;
	u8 *src;

	copy_left = len;
	src = (u8 *)buf;

	writeb(F_SF_0099_DATA_HIT, hs->iobase + R_SF_CONF1);

	/*
	 * Read IO register to confirm the configuration before transfer, otherwise
	 * there might be cc error
	 */
	ioread32(hs->iobase);

	/*
	 * PIO mode always using cpu copy
	 */

	if (hs->dma_force) {
		while (copy_left) {
			if (copy_left > DMA_LEN_MAX)
				dma_copy_left = DMA_LEN_MAX;
			else
				dma_copy_left = copy_left;
			hc_spi_force_dma_tx(hs, src, dma_copy_left);
			src += dma_copy_left;
			copy_left -= dma_copy_left;
		}
		return;
	}

	if (len < DMA_LEN_MIN) {
		hc_spi_pio_tx(hs, buf, len);
		return;
	}

	pioaddr = (u32)hs->piobase[hs->spi->chip_select];
	while (copy_left) {
		if (!IS_ALIGNED((u32)src, DMA_ADDR_ALIGN) ||
		    copy_left < DMA_LEN_ALIGN) {
			*(u8 *)(pioaddr + *src) = *src;
			src++;
			copy_left--;
			continue;
		}

		dma_copy_left = ALIGN_DOWN(copy_left, DMA_LEN_ALIGN);
		if (dma_copy_left) {
			if (dma_copy_left > DMA_LEN_MAX)
				dma_copy_left = DMA_LEN_MAX;

			hc_spi_dma_transfer(hs, src, dma_copy_left, SPI_DMA_TX);
			src += dma_copy_left;
			copy_left -= dma_copy_left;
		}
	}

	return;
}


static void hc_spi_transfer_rx(struct hc_spi *hs, void *buf, unsigned len, bool force_pio)
{
	u32 pioaddr;
	u32 copy_left;
	u32 dma_copy_left;
	u8 *dst;

	writeb(F_SF_0099_DATA_HIT, hs->iobase + R_SF_CONF1);

	/*
	 * Read IO register to confirm the configuration before transfer, otherwise
	 * there might be cc error
	 */
	ioread32(hs->iobase);

	/*
	 * PIO mode always using cpu copy
	 */
	if (len < DMA_LEN_MIN) {
		hc_spi_pio_rx(hs, buf, len);
		return;
	}

	if (force_pio) {
		hc_spi_pio_rx(hs, buf, len);
		return;
	}

	copy_left = len;
	dst = (u8 *)buf;
	pioaddr = (u32)hs->piobase[hs->spi->chip_select];
	while (copy_left) {
		if (!IS_ALIGNED((u32)dst, DMA_ADDR_ALIGN) ||
		    copy_left < DMA_LEN_ALIGN) {
			*dst++ = *(u8 *)pioaddr;
			copy_left--;
			continue;
		}

		dma_copy_left = ALIGN_DOWN(copy_left, DMA_LEN_ALIGN);
		if (dma_copy_left) {
			if (dma_copy_left > DMA_LEN_MAX)
				dma_copy_left = DMA_LEN_MAX;

			hc_spi_dma_transfer(hs, dst, dma_copy_left, SPI_DMA_RX);
			dst += dma_copy_left;
			copy_left -= dma_copy_left;
		}
	}

	return;
}

static unsigned int hc_spi_transfer_setup_mode(struct hc_spi *hs, struct spi_transfer *t)
{
	unsigned int mode;

	mode = max(t->rx_nbits, t->tx_nbits);

	switch (mode) {
	case SPI_NBITS_QUAD:
		/*cmd + addr + data : 1 + 1 + 4*/
		hc_spi_set_multi_io(hs, SFLASH_MODE_5);
		break;
	case SPI_NBITS_DUAL:
		/*cmd + addr + data : 1 + 1 + 2*/
		hc_spi_set_multi_io(hs, SFLASH_MODE_1);
		break;
	default:
		/*cmd + addr + data : 1 + 1 + 1*/
		hc_spi_set_multi_io(hs, SFLASH_MODE_0);
		break;
	}

	return mode;
}

static void hc_spi_set_cs(struct spi_device *spi, bool enable)
{
	struct hc_spi *hs = spi_controller_get_devdata(spi->master);
	u32 val;

	if (!enable) {
		if (hs->active_state) {
			pinmux_select_setting(hs->active_state);
		}
		val = readl(hs->iobase + R_SF_TIMING_CTRL);
		val &= ~(F_SF_00C8_CS_PROGRAM_M | F_SF_00C8_CS_PROGRAM_EN_M);
		val |= ((3 << F_SF_00C8_CS_PROGRAM_S) | F_SF_00C8_CS_PROGRAM_EN);
		writel(val, hs->iobase + R_SF_TIMING_CTRL);
	} else {
		val = readl(hs->iobase + R_SF_TIMING_CTRL);
		val &= ~F_SF_00C8_CS_PROGRAM_M;
		writel(val, hs->iobase + R_SF_TIMING_CTRL);
		val &= ~F_SF_00C8_CS_PROGRAM_EN;
		writel(val, hs->iobase + R_SF_TIMING_CTRL);
	}
}

static u32 hc_spi_addr_mapping(u32 dmaaddr, bool access)
{
	u32 pioaddr;
	u32 tmp_addr = dmaaddr & 0x00C00000;

	if (tmp_addr >= 0x00C00000)
		pioaddr = (dmaaddr - 0x00C00000);
	else if (tmp_addr >= 0x00800000)
		pioaddr = (dmaaddr - 0x00400000);
	else if (tmp_addr >= 0x00400000)
		pioaddr = (dmaaddr + 0x00400000);
	else
		pioaddr = (dmaaddr + 0x00C00000);

	if ((pioaddr & 0x00C00000) == 0x00C00000) {
		if (access)
			REG32_SET_BIT(0xb8870000, BIT(0));
		else
			REG32_CLR_BIT(0xb8870000, BIT(0));
	}
	return pioaddr;
}

static int __hc_hw_spi_transfer_one(struct spi_master *master,
			       struct spi_device *spi,
			       struct spi_transfer *xfer)
{
	struct hc_spi *hs = spi_master_get_devdata(master);
	hs->spi = spi;

	u32 dmaaddr;
	u32 pioaddr;
	u32 copy_left;
	u32 dma_copy_left;
	u8 *src;
	u8 sf_conf1 = 0;
	u8 sf_conf2 = 0;

	writeb(0, hs->iobase + R_SF_CONF0);
	writeb(0, hs->iobase + R_SF_CONF1);
	writel(0, hs->iobase + R_SF_DMA_FLASH_ADDR);
	writel(0, hs->iobase + R_SF_DMA_MEM_ADDR);
	writel(0, hs->iobase + R_SF_SQI_COUNT);
	writel(0, hs->iobase + R_SF_ADDR_CONF);
	writel(0, hs->iobase + R_SF_DMA_CTRL);

	sf_conf1 = readb(hs->iobase + R_SF_CONF1);
	sf_conf2 = readb(hs->iobase + R_SF_CONF2);

	/* cmd */
	sf_conf1 |= F_SF_0099_CODE_HIT;
	writeb(xfer->cmd, hs->iobase + R_SF_CONF0);

	/* addr */
	pioaddr = dmaaddr = (u32)hs->piobase[hs->spi->chip_select];
	if (xfer->addr) {
		sf_conf1 |= F_SF_0099_ADDR_HIT;
		sf_conf2 &= ~(F_SF_009A_CONTINUE_COUNT_EN_M | F_SF_009A_ADDR_BYTES_M);

		switch (xfer->n_addr) {
		case 1:
			sf_conf2 |= F_SF_009A_ADDR_1BYTES;
			break;
		case 2:
			sf_conf2 |= F_SF_009A_ADDR_2BYTES;
			break;
		case 3:
			sf_conf2 |= F_SF_009A_ADDR_3BYTES;
			break;
		case 4:
			sf_conf2 |= F_SF_009A_ADDR_4BYTES;
			writel(F_SF_00B8_ADDR_BYTE_PROG_EN | (*(u8 *)(xfer->addr)), hs->iobase + R_SF_ADDR_CONF);
			break;
		}

		for (int i = xfer->n_addr - 1; i >= 0; i--) {
			dmaaddr |= (*(u8 *)(xfer->addr + i) << (8 * (xfer->n_addr - i - 1)));
		}
	}

	/* dummy */
	if (xfer->dummy_data) {
		sf_conf1 |= F_SF_0099_DUMMY_HIT;

		switch (xfer->dummy_data) {
		case 2:
			sf_conf1 |= F_SF_0099_DUMMY_2_NUM;
			break;
		case 3:
			sf_conf1 |= F_SF_0099_DUMMY_3_NUM;
			break;
		case 4:
			sf_conf1 |= F_SF_0099_DUMMY_4_NUM;
			break;
		case 1:
		default:
			sf_conf1 |= F_SF_0099_DUMMY_1_NUM;
			break;
		}
	}

	/* tx/rx data */
	if (xfer->tx_len) {
		sf_conf1 |= F_SF_0099_DATA_HIT;

		if (xfer->tx_len > 1) {
			sf_conf1 |= F_SF_0099_CONTINUE_WRITE;
			sf_conf2 |= F_SF_009A_CONTINUE_COUNT_EN;
		}

		writeb(sf_conf1, hs->iobase + R_SF_CONF1);
		writeb(sf_conf2, hs->iobase + R_SF_CONF2);

		writel(xfer->tx_len, hs->iobase + R_SF_SQI_COUNT);

		u8 *src = (u8 *)xfer->tx_buf;
		u32 copy_left = xfer->tx_len;

		while (copy_left) {
			if (!IS_ALIGNED((u32)src, DMA_ADDR_ALIGN) || copy_left < DMA_LEN_ALIGN) {

				pioaddr = hc_spi_addr_mapping(dmaaddr, true);

				if ((copy_left >= 4) && ((u32)src % 4 == 0) && ((u32)pioaddr % 4 == 0)) {
					*(u32 *)pioaddr = *(u32 *)(src);
					src += 4;
					copy_left -= 4;
					dmaaddr += 4;
				} else {
					*(u8 *)(pioaddr) = *src;
					src++;
					copy_left--;
					dmaaddr++;
				}

				hc_spi_addr_mapping(dmaaddr, false);

				continue;
			}

			dma_copy_left = ALIGN_DOWN(copy_left, DMA_LEN_ALIGN);
			if (dma_copy_left) {
				if (dma_copy_left > DMA_LEN_MAX)
					dma_copy_left = DMA_LEN_MAX;

				writel(dmaaddr & 0x3FFFFFF, hs->iobase + R_SF_DMA_FLASH_ADDR);
				hc_spi_dma_transfer(hs, src, dma_copy_left, SPI_DMA_TX);
				src += dma_copy_left;
				copy_left -= dma_copy_left;
				dmaaddr += dma_copy_left;
			}
		}
	} else if (xfer->rx_len) {
		sf_conf1 |= F_SF_0099_DATA_HIT;

		if (xfer->rx_len > 1) {
			sf_conf1 |= F_SF_0099_CONTINUE_READ;
			sf_conf2 |= F_SF_009A_CONTINUE_COUNT_EN;
		}

		writeb(sf_conf1, hs->iobase + R_SF_CONF1);
		writeb(sf_conf2, hs->iobase + R_SF_CONF2);

		writel(xfer->rx_len, hs->iobase + R_SF_SQI_COUNT);

		u8 *dst = xfer->rx_buf;
		u32 copy_left = xfer->rx_len;

		while (copy_left) {
			if (!IS_ALIGNED((u32)dst, DMA_ADDR_ALIGN) || copy_left < DMA_LEN_ALIGN) {

				pioaddr = hc_spi_addr_mapping(dmaaddr, true);

				if ((copy_left >= 4) && ((u32)dst % 4 == 0) && ((u32)pioaddr % 4 == 0)) {
					*(u32 *)dst = *(u32 *)(pioaddr);
					dst += 4;
					copy_left -= 4;
					dmaaddr += 4;
				} else {
					*dst = *(u8 *)(pioaddr);
					dst++;
					copy_left--;
					dmaaddr++;
				}

				hc_spi_addr_mapping(dmaaddr, false);

				continue;
			}
			dma_copy_left = ALIGN_DOWN(copy_left, DMA_LEN_ALIGN);
			if (dma_copy_left) {
				if (dma_copy_left > DMA_LEN_MAX)
					dma_copy_left = DMA_LEN_MAX;

				writel(dmaaddr & 0x3FFFFFF, hs->iobase + R_SF_DMA_FLASH_ADDR);
				hc_spi_dma_transfer(hs, dst, dma_copy_left, SPI_DMA_RX);
				dst += dma_copy_left;
				copy_left -= dma_copy_left;
				dmaaddr += dma_copy_left;
			}
		}
	} else {
		writeb(sf_conf1, hs->iobase + R_SF_CONF1);
		writeb(sf_conf2, hs->iobase + R_SF_CONF2);

		pioaddr = hc_spi_addr_mapping(dmaaddr, true);

		*(u8 *)(pioaddr) = 0xFF;

		hc_spi_addr_mapping(dmaaddr, false);

	}
	return 0;
}

static int hc_spi_transfer_one(struct spi_master *master,
			       struct spi_device *spi,
			       struct spi_transfer *xfer)
{
	struct hc_spi *hs = spi_master_get_devdata(master);
	unsigned long speed_hz;
	bool force_pio = false;
	unsigned int mode;

	hs->spi = spi;

	speed_hz = spi->max_speed_hz;
	if (xfer->speed_hz)
		speed_hz = xfer->speed_hz;
	if (speed_hz > hs->sclk)
		speed_hz = hs->sclk;

	hc_spi_set_clk_rate(hs, speed_hz);

	hc_spi_set_clock_mode(hs, spi->mode);

	mode = hc_spi_transfer_setup_mode(hs, xfer);

	if (xfer->hw_transfers)
		return __hc_hw_spi_transfer_one(master, spi, xfer);

	if (xfer->tx_buf != NULL && xfer->len != 0) {
		hc_spi_transfer_tx(hs, (void *)xfer->tx_buf, xfer->len);
	}

	if (xfer->rx_buf != NULL && xfer->len != 0) {
#ifdef CONFIG_SOC_HC15XX
		if ((speed_hz > 27000000) || (mode != SPI_NBITS_SINGLE && speed_hz > 18000000)) {
			force_pio = true;
		}
#endif
		hc_spi_transfer_rx(hs, xfer->rx_buf, xfer->len, force_pio);
	}

	return 0;
}

static void hc_spi_interrupt(uint32_t priv)
{
	struct hc_spi *hs = (struct hc_spi *)priv;

	writel(F_SF_00A0_DMA_LEN_ERR_ST | F_SF_00A0_DMA_INT_ST, hs->iobase + R_SF_INT_ST);
	// writel(0 , hs->iobase + R_SF_DMA_CTRL);
	if (hs->spi)
		writel(0 | hs->spi->chip_select, hs->iobase + R_SF_DMA_CTRL);
	else
		writel(0, hs->iobase + R_SF_DMA_CTRL);

	complete(&hs->completion);

	return;
}

void hc_sf_spi_lock_async(void)
{
	mutex_lock(&sf_lock);
}

void hc_sf_spi_unlock_async(void)
{
	mutex_unlock(&sf_lock);
}

static int hc_spi_prot_open(struct file *filep)
{
	hc_sf_spi_lock_async();
	return 0;
}

static int hc_spi_prot_close(struct file *filep)
{
	hc_sf_spi_unlock_async();
	return 0;
}

static const struct file_operations sf_file_ops = {
	.open = hc_spi_prot_open, /* open */
	.close = hc_spi_prot_close, /* close */
	.read = dummy_read, /* read */
	.write = dummy_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = NULL, /* ioctl */
	.poll = NULL/* poll */
};

static void hc_spi_device_add(struct hc_spi *hs)
{
	register_driver("/dev/sf_protect", &sf_file_ops, 0666, NULL);
	return;
}

static void hc_spi_reset(void)
{
	REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x80, BIT14);
	usleep(10);
	REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x80, BIT14);
}

static int hc_spi_probe(const char *node)
{
	struct spi_master *master;
	struct hc_spi *hs;
	size_t ctlr_size = ALIGN(sizeof(*master), 32);
	struct pinmux_setting *active_state;
	int np;
	int ret;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	master = kzalloc(sizeof(*hs) + ctlr_size, GFP_KERNEL);
	if (!master) {
		return -ENOMEM;
	}

	hc_clk_enable(SFLASH_CLK);
	hc_spi_reset();

	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_TX_DUAL |
			    SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD;
	master->setup = hc_spi_setup;
	master->transfer_one = hc_spi_transfer_one;
	master->set_cs = hc_spi_set_cs;
	master->name = "hc-sf-spi";

	spi_controller_set_devdata(master, (void *)master + ctlr_size);

	hs = spi_controller_get_devdata(master);

        mutex_init(&sf_lock);

	hs->iobase = (void *)&SFSPI0;
	hs->num_cs = 2;

	master->num_chipselect = hs->num_cs;

	hs->piobase = hs->inlinepiobase;

	if (get_processor_id() == 0 && REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1600) {
		hs->piobase[0] = (void *)0xafc00000; /* for CS0 */
		hs->piobase[1] = (void *)0xadc00000; /* for CS1 */
	} else {
		hs->piobase[0] = (void *)0xbf000000; /* for CS0 */
		hs->piobase[1] = (void *)0xbd000000; /* for CS1 */
	}

	hs->irq = (int)&SF_INTR;

	hs->sclk = 54000000;

	hs->active_state = fdt_get_property_pinmux(np, "active");
	if (hs->active_state) {
		pinmux_select_setting(hs->active_state);
	}

	fdt_get_property_u_32_index(np, "sclk", 0, (u32 *)&hs->sclk);
	if (hs->sclk > SFSPI_MAX_CLK)
		hs->sclk = SFSPI_MAX_CLK;

	if (get_processor_id() == 0 && REG32_GET_FIELD2((uint32_t)&MSYSIO0 + 0x0, 16, 16) == 0x1600) {
		hs->dma_force = 1;
	} else {
		hs->dma_force = 0;
	}

	master->max_speed_hz = hs->sclk;

	init_completion(&hs->completion);

	xPortInterruptInstallISR(hs->irq, hc_spi_interrupt, (uint32_t)hs);

	ret = spi_register_master(master);
	if (ret) {
		dev_err(dev, "failed register spi master!\n");
		goto err_free_irq;
	}

	hc_spi_device_add(hs);

	return 0;

err_free_irq:
	xPortInterruptRemoveISR(hs->irq, hc_spi_interrupt);

err_put_master:
	free(master);

	return ret;
}

static int hc_spi_init(void)
{
	int rc = 0;

	rc = hc_spi_probe("/hcrtos/sfspi");

	return rc;
}

module_system(hc_spisf, hc_spi_init, NULL, 0)
