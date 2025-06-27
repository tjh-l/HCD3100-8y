#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/mipi.h>
#include <kernel/lib/console.h>
#include <string.h>
#include <hcuapi/lvds.h>
#include <hcuapi/gpio.h>
#include <kernel/delay.h>
#include <getopt.h>
#include <dt-bindings/gpio/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <kernel/delay.h>

#define MIPIDEV_PATH  "/dev/mipi"
static struct hc_dcs_cmds mipi_panel_cmds;

static int mipi_test_enter(int argc, char *argv[])
{
	return 0;
}

static int mipi_dsi_on_cb(int argc, char *argv[])
{
	int fd = 0;
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	ioctl(fd, MIPI_DSI_SET_ON, NULL);
	close(fd);
	return 0;
}

static int mipi_dsi_off_cb(int argc, char *argv[])
{
	int fd = 0;
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	ioctl(fd, MIPI_DSI_SET_OFF, NULL);
	close(fd);
	return 0;
}

static int mipi_set_dsi_format_cb(int argc, char *argv[])
{
	int fd = 0;
	unsigned char arg_temp = 0;
	unsigned long arg = 0;
	char *ptr = NULL;
	int argc_temp = 0;
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	argc_temp = argc;
	while (argc_temp)
		printf("%s\n", argv[--argc_temp]);
	if (argc > 1) {
		arg_temp = (unsigned char)strtol(argv[1], &ptr, 16);
		arg = arg_temp;
		printf("arg=%d\n", arg_temp);
		ioctl(fd, MIPI_DSI_SET_FORMAT, arg);
	}
	close(fd);
	return 0;
}

static int mipi_set_dsi_cfg_cb(int argc, char *argv[])
{
	int fd = 0;
	unsigned char arg = 0;
	char *ptr = NULL;
	int argc_temp = 0;
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	argc_temp = argc;
	while (argc_temp)
		printf("%s\n", argv[--argc_temp]);
	if (argc > 1) {
		arg = (unsigned char)strtol(argv[1], &ptr, 16);
		printf("arg=%d\n", arg);
		ioctl(fd, MIPI_DSI_SET_CFG, (unsigned char)arg);
	}
	close(fd);
	return 0;
}

static int mipi_dsi_init_cb(int argc, char *argv[])
{
	int fd = 0;
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	ioctl(fd, MIPI_DSI_INIT, NULL);
	close(fd);
	return 0;
}

static int mipi_sdi_send_cmds_cb(int argc, char *argv[])
{
	int fd = 0;
	unsigned long arg = 0;
	struct hc_dcs_cmds cmds;
	unsigned char mipi_data_buf[3] = { 0 };
	int len = 0, mipi_add = 0, count = 0, i = 1, j = 0;
	char *ptr = NULL;
	count = argc;
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	if (count > 1) {
		cmds.count = count - 1;
		memset(cmds.packet, 0, sizeof(cmds.packet));
		printf("mipi_send_cmds :");
		// ioctl(fd,MIPI_DSI_SET_OFF,NULL);
		while (count--) {
			len = strlen(argv[i]);
			if (len > 2)
				printf("len >2 %d %s", i, argv[i]);
			else {
				memcpy(mipi_data_buf, argv[i], len);
				cmds.packet[i - 1] = (unsigned char)strtol(
					mipi_data_buf, &ptr, 16);
			}
			printf("0x%02x ", cmds.packet[i - 1]);
			i++;
			if (count == 1)
				break;
		}
		printf("cmds.count=%d \n", cmds.count);
		arg = (unsigned long)&cmds;
		ioctl(fd, MIPI_DSI_SEND_DCS_CMDS, arg);
		// ioctl(fd,MIPI_DSI_SET_ON,NULL);
	}
	close(fd);
	return 0;
}

static int mipi_sdi_send_dcs_init_sequence_cb(int argc, char *argv[])
{
	int fd = 0;
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	ioctl(fd, MIPI_DSI_SEND_DCS_INIT_SEQUENCE, NULL);
	close(fd);
	return 0;
}

static int mipi_sdi_panel_set_cb(int argc, char *argv[])
{
	int fd = 0;
	unsigned long arg = 0;
	unsigned char mipi_data_buf[3] = { 0 };
	int len = 0, count = 0, i = 1, j = 0;
	char *ptr = NULL;
	count = argc;
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}

	if (count > 1) {
		// printf("%s\n", argv[1]);
		if(strcmp(argv[1], "-clean") == 0)
		{
			memset(&mipi_panel_cmds, 0, sizeof(struct hc_dcs_cmds));
			goto retVal;
		}
		if(strcmp(argv[1], "-sendcmds") == 0)
		{
			printf("mipi_panel_cmds.count=%d \n", mipi_panel_cmds.count);
			arg = (unsigned long)&mipi_panel_cmds;
			ioctl(fd, MIPI_DSI_SET_OFF, NULL);
			ioctl(fd, MIPI_DSI_SEND_DCS_CMDS, arg);
			ioctl(fd, MIPI_DSI_SET_ON, NULL);
			goto retVal;
		}
		if(strcmp(argv[1], "-copytopanel") == 0)
		{
			printf("mipi_panel_cmds.count=%d \n", mipi_panel_cmds.count);
			arg = (unsigned long)&mipi_panel_cmds;
			ioctl(fd, MIPI_DSI_SET_DCS_INIT_SEQUENCE, arg);
			goto retVal;
		}
		printf("mipi_send_mipi_panel_cmds :");
		// ioctl(fd,MIPI_DSI_SET_OFF,NULL);
		j = mipi_panel_cmds.count;
		mipi_panel_cmds.count += count - 1;
		while (count--) {
			len = strlen(argv[i]);
			if (len > 2)
			{
				printf("error len >2 %d %s\n", i, argv[i]);
				goto retVal;
			}
			else {
				memcpy(mipi_data_buf, argv[i], len);
				mipi_panel_cmds.packet[j + i - 1] = (unsigned char)strtol(mipi_data_buf, &ptr, 16);
			}
			printf("0x%02x ", mipi_panel_cmds.packet[j + i - 1]);
			i++;
			if (count == 1)
				break;
		}
		printf("\n");
	}
retVal:
	close(fd);
	return 0;
}

static int mipi_dsi_dcs_wr_cb(int argc, char *argv[])
{
	int fd = 0;
	unsigned long arg = 0;
	struct hc_dcs_read rd_data = { 0 };
	struct hc_dcs_write wd_data = { 0 };
	int i = 0, ret = 0;

	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	ioctl(fd,MIPI_DSI_SET_OFF,NULL);

	wd_data.type = 0x37;
	wd_data.delay_ms = 0x01;
	wd_data.len = 2;
	wd_data.payload[0] = 1;
	wd_data.payload[1] = 0;
	arg = (unsigned long)&wd_data;
	ioctl(fd, MIPI_DSI_DCS_WRITE, arg);

	rd_data.type = 0x14;
	rd_data.delay_ms = 0x00;
	rd_data.received_size = 0x02;
	rd_data.command[0] = 0x04;
	rd_data.command[1] = 0x02;
	arg = (unsigned long)&rd_data;
	ret = ioctl(fd, MIPI_DSI_DCS_READ, arg);
	if (ret == 0) {
		for (i = 0; i < 3; i++)
			printf("mipi_rd data[%d]=%d\n", i,rd_data.received_data[i]);
	}
	ioctl(fd, MIPI_DSI_SET_ON, NULL);
	close(fd);
	return 0;
}

