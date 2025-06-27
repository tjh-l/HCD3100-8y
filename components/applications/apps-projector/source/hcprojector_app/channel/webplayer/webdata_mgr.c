#include "app_config.h"
#ifdef HCIPTV_YTB_SUPPORT
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <cjson/cJSON.h>
#include "glist.h"
#include "com_api.h"
#include "webdata_mgr.h"
#include "win_webservice.h"
#include "../local_mp/file_mgr.h"
#include "os_api.h"
#include "factory_setting.h"
#include <hccast/hccast_iptv.h>
#include <app_log.h>

typedef enum{
    // 0: category, 1: video, 2: trend
    YTB_TYPE_CATE = 0,
    YTB_TYPE_VIDEO,
    YTB_TYPE_TREND,
}ytb_list_req_type;

pthread_t pthread_id;
pthread_mutex_t web_lock;
pthread_mutex_t pic_download_lock;
static volatile bool stop_webservice=0;
uint32_t msg_id=0;

// iptv data struct
void *inst =NULL;
// hccast_iptv_handle used for iptv APIs 
webcategory_list_t webcate_list={0};
webvideo_list_t webvideo_list={0};
hccast_iptv_links_st *webvideo_link = NULL;
// source video link struct with diff urls
hccast_iptv_info_req_st urls_info={0};
// this data struct with a url link to both audio video or one of them


/*
input : index in the categroy list , it can attach with obj index 
*/ 
char* webcategory_list_strid_get_by_index(int index)
{
    char * out=NULL;
    if(webcate_list.iptv_cate!=NULL){
        if(index<=webcate_list.iptv_cate->cate_num){
            hccast_iptv_cate_node_st *cate_node=NULL;
            // printf(">>! %s(), line:%d, index=%d\n",__func__,__LINE__,index);
            cate_node=(hccast_iptv_cate_node_st *)list_nth_data(&webcate_list.iptv_cate->list,index);
            if(cate_node!=NULL){
                out=cate_node->cate_id;
            }
        }
    }
    if(out==NULL){
        printf("category get id error\n");
    }
    return out;
}

int webservice_cate_video_list_get(char * cate_id,webvideo_list_t* webvideo_list,pagetoken_e token,char type)
{
    int yt_ret = -1;
    hccast_iptv_cate_req_st cate_req = {0};
    hccast_iptv_info_list_st *info_list = NULL;
    pthread_mutex_lock(&web_lock);

    if(token==NULLTOKEN){
        /*When you get a new category, stop downloading the previous category*/
        hccast_iptv_handle_abort(inst);

        // Source data
        if(type == YTB_TYPE_CATE){
            if(cate_id){
                snprintf(cate_req.cate_id, sizeof(cate_req.cate_id), "%s", cate_id);
                cate_req.type = YTB_TYPE_CATE;
                // transmit cate id for diff cata ,NULL id for trending cate
            }else{
                cate_req.type = YTB_TYPE_TREND;
            }
        }else if(type == YTB_TYPE_VIDEO){
            cate_req.video_id = cate_id;
            cate_req.type = YTB_TYPE_VIDEO;
        }
        if(cate_id)
            printf("cate_id = %s\n", cate_id);
    }

    /*retry 3time for this apis in once*/ 
    for (int i = 1; i < (WEBAPIS_RETRY_NUM + 1) && !stop_webservice; i++){
        if(token==NULLTOKEN){
            yt_ret = hccast_iptv_cate_fetch(inst, &cate_req, &webvideo_list->info_list);
        }else if(token==PREVPAGETOKEN){
            yt_ret=hccast_iptv_page_get(inst,HCCAST_IPTV_PAGE_PREV,&webvideo_list->info_list);
            if(yt_ret==0){
                webvideo_list->offset = webvideo_list->offset - YTB_PAGE_CACHA_NUM;
            }
        }else if(token==NEXTPAGETOKEN){
            yt_ret=hccast_iptv_page_get(inst,HCCAST_IPTV_PAGE_NEXT,&info_list);
            if(yt_ret==0 && info_list!=NULL && info_list->info_num==0){
                printf("info_num = %d\n", info_list->info_num);
                webvideo_list->is_eoh = true;
            }else if(yt_ret==0 && info_list!=NULL){
                webvideo_list->offset = webvideo_list->offset + YTB_PAGE_CACHA_NUM;
                webvideo_list->info_list = info_list;
            }
        }
        if (!yt_ret || HCCAST_IPTV_RET_USER_ABORT_HANDLE == yt_ret){
            break;
        }
        printf("cate_fetch time : %d\n", i);
        usleep(i * 500 * 1000);
    }

    printf(">>func:%s  line:%d  !yt_ret:%d  token:%d\n",__func__,__LINE__,yt_ret,token);
    pthread_mutex_unlock(&web_lock);
    return yt_ret;
}

int webservice_video_list_get(char * cate_id,webvideo_list_t* webvideo_list,pagetoken_e token)
{
    return webservice_cate_video_list_get(cate_id, webvideo_list, token, YTB_TYPE_CATE);
}

int webservice_video_link_get(hccast_iptv_info_req_st* urls_info,videolink_ask_e vlink_ask)
{
    int yt_ret = -1;
    memset(urls_info,0,sizeof(hccast_iptv_info_req_st));
    int index=0;
    // index of the data list(webvideo_list) which only get some data of source list.
    if(vlink_ask==CURRRENT_LINK_ASK){
        index=webvideo_list.list_idx-webvideo_list.offset;
    }else if(vlink_ask==NEXT_LINK_ASK){
        index=webvideo_list.list_idx-webvideo_list.offset +1;
        webvideo_list.list_idx++;
    }else if(vlink_ask==PREV_LINK_ASK){
        index=webvideo_list.list_idx-webvideo_list.offset -1;
        webvideo_list.list_idx--;
    }
    // in a range of webvideo list 
    if(index>=0&&index<webvideo_list.info_list->info_num){
        list_node_t* sel_node=list_at(&webvideo_list.info_list->list,index);
        hccast_iptv_info_node_st* sel_videonode=(hccast_iptv_info_node_st*)sel_node->val;
        if(sel_videonode){
            snprintf(urls_info->id, sizeof(urls_info->id), "%s", sel_videonode->id);
            /*retry 3time for this apis in once*/ 
            for (int i = 1; i < (WEBAPIS_RETRY_NUM + 1) && !stop_webservice; i++){
                yt_ret = hccast_iptv_link_fetch(inst,urls_info, &webvideo_link);
                if (!yt_ret || HCCAST_IPTV_RET_USER_ABORT_HANDLE == yt_ret){
                    break;
                }
                printf("link_fetch time : %d\n",i);
                usleep(i * 500 * 1000);
            }
        }
    }

    printf(">>! yt_ret:%d \n",yt_ret);
    return yt_ret;
}

int webservice_search_list_get(webvideo_list_t* webvideo_list)
{
    int yt_ret = -1;
    hccast_iptv_search_req_st req = {0};
    req.key_word = win_search_cont_str_get();
    unsigned int search_option = sysdata_get_iptv_app_search_option();
    req.option = HCCAST_IPTV_SEARCH_TYPE_VIDEO | search_option;

    for (int i = 1; i < (WEBAPIS_RETRY_NUM + 1) && !stop_webservice; i++)
    {
        yt_ret = hccast_iptv_search_fetch(inst,&req,&webvideo_list->info_list);
        if (!yt_ret || HCCAST_IPTV_RET_USER_ABORT_HANDLE == yt_ret){
            break;
        }
        printf("search_list time : %d\n", i);
        usleep(i * 500 * 1000);
    }

    printf(">>! yt_ret=%d\n",yt_ret);
    return yt_ret;
}

void* webservice_pthread(void * args)
{
    control_msg_t webservice_msg;
    webservice_msg_t rsc_msg;
    int ret_code;
    stop_webservice = false;
    if(msg_id==0){
        msg_id=api_message_create(100,sizeof(webservice_msg_t));
    }
    if(webcate_list.iptv_cate==NULL){
        //category data get once 
        if(!hccast_iptv_cate_get(inst,&webcate_list.iptv_cate)){
            //data get success,send message to refresh lvgl display
            webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_START;
            webservice_msg.msg_code=WEBCATEGORY_LIST_GET_SUCCESS;
        }else{
            //data get do not get success,do something to reset the 
            // free data struct and send message to show a message box
            webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_START_ERROR;
            webservice_msg.msg_code=WEBCATEGORY_LIST_GET_ERROR;
        }
        api_control_send_msg(&webservice_msg);
    }
    if(webvideo_list.info_list==NULL){
        ret_code = webservice_video_list_get(NULL, &webvideo_list, NULLTOKEN);
        if(ret_code == 0){
            // video info data get sucess ,send message to refresh lvgl display
            webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_START;
            webservice_msg.msg_code=WEBVIDEO_INFO_GET_SUCCESS;
        }else if(ret_code == HCCAST_IPTV_RET_USER_ABORT_HANDLE){
            webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_START;
            webservice_msg.msg_code=WEBVIDEO_INFO_GET_ABORT;
        }else{
            webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_START;
            webservice_msg.msg_code=WEBVIDEO_INFO_GET_ERROR;
        }
        api_control_send_msg(&webservice_msg);
    }
    while(!stop_webservice){
        if(api_message_receive(msg_id,&rsc_msg,sizeof(webservice_msg_t))==0){
            if(rsc_msg.type==MSG_VIDEOLIST_REFRESH || rsc_msg.type==MSG_HISTORY_VIDEOLIST_REFRESH){
                char* strid=(char *)rsc_msg.code;
                // handle webvideo to do something
                // webvideo_list_data_reset();
                memset(&webvideo_list,0,sizeof(webvideo_list_t));
                /*When you get a new category, stop the image download thread*/
                webservice_pic_download_stop();
                if(rsc_msg.type==MSG_VIDEOLIST_REFRESH)
                    ret_code = webservice_video_list_get(strid, &webvideo_list, NULLTOKEN);
                else
                    ret_code = webservice_cate_video_list_get(strid, &webvideo_list, NULLTOKEN, YTB_TYPE_VIDEO);
                if(ret_code == 0){
                    // video info data get sucess ,send message to refresh lvgl display
                    webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                    webservice_msg.msg_code=WEBVIDEO_INFO_GET_SUCCESS;
                }else if(ret_code == HCCAST_IPTV_RET_USER_ABORT_HANDLE){
                    webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                    webservice_msg.msg_code=WEBVIDEO_INFO_GET_ABORT;
                }else{
                    webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                    webservice_msg.msg_code=WEBVIDEO_INFO_GET_ERROR;
                }
            }else if(rsc_msg.type==MSG_VIDEOLINK_ACK){
                printf("pthread rsc data form lvgl\n");
                videolink_ask_e rsc_code=*(videolink_ask_e *)rsc_msg.code;
                if(rsc_code==CURRRENT_LINK_ASK)
                    webservice_pic_download_stop();
                webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                ret_code = webservice_video_link_get(&urls_info, rsc_code);
                if(ret_code == 0){
                    if(rsc_code==CURRRENT_LINK_ASK){
                        webservice_msg.msg_code=WEBVIDEO_LINK_GET_SUCCESS;
                    }else if(rsc_code==NEXT_LINK_ASK){
                        webservice_msg.msg_code=WEBVIDEO_NEXT_LINK_GET_SUCCESS;
                    }else if(rsc_code==PREV_LINK_ASK){
                        webservice_msg.msg_code=WEBVIDEO_PREV_LINK_GET_SUCCESS;
                    }
                    ytb_history_video_id_save(urls_info.id);
                }else if(ret_code == HCCAST_IPTV_RET_USER_ABORT_HANDLE){
                    webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                    webservice_msg.msg_code=WEBVIDEO_LINK_GET_ABORT;
                }else{
                    webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                    if(rsc_code==CURRRENT_LINK_ASK){
                        webservice_msg.msg_code=WEBVIDEO_LINK_GET_ERROR;
                    }else if(rsc_code==NEXT_LINK_ASK){
                        webservice_msg.msg_code=WEBVIDEO_NEXT_LINK_GET_ERROR;
                    }else if(rsc_code==PREV_LINK_ASK){
                        webservice_msg.msg_code=WEBVIDEO_PREV_LINK_GET_ERROR;
                    }
                }
            }else if(rsc_msg.type==MSG_VIDEOLIST_NEXTPAGE_ACK){
                char current_id[16]={0};
                memcpy(current_id,webvideo_list.info_list->cate_id,16);
                if(!webservice_video_list_get(current_id,&webvideo_list,NEXTPAGETOKEN)){
                    int tmp = 0;
                    grid_list_data_t * gridlist=grid_list_data_get();
                    if(webvideo_list.info_list->info_num<YTB_PAGE_CACHA_NUM){
                        gridlist->list_cnt = webvideo_list.offset + webvideo_list.info_list->info_num;
                        gridlist->page_cnt = gridlist->list_cnt / gridlist->depth + (tmp = (gridlist->list_cnt % gridlist->depth > 0 ? 1 : 0));
                    }else if(webvideo_list.is_eoh){
                        // The next page is successful, but the info_num is 0, indicating that it is the last page
                        gridlist->list_cnt = webvideo_list.offset + YTB_PAGE_CACHA_NUM;
                        gridlist->page_cnt = gridlist->list_cnt / gridlist->depth + (tmp = (gridlist->list_cnt % gridlist->depth > 0 ? 1 : 0));
                    }
                    // video info data get sucess ,send message to refresh lvgl display
                    // need to diff msg type to distinguish in some case ?  
                    webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                    webservice_msg.msg_code=WEBVIDEO_NEXTPAGETOKEN_SUCCESS;
                }else {
                    webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                    webservice_msg.msg_code=WEBVIDEO_NEXTPAGETOKEN_ERROR;
                }
            }else if(rsc_msg.type==MSG_VIDEOLIST_PREVPAGE_ACK){
                char current_id[16]={0};
                memcpy(current_id,webvideo_list.info_list->cate_id,16);
                if(!webservice_video_list_get(current_id,&webvideo_list,PREVPAGETOKEN)){
                    // video info data get sucess ,send message to refresh lvgl display
                    // need to diff msg type to distinguish in some case ?  
                    webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                    webservice_msg.msg_code=WEBVIDEO_PREVPAGETOKEN_SUCCESS;
                }else {
                    webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                    webservice_msg.msg_code=WEBVIDEO_PREVPAGETOKEN_ERROR;
                }
            }else if(rsc_msg.type==MSG_VIDOE_SEARCH_ACK){           
                if(rsc_msg.code==NULL){        
                    /*When you get a new category, stop downloading the previous category, stop the image download thread*/
                    hccast_iptv_handle_abort(inst);
                    webservice_pic_download_stop();
                    /* for test , memory test*/
                    memset(&webvideo_list,0,sizeof(webvideo_list_t));
                    ret_code = webservice_search_list_get(&webvideo_list);
                    if(ret_code == 0){
                        webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                        webservice_msg.msg_code=WEBVIDEO_SEARCH_GET_SUCCESS;
                    }else if(ret_code == HCCAST_IPTV_RET_USER_ABORT_HANDLE){
                        webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                        webservice_msg.msg_code=WEBVIDEO_SEARCH_GET_ABORT;
                    }else{
                        webservice_msg.msg_type=MSG_TYPE_WEBSERVICE_LOADED;
                        webservice_msg.msg_code=WEBVIDEO_SEARCH_GET_ERROR;
                    }
                }
            }
            api_control_send_msg(&webservice_msg);
        }
        api_sleep_ms(10);
    }

    webservice_pic_download_stop();
    hccast_iptv_handle_abort(inst);
    hccast_iptv_app_deinit(inst);

    printf("!!!!!!stop_webservice=%d ,pthread has return and join\n",stop_webservice);
    return NULL;
}

