/**
 * @file
 * @brief                hudi screen pq interface
 * @par Copyright(c):    Hichip Semiconductor (c) 2024
 */

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <hcuapi/pq.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_screen.h>

#include "hudi_screen_inter.h"

static pthread_mutex_t g_hudi_pq_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_pq_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_pq_mutex);

    return 0;
}

static int _hudi_pq_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_pq_mutex);

    return 0;
}

int hudi_pq_set(hudi_handle handle, struct pq_settings *pq)
{
    hudi_pq_instance_t *inst = (hudi_pq_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Pq not open\n");
        return -1;
    }

    _hudi_pq_mutex_lock();

    if (ioctl(inst->fd, PQ_SET_PARAM, pq) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Pq set param fail\n");
        _hudi_pq_mutex_unlock();
        return -1;
    }

    _hudi_pq_mutex_unlock();

    return 0;
}

int hudi_pq_start(hudi_handle handle)
{
    hudi_pq_instance_t *inst = (hudi_pq_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Pq not open\n");
        return -1;
    }

    _hudi_pq_mutex_lock();

    if (ioctl(inst->fd, PQ_START) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Pq start fail\n");
        _hudi_pq_mutex_unlock();
        return -1;
    }

    _hudi_pq_mutex_unlock();

    return 0;
}

int hudi_pq_stop(hudi_handle handle)
{
    hudi_pq_instance_t *inst = (hudi_pq_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Pq not open\n");
        return -1;
    }

    _hudi_pq_mutex_lock();

    if (ioctl(inst->fd, PQ_STOP) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Pq stop fail\n");
        _hudi_pq_mutex_unlock();
        return -1;
    }

    _hudi_pq_mutex_unlock();

    return 0;
}

int hudi_pq_open(hudi_handle *handle)
{
    hudi_pq_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid pq parameters\n");
        return -1;
    }

    _hudi_pq_mutex_lock();

    inst = (hudi_pq_instance_t *)malloc(sizeof(hudi_pq_instance_t));
    memset(inst, 0, sizeof(hudi_pq_instance_t));

    inst->fd = open(HUDI_PQ_DEV, O_WRONLY);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open pq fail\n");
        free(inst);
        _hudi_pq_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_pq_mutex_unlock();

    return 0;
}

int hudi_pq_close(hudi_handle handle)
{
    hudi_pq_instance_t *inst = (hudi_pq_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Pq not open\n");
        return -1;
    }

    _hudi_pq_mutex_lock();

    close(inst->fd);

    memset(inst, 0, sizeof(hudi_pq_instance_t));
    free(inst);

    _hudi_pq_mutex_unlock();

    return 0;
}