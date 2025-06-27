/**
* @file
* @brief                hudi audio decoder interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_ADEC_H__
#define __HUDI_ADEC_H__

#include <stdint.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/audsink.h>
#include <hcuapi/auddec.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/avsync.h>

#include "hudi_com.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*hudi_adec_cb)(hudi_handle handle, unsigned int event, void *arg, void *user_data);

/**
* @brief       Open an hudi audio decoder module instance
* @param[out]  handle  Output the handle of the instance
* @param[in]   config  Decode parameters
* @param[in]   func    Event notifier callback function
* @retval      0       Success
* @retval      other   Error
*/
int hudi_adec_open(hudi_handle *handle, struct audio_config *config);

/**
* @brief       Close an hudi audio decoder module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_adec_close(hudi_handle handle);

/**
* @brief       Start audio decoding
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_adec_start(hudi_handle handle);

/**
* @brief       Pause audio decoding
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_adec_pause(hudi_handle handle);

/**
* @brief       Feed frame packet into audio decoder
* @param[in]   handle   Handle of the instance
* @param[in]   data     Audio data
* @param[in]   pkt      Audio packet parameters
* @retval      0        Success
* @retval      other    Error
*/
int hudi_adec_feed(hudi_handle handle, char *data, AvPktHd *pkt);

#ifdef __cplusplus
}
#endif

#endif
