/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-11-28 21:00:09
 */
#ifndef __MP_CTRLBARPAGEPAGE_H_
#define __MP_CTRLBARPAGEPAGE_H_

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"
#include "osd_com.h"
#include <ffplayer.h>
#include "file_mgr.h"
#include "app_config.h"
#include<stdio.h>
#include<math.h>
#ifdef __cplusplus
extern "C" {
#endif
#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define LEFT_SCROOLL_IDX 6
    #define RIGHT_SCROOLL_IDX 5 
#else
    #define LEFT_SCROOLL_IDX 7
    #define RIGHT_SCROOLL_IDX 6   
#endif

extern lv_group_t * main_group ;
extern lv_group_t * sub_group ;
extern lv_group_t * fs_group ;
extern lv_group_t * play_bar_group;
extern lv_indev_t * indev_keypad;
extern bool m_play_bar_show;
extern lv_timer_t * bar_show_timer;

#ifdef RTOS_SUBTITLE_SUPPORT
typedef enum subtitles_event_t_{

    SUBTITLES_EVENT_SHOW,
    SUBTITLES_EVENT_HIDDEN,
    SUBTITLES_EVENT_PAUSE,
    SUBTITLES_EVENT_RESUME,
    SUBTITLES_EVENT_CLOSE,
} subtitles_event_t;

typedef struct ext_subtitle{
    int ext_subs_count;
    char ** uris;
}ext_subtitle_t ;

ext_subtitle_t * ext_subtitle_data_get(void);
int ext_subtitles_init(file_list_t* src_list);
int ext_subtitle_deinit(void);

#endif

void ctrl_bar_keyinput_event_cb(lv_event_t *event);
void format_time(uint32_t time, char *time_fmt);
void sec_timer_cb(lv_timer_t * t);
void show_play_bar(bool show);
void bar_show_timer_cb(lv_timer_t * t);
void unsupport_win_timer_cb(lv_timer_t * t);
void create_subtitles_rect(void);

#ifdef RTOS_SUBTITLE_SUPPORT
void subtitles_event_send(int e, lv_subtitle_t *subtitle);
#endif

void del_subtitles_rect(void);
void show_subtitles( char *str);
void show_subtitles_pic(lv_img_dsc_t *dsc);
char* subtitles_str_remove_prefix(char* str);
void subtitles_str_get_text(char* str);
void subtitles_str_set_style(char *str);
void ctrlbarpage_open(void);
int  ctrlbarpage_close(bool force);
void ctrlbar_reflesh_speed(void);
static void __media_seek_proc(uint32_t key);
static int vkey_transfer_btn(uint32_t vkey);
image_effect_t*  get_img_effect_mode(void);
rotate_type_e get_img_rotate_type(void);

void hccast_media_reset_aspect_mode();
void media_player_close(void);
void media_player_open(void);

int ctrlbar_reset_mpbacklight(void);
uint16_t ctrl_bar_get_img_dis_str_id(void);

extern int mp_infowin_add_data(media_type_t media_type,mp_info_t mp_info);
#ifdef HC_MEDIA_MEMMORY_PLAY
void media_info_save_memmory(void *media_info, char *file_name,int type);
void save_cur_play_info_to_disk(int time);
#endif
image_effect_t* mpimage_effect_param_get(void);
void* mpimage_effect_info_get(void);
int ctrlbar_btn_enter(lv_obj_t* target);
int ctrlbar_zoomfunction_btn_update(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif



