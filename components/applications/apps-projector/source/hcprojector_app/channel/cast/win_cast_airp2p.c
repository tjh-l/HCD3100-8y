/*
win_cast_airp2p.c
 */
#include "app_config.h"

#ifdef CAST_SUPPORT// WIFI_SUPPORT
#ifdef AIRP2P_SUPPORT
#include <pthread.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "osd_com.h"
#include <fcntl.h>
#include <libusb.h>

#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"
#ifdef __HCRTOS__
#include <kernel/drivers/hcusb.h>
#endif

#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_dhcpd.h>
#include <hccast/hccast_httpd.h>

#include <hcfota.h>
#include "screen.h"
#include "cast_api.h"
#include "setup.h"
#include "win_cast_root.h"

#include "com_api.h"
#include "cast_api.h"
#include "network_api.h"
#include "factory_setting.h"
#include "hcstring_id.h"

#ifdef MIRACAST_SUPPORT
#define P2P_SWITCH_ENABLE //support airp2p mira coexist.
#endif

#define LABEL_AIRP2P_SSID_W   500
#define LABEL_AIRP2P_SSID_X  ((OSD_MAX_WIDTH-LABEL_AIRP2P_SSID_W) >> 1) //480
#define LABEL_AIRP2P_SSID_Y   30

#define LABEL_AIRP2P_PWD_X  LABEL_AIRP2P_SSID_X + LABEL_AIRP2P_SSID_W + 100
#define LABEL_AIRP2P_PWD_Y   LABEL_AIRP2P_SSID_Y
#define LABEL_AIRP2P_PWD_W   300

LV_FONT_DECLARE(cast_font_chn);

lv_obj_t * ui_airp2p_cast_root = NULL;
static lv_obj_t *win_airp2p_root_obj = NULL;
static lv_obj_t *m_label_local_ssid = NULL;
static lv_obj_t *m_label_password = NULL; 
static lv_obj_t *m_label_state_msg = NULL;
static lv_group_t *m_cast_airp2p_group = NULL;

static lv_style_t m_large_text_style;
static lv_style_t m_mid_text_style;
static lv_style_t m_small_text_style;

static volatile bool m_win_cast_airp2p_open = false;
static volatile bool m_airp2p_service_need_exit = false;
static volatile bool m_airp2p_service_en = false;
static volatile bool m_airp2p_playing = false;
static lv_timer_t *m_wait_wifi_ready_timer = NULL;
static int m_airp2p_ready = 0;
static int m_wifi_bus_port = -1;
static int m_airp2p_resetting = 0;

static void event_handler(lv_event_t * e);

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

static void win_cast_label_font_set(lv_obj_t *label, int font_idx)
{
    lv_font_t *font;
    font = win_cast_font_get(font_idx);
    lv_obj_set_style_text_font(label, font, 0);
}

static void win_cast_label_txt_set_fmt(lv_obj_t *label, uint32_t str_id, ...)
{
    char *rsc_str = (char *)api_rsc_string_get(str_id);
    char text_str[MAX_UI_TIPS_STR_LEN] = {0};
    va_list args;
    va_start(args, str_id);
    vsnprintf(text_str, sizeof(text_str), rsc_str, args);
    va_end(args);

    lv_label_set_text(label, text_str);
}

static void win_cast_label_txt_set(lv_obj_t *label, uint32_t str_id)
{
    win_cast_label_txt_set_fmt(label, str_id);
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

static void wifi_ready_timer_cb(lv_timer_t * t)
{  
    if (!network_wifi_module_get())
    {
        lv_label_set_text_fmt(m_label_local_ssid, "%s %s", LV_SYMBOL_CLOSE, "No WiFi device");
    }
    
    lv_timer_pause(m_wait_wifi_ready_timer);
    printf("wifi_ready_timer_cb done!\n");
}

static void start_wifi_ready_detect_timer(void)
{
    if (m_wait_wifi_ready_timer)
    {
        lv_timer_pause(m_wait_wifi_ready_timer);
        lv_timer_del(m_wait_wifi_ready_timer);
        m_wait_wifi_ready_timer = NULL;
    }

    m_wait_wifi_ready_timer = lv_timer_create(wifi_ready_timer_cb, 10000, NULL);
}

static void stop_wifi_ready_detect_timer(void)
{
    if (m_wait_wifi_ready_timer)
    {
        lv_timer_pause(m_wait_wifi_ready_timer);
        lv_timer_del(m_wait_wifi_ready_timer);
        m_wait_wifi_ready_timer = NULL;
    }
}

static bool win_cast_airp2p_wait_open(uint32_t timeout)
{
    uint32_t count;
    int wifi_module = 0;

    if (win_exit_to_cast_root_by_key_get()) //exit from aircast, do not need wait
    {
        return true;
    }
    
#ifdef WIFI_SUPPORT
    wifi_module = network_wifi_module_get();
#endif    
    count = timeout/20;

    while(count--){
        if (m_win_cast_airp2p_open || !win_cast_get_play_request_flag() || !wifi_module)
            break;
        api_sleep_ms(20);
    }
    printf("%s(), m_win_cast_airp2p_open(%d):%d\n", __func__, (int)m_win_cast_airp2p_open, (int)count);
    return m_win_cast_airp2p_open; 
}

static int win_cast_airp2p_check_wifi_gpio_enable(void)
{
    static int get_wifi_gpio_inited = 0;
    const char *st;
    static int wifi_gpio_enable = 0;
    int np;
    
    if (get_wifi_gpio_inited)
    {
        return wifi_gpio_enable;
    }

    get_wifi_gpio_inited = 1;
    
#ifdef __HCRTOS__     
    np = fdt_node_probe_by_path("/hcrtos/wifi_pw_enable");
    if(np>=0)
    {
        fdt_get_property_string_index(np, "status", 0, &st);
        if (!strcmp(st, "okay"))
        {   
            wifi_gpio_enable = 1;
        }
    }
#endif    

    return wifi_gpio_enable;
}

static int win_cast_airp2p_get_wifi_bus_port(void) 
{
#ifdef __HCRTOS__
    return api_wifi_get_bus_port();   
#else
    int bus_port = -1;
    int res;
    libusb_device **devs;
    struct libusb_device_descriptor desc;
    int cnt;
    int ret;
    char buf[32] = {0};
    char *wifi_support_list[] = {"v0BDApB733", "v0BDApF72B", "v0BDAp8731", "v0BDApC811"};
    
    res = libusb_init(NULL);
    libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

    if (res != 0)
    {
        printf("%s libusb_init failed: %s\n", __func__, libusb_error_name(res));
        return -1;
    }

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt > 0)
    {
        for (int j = 0; j < sizeof(wifi_support_list)/sizeof(char*); j++)
        {
            for (int i = 0; i < cnt; i++)
            {
                ret = libusb_get_device_descriptor(devs[i], &desc);
                if (ret != LIBUSB_SUCCESS)
                {
                    printf("Error getting enumerate device descriptor\n");
                    continue;
                }

                memset(buf, 0, sizeof(buf));
                snprintf(buf, sizeof(buf), "v%04xp%04x", desc.idVendor, desc.idProduct);

                if (!strcasecmp(wifi_support_list[j], buf))
                {
                    bus_port = libusb_get_bus_number(devs[i]);
                    if (libusb_get_bus_number(devs[i]) == 1)
                    {
                        bus_port = 0;
                    }
                    else if (libusb_get_bus_number(devs[i]) == 2)
                    {
                        bus_port = 1;
                    }

                    printf("airp2p get wifi bus port: %d\n", bus_port);

                    libusb_free_device_list(devs, 1);
                    libusb_exit(NULL);
                    return bus_port;
                }
            }
        }
        
        libusb_free_device_list(devs, 1);
    }

    libusb_exit(NULL);
    return bus_port;
#endif
}

static void cast_airp2p_reset_wifi(void)
{
    if (win_cast_airp2p_check_wifi_gpio_enable())
    {
        hccast_wifi_mgr_power_off(HCCAST_WIFI_PM_MODE_HW);
        hccast_wifi_mgr_power_on(HCCAST_WIFI_PM_MODE_HW);
    }
    else
    {
        m_wifi_bus_port = win_cast_airp2p_get_wifi_bus_port();
        if (m_wifi_bus_port != -1)
        {
        #ifdef __HCRTOS__              
            hcusb_set_mode(m_wifi_bus_port, MUSB_PERIPHERAL);
            usleep(100*1000);
            hcusb_set_mode(m_wifi_bus_port, MUSB_HOST);
        #else
            int fd = -1;
            if (m_wifi_bus_port == 0)
            {
                fd = open("/sys/devices/platform/soc/18844000.usb/musb-hdrc.0.auto/mode", O_RDWR);
            }
            else if(m_wifi_bus_port == 1)
            {
                fd = open("/sys/devices/platform/soc/18850000.usb/musb-hdrc.1.auto/mode", O_RDWR);
            }

            write(fd, "peripheral", strlen("peripheral"));
            usleep(100 * 1000);
            write(fd, "host", strlen("host"));
            close(fd);
        #endif
            printf("[%s] set Master-slave done.\n", __func__);
        }    
    }
}

static void cast_airp2p_start_prepare()
{
    wifi_config_t *wifi_config = NULL;
    int need_reset = 0;
    
    if (m_airp2p_service_en)
        return;

    printf("%s beging.\n", __func__);

    m_airp2p_resetting = 0;
    m_wifi_bus_port = -1;
    m_airp2p_ready = 0;
    win_cast_set_play_request_flag(true);
    m_airp2p_service_en = true;
    cast_main_ui_wait_init(win_cast_airp2p_wait_open);
#ifdef P2P_SWITCH_ENABLE
    cast_set_p2p_switch_enable(1);
#else
    hccast_scene_set_mira_restart_enable(false); 
#endif
   
    network_set_airp2p_state(true);
    wifi_config = app_wifi_config_get();

    if (api_get_wifi_pm_state() && (api_get_wifi_pm_mode() == HCCAST_WIFI_PM_MODE_HW))
    {
        network_wifi_pm_stop();
    }
    else
    {
        /*step1: Wait until wifi-connect or wifi-scan stop. */
        network_set_current_state(false);
        
        while(wifi_config->wifi_connecting || wifi_config->wifi_scanning 
#ifdef MIRACAST_SUPPORT
            || hccast_mira_get_restart_state()
#endif
        )
        {
            hccast_wifi_mgr_op_abort();
            usleep(10*1000);
        }

        printf("%s wifi state check done.\n", __func__);

#ifdef __HCRTOS__
        if (hccast_wifi_mgr_get_station_status() && hccast_wifi_mgr_get_connect_status())
#else
        if (hccast_wifi_mgr_get_connect_status())
#endif
        {
            hccast_wifi_mgr_disconnect_no_message();
        }
        
        wifi_config->bConnected = false;
        wifi_config->bReconnect = false;
        hccast_wifi_mgr_udhcpc_stop();
        hccast_stop_services();
        
        /*step2: Stop wpa.*/
        if (app_wifi_get_work_mode())
        {
            need_reset = 1;
        }
        
        app_wifi_switch_work_mode(WIFI_MODE_NONE);
        
        /*step3: Trigger wifi power off.*/
        if (need_reset)
        {
            network_set_current_state(true);
            network_wifi_module_set(0);
            cast_airp2p_reset_wifi();
        }
        else
        {
            network_set_current_state(true);
            network_connect();
        }
    } 
}

static void cast_airp2p_stop_service(void)
{
    if (!m_airp2p_service_need_exit)
        return;

    cast_air_set_default_res();
    m_airp2p_service_need_exit = false;
    m_airp2p_service_en = false;
    win_cast_set_play_request_flag(false);
    cast_main_ui_wait_init(win_cast_root_wait_open);    
    hccast_scene_set_mira_restart_enable(true);
    
    network_set_airp2p_state(false);
    cast_airp2p_enable(0);

    if (hccast_air_service_is_start())
    {
        network_airp2p_stop();
        cast_set_p2p_switch_enable(0);

        network_wifi_module_set(0);

        if (win_cast_airp2p_check_wifi_gpio_enable())
        {
            hccast_wifi_mgr_power_off(HCCAST_WIFI_PM_MODE_HW);
            hccast_wifi_mgr_power_on(HCCAST_WIFI_PM_MODE_HW);
        }
        else
        {
            cast_airp2p_reset_wifi();
        }
    }
    else
    {
        network_connect();
    }

    printf("%s done.\n", __func__);          
}

static int win_cast_airp2p_state_update()
{
    stop_wifi_ready_detect_timer();
    if (!network_wifi_module_get())
    {
        if (!m_airp2p_ready)
        {
            lv_label_set_text(m_label_password, "");
            lv_label_set_text_fmt(m_label_local_ssid, "%s", "WiFi detecting ...");
            start_wifi_ready_detect_timer();
        }
        else
        {
            lv_label_set_text_fmt(m_label_local_ssid, "%s %s", LV_SYMBOL_CLOSE, "No WiFi device");
            lv_label_set_text(m_label_password, "");
        }    
    }
    else
    {
        lv_label_set_text_fmt(m_label_local_ssid, "SSID: %s_%s", projector_get_some_sys_param(P_DEVICE_NAME), AIRP2P_NAME);
        lv_label_set_text_fmt(m_label_password, "Pin: %s", "1234");
    }

    return 0;
}

static int win_cast_airp2p_open(void)
{   
    m_airp2p_service_need_exit = true;
    win_exit_to_cast_root_by_key_set(false);

  #ifdef USB_MIRROR_FAST_SUPPORT
    um_service_wait_exit();
  #endif    

    cast_airp2p_start_prepare();
    
    win_airp2p_root_obj = lv_obj_create(ui_airp2p_cast_root);
    struct sysdata *sys_data;
    osd_draw_background(win_airp2p_root_obj, true);
    lv_obj_clear_flag(win_airp2p_root_obj, LV_OBJ_FLAG_SCROLLABLE);
   	
    m_cast_airp2p_group = lv_group_create();
    key_set_group(m_cast_airp2p_group);

    lv_obj_add_event_cb(win_airp2p_root_obj, event_handler, LV_EVENT_ALL, NULL);    
    lv_group_add_obj(m_cast_airp2p_group, win_airp2p_root_obj);
    lv_group_focus_obj(win_airp2p_root_obj);

    sys_data = &(projector_get_sys_param()->sys_data);

    printf("%s(), line: %d!, fw_ver: 0x%x, product_id:%s\n", 
        __func__, __LINE__, (unsigned int)sys_data->firmware_version, sys_data->product_id);
    set_display_zoom_when_sys_scale();

    api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE);    
    //api_logo_show(NULL);
#if CASTING_CLOSE_FB_SUPPORT	    
    api_osd_show_onoff(true);
#endif

    lv_obj_t *parent_obj = win_airp2p_root_obj;

    lv_style_init(&m_large_text_style);
    lv_style_init(&m_mid_text_style);
    lv_style_init(&m_small_text_style);
    lv_style_set_text_font(&m_large_text_style, osd_font_get_by_langid(0,FONT_LARGE));
    lv_style_set_text_font(&m_mid_text_style, osd_font_get_by_langid(0,FONT_MID));
    lv_style_set_text_font(&m_small_text_style, osd_font_get_by_langid(0,FONT_MID));
    lv_style_set_text_color(&m_large_text_style, COLOR_WHITE);
    lv_style_set_text_color(&m_mid_text_style, COLOR_WHITE);
    lv_style_set_text_color(&m_small_text_style, COLOR_WHITE);

    m_label_local_ssid = lv_label_open(parent_obj, LABEL_AIRP2P_SSID_X, LABEL_AIRP2P_SSID_Y, LABEL_AIRP2P_SSID_W, NULL, &m_mid_text_style);
    //lv_label_set_long_mode(m_label_local_ssid, LV_LABEL_LONG_DOT);
    lv_label_set_long_mode(m_label_local_ssid, LV_LABEL_LONG_CLIP);

    m_label_password = lv_label_open(parent_obj, LABEL_AIRP2P_PWD_X, LABEL_AIRP2P_PWD_Y, LABEL_AIRP2P_PWD_W, NULL, &m_mid_text_style);

    m_label_state_msg = lv_label_open(parent_obj, 0, 0, 0, NULL, &m_mid_text_style);
    lv_obj_align(m_label_state_msg, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_font(m_label_state_msg, osd_font_get(FONT_MID),0);
    lv_label_set_text(m_label_state_msg, "");

    if(hccast_air_audio_state_get())
    {
        //lv_label_set_text(m_label_state_msg, "Aircast is playing music");
        win_cast_label_txt_set(m_label_state_msg, STR_AIR_PLAY_MUSIC);
    }

    win_cast_airp2p_state_update();
    m_win_cast_airp2p_open = true;

    return API_SUCCESS;
}

static int win_cast_airp2p_close(void)
{
    cast_airp2p_stop_service();
    
    stop_wifi_ready_detect_timer();
    lv_obj_del(win_airp2p_root_obj);
	
    api_osd_off_time(500);
    api_logo_off();

    if (m_cast_airp2p_group){
        lv_group_remove_all_objs(m_cast_airp2p_group);
        lv_group_del(m_cast_airp2p_group);
        lv_group_set_default(NULL);
    }

    m_airp2p_playing = false;
    m_win_cast_airp2p_open = false;
    win_airp2p_root_obj = NULL;
    
    //recover the dispaly aspect.
    if (m_airp2p_service_need_exit)
        api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE);

    win_clear_popup();

    printf("%s(), line: %d!\n", __func__, __LINE__);
    
    return API_SUCCESS;
}

static void win_cast_airp2p_msg_proc(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    struct sysdata *sys_data;

    sys_data = &(projector_get_sys_param()->sys_data);

    switch (ctl_msg->msg_type)
    {
        case MSG_TYPE_CAST_AIRMIRROR_START:
        case MSG_TYPE_CAST_MIRACAST_START:
            m_airp2p_service_need_exit = false;
            m_airp2p_playing = true;
            win_cast_set_play_param((uint32_t)ctl_msg->msg_type);
            if (ui_cast_play)
            {
                printf("%s(), line:%d. _ui_screen_change: cast play.\n", __func__, __LINE__);
                _ui_screen_change(ui_cast_play,0,0);
            }
            break;
        case MSG_TYPE_CAST_AIRMIRROR_STOP:
            m_airp2p_playing = false;
            break;
        case MSG_TYPE_USB_WIFI_PLUGOUT:
            if (m_airp2p_ready && !m_airp2p_resetting)
            {
                lv_label_set_text_fmt(m_label_local_ssid, "%s %s", LV_SYMBOL_CLOSE, "No WiFi device");
            }  
            break;
        case MSG_TYPE_USB_WIFI_PLUGIN:
            break;
        case MSG_TYPE_CAST_AIRCAST_AUDIO_START:
            win_cast_label_txt_set(m_label_state_msg, STR_AIR_PLAY_MUSIC);
            break;
        case MSG_TYPE_CAST_AIRCAST_AUDIO_STOP:    
            lv_label_set_text(m_label_state_msg, "");
            break;
        case MSG_TYPE_CAST_AIRP2P_READY: 
            if (m_airp2p_resetting)
            {
                m_airp2p_resetting = 0;
            }
            else
            {
                win_cast_airp2p_state_update();
            }     

            m_airp2p_ready = 1;
            break;
        case MSG_TYPE_CAST_MIRACAST_CONNECTING:
            win_cast_label_txt_set(m_label_state_msg, STR_MIRA_CONNECTING);
            break;
        case MSG_TYPE_CAST_MIRACAST_CONNECTED:
            win_cast_label_txt_set_fmt(m_label_state_msg, STR_MIRA_CONNECT_OK, hccast_wifi_mgr_get_current_freq_p2p());
            break;
        case MSG_TYPE_CAST_MIRACAST_GOT_IP:
            win_cast_label_txt_set_fmt(m_label_state_msg, STR_MIRA_GOT_IP_OK, hccast_wifi_mgr_get_current_freq_p2p());
            break;            
        case MSG_TYPE_CAST_MIRACAST_SSID_DONE:
            lv_label_set_text(m_label_state_msg, "");      
            break;
        case MSG_TYPE_CAST_MIRACAST_RESET:
#ifdef P2P_SWITCH_ENABLE
            hccast_mira_service_stop();
            hccast_mira_service_start();   
            cast_reset_p2p_switch_state();
#endif
            break;
        case MSG_TYPE_CAST_AIRP2P_RESET:
            printf("Beging to reset airp2p.\n");
            m_airp2p_resetting = 1;
            network_set_current_state(true);
            network_wifi_module_set(0);
            network_airp2p_stop();
            cast_airp2p_reset_wifi();
            break;
        default:
            break;	
    }
}

static void win_cast_airp2p_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;

    if (ctl_msg->msg_type == MSG_TYPE_KEY)
    {
    
    }
    else if (ctl_msg->msg_type > MSG_TYPE_KEY) 
    {
    	win_cast_airp2p_msg_proc(arg1, arg2);
    }
    else
    {
    
    }	
}

static void win_cast_airp2p_rotate_switch(void)
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
}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if (ta == ui_airp2p_cast_root)
    {
        if(event == LV_EVENT_SCREEN_LOAD_START) 
        {
            win_cast_airp2p_open();
        }
        else if(event == LV_EVENT_SCREEN_UNLOAD_START) 
        {
            win_cast_airp2p_close();
        }
    } 
    else if (ta == win_airp2p_root_obj)
    {
        lv_indev_t *key_indev = lv_indev_get_act();
        if(event == LV_EVENT_KEY && key_indev->proc.state == LV_INDEV_STATE_PRESSED)
        {
            uint32_t value = lv_indev_get_key(key_indev);
            if (value == LV_KEY_ESC)            
            {
                change_screen(SCREEN_CHANNEL_MAIN_PAGE);
            } 
            else if (value == FUNC_KEY_SCREEN_ROTATE)
            {
                if (m_airp2p_playing)
                {
                    win_cast_airp2p_rotate_switch();
                }    
            }
        }
    }
}

void ui_cast_airp2p_init(void)
{
    screen_entry_t cast_root_entry;

    ui_airp2p_cast_root = lv_obj_create(NULL);

    lv_obj_clear_flag(ui_airp2p_cast_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_airp2p_cast_root, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_opa(ui_airp2p_cast_root, LV_OPA_TRANSP, 0);

    cast_root_entry.screen = ui_airp2p_cast_root;
    cast_root_entry.control = win_cast_airp2p_control;
    api_screen_regist_ctrl_handle(&cast_root_entry);

    //cast_main_ui_wait_init(win_cast_root_wait_open);
}

#endif
#endif

