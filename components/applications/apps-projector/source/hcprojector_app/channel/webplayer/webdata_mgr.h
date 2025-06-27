/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2023-04-06 15:55:14
 */
/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2023-03-29 19:17:20
 */
#ifndef __WEBDATA_MGR_H
#define __WEBDATA_MRG_H
#include <stdio.h> //printf()
#include <stdlib.h>
#include <string.h> //memcpy()
#include <unistd.h> //usleep()
#include <stdbool.h> //bool
#include <stdint.h> //uint32_t
#include <pthread.h>
#include <hccast/hccast_iptv.h>
#include "lvgl/lvgl.h"
// 获取buffer 中的个数
#define WEBVIDEO_BUFFER_NUM 20 
#define WEBAPIS_RETRY_NUM 3

// category list prop
typedef struct{
    hccast_iptv_cate_st* iptv_cate;
    int objsel_index;
}webcategory_list_t;


//video info prop
typedef struct {
    hccast_iptv_info_list_st* info_list;
    uint32_t offset;        //beacuse list buffer get < src video list cnt,offset to record its pos 
    uint32_t list_idx;      //src video list index
    uint32_t list_count;    //src video list cnt
    uint32_t is_eoh;        //end or start of list
}webvideo_list_t;

typedef enum{
    CURRRENT_LINK_ASK,
    NEXT_LINK_ASK,
    PREV_LINK_ASK,
}videolink_ask_e;

extern pthread_mutex_t pic_download_lock;
extern void *inst;

// mean something send to webservice_phtread  

int webservice_pthread_create(void);
int webservice_pthread_delete(void);
char* webcategory_list_strid_get_by_index(int index);
void* webcategory_list_data_get(void);
void* webvideo_list_data_get(void);
// void webvideo_list_free(void);
// void webcategory_list_free(void);
uint32_t webservice_pthread_msgid_get(void);
// void* webvideo_list_urls_get(void);

// void* list_nth_node(struct list_head * head ,int n);
void* list_nth_data(list_t * list,int n);
void* webvideo_url_data_get(void);
void webvideo_list_data_reset(void);
void* hciptv_handle_get(void);
void hccast_iptv_callback_event(hccast_iptv_evt_e event, void *arg);
void hciptv_y2b_service_init(hccast_iptv_notifier notifier);


#endif