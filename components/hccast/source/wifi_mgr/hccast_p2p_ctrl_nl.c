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

// P2P: Starting Listen timeout(5,0) on freq=2437 based on callback  ----- timeout(pending_listen_sec, pending_listen_usec)
// P2P: Set timeout (state=LISTEN_ONLY): 5.020000 sec  ---- Add 20 msec extra wait to avoid race condition
#define P2P_LISTEN_TIMEOUT (300000000LL)
#define max_args 10

#ifdef __linux__
#define USE_WPAS_P2P_CONF_FILE
#endif

static p2p_network_list_st g_p2p_network_list = {0};

/* ***************************************** */

int p2p_ctrl_nl_get_connect_stat(void)
{
    return g_p2p_connect;
}

int p2p_ctrl_nl_set_connect_stat(bool stat)
{
    return g_p2p_connect = !!stat;
}

static int64_t get_time_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000LL + tv.tv_usec;
}

static int tokenize_cmd(char *cmd, char *argv[])
{
    char *pos = NULL;
    int argc = 0;

    pos = cmd;
    for (;;)
    {
        while (*pos == ' ')
            pos++;
        if (*pos == '\0')
            break;
        argv[argc] = pos;
        argc++;
        if (argc == max_args)
            break;
        if (*pos == '"')
        {
            char *pos2 = strrchr(pos, '"');
            if (pos2)
                pos = pos2 + 1;
        }
        while (*pos != '\0' && *pos != ' ')
            pos++;
        if (*pos == ' ')
            *pos++ = '\0';
    }

    return argc;
}

static int result_get(char *str, char *key, char *val, int val_len)
{
    if (NULL == str || NULL == key || NULL == val)
    {
        hccast_log(LL_ERROR, "param error!\n");
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    hccast_log(LL_DEBUG, "query key = %s\n", key);
    char keys[64] = {0};
    char vals[256] = {0};
    char *token;
    char *saveptr;

    char *strs = strdup(str);
    token = strtok_r(strs, " ", &saveptr);

    while (token != NULL)
    {
        sscanf(token, "%[^=]=%[^\n]", keys, vals);

        if (!strcmp(key, keys))
        {
            //printf("keys: %s, vals: %s.\n", keys, vals);
            strncpy(val, vals, val_len);
            break;
        }

        token = strtok_r(NULL, " ", &saveptr);
    }

    free(strs);

    return HCCAST_WIFI_ERR_NO_ERROR;
}

/*
> p2p_peer 9a:ac:cc:96:2d:6b
9a:ac:cc:96:2d:6b
pri_dev_type=10-0050F204-5
device_name=Android_3db5
manufacturer=
model_name=
model_number=
serial_number=
config_methods=0x80
dev_capab=0x25
group_capab=0x0
level=0
age=1
listen_freq=2437
wps_method=not-ready
interface_addr=00:00:00:00:00:00
member_in_go_dev=00:00:00:00:00:00
member_in_go_iface=00:00:00:00:00:00
go_neg_req_sent=0
go_state=unknown
dialog_token=0
intended_addr=00:00:00:00:00:00
country=__
oper_freq=0
req_config_methods=0x0
flags=[REPORTED][PEER_WAITING_RESPONSE]
status=1
invitation_reqs=0
wfd_subelems=00000600101c440032
*/
static int p2p_ctrl_peer(const char *peer_dev, p2p_peer_result_st *result)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char cmd[64] = {0};
    char val[512] = {0};
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;

    if (NULL == result || NULL == peer_dev)
    {
        hccast_log(LL_ERROR, "param error!\n");
        ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
        goto ERROR;
    }

    sprintf(cmd, "P2P_PEER %s", peer_dev);
    memset(result, 0x00, sizeof(p2p_peer_result_st));

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

    result_get(reply, "device_name", result->device_name, sizeof(result->device_name));
    result_get(reply, "wfd_subelems", result->wfd_subelems, sizeof(result->wfd_subelems));

    if (result_get(reply, "config_methods", val, sizeof(val)) == 0)
    {
        result->config_methods = strtol(val, NULL, 16);
    }
    if (result_get(reply, "dev_capab", val, sizeof(val)) == 0)
    {
        result->dev_capab = strtol(val, NULL, 16);
    }
    if (result_get(reply, "group_capab", val, sizeof(val)) == 0)
    {
        result->group_capab = strtol(val, NULL, 16);
    }
    if (result_get(reply, "listen_freq", val, sizeof(val)) == 0)
    {
        result->listen_freq = strtol(val, NULL, 16);
    }

ERROR:
    return ret;
}

