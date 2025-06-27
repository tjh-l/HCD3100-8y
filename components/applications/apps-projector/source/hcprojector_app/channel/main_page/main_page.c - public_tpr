
#include "app_config.h"

#include <stdio.h>
#include "setup.h"
#include "factory_setting.h"
#include "mul_lang_text.h"
#include "hcstring_id.h"
#include "main_page.h"
#include "com_api.h"
#include "osd_com.h"
#include "screen.h"
#include "../../app_config.h"
#ifdef HCIPTV_YTB_SUPPORT
#include "../webplayer/webdata_mgr.h"
#endif 
#include "glist.h"
//#include "network_api.h"

#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif

//lv_obj_t *btn_meida = NULL;
//lv_obj_t *btn_setting = NULL;
//lv_obj_t *btn_ytb = NULL;
//lv_obj_t *btn_multi_os = NULL;
//lv_obj_t *btn_airp2p = NULL;

#ifdef BLUETOOTH_SUPPORT
    //LV_IMG_DECLARE(bt_con);
    //LV_IMG_DECLARE(bt_discon);
	LV_IMG_DECLARE(public_bt);
	LV_IMG_DECLARE(public_bt_no);
    lv_obj_t *bt_show;
#endif


lv_obj_t* prompt_show;
lv_obj_t *wifi_show;
lv_obj_t *usb_show;


LV_IMG_DECLARE(null_source);  
LV_IMG_DECLARE(null_media);  
LV_IMG_DECLARE(null_wifi);  
LV_IMG_DECLARE(null_wrieless);  
LV_IMG_DECLARE(null_wired);  
LV_IMG_DECLARE(null_setting);  

LV_IMG_DECLARE(public_usb);
//LV_IMG_DECLARE(public_tf);  
LV_IMG_DECLARE(public_wifi);  
LV_IMG_DECLARE(public_usb_no);
//LV_IMG_DECLARE(public_tf_no);  
LV_IMG_DECLARE(public_wifi_no);


lv_obj_t *btn_channel = NULL;
lv_obj_t *btn_meida = NULL;
lv_obj_t *btn_wifi = NULL;
lv_obj_t *btn_wireless = NULL;
lv_obj_t *btn_wired = NULL;
lv_obj_t *btn_setting = NULL;


lv_obj_t** btns[] = {&btn_channel, &btn_meida, &btn_wifi ,&btn_wireless, &btn_wired,  &btn_setting};
void * imgs[] = {&null_source, &null_media, &null_wifi,  &null_wrieless, &null_wired, &null_setting};


#define BTNS_SIZE (sizeof(btns)/sizeof(btns[0]))

int r[] = {  1, 249,   0, 162, 251,  74};
int g[] = {138,  56, 191,   3, 105,  97};
int b[] = {213,  33,  79, 237,  19, 156};            
int text_ids[] = {STR_INPUT_SOURCE,STR_MEDIA_TITLE,STR_WIFI_TITLE,STR_WIRELESS_TITLE, STR_WIRED_TITLE,STR_SETTING_TITLE};
int x[] = 	{-40, 320, 750, 320, 570, 820};
int y[] = 	{ -2,   2,   2, 248, 248, 248};
int k[] = 	{350, 420, 310, 240, 240, 240};
int gao[] = {490, 240, 240, 240, 240, 240};

#if 1
#if defined(CAST_SUPPORT) || defined(WIFI_SUPPORT)
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_dhcpd.h>
#include "include/network_api.h"
LV_IMG_DECLARE(wifi_limited);
#endif
#else
#if defined(CAST_SUPPORT) || defined(WIFI_SUPPORT)
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_dhcpd.h>
#include "include/network_api.h"

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define WIFI_SHOW_FONT SiYuanHeiTi_Light_3500_12_1b
    #define WIFI_SHOW_IMG_GROUP 2
    #define WIFI_NAME_SHOW 0
#else
    #define WIFI_SHOW_FONT LISTFONT_3000
    #define WIFI_SHOW_IMG_GROUP 2
    #define WIFI_NAME_SHOW 1
