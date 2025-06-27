//This file is used to handle lvgl ui related logic and operations
//all most ui draw in local mp ui.c 
#include "app_config.h"
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"

#include "media_player.h"
#include <dirent.h>
#include "glist.h"
#include <sys/stat.h>
#include "win_media_list.h"
#include <hcuapi/input-event-codes.h>
#include <hcuapi/dis.h>
#include <ffplayer.h>
#include "factory_setting.h"
#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_mainpage.h"
#include "mp_ctrlbarpage.h"
#include "mp_fspage.h"
#include "mp_ebook.h"
#include "screen.h"
#include "../../volume/volume.h"
#include "setup.h"

#include "mul_lang_text.h"

#include "media_spectrum.h"
#include "mp_bsplayer_list.h"
#include "backstage_player.h"
#include "mp_playlist.h"
#include "mp_preview.h"
#include "mp_playerinfo.h"
#include "mp_spectdis.h"
#include "com_api.h"
#include "mp_zoom.h"
#include "mp_key_num.h"
#include <string.h>
#include "app_log.h"
#ifdef HC_MEDIA_MEMMORY_PLAY
#include "memmory_play.h"
#endif
lv_timer_t * sec_counter_timer = NULL;
lv_timer_t * bar_show_timer = NULL;
bool m_play_bar_show = true;
extern media_handle_t *m_cur_media_hld; 

static media_handle_t *m_player_hld[MEDIA_TYPE_COUNT] = {NULL,};
lv_obj_t * focus_obj=NULL;
extern SCREEN_TYPE_E prev_scr;
extern SCREEN_TYPE_E last_scr;
extern SCREEN_TYPE_E cur_scr;
extern SCREEN_SUBMP_E screen_submp;
static bool blacklight_val=false;
static int _media_auto_seekopt(media_state_t set_state);
int Ctrlbar_mediastate_refr(void);
static mpimage_effect_t mpimage_effect={0};
static void mpimage_effect_param_set(int type);
static int win_ctrlbar_ui_update(void);

static char *m_str_ff[] = {"", ">> x2", ">> x4", ">> x8", ">> x16", ">> x24", ">> x32"};
static char *m_str_fb[] = {"", "<< x2", "<< x4", "<< x8", "<< x16", "<< x24", "<< x32"};
static char *m_str_sf[] = {"", ">> 1/2", ">> 1/4", ">> 1/8", ">> 1/16", ">> 1/24"};
static char *m_str_sb[] = {"", "<< 1/2", "<< 1/4", "<< 1/8", "<< 1/16", "<< 1/24"};
static char *m_str_zi[] = {"", "x2", "x4", "x8"};
static char *m_str_zo[] = {"", "x1/2", "x1/4", "x1/8"};


static bool isGifFile(const char* filename) {
    const char* extension = strrchr(filename, '.');
    if (extension != NULL) {
        if (strcasecmp(extension, ".gif") == 0) {
            return true;  
        }
    }
    return false;  
}
#ifdef HC_MEDIA_MEMMORY_PLAY
void save_cur_play_info_to_disk(int time)
{
    if(m_cur_file_list != NULL && (m_cur_file_list->file_count > 0))
    {
        file_node_t *file_node = NULL;
        char m_play_path_name[1024];
        file_node = file_mgr_get_file_node(app_get_playlist_t(),app_get_playlist_index());
        file_mgr_get_fullname(m_play_path_name, m_cur_file_list->dir_path, file_node->name);
        sys_data_set_media_play_cur_time(m_play_path_name, m_cur_media_hld->type, time);
    }
    memmory_play_set_state(0);
}

void media_info_save_memmory(void *media_info, char *file_name,int type)
{
    //do not do memory play for image.
    if (MEDIA_TYPE_PHOTO == type)
        return;
/*
    play_info_t media_info_data;
    media_info_data.current_time = 0;
    media_info_data.current_offset = 0;
    media_info_data.last_page = 0;
    memcpy(media_info_data.path,file_name,sizeof(media_info_data.path));
    media_info_data.type = type;
    */
    play_info_t media_info_data;
    memcpy(&media_info_data, media_info, sizeof(play_info_t));
    sys_data_set_media_info(&media_info_data);
}

#endif
static int user_vkey_action_ctrl(uint32_t user_vkey,lv_obj_t* target)
{
    media_state_t P_STA;
    char* full_filename=NULL;
    char *m_play_file_name=NULL;
    int key_ret = 0;
    switch (user_vkey){
                case V_KEY_RIGHT :
                    if(lv_group_get_focused(lv_group_get_default())==ui_playbar){
                        __media_seek_proc(LV_KEY_RIGHT);
                    }
                    else{
                        if(lv_obj_get_index(target)==RIGHT_SCROOLL_IDX){
                            lv_obj_set_scroll_snap_x(lv_obj_get_parent(target),LV_SCROLL_SNAP_START);
                        }else{
                             lv_obj_set_scroll_snap_x(lv_obj_get_parent(target),LV_SCROLL_SNAP_NONE);
                        }
                        lv_group_focus_next(lv_group_get_default());
                    }
                    show_play_bar(true);
                    lv_timer_reset(bar_show_timer);
                    break;
                case V_KEY_LEFT :
                    if(lv_group_get_focused(lv_group_get_default())==ui_playbar){
                        __media_seek_proc(LV_KEY_LEFT);
                    }else{
                        if(lv_obj_get_index(target)==LEFT_SCROOLL_IDX){
                            lv_obj_set_scroll_snap_x(lv_obj_get_parent(target),LV_SCROLL_SNAP_END);
                        }else{
                             lv_obj_set_scroll_snap_x(lv_obj_get_parent(target),LV_SCROLL_SNAP_NONE);
                        }
                        lv_group_focus_prev(lv_group_get_default());
                    }
                    show_play_bar(true);
                    lv_timer_reset(bar_show_timer);
                    break; 
                case V_KEY_UP :
                    if(m_cur_file_list->media_type==MEDIA_TYPE_VIDEO||m_cur_file_list->media_type==MEDIA_TYPE_MUSIC){
                        //used for bar if 
                        if(lv_group_get_focused(lv_group_get_default())!=ui_playbar)
                        {
                            focus_obj=lv_group_get_focused(lv_group_get_default());
                            lv_group_add_obj(lv_group_get_default(),ui_playbar);
                            lv_group_focus_obj(ui_playbar);
                        }
                    }
                    show_play_bar(true);
                    lv_timer_reset(bar_show_timer);
                    break;
                case V_KEY_DOWN:
                    if(m_cur_file_list->media_type==MEDIA_TYPE_VIDEO||m_cur_file_list->media_type==MEDIA_TYPE_MUSIC){
                        //used for bar 
                        if(lv_group_get_focused(lv_group_get_default())==ui_playbar)
                        {
                            lv_obj_clear_state(ui_playbar,LV_STATE_ANY);
                            lv_group_remove_obj(ui_playbar);
                            lv_group_focus_obj(focus_obj);
                        }
                    }
                    show_play_bar(true);
                    lv_timer_reset(bar_show_timer);
                    break;
                case V_KEY_ENTER:
                    if(target == ui_playbar){
                        /* group focus on progress bar ,do not reponse this key*/
                        break;
                    }
                    key_ret = ctrlbar_btn_enter(target);
                    if(key_ret == 0){  
                        show_play_bar(true);
                        lv_timer_reset(bar_show_timer);
                    }
                    break;
                case V_KEY_EXIT : 
                    if (m_play_bar_show){
                        show_play_bar(false);
                    }
                    break;
                case V_KEY_PLAY:
                case V_KEY_PAUSE:
                    if(m_cur_media_hld!=NULL){
                        P_STA= media_get_state(m_cur_media_hld);
                        if(P_STA == MEDIA_PAUSE){
                            media_resume(m_cur_media_hld);
                            lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                            set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                            set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
                        }else {
                            media_pause(m_cur_media_hld);
                            lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                            set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                            set_label_text2(ui_playstate,STR_PAUSE,FONT_MID);
                        }
                    }//ebook has not this func
                    break;
                case V_KEY_STOP:
                    _ui_screen_change(ui_fspage,0,0);
                    break;                
                case V_KEY_FB:
                    if(m_cur_media_hld!=NULL&&m_cur_media_hld->type==MEDIA_TYPE_VIDEO){
                        P_STA= media_get_state(m_cur_media_hld);
                        if (P_STA!=MEDIA_STOP ){
                            media_fastbackward(m_cur_media_hld);
                        }
                        Ctrlbar_mediastate_refr();
                    }
                    break;
                case V_KEY_FF:
                    if(m_cur_media_hld!=NULL&&m_cur_media_hld->type==MEDIA_TYPE_VIDEO){
                        P_STA= media_get_state(m_cur_media_hld);
                        if (P_STA!=MEDIA_STOP ){
                            media_fastforward(m_cur_media_hld);
                        }    
                        Ctrlbar_mediastate_refr();
                    }
                    break;
                case V_KEY_PREV:
                    full_filename = win_media_get_pre_file(app_get_playlist_t()); 
                    if (full_filename){
                        if(m_cur_media_hld!=NULL){
                            P_STA= media_get_state(m_cur_media_hld);
                            if (MEDIA_STOP != P_STA)
                                media_stop(m_cur_media_hld);
                            media_play(m_cur_media_hld, full_filename);
                        }else{
                            ebook_read_file(full_filename);
                        }
                        m_play_file_name = win_media_get_cur_file_name(app_get_playlist_t());
                        lv_label_set_text(ui_playname, m_play_file_name);
                    }
                    break;                
                case V_KEY_NEXT:
                    full_filename = win_media_get_next_file(app_get_playlist_t()); 
                    if (full_filename){
                        if(m_cur_media_hld!=NULL){
                            P_STA= media_get_state(m_cur_media_hld);
                            if (MEDIA_STOP != P_STA)
                                media_stop(m_cur_media_hld);
                            media_play(m_cur_media_hld, full_filename);
                        }else{
                            ebook_read_file(full_filename);
                        }
                        m_play_file_name = win_media_get_cur_file_name(app_get_playlist_t());
                        lv_label_set_text(ui_playname, m_play_file_name);
                    }
                    break;
                default:   
                    break;
            }

    return 0;
}

