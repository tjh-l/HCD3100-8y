#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#ifdef HC_RTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#else
#include <sys/msg.h>
#include <sys/ipc.h>
#endif

#include <hudi/hudi_com.h>
#include <hudi/hudi_snd.h>
#include <hudi/hudi_display.h>
#include <hudi/hudi_vdec.h>
#include <hudi/hudi_vidsink.h>

#include <hccast_com.h>
#include <hccast_media.h>
#include <hccast_scene.h>
#include <hccast_av.h>
#include <hccast_log.h>

static hccast_media_player_t *g_mediaplayer_t = NULL;
static hccast_media_event_callback mp_func = NULL;
static hccast_media_air_event_callback air_func = NULL;
static hccast_media_dial_event_callback dial_func = NULL;
static int msgkey = 0;
static int msgkey1 = 0;
static hccast_media_url_mode_e player_url_mode = HCCAST_MEDIA_URL_INVALID;
static int g_media_type = HCCAST_MEDIA_INVALID;
static hccast_media_yt_playlist_t yt_playlist[2] = {0};
static pthread_mutex_t g_seek_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_seek_cond = PTHREAD_COND_INITIALIZER;
static pthread_t g_seek_tid = 0;  
static int g_seek_thread_exit = 0;
static int g_seek_position = -1;
static int g_seek_by_key = 0;
static int g_seek_update = 0;
static int g_seek_first = 0;
static int g_seek_thread_en = 1;

#define HCCAST_MEDIA_SETBIT(x,bit) (x |= (1<<bit))
#define HCCAST_MEDIA_CLRBIT(x,bit) (x &=(~(1<<bit)))

#define MEDIA_PLAYER (1<<31)
#define MEDIA_PLAYER1 (0<<31)

#if (!defined(SUPPORT_DLNA) && defined(SUPPORT_AIRCAST) && defined(AIRCAST_SUPPORT_MIRROR_ONLY)) || \
    (!defined(SUPPORT_DLNA) && !defined(SUPPORT_AIRCAST))

void hccast_media_reset_aspect_mode(void)
{
}

void hccast_media_air_event_init(hccast_media_air_event_callback air_cb)
{
}

void hccast_media_state_init(void)
{
}

void hccast_media_callback_event(hccast_media_event_e event_type, void* param)
{
}

void hccast_media_seek(int64_t position)
{
}

int hccast_media_get_volume(void)
{
    return 0;
}

void hccast_media_set_volume(int vol)
{
}

long hccast_media_get_duration(void)
{
    return 0;
}

void hccast_media_set_duration(void)
{
}

long hccast_media_get_position(void)
{
}

int hccast_media_get_status(void)
{
    return 0;
}

void hccast_media_resume(void)
{
}

void hccast_media_pause(void)
{
}

void hccast_media_stop(void)
{
}

void hccast_media_playbackend(int player_msgkey)
{
}

void hccast_media_double_url_init_args(hccast_media_url_t *mp_url)
{
}

void hccast_media_init_arg(hccast_media_url_t *mp_url)
{
}

void hccast_media_seturl(hccast_media_url_t *mp_url)
{
}

void hccast_media_status_callback(void* msg_arg, hccast_media_player_t *p)
{
}

static void *hccast_media_status_thread(void *args)
{
}

void hccast_media_ytb_playlist_buf_reset(void)
{
}

int hccast_media_yt_check_m3u8_status(hccast_media_yt_playlist_t* yt_playlist)
{
    return 0;
}

int hccast_media_yt_playlist_read(void * opaque, uint8_t *buf, int bufsize)
{
    return 0;
}

void hccast_media_yt_update_m3u8_playlist(int id, char* m3u8, int m3u8_size)
{
}

int hccast_media_yt_set_m3u8_playlist(int id, hccast_media_yt_m3u8_t* m3u8_info)
{
    return 0;
}

void hccast_media_yt_playlist_init(void)
{
}

void hccast_media_destroy(void)
{
}

void hccast_media_init(hccast_media_event_callback mp_cb)
{
}

void *hccast_media_player_get(void)
{
    return NULL;
}

void hccast_media_stop_by_key(void)
{
}

void hccast_media_seek_by_key(int64_t position)
{
}

void hccast_media_pause_by_key(void)
{
}

void hccast_media_resume_by_key(void)
{
}

#else
#include <ffplayer.h>
#include <ffplayer_manager.h>

//call VIDSINK_DISABLE_IMG_EFFECT command to release
//the picture effect buffer.
int hccast_meida_pic_effect_enable(bool enable)
{
    hudi_handle vidsink_hdl = NULL;

    if (hudi_vidsink_open(&vidsink_hdl) != 0)
    {
        return -1;
    }

    if (enable)
    {
        hudi_vidsink_img_effect_set(vidsink_hdl, 1);
    }
    else
    {
        hudi_vidsink_img_effect_set(vidsink_hdl, 0);
    }

    if (vidsink_hdl)
    {
        hudi_vidsink_close(vidsink_hdl);
        vidsink_hdl = NULL;
    }

    return 0;
}

static void hccast_media_pic_aspect_set()
{
    hudi_handle hdl = NULL;
    dis_aspect_mode_t aspect_info = {0};

    if (0 != hudi_display_open(&hdl))
    {
        return ;
    }

    aspect_info.distype = DIS_TYPE_HD;
    aspect_info.dis_mode = DIS_NORMAL_SCALE;
    aspect_info.tv_mode = DIS_TV_AUTO;
    aspect_info.active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
    hudi_display_aspect_set(hdl, &aspect_info);

    hudi_display_close(hdl);
}

void hccast_media_reset_aspect_mode()
{
    hudi_handle hdl = NULL;
    dis_zoom_t zoom_info = {0};
    dis_aspect_mode_t aspect_info = {0};

    if (0 != hudi_display_open(&hdl))
    {
        return ;
    }

    zoom_info.distype = DIS_TYPE_HD;
    zoom_info.layer = DIS_LAYER_MAIN;
    zoom_info.src_area.x = 0;
    zoom_info.src_area.y = 0;
    zoom_info.src_area.w = 1920;
    zoom_info.src_area.h = 1080;
    zoom_info.dst_area.x = 0;
    zoom_info.dst_area.y = 0;
    zoom_info.dst_area.w = 1920;
    zoom_info.dst_area.h = 1080;
    zoom_info.active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
    hudi_display_zoom_set(hdl, &zoom_info);

    aspect_info.distype = DIS_TYPE_HD;
    aspect_info.dis_mode = DIS_PILLBOX;
    aspect_info.tv_mode = DIS_TV_16_9;
    aspect_info.active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
    hudi_display_aspect_set(hdl, &aspect_info);

    hudi_display_close(hdl);
}

void hccast_media_air_event_init(hccast_media_air_event_callback air_cb)
{
    if (air_cb)
    {
        air_func = air_cb;
        hccast_log(LL_INFO, "%s init media air event\n", __func__);
    }
}

void hccast_media_dial_event_init(hccast_media_dial_event_callback dial_cb)
{
    if (dial_cb)
    {
        dial_func = dial_cb;
        hccast_log(LL_INFO, "%s init media dial event\n", __func__);
    }
}

void hccast_media_state_init()
{
    g_mediaplayer_t->position = 0;
    g_mediaplayer_t->duration = 0;
    g_mediaplayer_t->ready = 0;
    g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_STOP;

    g_mediaplayer_t->is_double_url = 0;
}

void hccast_media_callback_event(hccast_media_event_e event_type, void* param)
{
    if (mp_func)
    {
        mp_func(event_type, param);
    }
}

static int _hccast_media_seek(int64_t position, int seek_by_key)
{
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>enter: %s\n", __func__);
    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);

        if (g_media_type == HCCAST_MEDIA_PHOTO)
        {
            hccast_log(LL_NOTICE, "[media]:Picture not support seek.\n");
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);
            return -1;
        }

        if (!g_seek_update && g_seek_thread_en)
        {
            hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>leave: %s, seek update:%d\n", __func__, g_seek_update);
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);
            return -1;
        }

        g_seek_update = 0;
        g_mediaplayer_t->position = position;
        if (g_mediaplayer_t->is_double_url)
        {
            if (g_mediaplayer_t->player1)
                hcplayer_multi_seek(g_mediaplayer_t->player1, position);
        }
        else
        {
            if (g_mediaplayer_t->player)
                hcplayer_seek (g_mediaplayer_t->player, position);
        }

        if (seek_by_key)
        {
            //fix for iphone player bar can not seek back.
            if (player_url_mode == HCCAST_MEDIA_URL_AIRCAST)
            {
                if (air_func)
                {
                    air_func(HCCAST_MEDIA_AIR_VIDEO_LOADING, 0);
                }
            }
        }
        
        pthread_mutex_unlock(&g_mediaplayer_t->mutex);

    }
    
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>leave: %s\n", __func__);
    return 0;
}

static void *_hccast_media_seek_thread(void *arg)
{
    int ret = 0;
    int need_seek = 0;
    int seek_success = 0;
    struct timespec tv;

    while(!g_seek_thread_exit)
    {
        pthread_mutex_lock(&g_seek_mutex);
        clock_gettime(CLOCK_REALTIME, &tv);
        tv.tv_sec += 1;
        ret = pthread_cond_timedwait(&g_seek_cond, &g_seek_mutex, &tv);
        if (g_seek_position != -1 && ((ret == ETIMEDOUT) || g_seek_first))
        {
            seek_success = _hccast_media_seek(g_seek_position, g_seek_by_key);
            g_seek_position = -1;
            g_seek_by_key = 0;
            g_seek_first = 0;
            
            if (g_mediaplayer_t && !seek_success)
            {
                pthread_mutex_lock(&g_mediaplayer_t->mutex);

                g_seek_update = 0;
                if (g_mediaplayer_t->status != HCCAST_MEDIA_STATUS_PAUSED)
                {
                    if (g_mediaplayer_t->is_double_url)
                    {
                        if (g_mediaplayer_t->player1)
                        {
                            hcplayer_multi_play(g_mediaplayer_t->player1);
                        }
                    }
                    else
                    {
                        if (g_mediaplayer_t->player)
                        {
                            hcplayer_resume(g_mediaplayer_t->player);
                        }
                    }
                }       
                pthread_mutex_unlock(&g_mediaplayer_t->mutex);
            }
        }
        
        pthread_mutex_unlock(&g_seek_mutex);
    }

    hccast_log(LL_NOTICE, "Media seek thread exit.\n");

    return NULL;
}

static void _hccast_media_seek_thread_stop(void)
{
#if 1
    g_seek_update = 0;       
    g_seek_position = -1;
    g_seek_by_key = 0;
#else
    if (g_seek_thread_en)
    {
        if (g_seek_tid > 0)
        {
            g_seek_thread_exit = 1;
            pthread_mutex_lock(&g_seek_mutex);
            pthread_cond_signal(&g_seek_cond);
            pthread_mutex_unlock(&g_seek_mutex);
            pthread_join(g_seek_tid, NULL);
            g_seek_tid = 0;
        }
    } 
#endif    
}

static int _hccast_media_seek_thread_start(void)
{
    if (g_seek_thread_en)
    {
        if (g_seek_tid > 0)
        {
            hccast_log(LL_WARNING, "Media seek thread has started.\n");
            return -1;
        }

        g_seek_thread_exit = 0;
        g_seek_position = -1;
        g_seek_by_key = 0;
        g_seek_update = 0;
        g_seek_first = 1;
        if (pthread_create(&g_seek_tid, NULL, _hccast_media_seek_thread, NULL) != 0)
        {
            hccast_log(LL_ERROR, "Create media seek thread fail.\n");
            return -1;
        }
    }
    
    return 0;
}

void hccast_media_seek(int64_t position)
{  
    if (!g_seek_thread_en)
    {
        _hccast_media_seek(position, 0);
    }
    else
    {
        pthread_mutex_lock(&g_seek_mutex);
        
        if (g_mediaplayer_t)
        {
            pthread_mutex_lock(&g_mediaplayer_t->mutex);

            g_seek_update = 1;     
            g_seek_position = position;
            g_seek_by_key = 0;
            
            if (g_mediaplayer_t->is_double_url)
            {
                if (g_mediaplayer_t->player1)
                {
                    hcplayer_multi_pause(g_mediaplayer_t->player1);
                }
            }
            else
            {
                if (g_mediaplayer_t->player)
                {
                    hcplayer_pause(g_mediaplayer_t->player);
                }
            }
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);
        }
        
        pthread_cond_signal(&g_seek_cond);
        pthread_mutex_unlock(&g_seek_mutex);
    }
    
    hccast_media_callback_event(HCCAST_MEDIA_EVENT_URL_SEEK, (void*)position);
}

int hccast_media_get_volume(void)
{
    hudi_handle snd_hdl = NULL;
    unsigned int audsink;
    int vol = 0;

    hccast_com_audsink_get(&audsink);
    if (0 != hudi_snd_open(&snd_hdl, audsink))
    {
        return -1;
    }
    hudi_snd_volume_get(snd_hdl, &vol);
    hudi_snd_close(snd_hdl);

    return vol;
}

void hccast_media_set_volume(int vol)
{
    hccast_media_callback_event(HCCAST_MEDIA_EVENT_SET_VOLUME, (void*)vol);
}

int hccast_media_get_mirror_rotation()
{
    int rotate = 0;

    //Enable this, dlna video will rotate 90 degree while mirror rotate flag is set by httpd
    //hccast_media_callback_event(HCCAST_MEDIA_EVENT_GET_MIRROR_ROTATION, (void*)&rotate);

    return rotate;
}

int hccast_media_get_flip_mode()
{
    int flip_mode = 0;
    hccast_media_callback_event(HCCAST_MEDIA_EVENT_GET_FLIP_MODE, (void*)&flip_mode);
    return flip_mode;
}

static video_pbp_mode_e _hccast_media_video_pbp_mode_translate(unsigned int mode)
{
    video_pbp_mode_e pbp_mode;
    
    switch(mode)
    {
        case HCCAST_COM_VIDEO_PBP_OFF:
            pbp_mode = VIDEO_PBP_OFF;
            break;       
        case HCCAST_COM_VIDEO_PBP_2P_ON:
            pbp_mode = VIDEO_PBP_2P_ON;
            break;
        default:
            pbp_mode = VIDEO_PBP_OFF;
            break;
    }
    
    return pbp_mode;
}

static dis_type_e _hccast_media_video_dis_type_translate(unsigned int mode)
{
    dis_type_e dis_type;
    
    switch(mode)
    {
        case HCCAST_COM_VIDEO_DIS_TYPE_HD:
            dis_type = DIS_TYPE_HD;
            break;       
        case HCCAST_COM_VIDEO_DIS_TYPE_UHD:
            dis_type = DIS_TYPE_UHD;
            break;
        default:
            dis_type = DIS_TYPE_HD;
            break;
    }
    
    return dis_type;
}

static dis_layer_e _hccast_media_video_dis_layer_translate(unsigned int mode)
{
    dis_layer_e dis_layer;
    
    switch(mode)
    {
        case HCCAST_COM_VIDEO_DIS_LAYER_MAIN:
            dis_layer = DIS_LAYER_MAIN;
            break;       
        case HCCAST_COM_VIDEO_DIS_LAYER_AUXP:
            dis_layer = DIS_LAYER_AUXP;
            break;
        default:
            dis_layer = DIS_LAYER_MAIN;
            break;
    }
    
    return dis_layer;
}

long hccast_media_get_duration()
{
    long duration = 0;

    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);

        if (g_mediaplayer_t->is_double_url)
        {
            if (g_mediaplayer_t->player1)
                duration = hcplayer_multi_duration(g_mediaplayer_t->player1);
        }
        else
        {
            if (g_mediaplayer_t->player)
                duration = hcplayer_get_duration(g_mediaplayer_t->player);
        }

        if (duration < 0)
            duration = (long)g_mediaplayer_t->duration;

        //printf("duration: %d\n",duration);
        pthread_mutex_unlock(&g_mediaplayer_t->mutex);
    }

    return duration;
}

void hccast_media_set_duration(void)
{
    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);

        if (g_mediaplayer_t->is_double_url)
        {
            if (g_mediaplayer_t->player1)
                g_mediaplayer_t->duration = hcplayer_multi_duration(g_mediaplayer_t->player1);
        }
        else
        {
            if (g_mediaplayer_t->player)
                g_mediaplayer_t->duration = hcplayer_get_duration(g_mediaplayer_t->player);
        }

        pthread_mutex_unlock(&g_mediaplayer_t->mutex);
    }
}

long hccast_media_get_position(void)
{
    long position = 0;

    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);

        if (g_mediaplayer_t->is_double_url)
        {
            if (g_mediaplayer_t->player1)
                position = (long)hcplayer_multi_position(g_mediaplayer_t->player1);
        }
        else
        {
            if (g_mediaplayer_t->player)
                position = (long)hcplayer_get_position(g_mediaplayer_t->player);
        }

        if (g_seek_thread_en && g_seek_update && (g_seek_position != -1))
        {
            position = g_seek_position;
        }

        if (position < 0)
            position = 0;

        if (position > g_mediaplayer_t->duration)
            position = g_mediaplayer_t->position;//fix position for seek case.

        if (g_mediaplayer_t->status == HCCAST_MEDIA_STATUS_STOP)
            position = 0;
        pthread_mutex_unlock(&g_mediaplayer_t->mutex);
    }
    return position;
}

int hccast_media_get_status(void)
{
    int status = HCCAST_MEDIA_STATUS_STOP;

    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);

        status = g_mediaplayer_t->status;

        pthread_mutex_unlock(&g_mediaplayer_t->mutex);
    }

    return status;
}

