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


#define BLUETOOTH_TYPE_LEN 4
typedef enum
{
    CMD_SET_BT_POWER_ON = 0x00,
    CMD_SET_INQUIRY_START = 0x01,
    CMD_SET_INQUIRY_STOP = 0x02,
    CMD_GET_CONNECT_INFO = 0x03,
    CMD_SET_DISCONNECT = 0x04,
    CMD_SET_POWER_ON_TO_RX = 0x05,
    CMD_SET_BT_POWER_OFF = 0x06,
    CMD_GET_BT_CONNECT_STATE = 0x07,
    CMD_SET_DELETE_LAST_DEVICE = 0x08,
    CMD_SET_DELETE_ALL_DEVICE = 0x09,
    CMD_SET_BT_POWER_ON_2 = 0x0a,      /* no user call */
    CMD_SET_CHANNEL_MAP = 0x0b,
    CMD_GET_CHANNEL_MAP = 0x11,
    CMD_GET_VERSION = 0x0d,
    CMD_SET_RESET = 0x21,
    CMD_SET_WILDCARD = 0x88,
    CMD_SET_CONNECT = 0xdd,
    CMD_SET_SHIELDED_CHANNEL_MAP = 0xfb,
} hudi_bluetooth_ac6955fgx_write_cmd_id_e;

typedef enum
{
    CMD_GET_CONNECT_STATUS = 0x00,
    CMD_GET_ACK_OK = 0x01,
    CMD_GET_SEARCHING = 0x02,
    CMD_GET_RECONNECTING = 0x03,
    CMD_GET_TIMED_OUT = 0x04,
    CMD_GET_BT_INIT_OK = 0x05,
    CMD_GET_BT_VERSOION = 0x0C,
    CMD_REPORT_CHANNEL_MAP = 0x10,
    CMD_GET_CONNECTED = 0X66,
    CMD_GET_INQUIRY_STOP_SEARCH = 0x70,
    CMD_GET_DISCONNECTED = 0x99,
    CMD_GET_INQUIRY = 0xcc,
    CMD_GET_WILDCARD = 0xfa,
    CMD_GET_RECONNECTED_MAC_NAME = 0xfd,
} hudi_bluetooth_ac6955fgx_read_cmd_id_e;


typedef struct
{
    unsigned short frame_head;
    unsigned char cmd_id;
    unsigned char cmd_len;
    unsigned char cmd_value[128];
    unsigned char checksum;
} hudi_bluetooth_ac6955fgx_frame_t;


#ifdef __HCRTOS__

#include <kernel/completion.h>
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

#endif


static void _hudi_bluetooth_ac6955fgx_checksum_calc(hudi_bluetooth_ac6955fgx_frame_t *frame)
{
    unsigned char sum = 0;
    if (frame == NULL)
        return;
    sum += (frame->frame_head & 0x00ff);
    sum += (frame->frame_head >> 8);
    sum += frame->cmd_id;
    sum += frame->cmd_len;
    if (frame->cmd_id != CMD_SET_WILDCARD && frame->cmd_id != CMD_GET_WILDCARD)
    {
        for (int i = 0; i < frame->cmd_len; i++)
        {
            sum += frame->cmd_value[i];
        }
    }
    frame->checksum = sum;
}

static int _hudi_bluetooth_ac6955fgx_proto_framecheck(char *buf, int len)
{
    int ret = -1;
    hudi_bluetooth_ac6955fgx_frame_t frame = {0};
    unsigned char *offset = (unsigned char *)buf;
    unsigned char temp = 0;

    frame.frame_head = *offset++;
    if (frame.frame_head != 0X55)
    {
        return ret;
    }

    frame.frame_head <<= 8;
    frame.frame_head |= (*offset++);
    if (frame.frame_head != 0X55AA)
    {
        return ret;
    }
    frame.cmd_id = *offset++;
    frame.cmd_len = *offset++;

    if (frame.cmd_id != CMD_GET_WILDCARD && frame.cmd_id != CMD_SET_WILDCARD)
    {
        if (frame.cmd_len >= PROTO_FRAME_CMD_SIZE)
        {
            return ret;
        }
    }

    if (frame.cmd_id != CMD_GET_WILDCARD && frame.cmd_id != CMD_SET_WILDCARD)
    {
        if (frame.cmd_len + 5 > len)
        {
            return 1;
        }
        if (frame.cmd_len)
            memcpy(frame.cmd_value, offset++, frame.cmd_len);
    }

    _hudi_bluetooth_ac6955fgx_checksum_calc(&frame);
    temp = buf[len - 1];
    if (frame.checksum == temp)
    {
        return 0;
    }
    else
    {
        return -1;
    }

    return ret;
}

