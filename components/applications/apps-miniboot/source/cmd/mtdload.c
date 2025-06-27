#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/io.h>
#include <kernel/ld.h>
#include <kernel/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <kernel/lib/fdt_api.h>
#include <hcuapi/pinmux.h>

#include <image.h>
#include <mtdload.h>

#if defined CONFIG_SOC_HC16XX || CONFIG_SOC_HC15XX
#define SF_BASE 0xb882e000
#endif

static void spi_dma_transfer_rx(void *rx_buf, uint32_t rx_len, uint32_t offset)
{
#define DMA_ADDR_ALIGN (32)
	int use_bounce_buf = 0;
	u32 val;
	void *tmp = rx_buf;
	offset += 0xafc00000;

	if (!IS_ALIGNED((u32)rx_buf, DMA_ADDR_ALIGN)) {
		tmp = memalign(32, rx_len + 32);
		use_bounce_buf = 1;
	}

	cache_flush_invalidate(tmp, rx_len);

	REG32_WRITE(SF_BASE + 0x58, (unsigned int)tmp & 0x0fffffff);

	REG32_WRITE(SF_BASE + 0x60, rx_len);

	REG32_SET_BIT(SF_BASE + 0x64, BIT20);
	REG32_SET_BIT(SF_BASE + 0x64, BIT19);
	REG32_SET_BIT(SF_BASE + 0x64, BIT18);
	REG32_SET_BIT(SF_BASE + 0x64, BIT17);

	REG32_SET_BIT(SF_BASE + 0xa0, BIT0);
	REG32_SET_BIT(SF_BASE + 0x64, BIT5);

	while (!REG32_GET_BIT(SF_BASE + 0xa0, BIT0));

	REG32_SET_BIT(SF_BASE + 0xa0, BIT0);

	cache_invalidate(tmp, rx_len);

	if (use_bounce_buf) {
		memcpy(rx_buf, tmp, rx_len);
		free(tmp);
	}
}

static void spi_transfer(void *rx_buf, uint32_t rx_len, uint32_t offset, int wire)
{
	uint32_t i, pio_base = 0xafc00000;
	uint8_t tx_buf[5] = { 0 };
	uint8_t tx_len;
	uint32_t *rx_buf_tmp = (uint32_t *)rx_buf;
	uint32_t rx_len_tmp;
	uint8_t *rx_buf_tmp_2 = (uint8_t *)rx_buf;

	if (REG8_READ(0xb8800003) == 0x16 || REG8_READ(0xb8800003) == 0x15) {
		if (REG32_GET_BIT(0xb8800223, BIT0))
			pio_base = 0xbf000000;
		else
			pio_base = 0xafc00000;
	}

	/* set tx send address + dummy */
	tx_buf[1] = (offset >> 16) & 0xff;
	tx_buf[2] = (offset >>  8) & 0xff;
	tx_buf[3] = (offset >>  0) & 0xff;
	tx_buf[4] = 0xff;
	if (wire == 1) {
		tx_buf[0] = 0x03;
		tx_len = 4;
	} else {
		tx_buf[0] = 0x3b;
		tx_len = 5;
	}

	/* cs pull down */
	REG32_SET_BITS(SF_BASE + 0xc8, 0x03 << 25, 0x03 << 25);

	/* set tx transfer mode */
	REG32_SET_BITS(SF_BASE + 0x98, 0x00 << 16, 0x07 << 16);

	/* send tx data */
	for (i = 0; i < tx_len; i++)
		REG8_WRITE(pio_base, tx_buf[i]);

	/* set rx transfer mode */
	if (wire == 1)
		REG32_SET_BITS(SF_BASE + 0x98, 0x00 << 16, 0x07 << 16); // mode 0
	else
		REG32_SET_BITS(SF_BASE + 0x98, 0x01 << 16, 0x07 << 16); // mode 1

	/* receive data */
	if (REG8_READ(0xb8800003) == 0x15) { /* pio rx mode */
		rx_len_tmp = rx_len % 4;
		rx_len /= 4;

		for (i = 0; i < rx_len; i++)
			rx_buf_tmp[i] = REG32_READ(pio_base);

		for (i = 0; i < rx_len_tmp; i++)
			rx_buf_tmp_2[rx_len * 4 + i] = REG8_READ(pio_base);

	} else if (REG8_READ(0xb8800003) == 0x16) { /* dma rx mode */
		spi_dma_transfer_rx(rx_buf, rx_len, offset);
	}

	/* cs pull up */
	REG32_SET_BITS(SF_BASE + 0xc8, 0x00 << 25, 0x03 << 25);
}

static void spi_nor_read(void *buf, uint32_t offset, uint32_t size)
{
	int np, div_clk, wire = 0;
	uint32_t clk = 0;
	uint32_t rx_len;
	uint32_t *rx_buf = (uint32_t *)buf;
	struct pinmux_setting *active_state;

	np = fdt_node_probe_by_path("/hcrtos/sfspi");
	fdt_get_property_u_32_index(np, "sclk", 0, (u32 *)&clk);

	active_state = fdt_get_property_pinmux(np, "active");
	if (active_state) {
		pinmux_select_setting(active_state);
		free(active_state);
	}

	np = fdt_node_probe_by_path("/hcrtos/sfspi/spi_nor_flash");
	fdt_get_property_u_32_index(np, "spi-rx-bus-width", 0, (u32 *)&wire);

	if (REG8_READ(0xb8800003) == 0x16) {
		REG32_SET_BITS(0xb880007c, 0x02 << 14, 0x03 << 14); //使用198M时钟
		if (clk >= 100000000)
			REG32_SET_BITS(0xb882e098, 0x00 << 24, 0x0f << 24);
		else if (clk >= 54000000)
			REG32_SET_BITS(0xb882e098, 0x01 << 24, 0x0f << 24);
		else if (clk >= 27000000)
			REG32_SET_BITS(0xb882e098, 0x02 << 24, 0x0f << 24);
	} else if (REG8_READ(0xb8800003) == 0x15) {
		if (clk >= 54000000)
			REG32_SET_BITS(SF_BASE + 0x98, 0x00 << 24, 0x0f << 24);
		else if (clk >= 27000000)
			REG32_SET_BITS(SF_BASE + 0x98, 0x01 << 24, 0x0f << 24);
	}

	/*manual control cs pin*/
	REG32_SET_BIT(SF_BASE + 0xc8, BIT24);

	/*cancel controller send cmd, dummy, addr, only send data*/
	REG32_SET_BIT(SF_BASE + 0x98, BIT8);
	REG32_CLR_BIT(SF_BASE + 0x98, BIT9);
	REG32_CLR_BIT(SF_BASE + 0x98, BIT10);
	REG32_CLR_BIT(SF_BASE + 0x98, BIT11);

	spi_transfer(buf, size, offset, wire);
}

int mtdloadraw(unsigned char dev_type, void *dtb, u32 dtb_size, u32 start, u32 size)
{
	u32 length = (dtb_size < size) ? dtb_size : size;

	if (dev_type == IH_DEVT_SPINOR) {
		spi_nor_read(dtb, start, length);
	} else if (dev_type == IH_DEVT_SPINAND) {
		return -1;
	}

	return -1;
}

int mtdloaduImage(unsigned char dev_type, u32 start, u32 size)
{
	image_header_t hdr = { 0 };
	ssize_t length = 0;
	unsigned int bootsize;

	if (dev_type == IH_DEVT_SPINOR) {
		length = image_get_header_size();
		spi_nor_read(&hdr, start, length);

		length = image_get_image_size(&hdr);
		if ((void *)image_load_addr == NULL)
			image_load_addr = (unsigned long)malloc(length);
		else
			image_load_addr = (unsigned long)realloc((void *)image_load_addr, length);
		if (!image_load_addr) {
			printf("ERROR: malloc for image with size %ld\n", length);
			return -1;
		}
		printf("default image load address = 0x%08lx\n", image_load_addr);
		spi_nor_read((void *)image_load_addr, start, length);
		return 0;
	} else if (dev_type == IH_DEVT_SPINAND) {
		return -1;
	}

	return -1;
}
