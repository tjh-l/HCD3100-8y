#include "key_event.h"
#include "lvgl.h"
#include "lvgl/lvgl.h"
//#include "ba.h"
#include "screen.h"
#include "osd_com.h"
#include "hcstring_id.h"
#include "../include/com_api.h"


#include "../../../../liblvgl/source/lvgl/src/widgets/lv_label.h"
#include "bluetooth.h"
#include "app_config.h"
#include <time.h>

#include <hcuapi/gpio.h>
#include "factory_setting.h"
#ifdef SUPPORT_BLUE_RECEIVE
LV_IMG_DECLARE(cj_ba);
lv_obj_t *ba_scr;
lv_group_t *ba_g;
#ifdef SUPPORT_BA_BTN_ON_OFF
lv_obj_t *btnrr_sw;
lv_obj_t *btn_sw;
lv_obj_t *sw_i;
lv_obj_t *labelr_sw;
lv_obj_t *labelrr_sw;
static bool is_close = 1;
#endif
lv_obj_t* device_ba_name;
static lv_obj_t* device_local_name;
lv_obj_t* ba_local_set;
lv_obj_t* ba_local_set1;
static void ba_event_mainpage(lv_event_t *e);
extern int ba_connect;
//extern SCREEN_TYPE_E cur_scr;

void ba_event_handle(lv_event_t *event){
	lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t*  target=lv_event_get_target(event);
	if(code == LV_EVENT_REFRESH){
		if(ba_connect == 1)
        {
        	
			//printf("==========BT_CONNECT_STATUS_CONNECTED================%d",ba_connect);
			lv_label_set_text_fmt(device_ba_name, "%s", "Connected");
			//lv_obj_clean(device_ba_name);
        	//device_ba_name = lv_label_create(ba_scr);
			 //set_label_text2(device_ba_name,STR_BT_CONNECTED,FONT_NORMAL);
		}else{
			lv_label_set_text_fmt(device_ba_name, "%s", "No connection");
			//lv_obj_clean(device_ba_name);
        	//device_ba_name = lv_label_create(ba_scr);
			//set_label_text2(device_ba_name,STR_BT_DISCONNECTED,FONT_NORMAL);
			//printf("==========DISCONNECTED================%d",ba_connect);
		}
	}
}

int create_ba_scr(void)
{	ba_local_set = lv_label_create(ba_scr);
	lv_obj_set_size(ba_local_set,400,60);
	lv_obj_set_style_border_width(ba_local_set,0,0);
	lv_obj_set_style_border_opa(ba_local_set,0,0);
	lv_obj_align(ba_local_set,LV_ALIGN_TOP_LEFT,80,230);//110 230
	lv_obj_set_style_text_opa(ba_local_set,0,0);

	device_ba_name = lv_label_create(ba_scr);
	lv_obj_set_size(device_ba_name,400,60);
	lv_obj_align_to(device_ba_name,ba_local_set,LV_ALIGN_OUT_TOP_MID,0,0);
	set_label_text2(device_ba_name,STR_BT_SETTING, FONT_MID);
	lv_label_set_text_fmt(device_ba_name, "%s", "No connection");
	lv_obj_add_event_cb(device_ba_name, ba_event_handle, LV_EVENT_ALL, NULL);
	//lv_label_set_text_fmt(device_ba_name, "%s", "Connected");
	lv_obj_set_style_text_color(device_ba_name, lv_color_hex(0xffffff),LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(device_ba_name, LV_TEXT_ALIGN_CENTER,LV_STATE_DEFAULT);

	ba_local_set1 = lv_label_create(ba_scr);
	lv_obj_set_size(ba_local_set1,400,60);
	lv_obj_set_style_border_width(ba_local_set1,0,0);
	lv_obj_set_style_border_opa(ba_local_set1,0,0);
	lv_obj_align(ba_local_set1,LV_ALIGN_TOP_LEFT,780,230);//790 230
	lv_obj_set_style_text_opa(ba_local_set1,0,0);

	device_local_name = lv_label_create(ba_scr);
	lv_obj_set_size(device_local_name,400,60);
	lv_obj_align_to(device_local_name,ba_local_set1,LV_ALIGN_OUT_TOP_MID,0,0);
	//set_label_text2(device_local_name,STR_BA, FONT_MID);
	set_label_text2(device_local_name,STR_BT_SETTING, FONT_MID);

	lv_label_set_text_fmt(device_local_name, "%s", "SRT656");//SRT656	
	
	lv_obj_set_style_text_color(device_local_name, lv_color_hex(0xffffff),LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(device_local_name, LV_TEXT_ALIGN_CENTER,LV_STATE_DEFAULT);


#ifdef SUPPORT_BA_BTN_ON_OFF
	lv_obj_t *sw_obj = lv_obj_create(ba_scr);
	lv_obj_set_width(sw_obj, 880);
	lv_obj_set_height(sw_obj, 60);
	lv_obj_set_pos(sw_obj, 200, 150);
	lv_obj_set_style_bg_opa(sw_obj, 0, 0);
	lv_obj_set_style_border_color(sw_obj, lv_color_hex(0x0ACAE1), 0);
	lv_obj_set_style_radius(sw_obj, 50, LV_STATE_DEFAULT);

	labelr_sw = lv_label_create(sw_obj);
	lv_obj_align(labelr_sw, LV_ALIGN_LEFT_MID, 0, 0);
	set_label_text2(labelr_sw, STR_BA, FONT_MID);
	lv_obj_set_style_text_color(labelr_sw, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
	lv_obj_set_style_pad_hor(labelr_sw, 10, 0);

	btn_sw = lv_btn_create(sw_obj);
	lv_obj_align(btn_sw, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_obj_set_style_bg_opa(btn_sw, 0, 0);
	lv_obj_add_event_cb(btn_sw, ba_event_mainpage, LV_EVENT_ALL, 0);

	labelrr_sw = lv_label_create(btn_sw);
	lv_obj_center(labelrr_sw);
	lv_obj_set_style_text_align(labelrr_sw, LV_TEXT_ALIGN_CENTER,LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelrr_sw, lv_color_hex(0xffffff),LV_STATE_DEFAULT);

	sw_i = lv_switch_create(btn_sw);
	lv_obj_set_style_text_align(labelrr_sw, LV_TEXT_ALIGN_CENTER,
				    LV_STATE_DEFAULT);
	lv_obj_set_size(sw_i, 76, 45);

	if (is_close) {
		set_label_text2(labelrr_sw, STR_OFF, FONT_LARGE);
		lv_obj_clear_state(sw_i, LV_STATE_CHECKED);

	}else {
		set_label_text2(labelrr_sw, STR_ON, FONT_LARGE);
		lv_obj_add_state(sw_i, LV_STATE_CHECKED);
	}
#endif

	return 0;
}

int clear_ba_scr(void)
{
	lv_obj_clean(ba_scr);
	return 0;
}

void set_bakey_group(lv_group_t * group)
{
	lv_indev_set_group(indev_keypad, group);
	lv_group_set_default(group);
    lv_group_add_obj(group,ba_scr);
}

int ba_scr_open(void)
{
	ba_g = lv_group_create();
	set_bakey_group(ba_g);
	lv_obj_set_style_bg_img_src(ba_scr,&cj_ba,0);
	//api_logo_show(BA_LOGO);
	create_ba_scr();
	#ifdef SUPPORT_BT_RECEIVE_MODE
	int bt_model_onoff = projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_ON ? STR_ON : STR_OFF;
	if(bt_model_onoff == STR_OFF){
		projector_set_some_sys_param(P_BT_SETTING,BLUETOOTH_ON);
		bluetooth_poweron();
		usleep(30000);
		bluetooth_stop_scan();		
	} 		
	usleep(20000);
	ba_rx_support();
	usleep(2000);
#ifdef SUPPORT_INPUT_BLUE_SPDIF_IN	
	bluetooth_set_volume_level(projector_get_some_sys_param(P_VOLUME));      
#endif 	
#if 1//def CONFIG_SUPPORT_TM_S2_V12_BOARD
	gpio_configure(PINPAD_T05, GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_T05, 1);
	printf("%s %d \n",__FUNCTION__,__LINE__);
	//api_set_i2so_gpio_mute(0);
#else
	gpio_configure(PINPAD_T05, GPIO_DIR_OUTPUT);
	gpio_set_output(PINPAD_T05, 0);	
	printf("%s %d \n",__FUNCTION__,__LINE__);
	api_set_i2so_gpio_mute(0)
#endif 	
	#endif	
	return 0;
}

int ba_scr_close(void)
{
	ba_connect = 0;
//    lv_group_remove_all_objs(ba_g);
//    lv_group_del(ba_g);
    clear_ba_scr();
    //api_logo_off();
	#ifdef SUPPORT_BT_RECEIVE_MODE
	int bt_model_onoff = projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_ON ? STR_ON : STR_OFF;
	if(bt_model_onoff == STR_ON){
		ba_tx_support();
	}else if(bt_model_onoff == STR_OFF){
		//ba_tx_support();//zhp 2023-09-20
		bluetooth_poweroff();//zhp 2023-09-19
	}    	
	#endif
#ifdef SUPPORT_INPUT_BLUE_SPDIF_IN	
	// bluetooth_set_volume_level(100);      
#endif 	
    return 0;
}

static void ba_event_mainpage(lv_event_t *e)
{
	lv_event_code_t event = lv_event_get_code(e);
	lv_obj_t *ta = lv_event_get_target(e);
	if (event == LV_EVENT_SCREEN_LOAD_START) {
		ba_scr_open();
        lv_group_focus_obj(ba_scr);
	}
	if (event == LV_EVENT_SCREEN_UNLOAD_START) {
		ba_scr_close();
	}
	if (event == LV_EVENT_KEY) {
		int key = lv_indev_get_key(lv_indev_get_act());
		//if (key == LV_KEY_ESC || key == LV_KEY_HOME) 
		if (key == LV_KEY_ESC || key == LV_KEY_HOME) 
		{
			//change_screen(SCREEN_MORE);
			printf("%s %d \n",__FUNCTION__,__LINE__);
  		    projector_set_some_sys_param(P_CUR_CHANNEL, SCREEN_CHANNEL_MAIN_PAGE);
			change_screen(SCREEN_CHANNEL_MAIN_PAGE);
		} 
		if (key == LV_KEY_ENTER || key == LV_KEY_LEFT || key == LV_KEY_RIGHT || key == LV_KEY_UP || key == LV_KEY_DOWN) {
			lv_event_send(device_ba_name,LV_EVENT_REFRESH,0);
		}
#ifdef SUPPORT_BA_BTN_ON_OFF
		else if (key == LV_KEY_ENTER) {
			//if(bluetooth_poweron() == 1){
			is_close = is_close == 1 ? 0 : 1;
			if (is_close == 1) {
				set_label_text2(labelrr_sw, STR_OFF, FONT_LARGE);
				lv_obj_clear_state(sw_i, LV_STATE_CHECKED);
				#ifdef SUPPORT_BT_RECEIVE_MODE
				ba_tx_support();
				//bluetooth_poweroff();
#ifdef CONFIG_SUPPORT_TM_S2_V12_BOARD
				gpio_configure(PINPAD_T05, GPIO_DIR_OUTPUT);
				gpio_set_output(PINPAD_T05, 1);
#else				
				gpio_configure(PINPAD_T05, GPIO_DIR_OUTPUT);
				gpio_set_output(PINPAD_T05, 0);
#endif 				
				#endif
				//bluetooth_set_bt_off_support();
				// lv_obj_clear_state(sw_i,LV_STATE_CHECKED);
			}

			else {
				set_label_text2(labelrr_sw, STR_ON, FONT_LARGE);
				#ifdef SUPPORT_BT_RECEIVE_MODE
				bluetooth_poweron();
				usleep(3000);
				ba_rx_support();
				#ifdef SUPPORT_BA_BTN_ON_OFF
				lv_obj_add_state(sw_i, LV_STATE_CHECKED);
				#endif
#ifdef CONFIG_SUPPORT_TM_S2_V12_BOARD
				gpio_configure(PINPAD_T05, GPIO_DIR_OUTPUT);
				gpio_set_output(PINPAD_T05, 0);
#else					
				 gpio_configure(PINPAD_T05, GPIO_DIR_OUTPUT);
				 gpio_set_output(PINPAD_T05, 1);
				 //api_set_i2so_gpio_mute(1);
#endif 				
				#endif
				// lv_obj_add_state(sw_i,LV_STATE_CHECKED);
			}
		//}
		}
#endif
	}
}


void ba_screen_init(void)
{
    // just create a screen and add event ,weight display in event cb  
    ba_scr = lv_obj_create(NULL);
    lv_obj_clear_flag(ba_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ba_scr, ba_event_mainpage, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(ba_scr, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ba_scr, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

}
// lv_timer_t * baconnect_refresh = NULL;
// void baconnect_refresh_cb(lv_timer_t * tmr)
// {
// 	lv_event_send(device_ba_name,LV_EVENT_REFRESH,0);
// 	usleep(3000);
// 	baconnect_refresh = lv_timer_create(baconnect_refresh_cb, 1000, 0); 
// 	printf("++++++++++++++++baconnect_refresh_cb+++++++++++++++\n");
// }

// static int ba_connect_refresh()
// {
// 	lv_event_send(device_ba_name,LV_EVENT_REFRESH,0);
// 	baconnect_refresh = lv_timer_create(baconnect_refresh_cb, 1000, 0); 
// 	printf("===============baconnect_refresh============\n");
// 	return 0;
// }

#endif