static int _hudi_bluetooth_ac6955fgx_frame_fmt_package(char *buf, int len, hudi_bluetooth_ac6955fgx_frame_t *frame)
{
    int ret = -1;
    unsigned char *offset = (unsigned char *)buf;
    unsigned char temp = 0;

    frame->frame_head = *offset++;
    if (frame->frame_head != 0X55)
    {
        return ret;
    }

    frame->frame_head <<= 8;
    frame->frame_head |= (*offset++);
    if (frame->frame_head != 0X55AA)
    {
        return ret;
    }
    frame->cmd_id = *offset++;
    frame->cmd_len = *offset++;

    if (frame->cmd_id != CMD_GET_WILDCARD && frame->cmd_id != CMD_SET_WILDCARD)
    {
        if (frame->cmd_len + 5 > len)
        {
            return ret;
        }
        if (frame->cmd_len)
            memcpy(frame->cmd_value, offset++, frame->cmd_len);
    }

    _hudi_bluetooth_ac6955fgx_checksum_calc(frame);
    return 0;


}

static int _hudi_bluetooth_ac6955fgx_frame_fmt_parse2buf(hudi_bluetooth_ac6955fgx_frame_t *frame, char *buf)
{
    unsigned char *offset = buf;

    if (frame->cmd_id == CMD_SET_CONNECT && frame->cmd_len + 9 >= UART_RX_BUFFER_SIZE)
    {
        return -1;
    }
    _hudi_bluetooth_ac6955fgx_checksum_calc(frame);
    *offset++ = frame->frame_head >> 8;
    *offset++ = frame->frame_head & 0x00ff;
    *offset++ = frame->cmd_id;
    *offset++ = frame->cmd_len;

    if (frame->cmd_id != CMD_SET_WILDCARD && frame->cmd_id != CMD_GET_WILDCARD)
    {
        memcpy(offset, frame->cmd_value, frame->cmd_len);
        buf[frame->cmd_len + 5 - 1] = frame->checksum;
    }
    else
        *offset = frame->checksum;



    return 0;
}

static int _hudi_bluetooth_ac6955fgx_proto_write(hudi_handle handle, int proto_cmd, char *cmd_value)
{
    char buf[1024] = {0};
    int buf_len = 0;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    hudi_bluetooth_module_st *module = inst->module;
    hudi_bluetooth_ac6955fgx_frame_t frame = {0};


    switch (proto_cmd)
    {
        case CMD_SET_BT_POWER_ON:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = 0x01;
            break;
        case CMD_SET_INQUIRY_START:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = 0x08;
            break;
        case CMD_SET_INQUIRY_STOP:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = 0x09;
            break;
        case CMD_SET_CONNECT:
            memcpy(frame.cmd_value, cmd_value, 6);
            frame.cmd_id = CMD_SET_CONNECT;
            frame.cmd_len = 0x06;
            break;
        case CMD_GET_BT_CONNECT_STATE:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = 0x10;
            break;
        case CMD_SET_DISCONNECT:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = 0x04;
            break;
        case CMD_SET_BT_POWER_OFF:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = 0x02;
            module->status = HUDI_BLUETOOTH_STATUS_NONE;
            break;
        case CMD_SET_DELETE_LAST_DEVICE:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = 0x20;
            break;
        case CMD_SET_DELETE_ALL_DEVICE:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = 0x20;
            break;
        case CMD_SET_POWER_ON_TO_RX:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = CMD_SET_POWER_ON_TO_RX;
            break;
        case CMD_SET_RESET:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = CMD_SET_RESET;
            break;
        case CMD_SET_BT_POWER_ON_2:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = CMD_SET_BT_POWER_ON_2;
            break;
        case CMD_SET_CHANNEL_MAP:
            frame.cmd_id = CMD_SET_CHANNEL_MAP;
            frame.cmd_len = 0x0a;
            memcpy(frame.cmd_value, cmd_value, frame.cmd_len);
            break;
        case CMD_GET_CHANNEL_MAP:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = CMD_GET_CHANNEL_MAP;
            break;
        case CMD_SET_SHIELDED_CHANNEL_MAP:
            frame.cmd_id = CMD_SET_SHIELDED_CHANNEL_MAP;
            frame.cmd_len = 0x01;
            memcpy(frame.cmd_value, cmd_value, frame.cmd_len);
            break;
        case CMD_GET_VERSION:
            frame.cmd_id = CMD_SET_WILDCARD;
            frame.cmd_len = CMD_GET_VERSION;
            break;
        default:
            break;
    }
    /*uart send data*/
    frame.frame_head = 0x55aa;
    _hudi_bluetooth_ac6955fgx_checksum_calc(&frame);
    _hudi_bluetooth_ac6955fgx_frame_fmt_parse2buf(&frame, buf);
    if (frame.cmd_id != CMD_SET_WILDCARD && frame.cmd_id != CMD_GET_WILDCARD)
    {
        buf_len = frame.cmd_len + 5;
    }
    else
    {
        buf_len = 5;
    }
    return hudi_bluetooth_write(handle, buf, buf_len);
}

