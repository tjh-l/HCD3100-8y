#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <kernel/io.h>
#include <kernel/ld.h>
#include <kernel/types.h>
#include <kernel/module.h>
#include <kernel/soc/soc_common.h>
#include <hcuapi/sci.h>
#include <hcuapi/pinmux.h>
#include <kernel/lib/fdt_api.h>
#include <generated/br2_autoconf.h>

#include <kernel/drivers/hc_clk_gate.h>

#define SCI_PARITY_NONE 0x0000
#define SCI_PARITY_EVEN 0x0001
#define SCI_PARITY_ODD 0x0002

#define IIR_PRIOR 0x06

/*
* UART mode master clock, Programmable baud generator divides 
* any input clock by 1 to (216 – 1) and generates the 16X clock 
*/
/* 1.843Mhz, as normal speed mode */
#define UART_CLK_SRC_NORMAL_SPEED_MODE 1843000
/* 54Mhz, as high speed mode */
#define UART_CLK_SRC_HIGH_SPEED_MODE 54000000
/* 14.7456Mhz, as high speed mode, using DPLL, for baudrate 921600 */
#define UART_CLK_SRC_HIGH_SPEED_MODE_DPLL 14745600

#define UART_NORMAL_BAUDRATE		115200
#define UART_GPIO_TX_BAUDRATE		9600
#define TX_SAMPLE_TICK			102

#define UART_TX_TMOUT 5
#define UART_EVENT_RX_AVAIL (1 << 0)
#define UART_EVENT_TX_EMPTY (1 << 1)

#ifndef CONFIG_MINIBOOT_DISABLE_LOG

struct sci_device {
	int id;
	uart_reg_t *reg;
	uint32_t baud_rate;
	struct pinmux_setting *active_state;
};

static int uart_gate_array[4] = {
	UART1_CLK,
	UART2_CLK,
	UART3_4_CLK,
	UART3_4_CLK,
};

static struct sci_device g_sci_dev;

static void sci_16550uart_setting(struct sci_device *dev,
				  struct sci_setting *setting)
{
	if (NULL == dev)
		return;

	switch (setting->parity_mode) {
	case PARITY_EVEN:
		dev->reg->ulcr.pen = 0x1; /* Parity Enable */
		dev->reg->ulcr.eps = 0x1; /* selects even parity */
		break;
	case PARITY_ODD:
		dev->reg->ulcr.pen = 0x1; /* Parity Enable */
		dev->reg->ulcr.eps = 0x0; /* selects odd parity */
		break;
	default:
		dev->reg->ulcr.pen = 0x0; /* no Parity Enable */
		dev->reg->ulcr.eps = 0x0; /* selects odd parity */
		break;
	};

	switch (setting->bits_mode) {
	case bits_mode_default:
		dev->reg->ulcr.wls = 0x3; /* set  word length as 8 bits */
		dev->reg->ulcr.stb = 0x0; /* set stop bit length  as 1 bit */
		break;
	case bits_mode1:
		dev->reg->ulcr.wls = 0x0; /* set  word length as 5 bits */
		dev->reg->ulcr.stb = 0x1; /* set stop bit length  as 1.5 bit */
		break;
	case bits_mode2:
		dev->reg->ulcr.wls = 0x1; /* set  word length as 6 bits */
		dev->reg->ulcr.stb = 0x1; /* set stop bit length  as 2 bit */
		break;
	case bits_mode3:
		dev->reg->ulcr.wls = 0x2; /* set  word length as 7 bits */
		dev->reg->ulcr.stb = 0x1; /* set stop bit length  as 2 bit */
		break;
	case bits_mode4:
		dev->reg->ulcr.wls = 0x3; /* set  word length as 8 bits */
		dev->reg->ulcr.stb = 0x1; /* set stop bit length  as 2 bit */
		break;
	}

	/* Enable and reset  FIFO,  threshold is 4 bytes */
	dev->reg->uiir.val = 0x47;

	dev->reg->ulsr.val = 0x0; /* Reset line status */
	dev->reg->umcr.val = 0x0;
	dev->reg->umcr.dtr = 0x1;
	dev->reg->umcr.rts = 0x1; /* Reset line status */
}

static void sci_16550uart_reset(struct sci_device *dev)
{
	switch (dev->id) {
	case 0:
		REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x80, BIT16);
		usleep(100);
		REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x80, BIT16);
		break;
	case 1:
		REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x80, BIT17);
		usleep(100);
		REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x80, BIT17);
		break;
	case 2:
		REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x80, BIT9);
		usleep(100);
		REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x80, BIT9);
		break;
	case 3:
		REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x80, BIT10);
		usleep(100);
		REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x80, BIT10);
		break;
	default:
		break;
	}
}

