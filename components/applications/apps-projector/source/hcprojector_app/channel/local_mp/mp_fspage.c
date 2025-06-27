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

#include "factory_setting.h"

#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "mp_fspage.h"
#include "mp_mainpage.h"

#include "screen.h"
#include "setup.h"
#include "mul_lang_text.h"
#include "mp_preview.h"
#include "mp_bsplayer_list.h"
#include "mp_ebook.h"
#include "mp_key_num.h"
#include <semaphore.h>
#include <hcuapi/common.h>

file_list_t *m_cur_file_list=NULL;
file_list_t  m_file_list[MEDIA_TYPE_COUNT]={0}; 
obj_list_ctrl_t m_list_ctrl;
extern media_type_t media_type;

media_handle_t *m_cur_media_hld = NULL;

extern SCREEN_TYPE_E cur_scr;
extern SCREEN_SUBMP_E screen_submp;
int last_page=1; //start form 1st page 
int cur_page,page_num;

#ifdef MP_TRANS_PREV_SUPPORT

static lv_img_dsc_t m_trans_logos[CONT_WINDOW_CNT];

static int mp_transcode_multi(char *src_path, lv_img_dsc_t *logo, file_type_t file_type);
static void mp_trans_dec_file(void);
static void mp_trans_prev_init(void);
static void mp_trans_prev_play(void);
static void mp_trans_prev_stop(void);

#endif


extern void media_player_close(void);
void *mp_get_cur_player(void)
{
    if(m_cur_media_hld)
        return m_cur_media_hld->player;
    else
        return NULL;
}
void *mp_get_cur_player_hdl(void)
{
    return m_cur_media_hld;
}
void* app_get_file_list()
{
    return m_cur_file_list;
}

void fs_page_keyinput_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    // lv_obj_t * parent_target=lv_event_get_target(event);
    int keypad_value,i;
    int v_key=0;
    uint16_t new_item_idx;
    uint16_t old_item_idx;
	file_node_t *file_node = NULL;
    if(code == LV_EVENT_KEY)
    {
        file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
        keypad_value = lv_indev_get_key(lv_indev_get_act());
        uint32_t conv_key = key_convert_vkey(keypad_value);
#ifndef LVGL_RESOLUTION_240P_SUPPORT
        /*for preview ,if preview init had to code here*/  
    #ifndef MP_TRANS_PREV_SUPPORT
        if(win_preview_state_get()){
            preview_key_ctrl(keypad_value);
        }else{
            if(preview_timer_handle!=NULL)
                lv_timer_reset(preview_timer_handle);
        }
    #endif
        /*for preview */
#endif
        switch (keypad_value)
        {
            case LV_KEY_RIGHT :
                if(m_cur_file_list->item_index==m_list_ctrl.count-1)
                    break;
                else
                {
                    lv_group_focus_next(lv_group_get_default());
                    v_key=V_KEY_RIGHT;
                    break;
                }
            case LV_KEY_LEFT :
                if(m_cur_file_list->item_index==0)
                    break;
                else
                {
                    lv_group_focus_prev(lv_group_get_default());
                    v_key=V_KEY_LEFT;
                    break; 
                }
            case LV_KEY_UP :
                if(cur_page==1 &&m_list_ctrl.cur_pos<4)
                    break;    
                else{
                    for(i=0;i<4;i++)
                        lv_group_focus_prev(lv_group_get_default());
                    v_key=V_KEY_UP;
                    break;
                }
            case LV_KEY_DOWN:
                if(m_list_ctrl.cur_pos>(m_list_ctrl.count-(m_list_ctrl.count%4)))
                    break;
                else if(m_list_ctrl.cur_pos+4>m_list_ctrl.count-1)
                    break;
                else
                {
                    for(i=0;i<4;i++)
                        lv_group_focus_next(lv_group_get_default());
                    v_key=V_KEY_DOWN;
                    break;
                }
            case LV_KEY_ENTER:
				if(MEDIA_TYPE_TXT == m_cur_file_list->media_type && FILE_TXT == file_node->type)
				{
					_ui_screen_change(ui_ebook_txt,0,0); 
				}
				else
                	media_fslist_enter(m_cur_file_list->item_index);
                break;
            case LV_KEY_ESC : //back btn value in lvgl mmap
                if(lv_obj_is_valid(ui_win_zoom)){ 
                #ifndef MP_TRANS_PREV_SUPPORT
                    preview_reset();
                #endif
                }else{
                    media_fslist_enter(0);//0 mean back to upper
                }
                break;
        }
        //when left or right key updata the cursor and list item  
        if((keypad_value==LV_KEY_RIGHT||keypad_value==LV_KEY_LEFT||keypad_value==LV_KEY_DOWN||keypad_value==LV_KEY_UP)&&v_key!=0)
        {
            old_item_idx = m_cur_file_list->item_index;
            new_item_idx = fslist_ctl_proc(&m_list_ctrl, v_key, old_item_idx);
            m_cur_file_list->item_index = new_item_idx;
        }
    }
}


