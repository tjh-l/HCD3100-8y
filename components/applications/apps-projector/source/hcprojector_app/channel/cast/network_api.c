/*
network_api.c: use for network, include wifi, etc

 */

#include "app_config.h"

#ifdef WIFI_SUPPORT

#include <math.h>
#include <stdio.h> //printf()
#include <stdlib.h>
#include <string.h> //memcpy()
#include <unistd.h> //usleep()
#include <pthread.h>

#include <hccast/hccast_dhcpd.h>
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_httpd.h>

#include <hccast/hccast_media.h>
#include <hccast/hccast_net.h>
#include <hccast/hccast_com.h>

#include <cjson/cJSON.h>

#include "network_api.h"
#include <hcuapi/dis.h>
#include "cast_api.h"
#include "app_config.h"
#include "app_log.h"
#include "../wifi/wifi.h"
#include <arpa/inet.h>
#include "network_upg.h"

#define UUID_HEADER "HCcast"

#include "com_api.h"
#include "factory_setting.h"
#include "tv_sys.h"
#include "network_ping.h"
#include "setup.h"

wifi_config_t m_wifi_config = {0};

static char g_connecting_ssid[WIFI_MAX_SSID_LEN] = {0};

static int m_probed_wifi_module = 0;
static int hostap_connect_count = 0;
static int factary_init = 0;
static int hostap_discover_ok = 0;
static int m_airp2p_state = 0;//0 -- disable, 1 -- enable.
static int m_network_cur_state = 1;//0 -- disable, 1 -- enable.
static int hccast_service_need_start = 0;
static int m_wifi_resetting = 0;
static pthread_mutex_t m_wifi_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m_service_en_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m_wifi_switch_mode_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m_airp2p_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m_network_cur_state_mutex = PTHREAD_MUTEX_INITIALIZER;

static int wifi_mgr_callback_func(hccast_wifi_event_e event, void* in, void* out);

#ifdef HTTPD_SERVICE_SUPPORT
static int httpd_callback_func(hccast_httpd_event_e event, void* in, void* out);
#endif

static void media_callback_func(hccast_media_event_e msg_type, void* param);
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
static void hostap_config_init(void);
#endif
#endif

#if defined(AUTO_HTTP_UPGRADE) || defined(MANUAL_HTTP_UPGRADE)
static char m_http_upgrade_check_first = 0;
int network_upgrade_start();
#endif

int network_get_current_state(void)
{
    int state = 0;
    pthread_mutex_lock(&m_network_cur_state_mutex);
    state = m_network_cur_state;
    pthread_mutex_unlock(&m_network_cur_state_mutex);

    return state;
}

void network_set_current_state(int state)
{
    pthread_mutex_lock(&m_network_cur_state_mutex);
    m_network_cur_state = state;
    pthread_mutex_unlock(&m_network_cur_state_mutex);
}

int network_get_airp2p_state(void)
{
    int state = 0;
    pthread_mutex_lock(&m_airp2p_state_mutex);
    state = m_airp2p_state;
    pthread_mutex_unlock(&m_airp2p_state_mutex);

    return state;
}

void network_set_airp2p_state(int state)
{
    pthread_mutex_lock(&m_airp2p_state_mutex);
    m_airp2p_state = state;
    pthread_mutex_unlock(&m_airp2p_state_mutex);
}

char *app_get_connecting_ssid(void)
{
    return g_connecting_ssid;
}

void hccast_start_services(void)
{
    if (hccast_get_current_scene() != HCCAST_SCENE_NONE)
    {
        hccast_scene_switch(HCCAST_SCENE_NONE);
    }

    printf("[%s]  begin start services.\n", __func__);

#ifdef DLNA_SUPPORT
    hccast_dlna_service_stop();
    hccast_dlna_service_start();
#ifdef DIAL_SUPPORT
    hccast_dial_service_stop();
    hccast_dial_service_start();
#endif
#endif

#ifdef AIRCAST_SUPPORT    
    hccast_air_service_stop();
    hccast_air_service_start();
#endif

#ifdef MIRACAST_SUPPORT    
    hccast_mira_service_start();
#endif    
}

void hccast_stop_services(void)
{
    printf("[%s]  begin stop services.\n", __func__);

#ifdef DLNA_SUPPORT
    hccast_dlna_service_stop();
#ifdef DIAL_SUPPORT
    hccast_dial_service_stop();
#endif
#endif

#ifdef AIRCAST_SUPPORT        
    hccast_air_service_stop();
#endif

#ifdef MIRACAST_SUPPORT    
    hccast_mira_service_stop();
#endif    
}


static void hccast_ap_dlna_aircast_start(void)
{
#ifdef DLNA_SUPPORT    
    hccast_dlna_service_start();
#ifdef DIAL_SUPPORT
    hccast_dial_service_start();
#endif
#endif
#ifdef AIRCAST_SUPPORT
    hccast_air_service_start();
#endif
}

static void hccast_ap_dlna_aircast_stop(void)
{
#ifdef DLNA_SUPPORT
    hccast_dlna_service_stop();
#ifdef DIAL_SUPPORT
    hccast_dial_service_stop();
#endif
#endif
#ifdef AIRCAST_SUPPORT
    hccast_air_service_stop();
#endif
}

bool app_wifi_connect_status_get(void)
{
    return m_wifi_config.bConnected;
}

bool app_wifi_is_limited_internet(void)
{
    return m_wifi_config.bLimitedInternet;
}

void app_wifi_set_limited_internet(bool limited)
{
    m_wifi_config.bLimitedInternet = limited;
}

void hccast_service_need_start_flag(bool flag)
{
    hccast_service_need_start = flag;
}

#ifdef WIFI_SUPPORT

