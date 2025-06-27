/**
* @file
* @brief                hudi audio decoder interface
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

#include "hudi_audio_inter.h"

static pthread_mutex_t g_hudi_adec_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_adec_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_adec_mutex);

    return 0;
}

static int _hudi_adec_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_adec_mutex);

    return 0;
}

int hudi_adec_open(hudi_handle *handle, struct audio_config *config)
{
    hudi_adec_instance_t *inst = NULL;

    if (!handle || !config)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid adec parameters\n");
        return -1;
    }

    _hudi_adec_mutex_lock();

    inst = (hudi_adec_instance_t *)malloc(sizeof(hudi_adec_instance_t));
    memset(inst, 0, sizeof(hudi_adec_instance_t));

    inst->fd = open(HUDI_ADEC_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open adec fail\n");
        free(inst);
        _hudi_adec_mutex_unlock();
        return -1;
    }

    if (ioctl(inst->fd, AUDDEC_INIT, config) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "Init adec fail\n");
        close(inst->fd);
        free(inst);
        _hudi_adec_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_adec_mutex_unlock();

    return 0;
}

int hudi_adec_close(hudi_handle handle)
{
    hudi_adec_instance_t *inst = (hudi_adec_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "ADEC not open\n");
        return -1;
    }

    _hudi_adec_mutex_lock();

    close(inst->fd);

    memset(inst, 0, sizeof(hudi_adec_instance_t));
    free(inst);

    _hudi_adec_mutex_unlock();

    return 0;
}

int hudi_adec_start(hudi_handle handle)
{
    hudi_adec_instance_t *inst = (hudi_adec_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "ADEC not open\n");
        return -1;
    }

    _hudi_adec_mutex_lock();

    ret = ioctl(inst->fd, AUDDEC_START, 0);

    _hudi_adec_mutex_unlock();

    return ret;
}

int hudi_adec_pause(hudi_handle handle)
{
    hudi_adec_instance_t *inst = (hudi_adec_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_adec_mutex_lock();

    ret = ioctl(inst->fd, AUDDEC_PAUSE, 0);

    _hudi_adec_mutex_unlock();

    return ret;
}

int hudi_adec_feed(hudi_handle handle, char *data, AvPktHd *pkt)
{
    hudi_adec_instance_t *inst = (hudi_adec_instance_t *)handle;
    
    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "ADEC not open\n");
        return -1;
    }

    _hudi_adec_mutex_lock();

    if (write(inst->fd, (uint8_t *)pkt, sizeof(AvPktHd)) != sizeof(AvPktHd))
    {
        hudi_log(HUDI_LL_ERROR, "Write apkt fail\n");
        ioctl(inst->fd, AUDDEC_FLUSH, 0);
        _hudi_adec_mutex_unlock();
        return -1;
    }
    if (write(inst->fd, data, pkt->size) != pkt->size)
    {
        hudi_log(HUDI_LL_ERROR, "Write aframe fail\n");
        ioctl(inst->fd, AUDDEC_FLUSH, 0);
        _hudi_adec_mutex_unlock();
        return -1;
    }

    _hudi_adec_mutex_unlock();

    return 0;
}

