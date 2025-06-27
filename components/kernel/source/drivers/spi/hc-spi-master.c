#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/module.h>
#include <kernel/completion.h>
#include <kernel/drivers/spi.h>
#include <kernel/lib/fdt_api.h>
#include <hcuapi/pinmux.h>
#include <hcuapi/gpio.h>
#include <linux/io.h>
#include <kernel/ld.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <kernel/drivers/hc_clk_gate.h>

#include "spi_reg_struct.h"

#define TMOD_TX_AND_RX				0x00 << 8
#define TMOD_TX_ONLY				0x01 << 8
#define TMOD_RX_ONLY				0x02 << 8
#define MOTOROLA_SPI				0x00 << 4
#define SCPOL_HIGH				0x01 << 6
#define SCPH_HIGH				0x01 << 6
#define SCPOL_LOW				0x00 << 6
#define SCPH_LOW				0x00 << 6
#define DFS_8BIT				0x07 << 0

#define FIFO_HALF				0x08
#define FIFO_LENGTH				16

#define DMA_TIMEOUT				msecs_to_jiffies(10000)
#define DMA_LEN_MIN				(0x100)

#define HC_SPIM_CLK				198 * 1000 * 1000

struct hc_spim_priv {
	int			irq;
	int			err;

	uint32_t		dma;
	uint32_t		clk;
	uint32_t		sclk;
	uint32_t		mode;

	void			*base;

	struct pinctrl		*pctl;
	struct pinctrl_state	*active;

	struct spi_device	*spi;
	struct platform_device	*pdev;

	struct completion	completion;

	uint32_t 		num_cs;
	uint32_t 		cs_padctl;

};

static void hc_spim_interrupt(uint32_t param)
{
	struct hc_spim_priv *priv	= (struct hc_spim_priv *)param;
	spi_reg_t *reg 			= (spi_reg_t *)priv->base;
	uint32_t status			= reg->isr.val;
	uint32_t temp;

	temp = (reg->msticr.val >> 5) & 0x1;
	complete(&priv->completion);

	return;
}

static void hc_spim_set_cs(struct spi_device *spi, bool enable)
{
	struct hc_spim_priv *priv = spi_master_get_devdata(spi->master);
	if (priv->cs_padctl != PINPAD_INVALID)
		gpio_set_output(priv->cs_padctl, enable);

	return;
}

static void hc_spim_calcu_div(struct hc_spim_priv *priv, uint32_t clk)
{
	uint16_t div = 4, i = 4;
	spi_reg_t *reg = (spi_reg_t *)priv->base;
	uint32_t tmp1 = HC_SPIM_CLK, tmp2 = HC_SPIM_CLK;

	reg->ssienr.ssi_en = 0x00;

	/* div value must be even */
	for (i; i <= 0x20; i += 2) {
		tmp1 = HC_SPIM_CLK / i;
		if (abs(tmp1 - clk) < tmp2) {
			tmp2 = abs(tmp1 - clk);
			div = i;
		}
	}

	reg->baudr.sckdv = div;

	reg->ssienr.ssi_en = 0x01;
}

static void hc_spim_init(struct hc_spim_priv *priv)
{
	spi_reg_t *reg = (spi_reg_t *)priv->base;

	reg->ssienr.ssi_en = 0x00;

	if (priv->cs_padctl != PINPAD_INVALID) {
		gpio_configure(priv->cs_padctl, GPIO_DIR_OUTPUT);
		gpio_set_output(priv->cs_padctl, 1);
	}

	reg->ctrl0.tmod	= 0x00;
	reg->ctrl0.dfs	= 0x07;
	reg->ctrl0.frf	= 0x00;
	reg->txftlr.tft = 0x08;
	reg->rxftlr.rft = 0x08;

	switch (priv->mode) {
	case 0:
		reg->ctrl0.scph	= 0;
		reg->ctrl0.scpol= 0;
		break;
	case 1:
		reg->ctrl0.scph	= 1;
		reg->ctrl0.scpol= 0;
		break;
	case 2:
		reg->ctrl0.scph	= 0;
		reg->ctrl0.scpol= 1;
		break;
	case 3:
		reg->ctrl0.scph	= 1;
		reg->ctrl0.scpol= 1;
		break;
	default:
		dev_err(&pdev->dev, "spi mode select error\n");
		break;
	}

	hc_spim_calcu_div(priv, priv->clk);

	reg->imr.val		= 0x00;
	reg->ser.ser		= 0x01;
	reg->mvcr.val		= 0x00;

	reg->spi_lsbf.val	= 0x00; //获取 LSB/MSB
}

