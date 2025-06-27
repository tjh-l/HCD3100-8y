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
#include <hcuapi/lvds.h>
#define LVDSDEV_PATH  "/dev/lvds"

static int lvds_test_enter(int argc, char *argv[])
{
	return 0;
}

static void lvds_set_info_print_usage(const char *prog)
{
		printf("Usage: %s \n", prog);
		puts(	"  -c --channel0				set lvds channel0\n"
				"  -C --channel1				set lvds channel1\n"
				"  -m --channel_mode			set lvds channel_mode\n"
				"  -a --map_mode				set lvds map_mode\n"
				"  -s --ch0_src_sel			set lvds ch0_src_sel\n"
				"  -S --ch1_src_sel			set lvds ch1_src_sel\n"
				"  -i --ch0_invert_clk_sel	set lvds ch0_invert_clk_sel\n"
				"  -I --ch1_invert_clk_sel	set lvds ch1_invert_clk_sel\n"
				"  -g --ch0_clk_gate			set lvds ch0_clk_gate\n"
				"  -G --ch1_clk_gate			set lvds ch1_clk_gate\n"
				"  -H --hsync_polarity		set lvds hsync_polarity\n"
				"  -V --vsync_polarity		set lvds vsync_polarity\n"
				"  -e --even_odd_adjust_mode	set lvds even_odd_adjust_mode\n"
				"  -E --even_odd_init_value	set lvds even_odd_init_value\n"
				"  -p --chx_swap_ctrl	set lvds chx_swap_ctrl\n"
				"  -d --lvds_drive_strength	set lvds_drive_strength\n"
				"  -r --src_sel	set lvds src_sel\n"
				"  -R --src1_sel	set lvds src1_sel\n"
				"  -h --help\n"
				"eg: eg: lvds_set -c 5 -C 5 -m 0 -a 0 -s 0 -S 0 -i 0 -I 0 -k 0 -K 0 -H 0 -V 0 -e 0 -E 0 -p 0 -d 0 -r 0"
		);
}

static void rgb_set_info_print_usage(const char *prog)
{
		printf("Usage: %s \n", prog);
		puts(	"  -c --channel		set rgb channel\n"
				"  -r --src_sel		set rgb src_sel\n"
				"  -n --rgb_clk_inv set rgb clk_inv\n"
				"  -R --RED		set rgb RED\n"
				"  -G --GREEN		set rgb GREEN\n"
				"  -B --BLUE		set rgb BLUE\n"
				"  -D --ttl_drive_strength	ttl_drive_strength\n"
				"  -h --help\n"
				"eg: rgb_set -c 0 -C 0 -n 0 -l 7 -R 0 -G 1 -B 2 -D 1"
		);
}

