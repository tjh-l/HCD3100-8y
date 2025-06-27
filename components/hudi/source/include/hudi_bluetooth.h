#ifndef __HUDI_BLUETOOTH_H__
#define __HUDI_BLUETOOTH_H__

#include "hudi_com.h"


#ifdef __cpluscplus
extern "C" {
#endif

typedef enum
{
    HUDI_BLUETOOTH_EVT_CONNECT_RES = 0,             /*< ! arg : hudi_bluetooth_device_t*  */
    HUDI_BLUETOOTH_EVT_CONNECT_TIMEOUT,             /*< ! arg : NULL */
    HUDI_BLUETOOTH_EVT_RECONNECTING,                /*< ! arg : NULL */
    HUDI_BLUETOOTH_EVT_RECONNECTED,                 /*< ! arg : NULL */
    HUDI_BLUETOOTH_EVT_DISCONNECT_RES,              /*< ! arg : NULL */
    HUDI_BLUETOOTH_EVT_DEV_STATUS_CONNECTED,        /*< ! arg : NULL */
    HUDI_BLUETOOTH_EVT_DEV_STATUS_DISCONNECTED,     /*< ! arg : NULL */
    HUDI_BLUETOOTH_EVT_INQUIRY_COMPLETE,            /*< ! arg : NULL */
    HUDI_BLUETOOTH_EVT_INQUIRING,                   /*< ! arg : NULL */
    HUDI_BLUETOOTH_EVT_INQUIRY_RES,                 /*< ! arg : hudi_bluetooth_device_t*  */
    HUDI_BLUETOOTH_EVT_GPI_STATUS,                  /*< ! arg : hudi_bluetooth_extio_input_t*  */
    HUDI_BLUETOOTH_EVT_MAX,
} hudi_bluetooth_event_e;

typedef enum
{
    HUDI_BLUETOOTH_STATUS_NONE = 0,
    HUDI_BLUETOOTH_STATUS_CONNECTED,
    HUDI_BLUETOOTH_STATUS_DISCONNECTED,
    HUDI_BLUETOOTH_STATUS_INQUIRY,
    HUDI_BLUETOOTH_STATUS_INQUIRY_COMPLETE,
} hudi_bluetooth_status_e;

typedef enum
{
    HUDI_BLUETOOTH_MODE_MASTER,
    HUDI_BLUETOOTH_MODE_SLAVE,
} hudi_bluetooth_mode_e;

typedef enum
{
    HUDI_BLUETOOTH_AC6956CGX_EXTIO_PB10  = 6,
    HUDI_BLUETOOTH_AC6956CGX_EXTIO_PB9   = 7,
    HUDI_BLUETOOTH_AC6956CGX_EXTIO_PB2   = 13,
    HUDI_BLUETOOTH_AC6956CGX_EXTIO_PB0   = 15,
    HUDI_BLUETOOTH_AC6956CGX_EXTIO_PC3   = 21,
    HUDI_BLUETOOTH_AC6956CGX_EXTIO_PC2   = 22,
    HUDI_BLUETOOTH_AC6956CGX_EXTIO_PA10  = 25,
} hudi_bluetooth_ac6966cgx_extio_e;

typedef enum
{
    HUDI_BLUETOOTH_EXTIO_FUNC_INPUT = 0,
    HUDI_BLUETOOTH_EXTIO_FUNC_OUTPUT,
    HUDI_BLUETOOTH_EXTIO_FUNC_PWM,
} hudi_bluetooth_extio_func_e;

typedef enum
{
    HUDI_BLUETOOTH_AUDIO_CH_LINEIN = 0,
    HUDI_BLUETOOTH_AUDIO_CH_SPDIF,
} hudi_bluetooth_audio_ch_e;

typedef enum
{
    HUDI_BLUETOOTH_AUDIO_CTRL_PLAY = 0,
    HUDI_BLUETOOTH_AUDIO_CTRL_NEXT,
    HUDI_BLUETOOTH_AUDIO_CTRL_PREV,
    HUDI_BLUETOOTH_AUDIO_CTRL_STOP,
    HUDI_BLUETOOTH_AUDIO_CTRL_VOL_UP,
    HUDI_BLUETOOTH_AUDIO_CTRL_VOL_DWOM,
    HUDI_BLUETOOTH_AUDIO_CTRL_MUTE,
    HUDI_BLUETOOTH_AUDIO_CTRL_UNMUTE,
} hudi_bluetooth_audio_ctrl_e;

typedef struct
{
    char mac[6];
    char name[128];
} hudi_bluetooth_device_t;

typedef struct
{
    int pinpad;
    int value;
} hudi_bluetooth_extio_input_t;


typedef struct
{
    unsigned short type;
    unsigned short code;
    unsigned int value;
} hudi_bluetooth_input_event_t;


typedef int (*hudi_bluetooth_cb)(hudi_handle handle, hudi_bluetooth_event_e event, void *arg, void *user_data);
typedef int (*hudi_bluetooth_input_evt_cb)(hudi_bluetooth_input_event_t event);

/**
* @brief        Open hudi bluetooth instance
* @param[out]   handle  Output the handle of the instance
* @param[in]    module_id    Bluetooth module id
* @retval       0       Success
* @retval       other   Error
*/
int hudi_bluetooth_open(hudi_handle *handle);

/**
* @brief        Close hudi bluetooth instance
* @param[in]    handle  Handle of the instance
* @retval       0       Success
* @retval       other   Error
*/
int hudi_bluetooth_close(hudi_handle handle);

/**
* @brief       Bluetooth module ac6956cgx attach to hudi bluetooth layer
* @param[in]   handle   Handle of the instance
* @retval      0        Success
* @retval      other    Error
*/
int hudi_bluetooth_ac6956cgx_attach(hudi_handle handle);

/**
* @brief       Bluetooth module ac6955fgx attach to hudi bluetooth layer
* @param[in]   handle   Handle of the instance
* @retval      0        Success
* @retval      other    Error
*/
int hudi_bluetooth_ac6955fgx_attach(hudi_handle handle);

/**
* @brief       Register a callback function for bluetooth event
* @param[in]   handle  Handle of the instance
* @param[in]   func    Callback function
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_event_register(hudi_handle handle, hudi_bluetooth_cb func, void* user_data);

/**
* @brief       Register a callback function for bluetooth input device(such as: ADC_KEY/IR Remote)
* @param[in]   handle  Handle of the instance
* @param[in]   func    Callback function
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_input_event_register(hudi_bluetooth_input_evt_cb func);

/**
* @brief       Power on bluetooth rf and inquiry bluetooth device
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_rf_on(hudi_handle handle);

/**
* @brief       Power off bluetooth rf
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_rf_off(hudi_handle handle);

/**
* @brief       scan bluetooth device and return scan result in event callback
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_scan_async(hudi_handle handle);

/**
* @brief       abort scan bluetooth device
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_scan_abort(hudi_handle handle);

/**
* @brief       Connect bluetooth device
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_connect(hudi_handle handle, char *device_mac);

/**
* @brief       disconnect bluetooth device
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_disconnect(hudi_handle handle);

/**
* @brief       ignore connncted bluetooth device
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_ignore(hudi_handle handle);

/**
* @brief       Set bluetooth volume
* @param[in]   handle  Handle of the instance
* @param[in]   vol     Volume vaule
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_volume_set(hudi_handle handle, int vol);

/**
* @brief       Get bluetooth volume
* @param[in]   handle  Handle of the instance
* @param[out]  vol     Volume vaule
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_volume_get(hudi_handle handle, int *vol);

/**
* @brief       Set bluetooth audio input channel
* @param[in]   handle  Handle of the instance
* @param[in]   ch      Audio channel
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_audio_input_ch_set(hudi_handle handle, hudi_bluetooth_audio_ch_e ch);

/**
* @brief       Get bluetooth audio input channel
* @param[in]   handle  Handle of the instance
* @param[out]  ch      Audio channel
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_audio_input_ch_get(hudi_handle handle, hudi_bluetooth_audio_ch_e *ch);

/**
* @brief       Ctrl bluetooth audio output (mute/unmute......)
* @param[in]   handle  Handle of the instance
* @param[in]   ctrl    Audio ctrlcmd
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_audio_output_ctrl(hudi_handle handle, hudi_bluetooth_audio_ctrl_e ctrl);

/**
* @brief       Switch bluetooth mode (master mode/ slave mode)
* @param[in]   handle  Handle of the instance
* @param[in]   mode    Audio channel
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_mode_switch(hudi_handle handle, hudi_bluetooth_mode_e mode);

/**
* @brief       Get bluetooth mode
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_mode_get(hudi_handle handle, hudi_bluetooth_mode_e *mode);

/**
* @brief       Get bluetooth device status
* @param[in]   handle  Handle of the instance
* @param[out]  sta     Bluetooth status
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_status_get(hudi_handle handle, hudi_bluetooth_status_e *status);

/**
* @brief       Modified bluetooth name when setting bluetooth as soundbox(slave mode)
* @param[in]   handle  Handle of the instance
* @param[in]   name    Bluetooth name
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_localname_set(hudi_handle handle, char *name);

/**
* @brief       Get bluetooth name when setting bluetooth as soundbox(slave mode)
* @param[in]   handle  Handle of the instance
* @param[out]  name    Bluetooth name
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_localname_get(hudi_handle handle, char *name);

/**
* @brief       Get bluetooth firmware version
* @param[in]   handle  Handle of the instance
* @param[out]  version    Bluetooth firmware version
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_firmware_ver_get(hudi_handle handle, char *version);

/**
* @brief       Setup bluetooth extio pinpad function
* @param[in]   handle  Handle of the instance
* @param[in]   pinpad  Bluetooth extio pinpad
* @param[in]   func    Bluetooth extio function
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_extio_func_set(hudi_handle handle, int pinpad, hudi_bluetooth_extio_func_e func);

/**
* @brief       Get bluetooth extio input value when setup pinpad as input function
* @param[in]   handle  Handle of the instance
* @param[in]   pinpad  Bluetooth extio pinpad
* @param[out]  value   Bluetooth pinpad value
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_extio_input(hudi_handle handle, int pinpad, int *value);

/**
* @brief       Setup bluetooth extio output level when setup pinpad as output function
* @param[in]   handle  Handle of the instance
* @param[in]   pinpad  Bluetooth extio pinpad
* @param[in]   value   Bluetooth pinpad level
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_extio_output(hudi_handle handle, int pinpad, int value);


/**
* @brief       Setup bluetooth extio pwm freq and duty when setup pinpad as pwm function
* @param[in]   handle  Handle of the instance
* @param[in]   pinpad  Bluetooth extio pinpad
* @param[in]   freq    Pwm frequency (unit : khz Max 100Khz)
* @param[in]   duty    Pwm duty (uint : present Max 100 %)
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_extio_pwm_set(hudi_handle handle, int pinpad, int freq, int duty);

/**
* @brief       Set bluetooth extio level when bluetooth in standby
* @param[in]   handle  Handle of the instance
* @param[in]   pinset  bluetooth pinpad set
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_extio_standby_status_set(hudi_handle handle, int pinpad, int value);

/**
* @brief       Set bluetooth enter standby keycode
* @param[in]   handle  Handle of the instance
* @param[in]   standby_key  Standby keycode
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_standby_key_set(hudi_handle handle, int standby_key);

/**
* @brief       Bluetooth enter standby
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_standby_enter(hudi_handle handle);

/**
* @brief       Bluetooth reboot
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_reboot(hudi_handle handle);

/**
* @brief       Setup bluetooth ir usercode when bluetooth peripheral io as ir
* @param[in]   handle  Handle of the instance
* @param[in]   ir_usercode  Ir remote control's usercode
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_ir_usercode_set(hudi_handle handle, int ir_usercode);

/**
* @brief       Set bluetooth 2.4G signal channel bitmap
* @param[in]   handle  Handle of the instance
* @param[in]   bitmap  channel bitmap(80bit)
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_signal_channel_set(hudi_handle handle, char *bitmap);

/**
* @brief       Get bluetooth 2.4G signal channel bitmap
* @param[in]   handle  Handle of the instance
* @param[out]  bitmap  channel bitmap(80bit)
* @retval      0       Success
* @retval      other   Error
*/
int hudi_bluetooth_signal_channel_get(hudi_handle handle, char *bitmap);




#ifdef __cplusplus
}
#endif

#endif
