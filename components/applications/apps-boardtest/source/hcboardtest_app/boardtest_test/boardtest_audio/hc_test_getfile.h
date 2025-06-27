#ifndef __HC_TEST_GETFILEPATH_H__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <iniparser.h>
#include <linux/delay.h>
#include <hcuapi/snd.h>
#include <hcuapi/gpio.h>
#include <hcuapi/audsink.h>
#include <hcuapi/hdmi_tx.h>
#include <boardtest_module.h>
#include <kernel/drivers/platform_register.h>
#include <generated/br2_autoconf.h>

struct wave_header_tag {
	char chunk_id[4];
	unsigned long chunk_size;
	char format[4];
};

struct wave_format {
	char sub_chunk1_id[4];
	unsigned long sub_chunk1_size;
	unsigned short audio_format;
	unsigned short channels_num;
	unsigned long sample_rate;
	unsigned long byte_rate;
	unsigned short block_align;
	unsigned short bits_per_sample;
};

struct wave_data {
	char sub_chunk2_id[4];
	unsigned long sub_chunk2_size;
};

struct wave_header {
	struct wave_header_tag wav_header_tag;
	struct wave_format wav_fmt;
	struct wave_data wav_dat;
};

void hc_test_findfile(char **userpath, const char *filetype);

#endif
