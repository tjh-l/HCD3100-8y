#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <wpa_ctrl.h>
#include <iwlib.h>

#ifdef __linux__
#include <sys/prctl.h>
#endif

#include "sys/unistd.h"
#include "hccast_log.h"
#include "hccast_dhcpd.h"
#include "hccast_p2p_ctrl.h"
#include "hccast_wifi_ctrl.h"
#include "hccast_wifi_mgr.h"
#include "hccast_net.h"

typedef enum _hccast_wifi_p2p_op_mode_e_
{
    HCCAST_P2P_OP_NONE = 0,
    HCCAST_P2P_OP_WEXT,
    HCCAST_P2P_OP_NL,
} hccast_wifi_p2p_op_mode_e;

static struct wpa_ctrl *g_p2p_ifrecv = NULL;
static pthread_t g_p2p_tid = 0;
static pthread_mutex_t g_p2p_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_p2p_thread_running = false;
static bool g_p2p_device_is_go = false;
static bool g_p2p_enable = false;
struct in_addr g_ip_addr;
static p2p_param_st g_mira_param;
static int64_t timestamp_listen = 0;
static bool g_p2p_connect = false;
static hccast_wifi_p2p_op_mode_e g_p2p_op_mode = HCCAST_P2P_OP_NONE;

/* ***************************************** */

int p2p_ctrl_set_p2p_op_mode(int type)
{
    switch(type)
    {
#ifdef HC_RTOS
        case HCCAST_NET_WIFI_ECR6600U:
            g_p2p_op_mode = HCCAST_P2P_OP_NL;
            break;
        case HCCAST_NET_WIFI_8733BU:
        case HCCAST_NET_WIFI_8188FTV:
            g_p2p_op_mode = HCCAST_P2P_OP_WEXT;
            break;
        case HCCAST_NET_WIFI_NONE:
            g_p2p_op_mode = HCCAST_P2P_OP_NONE;
            break;
        default:
            g_p2p_op_mode = HCCAST_P2P_OP_WEXT;
            break;
#else
        case HCCAST_NET_WIFI_8800D:
        case HCCAST_NET_WIFI_8733BU:
        case HCCAST_NET_WIFI_8811FTV:
        case HCCAST_NET_WIFI_8188FTV:
        case HCCAST_NET_WIFI_ECR6600U:
            g_p2p_op_mode = HCCAST_P2P_OP_NL;
            break;
        case HCCAST_NET_WIFI_NONE:
            g_p2p_op_mode = HCCAST_P2P_OP_NONE;
            break;
        default:
            g_p2p_op_mode = HCCAST_P2P_OP_NL;
            break;
#endif
    }

    return g_p2p_op_mode;
}

int p2p_ctrl_get_connect_stat(void)
{
    int ret = -1;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_get_connect_stat();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_get_connect_stat();
    }

    return ret;
}

int p2p_ctrl_set_connect_stat(bool stat)
{
    int ret = -1;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_set_connect_stat(stat);
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_set_connect_stat(stat);
    }

    return ret;
}

int p2p_ctrl_send_cmd(char *cmd)
{
    int ret = 0;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        ret = -2;
        hccast_log(LL_ERROR, "%s run error (%s)!\n", cmd, reply);
    }

    return ret;
}

void p2p_ctrl_device_init()
{
    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        p2p_ctrl_wext_device_init();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        p2p_ctrl_nl_device_init();
    }

    return;
}

int p2p_ctrl_p2p_remove_group(void)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    char *cmd = "P2P_GROUP_REMOVE p2p0";

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
    }

    return ret;
}

static int p2p_ctrl_is_persistent_current_network(char *ssid)
{
    bool ret = false;

    hccast_wifi_list_net_result_t *res = NULL;
    res = (hccast_wifi_list_net_result_t *)calloc(1, sizeof(hccast_wifi_list_net_result_t));
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return ret;
    }

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    wifi_ctrl_get_list_net(ifname, res);
    for (int i = 0; i < res->net_num; i++)
    {
        if (strstr(res->netinfo[i].ssid, ssid))
        {
            if (strstr(res->netinfo[i].flags, "[P2P-PERSISTENT]"))
            {
                ret = true;
                break;
            }
        }
    }

    free(res);
    return ret;
}

static int p2p_ctrl_get_current_bssid(char *bssid, unsigned int len)
{
    char found = 0;
    int ret = HCCAST_WIFI_ERR_NO_ERROR;

    if (!bssid || len == 0)
    {
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    hccast_wifi_status_result_t *res = NULL;
    res = (hccast_wifi_status_result_t *)calloc(1, sizeof(hccast_wifi_status_result_t));
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return HCCAST_WIFI_ERR_MEM;
    }

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    ret = wifi_ctrl_get_status(ifname, HCCAST_WIFI_MODE_STA, res);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "wifi_ctrl_get_status failed!\n");
        free(res);
        return ret;
    }

    strncpy(bssid, res->bssid, strlen(res->bssid) > len ? len : strlen(res->bssid));

    free(res);
    return ret;
}

static int p2p_ctrl_get_current_ssid(char *ssid, unsigned int len)
{
    char found = 0;
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    unsigned int real_len = 0;

    if (!ssid || len == 0)
    {
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    hccast_wifi_status_result_t *res = NULL;
    res = (hccast_wifi_status_result_t *)calloc(1, sizeof(hccast_wifi_status_result_t));
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return HCCAST_WIFI_ERR_MEM;
    }

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    ret = wifi_ctrl_get_status(ifname, HCCAST_WIFI_MODE_STA, res);
    if (ret < 0)
    {
        hccast_log(LL_ERROR, "wifi_ctrl_get_status failed!\n");
        free(res);
        return ret;
    }

    real_len = strlen(res->ssid) > (len - 1) ? (len - 1) : strlen(res->ssid);
    strncpy(ssid, res->ssid, real_len);
    ssid[real_len + 1] = '\0';

    free(res);
    return ret;
}

/**
 * It gets the current network  of the WiFi interface
 *
 * @param ifname the interface name, such as wlan0, p2p0, etc.
 * @param mode the mode of the interface, STA or AP
 * @param id the network id
 * @param psk the psk of the query
 */
static int p2p_ctrl_get_network_psk(char id, char *psk, unsigned int len)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char val[512] = {0};
    char reply[1024] = {0};
    char cmd[256] = {0};

    snprintf(cmd, sizeof(cmd), "GET_NETWORK %d psk", id);

    if (NULL == psk || len <= 0)
    {
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        hccast_log(LL_ERROR, "param error!\n");
        goto ERROR;
    }

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        goto ERROR;
    }

    if (strncmp(reply, "FAIL", 4) == 0)
    {
        ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        hccast_log(LL_ERROR, "%s (FAIL)\n", cmd);
        goto ERROR;
    }

    memcpy(psk, reply, len);
	hccast_log(LL_DEBUG, "psk == %s\n", psk);
ERROR:
    return ret;
}

static int p2p_ctrl_get_network_pairwise(int id, char *pairwise, unsigned int len)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char val[512] = {0};
    char reply[1024] = {0};
    char cmd[256] = {0};

    snprintf(cmd, sizeof(cmd), "GET_NETWORK %d pairwise", id);

    if (NULL == pairwise || len <= 0)
    {
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        hccast_log(LL_ERROR, "param error!\n");
        goto ERROR;
    }

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
        goto ERROR;
    }

    if (strncmp(reply, "FAIL", 4) == 0)
    {
        ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        hccast_log(LL_ERROR, "%s (FAIL)\n", cmd);
        goto ERROR;
    }

    hccast_log(LL_NOTICE, "pairwise == %s\n", reply);
    memcpy(pairwise, reply, len);

ERROR:
    return ret;
}

static int p2p_ctrl_get_current_network_id_bssid(char *ssid, char *bssid)
{
    int id = -1;
    int i  = 0;
    hccast_wifi_list_net_result_t *res = NULL;
    res = (hccast_wifi_list_net_result_t *)calloc(1, sizeof(hccast_wifi_list_net_result_t));
    if (res == NULL)
    {
        hccast_log(LL_ERROR, "%s %d calloc fail\n", __func__, __LINE__);
        return id;
    }

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    wifi_ctrl_get_list_net(ifname, res);
    for (i = 0; i < res->net_num; i++)
    {
        if (strstr(res->netinfo[i].flags, "[CURRENT]"))
        {
            continue;
        }
        if (strstr(res->netinfo[i].ssid, ssid))
        {
            id = strtol(res->netinfo[i].net_id, NULL, 10);
            break;
        }
    }

    if (i < res->net_num) 
    {
        memcpy(bssid, res->netinfo[i].bssid, WIFI_MAX_SSID_LEN);
        free(res);
        return id;
    }
    
    for (i = 0; i < res->net_num; i++)
    {
        if (strstr(res->netinfo[i].flags, "[CURRENT]"))
        {
            id = strtol(res->netinfo[i].net_id, NULL, 10);
            break;
        }
    }

    memcpy(bssid, res->netinfo[i].bssid, WIFI_MAX_SSID_LEN);
    free(res);
    return id;
}

