#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#ifdef HC_RTOS
#include <um/iumirror_api.h>
#include <um/aumirror_api.h>
#include <um/um_api.h>
#else
#include <hccast/iumirror_api.h>
#include <hccast/aumirror_api.h>
#include <hccast/um_api.h>
#endif
#include <hccast_um.h>
#include <hccast_log.h>
#include "hccast_um_avplayer.h"
#include "hccast_um_api.h"

extern ium_av_func_t ium_av_func;
extern um_ioctl ium_api_ioctl;
extern um_ioctl ium_api_ioctl_slave;
extern um_api ium_api_start;
extern um_api ium_api_stop;
static int m_ium_start = 0;

hccast_um_cb g_ium_evt_cb = NULL;

static void hccast_ium_event_process(int event, void *param1, void *param2)
{
    ium_upg_buf_obj_t *pbo;
    hccast_ium_upg_bo_t bo;
    ium_upg_buf_t *pbi;
    hccast_ium_upg_bi_t bi;
    hccast_um_param_t *um_param = hccast_um_inter_param_get();

    if (!g_ium_evt_cb)
    {
        hccast_log(LL_ERROR, "No event process function register\n");
        return ;
    }

    switch (event)
    {
        case IUM_EVT_DEVICE_ADD:
            g_ium_evt_cb(HCCAST_IUM_EVT_DEVICE_ADD, NULL, NULL);
            break;
        case IUM_EVT_DEVICE_REMOVE:
            g_ium_evt_cb(HCCAST_IUM_EVT_DEVICE_REMOVE, NULL, NULL);
            break;
        case IUM_EVT_MIRROR_START:
            g_ium_evt_cb(HCCAST_IUM_EVT_MIRROR_START, NULL, NULL);
            break;
        case IUM_EVT_MIRROR_STOP:
            g_ium_evt_cb(HCCAST_IUM_EVT_MIRROR_STOP, NULL, NULL);
            break;
        case IUM_EVT_SAVE_PAIR_DATA:
            g_ium_evt_cb(HCCAST_IUM_EVT_SAVE_PAIR_DATA, param1, param2);
            break;
        case IUM_EVT_GET_PAIR_DATA:
            g_ium_evt_cb(HCCAST_IUM_EVT_GET_PAIR_DATA, param1, param2);
            break;
        case IUM_EVT_NEED_USR_TRUST:
            g_ium_evt_cb(HCCAST_IUM_EVT_NEED_USR_TRUST, NULL, NULL);
            break;
        case IUM_EVT_USR_TRUST_DEVICE:
            g_ium_evt_cb(HCCAST_IUM_EVT_USR_TRUST_DEVICE, NULL, NULL);
            break;
        case IUM_EVT_CREATE_CONN_FAILED:
            g_ium_evt_cb(HCCAST_IUM_EVT_CREATE_CONN_FAILED, NULL, NULL);
            break;
        case IUM_EVT_CANNOT_GET_AV_DATA:
            break;
        case IUM_EVT_UPG_DOWNLOAD_PROGRESS:
            g_ium_evt_cb(HCCAST_IUM_EVT_UPG_DOWNLOAD_PROGRESS, param1, param2);
            break;
        case IUM_EVT_GET_UPGRADE_DATA:
            pbo = (ium_upg_buf_obj_t *)param1;
            bo.buf = pbo->buf;
            bo.len = pbo->len;
            bo.crc = pbo->crc;
            bo.crc_chk_ok = pbo->crc_chk_ok;
            g_ium_evt_cb(HCCAST_IUM_EVT_GET_UPGRADE_DATA, &bo, NULL);
            break;
        case IUM_EVT_SAVE_UUID:
            g_ium_evt_cb(HCCAST_IUM_EVT_SAVE_UUID, param1, NULL);
            break;
        case IUM_EVT_CERT_INVALID:
            g_ium_evt_cb(HCCAST_IUM_EVT_CERT_INVALID, NULL, NULL);
            break;
        case IUM_EVT_SET_ROTATE:
            g_ium_evt_cb(HCCAST_IUM_EVT_SET_ROTATE, param1, NULL);
            um_param->screen_rotate_en = (unsigned int)param1;
            break;
        case IUM_EVT_FAKE_LIB:
            g_ium_evt_cb(HCCAST_IUM_EVT_FAKE_LIB, NULL, NULL);
            break;
        case IUM_EVT_NO_DATA:
            g_ium_evt_cb(HCCAST_IUM_EVT_NO_DATA, NULL, NULL);
            break;
        case IUM_EVT_GET_UPGRADE_BUF:
            pbi = (ium_upg_buf_t *)param1;
            memset(&bi, 0, sizeof(bi));
            bi.len = pbi->len;
            g_ium_evt_cb(HCCAST_IUM_EVT_GET_UPGRADE_BUF, &bi, NULL);
            pbi->buf = bi.buf;
            break;
        case IUM_EVT_COPYRIGHT_PROTECTION:
            g_ium_evt_cb(HCCAST_IUM_EVT_COPYRIGHT_PROTECTION, NULL, NULL);
            break;
        default:
            hccast_log(LL_WARNING, "Unknown ium event %d\n", event);
            break;
    }
}

int hccast_ium_get_flip_mode()
{
    int flip_mode = 0;
    g_ium_evt_cb(HCCAST_IUM_EVT_GET_FLIP_MODE, (void*)&flip_mode, NULL);
    return flip_mode;
}

int hccast_ium_start(char *uuid, hccast_um_cb event_cb)
{
    if (m_ium_start)
        return 0;

    if (!ium_api_ioctl || !ium_api_start)
    {
        return -1;
    }

    g_ium_evt_cb = event_cb;

    ium_api_ioctl(IUM_CMD_SET_UUID, uuid, NULL);
    ium_api_ioctl(IUM_CMD_SET_EVENT_CB, hccast_ium_event_process, NULL);
    ium_api_ioctl(IUM_CMD_SET_AV_FUNC, &ium_av_func, NULL);
    ium_api_start();
    m_ium_start = 1;
    return 0;
}

int hccast_ium_stop()
{
    if (!m_ium_start)
        return 0;

    if (!ium_api_stop)
    {
        return -1;
    }

    ium_api_stop();

    g_ium_evt_cb = NULL;
    m_ium_start = 0;

    return 0;
}

int hccast_ium_init(hccast_um_cb event_cb)
{
    if (!ium_api_ioctl)
    {
        return -1;
    }

    g_ium_evt_cb = event_cb;

    ium_api_ioctl(IUM_CMD_SET_EVENT_CB, hccast_ium_event_process, NULL);
    ium_api_ioctl(IUM_CMD_INIT, NULL, NULL);
    if (ium_api_ioctl_slave)
    {
        ium_api_ioctl_slave(IUM_CMD_INIT, NULL, NULL);
    }

    return 0;
}

int hccast_ium_stop_mirroring(void)
{
    if (!ium_api_ioctl)
    {
        return -1;
    }
    ium_api_ioctl(IUM_CMD_STOP_MIRRORING, NULL, NULL);

    return 0;
}

int hccast_ium_ioctl(int cmd, void *param1, void *param2)
{
    if (!ium_api_ioctl)
    {
        return -1;
    }

    if (cmd == HCCAST_UM_CMD_SET_IUM_RESOLUTION)
    {
        if ((param1 == NULL) || (param2 == NULL))
        {
            return -1;
        }

        ium_api_ioctl(IUM_CMD_SET_RESOLUTION, param1, param2);
    }

    return 0;
}

int hccast_ium_set_upg_buf(unsigned char *buf, unsigned int len)
{
    if (!ium_api_ioctl)
    {
        return -1;
    }

    ium_api_ioctl(IUM_CMD_SET_UPGRADE_BUF, (void*)buf, (void*)len);

    return 0;
}

int hccast_ium_set_frm_buf(unsigned char *buf, unsigned int len)
{
    if (!ium_api_ioctl)
    {
        return -1;
    }

    ium_api_ioctl(IUM_CMD_SET_FRAME_BUF, (void*)buf, (void*)len);

    return 0;
}

int hccast_ium_set_resolution(int width, int height)
{
    if (!ium_api_ioctl)
    {
        return -1;
    }

    ium_api_ioctl(IUM_CMD_SET_RESOLUTION, (void *)width, (void *)height);

    return 0;
}

int hccast_ium_set_demo_mode(int enable)
{
    if (!ium_api_ioctl)
    {
        return -1;
    }

    ium_api_ioctl(IUM_CMD_SET_DEMO_MODE, (void *)enable, NULL);

    return 0;
}

int hccast_ium_set_hid_onoff(int on)
{
    if (!ium_api_ioctl)
    {
        return -1;
    }

    ium_api_ioctl(IUM_CMD_SET_HID_ONOFF, (void *)on, NULL);

    return 0;
}

int hccast_ium_set_audio_onoff(int on)
{
    if (!ium_api_ioctl)
    {
        return -1;
    }

    ium_api_ioctl(IUM_CMD_SET_AUDIO_ONOFF, (void*)on, NULL);

    return 0;
}

int hccast_ium_slave_start(unsigned char usb_port, const char *udc_name, hccast_um_dev_type_e type)
{
    um_slave_gadget_type_e utype = UM_SLAVE_GADGET_IUM;

    if (!ium_api_ioctl_slave)
    {
        return -1;
    }

    switch (type)
    {
        case HCCAST_UM_DEV_IUM:
            utype = UM_SLAVE_GADGET_IUM;
            break;
        case HCCAST_UM_DEV_IUM_UAC:
            utype = UM_SLAVE_GADGET_IUM_UAC;
            break;
        case HCCAST_UM_DEV_WCID_UAC:
            utype = UM_SLAVE_GADGET_WCID_UAC;
            break;
        case HCCAST_UM_DEV_IUM_HID:
            utype = UM_SLAVE_GADGET_IUM_HID;
            break;
        case HCCAST_UM_DEV_IUM_UAC_HID:
            utype = UM_SLAVE_GADGET_IUM_UAC_HID;
            break;
    }

    ium_api_ioctl_slave(IUM_CMD_SLAVE_DEV_TYPE, (void *)&type, NULL);
    ium_api_ioctl_slave(IUM_CMD_SLAVE_START, (void *)&usb_port, (void *)udc_name);

    return 0;
}

int hccast_ium_slave_stop(unsigned char usb_port)
{
    if (!ium_api_ioctl_slave)
    {
        return -1;
    }

    ium_api_ioctl_slave(IUM_CMD_SLAVE_STOP, (void *)&usb_port, NULL);

    return 0;
}
