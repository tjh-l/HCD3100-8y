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
#include <hcuapi/lcd.h>
#include <hcuapi/backlight.h>

#define LCDDEV_PATH  "/dev/lcddev"
#define BACKLIGHT_PATH "/dev/backlight"

static int lcd_test_enter(int argc, char *argv[])
{
	return 0;
}

static int lcd_init_all_cb(int argc, char *argv[])
{
	int fd = 0;
	fd = open(LCDDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LCDDEV_PATH, fd);
		return -1;
	}
	ioctl(fd, LCD_INIT, NULL);
	close(fd);
	return 0;
}

static int lcd_init_driver_cb(int argc, char *argv[])
{
	int fd = 0;
	fd = open(LCDDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LCDDEV_PATH, fd);
		return -1;
	}
	ioctl(fd, LCD_SEND_INIT_SEQUENCE, NULL);
	close(fd);
	return 0;
}

static int lcd_rotate_cb(int argc, char *argv[])
{
	int fd = 0;
	int val = 0;
	fd = open(LCDDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LCDDEV_PATH, fd);
		return -1;
	}

	if (argc > 1) {
		val = atoi(argv[1]);
		ioctl(fd, LCD_SET_ROTATE, val);
		printf("rotate val = %d\n", val);
		close(fd);
	}
	return 0;
}

static int lcd_cmds_data_cb(int argc, char *argv[])
{
	int fd = 0;
	int val = 0, i = 1;
	unsigned char lcd_data_buf[5] = { 0 };
	char *ptr;
	unsigned long arg = 0;
	struct hc_lcd_write cmds;
	fd = open(LCDDEV_PATH, O_RDWR);
	int count = 0;
	if (argc == 2) {
		/*
			lcddev send cmds
		*/
		cmds.count = 1;
		memcpy(lcd_data_buf, argv[i], 4);
		cmds.packet[i - 1] = (unsigned short)strtol(lcd_data_buf, &ptr, 16);
		printf("packet = %d cmds.count = %d \n", cmds.packet[0], cmds.count);
		arg = (unsigned long)&cmds;
		ioctl(fd, LCD_SEND_CMDS, arg);
		close(fd);
	} else if (argc > 2) {
		/*
			lcddev send cmds
		*/
		cmds.count = 1;
		memcpy(lcd_data_buf, argv[i], 4);
		cmds.packet[0] = (unsigned short)strtol(lcd_data_buf, &ptr, 16);
		printf("packet = %d cmds.count = %d \n", cmds.packet[0],cmds.count);
		arg = (unsigned long)&cmds;
		ioctl(fd, LCD_SEND_CMDS, arg);

		/*
			lcddev send data
		*/
		count = cmds.count = argc - 2;
		memset(cmds.packet, 0, count);
		printf("lcd cmds:");
		while (count-- > 0) {
			memcpy(lcd_data_buf, argv[i + 1], 4);
			cmds.packet[i - 1] = (unsigned short)strtol(lcd_data_buf, &ptr, 16);
			printf("0x%04x ", cmds.packet[i - 1]);
			i++;
		}
		printf("cmds.count=%d \n", cmds.count);
		arg = (unsigned long)&cmds;
		ioctl(fd, LCD_SEND_DATA, arg);
		close(fd);
	}
}

static int lcd_send_data_cb(int argc, char *argv[])
{
	int fd = 0;
	int val = 0, i = 1;
	unsigned char lcd_data_buf[5] = { 0 };
	char *ptr;
	unsigned long arg = 0;
	struct hc_lcd_write cmds;
	fd = open(LCDDEV_PATH, O_RDWR);
	int count = 0;
	if (argc > 1) {
		count = cmds.count = argc - 1;
		memset(cmds.packet, 0, sizeof(cmds.packet));
		printf("lcd cmds:");
		while (count-- > 0) {
			memcpy(lcd_data_buf, argv[i], 4);
			cmds.packet[i - 1] =
				(unsigned short)strtol(lcd_data_buf, &ptr, 16);
			printf("0x%04x ", cmds.packet[i - 1]);
			i++;
		}
		printf("cmds.count=%d \n", cmds.count);
		arg = (unsigned long)&cmds;
		ioctl(fd, LCD_SEND_DATA, arg);
		close(fd);
	}
}

