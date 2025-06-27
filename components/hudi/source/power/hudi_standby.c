/**
 * @file
 * @brief                hudi standby interface
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

#include <hcuapi/standby.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_power.h>

#include "hudi_power_inter.h"

static pthread_mutex_t g_hudi_standby_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_standby_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_standby_mutex);

    return 0;
}

static int _hudi_standby_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_standby_mutex);

    return 0;
}

int hudi_standby_enter(hudi_handle handle)
{
    hudi_standby_instance_t *inst = (hudi_standby_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Standby not open\n");
        return -1;
    }

    _hudi_standby_mutex_lock();

    if (ioctl(inst->fd, STANDBY_ENTER) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Standby enter fail\n");
        _hudi_standby_mutex_unlock();
        return -1;
    }

    _hudi_standby_mutex_unlock();

    return 0;
}

int hudi_standby_ddr_pwroff_set(hudi_handle handle, pinpad_e pin, int polarity)
{
    hudi_standby_instance_t *inst = (hudi_standby_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Standby not open\n");
        return -1;
    }

    _hudi_standby_mutex_lock();

    struct standby_pwroff_ddr_setting ddr = { 0 };
    ddr.polarity = polarity;
    ddr.pin = pin;

    if (ioctl(inst->fd, STANDBY_SET_PWROFF_DDR, &ddr) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Standby ddr pwroff set fail\n");
        _hudi_standby_mutex_unlock();
        return -1;
    }

    _hudi_standby_mutex_unlock();

    return 0;
}

int hudi_standby_saradc_wakeup_set(hudi_handle handle, int channel, int min, int max)
{
    hudi_standby_instance_t *inst = (hudi_standby_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Standby not open\n");
        return -1;
    }

    _hudi_standby_mutex_lock();

    struct standby_saradc_setting adc = { 0 };
    adc.channel = channel;
    adc.min = min;
    adc.max = max;

    if (ioctl(inst->fd, STANDBY_SET_WAKEUP_BY_SARADC, &adc) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Standby saradc wakeup set fail\n");
        _hudi_standby_mutex_unlock();
        return -1;
    }

    _hudi_standby_mutex_unlock();

    return 0;
}

int hudi_standby_gpio_wakeup_set(hudi_handle handle, pinpad_e pin, int polarity)
{
    hudi_standby_instance_t *inst = (hudi_standby_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Standby not open\n");
        return -1;
    }

    _hudi_standby_mutex_lock();

    struct standby_gpio_setting gpio = { 0 };
    gpio.polarity = polarity;
    gpio.pin = pin;

    if (ioctl(inst->fd, STANDBY_SET_WAKEUP_BY_GPIO, &gpio) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Standby gpio wakeup set fail\n");
        _hudi_standby_mutex_unlock();
        return -1;
    }

    _hudi_standby_mutex_unlock();

    return 0;
}

int hudi_standby_ir_wakeup_set(hudi_handle handle, struct standby_ir_setting *ir)
{
    hudi_standby_instance_t *inst = (hudi_standby_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Standby not open\n");
        return -1;
    }

    _hudi_standby_mutex_lock();

    if (ioctl(inst->fd, STANDBY_SET_WAKEUP_BY_IR, ir) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Standby ir wakeup set fail\n");
        _hudi_standby_mutex_unlock();
        return -1;
    }

    _hudi_standby_mutex_unlock();

    return 0;
}

int hudi_standby_bootup_slot_set(hudi_handle handle, unsigned long slot)
{
    hudi_standby_instance_t *inst = (hudi_standby_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Standby not open\n");
        return -1;
    }

    _hudi_standby_mutex_lock();

    if (ioctl(inst->fd, STANDBY_SET_BOOTUP_SLOT, slot) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Standby bootup slot set fail\n");
        _hudi_standby_mutex_unlock();
        return -1;
    }

    _hudi_standby_mutex_unlock();

    return 0;
}

int hudi_standby_open(hudi_handle *handle)
{
    hudi_standby_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid standby parameters\n");
        return -1;
    }

    _hudi_standby_mutex_lock();

    inst = (hudi_standby_instance_t *)malloc(sizeof(hudi_standby_instance_t));
    memset(inst, 0, sizeof(hudi_standby_instance_t));

    inst->fd = open(HUDI_STANDBY_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open standby fail\n");
        free(inst);
        _hudi_standby_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_standby_mutex_unlock();

    return 0;
}

int hudi_standby_close(hudi_handle handle)
{
    hudi_standby_instance_t *inst = (hudi_standby_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Standby not open\n");
        return -1;
    }

    _hudi_standby_mutex_lock();

    close(inst->fd);

    memset(inst, 0, sizeof(hudi_standby_instance_t));
    free(inst);

    _hudi_standby_mutex_unlock();

    return 0;
}