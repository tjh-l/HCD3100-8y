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
#include "wavreader.h"

static void usage(const char *name)
{
	printf("Usage: %s path-to-wav.wav\n", name);
}

static int i2so_test(int argc, char *argv[])
{
	struct snd_pcm_params params = { 0 };
	const char *infile;
	uint8_t* inputBuf;
	void *wav;
	int format, sampleRate, channels, bitsPerSample;
	int inputSize;
	int fd, ret;
	struct pollfd pfd;
	int read;

	if (argc != 2)
		usage(argv[0]);
	infile = argv[1];

	wav = wav_read_open(infile);
	if (!wav) {
		usage(argv[0]);
		return 1;
	}

	if (!wav_get_header(wav, &format, &channels, &sampleRate, &bitsPerSample, NULL)) {
		printf("Bad wav file %s\n", infile);
		return 1;
	}
	if (format != 1) {
		printf("Unsupported WAV format %d\n", format);
		return 1;
	}

	inputSize = channels*2*160;
	inputBuf = (uint8_t*) malloc(inputSize);

	fd = open("/dev/sndC0i2so", O_WRONLY);

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = fd;
	pfd.events = POLLOUT | POLLWRNORM;

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

	while (1) {
		read = wav_read_data(wav, inputBuf, inputSize);
		if (read < inputSize) {
			break;
			wav_read_close(wav);
			wav = wav_read_open(infile);
			continue;
		}

		do {
			struct snd_xfer xfer = {0};
			xfer.data = inputBuf;
			xfer.frames = (read/channels) >> 1;
			xfer.tstamp_ms = 0;
			ret = ioctl(fd, SND_IOCTL_XFER, &xfer);
			if (ret < 0) {
				poll(&pfd, 1, 100);
			}
		} while (ret < 0);
	}

#if 1
	do {
		snd_pcm_uframes_t delay = 0;
		ioctl(fd, SND_IOCTL_DELAY, &delay);
		if ((int)delay * ((channels * bitsPerSample) >> 3) > (int)params.period_size) {
			printf ("delay %ld\n", delay);
			usleep(100000);
		} else {
			break;
		}
	} while(1);
#else
	ioctl(fd, SND_IOCTL_DRAIN, 0);
#endif

	ioctl(fd, SND_IOCTL_DROP, 0);
	ioctl(fd, SND_IOCTL_HW_FREE, 0);

	close(fd);
	wav_read_close(wav);
	free(inputBuf);

	return 0;
}

CONSOLE_CMD(i2so, "snd", i2so_test, CONSOLE_CMD_MODE_SELF,
    "do i2so test")


//////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
/* do i2s record and play at sametime */
static int i2so_fd = -1;
static int i2si_fd = -1;
static uint8_t *buf = NULL;
static int stop_i2s_record_play = 0;
static int record_play_rate = 48000;

static int i2sin_pcm_dest = SND_PCM_DEST_DMA;

int i2so_open(int rate, int channels, int bitdepth, int format, int poll_size)
{
	struct snd_pcm_params params = {0};
	int i2so_fd;

	i2so_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (i2so_fd < 0) {
		printf("open i2so failed\n");
		return i2so_fd;
	}
	printf ("i2so_fd fd %d\n", i2so_fd);

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.rate = rate;
	params.channels = channels;
	params.bitdepth = bitdepth;
	params.format = format;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = SND_PCM_ALIGN_RIGHT;
	params.period_size = 2048;
	params.periods = 16;
	params.start_threshold = 1;
	//params.audio_flush_thres = 50;
	ioctl(i2so_fd, SND_IOCTL_HW_PARAMS, &params);

	ioctl(i2so_fd, SND_IOCTL_AVAIL_MIN, &poll_size);
	printf ("i2so_fd SND_IOCTL_AVAIL_MIN\n");
	ioctl(i2so_fd, SND_IOCTL_START, 0);
	printf ("i2so_fd SND_IOCTL_START\n");
	int volume = 10;
	ioctl(i2so_fd, SND_IOCTL_SET_VOLUME, &volume);

	return i2so_fd;
}

