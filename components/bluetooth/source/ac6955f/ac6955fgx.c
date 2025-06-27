#ifdef BLUETOOTH_AC6955F_GX
#ifdef HC_LINUX
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
#include "bluetooth.h"
#include <hcuapi/sci.h>
#include <poll.h>
#include <hcuapi/input-event-codes.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <termios.h>
static pthread_t bluetooth_thread_id = 0;
#else
#define LOG_TAG "BLUETOOTH"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

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
#include <hcuapi/sci.h>
#include <time.h>
#include <signal.h>
#include <kernel/completion.h>
#include "bluetooth.h"
#include <hcuapi/input-event-codes.h>
static struct completion bt_ac6955fgx_task_completion;
#endif
#include <stdarg.h>

#define BLUETOOTH_TEST
#ifdef BLUETOOTH_TEST
#define bt_printf(fmt, args...) printf("\033[32m[%s:%d]"#fmt" \033[0m\n", __func__, __LINE__, ##args);
#define bt_log_e(fmt, args...) printf("\033[31m[%s:%d]"#fmt" \033[0m\n", __func__, __LINE__, ##args);
#define bt_debuf_printf printf
#else
#define bt_printf
#define bt_debuf_printf
#define bt_log_e
#endif

#define BT_RET_EXIT     1
#define BT_DATA_AGAIN   2
#define BT_ERROR_DATA       0x100
#define BT_ERROR_DATA_OFFSET_1  0x101
#define BT_ERROR_DATA_OFFSET_2  0x102
#define BT_ERROR_DATA_MAX       0x103
#define UART_RX_RECEIVE_MAX_BUFF 1024
#define UART_TX_WRITE_MAX_BUFF 256

#define AC6955FGX_MAX_WRITE_SIZE          128
#define BLUETOOTH_DATE_TIMEOUT 1000
#define BLUETOOTH_MINIMUM_CHECKSUM 5
#define BT_FRAME_HEAD_ID_LOW 0X55
#define BT_FRAME_HEAD_ID 0X55AA
#define BT_SET_CMDS_ID_LEN_5 0x88
#define BT_REC_CMDS_ID_LEN_5 0xFA

typedef enum _E_BT_SCAN_STATUS_
{
    BT_SCAN_STATUS_DEFAULT = 0,
    BT_SCAN_STATUS_IS_SEARCHING,
    BT_SCAN_STATUS_GET_DATA_SEARCHED,
    BT_SCAN_STATUS_GET_DATA_FINISHED,
} bt_scan_status;

typedef enum _E_BT_CONNECT_STATUS_
{
    BT_CONNECT_STATUS_DEFAULT = 0,
    BT_CONNECT_STATUS_DISCONNECTED,
    BT_CONNECT_STATUS_TIMEOUT,
    BT_CONNECT_STATUS_RECONNECTING,
    BT_CONNECT_STATUS_CONNECTED,
    BT_CONNECT_STATUS_GET_CONNECTED_INFO,
} bt_connect_status_e;

typedef enum _E_BLUETOOTH_ID_GET_ID_
{
    BT_CMD_ID_GET_ACK_ERROR = 0x00,
    BT_CMD_ID_GET_CONNECT_STATUS = 0x00,
    BT_CMD_ID_GET_ACK_OK = 0x01,
    BT_CMD_ID_GET_SEARCHING = 0x02,
    BT_CMD_ID_GET_RECONNECTING = 0x03,
    BT_CMD_ID_GET_TIMED_OUT = 0x04,
    BT_CMD_ID_GET_BT_INIT_OK = 0x05,
    BT_CMD_ID_GET_BT_VERSOION = 0x0C,
    BT_CMD_ID_GET_CHANNEL_MAP = 0x10,
    BT_CMD_ID_GET_CONNECTED = 0X66,
    BT_CMD_ID_GET_INQUIRY_STOP_SEARCH = 0x70,
    BT_CMD_ID_GET_DISCONNECTED = 0x99,
    BT_CMD_ID_GET_INQUIRY = 0xCC,
    BT_CMD_ID_GET_RECONNECTED_MAC_NAME = 0xFD,
} bt_bluetooth_get_id;

typedef enum _E_BLUETOOTH_CMD_SENDS_
{
    BT_CMD_SET_BT_POWER_ON = 0,
    BT_CMD_SET_INQUIRY_START,
    BT_CMD_SET_INQUIRY_STOP,
    BT_CMD_GET_CONNECT_INFO,
    BT_CMD_SET_DISCONNECT,
    BT_CMD_SET_POWER_ON_TO_RX = 0x05,
    BT_CMD_SET_BT_POWER_OFF,
    BT_CMD_GET_BT_CONNECT_STATE,
    BT_CMD_SET_DELETE_LAST_DEVICE,
    BT_CMD_SET_DELETE_ALL_DEVICE,
    BT_CMD_SET_BT_POWER_ON_2 = 0x0a,
    BT_CMD_SET_CHANNEL_MAP = 0x0b,
    BT_CMD_GET_CHANNEL_MAP = 0x11,
    BT_CMD_GET_VERSION = 0x0d,
    BT_CMD_SET_RESET = 0x21,
    BT_CMD_SET_CONNECT = 0xDD,
    BT_CMD_SET_SHIELDED_CHANNEL_MAP = 0xfb,
} bt_ac6955fgx_cmd_sends_e;