static void sci_16550uart_set_mode(struct sci_device *dev, uint32_t parity)
{
	uint32_t div = 0;
	uint32_t tmp = 0;
	uint32_t clk_src;

	if (NULL == dev)
		return;

	if (dev->baud_rate <= 115200) {
		/* normal speed mode */
		dev->reg->dev_ctrl.fin_sel = 0x0;
		dev->reg->ulcr.dlab = 0x1; /* start to set UART Divisor Latch	*/
		dev->reg->urbr_utbr.val = 0x1 & 0xff;
		/* configuare baud rate as 115200Hz */
		dev->reg->uier.val = (0x1 >> 8) & 0xff;
		dev->reg->ulcr.dlab = 0x0; /*finish seting UART Divisor Latch	*/
		clk_src = UART_CLK_SRC_NORMAL_SPEED_MODE;
	} else {
		/* TODO */
	}

	/*while write first char after mode set, takes 5ms*/
	//dev->timeout = 20000;

	/* Disable all interrupt */
	dev->reg->uier.val = 0;

	div = clk_src / (16 * dev->baud_rate);
	if ((clk_src % (16 * dev->baud_rate)) > (8 * dev->baud_rate))
		div += 1;

	dev->reg->ulcr.wls = 0x3; /* set  word length as 8 bits */
	dev->reg->ulcr.stb = 0x0; /* set stop bit length  as 1 bit */
	//dev->reg->ulcr.stb = 0x1;  /* set stop bit length  as 2 bit */

	dev->reg->ulcr.pen = 0x1; /* Parity Enable */
	dev->reg->ulcr.eps = 0x1; /* selects even parity */
	dev->reg->ulcr.sp = 0x0; /* disbale Stick Parity */
	dev->reg->ulcr.break_ctrl = 0x0;
	dev->reg->ulcr.dlab = 0x1; /* start to set UART Divisor Latch  */

	//must be delete before release
	//dev->reg->dev_ctrl.fin_sel = 0x1; /*set high speed mode */

	dev->reg->urbr_utbr.val = div & 0xff;
	dev->reg->uier.val = (div >> 8) & 0xff;

	switch (parity & 0x03) {
	case SCI_PARITY_EVEN:
		dev->reg->ulcr.pen = 0x1; /* Parity Enable */
		dev->reg->ulcr.eps = 0x1; /* selects even parity */
		break;
	case SCI_PARITY_ODD:
		dev->reg->ulcr.pen = 0x1; /* Parity Enable */
		dev->reg->ulcr.eps = 0x0; /* selects odd parity */
		break;
	default:
		dev->reg->ulcr.pen = 0x0; /* no Parity Enable */
		dev->reg->ulcr.eps = 0x0; /* selects odd parity */
		break;
	};

	tmp = (parity >> 6) & 0x4;
	tmp |= (~(parity >> 4) & 0x03);

	dev->reg->ulcr.wls = tmp; /* Word Length Select  */
	dev->reg->ulcr.dlab = 0x0; /*finish seting UART Divisor Latch  */

	/* Enable and reset  FIFO,  threshold is 4 bytes */
	dev->reg->uiir.val = 0x47;

	dev->reg->ulsr.val = 0x0; /* Reset line status */
	dev->reg->umcr.val = 0x0;
	dev->reg->umcr.dtr = 0x1;
	dev->reg->umcr.rts = 0x1; /* Reset line status */

	/* Enable receiver interrupt */
	dev->reg->uier.val = 0x0;
	dev->reg->uier.erdvi = 1;
	//dev->reg->tx_fifocnt.val = 16;
	dev->reg->uier.erlsi = 1; /* Enable RX & timeout interrupt */

	dev->reg->uier.ethrei = 1; /* Enable TX interrupt */
}

static void sci_16550uart_write_bytes_from_isr(struct sci_device *dev, const char *buf, int32_t bytes)
{
#define FIFO_MAX 16
	int sent = 0;
	char ch;
	bool add_cr = true;
	int fifo = 0;

	if (NULL == dev || NULL == buf || bytes == 0)
		return;

	while (!dev->reg->ulsr.thre)
		;

	do {
		fifo = 0;
		do {
			ch = buf[sent];
			if (ch == '\n' && add_cr) {
				dev->reg->urbr_utbr.val = '\r';
				fifo++;
				if (fifo == FIFO_MAX) {
					add_cr = false;
					break;
				}
			}
			dev->reg->urbr_utbr.val = ch;
			fifo++;
			sent++;
			add_cr = true;
		} while (fifo < FIFO_MAX && sent < bytes);

		/* timeout, do polling wait */
		while (!dev->reg->ulsr.thre)
			;
	} while (sent < bytes);

	return;
}

ssize_t sci_write(const char *buffer, size_t buflen)
{
	sci_16550uart_write_bytes_from_isr(&g_sci_dev, buffer, buflen);

	return buflen;
}

static const char *fdt_get_stdio_path(void)
{
	return "/hcrtos/boot-stdio";
}

void stdio_initalize(void)
{
	const char *path = NULL;
	int np;

	np = fdt_get_node_offset_by_path(fdt_get_stdio_path());
	assert(np >= 0);
	assert(fdt_get_property_string_index(np, "serial0", 0, &path) == 0);
	assert(path != NULL);

	np = fdt_node_probe_by_path(path);
	if (np < 0)
		return;

	if (!strcmp(path, "/hcrtos/uart@0")) {
		g_sci_dev.id = 0;
		g_sci_dev.reg = (uart_reg_t *)&UART0;
	} else if (!strcmp(path, "/hcrtos/uart@1")) {
		g_sci_dev.id = 1;
		g_sci_dev.reg = (uart_reg_t *)&UART1;
	} else if (!strcmp(path, "/hcrtos/uart@2")) {
		g_sci_dev.id = 2;
		g_sci_dev.reg = (uart_reg_t *)&UART2;
	} else if (!strcmp(path, "/hcrtos/uart@3")) {
		g_sci_dev.id = 3;
		g_sci_dev.reg = (uart_reg_t *)&UART3;
	}

	g_sci_dev.baud_rate = 115200;
	g_sci_dev.active_state = fdt_get_property_pinmux(np, "active");
	hc_clk_enable(uart_gate_array[g_sci_dev.id]);

	sci_16550uart_reset(&g_sci_dev);
	sci_16550uart_set_mode(&g_sci_dev, 0);
	pinmux_select_setting(g_sci_dev.active_state);
}

#else
void stdio_initalize(void)
{
}
ssize_t sci_write(const char *buffer, size_t buflen)
{
	return buflen;
}
#endif
