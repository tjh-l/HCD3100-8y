#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <hcuapi/standby.h>
#include <kernel/lib/console.h>
#include <kernel/io.h>
#include <getopt.h>

char ip_table[][32] = {
	"maincpu",
	"hccpu",
	"fxdemv",
	"fxdemg",
	"ve_sh",
	"ve_l",
	"aud",
	"ve_ctrl",
	"ge",
	"usb",
	"SG1",
	"SDIO",
	"TSDMA",
	"DEMUX2",
	"DEMUX1",
	"TSG",
	"DSC",
	"SF",
	"jenc",
	"fxdecg",
	"fxdeca",
	"eth_mac",
	"usb2",
	"vin",
	"nf",
	"SPI",
	"4kdemv",
	"4kdemg",
	"4kdecq",
	"4kdeca",
	"vid_cvb",
	"vid_hdmi",
	"SG2",
};

static void print_usage(const char *prog)
{
	printf("usage: %s [-igasppclh]\n", prog);
	puts("  -t --time	test time [default 5 sec]\n");
}

static int show_meminfo(void)
{
	uint32_t size = 0, type = 0, frequency = 0, ic;

	type = REG32_GET_FIELD2(0xb8801000, 23, 1);
	size = REG32_GET_FIELD2(0xb8801000, 0, 3);
	frequency = REG32_GET_FIELD2(0xb8800070, 4, 3);

	type = type + 2;

	size = 16 << (size);

	switch (frequency) {
	case 0:
		frequency = 800;
		break;
	case 1:
		frequency = 1066;
		break;
	case 2:
		frequency = 1333;
		break;
	case 3:
		frequency = 1600;
		break;
	case 4:
		frequency = 600;
		break;
	default:
		break;
	}

	ic = REG8_READ(0xb8800003);
	if (REG16_GET_BIT(0xb880048a, BIT15)) {
		if (ic == 0x15)
			frequency = REG16_GET_FIELD2(0xb880048a, 0, 15) * 12;
		else if (ic == 0x16)
			frequency = REG16_GET_FIELD2(0xb880048a, 0, 15) * 24;
	}

	return frequency;
}

static int ddr_monitor(int argc, char **argv)
{
	uint32_t t_r[33] = {0};
	uint32_t t_w[33] = {0};
	int i,j, temp, time_2, time = 5;
	int dev_num = sizeof(ip_table) / 32;
	uint32_t ddr_freq  = show_meminfo();
	ddr_freq *= 1000000;
	ddr_freq /= 4096;

	REG32_WRITE(0xb8801940, 0xc0000000);
	REG32_SET_BITS(0xb8801940, ddr_freq << 0, 0x3fffff << 0);
	REG32_WRITE(0xb8801900, 0x0f006000);

	opterr = 0;
	optind = 0;

	while (1) {
		static const struct option lopts[] = {
			{ "time",	1, 0, 't' },
			{ NULL,		0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "t:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 't':
			time = atoi((const char *)optarg);
			break;
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	printf("test time %d seconds\n", time);
	time_2 = time;
	sleep(2);

	while (time--) {
		sleep(1);
		for (i = 0; i < dev_num; i++) {
			REG32_SET_BITS(0xb8801900, (i/4) << 16, 0xff << 16);
			temp = (REG32_READ(0xb8801918 + (4*(i%4))) | REG32_READ(0xb880192c + (4*(i%4))));

			if (temp != 0) {
				printf("%-15sr: %-25ldw: %-25ld\n", ip_table[i], REG32_READ(0xb8801918 + (4*(i%4))), REG32_READ(0xb880192c + (4*(i%4))));
				t_r[i] += REG32_READ(0xb8801918 + (4*(i%4)));
				t_w[i] += REG32_READ(0xb880192c + (4*(i%4)));
			}
		}
		printf("------------------------------------------------------\n");
	}

	REG32_WRITE(0xb8801900, 0x00005000);

	char *s0 = "IP Name";
	char *s1 = "Write BandWith(B/s)";
	char *s2 = "Read BandWith(B/s)";
	printf("\ntest result:\n");
	printf("\t%-15s %-25s %-25s\n", s0, s1, s2);
	for (i = 0; i < dev_num; i++) {
		if (t_r[i] != 0 || t_w[i] != 0)
			printf("\t%-15s %-25ld %-25ld\n", ip_table[i] ,t_r[i]/time_2, t_w[i]/time_2);
	}

	return 0;
}

CONSOLE_CMD(ddr_monitoring, NULL, ddr_monitor, CONSOLE_CMD_MODE_SELF, "DDR usage monitoring")