void hccast_media_resume(void)
{
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>enter: %s\n", __func__);
    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);
        hccast_log(LL_INFO, "g_mediaplayer->status: %d, g_mediaplayer->ready: %d\n", g_mediaplayer_t->status, g_mediaplayer_t->ready);
        if (g_mediaplayer_t->status != HCCAST_MEDIA_STATUS_PAUSED)
        {
            hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>nothing to do.%s\n", __func__);
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);
            return;
        }

        if (g_mediaplayer_t->is_double_url)
        {
            if (g_mediaplayer_t->ready /*&& g_mediaplayer_t->ready1*/)
            {
                if (g_mediaplayer_t->player1)
                    hcplayer_multi_play(g_mediaplayer_t->player1);
                g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_PLAYING;
            }
        }
        else
        {
            if (g_mediaplayer_t->ready)
            {
                if (g_mediaplayer_t->player)
                    hcplayer_resume(g_mediaplayer_t->player);

                g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_PLAYING;
            }
        }
        pthread_mutex_unlock(&g_mediaplayer_t->mutex);
        hccast_media_callback_event(HCCAST_MEDIA_EVENT_PLAYING, NULL);
    }
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>leave: %s\n", __func__);
}

void hccast_media_pause(void)
{
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>enter: %s\n", __func__);
    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);
        if (g_mediaplayer_t->status != HCCAST_MEDIA_STATUS_PLAYING)
        {
            hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>nothing to do.%s\n", __func__);
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);
            return;
        }

        if (g_mediaplayer_t->is_double_url)
        {
            if (g_mediaplayer_t->player1)
                hcplayer_multi_pause(g_mediaplayer_t->player1);
            g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_PAUSED;
        }
        else
        {
            if (g_mediaplayer_t->player)
                hcplayer_pause(g_mediaplayer_t->player);
            g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_PAUSED;
        }
        pthread_mutex_unlock(&g_mediaplayer_t->mutex);
        hccast_media_callback_event(HCCAST_MEDIA_EVENT_PAUSE, NULL);
    }
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>leave: %s\n", __func__);
}

void hccast_media_stop(void)
{
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>enter: %s\n", __func__);
    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);

        if (g_mediaplayer_t->status == HCCAST_MEDIA_STATUS_STOP)
        {
            hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>nothing to do.%s\n", __func__);
            if (g_media_type == HCCAST_MEDIA_PHOTO)
            {
                hccast_media_reset_aspect_mode();
                hccast_meida_pic_effect_enable(0);
            }

            g_media_type = HCCAST_MEDIA_INVALID;
            g_mediaplayer_t->media_play_idx++;
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);
            return;
        }

        hccast_log(LL_NOTICE, "[%s]:g_mediaplayer_t->player:%p,  g_mediaplayer_t->player1:%p\n", __func__, g_mediaplayer_t->player, g_mediaplayer_t->player1);
        if (g_mediaplayer_t->player)
        {
            hcplayer_stop(g_mediaplayer_t->player);
            g_mediaplayer_t->player = NULL;
        }

        //for close audio url.
        if (g_mediaplayer_t->player1)
        {
            hcplayer_multi_destroy(g_mediaplayer_t->player1);
            g_mediaplayer_t->player1 = NULL;
        }

        g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_STOP;

        if (!hccast_scene_switch_happened(1000))
        {
            hccast_log(LL_NOTICE, "hccast_scene_playback_end_reset from here, %s:%d\n", __FUNCTION__, __LINE__);
            hccast_scene_playback_end_reset();
        }

        if ((player_url_mode == HCCAST_MEDIA_URL_AIRCAST) \
            && (!g_mediaplayer_t->is_double_url))
        {
            if (air_func)
            {
                air_func(HCCAST_MEDIA_AIR_VIDEO_END, 0);
            }
        }

        hccast_media_yt_playlist_buf_reset();
        if (g_media_type == HCCAST_MEDIA_PHOTO)
        {
            hccast_media_reset_aspect_mode();
            hccast_meida_pic_effect_enable(0);
        }
        g_media_type = HCCAST_MEDIA_INVALID;

        _hccast_media_seek_thread_stop();
        
        pthread_mutex_unlock(&g_mediaplayer_t->mutex);
        
        hccast_media_callback_event(HCCAST_MEDIA_EVENT_PLAYBACK_END, NULL);
    }
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>leave: %s\n", __func__);
}

void hccast_media_playbackend(int player_msgkey)
{
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>enter: %s\n", __func__);

    int photo_need_exit_menu = 0;

    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);

        if ( (g_mediaplayer_t->status == HCCAST_MEDIA_STATUS_STOP))
        {
            hccast_log(LL_NOTICE, "[media]:nothing to do.%s, status:%d,player_msgkey:%d, msgkey:%d\n", \
                       __func__, g_mediaplayer_t->status, player_msgkey, msgkey);
            if (g_media_type == HCCAST_MEDIA_PHOTO)
            {
                hccast_media_reset_aspect_mode();
                hccast_meida_pic_effect_enable(0);
            }

            g_media_type = HCCAST_MEDIA_INVALID;
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);
            return;
        }

        hccast_log(LL_NOTICE, "[%s]:g_mediaplayer_t->player:%p,  g_mediaplayer_t->player1:%p\n", __func__,
                   g_mediaplayer_t->player, g_mediaplayer_t->player1);
        if (g_mediaplayer_t->player)
        {
            hcplayer_stop(g_mediaplayer_t->player);
            g_mediaplayer_t->player = NULL;
        }

        //for close audio url.
        if (g_mediaplayer_t->player1)
        {
            hcplayer_multi_destroy(g_mediaplayer_t->player1);
            g_mediaplayer_t->player1 = NULL;
        }

        g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_STOP;

        if (!hccast_scene_switch_happened(1000))
        {
            hccast_log(LL_NOTICE, "hccast_scene_playback_end_reset from here, %s:%d\n", __FUNCTION__, __LINE__);
            hccast_scene_playback_end_reset();
        }

        if (player_url_mode == HCCAST_MEDIA_URL_AIRCAST)
        {
            if (air_func)
            {
                air_func(HCCAST_MEDIA_AIR_VIDEO_END, 0);
            }
        }

        hccast_media_yt_playlist_buf_reset();
        if (g_media_type == HCCAST_MEDIA_PHOTO)
        {
            hccast_media_reset_aspect_mode();
            photo_need_exit_menu = 1;//fix play photo but can recive eos msg.
            hccast_meida_pic_effect_enable(0);
        }

        g_media_type = HCCAST_MEDIA_INVALID;

        _hccast_media_seek_thread_stop();

        pthread_mutex_unlock(&g_mediaplayer_t->mutex);

        if ((hccast_get_current_scene() == HCCAST_SCENE_NONE) || (hccast_get_current_scene() == HCCAST_SCENE_DIAL_PLAY) || (photo_need_exit_menu))
        {
            hccast_media_callback_event(HCCAST_MEDIA_EVENT_PLAYBACK_END, NULL);
        }
    }
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>leave: %s\n", __func__);
}

void hccast_media_stop_by_key(void)
{
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>enter: %s\n", __func__);
    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);

        if (g_mediaplayer_t->status == HCCAST_MEDIA_STATUS_STOP)
        {
            hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>nothing to do.%s\n", __func__);
            if (g_media_type == HCCAST_MEDIA_PHOTO)
            {
                hccast_media_reset_aspect_mode();
                hccast_meida_pic_effect_enable(0);
            }

            g_media_type = HCCAST_MEDIA_INVALID;
            g_mediaplayer_t->media_play_idx++;
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);
            return;
        }

        hccast_log(LL_NOTICE, "[%s]:g_mediaplayer_t->player:%p,  g_mediaplayer_t->player1:%p\n", __func__, g_mediaplayer_t->player, g_mediaplayer_t->player1);
        if (g_mediaplayer_t->player)
        {
            hcplayer_stop(g_mediaplayer_t->player);
            g_mediaplayer_t->player = NULL;
        }

        //for close audio url.
        if (g_mediaplayer_t->player1)
        {
            hcplayer_multi_destroy(g_mediaplayer_t->player1);
            g_mediaplayer_t->player1 = NULL;
        }

        g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_STOP;

        if (!hccast_scene_switch_happened(1000))
        {
            hccast_log(LL_NOTICE, "hccast_scene_playback_end_reset from here, %s:%d\n", __FUNCTION__, __LINE__);
            hccast_scene_playback_end_reset();
        }

        if (player_url_mode == HCCAST_MEDIA_URL_AIRCAST)
        {
            if (air_func)
            {
                air_func(HCCAST_MEDIA_AIR_VIDEO_USEREXIT, 0);
            }
        }

        hccast_media_yt_playlist_buf_reset();
        if (g_media_type == HCCAST_MEDIA_PHOTO)
        {
            hccast_media_reset_aspect_mode();
            hccast_meida_pic_effect_enable(0);
        }
        g_media_type = HCCAST_MEDIA_INVALID;

        _hccast_media_seek_thread_stop();
        pthread_mutex_unlock(&g_mediaplayer_t->mutex);

        hccast_media_callback_event(HCCAST_MEDIA_EVENT_PLAYBACK_END, NULL);
    }
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>leave: %s\n", __func__);
}

