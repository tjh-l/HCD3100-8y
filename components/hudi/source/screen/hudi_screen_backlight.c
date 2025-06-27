/**
 * @file
 * @brief                hudi screen backlight interface
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
#include <ctype.h>
#include <unistd.h>
#include <hcuapi/pinmux.h>
#include <hcuapi/pinpad.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_screen.h>

#include "hudi_gpio_ctrl.h"
#include "hudi_screen_inter.h"

/**
 * @date 2024-08-30
 * @brief hc backlight function
 * @note Only GPIO function dts config

    backlight {
        backlight-gpios-rtos = <PINPAD_T04 GPIO_ACTIVE_HIGH PINPAD_T02 GPIO_ACTIVE_HIGH>;
        default-off;
        status = "okay";
    };

 * @note pwm backlight levels dts config

    pwm@0 {
        pinmux-active = <PINPAD_L00 4>;
        devpath = "/dev/pwm0";
        status = "okay";
    };

    backlight {
        backlight-frequency = <10000>;
        backlight-pwm-polarity = <0>;
        backlight-pwmdev = "/dev/pwm0";
        brightness-levels = <0 4 8 16 32 64 128 255>;
        default-brightness-level = <7>;//max
        // backlight-gpios-rtos = <PINPAD_T04 GPIO_ACTIVE_HIGH>;//Can be added
        default-off;
        status = "okay";
    };

 * @note set backlight range dts config

    pwm@0 {
        pinmux-active = <PINPAD_L00 4>;
        devpath = "/dev/pwm0";
        status = "okay";
    };

    backlight {
        backlight-frequency = <10000>;
        backlight-pwm-polarity = <0>;
        backlight-pwmdev = "/dev/pwm0";
        default-brightness-level = <80>; // max
        scale = <100>;
        range = "20-100";
        // backlight-gpios-rtos = <PINPAD_T04 GPIO_ACTIVE_HIGH>;//Can be added
        default-off;
        status = "okay";
    };
*/

static hudi_backlight_pwm_t *g_hudi_blpwm = NULL;

#ifdef __HCRTOS__

#include <hcuapi/pwm.h>
#include <freertos/FreeRTOS.h>
#include <kernel/lib/fdt_api.h>

static SemaphoreHandle_t g_hudi_backlight_mutex = NULL;

static int _hudi_backlight_mutex_lock(void)
{
    if (g_hudi_backlight_mutex == NULL)
    {
        g_hudi_backlight_mutex = xSemaphoreCreateMutex();
    }
    xSemaphoreTake(g_hudi_backlight_mutex, portMAX_DELAY);
    return 0;
}

static int _hudi_backlight_mutex_unlock(void)
{
    xSemaphoreGive(g_hudi_backlight_mutex);
    return 0;
}

static int _hudi_backlight_pwm_duty_set(unsigned int duty_cycle)
{
    struct pwm_info_s info = {0};
    int fd = 0;
    if (strlen(g_hudi_blpwm->pwmbl_path) == 0)
        return 0;

    fd = open(g_hudi_blpwm->pwmbl_path, O_RDWR);
    if (fd <= 0)
    {
        hudi_log(HUDI_LL_ERROR, "backlight open %s error\n", g_hudi_blpwm->pwmbl_path);
        return -1;
    }

    info.period_ns = 1000000000 / g_hudi_blpwm->info.pwm_frequency;
    info.polarity = g_hudi_blpwm->info.pwm_polarity;
    if (duty_cycle == HUDI_BACKLIGHT_PWM_SET_STOP)
    {
        ioctl(fd, PWMIOC_STOP, 0);
    }
    else if (g_hudi_blpwm->is_level_pwm_work)
    {
        if (duty_cycle >= g_hudi_blpwm->info.levels_count)
        {
            duty_cycle = g_hudi_blpwm->dft_brightness_max;
        }
        info.duty_ns = info.period_ns * g_hudi_blpwm->dts_levels[duty_cycle] / g_hudi_blpwm->scale;
        ioctl(fd, PWMIOC_SETCHARACTERISTICS, &info);
        ioctl(fd, PWMIOC_START);
    }
    else
    {
        if(duty_cycle > g_hudi_blpwm->scale)
        {
            duty_cycle = g_hudi_blpwm->scale;
        }

        if((g_hudi_blpwm->info.levels_count - 1) < duty_cycle)
        {
            duty_cycle = g_hudi_blpwm->range.upper_limit;
        }
        else
        {
            duty_cycle = g_hudi_blpwm->range.lower_limit + duty_cycle;
        }

        info.duty_ns = info.period_ns * duty_cycle / g_hudi_blpwm->scale;
        ioctl(fd, PWMIOC_SETCHARACTERISTICS, &info);
        ioctl(fd, PWMIOC_START);
    }
    close(fd);

    return 0;
}

static int _hudi_fdt_get_property_data_by_name(int offset, const char *name, int *length)
{
    if (fdt_get_property_data_by_name(offset, name, length) == NULL)
        return -1;
    else
        return 0;
}

int _hudi_fdt_node_probe_by_path(const char *name)
{
    return fdt_node_probe_by_path(name);
}

static int _hudi_fdt_get_property_u_32_index(int offset, const char *name, int index, unsigned int *outval)
{
    return fdt_get_property_u_32_index(offset, name, index, outval);
}

static int _hudi_fdt_get_property_string_index(int offset, const char *name, int index, const char **outval)
{
    return fdt_get_property_string_index(offset, name, index, outval);
}

static int _hudi_dts_backlight_info_get(int np)
{
    const char *papam = NULL;
    if (fdt_get_property_string_index(np, "backlight-pwmdev", 0, &papam) == 0)
    {
        strncpy(g_hudi_blpwm->pwmbl_path, papam, sizeof(g_hudi_blpwm->pwmbl_path));
    }
    if (fdt_get_property_string_index(np, "range", 0, &papam) == 0)
    {
        unsigned int number[2] = {0};
        int isNumber = 0;
        int number_count = 0;
        for (int i = 0; papam[i] != '\0'; i++)
        {
            if (isspace((unsigned char)papam[i]))
            {
                continue;
            }
            if (isdigit((unsigned char)papam[i]))
            {
                isNumber = 1;
                number[number_count] = number[number_count] * 10 + (papam[i] - '0');
            } else if (isNumber)
            {
                isNumber = 0;
                number_count++;
                if(number_count > 1)
                    break;
            }
        }
        if(number[0] > number[1])
        {
            g_hudi_blpwm->range.upper_limit = number[0];
            g_hudi_blpwm->range.lower_limit = number[1];
        } else {
            g_hudi_blpwm->range.lower_limit = number[0];
            g_hudi_blpwm->range.upper_limit = number[1];
        }
    }
}

#else

#include <errno.h>

#define HUDI_PWM_EXPORT_PATH "/sys/class/pwm/pwmchip0/export"
#define HUDI_PWM_PATH "/sys/class/pwm/pwmchip0/pwm"
#define HUDI_PWM_PERIOD_PATH "/period"
#define HUDI_PWM_ENABLE_PATH "/enable"
#define HUDI_PWM_POLARITY_PATH "/polarity"
#define HUDI_PWM_DUTY_CYCLE_PATH "/duty_cycle"

#define HUDI_DTS_BACKLIGHT_PATH    "/proc/device-tree/hcrtos/backlight"

static pthread_mutex_t g_hudi_backlight_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_backlight_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_backlight_mutex);

    return 0;
}

static int _hudi_backlight_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_backlight_mutex);

    return 0;
}

static int write_to_file(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "open %s error", path);
        return -1;
    }

    if (write(fd, value, strlen(value)) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "write %s error", path);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static char *_hudi_extract_last_digit_str(const char *path)
{
    char *last_digit = NULL;

    for (char *p = (char*)path + strlen(path) - 1; p >= path; p--)
    {
        if (isdigit(*p))
        {
            last_digit = p;
            break;
        }
    }

    if (last_digit)
    {
        return last_digit;
    }

    return NULL;
}

static int _hudi_backlight_pwm_duty_set(unsigned int duty_cycle)
{
    if (strlen(g_hudi_blpwm->pwmbl_path) == 0)
        return 0;

    unsigned int duty_ns;
    char path[64] = {0};
    char param[32] = {0};

    int export_fd = open(HUDI_PWM_EXPORT_PATH, O_WRONLY);
    if (export_fd < 0) {
        hudi_log(HUDI_LL_ERROR, "open export error");
        return -1;
    }

    char *pwm_export = _hudi_extract_last_digit_str(g_hudi_blpwm->pwmbl_path);
    if (pwm_export == NULL)
    {
        hudi_log(HUDI_LL_ERROR, "Failed to extract PWM index from path");
        close(export_fd);
        return -1;
    }

    if (write(export_fd, pwm_export, 1) < 0)
    {
        if (errno != EBUSY)
        { // Ignore EBUSY error if PWM is already exported
            hudi_log(HUDI_LL_ERROR, "write export error");
            close(export_fd);
            return -1;
        }
    }
    close(export_fd);

    snprintf(param, sizeof(param), "%u", 1000000000 / g_hudi_blpwm->info.pwm_frequency);
    snprintf(path, sizeof(path), "%s%s%s", HUDI_PWM_PATH, pwm_export, HUDI_PWM_PERIOD_PATH);
    if (write_to_file(path, param) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "set period error");
        return -1;
    }

    if (duty_cycle == HUDI_BACKLIGHT_PWM_SET_STOP)
    {
        snprintf(path, sizeof(path), "%s%s%s", HUDI_PWM_PATH, pwm_export, HUDI_PWM_ENABLE_PATH);
        if (write_to_file(path, "0") < 0)
        {
            hudi_log(HUDI_LL_ERROR, "disable pwm error");
            return -1;
        }
    }
    else
    {
        if (g_hudi_blpwm->is_level_pwm_work)
        {
            if (duty_cycle >= g_hudi_blpwm->info.levels_count)
            {
                duty_cycle = g_hudi_blpwm->dft_brightness_max;
            }
            duty_ns = (1000000000 / g_hudi_blpwm->info.pwm_frequency) * g_hudi_blpwm->dts_levels[duty_cycle] / g_hudi_blpwm->scale;
        }
        else
        {
            if (duty_cycle > g_hudi_blpwm->scale)
            {
                duty_cycle = g_hudi_blpwm->scale;
            }

            if ((g_hudi_blpwm->info.levels_count - 1) < duty_cycle)
            {
                duty_cycle = g_hudi_blpwm->range.upper_limit;
            }
            else
            {
                duty_cycle = g_hudi_blpwm->range.lower_limit + duty_cycle;
            }

            duty_ns = (1000000000 / g_hudi_blpwm->info.pwm_frequency) * duty_cycle / g_hudi_blpwm->scale;
        }

        snprintf(param, sizeof(param), "%u", duty_ns);
        snprintf(path, sizeof(path), "%s%s%s", HUDI_PWM_PATH, pwm_export, HUDI_PWM_DUTY_CYCLE_PATH);
        if (write_to_file(path, param) < 0)
        {
            hudi_log(HUDI_LL_ERROR, "set duty cycle error");
            return -1;
        }

        snprintf(path, sizeof(path), "%s%s%s", HUDI_PWM_PATH, pwm_export, HUDI_PWM_ENABLE_PATH);
        if (write_to_file(path, "1") < 0)
        {
            hudi_log(HUDI_LL_ERROR, "enable pwm error");
            return -1;
        }
    }

    return 0;
}

static int _hudi_dts_length_get(const char *path, int *length)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        hudi_log(HUDI_LL_DEBUG, "open %s error", path);
        return -1;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size < 0) {
        hudi_log(HUDI_LL_ERROR, "open %s error", path);
        close(fd);
        return -1;
    }

    close(fd);
    if (length != NULL)
        *length = (int)file_size;
    return 0;
}

static int _hudi_dts_uint32_index_get(const char *path, unsigned int offset, unsigned int *outval)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        hudi_log(HUDI_LL_DEBUG, "open %s error", path);
        return -1;
    }
    uint8_t buf[4];
    if (lseek(fd, offset, SEEK_SET) < 0)
    {
        hudi_log(HUDI_LL_ERROR, "open %s error", path);
        close(fd);
        return -1;
    }
    ssize_t bytesRead = read(fd, buf, 4);
    if (bytesRead != 4)
    {
        hudi_log(HUDI_LL_ERROR, "open %s error", path);
        close(fd);
        return -1;
    }
    *outval = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
    close(fd);

    return 0;
}

static int _hudi_dts_string_get(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        hudi_log(HUDI_LL_DEBUG, "open %s error", path);
        return -1;
    }

    int bytes_read = read(fd, string, size - 1);
    if (bytes_read < 0)
    {
        hudi_log(HUDI_LL_ERROR, "open %s error", path);
        close(fd);
        return -1;
    }
    string[bytes_read] = '\0';
    close(fd);

    return 0;
}

static int _hudi_fdt_get_property_data_by_name(int offset, const char *name, int *length)
{
    char path[64] = {0};
    snprintf(path, sizeof(path), "%s/%s", HUDI_DTS_BACKLIGHT_PATH, name);
    return _hudi_dts_length_get(path, length);
}

static int _hudi_fdt_get_property_u_32_index(int offset, const char *name, int index, unsigned int *outval)
{
    char path[64] = {0};
    snprintf(path, sizeof(path), "%s/%s", HUDI_DTS_BACKLIGHT_PATH, name);
    return _hudi_dts_uint32_index_get(path, index * 4, outval);
}

int _hudi_fdt_node_probe_by_path(const char *name)
{
    char path[64] = {0};
    char outval[32] = {0};
    snprintf(path, sizeof(path), "%s/status", HUDI_DTS_BACKLIGHT_PATH, name);
    if (_hudi_dts_string_get(path, outval, sizeof(path)) == 0)
    {
        if (strcmp(outval, "okay") == 0)
        {
            return 0;
        }
    }
    return -1;
}

static int _hudi_fdt_get_property_string_index(const char *name, char *outval, int size)
{
    char path[64] = {0};
    snprintf(path, sizeof(path), "%s/%s", HUDI_DTS_BACKLIGHT_PATH, name);
    return _hudi_dts_string_get(path, outval, size);
}

static int _hudi_dts_backlight_info_get(int np)
{
    char papam[32] = {0};
    if (_hudi_fdt_get_property_string_index("backlight-pwmdev", papam, sizeof(papam)) == 0)
    {
        strncpy(g_hudi_blpwm->pwmbl_path, papam, sizeof(g_hudi_blpwm->pwmbl_path));
    }
    memset(papam , 0, sizeof(papam));
    if (_hudi_fdt_get_property_string_index("range", papam, sizeof(papam)) == 0)
    {
        unsigned int number[2] = {0};
        int isNumber = 0;
        int number_count = 0;
        for (int i = 0; papam[i] != '\0'; i++)
        {
            if (isspace((unsigned char)papam[i]))
            {
                continue;
            }
            if (isdigit((unsigned char)papam[i]))
            {
                isNumber = 1;
                number[number_count] = number[number_count] * 10 + (papam[i] - '0');
            } else if (isNumber)
            {
                isNumber = 0;
                number_count++;
                if(number_count > 1)
                    break;
            }
        }
        if(number[0] > number[1])
        {
            g_hudi_blpwm->range.upper_limit = number[0];
            g_hudi_blpwm->range.lower_limit = number[1];
        } else {
            g_hudi_blpwm->range.lower_limit = number[0];
            g_hudi_blpwm->range.upper_limit = number[1];
        }
    }
}

#endif

static void _hudi_backlight_gpio_status_set(char value)
{
    unsigned int i = 0;
    for (i = 0; i < g_hudi_blpwm->gpio_backlight.num; i++)
    {
        hudi_gpio_configure(g_hudi_blpwm->gpio_backlight.pad[i].padctl, HUDI_GPIO_DIR_OUTPUT);
        if (value != 0)
        {
            hudi_gpio_output_set(g_hudi_blpwm->gpio_backlight.pad[i].padctl, !g_hudi_blpwm->gpio_backlight.pad[i].active);
        }
        else
        {
            hudi_gpio_output_set(g_hudi_blpwm->gpio_backlight.pad[i].padctl, g_hudi_blpwm->gpio_backlight.pad[i].active);
        }
    }
}

static int _hudi_backlight_start(unsigned int val)
{
    int ret = 0;
    g_hudi_blpwm->info.brightness_value = val;
    hudi_log(HUDI_LL_DEBUG, "set brightness_value %u\n", val);
    ret = _hudi_backlight_pwm_duty_set(g_hudi_blpwm->info.brightness_value);
    _hudi_backlight_gpio_status_set(g_hudi_blpwm->info.brightness_value);
    g_hudi_blpwm->start_flag = 1;
    return ret;
}

static int _hudi_backlight_stop(void)
{
    _hudi_backlight_pwm_duty_set(HUDI_BACKLIGHT_PWM_SET_STOP);
    _hudi_backlight_gpio_status_set(0);
    return 0;
}

static int _hudi_backlight_get_info(hudi_backlight_info_t *info)
{
    if (info == NULL)
        return -1;

    memcpy(info, &g_hudi_blpwm->info, sizeof(hudi_backlight_info_t));
    return 0;
}

static int _hudi_backlight_set_info(hudi_backlight_info_t *info)
{
    if(info == NULL)
        return -1;

    g_hudi_blpwm->info.pwm_frequency = info->pwm_frequency;
    g_hudi_blpwm->info.brightness_value = info->brightness_value;
    g_hudi_blpwm->info.pwm_polarity = info->pwm_polarity;

    _hudi_backlight_start(g_hudi_blpwm->info.brightness_value);
    return 0;
}

