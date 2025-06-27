#ifndef __MP_FSPAGE_H__
#define __MP_FSPAGE_H__

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"
#include "osd_com.h"
#include "file_mgr.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILELIST_ITEMS CONT_WINDOW_CNT

extern file_list_t *m_cur_file_list;

void *mp_get_cur_player(void);
void fs_page_keyinput_event_cb(lv_event_t *event);
int media_fslist_enter(int index);
void media_fslist_open(void);
void media_fslist_close(void);
uint16_t fslist_ctl_proc(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t pos);
void osd_list_update_top(obj_list_ctrl_t *list_ctrl);
int label_set_long_mode_with_state(int foucus_idx);
int app_media_list_all_free();

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __MP_FSPAGE_H__
