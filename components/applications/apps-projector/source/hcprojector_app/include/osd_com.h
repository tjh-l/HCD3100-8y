#ifndef __OSD_COM_H__
#define __OSD_COM_H__

//#include "list.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/font/lv_font.h"
#include "lan_str_id.h"
#include "app_config.h"
#include "hcstring_id.h"
#ifdef __cplusplus
extern "C" {
#endif

#define GET_LABEL_ID(i) (((i)&0XFFFF)>>4)
#define GET_FONT_ID(i) ((i)&0xf)
#define STORE_LABEL_AND_FONT_ID(l_id, f_id) (((l_id)<<4)|(f_id))//左边12存储label_id,4028个label_id，右边4位存储font_id,16种字库大小
#define STR_NONE 0

typedef enum{
    LANGUAGE_ENGLISH,
#if 1//(SUPPORT_OSD_SCHINESE==1)    
    LANGUAGE_CHINESE,
#endif     
#if (SUPPORT_OSD_TCHINESE==1)
    LANGUAGE_TCHINESE,
#endif
#if (SUPPORT_OSD_FRENCH==1)
    LANGUAGE_FRENCH,
#endif
#if (SUPPORT_OSD_GERMAN==1)
    LANGUAGE_GERMAN,
#endif
#if (SUPPORT_OSD_SPANISH==1)
    LANGUAGE_SPANISH,
#endif
#if (SUPPORT_OSD_PORTUGUESE==1)
    LANGUAGE_PORTUGUESE,
#endif
#if (SUPPORT_OSD_ITALIAN==1)
    LANGUAGE_ITALIAN,
#endif

#if (SUPPORT_OSD_POLISH==1)
    LANGUAGE_POLISH,
#endif
#if (SUPPORT_OSD_SWEDISH==1)
    LANGUAGE_SWEDISH,
#endif
#if (SUPPORT_OSD_FINNISH==1)
    LANGUAGE_FINNISH,
#endif
#if (SUPPORT_OSD_GREEK==1)
    LANGUAGE_GREEK,
#endif
#if (SUPPORT_OSD_DANISH==1)
    LANGUAGE_DANISH,
#endif

#if (SUPPORT_OSD_NORWEGIAN==1)
    LANGUAGE_NORWEGIAN,
#endif

#if (SUPPORT_OSD_HUNGARY==1)
    LANGUAGE_HUNGARY,
#endif

#if (SUPPORT_OSD_HEBREW==1)
    LANGUAGE_HEBREW,
#endif

#if (SUPPORT_OSD_RUSSIAN==1)
    LANGUAGE_RUSSIAN,
#endif

#if (SUPPORT_OSD_VIETNAMESE==1)
     LANGUAGE_VIETNAMESE,
#endif

#if (SUPPORT_OSD_THAI==1)
     LANGUAGE_THAI,
#endif

#if (SUPPORT_OSD_ARABIC==1)
    LANGUAGE_ARABIC,
#endif
#if (SUPPORT_OSD_JAPANESE==1)
    LANGUAGE_JAPANESE,
#endif
#if (SUPPORT_OSD_KOREAN==1)
    LANGUAGE_KOREAN,
#endif
#if (SUPPORT_OSD_INDONESIAN==1)
    LANGUAGE_INDONESIAN,
#endif
#if (SUPPORT_OSD_DUTCH==1)
    LANGUAGE_DUTCH,
#endif
#if (SUPPORT_OSD_TURKEY==1)
    LANGUAGE_TURKEY,
#endif
 //   LANGUAGE_SPANISH,
	LANGUAGE_MAX  // the max num of language supported
}language_type_t;

#ifdef LVGL_RESOLUTION_240P_SUPPORT
LV_FONT_DECLARE(SiYuanHeiTi_Light_3500_12_1b)
LV_FONT_DECLARE(font_china_18)
LV_FONT_DECLARE(font_china_14)
LV_FONT_DECLARE(font_china_10)
#else 
LV_FONT_DECLARE(SiYuanHeiTi_Light_3000_28_1b)
LV_FONT_DECLARE(lv_font_montserrat_28);
LV_FONT_DECLARE(lv_font_montserrat_22);
LV_FONT_DECLARE(font_china_26)
LV_FONT_DECLARE(font_china_36)
LV_FONT_DECLARE(font_china_18)
LV_FONT_DECLARE(font_china_14)
#endif
#if (SUPPORT_OSD_TCHINESE==1)
LV_FONT_DECLARE(font_trad_china_36)
LV_FONT_DECLARE(font_trad_china_28)
LV_FONT_DECLARE(font_trad_china_26)
LV_FONT_DECLARE(font_trad_china_14)
#endif
#if (SUPPORT_OSD_FRENCH==1)
LV_FONT_DECLARE(french_36)
LV_FONT_DECLARE(french_28)
LV_FONT_DECLARE(french_26)
LV_FONT_DECLARE(french_14)
#endif

#if (SUPPORT_OSD_GERMAN==1)
LV_FONT_DECLARE(font_german_36)
LV_FONT_DECLARE(font_german_28)
LV_FONT_DECLARE(font_german_26)
LV_FONT_DECLARE(font_german_14)
#endif

#if (SUPPORT_OSD_SPANISH==1)
LV_FONT_DECLARE(font_spanish_36)
LV_FONT_DECLARE(font_spanish_28)
LV_FONT_DECLARE(font_spanish_26)
LV_FONT_DECLARE(font_spanish_14)
#endif

#if (SUPPORT_OSD_PORTUGUESE==1)
LV_FONT_DECLARE(font_Portuguese_36)
LV_FONT_DECLARE(font_Portuguese_28)
LV_FONT_DECLARE(font_Portuguese_26)
LV_FONT_DECLARE(font_Portuguese_14)
#endif

#if (SUPPORT_OSD_ITALIAN==1)
LV_FONT_DECLARE(font_Italian_36)
LV_FONT_DECLARE(font_Italian_28)
LV_FONT_DECLARE(font_Italian_26)
LV_FONT_DECLARE(font_Italian_14)
#endif

#if (SUPPORT_OSD_POLISH==1)
LV_FONT_DECLARE(Polish_36)
LV_FONT_DECLARE(Polish_28)
LV_FONT_DECLARE(Polish_26)
LV_FONT_DECLARE(Polish_14)
#endif

#if (SUPPORT_OSD_SWEDISH==1)
LV_FONT_DECLARE(Swedish_36)
LV_FONT_DECLARE(Swedish_28)
LV_FONT_DECLARE(Swedish_26)
LV_FONT_DECLARE(Swedish_14)
#endif

#if (SUPPORT_OSD_FINNISH==1)
LV_FONT_DECLARE(Finnish_36)
LV_FONT_DECLARE(Finnish_28)
LV_FONT_DECLARE(Finnish_26)
LV_FONT_DECLARE(Finnish_14)
#endif

#if (SUPPORT_OSD_GREEK==1)
LV_FONT_DECLARE(Greek_36)
LV_FONT_DECLARE(Greek_28)
LV_FONT_DECLARE(Greek_26)
LV_FONT_DECLARE(Greek_14)
#endif
#if (SUPPORT_OSD_DANISH==1)
LV_FONT_DECLARE(Danish_36)
LV_FONT_DECLARE(Danish_28)
LV_FONT_DECLARE(Danish_26)
LV_FONT_DECLARE(Danish_14)
#endif

#if (SUPPORT_OSD_NORWEGIAN==1)
LV_FONT_DECLARE(norwegian_36)
LV_FONT_DECLARE(norwegian_28)
LV_FONT_DECLARE(norwegian_26)
LV_FONT_DECLARE(norwegian_14)
#endif

#if (SUPPORT_OSD_HUNGARY==1)
LV_FONT_DECLARE(Hungary_36)
LV_FONT_DECLARE(Hungary_28)
LV_FONT_DECLARE(Hungary_26)
LV_FONT_DECLARE(Hungary_14)
#endif

#if (SUPPORT_OSD_HEBREW==1)
LV_FONT_DECLARE(Hebrew_36)
LV_FONT_DECLARE(Hebrew_28)
LV_FONT_DECLARE(Hebrew_26)
LV_FONT_DECLARE(Hebrew_14)
#endif

#if (SUPPORT_OSD_RUSSIAN==1)
LV_FONT_DECLARE(Russian_36)
LV_FONT_DECLARE(Russian_28)
LV_FONT_DECLARE(Russian_26)
LV_FONT_DECLARE(Russian_14)
#endif

#if (SUPPORT_OSD_VIETNAMESE==1)
LV_FONT_DECLARE(Vietnamese_36)
LV_FONT_DECLARE(Vietnamese_28)
LV_FONT_DECLARE(Vietnamese_26)
LV_FONT_DECLARE(Vietnamese_14)
#endif

#if (SUPPORT_OSD_THAI==1)
LV_FONT_DECLARE(Thai_36)
LV_FONT_DECLARE(Thai_28)
LV_FONT_DECLARE(Thai_26)
LV_FONT_DECLARE(Thai_14)
#endif

#if (SUPPORT_OSD_ARABIC==1)
LV_FONT_DECLARE(Arabic_36)
LV_FONT_DECLARE(Arabic_28)
LV_FONT_DECLARE(Arabic_26)
LV_FONT_DECLARE(Arabic_14)
#endif

#if (SUPPORT_OSD_JAPANESE==1)
LV_FONT_DECLARE(Japanese_36)
LV_FONT_DECLARE(Japanese_28)
LV_FONT_DECLARE(Japanese_26)
LV_FONT_DECLARE(Japanese_14)
#endif

#if (SUPPORT_OSD_KOREAN==1)
LV_FONT_DECLARE(Korean_36)
LV_FONT_DECLARE(Korean_28)
LV_FONT_DECLARE(Korean_26)
LV_FONT_DECLARE(Korean_14)
#endif

#if (SUPPORT_OSD_INDONESIAN==1)
LV_FONT_DECLARE(Indonesian_36)
LV_FONT_DECLARE(Indonesian_28)
LV_FONT_DECLARE(Indonesian_26)
LV_FONT_DECLARE(Indonesian_14)
#endif

#if (SUPPORT_OSD_DUTCH==1)
LV_FONT_DECLARE(Dutch_36)
LV_FONT_DECLARE(Dutch_28)
LV_FONT_DECLARE(Dutch_26)
LV_FONT_DECLARE(Dutch_14)
#endif

#if (SUPPORT_OSD_TURKEY==1)
LV_FONT_DECLARE(Turkey_36)
LV_FONT_DECLARE(Turkey_28)
LV_FONT_DECLARE(Turkey_26)
LV_FONT_DECLARE(Turkey_14)
#endif


#define CONVSTR_MAXSIZE 512

#define FONT_SIZE_LARGE (&lv_font_montserrat_28) //large font size
#define FONT_SIZE_MID (&lv_font_montserrat_22) //middle font size
#define FONT_SIZE_SMALL (&lv_font_montserrat_18) //small font size

#define COLOR_WHITE lv_color_hex(0xFFFFFF) //white
#define COLOR_RED lv_color_hex(0xC00000) //red
#define COLOR_GREEN lv_color_hex(0x00FF00) //deep green
#define COLOR_YELLOW lv_color_hex(0xE0E000) //yellow

#define COLOR_BLUE lv_color_hex(0x4B50B0) //0x4B50B0

#define FONT_COLOR_HIGH lv_color_hex(0xE0E000) //highlight color, yelow

#define FONT_COLOR_GRAY lv_color_hex(0x7F7F7F) //grey

#define COLOR_DEEP_GREY lv_color_hex(0x303030) 
//#define COLOR_FRENCH_GREY lv_color_hex(0x505050) 
#define COLOR_FRENCH_GREY lv_color_hex(0x303841) 

extern lv_style_t m_text_style_mid_normal;
extern lv_style_t m_text_style_mid_high;
extern lv_style_t m_text_style_mid_disable;
extern lv_style_t m_text_style_left_normal;
extern lv_style_t m_text_style_left_high;
extern lv_style_t m_text_style_left_disable;
extern lv_style_t m_text_style_right_normal;
extern lv_style_t m_text_style_right_high;
extern lv_style_t m_text_style_right_disable;

#define TEXT_STY_MID_NORMAL				(&m_text_style_mid_normal)
#define TEXT_STY_MID_HIGH				(&m_text_style_mid_high)
#define TEXT_STY_MID_DISABLE			(&m_text_style_mid_disable)
#define TEXT_STY_LEFT_NORMAL			(&m_text_style_left_normal)
#define TEXT_STY_LEFT_HIGH				(&m_text_style_left_high)
#define TEXT_STY_LEFT_DISABLE			(&m_text_style_left_disable)
#define TEXT_STY_RIGHT_NORMAL			(&m_text_style_right_normal)
#define TEXT_STY_RIGHT_HIGH				(&m_text_style_right_high)
#define TEXT_STY_RIGHT_DISABLE			(&m_text_style_right_disable)

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define ENG_BIG lv_font_montserrat_18
    #define ENG_MID lv_font_montserrat_14
    #define ENG_NORMAL lv_font_montserrat_12 
    #define ENG_SMALL lv_font_montserrat_10
    #define MSGBOX_Y_OFS   -20
    #define LISTFONT_3000   SiYuanHeiTi_Light_3500_12_1b
    #define CHN_LARGE   font_china_18
    #define CHN_MID     SiYuanHeiTi_Light_3500_12_1b
    #define CHN_NORMAL  SiYuanHeiTi_Light_3500_12_1b
    #define CHN_SMALL   CHN_NORMAL // font_china_10
#else
    #define ENG_BIG lv_font_montserrat_36
    #define ENG_MID lv_font_montserrat_28
    #define ENG_NORMAL lv_font_montserrat_26 
    #define ENG_SMALL lv_font_montserrat_18
    #define MSGBOX_Y_OFS   -100
    #define LISTFONT_3000   SiYuanHeiTi_Light_3000_28_1b
    #define CHN_LARGE   font_china_36
    #define CHN_MID     LISTFONT_3000
    #define CHN_NORMAL  font_china_26
    #define CHN_SMALL   font_china_18//CHN_MID //font_china_14

#endif

/**
 * The structure is to adjust the position information of the list(LV widgets)
 */
#if 1
typedef struct{
    uint16_t top;        //In a page, the start index of file list
    uint16_t depth;      //In a page, the max count of list items 
    uint16_t cur_pos;        //the current index of file list
    uint16_t new_pos;        //the new index of file list
    uint16_t select;        //the select index of file list
    uint16_t count; // all the count(dirs and files) of file list
}obj_list_ctrl_t;

#else
typedef struct{
    uint16_t start;      //In a page, the start index of file list
    uint16_t end;        //In a page, the end index of file list
    uint16_t pos;        //In a page, the current position of file list. start <= pos <= end.
    uint16_t list_count; // all the count(dirs and files) of file list
}obj_list_ctrl_t;
#endif

typedef void (*msg_timeout_func)(void *user_data);
typedef void (*user_msgbox_cb)(int btn_sel, void *user_data);


lv_obj_t *obj_img_open(void *parent, void *bitmap, int x, int y);
lv_obj_t *obj_label_open(void *parent, int x, int y, int w, char *str);
void osd_draw_background(void *parent, bool if_transp);
char *osd_get_string(int string_id);

language_type_t obj_get_language_type(void);

void osd_list_ctrl_reset(obj_list_ctrl_t *list_ctrl, uint16_t count, uint16_t item_count, uint16_t new_pos);
bool osd_list_ctrl_update(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t old_pos, uint16_t *new_pos);
bool osd_list_ctrl_update2(obj_list_ctrl_t *list_ctrl, uint16_t vkey, uint16_t old_pos, uint16_t *new_pos);
language_type_t osd_get_language(void);
void osd_obj_com_set(void);
bool osd_list_ctrl_shift(obj_list_ctrl_t *list_ctrl, int16_t shift, uint16_t *new_top, uint16_t *new_pos);

void win_msgbox_msg_fmt_open(uint32_t timeout, msg_timeout_func timeout_func, void *user_data, int str_msg_id, ...);
void win_msgbox_msg_open(int str_msg_id, uint32_t timeout, msg_timeout_func timeout_func, void *user_data);
void win_msgbox_msg_close(void);
void win_msgbox_btn_open(lv_obj_t* parent, char *str_msg, user_msgbox_cb cb, void *user_data);

void win_data_buffing_open(void *parent);
void win_data_buffing_update(int percent);
void win_data_buffing_close(void);

void key_set_group(lv_group_t *key_group);
lv_font_t *osd_font_get(int font_idx);
lv_font_t *osd_font_get_by_langid(int lang_id, int font_idx);
#define DEFAULT_LV_DISP_W 1280
#define DEFAULT_LV_DISP_H 720
int lv_xpixel_transform(int x);
int lv_ypixel_transform(int y);
void win_data_buffing_label_set(char* user_str);
bool win_data_buffing_is_open(void);
void win_msgbox_msg_open_on_top(int str_msg_id, uint32_t timeout, msg_timeout_func timeout_func, void *user_data);
void win_msgbox_msg_close_on_top(void);
int string_fmt_conv_to_utf8(unsigned char* buff,char* out_buff);
int string_dec_conv2_utf8(unsigned char* buff,char* out_buff, int out_size);
void win_clear_popup(void);

typedef enum Font_size
{
    FONT_LARGE=0,
    FONT_MID,
    FONT_NORMAL,
    FONT_SMALL,
}Font_size_e;

void set_label_text2(lv_obj_t* obj, uint16_t label_id, uint16_t font_id);

#ifdef LVGL_MBOX_STANDBY_SUPPORT
void win_open_lvmbox_standby(void);
void win_del_lvmbox_standby(void);
#endif

void api_scr_go_back(void);
void win_msgbox_msg_set_pos(lv_align_t align,lv_coord_t x_ofs, lv_coord_t y_ofs);
bool win_msgbox_msg_is_open(void);


bool sys_media_playing(void);

#ifdef FOCUSING_ANIMATION_ON
void focus_msgbox_msg_open(int flag);
void focus_msgbox_msg_close(void);
void pause_anim_exec_cb();
#endif
#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
