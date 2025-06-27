#ifndef __HCCAST_AIR_SERVICE_H__
#define __HCCAST_AIR_SERVICE_H__

#define HCCAST_AIR_PIN_LEN (5)

typedef enum
{
    /* parameters get */
    HCCAST_AIR_GET_SERVICE_NAME = 1,
    HCCAST_AIR_GET_NETWORK_DEVICE,
    HCCAST_AIR_GET_MIRROR_MODE,
    HCCAST_AIR_GET_4K_MODE,
    HCCAST_AIR_GET_NETWORK_STATUS,
    HCCAST_AIR_GET_AIRP2P_PIN,
    HCCAST_AIR_GET_MIRROR_QUICK_MODE_NUM,
    HCCAST_AIR_GET_MIRROR_ROTATION_INFO,
    HCCAST_AIR_GET_ABDISCONNECT_STOP_PLAY_EN,
    HCCAST_AIR_GET_PREVIEW_INFO,
    HCCAST_AIR_GET_VIDEO_CONFIG,

    /* event notify or parameters set */
    HCCAST_AIR_MIRROR_START = 100,
    HCCAST_AIR_MIRROR_STOP,
    HCCAST_AIR_AUDIO_START,
    HCCAST_AIR_AUDIO_STOP,
    HCCAST_AIR_INVALID_CERT,
    HCCAST_AIR_BAD_NETWORK,
    HCCAST_AIR_HOSTAP_MODE_SKIP_URL,
    HCCAST_AIR_FAKE_LIB,
    HCCAST_AIR_URL_ENABLE_SET_DEFAULT_VOL,
    HCCAST_AIR_SET_AUDIO_VOL,
    HCCAST_AIR_PHONE_MODEL,
    HCCAST_AIR_PHONE_CONNECT,
    HCCAST_AIR_PHONE_DISCONNECT,
    HCCAST_AIR_P2P_INVALID_CERT,
    HCCAST_AIR_MIRROR_SCREEN_DETECT_NOTIFY,
} hccast_air_event_e;

typedef enum
{
    HCCAST_AIR_SCREEN_ROTATE_0,
    HCCAST_AIR_SCREEN_ROTATE_270,
    HCCAST_AIR_SCREEN_ROTATE_90,
    HCCAST_AIR_SCREEN_ROTATE_180,
} hccast_air_rotate_type_e;

typedef enum
{
    HCCAST_AIR_MODE_MIRROR_ONLY,
    HCCAST_AIR_MODE_MIRROR_STREAM,
} hccast_air_mode_e;

typedef struct
{
    int src_w;
    int src_h;
    int rotate_angle;
    int flip_mode;
} hccast_air_rotation_t;

typedef struct
{
    unsigned short x;
    unsigned short y;
    unsigned short w;
    unsigned short h;
} hccast_air_rect_t;

typedef struct
{
    hccast_air_rect_t src_rect;
    hccast_air_rect_t dst_rect;
    int preview_en;
} hccast_air_deview_t;

typedef int (*hccast_air_event_callback) (hccast_air_event_e event_type, void* arg1, void* arg2);

#ifdef __cplusplus
extern "C" {
#endif

int hccast_air_service_init(hccast_air_event_callback aircast_cb);
int hccast_air_service_start(void);
int hccast_air_service_stop(void);
int hccast_air_service_is_start(void);
int hccast_air_mdnssd_start(void);
int hccast_air_mdnssd_stop(void);
int hccast_air_p2p_start(char *if_name, int ch);
int hccast_air_p2p_stop(void);
int hccast_air_set_resolution(unsigned int width, unsigned int height, unsigned int fps);
int hccast_air_connect_state_get(void);
int hccast_air_audio_state_get(void);
int hccast_air_stop_playing(void);
int hccast_air_p2p_channel_set(int channel);
int hccast_air_preemption_set(int enable);
int hccast_air_p2p_pin_set(char *pin, int pk_random);

// For debug use.
int hccast_air_es_dump_start(char *folder);
int hccast_air_es_dump_stop(void);

#ifdef __cplusplus
}
#endif

#endif
