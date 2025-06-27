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
#include <hcuapi/dis.h>
#include <kernel/lib/fdt_api.h>
#include <hcuapi/mipi.h>
#include <hcuapi/lvds.h>
#include <hcuapi/fb.h>
#include <hcuapi/lcd.h>

#define OSD_MAX_WIDTH 1280
#define OSD_MAX_HEIGHT 720
#define     DEV_FB          "/dev/fb0"
#define		DEV_FB_PATH		"/hcrtos/fb0"

static dis_rgb_timing_param_t de_timing={0};
static hcfb_scale_t scale_param1 = { OSD_MAX_WIDTH, OSD_MAX_HEIGHT, 1920, 1080};
static u32 tv_type = 15;

static int get_dts_de_timing(void)
{
	int np = 0;
	unsigned int temp = 0;
	static int frist = 0;
	if(frist == 0)
	{
		frist = 1;
		np = fdt_get_node_offset_by_path( "/hcrtos/de-engine");
		if(np > 0)
		{
			fdt_get_property_u_32_index(np, "tvtype", 0, (u32 *)&tv_type);
		}

		np = fdt_get_node_offset_by_path( "/hcrtos/de-engine/VPInitInfo/rgb-cfg/timing-para");
		if(np > 0)
		{
			fdt_get_property_u_32_index(np, "h-active-len", 0, (u32 *)&de_timing.h_active_len);
			fdt_get_property_u_32_index(np, "h-back-len", 0, (u32 *)&de_timing.h_back_len);
			fdt_get_property_u_32_index(np, "h-front-len", 0, (u32 *)&de_timing.h_front_len);
			fdt_get_property_u_32_index(np, "h-sync-len", 0, (u32 *)&de_timing.h_sync_len);
			fdt_get_property_u_32_index(np, "h-total-len", 0, (u32 *)&de_timing.h_total_len);
			fdt_get_property_u_32_index(np, "v-active-len", 0, (u32 *)&de_timing.v_active_len);
			fdt_get_property_u_32_index(np, "v-back-len", 0, (u32 *)&de_timing.v_back_len);
			fdt_get_property_u_32_index(np, "v-front-len", 0, (u32 *)&de_timing.v_front_len);
			fdt_get_property_u_32_index(np, "v-sync-len", 0, (u32 *)&de_timing.v_sync_len);
			fdt_get_property_u_32_index(np, "v-total-len", 0, (u32 *)&de_timing.v_total_len);
			fdt_get_property_u_32_index(np, "frame-rate", 0, (u32 *)&de_timing.frame_rate);
			de_timing.active_polarity = 1;
			if(!fdt_get_property_u_32_index(np, "active-polarity", 0, (u32 *)&temp))
			{
				de_timing.active_polarity = temp;
			}
			if(!fdt_get_property_u_32_index(np, "h-sync-level", 0, &temp))
			{
				de_timing.h_sync_level = temp;
			}

			if(!fdt_get_property_u_32_index(np, "v-sync-level", 0, &temp))
			{
				de_timing.v_sync_level = temp;
			}

			if(!fdt_get_property_u_32_index(np, "output-clock", 0, &temp))
			{
				de_timing.rgb_clock = temp;
			}

			fdt_get_property_u_32_index(np, "dpll-clock-reg-value", 0, (u32 *)&de_timing.dpll_clock_reg_value);

			if(!fdt_get_property_u_32_index(np, "b-enable", 0, &temp))
			{
				de_timing.b_enable = temp;
			}
		}
	}
	return 0;
}

static int de_timing_test_enter(int argc, char *argv[])
{
	get_dts_de_timing();
	#if 0
	printf("de_timing_get -o %d -t %ld -a %ld -f %ld -s %ld -b %ld -T %ld -A %ld -F %ld -S %ld -B %ld -m %ld -H %d -V %d -D %ld -d %ld -e %d -v %d -L 0x%08lx\n",
							de_timing.rgb_clock,de_timing.h_total_len,de_timing.h_active_len,\
							de_timing.h_front_len,de_timing.h_sync_len,de_timing.h_back_len,\
							de_timing.v_total_len,de_timing.v_active_len,de_timing.v_front_len,\
							de_timing.v_sync_len,de_timing.v_back_len,\
							de_timing.frame_rate,\
							de_timing.h_sync_level,de_timing.v_sync_level,\
							de_timing.h_display_len, de_timing.v_display_len,\
							de_timing.b_enable, tv_type, de_timing.dpll_clock_reg_value);
	#endif
	return 0;
}


static int Get_Max_Comm_Divisor(int num1, int num2)
{
	int i = 0;
	int min = num1 < num2 ? num1 : num2;
	for(i = min; i > 0; i--)
	{
		if(num1 % i == 0 && num2 % i == 0)
			break;
	}
	return i;
}

static int de_timing_set_dpll_cb(int argc, char *argv[])
{
	float clk_val = 0;
	int con_clk_val = 0 , min_gcd = 0, count = 0;
	int M = 0, N = 0 , multiple_count = 1;
	unsigned char L = 0, gcd_clk_val = 0;
	if(argc > 1)
	{
		clk_val = atof(argv[1]);
		if(clk_val > 1000.0)
		{
			printf("The input exceeds 1000MCLK, the incorrect clock is %s MCLK\n", argv[1]);
			return 0;
		}

		clk_val *= 2;
	calculate:
		if(clk_val - (int)clk_val == 0)
		{
			con_clk_val = (int)clk_val;
			min_gcd = Get_Max_Comm_Divisor(con_clk_val, 24 * multiple_count);
			M = (con_clk_val)  / min_gcd;
			N = (24 * multiple_count)/ min_gcd;
			while(M > 1023 && count < 100)
			{
				M /= 2;
				N /= 2;
				count++;
			}

			// L = multiple_count;
			L = 1;
			printf("con_clk_val = %d min_gcd = %d M = %d N = %d L = %d multiple_count %d\n", con_clk_val, min_gcd, M, N, L, multiple_count);
			printf("dpll-clock-reg-clk_value = %08x %u\n", (M - 1) << 16 | (N -1) << 8 | (L - 1) | 0x80000000, (M - 1) << 16 | (N -1) << 8 | (L - 1) | 0x80000000);
			clk_val = (24 * (float)M)/ (float)N / L / 2;
			printf("CLK OUT IS %f\n", clk_val);
		}
		else
		{
			multiple_count *= 10;
			clk_val *= 10;
			goto calculate;
		}
	}
	return 0;
}

static void de_timing_print_usage(const char *prog)
{
		printf("Usage: %s \n", prog);
		puts(	"  -o --clock			set timing clock\n"
				"  -t --h_total_len		set timing h-total-len\n"
				"  -a --h_active_len	set timing h-total-len\n"
				"  -f --h_front_len		set timing h-front-len\n"
				"  -s --h_sync_len		set timing h-sync-len\n"
				"  -b --h_back_len		set timing h-back-len\n"
				"  -T --v_total_len		set timing v-total-len\n"
				"  -A --v_active_len	set timing v-active-len\n"
				"  -F --v_front_len		set timing v-front-len\n"
				"  -S --v_sync_len		set timing v-sync-len\n"
				"  -B --v_back_len		set timing v-back-len\n"
				"  -m --format_rate		set timing dsi,format\n"
				"  -H --h_sync_level	set timing h_sync_level\n"
				"  -V --v_sync_level	set timing v_sync_level\n"
				"  -e --b_enable        set timing b_enabled"
				"  -P --active_polarity set timing active_polarity"
				"  -L --dpll_clock_reg_value	set mipi timing dpll_clock_reg_value\n"
				"eg: timing_set -t 2020 -a 1920 -f 60 -s 20 -b 20 -T 1218 -A 1200 -F 8 -S 5 -B 5 -o 12 -L 2153907968"
		);
}

static int de_timing_set_timing_cb(int argc, char *argv[])
{
	int fb = 0;
	static const struct option lopts[] = {
			{ "clock",			1, 0, 'o' },
			{ "h_total_len",	1, 0, 't' },
			{ "h_active_len",	1, 0, 'a' },
			{ "h_front_len",	1, 0, 'f' },
			{ "h_sync_len",		1, 0, 's' },
			{ "h_back_len",		1, 0, 'b' },
			{ "v_total_len",	1, 0, 'T' },
			{ "v_active_len",	1, 0, 'A' },
			{ "v_front_len",	1, 0, 'F' },
			{ "v_sync_len",		1, 0, 'S' },
			{ "v_back_len",		1, 0, 'B' },
			{ "format_rate",	1, 0, 'm' },
			{ "h_sync_level",	1, 0, 'H' },
			{ "v_sync_level",	1, 0, 'V' },
			{ "h_display_len",	1, 0, 'D' },
			{ "v_display_len",	1, 0, 'd' },
			{ "b_enable",		1, 0, 'e' },
			{ "tvtype",			1, 0, 'v' },
			{ "active_polarity",1, 0, 'P'},
			{ "dpll_clock_reg_value",	1, 0, 'L' },
			{ "help",			1, 0, 'h' },
			{ "NULL",			0, 0, 0 },
		};
	get_dts_de_timing();
	opterr = 0;
	optind = 0;
	while (1) {
		int c;

		c = getopt_long(argc, argv, "C:t:a:f:s:b:T:A:F:S:B:c:o:l:m:H:V:h:D:d:L:e:v:P:", lopts, NULL);
		// printf("%s %d %c\n",__func__,__LINE__,c);
		if (c == -1) {
			break;
		}

		switch(c) {
		case 'o':
			de_timing.rgb_clock = atoi((const char*)optarg);
			break;
		case 't':
			de_timing.h_total_len = atoi((const char*)optarg);
			break;
		case 'a':
			de_timing.h_active_len = atoi((const char*)optarg);
			break;
		case 'f':
			de_timing.h_front_len = atoi((const char*)optarg);
			break;
		case 's':
			de_timing.h_sync_len = atoi((const char*)optarg);
			break;
		case 'b':
			de_timing.h_back_len = atoi((const char*)optarg);
			break;
		case 'T':
			de_timing.v_total_len = atoi((const char*)optarg);
			break;
		case 'A':
			de_timing.v_active_len = atoi((const char*)optarg);
			break;
		case 'F':
			de_timing.v_front_len = atoi((const char*)optarg);
			break;
		case 'S':
			de_timing.v_sync_len = atoi((const char*)optarg);
			break;
		case 'B':
			de_timing.v_back_len = atoi((const char*)optarg);
			break;
		case 'm':
			de_timing.frame_rate = atoi((const char*)optarg);
			break;
		case 'H':
			de_timing.h_sync_level = atoi((const char*)optarg);
			break;
		case 'V':
			de_timing.v_sync_level = atoi((const char*)optarg);
			break;
		case 'D':
			de_timing.h_display_len = atoi((const char*)optarg);
			break;
		case 'd':
			de_timing.v_display_len = atoi((const char*)optarg);
			break;
		case 'e':
			de_timing.b_enable = atoi((const char*)optarg);
			break;
		case 'v':
			tv_type = atoi((const char*)optarg);
			break;
		case 'L':
			de_timing.dpll_clock_reg_value = atoll((const char*)optarg);
			break;
		case 'P':
			de_timing.active_polarity = atoll((const char*)optarg);
			break;
		case 'h':
			de_timing_print_usage(argv[0]);
			break;
		default:
			de_timing_print_usage(argv[0]);
			return -1;
		}
	}

	printf("de_timing_set -o %d -t %ld -a %ld -f %ld -s %ld -b %ld -T %ld -A %ld -F %ld -S %ld -B %ld -m %ld -H %d -V %d -D %ld -d %ld -e %d -v %d -P %d -L 0x%08lx\n",
								de_timing.rgb_clock,de_timing.h_total_len,de_timing.h_active_len,\
								de_timing.h_front_len,de_timing.h_sync_len,de_timing.h_back_len,\
								de_timing.v_total_len,de_timing.v_active_len,de_timing.v_front_len,\
								de_timing.v_sync_len,de_timing.v_back_len,\
								de_timing.frame_rate,\
								de_timing.h_sync_level,de_timing.v_sync_level,\
								de_timing.h_display_len, de_timing.v_display_len,\
								de_timing.b_enable, tv_type, de_timing.active_polarity, de_timing.dpll_clock_reg_value);

	return 0;
}

/**
 * @fn static int get_dts_is_screen_type(void)
 * @brief 获得dts上是否配置了横竖屏的设置
 *
 * 获得dts上是否配置了横竖屏的设置， 并且设置横屏的内容
 *
 * @return 0
 */

static int get_dts_info_screen_type(hcfb_scale_t *scale)
{
	static bool frist = 0;
	u32 x = 0, y = 0, temp = 0;

	if(frist == 0)
	{
		frist = 1;
		int np = fdt_node_probe_by_path(DEV_FB_PATH);
		if(np)
		{
			fdt_get_property_u_32_index(np, "xres", 0, (u32 *)&x);
			fdt_get_property_u_32_index(np, "yres", 0, (u32 *)&y);
			scale->h_div = x;
			scale->v_div = y;
		}
	}
	return 0;
}

