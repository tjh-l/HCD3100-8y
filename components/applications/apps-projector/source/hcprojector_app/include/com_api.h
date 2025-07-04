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

#include <stdio.h> //printf()
#include <stdlib.h>
#include <string.h> //memcpy()
#include <unistd.h> //usleep()
#include <stdbool.h> //bool
#include <stdint.h> //uint32_t
#include <hcuapi/dis.h>
#include <hcuapi/viddec.h>
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

#define DIS_SOURCE_FULL_W  1920
#define DIS_SOURCE_FULL_H  1080
#define DIS_SOURCE_FULL_X  0
#define DIS_SOURCE_FULL_Y  0

#ifdef LVGL_RESOLUTION_240P_SUPPORT
#define OSD_MAX_WIDTH   320
#define OSD_MAX_HEIGHT  240
#else
#define OSD_MAX_WIDTH   1280
#define OSD_MAX_HEIGHT  720
#endif

#define SUB_NAME_X  140
#define SUB_NAME_Y  80
#define SUB_NAME_W  200

#define SUB_MENU_IMG_W  40
#define SUB_MENU_ITEM_W  400
#define SUB_MENU_ITEM_H  60

#define SUB_MENU_MAX    10

#define MAX_FILE_NAME 1024    

#define MAX_UI_TIPS_STR_LEN   (256)

//#define GPIO_RESET_NUMBER   PINPAD_L08
#define GPIO_RESET_NUMBER   INVALID_VALUE_32
#define GPIO_RESET_ACTIVE_VAL   0 //Read the GPIO, the value is actived


#define KEY_FLIP KEY_RED//398
#define KEY_KEYSTONE KEY_GREEN//399

#define FUNC_KEY_SCREEN_ROTATE LV_KEY_ENTER 


#define MAX_UDISK_DEV_LEN   64
#define MOUNT_ROOT_DIR    "/media"

#ifndef HC_ARRAY_SIZE
  #define HC_ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#endif    

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
}USB_STATE;


typedef enum TV_SYS_TYPE
{
    TV_MODE_AUTO = 0,
    TV_MODE_PAL,
    TV_MODE_PAL_M,//  PAL3.58
    TV_MODE_PAL_N,
    TV_MODE_NTSC358,//NTSC3.58
    TV_MODE_NTSC443,
    TV_MODE_SECAM,
#if 1
    TV_MODE_576P,
    TV_MODE_480P,
    TV_MODE_720P_50,
    TV_MODE_720P_60,
    TV_MODE_1080I_25,
    TV_MODE_1080I_30,   
    TV_MODE_BY_EDID,
    TV_MODE_1080P_50,
    TV_MODE_1080P_60,   
    TV_MODE_1080P_25,
    TV_MODE_1080P_30,   
    TV_MODE_1080P_24,
#endif
    TV_MODE_800_480,
    TV_MODE_1024X768_60,
    TV_MODE_1360X768_60,
    TV_MODE_1280X960_60,
    TV_MODE_1280X1024_60,
    TV_MODE_1024X768_50,
    TV_MODE_1080P_55,   
    TV_MODE_768X1024_60,
    TV_MODE_COUNT,
}TV_SYS_TYPE;


typedef enum{

    UPG_STATUS_USB_OPEN_FILE_ERR = 0,
    UPG_STATUS_USB_FILE_TOO_LARGE,
    UPG_STATUS_USB_READ_OK,
    UPG_STATUS_USB_READ_ERR,

    UPG_STATUS_BURN_START,
    UPG_STATUS_BURN_OK,

    UPG_STATUS_PRODUCT_ID_MISMATCH,
    UPG_STATUS_VERSION_IS_OLD,
    UPG_STATUS_FILE_CRC_ERROR,
    UPG_STATUS_FILE_UNZIP_ERROR,
    UPG_STATUS_BURN_FAIL,

    UPG_STATUS_SERVER_FAIL,
    UPG_STATUS_USER_STOP_DOWNLOAD,

    UPG_STATUS_CONFIG_ERROR,
    UPG_STATUS_NETWORK_ERROR,

}upgrade_status_t;

typedef enum{
    MSGBOX_OK = 0,
    MSGBOX_CANCEL = 1,
}msgbox_code_t;

