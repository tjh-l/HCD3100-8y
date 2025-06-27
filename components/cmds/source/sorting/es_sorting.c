#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <hcuapi/gpio.h>
#include <hcuapi/i2c-master.h>
#include <kernel/lib/console.h>
#include <kernel/delay.h>


#define CLOCK_12M	PINPAD_L24
#define RESET_PIN	PINPAD_L23	

#define ES_MODE_PIN		PINPAD_L22
#define ES_PIN_03		PINPAD_B00
#define ES_PIN_04		PINPAD_L25
#define ES_PIN_06		PINPAD_L26

static int i2c_fd = -1;

static void open_i2c(void)
{
	i2c_fd = open("/dev/i2c1", O_RDWR);
	if(i2c_fd < 0)
	{
		printf("open i2c1 error fd=%d\n", i2c_fd);
		return;
	}	
}

static void close_i2c(void)
{
        close(i2c_fd);
}

void printf_red(const char *s)
{
    printf("\033[0m\033[1;31m%s\033[0m", s);
}

void printf_green(const char *s)
{
    printf("\033[0m\033[1;32m%s\033[0m", s);
}

static int i2c_rw_test(int argc, char **argv)
{
	int i;
        struct i2c_transfer_s xfer;
        struct i2c_msg_s i2c_msg[3] = {0};

	uint8_t wbuf[1] = {8};
	uint8_t rbuf[256] = {0};

	open_i2c();

	i2c_msg[0].addr = 0x60;
	i2c_msg[0].flags = 0x00;
	i2c_msg[0].buffer = wbuf;
	i2c_msg[0].length = 1;

	i2c_msg[1].addr = 0x50;
	i2c_msg[1].flags = 0x00;
	i2c_msg[1].buffer = wbuf;
	i2c_msg[1].length = 1;

	i2c_msg[2].addr = 0x50;
	i2c_msg[2].flags = 0x01;
	i2c_msg[2].buffer = rbuf;
	i2c_msg[2].length = 128;

        xfer.msgv = i2c_msg;
        xfer.msgc = 3;

        ioctl(i2c_fd,I2CIOC_TRANSFER,&xfer);
	close_i2c();
	printf("i2c eddc read test\n");
	for (i = 0; i < 128; i++) {
		printf("0x%02x ", rbuf[i]);
		if (i > 0 && i % 16 == 0)
			printf("\n");
	}

	printf("\n");

	return 0;
}

static void es_i2c_write(uint8_t addr, uint8_t data)
{
        struct i2c_transfer_s xfer_write;
        struct i2c_msg_s i2c_msg_write[2] = {0};

        uint8_t buf[2] = {addr, data};
        i2c_msg_write[0].addr = 0x35;
        i2c_msg_write[0].flags = 0x0;
        i2c_msg_write[0].buffer = buf;
        i2c_msg_write[0].length = 2;

        xfer_write.msgv = i2c_msg_write;
        xfer_write.msgc = 1;

        ioctl(i2c_fd,I2CIOC_TRANSFER,&xfer_write);
}

static void es_i2c_read(uint8_t addr, uint8_t *data)
{
        struct i2c_transfer_s xfer_read;
        struct i2c_msg_s i2c_msg_read[2] = {0};

        i2c_msg_read[0].addr = 0x35;
        i2c_msg_read[0].flags = 0x0;
        i2c_msg_read[0].buffer = &addr;
        i2c_msg_read[0].length = 1;

        i2c_msg_read[1].addr = 0x35;
        i2c_msg_read[1].flags = 0x1;
        i2c_msg_read[1].buffer = data;
        i2c_msg_read[1].length = 1;

        xfer_read.msgv = i2c_msg_read;
        xfer_read.msgc = 2;

        ioctl(i2c_fd,I2CIOC_TRANSFER,&xfer_read);
}


static void enter_es_mode(void)
{
	gpio_configure(ES_MODE_PIN, GPIO_DIR_OUTPUT);
	gpio_configure(RESET_PIN, GPIO_DIR_OUTPUT);
	gpio_set_output(RESET_PIN, GPIO_OUTPUT_HIGH);
	msleep(10);

	gpio_set_output(ES_MODE_PIN, GPIO_OUTPUT_LOW);
	gpio_set_output(RESET_PIN, GPIO_OUTPUT_LOW);
	msleep(50);
	gpio_set_output(RESET_PIN, GPIO_OUTPUT_HIGH);
	msleep(50);
	gpio_set_output(ES_MODE_PIN, GPIO_OUTPUT_HIGH);
}

static int sram_bitst_test(int argc, char **argv)
{
	uint8_t data0, data1;

	open_i2c();	

	enter_es_mode();

	es_i2c_write(0xff, 0x00);
	msleep(10);
	es_i2c_write(0x00, 0x0d);
	msleep(10);
	es_i2c_write(0x01, 0x02);
	msleep(10);
	es_i2c_write(0x07, 0x80);
	msleep(40);
	msleep(10);
	es_i2c_read(0x0b, &data0);
	msleep(10);
	es_i2c_read(0x0f, &data1);

	data0 >>= 7;
	data0 &= 0x01;
	data1 >>= 7;
	data1 &= 0x01;

	int i = 10;
	while (i--) {
		msleep(10);
		es_i2c_read(0x0b, &data0);		
		msleep(10);
		es_i2c_read(0x0f, &data1);	
		data0 >>= 7;
		data0 &= 0x01;
		data1 >>= 7;
		data1 &= 0x01;

		if (data0 == 0 && data1 == 1) {
			printf_green("sram test pass\n");
			close_i2c();
			return 0;
		} 
	}

	printf_red("sram test fail\n");
	close_i2c();
	return 0;
}

static int saradc_test(int argc, char **argv)
{
	uint8_t data0, data1;

	open_i2c();	
	
	es_i2c_write(0xff, 0x01);
	es_i2c_write(0xb6, 0xc1);
	es_i2c_write(0xb5, 0x00);
	es_i2c_write(0xb4, 0x80);


	close_i2c();
	return 0;
}

static int ddr_test(int argc, char **argv)
{
	uint8_t data0, data1;

	open_i2c();

	gpio_configure(ES_PIN_03, GPIO_DIR_INPUT);
	gpio_configure(ES_PIN_04, GPIO_DIR_INPUT);
	
	enter_es_mode();

	es_i2c_write(0xff, 0x04);
	es_i2c_write(0xa0, 0x5a);
	es_i2c_write(0xab, 0x10);
	es_i2c_write(0xa1, 0x00);
	es_i2c_write(0xa9, 0x21);
	es_i2c_write(0xa2, 0x12);
	es_i2c_write(0xab, 0x11);
	es_i2c_write(0xff, 0x03);
	msleep(10);
	data0 = gpio_get_input(ES_PIN_03);
	data1 = gpio_get_input(ES_PIN_04);
	if (data0 == 1 && data1 == 0)
		printf_green("ddr 33M test pass\n");
	else
		printf_red("ddr 33M test fail\n");
	
	es_i2c_write(0xff, 0x04);
	es_i2c_write(0xa0, 0x5a);
	es_i2c_write(0xab, 0x10);
	es_i2c_write(0xa9, 0x2c);
	es_i2c_write(0xa2, 0x14);
	es_i2c_write(0xab, 0x11);
	es_i2c_write(0xff, 0x03);
	msleep(10);
	data0 = gpio_get_input(ES_PIN_03);
	data1 = gpio_get_input(ES_PIN_04);
	if (data0 == 1 && data1 == 0)
		printf_green("ddr 22M test pass\n");
	else
		printf_red("ddr 22M test fail\n");

	es_i2c_write(0xff, 0x04);
	es_i2c_write(0xa0, 0x5a);
	es_i2c_write(0xab, 0x10);
	es_i2c_write(0xa1, 0x08);
	es_i2c_write(0xa9, 0x37);
	es_i2c_write(0xa2, 0x14);
	es_i2c_write(0xab, 0x11);
	es_i2c_write(0xff, 0x03);
	msleep(10);
	data0 = gpio_get_input(ES_PIN_03);
	data1 = gpio_get_input(ES_PIN_04);
	if (data0 == 1 && data1 == 0)
		printf_green("ddr 27.5M test pass\n");
	else
		printf_red("ddr 27.6M test fail\n");

	es_i2c_write(0xff, 0x04);
	es_i2c_write(0xa0, 0x5a);
	es_i2c_write(0xab, 0x10);
	es_i2c_write(0xa1, 0x0c);
	es_i2c_write(0xa9, 0x21);
	es_i2c_write(0xa2, 0x14);
	es_i2c_write(0xab, 0x11);
	es_i2c_write(0xff, 0x03);
	msleep(10);
	data0 = gpio_get_input(ES_PIN_03);
	data1 = gpio_get_input(ES_PIN_04);
	if (data0 == 1 && data1 == 0)
		printf_green("ddr 16.5M test pass\n");
	else
		printf_red("ddr 16.5M test fail\n");

	es_i2c_write(0xff, 0x04);
	es_i2c_write(0xab, 0x5a);
	es_i2c_write(0xab, 0x10);
	es_i2c_write(0xa1, 0x10);
	es_i2c_write(0xa9, 0x18);
	es_i2c_write(0xa2, 0x14);
	es_i2c_write(0xab, 0x11);
	es_i2c_write(0xff, 0x03);
	msleep(10);
	data0 = gpio_get_input(ES_PIN_03);
	data1 = gpio_get_input(ES_PIN_04);
	if (data0 == 1 && data1 == 0)
		printf_green("ddr 12M test pass\n");
	else
		printf_red("ddr 12M test fail\n");

	close_i2c();
	return 0;
}

static int usb_test(int arg, char **argv)
{
	uint8_t data0;

	open_i2c();

	es_i2c_write(0xff, 0x05);
	es_i2c_write(0xbe, 0x1e);
	es_i2c_write(0xbc, 0xa2);
	es_i2c_write(0xbe, 0x1f);
	es_i2c_write(0xbe, 0x1d);
	es_i2c_write(0xbc, 0xe6);
	msleep(10);
	es_i2c_read(0xbd, &data0);	

	if (data0 == 0x66)
		printf_green("usb fs test pass\n");
	else
		printf_red("usb fs test fail\n");

	es_i2c_write(0xff, 0x05);
	es_i2c_write(0xbe, 0x1e);
	es_i2c_write(0xbc, 0x91);
	es_i2c_write(0xbe, 0x1f);
	es_i2c_write(0xbe, 0x1d);
	es_i2c_write(0xbe, 0x11);
	es_i2c_write(0xbc, 0xd5);
	msleep(10);
	es_i2c_read(0xbd, &data0);	

	if (data0 == 0x55)
		printf_green("usb hs test pass\n");
	else
		printf_red("usbtest hs fail\n");

	close_i2c();

	return 0;
}

static int hdmi_phy_test(int argc, char **argv)
{
	uint8_t data0 = 0;
	open_i2c();
	
	enter_es_mode();

	es_i2c_write(0xff, 0x06);
	pinmux_configure(CLOCK_12M, 3);
	REG32_WRITE(0xb8800148, 0x0c182080);
	es_i2c_write(0xba, 0x80);
	es_i2c_write(0xba, 0x81);
	es_i2c_write(0xba, 0x85);
	es_i2c_write(0xba, 0x81);
	es_i2c_write(0xba, 0x83);
	es_i2c_write(0xba, 0x81);

	msleep(10);
	es_i2c_read(0xbb, &data0);	

	if ((data0 &0x0f) == 0x0f)
		printf_green("hdmi phy test pass\n");
	else
		printf_red("hdmi phy test fail\n");

	pinmux_configure(CLOCK_12M, 0);
	close_i2c();

	return 0;
}

static int mipi_test(int argc, char **argv)
{
	uint8_t data0 = 0;
	open_i2c();

	enter_es_mode();

	es_i2c_write(0xff, 0x09);
	pinmux_configure(CLOCK_12M, 3);
	REG32_WRITE(0xb8800148, 0x0c182080);
	es_i2c_write(0xcc, 0x14);
	es_i2c_write(0xcc, 0x16);
	es_i2c_write(0xcc, 0x12);
	es_i2c_write(0xcc, 0x16);
	es_i2c_write(0xcc, 0x17);
	es_i2c_write(0xcc, 0x16);
	msleep(100);
	es_i2c_read(0xcc, &data0);	
	
	if ((data0 & 0xc0) == 0xc0)
		printf_green("mipi test pass\n");
	else
		printf_red("mipi test fail\n");

	pinmux_configure(CLOCK_12M, 0);
	close_i2c();

	return 0;
}