static int lcd_onoff_cb(int argc, char *argv[])
{
	int fd = 0;
	int val = 0;
	fd = open(LCDDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LCDDEV_PATH, fd);
		return -1;
	}

	if (argc > 1) {
		val = atoi(argv[1]);
		ioctl(fd, LCD_SET_ONOFF, val);
		printf("%s %d val = %d\n", __func__, __LINE__, val);
		close(fd);
	}
	return 0;
}

static int lcd_pwm_vcom_cb(int argc, char *argv[])
{
	int fd = 0;
	int val = 0;
	fd = open(LCDDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LCDDEV_PATH, fd);
		return -1;
	}

	if (argc > 1) {
		val = atoi(argv[1]);
		ioctl(fd, LCD_SET_ONOFF, val);
		printf("%s %d val = %d\n", __func__, __LINE__, val);
		close(fd);
	}

	return 0;
}

static int lcd_gpio_power_cb(int argc, char *argv[])
{
	int fd = 0;
	int val = 0;
	fd = open(LCDDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LCDDEV_PATH, fd);
		return -1;
	}

	if (argc > 1) {
		val = atoi(argv[1]);
		ioctl(fd, LCD_SET_POWER_GPIO, val);
		printf("%s %d val = %d\n", __func__, __LINE__, val);
		close(fd);
	}

	return 0;
}

static int lcd_reset_cb(int argc, char *argv[])
{
	int fd = 0;
	int val = 0;
	fd = open(LCDDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LCDDEV_PATH, fd);
		return -1;
	}

	if (argc > 1) {
		val = atoi(argv[1]);
		ioctl(fd, LCD_SET_RESET_GPIO, val);
		printf("%s %d val = %d\n", __func__, __LINE__, val);
		close(fd);
	}

	return 0;
}

static void io_lvds_gpio_set_output(unsigned int pad,unsigned char value)
{
	int fd = 0;
	fd = open("/dev/lvds", O_RDWR);
	if (fd < 0) {
		printf("open /dev/lvds failed, ret=%d\n", fd);
		return;
	}
	struct lvds_set_gpio lvds_pad;
	lvds_pad.padctl = pad;
	lvds_pad.value = value;
	ioctl(fd, LVDS_SET_GPIO_OUT, &lvds_pad);
	close(fd);
}

static int lvds_gpio_test(int argc, char *argv[])
{
	struct lvds_set_gpio lvds_pad;
	if (argc > 2) {
		lvds_pad.padctl = atoi(argv[1]);
		lvds_pad.value = atoi(argv[2]);
		printf("pad= %d value=%d\n", lvds_pad.padctl, lvds_pad.value);
		io_lvds_gpio_set_output(lvds_pad.padctl, lvds_pad.value);
	}
	return 0;
}

static int gpio_set_out(int argc, char *argv[])
{
	int gpio_pin = 0;
	int val = 0;
	if (argc > 2) {
		gpio_pin = atoi(argv[1]);
		val = atoi(argv[2]);
		gpio_set_output(gpio_pin, (bool)val);
		gpio_configure(gpio_pin, GPIO_DIR_OUTPUT);
		printf("gpio_pin = %d, val = %d\n", gpio_pin, val);
	}
	return 0;
}

static int gpio_set_pinpad_function(int argc, char *argv[])
{
	int gpio_pin = 0;
	int val = 0;
	if (argc > 2) {
		gpio_pin = atoi(argv[1]);
		val = atoi(argv[2]);
		gpio_configure(gpio_pin, val);
		printf("gpio_pin = %d, val = %d\n", gpio_pin, val);
	}
	return 0;
}

static int lcd_chmod_pin_mode_cb(int argc, char *argv[])
{
	int gpio_pin = 0;
	int val = 0;
	if (argc > 2) {
		gpio_pin = atoi(argv[1]);
		val = atoi(argv[2]);
		pinmux_configure(gpio_pin, val);
		printf("mode = %d, val = %d\n", gpio_pin, val);
	}
	return 0;
}