void clear_fslist_obj(void)
{   
    int i ; 
    for(i=0;i<12;i++)
    {
        if(obj_labelitem[i]!= NULL)
        {
            lv_label_set_text(obj_labelitem[i],"");
            //need to set img with balck bg 
            //recreate obj will set img scr null 
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&black_bg);
        }   
    }
    lv_group_remove_all_objs(lv_group_get_default());

}   

/*fspage handle ,add lab & img in fs obj  */
void draw_media_fslist(lv_obj_t * parent,char * dir_path)
{
    int i;
    glist *list_it;
    file_node_t *file_node;
    int foucus_idx,count,node_start;
    node_start = m_list_ctrl.top;
    count = m_list_ctrl.depth;
    list_it = (glist*)m_cur_file_list->list;
    foucus_idx = m_cur_file_list->item_index-node_start;
    m_cur_file_list->media_type=media_type;
    //clear all lab & image
    clear_fslist_obj();
    //this loop for add lab and img
    for(i=0;i<count;i++)
    {
        if (NULL == list_it)
            break;
        if(node_start==m_list_ctrl.count)
            break;
        //add img 
        file_node = file_mgr_get_file_node(m_cur_file_list, node_start);
        if (FILE_DIR == file_node->type){
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&cultraview_folder);
        } else if (FILE_VIDEO == file_node->type){
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_movie);
        } else if (FILE_MUSIC == file_node->type){
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_music);
        } else if(FILE_IMAGE == file_node->type){
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_photo);
        }
        else {
            lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_text);
        }
        //add lab  
        if(file_node==NULL) 
            break;
        lv_label_set_text_fmt(obj_labelitem[i],"%s",file_node->name);
        lv_group_add_obj(lv_group_get_default(),obj_item[i]);
        node_start++;
        list_it = list_it->next;
    }

#ifdef MP_TRANS_PREV_SUPPORT    
    //stop last tanscoding and free image data buffer
    mp_trans_prev_stop();
    mp_trans_prev_play();
#endif    

    //handle ui foucus_idx
    lv_group_focus_obj(obj_item[foucus_idx]);
    label_set_long_mode_with_state(foucus_idx);
    //updata title

    if((m_cur_file_list->file_count+m_cur_file_list->dir_count)%12==0)
        page_num=(m_cur_file_list->file_count+m_cur_file_list->dir_count)/12; 
    else 
        page_num=(m_cur_file_list->file_count+m_cur_file_list->dir_count)/12 +1;

    if((m_cur_file_list->item_index+1)%12==0)//item_index from 0 start,so offset 1 
        last_page=cur_page= (m_cur_file_list->item_index+1)/12;
    else 
        last_page=cur_page= (m_cur_file_list->item_index+1)/12+1;

    //handle the first icon
    if(cur_page==1)
    {
        lv_img_set_src(lv_obj_get_child(obj_item[0],0),&Thumbnail_Upfolder);
        lv_label_set_text(obj_labelitem[0], api_rsc_string_get(STR_BACK));
    }
    lv_label_set_text_fmt(lv_obj_get_child(lv_obj_get_child(ui_fspage,1),1),"%d / %d",cur_page,page_num);
    lv_label_set_text_fmt(lv_obj_get_child(lv_obj_get_child(ui_fspage,1),0),"%s",m_cur_file_list->dir_path);    
}

