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
#include <hcuapi/audsink.h>
#include <pthread.h>


static FILE *wav_file = NULL;
static int stop_wav = 0;
static pthread_t audsink_thread_id  = 0; 
static int period_size = 1536;
static int wav_audsink_init(int channels, int bitdepth, int rate)
{
	if ((bitdepth == 0) || (channels == 0) || (rate == 0)) {
		printf("wav_audsink_init fail: bitdepth %d, channels %d, rate %d\n",bitdepth,channels,rate);
		return -1;
	}
    
	struct audsink_init_params asink_params = {0};
	asink_params.buf_size = channels * rate * bitdepth / 8;
	asink_params.snd_devs = AUDSINK_SND_DEVBIT_I2SO;    
	asink_params.sync_type = 0;
	asink_params.audio_flush_thres = 0;
	asink_params.pcm.sync_mode = AVSYNC_TYPE_FREERUN;
	asink_params.pcm.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	asink_params.pcm.format = SND_PCM_FORMAT_S16_LE;
	asink_params.pcm.align = SND_PCM_ALIGN_RIGHT;
	asink_params.pcm.rate = rate;
	asink_params.pcm.channels = channels;
	asink_params.pcm.bitdepth = bitdepth;
	asink_params.pcm.period_size = period_size;
	asink_params.pcm.periods = (rate / 2) / (asink_params.pcm.period_size);//0.5s
	asink_params.pcm.start_threshold = rate / 20;//50 ms (start_threshold unit is sample)

	int fd = open("/dev/audsink" , O_RDWR);
	if (fd < 0) {
		printf("Open /dev/audsink error.\n");
		return -1;
	}

	if (ioctl(fd , AUDSINK_IOCTL_INIT , &asink_params) != 0) {
		printf("Init audsink error\n");
		close(fd); 
		return -1;
	}
	return fd;
}

static void *audsink_play_thread(void *arg)
{

	int ret = 0;
	void *wav;
	int format, sampleRate, channels, bitsPerSample;
	uint32_t data_size = 0;
	char *wav_name = (char *)arg;
	wav = wav_read_open(wav_name);

	if (!wav) {
		printf("wav_read_open %s fail\n", wav_name);
		return NULL;
	}

	if (!wav_get_header(wav, &format, &channels, &sampleRate, &bitsPerSample, NULL)) {
		printf("wav_get_header %s fail\n", wav_name);
		return NULL;
	}
	if(format !=1) {
		printf("Bad wav file %s,format %d\n", wav_name,format);
	}
	wav_read_close(wav);

	int fd = wav_audsink_init(channels, bitsPerSample, sampleRate);
	struct audsink_xfer xfer = { 0 };
    struct pollfd pfd = { 0 };
    pfd.fd = fd;
    pfd.events = POLLOUT | POLLWRNORM; 
	uint32_t inputSize = period_size;
	uint8_t *inputBuf = (uint8_t*) malloc(inputSize);

	fseek(wav_file, 40, SEEK_SET);
	fread(&data_size, 1, 4, wav_file);

	if (data_size == 0)
		data_size = 0xffffffff;

	while (data_size > 0 && (!stop_wav)) {

		ret = fread(inputBuf, 1, (inputSize > data_size) ? data_size : inputSize, wav_file);
		if (ret <= 0) {
			break;
		}
		data_size -= ret;
		xfer.data = inputBuf;
		xfer.frames = (ret / (channels * (bitsPerSample/8))) ;
		xfer.pitch = (ret / (channels * (bitsPerSample/8))) ;
		xfer.tstamp_ms = -1;
		ret = ioctl(fd , AUDSINK_IOCTL_XFER , &xfer);
		while (ret < 0 && (!stop_wav)) {
			poll(&pfd, 1, 100);
			continue;
		}
	} 

	do {
		snd_pcm_uframes_t delay = 0;
		ioctl(fd, AUDSINK_IOCTL_DELAY, &delay);
		if((int)delay < (period_size / (channels * bitsPerSample / 8))) {
			break;
		} else {
			usleep(30*1000);
		}
	} while(1);

	close(fd);

	if(inputBuf)
		free(inputBuf);

	if (wav_file) {
		printf("close file\n");
		fclose(wav_file);
		wav_file = NULL;
	}
	free(arg);
	return NULL;
}

static int audsink_play_start(int argc, char *argv[])
{

	int ret = 0;
	stop_wav = 0;
	wav_file = fopen(argv[1], "rb");
	if (!wav_file)
	{
		printf("fopen file fail,please check the file\n");
		return -1;
	}
	ret = pthread_create(&audsink_thread_id, NULL, audsink_play_thread, strdup(argv[1]));
	if(ret) {
		return -1;
	}

	return 0;
}

static int audsink_play_stop(int argc, char *argv[])
{
	stop_wav = 1;
	if(audsink_thread_id)
		pthread_join(audsink_thread_id, NULL);
	audsink_thread_id = 0;
	printf("stop\n");
	return 0;
}

CONSOLE_CMD(audsink_play_start, "snd", audsink_play_start, CONSOLE_CMD_MODE_SELF,"audsink_play_start filepath")
CONSOLE_CMD(audsink_play_stop, "snd", audsink_play_stop, CONSOLE_CMD_MODE_SELF, "audsink_play_stop")
