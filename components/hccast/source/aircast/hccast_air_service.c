#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#ifdef HC_RTOS
#include <aircast/aircast_api.h>
#include <aircast/aircast_mdns.h>
#include <aircast/aircast_urlplay.h>
#else
#include <hccast/aircast_api.h>
#include <hccast/aircast_mdns.h>
#include <hccast/aircast_urlplay.h>
#endif
#include <hccast_air.h>
#include <stdint.h>
#include <hccast_dsc.h>
#include <hccast_av.h>
#include <hccast_media.h>
#include <hccast_scene.h>
#include <hccast_dsc.h>
#include <hccast_log.h>
#include "hccast_air_api.h"

#define MAX_M3U8_INFO_NUM           (2)
#define AIRCAST_DEFAULT_VOLUME      (80)

typedef struct
{
    hccast_air_event_callback notifier;
    pthread_mutex_t srv_mutex;
    aircast_dsc_func_t dsc_func;
    int srv_started;
    int p2p_started;
    int dnssd_started;
    int aircast_mode;
    int connection_cnt;
    int mirror_cnt;
    int mirror_fps;
    int audio_cnt;
    int audio_opened;
    int url_cid;
    int url_type;
    int url_duration;
    int url_is_ad;
    int url_need_skip;
    int url_ad_duration;
    int mp_duration;
    int mp_url;
    int mp_event;
    int mp_default_vol;
    int mp_last_seek_pos;
    int mp_position;
    int res_width;
    int res_height;
    int res_fps;
    char airp2p_pin[HCCAST_AIR_PIN_LEN];
    hccast_media_yt_m3u8_t yt_m3u8[MAX_M3U8_INFO_NUM];
} _hccast_air_inst_t;

static _hccast_air_inst_t *g_aircast_inst = NULL;
extern aircast_av_func_t g_aircast_av_func;

static unsigned int _hccast_air_get_tick(void)
{
    unsigned int cur_tick;
    struct timespec time;

    clock_gettime(CLOCK_REALTIME, &time);

    cur_tick = (time.tv_sec * 1000) + (time.tv_nsec / 1000000);

    return (cur_tick);
}

static int _hccast_air_mirror_mode_get(void)
{
#ifndef AIRCAST_SUPPORT_MIRROR_ONLY
    int data_mode = HCCAST_AIR_MODE_MIRROR_ONLY;
    int mirror_mode = AIRCAST_MIRROR_ONLY;

    if (g_aircast_inst->notifier)
    {
        g_aircast_inst->notifier(HCCAST_AIR_GET_MIRROR_MODE, (void*)&data_mode, NULL);
    }

    if (data_mode == HCCAST_AIR_MODE_MIRROR_STREAM)
    {
        mirror_mode = AIRCAST_MIRROR_WITH_HTTP_STREAM;
    }
    else if (data_mode == HCCAST_AIR_MODE_MIRROR_ONLY)
    {
        mirror_mode = AIRCAST_MIRROR_ONLY;
    }
    else
    {
        hccast_log(LL_WARNING, "data_mode is a bad value: %d\n", data_mode);
    }

    hccast_log(LL_INFO, "mirror_mode: %d\n", mirror_mode);
    return mirror_mode;
#else
    return AIRCAST_MIRROR_ONLY;
#endif
}

static void _hccast_air_mirror_fps_set(void)
{
    int is_4k_mdoe = 0;

    if (g_aircast_inst->notifier)
    {
        g_aircast_inst->notifier(HCCAST_AIR_GET_4K_MODE, (void*)&is_4k_mdoe, NULL);
        if (is_4k_mdoe)
        {
            g_aircast_inst->mirror_fps = 30;
        }
        else
        {
            if (g_aircast_inst->res_fps)
            {
                g_aircast_inst->mirror_fps = g_aircast_inst->res_fps;
            }
            else
            {
                g_aircast_inst->mirror_fps = 60;
            }
        }
    }

}

static void _hccast_air_mirror_res_set(void)
{
    int width = 0;
    int height = 0;

    if (g_aircast_inst->res_width && g_aircast_inst->res_height)
    {
        width = g_aircast_inst->res_width;
        height = g_aircast_inst->res_height;
    }
    else
    {
        width = 1920;
        height = 1080;
    }

    hccast_log(LL_NOTICE, "[%s][%d] %dx%dP%d \n", __func__, __LINE__, width, height, g_aircast_inst->mirror_fps);
    hccast_air_api_set_resolution(width, height, g_aircast_inst->mirror_fps);
}

static int _hccast_air_player_status_translate(int status)
{
    int air_state;

    switch (status)
    {
        case HCCAST_MEDIA_STATUS_PAUSED:
            air_state = AIRCAST_MEDIA_STATUS_PAUSED;
            break;
        case HCCAST_MEDIA_STATUS_PLAYING:
            air_state = AIRCAST_MEDIA_STATUS_PLAYING ;
            break;
        case HCCAST_MEDIA_STATUS_STOP:
        default:
            air_state = AIRCAST_MEDIA_STATUS_STOPPED ;
            break;
    }

    return air_state;
}

static void _hccast_air_get_player_state(AircastPlayerState_T *pplayerStat)
{
    int status;

    memset(pplayerStat, 0, sizeof(*pplayerStat));
    if (g_aircast_inst->url_is_ad)
    {
        pplayerStat->totalTime = g_aircast_inst->url_ad_duration;
    }
    else
    {
        pplayerStat->totalTime = ((long)hccast_media_get_duration() / 1000);
    }

    pplayerStat->currTime = (hccast_media_get_position() / 1000);
    pplayerStat->status = _hccast_air_player_status_translate(hccast_media_get_status());
}

