#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#ifdef HC_RTOS
#include <aircast/aircast_api.h>
#include <freertos/timers.h>
#include <kernel/dynload.h>
#else
#include <hccast/aircast_api.h>
#endif

#include <hudi/hudi_com.h>
#include <hudi/hudi_vdec.h>
#include <hudi/hudi_audsink.h>
#include <hudi/hudi_snd.h>
#include <hudi/hudi_adec.h>
#include <hudi/hudi_display.h>

#include <hccast_com.h>
#include <hccast_log.h>
#include <hccast_air.h>
#include <hccast_av.h>

#include "hccast_air_avplayer.h"
#include "hccast_air_api.h"

static unsigned int g_video_width = 0;
static unsigned int g_video_height = 0;
static int g_vdec_started = 0;
static int g_audio_started = 0;
static int g_audio_type = -1;
static unsigned int g_audio_pts = 0;
static void *g_audio_renderer = NULL;
static unsigned char *g_aac_decode_buf = NULL;
static unsigned int g_vfeed_cnt = 0;
static unsigned int g_vfeed_len = 0;
static unsigned int g_afeed_cnt = 0;
static unsigned int g_afeed_len = 0;
static unsigned int g_afeed_drop_cnt = 0;
static unsigned int g_acontinue_drop_cnt = 0;
static int g_air_timer_start = 0;
static int g_audio_sync_thresh = 0;
static int g_afeed_err_cnt = 0;

static hudi_handle g_vdec_hdl = NULL;
static hudi_handle g_adec_hdl = NULL;
static hudi_handle g_snd_hdl = NULL;
static hudi_handle g_avsync_hdl = NULL;
static hudi_handle g_dis_hdl = NULL;

static pthread_mutex_t g_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_audio_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_video_mutex = PTHREAD_MUTEX_INITIALIZER;

static uint8_t g_es_dump_en = 0;
static char g_es_dump_folder[64] = {0};
static unsigned int g_es_dump_cnt = 0;
static FILE *g_es_vfp = NULL;

#ifdef HC_RTOS
extern void *create_aac_eld(void);
extern void aac_eld_decode_frame(void *aac_eld, unsigned char *inbuffer, int inputsize, void *outbuffer, int *outputsize);
extern void destroy_aac_eld(void *aac);
#endif

#ifndef MKTAG
#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#endif

static void *_hccast_air_player_state_timer(void *args)
{
    while (g_vdec_started)
    {
        hccast_log(LL_NOTICE, "[FEED] V(%d:%d) A(%d:%d:%d)\n", g_vfeed_cnt, g_vfeed_len, g_afeed_cnt, g_afeed_len, g_afeed_drop_cnt);
        g_vfeed_cnt = 0;
        g_vfeed_len = 0;
        g_afeed_cnt = 0;
        g_afeed_len = 0;
        g_afeed_drop_cnt = 0;
        usleep(2 * 1000 * 1000);
    }

    pthread_mutex_lock(&g_timer_mutex);
    g_air_timer_start = 0;
    pthread_mutex_unlock(&g_timer_mutex);

    return NULL;
}

static void _hccast_air_audio_sync_thresh_set()
{
    if (g_avsync_hdl)
    {
        hudi_avsync_audsync_thres_get(g_avsync_hdl, &g_audio_sync_thresh);
        hudi_avsync_audsync_thres_set(g_avsync_hdl, 300);
    }
}

static void _hccast_air_audio_sync_thresh_restore()
{
    if (g_avsync_hdl)
    {
        hudi_avsync_audsync_thres_set(g_avsync_hdl, g_audio_sync_thresh);
        g_audio_sync_thresh = 0;
    }
}

static int _hccast_air_quick_mode_get()
{
    int quick_mode = 2;

    hccast_air_event_notify(HCCAST_AIR_GET_MIRROR_QUICK_MODE_NUM, (void *)&quick_mode, NULL);

    return quick_mode;
}

