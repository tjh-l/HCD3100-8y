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
#include <hcuapi/common.h>
#include <kernel/lib/console.h>
#include "wavreader.h"

static uint8_t g_volume = 100;

static int set_i2so_volume(uint8_t volume)
{
	int snd_fd = -1;

	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf("open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
	volume = 0;
	ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &g_volume);
	printf("current volume is %d\n", g_volume);

	close(snd_fd);
	return 0;
}

static void usage(const char *name)
{
	printf("Usage: %s path-to-wav.wav\n", name);
}

static int pcm_dec(int argc, char *argv[])
{
	struct audio_config cfg = { 0 };
	const char *infile;
	uint8_t* inputBuf;
	void *wav;
	int format, sampleRate, channels, bitsPerSample;
	int inputSize;
	int fd, ret;
	int read;
	AvPktHd pkthd = { 0 };
	int eos = 0;

	if (argc != 2)
		usage(argv[0]);
	infile = argv[1];

	set_i2so_volume(100);

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

	inputSize = channels * 2 * 160;
	inputBuf = (uint8_t*) malloc(inputSize);

	fd = open("/dev/auddec", O_RDWR);
	if (fd < 0) {
		wav_read_close(wav);
		printf("Open /dev/auddec error.");
		return 1;
	}

	cfg.bits_per_coded_sample = bitsPerSample;
	cfg.channels = channels;
	cfg.sample_rate = sampleRate;
	cfg.codec_id = HC_AVCODEC_ID_PCM_S16LE;
	printf("bitsPerSample %d, channels %d, sample_rate %d\r\n", bitsPerSample, channels, sampleRate);

	if (ioctl(fd, AUDDEC_INIT, &cfg) != 0) {
		printf("Init auddec error.");
		wav_read_close(wav);
		close(fd);
		return 1;
	}

	if (ioctl(fd, AUDDEC_START, 0) < 0) {
		printf("AUDDEC_START failed\r\n");
		wav_read_close(wav);
		close(fd);
		return 1;
	}

	while (1) {
		read = wav_read_data(wav, inputBuf, inputSize);
		if (read < inputSize) {
			break;
			wav_read_close(wav);
			wav = wav_read_open(infile);
			continue;
		}

		memset(&pkthd, 0, sizeof(pkthd));
		pkthd.dur = 0;
		pkthd.size = inputSize;
		pkthd.flag = AV_PACKET_ES_DATA;
		pkthd.pts = -1;

		while (write(fd, (uint8_t *)&pkthd, sizeof(AvPktHd)) != sizeof(AvPktHd))
			usleep(20 * 1000);

		while (write(fd, inputBuf, inputSize) != (int)inputSize)
			usleep(20 * 1000);
	}

	memset(&pkthd, 0, sizeof(pkthd));
	pkthd.flag = AV_PACKET_EOS;
	while (write(fd, (uint8_t *)&pkthd, sizeof(AvPktHd)) != sizeof(AvPktHd))
		usleep(20 * 1000);

	while (1) {
		ioctl(fd, AUDDEC_CHECK_EOS, &eos);
		if (!eos)
			usleep(50 * 1000);
		else
			break;
	}

	close(fd);
	wav_read_close(wav);
	free(inputBuf);

	return 0;
}

CONSOLE_CMD(pcm_play, "snd", pcm_dec, CONSOLE_CMD_MODE_SELF, "do pcm dec")
