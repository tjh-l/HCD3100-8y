/*
win_cast_root.c
 */
#include "app_config.h"

#ifdef CAST_SUPPORT// WIFI_SUPPORT

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
#include <hccast/hccast_net.h>

#include <hcfota.h>
#include "screen.h"
#include "cast_api.h"
#include "win_cast_root.h"
#include "setup.h"

//#include "../lv_drivers/display/fbdev.h"

//#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"
#include "network_api.h"
//#include "data_mgr.h"
#include "factory_setting.h"
//#include "mul_lang_text.h"
#include "hcstring_id.h"
#include "../../app_config.h"

//#define SHOW_VERSION
#ifdef LVGL_RESOLUTION_240P_SUPPORT

#define IMG_PHONE_X  42
#define IMG_PHONE_Y  65

#define IMG_DONGLE_X  68
#define IMG_DONGLE_Y   72

#define LABEL_LOCAL_SSID_W   240
#define LABEL_LOCAL_SSID_X  ((OSD_MAX_WIDTH-LABEL_LOCAL_SSID_W) >> 1) //480
#define LABEL_LOCAL_SSID_Y   5
#define LABEL_LOCAL_SSID_EXTRA_W 4

#define LABEL_WIFI_SSID_X  LABEL_LOCAL_SSID_X//5
#define LABEL_WIFI_SSID_Y  25
#define LABEL_WIFI_SSID_W_PCT  50//280
#define LBAEL_WIFI_SSID_H_PCT 7

#define LABEL_LOCAL_PWD_X  LABEL_LOCAL_SSID_X + 150
#define LABEL_LOCAL_PWD_Y   LABEL_LOCAL_SSID_Y
#define LABEL_LOCAL_PWD_W   200

#define LABEL_VERSION_X  LABEL_WIFI_SSID_X
#define LABEL_VERSION_Y  (OSD_MAX_HEIGHT-10)
#define LABEL_VERSION_W   37

#define AIR_DEMO_X	(OSD_MAX_WIDTH-AIR_DEMO_W)//30
#define AIR_DEMO_Y	LABEL_VERSION_Y
#define AIR_DEMO_W 24

#define AIR_STATUS_W 50
#define AIR_STATUS_X	((OSD_MAX_WIDTH-AIR_STATUS_W)/2)
#define AIR_STATUS_Y	LABEL_VERSION_Y



#define LABEL_CONNECT_MSG_W   200
#define LABEL_CONNECT_MSG_X  ((OSD_MAX_WIDTH-LABEL_CONNECT_MSG_W) >> 1) 
#define LABEL_CONNECT_MSG_Y  LABEL_VERSION_Y

#define QR_MSG_X        2
#define QR_MSG_Y        80
#define QR_MSG_W        110

#define QR_BOX_X        QR_MSG_X+2
#define QR_BOX_Y        QR_MSG_Y+50
#define QR_BOX_W        60
#define EN_FONT1 &lv_font_montserrat_10
#define EN_FONT2 &lv_font_montserrat_14
#define EN_FONT3 &lv_font_montserrat_18
#define FONT_3000 SiYuanHeiTi_Light_3500_12_1b 



#else

#define IMG_PHONE_X  336
#define IMG_PHONE_Y  524

#define IMG_DONGLE_X  546
#define IMG_DONGLE_Y   434

#define LABEL_LOCAL_SSID_W   400
#define LABEL_LOCAL_SSID_X  (((OSD_MAX_WIDTH-LABEL_LOCAL_SSID_W) >> 1)-50) //480
#define LABEL_LOCAL_SSID_Y   30
#define LABEL_LOCAL_SSID_EXTRA_W 10


#define LABEL_WIFI_SSID_X  LABEL_LOCAL_SSID_X//40
#define LABEL_WIFI_SSID_Y  110
#define LABEL_WIFI_SSID_W_PCT  40//280
#define LBAEL_WIFI_SSID_H_PCT 6

#define LABEL_LOCAL_PWD_X  (LABEL_LOCAL_SSID_X+LABEL_LOCAL_SSID_W+100)
#define LABEL_LOCAL_PWD_Y   LABEL_LOCAL_SSID_Y
#define LABEL_LOCAL_PWD_W   300

#define LABEL_VERSION_X  LABEL_WIFI_SSID_X
#define LABEL_VERSION_Y  (OSD_MAX_HEIGHT-80)
#define LABEL_VERSION_W   300

#define AIR_DEMO_X	(OSD_MAX_WIDTH-AIR_DEMO_W)//30
#define AIR_DEMO_Y	LABEL_VERSION_Y
#define AIR_DEMO_W 200

#define AIR_STATUS_W 400
#define AIR_STATUS_X	((OSD_MAX_WIDTH-AIR_STATUS_W)/2)
#define AIR_STATUS_Y	LABEL_VERSION_Y



#define LABEL_CONNECT_MSG_W   600
#define LABEL_CONNECT_MSG_X  ((OSD_MAX_WIDTH-LABEL_CONNECT_MSG_W) >> 1) 
#define LABEL_CONNECT_MSG_Y  LABEL_VERSION_Y

//QR init
#define GAP_W 30
#define GAP_H 10
#define START_X        20

//#define QR_BOX_X        START_X
//#define QR_BOX_Y        170
//#define QR_BOX_W        110

#define QR_MSG_X        20
#define QR_MSG_Y        220
#define QR_MSG_W        300

#define QR_MSG_BMP_X START_X
#define QR_MSG_BMP_Y QR_BOX_Y + QR_BOX_W+GAP_H
#define QR_MSG_BMP_W 16

//#define QR_MSG_X        QR_MSG_BMP_X + QR_MSG_BMP_W + 2
//#define QR_MSG_Y         QR_BOX_Y + QR_BOX_W+GAP_H
//#define QR_MSG_W        500

#define QR_BOX_X        QR_MSG_X+10
#define QR_BOX_Y        QR_MSG_Y+260
#define QR_BOX_W        160

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

#define QR_MSG_FONT (&lv_font_montserrat_14) //small font size

