//use for player player from list 
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
// #include "local_mp_ui_lld.h"
#include "mp_mainpage.h"
#include "mp_subpage.h"
#include "mp_fspage.h"
#include "mp_ctrlbarpage.h"
#include "mp_ebook.h"
#include "src/font/lv_font.h"
#include "setup.h"
#include "factory_setting.h"

#include "mul_lang_text.h"

#include "mp_bsplayer_list.h"
#include "backstage_player.h"
#include "mp_playerinfo.h"
#include "mp_ebook.h"

/*
all global variable
*/
lv_obj_t * playlist_item[MAX_MPLIST_ITEM]={NULL};
file_list_t  playlist={0};
obj_list_ctrl_t playlist_ctrl={0}; 
int   cur_playing_index;
char * cur_playing_filename;
static lv_obj_t* playerlist_container = NULL;
typedef void (*playlist_win_event_callback)(int argc, void* argv);
playlist_win_event_callback playlist_win_func; 

int filelist_enter(int index);
int playlist_ctrl_proc(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t pos);
void file_list_del_node(file_list_t * file_list,int index_to_del);
int draw_playlist(void);
int playlist_update_listindex(file_list_t *scr_list,file_list_t *dst_list);
/*
* all ctrl in here 
*/

void mp_playlist_win_event_cb(lv_event_t * e )
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t * user_data=lv_event_get_user_data(e);

    if(code ==LV_EVENT_KEY){
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        uint32_t vkey_value=key_convert2_vkey(key);
        int act_key=0;
        uint16_t old_item_idx = playlist.item_index;  
        switch (key)
        {
            case LV_KEY_UP:
                act_key=V_KEY_UP;
                break;
            case LV_KEY_DOWN:
                act_key=V_KEY_DOWN;
                break;
            case LV_KEY_RIGHT:
                break;
            case LV_KEY_LEFT:
                break;
            case LV_KEY_ENTER:
                act_key=V_KEY_OK;
                break;
            case LV_KEY_ESC:
                if(playerlist_container && lv_obj_is_valid(playerlist_container)){
                    lv_obj_del(playerlist_container );
                    playerlist_container = NULL;
                }
            lv_group_focus_obj(user_data);
                break;
            default :
                break;
        }
        if(act_key==V_KEY_DOWN||act_key==V_KEY_UP){
            playlist.item_index = playlist_ctrl_proc(&playlist_ctrl, act_key, old_item_idx);
        }else if(act_key==V_KEY_OK){
            playlist.item_index = playlist.item_index;
            filelist_enter(playlist.item_index);
        }
        if(vkey_value==V_KEY_BLUE){
            file_list_del_node(&playlist,playlist.item_index);
            draw_playlist();
            playlist.file_count--;
            osd_list_ctrl_reset(&playlist_ctrl, MAX_MPLIST_ITEM, 
            playlist.file_count+ playlist.dir_count, playlist.item_index);
            printf("del node from file_list_t\n");
        }
    }

}

int clear_playlist(void)
{
    for(int i =0;i<MAX_MPLIST_ITEM;i++){
        lv_label_set_text(playlist_item[i]," ");
        lv_obj_clear_state(playlist_item[i],LV_STATE_CHECKED);
        lv_label_set_long_mode(playlist_item[i], LV_LABEL_LONG_CLIP);
    }
    return 0;
}
void file_list_del_node(file_list_t * file_list,int index_to_del)
{
    glist* glist_del = NULL;
    glist_del=glist_nth(file_list->list,index_to_del);
    file_list->list=glist_delete_link(file_list->list,glist_del);
}
//only show file 
int draw_playlist(void)
{
    file_node_t *file_node = NULL;
    glist* file_node_glist = NULL;
    int foucus_idx,node_start;
    node_start = playlist_ctrl.top;
    foucus_idx = playlist.item_index-node_start;
    clear_playlist();
    for(int i =0;i<MAX_MPLIST_ITEM;i++){
        file_node=file_mgr_get_file_node(&playlist,node_start);
        if(file_node==NULL)
           break;
        if(file_node->type!= FILE_DIR)
            lv_label_set_text_fmt(playlist_item[i],"%s",file_node->name);
        node_start++;
    }
    lv_obj_add_state(playlist_item[foucus_idx],LV_STATE_CHECKED);
    lv_label_set_long_mode(playlist_item[foucus_idx], LV_LABEL_LONG_SCROLL_CIRCULAR);
    return 0;
}