void p2p_ctrl_nl_device_init()
{
    hccast_log(LL_NOTICE, "%s p2p device init\n", __func__);

    p2p_ctrl_send_cmd("DISCONNECT");
    p2p_ctrl_send_cmd("P2P_FLUSH");
    //p2p_ctrl_send_cmd("P2P_FIND 1");
    //usleep(1000);

    timestamp_listen = get_time_us();
    p2p_ctrl_send_cmd("P2P_LISTEN");
    g_mira_param.state_update_func(HCCAST_P2P_STATE_LISTEN);
}

/**
 * It opens a connection to the wpa_supplicant control interface, and returns the file descriptor for
 * the connection
 *
 * @return The file descriptor of the control interface for recv.
 */
static int p2p_ctrl_nl_wpas_init(void)
{
    hccast_log(LL_NOTICE, "Enter %s!\n", __func__);

    char ctrl_iface[128];
    int err = HCCAST_WIFI_ERR_NO_ERROR;

    if (g_p2p_ifrecv)
    {
        return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
    }

    snprintf(ctrl_iface, sizeof(ctrl_iface), "%s/%s", WIFI_CTRL_PATH_STA, (char *)g_mira_param.p2p_ifname);

#ifdef __linux__
    if (access(ctrl_iface, F_OK)) // no exist
    {
        hccast_log(LL_ERROR, "ctrl_iface: %s non-existent!\n", ctrl_iface);
        return HCCAST_WIFI_ERR_CMD_WPAS_NO_RUN;
    }
#endif

    hccast_log(LL_INFO, "ctrl_iface: %s\n", ctrl_iface);

#ifdef __linux__
    g_p2p_ifrecv = wpa_ctrl_open(ctrl_iface);
#else
    g_p2p_ifrecv = wpa_ctrl_open(ctrl_iface, WPA_CTRL_P2P_IFACE_PORT);
#endif

    if (!g_p2p_ifrecv)
    {
        hccast_log(LL_ERROR, "g_p2p_ifrecv open error!\n");
        return HCCAST_WIFI_ERR_CMD_WPAS_OPEN_FAILED;
    }

    err = wpa_ctrl_attach(g_p2p_ifrecv);
    if (err)
    {
        hccast_log(LL_ERROR, "g_p2p_ifrecv attach error!\n");
        wpa_ctrl_close(g_p2p_ifrecv);
        g_p2p_ifrecv = NULL;
        return HCCAST_WIFI_ERR_CMD_WPAS_ATTACH_FAILED;
    }

    return wpa_ctrl_get_fd(g_p2p_ifrecv);
}

/**
 * It initializes the P2P wpas attr interface
 */
void p2p_ctrl_nl_wpas_attr_init(void)
{
    char aSend[256] = {0};
    p2p_ctrl_p2p_remove_group();
    p2p_ctrl_send_cmd("DISCONNECT"); // wifi wpa_cli -x 9890 -i p2p0 disconnect
    p2p_ctrl_send_cmd("REMOVE_NETWORK all"); // wifi wpa_cli -x 9890 -i p2p0 remove_network all

#ifdef USE_WPAS_P2P_CONF_FILE
    p2p_ctrl_send_cmd("RECONFIGURE");
#endif

    sprintf(aSend, "SET device_name %s", g_mira_param.device_name);
    p2p_ctrl_send_cmd(aSend);

#ifdef USE_WPAS_P2P_CONF_FILE
    p2p_ctrl_send_cmd("SET update_config 1");
#endif

    /*if (HCCAST_NET_WIFI_8800D == hccast_wifi_mgr_get_wifi_model())*/
    {
        p2p_ctrl_send_cmd("SET wifi_display 1");
        p2p_ctrl_send_cmd("SET bssid_filter ");
        p2p_ctrl_send_cmd("SET p2p_no_group_iface 1");
        //p2p_ctrl_send_cmd("WFD_SUBELEM_SET 0 000600111c440006");
        p2p_ctrl_send_cmd("WFD_SUBELEM_SET 7 00020001");
        p2p_ctrl_send_cmd("WFD_SUBELEM_SET 0 00060151022a012c");
        p2p_ctrl_send_cmd("WFD_SUBELEM_SET 1 0006000000000000");
        p2p_ctrl_send_cmd("WFD_SUBELEM_SET 6 000700000000000000");
    }

    p2p_ctrl_send_cmd("SET device_type 7-0050F204-1");
    p2p_ctrl_send_cmd("SET config_methods push_button");
    p2p_ctrl_send_cmd("SET manufacturer hichip");
    p2p_ctrl_send_cmd("SET persistent_reconnect 1");
    p2p_ctrl_send_cmd("SET p2p_listen_reg_class 81");
    p2p_ctrl_send_cmd("SET p2p_oper_reg_class 81");
    p2p_ctrl_send_cmd("SET p2p_go_intent 7");
    p2p_ctrl_send_cmd("WFD_SUBELEM_SET 0 00060051022a012c");
    p2p_ctrl_send_cmd("SET wifi_display 1");
    //p2p_ctrl_send_cmd("SET p2p_go_vht 1");
    //p2p_ctrl_send_cmd("SET p2p_go_ht40 1");

    hccast_wifi_status_result_t status_result = {0};

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    wifi_ctrl_get_status(ifname, HCCAST_WIFI_MODE_STA, &status_result);
    sprintf(aSend, "SET p2p_ssid_postfix %s%c%c%c%c", "_hccast_", \
            status_result.p2p_device_address[12], status_result.p2p_device_address[13], \
            status_result.p2p_device_address[15], status_result.p2p_device_address[16]);

    p2p_ctrl_send_cmd(aSend);

    if (HCCAST_NET_WIFI_ECR6600U == hccast_wifi_mgr_get_wifi_model())
    {
        printf("[%s][%d]listen_channel=%d\n",__FUNCTION__,__LINE__,g_mira_param.listen_channel);
        sprintf(aSend, "P2P_SET listen_channel %d", g_mira_param.listen_channel);
    }
    else
    {
        sprintf(aSend, "SET p2p_listen_channel %d", g_mira_param.listen_channel);
    }

    p2p_ctrl_send_cmd(aSend);

    // sprintf(aSend, "SET p2p_oper_channel %d", g_mira_param.oper_channel);
    // p2p_ctrl_send_cmd(aSend);
}

void p2p_ctrl_nl_device_enable(void)
{
    p2p_ctrl_send_cmd("DRIVER wfd-enable");
    p2p_ctrl_send_cmd("SET wifi_display 1");
    usleep(200 * 1000);

    p2p_ctrl_send_cmd("P2P_SET disabled 0");
    p2p_ctrl_send_cmd("P2P_FLUSH");
    p2p_ctrl_send_cmd("P2P_LISTEN");
    g_mira_param.state_update_func(HCCAST_P2P_STATE_LISTEN);
}

void p2p_ctrl_nl_device_disable(void)
{
    g_p2p_device_is_go = false;
    p2p_ctrl_send_cmd("DISCONNECT"); // wifi wpa_cli -x 9890 -i p2p0 disconnect
    usleep(1000);
    p2p_ctrl_send_cmd("DRIVER wfd-disable");
    p2p_ctrl_send_cmd("SET wifi_display 0");
    usleep(200 * 1000);
    p2p_ctrl_send_cmd("P2P_SET disabled 1");
    // p2p_ctrl_send_cmd("P2P_FLUSH"); // in "P2P_SET disabled 1"
    g_mira_param.state_update_func(HCCAST_P2P_STATE_NONE);
}

#ifdef HC_RTOS // hcrtos

int p2p_ctrl_nl_iwlist_scan_cmd(char *inf)
{
    int ret = -1;
    return ret;
}

int p2p_ctrl_nl_device_power_on(void)
{
    int ret = -1;
    return ret;
}

int p2p_ctrl_nl_device_power_off(void)
{
    int ret = -1;
    return ret;
}

#endif

void p2p_ctrl_nl_device_abort_scan(void)
{
    int ret = 0;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    char *cmd = "ABORT_SCAN";
    char ifname[32] = {0};

    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        ret = -2;
    }

    return;
}

/**
 * get p2p device enable status
 *
 * @return g_p2p_enable
 */
bool p2p_ctrl_nl_get_enable(void)
{
    return g_p2p_enable;
}

static int p2p_ctrl_p2p_remove_all(void)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    char cmd[256] = {0};

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    sprintf(cmd, "%s", "DISCONNECT");
    ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
    }

    sprintf(cmd, "%s", "REMOVE_NETWORK all");
    ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        hccast_log(LL_ERROR, "%s run error!\n", cmd);
    }

    return ret;
}

