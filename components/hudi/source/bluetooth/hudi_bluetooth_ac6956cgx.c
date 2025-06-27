/**
* @file
* @brief                hudi bluetooth interface
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <hcuapi/input-event-codes.h>
#include <semaphore.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_bluetooth.h>
#include "hudi_bluetooth_inter.h"

typedef enum
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
    CMD_SET_BT_POWER_ON_TO_RX = 0x40,
    CMD_SET_BT_POWER_ON_TO_TX = 0x41,
    CMD_GET_BT_RX_OR_TX_MODE = 0x42,
    CMD_REPORT_BT_RX_OR_TX_MODE = 0x43,
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
    /* HICHIP DEFINE CMD */
    CMD_SET_BT_PIN_FUNCTION = 0xe0,
    CMD_SET_BT_GPIO_POL = 0xe1,
    CMD_REPORT_BT_GPI_STAT = 0xe2,
    CMD_SET_BT_IR_PROTOCAL = 0xe3, //unspport
    CMD_REPORT_BT_IR_KEY = 0xe4,
    CMD_REPORT_BT_ADC_KEY = 0xe5,
    CMD_SET_BT_IR_POWER_KEY = 0xe7,
    CMD_SET_BT_FM = 0xe8, //unspport
    CMD_SET_IR_USERCODE = 0xe9,
    CMD_SET_BT_POWER_PINPAD_OFF = 0xea,
    CMD_SET_BT_PWM_PARAM = 0xeb,
    CMD_SET_BT_DEFLAULT_IO = 0xec,
    CMD_SET_CHANNEL_MAP = 0xfa,
    CMD_REPORT_CHANNEL_MAP = 0xfb,
} hudi_bluetooth_ac6956cgx_cmd_id_e;

typedef enum
{
    DEVICE_STATUS_DISCONNECTED = 0,
    DEVICE_STATUS_CONNECTED,
} hudi_bluetooth_dev_connect_sta_e;

typedef enum
{
    BT_IR_KEY = 0,
    BT_ADC_KEY,
    BT_GPIO_KEY,
} bt_keytype_e;


typedef struct
{
    uint32_t report_code;
    uint16_t keycode;
} bt_keymap_table_t;

typedef struct
{
    unsigned short  frame_head;
    unsigned short  frame_len;
    unsigned char   frame_id;
    unsigned char   cmd_id;
    unsigned char   cmd_len;
    unsigned char   cmd_value[128];
    unsigned short  checksum;
} hudi_bluetooth_ac6956cgx_frame_t;

typedef struct
{
    int voltage_min;
    int voltage_max;
    int keycode;
} adckey_map_t;

typedef struct
{
    int ir_usercode;
    int ir_powerkey_code;
    int pinpad_lineout_det;
    int pinset_lineout_det;
    int pinpad_lcd_backlight;
    int pinset_lcd_backlight;
    int wifien_gpios;
    int wifien_gpios_value;
    int adckey_num;
    adckey_map_t adckey_array[BLUETOOTH_ADCKEY_MAX_NUM];
} ac6956cgx_dts_resource_t;

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

static ac6956cgx_dts_resource_t m_ac6956cgx_dts_res = {0};

#ifdef __HCRTOS__

#include <kernel/completion.h>
#include <kernel/lib/fdt_api.h>

static int g_hudi_sem_inited = 0;
static struct completion g_hudi_bluetooth_sem;

static int _hudi_bluetooth_sem_init()
{
    if (g_hudi_sem_inited == 0)
    {
        init_completion(&g_hudi_bluetooth_sem);
        g_hudi_sem_inited = 1;
    }
    return 0;
}

static int _hudi_bluetooth_sem_timewait(int ms)
{
    return wait_for_completion_timeout(&g_hudi_bluetooth_sem, ms); //unit ms
}

static int _hudi_bluetooth_sem_post(void)
{
    complete(&g_hudi_bluetooth_sem);
    return 0;
}

static int _hudi_bluetooth_ac6956cgx_dts_parse(hudi_handle handle)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    int np = fdt_node_probe_by_path("/hcrtos/bluetooth");
    if (np < 0)
    {
        return ret;
    }
    fdt_get_property_u_32_index(np, "ir_usercode", 0, (u32 *)&m_ac6956cgx_dts_res.ir_usercode);
    fdt_get_property_u_32_index(np, "ir_powerkey_code", 0, (u32 *)&m_ac6956cgx_dts_res.ir_powerkey_code);
    fdt_get_property_u_32_index(np, "adckey-num", 0, (u32 *)&m_ac6956cgx_dts_res.adckey_num);
    fdt_get_property_u_32_array(np, "adckey-map", (u32 *)m_ac6956cgx_dts_res.adckey_array, m_ac6956cgx_dts_res.adckey_num * 3);
    fdt_get_property_u_32_index(np, "pinmux-lineout-det", 0, (u32 *)&m_ac6956cgx_dts_res.pinpad_lineout_det);
    fdt_get_property_u_32_index(np, "pinmux-lineout-det", 1, (u32 *)&m_ac6956cgx_dts_res.pinset_lineout_det);
    fdt_get_property_u_32_index(np, "pinmux-lcd-backlight", 0, (u32 *)&m_ac6956cgx_dts_res.pinpad_lcd_backlight);
    fdt_get_property_u_32_index(np, "pinmux-lcd-backlight", 1, (u32 *)&m_ac6956cgx_dts_res.pinset_lcd_backlight);
    fdt_get_property_u_32_index(np, "wifien-gpios", 0, (u32 *)&m_ac6956cgx_dts_res.wifien_gpios);
    fdt_get_property_u_32_index(np, "wifien-gpios", 1, (u32 *)&m_ac6956cgx_dts_res.wifien_gpios_value);
    inst->dts_res.priv = &m_ac6956cgx_dts_res;

    return 0;
}

#else
static int g_hudi_sem_inited = 0;
static sem_t g_hudi_bluetooth_sem;
static int _hudi_bluetooth_sem_init()
{
    if (g_hudi_sem_inited == 0)
    {
        sem_init(&g_hudi_bluetooth_sem, 0, 0);
        g_hudi_sem_inited = 1;
    }
    return 0;
}

static int _hudi_bluetooth_sem_timewait(int ms)
{
    int ret = 0;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long secs = ms / 1000;
    ms = ms % 1000;

    long add = 0;
    ms = ms * 1000 * 1000 + ts.tv_nsec;
    add = ms / (1000 * 1000 * 1000);
    ts.tv_sec += (add + secs);
    ts.tv_nsec = ms % (1000 * 1000 * 1000);
    ret = sem_timedwait(&g_hudi_bluetooth_sem, &ts);
    return ret;
}

static int _hudi_bluetooth_sem_post(void)
{
    return sem_post(&g_hudi_bluetooth_sem);
}

static int _hudi_bluetooth_dts_uint32_get(const char *path)
{
    int fd = open(path, O_RDONLY);
    int value = -1;
    if (fd >= 0)
    {
        uint8_t buf[4];
        if (read(fd, buf, 4) != 4)
        {
            close(fd);
            return value;
        }
        close(fd);
        value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
    }
    return value;
}

static int _hudi_bluetooth_dts_uint32_get_by_index(const char *path, int index)
{
    int fd = open(path, O_RDONLY);
    int value = -1;
    if (fd >= 0)
    {
        lseek(fd, 4 * index, SEEK_SET);
        uint8_t buf[4];
        if (read(fd, buf, 4) != 4)
        {
            close(fd);
            return value;
        }
        close(fd);
        value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
    }
    return value;

}

static int _hudi_bluetooth_ac6956cgx_dts_parse(hudi_handle handle)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;

    m_ac6956cgx_dts_res.ir_usercode = _hudi_bluetooth_dts_uint32_get(BLUETOOTH_DEVICE_TREE_PATH "ir_usercode");
    m_ac6956cgx_dts_res.ir_powerkey_code = _hudi_bluetooth_dts_uint32_get(BLUETOOTH_DEVICE_TREE_PATH "ir_powerkey_code");
    m_ac6956cgx_dts_res.adckey_num = _hudi_bluetooth_dts_uint32_get(BLUETOOTH_DEVICE_TREE_PATH "adckey-num");

    for (int i = 0; i < m_ac6956cgx_dts_res.adckey_num; i++)
    {
        m_ac6956cgx_dts_res.adckey_array[i].voltage_min = _hudi_bluetooth_dts_uint32_get_by_index(BLUETOOTH_DEVICE_TREE_PATH "adckey-map", i * 3);
        m_ac6956cgx_dts_res.adckey_array[i].voltage_max = _hudi_bluetooth_dts_uint32_get_by_index(BLUETOOTH_DEVICE_TREE_PATH "adckey-map", i * 3 + 1);
        m_ac6956cgx_dts_res.adckey_array[i].keycode = _hudi_bluetooth_dts_uint32_get_by_index(BLUETOOTH_DEVICE_TREE_PATH "adckey-map", i * 3 + 2);
    }
    m_ac6956cgx_dts_res.pinpad_lineout_det = _hudi_bluetooth_dts_uint32_get_by_index(BLUETOOTH_DEVICE_TREE_PATH "pinmux-lineout-det", 0);
    m_ac6956cgx_dts_res.pinset_lineout_det = _hudi_bluetooth_dts_uint32_get_by_index(BLUETOOTH_DEVICE_TREE_PATH "pinmux-lineout-det", 1);
    m_ac6956cgx_dts_res.pinpad_lcd_backlight = _hudi_bluetooth_dts_uint32_get_by_index(BLUETOOTH_DEVICE_TREE_PATH "pinmux-lcd-backlight", 0);
    m_ac6956cgx_dts_res.pinset_lcd_backlight = _hudi_bluetooth_dts_uint32_get_by_index(BLUETOOTH_DEVICE_TREE_PATH "pinmux-lcd-backlight", 1);
    m_ac6956cgx_dts_res.wifien_gpios = _hudi_bluetooth_dts_uint32_get_by_index(BLUETOOTH_DEVICE_TREE_PATH "wifien-gpios", 0);
    m_ac6956cgx_dts_res.wifien_gpios_value = _hudi_bluetooth_dts_uint32_get_by_index(BLUETOOTH_DEVICE_TREE_PATH "wifien-gpios", 1);

    inst->dts_res.priv = &m_ac6956cgx_dts_res;

    return 0;
}

#endif


static void _hudi_bluetooth_ac6956cgx_checksum_calc(hudi_bluetooth_ac6956cgx_frame_t *frame)
{
    unsigned short sum = 0;
    if (frame == NULL)return;
    sum += (frame->frame_head & 0x00ff);
    sum += (frame->frame_head >> 8);
    sum += (frame->frame_len & 0x00ff);
    sum +=  (frame->frame_len >> 8);
    sum +=  frame->frame_id;
    sum +=  frame->cmd_id;
    sum +=  frame->cmd_len;
    for (int i = 0; i < frame->cmd_len; i++)
    {
        sum += frame->cmd_value[i];
    }
    frame->checksum = sum;
}

static uint16_t _hudi_bluetooth_reportkey_mapping(bt_keytype_e keytype, uint32_t btcode)
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
        /*reserve*/
    }
    else if (keytype == BT_GPIO_KEY)
    {
        /*reserve*/
    }
}

static uint16_t _hudi_bluetooth_adckey_voltage_mapping(hudi_bluetooth_instance_t *inst, uint16_t value)
{
    int key_num;
    uint16_t ret = 0 ;
    ac6956cgx_dts_resource_t *attr = (ac6956cgx_dts_resource_t *)inst->dts_res.priv;
    if (attr == NULL)
    {
        hudi_log(HUDI_LL_ERROR, "Ac6956cgx dts resource parse error\n");
        return ret;
    }

    key_num = attr->adckey_num;
    for (int i = 0; i < key_num ; i++)
    {
        if (value >= attr->adckey_array[i].voltage_min
            && value <=  attr->adckey_array[i].voltage_max)
        {
            ret = attr->adckey_array[i].keycode;
        }
    }
    return ret;
}

static int _hudi_bluetooth_ac6956cgx_proto_framecheck(char *buf, int len)
{
    int ret = -1;
    hudi_bluetooth_ac6956cgx_frame_t frame = {0};
    memset(&frame, 0, sizeof(hudi_bluetooth_ac6956cgx_frame_t));
    unsigned char *offset = (unsigned char *)buf;
    unsigned int temp = 0, temp1 = 0;

    frame.frame_head = *offset++;
    if (frame.frame_head != 0xac)
    {
        return ret;
    }

    frame.frame_head <<= 8;
    frame.frame_head |= (*offset++);

    if (frame.frame_head != 0xac69)
    {
        return ret;
    }
    frame.frame_len = *offset++;
    temp = *offset++;
    frame.frame_len |= (temp >> 8);
    if (frame.frame_len != len)
    {
        return 1;
    }
    frame.frame_id = *offset++;
    frame.cmd_id = *offset++;
    frame.cmd_len = *offset++;
    if (frame.cmd_len >= PROTO_FRAME_CMD_SIZE)
    {
        return ret;
    }
    memcpy(frame.cmd_value, offset, frame.cmd_len);
    offset += frame.cmd_len;
    _hudi_bluetooth_ac6956cgx_checksum_calc(&frame);
    temp = 0;
    temp1 = (*offset++) & 0x00ff;
    temp = *offset;
    temp <<= 8;
    temp += temp1;
    if (frame.checksum == temp)
    {
        ret = 0;
    }
    else
    {
        ret = -1;
    }
    return ret;
}

static int _hudi_bluetooth_ac6956cgx_frame_fmt_package(char *buf, int len, hudi_bluetooth_ac6956cgx_frame_t *frame)
{
    unsigned char *offset = (unsigned char *)buf;
    unsigned int temp = 0, temp1 = 0;
    if (frame == NULL || buf == NULL || len <= 0)
        return -1;
    frame->frame_head = *offset++;
    frame->frame_head <<= 8;
    frame->frame_head += (*offset++);
    if (frame->frame_head != 0xac69)
    {
        return -1;
    }

    frame->frame_len = *offset++;
    temp = *offset++;
    frame->frame_len |= (temp >> 8);
    frame->frame_id = *offset++;
    frame->cmd_id = *offset++;
    frame->cmd_len = *offset++;
    if (frame->cmd_len + 9 >= UART_RX_BUFFER_SIZE || frame->cmd_len >= PROTO_FRAME_CMD_SIZE)
    {
        return -1;
    }

    memcpy(frame->cmd_value, offset, frame->cmd_len);
    offset += frame->cmd_len;
    _hudi_bluetooth_ac6956cgx_checksum_calc(frame);
    temp = 0;
    temp1 = (*offset++) & 0x00ff;
    temp = *offset;
    temp <<= 8;
    temp += temp1;
    if (frame->checksum == temp)
        return 0;
    else
        return -1;
}

