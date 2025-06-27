
#define LOG_TAG "hc_test_wifi"
#define ELOG_OUTPUT_LVL ELOG_LVL_ALL
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <wpa_ctrl.h>
#include <dirent.h>
#include <sys/wait.h>
#include "iniparser.h"

#include "hc_test_wifi.h"
#include "app_config.h"

#ifdef BOARDTEST_CJSON_SUPPORT
#include <cjson/cJSON.h>
#endif

#ifdef BOARDTEST_NET_SUPPORT
#include <uapi/linux/wireless.h>
#endif

#ifdef __HCRTOS__
#include <kernel/lib/console.h>
#include <kernel/elog.h>

#ifdef BOARDTEST_WIFI_SUPPORT
extern int wpa_supplicant_main(int argc, char *argv[]);
extern int eloop_is_run(void);
#endif

#else
#include <sys/prctl.h>
#define log_e printf
#define log_w printf
#define log_i printf
#endif

extern unsigned int OsShellDhclient2(int argc, const char **argv);
extern unsigned int lwip_ifconfig2(int argc, const char **argv);

static pthread_mutex_t g_wifi_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t wpa_supplicant_tid;
bool wpa_sup_init = false;
bool test_wifi_flag = false;

/**
 * @description: Destroy the wpa supplicant parameter structure
 * @param {wpa_params} *params
 * @return {*}
 */
static void wpa_params_destroy(struct wpa_params *params)
{
    int i = 0;
    if (params)
    {
        if (params->argv)
        {
            for (i = 0; i < params->argc; i++)
                free(params->argv[i]);
            free(params->argv);
        }
        free(params);
    }
}

/**
 * @description: Execute system commands
 * @param {char} *cmd
 * @return {*}
 */
static int hc_test_wifi_system_cmd(const char *cmd)
{
    pid_t pid;
    if (-1 == (pid = vfork()))
    {
        return 1;
    }
    if (0 == pid)
    {
        execl("/bin/sh", "sh", "-c", cmd, (char *)0);
        return  0;
    }
    else
    {
        wait(&pid);
    }
    return 0;
}

/**
 * @description: Remove Spaces in the configuration file
 * @param {char} *remove
 * @param {int} remove_len
 * @return {*}
 */
static void hc_test_wifi_clear_space(char *remove, int remove_len)
{
    int i = 0;
    int j = 0;
    for (i = 0; i < remove_len; i++)
    {
        if (remove[i] != ' ')
        {
            remove[j++] = remove[i];
        }
        else if (remove[i] == ' ')
        {
            continue;
        }
    }
    remove[j] = '\0';
}


static int my_iniparser_getstring(const char *filename, const char *section, const char *key, char *code)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Error opening INI file\n");
        return 0;
    }

    char line[128];
    char search_str[128];
    snprintf(search_str, sizeof(search_str), "[%s]", section);

    int in_section = 0;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Remove newline characters from the end of a line
        line[strcspn(line, "\n")] = '\0';

        // Determines if in the specified section
        if (strcmp(line, search_str) == 0)
        {
            in_section = 1;
        }
        else if (in_section && line[0] == '[')
        {
            // The specified section has been left
            break;
        }

        // Finds the key in the specified section
        if (in_section)
        {
            char *equals = strchr(line, '=');

            if (equals != NULL)
            {
                *equals = '\0'; // Replace the equal sign with the string terminator
                char *trimmed_key = strtok(line, " \t"); // Remove whitespace characters from both ends of the key
                char *trimmed_value = equals + 1; // Retrieves the value after the equal sign
                strcpy(code, equals + 1);

                // Check if the key matches
                if (trimmed_key != NULL && strcmp(trimmed_key, key) == 0)
                {
                    fclose(file);
                    return 1;
                }
            }
        }
    }
    fclose(file);
    return 0;
}

/**
 * @description: Read wifi profile in USB or sd
 * @return {*}
 */
static wifi_config hc_test_wifi_read_content(void)
{
    DIR *dir;
    struct dirent *em;
    const char ini_path[320] = {0};
    wifi_config test_wifi = {0};
    char ssid[64] = {0};
    char pwd[64] = {0};
    int ret = 0;

    dir = opendir("/media");
    while ((em = readdir(dir)) != NULL)
    {
        log_w("\n\t/media/%s: \n", em->d_name);
        sprintf(ini_path, "/media/%s/%s", em->d_name, BOARDTEST_INI_NAME);

        log_w("open dir == %s\n", ini_path);
        if (access(ini_path, F_OK) != -1)
        {
            ret = 1;
            break;
        }
    }
    closedir(dir);
    if(ret != 1)
    {
        test_wifi.get_results = 0;
        return test_wifi;
    }

    ret = my_iniparser_getstring(ini_path, "WIFI_TEST", "SSID", test_wifi.ssid);
    ret = my_iniparser_getstring(ini_path, "WIFI_TEST", "PWD", test_wifi.pwd);

    hc_test_wifi_clear_space(test_wifi.ssid, strlen(test_wifi.ssid));
    hc_test_wifi_clear_space(test_wifi.pwd, strlen(test_wifi.pwd));

    log_i("ssid = %s\n", test_wifi.ssid);
    log_i("pwd = %s\n", test_wifi.pwd);

    if (ret == 0)
    {
        test_wifi.get_results = 0;
    }
    else
    {
        test_wifi.get_results = 1;
    }

    return test_wifi;
}

#ifdef BOARDTEST_NET_SUPPORT
/**
 * @description: Get the mac address of the wifi
 * @param {char} *mac
 * @return {*}
 */
static int hc_test_wifi_get_mac_addr(char *mac)
{
#ifdef __linux__
    int ret = 0;
    char buffer[32] = {0};
    int fd = 0;

    if (mac == NULL)
    {
        log_e("%s:%d: the parameter is invalid\n", __func__, __LINE__);
        return -1;
    }

    fd = open(WIFI_MAC_PATH, O_RDONLY);
    if (fd < 0)
    {
        log_e("err: api_get_mac_addr: could not open %s\n", WIFI_MAC_PATH);
        return -1;
    }

    ret = read(fd, (void *)&buffer, sizeof(buffer));
    if (ret <= 0)
    {
        log_e("api_get_mac_addr: read failed\n");
        goto - 1;
    }

    log_i("%s mac: %s\n", WIFI_MAC_PATH, buffer);

    sscanf(buffer, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

    close(fd);

#else
    struct ifreq ifr;
    int skfd;

    if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        log_e("socket error\n");
        return -1;
    }

    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ);
    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) != 0)
    {
        log_e( "%s net_get_hwaddr: ioctl SIOCGIFHWADDR\n", __func__);
        close(skfd);
        return -1;
    }
    close(skfd);
    memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
#endif

    return 0;
}


/**
 * @description: Check whether there is a wifi module
 * @return {*}
 */
static int hc_test_wifi_module_get(void)
{
    int wifi_module = 0;

    unsigned char mac[6] = {0};
    if (0 == hc_test_wifi_get_mac_addr((char *)mac))
        wifi_module = 1;

    return wifi_module;
}
#endif

#ifdef BOARDTEST_WIFI_SUPPORT

/**
 * @description: Perform the WPA CLI operation
 * @param {char} *ifname
 * @param {char} *cmd
 * @param {char} *result
 * @param {size_t} *len
 * @return {*}succeed 1 fail 0
 */
static int wifi_ctrl_run_cmd(const char *ifname, char *cmd, char *result, size_t *len)
{
    char path[128] = {"\0"};

#ifdef __HCRTOS__
    int port = 0;

    if (eloop_is_run() != 1)
    {
        log_e("%s: WPAS NO RUN!\n", __func__);
        return 0;
    }
#endif

    if (NULL == ifname || NULL == cmd || NULL == result)
    {
        log_e("param error!\n");
        return 0;
    }

    sprintf(path, WIFI_CTRL_PATH_STA"/%s", ifname);
#ifdef __HCRTOS__
    if (strcmp(ifname, WIFI_CTRL_IFACE_NAME) == 0)
    {
        port = WPA_CTRL_WPA_IFACE_PORT;
    }
    else if (strcmp(ifname, P2P_CTRL_IFACE_NAME) == 0)
    {
        port = WPA_CTRL_P2P_IFACE_PORT;
    }
    else
    {
        log_e("%s mode error! ()\n", __func__);
        return 0;
    }
#endif

#ifdef __linux__
    if (access(path, F_OK)) // no exist
    {
        log_e("ctrl_iface %s non-existent!\n", path);
        return 0;
    }
#endif

    pthread_mutex_lock(&g_wifi_mutex);
#ifdef __HCRTOS__
    struct wpa_ctrl *wpa_ctrl = wpa_ctrl_open(path, port);
#else
    struct wpa_ctrl *wpa_ctrl = wpa_ctrl_open(path);
#endif
    if (!wpa_ctrl)
    {
        log_e("wpa_ctrl_open failed:!\n");
        pthread_mutex_unlock(&g_wifi_mutex);
        return 0;
    }

    int ret = wpa_ctrl_request(wpa_ctrl, cmd, strlen(cmd), result, len, NULL);
    result[*len] = 0;
    pthread_mutex_unlock(&g_wifi_mutex);

    wpa_ctrl_close(wpa_ctrl);

    return 1;
}