static void lcd_spi_info_print_usage(const char *prog)
{
		printf("Usage: %s \n", prog);
		puts(	"  -k --sck		set spi sck\n"
				"  -o --mosi		set spi sck\n"
				"  -i --miso set spi sck\n"
				"  -c --cs		set spi sck\n"
				"  -M --mode		set spi sck\n"
				"  -B --bit		set spi bit\n"
				"  -D --ordata		set or data\n"
				"  -C --orcmds		set or cmds\n"
				"  -h --help\n"
				"eg: spi_info -k 0 -o 0 -i 0 -c 7 -M 0 -B 9 -D 256 -C 0\n"
		);
}

static int lcd_set_spi_info_cb(int argc, char *argv[])
{
	int fd = 0;
	int ret = 0;
	static const struct option lopts[] = {
			{ "sck",		1, 0, 'k' },
			{ "mosi",		1, 0, 'o' },
			{ "miso",		1, 0, 'i' },
			{ "cs",			1, 0, 'c' },
			{ "mode",		1, 0, 'M' },
			{ "bit",		1, 0, 'B' },
			{ "ordata",		1, 0, 'D' },
			{ "orcmds",		1, 0, 'C' },
			{ "help",		1, 0, 'h' },
			{ "NULL",		0, 0, 0 },
		};
	struct hc_lcd_mode_info info;
	fd = open(LCDDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LCDDEV_PATH, fd);
		return -1;
	}

	ret = ioctl(fd, LCD_GET_MODE_INFO, &info);
	if(ret < 0)
		return 0;

	if(info.mode & LCD_INIT_SEQUENCE_SPI == 0)
		return 0;

	opterr = 0;
	optind = 0;
	while (1) {
		int c;

		c = getopt_long(argc, argv, "k:o:i:c:M:B:D:C:h:", lopts, NULL);
		// printf("%s %d %c\n",__func__,__LINE__,c);
		if (c == -1) {
			break;
		}

		switch(c) {
		case 'k':
			info.spi.sck = atoi((const char*)optarg);
			break;
		case 'o':
			info.spi.mosi = atoi((const char*)optarg);
			break;
		case 'i':
			info.spi.miso = atoi((const char*)optarg);
			break;
		case 'c':
			info.spi.cs = atoi((const char*)optarg);
			break;
		case 'M':
			info.spi.mode = atoi((const char*)optarg);
			break;
		case 'B':
			info.spi.bit = atoi((const char*)optarg);
			break;
		case 'C':
			info.spi.orcmds = atoi((const char*)optarg);
			break;
		case 'D':
			info.spi.ordata = atoi((const char*)optarg);
			break;
		case 'h':
			lcd_spi_info_print_usage(argv[0]);
			break;
		default:
			lcd_spi_info_print_usage(argv[0]);
			return -1;
		}
	}
	printf("info.spi -k %d -o %d -i %d -c %d -M %d -B %d -D %d -C %d\n", info.spi.sck, info.spi.mosi, info.spi.miso, info.spi.cs, info.spi.mode, info.spi.bit, info.spi.orcmds, info.spi.ordata);
	ioctl(fd, LCD_SET_MODE_INFO, &info);

	close(fd);
	return ret;
}

static void lcd_i2c_info_print_usage(const char *prog)
{
		printf("Usage: %s \n", prog);
		puts(	"  -k --scl		set i2c scl\n"
				"  -o --sda		set i2c sda\n"
				"  -D --ordata		set or data\n"
				"  -C --orcmds		set or cmds\n"
				"  -A --addr		set addr\n"
				"  -h --help\n"
				"eg: i2c_info -k 0 -o 0 -D 256 -C 0 -A 255\n"
		);
}

