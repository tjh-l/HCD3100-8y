#ifndef __HCCAST_MIRACAST_SERVICE_H__
#define __HCCAST_MIRACAST_SERVICE_H__

#define MIRA_NAME_LEN (64)

typedef enum
{
    HCCAST_MIRA_RES_720P30,
    HCCAST_MIRA_RES_1080P30,
    HCCAST_MIRA_RES_480P60,
    HCCAST_MIRA_RES_VESA1400,
    HCCAST_MIRA_RES_1080P60,
    HCCAST_MIRA_RES_1080F30,
    HCCAST_MIRA_RES_2160p30
} hccast_mira_res_e;

typedef enum
{
    HCCAST_MIRA_SCREEN_ROTATE_0,
    HCCAST_MIRA_SCREEN_ROTATE_270,
    HCCAST_MIRA_SCREEN_ROTATE_90,
    HCCAST_MIRA_SCREEN_ROTATE_180,
} hccast_mira_rotate_type_e;

typedef enum
{
    /* parameters get */
    HCCAST_MIRA_GET_DEVICE_NAME = 1,
    HCCAST_MIRA_GET_CUR_WIFI_INFO,
    HCCAST_MIRA_GET_MIRROR_QUICK_MODE_NUM,
    HCCAST_MIRA_GET_MIRROR_ROTATION_INFO,
    HCCAST_MIRA_GET_CONTINUE_ON_ERROR,
    HCCAST_MIRA_GET_DEVICE_PARAM,
    HCCAST_MIRA_GET_PREVIEW_INFO,
    HCCAST_MIRA_GET_VIDEO_CONFIG,
    /* event notify or parameters set */
    HCCAST_MIRA_SSID_DONE = 100,
    HCCAST_MIRA_CONNECT,
    HCCAST_MIRA_CONNECTED,
    HCCAST_MIRA_START_DISP,
    HCCAST_MIRA_DISCONNECT,
    HCCAST_MIRA_STOP_DISP,
    HCCAST_MIRA_START_FIRST_FRAME_DISP,
    HCCAST_MIRA_RESET,
    HCCAST_MIRA_MIRROR_SCREEN_DETECT_NOTIFY,
    HCCAST_MIRA_GOT_IP,
    HCCAST_MIRA_UIBC_ENABLE,
    HCCAST_MIRA_UIBC_DISABLE,
} hccast_mira_event_e;

typedef int (*hccast_mira_event_callback)(hccast_mira_event_e event, void* in, void* out);

typedef struct
{
    unsigned short x;       //!< Horizontal start point.
    unsigned short y;       //!< Vertical start point.
    unsigned short w;       //!< Horizontal size.
    unsigned short h;       //!< Vertical size.
} hccast_mira_rect_t;

typedef struct
{
    hccast_mira_rect_t src_rect;
    hccast_mira_rect_t dst_rect;
    int dis_active_mode;
} hccast_mira_zoom_info_t;

typedef struct
{
    int rotate_angle;
    int flip_mode;
} hccast_mira_rotation_t;

typedef struct
{
    hccast_mira_rect_t src_rect;
    hccast_mira_rect_t dst_rect;
    int preview_en;
} hccast_mira_preview_info_t;

typedef enum
{
    HCCAST_MIRA_CAT_NONE    = 0,
    HCCAST_MIRA_CAT_GENERIC = 1,    // bitmap (0), not support
    HCCAST_MIRA_CAT_HID     = 2,    // bitmap (1)
} hccast_mira_cat_t;

typedef struct {
    int valid;  // 0=not valid, 1=valid
    int cat;    // 0=Generic, 1=HIDC
    int type;   // reference Specification Table 24. 0=keyboard, 1=mouse, 2=SingleTouch, 3=MultiTouch, 4=Joystick 5=Camera, 6=Gesture, 7=RemoteControl
    int path;   // Generic dont need, HID reference Specification Table 23, 0=interface, 1=USB, 2=BT, 3=Zigbee, 4=WI-FI, 5=No-SP
} hccast_mira_dev_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * It starts the Miracast service.
 *
 * @return 0: success; <0: failed.
 */
int hccast_mira_service_start(void);

/**
 * It stops the Miracast service.
 *
 * @return 0
 */
int hccast_mira_service_stop(void);

/**
 * It disconnects the Miracast connection.
 *
 * @return 0
 */
int hccast_mira_disconnect(void);

int hccast_mira_player_init(void);
int hccast_mira_get_stat(void);
int hccast_mira_get_restart_state(void);
int hccast_mira_service_init(hccast_mira_event_callback func);
int hccast_mira_service_uninit(void);
int hccast_mira_service_set_resolution(hccast_mira_res_e res);

int hccast_mira_uibc_get_supported(hccast_mira_cat_t *cat);
int hccast_mira_uibc_get_port(void);
int hccast_mira_uibc_add_device(hccast_mira_dev_t *dev);
int hccast_mira_uibc_disable_get(int *en);
int hccast_mira_uibc_disable_set(int *en);
int hccast_mira_uibc_is_supported_device(int cat, int dev);

// For debug use
int hccast_mira_es_dump_start(char *folder);
int hccast_mira_es_dump_stop(void);

#ifdef __cplusplus
}
#endif

#endif
