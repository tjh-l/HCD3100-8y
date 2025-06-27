/**
 * @file
 * @brief                hudi get/set the GPIO value
 * @par Copyright(c):    Hichip Semiconductor (c) 2024
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdbool.h> //bool
#include <dirent.h>
#include <hudi_com.h>
#include <hudi_log.h>
#include "hudi_gpio_ctrl.h"

#ifdef __HCRTOS__

#include <hcuapi/gpio.h>

int hudi_gpio_configure(pinpad_e padctl, hudi_gpio_pinset_t pinset)
{
    if (pinset == HUDI_GPIO_DIR_INPUT)
        return gpio_configure(padctl, GPIO_DIR_INPUT);
    else if (pinset == HUDI_GPIO_DIR_OUTPUT)
        return gpio_configure(padctl, GPIO_DIR_OUTPUT);
}

void hudi_gpio_output_set(pinpad_e padctl, bool val)
{
    gpio_set_output(padctl, val);
}

int hudi_gpio_input_get(pinpad_e padctl)
{
    return gpio_get_input(padctl);
}

#else

static int _hudi_file_exist(const char *path)
{
    DIR *dirp;
    if ((dirp = opendir(path)) == NULL)
    {
        return 0;
    }
    else
    {
        closedir(dirp);
        return 1;
    }
}

static void _hudi_gpio_active(pinpad_e padctl)
{
    int fd = -1;
    char str_buf[64] = {0};

    sprintf(str_buf, "/sys/class/gpio/gpio%d", padctl);
    //if /sys/class/gpio/gpio8 is exist, not need create again.
    if (!_hudi_file_exist(str_buf))
    {
        fd = open("/sys/class/gpio/export", O_WRONLY);
        if (fd < 0)
        {
            hudi_log(HUDI_LL_ERROR, "open gpio error");
            return;
        }
        sprintf(str_buf, "%d", padctl);
        if (write(fd, str_buf, strlen(str_buf)) < 0)
        {
            hudi_log(HUDI_LL_ERROR, "write error");
        }
        close(fd);
        usleep(10*1000);
    }
}

int hudi_gpio_configure(pinpad_e padctl, hudi_gpio_pinset_t pinset)
{
    int fd = -1;
    int ret = -1;
    char str_buf[64] = {0};

    if (padctl == HUDI_INVALID_VALUE_32)
        return ret;

    _hudi_gpio_active(padctl);
    // echo in > /sys/class/gpio/gpio8/direction
    sprintf(str_buf, "/sys/class/gpio/gpio%d/direction", padctl);
    fd = open(str_buf, O_WRONLY);
    if (fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "open gpio error");
        return ret;
    }

    if (HUDI_GPIO_DIR_INPUT == pinset)
        sprintf(str_buf, "in");
    else if (HUDI_GPIO_DIR_OUTPUT == pinset)
        sprintf(str_buf, "out");

    if (write(fd, str_buf, strlen(str_buf)) < 0)
        hudi_log(HUDI_LL_ERROR, "write error");
    else
        ret = 0;

    close(fd);

    return ret;
}

void hudi_gpio_output_set(pinpad_e padctl, bool val)
{
    int fd = -1;
    char str_buf[64] = {0};

    if (padctl == HUDI_INVALID_VALUE_32)
        return;

    _hudi_gpio_active(padctl);
    sprintf(str_buf, "/sys/class/gpio/gpio%d/value", padctl);
    fd = open(str_buf, O_WRONLY);
    if (fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "open gpio error");
        return;
    }

    if (val)
        sprintf(str_buf, "1");
    else
        sprintf(str_buf, "0");

    if (write(fd, str_buf, strlen(str_buf)) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "write error");
    }
    close(fd);

    return;
}

int hudi_gpio_input_get(pinpad_e padctl)
{
    int fd = -1;
    int ret = -1;
    int value = -1;
    char buffer[16] = {0};
    char str_buf[64] = {0};

    if (padctl == HUDI_INVALID_VALUE_32)
        return ret;

    _hudi_gpio_active(padctl);
    //"cat /sys/class/gpio/gpio8/value"
    sprintf(str_buf, "/sys/class/gpio/gpio%d/value", padctl);
    fd = open(str_buf, O_RDWR);

    if (fd < 0){
        hudi_log(HUDI_LL_ERROR, "open gpio error");
        return value;
    }

    ret = read(fd, buffer, sizeof(buffer));
    if (ret > 0)
    {
        value = atoi(buffer);
    }
    close(fd);
    
    return value;
}

void hudi_gpio_close(pinpad_e padctl)
{
    int fd = -1;
    char str_buf[64] = {0};

    if (padctl == HUDI_INVALID_VALUE_32)
        return;

    //"echo 8 > /sys/class/gpio/unexport"
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "open gpio error");
        return;
    }
    sprintf(str_buf, "%d", padctl);
    if (write(fd, str_buf, strlen(str_buf)) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "write error");
    }
    close(fd);

    return;
}

#endif