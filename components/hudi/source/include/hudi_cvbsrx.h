/**
* @file
* @brief                hudi cvbs rx interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_CVBS_RX_H__
#define __HUDI_CVBS_RX_H__
#include <stdint.h>

#include "hudi_com.h"
#include <hcuapi/tvtype.h>
#include <hcuapi/tvdec.h>
#include <hcuapi/viddec.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef int (* hudi_cvbsrx_event_cb)(hudi_handle handle, unsigned int event, void *arg, void *user_data);

/**
* @brief       Open a cvbs rx instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_cvbsrx_open(hudi_handle *handle);

/**
* @brief       Close a cvbs rx instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_cvbsrx_close(hudi_handle handle);

/**
* @brief       Register and start events polling
* @param[in]   handle       Handle of the instance
* @param[in]   func         Callback function
* @param[in]   event_type   Event type
* @param[in]   user_data    User data pointer
* @retval      0       Success
* @retval      other   Error
*/
int hudi_cvbsrx_event_register(hudi_handle handle, hudi_cvbsrx_event_cb func, int event_type, void *user_data);

/**
* @brief       Unregister and start events polling
* @param[in]   handle       Handle of the instance
* @param[in]   event_type   Event type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_event_unregister(hudi_handle handle, int event_type);

/**
* @brief       Start cvbs rx output
* @param[in]   handle       Handle of the instance
* @param[in]   type         Event type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_start(hudi_handle handle, tvtype_e type);

/**
* @brief       Stop cvbs rx output
* @param[in]   handle       Handle of the instance
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_stop(hudi_handle handle);

/**
* @brief       Setup cvbs rx stop mode
* @param[in]   handle       Handle of the instance
* @param[in]   value        Stop mode vlaue, keep last frame: 1 Black screen: 0
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_stop_mode_set(hudi_handle handle, int value);

/**
* @brief       Setup cvbs rx data output path
* @param[in]   handle       Handle of the instance
* @param[in]   path         Cvbs output channel path
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_data_path_set(hudi_handle handle, tvdec_video_data_path_e path);

/**
* @brief       Setup cvbs rx screen rotate mode
* @param[in]   handle       Handle of the instance
* @param[in]   type         Rotate type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_rotate_mode_set(hudi_handle handle, rotate_type_e type);

/**
* @brief       Setup cvbs rx screen mirror mode
* @param[in]   handle       Handle of the instance
* @param[in]   type         Mirror type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_mirror_mode_set(hudi_handle handle, mirror_type_e type);

/**
* @brief       Get cvbs rx screen info mode
* @param[in]   handle       Handle of the instance
* @param[out]  info         cvbs rx info
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_info_get(hudi_handle handle, struct tvdec_video_info *info);

/**
* @brief       Start cvbs rx training
* @param[in]   handle       Handle of the instance
* @param[in]   tvtype       tvtype
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_training_start(hudi_handle handle, tvtype_e tvtype);

/**
* @brief       Setup cvbs rx training param
* @param[in]   handle       Handle of the instance
* @param[in]   params       Setup training params
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_training_param_set(hudi_handle handle, struct tvdec_training_params *params);

/**
* @brief       Setup cvbs rx training param
* @param[in]   handle       Handle of the instance
* @param[out]  res          Cvbs training result
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_training_result_get(hudi_handle handle, struct tvdec_training_result *res);

/**
* @brief       Setup cvbs rx dc register offset value
* @param[in]   handle       Handle of the instance
* @param[in]   value        Dc register value
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_training_dc_offset_set(hudi_handle handle, int value);

/**
* @brief       Setup cvbs rx brightness
* @param[in]   handle       Handle of the instance
* @param[in]   value        Brightness value
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_brightness_set(hudi_handle handle, int value);

/**
* @brief       Setup cvbs rx display info
* @param[in]   handle       Handle of the instance
* @param[in]   info         display info
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_display_info_set(hudi_handle handle, struct tvdec_display_info *info);

/**
* @brief       Setup cvbs rx display rect
* @param[in]   handle       Handle of the instance
* @param[in]   rect         display rect
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_display_rect_set(hudi_handle handle, struct vdec_dis_rect *rect);

/**
* @brief       Setup cvbs rx picture by picture mode
* @param[in]   handle       Handle of the instance
* @param[in]   mode         pbp mode
* @retval      0            Success
* @retval      other        Error
*/
int hudi_cvbsrx_pbp_mode_set(hudi_handle handle, video_pbp_mode_e mode);


#ifdef __cplusplus
}
#endif

#endif