int webservice_pthread_delete(void)
{
    stop_webservice = true;
    hccast_iptv_handle_abort(inst);
    webvideo_list_token_state_set(NULLTOKEN);
    webservice_set_webservice_msg_flag(false);
    pthread_mutex_destroy(&web_lock);
    pthread_mutex_destroy(&pic_download_lock);
    printf(">>!stop_webservice %d\n",stop_webservice);
    if (pthread_id){
        pthread_join(pthread_id, NULL);
    }
    pthread_id = 0;
    memset(&webcate_list,0,sizeof(webcategory_list_t));
    memset(&webvideo_list,0,sizeof(webvideo_list_t));
    api_hotkey_disable_clear();
    printf("\n>>> %s\n\n", __func__);
    return 0;
}

void ytb_app_config_read(hccast_iptv_app_config_st *hccast_iptv_config, hciptv_ytb_app_config_t *ytb_app_config)
{
    hccast_iptv_config->log_level = LL_NOTICE;
    snprintf(hccast_iptv_config->region, sizeof(hccast_iptv_config->region), "%s", ytb_app_config->iptv_config.region);
    snprintf(hccast_iptv_config->page_max, sizeof(hccast_iptv_config->page_max), "%s", ytb_app_config->iptv_config.page_max);
    hccast_iptv_config->quality_option = ytb_app_config->iptv_config.quality_option;
    snprintf(hccast_iptv_config->service_addr, sizeof(hccast_iptv_config->service_addr), "%s", ytb_app_config->iptv_config.service_addr);
}

