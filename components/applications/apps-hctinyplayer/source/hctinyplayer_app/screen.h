#ifndef _TINYPLAYERSETTING_UI_H
#define _TINYPLAYERSETTING_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"
#include "lvgl/lvgl.h"

extern lv_obj_t *volume_scr;
extern lv_obj_t *volume_bar;
extern lv_indev_t *indev_keypad;

void _ui_screen_change(lv_obj_t *target,  int spd, int delay);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
