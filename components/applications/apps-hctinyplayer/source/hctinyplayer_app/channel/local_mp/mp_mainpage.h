#ifndef __MP_MAINPAGE_H__
#define __MP_MAINPAGE_H__

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"
#include "osd_com.h"
#include "media_player.h"

#ifdef __cplusplus
extern "C" {
#endif

extern lv_indev_t *indev_keypad;

void set_key_group(lv_group_t *group);
void main_page_keyinput_event_cb(lv_event_t *event);

int mainpage_open(void);
int mainpage_close(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __MP_MAINPAGE_H__