LV_IMG_DECLARE(camera)
LV_IMG_DECLARE(wechat)
LV_IMG_DECLARE(browser)

#endif

typedef enum{
    QR_CLEAR,
    QR_CONNECT_AP,
    QR_SCAN_WIFI,
    QR_CONFIG,
}qr_show_type_t;

lv_obj_t * ui_wifi_cast_root = NULL;
static lv_obj_t *win_root_obj = NULL;

static lv_obj_t *m_label_local_ssid = NULL;
static lv_obj_t *m_label_password = NULL; //also for ip addr

static lv_obj_t *m_wifi_ssid = NULL;

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
static lv_group_t *m_cast_group = NULL;
static lv_timer_t *m_wait_wifi_ready_timer = NULL;
static int m_wifi_ready = 0;
static int m_upgrade_doing = 0;

static bool m_stop_service_exit = false;

static void event_handler(lv_event_t * e);

//Exit from dlna/miracast/aircast to cast ui by remote key.
//This cast, do not need to wait cast UI open before stop dlna/miracast/aircast.
static bool m_exit_to_cast_by_key = false;

//The first time enter screen application, WiFi should be station mode
//if there is wifi ap information in data node.
static volatile int m_first_flag = 1;

//if cast menu is exiting to the main menu, we shuold skip any cast play request.
static bool m_cast_enable_play_request = false;

LV_FONT_DECLARE(cast_font_chn);
#define SYMBOL_WIFI_LIMITED  "\xEE\xA6\x94" /*Unicode 0xe994*/
LV_FONT_DECLARE(wifi_font_limited);


static lv_font_t *font_english[] = 
{
    &lv_font_montserrat_18,
    &lv_font_montserrat_22,
    &lv_font_montserrat_28
};

static lv_font_t *font_chn[] = 
{
    &cast_font_chn,
    &cast_font_chn,
    &cast_font_chn,
};

static lv_font_t **cast_font_array[] = 
{
    font_english,
    font_chn,
};

extern const char* get_some_language_str(const char *str, int index);
extern const char *get_string_by_string_id(int str_id, int lang_idx);

bool win_cast_get_m_stop_service_exit(void)
{
	return m_stop_service_exit;
}

void win_cast_set_m_stop_service_exit(bool val)
{
	m_stop_service_exit = val;
}

bool win_cast_get_play_request_flag(void)
{
	return m_cast_enable_play_request;
}

void win_cast_set_play_request_flag(bool val)
{
	m_cast_enable_play_request = val;
}


//font_idx: select the font belong to same language enviriment
static lv_font_t *win_cast_font_get(int font_idx)
{
    lv_font_t **font;
    int lang_id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    font = cast_font_array[lang_id];
    return font[font_idx];
}

static const char *win_cast_string_get(int string_id)
{
    int lang_id = projector_get_some_sys_param(P_OSD_LANGUAGE);
    return get_string_by_string_id(string_id, lang_id);
}

void win_cast_label_font_set(lv_obj_t *label, int font_idx)
{
    lv_font_t *font;
    font = win_cast_font_get(font_idx);
    lv_obj_set_style_text_font(label, font, 0);
}

void win_cast_label_txt_set_fmt(lv_obj_t *label, uint32_t str_id, ...)
{
    char *rsc_str = (char *)api_rsc_string_get(str_id);
    char text_str[MAX_UI_TIPS_STR_LEN] = {0};
    va_list args;
    va_start(args, str_id);
    vsnprintf(text_str, sizeof(text_str), rsc_str, args);
    va_end(args);

    lv_label_set_text(label, text_str);
}

void win_cast_label_txt_set(lv_obj_t *label, uint32_t str_id)
{
    win_cast_label_txt_set_fmt(label, str_id);
}

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

