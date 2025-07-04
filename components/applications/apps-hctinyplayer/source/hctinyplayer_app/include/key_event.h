#ifndef __KEY_H__
#define __KEY_H__

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"

#include <hcuapi/input.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USER_KEY_FLAG   (0x10000)
enum ui_key
{
    V_KEY_0,    // 0
    V_KEY_1,
    V_KEY_2,
    V_KEY_3,
    V_KEY_4,
    V_KEY_5,
    V_KEY_6,
    V_KEY_7,
    V_KEY_8,
    V_KEY_9,
    V_KEY_10,

    V_KEY_LEFT,//11
    V_KEY_RIGHT,
    V_KEY_UP,
    V_KEY_DOWN,

    V_KEY_MENU,
    V_KEY_OK,
    V_KEY_ENTER,
    V_KEY_EXIT,
    V_KEY_EXIT_ALL,
    V_KEY_CLEAR,

    V_KEY_MUTE,
    V_KEY_STATUS,
    V_KEY_TVRADIO,
    V_KEY_POWER,
    V_KEY_AUDIO,

    V_KEY_C_UP, //ch+
    V_KEY_C_DOWN,   //ch-
    V_KEY_V_UP, //vol+
    V_KEY_V_DOWN,   //vol-
    V_KEY_P_UP, //page+
    V_KEY_P_DOWN,//page-

    V_KEY_COLOR0,//V_KEY_RF
    V_KEY_COLOR1,
    V_KEY_COLOR2,
    V_KEY_COLOR3,   //V_KEY_EPG
    V_KEY_COLOR4,   //V_KEY_PAUSE

    V_KEY_PLAY, //for player
    V_KEY_PAUSE,
    V_KEY_STOP,
    V_KEY_FF,   //fast forward
    V_KEY_FB,   //fast backward
    V_KEY_SF,   //slow forward
    V_KEY_SB,   //slow backward
    V_KEY_PREV, //play previous
    V_KEY_NEXT, //play next

    V_KEY_RECORD,
    V_KEY_TIMESHIFT,

    V_KEY_RED,
    V_KEY_GREEN,
    V_KEY_YELLOW,
    V_KEY_BLUE,

    V_KEY_INFO,
    V_KEY_EDIT,
    V_KEY_TEXT,
    V_KEY_SUB,
    V_KEY_FIND,
    V_KEY_FAV,
    V_KEY_RECALL,
    V_KEY_CONTRAST,
    V_KEY_HELP,
    V_KEY_SLEEP,

    V_KEY_RF,
    V_KEY_EPG,
    V_KEY_LIST,
    V_KEY_SWITCH,
    V_KEY_PAL_NTSC,
    V_KEY_PREVIEW,
    V_KEY_RESCAN,
    V_KEY_ZOOM,
    V_KEY_MP,
    V_KEY_LAST,
    V_KEY_ROTATE,
    V_KEY_HOME,

    VKEY_NULL = 0xFFFF,
};

typedef enum
{
    KEY_TYPE_IR = 0,
    KEY_TYPE_ADC,
    KEY_TYPE_GPIO,
} KEY_TYPE_e;

typedef struct
{
    KEY_TYPE_e key_type;
    struct input_event key_event;
} KEY_MSG_s;

//key queue
#define KEY_QUEUE_MAX 5

typedef struct
{
    KEY_MSG_s key_msg[KEY_QUEUE_MAX];
    int front;
    int rear;
    int size;
} KEY_QUEUE_s;

uint32_t key_convert2_vkey(uint32_t lv_key);
uint32_t key_convert_vkey(uint32_t lv_key);
int key_regist(lv_group_t *group);
void api_key_get_init();
KEY_MSG_s *api_key_msg_get(void);
bool key_queue_empty(KEY_QUEUE_s *q);
bool key_queue_write(KEY_QUEUE_s *q, KEY_MSG_s *x);
bool key_queue_read(KEY_QUEUE_s *q, KEY_MSG_s **x);
bool key_queue_head_get(KEY_QUEUE_s *q, KEY_MSG_s **x);
int key_queue_num(KEY_QUEUE_s *q);
void api_key_queue_send(uint32_t act_key, int32_t press);
void api_key_disable(char disable);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __KEY_H__
