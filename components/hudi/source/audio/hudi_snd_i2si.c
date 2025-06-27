/**
* @file
* @brief                hudi sound i2si interface
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
#include <sys/poll.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_snd_i2si.h>

#include "hudi_audio_inter.h"

static pthread_mutex_t g_hudi_snd_i2si_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_snd_i2si_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_snd_i2si_mutex);

    return 0;
}

static int _hudi_snd_i2si_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_snd_i2si_mutex);

    return 0;
}

static int _hudi_snd_i2si_dev_start(int fd, struct snd_pcm_params *config)
{
    if (!config->channels || !config->bitdepth)
    {
        hudi_log(HUDI_LL_ERROR, "Incorrect parameters\n");
        return 0;
    }

    if (ioctl(fd, SND_IOCTL_HW_PARAMS, config) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "SND i2si Parameters set fail\n");
        return -1;
    }

    ioctl(fd, SND_IOCTL_START, 0);

    return 0;
}

static int _hudi_snd_i2si_dev_stop(int fd)
{
    if (fd > 0)
    {
        ioctl(fd, SND_IOCTL_DROP, 0);
        ioctl(fd, SND_IOCTL_HW_FREE, 0);
    }

    return 0;
}

int hudi_snd_i2si_open(hudi_handle *handle)
{
    hudi_snd_i2si_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid snd i2si parameters\n");
        return -1;
    }

    _hudi_snd_i2si_mutex_lock();

    inst = (hudi_snd_i2si_instance_t *)malloc(sizeof(hudi_snd_i2si_instance_t));
    memset(inst, 0, sizeof(hudi_snd_i2si_instance_t));
    inst->fd_i2si = -1;
    
    inst->fd_i2si = open(HUDI_SND_I2SI_DEV, O_RDWR);
    if (inst->fd_i2si < 0)
    {
        free(inst);
        hudi_log(HUDI_LL_ERROR, "Open %s fail\n", HUDI_SND_I2SI_DEV);
        _hudi_snd_i2si_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_snd_i2si_mutex_unlock();

    return 0;
}

int hudi_snd_i2si_close(hudi_handle handle)
{
    hudi_snd_i2si_instance_t *inst = (hudi_snd_i2si_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND i2si not open\n");
        return -1;
    }

    _hudi_snd_i2si_mutex_lock();

    if (inst->fd_i2si >= 0)
    {
        if (inst->started)
        {
            _hudi_snd_i2si_dev_stop(inst->fd_i2si);
        }
        close(inst->fd_i2si);
    }

    memset(inst, 0, sizeof(hudi_snd_i2si_instance_t));
    free(inst);

    _hudi_snd_i2si_mutex_unlock();

    return 0;
}

int hudi_snd_i2si_start(hudi_handle handle, struct snd_pcm_params *config)
{
    hudi_snd_i2si_instance_t *inst = (hudi_snd_i2si_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND i2si not open\n");
        return -1;
    }

    if (inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND i2si already started\n");
        return -1;
    }

    _hudi_snd_i2si_mutex_lock();

    if (inst->fd_i2si >= 0)
    {
        if (0 != _hudi_snd_i2si_dev_start(inst->fd_i2si, config))
        {
            hudi_log(HUDI_LL_ERROR, "SND i2si start fail\n");
            _hudi_snd_i2si_mutex_unlock();
            return -1;
        }
    }

    inst->started = 1;

    _hudi_snd_i2si_mutex_unlock();

    return 0;
}

int hudi_snd_i2si_stop(hudi_handle handle)
{
    hudi_snd_i2si_instance_t *inst = (hudi_snd_i2si_instance_t *)handle;

    if (!inst || !inst->inited || !inst->started)
    {
        hudi_log(HUDI_LL_ERROR, "SND i2si not start\n");
        return -1;
    }

    _hudi_snd_i2si_mutex_lock();

    if (inst->fd_i2si >= 0)
    {
        _hudi_snd_i2si_dev_stop(inst->fd_i2si);
    }

    inst->started = 0;

    _hudi_snd_i2si_mutex_unlock();

    return 0;
}

int hudi_snd_i2si_volume_get(hudi_handle handle, unsigned char *volume)
{
    hudi_snd_i2si_instance_t *inst = (hudi_snd_i2si_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited || !volume)
    {
        hudi_log(HUDI_LL_ERROR, "SND i2si not open\n");
        return -1;
    }

    _hudi_snd_i2si_mutex_lock();

    if (inst->fd_i2si >= 0)
    {
        ret = ioctl(inst->fd_i2si, SND_IOCTL_GET_VOLUME, volume);
    }

    _hudi_snd_i2si_mutex_unlock();

    return ret;
}

int hudi_snd_i2si_volume_set(hudi_handle handle, unsigned char volume)
{
    hudi_snd_i2si_instance_t *inst = (hudi_snd_i2si_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND i2si not open\n");
        return -1;
    }

    _hudi_snd_i2si_mutex_lock();

    if (inst->fd_i2si >= 0)
    {
        ret = ioctl(inst->fd_i2si, SND_IOCTL_SET_VOLUME, &volume);
    }

    _hudi_snd_i2si_mutex_unlock();

    return ret;
}

int hudi_snd_i2si_mute_set(hudi_handle handle, unsigned int mute)
{
    hudi_snd_i2si_instance_t *inst = (hudi_snd_i2si_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "SND i2si not open\n");
        return -1;
    }

    _hudi_snd_i2si_mutex_lock();

    if (inst->fd_i2si >= 0)
    {
        ret = ioctl(inst->fd_i2si, SND_IOCTL_SET_MUTE, mute);
    }
    
    _hudi_snd_i2si_mutex_unlock();

    return ret;
}