static int _hudi_backlight_init(const char *node)
{
    int ret;
    unsigned int i = 0;
    unsigned int tmpVal = 0;
    char path[32] = {0};
    char str_range[32] = {0};
    int np = _hudi_fdt_node_probe_by_path(node);

    if (np < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Dts %s node not found\n", node);
        return -1;
    }

    if (g_hudi_blpwm != NULL)
    {
        return 0;
    }

    g_hudi_blpwm = (hudi_backlight_pwm_t *)malloc(sizeof(hudi_backlight_pwm_t));
    if (g_hudi_blpwm == NULL)
    {
        hudi_log(HUDI_LL_ERROR, "malloc error\n");
        return -1;
    }
    memset(g_hudi_blpwm, 0, sizeof(hudi_backlight_pwm_t));

    _hudi_dts_backlight_info_get(np);

    if (_hudi_fdt_get_property_data_by_name(np, "backlight-gpios-rtos", &g_hudi_blpwm->gpio_backlight.num) != 0)
    {
        g_hudi_blpwm->gpio_backlight.num = 0;
    } 
    else
    {
        g_hudi_blpwm->info.brightness_value = 1;
    }

    g_hudi_blpwm->gpio_backlight.num >>= 3;

    if (g_hudi_blpwm->gpio_backlight.num > HUDI_BACKLIGHT_PINPAD_GPIO_NUM)
    {
        g_hudi_blpwm->gpio_backlight.num = HUDI_BACKLIGHT_PINPAD_GPIO_NUM;
    }

    for (i = 0; i < g_hudi_blpwm->gpio_backlight.num; i++)
    {
        g_hudi_blpwm->gpio_backlight.pad[i].padctl = PINPAD_INVALID;
        g_hudi_blpwm->gpio_backlight.pad[i].active = 0;

        if (_hudi_fdt_get_property_u_32_index(np, "backlight-gpios-rtos", i * 2, &tmpVal) == 0)
            g_hudi_blpwm->gpio_backlight.pad[i].padctl = tmpVal;
        if (_hudi_fdt_get_property_u_32_index(np, "backlight-gpios-rtos", i * 2 + 1, &tmpVal) == 0)
            g_hudi_blpwm->gpio_backlight.pad[i].active = tmpVal;
    }

    g_hudi_blpwm->scale = 100;
    g_hudi_blpwm->is_level_pwm_work = 0;
    g_hudi_blpwm->info.levels_count = 0;
    g_hudi_blpwm->dft_brightness_max = 1;

    if (strlen(g_hudi_blpwm->pwmbl_path) > 0)
    {
        if (_hudi_fdt_get_property_u_32_index(np, "brightness-levels", 0, &tmpVal) == 0)
        {
            _hudi_fdt_get_property_data_by_name(np, "brightness-levels", &g_hudi_blpwm->info.levels_count);
            g_hudi_blpwm->info.levels_count >>= 2;
        }

        if (g_hudi_blpwm->info.levels_count > 0)
        {
            g_hudi_blpwm->scale = 0;
            g_hudi_blpwm->dts_levels = (unsigned int *)malloc(sizeof(unsigned int) * g_hudi_blpwm->info.levels_count);

            if(!g_hudi_blpwm->dts_levels)
            {
                hudi_log(HUDI_LL_ERROR, "malloc error\n");
                free(g_hudi_blpwm);
                return -1;
            }

            memset(g_hudi_blpwm->dts_levels, 0, sizeof(unsigned int) * g_hudi_blpwm->info.levels_count);
            for (i = 0; i < g_hudi_blpwm->info.levels_count; i++)
            {
                _hudi_fdt_get_property_u_32_index(np, "brightness-levels", i, &g_hudi_blpwm->dts_levels[i]);
                if (g_hudi_blpwm->dts_levels[i] > g_hudi_blpwm->scale)
                {
                    g_hudi_blpwm->dft_brightness_max = i;
                    g_hudi_blpwm->scale = g_hudi_blpwm->dts_levels[i];
                }
            }
            if(g_hudi_blpwm->info.levels_count < HUDI_BACKLIGHT_LEVEL_SIZE)
                memcpy(g_hudi_blpwm->info.levels, g_hudi_blpwm->dts_levels, g_hudi_blpwm->info.levels_count * sizeof(unsigned int));
            else
                memcpy(g_hudi_blpwm->info.levels, g_hudi_blpwm->dts_levels, HUDI_BACKLIGHT_LEVEL_SIZE * sizeof(unsigned int));
            g_hudi_blpwm->is_level_pwm_work = 1;
        }
        else
        {
            if (_hudi_fdt_get_property_u_32_index(np, "scale", 0, &tmpVal) == 0)
            {
                g_hudi_blpwm->scale = tmpVal;
            }
            g_hudi_blpwm->info.levels_count = g_hudi_blpwm->scale + 1;
            if (g_hudi_blpwm->range.upper_limit != 0)
            {
                if(g_hudi_blpwm->range.upper_limit > g_hudi_blpwm->scale)
                {
                    g_hudi_blpwm->range.upper_limit = g_hudi_blpwm->scale;
                    hudi_log(HUDI_LL_ERROR, "upper_limit > scale\n");
                }
                else
                {
                    g_hudi_blpwm->info.levels_count = g_hudi_blpwm->range.upper_limit - g_hudi_blpwm->range.lower_limit + 1;
                }
            }
            else
            {
                g_hudi_blpwm->range.lower_limit = 0;
                g_hudi_blpwm->range.upper_limit = g_hudi_blpwm->scale;
            }
            g_hudi_blpwm->dft_brightness_max = g_hudi_blpwm->info.levels_count - 1;
        }

        tmpVal = g_hudi_blpwm->dft_brightness_max;
        _hudi_fdt_get_property_u_32_index(np, "default-brightness-level", 0, &tmpVal);
        g_hudi_blpwm->info.default_brightness_level = g_hudi_blpwm->info.brightness_value = tmpVal;

        tmpVal = 0;
        _hudi_fdt_get_property_u_32_index(np, "backlight-pwm-polarity", 0, &tmpVal);
        g_hudi_blpwm->info.pwm_polarity = tmpVal;

        _hudi_fdt_get_property_u_32_index(np, "backlight-frequency", 0, &g_hudi_blpwm->info.pwm_frequency);
    }

    if (g_hudi_blpwm->scale == 0)
    {
        hudi_log(HUDI_LL_ERROR, "g_hudi_blpwm->scale == 0\n");
        free(g_hudi_blpwm->dts_levels);
        free(g_hudi_blpwm);
        return -1;
    }

    hudi_log(HUDI_LL_NOTICE, "scale : %d\n", g_hudi_blpwm->scale);
    hudi_log(HUDI_LL_NOTICE, "pwmdev : %s\n", g_hudi_blpwm->pwmbl_path);
    hudi_log(HUDI_LL_NOTICE, "frequency : %d\n", g_hudi_blpwm->info.pwm_frequency);
    hudi_log(HUDI_LL_NOTICE, "levels_count : %d\n", g_hudi_blpwm->info.levels_count);
    hudi_log(HUDI_LL_NOTICE, "default_brightness_level : %d\n", g_hudi_blpwm->info.default_brightness_level);
    hudi_log(HUDI_LL_NOTICE, "range : %d - %d\n", g_hudi_blpwm->range.lower_limit, g_hudi_blpwm->range.upper_limit);
    for (i = 0; i < g_hudi_blpwm->gpio_backlight.num; i++)
    {
        hudi_log(HUDI_LL_NOTICE, "pad<%d> : %d\n", g_hudi_blpwm->gpio_backlight.pad[i].padctl, g_hudi_blpwm->gpio_backlight.pad[i].active);
    }

    return 0;
}