static int p2p_ctrl_nl_p2p_add_network(bool isAll, p2p_network_list_st *list, p2p_network_st *network)
{
    int ret = HCCAST_WIFI_ERR_NO_ERROR;
    char reply[1024] = {0};
    unsigned int len = sizeof(reply) - 1;
    char cmd[256] = {0};
    int net_id = -1, i = 0;
    unsigned char pw[32] = {0};

    if (!list && isAll)
    {
        hccast_log(LL_ERROR, "param error!\n");
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }
    else if (!list && !network && !isAll)
    {
        hccast_log(LL_ERROR, "param error!\n");
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    if (isAll)
    {
        for (int i = 0; i < P2P_MAX_NETWORK_LIST; i++)
        {
            if (strlen(list->network[i].bssid) <= 0)
            {
                continue;
            }

            sprintf(cmd, "%s", "ADD_NETWORK");
            ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
                goto EXIT;
            }

            net_id = atoi(reply);

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d ssid \"%s\"", net_id, list->network[i].ssid);
            ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d bssid %s", net_id, list->network[i].bssid);
            ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            char psk[96] = {0};
            memcpy(psk, list->network[i].psk, sizeof(list->network[i].psk));
            sprintf(cmd, "SET_NETWORK %d psk %s", net_id, psk);
            ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d pairwise %s", net_id, list->network[i].pairwise);
            ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d disabled %d", net_id, 2);
            ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d pbss %d", net_id, 0);
            ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
                goto EXIT;
            }

            if (!strcmp(reply, "ok"))
            {
                ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
                hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d key_mgmt WPA-PSK", net_id);
            ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
                goto EXIT;
            }

            memset(reply, 0, sizeof(reply));
            len = sizeof(reply) - 1;

            sprintf(cmd, "SET_NETWORK %d auth_alg OPEN", net_id);
            ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
            if (ret < 0 || len == 0)
            {
                hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
                goto EXIT;
            }
        }
    }
    else
    {
        if (strlen(network->bssid) <= 0)
        {
            ret = HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
            goto EXIT;
        }

        sprintf(cmd, "%s", "ADD_NETWORK");
        ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
            goto EXIT;
        }

        net_id = atoi(reply);

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d ssid \"%s\"", net_id, network->ssid);
        ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
            goto EXIT;
        }

        if (!strcmp(reply, "ok"))
        {
            ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;

        char psk[96] = {0};
        memcpy(psk, list->network[i].psk, sizeof(network->psk));

        sprintf(cmd, "SET_NETWORK %d psk %s", net_id, psk);
        ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
            goto EXIT;
        }

        if (!strcmp(reply, "ok"))
        {
            ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d pairwise %s", net_id, network->pairwise);
        ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
            goto EXIT;
        }

        if (!strcmp(reply, "ok"))
        {
            ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d disabled %d", net_id, 2);
        ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
            goto EXIT;
        }

        if (!strcmp(reply, "ok"))
        {
            ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d pbss %d", net_id, 0);
        ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s %d: %s, ret = %d\n", __func__, __LINE__, cmd, ret);
            goto EXIT;
        }

        if (!strcmp(reply, "ok"))
        {
            ret = HCCAST_WIFI_ERR_CMD_RUN_FAILED;
            hccast_log(LL_ERROR, "%s %d: %s\n", __func__, __LINE__, reply);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d key_mgmt WPA-PSK", net_id);
        ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
            goto EXIT;
        }

        memset(reply, 0, sizeof(reply));
        len = sizeof(reply) - 1;
        sprintf(cmd, "SET_NETWORK %d auth_alg OPEN", net_id);
        ret = wifi_ctrl_run_cmd(ifname, HCCAST_WIFI_MODE_STA, cmd, reply, &len);
        if (ret < 0 || len == 0)
        {
            hccast_log(LL_ERROR, "%s, ret = %d\n", cmd, ret);
            goto EXIT;
        }
    }

EXIT:
    return ret;
}

/**
 * It sets the global variable g_p2p_enable to the value of the parameter enable
 *
 * @param enable true to enable, false to disable
 *
 * @return The value of g_p2p_enable
 */
bool p2p_ctrl_nl_set_enable(bool enable)
{
    g_p2p_enable = enable;
    if (enable)
    {
        hccast_log(LL_NOTICE, "%s: p2p device enable.\n", __FUNCTION__);
        p2p_ctrl_nl_device_enable();
#ifdef HC_RTOS
        if (HCCAST_NET_WIFI_ECR6600U == hccast_wifi_mgr_get_wifi_model())
        {
            p2p_ctrl_nl_p2p_add_network(true, &g_p2p_network_list, NULL);
        }
#endif
    }
    else
    {
        hccast_log(LL_NOTICE, "%s: p2p device disenable.\n", __FUNCTION__);
#ifdef HC_RTOS
        if (HCCAST_NET_WIFI_ECR6600U == hccast_wifi_mgr_get_wifi_model())
        {
            p2p_ctrl_p2p_remove_group();
            p2p_ctrl_p2p_remove_all();
        }
#endif
        p2p_ctrl_nl_device_disable();
    }

    return g_p2p_enable;
}

int p2p_ctrl_nl_device_is_go(void)
{
    return g_p2p_device_is_go;
}

unsigned int p2p_ctrl_nl_get_device_ip(void)
{
    if (g_ip_addr.s_addr)
    {
        return g_ip_addr.s_addr;
    }

    return 0;
}

int p2p_ctrl_nl_set_channel_fuse(unsigned int bitmap)
{
    char cmd[128] = {0};
    int len_freq = 0;
    unsigned int len = 0;
    len = snprintf(cmd, sizeof(cmd), "%s", "P2P_SET disallow_freq ");

    for (int i = 0; i < 13; i++)
    {
        if ((bitmap >> i) & 1)
        {
            int frequency = 2412 + i * 5;
            len_freq += snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), "%d,", frequency);
        }
    }

    if (strlen(cmd) > len)
    {
        cmd[len + len_freq - 1] = '\0';
    }

    return p2p_ctrl_send_cmd(cmd);
}