/**
 * @description: Thread that creates the wpa supplicant
 * @param {wpa_params} *params
 * @param {void} *
 * @return {*}
 */
static int sup_pthread_create(struct wpa_params *params, void *(*entry) (void *))
{
    pthread_attr_t attr;
    int stack_size = 64 * 1024;
    int ret = 0;

    if (!params)
    {
        return 0;
    }
    ret = pthread_attr_init(&attr);
    if (ret)
    {
        log_e("Init pthread_attr_t error.\n");
        wpa_params_destroy(params);
        return 0;
    }

    ret = pthread_attr_setstacksize(&attr, stack_size);
    if (ret != 0)
    {
        log_e("Set stack size error.\n");
        wpa_params_destroy(params);
        return 0;
    }

    ret = pthread_create(&wpa_supplicant_tid, &attr, entry, params);
    if (ret != 0)
    {
        log_e("Create wpa_supplicant error.\n");
        wpa_params_destroy(params);
        return 0;
    }

    while (1)
    {

        if (eloop_is_run() == 1)
        {
            log_w("wifi_ctrl_sta_thread_start ok\n");
            break;
        }

        usleep(100 * 1000);
    }
    log_i("Create thread success.\n");
    return 1;
}

/**
 * @description: Initialization thread function for wpa supplicant
 * @param {void} *args
 * @return {*}
 */
static void *wpa_supplicant_thread(void *args)
{
    struct wpa_params *params = (struct wpa_params *)args;
    wpa_supplicant_main(params->argc, params->argv);
    wpa_params_destroy(params);
    log_i("%s:exit\n", __func__);
    return NULL;
}


/**
 * @description: Initialization of the wpa supplicant
 * @return {*}
 */
static int hc_test_wifi_sup_init(void)
{
    struct wpa_params *params = (struct wpa_params *)calloc(1, sizeof(struct wpa_params));
    int i = 0;
    char arguments[][64] = {"wpa_supplicant", "-i", "wlan0", "-Dwext", "-c", "/etc/wpa_supplicant.conf", \
                            "-C", "/var/run/wpa_supplicant"
                           };

    params->argv = (char **)calloc(8, sizeof(char *));
    params->argc = 8;
    for (i = 0; i < params->argc; i++)
    {
        params->argv[i] = (char *)strdup(arguments[i]);
        log_w("argv[%d] = %s\n", i, params->argv[i]);
    }
    sup_pthread_create(params, wpa_supplicant_thread);

    return 1;
}


/**
 * @description: The ADD NETWORK operation of the WPA CLI
 * @return {*}succeed 1 fail 0
 */
static int hc_test_wifi_cli_init(void)
{
    int ret = 0;
    char cmd[256] = {0};
    char reply[1024] = {0};
    size_t len = sizeof(reply) - 1;
    int net_id = -1;

    sprintf(cmd, "%s", "ADD_NETWORK");
    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        log_e("%s, ret = %d\n", cmd, ret);
        return 0;
    }
    net_id = atoi(reply);
    log_i("net_id = %d\n", net_id);

    return 1;
}
/**
 * @description: The SET NETWORK ssid operation of the WPA_CLI
 * @param {char} *ssid
 * @return {*} succeed 1 fail 0
 */
