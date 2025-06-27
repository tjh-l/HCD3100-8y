#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>

#ifdef HC_RTOS
#include <miracast/miracast_api.h>
#else
#include <hccast/miracast_api.h>
#endif

#include <hudi/hudi_com.h>
#include <hudi/hudi_vdec.h>
#include <hudi/hudi_audsink.h>
#include <hudi/hudi_snd.h>
#include <hudi/hudi_adec.h>
#include <hudi/hudi_display.h>

#include <hccast_com.h>
#include <hccast_log.h>
#include <hccast_av.h>
#include <hccast_dsc.h>
#include <hccast_mira.h>

#include "hccast_mira_avplayer.h"

static unsigned int g_vfeed_cnt = 0;
static unsigned int g_vfeed_cnt_for_order = 0;
static unsigned int g_vfeed_len = 0;
static unsigned int g_afeed_cnt = 0;
static unsigned char g_afeed_err_cnt = 0;
static unsigned int g_afeed_len = 0;
static unsigned int g_vfeed_pts = 0;
static int g_audio_type = -1;
static int g_mira_vdec_started = 0;
static int g_audio_sync_thresh = 0;

static wfd_resolution_t g_mira_resolution = WFD_1080P30;

static pthread_mutex_t g_mira_mutex = PTHREAD_MUTEX_INITIALIZER;

static hudi_handle g_adec_hdl = NULL;
static hudi_handle g_snd_hdl = NULL;
static hudi_handle g_vdec_hdl = NULL;
static hudi_handle g_dis_hdl = NULL;
static hudi_handle g_avsync_hdl = NULL;

static uint8_t g_es_dump_en = 0;
static char g_es_dump_folder[64] = {0};
static unsigned int g_es_dump_cnt = 0;
static FILE *g_es_vfp = NULL;

extern hccast_mira_event_callback mira_callback;
static int g_video_first_showed = 0;

int hccast_mira_set_default_resolution(int res)
{
    g_mira_resolution = res;
    miracast_set_resolution(g_mira_resolution);

    return 0;
}

static void *_hccast_mira_player_state_timer(void *arg)
{
    (void) arg;

    while (g_mira_vdec_started)
    {
        miracast_player_show_state();
        usleep(2 * 1000 * 1000);
    }

    return NULL;
}

static void *_hccast_mira_order_timer(void *arg)
{
    (void) arg;
    g_vfeed_cnt_for_order = 0;
    bool order_en = true;
    miracast_ioctl(WFD_CMD_SET_RTP_ORDER_EN, (unsigned long)&order_en, 0);

    while (order_en && g_mira_vdec_started)
    {
        if (g_vfeed_cnt_for_order >= 600)
        {
            g_vfeed_cnt_for_order = 0;
            order_en = !order_en;
            miracast_ioctl(WFD_CMD_SET_RTP_ORDER_EN, (unsigned long)&order_en, 0);
        }

        usleep(1 * 1000 * 1000);
    }

    return NULL;
}

#define _READ(__data, __idx, __size, __shift) \
    (((__uint##__size##_t) (((const unsigned char *) (__data))[__idx])) << (__shift))

#define S16BETOS16(pin) (_READ(pin, 0, 16, 8) | _READ(pin, 1, 16, 0))

static void _hccast_mira_audio_sync_thres_set()
{
    if (g_avsync_hdl)
    {
        hudi_avsync_audsync_thres_get(g_avsync_hdl, &g_audio_sync_thresh);
        hudi_avsync_audsync_thres_set(g_avsync_hdl, 300);
    }
}

static void _hccast_mira_audio_sync_thres_restore()
{
    if (g_avsync_hdl)
    {
        hudi_avsync_audsync_thres_set(g_avsync_hdl, g_audio_sync_thresh);
        g_audio_sync_thresh = 0;
    }
}

static int _hccast_mira_quick_mode_get()
{
    int quick_mode = 2;

    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_GET_MIRROR_QUICK_MODE_NUM, (void *)&quick_mode, NULL);
    }

    return quick_mode;
}

static int _hccast_mira_rotation_info_get(hccast_mira_rotation_t *roate_info)
{
    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_GET_MIRROR_ROTATION_INFO, NULL, (void*)roate_info);
    }

    return 0;
}

static void _hccast_mira_preview_info_get(hccast_mira_preview_info_t *preview_info)
{
    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_GET_PREVIEW_INFO, (void*)preview_info, NULL);
    }
}

static void _hccast_mira_video_config_get(hccast_com_video_config_t *video_config)
{
    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_GET_VIDEO_CONFIG, (void*)video_config, NULL);
    }
}

static int _hccast_mira_vdec_event_handler(hudi_handle handle, unsigned int event,
                                           void *arg, void *user_data)
{
    switch (event)
    {
        case AVEVENT_VIDDEC_FIRST_FRAME_DECODED:
        case AVEVENT_VIDDEC_FIRST_FRAME_SHOWED:
            pthread_mutex_lock(&g_mira_mutex);
            if (mira_callback && !g_video_first_showed)
            {
                g_video_first_showed = 1;
                mira_callback(HCCAST_MIRA_START_FIRST_FRAME_DISP, NULL, NULL);
            }
            pthread_mutex_unlock(&g_mira_mutex);
            break;
        default:
            break;
    }

    return 0;
}

static int _hccast_mira_display_event_handler(hudi_handle handle, unsigned int event,
                                              void *arg, void *user_data)
{
    unsigned long b_vscreen = 0;

    switch (event)
    {
        case DIS_NOTIFY_MIRACAST_VSRCEEN:
            if (mira_callback)
            {
                pthread_mutex_lock(&g_mira_mutex);
                b_vscreen = (unsigned long)arg;
                mira_callback(HCCAST_MIRA_MIRROR_SCREEN_DETECT_NOTIFY, (void*)&b_vscreen, NULL);
                pthread_mutex_unlock(&g_mira_mutex);
            }
            break;
        default:
            break;
    }

    return 0;
}

int hccast_mira_es_dump_start(char *folder)
{
    memset(g_es_dump_folder, 0, 64);
    strcat(g_es_dump_folder, folder);
    g_es_dump_en = 1;

    return 0;
}

int hccast_mira_es_dump_stop()
{
    g_es_dump_en = 0;

    return 0;
}

static int hccast_mira_video_open(void)
{
    pthread_t tid;
    pthread_t order_tid;
    pthread_attr_t thread_attr;
    struct video_config mvcfg;
    char path[128] = {0};
    int rotate = 0;
    int ret = 2;
    hccast_mira_preview_info_t preview_info = {0};
    hccast_com_video_config_t video_config = {0};
    wfd_resolution_t res = miracast_get_resolution();

    pthread_mutex_lock(&g_mira_mutex);

    if (g_vdec_hdl)
    {
        hccast_log(LL_ERROR, "Miracast already started\n");
        pthread_mutex_unlock(&g_mira_mutex);
        return -1;
    }

#ifdef HC_RTOS
    vTaskEnableGlobalTimeSlice();
    vTaskSetGlobalTimeSlice(1);
#endif

    hccast_log(LL_NOTICE, "Miracast resolution: %d\n", res);

    video_config.video_pbp_mode = HCCAST_COM_VIDEO_PBP_OFF;
    video_config.video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_HD;
    video_config.video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_MAIN;
    _hccast_mira_video_config_get(&video_config);

    memset(&mvcfg, 0, sizeof(struct video_config));
    mvcfg.codec_id = HC_AVCODEC_ID_H264;
    mvcfg.sync_mode = AVSYNC_TYPE_UPDATESTC;
    mvcfg.decode_mode = VDEC_WORK_MODE_KSHM;
    mvcfg.pixel_aspect_x = 1;
    mvcfg.pixel_aspect_y = 1;
    mvcfg.quick_mode = _hccast_mira_quick_mode_get();
    mvcfg.rotate_enable = 1;
    mvcfg.preview = 0;
    mvcfg.extradata_size = 0;
    mvcfg.kshm_size = 0x400000;
    mvcfg.pbp_mode = hccast_com_video_pbp_mode_translate(video_config.video_pbp_mode);
    mvcfg.dis_type = hccast_com_video_dis_type_translate(video_config.video_dis_type);
    mvcfg.dis_layer = hccast_com_video_dis_layer_translate(video_config.video_dis_layer);
    mvcfg.auto_audio_master = 1;

    if (WFD_2160P30 == res)
    {
        mvcfg.pic_width = 4096;
        mvcfg.pic_height = 2160;
        mvcfg.frame_rate = 60 * 1000;
    }
    else if (WFD_1080P60 == res || WFD_1080P30 == res || WFD_1080F30 == res)
    {
        mvcfg.pic_width = 1920;
        mvcfg.pic_height = 1080;
        mvcfg.frame_rate = 60 * 1000;
    }
    else if (WFD_720P60 == res || WFD_720P30 == res)
    {
        mvcfg.pic_width = 1280;
        mvcfg.pic_height = 720;
        mvcfg.frame_rate = 60 * 1000;
    }
    else if (WFD_480P60 == res)
    {
        mvcfg.pic_width = 720;
        mvcfg.pic_height = 480;
        mvcfg.frame_rate = 60 * 1000;
    }

    _hccast_mira_preview_info_get(&preview_info);
    if (preview_info.preview_en == 1)
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

    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_GET_CONTINUE_ON_ERROR, NULL, &ret);
        if (ret < 0 || ret > 2)
        {
            ret = 2;
        }
    }

    if (0 != hudi_vdec_open(&g_vdec_hdl, &mvcfg))
    {
        pthread_mutex_unlock(&g_mira_mutex);
        return -1;
    }
    
    hudi_vdec_masaic_mode_set(g_vdec_hdl, ret);
    hudi_vdec_start(g_vdec_hdl);

    if (!g_avsync_hdl)
    {
        hudi_avsync_open(&g_avsync_hdl);
    }

    _hccast_mira_audio_sync_thres_set();

    g_mira_vdec_started = 1;
    g_video_first_showed = 0;

    hudi_vdec_event_register(g_vdec_hdl, _hccast_mira_vdec_event_handler, AVEVENT_VIDDEC_FIRST_FRAME_DECODED, NULL);
    hudi_vdec_event_register(g_vdec_hdl, _hccast_mira_vdec_event_handler, AVEVENT_VIDDEC_FIRST_FRAME_SHOWED, NULL);

    if (!g_dis_hdl)
    {
        hudi_display_open(&g_dis_hdl);
    }
    if (g_dis_hdl)
    {
        hudi_display_event_register(g_dis_hdl, _hccast_mira_display_event_handler, DIS_NOTIFY_MIRACAST_VSRCEEN, NULL);
    }

    pthread_mutex_unlock(&g_mira_mutex);

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &thread_attr, _hccast_mira_player_state_timer, NULL);
    pthread_create(&order_tid, &thread_attr, _hccast_mira_order_timer, NULL);
    pthread_attr_destroy(&thread_attr);

    if (g_es_dump_en)
    {
        sprintf(path, "%s/miracast-%d.h264", g_es_dump_folder, g_es_dump_cnt);
        g_es_dump_cnt ++;

        g_es_vfp = fopen(path, "w+");
    }

    g_vfeed_pts = 0;

    return 0;
}

static void hccast_mira_video_close(void)
{
    hccast_log(LL_INFO, "func: %s\n", __func__);

#ifdef HC_RTOS
    vTaskDisableGlobalTimeSlice();
#endif

    if (g_es_vfp)
    {
        fflush(g_es_vfp);
        fclose(g_es_vfp);
        g_es_vfp = NULL;
    }

    pthread_mutex_lock(&g_mira_mutex);

    if (g_vdec_hdl)
    {
        hudi_vdec_close(g_vdec_hdl, 0);
        g_vdec_hdl = NULL;
    }

    if (g_dis_hdl)
    {
        hudi_display_close(g_dis_hdl);
        g_dis_hdl = NULL;
    }

    _hccast_mira_audio_sync_thres_restore();

    if (g_avsync_hdl)
    {
        hudi_avsync_close(g_avsync_hdl);
        g_avsync_hdl = NULL;
    }

    g_mira_vdec_started = 0;

    pthread_mutex_unlock(&g_mira_mutex);
}

static int hccast_mira_video_feed(unsigned char *data, unsigned int len, unsigned int pts,
                                  int last_slice, unsigned int width, unsigned int height)
{
    float rate = 1;
    hccast_mira_rotation_t rotate_info = {0};
    unsigned char data_aux[8] = {0};
    AvPktHd pkthd = {0};
    struct vdec_decore_status stat;

    g_vfeed_cnt ++;
    g_vfeed_len += len;

    if (g_es_vfp)
        fwrite(data, 1, len, g_es_vfp);

    bool order_en = false;
    miracast_ioctl(WFD_CMD_GET_RTP_ORDER_EN, 0, (unsigned long)&order_en);
    if (order_en)
    {
        g_vfeed_cnt_for_order ++;
    }

    pthread_mutex_lock(&g_mira_mutex);

    if (!g_vdec_hdl)
    {
        hccast_log(LL_ERROR, "Video not started\n");
        pthread_mutex_unlock(&g_mira_mutex);
        return -1;
    }

    _hccast_mira_rotation_info_get(&rotate_info);

    memset(&pkthd, 0, sizeof(pkthd));
    pkthd.dur = 0;
    pkthd.size = len;
    pkthd.flag = AV_PACKET_ES_DATA;
    pkthd.pts = pts;
    pkthd.video_rotate_mode = rotate_info.rotate_angle;
    pkthd.video_mirror_mode = rotate_info.flip_mode;
    hudi_vdec_feed(g_vdec_hdl, data, &pkthd);

    if (1)
    {
        data_aux[0] = 0x00;
        data_aux[1] = 0x00;
        data_aux[2] = 0x00;
        data_aux[3] = 0x01;
        data_aux[4] = 0x09;
        pkthd.size = 5;
        hudi_vdec_feed(g_vdec_hdl, data_aux, &pkthd);

        if (mira_callback && !g_video_first_showed)
        {
            hudi_vdec_stat_get(g_vdec_hdl, &stat);
            if (stat.frames_decoded > 0)
            {
                g_video_first_showed = 1;
                mira_callback(HCCAST_MIRA_START_FIRST_FRAME_DISP, NULL, NULL);
            }
        }
    }

    if (pts)
    {
        g_vfeed_pts = pts;
    }

    pthread_mutex_unlock(&g_mira_mutex);

    return 0;
}