void ctrl_bar_keyinput_event_cb(lv_event_t *event){
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target =lv_event_get_target(event);
    if(code == LV_EVENT_PRESSED){
        lv_obj_clear_state(target,LV_STATE_PRESSED);
    }else if(code == LV_EVENT_KEY){
        uint32_t lv_key = lv_indev_get_key(lv_indev_get_act());
        uint32_t user_vkey=key_convert_vkey(lv_key);   // other key convert2 user_vkey

        // for backlight func press any btn to set backlight  on when backlight is off 
        if(blacklight_val==true){
            app_set_blacklight(blacklight_val);
            printf("backlight:%d\n",blacklight_val);
            blacklight_val=!blacklight_val;
            return;
        }
        if(lv_obj_has_flag(ui_play_bar,LV_OBJ_FLAG_HIDDEN)){
            //mean ctrlbar is hidden
            if(lv_key>LV_KEY_ESC){
                //mean press user_vkey
               user_vkey_action_ctrl(user_vkey,target);
            }else{
                if(lv_key==LV_KEY_ESC){
                    _ui_screen_change(ui_fspage,0,0);
                }else{
                    show_play_bar(true);
                    lv_timer_reset(bar_show_timer);
                }
            } 
        }else{
            //mean ctrlbar is on show 
            user_vkey_action_ctrl(user_vkey,target);    
        } 
    }
}


static volatile uint8_t m_cur_is_mode_sel = 0;

#define IMG_DIS_ALL_ENTRIES()     \
    DIS_ENTRY(IMG_DIS_AUTO, STR_IMG_DIS_AUTO, "AUTO")     \
    DIS_ENTRY(IMG_DIS_REALSIZE, STR_IMG_DIS_REALSIZE, "Realsize")     \
    DIS_ENTRY(IMG_DIS_CROP, STR_IMG_DIS_CROP, "Crop")    \
    DIS_ENTRY(IMG_DIS_FULLSCREEN, STR_IMG_DIS_FULLSCREEN, "Full screen")    
//    DIS_ENTRY(IMG_DIS_THUMBNAIL, STR_IMG_DIS_THUMBNAIL, "Thumbnail")

#define DIS_ENTRY(mode, id, str)    mode,
static img_dis_mode_e m_img_dis_mode_array[] = 
{
    IMG_DIS_ALL_ENTRIES()
};
#undef DIS_ENTRY

#define DIS_ENTRY(mode, id, str)    id,
static uint16_t m_img_dis_id_array[] = 
{
    IMG_DIS_ALL_ENTRIES()
};
#undef DIS_ENTRY

#define DIS_ENTRY(mode, id, str)    str,
static char *m_img_dis_str_array[] = 
{
    IMG_DIS_ALL_ENTRIES()
};
#undef DIS_ENTRY

#define DIS_MODE_COUNT  sizeof(m_img_dis_mode_array)/sizeof(m_img_dis_mode_array[0])
static void _ctrl_bar_img_dis_mode_switch(void)
{
    m_cur_is_mode_sel++;
    if (m_cur_is_mode_sel >= DIS_MODE_COUNT)
        m_cur_is_mode_sel = 0;

    media_play_set_img_dis_mode(m_img_dis_mode_array[m_cur_is_mode_sel]);

    set_label_text2(lv_obj_get_child(ctrlbarbtn[15],0),m_img_dis_id_array[m_cur_is_mode_sel],FONT_MID); 

    printf("%s(), mod_sel=%d, dis_mod:%s\n", __func__, m_cur_is_mode_sel, m_img_dis_str_array[m_cur_is_mode_sel]);    
}


uint16_t ctrl_bar_get_img_dis_str_id(void)
{
    return m_img_dis_id_array[m_cur_is_mode_sel];
}

