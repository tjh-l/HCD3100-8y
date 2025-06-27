#include "app_config.h"
#ifdef  HCIPTV_YTB_SUPPORT
#include "lvgl/lvgl.h"
#include "win_webservice.h" 
#include "com_api.h"
#include "webdata_mgr.h"
#include "glist.h"
#include "screen.h"
#include "os_api.h"
#include "win_webplay.h"
#include "osd_com.h"
#include "setup.h"
#include "network_api.h"
#include "key_event.h"
#include "factory_setting.h"

/*  
    reg  : iptv region id
    desc : The full name of the region
*/
region_code_t g_region_code[] = {
    {.reg = "US", .desc = STR_REGION_US},
    {.reg = "HK", .desc = STR_REGION_HK},
    {.reg = "TW", .desc = STR_REGION_TW},
    {.reg = "JP", .desc = STR_REGION_JP},
};

/*  
    reg  : hccast_iptv_video_quality_e
    desc : description
*/
quality_code_t g_quality_code[] = {
    {.qual = HCCAST_IPTV_VIDEO_1080P, .desc = "1080P"},
    {.qual = HCCAST_IPTV_VIDEO_720P,  .desc = "720P"},
    {.qual = HCCAST_IPTV_VIDEO_480P,  .desc = "480P"},
};

/*
    sear        : hccast_iptv_video_quality_e
    desc        : description

    Relevance   : Resources are sorted based on their relevance to the search query. This is the default value for this parameter.
    Upload Date : Resources are sorted in reverse chronological order based on the date they were created.
    Rating      : Resources are sorted from highest to lowest rating.
    Title       : Resources are sorted alphabetically by title.
    View Count  : Resources are sorted from highest to lowest number of views. For live broadcasts,
                    videos are sorted by number of concurrent viewers while the broadcasts are ongoing.
    Video Count : Channels are sorted in descending order of their number of uploaded videos.
*/
search_option_t g_search_option[] = {
    {.sear = HCCAST_IPTV_SEARCH_ORDER_RELEVANCE,  .desc = STR_RELEVANCE},
    {.sear = HCCAST_IPTV_SEARCH_ORDER_DATE,       .desc = STR_UPLOAD_DATE},
    {.sear = HCCAST_IPTV_SEARCH_ORDER_VIEWCOUNT,  .desc = STR_VIEW_COUNT},
    {.sear = HCCAST_IPTV_SEARCH_ORDER_RATING,     .desc = STR_RATING},
    {.sear = HCCAST_IPTV_SEARCH_ORDER_TITLE,      .desc = STR_TITLE}, /*It is not recommended that the page YouTube is not available above*/
    //{.sear = HCCAST_IPTV_SEARCH_ORDER_VIDEOCOUNT, .desc = "Video Count"}, /*unsupported*/
};

lv_obj_t * webservice_scr = NULL;
static lv_obj_t* top_cont = NULL;
static lv_obj_t* list_obj = NULL;
static lv_obj_t* objcont = NULL;
static lv_group_t* gp;

static thumbnail_info_t thumb_info[SUBCONT]={0};
static bool webservice_msg_flag = false; // Wait for the msg to return for the next key response
static char ytb_history_cate_id[33 * MAX_YTB_HISTORY_VIDEO_SAVE + 1];
static uint32_t webservice_old_page_idx = 0;

void main_cont_list_add_data_by_web(lv_obj_t* obj,webcategory_list_t* webcate_list);
static void win_webservice_msg_handle(void* arg1 ,void *arg2);
static void event_handler_list_btn_item(lv_event_t* event);
static void event_handler_sobjcont_item(lv_event_t* event);
int webservice_pic_downloaded(webvideo_list_t* webvideo_list);
static int objcont_group_refocus(grid_list_data_t* obj_data);
int webservice_pic_download_stop(void);
void webservice_thumb_buffer_free(void);
void webservice_pic_flush_by_thumb_info(void);
void list_obj_group_refocus(lv_obj_t* f_obj);

void webservice_set_webservice_msg_flag(bool en)
{
    webservice_msg_flag = en;
}