static int mipi_sdi_get_timing_cb(int argc, char *argv[])
{
	struct mipi_display_timing timing={0};
	int fd = 0;
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	ioctl(fd,MIPI_DSI_GET_TIMING,&timing);
	printf("mipi_get_timing -C %d -t %d -a %d -f %d -s %d -b %d -T %d -A %d -F %d -S %d -B %d -c %d -o %d -l %d -m %d -H %d -V %d -D %d -d %d\n",
										timing.clk_frequency,timing.h_total_len,timing.h_active_len,\
										timing.h_front_len,timing.h_sync_len,timing.h_back_len,\
										timing.v_total_len,timing.v_active_len,timing.v_front_len,\
										timing.v_sync_len,timing.v_back_len,\
										timing.packet_cfg,timing.pclk,timing.lane_num,timing.color_mode,\
										timing.h_sync_level,timing.v_sync_level, timing.drive_strength, timing.dsi_flags);

	close(fd);
}

static void mipi_sdi_set_timing_print_usage(const char *prog)
{
		printf("Usage: %s \n", prog);
		puts(	"  -C --clock			set mipi timing clock-frequency\n"
				"  -t --h_total_len		set mipi timing h-total-len\n"
				"  -a --h_active_len	set mipi timing h-total-len\n"
				"  -f --h_front_len		set mipi timing h-front-len\n"
				"  -s --h_sync_len		set mipi timing h-sync-len\n"
				"  -b --h_back_len		set mipi timing h-back-len\n"
				"  -T --v_total_len		set mipi timing v-total-len\n"
				"  -A --v_active_len	set mipi timing v-active-len\n"
				"  -F --v_front_len		set mipi timing v-front-len\n"
				"  -S --v_sync_len		set mipi timing v-sync-len\n"
				"  -B --v_back_len		set mipi timing v-back-len\n"
				"  -c --cfg				set mipi timing dsi,cfg\n"
				"  -o --output_clock	set mipi timing output-clock\n"
				"  -l --lanes			set mipi timing dsi,lanes\n"
				"  -m --format			set mipi timing dsi,format\n"
				"  -D --drive_strength	set mipi timing drive_strength\n"
				"  -d --dsi_flags		set mipi timing dsi,flags\n"
				"  -p --swap			set mipi timing dsi,swap\n"
				"  -H --h_sync_level	set mipi timing h_sync_level\n"
				"  -V --v_sync_level	set mipi timing v_sync_level\n"
				"eg: timing_set -C 0 -t 1228 -a 1080 -f 116 -s 16 -b 16 -T 2015 -A 1920 -F 59 -S 4 -B 32 -c 28 -o 14 -l 4 -m 5 -p 0 -H 1 -V 1"
		);
}

