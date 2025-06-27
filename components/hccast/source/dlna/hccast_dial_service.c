#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <hccast_iptv.h>
#include <hccast_dial.h>
#include <hccast_dlna.h>
#include <hccast_media.h>
#include <hccast_log.h>
#include <hccast_scene.h>
#include <hccast_wifi_mgr.h>
#include <hccast_dial_api.h>

hccast_dial_event_callback dial_callback = NULL;
static bool g_dial_started = false;
static void *g_yt_inst = NULL;
static pthread_mutex_t g_dial_svr_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_is_seek = false;

static int dial_service_get_uri(hccast_iptv_info_req_st *info_req, hccast_iptv_links_st *links)
{
    (void)links;

    if (!info_req)
    {
        return -1;
    }

    for (int i = 0; i < 3; i++)
    {
        int yt_ret = hccast_iptv_link_fetch(g_yt_inst, info_req, NULL);

        if (!yt_ret && g_dial_started)
        {
            return 0;
        }
        else if (!g_dial_started)
        {
            return -1;
        }

        usleep(150 * 1000);
    }

    return 1;
}

int hccast_dial_ui_handle(dial_event_t *event, ...)
{
    if (!event)
    {
        return DIALR_PARAM_ERR;
    }

    switch (event->act)
    {
        case DIAL_EVENT_CONN_CONNECTING:
            hccast_log(LL_NOTICE, "dial has device connecting\n");

            if (dial_callback)
            {
                dial_callback(HCCAST_DIAL_CONN_CONNECTING, NULL, NULL);
            }

            hccast_scene_switch(HCCAST_SCENE_DIAL_PLAY);
            break;

        case DIAL_EVENT_CONN_CONNECTED:
            if (event->out)
            {
                const dial_device_conn_st *pDev = (dial_device_conn_st *)event->out;

                hccast_dial_device_conn_st dev = {0};
                snprintf(dev.device_name, sizeof(dev.device_name), "%s", pDev->device_name);
                snprintf(dev.device_id, sizeof(dev.device_id), "%s", pDev->device_id);
                dev.userAvatarUri = pDev->userAvatarUri;
                dev.userName      = pDev->userName;

                hccast_log(LL_NOTICE, "device connected success!\n");
                hccast_log(LL_NOTICE, "device name  : %s\n", dev.device_name);
                hccast_log(LL_INFO,   "device id    : %s\n", dev.device_id);
                hccast_log(LL_NOTICE, "user name    : %s\n", dev.userName ? dev.userName : "NULL");
                hccast_log(LL_INFO,   "userAvatarUri: %s\n", dev.userAvatarUri ? dev.userAvatarUri : "NULL");

                if (dial_callback)
                {
                    dial_callback(HCCAST_DIAL_CONN_CONNECTED, NULL, (void *)&dev);
                }

                hccast_scene_switch(HCCAST_SCENE_DIAL_PLAY);
            }

            break;

        case DIAL_EVENT_CONN_CONNECTED_FAILED:
            if (dial_callback)
            {
                dial_callback(HCCAST_DIAL_CONN_CONNECTED_FAILED, NULL, NULL);
            }

            hccast_scene_reset(HCCAST_SCENE_DIAL_PLAY, HCCAST_SCENE_NONE);
            break;

        case DIAL_EVENT_CONN_DISCONNECTING:
            break;

        case DIAL_EVENT_CONN_DISCONNECTED:
            if (event->out)
            {
                const dial_device_conn_st *pDev = (dial_device_conn_st *)event->out;

                hccast_dial_device_conn_st dev = {0};
                snprintf(dev.device_name, sizeof(dev.device_name), "%s", pDev->device_name);
                snprintf(dev.device_id, sizeof(dev.device_id), "%s", pDev->device_id);
                dev.userAvatarUri = pDev->userAvatarUri;
                dev.userName      = pDev->userName;

                hccast_log(LL_NOTICE, "device disconnected success!\n");
                hccast_log(LL_NOTICE, "device name  : %s\n", dev.device_name);
                hccast_log(LL_INFO,   "device id    : %s\n", dev.device_id);
                hccast_log(LL_NOTICE, "user name    : %s\n", dev.userName ? dev.userName : "NULL");
                hccast_log(LL_INFO,   "userAvatarUri: %s\n", dev.userAvatarUri ? dev.userAvatarUri : "NULL");

                if (dial_callback)
                {
                    dial_callback(HCCAST_DIAL_CONN_DISCONNECTED, NULL, (char *)event->out);
                }
            }

            break;

        case DIAL_EVENT_CONN_DISCONNECTED_FAILED:
            break;

        case DIAL_EVENT_CONN_DISCONNECTED_ALL:
        {
            int *flag = (int*)event->out;
            if (dial_callback)
            {
                dial_callback(HCCAST_DIAL_CONN_DISCONNECTED_ALL, NULL, NULL);
            }

            if (flag && *flag)
            {
                if (hccast_get_current_scene() == HCCAST_SCENE_DIAL_PLAY)
                {
                    hccast_scene_reset(HCCAST_SCENE_DIAL_PLAY, HCCAST_SCENE_NONE);
                }
            }
            else
            {
                if (HCCAST_MEDIA_STATUS_STOP == hccast_media_get_status())
                {
                    hccast_scene_reset(HCCAST_SCENE_DIAL_PLAY, HCCAST_SCENE_NONE);
                }
                else
                {
                   hccast_scene_switch(HCCAST_SCENE_NEED_REST);
                }
            }

            break;
        }
        case DIAL_EVENT_CONN_UNSUPPORTED:
            hccast_log(LL_WARNING, "DIAL_EVENT_CONN_UNSUPPORTED!\n");
            break;

        case DIAL_EVENT_USER_ADD_VIDEO:
            if (event->out)
            {
                const dial_device_conn_st *pDev = (dial_device_conn_st *)event->out;
                hccast_dial_device_conn_st dev = {0};
                snprintf(dev.device_name, sizeof(dev.device_name), "%s", pDev->device_name);
                snprintf(dev.device_id, sizeof(dev.device_id), "%s", pDev->device_id);
                dev.userAvatarUri = pDev->userAvatarUri;
                dev.userName      = pDev->userName;

                hccast_log(LL_NOTICE, "User add video!\n");
                hccast_log(LL_NOTICE, "user name    : %s\n", dev.userName ? dev.userName : "NULL");
                hccast_log(LL_INFO,   "userAvatarUri: %s\n", dev.userAvatarUri ? dev.userAvatarUri : "NULL");

                if (dial_callback)
                {
                    dial_callback(HCCAST_DIAL_USER_ADD_VIDEO, NULL, (char *)event->out);
                }
            }

            break;

        case DIAL_EVENT_USER_DEL_VIDEO:
            if (event->out)
            {
                const dial_device_conn_st *pDev = (dial_device_conn_st *)event->out;
                hccast_dial_device_conn_st dev = {0};
                snprintf(dev.device_name, sizeof(dev.device_name), "%s", pDev->device_name);
                snprintf(dev.device_id, sizeof(dev.device_id), "%s", pDev->device_id);
                dev.userAvatarUri = pDev->userAvatarUri;
                dev.userName      = pDev->userName;

                hccast_log(LL_NOTICE, "User delete video!\n");
                hccast_log(LL_NOTICE, "user name    : %s\n", dev.userName ? dev.userName : "NULL");
                hccast_log(LL_INFO,   "userAvatarUri: %s\n", dev.userAvatarUri ? dev.userAvatarUri : "NULL");

                if (dial_callback)
                {
                    dial_callback(HCCAST_DIAL_USER_DEL_VIDEO, NULL, (char *)event->out);
                }
            }

            break;

        case DIAL_EVENT_USER_ADD_PLAYLIST:
            if (event->out)
            {
                const dial_device_conn_st *pDev = (dial_device_conn_st *)event->out;
                hccast_dial_device_conn_st dev = {0};
                snprintf(dev.device_name, sizeof(dev.device_name), "%s", pDev->device_name);
                snprintf(dev.device_id, sizeof(dev.device_id), "%s", pDev->device_id);
                dev.userAvatarUri = pDev->userAvatarUri;
                dev.userName      = pDev->userName;

                hccast_log(LL_NOTICE, "User add playlist!\n");
                hccast_log(LL_NOTICE, "user name    : %s\n", dev.userName ? dev.userName : "NULL");
                hccast_log(LL_INFO,   "userAvatarUri: %s\n", dev.userAvatarUri ? dev.userAvatarUri : "NULL");

                if (dial_callback)
                {
                    dial_callback(HCCAST_DIAL_USER_ADD_PLAYLIST, NULL, (char *)event->out);
                }
            }

            break;

        case DIAL_EVENT_USER_DEL_PLAYLIST:
            if (event->out)
            {
                const dial_device_conn_st *pDev = (dial_device_conn_st *)event->out;
                hccast_dial_device_conn_st dev = {0};
                snprintf(dev.device_name, sizeof(dev.device_name), "%s", pDev->device_name);
                snprintf(dev.device_id, sizeof(dev.device_id), "%s", pDev->device_id);
                dev.userAvatarUri = pDev->userAvatarUri;
                dev.userName      = pDev->userName;

                hccast_log(LL_NOTICE, "User delete playlist!\n");
                hccast_log(LL_NOTICE, "user name    : %s\n", dev.userName ? dev.userName : "NULL");
                hccast_log(LL_INFO,   "userAvatarUri: %s\n", dev.userAvatarUri ? dev.userAvatarUri : "NULL");

                if (dial_callback)
                {
                    dial_callback(HCCAST_DIAL_USER_DEL_PLAYLIST, NULL, (char *)event->out);
                }
            }

            break;

        default:
            break;
    }

    return 0;
}

