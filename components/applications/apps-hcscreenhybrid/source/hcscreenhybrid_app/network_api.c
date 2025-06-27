/*
 * NETWORK_SUPPORT.c: use for network, include wifi, etc
**/

#include <math.h>
#include <stdio.h> //printf()
#include <stdlib.h>
#include <string.h> //memcpy()
#include <unistd.h> //usleep()
#include <pthread.h>
#include <arpa/inet.h>

#include <hccast/hccast_dhcpd.h>
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_httpd.h>

#include <hccast/hccast_media.h>
#include <hccast/hccast_net.h>

#include <hccast/hccast_net.h>
#include "network_api.h"
#include <hcuapi/dis.h>
#include "cast_api.h"
#include "app_log.h"
#include "network_upg.h"
#include "network_ping.h"
#include "data_mgr.h"

#define UUID_HEADER "HCcast"
//#define AUTO_HTTP_UPGRADE
/**********************************************************************
NETWORK_UPGRADE_URL:
config name: HCFOTA.jsonp
example:http://172.16.12.81:80/hccast/rtos/HC15A210/hcscreen/HCFOTA.jsonp
**********************************************************************/
#ifdef __linux__
#define NETWORK_UPGRADE_URL "http://172.16.12.81:80/hccast/linux/%s/hcscreen/HCFOTA.jsonp"
#else
#define NETWORK_UPGRADE_URL "http://172.16.12.81:80/hccast/rtos/%s/hcscreen/HCFOTA.jsonp"
#endif

#include "com_api.h"
#include "data_mgr.h"
#include "tv_sys.h"


static wifi_config_t m_wifi_config = {0};

static char g_connecting_ssid[WIFI_MAX_SSID_LEN] = {0};
static pthread_mutex_t m_wifi_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m_service_en_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m_wifi_switch_mode_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_net_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_net_enable = 1;
static int g_wl_services_enable = 1;

#ifdef NETWORK_SUPPORT
    #ifdef WIFI_SUPPORT
        static int m_probed_wifi_module = 0;
        static int hostap_connect_count = 0;
        static int hostap_discover_ok = 0;
        static int g_wifi_connect_status = 0;

        static int wifi_mgr_callback_func(hccast_wifi_event_e event, void *in, void *out);
        #ifdef __linux__
            static void network_probed_wifi(void);
        #endif // __linux__
    #endif // WIFI_SUPPORT
#endif // NETWORK_SUPPORT

static int factary_init = 0;

static int httpd_callback_func(hccast_httpd_event_e event, void *in, void *out);
static void media_callback_func(hccast_media_event_e msg_type, void *param);
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
static void hostap_config_init(void);
#endif
#endif

static pthread_mutex_t g_wifi_status_mutex = PTHREAD_MUTEX_INITIALIZER;

static int network_get_net_enable(void)
{
    int status = 0;
    pthread_mutex_lock(&g_net_state_mutex);
    status = g_net_enable;
    pthread_mutex_unlock(&g_net_state_mutex);
    return status;
}

void network_set_net_enable(int enable)
{
    pthread_mutex_lock(&g_net_state_mutex);
    g_net_enable = enable;
    pthread_mutex_unlock(&g_net_state_mutex);
}

char *app_get_connecting_ssid()
{
    return g_connecting_ssid;
}

int app_get_wifi_connect_status()
{
    int status = 0;
    pthread_mutex_lock(&g_wifi_status_mutex);
    status = g_wifi_connect_status;
    pthread_mutex_unlock(&g_wifi_status_mutex);
    return status;
}

void app_set_wifi_connect_status(int status)
{
    pthread_mutex_lock(&g_wifi_status_mutex);
    g_wifi_connect_status = !!status;
    pthread_mutex_unlock(&g_wifi_status_mutex);
}

bool app_wifi_is_limited_internet(void)
{
    return m_wifi_config.bLimitedInternet;
}

void app_wifi_set_limited_internet(bool limited)
{
    m_wifi_config.bLimitedInternet = limited;
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

#ifdef __linux__
#else // HC_RTOS
#include <linux/interrupt.h>

static int app_softirqd_set_priority(int priority)
{
#ifdef SOC_HC15XX

#if CONFIG_SCHED_SOFTIRQD_PRIORITY
    softirqd_set_priority(priority, 0);
#endif

#if CONFIG_SCHED_SOFTIRQD_RESCHED_PRIORITY
    softirqd_set_resched_priority(priority, 0);
#endif

#endif
}

static int app_softirqd_reset_priority()
{
#ifdef SOC_HC15XX

#if CONFIG_SCHED_SOFTIRQD_PRIORITY
    softirqd_set_priority(CONFIG_SCHED_SOFTIRQD_PRIORITY, 0);
#endif

#if CONFIG_SCHED_SOFTIRQD_RESCHED_PRIORITY
    softirqd_set_resched_priority(CONFIG_SCHED_SOFTIRQD_RESCHED_PRIORITY, 0);
#endif

#endif
}
#endif

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT

#ifdef AUTO_HTTP_UPGRADE
#include <cjson/cJSON.h>

static int g_http_upgrade_check_first = 0;
static char m_upgrade_url[256] = {0};
#define UPGRADE_CONFIG_FILE_LEN 4096

char *network_check_upgrade_url(void)
{
    //Read network upgrade config.
    char *buffer = NULL;
    int config_len = 0;
    char request_url[256];
    char product_id[32];
    uint32_t local_version;
    sys_data_t* sys_data = data_mgr_sys_get();

    strcpy((char*)product_id, (char*)sys_data->product_id);
    snprintf(request_url, sizeof(request_url), NETWORK_UPGRADE_URL, sys_data->product_id);

    buffer = (char*)malloc(UPGRADE_CONFIG_FILE_LEN);
    if(buffer == NULL)
    {   
        printf("%s buffer malloc error\n",__func__);
        return NULL;
    }

    memset(buffer, 0, UPGRADE_CONFIG_FILE_LEN);
    
    printf("\n\nrequest_url:%s\n", request_url);
    config_len = network_upg_download_config(request_url, buffer, UPGRADE_CONFIG_FILE_LEN);
    if (config_len <= 0){
        printf("%s(), line:%d. download upgrade config fail!\n", __func__, __LINE__);
        if(buffer)
        {
            free(buffer);
        }
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
        return NULL;
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

    local_version = (uint32_t)sys_data->firmware_version;
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
                goto fail_exit;
            }
        }
        else
        {
            printf("product name not match!\n");
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
        
    return NULL;
}

static void *network_upgrade_task(void *arg)
{
    //step 1
    //Read network upgrade config.
    char* upgrade_url = NULL;
    upgrade_url = network_check_upgrade_url();
    if (NULL == upgrade_url)
        return NULL;

    //step 2 
    //Download corresponding upgraded file.
    printf("* updatde request_url:%s\n", upgrade_url);
    network_upg_start(upgrade_url);
    return NULL;
}

int network_upgrade_start(void)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, network_upgrade_task, NULL)) 
    {
        pthread_attr_destroy(&attr);
        return -1;
    }
    pthread_attr_destroy(&attr);

    return 0;
}
#endif