static int _hccast_air_rotation_info_get(hccast_air_rotation_t *rotate_info)
{
    hccast_air_event_notify(HCCAST_AIR_GET_MIRROR_ROTATION_INFO, (void*)rotate_info, NULL);

    return 0;
}

static int _hccast_air_preview_info_get(hccast_air_deview_t *preview)
{
    hccast_air_event_notify(HCCAST_AIR_GET_PREVIEW_INFO, (void *)preview, NULL);

    return 0;
}

static int _hccast_air_video_config_get(hccast_com_video_config_t *video_config)
{
    hccast_air_event_notify(HCCAST_AIR_GET_VIDEO_CONFIG, (void*)video_config, NULL);

    return 0;
}

static void _hccast_air_aaceld_reset()
{
#ifdef HC_RTOS
    if (g_audio_renderer)
    {
        destroy_aac_eld(g_audio_renderer);
        g_audio_renderer = NULL;
    }

    if (g_aac_decode_buf)
    {
        free(g_aac_decode_buf);
        g_aac_decode_buf = NULL;
    }

    g_audio_renderer = create_aac_eld();
    g_aac_decode_buf = malloc(8194);
#endif
}

int hccast_air_es_dump_start(char *folder)
{
    memset(g_es_dump_folder, 0, 64);
    strcat(g_es_dump_folder, folder);
    g_es_dump_en = 1;

    return 0;
}

int hccast_air_es_dump_stop()
{
    g_es_dump_en = 0;

    return 0;
}

static int _hccast_air_display_event_handler(hudi_handle handle, unsigned int event,
                                              void *arg, void *user_data)
{
    unsigned long b_vscreen = 0;

    switch (event)
    {
        case DIS_NOTIFY_MIRACAST_VSRCEEN:
            pthread_mutex_lock(&g_video_mutex);
            b_vscreen = (unsigned long)arg;
            hccast_air_event_notify(HCCAST_AIR_MIRROR_SCREEN_DETECT_NOTIFY, (void*)&b_vscreen, NULL);
            pthread_mutex_unlock(&g_video_mutex);
            break;
        default:
            break;
    }

    return 0;
}

int hccast_air_video_open()
{
    pthread_t tid;
    pthread_attr_t thread_attr;
    char path[128] = {0};
    hccast_air_deview_t preview_info = {0};
    hccast_com_video_config_t video_config = {0};
    struct video_config mvcfg;
    dis_aspect_mode_t aspect_info = {0};
    int frame_rate = 0;

#ifdef HC_RTOS
    vTaskEnableGlobalTimeSlice();
    vTaskSetGlobalTimeSlice(1);
#endif

    hccast_log(LL_NOTICE, "[%s - %d]\n", __func__, __LINE__);

    pthread_mutex_lock(&g_video_mutex);
    if (g_vdec_started || g_vdec_hdl)
    {
        hccast_log(LL_ERROR, "Video already start\n");
        pthread_mutex_unlock(&g_video_mutex);
        return -1;
    }

    g_video_width = 0;
    g_video_height = 0;
    video_config.video_pbp_mode = HCCAST_COM_VIDEO_PBP_OFF;
    video_config.video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_HD;
    video_config.video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_MAIN;
    frame_rate = hccast_air_mirror_fps_get();
    _hccast_air_preview_info_get(&preview_info);
    _hccast_air_video_config_get(&video_config);

    memset(&mvcfg, 0, sizeof(struct video_config));
    mvcfg.codec_id = HC_AVCODEC_ID_H264;
    mvcfg.sync_mode = AVSYNC_TYPE_UPDATESTC;
    mvcfg.decode_mode = VDEC_WORK_MODE_KSHM;
    mvcfg.pic_width = 1920;
    mvcfg.pic_height = 1080;
    mvcfg.pixel_aspect_x = 1;
    mvcfg.pixel_aspect_y = 1;
    mvcfg.frame_rate = frame_rate * 1000;
    mvcfg.preview = 0;
    mvcfg.extradata_size = 0;
    mvcfg.quick_mode = _hccast_air_quick_mode_get();
    mvcfg.pbp_mode = hccast_com_video_pbp_mode_translate(video_config.video_pbp_mode);
    mvcfg.dis_type = hccast_com_video_dis_type_translate(video_config.video_dis_type);
    mvcfg.dis_layer = hccast_com_video_dis_layer_translate(video_config.video_dis_layer);
    mvcfg.kshm_size = 0x400000;
    mvcfg.rotate_enable = 1;
    mvcfg.auto_audio_master = 1;
    
    if (preview_info.preview_en)
    {
        mvcfg.preview = 1;
        mvcfg.src_area.x = preview_info.src_rect.x;
        mvcfg.src_area.y = preview_info.src_rect.y;
        mvcfg.src_area.w = preview_info.src_rect.w;
        mvcfg.src_area.h = preview_info.src_rect.h;
        mvcfg.dst_area.x = preview_info.dst_rect.x;
        mvcfg.dst_area.y = preview_info.dst_rect.y;
        mvcfg.dst_area.w = preview_info.dst_rect.w;
        mvcfg.dst_area.h = preview_info.dst_rect.h;
    }
    
    if (0 != hudi_vdec_open(&g_vdec_hdl, &mvcfg))
    {
        return -1;
    }
    
    hudi_vdec_start(g_vdec_hdl);

    if (!g_dis_hdl)
    {
        hudi_display_open(&g_dis_hdl);
    }

    if (g_dis_hdl)
    {
        hudi_display_event_register(g_dis_hdl, _hccast_air_display_event_handler, DIS_NOTIFY_MIRACAST_VSRCEEN, NULL);
    }

    if (!g_avsync_hdl)
    {
        hudi_avsync_open(&g_avsync_hdl);
    }

    g_vdec_started = 1;

    _hccast_air_audio_sync_thresh_set();

    pthread_mutex_lock(&g_timer_mutex);
    if (g_air_timer_start == 0)
    {
        g_air_timer_start = 1;

        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&tid, &thread_attr, _hccast_air_player_state_timer, NULL) != 0)
        {
            g_air_timer_start = 0;
        }

        pthread_attr_destroy(&thread_attr);
    }
    pthread_mutex_unlock(&g_timer_mutex);
    
    if (g_es_dump_en)
    {
        sprintf(path, "%s/aircast-%d.h264", g_es_dump_folder, g_es_dump_cnt);
        g_es_dump_cnt ++;

        g_es_vfp = fopen(path, "w+");
    }
    
    pthread_mutex_unlock(&g_video_mutex);
    
    return 0;
}

void hccast_air_video_close()
{
    hccast_log(LL_NOTICE, "[%s - %d]\n", __func__, __LINE__);

#ifdef HC_RTOS
    vTaskDisableGlobalTimeSlice();
#endif

    pthread_mutex_lock(&g_video_mutex);
    if (g_es_vfp)
    {
        fflush(g_es_vfp);
        fclose(g_es_vfp);
        g_es_vfp = NULL;
    }

    if (!g_vdec_started)
    {
        hccast_log(LL_WARNING, "[%s - %d] video has been close.\n", __func__, __LINE__);
        pthread_mutex_unlock(&g_video_mutex);
        return ;
    }

    if (g_vdec_hdl)
    {
        hudi_vdec_close(g_vdec_hdl, 0);
        g_vdec_hdl = NULL;
    }

    _hccast_air_audio_sync_thresh_restore();

    if (g_avsync_hdl)
    {
        hudi_avsync_close(g_avsync_hdl);
        g_avsync_hdl = NULL;
    }

    if (g_dis_hdl)
    {
        hudi_display_close(g_dis_hdl);
        g_dis_hdl = NULL;
    }

    g_vdec_started = 0;
    g_video_width = 0;
    g_video_height = 0;
    pthread_mutex_unlock(&g_video_mutex);
}

int hccast_air_video_feed(unsigned char *data, unsigned int len,
                          unsigned int pts, int last_slice, unsigned int width, unsigned int height)
{
    int resolution_changed = 0;
    hccast_air_rotation_t rotate_info = {0};
    unsigned char data_aux[8] = {0};
    AvPktHd pkt;

    if (g_es_vfp)
        fwrite(data, 1, len, g_es_vfp);

    pthread_mutex_lock(&g_video_mutex);
    if (!g_vdec_started)
    {
        hccast_log(LL_WARNING, "Video is not started\n");
        pthread_mutex_unlock(&g_video_mutex);
        return -1;
    }

    g_vfeed_cnt ++;
    g_vfeed_len += len;


    if (width && (g_video_width != width))
    {
        if (g_video_width)
        {
            resolution_changed = 1;
        }
        g_video_width = width;
        //hccast_log(LL_WARNING, "Video width: %d\n", width);
    }

    if (height && (g_video_height != height))
    {
        if (g_video_height)
        {
            resolution_changed = 1;
        }
        g_video_height = height;
        //hccast_log(LL_WARNING, "Video height: %d\n", height);
    }

    rotate_info.src_w = g_video_width;
    rotate_info.src_h = g_video_height;
    _hccast_air_rotation_info_get(&rotate_info);
    memset(&pkt, 0, sizeof(pkt));

    if (g_vdec_hdl)
    {
        pkt.dur = 0;
        pkt.size = len;
        pkt.flag = AV_PACKET_ES_DATA;
        pkt.pts = pts;
        pkt.video_rotate_mode = rotate_info.rotate_angle;
        pkt.video_mirror_mode = rotate_info.flip_mode;
        hudi_vdec_feed(g_vdec_hdl, data, &pkt);

        if (last_slice)
        {
            data_aux[0] = 0x00;
            data_aux[1] = 0x00;
            data_aux[2] = 0x00;
            data_aux[3] = 0x01;
            data_aux[4] = 0x09;
            pkt.size = 5;
            hudi_vdec_feed(g_vdec_hdl, data_aux, &pkt);
        }
    }

    pthread_mutex_unlock(&g_video_mutex);

    return 0;
}

int hccast_air_audio_open()
{
    unsigned int audsink = AUDSINK_SND_DEVBIT_I2SO;
    struct snd_pcm_params snd_config;
    struct audio_config adec_config;
    unsigned int force_auddec = 0;
    unsigned int audio_disable = 0;

    hccast_com_audio_disable_get(&audio_disable);
    hccast_com_audsink_get(&audsink);
    hccast_com_force_auddec_get(&force_auddec);

    if (audio_disable)
    {
        return -1;
    }
    
    pthread_mutex_lock(&g_audio_mutex);
    if (g_audio_started)
    {
        hccast_log(LL_WARNING, "Audio has been started!\n");
        pthread_mutex_unlock(&g_audio_mutex);
        return -1;
    }

#ifdef HC_RTOS
    dynload_builtin_section(DYNLOAD_SECTION_ID_AACELD);

    if (force_auddec)
    {
        memset(&adec_config, 0, sizeof(struct audio_config));
        adec_config.codec_id = HC_AVCODEC_ID_PCM_S16LE;
        adec_config.sync_mode = AVSYNC_TYPE_SYNCSTC;
        adec_config.bits_per_coded_sample = 16;
        adec_config.channels = 2;
        adec_config.sample_rate = 44100;
        adec_config.kshm_size = 0xa0000;
        adec_config.snd_devs = audsink;
        adec_config.auto_audio_master = 1;
        if (0 != hudi_adec_open(&g_adec_hdl, &adec_config))
        {
            return -1;
        }
        hudi_adec_start(g_adec_hdl);
    }
    else
    {
        if (0 != hudi_snd_open(&g_snd_hdl, audsink))
        {
            pthread_mutex_unlock(&g_audio_mutex);
            return -1;
        }

        memset(&snd_config, 0, sizeof(struct snd_pcm_params));
        snd_config.access = SND_PCM_ACCESS_RW_INTERLEAVED;
        snd_config.format = SND_PCM_FORMAT_S16_LE;
        snd_config.sync_mode = AVSYNC_TYPE_SYNCSTC;
        snd_config.rate = 44100;
        snd_config.period_size = 3072;
        snd_config.periods = 40;
        snd_config.start_threshold = 2;
        snd_config.channels = 2;
        snd_config.bitdepth = 16;
        snd_config.auto_audio_master = 1;
        if (0 != hudi_snd_start(g_snd_hdl, &snd_config))
        {
            hudi_snd_close(g_snd_hdl);
            g_snd_hdl = NULL;
            pthread_mutex_unlock(&g_audio_mutex);
            return -1;
        }
    }

    g_audio_renderer = create_aac_eld();
    g_aac_decode_buf = malloc(8194);
#else
    memset(&adec_config, 0, sizeof(struct audio_config));
    adec_config.codec_id = HC_AVCODEC_ID_AAC;
    adec_config.sync_mode = AVSYNC_TYPE_SYNCSTC;
    adec_config.codec_tag = MKTAG('e', 'l', 'f', ' ');
    adec_config.sample_rate = 44100;
    adec_config.snd_devs = audsink;
    adec_config.bits_per_coded_sample = 16;
    adec_config.channels = 2;
    adec_config.auto_audio_master = 1;
    if (0 != hudi_adec_open(&g_adec_hdl, &adec_config))
    {
        pthread_mutex_unlock(&g_audio_mutex);
        return -1;
    }
    hudi_adec_start(g_adec_hdl);
#endif

    g_audio_started = 1;
    g_audio_type = HCCAST_AIR_AUDIO_AAC_ELD;
    g_acontinue_drop_cnt = 0;
    g_afeed_err_cnt = 0;

    pthread_mutex_unlock(&g_audio_mutex);

    return 0;
}

