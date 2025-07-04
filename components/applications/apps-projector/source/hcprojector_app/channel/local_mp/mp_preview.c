//This file is usse for media_player in small window
//all most ui draw in local mp ui.c 
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"
#include "app_config.h"

#include "media_player.h"
#include <dirent.h>
#include "glist.h"
#include <sys/stat.h>
#include "win_media_list.h"
#include <hcuapi/input-event-codes.h>
#include <hcuapi/dis.h>
#include <ffplayer.h>
#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_mainpage.h"
#include "mp_ctrlbarpage.h"
#include "mp_fspage.h"
#include "screen.h"
#include "setup.h"
#include "factory_setting.h"

#include "mul_lang_text.h"
#include "mp_preview.h"

lv_timer_t * preview_timer_handle=NULL;
bool is_preview_error=false;

preview_state_e preview_state=PREVIEW_NONE;

/*lvlg operation*/
lv_obj_t * ui_win_zoom=NULL;
lv_obj_t * ui_win_name=NULL;
lv_obj_t * ui_file_info=NULL;
lv_obj_t* create_2text_inlabel(lv_obj_t *p)
{
    lv_obj_t* obj=lv_label_create(p);
    lv_obj_set_width(obj, LV_PCT(100));
    lv_obj_set_height(obj, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(obj, LV_ALIGN_TOP_MID);
    lv_label_set_text(obj, "");
    lv_obj_set_style_text_color(obj, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(obj, &LISTFONT_3000, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* text1 = lv_label_create(obj);
    lv_label_set_long_mode(text1,LV_LABEL_LONG_DOT);
    lv_obj_set_width(text1, LV_PCT(36));   /// 1
    lv_obj_set_height(text1, 30);    /// 1
    lv_obj_set_align(text1, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_text_align(text1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(text1, "");
    

    lv_obj_t* text2 = lv_label_create(obj);
    lv_label_set_long_mode(text2,LV_LABEL_LONG_DOT);
    lv_obj_set_width(text2, LV_PCT(64));   /// 1
    lv_obj_set_height(text2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(text2, LV_ALIGN_RIGHT_MID);
    lv_obj_set_style_text_align(text2, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(text2,"");  
    // lv_obj_set_style_text_font(obj, &LISTFONT_3000, LV_PART_MAIN | LV_STATE_DEFAULT);
 
    return obj;
}
#define  MAX_OBJ_ITEM 5
static lv_obj_t * preview_info_obj[MAX_OBJ_ITEM]={NULL}; 
void preview_win_create_info_cont(lv_obj_t* p,int num)
{
    lv_obj_set_flex_flow(p, LV_FLEX_FLOW_COLUMN);
    for(int i=0;i<num;i++){
        preview_info_obj[i]=create_2text_inlabel(p);
    }
}

int win_preview_add_data(file_list_t* fspage_flist)
{
    mp_info_t mp_info={0};
    media_get_info(mp_get_cur_player_hdl(),&mp_info);
    file_node_t *file_node=file_mgr_get_file_node(fspage_flist,fspage_flist->item_index);
    lv_label_set_text_fmt(lv_obj_get_child(ui_win_name,0),"%s",file_node->name);
    char show_txt[32] = {0};

    switch(fspage_flist->media_type){
        case MEDIA_TYPE_VIDEO:{
            set_label_text2(lv_obj_get_child(preview_info_obj[0],0),STR_INFO_RES,FONT_MID);
            lv_label_set_text_fmt(lv_obj_get_child(preview_info_obj[0],1),"%dx%d",
            mp_info.video_info.width,mp_info.video_info.height);

            set_label_text2(lv_obj_get_child(preview_info_obj[1],0), STR_INFO_AUDIO,FONT_MID);
            if(mp_info.audio_tracks_count>0)  //audio_tracks 
                lv_label_set_text_fmt(lv_obj_get_child(preview_info_obj[1],1),"%d/%d",mp_info.audio_info.index+1,mp_info.audio_tracks_count);
            else 
                lv_label_set_text(lv_obj_get_child(preview_info_obj[1],1),"--/--");

            set_label_text2(lv_obj_get_child(preview_info_obj[2],0), STR_INFO_SUBTITLE,FONT_MID);
            if(mp_info.subtitles_count>0)
                lv_label_set_text_fmt(lv_obj_get_child(preview_info_obj[2],1),"%d/%d",mp_info.subtitle_info.index+1,mp_info.subtitles_count);
            else
                lv_label_set_text(lv_obj_get_child(preview_info_obj[2],1),"--/--");

            set_label_text2(lv_obj_get_child(preview_info_obj[3],0), STR_INFO_SIZE,FONT_MID);
            //lv_label_set_text_fmt(lv_obj_get_child(preview_info_obj[3],1),"%.2fMB",mp_info.filesize);
            
            sprintf(show_txt, "%.2fMB", mp_info.filesize);
            lv_label_set_text(lv_obj_get_child(preview_info_obj[3],1), show_txt);
            }
            break;
        case MEDIA_TYPE_MUSIC:{
                set_label_text2(lv_obj_get_child(preview_info_obj[0],0),STR_INFO_ALBUM,FONT_MID);
                if(mp_info.media_info.album==NULL)
                    lv_label_set_text(lv_obj_get_child(preview_info_obj[0],1),"Unknown");
                else 
                    lv_label_set_text_fmt(lv_obj_get_child(preview_info_obj[0],1),"%s",mp_info.media_info.album);
                
                set_label_text2(lv_obj_get_child(preview_info_obj[1],0),STR_INFO_ARTIST,FONT_MID);
                if(mp_info.media_info.artist==NULL)
                    lv_label_set_text(lv_obj_get_child(preview_info_obj[1],1),"Unknown");
                else 
                    lv_label_set_text_fmt(lv_obj_get_child(preview_info_obj[1],1),"%s",mp_info.media_info.artist);

                set_label_text2(lv_obj_get_child(preview_info_obj[2],0),STR_INFO_SIZE,FONT_MID);
                //lv_label_set_text_fmt(lv_obj_get_child(preview_info_obj[2],1),"%.2fMB",mp_info.filesize);
                
                sprintf(show_txt, "%.2fMB", mp_info.filesize);
                lv_label_set_text(lv_obj_get_child(preview_info_obj[2],1), show_txt);
            }
            break;
        case MEDIA_TYPE_PHOTO:
            set_label_text2(lv_obj_get_child(preview_info_obj[0],0),STR_INFO_RES,FONT_MID);
            lv_label_set_text_fmt(lv_obj_get_child(preview_info_obj[0],1),"%dx%d",
            mp_info.video_info.width,mp_info.video_info.height);

            set_label_text2(lv_obj_get_child(preview_info_obj[1],0),STR_INFO_SIZE,FONT_MID);
            // lv_label_set_text_fmt(lv_obj_get_child(preview_info_obj[1],1),"%.2fMB",mp_info.filesize);

            if(mp_info.filesize<0.01){
                sprintf(show_txt, "%.2fKB", mp_info.filesize*1024);
            }else{
                sprintf(show_txt, "%.2fMB", mp_info.filesize);
            }
            lv_label_set_text(lv_obj_get_child(preview_info_obj[1],1), show_txt);

            break;
        default :
            break;
    }
}

preview_state_e win_preview_state_get(void)
{
    return preview_state;
}


void win_preview_create(lv_obj_t* parent)
{
#if 1   //ui draw
    ui_win_zoom = lv_obj_create(parent);
    lv_obj_set_size(ui_win_zoom,PREVIEW_WIN_W_PCT, PREVIEW_WIN_H_PCT);
    lv_obj_align(ui_win_zoom, LV_ALIGN_CENTER,0,-1);
    lv_obj_clear_flag(ui_win_zoom, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_win_zoom, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_win_zoom, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_win_zoom, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_win_zoom, lv_color_hex(0x0478F7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_win_zoom, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_win_zoom, PREVIEW_WIN_BORDER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_win_zoom, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_win_name = lv_obj_create(parent);
    lv_obj_set_size(ui_win_name,LV_PCT(34),LV_PCT(7));
    lv_obj_align_to(ui_win_name,ui_win_zoom,LV_ALIGN_OUT_BOTTOM_MID,0,PREVIEW_WINNAME_Y_OFS);
    lv_obj_clear_flag(ui_win_name, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_win_name, lv_color_hex(0xAD31F9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_win_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_win_name, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_win_name, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t * ui_name = lv_label_create(ui_win_name);
    lv_obj_set_size(ui_name,LV_PCT(100),LV_SIZE_CONTENT);
    lv_obj_set_align(ui_name, LV_ALIGN_CENTER);
#ifdef  CONFIG_SOC_HC15XX
    lv_label_set_long_mode(ui_name, LV_LABEL_LONG_CLIP);
#else 
    lv_label_set_long_mode(ui_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
#endif 

    lv_obj_set_style_text_align(ui_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    // set_label_text2(ui_name,0,FONT_MID);
    lv_obj_set_style_text_font(ui_name,&LISTFONT_3000,0);
    lv_label_set_text(ui_name,"");
    ui_file_info = lv_obj_create(parent);
    lv_obj_set_size(ui_file_info,LV_PCT(24),LV_PCT(42));
    lv_obj_align_to(ui_file_info,ui_win_zoom,LV_ALIGN_OUT_RIGHT_TOP,5,0);
    lv_obj_clear_flag(ui_file_info, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_file_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_file_info, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_file_info, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_file_info, lv_color_hex(0x0478F7), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_file_info, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_file_info, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_file_info, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui_file_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(ui_file_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_x(ui_file_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_y(ui_file_info, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_file_info, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_file_info, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_file_info, &lv_font_montserrat_28, LV_PART_MAIN | LV_STATE_DEFAULT);
#endif
    preview_win_create_info_cont(ui_file_info,MAX_OBJ_ITEM);
}
void win_preview_clear(void)
{
    if(ui_win_zoom!=NULL&&lv_obj_is_valid(ui_win_zoom)){
        lv_obj_del(ui_win_zoom);
        lv_obj_del(ui_file_info);
        lv_obj_del(ui_win_name);
        ui_win_zoom=NULL;
        ui_file_info= NULL;
        ui_win_name=NULL;
        printf("%s ,%d\n",__FUNCTION__, __LINE__);
    } 

}




/*de & media operation*/
int mp_set_preview_layer_order(int mode)
{
	struct dis_layer_blend_order vhance = { 0 };
	int fd;
	
	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	vhance.distype = DIS_TYPE_HD;
	if(mode == 1){	// preview mode, main layer on top
		//swap main layer with fb0 layer
        //main picture layer at the top
        vhance.auxp_layer = 3;
		vhance.main_layer = 2;	
        vhance.gmas_layer = 1;
		vhance.gmaf_layer = 0;
	}
	else{
        vhance.gmas_layer = 3;
		vhance.gmaf_layer = 2;
        //main picture layer at the bottom
        vhance.auxp_layer = 1;
		vhance.main_layer = 0;	
	}
	ioctl(fd , DIS_SET_LAYER_ORDER , &vhance);
	close(fd);
}

void app_set_de_dismode(media_play_mode_t mode)
{
    vdec_dis_rect_t dis_rect;
    if(mode==MEDIA_PLAY_NORMAL)
    {
        mp_set_preview_layer_order(0);
        dis_rect.src_rect.x = DIS_SOURCE_FULL_X;
        dis_rect.src_rect.y = DIS_SOURCE_FULL_Y;
        dis_rect.src_rect.w = DIS_SOURCE_FULL_W;
        dis_rect.src_rect.h = DIS_SOURCE_FULL_H;
        dis_rect.dst_rect.x = DIS_SOURCE_FULL_X;
        dis_rect.dst_rect.y = DIS_SOURCE_FULL_Y;
        dis_rect.dst_rect.w = DIS_SOURCE_FULL_W;
        dis_rect.dst_rect.h = DIS_SOURCE_FULL_H;
        media_set_play_mode(mode, &dis_rect);
    }
    else if(mode==MEDIA_PLAY_PREVIEW)
    {
        mp_set_preview_layer_order(1);
        dis_rect.src_rect.x = DIS_SOURCE_FULL_X;
        dis_rect.src_rect.y = DIS_SOURCE_FULL_Y;
        dis_rect.src_rect.w = DIS_SOURCE_FULL_W;
        dis_rect.src_rect.h = DIS_SOURCE_FULL_H;
        dis_rect.dst_rect.x = get_display_x() + get_display_h()/3;
        dis_rect.dst_rect.y = get_display_y() + get_display_v()/3;
        dis_rect.dst_rect.w = get_display_h()/3;
        dis_rect.dst_rect.h = get_display_v()/3;
        media_set_play_mode(mode, &dis_rect);
    }
}

/*preview stop and start in next peroid */
int preview_reset(void)
{
    media_handle_t* mp_hdl=mp_get_cur_player_hdl();
    if(mp_hdl && ui_win_zoom){
        win_preview_clear();
        preview_state=PREVIEW_NONE;
#ifdef RTOS_SUBTITLE_SUPPORT
        ext_subtitle_deinit();
#endif 
        media_player_close();
        app_set_de_dismode(MEDIA_PLAY_NORMAL);
        lv_timer_resume(preview_timer_handle);
        lv_timer_reset(preview_timer_handle);
    }
    return 0;
}
static int preview_stop(void){
    win_preview_clear();
    preview_state=PREVIEW_ERROR;
#ifdef RTOS_SUBTITLE_SUPPORT
    ext_subtitle_deinit();
#endif 
    media_player_close();
    app_set_de_dismode(MEDIA_PLAY_NORMAL);
    lv_timer_pause(preview_timer_handle);
    is_preview_error=true;
    return 0;
}
static int preview_start(void){
    if(preview_state==PREVIEW_ERROR){
        /* close the error hint inmediately */
        win_msgbox_msg_close();
        lv_timer_resume(preview_timer_handle);
        lv_timer_reset(preview_timer_handle);
        is_preview_error=false;
        preview_state=PREVIEW_NONE;
    }else if(preview_state==PREVIEW_NORMAL){
        preview_reset();
    }
    return 0;
}
int preview_deinit(void)
{
    preview_reset();
    if(preview_timer_handle){
        lv_timer_del(preview_timer_handle);
        preview_timer_handle=NULL;
    }
    win_msgbox_msg_close();
    preview_state=PREVIEW_NONE;

    //reset dis dev 
    int ratio = projector_get_some_sys_param(P_ASPECT_RATIO);
    api_set_display_area(ratio);
}


int preview_init()
{
    //create tim 创建定时器，即可将show pre win 
    if(!preview_timer_handle)
        preview_timer_handle=lv_timer_create(preview_timer_cb,2000,NULL);
        //init handle in timer cb
    return 0;
}
//meida play for preview depend on file_list_t 
void preview_timer_cb(lv_timer_t * t)
{
    
    file_list_t* fspage_filelist=app_get_file_list();
    file_node_t* file_node =file_mgr_get_file_node(fspage_filelist,fspage_filelist->item_index);
	
    if(file_node->type!=FILE_DIR){
        lv_timer_pause(preview_timer_handle);
        win_preview_create(ui_fspage);
        preview_state=PREVIEW_NORMAL;
        if(fspage_filelist->media_type ==  MEDIA_TYPE_VIDEO){
            api_set_display_aspect(DIS_TV_16_9,DIS_PILLBOX);
        }else{
            api_set_display_aspect(DIS_TV_16_9,DIS_NORMAL_SCALE);
        }
        app_set_de_dismode(MEDIA_PLAY_PREVIEW);
#ifdef RTOS_SUBTITLE_SUPPORT
        ext_subtitles_init(fspage_filelist);
#endif 
        media_player_open();
    }else{
        lv_timer_reset(preview_timer_handle);
    }
}
int preview_key_ctrl(uint32_t key)
{
    switch (key)
    {
        case LV_KEY_ENTER:
            win_msgbox_msg_close();
            break;
        case LV_KEY_UP:
        case LV_KEY_DOWN:
        case LV_KEY_LEFT:
        case LV_KEY_RIGHT:
        /* key move  just resume preview timer when is preview error
           or reset preview options */  
            preview_start();
            break;
        default :
            break;
    }
    return 0;
}
//for preview player message handle 
void preview_player_message_ctrl(void * arg1,void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    media_handle_t* media_handle=mp_get_cur_player_hdl();
    if(!media_handle)
        return ;
    if(MSG_TYPE_MSG == ctl_msg->msg_type)
    {
        uint32_t mp_code=(ctl_msg->msg_code>>16)&0xffff;
        switch (mp_code){
            case HCPLAYER_MSG_FIRST_VIDEO_FRAME_SHOWED:
                if(ui_win_zoom){
                    win_preview_add_data(app_get_file_list());
                    /* add data after player playing start */
                } 
                break;
            case HCPLAYER_MSG_OPEN_FILE_FAILED:
                /*stop the preview timer and clear the preview win*/ 
                preview_stop();
                win_msgbox_msg_open(STR_PREVIEW_ERROR,3000,NULL,NULL);
                win_msgbox_msg_set_pos(LV_ALIGN_CENTER,0,0);
                break;
            case HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO:
            case HCPLAYER_MSG_VIDEO_DECODE_ERR:
                if(media_handle->type==MEDIA_TYPE_VIDEO||media_handle->type==MEDIA_TYPE_PHOTO){
                    preview_stop();
                    win_msgbox_msg_open(STR_PREVIEW_ERROR,3000,NULL,NULL);
                    win_msgbox_msg_set_pos(LV_ALIGN_CENTER,0,0);
                }
                break;
            case HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO:
            case HCPLAYER_MSG_AUDIO_DECODE_ERR:
                if(media_handle->type==MEDIA_TYPE_MUSIC){
                    preview_stop();
                    win_msgbox_msg_open(STR_PREVIEW_ERROR,3000,NULL,NULL);
                    win_msgbox_msg_set_pos(LV_ALIGN_CENTER,0,0);
                }
                break;
            default :
                break;
        }
    }

}


