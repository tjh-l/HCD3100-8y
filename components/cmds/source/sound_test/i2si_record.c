#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <hcuapi/auddec.h>
#include <hcuapi/snd.h>
#include <hcuapi/common.h>
#include <hcuapi/avsync.h>
#include <kernel/lib/console.h>
#include <kernel/delay.h>
#include <kernel/io.h>

static void print_usage(const char *prog)
{
	printf("Usage: %s [-crfosih]\n", prog);
	puts("  -c --channel  record channels, default [2]\n"
	     "  -r --rate     sample rate(Hz), default [16000]\n"
	     "  -f --format   refer to SND_PCM_FORMAT_* in hcuapi/snd.h, eg 6 is for SND_PCM_FORMAT_S24_LE, and 2 is for SND_PCM_FORMAT_S16_LE, default [2]\n"
	     "  -o --output   record content to output, mem or file, default [/dev/null]\n"
	     "  -s --size     record content size if output is to mem, default 5M\n"
	     "  -i --source   record from the source, default [/dev/sndC0i2si]\n"
	     "  -h --help     print help usage\n");
}

static int i2si_record(int argc, char *argv[])
{
	struct snd_pcm_params params = { 0 };
	const char *output = "/dev/null";
	const char *input = "/dev/sndC0i2si";
	char *tmp = NULL;
	char *buf = NULL;
	ssize_t buf_sz = 0x500000;
	ssize_t buf_copied = 0;
	struct pollfd pfd;
	int fd = -1;
	FILE *fout = NULL;
	int rc = 0;
	struct snd_xfer xfer = {0};
	useconds_t usec_sleep = 0;

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S16_LE;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = SND_PCM_ALIGN_RIGHT;
	params.rate = 16000;
	params.channels = 2;
	params.period_size = 1536;
	params.periods = 16;
	params.bitdepth = 16;
	params.start_threshold = 1;
	params.pcm_source = SND_PCM_SOURCE_AUDPAD;
	params.pcm_dest = SND_PCM_DEST_DMA;

	while (1) {
		static const struct option lopts[] = {
			{ "channel", 1, 0, 'c' },
			{ "rate",    1, 0, 'r' },
			{ "fmt",     1, 0, 'f' },
			{ "output",  1, 0, 'o' },
			{ "size",    1, 0, 's' },
			{ "source",  1, 0, 'i' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "c:r:f:o:s:i:h", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'c':
			params.channels = atoi(optarg);
			break;
		case 'r':
			params.rate = atoi(optarg);
			break;
		case 'f':
			params.format = atoi(optarg);
			if (params.format <= 1)
				params.bitdepth = 8;
			else if (params.format >= 2 && params.format <= 5)
				params.bitdepth = 16;
			else if (params.format >= 6 && params.format <= 13)
				params.bitdepth = 32;
			break;
		case 'o':
			output = optarg;
			break;
		case 's':
			buf_sz = atoi(optarg);
			break;
		case 'i':
			input = optarg;
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			return 1;
		}
	}

	fd = open(input, O_WRONLY);
	if (fd < 0) {
		printf("open %s failed\r\n", input);
		rc = 1;
		goto err_ret;
	}

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = fd;
	pfd.events = POLLIN | POLLRDNORM;

	tmp = malloc(params.period_size);
	if (!tmp) {
		rc = -1;
		goto err_ret;
	}

	if (!strcmp(output, "mem")) {
		buf = malloc(buf_sz);
		if (!buf) {
			rc = -1;
			goto err_ret;
		}
	} else {
		fout = fopen(output, "wb");
		if (!fout) {
			rc = -1;
			goto err_ret;
		}
	}

	rc = ioctl(fd, SND_IOCTL_HW_PARAMS, &params);
	if (rc < 0) {
		printf("SND_IOCTL_HW_PARAMS error\n");
		goto err_ret;
	}

	rc = ioctl(fd, SND_IOCTL_START, 0);
	if (rc < 0) {
		printf("SND_IOCTL_START error\n");
		goto err_ret;
	}

	xfer.data = tmp;
	xfer.frames = params.period_size / (params.channels * params.bitdepth / 8);
	usec_sleep = xfer.frames * 1000 / params.rate;
	usec_sleep *= 1000;
	while (1) {
		rc = ioctl(fd, SND_IOCTL_XFER, &xfer);
		if (rc < 0) {
			usleep(usec_sleep);
			continue;
		}
		printf("record %ld\r\n", params.period_size);
		if (buf) {
			if ((ssize_t)(buf_copied + params.period_size) > buf_sz)
				break;
			memcpy(buf + buf_copied, xfer.data, params.period_size);
			buf_copied += params.period_size;
		} else if (fout) {
			if (buf_sz > 0 && (ssize_t)(buf_copied + params.period_size) > buf_sz)
				break;
			fwrite(xfer.data, params.period_size, 1, fout);
			buf_copied += params.period_size;
		}
	}

	ioctl(fd, SND_IOCTL_DROP, 0);
	ioctl(fd, SND_IOCTL_HW_FREE, 0);

	rc = 0;

err_ret:
	if (tmp)
		free(tmp);
	if (fd >= 0)
		close(fd);
	if (fout)
		fclose(fout);
	if (buf) {
		printf("record finished, stop here, mem=0x%08lx, size=%ld\r\n", (unsigned long)buf, buf_copied);
		REG32_CLR_BIT(0xb8800094, BIT16);
		while (1);
		free(buf);
	}

	return rc;
}

CONSOLE_CMD(i2si_rec, "snd", i2si_record, CONSOLE_CMD_MODE_SELF, "do i2si test")
