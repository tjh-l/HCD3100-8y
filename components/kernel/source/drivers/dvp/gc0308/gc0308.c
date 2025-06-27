#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <stdio.h>
#include <fcntl.h>
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
#include <hcuapi/pwm.h>
#include <dt-bindings/gpio/gpio.h>
#include <kernel/ld.h>
#include <linux/io.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include "../dvp_i2c.h"
#include "gc0308_reg.h"

struct gc0308_chrdev
{
    const char *i2c_devpath;
    uint32_t    i2c_addr;
    uint32_t    reset_gpio;
    uint32_t    reset_active;
    uint32_t    b_support_input_detect;
    uint32_t    power_gpio;
    uint32_t    power_active;
};

static void gc0308_dts_init(struct gc0308_chrdev *chrdev)
{
    int np;

    np = fdt_get_node_offset_by_path("/hcrtos/dvp-gc0308");

    chrdev->reset_gpio = 0xffffffff;
    chrdev->i2c_addr = 0xffffffff;
    chrdev->reset_active = 0xffffffff;

    if (np >= 0)
    {
        if (chrdev != NULL)
        {
            fdt_get_property_string_index(np , "i2c-devpath" , 0 , &chrdev->i2c_devpath);
            fdt_get_property_u_32_index(np , "i2c-addr" , 0 , (u32 *)(&chrdev->i2c_addr));
            fdt_get_property_u_32_index(np , "reset-gpio" , 0 , (u32 *)(&chrdev->reset_gpio));
            fdt_get_property_u_32_index(np , "reset-gpio" , 1 , (u32 *)(&chrdev->reset_active));
            fdt_get_property_u_32_index(np , "power-gpio" , 0 , (u32 *)(&chrdev->power_gpio));
            fdt_get_property_u_32_index(np , "power-gpio" , 1 , (u32 *)(&chrdev->power_active));
            if (chrdev->reset_gpio != 0xffffffff)
            {
                if (chrdev->reset_active == GPIO_ACTIVE_LOW)
                {
                    chrdev->reset_active = 0;
                }
                else
                {
                    chrdev->reset_active = 1;
                }
            }
            if (chrdev->power_gpio != 0xffffffff)
            {
                if (chrdev->power_active == GPIO_ACTIVE_LOW)
                {
                    chrdev->power_active = 0;
                }
                else
                {
                    chrdev->power_active = 1;
                }
            }
        }
    }
}

static int gc0308_read_reg(uint8_t cmd , uint8_t *p_data , uint32_t len)
{
    int ret = 0;

    ret = dvp_i2c_write(&cmd , 1);

    if (ret < 0)
    {
        printk("%s %d fail\n" , __FUNCTION__ , __LINE__);
        return ret;
    }

    ret = dvp_i2c_read(p_data , len);
    if (ret < 0)
    {
        return ret;
    }

    return ret;
}

static int gc0308_write_reg(uint8_t cmd , uint8_t *p_data , uint32_t len)
{
    int ret = 0;

    uint8_t *array = NULL;
    uint8_t i = 0;
    //uint8_t value = 0;

    array = malloc(len + 1);
    if (NULL == array)
    {
        return -1;
    }

    array[0] = cmd;
    for (i = 0;i < len;i++)
    {
        array[i + 1] = p_data[i];
    }
    ret = dvp_i2c_write(array , len + 1);

    free(array);
    if (ret < 0)
    {
        printk("%s %d fail\n" , __FUNCTION__ , __LINE__);
        return -1;
    }

    return ret;
}

static int gc0308_write_reg_byte(uint8_t cmd , uint8_t data)
{
    int ret = 0;
    uint8_t array[2];

    array[0] = cmd;
    array[1] = data;

    ret = dvp_i2c_write(array , 2);
    if (ret < 0)
    {
        printk("%s %d fail\n" , __FUNCTION__ , __LINE__);
    }
    //printk("%s cmd = 0x%x data=0x%x\n" , __FUNCTION__ , cmd , data);
    return ret;
}

