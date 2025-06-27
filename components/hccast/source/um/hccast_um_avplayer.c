#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>

#include <hudi/hudi_com.h>
#include <hudi/hudi_vdec.h>
#include <hudi/hudi_audsink.h>
#include <hudi/hudi_snd.h>
#include <hudi/hudi_adec.h>
#include <hudi/hudi_avsync.h>
#include <hudi/hudi_display.h>

#ifdef HC_RTOS
#include <um/iumirror_api.h>
#include <um/aumirror_api.h>
#else
#include <hccast/iumirror_api.h>
#include <hccast/aumirror_api.h>
#endif

#include <hccast_com.h>
#include <hccast_log.h>
#include <hccast_av.h>

#include <hccast_um.h>
#include "hccast_um_avplayer.h"
#include "hccast_um_api.h"

static aum_screen_mode_t g_aum_screen_mode;
static unsigned int g_um_type = 0;
static unsigned int g_video_width = 0;
static unsigned int g_video_height = 0;
static unsigned int g_rotate_mode = 0xFF;
static unsigned int g_video_last_tick = 0;
static unsigned int g_vfeed_cnt = 0;
static unsigned int g_vfeed_len = 0;
static unsigned int g_afeed_cnt = 0;
static unsigned int g_afeed_len = 0;
static unsigned int g_video_framerate = 0;
static unsigned int g_play_speed = 0;
static unsigned char g_play_mode = UM_PLAY_MODE_NORMAL;
static int g_vdec_started = 0;
static int g_audio_started = 0;
static int g_dis_backup = 0;
static int g_ium_audio_mute = 0;

static hudi_handle g_vdec_hdl = NULL;
static hudi_handle g_adec_hdl = NULL;
static hudi_handle g_avsync_hdl = NULL;
static hudi_handle g_dis_hdl = NULL;

static pthread_mutex_t g_um_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_vdec_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_adec_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_stat_mutex = PTHREAD_MUTEX_INITIALIZER;

static uint8_t g_es_dump_en = 0;
static char g_es_dump_folder[64] = {0};
static unsigned int g_es_dump_cnt = 0;
static FILE *g_es_vfp = NULL;

extern um_ioctl ium_api_ioctl;
extern um_ioctl aum_api_ioctl;
extern hccast_um_cb g_ium_evt_cb;
extern hccast_um_cb g_aum_evt_cb;

static inline unsigned int hccast_um_get_tick(void)
{
    unsigned int cur_tick = 0;
    struct timespec time;

    clock_gettime(CLOCK_REALTIME, &time);

    cur_tick = (time.tv_sec * 1000) + (time.tv_nsec / 1000000);

    return cur_tick;
}

static void *hccast_um_player_state_timer(void *args)
{
    struct vdec_decore_status stat;

    while (g_vdec_started)
    {
        pthread_mutex_lock(&g_vdec_mutex);
        if (g_vdec_hdl)
        {
            hudi_vdec_stat_get(g_vdec_hdl, &stat);
        }
        pthread_mutex_unlock(&g_vdec_mutex);

        if (g_um_type == UM_TYPE_IUM)
        {
            if (!g_afeed_cnt)
            {
                if (!g_ium_audio_mute)
                {
                    g_ium_audio_mute = 1;
                    g_ium_evt_cb(HCCAST_IUM_EVT_AUDIO_NO_DATA, (void*)1, NULL);
                }
            }
            else
            {
                if (g_ium_audio_mute)
                {
                    g_ium_audio_mute = 0;
                    g_ium_evt_cb(HCCAST_IUM_EVT_AUDIO_NO_DATA, (void*)0, NULL);
                }
            }
        }

        pthread_mutex_lock(&g_stat_mutex);

        hccast_log(LL_NOTICE, "[FEED] V(%d:%d) A(%d:%d) FPS(%d:%d)\n", g_vfeed_cnt, g_vfeed_len,
                   g_afeed_cnt, g_afeed_len, 0, stat.frames_decoded);
        g_vfeed_cnt = 0;
        g_vfeed_len = 0;
        g_afeed_cnt = 0;
        g_afeed_len = 0;

        pthread_mutex_unlock(&g_stat_mutex);

        sleep(2);
    }

    return NULL;
}

static void hccast_um_get_rotate_info(hccast_um_rotate_info_t *rotate_info)
{
    if (UM_TYPE_AUM == g_um_type)
    {
        if (g_aum_evt_cb)
        {
            hccast_aum_screen_mode_t screen_mode;
            screen_mode.mode = g_aum_screen_mode.mode;
            screen_mode.screen_height = g_aum_screen_mode.screen_height;
            screen_mode.screen_width = g_aum_screen_mode.screen_width;
            screen_mode.video_height = g_aum_screen_mode.video_height;
            screen_mode.video_width = g_aum_screen_mode.video_width;
            g_aum_evt_cb(HCCAST_AUM_EVT_GET_ROTATION_INFO, (void*)&screen_mode, (void*)rotate_info);
        }
    }
    else
    {
        if (g_ium_evt_cb)
        {
            hccast_ium_screen_mode_t screen_mode;
            screen_mode.rotate_mode = g_rotate_mode;
            screen_mode.video_height = g_video_height;
            screen_mode.video_width = g_video_width;
            g_ium_evt_cb(HCCAST_IUM_EVT_GET_ROTATION_INFO, (void*)&screen_mode, (void*)rotate_info);
        }
    }
}

static int hccast_um_get_preview_info(int um_type, hccast_um_preview_info_t *preview_info)
{

    if (UM_TYPE_AUM == um_type)
    {
        hccast_aum_screen_mode_t screen_mode;
        screen_mode.mode = g_aum_screen_mode.mode;
        screen_mode.screen_height = g_aum_screen_mode.screen_height;
        screen_mode.screen_width = g_aum_screen_mode.screen_width;
        screen_mode.video_height = g_aum_screen_mode.video_height;
        screen_mode.video_width = g_aum_screen_mode.video_width;
        g_aum_evt_cb(HCCAST_AUM_EVT_GET_PREVIEW_INFO, (void*)preview_info, (void*)&screen_mode);
    }
    else
    {
        g_ium_evt_cb(HCCAST_IUM_EVT_GET_PREVIEW_INFO, (void*)preview_info, NULL);
    }

    return 0;
}

static int hccast_um_get_video_config(int um_type, hccast_com_video_config_t *video_config)
{

    if (UM_TYPE_AUM == um_type)
    {
        g_aum_evt_cb(HCCAST_AUM_EVT_GET_VIDEO_CONFIG, (void*)video_config, NULL);
    }
    else
    {
        g_ium_evt_cb(HCCAST_IUM_EVT_GET_VIDEO_CONFIG, (void*)video_config, NULL);
    }

    return 0;
}

static void hccast_um_video_aspect_set()
{
    pthread_mutex_lock(&g_um_mutex);

    if (UM_TYPE_AUM == g_um_type)
    {
        if (g_aum_evt_cb)
        {
            hccast_aum_screen_mode_t screen_mode;
            screen_mode.mode = g_aum_screen_mode.mode;
            screen_mode.screen_height = g_aum_screen_mode.screen_height;
            screen_mode.screen_width = g_aum_screen_mode.screen_width;
            screen_mode.video_height = g_aum_screen_mode.video_height;
            screen_mode.video_width = g_aum_screen_mode.video_width;
            g_aum_evt_cb(HCCAST_AUM_EVT_SET_DIS_ASPECT, (void*)&screen_mode, NULL);
        }
    }
    else
    {
        if (g_ium_evt_cb)
        {
            g_ium_evt_cb(HCCAST_IUM_EVT_SET_DIS_ASPECT, NULL, NULL);
        }
    }

    pthread_mutex_unlock(&g_um_mutex);
}

