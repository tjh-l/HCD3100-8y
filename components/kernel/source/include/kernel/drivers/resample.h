#ifndef _HCUAPI_RESAMPLE_H_
#define _HCUAPI_RESAMPLE_H_

#include <kernel/drivers/snd.h>
#include <errno.h>

#define MAX_RESCALE_CHANNELS 8

typedef struct {
  int S_16_32[8];
  int S_32_24[8];
  int S_24_48[8];
} WebRtcSpl_State16khzTo48khz;

typedef struct {
  int S_48_48[16];
  int S_48_32[8];
  int S_32_16[8];
} WebRtcSpl_State48khzTo16khz;

#define RESAMPLE_REMAINS_LEN 1280
struct rescale_webrtc {
	int sample_rate_in;
	int sample_rate_out;
	int16_t *tmp_mem;
	size_t tmp_mem_size;
	void *tmp_data_planner;
	size_t tmp_data_planner_size;
	int16_t remains_buf[2][RESAMPLE_REMAINS_LEN * 2];
	size_t remains[2];
	int tmp_value[496];
	int state[10][8];
	WebRtcSpl_State16khzTo48khz state2[2];
	WebRtcSpl_State48khzTo16khz state3[2];
	int access;
	int format;
	int align;
};

struct resample_pcm_params {
	/* SND_PCM_ACCESS_* */
	int access;

	/* SND_PCM_FORMAT_* */
	int format;

	/* rate in Hz, target rate */
	unsigned int rate;

	/* rate in Hz, original rate of source in Hz */
	unsigned int src_rate;

	/*
	 * 8/16/32
	 * Do not support SND_PCM_FMTBIT_*_3LE
	*/
	uint8_t bitdepth;

	uint8_t channels;

	uint8_t align;
};

struct audio_rescale_handle_t {
	struct rescale_webrtc *rescale_w;
	void *rescale_s;
	void *rescale_tmp_buf;
	size_t rescale_tmp_size;
	void *rescale_buf;
	size_t rescale_size;
	int scale_case;
	struct resample_pcm_params params;
};

enum snd_rate_group {
	SND_RATE_GROUP_44_1_K,
	SND_RATE_GROUP_48_K,
	SND_RATE_GROUP_OTHER,
};

int resample_process_multi_times(struct rescale_webrtc *param,void *in,void *out,
         int frames,int ipitch,uint8_t channels_in, int pts_in, int *pts_out);
enum snd_rate_group rate_group(int rate);
enum snd_rate_group rate_group2(int rate);
uint32_t get_closest_rate_in_group(uint32_t rate, enum snd_rate_group dst_group);
void dsp_eq_init(int rate, int channels, int bitdepth);
void dsp_eq_process(void *data, uint32_t size);
void dsp_eq_enable(int enable);
void dsp_set_eq_coefs(int band, int cutoff, int q, int gain);

#endif	/* _HCUAPI_RESAMPLE_H_ */