static int wifi_mgr_callback_func(hccast_wifi_event_e event, void* in, void* out)
{
    app_log(LL_NOTICE, "[%s] event: %d", __func__,event);
    control_msg_t ctl_msg = {0};

    switch (event)
    {
        case HCCAST_WIFI_SCAN:
        {
            //WiFi start scan...
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_SCANNING;
            api_control_send_msg(&ctl_msg);
            break;
        }
        case HCCAST_WIFI_SCAN_RESULT:
        {
            hccast_wifi_scan_result_t *res = (hccast_wifi_scan_result_t*)out;

            app_log(LL_INFO, "AP NUM: %d\n***********", res->ap_num);
            int j;
            wifi_list_set_zero();
            bool same_wifi = 0;
            uint16_t same_wifi_num = 1;
            wifi_scan_node *node;
            hccast_wifi_ap_info_t *info;

            /*Set all saved wifi signals to -1 before scanning, if the signal strength is -1, 
            the wifi is considered to be turned off and will not be displayed*/
            j = app_wifi_connect_status_get() ? 1 : 0;
            for (;j<sysdata_get_saved_wifi_count();j++)
            {
                info = sysdata_get_wifi_info_by_index(j);
                if(info)
                    info->quality = -1;
            }

            for (int i = 0; i < res->ap_num; i++)
            {
                app_log(LL_INFO, "ssid: %s, quality: %d", res->apinfo[i].ssid, res->apinfo[i].quality);
                if(strlen(res->apinfo[i].ssid) == 0){
                    continue;
                }
                if(wifi_list_update_is_enable())
                {
                    same_wifi_num = 1;
                    node = wifi_list_get_tail();
                    while(node){
                        if (strcmp(node->res.ssid, res->apinfo[i].ssid) == 0){
                            same_wifi_num = node->same_wifi_num + 1;
                            node->same_wifi_num = same_wifi_num;
#ifdef __LINUX__
                            if (node->res.quality < res->apinfo[i].quality)
#else
                            if (node->res.encryptMode != HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_SAE &&
                                    res->apinfo[i].encryptMode == HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_SAE)
                                same_wifi = 1;
                            else if (node->res.encryptMode == HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_SAE &&
                                    res->apinfo[i].encryptMode != HCCAST_WIFI_ENCRYPT_MODE_WPA2PSK_SAE)
                                wifi_list_remove(node, false);
                            else if (node->res.quality < res->apinfo[i].quality)
#endif
                                wifi_list_remove(node, false);
                            else
                                same_wifi = 1;
                            break;
                        } else{
                            node = node->prev;
                        }
                    }

                    if (same_wifi)
                    {
                        same_wifi = 0;
                        continue;
                    }

                    if((j=sysdata_get_wifi_index_by_ssid(res->apinfo[i].ssid)) < 0){
                        if(!wifi_list_mutex_lock()){
                            break;
                        }
                        node = wifi_list_get_tail();
                        while(node){
                            if(node->res.quality < res->apinfo[i].quality){
                                node = node->prev;
                            }else{
                                break;
                            }
                        }

                        if(node){
                            wifi_list_insert(&res->apinfo[i], same_wifi_num, node);
                        }else{
                            wifi_list_insert(&res->apinfo[i], same_wifi_num, (wifi_scan_node*)wifi_list_get_head());
                        }
                        wifi_list_mutex_unlock();
                    }else{
                        info = sysdata_get_wifi_info_by_index(j);
                        if(info){
                            info->quality = res->apinfo[i].quality;
                            info->encryptMode = res->apinfo[i].encryptMode;
                        }
                    }
                }
                else
                {
                    if((j=sysdata_get_wifi_index_by_ssid(res->apinfo[i].ssid)) >= 0)
                    {
                        info = sysdata_get_wifi_info_by_index(j);
                        if(info){
                            info->quality = res->apinfo[i].quality;
                            info->encryptMode = res->apinfo[i].encryptMode;
                        }
                    }
                }
            }
            projector_sys_param_save();

            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_SCAN_DONE;
            api_control_send_msg(&ctl_msg);
			
			//Do not free here, because the res buffer is not malloc here.
			//The buffer should be free after using hccast_wifi_mgr_scan(res)
			//free(res);

            break;
        }
        case HCCAST_WIFI_CONNECT:
        {
            memset(m_wifi_config.local_ip, 0, sizeof(m_wifi_config.local_ip));
            if (in!=NULL)
            {
                memcpy(g_connecting_ssid, (char*)in, strlen((char*)in));
            }
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTING;
            api_control_send_msg(&ctl_msg);
            break;
        }
        case HCCAST_WIFI_CONNECT_SSID:
        {
            app_log(LL_INFO, "SSID: %s", (char*)out);
            break;
        }
        case HCCAST_WIFI_CONNECT_RESULT: //station
        {
            hccast_udhcp_result_t* result = (hccast_udhcp_result_t*) out;
            if (result)
            {
                app_log(LL_NOTICE, "wifi connect result: %s!\n", result->stat ? "OK" : "fail");
                if (result->stat)
                {
                    app_log(LL_INFO, "ifname: %s", result->ifname);
                    app_log(LL_INFO, "last_ip addr: %s", result->last_ip);
                    app_log(LL_INFO, "ip addr: %s", result->ip);
                    app_log(LL_INFO, "mask addr: %s", result->mask);
                    app_log(LL_INFO, "gw addr: %s", result->gw);
                    app_log(LL_INFO, "dns: %s", result->dns);

                    hccast_net_ifconfig(result->ifname, result->ip, result->mask, result->gw);
                    hccast_net_set_dns(result->ifname, result->dns);

                    if((hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR) || (hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
                    {
                        printf("Cur scene is doing USB MIRROR\n");
                    }
                    else
                    {
                        pthread_mutex_lock(&m_service_en_mutex);
                        if (network_service_enable_get())
                        {
                            if (!m_wifi_config.bReconnect || strcmp(result->last_ip, result->ip) || hccast_service_need_start)
                            {
                                printf("hccast_start_services\n");
                                hccast_start_services();
                            }
                        }
                        pthread_mutex_unlock(&m_service_en_mutex);
                    }

                    m_wifi_config.sta_ip_ready = true;
                    m_wifi_config.bConnectedByPhone = false;
                    m_wifi_config.host_ap_ip_ready = false;
                    m_wifi_config.bConnected = true;
                    m_wifi_config.bReconnect = false;

                    strncpy(m_wifi_config.local_ip, result->ip, MAX_IP_STR_LEN);
                    snprintf(m_wifi_config.local_gw, MAX_IP_STR_LEN, "%s", result->gw);
                    wifi_is_reconning_set(false);
                    wifi_get_udhcp_result(result);
                    ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTED;
                    api_control_send_msg(&ctl_msg);

                    app_wifi_set_limited_internet(false);
                    app_ping_stop_thread();
                    app_ping_start_thread(m_wifi_config.local_gw);
        #ifdef AUTO_HTTP_UPGRADE
                    if(!network_upgrade_flag_get() && m_http_upgrade_check_first == 0)
                    {
                        m_http_upgrade_check_first = 1;
                        network_upgrade_start();
                    }
        #endif
                }
                else
                {
                    app_ping_stop_thread();
                    hccast_wifi_mgr_udhcpc_stop();
                    hccast_wifi_mgr_disconnect_no_message();

                    m_wifi_config.bConnected = false;  
                    m_wifi_config.bConnectedByPhone = false;
                    m_wifi_config.host_ap_ip_ready = false;
                    m_wifi_config.sta_ip_ready = false;
                    m_wifi_config.bReconnect = false;

                    memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
                    memset(m_wifi_config.local_ip, 0, sizeof(m_wifi_config.local_ip));

                    usleep(50*1000);
                    ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL;
                    api_control_send_msg(&ctl_msg);

                    if((hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR) || (hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
                    {
                        printf("Cur scene is doing USB MIRROR\n");
                    }
                    else
                    {
                        pthread_mutex_lock(&m_service_en_mutex);
                        if (network_service_enable_get())
                        {    
                            app_wifi_switch_work_mode(WIFI_MODE_AP);
                        #ifdef MIRACAST_SUPPORT
                            hccast_mira_service_stop();
                            hccast_mira_service_start();
                        #endif
                        }
                        pthread_mutex_unlock(&m_service_en_mutex);
                    }	
                    m_wifi_config.sta_ip_ready = false;
                    m_wifi_config.bConnected = false;
                    m_wifi_config.bConnectedByPhone = false;
                }
            }

            wifi_is_reconning_set(false);
            hccast_service_need_start = false;

            break;
        }

        case HCCAST_WIFI_DISCONNECT:
        {
            app_ping_stop_thread();
            m_wifi_config.bConnected = false;  
            m_wifi_config.bConnectedByPhone = false;
            m_wifi_config.bLimitedInternet = false;
            m_wifi_config.host_ap_ip_ready = false;
            m_wifi_config.sta_ip_ready = false;
            m_wifi_config.bReconnect = false;
            memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
            memset(m_wifi_config.local_ip, 0, sizeof(m_wifi_config.local_ip));
            memset(m_wifi_config.local_gw, 0, sizeof(m_wifi_config.local_gw));
            hccast_wifi_mgr_udhcpc_stop();
            if (hccast_wifi_mgr_p2p_get_connect_stat() == 0)
            {
                printf("%s Wifi has been disconnected, beging change to host ap mode\n",__func__);
                hccast_media_stop();
                hccast_ap_dlna_aircast_stop();
            #ifdef MIRACAST_SUPPORT
                hccast_mira_service_stop();
            #endif

                pthread_mutex_lock(&m_service_en_mutex);
                if (network_service_enable_get())
                {
                    app_wifi_switch_work_mode(WIFI_MODE_AP);
                #ifdef MIRACAST_SUPPORT
                    hccast_mira_service_stop();
                    hccast_mira_service_start();
                #endif
                }
                pthread_mutex_unlock(&m_service_en_mutex);
            }
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_DISCONNECTED;
            api_control_send_msg(&ctl_msg);
            break;
        }
        case HCCAST_WIFI_HOSTAP_OFFER:
        {
            if (out)
            {
                struct in_addr tmp_addr =
                {
                    .s_addr = (unsigned int)out
                };

                hostap_discover_ok = 1;

                app_log(LL_INFO, "addr: %s", inet_ntoa(tmp_addr));
                strncpy(m_wifi_config.connected_phone_ip, inet_ntoa(tmp_addr), MAX_IP_STR_LEN);
            }
            m_wifi_config.sta_ip_ready = false;
            m_wifi_config.bConnected = false;  
            //if (network_service_enable_get())
            //    hccast_ap_dlna_aircast_start();

            break;
        }

        case HCCAST_WIFI_RECONNECT:
        {
            printf("HCCAST_WIFI_RECONNECT\n");
            m_wifi_config.bConnected = false;
            wifi_is_reconning_set(true);
            m_wifi_config.bReconnect = true;
            app_wifi_reconnect(NULL, NETWORK_WIFI_CONNECTING_TIMEOUT);
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_RECONNECTE;
            api_control_send_msg(&ctl_msg);
            break;
        }
        case HCCAST_WIFI_RECONNECTED:
            printf("HCCAST_WIFI_RECONNECTED\n");
            m_wifi_config.bConnected = true;
            wifi_is_reconning_set(false);
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_RECONNECTED;
            api_control_send_msg(&ctl_msg);
            break;
        case HCCAST_WIFI_HOSTAP_ENABLE : 
            ctl_msg.msg_type = MSG_TYPE_NETWORK_HOSTAP_ENABLE;
            api_control_send_msg(&ctl_msg);
        default:
            break;
    }

    return 0;

}


char *app_wifi_local_ip_get(void)
{
    return m_wifi_config.local_ip;
}

char *app_wifi_connected_phone_ip_get(void)
{
    return m_wifi_config.connected_phone_ip;
}

static void *wifi_disconnect_thread(void *args)
{
    printf("----------------------------wifi_disconnect_thread is running.-----------------------------\n");
    hccast_wifi_mgr_udhcpc_stop();
    projector_sys_param_save();
    hccast_wifi_mgr_disconnect();
    return NULL;
}

static void *wifi_connect_thread(void *args)
{
    hccast_wifi_ap_info_t *ap_wifi = (hccast_wifi_ap_info_t*)args;
    hccast_wifi_ap_info_t conn_wifi_ap;
    printf("----------------------------wifi_connect_thread is running.-----------------------------\n");

    hccast_stop_services();

    app_wifi_switch_work_mode(WIFI_MODE_STATION);

    hccast_wifi_mgr_udhcpc_stop();

    memcpy(g_connecting_ssid, ap_wifi->ssid, sizeof(g_connecting_ssid));
    memcpy(&conn_wifi_ap, ap_wifi, sizeof(hccast_wifi_ap_info_t));
    if (HCCAST_NET_WIFI_ECR6600U == hccast_wifi_mgr_get_wifi_model() \
        && conn_wifi_ap.encryptMode != HCCAST_WIFI_ENCRYPT_MODE_OPEN_WEP \
        && conn_wifi_ap.encryptMode != HCCAST_WIFI_ENCRYPT_MODE_SHARED_WEP \
        )
        conn_wifi_ap.encryptMode = HCCAST_WIFI_ENCRYPT_MODE_NOT_HANDLE;
    int ret = hccast_wifi_mgr_connect(&conn_wifi_ap);
    if (HCCAST_WIFI_ERR_USER_ABORT == ret)
    {
        hccast_wifi_mgr_disconnect_no_message();
        m_wifi_config.bConnected = false;
        m_wifi_config.bConnectedByPhone = false;
        m_wifi_config.host_ap_ip_ready = false;
        m_wifi_config.sta_ip_ready = false;
        memset(m_wifi_config.local_ip, 0, MAX_IP_STR_LEN);
        memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));

        goto EXIT;
    }

    if (hccast_wifi_mgr_get_connect_status())
    {
        hccast_wifi_mgr_udhcpc_start();
        ap_wifi->quality = network_cur_wifi_quality_update();

        sysdata_wifi_ap_resave(ap_wifi);
    }
    else
    {
        hccast_wifi_mgr_disconnect_no_message();
        m_wifi_config.bConnected = false;  
        m_wifi_config.bConnectedByPhone = false;
        m_wifi_config.host_ap_ip_ready = false;
        m_wifi_config.sta_ip_ready = false;
        memset(m_wifi_config.local_ip, 0, MAX_IP_STR_LEN);
        memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
        
        if (api_get_wifi_pm_state() || network_get_airp2p_state())
        {
            goto EXIT;
        }
        
        usleep(50*1000);
        control_msg_t ctl_msg = {0};
        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL;
        api_control_send_msg(&ctl_msg);

        pthread_mutex_lock(&m_service_en_mutex);
        if(network_service_enable_get())
        {
            app_wifi_switch_work_mode(WIFI_MODE_AP);
        #ifdef MIRACAST_SUPPORT
            hccast_mira_service_start();
        #endif
        }
        pthread_mutex_unlock(&m_service_en_mutex);
    }

EXIT:
    free(ap_wifi);
    pthread_mutex_lock(&m_wifi_state_mutex);
    m_wifi_config.wifi_connecting = false;
    pthread_mutex_unlock(&m_wifi_state_mutex);
    return NULL;
}

static void *wifi_switch_mode_thread(void* arg)
{
    int wifi_ch = 0;
    hccast_wifi_freq_mode_e mode = (hccast_wifi_freq_mode_e)arg;

    sys_param_t *sys_param = projector_get_sys_param();

    if (HCCAST_WIFI_FREQ_MODE_24G == mode)
    {
        wifi_ch = sys_param->app_data.cast_setting.wifi_ch;
    }
    else if (HCCAST_WIFI_FREQ_MODE_5G == mode)
    {
        wifi_ch = sys_param->app_data.cast_setting.wifi_ch5g;
    }

    //hccast_wifi_mgr_hostap_switch_mode(mode);
    hccast_wifi_mgr_hostap_switch_mode_ex(mode, wifi_ch, 0);

    pthread_detach(pthread_self());

    return NULL;
}

static void *wifi_switch_hs_channel_thread(void* arg)
{
    hccast_mira_service_stop();
    hccast_wifi_mgr_hostap_switch_channel((int)arg);
#ifdef MIRACAST_SUPPORT
    hccast_mira_service_start();
#endif
    pthread_detach(pthread_self());

    return NULL;
}

int network_cur_wifi_quality_update(void){
    int quality = 100;

    if (!app_wifi_connect_status_get())
        return quality;

    hccast_wifi_ap_info_t *ap_wifi = sysdata_get_wifi_info_by_index(0);
    hccast_wifi_signal_poll_result_t ap_wifi_signal;
    hccast_wifi_mgr_get_signal_poll(&ap_wifi_signal);
#ifdef __linux__
    // 2 * (dBm + 100)
    ap_wifi->quality = ap_wifi_signal.rssi > -50 ? 100 : 2 * (ap_wifi_signal.rssi + 100);
#else
    if (ap_wifi_signal.rssi >= 0)
    {
        ap_wifi->quality = ap_wifi_signal.rssi > 50 ? 100 : 2 * ap_wifi_signal.rssi;
    }
    else
    {
        ap_wifi->quality = ap_wifi_signal.rssi > -50 ? 100 : 2 * (ap_wifi_signal.rssi + 100);
    }
#endif
    app_log(LL_INFO,"ssid : %s  quality : %d\n", ap_wifi->ssid, ap_wifi->quality);
    quality = ap_wifi->quality;

    projector_sys_param_save();
    return quality;
}

#endif

static int hccast_itoa(char * str, unsigned int val)
{
    char *p;
    char *first_dig;
    char temp;
    unsigned t_val;
    int len = 0;
    p = str;
    first_dig = p;

    do
    {
        t_val = (unsigned)(val % 0x0a);
        val   /= 0x0a;
        *p++ = (char)(t_val + '0');
        len++;
    }
    while (val > 0);
    *p-- = '\0';

    do
    {
        temp = *p;
        *p   = *first_dig;
        *first_dig = temp;
        --p;
        ++first_dig;
    }
    while (first_dig < p);
    return len;
}


#ifdef HTTPD_SERVICE_SUPPORT
static int httpd_callback_func(hccast_httpd_event_e event, void* in, void* out)
{
    int ret = 0;
    char mac[6];
    int cur_scene = 0;
    int last_resolution = 0;
    hccast_wifi_ap_info_t *save_ap = NULL;
    hccast_wifi_ap_info_t *con_ap = NULL;
    hccast_wifi_ap_info_t *check_ap = NULL;
    hccast_wifi_ap_info_t *del_ap = NULL;
    int index;

    sys_param_t *sys_param = projector_get_sys_param();
    int ap_tv_sys;
    hccast_wifi_ap_info_t *ap_wifi = NULL;
    unsigned int version;
	int temp;
	char cur_ssid[WIFI_MAX_SSID_LEN] = {0};
	hccast_wifi_scan_result_t *scan_res = NULL;
    control_msg_t msg = {0};

    switch (event)
    {
        case HCCAST_HTTPD_GET_DEV_PRODUCT_ID:
            strcpy((char*)in, (char*)projector_get_some_sys_param(P_DEV_PRODUCT_ID));
            printf("product_id: %s\n", (char*)in);
            break;
        case HCCAST_HTTPD_GET_DEV_VERSION:
            version = projector_get_some_sys_param(P_DEV_VERSION);
            hccast_itoa((char*)in,version);
            printf("version: %s\n",(char*)in);
            break;
        case HCCAST_HTTPD_GET_MIRROR_MODE:
            ret = projector_get_some_sys_param(P_MIRROR_MODE);
            break;
        case HCCAST_HTTPD_SET_MIRROR_MODE:
            projector_set_some_sys_param(P_MIRROR_MODE, (int)in);
            break;
        case HCCAST_HTTPD_GET_AIRCAST_MODE:
            ret = projector_get_some_sys_param(P_AIRCAST_MODE);
            break;
        case HCCAST_HTTPD_SET_AIRCAST_MODE:
			temp = (int)in;
			if(temp != projector_get_some_sys_param(P_AIRCAST_MODE))
			{
                projector_set_some_sys_param(P_AIRCAST_MODE, temp);
                projector_sys_param_save();
	            if(hccast_get_current_scene() == HCCAST_SCENE_NONE)
	            {
            #ifdef AIRCAST_SUPPORT
                    hccast_air_service_stop();
	                hccast_air_service_start();
            #endif
	            }
			}
            break;
        case HCCAST_HTTPD_GET_MIRROR_FRAME:
            ret = projector_get_some_sys_param(P_MIRROR_FRAME);
            break;
        case HCCAST_HTTPD_SET_MIRROR_FRAME:
            projector_set_some_sys_param(P_MIRROR_FRAME, (int)in);
            projector_sys_param_save();
            break;
        case HCCAST_HTTPD_GET_BROWSER_LANGUAGE:
            ret = projector_get_some_sys_param(P_BROWSER_LANGUAGE);
            break;
        case HCCAST_HTTPD_SET_BROWSER_LANGUAGE:
            projector_set_some_sys_param(P_BROWSER_LANGUAGE, (int)in);
            projector_sys_param_save();
            break;
        case HCCAST_HTTPD_GET_SYS_RESOLUTION:
            ret = projector_get_some_sys_param(P_SYS_RESOLUTION);
            break;
        case HCCAST_HTTPD_SET_SYS_RESOLUTION:
            ap_tv_sys = (int)in;
            if (ap_tv_sys != APP_TV_SYS_AUTO &&
                projector_get_some_sys_param(P_SYS_RESOLUTION) == ap_tv_sys)
            {
                printf("%s(), same tvsys:%d, not change TV sys\n",
                       __func__, ap_tv_sys);
                break;
            }

            last_resolution = projector_get_some_sys_param(P_SYS_RESOLUTION);

            ret = tv_sys_app_set(ap_tv_sys);

            if (API_SUCCESS == ret)
            {
                printf("%s(), line:%d. save app tv sys: %d!\n",
                       __func__, __LINE__, ap_tv_sys);
                projector_set_some_sys_param(P_SYS_RESOLUTION, ap_tv_sys);

                if(((last_resolution == APP_TV_SYS_4K)&&(ap_tv_sys != APP_TV_SYS_4K)) \
                   || ((last_resolution != APP_TV_SYS_4K)&&(ap_tv_sys == APP_TV_SYS_4K)) \
                   || ((last_resolution != APP_TV_SYS_AUTO)&&(ap_tv_sys == APP_TV_SYS_AUTO)) \
                   || ((last_resolution == APP_TV_SYS_AUTO)&&(ap_tv_sys != APP_TV_SYS_AUTO)) \
                  )
                {
                    cur_scene = hccast_get_current_scene();
                    if((cur_scene != HCCAST_SCENE_AIRCAST_PLAY) && (cur_scene != HCCAST_SCENE_AIRCAST_MIRROR))
                    {
                        if(hccast_air_service_is_start())
                        {
                        #ifdef AIRCAST_SUPPORT
                            hccast_air_service_stop();
                            hccast_air_service_start();
                        #endif
                        }
                    }
                }

                projector_sys_param_save();
            }
            break;
        case HCCAST_HTTPD_GET_DEVICE_MAC:
            api_get_mac_addr(mac);
            memcpy(in, mac,sizeof(mac));
            break;
        case HCCAST_HTTPD_GET_DEVICE_NAME:
            strcpy((char*)in, (char*)projector_get_some_sys_param(P_DEVICE_NAME));
            break;
        case HCCAST_HTTPD_SET_DEVICE_NAME:
            projector_set_some_sys_param(P_DEVICE_NAME, (int)in);
            projector_sys_param_save();
            api_sleep_ms(1000);
            api_system_reboot();
            msg.msg_type = MSG_TYPE_NETWORK_DEV_NAME_SET;
            api_control_send_msg(&msg);
            break;
        case HCCAST_HTTPD_GET_DEVICE_PSK:
            strcpy((char*)in, (char*)projector_get_some_sys_param(P_DEVICE_PSK));
            break;
        case HCCAST_HTTPD_SET_DEVICE_PSK:
            projector_set_some_sys_param(P_DEVICE_PSK, (int)in);
            projector_sys_param_save();
            api_sleep_ms(1000);
            api_system_reboot();
            break;
        case HCCAST_HTTPD_SET_SYS_RESTART:
            printf("HCCAST_HTTPD_SET_SYS_RESTART\n");
            api_system_reboot();
            break;
        case HCCAST_HTTPD_SET_SYS_RESET:
            printf("HCCAST_HTTPD_SET_SYS_RESET\n");
            factary_init = 1;
            projector_factory_reset();
            api_system_reboot();
            break;
        case HCCAST_HTTPD_WIFI_AP_DISCONNECT:
        {
            pthread_t tid;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, 0x2000);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
            if(pthread_create(&tid, &attr, wifi_disconnect_thread, NULL) != 0)
            {
                pthread_attr_destroy(&attr);
                return -1;
            }
            pthread_attr_destroy(&attr);

            sysdata_wifi_ap_set_nonauto(0);
            break;
        }
        case HCCAST_HTTPD_CHECK_AP_SAVE:
        {
            check_ap = (hccast_wifi_ap_info_t*)in;
            save_ap = (hccast_wifi_ap_info_t *)out;
            index =sysdata_check_ap_saved(check_ap);

            if(index >= 0)
            {
                if(save_ap)
                {
                    strcpy(save_ap->ssid, sys_param->app_data.cast_setting.wifi_ap[index].ssid);
                    strcpy(save_ap->pwd, sys_param->app_data.cast_setting.wifi_ap[index].pwd);
                }
                ret = 0;
            }
            else
            {
                ret = -1;
            }

            break;
        }
        case HCCAST_HTTPD_WIFI_AP_CONNECT:
        {
            con_ap = (hccast_wifi_ap_info_t *)in;
            if(con_ap)
            {
                ap_wifi = (hccast_wifi_ap_info_t *)malloc(sizeof(hccast_wifi_ap_info_t));
                if(ap_wifi)
                {
                    pthread_t tid;
                    pthread_attr_t attr;
                    
                    pthread_mutex_lock(&m_wifi_state_mutex);
                    memcpy(ap_wifi,con_ap,sizeof(hccast_wifi_ap_info_t));
                    
                    if (m_wifi_config.wifi_connecting || api_get_wifi_pm_state() || network_get_airp2p_state())
                    {
                        printf("wifi is connecting skip this time connect\n");
                        pthread_mutex_unlock(&m_wifi_state_mutex);
                        return -1;
                    }
                    
                    m_wifi_config.wifi_connecting = 1;
                    memcpy(g_connecting_ssid, con_ap->ssid, sizeof(g_connecting_ssid));
                    pthread_attr_init(&attr);
                    pthread_attr_setstacksize(&attr, 0x2000);
                    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                    if (pthread_create(&tid, &attr,wifi_connect_thread,(void*)ap_wifi) != 0)
                    {
                        printf("crate wifi_connect_thread fail.\n");
                        m_wifi_config.wifi_connecting = 0;
                        free(ap_wifi);
                        pthread_mutex_unlock(&m_wifi_state_mutex);
                        pthread_attr_destroy(&attr);
                        return -1;
                    } 

                    pthread_attr_destroy(&attr);
                    pthread_mutex_unlock(&m_wifi_state_mutex);
                }
                else
                {
                    return -1;
                }
            }
            break;
        }
        case HCCAST_HTTPD_DELETE_WIFI_INFO:
            del_ap = (hccast_wifi_ap_info_t *)in;
            if(del_ap)
            {
                index =sysdata_check_ap_saved(del_ap);
                printf("del_ap->ssid:%s, index: %d\n",del_ap->ssid,index);
                if(index >= 0)
                {
                    sysdata_wifi_ap_delete(index);
                    projector_sys_param_save();
                }
            }
            break;
        case HCCAST_HTTPD_GET_CUR_WIFI_INFO:
        {
            hccast_wifi_ap_info_t *cur_ap;
            network_cur_wifi_quality_update();
            ret = hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
            cur_ap = sysdata_get_wifi_info(cur_ssid);
            if(cur_ap)
                memcpy(in,cur_ap,sizeof(hccast_wifi_ap_info_t));
            break;
        }
        case HCCAST_HTTPD_SHOW_PROGRESS:
        {
            printf("progress: %d\n",(int)in);
            msg.msg_type = MSG_TYPE_UPG_DOWNLOAD_PROGRESS;
            msg.msg_code = (uint32_t)in;
            api_control_send_msg(&msg);
            break;
        }
        case HCCAST_HTTPD_GET_UPLOAD_DATA_START:
        {
        	hccast_stop_services();//stop all services, avoid download data abort.
        	
            msg.msg_type = MSG_TYPE_NET_UPGRADE;
            api_control_send_msg(&msg);
            api_sleep_ms(500);

            break;
        }
        case HCCAST_HTTPD_GET_UPLOAD_DATA_FAILED:
        {
        	hccast_start_services();//restart all services.
        	
        	msg.msg_type = MSG_TYPE_UPG_STATUS;
        	msg.msg_code = UPG_STATUS_SERVER_FAIL;
        	api_control_send_msg(&msg);
            break;
        }
        case HCCAST_HTTPD_MSG_UPGRADE_BAD_RES:
        {
            //http server return 4xx.
            break;
        }
        case HCCAST_HTTPD_MSG_UPLOAD_DATA_SUC:
        {
            hccast_web_upload_info_st* info = NULL;
            info = (hccast_web_upload_info_st*)in;
            if(info)
            {
                printf("upload len: %d\n",info->length);
                printf("upload buf: %p\n",info->buf);
                //free(info->buf);
            }
            sys_upg_flash_burn(info->buf, info->length);
            break;
        }
        case HCCAST_HTTPD_GET_WIFI_FREQ_MODE_EN:
            // aic 8800D 5G AP exist bug 
            if (HCCAST_NET_WIFI_8800D == hccast_wifi_mgr_get_wifi_model())
            {
                ret = 0;
            }
            else
            {
                if(HCCAST_WIFI_FREQ_MODE_5G == hccast_wifi_mgr_freq_support_mode() \
                   /*HCCAST_WIFI_FREQ_MODE_60G == hccast_wifi_mgr_freq_support_mode()*/)
                {
                    ret = 1;
                }
                else
                {
                    ret = 0;
                }
            }
            break;
        case HCCAST_HTTPD_GET_WIFI_FREQ_MODE:
            return hccast_wifi_mgr_get_current_freq_mode();
        case HCCAST_HTTPD_SET_WIFI_FREQ_MODE:
        {
            if ((int)in == projector_get_some_sys_param(P_WIFI_MODE))
            {
                return 0;
            }

            projector_set_some_sys_param(P_WIFI_MODE, (int)in);
            projector_sys_param_save();
            hccast_wifi_mgr_hostap_stop();
            api_system_reboot();
            break;
        }
        case HCCAST_HTTPD_GET_WIFI_HS_CHANNEL:
        {
            int ch = projector_get_some_sys_param(P_WIFI_CHANNEL);
            if (HOSTAP_CHANNEL_AUTO == ch)
            {
                return ch;
            }

#if 1       // rtos throught wpas get freq possible error.
            ch = hccast_wifi_mgr_get_current_freq();
#endif
            return ch;
        }
        case HCCAST_HTTPD_GET_WIFI_HS_CHANNEL_BY_FREQ_MODE:
        {
            if (HCCAST_WIFI_FREQ_MODE_24G == (int)in)
            {
                return projector_get_some_sys_param(P_WIFI_CHANNEL_24G);
            }
            else if (HCCAST_WIFI_FREQ_MODE_5G == (int)in)
            {
                return projector_get_some_sys_param(P_WIFI_CHANNEL_5G);
            }

            return projector_get_some_sys_param(P_WIFI_CHANNEL);
        }
        case HCCAST_HTTPD_SET_WIFI_HS_CHANNEL:
        {
            int wifi_ch = (int)in;
            if (wifi_ch == projector_get_some_sys_param(P_WIFI_CHANNEL))
            {
                return 0;
            }

            if ((wifi_ch >= 0 && wifi_ch <= 14) || (wifi_ch >= 34 && wifi_ch <= 196))
            {
                projector_set_some_sys_param(P_WIFI_CHANNEL, wifi_ch);
            }

            projector_sys_param_save();
#ifdef __linux__
#else
            int wifi_mode = projector_get_some_sys_param(P_WIFI_MODE);
            if (HOSTAP_CHANNEL_AUTO == wifi_ch)
            {
                //hccast_net_set_if_updown(HCCAST_HOSTAP_INF, HCCAST_NET_IF_UP);
                //hccast_wifi_mgr_trigger_scan(HCCAST_HOSTAP_INF);

                int *argv[2];
                hccast_wifi_mgr_get_best_channel(2, argv);
                hccast_wifi_mgr_set_best_channel(argv[0], argv[1]);

                if (1 == wifi_mode) // 24G
                {
                    wifi_ch = (int)argv[0];
                }
                else if (2 == wifi_mode) // 5G
                {
                    wifi_ch = (int)argv[1];
                }/*
                else if (3 == app_data->wifi_mode) // 6G
                {
                    wifi_ch = (int)argv[1];
                }*/
                else
                {
                    wifi_ch = (int)argv[0];
                }
            }
#endif
            api_sleep_ms(1000);
            api_system_reboot();
            break;
        }

        case HCCAST_HTTPD_GET_CUR_SCENE_PLAY:

			if(hccast_air_audio_state_get())
			{
				ret = 1;
			}
			else if(hccast_get_current_scene() != HCCAST_SCENE_NONE)
			{
				if((hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY) || ((hccast_get_current_scene() == HCCAST_SCENE_DLNA_PLAY)))
				{
					if(hccast_media_get_status() == HCCAST_MEDIA_STATUS_STOP)
					{
						ret = 0;
					}
					else
					{
						ret = 1;
					}
				}
				else
				{
					ret = 1;
				}
			}
			else
			{
				ret = 0;
			}

            break;
        case HCCAST_HTTPD_STOP_MIRA_SERVICE:
            if (!api_get_wifi_pm_state() || !network_get_airp2p_state())
            {
            #ifdef MIRACAST_SUPPORT
                if (network_service_enable_get())
                {
                    hccast_mira_service_stop();
                }
            #endif
            }
            break;
        case HCCAST_HTTPD_START_MIRA_SERVICE:
            if (!api_get_wifi_pm_state() || !network_get_airp2p_state())
            {
            #ifdef MIRACAST_SUPPORT
                if (network_service_enable_get())
                {
                    hccast_mira_service_start();
                }
            #endif
            }
            break;
	case HCCAST_HTTPD_GET_MIRROR_ROTATION:
		ret = projector_get_some_sys_param(P_MIRROR_ROTATION);
		break;
	case HCCAST_HTTPD_SET_MIRROR_ROTATION:
        temp = (int)in;
        if(projector_get_some_sys_param(P_MIRROR_ROTATION) != temp)
        {
            projector_set_some_sys_param(P_MIRROR_ROTATION, temp);
            projector_sys_param_save();
        #ifdef USBMIRROR_SUPPORT
            cast_usb_mirror_rotate_init();
        #endif
        }	

	    break;
	case HCCAST_HTTPD_GET_MIRROR_VSCREEN_AUTO_ROTATION:
		ret = projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION);
		break;
	case HCCAST_HTTPD_SET_MIRROR_VSCREEN_AUTO_ROTATION:
        temp = (int)in;
        if(projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION) != temp)
        {
            projector_set_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION, temp);
            projector_sys_param_save();
        #ifdef USBMIRROR_SUPPORT
            cast_usb_mirror_rotate_init();
        #endif
        }	

	    break;    
	    
        case HCCAST_HTTPD_GET_WIFI_CONNECT_STATUS:
            ret = hccast_wifi_mgr_get_connect_status();	
            break;
        case HCCAST_HTTPD_GET_CUR_WIFI_SSID:
            ret = hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
            memcpy(in,cur_ssid,sizeof(cur_ssid));
            break;
        case HCCAST_HTTPD_WIFI_SCAN	:
            scan_res = (hccast_wifi_scan_result_t*)in;
            if(scan_res)
            {
                m_wifi_config.wifi_scanning = 1;        
                hccast_wifi_mgr_scan(scan_res);
                m_wifi_config.wifi_scanning = 0;
            }
            else
            {
                printf("HCCAST_HTTPD_WIFI_SCAN error\n");
            }
            break;
        case HCCAST_HTTPD_WIFI_STAT_CHECK_BUSY:
            {
                ret = 0;
                ret |= hccast_wifi_mgr_is_connecting();
                ret |= hccast_wifi_mgr_is_scaning();
            }
            break;
    case HCCAST_HTTPD_GET_UPGRADE_URL:    
        sprintf(in, NETWORK_UPGRADE_URL, (char*)projector_get_some_sys_param(P_DEV_PRODUCT_ID));     
        break;
    case HCCAST_HTTPD_START_ONLINE_UPGRADE:
