/**
* @file
* @brief                hudi sound driver interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_SND_H__
#define __HUDI_SND_H__

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

typedef int (*hudi_snd_cb)(hudi_handle handle, unsigned int event, void *arg, void *user_data);

/**
* @brief       Open an hudi sound module instance
* @param[out]  handle  Output the handle of the instance
* @param[in]   audsink Audio sink
* @param[in]   func    Event notifier callback function
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_open(hudi_handle *handle, unsigned int audsink);

/**
* @brief       Close an hudi sound module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_close(hudi_handle handle);

/**
* @brief       Start sound
* @param[in]   handle  Handle of the instance
* @param[in]   config  Sound configuration
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_start(hudi_handle handle, struct snd_pcm_params *config);

/**
* @brief       Close an hudi sound module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_stop(hudi_handle handle);

/**
* @brief       Pause sound
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_pause(hudi_handle handle);

/**
* @brief       Resume sound
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_resume(hudi_handle handle);

/**
* @brief       Feed frame packet into sound
* @param[in]   handle  Handle of the instance
* @param[in]   pkt     Audio packet parameters
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_feed(hudi_handle handle, struct snd_xfer *pkt);

/**
* @brief       Flush the sound buffer
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_flush(hudi_handle handle);

/**
* @brief       Start the sound record
* @param[in]   handle           Handle of the instance
* @param[in]   rec_buf_size     Record buf size
* @retval      0                Success
* @retval      other            Error
*/
int hudi_snd_record_start(hudi_handle handle, int rec_buf_size);

/**
* @brief       Stop the sound record
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_record_stop(hudi_handle handle);

/**
* @brief       Sound record data read
* @param[in]   handle       Handle of the instance
* @param[out]  buf          Out data pointer
* @param[in]   size         Read size
* @retval      0            Success
* @retval      other        Error
*/
int hudi_snd_record_read(hudi_handle handle, char *buf, int size);

/**
* @brief       Get the volume
* @param[in]   handle  Handle of the instance
* @param[out]  volume  Out data pointer
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_volume_get(hudi_handle handle, unsigned char *volume);

/**
* @brief       Set the volume
* @param[in]   handle  Handle of the instance
* @param[in]   volume  Volume value
* @retval      0       Success
* @retval      other   Error
*/
int hudi_snd_volume_set(hudi_handle handle, unsigned char volume);

/**
* @brief       Set the mute
* @param[in]   handle   Handle of the instance
* @param[in]   mute     mute value
* @retval      0        Success
* @retval      other    Error
*/
int hudi_snd_mute_set(hudi_handle handle, unsigned int mute);

/**
* @brief       Get the snd dynamic range control
* @param[in]   handle       Handle of the instance
* @param[out]  peak_dBFS    Out data pointer
* @param[out]  gain_dBFS    Out data pointer
* @retval      0            Success
* @retval      other        Error
*/
int hudi_snd_drc_param_get(hudi_handle handle, float *peak_dBFS, float *gain_dBFS);

/**
* @brief        Set the snd dynamic range control
* @param[in]    handle      Handle of the instance
* @param[in]    peak_dBFS   Peak value
* @param[in]    gain_dBFS   Gain value
* @retval       0           Success
* @retval       other       Error
*/
int hudi_snd_drc_param_set(hudi_handle handle, float peak_dBFS, float gain_dBFS);

/**
* @brief        Set the snd dual tone
* @param[in]    handle          Handle of the instance
* @param[in]    on              1--enable, 0--disable
* @param[in]    bass_index      Set the bass index
* @param[in]    treble_index    Set the treble index
* @param[in]    mode            Set the value of snd_twotone_mode_e
* @retval       0               Success
* @retval       other           Error
*/
int hudi_snd_dual_tone_set(hudi_handle handle, int on, int bass_index, int treble_index, snd_twotone_mode_e mode);

/**
* @brief        Set the snd balance
* @param[in]    handle          Handle of the instance
* @param[in]    on              1--enable, 0--disable
* @param[in]    balance_index   Set the balance index
* @retval       0               Success
* @retval       other           Error
*/
int hudi_snd_balance_set(hudi_handle handle, int on, int balance_index);

/**
* @brief        Set the snd equalizer
* @param[in]    handle  Handle of the instance
* @param[in]    on      1--enable, 0--disable
* @retval       0       Success
* @retval       other   Error
*/
int hudi_snd_equalizer_set(hudi_handle handle, int on);

/**
* @brief        Set the snd equalizer band
* @param[in]    handle  Handle of the instance
* @param[in]    band    Set the band
* @param[in]    cutoff  Set the cutoff
* @param[in]    q       Set the quantize
* @param[in]    gain    Set the gain
* @retval       0       Success
* @retval       other   Error
*/
int hudi_snd_equalizer_band_set(hudi_handle handle, int band, int cutoff, int q, int gain);

/**
* @brief       Get the snd hw info
* @param[in]   handle   Handle of the instance
* @param[out]  info     Out data pointer
* @retval      0        Success
* @retval      other    Error
*/
int hudi_snd_hw_info_get(hudi_handle handle, struct snd_hw_info *info);

/**
* @brief       Enable/Disable the auto mute
* @param[in]   handle   Handle of the instance
* @param[in]   mute     1--enable, 0--disable 
* @retval      0        Success
* @retval      other    Error
*/
int hudi_snd_auto_mute_set(hudi_handle handle, unsigned int mute);

#ifdef __cplusplus
}
#endif

#endif