void hccast_media_seek_by_key(int64_t position)
{
    if (!g_seek_thread_en)
    {
        _hccast_media_seek(position, 1);
    }
    else
    {
        pthread_mutex_lock(&g_seek_mutex);
    
        if (g_mediaplayer_t)
        {
            pthread_mutex_lock(&g_mediaplayer_t->mutex);

            g_seek_update = 1;
            g_seek_position = position;
            g_seek_by_key = 1;

            if (g_mediaplayer_t->is_double_url)
            {
                if (g_mediaplayer_t->player1)
                {
                    hcplayer_multi_pause(g_mediaplayer_t->player1);
                }
            }
            else
            {
                if (g_mediaplayer_t->player)
                {
                    hcplayer_pause(g_mediaplayer_t->player);
                }
            }
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);
        }

        pthread_cond_signal(&g_seek_cond);
        pthread_mutex_unlock(&g_seek_mutex);
    }
    
    hccast_log(LL_NOTICE, "%s position: %ds\n", __func__, position/1000);
}

void hccast_media_pause_by_key(void)
{
    int send_event = 0;

    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>enter: %s\n", __func__);
    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);
        if (g_mediaplayer_t->status != HCCAST_MEDIA_STATUS_PLAYING)
        {
            hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>nothing to do.%s\n", __func__);
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);
            return;
        }

        if (g_mediaplayer_t->is_double_url)
        {
            if (g_mediaplayer_t->player1)
            {
                hcplayer_multi_pause(g_mediaplayer_t->player1);
                send_event = 1;
            }
            g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_PAUSED;
        }
        else
        {
            if (g_mediaplayer_t->player)
            {
                hcplayer_pause(g_mediaplayer_t->player);
                send_event = 1;
            }
            g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_PAUSED;
        }

        if ((player_url_mode == HCCAST_MEDIA_URL_AIRCAST) && send_event)
        {
            if (air_func)
            {
                air_func(HCCAST_MEDIA_AIR_VIDEO_PAUSE, 0);
            }
        }

        pthread_mutex_unlock(&g_mediaplayer_t->mutex);
    }
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>leave: %s\n", __func__);
}

void hccast_media_resume_by_key(void)
{
    int send_event = 0;

    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>enter: %s\n", __func__);
    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);
        hccast_log(LL_INFO, "g_mediaplayer->status: %d, g_mediaplayer->ready: %d\n", g_mediaplayer_t->status, g_mediaplayer_t->ready);
        if (g_mediaplayer_t->status != HCCAST_MEDIA_STATUS_PAUSED)
        {
            hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>nothing to do.%s\n", __func__);
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);
            return;
        }

        if (g_mediaplayer_t->is_double_url)
        {
            if (g_mediaplayer_t->ready /*&& g_mediaplayer_t->ready1*/)
            {
                if (g_mediaplayer_t->player1)
                {
                    hcplayer_multi_play(g_mediaplayer_t->player1);
                    send_event = 1;
                }
                g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_PLAYING;
            }
        }
        else
        {
            if (g_mediaplayer_t->ready)
            {
                if (g_mediaplayer_t->player)
                {
                    hcplayer_resume(g_mediaplayer_t->player);
                    send_event = 1;
                }

                g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_PLAYING;
            }
        }

        if ((player_url_mode == HCCAST_MEDIA_URL_AIRCAST) && send_event)
        {
            if (air_func)
            {
                air_func(HCCAST_MEDIA_AIR_VIDEO_PLAY, 0);
            }
        }

        pthread_mutex_unlock(&g_mediaplayer_t->mutex);
    }
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>leave: %s\n", __func__);
}


void hccast_media_double_url_init_args(hccast_media_url_t *mp_url)
{
    HCPlayerInitArgs video_initargs;
    HCPlayerInitArgs audio_initargs;
    hccast_com_video_config_t video_config = {0};
    unsigned int audio_disable = 0;
    
    memset(&video_initargs, 0, sizeof(HCPlayerInitArgs));
    memset(&audio_initargs, 0, sizeof(HCPlayerInitArgs));

    int flip_mode = hccast_media_get_flip_mode();
    int m_flip_rotate = (flip_mode & 0xffff0000) >> 16;
    int m_flip_mirror = flip_mode & 0xffff;
    video_config.video_pbp_mode = HCCAST_COM_VIDEO_PBP_OFF;
    video_config.video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_HD;
    video_config.video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_MAIN;
    hccast_media_callback_event(HCCAST_MEDIA_EVENT_GET_VIDEO_CONFIG, (void*)&video_config);
    hccast_com_audio_disable_get(&audio_disable);

    if (mp_url->url_mode == HCCAST_MEDIA_URL_AIRCAST)
    {
        video_initargs.uri = NULL;
        video_initargs.readdata_opaque = &yt_playlist[HCCAST_MEDIA_YT_VIDEO];
        video_initargs.readdata_callback = hccast_media_yt_playlist_read;

        audio_initargs.uri = NULL;
        audio_initargs.readdata_opaque = &yt_playlist[HCCAST_MEDIA_YT_AUDIO];
        audio_initargs.readdata_callback = hccast_media_yt_playlist_read;
    }
    else
    {
        video_initargs.uri = mp_url->url;
        audio_initargs.uri = mp_url->url1;
    }

#ifdef HC_RTOS
    video_initargs.msg_id = (int)g_mediaplayer_t->msgid;
    audio_initargs.msg_id = (int)g_mediaplayer_t->msgid;
#else
    video_initargs.msg_id = g_mediaplayer_t->msgid;
    audio_initargs.msg_id = g_mediaplayer_t->msgid;
#endif
    //video_initargs.play_attached_file = 1;
    //Init video args.
    video_initargs.sync_type = HCPLAYER_AUDIO_MASTER;
    msgkey++;
    HCCAST_MEDIA_SETBIT(msgkey, 31);
    video_initargs.user_data = msgkey;

    video_initargs.rotate_enable = 1;
    if (hccast_media_get_mirror_rotation())
    {
        if (ROTATE_TYPE_0 == m_flip_rotate)
            video_initargs.rotate_type = ROTATE_TYPE_270;
        else if (ROTATE_TYPE_90 == m_flip_rotate)
            video_initargs.rotate_type = ROTATE_TYPE_0;
        else if (ROTATE_TYPE_180 == m_flip_rotate)
            video_initargs.rotate_type = ROTATE_TYPE_90;
        else if (ROTATE_TYPE_270 == m_flip_rotate)
            video_initargs.rotate_type = ROTATE_TYPE_180;
    }
    else
    {
        video_initargs.rotate_type = m_flip_rotate;
    }
    video_initargs.mirror_type = m_flip_mirror;
    video_initargs.pbp_mode = _hccast_media_video_pbp_mode_translate(video_config.video_pbp_mode);
    video_initargs.dis_type = _hccast_media_video_dis_type_translate(video_config.video_dis_type);
    video_initargs.dis_layer = _hccast_media_video_dis_layer_translate(video_config.video_dis_layer);

    hccast_media_zoom_info_t zoom_info;
    memset(&zoom_info, 0, sizeof(hccast_media_zoom_info_t));
    hccast_media_callback_event(HCCAST_MEDIA_EVENT_GET_ZOOM_INFO, &zoom_info);

    if (zoom_info.enable)
    {
        video_initargs.src_area.x = zoom_info.src_rect.x;
        video_initargs.src_area.y = zoom_info.src_rect.y;
        video_initargs.src_area.w = zoom_info.src_rect.w;
        video_initargs.src_area.h = zoom_info.src_rect.h;
        video_initargs.dst_area.x = zoom_info.dst_rect.x;
        video_initargs.dst_area.y = zoom_info.dst_rect.y;
        video_initargs.dst_area.w = zoom_info.dst_rect.w;
        video_initargs.dst_area.h = zoom_info.dst_rect.h;
        video_initargs.preview_enable = 1;
    }

    //Init audio args.
    audio_initargs.play_attached_file = 1;
    audio_initargs.sync_type = HCPLAYER_AUDIO_MASTER;

    audio_initargs.user_data = msgkey;
    hccast_com_audsink_get(&audio_initargs.snd_devs);
    audio_initargs.disable_audio = audio_disable;

    hccast_media_state_init();
    g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_BUFFERING;

    g_mediaplayer_t->player1 = hcplayer_multi_create(&audio_initargs, &video_initargs);
    if (g_mediaplayer_t->player1 == NULL)
    {
        hccast_log(LL_ERROR, "Create player1 handler error.\n");
    }
    hcplayer_multi_play(g_mediaplayer_t->player1);
    g_mediaplayer_t->is_double_url = 1;
    hccast_log(LL_INFO, "%s g_mediaplayer_t->player1:%p \n", __func__, g_mediaplayer_t->player1);
}


