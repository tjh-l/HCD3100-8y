/**
 * @file cast_api.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __CAST_API_H__
#define __CAST_API_H__

#ifdef DLNA_SUPPORT
#include <hccast/hccast_dlna.h>
#endif

#ifdef DIAL_SUPPORT
#include <hccast/hccast_dial.h>
#endif

#ifdef AIRCAST_SUPPORT
#include <hccast/hccast_air.h>
#endif

#ifdef MIRACAST_SUPPORT
#include <hccast/hccast_mira.h>
#endif

#ifdef USBMIRROR_SUPPORT
#include <hccast/hccast_um.h>
#endif

#include <hccast/hccast_scene.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    CAST_TYPE_AIRCAST = 0,
    CAST_TYPE_DLNA,
    CAST_TYPE_MIRACAST,

    CAST_TYPE_NONE,
}cast_type_t;

typedef enum
{
    CAST_DIAL_CONN_NONE = 0,
    CAST_DIAL_CONN_CONNECTING,
    CAST_DIAL_CONN_CONNECTED,
}cast_dial_conn_e;

int cast_get_service_name(cast_type_t cast_type, char *service_name, int length);
void cast_restart_services(void);
int cast_init(void);
int cast_deinit(void);
bool cast_air_is_demo(void);
bool cast_dial_is_demo(void);
bool cast_um_is_demo(void);
cast_dial_conn_e cast_dial_connect_state(void);
int cast_api_mira_set_default_res(void);
int cast_set_drv_hccast_type(cast_type_t type);
void cast_air_reset_video(void);

#ifdef DLNA_SUPPORT
int hccast_dlna_callback_func(hccast_dlna_event_e event, void* in, void* out);
#ifdef DIAL_SUPPORT
int hccast_dial_callback_func(hccast_dial_event_e event, void* in, void* out);
#else
int hccast_dial_service_uninit(void);
int hccast_dial_service_start(void);
int hccast_dial_service_stop(void);
#endif
#else
int hccast_dlna_service_uninit(void);
int hccast_dlna_service_start(void);
int hccast_dlna_service_stop(void);
#endif

#ifdef MIRACAST_SUPPORT
int hccast_mira_callback_func(hccast_mira_event_e event, void* in, void* out);
#else
int hccast_mira_service_start(void);
int hccast_mira_service_stop(void);
int hccast_mira_player_init(void);
int hccast_mira_get_stat(void);
int hccast_mira_service_uninit(void);
#endif

#ifdef AIRCAST_SUPPORT
int hccast_air_callback_event(hccast_air_event_e event, void* in, void* out);
#else
int hccast_air_audio_state_get(void);
int hccast_air_service_start(void);
int hccast_air_service_stop(void);
int hccast_air_mdnssd_start(void);
int hccast_air_mdnssd_stop(void);
int hccast_air_service_is_start(void);
#endif


#ifdef USBMIRROR_SUPPORT
int cast_usb_mirror_init(void);
int cast_usb_mirror_deinit(void);
int cast_usb_mirror_start(void);
int cast_usb_mirror_stop(void);
int cast_um_dev_is_connected(void);
#else
/*
int hccast_um_init(void);
int hccast_um_deinit(void);
int hccast_um_param_set(hccast_um_param_t *param);
int hccast_ium_start(char *uuid, hccast_um_cb event_cb);
int hccast_ium_stop(void);
int hccast_aum_start(hccast_aum_param_t *param, hccast_um_cb event_cb);
int hccast_aum_stop(void);
*/
#endif

void restart_air_service_by_hdmi_change(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif



