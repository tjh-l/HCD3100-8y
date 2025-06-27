/**
 * NETWORK_SUPPORT.h
 */

#ifndef __NETWORK_SUPPORT_H__
#define __NETWORK_SUPPORT_H__

#include <hccast/hccast_wifi_mgr.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_IP_STR_LEN 32

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
#endif // NETWORK_SUPPORT
void hccast_start_services(void);
void hccast_stop_services(void);
bool network_service_enable_get(void);
void network_service_enable_set(bool start);
int app_get_wifi_connect_status(void);
void app_set_wifi_connect_status(int status);
bool app_get_wifi_init_done(void);
void app_set_wifi_init_done(int state);
char *app_get_connecting_ssid();
wifi_config_t *app_wifi_config_get(void);
int app_wifi_reconnect(hccast_wifi_ap_info_t *wifi_ap);
int network_disconnect(void);
int network_plugout_reboot(void);
void app_wifi_set_limited_internet(bool limited);
bool app_wifi_is_limited_internet(void);
int app_wifi_switch_work_mode(WIFI_MODE_e wifi_mode);
int network_disable_net_state(void);
int network_enable_net_state(void);
void network_start_services(void);
void network_stop_services(void);

//#define HOSTAP_CHANNEL_24G  6     //ref data_mgr.h
//#define HOSTAP_CHANNEL_5G   36    // ref data_mgr.h

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif    

