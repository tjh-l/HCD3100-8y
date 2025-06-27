/**
 * @file
 * @brief                hudi watchdog interface
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

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_power.h>

#include "hudi_power_inter.h"

static pthread_mutex_t g_hudi_watchdog_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_watchdog_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_watchdog_mutex);

    return 0;
}

static int _hudi_watchdog_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_watchdog_mutex);

    return 0;
}

#ifdef __HCRTOS__

#include <hcuapi/watchdog.h>

int hudi_watchdog_timeout_set(hudi_handle handle, unsigned int timeout)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    unsigned int timeout_us = timeout * 1000;

    if (ioctl(inst->fd, WDIOC_SETTIMEOUT, timeout_us) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog timeout set fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    _hudi_watchdog_mutex_unlock();

    return 0;
}

int hudi_watchdog_timeout_get(hudi_handle handle, unsigned int *timeout)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    unsigned int timeout_us = 0;

    if (ioctl(inst->fd, WDIOC_GETTIMEOUT, &timeout_us) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog timeout get fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    *timeout = timeout_us / 1000;

    _hudi_watchdog_mutex_unlock();

    return 0;
}

int hudi_watchdog_start(hudi_handle handle)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    if (ioctl(inst->fd, WDIOC_START, NULL) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog start fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    _hudi_watchdog_mutex_unlock();

    return 0;
}

int hudi_watchdog_stop(hudi_handle handle)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    if (ioctl(inst->fd, WDIOC_STOP, NULL) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog stop fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    _hudi_watchdog_mutex_unlock();

    return 0;
}

int hudi_watchdog_status_get(hudi_handle handle, unsigned int *status)
{
    return 0;
}

int hudi_watchdog_mode_set(hudi_handle handle, unsigned int mode)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    if (ioctl(inst->fd, WDIOC_SETMODE, mode) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog mode set fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    _hudi_watchdog_mutex_unlock();

    return 0;
}

#else

#include <linux/watchdog.h>

int hudi_watchdog_timeout_set(hudi_handle handle, unsigned int timeout)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    unsigned int timeout_s = timeout / 1000;

    if (ioctl(inst->fd, WDIOC_SETTIMEOUT, &timeout_s) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog timeout set fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    _hudi_watchdog_mutex_unlock();

    return 0;
}

int hudi_watchdog_timeout_get(hudi_handle handle, unsigned int *timeout)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    unsigned int timeout_s = 0;

    if (ioctl(inst->fd, WDIOC_GETTIMEOUT, &timeout_s) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog timeout get fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    *timeout = timeout_s * 1000;

    _hudi_watchdog_mutex_unlock();

    return 0;
}

int hudi_watchdog_start(hudi_handle handle)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    unsigned int option = WDIOS_ENABLECARD;

    if (ioctl(inst->fd, WDIOC_SETOPTIONS, &option) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog start fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    _hudi_watchdog_mutex_unlock();

    return 0;
}

int hudi_watchdog_stop(hudi_handle handle)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    unsigned int option = WDIOS_DISABLECARD;
    int ret = 0;

    ret = write(inst->fd, "V", 1);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog write fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    if (ioctl(inst->fd, WDIOC_SETOPTIONS, &option) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog stop fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    _hudi_watchdog_mutex_unlock();

    return 0;
}

int hudi_watchdog_status_get(hudi_handle handle, unsigned int *status)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    if (ioctl(inst->fd, WDIOC_GETSTATUS, status) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog get status fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    _hudi_watchdog_mutex_unlock();

    return 0;
}

int hudi_watchdog_mode_set(hudi_handle handle, unsigned int mode)
{
    return 0;
}

#endif // __HCRTOS__

int hudi_watchdog_alive_keep(hudi_handle handle)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    if (ioctl(inst->fd, WDIOC_KEEPALIVE, NULL) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog keep alive fail\n");
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    _hudi_watchdog_mutex_unlock();

    return 0;
}

int hudi_watchdog_open(hudi_handle *handle)
{
    hudi_watchdog_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid watchdog parameters\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    inst = (hudi_watchdog_instance_t *)malloc(sizeof(hudi_watchdog_instance_t));
    memset(inst, 0, sizeof(hudi_watchdog_instance_t));

    inst->fd = open(HUDI_WATCHDOG_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open watchdog fail\n");
        free(inst);
        _hudi_watchdog_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_watchdog_mutex_unlock();

    return 0;
}

int hudi_watchdog_close(hudi_handle handle)
{
    hudi_watchdog_instance_t *inst = (hudi_watchdog_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Watchdog not open\n");
        return -1;
    }

    _hudi_watchdog_mutex_lock();

    close(inst->fd);

    memset(inst, 0, sizeof(hudi_watchdog_instance_t));
    free(inst);

    _hudi_watchdog_mutex_unlock();

    return 0;
}