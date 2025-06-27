#ifndef __MAIN_PAGE_H__
#define __MAIN_PAGE_H__

#include "lvgl/lvgl.h"

void main_page_init(void);
void main_page_state_detail_update(int sort);
void main_page_g_back(int sort);
void main_page_ini_init(void);
extern lv_indev_t *indev_keypad;

void lvgl_osd_open(void);
void lvgl_osd_close(void);

void main_page_total_result_update(void);
void main_page_run_detail_update(int sort);

#endif
