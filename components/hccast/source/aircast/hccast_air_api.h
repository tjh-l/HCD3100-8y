#ifndef __HCCAST_AIR_API_H__
#define __HCCAST_AIR_API_H__

typedef int (*air_api_service_init)(void);
typedef int (*air_api_service_start)(char *name, char* ifname);
typedef int (*air_api_service_stop)(void);
typedef void (*air_api_set_event_callback)(evt_cb event_cb);
typedef int (*air_api_set_resolution)(int width, int height, int fps);
typedef void (*air_api_event_notify)(int event_type, void *param);
typedef int (*air_api_ioctl)(int req_cmd, void *param1, void *param2);
typedef int (*air_p2p_start)(char *if_name, int ch);
typedef int (*air_p2p_stop)(void);
typedef int (*air_p2p_set_channel)(int channel);
typedef int (*air_p2p_service_init)(void);

int hccast_air_api_service_init(void);
int hccast_air_api_service_start(char *name, char* ifname);
int hccast_air_api_service_stop(void);
int hccast_air_api_set_notifier(evt_cb event_cb);
int hccast_air_api_set_resolution(int width, int height, int fps);
int hccast_air_api_event_notify(int event_type, void *param);
int hccast_air_api_ioctl(int req_cmd, void *param1, void *param2);
int hccast_air_api_p2p_start(char *intf_name, int chan_num);
int hccast_air_api_p2p_stop(void);
int hccast_air_api_p2p_set_channel(int channel);
int hccast_air_api_p2p_service_init(void);
int hccast_air_api_init(void);
int hccast_air_is_playing_music(void);
int hccast_air_media_state_set(int type, void *param);
int hccast_air_mirror_fps_get(void);
int hccast_air_mirror_stat_get(void);
int hccast_air_url_skip_get(void);
int hccast_air_url_skip_set(int skip);
int hccast_air_event_notify(int msg_type, void* in, void* out);

#endif