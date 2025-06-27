/**
 * win_media_list.c, use to list media files(USB).
 */
#include <stdio.h>
#include <unistd.h>
#include "lvgl/lvgl.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"
#include "media_player.h"
#include "file_mgr.h"
#include <glist.h>
#include "win_media_list.h"

bool win_media_is_user_rootdir(file_list_t *file_list)
{
    partition_info_t *m_cur_partinfo = mmp_get_partition_info();
    if (m_cur_partinfo != NULL)
    {
        char *root_path = NULL;
        root_path = m_cur_partinfo->used_dev;
        if (strcmp(file_list->dir_path, root_path))
        {
            return false;
        }
        else
        {
            return true;
        }
    }
}

void win_get_parent_dirname(char *parentpath, char *path)
{
    //get parent dir's path, as: /a/b/->/a/
    uint16_t i;

    strcpy(parentpath, path);
    i = strlen(parentpath);
    while ((parentpath[i] != '/') && (i > 0))
    {
        i--;
    }
    if (i != 0)
    {
        parentpath[i] = '\0';
    }
    else
    {
        i += 1;
        parentpath[i] = '\0';
    }
}


uint16_t win_get_file_idx_fullname(file_list_t *file_list, char *file_name)
{
    glist *list_tmp;
    file_node_t *file_node;
    uint16_t index = 0;
    bool found = false;

    char full_name[MAX_FILE_NAME + 1] = {0};

    list_tmp = (glist *)file_list->list;
    while (list_tmp)
    {
        file_node = (file_node_t *)(list_tmp->data);
        file_mgr_get_fullname(full_name, file_list->dir_path, file_node->name);
        if (!strcmp(file_name, full_name))
        {
            found = true;
            break;
        }
        list_tmp = list_tmp->next;
        index ++;
    }
    if (!found)
        index = INVALID_VALUE_16;

    return index;
}
