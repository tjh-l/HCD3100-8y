
//
// Created by huailong.zhou on 22-8-11.
//

#ifndef LVGL_VOLUME_H
#define LVGL_VOLUME_H

#include <stdint.h>

//extern lv_indev_t * indev_keypad;
void create_volume();
void create_mute_icon();
void del_volume();
void set_volume1(uint8_t vol);
#ifdef LVGL_MBOX_STANDBY_SUPPORT
void close_volume_for_open_lvmbox_standby();
#endif
#endif //LVGL_VOLUME_H