void hccast_media_init_arg(hccast_media_url_t *mp_url)
{
    HCPlayerInitArgs args;
    hccast_com_video_config_t video_config = {0};
    unsigned int audio_disable = 0;

    int flip_mode = hccast_media_get_flip_mode();
    int m_flip_rotate = (flip_mode & 0xffff0000) >> 16;
    int m_flip_mirror = flip_mode & 0xffff;
    video_config.video_pbp_mode = HCCAST_COM_VIDEO_PBP_OFF;
    video_config.video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_HD;
    video_config.video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_MAIN;
    hccast_media_callback_event(HCCAST_MEDIA_EVENT_GET_VIDEO_CONFIG, (void*)&video_config);
    hccast_com_audio_disable_get(&audio_disable);

    memset(&args, 0, sizeof(HCPlayerInitArgs));
    args.uri = mp_url->url;
#ifdef HC_RTOS
    args.msg_id = (int)g_mediaplayer_t->msgid;
#else
    args.msg_id = g_mediaplayer_t->msgid;
#endif
    //args.play_attached_file = 1;
    args.sync_type = HCPLAYER_AUDIO_MASTER;
    msgkey++;
    HCCAST_MEDIA_SETBIT(msgkey, 31);

    args.user_data = msgkey;

    //for picture.
    args.img_dis_hold_time = 60 * 60 * 1000; //show picture time. ms.
    args.gif_dis_interval = 0;
    args.img_alpha_mode = 1;//black background.
    args.pbp_mode = _hccast_media_video_pbp_mode_translate(video_config.video_pbp_mode);
    args.dis_type = _hccast_media_video_dis_type_translate(video_config.video_dis_type);
    args.dis_layer = _hccast_media_video_dis_layer_translate(video_config.video_dis_layer);
    args.disable_audio = audio_disable;

    hccast_media_zoom_info_t zoom_info;
    memset(&zoom_info, 0, sizeof(hccast_media_zoom_info_t));
    hccast_media_callback_event(HCCAST_MEDIA_EVENT_GET_ZOOM_INFO, &zoom_info);

    if (zoom_info.enable)
    {
        args.src_area.x = zoom_info.src_rect.x;
        args.src_area.y = zoom_info.src_rect.y;
        args.src_area.w = zoom_info.src_rect.w;
        args.src_area.h = zoom_info.src_rect.h;
        args.dst_area.x = zoom_info.dst_rect.x;
        args.dst_area.y = zoom_info.dst_rect.y;
        args.dst_area.w = zoom_info.dst_rect.w;
        args.dst_area.h = zoom_info.dst_rect.h;
        args.preview_enable = 1;

        if (g_media_type == HCCAST_MEDIA_PHOTO)
        {
            args.img_dis_mode = IMG_DIS_AUTO;
        }
    }
    else
    {
        if (g_media_type == HCCAST_MEDIA_PHOTO)
        {
            args.img_dis_mode = IMG_DIS_REALSIZE;
        }
    }

    args.buffering_enable = 1;
    args.buffering_start = 500;
    args.buffering_end = 5000;

    args.rotate_enable = 1;
    if (hccast_media_get_mirror_rotation())
    {
        if (ROTATE_TYPE_0 == m_flip_rotate)
            args.rotate_type = ROTATE_TYPE_270;
        else if (ROTATE_TYPE_90 == m_flip_rotate)
            args.rotate_type = ROTATE_TYPE_0;
        else if (ROTATE_TYPE_180 == m_flip_rotate)
            args.rotate_type = ROTATE_TYPE_90;
        else if (ROTATE_TYPE_270 == m_flip_rotate)
            args.rotate_type = ROTATE_TYPE_180;
    }
    else
    {
        args.rotate_type = m_flip_rotate;
    }

    args.mirror_type = m_flip_mirror;
    hccast_com_audsink_get(&args.snd_devs);

    hccast_media_state_init();
    g_mediaplayer_t->status = HCCAST_MEDIA_STATUS_BUFFERING;

    // bilibili live only allow this User-Agent: Bilibili Freedoooooom/MarkII
    if (mp_url->url_mode == HCCAST_MEDIA_URL_DLNA && \
            (strstr(mp_url->url, "bilivideo.com") \
            || strstr(mp_url->url, "bilibili")))
    {
        args.user_agent = "Bilibili Freedoooooom/MarkII";
    }

    hccast_log(LL_INFO, "args.quick_mode: %d\n", args.quick_mode);
    g_mediaplayer_t->player = hcplayer_create(&args);
    if (g_mediaplayer_t->player == NULL)
    {
        hccast_log(LL_ERROR, "Create player handler error.\n");
    }
    g_mediaplayer_t->is_double_url = 0;
    hcplayer_play(g_mediaplayer_t->player);
    hccast_log(LL_INFO, "g_mediaplayer_t->player:%p \n", g_mediaplayer_t->player);
}

void hccast_media_seturl(hccast_media_url_t *mp_url)
{
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>enter: %s\n", __func__);
    hccast_media_play_info_t media_url_info = {0};
    int play_idx = 0;

    if (g_mediaplayer_t)
    {
        if (mp_url->url_mode == HCCAST_MEDIA_URL_AIRCAST)
        {
            if (hccast_scene_get_switching() \
                && hccast_get_current_scene() != HCCAST_SCENE_AIRCAST_MIRROR)
            {
                hccast_log(LL_NOTICE, "%s %d skip aircast when switching\n", __func__, __LINE__);
                goto EXIT ;
            }

            hccast_scene_switch(HCCAST_SCENE_AIRCAST_PLAY);
        }
        else if (mp_url->url_mode == HCCAST_MEDIA_URL_DLNA \
                 || mp_url->url_mode == HCCAST_MEDIA_URL_DIAL)
        {
            /*when is playing dlna,and this time dlna and air-mirror switching at the same time,will cause air switch wait dlna service stop,
                dlna switch wait air-mirror stop.
            */
            if (hccast_scene_get_switching() && ((hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_MIRROR) || \
                                                 (hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY)))
            {
                hccast_log(LL_NOTICE, "%s %d skip dlna when switching\n", __func__, __LINE__);
                goto EXIT;
            }

            if (mp_url->url_mode == HCCAST_MEDIA_URL_DLNA)
            {
                hccast_scene_switch(HCCAST_SCENE_DLNA_PLAY);
            }
            if (mp_url->url_mode == HCCAST_MEDIA_URL_DIAL)
            {
                hccast_scene_switch(HCCAST_SCENE_DIAL_PLAY);
            }
        }
        else
        {
            hccast_log(LL_NOTICE, "%s Invalid url media mode.\n", __func__);
            goto EXIT;
        }

        pthread_mutex_lock(&g_mediaplayer_t->mutex);
        play_idx = ++g_mediaplayer_t->media_play_idx;
        pthread_mutex_unlock(&g_mediaplayer_t->mutex);

        media_url_info.media_type = mp_url->media_type;
        media_url_info.url_mode = mp_url->url_mode;
        media_url_info.enable_url_play = 1;//default enable play.
        hccast_media_callback_event(HCCAST_MEDIA_EVENT_URL_START_PLAY, (void*)&media_url_info);
        if (media_url_info.interface_valid)
        {
            if (!media_url_info.enable_url_play)
            {
                hccast_log(LL_NOTICE, "[%s]:Skip This time URL play back\n", __func__);
                goto EXIT;
            }
        }
        else
        {
            //use old callback.
            if (mp_url->url_mode == HCCAST_MEDIA_URL_AIRCAST)
            {
                hccast_media_callback_event(HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST, (void*)mp_url->media_type);
            }
            else if (mp_url->url_mode == HCCAST_MEDIA_URL_DLNA)
            {
                hccast_media_callback_event(HCCAST_MEDIA_EVENT_URL_FROM_DLNA, (void*)mp_url->media_type);
            }
            else if (mp_url->url_mode == HCCAST_MEDIA_URL_DIAL)
            {
                hccast_media_callback_event(HCCAST_MEDIA_EVENT_URL_FROM_DIAL, (void*)mp_url->media_type);
            }
        }

        pthread_mutex_lock(&g_mediaplayer_t->mutex);

        if (play_idx == g_mediaplayer_t->media_play_idx)
        {
            player_url_mode = mp_url->url_mode;
            hccast_log(LL_NOTICE, "[media]:last_media_type: %d, cur_media_type: %d, url_mode: %d\n", \
                       g_media_type, mp_url->media_type, player_url_mode);

            //here for not received stop cmd before seturl.
            if (g_mediaplayer_t->status != HCCAST_MEDIA_STATUS_STOP)
            {
                hccast_log(LL_NOTICE, "%s %d\n", __func__, __LINE__);
                if (g_mediaplayer_t->player)
                {
                    //fix for some app seturl music not stop first.show music logo.
                    if ((g_media_type == HCCAST_MEDIA_MUSIC) && (mp_url->media_type == HCCAST_MEDIA_MUSIC))
                    {
                        hccast_log(LL_NOTICE, "%s %d,not close vp\n", __func__, __LINE__);
                        hcplayer_stop2(g_mediaplayer_t->player, 0, 0);
                    }
                    else
                    {
                        hcplayer_stop(g_mediaplayer_t->player);
                    }

                    g_mediaplayer_t->player = NULL;
                }

                if (g_mediaplayer_t->player1)
                {
                    hcplayer_multi_destroy(g_mediaplayer_t->player1);
                    g_mediaplayer_t->player1 = NULL;
                }

                hccast_log(LL_NOTICE, "%s %d\n", __func__, __LINE__);

                hccast_media_yt_playlist_buf_reset();
                if (g_media_type == HCCAST_MEDIA_PHOTO)
                {
                    hccast_media_reset_aspect_mode();
                    hccast_meida_pic_effect_enable(0);
                }

                g_media_type = HCCAST_MEDIA_INVALID;
            }

            if (mp_url->media_type == HCCAST_MEDIA_PHOTO)
            {
                hccast_media_pic_aspect_set();
            }

            g_media_type = mp_url->media_type;

            if (mp_url->url && mp_url->url1) //double url.
            {
                if (mp_url->url_mode == HCCAST_MEDIA_URL_AIRCAST)
                {
                    hccast_media_yt_set_m3u8_playlist(HCCAST_MEDIA_YT_VIDEO, mp_url->yt_m3u8[HCCAST_MEDIA_YT_VIDEO]);
                    hccast_media_yt_set_m3u8_playlist(HCCAST_MEDIA_YT_AUDIO, mp_url->yt_m3u8[HCCAST_MEDIA_YT_AUDIO]);
                }

                hccast_media_double_url_init_args(mp_url);
            }
            else
            {
                hccast_media_init_arg(mp_url);
            }

            _hccast_media_seek_thread_stop();
            _hccast_media_seek_thread_start();
        }
        else
        {
            hccast_log(LL_NOTICE, "%s %d play_idx:%d, media_idx:%d\n", __func__, __LINE__, play_idx, g_mediaplayer_t->media_play_idx);
        }

        pthread_mutex_unlock(&g_mediaplayer_t->mutex);
    }

EXIT:
    hccast_log(LL_NOTICE, "[media]:>>>>>>>>>>>>>leave: %s\n", __func__);
}

void hccast_media_status_callback(void* msg_arg, hccast_media_player_t *p)
{
    HCPlayerMsg* msg = (HCPlayerMsg*)msg_arg;

    switch (msg->type)
    {
        case HCPLAYER_MSG_STATE_EOS:
            hccast_log(LL_NOTICE, "[video-media]:Get eos msg\n");
            hccast_media_playbackend(msg->user_data);
            break;
        case HCPLAYER_MSG_STATE_PLAYING:
            hccast_log(LL_INFO, "[video-media]:Get playing msg\n");
            hccast_media_callback_event(HCCAST_MEDIA_EVENT_PLAYING, NULL);
            break;
        case HCPLAYER_MSG_STATE_PAUSED:
            hccast_log(LL_INFO, "[video-media]:Get pause msg\n");
            hccast_media_callback_event(HCCAST_MEDIA_EVENT_PAUSE, NULL);
            break;
        case HCPLAYER_MSG_STATE_READY: //just only recv once time when after set url.

            pthread_mutex_lock(&p->mutex);
            hccast_log(LL_INFO, "[video-media]:Player ready\n");

            p->ready = 1;
            if (p->is_double_url == 0)
            {
                if (p->player)
                {
                    //hcplayer_resume(p->player);
                    hccast_log(LL_NOTICE, "[%s][%d] ready to play.\n", __func__, __LINE__);
                    p->status = HCCAST_MEDIA_STATUS_PLAYING;
                }
                else
                {
                    hccast_log(LL_NOTICE, "[%s][%d] p->player has been close.\n", __func__, __LINE__);
                }
            }
            else //double url.
            {
                //It can't be played until the video and audio are ready.
                if (p->ready /*&& p->ready1*/)
                {
                    if (p->player1)
                    {
                        //hcplayer_multi_play(p->player1);
                        hccast_log(LL_NOTICE, "[%s][%d]ready to play double url.\n", __func__, __LINE__);
                        p->status = HCCAST_MEDIA_STATUS_PLAYING;
                    }
                    else
                    {
                        hccast_log(LL_NOTICE, "[%s][%d] p->player1 has been close.\n", __func__, __LINE__);
                    }
                }
            }

            if (player_url_mode == HCCAST_MEDIA_URL_AIRCAST)
            {
                if (air_func)
                {
                    air_func(HCCAST_MEDIA_AIR_VIDEO_PLAY, 0);
                }
            }

            pthread_mutex_unlock(&p->mutex);

            hccast_media_set_duration();//store tol time.

            //for double url case.
            if (p->status == HCCAST_MEDIA_STATUS_PLAYING)
                hccast_media_callback_event(HCCAST_MEDIA_EVENT_PARSE_END, NULL);

            break;

        case HCPLAYER_MSG_UNSUPPORT_VIDEO_TYPE://open viddec err
        {
            HCPlayerVideoInfo video_info = {0};
            char *video_type = "unknow";
            if (!hcplayer_get_nth_video_stream_info (p->player, msg->val, &video_info))
            {
                /* only a simple sample, app developers use a static struct to mapping them. */
                if (video_info.codec_id == HC_AVCODEC_ID_HEVC)
                {
                    video_type = "h265";
                }
                else if (video_info.codec_id == HC_AVCODEC_ID_VP9)
                {
                    video_type = "vp9";
                }
                else if (video_info.codec_id == HC_AVCODEC_ID_AMV)
                {
                    video_type = "amv";
                }
            }
            hccast_log(LL_NOTICE, "unsupport video type %s, codec id %d\n", video_type, video_info.codec_id);
        }
        break;

        case HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE://open auddec err
        {
            HCPlayerAudioInfo audio_info = {0};
            char *audio_type = "unknow";
            if (!hcplayer_get_nth_audio_stream_info (p->player, msg->val, &audio_info))
            {
                /* only a simple sample, app developers use a static struct to mapping them. */
                if (audio_info.codec_id < 0x11000)
                {
                    audio_type = "pcm";
                }
                else if (audio_info.codec_id < 0x12000)
                {
                    audio_type = "adpcm";
                }
                else if (audio_info.codec_id == HC_AVCODEC_ID_DTS)
                {
                    audio_type = "dts";
                }
                else if (audio_info.codec_id == HC_AVCODEC_ID_EAC3)
                {
                    audio_type = "eac3";
                }
                else if (audio_info.codec_id == HC_AVCODEC_ID_APE)
                {
                    audio_type = "ape";
                }
            }
            hccast_log(LL_NOTICE, "unsupport audio type %s\n", audio_type);
        }
        break;

        case HCPLAYER_MSG_UNSUPPORT_SUBTITLE_TYPE://open subdec err
            break; //do nothing

        case HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO://open viddec err
        {
            HCPlayerAudioInfo audio_info = {0};
            char *audio_type = "unknow";
            int codec_id = HC_AVCODEC_ID_NONE;
            if (p->player1)
            {
                codec_id = msg->val;
            }
            else if (p->player)
            {
                if (!hcplayer_get_nth_audio_stream_info (p->player, 0, &audio_info))
                {
                    codec_id = audio_info.codec_id;
                }
            }
            /* only a simple sample, app developers use a static struct to mapping them. */
            if (codec_id <= HC_AVCODEC_ID_PCM_SGA)
            {
                audio_type = "pcm";
            }
            else if (codec_id <= HC_AVCODEC_ID_ADPCM_IMA_MOFLEX)
            {
                audio_type = "adpcm";
            }
            else if (HC_AVCODEC_ID_ROQ_DPCM <= codec_id &&
                     codec_id <= HC_AVCODEC_ID_DERF_DPCM)
            {
                audio_type = "dpcm";
            }
            else if (codec_id == HC_AVCODEC_ID_DTS)
            {
                audio_type = "dts";
            }
            else if (codec_id == HC_AVCODEC_ID_EAC3)
            {
                audio_type = "eac3";
            }
            else if (codec_id == HC_AVCODEC_ID_ATRAC3)
            {
                audio_type = "atrac3";
            }
            else if (codec_id == HC_AVCODEC_ID_TRUEHD)
            {
                audio_type = "truehd";
            }
            hccast_media_callback_event(HCCAST_MEDIA_EVENT_AUDIO_NOT_SUPPORT, (void*)codec_id);
            hccast_log(LL_NOTICE, "[audio-media]:no audio track or no supported audio track: %s\n", audio_type);
        }
        break;
        case HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO://open auddec err
        {
            HCPlayerVideoInfo video_info = {0};
            char *video_type = "unknow";
            int codec_id = HC_AVCODEC_ID_NONE;
            if (p->player1)
            {
                codec_id = msg->val;
                printf ("p->player1 codec_id %d\n", codec_id);
            }
            else if (p->player)
            {
                if (!hcplayer_get_nth_video_stream_info (p->player, 0, &video_info))
                {
                    codec_id = video_info.codec_id;
                }
            }
            /* only a simple sample, app developers use a static struct to mapping them. */
            if (codec_id == HC_AVCODEC_ID_HEVC)
            {
                video_type = "h265";
            }
            else if (codec_id == HC_AVCODEC_ID_VP9)
            {
                video_type = "vp9";
            }
            else if (codec_id == HC_AVCODEC_ID_AMV)
            {
                video_type = "amv";
            }
            else if (video_info.codec_id == HC_AVCODEC_ID_H263)
            {
                video_type = "h263";
            }
            hccast_media_callback_event(HCCAST_MEDIA_EVENT_VIDEO_NOT_SUPPORT, (void*)codec_id);
            hccast_log(LL_NOTICE, "[video-media]:no video track or no supported video track: %s\n", video_type);
        }
        break;
        case HCPLAYER_MSG_VIDEO_DECODE_ERR://viddec dec err
            hccast_log(LL_NOTICE, "[video-media]:HCPLAYER_MSG_VIDEO_DECODE_ERR\n");

            pthread_mutex_lock(&p->mutex);
            if (p->is_double_url == 0)
            {
                if (p->player)
                    hcplayer_change_video_track(p->player, -1);
            }
            pthread_mutex_unlock(&p->mutex);

            hccast_media_callback_event(HCCAST_MEDIA_EVENT_VIDEO_DECODER_ERROR, NULL);
            break;
        case HCPLAYER_MSG_AUDIO_DECODE_ERR://auddec dec err
            hccast_log(LL_NOTICE, "[video-media]:HCPLAYER_MSG_AUDIO_DECODE_ERR\n");

            pthread_mutex_lock(&p->mutex);
            if (p->is_double_url == 0)
            {
                if (p->player)
                    hcplayer_change_audio_track(p->player, -1);
            }

            pthread_mutex_unlock(&p->mutex);

            hccast_media_callback_event(HCCAST_MEDIA_EVENT_AUDIO_DECODER_ERROR, NULL);
            break;
        case HCPLAYER_MSG_SUBTITLE_DECODE_ERR://subdec dec err

            pthread_mutex_lock(&p->mutex);
            if (p->is_double_url == 0)
            {
                if (p->player)
                    hcplayer_change_subtitle_track(p->player, -1);
            }
            pthread_mutex_unlock(&p->mutex);

            hccast_log(LL_NOTICE, "[video-media]:HCPLAYER_MSG_SUBTITLE_DECODE_ERR\n");
            break;
        case HCPLAYER_MSG_ERR_UNDEFINED:
        case HCPLAYER_MSG_OPEN_FILE_FAILED://ffmpeg open err
        case HCPLAYER_MSG_UNSUPPORT_FORMAT://ffmpeg read/container err
        case HCPLAYER_MSG_READ_TIMEOUT:
            hccast_log(LL_NOTICE, "[video-media]:Get error msg\n");

            pthread_mutex_lock(&g_mediaplayer_t->mutex);
            p->status = HCCAST_MEDIA_STATUS_STOP;

            if (player_url_mode == HCCAST_MEDIA_URL_AIRCAST)
            {
                if (air_func)
                {
                    air_func(HCCAST_MEDIA_AIR_VIDEO_END, 0);
                }
            }

            hccast_log(LL_NOTICE, "[video-media]:g_mediaplayer_t->player:%p,  g_mediaplayer_t->player1:%p\n", g_mediaplayer_t->player, g_mediaplayer_t->player1);
            if (g_mediaplayer_t->player)
            {
                hcplayer_stop(g_mediaplayer_t->player);
                g_mediaplayer_t->player = NULL;
            }

            if (g_mediaplayer_t->player1)
            {
                hcplayer_multi_destroy(g_mediaplayer_t->player1);
                g_mediaplayer_t->player1 = NULL;
            }

            if (g_media_type == HCCAST_MEDIA_PHOTO)
            {
                hccast_media_reset_aspect_mode();
                hccast_meida_pic_effect_enable(0);
            }
            g_media_type = HCCAST_MEDIA_INVALID;

            hccast_media_yt_playlist_buf_reset();
            if (!hccast_scene_switch_happened(1000))
            {
                hccast_log(LL_NOTICE, "hccast_scene_playback_end_reset from here, %s:%d\n", __FUNCTION__, __LINE__);
                hccast_scene_playback_end_reset();
            }

            _hccast_media_seek_thread_stop();
            
            pthread_mutex_unlock(&g_mediaplayer_t->mutex);

            if (msg->type == HCPLAYER_MSG_READ_TIMEOUT)
            {
                hccast_media_callback_event(HCCAST_MEDIA_EVENT_PLAYBACK_END, NULL);
            }
            else
            {
                hccast_media_callback_event(HCCAST_MEDIA_EVENT_NOT_SUPPORT, NULL);
            }
            break;
        case HCPLAYER_MSG_BUFFERING:
            hccast_media_callback_event(HCCAST_MEDIA_EVENT_BUFFERING, (void*)msg->val);
            break;
#ifdef HC_RTOS
        case HCPLAYER_MSG_HTTP_FORBIDDEN:
            if (dial_func)
            {
                dial_func(HCCAST_MEDIA_DIAL_EVENT_FORBIDDEN, NULL);
            }
            break;
#endif
        default:
            break;
    }
}

static void *hccast_media_status_thread(void *args)
{
    hccast_media_player_t *p = (hccast_media_player_t *)args;
    HCPlayerMsg msg = {0};
    hccast_log(LL_NOTICE, "[hccast-media]:**************************************************\n");

    //block to receive ffplayer msg.
    while (p->media_running)
    {
#ifndef HC_RTOS
        if ((msgrcv(p->msgid, (void *) &msg, sizeof(HCPlayerMsg) - sizeof(msg.type), 0, 0)) == -1L)
        {
            if (errno != ENOMSG)
            {
                hccast_log(LL_ERROR, "[hccast-media]:msgrcv error: %s\n", strerror(errno));
                msgctl(p->msgid, IPC_RMID, NULL);
                p->msgid = 0;
                p->msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
                if (p->msgid <= 0)
                {
                    hccast_log(LL_ERROR, "[hccast-media]:Create msgid error: %s\n", strerror(errno));
                }
            }
        }
#else
        if (xQueueReceive((QueueHandle_t)p->msgid, (void *)&msg, -1) != pdPASS)
        {
            hccast_log(LL_ERROR, "[hccast-media]:msgrcv error\n");
        }
#endif
        else
        {
            if (msg.type != HCPLAYER_MSG_BUFFERING)
                hccast_log(LL_NOTICE, "[hccast-media]:Msg type: %ld, msgkey: %08x, msgkey1: %08x, user_data: %08x\n", msg.type, msgkey, msgkey1, (int)msg.user_data);

            if (((int)msg.user_data & MEDIA_PLAYER))
            {
                if (msgkey != msg.user_data)
                {
                    continue;
                }

                hccast_media_status_callback(&msg, p);

            }
        }
        //printf("[hccast-media]:======= revice one msg.\n");
    }
    hccast_log(LL_NOTICE, "[hccast-media]:Get message thread exit, p=%p\n", p);
    return NULL;
}

void hccast_media_yt_playlist_buf_reset(void)
{
    int i = 0;

    for (i = 0; i < 2; i++)
    {
        pthread_mutex_lock(&yt_playlist[i].mutex);
        if (yt_playlist[i].m3u8_info.data)
        {
            hccast_log(LL_INFO, "[%s:%d]: reset media m3u8 buf, num: %d\n", __func__, __LINE__, i);
            free(yt_playlist[i].m3u8_info.data);
            yt_playlist[i].m3u8_info.data = NULL;
            yt_playlist[i].m3u8_info.size = 0;
        }
        pthread_mutex_unlock(&yt_playlist[i].mutex);
    }
}

int hccast_media_yt_check_m3u8_status(hccast_media_yt_playlist_t* ytb_playlist)
{
    return ytb_playlist->readding;
}

int hccast_media_yt_playlist_read(void * opaque, uint8_t *buf, int bufsize)
{
    hccast_media_yt_playlist_t *ytblist = (hccast_media_yt_playlist_t *)opaque;
    int remain = 0;
    int real_read_size = 0;

    if (ytblist == NULL || buf == NULL )
    {
        hccast_log(LL_WARNING, "[%s]: ytblist: %p ,buf: %p\n", __func__, ytblist, buf);
        return HCAVERROR_EOF;
    }

    pthread_mutex_lock(&ytblist->mutex);

    if (ytblist->m3u8_info.data == NULL)
    {
        hccast_log(LL_WARNING, "ytblist->m3u8_info.data is NULL\n");
        ytblist->readding = 0;
        ytblist->offset = 0;
        pthread_mutex_unlock(&ytblist->mutex);
        return HCAVERROR_EOF;
    }

    remain = ytblist->m3u8_info.size - ytblist->offset;
    if (remain == 0)
    {
        hccast_log(LL_INFO, "[%s]:%d [ytb:%s] m3u8 is read eos. %p\n", __func__, __LINE__, (ytblist->m3u8_info.type ? "audio" : "video"), ytblist);
        ytblist->readding = 0;
        ytblist->offset = 0;
        pthread_mutex_unlock(&ytblist->mutex);
        return HCAVERROR_EOF;
    }

    ytblist->readding = 1;//it mean this m3u8 buf is readding by ffplayer.

    real_read_size = (bufsize >= remain) ? remain : bufsize;
    memcpy (buf, ytblist->m3u8_info.data + ytblist->offset, real_read_size);
    ytblist->offset += real_read_size;

    pthread_mutex_unlock(&ytblist->mutex);

    return real_read_size;
}

void hccast_media_yt_update_m3u8_playlist(int id, char* m3u8, int m3u8_size)
{
    int count = 1000;

    if (id >= 2 || m3u8 == NULL)
    {
        hccast_log(LL_WARNING, "[%s]: error\n", __func__);
        return;
    }

    pthread_mutex_lock(&yt_playlist[id].mutex);
    while (count)
    {
        if (hccast_media_yt_check_m3u8_status(&yt_playlist[id]))
        {
            //printf("[%s]: waitting ytb m3u8 list readding\n",__func__);
            pthread_mutex_unlock(&yt_playlist[id].mutex);
            usleep(100 * 1000);
            pthread_mutex_lock(&yt_playlist[id].mutex);
            count --;
        }
        else
        {
            hccast_log(LL_INFO, "[%s]: waitting ytb m3u8 list readding ok\n", __func__);
            break;
        }
    }

    //update the m3u8 buf for playlist.
    if (yt_playlist[id].m3u8_info.data)
    {
        free(yt_playlist[id].m3u8_info.data);
        yt_playlist[id].m3u8_info.data = NULL;
    }

    yt_playlist[id].m3u8_info.data = malloc(m3u8_size + 1);
    if (yt_playlist[id].m3u8_info.data == NULL)
    {
        hccast_log(LL_WARNING, "[%s]: malloc ytb m3u8 buf error\n", __func__);
    }
    else
    {
        memcpy(yt_playlist[id].m3u8_info.data, m3u8, m3u8_size);
        yt_playlist[id].m3u8_info.data[m3u8_size] = '\0';

        yt_playlist[id].m3u8_info.size = m3u8_size + 1;
        hccast_log(LL_INFO, "[%s]: %s, m3u8 size: %d\n", __func__, (id ? "audio" : "video"), yt_playlist[id].m3u8_info.size);
    }

    yt_playlist[id].offset = 0;//reset offset to read.

    pthread_mutex_unlock(&yt_playlist[id].mutex);
}

int hccast_media_yt_set_m3u8_playlist(int id, hccast_media_yt_m3u8_t* m3u8_info)
{
    if (id >= 2 || m3u8_info == NULL)
    {
        hccast_log(LL_WARNING, "%s, error\n", __func__);
        return -1;
    }

    pthread_mutex_lock(&yt_playlist[id].mutex);

    hccast_log(LL_INFO, "[%s]: %s, begin tick:%lld\n", __func__, (id ? "audio" : "video"), time(NULL));

    //free the m3u8 buf before.
    if (yt_playlist[id].m3u8_info.data)
    {
        hccast_log(LL_INFO, "[%s:%d]: reset media m3u8 buf, tick: %lld\n", __func__, id, time(NULL));
        free(yt_playlist[id].m3u8_info.data);
        yt_playlist[id].m3u8_info.data = NULL;
    }

    hccast_log(LL_NOTICE, "[%s]: %s, m3u8_info->size:%d\n", __func__, (id ? "audio" : "video"), m3u8_info->size);
    yt_playlist[id].m3u8_info.data = malloc(m3u8_info->size);
    if (yt_playlist[id].m3u8_info.data == NULL)
    {
        hccast_log(LL_ERROR, "[%s]: %s, malloc fail\n", __func__, (id ? "audio" : "video"));
    }
    else
    {
        if (!m3u8_info->data)
        {
            hccast_log(LL_ERROR, "err, m3u8_info->data is null\n");
            free(yt_playlist[id].m3u8_info.data);
            yt_playlist[id].m3u8_info.data = NULL;
            pthread_mutex_unlock(&yt_playlist[id].mutex);
            return 0;
        }
        memcpy(yt_playlist[id].m3u8_info.data, m3u8_info->data, m3u8_info->size);
        yt_playlist[id].m3u8_info.size = m3u8_info->size;
        yt_playlist[id].m3u8_info.type = m3u8_info->type;
    }


    yt_playlist[id].offset = 0;//reset offset to read.
    yt_playlist[id].readding = 0;


    hccast_log(LL_INFO, "[%s]: %s, m3u8 size: %d, tick:%lld\n", __func__, (id ? "audio" : "video"), yt_playlist[id].m3u8_info.size, time(NULL));

    pthread_mutex_unlock(&yt_playlist[id].mutex);

    return 0;
}

void hccast_media_yt_playlist_init(void)
{
    int i = 0;

    memset(yt_playlist, 0, (sizeof(yt_playlist) / sizeof(yt_playlist[0]))*sizeof(hccast_media_yt_playlist_t) );

    for (i = 0; i < (sizeof(yt_playlist) / sizeof(yt_playlist[0])); i++)
    {
        pthread_mutex_init(&yt_playlist[i].mutex, NULL);
    }
}

void hccast_media_destroy(void)
{
    hccast_log(LL_NOTICE, "[hccast-media]:mediaplayer is stop now.\n");
    if (g_mediaplayer_t)
    {
        g_mediaplayer_t->media_running = false;

        if (g_mediaplayer_t->tid > 0)
            pthread_cancel(g_mediaplayer_t->tid);

        hccast_log(LL_INFO, "[hccast-media]:g_mediaplayer->tid: %ld\n", g_mediaplayer_t->tid);
#ifdef HC_RTOS
        if (g_mediaplayer_t->msgid)
        {
            vQueueDelete(g_mediaplayer_t->msgid);
        }
#else
        if (g_mediaplayer_t->msgid >= 0)
        {
            msgctl(g_mediaplayer_t->msgid, IPC_RMID, NULL);
        }
#endif
        pthread_mutex_destroy(&g_mediaplayer_t->mutex);

        free(g_mediaplayer_t);
        g_mediaplayer_t = NULL;
    }
    hccast_log(LL_NOTICE, "[hccast-media]:mediaplayer is stop done.\n");
}

void hccast_media_init(hccast_media_event_callback mp_cb)
{
    g_mediaplayer_t = (hccast_media_player_t *)calloc(1, sizeof(hccast_media_player_t));
    if (g_mediaplayer_t == NULL)
    {
        hccast_log(LL_FATAL, "[hccast-media]:Not enough memory.\n");
        return ;
    }

    pthread_mutex_init(&g_mediaplayer_t->mutex, NULL);
#ifdef HC_RTOS
    g_mediaplayer_t->msgid = xQueueCreate(( UBaseType_t )configPLAYER_QUEUE_LENGTH, sizeof(HCPlayerMsg));
    if (!g_mediaplayer_t->msgid)
    {
        hccast_log(LL_ERROR, "[hccast-media]:Create msgid error.\n");
        goto fail;
    }
#else
    g_mediaplayer_t->msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (g_mediaplayer_t->msgid < 0)
    {
        hccast_log(LL_ERROR, "[hccast-media]:Create msgid error.\n");
        goto fail;
    }
#endif

    g_mediaplayer_t->media_running = true;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x4000);
    if (pthread_create(&g_mediaplayer_t->tid, &attr, hccast_media_status_thread, g_mediaplayer_t))
    {
        hccast_log(LL_ERROR, "[hccast-media]:Create hc_mp_status_thread error.\n");
        pthread_attr_destroy(&attr);
        goto fail;
    }

    pthread_attr_destroy(&attr);

    hccast_media_state_init();
    hccast_media_yt_playlist_init();

    if (mp_cb)
        mp_func = mp_cb;

    hccast_log(LL_NOTICE, "[hccast-media]:mediaplayer_init done\n");
    return ;
fail:
    if (g_mediaplayer_t)
    {
        g_mediaplayer_t->media_running = false;

        if (g_mediaplayer_t->tid > 0)
            pthread_join(g_mediaplayer_t->tid, NULL);
#ifdef HC_RTOS
        if (g_mediaplayer_t->msgid)
            vQueueDelete(g_mediaplayer_t->msgid);
#else
        if (g_mediaplayer_t->msgid >= 0)
            msgctl(g_mediaplayer_t->msgid, IPC_RMID, NULL);
#endif

        pthread_mutex_destroy(&g_mediaplayer_t->mutex);
        free(g_mediaplayer_t);
        g_mediaplayer_t = NULL;
        hccast_log(LL_NOTICE, "[hccast-media]:mediaplayer_init fail\n");
    }
}

void *hccast_media_player_get(void)
{
    void *arg = NULL;
    if (g_mediaplayer_t)
    {
        pthread_mutex_lock(&g_mediaplayer_t->mutex);
        if (g_mediaplayer_t->is_double_url)
        {
            //arg = g_mediaplayer_t->player1;
            arg = hcplayer_multi_get_vplayer(g_mediaplayer_t->player1);
        }
        else
        {
            arg = g_mediaplayer_t->player;
        }
        pthread_mutex_unlock(&g_mediaplayer_t->mutex);
        return arg;
    }
    else
        return NULL;
}

#endif