static int hc_test_wifi_set_network_ssid(char *ssid)
{
    int ret = 0;
    char cmd[256] = {0};
    char reply[1024] = {0};
    size_t len = sizeof(reply) - 1;
    sprintf(cmd, "SET_NETWORK %d ssid \"%s\"", 0, ssid);

    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        log_e("%s, ret = %d\n", cmd, ret);
        return 0;
    }
    log_i("%s", reply);
    if (!strcmp(reply, "ok"))
    {
        log_e("%s\n", reply);

        return 0;
    }

    return 1;
}

/**
 * @description: The SET NETWORK psk operation of the WPA_CLI
 * @param {char} *pwd
 * @return {*}succeed 1 fail 0
 */
static int hc_test_wifi_set_network_psk(char *pwd)
{

    int ret = 0;
    char cmd[256] = {0};
    char reply[1024] = {0};
    size_t len = sizeof(reply) - 1;

    sprintf(cmd, "SET_NETWORK %d psk \"%s\"", 0, pwd);
    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        log_e("%s, ret = %d\n", cmd, ret);
        return 0;
    }
    log_i("%s", reply);
    if (!strcmp(reply, "ok"))
    {
        log_e("%s\n", reply);

        return 0;
    }

    return 1;
}

/**
 * @description: The SELECT_NETWORK operation of the WPA_CLI
 * @return {*}succeed 1 fail 0
 */
static int hc_test_wifi_select_network(void)
{

    int ret = 0;
    char cmd[256] = {0};
    char reply[1024] = {0};
    size_t len = sizeof(reply) - 1;

    sprintf(cmd, "SELECT_NETWORK %d", 0);
    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        log_e("%s, ret = %d\n", cmd, ret);
        return 0;
    }
    log_i("%s", reply);
    if (!strcmp(reply, "ok"))
    {
        log_e("%s\n", reply);

        return 0;
    }

    return 1;
}

static int hc_test_wifi_remove_network(void)
{
    int ret = 0;
    char cmd[256] = {0};
    char reply[1024] = {0};
    size_t len = sizeof(reply) - 1;
    sprintf(cmd, "DISABLE_NETWORK %d", 0);

    ret = wifi_ctrl_run_cmd(WIFI_CTRL_IFACE_NAME, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        log_e("%s, ret = %d\n", cmd, ret);
        return 0;
    }
    log_i("%s", reply);
    if (!strcmp(reply, "ok"))
    {
        log_e("%s\n", reply);

        return 0;
    }

    return 1;
}

/**
 * @description: Fetching results
 * @param {char} *str
 * @param {char} *key
 * @param {char} *val
 * @param {int} val_len
 * @return {*}succeed 1 fail 0
 */
static int wifi_info_get(char *str, char *key, char *val, int val_len)
{
    if (NULL == str || NULL == key || NULL == val)
    {
        log_e("param error!\n");
        return 0;
    }

    char keys[64] = {0};
    char vals[256] = {0};
    char *token;
    char *saveptr;

    char *strs = strdup(str);
    token = strtok_r(strs, "\n", &saveptr);

    while (token != NULL)
    {
        sscanf(token, "%[^=]=%[^'\n']", keys, vals);

        if (!strcmp(key, keys))
        {
            memcpy(val, vals, val_len);
            break;
        }

        token = strtok_r(NULL, "\n", &saveptr);
    }

    free(strs);

    return 1;
}


/**
 * @description: Conversion to Chinese
 * @param {char} *srcStr
 * @return {*}
 */
static char *wifi_ctrl_chinese_conversion(char *srcStr)
{
    int src_i, dest_i;
    int srcLen = 0;

    if (NULL == srcStr)
    {
        log_e("param error!\n");
        return NULL;
    }

    srcLen = strlen(srcStr);

    for (src_i = 0, dest_i = 0; src_i < srcLen; src_i++, dest_i++)
    {
        if (srcStr[src_i] == '\\' && (srcStr[src_i + 1] == 'x' || srcStr[src_i + 1] == 'X'))
        {
            char tmp[3] = "";
            tmp[0] = srcStr[src_i + 2];
            tmp[1] = srcStr[src_i + 3];
            srcStr[dest_i] = strtoul(tmp, NULL, 16);
            src_i += 3;
        }
        else
        {
            srcStr[dest_i] = srcStr[src_i];
        }
    }
    srcStr[dest_i] = '\0';

    return srcStr;
}

/**
 * @description: Get the wifi status
 * @param {char} *ifname
 * @param {hc_test_wifi_status_result_t} *result
 * @return {*}
 */
static int wifi_ctrl_get_status(const char *ifname, hc_test_wifi_status_result_t *result)
{
    int ret = -0x0;
    char val[512] = {0};
    char reply[1024] = {0};
    size_t len = sizeof(reply) - 1;
    char *cmd = "STATUS";

    if (NULL == result || NULL == ifname)
    {
        log_e("param error!\n");
        ret = 0;
        goto ERROR;
    }

    memset(result, 0x00, sizeof(hc_test_wifi_status_result_t));
    ret = wifi_ctrl_run_cmd(ifname, cmd, reply, &len);
    if (ret < 0 || len == 0)
    {
        log_e("%s run error!\n", cmd);
        goto ERROR;
    }

    if (strncmp(reply, "FAIL", 4) == 0)
    {
        log_e("%s (FAIL)\n", cmd);
        ret = 0;
        goto ERROR;
    }

    wifi_info_get(reply, "bssid", result->bssid, sizeof(result->bssid));

    if (wifi_info_get(reply, "freq", val, sizeof(val)) == 0)
    {
        result->freq = strtol(val, NULL, 10);
    }

    wifi_info_get(reply, "ssid", val, sizeof(val));
    wifi_ctrl_chinese_conversion(val);

    memcpy(result->ssid, val, sizeof(result->ssid));

    if (wifi_info_get(reply, "id", val, sizeof(val)) == 0)
    {
        result->id = strtol(val, NULL, 10);
    }

    wifi_info_get(reply, "mode", result->mode, sizeof(result->mode));
    wifi_info_get(reply, "pairwise_cipher", result->pairwise_cipher, sizeof(result->pairwise_cipher));
    wifi_info_get(reply, "group_cipher", result->group_cipher, sizeof(result->group_cipher));
    wifi_info_get(reply, "key_mgmt", result->key_mgmt, sizeof(result->key_mgmt));
    wifi_info_get(reply, "wpa_state", result->wpa_state, sizeof(result->wpa_state));
    wifi_info_get(reply, "ip_address", result->ip_address, sizeof(result->ip_address));
    wifi_info_get(reply, "address", result->address, sizeof(result->address));
    wifi_info_get(reply, "uuid", result->uuid, sizeof(result->uuid));
    wifi_info_get(reply, "p2p_device_address", result->p2p_device_address, sizeof(result->p2p_device_address));

    return 1;

ERROR:
    return 0;
}
#endif



/**
 * @description: wifi test program initialization
 * @return {*}
 */
static int hc_test_wifi_init(void)
{
    int ret;
    wifi_config check_existence = {0};
    const char *ifconfig_up[20];
    ifconfig_up[0] = "wlan0";
    ifconfig_up[1] = "up";

#ifdef BOARDTEST_WIFI_SUPPORT
    ret = hc_test_wifi_module_get();
    check_existence = hc_test_wifi_read_content();
    test_wifi_flag = false;

    if (ret == 0 || check_existence.get_results == 0)
    {
        log_e("no module\n");
        write_boardtest_detail(BOARDTEST_WIFI_TEST, "no wifi module");
        return BOARDTEST_FAIL;
    }

#ifdef __HCRTOS__
    if (wpa_sup_init == false)
    {
        lwip_ifconfig2(2, ifconfig_up);
        wpa_sup_init = true;
        hc_test_wifi_sup_init();
        hc_test_wifi_cli_init();
    }
#endif
    return BOARDTEST_PASS;

#else
    return BOARDTEST_FAIL;

#endif
}

/**
 * @description: wifi connection
 * @return {*}
 */