int i2si_open(int rate, int channels, int bitdepth, int format, int poll_size)
{
	struct snd_pcm_params params = {0};
	int i2si_fd;

	i2si_fd = open("/dev/sndC0i2si", O_WRONLY);
	printf ("i2si fd %d\n", i2si_fd);

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = format;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = SND_PCM_ALIGN_RIGHT;
	params.rate = rate;
	params.channels = channels;
	params.period_size = 2048;
	params.periods = 16;
	params.bitdepth = bitdepth;
	params.start_threshold = 1;
	params.pcm_source = SND_PCM_SOURCE_AUDPAD;
	if (i2sin_pcm_dest) {
		params.pcm_dest = SND_PCM_DEST_BYPASS;
	} else {
		params.pcm_dest = SND_PCM_DEST_DMA;
	}

	ioctl(i2si_fd, SND_IOCTL_HW_PARAMS, &params);
	ioctl(i2si_fd, SND_IOCTL_AVAIL_MIN, &poll_size);
	printf ("i2si SND_IOCTL_AVAIL_MIN\n");
	ioctl(i2si_fd, SND_IOCTL_START, 0);
	printf ("i2si SND_IOCTL_START\n");
	return i2si_fd;
}

static void i2s_record_play_thread(void *arg)
{

	int channels = 2;
	int bitdepth = 32;
	int format = SND_PCM_FORMAT_S24_LE;
	int read_size = 2048;

	printf("i2si_rec_play_start: rate %d\n",record_play_rate);
	if (i2so_fd == -1) {
		i2so_fd = i2so_open(record_play_rate, channels, bitdepth, format, read_size/(channels * bitdepth / 8));
	}
	if (i2si_fd == -1) {
		i2si_fd = i2si_open(record_play_rate, channels, bitdepth, format, read_size/(channels * bitdepth / 8));
	}
	if(!i2sin_pcm_dest) {
		struct snd_xfer xfer = {0};
		buf = malloc(read_size);
		int ret;
		struct pollfd pfdin;
		struct pollfd pfdout;

		memset(&pfdin, 0, sizeof(struct pollfd));
		pfdin.fd = i2si_fd;
		pfdin.events = POLLIN | POLLRDNORM;

		memset(&pfdout, 0, sizeof(struct pollfd));
		pfdout.fd = i2so_fd;
		pfdout.events = POLLOUT | POLLWRNORM;

		xfer.data = buf;
		while(1) {
			if (stop_i2s_record_play) {
				printf("stop i2s record play\n");
				break;
			}
			xfer.frames = read_size / (channels * bitdepth / 8);
			ret = ioctl(i2si_fd, SND_IOCTL_XFER, &xfer);
			if (ret < 0) {
				poll(&pfdin, 1, 50);
				continue;
			}
			//printf("```0x%x, 0x%x, 0x%x, 0x%x```\n", buf[0], buf[1], buf[2], buf[3]);
			ret = ioctl(i2so_fd, SND_IOCTL_XFER, &xfer);
			if (ret < 0) {
				poll(&pfdout, 1, 50);
				continue;
			}
		}
	}
	ioctl(i2so_fd, SND_IOCTL_DROP, 0);
	ioctl(i2so_fd, SND_IOCTL_HW_FREE, 0);
	ioctl(i2si_fd, SND_IOCTL_DROP, 0);
	ioctl(i2si_fd, SND_IOCTL_HW_FREE, 0);
	if (buf) {
		free(buf);
		buf = NULL;
	}

	if (i2so_fd) {
		close(i2so_fd);
		i2so_fd = -1;
	}
	if (i2si_fd) {
		close(i2si_fd);
		i2si_fd = -1;
	}
	vTaskDelete(NULL);
}