static int mipi_sdi_set_timing_cb(int argc, char *argv[])
{
	int fd = 0;
	static const struct option lopts[] = {
			{ "clock",			1, 0, 'C' },
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
			{ "cfg",			1, 0, 'c' },
			{ "output_clock",	1, 0, 'o' },
			{ "lanes",			1, 0, 'l' },
			{ "format",			1, 0, 'm' },
			{ "h_sync_level",	1, 0, 'H' },
			{ "v_sync_level",	1, 0, 'V' },
			{ "drive_strength",	1, 0, 'D' },
			{ "dsi_flags",		1, 0, 'd' },
			{ "swap",			1, 0, 'p' },
			{ "help",			1, 0, 'h' },
			{ "NULL",			0, 0, 0 },
		};
	struct mipi_display_timing timing={0};
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	ioctl(fd,MIPI_DSI_GET_TIMING,&timing);
	opterr = 0;
	optind = 0;
	while (1) {
		int c;

		c = getopt_long(argc, argv, "C:t:a:f:s:b:T:A:F:S:B:c:o:l:m:H:V:h:D:d:p:", lopts, NULL);
		// printf("%s %d %c\n",__func__,__LINE__,c);
		if (c == -1) {
			break;
		}

		switch(c) {
		case 'C':
			timing.clk_frequency = atoi((const char*)optarg);
			break;
		case 't':
			timing.h_total_len = atoi((const char*)optarg);
			break;
		case 'a':
			timing.h_active_len = atoi((const char*)optarg);
			break;
		case 'f':
			timing.h_front_len = atoi((const char*)optarg);
			break;
		case 's':
			timing.h_sync_len = atoi((const char*)optarg);
			break;
		case 'b':
			timing.h_back_len = atoi((const char*)optarg);
			break;
		case 'T':
			timing.v_total_len = atoi((const char*)optarg);
			break;
		case 'A':
			timing.v_active_len = atoi((const char*)optarg);
			break;
		case 'F':
			timing.v_front_len = atoi((const char*)optarg);
			break;
		case 'S':
			timing.v_sync_len = atoi((const char*)optarg);
			break;
		case 'B':
			timing.v_back_len = atoi((const char*)optarg);
			break;
		case 'c':
			timing.packet_cfg = atoi((const char*)optarg);
			break;
		case 'o':
			timing.pclk = atoi((const char*)optarg);
			break;
		case 'l':
			timing.lane_num = atoi((const char*)optarg);
			break;
		case 'm':
			timing.color_mode = atoi((const char*)optarg);
			break;
		case 'H':
			timing.h_sync_level = atoi((const char*)optarg);
			break;
		case 'V':
			timing.v_sync_level = atoi((const char*)optarg);
			break;
		case 'D':
			timing.drive_strength = atoi((const char*)optarg);
			break;
		case 'd':
			timing.dsi_flags = atoi((const char*)optarg);
			break;
		case 'p':
			timing.swap_flags = atoi((const char*)optarg);
			break;
		case 'h':
			mipi_sdi_set_timing_print_usage(argv[0]);
			break;
		default:
			mipi_sdi_set_timing_print_usage(argv[0]);
			return -1;
		}
	}
	printf("mipi_get_timing -C %d -t %d -a %d -f %d -s %d -b %d -T %d -A %d -F %d -S %d -B %d -c %d -o %d -l %d -m %d -H %d -V %d -D %d -d %d -p %d\n",
										timing.clk_frequency,timing.h_total_len,timing.h_active_len,\
										timing.h_front_len,timing.h_sync_len,timing.h_back_len,\
										timing.v_total_len,timing.v_active_len,timing.v_front_len,\
										timing.v_sync_len,timing.v_back_len,\
										timing.packet_cfg,timing.pclk,timing.lane_num,timing.color_mode,\
										timing.h_sync_level,timing.v_sync_level, timing.drive_strength, timing.dsi_flags,\
										timing.swap_flags);
	ioctl(fd,MIPI_DSI_SET_TIMING,&timing);
	close(fd);
	return 0;
}

static int mipi_sdi_set_timing_init_cb(int argc, char *argv[])
{
	int fd = 0;
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	ioctl(fd,MIPI_DSI_TIMING_INIT,NULL);
	close(fd);
}

static void mipi_sdi_reset_print_usage(const char *prog)
{
		printf("Usage: %s \n", prog);
		puts(	"  -T --time   		set reset time\n"
				"  -p --pinpad  	set reset pinpad\n"
				"  -a --active	 	set reset pinpad active\n"
				"	eg: reset_test -t 200 -p 99 -a 1\n"
		 );
}

