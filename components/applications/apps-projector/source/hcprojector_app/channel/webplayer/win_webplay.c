#include "app_config.h"
#ifdef HCIPTV_YTB_SUPPORT
#include "lvgl/lvgl.h"
#include "win_webservice.h" 
#include "com_api.h"
#include "webdata_mgr.h"
#include "glist.h"
#include "screen.h"
#include "win_webplay.h"
#include "channel/local_mp/media_player.h"
#include "osd_com.h"
#include "key_event.h"
#include "os_api.h"
#include "ffplayer_manager.h"

lv_obj_t * webplay_scr=NULL;
lv_obj_t *cont_obj;
lv_obj_t * slider_obj;
lv_obj_t* img_play;
lv_obj_t *play_time_obj;
lv_obj_t *total_time_obj;
lv_obj_t * videoname;
static lv_timer_t*bar_show_timer=NULL;
static lv_timer_t* sec_counter_timer=NULL;

static int win_seeking = 0;
static int win_seek_position = 0;
static lv_timer_t *win_seek_timer = NULL;

media_handle_t* webplayer_hdl=NULL;
media_urls_t mp_urls={0};
// urls_data use to stroage double urls
#define URL_VIDEO_TOKEN     "v://"
#define URL_AUDIO_TOKEN     "a://"
#define URL_SINGLE_AVTOKEN  "av://"
#define URL_LIVE_AVTOKEN    "hls://"

char tmp_buf[2048]={0};
// tmp buf use to audio_urls 

static void win_webplay_msg_handle(void* arg1 ,void *arg2);
static void event_handler_contobj(lv_event_t* event);
int create_win_webplayer(lv_obj_t* p);
bool win_webplayer_action(videolink_ask_e vlink_ask);
void webvideo_urls_data_reload(void* arg1,void* arg2);
index_pos_e grid_list_data_is_endof_index(void);
void win_cont_videoname_updata_by_action(void);
/**
 * @description: select the audio urls and video urls form source urls data 
 * @param {hccast_iptv_info_req_st *} urls_data ,source urls data
 * @return {*}
 * @author: Yanisin
 */
media_urls_t  weburls_node_optional_select(hccast_iptv_info_req_st * urls_data)
{
    media_urls_t media_urls={0};
    memset(&media_urls,0,sizeof(media_urls_t));
    if(strstr(urls_data->url,URL_AUDIO_TOKEN)&&strstr(urls_data->url,URL_VIDEO_TOKEN)){
        media_urls.is_double_urls=true;
        char * v_tmp=strstr(urls_data->url,URL_VIDEO_TOKEN);
        media_urls.url1=v_tmp+strlen(URL_VIDEO_TOKEN);
        char * a_tmp=urls_data->url+strlen(URL_AUDIO_TOKEN);
        memset(tmp_buf,0,strlen(tmp_buf));
        strncpy(tmp_buf,a_tmp,strlen(a_tmp)-strlen(v_tmp));
        media_urls.url2=tmp_buf;
    }else{
        if(strstr(urls_data->url,URL_SINGLE_AVTOKEN)){
            media_urls.is_double_urls=false;
            media_urls.url1=urls_data->url+strlen(URL_SINGLE_AVTOKEN);
        }else if(strstr(urls_data->url,URL_LIVE_AVTOKEN)){
            media_urls.is_double_urls=false;
            media_urls.url1=urls_data->url+strlen(URL_LIVE_AVTOKEN);
        }
    }
    return media_urls;
}

void webplayer_handle_create(void)
{
    if(!webplayer_hdl){
        webplayer_hdl=media_open(MEDIA_TYPE_VIDEO);
        media_func_callback_register(webplayer_hdl,webvideo_urls_data_reload);
        /*register a func callback for player next/prev func*/
    }
    api_set_display_aspect(DIS_TV_16_9,DIS_PILLBOX);
    api_ffmpeg_player_get_regist(webplayer_ffmepg_player_get);

}
/**
 * @description: 网络视频播放接口，调用实现可播放视频
 * @return {*}
 * @author: Yanisin
*/
static int webplayer_open(void)
{
    // webplayer_handle_create();
    // api_set_display_aspect(DIS_TV_16_9,DIS_PILLBOX);
    hccast_iptv_info_req_st* urls_data=webvideo_url_data_get();
    mp_urls=weburls_node_optional_select(urls_data);
    if(mp_urls.url1!=NULL||mp_urls.url2!=NULL){
        if(mp_urls.is_double_urls==true){
            media_play_with_double_url(webplayer_hdl,&mp_urls);
            printf(">>! v_url: %s\n",mp_urls.url1);
            printf(">>! a_url: %s\n",mp_urls.url2);
        }else {
            media_play(webplayer_hdl,mp_urls.url1);
        }
    }
    win_seeking = 0;
    win_seek_position = 0;
    return 0;
}




int webplayer_stop(void)
{
    if(webplayer_hdl!=NULL){
        if(mp_urls.is_double_urls==true){
            media_stop_with_double_url(webplayer_hdl);
        }else{
            media_stop(webplayer_hdl);
        }
    }
    return 0;
}
/**
 * @description: 网络视频关闭接口，调用实现可关闭视频
 * @return {*}
 * @author: Yanisin
 */
static int webplayer_close(void)
{
    webplayer_stop();
    if(webplayer_hdl){
        media_close(webplayer_hdl);
        webplayer_hdl=NULL;
    }
    api_set_display_aspect(DIS_TV_AUTO,DIS_NORMAL_SCALE);
    api_ffmpeg_player_get_regist(NULL);
    // reset the de display mode which it press into 
    return 0;
}

static void win_seek_timer_cb(lv_timer_t *t)
{
    if (webplayer_hdl != NULL)
    {
        media_seek(webplayer_hdl, win_seek_position);
        media_resume(webplayer_hdl);
    }
    win_seeking = 0;
    win_seek_position = 0;
    lv_timer_pause(win_seek_timer);
    return;
}

/**
 * @description: LVGL UI operation ,to show something in webplayer screen
 */