static int wifi_mgr_callback_func(hccast_wifi_event_e event, void *in, void *out)
{
    app_log(LL_INFO, "[%s] event: %d", __func__, event);
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
        hccast_wifi_scan_result_t *res = (hccast_wifi_scan_result_t *)out;

        app_log(LL_INFO, "AP NUM: %d\n***********", res->ap_num);

        for (int i = 0; i < res->ap_num; i++)
        {
            app_log(LL_INFO, "ssid: %s, quality: %d", res->apinfo[i].ssid, res->apinfo[i].quality);
        }

        app_log(LL_INFO, "\n***********");
        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_SCAN_DONE;
        api_control_send_msg(&ctl_msg);

        break;
    }
    case HCCAST_WIFI_CONNECT:
    {
        memset(m_wifi_config.local_ip, 0, sizeof(m_wifi_config.local_ip));
        if (in != NULL)
        {
            memcpy(g_connecting_ssid, (char *)in, strlen((char *)in));
        }
        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTING;
        api_control_send_msg(&ctl_msg);
        break;
    }
    case HCCAST_WIFI_CONNECT_SSID:
    {
        app_log(LL_INFO, "SSID: %s", (char *)out);
        break;
    }
    case HCCAST_WIFI_CONNECT_RESULT: //station
    {
        hccast_udhcp_result_t *result = (hccast_udhcp_result_t *) out;
        if (result)
        {
            app_log(LL_INFO, "state: %d", result->stat);
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

                if ((hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR) || (hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
                {
                    printf("Cur scene is doing USB MIRROR\n");
                }
                else
                {
                    pthread_mutex_lock(&m_service_en_mutex);
                    if (network_service_enable_get())
                    {
                        if (!m_wifi_config.bReconnect || strcmp(result->last_ip, result->ip))
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
                snprintf(m_wifi_config.local_ip, MAX_IP_STR_LEN, "%s", result->ip);
                snprintf(m_wifi_config.local_gw, MAX_IP_STR_LEN, "%s", result->gw);
                app_set_wifi_connect_status(1);
                ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTED;
                api_control_send_msg(&ctl_msg);

                app_wifi_set_limited_internet(false);
                
#ifdef USBMIRROR_SUPPORT                
                if (!cast_um_dev_is_connected())
                {
                    app_ping_start_thread(m_wifi_config.local_gw);
                }    
 #endif
 
#ifdef AUTO_HTTP_UPGRADE
                if(g_http_upgrade_check_first == 0)
                {
                    g_http_upgrade_check_first = 1;
                    network_upgrade_start();
                }
#endif      
            }
            else
            {
                app_ping_stop_thread();
                app_set_wifi_connect_status(0);
                hccast_wifi_mgr_udhcpc_stop();
                hccast_wifi_mgr_disconnect_no_message();
                m_wifi_config.bConnected = false;  
                m_wifi_config.bConnectedByPhone = false;
                m_wifi_config.host_ap_ip_ready = false;
                m_wifi_config.sta_ip_ready = false;
                m_wifi_config.bReconnect = false;

                memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
                memset(m_wifi_config.local_ip, 0, sizeof(m_wifi_config.local_ip));

                usleep(50 * 1000);
                ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_DISCONNECTED;
                api_control_send_msg(&ctl_msg);

#ifdef __linux__
#else
                int wifi_ch = date_mgr_get_device_wifi_channel();
                if (HOSTAP_CHANNEL_AUTO == wifi_ch)
                {
                    //hccast_net_set_if_updown(HCCAST_HOSTAP_INF, HCCAST_NET_IF_UP);
                    //hccast_wifi_mgr_trigger_scan(HCCAST_HOSTAP_INF);

                    int *argv[2];
                    hccast_wifi_mgr_get_best_channel(2, argv);
                    hccast_wifi_mgr_set_best_channel(argv[0], argv[1]);
                }
#endif
                //hccast_wifi_mgr_hostap_start();
                if ((hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR) || (hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR))
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
            }

#ifdef SOC_HC15XX
            hccast_wifi_freq_mode_e mode = hccast_wifi_mgr_get_current_freq_mode();
            if (HCCAST_WIFI_FREQ_MODE_5G == mode)
            {
                app_softirqd_set_priority(CONFIG_SCHED_SOFTIRQD_PRIORITY - 1);
            }
            else
            {
                app_softirqd_set_priority(CONFIG_SCHED_SOFTIRQD_PRIORITY);
            }
#endif
        }

        break;
    }
    case HCCAST_WIFI_DISCONNECT:
    {
        app_ping_stop_thread();
        m_wifi_config.bConnected = false;  
        m_wifi_config.bConnectedByPhone = false;
        m_wifi_config.host_ap_ip_ready = false;
        m_wifi_config.sta_ip_ready = false;		
        m_wifi_config.bLimitedInternet = false;
        m_wifi_config.bReconnect = false;
        memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
        memset(m_wifi_config.local_ip, 0, sizeof(m_wifi_config.local_ip));
        memset(m_wifi_config.local_gw, 0, sizeof(m_wifi_config.local_gw));
        hccast_wifi_mgr_udhcpc_stop();
        app_set_wifi_connect_status(0);
        if (hccast_wifi_mgr_p2p_get_connect_stat() == 0)
        {
            printf("%s Wifi has been disconnected, beging change to host ap mode\n", __func__);
            hccast_media_stop();
            hccast_ap_dlna_aircast_stop();
#ifndef __linux__
#ifdef MIRACAST_SUPPORT
            hccast_mira_service_stop();
#endif
#endif

#ifdef __linux__
#else
            hostap_config_init();
#endif

            //hccast_wifi_mgr_hostap_start();
            pthread_mutex_lock(&m_service_en_mutex);
            if (network_service_enable_get())
            {
                app_wifi_switch_work_mode(WIFI_MODE_AP);
                #ifndef __linux__
                #ifdef MIRACAST_SUPPORT
                hccast_mira_service_start();
                #endif
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

            log(WIFI, INFO, "addr: %s", inet_ntoa(tmp_addr));

            strncpy(m_wifi_config.connected_phone_ip, inet_ntoa(tmp_addr), MAX_IP_STR_LEN);
        }
        //hccast_ap_dlna_aircast_start();
        m_wifi_config.sta_ip_ready = false;
        m_wifi_config.bConnected = false;
        break;
    }

    case HCCAST_WIFI_RECONNECT:
    {
        m_wifi_config.bReconnect = true;
        app_wifi_reconnect(NULL);
        break;
    }

    default:
        break;
    }

    return 0;

}

char *wifi_local_ip_get(void)
{
    return m_wifi_config.local_ip;
}

char *wifi_connected_phone_ip_get(void)
{
    return m_wifi_config.connected_phone_ip;
}

static void *wifi_disconnect_thread(void *args)
{
    printf("----------------------------wifi_disconnect_thread is running.-----------------------------\n");
    app_set_wifi_connect_status(0);
    hccast_wifi_mgr_udhcpc_stop();
    hccast_wifi_mgr_disconnect();

#ifdef SOC_HC15XX
    hccast_wifi_freq_mode_e mode = hccast_wifi_mgr_get_current_freq_mode();
    if (HCCAST_WIFI_FREQ_MODE_5G == mode)
    {
        app_softirqd_set_priority(CONFIG_SCHED_SOFTIRQD_PRIORITY - 1);
    }
    else
    {
        app_softirqd_set_priority(CONFIG_SCHED_SOFTIRQD_PRIORITY);
    }
#endif

    return NULL;
}

static void *wifi_connect_thread(void *args)
{
    hccast_wifi_ap_info_t *ap_wifi = (hccast_wifi_ap_info_t *)args;
    hccast_wifi_ap_info_t conn_wifi_ap;
    int index;
    printf("----------------------------wifi_connect_thread is running.-----------------------------\n");
    app_set_wifi_connect_status(0);
    hccast_stop_services();

    app_wifi_switch_work_mode(WIFI_MODE_STATION);
    
    memcpy(g_connecting_ssid, ap_wifi->ssid, sizeof(g_connecting_ssid));

    hccast_wifi_mgr_udhcpc_stop();

    memcpy(&conn_wifi_ap, ap_wifi, sizeof(hccast_wifi_ap_info_t));
    if (HCCAST_NET_WIFI_ECR6600U == hccast_wifi_mgr_get_wifi_model() \
        && conn_wifi_ap.encryptMode != HCCAST_WIFI_ENCRYPT_MODE_OPEN_WEP \
        && conn_wifi_ap.encryptMode != HCCAST_WIFI_ENCRYPT_MODE_SHARED_WEP \
        )
        conn_wifi_ap.encryptMode = HCCAST_WIFI_ENCRYPT_MODE_NOT_HANDLE;
    int ret = hccast_wifi_mgr_connect(&conn_wifi_ap);
    if (HCCAST_WIFI_ERR_USER_ABORT == ret)
    {
        hccast_wifi_mgr_udhcpc_stop();
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

        index = data_mgr_check_ap_saved(ap_wifi);
        printf("ssid index: %d\n", index);
        if (index >= 0) //set the index ap to first.
        {
            data_mgr_wifi_ap_delete(index);
            data_mgr_wifi_ap_save(ap_wifi);
        }
        else
        {
            data_mgr_wifi_ap_save(ap_wifi);
        }

        data_mgr_save();
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
        
        if (!network_get_net_enable())
        {
            goto EXIT;
        }

        usleep(50 * 1000);
        control_msg_t ctl_msg = {0};
        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_DISCONNECTED;//MSG_TYPE_NETWORK_WIFI_DISCONNECTED;
        api_control_send_msg(&ctl_msg);

#ifdef __linux
#else
        int wifi_ch = date_mgr_get_device_wifi_channel();
        if (HOSTAP_CHANNEL_AUTO == wifi_ch)
        {
            //hccast_net_set_if_updown(HCCAST_HOSTAP_INF, HCCAST_NET_IF_UP);
            //hccast_wifi_mgr_trigger_scan(HCCAST_HOSTAP_INF);

            int *argv[2];
            hccast_wifi_mgr_get_best_channel(2, argv);
            hccast_wifi_mgr_set_best_channel(argv[0], argv[1]);
        }
#endif

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

static void *wifi_switch_mode_thread(void *arg)
{
    int wifi_ch;
    hccast_wifi_freq_mode_e mode = (hccast_wifi_freq_mode_e)arg;

    app_data_t *app_data = data_mgr_app_get();

    if (HCCAST_WIFI_FREQ_MODE_24G == mode)
    {
        wifi_ch = app_data->wifi_ch;
    }
    else if (HCCAST_WIFI_FREQ_MODE_5G == mode)
    {
        wifi_ch = app_data->wifi_ch5g;
    }

    hccast_wifi_mgr_hostap_switch_mode_ex(mode, wifi_ch, 0);
    //hccast_wifi_mgr_hostap_switch_mode(mode);

    pthread_detach(pthread_self());

    return NULL;
}

static void *wifi_switch_hs_channel_thread(void *arg)
{
    hccast_mira_service_stop();
    hccast_wifi_mgr_hostap_switch_channel((int)arg);
    #ifdef MIRACAST_SUPPORT
    hccast_mira_service_start();
    #endif
    pthread_detach(pthread_self());

    return NULL;
}
#else // Ethernet 
char *eth_local_ip_get(void)
{
    return m_wifi_config.local_ip;
}
#endif // WIFI_SUPPORT
#endif // NETWORK_SUPPORT

static void network_cur_wifi_quality_update(void){
    if(!app_get_wifi_connect_status())
        return;

    char* cur_ssid = app_get_connecting_ssid();
    hccast_wifi_ap_info_t * ap_wifi = data_mgr_get_wifi_info(cur_ssid);
    if(!ap_wifi)
        return;

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

    data_mgr_save();
}

static int hccast_itoa(char *str, unsigned int val)
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

static int httpd_callback_func(hccast_httpd_event_e event, void *in, void *out)
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
    app_data_t *app_data = data_mgr_app_get();
    sys_data_t *sys_data = data_mgr_sys_get();
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
        printf("product_id: %s\n", sys_data->product_id);
        strcpy((char *)in, sys_data->product_id);
        break;
    case HCCAST_HTTPD_GET_DEV_VERSION:
        version = sys_data->firmware_version;
        hccast_itoa((char *)in, version);
        printf("version: %s\n", (char *)in);
        break;
    case HCCAST_HTTPD_GET_MIRROR_MODE:
        ret = app_data->mirror_mode;
        break;
    case HCCAST_HTTPD_SET_MIRROR_MODE:
        app_data->mirror_mode = (int)in;
        data_mgr_save();
        break;
    case HCCAST_HTTPD_GET_AIRCAST_MODE:
        ret = app_data->aircast_mode;
        break;
    case HCCAST_HTTPD_SET_AIRCAST_MODE:
        temp = (int)in;
        if (temp != app_data->aircast_mode)
        {
            app_data->aircast_mode = temp;
            data_mgr_save();
            if (hccast_get_current_scene() == HCCAST_SCENE_NONE)
            {
#ifdef AIRCAST_SUPPORT
                hccast_air_service_stop();
                hccast_air_service_start();
#endif
            }
        }
        break;
    case HCCAST_HTTPD_GET_MIRROR_FRAME:
        ret = app_data->mirror_frame;
        break;
    case HCCAST_HTTPD_SET_MIRROR_FRAME:
        app_data->mirror_frame = (int)in;
        data_mgr_save();
        break;
    case HCCAST_HTTPD_GET_BROWSER_LANGUAGE:
        ret = app_data->browserlang;
        break;
    case HCCAST_HTTPD_SET_BROWSER_LANGUAGE:
        app_data->browserlang = (int)in;
        data_mgr_save();
        break;
    case HCCAST_HTTPD_GET_SYS_RESOLUTION:
        ret = data_mgr_app_tv_sys_get();
        break;
    case HCCAST_HTTPD_SET_SYS_RESOLUTION:
    {
        int support_tv_type;
        ap_tv_sys = (int)in;
        if (ap_tv_sys != APP_TV_SYS_AUTO &&
                data_mgr_app_tv_sys_get() == ap_tv_sys)
        {
            printf("%s(), same tvsys:%d, not change TV sys\n",
                   __func__, ap_tv_sys);
            break;
        }

        last_resolution = data_mgr_app_tv_sys_get();

        support_tv_type = tv_sys_app_set(ap_tv_sys);
        if (support_tv_type >= 0)
        {
            printf("%s(), line:%d. save app tv sys: %d， tv_type:%d!\n",
                   __func__, __LINE__, ap_tv_sys, support_tv_type);
            data_mgr_app_tv_sys_set(ap_tv_sys);
            data_mgr_de_tv_sys_set(support_tv_type);

            if (((last_resolution == APP_TV_SYS_4K) && (ap_tv_sys != APP_TV_SYS_4K)) \
                    || ((last_resolution != APP_TV_SYS_4K) && (ap_tv_sys == APP_TV_SYS_4K)) \
                    || ((last_resolution != APP_TV_SYS_AUTO) && (ap_tv_sys == APP_TV_SYS_AUTO)) \
                    || ((last_resolution == APP_TV_SYS_AUTO) && (ap_tv_sys != APP_TV_SYS_AUTO)) \
               )
            {
                cur_scene = hccast_get_current_scene();
                if ((cur_scene != HCCAST_SCENE_AIRCAST_PLAY) && (cur_scene != HCCAST_SCENE_AIRCAST_MIRROR))
                {
                    if (hccast_air_service_is_start())
                    {
#ifdef AIRCAST_SUPPORT
                        hccast_air_service_stop();
                        hccast_air_service_start();
#endif
                    }
                    
#ifdef MIRACAST_SUPPORT
                    if (data_mgr_de_tv_sys_get() >= TV_LINE_4096X2160_30)
                    {
                        hccast_mira_service_set_resolution(HCCAST_MIRA_RES_1080F30);
                    }
                    else
                    {
                        cast_api_mira_set_default_res();
                    }
#endif  
                }
            }

            data_mgr_save();
        }
        else
        {
            ret = -1;
        }
    }
        break;
    case HCCAST_HTTPD_GET_DEVICE_MAC:
        api_get_mac_addr(mac);
        memcpy(in, mac, sizeof(mac));
        break;
    case HCCAST_HTTPD_GET_DEVICE_NAME:
        strcpy((char *)in, app_data->cast_dev_name);
        break;
    case HCCAST_HTTPD_SET_DEVICE_NAME:
        strncpy(app_data->cast_dev_name, (char *)in, SERVICE_NAME_MAX_LEN);
        data_mgr_cast_dev_name_changed_set(1);
        data_mgr_save();
        api_system_reboot();
        msg.msg_type = MSG_TYPE_NETWORK_DEV_NAME_SET;
        api_control_send_msg(&msg);
        break;
    case HCCAST_HTTPD_GET_DEVICE_PSK:
        strcpy((char *)in, app_data->cast_dev_psk);
        break;
    case HCCAST_HTTPD_SET_DEVICE_PSK:
        strncpy(app_data->cast_dev_psk, (char *)in, DEVICE_PSK_MAX_LEN);
        data_mgr_save();
        api_system_reboot();
        break;
    case HCCAST_HTTPD_SET_SYS_RESTART:
        printf("HCCAST_HTTPD_SET_SYS_RESTART\n");
        api_system_reboot();
        break;
    case HCCAST_HTTPD_SET_SYS_RESET:
        printf("HCCAST_HTTPD_SET_SYS_RESET\n");
        data_mgr_factory_reset();
        factary_init = 1;
        api_system_reboot();
        break;

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
    case HCCAST_HTTPD_WIFI_AP_DISCONNECT:
    {
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x2000);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));
        
        if(pthread_create(&tid, &attr,wifi_disconnect_thread, NULL) != 0)
        {
            pthread_attr_destroy(&attr);
            return -1;
        }
        
        pthread_attr_destroy(&attr);
        break;
    }
    case HCCAST_HTTPD_CHECK_AP_SAVE:
    {
        check_ap = (hccast_wifi_ap_info_t *)in;
        save_ap = (hccast_wifi_ap_info_t *)out;
        index = data_mgr_check_ap_saved(check_ap);

        if (index >= 0)
        {
            if (save_ap)
            {
                strcpy(save_ap->ssid, app_data->wifi_ap[index].ssid);
                strcpy(save_ap->pwd, app_data->wifi_ap[index].pwd);
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
                
                if (m_wifi_config.wifi_connecting || !network_get_net_enable())
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
                    return -1;
                } 
                
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
        if (del_ap)
        {
            index = data_mgr_check_ap_saved(del_ap);
            printf("del_ap->ssid:%s, index: %d\n", del_ap->ssid, index);
            if (index >= 0)
            {
                data_mgr_wifi_ap_delete(index);
                data_mgr_save();
            }
        }
        break;
    case HCCAST_HTTPD_GET_CUR_WIFI_INFO:
    {
        char cur_ssid[64] = {0};
        hccast_wifi_ap_info_t *cur_ap;
        network_cur_wifi_quality_update();
        ret = hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
        cur_ap = data_mgr_get_wifi_info(cur_ssid);
        if (cur_ap)
            memcpy(in, cur_ap, sizeof(hccast_wifi_ap_info_t));
        break;
    }
#endif
#endif
    case HCCAST_HTTPD_SHOW_PROGRESS:
    {
        printf("progress: %d\n", (int)in);
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
        hccast_web_upload_info_st *info = NULL;
        info = (hccast_web_upload_info_st *)in;
        if (info)
        {
            printf("upload len: %d\n", info->length);
            printf("upload buf: %p\n", info->buf);
            //free(info->buf);
        }
        sys_upg_flash_burn(info->buf, info->length);
        break;
    }
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
    case HCCAST_HTTPD_GET_WIFI_FREQ_MODE_EN:
        if (HCCAST_WIFI_FREQ_MODE_5G == hccast_wifi_mgr_freq_support_mode() \
                /*HCCAST_WIFI_FREQ_MODE_60G == hccast_wifi_mgr_freq_support_mode()*/)
        {
            ret = 1;
        }
        else
        {
            ret = 0;
        }
        break;
    case HCCAST_HTTPD_GET_WIFI_FREQ_MODE:
        return hccast_wifi_mgr_get_current_freq_mode();
        //ret = app_data->wifi_mode;
        //break;
    case HCCAST_HTTPD_SET_WIFI_FREQ_MODE:
    {
        if ((int)in == app_data->wifi_mode)
        {
            return 0;
        }

        app_data->wifi_mode = (int)in;
        data_mgr_save();
        hccast_wifi_mgr_hostap_stop();
        api_system_reboot();
        break;
    }
    case HCCAST_HTTPD_GET_WIFI_HS_CHANNEL:
    {
        int wifi_ch = date_mgr_get_device_wifi_channel();
        if (HOSTAP_CHANNEL_AUTO == wifi_ch)
        {
            return wifi_ch;
        }

#if 1   // rtos throught wpas get freq possible error.
        wifi_ch = hccast_wifi_mgr_get_current_freq(); 
#endif

        app_log(LL_INFO, "get hs channel: %d/%d\n", wifi_ch, hccast_wifi_mgr_get_current_freq());

        return wifi_ch;
    }
    case HCCAST_HTTPD_GET_WIFI_HS_CHANNEL_BY_FREQ_MODE:
    {
        return date_mgr_get_device_wifi_channel_by_mode((int)in);
    }
    case HCCAST_HTTPD_SET_WIFI_HS_CHANNEL:
    {
        int wifi_ch = (int)in;
        if (wifi_ch == app_data->wifi_ch && HCCAST_WIFI_FREQ_MODE_24G == hccast_wifi_mgr_get_current_freq_mode())
        {
            return 0;
        }

        if (wifi_ch == app_data->wifi_ch5g && HCCAST_WIFI_FREQ_MODE_5G == hccast_wifi_mgr_get_current_freq_mode())
        {
            return 0;
        }

        if (wifi_ch >= 0 && wifi_ch <= 14)
        {
            app_data->wifi_ch = wifi_ch;
        }
        else if (wifi_ch >= 34 && wifi_ch <= 196)
        {
            app_data->wifi_ch5g = wifi_ch;
        }

        data_mgr_save();

#ifdef __linux__
#else
        if (HOSTAP_CHANNEL_AUTO == wifi_ch)
        {
            //hccast_net_set_if_updown(HCCAST_HOSTAP_INF, HCCAST_NET_IF_UP);
            //hccast_wifi_mgr_trigger_scan(HCCAST_HOSTAP_INF);

            int *argv[2];
            hccast_wifi_mgr_get_best_channel(2, argv);
            hccast_wifi_mgr_set_best_channel(argv[0], argv[1]);

            if (1 == app_data->wifi_mode) // 24G
            {
                wifi_ch = (int)argv[0];
            }
            else if (2 == app_data->wifi_mode) // 5G
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

        api_system_reboot();
        break;
    }
#endif
#endif
    case HCCAST_HTTPD_GET_CUR_SCENE_PLAY:
        if (hccast_air_audio_state_get())
        {
            printf("%s %d\n", __func__, __LINE__);
            ret = 1;
        }
        else if (hccast_get_current_scene() != HCCAST_SCENE_NONE)
        {
            if ((hccast_get_current_scene() == HCCAST_SCENE_AIRCAST_PLAY) \
                || (hccast_get_current_scene() == HCCAST_SCENE_DLNA_PLAY) \
                || (hccast_get_current_scene() == HCCAST_SCENE_DIAL_PLAY) )
            {
                if (hccast_media_get_status() == HCCAST_MEDIA_STATUS_STOP)
                {
                    ret = 0;
                }
                else
                {
                    printf("%s %d, scene:%d, status:%d\n", __func__, __LINE__, hccast_get_current_scene(), hccast_media_get_status());
                    ret = 1;
                }
            }
            else
            {
                printf("%s %d, scene:%d\n", __func__, __LINE__, hccast_get_current_scene());
                ret = 1;
            }
        }
        else
        {
            ret = 0;
        }
        break;
    case HCCAST_HTTPD_STOP_MIRA_SERVICE:
#ifndef __linux__
        hccast_scene_set_mira_restart_flag(0);
        usleep(300 * 1000); // mira restart check thread usleep (200*1000).
#endif
        if (network_get_net_enable())
        {
            #ifdef MIRACAST_SUPPORT
            hccast_mira_service_stop();
            #endif
        }  
        break;
    case HCCAST_HTTPD_START_MIRA_SERVICE:
        if (network_get_net_enable())
        {
            #ifdef MIRACAST_SUPPORT
            hccast_mira_service_start();
            #endif
        }    
        break;
    case HCCAST_HTTPD_GET_MIRROR_ROTATION:
        ret = app_data->mirror_rotation;
        break;
    case HCCAST_HTTPD_SET_MIRROR_ROTATION:
    {
        temp = (int)in;
        if (app_data->mirror_rotation != temp)
        {
            app_data->mirror_rotation = temp;
            data_mgr_save();
        }
        break;
    }
    case HCCAST_HTTPD_GET_MIRROR_VSCREEN_AUTO_ROTATION:
        ret = app_data->mirror_vscreen_auto_rotation;
        break;
    case HCCAST_HTTPD_SET_MIRROR_VSCREEN_AUTO_ROTATION:
    {
        temp = (int)in;
        if (app_data->mirror_vscreen_auto_rotation != temp)
        {
            app_data->mirror_vscreen_auto_rotation = temp;
            data_mgr_save();
        }
         break;
    } 
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
    case HCCAST_HTTPD_GET_WIFI_CONNECT_STATUS:
        ret = hccast_wifi_mgr_get_connect_status();
        break;
    case HCCAST_HTTPD_GET_CUR_WIFI_SSID:
        ret = hccast_wifi_mgr_get_connect_ssid(cur_ssid, sizeof(cur_ssid));
        memcpy(in, cur_ssid, sizeof(cur_ssid));
        break;
    case HCCAST_HTTPD_WIFI_SCAN:
        scan_res = (hccast_wifi_scan_result_t *)in;
        if (scan_res)
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
#endif
#endif
    case HCCAST_HTTPD_GET_NOT_SUPPORT_4K:
#ifdef NOT_SUPPORT_4K
        ret = 1;
#else
        ret = 0;
#endif
        break;
    case HCCAST_HTTPD_GET_UPGRADE_URL:    
        sprintf(in, NETWORK_UPGRADE_URL, sys_data->product_id);     
        break;
    case HCCAST_HTTPD_START_ONLINE_UPGRADE:
        network_upg_start(in);
        break;
    case HCCAST_HTTPD_MSG_USER_UPGRADE_ABORT:
        if(network_upg_get_status() == NETWORK_UPG_DOWNLOAD)
        {
            network_upg_set_user_abort(1);
            ret = 1;
        }    
        break;
    case HCCAST_HTTPD_SET_AIRP2P_SWITCH_MODE:
    {
        temp = (int)in;
        if (app_data->airp2p_mode_switch != temp)
        {
            app_data->airp2p_mode_switch = temp;
            data_mgr_save();
        }
        break;
    }      
    case HCCAST_HTTPD_GET_AIRP2P_SWITCH_MODE:
    {       
#ifdef AIRP2P_SUPPORT
        ret = app_data->airp2p_mode_switch;
#endif
        break;
    }        
    default :
        break;
    }

    return ret;
}

static void media_callback_func(hccast_media_event_e msg_type, void *param)
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
        printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_DLNA, media_type: %d\n", __func__, __LINE__, (hccast_media_type_e)param);

        ctl_msg.msg_type = MSG_TYPE_CAST_DLNA_START;
        ctl_msg.msg_code = (uint32_t)param;
        break;
    case HCCAST_MEDIA_EVENT_URL_FROM_DIAL:
        printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_DIAL, media_type: %d\n", __func__, __LINE__, (hccast_media_type_e)param);

        ctl_msg.msg_type = MSG_TYPE_CAST_DIAL_START;
        ctl_msg.msg_code = (uint32_t)param;
        break;
    case HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST:
        printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST, media_type: %d\n", __func__, __LINE__, (hccast_media_type_e)param);
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
        *(int*)param = data_mgr_cast_rotation_get();           
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
        bool win_cast_root_wait_open(uint32_t timeout);
        win_cast_root_wait_open(20000);
        cast_usb_mirror_start();
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
        cast_usb_mirror_stop();
        bool win_dlna_wait_open(uint32_t timeout);
        menu_status = win_dlna_wait_open(20000);
        if(media_paly_info && !menu_status)
        {
            media_paly_info->enable_url_play = 0;
        }
        // printf("[%s] wait dlna menu open end tick: %d\n",__func__,(int)time(NULL));
    }
}

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
#ifdef __linux__
static void network_probed_wifi(void)
{
    if (0 == access("/var/lib/misc/RTL8188FTV.probe", F_OK))
    {
        printf("Wi-Fi: RTL8188FTV\n");
        m_probed_wifi_module = HCCAST_NET_WIFI_8188FTV;
    }
    else if (0 == access("/var/lib/misc/RTL8811FTV.probe", F_OK))
    {
        printf("Wi-Fi: RTL8811FTV\n");
        m_probed_wifi_module = HCCAST_NET_WIFI_8811FTV;
    }

    //it must have node wlan0 to make wifi work.
    if (access("/var/run/wpa_supplicant/wlan0", F_OK))
    {
        m_probed_wifi_module = 0;
    }

    hccast_wifi_mgr_set_wifi_model(m_probed_wifi_module);
}

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
        is_file_exist("/var/run/hostapd/wlan0") && 
        is_file_exist("/var/run/wpa_supplicant/p2p0") && 
        is_file_exist("/var/run/wpa_supplicant/wlan0")
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

