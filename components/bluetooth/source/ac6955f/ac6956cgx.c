#ifdef BLUETOOTH_AC6956C_GX
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <hcuapi/sci.h>
#include <time.h>
#include <signal.h>
#include <hcuapi/input-event-codes.h>
#include <semaphore.h>
#include <stdarg.h>
#include <poll.h>
#include "bluetooth.h"
#include "bluetooth_io.h"
#ifdef HC_RTOS
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/module.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <kernel/completion.h>
#include <kernel/delay.h>
#else
#include <termios.h> 
#endif

//#define BLUETOOTH_DEBUG
#ifdef BLUETOOTH_DEBUG
#define BLUETOOTH_LOG(format, ...) printf(format, ##__VA_ARGS__)
#else
#define BLUETOOTH_LOG(format, ...)
#endif
// bt io cmd 
typedef enum _E_HC_BT_CMD
{
    CMD_SET_BT_CMD_ACK = 0x00,
    CMD_SET_BT_POWER_ON = 0x02,
    CMD_SET_BT_POWER_OFF = 0x03,
    CMD_REPORT_CONNECT_RESULTS = 0x04,
    CMD_REPORT_DISCONNECT_RESULTS = 0x05,
    CMD_SET_INQUIRY_START = 0x06,
    CMD_SET_INQUIRY_STOP = 0x07,
    CMD_SET_CONNECT = 0x08,
    CMD_SET_DISCONNECT = 0x09,
    CMD_REPORT_CONNECT_STATUS = 0x0a,
    CMD_REPORT_INQUIRY_COMPLETE = 0x0b,
    CMD_REPORT_INQUIRY_RESULTS = 0x0c,
    CMD_SET_BT_MUSIC_VOL_VALUE = 0x0e,
    CMD_SET_AUDIO_CHANNEL_INPUT_SELECT = 0x11,
    CMD_GET_AUDIO_CHANNEL_INPUT_SELECT = 0x12,
    CMD_REPORT_AUDIO_CHANNEL_INPUT_SELECT = 0x13,
    CMD_SET_DELETE_LAST_DEVICE = 0x20,
    CMD_SET_DELETE_ALL_DEVICE = 0x21,
    CMD_SET_MEMORY_CONNECTION = 0x23, //not support
    CMD_SET_BT_POWER_ON_TO_RX =0x40,
    CMD_SET_BT_POWER_ON_TO_TX =0x41,
    CMD_GET_BT_RX_OR_TX_MODE =0x42,
    CMD_REPORT_BT_RX_OR_TX_MODE =0x43,
    CMD_GET_BT_CONNECT_STATE = 0x44,
    CMD_SET_BT_CTRL_CMD = 0x4a,
    CMD_GET_BT_MUSIC_VOL_RESULTS = 0x4b,
    CMD_REPORT_BT_MUSIC_VOL_RESULTS = 0x4c,
    CMD_SET_BT_LOCAL_NAME = 0xa1,
    CMD_GET_BT_LOCAL_NAME = 0xa2,
    CMD_REPORT_BT_LOCAL_NAME = 0xa3,
    CMD_GET_BT_VERSION = 0xa4,
    CMD_REPORT_BT_VERSION = 0xa5,
    CMD_SET_BT_RESET = 0xa7,
    CMD_GET_CHANNEL_MAP = 0xa8,
    // HC DEFINE CMD
    CMD_SET_BT_PIN_FUNCTION = 0xe0,
    CMD_SET_BT_GPIO_POL = 0xe1,
    CMD_REPORT_BT_GPI_STAT = 0xe2,
    CMD_SET_BT_IR_PROTOCAL = 0xe3, //unspport
    CMD_REPORT_BT_IR_KEY = 0xe4,
    CMD_REPORT_BT_ADC_KEY = 0xe5,
    // 0xe6??
    CMD_SET_BT_IR_POWER_KEY = 0xe7,
    CMD_SET_BT_FM = 0xe8,
    CMD_SET_IR_USERCODE = 0xe9,
    CMD_SET_BT_POWER_PINPAD_OFF = 0xea,
    CMD_SET_BT_PWM_PARAM = 0xeb,
    CMD_SET_BT_DEFLAULT_IO = 0xec,
    CMD_SET_CHANNEL_MAP = 0xfa,
    CMD_REPORT_CHANNEL_MAP = 0xfb,
} bt_cmds_e;


typedef struct
{
    uint32_t report_code;
    uint16_t keycode;
} bt_keymap_table_t;



// bt adckey report keycode map
static bt_keymap_table_t adc_keymap_table[] =
{
    {0x00, KEY_POWER},
    {0X01, KEY_LEFT},
    {0x02, KEY_RIGHT},
    {0X03, KEY_EXIT},
    {0x04, KEY_DOWN},
    {0X05, KEY_UP},
    {0X06, KEY_OK},
};

// bt irkey report keycode map
static bt_keymap_table_t ir_keymap_table[] =
{

    /*  0x1c   0x3   0x42  0x8  *
     * POWER  AUDIO  SAT   MUTE *
     *                          */
    { 0x1c, KEY_POWER },
    { 0x03, KEY_AUDIO },
    { 0x42, KEY_SAT   },
    { 0x08, KEY_MUTE  },

    /*  0x55    0x51       0x5e *
     * ZOOM    TIMESHIFT   SUB  *
     *                          */
    { 0x55, KEY_ZOOM     },
    { 0x51, KEY_TIME     },
    { 0x5e, KEY_SUBTITLE },

    /*  0x5a    0x52     0x5d *
     * TV/RADIO TTX   FILELIST  *
     *                          */
    { 0x5a, KEY_TV   },
    { 0x52, KEY_TEXT },
    { 0x5d, KEY_LIST },

    /*  0x18           0x17  *
     * MENU           EXIT   *
     *                       */
    { 0x18, KEY_MENU },
    { 0x17, KEY_EXIT },

    /*          0x1a          *
     *           Up           *
     *                        *
     *  0x47    0x06    0x07  *
     *  Left     Ok     Right *
     *                        *
     *         0x48           *
     *         Down           *
     *                        */
    { 0x47, KEY_LEFT  },
    { 0x1a, KEY_UP    },
    { 0x07, KEY_RIGHT },
    { 0x48, KEY_DOWN  },
    { 0x06, KEY_OK },

    /*  0x49           0xa  *
     * EPG           INFO   *
     *                      */
    { 0x49, KEY_EPG  },
    { 0x0a, KEY_INFO },

    /*  0x54    0x16    0x15  *
     *   1       2       3    *
     *                        *
     *  0x50    0x12    0x11  *
     *   4       5       6    *
     *                        *
     *  0x4c    0xe    0xd    *
     *   7       8       9    *
     *                        */
    { 0x54, KEY_NUMERIC_1 },
    { 0x16, KEY_NUMERIC_2 },
    { 0x15, KEY_NUMERIC_3 },
    { 0x50, KEY_NUMERIC_4 },
    { 0x12, KEY_NUMERIC_5 },
    { 0x11, KEY_NUMERIC_6 },
    { 0x4c, KEY_NUMERIC_7 },
    { 0x0e, KEY_NUMERIC_8 },
    { 0x0d, KEY_NUMERIC_9 },

    /*  0x10    0x41    0xc  *
     * RECALL    FAV      0  *
     *                       */
    { 0x10, KEY_AGAIN     },
    { 0x41, KEY_FAVORITES },
    { 0x0c, KEY_NUMERIC_0 },


    /*  0x09       0x05        0x4b   0x4f *
     *  LEFTSHIFT RIGHTSHIFT PREVIOUS NEXT *
     *                                     */
    { 0x09, KEY_LEFTSHIFT  },
    { 0x05, KEY_RIGHTSHIFT },
    { 0x4b, KEY_PREVIOUS   },
    { 0x4f, KEY_NEXT       },

    /*  0x01  0x5f  0x19  0x58 *
     *  PLAY PAUSE STOP RECORD *
     *                         */
    { 0x01, KEY_PLAY   },
    { 0x5f, KEY_PAUSE  },
    { 0x19, KEY_STOP   },
    { 0x58, KEY_RECORD },

    /*  0x56  0x57  0x1f  0x5b *
     *  RED  GREEN YELLO BLUE  *
     *                         */
    { 0x56, KEY_RED    },
    { 0x57, KEY_GREEN  },
    { 0x1f, KEY_YELLOW },
    { 0x5b, KEY_BLUE   },

    /*  0x14              0x13      *
     *  KEY_VOLUMEUP KEY_VOLUMEDOWN *
     *                              */
    { 0x14, KEY_VOLUMEUP    },
    { 0x13, KEY_VOLUMEDOWN  },
};

static bluetooth_ir_control_t bt_control;
int bluetooth_ir_key_init(bluetooth_ir_control_t control)
{
    if (control == NULL)
        return BT_RET_ERROR;

    bt_control = control;
    return BT_RET_SUCCESS;
}

