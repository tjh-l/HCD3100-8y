/**
 * @file
 * @brief                hudi screen lvds interface
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

#include <hcuapi/lvds.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_screen.h>

#include "hudi_screen_inter.h"

static pthread_mutex_t g_hudi_lvds_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_lvds_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_lvds_mutex);

    return 0;
}

static int _hudi_lvds_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_lvds_mutex);

    return 0;
}

int hudi_lvds_gpio_power_set(hudi_handle handle, int val)
{
    hudi_lvds_instance_t *inst = (hudi_lvds_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Lvds not open\n");
        return -1;
    }

    _hudi_lvds_mutex_lock();

    if (ioctl(inst->fd, LVDS_SET_GPIO_POWER, val) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Lvds set power gpio fail\n");
        _hudi_lvds_mutex_unlock();
        return -1;
    }

    _hudi_lvds_mutex_unlock();

    return 0;
}

int hudi_lvds_pwm_backlight_set(hudi_handle handle, int val)
{
    hudi_lvds_instance_t *inst = (hudi_lvds_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Lvds not open\n");
        return -1;
    }

    _hudi_lvds_mutex_lock();

    if (ioctl(inst->fd, LVDS_SET_PWM_BACKLIGHT, val) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Lvds set pwm backlight fail\n");
        _hudi_lvds_mutex_unlock();
        return -1;
    }

    _hudi_lvds_mutex_unlock();

    return 0;
}

int hudi_lvds_gpio_backlight_set(hudi_handle handle, int val)
{
    hudi_lvds_instance_t *inst = (hudi_lvds_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Lvds not open\n");
        return -1;
    }

    _hudi_lvds_mutex_lock();

    if (ioctl(inst->fd, LVDS_SET_GPIO_BACKLIGHT, val) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Lvds set gpio backlight fail\n");
        _hudi_lvds_mutex_unlock();
        return -1;
    }

    _hudi_lvds_mutex_unlock();

    return 0;
}

int hudi_lvds_gpio_out_set(hudi_handle handle, unsigned int padctl, unsigned char value)
{
    hudi_lvds_instance_t *inst = (hudi_lvds_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Lvds not open\n");
        return -1;
    }

    _hudi_lvds_mutex_lock();

    struct lvds_set_gpio gpio_info = { 0 };
    gpio_info.value = value;
    gpio_info.padctl = padctl;

    if (ioctl(inst->fd, LVDS_SET_GPIO_OUT, &gpio_info) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Lvds set gpio out fail\n");
        _hudi_lvds_mutex_unlock();
        return -1;
    }

    _hudi_lvds_mutex_unlock();

    return 0;
}

int hudi_lvds_open(hudi_handle *handle)
{
    hudi_lvds_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid lvds parameters\n");
        return -1;
    }

    _hudi_lvds_mutex_lock();

    inst = (hudi_lvds_instance_t *)malloc(sizeof(hudi_lvds_instance_t));
    memset(inst, 0, sizeof(hudi_lvds_instance_t));

    inst->fd = open(HUDI_LVDS_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open lvds fail\n");
        free(inst);
        _hudi_lvds_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_lvds_mutex_unlock();

    return 0;
}

int hudi_lvds_close(hudi_handle handle)
{
    hudi_lvds_instance_t *inst = (hudi_lvds_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Lvds not open\n");
        return -1;
    }

    _hudi_lvds_mutex_lock();

    close(inst->fd);

    memset(inst, 0, sizeof(hudi_lvds_instance_t));
    free(inst);

    _hudi_lvds_mutex_unlock();

    return 0;
}