#ifdef NETWORK_SUPPORT    
        network_upg_set_user_abort(false);
        network_upg_start(in);
#endif        
        break;
    case HCCAST_HTTPD_MSG_USER_UPGRADE_ABORT:
#ifdef NETWORK_SUPPORT     
        if(network_upg_get_status() == NETWORK_UPG_DOWNLOAD)
        {
            network_upg_set_user_abort(1);
            ret = 1;
        }
#endif        
        break;     
    case HCCAST_HTTPD_GET_RESOLUTION_CONFIG_EN:      
        ret = -1;
        break;
    case HCCAST_HTTPD_GET_MIRROR_MODE_CONFIG_EN:    
        ret = -1;
        break;
#ifdef AIRP2P_SUPPORT        
    case HCCAST_HTTPD_GET_AIRP2P_CHANNEL:
        ret = projector_get_some_sys_param(P_AIRP2P_CH);
        break;
    case HCCAST_HTTPD_SET_AIRP2P_CHANNEL:
        temp = (int)in;
        if(projector_get_some_sys_param(P_AIRP2P_CH) != temp)
        {
            projector_set_some_sys_param(P_AIRP2P_CH, temp);
            projector_sys_param_save();
        }	
        break;     
#endif        
    case HCCAST_HTTPD_ALLOC_UPGRADE_BUFF:
    {
        hccast_web_upload_info_st* info = NULL;
        info = (hccast_web_upload_info_st*)in;
        info->buf = (char*)api_upgrade_buffer_alloc(info->length);
        break;
    }
    case HCCAST_HTTPD_FREE_UPGRADE_BUFF:
    {
        hccast_web_upload_info_st* info = NULL;
        info = (hccast_web_upload_info_st*)in;
        if (info && info->buf)
            api_upgrade_buffer_free(info->buf);       
        break;
    }
    default :
        ret = -1;
        break;
    }

    return ret;
}
#endif