struct AC6955FGX_MessageBody
{
    unsigned short frame_head;
    unsigned char cmd_id;
    unsigned char cmd_len;
    unsigned char cmd_value[AC6955FGX_MAX_WRITE_SIZE];
    unsigned char Frame_checksum;
};

struct bt_ac6955fgx_priv
{
    int uartfd;
    int cref;
    bluetooth_callback_t callback;
    struct bluetooth_slave_dev inquiry_info;
    struct bluetooth_slave_dev connet_info;
    bt_device_status_e get_dev_status;
    bt_ac6955fgx_cmd_sends_e bt_cmds;
    int bt_reset_flag;
};

static int bt_ac6955fgx_task_start_flag = 0;
static struct bt_ac6955fgx_priv *gbt = NULL;

static bt_device_status_e bt_get_device_sys_status(void)
{
    if (gbt == NULL)
        return EBT_DEVICE_STATUS_NOWORKING_DEFAULT;
    return gbt->get_dev_status;
}

static int bt_get_device_sys_connet_info(struct bluetooth_slave_dev *con_data)
{
    bt_device_status_e sys_status = bt_get_device_sys_status();
    if (gbt == NULL || con_data == NULL)
        return BT_RET_ERROR;
    if (sys_status == EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO)
    {
        memcpy(con_data, &gbt->connet_info,
               sizeof(struct bluetooth_slave_dev));
        return BT_RET_SUCCESS;
    }
    else if (sys_status < EBT_DEVICE_STATUS_WORKING_CONNECTED)
        return 1;

    return BT_RET_ERROR;
}

