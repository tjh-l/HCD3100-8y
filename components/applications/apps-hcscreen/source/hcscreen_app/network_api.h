/**
 * NETWORK_SUPPORT.h
 */

#ifndef __NETWORK_API_H__
#define __NETWORK_API_H__

#include <hccast/hccast_wifi_mgr.h>
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************
NETWORK_UPGRADE_URL:
config name: HCFOTA.jsonp
example:http://172.16.12.81:80/hccast/rtos/HC15A210/hcscreen/HCFOTA.jsonp
**********************************************************************/
#ifndef  CONFIG_HTTP_UPGRADE_URL
#define  CONFIG_HTTP_UPGRADE_URL "http://172.16.12.81:80"
#endif    

#ifdef __linux__
#define NETWORK_UPGRADE_URL (CONFIG_HTTP_UPGRADE_URL "/hccast/linux/%s/hcscreen/HCFOTA.jsonp")
#else
#define NETWORK_UPGRADE_URL (CONFIG_HTTP_UPGRADE_URL "/hccast/rtos/%s/hcscreen/HCFOTA.jsonp")
#endif

#define NETWORK_ETH0_IFACE_NAME     "eth0"
#define NETWORK_WIFI_IFACE_NAME     "wlan0"
#define MAX_IP_STR_LEN 32

typedef enum{
    WIFI_MODE_NONE,
    WIFI_MODE_STATION,
    WIFI_MODE_AP,
}WIFI_MODE_e;

typedef struct 
{
    char name[16];
    char desc[16];
    int  type;
} wifi_model_st;

typedef struct
{
    char local_ip[MAX_IP_STR_LEN];
    char connected_phone_ip[MAX_IP_STR_LEN];
    bool wifi_connected;         // 1: Connect to a wifi AP; 0: Not connect to wifi ap
    bool wifi_connecting;
    bool wifi_scanning;
    bool wifi_init_done;
    char local_gw[MAX_IP_STR_LEN];
    bool bLimitedInternet;   // true: Limited Internet; false: Unlimited Internet
    bool bReconnect;
    WIFI_MODE_e mode;
} wifi_config_t;

int network_init(void);
int network_deinit(void);
int network_connect(void);

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
void network_wifi_module_set(int wifi_module);
int network_wifi_module_get(void);
int hostap_get_connect_count(void);
char *wifi_local_ip_get(void);
char *wifi_connected_phone_ip_get(void);
#else // Ethernet
char *eth_local_ip_get(void);
#endif

#ifdef HOTSPOT_SUPPORT
bool network_hotspot_status_get(void);
int app_hotspot_start();
int app_hotspot_stop();
#endif

#endif // NETWORK_SUPPORT
void hccast_start_services(void);
void hccast_stop_services(void);
int app_get_wifi_connect_status(void);
void app_set_wifi_connect_status(int status);
int network_plugout_reboot(void);
void app_wifi_set_limited_internet(bool limited);
bool app_wifi_is_limited_internet(void);
int app_wifi_reconnect(hccast_wifi_ap_info_t *wifi_ap);

char *app_get_connecting_ssid(void);
wifi_config_t *app_wifi_config_get(void);
int app_wifi_switch_work_mode(WIFI_MODE_e wifi_mode);
WIFI_MODE_e app_wifi_get_work_mode(void);
int network_get_airp2p_state(void);
void network_set_airp2p_state(int state);
int network_get_current_state(void);
void network_set_current_state(int state);
int network_airp2p_start(void);
int network_airp2p_stop(void);

int app_wifi_init(void);
int app_wifi_deinit(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
