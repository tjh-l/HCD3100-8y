#include "app_config.h"
#include <hcuapi/dis.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifdef __HCRTOS__
#include <freertos/FreeRTOS.h>
#include <hcuapi/sys-blocking-notify.h>
#include <kernel/fb.h>
#include <kernel/io.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/notify.h>
#include <linux/notifier.h>
#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#include <sys/mount.h>
#endif

#else
#include <linux/fb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif

#include <fcntl.h>
#include <sys/ioctl.h>

#include "com_api.h"
#include "os_api.h"

static uint32_t m_control_msg_id = INVALID_ID;
static uint32_t boardtest_run_control_msg_id = INVALID_ID;
static uint32_t boardtest_exit_control_msg_id = INVALID_ID;
static int m_usb_state = USB_STAT_INVALID;
static char sd_add[10];

char *api_get_ad_mount_add(void)
{
    return sd_add;
}

static int usbd_notify(struct notifier_block *self, unsigned long action, void *dev)
{
    switch (action)
    {
        case USB_MSC_NOTIFY_MOUNT:
            if (strstr(dev, "sd"))
            {
                m_usb_state = USB_STAT_MOUNT;
            }
            else if (strstr(dev, "mmc"))
            {
                m_usb_state = SD_STAT_MOUNT;
            }
            if (dev)
                printf("USB_STAT_MOUNT: %s\n", (char *)dev);

            strncpy(sd_add, dev, sizeof(sd_add) - 1);
            control_msg_t msg = {0};
            msg.msg_type = MSG_TYPE_USB_MOUNT;
            api_control_send_msg(&msg);
            break;
        case USB_MSC_NOTIFY_UMOUNT:
            if (strstr(dev, "sd"))
            {
                m_usb_state = USB_STAT_UNMOUNT;
            }
            else if (strstr(dev, "mmc"))
            {
                m_usb_state = SD_STAT_UNMOUNT;
            }
            break;
        case USB_MSC_NOTIFY_MOUNT_FAIL:
            if (strstr(dev, "sd"))
            {
                m_usb_state = USB_STAT_MOUNT_FAIL;
            }
            else if (strstr(dev, "mmc"))
            {
                m_usb_state = SD_STAT_MOUNT_FAIL;
            }
            break;
        case USB_MSC_NOTIFY_UMOUNT_FAIL:
            if (strstr(dev, "sd"))
            {
                m_usb_state = USB_STAT_UNMOUNT_FAIL;
            }
            else if (strstr(dev, "mmc"))
            {
                m_usb_state = SD_STAT_UNMOUNT_FAIL;
            }
            break;
        default:
            return NOTIFY_OK;
            break;
    }

    return NOTIFY_OK;
}

static struct notifier_block usb_switch_nb =
{
    .notifier_call = usbd_notify,
};

void boardtest_read_ini_init(void)
{
    sys_register_notify(&usb_switch_nb);
}

void boardtest_read_ini_exit(void)
{
    sys_unregister_notify(&usb_switch_nb);
}

/**
 * @brief turn on/off the video frame output
 *
 * @param on_off
 * @return int
 */
int api_dis_show_onoff(bool on_off)
{
    int fd = -1;
    struct dis_win_onoff winon = {0};

    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0)
    {
        return -1;
    }

    winon.distype = DIS_TYPE_HD;
    winon.layer = DIS_LAYER_MAIN;
    winon.on = on_off ? 1 : 0;

    ioctl(fd, DIS_SET_WIN_ONOFF, &winon);
    close(fd);

    return 0;
}

int api_control_send_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == m_control_msg_id)
    {
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id)
        {
            return -1;
        }
    }
    return api_message_send(m_control_msg_id, control_msg, sizeof(control_msg_t));
}

int api_control_receive_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == m_control_msg_id)
    {
        return -1;
    }
    return api_message_receive_tm(m_control_msg_id, control_msg, sizeof(control_msg_t), 5);
}

/*The LVGL thread communicates with the boardtest calling test item thread*/
int boardtest_run_control_send_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == boardtest_run_control_msg_id)
    {
        boardtest_run_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == boardtest_run_control_msg_id)
        {
            return -1;
        }
    }
    return api_message_send(boardtest_run_control_msg_id, control_msg, sizeof(control_msg_t));
}

/*The original code will only be initialized when it is sent, and it will be initialized when it is received here*/
int boardtest_run_control_receive_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == boardtest_run_control_msg_id)
    {
        boardtest_run_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == boardtest_run_control_msg_id)
        {
            return -1;
        }
    }
    return api_message_receive_tm(boardtest_run_control_msg_id, control_msg, sizeof(control_msg_t), 5);
}

int boardtest_exit_control_send_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == boardtest_exit_control_msg_id)
    {
        boardtest_exit_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == boardtest_exit_control_msg_id)
        {
            return -1;
        }
    }
    return api_message_send(boardtest_exit_control_msg_id, control_msg, sizeof(control_msg_t));
}

int boardtest_exit_control_receive_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == boardtest_exit_control_msg_id)
    {
        boardtest_exit_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == boardtest_exit_control_msg_id)
        {
            return -1;
        }
    }
    return api_message_receive_tm(boardtest_exit_control_msg_id, control_msg, sizeof(control_msg_t), 5);
}

void api_control_clear_msg(msg_type_t msg_type)
{
    control_msg_t msg_buffer;
    while (1)
    {
        if (api_control_receive_msg(&msg_buffer))
        {
            break;
        }
    }
    return;
}

int api_control_send_key(uint32_t key)
{
    control_msg_t control_msg;
    control_msg.msg_type = MSG_TYPE_KEY;
    control_msg.msg_code = key;

    if (INVALID_ID == m_control_msg_id)
    {
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id)
        {
            return -1;
        }
    }
    return api_message_send(m_control_msg_id, &control_msg, sizeof(control_msg_t));
}

void api_sleep_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

uint32_t api_sys_tick_get(void)
{
    extern uint32_t custom_tick_get(void);
    return custom_tick_get();
}
