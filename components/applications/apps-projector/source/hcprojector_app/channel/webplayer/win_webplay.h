/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2023-05-31 17:26:00
 */
#ifndef __WEBDATA_MGR_H
#define __WEBDATA_MRG_H
#include <stdio.h> //printf()
#include <stdlib.h>
#include <string.h> //memcpy()
#include <unistd.h> //usleep()
#include <stdbool.h> //bool
#include <stdint.h> //uint32_t
#include <time.h>

#define PLAYER_BAR_H    160
#define PLAYER_BAR_W    (OSD_MAX_WIDTH-80)
#define PLAYER_BAR_X    (OSD_MAX_WIDTH-PLAYER_BAR_W)/2
#define PLAYER_BAR_Y    (OSD_MAX_HEIGHT-PLAYER_BAR_H-20)

#define SLIDE_W    (PLAYER_BAR_W - 300)
#define SLIDE_H    50
#define SLIDE_X    (PLAYER_BAR_W - SLIDE_W)/2
#define SLIDE_Y    90
#define STR_BUFFER_ERR "Video Buffer Error"
#define WIN_WEB_MSG_TIMEOUT 3000

LV_IMG_DECLARE(img_lv_demo_music_slider_knob);
LV_IMG_DECLARE(img_lv_demo_music_btn_play);
LV_IMG_DECLARE(img_lv_demo_music_btn_pause);
void webplay_screen_init(void);
void webplayer_handle_create(void);
void* webplayer_handle_get();
void* webplayer_ffmepg_player_get();


typedef enum{
    MID_POS = 0,
    FIRST_POS,
    LAST_POS,
    PAGE_FIRST_POS,
    PAGE_LAST_POS,
}index_pos_e;



#endif