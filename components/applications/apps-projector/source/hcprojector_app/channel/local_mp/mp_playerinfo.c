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
#include "win_media_list.h"
#include <hcuapi/vidmp.h>

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_mainpage.h"
#include "mp_subpage.h"
#include "mp_fspage.h"
#include "mp_ctrlbarpage.h"
#include "mp_ebook.h"
#include "src/font/lv_font.h"
#include "setup.h"
#include "factory_setting.h"

#include "mul_lang_text.h"
#include "mp_playlist.h"
#include "mp_bsplayer_list.h"
#include "backstage_player.h"
#include "mp_playerinfo.h"
#include "app_config.h"


//create list obj with infowin style
lv_obj_t* create_list_sub_text_obj(lv_obj_t *parent,int w, int h,char * str1,int str_id){
    static lv_obj_t *list_label;
    list_label = lv_list_add_text(parent, " ");
    lv_obj_set_style_text_align(list_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_size(list_label,LV_PCT(w),LV_SIZE_CONTENT);
    lv_obj_set_style_pad_ver(list_label, 3, 0);

    lv_obj_set_style_border_width(list_label, 2, 0);
    lv_obj_set_style_border_color(list_label, lv_color_white(), 0);
    lv_obj_set_style_border_opa(list_label, LV_OPA_0, 0);
    // lv_obj_set_style_border_opa(list_label, LV_OPA_100, LV_STATE_FOCUS_KEY);

    lv_obj_set_style_bg_opa(list_label, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(list_label, LV_OPA_100, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_color(list_label, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_FOCUS_KEY);

    lv_obj_set_style_text_color(list_label, lv_color_white(), 0);
    lv_obj_set_style_text_color(list_label, lv_color_black(), LV_STATE_FOCUS_KEY);
    //select char or char index 
    if(str_id==0){
        set_label_text2(list_label,0,FONT_MID);        
        lv_label_set_text(list_label,str1);
    }
    else{
        set_label_text2(list_label,str_id,FONT_MID);        
    }
#ifdef  CONFIG_SOC_HC15XX   //HC15XX will cost a lot of CPU when lvgl scroll pixel, So Disabel Here
    lv_label_set_long_mode(list_label,  LV_LABEL_LONG_CLIP);
#else
    lv_label_set_long_mode(list_label,  LV_LABEL_LONG_SCROLL_CIRCULAR);
#endif 
   return list_label;
}

lv_obj_t* create_list_sub_btn_obj(lv_obj_t *parent){
    lv_obj_t *list_btn;
    list_btn = lv_list_add_btn(parent, NULL, " ");
    // lv_group_remove_obj(list_btn);

    lv_obj_set_size(list_btn,LV_PCT(100),LV_SIZE_CONTENT);
    lv_obj_set_style_pad_ver(list_btn, MPINFO_BTN_PADVER, 0); 
    //font size too large so had to recover  text pos 
    //lv_obj_set_size(list_btn,LV_PCT(100),LV_SIZE_CONTENT);

    lv_obj_set_style_border_side(list_btn, LV_BORDER_SIDE_FULL, 0);

    lv_obj_set_style_border_width(list_btn, 2, 0);
    lv_obj_set_style_border_color(list_btn, lv_color_white(), 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_0, 0);
    // lv_obj_set_style_border_opa(list_btn, LV_OPA_100,  LV_STATE_FOCUS_KEY);

    lv_obj_set_style_bg_opa(list_btn, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(list_btn, LV_OPA_100,  LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_color(list_btn, lv_palette_main(LV_PALETTE_BLUE),  LV_STATE_FOCUS_KEY);

    lv_obj_set_style_text_color(list_btn, lv_color_white(), 0);
    lv_obj_set_style_text_color(list_btn, lv_color_black(),  LV_STATE_FOCUS_KEY);

    lv_obj_t* label = lv_obj_get_child(list_btn, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_long_mode(label,LV_LABEL_LONG_DOT);
    lv_obj_set_size(label,LV_PCT(50),LV_SIZE_CONTENT);


    label = lv_label_create(list_btn);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_width(label,LV_PCT(50));    
    lv_label_set_long_mode(label,LV_LABEL_LONG_DOT);

    return list_btn;
}

lv_obj_t* create_list_sub_btn_obj3(lv_obj_t *parent, int str1, char * str2){
    lv_obj_t *list_btn = create_list_sub_btn_obj(parent);   //sub button sytle here
    
    lv_obj_t *label = lv_obj_get_child(list_btn, 0);
    if(str1>=0){
        set_label_text2(label,str1,FONT_MID);
    }else{
        lv_label_set_text(label, " ");
    }

    lv_obj_t *label2 = lv_obj_get_child(list_btn, 1);
    set_label_text2(label2,1,FONT_MID);
    lv_label_set_text(label2, str2);
    lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);
    return list_btn;
}

lv_obj_t* create_list_obj2(lv_obj_t *parent, int w, int h)
{
    //with style 
    lv_obj_t *obj = lv_list_create(parent);
    lv_obj_align(obj, LV_ALIGN_TOP_RIGHT,MPLIST_WIN_X_OFS,MPLIST_WIN_Y_OFS);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(obj, 0, 0);
    lv_obj_set_style_pad_right(obj, 0, 0);
    lv_obj_set_size(obj, LV_PCT(w),LV_SIZE_CONTENT);
    lv_group_add_obj(lv_group_get_default(), obj);
    return obj;

}
#define MAX_INFO_ITEM   10
lv_obj_t* subwin_mpinfo=NULL;
lv_obj_t* mpinfo_subobj[MAX_INFO_ITEM]={NULL};
lv_obj_t* create_mpinfo_subwin(lv_obj_t *parent, int w, int h)
{
    lv_obj_t* obj = create_list_obj2(parent, w, h); 
    lv_obj_align(obj,LV_ALIGN_TOP_RIGHT,MPINFO_WIN_X_OFS,MPINFO_WIN_Y_OFS);
    return obj ;
}
int mp_infowin_add_data(media_type_t media_type,mp_info_t mp_info)
{
    char info_buf[128]={0};
    char  * title_name =lv_label_get_text(ui_playname);
    char tm_str[100] = {0};

    mpinfo_subobj[0]=create_list_sub_text_obj(subwin_mpinfo,100,11,title_name,0);    //title
    lv_obj_set_style_text_font(mpinfo_subobj[0],&LISTFONT_3000,0);
    switch(media_type)
    {
        case MEDIA_TYPE_VIDEO:
            sprintf(info_buf,"%dx%d",mp_info.video_info.width,mp_info.video_info.height);//size 
            mpinfo_subobj[1]=create_list_sub_btn_obj3(subwin_mpinfo,STR_INFO_RES,info_buf);
            memset(info_buf,0,sizeof(info_buf));

            if(mp_info.audio_tracks_count)  //audio_tracks 
                sprintf(info_buf, "< %d/%d >",mp_info.audio_info.index+1,mp_info.audio_tracks_count);
            else
                sprintf(info_buf, "< --/-- >");            
            mpinfo_subobj[2]=create_list_sub_btn_obj3(subwin_mpinfo,STR_INFO_AUDIO,info_buf);
            memset(info_buf,0,sizeof(info_buf));

            if(mp_info.subtitles_count) //subtitles_count
                sprintf(info_buf, "< %d/%d >",mp_info.subtitle_info.index+1,mp_info.subtitles_count);
            else
                sprintf(info_buf, "< --/-- >");            
            mpinfo_subobj[3]=create_list_sub_btn_obj3(subwin_mpinfo,STR_INFO_SUBTITLE,info_buf);
            memset(info_buf,0,sizeof(info_buf));

            if(mp_info.filesize<0.01)
                mp_info.filesize=0.01;
            sprintf(info_buf,"%.2fMB",mp_info.filesize);//file size
            mpinfo_subobj[4]=create_list_sub_btn_obj3(subwin_mpinfo,STR_INFO_SIZE,info_buf);
            memset(info_buf,0,sizeof(info_buf));

            mpinfo_subobj[5]=create_list_sub_text_obj(subwin_mpinfo,100,11,NULL,STR_INFO_EXIT);
            break;
        case MEDIA_TYPE_MUSIC:
            if(mp_info.media_info.album==NULL)
                sprintf(info_buf,"Unknown");
            else 
                string_dec_conv2_utf8(mp_info.media_info.album, info_buf, sizeof(info_buf));

            mpinfo_subobj[1]=create_list_sub_btn_obj3(subwin_mpinfo,STR_INFO_ALBUM,info_buf);
            memset(info_buf,0,sizeof(info_buf));

            if(mp_info.media_info.artist==NULL)
                sprintf(info_buf,"Unknown");
            else 
                string_dec_conv2_utf8(mp_info.media_info.artist, info_buf, sizeof(info_buf));

            mpinfo_subobj[2]=create_list_sub_btn_obj3(subwin_mpinfo,STR_INFO_ARTIST,info_buf);
            memset(info_buf,0,sizeof(info_buf));

            sprintf(info_buf,"%.2fMB",mp_info.filesize);//file size
            mpinfo_subobj[3]=create_list_sub_btn_obj3(subwin_mpinfo,STR_INFO_SIZE,info_buf);
            memset(info_buf,0,sizeof(info_buf));

            mpinfo_subobj[4]=create_list_sub_text_obj(subwin_mpinfo,100,11,NULL,STR_INFO_EXIT);
            break;
        case MEDIA_TYPE_PHOTO: 
            sprintf(info_buf,"%dx%d",mp_info.video_info.width,mp_info.video_info.height);//size 
            mpinfo_subobj[1]=create_list_sub_btn_obj3(subwin_mpinfo,STR_INFO_RES,info_buf);
            memset(info_buf,0,sizeof(info_buf));

            if(mp_info.filesize<0.01)
                sprintf(info_buf,"%.2fKB",mp_info.filesize*1024);//file size
            else
                sprintf(info_buf,"%.2fMB",mp_info.filesize);//file size
            mpinfo_subobj[2]=create_list_sub_btn_obj3(subwin_mpinfo,STR_INFO_SIZE,info_buf);
            memset(info_buf,0,sizeof(info_buf));

            strftime(tm_str, sizeof(tm_str), "%Y-%m-%d", localtime(&mp_info.stat_buf.st_mtime));
            mpinfo_subobj[3]=create_list_sub_btn_obj3(subwin_mpinfo,STR_FILE_DATE,tm_str);
            memset(tm_str,0,sizeof(tm_str));
            

            strftime(tm_str, sizeof(tm_str), "%H:%M:%S", localtime(&mp_info.stat_buf.st_mtime));
            mpinfo_subobj[4]=create_list_sub_btn_obj3(subwin_mpinfo,STR_FILE_TIME,tm_str);
            memset(tm_str,0,sizeof(tm_str));

            mpinfo_subobj[5]=create_list_sub_text_obj(subwin_mpinfo,100,11,NULL,STR_INFO_EXIT);
            break;
        case MEDIA_TYPE_TXT:
            if(mp_info.filesize>1024.0){
                mp_info.filesize=mp_info.filesize/1024.0;
                sprintf(info_buf,"%.2fMB",mp_info.filesize);//file size
            }else{
                sprintf(info_buf,"%.2fKB",mp_info.filesize);//file size
            }
            mpinfo_subobj[1]=create_list_sub_btn_obj3(subwin_mpinfo,STR_INFO_SIZE,info_buf);
            memset(info_buf,0,sizeof(info_buf));

            mpinfo_subobj[2]=create_list_sub_text_obj(subwin_mpinfo,100,11,NULL,STR_INFO_EXIT);

            break;
    }
    lv_group_focus_obj(subwin_mpinfo);

}

mp_info_t mp_info;
file_list_t * fspage_filelist;
media_handle_t *mp_hdl = NULL;

static int media_ebook_get_info(file_list_t * fspage_filelist,mp_info_t* mp_info)
{
    char ebook_file_name[MAX_FILE_NAME]={0};
	file_node_t *file_node = file_mgr_get_file_node(fspage_filelist, fspage_filelist->item_index);
	file_mgr_get_fullname(ebook_file_name,fspage_filelist->dir_path,file_node->name);
    if(stat(ebook_file_name,&mp_info->stat_buf)==0){
        mp_info->filesize=mp_info->stat_buf.st_size;//size B
        mp_info->filesize=mp_info->filesize/1024.0;//size KB
    }else{
        memset(mp_info,0,sizeof(mp_info_t));
    }
    return 0;
}


int create_mpinfo_win(lv_obj_t *p,lv_obj_t * sub_btn)
{
    fspage_filelist=app_get_file_list();
    mp_hdl=mp_get_cur_player_hdl();
    if(fspage_filelist->media_type==MEDIA_TYPE_TXT) {
        //get info from stat func
        media_ebook_get_info(fspage_filelist,&mp_info);
    }else{
        //mean get info from ffplayer 
        media_get_info(mp_hdl,&mp_info);
    }
    subwin_mpinfo=create_mpinfo_subwin(p,MPINFO_WIN_W_PCT,55);
    lv_obj_add_event_cb(subwin_mpinfo, mpinfo_win_event_cb, LV_EVENT_ALL, sub_btn);
    mp_infowin_add_data(fspage_filelist->media_type,mp_info);
    lv_obj_move_foreground(subwin_mpinfo);
    return 0;
} 

int win_refresh_info(media_type_t media_type,mp_info_t mp_info)
{
    
    char  * title_name =lv_label_get_text(ui_playname);
    char tm_str2[128] = {0};
    char show_txt[32]={0};
    switch (media_type){
        case MEDIA_TYPE_VIDEO:
            lv_label_set_text_fmt(mpinfo_subobj[0],"%s",title_name);
            lv_label_set_text_fmt(lv_obj_get_child(mpinfo_subobj[1],1),"%dx%d",mp_info.video_info.width,mp_info.video_info.height);
            if(mp_info.audio_tracks_count)  //audio_tracks 
                lv_label_set_text_fmt(lv_obj_get_child(mpinfo_subobj[2],1),"< %d/%d >",mp_info.audio_info.index+1,mp_info.audio_tracks_count);
            else 
                lv_label_set_text(lv_obj_get_child(mpinfo_subobj[2],1),"< --/-- >");
            if(mp_info.subtitles_count) //subtitles_count
                lv_label_set_text_fmt(lv_obj_get_child(mpinfo_subobj[3],1), "< %d/%d >",mp_info.subtitle_info.index+1,mp_info.subtitles_count);
            else
                lv_label_set_text(lv_obj_get_child(mpinfo_subobj[3],1), "< --/-- >");
            sprintf(show_txt, "%.2fMB", mp_info.filesize);
            lv_label_set_text(lv_obj_get_child(mpinfo_subobj[4],1),show_txt);
            break;
        case MEDIA_TYPE_MUSIC:
            lv_label_set_text_fmt(mpinfo_subobj[0],"%s",title_name);
            if(mp_info.media_info.album!=NULL)
                lv_label_set_text_fmt(lv_obj_get_child(mpinfo_subobj[1],1),"%s",mp_info.media_info.album);
            else 
                lv_label_set_text(lv_obj_get_child(mpinfo_subobj[1],1),"Unknown");
            
            if(mp_info.media_info.artist!=NULL)
                lv_label_set_text_fmt(lv_obj_get_child(mpinfo_subobj[2],1),"%s",mp_info.media_info.artist);
            else 
                lv_label_set_text(lv_obj_get_child(mpinfo_subobj[2],1),"Unknown");
            sprintf(show_txt, "%.2fMB", mp_info.filesize);
            lv_label_set_text(lv_obj_get_child(mpinfo_subobj[3],1),show_txt);
            break;
        case MEDIA_TYPE_PHOTO:
            lv_label_set_text_fmt(mpinfo_subobj[0],"%s",title_name);
            lv_label_set_text_fmt(lv_obj_get_child(mpinfo_subobj[1],1),"%dx%d",mp_info.video_info.width,mp_info.video_info.height);
            if(mp_info.filesize<0.01)
                sprintf(show_txt, "%.2fKB", mp_info.filesize*1024);
            else
                sprintf(show_txt, "%.2fMB", mp_info.filesize);
            lv_label_set_text(lv_obj_get_child(mpinfo_subobj[2],1),show_txt);
            
            strftime(tm_str2, sizeof(tm_str2), "%Y-%m-%d", localtime(&mp_info.stat_buf.st_mtime));
            printf("CTIME :%s \n",ctime(&mp_info.stat_buf.st_mtime));
            lv_label_set_text_fmt(lv_obj_get_child(mpinfo_subobj[3],1),"%s",tm_str2);
            memset(tm_str2,0,sizeof(tm_str2));
            strftime(tm_str2, sizeof(tm_str2), "%H:%M:%S", localtime(&mp_info.stat_buf.st_mtime));
            lv_label_set_text_fmt(lv_obj_get_child(mpinfo_subobj[4],1),"%s",tm_str2);         
            break;
        case MEDIA_TYPE_TXT:
            break;
        default: 
            break;
    }
    return 0;
}


/* all key ctrk here */
void mpinfo_win_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *target = lv_event_get_target(event);
    lv_obj_t * user_data = lv_event_get_user_data(event);
    int sel_count = lv_obj_get_child_cnt(target);
    static int sel_id=0;
    int ret=0;
    if(code==LV_EVENT_FOCUSED)  //focused on list
    {
        int first_sel_id=0 ;
        switch (fspage_filelist->media_type)
        {
            case MEDIA_TYPE_VIDEO:
                if(mp_info.audio_tracks_count>1){
                    first_sel_id=2;
                }else
                    first_sel_id=5;
                break;
            case MEDIA_TYPE_MUSIC:
                first_sel_id=4;
                break;
            case MEDIA_TYPE_PHOTO:
                first_sel_id=5;
                break;
            case MEDIA_TYPE_TXT :
                first_sel_id=2;
            default :
                break;

        }
        //GDB Error here OBJ->NULL  
        if(!lv_obj_has_state(lv_obj_get_child(target, sel_id), LV_STATE_FOCUS_KEY)){
            lv_obj_add_state(lv_obj_get_child(target, first_sel_id), LV_STATE_FOCUS_KEY);
            sel_id = first_sel_id;
        }
    }
    else if(code == LV_EVENT_KEY)
    {
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key== LV_KEY_UP){
            if(fspage_filelist->media_type==MEDIA_TYPE_VIDEO){
                lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_FOCUS_KEY); //clear  focuse
                switch(sel_id)
                {
                    case 0:
                        sel_id=5;
                        break;
                    case 5:
                        sel_id=3;
                        break;
                    case 3:
                        sel_id=2;
                        break;
                    case 2:
                        sel_id=5;
                        break;
                }
                lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_FOCUS_KEY);
            }
        }else if(key == LV_KEY_DOWN){
            if(fspage_filelist->media_type==MEDIA_TYPE_VIDEO){
                lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_FOCUS_KEY); //clear  focuse
                // sel_id=sel_id<sel_count-1?sel_id+1:0;
                switch(sel_id)
                {
                    case 0:
                        sel_id=2;
                        break;
                    case 2:
                        sel_id=3;
                        break;
                    case 3:
                        sel_id=5;
                        break;
                    case 5:
                        sel_id=2;
                        break;
                }
                lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_FOCUS_KEY);
            }
        }else if(key==LV_KEY_RIGHT){
            if(fspage_filelist->media_type==MEDIA_TYPE_VIDEO){
                switch (sel_id)
                {
                    case 2: //audio track selected 
                        if(mp_info.audio_tracks_count<=1 || mp_hdl->state != MEDIA_PLAY)
                            break;
                        // when change audio/vidoe index it will call any playing msg
                        mp_info.audio_info.index=mp_info.audio_info.index<mp_info.audio_tracks_count-1?mp_info.audio_info.index+1:0;
                        if(hcplayer_change_audio_track(mp_hdl->player,mp_info.audio_info.index)!=0){
                            win_msgbox_msg_open(STR_AUDIO_USPT, 2000, NULL, NULL);
                        }
                        //change ui show
                        lv_obj_t * label=lv_obj_get_child(lv_obj_get_child(target,sel_id),1);
                        lv_label_set_text_fmt(label,"< %d/%d >",mp_info.audio_info.index+1,mp_info.audio_tracks_count);
                        break;
                    case 3: //subtitle
                        if(mp_info.subtitles_count<=1)
                            break;                    
                        //player change 
                        mp_info.subtitle_info.index=mp_info.subtitle_info.index<mp_info.subtitles_count-1?mp_info.subtitle_info.index+1:0;
                        if(hcplayer_change_subtitle_track(mp_hdl->player,mp_info.subtitle_info.index)!=0){
                            win_msgbox_msg_open(STR_SUBTITLE_MSG, 2000, NULL, NULL);
                        }
                        //change ui show
                        lv_obj_t * label2=lv_obj_get_child(lv_obj_get_child(target,sel_id),1);
                        lv_label_set_text_fmt(label2,"< %d/%d >",mp_info.subtitle_info.index+1,mp_info.subtitles_count);
                        break;
                    default : 
                        break;

                }

            }
        }else if(key== LV_KEY_LEFT){
            if(fspage_filelist->media_type==MEDIA_TYPE_VIDEO){
                switch (sel_id)
                {
                    case 2: //audio track selected 
                        if(mp_info.audio_tracks_count<=1 || mp_hdl->state != MEDIA_PLAY)
                            break;
                        //player change 
                        mp_info.audio_info.index=mp_info.audio_info.index<=0?mp_info.audio_tracks_count-1:mp_info.audio_info.index-1;
                        if(hcplayer_change_audio_track(mp_hdl->player,mp_info.audio_info.index)!=0){
                            win_msgbox_msg_open(STR_AUDIO_USPT, 2000, NULL, NULL);
                        }                        
                        //change ui show
                        lv_obj_t * label=lv_obj_get_child(lv_obj_get_child(target,sel_id),1);
                        lv_label_set_text_fmt(label,"< %d/%d >",mp_info.audio_info.index+1,mp_info.audio_tracks_count);
                        break;
                    case 3: //subtitle
                        if(mp_info.subtitles_count<=1)
                            break;
                        //player change 
                        mp_info.subtitle_info.index=mp_info.subtitle_info.index<=0?mp_info.subtitles_count-1:mp_info.subtitle_info.index-1;
                        if(hcplayer_change_subtitle_track(mp_hdl->player,mp_info.subtitle_info.index)!=0){
                            win_msgbox_msg_open(STR_SUBTITLE_MSG, 2000, NULL, NULL);
                        }                        
                        //change ui show
                        lv_obj_t * label2=lv_obj_get_child(lv_obj_get_child(target,sel_id),1);
                        lv_label_set_text_fmt(label2,"< %d/%d >",mp_info.subtitle_info.index+1,mp_info.subtitles_count);
                        break;
                    default : 
                        break;

                }

            }
        }else if(key== LV_KEY_ESC){
            sel_id=0;
            if(lv_obj_is_valid(target))
                lv_obj_del(target);
            lv_group_focus_obj(user_data);
        }
        else if(key==LV_KEY_ENTER){
            int esc_id=0;
            switch (fspage_filelist->media_type)
            {
                case MEDIA_TYPE_VIDEO:
                    esc_id=5;
                    break;
                case MEDIA_TYPE_MUSIC:
                    esc_id=4;
                    break;
                case MEDIA_TYPE_PHOTO:
                    esc_id=5;
                    break;
                case MEDIA_TYPE_TXT:
                    esc_id=2;
                    break;
                default :
                    break;

            }
            if(sel_id==esc_id){
                if(lv_obj_is_valid(target))
                    lv_obj_del(target);
                lv_group_focus_obj(user_data);
            }
        }
        
        
    }
    else if(code==LV_EVENT_REFRESH){
        /*do strcmp to judge if obj need to refresh*/ 
        char* current_name=win_media_get_cur_file_name(app_get_playlist_t());
        char* info_name=lv_label_get_text(mpinfo_subobj[0]);
        if(strcmp(current_name,info_name)){
            mp_hdl=mp_get_cur_player_hdl();
            memset(&mp_info,0,sizeof(mp_info_t));
            media_get_info(mp_hdl,&mp_info);
            win_refresh_info(mp_hdl->type,mp_info);
        }
    }else if(code==LV_EVENT_DELETE){
        sel_id=0;
    }
}