static int de_timing_init_cb(int argc, char *argv[])
{
	dis_rgb_param_t param = {0};
	struct dis_tvsys tvsys = { 0 };
	int fd = 0;
	dis_screen_info_t lcd_area={0};

	get_dts_de_timing();
	param.distype = DIS_TYPE_HD;
	param.timing = de_timing;
	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0)
	{
		printf("open /dev/dis error\n");
		return -1;
	}

	tvsys.layer = 1;
	tvsys.distype = DIS_TYPE_HD;
	tvsys.tvtype = tv_type;
	tvsys.progressive = 1;

	ioctl(fd , DIS_SET_LCD_PARAM , &param);
	ioctl(fd , DIS_SET_TVSYS , &tvsys);
	lcd_area.distype=DIS_TYPE_HD;
    ioctl(fd , DIS_GET_SCREEN_INFO, &lcd_area);
	scale_param1.h_mul = lcd_area.area.w;
	scale_param1.v_mul = lcd_area.area.h;
	get_dts_info_screen_type(&scale_param1);
	
	close(fd);
	printf("%s %d h_mul %d v_mul %d\n", __func__, __LINE__, scale_param1.h_mul, scale_param1.v_mul);
	fd = open(DEV_FB , O_WRONLY);
	if(fd < 0)
	{
		printf("open %s error\n", DEV_FB);
		return 0;
	}

	ioctl(fd, HCFBIOSET_SCALE, &scale_param1);
	close(fd);
	return 0;
}

static int de_timing_get_cb(int argc, char *argv[])
{
	get_dts_de_timing();
	printf("de_timing_set -o %d -t %ld -a %ld -f %ld -s %ld -b %ld -T %ld -A %ld -F %ld -S %ld -B %ld -m %ld -H %d -V %d -D %ld -d %ld -e %d -v %d -P %d -L 0x%08lx\n",
							de_timing.rgb_clock,de_timing.h_total_len,de_timing.h_active_len,\
							de_timing.h_front_len,de_timing.h_sync_len,de_timing.h_back_len,\
							de_timing.v_total_len,de_timing.v_active_len,de_timing.v_front_len,\
							de_timing.v_sync_len,de_timing.v_back_len,\
							de_timing.frame_rate,\
							de_timing.h_sync_level,de_timing.v_sync_level,\
							de_timing.h_display_len, de_timing.v_display_len,\
							de_timing.b_enable, tv_type, de_timing.active_polarity, de_timing.dpll_clock_reg_value);
	return 0;
}
#define VERSTR  "timing V1.0.1.202311.25"
static int de_timing_version_cb(int argc, char *argv[])
{
	int fd = -1;
	int np = -1;
	struct mipi_display_timing timing = { 0 };
	struct hc_lcd_mode_info lcd_info;
	lvds_info_t lvds_info = {0};
	u32 tmpVal = 0;
	const char *info = NULL;
	int ret = -1;
	get_dts_de_timing();
	printf("current_version %s\n", VERSTR);
	fd = open("/dev/dis", O_RDWR);
	{
		printf("de_current_timing -o %d -t %ld -a %ld -f %ld -s %ld -b %ld -T %ld -A %ld -F %ld -S %ld -B %ld -m %ld -H %d -V %d -D %ld -d %ld -e %d -v %d -P %d -L 0x%08lx\n",
									de_timing.rgb_clock,de_timing.h_total_len,de_timing.h_active_len,\
									de_timing.h_front_len,de_timing.h_sync_len,de_timing.h_back_len,\
									de_timing.v_total_len,de_timing.v_active_len,de_timing.v_front_len,\
									de_timing.v_sync_len,de_timing.v_back_len,\
									de_timing.frame_rate,\
									de_timing.h_sync_level,de_timing.v_sync_level,\
									de_timing.h_display_len, de_timing.v_display_len,\
									de_timing.b_enable, tv_type, de_timing.active_polarity, de_timing.dpll_clock_reg_value);
	}
	fd = open("/dev/mipi", O_RDWR);
	if(fd > 0)
	{
		ioctl(fd, MIPI_DSI_GET_TIMING, &timing);
		printf("mipi_current_timing -C %d -t %d -a %d -f %d -s %d -b %d -T %d -A %d -F %d -S %d -B %d -c %d -o %d -l %d -m %d -H %d -V %d -D %d -d %d -p %d\n",
										timing.clk_frequency,timing.h_total_len,timing.h_active_len,\
										timing.h_front_len,timing.h_sync_len,timing.h_back_len,\
										timing.v_total_len,timing.v_active_len,timing.v_front_len,\
										timing.v_sync_len,timing.v_back_len,\
										timing.packet_cfg,timing.pclk,timing.lane_num,timing.color_mode,\
										timing.h_sync_level,timing.v_sync_level, timing.drive_strength, timing.dsi_flags,\
										timing.swap_flags);
		close(fd);
	}

	fd = open("/dev/lvds", O_RDWR);
	if(fd > 0)
	{
		ioctl(fd, LVDS_GET_CHANNEL_INFO, &lvds_info);
		printf("lvds_current_info -c %d -C %d -m %d -a %d -s %d -S %d -i %d -I %d -k %d -K %d -H %d -V %d -e %d -E %d -p %d -d %d -r %d -n %ld -l %d -R %d -G %d -B %d -D %d\n",
										lvds_info.channel_type[0],lvds_info.channel_type[1],lvds_info.lvds_cfg.channel_mode,\
										lvds_info.lvds_cfg.map_mode, lvds_info.lvds_cfg.ch0_src_sel,\
										lvds_info.lvds_cfg.ch1_src_sel, lvds_info.lvds_cfg.ch0_invert_clk_sel,\
										lvds_info.lvds_cfg.ch1_invert_clk_sel, lvds_info.lvds_cfg.ch0_clk_gate,\
										lvds_info.lvds_cfg.ch1_clk_gate, lvds_info.lvds_cfg.hsync_polarity,\
										lvds_info.lvds_cfg.vsync_polarity, lvds_info.lvds_cfg.even_odd_adjust_mode,\
										lvds_info.lvds_cfg.even_odd_init_value, lvds_info.lvds_cfg.chx_swap_ctrl,\
										lvds_info.lvds_cfg.drive_strength, lvds_info.lvds_cfg.src_sel,\
										lvds_info.ttl_cfg.rgb_clk_inv, lvds_info.ttl_cfg.src_sel,\
										lvds_info.ttl_cfg.rgb_src_sel[0], lvds_info.ttl_cfg.rgb_src_sel[1],\
										lvds_info.ttl_cfg.rgb_src_sel[2], lvds_info.ttl_cfg.drive_strength);
		close(fd);
	}

	fd = open("/dev/lcddev", O_RDWR);
	if (fd > 0) {
		ret = ioctl(fd, LCD_GET_MODE_INFO, &lcd_info);
		if(ret >= 0)
		{
			if(lcd_info.mode & LCD_INIT_SEQUENCE_SPI)
			{
				printf("lcd_spi_current_info -k %d -o %d -i %d -c %d -M %d -B %d -D %d -C %d\n", lcd_info.spi.sck, lcd_info.spi.mosi, lcd_info.spi.miso, lcd_info.spi.cs, lcd_info.spi.mode, lcd_info.spi.bit, lcd_info.spi.orcmds, lcd_info.spi.ordata);
			}
			if(lcd_info.mode & LCD_INIT_SEQUENCE_I2C)
			{
				printf("lcd_i2c_current_info -k %d -o %d -D %d -C %d -A %d\n", lcd_info.i2c.scl, lcd_info.i2c.sda, lcd_info.i2c.ordata, lcd_info.i2c.orcmds, lcd_info.i2c.addr);
			}
			printf("lcd_pinpad_current_info -o %d -r %d -s %d -p %d -P %d\n", lcd_info.pinpad.power, lcd_info.pinpad.reset, lcd_info.pinpad.stbyb, lcd_info.pinpad.pinpad0, lcd_info.pinpad.pinpad1);
		}
		close(fd);
	}

	np = fdt_get_node_offset_by_path( "/hcrtos/rgb");
	if(np > 0)
	{
		#define SYSTEM_CLOCK_CONTROL_REG        0xB8800078
		tmpVal = REG8_GET_BIT(SYSTEM_CLOCK_CONTROL_REG, BIT(15));
		printf("rgb_current_info -n %d\n",tmpVal);
	}

	np = fdt_get_node_offset_by_path( "/hcrtos/board");
	if(np > 0)
	{
		fdt_get_property_string_index(np, "label", 0, &info);
		printf("board_current_info %s\n", info);
	}
	return 0;
}

CONSOLE_CMD(de_timing, NULL, de_timing_test_enter, CONSOLE_CMD_MODE_SELF, "enter lcd timing test")
CONSOLE_CMD(dpll, "de_timing", de_timing_set_dpll_cb, CONSOLE_CMD_MODE_SELF, "dpll 49.9")
CONSOLE_CMD(timing_set, "de_timing", de_timing_set_timing_cb, CONSOLE_CMD_MODE_SELF, "timing_set, eg: timing_set -t 2020 -a 1920 -f 60 -s 20 -b 20 -T 1218 -A 1200 -F 8 -S 5 -B 5 -o 12 -L 2153907968")
CONSOLE_CMD(init, "de_timing", de_timing_init_cb, CONSOLE_CMD_MODE_SELF, "eg: init")
CONSOLE_CMD(timing_get, "de_timing", de_timing_get_cb, CONSOLE_CMD_MODE_SELF, "eg: timing_get")
CONSOLE_CMD(version, "de_timing", de_timing_version_cb, CONSOLE_CMD_MODE_SELF, "eg: version")