static int lcd_set_i2c_info_cb(int argc, char *argv[])
{
	int fd = 0;
	int ret = 0;
	static const struct option lopts[] = {
			{ "scl",		1, 0, 'k' },
			{ "sda",		1, 0, 'o' },
			{ "ordata",		1, 0, 'D' },
			{ "orcmds",		1, 0, 'C' },
			{ "addr",		1, 0, 'A' },
			{ "help",		1, 0, 'h' },
			{ "NULL",		0, 0, 0 },
		};
	struct hc_lcd_mode_info info;
	fd = open(LCDDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LCDDEV_PATH, fd);
		return -1;
	}

	ret = ioctl(fd, LCD_GET_MODE_INFO, &info);
	if(ret < 0)
		return -1;

	if(info.mode & LCD_INIT_SEQUENCE_I2C == 0)
		return -1;

	opterr = 0;
	optind = 0;
	while (1) {
		int c;

		c = getopt_long(argc, argv, "k:o:D:C:A:h:", lopts, NULL);
		// printf("%s %d %c\n",__func__,__LINE__,c);
		if (c == -1) {
			break;
		}

		switch(c) {
		case 'k':
			info.i2c.scl = atoi((const char*)optarg);
			break;
		case 'o':
			info.i2c.sda = atoi((const char*)optarg);
			break;
		case 'C':
			info.i2c.orcmds = atoi((const char*)optarg);
			break;
		case 'D':
			info.i2c.ordata = atoi((const char*)optarg);
			break;
		case 'A':
			info.i2c.addr = atoi((const char*)optarg);
			break;
		case 'h':
			lcd_i2c_info_print_usage(argv[0]);
			break;
		default:
			lcd_i2c_info_print_usage(argv[0]);
			return -1;
		}
	}
	printf("set_i2c_info -k %d -o %d -D %d -C %d -A %d\n", info.i2c.scl, info.i2c.sda, info.i2c.orcmds, info.i2c.ordata, info.i2c.addr);
	ioctl(fd, LCD_SET_MODE_INFO, &info);

	close(fd);
	return ret;
}

static void lcd_pinpad_info_print_usage(const char *prog)
{
		printf("Usage: %s \n", prog);
		puts(	"  -o --power		set pinpad power\n"
				"  -r --reset		set pinpad reset\n"
				"  -s --stbyb		set pinpad stbyb\n"
				"  -p --pinpad0		set pinpad pinpad0\n"
				"  -P --pinpad1		set pinpad pinpad1\n"
				"  -h --help\n"
				"eg: pinpad_info -o 1 -r 2 -s 3 -p 4 -P 5\n"
		);
}

static int lcd_set_pinpad_info_cb(int argc, char *argv[])
{
	int fd = 0;
	int ret = 0;
	static const struct option lopts[] = {
			{ "power",		1, 0, 'o' },
			{ "reset",		1, 0, 'r' },
			{ "stbyb",		1, 0, 's' },
			{ "pinpad0",	1, 0, 'p' },
			{ "pinpad1",	1, 0, 'P' },
			{ "help",		1, 0, 'h' },
			{ "NULL",		0, 0, 0 },
		};
	struct hc_lcd_mode_info info;
	fd = open(LCDDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LCDDEV_PATH, fd);
		return -1;
	}

	ret = ioctl(fd, LCD_GET_MODE_INFO, &info);
	if(ret < 0)
	{
		goto rethanle;
	}

	opterr = 0;
	optind = 0;
	while (1) {
		int c;

		c = getopt_long(argc, argv, "o:r:s:p:P:h:", lopts, NULL);
		// printf("%s %d %c\n",__func__,__LINE__,c);
		if (c == -1) {
			break;
		}

		switch(c) {
		case 'o':
			info.pinpad.power = atoi((const char*)optarg);
			break;
		case 'r':
			info.pinpad.reset = atoi((const char*)optarg);
			break;
		case 's':
			info.pinpad.stbyb = atoi((const char*)optarg);
			break;
		case 'p':
			info.pinpad.pinpad0 = atoi((const char*)optarg);
			break;
		case 'P':
			info.pinpad.pinpad1 = atoi((const char*)optarg);
			break;
		case 'h':
			lcd_pinpad_info_print_usage(argv[0]);
			break;
		default:
			lcd_pinpad_info_print_usage(argv[0]);
		}
	}
	printf("set_pinpad_info -o %d -r %d -s %d -p %d -P %d\n", info.pinpad.power, info.pinpad.reset, info.pinpad.stbyb, info.pinpad.pinpad0, info.pinpad.pinpad1);
	ioctl(fd, LCD_SET_MODE_INFO, &info);

rethanle:
	close(fd);
	return ret;
}

