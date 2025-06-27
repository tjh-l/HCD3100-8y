/**
 * @file
 * @brief                hudi screen mipi interface
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

#include <hcuapi/mipi.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_screen.h>

#include "hudi_screen_inter.h"

static pthread_mutex_t g_hudi_mipi_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_mipi_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_mipi_mutex);

    return 0;
}

static int _hudi_mipi_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_mipi_mutex);

    return 0;
}

int hudi_mipi_dsi_gpio_set(hudi_handle handle, int val)
{
    hudi_mipi_instance_t *inst = (hudi_mipi_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Mipi not open\n");
        return -1;
    }

    _hudi_mipi_mutex_lock();

    if (ioctl(inst->fd, MIPI_DSI_GPIO_ENABLE, val) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Mipi start fail\n");
        _hudi_mipi_mutex_unlock();
        return -1;
    }

    _hudi_mipi_mutex_unlock();

    return 0;
}

int hudi_mipi_open(hudi_handle *handle)
{
    hudi_mipi_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid mipi parameters\n");
        return -1;
    }

    _hudi_mipi_mutex_lock();

    inst = (hudi_mipi_instance_t *)malloc(sizeof(hudi_mipi_instance_t));
    memset(inst, 0, sizeof(hudi_mipi_instance_t));

    inst->fd = open(HUDI_MIPI_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open mipi fail\n");
        free(inst);
        _hudi_mipi_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_mipi_mutex_unlock();

    return 0;
}

int hudi_mipi_close(hudi_handle handle)
{
    hudi_mipi_instance_t *inst = (hudi_mipi_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Mipi not open\n");
        return -1;
    }

    _hudi_mipi_mutex_lock();

    close(inst->fd);

    memset(inst, 0, sizeof(hudi_mipi_instance_t));
    free(inst);

    _hudi_mipi_mutex_unlock();

    return 0;
}