static int hc_test_wifi_start(void)
{
    hc_test_wifi_status_result_t wifi_res = {0};
    wifi_config test_wifi = {0};
    const char *wifi_name[16];
    wifi_name[0] = "wlan0";
    int i = 0;

#ifdef BOARDTEST_WIFI_SUPPORT
    wifi_ctrl_get_status("wlan0", &wifi_res);
    test_wifi = hc_test_wifi_read_content();
    if (test_wifi.get_results == 0 && strcmp(wifi_res.wpa_state, "COMPLETED") != 0)
    {
        log_e("no get configuration information");
        write_boardtest_detail(BOARDTEST_WIFI_TEST, "no get configuration information");
        return BOARDTEST_FAIL;
    }

    log_i("test_wifi.get_results = %d\n", test_wifi.get_results);
    log_i("test_wifi.ssid = %s\n", test_wifi.ssid);
    log_i("test_wifi.pwd = %s\n", test_wifi.pwd);

    usleep(3 * 1000);
    hc_test_wifi_set_network_ssid(test_wifi.ssid);
    usleep(3 * 1000);
    hc_test_wifi_set_network_psk(test_wifi.pwd);
    usleep(3 * 1000);
    hc_test_wifi_select_network();

    while (strcmp(wifi_res.wpa_state, "COMPLETED") != 0)
    {
        wifi_ctrl_get_status("wlan0", &wifi_res);
        log_w("wifi_res.wpa_state = %s\n", wifi_res.wpa_state);
        usleep(100 * 1000);
        if (i == 120 || test_wifi_flag == true)
        {
            log_e("connection timeout\n");
            i = 0;
            write_boardtest_detail(BOARDTEST_WIFI_TEST, "connection timeout");
            return BOARDTEST_FAIL;
        }
        i++;
    }
    i = 0;

#ifdef __HCRTOS__
    OsShellDhclient2(1, wifi_name);
#else
    hc_test_wifi_system_cmd("udhcpc -i wlan0 -t 3 -n");
#endif
    while (strcmp(wifi_res.ip_address, "0.0.0.0") == 0)
    {
        wifi_ctrl_get_status("wlan0", &wifi_res);
        log_w("wifi_res.ipaddr = %s\n", wifi_res.ip_address);
        usleep(100 * 1000);
        if (i == 120 || test_wifi_flag == true)
        {
            i = 0;
            log_e("Failed to assign ip address\n");
            write_boardtest_detail(BOARDTEST_WIFI_TEST, "Failed to assign ip address");
            return BOARDTEST_FAIL;
        }
        i++;
    }
    return BOARDTEST_PASS;

#else
    return BOARDTEST_FAIL;
#endif
}


/**
 * @description: Test result acquisition
 * @return {*}
 */
static int hc_test_wifi_exit(void)
{
    hc_test_wifi_status_result_t wifi_res = {0};
    char test_result[128] = {0};
    int ret;
    const char *close_ip[20];
    close_ip[0] = "-x";
    close_ip[1] = "wlan0";

#ifdef BOARDTEST_WIFI_SUPPORT
    test_wifi_flag = true;
    ret = hc_test_wifi_module_get();
    if (ret == 0)
    {
        log_e("no wifi module\n");
        write_boardtest_detail(BOARDTEST_WIFI_TEST, "no wifi module");
        return BOARDTEST_FAIL;
    }

    wifi_ctrl_get_status("wlan0", &wifi_res);
    if (strcmp(wifi_res.wpa_state, "COMPLETED"))
    {
        sprintf(test_result, "stats:%s", wifi_res.wpa_state);
        write_boardtest_detail(BOARDTEST_WIFI_TEST, test_result);
        return BOARDTEST_FAIL;
    }

    sprintf(test_result, "ipaddr:%s \nssid:%s \nstats:%s", wifi_res.ip_address, wifi_res.ssid, wifi_res.wpa_state);
    write_boardtest_detail(BOARDTEST_WIFI_TEST, test_result);
    hc_test_wifi_remove_network();

#ifdef __HCRTOS__
    OsShellDhclient2(2, close_ip);
#endif

    return BOARDTEST_PASS;
#else
    return BOARDTEST_FAIL;
#endif

}

/*----------------------------------------------------------------------------------*/
/**
 * @brief hc_boardtest_<module>_auto_register
 */
static int hc_boardtest_WIFI_auto_register(void)
{
    hc_boardtest_msg_reg_t *test_wifi = malloc(sizeof(hc_boardtest_msg_reg_t));

    test_wifi->english_name = "WIFI_TEST";
    test_wifi->sort_name = BOARDTEST_WIFI_TEST;
    test_wifi->init = hc_test_wifi_init;
    test_wifi->run = hc_test_wifi_start;
    test_wifi->exit = hc_test_wifi_exit;
    test_wifi->tips = NULL; /*mbox tips*/

    hc_boardtest_module_register(test_wifi);

    return 0;
}

__initcall(hc_boardtest_WIFI_auto_register)