/* need to update media ui obj with its logic  */
int ctrlbar_btn_enter(lv_obj_t* target)
{
    int btnid = lv_obj_get_index(target);
    int ret = 0;
    if(m_cur_file_list->media_type == MEDIA_TYPE_VIDEO){
        switch(btnid){
            case VIDEO_PLAY_ID :
                {
                    if(media_get_state(m_cur_media_hld) == MEDIA_PAUSE){
                        media_resume(m_cur_media_hld);
                        /* unit ref media_state */
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                    }else{
                        media_pause(m_cur_media_hld);
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                    }
                    Ctrlbar_mediastate_refr();
                }
                break;
            case VIDEO_FB_ID :
                if(media_get_state(m_cur_media_hld) != MEDIA_PAUSE){
                    media_fastbackward(m_cur_media_hld);
                    /* ref media_state*/
                    Ctrlbar_mediastate_refr();
                }
                break;
            case VIDEO_FF_ID :
                if(media_get_state(m_cur_media_hld) != MEDIA_PAUSE){
                    media_fastforward(m_cur_media_hld);
                    Ctrlbar_mediastate_refr();
                }
                break;
            case VIDEO_PREV_ID :
                {
                    char* play_name = win_media_get_pre_file(app_get_playlist_t()); 
                    if (play_name){
                        media_stop(m_cur_media_hld);
                        media_play(m_cur_media_hld, play_name);
                        /* ref ctrlbar-> obj */
                        win_ctrlbar_ui_update();
                    }
                }
                break;
            case VIDEO_NEXT_ID :
                {
                    char* play_name = win_media_get_next_file(app_get_playlist_t()); 
                    if (play_name){
                        media_stop(m_cur_media_hld);
                        media_play(m_cur_media_hld, play_name);
                        win_ctrlbar_ui_update();
                    }
                }
                break;
            case VIDEO_STOP_ID :
                _ui_screen_change(ui_fspage,0,0);
                ret = -1;
                break;
            case VIDEO_PLAYLOOP_ID:
                if(m_cur_media_hld->loop_type == PlAY_LIST_SEQUENCE){
                    media_looptype_state_set(m_cur_media_hld, PLAY_LIST_ONE);
                    set_label_text2(lv_obj_get_child(ctrlbarbtn[6],0),STR_SINGLEROUND,FONT_MID);
                }else if(m_cur_media_hld->loop_type == PLAY_LIST_ONE) {
                    media_looptype_state_set(m_cur_media_hld, PlAY_LIST_SEQUENCE);
                    set_label_text2(lv_obj_get_child(ctrlbarbtn[6],0),STR_LISTROUND,FONT_MID);
                }
                break;
            case VIDEO_INFO_ID :
                create_mpinfo_win(lv_scr_act(),target);
                break;
            case VIDEO_PLAYLIST_ID : 
                create_playlist_win(lv_scr_act(),target);
                break;
            case VIDEO_ZOOMIN_ID : 
                {
                    if(m_cur_media_hld->state!=MEDIA_PLAY_END){
                        int ret = meida_display_zoom(m_cur_media_hld,MPZOOM_IN);
                        if(ret < 0){
                            app_log(LL_DEBUG,"media_display_zoom return fail");
                            break;
                        }
                        /*refr media ui -> obj */
                        Zoom_Param_t* zoom_mode=app_get_zoom_param();
                        if(zoom_mode->zoom_state>0){
                            set_label_text2(ui_playstate,STR_ZOOMIN,FONT_MID);
                            lv_label_set_text(ui_speed, m_str_zi[abs(zoom_mode->zoom_state)]);
                        }else if (zoom_mode->zoom_state == 0){
                            Ctrlbar_mediastate_refr();
                            lv_label_set_text(ui_speed, m_str_zi[abs(zoom_mode->zoom_state)]);
                        }else if(zoom_mode->zoom_state < 0){
                            set_label_text2(ui_playstate,STR_ZOOMOUT,FONT_MID);                  
                            lv_label_set_text(ui_speed, m_str_zo[abs(zoom_mode->zoom_state)]);
                        }
                    }
                }
                break;
            case VIDEO_ZOOMOUT_ID :
                {
                    if(m_cur_media_hld->state!=MEDIA_PLAY_END){
                        int ret = meida_display_zoom(m_cur_media_hld,MPZOOM_OUT);
                        if(ret < 0){
                            app_log(LL_DEBUG,"media_display_zoom return fail");
                            break;
                        }
                        /*refr media_ui -> obj */
                        Zoom_Param_t* zoom_mode=app_get_zoom_param();
                        if(zoom_mode->zoom_state > 0){
                            set_label_text2(ui_playstate,STR_ZOOMIN,FONT_MID);
                            lv_label_set_text(ui_speed, m_str_zi[abs(zoom_mode->zoom_state)]);
                        }else if(zoom_mode->zoom_state < 0){
                            set_label_text2(ui_playstate,STR_ZOOMOUT,FONT_MID);                  
                            lv_label_set_text(ui_speed, m_str_zo[abs(zoom_mode->zoom_state)]);
                        }else if (zoom_mode->zoom_state == 0){
                            Ctrlbar_mediastate_refr();
                            lv_label_set_text(ui_speed, m_str_zo[abs(zoom_mode->zoom_state)]);
                        }
                    }
                }
                break;
            case VIDEO_SF_ID :
                {
                    if(m_cur_media_hld->state != MEDIA_PAUSE){
                        media_slowforward(m_cur_media_hld);
                        Ctrlbar_mediastate_refr();
                    }
                }
                break;
            case VIDEO_STEP_ID : 
                {
                    if(media_get_state(m_cur_media_hld) == MEDIA_PAUSE ){
                        media_resume(m_cur_media_hld);
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                    }
                    api_sleep_ms(200);
                    media_pause(m_cur_media_hld);
                    lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                    set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                    Ctrlbar_mediastate_refr();
                }
                break;
            case VIDEO_RATIO_ID: 
                {
                    switch (m_cur_media_hld->ratio_mode){
                        case DIS_TV_AUTO:
                            if(media_display_ratio_set(m_cur_media_hld, DIS_TV_4_3) == API_SUCCESS){
                                set_label_text2(lv_obj_get_child(ctrlbarbtn[13],0),STR_ASPECT_4_3,FONT_MID);
                            }
                            break;
                        case DIS_TV_4_3:
                            if(media_display_ratio_set(m_cur_media_hld, DIS_TV_16_9) == API_SUCCESS){
                                set_label_text2(lv_obj_get_child(ctrlbarbtn[13],0),STR_ASPECT_16_9,FONT_MID);
                            }
                            break;
                        case DIS_TV_16_9:
                            if(media_display_ratio_set(m_cur_media_hld, DIS_TV_AUTO) == API_SUCCESS){
                                set_label_text2(lv_obj_get_child(ctrlbarbtn[13],0),STR_ASPECT_AUTO,FONT_MID);
                            }
                            break;
                        default:
                            break;
                    }
                    /* refr meida_ui -> obj show */
                    Zoom_Param_t* zoom_mode=app_get_zoom_param();
                    if (zoom_mode->zoom_state == 0){
                        Ctrlbar_mediastate_refr();
                        lv_label_set_text(ui_speed, m_str_zo[abs(zoom_mode->zoom_state)]);
                    }
                }
                break;

        }
    }else if (m_cur_file_list->media_type == MEDIA_TYPE_MUSIC){
        switch(btnid){
            case MUSIC_PLAY_ID :
                {
                    if(media_get_state(m_cur_media_hld) == MEDIA_PAUSE){
                        media_resume(m_cur_media_hld);
                        /* unit ref media_state */
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                    }else{
                        media_pause(m_cur_media_hld);
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                    }
                    Ctrlbar_mediastate_refr();
                }
                break;
            case MUSIC_FB_ID :
                if(media_get_state(m_cur_media_hld) == MEDIA_PAUSE){
                    media_resume(m_cur_media_hld);
                    lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                    set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                }
                media_manual_fbsetting(m_cur_media_hld);
                Ctrlbar_mediastate_refr();
                break;
            case MUSIC_FF_ID :
                if(media_get_state(m_cur_media_hld) == MEDIA_PAUSE){
                    media_resume(m_cur_media_hld);
                    lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                    set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                }
                media_manual_ffsetting(m_cur_media_hld);
                Ctrlbar_mediastate_refr();
                break;
            case MUSIC_PREV_ID :
                {
                    char* play_name = win_media_get_pre_file(app_get_playlist_t()); 
                    if (play_name){
                        media_stop(m_cur_media_hld);
                        media_play(m_cur_media_hld, play_name);
                        /* ref ctrlbar-> obj */
                        win_ctrlbar_ui_update();
                    }
                }
                break;
            case MUSIC_NEXT_ID :
                {
                    char* play_name = win_media_get_next_file(app_get_playlist_t()); 
                    if (play_name){
                        media_stop(m_cur_media_hld);
                        media_play(m_cur_media_hld, play_name);
                        win_ctrlbar_ui_update();
                    }
                }
                break;
            case MUSIC_STOP_ID :
                _ui_screen_change(ui_fspage,0,0);
                ret = -1;
                break;
            case MUSIC_PLAYLOOP_ID:
                if(m_cur_media_hld->loop_type == PlAY_LIST_SEQUENCE){
                    media_looptype_state_set(m_cur_media_hld, PLAY_LIST_ONE);
                    set_label_text2(lv_obj_get_child(ctrlbarbtn[6],0),STR_SINGLEROUND,FONT_MID);
                }else if(m_cur_media_hld->loop_type == PLAY_LIST_ONE) {
                    media_looptype_state_set(m_cur_media_hld, PlAY_LIST_SEQUENCE);
                    set_label_text2(lv_obj_get_child(ctrlbarbtn[6],0),STR_LISTROUND,FONT_MID);
                }
                break;
            case MUSIC_INFO_ID :
                create_mpinfo_win(lv_scr_act(),target);
                break;
            case MUSIC_PLAYLIST_ID : 
                create_playlist_win(lv_scr_act(),target);
                break;
            case MUSIC_BACKLIGT_ID : 
                app_set_blacklight(blacklight_val); //off backlight
                blacklight_val=!blacklight_val;
                break;
            case MUSIC_MUTE_ID : 
                {
                    ui_mute_set();
                }
                break;
        }
    }else if (m_cur_file_list->media_type == MEDIA_TYPE_PHOTO){
         switch(btnid){
            case PHOTO_PLAY_ID :
                {
                    if(media_get_state(m_cur_media_hld) == MEDIA_PAUSE){
                        media_resume(m_cur_media_hld);
                        /* unit ref media_state */
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
                    }else{
                        media_pause(m_cur_media_hld);
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                    }
                    Ctrlbar_mediastate_refr();
                }
                break;
            case PHOTO_CLOCKWISE_ID :
                if(media_get_state(m_cur_media_hld) == MEDIA_PLAY){
                    media_pause(m_cur_media_hld);
                }
                media_change_rotate_type(m_cur_media_hld ,CLOCKWISE);
                lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                Ctrlbar_mediastate_refr();
                break;
            case PHOTO_ANTICLOCKWISE_ID :
                if(media_get_state(m_cur_media_hld) == MEDIA_PLAY){
                    media_pause(m_cur_media_hld);
                }
                media_change_rotate_type(m_cur_media_hld ,ANTICLOCKWISE);
                lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                Ctrlbar_mediastate_refr();
                break;
            case PHOTO_PREV_ID :
                {
                    char* play_name = win_media_get_pre_file(app_get_playlist_t()); 
                    if (play_name){
                        media_stop(m_cur_media_hld);
                        media_play(m_cur_media_hld, play_name);
                        /* ref ctrlbar-> obj */
                        win_ctrlbar_ui_update();
                    }
                }
                break;
            case PHOTO_NEXT_ID :
                {
                    char* play_name = win_media_get_next_file(app_get_playlist_t()); 
                    if (play_name){
                        media_stop(m_cur_media_hld);
                        media_play(m_cur_media_hld, play_name);
                        win_ctrlbar_ui_update();
                    }
                }
                break;
            case PHOTO_STOP_ID :
                _ui_screen_change(ui_fspage,0,0);
                ret = -1;
                break;
            case PHOTO_PLAYLOOP_ID:
                if(m_cur_media_hld->loop_type == PlAY_LIST_SEQUENCE){
                    media_looptype_state_set(m_cur_media_hld, PLAY_LIST_ONE);
                    set_label_text2(lv_obj_get_child(ctrlbarbtn[6],0),STR_SINGLEROUND,FONT_MID);
                }else if(m_cur_media_hld->loop_type == PLAY_LIST_ONE) {
                    media_looptype_state_set(m_cur_media_hld, PlAY_LIST_SEQUENCE);
                    set_label_text2(lv_obj_get_child(ctrlbarbtn[6],0),STR_LISTROUND,FONT_MID);
                }
                break;
            case PHOTO_INFO_ID :
                create_mpinfo_win(lv_scr_act(),target);
                break;
            case PHOTO_PLAYLIST_ID : 
                create_playlist_win(lv_scr_act(),target);
                break;
            case PHOTO_ZOOMIN_ID : 
                {
                    if (media_get_state(m_cur_media_hld) == MEDIA_PLAY){
                        media_pause(m_cur_media_hld);
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                    } 
                    if(m_cur_media_hld->state!=MEDIA_PLAY_END){
                        int ret = meida_display_zoom(m_cur_media_hld,MPZOOM_IN);
                        if(ret < 0){
                            app_log(LL_DEBUG,"media_display_zoom return fail");
                            break;
                        }
                        Zoom_Param_t* zoom_mode=app_get_zoom_param();
                        if(zoom_mode->zoom_state>0){
                            set_label_text2(ui_playstate,STR_ZOOMIN,FONT_MID);
                            lv_label_set_text(ui_speed, m_str_zi[abs(zoom_mode->zoom_state)]);
                            if(lv_obj_has_state(ctrlbarbtn[PHOTO_ZOOMMOVE_ID],LV_STATE_DISABLED))
                                lv_obj_clear_state(ctrlbarbtn[PHOTO_ZOOMMOVE_ID],LV_STATE_DISABLED);
                        }else if (zoom_mode->zoom_state == 0){
                            Ctrlbar_mediastate_refr();
                            lv_label_set_text(ui_speed, m_str_zi[abs(zoom_mode->zoom_state)]);
                            lv_obj_add_state(ctrlbarbtn[PHOTO_ZOOMMOVE_ID],LV_STATE_DISABLED);
                        }else if(zoom_mode->zoom_state < 0){
                            set_label_text2(ui_playstate,STR_ZOOMOUT,FONT_MID);                  
                            lv_label_set_text(ui_speed, m_str_zo[abs(zoom_mode->zoom_state)]);
                        }
                    }
                }
                break;
            case PHOTO_ZOOMOUT_ID :
                {
                    if (media_get_state(m_cur_media_hld) == MEDIA_PLAY){
                        media_pause(m_cur_media_hld);
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Play,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PLAY,FONT_MID);
                    }                
                    if(m_cur_media_hld->state!=MEDIA_PLAY_END){
                        int ret = meida_display_zoom(m_cur_media_hld,MPZOOM_OUT);
                        if(ret < 0){
                            app_log(LL_DEBUG,"media_display_zoom return fail");
                            break;
                        }
                        Zoom_Param_t* zoom_mode=app_get_zoom_param();
                        if(zoom_mode->zoom_state>0){
                            set_label_text2(ui_playstate,STR_ZOOMIN,FONT_MID);
                            lv_label_set_text(ui_speed, m_str_zi[abs(zoom_mode->zoom_state)]);
                            if(lv_obj_has_state(ctrlbarbtn[PHOTO_ZOOMMOVE_ID],LV_STATE_DISABLED))
                                lv_obj_clear_state(ctrlbarbtn[PHOTO_ZOOMMOVE_ID],LV_STATE_DISABLED);
                        }else if (zoom_mode->zoom_state == 0){
                            Ctrlbar_mediastate_refr();
                            lv_label_set_text(ui_speed, m_str_zi[abs(zoom_mode->zoom_state)]);
                            lv_obj_add_state(ctrlbarbtn[PHOTO_ZOOMMOVE_ID],LV_STATE_DISABLED);
                        }else if(zoom_mode->zoom_state < 0){
                            set_label_text2(ui_playstate,STR_ZOOMOUT,FONT_MID);                  
                            lv_label_set_text(ui_speed, m_str_zo[abs(zoom_mode->zoom_state)]);
                        }
                    }
                }
                break;
            case PHOTO_ZOOMMOVE_ID : 
                {
                    Zoom_Param_t* cur_zoomparam=app_get_zoom_param();
                    if(cur_zoomparam->zoom_size>ZOOM_NORMAL)
                        create_zoommoove_win(ui_play_bar,target);
                }
                break;
            case PHOTO_ONOFF_MUSIC_ID : 
                {
                    if(backstage_media_handle_get() == NULL){
                        /*start play music if has songs on bslist*/
                        glist* bsplaylist=app_get_bsplayer_glist();
                        if(bsplaylist!=NULL&&bsplaylist->next!=NULL){
                            lv_obj_set_style_bg_img_src(ctrlbarbtn[12],&IDB_Hint_Music_On,0);
                            set_label_text2(lv_obj_get_child(ctrlbarbtn[12],0),STR_MUSIC_OFF,FONT_MID);
                            backstage_player_task_start(0,NULL);
                        }
                    }else{
                        lv_obj_set_style_bg_img_src(ctrlbarbtn[12],&IDB_Hint_Music_Off,0);
                        set_label_text2(lv_obj_get_child(ctrlbarbtn[12],0),STR_MUSIC_ON,FONT_MID);
                        backstage_player_task_stop(0,NULL);
                    }               
                }
                break;
            case PHOTO_MUSIC_RES_ID : 
                create_musiclist_win(lv_scr_act(),target);
                break;
            case PHOTO_EFFECT_ID : 
                {
                    switch(mpimage_effect.type){
                        case EFFECT_NULL:
                            mpimage_effect.type=EFFECT_SHUTTERS;
                            set_label_text2(lv_obj_get_child(target,0),STR_SHUTTER_EFFECT,FONT_MID);        
                            break;
                        case EFFECT_SHUTTERS: 
                            mpimage_effect.type=EFFECT_BRUSH;
                            set_label_text2(lv_obj_get_child(target,0),STR_BRUSH_EFFECT,FONT_MID);        
                            break;
                        case EFFECT_BRUSH: 
                            mpimage_effect.type=EFFECT_SLIDE;
                            set_label_text2(lv_obj_get_child(target,0),STR_SLIDE_EFFECT,FONT_MID);        
                            break;
                        case EFFECT_SLIDE: 
                            mpimage_effect.type=EFFECT_MOSAIC;
                            set_label_text2(lv_obj_get_child(target,0),STR_MOSAIC_EFFECT,FONT_MID);        
                            break;
                        case EFFECT_MOSAIC: 
                            mpimage_effect.type=EFFECT_FADE;
                            set_label_text2(lv_obj_get_child(target,0),STR_FADE_EFFECT,FONT_MID);        
                            break;
                        case EFFECT_FADE: 
                            mpimage_effect.type=EFFECT_RANDOM;
                            set_label_text2(lv_obj_get_child(target,0),STR_RAND_EFFECT,FONT_MID);
                            break;
                        case EFFECT_RANDOM:
                            mpimage_effect.type=EFFECT_NULL;
                            set_label_text2(lv_obj_get_child(target,0),STR_NO_EFFECT,FONT_MID);        
                        default :
                            break;
                    } 
                    mpimage_effect_param_set(mpimage_effect.type);
                }               
                break;
            case PHOTO_MODE_ID : 
                #ifdef IMAGE_DISPLAY_MODE_SUPPORT
                _ctrl_bar_img_dis_mode_switch();
                #endif
                break;
        }
    }else if(m_cur_file_list->media_type == MEDIA_TYPE_TXT){
        switch(btnid){
            case EBOOK_PAGEUP_ID :
                change_ebook_txt_info(LV_KEY_UP,0);
                break;
            case EBOOK_PAGEDOWN_ID : 
                change_ebook_txt_info(LV_KEY_DOWN , 0);
                break;
            case EBOOK_PREV_ID : 
                {
                    char *play_name = win_media_get_pre_file(app_get_playlist_t()); 
                    if(play_name){
                        ebook_read_file(play_name);
                        char * m_play_file_name = win_media_get_cur_file_name(app_get_playlist_t());
                        lv_label_set_text(ui_playname, m_play_file_name);
                    }
                }
                break;
            case EBOOK_NEXT_ID : 
                {
                    char *play_name = win_media_get_next_file(app_get_playlist_t()); 
                    if(play_name){
                        ebook_read_file(play_name);
                        char * m_play_file_name = win_media_get_cur_file_name(app_get_playlist_t());
                        lv_label_set_text(ui_playname, m_play_file_name);
                    }
                }
                break;
            case EBOOK_STOP_ID : 
                _ui_screen_change(ui_fspage,0,0);
                ret = -1;
                break;
            case EBOOK_ONOFF_MUSIC_ID : 
                {
                    if(backstage_media_handle_get() == NULL){
                        //start play music if has songs on bslist
                        glist* bsplaylist=app_get_bsplayer_glist();
                        if(bsplaylist!=NULL&&bsplaylist->next!=NULL){
                            lv_obj_set_style_bg_img_src(target,&IDB_Hint_Music_On,0);
                            set_label_text2(lv_obj_get_child(target,0),STR_MUSIC_OFF,FONT_MID);
                            backstage_player_task_start(0,NULL);
                        }   
                    }else{
                        lv_obj_set_style_bg_img_src(target,&IDB_Hint_Music_Off,0);
                        set_label_text2(lv_obj_get_child(target,0),STR_MUSIC_ON,FONT_MID);
                        backstage_player_task_stop(0,NULL);
                    } 
                }
                break;
            case EBOOK_PLAYLIST_ID :
                create_playlist_win(ui_ebook_txt,target);
                break;
            case EBOOK_INFO_ID : 
                create_mpinfo_win(ui_ebook_txt,target);
                break;
            case EBOOK_MUSIC_RES_ID : 
                create_musiclist_win(ui_ebook_txt,target);
                break;
        }
    }
    return ret;
}

