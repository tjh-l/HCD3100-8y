//
// Created by huailong.zhou on 22-8-11.
//

#ifndef LVGL_SETUP_H
#define LVGL_SETUP_H

#include "lvgl/lvgl.h"
#include "../screen.h"

#include <sys/types.h>
#ifdef __HCRTOS__
#include <kernel/lib/fdt_api.h>
#endif

#define STR_LANGUAGE(A) #A

#define HEAD_H 30
#define NEW_WIDGET_LINE_NUM 3

#define VIDEO_DELAY_MIN_V 20
#define VIDEO_DELAY_MAX_V 1000

#define X_AMENDMENT 2
#define Y_AMENDMENT 1

enum  tab_id {
    TAB_PICTURE = 1,
    TAB_SOUND,
    TAB_SETTING,
#ifdef BLUETOOTH_SUPPORT
    TAB_BT,
#endif
#ifdef KEYSTONE_SUPPORT
    TAB_KEYSTONE,
#endif    
    TAB_MAX,
};

typedef enum list_sub_param_type_{
    LIST_PARAM_TYPE_STR,
    LIST_PARAM_TYPE_INT
} list_sub_param_type;

typedef union list_sub_param_
{
    char* str;
    int str_id;
} list_sub_param;


typedef struct mode_items_
{
    lv_obj_t *obj;
    int index;
} mode_items;

#ifdef SYS_ZOOM_SUPPORT
typedef enum mainlayer_scale_t_
{
    MAINLAYER_SCALE_TYPE_LOCAL,
    MAINLAYER_SCALE_TYPE_HDMI,
    MAINLAYER_SCALE_TYPE_CVBS,
    MAINLAYER_SCALE_TYPE_CAST,

    MAINLAYER_SCALE_TYPE_NORMAL,
} mainlayer_scale_t;
#endif

typedef struct {
    //const char* name;
    uint16_t name;
    union {
        //const char* v1;
        int16_t v1;
        int16_t v2;
    } value;
    
    bool is_number;
    bool is_disabled;
    void (*event_func)(lv_event_t*);
    bool is_hide;
} choose_item;

typedef void(*widget)(lv_obj_t* btn);
typedef int(*btn_matrix_func)(int);
typedef void(*focus_next_or_pre)(lv_group_t*);
typedef bool(*run_cb)(void);

typedef struct {
    uint16_t len;
    uint8_t* change_lang;
    const char** str_p_p;
    const char **str_p_s;
} btnmatrix_p;

typedef struct {
    uint16_t len;
    //uint8_t* change_lang;
    const char** str_p_p;
    int *str_id_vec;
} btnmatrix_p1;

typedef struct{
    lv_font_t **font_pp;
    char **lang_pp;
} label_p;

typedef enum{
    NO_USB,
    NO_SOFTWARE,
    READY_UPDATE,
} SOFTWARE_UPDATE;

typedef enum _E_BT_SCAN_STATUS_
{
    BT_SCAN_STATUS_DEFAULT,
    BT_SCAN_STATUS_GET_DATA_SEARCHING,
    BT_SCAN_STATUS_GET_DATA_SEARCHED,
    BT_SCAN_STATUS_GET_DATA_FINISHED,
}bt_scan_status;

typedef enum _BT_DEV_POWER_STATUS_
{
    BT_DEV_POWER_STATUS_DEFAULT=0,
    BT_DEV_POWER_STATUS_WORK,
}bt_dev_power_status;

typedef char* str_n_vec[2];
typedef str_n_vec * str_n_vec_p;


#define INIT_STYLE_BG(STYLE) \
lv_style_init(STYLE);\
lv_style_set_pad_all(STYLE, 0);\
lv_style_set_pad_gap(STYLE, 0);\
lv_style_set_border_width(STYLE, 0);\
lv_style_set_outline_width(STYLE, 0);


#define SOURCE_STYLE_BG(STYLE) \
 lv_style_init(STYLE);\
lv_style_set_radius(STYLE, 0);\
lv_style_set_border_width(STYLE, 1);\
lv_style_set_border_opa(STYLE, LV_OPA_50);\
lv_style_set_border_color(STYLE, lv_color_make(140,140,198));\
lv_style_set_bg_color(STYLE, lv_color_make(61,101,177));\
lv_style_set_border_side(STYLE, LV_BORDER_SIDE_INTERNAL);\
lv_style_set_text_color(STYLE,lv_color_make(106,102,102));
#define NEW_SOURCE_STYLE_BG(STYLE) \
 lv_style_init(STYLE);\