static void _hccast_air_player_handler(int type, void *param)
{
    int air_event;

    switch (type)
    {
        case HCCAST_MEDIA_AIR_VIDEO_END:
            air_event = AIRCAST_VIDEO_ENDED;
            hccast_air_media_state_set(air_event, param);
            break;
        case HCCAST_MEDIA_AIR_VIDEO_USEREXIT:
            air_event = AIRCAST_VIDEO_USEREXIT;
            hccast_air_media_state_set(air_event, param);
            break;
        case HCCAST_MEDIA_AIR_VIDEO_PAUSE:
            air_event = AIRCAST_VIDEO_PAUSED;
            hccast_air_media_state_set(air_event, param);
            break;
        case HCCAST_MEDIA_AIR_VIDEO_PLAY:
            air_event = AIRCAST_VIDEO_PLAY;
            hccast_air_media_state_set(air_event, param);
            break;
        case HCCAST_MEDIA_AIR_VIDEO_LOADING:
            air_event = AIRCAST_VIDEO_LOADING;
            hccast_air_media_state_set(air_event, param);
            break;
        default :
            break;
    }
}

static void _hccast_air_m3u_buf_free()
{
    int i = 0;

    for (i = 0; i < MAX_M3U8_INFO_NUM; i++)
    {
        if (g_aircast_inst->yt_m3u8[i].data)
        {
            hccast_log(LL_DEBUG, "%s free num:%d\n", __func__, i);
            free(g_aircast_inst->yt_m3u8[i].data);
            g_aircast_inst->yt_m3u8[i].data = NULL;
            g_aircast_inst->yt_m3u8[i].size = 0;
        }
    }
}

static void _hccast_air_m3u_url_set(int idx, struct mlhls_m3u8_info* play_info)
{
    int m3u8_len;
    int m3u8_buf_size;

    //we should free at before.
    if (g_aircast_inst->yt_m3u8[idx].data)
    {
        free(g_aircast_inst->yt_m3u8[idx].data);
        g_aircast_inst->yt_m3u8[idx].data = NULL;
        g_aircast_inst->yt_m3u8[idx].size = 0;
    }

    m3u8_len = strlen(play_info->m3u8);
    m3u8_buf_size = m3u8_len + 1;

    g_aircast_inst->yt_m3u8[idx].data = malloc(m3u8_buf_size);
    if (g_aircast_inst->yt_m3u8[idx].data == NULL)
    {
        hccast_log(LL_ERROR, "[%s]: g_aircast_inst->yt_m3u8[%d].data malloc fail\n", __func__, idx);
    }
    else
    {
        g_aircast_inst->yt_m3u8[idx].type = idx;
        g_aircast_inst->yt_m3u8[idx].size = m3u8_buf_size;
        memcpy(g_aircast_inst->yt_m3u8[idx].data, play_info->m3u8, m3u8_len);
        g_aircast_inst->yt_m3u8[idx].data[m3u8_len] = '\0';
        hccast_log(LL_NOTICE, "[%s]: g_aircast_inst->yt_m3u8[%d].data size :%d\n", __func__, idx, g_aircast_inst->yt_m3u8[idx].size);
    }
}

static void _hccast_air_m3u_info_set(struct mlhls_m3u8_info* play_info)
{
    static int last_live_cid;
    static int live_set_play_info_cnt = 0;

    int cid = play_info->id >> 8;


    if (g_aircast_inst->url_type == 2)//this is live stream
    {
        //check wheather the same client.
        if (cid != last_live_cid)
        {
            last_live_cid = cid;
            live_set_play_info_cnt = 1;
        }
        else
        {
            live_set_play_info_cnt++;//it mean the same stream.
        }
    }
    else
    {
        live_set_play_info_cnt = 0;
    }

    if (live_set_play_info_cnt >= 2)
    {
        //update live stream m3u8 playlist.
        if (play_info->type == 2)//video
        {
            hccast_media_yt_update_m3u8_playlist(HCCAST_MEDIA_YT_VIDEO, play_info->m3u8, strlen(play_info->m3u8));
            //printf("[%s]:YTB:V: id = %d, total_len = %d, g_aircast_inst->mp_duration = %d \n", __func__,play_info->id, play_info->total_len, play_info->g_aircast_inst->mp_duration);
            g_aircast_inst->url_duration = play_info->total_time;
            g_aircast_inst->url_type = play_info->url_type;
        }
        else if (play_info->type == 1)//audio
        {
            hccast_media_yt_update_m3u8_playlist(HCCAST_MEDIA_YT_AUDIO, play_info->m3u8, strlen(play_info->m3u8));
            //printf("[%s]:YTB:A: id = %d, total_len = %d, g_aircast_inst->mp_duration = %d \n",  __func__, play_info->id, play_info->total_len, play_info->g_aircast_inst->mp_duration);
        }
        else
        {
            hccast_log(LL_WARNING, "%s: unknown type.\n", __FUNCTION__);
        }
    }
    else  //here is the first stream m3u8 store.
    {
        if (play_info->type == 2)//video
        {
            _hccast_air_m3u_url_set(HCCAST_MEDIA_YT_VIDEO, play_info);
            hccast_log(LL_INFO, "[%s]:V: id = %d, total_len = %d, g_aircast_inst->mp_duration = %d \n", __func__, play_info->id, play_info->total_len, play_info->total_time);
            g_aircast_inst->url_duration = play_info->total_time;
            g_aircast_inst->url_type = play_info->url_type;
        }
        else if (play_info->type == 1)//audio
        {
            _hccast_air_m3u_url_set(HCCAST_MEDIA_YT_AUDIO, play_info);
            hccast_log(LL_INFO, "[%s]:A: id = %d, total_len = %d, g_aircast_inst->mp_duration = %d \n", __func__, play_info->id, play_info->total_len, play_info->total_time);
        }
        else
        {
            hccast_log(LL_WARNING, "%s: unknown type.\n", __FUNCTION__);
        }
    }

    g_aircast_inst->url_ad_duration = play_info->total_time;
}