static int gpio_reset_time = 200;
static int gpio_reset_pin = PINPAD_INVALID;
static bool gpio_reset_active = GPIO_ACTIVE_HIGH;
static int mipi_sdi_reset_test_init(int argc, char *argv[])
{
	int fd = 0;
	fd = open(MIPIDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", MIPIDEV_PATH, fd);
		return -1;
	}
	static const struct option lopts[] = {
			{ "time",			1, 0, 't' },
			{ "pinpad",			1, 0, 'p' },
			{ "active",			1, 0, 'a' },
			{ "help",			1, 0, 'h' },
			{ "NULL",			0, 0, 0 },
		};
	opterr = 0;
	optind = 0;
	while (1) {
		int c;

		c = getopt_long(argc, argv, "t:p:a:h:", lopts, NULL);
		// printf("%s %d %c\n",__func__,__LINE__,c);
		if (c == -1) {
			break;
		}

		switch(c) {
		case 't':
			gpio_reset_time = atoi((const char*)optarg);
			break;
		case 'p':
			gpio_reset_pin = atoi((const char*)optarg);
			break;
		case 'a':
			gpio_reset_active = atoi((const char*)optarg);
			break;
		case 'h':
			mipi_sdi_reset_print_usage(argv[0]);
			break;
		default:
			mipi_sdi_reset_print_usage(argv[0]);
			return -1;
		}
	}

	printf("gpio_reset_time = %d,gpio_reset_pin = %d,gpio_reset_active = %d\n",gpio_reset_time,gpio_reset_pin,gpio_reset_active);
	gpio_configure(gpio_reset_pin,GPIO_DIR_OUTPUT);
	gpio_set_output(gpio_reset_pin,!gpio_reset_active);
	msleep(gpio_reset_time);
	gpio_set_output(gpio_reset_pin,gpio_reset_active);
	msleep(gpio_reset_time);
	gpio_set_output(gpio_reset_pin,!gpio_reset_active);
	msleep(gpio_reset_time);
	ioctl(fd,MIPI_DSI_TIMING_INIT,NULL);
	ioctl(fd,MIPI_DSI_SEND_DCS_INIT_SEQUENCE,NULL);
	ioctl(fd,MIPI_DSI_SET_ON,NULL);
	close(fd);
	return 0;
}

static void mipi_dsi_init_task_print_usage(const char *prog)
{
		printf("Usage: %s \n", prog);
		puts(
				"  -I --init-delay-ms     set init-delay-ms\n"
				"  -R --reset-delay-ms    set reset-delay-ms\n"
				"  -E --enable-delay-ms   set enable-delay-ms\n"
				"  -P --prepare-delay-ms  set prepare-delay-ms\n"
				"  -D --task-delay-ms     set task-delay-ms\n"
				"  -p --enable-pinpad     set reset pinpad\n"
				"  -a --enable-active     set reset pinpad active\n"
				"  -p --pinpad            set reset pinpad\n"
				"  -a --active            set reset pinpad active\n"
				"  -t --task              set task 0 : INIT_TEST_DEFAULT, 1: INIT_TEST_CREATE, 2 : INIT_TEST_STOP, 3 : INIT_TEST_RESUME 4: INIT_TEST_DELETE\n"
				"  -h --help              help\n"
				"  eg: task_test -I 0 -R 0 -E 0 -P 0 -D 10000 -e 160 -o 1 -p 160 -a 0 -t 0\n"
		 );
}

typedef enum INIT_TASK_STATUS_E_
{
	INIT_TEST_DEFAULT = 0,
	INIT_TEST_CREATE,
	INIT_TEST_STOP,
	INIT_TEST_RESUME,
	INIT_TEST_DELETE,
}init_task_status_e;

static init_task_status_e init_task_status = INIT_TEST_DEFAULT;
static int gpio_enable_pin = PINPAD_INVALID;
static int gpio_enable_active = 1;
static int init_delay_ms;
static int reset_delay_ms;
static int enable_delay_ms;
static int prepare_delay_ms;
static int task_delay_ms = 10000;
static int tast_count = 0;
TaskHandle_t init_test_handle = NULL;

static void mipi_dsi_init_task(void *args)
{
	while(1)
	{
		if(init_task_status == INIT_TEST_DELETE)
			break;

	#if 1
		msleep(init_delay_ms);
		gpio_configure(gpio_enable_pin, GPIO_DIR_OUTPUT);
		gpio_set_output(gpio_enable_pin, gpio_enable_active);
		msleep(enable_delay_ms);
		gpio_configure(gpio_reset_pin,GPIO_DIR_OUTPUT);
		gpio_set_output(gpio_reset_pin,gpio_reset_active);
		msleep(gpio_reset_time);
		gpio_configure(gpio_reset_pin,GPIO_DIR_OUTPUT);
		gpio_set_output(gpio_reset_pin,!gpio_reset_active);
		msleep(prepare_delay_ms);
		msleep(10);

		mipi_dsi_init_cb(1, ((char *[]){ "init" }));
		tast_count++;
		msleep(task_delay_ms);
	#else
		/*
			System Reset
			console_run_cmd("mipi_test task_test -t 1\n");//on app
		*/
		msleep(task_delay_ms);
		extern int reset(void);
		reset();
	#endif
	}
	printf("task delete tast_count = %d \n", tast_count);
	vTaskDelete(init_test_handle);
	// init_test_handle = NULL;
}

static int mipi_dsi_init_test_cb(int argc, char *argv[])
{
	eTaskState pThreadState;
	static const struct option lopts[] = {
			{ "init-delay-ms",		1, 0, 'I' },
			{ "reset-delay-ms",		1, 0, 'R' },
			{ "enable-delay-ms",	1, 0, 'E' },
			{ "prepare-delay-ms",	1, 0, 'P' },
			{ "task-delay-ms",		1, 0, 'D' },
			{ "enable-pinpad",		1, 0, 'e' },
			{ "enable-active",		1, 0, 'o' },
			{ "reset-pinpad",		1, 0, 'p' },
			{ "active",				1, 0, 'a' },
			{ "task",				1, 0, 't' },
			{ "help",				0, 0, 'h' },
			{ "NULL",				0, 0, 0 },
		};
	opterr = 0;
	optind = 0;
	init_task_status = INIT_TEST_DEFAULT;
	while (1) {
		int c;

		c = getopt_long(argc, argv, "I:R:E:P:D:e:o:p:a:t:h", lopts, NULL);
		// printf("%s %d %c\n",__func__,__LINE__,c);
		if (c == -1) {
			break;
		}

		switch(c) {
		case 'I':
			init_delay_ms = atoi((const char*)optarg);
			break;
		case 'R':
			reset_delay_ms = atoi((const char*)optarg);
			break;
		case 'E':
			enable_delay_ms = atoi((const char*)optarg);
			break;
		case 'P':
			prepare_delay_ms = atoi((const char*)optarg);
			break;
		case 'D':
			task_delay_ms = atoi((const char*)optarg);
			break;
		case 'e':
			gpio_enable_pin = atoi((const char*)optarg);
			break;
		case 'o':
			gpio_enable_active = atoi((const char*)optarg);
			break;
		case 'p':
			gpio_reset_pin = atoi((const char*)optarg);
			break;
		case 'a':
			gpio_reset_active = atoi((const char*)optarg);
			break;
		case 't':
			init_task_status = atoi((const char*)optarg);
			break;
		case 'h':
			mipi_dsi_init_task_print_usage(argv[0]);
			break;
		default:
			mipi_dsi_init_task_print_usage(argv[0]);
		}
	}

	if(init_test_handle == NULL)
	{
		if(init_task_status == INIT_TEST_CREATE)
		{
			printf("task create\n");
			if(xTaskCreate(mipi_dsi_init_task , "mipi_dsi_init_task" ,\
									0x1000 , NULL , portPRI_TASK_NORMAL , &init_test_handle) != pdTRUE)
			{
				init_test_handle == NULL;
				printf("kshm recv thread create failed\n");
			}
		}
	}
	else
	{
		pThreadState = eTaskGetState(init_test_handle);
		printf("pThreadState = %d tast_count = %d\n", pThreadState, tast_count);
		if((pThreadState == eDeleted || pThreadState == eInvalid)&& init_task_status == INIT_TEST_CREATE)
		{
			printf("task create\n");
			if(xTaskCreate(mipi_dsi_init_task , "mipi_dsi_init_task" ,\
									0x1000 , NULL , portPRI_TASK_NORMAL , &init_test_handle) != pdTRUE)
			{
				printf("kshm recv thread create failed\n");
			}
		}
		else if(init_task_status == INIT_TEST_STOP)
		{
			if(pThreadState == eRunning || pThreadState == eReady || pThreadState ==  eBlocked)
			{
				printf("task stop \n");
				vTaskSuspend(init_test_handle);
			}
		}
		else if(init_task_status == INIT_TEST_RESUME)
		{
			if(pThreadState == eSuspended)
			{
				printf("task resume \n");
				vTaskResume(init_test_handle);
			}
		}
	}
	
	printf("mipi_test -I %d -R %d -E %d -P %d -D %d -e %d -o %d -p %d -a %d -t %d\n", \
			init_delay_ms, reset_delay_ms, enable_delay_ms, prepare_delay_ms, task_delay_ms, \
			gpio_enable_pin, gpio_enable_active, gpio_reset_pin, gpio_reset_active, init_task_status);

	return 0;
}


CONSOLE_CMD(mipi_test, NULL, mipi_test_enter, CONSOLE_CMD_MODE_SELF, "enter mipi test")
CONSOLE_CMD(on, "mipi_test", mipi_dsi_on_cb, CONSOLE_CMD_MODE_SELF, "mipi hal on send, eg: on")
CONSOLE_CMD(off, "mipi_test", mipi_dsi_off_cb, CONSOLE_CMD_MODE_SELF, "mipi hal off send, eg: off")
CONSOLE_CMD(color, "mipi_test", mipi_set_dsi_format_cb, CONSOLE_CMD_MODE_SELF, "mipi dsi color set,0<vlaue<5, eg: color 5")
CONSOLE_CMD(cfg, "mipi_test", mipi_set_dsi_cfg_cb, CONSOLE_CMD_MODE_SELF, "mipi dsi cfg set, <! u8 config: [0, 3f], eg: cfg 1c")
CONSOLE_CMD(init, "mipi_test", mipi_dsi_init_cb, CONSOLE_CMD_MODE_SELF, "Re execute initialization Mipi, eg: init")
CONSOLE_CMD(cmds, "mipi_test", mipi_sdi_send_cmds_cb, CONSOLE_CMD_MODE_SELF, "mipi d0 send cmds, eg: cmds 05 00 01 29")
CONSOLE_CMD(init_seq, "mipi_test", mipi_sdi_send_dcs_init_sequence_cb, CONSOLE_CMD_MODE_SELF, "mipi send panel-init-sequence data")
CONSOLE_CMD(panel_set, "mipi_test", mipi_sdi_panel_set_cb, CONSOLE_CMD_MODE_SELF, "set panel-init-sequence, eg: panel_set 05 00 01 29")
CONSOLE_CMD(wr, "mipi_test", mipi_dsi_dcs_wr_cb, CONSOLE_CMD_MODE_SELF, "Mipi read / write, eg: wr")
CONSOLE_CMD(timing_get, "mipi_test", mipi_sdi_get_timing_cb, CONSOLE_CMD_MODE_SELF, "mipi get timing data, eg: timing_get")
CONSOLE_CMD(timing_set, "mipi_test", mipi_sdi_set_timing_cb, CONSOLE_CMD_MODE_SELF, "set mipi timing, eg: timing_set -C 0 -t 1228 -a 1080 -f 116 -s 16 -b 16 -T 2015 -A 1920 -F 59 -S 4 -B 32 -c 28 -o 14 -l 4 -m 5 -H 1 -V 1")
CONSOLE_CMD(timing_init, "mipi_test", mipi_sdi_set_timing_init_cb, CONSOLE_CMD_MODE_SELF, "mipi set timing init, eg: timing_init")
CONSOLE_CMD(reset_test, "mipi_test", mipi_sdi_reset_test_init, CONSOLE_CMD_MODE_SELF, "mipi reset test, eg: reset_test -t 200 -p 99 -a 1")
CONSOLE_CMD(task_test, "mipi_test", mipi_dsi_init_test_cb, CONSOLE_CMD_MODE_SELF, "mipi task test, eg: task_test -I 0 -R 0 -E 0 -P 0 -D 10000 -e 160 -o 1 -p 160 -a 0 -t 0")
