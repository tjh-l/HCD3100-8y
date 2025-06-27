#ifndef __MP_PLAYERPAGE_H__
#define __MP_PLAYERPAGE_H__

#include <stdint.h> //uint32_t
#include "lvgl/lvgl.h"
#include "osd_com.h"
#include <ffplayer.h>
#include "file_mgr.h"
#include <stdio.h>
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif

void ctrlbarpage_open(void);
int ctrlbarpage_close();
void media_player_close(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __MP_PLAYERPAGE_H__
