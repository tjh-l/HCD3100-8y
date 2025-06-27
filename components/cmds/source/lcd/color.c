#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/lib/console.h>
#include <string.h>
#include <hcuapi/lvds.h>
#include <hcuapi/gpio.h>
#include <kernel/delay.h>
#include <getopt.h>
#include <dt-bindings/gpio/gpio.h>
#include <kernel/lib/fdt_api.h>


static int color_test_enter(int argc, char *argv[])
{
	return 0;
}

static int get_de_mode(void)
{
	static unsigned int de_4kwork = 0;
	static int on_work = 0;
	int np = 0;
	if(on_work == 0)
	{
		np = fdt_get_node_offset_by_path( "/hcrtos/de-engine");
		if (np < 0) {
			printf("Get error /hcrtos/de-engine/\n");
			return -1;
		}
		on_work = 1;
		fdt_get_property_u_32_index(np, "de4k-output", 0, &de_4kwork);
	}
	
	return de_4kwork;

}
static int color_set_cb(int argc, char *argv[])
{
	u32 colorVal = 0;
	unsigned char color_data_buf[9] = { 0 };
	char *ptr = NULL;
	int len = 0;
	int val = 0;
	val = get_de_mode();
	if(argc > 1)
	{
		len = strlen(argv[1]);
		if (len > 8)
			printf("len >8 %s",  argv[1]);
		else
			memcpy(color_data_buf, argv[1], len);
		colorVal = strtoll(color_data_buf, &ptr, 16);
		printf("%s %d 0x%x \n", __func__, __LINE__, colorVal);
		colorVal = ((colorVal & 0xff0000) >>16) | ((colorVal & 0x00ff00) << 8) | ((colorVal & 0xff) << 8);
		if(val == 0)
			REG32_WRITE(0xb8808088, colorVal);
		else
			REG32_WRITE(0xb883a088, colorVal);
	}
	return 0;
}

static int color_start_cb(int argc, char *argv[])
{
	int val = 0;
	val = get_de_mode();
	if(val == 0)
	{
		REG32_SET_BIT(0xb8808080, BIT(21));
		REG32_SET_BIT(0xb8808084, BIT(21));
		REG32_CLR_BIT(0xb8808000, BIT(2));
		REG32_SET_BIT(0xb8808000, BIT(2));
	}
	else if(val == 1)
	{
		REG32_SET_BIT(0xb883a080, BIT(21));
		REG32_SET_BIT(0xb883a084, BIT(21));
		REG32_CLR_BIT(0xb883a000, BIT(2));
		REG32_SET_BIT(0xb883a000, BIT(2));
	}
	
	return 0;
}

static int color_stop_cb(int argc, char *argv[])
{
	int val = 0;
	val = get_de_mode();
	if(val == 0)
	{
		REG32_CLR_BIT(0xb8808080, BIT(21));
		REG32_CLR_BIT(0xb8808084, BIT(21));
		REG32_WRITE(0xb8808000, 0x10);
		REG32_WRITE(0xb8808000, 0x15);
	}
	else if(val == 1)
	{
		REG32_CLR_BIT(0xb883a080, BIT(21));
		REG32_CLR_BIT(0xb883a084, BIT(21));
		REG32_WRITE(0xb883a000, 0x10);
		REG32_WRITE(0xb883a000, 0x15);
	}
	return 0;
}

CONSOLE_CMD(color, NULL, color_test_enter, CONSOLE_CMD_MODE_SELF, "enter color test")
CONSOLE_CMD(set, "color", color_set_cb, CONSOLE_CMD_MODE_SELF, "eg: set 00ff00")
CONSOLE_CMD(start, "color", color_start_cb, CONSOLE_CMD_MODE_SELF, "eg: start")
CONSOLE_CMD(stop, "color", color_stop_cb, CONSOLE_CMD_MODE_SELF, "eg: stop")
