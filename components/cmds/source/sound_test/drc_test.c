#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <hcuapi/auddec.h>
#include <hcuapi/snd.h>
#include <kernel/lib/console.h>

static int drc(int argc , char *argv[])
{
	int opt;
	int fd;
	struct snd_drc_setting setting = { 0 };

	fd = open("/dev/sndC0i2so", O_WRONLY);
	if (fd < 0) {
		printf ("open snd_fd %d failed\n", fd);
		return -1;
	}

	//ioctl(fd, SND_IOCTL_GET_DRC_PARAM, &setting);
	while ((opt = getopt(argc, argv, "l:g:")) != -1) {
		switch (opt) {
		case 'l':
			setting.peak_dBFS = atof(optarg);
			if (setting.peak_dBFS > 0) {
				printf("bad level limit: %.4f\n", setting.peak_dBFS);
				return -1;
			}
			break;
		case 'g':
			setting.gain_dBFS = atof(optarg);
			break;
		}
	}

	printf("gain_dBFS %f\r\n", setting.gain_dBFS);
	printf("level_limit %f\r\n", setting.peak_dBFS);
	ioctl(fd, SND_IOCTL_SET_DRC_PARAM, &setting);
	close(fd);
	return 0;
}

static int drc_limiter(int argc , char *argv[])
{
	int opt;
	int fd;
	struct snd_limiter_setting limiter_setting = { 0 };

	fd = open("/dev/sndC0i2so", O_WRONLY);
	if (fd < 0) {
		printf ("limiter open snd_fd %d failed\n", fd);
		return -1;
	}

	while ((opt = getopt(argc, argv, "l:g:")) != -1) {
		switch (opt) {
		case 'l':
			limiter_setting.target_dBFS = atof(optarg);
			if (limiter_setting.target_dBFS > 0) {
				printf("limiter bad level limit: %.4f\n", limiter_setting.target_dBFS);
				return -1;
			}
			break;
		case 'g':
			limiter_setting.threshold_dBFS = atof(optarg);
			break;
		}
	}

	printf("limiter gain_dBFS %f\r\n", limiter_setting.threshold_dBFS);
	printf("limiter level_limit %f\r\n", limiter_setting.target_dBFS);
	ioctl(fd, SND_IOCTL_SET_LIMITER_PARAM, &limiter_setting);
	close(fd);
	return 0;
}

CONSOLE_CMD(drc, "snd", drc, CONSOLE_CMD_MODE_SELF, "set drc compressor gain/peak, usage drc -l 0 -g 12")
CONSOLE_CMD(drc_limiter, "snd", drc_limiter, CONSOLE_CMD_MODE_SELF, "set drc limiter threshold / target, usage drc_limiter -l -6 -g -6")
