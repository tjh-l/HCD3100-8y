#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __linux__
    #include <sys/prctl.h>
    #include <hccast/miracast_api.h>
#else
    #include <miracast/miracast_api.h>
#endif

#include <hccast_dsc.h>

#include "hccast_mira_avplayer.h"
#include "hccast_mira.h"
#include "hccast_wifi_mgr.h"
#include <hccast_scene.h>
#include <hccast_air.h>
#include <hccast_net.h>
#include <hccast_log.h>

static char g_cur_ssid[WIFI_MAX_SSID_LEN] = {0};
static int g_mira_start = 0;
static E_P2P_STATE g_mira_connect_stat = P2P_STATE_NONE;
static pthread_mutex_t g_mira_svr_mutex = PTHREAD_MUTEX_INITIALIZER;
static hccast_wifi_ap_info_t g_mira_cur_ap;

static bool g_mira_starting = false;
static int g_mira_restart_state = 0;

int hccast_mira_update_p2p_state(hccast_p2p_state_e state);

#ifdef HC_RTOS
    #include <linux/interrupt.h>
#endif

hccast_mira_event_callback mira_callback = NULL;

enum
{
    MIRA_WIFI_STAT_NONE,
    MIRA_WIFI_STAT_PREVIOUS,
    MIRA_WIFI_STAT_ONGOING,
    MIRA_WIFI_STAT_DISABLED,
};

static unsigned char g_mira_wifi_stat = MIRA_WIFI_STAT_NONE;

int hccast_mira_get_stat(void)
{
    int state;
    
    pthread_mutex_lock(&g_mira_svr_mutex);
    state = g_mira_start;
    pthread_mutex_unlock(&g_mira_svr_mutex);
    
    return state;
}

bool hccast_mira_is_starting(void)
{
    bool stat = g_mira_starting;
    hccast_log(LL_DEBUG, "g_mira_starting: %d\n", g_mira_starting);

    return stat;
}

int hccast_mira_get_restart_state(void)
{
    return g_mira_restart_state;
}

void *hccast_mira_wifi_reconnect_thread(void *arg)
{
    (void)arg;
#ifdef __linux__
    prctl(PR_SET_NAME, __func__);
#endif

    hccast_mira_service_stop();

    // netif
    char ap_ifname[32] = {0};
    char sta_ifname[32] = {0};
    hccast_wifi_mgr_get_ap_ifname(ap_ifname, sizeof(ap_ifname));
    hccast_wifi_mgr_get_sta_ifname(sta_ifname, sizeof(sta_ifname));

    hccast_net_set_if_updown(ap_ifname, HCCAST_NET_IF_DOWN);
    hccast_net_set_if_updown(sta_ifname, HCCAST_NET_IF_UP);

    hccast_log(LL_NOTICE, "mira reconnect ssid: %s\n", g_mira_cur_ap.ssid);
    int ret = hccast_wifi_mgr_connect(&g_mira_cur_ap);
    if (HCCAST_WIFI_ERR_USER_ABORT == ret)
    {
        pthread_mutex_lock(&g_mira_svr_mutex);
        hccast_wifi_mgr_p2p_set_connect_stat(false);
        pthread_mutex_unlock(&g_mira_svr_mutex);
        goto EXIT;
    }

    if (hccast_wifi_mgr_get_connect_status())
    {
        hccast_wifi_mgr_udhcpc_stop();
        hccast_wifi_mgr_udhcpc_start();
        pthread_mutex_lock(&g_mira_svr_mutex);
        hccast_wifi_mgr_p2p_set_connect_stat(false);
        pthread_mutex_unlock(&g_mira_svr_mutex);
    }
    else
    {
        pthread_mutex_lock(&g_mira_svr_mutex);
        hccast_wifi_mgr_p2p_set_connect_stat(false);
        pthread_mutex_unlock(&g_mira_svr_mutex);
        hccast_wifi_mgr_udhcpc_stop();
        hccast_wifi_mgr_disconnect();
    }

EXIT:
    g_mira_wifi_stat = MIRA_WIFI_STAT_NONE;
    g_mira_restart_state = 0;
    return NULL;
}

void *hccast_mira_hostap_enable_thread(void *arg)
{
    (void)arg;
#ifdef __linux__
    prctl(PR_SET_NAME, __func__);
#endif

    pthread_mutex_lock(&g_mira_svr_mutex);
    hccast_wifi_mgr_p2p_set_connect_stat(false);
    pthread_mutex_unlock(&g_mira_svr_mutex);

    int started = hccast_mira_get_stat();

    if (started) // solve service start after service stop.
    {
        hccast_mira_service_stop();
    }

    // netif
    char ap_ifname[32] = {0};
    char sta_ifname[32] = {0};
    hccast_wifi_mgr_get_ap_ifname(ap_ifname, sizeof(ap_ifname));
    hccast_wifi_mgr_get_sta_ifname(sta_ifname, sizeof(sta_ifname));

    hccast_net_set_if_updown(sta_ifname, HCCAST_NET_IF_DOWN);
    hccast_net_set_if_updown(ap_ifname, HCCAST_NET_IF_UP);

#ifdef __linux__
    hccast_wifi_mgr_hostap_start();
#else
    hccast_wifi_mgr_hostap_enable();
    hccast_wifi_mgr_udhcpd_stop();
    hccast_wifi_mgr_udhcpd_start();
#endif

    if (started) // solve service start after service stop.
    {
        hccast_mira_service_start();
    }

    g_mira_restart_state = 0;
    return NULL;
}

static void *hccast_mira_stop_services_thread(void *arg)
{
    (void)arg;
    hccast_log(LL_NOTICE, "hccast_mira_stop_services_thread run...\n");
    hccast_scene_mira_stop_services();
    return NULL;
}

int hccast_mira_wfd_event_cb(const wfd_status_t event, const void *data)
{
    (void)data;

    hccast_log(LL_INFO, "%s: wfd event: %d\n", __FUNCTION__, event);

    switch (event)
    {
        case WFD_SET_SSID_DONE:
            if (hccast_wifi_mgr_p2p_get_connect_stat())
            {
                hccast_mira_update_p2p_state(HCCAST_P2P_STATE_LISTEN);
            }

            if (mira_callback)
            {
                mira_callback(HCCAST_MIRA_SSID_DONE, NULL, NULL);
            }
            break;

        case WFD_CONNECTED:
            break;

        case WFD_DISCONNECTED:
        {
            if (mira_callback)
            {
                mira_callback(HCCAST_MIRA_DISCONNECT, NULL, NULL);
            }
            break;
        }

        case WFD_START_PLAYER:
        {
            if (mira_callback)
            {
                mira_callback(HCCAST_MIRA_START_DISP, NULL, NULL);
            }
            break;
        }

        case WFD_STOP_PLAYER:
        {
            if (mira_callback)
            {
                mira_callback(HCCAST_MIRA_STOP_DISP, NULL, NULL);
            }
            break;
        }

        case WFD_EVENT_UIBC_ENABLE:
        {
            if (mira_callback)
            {
                mira_callback(HCCAST_MIRA_UIBC_ENABLE, NULL, NULL);
            }
            break;
        }

        case WFD_EVENT_UIBC_DISABLE:
        {
            if (mira_callback)
            {
                mira_callback(HCCAST_MIRA_UIBC_DISABLE, NULL, NULL);
            }
            break;
        }

        default:
            break;
    }

    return 0;
}

int  hccast_mira_player_init()
{
    return mira_av_player_init();
}

static wfd_manage_func_t func =
{
    ._p2p_device_get_ip     = (void *)hccast_wifi_mgr_p2p_get_ip,
    ._p2p_device_event      = (void *)hccast_mira_wfd_event_cb,
    ._p2p_device_get_rtsp_port = (void *)hccast_wifi_mgr_p2p_get_rtsp_port,
};

int hccast_miracast_wfd_manage_init()
{
    miracast_ioctl(WFD_CMD_SET_MANAGE_FUNC, (unsigned long)&func, (unsigned long)0);

    return 0;
}

int hccast_miracast_service_init(hccast_mira_event_callback func)
{
    hccast_mira_event_callback callback = func;
    return 0;
}

