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
#include <hcuapi/auddec.h>
#include <hcuapi/common.h>
#include <pthread.h>

static int stop_play = 0;
static pthread_t auddec_thread_id  = 0; 
static int auddec_init(int channels, int bitsPerSample, int sampleRate)
{
	int fd, ret;
	struct audio_config cfg = {0};

	fd = open("/dev/auddec" , O_RDWR);
	if (fd < 0) {
		printf("open auddec err\n");
		return fd;
	}

	cfg.channels = channels;
	cfg.bits_per_coded_sample = bitsPerSample;
	if (cfg.bits_per_coded_sample == 8) {
		cfg.codec_id = HC_AVCODEC_ID_PCM_U8;
	} else if (cfg.bits_per_coded_sample == 16) {
		cfg.codec_id = HC_AVCODEC_ID_PCM_S16LE;
	} else if (cfg.bits_per_coded_sample == 32) {
		cfg.codec_id = HC_AVCODEC_ID_PCM_S32LE;
	} 
	cfg.sample_rate = sampleRate;
	cfg.sync_mode = 0;
	cfg.kshm_size = 0x200000;

	ret = ioctl(fd, AUDDEC_INIT, &cfg);
	if (ret < 0) {
		close(fd);
		return ret;
	}

	ret = ioctl(fd, AUDDEC_START, 0);
	if (ret < 0) {
		close(fd);
		return ret;
	}

	return fd;
}

int auddec_decode(int fd, uint8_t *audio_frame, size_t packet_size)
{
    AvPktHd pkthd = { 0 };
    pkthd.dur = 0;
    pkthd.size = packet_size;
    pkthd.flag = AV_PACKET_ES_DATA;
    pkthd.pts = -1;
    do { 
        if(write(fd, (uint8_t *)&pkthd , sizeof(AvPktHd)) == sizeof(AvPktHd))
            break;
        printf("Write AvPktHd fail\n");
        usleep(1000);
    } while (!stop_play);

    do {
        if(write(fd , audio_frame , packet_size) == (int)packet_size)
            break;
        printf("Write audio_frame error fail\n");
        usleep(1000);
    } while (!stop_play);

    return 0;
}

static void play_wav_file(char *wav_name, int fd)
{
	FILE *wav_file;
	int ret;
	uint32_t inputSize = 2048;
	uint8_t *inputBuf;
	uint32_t data_size = 0;

	wav_file = fopen(wav_name, "rb");
	if (!wav_file) {
		printf("fopen file \"%s\" fail, please check the file\n", wav_name);
		return;
	}
	printf("fopen file ok\n");

	inputBuf = (uint8_t*) malloc(inputSize);
	if (!inputBuf) {
		fclose(wav_file);
		printf("no mem\n");
		return;
	}

	fseek(wav_file, 40, SEEK_SET);
	fread(&data_size, 1, 4, wav_file);

	if (data_size == 0)
		data_size = 0xffffffff;
	while (!stop_play && data_size > 0) {
		ret = fread(inputBuf, 1, (inputSize > data_size) ? data_size : inputSize, wav_file);
		if (ret <= 0) {
			break;
		}
		data_size -= ret;
		auddec_decode(fd, inputBuf, ret);
	}
	//write eos
	AvPktHd pkthd = { 0 };
	pkthd.pts = 0;
	pkthd.size = 0;
	pkthd.flag = AV_PACKET_EOS;
	do {
		if(write(fd, (uint8_t *)&pkthd , sizeof(AvPktHd)) == sizeof(AvPktHd))
			break;
		printf("Write AvPktHd fail\n");
		usleep(1000);
	} while (!stop_play);

	while (!stop_play) {/* 如果不需要马上关闭，这一步不需要 */
		int eos;
		ioctl(fd, AUDDEC_CHECK_EOS, &eos);
		if (eos)
			break;
		usleep(1000);
	}

	printf("eos stop\n");

	free(inputBuf);
	fclose(wav_file);
}

static void *auddec_play_thread(void *arg)
{
	int ret = 0;
	void *wav;
	int format, sampleRate, channels, bitsPerSample;
	int fd = -1;
	char *wav_name = (char *)arg;
	wav = wav_read_open(wav_name);

	if (!wav) {
		printf("wav_read_open %s fail\n", wav_name);
		free(arg);
		return NULL;
	}

	if (!wav_get_header(wav, &format, &channels, &sampleRate, &bitsPerSample, NULL)) {
		printf("wav_get_header %s fail\n", wav_name);
		wav_read_close(wav);
		free(arg);
		return NULL;
	}

	if(format != 1 && (bitsPerSample != 16 || bitsPerSample != 8 || bitsPerSample != 32)) {
		printf("unspport wav file %s,format %d, bitsPerSample %d\n", wav_name, format, bitsPerSample);
		wav_read_close(wav);
		free(arg);
		return NULL;
	}

	wav_read_close(wav);

	fd = auddec_init(channels, bitsPerSample, sampleRate);
	if (fd < 0) {
		free(arg);
		return NULL;
	}

	play_wav_file(wav_name, fd);
	close(fd);

	free(arg);
	return NULL;
}


static int auddec_play_start(int argc, char *argv[])
{
	int ret = 0;

	if (argc < 2) {
		printf("please enter like: auddec_play_start /media/sda1/test.wav\n");
		return -1;
	}

	stop_play = 0;
	ret = pthread_create(&auddec_thread_id, NULL, auddec_play_thread, strdup(argv[1]));
	if(ret) {
		return -1;
	}
	return 0;
}

static int auddec_play_stop(int argc, char *argv[])
{
	stop_play = 1;
	if(auddec_thread_id)
		pthread_join(auddec_thread_id, NULL);
	auddec_thread_id = 0;
	return 0;
}

CONSOLE_CMD(auddec_play_start, "snd", auddec_play_start, CONSOLE_CMD_MODE_SELF,"auddec_play_start filepath")
CONSOLE_CMD(auddec_play_stop, "snd", auddec_play_stop, CONSOLE_CMD_MODE_SELF, "auddec_play_stop")