#ifdef DLNA_SUPPORT
static void media_callback_func(hccast_media_event_e msg_type, void* param)
{
    control_msg_t ctl_msg = {0};
    ctl_msg.msg_code = (uint32_t)param;
    hccast_media_play_info_t *media_paly_info = NULL;
    int menu_status;

    switch (msg_type)
    {
        case HCCAST_MEDIA_EVENT_PARSE_END:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PARSE_END\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_CAST_PARSE_END;
            break;
        case HCCAST_MEDIA_EVENT_PLAYING:
            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_PLAY;
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PLAYING\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_PAUSE:
            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_PAUSE;
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PAUSE\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_BUFFERING:
            ctl_msg.msg_type = MSG_TYPE_MEDIA_BUFFERING;
            ctl_msg.msg_code = (uint32_t)param;
            //printf("[%s] %d   HCCAST_MEDIA_EVENT_BUFFERING, %d%%\n", __func__, __LINE__, (int)param);
            break;
        case HCCAST_MEDIA_EVENT_PLAYBACK_END:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PLAYBACK_END\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_STOP;
            break;
        case HCCAST_MEDIA_EVENT_VIDEO_DECODER_ERROR:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_VIDEO_DECODER_ERROR\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_MEDIA_VIDEO_DECODER_ERROR;
            break;
        case HCCAST_MEDIA_EVENT_AUDIO_DECODER_ERROR:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_AUDIO_DECODER_ERROR\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_MEDIA_AUDIO_DECODER_ERROR;
            break;
        case HCCAST_MEDIA_EVENT_VIDEO_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_VIDEO_NOT_SUPPORT\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_MEDIA_VIDEO_NOT_SUPPORT;
            ctl_msg.msg_code = (uint32_t)param;
            break;
        case HCCAST_MEDIA_EVENT_AUDIO_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_AUDIO_NOT_SUPPORT\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_MEDIA_AUDIO_NOT_SUPPORT;
            ctl_msg.msg_code = (uint32_t)param;
            break;
        case HCCAST_MEDIA_EVENT_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_NOT_SUPPORT\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_MEDIA_NOT_SUPPORT;
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_DLNA:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_DLNA, media_type: %d\n", __func__, __LINE__,(hccast_media_type_e)param);

            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_START;
            ctl_msg.msg_code = (uint32_t)param;
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_DIAL:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_DIAL, media_type: %d\n", __func__, __LINE__, (hccast_media_type_e)param);

            ctl_msg.msg_type = MSG_TYPE_CAST_DIAL_START;
            ctl_msg.msg_code = (uint32_t)param;
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST, media_type: %d\n", __func__, __LINE__,(hccast_media_type_e)param);
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRCAST_START;
            ctl_msg.msg_code = (uint32_t)param;
            break;
        case HCCAST_MEDIA_EVENT_URL_SEEK:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_SEEK  position: %ds\n", __func__, __LINE__, (int)param / 1000);
            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_SEEK;
            ctl_msg.msg_code = (uint32_t)param;
            break;
        case HCCAST_MEDIA_EVENT_SET_VOLUME:
            ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_VOL_SET;
            ctl_msg.msg_code = (uint32_t)param;
            app_log(LL_INFO, "[%s] %d   HCCAST_MEDIA_EVENT_SET_VOLUME  volume: %d\n", __func__, __LINE__, (int)param);
            break;
        case HCCAST_MEDIA_EVENT_GET_MIRROR_ROTATION:
            *(int*)param = projector_get_some_sys_param(P_MIRROR_ROTATION);           
            break;  
        case HCCAST_MEDIA_EVENT_GET_FLIP_MODE:
        {
            int flip_mode;
            int rotate;     
            int flip;
            api_get_flip_info(&rotate, &flip);
            flip_mode = (rotate & 0xffff) << 16 | (flip & 0xffff);
            *(int*)param = flip_mode;
            break;
        }
        case HCCAST_MEDIA_EVENT_URL_START_PLAY:
        {   
            media_paly_info = (hccast_media_play_info_t *)param;
            media_paly_info->interface_valid = 1;
            ctl_msg.msg_code = (uint32_t)media_paly_info->media_type;

            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_START_PLAY, media_type: %d, url_mode:%d\n", \
                __func__, __LINE__,media_paly_info->media_type, media_paly_info->url_mode);
            if(media_paly_info->url_mode == HCCAST_MEDIA_URL_DLNA)
            {
                ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_START;
            }
            else if(media_paly_info->url_mode == HCCAST_MEDIA_URL_AIRCAST)
            {
                ctl_msg.msg_type = MSG_TYPE_CAST_AIRCAST_START;
            }
            else if(media_paly_info->url_mode == HCCAST_MEDIA_URL_DIAL)
            {
                ctl_msg.msg_type = MSG_TYPE_CAST_DIAL_START;
            }

            break;
        }
#ifdef SYS_ZOOM_SUPPORT        
        case HCCAST_MEDIA_EVENT_GET_ZOOM_INFO:
        {   
            hccast_media_zoom_info_t *zoom_info = (hccast_media_zoom_info_t*)param;

            if(projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT) == 0)
            {
                zoom_info->enable = 0;
            }
            else
            {
                zoom_info->enable = 1;
            }
            
            zoom_info->src_rect.x = DIS_SOURCE_FULL_X;
            zoom_info->src_rect.y = DIS_SOURCE_FULL_Y;
            zoom_info->src_rect.w = DIS_SOURCE_FULL_W;
            zoom_info->src_rect.h = DIS_SOURCE_FULL_H;
            zoom_info->dst_rect.x = get_display_x();
            zoom_info->dst_rect.y = get_display_y();
            zoom_info->dst_rect.w = get_display_h();
            zoom_info->dst_rect.h = get_display_v();
            break;
        }
#endif
        case HCCAST_MEDIA_EVENT_GET_VIDEO_CONFIG:
        {   
            hccast_com_video_config_t *video_config = (hccast_com_video_config_t*)param;
            if (api_video_pbp_get_support()){
                video_config->video_pbp_mode = HCCAST_COM_VIDEO_PBP_2P_ON;

                if (APP_DIS_TYPE_HD == api_video_pbp_get_dis_type(VIDEO_PBP_SRC_DLNA))
                    video_config->video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_HD;
                else
                    video_config->video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_UHD;

                if (APP_DIS_LAYER_MAIN == api_video_pbp_get_dis_layer(VIDEO_PBP_SRC_DLNA))
                    video_config->video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_MAIN;
                else
                    video_config->video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_AUXP;                

            }
            break;
        }        
        default:
            break;
    }

    if (0 != ctl_msg.msg_type)
        api_control_send_msg(&ctl_msg);

    if (MSG_TYPE_CAST_DLNA_STOP == ctl_msg.msg_type)
    {
        //While air mirror preempt dlna(air play->air mirror), dlna stop and air mirror start,
        //sometime it is still in dlna play UI(not exit to win root UI),
        //the next air mirror play is starting, then the UI/logo may block the air mirror playing.
        //So here exit callback function wait for win cast root UI opening
        // printf("[%s] wait cast root start tick: %d\n",__func__,(int)time(NULL));
        cast_set_drv_hccast_type(CAST_TYPE_NONE);
        if (cast_main_ui_wait_ready)
            cast_main_ui_wait_ready(20000);
        // printf("[%s] wait cast root end tick: %d\n",__func__,(int)time(NULL));
    }

    if ((MSG_TYPE_CAST_DLNA_START == ctl_msg.msg_type) \
        || (MSG_TYPE_CAST_AIRCAST_START == ctl_msg.msg_type) \
        || (MSG_TYPE_CAST_DIAL_START == ctl_msg.msg_type) )
    {
        //Wait the win dlna is open, then start play to avoid the OSD
        // close slowly.
        // printf("[%s] wait dlna menu open start tick: %d\n",__func__,(int)time(NULL));
        cast_set_drv_hccast_type(CAST_TYPE_DLNA);
        if (dlna_ui_wait_ready)
        {
            menu_status = dlna_ui_wait_ready(20000);
            if(media_paly_info && !menu_status)
            {
                media_paly_info->enable_url_play = 0;
            }
        }    
        // printf("[%s] wait dlna menu open end tick: %d\n",__func__,(int)time(NULL));
    }
}
#endif


