/*
win_cast_root.c
 */
#include <pthread.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "osd_com.h"
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_dhcpd.h>
#include <hccast/hccast_httpd.h>
#include <hcfota.h>

#include "cast_api.h"
#include "win_cast_root.h"

//#include "../lv_drivers/display/fbdev.h"

#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"
#include "network_api.h"
#include "data_mgr.h"


#define IMG_PHONE_X  336
#define IMG_PHONE_Y  524

#define IMG_DONGLE_X  546
#define IMG_DONGLE_Y   434

#define LABEL_LOCAL_SSID_W   300
#define LABEL_LOCAL_SSID_X  ((OSD_MAX_WIDTH-LABEL_LOCAL_SSID_W) >> 1) //480
#define LABEL_LOCAL_SSID_Y   30
#define LABEL_LOCAL_SSID_GAP 100
#define LABEL_LOCAL_SSID_EXTRA_W (LABEL_LOCAL_SSID_GAP - 10)

#define LABEL_WIFI_SSID_X  30
#define LABEL_WIFI_SSID_Y  120
#define LABEL_WIFI_SSID_W  600//280

#define LABEL_LOCAL_PWD_X  (LABEL_LOCAL_SSID_X + LABEL_LOCAL_SSID_W +LABEL_LOCAL_SSID_GAP)
#define LABEL_LOCAL_PWD_Y   LABEL_LOCAL_SSID_Y
#define LABEL_LOCAL_PWD_W   300

#define LABEL_VERSION_X  LABEL_WIFI_SSID_X
#define LABEL_VERSION_Y  (OSD_MAX_HEIGHT-80)
#define LABEL_VERSION_W   300

#define AIR_DEMO_X  (OSD_MAX_WIDTH-AIR_DEMO_W)//30
#define AIR_DEMO_Y  LABEL_VERSION_Y
#define AIR_DEMO_W 200

#define AIR_STATUS_W 400
#define AIR_STATUS_X    ((OSD_MAX_WIDTH-AIR_STATUS_W)/2)
#define AIR_STATUS_Y    LABEL_VERSION_Y

#define LABEL_CONNECT_MSG_W   600
#define LABEL_CONNECT_MSG_X  ((OSD_MAX_WIDTH-LABEL_CONNECT_MSG_W) >> 1)
#define LABEL_CONNECT_MSG_Y  LABEL_VERSION_Y

#define GAP_W 30
#define GAP_H 10
#define START_X        20

#define QR_BOX_X        START_X
#define QR_BOX_Y        170
#define QR_BOX_W        110

#define QR_MSG_BMP_X START_X
#define QR_MSG_BMP_Y QR_BOX_Y + QR_BOX_W+GAP_H
#define QR_MSG_BMP_W 16

#define QR_MSG_X        QR_MSG_BMP_X + QR_MSG_BMP_W + 2
#define QR_MSG_Y         QR_BOX_Y + QR_BOX_W+GAP_H
#define QR_MSG_W        500

#define QR_WECHAT_X        START_X
#define QR_WECHAT_Y        QR_MSG_Y+GAP_W 

#define QR_WECHAT_BMP_X     START_X
#define QR_WECHAT_BMP_Y        QR_WECHAT_Y+QR_BOX_W+GAP_H
#define QR_WECHAT_BMP_W 16

#define QR_WECHAT_MSG_X     START_X + QR_WECHAT_BMP_W + 2
#define QR_WECHAT_MSG_Y        QR_WECHAT_Y+QR_BOX_W+GAP_H

#define QR_GUIDE_X        START_X
#define QR_GUIDE_Y        QR_WECHAT_MSG_Y+GAP_W 

#define QR_GUIDE_BMP_X QR_GUIDE_X
#define QR_GUIDE_BMP_Y QR_GUIDE_Y+QR_BOX_W+GAP_H 
#define QR_GUIDE_BMP_W 16

#define QR_GUIDE_MSG_X  QR_GUIDE_BMP_X + QR_GUIDE_BMP_W + 2
#define QR_GUIDE_MSG_Y  QR_GUIDE_Y+QR_BOX_W+GAP_H

#define LABEL_AIRP2P_SSID_W   400
#define LABEL_AIRP2P_SSID_X  ((OSD_MAX_WIDTH-LABEL_AIRP2P_SSID_W) >> 1) //480
#define LABEL_AIRP2P_SSID_Y   30

#define LABEL_AIRP2P_PWD_X  LABEL_AIRP2P_SSID_X + LABEL_AIRP2P_SSID_W + 100
#define LABEL_AIRP2P_PWD_Y   LABEL_AIRP2P_SSID_Y
#define LABEL_AIRP2P_PWD_W   300


LV_IMG_DECLARE(camera)
LV_IMG_DECLARE(wechat)
LV_IMG_DECLARE(browser)


static lv_obj_t *win_root_obj = NULL;

static lv_obj_t *m_label_local_ssid = NULL;
static lv_obj_t *m_label_wifi_ssid = NULL;
static lv_obj_t *m_label_password = NULL; //also for ip addr

static lv_obj_t *m_label_ip = NULL;

static lv_obj_t *m_label_connect_state = NULL;
static lv_obj_t *m_label_wifi_mode = NULL;

//static lv_obj_t *m_label_connect_msg = NULL;

static lv_obj_t *m_label_qr_msg = NULL;
static lv_obj_t *m_label_state_msg = NULL;

static lv_obj_t *m_label_version = NULL;
static lv_obj_t *m_label_demo = NULL;

static lv_obj_t *m_cast_qr = NULL;
static lv_obj_t *m_wechat_qr = NULL;
static lv_obj_t *m_guide_qr = NULL;
static lv_obj_t *m_wechat_msg = NULL;
static lv_obj_t *m_guide_msg = NULL;
static lv_obj_t *m_camera_img = NULL;
static lv_obj_t *m_wechat_img = NULL;
static lv_obj_t *m_guide_img = NULL;
static lv_obj_t *m_browser_img = NULL;


static lv_style_t m_large_text_style;
static lv_style_t m_mid_text_style;
static lv_style_t m_small_text_style;

static volatile bool m_win_cast_open = false;

static lv_timer_t *m_connect_timer = NULL;

typedef enum
{
    QR_CLEAR,
    QR_CONNECT_AP,
    QR_SCAN_WIFI,
    QR_CONFIG,
} qr_show_type_t;

//Exit from dlna/miracast/aircast to cast ui by remote key or message.
//This cast, do not need to wait cast UI open before stop dlna/miracast/aircast.
static bool m_exit_to_cast_by_key = false;
#define SYMBOL_WIFI_LIMITED  "\xEE\xA6\x94" /*Unicode 0xe994*/
LV_FONT_DECLARE(wifi_font_limited);

#if 0
static void hc_background_show(bool show)
{
    lv_obj_t* bgk;
    bgk = lv_obj_create(lv_scr_act());//创建对象
    //lv_obj_clean_style_list(bgk, LV_OBJ_PART_MAIN); //清空对象风格
    lv_obj_set_style_bg_opa(bgk, LV_OPA_10, LV_PART_MAIN);//设置颜色覆盖度100%，数值越低，颜色越透。
    lv_obj_set_style_bg_color(bgk, lv_color_hex(0x4B50B0), LV_PART_MAIN);//设置背景颜色为绿色
    //省去下方两行代码，默认是从0,0处开始绘制
    lv_obj_set_x(bgk, 50);//设置X轴起点
    lv_obj_set_y(bgk, 50);//设置Y轴起点

    lv_obj_set_size(bgk, 600, 400);//设置覆盖大小
//  lv_task_create(bgk_anim, 500, LV_TASK_PRIO_LOW, bgk);//创建任务 500ms一次
}
#endif

static lv_obj_t *lv_img_open(lv_obj_t *parent, const char *bitmap, int x, int y)
{
    lv_obj_t * img = lv_img_create(parent);
    lv_img_set_src(img, bitmap);
    lv_obj_set_pos(img, x, y);
    //lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, 0);
    return img;
}

static lv_obj_t *lv_label_open(lv_obj_t *parent, int x, int y, int w, char *str, lv_style_t *style)
{
    lv_obj_t *label = lv_label_create(parent);

    if (x && y)
        lv_obj_set_pos(label, x, y);

    if (w)
        lv_obj_set_width(label, w);

    if (str)
        lv_label_set_text(label, str);
    else
        lv_label_set_text(label, "");

    lv_obj_add_style(label, style, 0);

    return label;
}

static void win_cast_update_qr_code(qr_show_type_t qr_type)
{
    /*Set data*/
    //const char * data = "https://lvgl.io";
    char qr_txt[128] = {0};
    char msg_txt[128] = {0};
    char wechat_msg_txt[32] = {0};
    char wechat_qr_txt[128] = {0};
    char support_5g = '0';
    hccast_wifi_ap_info_t * wifi_ap = NULL;

    if (network_wifi_module_get())
    {
        hccast_wifi_freq_mode_e mode = hccast_wifi_mgr_freq_support_mode();
        if (mode == HCCAST_WIFI_FREQ_MODE_5G)
        {
            support_5g = '1';
        }
        else
        {
            support_5g = '0';
        }
    }    

    lv_obj_clear_flag(m_cast_qr, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(m_wechat_qr, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(m_wechat_img, LV_OBJ_FLAG_HIDDEN);
    switch (qr_type)
    {
        case QR_CLEAR:
            sprintf(msg_txt," ");
            sprintf(wechat_msg_txt," ");
            lv_obj_add_flag(m_cast_qr, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(m_wechat_qr, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(m_camera_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(m_browser_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(m_wechat_img, LV_OBJ_FLAG_HIDDEN);
            break;
        case QR_CONNECT_AP:
            //sprintf(msg_txt,"First scan QR code\nSet AP network");
            lv_obj_clear_flag(m_camera_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(m_browser_img, LV_OBJ_FLAG_HIDDEN);
            sprintf(msg_txt,"Connect to AP");
            sprintf(qr_txt, "WIFI:T:WPA;S:%s;P:%s;", \
                    data_mgr_get_device_name(), data_mgr_get_device_psk());

            sprintf(wechat_msg_txt,"Connect Wi-Fi");        
            sprintf(wechat_qr_txt, "http://wx.hichiptech.com?s=%s&p=%s&ip=%s&st=0&s5g=%c", \
                    data_mgr_get_device_name(), data_mgr_get_device_psk(), HCCAST_HOSTAP_IP, support_5g);       
            break;
        case QR_SCAN_WIFI:
            //sprintf(msg_txt,"Second scan QR code\nSet WiFi network\nhttp://%s", HCCAST_HOSTAP_IP);
            lv_obj_clear_flag(m_browser_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(m_camera_img, LV_OBJ_FLAG_HIDDEN);
            sprintf(msg_txt,"Connect Wi-Fi");
            sprintf(qr_txt, "http://%s", HCCAST_HOSTAP_IP);

            sprintf(wechat_msg_txt,"Connect Wi-Fi");
            wifi_ap = data_mgr_get_wifi_info(app_get_connecting_ssid());
            sprintf(wechat_qr_txt, "http://wx.hichiptech.com?s=%s&p=%s&ip=%s&st=0&s5g=%c", \
                    data_mgr_get_device_name(), data_mgr_get_device_psk(), HCCAST_HOSTAP_IP, support_5g);
                    
            break;
        case QR_CONFIG:
            lv_obj_clear_flag(m_browser_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(m_camera_img, LV_OBJ_FLAG_HIDDEN);
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
            //sprintf(msg_txt,"Scan QR code\nSet device parameters\nhttp://%s", (const char*)wifi_local_ip_get());
            sprintf(wechat_msg_txt,"Setting");
            wifi_ap = data_mgr_get_wifi_info(app_get_connecting_ssid());
            sprintf(wechat_qr_txt, "http://wx.hichiptech.com?s=%s&p=%s&ip=%s&st=1&s5g=%c", \
                    app_get_connecting_ssid(), wifi_ap->pwd, (const char*)wifi_local_ip_get(), support_5g);
            
            sprintf(msg_txt,"Setting");
            sprintf(qr_txt, "http://%s", (const char*)wifi_local_ip_get());
#else // Ethernet
            sprintf(msg_txt,"Setting");
            sprintf(qr_txt, "http://%s", (const char*)eth_local_ip_get());
#endif
#endif
            break;
        default:
            break;
    }

    if (strlen(msg_txt))
        lv_label_set_text(m_label_qr_msg, msg_txt);

    if (strlen(wechat_msg_txt))
        lv_label_set_text(m_wechat_msg, wechat_msg_txt);     

    if (strlen(qr_txt))
        lv_qrcode_update(m_cast_qr, qr_txt, strlen(qr_txt));
        
    if (strlen(wechat_qr_txt))
        lv_qrcode_update(m_wechat_qr, wechat_qr_txt, strlen(wechat_qr_txt));
        
}

static void win_cast_no_wifi_device_show()
{
    //lv_label_set_text(m_label_local_ssid, "No WiFi device");
    lv_label_set_text_fmt(m_label_local_ssid, "%s %s", LV_SYMBOL_CLOSE, "No WiFi device");

    lv_label_set_text(m_label_ip, "");
    //lv_label_set_text(m_label_connect_state, "No connection");
    lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_CLOSE, "No connection");
    lv_label_set_text(m_label_wifi_mode, "");
    lv_label_set_text(lv_obj_get_child(m_label_wifi_ssid, 0), "");
    lv_label_set_text(lv_obj_get_child(m_label_wifi_ssid, 1), "");
    lv_label_set_text(m_label_password, "");

    win_cast_update_qr_code(QR_CLEAR);
}

static bool connect_show = 0;
static void connect_timer_cb(lv_timer_t * t)
{
    if (!win_root_obj)
        return;

    if (connect_show)
        lv_obj_add_flag(m_label_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_clear_flag(m_label_wifi_ssid, LV_OBJ_FLAG_HIDDEN);

    connect_show = !connect_show;
}

static void active_connect_timer(bool active)
{
    if (NULL == m_connect_timer)
        m_connect_timer = lv_timer_create(connect_timer_cb, 800, NULL);

    if (active)
    {
        lv_timer_resume(m_connect_timer);
    }
    else
    {
        lv_timer_pause(m_connect_timer);
        lv_obj_clear_flag(m_label_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
    }
}

//The first time enter screen application, WiFi should be station mode
//if there is wifi ap information in data node.
static volatile int m_first_flag = 1;
static void win_cast_connect_state_upate(bool force_station)
{
    char show_txt[64] = {0};
    int station_mode = 0;

    active_connect_timer(false);
    lv_label_set_text(m_label_state_msg, "");

#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
    if (!network_wifi_module_get())
    {
        win_cast_no_wifi_device_show();
        return;
    }

    //lv_label_set_text(m_label_local_ssid, data_mgr_get_device_name());
    lv_label_set_text_fmt(m_label_local_ssid, "SSID: %s", data_mgr_get_device_name());

    //lv_label_set_text(m_label_password, "12345678");
    lv_label_set_text_fmt(m_label_password, "Password: %s", data_mgr_get_device_psk());

    if(m_first_flag)
    {
        hccast_wifi_ap_info_t wifi_ap;
        if (data_mgr_wifi_ap_get(&wifi_ap))
        {
            station_mode = 1;
        }
    }
    else
    {
        char *cur_ssid = NULL;
        bool ssid_available = false;
        cur_ssid = app_get_connecting_ssid();
        if (strlen(cur_ssid))
        {
            ssid_available = true;
        }

        if (app_get_wifi_connect_status()){
            station_mode = 1;
        }else{
            if (ssid_available){
                station_mode = 1;
            }
        }
    }
#else
    station_mode = 1;
#endif // WIFI_SUPPORT
#else // NO NETWORK_SUPPORT
    win_cast_no_wifi_device_show();
    return;
#endif // NETWORK_SUPPORT

#ifdef NETWORK_SUPPORT
    m_first_flag = 0;

    if (force_station)
        station_mode = 1;

    printf("%s(), line:%d, force_station:%d, station_mode:%d\n", __func__, __LINE__, force_station, station_mode);

    if (station_mode)
    {
#ifdef WIFI_SUPPORT
        //connect to wifi
        char *cur_ssid = NULL;
        cur_ssid = app_get_connecting_ssid();
        if (strlen(cur_ssid))
        {
            printf("cur_ssid: %s\n", cur_ssid);
                    
            if (app_wifi_is_limited_internet())
            {
                lv_obj_set_style_text_font(lv_obj_get_child(m_label_wifi_ssid, 0), &wifi_font_limited, 0);
                lv_label_set_text_fmt(lv_obj_get_child(m_label_wifi_ssid, 0), "%s", SYMBOL_WIFI_LIMITED);
            }
            else
            {
                lv_label_set_text_fmt(lv_obj_get_child(m_label_wifi_ssid, 0), "%s", LV_SYMBOL_WIFI);
            }

            lv_label_set_text_fmt(lv_obj_get_child(m_label_wifi_ssid, 1), " %s", cur_ssid);
        }

        char *local_ip = wifi_local_ip_get();
#else // Ethernet
        lv_label_set_text_fmt(m_label_local_ssid, "%s %s", LV_SYMBOL_CLOSE, "No WiFi device");
        lv_label_set_text_fmt(lv_obj_get_child(m_label_wifi_ssid, 0), "%s", LV_SYMBOL_WIFI);
        lv_label_set_text(lv_obj_get_child(m_label_wifi_ssid, 1), " Ethernet");
        char *local_ip = eth_local_ip_get();
#endif // WIFI_SUPPORT

        sprintf(show_txt, "IP: %s", local_ip);
        if (local_ip[0])
        {
            lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_OK, "Connected");
            lv_label_set_text_fmt(m_label_ip, "%s %s", LV_SYMBOL_HOME, show_txt);
            win_cast_update_qr_code(QR_CONFIG);
        }
        else
        {
            lv_label_set_text(m_label_ip, "");
            lv_label_set_text(m_label_state_msg, "WiFi connecting ...");
            active_connect_timer(true);
            lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_CLOSE, "No connection");
            win_cast_update_qr_code(QR_CLEAR);
        }

#ifdef WIFI_SUPPORT
        lv_label_set_text(m_label_wifi_mode, "WiFi mode: Station");
#else // Ethernet
        lv_label_set_text(m_label_wifi_mode, "ETH mode");
#endif
    }
    else
    {
#ifdef WIFI_SUPPORT
        lv_label_set_text(lv_obj_get_child(m_label_wifi_ssid, 0), "");
        lv_label_set_text(lv_obj_get_child(m_label_wifi_ssid, 1), "");
        lv_label_set_text(m_label_wifi_mode, "WiFi mode: AP");
        int connected_cnt = hostap_get_connect_count();
        if ( connected_cnt > 0)
        {
            //AP mode, phone has connected.
            sprintf(show_txt, "IP: %s", HCCAST_HOSTAP_IP);
            //lv_label_set_text(m_label_ip, show_txt);
            lv_label_set_text_fmt(m_label_ip, "%s %s", LV_SYMBOL_HOME, show_txt);
            //lv_label_set_text(m_label_connect_state, "Connected");
            lv_label_set_text_fmt(m_label_connect_state, "%s [%d] %s", LV_SYMBOL_OK, connected_cnt, "Connected");
            win_cast_update_qr_code(QR_SCAN_WIFI);
        }
        else
#endif
        {
            lv_label_set_text(m_label_ip, "");
            lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_CLOSE, "No connection");
            win_cast_update_qr_code(QR_CONNECT_AP);
        }
    }
#endif
}

void win_cast_demo_show()
{
    char demo_content[32] = {0};
    int cast_is_demo = 0;
    sys_data_t *sys_data = data_mgr_sys_get();;
    
    if (cast_air_is_demo())
    {
        cast_is_demo = 1;
        strncat(demo_content, "a", sizeof(demo_content));
    }

    if (cast_dial_is_demo())
    {
        cast_is_demo = 1;
        strncat(demo_content, "d", sizeof(demo_content));
    }

    if (cast_um_is_demo())
    {
        cast_is_demo = 1;
        strncat(demo_content, "u", sizeof(demo_content));
    }

    if (cast_is_demo)
        lv_label_set_text_fmt(m_label_demo, "demo(%s): %s", demo_content, sys_data->product_id);
    else
        lv_label_set_text_fmt(m_label_demo, "%s", sys_data->product_id);
}


#ifdef AIRP2P_SUPPORT
static int win_cast_airp2p_state_update()
{
    app_data_t  *app_data = data_mgr_app_get();
    
    if (!network_wifi_module_get())
    {
        lv_label_set_text_fmt(m_label_local_ssid, "%s %s", LV_SYMBOL_CLOSE, "No WiFi device");
        lv_label_set_text(m_label_password, "");
    }
    else
    {
        lv_label_set_text_fmt(m_label_local_ssid, "SSID: %s_itv_p2p", data_mgr_get_device_name());
        lv_label_set_text_fmt(m_label_password, "Pin: %s", app_data->airp2p_pin);
    }

    return 0;
}

static int win_cast_airp2p_open()
{
    lv_obj_t *parent_obj = win_root_obj;
    sys_data_t *sys_data = data_mgr_sys_get();

#if CASTING_CLOSE_FB_SUPPORT	    
        api_osd_show_onoff(true);
#endif

    lv_style_init(&m_large_text_style);
    lv_style_init(&m_mid_text_style);
    lv_style_init(&m_small_text_style);
    lv_style_set_text_font(&m_large_text_style, FONT_SIZE_LARGE);
    lv_style_set_text_font(&m_mid_text_style, FONT_SIZE_MID);
    lv_style_set_text_font(&m_small_text_style, FONT_SIZE_SMALL);
    lv_style_set_text_color(&m_large_text_style, COLOR_WHITE);
    lv_style_set_text_color(&m_mid_text_style, COLOR_WHITE);
    lv_style_set_text_color(&m_small_text_style, COLOR_WHITE);

    m_label_local_ssid = lv_label_open(parent_obj, LABEL_AIRP2P_SSID_X, LABEL_AIRP2P_SSID_Y, LABEL_AIRP2P_SSID_W, NULL, &m_mid_text_style);
    lv_label_set_long_mode(m_label_local_ssid, LV_LABEL_LONG_DOT);
    m_label_password = lv_label_open(parent_obj, LABEL_AIRP2P_PWD_X, LABEL_AIRP2P_PWD_Y, LABEL_AIRP2P_PWD_W, NULL, &m_mid_text_style);

    m_label_demo = lv_label_open(parent_obj, 0, 0, 0, NULL, &m_mid_text_style);
    lv_obj_align(m_label_demo, LV_ALIGN_BOTTOM_LEFT, 20, -20);

    win_cast_demo_show();

    m_label_version = lv_label_open(parent_obj, 0, 0, 0,
                                    NULL, &m_mid_text_style);
    lv_label_set_text_fmt(m_label_version, "Ver: %u", sys_data->firmware_version);
    lv_obj_align(m_label_version, LV_ALIGN_BOTTOM_RIGHT, -20, -20);

    m_label_state_msg = lv_label_open(parent_obj, 0, 0, 0, NULL, &m_mid_text_style);
    lv_obj_align(m_label_state_msg, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_label_set_text(m_label_state_msg, "");

    if(hccast_air_audio_state_get())
    {
        lv_label_set_text(m_label_state_msg, "Aircast is playing music");
    }

    win_cast_airp2p_state_update();
    
    m_win_cast_open = true;

    return API_SUCCESS;

}
#endif

static int win_cast_open(void *arg)
{
    win_root_obj = lv_obj_create(lv_scr_act());
    sys_data_t *sys_data;
    osd_draw_background(win_root_obj, true);
    lv_obj_clear_flag(win_root_obj, LV_OBJ_FLAG_SCROLLABLE);
    win_exit_to_cast_root_by_key_set(false);
    
    printf("%s(), line: %d!\n", __func__, __LINE__);

    sys_data = data_mgr_sys_get();
           
   api_set_key_rotate_enable(0);      
   
#ifdef AIRP2P_SUPPORT
    app_data_t  *app_data = data_mgr_app_get();
    if(app_data->airp2p_en)
    {   
        return win_cast_airp2p_open();
    }
#endif

    api_logo_show(NULL);
#if CASTING_CLOSE_FB_SUPPORT	    
    api_osd_show_onoff(true);
#endif

//    lv_demo_music();
    lv_obj_t *parent_obj = win_root_obj;

    lv_style_init(&m_large_text_style);
    lv_style_init(&m_mid_text_style);
    lv_style_init(&m_small_text_style);
    lv_style_set_text_font(&m_large_text_style, FONT_SIZE_LARGE);
    lv_style_set_text_font(&m_mid_text_style, FONT_SIZE_MID);
    lv_style_set_text_font(&m_small_text_style, FONT_SIZE_SMALL);
    lv_style_set_text_color(&m_large_text_style, COLOR_WHITE);
    lv_style_set_text_color(&m_mid_text_style, COLOR_WHITE);
    lv_style_set_text_color(&m_small_text_style, COLOR_WHITE);

    // lv_style_set_text_color(&m_mid_text_style, COLOR_WHITE);
    // lv_style_set_text_font(&m_mid_text_style, FONT_SIZE_MID);

#ifdef USBMIRROR_SUPPORT
    lv_obj_t *usb_wire = lv_label_create(parent_obj);
    lv_obj_set_pos(usb_wire, LABEL_LOCAL_SSID_X - 80, LABEL_LOCAL_SSID_Y);
    lv_obj_set_style_text_font(usb_wire, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(usb_wire, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_label_set_text_fmt(usb_wire, "%s ", LV_SYMBOL_USB);
#endif
    m_label_local_ssid = lv_label_open(parent_obj, LABEL_LOCAL_SSID_X, LABEL_LOCAL_SSID_Y, LABEL_LOCAL_SSID_W + LABEL_LOCAL_SSID_EXTRA_W, NULL, &m_mid_text_style);
    lv_label_set_long_mode(m_label_local_ssid, LV_LABEL_LONG_DOT);

    m_label_wifi_ssid = lv_label_open(parent_obj, LABEL_WIFI_SSID_X, LABEL_WIFI_SSID_Y, LABEL_WIFI_SSID_W, NULL, &m_mid_text_style);
    lv_label_set_long_mode(m_label_wifi_ssid, LV_LABEL_LONG_DOT);

    lv_obj_set_flex_flow(m_label_wifi_ssid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_label_wifi_ssid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *wifi_level = lv_label_create(m_label_wifi_ssid);
    lv_obj_add_style(wifi_level, &m_mid_text_style, 0);
    lv_label_set_text(wifi_level, "");
    lv_obj_t *ssid = lv_label_create(m_label_wifi_ssid);
    lv_label_set_long_mode(ssid, LV_LABEL_LONG_DOT);
    lv_obj_add_style(ssid, &m_mid_text_style, 0);
    lv_label_set_text(ssid, "");

    m_label_password = lv_label_open(parent_obj, LABEL_LOCAL_PWD_X, LABEL_LOCAL_PWD_Y, LABEL_LOCAL_PWD_W, NULL, &m_mid_text_style);
    m_label_ip = lv_label_open(parent_obj, LABEL_LOCAL_SSID_X, LABEL_LOCAL_PWD_Y+40, LABEL_LOCAL_SSID_W, NULL, &m_mid_text_style);
    m_label_connect_state = lv_label_open(parent_obj, LABEL_LOCAL_PWD_X, LABEL_LOCAL_PWD_Y+40, LABEL_LOCAL_PWD_W, NULL, &m_mid_text_style);
    m_label_wifi_mode = lv_label_open(parent_obj, LABEL_LOCAL_PWD_X, LABEL_LOCAL_PWD_Y+40*2, LABEL_LOCAL_PWD_W, NULL, &m_mid_text_style);
    //the ending user may not need to know wifi mode, so hide here.
    lv_obj_add_flag(m_label_wifi_mode, LV_OBJ_FLAG_HIDDEN);

    m_label_qr_msg = lv_label_open(parent_obj, QR_MSG_X, QR_MSG_Y, QR_MSG_W, NULL, &m_small_text_style);

    m_label_demo = lv_label_open(parent_obj, 0, 0, 0, NULL, &m_mid_text_style);
    lv_obj_align(m_label_demo, LV_ALIGN_BOTTOM_LEFT, 20, -20);

    win_cast_demo_show();

    m_label_version = lv_label_open(parent_obj, 0, 0, 0,
                                    NULL, &m_mid_text_style);
    lv_label_set_text_fmt(m_label_version, "Ver: %u", sys_data->firmware_version);
    lv_obj_align(m_label_version, LV_ALIGN_BOTTOM_RIGHT, -20, -20);

    m_label_state_msg = lv_label_open(parent_obj, 0, 0, 0, NULL, &m_mid_text_style);
    lv_obj_align(m_label_state_msg, LV_ALIGN_BOTTOM_MID, 0, -20);

#if 0
//just test PNG decode
    lv_fs_if_init();
    lv_png_init();
    lv_bmp_init();
    lv_split_jpeg_init();

    lv_obj_t *img_png;
    img_png = lv_img_create(win_root_obj);
    //lv_img_set_src(img_png,  "S\\hc_cast\\ui_rsc\\images\\mainmenu\\setting_256x256_new.jpg");
    //lv_img_set_src(img2,  ".\\hc_cast\\ui_rsc\\images\\mainmenu\\im_iptv.png");
    //lv_img_set_src(img_png,  "S: /tmp/bb.bmp");

//    lv_img_set_src(img_png,  "S/tmp/2.jpg");

    //decode png
    //lv_img_set_src(img_png,  "/tmp/im_iptv_b.png");

//    lv_img_set_src(img_png,  "S:/tmp/2.jpg");

    lv_obj_set_pos(img_png, 80, 80);
#endif

    m_camera_img = lv_img_create(parent_obj);
    lv_img_set_src(m_camera_img, &camera);
    lv_obj_set_pos(m_camera_img, QR_MSG_BMP_X, QR_MSG_BMP_Y);

    m_guide_img = lv_img_create(parent_obj);
    lv_img_set_src(m_guide_img, &wechat);
    lv_obj_set_pos(m_guide_img, QR_GUIDE_BMP_X, QR_GUIDE_BMP_Y);

    m_wechat_img = lv_img_create(parent_obj);
    lv_img_set_src(m_wechat_img, &wechat);
    lv_obj_set_pos(m_wechat_img, QR_WECHAT_BMP_X, QR_WECHAT_BMP_Y);

    m_browser_img = lv_img_create(parent_obj);
    lv_img_set_src(m_browser_img, &browser);
    lv_obj_set_pos(m_browser_img, QR_MSG_BMP_X, QR_MSG_BMP_Y);

    //QR code init
    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);
    m_cast_qr = lv_qrcode_create(parent_obj, QR_BOX_W, fg_color, bg_color);
    lv_obj_set_pos(m_cast_qr, QR_BOX_X, QR_BOX_Y);
    /*Add a border with bg_color(white: 0)*/
    lv_obj_set_style_border_color(m_cast_qr, bg_color, 0);
    lv_obj_set_style_border_width(m_cast_qr, 3, 0);

    //Wechat
    m_wechat_qr = lv_qrcode_create(parent_obj, QR_BOX_W, fg_color, bg_color);
    lv_obj_set_pos(m_wechat_qr, QR_WECHAT_X, QR_WECHAT_Y);
    /*Add a border with bg_color(white: 0)*/
    lv_obj_set_style_border_color(m_wechat_qr, bg_color, 0);
    lv_obj_set_style_border_width(m_wechat_qr, 3, 0);
    m_wechat_msg = lv_label_open(parent_obj, QR_WECHAT_MSG_X, QR_WECHAT_MSG_Y, QR_MSG_W, NULL, &m_small_text_style);

    //User guide.
    m_guide_qr = lv_qrcode_create(parent_obj, QR_BOX_W, fg_color, bg_color);
    lv_obj_set_pos(m_guide_qr, QR_GUIDE_X, QR_GUIDE_Y);
    /*Add a border with bg_color(white: 0)*/
    lv_obj_set_style_border_color(m_guide_qr, bg_color, 0);
    lv_obj_set_style_border_width(m_guide_qr, 3, 0);
    lv_qrcode_update(m_guide_qr, "http://hccast.hichiptech.com/doc/cn", strlen("http://hccast.hichiptech.com/doc/cn"));
    m_guide_msg = lv_label_open(parent_obj, QR_GUIDE_MSG_X, QR_GUIDE_MSG_Y, QR_MSG_W, "User's manual", &m_small_text_style);

    win_cast_connect_state_upate(false);

    if (CAST_DIAL_CONN_CONNECTING == cast_dial_connect_state())
    {
        lv_label_set_text(m_label_state_msg, "YouTube connecting ...");
    }
    else if (CAST_DIAL_CONN_CONNECTED == cast_dial_connect_state())
    {
        lv_label_set_text(m_label_state_msg, "Your phone have connected!");
    }
    else if(hccast_air_audio_state_get())
    {
        lv_label_set_text(m_label_state_msg, "Aircast is playing music");
    }
    
#ifdef USBMIRROR_SUPPORT
    if (cast_um_dev_is_connected())
    {
        lv_label_set_text(m_label_state_msg, "Device connecting ...");
    }
#endif    

    m_win_cast_open = true;

    return API_SUCCESS;
}

static int win_cast_close(void *arg)
{
    printf("%s(), line: %d!\n", __func__, __LINE__);
    lv_obj_del(win_root_obj);
    // lv_obj_add_flag(lv_scr_act(), LV_OBJ_FLAG_HIDDEN);
    // lv_obj_add_flag(m_img_phone_connect, LV_OBJ_FLAG_HIDDEN);
    // lv_obj_add_flag(m_img_dongle_connect, LV_OBJ_FLAG_HIDDEN);
    win_msgbox_msg_close();

    //api_osd_off_time(1000); // move to dlna menu
    api_logo_off();

    if (m_connect_timer)
    {
        lv_timer_pause(m_connect_timer);
        lv_timer_del(m_connect_timer);
    }
    m_connect_timer = NULL;

    m_win_cast_open = false;
    win_root_obj = NULL;

#ifdef AIRP2P_SUPPORT    
    api_set_key_rotate_enable(1);
#endif
    
    //m_first_flag = 1;
    return API_SUCCESS;
}

static win_ctl_result_t win_cast_msg_proc(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;
    win_des_t *cur_win = NULL;
    int cur_scene;
    sys_data_t *sys_data;
    sys_data = data_mgr_sys_get();
    app_data_t  *app_data = data_mgr_app_get();

    switch (ctl_msg->msg_type)
    {
        case MSG_TYPE_CAST_DLNA_START:
        case MSG_TYPE_CAST_AIRCAST_START:
        case MSG_TYPE_CAST_DIAL_START:
            cur_win = &g_win_dlna_play;
            cur_win->param = (void*)(((ctl_msg->msg_type&0xFFFF) << 16) | (ctl_msg->msg_code&0xFFFF));
            menu_mgr_push(cur_win);
            ret = WIN_CTL_PUSH_CLOSE;
            break;
        case MSG_TYPE_CAST_AIRMIRROR_START:
        case MSG_TYPE_CAST_MIRACAST_START:
            cur_win = &g_win_cast_play;
            cur_win->param = (void*)ctl_msg->msg_type;
            menu_mgr_push(cur_win);
            ret = WIN_CTL_PUSH_CLOSE;
            break;
        case MSG_TYPE_CAST_AUSB_START:
        case MSG_TYPE_CAST_IUSB_START:
            win_msgbox_msg_close();
            cur_win = &g_win_um_play;
            cur_win->param = (void*)ctl_msg->msg_type;
            menu_mgr_push(cur_win);
            ret = WIN_CTL_PUSH_CLOSE;
            break;
        case MSG_TYPE_NETWORK_WIFI_CONNECTED:
        case MSG_TYPE_NETWORK_WIFI_DISCONNECTED:
        case MSG_TYPE_NETWORK_DEVICE_BE_CONNECTED:
        case MSG_TYPE_NETWORK_DEVICE_BE_DISCONNECTED:
            win_cast_connect_state_upate(false);
            break;
        case MSG_TYPE_NETWORK_WIFI_MAY_LIMITED:
        {
            char *cur_ssid = NULL;
            cur_ssid = app_get_connecting_ssid();
            lv_obj_set_style_text_font(lv_obj_get_child(m_label_wifi_ssid, 0), &wifi_font_limited, 0);
            lv_label_set_text_fmt(lv_obj_get_child(m_label_wifi_ssid, 0), "%s", SYMBOL_WIFI_LIMITED);
            break;
        }
        case MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL:
        {
            win_msgbox_msg_open("Connect WiFi fail!", 5000, NULL, NULL);
            win_cast_connect_state_upate(false);
            break;
        }
        case MSG_TYPE_NETWORK_WIFI_CONNECTING:
            win_cast_connect_state_upate(true);
            break;
        case MSG_TYPE_NETWORK_WIFI_SCANNING:
            lv_label_set_text(m_label_state_msg, "WiFi is scanning ...");
            break;
        case MSG_TYPE_NETWORK_WIFI_SCAN_DONE:
            lv_label_set_text(m_label_state_msg, "");
            break;
        case MSG_TYPE_CAST_MIRACAST_CONNECTING:
            lv_label_set_text(m_label_state_msg, "Miracast is connecting ...");
            break;
        case MSG_TYPE_CAST_MIRACAST_CONNECTED:
            lv_label_set_text_fmt(m_label_state_msg, "Miracast connected(%d), Video starting ...", \
                hccast_wifi_mgr_get_current_freq_p2p());
            break;
        case MSG_TYPE_CAST_MIRACAST_SSID_DONE:
#ifdef USBMIRROR_SUPPORT        
            if (!cast_um_dev_is_connected())
#endif            
                lv_label_set_text(m_label_state_msg, "");
            break;
        case MSG_TYPE_USB_WIFI_PLUGOUT:
            //stop current playing? stop http
#ifdef AIRP2P_SUPPORT            
            if(app_data->airp2p_en)
            {
                win_cast_airp2p_state_update();
            }
            else
#endif            
            {
                win_cast_no_wifi_device_show();
            }    
            break;
        case MSG_TYPE_AIR_INVALID_CERT:
        case MSG_TYPE_CAST_DIAL_INVALID_CERT:
            win_cast_demo_show();
            break;
        case MSG_TYPE_CAST_AIRCAST_AUDIO_START:
            lv_label_set_text(m_label_state_msg, "Aircast is playing music");
            break;
        case MSG_TYPE_CAST_AIRCAST_AUDIO_STOP:
#ifdef USBMIRROR_SUPPORT        
            if (!cast_um_dev_is_connected())
#endif            
                lv_label_set_text(m_label_state_msg, "");
            break;
        case MSG_TYPE_USB_UPGRADE:
        case MSG_TYPE_NET_UPGRADE:
            cur_win = &g_win_upgrade;
            cur_win->param = (void*)(ctl_msg->msg_type);
            menu_mgr_push(cur_win);
            ret = WIN_CTL_PUSH_CLOSE;
            break;
        case MSG_TYPE_NETWORK_DEV_NAME_SET:
            //lv_label_set_text(m_label_local_ssid, data_mgr_get_device_name());
            break;
        case MSG_TYPE_NETWORK_CONNECTING:
            break;
        case MSG_TYPE_NETWORK_INIT_OK:
            break;
        case MSG_TYPE_NETWORK_WIFI_PWD_WRONG:
            break;
        case MSG_TYPE_NETWORK_DHCP_ON:
            //start up web server.
            //webs_start();

            //start up cast service.

            break;
        case MSG_TYPE_HDMI_TX_CHANGED:
            cur_scene = hccast_get_current_scene();
            if((cur_scene != HCCAST_SCENE_AIRCAST_PLAY) && (cur_scene != HCCAST_SCENE_AIRCAST_MIRROR))
            {
                if(hccast_air_service_is_start())
                {
                    hccast_air_service_stop();
                    hccast_air_service_start();
                }
            }
            break;
        case MSG_TYPE_AIR_HOSTAP_SKIP_URL:
        case MSG_TYPE_DLNA_HOSTAP_SKIP_URL:
            win_msgbox_msg_open("Please connect WiFi first!", 3000, NULL, NULL);
            break;
#ifdef AIRP2P_SUPPORT            
        case MSG_TYPE_AIRP2P_READY:
            win_cast_airp2p_state_update();
            break;
#endif
#ifdef DIAL_SUPPORT
        case MSG_TYPE_CAST_DIAL_CONNECTING:
            lv_label_set_text(m_label_state_msg, "YouTube connecting ...");
            break;
        case MSG_TYPE_CAST_DIAL_CONNECTED:
            lv_label_set_text(m_label_state_msg, "Your phone have connected!");
            break;
        case MSG_TYPE_CAST_DIAL_CONNECTED_FAILED:
        case MSG_TYPE_CAST_DIAL_DISCONNECTED:
        case MSG_TYPE_CAST_DIAL_DISCONNECTED_ALL:
#ifdef USBMIRROR_SUPPORT        
            if (!cast_um_dev_is_connected())
#endif            
                lv_label_set_text(m_label_state_msg, "");
            break;
#endif
        case MSG_TYPE_CAST_AUSB_DEV_ADD:
        case MSG_TYPE_CAST_IUSB_DEV_ADD:
            lv_label_set_text(m_label_state_msg, "Device connecting ...");
            break;
        case MSG_TYPE_CAST_IUSB_NEED_TRUST:
            win_msgbox_msg_open("Please select \"Trust\" to \nstart mirror cast", 5000, NULL, NULL);
            break;
        case MSG_TYPE_CAST_AUSB_DEV_REMOVE:
        case MSG_TYPE_CAST_IUSB_DEV_REMOVE:
            lv_label_set_text(m_label_state_msg, "");
            win_msgbox_msg_close();
            break;
            
        default:
            break;
    }
    return ret;
}

static win_ctl_result_t win_cast_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;

    if (ctl_msg->msg_type == MSG_TYPE_KEY)
    {
        //remote or pan key process
    }
    else if (ctl_msg->msg_type > MSG_TYPE_KEY)
    {
        ret = win_cast_msg_proc(arg1, arg2);
    }
    else
    {
        ret = WIN_CTL_SKIP;
    }
    return ret;

}

//Exit from dlna/miracast/aircast to cast ui by remote key.
//This cast, do not need to wait cast UI open before stop dlna/miracast/aircast.
void win_exit_to_cast_root_by_key_set(bool exit_by_key)
{
    m_exit_to_cast_by_key = exit_by_key;
}

bool win_exit_to_cast_root_by_key_get(void)
{
    return m_exit_to_cast_by_key;
}

bool win_cast_root_wait_open(uint32_t timeout)
{
    uint32_t count;

    if (m_exit_to_cast_by_key)
        return true;

    count = timeout/20;

    while(count--)
    {
        if (m_win_cast_open)
            break;
        api_sleep_ms(20);
    }
    printf("%s(), m_win_cast_open(%d):%d\n", __func__, m_win_cast_open, count);
    return m_win_cast_open;
}

win_des_t g_win_cast_root =
{
    .open = win_cast_open,
    .close = win_cast_close,
    .control = win_cast_control,
};