typedef enum{
    //key
    MSG_TYPE_KEY = 0,

    //message
    MSG_TYPE_MSG = 10,
    MSG_TYPE_OPEN_WIN,
    MSG_TYPE_CLOSE_WIN,
    MSG_TYPE_NETWORK_CONNECTING,

    MSG_TYPE_USB_MOUNT,
    MSG_TYPE_USB_UNMOUNT,
    MSG_TYPE_USB_MOUNT_FAIL,
    MSG_TYPE_USB_UNMOUNT_FAIL,
    MSG_TYPE_SD_MOUNT,
    MSG_TYPE_SD_UNMOUNT,
    MSG_TYPE_SD_MOUNT_FAIL,
    MSG_TYPE_SD_UNMOUNT_FAIL,

    MSG_TYPE_USB_DISK_PLUGIN,
    MSG_TYPE_USB_DISK_PLUGOUT,

    MSG_TYPE_USB_WIFI_PLUGIN,
    MSG_TYPE_USB_WIFI_PLUGOUT,

    MSG_TYPE_USB_UPGRADE,
    MSG_TYPE_NET_UPGRADE,
    MSG_TYPE_UPG_STATUS, //the status of upgreade, error code, etc
    MSG_TYPE_UPG_DOWNLOAD_PROGRESS,
    MSG_TYPE_UPG_BURN_PROGRESS,//30

    MSG_TYPE_MEDIA_BUFFERING,
    MSG_TYPE_MEDIA_VIDEO_DECODER_ERROR, //video data decoded error
    MSG_TYPE_MEDIA_AUDIO_DECODER_ERROR, //audio data decoded error
    MSG_TYPE_MEDIA_VIDEO_NOT_SUPPORT, //video format not support
    MSG_TYPE_MEDIA_AUDIO_NOT_SUPPORT, //audio format not support
    MSG_TYPE_MEDIA_NOT_SUPPORT, //media container  not support//30

    MSG_TYPE_NETWORK_INIT_OK,
    MSG_TYPE_NETWORK_MAC_OK,
    MSG_TYPE_NETWORK_WIFI_CONNECTED, //connect wifi
    MSG_TYPE_NETWORK_WIFI_SCANNING, //connect wifi
    MSG_TYPE_NETWORK_WIFI_SCAN_DONE, //connect wifi
    MSG_TYPE_NETWORK_WIFI_DISCONNECTED, //reconnect wifi fail
    MSG_TYPE_NETWORK_WIFI_CONNECTING, //connecting wifi
    MSG_TYPE_NETWORK_WIFI_CONNECT_FAIL, //connect wifi fail
    MSG_TYPE_NETWORK_WIFI_STATUS_UPDATE,
    MSG_TYPE_NETWORK_WIFI_RECONNECTE,//reconnect wifi
    MSG_TYPE_NETWORK_WIFI_RECONNECTED,//reconnect wifi
    MSG_TYPE_NETWORK_WIFI_PWD_WRONG,
    MSG_TYPE_NETWORK_WIFI_INIT_DONE,
    MSG_TYPE_NETWORK_WIFI_MAY_LIMITED,
    MSG_TYPE_NETWORK_HOSTAP_ENABLE,
    MSG_TYPE_NETWORK_DHCP_ON,
    MSG_TYPE_NETWORK_DHCP_OFF,
    MSG_TYPE_NETWORK_DEVICE_BE_CONNECTED, //connected by phone
    MSG_TYPE_NETWORK_DEVICE_BE_DISCONNECTED, //disconected by phone
    MSG_TYPE_NETWORK_DEV_NAME_SET,

    MSG_TYPE_CAST_DLNA_START,
    MSG_TYPE_CAST_DLNA_STOP,
    MSG_TYPE_CAST_DLNA_PLAY,
    MSG_TYPE_CAST_DLNA_PAUSE,
    MSG_TYPE_CAST_DLNA_MUTE,
    MSG_TYPE_CAST_DLNA_SEEK,
    MSG_TYPE_CAST_DLNA_VOL_SET,
    MSG_TYPE_CAST_AIRCAST_START,
    MSG_TYPE_CAST_AIRCAST_AUDIO_START,
    MSG_TYPE_CAST_AIRCAST_AUDIO_STOP,
    MSG_TYPE_CAST_AIRCAST_STOP,
    MSG_TYPE_CAST_AIRCAST_VOL_SET,
    MSG_TYPE_CAST_AIRMIRROR_START,
    MSG_TYPE_CAST_AIRMIRROR_STOP,
    MSG_TYPE_CAST_AIRP2P_READY,
    MSG_TYPE_CAST_AIRP2P_RESET,
    MSG_TYPE_CAST_MIRACAST_START,
    MSG_TYPE_CAST_MIRACAST_STOP,
    MSG_TYPE_CAST_MIRACAST_CONNECTING,
    MSG_TYPE_CAST_MIRACAST_CONNECTED,
    MSG_TYPE_CAST_MIRACAST_SSID_DONE,
    MSG_TYPE_CAST_MIRACAST_RESET,
    MSG_TYPE_CAST_MIRACAST_GOT_IP,

    MSG_TYPE_CAST_PARSE_END,
    MSG_TYPE_CAST_AUSB_START, //android usb mirror start
    MSG_TYPE_CAST_AUSB_STOP,  //android usb mirror stop
    MSG_TYPE_CAST_AUSB_START_UPGRADE,
    MSG_TYPE_CAST_IUSB_START, //apple usb mirror start
    MSG_TYPE_CAST_IUSB_STOP,  //apple usb mirror stop
    MSG_TYPE_CAST_IUSB_NEED_TRUST, //apple usb mirror need user to trust
    MSG_TYPE_CAST_IUSB_DEVICE_REMOVE, //apple usb mirror need user to trust
    MSG_TYPE_CAST_IUSB_NO_DATA, 
    MSG_TYPE_CAST_IUSB_COPYRIGHT_PROTECTION, 

    MSG_TYPE_AIR_INVALID_CERT,
    MSG_TYPE_VIDEO_DECODER_ERROR,
    MSG_TYPE_AUDIO_DECODER_ERROR,
    MSG_TYPE_HDMI_TX_CHANGED,
    MSG_TYPE_DLNA_HOSTAP_SKIP_URL,
    MSG_TYPE_AIR_HOSTAP_SKIP_URL,
    MSG_TYPE_AIR_MIRROR_BAD_NETWORK,
    MSG_TYPE_KEY_TRIGER_RESET,
    MSG_TYPE_ENTER_STANDBY,

    MSG_TYPE_IUM_DEV_ADD,
    MSG_TYPE_AUM_DEV_ADD,

    MSG_TYPE_BT_CONNECTED,
    MSG_TYPE_BT_DISCONNECTED,
	MSG_TYPE_BT_SCANED,
    MSG_TYPE_BT_SCAN_FINISH,
	MSG_TYPE_KEY_NUM,
	MSG_TYPE_CHANGE_DEV,
	MSG_TYPE_MP_MEMMORY_PLAY,
	MSG_TYPE_MP_MEMMORY_READ_FILELIST_END,
	MSG_TYPE_MP_MEMMORY_SAVE_PLAY_TIME,
    //command
	MSG_TYPE_WEBSERVICE_START,
    MSG_TYPE_WEBSERVICE_START_ERROR,
    MSG_TYPE_WEBSERVICE_LOADED,
    MSG_TYPE_WEBSERVICE_STOP,
    MSG_TYPE_NETDOWNLOAD,    //command
    MSG_TYPE_HCIPTV_INVALID_CERT,
	MSG_TYPE_BAR_NO_FILE_MSG,

    MSG_TYPE_MP_TRANS_PLAY,

    MSG_TYPE_PM_BATTERY_MONITOR,

    MSG_TYPE_CAST_DIAL_START,
    MSG_TYPE_CAST_DIAL_STOP,
    MSG_TYPE_CAST_DIAL_CONNECTING,
    MSG_TYPE_CAST_DIAL_CONNECTED,
    MSG_TYPE_CAST_DIAL_CONNECTED_FAILED,
    MSG_TYPE_CAST_DIAL_DISCONNECTING,
    MSG_TYPE_CAST_DIAL_DISCONNECTED,
    MSG_TYPE_CAST_DIAL_DISCONNECTED_ALL,
    MSG_TYPE_CAST_DIAL_ADD_VIDEO,
    MSG_TYPE_CAST_DIAL_DEL_VIDEO,
    MSG_TYPE_CAST_DIAL_INVALID_CERT,

    MSG_TYPE_CVBS_TRAINING_FINISH,

    MSG_TYPE_CMD = 1000,
	MSG_TYPE_REMOTE,
}msg_type_t;