#ifdef WIFI_SUPPORT

#ifdef __linux__

static int is_file_exist(char *file_name)
{
    if(access(file_name,F_OK) == 0){
        return 1;
    }else{
        //printf("%s is not exist\n", file_name);
        return 0;
    }
}

static int network_wifi_is_ready(void)
{
    int ready = 0;

    if (
        // todo: Universal judgment
        is_file_exist("/var/run/hostapd/wlan0") && 
        is_file_exist("/var/run/wpa_supplicant/p2p0") && 
        (is_file_exist("/var/run/wpa_supplicant/wlan0")
        || is_file_exist("/var/run/wpa_supplicant/wlan1"))
       ){
        ready = 1;
    }else{
        ready = 0;
    }

    return ready;
}

#endif

void network_wifi_module_set(int wifi_module)
{
    m_probed_wifi_module = wifi_module;
    hccast_wifi_mgr_set_wifi_model(wifi_module);
}

int network_wifi_module_get(void)
{
    if (0 == m_probed_wifi_module)
        return 0;

    int wifi_module = m_probed_wifi_module;    
#ifdef __linux__
    if (0 == network_wifi_is_ready())
        wifi_module = 0;
#else
    unsigned char mac[6] = {0};
    if (0 != api_get_mac_addr((char*)mac))
        wifi_module = 0;   
#endif
    return wifi_module;
}

int hostap_get_connect_count(void)
{
    return hostap_connect_count;
}

static void hostap_set_connect_count(int count)
{
    hostap_connect_count = count;
}


static int needfresh = 0;
static int hostap_connect_update(void)
{
    int cur_connect = 0;
    int pre_connect = 0;
    control_msg_t ctl_msg = {0};

    //hostap not ready.
    if((hccast_wifi_mgr_get_hostap_status() == 0) || hccast_wifi_mgr_p2p_get_connect_stat() \
        || factary_init || api_get_wifi_pm_state() || network_get_airp2p_state())
    {
        needfresh = 0;
        hostap_set_connect_count(0);
        return 0;
    }


    if(hccast_wifi_mgr_p2p_get_connect_stat())
    {
        //printf("########%s mira connect do not anything.#########\n", __FUNCTION__);
        return 0;
    }

    cur_connect = hccast_wifi_mgr_hostap_get_sta_num(NULL);
    pre_connect = hostap_get_connect_count();

#if 0
    if(hostap_discover_ok == 0)
    {
        //wait real connect success at first time.
        cur_connect = 0;
        pre_connect = 0;
    }
#endif

    if (cur_connect != pre_connect)
    {
        printf("########%s cur_connect=%d pre_connect=%d#########\n", __FUNCTION__, cur_connect, pre_connect);
        hostap_set_connect_count(cur_connect);
        if (cur_connect == 0)
        {
            //send msg notify ui no phone had connect to dongle.
            //if (needfresh == 1)
            {
                printf("====================== no phone connect===============\n");
                ctl_msg.msg_type = MSG_TYPE_NETWORK_DEVICE_BE_DISCONNECTED;
                api_control_send_msg(&ctl_msg);
                m_wifi_config.bConnectedByPhone = false;
                m_wifi_config.host_ap_ip_ready = false;


                needfresh = 0;
                hostap_discover_ok = 0;
                hccast_ap_dlna_aircast_stop();
            }
        }
        else
        {
            //send msg notify ui have phone connect to dongle.
            printf("====================== new phone connect===============\n");
            if(needfresh == 0)
            {
                needfresh = 1;
                if (network_service_enable_get())
                    hccast_ap_dlna_aircast_start();
            }  
            ctl_msg.msg_type = MSG_TYPE_NETWORK_DEVICE_BE_CONNECTED;
            m_wifi_config.bConnectedByPhone = true;
            m_wifi_config.host_ap_ip_ready = true;
            api_control_send_msg(&ctl_msg); 
        }
    }

    return 0;
}

static void *hostap_connect_thread(void *args)
{
    control_msg_t ctl_msg = {0};

    printf("----------------------------hccast_hostap_connect_thread is running.-----------------------------\n");

    while (1)
    {
        hostap_connect_update();
        if (cast_get_p2p_switch_enable() && !m_wifi_resetting && cast_detect_p2p_exception())
        {
            printf("%s Airp2p reset\n", __func__);
            m_wifi_resetting = 1;
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRP2P_RESET;
            api_control_send_msg(&ctl_msg); 
        }
    
        api_sleep_ms(500);
    }
    
    return NULL;
}


static void hostap_connect_detect_init()
{
    pthread_t tid;

    if (pthread_create(&tid, NULL,hostap_connect_thread, NULL) < 0)
    {
        printf("Create hccast_hostap_connect_thread error.\n");
    }
}
#endif

int network_init(void)
{
    static int m_network_inited = 0;
    if (m_network_inited)
    {
        return API_SUCCESS;
    }
    
    m_network_inited = 1; 
    
#ifdef WIFI_SUPPORT

    udhcp_conf_t udhcpd_conf =
    {
        .func               = NULL, // use hccast default cb, otherwise, use user defined cb
        .ifname             = UDHCP_IF_WLAN0,
        .ip_start_def       = APP_HOSTAP_IP_START_ADDR,
        .ip_end_def         = APP_HOSTAP_IP_END_ADDR,
        .ip_host_def        = APP_HOSTAP_LOCAL_IP_ADDR,
        .subnet_mask_def    = APP_HOSTAP_MASK_ADDR,
    };

    hccast_wifi_mgr_set_udhcpd_conf(udhcpd_conf);

    udhcp_conf_t udhcpc_conf =
    {
        .func   = NULL, // use hccast default cb, otherwise, use user defined cb
        .ifname = UDHCP_IF_WLAN0,
        .option = UDHCPC_ABORT_IF_NO_LEASE,
    };

    if (HCCAST_NET_WIFI_8800D == network_wifi_module_get())
    {
        udhcpc_conf.ifname = UDHCP_IF_WLAN1;
    }

    hccast_wifi_mgr_set_udhcpc_conf(udhcpc_conf);

    hccast_wifi_mgr_init(wifi_mgr_callback_func);
#endif

#ifdef HTTPD_SERVICE_SUPPORT
    hccast_httpd_service_init(httpd_callback_func);
#endif

#ifdef DLNA_SUPPORT
    hccast_media_init(media_callback_func);
#endif

    cast_init();
#ifdef WIFI_SUPPORT
    hostap_connect_detect_init();
#endif

#ifdef HTTPD_SERVICE_SUPPORT
    if(network_service_enable_get())
    {
        hccast_httpd_service_start();
    }    
#endif

    return API_SUCCESS;
}

int network_deinit(void)
{
#ifdef WIFI_SUPPORT
    hccast_wifi_mgr_uninit();
#endif

#ifdef HTTPD_SERVICE_SUPPORT
    hccast_httpd_service_uninit();
#endif    

#ifdef MIRACAST_SUPPORT
    hccast_mira_service_uninit();
#endif
#ifdef DLNA_SUPPORT
    hccast_dlna_service_uninit();
#endif    
    return API_SUCCESS;
}

int app_wifi_init()
{
    //memset(&m_wifi_config, 0, sizeof(m_wifi_config));

    hccast_wifi_mgr_init(wifi_mgr_callback_func);

    return 0;
}

int app_wifi_deinit()
{
    m_wifi_config.bConnected = false;
    memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
    hccast_wifi_mgr_op_abort();
    app_wifi_switch_work_mode(WIFI_MODE_NONE);
    hccast_wifi_mgr_uninit();

    return 0;
}

#ifdef WIFI_SUPPORT
static void hostap_config_init(void)
{
    hccast_wifi_hostap_conf_t conf = {0};

    int wifi_mode = hccast_wifi_mgr_freq_support_mode();

    // aic 8800D 5G AP exist bug 
    if (HCCAST_NET_WIFI_8800D == hccast_wifi_mgr_get_wifi_model())
    {
        wifi_mode = HCCAST_WIFI_FREQ_MODE_24G;
    }

    //! The wpas conf will be overwritten here
    conf.mode    = projector_get_some_sys_param(P_WIFI_MODE);
    if (conf.mode <= wifi_mode)
    {
        //conf.mode = projector_get_some_sys_param(P_WIFI_MODE);
        conf.channel = projector_get_some_sys_param(P_WIFI_CHANNEL);
    }
    else
    {
        conf.mode = wifi_mode;

        if (HOSTAP_CHANNEL_AUTO != projector_get_some_sys_param(P_WIFI_CHANNEL))
        {
            if (HCCAST_WIFI_FREQ_MODE_24G == conf.mode)
            {
                conf.channel = projector_get_some_sys_param(P_WIFI_CHANNEL_24G);
            }
            else if (HCCAST_WIFI_FREQ_MODE_5G == conf.mode)
            {
                conf.channel = projector_get_some_sys_param(P_WIFI_CHANNEL_5G);
            }
            else
            {
                conf.channel = projector_get_some_sys_param(P_WIFI_CHANNEL);
            }
        }
    }

    app_log(LL_NOTICE, "mode: %d/%d, ch:%d\n", wifi_mode, conf.mode, conf.channel);

    //strcpy(conf.country_code, "CN");
    strncpy(conf.pwd, (char*)projector_get_some_sys_param(P_DEVICE_PSK), sizeof(conf.pwd));
    strncpy(conf.ssid, (char*)projector_get_some_sys_param(P_DEVICE_NAME), sizeof(conf.ssid));

#ifdef __linux__
    hccast_wifi_mgr_hostap_set_conf(&conf);
#else
    hccast_wifi_mgr_hostap_store_conf(&conf);
#endif
}

bool app_get_wifi_init_done(void)
{
    return m_wifi_config.wifi_init_done;
}

void app_set_wifi_init_done(int state)
{
    m_wifi_config.wifi_init_done = state;
}

#ifdef AIRP2P_SUPPORT
int network_airp2p_start(void)
{
    char hostname[64] = {0};
    control_msg_t ctl_msg = {0};
    int channel = projector_get_some_sys_param(P_AIRP2P_CH);

    pthread_mutex_lock(&m_airp2p_state_mutex);
    if (m_airp2p_state)
    {
        sysdata_init_device_name();
        cast_airp2p_enable(1);
        ctl_msg.msg_type = MSG_TYPE_CAST_AIRP2P_READY;
        printf("%s %d.channel: %d\n", __func__, __LINE__, channel);
        sprintf((char *)hostname, "%s_%s", projector_get_some_sys_param(P_DEVICE_NAME), AIRP2P_NAME);
        sethostname(hostname, strlen(hostname));
        hccast_air_set_resolution(1728, 972, 60);
        hccast_air_p2p_start(AIRP2P_INTF, channel);
        hccast_air_service_start();
        api_control_send_msg(&ctl_msg);
        if (cast_get_p2p_switch_enable())
        {
            network_init();
#ifdef __linux__
            hccast_mira_service_start();
            cast_p2p_switch_thread_start();
#else
            hccast_wifi_wpa_start();
            hccast_mira_service_start();
#endif
        } 
    }
    pthread_mutex_unlock(&m_airp2p_state_mutex);

    cast_reset_p2p_exception();
    m_wifi_resetting = 0;
    
    return 0;
}

int network_airp2p_stop(void)
{
    hccast_air_service_stop();
    hccast_air_p2p_stop();
    //cast_set_wifi_p2p_state(0);

    if (cast_get_p2p_switch_enable())
    {
#ifdef __linux__   
        hccast_mira_service_stop();
        cast_p2p_switch_thread_stop();
#else
        hccast_mira_service_stop();
        hccast_wifi_wpa_stop();
#endif
    }
    
    return 0;
}
#endif

#define WIFI_CHECK_TIME 50000000
static void *network_connect_task(void *arg)
{
    hccast_wifi_ap_info_t wifi_ap;
    hccast_wifi_ap_info_t conn_wifi_ap;
    uint32_t loop_cnt = WIFI_CHECK_TIME/100;
    //m_wifi_connecting = true;
    char connect_fail = 0;
    control_msg_t ctl_msg = {0};

    while(!network_wifi_module_get())
    {
        api_sleep_ms(100);
        if (0 == loop_cnt--){
            connect_fail = 1;
            goto connect_exit;
        }

        if (api_get_wifi_pm_state() || !network_get_current_state()){
            connect_fail = 1;
            goto connect_exit;
        }     
    }

#ifdef AIRP2P_SUPPORT
        if (network_get_airp2p_state())
        {
            network_airp2p_start();
            connect_fail = 1;
            goto connect_exit;
            //return NULL;
        }    
#endif

    network_init();

    //step 1: http server startup

    sysdata_init_device_name();
    hostap_config_init();
    udhcpc_set_hostname((char*)projector_get_some_sys_param(P_DEVICE_NAME));

    if (!projector_get_some_sys_param(P_WIFI_ONOFF))
    {
        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_INIT_DONE;
        api_control_send_msg(&ctl_msg); 
        goto connect_exit;
    }	

    if (api_get_wifi_pm_state() || network_get_airp2p_state())
    { 
        goto connect_exit;
    }

#if 1
    //step 2:
    if (sysdata_wifi_ap_get(&wifi_ap))
    {
        app_wifi_switch_work_mode(WIFI_MODE_STATION);
        memcpy(g_connecting_ssid, wifi_ap.ssid, sizeof(g_connecting_ssid));

        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTING;
        api_control_send_msg(&ctl_msg);

        hccast_wifi_mgr_udhcpc_stop();

        //Get wifi AP from flash, connect wifi
        printf("%s(), line:%d, connect to %s, pwd:%s\n", __FUNCTION__, __LINE__, \
               wifi_ap.ssid, wifi_ap.pwd);

        memcpy(&conn_wifi_ap, &wifi_ap, sizeof(hccast_wifi_ap_info_t));
        if (HCCAST_NET_WIFI_ECR6600U == hccast_wifi_mgr_get_wifi_model() \
            && conn_wifi_ap.encryptMode != HCCAST_WIFI_ENCRYPT_MODE_OPEN_WEP \
            && conn_wifi_ap.encryptMode != HCCAST_WIFI_ENCRYPT_MODE_SHARED_WEP \
            )
            conn_wifi_ap.encryptMode = HCCAST_WIFI_ENCRYPT_MODE_NOT_HANDLE;
        hccast_wifi_mgr_connect(&conn_wifi_ap);
        if (hccast_wifi_mgr_get_connect_status())
        {
            hccast_wifi_mgr_udhcpc_start();
        }
        else
        {
            printf("%s(), line:%d. connect timeout!\n", __func__, __LINE__);
            m_wifi_config.bConnected = false;  
            m_wifi_config.bConnectedByPhone = false;
            m_wifi_config.host_ap_ip_ready = false;
            m_wifi_config.sta_ip_ready = false;
            memset(m_wifi_config.local_ip, 0, MAX_IP_STR_LEN);
            memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));            

            if (api_get_wifi_pm_state() || network_get_airp2p_state())
            {
                goto connect_exit;
            }

            usleep(50*1000);
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL;
            api_control_send_msg(&ctl_msg);

            hccast_wifi_mgr_disconnect_no_message();

            pthread_mutex_lock(&m_service_en_mutex);
            if(network_service_enable_get())
            {
                app_wifi_switch_work_mode(WIFI_MODE_AP);
            #ifdef MIRACAST_SUPPORT
                hccast_mira_service_stop();
                hccast_mira_service_start();
            #endif
            }
            pthread_mutex_unlock(&m_service_en_mutex);
        }
    }
    else
    {
        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_STATUS_UPDATE;
        api_control_send_msg(&ctl_msg);
        
        pthread_mutex_lock(&m_service_en_mutex);
        if(network_service_enable_get())
        {
            //No wif AP in flash, entering AP mode
            app_wifi_switch_work_mode(WIFI_MODE_AP);
        #ifdef MIRACAST_SUPPORT
            hccast_mira_service_stop();
            hccast_mira_service_start();
        #endif

            printf("%s(), line:%d. AP mode start\n", __FUNCTION__, __LINE__);
        }
        pthread_mutex_unlock(&m_service_en_mutex);
    }
#endif

connect_exit:

    pthread_mutex_lock(&m_wifi_state_mutex);
    m_wifi_config.wifi_connecting = false;
    pthread_mutex_unlock(&m_wifi_state_mutex);

    if (connect_fail)    
        m_wifi_config.wifi_init_done = false;
    else
        m_wifi_config.wifi_init_done = true;

    return NULL;
}
#endif

#ifdef __HCRTOS__
static char net_read = 0;
static void udhcpc_cb(unsigned int data)
{
    hccast_udhcp_result_t *in = (hccast_udhcp_result_t*)data;

    if (in)
    {
        printf("udhcpc got ip: %s\n", in->ip);
        net_read = 1;
    }
    else
    {
        net_read = 0;
    }
}
#endif
//Connet wifi ap if there is valid wifi AP information in flash. Otherwise
//set device to AP mdoe
int network_connect(void)
{
#ifdef WIFI_SUPPORT

    pthread_mutex_lock(&m_wifi_state_mutex);
    if (m_wifi_config.wifi_connecting || api_get_wifi_pm_state())
    {
        pthread_mutex_unlock(&m_wifi_state_mutex);
        return API_SUCCESS;
    }
    
    m_wifi_config.wifi_connecting = true;
    pthread_t thread_id = 0;
    pthread_attr_t attr;
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, network_connect_task, NULL))
    {
        m_wifi_config.wifi_connecting = false;
        pthread_mutex_unlock(&m_wifi_state_mutex);
        pthread_attr_destroy(&attr);
        return API_FAILURE;
    }
    pthread_attr_destroy(&attr);

    pthread_mutex_unlock(&m_wifi_state_mutex);
#else // 

	static udhcp_conf_t eth_udhcpc_conf =
	{
	    .func = udhcpc_cb,
	    .ifname = UDHCP_IF_NONE,
	    .pid    = 0,
	    .run    = 0
	};

#ifdef NETWORK_API
    udhcpc_start(&eth_udhcpc_conf);

    while(!net_read)
    {
        sleep(1);
    }

    hccast_start_services();
#endif
#endif // WIFI_SUPPORT

    return API_SUCCESS;
}



static volatile bool m_service_enable = false;
/*
check if the network service can be 
start freedom. For some scene, dlna,miracast,aircast,
hostap,etc only start in cast window.
 */
bool network_service_enable_get()
{
    return m_service_enable;
}

void network_service_enable_set(bool start)
{
    pthread_mutex_lock(&m_service_en_mutex);
    m_service_enable = start;
    pthread_mutex_unlock(&m_service_en_mutex);
}

wifi_config_t *app_wifi_config_get(void)
{
    return &m_wifi_config;
}


#if defined(AUTO_HTTP_UPGRADE) || defined(MANUAL_HTTP_UPGRADE)

static bool network_upgrade_flag = false;

bool network_upgrade_flag_get(void)
{
    return network_upgrade_flag;
}

void network_upgrade_flag_set(bool en)
{
    network_upgrade_flag = en;
}

static void dump_buf(char *tag, uint8_t *buf, int len)
{
     int i;
     
     printf("******%s*******\n", tag);
     for(i = 0; i < len; i++)
    {
         if(i %16==0)
             printf("\n");
           printf("%02x ", buf[i]);
    }
    printf("\n\n");
}

static char m_upgrade_url[256] = {0};
#define UPGRADE_CONFIG_FILE_LEN 4096

