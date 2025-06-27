#ifndef __COM_API_H__
#define __COM_API_H__

#include "app_config.h"

#include <stdio.h> //printf()
#include <stdlib.h>
#include <string.h> //memcpy()
#include <unistd.h> //usleep()
#include <stdbool.h> //bool
#include <stdint.h> //uint32_t
#include <hcuapi/dis.h>
#include "lvgl/lvgl.h"
#include <hcuapi/input-event-codes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CTL_MSG_COUNT   100

#define API_SUCCESS     (0)
#define API_FAILURE     (-1)

#define INVALID_VALUE_8     (0xFF)
#define INVALID_VALUE_16    (0xFFFF)
#define INVALID_VALUE_32    (0xFFFFFFFF)

#define MAX_FILE_NAME 1024

#define MOUNT_ROOT_DIR    "/media"

#define HC_ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

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
    //key
    MSG_TYPE_KEY = 0,

    MSG_TYPE_USB_MOUNT,
    MSG_TYPE_USB_UNMOUNT,
    MSG_TYPE_USB_MOUNT_FAIL,
    MSG_TYPE_USB_UNMOUNT_FAIL,
    MSG_TYPE_SD_MOUNT,
    MSG_TYPE_SD_UNMOUNT,
    MSG_TYPE_SD_MOUNT_FAIL,
    MSG_TYPE_SD_UNMOUNT_FAIL,

    MSG_TYPE_MP_TRANS_PLAY,
} msg_type_t;


#define SYS_HALT()      \
{                       \
    while(1);           \
}

#define ASSERT_API(expression)              \
    {                                   \
        if (!(expression))              \
        {                               \
            printf("assertion(%s) failed: file \"%s\", line %d\n",   \
                #expression, __FILE__, __LINE__);   \
            SYS_HALT();                    \
        }                               \
    }

typedef struct
{
    msg_type_t  msg_type;
    uint32_t    msg_code;
} control_msg_t;


typedef void (*screen_ctrl)(void *arg1, void *arg2);
typedef struct
{
    void *screen;
    screen_ctrl control; //the control function is to process the message.
} screen_entry_t;


//use for app & ui
typedef struct partition_info
{
    int count; //number of all partitions
    void *dev_list;
    char *used_dev;
    int m_storage_state;
} partition_info_t;

void api_system_pre_init(void);
int api_system_init(void);
uint32_t api_sys_tick_get(void);
void api_sys_clock_time_check_start(void);
void api_sleep_ms(uint32_t ms);

int api_control_send_msg(control_msg_t *control_msg);
int api_control_receive_msg(control_msg_t *control_msg);
int api_control_send_key(uint32_t key);

int api_dis_show_onoff(bool on_off);
void app_ffplay_init(void);
void *api_ffmpeg_player_get(void);
void api_ffmpeg_player_get_regist(void *(func)(void));
int api_media_pic_backup_free(void);
bool api_media_pic_is_backup(void);

screen_ctrl api_screen_get_ctrl(void *screen);
int api_romfs_resources_mount(void);

void *mmp_get_partition_info();
int api_set_partition_info(int index);
int partition_info_update(int usb_state, void *dev);
bool api_check_partition_used_dev_ishotplug(void);
bool api_storage_devinfo_state_get(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __COM_API_H__