static int _hudi_bluetooth_ac6956cgx_frame_fmt_parse2buf(hudi_bluetooth_ac6956cgx_frame_t *frame, char *buf)
{
    *buf++ = frame->frame_head >> 8;
    *buf++ = frame->frame_head & 0x00ff;
    *buf++ = frame->frame_len & 0x00ff;
    *buf++ = frame->frame_len >> 8;
    *buf++ = frame->frame_id;
    *buf++ = frame->cmd_id;
    *buf++ = frame->cmd_len;
    memcpy(buf, frame->cmd_value, frame->cmd_len);
    buf += frame->cmd_len;
    *buf++ = frame->checksum & 0x00ff;
    *buf = frame->checksum >> 8;

    return 0;
}

static int _hudi_bluetooth_ac6956cgx_proto_write(hudi_handle handle, hudi_bluetooth_ac6956cgx_cmd_id_e proto_cmd, char *cmd_value)
{
    char buf[1024] = {0};
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    hudi_bluetooth_module_st *module = inst->module;
    hudi_bluetooth_ac6956cgx_frame_t frame = {0};


    switch (proto_cmd)
    {
        case CMD_SET_BT_POWER_ON:
            frame.cmd_len = 0;
            break;
        case CMD_SET_INQUIRY_START:
            frame.cmd_len = 0;
            break;
        case CMD_SET_CONNECT:
            frame.cmd_len = 0x06;
            break;
        case CMD_SET_BT_POWER_OFF:
            module->status = HUDI_BLUETOOTH_STATUS_NONE;
            break;
        case CMD_SET_MEMORY_CONNECTION:
            frame.cmd_len = 0x01;
            break;
        case CMD_SET_BT_PIN_FUNCTION:
            frame.cmd_len = 0x02;
            break;
        case CMD_SET_BT_GPIO_POL:
            frame.cmd_len = 0x02;
            break;
        case CMD_SET_BT_IR_POWER_KEY:
            frame.cmd_len = 0x02;
            break;
        case CMD_SET_IR_USERCODE:
            frame.cmd_len = 0x02;
            break;
        case CMD_SET_AUDIO_CHANNEL_INPUT_SELECT:
            frame.cmd_len = 0x01;
            break;
        case CMD_SET_BT_PWM_PARAM:
            frame.cmd_len = 0x03;
            break;
        case CMD_SET_BT_CTRL_CMD:
            frame.cmd_len = 0x01;
            break;
        case CMD_SET_BT_MUSIC_VOL_VALUE:
            frame.cmd_len = 0x01;
            break;
        case CMD_SET_BT_POWER_ON_TO_RX:
            module->mode = HUDI_BLUETOOTH_MODE_SLAVE;
            break;
        case CMD_SET_BT_POWER_ON_TO_TX:
            module->mode = HUDI_BLUETOOTH_MODE_MASTER;
            break;
        case CMD_SET_BT_LOCAL_NAME:
            frame.cmd_len = strlen(cmd_value) + 1;
            break;
        case CMD_SET_CHANNEL_MAP:
            frame.cmd_len = 0x0a;
            break;
        default:
            break;
    }

    frame.frame_head = 0xac69;
    frame.frame_len = 9 + frame.cmd_len;
    frame.frame_id = 0x00;
    memcpy(frame.cmd_value, cmd_value, frame.cmd_len);
    frame.cmd_id = proto_cmd;
    _hudi_bluetooth_ac6956cgx_checksum_calc(&frame);

    _hudi_bluetooth_ac6956cgx_frame_fmt_parse2buf(&frame, buf);
    return hudi_bluetooth_write(handle, buf, frame.frame_len);
}

/* just parse a frame */
static int _hudi_bluetooth_ac6956cgx_frame_parse(hudi_handle handle, char *buf, int len)
{
    int ret = -1;
    int temp_value = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *) handle;
    void *user_data = inst->user_data;
    hudi_bluetooth_ac6956cgx_frame_t frame = {0};
    hudi_bluetooth_module_st *module = inst->module;
    hudi_bluetooth_device_t *connected_data = &module->connected_res;
    hudi_bluetooth_device_t *inquiry_data = &module->inquiry_res;
    hudi_bluetooth_extio_input_t *input_pinset = &module->input_pinset;
    hudi_bluetooth_input_event_t key_event = {0};

    if (!inst || !inst->notifier)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth instance error\n");
        return ret;
    }

    ret = _hudi_bluetooth_ac6956cgx_proto_framecheck(buf, len);
    if (ret == -1)
    {
        hudi_log(HUDI_LL_DEBUG, "Protocal frame check error\n");
        return ret;
    }

    ret = _hudi_bluetooth_ac6956cgx_frame_fmt_package(buf, len, &frame);
    if (ret == -1)
    {
        hudi_log(HUDI_LL_ERROR, "Frame format package error\n");
        return ret;
    }
    _hudi_bluetooth_sem_init();

    switch (frame.cmd_id)
    {
        case CMD_REPORT_CONNECT_STATUS:
            if (frame.cmd_value[0] == DEVICE_STATUS_CONNECTED)
            {
                module->status = HUDI_BLUETOOTH_STATUS_CONNECTED;
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_DEV_STATUS_CONNECTED, NULL, user_data);
            }
            else
            {
                module->status = HUDI_BLUETOOTH_STATUS_DISCONNECTED;
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_DEV_STATUS_DISCONNECTED, NULL, user_data);
            }
            break;
        case CMD_REPORT_CONNECT_RESULTS:
            memset(connected_data, 0, sizeof(hudi_bluetooth_device_t));
            memcpy(connected_data->mac, frame.cmd_value, BLUETOOTH_MAC_LEN);
            if (frame.cmd_len - BLUETOOTH_MAC_LEN < BLUETOOTH_NAME_LEN)
            {
                memcpy(connected_data->name, &frame.cmd_value[BLUETOOTH_MAC_LEN], frame.cmd_len - BLUETOOTH_MAC_LEN);
            }
            else
            {
                memcpy(connected_data->name, &frame.cmd_value[BLUETOOTH_MAC_LEN], BLUETOOTH_NAME_LEN);
                connected_data->name[BLUETOOTH_NAME_LEN - 1] = 0;
                hudi_log(HUDI_LL_WARNING, "Bluetooth device name too long \n");
            }

            if (module->mode == HUDI_BLUETOOTH_MODE_SLAVE)
            {
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_CONNECT_RES, (void *)connected_data, user_data);
            }
            else
            {
                temp_value = 1;
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_DEV_STATUS_CONNECTED, NULL, user_data);
                module->status = HUDI_BLUETOOTH_STATUS_CONNECTED;
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_CONNECT_RES, (void *)connected_data, user_data);
            }
            break;
        case CMD_REPORT_INQUIRY_RESULTS:
            /* special logic pay attention */
            if (module->mode == HUDI_BLUETOOTH_MODE_MASTER)
            {
                memset(inquiry_data, 0, sizeof(hudi_bluetooth_device_t));
                int name_cnt = frame.cmd_len - BLUETOOTH_MAC_LEN - 1;
                memcpy(inquiry_data->mac, frame.cmd_value, BLUETOOTH_MAC_LEN);
                if (name_cnt < BLUETOOTH_NAME_LEN)
                {
                    memcpy(inquiry_data->name, &frame.cmd_value[BLUETOOTH_MAC_LEN + 1], name_cnt);
                }
                else
                {
                    memcpy(inquiry_data->name, &frame.cmd_value[BLUETOOTH_MAC_LEN + 1], BLUETOOTH_NAME_LEN);
                    inquiry_data->name[BLUETOOTH_NAME_LEN - 1] = 0;
                    hudi_log(HUDI_LL_WARNING, "Bluetooth device name too long \n");
                }

                module->status = HUDI_BLUETOOTH_STATUS_INQUIRY;
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_INQUIRY_RES, (void *)inquiry_data, user_data);
            }
            break;
        case CMD_REPORT_DISCONNECT_RESULTS:
            if (module->mode == HUDI_BLUETOOTH_MODE_MASTER)
            {
                module->status = HUDI_BLUETOOTH_STATUS_DISCONNECTED;
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_DISCONNECT_RES, NULL, user_data);
            }
            break;
        case CMD_REPORT_INQUIRY_COMPLETE:
            if (module->mode == HUDI_BLUETOOTH_MODE_MASTER)
            {
                module->status = HUDI_BLUETOOTH_STATUS_INQUIRY_COMPLETE;
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_INQUIRY_COMPLETE, NULL, user_data);
            }
            break;
        case CMD_REPORT_BT_GPI_STAT:
            memset(input_pinset, 0, sizeof(hudi_bluetooth_extio_input_t));
            memcpy(&input_pinset->pinpad, frame.cmd_value, 1);
            memcpy(&input_pinset->value, frame.cmd_value + 1, 1);
            //_hudi_bluetooth_sem_post();
            inst->notifier(handle, HUDI_BLUETOOTH_EVT_GPI_STATUS, (void *)input_pinset, user_data);
            break;
        case CMD_REPORT_BT_IR_KEY:
            //cmd_value =type(1B IR/ADC)+keycode(4B)+ status(repeat/press/release)(1B)
            temp_value = frame.cmd_value[1];
            key_event.code = _hudi_bluetooth_reportkey_mapping(BT_IR_KEY, temp_value);
            if (frame.cmd_value[5] == 0x00)
            {
                key_event.value = 1;
            }
            else if (frame.cmd_value[5] == 0x02)
            {
                key_event.value = 0; // report real value or def by user ?
            }
            else
            {
                key_event.value = frame.cmd_value[5];
            }
            hudi_bluetooth_input_event_send(key_event);
            break;
        case CMD_REPORT_BT_ADC_KEY:
            // cmd_value = type(1B IR/ADC)+ir adckey_volatage(2B)+ status(press/repeat/release)(1B)
            temp_value = frame.cmd_value[2] << 8;
            temp_value |= frame.cmd_value[1];
            key_event.code = _hudi_bluetooth_adckey_voltage_mapping(inst, temp_value);
            if (frame.cmd_value[3] == 0x00)
            {
                key_event.value = 1;
            }
            else if (frame.cmd_value[3] == 0x02)
            {
                key_event.value = 0; // report real value or def by user ?
            }
            else
            {
                key_event.value = frame.cmd_value[3];
            }
            hudi_bluetooth_input_event_send(key_event);
            break;
        case CMD_SET_BT_CMD_ACK:
            /* only for bluetooth cmd respone */
            break;
        case CMD_REPORT_AUDIO_CHANNEL_INPUT_SELECT:
            memcpy(&module->audio_input_ch, frame.cmd_value, 1);
            _hudi_bluetooth_sem_post();
            break;
        case CMD_REPORT_BT_MUSIC_VOL_RESULTS:
            frame.cmd_value[0] = frame.cmd_value[0] * 3;
            memcpy(&module->audio_volume, frame.cmd_value, 1);
            _hudi_bluetooth_sem_post();
            break;
        case CMD_REPORT_BT_LOCAL_NAME:
            memset(module->device_name, 0, BLUETOOTH_LOCAL_NAME_LEN);
            memcpy(module->device_name, frame.cmd_value, frame.cmd_len);
            _hudi_bluetooth_sem_post();
            break;
        case CMD_REPORT_BT_VERSION:
            memset(module->firmware_ver, 0, 32);
            memcpy(module->firmware_ver, frame.cmd_value, frame.cmd_len);
            _hudi_bluetooth_sem_post();
            break;
        case CMD_REPORT_CHANNEL_MAP:
            memset(module->signal_channelmap, 0, sizeof(module->signal_channelmap));
            memcpy(module->signal_channelmap, frame.cmd_value, frame.cmd_len);
            _hudi_bluetooth_sem_post();
            break;
        default :
            break;
    }

    return ret;

}