static int hccast_mira_audio_open(int codec_tag)
{
    unsigned int audsink = AUDSINK_SND_DEVBIT_I2SO;
    struct snd_pcm_params snd_config;
    struct audio_config adec_config;
    unsigned int force_auddec = false;
    unsigned int audio_disable = 0;

    hccast_log(LL_INFO,  "Audio format: 0x%.x\n", codec_tag);

    hccast_com_audsink_get(&audsink);
    hccast_com_force_auddec_get(&force_auddec);
    hccast_com_audio_disable_get(&audio_disable);

    if (audio_disable)
    {
        return -1;
    }

    g_audio_type = codec_tag;
    if (g_audio_type == CODEC_ID_PCM_S16BE)
    {
        if (force_auddec)
        {
            memset(&adec_config, 0, sizeof(struct audio_config));
            adec_config.codec_id = HC_AVCODEC_ID_PCM_S16BE;
            adec_config.sync_mode = AVSYNC_TYPE_SYNCSTC;
            adec_config.bits_per_coded_sample = 16;
            adec_config.channels = 2;
            adec_config.snd_devs = audsink;
            adec_config.sample_rate = 48000;
            adec_config.kshm_size = 0xa0000;
            adec_config.auto_audio_master = 1;
            if (0 != hudi_adec_open(&g_adec_hdl, &adec_config))
            {
                return -1;
            }
            hudi_adec_start(g_adec_hdl);
        }
        else
        {

            if (g_snd_hdl)
            {
                hccast_log(LL_ERROR, "Audio already open\n");
                return -1;
            }

            if (0 != hudi_snd_open(&g_snd_hdl, audsink))
            {
                return -1;
            }

            memset(&snd_config, 0, sizeof(struct snd_pcm_params));
            snd_config.access = SND_PCM_ACCESS_RW_INTERLEAVED;
            snd_config.format = SND_PCM_FORMAT_S16_LE;
            snd_config.sync_mode = AVSYNC_TYPE_SYNCSTC;
            snd_config.rate = 48000;
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
                return -1;
            }
        } 
    }
    else
    {
        memset(&adec_config, 0, sizeof(struct audio_config));
        adec_config.codec_id = HC_AVCODEC_ID_AAC;
        adec_config.sync_mode = AVSYNC_TYPE_SYNCSTC;
        adec_config.sample_rate = 48000;
        adec_config.snd_devs = audsink;
        adec_config.bits_per_coded_sample = 16;
        adec_config.channels = 2;
        adec_config.auto_audio_master = 1;
        if (0 != hudi_adec_open(&g_adec_hdl, &adec_config))
        {
            return -1;
        }
        hudi_adec_start(g_adec_hdl);
    }

    return 0;
}

static void hccast_mira_audio_close()
{
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

    g_audio_type = -1;
}

static int hccast_mira_audio_feed(int type, unsigned char *buf, int length, unsigned int pts)
{
    AvPktHd adec_pkt;
    struct snd_xfer snd_pkt;
    unsigned short *in, *out ;
    unsigned int audio_disable = 0;
    unsigned int force_auddec = 0;
    int ret = -1;

    hccast_com_force_auddec_get(&force_auddec);
    hccast_com_audio_disable_get(&audio_disable);
    if (audio_disable)
    {
        return -1;
    }

    g_afeed_cnt ++;
    g_afeed_len += length;

    if (g_audio_type == CODEC_ID_PCM_S16BE)
    {
        if (length != 1920)
        {
            hccast_log(LL_WARNING, "Skip PCM %d\n", length);
            return -1;
        }

        if (force_auddec)
        {
            if (!g_adec_hdl)
            {
                hccast_log(LL_ERROR, "Audio not open\n");
                return -1;
            }

            memset(&adec_pkt, 0, sizeof(AvPktHd));
            adec_pkt.size = length;
            adec_pkt.pts = pts;
            adec_pkt.flag = AV_PACKET_ES_DATA;
            hudi_adec_feed(g_adec_hdl, buf, &adec_pkt);
        }
        else
        {
            if (!g_snd_hdl)
            {
                hccast_log(LL_ERROR, "Audio not open\n");
                return -1;
            }

            out = (unsigned short *)buf;
            in = (unsigned short *)buf;
            for (int i = 0; i < length / 2; i ++)
            {
                *out = S16BETOS16(in);
                out ++;
                in ++;
            }

            memset(&snd_pkt, 0, sizeof(struct snd_xfer));
            snd_pkt.data = buf;
            snd_pkt.frames = length / 4;
            snd_pkt.tstamp_ms = pts;
            hudi_snd_feed(g_snd_hdl, &snd_pkt);
        }
    }
    else
    {
        if (!g_adec_hdl)
        {
            hccast_log(LL_ERROR, "Audio not open\n");
            return -1;
        }

        memset(&adec_pkt, 0, sizeof(AvPktHd));
        adec_pkt.size = length;
        adec_pkt.pts = pts;
        adec_pkt.flag = AV_PACKET_ES_DATA;
        hudi_adec_feed(g_adec_hdl, buf, &adec_pkt);
    }

    return 0;
}

static void hccast_mira_av_state(char *s)
{
    int64_t vpts = 0;

    pthread_mutex_lock(&g_mira_mutex);

    if (g_vdec_hdl)
    {
        hudi_vdec_pts_get(g_vdec_hdl, &vpts);
    }

    pthread_mutex_unlock(&g_mira_mutex);

    bool order_en = false;
    miracast_ioctl(WFD_CMD_GET_RTP_ORDER_EN, 0, (unsigned long)&order_en);

    hccast_log(LL_NOTICE, "[FEED] V(%d:%d) A(%d:%d) P(%.8x:%.8x) O(%d) %s\n",
               g_vfeed_cnt, g_vfeed_len, g_afeed_cnt, g_afeed_len, g_vfeed_pts, (unsigned int)vpts, order_en, s);

    g_vfeed_cnt = 0;
    g_vfeed_len = 0;
    g_afeed_cnt = 0;
    g_afeed_len = 0;
}

static void hccast_mira_av_reset()
{
}

static miracast_av_func_t hccast_mira_av_driver =
{
    ._video_open = hccast_mira_video_open,
    ._video_close = hccast_mira_video_close,
    ._video_feed = hccast_mira_video_feed,
    ._audio_open = hccast_mira_audio_open,
    ._audio_close = hccast_mira_audio_close,
    ._audio_feed = hccast_mira_audio_feed,
    ._av_state = hccast_mira_av_state,
    ._av_reset = hccast_mira_av_reset,
};

static miracast_dsc_func_t hccast_mira_dsc_driver =
{
    .dsc_aes_ctr_open = hccast_dsc_aes_ctr_open,
    .dsc_aes_cbc_open = hccast_dsc_aes_cbc_open,
    .dsc_ctx_destroy  = hccast_dsc_ctx_destroy,
    .dsc_aes_decrypt  = hccast_dsc_decrypt,
    .dsc_aes_encrypt  = hccast_dsc_encrypt,
};

int mira_av_player_init(void)
{
    miracast_set_resolution(g_mira_resolution);

    miracast_ioctl(WFD_CMD_SET_DSC_FUNC, (unsigned long)&hccast_mira_dsc_driver, (unsigned long)0);
    miracast_ioctl(WFD_CMD_SET_AV_FUNC, (unsigned long)&hccast_mira_av_driver, (unsigned long)0);

#ifdef HC_RTOS
    miracast_ioctl(WFD_CMD_ENABLE_AAC, 0, 0);
#endif
    //miracast_ioctl(WFD_CMD_DISABLE_AUDIO, 1, 0);

    return 0;
}