typedef enum{
    CAST_STATE_IDLE = 0,
    CAST_STATE_AIRCAST_PLAY,
    CAST_STATE_ARIPLAY_PAUSE,
    CAST_STATE_DLNA_PLAY,
    CAST_STATE_DLNA_PAUSE,
    CAST_STATE_MIRACAST_PLAY,
    CAST_STATE_MIRACAST_PAUSE,
}cast_play_state_t;

typedef enum{
    NETWORK_STATE_DISCONNECT = 0,
    NETWORK_STATE_DHCP_ON,
    NETWORK_STATE_DHCP_OFF,
    NETWORK_STATE_CONNECT,
}cast_network_state_t;

typedef enum _E_BT_CONNECT_STATUS_
{
    BT_CONNECT_STATUS_DEFAULT,
    BT_CONNECT_STATUS_CONNECTING,
    BT_CONNECT_STATUS_DISCONNECTING,
    BT_CONNECT_STATUS_DISCONNECTED,
    BT_CONNECT_STATUS_CONNECTED,
}bt_connect_status_e;

typedef enum{
    WIFI_PM_STATE_NONE,
    WIFI_PM_STATE_GPIO,
    WIFI_PM_STATE_PS,
}wifi_pm_state_e;

typedef enum _mp_ui_err_e_
{
    MP_UI_ERR_OK = 0,
    MP_UI_ERR_UNKNOWN,
    MP_UI_ERR_UNDEFINE,
    MP_UI_ERR_CONTAINER_NOT_SUPPORT = 90,
    MP_UI_ERR_AUDIO_DECODE_ERROR    = 100,
    MP_UI_ERR_AUDIO_NOT_SUPPORT,
    MP_UI_ERR_VIDEO_DECODE_ERROR    = 200,
    MP_UI_ERR_VIDEO_NOT_SUPPORT,
    MP_UI_ERR_IMG_DECODE_ERROR      = 300,
    MP_UI_ERR_NETWORK_ERROR         = 400,
} mp_ui_err_e;

