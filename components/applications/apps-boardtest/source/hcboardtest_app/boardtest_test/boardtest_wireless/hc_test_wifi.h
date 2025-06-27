
#ifndef __WIFI_TEST_H__
#define __WIFI_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "boardtest_module.h"

#define WIFI_CTRL_PATH_STA      "/var/run/wpa_supplicant"
#define WIFI_CTRL_IFACE_NAME    "wlan0"
#define P2P_CTRL_IFACE_NAME     "p2p0"
#define WIFI_MAC_PATH           "/sys/class/net/wlan0/address"


typedef struct _hc_test_wifi_status_result_t_
{
    char bssid[18];
    int freq;
    char ssid[32];
    int id;
    char mode[16];
    char pairwise_cipher[16];
    char group_cipher[16];
    char key_mgmt[16];
    char wpa_state[32];
    char ip_address[16];
    char address[18];
    char uuid[64];
    char p2p_device_address[18];
} hc_test_wifi_status_result_t;

typedef struct _hc_wifi_configuration_
{
    int get_results;
    char ssid[64];
    char pwd[64];
}wifi_config;

typedef struct _hc_test_wifi_result_t
{
    char ssid[32];
    char ipaddr[16];
    char wifi_state[32];
}test_result;

struct wpa_params{
	int argc;
	char **argv;
};


#ifdef BOARDTEST_NET_SUPPORT
static int hc_test_wifi_get_mac_addr(char *mac);
static int hc_test_wifi_module_get(void);
#endif


#ifdef BOARDTEST_WIFI_SUPPORT
static int hc_test_wifi_sup_init(void);
static void *wpa_supplicant_thread(void *args);
static int wpa_start_helper(struct wpa_params *params, void *(*entry) (void *));
static int hc_test_wifi_cli_init(void);
static int hc_test_wifi_set_network_ssid(char *ssid);
static int hc_test_wifi_set_network_psk(char *pwd);
static int hc_test_wifi_select_network(void);
static int result_get(char *str, char *key, char *val, int val_len);
static int wifi_ctrl_run_cmd(const char *ifname,char *cmd, char *result, size_t *len);
static int wifi_ctrl_get_status(const char *ifname, hc_test_wifi_status_result_t *result);
#endif

static void wpa_params_destroy(struct wpa_params *params);
static void hc_test_wifi_clear_space(char *remove,int remove_len);
static wifi_config hc_test_wifi_read_content(void);
static char *wifi_ctrl_chinese_conversion(char *srcStr);
static int castapp_system_cmd(const char *cmd);


static int hc_test_wifi_init(void);
static int hc_test_wifi_run(void);
static int hc_test_wifi_exit(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