#ifdef WIFI_SUPPORT
static void win_cast_update_qr_code(qr_show_type_t qr_type)
{

#ifndef HTTPD_SERVICE_SUPPORT
    return;
#endif

#if 1//def LVGL_RESOLUTION_240P_SUPPORT
    /*Set data*/
    //const char * data = "https://lvgl.io";
    char qr_txt[128] = {0};
    char msg_txt[128] = {0};

    int str_id = 0;
    lv_obj_clear_flag(m_cast_qr, LV_OBJ_FLAG_HIDDEN);
    switch (qr_type)
    {
        case QR_CLEAR:
            //sprintf(msg_txt,"");
            memset(msg_txt, 0, sizeof(msg_txt));
            lv_obj_add_flag(m_cast_qr, LV_OBJ_FLAG_HIDDEN);
            break;
        case QR_CONNECT_AP:
            str_id = STR_QR_CONNECT_AP;
            //1. 请扫码连接本设备
            sprintf(msg_txt,"First scan QR code\nSet AP network");
            sprintf(qr_txt, "WIFI:T:WPA;S:%s;P:%s;", \
                (char*)projector_get_some_sys_param(P_DEVICE_NAME), (char*)projector_get_some_sys_param(P_DEVICE_PSK));
            break;
        case QR_SCAN_WIFI:
            str_id = STR_QR_SCAN_WIFI;
            //2. 请扫码联网
            sprintf(msg_txt,"Second scan QR code\nSet WiFi network\nhttp://%s", HCCAST_HOSTAP_IP);
            sprintf(qr_txt, "http://%s", HCCAST_HOSTAP_IP);
            break;
        case QR_CONFIG:
            str_id = STR_QR_CONFIG;
            //3. 请扫码联网配置参数
            sprintf(msg_txt,"Scan QR code\nSet device parameters\nhttp://%s", (const char*)app_wifi_local_ip_get());
            sprintf(qr_txt, "http://%s", (const char*)app_wifi_local_ip_get());
            break;
        default:
            break;
    }

    if (strlen(msg_txt))
    {
        //lv_label_set_text(m_label_qr_msg, msg_txt);
        win_cast_label_txt_set(m_label_qr_msg, str_id);
    }
    else
        lv_label_set_text(m_label_qr_msg, "");

    if (strlen(qr_txt))
        lv_qrcode_update(m_cast_qr, qr_txt, strlen(qr_txt));

#else

    /*Set data*/
    //const char * data = "https://lvgl.io";
    char qr_txt[128] = {0};
    char msg_txt[128] = {0};
    char wechat_msg_txt[32] = {0};
    char wechat_qr_txt[128] = {0};
    char support_5g = '0';
    hccast_wifi_ap_info_t * wifi_ap = NULL;

    hccast_wifi_freq_mode_e mode = hccast_wifi_mgr_freq_support_mode();
    if (mode == HCCAST_WIFI_FREQ_MODE_5G)
    {
        support_5g = '1';
    }
    else
    {
        support_5g = '0';
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
                    (char*)projector_get_some_sys_param(P_DEVICE_NAME), (char*)projector_get_some_sys_param(P_DEVICE_PSK));

            sprintf(wechat_msg_txt,"Connect Wi-Fi");        
            sprintf(wechat_qr_txt, "http://wx.hichiptech.com?s=%s&p=%s&ip=%s&st=0&s5g=%c", \
                    (char*)projector_get_some_sys_param(P_DEVICE_NAME), \
                    (char*)projector_get_some_sys_param(P_DEVICE_PSK),  HCCAST_HOSTAP_IP, support_5g);       
            break;
        case QR_SCAN_WIFI:
            //sprintf(msg_txt,"Second scan QR code\nSet WiFi network\nhttp://%s", HCCAST_HOSTAP_IP);
            lv_obj_clear_flag(m_browser_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(m_camera_img, LV_OBJ_FLAG_HIDDEN);
            sprintf(msg_txt,"Connect Wi-Fi");
            sprintf(qr_txt, "http://%s", HCCAST_HOSTAP_IP);

            sprintf(wechat_msg_txt,"Connect Wi-Fi");
            wifi_ap = sysdata_get_wifi_info(app_get_connecting_ssid());
            sprintf(wechat_qr_txt, "http://wx.hichiptech.com?s=%s&p=%s&ip=%s&st=0&s5g=%c", \
                    (char*)projector_get_some_sys_param(P_DEVICE_NAME), \
                    (char*)projector_get_some_sys_param(P_DEVICE_PSK), HCCAST_HOSTAP_IP, support_5g);
                    
            break;
        case QR_CONFIG:
            lv_obj_clear_flag(m_browser_img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(m_camera_img, LV_OBJ_FLAG_HIDDEN);

            //sprintf(msg_txt,"Scan QR code\nSet device parameters\nhttp://%s", (const char*)wifi_local_ip_get());
            sprintf(wechat_msg_txt,"Seting");
            wifi_ap = sysdata_get_wifi_info(app_get_connecting_ssid());
            sprintf(wechat_qr_txt, "http://wx.hichiptech.com?s=%s&p=%s&ip=%s&st=1&s5g=%c", \
                    app_get_connecting_ssid(), wifi_ap->pwd, (const char*)app_wifi_local_ip_get(), support_5g);
            
            sprintf(msg_txt,"Seting");
            sprintf(qr_txt, "http://%s", (const char*)app_wifi_local_ip_get());

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

#endif

}
#endif

static void win_cast_no_wifi_device_show()
{
    //lv_label_set_text(m_label_local_ssid, "No WiFi device");
    lv_label_set_text_fmt(m_label_local_ssid, "%s %s", LV_SYMBOL_CLOSE, "No WiFi device");

    lv_label_set_text(m_label_ip, "");
    //lv_label_set_text(m_label_connect_state, "No connection");
    lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_CLOSE, "No connection");
    lv_label_set_text(m_label_wifi_mode, "");
    lv_label_set_text(lv_obj_get_child(m_wifi_ssid, 0), "");
    lv_label_set_text(lv_obj_get_child(m_wifi_ssid, 1), "");

    lv_label_set_text(m_label_password, "");
#ifdef WIFI_SUPPORT
    win_cast_update_qr_code(QR_CLEAR);
#endif
}

static bool connect_show = 0;
static void connect_timer_cb(lv_timer_t * t)
{
    if (!win_root_obj)
        return;

    if (connect_show){
        lv_obj_add_flag(m_wifi_ssid, LV_OBJ_FLAG_HIDDEN);

    }
    else{
        lv_obj_clear_flag(m_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
    }

    connect_show = !connect_show;
}

static void active_connect_timer(bool active)
{
    if (NULL == m_connect_timer)
        m_connect_timer = lv_timer_create(connect_timer_cb, 800, NULL);

    if (active){
        lv_timer_resume(m_connect_timer);
    }
    else{
        lv_timer_pause(m_connect_timer);
        lv_obj_clear_flag(m_wifi_ssid, LV_OBJ_FLAG_HIDDEN);
    }
}

#ifdef WIFI_SUPPORT
static void wifi_ready_timer_cb(lv_timer_t * t)
{  
    if (!network_wifi_module_get())
    {
        lv_label_set_text_fmt(m_label_local_ssid, "%s %s", LV_SYMBOL_CLOSE, "No WiFi device");
    }
    
    lv_timer_pause(m_wait_wifi_ready_timer);
    printf("wifi_ready_timer_cb done!\n");
}

static void start_wifi_ready_detect_timer()
{
    if (m_wait_wifi_ready_timer)
    {
        lv_timer_pause(m_wait_wifi_ready_timer);
        lv_timer_del(m_wait_wifi_ready_timer);
        m_wait_wifi_ready_timer = NULL;
    }

    m_wait_wifi_ready_timer = lv_timer_create(wifi_ready_timer_cb, 2000, NULL);
}

static void stop_wifi_ready_detect_timer()
{
    if (m_wait_wifi_ready_timer)
    {
        lv_timer_pause(m_wait_wifi_ready_timer);
        lv_timer_del(m_wait_wifi_ready_timer);
        m_wait_wifi_ready_timer = NULL;
    }
}
#endif

static void win_cast_connect_state_upate(bool force_station)
{
#ifdef WIFI_SUPPORT
    char show_txt[64] = {0};
    int station_mode = 0;

    stop_wifi_ready_detect_timer();
    active_connect_timer(false);
    if (!hccast_wifi_mgr_p2p_get_connect_stat())
       lv_label_set_text(m_label_state_msg, "");

    if (!m_wifi_ready)
    {
        win_cast_no_wifi_device_show();
        lv_label_set_text_fmt(m_label_local_ssid, "%s", "WiFi detecting ...");
        start_wifi_ready_detect_timer();
        return;
    }
    
    lv_label_set_text_fmt(m_label_local_ssid, "SSID: %s", (char*)projector_get_some_sys_param(P_DEVICE_NAME));
    lv_label_set_text_fmt(m_label_password, "Password: %s", (char*)projector_get_some_sys_param(P_DEVICE_PSK));

    char *cur_ssid = NULL;
    bool ssid_available = false;
    cur_ssid = app_get_connecting_ssid();
    if (strlen(cur_ssid))
    {
        ssid_available = true;
    }

    if (app_wifi_connect_status_get()){
        station_mode = 1;
    }else{
        if (ssid_available){
            station_mode = 1;
        }
    }

    m_first_flag = 0;

    if (force_station)
        station_mode = 1;

    printf("%s(), line:%d, force_station:%d, station_mode:%d\n", __func__, __LINE__, force_station, station_mode);

    if (station_mode){
        //connect to wifi
        if (ssid_available)
        {
            printf("cur_ssid: %s\n", cur_ssid);
                    
            if (app_wifi_is_limited_internet())
            {
                lv_obj_set_style_text_font(lv_obj_get_child(m_wifi_ssid, 0), &wifi_font_limited, 0);
                lv_label_set_text_fmt(lv_obj_get_child(m_wifi_ssid, 0), "%s", SYMBOL_WIFI_LIMITED);
            }
            else
            {
                lv_obj_set_style_text_font(lv_obj_get_child(m_wifi_ssid, 0),osd_font_get_by_langid(0,FONT_MID),0);
                lv_label_set_text_fmt(lv_obj_get_child(m_wifi_ssid, 0), "%s", LV_SYMBOL_WIFI);
            }

            lv_label_set_text(lv_obj_get_child(m_wifi_ssid, 1), cur_ssid);

        }


        char *local_ip = app_wifi_local_ip_get();
        sprintf(show_txt, "IP: %s", local_ip);
        if (local_ip[0] && !hccast_wifi_is_connecting()){
            lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_OK, "Connected");
            lv_label_set_text_fmt(m_label_ip, "%s %s", LV_SYMBOL_HOME, show_txt);
            win_cast_update_qr_code(QR_CONFIG);
        }else{
            lv_label_set_text(m_label_ip, "");
            //lv_label_set_text(m_label_state_msg, "WiFi connecting ...");
            win_cast_label_txt_set(m_label_state_msg, STR_WIFI_CONNECTING);

            active_connect_timer(true);
            lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_CLOSE, "No connection");
            win_cast_update_qr_code(QR_CLEAR);
        }


        lv_label_set_text(m_label_wifi_mode, "WiFi mode: Station");


    }else{
        lv_label_set_text(lv_obj_get_child(m_wifi_ssid, 0), "");
        lv_label_set_text(lv_obj_get_child(m_wifi_ssid, 1), "");

        lv_label_set_text(m_label_wifi_mode, "WiFi mode: AP");

        int connected_cnt = hostap_get_connect_count();
        if ( connected_cnt > 0){
        //AP mode, phone has connected.
            sprintf(show_txt, "IP: %s", HCCAST_HOSTAP_IP);
            //lv_label_set_text(m_label_ip, show_txt);
            lv_label_set_text_fmt(m_label_ip, "%s %s", LV_SYMBOL_HOME, show_txt);
            //lv_label_set_text(m_label_connect_state, "Connected");
            lv_label_set_text_fmt(m_label_connect_state, "%s [%d] %s", LV_SYMBOL_OK, 
                connected_cnt, "Connected");

            win_cast_update_qr_code(QR_SCAN_WIFI);
        }else{
            lv_label_set_text(m_label_ip, "");
            lv_label_set_text_fmt(m_label_connect_state, "%s %s", LV_SYMBOL_CLOSE, "No connection");

            win_cast_update_qr_code(QR_CONNECT_AP);
        }
    }
#endif
}