/*
function: Bluetooth sending infrared button
examples: bluetooth_ir_key_send(KEY_DOWN);
*/
int bluetooth_ir_key_send(unsigned short code)
{
    struct input_event_bt event_key = {0};
    if (bt_control == NULL)
        return BT_RET_ERROR;
    event_key.type = EV_KEY;
    event_key.value = 1;
    event_key.code = code;
    bt_control(event_key);
    return BT_RET_SUCCESS;
}

int bluetooth_input_event_send(struct input_event_bt event_key)
{
    if (bt_control == NULL)
        return BT_RET_ERROR;
    event_key.type = EV_KEY;
    bt_control(event_key);
    return BT_RET_SUCCESS;
}




int bluetooth_set_cvbs_aux_mode(void)
{
    return 0;
}
int bluetooth_set_cvbs_fiber_mode(void)
{
    return 0;
}

int bluetooth_set_music_vol(unsigned char val)
{
    bluetooth_ioctl(BLUETOOTH_SET_MUSIC_VOL_VALUE,val);
    return 0;
}

int bluetooth_set_connection_cvbs_aux_mode(void)
{
    return 0;
}

int bluetooth_set_connection_cvbs_fiber_mode(void)
{
    return 0;
}

int bluetooth_power_on_to_rx(void)
{
    return 0;
}
int bluetooth_factory_reset(void)
{
    return 0;
}

#define BT_RET_EXIT     1
#define BT_DATA_AGAIN   2
#define BT_ERROR_DATA       0x100
#define BT_ERROR_DATA_OFFSET_1  0x101
#define BT_ERROR_DATA_OFFSET_2  0x102
#define BT_ERROR_DATA_MAX       0x103
#define UART_RX_RECEIVE_MAX_BUFF 1024
#define UART_TX_WRITE_MAX_BUFF 256

#define AC6956C_MAX_WRITE_SIZE          128
#define BLUETOOTH_DATE_TIMEOUT 1000
#define BLUETOOTH_MINIMUM_CHECKSUM 7
#define BLUETOOTH_MAX_VOLUME_RANGE 31
#define BLUETOOTH_MAX_LOCALNAME_SIZE 32
#define BLUETOOTH_ADCKEY_MAX_NUM 16

struct AC6956C_MessageBody
{
    unsigned short  frame_head;
    unsigned short  frame_len;
    unsigned char   frame_id;
    unsigned char   cmd_id;
    unsigned char   cmd_len;
    unsigned char   cmd_value[AC6956C_MAX_WRITE_SIZE];
    unsigned short  Frame_checksum;
};

typedef struct
{
    unsigned int voltage_min;
    unsigned int voltage_max;
    unsigned int keycode;
}adckey_map_t;

typedef struct {
    unsigned int ir_usercode;
    unsigned int ir_powerkey_code;
    unsigned int adckey_num;
    adckey_map_t adckey_array[BLUETOOTH_ADCKEY_MAX_NUM];
    unsigned int pinpad_lineout_det;
    unsigned int pinset_lineout_det;
    unsigned int pinpad_lcd_backlight;
    unsigned int pinset_lcd_backlight;
    unsigned int wifien_gpios;
    unsigned int wifien_gpios_value;
}ac6956c_configure_t;

struct bt_ac6956c_priv
{
    int uartfd;
    int cref;
    bluetooth_callback_t callback;
    struct bluetooth_slave_dev inquiry_info;
    struct bluetooth_slave_dev connet_info;
    bt_device_status_e dev_status;
    bt_cmds_e bt_cmds;
    bt_gpio_set_t bt_pinpad_level;
    uint8_t dac_volume;
    uint8_t dac_input_channel;  // 0: LINEIN  1: SPDIF
    char device_name[BLUETOOTH_MAX_LOCALNAME_SIZE];
    ac6956c_configure_t dts_info;
    char firmware_version[32];
    uint8_t signal_channelmap[10];
};


static void bt_ac6956c_cp_connet_info(struct bluetooth_slave_dev *connet_cmds, struct AC6956C_MessageBody *obj);
static int bt_ac6956c_messagebody_switch_str(struct AC6956C_MessageBody *body, unsigned char *buf);
#ifdef HC_RTOS
static void bt_ac6956c_read_thread(void *args);
#else
static void* bt_ac6956c_read_thread(void *args);
#endif 
static void bt_ac6956c_set_poll_timeout(int cmds, int *poll_t);
static void bt_ac6956c_serial_data_judg(bt_cmds_e cmd, char *buf, unsigned int count);
static int bt_ac6956c_set_action(bt_cmds_e cmd, unsigned char *send_buf);
static int str_switch_bt_ac6956c_messagebody(char *buf, unsigned int *count, struct AC6956C_MessageBody *body);
static void printf_bt_ac6956c_dev_status(bt_device_status_e status);
static bt_device_status_e bt_get_device_sys_status(void);
static int volatile bt_ac6956c_task_start_flag = 0;
static struct bt_ac6956c_priv *gbt = NULL;
static int bt_get_device_sys_connet_info(struct bluetooth_slave_dev *con_data);
static int str_with_ac6956c_messagebody_compare(char *buf, unsigned char count);
static uint16_t bt_reportkey_mapping(bt_keytype_e keytype, uint32_t btcode);
#ifdef HC_RTOS
static struct completion bluetooth_cmds_sem;
static struct completion bluetooth_rectask_sem;
#else
static pthread_t bluetooth_thread_id;
static sem_t bluetooth_cmds_sem;
#endif

#ifdef HC_LINUX
static int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop);
#endif
static int bluetooth_get_dts_uint32(const char *path);
static int bluetooth_get_dts_uint32_by_index(const char *path,int index);
static void bluetooth_get_dts_string(const char *path, char *string, int size);
static void bluetooth_get_dts_info(void);
static int bluetooth_set_close_channel_map(struct bluetooth_channel_map *map);
static int bluetooth_set_open_channel_map(struct bluetooth_channel_map *map);
static int bluetooth_sem_timewait(int ms);
static int bluetooth_sem_post(void);
static uint16_t bluetooth_adckey_voltage_mapping(uint16_t value);

int bluetooth_init(const char *uart_path, bluetooth_callback_t callback)
{
    int fd;
    int ret = BT_RET_SUCCESS;
    if (gbt == NULL)
    {
        gbt = (struct bt_ac6956c_priv *)malloc(sizeof(struct bt_ac6956c_priv));
        if (gbt == NULL)goto error;
        memset(gbt, 0, sizeof(struct bt_ac6956c_priv));
    }
    if (gbt->cref == 0)
    {
        /* init bt */
        if (uart_path == NULL)
        {
            goto null_error;
        }

        fd = open(uart_path, O_RDWR);
        if (fd < 0)
        {
            BLUETOOTH_LOG("open %s error %d\n",uart_path,fd);
            goto uart_error;
        }
        gbt->uartfd = fd;
        bt_ac6956c_task_start_flag = 1;
#ifdef HC_RTOS
        struct sci_setting bt_ad6956f_sci;
        bt_ad6956f_sci.parity_mode = PARITY_NONE;
        bt_ad6956f_sci.bits_mode = bits_mode_default;
        ioctl(fd, SCIIOC_SET_BAUD_RATE_115200, NULL);
        ioctl(fd, SCIIOC_SET_SETTING, &bt_ad6956f_sci);
        init_completion(&bluetooth_cmds_sem);
        init_completion(&bluetooth_rectask_sem);
        ret = xTaskCreate(bt_ac6956c_read_thread, "bt_ac6955fgx_read_thread",
                          0x1000, NULL, portPRI_TASK_NORMAL, NULL);


#else
        set_opt(fd,115200,8,'N',1);
        sem_init(&bluetooth_cmds_sem, 0, 0);
        /* create task */
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x1000);
        if (pthread_create(&bluetooth_thread_id, &attr, bt_ac6956c_read_thread, NULL))
        {
            printf("pthread_create receive_event_func fail\n");
            goto taskcreate_error;
        }
        pthread_attr_destroy(&attr);
        /* init success */
#endif

        gbt->callback = callback;
        gbt->cref++;
    } else {
        gbt->cref++;
    }
    /* report bt_software version in bt_recive pthread*/
    bluetooth_get_dts_info();
    bluetooth_ioctl(BLUETOOTH_GET_VERSION,NULL);
    return BT_RET_SUCCESS;

taskcreate_error:
    bt_ac6956c_task_start_flag=0;
    close(fd);
uart_error:
    gbt->uartfd = -1;
null_error:
    free(gbt);
    gbt = NULL;
error:
    return BT_RET_ERROR;
}

