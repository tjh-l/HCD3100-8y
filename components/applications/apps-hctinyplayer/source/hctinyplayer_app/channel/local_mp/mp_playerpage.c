//This file is used to handle lvgl ui related logic and operations
//all most ui draw in local mp ui.c
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "local_mp_ui.h"
#include "mp_mainpage.h"
#include "mp_playerpage.h"
#include "mp_fspage.h"

extern media_handle_t *m_cur_media_hld;
static media_handle_t *m_player_hld[MEDIA_TYPE_COUNT] = {NULL,};

//for media play bar
void ctrlbarpage_open(void)
{
    player_group = lv_group_create();
    set_key_group(player_group);
    player_group->auto_focus_dis = 1;
    lv_group_add_obj(lv_group_get_default(), ui_player);

    file_node_t *file_node = NULL;
    char m_play_path_name[MAX_FILE_NAME] = {0};
    file_node = file_mgr_get_file_node(m_cur_file_list, m_cur_file_list->item_index);
    api_ffmpeg_player_get_regist(mp_get_cur_player);

    m_cur_media_hld = m_player_hld[m_cur_file_list->media_type];
    if (NULL == m_cur_media_hld)
    {
        m_cur_media_hld = media_open(m_cur_file_list->media_type);
        m_player_hld[m_cur_file_list->media_type] = m_cur_media_hld;
    }

    memset(m_play_path_name, 0, MAX_FILE_NAME);
    file_mgr_get_fullname(m_play_path_name, m_cur_file_list->dir_path, file_node->name);

    media_play(m_cur_media_hld, m_play_path_name);
}

void media_player_close(void)
{
    if (m_cur_media_hld)
    {
        media_stop(m_cur_media_hld);
        media_close(m_cur_media_hld);
        m_player_hld[m_cur_file_list->media_type] = NULL;
        m_cur_media_hld = NULL;
    }
}

int ctrlbarpage_close()
{
    lv_group_remove_all_objs(player_group);
    lv_group_del(player_group);
    media_player_close();
    return 0;
}