static int _hccast_air_url_parse(struct air_url_info *puri_info, hccast_media_url_t* mp_url_info)
{
    int ret = 0;
    char *video_url = NULL;
    char *audio_url = NULL;

    if ((puri_info == NULL) || (mp_url_info == NULL))
    {
        return -1;
    }

    //advertisement double url.
    if ((strstr(puri_info->url, "Advertisement")) && (strstr(puri_info->url, "V://")) && (strstr(puri_info->url, "A://")) )
    {
        hccast_log(LL_NOTICE, "%s()This is Advertisement url.\n", __func__);

        if ((strstr(puri_info->url, "V://")) && (strstr(puri_info->url, "A://")))
        {
            //parse audio url
            char* a_url_beging = strstr(puri_info->url, "A://");
            audio_url = a_url_beging + strlen("A://");
            //parse video url
            video_url = strstr(puri_info->url, "V://");
            video_url += strlen("V://");
            *a_url_beging = '\0';

            char* tmp = strstr(video_url, "index.m3u8");
            tmp += strlen("index.m3u8");
            *tmp = '\0';

            tmp = strstr(audio_url, "index.m3u8");
            tmp += strlen("index.m3u8");
            *tmp = '\0';

            mp_url_info->url = video_url;
            mp_url_info->url1 = audio_url;
            mp_url_info->url_mode = HCCAST_MEDIA_URL_AIRCAST;
            mp_url_info->media_type = HCCAST_MEDIA_MOVIE;
            mp_url_info->yt_m3u8[HCCAST_MEDIA_YT_VIDEO] = &g_aircast_inst->yt_m3u8[HCCAST_MEDIA_YT_VIDEO];
            mp_url_info->yt_m3u8[HCCAST_MEDIA_YT_AUDIO] = &g_aircast_inst->yt_m3u8[HCCAST_MEDIA_YT_AUDIO];

            g_aircast_inst->url_is_ad = 1;
            ret = 1;
            hccast_log(LL_NOTICE, "video:%s\n", mp_url_info->url);
            hccast_log(LL_NOTICE, "audio:%s\n", mp_url_info->url1);
        }
    }
    else if ((strstr(puri_info->url, "V://")) && (strstr(puri_info->url, "A://")) ) //double url
    {
        hccast_log(LL_NOTICE, "%s()This is double aircast url.\n", __func__);
        //parse audio url
        char* a_url_beging = strstr(puri_info->url, "A://");
        audio_url = a_url_beging + strlen("A://");
        //parse video url
        video_url = strstr(puri_info->url, "V://");
        video_url += strlen("V://");
        *a_url_beging = '\0';

        mp_url_info->url = video_url;
        mp_url_info->url1 = audio_url;
        mp_url_info->url_mode = HCCAST_MEDIA_URL_AIRCAST;
        mp_url_info->media_type = HCCAST_MEDIA_MOVIE;
        mp_url_info->yt_m3u8[HCCAST_MEDIA_YT_VIDEO] = &g_aircast_inst->yt_m3u8[HCCAST_MEDIA_YT_VIDEO];
        mp_url_info->yt_m3u8[HCCAST_MEDIA_YT_AUDIO] = &g_aircast_inst->yt_m3u8[HCCAST_MEDIA_YT_AUDIO];

        g_aircast_inst->url_is_ad = 0;
        ret = 1;
        hccast_log(LL_NOTICE, "video:%s\n", mp_url_info->url);
        hccast_log(LL_NOTICE, "audio:%s\n", mp_url_info->url1);

    }
    else //normal url
    {
        mp_url_info->url = puri_info->url;
        mp_url_info->url1 = NULL;
        mp_url_info->url_mode = HCCAST_MEDIA_URL_AIRCAST;
        mp_url_info->media_type = HCCAST_MEDIA_MOVIE;
        g_aircast_inst->url_is_ad = 0;
        ret = 0;
        hccast_log(LL_NOTICE, "video+audio:%s\n", mp_url_info->url);
    }

    return ret;
}

static void _hccast_air_url_play(void *param)
{
    struct air_url_info *puri_info = param;
    int ytb_flag = 0;
    int i = 0;
    int hostap_en = 0;

    hccast_log(LL_NOTICE, "[%s][%d]: AIRCAST_SET_URL \n", __func__, __LINE__);

    //This case for aircast mirror callback.
    if (strstr(puri_info->url, "music.tc.qq"))
    {
        g_aircast_inst->url_need_skip = 1;
        hccast_log(LL_NOTICE, "AIRCAST_SET_URL---mark special url, need ignore\n");
        return ;
    }
    else if (strstr(puri_info->url, "https://adsmind.gdtimg.com/") || strstr(puri_info->url, "GDTMediaCache+")) //skip can not play advertisment.
    {
        hccast_log(LL_NOTICE, "AIRCAST_SET_URL---mark special url, need ignore\n");
        return ;
    }

    g_aircast_inst->url_need_skip = 0;

    if (g_aircast_av_func._video_close)
    {
        g_aircast_av_func._video_close();
    }

    if (g_aircast_av_func._audio_close)
    {
        g_aircast_av_func._audio_close();
    }

    if (((hccast_get_current_scene() == HCCAST_SCENE_DLNA_PLAY) && hccast_air_mirror_stat_get())
        || (hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_MIRROR))
    {
        hccast_air_event_notify(HCCAST_AIR_MIRROR_STOP, NULL, NULL);
    }

    if (g_aircast_inst->aircast_mode == AIRCAST_MIRROR_WITH_HTTP_STREAM)
    {
        if (g_aircast_inst->notifier)
        {
            g_aircast_inst->notifier(HCCAST_AIR_GET_NETWORK_STATUS, (void*)&hostap_en, NULL);
            if (hostap_en)
            {
                g_aircast_inst->notifier(HCCAST_AIR_HOSTAP_MODE_SKIP_URL, NULL, NULL);
                hccast_log(LL_WARNING, "AIRCAST_SET_URL---skip it when is hostap mode.\n");
                return;
            }
        }
    }

    hccast_media_url_t mp_url;
    memset(&mp_url, 0, sizeof(hccast_media_url_t));

    ytb_flag = _hccast_air_url_parse(puri_info, &mp_url);

    hccast_media_seturl(&mp_url);


    if (ytb_flag)
    {
        _hccast_air_m3u_buf_free();
    }

    g_aircast_inst->url_cid = puri_info->cid;

    g_aircast_inst->mp_url = 1;
    hccast_air_media_state_set(AIRCAST_VIDEO_LOADING, (void *)g_aircast_inst->url_cid);
    g_aircast_inst->mp_duration = 0;
    g_aircast_inst->mp_position = 0;

    if (g_aircast_inst->mp_default_vol)
    {
        hccast_air_event_notify(HCCAST_AIR_SET_AUDIO_VOL, (void*)AIRCAST_DEFAULT_VOLUME, NULL);
    }
}

static void _hccast_air_url_stop(void *param)
{
    unsigned int stop_param = (unsigned int)param;
    unsigned short cid = stop_param & 0xffff;
    unsigned short abnormal_disconnect = (stop_param & 0xffff0000) >> 16;
    int stop_play = 1;

    hccast_log(LL_NOTICE, "[%s][%d]: AIRCAST_STOP_PLAY %d, %d\n", __FUNCTION__, __LINE__, cid, abnormal_disconnect);
    g_aircast_inst->url_duration = 0;
    g_aircast_inst->url_type = 0;

    if (hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY)
    {
        hccast_air_event_notify(HCCAST_AIR_GET_ABDISCONNECT_STOP_PLAY_EN, (void*)&stop_play, NULL);

        if (abnormal_disconnect)
        {
            if (stop_play)
            {
                hccast_media_stop();
                g_aircast_inst->url_is_ad = 0;
                g_aircast_inst->mp_url = 0;
            }
            else
            {
                hccast_set_current_scene(HCCAST_SCENE_NONE);
                hccast_log(LL_NOTICE, "Not stop play for abdisconnect.\n");
            }

        }
        else
        {
            hccast_media_stop();
            g_aircast_inst->url_is_ad = 0;
            g_aircast_inst->mp_url = 0;
        }

    }

    hccast_log(LL_NOTICE, "%s: AIRCAST_STOP_PLAY done\n", __FUNCTION__);

    g_aircast_inst->mp_duration = 0;
}

static void _hccast_air_url_seek(void *param)
{
    int64_t seek_time_ms = 0;

    if (g_aircast_inst->url_type == 1 || g_aircast_inst->url_type == 2)
    {
        hccast_log(LL_WARNING, "[%s][%d]:ignore seek, url_type = %d \n", __func__, __LINE__, g_aircast_inst->url_type);
        return;
    }

    hccast_log(LL_NOTICE, "[%s][%d]:seek position = %d\n", __func__, __LINE__, (unsigned int)param);
    seek_time_ms = (int)param * 1000;

    hccast_media_seek(seek_time_ms);//need to change to ms.

    AircastPlayerState_T playerStat;
    _hccast_air_get_player_state(&playerStat);
    if (playerStat.ret == 0)
    {
        g_aircast_inst->mp_last_seek_pos = playerStat.currTime;
    }

    hccast_air_media_state_set(AIRCAST_VIDEO_LOADING, (void *)g_aircast_inst->url_cid);
}

