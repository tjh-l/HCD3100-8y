#ifndef __HUDI_BLUETOOTH_INTER_H__
#define __HUDI_BLUETOOTH_INTER_H__

#include <hudi_com.h>
#include <hudi_bluetooth.h>

#define UART_RX_BUFFER_SIZE     1024
#define PROTO_FRAME_CMD_SIZE    128
#define BLUETOOTH_MAC_LEN       6
#define BLUETOOTH_NAME_LEN      128
#define BLUETOOTH_ADCKEY_MAX_NUM    16
#define BLUETOOTH_LOCAL_NAME_LEN    32
#define BLUETOOTH_MAX_VOLUME_RANGE   32
#define BLUETOOTH_DEVICE_TREE_PATH      "/proc/device-tree/bluetooth/"


typedef struct
{
    char device_id [32];
    char device_name[32];
    char firmware_ver[32];
    int baudrate;
    int audio_volume;
    int audio_input_ch;
    uint8_t signal_channelmap[10];
    hudi_bluetooth_mode_e mode;
    hudi_bluetooth_status_e status;
    hudi_bluetooth_device_t connected_res;
    hudi_bluetooth_device_t inquiry_res;
    hudi_bluetooth_extio_input_t input_pinset;
    int (*proto_parse)(hudi_handle handle, char *buf, int len);
    int (*rf_on)(hudi_handle handle);
    int (*rf_off)(hudi_handle handle);
    int (*scan_async)(hudi_handle handle);
    int (*scan_abort)(hudi_handle handle);
    int (*connect)(hudi_handle handle, char *device_mac);
    int (*disconnect)(hudi_handle handle);
    int (*ignore)(hudi_handle handle);
    int (*volume_set)(hudi_handle handle, int vol);
    int (*volume_get)(hudi_handle handle, int *vol);
    int (*audio_input_ch_set)(hudi_handle handle, hudi_bluetooth_audio_ch_e ch);
    int (*audio_input_ch_get)(hudi_handle handle, hudi_bluetooth_audio_ch_e *ch);
    int (*audio_output_ctrl)(hudi_handle handle, hudi_bluetooth_audio_ctrl_e ctrl);
    int (*mode_switch)(hudi_handle handle, hudi_bluetooth_mode_e mode);
    int (*mode_get)(hudi_handle handle, hudi_bluetooth_mode_e *mode);
    int (*status_get)(hudi_handle handle, hudi_bluetooth_status_e *sta);
    int (*localname_set)(hudi_handle handle, char *name);
    int (*localname_get)(hudi_handle handle, char *name);
    int (*firmware_ver_get)(hudi_handle handle, char *version);
    int (*extio_func_set)(hudi_handle handle, int pinpad, hudi_bluetooth_extio_func_e func);
    int (*extio_input)(hudi_handle handle, int pinpad, int *value);
    int (*extio_output)(hudi_handle handle, int pinpad, int value);
    int (*extio_pwm_set)(hudi_handle handle, int pinpad, int freq, int duty);
    int (*standby_key_set)(hudi_handle handle, int standby_irkey);
    int (*standby_enter)(hudi_handle handle);
    int (*reboot)(hudi_handle handle);
    int (*ir_usercode_set)(hudi_handle handle, int usercode);
    int (*signal_ch_set)(hudi_handle handle, char *bitmap);
    int (*signal_ch_get)(hudi_handle handle, char *bitmap);
    int (*extio_standby_status_set)(hudi_handle handle, int pinpad, int value);
} hudi_bluetooth_module_st;


typedef struct
{
    char dev_path[16];
    char status[16];
    void *priv;
} bluetooth_dts_resource_t;

typedef struct
{
    int inited;
    int fd;
    int event_polling;
    int polling_stop;
    long int polling_tid;
    void *polling_sem;
    void *user_data;
    hudi_bluetooth_module_st *module;
    hudi_bluetooth_cb notifier;
    bluetooth_dts_resource_t dts_res;
} hudi_bluetooth_instance_t;



/**
* @brief       Register specific bluetooth module instance to hudi interface layer
* @param[in]   handle  Handle of the instance
* @param[in]   module  Bluetooth module instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_module_register(hudi_handle handle, hudi_bluetooth_module_st *module);

/**
* @brief       Write data to bluetooth fd
* @param[in]   handle  Handle of the instance
* @param[in]   buf     Write buffer pointer
* @param[in]   len     Write buffer size
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_write(hudi_handle handle, char *buf, int len);

/**
* @brief       Read data from bluetooth fd
* @param[in]   handle  Handle of the instance
* @param[in]   buf     Read buffer pointer
* @param[in]   len     Read buffer size
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_read(hudi_handle handle, char *buf, int len);


/**
* @brief       Send bluetooth input device event
* @param[in]   handle  Handle of the instance
* @param[in]   func    Callback function
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_input_event_send(hudi_bluetooth_input_event_t event);

/**
* @brief       Configure bluetooth uart baudrate
* @param[in]   fd       file fd
* @param[in]   baudrate uart baudrate
* @retval      0        Success
* @retval      other    Error
*/
int hudi_bluetooth_uart_configure(int fd, int baudrate);

#endif