static int hc_spi_busy(struct hc_spim_priv *priv)
{
	unsigned long timeout_jiffies = 10000;
	unsigned long deadline;
	int timeout = 0;
	spi_reg_t *reg = (spi_reg_t *)priv->base;

	deadline = jiffies + timeout_jiffies;
	while (!timeout) {
		if (time_after_eq(jiffies, deadline))
			timeout = 1;

		if (reg->sr.busy == 0)
			return 0;

		cond_resched();
	}

	dev_err(&priv->pdev->dev, "spim busy time out\n");

	return -ETIMEDOUT;
}

static int hc_spi_rxfifo_no_empty(struct hc_spim_priv* priv)
{
	unsigned long timeout_jiffies = 10000;
	unsigned long deadline;
	int timeout = 0;
	spi_reg_t *reg = (spi_reg_t *)priv->base;

	deadline = jiffies + timeout_jiffies;
	while (!timeout) {
		if (time_after_eq(jiffies, deadline))
			timeout = 1;

		if (reg->sr.rfne == 1)
			return 0;

		cond_resched();
	}

	dev_err(&priv->pdev->dev, "rx fifo no empty time out\n");

	return -ETIMEDOUT;
}

static int hc_spi_txfifo_empty(struct hc_spim_priv* priv)
{
	unsigned long timeout_jiffies = 10000;
	unsigned long deadline;
	int timeout = 0;
	spi_reg_t *reg = (spi_reg_t *)priv->base;

	deadline = jiffies + timeout_jiffies;
	while (!timeout) {
		if (time_after_eq(jiffies, deadline))
			timeout = 1;

		if (reg->sr.tfe == 1)
			return 0;

		cond_resched();
	}

	dev_err(&priv->pdev->dev, "tx fifo full empty out\n");

	return -ETIMEDOUT;

}

static int hc_spi_txfifo_no_full(struct hc_spim_priv* priv)
{
	unsigned long timeout_jiffies = 10000;
	unsigned long deadline;
	int timeout = 0;
	spi_reg_t *reg = (spi_reg_t *)priv->base;

	deadline = jiffies + timeout_jiffies;
	while (!timeout) {
		if (time_after_eq(jiffies, deadline))
			timeout = 1;

		if (reg->sr.tfnf == 1)
			return 0;

		cond_resched();
	}

	dev_err(&priv->pdev->dev, "tx fifo no full time out\n");

	return -ETIMEDOUT;


}

static void hc_spi_pio_tx(struct hc_spim_priv *priv, uint8_t *buf, uint32_t len)
{

	int index = 0;
	spi_reg_t *reg = (spi_reg_t *)priv->base;

	reg->ssienr.ssi_en = 0x00;
	reg->ctrl1.ndf = 0x0;
	reg->ctrl0.tmod = 0x01;
	reg->ssienr.ssi_en = 0x01;

	while (index != len) {
		hc_spi_txfifo_no_full(priv);
		reg->dr_low[0] = buf[index++];
	}

	hc_spi_txfifo_empty(priv);

	return;
}

