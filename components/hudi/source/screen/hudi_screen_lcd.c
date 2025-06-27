/**
 * @file
 * @brief                hudi screen lcd interface
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

#include <hcuapi/lcd.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_screen.h>

#include "hudi_screen_inter.h"

static pthread_mutex_t g_hudi_lcd_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_lcd_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_lcd_mutex);

    return 0;
}

static int _hudi_lcd_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_lcd_mutex);

    return 0;
}

int hudi_lcd_gpio_power_set(hudi_handle handle, int val)
{
    hudi_lcd_instance_t *inst = (hudi_lcd_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Lcd not open\n");
        return -1;
    }

    _hudi_lcd_mutex_lock();

    if (ioctl(inst->fd, LCD_SET_POWER_GPIO, val) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Lcd set gpio power fail\n");
        _hudi_lcd_mutex_unlock();
        return -1;
    }

    _hudi_lcd_mutex_unlock();

    return 0;
}

int hudi_lcd_pwm_vcom_set(hudi_handle handle, int val)
{
    hudi_lcd_instance_t *inst = (hudi_lcd_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Lcd not open\n");
        return -1;
    }

    _hudi_lcd_mutex_lock();

    if (ioctl(inst->fd, LCD_SET_PWM_VCOM, val) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Lcd set pwm vcom fail\n");
        _hudi_lcd_mutex_unlock();
        return -1;
    }

    _hudi_lcd_mutex_unlock();

    return 0;
}

int hudi_lcd_rotate_set(hudi_handle handle, lcd_rotate_type_e mode)
{
    hudi_lcd_instance_t *inst = (hudi_lcd_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Lcd not open\n");
        return -1;
    }

    _hudi_lcd_mutex_lock();

    if (ioctl(inst->fd, LCD_SET_ROTATE, mode) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Lcd set rotate fail\n");
        _hudi_lcd_mutex_unlock();
        return -1;
    }

    _hudi_lcd_mutex_unlock();

    return 0;
}

int hudi_lcd_open(hudi_handle *handle)
{
    hudi_lcd_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid lcd parameters\n");
        return -1;
    }

    _hudi_lcd_mutex_lock();

    inst = (hudi_lcd_instance_t *)malloc(sizeof(hudi_lcd_instance_t));
    memset(inst, 0, sizeof(hudi_lcd_instance_t));

    inst->fd = open(HUDI_LCD_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open lcd fail\n");
        free(inst);
        _hudi_lcd_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_lcd_mutex_unlock();

    return 0;
}

int hudi_lcd_close(hudi_handle handle)
{
    hudi_lcd_instance_t *inst = (hudi_lcd_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Lcd not open\n");
        return -1;
    }

    _hudi_lcd_mutex_lock();

    close(inst->fd);

    memset(inst, 0, sizeof(hudi_lcd_instance_t));
    free(inst);

    _hudi_lcd_mutex_unlock();

    return 0;
}