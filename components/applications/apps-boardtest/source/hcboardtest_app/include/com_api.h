/**
 * @file com_api.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-01-20
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef __COM_API_H__
#define __COM_API_H__

#include "app_config.h"

#include "lvgl/lvgl.h"
#include <hcuapi/dis.h>
#include <hcuapi/input-event-codes.h>
#include <stdbool.h> //bool
#include <stdint.h>  //uint32_t
#include <stdio.h>   //printf()
#include <stdlib.h>
#include <string.h> //memcpy()
#include <unistd.h> //usleep()

#ifdef __cplusplus
extern "C"
{
#endif

#define CTL_MSG_COUNT 100

#define INVALID_VALUE_32 (0xFFFFFFFF)

typedef enum
{
    USB_STAT_MOUNT,
    USB_STAT_UNMOUNT,
    USB_STAT_MOUNT_FAIL,
    USB_STAT_UNMOUNT_FAIL,
    USB_STAT_CONNECTED,
    USB_STAT_DISCONNECTED,
    USB_STAT_INVALID,
    SD_STAT_MOUNT,
    SD_STAT_UNMOUNT,
    SD_STAT_MOUNT_FAIL,
    SD_STAT_UNMOUNT_FAIL,
} USB_STATE;

typedef enum
{
    MSG_TYPE_KEY = 0,

    MSG_TYPE_PASSFAIL_MBOX_CREATE, /*create a mbox, have pass or fail butten*/
    MSG_TYPE_OK_MBOX_CREATE,       /*create a mbox, have ok butten*/
    MSG_TYPE_BOARDTEST_SORT_SEND,  /*Send the boardtest sort*/
    MSG_TYPE_MBOX_RESULT,          /*Returning the mbox result.*/
    MSG_TYPE_BOARDTEST_EXIT,       /*End of the boardtest*/
    MSG_TYPE_BOARDTEST_AUTO,       /*Start the automated testing process*/
    MSG_TYPE_BOARDTEST_STOP,       /*force stop*/
    MSG_TYPE_MBOX_CLOSE,           /*mbox close*/
    MSG_TYPE_OSD_CLOSE,            /*lvgl osd close*/
    MSG_TYPE_OSD_OPEN,
    MSG_TYPE_BOARDTEST_RUN_OVER,
    MSG_TYPE_BOARDTEST_AUTO_OVER,
    MSG_TYPE_DISPLAY_DETAIL,

    MSG_TYPE_USB_MOUNT,
    MSG_TYPE_USB_UNMOUNT,
    MSG_TYPE_USB_MOUNT_FAIL,
    MSG_TYPE_USB_UNMOUNT_FAIL,
    MSG_TYPE_SD_MOUNT,
    MSG_TYPE_SD_UNMOUNT,
    MSG_TYPE_SD_MOUNT_FAIL,
    MSG_TYPE_SD_UNMOUNT_FAIL,

} msg_type_t;

typedef struct
{
    msg_type_t msg_type;
    uint32_t msg_code;
} control_msg_t;

int api_control_send_msg(control_msg_t *control_msg);
int api_control_receive_msg(control_msg_t *control_msg);
int api_control_send_key(uint32_t key);

int boardtest_run_control_send_msg(control_msg_t *control_msg);
int boardtest_run_control_receive_msg(control_msg_t *control_msg);

int boardtest_exit_control_send_msg(control_msg_t *control_msg);
int boardtest_exit_control_receive_msg(control_msg_t *control_msg);

void boardtest_read_ini_init(void);
void boardtest_read_ini_exit(void);

int api_dis_show_onoff(bool on_off);

void api_sleep_ms(uint32_t ms);

char *api_get_ad_mount_add(void);

uint32_t api_sys_tick_get(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
