/**
* @file
* @brief                hudi hdmirx switch interface
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
#include <hudi_log.h>
#include <hudi_hdmirx.h>
#include "hudi_hdmirx_inter.h"



static pthread_mutex_t g_hudi_hdmirx_ms9601a_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_hdmirx_ms9601a_mutex_lock(void)
{
    pthread_mutex_lock(&g_hudi_hdmirx_ms9601a_mutex);
    return 0;
}

static int _hudi_hdmirx_ms9601a_mutex_unlock(void)
{
    pthread_mutex_unlock(&g_hudi_hdmirx_ms9601a_mutex);
    return 0;
}

int hudi_hdmirx_ms9601a_open(hudi_handle *handle)
{
    hudi_hdmirx_ms9601a_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid hdmi rx parameters\n");
        return -1;
    }

    _hudi_hdmirx_ms9601a_mutex_lock();

    inst = (hudi_hdmirx_ms9601a_instance_t *)malloc(sizeof(hudi_hdmirx_ms9601a_instance_t));
    memset(inst, 0, sizeof(hudi_hdmirx_ms9601a_instance_t));

    inst->fd = open(HUDI_HDMI_RX_SWITCH_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open hdmi rx fail\n");
        free(inst);
        _hudi_hdmirx_ms9601a_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_hdmirx_ms9601a_mutex_unlock();

    return 0;
}

int hudi_hdmirx_ms9601a_close(hudi_handle handle)
{
    int ret = -1;
    hudi_hdmirx_ms9601a_instance_t *inst = (hudi_hdmirx_ms9601a_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi hdmi rx not open hdmi rx\n");
        return ret;
    }


    _hudi_hdmirx_ms9601a_mutex_lock();

    close(inst->fd);
    memset(inst, 0, sizeof(hudi_hdmirx_ms9601a_instance_t));
    free(inst);

    _hudi_hdmirx_ms9601a_mutex_unlock();

    return 0;
}

int hudi_hdmirx_ms9601a_channel_set(hudi_handle handle, ms9601_idx_e index)
{
    int ret = -1;
    hudi_hdmirx_ms9601a_instance_t *inst = (hudi_hdmirx_ms9601a_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi hdmi rx not open \n");
        return ret;
    }

    _hudi_hdmirx_ms9601a_mutex_lock();

    ret = ioctl(inst->fd, MS9601A_SET_CH_SEL, index);

    _hudi_hdmirx_ms9601a_mutex_unlock();

    return ret;
}

int hudi_hdmirx_ms9601a_channel_auto_set(hudi_handle handle)
{
    int ret = -1;
    hudi_hdmirx_ms9601a_instance_t *inst = (hudi_hdmirx_ms9601a_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi hdmi rx not open \n");
        return ret;
    }

    _hudi_hdmirx_ms9601a_mutex_lock();

    ret = ioctl(inst->fd, MS9601A_SET_CH_AUTO);

    _hudi_hdmirx_ms9601a_mutex_unlock();

    return ret;
}