void format_time(uint32_t time, char *time_fmt)
{
    uint32_t hour;
    uint32_t min;
    uint32_t second;

    if (0 == time){
        sprintf(time_fmt, "00:00:00");
        return;
    }

    hour = time / 3600;
    min = (time % 3600) / 60;
    second = time % 60;
    if (hour > 0)
        sprintf(time_fmt, "%02lu:%02lu:%02lu", hour, min, second);
    else
        sprintf(time_fmt, "%02lu:%02lu", min, second);

}

void sec_timer_cb(lv_timer_t * t)
{
    uint32_t play_time = 0;
    uint32_t total_time = 0;
    char time_fmt[16];
	control_msg_t ctl_msg = {0};
    play_time = media_get_playtime(m_cur_media_hld);
    format_time(play_time, time_fmt);
    lv_label_set_text(lv_obj_get_child(ui_play_bar,2), time_fmt);
    lv_slider_set_value(ui_playbar, play_time, LV_ANIM_ON);

    total_time = media_get_totaltime(m_cur_media_hld);
    format_time(total_time, time_fmt);
    lv_label_set_text(lv_obj_get_child(ui_play_bar,4), time_fmt);
    if (total_time > 0)
        lv_slider_set_range(ui_playbar, 0, total_time);
#ifdef HC_MEDIA_MEMMORY_PLAY
    if(play_time%30 == 2)
    {
    	ctl_msg.msg_type = MSG_TYPE_REMOTE;
		ctl_msg.msg_code = MSG_TYPE_MP_MEMMORY_SAVE_PLAY_TIME;
    	api_control_send_msg(&ctl_msg);
	}
#endif
    if(m_cur_media_hld->type==MEDIA_TYPE_MUSIC){
        media_manual_seekopt(m_cur_media_hld);
        /*reset mieda_player state and ui display 
        when seek to the begin of process bar*/
        if(m_cur_media_hld->state==MEDIA_FB&&m_cur_media_hld->seek_step==0){
            media_resume(m_cur_media_hld);
            set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
            set_label_text2(ui_speed,STR_NONE,FONT_MID);
        }
    }
}

void show_play_bar(bool show)
{
    if (show){
        lv_obj_clear_flag(ui_play_bar, LV_OBJ_FLAG_HIDDEN);
    }else{
        lv_obj_add_flag(ui_play_bar, LV_OBJ_FLAG_HIDDEN);
    }
    m_play_bar_show = show;

}

void bar_show_timer_cb(lv_timer_t * t)
{
    show_play_bar(false);

}


#ifdef RTOS_SUBTITLE_SUPPORT
subtitles_event_t subtitles_e = -1;
char *subtitles_str = NULL;
static lv_img_dsc_t subtitles_img_dsc = {0};
lv_timer_t *subtitles_timer = NULL;
uint16_t subtitles_type = -1;