#define NETWORK_STATE_DISCONNECT         (0)
#define NETWORK_STATE_DHCP_ON            (1 << 0)
#define NETWORK_STATE_PHONE_CONNECT      (1 << 1) //the phone connnet to device OK
#define NETWORK_STATE_WIFI_AP_CONNECT    (1 << 2) //the devoce connect to wifi AP OK


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


#ifndef MKTAG
  #define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#endif

typedef struct{
	msg_type_t	msg_type;
	uint32_t	msg_code;
}control_msg_t;


typedef void (*screen_ctrl)(void *arg1, void *arg2);
typedef struct{
    void *screen;
    screen_ctrl control; //the control function is to process the message.
}screen_entry_t;


//use for app & ui 
typedef struct partition_info{
    int count; //number of all partitions
    void* dev_list;
	char * used_dev;
    int m_storage_state;    //当前的存储状态
}partition_info_t;

typedef struct{
    uint16_t left;
    uint16_t top;

    uint16_t h_div;
    uint16_t v_div;
    uint16_t h_mul;
    uint16_t v_mul;
}fb_zoom_t;

enum{
    APP_DIS_TYPE_SD = 0,
    APP_DIS_TYPE_HD,
    APP_DIS_TYPE_UHD,
};

enum{
    APP_DIS_LAYER_MAIN = 0,
    APP_DIS_LAYER_AUXP,
};

enum{
    VIDEO_PBP_SRC_MP = 0,
    VIDEO_PBP_SRC_HDMI_IN,
    VIDEO_PBP_SRC_CVBS_IN,
    VIDEO_PBP_SRC_DLNA,
    VIDEO_PBP_SRC_MIRACAST,
    VIDEO_PBP_SRC_AIRCAST,
    VIDEO_PBP_SRC_AIRP2P,
    VIDEO_PBP_SRC_USB_MIRROR,
};


//#define BACK_LOGO "/hcdemo_files/HCCAST_1920x1080.264"
#define BACK_LOGO "/hcdemo_files/public_cast_root_bg.264"
#define MUSIC_LOGO "/hcdemo_files/music_bg_logo.264"
#define UM_BACK_LOGO "/hcdemo_files/um_bg_logo.264"

#define WLAN0_NAME      "wlan0"
#define WLAN1_NAME      "wlan1"

#define     DEV_HDMI        "/dev/hdmi"
#define     DEV_DIS         "/dev/dis"
#define     DEV_FB          "/dev/fb0"

void hc_control_init(void);

void api_system_pre_init(void);
int api_system_init(void);
int api_video_init(void);
int api_audio_init(void);

cast_play_state_t api_cast_play_state_get(void);
void api_cast_play_state_set(cast_play_state_t state);

int api_control_send_msg(control_msg_t *control_msg);
int api_control_receive_msg(control_msg_t *control_msg);
void api_control_clear_msg(void);
int api_control_send_key(uint32_t key);

int api_dis_suspend(void);
int api_dis_show_onoff(bool on_off);
int api_logo_show(const char *file);
int api_logo_reshow(void);
int api_logo_off(void);
int api_logo_off2(int closevp, int fillblack);
int api_logo_off_no_black(void);

bool api_is_ip_addr(char *ip_buff);
int api_get_mac_addr(char *mac);

int webs_start(void);
int webs_stop(void);

void api_sleep_ms(uint32_t ms);
int api_shell_exe_result(char *cmd);

void app_ffplay_init(void);
void app_ffplay_deinit(void);
void app_exit(void);
int mmp_get_usb_stat(void);
char *mmp_get_usb_dev_name(void);
int api_control_send_media_message(uint32_t mediamsg_type);

void api_system_reboot(void);

#ifdef __linux__
int api_dts_uint32_get(const char *path);
void api_dts_string_get(const char *path, char *string, int size);
#endif
void api_screen_regist_ctrl_handle(screen_entry_t *entry);
screen_ctrl api_screen_get_ctrl(void *screen);
screen_entry_t* api_screen_get_ctrl_entry(void *screen);
int sys_upg_flash_burn(char *buff, uint32_t length);
void key_set_group(lv_group_t *key_group);
void set_remote_control_disable(bool b);
int api_osd_show_onoff(bool on_off);
void api_system_standby(void);
void api_osd_off_time(uint32_t timeout);

bool api_hotkey_enable_get(uint32_t key);
void api_hotkey_enable_set(uint32_t *enable_key, uint32_t key_cnt);
void api_hotkey_disable_clear(void);
void api_hotkey_disable_all(void);

extern int hccast_upgrade_download_config(char *url, char *buffer, int buffer_len);
extern void hccast_web_uprade_download_start(char *url);
extern void hccast_web_set_user_abort(int flag);

int api_get_flip_info(int *rotate_type, int *flip_type);

int api_get_mtdblock_devpath(char *devpath, int len, const char *partname);
#ifdef __HCRTOS__
int api_romfs_resources_mount(void);

int api_ramdisk_open(uint32_t size);
int api_ramdisk_close();
char *api_ramdisk_path_get();

#endif

int api_media_mute(bool mute);
int api_media_set_vol(uint8_t volume);
void api_set_i2so_gpio_mute(bool val);//1 mute 0 no mute
void api_set_i2so_gpio_mute_auto(void);
void api_get_ratio_rect(struct vdec_dis_rect *new_rect, dis_tv_mode_e ratio);
void api_set_display_area(dis_tv_mode_e ratio);
void api_set_display_zoom(int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h);
void api_set_display_aspect(dis_tv_mode_e ratio , dis_mode_e dis_mode);
int api_get_display_area(dis_screen_info_t * dis_area);
int api_set_display_zoom2(dis_zoom_t* diszoom_param);
int api_get_screen_info(dis_screen_info_t * dis_area);

void *api_ffmpeg_player_get(void);
void api_ffmpeg_player_get_regist(void *(func)(void));
int api_usb_dev_path_get(char dev_path[][MAX_UDISK_DEV_LEN], int dev_cnt);
void api_transfer_rotate_mode_for_screen(int init_rotate ,
                                         int init_h_flip ,
                                         int init_v_flip ,
                                         int *p_rotate_mode ,
                                         int *p_h_flip ,
                                         int *p_v_flip ,
                                         int *p_fbdev_rotate);

void api_usb_dev_check(void);
int api_pic_effect_enable(bool enable);