lv_group_t* gp2=NULL;
void webplay_init(void* args)
{
    //lvgl something 
    gp2=lv_group_create();
    lv_group_set_default(gp2);
    lv_indev_set_group(indev_keypad, gp2);     // 键盘
    // lv_obj_t* obj=lv_obj_create(webplay_scr);
    create_win_webplayer(webplay_scr);

    win_data_buffing_open(webplay_scr);
    win_data_buffing_label_set("Loading...");
    win_seek_timer = lv_timer_create(win_seek_timer_cb, 1000, NULL);
    lv_timer_pause(win_seek_timer);
}
void webplay_deinit(void* args)
{
    lv_group_remove_all_objs(gp2);
    lv_group_del(gp2);
    lv_obj_clean(webplay_scr);
    if(win_data_buffing_is_open()){
        win_data_buffing_close();
    }
    win_msgbox_msg_close(); 
    lv_timer_del(bar_show_timer);
    lv_timer_del(sec_counter_timer);
    lv_timer_del(win_seek_timer);
}
/*hide cont obj after 3s,3s call again*/ 
static void bar_show_timer_cb(lv_timer_t * t)
{
    if(lv_obj_is_valid(cont_obj)){
        if(!lv_obj_has_flag(cont_obj,LV_OBJ_FLAG_HIDDEN)){
            lv_obj_add_flag(cont_obj,LV_OBJ_FLAG_HIDDEN);
        }
    }
    return;
}
/*show cont obj or hide cont obj immediately*/
static void win_contobj_show_or_hide(bool show)
{
    if(show){
        lv_timer_reset(bar_show_timer);
        // lv_timer_pause(bar_show_timer);
        if(lv_obj_has_flag(cont_obj,LV_OBJ_FLAG_HIDDEN)){
            lv_obj_clear_flag(cont_obj,LV_OBJ_FLAG_HIDDEN);
        }
    }else{
        // hide cont obj immediately if it is show 
        lv_timer_reset(bar_show_timer);
        lv_timer_resume(bar_show_timer);
        if(!lv_obj_has_flag(cont_obj,LV_OBJ_FLAG_HIDDEN)){
            lv_obj_add_flag(cont_obj,LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void format_time(uint32_t time, char *time_fmt)
{
    uint32_t hour;
    uint32_t min;
    uint32_t second;
    uint32_t hours_24 = 24*3600;

    if (0 == time || time > hours_24){
        sprintf(time_fmt, "00:00:00");
        return;
    }

    hour = time / 3600;
    min = (time % 3600) / 60;
    second = time % 60;
    if (hour > 0)
        sprintf(time_fmt, "%02d:%02d:%02d", (int)hour, (int)min, (int)second);
    else
        sprintf(time_fmt, "%02d:%02d", (int)min, (int)second);

}
static void sec_timer_cb(lv_timer_t * t)
{
    uint32_t play_time = 0;
    uint32_t total_time = 0;
    char time_fmt[16];

    if(cont_obj==NULL)
        return;

    if (win_seeking)
    {
        play_time = win_seek_position;
    }
    else if(webplayer_hdl!=NULL&&webplayer_hdl->player!=NULL)
    {
        play_time = media_get_playtime(webplayer_hdl);
    }

    format_time(play_time, time_fmt);
    lv_label_set_text(play_time_obj, time_fmt);
    lv_slider_set_value(slider_obj, play_time, LV_ANIM_ON);
    if(webplayer_hdl!=NULL&&webplayer_hdl->player!=NULL)
        total_time = media_get_totaltime(webplayer_hdl);
    format_time(total_time, time_fmt);
    lv_label_set_text(total_time_obj, time_fmt);
    if (total_time > 0)
        lv_slider_set_range(slider_obj, 0, total_time);

    return;
}


#define INTERVAL_JUMP_MAX  (300) //300 s
#define INTERVAL_JUMP_STEP  (10) //10 s

static void win_webplay_key_seek_do(uint32_t vkey)
{
    uint32_t play_time = 0;
    uint32_t seek_time = 0;
    uint32_t jump_interval = 0;
    uint32_t total_time = media_get_totaltime(webplayer_hdl);

    if (total_time < jump_interval)
        return;

    if (win_seeking)
    {
        play_time = win_seek_position;
    }
    else
    {
        play_time = media_get_playtime(webplayer_hdl);
    }

    int64_t cur_op_time = api_get_sys_clock_time();
    if ((cur_op_time - webplayer_hdl->last_seek_op_time) < 800) {
        webplayer_hdl->jump_interval += INTERVAL_JUMP_STEP;
    } else {
        webplayer_hdl->jump_interval = INTERVAL_JUMP_STEP;
    }

    webplayer_hdl->last_seek_op_time = cur_op_time;

    if (webplayer_hdl->jump_interval < INTERVAL_JUMP_MAX)
        jump_interval = webplayer_hdl->jump_interval;
    else
        jump_interval = INTERVAL_JUMP_MAX;

    if (V_KEY_LEFT == vkey){//seek backward
        if (play_time > jump_interval)
            seek_time = play_time - jump_interval;
        else
            seek_time = 0;
    }else if(V_KEY_RIGHT==vkey)
    { //seek forward
        if ((play_time + jump_interval) > total_time)
            seek_time = total_time;
        else
            seek_time = play_time + jump_interval;
    }
    win_seeking = 1;
    win_seek_position = seek_time;
    media_pause(webplayer_hdl);
    lv_timer_reset(win_seek_timer);
    lv_timer_resume(win_seek_timer);
}

static void win_webplayer_pause(void)
{
    int media_status = -1; 
    if(webplayer_hdl==NULL||webplayer_hdl->player==NULL)
        return ;
    media_status = media_get_state(webplayer_hdl);
    if (MEDIA_PAUSE == media_status){
        media_resume(webplayer_hdl);
        lv_img_set_src(img_play, &img_lv_demo_music_btn_pause);
    } else if (MEDIA_PLAY == media_status){
        media_pause(webplayer_hdl);
        lv_img_set_src(img_play, &img_lv_demo_music_btn_play);
    }    

}
int create_win_webplayer(lv_obj_t* p)
{
    cont_obj = lv_obj_create(p);
    lv_obj_clear_flag(cont_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(cont_obj, PLAYER_BAR_X, PLAYER_BAR_Y);
    lv_obj_set_size(cont_obj, PLAYER_BAR_W, PLAYER_BAR_H);
    lv_obj_set_style_bg_color(cont_obj, COLOR_DEEP_GREY, 0); //grey

    lv_group_add_obj(lv_group_get_default(),cont_obj);
    lv_obj_add_event_cb(cont_obj,event_handler_contobj,LV_EVENT_ALL,0);

    slider_obj = lv_slider_create(cont_obj);
    lv_obj_set_style_anim_time(slider_obj, 100, 0);
    lv_obj_add_flag(slider_obj, LV_OBJ_FLAG_CLICKABLE); /*No input from the slider*/
    lv_obj_set_size(slider_obj, SLIDE_W, SLIDE_H);
    lv_obj_align(slider_obj, LV_ALIGN_CENTER, 0, 42);

    lv_obj_set_height(slider_obj, 6);
    lv_obj_set_grid_cell(slider_obj, LV_GRID_ALIGN_STRETCH, 1, 4, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_style_bg_img_src(slider_obj, &img_lv_demo_music_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider_obj, 30, LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(slider_obj, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_obj, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(slider_obj, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(slider_obj, 0, 0);

    //play/pause icon
    img_play = lv_img_create(cont_obj);
    lv_img_set_src(img_play, &img_lv_demo_music_btn_pause);
    lv_obj_align(img_play, LV_ALIGN_CENTER, 0, -36);


    videoname=lv_label_create(cont_obj);
    webvideo_list_t * webvideo_list=webvideo_list_data_get();
    hccast_iptv_info_node_st *video_node=(hccast_iptv_info_node_st *)list_nth_data(&webvideo_list->info_list->list,webvideo_list->list_idx-webvideo_list->offset);
    lv_obj_set_style_text_font(videoname, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(videoname, lv_color_hex(0x8a86b8), 0);
    lv_obj_align(videoname,LV_ALIGN_TOP_LEFT,3,8);
    lv_obj_set_size(videoname,lv_pct(45),30);
    lv_label_set_long_mode(videoname,LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(videoname,video_node->title);

    //create play time text
    play_time_obj = lv_label_create(cont_obj);
    lv_obj_set_style_text_font(play_time_obj, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(play_time_obj, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(play_time_obj, "00:00:00");
    lv_obj_set_pos(play_time_obj, 10, SLIDE_Y-4);

    //create total time text
    total_time_obj = lv_label_create(cont_obj);
    lv_obj_set_style_text_font(total_time_obj, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(total_time_obj, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(total_time_obj, "00:00:00");
    lv_obj_set_pos(total_time_obj, SLIDE_X+SLIDE_W+6, SLIDE_Y-4);

    lv_group_focus_obj(cont_obj);
    bar_show_timer = lv_timer_create(bar_show_timer_cb, 4000, NULL);
    sec_counter_timer = lv_timer_create(sec_timer_cb, 1000, NULL);
    return 0;

}

static void screen_event_cb(lv_event_t* event){
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target = lv_event_get_target(event);
    if(code==LV_EVENT_SCREEN_LOADED){  
        webplay_init(NULL);
    }else if(code ==LV_EVENT_SCREEN_UNLOADED){
        if(lv_scr_act()==setup_scr||lv_scr_act()==channel_scr||lv_scr_act()==main_page_scr){
            //hccast_iptv_handle_abort(hciptv_handle_get());
            /*stop webs apis if it is not ready,but this will abort next time*/ 
            if(win_data_buffing_is_open()){
                win_data_buffing_close();
            }
            webplayer_close();
            webservice_pthread_delete();
            webservice_thumb_buffer_free();
            /*del web pthread if it turn into channel or setup*/
        }
        webplay_deinit(NULL);
    }
    return;
}
void webplay_screen_init(void)
{
    webplay_scr=lv_obj_create(NULL);
    lv_obj_set_size(webplay_scr,LV_PCT(100),LV_PCT(100));
    lv_obj_add_event_cb(webplay_scr,screen_event_cb,LV_EVENT_ALL,0);
    lv_obj_set_style_bg_opa(webplay_scr,LV_OPA_TRANSP,0);

    screen_entry_t webplay_entry;
    webplay_entry.screen = webplay_scr;
    webplay_entry.control = win_webplay_msg_handle;
    api_screen_regist_ctrl_handle(&webplay_entry);

}


static void event_handler_contobj(lv_event_t* event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target = lv_event_get_target(event);
    uint32_t msgid=webservice_pthread_msgid_get();
    webservice_msg_t msg = {0};
    if(code==LV_EVENT_KEY){
        lv_indev_t *key_indev = lv_indev_get_act();
        uint32_t lv_key = lv_indev_get_key(lv_indev_get_act());
        uint32_t user_vkey=key_convert_vkey(lv_key);
        index_pos_e index_pos=0;
        if (key_indev->proc.state == LV_INDEV_STATE_PRESSED){
            if(lv_obj_has_flag(target,LV_OBJ_FLAG_HIDDEN)){
                // context obj is hidden
                if(user_vkey==V_KEY_EXIT){
                    hccast_iptv_handle_abort(hciptv_handle_get());
                    /*stop webs apis if it is not ready,but this will abort next time*/ 
                    webplayer_close();
                    change_screen(SCREEN_WEBSERVICE);
                    return;
                }
                win_contobj_show_or_hide(true);
            }else{
                // context obj is show
                switch(user_vkey){
                    case V_KEY_MENU:
                        break;
                    case V_KEY_PLAY:
                        lv_img_set_src(img_play, &img_lv_demo_music_btn_pause);
                        win_contobj_show_or_hide(true);
                        if(webplayer_hdl!=NULL&&webplayer_hdl->state==MEDIA_PAUSE){
                            media_resume(webplayer_hdl);
                        }
                        break;
                    case V_KEY_PAUSE:
                        lv_img_set_src(img_play, &img_lv_demo_music_btn_play);
                        win_contobj_show_or_hide(true);
                        if(webplayer_hdl!=NULL&&webplayer_hdl->state==MEDIA_PLAY){
                            media_pause(webplayer_hdl);
                        }
                        break;
                    case V_KEY_LEFT: //for seek
                    case V_KEY_RIGHT: //for seek
                        win_contobj_show_or_hide(true);
                        if(webplayer_hdl!=NULL){
                            if(webplayer_hdl->state==MEDIA_PLAY||webplayer_hdl->state==MEDIA_PAUSE)
                                win_webplay_key_seek_do(user_vkey);
                        }
                        break;
                    case V_KEY_ENTER:
                        win_webplayer_pause();
                        win_contobj_show_or_hide(true);
                        break;
                    case V_KEY_UP:
                        index_pos=grid_list_data_is_endof_index();
                        if(index_pos==FIRST_POS){
                            win_msgbox_msg_open(STR_FIRST_VIDEO,WIN_WEB_MSG_TIMEOUT,0,0);
                            break;
                        }
                        if(webplayer_hdl->state==MEDIA_STOP){
                            // respond to key presses when getting a link
                            // clear the win msg and win loading and show a hint
                            win_msgbox_msg_close();
                            if(win_data_buffing_is_open())
                                win_data_buffing_close();
                            hccast_iptv_handle_abort(hciptv_handle_get());
                        }else{
                            webplayer_stop();
                        }
                        win_data_buffing_open(lv_scr_act());
                        win_data_buffing_label_set(" Loading \n Prev...");
                        if(index_pos==PAGE_FIRST_POS){
                            msg.type=MSG_VIDEOLIST_PREVPAGE_ACK;
                            msg.code=NULL;
                            api_message_send(msgid,&msg,sizeof(webservice_msg_t));
                            break;
                        }
                        win_webplayer_action(PREV_LINK_ASK);
                        break;
                    case V_KEY_DOWN:
                        index_pos=grid_list_data_is_endof_index();
                        if(index_pos==LAST_POS){
                            win_msgbox_msg_open(STR_LAST_VIDEO,WIN_WEB_MSG_TIMEOUT,0,0);
                            break;
                        }
                        if(webplayer_hdl->state==MEDIA_STOP){
                            win_msgbox_msg_close();
                            if(win_data_buffing_is_open())
                                win_data_buffing_close();
                            hccast_iptv_handle_abort(hciptv_handle_get());
                        }else{
                            webplayer_stop();
                        }
                        win_data_buffing_open(lv_scr_act());
                        win_data_buffing_label_set(" Loading \n Next...");
                        if(index_pos==PAGE_LAST_POS){
                            msg.type=MSG_VIDEOLIST_NEXTPAGE_ACK;
                            msg.code=NULL;
                            api_message_send(msgid,&msg,sizeof(webservice_msg_t));
                            break;
                        }
                        win_webplayer_action(NEXT_LINK_ASK);
                        break;
                    case V_KEY_EXIT:
                        win_contobj_show_or_hide(false);
                        break;
                }
            }
        }
        return ;
    }
}
static void win_webplayer_action_after_timer(void *user_data)
{
    webservice_msg_t msg = {0};
    uint32_t msgid=webservice_pthread_msgid_get();
    index_pos_e index_pos=grid_list_data_is_endof_index();
    win_data_buffing_open(lv_scr_act());
    win_data_buffing_label_set(" Loading...");
    if(strstr(user_data,"next")){
        if(index_pos==PAGE_LAST_POS){
            msg.type=MSG_VIDEOLIST_NEXTPAGE_ACK;
            msg.code=NULL;
            api_message_send(msgid,&msg,sizeof(webservice_msg_t));
            return;
        }
        win_webplayer_action(NEXT_LINK_ASK);
    }else if(strstr(user_data,"prev")){
        if(index_pos==PAGE_FIRST_POS){
            msg.type=MSG_VIDEOLIST_PREVPAGE_ACK;
            msg.code=NULL;
            api_message_send(msgid,&msg,sizeof(webservice_msg_t));
            return;
        }
        win_webplayer_action(PREV_LINK_ASK);
    }
    return;
}
static void win_webplay_exit_after_timer(void * user_data){
    webplayer_close();
    change_screen(SCREEN_WEBSERVICE);
    return ;
}

static void win_webplay_msg_handle(void* arg1 ,void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    uint32_t msg_code=ctl_msg->msg_code;
    if(ctl_msg->msg_type==MSG_TYPE_WEBSERVICE_LOADED){
        printf("into win_webplay msg handle type:%d code:%ld\n",ctl_msg->msg_type,msg_code);
        if(msg_code==WEBVIDEO_NEXTPAGETOKEN_SUCCESS){
            win_webplayer_action(NEXT_LINK_ASK);
            return;
        }else if(msg_code==WEBVIDEO_PREVPAGETOKEN_SUCCESS){
            win_webplayer_action(PREV_LINK_ASK);
            return;
        }else if(msg_code == WEBVIDEO_NEXTPAGETOKEN_ERROR || msg_code == WEBVIDEO_PREVPAGETOKEN_ERROR){
            win_msgbox_msg_open(STR_VIDEO_INFO_ERR_RETRY_AGAIN, WIN_WEB_MSG_TIMEOUT, NULL, NULL);
        }
        if(msg_code==WEBVIDEO_LINK_GET_SUCCESS){
            webplayer_open();
        }else if(msg_code==WEBVIDEO_LINK_GET_ABORT){
            /*nothing to do*/
        }else if(msg_code==WEBVIDEO_LINK_GET_ERROR){
            if(grid_list_data_is_endof_index() == LAST_POS){ // The last one exits directly, otherwise the next one
                win_msgbox_msg_open(STR_VIDEO_LINK_ERR,WIN_WEB_MSG_TIMEOUT,win_webplay_exit_after_timer,NULL);
            }else{
                win_msgbox_msg_open(STR_VIDEO_LINK_ERR_PLAY_NEXT,WIN_WEB_MSG_TIMEOUT,win_webplayer_action_after_timer,"next");
            }
        }else if(msg_code==WEBVIDEO_NEXT_LINK_GET_SUCCESS){
            media_play_next_program(webplayer_hdl);
            win_cont_videoname_updata_by_action();
        }else if(msg_code==WEBVIDEO_PREV_LINK_GET_SUCCESS){
            media_play_prev_program(webplayer_hdl);
            win_cont_videoname_updata_by_action();
        }else if(msg_code==WEBVIDEO_NEXT_LINK_GET_ERROR){
            /* need to show a hint : net work errror /play fail   loading next*/ 
            if(grid_list_data_is_endof_index()){
                // end of the video ,close close  
                win_msgbox_msg_open(STR_VIDEO_LINK_ERR,WIN_WEB_MSG_TIMEOUT,win_webplay_exit_after_timer,NULL); //sec timer to exit 
            }else {
                win_msgbox_msg_open(STR_VIDEO_LINK_ERR_PLAY_NEXT,WIN_WEB_MSG_TIMEOUT,win_webplayer_action_after_timer,"next"); //sec time to do next
                //sec timer to show loading and  do next
            }
        }else if(msg_code==WEBVIDEO_PREV_LINK_GET_ERROR){
            if(grid_list_data_is_endof_index()){
                win_msgbox_msg_open(STR_VIDEO_LINK_ERR,WIN_WEB_MSG_TIMEOUT,win_webplay_exit_after_timer,NULL); //sec timer to exit 
            }else{
                win_msgbox_msg_open(STR_VIDEO_LINK_ERR_PLAY_PREV,WIN_WEB_MSG_TIMEOUT,win_webplayer_action_after_timer,"prev");// sec time to do next
            }

        }
    }else if(ctl_msg->msg_type==MSG_TYPE_MSG){
        msg_code=(msg_code>>16)&0xffff;
        printf("into win_webplay msg handle type:%d code:%ld\n",ctl_msg->msg_type,msg_code);
        if(msg_code&HCPLAYER_MSG_BUFFING_FLAG){ // msg_code >= 4096
            int buff_val=msg_code&0xff; 
            if(win_data_buffing_is_open()){
                win_data_buffing_update(buff_val);
            }else {
                win_data_buffing_open(lv_scr_act());
            }
        }else if(msg_code==HCPLAYER_MSG_OPEN_FILE_FAILED){ // msg_code = 9
            if(win_data_buffing_is_open()){
                win_data_buffing_close();
            }
            // to do open fail to play next    
            index_pos_e index_pos=grid_list_data_is_endof_index();
            if(index_pos==LAST_POS){
                win_msgbox_msg_open(STR_VIDEO_PLAY_FAIL,WIN_WEB_MSG_TIMEOUT,win_webplay_exit_after_timer,NULL); //sec timer to do eixt
            }else{
                win_msgbox_msg_open(STR_VIDEO_PLAY_FAIL_PLAY_NEXT,WIN_WEB_MSG_TIMEOUT,win_webplayer_action_after_timer,"next"); //sec time to do next
                //sec timer to show loading and  do next
            }
        }else if(msg_code==HCPLAYER_MSG_STATE_EOS||msg_code==HCPLAYER_MSG_STATE_TRICK_EOS){ // msg_code = 5 || 6 play end
            index_pos_e index_pos=grid_list_data_is_endof_index();
            if(index_pos==LAST_POS){
                win_msgbox_msg_open(STR_LAST_VIDEO,WIN_WEB_MSG_TIMEOUT,win_webplay_exit_after_timer,NULL); //sec timer to do eixt
            }else{
                win_webplayer_action_after_timer("next");
            }
        }else if(msg_code==HCPLAYER_MSG_HTTP_FORBIDDEN){ // msg_code = 25
            //https urls is Timeout,flush it and exit player
            hccast_iptv_handle_flush(hciptv_handle_get());
            win_webplay_exit_after_timer(NULL);
        }else if(msg_code==HCPLAYER_MSG_READ_TIMEOUT){ // msg_code = 8
            win_msgbox_msg_open(STR_NETWORK_ERR, WIN_WEB_MSG_TIMEOUT, win_webplay_exit_after_timer, NULL);
        }
    }else if(ctl_msg->msg_type==MSG_TYPE_USB_WIFI_PLUGOUT){
        win_msgbox_msg_open(STR_WIFI_DISCONNECT, WIN_WEB_MSG_TIMEOUT, win_webplay_exit_after_timer, NULL);
    }else if(ctl_msg->msg_type==MSG_TYPE_NETWORK_WIFI_DISCONNECTED){
        win_msgbox_msg_open(STR_WIFI_DISCONNECT, WIN_WEB_MSG_TIMEOUT, win_webplay_exit_after_timer, NULL);
    }

    return;    
}
void* webplayer_handle_get()
{
    return webplayer_hdl;
}
void* webplayer_ffmepg_player_get()
{
    if(webplayer_hdl)
    {
        if(webplayer_hdl->is_double_urls)
            return hcplayer_multi_get_vplayer(webplayer_hdl->player);
        else
            return webplayer_hdl->player;
    }
    else
        return NULL;
}


// 获取urls的操作方法,diff app has diff urls data get 
void webvideo_urls_data_reload(void* arg1,void* arg2)
{
    // media optation in msg handle 
    media_urls_t* nextorprev_urls=(media_urls_t* )arg2;
    hccast_iptv_info_req_st* urls_data=webvideo_url_data_get();
    memset(&mp_urls,0,sizeof(media_urls_t));
    mp_urls=weburls_node_optional_select(urls_data);
    // nextorprev_urls=&mp_urls;
    memcpy(nextorprev_urls,&mp_urls,sizeof(media_urls_t));
    // debug here 
}

videolink_ask_e mp_vlink_ask;
bool win_webplayer_action(videolink_ask_e vlink_ask)
{
    // just send a message 
    webservice_msg_t msg={0};
    grid_list_data_t* m_grid_list=grid_list_data_get();
    bool is_firstorlast=false;
    mp_vlink_ask=vlink_ask;
    msg.type=MSG_VIDEOLINK_ACK;
    msg.code=&mp_vlink_ask;
    uint32_t msg_id=webservice_pthread_msgid_get();
    if(m_grid_list->list_sel_idx>=0&&m_grid_list->list_sel_idx<m_grid_list->list_cnt){
        if(vlink_ask==NEXT_LINK_ASK){
            if(m_grid_list->list_sel_idx==m_grid_list->list_cnt-1){
                is_firstorlast=true;
            }else{
                if(m_grid_list->cur_pos == SUBCONT-1){
                    m_grid_list->page_idx++;
                    m_grid_list->top += m_grid_list->depth;
                }
                m_grid_list->list_sel_idx++;
                m_grid_list->cur_pos=m_grid_list->list_sel_idx-m_grid_list->top;
                api_message_send(msg_id,&msg,sizeof(webservice_msg_t));
            }
        }else if(vlink_ask==PREV_LINK_ASK){
            if(m_grid_list->list_sel_idx==0){
                is_firstorlast=true;
            }else{
                if(m_grid_list->cur_pos == 0){
                    m_grid_list->page_idx--;
                    m_grid_list->top -= m_grid_list->depth;
                }
                m_grid_list->list_sel_idx--;
                m_grid_list->cur_pos=m_grid_list->list_sel_idx-m_grid_list->top;
                api_message_send(msg_id,&msg,sizeof(webservice_msg_t));
            }
        }
    }
    return is_firstorlast;
}

index_pos_e grid_list_data_is_endof_index(void)
{
    index_pos_e flag=0;
    grid_list_data_t* m_grid_list=grid_list_data_get();
    if(m_grid_list->list_sel_idx==m_grid_list->list_cnt-1){
        flag=LAST_POS;
        printf(">>! the last video\n");
    }else if(m_grid_list->list_sel_idx==0){
        flag=FIRST_POS;
        printf(">>! the first video\n");
    }else if(((m_grid_list->list_sel_idx+1)%YTB_PAGE_CACHA_NUM)==0){
        flag=PAGE_LAST_POS;
        printf(">>! the page last video\n");
    }else if((m_grid_list->list_sel_idx%YTB_PAGE_CACHA_NUM)==0){
        flag=PAGE_FIRST_POS;
        printf(">>! the page first video\n");
    }
    return flag;
}

void win_cont_videoname_updata_by_action(void)
{
    webvideo_list_t * webvideo_list=webvideo_list_data_get();
    hccast_iptv_info_node_st *video_node=(hccast_iptv_info_node_st *)list_nth_data(&webvideo_list->info_list->list,webvideo_list->list_idx-webvideo_list->offset);
    lv_label_set_text(videoname,video_node->title);
}



#endif 