int media_fslist_enter(int index)
{
    bool is_root = false;
    uint16_t file_idx;   
    char *cur_file_name = NULL;
    file_node_t *file_node = NULL;
    cur_file_name = calloc(1, MAX_FILE_NAME + 1);
    char *m_cur_fullname=(char *)malloc(1024);
    if(!cur_file_name){
	    printf("Not enough memory.\n");
    }
    is_root = win_media_is_user_rootdir(m_cur_file_list);
    

    //handle fslist index
    if (index == 0){

        if (is_root)
        {
            _ui_screen_change(ui_subpage, 0, 0);
        } else {
        // go to upper dir.
            char *parent_dir;
            parent_dir = calloc(1, MAX_FILE_NAME);
            if(!parent_dir){
                if(cur_file_name)
                {
                    free(cur_file_name);
                    cur_file_name=NULL;
                }
                    
            }

        #ifdef MP_TRANS_PREV_SUPPORT   
	        //stop last tanscoding and free image data buffer 
            mp_trans_prev_stop();
        #endif    

            strcpy(cur_file_name, m_cur_file_list->dir_path);
            win_get_parent_dirname(parent_dir, m_cur_file_list->dir_path);
            file_mgr_create_list(m_cur_file_list, parent_dir);

            file_idx = win_get_file_idx_fullname(m_cur_file_list, cur_file_name);
            if (INVALID_VALUE_16 == file_idx)
                file_idx = 0;
            //keep the upper directory's foucs 
            m_cur_file_list->item_index = file_idx;
            osd_list_ctrl_reset(&m_list_ctrl, 12, 
                m_cur_file_list->dir_count + m_cur_file_list->file_count, file_idx);
            m_list_ctrl.top =12*(m_list_ctrl.cur_pos/12);
            draw_media_fslist(ui_fspage,parent_dir);
	        free(parent_dir);
        }
        free(cur_file_name);
        cur_file_name=NULL;

    }
    else
    {
        file_node = file_mgr_get_file_node(m_cur_file_list, index);
        if (NULL == file_node){
            printf("file_mgr_get_file_node() fail!\n");
            if(cur_file_name!=NULL){
                free(cur_file_name);
                cur_file_name=NULL;
            }else if (m_cur_fullname!=NULL)
            {
                free(m_cur_fullname);
                m_cur_fullname=NULL;
            }
            return -1;
        }   
        file_mgr_get_fullname(m_cur_fullname, m_cur_file_list->dir_path, file_node->name);
        if (file_node->type == FILE_DIR){

        #ifdef MP_TRANS_PREV_SUPPORT    
            //stop last tanscoding and free image data buffer
            mp_trans_prev_stop();
        #endif    

            //enter next dir
            file_mgr_create_list(m_cur_file_list, m_cur_fullname);
            //draw lvgl obj
            m_cur_file_list->item_index = 0;
            osd_list_ctrl_reset(&m_list_ctrl, 12, 
            m_cur_file_list->dir_count + m_cur_file_list->file_count, 0);
            draw_media_fslist(ui_fspage,file_node->name);
        }
        else {
            //play media file
            _ui_screen_change(ui_ctrl_bar,0,0);
        }
    }
    if (m_cur_fullname!=NULL)
    {
        free(m_cur_fullname);
        m_cur_fullname=NULL;
    }
    if(cur_file_name!=NULL)
    {
        free(cur_file_name);
        cur_file_name=NULL;
    }
       
    return 0;
    

}

