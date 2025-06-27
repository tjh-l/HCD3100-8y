/**
* @file
* @brief                hudi vidsink interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_VIDSINK_H__
#define __HUDI_VIDSINK_H__

#include <stdint.h>
#include <hcuapi/common.h>
#include <hcuapi/vidsink.h>

#include "hudi_com.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief       Open a hudi vidsink module instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_vidsink_open(hudi_handle *handle);

/**
* @brief       Close a hudi vidsink module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_vidsink_close(hudi_handle handle);

/**
* @brief       Enable/Disable imgage effect
* @param[in]   handle  Handle of the vidsink instance
* @param[in]   enable  1--enable, 0--disable
* @retval      0       Success
* @retval      other   Error
*/
int hudi_vidsink_img_effect_set(hudi_handle handle, unsigned int enable);

#ifdef __cplusplus
}
#endif

#endif
