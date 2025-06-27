/**
* @file
* @brief                hudi audio & video synchronization interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_AVSYNC_H__
#define __HUDI_AVSYNC_H__

#include <stdint.h>
#include <hcuapi/common.h>
#include <hcuapi/avsync.h>

#include "hudi_com.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*hudi_avsync_cb)(hudi_handle handle, unsigned int event, void *arg, void *user_data);

/**
* @brief       Open a hudi avsync module instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_avsync_open(hudi_handle *handle);

/**
* @brief       Close a hudi avsync module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_avsync_close(hudi_handle handle);

/**
* @brief       Set the stc
* @param[in]   handle  Handle of the avsync instance
* @param[in]   stc_ms  STC value in millisecond
* @retval      0       Success
* @retval      other   Error
*/
int hudi_avsync_stc_set(hudi_handle handle, unsigned int stc_ms);

/**
* @brief       Get the audio sync threshold value
* @param[in]   handle  Handle of the avsync instance
* @param[in]   rate    Rate to be set
* @retval      0       Success
* @retval      other   Error
*/
int hudi_avsync_stc_rate_set(hudi_handle handle, float rate);

/**
* @brief       Get the audio sync threshold value
* @param[in]   handle  Handle of the avsync instance
* @param[out]  value   Pointer to the output value
* @retval      0       Success
* @retval      other   Error
*/
int hudi_avsync_audsync_thres_get(hudi_handle handle, int *value);

/**
* @brief       Set the audio sync threshold value
* @param[in]   handle  Handle of the avsync instance
* @param[in]   value   Value to be set
* @retval      0       Success
* @retval      other   Error
*/
int hudi_avsync_audsync_thres_set(hudi_handle handle, int value);

/**
* @brief       Get the video sync delay value
* @param[in]   handle       Handle of the avsync instance
* @param[out]  delay_ms     Pointer to the output value in millisecond
* @retval      0            Success
* @retval      other        Error
*/
int hudi_avsync_vidsync_delay_get(hudi_handle handle, int *delay_ms);

/**
* @brief       Set the video sync delay value
* @param[in]   handle       Handle of the avsync instance
* @param[in]   delay_ms     Delay value in millisecond
* @retval      0            Success
* @retval      other        Error
*/
int hudi_avsync_vidsync_delay_set(hudi_handle handle, int delay_ms);

#ifdef __cplusplus
}
#endif

#endif