void hccast_air_audio_close()
{
    pthread_mutex_lock(&g_audio_mutex);

    if (!g_audio_started)
    {
        hccast_log(LL_WARNING, "[%s - %d] audio has been close.\n", __func__, __LINE__);
        pthread_mutex_unlock(&g_audio_mutex);
        return ;
    }

    if (g_adec_hdl)
    {
        hudi_adec_close(g_adec_hdl);
        g_adec_hdl = NULL;
    }

    if (g_snd_hdl)
    {
        hudi_snd_close(g_snd_hdl);
        g_snd_hdl = NULL;
    }

#ifdef HC_RTOS
    if (g_audio_renderer)
    {
        destroy_aac_eld(g_audio_renderer);
        g_audio_renderer = NULL;
    }
#endif

    if (g_aac_decode_buf)
    {
        free(g_aac_decode_buf);
        g_aac_decode_buf = NULL;
    }

    g_audio_started = 0;
    g_audio_pts = 0;
    g_acontinue_drop_cnt = 0;
    g_audio_type = -1;

    pthread_mutex_unlock(&g_audio_mutex);
}

static int hccast_air_audio_reopen(int type)
{
    unsigned int audsink = AUDSINK_SND_DEVBIT_I2SO;
    struct audio_config adec_config;

    if (g_adec_hdl)
    {
        hudi_adec_close(g_adec_hdl);
        g_adec_hdl = NULL;
    }

    hccast_com_audsink_get(&audsink);

    memset(&adec_config, 0, sizeof(struct audio_config));
    if (HCCAST_AIR_AUDIO_AAC_ELD == type)
    {
        adec_config.codec_id = HC_AVCODEC_ID_AAC;
        adec_config.codec_tag = MKTAG('e', 'l', 'f', ' ');
    }
    else if (HCCAST_AIR_AUDIO_PCM == type)
    {
        adec_config.codec_id = HC_AVCODEC_ID_PCM_S16LE;
    }
    else
    {
        hccast_log(LL_ERROR, "Unsupport audio format(0x%x)!\n", type);
        return -1;
    }
    adec_config.sync_mode = AVSYNC_TYPE_SYNCSTC;
    adec_config.sample_rate = 44100;
    adec_config.snd_devs = audsink;
    adec_config.bits_per_coded_sample = 16;
    adec_config.channels = 2;
    if (0 != hudi_adec_open(&g_adec_hdl, &adec_config))
    {
        return -1;
    }
    hudi_adec_start(g_adec_hdl);

    return 0;
}