static void printf_bt_ac6955fgx_dev_status(bt_device_status_e status)
{
    switch (status)
    {
        case EBT_DEVICE_STATUS_NOWORKING_DEFAULT:
            bt_printf("EBT_DEVICE_STATUS_NOWORKING_DEFAULT\n");
            break;
        case EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR:
            bt_printf("EBT_DEVICE_STATUS_NOWORKING_STATUS_ERROR\n");
            break;
        case EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT:
            bt_printf("EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT\n");
            break;
        case EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF:
            bt_printf("EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_EXISTENT:
            bt_printf("EBT_DEVICE_STATUS_WORKING_EXISTENT\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED:
            bt_printf("EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING:
            bt_printf("EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_DISCONNECTED:
            bt_printf("EBT_DEVICE_STATUS_WORKING_DISCONNECTED\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_CONNECTED:
            bt_printf("EBT_DEVICE_STATUS_WORKING_CONNECTED\n");
            break;
        case EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO:
            bt_printf("EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO\n");
            break;
        case EBT_DEVICE_STATUS_NOWORKING_BT_RESET_OK:
            bt_printf("EBT_DEVICE_STATUS_NOWORKING_BT_RESET_OK\n");
            break;
        default:
            bt_printf("other\n");
            break;
    }
}

static void bt_printf_buf(unsigned char *buf, int count)
{
    bt_debuf_printf("\033[33m buf: ");
    for (int i = 0; i < count; i++)
        bt_debuf_printf("0x%02x ", buf[i]);
    bt_debuf_printf("\033[0m \n");
}

static void checksum_calculate(struct AC6955FGX_MessageBody *body)
{
    unsigned char sum = 0;
    if (body == NULL)
        return;
    sum += (body->frame_head & 0x00ff);
    sum += (body->frame_head >> 8);
    sum += body->cmd_id;
    sum += body->cmd_len;
    if (body->cmd_id != BT_REC_CMDS_ID_LEN_5 && body->cmd_id != BT_SET_CMDS_ID_LEN_5)
    {
        for (int i = 0; i < body->cmd_len; i++)
        {
            sum += body->cmd_value[i];
        }
    }
    body->Frame_checksum = sum;
}

static void bt_ac6955fgx_mes_printf(struct AC6955FGX_MessageBody *body)
{
    if (body == NULL)
        return;
    bt_debuf_printf("\033[33m");
    bt_debuf_printf("ad6956 a cmds : %02x %02x %02x %02x ",
                    (body->frame_head >> 8), (body->frame_head & 0x00ff), body->cmd_id, body->cmd_len);

    if (body->cmd_id != BT_REC_CMDS_ID_LEN_5 && body->cmd_id != BT_SET_CMDS_ID_LEN_5)
    {
        for (int i = 0; i < body->cmd_len; i++)
            bt_debuf_printf("%02x ", body->cmd_value[i]);
    }

    bt_debuf_printf("%02x \033[0m\n", body->Frame_checksum);
}

// static void bluetooth_inquiry_printf(struct bluetooth_slave_dev *data)
// {
//     printf("dev mac : ");
//     for(int i=0; i<6; i++)
//     printf("%02x ", data->mac[i]);
//     printf("\n");
//     printf("dev name %s\n",data->name);
// }

static int bt_ac6955fgx_messagebody_switch_str(struct AC6955FGX_MessageBody *body,
                                               unsigned char *buf)
{
    unsigned char *offset = buf;
    if (body == NULL)
        return BT_RET_ERROR;

    if (body->cmd_id == BT_CMD_SET_CONNECT && body->cmd_len + 9 >= UART_RX_RECEIVE_MAX_BUFF)
    {
        bt_printf("Buf is too long\n");
        return BT_RET_ERROR;
    }
    checksum_calculate(body);
    *offset++ = body->frame_head >> 8;
    *offset++ = body->frame_head & 0x00ff;
    *offset++ = body->cmd_id;
    *offset++ = body->cmd_len;

    if (body->cmd_id != BT_REC_CMDS_ID_LEN_5 && body->cmd_id != BT_SET_CMDS_ID_LEN_5)
    {
        memcpy(offset, body->cmd_value, body->cmd_len);
        buf[body->cmd_len + 5 - 1] = body->Frame_checksum;
    }
    else
        *offset = body->Frame_checksum;

    if (body->cmd_id != BT_REC_CMDS_ID_LEN_5 && body->cmd_id != BT_SET_CMDS_ID_LEN_5)
    {
        bt_printf_buf(buf, body->cmd_len + 5);
    }
    else
    {
        bt_printf_buf(buf, 5);
    }
    return BT_RET_SUCCESS;
}

static int str_with_ac6955fgx_messagebody_compare(unsigned char *buf, unsigned char count, struct AC6955FGX_MessageBody *body)
{
    // struct AC6955FGX_MessageBody body = {0};
    unsigned char *offset = (unsigned char*)buf;
    unsigned char temp = 0;

    body->frame_head = *offset++;
    if (body->frame_head != BT_FRAME_HEAD_ID_LOW)
    {
        bt_log_e("ac6955fgx frame head !=0x55 error body->frame_head = %d\n", body->frame_head);
        return BT_ERROR_DATA_OFFSET_1;
    }

    body->frame_head <<= 8;
    body->frame_head |= (*offset++);

    if (body->frame_head != BT_FRAME_HEAD_ID)
    {
        bt_log_e("ac6955fgx frame head !=0x55 aa error body->frame_head = %d\n", body->frame_head);
        return BT_ERROR_DATA_OFFSET_2;
    }
    body->cmd_id = *offset++;

    body->cmd_len = *offset++;

    if (body->cmd_id != BT_REC_CMDS_ID_LEN_5 && body->cmd_id != BT_SET_CMDS_ID_LEN_5)
    {
        if (body->cmd_len >= AC6955FGX_MAX_WRITE_SIZE)
        {
            bt_log_e("Cmd_len buf is too long\n");
            return BT_RET_ERROR;
        }
    }

    if (body->cmd_id != BT_REC_CMDS_ID_LEN_5 && body->cmd_id != BT_SET_CMDS_ID_LEN_5)
    {
        if (body->cmd_len + 5 > count)
        {
            return BT_DATA_AGAIN;
        }
        if (body->cmd_len)
            memcpy(body->cmd_value, offset++, body->cmd_len);
    }

    checksum_calculate(body);
    temp = buf[count - 1];

    bt_ac6955fgx_mes_printf(body);
    bt_printf("body->Frame_checksum =%04x temp= %04x\n", body->Frame_checksum, temp);
    if (body->Frame_checksum == temp)
    {
        return BT_RET_SUCCESS;
    }
    else
    {
        return BT_RET_ERROR;
    }
}

static int bt_ac6955fgx_set_action(bt_ac6955fgx_cmd_sends_e cmd,
                                   unsigned char *send_buf)
{
    struct AC6955FGX_MessageBody body_m = { 0 };
    unsigned char buf[UART_TX_WRITE_MAX_BUFF] = { 0 };
    int ret = BT_RET_SUCCESS;

    if (gbt == NULL)
    {
        bt_log_e("gbt = NULL \n");
        ret = BT_RET_ERROR;
        goto exit;
    }

    if (gbt->uartfd < 0)
    {
        bt_log_e("gbt->uartfd  =%d\n", gbt->uartfd);
        ret = BT_RET_ERROR;
        goto exit;
    }

    if (cmd != BT_CMD_SET_BT_POWER_ON)
    {
        gbt->get_dev_status = EBT_DEVICE_STATUS_NOWORKING_DEFAULT;
    }
    switch (cmd)
    {
        case BT_CMD_SET_BT_POWER_ON:
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = 0x01;
            if (gbt->get_dev_status < EBT_DEVICE_STATUS_WORKING_CONNECTED)
            {
                gbt->get_dev_status = EBT_DEVICE_STATUS_NOWORKING_DEFAULT;
            }
            break;
        case BT_CMD_SET_INQUIRY_START:
            memset(&gbt->inquiry_info, 0,
                   sizeof(struct bluetooth_slave_dev));
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = 0x08;
            break;
        case BT_CMD_SET_INQUIRY_STOP:
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = 0x09;
            break;
        case BT_CMD_SET_CONNECT:
            if (send_buf == NULL)
                goto error;
            memcpy(body_m.cmd_value, send_buf, 6);
            body_m.cmd_id = BT_CMD_SET_CONNECT;
            body_m.cmd_len = 0x06;
            break;
        case BT_CMD_GET_BT_CONNECT_STATE:
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = 0x10;
            break;
        case BT_CMD_SET_DISCONNECT:
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = 0x04;
            break;
        case BT_CMD_SET_BT_POWER_OFF:
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = 0x02;
            gbt->get_dev_status = EBT_DEVICE_STATUS_NOWORKING_NOT_EXISTENT;
            break;
        case BT_CMD_SET_DELETE_LAST_DEVICE:
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = 0x20;
            break;
        case BT_CMD_SET_DELETE_ALL_DEVICE:
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = 0x20;
            break;
        case BT_CMD_SET_POWER_ON_TO_RX:
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = BT_CMD_SET_POWER_ON_TO_RX;
            break;
        case BT_CMD_SET_RESET:
            gbt->bt_reset_flag = 1;
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = BT_CMD_SET_RESET;
            break;
        case BT_CMD_SET_BT_POWER_ON_2:
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = BT_CMD_SET_BT_POWER_ON_2;
            break;
        case BT_CMD_SET_CHANNEL_MAP:
            body_m.cmd_id = BT_CMD_SET_CHANNEL_MAP;
            body_m.cmd_len = 0x0a;
            if (send_buf == NULL)
                goto error;
            memcpy(body_m.cmd_value, send_buf, body_m.cmd_len);
            break;
        case BT_CMD_GET_CHANNEL_MAP:
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = BT_CMD_GET_CHANNEL_MAP;
            break;
        case BT_CMD_SET_SHIELDED_CHANNEL_MAP:
            body_m.cmd_id = BT_CMD_SET_SHIELDED_CHANNEL_MAP;
            body_m.cmd_len = 0x01;
            if (send_buf == NULL)
                goto error;
            memcpy(body_m.cmd_value, send_buf, body_m.cmd_len);
            break;
        case BT_CMD_GET_VERSION:
            body_m.cmd_id = BT_SET_CMDS_ID_LEN_5;
            body_m.cmd_len = BT_CMD_GET_VERSION;
            break;
        default:
            ret = BT_RET_ERROR;
            break;
    }
    if (ret != BT_RET_SUCCESS)
        goto error;
    gbt->bt_cmds = cmd;
    /*uart send data*/
    body_m.frame_head = 0x55aa;
    memset((void *)buf, 0, UART_TX_WRITE_MAX_BUFF);

    if (bt_ac6955fgx_messagebody_switch_str(&body_m, buf) == BT_RET_ERROR)
        goto error;

    if (body_m.cmd_id != BT_REC_CMDS_ID_LEN_5 && body_m.cmd_id != BT_SET_CMDS_ID_LEN_5)
    {
        ret = write(gbt->uartfd, buf, body_m.cmd_len + 5);
    }
    else
    {
        ret = write(gbt->uartfd, buf, 5);
    }


    if (ret < 0)
        goto error;
    ret = BT_RET_SUCCESS;
exit:
    return ret;
error:
    bt_log_e("data error\n");
    ret = BT_RET_ERROR;
    return ret;
}

static void bt_ac6955fgx_cp_connet_info(struct bluetooth_slave_dev *connet_cmds,
                                        struct AC6955FGX_MessageBody *obj)
{
    if (connet_cmds == NULL || obj == NULL)
        return;
    memcpy(obj->cmd_value, connet_cmds->mac, BLUETOOTH_MAC_LEN);
    obj->cmd_len = BLUETOOTH_MAC_LEN;
}

// static int bt_max(unsigned char *str1, unsigned char *str2, int count)
// {
//  for(int i = 0; i < count; i++)
//  {
//      if(str1[i] != str2[i])
//      {
//          return -1;
//      }
//  }
//  return 0;
// }

static void bt_ac6955fgx_serial_data_judg(bt_ac6955fgx_cmd_sends_e cmd, struct AC6955FGX_MessageBody body_m,
                                          unsigned int count)
{
    int ret = BT_RET_SUCCESS;
    struct bluetooth_slave_dev *acq_dev_info = &gbt->inquiry_info;
    struct bluetooth_slave_dev *connet_data = &gbt->connet_info;
    unsigned char name_cnt = 0;
    bt_printf("%s %d cmd = %d \n", __func__, __LINE__, cmd);

    switch (cmd)
    {
        case BT_CMD_SET_BT_POWER_ON:
            if (gbt->get_dev_status < EBT_DEVICE_STATUS_WORKING_CONNECTED)
            {
                gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_EXISTENT;
            }
            // ret = BT_RET_EXIT;
            break;
        case BT_CMD_SET_BT_POWER_OFF:
            gbt->get_dev_status = EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF;
            if (body_m.cmd_id == 0xFA && body_m.cmd_len == 0x01)
            {
                ret = BT_RET_EXIT;
            }
            break;
        default:
            break;
    }
    if (ret != BT_RET_EXIT)
    {
        switch (body_m.cmd_id)
        {
            case 0xFA:
                if (body_m.cmd_len == BT_CMD_ID_GET_DISCONNECTED)
                {
                    gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_DISCONNECTED;
                    gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED, 0);
                }
                if (body_m.cmd_len == BT_CMD_ID_GET_CONNECTED)
                {
                    gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_CONNECTED;
                    gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED, 0);
                }
                else if (body_m.cmd_len == BT_CMD_ID_GET_INQUIRY_STOP_SEARCH)
                {
                    gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_INQUIRY_STOP_SEARCHING;
                    gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED, 0);
                }
                else if (body_m.cmd_len == BT_CMD_ID_GET_ACK_OK)
                {
                    if (gbt->get_dev_status < EBT_DEVICE_STATUS_WORKING_CONNECTED)
                    {
                        gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_EXISTENT;
                    }
                }
                else if (body_m.cmd_len == BT_CMD_ID_GET_TIMED_OUT)
                {
                    gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_CONNECTION_TIMED_OUT, 0);
                }
                else if (body_m.cmd_len == BT_CMD_ID_GET_RECONNECTING)
                {
                    gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_IS_RECONNECTING, 0);
                    bt_log_e("Please use the latest Bluetooth firmware \n");
                }
                else if (body_m.cmd_len == BT_CMD_ID_GET_SEARCHING)
                {
                    gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_IS_SEARCHING, 0);
                    gbt->get_dev_status = EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF;
                }
                else if (body_m.cmd_len == BT_CMD_ID_GET_BT_INIT_OK)
                {
                    gbt->get_dev_status = EBT_DEVICE_STATUS_NOWORKING_BT_RESET_OK;
                    if (gbt->bt_reset_flag)
                        gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_GET_INIT_WORKING_STATE, 0);
                    gbt->bt_reset_flag = 0;
                    bt_log_e("int ok \n");
                }
                else if (body_m.cmd_len == BT_CMD_ID_GET_ACK_ERROR)
                {
                    bt_log_e("ack error\n");
                }
                break;
            case BT_CMD_ID_GET_CONNECT_STATUS:
                memset(connet_data, 0, sizeof(struct bluetooth_slave_dev));
                memcpy(connet_data->mac, body_m.cmd_value, BLUETOOTH_MAC_LEN);
                if (body_m.cmd_len - BLUETOOTH_MAC_LEN - 1 < BLUETOOTH_NAME_LEN)
                {
                    memcpy(connet_data->name, &body_m.cmd_value[BLUETOOTH_MAC_LEN + 1], body_m.cmd_len - BLUETOOTH_MAC_LEN - 1);
                }
                else
                {
                    memcpy(connet_data->name, &body_m.cmd_value[BLUETOOTH_MAC_LEN + 1], BLUETOOTH_NAME_LEN);
                    connet_data->name[BLUETOOTH_NAME_LEN - 1] = 0;
                }

                gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED, 0);
                gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_GET_RECONNECTED_INFO;
                gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_GET_CONNECTED_INFO, (unsigned long)connet_data);
                break;
            case BT_CMD_ID_GET_RECONNECTED_MAC_NAME:
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
                gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_GET_RECONNECTED_INFO;
                gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_IS_RECONNECTING, (unsigned long)acq_dev_info);
                // bluetooth_inquiry_printf(acq_dev_info);
                break;
            case BT_CMD_ID_GET_INQUIRY:
                memset(acq_dev_info, 0, sizeof(struct bluetooth_slave_dev));
                name_cnt = body_m.cmd_len - BLUETOOTH_MAC_LEN - BLUETOOTH_TYPE_LEN;
                memcpy(acq_dev_info->mac, body_m.cmd_value, BLUETOOTH_MAC_LEN);
                acq_dev_info->type_value = body_m.cmd_value[BLUETOOTH_MAC_LEN + 0];
                acq_dev_info->type_value <<= 8;
                acq_dev_info->type_value |= body_m.cmd_value[BLUETOOTH_MAC_LEN + 1];
                acq_dev_info->type_value <<= 8;
                acq_dev_info->type_value |= body_m.cmd_value[BLUETOOTH_MAC_LEN + 2];
                acq_dev_info->type_value <<= 8;
                acq_dev_info->type_value |= body_m.cmd_value[BLUETOOTH_MAC_LEN + 3];
                bt_printf("warn type_value =%x \n", acq_dev_info->type_value);
                if (name_cnt < BLUETOOTH_NAME_LEN)
                {
                    memcpy(acq_dev_info->name, &body_m.cmd_value[BLUETOOTH_MAC_LEN + BLUETOOTH_TYPE_LEN], name_cnt);
                }
                else
                {
                    memcpy(acq_dev_info->name, &body_m.cmd_value[BLUETOOTH_MAC_LEN + BLUETOOTH_TYPE_LEN], BLUETOOTH_NAME_LEN);
                    acq_dev_info->name[BLUETOOTH_NAME_LEN - 1] = 0;
                }
                if (body_m.cmd_len == 7)
                {
                    bt_printf("warn cmd len ==7\n");
                    break;
                }
                gbt->get_dev_status = EBT_DEVICE_STATUS_WORKING_INQUIRY_DATA_SEARCHED;
                gbt->callback(BLUETOOTH_EVENT_SLAVE_DEV_SCANNED, (unsigned long)acq_dev_info);
                break;
            case BT_CMD_ID_GET_BT_VERSOION:
                gbt->callback(BLUETOOTH_EVENT_SLAVE_GET_VERSION, (unsigned long)body_m.cmd_value);
                break;
            case BT_CMD_ID_GET_CHANNEL_MAP:
                break;
            default:
                break;
        }
    }
    printf_bt_ac6955fgx_dev_status(gbt->get_dev_status);
}