static void subtitles_event_cb(){
    switch (subtitles_e)
    {
    case SUBTITLES_EVENT_SHOW:
        if(subtitles_type == 1){
            show_subtitles(subtitles_str);             
        }else{
            show_subtitles_pic(&subtitles_img_dsc);
        }
        
        break;
    case SUBTITLES_EVENT_HIDDEN:
        if(subtitles_type == 1){
            show_subtitles("");            
        }else{
            show_subtitles_pic(NULL);
        }

        break;
    case SUBTITLES_EVENT_PAUSE:
        printf("subtitles pause\n");
        if(subtitles_timer){
            lv_timer_pause(subtitles_timer);
        }
        break;
    case SUBTITLES_EVENT_CLOSE:
        if(subtitles_timer){
            lv_timer_pause(subtitles_timer);
            lv_timer_del(subtitles_timer);
            subtitles_timer = NULL;
            printf("subtitles close\n");
        }

        break;
    default:
        break;
    }
    subtitles_e = -1;
}

void subtitles_event_send(int e, lv_subtitle_t *subtitle){

    
    if(e == SUBTITLES_EVENT_RESUME){
        if (subtitles_timer ){
            lv_timer_resume(subtitles_timer);
            lv_timer_reset(subtitles_timer);
        }        
    }else if(e == SUBTITLES_EVENT_CLOSE){
        subtitles_e = -1 ;
        if(subtitles_timer){
            show_subtitles(""); 
            show_subtitles_pic(NULL);
        }
    }else{
        subtitles_e = e;
        if(subtitle == NULL){
            return;
        }
        if(subtitle->type == 1){
            subtitles_str = (char *)subtitle->data;     
        }else if(subtitle->type == 0){
            subtitles_img_dsc.header.w = subtitle->w;
            subtitles_img_dsc.header.h = subtitle->h;
            subtitles_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
            subtitles_img_dsc.data_size = subtitle->w*subtitle->h*LV_IMG_PX_SIZE_ALPHA_BYTE;
            subtitles_img_dsc.data = subtitle->data;
        }
       subtitles_type = subtitle->type;
    }

}

static void subtitles_timer_handle(lv_timer_t *e){
    subtitles_event_cb();
}

