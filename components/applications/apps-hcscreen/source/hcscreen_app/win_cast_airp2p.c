/*
win_cast_airp2p.c
 */
#include "app_config.h"

#ifdef AIRP2P_SUPPORT
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
#include <kernel/drivers/hcusb.h>
#include <libusb.h>
#include <kernel/lib/fdt_api.h>

#include "cast_api.h"
#include "win_cast_root.h"

#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"
#include "network_api.h"
#include "data_mgr.h"

#ifdef MIRACAST_SUPPORT
#define P2P_SWITCH_ENABLE //support airp2p mira coexist.
#endif

#define LABEL_AIRP2P_SSID_W   400
#define LABEL_AIRP2P_SSID_X  ((OSD_MAX_WIDTH-LABEL_AIRP2P_SSID_W) >> 1) //480
#define LABEL_AIRP2P_SSID_Y   30

#define LABEL_AIRP2P_PWD_X  LABEL_AIRP2P_SSID_X + LABEL_AIRP2P_SSID_W + 100
#define LABEL_AIRP2P_PWD_Y   LABEL_AIRP2P_SSID_Y
#define LABEL_AIRP2P_PWD_W   300

static lv_obj_t *win_airp2p_root_obj = NULL;
static lv_obj_t *m_label_airp2p_ssid = NULL;
static lv_obj_t *m_label_airp2p_pin = NULL; 
static lv_obj_t *m_label_airp2p_state_msg = NULL;
static lv_obj_t *m_label_airp2p_version = NULL;
static lv_obj_t *m_label_airp2p_demo = NULL;

static lv_style_t m_large_text_style;
static lv_style_t m_mid_text_style;
static lv_style_t m_small_text_style;

extern wifi_model_st g_hotplug_wifi_model;
static volatile bool m_win_cast_airp2p_open = false;
static volatile bool m_airp2p_service_need_exit = false;
static volatile bool m_airp2p_service_en = false;
static volatile bool m_airp2p_playing = false;
static int m_wifi_usb_port = -1;
static int m_airp2p_resetting = 0;

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

static void win_cast_airp2p_demo_show(void)
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

    if (cast_airp2p_is_demo())
    {
        cast_is_demo = 1;
        strncat(demo_content, "p", sizeof(demo_content));
    }

    if (cast_is_demo)
        lv_label_set_text_fmt(m_label_airp2p_demo, "demo(%s): %s", demo_content, sys_data->product_id);
    else
        lv_label_set_text_fmt(m_label_airp2p_demo, "%s", sys_data->product_id);
}

bool win_cast_airp2p_wait_open(uint32_t timeout)
{
    uint32_t count;
    int wifi_module = 0;

    if (win_exit_to_cast_root_by_key_get()) //exit from aircast, do not need wait
    {
        return true;
    }

    wifi_module = network_wifi_module_get();
    count = timeout/20;

    while(count--){
        if (m_win_cast_airp2p_open || !wifi_module)
            break;
        api_sleep_ms(20);
    }
    printf("%s(), m_win_cast_airp2p_open(%d):%d\n", __func__, (int)m_win_cast_airp2p_open, (int)count);
    return m_win_cast_airp2p_open; 
}

static int win_cast_airp2p_get_pw_enable(void)
{
    static int get_wifi_pw_inited = 0;
    const char *st;
    static int wifi_pw_enable = 0;
    int np;
    
    if (get_wifi_pw_inited)
    {
        return wifi_pw_enable;
    }

    get_wifi_pw_inited = 1;
    
#ifdef __HCRTOS__     
    np = fdt_node_probe_by_path("/hcrtos/wifi_pw_enable");
    if(np>=0)
    {
        fdt_get_property_string_index(np, "status", 0, &st);
        if (!strcmp(st, "okay"))
        {   
            wifi_pw_enable = 1;
        }
    }
#endif    

    return wifi_pw_enable;
}

static int win_cast_airp2p_get_wifi_port(void)
{
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
#ifdef __linux__ 
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
#else
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

            if (!strcasecmp(g_hotplug_wifi_model.desc, buf))
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

                printf("get wifi bus port: %d\n", bus_port);
                break;
            }
        }
#endif
        libusb_free_device_list(devs, 1);
    }

    libusb_exit(NULL);

    return bus_port;
}

static void cast_airp2p_reset_wifi(void)
{
    if (m_wifi_usb_port == -1)
    {
        m_wifi_usb_port = win_cast_airp2p_get_wifi_port();
    }

    if (m_wifi_usb_port != -1)
    {
#ifdef __HCRTOS__              
        hcusb_set_mode(m_wifi_usb_port, MUSB_PERIPHERAL);
        usleep(100*1000);
        hcusb_set_mode(m_wifi_usb_port, MUSB_HOST);
#else
        int fd = -1;
        if (m_wifi_usb_port == 0)
        {
            fd = open("/sys/devices/platform/soc/18844000.usb/musb-hdrc.0.auto/mode", O_RDWR);
        }
        else if(m_wifi_usb_port == 1)
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

static void win_cast_airp2p_start_prepare()
{
    wifi_config_t *wifi_config = app_wifi_config_get();;
    int need_reset = 0;
    
    if (m_airp2p_service_en)
        return;

    printf("%s beging.\n", __func__);          

    m_airp2p_resetting = 0;
    m_wifi_usb_port = -1;
    cast_api_set_play_request_flag(true);
    m_airp2p_service_en = true;

#ifdef P2P_SWITCH_ENABLE
    cast_set_p2p_switch_enable(1);
#else
    hccast_scene_set_mira_restart_enable(false); 
#endif
    network_set_airp2p_state(true);

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

    if (hccast_wifi_mgr_get_station_status() && hccast_wifi_mgr_get_connect_status())
    {
        hccast_wifi_mgr_disconnect_no_message();
    }
    
    app_set_wifi_connect_status(false);
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

static void win_cast_airp2p_stop_service(void)
{
    if (!m_airp2p_service_need_exit)
        return;

    m_airp2p_service_need_exit = false;
    m_airp2p_service_en = false;
    cast_api_set_play_request_flag(false);
    hccast_scene_set_mira_restart_enable(true);
    
    network_set_airp2p_state(false);
    
    if (hccast_air_service_is_start())
    {
        network_airp2p_stop();
        cast_set_p2p_switch_enable(0);

        network_wifi_module_set(0);
        cast_airp2p_reset_wifi(); 
    }
    else
    {
        network_connect();
    }

    printf("%s done.\n", __func__);          
}

static int win_cast_airp2p_state_update()
{
    app_data_t  *app_data = data_mgr_app_get();
    
    if (!network_wifi_module_get())
    {
        lv_label_set_text_fmt(m_label_airp2p_ssid, "%s %s", LV_SYMBOL_CLOSE, "No WiFi device");
        lv_label_set_text(m_label_airp2p_pin, "");
    }
    else
    {
        lv_label_set_text_fmt(m_label_airp2p_ssid, "SSID: %s_p2p", data_mgr_get_device_name());
        lv_label_set_text_fmt(m_label_airp2p_pin, "Pin: %s", app_data->airp2p_pin);
    }

    return 0;
}

static int win_cast_airp2p_open(void *arg)
{
    win_airp2p_root_obj = lv_obj_create(lv_scr_act());
    osd_draw_background(win_airp2p_root_obj, true);
    lv_obj_clear_flag(win_airp2p_root_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *parent_obj = win_airp2p_root_obj;
    sys_data_t *sys_data = data_mgr_sys_get();

    printf("%s(), line: %d!, fw_ver: 0x%x, product_id:%s\n", \
           __func__, __LINE__, sys_data->firmware_version, sys_data->product_id);

    m_airp2p_service_need_exit = true;
    api_set_key_rotate_enable(false);
    api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
    win_cast_airp2p_start_prepare();

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

    m_label_airp2p_ssid = lv_label_open(parent_obj, LABEL_AIRP2P_SSID_X, LABEL_AIRP2P_SSID_Y, LABEL_AIRP2P_SSID_W, NULL, &m_mid_text_style);
    lv_label_set_long_mode(m_label_airp2p_ssid, LV_LABEL_LONG_DOT);
    m_label_airp2p_pin = lv_label_open(parent_obj, LABEL_AIRP2P_PWD_X, LABEL_AIRP2P_PWD_Y, LABEL_AIRP2P_PWD_W, NULL, &m_mid_text_style);

    m_label_airp2p_demo = lv_label_open(parent_obj, 0, 0, 0, NULL, &m_mid_text_style);
    lv_obj_align(m_label_airp2p_demo, LV_ALIGN_BOTTOM_LEFT, 20, -20);

    win_cast_airp2p_demo_show();

    m_label_airp2p_version = lv_label_open(parent_obj, 0, 0, 0,
                                    NULL, &m_mid_text_style);
    lv_label_set_text_fmt(m_label_airp2p_version, "Ver: %u", sys_data->firmware_version);
    lv_obj_align(m_label_airp2p_version, LV_ALIGN_BOTTOM_RIGHT, -20, -20);

    m_label_airp2p_state_msg = lv_label_open(parent_obj, 0, 0, 0, NULL, &m_mid_text_style);
    lv_obj_align(m_label_airp2p_state_msg, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_label_set_text(m_label_airp2p_state_msg, "");

    if(hccast_air_audio_state_get())
    {
        lv_label_set_text(m_label_airp2p_state_msg, "Aircast is playing music");
    }

    win_cast_airp2p_state_update();
    
    m_win_cast_airp2p_open = true;
    api_logo_dis_backup_free();

    return API_SUCCESS;

}

static int win_cast_airp2p_close(void *arg)
{
    win_cast_airp2p_stop_service();
    
    lv_obj_del(win_airp2p_root_obj);
    
    win_msgbox_msg_close();
    api_logo_off();

    m_airp2p_playing = false;
    m_win_cast_airp2p_open = false;
    win_airp2p_root_obj = NULL;

    api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);

    printf("%s(), line: %d!\n", __func__, __LINE__);
    
    return API_SUCCESS;
}

static win_ctl_result_t win_cast_airp2p_msg_proc(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;
    win_des_t *cur_win = NULL;
    struct sysdata *sys_data = data_mgr_sys_get();;
    app_data_t  *app_data = data_mgr_app_get();

    switch (ctl_msg->msg_type)
    {
        case MSG_TYPE_CAST_AIRMIRROR_START:
        case MSG_TYPE_CAST_MIRACAST_START:
            api_set_key_rotate_enable(true);
            m_airp2p_service_need_exit = false;
            cur_win = &g_win_cast_play;
            cur_win->param = (void*)ctl_msg->msg_type;
            menu_mgr_push(cur_win);
            ret = WIN_CTL_PUSH_CLOSE;
            break;
        case MSG_TYPE_CAST_AIRMIRROR_STOP:
            m_airp2p_playing = false;
            break;
        case MSG_TYPE_CAST_AIRCAST_AUDIO_START: 
            lv_label_set_text(m_label_airp2p_state_msg, "Aircast is playing music");
            break;
        case MSG_TYPE_CAST_AIRCAST_AUDIO_STOP:
            lv_label_set_text(m_label_airp2p_state_msg, "");
            break;
        case MSG_TYPE_USB_WIFI_PLUGOUT:
            if (!m_airp2p_resetting)
                win_cast_airp2p_state_update();  
            break;
        case MSG_TYPE_USB_WIFI_PLUGIN:
            break;
        case MSG_TYPE_AIRP2P_READY: 
            if (m_airp2p_resetting)
            {
                m_airp2p_resetting = 0;
            }
            else
            {
                win_cast_airp2p_state_update();
            }
            
            api_set_key_switch_enable(true);
            break;
        case MSG_TYPE_AIR_INVALID_CERT:
        case MSG_TYPE_CAST_DIAL_INVALID_CERT:
            win_cast_airp2p_demo_show();
            break;
        case MSG_TYPE_USB_UPGRADE:      
            cur_win = &g_win_upgrade;
            cur_win->param = (void*)(ctl_msg->msg_type);
            menu_mgr_push(cur_win);
            ret = WIN_CTL_PUSH_CLOSE;
            break;
        case MSG_TYPE_KEY_SWITCH_CAST_MODE:
            cast_air_set_default_res();
            api_set_key_switch_enable(false);
            cur_win = &g_win_cast_root;
            cur_win->param = (void*)ctl_msg->msg_type;
            menu_mgr_pop_all();
            menu_mgr_push(cur_win);
            ret = WIN_CTL_PUSH_CLOSE;
            break;
        case MSG_TYPE_CAST_MIRACAST_CONNECTING:
            lv_label_set_text(m_label_airp2p_state_msg, "Miracast is connecting ...");
            break;
        case MSG_TYPE_CAST_MIRACAST_CONNECTED:
            lv_label_set_text_fmt(m_label_airp2p_state_msg, "Miracast is obtaining IP (%d)...", \
                hccast_wifi_mgr_get_current_freq_p2p());
            break;
        case MSG_TYPE_CAST_MIRACAST_GOT_IP:
            lv_label_set_text_fmt(m_label_airp2p_state_msg, "Miracast is obtaining stream (%d) ...", \
                hccast_wifi_mgr_get_current_freq_p2p());
            break;            
        case MSG_TYPE_CAST_MIRACAST_SSID_DONE:
            lv_label_set_text(m_label_airp2p_state_msg, "");      
            break;
        case MSG_TYPE_CAST_MIRACAST_RESET:
#ifdef P2P_SWITCH_ENABLE
            hccast_mira_service_stop();
            hccast_mira_service_start();   
            cast_reset_p2p_switch_state();
#endif
            break;  
        case MSG_TYPE_AIRP2P_RESET:
            printf("Beging to reset airp2p.\n");
            //api_set_key_switch_enable(false);
            m_airp2p_resetting = 1;
            network_set_current_state(true);
            network_wifi_module_set(0);
            network_airp2p_stop();
            cast_airp2p_reset_wifi();
            break;
        default:
            break;	
    }

    return ret;
}

static win_ctl_result_t win_cast_airp2p_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_NONE;

    if (ctl_msg->msg_type == MSG_TYPE_KEY)
    {
    
    }
    else if (ctl_msg->msg_type > MSG_TYPE_KEY) 
    {
    	ret = win_cast_airp2p_msg_proc(arg1, arg2);
    }
    else
    {
        ret = WIN_CTL_SKIP;
    }	

    return ret;
}

win_des_t g_win_cast_airp2p_root =
{
    .open = win_cast_airp2p_open,
    .close = win_cast_airp2p_close,
    .control = win_cast_airp2p_control,
};
#endif