static void hccast_um_video_restart(int play_mode)
{
    struct video_config vdec_config;
    hccast_um_preview_info_t preview_info = {0};
    hccast_com_video_config_t video_config = {0};

    hccast_log(LL_DEBUG, "[%s - %d]\n", __func__, __LINE__);

    pthread_mutex_lock(&g_vdec_mutex);

    if (g_vdec_hdl)
    {
        hudi_vdec_close(g_vdec_hdl, 1);
        g_vdec_hdl = NULL;
        if (g_dis_hdl)
        {
            hudi_display_pic_backup(g_dis_hdl, DIS_TYPE_HD);
            g_dis_backup = 1;
        }
    }
    
    video_config.video_pbp_mode = HCCAST_COM_VIDEO_PBP_OFF;
    video_config.video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_HD;
    video_config.video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_MAIN;
    hccast_um_get_preview_info(g_um_type, &preview_info);
    hccast_um_get_video_config(g_um_type, &video_config);

    memset(&vdec_config, 0, sizeof(struct video_config));
    vdec_config.codec_id = HC_AVCODEC_ID_H264;
    vdec_config.sync_mode = AVSYNC_TYPE_UPDATESTC;
    vdec_config.decode_mode = VDEC_WORK_MODE_KSHM;
    vdec_config.pic_width = 1920;
    vdec_config.pic_height = 1080;
    vdec_config.pixel_aspect_x = 1;
    vdec_config.pixel_aspect_y = 1;
    vdec_config.preview = 0;
    vdec_config.extradata_size = 0;
    vdec_config.frame_rate = 60 * 1000;
    vdec_config.quick_mode = 2;
    vdec_config.rotate_enable = 1;
    vdec_config.kshm_size = 0x400000;
    vdec_config.pbp_mode = hccast_com_video_pbp_mode_translate(video_config.video_pbp_mode);
    vdec_config.dis_type = hccast_com_video_dis_type_translate(video_config.video_dis_type);
    vdec_config.dis_layer = hccast_com_video_dis_layer_translate(video_config.video_dis_layer);

    if (preview_info.preview_en == 1)
    {
        vdec_config.preview = 1;
        vdec_config.src_area.x = preview_info.src_rect.x;
        vdec_config.src_area.y = preview_info.src_rect.y;
        vdec_config.src_area.w = preview_info.src_rect.w;
        vdec_config.src_area.h = preview_info.src_rect.h;

        vdec_config.dst_area.x = preview_info.dst_rect.x;
        vdec_config.dst_area.y = preview_info.dst_rect.y;
        vdec_config.dst_area.w = preview_info.dst_rect.w;
        vdec_config.dst_area.h = preview_info.dst_rect.h;
    }

    if (UM_PLAY_MODE_STREAM == play_mode)
    {
        vdec_config.sync_mode = AVSYNC_TYPE_SYNCSTC;
        vdec_config.buffering_start = 0;
        vdec_config.buffering_end = 1000;
        vdec_config.quick_mode = 0;
    }

    if (0 != hudi_vdec_open(&g_vdec_hdl, &vdec_config))
    {
        hccast_log(LL_ERROR, "Open vdec fail\n");
        pthread_mutex_unlock(&g_vdec_mutex);
        return;
    }

    hudi_vdec_masaic_mode_set(g_vdec_hdl, 2);
    hudi_vdec_start(g_vdec_hdl);

    g_video_framerate = 0;
    g_play_speed = 0;
    pthread_mutex_unlock(&g_vdec_mutex);
}

static void hccast_um_audio_restart(int play_mode)
{
    struct audio_config macfg;
    unsigned int audsink = AUDSINK_SND_DEVBIT_I2SO;
    unsigned int audio_disable = 0;

    hccast_log(LL_DEBUG, "[%s - %d]\n", __func__, __LINE__);

    hccast_com_audsink_get(&audsink);
    hccast_com_audio_disable_get(&audio_disable);
    if (audio_disable)
    {   
        return ;
    }
    
    pthread_mutex_lock(&g_adec_mutex);
    if (g_adec_hdl)
    {
        hudi_adec_close(g_adec_hdl);
        g_adec_hdl = NULL;
    }

    memset(&macfg, 0, sizeof(struct audio_config));
    macfg.codec_id = HC_AVCODEC_ID_PCM_S16LE;
    macfg.sync_mode = AVSYNC_TYPE_FREERUN;
    macfg.bits_per_coded_sample = 16;
    macfg.channels = 2;
    macfg.sample_rate = 48000;
    macfg.audio_flush_thres = 200;
    macfg.kshm_size = 0xa0000;
    macfg.snd_devs = audsink;
    
    if (0 != hudi_adec_open(&g_adec_hdl, &macfg))
    {
        pthread_mutex_unlock(&g_adec_mutex);
        return ;
    }

    if (0 != hudi_adec_start(g_adec_hdl))
    {
        hudi_adec_close(g_adec_hdl);
        g_adec_hdl = NULL;
        pthread_mutex_unlock(&g_adec_mutex);
        return ;
    }

    pthread_mutex_unlock(&g_adec_mutex);
}