static void _hccast_air_url_play_info_get(AircastPlayerState_T *pplayerStat)
{
    int cid = pplayerStat->ret;
    char *fix_str = "";
    int diff = 0;

    _hccast_air_get_player_state(pplayerStat);
    if (pplayerStat->status == AIRCAST_MEDIA_STATUS_PLAYING \
        && !pplayerStat->totalTime \
        && pplayerStat->currTime > pplayerStat->totalTime)
    {
        pplayerStat->totalTime = g_aircast_inst->url_duration;
        fix_str = "f1";
    }
    pplayerStat->volume = g_aircast_inst->url_cid;
    if (!g_aircast_inst->mp_duration && pplayerStat->status == AIRCAST_MEDIA_STATUS_PLAYING)
        g_aircast_inst->mp_duration = pplayerStat->totalTime;

    if ((g_aircast_inst->mp_event == AIRCAST_VIDEO_LOADING) && (pplayerStat->status != AIRCAST_MEDIA_STATUS_PAUSED))
    {
        diff = pplayerStat->currTime > g_aircast_inst->mp_last_seek_pos ? pplayerStat->currTime - g_aircast_inst->mp_last_seek_pos : g_aircast_inst->mp_last_seek_pos - pplayerStat->currTime;
        if (diff > 1)
        {
            hccast_air_media_state_set(AIRCAST_VIDEO_PLAY, 0);
        }
    }
    if ((pplayerStat->ret == 0) && (pplayerStat->status == AIRCAST_MEDIA_STATUS_STOPPED))
    {
        if (cid > g_aircast_inst->url_cid)
        {
            pplayerStat->currTime = 0;//g_aircast_inst->mp_duration;
            pplayerStat->totalTime = 0;//g_aircast_inst->mp_duration;
            fix_str = "f2";
        }
        else if (g_aircast_inst->mp_duration)
        {

            pplayerStat->currTime = g_aircast_inst->mp_duration;
            pplayerStat->totalTime = g_aircast_inst->mp_duration;
            fix_str = "f3";
        }
    }

    if (g_aircast_inst->url_type == 2 && pplayerStat->currTime > pplayerStat->totalTime)
    {
        pplayerStat->totalTime = pplayerStat->currTime + 10;
        fix_str = "f4";
    }

    if (g_aircast_inst->url_is_ad)
    {
        if ((g_aircast_inst->mp_position > pplayerStat->currTime) && (pplayerStat->status == AIRCAST_MEDIA_STATUS_PLAYING))
        {
            pplayerStat->currTime = g_aircast_inst->mp_position;
            fix_str = "f5";
        }
    }

    g_aircast_inst->mp_position = pplayerStat->currTime;
}

static void _hccast_air_mirror_start()
{
    int enable_play = 1;//when APP set flag at (0) ,it mean not open video dec. 1 -- default open.

    g_aircast_inst->mirror_cnt ++;
    g_aircast_inst->url_need_skip = 0;
    hccast_scene_switch(HCCAST_SCENE_AIRCAST_MIRROR);

    hccast_log(LL_NOTICE, "[%s][%d]:AIRCAST_MIRROR_START (%d)\n", __func__, __LINE__, g_aircast_inst->mirror_cnt);
    usleep(100 * 1000);

    hccast_media_stop();
    hccast_air_event_notify(HCCAST_AIR_MIRROR_START, (void *)&enable_play, NULL);

    if (enable_play)
    {
        if (g_aircast_av_func._video_open)
        {
            g_aircast_av_func._video_open();
        }

        if (g_aircast_av_func._audio_open)
        {
            g_aircast_av_func._audio_open();
        }
    }
    else
    {
        hccast_log(LL_NOTICE, "[%s][%d]: App condition not allow to open video and audio.\n", __func__, __LINE__);
    }
}