static struct hc_lcd_init_sequence lcd_panel_cmds;
static int lcd_panel_set_cb(int argc, char *argv[])
{
	int fd = 0;
	unsigned long arg = 0;
	unsigned char lcd_data_buf[3] = { 0 };
	int len = 0, count = 0, i = 1, j = 0;
	char *ptr = NULL;
	count = argc;
	fd = open(LCDDEV_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", LCDDEV_PATH, fd);
		return -1;
	}

	if (count > 1) {
		if(strcmp(argv[1], "-clean") == 0)
		{
			memset(&lcd_panel_cmds, 0, sizeof(struct hc_lcd_init_sequence));
			goto retVal;
		}
		else if(strcmp(argv[1], "-sendcmds") == 0)
		{
			printf("lcd_panel_cmds.count=%d \n", lcd_panel_cmds.count);
			arg = (unsigned long)&lcd_panel_cmds;
			ioctl(fd, LCD_SET_INIT_SEQUENCE, arg);
			ioctl(fd, LCD_INIT, NULL);
			goto retVal;
		}
		else if(strcmp(argv[1], "-spimode") == 0)
		{
			lcd_panel_cmds.flag = LCD_INIT_SEQUENCE_SPI;
			goto retVal;
		}
		else if(strcmp(argv[1], "-i2cmode") == 0)
		{
			lcd_panel_cmds.flag = LCD_INIT_SEQUENCE_I2C;
			goto retVal;
		}
		printf("mipi_send_panel_cmds :");
		// ioctl(fd,MIPI_DSI_SET_OFF,NULL);
		j = lcd_panel_cmds.count;
		lcd_panel_cmds.count += count - 1;
		while (count--) {
			len = strlen(argv[i]);
			if (len > 2)
			{
				printf("error len >2 %d %s\n", i, argv[i]);
				goto retVal;
			}
			else {
				memcpy(lcd_data_buf, argv[i], len);
				lcd_panel_cmds.packet[j + i - 1] = (unsigned char)strtol(lcd_data_buf, &ptr, 16);
			}
			printf("0x%02x ", lcd_panel_cmds.packet[j + i - 1]);
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

static void backlight_info_print_usage(const char *prog)
{
		printf("Usage: %s \n", prog);
		puts(	"  -b --brightness		set pinpad power\n"
				"  -f --frequency		set pinpad reset\n"
				"  -p --pwmpolarity		set pinpad stbyb\n"
				"  -h --help\n"
				"eg: backlight_test -b 1 -f 1000\n"
		);
}

static int lcd_backlight_test(int argc, char *argv[])
{
	int fd = 0;
	unsigned int i = 0;
	struct backlight_info info = {0};
	int stop_flag = 0;
	static const struct option lopts[] = {
			{ "brightness",		1, 0, 'b' },
			{ "frequency",		1, 0, 'f' },
			{ "pwmpolarity",	1, 0, 'p' },
			{ "stop",		1, 0, 's' },
			{ "help",		1, 0, 'h' },
			{ "NULL",		0, 0, 0 },
		};
	fd = open(BACKLIGHT_PATH, O_RDWR);
	if (fd < 0) {
		printf("open %s failed, ret=%d\n", BACKLIGHT_PATH, fd);
		return -1;
	}
	ioctl(fd, BACKLIGHT_GET_INFO, &info);
	opterr = 0;
	optind = 0;
	while (1) {
		int c;
		c = getopt_long(argc, argv, "b:f:p:sh:", lopts, NULL);
		if (c == -1) {
			break;
		}

		switch (c) {
		case 'f':
			info.pwm_frequency = atoi((const char *)optarg);
			break;
		case 'b':
			info.brightness_value = atoi((const char *)optarg);
			break;
		case 'p':
			info.pwm_polarity = atoi((const char *)optarg);
			break;
		case 's':
			stop_flag = 1;
			break;
		case 'h':
			backlight_info_print_usage(argv[0]);
			break;
		default:
			backlight_info_print_usage(argv[0]);
		}
	}

	if (!stop_flag) {
		ioctl(fd, BACKLIGHT_SET_INFO, &info);
		ioctl(fd, BACKLIGHT_START, NULL);
	} else
		ioctl(fd, BACKLIGHT_STOP, NULL);

	ioctl(fd, BACKLIGHT_GET_INFO, &info);
	printf("backlight_test -b %u -f %u -p %u -s %u\n", info.brightness_value, info.pwm_frequency, info.pwm_polarity ,stop_flag);
	printf("default_brightness_level %u levels_count %u \n", info.default_brightness_level, info.levels_count);
	if(info.levels_count)
	{
		printf("levels buff:");
		for(i = 0; i < BACKLIGHT_LEVEL_SIZE; i++)
			printf("%x ", info.levels[i]);
		printf("\n");
	}
	close(fd);

	return 0;
}

CONSOLE_CMD(lcd_test, NULL, lcd_test_enter, CONSOLE_CMD_MODE_SELF, "enter lcddev test")
CONSOLE_CMD(init, "lcd_test", lcd_init_all_cb, CONSOLE_CMD_MODE_SELF, "lcddev init all, eg: init")
CONSOLE_CMD(init_drv, "lcd_test", lcd_init_driver_cb, CONSOLE_CMD_MODE_SELF, "lcddev driver init, eg: init_drv")
CONSOLE_CMD(rotate, "lcd_test", lcd_rotate_cb, CONSOLE_CMD_MODE_SELF, "lcddev set rotate, eg: rotate 0")
CONSOLE_CMD(cmds_data, "lcd_test", lcd_cmds_data_cb, CONSOLE_CMD_MODE_SELF, "lcddev send cmds and data, eg: cmds_data 0x2000 0x0000")
CONSOLE_CMD(send_data, "lcd_test", lcd_send_data_cb, CONSOLE_CMD_MODE_SELF, "lcddev send data, eg: send_data 0x1000")
CONSOLE_CMD(onoff, "lcd_test", lcd_onoff_cb, CONSOLE_CMD_MODE_SELF, "lcd onoff, eg: onoff 0")
CONSOLE_CMD(vcom, "lcd_test", lcd_pwm_vcom_cb, CONSOLE_CMD_MODE_SELF, "lcd pwm vcom, eg: vcom 100")
CONSOLE_CMD(power, "lcd_test", lcd_gpio_power_cb, CONSOLE_CMD_MODE_SELF, "lcd set gpio power, eg: power 1")
CONSOLE_CMD(reset, "lcd_test", lcd_reset_cb, CONSOLE_CMD_MODE_SELF, "lcd set gpio power, eg: reset 1")
CONSOLE_CMD(lvds_gpio, "lcd_test", lvds_gpio_test, CONSOLE_CMD_MODE_SELF, "lvds_gpio_test, eg: lvds_gpio 130 1")
CONSOLE_CMD(gpio, "lcd_test", gpio_set_out, CONSOLE_CMD_MODE_SELF, "gpio_set_out, eg: gpio 99 1")
CONSOLE_CMD(pinmode, "lcd_test", gpio_set_pinpad_function, CONSOLE_CMD_MODE_SELF, "gpio set pinpad function, eg: pinmode 99 0")
CONSOLE_CMD(spi_info, "lcd_test", lcd_set_spi_info_cb, CONSOLE_CMD_MODE_SELF, "spi set info, eg: spi_pinpad -k 0 -o 0 -i 0 -c 7 -M 0 -B 9 -D 256 -C 0")
CONSOLE_CMD(i2c_info, "lcd_test", lcd_set_i2c_info_cb, CONSOLE_CMD_MODE_SELF, "i2c set info, eg: i2c_info -k 0 -o 0 -D 256 -C 0 -A 255")
CONSOLE_CMD(pinpad_info, "lcd_test", lcd_set_pinpad_info_cb, CONSOLE_CMD_MODE_SELF, "pinpad_info -o 1 -r 2 -s 3 -p 4 -P 5")
CONSOLE_CMD(sequence, "lcd_test", lcd_panel_set_cb, CONSOLE_CMD_MODE_SELF, "eg: sequence 02 02 11 00")
CONSOLE_CMD(backlight_test, "lcd_test", lcd_backlight_test, CONSOLE_CMD_MODE_SELF, "eg: backlight_test 100")
CONSOLE_CMD(chmode, "lcd_test", lcd_chmod_pin_mode_cb, CONSOLE_CMD_MODE_SELF, "gpio chmod pin mode, eg: chmode 99 3")