void create_subtitles_rect(void){

    if(m_cur_media_hld->type == MEDIA_TYPE_VIDEO||m_cur_media_hld->type == MEDIA_TYPE_MUSIC){
        lv_obj_t *obj = lv_obj_create(ui_ctrl_bar);
        lv_obj_move_background(obj);
        lv_obj_set_size(obj, LV_PCT(80), LV_PCT(18));
        lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);
        lv_obj_set_style_pad_all(obj, 0, 0);
        lv_obj_set_style_border_width(obj, 0, 0);
        lv_obj_set_style_outline_width(obj, 0, 0);
        lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

        subtitles_obj = lv_label_create(obj);
        lv_obj_set_style_border_width(subtitles_obj, 0, 0);
        lv_obj_set_style_pad_bottom(subtitles_obj, 0, 0);
        lv_obj_set_size(subtitles_obj, LV_PCT(100), LV_SIZE_CONTENT);
        lv_label_set_text(subtitles_obj, "");
        lv_obj_set_style_text_font(subtitles_obj, &LISTFONT_3000, 0);
        lv_obj_set_style_text_color(subtitles_obj, lv_color_white(), 0);
        lv_obj_set_style_text_align(subtitles_obj, LV_TEXT_ALIGN_CENTER, 0);
        //lv_obj_center(subtitles_obj);
        lv_obj_align(subtitles_obj, LV_ALIGN_BOTTOM_MID, 0, 0);

        subtitles_obj_pic = lv_img_create(obj);
        lv_img_set_src(subtitles_obj_pic, NULL);
        //lv_obj_center(subtitles_obj_pic);
        lv_obj_align(subtitles_obj_pic, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_size(subtitles_obj_pic, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

        if(!subtitles_timer){
            subtitles_e = -1;
            subtitles_timer = lv_timer_create(subtitles_timer_handle, 500, 0);
            //lv_timer_set_repeat_count(subtitles_timer, -1);
            printf("subtitles timer create\n");
            lv_timer_reset(subtitles_timer);
        }



    }

}

void del_subtitles_rect(void){
    if(subtitles_timer){
        lv_timer_pause(subtitles_timer);
        lv_timer_del(subtitles_timer);
        subtitles_timer = NULL;
        lv_obj_del(subtitles_obj->parent);
        /* this obj was local var,form its child to link it*/ 
        subtitles_obj=NULL;
        subtitles_obj_pic=NULL;
        printf("subtitles close\n");
    }
}

void show_subtitles( char *str){
    if(!str){
        lv_label_set_text(subtitles_obj, "");
        return;
    }
    str = subtitles_str_remove_prefix(str);
    lv_obj_set_size(subtitles_obj->parent,lv_pct(80),lv_pct(18));
    subtitles_str_get_text(str);   
    lv_obj_move_foreground(subtitles_obj);
}

void show_subtitles_pic(lv_img_dsc_t *dsc){
    if(dsc == NULL){
        if(!lv_obj_has_flag(subtitles_obj_pic,LV_OBJ_FLAG_HIDDEN)){
            lv_obj_add_flag(subtitles_obj_pic, LV_OBJ_FLAG_HIDDEN);
        }
    }else{
        lv_obj_clear_flag(subtitles_obj_pic, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(subtitles_obj_pic->parent,dsc->header.w,dsc->header.h);
        lv_img_set_src(subtitles_obj_pic, dsc);
        lv_obj_move_foreground(subtitles_obj_pic);
    }

}

char* subtitles_str_remove_prefix(char* str){
    int dot_count = 0;
    for(int i=0; i<strlen(str);i++){
        if(str[i] == ','){
            dot_count++;
            if(dot_count == 9){
                if(i+1 < strlen(str)){
                    return str+i+1;
                }else{
                    return "";
                }
                
            }
        }
    }
    return "";
}
/*
* 
*/
void subtitles_str_get_text(char* str){
    int a=-1, first = -1, second = -1;
    int size = strlen(str) + 1;
    char str_line[CONVSTR_MAXSIZE*2]={0};
    char str_line2[CONVSTR_MAXSIZE*2]={0};
    if(size>512){
        printf("%s,str too long,str size:%d\n", __func__,size);
        return;
    }
    for(int i=0; i<size;i++){
        //printf("%c\n", str[i]);
        if(str[i] == 0x5c && i+1<size && str[i+1] == 'N'){
            str[i] = '\0';
            i+=2;
            a=i;
        }
        while(str[i] == '{'){
            str[i] = '\0';
            a = 1+i;
            while (i < size && str[i] != '}'){
                i++;
            }
            if(i<size && str[i] == '}'){
                str[i] = '\0';
                subtitles_str_set_style(str+a);
            }
            i++;
        }
        if(str[i] == '\n' || str[i] == '\r'){
            str[i] = '\0';
        }

        if(first == -1){
            first = i;
        }else if(a > 0 && second == -1){
            second = i;
        }
        a=-1;
    }
    if(first>=0){
        if(second>first){
            string_fmt_conv_to_utf8((unsigned char*)str+first,str_line);
            string_fmt_conv_to_utf8((unsigned char*)str+second,str_line2);
            lv_label_set_text_fmt(subtitles_obj,"%s\n%s", str_line,str_line2);             
        }else{
            string_fmt_conv_to_utf8((unsigned char*)str+first,str_line);
            lv_label_set_text(subtitles_obj,str_line);
        }
    }else{
        lv_label_set_text(subtitles_obj, "");
    }
}

void subtitles_str_set_style(char *str){
    // if(strncmp(str, "\c&H", 4) == 0){

    // }
}
/**
 * @description: sepecial handle for ext subtitle format .idx , if has this format(xxx.idx)
 * do not add xxx.sub in subtitle list or del it in subs list 
 * @return {*}
 * @author: Yanisin
 */
static glist* ext_subtitle_format_handle(glist* subs_glist)
{
    glist* dst_glist=NULL;
    // scan subglist ,if subsglist has a .idx 
    if(subs_glist!=NULL){
        dst_glist=subs_glist;
        for(int i=0;i<glist_length(subs_glist);i++){
            char *file_str=(char *)glist_nth_data(subs_glist,i);
            if(strstr(file_str,".idx")){
                char file_without_ext[1024]={0};
                file_mgr_rm_extension(file_without_ext,file_str);
                for(int j=0;j<glist_length(subs_glist);j++){
                    if(!strncmp(file_without_ext,(char *)glist_nth_data(subs_glist,j),strlen(file_without_ext))){
                        // finded samename in glist
                        if(file_mgr_optional_filter((char *)glist_nth_data(subs_glist,j),"sub")){
                            dst_glist=glist_delete_link(subs_glist,glist_nth(subs_glist,j));
                        }
                    }
                }
            }
        }
    }
    return dst_glist;
}
static ext_subtitle_t ext_subtitle;
#define MAX_EXT_SUBTITLE_NUM 128
char *m_uris[MAX_EXT_SUBTITLE_NUM]={NULL};
int ext_subtitles_init(file_list_t* src_list)
{
    int j=0;
    if(src_list==NULL||src_list->list==NULL){
        return 0;
    }
    if(src_list->media_type==MEDIA_TYPE_VIDEO||src_list->media_type==MEDIA_TYPE_MUSIC){
        glist* subs_list=file_mgr_subtitile_list_get();
        subs_list=ext_subtitle_format_handle(subs_list);
        // idx file had to specal handle 
        if(subs_list!=NULL){
            int len=glist_length(subs_list);
            file_node_t * file_node = file_mgr_get_file_node(src_list, src_list->item_index);
            if(file_node->name!=NULL){
                char file_without_ext[MAX_FILE_NAME]={0};
                file_mgr_rm_extension(file_without_ext,file_node->name);
                char* sub_without_ext=(char*)malloc(MAX_FILE_NAME);
                if(!sub_without_ext){
                    return -1;
                }
                for(int i=0;i<len;i++){
                    memset(sub_without_ext,0,MAX_FILE_NAME);
                    file_mgr_rm_extension(sub_without_ext,(char *)glist_nth_data(subs_list,i));
                    if(!strcmp(sub_without_ext,file_without_ext)){
                        ext_subtitle.ext_subs_count++;
                        char url_single[MAX_FILE_NAME]={0};
                        file_mgr_get_fullname(url_single,src_list->dir_path,glist_nth_data(subs_list,i));
                        m_uris[j]=strdup(url_single);
                        j++;
                        ext_subtitle.uris=m_uris;
                    }
                }
                free(sub_without_ext);            
            }
        }
    }
    return 0;
}
int ext_subtitle_deinit(void)
{
    glist* subs_list=file_mgr_subtitile_list_get();
    if(subs_list!=NULL&&ext_subtitle.ext_subs_count!=0){
            for(int i=0;i<ext_subtitle.ext_subs_count;i++){
                free(m_uris[i]);
                m_uris[i]=NULL;
            }
        memset(&ext_subtitle,0,sizeof(ext_subtitle_t));
    }
    return 0;
}
ext_subtitle_t * ext_subtitle_data_get(void)
{
    return &ext_subtitle;
}
#endif


//for media play bar 
void ctrlbarpage_open(void)
{
    file_node_t *file_node = NULL;
    char m_play_path_name[MAX_FILE_NAME] = {0};
	char *token;
	char *token_tmp=NULL;
	int last_media_time=0;
#ifdef HC_MEDIA_MEMMORY_PLAY
	save_node_t save_node;
#endif

    api_ffmpeg_player_get_regist(mp_get_cur_player);
    if(last_scr==SCREEN_SETUP||last_scr==SCREEN_CHANNEL)
    {
        if (m_cur_media_hld && MEDIA_PLAY_END == media_get_state(m_cur_media_hld)){
            //Continue play next media if back from channel or setup menu.            
            if(m_cur_media_hld->loop_type==PlAY_LIST_SEQUENCE){
                ctrlbar_btn_enter(ctrlbarbtn[4]); //play next media
            }else if(m_cur_media_hld->loop_type==PLAY_LIST_ONE){
                char * play_name = win_media_get_cur_file_name(app_get_playlist_t()); 
                if (play_name){
                    if (MEDIA_STOP != media_get_state(m_cur_media_hld))
                        media_stop(m_cur_media_hld);
                    media_play(m_cur_media_hld, m_cur_media_hld->play_name);
                    set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
                    set_label_text2(ui_speed,STR_NONE,FONT_MID);
                }
            }

        }
        if (play_bar_group)
            set_key_group(play_bar_group);
        show_play_bar(true);
        Ctrlbar_mediastate_refr();
        ctrlbar_zoomfunction_btn_update();
        //change screen from setup or channel, do not need
        //create object again
        return;
    }
#ifdef SYS_ZOOM_SUPPORT
    app_has_zoom_operation_set(false);
#endif
    //add group 
    play_bar_group= lv_group_create();
    play_bar_group->auto_focus_dis=1;
    set_key_group(play_bar_group);
    create_ctrlbarpage_scr(ui_ctrl_bar,ctrl_bar_keyinput_event_cb);
    file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
    lv_label_set_text(ui_playname,file_node->name);
    set_label_text2(ui_playstate,STR_PLAY,FONT_MID);

    m_cur_media_hld = m_player_hld[m_cur_file_list->media_type];
    if (NULL == m_cur_media_hld){
        m_cur_media_hld = media_open(m_cur_file_list->media_type);
        // media_msg_func_callback_register(m_cur_media_hld,win_media_msg_callback);
        m_player_hld[m_cur_file_list->media_type] = m_cur_media_hld;
    }

    set_label_text2(ui_playstate,STR_PLAY,FONT_MID);
		
    memset(m_play_path_name,0,MAX_FILE_NAME);
    file_mgr_get_fullname(m_play_path_name, m_cur_file_list->dir_path, file_node->name);
#ifdef HC_MEDIA_MEMMORY_PLAY
    if((1==mp_get_auto_play_state()) && 0<=(auto_playing_from_disk(m_cur_file_list,m_play_path_name, \
        &save_node,m_cur_file_list->media_type)))
    {
        mp_set_auto_play_state(0);
        strcpy(m_play_path_name,save_node.save_name);
        token = strtok(save_node.save_name,"/");
        while(token != NULL)
        {
            token = strtok(NULL,"/");
            if(token != NULL)
            {
                token_tmp = token;
            }
        }
        if(token_tmp != NULL)
        {
            int get_ret = file_mgr_get_index_by_file_name(m_cur_file_list, token_tmp);
            if (get_ret < 0)
            {
                printf("memory play error ,file_name:%s \n", token_tmp);
                control_msg_t ctl_msg = {0};
                ctl_msg.msg_type = MSG_TYPE_REMOTE;
                ctl_msg.msg_code = MSG_TYPE_BAR_NO_FILE_MSG;
                api_control_send_msg(&ctl_msg);
                return;
            }

            m_cur_file_list->item_index = get_ret;
            file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
            last_media_time = save_node.save_time;
            strcpy(file_node->name,token_tmp);
        }
    }
    else if(get_cur_playing_from_disk(m_cur_file_list,m_play_path_name,&save_node,m_cur_file_list->media_type)>=0)
    {
        last_media_time = save_node.save_time;
    }
#endif
    playlist_init();/*init it before play media*/
    lv_label_set_text(ui_playname,file_node->name);
    media_play(m_cur_media_hld, m_play_path_name);
    media_seek(m_cur_media_hld, last_media_time);
    //set_display_zoom_when_sys_scale();
    switch(m_cur_media_hld->type){
        case MEDIA_TYPE_VIDEO:
        {
            dis_mode_e dis_mode = projector_get_some_sys_param(P_ASPECT_RATIO) == DIS_TV_AUTO ? DIS_PILLBOX : DIS_NORMAL_SCALE;
            api_set_display_aspect(projector_get_some_sys_param(P_ASPECT_RATIO),dis_mode);
            create_ctrlbar_in_video(ui_winbar);
            break;
        }
        case MEDIA_TYPE_MUSIC:
            create_ctrlbar_in_music(ui_winbar);
            music_spectrum_start();
            api_set_display_aspect(DIS_TV_AUTO,DIS_NORMAL_SCALE);
            break;
        case MEDIA_TYPE_PHOTO:
            create_ctrlbar_in_photo(ui_winbar);
            api_set_display_aspect(DIS_TV_AUTO,DIS_NORMAL_SCALE);
            break;
        default :
            break;
    }
    show_play_bar(true);
    sec_counter_timer = lv_timer_create(sec_timer_cb, 1000, NULL); 
    bar_show_timer = lv_timer_create(bar_show_timer_cb, 5000, NULL);
    screen_submp=SCREEN_SUBMP3;
} 

void media_player_close(void)
{
    
    if(m_cur_media_hld){
        media_stop(m_cur_media_hld);
        media_close(m_cur_media_hld);
        m_player_hld[m_cur_file_list->media_type] = NULL;
        m_cur_media_hld = NULL;

    }
    /* if it play Music album cover reset /dev/dis display area when close player 
        so do it when call media_stop
    dis_zoom_t dis_zoom={
        .src_area.h=1080,
        .src_area.w=1920,
        .src_area.x=0,
        .src_area.y=0,
        .dst_area.h=1080,
        .dst_area.w=1920,
        .dst_area.x=0,
        .dst_area.y=0,
    };
    api_set_display_zoom2(&dis_zoom);
    */
    api_pic_effect_enable(false);
}

void media_player_open(void)
{
    char m_play_path_name[1024];
    file_node_t *file_node = NULL;
    file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
    m_cur_media_hld = m_player_hld[m_cur_file_list->media_type];
    if (NULL == m_cur_media_hld){
        m_cur_media_hld = media_open(m_cur_file_list->media_type);
        m_player_hld[m_cur_file_list->media_type] = m_cur_media_hld;
    }
    file_mgr_get_fullname(m_play_path_name, m_cur_file_list->dir_path, file_node->name);
    media_play(m_cur_media_hld, m_play_path_name);
}

int ctrlbarpage_close(bool force)
{
    win_msgbox_msg_close();

    if (!force){
        //change screen to setup or channel, do not need
        //delete objects. playing background.
        if(cur_scr==SCREEN_CHANNEL||cur_scr==SCREEN_SETUP)//perss EPG /MENU
            return 0;
    }

    if (!play_bar_group)
        return 0;

    if(sec_counter_timer){
        lv_timer_pause(sec_counter_timer);
        lv_timer_del(sec_counter_timer);
        sec_counter_timer = NULL;
    }

    if(bar_show_timer){
        lv_timer_pause(bar_show_timer);
        lv_timer_del(bar_show_timer); 
        bar_show_timer = NULL;
    }

    clear_ctrlbarpage_scr();
    lv_group_remove_all_objs(play_bar_group);
    lv_group_del(play_bar_group);
    play_bar_group=NULL;
    
#ifdef HC_MEDIA_MEMMORY_PLAY
    //only valid playing file save to memory paly
    if (media_get_playtime(m_cur_media_hld) > 5)
	   save_cur_play_info_to_disk(media_get_playtime(m_cur_media_hld));
#endif
    switch(m_cur_media_hld->type){
        case MEDIA_TYPE_VIDEO:
            break;
        case MEDIA_TYPE_MUSIC:
            music_spectrum_stop();
            //reset backlight 
            if(blacklight_val==true){
                app_set_blacklight(blacklight_val);
                printf("backlight:%d\n",blacklight_val);
                blacklight_val=!blacklight_val;
            }
            break;
        case MEDIA_TYPE_PHOTO:
            if(backstage_media_handle_get()){
                backstage_player_task_stop(0,NULL);
            }
            break;
        default :
            break;
    }   

    playlist_deinit();    
    media_player_close();
    return 0;
} 


void ctrlbar_reflesh_speed(void)
{
    uint8_t speed = 0;
    char **str_speed = NULL;

    if (m_cur_media_hld==NULL)
        return ;
    media_state_t play_state = media_get_state(m_cur_media_hld);
    speed = media_get_speed(m_cur_media_hld);
    Zoom_Param_t* zoom_mode=app_get_zoom_param();
    Zoom_size_e z_size =zoom_mode->zoom_size;
    if (speed==0&&z_size==0){
        lv_label_set_text(ui_speed, "");
        return;
    }
    if (MEDIA_FF == play_state){
        str_speed = m_str_ff;
        lv_label_set_text(ui_speed, str_speed[speed]); 
    }else if (MEDIA_FB == play_state){
        str_speed = m_str_fb;
        lv_label_set_text(ui_speed, str_speed[speed]); 
    }else if (MEDIA_SF == play_state){
        str_speed = m_str_sf;
        lv_label_set_text(ui_speed, str_speed[speed]); 
    }else if (MEDIA_SB == play_state){
        str_speed = m_str_sb;
        lv_label_set_text(ui_speed, str_speed[speed]);
    }else{
        lv_label_set_text(ui_speed, "");
    }
}


static void __media_seek_proc(uint32_t key)
{
    uint32_t total_time = media_get_totaltime(m_cur_media_hld);
    uint32_t play_time_initial = media_get_playtime(m_cur_media_hld);
    uint32_t play_time = play_time_initial;
    uint32_t jump_interval = 0;
    uint32_t seek_time = 0;

    if (total_time < jump_interval)
        return ;
    if(play_time_initial>=total_time){
        return;
    }

    int64_t cur_op_time = api_get_sys_clock_time();
    if ((cur_op_time - m_cur_media_hld->last_seek_op_time) < 800) {
        m_cur_media_hld->jump_interval += 10;
    } else {
        m_cur_media_hld->jump_interval = 10;
    }

    m_cur_media_hld->last_seek_op_time = cur_op_time;
    
    if (m_cur_media_hld->jump_interval < 300)
        jump_interval = m_cur_media_hld->jump_interval;
    else
        jump_interval = 300;

    if (LV_KEY_LEFT == key){//seek backward
        if (play_time > jump_interval)
            seek_time = play_time - jump_interval;
        else
            seek_time = 0;
    }else if(LV_KEY_RIGHT==key){ //seek forward
        if ((play_time + jump_interval) > total_time)
            seek_time = total_time;
        else
            seek_time = play_time + jump_interval;
    }
    media_seek(m_cur_media_hld, seek_time);
}


// refr ui_obj in somecase
static int win_ctrlbar_ui_update(void)
{
    // update ui obj -> ui_btn ,ui_filename,ui_state,ui_speed in somecase
    char* filename=win_media_get_cur_file_name(app_get_playlist_t());
    lv_label_set_text(ui_playname,filename);
    lv_obj_set_style_bg_img_src(ctrlbarbtn[0],&Hint_Pause,0);
    set_label_text2(lv_obj_get_child(ctrlbarbtn[0],0),STR_PAUSE,FONT_MID);
    if(m_cur_file_list->media_type==MEDIA_TYPE_PHOTO){
        Zoom_Param_t* zoom_param=(Zoom_Param_t*)app_get_zoom_param();
        if(zoom_param->zoom_size==ZOOM_NORMAL){
            if(lv_group_get_focused(lv_group_get_default()) == ctrlbarbtn[PHOTO_ZOOMMOVE_ID]){
                lv_group_focus_next(lv_group_get_default());
            }
            lv_obj_add_state(ctrlbarbtn[PHOTO_ZOOMMOVE_ID],LV_STATE_DISABLED);
        }else{
            if(lv_obj_has_state(ctrlbarbtn[PHOTO_ZOOMMOVE_ID],LV_STATE_DISABLED))
                lv_obj_clear_state(ctrlbarbtn[PHOTO_ZOOMMOVE_ID],LV_STATE_DISABLED);
        }
    }
    Ctrlbar_mediastate_refr();
    return 0;
}

static void win_media_msg_callback(void* arg1)
{
    uint32_t msg=(uint32_t)arg1;
    char *uri=NULL;
    if(msg==HCPLAYER_MSG_STATE_EOS||msg==HCPLAYER_MSG_STATE_TRICK_EOS){
        if(m_cur_media_hld->loop_type==PlAY_LIST_SEQUENCE){
            uri=win_media_get_next_file(app_get_playlist_t()); 
        }else if(m_cur_media_hld->loop_type==PLAY_LIST_ONE){
            uri=m_cur_media_hld->play_name;
        }
    }else{
        uri = win_media_get_next_file(app_get_playlist_t()); 
    }
    if (uri){
        media_stop(m_cur_media_hld);
        media_play(m_cur_media_hld, uri);
    }
    /* something objs need to update and refr when hcplayer msg get
     * */
    win_ctrlbar_ui_update();
    win_playlist_ui_update(V_KEY_DOWN, NULL);
    return ;
}

static int media_msg_handle(uint32_t msg_type)
{
    int  ret=0;
    uint32_t mp_msg_type=0;
    mp_msg_type=(msg_type>>16)&0xffff;
    uint16_t mp_msg_userdata=msg_type&0xffff;

    if(m_cur_media_hld->play_id!=mp_msg_userdata)
        return ret;
    switch (mp_msg_type)
    {
    case HCPLAYER_MSG_OPEN_FILE_FAILED:
    case HCPLAYER_MSG_ERR_UNDEFINED:
    case HCPLAYER_MSG_UNSUPPORT_FORMAT:
        media_play_id_update(m_cur_media_hld);
        win_msgbox_msg_open(STR_FILE_FAIL, 2000, win_media_msg_callback,(void*)mp_msg_type);
        if(lv_obj_is_valid(subwin_mpinfo)){
            lv_event_send(subwin_mpinfo,LV_EVENT_REFRESH,NULL);
            //updata playinfo obj when it open file
        }
        partition_info_t*  p_info=mmp_get_partition_info();
        api_storage_devinfo_check(p_info->used_dev,m_cur_media_hld->play_name);
        break;
    case HCPLAYER_MSG_STATE_READY:
        if(m_cur_file_list->media_type == MEDIA_TYPE_PHOTO){
            //gif file do not has effect, so disable the effect button
            if (isGifFile(media_get_cur_play_file(m_cur_media_hld))){
                lv_obj_add_state(ctrlbarbtn[PHOTO_EFFECT_ID],LV_STATE_DISABLED);
                lv_obj_t* focused_obj=lv_group_get_focused(lv_group_get_default());
                if(focused_obj==ctrlbarbtn[PHOTO_EFFECT_ID])
                    lv_group_focus_next(lv_group_get_default());
            }else{
                lv_obj_clear_state(ctrlbarbtn[PHOTO_EFFECT_ID],LV_STATE_DISABLED);
            }
        }
        break;   
    case HCPLAYER_MSG_STATE_PLAYING:
        if(lv_obj_is_valid(subwin_mpinfo)){
            lv_event_send(subwin_mpinfo,LV_EVENT_REFRESH,NULL);
        }
        break;   
    case HCPLAYER_MSG_STATE_EOS :
    #ifdef HC_MEDIA_MEMMORY_PLAY
		if(m_cur_file_list->media_type != MEDIA_TYPE_PHOTO)//play end save cur time set 0
		{
			save_cur_play_info_to_disk(0);
			//memset(save_node.save_name,0,512);
		}
    #endif
        win_media_msg_callback((void*)mp_msg_type);
        break;
    case HCPLAYER_MSG_STATE_TRICK_EOS: 
        win_media_msg_callback((void*)mp_msg_type);
        break;
    case HCPLAYER_MSG_STATE_TRICK_BOS:
        media_resume(m_cur_media_hld);
        win_ctrlbar_ui_update();
        if(m_cur_media_hld->type==MEDIA_TYPE_MUSIC){
            media_manual_seek_end(m_cur_media_hld);
        }
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO:
        if(m_cur_media_hld->type==MEDIA_TYPE_VIDEO){
            win_msgbox_msg_open(STR_AUDIO_USPT, 2000, NULL, NULL); 
        }else if(m_cur_media_hld->type==MEDIA_TYPE_MUSIC){
            media_play_id_update(m_cur_media_hld);
            win_msgbox_msg_open(STR_AUDIO_USPT, 2000, win_media_msg_callback,(void*)mp_msg_type); 
        }
        break;
    case HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO:
        if(m_cur_media_hld->type==MEDIA_TYPE_VIDEO){
            media_play_id_update(m_cur_media_hld);
            win_msgbox_msg_open(STR_VIDEO_USPT, 2000, win_media_msg_callback,(void*)mp_msg_type);
        }
        break;
    case HCPLAYER_MSG_AUDIO_DECODE_ERR:
        switch (m_cur_media_hld->type){
            case MEDIA_TYPE_VIDEO:
            	win_msgbox_msg_open(STR_AUDIO_ERROR, 2000, NULL, NULL);
                break;
            case MEDIA_TYPE_MUSIC:
                media_play_id_update(m_cur_media_hld);
                win_msgbox_msg_open(STR_AUDIO_ERROR, 2000, win_media_msg_callback,(void*)mp_msg_type);
                break;
            default :
                break;
        }
        break;
    case HCPLAYER_MSG_VIDEO_DECODE_ERR:
        media_play_id_update(m_cur_media_hld);
        switch (m_cur_media_hld->type){
            case MEDIA_TYPE_VIDEO:
                win_msgbox_msg_open(STR_VIDEO_ERROR, 2000,  win_media_msg_callback,(void*)mp_msg_type);
                break;
            case MEDIA_TYPE_PHOTO:
                win_msgbox_msg_open(STR_PIC_ERROR, 2000,  win_media_msg_callback,(void*)mp_msg_type);
            default :
                break;
        }
        break;
    default:
        break;
    }
    return ret;
}

static int media_msg_remote(uint32_t msg_type)
{
	switch (msg_type)
	{
		//case 
		#ifdef HC_MEDIA_MEMMORY_PLAY
		case MSG_TYPE_MP_MEMMORY_SAVE_PLAY_TIME:
	    	save_cur_play_info_to_disk(media_get_playtime(m_cur_media_hld));
			break;
		#endif
		case MSG_TYPE_BAR_NO_FILE_MSG:
			//
			if(m_cur_file_list->media_type == MEDIA_TYPE_TXT)
				_ui_screen_change(ui_fspage,0,0);
			else
				_ui_screen_change(ui_fspage,0,0);
	        set_keystone_disable(false);
			break;
		case MSG_TYPE_KEY_NUM:
            if (!m_cur_media_hld && lv_scr_act()!=ui_ebook_txt)
                return 0;

			if(get_key_num() > 0 && lv_scr_act()==ui_ebook_txt && m_cur_file_list->media_type == MEDIA_TYPE_TXT)
			{
				change_ebook_txt_info(LV_KEY_ENTER,get_key_num());
			}
			break;
	
		default:
			break;
	}
    return 0;
}
static int win_backstage_music_reset(int argc, void* argv)
{    
    char * msg_code = (char*) argv;
    if(argc == MSG_TYPE_USB_UNMOUNT || argc == MSG_TYPE_USB_UNMOUNT_FAIL||
       argc == MSG_TYPE_SD_UNMOUNT || argc == MSG_TYPE_SD_UNMOUNT_FAIL){
        media_handle_t* media_handle = (media_handle_t*)backstage_media_handle_get();
        if(media_handle && strstr(media_handle->play_name,msg_code)){
            backstage_player_task_stop(0 ,NULL);
            /* rerf ui->obj when stop backstage media */
            if(lv_scr_act() == ui_ctrl_bar){
                lv_obj_set_style_bg_img_src(ctrlbarbtn[12],&IDB_Hint_Music_Off,0);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[12],0),STR_MUSIC_ON,FONT_MID);
            }else if(lv_scr_act() == ui_ebook_txt){
                lv_obj_set_style_bg_img_src(ctrlbarbtn[5],&IDB_Hint_Music_Off,0);
                set_label_text2(lv_obj_get_child(ctrlbarbtn[5],0),STR_MUSIC_ON,FONT_MID); 
            }
        }        
    } 
    return 0;
}

