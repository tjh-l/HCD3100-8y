#ifndef __LOCAL_MP_UI_H__
#define __LOCAL_MP_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

#define CONT_WINDOW_CNT  9

extern lv_obj_t *ui_mainpage;
extern lv_obj_t *mp_partition;
extern lv_obj_t *ui_fspage;
extern lv_obj_t *ui_player;

extern lv_group_t *main_group;
extern lv_group_t *fs_group;
extern lv_group_t *player_group;

extern lv_obj_t *obj_item[CONT_WINDOW_CNT];
extern lv_obj_t *obj_labelitem[CONT_WINDOW_CNT];

void ui_mainpage_screen_init(void);
void ui_fspage_screen_init(void);
void ui_player_screen_init(void);

int create_mainpage_scr(void);
int clear_mainpage_scr(void);

int create_fspage_scr(void);
int clear_fapage_scr(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __LOCAL_MP_UI_H__
