#ifndef __HC_BOARDTEST_MODULE_
#define __HC_BOARDTEST_MODULE_
#ifdef __cplusplus
extern "C"
{
#endif

#include <kernel/module.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BOARDTEST_INI_NAME "hichip_boardtest/boardtest_config.ini"

typedef enum
{
    BOARDTEST_CALL_PASS = 0, /*call the result, if run function return this, and no mbox result, then thinks the test is stress test*/
    BOARDTEST_PASS,          /*test result, If the run function can directly produce the final result of the entire test item, it returns this*/
    BOARDTEST_RESULT_PASS,
    BOARDTEST_FAIL,
    BOARDTEST_GOING,
    BOARDTEST_PENDING,
    /*error*/
    BOARDTEST_ERROR_OPEN_FILE,     /*open file failed*/
    BOARDTEST_ERROR_CLOSE_FILE,    /*close file failed*/
    BOARDTEST_ERROR_READ_FILE,     /*read file failed*/
    BOARDTEST_ERROR_OPEN_DEVICE,   /*open device failed*/
    BOARDTEST_ERROR_MOLLOC_MEMORY, /*malloc memory failed*/
    BOARDTEST_ERROR_FREE_MEMORY,   /*free memory failed*/
    BOARDTEST_ERROR_MEDIA_FILE,    /*play media file failed*/
    BOARDTEST_ERROR_IOCTL_DEVICE,  /*ioctl device failed*/
    BOARDTEST_ERROR_CREATE_DATA,   /*create sys data failed*/
    BOARDTEST_ERROR_STORE_DATA,    /*store app data failed*/
    BOARDTEST_ERROR_PRAR_DATA,     /*wrong parameter data*/
    BOARDTEST_ERROR_CHECK_DATA,    /*check data wrong*/
    BOARDTEST_ERROR_SOCKET_DATA,   /*socket failed*/

    /*
    ... ...
    */
    BOARDTEST_ERROR_NUM,
} boardtest_status_e;

typedef enum
{
    /*Automated test items*/
    BOARDTEST_FW_VERSION = 0,       /*Firmware version*/
    BOARDTEST_PRODUTC_ID,           /*Device model*/
    BOARDTEST_WIFI_MAC,             /*WiFi MAC*/
    BOARDTEST_ETH_MAC,              /*eth MAC*/
    BOARDTEST_DDR_DATA,             /*DDR data*/
    BOARDTEST_CPU_DATA,             /*CPU data*/
    BOARDTEST_USB_TEST,             /*USB mounting*/
    BOARDTEST_SD_TEST,              /*SD mounting*/
    BOARDTEST_ETHERNET_LINK_STATUS, /*Ethernet connection status*/
    BOARDTEST_WIFI_TEST,            /*WiFi test*/
    BOARDTEST_BLUETOOTH_TEST,       /*Bluetooth test*/
    /*Test items manually*/
    BOARDTEST_LED_DISPLAY,         /*LED display*/
    BOARDTEST_SPDIF_AUDIO_TEST,    /*SPDIF left channel test*/
    BOARDTEST_HDMI_TX_STATUS,      /*HDMI TX Access status*/
    BOARDTEST_HDMI_AUDIO_TEST,     /*Sound test*/
    BOARDTEST_CVBS_STATUS,         /*CVBS state*/
    BOARDTEST_CVBS_LEFT_CHANNEL,   /*CVBS left channel test*/
    BOARDTEST_CVBS_RIGHT_CHANNEL,  /*CVBS right channel test*/
    BOARDTEST_VIDEO_TEST,          /*Video test*/
    BOARDTEST_INPUT_TEST,          /*Input test*/
    BOARDTEST_HDMI_IN,             /*HDMI IN test*/
    BOARDTEST_CVBS_IN,             /*CVBS-in test*/
    /*Stress automatic test items*/
    BOARDTEST_DDR_STRESS_TEST,  /*DDR pressure automatic test*/
    BOARDTEST_VIDEO_IMAGE_LOOP, /*Video pictures loop*/
    /*
    ......
    */
    BOARDTEST_NUM, /*29*/
} boardtest_module_e;

typedef enum
{
    BOARDTEST_DISABLE = 0,
    BOARDTEST_ENABLE,
} boardtest_isabled_e;

typedef struct
{
    // char *chinese_name;
    char *english_name;         /*Corresponds to .ini profile*/
    boardtest_module_e sort_name; /*The header file is defined*/
    int (*init)(void);
    int (*run)(void);
    int (*exit)(void);
    char *tips; /*mbox prompt content*/
} hc_boardtest_msg_reg_t;

typedef struct
{
    hc_boardtest_msg_reg_t *boardtest_msg_reg;
    boardtest_isabled_e isabled; /*read ini isabled*/
    boardtest_status_e state;    /*state + error code*/
    int run_time;
    bool msg_reg_flag;
    char *detail;
} hc_boardtest_msg_t;

/**
 * @description: boardtest registration function
 * @param {hc_boardtest_msg_reg_t} *test
 * @return {*}
 */
void hc_boardtest_module_register(hc_boardtest_msg_reg_t *test);

/**
 * @description: create mbox, fail or pass butten
 * @param {boardtest_module_e} sort_name
 *         name of this boardtest enumeration
 * @return {*}
 */
void create_boardtest_passfail_mbox(boardtest_module_e sort_name);

/**
 * @description: write boardtest detail ,will
 * @param {boardtest_module_e} sort_name
 *         name of this boardtest enumeration
 * @return {*}
 */
void write_boardtest_detail(boardtest_module_e sort_name, char *detail);

/*displayed in the interface instantly*/
void write_boardtest_detail_instantly(boardtest_module_e sort_name, char *detail);

/*simulation of full registration*/
void hc_boardtest_module_all_register(void);

/*close lvgl osd, mbox on the top, don't be closed*/
void close_lvgl_osd(void);

void open_lvgl_osd(void);

hc_boardtest_msg_t *hc_boardtest_msg_get(int sort);

const char *hc_boardtest_error_msg_get(int sort);

#ifdef __cplusplus
}
#endif
#endif
