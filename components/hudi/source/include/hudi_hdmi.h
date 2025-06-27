/**
* @file
* @brief                hudi hdmi interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_HDMI_H__
#define __HUDI_HDMI_H__

#include <hcuapi/common.h>
#include <hcuapi/hdmi_tx.h>

#include "hudi_com.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*hudi_hdmi_cb)(hudi_handle handle, unsigned int event, void *arg, void *user_data);

/**
* @brief       Open a hudi hdmi module instance
* @param[out]  handle  Output the handle of the instance
* @param[in]   config  Decode parameters
* @retval      0       Success
* @retval      other   Error
*/
int hudi_hdmi_open(hudi_handle *handle);

/**
* @brief       Close a hudi hdmi module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_hdmi_close(hudi_handle handle);

/**
* @brief       Register and start events polling
* @param[in]   handle       Handle of the instance
* @param[in]   func         Callback function
* @param[in]   event_type   Event type
* @param[in]   user_data    User data pointer
* @retval      0       Success
* @retval      other   Error
*/
int hudi_hdmi_event_register(hudi_handle handle, hudi_hdmi_cb func, int event_type, void *user_data);

/**
* @brief       Unregister events
* @param[in]   handle       Handle of the instance
* @param[in]   event_type   Event type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmi_event_unregister(hudi_handle handle, int event_type);

/**
* @brief       Get hdmi edid tvsys
* @param[in]   handle           Handle of the instance
* @param[in]   tvsys_info       Out data pointer
* @retval      0                Success
* @retval      other            Error
*/
int hudi_hdmi_edid_tvsys_get(hudi_handle handle, tvsys_e *tvsys_info);

/**
* @brief       Get hdmi edid info
* @param[in]   handle           Handle of the instance
* @param[in]   edid_info        Out data pointer
* @retval      0                Success
* @retval      other            Error
*/
int hudi_hdmi_edid_info_get(hudi_handle handle, hdmi_edidinfo_t *edid_info);

#ifdef __cplusplus
}
#endif

#endif