static int media_hotplug_msg_handle(void* msg)
{
    control_msg_t * ctl_msg = (control_msg_t*)msg;
    win_music_recources_ui_update(ctl_msg->msg_type, (void*)ctl_msg->msg_code);
    win_backstage_music_reset(ctl_msg->msg_type, (void*)ctl_msg->msg_code);
    return 0;
}


void media_playbar_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
	if(ctl_msg->msg_type == MSG_TYPE_MSG){
	    media_msg_handle(ctl_msg->msg_code);
	}else if(ctl_msg->msg_type== MSG_TYPE_CMD){
	    spectrum_uimsg_handle(ctl_msg->msg_code);
	}else if(ctl_msg->msg_type == MSG_TYPE_REMOTE){
		media_msg_remote(ctl_msg->msg_code);
	}else if(ctl_msg->msg_type == MSG_TYPE_USB_MOUNT 
         ||ctl_msg->msg_type == MSG_TYPE_USB_UNMOUNT
         ||ctl_msg->msg_type == MSG_TYPE_USB_UNMOUNT_FAIL
         ||ctl_msg->msg_type == MSG_TYPE_SD_MOUNT
         ||ctl_msg->msg_type == MSG_TYPE_SD_UNMOUNT
         ||ctl_msg->msg_type == MSG_TYPE_SD_UNMOUNT_FAIL){
        media_hotplug_msg_handle(ctl_msg);   
    }
}