#endif

lv_obj_t *wifi_show;

lv_obj_t *btn_channel;
lv_obj_t *btn_wired;
lv_obj_t *btn_wireless;
lv_obj_t *btn_wifi;

extern lv_obj_t* wifi_lists;
extern hccast_wifi_ap_info_t cur_conne;
extern hccast_wifi_ap_info_t *cur_conne_p;

lv_obj_t** btns[] = {
    &btn_channel, 
    &btn_wired,  
    &btn_wifi, 
    &btn_wireless, 
    &btn_meida, 
    &btn_setting, 
#ifdef HCIPTV_YTB_SUPPORT        
    &btn_ytb,
#endif        
#ifdef MULTI_OS_SUPPORT
    &btn_multi_os,
#endif
#ifdef AIRP2P_SUPPORT
    &btn_airp2p,
#endif    
    };
    
#define BTNS_SIZE (sizeof(btns)/sizeof(btns[0]))

static int r[] = {
    133,
    91,
    81,
    60,
    50,
    255,
#ifdef HCIPTV_YTB_SUPPORT     
    0,
#endif    
#ifdef MULTI_OS_SUPPORT
    25,
#endif
#ifdef AIRP2P_SUPPORT
    128,
#endif    
    };
    
static int g[] = {
    105,
    75,
    125,
    113,
    139,
    201,
#ifdef HCIPTV_YTB_SUPPORT    
    64,
#endif    
#ifdef MULTI_OS_SUPPORT
    68,
#endif
#ifdef AIRP2P_SUPPORT
    128,
#endif    
    };
    
static int b[] = {
    54,
    112,
    60,
    128,
    137,
    14,
#ifdef HCIPTV_YTB_SUPPORT     
    64,
#endif  
#ifdef MULTI_OS_SUPPORT
    95,
#endif
#ifdef AIRP2P_SUPPORT
    192,
#endif    
    };
    
static int text_ids[] = {
    STR_INPUT_SOURCE,
    STR_WIRED_TITLE,
    STR_WIFI_TITLE,
    STR_WIRELESS_TITLE, 
    STR_MEDIA_TITLE,
    STR_SETTING_TITLE,
#ifdef HCIPTV_YTB_SUPPORT    
    STR_IPTV_YTB, 
#endif    
#ifdef MULTI_OS_SUPPORT
    STR_MULTI_OS,
#endif
#ifdef AIRP2P_SUPPORT
    STR_AIRP2P,
#endif    
    };


LV_IMG_DECLARE(wifi4);
LV_IMG_DECLARE(wifi3);
LV_IMG_DECLARE(wifi2);
LV_IMG_DECLARE(wifi1);
LV_IMG_DECLARE(wifi0);
LV_IMG_DECLARE(wifi_no);
LV_IMG_DECLARE(wifi_suspend);
LV_IMG_DECLARE(wifi_limited);


// LV_IMG_DECLARE(input_source_icon);
// LV_IMG_DECLARE(internet_icon);
// LV_IMG_DECLARE(media_icon);
// LV_IMG_DECALRE(message_icon);
// LV_IMG_DECALRE(setting_icon);
// LV_IMG_DECLARE(wifi_cast_icon);

//lv_font_t **icons[] = {};
extern void set_wifi_had_con_by_cast(bool b);
extern bool wifi_is_conning_get();

#else // else defined(CAST_SUPPORT) || defined(WIFI_SUPPORT)

lv_obj_t *btn_hdmi = NULL;
lv_obj_t *btn_av = NULL;

lv_obj_t** btns[] = {
    &btn_hdmi, 
    &btn_av, 
    &btn_meida, 
    &btn_setting, 
#ifdef MULTI_OS_SUPPORT    
    &btn_multi_os,
#endif    
    };
    
#define BTNS_SIZE (sizeof(btns)/sizeof(btns[0]))

static int r[] = {
    133,
    91,
    81,
    60,
#ifdef MULTI_OS_SUPPORT    
    25,
#endif    
    };
    
