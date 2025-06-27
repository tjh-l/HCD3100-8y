/**
* @file
* @brief                hudi display engine interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_DISPLAY_H__
#define __HUDI_DISPLAY_H__

#include <stdint.h>
#include <hcuapi/common.h>
#include <hcuapi/dis.h>

#include "hudi_com.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*hudi_display_cb)(hudi_handle handle, unsigned int event, void *arg, void *user_data);

/**
* @brief       Open a hudi display engine module instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_display_open(hudi_handle *handle);

/**
* @brief       Close a hudi display engine module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_display_close(hudi_handle handle);

/**
* @brief       Set display aspect mode
* @param[in]   handle       Handle of the instance
* @param[in]   aspect_args  Aspect arguments
* @retval      0            Success
* @retval      other        Error
*/
int hudi_display_aspect_set(hudi_handle handle, dis_aspect_mode_t *aspect_args);

/**
* @brief       Set display zoom
* @param[in]   handle       Handle of the instance
* @param[in]   zoom_args    Zoom arguments
* @retval      0            Success
* @retval      other        Error
*/
int hudi_display_zoom_set(hudi_handle handle, dis_zoom_t *zoom_args);

/**
* @brief       Backup main picture of display
* @param[in]   handle       Handle of the instance
* @param[in]   dis_type     Display type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_display_pic_backup(hudi_handle handle, dis_type_e dis_type);

/**
* @brief       Free backup main picture
* @param[in]   handle       Handle of the instance
* @param[in]   dis_type     Display type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_display_pic_free(hudi_handle handle, dis_type_e dis_type);

/**
* @brief       Register and start events polling
* @param[in]   handle       Handle of the instance
* @param[in]   func         Callback function
* @param[in]   event_type   Event type
* @param[in]   user_data    User data pointer
* @retval      0            Success
* @retval      other        Error
*/
int hudi_display_event_register(hudi_handle handle, hudi_display_cb func, int event_type, void *user_data);

/**
* @brief       Unregister events
* @param[in]   handle       Handle of the instance
* @param[in]   event_type   Event type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_display_event_unregister(hudi_handle handle, int event_type);

/**
* @brief       Get display screen info
* @param[in]   handle       Handle of the instance
* @param[in]   distype      Display type
* @param[out]  screen_info  Out data pointer
* @retval      0            Success
* @retval      other        Error
*/
int hudi_display_screen_info_get(hudi_handle handle, dis_type_e distype, dis_area_t *screen_info);

/**
* @brief       Get display area info
* @param[in]   handle       Handle of the instance
* @param[in]   distype      Display type
* @param[out]  area_info    Out data pointer
* @retval      0            Success
* @retval      other        Error
*/
int hudi_display_area_info_get(hudi_handle handle, dis_type_e distype, dis_area_t *area_info);

/**
* @brief       Get display picture area
* @param[in]   handle           Handle of the instance
* @param[in]   distype          Display type
* @param[out]  picture_area     Out data pointer
* @retval      0                Success
* @retval      other            Error
*/
int hudi_display_picture_area_get(hudi_handle handle, dis_type_e distype, dis_area_t *picture_area);

/**
* @brief       Get display info
* @param[in]   handle       Handle of the instance
* @param[in]   distype      Display type
* @param[in]   layer        Layer type
* @param[out]  dis_info     Out data pointer
* @retval      0            Success
* @retval      other        Error
*/
int hudi_display_info_get(hudi_handle handle, dis_type_e distype, dis_layer_e layer, dis_display_info_t *dis_info);

/**
* @brief       Set display dynamic enhance onoff
* @param[in]   handle   Handle of the instance
* @param[in]   distype  Set the value of dis_type_e
* @param[in]   on       1--enable, 0--disable
* @retval      0        Success
* @retval      other    Error
*/
int hudi_display_dynamic_enhance_set(hudi_handle handle, dis_type_e distype, unsigned int on);

/**
* @brief       Set display layer blend order
* @param[in]   handle       Handle of the instance
* @param[in]   layer_args   layer order arguments
* @retval      0            Success
* @retval      other        Error
*/
int hudi_display_layer_order_set(hudi_handle handle, dis_layer_blend_order_t *layer_args);

/**
* @brief       Set display video enhance
* @param[in]   handle           Handle of the instance
* @param[in]   enhance_args     Enhance arguments
* @retval      0                Success
* @retval      other            Error
*/
int hudi_display_enhance_set(hudi_handle handle, dis_video_enhance_t *enhance_args);

/**
* @brief       Set display video vscreen detect
* @param[in]   distype  Set the value of dis_type_e
* @param[in]   layer    Set the value of dis_layer_e
* @param[in]   on       1--enable, 0--disable
* @retval      0        Success
* @retval      other    Error
*/
int hudi_display_vscreen_detect_set(hudi_handle handle, dis_type_e distype, dis_layer_e layer, unsigned char on);

/**
* @brief       Set display suspend
* @param[in]   handle       Handle of the instance
* @param[in]   distype      hudi_display_type_e distype
* @retval      0            Success
* @retval      other        Error
*/
int hudi_display_suspend(hudi_handle handle, dis_type_e distype);

/**
* @brief       Set display onoff
* @param[in]   handle           Handle of the instance
* @param[in]   distype          Set the value of dis_type_e
* @param[in]   layer            Set the value of dis_layer_e
* @param[in]   on               1--enable, 0--disable
* @retval      0                Success
* @retval      other            Error
*/
int hudi_display_onoff_set(hudi_handle handle, dis_type_e distype, dis_layer_e layer, unsigned int on);

/**
* @brief       Get display keystone info
* @param[in]   handle           Handle of the instance
* @param[in]   distype          Display type
* @param[out]  keystone_info    Out data pointer
* @retval      0                Success
* @retval      other            Error
*/
int hudi_display_keystone_get(hudi_handle handle, dis_type_e distype, dis_keystone_param_t *keystone_info);

/**
* @brief       Set display keystone info
* @param[in]   handle           Handle of the instance
* @param[in]   distype          Display type
* @param[in]   keystone_args    Keystone arguments
* @retval      0                Success
* @retval      other            Error
*/
int hudi_display_keystone_set(hudi_handle handle, dis_type_e distype, dis_keystone_param_t *keystone_args);

/**
* @brief       Get display tvsys
* @param[in]   handle           Handle of the instance
* @param[in]   tvsys_info       Out data pointer
* @retval      0                Success
* @retval      other            Error
*/
int hudi_display_tvsys_get(hudi_handle handle, dis_tvsys_t *tvsys_info);

/**
* @brief       Set display tvsys
* @param[in]   handle           Handle of the instance
* @param[in]   tvsys_args       Tvsys arguments
* @retval      0                Success
* @retval      other            Error
*/
int hudi_display_tvsys_set(hudi_handle handle, dis_tvsys_t *tvsys_args);

/**
* @brief       Register display dac module
* @param[in]   handle           Handle of the instance
* @param[in]   dac_args         Dac arguments
* @retval      0                Success
* @retval      other            Error
*/
int hudi_display_dac_register(hudi_handle handle, dis_dac_param_t *dac_args);

/**
* @brief       Unregister display dac module
* @param[in]   handle           Handle of the instance
* @param[in]   dac_args         Dac arguments
* @retval      0                Success
* @retval      other            Error
*/
int hudi_display_dac_unregister(hudi_handle handle, dis_dac_param_t *dac_args);

#ifdef __cplusplus
}
#endif

#endif
