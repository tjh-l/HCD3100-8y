#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"

#include "media_player.h"
#include <dirent.h>
#include "glist.h"
#include <sys/stat.h>

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_mainpage.h"
#include "mp_fspage.h"
#include "screen.h"
#include "setup.h"
#include "factory_setting.h"
#include "mp_ebook.h"
#include "mul_lang_text.h"

SCREEN_SUBMP_E screen_submp;
media_type_t media_type;
static int fc_obj_index=0;
#define OBJITEM_CNT 4
void set_key_group(lv_group_t * group)
{
	lv_indev_set_group(indev_keypad, group);
	lv_group_set_default(group);
}
SCREEN_SUBMP_E get_screen_submp(void)
{
    return screen_submp;
}

int app_set_screen_submp(SCREEN_SUBMP_E subpage)
{
    screen_submp=subpage;
    return 0;
}



void main_page_keyinput_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target = lv_event_get_target(event);
    if(code == LV_EVENT_KEY)
    {
        int keypad_value = lv_indev_get_key(lv_indev_get_act());
        partition_info_t * cur_partition_info1=mmp_get_partition_info();
        //int index=lv_obj_get_index(lv_obj_get_parent(target));
        int index=lv_obj_get_index((target));
        switch(keypad_value){
            case LV_KEY_UP:
                lv_group_focus_next(main_group);
                break;
            case LV_KEY_DOWN:
                lv_group_focus_prev(main_group);
                break;
            case LV_KEY_LEFT:
                lv_group_focus_prev(main_group);
                break;
            case LV_KEY_RIGHT:
                lv_group_focus_next(main_group);
                break;
            case LV_KEY_ENTER:
                if(cur_partition_info1!=NULL&&cur_partition_info1->count>0){
                    switch(index){
                        case  0:
                            media_type=MEDIA_TYPE_VIDEO;
                            break;
                        case 1 :
                            media_type=MEDIA_TYPE_MUSIC;
                            break;
                        case 2 :
                            media_type=MEDIA_TYPE_PHOTO;
                            break;
                        case 3 :
                            media_type=MEDIA_TYPE_TXT;
                            break;
                    }
                    fc_obj_index=index;
                    _ui_screen_change(ui_subpage, 0, 0); 
                }else
                	win_msgbox_msg_open(STR_UPGRADE_NO_DEV, 3000, NULL, NULL);
                break;
            case LV_KEY_ESC:
                change_screen(SCREEN_CHANNEL_MAIN_PAGE);
                break;
            default: 
                break;
        }if(code==LV_EVENT_DRAW_POST_END){
            // fc_obj=lv_group_get_focused(lv_group_get_default());
        }
    }
}
/**
 * @description:func call back mean has a hotplug event,so had to refresh icon  
 * @param {lv_event_t} *event
 * @return {*}
 * @author: Yanisin
 */
 LV_IMG_DECLARE(public_usb);
 LV_IMG_DECLARE(public_usb_no);
void storage_icon_refresh_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t*  target=lv_event_get_target(event);
    void * user_data= lv_event_get_user_data(event);
    if(code == LV_EVENT_REFRESH)
    {
        partition_info_t * partition_info2=mmp_get_partition_info();

        //lv_obj_clean(target);
        //lv_obj_t* usb_icon=lv_label_create(target);
        //lv_obj_set_size(usb_icon,LV_PCT(7),LV_SIZE_CONTENT);
        //lv_label_set_text(usb_icon,"");
        //lv_obj_t* sd_icon=lv_label_create(target);
        //lv_obj_set_size(sd_icon,LV_PCT(7),LV_SIZE_CONTENT);
        //lv_label_set_text(sd_icon,"");
        if(partition_info2 != NULL){
            if(partition_info2->count==1){
	            for(int i=0;i<partition_info2->count;i++){
	                char *dev_name=glist_nth_data(partition_info2->dev_list,i);

	                //rtos: sdaxx; linux: /media/hddxx /media/sdxx
	                if(strstr(dev_name,"sd")|| strstr(dev_name,"hd")||strstr(dev_name,"usb")){
	                    //lv_label_set_text(usb_icon,LV_SYMBOL_USB);
	    				lv_img_set_src(lv_obj_get_child(lv_obj_get_child(target, 0),0), &public_usb);
	                }else{
	                    lv_img_set_src(lv_obj_get_child(lv_obj_get_child(target, 0),0), &public_usb_no);
	                }
	                //if(strstr(dev_name,"mmc")){
	                //    lv_label_set_text(sd_icon,LV_SYMBOL_SD_CARD);
	                //}
	            }
			}
			else if(partition_info2->count==2){
                lv_img_set_src(lv_obj_get_child(lv_obj_get_child(target, 0),0), &public_usb);
            }else if(partition_info2->count==0){
                lv_img_set_src(lv_obj_get_child(lv_obj_get_child(target, 0),0), &public_usb_no);
            }
            //if(partition_info2->count==0){
            //    lv_obj_clean(target);
            //    lv_obj_t* icon = lv_label_create(target);
            //    set_label_text2(icon,STR_MP_NO_DEV,FONT_NORMAL);
            //}
        }
        //lv_obj_invalidate(ui_mainpage);
    }
}

static int scr_handle_group_focus(void)
{
    lv_obj_t* temp_g[OBJITEM_CNT]={NULL};
    for(int i=0;i<OBJITEM_CNT;i++){
        //temp_g[i]=lv_obj_get_child(lv_obj_get_child(main_cont1,i),1);
        temp_g[i]=lv_obj_get_child(main_cont1,i);
        //index 1 mean btn in cont 
    }    
    if(lv_obj_is_valid(temp_g[fc_obj_index])){
        lv_group_focus_obj(temp_g[fc_obj_index]);
    }
    return 0;
}

int mainpage_open(void)
{
    main_group= lv_group_create();
    set_key_group(main_group);
    create_mainpage_scr();
    scr_handle_group_focus();
    screen_submp=SCREEN_SUBMP0;
    return 0;
}

int mainpage_close(void)
{
    win_msgbox_msg_close_on_top();
    lv_group_remove_all_objs(main_group);
    lv_group_del(main_group);
    clear_mainpage_scr();
    return 0;
}
media_type_t get_media_type(void)
{
    return media_type;
} 
