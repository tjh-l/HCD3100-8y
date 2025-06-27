/**
* @file
* @brief                hudi A/V synchronization interface
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

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_avsync.h>

#include "hudi_avsync_inter.h"

static pthread_mutex_t g_hudi_avsync_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_avsync_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_avsync_mutex);

    return 0;
}

static int _hudi_avsync_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_avsync_mutex);

    return 0;
}

int hudi_avsync_open(hudi_handle *handle)
{
    hudi_avsync_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid avsync parameters\n");
        return -1;
    }

    _hudi_avsync_mutex_lock();

    inst = (hudi_avsync_instance_t *)malloc(sizeof(hudi_avsync_instance_t));
    memset(inst, 0, sizeof(hudi_avsync_instance_t));

    inst->fd = open(HUDI_AVSYNC_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open avsync fail\n");
        free(inst);
        _hudi_avsync_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_avsync_mutex_unlock();

    return 0;
}

int hudi_avsync_close(hudi_handle handle)
{
    hudi_avsync_instance_t *inst = (hudi_avsync_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Avsync not open\n");
        return -1;
    }

    _hudi_avsync_mutex_lock();

    close(inst->fd);
    memset(inst, 0, sizeof(hudi_avsync_instance_t));
    free(inst);

    _hudi_avsync_mutex_unlock();

    return 0;
}

int hudi_avsync_stc_set(hudi_handle handle, unsigned int stc_ms)
{
    hudi_avsync_instance_t *inst = (hudi_avsync_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Avsync not open\n");
        return -1;
    }

    _hudi_avsync_mutex_lock();

    ioctl(inst->fd, AVSYNC_SET_STC_MS, stc_ms);

    _hudi_avsync_mutex_unlock();

    return 0;
}

int hudi_avsync_stc_rate_set(hudi_handle handle, float rate)
{
    hudi_avsync_instance_t *inst = (hudi_avsync_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Avsync not open\n");
        return -1;
    }

    _hudi_avsync_mutex_lock();

    ioctl(inst->fd, AVSYNC_SET_STC_RATE, &rate);

    _hudi_avsync_mutex_unlock();

    return 0;
}

int hudi_avsync_audsync_thres_get(hudi_handle handle, int *value)
{
    hudi_avsync_instance_t *inst = (hudi_avsync_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Avsync not open\n");
        return -1;
    }

    if (!value)
    {
        hudi_log(HUDI_LL_ERROR, "Avsync not open\n");
        return -1;
    }

    _hudi_avsync_mutex_lock();

    ioctl(inst->fd, AVSYNC_GET_AUD_SYNC_THRESH, value);

    _hudi_avsync_mutex_unlock();

    return 0;
}

int hudi_avsync_audsync_thres_set(hudi_handle handle, int value)
{
    hudi_avsync_instance_t *inst = (hudi_avsync_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Avsync not open\n");
        return -1;
    }

    _hudi_avsync_mutex_lock();

    ioctl(inst->fd, AVSYNC_SET_AUD_SYNC_THRESH, value);

    _hudi_avsync_mutex_unlock();

    return 0;
}

int hudi_avsync_vidsync_delay_get(hudi_handle handle, int *delay_ms)
{
    hudi_avsync_instance_t *inst = (hudi_avsync_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Avsync not open\n");
        return -1;
    }

    if (!delay_ms)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid avsync parameters\n");
        return -1;
    }

    _hudi_avsync_mutex_lock();

    ioctl(inst->fd, AVSYNC_GET_VIDEO_DELAY_MS, delay_ms);

    _hudi_avsync_mutex_unlock();

    return 0;
}

int hudi_avsync_vidsync_delay_set(hudi_handle handle, int delay_ms)
{
    hudi_avsync_instance_t *inst = (hudi_avsync_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Avsync not open\n");
        return -1;
    }

    _hudi_avsync_mutex_lock();

    ioctl(inst->fd, AVSYNC_SET_VIDEO_DELAY_MS, delay_ms);

    _hudi_avsync_mutex_unlock();

    return 0;
}
