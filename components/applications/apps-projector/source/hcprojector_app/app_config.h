/*
app_config.h: the global config header file for application
 */
#ifndef __HCDEMO_CONFIG_H__
#define __HCDEMO_CONFIG_H__

#ifdef __linux__
#include <stdbool.h> //bool
#include <stdint.h> //uint32_t
#include <sys/types.h> //uint
#else
#include <generated/br2_autoconf.h>
#endif




//--------------------- custom (客户名: xxx)                                                                                                                                                                                                                                

#define ADD_SELECT_BLUE_RECEIVE_FOR_SOURCE
#define SUPPORT_BLUE_RECEIVE
#define SUPPORT_BT_RECEIVE_MODE







//--------------------- public macro(宏) 

#define PIN_MUTE PINPAD_T05   //hight mute,low no-mute
#if 0//0
    /*---------------hight mute, low no-mute---------------*/
    #define SET_MUTE \
            {gpio_configure(PIN_MUTE, GPIO_DIR_OUTPUT); \
            gpio_set_output(PIN_MUTE, 1);}

    #define SET_UNMUTE \
            {gpio_configure(PIN_MUTE, GPIO_DIR_OUTPUT); \
            gpio_set_output(PIN_MUTE, 0);}
#else
    /*---------------low mute, hight no-mute---------------*/
    #define SET_MUTE \
            {gpio_configure(PIN_MUTE, GPIO_DIR_OUTPUT); \
            gpio_set_output(PIN_MUTE, 0);}

    #define SET_UNMUTE \
            {gpio_configure(PIN_MUTE, GPIO_DIR_OUTPUT); \
            gpio_set_output(PIN_MUTE, 1);}
#endif



#ifdef CONFIG_APPS_PROJECTOR_BT_DAC_SPDIF
    #define SUPPORT_INPUT_BLUE_SPDIF_IN
    #define SUPPORT_BT_CTRL_VOLUME_LEVER
#endif

#define PANEL_WIDTH 1280
#define AUTOKEYSTONE_SWITCH

#ifdef AUTOKEYSTONE_SWITCH
    #define ANGLE_ALPHA 22
	#define ATK_CALIBRATION
#endif

#define NEW_SETUP_ITEM_CTRL

#ifdef NEW_SETUP_ITEM_CTRL
    #if PANEL_WIDTH == 1920
        #define ADJUSTMENT_SCALE 16  // 手动梯形调节尺度
    #elif PANEL_WIDTH == 1280
        #define ADJUSTMENT_SCALE 8   // 手动梯形调节尺度
    #elif PANEL_WIDTH == 600
        #define ADJUSTMENT_SCALE 5   // 手动梯形调节尺度
    #endif
#endif


#define FOCUSING_ANIMATION_ON
#define VMOTOR_LIMIT_ON

#ifdef VMOTOR_LIMIT_ON
    #define PIN_VMOTOR_LIMIT PINPAD_T03
    #define LIMIT_LEVEL_SIGNAL 0
#endif


//#define SUPPORT_UM_QR_SCAN





//---- osd language -----------------
#define LANGUAGE_UI_CHANGE

#define DEFAULT_OSD_ENGLISH

#define SUPPORT_OSD_TCHINESE  (1)
#define SUPPORT_OSD_FRENCH    (1)
#define SUPPORT_OSD_GERMAN    (1)
#define SUPPORT_OSD_SPANISH   (1)
#define SUPPORT_OSD_PORTUGUESE   (1)
#define SUPPORT_OSD_ITALIAN   (1)
#define SUPPORT_OSD_POLISH    (1)
#define SUPPORT_OSD_SWEDISH   (1)
#define SUPPORT_OSD_FINNISH   (1)
#define SUPPORT_OSD_GREEK     (1)
#define SUPPORT_OSD_DANISH    (1)
#define SUPPORT_OSD_NORWEGIAN  (1)
#define SUPPORT_OSD_HUNGARY    (1)
#define SUPPORT_OSD_HEBREW     (1)
#define SUPPORT_OSD_RUSSIAN    (1)
#define SUPPORT_OSD_VIETNAMESE (1)
#define SUPPORT_OSD_THAI       (1)
#define SUPPORT_OSD_ARABIC  (1)
#define SUPPORT_OSD_JAPANESE    (1)
#define SUPPORT_OSD_KOREAN    (1)
#define SUPPORT_OSD_INDONESIAN   (1)
#define SUPPORT_OSD_DUTCH   (1)
#define SUPPORT_OSD_TURKEY   (1)
//-------------------------------------------------------------


#ifdef CONFIG_APPS_PROJECTOR_HDMIIN
#define HDMIIN_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_CVBSIN
#define CVBSIN_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_CVBS_TRAINING
#define CVBSIN_TRAINING_SUPPORT
#endif

#ifdef CONFIG_APPS_WATCHDOG_SUPPORT
#define WATCHDOG_SUPPORT
#endif

#ifdef CONFIG_APPS_WATCHDOG_TIMEOUT
    #define WATCHDOG_TIMEOUT CONFIG_APPS_WATCHDOG_TIMEOUT
#else
    #define WATCHDOG_TIMEOUT 30000
#endif

#ifdef CONFIG_APPS_PROJECTOR_BLUETOOTH
#define BLUETOOTH_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_DLNA
#define DLNA_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_DIAL
#define DIAL_SUPPORT
#endif

#ifdef BR2_PACKAGE_HUDI_FLASH
#define HUDI_FLASH_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_AIRCAST
#define AIRCAST_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_MIRACAST
#define MIRACAST_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_USBMIRROR
#define USBMIRROR_SUPPORT
#endif

#if defined(DLNA_SUPPORT) || defined(AIRCAST_SUPPORT) || defined(MIRACAST_SUPPORT) || \
defined(CONFIG_AUTO_HTTP_UPGRADE) || defined(CONFIG_MANUAL_HTTP_UPGRADE)
#define NETWORK_SUPPORT
#define WIFI_SUPPORT
#endif


#if defined(DLNA_SUPPORT) || defined(AIRCAST_SUPPORT) || defined(MIRACAST_SUPPORT) || defined(USBMIRROR_SUPPORT)
#define CAST_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_VMOTOR_DRIVE
#define PROJECTOR_VMOTOR_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_MAIN_PAGE
#define MAIN_PAGE_SUPPORT
#endif

#define PROJECTER_C1_VERSION  0 //customer 1
#define PROJECTER_C2_VERSION  0 //customer 2
#define PROJECTER_C2_D3000_VERSION  0 //customer 2

#ifdef CONFIG_APPS_PROJECTOR_CVBS_AUDIO_I2SI_I2SO
#define CVBS_AUDIO_I2SI_I2SO    // this macro is for c1 ddr3 custmboard
#endif

#ifdef CONFIG_APPS_PROJECTOR_CAST_720P
#define CAST_720P_SUPPORT  1 // default 1080p
#endif

#define HTTPD_SERVICE_SUPPORT


//dump air/miracast/usb mirror ES data to U-disk
//#define MIRROR_ES_DUMP_SUPPORT

#ifdef BR2_PACKAGE_FFMPEG_SWSCALE
  #define  RTOS_SUBTITLE_SUPPORT
#endif

#if defined(BR2_PACKAGE_PREBUILTS_LIBSPECTRUM) || defined(BR2_PACKAGE_PREBUILTS_SPECTRUM)
  #define AUDIO_SPECTRUM_SUPPORT
#endif

#ifdef BR2_PACKAGE_FFMPEG_SWSCALE
  #define FFMPEG_SWSCALE_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_KEYSTONE
  #define KEYSTONE_SUPPORT 1
  #define KEYSTONE_STRETCH_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_BACKLIGHT_MONITOR
#define BACKLIGHT_MONITOR_SUPPORT
#endif

#ifdef CONFIG_SOC_HC15XX
	#define CASTING_CLOSE_FB_SUPPORT 1  // close fb during mirroring
#else
	#define CASTING_CLOSE_FB_SUPPORT 0
#endif

#ifdef __HCRTOS__
    #if CONFIG_WDT_AUTO_FEED 
      #define WATCHDOG_KERNEL_FEED
    #endif
#endif

#ifdef CONFIG_APPS_PROJECTOR_LVGL_RESOLUTION_240P
  	#define LVGL_RESOLUTION_240P_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_LVGL_MBOX_STANDBY
#define LVGL_MBOX_STANDBY_SUPPORT
#endif

#ifdef CONFIG_AUTO_HTTP_UPGRADE
  #define AUTO_HTTP_UPGRADE
#endif

#ifdef CONFIG_MANUAL_HTTP_UPGRADE
  #define MANUAL_HTTP_UPGRADE
#endif

#ifdef CONFIG_APPS_PROJECTOR_SYS_ZOOM
  #define SYS_ZOOM_SUPPORT 1
#endif

#ifdef BR2_PACKAGE_LIBCURL
  #define LIBCURL_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_USB_AUTO_UPGRADE
  #define USB_AUTO_UPGRADE
#endif

#ifdef CONFIG_APPS_PROJECTOR_HCIPTV_YTB
  #define HCIPTV_YTB_SUPPORT
#endif

#if defined(CONFIG_APPS_PROJECTOR_AIRP2P) && defined(BR2_PACKAGE_HCCAST_AIRP2P_SUPPORT)
  #define AIRP2P_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_MULTI_OS
  #define MULTI_OS_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_FACTORY_SET
  #define HC_FACTORY_TEST 1
#endif

#ifdef CONFIG_APPS_IMAGE_DISPLAY_MODE_CHANGE
//enable it. support change image display mode(realsize, full,etc) in playbar
  #define IMAGE_DISPLAY_MODE_SUPPORT
#endif

#ifdef CONFIG_APPS_USB_MIRROR_FAST_MODE
#define USB_MIRROR_FAST_SUPPORT
#endif

#ifdef CONFIG_APPS_TRANSCODE_THUMBNIAL_SHOW
//tanscode prevew in media list support.
#define MP_TRANS_PREV_SUPPORT
#endif

#ifdef BR2_PACKAGE_AC6956C_GX
#define BT_AC6956C_GX
#endif

#ifdef CONFIG_APPS_PROJECTOR_HDMI_SWITCH_SUPPORT
#define HDMI_SWITCH_SUPPORT
#endif

#if PROJECTER_C2_VERSION
#define HDMI_SWITCH_SUPPORT
#endif

#ifdef CONFIG_APPS_MEDIA_MEMORY_PLAY
#define	HC_MEDIA_MEMMORY_PLAY		//memmory play ebook video
#endif

#ifdef CONFIG_APPS_PROJECTOR_BATTERY_MONITOR
#define BATTERY_SUPPORT
#endif

#if defined(__HCRTOS__) && defined(WIFI_SUPPORT)
#define WIFI_PM_SUPPORT
#endif

#ifdef CONFIG_APPS_BLUETOOTH_SPEAKER_MODE_SUPPORT
#define BLUETOOTH_SPEAKER_MODE_SUPPORT
#endif 

#ifdef CONFIG_APP_DRC_GAIN
#define DRC_GAIN_SUPPORT
#endif 
#ifdef CONFIG_APPS_LCD_ROTATE
#define LCD_ROTATE_SUPPORT
#endif

#ifdef CONFIG_APPS_VIDEO_PBP
#define VIDEO_PBP_MODE_SUPPORT
#endif

#ifdef CONFIG_APPS_PROJECTOR_BT_ANTI_INTERFERENCE
#define BLUETOOTH_CHANNEL_OPTIMIZE
#endif

#ifdef CONFIG_APPS_PROJECTOR_UIBC_SUPPORT
#define UIBC_SUPPORT
#endif

#ifdef BR2_PACKAGE_APPMANAGER
#define APPMANAGER_SUPPORT
#endif

#ifdef CONFIG_HCAPP_NAME_PROJECTOR
#define HC_APP_NAME CONFIG_HCAPP_NAME_PROJECTOR
#else
#define HC_APP_NAME "hcprojector"
#endif

#ifdef CONFIG_HCAPP_NAME_UPGRADE
#define HC_APP_UPGRADE CONFIG_HCAPP_NAME_UPGRADE
#else
#define HC_APP_UPGRADE "upgradeapp"
#endif

#ifdef CONFIG_APPS_HDMI_RX_CEC_SUPPORT
#define HDMI_RX_CEC_SUPPORT
#endif

#endif //end of __HCDEMO_CONFIG_H__
