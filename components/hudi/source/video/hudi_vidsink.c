/**
* @file
* @brief                hudi vidsink interface
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

#include <hcuapi/vidsink.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_vdec.h>

#include "hudi_video_inter.h"

static pthread_mutex_t g_hudi_vidsink_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_vidsink_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_vidsink_mutex);

    return 0;
}

static int _hudi_vidsink_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_vidsink_mutex);

    return 0;
}

int hudi_vidsink_open(hudi_handle *handle)
{
    hudi_vidsink_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid vidsink parameters\n");
        return -1;
    }

    _hudi_vidsink_mutex_lock();

    inst = (hudi_vidsink_instance_t *)malloc(sizeof(hudi_vidsink_instance_t));
    memset(inst, 0, sizeof(hudi_vidsink_instance_t));

    inst->fd = open(HUDI_VIDSINK_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open vidsink fail\n");
        free(inst);
        _hudi_vidsink_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_vidsink_mutex_unlock();

    return 0;
}

int hudi_vidsink_close(hudi_handle handle)
{
    hudi_vidsink_instance_t *inst = (hudi_vidsink_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Vidsink not open\n");
        return -1;
    }

    _hudi_vidsink_mutex_lock();

    close(inst->fd);
    memset(inst, 0, sizeof(hudi_vidsink_instance_t));
    free(inst);

    _hudi_vidsink_mutex_unlock();

    return 0;
}

int hudi_vidsink_img_effect_set(hudi_handle handle, unsigned int enable)
{
    hudi_vidsink_instance_t *inst = (hudi_vidsink_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND not open\n");
        return -1;
    }

    _hudi_vidsink_mutex_lock();

    if (enable)
    {
        ret = ioctl(inst->fd, VIDSINK_ENABLE_IMG_EFFECT, enable);
    }
    else
    {
        ret = ioctl(inst->fd, VIDSINK_DISABLE_IMG_EFFECT, enable);
    }
    
    _hudi_vidsink_mutex_unlock();

    return ret;
}
