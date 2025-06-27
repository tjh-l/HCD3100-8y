#include "app_config.h"

#include <stdio.h>

#ifdef  HDMI_RX_CEC_SUPPORT
#include <hudi/hudi_cec.h>
#endif
#include "hdmi_rx.h"
#include "screen.h"
#include "setup.h"
#include "factory_setting.h"
#include "com_api.h"
#include "hcstring_id.h"
#include "osd_com.h"
#include "key_event.h"
#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define NO_SIG_RECT_W 25
    #define NO_SIG_RECT_H  20

#else
    #define NO_SIG_RECT_W 15
    #define NO_SIG_RECT_H 12
#endif

#define min(a, b) ((a)<(b) ? (a) : (b))
#define max(a, b) ((a)<(b) ? (b) : (a))

lv_obj_t* hdmi_scr = NULL;
lv_group_t *hdmi_g = NULL;
lv_obj_t* no_signal_rect = NULL;

char* no_sig_str = "NO SIGNAL\0无信号\0PAS DE SIGNAL";
static char* hdmi_str = "HDMI\0高清";

static lv_timer_t* timer_start = NULL;
static lv_timer_t *timer_err_input = NULL;
static lv_coord_t h=0;
static lv_coord_t w=0;
static lv_timer_t *timer = NULL;
static bool up_dir = true;
static bool left_dir = true;

extern lv_font_t* select_font_normal[3];

static void hdmi_screen_plugoutin(void);
static void hdmi_screen_start(void);
static void timer_err_input_handle(lv_timer_t *e);
#ifdef  HDMI_RX_CEC_SUPPORT
static uint32_t m_hotkey[] = {KEY_POWER, \
                    KEY_MENU, KEY_HOME,KEY_FORWARD,KEY_BACK, KEY_FLIP, KEY_EPG/*projector-flip*/};



static int _hdmirx_key_preproc(uint32_t v_key)
{
    switch (v_key)
    {
        case V_KEY_V_UP:
            hudi_cec_audio_volup(hdmirx_cec_handle_get(), projector_get_some_sys_param(P_CEC_DEVICE_LA));
            break;
        case V_KEY_V_DOWN:
            hudi_cec_audio_voldown(hdmirx_cec_handle_get(), projector_get_some_sys_param(P_CEC_DEVICE_LA));
            break;
        case V_KEY_MUTE:
            hudi_cec_audio_toggle_mute(hdmirx_cec_handle_get(), projector_get_some_sys_param(P_CEC_DEVICE_LA));
            break;
        case V_KEY_UP:
            hudi_cec_key_press_through(hdmirx_cec_handle_get(), projector_get_some_sys_param(P_CEC_DEVICE_LA),HUDI_CEC_USER_CONTROL_CODE_UP);
            break;
        case V_KEY_DOWN:
            hudi_cec_key_press_through(hdmirx_cec_handle_get(), projector_get_some_sys_param(P_CEC_DEVICE_LA),HUDI_CEC_USER_CONTROL_CODE_DOWN);
            break;
        case V_KEY_LEFT:
            hudi_cec_key_press_through(hdmirx_cec_handle_get(), projector_get_some_sys_param(P_CEC_DEVICE_LA),HUDI_CEC_USER_CONTROL_CODE_LEFT);
            break;
        case V_KEY_RIGHT:
            hudi_cec_key_press_through(hdmirx_cec_handle_get(), projector_get_some_sys_param(P_CEC_DEVICE_LA),HUDI_CEC_USER_CONTROL_CODE_RIGHT);
            break;
        case V_KEY_ENTER:
            hudi_cec_key_press_through(hdmirx_cec_handle_get(), projector_get_some_sys_param(P_CEC_DEVICE_LA),HUDI_CEC_USER_CONTROL_CODE_ENTER);
            break;
        case V_KEY_EXIT:
            hudi_cec_key_press_through(hdmirx_cec_handle_get(), projector_get_some_sys_param(P_CEC_DEVICE_LA),HUDI_CEC_USER_CONTROL_CODE_EXIT);
            break;
        default:
            break;
    }

    return 0;
   
}

static int _hdmirx_key_boardcast_proc(uint32_t v_key)
{
    hdmirx_cec_device_res_t * res = (hdmirx_cec_device_res_t *)hdmirx_cec_device_res_get();
    switch (v_key)
    {
        case V_KEY_V_UP:
            {
                for(int i = 0; i < res->count; i++)
                    hudi_cec_audio_volup(hdmirx_cec_handle_get(), res->device_la[i]);
            }
            break;
        case V_KEY_V_DOWN:
            {
                for(int i = 0; i < res->count; i++)
                    hudi_cec_audio_voldown(hdmirx_cec_handle_get(), res->device_la[i]);
            }
            break;
        case V_KEY_MUTE:
            {
                for(int i = 0; i < res->count; i++)
                    hudi_cec_audio_toggle_mute(hdmirx_cec_handle_get(), res->device_la[i]);
            }
            break;
        case V_KEY_UP:
            {
                for(int i = 0; i < res->count; i++)
                    hudi_cec_key_press_through(hdmirx_cec_handle_get(), res->device_la[i],HUDI_CEC_USER_CONTROL_CODE_UP);
            }
            break;
        case V_KEY_DOWN:
            {
                for(int i = 0; i < res->count; i++)
                    hudi_cec_key_press_through(hdmirx_cec_handle_get(), res->device_la[i],HUDI_CEC_USER_CONTROL_CODE_DOWN);
            }
            break;
        case V_KEY_LEFT:
            {
                for(int i = 0; i < res->count; i++)
                    hudi_cec_key_press_through(hdmirx_cec_handle_get(), res->device_la[i],HUDI_CEC_USER_CONTROL_CODE_LEFT);
            }
            break;
        case V_KEY_RIGHT:
            {
                for(int i = 0; i < res->count; i++)
                    hudi_cec_key_press_through(hdmirx_cec_handle_get(), res->device_la[i],HUDI_CEC_USER_CONTROL_CODE_RIGHT);
            }
            break;
        case V_KEY_ENTER:
            {
                for(int i = 0; i < res->count; i++)
                    hudi_cec_key_press_through(hdmirx_cec_handle_get(), res->device_la[i],HUDI_CEC_USER_CONTROL_CODE_ENTER);
            }
            break;
        case V_KEY_EXIT:
            {
                for(int i = 0; i < res->count; i++)
                    hudi_cec_key_press_through(hdmirx_cec_handle_get(), res->device_la[i],HUDI_CEC_USER_CONTROL_CODE_EXIT);
            }
            break;
        default:
            break;
    }
}

#endif
static void timer_err_input_handle(lv_timer_t *e){
    lv_obj_t *msgbox = (lv_obj_t*)e->user_data;
    lv_msgbox_close(msgbox);
    if(lv_obj_has_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN)){
        lv_obj_clear_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_opa(hdmi_scr, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(hdmi_scr, lv_color_hex(0x0000ff), LV_PART_MAIN);
		
    }        
    timer_err_input = NULL;
}

static void timer_handle(lv_timer_t *e){
    if(hdmirx_get_plug_status() == HDMI_RX_STATUS_ERR_INPUT){
        if(lv_obj_has_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN)){
            if(timer_err_input == NULL){
                lv_obj_t* msgbox = lv_msgbox_create(lv_layer_top(), NULL, api_rsc_string_get(STR_HDMI_FORMAT_USPT), NULL, false);
                lv_obj_set_style_text_font(msgbox, osd_font_get(FONT_MID), 0);

                timer_err_input = lv_timer_create(timer_err_input_handle, 3000, msgbox);
                lv_timer_set_repeat_count(timer_err_input, 1);
                lv_timer_reset(timer_err_input);
            }
        return;
        }
    }
    if(hdmirx_get_plug_status() == HDMI_RX_STATUS_PLUGIN){
        if(!lv_obj_has_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN)){
    		 lv_obj_set_style_bg_opa(hdmi_scr, LV_OPA_TRANSP, LV_PART_MAIN);
             lv_obj_add_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN);
        }        
        return;
    }

    if(lv_obj_has_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN)){
        lv_obj_clear_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_opa(hdmi_scr, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(hdmi_scr, lv_color_hex(0x0000ff), LV_PART_MAIN);
    }

    lv_coord_t p_h = lv_obj_get_x(no_signal_rect);
    lv_coord_t p_v = lv_obj_get_y(no_signal_rect);
    if(up_dir){
        p_v = max(0, p_v - 10);
        if(p_v == 0){
            up_dir = false;
        }
    }else{
        p_v = min(h/6*5, p_v+10);
        if(p_v == h/6*5){
            up_dir = true;
        }
    }

    if(left_dir){
        p_h = max(0, p_h - 20);
        if(p_h == 0){
            left_dir = false;
        }
    }else{
        p_h = min(w/5*4, p_h + 20);
        if(p_h == w/5*4){
            left_dir = true;
        }
    }
    lv_obj_set_pos(no_signal_rect, p_h, p_v);
}

static void timer_handle1(lv_timer_t* timer_){
    timer_start = NULL;
    lv_obj_clean(hdmi_scr);
    hdmi_screen_plugoutin();
    if ( hdmirx_get_plug_status() == HDMI_RX_STATUS_PLUGIN){
    	lv_obj_set_style_bg_opa(hdmi_scr, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_add_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN);        
    }
    
}

static void hdmi_event_handle(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SCREEN_LOAD_START){

    #ifdef __HCRTOS__
        //Lower the task timeslice to speed up the realtime performance. so that
        //lower the video delay of hdmi in
        vTaskEnableGlobalTimeSlice();
        vTaskSetGlobalTimeSlice(1);
    #endif    

        key_set_group(hdmi_g);
        hdmi_screen_start();
        hdmi_rx_enter();
        timer_start = lv_timer_create(timer_handle1, 3000, 0);
        lv_timer_set_repeat_count(timer_start, 1);
        lv_timer_reset(timer_start);

        if (hdmirx_get_plug_status() == HDMI_RX_STATUS_PLUGIN){
            lv_obj_set_style_bg_opa(hdmi_scr, LV_OPA_TRANSP, LV_PART_MAIN);
        }
        else {
            lv_obj_set_style_bg_opa(hdmi_scr, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_bg_color(hdmi_scr, lv_color_hex(0x0000ff), LV_PART_MAIN);
        }


    }else if(code == LV_EVENT_SCREEN_UNLOAD_START){

    #ifdef __HCRTOS__
        //Recover the task timeslice
        vTaskDisableGlobalTimeSlice();
    #endif    

#ifdef HDMI_RX_CEC_SUPPORT
        api_hotkey_disable_clear();
#endif

        if(timer){
            lv_timer_del(timer);
            timer = NULL;
        }
        if(timer_start){
            lv_timer_del(timer_start);
            timer_start = NULL;
        }

        //hdmirx_pause();//hdmi_rx_leave();		// move to sourc change in projector.c
        if(timer_err_input){
            lv_timer_ready(timer_err_input);
        }
        lv_obj_clean(hdmi_scr);
        if(lv_obj_has_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN)){
            lv_obj_clear_flag(hdmi_scr, LV_OBJ_FLAG_HIDDEN);
        }


   		lv_obj_set_style_bg_opa(hdmi_scr, LV_OPA_TRANSP, LV_PART_MAIN);

    }else if(code == LV_EVENT_KEY){
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        uint32_t v_key = key_convert_vkey(key);
        printf(" vkey : %d\n",  v_key);

#ifdef HDMI_RX_CEC_SUPPORT
        if(hdmirx_cec_dev_active_status_get() == 1){
            if(hdmirx_cec_boardcast_en_get() == 1){
                _hdmirx_key_boardcast_proc(v_key);
            }else{
                _hdmirx_key_preproc(v_key);
            }
        }else{
            if(v_key == V_KEY_EXIT){
                change_screen(SCREEN_CHANNEL_MAIN_PAGE);
            }
        }
#else
        if(v_key == V_KEY_EXIT){
            change_screen(SCREEN_CHANNEL_MAIN_PAGE);
        }
#endif
    }
}

static void hdmi_screen_start(void){
    lv_obj_t *obj = lv_obj_create(hdmi_scr);

    lv_obj_set_style_bg_color(obj, lv_palette_lighten(LV_PALETTE_BLUE, 1), 0);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(8));
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t* label = lv_label_create(obj);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(label, api_rsc_string_get(STR_HDMI_TITLE));
    lv_obj_set_style_text_font(label, osd_font_get(FONT_MID),0);
}



static void hdmi_screen_plugoutin(void){
    no_signal_rect = lv_obj_create(hdmi_scr);
    
    lv_obj_set_size(no_signal_rect, LV_PCT(NO_SIG_RECT_W), LV_PCT(NO_SIG_RECT_H));
    lv_obj_set_style_bg_color(no_signal_rect, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_radius(no_signal_rect, 5, 0);
    timer = lv_timer_create(timer_handle, 1000, NULL);
    lv_timer_set_repeat_count(timer, -1);
    lv_timer_ready(timer);

    lv_obj_t *label = lv_label_create(no_signal_rect);
    lv_obj_center(label);
    lv_label_set_text(label, api_rsc_string_get(STR_NO_SIGNAL));

    lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);
}


void hdmi_screen_init(void)
{
    hdmi_scr = lv_obj_create(NULL);
    hdmi_g = lv_group_create();
    lv_group_t *g = lv_group_get_default();
    lv_group_set_default(hdmi_g);
    lv_group_add_obj(hdmi_g, hdmi_scr);

    lv_obj_set_style_bg_opa(hdmi_scr, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_add_event_cb(hdmi_scr, hdmi_event_handle, LV_EVENT_ALL, 0);

    h = lv_disp_get_ver_res(lv_disp_get_default());
    w = lv_disp_get_hor_res(lv_disp_get_default());

    lv_group_set_default(g);
   
}