static char *network_check_upgrade_url(void)
{
    //Read network upgrade config.
    char *buffer = NULL;
    int config_len = 0;
    char request_url[256];
    char product_id[32];
    uint32_t local_version;
    int error_code = UPG_STATUS_CONFIG_ERROR;
    control_msg_t msg = { 0 };

    strcpy((char*)product_id, (char*)projector_get_some_sys_param(P_DEV_PRODUCT_ID));
    snprintf(request_url, sizeof(request_url), NETWORK_UPGRADE_URL, product_id);

    buffer = (char*)malloc(UPGRADE_CONFIG_FILE_LEN);
    if(buffer == NULL)
    {   
        printf("%s buffer malloc error\n",__func__);
        msg.msg_type = MSG_TYPE_UPG_STATUS;
        api_control_send_msg(&msg);
        return NULL;
    }
    memset(buffer, 0, UPGRADE_CONFIG_FILE_LEN);

    printf("*request_url:%s\n", request_url);
    config_len = network_upg_download_config(request_url, buffer, UPGRADE_CONFIG_FILE_LEN);
    if (config_len <= 0){
        printf("%s(), line:%d. download upgrade config fail!\n", __func__, __LINE__);
        if(buffer)
        {
            free(buffer);
        }
        msg.msg_type = MSG_TYPE_UPG_STATUS;
        msg.msg_code = UPG_STATUS_NETWORK_ERROR;
        api_control_send_msg(&msg);
        return NULL;
    }
    //printf("upgrade config:\n%s\n", buffer);


    //Get the product_id, version of upgrade config
    cJSON* cjson_obj = NULL;
    cJSON* cjson_force_upgrade = NULL;
    cJSON* cjson_product = NULL;
    cJSON* cjson_version = NULL;
    cJSON* cjson_url = NULL;
    char* cjson_data = NULL;
    int skip_len = 0;
    int cjson_data_len = 0;
    int i = 0;

    cjson_data = strstr(buffer, "jsonp_callback(");
    if(cjson_data == NULL)
    {
        printf("%s can not parse cjson_data\n",__func__);
        goto fail_exit;
    }

    
    skip_len = strlen("jsonp_callback(");
    cjson_data += skip_len;
    
    cjson_data_len = strlen(cjson_data);
    for(i = cjson_data_len; i >= 0; i--)
    {
        if(cjson_data[i] == ')')
        {
            cjson_data[i] = '\0';
            break;
        }
    }
 
    //printf("cjson_data:\n%s\n",cjson_data);
    cjson_obj = cJSON_Parse(cjson_data);
    if (!cjson_obj){
        printf("%s(), line:%d. parse config json error\n", __func__, __LINE__);
        goto fail_exit;
    }

    cjson_url = cJSON_GetObjectItem(cjson_obj, "url");
    cjson_version = cJSON_GetObjectItem(cjson_obj, "version");
    cjson_product = cJSON_GetObjectItem(cjson_obj, "product");
    cjson_force_upgrade = cJSON_GetObjectItem(cjson_obj, "force_upgrade");
    if (!cjson_url || !cjson_version || !cjson_product || !cjson_force_upgrade)
    {
        printf("No upgrade file!\n");
        goto fail_exit;
    }        
    printf("web file:%s\n", cjson_url->valuestring);

    local_version = (uint32_t)projector_get_some_sys_param(P_DEV_VERSION);
    long long version_val = atoll(cjson_version->valuestring);
    uint32_t web_version = (uint32_t)version_val;
    int force_upgrade = (cjson_force_upgrade->type == cJSON_True) ? 1 : 0;

    printf("web version: %u, local version:%u, web_product:%s, local_product:%s, force_upgrade:%d\n", \
        (unsigned int)web_version, (unsigned int)local_version, cjson_product->valuestring, product_id, force_upgrade);

    if(force_upgrade)
    {
        if(strcmp(cjson_product->valuestring, product_id) == 0)
        {   
            if (web_version <= local_version){
                printf("only old version, do not upgrade!\n");
                error_code = UPG_STATUS_VERSION_IS_OLD;
                goto fail_exit;
            }
        }
        else
        {
            printf("product name not match!\n");
            error_code = UPG_STATUS_PRODUCT_ID_MISMATCH;
            goto fail_exit;
        }
    }
    else
    {
        printf("not support force upgrade!\n");
        goto fail_exit;
    }

    snprintf(m_upgrade_url, sizeof(m_upgrade_url), "%s", cjson_url->valuestring);

    if (cjson_obj)
        cJSON_Delete(cjson_obj);

    if(buffer)
        free(buffer);

    return m_upgrade_url;

fail_exit:
    if (cjson_obj)
        cJSON_Delete(cjson_obj);
    
    if(buffer)
        free(buffer);

    msg.msg_type = MSG_TYPE_UPG_STATUS;
    msg.msg_code = error_code;
    api_control_send_msg(&msg);

    return NULL;
}

char *network_get_upgrade_url(void)
{
    return m_upgrade_url;
}


static void *network_upgrade_task(void *arg)
{
    //step 1
    //Check if wifi network(station mode) is ready.
    //
    control_msg_t msg = { 0 };
    int loop_cnt = WIFI_CHECK_TIME/100;
    while (!(network_wifi_module_get() && hccast_wifi_mgr_get_connect_status())){
        api_sleep_ms(100);
        //if network is not ready, return
        if (0 == loop_cnt--){
            printf("%s(), line:%d. network not ready, dont update!", __func__, __LINE__);
            return NULL;
        }
    }

    //step 2
    //Read network upgrade config.
    char* upgrade_url = NULL;
    upgrade_url = network_check_upgrade_url();
    if (NULL == upgrade_url)
    { 
        network_upgrade_flag_set(false);
        return NULL;
    }

    //step 3 
    //Download corresponding upgraded file.
    printf("* updatde request_url:%s\n", upgrade_url);
    if (network_upg_start(upgrade_url))
    {
        msg.msg_type = MSG_TYPE_UPG_STATUS;
        msg.msg_code = UPG_STATUS_CONFIG_ERROR;
        api_control_send_msg(&msg);
        network_upgrade_flag_set(false);
    }

    return NULL;
}

int network_upgrade_start()
{
    network_upgrade_flag_set(true);
    network_upg_set_user_abort(false);

    pthread_t thread_id = 0;
    pthread_attr_t attr;
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, network_upgrade_task, NULL)) {
        pthread_attr_destroy(&attr);
        network_upgrade_flag_set(false);
        return -1;
    }
    pthread_attr_destroy(&attr);

    return 0;
        
}
#endif

static void wifi_do_connect(hccast_wifi_ap_info_t *wifi_info, int timeout)
{
    char wifi_ap_exist = 0;
    hccast_wifi_ap_info_t wifi_ap;
    hccast_wifi_ap_info_t conn_wifi_ap;
    control_msg_t ctl_msg = {0};

    if (NULL == wifi_info){
        if (sysdata_wifi_ap_get(&wifi_ap))
            wifi_ap_exist = 1;
    }else{
        memcpy(&wifi_ap, wifi_info, sizeof(hccast_wifi_ap_info_t));
        wifi_ap_exist = 1;
    }

    if (wifi_ap_exist)
    {
        app_wifi_switch_work_mode(WIFI_MODE_STATION);

        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTING;
        api_control_send_msg(&ctl_msg);

        hccast_wifi_mgr_udhcpc_stop();
#ifdef MIRACAST_SUPPORT
        if (network_service_enable_get() && m_wifi_config.bReconnect)
        {
            hccast_mira_service_stop();
            hccast_scene_set_mira_restart_flag(1);
        }
#endif

        //Get wifi AP from flash, connect wifi
        printf("%s(), line:%d, connect to %s, pwd:%s\n", __FUNCTION__, __LINE__, \
               wifi_ap.ssid, wifi_ap.pwd);

        memcpy(g_connecting_ssid, wifi_ap.ssid, sizeof(g_connecting_ssid));
        hccast_wifi_mgr_disconnect_no_message();
        usleep(50*1000);

        memcpy(&conn_wifi_ap, &wifi_ap, sizeof(hccast_wifi_ap_info_t));
        if (HCCAST_NET_WIFI_ECR6600U == hccast_wifi_mgr_get_wifi_model() \
            && conn_wifi_ap.encryptMode != HCCAST_WIFI_ENCRYPT_MODE_OPEN_WEP \
            && conn_wifi_ap.encryptMode != HCCAST_WIFI_ENCRYPT_MODE_SHARED_WEP \
            )
            conn_wifi_ap.encryptMode = HCCAST_WIFI_ENCRYPT_MODE_NOT_HANDLE;
        int ret = hccast_wifi_mgr_connect_timeout(&conn_wifi_ap, timeout);
        wifi_is_reconning_set(false);
        if (HCCAST_WIFI_ERR_USER_ABORT == ret)
        {
            printf("%s(), line:%d. user abort.\n", __FUNCTION__, __LINE__);
            m_wifi_config.bConnected = false;  
            m_wifi_config.bConnectedByPhone = false;
            m_wifi_config.host_ap_ip_ready = false;
            m_wifi_config.sta_ip_ready = false;
            m_wifi_config.bReconnect = false;
            hccast_service_need_start = false;
            memset(m_wifi_config.local_ip, 0, MAX_IP_STR_LEN);
            memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
            
            hccast_wifi_mgr_disconnect_no_message();
            return ;
        }

        if (hccast_wifi_mgr_get_connect_status())
        {
            hccast_wifi_mgr_udhcpc_start();

            sysdata_wifi_ap_resave(&wifi_ap);
            memcpy(g_connecting_ssid, wifi_ap.ssid, sizeof(g_connecting_ssid));
        }
        else
        {
            printf("%s(), line:%d. connect timeout!\n", __func__, __LINE__);
            m_wifi_config.bConnected = false;  
            m_wifi_config.bConnectedByPhone = false;
            m_wifi_config.host_ap_ip_ready = false;
            m_wifi_config.sta_ip_ready = false;
            hccast_service_need_start = false;
            memset(m_wifi_config.local_ip, 0, MAX_IP_STR_LEN);
            memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));

            if (api_get_wifi_pm_state() || network_get_airp2p_state())
            {
                return ;
            }

            usleep(50*1000);

            if (m_wifi_config.bReconnect)
            {
                hccast_media_stop();
                hccast_ap_dlna_aircast_stop();

                ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_DISCONNECTED;
                m_wifi_config.bReconnect = false;
            }
            else
            {
                ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL;
            }

            api_control_send_msg(&ctl_msg);
            hccast_wifi_mgr_disconnect_no_message();

            pthread_mutex_lock(&m_service_en_mutex);
            if(network_service_enable_get())
            {
                app_wifi_switch_work_mode(WIFI_MODE_AP);
            #ifdef MIRACAST_SUPPORT
                hccast_mira_service_stop();
                hccast_mira_service_start();
            #endif
            }
            pthread_mutex_unlock(&m_service_en_mutex);
        }
    }
    else
    {
        //No wif AP in flash, entering AP mode
        pthread_mutex_lock(&m_service_en_mutex);
        if(network_service_enable_get())
        {
            app_wifi_switch_work_mode(WIFI_MODE_AP);
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTED;
            api_control_send_msg(&ctl_msg);
        #ifdef MIRACAST_SUPPORT
            hccast_mira_service_start();
        #endif
        }
        m_wifi_config.bReconnect = false;
        pthread_mutex_unlock(&m_service_en_mutex);
        printf("%s(), line:%d. AP mode start\n", __FUNCTION__, __LINE__);
    }
}

static void *wifi_connect_task(void *arg)
{
    network_wifi_ap_info_t *ap_info = (network_wifi_ap_info_t*)arg;
    //m_wifi_connecting = true;

    if (network_wifi_module_get()){
        wifi_do_connect(ap_info->wifi_ap, ap_info->timeout);
    }

    free(ap_info);
    pthread_mutex_lock(&m_wifi_state_mutex);
    m_wifi_config.wifi_connecting = false;
    pthread_mutex_unlock(&m_wifi_state_mutex);
    return NULL;
}

int app_wifi_reconnect(hccast_wifi_ap_info_t *wifi_ap, int timeout)
{
    pthread_mutex_lock(&m_wifi_state_mutex);
    if (m_wifi_config.wifi_connecting || api_get_wifi_pm_state() || network_get_airp2p_state())
    {
        pthread_mutex_unlock(&m_wifi_state_mutex);
        return 1;
    }

    m_wifi_config.wifi_connecting = true;

    network_wifi_ap_info_t *ap_info = (network_wifi_ap_info_t *)malloc(sizeof(network_wifi_ap_info_t));
    if (NULL == ap_info)
    {
        printf("%s malloc ap_info fail\n", __func__);
        pthread_mutex_unlock(&m_wifi_state_mutex);
        return 1;
    }
    ap_info->wifi_ap = wifi_ap;
    ap_info->timeout = timeout;

    pthread_t thread_id = 0;
    pthread_attr_t attr;
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&thread_id, &attr, wifi_connect_task, (void*)ap_info) != 0)
    {
        m_wifi_config.wifi_connecting = false;
        free(ap_info);
        pthread_mutex_unlock(&m_wifi_state_mutex);
        pthread_attr_destroy(&attr);
        return -1;
    }

    pthread_attr_destroy(&attr);
    pthread_mutex_unlock(&m_wifi_state_mutex);
    return 0;
}

WIFI_MODE_e app_wifi_get_work_mode()
{
    WIFI_MODE_e mode = WIFI_MODE_NONE;
    pthread_mutex_lock(&m_wifi_switch_mode_mutex);
    mode = m_wifi_config.mode;
    pthread_mutex_unlock(&m_wifi_switch_mode_mutex);

    return mode;
}

int app_wifi_switch_work_mode(WIFI_MODE_e wifi_mode)
{
    pthread_mutex_lock(&m_wifi_switch_mode_mutex);

    if (WIFI_MODE_STATION == wifi_mode){
        if (hccast_wifi_mgr_get_hostap_status()){
            hccast_wifi_mgr_hostap_stop();
        }
    #ifdef __HCRTOS__
        if (!hccast_wifi_mgr_get_station_status()){
            hccast_wifi_mgr_enter_sta_mode();
        }
    #endif

        // netif
        if (HCCAST_NET_WIFI_8800D == network_wifi_module_get())
        {
            char ap_ifname[32] = {0};
            char sta_ifname[32] = {0};
            hccast_wifi_mgr_get_ap_ifname(ap_ifname, sizeof(ap_ifname));
            hccast_wifi_mgr_get_sta_ifname(sta_ifname, sizeof(sta_ifname));

            hccast_net_set_if_updown(ap_ifname, HCCAST_NET_IF_DOWN);
            hccast_net_set_if_updown(sta_ifname, HCCAST_NET_IF_UP);
        }
    } else if (WIFI_MODE_AP == wifi_mode){
    #ifdef __HCRTOS__
        if (hccast_wifi_mgr_get_station_status()){
            hccast_wifi_mgr_exit_sta_mode();
        }
    #endif

        // netif
        if (HCCAST_NET_WIFI_8800D == network_wifi_module_get())
        {
            char ap_ifname[32] = {0};
            char sta_ifname[32] = {0};
            hccast_wifi_mgr_get_ap_ifname(ap_ifname, sizeof(ap_ifname));
            hccast_wifi_mgr_get_sta_ifname(sta_ifname, sizeof(sta_ifname));

            hccast_net_set_if_updown(sta_ifname, HCCAST_NET_IF_DOWN);
            hccast_net_set_if_updown(ap_ifname, HCCAST_NET_IF_UP);
        }

        if (!hccast_wifi_mgr_get_hostap_status()) {
    #ifdef __HCRTOS__
            hostap_config_init();
    #endif
            hccast_wifi_mgr_hostap_start();
        }
        else if (2 == hccast_wifi_mgr_get_hostap_status()) // current status is disable
        {
        #ifdef __HCRTOS__
            hccast_wifi_mgr_hostap_enable();
            hccast_wifi_mgr_udhcpd_stop();
            hccast_wifi_mgr_udhcpd_start();
        #endif
        }
    } else if (WIFI_MODE_NONE == wifi_mode){
        if (hccast_wifi_mgr_get_hostap_status()){      
            hccast_wifi_mgr_hostap_stop();
        }
    #ifdef __HCRTOS__
        if (hccast_wifi_mgr_get_station_status()){
            hccast_wifi_mgr_exit_sta_mode();
        }
    #endif
    }
    m_wifi_config.mode = wifi_mode;

    pthread_mutex_unlock(&m_wifi_switch_mode_mutex);
    return 0;
}


#ifdef LIBCURL_SUPPORT

#include <curl/curl.h>

typedef struct{
    char url[1024];
    char file_name[128];
    uint8_t *data;
    uint32_t size;
    uint32_t data_pos;

    void *user_data;
    net_dowload_cb cb_func;
    FILE *fp;
    uint8_t download_stop;
    pthread_t thread_id;
}NET_DOWNLOAD_PARAM_s;

static uint32_t _curl_write_cb(void *data, uint32_t size, uint32_t nmemb, void *param)
{
    FILE* fp = NULL;  
    NET_DOWNLOAD_PARAM_s *down_param = (NET_DOWNLOAD_PARAM_s*)param;
    uint32_t realsize = size * nmemb;

    if (down_param->download_stop)
        return 0;

    //printf("%s(), line: %d. size:%d, nmemb:%d\n", __func__, __LINE__, size, nmemb);    
    //printf(".");
    fp = down_param->fp;
    if (fp)
    {
        fwrite(data, size, nmemb, fp);
    }
    else
    {        
        if (down_param->data && (down_param->data_pos + realsize <= down_param->size))
        {
            memcpy((down_param->data+down_param->data_pos), data, realsize);
        }
        else
        {
            printf("%s mem overlap\n", __func__);
            return 0; //stop download.
        }
    }

    //return the real size, else if network would not continue download ...
    down_param->data_pos += realsize;
    return realsize;  
}

static int _curl_progress_cb(void *param,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
    NET_DOWNLOAD_PARAM_s *down_param = (NET_DOWNLOAD_PARAM_s*)param;

    //return 1, libcurl stop download
    if (down_param->download_stop)
        return 1;
    api_sleep_ms(1);    //yiet cpu 
    return 0;
}

static void *_network_data_down_task(void *param)
{
    FILE *fp = NULL;
    int fd=0;
    NET_DOWNLOAD_PARAM_s *down_param;
    CURL *curl_handle = NULL;
    CURLcode retcCode;
    int curl_reload_time = 0;

    down_param = (NET_DOWNLOAD_PARAM_s*)param;
    
    curl_handle = curl_easy_init();
    if (!curl_handle)
    {
        printf("%s(), line: %d. , curl_easy_init fail\n", __func__, __LINE__);
        return NULL;
    }

    do {
        if (strlen(down_param->file_name))
        {
            fp = fopen(down_param->file_name, "wb+");
            if (!fp)
            {
                printf("%s(), line: %d. , open %s fail\n", __func__, __LINE__, down_param->file_name);    
                break;
            }
            down_param->fp = fp;
        }
        curl_easy_reset(curl_handle);

        curl_easy_setopt(curl_handle, CURLOPT_URL, down_param->url);

        /* send all data to this function  */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, _curl_write_cb);
        /* we pass our 'down_param' struct to the callback function */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)down_param);

        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, _curl_progress_cb);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, (void *)down_param);
        
        curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "gzip");

        curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 5);  
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);  

        //do not vefificate host, else if curl_easy_perform return error: CURLE_PEER_FAILED_VERIFICATION
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);  
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);  

        //enable log
        //curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 60L);  
        curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 20L);  

        /* send a request */
        retcCode = curl_easy_perform(curl_handle);

        if (fp){
            fflush(fp);
            fsync(fileno(fp));
            fclose(fp);
        }

        if (retcCode)
        {
            const char* pError = curl_easy_strerror(retcCode);        
            printf("%s(), pError:%s(%d)!\n", __func__, pError, retcCode);  
        }
        else
        {
            if (down_param->cb_func)
                down_param->cb_func(down_param->user_data, down_param->data_pos);
            //printf("%s(), download OK, file length:%ld!\n", __func__, down_param->data_pos);  
        }

        curl_reload_time++;

    } while (retcCode && (retcCode != CURLE_ABORTED_BY_CALLBACK) && (curl_reload_time < 3));

    if (curl_handle)
        curl_easy_cleanup(curl_handle);

    return NULL;
}

//Download URL to file or memory in block mode or none-block mode.
//url: the url to download
//file_name: Download URL data and save to file if it is not NULL. Choose one of file_name and data buffer
//data:  Download URL data the the buffer if it is not NULL. Choose one of file_name and data buffer
//size:  Use with parameter data, the size of data buffer 
//net_cb: Call the callback function while finish download if it is non-block mode.
//user_data: The user data will pass back to user in callback function
//block: blocked mode or none-block mode. The api return when download finished in block mode.
// return: network download handle.
// The api return immediately in none-block mode, download finished while net_cb is called.
void *api_network_download_start(char *url, char *file_name, uint8_t *data, uint32_t size, \
        net_dowload_cb net_cb, void *user_data, bool block)
{
    pthread_attr_t attr;
    pthread_t thread_id = 0;
    NET_DOWNLOAD_PARAM_s *down_param;
    down_param = malloc(sizeof(NET_DOWNLOAD_PARAM_s));
    if (!down_param)
        return NULL;

    printf("%s(), url:%s\n", __func__, url);
    memset(down_param, 0, sizeof(NET_DOWNLOAD_PARAM_s));
    strncpy(down_param->url, url, sizeof(down_param->url));
    if (file_name)
        strncpy(down_param->file_name, file_name, sizeof(down_param->file_name));
    down_param->user_data = user_data;
    down_param->cb_func = net_cb;
    down_param->data = data;
    down_param->size = size;

    if (block)
    {    
        _network_data_down_task((void *)down_param);
    }
    else
    {
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x4000);
        //pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
        if (pthread_create(&thread_id, &attr, _network_data_down_task, (void *)down_param)) {
            printf("pthread_create receive_cvbs_in_event_func fail\n");
            pthread_attr_destroy(&attr);
            return NULL;
        }
        down_param->thread_id = thread_id;
        pthread_attr_destroy(&attr);
    }

    return (void*)down_param;
}

void api_network_download_flag_stop(void *handle)
{
    if (handle == NULL)
        return;
    NET_DOWNLOAD_PARAM_s *down_param;
    down_param = (NET_DOWNLOAD_PARAM_s*)handle;
    down_param->download_stop = 1;
}

void api_network_download_pthread_stop(void *handle)
{
    if (handle == NULL)
        return;
    NET_DOWNLOAD_PARAM_s *down_param;
    down_param = (NET_DOWNLOAD_PARAM_s*)handle;
    if (down_param->thread_id)
        pthread_join(down_param->thread_id, NULL);
    free(down_param);
}

#endif //end of #ifdef LIBCURL_SUPPORT

static void *network_plugout_reboot_task(void *arg)
{
    printf("%s\n", __func__);

    usleep(5 * 1000 * 1000);
    api_system_reboot();

    return NULL;
}

int network_plugout_reboot(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, network_plugout_reboot_task, NULL);
    return 0;
}

int network_wifi_pm_plugin_handle(void)
{
    if (api_get_wifi_pm_state() == WIFI_PM_STATE_PS)
    {
        hccast_wifi_mgr_power_off(HCCAST_WIFI_PM_MODE_PS);
    }   
     
    return 0;
}

int network_wifi_pm_open(void)
{
    int pm_mode = api_get_wifi_pm_mode();

    if (pm_mode && !api_get_wifi_pm_state())
    {
        if (pm_mode == HCCAST_WIFI_PM_MODE_HW)
        {
            /*step1: Set mpwr state flag.*/
            api_set_wifi_pm_state(WIFI_PM_STATE_GPIO);
            
            /*step2: Wait until wifi-connect or wifi-scan stop. */
            while(m_wifi_config.wifi_connecting || m_wifi_config.wifi_scanning 
#ifdef MIRACAST_SUPPORT
                || hccast_mira_get_restart_state()
#endif
            )
            {
                hccast_wifi_mgr_op_abort();
                usleep(10*1000);
            }
            
            m_wifi_config.bConnected = false;
            m_wifi_config.bReconnect = false;
            hccast_wifi_mgr_udhcpc_stop();
            hccast_stop_services();
            
            /*step3: Stop wpa.*/
            app_wifi_switch_work_mode(WIFI_MODE_NONE);
            
            /*step4: Trigger wifi power off.*/
            hccast_wifi_mgr_power_off(HCCAST_WIFI_PM_MODE_HW);

            /*step5: Wait wifi plug out */
            while(m_probed_wifi_module)
            {
                usleep(10*1000);
            }
        }
        else if (pm_mode == HCCAST_WIFI_PM_MODE_PS)
        {
            api_set_wifi_pm_state(WIFI_PM_STATE_PS);

            if (m_probed_wifi_module)
            {
                while(m_wifi_config.wifi_connecting || m_wifi_config.wifi_scanning
#ifdef MIRACAST_SUPPORT
                    || hccast_mira_get_restart_state()
#endif 
                )
                {
                    hccast_wifi_mgr_op_abort();
                    usleep(10*1000);
                }
                
                m_wifi_config.bConnected = false;
                m_wifi_config.bReconnect = false;
                hccast_wifi_mgr_udhcpc_stop();
                app_wifi_switch_work_mode(WIFI_MODE_NONE);
                
                hccast_wifi_mgr_power_off(HCCAST_WIFI_PM_MODE_PS);
           }     
        }

        printf("wifi mpwr open success.\n");
    }    
    
    return 0;
}

int network_wifi_pm_stop(void)
{
    int pm_mode = api_get_wifi_pm_mode();
    int cur_pm_state = api_get_wifi_pm_state();
    hccast_wifi_ap_info_t wifi_ap;
    //step1: if current is low power status need resume.
    if (cur_pm_state)
    {
        api_set_wifi_pm_state(WIFI_PM_STATE_NONE);

        if (pm_mode == HCCAST_WIFI_PM_MODE_HW)
        {
            //trigger wifi power on.
            hccast_wifi_mgr_power_on(HCCAST_WIFI_PM_MODE_HW);
        }    
        else if (pm_mode == HCCAST_WIFI_PM_MODE_PS)
        {
            hccast_wifi_mgr_power_on(HCCAST_WIFI_PM_MODE_PS);
            if (!app_get_wifi_init_done())//Not inited.
            {
                network_connect();
            }
            else
            {
                if (sysdata_wifi_ap_get(&wifi_ap))
                {
                    app_wifi_reconnect(NULL, NETWORK_WIFI_CONNECTING_TIMEOUT);
                }    
            }
        }
    }

    return 0;
}


#endif //end of #ifdef WIFI_SUPPORT
