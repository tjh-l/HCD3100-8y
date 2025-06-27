#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <kernel/lib/console.h>
#include <hcuapi/snd.h>
#include <hcuapi/avsync.h>

#include "sine_pcm.h"

static void usage(const char *name)
{
	printf("Usage: %s path-to-wav.wav\n", name);
}

static int i2so_test(int argc, char *argv[])
{
	struct snd_pcm_params params = { 0 };
	const char *infile;
	uint8_t* inputBuf;
	int format, sampleRate, channels, bitsPerSample;
	int inputSize;
	int fd, ret;
	struct pollfd pfd;

    channels = 1;
    sampleRate = 8000;
    bitsPerSample = 16;

	inputSize = channels*2*160;
	inputBuf = (uint8_t*) malloc(inputSize);

	fd = open("/dev/sndC0i2so", O_WRONLY);

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S16_LE;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = SND_PCM_ALIGN_LEFT;
	params.rate = sampleRate;
	params.channels = channels;
	params.period_size = inputSize;
	params.periods = 8;
	params.bitdepth = bitsPerSample;
	params.start_threshold = 2;

	ret = ioctl(fd, SND_IOCTL_HW_PARAMS, &params);
	if (ret < 0) {
		printf("snd hw param failed\n");
		return 1;
	} else {
		printf("snd hw param success\n");
	}

	ret = ioctl(fd, SND_IOCTL_START, 0);
	if (ret < 0) {
		printf("snd start failed\n");
		return 1;
	} else {
		printf("snd start success\n");
	}

    uint8_t *cur = &sine_pcm[0];
    int rest = sine_pcm_len;
    int xfers = 360;

	while (1) {
		do {
			struct snd_xfer xfer = {0};
			xfer.data = cur;
			xfer.frames = xfers;
			xfer.tstamp_ms = 0;
			ret = ioctl(fd, SND_IOCTL_XFER, &xfer);
			if (ret < 0) {
				poll(&pfd, 1, 100);
			}
		} while (ret < 0);

        if(rest < xfers) {
            cur = &sine_pcm[0];
            rest = sine_pcm_len;
        }else {
            cur += xfers;
            rest -= xfers;
        }
	}

	ioctl(fd, SND_IOCTL_DROP, 0);
	ioctl(fd, SND_IOCTL_HW_FREE, 0);

	close(fd);
	free(inputBuf);

	return 0;
}

CONSOLE_CMD(lock_speaker, NULL, i2so_test, CONSOLE_CMD_MODE_SELF,
        "testing speaker for doorlock")