#ifdef HC_LINUX
#define bt_clock_systick_get clock_systick_get
static unsigned long long clock_systick_get(void)
{
    int ret = -1;
    unsigned long long time;
    int cnt = 0;
    struct timespec  now = {0, 0};

    while (ret < 0 && cnt < 3)
    {
        ret = clock_gettime(CLOCK_MONOTONIC, &now); //获取失败重试，最大执行3次
        cnt++;
    }
    time = now.tv_sec * 1000 + now.tv_nsec / (1000000);
    return time;
}

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

#else
#define bt_clock_systick_get xTaskGetTickCount
#endif


#ifdef HC_LINUX
static void *bt_ac6955fgx_read_thread(void *args)
#else
static void bt_ac6955fgx_read_thread(void *args)
#endif
{
    unsigned char *rx_buf = (unsigned char *)malloc(1 * UART_RX_RECEIVE_MAX_BUFF);
    char byte = 0;
    struct pollfd fds[1];
    nfds_t nfds = 1;
    static unsigned char date_off_set = 0;
    static int count = 0;
    int ret = BT_RET_SUCCESS;
    long long tTickNow = 0;
    struct AC6955FGX_MessageBody body = {0};

    if (gbt == NULL)
        goto exit;

    fds[0].fd = gbt->uartfd;
    fds[0].events = POLLIN | POLLRDNORM;
    fds[0].revents = 0;
    bt_printf("fds[0].fd =%d \n", fds[0].fd);
    bt_printf("init \n");
    while (!bt_ac6955fgx_task_start_flag)
    {
        if (gbt->uartfd < 0)
            break;

        ret = poll(fds, nfds, 0);
        if (ret > 0)
        {
            if (fds[0].revents & (POLLRDNORM | POLLIN))
            {
                if (read(gbt->uartfd, &byte, 1))
                {
                    rx_buf[count++] = byte;
                    tTickNow = bt_clock_systick_get();
                    if (count == UART_RX_RECEIVE_MAX_BUFF)
                    {
                        date_off_set = 0;
                        count = 0;
                    }
                }
            }
        }
        /*Serial port returns data for judgment*/
        if (count >= BLUETOOTH_MINIMUM_CHECKSUM)
        {
            ret = str_with_ac6955fgx_messagebody_compare(&rx_buf[date_off_set], count - date_off_set, &body);
            if (ret == BT_RET_SUCCESS)
            {
                bt_printf("count =%d\n", count);
                bt_ac6955fgx_serial_data_judg(gbt->bt_cmds, body, count - date_off_set);
                memset(rx_buf, 0, count);
                memset(&body, 0, count);
                count = 0;
                date_off_set = 0;
            }
            else if (ret == BT_ERROR_DATA_OFFSET_1 || ret == BT_ERROR_DATA_OFFSET_2)
            {
                date_off_set += ret - BT_ERROR_DATA;
                if (date_off_set > 10)
                {
                    bt_log_e("date_off_set too large\n");
                    // elog_hexdump2(ELOG_LVL_ERROR, "bt rxbuf data", 16, rx_buf, count);
                    memset(rx_buf, 0, count);
                    memset(&body, 0, count);
                    date_off_set = 0;
                    count = 0;
                }
            }
            else if (ret == BT_RET_ERROR)
            {
                bt_log_e("rx buff error\n");
                // elog_hexdump2(ELOG_LVL_ERROR, "bt rxbuf data", 16, rx_buf, count);
                bt_printf_buf(rx_buf, count);
                memset(rx_buf, 0, count);
                memset(&body, 0, count);
                date_off_set = 0;
                count = 0;
            }
        }

        if (count > 0 && bt_clock_systick_get() - tTickNow > BLUETOOTH_DATE_TIMEOUT)
        {
            bt_log_e("data error timeout\n");
            // elog_hexdump2(ELOG_LVL_ERROR, "bt rxbuf data", 16, rx_buf, count);
            bt_printf_buf(rx_buf, count);
            memset(rx_buf, 0, count);
            count = 0;
            date_off_set = 0;
        }

        usleep(5000);
    }
exit:
    free(rx_buf);
    usleep(1000);
#ifndef __linux__
    complete(&bt_ac6955fgx_task_completion);
    vTaskDelete(NULL);
#else
    return NULL;
#endif
}

int bluetooth_init(const char *uart_path, bluetooth_callback_t callback)
{
    int fd;

    int ret = BT_RET_SUCCESS;
    if (gbt == NULL)
    {
        gbt = (struct bt_ac6955fgx_priv *)malloc(
                  sizeof(struct bt_ac6955fgx_priv));
        if (gbt == NULL)
            goto error;
        memset(gbt, 0, sizeof(struct bt_ac6955fgx_priv));
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
            bt_log_e("open %s error %d\n", uart_path, fd);
            goto uart_error;
        }


        /* create task */
        gbt->uartfd = fd;
        bt_ac6955fgx_task_start_flag = 0;
#ifdef __linux__
        // set_opt(fd, 9600, 8, 'N', 1);
        set_opt(fd, 115200, 8, 'N', 1);
        ret = pthread_create(&bluetooth_thread_id, NULL, bt_ac6955fgx_read_thread, NULL);
        if (ret != 0)
        {
            bt_log_e("kshm recv thread create failed\n");
            goto taskcreate_error;
        }
#else
        struct sci_setting bt_ac6955fgx_sci;
        bt_ac6955fgx_sci.parity_mode = PARITY_NONE;
        bt_ac6955fgx_sci.bits_mode = bits_mode_default;
        ioctl(fd, SCIIOC_SET_BAUD_RATE_115200, NULL);
        ioctl(fd, SCIIOC_SET_SETTING, &bt_ac6955fgx_sci);
        init_completion(&bt_ac6955fgx_task_completion);
        ret = xTaskCreate(bt_ac6955fgx_read_thread, "bt_ac6955fgx_read_thread",
                          0x1000, &gbt->bt_cmds, portPRI_TASK_NORMAL, NULL);
#endif
        /* init success */
        gbt->callback = callback;
        gbt->cref++;
    }
    else
    {
        gbt->cref++;
    }

    /* SUCCESS */
    return BT_RET_SUCCESS;