static int lvds_set_info_cb(int argc, char *argv[])
{
	int fd = 0;
	lvds_info_t lvds_info;
	static const struct option lopts[] = {
			{ "channel0",				1, 0, 'c' },
			{ "channel1",				1, 0, 'C' },
			{ "channel_mode",			1, 0, 'm' },
			{ "map_mode",				1, 0, 'a' },
			{ "ch0_src_sel",			1, 0, 's' },
			{ "ch1_src_sel",			1, 0, 'S' },
			{ "ch0_invert_clk_sel",		1, 0, 'i' },
			{ "ch1_invert_clk_sel",		1, 0, 'I' },
			{ "ch0_clk_gate",			1, 0, 'g' },
			{ "ch1_clk_gate",			1, 0, 'G' },
			{ "hsync_polarity",			1, 0, 'H' },
			{ "vsync_polarity",			1, 0, 'V' },
			{ "even_odd_adjust_mode",	1, 0, 'e' },
			{ "even_odd_init_value",	1, 0, 'E' },
			{ "chx_swap_ctrl",			1, 0, 'p' },
			{ "lvds_drive_strength",	1, 0, 'd' },
			{ "src_sel",				1, 0, 'r' },
			{ "src1_sel",				1, 0, 'R' },
			{ "help",					1, 0, 'h' },
			{ "NULL",					0, 0, 0 },
		};

	fd = open(LVDSDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LVDSDEV_PATH, fd);
		return -1;
	}
	ioctl(fd, LVDS_GET_CHANNEL_INFO, &lvds_info);
	opterr = 0;
	optind = 0;
	while (1) {
		int c;

		c = getopt_long(argc, argv, "c:C:m:a:s:S:i:I:k:K:H:V:e:E:p:d:r:R:h", lopts, NULL);
		// printf("%s %d %c\n",__func__,__LINE__,c);
		if (c == -1) {
			break;
		}

		switch(c) {
		case 'c':
			lvds_info.channel_type[0] = atoi((const char*)optarg);
			break;
		case 'C':
			lvds_info.channel_type[1] = atoi((const char*)optarg);
			break;
		case 'm':
			lvds_info.lvds_cfg.channel_mode = atoi((const char*)optarg);
			break;
		case 'a':
			lvds_info.lvds_cfg.map_mode = atoi((const char*)optarg);
			break;
		case 's':
			lvds_info.lvds_cfg.ch0_src_sel = atoi((const char*)optarg);
			break;
		case 'S':
			lvds_info.lvds_cfg.ch1_src_sel = atoi((const char*)optarg);
			break;
		case 'i':
			lvds_info.lvds_cfg.ch0_invert_clk_sel = atoi((const char*)optarg);
			break;
		case 'I':
			lvds_info.lvds_cfg.ch1_invert_clk_sel = atoi((const char*)optarg);
			break;
		case 'k':
			lvds_info.lvds_cfg.ch0_clk_gate = atoi((const char*)optarg);
			break;
		case 'K':
			lvds_info.lvds_cfg.ch1_clk_gate = atoi((const char*)optarg);
			break;
		case 'H':
			lvds_info.lvds_cfg.hsync_polarity = atoi((const char*)optarg);
			break;
		case 'V':
			lvds_info.lvds_cfg.vsync_polarity = atoi((const char*)optarg);
			break;
		case 'e':
			lvds_info.lvds_cfg.even_odd_adjust_mode = atoi((const char*)optarg);
			break;
		case 'E':
			lvds_info.lvds_cfg.even_odd_init_value = atoi((const char*)optarg);
			break;
		case 'p':
			lvds_info.lvds_cfg.chx_swap_ctrl = atoi((const char*)optarg);
			break;
		case 'd':
			lvds_info.lvds_cfg.drive_strength = atoi((const char*)optarg);
			break;
		case 'r':
			lvds_info.lvds_cfg.src_sel = atoi((const char*)optarg);
			break;
		case 'R':
			lvds_info.lvds_cfg.src1_sel = atoi((const char*)optarg);
			break;
		case 'h':
			lvds_set_info_print_usage(argv[0]);
			break;
		default:
			lvds_set_info_print_usage(argv[0]);
			return -1;
		}
	}
	ioctl(fd, LVDS_SET_CHANNEL_INFO, &lvds_info);
	close(fd);
	printf("lvds_get_info -c %d -C %d -m %d -a %d -s %d -S %d -i %d -I %d -k %d -K %d -H %d -V %d -e %d -E %d -p %d -d %d -r %d -R %d\n",
										lvds_info.channel_type[0],lvds_info.channel_type[1],lvds_info.lvds_cfg.channel_mode,\
										lvds_info.lvds_cfg.map_mode, lvds_info.lvds_cfg.ch0_src_sel,\
										lvds_info.lvds_cfg.ch1_src_sel, lvds_info.lvds_cfg.ch0_invert_clk_sel,\
										lvds_info.lvds_cfg.ch1_invert_clk_sel, lvds_info.lvds_cfg.ch0_clk_gate,\
										lvds_info.lvds_cfg.ch1_clk_gate, lvds_info.lvds_cfg.hsync_polarity,\
										lvds_info.lvds_cfg.vsync_polarity, lvds_info.lvds_cfg.even_odd_adjust_mode,\
										lvds_info.lvds_cfg.even_odd_init_value, lvds_info.lvds_cfg.chx_swap_ctrl,\
										lvds_info.lvds_cfg.drive_strength, lvds_info.lvds_cfg.src_sel, lvds_info.lvds_cfg.src1_sel);
	return 0;
}