static int hostap_connect_update(void)
{
    int cur_connect = 0;
    int pre_connect = 0;
    static int needfresh = 0;
    control_msg_t ctl_msg = {0};

    //hostap not ready.
    if ((hccast_wifi_mgr_get_hostap_status() == 0) || hccast_wifi_mgr_p2p_get_connect_stat() \
        || factary_init || !network_get_net_enable())
    {
        needfresh = 0;
        hostap_set_connect_count(0);
        return 0;
    }

    if (hccast_wifi_mgr_p2p_get_connect_stat())
    {
        //printf("########%s mira connect do not anything.#########\n", __FUNCTION__);
        return 0;
    }

    cur_connect = hccast_wifi_mgr_hostap_get_sta_num(NULL);
    pre_connect = hostap_get_connect_count();

#if 0
    if (hostap_discover_ok == 0)
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
                {
                    hccast_ap_dlna_aircast_start();
                }
            }  
            m_wifi_config.bConnectedByPhone = true;
            m_wifi_config.host_ap_ip_ready = true;
            ctl_msg.msg_type = MSG_TYPE_NETWORK_DEVICE_BE_CONNECTED;
            api_control_send_msg(&ctl_msg);
        }
    }

    return 0;
}

static void *hostap_connect_thread(void *args)
{
    printf("----------------------------hccast_hostap_connect_thread is running.-----------------------------\n");
    while (1)
    {
        hostap_connect_update();
        api_sleep_ms(500);
    }
    return NULL;
}