static int hrx_test(int argc, char **argv)
{
	open_i2c();

	enter_es_mode();

	pinmux_configure(ES_MODE_PIN, GPIO_DIR_INPUT);
	pinmux_configure(ES_PIN_06, GPIO_DIR_INPUT);
	
	REG32_SET_BIT(0xb8800044, BIT26);
	REG32_SET_BIT(0xb8800048, BIT26);
	REG32_SET_BIT(0xb880004c, BIT26);

	es_i2c_write(0xff, 0x09);
	pinmux_configure(CLOCK_12M, 3);
	REG32_WRITE(0xb8800148, 0x0c182080);
	es_i2c_write(0xcc, 0x14);
	es_i2c_write(0xcc, 0x10);
	es_i2c_write(0xcc, 0x14);
	es_i2c_write(0xce, 0x12);
	REG32_SET_BIT(0xb880005c, BIT26);
	es_i2c_write(0xce, 0x13);
	es_i2c_write(0xce, 0x12);

	if (REG32_GET_BIT(0xb880005c, BIT26))
		printf_green("hrx test pass\n");
	else
		printf_red("hrx test fail\n");

	pinmux_configure(CLOCK_12M, 0);
	close_i2c();

	return 0;
}

static int lvds_test(int argc, char **argv)
{
	uint8_t data0 = 0;
	open_i2c();

	enter_es_mode();

	es_i2c_write(0xff, 0x09);
	pinmux_configure(CLOCK_12M, 3);
	REG32_WRITE(0xb8800148, 0x0c182080);
	es_i2c_write(0xcc, 0x14);
	es_i2c_write(0xcd, 0x11);
	es_i2c_write(0xcc, 0x10);
	es_i2c_write(0xcc, 0x14);
	es_i2c_write(0xcd, 0x13);
	es_i2c_write(0xcd, 0x11);
	msleep(10);
	
	es_i2c_read(0xcd, &data0);	

	if ((data0 & 0xc0) == 0xc0)
		printf_green("lvds test pass\n");
	else
		printf_red("lvds test fail\n");

	pinmux_configure(CLOCK_12M, 0);
	close_i2c();

	return 0;
}

static int exit_es_mode(int argc, char **argv)
{

	gpio_configure(ES_MODE_PIN, GPIO_DIR_INPUT);
	gpio_configure(RESET_PIN, GPIO_DIR_OUTPUT);
	gpio_configure(PINPAD_B00, GPIO_DIR_INPUT);
	gpio_configure(PINPAD_L25, GPIO_DIR_INPUT);
	gpio_set_output(RESET_PIN, GPIO_OUTPUT_HIGH);
	msleep(10);

	gpio_set_output(RESET_PIN, GPIO_OUTPUT_LOW);
	msleep(50);
	gpio_set_output(RESET_PIN, GPIO_OUTPUT_HIGH);
	msleep(50);
}
#ifdef CONFIG_SORTING_A3200
int es_test(int argc, char **argv)
{
	printf("start test----------------------------------------\n");
	gpio_configure(PINPAD_B01, GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_HIGH);
	msleep(10);
	sram_bitst_test(0, NULL);
	ddr_test(0, NULL);
	usb_test(0, NULL);
	hdmi_phy_test(0, NULL);
	mipi_test(0, NULL);
	hrx_test(0, NULL);
	lvds_test(0, NULL);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_LOW);
	exit_es_mode(0, NULL);
	printf("end test-----------------------------------------\n");

	return 0;
}
#elif CONFIG_SORTING_C3000
int es_test(int argc, char **argv)
{
	printf("start test----------------------------------------\n");
	gpio_configure(PINPAD_B01, GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_HIGH);
	msleep(10);
	sram_bitst_test(0, NULL);
	ddr_test(0, NULL);
	usb_test(0, NULL);
	//hdmi_phy_test(0, NULL);
	mipi_test(0, NULL);
	//hrx_test(0, NULL);
	lvds_test(0, NULL);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_LOW);
	exit_es_mode(0, NULL);
	printf("end test-----------------------------------------\n");

	return 0;
}
#elif CONFIG_SORTING_C5200
int es_test(int argc, char **argv)
{
	printf("start test----------------------------------------\n");
	while (1) {
		hrx_test(0, NULL);
		msleep(5000);
	}
	gpio_configure(PINPAD_B01, GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_HIGH);
	msleep(10);
	sram_bitst_test(0, NULL);
	ddr_test(0, NULL);
	usb_test(0, NULL);
	mipi_test(0, NULL);
	lvds_test(0, NULL);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_LOW);
	exit_es_mode(0, NULL);
	printf("end test-----------------------------------------\n");

	return 0;
}
#elif CONFIG_SORTING_D5200
int es_test(int argc, char **argv)
{
	printf("start test----------------------------------------\n");
	gpio_configure(PINPAD_B01, GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_HIGH);
	msleep(10);
	ddr_test(0, NULL);
	usb_test(0, NULL);
	hdmi_phy_test(0, NULL);
	mipi_test(0, NULL);
	hrx_test(0, NULL);
	lvds_test(0, NULL);
	sram_bitst_test(0, NULL);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_LOW);
	exit_es_mode(0, NULL);
	printf("end test-----------------------------------------\n");

	return 0;
}
#elif CONFIG_SORTING_D3000
int es_test(int argc, char **argv)
{
	printf("start test----------------------------------------\n");
	gpio_configure(PINPAD_B01, GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_HIGH);
	msleep(10);
	sram_bitst_test(0, NULL);
	ddr_test(0, NULL);
	usb_test(0, NULL);
	hdmi_phy_test(0, NULL);
	mipi_test(0, NULL);
	hrx_test(0, NULL);
	lvds_test(0, NULL);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_LOW);
	exit_es_mode(0, NULL);
	printf("end test-----------------------------------------\n");

	return 0;
}
#elif CONFIG_SORTING_C3100
int es_test(int argc, char **argv)
{
	printf("start test----------------------------------------\n");
	gpio_configure(PINPAD_B01, GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_HIGH);
	msleep(10);
	sram_bitst_test(0, NULL);
	ddr_test(0, NULL);
	usb_test(0, NULL);
	mipi_test(0, NULL);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_LOW);
	exit_es_mode(0, NULL);
	printf("end test-----------------------------------------\n");

	return 0;
}
#elif CONFIG_SORTING_B3100
int es_test(int argc, char **argv)
{
	printf("start test----------------------------------------\n");
	gpio_configure(PINPAD_B01, GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_HIGH);
	msleep(10);
	sram_bitst_test(0, NULL);
	ddr_test(0, NULL);
	usb_test(0, NULL);
	lvds_test(0, NULL);
	gpio_set_output(PINPAD_B01, GPIO_OUTPUT_LOW);
	exit_es_mode(0, NULL);
	printf("end test-----------------------------------------\n");

	return 0;
}
#endif



CONSOLE_CMD(sram_test, NULL, sram_bitst_test, CONSOLE_CMD_MODE_SELF, "sram test");
CONSOLE_CMD(ddr_test, NULL, ddr_test, CONSOLE_CMD_MODE_SELF, "ddr test");
CONSOLE_CMD(usb_test, NULL, usb_test, CONSOLE_CMD_MODE_SELF, "usb test");
CONSOLE_CMD(hdmi_phy_test, NULL, hdmi_phy_test, CONSOLE_CMD_MODE_SELF, "hdmi phy test");
CONSOLE_CMD(mipi_test, NULL, mipi_test, CONSOLE_CMD_MODE_SELF, "mipi test");
CONSOLE_CMD(hrx_test, NULL, hrx_test, CONSOLE_CMD_MODE_SELF, "hrx rs test");
CONSOLE_CMD(lvds_test, NULL, lvds_test, CONSOLE_CMD_MODE_SELF, "lvds test");
CONSOLE_CMD(es_test, NULL, es_test, CONSOLE_CMD_MODE_SELF, "es test");
CONSOLE_CMD(i2c_wr_test, NULL, i2c_rw_test, CONSOLE_CMD_MODE_SELF, "i2c test");
