#ifndef _P2P_CTRL_H_
#define _P2P_CTRL_H_

#include <stdbool.h>
#include "hccast_wifi_mgr.h"

#define MIRA_RTSP_PORT (7236)

typedef int (*p2p_ctrl_state_callback)(int state);
typedef int (*p2p_ctrl_set_res_callback)(int state);

typedef struct _p2p_in_param_st
{
    char wifi_ifname[64];                            // wifi interface name
    char p2p_ifname[64];                             // p2p interface name
    char device_name[64];                            // p2p device name
    int  listen_channel;                             // p2p listen channel
    int  oper_channel;                               // p2p oper channel
    p2p_ctrl_state_callback state_update_func;       // p2p stat update func
    p2p_ctrl_set_res_callback res_set_func;          // p2p set resolution
} p2p_param_st;

typedef struct _p2p_peer_result_st_
{
    char device_name[64];
    char wfd_subelems[64];
    int config_methods;
    int dev_capab;
    int group_capab;
    int listen_freq;
} p2p_peer_result_st;


int p2p_ctrl_init(p2p_param_st *params);
int p2p_ctrl_uninit(void);

void p2p_ctrl_device_init(void);
int p2p_ctrl_device_is_go(void);
void p2p_ctrl_device_enable(void);
void p2p_ctrl_device_disable(void);

bool p2p_ctrl_get_enable(void);
bool p2p_ctrl_set_enable(bool enable);
unsigned int p2p_ctrl_get_device_ip(void);
int p2p_ctrl_get_device_rtsp_port(void);

int p2p_ctrl_get_connect_stat(void);
int p2p_ctrl_set_p2p_op_mode(int type);
int p2p_ctrl_set_connect_stat(bool stat);
int p2p_ctrl_set_channel_fuse(unsigned int bitmap);

void p2p_ctrl_device_abort_scan(void);

#ifdef HC_RTOS
/**
 * It sends a command to the driver to abort the p2p scan
 * note: unofficial cmd
 */
int p2p_ctrl_iwlist_scan_cmd(char *inf);
int p2p_ctrl_device_power_on(void);
int p2p_ctrl_device_power_off(void);

/*interface*/
int p2p_ctrl_nl_device_power_on(void);
int p2p_ctrl_nl_device_power_off(void);
int p2p_ctrl_nl_iwlist_scan_cmd(char *inf);

int p2p_ctrl_wext_device_power_on(void);
int p2p_ctrl_wext_device_power_off(void);
int p2p_ctrl_wext_iwlist_scan_cmd(char *inf);
#endif

/*interface*/
#define P2P_MAX_NETWORK_LIST (10)

#define BUFFER_LEN (1024)

typedef struct _p2p_network_st_
{
    char bssid[WIFI_MAX_SSID_LEN];
    char mac[WIFI_MAX_SSID_LEN];
    char ssid[WIFI_MAX_SSID_LEN];
    char psk[WIFI_MAX_SSID_LEN];
    char pairwise[WIFI_MAX_PWD_LEN];
    unsigned long last_time;
} p2p_network_st;

typedef struct _p2p_network_list_st_
{
    p2p_network_st network[P2P_MAX_NETWORK_LIST];
} p2p_network_list_st;

int p2p_ctrl_send_cmd(char *cmd);
int p2p_ctrl_p2p_remove_group(void);
int p2p_ctrl_p2p_store_network(p2p_network_list_st *list);
int p2p_ctrl_get_peer_ifa(char *ssid, size_t ssid_len, char *bssid, size_t bssid_len, char *mac, size_t mac_len);

int p2p_ctrl_nl_uninit(void);
int p2p_ctrl_nl_device_is_go(void);
int p2p_ctrl_nl_get_connect_stat(void);
int p2p_ctrl_nl_init(p2p_param_st *params);
int p2p_ctrl_nl_set_connect_stat(bool stat);
int p2p_ctrl_nl_set_channel_fuse(unsigned int bitmap);
void p2p_ctrl_nl_device_init(void);
void p2p_ctrl_nl_device_enable(void);
void p2p_ctrl_nl_wpas_attr_init(void);
void p2p_ctrl_nl_device_disable(void);
void p2p_ctrl_nl_device_abort_scan(void);
bool p2p_ctrl_nl_get_enable(void);
bool p2p_ctrl_nl_set_enable(bool enable);
unsigned int p2p_ctrl_nl_get_device_ip(void);

int p2p_ctrl_wext_uninit(void);
int p2p_ctrl_wext_device_is_go(void);
int p2p_ctrl_wext_get_connect_stat(void);
int p2p_ctrl_wext_init(p2p_param_st *params);
int p2p_ctrl_wext_set_connect_stat(bool stat);
int p2p_ctrl_wext_set_channel_fuse(unsigned int bitmap);
void p2p_ctrl_wext_device_init(void);
void p2p_ctrl_wext_device_enable(void);
void p2p_ctrl_wext_wpas_attr_init(void);
void p2p_ctrl_wext_device_disable(void);
void p2p_ctrl_wext_device_abort_scan(void);
bool p2p_ctrl_wext_get_enable(void);
bool p2p_ctrl_wext_set_enable(bool enable);
unsigned int p2p_ctrl_wext_get_device_ip(void);

#endif