lv_style_set_radius(STYLE, 0);\
lv_style_set_border_width(STYLE, 1);\
lv_style_set_border_opa(STYLE, LV_OPA_50);\
lv_style_set_border_color(STYLE, lv_color_make(140,140,198));\
lv_style_set_bg_color(STYLE, lv_color_make(61,101,177));\
lv_style_set_border_side(STYLE, LV_BORDER_SIDE_INTERNAL);\
lv_style_set_text_color(STYLE,lv_color_make(255,255,255));
#define set_pad_and_border_and_outline(obj) do { \
    lv_obj_set_style_pad_hor(obj, 0, 0);\
    lv_obj_set_style_border_width(obj, 0, 0); \
     lv_obj_set_style_outline_width(obj,0,0);\
}while(0)

#define INIT_STYLE_ITEM(STYLE) \
 lv_style_init(STYLE);\
lv_style_set_radius(STYLE, 0);\
lv_style_set_border_width(STYLE, 1);\
lv_style_set_border_opa(STYLE, LV_OPA_50);\
lv_style_set_border_color(STYLE, lv_color_make(140,140,198));\
lv_style_set_bg_color(STYLE, lv_color_make(101,101,177));\
lv_style_set_border_side(STYLE, LV_BORDER_SIDE_INTERNAL);

// #define set_pad_and_border_and_outline(obj) do { \
//     lv_obj_set_style_pad_hor(obj, 0, 0);\
//     lv_obj_set_style_border_width(obj, 0, 0); \
//      lv_obj_set_style_outline_width(obj,0,0);\
// } while(0)


LV_IMG_DECLARE(MAINMENU_IMG_PICTURE)
LV_IMG_DECLARE(MAINMENU_IMG_PICTURE_FOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_PICTURE_S_UNFOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_OPTIONS_S_UNFOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_OPTION_FOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_AUDIO_S_UNFOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_AUDIO_FOCUS)
LV_IMG_DECLARE(MAINMENU_IMG_AUDIO)
LV_IMG_DECLARE(MAINMENU_IMG_OPTIONS)
LV_IMG_DECLARE(MENU_IMG_LIST_MENU)
LV_IMG_DECLARE(MENU_IMG_POP_UP_RIGHT_ARROW)
LV_IMG_DECLARE(MENU_IMG_POP_UP_LEFT_ARROW)
LV_IMG_DECLARE(MENU_IMG_LIST_TABLE)
LV_IMG_DECLARE(MENU_IMG_LIST_OK)
LV_IMG_DECLARE(MENU_IMG_LIST_MOVE)
LV_IMG_DECLARE(bt_unfocus)
LV_IMG_DECLARE(bt_focus)
LV_IMG_DECLARE(keystone_unfocus)
LV_IMG_DECLARE(keystone_focus)



void create_balance_ball(lv_obj_t* parent, lv_coord_t radius, lv_coord_t width);
lv_obj_t * create_display_bar_widget(lv_obj_t *parent, int w, int h);
lv_obj_t *create_display_bar_name_part(lv_obj_t* parent,char* name, int w, int h);
lv_obj_t * create_display_bar_main(lv_obj_t* parent, int w, int h, int ball_count, int width);
lv_obj_t *create_display_bar_show(lv_obj_t* parent, int w, int h, int num);

// void set_label_text(lv_obj_t * label,int id, char* color);
// void set_label_text_with_font(lv_obj_t * label,int  id, char* color, lv_font_t* font);
void set_label_text1(lv_obj_t * label,int id, char* color);
void set_label_text_with_font1(lv_obj_t * label,int  id, char* color, lv_font_t* font);
// void set_label_text2(lv_obj_t* obj, uint16_t label_id, uint16_t font_id);
//btnmatrix_p* language_choose_add_label(lv_obj_t* label,const char* p, uint8_t len);
void language_choose_add_label1(lv_obj_t* obj, uint32_t label_id);
void language_choose_add_label_with_font(lv_obj_t* obj, uint32_t label_id, lv_font_t **font_pp);
//btnmatrix_p1* language_choose_add_btns(lv_obj_t* label, int *p, uint8_t len);

// void set_btnmatrix_language_with_font(lv_obj_t* obj, int id, lv_font_t *font);
// void set_btnmatrix_language(lv_obj_t * obj,int  id);
// void set_btns_lang_with_font(lv_obj_t* obj, int id, lv_font_t *font);
// void set_btns_lang(lv_obj_t *obj, int id);
void set_btns_lang2(lv_obj_t* btns, int len, int font_id, int *p);


const char* get_some_language_str(const char *str, int index);
const char* get_some_language_str1(str_n_vec str_v, int index);
void del_setup_slave_scr_obj(void);
extern lv_obj_t* setup_slave_root;
extern lv_obj_t* setup_root;
void keystone_screen_init(lv_obj_t*);
void focus_list_init(lv_obj_t *parent);
void turn_to_setup_root(void);
void turn_to_main_scr(void);
int set_color_temperature(int mode);
void set_keystone(int top_w, int bottom_w);
void keystone_adjust(void);
void keystone_fb_viewport_scale(void);
void video_delay_ms_turn_up(void);
void video_delay_ms_turn_down(void);
void video_delay_ms_zero(void);
void video_delay_ms_set(void);

lv_obj_t* create_widget_head(lv_obj_t* parent, int title, int h);
void create_widget_foot(lv_obj_t* parent, int h, void* user_data);
lv_obj_t *create_widget_btnmatrix(lv_obj_t *parent,int w, int h,const int* btn_map, int len);
lv_obj_t *create_widget_btnmatrix1(lv_obj_t *parent,int w, int h,const char** btn_map);
lv_obj_t *create_new_widget(int w, int h);
lv_obj_t* create_list_obj1(lv_obj_t *parent, int w, int h);
lv_obj_t* create_list_sub_text_obj3 (lv_obj_t *parent,int w, int h, char* str1);
lv_obj_t* create_list_sub_text_obj1(lv_obj_t *parent,int w, int h, int str1, int font_id);
lv_obj_t* create_list_sub_text_obj4(lv_obj_t *parent, int w, int h, int str1);

#ifdef BLUETOOTH_SUPPORT
void del_bt_wait_anim();
void setup_bt_control(void *arg1, void *arg2);
bool app_bt_is_connecting();
bool app_bt_is_scanning();
int bt_event(unsigned long event, unsigned long param);
bool app_bt_is_connected();
void bt_init();
#endif

lv_obj_t* create_message_box(char* str);
lv_obj_t* delete_message_box(lv_obj_t *obj);
lv_obj_t* create_message_box1(int msg_id, int btn_id1, int btn_id2, lv_event_cb_t cb,int w, int h);
lv_obj_t* loader_with_arc(char* str, lv_anim_exec_xcb_t exec_cb);
void set_display_zoom_when_sys_scale(void);

#ifdef SYS_ZOOM_SUPPORT
bool app_has_zoom_operation_get(void);
void app_has_zoom_operation_set(bool b);
void mainlayer_scale_type_set(mainlayer_scale_t type);
void create_scale_widget(lv_obj_t* btn);
void sys_scale_fb_adjust(void);
void sys_scala_init(void);
int get_screen_mode(void);
#endif
void auto_mode_get_rect_by_ratio(int *h, int *v, int ratio);
bool get_screen_is_vertical(void);
int get_display_x(void);
int get_display_y(void);
int get_display_h(void);
int get_display_v(void);
int get_cur_osd_x(void);
int get_cur_osd_y(void);
int get_cur_osd_h(void);
int get_cur_osd_v(void);
void factorymenu_open(lv_obj_t* e);
void factorymenu_init();
void set_enhance1(int value, uint8_t op);
int set_balance(int v);
int set_twotone(int mode, int treble, int bass);

void sound_mode_widget(lv_obj_t* btn);
void sound_output_mode_widget(lv_obj_t* e);
void sound_spdif_mode_widget(lv_obj_t* e);
void mp_eq_init();
void create_eq_widget(lv_obj_t* btn);
void osd_language_widget(lv_obj_t* e);
void software_update_widget(lv_obj_t* e);
#ifdef MANUAL_HTTP_UPGRADE
void software_network_update_widget(lv_obj_t* e);
#endif
void auto_sleep_widget(lv_obj_t *btn);
void airp2p_ch_widget(lv_obj_t *btn);
lv_obj_t* create_bt_page(lv_obj_t* parent);

void label_set_text_color(lv_obj_t* label,const char* text, char* color);
int set_auto_sleep(int mode);
void autosleep_reset_timer(void);
void sound_set_outupt_obj(void *obj);
void setup_sys_scale_set_disable(bool disable);
void slave_scr_obj_set(lv_obj_t* obj);
int airp2p_ch_translate_to_index(int channel);

#ifdef USB_AUTO_UPGRADE
int sys_upg_usb_check_init(void);
int sys_upg_usb_check_notify(void);
void del_upgrade_prompt(void);
//int sys_upg_usb_check(uint32_t timeout);
#endif
#endif //LVGL_SETUP_H