static uint32_t m_hotkey[] = {KEY_POWER, KEY_VOLUMEUP, \
                    KEY_VOLUMEDOWN, KEY_MUTE, KEY_ROTATE_DISPLAY, KEY_FLIP,KEY_CAMERA_FOCUS,KEY_FORWARD,KEY_BACK,KEY_HOME};

int webservice_pthread_create(void)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr,0x8000);

    //if use pthread_join() to wait and recovery the thread resource, DO NOT set the thread to detached state.
    // pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    hccast_iptv_app_config_st hccast_iptv_config = {0};
    hccast_iptv_config_get(inst,&hccast_iptv_config);
    hciptv_ytb_app_config_t *ytb_app_config;
    ytb_app_config = sysdata_get_iptv_app_config();
    ytb_app_config_read(&hccast_iptv_config, ytb_app_config);
    hccast_iptv_app_init(inst, &hccast_iptv_config, NULL);

    api_hotkey_enable_set(m_hotkey, sizeof(m_hotkey)/sizeof(m_hotkey[0]));

    if(pthread_create(&pthread_id,&attr,webservice_pthread,(void*)NULL)){
        printf("create pthread fail %s,%d \n", __func__, __LINE__);
    }
    pthread_attr_destroy(&attr);
    pthread_mutex_init(&web_lock, NULL);
    pthread_mutex_init(&pic_download_lock, NULL);
    return 0;
}

/*返回使用到的全局变量*/
void* webcategory_list_data_get(void)
{
    return &webcate_list;
}
/*apply the pthread mutex to sync lvgl pthread and webdata pthread?*/ 
void* webvideo_list_data_get(void)
{
    return &webvideo_list;
}

uint32_t webservice_pthread_msgid_get(void)
{
    return msg_id;
}
void webvideo_list_data_reset(void)
{
    pthread_mutex_lock(&web_lock);
    webvideo_list.offset=0;
    webvideo_list.list_idx=0;
    webvideo_list.list_count=0;
    webvideo_list.is_eoh=0;
    pthread_mutex_unlock(&web_lock);
}

void* list_nth_data(list_t * list,int n)
{
    list_node_t* node=NULL;
    if(list)
        node=list_at(list,n);
    return node ? node->val : NULL;
}

void* webvideo_url_data_get(void)
{
    return &urls_info;
}

void* hciptv_handle_get(void)
{
    return inst;
}
static bool is_demo_flag=false;
bool iptv_is_demo(void){
    return is_demo_flag;
}
void hccast_iptv_callback_event(hccast_iptv_evt_e event, void *arg)
{
    control_msg_t ctl_msg = {0};
    arg=NULL;
    switch (event){
        case HCCAST_IPTV_EVT_INVALID_CERT:
            ctl_msg.msg_type=MSG_TYPE_HCIPTV_INVALID_CERT;
            printf(">>!%s ,%d\n",__func__,__LINE__);
            is_demo_flag=true;
            break;
        default: 
            break;
    }
    if (0 != ctl_msg.msg_type){
        api_control_send_msg(&ctl_msg);
    }
    return;
}

void hciptv_y2b_service_init(hccast_iptv_notifier notifier)
{
    hccast_iptv_service_init();
    hccast_iptv_service_app_init(HCCAST_IPTV_APP_YT);
    inst = hccast_iptv_app_open(HCCAST_IPTV_APP_YT);
    hccast_iptv_app_config_st hccast_iptv_config = {0};
    hciptv_ytb_app_config_t *ytb_app_config;

    ytb_app_config = sysdata_get_iptv_app_config();
    hccast_iptv_config_get(inst, &hccast_iptv_config);
    ytb_app_config_read(&hccast_iptv_config, ytb_app_config);

    hccast_iptv_app_init(inst, &hccast_iptv_config, notifier);
    return;
}

#endif