void mp_fs_message_ctrl(void * arg1,void *arg2)
{
    (void)arg2;
    int index;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;

    if (MSG_TYPE_MP_TRANS_PLAY == ctl_msg->msg_type) {
    #ifdef MP_TRANS_PREV_SUPPORT        
        index = (int)ctl_msg->msg_code;
        if (index < CONT_WINDOW_CNT)
            lv_img_set_src(lv_obj_get_child(obj_item[index],0),&m_trans_logos[index]);        
    #endif

    } else {

    #ifndef MP_TRANS_PREV_SUPPORT
        preview_player_message_ctrl(arg1, arg2);
    #endif    

    }

}

void media_fslist_open(void)
{
    fs_group= lv_group_create();
    set_key_group(fs_group);
    fs_group->auto_focus_dis=1;
    //create fspage ui 
    create_fspage_scr(); 
#ifdef MP_TRANS_PREV_SUPPORT
    mp_trans_prev_init();
#endif    
    //init m_cur_file_list
    static media_type_t last_media_type=MEDIA_TYPE_COUNT;
    if(last_media_type!=media_type||m_cur_file_list->list==NULL)
    {
        m_cur_file_list = &m_file_list[media_type];
        m_cur_file_list->media_type=media_type;
        partition_info_t * m_cur_partinfo=mmp_get_partition_info();
        char *root_path = NULL;
        root_path = m_cur_partinfo->used_dev;
        if ('\0' == m_cur_file_list->dir_path[0])
            strncpy(m_cur_file_list->dir_path,root_path, MAX_FILE_NAME-1);
        file_mgr_create_list(m_cur_file_list, m_cur_file_list->dir_path);
        last_media_type=media_type;
    }

    osd_list_ctrl_reset(&m_list_ctrl, MAX_FILELIST_ITEMS, m_cur_file_list->dir_count + m_cur_file_list->file_count, m_cur_file_list->item_index); 
    osd_list_update_top(&m_list_ctrl);  //list->top diff in this case so handle here
    draw_media_fslist(ui_fspage, MOUNT_ROOT_DIR);  
    screen_submp=SCREEN_SUBMP2;
#if !defined(LVGL_RESOLUTION_240P_SUPPORT) && !defined(MP_TRANS_PREV_SUPPORT)
    if(m_cur_file_list->media_type!=MEDIA_TYPE_TXT)
        preview_init();
#endif
    api_ffmpeg_player_get_regist(mp_get_cur_player);

}

void media_fslist_close(void)
{
#ifndef MP_TRANS_PREV_SUPPORT
    preview_deinit();
#endif    
#ifdef MP_TRANS_PREV_SUPPORT    
    mp_trans_prev_stop();
#endif    
    clear_fapage_scr();
    lv_group_remove_all_objs(fs_group);
    lv_group_del(fs_group);
}

uint16_t fslist_ctl_proc(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t pos)
{
    int i; 
    uint16_t count;
    int node_start;
    uint16_t item_new_pos = 0;
    count = list_ctrl->depth;
    file_node_t*  file_node;
    int focus_idx=0;
    //update the list ctrl postion information
    osd_list_ctrl_update(list_ctrl, vkey, pos, &item_new_pos);
    //updata pagenum 
    if((item_new_pos+1)% 12==0) //item offset 1
        cur_page=(item_new_pos+1)/12;
    else 
        cur_page=(item_new_pos+1)/12+1;


    lv_label_set_text_fmt(lv_obj_get_child(lv_obj_get_child(ui_fspage,1),1),"%d / %d",cur_page,page_num);
    //draw page num
    node_start=12*(cur_page-1);
    list_ctrl->top=12*(cur_page-1);
    //turn  page operation last page != cur page 
    if (last_page!=cur_page){ 
        lv_group_remove_all_objs(lv_group_get_default());
        //redraw the list label according the new postion infromation
        for (i = 0; i < count; i ++){
            // file_node = file_mgr_get_file_node(m_cur_file_list, node_start);
            if(node_start < list_ctrl->count)
            {
                //add image 
                file_node = file_mgr_get_file_node(m_cur_file_list, node_start);
                if (FILE_DIR == file_node->type){
                    lv_img_set_src(lv_obj_get_child(obj_item[i],0),&cultraview_folder);     
                } else if (FILE_VIDEO == file_node->type){
                    lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_movie);
                } else if (FILE_MUSIC == file_node->type){
                    lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_music);
                } else if(FILE_IMAGE == file_node->type){
                    lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_photo);
                }
                else {
                    lv_img_set_src(lv_obj_get_child(obj_item[i],0),&IDB_FileSelect_text);
                }
                lv_label_set_text_fmt(obj_labelitem[i],"%s",file_node->name);
                //group
                lv_group_add_obj(lv_group_get_default(),obj_item[i]);
            }
            else
            {
                lv_img_set_src(lv_obj_get_child(obj_item[i],0),&black_bg);
                lv_label_set_text(obj_labelitem[i],"");
            }   
            node_start ++; 
        }
        if(cur_page==1)
        {
            lv_img_set_src(lv_obj_get_child(obj_item[0],0),&Thumbnail_Upfolder);
            set_label_text2(obj_labelitem[0],STR_BACK,FONT_MID);
        }
        last_page=cur_page;
    #ifdef MP_TRANS_PREV_SUPPORT
        //stop last tanscoding and free image data buffer	
        mp_trans_prev_stop();
        mp_trans_prev_play();
    #endif
    }
    //handle ui foucus 
    focus_idx = item_new_pos - list_ctrl->top;
    lv_group_focus_obj(obj_item[focus_idx]);
    label_set_long_mode_with_state(focus_idx);
    return item_new_pos; 
} 


