/**
* @file
* @brief                hudi sound in driver interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_SND_I2SI_H__
#define __HUDI_SND_I2SI_H__

#include <stdint.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/audsink.h>
#include <hcuapi/snd.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/avsync.h>
#include <hcuapi/kshm.h>

#include "hudi_com.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief       Open an hudi sound i2si module instance
* @param[out]  handle  Output the handle of the instance
* @param[in]   func    Event notifier callback function
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_i2si_open(hudi_handle *handle);

/**
* @brief       Close an hudi sound i2si module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_i2si_close(hudi_handle handle);

/**
* @brief       Start sound i2si
* @param[in]   handle  Handle of the instance
* @param[in]   config  Sound configuration
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_i2si_start(hudi_handle handle, struct snd_pcm_params *config);

/**
* @brief       Stop sound i2si
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_i2si_stop(hudi_handle handle);

/**
* @brief       Get the sound i2si volume
* @param[in]   handle  Handle of the instance
* @param[out]  volume  Out data pointer
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_i2si_volume_get(hudi_handle handle, unsigned char *volume);

/**
* @brief       Set the sound i2si volume
* @param[in]   handle  Handle of the instance
* @param[in]   volume  Volume value
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_i2si_volume_set(hudi_handle handle, unsigned char volume);

/**
* @brief       Set the sound i2si mute
* @param[in]   handle   Handle of the instance
* @param[in]   mute     mute value
* @retval      0        Success
* @retval      other    Error
*/
int hudi_snd_i2si_mute_set(hudi_handle handle, unsigned int mute);

#ifdef __cplusplus
}
#endif

#endif
