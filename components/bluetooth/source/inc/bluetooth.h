#ifndef _BLUETOOTH_H
#define _BLUETOOTH_H

#define BLUETOOTH_MAC_LEN 6
#define BLUETOOTH_TYPE_LEN 4
#define BLUETOOTH_NAME_LEN 128

#include "bluetooth_io.h"

struct bluetooth_slave_dev {
	unsigned char mac[BLUETOOTH_MAC_LEN];
	char name[BLUETOOTH_NAME_LEN];
	unsigned int type_value;
};

#define BT_RET_NOT_CONNECTED 1
#define BT_RET_SUCCESS 0
#define BT_RET_ERROR -1
#define BT_RET_TIMEOUT -2

#define BLUETOOTH_EVENT_SLAVE_DEV_SCANNED		        0	/* param = (struct bluetooth_slave_dev *) param */
#define BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED	    	1	/* param = 0 */
#define BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED		    2	/* param = 0 */
#define BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED		    	3	/* param = 0 */
#define BLUETOOTH_EVENT_SLAVE_DEV_GET_CONNECTED_INFO 	4	/* param = (struct bluetooth_slave_dev *) param */
#define BLUETOOTH_EVENT_SLAVE_DEV_CONNECTION_TIMED_OUT	5	/* param = 0 */
#define BLUETOOTH_EVENT_SLAVE_DEV_IS_RECONNECTING		6	/* param = (struct bluetooth_slave_dev *) param */
#define BLUETOOTH_EVENT_SLAVE_DEV_IS_SEARCHING			7	/* param = 0 */
#define BLUETOOTH_EVENT_REPORT_GPI_STAT					8	/* param = 0 */
#define BLUETOOTH_EVENT_SLAVE_DEV_GET_INIT_WORKING_STATE	9	/* param = 0 */
#define BLUETOOTH_EVENT_SLAVE_GET_VERSION				10	/* param = 0 */
#define BLUETOOTH_EVENT_SLAVE_RXMODE_CONNECT_RESULTS	11	/* param = 0 */
#define BLUETOOTH_EVENT_SLAVE_REPORT_LOCALNAME			12

struct input_event_bt {
	unsigned short type;
	unsigned short code;
	unsigned int value;
};

typedef enum _E_BT_DEVICE_STATUS_
{
	EBT_DEVICE_STATUS_NOWORKING_DEFAULT = 0,
	EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR,
	EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF,
	EBT_DEVICE_STATUS_NOWORKING_BT_RESET_OK,
	EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT,
	EBT_DEVICE_STATUS_CMD_ACK_OK,
	EBT_DEVICE_STATUS_WORKING_EXISTENT,
	EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED,
	EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING,
	EBT_DEVICE_STATUS_WORKING_GET_RECONNECTED_INFO,
	EBT_DEVICE_STATUS_WORKING_CONNECTION_TIMEOUT,
	EBT_DEVICE_STATUS_WORKING_DISCONNECTED,
	EBT_DEVICE_STATUS_WORKING_CONNECTED,
	EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO,
	EBT_DEVICE_STATUS_WORKING_ON_RX_MODE,
	EBT_DEVICE_STATUS_WORKING_ON_TX_MODE,
} bt_device_status_e;

typedef int (*bluetooth_callback_t)(unsigned long event, unsigned long param);
typedef int (*bluetooth_ir_control_t)(struct input_event_bt event);

int bluetooth_init(const char *uart_path, bluetooth_callback_t callback);
int bluetooth_deinit(void);
int bluetooth_poweron(void);
int bluetooth_poweroff(void);
int bluetooth_scan(void);
int bluetooth_stop_scan(void);
int bluetooth_connect(unsigned char *mac);
int bluetooth_is_connected(void);
int bluetooth_disconnect(void);
int bluetooth_set_music_vol(unsigned char val);
int bluetooth_memory_connection(unsigned char value);//0 memory connection 1 no memory connection
int bluetooth_set_gpio_backlight(unsigned char value);//0 disable 1 enable
int bluetooth_set_gpio_mutu(unsigned char value);//0 disable 1 enable
int bluetooth_set_cvbs_aux_mode(void);
int bluetooth_set_cvbs_fiber_mode(void);
int bluetooth_del_all_device(void);
int bluetooth_del_list_device(void);
int bluetooth_set_connection_cvbs_aux_mode(void);
int bluetooth_set_connection_cvbs_fiber_mode(void);
int bluetooth_ir_key_init(bluetooth_ir_control_t control);
int bluetooth_power_on_to_rx(void);
int bluetooth_factory_reset(void);
int bluetooth_ioctl(int cmd, ...);

    int bluetooth_channel_slect_support(unsigned char value);
int bluetooth_irc_ble_support(unsigned char value);
    int bluetooth_set_bt_mute_support(unsigned char value);
int bt_get_local_name(void);
    int bluetooth_set_volume_level(unsigned char value);
int ba_tx_support(void);
int ba_rx_support(void);

#endif