/* just parse a frame */
static int _hudi_bluetooth_ac6955fgx_frame_parse(hudi_handle handle, char *buf, int len)
{
    int ret = -1;
    int name_cnt = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *) handle;
    void *user_data = inst->user_data;
    hudi_bluetooth_ac6955fgx_frame_t frame = {0};
    hudi_bluetooth_module_st *module = inst->module;
    hudi_bluetooth_device_t *connected_data = &module->connected_res;
    hudi_bluetooth_device_t *inquiry_data = &module->inquiry_res;
    hudi_bluetooth_input_event_t key_event = {0};

    if (!inst || !inst->notifier)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth instance error\n");
        return ret;
    }

    ret = _hudi_bluetooth_ac6955fgx_proto_framecheck(buf, len);
    if (ret == -1)
    {
        hudi_log(HUDI_LL_ERROR, "Protocal frame check error\n");
        return ret;
    }

    ret = _hudi_bluetooth_ac6955fgx_frame_fmt_package(buf, len, &frame);
    if (ret == -1)
    {
        hudi_log(HUDI_LL_ERROR, "Frame format package error\n");
        return ret;
    }
    _hudi_bluetooth_sem_init();

    if (frame.cmd_id == CMD_GET_WILDCARD)
    {
        switch (frame.cmd_len)
        {
            case CMD_GET_DISCONNECTED :
                module->status = HUDI_BLUETOOTH_STATUS_DISCONNECTED;
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_DISCONNECT_RES, NULL, user_data);
                break;

            case CMD_GET_CONNECTED :
                module->status = HUDI_BLUETOOTH_STATUS_CONNECTED;
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_DEV_STATUS_CONNECTED, NULL, user_data);
                break;

            case CMD_GET_INQUIRY_STOP_SEARCH :
                module->status = HUDI_BLUETOOTH_STATUS_INQUIRY_COMPLETE;
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_INQUIRY_COMPLETE, NULL, user_data);
                break;
            case CMD_GET_ACK_OK:
                break;

            case CMD_GET_TIMED_OUT:
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_CONNECT_TIMEOUT, NULL, user_data);
                break;

            case CMD_GET_RECONNECTING:
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_RECONNECTING, NULL, user_data);
                break;

            case CMD_GET_SEARCHING:
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_INQUIRING, NULL, user_data);
                break;
            case CMD_GET_BT_INIT_OK:
                break;

        }

    }

    switch (frame.cmd_id)
    {
        case CMD_GET_CONNECT_STATUS:
            memset(connected_data, 0, sizeof(hudi_bluetooth_device_t));
            memcpy(connected_data->mac, frame.cmd_value, BLUETOOTH_MAC_LEN);
            if (frame.cmd_len - BLUETOOTH_MAC_LEN - 1 < BLUETOOTH_NAME_LEN)
            {
                memcpy(connected_data->name, &frame.cmd_value[BLUETOOTH_MAC_LEN + 1], frame.cmd_len - BLUETOOTH_MAC_LEN - 1);
            }
            else
            {
                memcpy(connected_data->name, &frame.cmd_value[BLUETOOTH_MAC_LEN + 1], BLUETOOTH_NAME_LEN);
                connected_data->name[BLUETOOTH_NAME_LEN - 1] = 0;
                hudi_log(HUDI_LL_WARNING, "Bluetooth device name too long \n");
            }

            if (module->mode == HUDI_BLUETOOTH_MODE_SLAVE)
            {
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_CONNECT_RES, (void *)connected_data, user_data);
            }
            else
            {
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_DEV_STATUS_CONNECTED, NULL, user_data);
                module->status = HUDI_BLUETOOTH_STATUS_CONNECTED;
                inst->notifier(handle, HUDI_BLUETOOTH_EVT_CONNECT_RES, (void *)connected_data, user_data);
            }
            break;
        case CMD_GET_RECONNECTED_MAC_NAME:
            memset(connected_data, 0, sizeof(hudi_bluetooth_device_t));
            memcpy(connected_data->mac, frame.cmd_value, BLUETOOTH_MAC_LEN);
            if (frame.cmd_len - BLUETOOTH_MAC_LEN - 1 < BLUETOOTH_NAME_LEN)
            {
                memcpy(connected_data->name, &frame.cmd_value[BLUETOOTH_MAC_LEN + 1], frame.cmd_len - BLUETOOTH_MAC_LEN - 1);
            }
            else
            {
                memcpy(connected_data->name, &frame.cmd_value[BLUETOOTH_MAC_LEN + 1], BLUETOOTH_NAME_LEN);
                connected_data->name[BLUETOOTH_NAME_LEN - 1] = 0;
                hudi_log(HUDI_LL_WARNING, "Bluetooth device name too long \n");
            }
            module->status = HUDI_BLUETOOTH_STATUS_CONNECTED;
            inst->notifier(handle, HUDI_BLUETOOTH_EVT_RECONNECTED, (void *)connected_data, user_data);
            break;

        case CMD_GET_INQUIRY:
            memset(inquiry_data, 0, sizeof(hudi_bluetooth_device_t));
            name_cnt = frame.cmd_len - BLUETOOTH_MAC_LEN - BLUETOOTH_TYPE_LEN;
            memcpy(inquiry_data->mac, frame.cmd_value, BLUETOOTH_MAC_LEN);
            if (name_cnt < BLUETOOTH_NAME_LEN)
            {
                memcpy(inquiry_data->name, &frame.cmd_value[BLUETOOTH_MAC_LEN + BLUETOOTH_TYPE_LEN], name_cnt);
            }
            else
            {
                memcpy(inquiry_data->name, &frame.cmd_value[BLUETOOTH_MAC_LEN + BLUETOOTH_TYPE_LEN], BLUETOOTH_NAME_LEN);
                inquiry_data->name[BLUETOOTH_NAME_LEN - 1] = 0;
            }
            if (frame.cmd_len == 7)
            {
                break;
            }
            module->status = HUDI_BLUETOOTH_STATUS_INQUIRY;
            inst->notifier(handle, HUDI_BLUETOOTH_EVT_INQUIRY_RES, (void *)inquiry_data, user_data);
            break;
        case CMD_GET_BT_VERSOION:
            memset(module->firmware_ver, 0, 32);
            memcpy(module->firmware_ver, frame.cmd_value, frame.cmd_len);
            _hudi_bluetooth_sem_post();
            break;
        case CMD_REPORT_CHANNEL_MAP:
            memset(module->signal_channelmap, 0, sizeof(module->signal_channelmap));
            memcpy(module->signal_channelmap, frame.cmd_value, frame.cmd_len);
            _hudi_bluetooth_sem_post();
            break;
    }

    return ret;

}