void* mmp_get_partition_info(void);
void api_set_bt_connet_status(bt_connect_status_e val);
bt_connect_status_e api_get_bt_connet_status(void);
uint16_t api_get_screen_init_rotate(void);
uint16_t api_get_screen_init_h_flip(void);
uint16_t api_get_screen_init_v_flip(void);
void api_get_screen_rotate_info(void);

int api_watchdog_init(void);
int api_watchdog_timeout_set(uint32_t timeout_ms);
int api_watchdog_timeout_get(uint32_t *timeout_ms);
int api_watchdog_start(void);
int api_watchdog_stop(void);
int api_watchdog_feed(void);
#ifdef BACKLIGHT_MONITOR_SUPPORT
void api_pwm_backlight_monitor_init(void);
void api_pwm_backlight_monitor_loop(void);
void api_pwm_backlight_monitor_update_status(void);
#endif
int api_set_partition_info(int index);
bool api_check_partition_used_dev_ishotplug(void);

char *api_rsc_string_get(uint32_t string_id);
uint32_t api_sys_tick_get(void);

char *api_rsc_string_get_by_langid(uint16_t lang_id, uint32_t string_id);
int api_media_pic_backup(int time);
int api_media_pic_backup_free(void);
void win_upgrade_type_set(uint32_t upgrade_type);
int api_set_backlight_brightness(int val);
int api_get_backlight_level_count(uint8_t *level_count);
int api_get_backlight_default_brightness_level(uint8_t *brightness_level);

bool api_group_is_valid(lv_group_t *g);
lv_group_t* api_get_group_by_scr(lv_obj_t* scr);
bool api_group_contain_obj(lv_group_t *g, lv_obj_t* obj);

bool ui_mute_get(void);
void ui_mute_set(void);
int api_logo_dis_backup_free(void);
int partition_info_update(int usb_state,void* dev);
bool api_storage_devinfo_state_get(void);
void api_storage_devinfo_state_set(bool state);
int api_storage_devinfo_check(char* device,char* filename);

int64_t api_get_sys_clock_time(void);

void api_sys_clock_time_check_start(void);
int64_t api_sys_clock_time_check_get(int64_t *last_clock);
char *api_get_partition_info_by_index(int index);
int api_partition_info_used_disk_set(char *disk);
bool api_media_pic_is_backup(void);

int api_set_next_flip_mode(void);
int api_set_flip_mode_enable(int enable);
bool api_backlight_is_enable(void);
int api_set_wifi_pm_state(int state);
int api_get_wifi_pm_state(void);
int api_get_wifi_pm_mode(void);
int api_wifi_pm_plugin_handle(void);
int api_wifi_pm_open(void);
int api_wifi_pm_stop(void);
void api_swap_value(void *a, void *b, int len);

void api_enter_flash_rw(void);
void api_leave_flash_rw(void);
void api_is_upgrade_set(bool upgrade);
bool api_is_upgrade_get(void);
int api_wifi_get_bus_port(void);

int api_wifi_disconnect(void);

#ifdef USBMIRROR_SUPPORT
bool cast_um_play_state(void);
#endif

#ifdef MULTI_OS_SUPPORT
void find_icube_enter_multi_os(void);
#endif

#ifdef __linux__
void api_hw_watchdog_mmap_open(void);
void api_hw_watchdog_mmap_close(void);
#endif

bool api_video_pbp_get_support(void);
int api_video_pbp_get_dis_type(int src_type);
int api_video_pbp_get_dis_layer(int src_type);
void api_video_pbp_set_dis_type(int src_type);
void api_video_pbp_set_dis_layer(int src_type);

void api_fb_set_zoom_arg(fb_zoom_t *fb_zoom);
int api_fb_get_zoom_arg(fb_zoom_t *fb_zoom);
void api_set_flip_arg(int rotate, int hor_flip, int ver_flip);
int api_get_flip_arg(int *rotate, int *hor_flip, int *ver_flip);

void *api_upgrade_buffer_alloc(unsigned long size);
void api_upgrade_buffer_sync(void);
void api_upgrade_buffer_free(void *ptr);
#ifdef APPMANAGER_SUPPORT
#define UPGRADE_APP_TEMP_OTA_BIN    "/tmp/HCFOTA.bin"
int api_app_command_send(const char *cmd);
int api_app_command_upgrade(const char *url);
#endif

int api_spdif_bypass_support_get(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