// top should ctrl 
void osd_list_update_top(obj_list_ctrl_t *list_ctrl)
{
    int lastwin_page_count=0;
    if(list_ctrl->count>list_ctrl->depth)
    {
        lastwin_page_count=list_ctrl->cur_pos/12+1;
    }
    else if(list_ctrl->count<=list_ctrl->depth)
    {
        lastwin_page_count=1;
    }
    list_ctrl->top=12*(lastwin_page_count-1);
}
void clear_fslist_path(void *path)
{
    memset(path,0,1024);
}

int label_set_long_mode_with_state(int foucus_idx)
{
    //press set style scroll+hignlight
    for(int i=0;i<12;i++)
    {
        if(i==foucus_idx)
        {
            lv_label_set_long_mode(obj_labelitem[i],LV_LABEL_LONG_SCROLL_CIRCULAR);
            lv_obj_set_style_text_color(obj_labelitem[i],lv_color_hex(0xFFFF00),0);
        }
        else
        {
            lv_label_set_long_mode(obj_labelitem[i],LV_LABEL_LONG_DOT);
            lv_obj_set_style_text_color(obj_labelitem[i],lv_color_hex(0xFFFFFF),0);
        } 

    }
    return 0;
}

int app_media_list_all_free()
{
    int media_type_cnt=4;
    for(int i=0;i<media_type_cnt;i++){
        file_list_t * single_fslist=&m_file_list[i];
        file_mgr_free_list(single_fslist);
    }
#ifdef RTOS_SUBTITLE_SUPPORT
    file_mgr_subtitle_list_free();
#endif
    return 0;
}

int vkey_other_btn(lv_obj_t *parent,int vkey,lv_group_t *group)
{
    if((0 <= vkey) && (vkey <= 9))
    {
        open_digital_num(parent,vkey,group);
    }
    return 0;
}
#ifdef HC_MEDIA_MEMMORY_PLAY
extern void memmory_play_callback(void *data);
static int mp_auto_play=0;
int mp_get_auto_play_state(void)
{
    return mp_auto_play;
}

void mp_set_auto_play_state(int flag)
{
    mp_auto_play = flag;
}

static int string_split(const char* input, char delimiter, int position, char* output, size_t output_size) 
{
    if (!input || !output || output_size == 0) {
        return -1; // Invalid arguments
    }

    size_t input_len = strlen(input);

    // for each input string
    int count = 0;
    for (size_t i = 0; i < input_len; i++) {
        if (input[i] == delimiter) {
            count++;
            if (count == position) {
                if (i + 1 > output_size) {
                    return -1; // Output buffer is too small
                }
                strncpy(output, input, i);
                output[i] = '\0';
                return 0; // Success
            }
        }
    }

    if (input_len + 1 > output_size) {
        return -2; // Output buffer is too small
    }
    strncpy(output, input, output_size - 1);
    output[output_size - 1] = '\0';

    return 1; // Delimiter position out of range
}



int read_file_list(void)
{
    file_list_t *m_cur_file_list_tmp=NULL;
    int ret = -1,token_len=-1;
    char *token;
    char name_tmp[MAX_FILE_NAME]={0};
    char media_disk[32] = {0};
    play_info_t	*media_play_info = NULL;
    partition_info_t * cur_partition_info=mmp_get_partition_info();

    media_play_info = sys_data_get_media_info((media_type_t)projector_get_some_sys_param(P_MEM_PLAY_MEDIA_TYPE));
    if (NULL == media_play_info)
        return -1;

    app_media_list_all_free();
    clear_all_bsplayer_mem();//for bs player music filelist 

    mp_auto_play = 1;
    media_type = (media_type_t)projector_get_some_sys_param(P_MEM_PLAY_MEDIA_TYPE);
    m_cur_file_list_tmp = &m_file_list[media_type];
    m_cur_file_list_tmp->media_type=media_type;
    //api_set_partition_info(cur_partition_info->count-1);
    memset(name_tmp,0,MAX_FILE_NAME);
    strcpy(name_tmp,media_play_info->path);
    token = strtok(name_tmp,"/");
    while(token != NULL)
    {
        token = strtok(NULL,"/");
		if(token != NULL)
			token_len = strlen(token);
    }
    memcpy(m_cur_file_list_tmp->dir_path,media_play_info->path,strlen(media_play_info->path)-token_len-1);
    file_mgr_create_list(m_cur_file_list_tmp, m_cur_file_list_tmp->dir_path);
    m_cur_file_list = m_cur_file_list_tmp;
    string_split(m_cur_file_list_tmp->dir_path, '/', 3, media_disk, sizeof(media_disk));
    ret = api_partition_info_used_disk_set(media_disk);
    if(ret < 0)
    {
        return ret;
    }
    return 0;
}

#endif //end of #ifdef HC_MEDIA_MEMMORY_PLAY


#ifdef MP_TRANS_PREV_SUPPORT

//#define TRANS_DUMP_SUPPORT

typedef struct{
    sem_t sem_tans_start;
    sem_t sem_tans_stop;

    pthread_mutex_t mutex_player;
    bool start;
    bool stop;
    bool exit;
    int img_width;
    int img_height;
}TRANS_PREV_t;

static bool m_trans_init = false;
static TRANS_PREV_t m_trans_prev;
static int mp_trans_interrupt_cb(void *param)
{
    if (m_trans_prev.stop)
        return 1; //tanscode stop;
    else
        return 0; //tanscode continue;
}


#ifdef TRANS_DUMP_SUPPORT

static int m_trans_dump_idx = 0;
static char m_trans_dump_dir[256] = {0};
static void transcode_clear_dir(const char *dir_path)
{
    DIR *dir;
    struct dirent *entry;
    struct stat entry_stat;

    dir = opendir(dir_path);
    if (dir == NULL) {
        printf("open %s\n", dir_path);
        perror("opendir1");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        stat(entry->d_name, &entry_stat);

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        if (DT_DIR == entry->d_type) {
            transcode_clear_dir(full_path);
            rmdir(full_path);
        } else {
            unlink(full_path);
        }
    }
    closedir(dir);    
}

//create /media/hdd(sda1)/trans_tmp directory for trans dump files
static void transcode_dump_init(void)
{
    char  *dev_name = NULL;

    m_trans_dump_idx = 0;
    memset(m_trans_dump_dir, 0, sizeof(m_trans_dump_dir));
    dev_name = api_get_partition_info_by_index(0);
    if (!dev_name){
        printf("No device for trans dump!");
        return;
    }
    sprintf(m_trans_dump_dir, "%s/trans_tmp", dev_name);

}

//dump bmp data to files of usb disk
static void transcode_dump_start(char *buf, int length)
{
    static char rec_name[256]={0};
    char *path = NULL;
    struct stat info;

    if (0 == m_trans_dump_idx){
        path = m_trans_dump_dir;
        if (stat(path, &info) == 0) {
            if (S_ISDIR(info.st_mode)) {
                transcode_clear_dir(path);
            } else {
                printf("Error: '%s' is not a directory.\n", path);
            }
        } else {
            if (mkdir(path, 0755) != 0) {
                perror("mkdir");
            }
        }    
        printf("%s(). create %s OK!\n", __func__, m_trans_dump_dir);
    }

    m_trans_dump_idx = m_trans_dump_idx % 12;
    snprintf(rec_name, sizeof(rec_name), "%s/trans_%d.bmp", m_trans_dump_dir, m_trans_dump_idx++);
    FILE *rec = fopen(rec_name, "wb+");
    if (rec) {
        fwrite(buf, length, 1, rec);
        fclose(rec);
    }

    printf("write %d %s(%dx%d) %s\n", (int)length, rec_name, 
        m_trans_prev.img_width, m_trans_prev.img_height,rec ? "OK!" : "fail!");
}

#endif

static int mp_transcode_multi(char *src_path, lv_img_dsc_t *logo, file_type_t file_type)
{
    uint8_t *out_buf;
    int out_buf_size;
    int ret = 0;
    //Transcode thumbnail at 60s, to avoid some black thumbnail image
    double start_time = 60000;

    int dst_width = m_trans_prev.img_width;
    int dst_height = m_trans_prev.img_height;

    if (m_trans_prev.stop)
        return 0;

    if (logo->data){
        free(logo->data);
        logo->data = NULL;   
    }    

    // ret = hcplayer_pic_transcode(src_path, &out_buf, &out_buf_size, \
    //     dst_width, dst_height, start_time, NULL, NULL, IMG_TRANSCODE_MODE_CENTER, mp_trans_interrupt_cb, NULL); 

    HCPlayerTranscodeArgs transcode_args={0};
    transcode_args.url=src_path;
    transcode_args.render_width=dst_width;
    transcode_args.render_height=dst_height;
    transcode_args.start_time=start_time;
    transcode_args.transcode_mode=IMG_TRANSCODE_MODE_CENTER;
    transcode_args.transcode_format=IMG_TRANSCODE_BGRA;
    transcode_args.interrupt_cb=mp_trans_interrupt_cb;
    if (api_video_pbp_get_support()){
        if (APP_DIS_LAYER_MAIN == api_video_pbp_get_dis_layer(VIDEO_PBP_SRC_MP))
            transcode_args.dis_layer = 1; //main layer
        else if (APP_DIS_LAYER_AUXP == api_video_pbp_get_dis_layer(VIDEO_PBP_SRC_MP))
            transcode_args.dis_layer = 2; //auxp layer
    }
    ret=hcplayer_pic_transcode2(&transcode_args);
    out_buf=transcode_args.out;
    out_buf_size=transcode_args.out_size;
    if (!ret && out_buf){
        logo->header.h = transcode_args.out_height;
        logo->header.w = transcode_args.out_width;
        logo->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
        logo->data_size = out_buf_size;
        logo->data = out_buf;
    }

    return ret;

}