// for anyother key reset the backlight
int ctrlbar_reset_mpbacklight(void)
{
    if(m_cur_media_hld==NULL){
        return -1;
    }else if(m_cur_media_hld->type==MEDIA_TYPE_MUSIC){
        if(blacklight_val==true){
            app_set_blacklight(blacklight_val);
            printf("backlight:%d\n",blacklight_val);
            blacklight_val=!blacklight_val;
        }
    }
    return 0;
}
/*
 for ui ctlbar show media_player state ,just refresh obj "ui_playstate" "ui_speed"
*/ 
int Ctrlbar_mediastate_refr(void)
{
    if(m_cur_media_hld!=NULL){
        switch(m_cur_media_hld->state){
            case MEDIA_PLAY:
                set_label_text2(ui_playstate,STR_PLAY,FONT_MID);        
                break;
            case MEDIA_PAUSE:
                set_label_text2(ui_playstate,STR_PAUSE,FONT_MID);        
                break;
            case MEDIA_FB:
                set_label_text2(ui_playstate,STR_FB,FONT_MID);        
                break;
            case MEDIA_FF:
                set_label_text2(ui_playstate,STR_FF,FONT_MID);        
                break;
            case MEDIA_SB:
                break;
            case MEDIA_SF:
                set_label_text2(ui_playstate,STR_SF,FONT_MID);                  
                break;
            default:
                break;
        }
    } 
    ctrlbar_reflesh_speed();       
    return 0;
}

/**
 * @description: Set image_effect param in global var mpimage_effect
 * @param {int} type ,effect_type
 */
static void mpimage_effect_param_set(int type)
{
    switch (type){
        case EFFECT_NULL:
            memset(&mpimage_effect.param,0,sizeof(image_effect_t));
            break;
        case EFFECT_SHUTTERS:
            mpimage_effect.param.mode=IMG_SHOW_SHUTTERS;
            mpimage_effect.param.mode_param.shuttles_param.time=50; 
            mpimage_effect.param.mode_param.shuttles_param.direction= 0; 
            mpimage_effect.param.mode_param.shuttles_param.type=0;
            break;
        case EFFECT_BRUSH:
            mpimage_effect.param.mode=IMG_SHOW_BRUSH;
            mpimage_effect.param.mode_param.brush_param.time=5; 
            mpimage_effect.param.mode_param.brush_param.direction= 0; 
            mpimage_effect.param.mode_param.brush_param.type=0;            
            break;
        case EFFECT_SLIDE:
            mpimage_effect.param.mode=IMG_SHOW_SLIDE;
            mpimage_effect.param.mode_param.slide_param.time=5; 
            mpimage_effect.param.mode_param.slide_param.direction= 0;
            mpimage_effect.param.mode_param.slide_param.type=0;            
            break;
        case EFFECT_MOSAIC:
            mpimage_effect.param.mode=IMG_SHOW_RANDOM;
            mpimage_effect.param.mode_param.random_param.time=3;  
            mpimage_effect.param.mode_param.random_param.type=0;            
            break;
        case EFFECT_FADE:
            mpimage_effect.param.mode=IMG_SHOW_FADE;
            mpimage_effect.param.mode_param.fade_param.time=1;  
            mpimage_effect.param.mode_param.fade_param.type=0;
            break;
        case EFFECT_RANDOM:
            /*Set the above EFFECT case param when set EFFECT_RANDOM */ 
            break;
        default :
            break;
    }
}

/**
 * @description: Get image_effect param in global var mpimage_effect
 * @return {*} 
 */
image_effect_t* mpimage_effect_param_get(void)
{
    static int type_index=EFFECT_SHUTTERS;
    /*Set the above EFFECT case when set EFFECT_RANDOM */ 
    if(mpimage_effect.type==EFFECT_RANDOM){
        type_index=type_index<EFFECT_RANDOM?type_index+1:EFFECT_SHUTTERS;
        mpimage_effect_param_set(type_index);
    }
    return &mpimage_effect.param;
}

void* mpimage_effect_info_get(void)
{
    return &mpimage_effect;
}

/* when do sys scale disable the zoom- function btn */
int ctrlbar_zoomfunction_btn_update(void)
{
    int zoom_out_count = projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT);
    int btn_id = 0;
    if(m_cur_media_hld->type == MEDIA_TYPE_VIDEO){
        btn_id = VIDEO_ZOOMOUT_ID;
    }else if(m_cur_media_hld->type == MEDIA_TYPE_PHOTO){
        btn_id = PHOTO_ZOOMOUT_ID; 
    }else{
        return 0;
    }

    if(zoom_out_count > 0){
        if(!lv_obj_has_state(ctrlbarbtn[btn_id],LV_STATE_DISABLED)){
            lv_obj_add_state(ctrlbarbtn[btn_id],LV_STATE_DISABLED);
        }
    }else if(zoom_out_count == 0){
        if(lv_obj_has_state(ctrlbarbtn[btn_id],LV_STATE_DISABLED)){
            lv_obj_clear_state(ctrlbarbtn[btn_id],LV_STATE_DISABLED);
        }
    }
    /* when group focuse on disable btn ,event_core will not into event_handle
     * so do group foucuse another */
    if(lv_group_get_focused(lv_group_get_default()) == ctrlbarbtn[btn_id])
        lv_group_focus_prev(lv_group_get_default());
    return 0;
}