#ifdef WIFI_SUPPORT
static void cast_start_service()
{
    if (network_service_enable_get())
        return;

    win_cast_set_play_request_flag(true);

    // first boot, network_connect_task ongoing
    if (!app_get_wifi_init_done()) {
        goto EXIT;
    }

    if (network_wifi_module_get()) {
        if (app_wifi_connect_status_get()) { // connected
            hccast_start_services();
        }
        else if (hccast_wifi_mgr_is_connecting()) { //connecting
            hccast_service_need_start_flag(1);
        }
        else if (hccast_wifi_mgr_get_connect_status() == 1) { //get ip
            hccast_service_need_start_flag(1);
        }
        else { // ap mode
            app_wifi_switch_work_mode(WIFI_MODE_AP);
            if (app_wifi_config_get()->host_ap_ip_ready) {
                hccast_dlna_service_start();
                hccast_air_service_start();
            }

            hccast_mira_service_start();
        }
    }

EXIT:
#ifdef HTTPD_SERVICE_SUPPORT
    hccast_httpd_service_start();
#endif

    network_service_enable_set(true);
}

void cast_stop_service(void)
{
    if (!m_stop_service_exit)
        return;

    m_stop_service_exit = false;
    network_service_enable_set(false);
    win_cast_set_play_request_flag(false);

#ifdef HTTPD_SERVICE_SUPPORT
    hccast_httpd_service_stop();
#endif

    hccast_stop_services();

    if (WIFI_MODE_AP == app_wifi_get_work_mode() && !m_upgrade_doing)
    {
    #ifdef __HCRTOS__
        hccast_wifi_mgr_hostap_disenable();
        hccast_wifi_mgr_udhcpd_stop();
    #else
        hccast_wifi_mgr_hostap_stop();
    #endif
    }
}