static int i2s_record_play_test(int argc, char *argv[])
{
	int ret = 0;
	i2sin_pcm_dest = SND_PCM_DEST_DMA;
	record_play_rate = atoi(argv[1]);
	if (record_play_rate == 0) {
		record_play_rate = 48000;
	}
	stop_i2s_record_play = 0;
	ret = xTaskCreate(i2s_record_play_thread, "i2s_record_play",
		0x2000, NULL, portPRI_TASK_HIGH, NULL);
	if(ret != pdTRUE) {
		return -1;
	}
	return 0;
}

static int stop_i2s_record_play_dma(int argc, char *argv[])
{

	printf("stop_i2s_record_play_dma\n");
	stop_i2s_record_play = 1;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/* i2s record and play use SND_PCM_DEST_DMA mode */

CONSOLE_CMD(i2s_record_play_dma, "snd", i2s_record_play_test, CONSOLE_CMD_MODE_SELF, "i2s_record_play 48000")
CONSOLE_CMD(stop_i2s_record_play_dma, "snd", stop_i2s_record_play_dma, CONSOLE_CMD_MODE_SELF, "stop_i2s_record_play")

/////////////////////////////////////////////////////////////
/* i2s record and play use SND_PCM_DEST_BYPASS mode */
static int i2s_record_play_bypass(int argc, char *argv[])
{

	int channels = 2;
	int bitdepth = 32;
	int format = SND_PCM_FORMAT_S24_LE;
	int read_size = 2048;

	record_play_rate = atoi(argv[1]);
	if(record_play_rate == 0) {
		record_play_rate = 48000;
		printf("is record_play_rate 48000?\n");
	}
	i2sin_pcm_dest = SND_PCM_DEST_BYPASS;
	if (i2so_fd == -1) {
		i2so_fd = i2so_open(record_play_rate, channels, bitdepth, format, read_size/(channels * bitdepth / 8));
	}
	if (i2si_fd == -1) {
		i2si_fd = i2si_open(record_play_rate, channels, bitdepth, format, read_size/(channels * bitdepth / 8));
	}
	return 0;
}

static int stop_i2s_record_play_bypass(int argc, char *argv[])
{

	printf("stop_i2s_record_play_bypass\n");
	close(i2so_fd);
	i2so_fd = -1;
	close(i2si_fd);
	i2si_fd = -1;
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////
CONSOLE_CMD(i2s_record_play_bypass, "snd", i2s_record_play_bypass, CONSOLE_CMD_MODE_SELF, "i2s_record_play_bypass 48000")
CONSOLE_CMD(stop_i2s_record_play_bypass, "snd", stop_i2s_record_play_bypass, CONSOLE_CMD_MODE_SELF, "stop_i2s_record_play_bypass")

///////////////////////////////////////////////////////////////////////////
/* i2s record to udisk	*/
static int stop_i2s_record = 0;
static FILE *record_file = NULL;
static int record_samplerate = 0;

static void i2s_record_thread(void *arg)
{

	struct snd_pcm_params params = {0};
	int i,j = 0;
	int total_read_times = 1000;
	int read_size = 1536;
	int read_times = 0;
	char *buf = NULL;
	int ret = 0;
	struct pollfd pfd;
	int snd_in_fd = 0;
	struct snd_xfer xfer = {0};

	buf = malloc (read_size);
	memset(buf, 0, read_size);
	snd_in_fd = open("/dev/sndC0i2si", O_WRONLY);
	printf ("i2si fd %d\n", snd_in_fd);
	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = snd_in_fd;
	pfd.events = POLLIN | POLLRDNORM;

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S24_LE;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = SND_PCM_ALIGN_RIGHT;
	params.rate = record_samplerate;
	params.channels = 2;
	params.period_size = 1536;
	params.periods = 16;
	params.bitdepth = 32;
	params.start_threshold = 1;
	params.pcm_source = SND_PCM_SOURCE_AUDPAD;

	ret = ioctl(snd_in_fd, SND_IOCTL_HW_PARAMS, &params);
	if (ret < 0) {
		printf("SND_IOCTL_HW_PARAMS error \n");
	}
	printf ("start record %d\n", ret);
	ret = ioctl(snd_in_fd, SND_IOCTL_START, 0);

	xfer.data = buf;
	xfer.frames = 1536 / ((params.channels) * (params.bitdepth) / 8);
	do {
		ret = ioctl(snd_in_fd, SND_IOCTL_XFER, &xfer);
		if (ret < 0) {
			poll(&pfd, 1, 100);
			continue;
		}
		fwrite (xfer.data, 1536, 1, record_file);
		//printf ("fwrite period_size %d\n",period_size);
		read_times++;
		if(stop_i2s_record || read_times >= 2000)
			break;
	} while (1);
	printf ("i2s record done\n");
	ioctl(snd_in_fd, SND_IOCTL_DROP, 0);
	ioctl(snd_in_fd, SND_IOCTL_HW_FREE, 0);
	close(snd_in_fd);
	if (record_file) {
		fclose(record_file);
		record_file = NULL;
		printf("fclose file \n");
	}
	if (buf)
		free(buf);
	vTaskDelete(NULL);
}

static int i2s_record(int argc, char *argv[])
{
	int ret;
	record_file = fopen(argv[1], "w+");
	record_samplerate = atoi(argv[2]);
	if (record_samplerate == 0) {
		record_samplerate = 48000;
		printf("record_samplerate is 48000\n");
	}
	stop_i2s_record = 0;
	if (!record_file) {
		printf("i2s_rec_start,fopen fail\n");
		return -1;
	} else {
		printf("i2s_rec_start,fopen %s\n",argv[1]);
	}
	ret = xTaskCreate(i2s_record_thread, "i2s_record_thread",
		0x2000, NULL, portPRI_TASK_HIGH, NULL);
	if(ret != pdTRUE) {
		return -1;
	}
	return 0;
}

static int stop_i2s_record_test(int argc, char *argv[])
{
	stop_i2s_record = 1;
	return 0;
}

CONSOLE_CMD(i2s_record, "snd", i2s_record, CONSOLE_CMD_MODE_SELF, "i2s_record filepath 48000")
CONSOLE_CMD(stop_i2s_record, "snd", stop_i2s_record_test, CONSOLE_CMD_MODE_SELF, "stop_i2s_record")

//////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
/* i2so play udisk files  */
FILE *rec_file = NULL;
static int file_sampleRate = 0;

static int stop_i2so_play = 0;
static void play_udisk_thread(void *arg)
{
	struct snd_pcm_params params = { 0 };
	uint8_t* inputBuf;
	int inputSize;
	int fd, ret;
	struct pollfd pfd;
	int read;
	stop_i2so_play = 0;
	int channels = 2;

	inputSize = 1536;
	inputBuf = (uint8_t*) malloc(inputSize);

	fd = open("/dev/sndC0i2so", O_WRONLY);

	memset(&pfd, 0, sizeof(struct pollfd));

	pfd.fd = fd;
	pfd.events = POLLOUT | POLLWRNORM;

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S24_LE;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = SND_PCM_ALIGN_RIGHT;
	params.rate = file_sampleRate;
	params.channels = channels;
	params.period_size = 1536;
	params.periods = 8;
	params.bitdepth = 32;
	params.start_threshold = 1;

	ret = ioctl(fd, SND_IOCTL_HW_PARAMS, &params);
	if (ret < 0) {
		printf("snd hw param failed\n");
		return;
	} else {
		printf("snd hw param success\n");
	}

	ret = ioctl(fd, SND_IOCTL_START, 0);
	if (ret < 0) {
		printf("snd start failed\n");
		return;
	} else {
		printf("snd start success\n");
	}
	int volume = 100;
	ioctl(fd, SND_IOCTL_SET_VOLUME, &volume);
	struct snd_xfer xfer = {0};
	while (1) {
		if(stop_i2so_play) {
			break;
		}
		ret = fread(inputBuf, 1, inputSize, rec_file);
		//printf("ret = %d\n", ret);
		if (ret <= 0)
			break;
		xfer.data = inputBuf;
		xfer.frames = (ret / (channels*(params.bitdepth/8))) ;
		xfer.tstamp_ms = 0;
		while (ioctl(fd, SND_IOCTL_XFER, &xfer) < 0) {
			poll(&pfd, 1, 100);
			continue;
		}
	}

	printf("drain\n");
	ioctl(fd, SND_IOCTL_DRAIN, 0);

	printf("stop\n");
	ioctl(fd, SND_IOCTL_DROP, 0);
	ioctl(fd, SND_IOCTL_HW_FREE, 0);
	close(fd);

	if(inputBuf)
		free(inputBuf);
	if (rec_file) {
		printf("close file\n");
		fclose(rec_file);
		rec_file = NULL;
	}
	vTaskDelete(NULL);
	return;
}

static int i2so_play_udisk(int argc, char *argv[])
{

	int ret = 0;
	rec_file = fopen(argv[1], "rb");
	if (!rec_file)
	{
		printf("fopen file fail,please check the file\n");
		return -1;
	}
	file_sampleRate = atoi(argv[2]);
	if (file_sampleRate == 0) {
		file_sampleRate = 48000;
		printf("samplerate is 48000,please check the samplerate\n");
	}
	ret = xTaskCreate(play_udisk_thread, "i2so_play_thread",
		0x2000, NULL, portPRI_TASK_HIGH, NULL);
	if(ret != pdTRUE) {
		return -1;
	}
	return 0;
}

static int stop_i2so_play_udisk(int argc, char *argv[])
{
	stop_i2so_play = 1;
	return 0;
}
CONSOLE_CMD(i2so_play_udisk, "snd", i2so_play_udisk, CONSOLE_CMD_MODE_SELF,"i2so_play_udisk filepath,48000")
CONSOLE_CMD(stop_i2so_play_udisk, "snd", stop_i2so_play_udisk, CONSOLE_CMD_MODE_SELF, "stop_i2so_play_udisk")


static int i2so_spdif_switch(bool i2so_inactive)// 1:i2so inactive,spdif active; 0:spdif inactive,i2so active
{
	int snd_i2so_fd = -1;
	int snd_spdif_fd = -1;

	snd_i2so_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_i2so_fd < 0) {
		printf ("open i2so snd_fd %d failed\n", snd_i2so_fd);
		return snd_i2so_fd;
	}

	snd_spdif_fd = open("/dev/sndC0spo", O_WRONLY);
	if (snd_spdif_fd < 0) {
		printf ("open spdif snd_fd %d failed\n", snd_spdif_fd);
		if(snd_i2so_fd >= 0)
			close(snd_i2so_fd);
		return snd_spdif_fd;
	}


	if(i2so_inactive) {
		printf("i2so_inactive\n");
		ioctl(snd_i2so_fd, SND_IOCTL_SET_PINMUX_DATA_INACTIVE, 1);
		ioctl(snd_spdif_fd, SND_IOCTL_SET_PINMUX_DATA_INACTIVE, 0);
	} else {
		printf("i2so_active\n");
		ioctl(snd_spdif_fd, SND_IOCTL_SET_PINMUX_DATA_INACTIVE, 1);
		ioctl(snd_i2so_fd, SND_IOCTL_SET_PINMUX_DATA_INACTIVE, 0);
	}


	close(snd_spdif_fd);
	close(snd_i2so_fd);
	return 0;
}

static int i2so_spdif_switch_test(int argc, char *argv[])
{
	i2so_spdif_switch(atoi(argv[1]));
	return 0;
}
CONSOLE_CMD(i2so_spdif, "snd", i2so_spdif_switch_test, CONSOLE_CMD_MODE_SELF,"i2so_spdif 0")
