#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <generated/br2_autoconf.h>
#include <kernel/drivers/avinit.h>
#include <kernel/module.h>
#include <sys/ioctl.h>
#include <freertos/FreeRTOS.h>
#include <hcuapi/avsync.h>
#include <hcuapi/snd.h>
#include <stdio.h>

static int hccast_type = HCCAST_TYPE_NONE;
/*
 * In case of boot show logo only (no audio), if there is no audio driver,
 * using the weak impl. to avoid compile error for video driver.
 *
 * But in case of boot show A/V, the audio driver would be selected by menuconfig
 * so that below weak impl. will be replaced by the impl. in audio driver.
 */
void __attribute__((weak)) avsync_set_stc_ms(enum AVSYNC_STCID id, uint32_t ms)
{
}

uint32_t __attribute__((weak)) avsync_ms_to_stc_tick(enum AVSYNC_STCID id, uint32_t ms)
{
	return ms * 45;
}

uint32_t __attribute__((weak)) avsync_stc_tick_to_ms(enum AVSYNC_STCID id, uint32_t tick)
{
	return (tick == 0xffffffff) ? 0 : tick / 45;
}

void __attribute__((weak)) avsync_video_update_stc(enum AVSYNC_STCID id , uint32_t tick)
{
    
}

uint32_t __attribute__((weak)) avsync_get_stc_tick(enum AVSYNC_STCID id)
{
    return 0;
}

uint32_t __attribute__((weak)) avsync_get_video_delay_ms(enum AVSYNC_STCID id)
{
    return 0;
}

void __attribute__((weak)) avsync_set_video_delay_ms(enum AVSYNC_STCID id , uint32_t delay)
{
    ;
}

void set_config_hccast_type(int type)
{
	hccast_type = type;
}

int get_config_hccast_type(void)
{
	return hccast_type;
}

#if CONFIG_VIDEO_VINDVP_SUPPORT
int get_config_vin_priority(void)
{
#ifdef CONFIG_VIN_PRIORITY
	return CONFIG_VIN_PRIORITY;
#else
	return (portPRI_TASK_HIGH+1);
#endif
}
#endif

#if CONFIG_AUDIO_SUPPORT
int get_config_audio_decoder_priority(void)
{
#ifdef CONFIG_AUDIO_DECODER_PRIORITY
	return CONFIG_AUDIO_DECODER_PRIORITY;
#else
	return portPRI_TASK_HIGH;
#endif
}

int get_config_audio_decoder_stacksize(void)
{
#ifdef CONFIG_AUDIO_DECODER_STACKSIZE
	return CONFIG_AUDIO_DECODER_STACKSIZE;
#else
	return 0x4000;
#endif
}

int get_config_audio_sink_priority(void)
{
#ifdef CONFIG_AUDIO_SINK_PRIORITY
	return CONFIG_AUDIO_SINK_PRIORITY;
#else
	return portPRI_TASK_HIGH;
#endif
}

int get_config_audio_sink_stacksize(void)
{
#ifdef CONFIG_AUDIO_SINK_STACKSIZE
	return CONFIG_AUDIO_SINK_STACKSIZE;
#else
	return 0x400;
#endif
}

module_system(apll_dai, apll_dai_init, NULL, 2)
module_driver(hc16xx_link, hc16xx_link_init, NULL, 2)

#if (CONFIG_AUDIO_I2SO_SUPPORT | CONFIG_AUDIO_PCMO_SUPPORT)
module_driver(audsink, audsink_driver_init, NULL, 0)
module_driver(auddec, auddec_driver_init, auddec_driver_exit, 0)
module_driver(i2so_platform, i2so_platform_init, NULL, 1)
#endif

#if (CONFIG_AUDIO_I2SO_SUPPORT | CONFIG_AUDIO_PCMO_SUPPORT | CONFIG_AUDIO_I2SI_SUPPORT | CONFIG_AUDIO_PCMI_SUPPORT)
module_driver(cjc8990_dai, cjc8990_dai_init, NULL, 0)
module_driver(cjc8988_dai, cjc8988_dai_init, NULL, 0)
module_driver(cl3016_dai, cl3016_dai_init, NULL, 0)
#endif

#if CONFIG_AUDIO_ES7243I_SUPPORT
module_driver(es7243i_dai, es7243i_dai_init, NULL, 0)
#endif

#if (CONFIG_AUDIO_I2SO_SUPPORT | CONFIG_AUDIO_PCMO_SUPPORT)
module_driver(cs4344_dai, cs4344_dai_init, NULL, 0)
module_driver(pwm_dac_dai, pwm_dai_init, NULL, 0)
#endif

#if (CONFIG_AUDIO_I2SO_SUPPORT | CONFIG_AUDIO_I2SI_SUPPORT | CONFIG_AUDIO_PCMI_SUPPORT | CONFIG_AUDIO_PCMO_SUPPORT | CONFIG_AUDIO_SPO_SUPPORT)
module_driver(i2s_dai, i2s_dai_init, NULL, 0)
#endif

#if (CONFIG_AUDIO_I2SO_SUPPORT | CONFIG_AUDIO_SPO_SUPPORT)
module_driver(hdmi_audio_dai, hdmi_audio_dai_init, NULL, 0)
#endif

#if (CONFIG_AUDIO_I2SI_SUPPORT | CONFIG_AUDIO_PCMI_SUPPORT)
module_driver(i2si_platform, i2si_platform_init, NULL, 1)
#endif

#if CONFIG_AUDIO_PCMI_SUPPORT
module_driver(pcmi_platform, pcmi_platform_init, NULL, 1)
#endif

#if (CONFIG_AUDIO_I2SI0_SUPPORT)
module_driver(i2si0_platform, i2si0_platform_init, NULL, 1)
module_driver(i2si0_dai, i2si0_dai_init, NULL, 0)
#endif

#if (CONFIG_AUDIO_I2SI1_SUPPORT)
module_driver(i2si1_platform, i2si1_platform_init, NULL, 1)
module_driver(i2si1_dai, i2si1_dai_init, NULL, 0)
#endif

#if (CONFIG_AUDIO_I2SI2_SUPPORT)
module_driver(i2si2_platform, i2si2_platform_init, NULL, 1)
module_driver(i2si2_dai, i2si2_dai_init, NULL, 0)
#endif

#if(CONFIG_AUDIO_PCMI0_SUPPORT)
module_driver(pcmi0_platform, pcmi0_platform_init, NULL, 1)
#endif

#if(CONFIG_AUDIO_PCMI1_SUPPORT)
module_driver(pcmi1_platform, pcmi1_platform_init, NULL, 1)
#endif

#if(CONFIG_AUDIO_PCMI2_SUPPORT)
module_driver(pcmi2_platform, pcmi2_platform_init, NULL, 1)
#endif

#if CONFIG_AUDIO_TDMI_SUPPORT
module_driver(tdmi_dai, tdmi_dai_init, NULL, 0)
module_driver(tdmi_platform, tdmi_platform_init, NULL, 1)
#endif

#if CONFIG_AUDIO_PDMI0_SUPPORT
module_driver(pdmi0_dai, pdmi0_dai_init, NULL, 0)
module_driver(pdmi0_platform, pdmi0_platform_init, NULL, 1)
#endif

#if CONFIG_AUDIO_SPO_SUPPORT
module_driver(spo_dai, spo_dai_init, NULL, 0)
module_driver(spo_platform, spo_platform_init, NULL, 1)
#endif

#if (CONFIG_AUDIO_PCMI_SUPPORT | CONFIG_AUDIO_PCMO_SUPPORT)
module_driver(pcmo_dai, pcmo_dai_init, NULL, 0)
module_driver(pcmo_platform, pcmo_platform_init, NULL, 1)
#endif

#if CONFIG_AUDIO_PCMO_SUPPORT
module_driver(wm8960_dai, wm8960_dai_init, NULL, 0)
#endif

#if (CONFIG_AUDIO_PCMI_SUPPORT | CONFIG_AUDIO_PCMI0_SUPPORT | CONFIG_AUDIO_PCMI1_SUPPORT | CONFIG_AUDIO_PCMI2_SUPPORT)
module_driver(wm8960i_dai, wm8960i_dai_init, NULL, 0)
#endif

#if CONFIG_AUDIO_PCMI0_SUPPORT
module_driver(pcmi0_dai, pcmi0_dai_init, NULL, 0)
#endif

#if CONFIG_AUDIO_PCMI1_SUPPORT
module_driver(pcmi1_dai, pcmi1_dai_init, NULL, 0)
#endif

#if CONFIG_AUDIO_PCMI2_SUPPORT
module_driver(pcmi2_dai, pcmi2_dai_init, NULL, 0)
#endif

#if CONFIG_AUDIO_SPIN_SUPPORT
module_driver(spin_platform, spin_platform_init, NULL, 1)
#endif
#endif

#if CONFIG_VIDEO_SUPPORT
module_driver(viddec, viddec_driver_init, viddec_driver_exit, 0)
module_driver(llav_dis, llav_dis_init, NULL, 0)
#endif

#if CONFIG_VIDEO_DECODER_SUPPORT
module_driver(llav_vdec, llav_vdec_init, NULL, 2)
#endif

#if CONFIG_KSC_SUPPORT
module_driver(llav_ksc, llav_ksc_init, NULL, 0)
#endif

#if CONFIG_VIDEO_HDMI_TX_SUPPORT
module_driver(llav_hdmi, llav_hdmi_init, NULL, 1)
#endif

#if CONFIG_VIDEO_HDMI_RX_SUPPORT
__attribute__((weak)) int hdmi_rx_dev_init(void)
{
	return 0;
}
module_driver(hdmi_rx, hdmi_rx_dev_init, NULL, 1)
module_driver(htx_dai, htx_dai_init, NULL, 0)
module_driver(htx_platform, htx_platform_init, NULL, 1)
#endif

#if CONFIG_VIDEO_TVDECODER_SUPPORT
module_driver(tv_decoder, tv_decoder_init, NULL, 0)
#endif

#if CONFIG_VIDEO_SINK_SUPPORT
module_driver(vidsink, vidsink_init, NULL, 0)
#endif

#if CONFIG_VIDEO_VINDVP_SUPPORT
module_driver(vindvp, vindvp_init, NULL, 0)
#endif

#if CONFIG_DRV_MIPI
module_system(mipi, mipi_init, mipi_exit, 4)
#endif

#if CONFIG_PQ_SUPPORT
module_driver(pq , pq_init , NULL , 0)
#endif

#if CONFIG_AVSYNC_SUPPORT
module_driver(avsync, avsync_driver_init, NULL, 0)
#endif
#if CONFIG_AUDIO_INIT_CLOCK_SUPPORT
static int clock_init(void)
{
	struct snd_pcm_params params = {0};
	int fd, fd2;

	fd = open("/dev/sndC0i2so", O_WRONLY);
	if (fd < 0)
		return 0;
	fd2 = open("/dev/sndC0spo", O_WRONLY);

	params.channels = 2;
	params.period_size = 1536;
	params.align = 0;
	params.rate = CONFIG_AUDIO_INIT_CLOCK_RATE;
	params.periods = 8;
	params.bitdepth = 16;
	params.start_threshold = 2;
	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S16_LE;
	params.sync_mode = 0;
	snd_pcm_uframes_t poll_size = 1536;

	ioctl(fd, SND_IOCTL_SET_MUTE, 1);
	ioctl(fd, SND_IOCTL_HW_PARAMS, &params);
	ioctl(fd, SND_IOCTL_AVAIL_MIN, &poll_size);
	ioctl(fd, SND_IOCTL_START, 0);
	ioctl(fd, SND_IOCTL_DROP, 0);
	ioctl(fd, SND_IOCTL_HW_FREE, 0);
	if (fd2 >= 0) {
		params.pcm_source = SND_SPO_SOURCE_SPODMA;
		ioctl(fd2, SND_IOCTL_HW_PARAMS, &params);
		ioctl(fd2, SND_IOCTL_START, 0);
		ioctl(fd2, SND_IOCTL_DROP, 0);
		ioctl(fd2, SND_IOCTL_HW_FREE, 0);
		close(fd2);
	}
	ioctl(fd, SND_IOCTL_SET_MUTE, 0);
	close(fd);
	return 0;
}

__initcall(clock_init);
#endif
