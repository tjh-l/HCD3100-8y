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

static int rgb_test_enter(int argc, char *argv[])
{
	return 0;
}

static void h1512_rgb_get_info_print_usage(const char *prog)
{
		printf("Usage: %s \n", prog);
		puts(
				"  -n --rgb_clk_inv set rgb clk_inv\n"
				"  -h --help\n"
				"eg: rgb_set -n 1"
		);
}

static int h1512_rgb_set_info_cb(int argc, char *argv[])
{
	static const struct option lopts[] = {
			{ "rgb_clk_inv",			1, 0, 'n' },
			{ "help",					1, 0, 'h' },
			{ "NULL",					0, 0, 0 },
		};
	int np = -1;
	bool val = 0;
	np = fdt_get_node_offset_by_path( "/hcrtos/rgb");
	
	if(np <= 0)
		return 0;

	opterr = 0;
	optind = 0;
	while (1) {
		int c;

		c = getopt_long(argc, argv, "n:h:", lopts, NULL);
		// printf("%s %d %c\n",__func__,__LINE__,c);
		if (c == -1) {
			break;
		}

		switch(c) {
		case 'n':
			val = atoi((const char*)optarg);
			#define SYSTEM_CLOCK_CONTROL_REG        0xB8800078
			if(val == 0)
				REG32_CLR_BIT(SYSTEM_CLOCK_CONTROL_REG, BIT(15));
			else
				REG32_SET_BIT(SYSTEM_CLOCK_CONTROL_REG, BIT(15));
			break;
		case 'h':
			h1512_rgb_get_info_print_usage(argv[0]);
			break;
		default:
			h1512_rgb_get_info_print_usage(argv[0]);
			return -1;
		}
	}

	printf("rgb_get_info -n %d\n", val);
	return 0;
}

CONSOLE_CMD(rgb_test, NULL, rgb_test_enter, CONSOLE_CMD_MODE_SELF, "enter rgb test")
CONSOLE_CMD(rgb_set, "rgb_test", h1512_rgb_set_info_cb, CONSOLE_CMD_MODE_SELF, "eg: rgb_set -n 1")