int p2p_ctrl_p2p_store_network(p2p_network_list_st *list)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char ssid[WIFI_MAX_SSID_LEN] = {0};

    if (!list)
    {
        hccast_log(LL_ERROR, "%s %d param error!\n", __func__, __LINE__);
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    p2p_ctrl_get_current_ssid(ssid, sizeof(ssid));
    bool isPersistent = p2p_ctrl_is_persistent_current_network(ssid);
    if (isPersistent)
    {
        char bssid[WIFI_MAX_SSID_LEN] = {0};
        char mac[WIFI_MAX_SSID_LEN] = {0};
        char psk[WIFI_MAX_SSID_LEN] = {0};
        char pairwise[WIFI_MAX_PWD_LEN] = {0};

        p2p_ctrl_get_peer_ifa(NULL, 0, NULL, 0, mac, sizeof(mac) - 1);
        p2p_ctrl_get_current_bssid(bssid, sizeof(bssid));

        int id = p2p_ctrl_get_current_network_id_bssid(ssid, bssid);
        if (id >= 0)
        {
            p2p_ctrl_get_network_psk(id, psk, sizeof(psk));
            p2p_ctrl_get_network_pairwise(id, pairwise, sizeof(pairwise));
        }
        else
        {
            hccast_log(LL_ERROR, "[%s %d]: get network id failed!\n", __func__, __LINE__);
            return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
        }

        bool found = false;
        int fill_id = -1;
        unsigned long last_time = time(NULL);

        for (int i = P2P_MAX_NETWORK_LIST - 1; i >= 0; i--)
        {
            if (!memcmp(list->network[i].ssid, ssid, sizeof(list->network[i].ssid)))
            {
                found = true;
                list->network[i].last_time = time(NULL);
                break;
            }

            if (list->network[i].last_time <= last_time)
            {
                fill_id = i;
                last_time = list->network[i].last_time;
            }
        }

        if (!found && fill_id >= 0)
        {
            memcpy(list->network[fill_id].bssid, bssid, sizeof(list->network[fill_id].bssid));
            memcpy(list->network[fill_id].ssid, ssid, sizeof(list->network[fill_id].ssid));
            memcpy(list->network[fill_id].psk, psk, sizeof(list->network[fill_id].psk));
            memcpy(list->network[fill_id].pairwise, pairwise, sizeof(list->network[fill_id].pairwise));
            memcpy(list->network[fill_id].mac, mac, sizeof(list->network[fill_id].mac));
            list->network[fill_id].last_time = time(NULL);
        }
    }

    return ret;
}

/**
 * It initializes the P2P wpas attr interface
 */
void p2p_ctrl_wpas_attr_init()
{
    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        p2p_ctrl_wext_wpas_attr_init();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        p2p_ctrl_nl_wpas_attr_init();
    }

    return;
}

void p2p_ctrl_device_enable()
{
    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        p2p_ctrl_wext_device_enable();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        p2p_ctrl_nl_device_enable();
    }

    return;
}

void p2p_ctrl_device_disable()
{
    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        p2p_ctrl_wext_device_disable();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        p2p_ctrl_nl_device_disable();
    }

    return;
}

#ifdef HC_RTOS // hcrtos

int p2p_ctrl_iwlist_scan_cmd(char *inf)
{
    int ret = -1;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_iwlist_scan_cmd(inf);
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_iwlist_scan_cmd(inf);
    }

    return ret;
}

int p2p_ctrl_device_power_on(void)
{
    int ret = -1;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_device_power_on();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_device_power_on();
    }

    return ret;
}

int p2p_ctrl_device_power_off(void)
{
    int ret = -1;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_device_power_off();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_device_power_off();
    }

    return ret;
}

#endif

void p2p_ctrl_device_abort_scan(void)
{
    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        p2p_ctrl_wext_device_abort_scan();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        p2p_ctrl_nl_device_abort_scan();
    }

    return;
}

/**
 * get p2p device enable status
 *
 * @return g_p2p_enable
 */
bool p2p_ctrl_get_enable(void)
{
    bool ret = 0;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_get_enable();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_get_enable();
    }

    return ret;
}

/**
 * It sets the global variable g_p2p_enable to the value of the parameter enable
 *
 * @param enable true to enable, false to disable
 *
 * @return The value of g_p2p_enable
 */
bool p2p_ctrl_set_enable(bool enable)
{
    bool ret = 0;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_set_enable(enable);
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_set_enable(enable);
    }

    return ret;
}

int p2p_ctrl_device_is_go()
{
    int ret = -1;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_device_is_go();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_device_is_go();
    }

    return ret;
}

unsigned int p2p_ctrl_get_device_ip()
{
    unsigned int ret = 0;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_get_device_ip();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_get_device_ip();
    }

    return ret;
}

int p2p_ctrl_get_device_rtsp_port()
{
    return MIRA_RTSP_PORT;
}

int p2p_ctrl_set_channel_fuse(unsigned int bitmap)
{
    int ret = -1;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_set_channel_fuse(bitmap);
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_set_channel_fuse(bitmap);
    }

    return ret;
}

/**
 * It creates a thread that runs the p2p_ctrl_thread function
 *
 * @param params: p2p device params
 *
 * @return 0: success, <0: failed.
 */
int p2p_ctrl_init(p2p_param_st *params)
{
    int ret = -1;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_init(params);
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_init(params);
    }

    return ret;
}

/**
 *  This function is used to stop the p2p thread
 */
int p2p_ctrl_uninit(void)
{
    int ret = -1;

    if (g_p2p_op_mode == HCCAST_P2P_OP_WEXT)
    {
        ret = p2p_ctrl_wext_uninit();
    }
    else if (g_p2p_op_mode == HCCAST_P2P_OP_NL)
    {
        ret = p2p_ctrl_nl_uninit();
    }

    return ret;
}