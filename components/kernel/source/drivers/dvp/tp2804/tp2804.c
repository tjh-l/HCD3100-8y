#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/module.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <kernel/lib/console.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/list.h>
#include <nuttx/wqueue.h>
#include <hcuapi/virtuart.h>
#include <hcuapi/tvtype.h>
#include <hcuapi/dvpdevice.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>
#include <kernel/ld.h>
#include <linux/io.h>
#include <linux/delay.h>
#include "../dvp_i2c.h"
#include "tp2804_reg.h"

struct tp2804_chrdev {
    const char *i2c_devpath;
    uint32_t    i2c_addr;
    uint32_t    b_support_input_detect;
    uint32_t    reset_gpio;
    uint32_t    reset_active;
};

static void tp2804_dts_init(struct tp2804_chrdev *chrdev)
{
    int np;

    np = fdt_get_node_offset_by_path("/hcrtos/dvp-tp2804");

    chrdev->reset_gpio = 0xffffffff;
    chrdev->i2c_addr = 0xffffffff;
    chrdev->reset_active = 0xffffffff;

    if (np >= 0) {
        if (chrdev != NULL) {
            fdt_get_property_string_index(np , "i2c-devpath" , 0 , &chrdev->i2c_devpath);
            fdt_get_property_u_32_index(np , "i2c-addr" , 0 , (u32 *)(&chrdev->i2c_addr));
            fdt_get_property_u_32_index(np , "reset-gpio" , 0 , (u32 *)(&chrdev->reset_gpio));
            fdt_get_property_u_32_index(np , "reset-gpio" , 1 , (u32 *)(&chrdev->reset_active));
            if (chrdev->reset_active == GPIO_ACTIVE_LOW) {
                chrdev->reset_active = 0;
            } else {
                chrdev->reset_active = 1;
            }
        }
    }
}

static void tp2804_hw_reset(struct tp2804_chrdev *chrdev)
{
    if (chrdev->reset_gpio != 0xffffffff) {
        printf("chrdev->reset_gpio=%ld chrdev->reset_active=%ld\n" ,
               chrdev->reset_gpio , chrdev->reset_active);
        gpio_configure(chrdev->reset_gpio , GPIO_DIR_OUTPUT);
        gpio_set_output(chrdev->reset_gpio , chrdev->reset_active);
        msleep(5);
        gpio_set_output(chrdev->reset_gpio , 1 - chrdev->reset_active);
        msleep(100);
    }
}

static int tp2804_read_reg(uint8_t cmd , uint8_t *p_data , uint32_t len)
{
    int ret = 0;

    ret = dvp_i2c_write(&cmd , 1);

    if (ret < 0) {
        printf("%s %d fail\n" , __FUNCTION__ , __LINE__);
        return ret;
    }

    ret = dvp_i2c_read(p_data , len);
    if (ret < 0) {
        return ret;
    }

    return ret;
}

static int tp2804_write_reg(uint8_t cmd , uint8_t *p_data , uint32_t len)
{
    int ret = 0;

    uint8_t *array = NULL;
    uint8_t i = 0;
    uint8_t value = 0;

    array = malloc(len + 1);
    if (NULL == array) {
        return -1;
    }

    array[0] = cmd;
    for (i = 0;i < len;i++) {
        array[i + 1] = p_data[i];
    }
    ret = dvp_i2c_write(array , len + 1);

    free(array);
    if (ret < 0) {
        printf("%s %d fail\n" , __FUNCTION__ , __LINE__);
        return -1;
    }
    tp2804_read_reg(cmd , &value , 1);

    return ret;
}

static bool tp2804_get_input_signal_status(uint8_t reg_value)
{
    return false;
}

static bool tp2804_get_input_signal_format_is_720p(uint8_t reg_value)
{
    return false;
}

static bool tp2804_get_input_signal_format_is_1080p(uint8_t reg_value)
{
    false;
}

static uint32_t tp2804_get_input_signal_frame_rate(uint8_t reg_value)
{
    return false;
}

static bool tp2804_get_input_signal_format(tvtype_e *tv_sys , bool *bProgressive)
{
    return false;
}

static int tp2804_init_config(void)
{
    return false;
}

static void tp2804_reset(void)
{
}

static int tp2804_set_tv_sys(tvtype_e tv_sys , bool bProgressive)
{
    int ret = 0;

    tp2804_reset();
    TP9951_sensor_init(VIN1 , HD25 , STD_HDA);
    return ret;
}

static void tp2804_set_input_port_num(uint8_t port_num)
{
}