static void create_top_cont(lv_obj_t* p)
{
    top_cont=lv_obj_create(p);
    lv_obj_set_size(top_cont,LV_PCT(100),LV_PCT(17));
    lv_obj_set_pos(top_cont, lv_pct(0), LV_PCT(0));
    lv_obj_set_style_radius(top_cont,0,0);
    lv_obj_set_style_bg_color(top_cont,lv_color_hex(BLACKCOLOR_LEVEL1),0);
    lv_obj_set_style_border_width(top_cont,0,0);
    lv_obj_set_style_pad_all(top_cont,3,0);
    lv_obj_clear_flag(top_cont,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* img=lv_img_create(top_cont);
    lv_img_set_src(img,&Youtube_logo);
    lv_img_set_zoom(img,210);   //to large set zoom with user size

    lv_obj_t* page_label=lv_label_create(top_cont);
    lv_obj_align(page_label,LV_ALIGN_BOTTOM_RIGHT,-10,0);
    lv_label_set_text(page_label,"");
    lv_obj_set_style_text_color(page_label,lv_color_hex(0xffffff),0);
    return;
}

int video_time_format_get(char *total_tim_str,char* time_format){
    int hour = 0, minute = 0, second = 0;
    // sscanf(total_tim_str, "PT%dh%dM%dS", &hour, &minute, &second);
    char* tmp=strchr(total_tim_str,'T');
    if (tmp == NULL)
    {
        printf("total_tim_str error\n");
        return 0;
    }
    int val=atoi(tmp+1);
    tmp=strchr(total_tim_str,'H');
    if(tmp){
        hour=val;
        if(strstr(total_tim_str,"M")){
            minute=atoi(tmp+1);
            tmp=strchr(total_tim_str,'M');
            if(tmp){
                if(strstr(total_tim_str,"S"))
                    second=atoi(tmp+1);
            }
        }else{
            if(strstr(total_tim_str,"S"))
                second=atoi(tmp+1);
        }
    }else{
        tmp=strchr(total_tim_str,'M');
        if(tmp){
            minute=val;
            if(strstr(total_tim_str,"S"))
                second=atoi(tmp+1);
        }else{
            second=val;
        }
    }
    sprintf(time_format, "%02d:%02d:%02d", (int)hour, (int)minute, (int)second);
    return 0;
}

/**
 * @description: a data struct for view to manage obj (like a list)
 * @author: Yanisin
 */
static grid_list_data_t grid_list_data={0};
/**
 * @description: to reset the grid_list st ,has a relation whth webvideo_list->cnt
 * @param {grid_list_data_t*} grid_list
 * @author: Yanisin
 */
static void grid_list_data_init(webvideo_list_t* webvideo_list){
    if(webvideo_list->info_list==NULL)
        return ;
    int tmp;
    grid_list_data.top=0;
    grid_list_data.depth=SUBCONT;
    grid_list_data.cur_pos=0;
    grid_list_data.list_sel_idx=0;
    /*When the YT is Unauthorized, only get one video message, the info_total_num are incorrect*/
    if(webvideo_list->info_list->info_num<YTB_PAGE_CACHA_NUM){
        grid_list_data.list_cnt = webvideo_list->offset + webvideo_list->info_list->info_num;
    }else{
        grid_list_data.list_cnt=webvideo_list->info_list->info_total_num;  /* For searches, the info_total_num are incorrect */
    }
    grid_list_data.page_cnt=grid_list_data.list_cnt/grid_list_data.depth+(tmp=(grid_list_data.list_cnt%grid_list_data.depth>0?1:0));
    if (grid_list_data.list_cnt == 0){
        grid_list_data.page_cnt = 1;
    }
    grid_list_data.page_idx=1;
}

grid_list_data_t * grid_list_data_get(void)
{
    return &grid_list_data;
}


static void grid_list_data_reset_by_pagetoken(webvideo_list_t* webvideo_list ,pagetoken_e pagetoken)
{
    if(pagetoken==NEXTPAGETOKEN && grid_list_data.page_idx>1){
        grid_list_data.page_idx--;
    }else if(pagetoken == PREVPAGETOKEN){
        grid_list_data.page_idx++;
    }else{
        grid_list_data.page_idx = 1;
        printf("%s():%d grid_list_data error\n", __func__, __LINE__);
    }

    grid_list_data.cur_pos = grid_list_data.list_sel_idx % (SUBCONT / 2); // (SUBCONT / 2) is the number of videos displayed per row
    grid_list_data.top = grid_list_data.depth * (grid_list_data.page_idx - 1);
    grid_list_data.list_sel_idx = grid_list_data.top + grid_list_data.cur_pos;
    webvideo_list->list_idx = grid_list_data.list_sel_idx;

    //printf("list_idx:%d,page:%d/%d,top:%d\n",grid_list_data.list_sel_idx,grid_list_data.page_idx,grid_list_data.page_cnt,grid_list_data.top);
    return;
}

static void grid_list_display_flush_by_empty(void)
{
    for(int k=0;k<SUBCONT;k++){
        lv_obj_t* child_obj=lv_obj_get_child(objcont,k);
        lv_obj_clean(child_obj);
    }
    return ;
}

/**
 * @description: flush obj display context by the src data struct (webvideo list)
 * @param {grid_list_data_t*} grid_list :  a data struct for obj display
 * @param {webvideo_list_t*} webvideo_list : a src data struct get form webservice pthread 
 * @author: Yanisin
 */
static void grid_list_display_flush(bool is_reload, grid_list_data_t* grid_list,webvideo_list_t* webvideo_list)
{
    int j=0;
    hccast_iptv_info_node_st* video_node=NULL;
    if(is_reload==false){
        webservice_pic_download_stop();
        lv_refr_now(lv_disp_get_default());
        printf("download over\n");
    }
    // clean context before flush new context
    for(int k=0;k<SUBCONT;k++){
        lv_obj_t* child_obj=lv_obj_get_child(objcont,k);
        if(child_obj)
            lv_obj_clean(child_obj);
    }
    //clean page_label before flush new context
    // flush new context
    if(webvideo_list!=NULL&&webvideo_list->info_list!=NULL){
       for(int i =grid_list->top;i<grid_list->depth+grid_list->top;i++){
            // printf(">>! grid_index:%d, offset:%d \n", i,webvideo_list->offset);
            // printf(">> %s(), line:%d, i=%d,offset=%d\n",__func__,__LINE__,i,webvideo_list->offset);
            video_node=(hccast_iptv_info_node_st *)list_nth_data(&webvideo_list->info_list->list,i-webvideo_list->offset);
            if(video_node!=NULL){
                // printf("flush :video_node->title:%s\n",video_node->title);
                lv_obj_t* subobj=lv_obj_get_child(objcont,j);
                lv_obj_t* label=lv_label_create(subobj);
                lv_obj_set_size(label,lv_pct(100),LV_SIZE_CONTENT);
                lv_obj_set_align(label,LV_ALIGN_BOTTOM_MID);
                lv_obj_set_style_text_color(label,lv_color_hex(0xffffff),0);
                lv_obj_set_style_text_font(label,&YTB_MID_FONT,0);
                lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_LEFT,0);
                lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
                lv_label_set_text(label,video_node->title);

                lv_obj_t* img_cont=lv_obj_create(subobj);
                lv_obj_set_style_bg_opa(img_cont,LV_OPA_TRANSP,0);
                lv_obj_set_style_border_width(img_cont,0,0);
                lv_obj_clear_flag(img_cont,LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_size(img_cont,LV_PCT(59),lv_pct(62));
                lv_obj_set_align(img_cont,LV_ALIGN_TOP_LEFT);
                thumb_info[j].img_obj =lv_img_create(img_cont);
                lv_obj_set_size(thumb_info[j].img_obj,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
                lv_obj_center(thumb_info[j].img_obj);
                lv_img_set_src(thumb_info[j].img_obj,&thumbnail_default);
                if(is_reload==false)
                    thumb_info[j].is_data_ready = false;

                lv_obj_t* label_cont=lv_obj_create(subobj);
                lv_obj_set_size(label_cont,LV_PCT(41),LV_PCT(62));
                lv_obj_set_style_bg_opa(label_cont,LV_OPA_TRANSP,0);
                lv_obj_set_style_border_width(label_cont,0,0);
                lv_obj_clear_flag(label_cont,LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_align_to(label_cont,img_cont,LV_ALIGN_OUT_RIGHT_MID,0,0);

                if(video_node->duration!=NULL){
                    lv_obj_t* total_tim=lv_label_create(label_cont);
                    lv_obj_set_align(total_tim,LV_ALIGN_TOP_LEFT);
                    char time_fmt[16]={0};
                    memset(time_fmt,0,16);
                    if (video_node->type == 0) //(0: vod, 1: live)
                    {
                        video_time_format_get(video_node->duration, time_fmt);
                        lv_obj_set_style_text_color(total_tim,lv_color_hex(0xffffff),0);
                    }
                    else if (video_node->type == 1)
                    {
                        sprintf(time_fmt, "LIVE");
                        lv_obj_set_style_text_color(total_tim,lv_color_hex(0xED1C24),0); //red
                    }
                    lv_label_set_text(total_tim,time_fmt);
                    lv_obj_set_style_text_font(total_tim,&YTB_MID_FONT,0);
                }

                if(video_node->viewCount>0){
                    lv_obj_t * view=lv_label_create(label_cont);
                    lv_obj_set_align(view,LV_ALIGN_BOTTOM_LEFT);
                    lv_label_set_text_fmt(view,"view: \n%ld",video_node->viewCount);
                    lv_obj_set_style_text_color(view,lv_color_hex(0xffffff),0);
                    lv_obj_set_style_text_font(view,&YTB_MID_FONT,0); 
                }

                j++;

            }
        }
        lv_obj_t* pagelabel=lv_obj_get_child(top_cont,1);
        /* while page_cnt == 0  is not displayed */
        if (grid_list->page_cnt == 0)
            lv_label_set_text_fmt(pagelabel,"[%ld / ...]",grid_list->page_idx);
        else
            lv_label_set_text_fmt(pagelabel,"[%ld / %ld]",grid_list->page_idx,grid_list->page_cnt);
        // video thumb downloading and then to flush thumb when downloaded
        // before dowmloaded new pic stop downloading pic if it is not ready
        if(is_reload==false){
            webservice_pic_downloaded(webvideo_list);
        }else if (is_reload ==true){
            // flush something when from webplayscr into,flush thumb whitout loaded
            webservice_pic_flush_by_thumb_info();
        }
    }
}

static pagetoken_e grid_list_update_by_key(grid_list_data_t* grid_list,uint32_t lv_key)
{
    pagetoken_e pagetoken=NULLTOKEN;
    int index;

    switch(lv_key){
        case LV_KEY_UP:
            if(grid_list->list_sel_idx-2>=0&&grid_list->list_sel_idx<=grid_list->list_cnt-1){
                grid_list->list_sel_idx=grid_list->list_sel_idx-2;
            }else{//边界处理，ps:处理聚焦时这个算法在总数是偶数是对的但是奇数，处理聚焦不对，list_index是对的
                // grid_list->list_sel_idx=grid_list->list_cnt-2+grid_list->list_sel_idx;
                pagetoken=PREVPAGETOKEN;
            }
            break;
        case LV_KEY_DOWN:
            if(grid_list->list_sel_idx>=0&&grid_list->list_sel_idx+2<=grid_list->list_cnt-1){
                grid_list->list_sel_idx=grid_list->list_sel_idx+2;
            }else if(grid_list->list_sel_idx>=0&&grid_list->list_sel_idx+1==grid_list->list_cnt-1){
                //You can turn the page when there is only one video on the next page
                grid_list->list_sel_idx++;
            }else{//边界处理
                // grid_list->list_sel_idx=grid_list->list_sel_idx+2-grid_list->list_cnt;
                pagetoken=NEXTPAGETOKEN;
            }
            break;
        case LV_KEY_LEFT:
            if(grid_list->list_sel_idx>=1&&grid_list->list_sel_idx<=grid_list->list_cnt-1){
                grid_list->list_sel_idx--;
            }else{
                // grid_list->list_sel_idx=grid_list->list_cnt-1;
                pagetoken=PREVPAGETOKEN;
            }
            break;
        case LV_KEY_RIGHT:
            if(grid_list->list_sel_idx>=0&&grid_list->list_sel_idx+1<=grid_list->list_cnt-1){
                grid_list->list_sel_idx++;
            }else{
                // grid_list->list_sel_idx=0;
                pagetoken=NEXTPAGETOKEN;
            }
            break;
        case LV_KEY_PREV:
            index = grid_list->list_sel_idx % SUBCONT + SUBCONT;
            if(grid_list->list_sel_idx-index>=0&&grid_list->list_sel_idx<=grid_list->list_cnt-1){
                grid_list->list_sel_idx=grid_list->list_sel_idx-index;
            }else{
                pagetoken=PREVPAGETOKEN;
            }
            break;
        case LV_KEY_NEXT:
            index = SUBCONT - (grid_list->list_sel_idx % SUBCONT);
            if(grid_list->list_sel_idx>=0&&grid_list->list_sel_idx+index<=grid_list->list_cnt-1){
                grid_list->list_sel_idx=grid_list->list_sel_idx+index;
            }else{
                pagetoken=NEXTPAGETOKEN;
            }
            break;
    }
    if(grid_list->list_sel_idx+1<=grid_list->depth){
        grid_list->page_idx=1;
    }else{
        grid_list->page_idx=(grid_list->list_sel_idx+1)/grid_list->depth+((grid_list->list_sel_idx+1)%grid_list->depth>0?1:0);
    }
    grid_list->top=grid_list->depth*(grid_list->page_idx-1);
    grid_list->cur_pos=grid_list->list_sel_idx-grid_list->top;
    // printf("list_idx:%d,page:%d/%d,top:%d\n",grid_list->list_sel_idx,grid_list->page_idx,grid_list->page_cnt,grid_list->top);
    return pagetoken ;
}
/**
 * @description:  handle fouces ,updata web datastruct and gridlist strcut(for view) when key input in objcont
 * @param {uint32_t} lv_key
 * @param {webvideo_list_t*} webvideo_list
 * @return {*}
 * @author: Yanisin
 */
static pagetoken_e grid_list_key_ctrl(lv_obj_t *obj,uint32_t lv_key,webvideo_list_t* webvideo_list)
{
    pagetoken_e vlist_update_f= NULLTOKEN;
    static int last_pageindex = 1;
    last_pageindex=grid_list_data.page_idx;
    pagetoken_e token_state=grid_list_update_by_key(&grid_list_data,lv_key);
    if(webvideo_list!=NULL&&webvideo_list->info_list!=NULL){
        webvideo_list->list_idx=grid_list_data.list_sel_idx;
        webvideo_list->list_count=grid_list_data.list_cnt;

        if(webvideo_list->list_idx>=webvideo_list->offset+webvideo_list->info_list->info_num&&token_state==NULLTOKEN) {
            vlist_update_f=NEXTPAGETOKEN;
        }
        if(webvideo_list->list_idx<webvideo_list->offset&&token_state==NULLTOKEN) {
            vlist_update_f=PREVPAGETOKEN;
        }

        if (vlist_update_f==NULLTOKEN && lv_obj_is_valid(lv_obj_get_child(objcont,grid_list_data.cur_pos)))
        {
            lv_group_focus_obj(lv_obj_get_child(objcont,grid_list_data.cur_pos));
        }

        if(last_pageindex!=grid_list_data.page_idx&&vlist_update_f==NULLTOKEN){
            grid_list_display_flush(false,&grid_list_data,webvideo_list);//do not flush when turn page
            last_pageindex=grid_list_data.page_idx;
        }
    }
    return vlist_update_f;
}



void main_cont_list_add_data_by_web(lv_obj_t* obj,webcategory_list_t* webcate_list)
{
    /*获取YouTube 的数据并解析为对应的数据结构*/
    // lv_group_remove_obj(webservice_scr);
    // lv_group_remove_all_objs(gp);
    /*提前添加两个分类，用于设置搜索及流行分类*/
    lv_obj_set_style_pad_hor(obj,0,0);
    for(int i=0;i<PREBUILD_CATE_CNT;i++){
        lv_obj_t* btn = lv_btn_create(obj);
        lv_obj_add_event_cb(btn,event_handler_list_btn_item,LV_EVENT_ALL,0);
        lv_obj_set_flex_align(btn,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
        //添加btn style
        lv_obj_t* label=lv_label_create(btn);
        lv_obj_set_style_text_font(label,&YTB_LARGET_FONT,0);
        if(i==YTB_HISTORY_CATEGORY){
            lv_label_set_text(label,PREBUILD_HISTORY_CATEGORY);
        }else if(i==YTB_SERARCH_CATEGORY){
            lv_label_set_text(label,PREBUILD_SERARCH_CATEGORY);
        }else if(i==YTB_TRENDING_CATEGORY){
            lv_label_set_text(label,PREBUILD_TRENDING_CATEGORY);
        }
        lv_obj_set_width(btn,lv_pct(100));
        lv_obj_set_style_bg_color(btn,lv_color_hex(BLACKCOLOR_LEVEL1),0);
        lv_obj_set_style_text_color(btn,lv_color_hex(0xffffff),0);
        lv_obj_set_style_text_align(btn,LV_ALIGN_CENTER,0);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_FULL, 0);
        lv_obj_set_style_border_width(btn, 3, 0);
        lv_obj_set_style_border_color(btn, lv_palette_lighten(LV_PALETTE_RED, 1), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_border_color(btn, lv_color_hex(BLACKCOLOR_LEVEL1), 0);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);  // LV_STATE_DEFAULT doesn't work
        lv_obj_set_style_radius(btn, 0, 0);
    }
    if(webcate_list->iptv_cate!=NULL){
        list_iterator_t *it = list_iterator_new(&webcate_list->iptv_cate->list, LIST_HEAD);
        list_node_t *node;
        while ((node = list_iterator_next(it))) {
            hccast_iptv_cate_node_st *cate_node = (hccast_iptv_cate_node_st *)node->val;
            if(!cate_node){
                list_iterator_destroy(it);
                return;
            }
            lv_obj_t* btn = lv_btn_create(obj);
            lv_obj_add_event_cb(btn,event_handler_list_btn_item,LV_EVENT_ALL,0);
            lv_obj_set_flex_align(btn,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
            //添加btn style    
            lv_obj_t* label=lv_label_create(btn);
            lv_label_set_text(label,cate_node->cate_name);
            lv_obj_set_style_text_font(label,&YTB_LARGET_FONT,0);
            lv_obj_set_width(btn,lv_pct(100));
            lv_obj_set_style_bg_color(btn,lv_color_hex(BLACKCOLOR_LEVEL1),0);
            lv_obj_set_style_text_color(btn,lv_color_hex(0xffffff),0);
            lv_obj_set_style_text_align(btn,LV_ALIGN_CENTER,0);
            lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_FULL, 0);
            lv_obj_set_style_border_width(btn, 3, 0);
            lv_obj_set_style_border_color(btn, lv_palette_lighten(LV_PALETTE_RED, 1), LV_STATE_FOCUS_KEY);
            lv_obj_set_style_border_color(btn, lv_color_hex(BLACKCOLOR_LEVEL1), 0);
            lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);  // LV_STATE_DEFAULT doesn't work
            lv_obj_set_style_radius(btn, 0, 0);
            // */
        }
        list_iterator_destroy(it);
        lv_group_focus_obj(lv_obj_get_child(list_obj, YTB_TRENDING_CATEGORY));
        // focus on the second obj
    }
}
void create_main_cont(lv_obj_t*p)
{
    lv_obj_t* main_cont=lv_obj_create(p);
    lv_obj_set_size(main_cont,lv_pct(100),lv_pct(75));
    lv_obj_set_pos(main_cont, lv_pct(0), LV_PCT(17));
    lv_obj_set_style_pad_all(main_cont,0,0);
    lv_obj_set_style_radius(main_cont,0,0);
    lv_obj_set_style_bg_color(main_cont,lv_color_hex(BLACKCOLOR_LEVEL1),0);
    lv_obj_set_style_border_width(main_cont,0,0);

    list_obj=lv_list_create(main_cont);
    lv_obj_set_align(list_obj,LV_ALIGN_LEFT_MID);
    lv_obj_set_size(list_obj,lv_pct(35),lv_pct(100));
    // /*   LV_LIST OBJ STYLE
    lv_obj_set_style_border_width(list_obj,4,0);
    lv_obj_set_style_radius(list_obj,0,0);
    lv_obj_set_style_bg_color(list_obj,lv_color_hex(BLACKCOLOR_LEVEL1),0);
    lv_obj_set_style_border_side(list_obj,LV_BORDER_SIDE_RIGHT,0);
    lv_obj_set_style_border_color(list_obj,lv_color_hex(0x3f9cd6),0);
    // */
    objcont=lv_obj_create(main_cont);
    lv_obj_set_align(objcont,LV_ALIGN_RIGHT_MID);
    lv_obj_set_size(objcont,lv_pct(65),lv_pct(100));
    lv_obj_set_flex_flow(objcont,LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(objcont,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    // /* OBJCONT STYLE 
    lv_obj_set_style_radius(objcont,0,0);
    lv_obj_set_style_bg_color(objcont,lv_color_hex(BLACKCOLOR_LEVEL1),0);
    lv_obj_set_style_border_width(objcont,0,0);
    // */
    for(int i=0;i<SUBCONT;i++){
        lv_obj_t* subobjcont=lv_obj_create(objcont);
        lv_obj_set_size(subobjcont,lv_pct(43),lv_pct(43));
        //add your style 
        lv_obj_set_style_border_color(subobjcont,lv_color_hex(0xFF0000),LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_color(subobjcont,lv_color_hex(BLACKCOLOR_LEVEL2),0);
        lv_obj_set_style_border_color(subobjcont,lv_color_hex(BLACKCOLOR_LEVEL2),0);
        lv_obj_set_style_text_color(subobjcont,lv_color_hex(0xffffff),0);
    }

    /*due to other pthread msg will send event core to obj ,so make sure obj was create then add event
      then event core will be cure event handler */
    for(int j=0;j<SUBCONT;j++){
        lv_obj_t* tmp_obj=lv_obj_get_child(objcont,j);
        if(tmp_obj)
            lv_obj_add_event_cb(tmp_obj,event_handler_sobjcont_item,LV_EVENT_ALL,0);
    }

}

void create_bottom_cont(lv_obj_t* p)
{
    lv_obj_t* foot = lv_obj_create(p);
    lv_obj_set_size(foot,LV_PCT(100),LV_PCT(8));
    lv_obj_set_pos(foot, lv_pct(0), LV_PCT(92));
    lv_obj_set_style_bg_color(foot, lv_color_hex(0x303030), 0);
    lv_obj_set_style_bg_opa(foot, LV_OPA_100, 0);
    lv_obj_set_style_border_width(foot, 2, 0);
    lv_obj_set_style_border_side(foot, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(foot, lv_color_hex(0x303030), 0);
    lv_obj_set_style_border_opa(foot, LV_OPA_100, 0);
    lv_obj_set_style_outline_width(foot, 0 ,0);
    lv_obj_set_style_pad_all(foot, 0, 0);
    lv_obj_set_style_pad_gap(foot,0,0);
    lv_obj_set_style_radius(foot,0,0 );
    
    lv_obj_set_flex_flow(foot, LV_FLEX_FLOW_ROW);
    lv_obj_t *obj,*label,*img;
    lv_img_dsc_t* img_dsc[4] = {&MENU_IMG_LIST_PAGE, &MENU_IMG_LIST_TABLE, &MENU_IMG_LIST_OK, &MENU_IMG_LIST_MENU};

    int foot_map[4] = {STR_FOOT_PAGE, STR_FOOT_MENU, STR_FOOT_SURE, STR_FOOT_OFF};
    for(int i=0; i<4;i++){
        obj = lv_obj_create(foot);
        lv_obj_set_size(obj, LV_PCT(25), LV_PCT(100));
        lv_obj_set_style_pad_ver(obj, 2, 0);
        lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
        //lv_obj_set_style_pad_hor(obj, LV_PCT(20), 0);
        set_pad_and_border_and_outline(obj);
        
        lv_obj_set_style_radius(obj, 0, 0);
        lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);

        img = lv_img_create(obj);
        lv_img_set_src(img, img_dsc[i]);
        //lv_obj_align(img, LV_ALIGN_TOP_LEFT, LV_PCT(30), 0);
        label = lv_label_create(obj);
        lv_obj_align_to(label, img, LV_ALIGN_OUT_RIGHT_MID, 2, -10);
        lv_label_set_recolor(label, true);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);

        //language_choose_add_label1(label, );
        set_label_text2(label, foot_map[i], FONT_NORMAL);
    }

}
int win_webservice_open(void* args)
{
    create_top_cont(webservice_scr);
    create_main_cont(webservice_scr);
    create_bottom_cont(webservice_scr);
    return 0;
}

int webservice_init(void *args)
{
    // webservice_pthread_create();
    // has start youtube pthread when press key 
    gp=lv_group_create();
    gp->auto_focus_dis = 1;
    lv_group_set_default(gp);
    lv_indev_set_group(indev_keypad, gp);     // 键盘
    // lv_group_add_obj(gp,webservice_scr);
    // lv_group_focus_obj(webservice_scr);
    win_webservice_open(NULL);
    // show a hint when press into 
    win_data_buffing_open(lv_layer_top());
    win_data_buffing_label_set("Loading...");
    return 0;
}
int webservice_deinit(void* args){
    // webservice_pthread_delete();
    lv_group_remove_all_objs(gp);
    lv_group_del(gp);
    lv_obj_clean(webservice_scr);
    if(win_data_buffing_is_open()){
        win_data_buffing_close();
    }
    win_msgbox_msg_close();
    return 0;
}

static void list_obj_add_checked(lv_obj_t * obj)
{
    if(obj == NULL||!lv_obj_is_valid(obj))
        return;
    lv_obj_set_style_bg_color(obj,lv_color_hex(0xffffff),0);
    lv_obj_set_style_text_color(obj,lv_color_hex(0x000000),0);
}

static void list_obj_clear_checked(lv_obj_t * obj)
{
    if(obj == NULL||!lv_obj_is_valid(obj))
        return;
    lv_obj_set_style_bg_color(obj,lv_color_hex(BLACKCOLOR_LEVEL1),0);
    lv_obj_set_style_text_color(obj,lv_color_hex(0xffffff),0);
}

static void screen_event_cb(lv_event_t* event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target = lv_event_get_target(event);
    if(code==LV_EVENT_SCREEN_LOAD_START){
        webservice_init(NULL);
        if(lv_scr_act()==webplay_scr){
            /* from webplay_scr turn into webservice scr */
            // reshow the data with webdata struct 
            main_cont_list_add_data_by_web(list_obj,webcategory_list_data_get());
            bool is_reload = (webservice_old_page_idx == grid_list_data.page_idx ? 1 : 0);
            grid_list_display_flush(is_reload,&grid_list_data,webvideo_list_data_get());
            // refocus obj which press into in last page
            objcont_group_refocus(&grid_list_data);
            webcategory_list_t* webcate_list=webcategory_list_data_get();
            target = lv_obj_get_child(list_obj, webcate_list->objsel_index);
            list_obj_add_checked(target);
            lv_obj_scroll_to_view_recursive(target, LV_ANIM_OFF);
        }else if(lv_scr_act()==main_page_scr){
            /*from main_page_scr turn into webservice scr */
            webservice_pthread_create();
        }
    }else if(code==LV_EVENT_SCREEN_LOADED){  
        // do not work in foreground
    }else if(code==LV_EVENT_SCREEN_UNLOAD_START){
        webservice_deinit(NULL);
    }else if (code==LV_EVENT_SCREEN_UNLOADED){
        if(lv_scr_act()==setup_scr||lv_scr_act()==channel_scr||lv_scr_act()==main_page_scr){
            /*when press into the setup scr or channel scr need to do something */ 
            webservice_pthread_delete();
            webservice_thumb_buffer_free();
        }
    }
    return;
}
void webservice_screen_init()
{
    webservice_scr=lv_obj_create(NULL);
    lv_obj_set_size(webservice_scr,LV_PCT(100),LV_PCT(100));
    lv_obj_add_event_cb(webservice_scr,screen_event_cb,LV_EVENT_ALL,0);
    lv_obj_set_style_pad_gap(webservice_scr,0,0);
    // lv_scr_load(webservice_scr);

    screen_entry_t webservice_entry;
    webservice_entry.screen = webservice_scr;
    webservice_entry.control = win_webservice_msg_handle;
    api_screen_regist_ctrl_handle(&webservice_entry);
}
static int objcont_group_refocus(grid_list_data_t* obj_data)
{
    lv_group_remove_all_objs(lv_group_get_default());
    for(int i=0;i<lv_obj_get_child_cnt(objcont);i++){
        lv_group_add_obj(lv_group_get_default(),lv_obj_get_child(objcont,i));                
    }
    // do focused handle in other obj
    if(lv_obj_is_valid(lv_obj_get_child(objcont,obj_data->cur_pos))){
        lv_group_focus_obj(lv_obj_get_child(objcont,obj_data->cur_pos));
    }else{
        printf("reset focus error\n");
    }
    return 0;
}
/* reset the new video list info when into diff cate */
static int webvideo_list_reset_by_webcate(void)
{
    void* iptv_hdl=hciptv_handle_get();
    hccast_iptv_handle_abort(iptv_hdl);
    webvideo_list_data_reset();
    return 0;
}
void list_obj_group_refocus(lv_obj_t* f_obj)
{
    // Make sure that the previous focus is able to delete it on its own
    for(int i=0;i<lv_obj_get_child_cnt(list_obj);i++){
        lv_group_add_obj(lv_group_get_default(),lv_obj_get_child(list_obj,i));
    }
    if(f_obj){
        lv_group_focus_obj(f_obj);
    }
}

static pagetoken_e token_state=NULLTOKEN;
void webvideo_list_token_state_set(pagetoken_e state)
{
    token_state=state;
}
static pagetoken_e webvideo_list_token_state_get()
{
    return token_state;
}

lv_obj_t* kb_obj;
lv_obj_t* ta;
lv_obj_t* ask_btn;
static uint16_t sel_id=0;
char search_str[1024]={0};
char* win_search_cont_str_get(void){
    return search_str;
}
static void  search_content_event_cb(lv_event_t* event){
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target = lv_event_get_target(event);
    uint32_t lv_key=0;
    
    if(target==kb_obj){
        if(code==LV_EVENT_KEY){
            lv_key = lv_indev_get_key(lv_indev_get_act());
            if(lv_key==LV_KEY_ESC){
                /*del kb obj and set flag*/
                /*  should delete the object before deleting the focus group
                    Otherwise, objects that have already been deleted 
                    also process LV_EVENT_CANCEL events that cause a crash  */
                lv_obj_del(kb_obj);
                lv_obj_del(lv_obj_get_parent(ta));
                kb_obj = NULL;
                list_obj_group_refocus(lv_obj_get_child(list_obj, YTB_SERARCH_CATEGORY));
                if(lv_obj_has_state(lv_obj_get_child(list_obj,YTB_SERARCH_CATEGORY),LV_STATE_PRESSED)){
                    lv_obj_clear_state(lv_obj_get_child(list_obj,YTB_SERARCH_CATEGORY),LV_STATE_PRESSED);
                }
                webservice_msg_flag = true;
            }else if(lv_key==LV_KEY_UP){
                lv_btnmatrix_t *btnm = &((lv_keyboard_t*)target)->btnm;
                if(btnm->button_areas[sel_id].y1 == btnm->button_areas[0].y1){
                    lv_group_focus_obj(ask_btn); 
                }
            }
            if(kb_obj)
                sel_id = ((lv_keyboard_t*)target)->btnm.btn_id_sel;
        }else if(code==LV_EVENT_PRESSED){
            const char * txt = lv_btnmatrix_get_btn_text(kb_obj, lv_btnmatrix_get_selected_btn(kb_obj));
            if (strcmp(txt, LV_SYMBOL_OK) == 0){
                char * obj_str=lv_textarea_get_text(ta);
                if(obj_str){
                    int size=strlen(obj_str);
                    if(size>1024) {
                        printf("too long string\n");
                        return ;
                    }
                    memset(search_str,0,1024);
                    memcpy(search_str,obj_str,strlen(obj_str));
                    lv_obj_del(kb_obj);
                    lv_obj_del(lv_obj_get_parent(ta));
                    if(search_str){
                        /**/ 
                        printf(">>! search keyword:%s \n",search_str);
                        webservice_msg_t msg;
                        msg.type=MSG_VIDOE_SEARCH_ACK;
                        msg.code=NULL;
                        uint32_t msgid=webservice_pthread_msgid_get();
                        api_message_send(msgid,&msg,sizeof(webservice_msg_t));
                        win_data_buffing_close();
                        win_data_buffing_open(lv_layer_top());
                        win_data_buffing_label_set("Loading...");
                    }
                    list_obj_group_refocus(lv_obj_get_child(list_obj, YTB_SERARCH_CATEGORY));
                }
            }
        }

    }else if(target==ask_btn){
        if(code==LV_EVENT_KEY){
            lv_key = lv_indev_get_key(lv_indev_get_act());
            if(lv_key==LV_KEY_ESC){
                lv_obj_del(kb_obj);
                lv_obj_del(lv_obj_get_parent(ta));
                kb_obj = NULL;
                list_obj_group_refocus(lv_obj_get_child(list_obj, YTB_SERARCH_CATEGORY));
                if(lv_obj_has_state(lv_obj_get_child(list_obj,YTB_SERARCH_CATEGORY),LV_STATE_PRESSED)){
                    lv_obj_clear_state(lv_obj_get_child(list_obj,YTB_SERARCH_CATEGORY),LV_STATE_PRESSED);
                }
                webservice_msg_flag = true;
            }else if(lv_key==LV_KEY_DOWN){
                lv_group_focus_obj(kb_obj);
            }
        }else if(code==LV_EVENT_PRESSED){
            char * obj_str=lv_textarea_get_text(ta);
            int size=strlen(obj_str);
            if(size>1024) {
                printf("too long string\n");
                return ;
            }
            memset(search_str,0,1024);
            memcpy(search_str,obj_str,strlen(obj_str));
            lv_obj_del(kb_obj);
            lv_obj_del(lv_obj_get_parent(ta));
            if(search_str){
                /**/ 
                printf(">>! search keyword:%s \n",search_str);
                webservice_msg_t msg;
                msg.type=MSG_VIDOE_SEARCH_ACK;
                msg.code=NULL;
                uint32_t msgid=webservice_pthread_msgid_get();
                api_message_send(msgid,&msg,sizeof(webservice_msg_t));
                win_data_buffing_close();
                win_data_buffing_open(lv_layer_top());
                win_data_buffing_label_set("Loading...");
            }
            list_obj_group_refocus(lv_obj_get_child(list_obj, YTB_SERARCH_CATEGORY));
        }
    }
    return ;
}

static int win_search_content_create(lv_obj_t* p)
{
    lv_group_remove_all_objs(lv_group_get_default());
    kb_obj = lv_keyboard_create(p);
    lv_obj_set_style_text_font(kb_obj, &lv_font_montserrat_28, 0);
   
    lv_obj_set_style_bg_color(kb_obj, lv_color_make(81, 100, 117), 0);
    lv_obj_set_size(kb_obj,LV_PCT(100),LV_PCT(46));
    lv_obj_align(kb_obj, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(kb_obj, search_content_event_cb, LV_EVENT_ALL, NULL);
    
    lv_keyboard_t *keyb = (lv_keyboard_t *)kb_obj;
    lv_obj_t *btns = (lv_obj_t*)(&keyb->btnm);
    lv_obj_set_style_bg_opa(btns, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(btns, lv_palette_darken(LV_PALETTE_GREY, 1), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(btns, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(btns, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(btns, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(btns, lv_color_white(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_pad_hor(btns, lv_disp_get_hor_res(lv_disp_get_default())/10, 0);
    
    lv_obj_t * cont=lv_obj_create(p);
    lv_obj_align_to(cont,kb_obj, LV_ALIGN_OUT_TOP_LEFT, 0, 50);
    lv_obj_set_size(cont,lv_pct(100),lv_pct(10));
    lv_obj_set_style_pad_ver(cont,5,0);
    ta = lv_textarea_create(cont);
    lv_obj_set_align(ta,LV_ALIGN_LEFT_MID);
    lv_obj_set_size(ta,lv_pct(93),lv_pct(100));
    lv_obj_set_style_text_font(ta,&lv_font_montserrat_28,0);
    lv_obj_set_style_text_align(ta,LV_ALIGN_CENTER,0);
    lv_textarea_set_placeholder_text(ta, "Please input keyword!");
    lv_keyboard_set_textarea(kb_obj,ta);
    ask_btn=lv_btn_create(cont);
    lv_obj_set_align(ask_btn,LV_ALIGN_RIGHT_MID);
    lv_obj_set_size(ask_btn,lv_pct(6),LV_PCT(100));
    lv_obj_set_style_text_font(ask_btn,&lv_font_montserrat_28,0);
    lv_obj_set_style_text_align(ask_btn,LV_ALIGN_CENTER,0);
    lv_obj_set_style_outline_width(ask_btn,6,LV_STATE_FOCUS_KEY);
    lv_obj_add_event_cb(ask_btn,search_content_event_cb,LV_EVENT_ALL,NULL);
    lv_obj_t* label=lv_label_create(ask_btn);
    lv_label_set_text(label,LV_SYMBOL_OK);
    lv_obj_center(label);

    sel_id=0;
    return 0;
}

static lv_obj_t *menu_obj;

static void menu_obj_del(void)
{
    hciptv_ytb_app_config_t *ytb_app_config;
    ytb_app_config = sysdata_get_iptv_app_config();
    hccast_iptv_app_config_st hccast_iptv_config = {0};
    hccast_iptv_config_get(inst, &hccast_iptv_config);
    bool region_modify_flag = 0;
    webcategory_list_t* webcate_list=webcategory_list_data_get();

    lv_obj_t *drop_obj = lv_obj_get_child(lv_obj_get_child(menu_obj, 0), 1);
    int sel = lv_dropdown_get_selected(drop_obj);
    if(!strlen(hccast_iptv_config.region) || strcmp(ytb_app_config->iptv_config.region,g_region_code[sel].reg)){
        printf("modify Region : %s ", ytb_app_config->iptv_config.region);
        snprintf(ytb_app_config->iptv_config.region, sizeof(ytb_app_config->iptv_config.region), "%s", g_region_code[sel].reg);
        snprintf(hccast_iptv_config.region, sizeof(hccast_iptv_config.region), "%s", g_region_code[sel].reg);
        hccast_iptv_config_set(inst, &hccast_iptv_config);
        printf("-> %s\n", hccast_iptv_config.region);
        projector_sys_param_save();
        if(webcate_list->objsel_index != YTB_SERARCH_CATEGORY && webcate_list->objsel_index != YTB_HISTORY_CATEGORY )
            region_modify_flag = 1;
    }

    drop_obj = lv_obj_get_child(lv_obj_get_child(menu_obj, 1), 1);
    sel = lv_dropdown_get_selected(drop_obj);
    if(ytb_app_config->iptv_config.quality_option != g_quality_code[sel].qual){
        printf("modify Quality : %d ", ytb_app_config->iptv_config.quality_option);
        ytb_app_config->iptv_config.quality_option = g_quality_code[sel].qual;
        hccast_iptv_config.quality_option = ytb_app_config->iptv_config.quality_option;
        hccast_iptv_config_set(inst, &hccast_iptv_config);
        printf("-> %d\n", hccast_iptv_config.quality_option);
        projector_sys_param_save();
    }

    drop_obj = lv_obj_get_child(lv_obj_get_child(menu_obj, 2), 1);
    sel = lv_dropdown_get_selected(drop_obj);
    if(ytb_app_config->iptv_config.search_option != g_search_option[sel].sear){
        printf("modify Search_option : %d ", ytb_app_config->iptv_config.search_option);
        ytb_app_config->iptv_config.search_option = g_search_option[sel].sear;
        printf("-> %d\n", ytb_app_config->iptv_config.search_option);
        projector_sys_param_save();
    }

    /*should delete the object before deleting the focus group*/
    lv_obj_del(menu_obj);

    if(region_modify_flag){
        lv_group_remove_all_objs(lv_group_get_default());
        list_obj_group_refocus(lv_obj_get_child(list_obj, webcate_list->objsel_index));

        webservice_pic_download_stop();
        hccast_iptv_handle_abort(inst);
        webservice_msg_flag = false;
        uint32_t msgid = webservice_pthread_msgid_get();
        char* cate_id = NULL;
        webservice_msg_t msg;
        msg.type = MSG_VIDEOLIST_REFRESH;
        if(webcate_list->objsel_index>=PREBUILD_CATE_CNT)
            cate_id = webcategory_list_strid_get_by_index(webcate_list->objsel_index-PREBUILD_CATE_CNT);
        msg.code = cate_id;
        api_message_send(msgid,&msg,sizeof(webservice_msg_t));
        if(!win_data_buffing_is_open()){
            win_data_buffing_open(lv_layer_top());
            win_data_buffing_label_set("Loading...");
        }
    }else{
        if(lv_group_get_obj_count(lv_group_get_default()) == SUBCONT)
            objcont_group_refocus(grid_list_data_get());
        else{
            lv_group_remove_all_objs(lv_group_get_default());
            list_obj_group_refocus(lv_obj_get_child(list_obj, webcate_list->objsel_index));
        }
    }
}

static void event_handler_menu_obj(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = lv_event_get_target(e);
    lv_obj_t *drop_obj = lv_obj_get_child(target, 1);
    int index = lv_obj_get_index(target);
    if(code == LV_EVENT_KEY){
        uint32_t lv_key = lv_indev_get_key(lv_indev_get_act());
        uint32_t user_vkey=key_convert_vkey(lv_key);
        static char parem;

        switch(user_vkey){
            case V_KEY_ENTER:
                if(lv_dropdown_is_open(drop_obj)){
                    parem = LV_KEY_ENTER;
                    lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
                }else
                    lv_dropdown_open(drop_obj);
                break;
            case V_KEY_UP:
                if(lv_dropdown_is_open(drop_obj)){
                    parem = LV_KEY_UP;
                    lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
                }else{
                    index = (index - 1 + Y2B_MENU_CNT) % Y2B_MENU_CNT;
                    lv_group_focus_obj(lv_obj_get_child(target->parent, index));
                }
                break;
            case V_KEY_DOWN:
                if(lv_dropdown_is_open(drop_obj)){
                    parem = LV_KEY_DOWN;
                    lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
                }else{
                    index = (index + 1 + Y2B_MENU_CNT) % Y2B_MENU_CNT;
                    lv_group_focus_obj(lv_obj_get_child(target->parent, index));
                }
                break;
            case V_KEY_MENU:
            case V_KEY_EXIT:
                if(lv_dropdown_is_open(drop_obj)){
                    parem = LV_KEY_ESC;
                    lv_event_send(drop_obj, LV_EVENT_KEY, &parem);
                }else{
                    menu_obj_del();
                }
                break;
        }
    }
}

void create_menu_cont(lv_obj_t* p)
{
    lv_obj_t* btn[Y2B_MENU_CNT];
    lv_obj_t* label[Y2B_MENU_CNT];
    lv_obj_t* drop_obj[Y2B_MENU_CNT];
    lv_obj_t* list;
    int sel = -1, i = 0;
    int menu_str[] = {STR_REGION, STR_QUALITY, STR_SEARCH_SOET_BY};

    menu_obj = lv_obj_create(p);
    lv_obj_set_size(menu_obj, lv_pct(35), lv_pct(9 * Y2B_MENU_CNT));
    lv_obj_set_pos(menu_obj, lv_pct(20), lv_pct(100 - 8 - 9 * Y2B_MENU_CNT));
    lv_obj_set_style_radius(menu_obj, 0, 0);
    lv_obj_set_style_pad_all(menu_obj, 0, 0);
    lv_obj_set_style_bg_color(menu_obj, lv_color_hex(BLACKCOLOR_LEVEL1), 0);
    lv_obj_set_style_outline_width(menu_obj, 1, 0);
    lv_obj_set_style_outline_color(menu_obj, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_outline_opa(menu_obj, LV_OPA_100, 0);
    lv_obj_set_flex_flow(menu_obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(menu_obj, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_SPACE_BETWEEN);
    lv_obj_set_scrollbar_mode(menu_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(menu_obj, LV_OBJ_FLAG_SCROLLABLE);
    for (i = 0; i < Y2B_MENU_CNT; i++)
    {
        btn[i] = lv_btn_create(menu_obj);
        lv_obj_set_size(btn[i], lv_pct(100), lv_pct(100 / Y2B_MENU_CNT));
        lv_obj_add_event_cb(btn[i], event_handler_menu_obj, LV_EVENT_ALL, 0);
        lv_obj_set_style_bg_color(btn[i], lv_color_hex(BLACKCOLOR_LEVEL1), 0);
        lv_obj_set_style_text_color(btn[i], lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_align(btn[i], LV_ALIGN_CENTER, 0);
        lv_obj_set_style_border_side(btn[i], LV_BORDER_SIDE_FULL, 0);
        lv_obj_set_style_border_width(btn[i], 3, 0);
        lv_obj_set_style_border_color(btn[i], lv_palette_lighten(LV_PALETTE_RED, 1), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_border_color(btn[i], lv_color_hex(BLACKCOLOR_LEVEL1), 0);
        lv_obj_set_style_outline_width(btn[i], 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_radius(btn[i], 0, 0);

        label[i] = lv_label_create(btn[i]);
        lv_obj_set_style_text_font(label[i], osd_font_get(FONT_NORMAL), 0);
        lv_label_set_text(label[i], api_rsc_string_get(menu_str[i]));
        lv_obj_set_align(label[i], LV_ALIGN_LEFT_MID);
        lv_obj_set_size(label[i], lv_pct(50), lv_pct(100));
        lv_obj_set_style_text_align(label[i], LV_TEXT_ALIGN_CENTER, 0);

        drop_obj[i] = lv_dropdown_create(btn[i]);
        lv_group_remove_obj(drop_obj[i]);
        lv_obj_set_align(drop_obj[i], LV_ALIGN_RIGHT_MID);
        lv_obj_set_size(drop_obj[i], LV_PCT(50), LV_PCT(100));
        lv_dropdown_clear_options(drop_obj[i]);
        lv_obj_set_style_pad_ver(drop_obj[i], 0, 0);
        lv_obj_set_style_border_width(drop_obj[i], 0, 0);
        lv_dropdown_set_dir(drop_obj[i], LV_DIR_RIGHT);
        lv_dropdown_set_symbol(drop_obj[i], NULL);
        lv_obj_set_style_text_color(drop_obj[i], lv_color_white(), 0);
        if(i == 1)
            lv_obj_set_style_text_font(drop_obj[i], &YTB_MENU_FONT, 0);
        else
            lv_obj_set_style_text_font(drop_obj[i], osd_font_get(FONT_NORMAL), 0);
        lv_obj_set_style_bg_opa(drop_obj[i], LV_OPA_0, LV_PART_MAIN);
        lv_obj_set_style_text_align(drop_obj[i], LV_TEXT_ALIGN_CENTER, 0);

        list = lv_dropdown_get_list(drop_obj[i]);
        if(i == 1)
            lv_obj_set_style_text_font(list, &YTB_MENU_FONT, 0);
        else
            lv_obj_set_style_text_font(list, osd_font_get(FONT_NORMAL), 0);
        lv_obj_set_style_bg_color(list, lv_color_hex(BLACKCOLOR_LEVEL1), 0);
        lv_obj_set_style_text_color(list, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_color(list, lv_color_hex(0x000000), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_color(list, lv_color_hex(0xffffff), LV_STATE_FOCUS_KEY);
    }

    /*Region*/
    hciptv_ytb_app_config_t *ytb_app_config;
    ytb_app_config = sysdata_get_iptv_app_config();
    hccast_iptv_app_config_st hccast_iptv_config = {0};
    hccast_iptv_config_get(inst, &hccast_iptv_config);
    for (i = 0; i < sizeof(g_region_code) / sizeof(g_region_code[0]); i++)
    {
        lv_dropdown_add_option(drop_obj[0], api_rsc_string_get(g_region_code[i].desc), LV_DROPDOWN_POS_LAST);
        if (!strcmp(hccast_iptv_config.region, g_region_code[i].reg))
            sel = i;
    }
    if (sel == -1)
    {
        printf("not find region, default is the first one\n");
        sel = 0;
    }
    lv_dropdown_set_selected(drop_obj[0], sel);

    /*Quality*/
    sel = -1;
    for (int i = 0; i < sizeof(g_quality_code) / sizeof(g_quality_code[0]); i++)
    {
        lv_dropdown_add_option(drop_obj[1], g_quality_code[i].desc, LV_DROPDOWN_POS_LAST);
        if (hccast_iptv_config.quality_option == g_quality_code[i].qual)
            sel = i;
    }
    if (sel == -1)
    {
        printf("not find quality, default is the first one\n");
        sel = 0;
    }
    lv_dropdown_set_selected(drop_obj[1], sel);

    /*Search Sort*/
    sel = -1;
    for (int i = 0; i < sizeof(g_search_option) / sizeof(g_search_option[0]); i++)
    {
        lv_dropdown_add_option(drop_obj[2], api_rsc_string_get(g_search_option[i].desc), LV_DROPDOWN_POS_LAST);
        if (ytb_app_config->iptv_config.search_option == g_search_option[i].sear)
            sel = i;
    }
    if (sel == -1)
    {
        printf("not find search_option, default is the first one\n");
        sel = 0;
    }
    lv_dropdown_set_selected(drop_obj[2], sel);

    lv_group_focus_obj(lv_obj_get_child(menu_obj, 0));
}

static char* ytb_history_video_id_get(void)
{
    hciptv_ytb_app_config_t *ytb_app_config;
    ytb_app_config = sysdata_get_iptv_app_config();
    memset(ytb_history_cate_id, 0, sizeof(ytb_history_cate_id));
    int total_num = ytb_app_config->ytb_history_video.total_index;
    int i = ytb_app_config->ytb_history_video.new_index - 1;

    if (total_num == 0)
        return NULL;
    if (i < 0)
        i = MAX_YTB_HISTORY_VIDEO_SAVE - 1;

    while (total_num)
    {
        strncat(ytb_history_cate_id, ytb_app_config->ytb_history_video.history_info[i].id, sizeof(ytb_app_config->ytb_history_video.history_info[i].id));
        strncat(ytb_history_cate_id, ",", 1);
        i--;
        if (i < 0)
            i = MAX_YTB_HISTORY_VIDEO_SAVE - 1;
        total_num--;
    }
    return ytb_history_cate_id;
}

void ytb_history_video_id_save(char* video_id)
{
    hciptv_ytb_app_config_t *ytb_app_config;
    ytb_app_config = sysdata_get_iptv_app_config();
    int index = ytb_app_config->ytb_history_video.new_index;
    ytb_app_config->ytb_history_video.new_index++;
    int total_num = ytb_app_config->ytb_history_video.total_index;
    int j = index - 1;
    int i;
    bool cmp_flag = false;
    ytb_app_config->ytb_history_video.new_index = ytb_app_config->ytb_history_video.new_index % MAX_YTB_HISTORY_VIDEO_SAVE;

    // Filter for the same video ID
    while (total_num)
    {
        if (cmp_flag || strcmp(ytb_app_config->ytb_history_video.history_info[j].id,video_id) == 0)
        {
            i = j - 1;
            if (i < 0)
                i = MAX_YTB_HISTORY_VIDEO_SAVE - 1;
            strcpy(ytb_app_config->ytb_history_video.history_info[j].id, ytb_app_config->ytb_history_video.history_info[i].id);
            cmp_flag = true;
        }
        j--;
        if (j < 0)
            j = MAX_YTB_HISTORY_VIDEO_SAVE - 1;
        total_num--;
    }

    if (ytb_app_config->ytb_history_video.total_index < MAX_YTB_HISTORY_VIDEO_SAVE && !cmp_flag)
        ytb_app_config->ytb_history_video.total_index++;

    snprintf(ytb_app_config->ytb_history_video.history_info[index].id, sizeof(ytb_app_config->ytb_history_video.history_info[index].id), "%s", video_id);
    projector_sys_param_save();
}

static void event_handler_list_btn_item(lv_event_t* event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target = lv_event_get_target(event);
    int obj_index=lv_obj_get_index(target);
    if(code==LV_EVENT_FOCUSED){
        printf("obj has focused\n");
    }else if(code==LV_EVENT_KEY){
        //添加上下左右的按键控制
        //1 自行添加状态 2、管理组内成员
        uint32_t lv_key = lv_indev_get_key(lv_indev_get_act());
        uint32_t user_vkey=key_convert_vkey(lv_key);
        grid_list_data_t * obj_data=grid_list_data_get();
        webvideo_list_t* webvideo_list=webvideo_list_data_get();
        switch(user_vkey){
            case V_KEY_UP:
                lv_obj_scroll_to_view(target,LV_ANIM_OFF);
                lv_group_focus_prev(lv_group_get_default());
                break;
            case V_KEY_DOWN:
                lv_obj_scroll_to_view(target,LV_ANIM_OFF);
                lv_group_focus_next(lv_group_get_default());
                break;
            case V_KEY_LEFT:

                break;
            case V_KEY_RIGHT:
                // if webvideo_list data refr ready? 
                if(webvideo_list->info_list && webvideo_list->info_list->info_total_num > 0){
                    objcont_group_refocus(obj_data);
                }
                break;
            case V_KEY_MENU:
                create_menu_cont(lv_layer_top());
                break;
            case V_KEY_EXIT:
                /* PRESS ESC TO MAIN_PAGE */
                change_screen(SCREEN_CHANNEL_MAIN_PAGE);
                break;
        }
    }else if(code==LV_EVENT_PRESSED){
        if (!app_wifi_connect_status_get()){
            win_msgbox_msg_open(STR_WIFI_NOT_CONNECTING, WIN_WEB_MSG_TIMEOUT, NULL, NULL);
            return;
        }

        // wait for msg return
        if(webservice_msg_flag==true){
            webcategory_list_t* webcate_list=webcategory_list_data_get();
            lv_obj_t *target_index;

            if(webcate_list->objsel_index >= 0){
                target_index = lv_obj_get_child(list_obj, webcate_list->objsel_index);
                list_obj_clear_checked(target_index);
            }
            list_obj_add_checked(target);
            webcate_list->objsel_index=obj_index;
            
            if(obj_index==YTB_SERARCH_CATEGORY){
                // search function
                win_msgbox_msg_close();
                if(win_data_buffing_is_open())
                    win_data_buffing_close();
                win_search_content_create(lv_layer_top());
            }else{
                uint32_t msgid=webservice_pthread_msgid_get();
                char* cate_id=NULL;
                webservice_msg_t msg;
                msg.type=MSG_VIDEOLIST_REFRESH;
                if(obj_index==YTB_HISTORY_CATEGORY){
                    cate_id = ytb_history_video_id_get();
                    if(!cate_id){
                        win_msgbox_msg_open(STR_NO_HISTORY,2000,NULL,NULL);
                        return;
                    }
                    msg.type=MSG_HISTORY_VIDEOLIST_REFRESH;
                }else if(obj_index>=PREBUILD_CATE_CNT){
                    cate_id=webcategory_list_strid_get_by_index(obj_index-PREBUILD_CATE_CNT);
                }
                msg.code=cate_id;
                api_message_send(msgid,&msg,sizeof(webservice_msg_t));
                win_msgbox_msg_close();
                win_data_buffing_open(lv_layer_top());
                win_data_buffing_label_set("Loading...");
            }
            webservice_msg_flag = false;
        }
        else
            printf(">>! Invalid value\n");
    }else if(code==LV_EVENT_REFRESH){
        void* objmsg_code=lv_event_get_param(event);
    }
    return;
}

videolink_ask_e tmp_ask;
static void event_handler_sobjcont_item(lv_event_t* event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target = lv_event_get_target(event);
    int sobj_index=lv_obj_get_index(target);
    webservice_msg_t msg;
    uint32_t msgid=webservice_pthread_msgid_get();
    pagetoken_e m_token=webvideo_list_token_state_get();

    if(code==LV_EVENT_KEY){
        uint32_t lv_key = lv_indev_get_key(lv_indev_get_act());
        uint32_t user_vkey=key_convert_vkey(lv_key);
        //disable the group focus when take pagetoken
        if(lv_key==LV_KEY_ESC){
            /*return to mainpage*/
            change_screen(SCREEN_CHANNEL_MAIN_PAGE);
            return;
        }else if(m_token==PREVPAGETOKEN||m_token==NEXTPAGETOKEN||lv_key==LV_KEY_ENTER){
            //disable the keyevent into when take pagetoken
            return;
        }else if(lv_key==LV_KEY_LEFT){
            if(sobj_index==0||sobj_index==2){
                lv_group_remove_all_objs(lv_group_get_default());
                for(int i=0;i<lv_obj_get_child_cnt(list_obj);i++){
                    lv_group_add_obj(lv_group_get_default(),lv_obj_get_child(list_obj,i));
                }
                // need to refocus index in list obj 
                webcategory_list_t* webcate_list=webcategory_list_data_get();
                lv_group_focus_obj(lv_obj_get_child(list_obj,webcate_list->objsel_index));
                lv_obj_invalidate(webservice_scr);
                return ;
            } 
        }else if(user_vkey==V_KEY_MENU){
            create_menu_cont(lv_layer_top());
            return;
        }
        pagetoken_e is_vlist_update=grid_list_key_ctrl(NULL,lv_key,webvideo_list_data_get());
        if(is_vlist_update==NEXTPAGETOKEN){
            /* need to reset grid list data and clear grid list view
            grid_list_data_reset just reset grid_list->list cnt number ,link whth webvideo list
            webvideo_list_data_reset just reset webvideo_list-> list number 
            and disabale key*/
            webvideo_list_token_state_set(NEXTPAGETOKEN);
            msg.type=MSG_VIDEOLIST_NEXTPAGE_ACK;
            msg.code=NULL;
            api_message_send(msgid,&msg,sizeof(webservice_msg_t));
            win_data_buffing_open(lv_layer_top());
            win_data_buffing_label_set("Loading...");
        }else if(is_vlist_update==PREVPAGETOKEN){
            webvideo_list_token_state_set(PREVPAGETOKEN);
            msg.type=MSG_VIDEOLIST_PREVPAGE_ACK;
            msg.code=NULL;
            api_message_send(msgid,&msg,sizeof(webservice_msg_t));
            win_data_buffing_open(lv_layer_top());
            win_data_buffing_label_set("Loading...");
        }
        // for test 
    }else if(code==LV_EVENT_PRESSED && m_token==NULLTOKEN){
        if (app_wifi_connect_status_get()){
            msg.type=MSG_VIDEOLINK_ACK;
            tmp_ask=CURRRENT_LINK_ASK;
            msg.code=&tmp_ask;
            webservice_old_page_idx = grid_list_data.page_idx;
            api_message_send(msgid,&msg,sizeof(webservice_msg_t));
            printf("lvgl pthread had send message to web pthread\n");
            webplayer_handle_create(); // create a player handle before the video link data get success
            change_screen(SCREEN_WEBPLAYER);
        }else{
            win_msgbox_msg_open(STR_WIFI_NOT_CONNECTING, WIN_WEB_MSG_TIMEOUT, NULL, NULL);
        }
    }else if (code==LV_EVENT_REFRESH){
        control_msg_t * msg=lv_event_get_param(event);
        if(msg->msg_type==MSG_TYPE_WEBSERVICE_START){
            grid_list_data_init(webvideo_list_data_get());
            grid_list_display_flush(false,&grid_list_data,webvideo_list_data_get());
        }else if(msg->msg_type==MSG_TYPE_WEBSERVICE_LOADED){
            // 翻页或者切换目录 切换目录需要初始化grid_list /翻页不需要
            if(msg->msg_code==WEBVIDEO_NEXTPAGETOKEN_SUCCESS||msg->msg_code==WEBVIDEO_PREVPAGETOKEN_SUCCESS){
                webvideo_list_t* webvideo_list=webvideo_list_data_get();
                pagetoken_e tmp=webvideo_list_token_state_get();
                if(tmp==PREVPAGETOKEN||tmp==NEXTPAGETOKEN){
                    webvideo_list_token_state_set(NULLTOKEN);
                }
                if(webvideo_list->is_eoh){
                    grid_list_data_reset_by_pagetoken(webvideo_list_data_get(),tmp);
                    lv_obj_t* pagelabel=lv_obj_get_child(top_cont,1);
                    if (grid_list_data.page_cnt != 0)
                        lv_label_set_text_fmt(pagelabel,"[%ld / %ld]",grid_list_data.page_idx,grid_list_data.page_cnt);
                    webvideo_list->is_eoh = false;
                    return;
                }
                if(webvideo_list->info_list->info_num<(objcont,grid_list_data.cur_pos+1)){
                    grid_list_data.cur_pos = webvideo_list->info_list->info_num - 1;
                }
                lv_group_focus_obj(lv_obj_get_child(objcont,grid_list_data.cur_pos));
                grid_list_display_flush(false,&grid_list_data,webvideo_list_data_get());
            }else if(msg->msg_code==WEBVIDEO_NEXTPAGETOKEN_ERROR||msg->msg_code==WEBVIDEO_PREVPAGETOKEN_ERROR){
                // to do something when download error
                pagetoken_e tmp=webvideo_list_token_state_get();
                grid_list_data_reset_by_pagetoken(webvideo_list_data_get(),tmp);
                if(tmp==PREVPAGETOKEN||tmp==NEXTPAGETOKEN){
                    webvideo_list_token_state_set(NULLTOKEN);
                }

                printf(">>! video_info get error,please retry\n");
                win_msgbox_msg_open(STR_VIDEO_INFO_ERR_RETRY_AGAIN,WIN_WEB_MSG_TIMEOUT,NULL,NULL);
            }else if(msg->msg_code==WEBVIDEO_INFO_GET_SUCCESS||msg->msg_code==WEBVIDEO_SEARCH_GET_SUCCESS){
                /*for test ,切换目录成功后reset webvideo list中的值 */
                webvideo_list_data_reset();
                grid_list_data_init(webvideo_list_data_get());
                /* For searches, the info_total_num are incorrect, while page_cnt == 0  is not displayed */
                if (msg->msg_code==WEBVIDEO_SEARCH_GET_SUCCESS){
                    if (grid_list_data.page_cnt >= 250000){
                        grid_list_data.page_cnt = 0;
                    }
                }
                grid_list_display_flush(false,&grid_list_data,webvideo_list_data_get());
            }
        }
    }else if(code==LV_EVENT_DEFOCUSED){

    }else if(code==LV_EVENT_FOCUSED){
        lv_obj_invalidate(webservice_scr);  
    }

    return ;
}

static char* webservice_thumb_data_select(int sel_node,char* quality,webvideo_list_t* webvideo_list)
{
    char * thumb_url=NULL;
    // printf(">>! %s(), line:%d, sel_node=%d,offset=%d\n",__func__,__LINE__,sel_node,webvideo_list->offset);
    hccast_iptv_info_node_st* node =(hccast_iptv_info_node_st*)list_nth_data(&webvideo_list->info_list->list,sel_node-webvideo_list->offset);
    if(node){
        for(int i=0;i<HCCAST_IPTV_THUMB_MAX;i++){
            if(strstr(node->thumb[i].quality,quality)){
                thumb_url=node->thumb[i].url;
                return thumb_url;
            }
        }
    }
    return thumb_url;
}


static int grid_list_display_flush_by_thumbnail(int id)
{
    thumbnail_info_t* info_arr=webservice_thumb_buffer_get();
    if(info_arr[id].is_data_ready == false){
        return -1;
    }
    info_arr[id].lv_img_dsc.header.cf = LV_IMG_CF_RAW;
    info_arr[id].lv_img_dsc.header.always_zero = 0;
    info_arr[id].lv_img_dsc.header.reserved = 0;
    info_arr[id].lv_img_dsc.header.w = 320;
    info_arr[id].lv_img_dsc.header.h = 180;
    if(info_arr[id].img_obj&&lv_obj_is_valid(info_arr[id].img_obj)){
        lv_img_set_src(info_arr[id].img_obj,&info_arr[id].lv_img_dsc);
    }
    //lv_obj_invalidate(webservice_scr);
    return 0;
}

// do not show lvgl here because this callback belong other network downloading pthread 
// goto this callback when downloading pthread run
static void Net_PicDownloaded_cb(void *user_data,int data_length)
{
    thumbnail_info_t* signle_thumb = (thumbnail_info_t *)user_data;
    int i=signle_thumb->obj_idx;    //form 0 to 3
    thumb_info[i].lv_img_dsc.data_size=data_length;
    thumb_info[i].is_data_loaded=true;
    thumb_info[i].is_data_ready=true;

    control_msg_t msg;
    msg.msg_type=MSG_TYPE_NETDOWNLOAD;
    msg.msg_code=signle_thumb->obj_idx;
    api_control_send_msg(&msg);
    //printf("%s(), line:%d. index: %d\n", __func__, __LINE__, i);
}
void* webservice_thumb_buffer_get(void)
{
    return thumb_info;
}
void webservice_thumb_buffer_free(void)
{
    for(int i=0;i<SUBCONT;i++){
        if(thumb_info[i].lv_img_dsc.data!=NULL){
            free(thumb_info[i].lv_img_dsc.data);
            thumb_info[i].lv_img_dsc.data=NULL;
        }
    }
}

int webservice_pic_downloaded(webvideo_list_t* webvideo_list)
{
    grid_list_data_t* gridlist=grid_list_data_get();
    for(int i=0;i<SUBCONT;i++){
        if(thumb_info[i].lv_img_dsc.data==NULL){
            thumb_info[i].lv_img_dsc.data=(uint8_t*)malloc(MAXTHUNB_DATASIZE);
        }
        thumb_info[i].top=gridlist->top;
        thumb_info[i].obj_idx=i;
        thumb_info[i].is_data_loaded=false;
        int weblist_index=thumb_info[i].top+i;
        // if(weblist_index>) to do something 
        char * pic_url=webservice_thumb_data_select(thumb_info[i].top+i,THUMBNAIL_QUALITY,webvideo_list);
        if(pic_url){
            thumb_info[i].curl_handle=api_network_download_start(pic_url, NULL, thumb_info[i].lv_img_dsc.data, MAXTHUNB_DATASIZE, Net_PicDownloaded_cb, (void*)&thumb_info[i],false);
        }
    }
    return 0; 
}

/**
 * @description: stop all the thumbnail pic downloaded pthread which if run or not run 
 */
int webservice_pic_download_stop(void)
{
    pthread_mutex_lock(&pic_download_lock);
    for(int i=0;i<SUBCONT;i++){
        if(thumb_info[i].curl_handle!=NULL){
            api_network_download_flag_stop(thumb_info[i].curl_handle);
        }
    }
    for(int i=0;i<SUBCONT;i++){
        if(thumb_info[i].curl_handle!=NULL){
            //api_network_download_stop(thumb_info[i].curl_handle);
            api_network_download_pthread_stop(thumb_info[i].curl_handle);
            thumb_info[i].curl_handle=NULL;
        }
    }
    pthread_mutex_unlock(&pic_download_lock);
    return 0;
}

void webservice_pic_flush_by_thumb_info(void)
{
    // to do somethig when turninto page after playing video  
    for(int i=0;i<SUBCONT;i++){
        if(thumb_info[i].is_data_loaded==true){
            grid_list_display_flush_by_thumbnail(i);
        }else{
            // dont stop single curl handle,beacuse it has stop all the curl handle in player 
            // api_network_download_stop(thumb_info[i].curl_handle);
            // thumb_info[i].curl_handle=NULL;
            char * pic_url=webservice_thumb_data_select(thumb_info[i].top+i,THUMBNAIL_QUALITY,webvideo_list_data_get());
            if(pic_url){
                thumb_info[i].is_data_ready = false;
                thumb_info[i].curl_handle=api_network_download_start(pic_url, NULL, thumb_info[i].lv_img_dsc.data, MAXTHUNB_DATASIZE, Net_PicDownloaded_cb, (void*)&thumb_info[i],false);
            }
        }
    }
}

// LVLG 收到网络服务线程数据后的消息回调函数
static void win_webservice_msg_handle(void* arg1 ,void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    uint32_t msg_code=ctl_msg->msg_code;
    printf(">>! into web msg handle,msg_code:%ld\n",msg_code);
    if(lv_scr_act()!=webservice_scr)
        return ;
    if(ctl_msg->msg_type==MSG_TYPE_WEBSERVICE_START){
        if(msg_code==WEBCATEGORY_LIST_GET_SUCCESS){
            if(lv_obj_is_valid(list_obj)){
                webcategory_list_t* webcate_list=webcategory_list_data_get();
                main_cont_list_add_data_by_web(list_obj,webcate_list);
                webcate_list->objsel_index=YTB_TRENDING_CATEGORY;
                list_obj_add_checked(lv_obj_get_child(list_obj,webcate_list->objsel_index));
                // for focus obj items.
            }
        }else if(msg_code==WEBCATEGORY_LIST_GET_ERROR){
            if(win_data_buffing_is_open()){
                win_data_buffing_close();
            }
            printf(">>! web service error \n");
        }else if(msg_code==WEBVIDEO_INFO_GET_SUCCESS){
            if(win_data_buffing_is_open()){
                win_data_buffing_close();
            }
            lv_event_send(lv_obj_get_child(objcont,0),LV_EVENT_REFRESH,ctl_msg);
            webservice_msg_flag = true;
        }else if(msg_code==WEBVIDEO_INFO_GET_ERROR){
            if(win_data_buffing_is_open()){
                win_data_buffing_close();
            }
            win_msgbox_msg_open(STR_VIDEO_INFO_ERR_RETRY,WIN_WEB_MSG_TIMEOUT,NULL,NULL);
            webservice_msg_flag = true;
        }else if(msg_code==WEBVIDEO_INFO_GET_ABORT){
            /*nothing to do*/
        }
    }else if(ctl_msg->msg_type==MSG_TYPE_WEBSERVICE_LOADED){
        if(msg_code==WEBVIDEO_INFO_GET_ABORT || msg_code==WEBVIDEO_SEARCH_GET_ABORT){
            grid_list_display_flush_by_empty();
            return; 
        }
        if(win_data_buffing_is_open()){
            win_data_buffing_close();
        }
        if(msg_code==WEBVIDEO_INFO_GET_SUCCESS||
            msg_code==WEBVIDEO_NEXTPAGETOKEN_SUCCESS||
            msg_code==WEBVIDEO_PREVPAGETOKEN_SUCCESS||
            msg_code==WEBVIDEO_SEARCH_GET_SUCCESS){
            lv_event_send(lv_obj_get_child(objcont,0),LV_EVENT_REFRESH,ctl_msg);
        }else if(msg_code==WEBVIDEO_SEARCH_GET_ERROR){
            /* flush empty then stop thumb pic download*/
            grid_list_display_flush_by_empty();
            win_msgbox_msg_open(STR_VIDEO_INFO_ERR_RETRY,WIN_WEB_MSG_TIMEOUT,NULL,NULL);
        }else if(msg_code==WEBVIDEO_NEXTPAGETOKEN_ERROR||msg_code==WEBVIDEO_PREVPAGETOKEN_ERROR){
            lv_event_send(lv_obj_get_child(objcont,0),LV_EVENT_REFRESH,ctl_msg);
        }else if(msg_code==WEBVIDEO_INFO_GET_ERROR){
            grid_list_display_flush_by_empty();
            win_msgbox_msg_open(STR_VIDEO_INFO_ERR_RETRY,WIN_WEB_MSG_TIMEOUT,NULL,NULL);
            // to do something if it do not get 
        }
        webservice_msg_flag = true;
    }else if(ctl_msg->msg_type==MSG_TYPE_NETDOWNLOAD){
        grid_list_display_flush_by_thumbnail(msg_code);
    }

    return;    
}

#endif