static int hudi_bluetooth_ac6956cgx_proto_parse(hudi_handle handle, char *buf, int len)
{
    unsigned short frame_head = 0;
    int ret = -1;
    int frame_len = 0;
    int offset = 0;
    unsigned char *p_offset = NULL;

    while (offset != len)
    {
        p_offset = buf;
        frame_head = *p_offset & 0x00ff;
        p_offset++;
        if (frame_head != 0xac)
        {
            return ret;
        }

        frame_head <<= 8;
        frame_head |= *p_offset & 0x00ff;
        p_offset++;
        if (frame_head != 0xac69)
        {
            return ret;
        }
        frame_len = *p_offset;

        _hudi_bluetooth_ac6956cgx_frame_parse(handle, buf, frame_len);
        offset += frame_len;
        buf += frame_len;
    }

    return 0;
}

static int hudi_bluetooth_ac6956cgx_rf_on(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_POWER_ON, NULL);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_rf_off(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_POWER_OFF, NULL);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_scan_async(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_INQUIRY_START, NULL);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_scan_abort(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_INQUIRY_STOP, NULL);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_connect(hudi_handle handle, char *device_mac)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_CONNECT, device_mac);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_disconnect(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_DISCONNECT, NULL);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_ignore(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_DELETE_ALL_DEVICE, NULL);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_volume_set(hudi_handle handle, int vol)
{
    int ret = -1;
    uint8_t value = (uint8_t)vol;
    char send_buf[1] = {0};
    value = value / 3 >= BLUETOOTH_MAX_VOLUME_RANGE ? BLUETOOTH_MAX_VOLUME_RANGE : value / 3;
    send_buf[0] = value;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_MUSIC_VOL_VALUE, send_buf);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_volume_get(hudi_handle handle, int *vol)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_GET_BT_MUSIC_VOL_RESULTS, NULL);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "AC6956CGX protocal write error\n");
        return ret;
    }

    ret = _hudi_bluetooth_sem_timewait(200);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "AC6956CGX protocal cmd response timeout \n");
        return ret;
    }

    *vol = inst->module->audio_volume;
    return ret;
}


static int hudi_bluetooth_ac6956cgx_audio_input_ch_set(hudi_handle handle, hudi_bluetooth_audio_ch_e  ch)
{
    int ret = -1;
    uint8_t value = 0;
    value = ch == HUDI_BLUETOOTH_AUDIO_CH_LINEIN ? 0 : 1 ;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_AUDIO_CHANNEL_INPUT_SELECT, (char *) &value);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_audio_input_ch_get(hudi_handle handle, hudi_bluetooth_audio_ch_e   *ch)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_GET_AUDIO_CHANNEL_INPUT_SELECT, NULL);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "AC6956CGX protocal write error\n");
        return ret;
    }

    ret = _hudi_bluetooth_sem_timewait(200);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "AC6956CGX protocal cmd response timeout \n");
        return ret;
    }

    *ch = inst->module->audio_input_ch;
    return ret;
}

static int hudi_bluetooth_ac6956cgx_audio_output_ctrl(hudi_handle handle, hudi_bluetooth_audio_ctrl_e ctrl)
{
    int ret = -1;
    uint8_t value = 0;
    value = (uint8_t)ctrl;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_CTRL_CMD, (char *) &value);
    return ret ;
}

static int hudi_bluetooth_ac6956cgx_mode_switch(hudi_handle handle, hudi_bluetooth_mode_e mode)
{
    int ret = -1;
    if (mode == HUDI_BLUETOOTH_MODE_MASTER)
    {
        ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_POWER_ON_TO_TX, NULL);
    }
    else if (mode == HUDI_BLUETOOTH_MODE_SLAVE)
    {
        ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_POWER_ON_TO_RX, NULL);
    }
    return ret;
}

static int hudi_bluetooth_ac6956cgx_mode_get(hudi_handle handle, hudi_bluetooth_mode_e *mode)
{
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    *mode = inst->module->mode;
    return 0;
}

static int hudi_bluetooth_ac6956cgx_status_get(hudi_handle handle, hudi_bluetooth_status_e *sta)
{
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    *sta = inst->module->status;
    return 0;
}


static int hudi_bluetooth_ac6956cgx_localname_set(hudi_handle handle, char *name)
{
    int ret = -1;
    char soundbox_name[BLUETOOTH_LOCAL_NAME_LEN] = {0};
    if (name == NULL)
    {
        return ret;
    }
    memcpy(soundbox_name, name, strlen(name));
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_LOCAL_NAME, soundbox_name);
    return ret ;
}

static int hudi_bluetooth_ac6956cgx_localname_get(hudi_handle handle, char *name)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_GET_BT_LOCAL_NAME, NULL);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "AC6956CGX protocal write error\n");
        return ret;
    }

    ret = _hudi_bluetooth_sem_timewait(200);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "AC6956CGX protocal cmd response timeout \n");
        return ret;
    }

    memcpy(name, inst->module->device_name, sizeof(inst->module->device_name));
    return ret;
}


static int hudi_bluetooth_ac6956cgx_firmware_ver_get(hudi_handle handle, char *version)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_GET_BT_VERSION, NULL);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "AC6956CGX protocal write error\n");
        return ret;
    }

    ret = _hudi_bluetooth_sem_timewait(200);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "AC6956CGX protocal cmd response timeout \n");
        return ret;
    }

    memcpy(version, inst->module->firmware_ver, sizeof(inst->module->firmware_ver));
    return ret;

}

static int hudi_bluetooth_ac6956cgx_extio_func_set(hudi_handle handle, int pinpad, hudi_bluetooth_extio_func_e func)
{
    int ret = -1;
    char write_buf[2] = {0};
    write_buf[0] = (char)pinpad;
    write_buf[1] = (char)func;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_PIN_FUNCTION, write_buf);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_extio_input(hudi_handle handle, int pinpad, int *value)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    hudi_bluetooth_extio_input_t *input_gpio = &inst->module->input_pinset;
    if (pinpad == input_gpio->pinpad)
    {
        *value = input_gpio->value;
        ret = 0;
    }

    return ret;
}

static int hudi_bluetooth_ac6956cgx_extio_output(hudi_handle handle, int pinpad, int value)
{
    int ret = -1;
    char write_buf[2] = {0};
    write_buf[0] = (char)pinpad;
    write_buf[1] = (char)value;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_GPIO_POL, write_buf);

    return ret;
}

static int hudi_bluetooth_ac6956cgx_extio_pwm_set(hudi_handle handle, int pinpad, int freq, int duty)
{
    int ret = -1;
    int duty_value = 0;
    char buf[3] = {0};
    char buf1[3] = {0};

    /* set pwm frquency param */
    if (freq > 100 || duty > 100)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid intput param\n");
        return ret;
    }
    buf[0] = (char)pinpad;
    buf[1] = 0;
    buf[2] = (char)freq;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_PWM_PARAM, buf);

    /* set pwm duty param */
    /*pwm duty polarity is 0 ,so convert here*/
    duty_value = 100 - duty;
    buf1[0] = (char)pinpad;
    buf1[1] = 1;
    buf1[2] = (char)duty_value;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_PWM_PARAM, buf1);

    return ret;
}

static int hudi_bluetooth_ac6956cgx_standby_key_set(hudi_handle handle, int standby_key)
{
    int ret = -1;
    char write_buf[2] = {0};
    write_buf[1] = standby_key & 0xff;
    write_buf[0] = (standby_key >> 8) & 0xff;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_IR_POWER_KEY, write_buf);
    return ret;

}

static int hudi_bluetooth_ac6956cgx_standby_enter(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_POWER_PINPAD_OFF, NULL);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_reboot(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_RESET, NULL);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_ir_usercode_set(hudi_handle handle, int usercode)
{
    int ret = -1;
    char send_buf[2] = {0};
    send_buf[0] = (usercode >> 8) & 0xff;
    send_buf[1] = usercode & 0xff;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_IR_USERCODE, send_buf);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_signal_channel_set(hudi_handle handle, char *bitmap)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_CHANNEL_MAP, bitmap);
    return ret;
}

static int hudi_bluetooth_ac6956cgx_signal_channel_get(hudi_handle handle, char *bitmap)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_GET_CHANNEL_MAP, NULL);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "AC6956CGX protocal write error\n");
        return ret;
    }

    ret = _hudi_bluetooth_sem_timewait(200);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "AC6956CGX protocal cmd response timeout \n");
        return ret;
    }

    memcpy(bitmap, inst->module->signal_channelmap, sizeof(inst->module->signal_channelmap));
    return ret;
}

static int hudi_bluetooth_ac6956cgx_extio_standby_status_set(hudi_handle handle, int pinpad, int value)
{
    int ret = -1;
    char write_buf[2] = {0};
    write_buf[0] = (char)pinpad;
    write_buf[1] = (char)value;
    ret = _hudi_bluetooth_ac6956cgx_proto_write(handle, CMD_SET_BT_DEFLAULT_IO, write_buf);
    return ret;
}

static int _hudi_bluetooth_ac6956cgx_configure(hudi_handle handle)
{
    ac6956cgx_dts_resource_t *node_attr = NULL;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *) handle;

    _hudi_bluetooth_ac6956cgx_dts_parse(handle);
    node_attr = (ac6956cgx_dts_resource_t *)inst->dts_res.priv;

    /* hudi bluetooth uart setup before bluetooth write */
    hudi_bluetooth_uart_configure(inst->fd, inst->module->baudrate);

    hudi_bluetooth_ac6956cgx_ir_usercode_set(handle, node_attr->ir_usercode);
    hudi_bluetooth_ac6956cgx_standby_key_set(handle, node_attr->ir_powerkey_code);

    hudi_bluetooth_ac6956cgx_audio_input_ch_set(handle, HUDI_BLUETOOTH_AUDIO_CH_SPDIF);

    /* bluetooth firmware will close audio output to avoid sonic boom
     * so open audio output when open this module */
    hudi_bluetooth_ac6956cgx_audio_output_ctrl(handle, HUDI_BLUETOOTH_AUDIO_CTRL_UNMUTE);

    return 0;
}

hudi_bluetooth_module_st m_ac6956cgx_module_inst =
{
    .device_id = "AC6956CGX",
    .baudrate = 115200,
    .proto_parse = hudi_bluetooth_ac6956cgx_proto_parse,
    .rf_on = hudi_bluetooth_ac6956cgx_rf_on,
    .rf_off = hudi_bluetooth_ac6956cgx_rf_off,
    .scan_async = hudi_bluetooth_ac6956cgx_scan_async,
    .scan_abort = hudi_bluetooth_ac6956cgx_scan_abort,
    .connect = hudi_bluetooth_ac6956cgx_connect,
    .disconnect = hudi_bluetooth_ac6956cgx_disconnect,
    .ignore = hudi_bluetooth_ac6956cgx_ignore,
    .volume_set = hudi_bluetooth_ac6956cgx_volume_set,
    .volume_get = hudi_bluetooth_ac6956cgx_volume_get,
    .audio_input_ch_set = hudi_bluetooth_ac6956cgx_audio_input_ch_set,
    .audio_input_ch_get = hudi_bluetooth_ac6956cgx_audio_input_ch_get,
    .audio_output_ctrl = hudi_bluetooth_ac6956cgx_audio_output_ctrl,
    .mode_switch = hudi_bluetooth_ac6956cgx_mode_switch,
    .mode_get = hudi_bluetooth_ac6956cgx_mode_get,
    .status_get = hudi_bluetooth_ac6956cgx_status_get,
    .localname_set = hudi_bluetooth_ac6956cgx_localname_set,
    .localname_get = hudi_bluetooth_ac6956cgx_localname_get,
    .firmware_ver_get = hudi_bluetooth_ac6956cgx_firmware_ver_get,
    .extio_func_set = hudi_bluetooth_ac6956cgx_extio_func_set,
    .extio_input = hudi_bluetooth_ac6956cgx_extio_input,
    .extio_output = hudi_bluetooth_ac6956cgx_extio_output,
    .extio_pwm_set = hudi_bluetooth_ac6956cgx_extio_pwm_set,
    .standby_key_set = hudi_bluetooth_ac6956cgx_standby_key_set,
    .standby_enter = hudi_bluetooth_ac6956cgx_standby_enter,
    .reboot = hudi_bluetooth_ac6956cgx_reboot,
    .ir_usercode_set = hudi_bluetooth_ac6956cgx_ir_usercode_set,
    .signal_ch_set = hudi_bluetooth_ac6956cgx_signal_channel_set,
    .signal_ch_get = hudi_bluetooth_ac6956cgx_signal_channel_get,
    .extio_standby_status_set = hudi_bluetooth_ac6956cgx_extio_standby_status_set,
};

int hudi_bluetooth_ac6956cgx_attach(hudi_handle handle)
{
    hudi_bluetooth_module_register(handle, &m_ac6956cgx_module_inst);
    _hudi_bluetooth_ac6956cgx_configure(handle);
    return 0;
}