int hccast_dial_ctrl_handle(dial_event_t *event, ...)
{
    dialr_code_e rc = DIALR_OK;

    if (!event)
    {
        return DIALR_PARAM_ERR;
    }

    switch (event->act)
    {
        case DIAL_EVENT_CTRL_SET_WATCH_ID:
        {
            if (!event->out)
            {
                hccast_log(LL_ERROR, "set watch id is null!\n");
                rc = DIALR_PARAM_ERR;
                goto EXIT;
            }

            hccast_media_stop();
            hccast_iptv_info_req_st info_req = {0};
            snprintf(info_req.id, sizeof(info_req.id), "%s", (char *)event->out);

            if (!dial_service_get_uri(&info_req, NULL) && strlen(info_req.url))
            {
                hccast_log(LL_NOTICE, "parser url quality: (%d/%d)\n", info_req.cur_option, info_req.option);
                hccast_media_url_t mp_url;
                memset(&mp_url, 0, sizeof(hccast_media_url_t));
                char *a_url  = strstr(info_req.url, "a://");
                char *v_url  = strstr(info_req.url, "v://");
                char *av_url = strstr(info_req.url, "av://");
                char *hls_url = strstr(info_req.url, "hls://");

                if (a_url && v_url)
                {
                    *v_url = '\0';
                    mp_url.url  = v_url + strlen("v://");
                    hccast_log(LL_NOTICE, "video url:\n[%s]\n", mp_url.url);
                    mp_url.url1 = a_url + strlen("a://");
                    hccast_log(LL_NOTICE, "audio url:\n[%s]\n", mp_url.url1);
                }
                else if (hls_url)
                {
                    mp_url.url = info_req.url + strlen("hls://");
                    hccast_log(LL_NOTICE, "hls url:\n[%s]\n", mp_url.url);
                }
                else if (av_url)
                {
                    mp_url.url = info_req.url + strlen("av://");
                    hccast_log(LL_NOTICE, "av url:\n[%s]\n", mp_url.url);
                }
                else
                {
                    hccast_log(LL_NOTICE, "no url!\n\n");
                }

                hccast_iptv_info_node_st *info_node = info_req.extra_info;

                if (info_node)
                {
                    hccast_log(LL_INFO, "info_node:\n");
                    hccast_log(LL_INFO, "videId:     %s\n", info_node->id);
                    hccast_log(LL_INFO, "title:      %s\n", info_node->title);
                    hccast_log(LL_INFO, "duration:   %s\n", info_node->duration);
                    hccast_log(LL_INFO, "viewCount:  %ld\n", info_node->viewCount);

                    for (int i = 0; i < HCCAST_IPTV_THUMB_MAX; i++)
                    {
                        printf("thumbnail %d:  %s\n", i, info_node->thumb[i].url ? info_node->thumb[i].url : "NULL");
                    }
                }

                mp_url.url_mode = HCCAST_MEDIA_URL_DIAL;
                mp_url.media_type = HCCAST_MEDIA_MOVIE;
                hccast_media_seturl(&mp_url);
            }
            else
            {
                hccast_log(LL_ERROR, "parser watch url failed!\n");
                rc = DIALR_PARAM_ERR;
            }
        }
        break;

        case DIAL_EVENT_CTRL_SET_VOL:
        {
            int *vol = event->out;

            if (vol)
            {
                hccast_media_set_volume(*vol);
            }
        }
        break;

        case DIAL_EVENT_CTRL_GET_VOL:
        {
            if (event->in)
            {
                if (dial_callback)
                {
                    int vol = -1;
                    dial_callback(HCCAST_DIAL_CTRL_GET_VOL, NULL, &vol);
                    if (vol > -1)
                    {
                        *(int *) event->in = vol;
                    }
                    else
                    {
                        *(int *) event->in = hccast_media_get_volume();
                    }
                }
            }
        }
        break;

        case DIAL_EVENT_CTRL_GET_POS:
        {
            if (event->in)
            {
                *(long *) event->in = hccast_media_get_position();
            }
        }
        break;

        case DIAL_EVENT_CTRL_GET_DUR:
        {
            if (event->in)
            {
                *(long *) event->in = hccast_media_get_duration();
            }
        }
        break;

        case DIAL_EVENT_CTRL_GET_STAT:
        {
            int *stat = event->in;

            if (stat)
            {
                switch (hccast_media_get_status())
                {
                    case HCCAST_MEDIA_STATUS_PAUSED:
                        if (g_is_seek)
                        {
                            *stat = DIAL_STATUS_LOADING;
                        }
                        else
                        {
                            *stat = DIAL_STATUS_PAUSED;
                        }
                        break;

                    case HCCAST_MEDIA_STATUS_BUFFERING:
                        *stat = DIAL_STATUS_LOADING;
                        break;

                    case HCCAST_MEDIA_STATUS_PLAYING:
                        *stat = DIAL_STATUS_PLAYING ;
                        break;

                    case HCCAST_MEDIA_STATUS_STOP:
                    default:
                        *stat = DIAL_STATUS_STOPPED ;
                }

                g_is_seek = false;
            }
        }
        break;

        case DIAL_EVENT_CTRL_SEEK:
        {
            int64_t *seek_tm = event->out;

            if (seek_tm)
            {
                hccast_media_seek(*seek_tm);
                g_is_seek = true;
            }
        }
        break;

        case DIAL_EVENT_CTRL_STOP:
            hccast_media_stop();
            break;

        case DIAL_EVENT_CTRL_PLAY:
            hccast_media_resume();
            break;

        case DIAL_EVENT_CTRL_PAUSE:
            hccast_media_pause();
            break;

        case DIAL_EVENT_CTRL_UNSUPPORTED:
            hccast_log(LL_WARNING, "DIAL_EVENT_CTRL_UNSUPPORTED!\n");
            break;

        case DIAL_EVENT_CTRL_INVALID_CERT:
            if (dial_callback)
            {
                dial_callback(HCCAST_DIAL_CTRL_INVALID_CERT, NULL, NULL);
            }
            break;
    }

EXIT:
    return rc;
}