taskcreate_error:
    bt_ac6955fgx_task_start_flag = 1;
#ifndef __linux__
    wait_for_completion_timeout(&bt_ac6955fgx_task_completion, 3000);
#endif
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
        /* delete task */
        bt_ac6955fgx_task_start_flag = 1;
#ifdef __linux__
        if (bluetooth_thread_id)
            pthread_join(bluetooth_thread_id, NULL);
        bluetooth_thread_id = 0;
#else
        wait_for_completion_timeout(&bt_ac6955fgx_task_completion, 3000);
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
    bt_device_status_e read_state = 0;
    int count = 0;
    if (bt_ac6955fgx_set_action(BT_CMD_SET_BT_POWER_ON, 0) == 0)
    {
        while (1)
        {
            read_state = bt_get_device_sys_status();
            if (read_state != EBT_DEVICE_STATUS_NOWORKING_DEFAULT)
            {
                if (read_state <
                    EBT_DEVICE_STATUS_WORKING_EXISTENT)
                    return BT_RET_ERROR;
                break;
            }
            if (count++ > 30)
                return BT_RET_ERROR;
            usleep(20 * 1000);
        }
    }
    else
        return BT_RET_ERROR;
    return BT_RET_SUCCESS;
}

int bluetooth_poweroff(void)
{
    bt_device_status_e read_state = 0;
    int count = 0;
    if (bt_ac6955fgx_set_action(BT_CMD_SET_BT_POWER_OFF, 0) == 0)
    {
        while (1)
        {
            read_state = bt_get_device_sys_status();
            if (read_state == EBT_DEVICE_STATUS_NOWORKING_BT_POWER_OFF)
            {
                break;
            }
            if (count++ > 35)
                return BT_RET_ERROR;
            usleep(20 * 1000);
        }
    }
    else
        return BT_RET_ERROR;
    return BT_RET_SUCCESS;
}

