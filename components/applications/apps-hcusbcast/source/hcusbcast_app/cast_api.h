/**
 * @file cast_api.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __CAST_API_H__
#define __CAST_API_H__

#ifdef USBMIRROR_SUPPORT
#include <hccast/hccast_um.h>
#endif

#include <hccast/hccast_scene.h>


#ifdef __cplusplus
extern "C" {
#endif

int cast_init(void);
int cast_deinit(void);
bool cast_is_demo(void);

int cast_usb_mirror_init(void);
int cast_usb_mirror_deinit(void);
int cast_usb_mirror_start(void);
int cast_usb_mirror_stop(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif



