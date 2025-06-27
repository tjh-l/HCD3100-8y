/*
 * @Description:
 * @Autor: Yanisin.chen
 * @Date: 2023-10-10 10:14:55
 */
#ifndef _BLUETOOTH_IO_H
#define _BLUETOOTH_IO_H

#include <stdint.h>
#include <asm/ioctls.h>
#define BLUETOOTH_IOCBASE 0x101

#define BLUETOOTH_SET_PINMUX				_IOW (BLUETOOTH_IOCBASE, 1, struct bt_pinmux_set)
#define BLUETOOTH_SET_GPIO_OUT				_IOW (BLUETOOTH_IOCBASE, 2, struct bt_gpio_set)
#define BLUETOOTH_SET_IR_POWERKEY			_IOW (BLUETOOTH_IOCBASE, 3,unsigned int)
#define BLUETOOTH_SET_CMD_ACK				_IO (BLUETOOTH_IOCBASE, 4)
#define BLUETOOTH_SET_IR_USERCODE			_IOW (BLUETOOTH_IOCBASE,5,unsigned int)
#define BLUETOOTH_SET_POWER_PINPAD_OFF		_IO (BLUETOOTH_IOCBASE, 6)
#define BLUETOOTH_SET_AUDIO_CHANNEL_INPUT	_IOW (BLUETOOTH_IOCBASE, 7,unsigned int)
#define BLUETOOTH_GET_AUDIO_CHANNEL_INPUT	_IOR (BLUETOOTH_IOCBASE, 8,unsigned int)
#define BLUETOOTH_SET_PWM_PARAM				_IOW (BLUETOOTH_IOCBASE, 9, struct bt_pwm_param)
#define BLUETOOTH_SET_CTRL_CMD				_IOW (BLUETOOTH_IOCBASE, 10,unsigned int)
#define BLUETOOTH_GET_MUSIC_VOL_RESULTS		_IOR (BLUETOOTH_IOCBASE, 11,unsigned int)
#define BLUETOOTH_SET_MUSIC_VOL_VALUE		_IOW (BLUETOOTH_IOCBASE, 12,unsigned int)
#define BLUETOOTH_SET_DEFAULT_CONFIG		_IO (BLUETOOTH_IOCBASE, 13)
#define BLUETOOTH_SET_RESET					_IO (BLUETOOTH_IOCBASE, 14)
#define BLUETOOTH_SET_RESET_BLOCK			_IO (BLUETOOTH_IOCBASE, 15)
#define BLUETOOTH_SET_POWERON				_IO (BLUETOOTH_IOCBASE, 16)
#define BLUETOOTH_SET_CLOSE_CHANNEL_MAP		_IOW (BLUETOOTH_IOCBASE, 17, struct bluetooth_channel_map)
#define BLUETOOTH_SET_OPEN_CHANNEL_MAP		_IOW (BLUETOOTH_IOCBASE, 18, struct bluetooth_channel_map)
#define BLUETOOTH_SET_SHIELDED_CHANNEL_MAP	_IO (BLUETOOTH_IOCBASE, 19)
#define BLUETOOTH_GET_VERSION				_IO (BLUETOOTH_IOCBASE, 20)
#define BLUETOOTH_GET_CHANNEL_MAP			_IO (BLUETOOTH_IOCBASE, 21)
#define BLUETOOTH_SET_BT_POWER_ON_TO_RX		_IO (BLUETOOTH_IOCBASE, 22)
#define BLUETOOTH_SET_BT_POWER_ON_TO_TX		_IO (BLUETOOTH_IOCBASE, 23)
#define BLUETOOTH_SET_LOCAL_NAME			_IOW (BLUETOOTH_IOCBASE, 24,unsigned int)
#define BLUETOOTH_GET_LOCAL_NAME			_IO (BLUETOOTH_IOCBASE,25)
#define BLUETOOTH_GET_DEVICE_STATUS			_IO (BLUETOOTH_IOCBASE,26)
#define BLUETOOTH_SET_CHANNEL_MAP			_IOW (BLUETOOTH_IOCBASE, 27,unsigned int)
#define BLUETOOTH_SET_DEFAULT_IO			_IOW (BLUETOOTH_IOCBASE, 28,unsigned int)

#define IRDEF_USERCODE 0x00FF
#define IRDEF_PWR_KEYCODE 0x1CE3  //rc remote code full data
#define LINEOUTDET_PINPAD PINPAD_BT_PB9

//only these pins can be config
typedef enum BT_PINPAD {
	PINPAD_BT_PB10 = 6,
	PINPAD_BT_PB9 = 7,
	PINPAD_BT_PB2 = 13,
	PINPAD_BT_PB0 = 15,
	PINPAD_BT_PC3 = 21,
	PINPAD_BT_PC2 = 22,
	PINPAD_BT_PA10 = 25,
} bt_pinpad_e;

/*only support this func */
typedef enum BT_PINMUX_PINSET {
	PINMUX_BT_GPIO_INPUT = 0,
	PINMUX_BT_GPIO_OUT,
    /* this enum has been abandoned and this enum has 
     * used new interface to setting this func  */
	PINMUX_BT_PWM,
	PINMUX_BT_ADC,/* not support yet*/
} bt_pinmux_pinset_e;

typedef enum BT_KEYTYPE{
	BT_IR_KEY = 0,
	BT_ADC_KEY,
	BT_GPIO_KEY,
} bt_keytype_e;

typedef struct bt_pinmux_set {
	bt_pinpad_e pinpad;
	bt_pinmux_pinset_e pinset;
} bt_pinmux_set_t;

/*only set gpio output */
typedef struct bt_gpio_set {
	bt_pinpad_e pinpad;
	uint8_t value;
} bt_gpio_set_t;

typedef struct bt_pwm_param {
	bt_pinpad_e pinpad;
	uint8_t type; //0:frequency  1: duty cycle
	uint8_t value;
} bt_pwm_param_t;

struct bluetooth_channel_map
{
	char x;
	char y;
};

typedef enum
{
    CMD_VALUE_PLAYORPAUSE = 0,
    CMD_VALUE_PREV = 1,
    CMD_VALUE_NEXT = 2,
    CMD_VALUE_STOP = 3,
    /*use for bt_rx_mode*/
    CMD_VALUE_VOL_UP = 4,
    CMD_VALUE_VOL_DOWN = 5,
    CMD_VALUE_MUTE = 6,
    CMD_VALUE_UNMUTE = 7,
    /*use for bt_rx or tx mode*/
} bt_ctrl_cmd_value_e;



#endif