static void _hccast_air_mirror_stop()
{
    hccast_log(LL_NOTICE, "[%s][%d]:AIRCAST_MIRROR_STOP (%d)\n", __func__, __LINE__, g_aircast_inst->mirror_cnt);

    if (g_aircast_inst->mirror_cnt > 1)
    {
        hccast_log(LL_NOTICE, "mirror not need close.\n");
        g_aircast_inst->mirror_cnt--;
        return ;
    }

    if (g_aircast_av_func._video_close)
    {
        g_aircast_av_func._video_close();
    }

    if (g_aircast_inst->audio_opened == 0)
    {
        hccast_log(LL_NOTICE, "[%s][%d]:audio need close.\n", __func__, __LINE__);
        if (g_aircast_av_func._audio_close)
        {
            g_aircast_av_func._audio_close();
        }
    }

    //for air-mirror change for airplay stream.
    if ((hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_MIRROR) \
        || (hccast_get_current_scene() == HCCAST_SCENE_NONE) )
    {
        hccast_scene_reset(HCCAST_SCENE_AIRCAST_MIRROR, HCCAST_SCENE_NONE);
        hccast_air_event_notify(HCCAST_AIR_MIRROR_STOP, NULL, NULL);
    }
    else if ((hccast_get_current_scene() == HCCAST_SCENE_DLNA_PLAY) || \
             (hccast_get_current_scene() == HCCAST_SCENE_DIAL_PLAY) || \
             (hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR) || \
             (hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
    {
        //just call to leave menu.because the current scene had changed to dlna for set url.
        hccast_air_event_notify(HCCAST_AIR_MIRROR_STOP, NULL, NULL);
    }

    g_aircast_inst->mirror_cnt--;
}

static void _hccast_air_volume_set(void *param)
{
    int volume = (int)param;
    int vol_cid = -1;
    hccast_air_api_ioctl(AIRCAST_GET_VOL_ID, &vol_cid, 0);
    int vol = 0;

    hccast_log(LL_DEBUG, "[%s][%d]AIRCAST_SET_VOLUME:vol=%d\n", __func__, __LINE__, volume);

    //range[-30 ~ 0]
    if (volume < -30)
    {
        hccast_log(LL_NOTICE, "[%s][%d]AIRCAST_SET_VOLUME: inval volume\n", __func__, __LINE__);
        return ;
    }
    else if (volume <= -29)
    {
        vol = 0;
    }
    else
    {
        vol = (30 + volume) * 100 / 30;
    }

    //set volume
    if ((hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY) && (vol == 0))
    {
        hccast_log(LL_NOTICE, "Aircast skip this time setting vol.\n");
    }
    else
    {
        hccast_air_event_notify(HCCAST_AIR_SET_AUDIO_VOL, (void*)vol, NULL);
    }
}

static void _hccast_air_audio_start()
{
    g_aircast_inst->audio_cnt ++;
    hccast_log(LL_NOTICE, "[%s][%d]:AIRCAST_AUDIO_START %d\n", __func__, __LINE__, g_aircast_inst->audio_cnt);

    hccast_scene_mira_stop();
    hccast_scene_set_mira_restart_flag(0);

    if ((hccast_get_current_scene() == HCCAST_SCENE_DLNA_PLAY))
    {
        hccast_media_stop();
    }
    else if (hccast_get_current_scene() == HCCAST_SCENE_DIAL_PLAY)
    {
        hccast_media_stop();
        hccast_scene_dial_restart();
    }

    if ((hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY) && (hccast_media_get_status() != HCCAST_MEDIA_STATUS_STOP))
    {
        hccast_log(LL_NOTICE, "[%s][%d] AIRCAST_AUDIO_START: now just skip audio open operation.\n", __func__, __LINE__);
    }
    else
    {
        if (g_aircast_av_func._audio_open)
        {
            hccast_media_stop();
            g_aircast_av_func._audio_open();
            g_aircast_inst->audio_opened = 1;
        }
    }

    if (hccast_get_current_scene() == HCCAST_SCENE_NONE)
    {
        hccast_air_event_notify(HCCAST_AIR_AUDIO_START, NULL, NULL);
    }

}

static void _hccast_air_audio_stop()
{
    hccast_log(LL_NOTICE, "[%s][%d]:AIRCAST_AUDIO_STOP %d\n", __func__, __LINE__, g_aircast_inst->audio_cnt);
    if (g_aircast_av_func._audio_close)
    {
        g_aircast_av_func._audio_close();
    }
    g_aircast_inst->audio_cnt --;
    g_aircast_inst->audio_opened = 0;
    hccast_scene_set_mira_restart_flag(1);
    usleep(20 * 1000);
    if (hccast_get_current_scene() == HCCAST_SCENE_NONE)
    {
        hccast_air_event_notify(HCCAST_AIR_AUDIO_STOP, NULL, NULL);
    }
}

static void _hccast_air_event_handler(int event_type, void *param)
{
    struct mlhls_m3u8_info play_info;

    switch (event_type)
    {
        case AIRCAST_SET_URL:
            _hccast_air_url_play(param);
            break;
        case AIRCAST_STOP_PLAY:
            _hccast_air_url_stop(param);
            break;
        case AIRCAST_GET_PLAY_INFO:
            _hccast_air_url_play_info_get(param);
            break;
        case AIRCAST_URL_SEEK:
            _hccast_air_url_seek(param);
            break;
        case AIRCAST_URL_PAUSE:
            hccast_log(LL_NOTICE, "[%s][%d]:AIRCAST_URL_PAUSE \n", __func__, __LINE__);
            hccast_media_pause();
            break;
        case AIRCAST_URL_RESUME_PLAY:
            hccast_log(LL_NOTICE, "[%s][%d]:AIRCAST_URL_RESUME_PLAY \n", __func__, __LINE__);
            hccast_media_resume();
            break;
        case AIRCAST_MIRROR_START:
            _hccast_air_mirror_start();
            break;
        case AIRCAST_MIRROR_STOP:
            _hccast_air_mirror_stop();
            break;
        case AIRCAST_SET_VOLUME:
            _hccast_air_volume_set(param);
            break;
        case AIRCAST_AUDIO_START:
            _hccast_air_audio_start();
            break;
        case AIRCAST_AUDIO_STOP:
            _hccast_air_audio_stop();
            break;
        case AIRCAST_SET_PLAY_INFO:
            memcpy(&play_info, param, sizeof(play_info));
            _hccast_air_m3u_info_set(&play_info);
            break;
        case AIRCAST_INVALID_CERT:
            hccast_air_event_notify(HCCAST_AIR_INVALID_CERT, NULL, NULL);
            break;
        case AIRCAST_CONNRESET:
            hccast_air_event_notify(HCCAST_AIR_BAD_NETWORK, NULL, NULL);
            break;
        case AIRCAST_FAKE_LIB:
            hccast_air_event_notify(HCCAST_AIR_FAKE_LIB, NULL, NULL);
            break;
        case AIRCAST_CONNECTIONS_ADD:
            g_aircast_inst->connection_cnt++;
            hccast_air_event_notify(HCCAST_AIR_PHONE_CONNECT, NULL, NULL);
            break;
        case AIRCAST_CONNECTIONS_REMOVE:
            g_aircast_inst->connection_cnt--;
            if (g_aircast_inst->connection_cnt == 0)
            {
                hccast_air_event_notify(HCCAST_AIR_PHONE_DISCONNECT, NULL, NULL);
            }     
            break;
        case AIRCAST_PHONE_MODEL:
            hccast_air_event_notify(HCCAST_AIR_PHONE_MODEL, (void*)param, NULL);
            break;
        case AIRCAST_P2P_INVALID_CERT:
            hccast_air_event_notify(HCCAST_AIR_P2P_INVALID_CERT, NULL, NULL);
            break;
        default:
            hccast_log(LL_NOTICE, "[%s][%d]event: %d\n", __func__, __LINE__, event_type);
            break;
    }
}

int hccast_air_event_notify(int msg_type, void* in, void* out)
{
    if (g_aircast_inst->notifier)
        g_aircast_inst->notifier(msg_type, in, out);

    return 0;
}

int hccast_air_mirror_stat_get(void)
{
    return g_aircast_inst->mirror_cnt;
}

int hccast_air_url_skip_get(void)
{
    return g_aircast_inst->url_need_skip;
}

int hccast_air_url_skip_set(int skip)
{
    g_aircast_inst->url_need_skip = skip;
}

int hccast_air_is_playing_music(void)
{
    return g_aircast_inst->audio_opened && g_aircast_inst->audio_cnt;
}

int hccast_air_media_state_set(int type, void *param)
{
    hccast_log(LL_INFO, "%s:%d\n", __FUNCTION__, type);
    g_aircast_inst->mp_event = type;
    hccast_air_api_event_notify(type, param);

    return 0;
}

int hccast_air_mirror_fps_get(void)
{
    return g_aircast_inst->mirror_fps;
}

int hccast_air_stop_playing()
{
    unsigned int wait_time;
    unsigned int tick = _hccast_air_get_tick();

    if (hccast_air_mirror_stat_get())
    {
        wait_time = 5000;
        hccast_log(LL_NOTICE, "[%s:%u] begin stop air-mirror.\n", __func__, tick);
        hccast_air_api_event_notify(AIRCAST_USER_MIRROR_STOP, 0);
        while (hccast_air_mirror_stat_get() && ((_hccast_air_get_tick() - tick) < wait_time))
        {
            usleep(20 * 1000);
        }
    }
    else if (hccast_air_audio_state_get())
    {
        wait_time = 2000;
        hccast_log(LL_NOTICE, "[%s:%u] begin stop air-music.\n", __func__, tick);
        hccast_air_api_event_notify(AIRCAST_USER_AUDIO_STOP, 0);
        while (hccast_air_audio_state_get() && (_hccast_air_get_tick() - tick) < wait_time)
        {
            usleep(20 * 1000);
        }
    }
    else if (hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY)
    {
        wait_time = 5000;
        hccast_log(LL_NOTICE, "[%s:%u] begin stop air-media.\n", __func__, tick);
        hccast_air_api_event_notify(AIRCAST_VIDEO_USEREXIT, 0);
        while (g_aircast_inst->mp_url && (_hccast_air_get_tick() - tick) < wait_time)
        {
            usleep(20 * 1000);
        }
    }
    else
    {
        hccast_log(LL_INFO, "%s nothing to do.\n", __func__);
    }

    hccast_log(LL_NOTICE, "[%s:%u] done.\n", __func__, _hccast_air_get_tick());

    return 0;
}

int hccast_air_connect_state_get(void)
{
    return ((g_aircast_inst->connection_cnt == 0) ? 0 : 1);
}

int hccast_air_mdnssd_start()
{
    int ipv6_en = 1;
    int mdns_poll_time = 200;//ms
    int mdns_refresh_time = 200;//ms

    if (!g_aircast_inst->dnssd_started)
    {
#ifdef SUPPORT_AIRP2P
        if (g_aircast_inst->p2p_started)
        {
            hc_mdns_ioctl(HC_MDNS_SET_IPV6_EN, (void*)ipv6_en, NULL);
            hc_mdns_ioctl(HC_MDNS_SET_POLL_TIME, (void*)mdns_poll_time, NULL);
            hc_mdns_ioctl(HC_MDNS_SET_REFRESH_TIME, (void*)mdns_refresh_time, NULL);
        }
#endif
        hc_mdns_daemon_start();
        g_aircast_inst->dnssd_started = 1;
    }

    return 0;
}

int hccast_air_service_is_start()
{
    int started = 0;

    pthread_mutex_lock(&g_aircast_inst->srv_mutex);
    started = g_aircast_inst->srv_started;
    pthread_mutex_unlock(&g_aircast_inst->srv_mutex);

    return started;
}

int hccast_air_mdnssd_stop()
{
    if (g_aircast_inst->dnssd_started)
    {
        hc_mdns_daemon_stop();
        g_aircast_inst->dnssd_started = 0;
    }

    return 0;
}

int hccast_air_service_start()
{
    int aircast_mode;
    char service_name[64] = "HCCAST_AIR_TEST";
    char device_name[32] = "wlan0";
    int airp2p_en = 0;
    char airp2p_pin[64] = {0};

    pthread_mutex_lock(&g_aircast_inst->srv_mutex);
    if (g_aircast_inst->srv_started)
    {
        hccast_log(LL_WARNING, "[%s]: aircast service has been start\n", __func__);
        pthread_mutex_unlock(&g_aircast_inst->srv_mutex);
        return 0;
    }

    hccast_air_event_notify(HCCAST_AIR_GET_SERVICE_NAME, (void*)service_name, NULL);
    hccast_air_event_notify(HCCAST_AIR_GET_NETWORK_DEVICE, (void*)device_name, NULL);
    hccast_air_event_notify(HCCAST_AIR_URL_ENABLE_SET_DEFAULT_VOL, (void*)&g_aircast_inst->mp_default_vol, NULL);
    hccast_air_event_notify(HCCAST_AIR_GET_AIRP2P_PIN, (void*)airp2p_pin, NULL);

    hc_mdns_set_devname(device_name);
    hccast_air_mdnssd_start();//just need start once time.

    _hccast_air_mirror_fps_set();
    _hccast_air_mirror_res_set();
    hccast_air_api_ioctl(AIRCAST_SET_AV_FUNC, &g_aircast_av_func, 0);
    hccast_air_api_ioctl(AIRCAST_SET_AES_FUNC, &g_aircast_inst->dsc_func, 0);
    hccast_air_api_set_notifier(_hccast_air_event_handler);

    if (g_aircast_inst->p2p_started)
    {
        aircast_mode = AIRCAST_MIRROR_ONLY;
    }
    else
    {
        aircast_mode = _hccast_air_mirror_mode_get();
    }

#ifdef SUPPORT_AIRP2P
    if (strlen(g_aircast_inst->airp2p_pin) == 0)
    {
        hccast_air_api_ioctl(AIRCAST_SET_AIRP2P_PIN, (void *)airp2p_pin, NULL);
    }       
#endif
    hccast_air_api_ioctl(AIRCAST_SET_MIRROR_MODE, (void *)aircast_mode, NULL);
    hccast_air_api_service_start(service_name, device_name);

    g_aircast_inst->aircast_mode = aircast_mode;
    g_aircast_inst->srv_started = 1;
    pthread_mutex_unlock(&g_aircast_inst->srv_mutex);
    hccast_log(LL_NOTICE, "[Aircast]: hc_aircast_service_start done\n");

    return 0;
}

int hccast_air_service_stop()
{
    if (!g_aircast_inst)
    {
        return 0;
    }

    pthread_mutex_lock(&g_aircast_inst->srv_mutex);
    if (g_aircast_inst->srv_started == 0)
    {
        hccast_log(LL_WARNING, "[%s]: aircast service has been stop\n", __func__);
        pthread_mutex_unlock(&g_aircast_inst->srv_mutex);
        return 0;
    }

    hccast_air_api_service_stop();
    hccast_air_mdnssd_stop();

    g_aircast_inst->srv_started = 0;
    g_aircast_inst->connection_cnt = 0;
    pthread_mutex_unlock(&g_aircast_inst->srv_mutex);

    hccast_log(LL_NOTICE, "[Aircast]: hc_aircast_service_stop done\n");

    return 0;
}

int hccast_air_service_init(hccast_air_event_callback aircast_cb)
{
    if (!g_aircast_inst)
    {
        g_aircast_inst = (_hccast_air_inst_t *)malloc(sizeof(_hccast_air_inst_t));
        memset(g_aircast_inst, 0, sizeof(_hccast_air_inst_t));
        pthread_mutex_init(&g_aircast_inst->srv_mutex, NULL);
    }
    g_aircast_inst->url_cid = -1;
    g_aircast_inst->dsc_func.dsc_aes_ctr_open = hccast_dsc_aes_ctr_open;
    g_aircast_inst->dsc_func.dsc_aes_cbc_open = hccast_dsc_aes_cbc_open;
    g_aircast_inst->dsc_func.dsc_ctx_destroy = hccast_dsc_ctx_destroy;
    g_aircast_inst->dsc_func.dsc_aes_decrypt = hccast_dsc_decrypt;
    g_aircast_inst->dsc_func.dsc_aes_encrypt = hccast_dsc_encrypt;

    if (aircast_cb)
    {
        g_aircast_inst->notifier = aircast_cb;
    }

    hccast_air_api_init();
    hccast_media_air_event_init(_hccast_air_player_handler);
    hccast_scene_air_event_init(hccast_air_api_event_notify);
    memset(g_aircast_inst->yt_m3u8, 0, sizeof(hccast_media_yt_m3u8_t) * MAX_M3U8_INFO_NUM);

    hccast_air_api_set_notifier(_hccast_air_event_handler);
    hccast_air_api_service_init();
#ifdef SUPPORT_AIRP2P    
    hccast_air_api_p2p_service_init();
#endif
    return 0;
}

int hccast_air_p2p_start(char *if_name, int ch)
{
#ifdef SUPPORT_AIRP2P
    pthread_mutex_lock(&g_aircast_inst->srv_mutex);
    if (g_aircast_inst->p2p_started)
    {
        hccast_log(LL_WARNING, "[%s]: airp2p has been start\n", __func__);
        pthread_mutex_unlock(&g_aircast_inst->srv_mutex);
        return 0;
    }

    hccast_air_api_p2p_start(if_name, ch);
    g_aircast_inst->p2p_started = 1;
    pthread_mutex_unlock(&g_aircast_inst->srv_mutex);
#endif
    return 0;
}

int hccast_air_p2p_stop(void)
{
#ifdef SUPPORT_AIRP2P
    pthread_mutex_lock(&g_aircast_inst->srv_mutex);
    if (!g_aircast_inst->p2p_started)
    {
        hccast_log(LL_WARNING, "[%s]: airp2p has been stop\n", __func__);
        pthread_mutex_unlock(&g_aircast_inst->srv_mutex);
        return 0;
    }

    hccast_air_api_p2p_stop();
    g_aircast_inst->p2p_started = 0;
    pthread_mutex_unlock(&g_aircast_inst->srv_mutex);
#endif
    return 0;
}

int hccast_air_p2p_channel_set(int channel)
{
    int ret = 0;
    
 #ifdef SUPPORT_AIRP2P
    ret = hccast_air_api_p2p_set_channel(channel);       
 #endif
 
    return ret;
}

int hccast_air_audio_state_get(void)
{
    return g_aircast_inst->audio_cnt;
}

int hccast_air_set_resolution(unsigned int width, unsigned int height, unsigned int fps)
{
    if (width && height && fps)
    {
        g_aircast_inst->res_width = width;
        g_aircast_inst->res_height = height;
        g_aircast_inst->res_fps = fps;

        return 0;
    }

    return -1;
}

int hccast_air_preemption_set(int enable)
{
    hccast_air_api_ioctl(AIRCAST_SET_PREEMPTION, (void *)enable, NULL);
    return 0;
}

int hccast_air_p2p_pin_set(char *pin, int pk_random)
{
    if (pin)
    {
        memset(g_aircast_inst->airp2p_pin, 0, sizeof(g_aircast_inst->airp2p_pin));
        snprintf(g_aircast_inst->airp2p_pin, sizeof(g_aircast_inst->airp2p_pin), "%s", pin);
        hccast_air_api_ioctl(AIRCAST_SET_AIRP2P_PIN, (void *)g_aircast_inst->airp2p_pin, pk_random);
    }

    return 0;
}
