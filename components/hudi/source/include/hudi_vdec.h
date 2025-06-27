/**
* @file
* @brief                hudi video decoder interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_VDEC_H__
#define __HUDI_VDEC_H__

#include <stdint.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/avsync.h>
#include <hcuapi/dis.h>
#include <hcuapi/avevent.h>

#include "hudi_com.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*hudi_vdec_cb)(hudi_handle handle, unsigned int event, void *arg, void *user_data);

/**
* @brief       Open a hudi video decoder module instance
* @param[out]  handle  Output the handle of the instance
* @param[in]   config  Decode parameters
* @retval      0       Success
* @retval      other   Error
*/
int hudi_vdec_open(hudi_handle *handle, struct video_config *config);

/**
* @brief       Close a hudi video decoder module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_vdec_close(hudi_handle handle, int frame_backup);

/**
* @brief       Start video decoding
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_vdec_start(hudi_handle handle);

/**
* @brief       Pause video decoding
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_vdec_pause(hudi_handle handle);

/**
* @brief       Feed frame packet into video decoder
* @param[in]   handle   Handle of the instance
* @param[in]   data     Video data
* @param[in]   pkt      Video packet parameters
* @retval      0        Success
* @retval      other    Error
*/
int hudi_vdec_feed(hudi_handle handle, char *data, AvPktHd *pkt);

/**
* @brief       Register and start events polling
* @param[in]   handle       Handle of the instance
* @param[in]   func         Callback function
* @param[in]   event_type   Event type
* @param[in]   user_data    User data pointer
* @retval      0            Success
* @retval      other        Error
*/
int hudi_vdec_event_register(hudi_handle handle, hudi_vdec_cb func, int event_type, void *user_data);

/**
* @brief       Unregister events
* @param[in]   handle       Handle of the instance
* @param[in]   event_type   Event type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_vdec_event_unregister(hudi_handle handle, int event_type);

/**
* @brief       Get the decoder status
* @param[in]   handle  Handle of the instance
* @param[out]  stat    Status information of decoder
* @retval      0       Success
* @retval      other   Error
*/
int hudi_vdec_stat_get(hudi_handle handle, struct vdec_decore_status *stat);

/**
* @brief       Get the video play time
* @param[in]   handle  Handle of the instance
* @param[out]  pts     PTS of video playing
* @retval      0       Success
* @retval      other   Error
*/
int hudi_vdec_pts_get(hudi_handle handle, long long *pts);

/**
* @brief       Set the frame rate
* @param[in]   handle  Handle of the instance
* @param[in]   fps     Frame rate to be set, e.g. 60fps - fps = 60 * 1000
* @retval      0       Success
* @retval      other   Error
*/
int hudi_vdec_fps_set(hudi_handle handle, unsigned int fps);

/**
* @brief       Get the buffering level
* @param[in]   handle  Handle of the instance
* @param[out]  percent Percentage of buffering
* @retval      0       Success
* @retval      other   Error
*/
int hudi_vdec_waterline_get(hudi_handle handle, unsigned int *percent);

/**
* @brief       Set the masaic mode
* @param[in]   handle   Handle of the instance
* @param[out]  mode     masaic mode
* @retval      0        Success
* @retval      other    Error
*/
int hudi_vdec_masaic_mode_set(hudi_handle handle, unsigned int mode);

#ifdef __cplusplus
}
#endif

#endif