int hccast_air_audio_feed(int type, unsigned char *buf, int length, unsigned int pts)
{
    AvPktHd adec_pkt;
    struct snd_xfer snd_pkt;
    unsigned int audio_disable = 0;

    hccast_com_audio_disable_get(&audio_disable);
    if (audio_disable)
    {
        return -1;
    }

    pthread_mutex_lock(&g_audio_mutex);
    if (!g_audio_started )
    {
        hccast_log(LL_WARNING, "Audio is not started\n");
        pthread_mutex_unlock(&g_audio_mutex);
        return -1;
    }

    if (g_audio_pts == 0) //every first open audio backup pts.
    {
        g_audio_pts = pts;
        g_acontinue_drop_cnt = 0;
    }
    else
    {
        if (pts <= g_audio_pts)
        {
            if (g_acontinue_drop_cnt == 3)
            {
                g_audio_pts = pts;
                g_acontinue_drop_cnt = 0;
            }
            else
            {
                //printf("-%ld, %u %u\n",g_audio_pts-pts,g_audio_pts,pts);
                g_acontinue_drop_cnt ++;
                g_afeed_drop_cnt ++;
                pthread_mutex_unlock(&g_audio_mutex);
                return 0;
            }
        }
        else
        {
            g_audio_pts = pts;
            g_acontinue_drop_cnt = 0;
        }
    }

    g_afeed_cnt ++;
    g_afeed_len += length;

#ifdef HC_RTOS
    if (HCCAST_AIR_AUDIO_AAC_ELD == type)
    {
        int out_len = 0;
        vTaskSuspendAll();
        aac_eld_decode_frame(g_audio_renderer, buf, length, g_aac_decode_buf, &out_len);
        xTaskResumeAll();
        portYIELD();
        if (out_len == 0)
        {
            g_afeed_err_cnt ++;
            if (g_afeed_err_cnt == 15)
            {
                _hccast_air_aaceld_reset();
                hccast_log(LL_ERROR, "%s %d, aac eld reset.\n", __func__, g_afeed_err_cnt);
                g_afeed_err_cnt = 0;
                pthread_mutex_unlock(&g_audio_mutex);
                return 0;
            }
            hccast_log(LL_ERROR, "AAC-ELD decode error\n");
            pthread_mutex_unlock(&g_audio_mutex);
            return 0;
        }
        else
        {
            g_afeed_err_cnt = 0;
        }
        buf = g_aac_decode_buf;
        length = out_len;
    }
#else
    if (type != g_audio_type)
    {
        hccast_air_audio_reopen(type);
        g_audio_type = type;
    }
#endif

    if (g_adec_hdl)
    {
        memset(&adec_pkt, 0, sizeof(AvPktHd));
        adec_pkt.size = length;
        adec_pkt.pts = pts;
        adec_pkt.flag = AV_PACKET_ES_DATA;
        hudi_adec_feed(g_adec_hdl, buf, &adec_pkt);
    }

    if (g_snd_hdl)
    {
        memset(&snd_pkt, 0, sizeof(struct snd_xfer));
        snd_pkt.data = buf;
        snd_pkt.frames = length / 4;
        snd_pkt.tstamp_ms = pts;
        hudi_snd_feed(g_snd_hdl, &snd_pkt);
    }

    pthread_mutex_unlock(&g_audio_mutex);

    return 0;
}

aircast_av_func_t g_aircast_av_func =
{
    ._video_open    = hccast_air_video_open,
    ._video_close   = hccast_air_video_close,
    ._video_feed    = hccast_air_video_feed,
    ._audio_open    = hccast_air_audio_open,
    ._audio_close   = hccast_air_audio_close,
    ._audio_feed    = hccast_air_audio_feed
};