int playlist_ctrl_proc(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t pos)
{
    int i; 
    uint16_t count;
    int node_start;
    file_node_t *file_node;
    int focus_idx;
    uint16_t item_new_pos = 0;
    if (osd_list_ctrl_update2(list_ctrl, vkey, pos, &item_new_pos))
    {
        //redraw the list label according the new postion infromation
        count = list_ctrl->depth;
        node_start = list_ctrl->top;
        for (i = 0; i < count; i ++){
            if (node_start < list_ctrl->count){
                file_node = file_mgr_get_file_node(&playlist, node_start);
                lv_label_set_text_fmt(playlist_item[i], "%s" ,file_node->name);
            }else{
                lv_label_set_text(playlist_item[i], "");
            }
            node_start ++;
        }
    }
    for(int j=0;j<list_ctrl->depth;j++){
        lv_obj_clear_state(playlist_item[j],LV_STATE_CHECKED);    //clear all state
        lv_label_set_long_mode(playlist_item[j], LV_LABEL_LONG_CLIP);
        // lv_obj_set_style_text_color
    }
    focus_idx = item_new_pos - list_ctrl->top; 
    lv_obj_add_state(playlist_item[focus_idx],LV_STATE_CHECKED);
    lv_label_set_long_mode(playlist_item[focus_idx], LV_LABEL_LONG_SCROLL_CIRCULAR);
    return item_new_pos; 
}

static int playlist_replay(file_list_t*m_filelist)
{
    // api_pic_effect_enable(false);
    // do not free effect when contiune play ,do it when exit whole player.
    media_handle_t* mp_hdl=mp_get_cur_player_hdl();
    media_stop(mp_hdl);
    char m_play_path_name[1024] = {0};
    file_node_t *file_node = file_mgr_get_file_node(m_filelist, m_filelist->item_index);
    file_mgr_get_fullname(m_play_path_name, m_filelist->dir_path, file_node->name);
    media_play(mp_hdl, m_play_path_name);
    return 0;
}

int filelist_enter(int index)
{
    char *cur_file_name = NULL;
    file_node_t *file_node = NULL;
    cur_file_name = calloc(1, MAX_FILE_NAME + 1);
    char * m_cur_fullname=(char *)malloc(1024);
    if(!cur_file_name){
	    printf("Not enough memory.\n");
    }
    file_node = file_mgr_get_file_node(&playlist, index);
    if (NULL == file_node){
        printf("file_mgr_get_file_node() fail!\n");
        if(cur_file_name!=NULL){
            free(cur_file_name);
            cur_file_name=NULL;
        }else if (m_cur_fullname!=NULL){
            free(m_cur_fullname);
            m_cur_fullname=NULL;
        }
        return -1;
    }   
    file_mgr_get_fullname(m_cur_fullname, playlist.dir_path, file_node->name);
    if (file_node->type == FILE_DIR){
        file_mgr_create_list(&playlist, m_cur_fullname);
        playlist.item_index = 0;
        osd_list_ctrl_reset(&playlist_ctrl, 12, 
        playlist.dir_count + playlist.file_count, 0);
        draw_playlist();
    }else {
        //refresh name & save a filenaem to use 
        lv_label_set_text(ui_playname,file_node->name);
        cur_playing_filename=lv_label_get_text(ui_playname);
        switch(playlist.media_type){
            case MEDIA_TYPE_TXT:
                /*do not need free ebook buffer,ebook_read_file will do this operation*/
                playlist_update_listindex(&playlist,app_get_file_list());
                ebook_read_file(m_cur_fullname);
                break;
            default :
                playlist_update_listindex(&playlist,app_get_file_list());
                playlist_replay(&playlist);
                break;
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

int create_playlist_from_filelist(file_list_t * source_list,file_list_t * playlist)
{
    glist* temp_glist =(glist* )source_list->list;
    while(temp_glist)
    {
        file_node_t * src_node=(file_node_t *)temp_glist->data;
        if(src_node->type!=FILE_DIR){
            int len = strlen(src_node->name);
            file_node_t * playlist_node = (file_node_t*)malloc(sizeof(file_node_t) + len + 1);
            playlist_node->type = src_node->type;
            playlist->file_count ++;
            strcpy(playlist_node->name, src_node->name);
            playlist->list = glist_append(playlist->list, (void *)playlist_node);
        }
        temp_glist=temp_glist->next;
    }
    return 0;
}

int playlist_update_listindex(file_list_t *scr_list,file_list_t *dst_list)
{
    file_node_t * file_node= file_mgr_get_file_node(scr_list, scr_list->item_index);
    if(file_node==NULL)
        return 0;
    glist* glist_tofind=(glist*)dst_list->list;
    file_node_t * playlist_filenode=(file_node_t*)glist_tofind->data;
    int playlist_index=0;
    while(glist_tofind){
        if(glist_tofind!=NULL&&glist_tofind->data!=NULL){
            if(strcmp(file_node->name,playlist_filenode->name)==0){
                break;
            }
            else{
                glist_tofind=glist_tofind->next;
				if(glist_tofind ==NULL || glist_tofind == 0x0)
					break;
				playlist_filenode=(file_node_t*)glist_tofind->data;
                playlist_index++;
            }
                
        }
    }
    dst_list->item_index=playlist_index;
    return playlist_index;

}

int filelist_updata_listindex(char* string,file_list_t *dst_list)
{
    glist* glist_tofind=(glist*)dst_list->list;
    file_node_t * playlist_filenode=(file_node_t*)glist_tofind->data;
    int playlist_index=0;
    while(glist_tofind){
        if(glist_tofind!=NULL&&glist_tofind->data!=NULL){
            printf("each node.name:%s\n",playlist_filenode->name);
            if(strcmp(string,playlist_filenode->name)==0){
                break;
            }
            else{
                glist_tofind=glist_tofind->next;//list  是否会改变，gdb上地址没有改变
                playlist_filenode=(file_node_t*)glist_tofind->data;
                playlist_index++;
            }
                
        }
    }
    dst_list->item_index=playlist_index;
    return playlist_index;

}
/*
 * playerlist only contain file in file path ，filelist contain file and dir in file path
 * create playerlist struct from filelist to record playerlist opt
 * */
int playlist_init()
{
    if(playlist.list==NULL){
        file_list_t * cur_filelist=app_get_file_list();
        memset(&playlist,0,sizeof(file_list_t));
        playlist.media_type=cur_filelist->media_type;
        memcpy(playlist.dir_path,cur_filelist->dir_path,MAX_FILE_NAME);
        create_playlist_from_filelist(cur_filelist,&playlist);
        playlist_update_listindex(cur_filelist,&playlist);
        osd_list_ctrl_reset(&playlist_ctrl, MAX_MPLIST_ITEM, 
        playlist.file_count+ playlist.dir_count, playlist.item_index); 
        //only show file without file dir
    }
    return 0;
}
int playlist_deinit()
{
    /*playlist will change index when operation 
    so had to updata file lsit index */
    playlist_update_listindex(&playlist,app_get_file_list());
    file_mgr_free_list(&playlist);
    return 0;
}
/* transform hcplayer_msg to ui's obj to reset obj focus
 * */ 
lv_obj_t* create_playlist_win(lv_obj_t * p,lv_obj_t *sub_btn)
{
    playerlist_container = create_list_obj2(p,MPPLAYLIST_WIN_W_PCT,67);
    #ifdef LVGL_RESOLUTION_240P_SUPPORT
    lv_obj_align(playerlist_container, LV_ALIGN_TOP_RIGHT,-15,10);
    #endif
    lv_obj_add_event_cb(playerlist_container, mp_playlist_win_event_cb, LV_EVENT_ALL, sub_btn);
    lv_group_focus_obj(playerlist_container);
    for(int i=0;i<MAX_MPLIST_ITEM;i++){
        playlist_item[i]=create_list_sub_text_obj2(playerlist_container,100,100/MAX_MPLIST_ITEM);
    }
    osd_list_ctrl_reset(&playlist_ctrl, MAX_MPLIST_ITEM, 
    playlist.file_count+ playlist.dir_count, playlist.item_index); //only show file without file dir
    draw_playlist();
    lv_obj_move_foreground(playerlist_container);
    return playerlist_container;
}
void * app_get_playlist_t()
{
    return &playlist;
}
int app_get_playlist_index()
{
    return playlist.item_index;
}
int app_get_playlist_all_file()
{
    return playlist.file_count;
}

void win_playlist_ui_update(int argc, void* argv)
{
    if(playerlist_container&&lv_obj_is_valid(playerlist_container)){
        playlist.item_index = playlist_ctrl_proc(&playlist_ctrl, argc, playlist.item_index);
    }
    return;
}