static int tp2804_set_color_adjust(dvp_enhance_t * color_adjust)
{
    int ret = 0;
    char x_min = 0, x_max = 100;
    char y_min = 0, y_max = 0;
    char y = 0, x = color_adjust->value;
    
    switch (color_adjust->type) {
        case DVP_ENHANCE_BRIGHTNESS:
            y_min = -64;
            y_max = 63;
            y = y_min + ((x - x_min) * (y_max - y_min) / (x_max - x_min));
            ret = tp2804_write_reg(0x10,&y,1);
            break;
        case DVP_ENHANCE_CONTRAST:
            y_min = 0;
            y_max = 127;
            y = y_min + ((x - x_min) * (y_max - y_min) / (x_max - x_min));
            ret = tp2804_write_reg(0x11,&y,1);
            break;
        case DVP_ENHANCE_SATURATION:
            y_min = 0;
            y_max = 127;
            y = y_min + ((x - x_min) * (y_max - y_min) / (x_max - x_min));
            ret = tp2804_write_reg(0x12,&y,1);
            break;
        case DVP_ENHANCE_HUE:
            y_min = -90;
            y_max = 90;
            y = y_min + ((x - x_min) * (y_max - y_min) / (x_max - x_min));
            ret = tp2804_write_reg(0x13,&y,1);
            break;
        default:
            ret = -1;
            break;    
    }

    if(ret < 0) {
        printf("tp2804_set_color_adjust fail\n");
        return ret;
    }

    return ret;
}

static int tp2804_open(struct file *file)
{
    struct tp2804_chrdev *chrdev = NULL;

    chrdev = malloc(sizeof(struct tp2804_chrdev));
    memset(chrdev , 0 , sizeof(struct tp2804_chrdev));
    file->f_priv = chrdev;

    tp2804_dts_init(chrdev);
    tp2804_hw_reset(chrdev);
    dvp_i2c_open(chrdev->i2c_devpath);
    dvp_i2c_set_slv_addr(chrdev->i2c_addr);
    chrdev->b_support_input_detect = false;

    return 0;
}

static int tp2804_close(struct file *file)
{
    struct tp2804_chrdev *chrdev = file->f_priv;

    printf("tp2804_close\n");
    dvp_i2c_close();
    if (chrdev) {
        free(chrdev);
    }
    file->f_priv = NULL;
    printf("tp2804_close end\n");

    return 0;
}

static int tp2804_ioctl(struct file *file , int cmd , unsigned long arg)
{
    int ret = -1;

    struct tp2804_chrdev *chrdev = file->f_priv;

    switch (cmd) {
        case DVPDEVICE_SET_INPUT_TVTYPE:
        {
            struct dvp_device_tvtype *device_tvtype = (struct dvp_device_tvtype *)arg;
            ret = tp2804_set_tv_sys(device_tvtype->tv_sys , device_tvtype->b_progressive);
            break;
        }
        case DVPDEVICE_GET_INPUT_DETECT_SUPPORT:
        {
            uint32_t *b_support_input_detect;
            b_support_input_detect = (uint32_t *)arg;
            *b_support_input_detect = chrdev->b_support_input_detect;
            ret = 0;
            break;
        }
        case DVPDEVICE_GET_INPUT_TVTYPE:
        {
            struct dvp_device_tvtype *device_tvtype = (struct dvp_device_tvtype *)arg;
            if (tp2804_get_input_signal_format(&device_tvtype->tv_sys , &device_tvtype->b_progressive) == true) {
                ret = 0;
            } else {
                ret = -1;
            }
            break;
        }
        case DVPDEVICE_SET_INPUT_PORT:
        {
            uint8_t port = (uint8_t)arg;
            tp2804_set_input_port_num(port);
            ret = 0;
            break;
        }
        case DVPDEVICE_SET_ENHANCE:
        {
            dvp_enhance_t *device_color = (dvp_enhance_t *)arg;
            ret = tp2804_set_color_adjust(device_color);
            break;
        }
        default:
            ret = -1;
            break;
    }

    return ret;
}

static const struct file_operations tp2804_fops = {
    .open = tp2804_open,
    .close = tp2804_close,
    .read = dummy_read,
    .write = dummy_write,
    .ioctl = tp2804_ioctl,
};

static int tp2804_init(void)
{
    int np;
    const char *status;

    np = fdt_node_probe_by_path("/hcrtos/dvp-tp2804");
    if (np < 0) {
        printf("cannot found efsue in DTS\n");
        return 0;
    }

    if (!fdt_get_property_string_index(np , "status" , 0 , &status) &&
        !strcmp(status , "disabled"))
        return 0;

    register_driver("/dev/dvp-tp2804" , &tp2804_fops , 0666 , NULL);

    return 0;
}

module_driver(tp2804 , tp2804_init , NULL , 0)