int hudi_backlight_info_get(hudi_handle handle, hudi_backlight_info_t *info)
{
    hudi_backlight_instance_t *inst = (hudi_backlight_instance_t *)handle;

    if (!inst || !inst->inited || !g_hudi_blpwm)
    {
        hudi_log(HUDI_LL_ERROR, "Backlight not open\n");
        return -1;
    }

    _hudi_backlight_mutex_lock();

    if (_hudi_backlight_get_info(info) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "Backlight get info fail\n");
        _hudi_backlight_mutex_unlock();
        return -1;
    }

    _hudi_backlight_mutex_unlock();

    return 0;
}

int hudi_backlight_info_set(hudi_handle handle, hudi_backlight_info_t *info)
{
    hudi_backlight_instance_t *inst = (hudi_backlight_instance_t *)handle;

    if (!inst || !inst->inited || !g_hudi_blpwm)
    {
        hudi_log(HUDI_LL_ERROR, "Backlight not open\n");
        return -1;
    }

    _hudi_backlight_mutex_lock();

    if (_hudi_backlight_set_info(info) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "Backlight set info fail\n");
        _hudi_backlight_mutex_unlock();
        return -1;
    }

    _hudi_backlight_mutex_unlock();

    return 0;
}

int hudi_backlight_start(hudi_handle handle)
{
    hudi_backlight_instance_t *inst = (hudi_backlight_instance_t *)handle;

    if (!inst || !inst->inited || !g_hudi_blpwm)
    {
        hudi_log(HUDI_LL_ERROR, "Backlight not open\n");
        return -1;
    }

    _hudi_backlight_mutex_lock();

    if (_hudi_backlight_start(g_hudi_blpwm->info.brightness_value) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "Backlight start fail\n");
        _hudi_backlight_mutex_unlock();
        return -1;
    }

    _hudi_backlight_mutex_unlock();

    return 0;
}

int hudi_backlight_open(hudi_handle *handle)
{
    int ret = 0;
    hudi_backlight_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid backlight parameters\n");
        return -1;
    }

    _hudi_backlight_mutex_lock();

    if (g_hudi_blpwm == NULL)
    {
        ret = _hudi_backlight_init("/hcrtos/backlight");
    }

    if (ret == 0)
    {
        inst = (hudi_backlight_instance_t *)malloc(sizeof(hudi_backlight_instance_t));
        memset(inst, 0, sizeof(hudi_backlight_instance_t));
        inst->inited = 1;
        *handle = inst;
    }

    _hudi_backlight_mutex_unlock();

    return ret;
}

int hudi_backlight_close(hudi_handle handle)
{
    hudi_backlight_instance_t *inst = (hudi_backlight_instance_t *)handle;

    if (!inst || !inst->inited || !g_hudi_blpwm)
    {
        hudi_log(HUDI_LL_ERROR, "Backlight not open\n");
        return -1;
    }

    _hudi_backlight_mutex_lock();

    memset(inst, 0, sizeof(hudi_backlight_instance_t));
    free(inst);

    _hudi_backlight_mutex_unlock();

    return 0;
}