static int hudi_bluetooth_ac6955fgx_proto_parse(hudi_handle handle, char *buf, int len)
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
        if (frame_head != 0x55)
        {
            return ret;
        }

        frame_head <<= 8;
        frame_head |= *p_offset & 0x00ff;
        p_offset++;
        if (frame_head != 0x55aa)
        {
            return ret;
        }
        if (*p_offset == CMD_SET_WILDCARD || *p_offset == CMD_GET_WILDCARD)
        {
            frame_len = 5;
        }
        else
        {
            p_offset++;
            frame_len = 5 + *p_offset;
        }

        _hudi_bluetooth_ac6955fgx_frame_parse(handle, buf, frame_len);
        offset += frame_len;
        buf += frame_len;
    }

    return 0;
}

static int hudi_bluetooth_ac6955fgx_rf_on(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_SET_BT_POWER_ON, NULL);
    return ret;
}

static int hudi_bluetooth_ac6955fgx_rf_off(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_SET_BT_POWER_OFF, NULL);
    return ret;
}

static int hudi_bluetooth_ac6955fgx_scan_async(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_SET_INQUIRY_START, NULL);
    return ret;
}

static int hudi_bluetooth_ac6955fgx_scan_abort(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_SET_INQUIRY_STOP, NULL);
    return ret;
}

