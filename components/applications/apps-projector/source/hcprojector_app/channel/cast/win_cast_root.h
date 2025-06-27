/*
win_cast_root.h
 */

#ifndef __WIN_CAST_ROOT_H__
#define __WIN_CAST_ROOT_H__

#ifdef __cplusplus
extern "C" {
#endif

//Exit from dlna/miracast/aircast to cast ui by remote key.
//This cast, do not need to wait cast UI open before stop dlna/miracast/aircast.
void win_exit_to_cast_root_by_key_set(bool exit_by_key);
bool win_exit_to_cast_root_by_key_get(void);
uint32_t win_cast_play_param();
void win_cast_mirror_rotate_switch(void);
void cast_stop_service(void);
void win_cast_set_m_stop_service_exit(bool val);
bool win_cast_get_play_request_flag(void);
void win_cast_set_play_request_flag(bool val);
bool win_cast_root_wait_open(uint32_t timeout);
void win_cast_set_play_param(uint32_t play_param);

#ifdef __cplusplus
} /*extern "C"*/
#endif


#endif
