#ifndef __OSD_COM_H__
#define __OSD_COM_H__

#include "app_config.h"
#include "lvgl/lvgl.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*msg_timeout_func)(void *user_data);
typedef void (*user_msgbox_cb)(int btn_sel, void *user_data);

void win_msgbox_passfail_open(lv_obj_t *parent, char *str_msg, user_msgbox_cb cb, void *user_data);
void win_msgbox_ok_open(lv_obj_t *parent, char *str_msg, user_msgbox_cb cb, void *user_data);
void win_msgbox_btn_close(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif