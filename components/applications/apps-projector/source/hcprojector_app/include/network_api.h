/**
 * network_api.h
 */

#ifndef __NETWORK_API_H__
#define __NETWORK_API_H__

#ifdef WIFI_SUPPORT
#include <hccast/hccast_wifi_mgr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************
NETWORK_UPGRADE_URL:
config name: HCFOTA.jsonp
example:http://http://172.16.12.81:80/hccast/rtos/HC15A210/hcprojector/HCFOTA.jsonp
**********************************************************************/
#ifndef  CONFIG_HTTP_UPGRADE_URL
#define  CONFIG_HTTP_UPGRADE_URL "http://172.16.12.81:80"
#endif    

#ifdef __linux__
//#define NETWORK_UPGRADE_URL "http://172.16.12.81:80/hccast/linux/%s/hcprojector/HCFOTA.jsonp"
#define NETWORK_UPGRADE_URL (CONFIG_HTTP_UPGRADE_URL "/hccast/linux/%s/hcprojector/HCFOTA.jsonp")
#else
//#define NETWORK_UPGRADE_URL "http://172.16.12.81:80/hccast/rtos/%s/hcprojector/HCFOTA.jsonp"
#define NETWORK_UPGRADE_URL (CONFIG_HTTP_UPGRADE_URL "/hccast/rtos/%s/hcprojector/HCFOTA.jsonp")
#endif

#define MAX_IP_STR_LEN 32
#define NETWORK_WIFI_CONNECTING_TIMEOUT  (60)

typedef enum{
    WIFI_MODE_NONE,
    WIFI_MODE_STATION,
    WIFI_MODE_AP,
}WIFI_MODE_e;

typedef struct
{
    char local_ip[MAX_IP_STR_LEN];
    char connected_phone_ip[MAX_IP_STR_LEN];

    bool bPlugged;           // 1: Plugged   0: UnPlugged
    bool bDeviceEnabled;     // 1: Enabled   0: Disabled
    bool bDeviceReady;       // 1: Ready 0: Not Ready
    bool bConnected;         // 1: Connect to a wifi AP; 0: Not connect to wifi ap
    bool bConnectedByPhone;  // 1: Connected by Phone; 0: Not connect by phone
    bool host_ap_ip_ready; //AP mode, station client get ip from device successful.
    bool sta_ip_ready; //station mode, device get ip from host clinet successful.
    int wifi_connecting;
    int wifi_scanning;
    int wifi_init_done;
    WIFI_MODE_e mode;
    char local_gw[MAX_IP_STR_LEN];
    bool bLimitedInternet;   // true: Limited Internet; false: Unlimited Internet
    bool bReconnect;
} wifi_config_t;

typedef struct _network_wifi_ap_info_t_
{
    hccast_wifi_ap_info_t *wifi_ap;
    int timeout;
} network_wifi_ap_info_t;

typedef void (*net_dowload_cb)(void *user_data, int length);

int network_init(void);
int network_deinit(void);
int network_connect(void);
void network_wifi_module_set(int wifi_module);
int network_wifi_module_get(void);
int hostap_get_connect_count(void);
char *app_wifi_local_ip_get(void);
char *app_wifi_connected_phone_ip_get(void);

void hccast_start_services(void);
void hccast_stop_services(void);
void hccast_service_need_start_flag(bool flag);

bool network_service_enable_get(void);
void network_service_enable_set(bool start);
wifi_config_t *app_wifi_config_get(void);
char *network_get_upgrade_url(void);
int app_wifi_reconnect(hccast_wifi_ap_info_t *wifi_ap, int timeout);
int app_wifi_switch_work_mode(WIFI_MODE_e wifi_mode);
char *app_get_connecting_ssid(void);
bool app_wifi_connect_status_get(void);
bool app_get_wifi_init_done(void);
void app_set_wifi_init_done(int state);
void app_wifi_set_limited_internet(bool limited);
bool app_wifi_is_limited_internet(void);
WIFI_MODE_e app_wifi_get_work_mode(void);

void *api_network_download_start(char *url, char *file_name, uint8_t *data, uint32_t size, \
        net_dowload_cb net_cb, void *user_data, bool block);
void api_network_download_pthread_stop(void *handle);
void api_network_download_flag_stop(void *handle);

int network_plugout_reboot(void);
int network_wifi_pm_plugin_handle(void);
int network_wifi_pm_open(void);
int network_wifi_pm_stop(void);
int network_get_airp2p_state(void);
void network_set_airp2p_state(int state);
int network_wifi_reset(void);
int network_get_current_state(void);
void network_set_current_state(int state);
int network_cur_wifi_quality_update(void);
int network_airp2p_stop(void);

int app_wifi_init();
int app_wifi_deinit();
// #define HOSTAP_CHANNEL_24G  6
// #define HOSTAP_CHANNEL_5G   36

#if defined(AUTO_HTTP_UPGRADE) || defined(MANUAL_HTTP_UPGRADE)
int network_upgrade_start();
bool network_upgrade_flag_get(void);
void network_upgrade_flag_set(bool en);
#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
#endif    