#ifdef MIRACAST_SUPPORT
static int cast_mira_reset(int reset_type)
{
    // first time boot up, if wifi not init, re-init and connect again.
    if (!app_get_wifi_init_done()){    
        network_service_enable_set(true);
        hccast_mira_service_stop();
        network_connect();
        return 0;
    }

    if (network_wifi_module_get())
    {
        if (reset_type){
            network_service_enable_set(true);
            hccast_mira_service_stop();
            app_wifi_reconnect(NULL, NETWORK_WIFI_CONNECTING_TIMEOUT);
        } else {
            //ap mode
            hccast_mira_service_stop();
            app_wifi_switch_work_mode(WIFI_MODE_AP);
            hccast_mira_service_start();
        }
    }
    return 0;
}
#endif // MIRACAST_SUPPORT End
#endif // WIFI_SUPPORT End

static int win_cast_open(void)
{   
    m_stop_service_exit = true;
	m_exit_to_cast_by_key = false;

  #ifdef USB_MIRROR_FAST_SUPPORT
    um_service_wait_exit();
  #endif    

#ifdef WIFI_SUPPORT
    if (!network_wifi_module_get())
    {
        printf("wifi not ready.\n");
        m_wifi_ready = 0;
        win_cast_set_play_request_flag(true);
        network_service_enable_set(true);
  #ifdef HTTPD_SERVICE_SUPPORT
        hccast_httpd_service_start();
  #endif
    }
    else
    {
        m_wifi_ready = 1;
        cast_start_service();
    }
#endif
    win_root_obj = lv_obj_create(ui_wifi_cast_root);
    struct sysdata *sys_data;
    osd_draw_background(win_root_obj, true);
    lv_obj_clear_flag(win_root_obj, LV_OBJ_FLAG_SCROLLABLE);
   	
    m_cast_group = lv_group_create();
    key_set_group(m_cast_group);

    lv_obj_add_event_cb(win_root_obj, event_handler, LV_EVENT_ALL, NULL);    
    lv_group_add_obj(m_cast_group, win_root_obj);
    lv_group_focus_obj(win_root_obj);

    sys_data = &(projector_get_sys_param()->sys_data);

    printf("%s(), line: %d!, fw_ver: 0x%x, product_id:%s\n", 
        __func__, __LINE__, (unsigned int)sys_data->firmware_version, sys_data->product_id);
    set_display_zoom_when_sys_scale();

    api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE);    
    api_logo_show(NULL);
#if CASTING_CLOSE_FB_SUPPORT	    
    api_osd_show_onoff(true);
#endif

//    lv_demo_music();
    lv_obj_t *parent_obj = win_root_obj;

    lv_style_init(&m_large_text_style);
    lv_style_init(&m_mid_text_style);
    lv_style_init(&m_small_text_style);
    lv_style_set_text_font(&m_large_text_style, osd_font_get_by_langid(0,FONT_LARGE));
    lv_style_set_text_font(&m_mid_text_style, osd_font_get_by_langid(0,FONT_MID));
    lv_style_set_text_font(&m_small_text_style, osd_font_get_by_langid(0,FONT_MID));
    lv_style_set_text_color(&m_large_text_style, COLOR_WHITE);
    lv_style_set_text_color(&m_mid_text_style, COLOR_WHITE);
    lv_style_set_text_color(&m_small_text_style, COLOR_WHITE);

    // lv_style_set_text_color(&m_mid_text_style, COLOR_WHITE);
    // lv_style_set_text_font(&m_mid_text_style, FONT_SIZE_MID);

//#ifdef USBMIRROR_SUPPORT
#if 0
    //show usb mirror icon means enter usb mirror and wifi cast in the same UI
    lv_obj_t *usb_wire = lv_label_create(parent_obj);
    lv_obj_set_pos(usb_wire, LABEL_LOCAL_SSID_X - 80, LABEL_LOCAL_SSID_Y);
    lv_obj_set_style_text_font(usb_wire, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(usb_wire, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_label_set_text_fmt(usb_wire, "%s ", LV_SYMBOL_USB);
#endif

    m_label_local_ssid = lv_label_open(parent_obj, LABEL_LOCAL_SSID_X, LABEL_LOCAL_SSID_Y, LABEL_LOCAL_SSID_W+LABEL_LOCAL_SSID_EXTRA_W, NULL, &m_mid_text_style);
    //lv_label_set_long_mode(m_label_local_ssid, LV_LABEL_LONG_DOT);
    lv_label_set_long_mode(m_label_local_ssid, LV_LABEL_LONG_CLIP);



    m_wifi_ssid = lv_obj_create(parent_obj);
    lv_obj_set_size(m_wifi_ssid, LV_PCT(LABEL_WIFI_SSID_W_PCT),LV_PCT(LBAEL_WIFI_SSID_H_PCT));
    lv_obj_set_scrollbar_mode(m_wifi_ssid, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(m_wifi_ssid, LV_OPA_0, 0);
    lv_obj_set_style_text_color(m_wifi_ssid, lv_color_white(), 0);
    lv_obj_align(m_wifi_ssid, LV_ALIGN_TOP_LEFT, LABEL_WIFI_SSID_X, LABEL_WIFI_SSID_Y);
    lv_obj_set_style_border_width(m_wifi_ssid, 0, 0);
    lv_obj_set_style_pad_all(m_wifi_ssid, 0, 0);

    lv_obj_set_flex_flow(m_wifi_ssid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(m_wifi_ssid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_t *wifi_level = lv_label_create(m_wifi_ssid);
    lv_obj_set_style_text_font(wifi_level, osd_font_get(FONT_MID), 0);
    lv_label_set_text(wifi_level, "");
    lv_obj_t *ssid = lv_label_create(m_wifi_ssid);
    lv_label_set_long_mode(ssid, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(ssid, &CHN_MID,0);
    // lv_obj_set_style_text_font(ssid, osd_font_get(FONT_MID),0);
    lv_label_set_text(ssid, "");

    m_label_password = lv_label_open(parent_obj, LABEL_LOCAL_PWD_X, LABEL_LOCAL_PWD_Y, LABEL_LOCAL_PWD_W, NULL, &m_mid_text_style);
    m_label_ip = lv_label_open(parent_obj, LABEL_LOCAL_SSID_X, LABEL_LOCAL_PWD_Y+40, LABEL_LOCAL_SSID_W, NULL, &m_mid_text_style);
    m_label_connect_state = lv_label_open(parent_obj, LABEL_LOCAL_PWD_X, LABEL_LOCAL_PWD_Y+40, LABEL_LOCAL_PWD_W, NULL, &m_mid_text_style);
    m_label_wifi_mode = lv_label_open(parent_obj, LABEL_LOCAL_PWD_X, LABEL_LOCAL_PWD_Y+40*2, LABEL_LOCAL_PWD_W, NULL, &m_mid_text_style);
    //the ending user may not need to know wifi mode, so hide here.
    lv_obj_add_flag(m_label_wifi_mode, LV_OBJ_FLAG_HIDDEN);
    
    m_label_qr_msg = lv_label_open(parent_obj, QR_MSG_X, QR_MSG_Y, QR_MSG_W, NULL, &m_small_text_style);
#if  1
    lv_obj_set_width(m_label_qr_msg, 1000);
    lv_obj_set_height(m_label_qr_msg, 100);
    lv_obj_set_x(m_label_qr_msg, 228);
    lv_obj_set_y(m_label_qr_msg, 274);
    lv_obj_set_align(m_label_qr_msg, LV_ALIGN_LEFT_MID);
    //lv_label_set_text(m_label_qr_msg, "1.Please scan the code of Android device to install software");
    lv_obj_set_style_text_color(m_label_qr_msg, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(m_label_qr_msg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(m_label_qr_msg, osd_font_get(FONT_MID), 0);
#else
    lv_obj_set_style_text_font(m_label_qr_msg, osd_font_get(FONT_SMALL),0);
#endif 
                
    m_label_demo = lv_label_open(parent_obj, 0, 0, 0, NULL, &m_mid_text_style);
    lv_obj_align(m_label_demo, LV_ALIGN_BOTTOM_LEFT, 20, -20);

    if (cast_is_demo())
        lv_label_set_text_fmt(m_label_demo, "demo: %s", sys_data->product_id);
    else
        lv_label_set_text_fmt(m_label_demo, "%s", sys_data->product_id);

    m_label_version = lv_label_open(parent_obj, 0, 0, 0, 
        NULL, &m_mid_text_style);
    lv_label_set_text_fmt(m_label_version, "Ver: %u", (unsigned int)sys_data->firmware_version);
    lv_obj_align(m_label_version, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
#ifndef SHOW_VERSION
    lv_obj_add_flag(m_label_demo, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(m_label_version, LV_OBJ_FLAG_HIDDEN);
#endif

	m_label_state_msg = lv_label_open(parent_obj, 0, 0, 0, NULL, &m_mid_text_style);
    lv_obj_align(m_label_state_msg, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_font(m_label_state_msg, osd_font_get(FONT_MID),0);


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
    
    //QR code init
    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);
    m_cast_qr = lv_qrcode_create(parent_obj, QR_BOX_W, fg_color, bg_color);
    lv_obj_set_pos(m_cast_qr, QR_BOX_X, QR_BOX_Y);
    /*Add a border with bg_color(white: 0)*/
    lv_obj_set_style_border_color(m_cast_qr, bg_color, 0);
    lv_obj_set_style_border_width(m_cast_qr, 3, 0);

#ifndef LVGL_RESOLUTION_240P_SUPPORT
#if 0//def HTTPD_SERVICE_SUPPORT
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

    lv_obj_set_style_text_font(m_label_qr_msg, QR_MSG_FONT, 0);
    
    //Wechat
    m_wechat_qr = lv_qrcode_create(parent_obj, QR_BOX_W, fg_color, bg_color);
    lv_obj_set_pos(m_wechat_qr, QR_WECHAT_X, QR_WECHAT_Y);
    /*Add a border with bg_color(white: 0)*/
    lv_obj_set_style_border_color(m_wechat_qr, bg_color, 0);
    lv_obj_set_style_border_width(m_wechat_qr, 3, 0);
    m_wechat_msg = lv_label_open(parent_obj, QR_WECHAT_MSG_X, QR_WECHAT_MSG_Y, QR_MSG_W, NULL, &m_small_text_style);
    lv_obj_set_style_text_font(m_wechat_msg, QR_MSG_FONT, 0);

    //User guide.
    m_guide_qr = lv_qrcode_create(parent_obj, QR_BOX_W, fg_color, bg_color);
    lv_obj_set_pos(m_guide_qr, QR_GUIDE_X, QR_GUIDE_Y);
    /*Add a border with bg_color(white: 0)*/
    lv_obj_set_style_border_color(m_guide_qr, bg_color, 0);
    lv_obj_set_style_border_width(m_guide_qr, 3, 0);
    lv_qrcode_update(m_guide_qr, "http://hccast.hichiptech.com/doc/cn", strlen("http://hccast.hichiptech.com/doc/cn"));
    m_guide_msg = lv_label_open(parent_obj, QR_GUIDE_MSG_X, QR_GUIDE_MSG_Y, QR_MSG_W, "User's manual", &m_small_text_style);
    lv_obj_set_style_text_font(m_guide_msg, QR_MSG_FONT, 0);
#endif    
#endif

#ifndef HTTPD_SERVICE_SUPPORT
    lv_obj_add_flag(m_cast_qr, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(m_label_qr_msg, LV_OBJ_FLAG_HIDDEN);
#endif

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

    m_win_cast_open = true;

    return API_SUCCESS;
}

static int win_cast_close(void)
{
    printf("%s(), line: %d!\n", __func__, __LINE__);
#ifdef WIFI_SUPPORT
    if(hccast_wifi_mgr_is_connecting() || hccast_wifi_mgr_is_scanning()){
        hccast_wifi_mgr_op_abort();
    }
    cast_stop_service();
    stop_wifi_ready_detect_timer();
#endif
    lv_obj_del(win_root_obj);
    // lv_obj_add_flag(lv_scr_act(), LV_OBJ_FLAG_HIDDEN);
    // lv_obj_add_flag(m_img_phone_connect, LV_OBJ_FLAG_HIDDEN);
    // lv_obj_add_flag(m_img_dongle_connect, LV_OBJ_FLAG_HIDDEN);
	
    api_osd_off_time(500);
    api_logo_off();

	//api_logo_off_no_black();

    if (m_cast_group){
        lv_group_remove_all_objs(m_cast_group);
        lv_group_del(m_cast_group);
        lv_group_set_default(NULL);
    }

    if (m_connect_timer){
        lv_timer_pause(m_connect_timer);
        lv_timer_del(m_connect_timer);
    }
    m_connect_timer = NULL;


    m_win_cast_open = false;
    win_root_obj = NULL;
    m_first_flag = 1;
    m_upgrade_doing = 0;
    
    //recover the dispaly aspect.
    if (m_stop_service_exit)
        api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE);

    win_clear_popup();
    
    return API_SUCCESS;
}


static uint32_t m_cast_play_param;
uint32_t win_cast_play_param()
{
    return m_cast_play_param;
}

void win_cast_set_play_param(uint32_t play_param)
{
    m_cast_play_param = play_param;
}

static void win_cast_msg_proc(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    struct sysdata *sys_data;

    sys_data = &(projector_get_sys_param()->sys_data);

    switch (ctl_msg->msg_type)
    {
#ifdef WIFI_SUPPORT 
    case MSG_TYPE_NETWORK_WIFI_MAY_LIMITED:
        if (network_wifi_module_get())
        {
            lv_obj_set_style_text_font(lv_obj_get_child(m_wifi_ssid, 0), &wifi_font_limited, 0);
            lv_label_set_text_fmt(lv_obj_get_child(m_wifi_ssid, 0), "%s", SYMBOL_WIFI_LIMITED);
        }
        break;
    case MSG_TYPE_CAST_DLNA_START:
    case MSG_TYPE_CAST_AIRCAST_START:
    case MSG_TYPE_CAST_DIAL_START:

        m_stop_service_exit = false;
        m_cast_play_param = (uint32_t)(((ctl_msg->msg_type&0xFFFF) << 16) | (ctl_msg->msg_code&0xFFFF));

    #ifdef DLNA_SUPPORT
        if (ui_cast_dlna){
            _ui_screen_change(ui_cast_dlna,0,0);
        }
    #endif

    	// cur_win = &g_win_dlna_play;
     //    cur_win->param = (void*)(((ctl_msg->msg_type&0xFFFF) << 16) | (ctl_msg->msg_code&0xFFFF));
     //    menu_mgr_push(cur_win);
     //    ret = WIN_CTL_PUSH_CLOSE;

        break;
    case MSG_TYPE_CAST_AIRMIRROR_START:
    case MSG_TYPE_CAST_MIRACAST_START:

#ifdef MIRROR_ES_DUMP_SUPPORT
    {
        extern bool api_mirror_dump_enable_get(char* folder);
        char dump_folder[64];
        if (USB_STAT_MOUNT != mmp_get_usb_stat())
        {
            printf("%s(), line: %d. No disk, disable dump!\n", __func__, __LINE__);
            if (MSG_TYPE_CAST_AIRMIRROR_START == ctl_msg->msg_type)
                hccast_air_es_dump_stop();
            else
                hccast_mira_es_dump_stop();

        } else {

            if (api_mirror_dump_enable_get(dump_folder)){
                if (MSG_TYPE_CAST_AIRMIRROR_START == ctl_msg->msg_type){
                    printf("%s(), line: %d. Statr air ES dump:%s!\n", __func__, __LINE__, dump_folder);
                    hccast_air_es_dump_start(dump_folder);
                }
                else{
                    printf("%s(), line: %d. Statr miracast ES dump:%s!\n", __func__, __LINE__, dump_folder);
                    hccast_mira_es_dump_start(dump_folder);
                }
            } else {
                if (MSG_TYPE_CAST_AIRMIRROR_START == ctl_msg->msg_type)
                    hccast_air_es_dump_stop();
                else
                    hccast_mira_es_dump_stop();
                
            }
        }
    }
#endif    
        m_stop_service_exit = false;
        m_cast_play_param = (uint32_t)ctl_msg->msg_type;
        if (ui_cast_play){
            printf("%s(), line:%d. _ui_screen_change: cast play.\n", __func__, __LINE__);
            _ui_screen_change(ui_cast_play,0,0);
        }

    	break;

#ifdef MIRACAST_SUPPORT
    case MSG_TYPE_CAST_MIRACAST_RESET:
        cast_mira_reset((int)ctl_msg->msg_code);
        break;
#endif // MIRACAST_SUPPORT End

    case MSG_TYPE_NETWORK_WIFI_CONNECTED:
    case MSG_TYPE_NETWORK_WIFI_DISCONNECTED:
    case MSG_TYPE_NETWORK_DEVICE_BE_CONNECTED:
    case MSG_TYPE_NETWORK_DEVICE_BE_DISCONNECTED:
    case MSG_TYPE_NETWORK_WIFI_STATUS_UPDATE:
        m_wifi_ready = 1;
        if (network_wifi_module_get())
            win_cast_connect_state_upate(false);
        break;
#endif // WIFI_SUPPORT End

    case MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL:
        win_msgbox_msg_open(STR_WIFI_CONNECT_ERR, 5000, NULL, NULL);
        win_cast_connect_state_upate(false);
        break;
    case MSG_TYPE_NETWORK_WIFI_CONNECTING:
        m_wifi_ready = 1;
        win_cast_connect_state_upate(true);
        break;
    case MSG_TYPE_NETWORK_WIFI_SCANNING:
        //lv_label_set_text(m_label_state_msg, "WiFi is scanning ...");
        win_cast_label_txt_set(m_label_state_msg, STR_WIFI_SCANNING);
        break;
    case MSG_TYPE_NETWORK_WIFI_SCAN_DONE:
        lv_label_set_text(m_label_state_msg, "");
        break;

    case MSG_TYPE_CAST_MIRACAST_CONNECTING:
        //lv_label_set_text(m_label_state_msg, "Miracast is connecting ...");
        win_cast_label_txt_set(m_label_state_msg, STR_MIRA_CONNECTING);

        break;
    case MSG_TYPE_CAST_MIRACAST_CONNECTED:
        //lv_label_set_text(m_label_state_msg, "Miracast connect OK.");
        win_cast_label_txt_set_fmt(m_label_state_msg, STR_MIRA_CONNECT_OK, hccast_wifi_mgr_get_current_freq_p2p());
        break;
    case MSG_TYPE_CAST_MIRACAST_GOT_IP:
        win_cast_label_txt_set_fmt(m_label_state_msg, STR_MIRA_GOT_IP_OK, hccast_wifi_mgr_get_current_freq_p2p());
        break;
    case MSG_TYPE_CAST_MIRACAST_SSID_DONE:
        lv_label_set_text(m_label_state_msg, "");
        break;
    case MSG_TYPE_USB_WIFI_PLUGOUT:
        //stop current playing? stop http
        win_cast_no_wifi_device_show();
        m_wifi_ready = 0;
        break;
    case MSG_TYPE_USB_WIFI_PLUGIN:
        m_wifi_ready = 1;
        break;

    case MSG_TYPE_AIR_INVALID_CERT:
        lv_label_set_text_fmt(m_label_demo, "demo: %s", sys_data->product_id);

        break;
	case MSG_TYPE_CAST_AIRCAST_AUDIO_START:
		//lv_label_set_text(m_label_state_msg, "Aircast is playing music");	
        win_cast_label_txt_set(m_label_state_msg, STR_AIR_PLAY_MUSIC);

		break;
	case MSG_TYPE_CAST_AIRCAST_AUDIO_STOP:
		lv_label_set_text(m_label_state_msg, "");
		break;
#if 1
    case MSG_TYPE_NET_UPGRADE:
        m_upgrade_doing = 1;
        break;
#endif		
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
	case MSG_TYPE_AIR_HOSTAP_SKIP_URL:
	case MSG_TYPE_DLNA_HOSTAP_SKIP_URL:
    #include "hcstring_id.h"
		win_msgbox_msg_open(STR_CONNECT_WIFI_FIRST, 3000, NULL, NULL);
		break;
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
            if (!hccast_wifi_mgr_p2p_get_connect_stat())
                lv_label_set_text(m_label_state_msg, "");
            break;
#endif
    default:
        break;
		
    }
}

static void win_cast_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;

	if (ctl_msg->msg_type == MSG_TYPE_KEY){
    //remote or pan key process
    
    } else if (ctl_msg->msg_type > MSG_TYPE_KEY) {
		win_cast_msg_proc(arg1, arg2);
    }
    else{
        //ret = WIN_CTL_SKIP;
    }	
//    return ret;

}

bool win_cast_root_wait_open(uint32_t timeout)
{
    uint32_t count;
    int wifi_module = 0;

    if (m_exit_to_cast_by_key) //exit from dlna/miracast/aircast, do not need wait
    {
        return true;
    }
    
#ifdef WIFI_SUPPORT
    wifi_module = network_wifi_module_get();
#endif    
    count = timeout/20;

    while(count--){
        if (m_win_cast_open || !win_cast_get_play_request_flag() || !wifi_module)
            break;
        api_sleep_ms(20);
    }
    printf("%s(), m_win_cast_open(%d):%d\n", __func__, (int)m_win_cast_open, (int)count);
    return m_win_cast_open; 
}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if (ta == ui_wifi_cast_root){
        if(event == LV_EVENT_SCREEN_LOAD_START) {
            win_cast_open();
        }
        if(event == LV_EVENT_SCREEN_UNLOAD_START) {
            win_cast_close();
        }
    } else if (ta == win_root_obj){
        lv_indev_t *key_indev = lv_indev_get_act();
        if(event == LV_EVENT_KEY && key_indev->proc.state == LV_INDEV_STATE_PRESSED){
            uint32_t value = lv_indev_get_key(key_indev);

            if (value == LV_KEY_ESC)            {
                //_ui_screen_change(channel_scr,0,0);
                change_screen(SCREEN_CHANNEL_MAIN_PAGE);
            }
        }


    }
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

void win_cast_mirror_rotate_switch(void)
{
    extern lv_obj_t* create_message_box(char* str);
    int rotate_enable = projector_get_some_sys_param(P_MIRROR_ROTATION);
    if (rotate_enable){
        projector_set_some_sys_param(P_MIRROR_ROTATION, 0);
        create_message_box((char*)api_rsc_string_get(STR_MIRROR_ROTATE_OFF));    
    }
    else{
        projector_set_some_sys_param(P_MIRROR_ROTATION, 1);
        create_message_box((char*)api_rsc_string_get(STR_MIRROR_ROTATE_ON));    
    }

    projector_sys_param_save();
#ifdef USBMIRROR_SUPPORT
    cast_usb_mirror_rotate_init();
    hccast_um_reset_video();
#endif
}


void ui_wifi_cast_init(void)
{
    screen_entry_t cast_root_entry;

    ui_wifi_cast_root = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_wifi_cast_root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(ui_wifi_cast_root, event_handler, LV_EVENT_ALL, NULL);
    // lv_obj_set_style_bg_color(ui_wifi_cast_root, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    // lv_obj_set_style_bg_opa(ui_wifi_cast_root, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_wifi_cast_root, LV_OPA_TRANSP, 0);

    cast_root_entry.screen = ui_wifi_cast_root;
    cast_root_entry.control = win_cast_control;
    api_screen_regist_ctrl_handle(&cast_root_entry);

    cast_main_ui_wait_init(win_cast_root_wait_open);
}

// win_des_t g_win_cast_root = 
// {
//     .open = win_cast_open,
//     .close = win_cast_close,
//     .control = win_cast_control,
// };


#endif