static int hudi_bluetooth_ac6955fgx_connect(hudi_handle handle, char *device_mac)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_SET_CONNECT, device_mac);
    return ret;
}

static int hudi_bluetooth_ac6955fgx_disconnect(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_SET_DISCONNECT, NULL);
    return ret;
}

static int hudi_bluetooth_ac6955fgx_ignore(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_SET_DELETE_ALL_DEVICE, NULL);
    return ret;
}



static int hudi_bluetooth_ac6955fgx_mode_switch(hudi_handle handle, hudi_bluetooth_mode_e mode)
{
    int ret = -1;
    if (mode == HUDI_BLUETOOTH_MODE_MASTER)
    {
        ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_SET_BT_POWER_ON, NULL);
    }
    else if (mode == HUDI_BLUETOOTH_MODE_SLAVE)
    {
        ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_SET_POWER_ON_TO_RX, NULL);
    }
    return ret;
}

static int hudi_bluetooth_ac6955fgx_mode_get(hudi_handle handle, hudi_bluetooth_mode_e *mode)
{
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    *mode = inst->module->mode;
    return 0;
}

static int hudi_bluetooth_ac6955fgx_status_get(hudi_handle handle, hudi_bluetooth_status_e *sta)
{
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    *sta = inst->module->status;
    return 0;
}

static int hudi_bluetooth_ac6955fgx_firmware_ver_get(hudi_handle handle, char *version)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_GET_VERSION, NULL);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "ac6955fgx protocal write error\n");
        return ret;
    }

    ret = _hudi_bluetooth_sem_timewait(200);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "ac6955fgx protocal cmd response timeout \n");
        return ret;
    }

    memcpy(version, inst->module->firmware_ver, sizeof(inst->module->firmware_ver));
    return ret;

}

static int hudi_bluetooth_ac6955fgx_reboot(hudi_handle handle)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_SET_RESET, NULL);
    return ret;
}


static int hudi_bluetooth_ac6955fgx_signal_channel_set(hudi_handle handle, char *bitmap)
{
    int ret = -1;
    ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_SET_CHANNEL_MAP, bitmap);
    return ret;
}

static int hudi_bluetooth_ac6955fgx_signal_channel_get(hudi_handle handle, char *bitmap)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    ret = _hudi_bluetooth_ac6955fgx_proto_write(handle, CMD_GET_CHANNEL_MAP, NULL);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "ac6955fgx protocal write error\n");
        return ret;
    }

    ret = _hudi_bluetooth_sem_timewait(200);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "ac6955fgx protocal cmd response timeout \n");
        return ret;
    }

    memcpy(bitmap, inst->module->signal_channelmap, sizeof(inst->module->signal_channelmap));
    return ret;
}

static int _hudi_bluetooth_ac6955fgx_board_configure(hudi_handle handle)
{
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *) handle;
    hudi_bluetooth_uart_configure(inst->fd, inst->module->baudrate);
    return 0;
}

hudi_bluetooth_module_st m_ac6955fgx_module_inst =
{
    .device_id = "ac6955fgx",
    .baudrate = 115200,
    .proto_parse = hudi_bluetooth_ac6955fgx_proto_parse,
    .rf_on = hudi_bluetooth_ac6955fgx_rf_on,
    .rf_off = hudi_bluetooth_ac6955fgx_rf_off,
    .scan_async = hudi_bluetooth_ac6955fgx_scan_async,
    .scan_abort = hudi_bluetooth_ac6955fgx_scan_abort,
    .connect = hudi_bluetooth_ac6955fgx_connect,
    .disconnect = hudi_bluetooth_ac6955fgx_disconnect,
    .ignore = hudi_bluetooth_ac6955fgx_ignore,
    .mode_switch = hudi_bluetooth_ac6955fgx_mode_switch,
    .mode_get = hudi_bluetooth_ac6955fgx_mode_get,
    .status_get = hudi_bluetooth_ac6955fgx_status_get,
    .firmware_ver_get = hudi_bluetooth_ac6955fgx_firmware_ver_get,
    .reboot = hudi_bluetooth_ac6955fgx_reboot,
    .signal_ch_set = hudi_bluetooth_ac6955fgx_signal_channel_set,
    .signal_ch_get = hudi_bluetooth_ac6955fgx_signal_channel_get,
};

int hudi_bluetooth_ac6955fgx_attach(hudi_handle handle)
{
    hudi_bluetooth_module_register(handle, &m_ac6955fgx_module_inst);
    _hudi_bluetooth_ac6955fgx_board_configure(handle);
    return 0;
}