static void udhcpd_cb(unsigned int yiaddr)
{
    g_ip_addr.s_addr = yiaddr;
    hccast_log(LL_NOTICE, "offer peer ip: %s\n", inet_ntoa(g_ip_addr));

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

    usleep(150 * 1000);

    hccast_net_ifconfig(ifname, HCCAST_P2P_IP, HCCAST_P2P_MASK, NULL);
    //g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTED);
    g_mira_param.state_update_func(HCCAST_P2P_STATE_GOT_IP);
}

static void udhcpc_cb(unsigned int data)
{
    hccast_udhcp_result_t *in = (hccast_udhcp_result_t *)data;
    if (in && in->stat)
    {
        inet_aton(in->gw, &g_ip_addr);
        hccast_log(LL_NOTICE, "got peer ip: %s\n", inet_ntoa(g_ip_addr));
        hccast_net_ifconfig(in->ifname, in->ip, in->mask, NULL);
        //g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTED);
        g_mira_param.state_update_func(HCCAST_P2P_STATE_GOT_IP);
    }
}

static udhcp_conf_t g_p2p_udhcpc_conf =
{
    .func   = udhcpc_cb,
    .ifname = UDHCP_IF_P2P0,
    .pid    = 0,
    .run    = 0,
    .option = UDHCPC_QUIT_AFTER_LEASE | UDHCPC_ABORT_IF_NO_LEASE,
};

static int p2p_ctrl_nl_udhcpc_start()
{
    udhcpc_start(&g_p2p_udhcpc_conf);
    return 0;
}

static int p2p_ctrl_nl_udhcpc_stop()
{
    udhcpc_stop(&g_p2p_udhcpc_conf);
    return 0;
}

/**
 * It's a thread that listens for events from the wpa_supplicant daemon
 *
 * @param arg the socket fd for attach wpas ctrl.
 *
 * @return The return value is a pointer to the new thread.
 */

static void *p2p_ctrl_nl_thread(void *arg)
{
#ifdef __linux__
    prctl(PR_SET_NAME, __func__);
#endif

    int fd = (int) arg;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    char aSend[256] = {0};
    char aResp[1024] = {0};
    size_t aRespN = 0;
    struct timeval tv = { 0, 0 };
    char peer_dev_addr[18] = {0};
    char peer_iface[18] = {0};
    bool group_started = false;
    int err = 0;
    char bssid_filter[128];
    char bssid_filter_idx = 0;

    udhcp_conf_t g_p2p_udhcpd_conf =
    {
        .func   = udhcpd_cb,
        .ifname = UDHCP_IF_P2P0,
        .pid    = 0,
        .run    = 0
    };

    udhcpd_start(&g_p2p_udhcpd_conf);

    char ifname[32] = {0};
    wifi_ctrl_get_p2p_ifname(ifname, sizeof(ifname));

AGAIN:
    g_p2p_device_is_go  = false;
    group_started = false;
    memset(&g_ip_addr, 0, sizeof(g_ip_addr));
    //p2p_ctrl_nl_device_init();

    while (g_p2p_thread_running)
    {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_usec = 100 * 1000;

        err = select(fd + 1, &fds, NULL, NULL, &tv);
        if (err < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            hccast_log(LL_ERROR, "select error!\n");
            goto EXIT;
        }
        else if (err == 0)
        {
            int64_t time_now = get_time_us();
            if (p2p_ctrl_nl_get_enable() && !p2p_ctrl_nl_get_connect_stat() && llabs(time_now - timestamp_listen) >= P2P_LISTEN_TIMEOUT)
            {
                timestamp_listen = time_now;
                hccast_log(LL_INFO, "listen timeout(%lld s), re listen now...\n", P2P_LISTEN_TIMEOUT / 1000000);
                //p2p_ctrl_send_cmd("P2P_LISTEN");
                p2p_ctrl_nl_device_init();
            }

            continue;
        }
        if (FD_ISSET(fd, &fds))
        {
            aRespN = sizeof aResp - 1;
            err = wpa_ctrl_recv(g_p2p_ifrecv, aResp, &aRespN);
            if (err)
            {
                hccast_log(LL_ERROR, "wpa recv error!\n");
                goto EXIT;
            }

            aResp[aRespN] = '\0';
            hccast_log(LL_DEBUG, " %s\n", aResp);

            if (!strncmp(aResp + 3, P2P_EVENT_GO_NEG_REQUEST, strlen(P2P_EVENT_GO_NEG_REQUEST)))
            {
                // <3>P2P-GO-NEG-REQUEST 9a:ac:cc:96:2d:6b dev_passwd_id=4 go_intent=15
                hccast_log(LL_NOTICE, "EVENT: P2P_EVENT_GO_NEG_REQUEST!\n");
                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_GO_NEG_FAILURE, strlen(P2P_EVENT_GO_NEG_FAILURE)))
            {
                hccast_log(LL_NOTICE, "EVENT: P2P_EVENT_GO_NEG_FAILURE!\n");

                if (HCCAST_NET_WIFI_ECR6600U == hccast_wifi_mgr_get_wifi_model())
                {
                    g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECT_FAILED);
                    goto AGAIN;
                }
                else
                {
                    char *argv[10];
                    int argc = 0;
                    char *pos = NULL;

                    char *copy = strdup(aResp + 3);
                    argc = tokenize_cmd(copy, argv);
                    free(copy);

                    if (argc < 1)
                    {
                        hccast_log(LL_ERROR, "tokenize_cmd!\n");
                        continue;
                    }

                    char *dev_addr = argv[1];

                    int  go_intent = 0;
                    if (argc > 3)
                    {
                        pos = strstr(argv[3], "go_intent=");
                        go_intent = atoi(pos + 10);
                    }

                    memset(aSend, 0, sizeof(aSend));
                    strcat(aSend, "P2P_CONNECT ");
                    strcat(aSend, dev_addr);
                    strcat(aSend, " pbc persistent");

                    if (0 == go_intent)
                    {
                        strcat(aSend, " go_intent=7");
                    }
                    else
                    {
                        p2p_peer_result_st p2p_peer = {0};
                        p2p_ctrl_peer(dev_addr, &p2p_peer);

                        if ((p2p_peer.group_capab & 1) != 0)
                        {
                            strcat(aSend, " join");
                        }
                        else
                        {
                            strcat(aSend, " go_intent=7");
                        }
                    }

                    //hccast_log(LL_INFO, "p2p aSend: %s\n", aSend);
                    p2p_ctrl_send_cmd(aSend);

                    char reply[1024] = {0};
                    unsigned int len = sizeof(reply) - 1;

                    g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
                }
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_PROV_DISC_PBC_REQ, strlen(P2P_EVENT_PROV_DISC_PBC_REQ)))
            {
                hccast_log(LL_DEBUG, "Event: DISC PBC REQ!\n");
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_GO_NEG_SUCCESS, strlen(P2P_EVENT_GO_NEG_SUCCESS)))
            {
                // P2P-GO-NEG-SUCCESS role=client freq=2437 ht40=1 peer_dev=9a:ac:cc:96:2d:6b peer_iface=9a:ac:cc:96:2d:6b wps_method=PBC
                hccast_log(LL_NOTICE, "EVENT: P2P_EVENT_GO_NEG_SUCCESS!\n");

                char val[32] = {0};
                if (result_get(aResp, "role", val, sizeof(val)) == 0)
                {
                    if (!strcasecmp(val, "client"))
                    {
                        g_p2p_device_is_go = false;
                    }
                    else if (!strcasecmp(val, "go"))
                    {
                        g_p2p_device_is_go = true;
                    }
                    else
                    {
                        g_p2p_device_is_go = false;
                    }
                }

                bssid_filter_idx = 0;
                bssid_filter_idx += sprintf(&bssid_filter[bssid_filter_idx], "SET bssid_filter ");
                if (result_get(aResp, "peer_dev", peer_dev_addr, sizeof(peer_dev_addr)) == 0)
                {
                    bssid_filter_idx += sprintf(&bssid_filter[bssid_filter_idx], "%s ", peer_dev_addr);
                }

                if (result_get(aResp, "peer_iface", peer_iface, sizeof(peer_iface)) == 0
                    && strncmp(peer_dev_addr, peer_iface, 17))
                {
                    bssid_filter_idx += sprintf(&bssid_filter[bssid_filter_idx], "%s ", peer_iface);
                }

                p2p_ctrl_send_cmd(bssid_filter);
#if 0
                if (log_level & LOG_LEVEL_DEBUG)
                {
                    int  freq = 0;
                    if (result_get(aResp, "freq", val, sizeof(val)) == 0)
                    {
                        freq = strtol(val, NULL, 10);
                    }
                    memset(peer_dev_addr, 0, sizeof(peer_dev_addr));
                    result_get(aResp, "peer_dev", peer_dev_addr, sizeof(peer_dev_addr));

                    hccast_log(LL_DEBUG, "Go Neo Result (role: '%s', freq: %d, peer_dev: '%s')\n", g_p2p_device_is_go ? "GO" : "client", freq, peer_dev_addr);
                }
#endif
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_DEVICE_FOUND, strlen(P2P_EVENT_DEVICE_FOUND)))
            {
                hccast_log(LL_NOTICE, "Event: P2P_EVENT_DEVICE_FOUND!\n");
                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_INVITATION_ACCEPTED, strlen(P2P_EVENT_INVITATION_ACCEPTED)))
            {
                // <3>P2P-INVITATION-ACCEPTED sa=9a:ac:cc:96:2d:6b persistent=1
                hccast_log(LL_NOTICE, "Event: INVITATION ACCEPTED!\n");
#if 0
                if (log_level & LOG_LEVEL_DEBUG)
                {
                    char val[32] = {0};
                    int  persistent = 0;

                    char *ptr = NULL;
                    ptr = strstr(aResp, "sa=");
                    if (ptr)
                    {
                        result_get(aResp, "sa", peer_dev_addr, sizeof(peer_dev_addr));

                        if (result_get(aResp, "persistent", val, sizeof(val)) == 0)
                        {
                            persistent = strtol(val, NULL, 10);
                        }
                    }

                    hccast_log(LL_DEBUG, "Result(sa: '%s', persistent: %d)\n", peer_dev_addr, persistent);
                }
#endif
                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTING);
            }
            else if (strstr(aResp + 3, "Trying to associate"))
            {
                hccast_log(LL_NOTICE, "Event: TRYING TO ASSOCIATE!\n");
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_GROUP_STARTED, strlen(P2P_EVENT_GROUP_STARTED)))
            {
                //<3>P2P-GROUP-STARTED p2p0 client ssid="DIRECT-XXX" freq=2462
                // psk=0d8a759a26a6cbe8c6ae7735ba39b4f3ff35b6c6002f3549e94003c34f88ffbe go_dev_addr=02:2e:2d:9d:78:58
                // [PERSISTENT] ip_addr=192.168.137.247 ip_mask=255.255.255.0 go_ip_addr=192.168.137.1
                hccast_log(LL_NOTICE, "Event: GROUP STARTED!\n");

                group_started = true;
                char *argv[10];
                int argc = 0;

                char *copy = strdup(aResp + 3);
                argc = tokenize_cmd(copy, argv);

                char *go_or_client = argv[2];
                if (strncasecmp(go_or_client, "client", strlen("client")) == 0)
                {
                    char ip_addr[32] = {0}, ip_mask[32] = {0}, go_ip_addr[32] = {0}, *ptr = NULL;
                    ptr = strstr(aResp, "ip_addr=");
                    if (ptr)
                    {
                        result_get(ptr, "ip_addr", ip_addr, sizeof(ip_addr));
                        result_get(ptr, "ip_mask", ip_mask, sizeof(ip_mask));
                        result_get(ptr, "go_ip_addr", go_ip_addr, sizeof(go_ip_addr));

                        hccast_log(LL_DEBUG, "result (ip_addr: '%s', go_ip_addr: '%s')\n", ip_addr, go_ip_addr);

                        inet_aton(go_ip_addr, &g_ip_addr);
                        hccast_net_ifconfig(ifname, ip_addr, ip_mask, go_ip_addr);
                        g_mira_param.state_update_func(HCCAST_P2P_STATE_GOT_IP);
                    }
                    else
                    {
                        p2p_ctrl_nl_udhcpc_stop();
                        p2p_ctrl_nl_udhcpc_start();
                    }
#ifdef HC_RTOS
                    char *presi = 0;
                    presi = strstr(aResp, "[PERSISTENT]");
                    if (presi)
                    {
                        printf("[%s][%d] PERSISTENT\n", __func__, __LINE__);
                        p2p_ctrl_p2p_store_network(&g_p2p_network_list);
                    }
#endif
                }
                else if(strncasecmp(go_or_client, "GO", strlen("GO")) == 0)
                {
                }

                hccast_log(LL_NOTICE, "%s %d: go_or_client: %s\n", __func__, __LINE__, go_or_client);

                free(copy);
            }
            else if (!strncmp(aResp + 3, WPA_EVENT_CONNECTED, strlen(WPA_EVENT_CONNECTED)))
            {
                // as Client
#ifdef USE_WPAS_P2P_CONF_FILE
                p2p_ctrl_send_cmd("SAVE_CONFIG");
#endif
                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTED);
                hccast_log(LL_INFO, "Event: CONNECTED!\n");
            }
            else if (!strncmp(aResp + 3, WPA_EVENT_DISCONNECTED, strlen(WPA_EVENT_DISCONNECTED)))
            {
                hccast_log(LL_NOTICE, "EVENT: DISCONNECT!\n");
                if (group_started)
                {
                    group_started = false;
                    p2p_ctrl_send_cmd("SET bssid_filter ");
                    g_mira_param.state_update_func(HCCAST_P2P_STATE_DISCONNECTED);
                }
                //goto again;
            }
            else if (!strncmp(aResp + 3, AP_STA_CONNECTED, strlen(AP_STA_CONNECTED)) && g_p2p_device_is_go)
            {
                // as Go
#ifdef USE_WPAS_P2P_CONF_FILE
                p2p_ctrl_send_cmd("SAVE_CONFIG");
#endif
                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECTED);
                hccast_log(LL_INFO, "EVENT: AP-STA-CONNECTED!\n");
            }
            else if (!strncmp(aResp + 3, AP_STA_DISCONNECTED, strlen(AP_STA_DISCONNECTED)) && g_p2p_device_is_go)
            {
                hccast_log(LL_NOTICE, "EVENT: AP-STA-DISCONNECTED!\n");
                if (group_started)
                {
                    group_started = false;
                    p2p_ctrl_send_cmd("SET bssid_filter ");
                    g_mira_param.state_update_func(HCCAST_P2P_STATE_DISCONNECTED);
                }
                //goto again;
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_GROUP_FORMATION_FAILURE, strlen(P2P_EVENT_GROUP_FORMATION_FAILURE)))
            {
                hccast_log(LL_NOTICE, "EVENT: GROUP FORMATION FAILURE!\n");
                p2p_ctrl_send_cmd("P2P_FLUSH");
                g_mira_param.state_update_func(HCCAST_P2P_STATE_CONNECT_FAILED);
                p2p_ctrl_send_cmd("REMOVE_NETWORK all");
                p2p_ctrl_send_cmd("SAVE_CONFIG");
                goto AGAIN;
            }
            else if (!strncmp(aResp + 3, P2P_EVENT_DEVICE_LOST, strlen(P2P_EVENT_DEVICE_LOST)))
            {
                hccast_log(LL_INFO, "EVENT: DEVICE LOST!\n");
            }
            else if (!strncmp(aResp + 3, WPA_EVENT_TERMINATING, strlen(WPA_EVENT_TERMINATING)))
            {
                g_p2p_thread_running = false;
                break;
            }
            else if (!strncmp(aResp + 3, WPA_EVENT_SCAN_RESULTS, strlen(WPA_EVENT_SCAN_RESULTS)) \
                     && wifi_ctrl_is_scanning() && !p2p_ctrl_nl_get_connect_stat())
            {
                hccast_log(LL_INFO, "EVENT: SCAN RESULTS!\n");
                wifi_ctrl_p2p_lock();
                wifi_ctrl_signal();
                wifi_ctrl_p2p_unlock();
            }
        }
    }