static int rgb_set_info_cb(int argc, char *argv[])
{
	int fd = 0;
	lvds_info_t lvds_info;
	static const struct option lopts[] = {
			{ "channel0",				1, 0, 'c' },
			{ "rgb_clk_inv",			1, 0, 'n' },
			{ "src_sel",				1, 0, 'l' },
			{ "RED",					1, 0, 'R' },
			{ "GREEN",					1, 0, 'G' },
			{ "BLUE",					1, 0, 'B' },
			{ "ttl_drive_strength",		1, 0, 'D' },
			{ "help",					1, 0, 'h' },
			{ "NULL",					0, 0, 0 },
		};

	fd = open(LVDSDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LVDSDEV_PATH, fd);
		return -1;
	}
	ioctl(fd, LVDS_GET_CHANNEL_INFO, &lvds_info);
	opterr = 0;
	optind = 0;
	while (1) {
		int c;

		c = getopt_long(argc, argv, "c:n:l:R:G:B:D:h:", lopts, NULL);
		// printf("%s %d %c\n",__func__,__LINE__,c);
		if (c == -1) {
			break;
		}

		switch(c) {
		case 'c':
			lvds_info.channel_type[1] = lvds_info.channel_type[0] = atoi((const char*)optarg);
			break;
		case 'n':
			lvds_info.ttl_cfg.rgb_clk_inv = atoi((const char*)optarg);
			break;
		case 'l':
			lvds_info.ttl_cfg.src_sel = atoi((const char*)optarg);
			break;
		case 'R':
			lvds_info.ttl_cfg.rgb_src_sel[0] = atoi((const char*)optarg);
			break;
		case 'G':
			lvds_info.ttl_cfg.rgb_src_sel[1] = atoi((const char*)optarg);
			break;
		case 'B':
			lvds_info.ttl_cfg.rgb_src_sel[2] = atoi((const char*)optarg);
			break;
		case 'D':
			lvds_info.ttl_cfg.drive_strength = atoi((const char*)optarg);
			break;
		case 'h':
			rgb_set_info_print_usage(argv[0]);
			break;
		default:
			rgb_set_info_print_usage(argv[0]);
			return -1;
		}
	}
	ioctl(fd, LVDS_SET_CHANNEL_INFO, &lvds_info);
	close(fd);
	printf("rgb_get_info -c %d -C %d -n %ld -l %d -R %d -G %d -B %d -D %d\n",
										lvds_info.channel_type[0],lvds_info.channel_type[1],\
										lvds_info.ttl_cfg.rgb_clk_inv, lvds_info.ttl_cfg.src_sel,\
										lvds_info.ttl_cfg.rgb_src_sel[0], lvds_info.ttl_cfg.rgb_src_sel[1],\
										lvds_info.ttl_cfg.rgb_src_sel[2], lvds_info.ttl_cfg.drive_strength);
	return 0;
}

CONSOLE_CMD(lvds_test, NULL, lvds_test_enter, CONSOLE_CMD_MODE_SELF, "enter lvds test")
CONSOLE_CMD(lvds_set, "lvds_test", lvds_set_info_cb, CONSOLE_CMD_MODE_SELF, "eg: lvds_set -c 5 -C 5 -m 0 -a 0 -s 0 -S 0 -i 0 -I 0 -k 0 -K 0 -H 0 -V 0 -e 0 -E 0 -p 0 -d 0 -r 0")
CONSOLE_CMD(rgb_set, "lvds_test", rgb_set_info_cb, CONSOLE_CMD_MODE_SELF, "eg: rgb_set -c 0 -C 0 -n 0 -l 7 -R 0 -G 1 -B 2 -D 1")