static void hc_spi_pio_rx(struct hc_spim_priv *priv, uint8_t *buf, uint32_t len)
{
	spi_reg_t *reg = (spi_reg_t *)priv->base;
	int index = 0;

	reg->ssienr.ssi_en = 0x00;
	reg->ctrl1.ndf = len - 1;
	reg->ctrl0.tmod = 0x02;
	//reg->rx_smp_dly.val = 0x01;
	reg->ssienr.ssi_en = 0x01;

	reg->dr_low[0] = 0x00;
	while (index != len) {
		hc_spi_rxfifo_no_empty(priv);
		buf[index++] = reg->dr_low[0];
	}
}

void hc_spi_dma_tx(struct hc_spim_priv *priv, void *buf, unsigned len)
{
	dma_addr_t tx_dma;
	spi_reg_t *reg = (spi_reg_t *)priv->base;

	tx_dma = dma_map_single(NULL, buf, len, DMA_TO_DEVICE);
	if (dma_mapping_error(&priv->pdev->dev, tx_dma)) {
		dev_err(&priv->pdev->dev, "DMA map tx fail!\n");
		return;
	}

	init_completion(&priv->completion);

	reg->ssienr.ssi_en	= 0x00;
	reg->ctrl0.val		= 0x107;
	reg->ctrl1.ndf		= len - 1;
	reg->dmamwar		= tx_dma & 0x0fffffff;
	reg->dmamrar		= tx_dma & 0x0fffffff;
	reg->imr.val		= 0x20;
	reg->ssienr.ssi_en	= 0x01;
	reg->dmacr.dmacr	= 0x01;

	if (wait_for_completion_timeout(&priv->completion, DMA_TIMEOUT) == 0) {
		dev_err(&priv->pdev->dev, "DMA tx time out!\n");
	}
	dma_unmap_single(NULL, tx_dma, len, DMA_TO_DEVICE);

	return;
}

void hc_spi_dma_rx(struct hc_spim_priv *priv, void *buf, unsigned len)
{
	dma_addr_t rx_dma;
	spi_reg_t *reg = (spi_reg_t *)priv->base;

	rx_dma = dma_map_single(NULL, buf, len, DMA_FROM_DEVICE);
	if (dma_mapping_error(&priv->pdev->dev, rx_dma)) {
		dev_err(&priv->pdev->dev, "DMA map rx fail!\n");
		return;
	}

	init_completion(&priv->completion);

	reg->ssienr.ssi_en	= 0x00;
	reg->ctrl0.val		= 0x207;
	reg->rx_smp_dly.val	= 0x03;
	reg->imr.val		= 0x20;
	reg->ctrl1.ndf		= len - 1;
	reg->dmamwar		= rx_dma & 0x0fffffff;
	reg->ssienr.ssi_en	= 0x01;
	reg->dmacr.dmacr	= 0x01;

	if (wait_for_completion_timeout(&priv->completion, DMA_TIMEOUT) == 0) {
		dev_err(&priv->pdev->dev, "DMA rx time out!\n");
	}

	dma_unmap_single(NULL, rx_dma, len, DMA_FROM_DEVICE);

	return;
}

static void hc_spi_transfer_tx(struct hc_spim_priv *priv, void *buf, unsigned len)
{
	unsigned size = 0x10000;
	unsigned index = 0;
	unsigned tmp = len;

	if (!priv->dma || len < DMA_LEN_MIN) {
		if (len > size) {
			while (index != tmp) {
				if (len > size)
					len -= size;
				else
					size = len;
				hc_spi_pio_tx(priv, buf + index, size);
				index += size;
			}
		} else
			hc_spi_pio_tx(priv, buf, len);
	} else {
		if (len > size) {
			while (index != tmp) {
				if (len > size)
					len -= size;
				else
					size = len;
				hc_spi_dma_tx(priv, buf + index, size);
				index += size;
			}
		} else
			hc_spi_dma_tx(priv, buf, len);
	}

	return;
}

