/**
* @file
* @brief                hudi hdmi rx interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_HDMIRX_H__
#define __HUDI_HDMIRX_H__
#include <stdint.h>

#include "hudi_com.h"
#include <hcuapi/viddec.h>
#include <hcuapi/ms9601.h>
#include <hcuapi/hdmi_rx.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef int (* hudi_hdmirx_event_cb)(hudi_handle handle, unsigned int event, void *arg, void *user_data);

/**
* @brief       Open a hdmi rx instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_hdmirx_open(hudi_handle *handle);

/**
* @brief       Close a hdmi rx instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_hdmirx_close(hudi_handle handle);

/**
* @brief       Register and start events polling
* @param[in]   handle       Handle of the instance
* @param[in]   func         Callback function
* @param[in]   event_type   Event type
* @param[in]   user_data    User data pointer
* @retval      0       Success
* @retval      other   Error
*/
int hudi_hdmirx_event_register(hudi_handle handle, hudi_hdmirx_event_cb func, int event_type, void *user_data);

/**
* @brief       Unregister and start events polling
* @param[in]   handle       Handle of the instance
* @param[in]   event_type   Event type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_event_unregister(hudi_handle handle, int event_type);

/**
* @brief       Start hdmi rx output
* @param[in]   handle       Handle of the instance
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_start(hudi_handle handle);

/**
* @brief       Stop hdmi rx output
* @param[in]   handle       Handle of the instance
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_stop(hudi_handle handle);

/**
* @brief       Pause hdmi rx output
* @param[in]   handle       Handle of the instance
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_pause(hudi_handle handle);

/**
* @brief       Resume hdmi rx output
* @param[in]   handle       Handle of the instance
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_resume(hudi_handle handle);

/**
* @brief       Setup hdmi rx stop mode
* @param[in]   handle       Handle of the instance
* @param[in]   value        Stop mode vlaue, keep last frame: 1 Black screen: 0
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_stop_mode_set(hudi_handle handle, int value);

/**
* @brief       Setup hdmi rx video data output path
* @param[in]   handle       Handle of the instance
* @param[in]   path         Cvbs output channel path
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_video_data_path_set(hudi_handle handle, hdmi_rx_video_data_path_e path);

/**
* @brief       Setup hdmi rx audio data output path
* @param[in]   handle       Handle of the instance
* @param[in]   path         Cvbs output channel path
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_audio_data_path_set(hudi_handle handle, hdmi_rx_audio_data_path_e path);

/**
* @brief       Setup hdmi rx screen rotate mode
* @param[in]   handle       Handle of the instance
* @param[in]   type         Rotate type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_rotate_mode_set(hudi_handle handle, rotate_type_e type);

/**
* @brief       Setup hdmi rx screen mirror mode
* @param[in]   handle       Handle of the instance
* @param[in]   type         Mirror type
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_mirror_mode_set(hudi_handle handle, mirror_type_e type);

/**
* @brief       Get hdmi rx screen info mode
* @param[in]   handle       Handle of the instance
* @param[out]  info         hdmi rx info
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_info_get(hudi_handle handle, hdmi_rx_video_info_t *info);

/**
* @brief       Setup hdmi rx video blank mode
* @param[in]   handle       Handle of the instance
* @param[in]   value        blank mode value: HDMI_RX_VIDEO_BLANK_UNBLANK / HDMI_RX_VIDEO_BLANK_NORMAL
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_video_blank_set(hudi_handle handle, int value);

/**
* @brief       Setup hdmi rx hdcp key param
* @param[in]   handle       Handle of the instance
* @param[in]   key          Setup hdcp key data
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_hdcp_key_set(hudi_handle handle, hdmi_rx_hdcp_key_t *key);

/**
* @brief       Setup hdmi rx edid set
* @param[in]   handle       Handle of the instance
* @param[out]  res          Cvbs training result
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_edid_set(hudi_handle handle, hdmi_rx_edid_data_t *data);

/**
* @brief       Setup hdmi rx buf yuv2rgb onoff
* @param[in]   handle       Handle of the instance
* @param[in]   value        Value : on : 1 off : 0
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_buf_yuv2rgb_onoff(hudi_handle handle, int value);

/**
* @brief       Setup video enc quality
* @param[in]   handle       Handle of the instance
* @param[in]   type         jpeg_enc_quality
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_video_enc_quality_set(hudi_handle handle, jpeg_enc_quality_type_e type);

/**
* @brief       Setup video enc quant
* @param[in]   handle       Handle of the instance
* @param[in]   quant        jpeg enc quant
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_video_enc_quant_set(hudi_handle handle, jpeg_enc_quant_t *quant);

/**
* @brief       Get video enc framerate
* @param[in]   handle       Handle of the instance
* @param[in]   rate         Framerate
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_enc_framerate_get(hudi_handle handle, uint32_t *rate);

/**
* @brief       Setup hdmi rx display info
* @param[in]   handle       Handle of the instance
* @param[in]   info         display info
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_display_info_set(hudi_handle handle, hdmi_rx_display_info_t *info);

/**
* @brief       Setup hdmi rx display rect
* @param[in]   handle       Handle of the instance
* @param[in]   rect         display rect
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_display_rect_set(hudi_handle handle, struct vdec_dis_rect *rect);

/**
* @brief       Setup hdmi rx picture by picture mode
* @param[in]   handle       Handle of the instance
* @param[in]   mode         pbp mode
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_pbp_mode_set(hudi_handle handle, video_pbp_mode_e mode);

/**
* @brief       Setup hdmi rx rec samplearate
* @param[in]   handle       Handle of the instance
* @param[in]   value        samplerate
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_rec_samplerate_set(hudi_handle handle, int value);

/**
* @brief       Open hdmi rx switch ms9601a instance
* @param[in]   handle       Handle of the instance
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_ms9601a_open(hudi_handle *handle);

/**
* @brief       Close hdmi rx switch ms9601a instance
* @param[in]   handle       Handle of the instance
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_ms9601a_close(hudi_handle handle);

/**
* @brief       Setup hdmi rx switch ms9601a input channel port
* @param[in]   handle       Handle of the instance
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_ms9601a_channel_set(hudi_handle handle, ms9601_idx_e index);

/**
* @brief       Auto Setup hdmi rx switch ms9601a input port
* @param[in]   handle       Handle of the instance
* @retval      0            Success
* @retval      other        Error
*/
int hudi_hdmirx_ms9601a_channel_auto_set(hudi_handle handle);


#ifdef __cplusplus
}
#endif

#endif
