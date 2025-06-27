// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.0.5
// LVGL VERSION: 8.2
// PROJECT: ProjectorSetting

#ifndef _PROJECTORSETTING_UI_H
#define _PROJECTORSETTING_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"

#if __has_include("lvgl.h")
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include <hcuapi/dis.h>


typedef enum flip_mode{
    FLIP_MODE_REAR,  //FLIP_MODE_NORMAL,
    FLIP_MODE_CEILING_REAR,  //FLIP_MODE_ROTATE_180, 
    FLIP_MODE_FRONT, //FLIP_MODE_H_MIRROR, 
    FLIP_MODE_CEILING_FRONT, //FLIP_MODE_V_MIRROR, 
}flip_mode_e;
typedef enum SCREEN_TYPE{
	SCREEN_CHANNEL_CVBS,
	SCREEN_CHANNEL_HDMI,
#ifdef HDMI_SWITCH_SUPPORT
	SCREEN_CHANNEL_HDMI2,
#endif
	SCREEN_CHANNEL_MP,
	SCREEN_CHANNEL_MAIN_PAGE,
	SCREEN_CHANNEL_MP_PLAYBAR,
	SCREEN_CHANNEL_MP_EBOOK,
	SCREEN_CHANNEL,
	SCREEN_SETUP,
	SCREEN_VOLUME,
	
#ifdef USBMIRROR_SUPPORT	
	SCREEN_CHANNEL_USB_CAST,
#endif	
#ifdef WIFI_SUPPORT
	SCREEN_WIFI,
	SCREEN_CHANNEL_WIFI_CAST,
#endif
#ifdef HCIPTV_YTB_SUPPORT
	SCREEN_WEBSERVICE,
	SCREEN_WEBPLAYER,
#endif
#ifdef SUPPORT_BLUE_RECEIVE                    
       SCREEN_BA,
#endif 
#ifdef HC_FACTORY_TEST
	SCREEN_FACTORY_TEST,
	SCREEN_FTEST_FULL_COLOR_DIS,
#endif
#ifdef AIRP2P_SUPPORT
        SCREEN_AIRP2P,
#endif
}SCREEN_TYPE_E;
extern SCREEN_TYPE_E last_scr;
extern SCREEN_TYPE_E prev_scr;
extern lv_obj_t *channel_scr;  
extern lv_obj_t *setup_scr;

extern lv_obj_t *volume_scr; 
extern lv_obj_t *volume_bar;
extern lv_obj_t *ui_mainpage; 
 extern lv_obj_t * ui_fspage;
 extern lv_obj_t * ui_subpage;
 extern lv_obj_t *ui_ctrl_bar;
 extern lv_indev_t* indev_keypad;
extern lv_obj_t *hdmi_scr;  
extern lv_obj_t *cvbs_scr; 
extern lv_obj_t *main_page_scr;
extern lv_obj_t * ui_ebook_txt;

extern lv_group_t *channel_g;
extern lv_group_t *setup_g;
extern lv_group_t *volume_g;
extern lv_group_t *mp_g;
extern lv_group_t *hdmi_g;
extern lv_group_t *cvbs_g;
extern lv_group_t *main_page_g;
extern bool is_mute;

#ifdef HCIPTV_YTB_SUPPORT
extern lv_obj_t *webplay_scr;
#endif

#ifdef AIRP2P_SUPPORT
extern lv_obj_t *ui_airp2p_cast_root;
extern void ui_cast_airp2p_init(void);
#endif

#ifdef USBMIRROR_SUPPORT	
extern lv_obj_t *ui_um_play;
extern lv_obj_t *ui_um_upgrade;
#endif
extern uint32_t act_key_code;

#ifdef USB_MIRROR_FAST_SUPPORT
extern lv_obj_t *ui_um_fast;
#endif

#ifdef WIFI_SUPPORT

extern lv_obj_t *wifi_scr;
extern lv_group_t *wifi_g;
extern lv_obj_t *ui_wifi_cast_root;
extern lv_obj_t *ui_network_upgrade;
void ui_network_upgrade_init(void);
#endif

#ifdef CAST_SUPPORT
extern lv_obj_t *ui_wifi_cast_root;
#endif

#ifdef DLNA_SUPPORT
extern lv_obj_t *ui_cast_dlna;
void ui_cast_dlna_init(void);
#endif

#if defined(AIRCAST_SUPPORT) || defined(MIRACAST_SUPPORT)
extern lv_obj_t *ui_cast_play;
void ui_cast_play_init(void);
#endif

#ifdef USB_MIRROR_FAST_SUPPORT
void ui_um_fast_init(void);
#endif


#ifdef HC_FACTORY_TEST
extern lv_obj_t *factory_settings_scr;
extern void factory_test_enter_check(uint16_t key);
extern void ui_factory_settings_init(void);
#endif

#ifdef BATTERY_SUPPORT
extern lv_obj_t* power_label;
extern int battery_screen_init(void);  //read pm state ,and sand LVGL event message
#endif

void change_screen(enum SCREEN_TYPE stype);
void _ui_screen_change(lv_obj_t * target,  int spd, int delay);
int projector_lvgl_start(int argc, char **argv);
void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

int set_flip_mode(flip_mode_e mode);
void get_rotate_by_flip_mode(flip_mode_e mode ,
                             int *p_rotate_mode ,
                             int *p_h_flip ,
                             int *p_v_flip);
void hdmi_rx_set_flip_mode(int rotate_type , int mirror_type);
void cvbs_rx_set_flip_mode(int rotate_type , int mirror_type);
//int is_epg_code();

extern void channel_screen_init(void);
extern void setup_screen_init(void);
extern void hdmi_screen_init(void);
extern void cvbs_screen_init(void);
extern void main_page_init(void);
extern void wifi_screen_init();
extern void volume_screen_init(void);
extern void ui_mainpage_screen_init(void);
extern void ui_subpage_screen_init(void);
extern void ui_fspage_screen_init(void);
extern void ui_ctrl_bar_screen_init(void);
extern void ui_ebook_screen_init(void);
extern void del_volume();
#ifdef LVGL_MBOX_STANDBY_SUPPORT
extern void close_volume_for_open_lvmbox_standby();
#endif
extern void BT_first_power_on();
uint8_t set_next_flip_mode(void);
extern void keystone_screen_init(lv_obj_t *btn);
extern void set_keystone_disable(bool en);
void change_keystone(void);
int hdmi_rx_enter(void);
extern int hdmi_rx_leave(void);
int hdmirx_resume(void);
int hdmirx_pause(void);
void enter_standby(void);
#ifdef WIFI_SUPPORT
void ui_wifi_cast_init(void);
#endif
#ifdef USBMIRROR_SUPPORT
void ui_um_play_init(void);
void ui_um_upgrade_init(void);
#ifdef USB_MIRROR_FAST_SUPPORT
  void ui_um_fast_proc(uint32_t msg_type);
  bool um_service_off_by_menu(lv_obj_t *scr);
  bool um_service_on_by_menu(lv_obj_t *scr);
  void um_service_wait_exit(void);
#endif
#endif
#ifdef MULTI_OS_SUPPORT
int avparam_data_init(void);
int avparam_set_rotate_param(int rotate, int h_flip, int v_flip);
int store_avparam(void);
//int avparam_set_color_temp_param(void *pq_setting);
#endif
int get_ir_key(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