int hccast_dial_event_cb(dial_event_t *event, ...)
{
    (void)event;
    dialr_code_e rc = DIALR_OK;

    if (!event)
    {
        hccast_log(LL_ERROR, "dial event param error!\n");
        return DIALR_PARAM_ERR;
    }

    hccast_log(LL_INFO, "hccast_dial_event_cb type: %d, act: %d.\n", event->type, event->act);

    switch (event->type)
    {
        case DIAL_EVENT_UI:
        {
            rc = hccast_dial_ui_handle(event);
        }
        break;

        case DIAL_EVENT_CTRL:
        {
            rc = hccast_dial_ctrl_handle(event);
        }

        case DIAL_EVENT_NOTIFY:
            break;

        default:
            break;
    }

    return rc;
}

void hccast_iptv_yt_event_cb(hccast_iptv_evt_e evt, void *arg)
{
    (void)arg;

    return;
}

int hccast_dial_media_event(int type, void *param)
{
    (void)param;

    if (hccast_get_current_scene() == HCCAST_SCENE_DIAL_PLAY)
    {
        switch (type)
        {
            case HCCAST_MEDIA_DIAL_EVENT_FORBIDDEN:
                hccast_log(LL_WARNING, "dial media event forbidden!\n");
                hccast_iptv_handle_flush(g_yt_inst);
                hccast_media_stop();
                break;

            default:
                break;
        }
    }

    return 0;
}