static int g[] = {
    105,
    75,
    125,
    113,
#ifdef MULTI_OS_SUPPORT    
    68,
#endif    
    };
    
static int b[] = {
    54,
    112,
    60,
    128,
#ifdef MULTI_OS_SUPPORT    
    95,
#endif    
    };
    
static int text_ids[] = {
    STR_HDMI_TITLE, 
    STR_AV_TITLE, 
    STR_MEDIA_TITLE, 
    STR_SETTING_TITLE, 
#ifdef MULTI_OS_SUPPORT    
    STR_MULTI_OS,
#endif    
    };


#endif

#endif


//LV_FONT_DECLARE(font40_china);
lv_obj_t *main_page_scr = NULL;
lv_group_t *main_page_g = NULL;
extern lv_font_t *select_font_normal[3];
lv_font_t *main_page_font[3];



static void event_handle(lv_event_t *e);
void main_page_init();

static void event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if(code == LV_EVENT_KEY){
        uint8_t key = lv_indev_get_key(lv_indev_get_act());
        int index = lv_obj_get_index(target);
        if(key == LV_KEY_LEFT){
			index = index == 0 ? BTNS_SIZE - 1 : index == 3 ? 0 : index-1;
            //if ((BTNS_SIZE%2) && (BTNS_SIZE > 2))
            //{
            //    index = index-2>=0 ? (index==BTNS_SIZE-1 ? index-1 : index-2) : (index==1 ? BTNS_SIZE-3 : BTNS_SIZE-1);
            //}
            //else
            //{
            //    index = index == 0 ? BTNS_SIZE-1 : index == 1 ? BTNS_SIZE-2 : index-2;
            //}
            
            lv_group_focus_obj(*btns[index]);           
        }else if(key == LV_KEY_RIGHT){
			index = index == BTNS_SIZE - 1 ? 0 : index + 1;
            //if ((BTNS_SIZE%2) && (BTNS_SIZE > 2))
            //{
            //    index = index+2<BTNS_SIZE ? index+2 : (index+1<BTNS_SIZE ? index+1 : 0);
            //}
            //else
            //{
            //    index = index == BTNS_SIZE-2 ? 1 : BTNS_SIZE-1 == index ? 0 : index+2;
            //}
            
            lv_group_focus_obj(*btns[index]);           
        }else if(key == LV_KEY_ENTER){
            if(target == btn_meida){
                change_screen(SCREEN_CHANNEL_MP);
                projector_set_some_sys_param(P_CUR_CHANNEL, SCREEN_CHANNEL_MP);
            }else if(target == btn_setting){
                change_screen(SCREEN_SETUP);
            }
        #ifdef AIRP2P_SUPPORT     
            else if(target == btn_airp2p){
                if(projector_get_some_sys_param(P_WIFI_ONOFF))
                {
                    change_screen(SCREEN_AIRP2P);
                }
                else
                {
                    win_msgbox_msg_open(STR_WIFI_NOT_CONNECT, 3000, NULL, NULL);
                }    
            }
        #endif     
        #ifdef MULTI_OS_SUPPORT
            else if(target == btn_multi_os){
                find_icube_enter_multi_os();
            }
        #endif    
        #ifdef CAST_SUPPORT
            else if(target == btn_wifi){				
#ifdef WIFI_SUPPORT
                api_wifi_pm_stop();
                change_screen(SCREEN_WIFI);
#endif
            }
            else if(target == btn_wired){
#ifdef USBMIRROR_SUPPORT	
                change_screen(SCREEN_CHANNEL_USB_CAST);
#endif
            }
            else if(target == btn_wireless){
#ifdef WIFI_SUPPORT
                //if (!projector_get_some_sys_param(P_WIFI_ONOFF))
                //{
                //    win_msgbox_msg_open(STR_WIFI_NOT_CONNECT, 3000, NULL, NULL);
                //}
                //else
                {
                    api_wifi_pm_stop();
                    change_screen(SCREEN_CHANNEL_WIFI_CAST);
                }                
#endif
            }else if(target == btn_channel){
                change_screen(SCREEN_CHANNEL);
            }
        #else
            else if(target == btn_hdmi){
#ifdef HDMIIN_SUPPORT				
                change_screen(SCREEN_CHANNEL_HDMI);
                projector_set_some_sys_param(P_CUR_CHANNEL, SCREEN_CHANNEL_HDMI);
#endif				
            } else if(target == btn_av){
#ifdef CVBSIN_SUPPORT
                change_screen(SCREEN_CHANNEL_CVBS);
                projector_set_some_sys_param(P_CUR_CHANNEL, SCREEN_CHANNEL_CVBS);
#endif
            }
        #endif
#ifdef  HCIPTV_YTB_SUPPORT
            else if(target== btn_ytb){
                api_wifi_pm_stop();
                if(!projector_get_some_sys_param(P_WIFI_ONOFF)){
                    win_msgbox_msg_open(STR_WIFI_NOT_CONNECT, 3000, NULL, NULL);
                }else if(!app_wifi_connect_status_get()){
                    win_msgbox_msg_open(STR_WIFI_NOT_CONNECTING, 3000, NULL, NULL);
                }else{
                    change_screen(SCREEN_WEBSERVICE);
                }
            }
#endif
            projector_sys_param_save();
        }else if(key == LV_KEY_UP || key == LV_KEY_DOWN){
            index = index == 0 ? 0 : index == 1 ? 3 : index == 2 ? 5 : index == 3 ? 1 : index == 4 ? 1 : 2;
            lv_group_focus_obj(*btns[index]);
            // if(index % 2 == 1){
            //     lv_group_focus_obj(*btns[index-1]);
            // }else if(index<BTNS_SIZE-1){
            //     lv_group_focus_obj(*btns[index+1]);
            // }
        }
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        if(main_page_g && target == lv_group_get_focused(main_page_g)){
            //dsc->rect_dsc->outline_color = lv_color_white();
            //dsc->rect_dsc->outline_width = 4;
            //dsc->rect_dsc->outline_pad = 0;

        }else{
             dsc->rect_dsc->outline_width = 0;
        }
        dsc->rect_dsc->shadow_width = 0;
    }else if(code == LV_EVENT_SCREEN_LOADED){
     
    }
}

static void mainpage_scr_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SCREEN_LOADED){
        projector_set_some_sys_param(P_CUR_CHANNEL, SCREEN_CHANNEL_MAIN_PAGE);
        projector_sys_param_save();

        lv_group_set_default(main_page_g);
        lv_event_send(prompt_show,LV_EVENT_REFRESH,0);
    }else if(code == LV_EVENT_SCREEN_UNLOADED){
        win_msgbox_msg_close();
    }
}


#if defined(CAST_SUPPORT) && defined(WIFI_SUPPORT)
static void wifi_show_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if(code == LV_EVENT_REFRESH){
        char *cur_ssid = app_get_connecting_ssid();
		
        if(!obj->user_data || !cur_ssid){
            //if (cur_ssid && cur_ssid[0] != '\0' && api_get_wifi_pm_state()){
            //    lv_img_set_src(lv_obj_get_child(obj, 0), &wifi_suspend);
            //}
            //else{
            //    lv_img_set_src(lv_obj_get_child(obj, 0), &wifi_no);
					lv_img_set_src(lv_obj_get_child(obj, 0), &public_wifi_no);
            //}

            //if(lv_obj_get_child_cnt(obj)>1){
            //    lv_label_set_text(lv_obj_get_child(obj, 1), "");
            //}
            return;
        }
        //if(lv_obj_get_child_cnt(obj)>1){
        //    lv_label_set_text(lv_obj_get_child(obj, 1), cur_ssid);
        //}
        hccast_wifi_ap_info_t * p = sysdata_get_wifi_info(cur_ssid);
        if(!p){
            return;
        }
        //if(app_wifi_is_limited_internet())
        //    lv_img_set_src(lv_obj_get_child(obj, 0), &wifi_limited);
        //else 
        //{
        //    int quality = p->quality;
        //    void *img = (quality>=68) ? &wifi4 :
        //                (quality<68 && quality>=46) ? &wifi3 :
        //                (quality<46 && quality>=24) ? &wifi2 :
        //                (quality<24) ? &wifi1: &wifi1;
        //    lv_img_set_src(lv_obj_get_child(obj, 0), img);
			lv_img_set_src(lv_obj_get_child(obj, 0), &public_wifi);
        //}
    }
}
#endif

#ifdef BLUETOOTH_SUPPORT
static void bt_show_event_handle(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if(code == LV_EVENT_REFRESH){
        char *cur_bt_name = projector_get_bt_name(0);
		if(!obj->user_data || !cur_bt_name){
			lv_img_set_src(lv_obj_get_child(obj, 0), &public_bt_no);
			//lv_img_set_src(lv_obj_get_child(obj, 0), &bt_discon);
			//if(lv_obj_get_child_cnt(obj)>1){
			//	lv_label_set_text(lv_obj_get_child(obj, 1), "");
			//}			
			return;
		}
		
        //if(lv_obj_get_child_cnt(obj)>1){
        //    lv_label_set_text(lv_obj_get_child(obj, 1), cur_bt_name);
        //}
		//lv_img_set_src(lv_obj_get_child(obj, 0), &bt_con);
		lv_img_set_src(lv_obj_get_child(obj, 0), &public_bt);
    }
}

#endif

static void win_main_page_control(void *arg1, void *arg2){
    (void)arg2;
     control_msg_t *ctl_msg = (control_msg_t*)arg1;

    switch (ctl_msg->msg_type){
    #if defined(CAST_SUPPORT) && defined(WIFI_SUPPORT)
        case MSG_TYPE_NETWORK_WIFI_CONNECTED:
            main_page_prompt_status_set(MAIN_PAGE_PROMPT_WIFI, MAIN_PAGE_PROMPT_STATUS_ON);
            lv_event_send(wifi_show, LV_EVENT_REFRESH, NULL);
            break;
        case MSG_TYPE_NETWORK_WIFI_DISCONNECTED:
        case MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL:
        case MSG_TYPE_USB_WIFI_PLUGOUT:
            main_page_prompt_status_set(MAIN_PAGE_PROMPT_WIFI, MAIN_PAGE_PROMPT_STATUS_OFF);
            lv_event_send(wifi_show, LV_EVENT_REFRESH, NULL);
            break;
        case MSG_TYPE_NETWORK_WIFI_MAY_LIMITED:
            if (network_wifi_module_get())
                lv_img_set_src(lv_obj_get_child(wifi_show, 0), &wifi_limited);
            break;
    #endif
    #ifdef BLUETOOTH_SUPPORT   
        case MSG_TYPE_BT_CONNECTED:
            main_page_prompt_status_set(MAIN_PAGE_PROMOT_BT,MAIN_PAGE_PROMPT_STATUS_ON);
            lv_event_send(bt_show, LV_EVENT_REFRESH, NULL);
            break;
        case MSG_TYPE_BT_DISCONNECTED:
             main_page_prompt_status_set(MAIN_PAGE_PROMOT_BT,MAIN_PAGE_PROMPT_STATUS_OFF);
             lv_event_send(bt_show, LV_EVENT_REFRESH, NULL);
             break;
    #endif
        //// for test usb msg
        //case MSG_TYPE_USB_MOUNT:
        //case MSG_TYPE_USB_UNMOUNT:
        //case MSG_TYPE_USB_UNMOUNT_FAIL:
        //case MSG_TYPE_SD_MOUNT:
        //case MSG_TYPE_SD_UNMOUNT:
        //case MSG_TYPE_SD_UNMOUNT_FAIL:
        //    lv_event_send(prompt_show,LV_EVENT_REFRESH,0);
        //    break;
        default:
            break;
    }
}

static void scr_event_handle(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
	char *cur_ssid = NULL;

    if(code == LV_EVENT_SCREEN_LOADED){
        key_set_group(main_page_g);
        #if defined(CAST_SUPPORT) && defined(WIFI_SUPPORT)
            if(!network_wifi_module_get()){
                main_page_prompt_status_set(MAIN_PAGE_PROMPT_WIFI, MAIN_PAGE_PROMPT_STATUS_OFF);
            }else{
                if(app_wifi_connect_status_get()){
                    main_page_prompt_status_set(MAIN_PAGE_PROMPT_WIFI, MAIN_PAGE_PROMPT_STATUS_ON);
                }else{
                    main_page_prompt_status_set(MAIN_PAGE_PROMPT_WIFI, MAIN_PAGE_PROMPT_STATUS_OFF);
                }
            }
            lv_event_send(wifi_show, LV_EVENT_REFRESH, NULL);


        #endif
            #ifdef BLUETOOTH_SUPPORT
                if(app_bt_is_connected()){
                    main_page_prompt_status_set(MAIN_PAGE_PROMOT_BT,MAIN_PAGE_PROMPT_STATUS_ON);
                }else{
                    main_page_prompt_status_set(MAIN_PAGE_PROMOT_BT,MAIN_PAGE_PROMPT_STATUS_OFF);
                }
                lv_event_send(bt_show, LV_EVENT_REFRESH, NULL);
            #endif      
    }
}


static lv_obj_t* main_page_prompt_create(lv_obj_t* parent,void* icon){
	lv_obj_t* show = lv_obj_create(parent);
    //
    lv_obj_set_size(show, LV_PCT(40),LV_PCT(100));
    lv_obj_set_style_border_width(show, 1, 0);
    lv_obj_set_style_pad_hor(show, 0, 0);

    lv_obj_set_scrollbar_mode(show, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(show, LV_OPA_0, 0);
    lv_obj_set_style_text_color(show, lv_color_white(), 0);
    //lv_obj_align(show, LV_ALIGN_TOP_MID, lv_pct(38), 20);
    lv_obj_set_style_border_width(show, 0, 0);
    lv_obj_set_style_pad_all(show, 0, 0);

    lv_obj_set_flex_flow(show, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(show, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    //lv_obj_t *img = lv_img_create(show);
    //lv_img_set_src(img, icon);

#if WIFI_NAME_SHOW
//    lv_obj_set_flex_grow(img, 2);
    lv_obj_t *label = lv_label_create(show);
    lv_label_set_text(label, "");
//    lv_obj_set_flex_grow(label, 17);
    lv_obj_set_style_text_font(label, &WIFI_SHOW_FONT, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
#endif
	
	return show;
}

void main_page_prompt_status_set(main_page_prompt_t t,main_page_prompt_v  v){
	switch (t){
		case MAIN_PAGE_PROMPT_WIFI:
            #if defined(CAST_SUPPORT) && defined(WIFI_SUPPORT)
			wifi_show->user_data = (void*)v;
            #endif
			break;
		case MAIN_PAGE_PROMOT_BT:
            #ifdef BLUETOOTH_SUPPORT
			    bt_show->user_data = (void*)v;
            #endif
			break;
        default:
            break;
	}
}

void mainpage_storage_icon_refresh_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t*  target=lv_event_get_target(event);
    void * user_data= lv_event_get_user_data(event);
    if(code == LV_EVENT_REFRESH)
    {
        partition_info_t * partition_info2=mmp_get_partition_info();


        if(partition_info2 != NULL){
            if(partition_info2->count==1){
            for(int i=0;i<partition_info2->count;i++){
                char *dev_name=glist_nth_data(partition_info2->dev_list,i);

                //rtos: sdaxx; linux: /media/hddxx /media/sdxx
                if(strstr(dev_name,"sd")|| strstr(dev_name,"hd")||strstr(dev_name,"usb")){
                    lv_img_set_src(lv_obj_get_child(lv_obj_get_child(target, 0),0), &public_usb);
                }else{
                    lv_img_set_src(lv_obj_get_child(lv_obj_get_child(target, 0),0), &public_usb_no);
                }
                // if(strstr(dev_name,"mmc")){
                //     lv_img_set_src(lv_obj_get_child(lv_obj_get_child(target, 0),0), &cj_sd);
                // }
            }
            }else if(partition_info2->count==2){
                lv_img_set_src(lv_obj_get_child(lv_obj_get_child(target, 0),0), &public_usb);
            }else if(partition_info2->count==0){
                lv_img_set_src(lv_obj_get_child(lv_obj_get_child(target, 0),0), &public_usb_no);
            }
        }
    }
}

void main_page_init(){
    main_page_scr = lv_obj_create(NULL);
    main_page_g = lv_group_create();
    lv_group_t* d_g = lv_group_get_default();
    lv_group_set_default(main_page_g);
    lv_obj_add_event_cb(main_page_scr, scr_event_handle, LV_EVENT_ALL, 0);


    screen_entry_t main_page_entry;
    main_page_entry.screen = main_page_scr;
    main_page_entry.control = win_main_page_control;
    api_screen_regist_ctrl_handle(&main_page_entry);

    int h = lv_disp_get_hor_res(NULL);
    int v = lv_disp_get_ver_res(NULL);
    printf("h is: %d, v is: %d\n", h, v);

    lv_obj_t *grid = lv_obj_create(main_page_scr);

    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_ver(grid, v/6,0);
    lv_obj_set_style_pad_hor(grid, h/9, 0);

    lv_obj_set_style_bg_color(grid, lv_color_make(0, 0, 255), 0);//blue
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(grid);
    lv_obj_set_size(grid, h, v);
    lv_obj_set_style_radius(grid, 0, 0);
    
    lv_obj_add_event_cb(main_page_scr, mainpage_scr_event_handle, LV_EVENT_ALL, 0);
    //lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_COLUMN_WRAP);
    //lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollbar_mode(main_page_scr, LV_SCROLLBAR_MODE_OFF); 

    // uint16_t col_num = BTNS_SIZE/2+(BTNS_SIZE%2);
    // uint16_t sub_width =  (h/9*7)/col_num;//根据数量计算宽度
    // uint16_t gap_col = sub_width/7*col_num/(col_num-1);
    // sub_width = sub_width*6/7;

    // uint16_t sub_height = (v/3*2)/2-v/36-1;
    // uint16_t gap_row = v/18;

    // lv_obj_set_style_pad_row(grid, gap_row, 0);
    // lv_obj_set_style_pad_column(grid, gap_col, 0);

    lv_obj_t *label;
    for(int i=0; i<BTNS_SIZE; i++){
        *btns[i] = lv_btn_create(grid);
        //lv_obj_set_flex_flow(*btns[i], LV_FLEX_FLOW_COLUMN);
        //lv_obj_set_flex_align(*btns[i], LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_add_event_cb(*btns[i], event_handle, LV_EVENT_ALL, 0);

        lv_obj_set_style_bg_opa(*btns[i], 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_size(*btns[i], k[i], gao[i]);
        lv_obj_set_pos(*btns[i], x[i], y[i]);
        //lv_obj_set_style_bg_color(*btns[i], lv_color_make(r[i], g[i], b[i]), 0);
        lv_obj_set_style_bg_img_src(*btns[i], imgs[i],0);
        lv_obj_clear_flag(*btns[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_shadow_opa(*btns[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT); 
        lv_obj_set_style_outline_color(*btns[i], lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(*btns[i], 5, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_opa(*btns[i], 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        //lv_obj_set_style_border_color(*btns[i], lv_color_hex(0xffffff), LV_STATE_FOCUS_KEY);
        //lv_obj_set_style_border_width(*btns[i], 5, LV_STATE_FOCUS_KEY);
        //lv_obj_set_style_border_opa(*btns[i], 255, LV_STATE_FOCUS_KEY);
        // if(i== BTNS_SIZE-1 && BTNS_SIZE%2!=0){
        //     lv_obj_set_size(*btns[i], sub_width, sub_height*2+v/18);
        // }else{
        //     lv_obj_set_size(*btns[i], sub_width, sub_height);
        // }

        label = lv_label_create(*btns[i]);
        //lv_obj_center(label);  
        if (i == 0)
            lv_obj_align(label,LV_ALIGN_BOTTOM_MID, 0, -50);
        else
            lv_obj_align(label,LV_ALIGN_BOTTOM_MID, 0, 2);
        lv_obj_set_style_bg_color(*btns[i], lv_color_make(r[i], g[i], b[i]), 0);
        set_label_text2(label, text_ids[i], FONT_NORMAL);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        // if(i== 6){
        //     lv_obj_set_style_text_font(label,osd_font_get(FONT_NORMAL),0);
        //     lv_label_set_text(label,"YouTube\n(simple)");
        // }
    }
	
	prompt_show = lv_obj_create(main_page_scr);
	lv_obj_set_size(prompt_show, lv_pct(35), lv_pct(12));
	lv_obj_set_style_bg_opa(prompt_show, LV_OPA_0, 0);
	lv_obj_align(prompt_show, LV_ALIGN_TOP_RIGHT, 88, 30);
	lv_obj_set_style_pad_all(prompt_show, 0, 0);
	lv_obj_set_style_border_width(prompt_show, 0, 0);
	lv_obj_set_scrollbar_mode(prompt_show, LV_SCROLLBAR_MODE_OFF);
	lv_obj_add_event_cb(prompt_show, mainpage_storage_icon_refresh_cb, LV_EVENT_ALL, 0);
	//lv_obj_set_flex_flow(prompt_show, LV_FLEX_FLOW_ROW);
	//lv_obj_set_flex_align(prompt_show, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    //static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    //static lv_coord_t col_dsc[] = {LV_GRID_FR(2), LV_GRID_FR(8), LV_GRID_TEMPLATE_LAST};

    //lv_obj_set_grid_dsc_array(prompt_show, col_dsc, row_dsc);
   
   
    usb_show = lv_obj_create(prompt_show);
    lv_obj_set_size(usb_show,LV_PCT(33),LV_PCT(100));
    lv_obj_set_pos(usb_show,20,-2);

    lv_obj_t* img = lv_img_create(usb_show);
    lv_img_set_src(img, &public_usb_no);
    lv_obj_set_scrollbar_mode(usb_show, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(usb_show, LV_OPA_0, 0);
    lv_obj_set_style_border_width(usb_show, 0, 0);	 


#if defined(CAST_SUPPORT) && defined(WIFI_SUPPORT)
    wifi_show = lv_obj_create(prompt_show);
    lv_obj_set_size(wifi_show,LV_PCT(33),LV_PCT(100));
    lv_obj_set_pos(wifi_show,146,0);
    lv_obj_t* img1 = lv_img_create(wifi_show);
    lv_img_set_src(img1, &public_wifi_no);
    lv_obj_set_scrollbar_mode(wifi_show, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(wifi_show, LV_OPA_0, 0);
    lv_obj_set_style_border_width(wifi_show, 0, 0);
	//wifi_show = main_page_prompt_create(prompt_show, &wifi_no);
    //lv_obj_set_grid_cell(wifi_show, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
	lv_obj_add_event_cb(wifi_show, wifi_show_event_handle, LV_EVENT_ALL, 0);
#endif

#ifdef BLUETOOTH_SUPPORT
    bt_show = lv_obj_create(prompt_show);
    lv_obj_set_size(bt_show,LV_PCT(33),LV_PCT(100));
    lv_obj_set_pos(bt_show,80,0);
    lv_obj_t* img2 = lv_img_create(bt_show);
    lv_img_set_src(img2, &public_bt_no);
    lv_obj_set_scrollbar_mode(bt_show, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(bt_show, LV_OPA_0, 0);
    lv_obj_set_style_border_width(bt_show, 0, 0);
	//bt_show = main_page_prompt_create(prompt_show, &bt_discon);
    lv_obj_add_event_cb(bt_show, bt_show_event_handle, LV_EVENT_ALL, 0);
    lv_obj_set_grid_cell(bt_show, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
#endif

    //usb_show = main_page_prompt_create(prompt_show, &wifi_no);
    //lv_obj_set_grid_cell(sub, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_group_set_default(d_g);
}