EXIT:
    if (HCCAST_NET_WIFI_NONE != hccast_wifi_mgr_get_cur_wifi_model())
    {
        p2p_ctrl_send_cmd("REMOVE_NETWORK all");
    }

    udhcpd_stop(&g_p2p_udhcpd_conf);
    udhcpc_stop(&g_p2p_udhcpc_conf);
    //p2p_ctrl_nl_device_disable();

    err = wpa_ctrl_detach(g_p2p_ifrecv);
    wpa_ctrl_close(g_p2p_ifrecv);

    g_p2p_ifrecv = NULL;
    g_p2p_thread_running = false;
    g_p2p_enable = false;

    hccast_log(LL_NOTICE, "%s end!\n", __func__);

    return NULL;
}

/**
 * It creates a thread that runs the p2p_ctrl_thread function
 *
 * @param params: p2p device params
 *
 * @return 0: success, <0: failed.
 */
int p2p_ctrl_nl_init(p2p_param_st *params)
{
    if (NULL == params)
    {
        hccast_log(LL_ERROR, "miracast param error!\n");
        return HCCAST_WIFI_ERR_CMD_PARAMS_ERROR;
    }

    memcpy(&g_mira_param, params, sizeof(p2p_param_st));

    int fd = p2p_ctrl_nl_wpas_init(); // need g_mira_param value.
    if (fd <= 0)
    {
        hccast_log(LL_NOTICE, "P2P init failed!\n");
        return fd;
    }

    p2p_ctrl_nl_wpas_attr_init();

    pthread_mutex_lock(&g_p2p_mutex);
    if (g_p2p_thread_running)
    {
        hccast_log(LL_NOTICE, "P2P already init!\n");
        pthread_mutex_unlock(&g_p2p_mutex);
        return 0;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0xc000);

    g_p2p_thread_running = true;
    if (pthread_create(&g_p2p_tid, &attr, p2p_ctrl_nl_thread, (int *)fd) != 0)
    {
        hccast_log(LL_ERROR, "create p2p thread error!\n");
        g_p2p_thread_running = false;
        pthread_mutex_unlock(&g_p2p_mutex);
        return HCCAST_WIFI_ERR_CMD_RUN_FAILED;
    }

    //pthread_detach(g_p2p_tid);
    pthread_mutex_unlock(&g_p2p_mutex);
    return 0;
}

/**
 *  This function is used to stop the p2p thread
 */
int p2p_ctrl_nl_uninit(void)
{
    pthread_mutex_lock(&g_p2p_mutex);
    if (g_p2p_tid)
    {
        g_p2p_thread_running = false;
        pthread_join(g_p2p_tid, NULL);
        g_p2p_tid = 0;
    }
    pthread_mutex_unlock(&g_p2p_mutex);

    return 0;
}