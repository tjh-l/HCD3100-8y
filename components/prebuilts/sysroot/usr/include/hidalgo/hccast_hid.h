#ifndef __HCCAST_HID_H__
#define __HCCAST_HID_H__

#include <hcuapi/input.h>

#define HCCAST_HID_TYPE_IUM         (1)
#define HCCAST_HID_TYPE_AUM         (2)

#define HCCAST_HID_TP_NORMAL        (1)
#define HCCAST_HID_TP_MOUSE         (2)

#define HCCAST_HID_EVT_MOUSE        (1)
#define HCCAST_HID_EVT_KEYBOARD     (2)
#define HCCAST_HID_EVT_TP_NORMAL    (3)
#define HCCAST_HID_EVT_TP_MOUSE     (4)

typedef struct
{
    unsigned int tp_width;
    unsigned int tp_height;
    unsigned int tp_h_inv;
    unsigned int tp_v_inv;
    unsigned int tp_type;
    unsigned int scr_rotate;
} hccast_hid_args_t;

typedef struct
{
    unsigned short w;
    unsigned short h;
    unsigned int full_screen;
    unsigned int cast_type;
    unsigned int rotate;
    unsigned int flip;
} hccast_hid_video_info_t;

typedef int (*hccast_hid_event_cb)(unsigned int, char *, unsigned int);

int hccast_hid_start(hccast_hid_args_t *arg, hccast_hid_event_cb evt_cb);
int hccast_hid_stop();
int hccast_hid_video_info_set(hccast_hid_video_info_t *video_info);
int hccast_hid_event_feed(struct input_event *event);

#endif