int hccast_um_es_dump_start(char *folder)
{
    memset(g_es_dump_folder, 0, 64);
    strcat(g_es_dump_folder, folder);
    g_es_dump_en = 1;

    return 0;
}

int hccast_um_es_dump_stop()
{
    g_es_dump_en = 0;

    return 0;
}

static int hccast_um_video_open(int um_type)
{
    struct video_config vdec_config;
    pthread_t tid;
    char path[128] = {0};
    pthread_attr_t thread_attr;
    hccast_um_preview_info_t preview_info = {0};
    hccast_com_video_config_t video_config = {0};

    hccast_log(LL_DEBUG, "[%s - %d]\n", __func__, __LINE__);

#ifdef HC_RTOS
    vTaskEnableGlobalTimeSlice();
    vTaskSetGlobalTimeSlice(1);
#endif

    pthread_mutex_lock(&g_vdec_mutex);
    if (g_vdec_started)
    {
        hccast_log(LL_WARNING, "Warning: aircast video has been started!\n");
    }

    g_um_type = um_type;

    if (g_vdec_hdl)
    {
        hudi_vdec_close(g_vdec_hdl, 0);
        g_vdec_hdl = NULL;
    }

    g_video_width = 0;
    g_video_height = 0;
    g_video_framerate = 0;
    g_play_speed = 0;
    g_video_last_tick = 0;
    video_config.video_pbp_mode = HCCAST_COM_VIDEO_PBP_OFF;
    video_config.video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_HD;
    video_config.video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_MAIN;
    hccast_um_get_preview_info(um_type, &preview_info);
    hccast_um_get_video_config(um_type, &video_config);

    memset(&vdec_config, 0, sizeof(struct video_config));
    vdec_config.codec_id = HC_AVCODEC_ID_H264;
    vdec_config.sync_mode = AVSYNC_TYPE_UPDATESTC;
    vdec_config.decode_mode = VDEC_WORK_MODE_KSHM;
    vdec_config.pic_width = 1920;
    vdec_config.pic_height = 1080;
    vdec_config.pixel_aspect_x = 1;
    vdec_config.pixel_aspect_y = 1;
    vdec_config.preview = 0;
    vdec_config.extradata_size = 0;
    vdec_config.frame_rate = 60 * 1000;
    vdec_config.quick_mode = 2;
    vdec_config.rotate_enable = 1;
    vdec_config.kshm_size = 0x400000;
    vdec_config.pbp_mode = hccast_com_video_pbp_mode_translate(video_config.video_pbp_mode);
    vdec_config.dis_type = hccast_com_video_dis_type_translate(video_config.video_dis_type);
    vdec_config.dis_layer = hccast_com_video_dis_layer_translate(video_config.video_dis_layer);

    if (preview_info.preview_en == 1)
    {
        vdec_config.preview = 1;
        vdec_config.src_area.x = preview_info.src_rect.x;
        vdec_config.src_area.y = preview_info.src_rect.y;
        vdec_config.src_area.w = preview_info.src_rect.w;
        vdec_config.src_area.h = preview_info.src_rect.h;

        vdec_config.dst_area.x = preview_info.dst_rect.x;
        vdec_config.dst_area.y = preview_info.dst_rect.y;
        vdec_config.dst_area.w = preview_info.dst_rect.w;
        vdec_config.dst_area.h = preview_info.dst_rect.h;
    }

    if (0 != hudi_vdec_open(&g_vdec_hdl, &vdec_config))
    {
        hccast_log(LL_ERROR, "hudi vdec open fail\n");
        pthread_mutex_unlock(&g_vdec_mutex);
        return -1;
    }

    hudi_vdec_masaic_mode_set(g_vdec_hdl, 2);                  
    hudi_vdec_start(g_vdec_hdl);

    hccast_um_video_aspect_set();

    if (!g_dis_hdl)
    {
        hudi_display_open(&g_dis_hdl);
    }

    g_vdec_started = 1;

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &thread_attr, hccast_um_player_state_timer, NULL);

    pthread_attr_destroy(&thread_attr);

    if (g_es_dump_en)
    {
        sprintf(path, "%s/um-%d.h264", g_es_dump_folder, g_es_dump_cnt);
        g_es_dump_cnt ++;

        g_es_vfp = fopen(path, "w+");
    }
    pthread_mutex_unlock(&g_vdec_mutex);
    return 0;
}

static void hccast_um_video_close()
{
    hccast_log(LL_DEBUG, "[%s - %d]\n", __func__, __LINE__);

#ifdef HC_RTOS
    vTaskDisableGlobalTimeSlice();
#endif

    pthread_mutex_lock(&g_vdec_mutex);
    if (g_es_vfp)
    {
        fflush(g_es_vfp);
        fclose(g_es_vfp);
        g_es_vfp = NULL;
    }

    if (!g_vdec_started)
    {
        hccast_log(LL_DEBUG, "[%s - %d] video has been close.\n", __func__, __LINE__);
        pthread_mutex_unlock(&g_vdec_mutex);
        return ;
    }

    if (g_vdec_hdl)
    {
        hudi_vdec_close(g_vdec_hdl, 0);
        g_vdec_hdl = NULL;
    }

    if (g_dis_backup && g_dis_hdl)
    {
        hudi_display_pic_free(g_dis_hdl, DIS_TYPE_HD);
    }

    if (g_dis_hdl)
    {
        hudi_display_close(g_dis_hdl);
        g_dis_hdl = NULL;
    }

    memset(&g_aum_screen_mode, 0, sizeof(g_aum_screen_mode));
    g_vdec_started = 0;
    g_video_width = 0;
    g_video_height = 0;
    g_rotate_mode = 0xFF;
    g_um_type = 0;
    g_play_mode = UM_PLAY_MODE_NORMAL;
    pthread_mutex_unlock(&g_vdec_mutex);
}

static int hccast_um_video_feed(unsigned char *data, unsigned int len,
                                unsigned long long pts, unsigned int rotate, int last_slice,
                                unsigned int width, unsigned int height, unsigned char play_mode)
{
    AvPktHd packet;
    hccast_um_rotate_info_t rotate_info = {0};
    int need_restart_decoder = 0;
    int resolution_changed = 0;
    unsigned char data_aux[5];

    g_video_last_tick = hccast_um_get_tick();

    if (g_es_vfp)
        fwrite(data, 1, len, g_es_vfp);

    if (!g_vdec_started)
    {
        hccast_log(LL_WARNING, "Video is not started\n");
        return -1;
    }

    pthread_mutex_lock(&g_stat_mutex);
    g_vfeed_cnt ++;
    g_vfeed_len += len;
    pthread_mutex_unlock(&g_stat_mutex);

    if ((0xFF != play_mode) && (play_mode != g_play_mode) && (g_um_type == UM_TYPE_IUM))
    {
        hccast_log(LL_NOTICE, "Play mode change %.2x\n", play_mode);
        g_play_mode = play_mode;
        need_restart_decoder = 1;
        hccast_um_audio_restart(g_play_mode);
    }

    if (width && (g_video_width != width))
    {
        if (g_video_width)
        {
            resolution_changed = 1;
        }
        g_video_width = width;

        hccast_log(LL_NOTICE, "Video width: %d\n", width);
    }
    if (height && (g_video_height != height))
    {
        if (g_video_height)
        {
            resolution_changed = 1;
        }
        g_video_height = height;

        hccast_log(LL_NOTICE, "Video height: %d\n", height);
    }

    if (need_restart_decoder || (resolution_changed && g_play_mode != UM_PLAY_MODE_STREAM))
    {
        hccast_um_video_restart(g_play_mode);
    }

    if (resolution_changed || need_restart_decoder)
    {
        //usb air mirror set full screen while video vertical output
        hccast_log(LL_NOTICE, "width: %d, height: %d\n", g_video_width, g_video_height);
    }

    if (UM_PLAY_MODE_STREAM == g_play_mode)
    {
        rotate = 0;
    }

    //Set rotation mode
    if ((rotate != g_rotate_mode) && (rotate != 0xFF))
    {
        g_rotate_mode = rotate;
        hccast_log(LL_NOTICE, "Change rotate %d\n", rotate);
    }

    pthread_mutex_lock(&g_vdec_mutex);
    if (!g_vdec_hdl)
    {
        pthread_mutex_unlock(&g_vdec_mutex);
        return -1;
    }

    hccast_um_get_rotate_info(&rotate_info);

    memset(&packet, 0, sizeof(packet));
    packet.dur = 0;
    packet.size = len;
    packet.flag = AV_PACKET_ES_DATA;
    packet.pts = pts;
    packet.video_rotate_mode = rotate_info.rotate_angle;
    packet.video_mirror_mode = rotate_info.flip_mode;
    hudi_vdec_feed(g_vdec_hdl, data, &packet);

    if (last_slice)
    {
        data_aux[0] = 0x00;
        data_aux[1] = 0x00;
        data_aux[2] = 0x00;
        data_aux[3] = 0x01;
        data_aux[4] = 0x09;
        packet.size = 5;
        hudi_vdec_feed(g_vdec_hdl, data_aux, &packet);
    }

    pthread_mutex_unlock(&g_vdec_mutex);

    return 0;
}

static void hccast_um_video_rotate(int rotate_en)
{
    hccast_um_video_aspect_set();
}

static void hccast_um_video_mode(int mode)
{
    hccast_um_video_aspect_set();
}

static void hccast_aum_screen_mode(aum_screen_mode_t *screen_mode)
{
    memcpy(&g_aum_screen_mode, screen_mode, sizeof(g_aum_screen_mode));

    if (g_vdec_hdl)
    {
        hccast_um_video_restart(0);
        hccast_um_video_aspect_set();
    }
}

static int hccast_um_audio_open()
{
    struct audio_config macfg;
    unsigned int audsink = AUDSINK_SND_DEVBIT_I2SO;
    unsigned int audio_disable = 0;

    hccast_log(LL_DEBUG, "[%s - %d]\n", __func__, __LINE__);

    hccast_com_audsink_get(&audsink);
    hccast_com_audio_disable_get(&audio_disable);
    pthread_mutex_lock(&g_adec_mutex);
    if (audio_disable)
    {
        pthread_mutex_unlock(&g_adec_mutex);       
        return -1;
    }
    
    if (g_audio_started)
    {
        hccast_log(LL_WARNING, "Warning: Audio has been started!\n");
        pthread_mutex_unlock(&g_adec_mutex);
        return -1;
    }

    if (g_adec_hdl)
    {
        hudi_adec_close(g_adec_hdl);
        g_adec_hdl = NULL;
    }

    memset(&macfg, 0, sizeof(struct audio_config));
    macfg.codec_id = HC_AVCODEC_ID_PCM_S16LE;
    macfg.sync_mode = AVSYNC_TYPE_FREERUN;
    macfg.bits_per_coded_sample = 16;
    macfg.channels = 2;
    macfg.sample_rate = 48000;
    macfg.audio_flush_thres = 200;
    macfg.kshm_size = 0xa0000;
    macfg.snd_devs = audsink;

    if (0 != hudi_adec_open(&g_adec_hdl, &macfg))
    {
        pthread_mutex_unlock(&g_adec_mutex);
        return -1;
    }

    if (0 != hudi_adec_start(g_adec_hdl))
    {
        hudi_adec_close(g_adec_hdl);
        g_adec_hdl = NULL;
        pthread_mutex_unlock(&g_adec_mutex);
        return -1;
    }

    if (!g_avsync_hdl)
    {
        hudi_avsync_open(&g_avsync_hdl);
    }

    g_audio_started = 1;

    pthread_mutex_unlock(&g_adec_mutex);

    return 0;
}

static void hccast_um_audio_close()
{
    hccast_log(LL_DEBUG, "[%s - %d]\n", __func__, __LINE__);

    pthread_mutex_lock(&g_adec_mutex);
    if (!g_audio_started)
    {
        hccast_log(LL_DEBUG, "[%s - %d] audio has been close.\n", __func__, __LINE__);
        pthread_mutex_unlock(&g_adec_mutex);
        return ;
    }

    if (g_adec_hdl)
    {
        hudi_adec_close(g_adec_hdl);
        g_adec_hdl = NULL;
    }

    if (g_avsync_hdl)
    {
        if (g_play_speed != 0)
        {
            hudi_avsync_stc_rate_set(g_avsync_hdl, 1);
            g_play_speed = 0;
        }
        
        hudi_avsync_close(g_avsync_hdl);
        g_avsync_hdl = NULL;
    }

    g_audio_started = 0;
    pthread_mutex_unlock(&g_adec_mutex);
}

static int hccast_um_audio_feed(int type, unsigned char *buf, int length, unsigned long long pts)
{
    AvPktHd pkt;
    unsigned int audio_disable = 1;
    int ret = 0;

    hccast_com_audio_disable_get(&audio_disable);
    if (audio_disable)
    {
        return -1;
    }

    pthread_mutex_lock(&g_adec_mutex);
    if (!g_audio_started )
    {
        hccast_log(LL_NOTICE, "Audio is not started\n");
        pthread_mutex_unlock(&g_adec_mutex);
        return -1;
    }

    pthread_mutex_lock(&g_stat_mutex);
    g_afeed_cnt ++;
    g_afeed_len += length;
    pthread_mutex_unlock(&g_stat_mutex);

    if (!g_adec_hdl)
    {
        pthread_mutex_unlock(&g_adec_mutex);
        return -1;
    }

    memset(&pkt, 0, sizeof(AvPktHd));
    pkt.pts = pts;
    pkt.dur = 0;
    pkt.size = length;
    pkt.flag = AV_PACKET_ES_DATA;
    ret = hudi_adec_feed(g_adec_hdl, buf, &pkt);

    pthread_mutex_unlock(&g_adec_mutex);

    return ret;
}

static void hccast_um_set_speed()
{
    float speed = 0.0;
    int invalid_speed = 0;

    if ((0 == g_play_speed) || !g_avsync_hdl)
    {
        return ;
    }

    switch (g_play_speed)
    {
        case 0x3FD0:
            speed = 0.25;
            break;
        case 0x3FE0:
            speed = 0.5;
            break;
        case 0x3FE8:
            speed = 0.75;
            break;
        case 0x3FF0:
            speed = 1.0;
            break;
        case 0x3FF4:
            speed = 1.25;
            break;
        case 0x3FF8:
            speed = 1.5;
            break;
        case 0x3FFC:
            speed = 1.75;
            break;
        case 0x4000:
            speed = 2.0;
            break;
        default:
            invalid_speed = 1;
            break;
    }

    if (invalid_speed)
    {
        return ;
    }

    hudi_avsync_stc_rate_set(g_avsync_hdl, speed);
}

static void hccast_um_set_framerate()
{
    if (!g_video_framerate || !g_vdec_hdl)
    {
        return ;
    }

    if (g_play_speed <= 0x3FF0)
    {
        hudi_vdec_fps_set(g_vdec_hdl, g_video_framerate * 1000);
    }
    else
    {
        if (g_video_framerate <= 30)
        {
            hudi_vdec_fps_set(g_vdec_hdl, 60 * 1000);
        }
        else
        {
            hudi_vdec_fps_set(g_vdec_hdl, 120 * 1000);
        }
    }
}

static void hccast_um_set_timebase(unsigned int time_ms, unsigned int speed)
{
    unsigned int percent = 0;

    pthread_mutex_lock(&g_vdec_mutex);
    if (g_vdec_hdl)
    {
        hudi_vdec_waterline_get(g_vdec_hdl, &percent);
    }

    if (g_avsync_hdl)
    {
        hccast_log(LL_NOTICE, "pt: %d, percent: %d\n", time_ms, percent);
        hudi_avsync_stc_set(g_avsync_hdl, time_ms);
    }

    if (g_play_speed != speed)
    {
        g_play_speed = speed;
        hccast_um_set_speed();
        hccast_um_set_framerate();
    }

    pthread_mutex_unlock(&g_vdec_mutex);
}

static void hccast_um_av_reset()
{
    hccast_log(LL_NOTICE, "[%s - %d]\n", __func__, __LINE__);
    hccast_um_video_restart(g_play_mode);
    hccast_um_audio_restart(g_play_mode);
}

static void hccast_um_video_pause(int pause)
{
    pthread_mutex_lock(&g_vdec_mutex);
    if (!g_vdec_hdl)
    {
        pthread_mutex_unlock(&g_vdec_mutex);
        return ;
    }

    hccast_log(LL_NOTICE, "[%s - %d] %d\n", __func__, __LINE__, pause);

    if (pause)
    {
        hudi_vdec_pause(g_vdec_hdl);
    }
    else
    {
        hudi_vdec_start(g_vdec_hdl);
    }

    pthread_mutex_unlock(&g_vdec_mutex);
}

static void hccast_um_set_video_fps(unsigned int fps)
{
    if (g_video_framerate != fps)
    {
        g_video_framerate = fps;
        pthread_mutex_lock(&g_vdec_mutex);
        hccast_um_set_framerate();
        pthread_mutex_unlock(&g_vdec_mutex);
    }
}

int hccast_um_reset_video(void)
{
    if (g_um_type == UM_TYPE_IUM)
    {
        if (ium_api_ioctl && (hccast_um_get_tick() - g_video_last_tick > 100) && (g_play_mode != UM_PLAY_MODE_STREAM))
        {
            ium_api_ioctl(IUM_CMD_RESET_MIRRORING, NULL, NULL);
        }
    }
    else if (g_um_type == UM_TYPE_AUM)
    {
        if (aum_api_ioctl)
        {
            hccast_um_param_t *um_param = hccast_um_inter_param_get();
            aum_api_ioctl(AUM_CMD_SETTINGS_CHANGE, (void*)um_param->screen_rotate_auto, (void*)um_param->full_screen_en);
            aum_api_ioctl(AUM_CMD_SET_VIDEO_ROTATE, (void*)um_param->screen_rotate_en, NULL);
        }
    }

    return 0;
}

ium_av_func_t ium_av_func =
{
    ._video_open    = hccast_um_video_open,
    ._video_close   = hccast_um_video_close,
    ._video_feed    = hccast_um_video_feed,
    ._video_rotate  = hccast_um_video_rotate,
    ._audio_open    = hccast_um_audio_open,
    ._audio_close   = hccast_um_audio_close,
    ._audio_feed    = hccast_um_audio_feed,
    ._set_timebase  = hccast_um_set_timebase,
    ._av_reset      = hccast_um_av_reset,
    ._video_pause   = hccast_um_video_pause,
    ._set_fps       = hccast_um_set_video_fps,
};

aum_av_func_t aum_av_func =
{
    ._video_open    = hccast_um_video_open,
    ._video_close   = hccast_um_video_close,
    ._video_feed    = hccast_um_video_feed,
    ._video_rotate  = hccast_um_video_rotate,
    ._video_mode    = hccast_um_video_mode,
    ._screen_mode   = hccast_aum_screen_mode,
    ._audio_open    = hccast_um_audio_open,
    ._audio_close   = hccast_um_audio_close,
    ._audio_feed    = hccast_um_audio_feed,
};