int hccast_dial_service_start()
{
    int ret = -1;
    char service_name[DIAL_SERVICE_NAME_LEN] = "hccast_dial";
    pthread_mutex_lock(&g_dial_svr_mutex);

    if (g_dial_started)
    {
        hccast_log(LL_WARNING, "dial service has been started\n");
        goto EXIT;
    }

    if (!hccast_dlna_service_is_start())
    {
        hccast_log(LL_WARNING, "hccast_dial_service_start failed! dlna service must be started.\n");
        goto EXIT;
    }

    struct dial_svr_param dial_params =
    {
        .ifname = DIAL_BIND_IFNAME,
        .svrname = service_name,
        .svrport = DIAL_UPNP_PORT,
        .event_cb = &hccast_dial_event_cb,
    };

    if (dial_callback)
    {
        dial_callback(HCCAST_DIAL_GET_SVR_NAME, &service_name, NULL);
    }

    int hostap_status = -1;
    if (dial_callback)
    {
        dial_callback(HCCAST_DIAL_GET_HOSTAP_STATE, &hostap_status, NULL);
    }

    if (-1 == hostap_status)
    {
        if (hccast_wifi_mgr_get_hostap_status())
        {
            hccast_log(LL_WARNING, "hccast_dial_service_start failed! current is hostap mod!\n");
            goto EXIT;
        }
    }
    else if (hostap_status) // 1 or 2 in hostap
    {
        hccast_log(LL_WARNING, "hccast_dial_service_start failed! current is hostap mod!\n");
        goto EXIT;
    }

    if (g_yt_inst)
    {
        hccast_iptv_app_config_st conf = {0};
        hccast_iptv_config_get(g_yt_inst, &conf);
        conf.quality_option = HCCAST_IPTV_VIDEO_1080P;
        //conf.log_level = LL_NOTICE;
        hccast_iptv_app_init(g_yt_inst, &conf, hccast_iptv_yt_event_cb);
    }
    else
    {
        hccast_log(LL_ERROR, "hccast_iptv_app_open no open or open failed!");
        goto EXIT;
    }

    ret = hccast_dial_api_service_start(&dial_params);

    if (!ret)
    {
        g_dial_started = true;
        hccast_log(LL_NOTICE, "dial service started\n");
    }
    else
    {
        hccast_log(LL_NOTICE, "dial service start failed\n");
    }

EXIT:
    pthread_mutex_unlock(&g_dial_svr_mutex);
    return ret;
}

int hccast_dial_set_video_quality(unsigned char quality)
{
    hccast_iptv_app_config_st conf = {0};
    hccast_iptv_config_get(g_yt_inst, &conf);

    switch (quality)
    {
        case 1:
            conf.quality_option = HCCAST_IPTV_VIDEO_240P;
            break;

        case 2:
            conf.quality_option = HCCAST_IPTV_VIDEO_360P;
            break;

        case 3:
            conf.quality_option = HCCAST_IPTV_VIDEO_480P;
            break;

        case 4:
            conf.quality_option = HCCAST_IPTV_VIDEO_720P;
            break;

        case 5:
            conf.quality_option = HCCAST_IPTV_VIDEO_1080P;
            break;

        default:
            conf.quality_option = HCCAST_IPTV_VIDEO_720P;
    }

    return hccast_iptv_config_set(g_yt_inst, &conf);
}

int hccast_dial_service_stop()
{
    pthread_mutex_lock(&g_dial_svr_mutex);

    if (g_dial_started)
    {
        g_dial_started = false;
        hccast_iptv_handle_abort(g_yt_inst);
        hccast_dial_api_service_stop();
        hccast_iptv_app_deinit(g_yt_inst);
    }

    pthread_mutex_unlock(&g_dial_svr_mutex);
    return 0;
}

int hccast_dial_service_is_started()
{
    return g_dial_started;
}

int hccast_dial_service_init(hccast_dial_event_callback func)
{
    dial_callback = func;
    hccast_iptv_service_init(); // init all iptv service.
    hccast_iptv_service_app_init(HCCAST_IPTV_APP_YT); // init yt iptv service.
    g_yt_inst = hccast_iptv_app_open(HCCAST_IPTV_APP_YT);

    hccast_dial_api_init();
    hccast_dial_api_service_init(hccast_dial_event_cb);
    hccast_media_dial_event_init(hccast_dial_media_event);
    return 0;
}

int hccast_dial_service_uninit(void)
{
    dial_callback = NULL;
    hccast_iptv_app_deinit(g_yt_inst);
    return 0;
}
