/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2023-03-24 20:27:23
 */
#ifndef __WIN_WEBSERVICE_H
#define __WIN_WEBSERVICE_H
#include <stdio.h> //printf()
#include <stdlib.h>
#include <string.h> //memcpy()
#include <unistd.h> //usleep()
#include <stdbool.h> //bool
#include <stdint.h> //uint32_t
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lvgl/lvgl.h"
#include <hccast/hccast_iptv.h>

#define BLACKCOLOR_LEVEL1  0x1e1e1e
#define BLACKCOLOR_LEVEL2  0x010208
#define REDLEVEL1 0xe83a1e

LV_IMG_DECLARE(Youtube_logo);
LV_IMG_DECLARE(thumbnail_default);
LV_IMG_DECLARE(MENU_IMG_LIST_PAGE);
#define STR_LOADING "Loading..."
#define YTB_LARGET_FONT  lv_font_montserrat_22
#define YTB_MID_FONT  lv_font_montserrat_16
#define YTB_MENU_FONT  lv_font_montserrat_26
#define PREBUILD_HISTORY_CATEGORY  "History"
#define PREBUILD_SERARCH_CATEGORY  "Search"
#define PREBUILD_TRENDING_CATEGORY "Trending"

#define SUBCONT 4
#define MAXTHUNB_DATASIZE 24*1024
#define THUMBNAIL_QUALITY "default"
#define Y2B_MENU_CNT 3
#define MAX_YTB_HISTORY_VIDEO_SAVE 20

/*A page returns 20 pieces of data, which must be a multiple of SUBCONT*/
#define YTB_PAGE_CACHA_NUM (SUBCONT*5) 

typedef enum{
    YTB_HISTORY_CATEGORY = 0,
    YTB_SERARCH_CATEGORY,
    YTB_TRENDING_CATEGORY,

    PREBUILD_CATE_CNT,
}ytb_user_category;

/* something catagroy has not get form source data,
so had to prebuild by user,like: Trending / Search */
/**
 * @description: data struct for lvgl grid list obj 
 */
typedef struct{
    uint32_t top;        	//列表在页面显示的起点
    uint32_t depth;      	//列表页面中共显示的个数 
    uint32_t cur_pos;       //列表当前聚焦的光标
    int32_t list_sel_idx;	//列表选中的索引
    uint32_t list_cnt; 		//列表要表示的总个数
    uint32_t page_idx;		//当前选中的光标所在页面数
    uint32_t page_cnt;		//列表页面中的总页面数
}grid_list_data_t;


typedef enum{
    WEBCATEGORY_LIST_GET_SUCCESS,
    WEBCATEGORY_LIST_GET_ERROR,
    WEBVIDEO_INFO_GET_SUCCESS,
    WEBVIDEO_INFO_GET_ERROR,
    WEBVIDEO_INFO_GET_ABORT,
    WEBVIDEO_LINK_GET_SUCCESS,
    WEBVIDEO_LINK_GET_ERROR,
    WEBVIDEO_NEXT_LINK_GET_SUCCESS,
    WEBVIDEO_PREV_LINK_GET_SUCCESS,
    WEBVIDEO_NEXT_LINK_GET_ERROR,
    WEBVIDEO_PREV_LINK_GET_ERROR,
    WEBVIDEO_LINK_GET_ABORT,
    THUMBNIAIL_DOWNLOAD_SUCCESS,
    WEBVIDEO_NEXTPAGETOKEN_SUCCESS,
    WEBVIDEO_NEXTPAGETOKEN_ERROR,
    WEBVIDEO_PREVPAGETOKEN_SUCCESS,
    WEBVIDEO_PREVPAGETOKEN_ERROR,
    WEBVIDEO_SEARCH_GET_SUCCESS,
    WEBVIDEO_SEARCH_GET_ERROR,
    WEBVIDEO_SEARCH_GET_ABORT,
}web_msgcode_e;


typedef enum {
    INVALID_REFRESH = 0,    
    MSG_VIDEOLIST_REFRESH,
    MSG_CAREGROYLIST_REFRESH ,
    MSG_VIDEOLINK_ACK,
    MSG_VIDEOLIST_NEXTPAGE_ACK,
    MSG_VIDEOLIST_PREVPAGE_ACK,
    MSG_VIDEOLIST_THUMB_REFRESH,
    MSG_VIDOE_SEARCH_ACK,
    MSG_HISTORY_VIDEOLIST_REFRESH,
}win_msgtype_e;

typedef enum{
    MSG_CODE_NEXTLINK,
    MSG_CODE_PREVLINK,
}win_msgcode_e;


typedef struct{
    win_msgtype_e type;
    void* code;
}webservice_msg_t;


// single thumbnail info to show in lvgl 
typedef struct{
    uint16_t obj_idx;
    uint16_t top;
    lv_obj_t* img_obj;
    lv_img_dsc_t lv_img_dsc;
    void* curl_handle;
    bool is_data_loaded;    //is pic loading, Whether you need to re-download the image
    bool is_data_ready;     //If you want to download an image, check whether the data is downloaded
}thumbnail_info_t;

typedef struct _region_code_t_ {
    unsigned char reg[4];
    uint32_t desc;
} region_code_t;

typedef struct _quality_code_t_ {
    uint32_t qual;
    unsigned char desc[8];
} quality_code_t;

typedef struct _search_option_t_ {
    uint32_t sear;
    uint32_t desc;
} search_option_t;

typedef enum {
    NULLTOKEN = 0,
    NEXTPAGETOKEN,
    PREVPAGETOKEN,
} pagetoken_e;

typedef struct{
    char id[32];        // video id
} ytb_history_info_t;

typedef struct{
    ytb_history_info_t history_info[MAX_YTB_HISTORY_VIDEO_SAVE];
    unsigned int new_index;
    unsigned int total_index;
} ytb_history_video_ids_t;

typedef struct{
    char region[4];                 // region code, i18n string
    char page_max[4];               // max page number, string
    unsigned int quality_option;        // supported quality option
    char service_addr[128];
    unsigned int search_option;
} ytb_app_config_t;

typedef struct{
    ytb_app_config_t iptv_config;
    ytb_history_video_ids_t ytb_history_video;
} hciptv_ytb_app_config_t;

extern lv_obj_t * webservice_scr;
void webservice_screen_init(void);
grid_list_data_t * grid_list_data_get(void);
void* webservice_thumb_buffer_get(void);
char* win_search_cont_str_get(void);
void webservice_set_webservice_msg_flag(bool en);
int webservice_pic_download_stop(void);
void ytb_history_video_id_save(char *video_id);
void webservice_thumb_buffer_free(void);
void webvideo_list_token_state_set(pagetoken_e state);

#endif // !1