static int gc0308_init_config(struct regval_list *reg_list , int len)
{
    struct regval_list *p_reg_list = reg_list;
    int ret = -1;
    int i = 0;

    printf("enter %s %d\n",__FUNCTION__,len);
    for (i = 0;i < len / 2;i++)
    {
        ret = gc0308_write_reg(p_reg_list[i].reg_num , &(p_reg_list[i].value) , 1);
    }
    printf("exit %s\n" , __FUNCTION__);

    return ret;
}

static void gc0308_hw_reset(struct gc0308_chrdev *chrdev)
{
    if (chrdev->reset_gpio != 0xffffffff)
    {
        printf("chrdev->reset_gpio=%ld chrdev->reset_active=%ld\n" ,
               chrdev->reset_gpio , chrdev->reset_active);
        printf("chrdev->power_gpio=%ld chrdev->power_active=%ld\n" ,
               chrdev->power_gpio , chrdev->power_active);

        gpio_configure(chrdev->power_gpio , GPIO_DIR_OUTPUT);

        gpio_set_output(chrdev->power_gpio , 1 - chrdev->power_active); // POWER OFF;
        msleep(10);
        gpio_set_output(chrdev->power_gpio , chrdev->power_active); // POWER ON;
        msleep(10);

        gpio_configure(chrdev->reset_gpio , GPIO_DIR_OUTPUT);
        gpio_set_output(chrdev->reset_gpio , 1 - chrdev->reset_active); //结束复位
        msleep(10);
        gpio_set_output(chrdev->reset_gpio , chrdev->reset_active);
        msleep(10);
        gpio_set_output(chrdev->reset_gpio , 1 - chrdev->reset_active); //结束复位
        msleep(100);
    }
}

static int gc0308_open(struct file *file)
{
    struct gc0308_chrdev *chrdev = NULL;

    chrdev = malloc(sizeof(struct gc0308_chrdev));
    memset(chrdev , 0 , sizeof(struct gc0308_chrdev));
    file->f_priv = chrdev;

    gc0308_dts_init(chrdev);
    gc0308_hw_reset(chrdev);
    dvp_i2c_open(chrdev->i2c_devpath);
    dvp_i2c_set_slv_addr(chrdev->i2c_addr);

    chrdev->b_support_input_detect = false;

    return 0;
}

static int gc0308_close(struct file *file)
{
    struct gc0308_chrdev *chrdev = file->f_priv;

    printk("gc0308_close\n");
    dvp_i2c_close();
    if (chrdev)
    {
        free(chrdev);
    }
    file->f_priv = NULL;
    printk("gc0308_close end\n");

    return 0;
}

static int gc0308_ioctl(struct file *file , int cmd , unsigned long arg)
{
    int ret = -1;

    struct gc0308_chrdev *chrdev = file->f_priv;

    switch (cmd)
    {
        case DVPDEVICE_SET_INPUT_TVTYPE:
        {
            ret = 0;
            gc0308_init_config(module_init_regs , sizeof(module_init_regs));
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
            ret = -1;
            break;
        }
        case DVPDEVICE_SET_INPUT_PORT:
        {
            ret = 0;
            break;
        }
        default:
            ret = -1;
            break;
    }

    return ret;
}

static const struct file_operations gc0308_fops = {
    .open = gc0308_open,
    .close = gc0308_close,
    .read = dummy_read,
    .write = dummy_write,
    .ioctl = gc0308_ioctl,
};

static int gc0308_init(void)
{
    int np;
    const char *status;

    printf("gc0308_init\n");
    np = fdt_node_probe_by_path("/hcrtos/dvp-gc0308");
    if (np < 0)
    {
        printf("cannot found dvp-gc0308DTS\n");
        return 0;
    }

    if (!fdt_get_property_string_index(np , "status" , 0 , &status) &&
        !strcmp(status , "disabled"))
        return 0;

    register_driver("/dev/dvp-gc0308" , &gc0308_fops , 0666 , NULL);

    return 0;
}

module_driver(gc0308 , gc0308_init , NULL , 0)
