#include "lvgl/lvgl.h"
#include <hccast/hccast_dhcpd.h>
#include <hccast/hccast_wifi_mgr.h>

#define REMOTE_KEY

// typedef enum list_sub_param_type_{
//     LIST_PARAM_TYPE_STR,
//     LIST_PARAM_TYPE_INT
// } list_sub_param_type;

// typedef union list_sub_param_
// {
//     char* str;
//     int str_id;
// } list_sub_param;

// enum {
//     STR_WIFI_SAVED,
//     STR_WIFI_NEARBY,
//     WIFI_SEZRCH,
//     STR_WIFI_RET,
//     STR_WIFI_STRENGTH,
//     STR_WIFI_SECURITY,
//     STR_WIFI_PWD,
//     STR_WIFI_SHOW_PWD,
//     STR_WIFI_NET_MODE,
//     STR_WIFI_IP_ADDR,
//     STR_WIFI_NET_MASK,
//     STR_WIFI_GATE_WAY,
//     STR_WIFI_DNS,
//     STR_WIFI_MAC,
//     STR_WIFI_STATIC_IP
// };

// typedef enum{
//     ENGLISH,
//     CHINA,
// } langs;
typedef struct wifi_scan_node_
{
    struct wifi_scan_node_ *next;
    struct wifi_scan_node_ *prev;
    hccast_wifi_ap_info_t res;
    uint16_t same_wifi_num;
} wifi_scan_node;

typedef struct wifi_scan_node_head_{
    wifi_scan_node *next;
} wifi_scan_node_head;

typedef struct wifi_scan_list_
{
    wifi_scan_node_head *head;
    uint16_t len;
    wifi_scan_node *tail;
    wifi_scan_node *p_n;
} wifi_scan_list;

typedef enum wifi_scan_type_{
    WIFI_SCAN_ENTER,//进入配网界面自动scan
    WIFI_SCAN_ONOFF,//打开wifi自动scan
    WIFI_SCAN_SEARCH,//主动scan
} wifi_scan_type;

typedef enum wifi_conn_type_{
    WIFI_CONN_NORMAL,//
    WIFI_CONN_HIDDEN,//连接隐藏网络
} wifi_conn_type;

typedef void (*wifi_msg_timeout_func)(void *user_data);

extern int saved_wifi_sig_strength_max_id;
void wifi_get_udhcp_result(hccast_udhcp_result_t* result);
void wifi_list_set_zero();
void wifi_list_add(void *p);
void wifi_list_insert(hccast_wifi_ap_info_t* info, uint16_t same_wifi_num, wifi_scan_node *p_after);
wifi_scan_node* wifi_list_get_tail();
wifi_scan_node* wifi_list_get_head();
hccast_wifi_ap_info_t * wifi_list_get_node_by_name(char* name);
void wifi_is_reconning_set(bool en);
void wifi_list_remove(wifi_scan_node *node, bool copy);
bool wifi_list_update_is_enable();
bool wifi_list_mutex_lock();
void wifi_list_mutex_unlock();