static void mp_trans_dec_file(void)
{
    int i;
    file_node_t *file_node;
    int node_start;
    int count;
    glist *list_it;
    int ret = 0;
    char file_name[1024];

    node_start = m_list_ctrl.top;
    count = m_list_ctrl.depth;
    list_it = (glist*)m_cur_file_list->list;
    m_cur_file_list->media_type=media_type;

    control_msg_t ctl_msg = {0};

#ifdef TRANS_DUMP_SUPPORT
    transcode_dump_init();
#endif

    for( i = 0; i < count; i++) {
        if (NULL == list_it)
            break;

        if (m_trans_prev.stop)
            break;

        if(node_start == m_list_ctrl.count)
            break;

        if (m_trans_prev.stop)
            break;
        file_node = file_mgr_get_file_node(m_cur_file_list, node_start);
        if (!file_node)
            continue;

        if (
            FILE_VIDEO != file_node->type &&
            FILE_MUSIC != file_node->type &&
            FILE_IMAGE != file_node->type 
            )
        {
            node_start++;
            list_it = list_it->next;
            api_sleep_ms(5);
            continue;
        }

        if (m_trans_prev.stop)
            break;

        file_mgr_get_fullname(file_name, m_cur_file_list->dir_path, file_node->name);

        if (m_trans_prev.stop)
            break;
        ret = mp_transcode_multi(file_name, &m_trans_logos[i], file_node->type);
        if (!ret && m_trans_logos[i].data){
            ctl_msg.msg_type = MSG_TYPE_MP_TRANS_PLAY;
            ctl_msg.msg_code = i;

            if (m_trans_prev.stop)
                break;
            api_control_send_msg(&ctl_msg);

    #ifdef TRANS_DUMP_SUPPORT
            transcode_dump_start(m_trans_logos[i].data, m_trans_logos[i].data_size);
    #endif

        }

        node_start++;
        list_it = list_it->next;
        api_sleep_ms(5);
    }
}

static void *mp_trans_prev_task(void *param)
{
    do {
        //start transcode.
        sem_wait(&m_trans_prev.sem_tans_start);

        //start transcode.
        printf("%s(), trans start!\n", __func__);
        mp_trans_dec_file();

        pthread_mutex_lock(&m_trans_prev.mutex_player);
        if (m_trans_prev.stop == true)
            sem_post(&m_trans_prev.sem_tans_stop);
        m_trans_prev.start = false;
        printf("%s(), trans finished!\n", __func__);
        pthread_mutex_unlock(&m_trans_prev.mutex_player);

        api_sleep_ms(10);
    } while (!m_trans_prev.exit);

    return NULL;
}

static void mp_trans_prev_stop(void)
{
    int i;

    pthread_mutex_lock(&m_trans_prev.mutex_player);
    if (!m_trans_prev.start){
        pthread_mutex_unlock(&m_trans_prev.mutex_player);
        goto prev_exit;
    }

    m_trans_prev.start = false;
    m_trans_prev.stop = true;
    pthread_mutex_unlock(&m_trans_prev.mutex_player);

    //wait transcode finished!
    sem_wait(&m_trans_prev.sem_tans_stop);

prev_exit:
    for (i = 0; i < CONT_WINDOW_CNT; i ++){
        if (m_trans_logos[i].data){
            free(m_trans_logos[i].data);
            m_trans_logos[i].data = NULL;
        }
    }
}


static void mp_trans_prev_play(void)
{
    pthread_mutex_lock(&m_trans_prev.mutex_player);
    m_trans_prev.start = true;
    m_trans_prev.stop = false;
    pthread_mutex_unlock(&m_trans_prev.mutex_player);

    sem_post(&m_trans_prev.sem_tans_start);
}


static void mp_trans_prev_init(void)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;
    lv_obj_t *obj;

    if (m_trans_init)
        return;
    m_trans_init = true;
    memset(&m_trans_prev, 0, sizeof(m_trans_prev));
    memset(&m_trans_logos, 0, sizeof(m_trans_logos));

    obj = obj_item[0];
    // m_trans_prev.img_width = (obj->coords.x2 - obj->coords.x1) + 6;
    // m_trans_prev.img_height = (obj->coords.y2 - obj->coords.y1) + 6;
    m_trans_prev.img_width = LV_HOR_RES / 5;//240;
    m_trans_prev.img_height = LV_HOR_RES / 5; //240;

    sem_init(&m_trans_prev.sem_tans_start, 0, 0);
    sem_init(&m_trans_prev.sem_tans_stop, 0, 0);
    pthread_mutex_init(&m_trans_prev.mutex_player, NULL);

    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    pthread_create(&thread_id, &attr, mp_trans_prev_task, NULL);
    pthread_attr_destroy(&attr);
}

#endif //end of #ifdef MP_TRANS_PREV_SUPPORT