static void hc_spi_transfer_rx(struct hc_spim_priv *priv, void *buf,
			       unsigned len)
{
	unsigned size = 0x10000;
	unsigned index = 0;
	unsigned tmp = len;

	if (len > size) {
		while (index != tmp) {
			if (len > size)
				len -= size;
			else
				size = len;
			hc_spi_dma_rx(priv, buf + index, size);
			index += size;
		}
	} else
		hc_spi_dma_rx(priv, buf, len);

	return;
}

static int hc_spim_setup(struct spi_device *spi)
{
	return 0;
}

static int hc_spim_transfer_one(struct spi_controller *master, 
		struct spi_device *spi,
		struct spi_transfer *xfer)
{
	struct hc_spim_priv *priv = spi_master_get_devdata(master);
	spi_reg_t *reg = (spi_reg_t *)priv->base;
	uint8_t *rx_buf = xfer->rx_buf;
	uint8_t *tx_buf = &((uint8_t *)xfer->tx_buf)[0];
	size_t len = xfer->len;

	uint32_t speed_hz = spi->max_speed_hz;

	if (xfer->speed_hz)
		speed_hz = xfer->speed_hz;
	if (speed_hz > priv->clk)
		speed_hz = priv->clk;

	hc_spim_calcu_div(priv, speed_hz);

	if (xfer->tx_buf != NULL && xfer->len != 0)
		if (speed_hz > 10 * 1000000)
			hc_spim_calcu_div(priv, 10 * 1000000);
		hc_spi_transfer_tx(priv, (void *)tx_buf, len);

	if (xfer->rx_buf != NULL && xfer->len != 0) {
		hc_spi_transfer_rx(priv, (void *)rx_buf, len);
	}

	//hc_spi_busy(priv);
	return 0;
}

static int hc_spim_probe(const char *node)
{
	struct spi_controller *master;
	struct hc_spim_priv *priv;
	size_t ctlr_size = ALIGN(sizeof(*master), 32);
	struct pinmux_setting *active_state;
	int np, ret;

	np = fdt_node_probe_by_path(node);
	if (np < 0) {
		return 0;
	}

	master = kzalloc(sizeof(*priv) + ctlr_size, GFP_KERNEL);
	if (!master)
		return -ENOMEM;

	hc_clk_enable(SPI_MASTER_CLK);

	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;

	master->name		= "hc-spi";
	master->setup		= hc_spim_setup;
	master->set_cs		= hc_spim_set_cs;
	master->transfer_one	= hc_spim_transfer_one;

	spi_controller_set_devdata(master, (void *)master + ctlr_size);

	priv = spi_controller_get_devdata(master);

	priv->base = (void *)&SPIMASTER ;
	priv->num_cs = 1;
	master->num_chipselect = priv->num_cs;

	priv->irq = (int)&SPI_MASTER_INTR;

	active_state = fdt_get_property_pinmux(np, "active");
	if (active_state) {
		pinmux_select_setting(active_state);
		free(active_state);
	}
	priv->cs_padctl = PINPAD_INVALID;
	priv->clk = 50 * 1000 * 1000;

	fdt_get_property_u_32_index(np, "cs-pin", 0, (u32 *)&priv->cs_padctl);
	fdt_get_property_u_32_index(np, "sclk", 0, (u32 *)&priv->clk);
	fdt_get_property_u_32_index(np, "dma-mode", 0, (u32 *)&priv->dma);
	fdt_get_property_u_32_index(np, "transfer-mode", 0, (u32 *)&priv->mode);

	master->max_speed_hz = priv->clk;

	init_completion(&priv->completion);

	xPortInterruptInstallISR(priv->irq, hc_spim_interrupt, (uint32_t)priv);

	hc_spim_init(priv);

	ret = spi_register_master(master);

	return 0;
}

static int hc_spim_initialize(void)
{
	int rc = 0;

	rc = hc_spim_probe("/hcrtos/spi-master");

	return rc;
}

module_system(spi_master, hc_spim_initialize, NULL, 0)