static void hostap_connect_detect_init()
{
    pthread_t tid;

    if (pthread_create(&tid, NULL, hostap_connect_thread, NULL) < 0)
    {
        printf("Create hccast_hostap_connect_thread error.\n");
    }
}
#endif
#endif

int network_init(void)
{
    static bool network_init_flag = false;

    if (network_init_flag)
        return 0;
    network_init_flag = true;
#ifdef NETWORK_SUPPORT
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

    hccast_wifi_mgr_init(wifi_mgr_callback_func);
#endif
    hccast_httpd_service_init(httpd_callback_func);
#endif

    hccast_media_init(media_callback_func);
    cast_init();

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
#ifdef __linux__
    network_probed_wifi();
#endif
    hostap_connect_detect_init();
#endif
#endif

    hccast_httpd_service_start();
    
    return API_SUCCESS;
}

int network_deinit(void)
{
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
    hccast_wifi_mgr_uninit();
#endif
    hccast_httpd_service_uninit();
#ifdef MIRACAST_SUPPORT
    hccast_mira_service_uninit();
#endif
#ifdef DLNA_SUPPORT
    hccast_dlna_service_uninit();
#endif
#endif
    return API_SUCCESS;
}

#ifdef AIRP2P_SUPPORT
int network_airp2p_start(void)
{
    control_msg_t ctl_msg = {0};
    char hostname[64] = {0};
    
    data_mgr_init_device_name();

    ctl_msg.msg_type = MSG_TYPE_AIRP2P_READY;
    api_control_send_msg(&ctl_msg);

    sprintf((char *)hostname, "%s_itv_p2p", data_mgr_get_device_name());
    
    sethostname(hostname, strlen(hostname));
    hccast_air_set_resolution(1728, 972, 60);
    hccast_air_p2p_start("wlan0", 149);
    hccast_air_service_start();
    
    return 0;
}
#endif


#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
static void hostap_config_init(void)
{
    hccast_wifi_hostap_conf_t conf = {0};

    int wifi_mode = hccast_wifi_mgr_freq_support_mode();

    //! The wpas conf will be overwritten here
    conf.mode    = date_mgr_get_device_wifi_mode();
    if (conf.mode <= wifi_mode)
    {
        //conf.mode = date_mgr_get_device_wifi_mode();
        conf.channel = date_mgr_get_device_wifi_channel();
    }
    else
    {
        conf.mode = wifi_mode;

        if (HOSTAP_CHANNEL_AUTO != date_mgr_get_device_wifi_channel())
        {
            conf.channel = date_mgr_get_device_wifi_channel_by_mode(conf.mode);
        }
    }

    app_log(LL_INFO, "#### supp_mode:%d, conf.mode:%d, conf.ch:%d\n", wifi_mode, conf.mode, conf.channel);

    //strcpy(conf.country_code, "CN");
    strncpy(conf.pwd, data_mgr_get_device_psk(), sizeof(conf.pwd));
    strncpy(conf.ssid, data_mgr_get_device_name(), sizeof(conf.ssid));

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

#define WIFI_CHECK_TIME 50000000
/* WIFI_SUPPORT */
static void *network_connect_task(void *arg)
{
    hccast_wifi_ap_info_t wifi_ap;
    hccast_wifi_ap_info_t conn_wifi_ap;
    int loop_cnt = WIFI_CHECK_TIME / 100;
    control_msg_t ctl_msg = {0};
    char connect_fail = 0;
    app_data_t  *app_data = data_mgr_app_get();

    while (!network_wifi_module_get())
    {
        api_sleep_ms(100);
        if (0 == loop_cnt--){
            connect_fail = 1;
            goto connect_exit;
        }
        
        if (!network_get_net_enable()){
            connect_fail = 1;
            goto connect_exit;
        }        
    }

#ifdef AIRP2P_SUPPORT
    if (app_data->airp2p_en)
    {
        network_airp2p_start();
        return NULL;
    }    
#endif

    network_init();

    //step 1: http server startup

    data_mgr_init_device_name();
    hostap_config_init();
    udhcpc_set_hostname(app_data->cast_dev_name);
    
#if 1
    //step 2:
    if (data_mgr_wifi_ap_get(&wifi_ap))
    {
        hccast_wifi_mgr_udhcpc_stop();

        //Get wifi AP from flash, connect wifi
        printf("%s(), line:%d, connect to %s, pwd:%s\n", __FUNCTION__, __LINE__, \
               wifi_ap.ssid, wifi_ap.pwd);
        memcpy(g_connecting_ssid, wifi_ap.ssid, sizeof(g_connecting_ssid));         

        app_wifi_switch_work_mode(WIFI_MODE_STATION);
        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTING;
        api_control_send_msg(&ctl_msg);

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

            if (!network_get_net_enable())
            {
                goto connect_exit;
            }
            
            data_mgr_wifi_ap_delete(0);//delete current wifi info.
            data_mgr_save();
            
            usleep(50 * 1000);
            ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL;
            api_control_send_msg(&ctl_msg);

            hccast_wifi_mgr_disconnect_no_message();
            api_sleep_ms(50);

#ifdef __linux__
#else
            int wifi_ch = date_mgr_get_device_wifi_channel();
            if (HOSTAP_CHANNEL_AUTO == wifi_ch)
            {
                //hccast_net_set_if_updown(HCCAST_HOSTAP_INF, HCCAST_NET_IF_UP);
                //hccast_wifi_mgr_trigger_scan(HCCAST_HOSTAP_INF);

                int *argv[2];
                hccast_wifi_mgr_get_best_channel(2, argv);
                hccast_wifi_mgr_set_best_channel(argv[0], argv[1]);
            }
#endif

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
    }
    else
    {
#ifdef __linux__
#else
        int wifi_ch = date_mgr_get_device_wifi_channel();
        if (HOSTAP_CHANNEL_AUTO == wifi_ch)
        {
            hccast_net_set_if_updown(HCCAST_HOSTAP_INF, HCCAST_NET_IF_UP);
            hccast_wifi_mgr_trigger_scan(HCCAST_HOSTAP_INF);

            int *argv[2];
            hccast_wifi_mgr_get_best_channel(2, argv);
            hccast_wifi_mgr_set_best_channel(argv[0], argv[1]);
        }
#endif
        //No wif AP in flash, entering AP mode
        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTED;
        api_control_send_msg(&ctl_msg);
        
        pthread_mutex_lock(&m_service_en_mutex);
        if(network_service_enable_get())
        {
            //No wif AP in flash, entering AP mode
            app_wifi_switch_work_mode(WIFI_MODE_AP);
            #ifdef MIRACAST_SUPPORT
            hccast_mira_service_start();
            #endif
        }
        pthread_mutex_unlock(&m_service_en_mutex);

#ifdef SOC_HC15XX
        hccast_wifi_freq_mode_e mode = hccast_wifi_mgr_get_current_freq_mode();
        if (HCCAST_WIFI_FREQ_MODE_5G == mode)
        {
            app_softirqd_set_priority(CONFIG_SCHED_SOFTIRQD_PRIORITY - 1);
        }
        else
        {
            app_softirqd_set_priority(CONFIG_SCHED_SOFTIRQD_PRIORITY);
        }
#endif

#endif
        printf("%s(), line:%d. AP mode start\n", __FUNCTION__, __LINE__);
    }

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
#else
static void udhcpc_cb(unsigned int data)
{
    hccast_udhcp_result_t *in = (hccast_udhcp_result_t *)data;
    control_msg_t ctl_msg = {0};

    if (in)
    {
        printf("udhcpc got ip: %s\n", in->ip);
        strncpy(m_wifi_config.local_ip, in->ip, MAX_IP_STR_LEN);

        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_CONNECTED;
        hccast_start_services();
    }
    else
    {
        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_DISCONNECTED;
    }

    api_control_send_msg(&ctl_msg);
}

/* no WIFI_SUPPORT */
static void *network_connect_task(void *arg)
{
    static udhcp_conf_t eth_udhcpc_conf =
    {
        .func       = udhcpc_cb,
        .ifname     = UDHCP_IF_NONE,
        .pid        = 0,
        .run        = 0
    };

    udhcpc_start(&eth_udhcpc_conf);

    network_init();
    data_mgr_init_device_name();
    //hccast_start_services();
}
#endif

//Connet wifi ap if there is valid wifi AP information in flash. Otherwise
//set device to AP mdoe
int network_connect(void)
{
#ifdef WIFI_SUPPORT
    pthread_mutex_lock(&m_wifi_state_mutex);
    if (m_wifi_config.wifi_connecting || !network_get_net_enable())
    {
        pthread_mutex_unlock(&m_wifi_state_mutex);
        return API_SUCCESS;
    }

    m_wifi_config.wifi_connecting = true;
    pthread_mutex_unlock(&m_wifi_state_mutex);
#endif

    pthread_t thread_id = 0;
    pthread_attr_t attr;
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&thread_id, &attr, network_connect_task, NULL))
    {
        m_wifi_config.wifi_connecting = false;
        return API_FAILURE;
    }

    pthread_mutex_unlock(&m_wifi_state_mutex);
    return API_SUCCESS;
}

