#ifndef VOLUME_H
#define VOLUME_H

#define set_pad_and_border_and_outline(obj) do { \
    lv_obj_set_style_pad_hor(obj, 0, 0);\
    lv_obj_set_style_border_width(obj, 0, 0); \
    lv_obj_set_style_outline_width(obj,0,0);\
} while(0)

void create_volume();
void create_mute_icon();
void del_volume();
void volume_screen_init(void);

#endif // VOLUME_H