int bluetooth_scan(void)
{
    if (bt_ac6955fgx_set_action(BT_CMD_SET_INQUIRY_START, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_stop_scan(void)
{
    if (bt_ac6955fgx_set_action(BT_CMD_SET_INQUIRY_STOP, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_connect(unsigned char *mac)
{
    if (bt_ac6955fgx_set_action(BT_CMD_SET_CONNECT, mac) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_get_connect_info(unsigned char *mac)
{
    if (bt_ac6955fgx_set_action(BT_CMD_GET_CONNECT_INFO, mac) == 0)
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
    if (bt_ac6955fgx_set_action(BT_CMD_GET_BT_CONNECT_STATE, 0) == 0)
    {
        while (1)
        {
            count++;
            read_state = bt_get_device_sys_status();
            if (read_state > EBT_DEVICE_STATUS_NOWORKING_DEFAULT)
            {
                if (read_state ==
                    EBT_DEVICE_STATUS_WORKING_CONNECTED)
                {
                    bt_printf("The device is already connected\n");
                }
                else if (read_state ==
                         EBT_DEVICE_STATUS_WORKING_GET_CONNECTED_INFO)
                {
                    bt_printf("The device is already connected\n");
                }
                else if (read_state ==
                         EBT_DEVICE_STATUS_WORKING_DISCONNECTED)
                {
                    bt_printf("Device not connected\n");
                    return BT_RET_NOT_CONNECTED;
                }
                break;
            }
            if (count > 25)
                return BT_RET_TIMEOUT;
            usleep(20 * 1000);
        }
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_disconnect(void)
{
    if (bt_ac6955fgx_set_action(BT_CMD_SET_DISCONNECT, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_del_all_device(void)
{
    if (bt_ac6955fgx_set_action(BT_CMD_SET_DELETE_ALL_DEVICE, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_del_list_device(void)
{
    if (bt_ac6955fgx_set_action(BT_CMD_SET_DELETE_LAST_DEVICE, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

int bluetooth_power_on_to_rx(void)
{
    if (bt_ac6955fgx_set_action(BT_CMD_SET_POWER_ON_TO_RX, 0) == 0)
        return BT_RET_SUCCESS;
    else
        return BT_RET_ERROR;
}

int bluetooth_write_buf(char *buf)
{
    if (gbt->uartfd < 0)
    {
        bt_printf("error %s %d\n", __func__, __LINE__);
        return 0;
    }
    if (buf == NULL)
    {
        bt_printf("error %s %d\n", __func__, __LINE__);
        return -1;
    }

    bt_printf("%s \n", buf);
    write(gbt->uartfd, buf, strlen(buf));
    return BT_RET_SUCCESS;
}

int bluetooth_factory_reset(void)
{
    // unsigned char buf[20] = {0x55,0xAA,0x88,0x01,0x88,0x00,0x00,0x55,0xaa,0x88,0x08,0x8f,0x00,0x00};
    // write(gbt->uartfd, buf, strlen(buf));
    bluetooth_poweron();
    bluetooth_scan();
    usleep(200 * 1000);
    bluetooth_del_all_device();
    usleep(200 * 1000);
    bluetooth_poweroff();
    return BT_RET_SUCCESS;
}

static int bluetooth_reset(int block_flag)
{
    if (bt_ac6955fgx_set_action(BT_CMD_SET_RESET, 0) != 0)
    {
        return BT_RET_ERROR;
    }
    if (block_flag)
    {
        bt_device_status_e read_state = 0;
        int count = 0;
        while (1)
        {
            read_state = bt_get_device_sys_status();
            if (read_state == EBT_DEVICE_STATUS_NOWORKING_BT_RESET_OK)
            {
                break;
            }
            if (count++ > 20)
                return BT_RET_ERROR;
            usleep(20 * 1000);
        }
    }
    return BT_RET_SUCCESS;
}

static int bluetooth_poweron2(void)
{
    if (bt_ac6955fgx_set_action(BT_CMD_SET_BT_POWER_ON_2, 0) == 0)
        return BT_RET_SUCCESS;
    else
        return BT_RET_ERROR;
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

    if (bt_ac6955fgx_set_action(BT_CMD_SET_CHANNEL_MAP, buf) == 0)
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
    if (bt_ac6955fgx_set_action(BT_CMD_SET_CHANNEL_MAP, buf) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

static int bluetooth_set_shielded_channel_map(unsigned char channel)
{
    // if (bt_ac6955fgx_set_action(BT_CMD_SET_SHIELDED_CHANNEL_MAP, &channel) == 0) {
    //  return BT_RET_SUCCESS;
    // } else
    //  return BT_RET_ERROR;
    return BT_RET_SUCCESS;
}

static int bluetooth_get_version(void)
{
    if (bt_ac6955fgx_set_action(BT_CMD_GET_VERSION, 0) == 0)
    {
        return BT_RET_SUCCESS;
    }
    else
        return BT_RET_ERROR;
}

static int bluetooth_get_channel_map(void)
{
    if (bt_ac6955fgx_set_action(BT_CMD_GET_CHANNEL_MAP, 0) == 0)
        return BT_RET_SUCCESS;
    else
        return BT_RET_ERROR;
}

static int _bluetooth_ioctl_(int cmd, unsigned long arg)
{
    int ret = BT_RET_SUCCESS;
    switch (cmd)
    {
        case BLUETOOTH_SET_RESET:
            ret = bluetooth_reset(0);
            break;
        case BLUETOOTH_SET_RESET_BLOCK:
            ret = bluetooth_reset(1);
            break;
        case BLUETOOTH_SET_POWERON:
            ret = bluetooth_poweron2();
            break;
        case BLUETOOTH_SET_CLOSE_CHANNEL_MAP:
            ret = bluetooth_set_close_channel_map((struct bluetooth_channel_map*)arg);
            break;
        case BLUETOOTH_SET_OPEN_CHANNEL_MAP:
            ret = bluetooth_set_open_channel_map((struct bluetooth_channel_map*)arg);
            break;
        case BLUETOOTH_SET_SHIELDED_CHANNEL_MAP:
            ret = bluetooth_set_shielded_channel_map((char)arg);
            break;
        case BLUETOOTH_GET_VERSION:
            ret = bluetooth_get_version();
            break;
        case BLUETOOTH_GET_CHANNEL_MAP:
            ret = bluetooth_get_channel_map();
            break;
        default:
            break;
    }
    return ret;
}

int bluetooth_ioctl(int cmd, ...)
{
    unsigned long arg = 0;
    int ret = BT_RET_SUCCESS;
    va_list args;
    va_start(args, cmd);
    arg = va_arg(args, unsigned long);
    ret = _bluetooth_ioctl_(cmd, arg);
    va_end(args);
    return ret;
}

int bluetooth_set_gpio_backlight(unsigned char value)
{
    return 0;
}

int bluetooth_set_gpio_mutu(unsigned char value)
{
    return 0;
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

int bluetooth_memory_connection(unsigned char value)
{
    return BT_RET_SUCCESS;
}

static bluetooth_ir_control_t bt_control;
int bluetooth_ir_key_init(bluetooth_ir_control_t control)
{
    if (control == NULL)
        return BT_RET_ERROR;

    bt_control = control;
    bt_printf("%s %d\n", __func__, __LINE__);
    return BT_RET_SUCCESS;
}

/*
function: Bluetooth sending infrared button
examples: bluetooth_ir_key_send(KEY_DOWN);
*/
int bluetooth_ir_key_send(unsigned short code)
{
    struct input_event_bt event_key = { 0 };
    if (bt_control == NULL)
        return BT_RET_ERROR;
    event_key.type = EV_KEY;
    event_key.value = 1;
    event_key.code = code;
    bt_control(event_key);
    bt_printf("%s %d\n", __func__, __LINE__);
    return BT_RET_SUCCESS;
}
#endif