static volatile bool m_service_enable = true;
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

#endif // NETWORK_SUPPORT


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
}

static void wifi_do_connect(hccast_wifi_ap_info_t *wifi_info)
{
    char wifi_ap_exist = 0;
    hccast_wifi_ap_info_t wifi_ap;
    hccast_wifi_ap_info_t conn_wifi_ap;
    control_msg_t ctl_msg = {0};

    if (NULL == wifi_info){
        if (data_mgr_wifi_ap_get(&wifi_ap))
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
        if (m_wifi_config.bReconnect)
        {
            hccast_mira_service_stop();
            hccast_scene_set_mira_restart_flag(1);
        }
#endif

        //Get wifi AP from flash, connect wifi
        printf("%s(), line:%d, connect to %s, pwd:%s\n", __FUNCTION__, __LINE__, \
               wifi_ap.ssid, wifi_ap.pwd);

        memcpy(g_connecting_ssid, wifi_ap.ssid, sizeof(g_connecting_ssid));

        memcpy(&conn_wifi_ap, &wifi_ap, sizeof(hccast_wifi_ap_info_t));
        if (HCCAST_NET_WIFI_ECR6600U == hccast_wifi_mgr_get_wifi_model() \
            && conn_wifi_ap.encryptMode != HCCAST_WIFI_ENCRYPT_MODE_OPEN_WEP \
            && conn_wifi_ap.encryptMode != HCCAST_WIFI_ENCRYPT_MODE_SHARED_WEP \
            )
            conn_wifi_ap.encryptMode = HCCAST_WIFI_ENCRYPT_MODE_NOT_HANDLE;
        int ret = hccast_wifi_mgr_connect(&conn_wifi_ap);
        if (HCCAST_WIFI_ERR_USER_ABORT == ret)
        {
            printf("%s(), line:%d. user abort.\n", __FUNCTION__, __LINE__);
            m_wifi_config.bConnected = false;  
            m_wifi_config.bConnectedByPhone = false;
            m_wifi_config.host_ap_ip_ready = false;
            m_wifi_config.sta_ip_ready = false;
            m_wifi_config.bReconnect = false;
            memset(m_wifi_config.local_ip, 0, MAX_IP_STR_LEN);
            memset(g_connecting_ssid, 0, sizeof(g_connecting_ssid));

            hccast_wifi_mgr_disconnect_no_message();

            return ;
        }
        
        if (hccast_wifi_mgr_get_connect_status())
        {
            hccast_wifi_mgr_udhcpc_start();

            //sysdata_wifi_ap_resave(&wifi_ap);
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

            if (!network_get_net_enable())
            {
                return ;
            }

            usleep(50*1000);
            
            if (m_wifi_config.bReconnect)
            {
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
    hccast_wifi_ap_info_t *wifi_ap = (hccast_wifi_ap_info_t*)arg;
    //m_wifi_connecting = true;

    if (network_wifi_module_get()){
        wifi_do_connect(wifi_ap);
    }

    pthread_mutex_lock(&m_wifi_state_mutex);
    m_wifi_config.wifi_connecting = false;
    pthread_mutex_unlock(&m_wifi_state_mutex);
    return NULL;
}

int app_wifi_reconnect(hccast_wifi_ap_info_t *wifi_ap)
{
    pthread_mutex_lock(&m_wifi_state_mutex);
    if (m_wifi_config.wifi_connecting || !network_get_net_enable())
    {
        pthread_mutex_unlock(&m_wifi_state_mutex);
        return 1;
    }
    
    m_wifi_config.wifi_connecting = true;
    pthread_t thread_id = 0;
    pthread_attr_t attr;
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&thread_id, &attr, wifi_connect_task, (void*)wifi_ap) != 0)
    {
        m_wifi_config.wifi_connecting = false;
        pthread_mutex_unlock(&m_wifi_state_mutex);
        return -1;
    }
    
    pthread_attr_destroy(&attr);
    pthread_mutex_unlock(&m_wifi_state_mutex);
    return 0;
}

int app_wifi_switch_work_mode(WIFI_MODE_e wifi_mode)
{
    pthread_mutex_lock(&m_wifi_switch_mode_mutex);

    if (WIFI_MODE_STATION == wifi_mode)
    {
        if (hccast_wifi_mgr_get_hostap_status())
        {
            hccast_wifi_mgr_hostap_stop();
        }
#ifdef __HCRTOS__
        if (!hccast_wifi_mgr_get_station_status())
        {
            hccast_wifi_mgr_enter_sta_mode();
        }
#endif   
    } 
    else if (WIFI_MODE_AP == wifi_mode)
    {
#ifdef __HCRTOS__
        if (hccast_wifi_mgr_get_station_status())
        {
            hccast_wifi_mgr_exit_sta_mode();
        }
#endif
        if (!hccast_wifi_mgr_get_hostap_status()) 
        {
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
    } 
    else if (WIFI_MODE_NONE == wifi_mode)
    {
        if (hccast_wifi_mgr_get_hostap_status())
        {      
            hccast_wifi_mgr_hostap_stop();
        }
#ifdef __HCRTOS__
        if (hccast_wifi_mgr_get_station_status())
        {
            hccast_wifi_mgr_exit_sta_mode();
        }
#endif
    }
    m_wifi_config.mode = wifi_mode;

    pthread_mutex_unlock(&m_wifi_switch_mode_mutex);
    return 0;
}

int network_disable_net_state(void)
{
    network_set_net_enable(0);
    network_service_enable_set(0);
    hccast_scene_set_mira_restart_flag(0);
    
    while(m_wifi_config.wifi_connecting || m_wifi_config.wifi_scanning 
#ifdef MIRACAST_SUPPORT
        || hccast_mira_get_restart_state()
#endif
    )
    {
        hccast_wifi_mgr_op_abort();
        usleep(10*1000);
    }
    
    app_set_wifi_connect_status(0);
    hccast_wifi_mgr_udhcpc_stop();
    hccast_stop_services();
    app_wifi_switch_work_mode(WIFI_MODE_NONE);
    
    printf("%s done.\n", __func__);
    
    return 0;
}

int network_enable_net_state(void)
{
    network_set_net_enable(1);
    network_service_enable_set(1);
    network_connect();
    
    printf("%s done.\n", __func__);
    
    return 0;
}

void network_start_services(void)
{
#ifdef WIFI_SUPPORT
    int wifi_connecting = 0;

    if (g_wl_services_enable)
    {
        return;
    }

    g_wl_services_enable = 1;

    if (hccast_get_current_scene() != HCCAST_SCENE_NONE)
    {
        hccast_scene_switch(HCCAST_SCENE_NONE);
    }
    
    pthread_mutex_lock(&m_wifi_state_mutex);
    wifi_connecting = m_wifi_config.wifi_connecting;
    pthread_mutex_unlock(&m_wifi_state_mutex);

    if (!wifi_connecting)
    {
    
        if (app_get_wifi_connect_status())
        {
            hccast_start_services();
        }
        else
        {
            if (hostap_get_connect_count())
            {
                #ifdef DLNA_SUPPORT
                hccast_dlna_service_start();
                #endif 

                #ifdef AIRCAST_SUPPORT
                hccast_air_service_start();
                #endif        
            }
            
            #ifdef MIRACAST_SUPPORT
            hccast_mira_service_start();
            #endif
        }
    }
    
    if (!hccast_httpd_service_is_start())
    {
        hccast_httpd_service_start();
    }  
    
#endif    

    printf("[network_start_services] done.\n");
}

void network_stop_services(void)
{
#ifdef WIFI_SUPPORT
    int wifi_need_reconnect = 0;
    
    if (!g_wl_services_enable)
    {
        return;
    }

    if (hccast_get_current_scene() == HCCAST_SCENE_MIRACAST)
    {
        wifi_need_reconnect = 1;
    }

    g_wl_services_enable = 0;
    hccast_stop_services();
    hccast_httpd_service_stop();
    
    if (wifi_need_reconnect)
    {
        app_wifi_reconnect(NULL);        
    }
    
#endif    

    printf("[network_stop_services] done.\n");
}

