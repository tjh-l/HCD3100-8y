#ifndef __WIN_MEIDA_LIST_H__
#define __WIN_MEIDA_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "file_mgr.h"

void win_get_parent_dirname(char *parentpath, char *path);
uint16_t win_get_file_idx_fullname(file_list_t *file_list, char *file_name);
bool win_media_is_user_rootdir(file_list_t *file_list);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __WIN_MEIDA_LIST_H__