int bluetooth_deinit(void)
{
    if (gbt == NULL)
        return BT_RET_ERROR;

    if (gbt->cref == 0)
        return BT_RET_ERROR;

    gbt->cref--;
    if (gbt->cref == 0)
    {
        // bluetooth_poweroff();
        /* delete task */
        bt_ac6956c_task_start_flag=0;
        #ifndef HC_RTOS
        if(bluetooth_thread_id){
            pthread_join(bluetooth_thread_id, NULL);
            bluetooth_thread_id = 0;
        }
        #else
        /* wait for read_task run to the end and recovery of resource */
        wait_for_completion_timeout(&bluetooth_rectask_sem,3000);
        #endif 
        /* deinit bt */
        close(gbt->uartfd);
        free(gbt);
        gbt = NULL;
    }

    return BT_RET_SUCCESS;
}

int bluetooth_poweron(void)
{
    if(bt_ac6956c_set_action(CMD_SET_BT_POWER_ON,0)==0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_poweroff(void)
{
    if (bt_ac6956c_set_action(CMD_SET_BT_POWER_OFF, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_scan(void)
{
    if (bt_ac6956c_set_action(CMD_SET_INQUIRY_START, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_stop_scan(void)
{
    if (bt_ac6956c_set_action(CMD_SET_INQUIRY_STOP, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_connect(unsigned char *mac)
{
    if (bt_ac6956c_set_action(CMD_SET_CONNECT, mac) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_get_connect_info(unsigned char *mac)
{
    int count = 0;
    if (bt_ac6956c_set_action(CMD_SET_CONNECT, mac) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_is_connected(void)
{
    bt_device_status_e read_state = 0;
    int count = 0;
    if (bt_ac6956c_set_action(CMD_GET_BT_CONNECT_STATE, 0) == 0)
    {
        while (1)
        {
            count++;
            read_state = bt_get_device_sys_status();
            if (read_state > EBT_DEVICE_STATUS_NOWORKING_DEFAULT)
            {
                if (read_state == EBT_DEVICE_STATUS_WORKING_CONNECTED)
                {
                    BLUETOOTH_LOG("Device exists\n");
                }
                else if (read_state == EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO)
                {
                    BLUETOOTH_LOG("Device exist\n");
                }
                else if (read_state == EBT_DEVICE_STATUS_WORKING_DISCONNECTED)
                {
                    BLUETOOTH_LOG("Device does not exist\n");
                    return BT_RET_ERROR;
                }
                break;
            }
            if (count > 200)return BT_RET_ERROR;
            usleep(20 * 1000);
        }
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_disconnect(void)
{
    if (bt_ac6956c_set_action(CMD_SET_DISCONNECT, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_del_all_device(void)
{
    if (bt_ac6956c_set_action(CMD_SET_DELETE_ALL_DEVICE, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_del_list_device(void)
{
    if (bt_ac6956c_set_action(CMD_SET_DELETE_LAST_DEVICE, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_memory_connection(unsigned char value)
{
    if (bt_ac6956c_set_action(CMD_SET_MEMORY_CONNECTION, &value) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int _bluetooth_ioctl_(int cmd, unsigned long arg)
{
    int ret = 0;
    if(!gbt)
    {
        BLUETOOTH_LOG("Bluetooth init error \n");
        return BT_RET_ERROR;
    }
    switch (cmd)
    {
        case BLUETOOTH_SET_PINMUX:
        {
            bt_pinmux_set_t* bt_pinmux = (bt_pinmux_set_t*)arg;
            char send_buf[2] = {0};
            //pinpad setting cmd_value size is 2Byte
            send_buf[0] = (char)bt_pinmux->pinpad;
            send_buf[1] = (char)bt_pinmux->pinset;
            if (bt_ac6956c_set_action(CMD_SET_BT_PIN_FUNCTION, send_buf) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_SET_GPIO_OUT:
        {
            bt_gpio_set_t* bt_gpioset = (bt_gpio_set_t*)arg;
            char send_buf[2] = {0};
            //pinpad setting cmd_value size is 2Byte
            send_buf[0] = (char)bt_gpioset->pinpad;
            send_buf[1] = (char)bt_gpioset->value;
            if (bt_ac6956c_set_action(CMD_SET_BT_GPIO_POL, send_buf) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_SET_IR_POWERKEY:
        {
            uint16_t keycode = (uint16_t)arg;
            char send_buf[2] = {0};
            send_buf[1] = keycode & 0xff;
            send_buf[0] = (keycode >> 8) & 0xff;
            if (bt_ac6956c_set_action(CMD_SET_BT_IR_POWER_KEY, send_buf) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_SET_CMD_ACK:
        {
            if (bt_ac6956c_set_action(CMD_SET_BT_CMD_ACK, NULL) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else
                ret = BT_RET_ERROR;
            break;
        }

        case BLUETOOTH_SET_IR_USERCODE:
        {
            uint16_t value = (uint16_t)arg;
            char send_buf[2] = {0};
            send_buf[0] = (value >> 8) & 0xff;
            send_buf[1] = value & 0xff;
            if (bt_ac6956c_set_action(CMD_SET_IR_USERCODE, send_buf) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_SET_POWER_PINPAD_OFF:
        {
            if (bt_ac6956c_set_action(CMD_SET_BT_POWER_PINPAD_OFF, NULL) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_SET_AUDIO_CHANNEL_INPUT:
        {
            uint8_t channel_val = (uint8_t)arg;
            if (channel_val > 0x01)
                ret = BT_RET_ERROR;
            char send_buf[1] = {0};
            send_buf[0] = channel_val;
            if (bt_ac6956c_set_action(CMD_SET_AUDIO_CHANNEL_INPUT_SELECT, send_buf) == 0)
            {
                gbt->dac_input_channel = channel_val;
                ret = BT_RET_SUCCESS;
            }
            else
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_GET_AUDIO_CHANNEL_INPUT:
        {
            if (bt_ac6956c_set_action(CMD_GET_AUDIO_CHANNEL_INPUT_SELECT, NULL) == 0)
            {
                uint8_t* channel_val = (uint8_t*)arg;
                bluetooth_sem_timewait(200);
                *channel_val = gbt->dac_input_channel;
            }
            else
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_SET_PWM_PARAM:
        {
            char send_buf[3] = {0};
            bt_pwm_param_t* pwm_param = (bt_pwm_param_t*)arg;
            send_buf[0] = (char)pwm_param->pinpad;
            send_buf[1] = (char)pwm_param->type;
            send_buf[2] = (char)pwm_param->value;
            if (bt_ac6956c_set_action(CMD_SET_BT_PWM_PARAM, send_buf) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_SET_CTRL_CMD:
        {
            char send_buf[1] = {0};
            uint8_t val = (uint8_t)arg;
            send_buf[0] = (char )val;
            bt_ac6956c_set_action(CMD_SET_BT_CTRL_CMD, send_buf);
            break;
        }
        case BLUETOOTH_GET_MUSIC_VOL_RESULTS:
        {
            if (bt_ac6956c_set_action(CMD_GET_BT_MUSIC_VOL_RESULTS, NULL) == BT_RET_SUCCESS){
                bluetooth_sem_timewait(200);
                uint8_t* vol = (uint8_t *)arg;
                *vol = gbt->dac_volume;
            }else{
                ret = BT_RET_ERROR;
            }
            break;
        }
        case BLUETOOTH_SET_MUSIC_VOL_VALUE:
        {
            uint8_t val = (uint8_t)arg;
            char send_buf[1] = {0};
            val = val / 3 > BLUETOOTH_MAX_VOLUME_RANGE ? BLUETOOTH_MAX_VOLUME_RANGE : val / 3;
            send_buf[0] = val;
            bt_ac6956c_set_action(CMD_SET_BT_MUSIC_VOL_VALUE, send_buf);
            break;
        }
        case BLUETOOTH_SET_DEFAULT_CONFIG:
        { 
            uint16_t value = gbt->dts_info.ir_usercode;
            char send_buf[2] = {0};
            send_buf[0] = (value >> 8) & 0xff;
            send_buf[1] = value & 0xff;
            bt_ac6956c_set_action(CMD_SET_IR_USERCODE, send_buf);

            uint16_t keycode = gbt->dts_info.ir_powerkey_code;
            memset(send_buf, 0, 2);
            send_buf[1] = keycode & 0xff;
            send_buf[0] = (keycode >> 8) & 0xff;
            bt_ac6956c_set_action(CMD_SET_BT_IR_POWER_KEY, send_buf);

            bt_pinmux_set_t lineout_det = {0};
            lineout_det.pinpad = gbt->dts_info.pinpad_lineout_det;
            lineout_det.pinset = gbt->dts_info.pinset_lineout_det;
            //pinpad setting cmd_value size is 2Byte
            memset(send_buf, 0, 2);
            send_buf[0] = (char)lineout_det.pinpad;
            send_buf[1] = (char)lineout_det.pinset;
            bt_ac6956c_set_action(CMD_SET_BT_PIN_FUNCTION, send_buf);

            /*wifi_en pinpad*/
            bt_gpio_set_t wifi_en = {0};
            wifi_en.pinpad = gbt->dts_info.wifien_gpios;
            wifi_en.value = gbt->dts_info.wifien_gpios_value;
            memset(send_buf, 0, 2);
            send_buf[0] = (char)wifi_en.pinpad;
            send_buf[1] = (char)wifi_en.value;
            bt_ac6956c_set_action(CMD_SET_BT_GPIO_POL, send_buf);

            memset(send_buf, 0, 2);
            send_buf[0] = 0x01;
            bt_ac6956c_set_action(CMD_SET_AUDIO_CHANNEL_INPUT_SELECT, send_buf);
            /*bt dac will mute to avoid boom sound*/
            memset(send_buf, 0, 2);
            send_buf[0] = CMD_VALUE_UNMUTE;
            bt_ac6956c_set_action(CMD_SET_BT_CTRL_CMD, send_buf);

            break;
        }
        case BLUETOOTH_SET_BT_POWER_ON_TO_RX: 
        {
            if (bt_ac6956c_set_action(CMD_SET_BT_POWER_ON_TO_RX, NULL) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_SET_BT_POWER_ON_TO_TX:
        {
            if (bt_ac6956c_set_action(CMD_SET_BT_POWER_ON_TO_TX, NULL) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else
                ret = BT_RET_ERROR;            
            break;
        }
        case BLUETOOTH_SET_LOCAL_NAME: 
        {
            char* local_name = (char*)arg;
            char send_buf[BLUETOOTH_MAX_LOCALNAME_SIZE] = {0};
            int str_length = strlen(local_name);
            if(str_length+1 > BLUETOOTH_MAX_LOCALNAME_SIZE)
                break; 
            memcpy(send_buf,local_name,str_length);
            bt_ac6956c_set_action(CMD_SET_BT_LOCAL_NAME,send_buf);
            break;
        }
        case BLUETOOTH_GET_LOCAL_NAME:
        {
            if(bt_ac6956c_set_action(CMD_GET_BT_LOCAL_NAME,NULL) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else 
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_GET_VERSION: 
        {
            if(bt_ac6956c_set_action(CMD_GET_BT_VERSION,NULL) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else 
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_GET_DEVICE_STATUS: 
        {
            int* device_status = (int*)arg;
            *device_status = gbt->dev_status;
            break;
        }
        case BLUETOOTH_SET_CLOSE_CHANNEL_MAP:
        {
            struct bluetooth_channel_map *map = (struct bluetooth_channel_map*)arg;
            bluetooth_set_close_channel_map(map);
            break;
        }
        case BLUETOOTH_SET_OPEN_CHANNEL_MAP:
        {
            struct bluetooth_channel_map *map = (struct bluetooth_channel_map*)arg;
            bluetooth_set_open_channel_map(map);
            break;
        }
        case BLUETOOTH_GET_CHANNEL_MAP : 
        {
            if(bt_ac6956c_set_action(CMD_GET_CHANNEL_MAP,NULL) == 0)
            {
                uint8_t* signal_channelmap = (uint8_t*)arg;
                bluetooth_sem_timewait(200);
                memcpy(signal_channelmap, gbt->signal_channelmap,sizeof(gbt->signal_channelmap));

            }
            else 
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_SET_RESET:
        {
            if(bt_ac6956c_set_action(CMD_SET_BT_RESET,NULL) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else 
                ret = BT_RET_ERROR;
            break;
        }
        case BLUETOOTH_SET_CHANNEL_MAP:
        {
            if(bt_ac6956c_set_action(CMD_SET_CHANNEL_MAP,(uint8_t*)arg) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else 
                ret = BT_RET_ERROR;
            break;

        }
        case BLUETOOTH_SET_DEFAULT_IO:
        {
            bt_gpio_set_t* gpio_default = (bt_gpio_set_t*)arg;
            char send_buf[2] = {0};
            send_buf[0] = gpio_default->pinpad;
            send_buf[1] = gpio_default->value;
            if(bt_ac6956c_set_action(CMD_SET_BT_DEFLAULT_IO,send_buf) == 0)
            {
                ret = BT_RET_SUCCESS;
            }
            else 
                ret = BT_RET_ERROR;
            break;
        }
        default:
            break;
    }
    return ret;
}

int bluetooth_ioctl(int cmd, ...)
{
    int ret = 0;
    va_list ap;
    va_start(ap, cmd);
    unsigned long arg = va_arg(ap, unsigned long);
    ret = _bluetooth_ioctl_(cmd, arg);
    va_end(ap);
    return ret;
}

int bluetooth_set_gpio_mutu(unsigned char value)
{
    // set gpiox output value
    bt_gpio_set_t mute_level = {0};
    mute_level.pinpad = PINPAD_BT_PC3;
    mute_level.value = value;
    bluetooth_ioctl(BLUETOOTH_SET_GPIO_OUT, &mute_level);
    return 0;
}

#ifdef HC_RTOS
static void bt_ac6956c_read_thread(void *args)
#else
static void* bt_ac6956c_read_thread(void *args)
#endif
{
    char *rx_buf = (char *)malloc(1 * UART_RX_RECEIVE_MAX_BUFF);
    char byte = 0;
    struct pollfd fds[1];
    nfds_t nfds = 1;
    static unsigned char date_off_set = 0;
    static int count = 0;
    int ret = BT_RET_SUCCESS;
    int poll_time = 100;

    fds[0].fd = gbt->uartfd;
    fds[0].events  = POLLIN | POLLRDNORM;
    fds[0].revents = 0;
    while (bt_ac6956c_task_start_flag)
    {
        if (gbt->uartfd < 0)
            break;

        ret = poll(fds, nfds, 0);   //poll bt uart
        if (ret > 0)
        {
            if (fds[0].revents & (POLLRDNORM | POLLIN))
            {
                if (read(gbt->uartfd, &byte, 1))
                {
                    rx_buf[count++] = byte;
                    if (count == UART_RX_RECEIVE_MAX_BUFF) //resc 1k data
                    {
                        date_off_set = 0;
                        count = 0;
                    }
                }
            }
        }
        /*Serial port returns data for judgment*/
        if (count >= BLUETOOTH_MINIMUM_CHECKSUM) //rx buf data must > 7B
        {
            ret = str_with_ac6956c_messagebody_compare(&rx_buf[date_off_set], count - date_off_set);
            if (ret == BT_RET_SUCCESS)
            {
                bt_ac6956c_serial_data_judg(gbt->bt_cmds,&rx_buf[date_off_set],count - date_off_set);
                memset(rx_buf, 0, count);
                count = 0;
                date_off_set = 0;
            }
            else if (ret == BT_ERROR_DATA_OFFSET_1 || ret == BT_ERROR_DATA_OFFSET_2)
            {
                date_off_set += ret - BT_ERROR_DATA;
                if (date_off_set > 10)
                {
                    BLUETOOTH_LOG("date_off_set too large\n");
                    memset(rx_buf, 0, count);
                    date_off_set = 0;
                    count = 0;
                }
            }
            else if (ret == BT_RET_ERROR)
            {
                BLUETOOTH_LOG("rx buff error\n");
                memset(rx_buf, 0, count);
                date_off_set = 0;
                count = 0;
            }
        }
        usleep(1000);
    }
    free(rx_buf);
    usleep(1000);
#ifdef HC_RTOS
    complete(&bluetooth_rectask_sem);
    vTaskDelete(NULL);
#endif
    return NULL;
}


static bt_device_status_e bt_get_device_sys_status(void)
{
    if (gbt == NULL)
        return EBT_DEVICE_STATUS_NOWORKING_DEFAULT;
    return gbt->dev_status;
}

static int bt_get_device_sys_connet_info(struct bluetooth_slave_dev *con_data)
{
    bt_device_status_e sys_status = bt_get_device_sys_status();
    if (gbt == NULL || con_data == NULL) return BT_RET_ERROR;
    if (sys_status == EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO)
    {
        memcpy(con_data, &gbt->connet_info, sizeof(struct bluetooth_slave_dev));
        return BT_RET_SUCCESS;
    }
    else if (sys_status < EBT_DEVICE_STATUS_WORKING_CONNECTED)
        return 1;

    return BT_RET_ERROR;
}

static int bt_ac6956c_set_action(bt_cmds_e cmd, unsigned char *send_buf)
{
    struct AC6956C_MessageBody body_m = {0};
    char buf[UART_TX_WRITE_MAX_BUFF] = {0};
    int ret = BT_RET_SUCCESS;
    struct bluetooth_slave_dev *acq_dev_info = NULL;

    bt_cmds_e *dev_status = NULL;
    if (gbt == NULL)
    {
        BLUETOOTH_LOG("gbt = NULL \n");
        return BT_RET_ERROR;
    }


    if (gbt->uartfd < 0)
    {
        BLUETOOTH_LOG("gbt->uartfd  =%d\n",gbt->uartfd);
        return BT_RET_ERROR;
    }
    body_m.cmd_id = cmd;
    body_m.cmd_len = 0x00;
    switch (cmd)
    {
        case CMD_SET_BT_POWER_ON:
            if (gbt->dev_status < EBT_DEVICE_STATUS_WORKING_CONNECTED)
            {
                gbt->dev_status = EBT_DEVICE_STATUS_NOWORKING_DEFAULT;
            }
            break;
        case CMD_SET_INQUIRY_START:
            memset(&gbt->inquiry_info, 0, sizeof(struct bluetooth_slave_dev));
            break;
        case CMD_SET_CONNECT:
            if (send_buf == NULL)goto error;
            body_m.cmd_len = 0x06;
            break;
        case CMD_SET_BT_POWER_OFF:
            gbt->dev_status = EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT;
            break;
        case CMD_SET_MEMORY_CONNECTION:
            body_m.cmd_len = 0x01;
            break;
        case CMD_SET_BT_PIN_FUNCTION:
            body_m.cmd_len = 0x02;
            break;
        case CMD_SET_BT_GPIO_POL:
            body_m.cmd_len = 0x02;
            break;
        case CMD_SET_BT_IR_POWER_KEY:
            body_m.cmd_len = 0x02;
            break;
        case CMD_SET_IR_USERCODE:
            body_m.cmd_id = CMD_SET_IR_USERCODE;
            body_m.cmd_len = 0x02;
            break;
        case CMD_SET_AUDIO_CHANNEL_INPUT_SELECT:
            body_m.cmd_len = 0x01;
            break;
        case CMD_SET_BT_PWM_PARAM:
            body_m.cmd_len = 0x03;
            break;
        case CMD_SET_BT_CTRL_CMD:
            body_m.cmd_len = 0x01;
            break;
        case CMD_SET_BT_MUSIC_VOL_VALUE:
            body_m.cmd_len = 0x01;
            break;
        case CMD_SET_BT_POWER_ON_TO_RX:
            gbt->dev_status = EBT_DEVICE_STATUS_WORKING_ON_RX_MODE;
            break;
        case CMD_SET_BT_POWER_ON_TO_TX:
            gbt->dev_status = EBT_DEVICE_STATUS_WORKING_ON_TX_MODE;
            break;
        case CMD_SET_BT_LOCAL_NAME:
            body_m.cmd_len = strlen(send_buf)+1;
            break;
        case CMD_SET_CHANNEL_MAP:
            body_m.cmd_len = 0x0a;
            break;
        case CMD_SET_BT_DEFLAULT_IO:
            body_m.cmd_len = 0x02;
            break;
        default:
            break;
    }
    if (ret != 0)goto error;
    gbt->bt_cmds = cmd;
    memcpy(body_m.cmd_value, send_buf, body_m.cmd_len);
    /*uart send data*/
    body_m.frame_id = 0x00;
    body_m.frame_head = 0xAC69;
    body_m.frame_len = 9 + body_m.cmd_len;
    memset(buf, 0, UART_TX_WRITE_MAX_BUFF);
    if (bt_ac6956c_messagebody_switch_str(&body_m, buf) == BT_RET_ERROR)goto error;
    ret = write(gbt->uartfd, buf, body_m.frame_len);
    if (ret < 0)goto error;
    ret = BT_RET_SUCCESS;
exit:
    return ret;
error:
    ret = BT_RET_ERROR;
    return ret;
}
/**
 * @description: updata bt_priv_t,do action by rx_data
 * @author: Yanisin
 */
static void bt_ac6956c_serial_data_judg(bt_cmds_e cmd, char *buf, unsigned int count)
{
    unsigned int i = 0;
    int ret = BT_RET_SUCCESS;
    struct AC6956C_MessageBody body_m = {0};
    struct bluetooth_slave_dev *acq_dev_info = &gbt->inquiry_info;
    struct bluetooth_slave_dev *connet_data = &gbt->connet_info;
    bt_gpio_set_t* pinpad_level = &(gbt->bt_pinpad_level);
    struct input_event_bt event_key = {0};
    unsigned char name_cnt = 0;
    unsigned char disconnet_repeat_cnt = 0;
    uint16_t tmp_code = 0;
    i = count;
    while (i >= BLUETOOTH_MINIMUM_CHECKSUM)
    {
        if (ret != 0 )
        {
            ret = BT_RET_SUCCESS;
            break;
        }
        if (str_switch_bt_ac6956c_messagebody(&buf[count - i], &i, &body_m) == BT_RET_SUCCESS)
        {
            if (body_m.frame_head != 0xac69)
            {
                gbt->dev_status = EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR;
                break;
            }
            switch (cmd)
            {
                case CMD_SET_BT_POWER_ON:
                    if (gbt->dev_status < EBT_DEVICE_STATUS_WORKING_CONNECTED)
                    {
                        gbt->dev_status = EBT_DEVICE_STATUS_WORKING_EXISTENT;
                    }
                    break;
                case CMD_SET_INQUIRY_START:
                    break;
                case CMD_SET_CONNECT:
                    break;
                case CMD_GET_BT_CONNECT_STATE:
                    if (body_m.cmd_id == 0X45)
                    {
                        if (body_m.cmd_value[0] == 0)
                        {
                            gbt->dev_status = EBT_DEVICE_STATUS_WORKING_CONNECTED;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED, 0);
                        }
                        else
                        {
                            gbt->dev_status = EBT_DEVICE_STATUS_WORKING_DISCONNECTED;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED, 0);
                            ret = BT_RET_EXIT;
                        }
                    }
                    break;
                case CMD_SET_DISCONNECT:
                    gbt->dev_status=EBT_DEVICE_STATUS_WORKING_DISCONNECTED;
                    gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED,0);
                    break;
                default:
                    // due to after poweroff bt_module has to do something
                    // ret = BT_RET_EXIT;
                    break;
            }
            if (ret != BT_RET_EXIT)
            {
                switch (body_m.cmd_id)
                {
                    // handler rx_buffer data ,do action by cmd_id
                    case CMD_REPORT_CONNECT_STATUS:
                        if (body_m.cmd_value[0] == 0x01)
                        {
                            gbt->dev_status = EBT_DEVICE_STATUS_WORKING_CONNECTED;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED, 0);
                        }
                        else
                        {
                            if (disconnet_repeat_cnt == 0)
                            {
                                memset(connet_data, 0, sizeof(struct bluetooth_slave_dev));
                                gbt->dev_status = EBT_DEVICE_STATUS_WORKING_DISCONNECTED;
                                gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED, 0);
                            }
                            disconnet_repeat_cnt++;
                        }
                        break;
                    case CMD_REPORT_CONNECT_RESULTS:
                        memset(connet_data, 0, sizeof(struct bluetooth_slave_dev));
                        memcpy(connet_data->mac, body_m.cmd_value, BLUETOOTH_MAC_LEN);
                        if (body_m.cmd_len - 6 < BLUETOOTH_NAME_LEN)
                        {
                            memcpy(connet_data->name, &body_m.cmd_value[BLUETOOTH_MAC_LEN], body_m.cmd_len - 6);
                        }
                        else
                        {
                            memcpy(connet_data->name, &body_m.cmd_value[BLUETOOTH_MAC_LEN], BLUETOOTH_NAME_LEN);
                            connet_data->name[BLUETOOTH_NAME_LEN - 1] = 0;
                        }
                        if(gbt->dev_status==EBT_DEVICE_STATUS_WORKING_ON_RX_MODE){
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_RXMODE_CONNECT_RESULTS, (unsigned long)connet_data);
                        }else{
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED,0);
                            gbt->dev_status = EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_GET_CONNECTED_INFO, (unsigned long)connet_data);
                        }
                        break;
                    case CMD_REPORT_INQUIRY_RESULTS:
                        if(gbt->dev_status==EBT_DEVICE_STATUS_WORKING_ON_TX_MODE){
                            /* to do */
                        }else{
                            memset(acq_dev_info, 0, sizeof(struct bluetooth_slave_dev));
                            name_cnt = body_m.cmd_len - BLUETOOTH_MAC_LEN - 1;
                            memcpy(acq_dev_info->mac, body_m.cmd_value, BLUETOOTH_MAC_LEN);
                            if (name_cnt < BLUETOOTH_NAME_LEN)
                            {
                                memcpy(acq_dev_info->name, &body_m.cmd_value[BLUETOOTH_MAC_LEN + 1], name_cnt);
                            }
                            else
                            {
                                memcpy(acq_dev_info->name, &body_m.cmd_value[BLUETOOTH_MAC_LEN + 1], BLUETOOTH_NAME_LEN);
                                acq_dev_info->name[BLUETOOTH_NAME_LEN - 1] = 0;
                            }
                            gbt->dev_status = EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_SCANNED, (unsigned long)acq_dev_info);
                        }
                        break;
                    case CMD_REPORT_DISCONNECT_RESULTS:
                        if(gbt->dev_status == EBT_DEVICE_STATUS_WORKING_ON_RX_MODE){
                            /* filter some useless rx_cmds*/
                        }else{
                            gbt->dev_status = EBT_DEVICE_STATUS_WORKING_DISCONNECTED;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED,0);
                        }
                        break;
                    case CMD_REPORT_INQUIRY_COMPLETE:
                        if(gbt->dev_status==EBT_DEVICE_STATUS_WORKING_ON_TX_MODE || 
                            gbt->dev_status==EBT_DEVICE_STATUS_WORKING_ON_RX_MODE ){
                            /* to do */
                        }else{
                            gbt->dev_status = EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING;
                            gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED, (unsigned long)acq_dev_info);
                        }
                        break;
                    case CMD_REPORT_BT_GPI_STAT:
                        memcpy(&pinpad_level->pinpad, body_m.cmd_value, 1);
                        memcpy(&pinpad_level->value, body_m.cmd_value + 1, 1);
                        gbt->callback(BLUETOOTH_EVENT_REPORT_GPI_STAT, (unsigned long)pinpad_level);
                        break;
                    case CMD_REPORT_BT_IR_KEY:
                        // cmd_value =type(1B IR/ADC)+ir keycode size(4B?)+ status(repeat/press/release)(1B)?
                        /*if ir report code is 2B ? */
                        tmp_code = body_m.cmd_value[1];
                        event_key.code = bt_reportkey_mapping(BT_IR_KEY, tmp_code); //map
                        if (body_m.cmd_value[5] == 0x00)
                        {
                            event_key.value = 1;
                        }
                        else if (body_m.cmd_value[5] == 0x02)
                        {
                            event_key.value = 0; // report real value or def by user ?
                        }
                        else
                        {
                            event_key.value = body_m.cmd_value[5];
                        }
                        bluetooth_input_event_send(event_key);
                        //it will call a register func to write key_event_queue
                        break;
                    case CMD_REPORT_BT_ADC_KEY:
                        // cmd_value = type(1B IR/ADC)+ir adckey_volatage(2B)+ status(press/repeat/release)(1B)
                        tmp_code = body_m.cmd_value[2] << 8;
                        tmp_code |= body_m.cmd_value[1];
                        event_key.code = bluetooth_adckey_voltage_mapping(tmp_code); //map
                        if (body_m.cmd_value[3] == 0x00)
                        {
                            event_key.value = 1;
                        }
                        else if (body_m.cmd_value[3] == 0x02)
                        {
                            event_key.value = 0; // report real value or def by user ?
                        }
                        else
                        {
                            event_key.value = body_m.cmd_value[3];
                        }
                        bluetooth_input_event_send(event_key);
                        break;
                    case CMD_SET_BT_CMD_ACK:
                        break;
                    case CMD_REPORT_AUDIO_CHANNEL_INPUT_SELECT:
                        gbt->dac_input_channel = body_m.cmd_value[0];
                        bluetooth_sem_post();
                        break;
                    case CMD_REPORT_BT_MUSIC_VOL_RESULTS:
                        body_m.cmd_value[0] = body_m.cmd_value[0] * 3;
                        memcpy(&gbt->dac_volume, body_m.cmd_value, 1);
                        bluetooth_sem_post();
                        break;
                    case CMD_REPORT_BT_LOCAL_NAME:
                        if(body_m.cmd_len > BLUETOOTH_MAX_LOCALNAME_SIZE)
                            body_m.cmd_len = BLUETOOTH_MAX_LOCALNAME_SIZE;
                        memset(gbt->device_name,0,BLUETOOTH_MAX_LOCALNAME_SIZE);
                        memcpy(gbt->device_name,body_m.cmd_value,body_m.cmd_len);
                        gbt->callback(BLUETOOTH_EVENT_SLAVE_REPORT_LOCALNAME,(unsigned long)gbt->device_name);
						break;
                    case CMD_REPORT_BT_VERSION:
                        memset(gbt->firmware_version,0,sizeof(gbt->firmware_version));
                        memcpy(gbt->firmware_version,body_m.cmd_value,body_m.cmd_len);
                        printf("BT_VERSION:%s\n",body_m.cmd_value);
                        break;
                    case CMD_REPORT_CHANNEL_MAP: 
                        memset(gbt->signal_channelmap,0,sizeof(gbt->signal_channelmap));
                        memcpy(gbt->signal_channelmap,body_m.cmd_value,body_m.cmd_len);
                        bluetooth_sem_post();
                        break;
                    default:
                        break;
                }
            }
        }
        else
        {
            gbt->dev_status = EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR;
            ret = BT_RET_ERROR;
        }
    }
}

static void bt_ac6956c_set_poll_timeout(int cmds, int *poll_t)
{
    switch (cmds)
    {
        case CMD_SET_BT_POWER_ON:
            *poll_t = 500;
            break;
        case CMD_SET_CONNECT:
            *poll_t = 1000;
            break;
        case CMD_SET_INQUIRY_START:
        case CMD_SET_INQUIRY_STOP:
        case CMD_SET_DISCONNECT:
        case CMD_SET_BT_POWER_OFF:
        case CMD_GET_BT_CONNECT_STATE:
            *poll_t = 200;
            break;
        default:
            *poll_t = 100;
            break;
    }
}

static void printf_bt_ac6956c_dev_status(bt_device_status_e status)
{
    switch (status)
    {
        case EBT_DEVICE_STATUS_NOWORKING_DEFAULT:
            BLUETOOTH_LOG("EBT_DEVICE_STATUS_NOWORKING_DEFAULT\n");
            break;
        case EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR:
            BLUETOOTH_LOG("EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR\n");
            break;
        case EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT:
            BLUETOOTH_LOG("EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT\n");
            break;
        case EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF:
            BLUETOOTH_LOG("EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_EXISTENT:
            BLUETOOTH_LOG("EBT_DEVICE_STATUS_WORKING_EXISTENT\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED:
            BLUETOOTH_LOG("EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING:
            BLUETOOTH_LOG("EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_DISCONNECTED:
            BLUETOOTH_LOG("EBT_DEVICE_STATUS_WORKING_DISCONNECTED\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_CONNECTED:
            BLUETOOTH_LOG("EBT_DEVICE_STATUS_WORKING_CONNECTED\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO:
            BLUETOOTH_LOG("EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO\n");
            break;
        default:
            BLUETOOTH_LOG("other\n");
            break;
            break;
    }
}
/**
 * @description: checksum calculate,add checksum end of MessageBody
 * @param {AC6956C_MessageBody} *body
 * @return {*}
 */
static void checksum_calculate(struct AC6956C_MessageBody *body)
{
    unsigned short sum = 0;
    if (body == NULL)return;
    sum += (body->frame_head & 0x00ff);
    sum += (body->frame_head >> 8);
    sum += (body->frame_len & 0x00ff);
    sum +=  (body->frame_len >> 8);
    sum +=  body->frame_id;
    sum +=  body->cmd_id;
    sum +=  body->cmd_len;
    for (int i = 0; i < body->cmd_len; i++)
    {
        sum += body->cmd_value[i];
    }
    body->Frame_checksum = sum;
}
// BLUETOOTH_LOG send msg
static void bt_ad6956f_mes_printf(struct AC6956C_MessageBody *body)
{
    if(body==NULL)return;
    BLUETOOTH_LOG("bt_msg: %02x %02x %02x %02x %02x %02x %02x\n",(body->frame_head>>8),(body->frame_head&0x00ff),(body->frame_len&0x00ff),(body->frame_len>>8),body->frame_id,body->cmd_id,body->cmd_len);
    BLUETOOTH_LOG("data: ");
    for(int i=0;i<body->cmd_len;i++){
        BLUETOOTH_LOG("%02x ",body->cmd_value[i]);
    }
    BLUETOOTH_LOG("\n");
    BLUETOOTH_LOG("checksum: %02x %02x\n",(body->Frame_checksum&0x00ff),(body->Frame_checksum>>8));
}

// convert msg_t to rx_buf data
static int bt_ac6956c_messagebody_switch_str(struct AC6956C_MessageBody *body, unsigned char *buf)
{
    unsigned char *offset = buf;
    if (body == NULL)return BT_RET_ERROR;
    if (body->cmd_len + 9 >= UART_RX_RECEIVE_MAX_BUFF)
    {
        BLUETOOTH_LOG("Buf is too long\n");
        return BT_RET_ERROR;
    }
    checksum_calculate(body);
    *offset++ = body->frame_head >> 8;
    *offset++ = body->frame_head & 0x00ff;
    *offset++ = body->frame_len & 0x00ff;
    *offset++ = body->frame_len >> 8;
    *offset++ = body->frame_id;
    *offset++ = body->cmd_id;
    *offset++ = body->cmd_len;
    memcpy(offset, body->cmd_value, body->cmd_len);
    offset += body->cmd_len;
    *offset++ = body->Frame_checksum & 0x00ff;
    *offset = body->Frame_checksum >> 8;
    BLUETOOTH_LOG("TX_CMD\n");
    bt_ad6956f_mes_printf(body);
    return BT_RET_SUCCESS;
}
/*uart rx_buf transform to messagebody_t */
static int str_switch_bt_ac6956c_messagebody(char *buf, unsigned int *count, struct AC6956C_MessageBody *body)
{
    unsigned char *offset = (unsigned char*)buf;
    unsigned int temp = 0, temp1 = 0;
    if (body == NULL || buf == NULL || *count <= 0)return BT_RET_ERROR;

    body->frame_head = *offset++;
    body->frame_head <<= 8;
    body->frame_head += (*offset++);

    if (body->frame_head != 0xac69)
    {
        *count -= 2;
        return BT_RET_ERROR;
    }
    body->frame_len = *offset++;
    temp = *offset++;
    body->frame_len |= (temp >> 8);
    body->frame_id = *offset++;
    body->cmd_id = *offset++;
    body->cmd_len = *offset++;
    if (body->cmd_len + 9 >= UART_RX_RECEIVE_MAX_BUFF || body->cmd_len >= AC6956C_MAX_WRITE_SIZE)
    {
        BLUETOOTH_LOG("Buf is too long\n");
        return BT_RET_ERROR;
    }
    memcpy(body->cmd_value, offset, body->cmd_len);
    offset += body->cmd_len;
    checksum_calculate(body);
    temp = 0;
    temp1 = (*offset++) & 0x00ff;
    temp = *offset;
    temp <<= 8;
    temp += temp1;
    BLUETOOTH_LOG("RX_CMD\n");
    bt_ad6956f_mes_printf(body);
    *count -= (body->cmd_len + 9);
    BLUETOOTH_LOG("body->Frame_checksum =%04x temp= %04x\n", body->Frame_checksum, temp);
    if (body->Frame_checksum == temp)
        return BT_RET_SUCCESS;
    else
        return BT_RET_ERROR;
}

/**
 * @description: judge rx_data whether it is bt_cmd format
 * @param {char} *buf
 * @param {unsigned char} count
 * @return {*} rx_data buffer is bt cmd ->BT_RET_SUCCESS ,else BT_RET_ERROR
 */
static int str_with_ac6956c_messagebody_compare(char *buf, unsigned char count)
{
    struct AC6956C_MessageBody body = {0};
    unsigned char *offset = (unsigned char*)buf;
    unsigned int temp = 0, temp1 = 0;

    body.frame_head = *offset++;
    if (body.frame_head != 0xac)
    {
        BLUETOOTH_LOG("ad6956f frame head !=0xac error body.frame_head = %d\n",body.frame_head);
        return BT_ERROR_DATA_OFFSET_1;
    }

    body.frame_head <<= 8;
    body.frame_head |= (*offset++);

    if (body.frame_head != 0xac69)
    {
        BLUETOOTH_LOG("ad6956f frame head !=0xac69 error body.frame_head = %d\n",body.frame_head);
        return BT_ERROR_DATA_OFFSET_2;
    }
    body.frame_len = *offset++;
    temp = *offset++;
    body.frame_len |= (temp >> 8);
    if (body.frame_len != count)
    {
        return BT_DATA_AGAIN;
    }
    body.frame_id = *offset++;
    body.cmd_id = *offset++;
    body.cmd_len = *offset++;
    if (body.cmd_len >= AC6956C_MAX_WRITE_SIZE)
    {
        BLUETOOTH_LOG("Cmd_len buf is too long\n");
        return BT_RET_ERROR;
    }
    memcpy(body.cmd_value, offset, body.cmd_len);
    offset += body.cmd_len;
    checksum_calculate(&body);
    temp = 0;
    temp1 = (*offset++) & 0x00ff;
    temp = *offset;
    temp <<= 8;
    temp += temp1;
    BLUETOOTH_LOG("body.Frame_checksum =%04x temp= %04x\n", body.Frame_checksum, temp);
    if (body.Frame_checksum == temp)
        return BT_RET_SUCCESS;
    else
    {
        return BT_RET_ERROR;
    }
}


static void bt_ac6956c_cp_connet_info(struct bluetooth_slave_dev *connet_cmds, struct AC6956C_MessageBody *obj)
{
    if (connet_cmds == NULL || obj == NULL)return;
    memcpy(obj->cmd_value, connet_cmds->mac, BLUETOOTH_MAC_LEN);
    obj->cmd_len = BLUETOOTH_MAC_LEN;
}


/**
 * @description: mapping bt_report key to keymap_table keycode
 * @param {bt_keytype_e} keytype to mapping diff keymap_table
 * @param {uint32_t} btcode bt_report keycode
 * @return {*} keymap_table's keycode
 * @author: Yanisin
 */
static uint16_t bt_reportkey_mapping(bt_keytype_e keytype, uint32_t btcode)
{
    if (keytype == BT_IR_KEY)
    {
        int key_cnt = sizeof(ir_keymap_table) / sizeof(ir_keymap_table[0]);
        for (int i = 0; i < key_cnt; i++)
        {
            if (btcode == ir_keymap_table[i].report_code)
            {
                return ir_keymap_table[i].keycode;
            }
        }
    }
    else if (keytype == BT_ADC_KEY)
    {
        int key_cnt = sizeof(adc_keymap_table) / sizeof(adc_keymap_table[0]);
        for (int i = 0; i < key_cnt; i++)
        {
            if (btcode == adc_keymap_table[i].report_code)
            {
                return adc_keymap_table[i].keycode;
            }
        }
    }
    else if (keytype == BT_GPIO_KEY)
    {
        /*reserve*/
    }
}



#ifdef HC_LINUX
/*set uart config opt*/
static int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio, oldtio;

    if ( tcgetattr( fd, &oldtio) != 0)
    {
        perror("SetupSerial 1");
        return -1;
    }

    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
    newtio.c_oflag  &= ~OPOST;   /*Output*/

    switch ( nBits )
    {
        case 7:
            newtio.c_cflag |= CS7;
            break;
        case 8:
            newtio.c_cflag |= CS8;
            break;
    }

    switch ( nEvent )
    {
        case 'O':
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'E':
            newtio.c_iflag |= (INPCK | ISTRIP);
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD;
            break;
        case 'N':
            newtio.c_cflag &= ~PARENB;
            break;
    }

    switch ( nSpeed )
    {
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
            break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
            break;
        default:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
    }

    if ( nStop == 1 )
        newtio.c_cflag &= ~CSTOPB;
    else if ( nStop == 2 )
        newtio.c_cflag |= CSTOPB;

    newtio.c_cc[VMIN]  = 1;
    newtio.c_cc[VTIME] = 0;

    tcflush(fd, TCIFLUSH);

    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0)
    {
        perror("com set error");
        return -1;
    }

    return 0;
}



#endif

int bluetooth_set_gpio_backlight(unsigned char value)
{
    if(!gbt)
    {
        BLUETOOTH_LOG("Bluetooth init error \n");
        return BT_RET_ERROR;
    }
    bt_pwm_param_t lcd_backlight = {0};
    lcd_backlight.pinpad = gbt->dts_info.pinpad_lcd_backlight;
    lcd_backlight.type = 0; //freq
    lcd_backlight.value = 10; //10Khz
    bluetooth_ioctl(BLUETOOTH_SET_PWM_PARAM, &lcd_backlight);
    if (value)
    {
        // set duty pwm polarity is false.set output level high
        lcd_backlight.pinpad = gbt->dts_info.pinpad_lcd_backlight;
        lcd_backlight.type = 1; //duty
        lcd_backlight.value = 0;
        bluetooth_ioctl(BLUETOOTH_SET_PWM_PARAM, &lcd_backlight);
    }
    else
    {
        // set duty pwm polarity is false.set output level low
        lcd_backlight.pinpad = gbt->dts_info.pinpad_lcd_backlight;
        lcd_backlight.type = 1; //duty
        lcd_backlight.value = 100;
        bluetooth_ioctl(BLUETOOTH_SET_PWM_PARAM, &lcd_backlight);
    }
    return BT_RET_SUCCESS;
}

static int bluetooth_get_dts_uint32(const char *path)
{
    int fd = open(path, O_RDONLY);
    int value = -1;
    if(fd >= 0){
        uint8_t buf[4];
        if(read(fd, buf, 4) != 4){
            close(fd);
            return value;
        }
        close(fd);
        value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
    }
    printf("fd:%d ,dts value: %x\n", fd,value);
    return value;
}

static int bluetooth_get_dts_uint32_by_index(const char *path,int index)
{
    int fd = open(path, O_RDONLY);
    int value = -1;
    if(fd >= 0){
        lseek(fd,4*index,SEEK_SET);
        uint8_t buf[4];
        if(read(fd, buf, 4) != 4){
            close(fd);
            return value;
        }
        close(fd);
        value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
    }
    printf("fd:%d ,dts value: %x\n", fd,value);
    return value;
   
}

static void bluetooth_get_dts_string(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);
    int value = 0;;
    if(fd >= 0){
        read(fd, string, size);
        close(fd);
    }
    //printf("dts string: %s\n", string);
}

#define BLUETOOTH_DEVICE_TREE_PATH      "/proc/device-tree/bluetooth/"
static void bluetooth_get_dts_info(void)
{
    #ifdef HC_RTOS
    int np = fdt_node_probe_by_path("/hcrtos/bluetooth");
    if(np > 0){
        fdt_get_property_u_32_index(np,"ir_usercode",0,(u32*)&gbt->dts_info.ir_usercode);
        fdt_get_property_u_32_index(np,"ir_powerkey_code",0,(u32*)&gbt->dts_info.ir_powerkey_code);
        fdt_get_property_u_32_index(np,"adckey-num",0,(u32*)&gbt->dts_info.adckey_num);
        fdt_get_property_u_32_array(np,"adckey-map",(u32*)gbt->dts_info.adckey_array,gbt->dts_info.adckey_num * 3);
        fdt_get_property_u_32_index(np,"pinmux-lineout-det",0,(u32*)&gbt->dts_info.pinpad_lineout_det);
        fdt_get_property_u_32_index(np,"pinmux-lineout-det",1,(u32*)&gbt->dts_info.pinset_lineout_det);
        fdt_get_property_u_32_index(np,"pinmux-lcd-backlight",0,(u32*)&gbt->dts_info.pinpad_lcd_backlight);
        fdt_get_property_u_32_index(np,"pinmux-lcd-backlight",1,(u32*)&gbt->dts_info.pinset_lcd_backlight);
        fdt_get_property_u_32_index(np,"wifien-gpios",0,(u32*)&gbt->dts_info.wifien_gpios);
        fdt_get_property_u_32_index(np,"wifien-gpios",1,(u32*)&gbt->dts_info.wifien_gpios_value);
    }
    #else
    char * status[16] = {0};
    bluetooth_get_dts_string(BLUETOOTH_DEVICE_TREE_PATH "status",status,sizeof(status));
    if(!strcmp(status,"okay")){
        gbt->dts_info.ir_usercode = bluetooth_get_dts_uint32( BLUETOOTH_DEVICE_TREE_PATH "ir_usercode");
        gbt->dts_info.ir_powerkey_code = bluetooth_get_dts_uint32( BLUETOOTH_DEVICE_TREE_PATH "ir_powerkey_code");
        gbt->dts_info.adckey_num = bluetooth_get_dts_uint32( BLUETOOTH_DEVICE_TREE_PATH "adckey-num");
        for(int i = 0; i < gbt->dts_info.adckey_num; i++){
            gbt->dts_info.adckey_array[i].voltage_min = bluetooth_get_dts_uint32_by_index(BLUETOOTH_DEVICE_TREE_PATH "adckey-map",i*3);
            gbt->dts_info.adckey_array[i].voltage_max = bluetooth_get_dts_uint32_by_index(BLUETOOTH_DEVICE_TREE_PATH "adckey-map",i*3+1);
            gbt->dts_info.adckey_array[i].keycode = bluetooth_get_dts_uint32_by_index(BLUETOOTH_DEVICE_TREE_PATH "adckey-map",i*3+2);
        }
        gbt->dts_info.pinpad_lineout_det = bluetooth_get_dts_uint32_by_index( BLUETOOTH_DEVICE_TREE_PATH "pinmux-lineout-det", 0);
        gbt->dts_info.pinset_lineout_det = bluetooth_get_dts_uint32_by_index( BLUETOOTH_DEVICE_TREE_PATH "pinmux-lineout-det", 1);
        gbt->dts_info.pinpad_lcd_backlight = bluetooth_get_dts_uint32_by_index( BLUETOOTH_DEVICE_TREE_PATH "pinmux-lcd-backlight", 0);
        gbt->dts_info.pinset_lcd_backlight = bluetooth_get_dts_uint32_by_index( BLUETOOTH_DEVICE_TREE_PATH "pinmux-lcd-backlight", 1);
        gbt->dts_info.wifien_gpios = bluetooth_get_dts_uint32_by_index( BLUETOOTH_DEVICE_TREE_PATH "wifien-gpios", 0);
        gbt->dts_info.wifien_gpios_value = bluetooth_get_dts_uint32_by_index( BLUETOOTH_DEVICE_TREE_PATH "wifien-gpios", 1);
    }    
    #endif 
}

#define CHANNEL_MAP_MAX_SIZE 10
static int bluetooth_set_close_channel_map(struct bluetooth_channel_map *map)
{
    unsigned char buf[CHANNEL_MAP_MAX_SIZE] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    int count = 0;
    int i = 0;
    int j = 0;
    if (map->x <= 2)
        map->x = 0;
    else
        map->x = map->x - 2;

    map->y = map->y - 2;

    count = map->y - map->x + 1;

    if (count < 0)
        return BT_RET_ERROR;

    if (map->y > CHANNEL_MAP_MAX_SIZE * 8)
    {
        map->y = CHANNEL_MAP_MAX_SIZE * 8;
    }

    if (count >= 11 && map->y <= (CHANNEL_MAP_MAX_SIZE * 8))
    {
        buf[map->x / 8] = (~((1 << ((map->x / 8 + 1) * 8 - map->x - 1)) * 2 - 1));
        buf[map->y / 8] = ((1 << ((map->y / 8 + 1) * 8 - map->y - 1)) - 1);

        i = map->y / 8 - map->x / 8;
        if (i > 1)
        {
            for (j = 1; j < i; j++)
            {
                buf[map->x / 8 + j] = 0;
            }
        }
    }

    if (bt_ac6956c_set_action(CMD_SET_CHANNEL_MAP , buf) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

static int bluetooth_set_open_channel_map(struct bluetooth_channel_map *map)
{
    unsigned char buf[CHANNEL_MAP_MAX_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int count = 0;
    int i = 0;
    int j = 0;
    if (map->x <= 2)
        map->x = 0;
    else
        map->x = map->x - 2;

    map->y = map->y - 2;

    count = map->y - map->x + 1;

    if (count < 0)
        return BT_RET_ERROR;

    if (map->y > CHANNEL_MAP_MAX_SIZE * 8)
    {
        map->y = CHANNEL_MAP_MAX_SIZE * 8;
    }

    if (count >= 11 && map->y <= CHANNEL_MAP_MAX_SIZE * 8)
    {
        buf[map->x / 8] = (1 << ((map->x / 8 + 1) * 8 - map->x - 1)) * 2 - 1;
        buf[map->y / 8] = ~((1 << ((map->y / 8 + 1) * 8 - map->y - 1)) - 1);

        i = map->y / 8 - map->x / 8;
        if (i > 1)
        {
            for (j = 1; j < i; j++)
            {
                buf[map->x / 8 + j] = 0xff;
            }
        }
    }   
    if (bt_ac6956c_set_action(CMD_SET_CHANNEL_MAP , buf) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

static int bluetooth_sem_timewait(int ms)
{
    int ret = 0;
    #ifdef HC_RTOS
    ret = wait_for_completion_timeout(&bluetooth_cmds_sem,ms); //unit ms 
    #else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long secs = ms/1000;
    ms = ms%1000;

    long add = 0;
    ms = ms*1000*1000 + ts.tv_nsec;
    add = ms / (1000*1000*1000);
    ts.tv_sec += (add + secs);
    ts.tv_nsec = ms%(1000*1000*1000);
    ret = sem_timedwait(&bluetooth_cmds_sem,&ts);
    #endif
    return ret;
}

static int bluetooth_sem_post(void)
{
    int ret = 0; 
    #ifdef HC_RTOS
    complete(&bluetooth_cmds_sem);
    #else
    sem_post(&bluetooth_cmds_sem);
    #endif 
    return ret;
}

static uint16_t bluetooth_adckey_voltage_mapping(uint16_t value)
{
    int key_num = gbt->dts_info.adckey_num;
    uint16_t ret = 0 ;
    for(int i = 0; i < key_num ;i++){
        if(value >= gbt->dts_info.adckey_array[i].voltage_min 
            && value <= gbt->dts_info.adckey_array[i].voltage_max){
            ret = gbt->dts_info.adckey_array[i].keycode;
        }
    }
    return ret;
}



#endif