int hccast_mira_update_p2p_state(hccast_p2p_state_e state)
{
    hccast_log(LL_DEBUG, "state: %d\n", state);

    char sta_ifname[32] = {0};
    char p2p_ifname[32] = {0};
    char ap_ifname[32]  = {0};

    hccast_wifi_mgr_get_sta_ifname(sta_ifname, sizeof(sta_ifname));
    hccast_wifi_mgr_get_p2p_ifname(p2p_ifname, sizeof(p2p_ifname));
    hccast_wifi_mgr_get_ap_ifname(ap_ifname, sizeof(ap_ifname));

    switch (state)
    {
        case HCCAST_P2P_STATE_NONE:
            miracast_update_p2p_status(P2P_STATE_NONE);
            break;

        case HCCAST_P2P_STATE_LISTEN:
            if (HCCAST_SCENE_MIRACAST == hccast_get_current_scene())
            {
                hccast_net_ifconfig(ap_ifname, HCCAST_HOSTAP_IP, HCCAST_HOSTAP_MASK, HCCAST_HOSTAP_GW);

                bool connected = hccast_wifi_mgr_p2p_get_connect_stat();
                hccast_log(LL_INFO, "mira connected: %d, last ssid: \"%s\"\n", connected, g_mira_cur_ap.ssid);

                if (connected)
                {
                    bool valid = false;
                    unsigned int wifi_connected = 0;

                    if (MIRA_WIFI_STAT_PREVIOUS == g_mira_wifi_stat)
                    {
                        wifi_connected = 1;
                    }
                    else if(MIRA_WIFI_STAT_DISABLED == g_mira_wifi_stat)
                    {
                        wifi_connected = 0;
                    }

                    if (mira_callback)
                    {
                        mira_callback(HCCAST_MIRA_RESET, &wifi_connected, &valid);
                    }

                    if (!valid)
                    {
                        if (MIRA_WIFI_STAT_PREVIOUS == g_mira_wifi_stat)
                        {
                            g_mira_wifi_stat = MIRA_WIFI_STAT_ONGOING;
                            pthread_t tid = 0;
                            pthread_attr_t thread_attr;

                            pthread_attr_init(&thread_attr);
                            pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
                            g_mira_restart_state = 1;

                            if (pthread_create(&tid, &thread_attr, (void *)hccast_mira_wifi_reconnect_thread, NULL) != 0)
                            {
                                g_mira_restart_state = 0;
                                hccast_log(LL_ERROR, "create hccast_mira_wifi_reconnect_thread failed!\n");
                            }

                            pthread_attr_destroy(&thread_attr);
                        }
                        else if (MIRA_WIFI_STAT_DISABLED == g_mira_wifi_stat)
                        {
                            pthread_t tid = 0;
                            pthread_attr_t thread_attr;

                            pthread_attr_init(&thread_attr);
                            pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
                            g_mira_restart_state = 1;

                            if (pthread_create(&tid, &thread_attr, (void *)hccast_mira_hostap_enable_thread, NULL) != 0)
                            {
                                g_mira_restart_state = 0;
                                hccast_log(LL_ERROR, "create hccast_mira_hostap_enable_thread failed!\n");
                            }

                            pthread_attr_destroy(&thread_attr);
                        }
                        else
                        {
                            hccast_wifi_mgr_p2p_set_connect_stat(false);
                        }
                    }
                    else
                    {
                        hccast_wifi_mgr_p2p_set_connect_stat(false);
                    }

                    if ((hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR) || (hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
                    {
                        hccast_log(LL_WARNING, "Cur scene is doing USB MIRROR\n");
                    }
                    else
                    {
                        hccast_scene_switch(HCCAST_SCENE_NONE);
                    }
                }

            }

            hccast_scene_set_mira_is_restarting(0);
            miracast_update_p2p_status(P2P_STATE_LISTEN);
            break;

        case HCCAST_P2P_STATE_CONNECTING:
            if (HCCAST_SCENE_MIRACAST != hccast_get_current_scene())
            {
                pthread_mutex_lock(&g_mira_svr_mutex);
                hccast_wifi_mgr_p2p_set_connect_stat(true);
                pthread_mutex_unlock(&g_mira_svr_mutex);

                if (mira_callback)
                {
                    mira_callback(HCCAST_MIRA_CONNECT, NULL, NULL);
                }

                memset(&g_mira_cur_ap, 0, sizeof(g_mira_cur_ap));
                if (mira_callback)
                {
                    mira_callback(HCCAST_MIRA_GET_CUR_WIFI_INFO, (void *)&g_mira_cur_ap, NULL);
                }

                hccast_log(LL_INFO, "%d: cur wifi connect ssid %s.\n", __LINE__, g_mira_cur_ap.ssid);
                if (strlen(g_mira_cur_ap.ssid) > 0)
                {
                    g_mira_wifi_stat = MIRA_WIFI_STAT_PREVIOUS;
#ifdef HC_RTOS
                    pthread_t tid = 0;
                    pthread_attr_t thread_attr;

                    pthread_attr_init(&thread_attr);
                    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
                    if (pthread_create(&tid, &thread_attr, (void *)hccast_mira_stop_services_thread, NULL) != 0)
                    {
                        hccast_log(LL_ERROR, "create hccast_mira_stop_services_thread failed!\n");
                    }

                    pthread_attr_destroy(&thread_attr);
#endif

                    hccast_net_ifconfig(sta_ifname, "129.0.0.1", "255.0.0.0", NULL);
                    hccast_net_ifconfig(p2p_ifname, HCCAST_P2P_IP, NULL, NULL);

                    hccast_wifi_mgr_udhcpc_stop();
                    hccast_wifi_mgr_disconnect();
                }
                else
                {
                    g_mira_wifi_stat = MIRA_WIFI_STAT_DISABLED;
#ifdef __linux__
                    hccast_wifi_mgr_hostap_stop();
#else
                    hccast_wifi_mgr_udhcpd_stop();
                    hccast_wifi_mgr_hostap_disenable();
#endif

                    hccast_net_ifconfig(ap_ifname, "129.0.0.1", "255.0.0.0", NULL);
                    hccast_net_ifconfig(p2p_ifname, HCCAST_P2P_IP, NULL, NULL);

                    hccast_scene_mira_stop_services();
                }

                hccast_scene_switch(HCCAST_SCENE_MIRACAST);
            }

            miracast_update_p2p_status(P2P_STATE_GONEGO_OK);
            break;

        case HCCAST_P2P_STATE_CONNECTED:
            if (mira_callback)
            {
                mira_callback(HCCAST_MIRA_CONNECTED, NULL, NULL);
            }
            break;

        case HCCAST_P2P_STATE_GOT_IP:
            miracast_update_p2p_status(P2P_STATE_PROVISIONING_DONE);
            if (mira_callback)
            {
                mira_callback(HCCAST_MIRA_GOT_IP, NULL, NULL);
            }
            break;

        case HCCAST_P2P_STATE_CONNECT_FAILED:
        case HCCAST_P2P_STATE_DISCONNECTED:
            miracast_update_p2p_status(P2P_STATE_IDLE);
            break;

        default:
            break;
    }

    return state;
}

/**
 * It disconnects the Miracast connection.
 *
 * @return 0
 */
int hccast_mira_disconnect()
{
    if (g_mira_start)
    {
        miracast_disconnect();
    }

    return 0;
}

/**
 * It stops the Miracast service.
 */
int hccast_mira_service_stop()
{
    hccast_scene_set_mira_restart_flag(0);

    pthread_mutex_lock(&g_mira_svr_mutex);
    if (hccast_mira_is_starting())
    {
        hccast_log(LL_NOTICE, "need waite mira started, then stop it!\n");
    }

    if (g_mira_start == 0)
    {
        pthread_mutex_unlock(&g_mira_svr_mutex);
        return 0;
    }

    g_mira_start = 0;
    hccast_log(LL_NOTICE, "%s mira stop begin.\n", __func__);
    miracast_stop();
    hccast_wifi_mgr_p2p_set_enable(false);
    pthread_mutex_unlock(&g_mira_svr_mutex);
    hccast_wifi_mgr_p2p_uninit();
    hccast_log(LL_NOTICE, "%s mira stop done.\n", __func__);
    return 0;
}

/**
 * It starts the Miracast service.
 *
 * @return 0: success; <0: failed.
 */
int hccast_mira_service_start()
{
    char dev_name[MIRA_NAME_LEN] = "hccast_mira";

    pthread_mutex_lock(&g_mira_svr_mutex);
    if (g_mira_start)
    {
        pthread_mutex_unlock(&g_mira_svr_mutex);
        return -1;
    }

    char ifname[32] = {0};
    hccast_wifi_mgr_get_p2p_ifname(ifname, sizeof(ifname));

    hccast_net_ifconfig(ifname, "128.0.0.1", "255.0.0.0", NULL);
    hccast_net_set_if_updown(ifname, HCCAST_NET_IF_UP);

#ifdef HC_RTOS
    int is_support_miracast = hccast_wifi_mgr_get_support_miracast();
    if (!is_support_miracast)
    {
        hccast_log(LL_NOTICE, "The wifi not support miracast!\n");
    }
#endif

    g_mira_starting = true;
    hccast_wifi_p2p_param_t p2p_param   = {0};

    if (mira_callback)
    {
        mira_callback(HCCAST_MIRA_GET_DEVICE_NAME, dev_name, NULL);
        mira_callback(HCCAST_MIRA_GET_DEVICE_PARAM, &p2p_param, NULL);
    }

    if (0 == strlen(p2p_param.device_name))
    {
        snprintf(p2p_param.device_name, sizeof(p2p_param.device_name), "%s", dev_name);
    }

    if (!p2p_param.func_update_state)
    {
        p2p_param.func_update_state = (void *)hccast_mira_update_p2p_state;
    }

    int ret = hccast_wifi_mgr_p2p_init(&p2p_param);
    if (ret)
    {
        hccast_log(LL_ERROR, "hc_wifi_service_p2p_init failed!\n");
        goto EXIT;
    }

    hccast_wifi_mgr_p2p_device_init();
    hccast_wifi_mgr_p2p_set_enable(true);
    miracast_start();
    g_mira_start = 1;
    hccast_log(LL_NOTICE, "%s mira start done.\n", __func__);

EXIT:
    g_mira_starting = false;
    pthread_mutex_unlock(&g_mira_svr_mutex);

    return 0;
}

int hccast_mira_service_init(hccast_mira_event_callback func)
{
    hccast_log(LL_NOTICE, "%s\n", miracast_get_version());

    hccast_miracast_wfd_manage_init();
    hccast_mira_player_init();
    //miracast_set_log_level(LL_INFO);

    mira_callback = func;
    return 0;
}

int hccast_mira_service_uninit()
{
    mira_callback = NULL;
    return 0;
}

int hccast_mira_service_set_resolution(hccast_mira_res_e res)
{
    if (HCCAST_MIRA_RES_1080P30 == res)
    {
        hccast_mira_set_default_resolution(WFD_1080P30);
    }
    else if (HCCAST_MIRA_RES_720P30 == res)
    {
        hccast_mira_set_default_resolution(WFD_720P30);
    }
    else if (HCCAST_MIRA_RES_480P60 == res)
    {
        hccast_mira_set_default_resolution(WFD_480P60);
    }
    else if (HCCAST_MIRA_RES_VESA1400 == res)
    {
        hccast_mira_set_default_resolution(WFD_VESA_1400);
    }
    else if (HCCAST_MIRA_RES_1080P60 == res)
    {
        hccast_mira_set_default_resolution(WFD_1080P60);
    }
    else if (HCCAST_MIRA_RES_1080F30 == res)
    {
        hccast_mira_set_default_resolution(WFD_1080F30);
    }
    else if (HCCAST_MIRA_RES_2160p30 == res)
    {
        hccast_mira_set_default_resolution(WFD_2160P30);
    }
    
    return 0;
}

int hccast_mira_uibc_get_port()
{
    return miracast_uibc_get_port();
}

int hccast_mira_uibc_get_supported(hccast_mira_cat_t *cat)
{
    int ret = miracast_uibc_get_supported();
    if (cat)
    {
        *cat = ret;
    }

    return ret;
}

int hccast_mira_uibc_add_device(hccast_mira_dev_t *dev)
{
    if (!dev)
        return -1;

    uibc_device_t uibc_device;

    uibc_device.cat = dev->cat;
    uibc_device.path = dev->path;
    uibc_device.type = dev->type;
    uibc_device.valid = dev->valid;

    return miracast_uibc_add_device(&uibc_device);
}

int hccast_mira_uibc_get_device(hccast_mira_dev_t *dev, int id)
{
    if (!dev || (id > UIBC_DEVICE_NUM_MAX && id < 0))
        return -1;

    uibc_capability_t caps;
    int ret = miracast_uibc_get_device(&caps);
    if (ret)
    {
        return -1;
    }

    dev->cat  = caps.uibc_devices[id].cat;
    dev->path = caps.uibc_devices[id].path;
    dev->type = caps.uibc_devices[id].type;
    dev->valid = caps.uibc_devices[id].valid;

    return 0;
}
int hccast_mira_uibc_disable_set(int *en)
{
    miracast_ioctl(WFD_CMD_SET_DISABLE_UIBC, (unsigned long)en, NULL);
    return 0;
}

int hccast_mira_uibc_disable_get(int *en)
{
    miracast_ioctl(WFD_CMD_GET_DISABLE_UIBC, NULL, (unsigned long)en);
    return 0;
}

int hccast_mira_uibc_is_supported_device(int cat, int dev)
{
    return miracast_uibc_is_supported_device(cat, dev);
}

