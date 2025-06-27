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
#include "bf3a03_reg.h"

struct bf3a03_chrdev
{
    const char *i2c_devpath;
    uint32_t    i2c_addr;

    const char *i2c1_devpath;
    uint32_t    i2c1_addr;

    uint32_t    b_support_input_detect;
    uint32_t    reset_gpio;
    uint32_t    reset_active;
    const char *pwm_devpath;
    int         pwm_fd;
    uint8_t     port;
    bool        b_init;
};

struct bf3a03_chrdev *g_bf3a03_chardev = NULL;

static void bf3a03_dts_init(struct bf3a03_chrdev *chrdev)
{
    int np;

    np = fdt_get_node_offset_by_path("/hcrtos/dvp-bf3a03");

    chrdev->reset_gpio = 0xffffffff;
    chrdev->i2c_addr = 0xffffffff;
    chrdev->reset_active = 0xffffffff;

    if (np >= 0)
    {
        if (chrdev != NULL)
        {
            fdt_get_property_string_index(np , "i2c-devpath" , 0 , &chrdev->i2c_devpath);
            fdt_get_property_u_32_index(np , "i2c-addr" , 0 , (u32 *)(&chrdev->i2c_addr));

            fdt_get_property_string_index(np , "i2c1-devpath" , 0 , &chrdev->i2c1_devpath);
            fdt_get_property_u_32_index(np , "i2c1-addr" , 0 , (u32 *)(&chrdev->i2c1_addr));

            fdt_get_property_u_32_index(np , "reset-gpio" , 0 , (u32 *)(&chrdev->reset_gpio));
            fdt_get_property_u_32_index(np , "reset-gpio" , 1 , (u32 *)(&chrdev->reset_active));
            fdt_get_property_string_index(np , "pwm-devpath" , 0 , &chrdev->pwm_devpath);
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
        }
    }
}

static void bf3a03_hw_reset(struct bf3a03_chrdev *chrdev)
{
    if (chrdev->reset_gpio != 0xffffffff)
    {
        printf("chrdev->reset_gpio=%ld chrdev->reset_active=%ld\n" ,
               chrdev->reset_gpio , chrdev->reset_active);
        gpio_configure(chrdev->reset_gpio , GPIO_DIR_OUTPUT);
        gpio_set_output(chrdev->reset_gpio , chrdev->reset_active);
        msleep(5);
        gpio_set_output(chrdev->reset_gpio , 1 - chrdev->reset_active);
        msleep(100);
    }
}

static int bf3a03_pwm_init(struct bf3a03_chrdev *chrdev)
{
    struct pwm_info_s info = { 0 };

    if (chrdev->pwm_devpath != NULL)
    {
        chrdev->pwm_fd = open(chrdev->pwm_devpath , O_RDWR);
        if (chrdev->pwm_fd < 0)
        {
            printf("%s open fail\n" , chrdev->pwm_devpath);
            return -1;
        }
    }
    info.polarity = 0;
    
    //74 37 -- 13.5Mhz
    info.period_ns = 74;
    info.duty_ns = 37;

    ioctl(chrdev->pwm_fd , PWMIOC_SETCHARACTERISTICS , (unsigned long)&info);

    ioctl(chrdev->pwm_fd , PWMIOC_START);

    return 0;
}

static int bf3a03_read_reg(uint8_t cmd , uint8_t *p_data , uint32_t len)
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

static int bf3a03_write_reg(uint8_t cmd , uint8_t *p_data , uint32_t len)
{
    int ret = 0;

    uint8_t *array = NULL;
    uint8_t i = 0;
    uint8_t value = 0;

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
    bf3a03_read_reg(cmd , &value , 1);
    printf("value = 0x%x\n", value);

    return ret;
}

static int bf3a03_init_config(struct regval_list *reg_list)
{
    struct regval_list *p_reg_list = reg_list;
    int ret = -1;

    for (;;)
    {
        if (p_reg_list->reg_num == 0xFF && p_reg_list->value == 0xFF)
        {
            break;
        }

        ret = bf3a03_write_reg(p_reg_list->reg_num , &(p_reg_list->value) , 1);
        p_reg_list++;
    }

    return ret;
}

static int bf3a03_select_i2c(struct bf3a03_chrdev *chrdev , int port)
{
    if (port == 0)
    {
        dvp_i2c_open(chrdev->i2c_devpath);
        dvp_i2c_set_slv_addr(chrdev->i2c_addr);
    }
    else
    {
        dvp_i2c_open(chrdev->i2c1_devpath);
        dvp_i2c_set_slv_addr(chrdev->i2c1_addr);
    }
}
static int  bf3a03_set_close_mode(struct bf3a03_chrdev *chrdev , int port , bool enable)
{
    int ret = -1;
    uint8_t value = 0;

    printf("close: p:%d en:%d\n" , port , enable);
    bf3a03_select_i2c(chrdev , port);
    if (true == enable)
    {
        value = 0x07;
    }
    else
    {
        value = 0x00;
    }

    ret = bf3a03_write_reg(0x21 , &value , 1);
    dvp_i2c_close();
    return ret;
}

static int  bf3a03_set_sleep_mode(struct bf3a03_chrdev *chrdev , int port , bool enable)
{
    int ret = -1;
    uint8_t value = 0xD5;

    printf("sleep: p:%d en:%d\n" , port , enable);
    bf3a03_select_i2c(chrdev , port);
    if (true == enable)
    {
        value = 0xD5;
    }
    else
    {
        value = 0x55;
    }

    ret = bf3a03_write_reg(0x9 , &value , 1);
    dvp_i2c_close();
    return ret;
}


int bf3a03_bus_switch(int port)
{

    if (g_bf3a03_chardev != NULL)
    {
        if(port==0)
        {
			bf3a03_set_close_mode(g_bf3a03_chardev , 1 , 1);
			bf3a03_set_close_mode(g_bf3a03_chardev , 0 , 0);
        }
		else if(port==1)
        {
			bf3a03_set_close_mode(g_bf3a03_chardev , 0 , 1);
			bf3a03_set_close_mode(g_bf3a03_chardev , 1 , 0);
        }			
        return 0;
    }
    else
    {
        return -1;
    }
}

static int bf3a03_open(struct file *file)
{
    struct bf3a03_chrdev *chrdev = NULL;

    chrdev = malloc(sizeof(struct bf3a03_chrdev));
    memset(chrdev , 0 , sizeof(struct bf3a03_chrdev));
    file->f_priv = chrdev;

    bf3a03_dts_init(chrdev);
    bf3a03_pwm_init(chrdev);
    bf3a03_hw_reset(chrdev);

    chrdev->b_support_input_detect = false;
    g_bf3a03_chardev = chrdev;
    return 0;
}

static int bf3a03_close(struct file *file)
{
    struct bf3a03_chrdev *chrdev = file->f_priv;

    //bf3a03_set_sleep_mode(chrdev , chrdev->port , true);
	bf3a03_set_close_mode(chrdev , 0 , true);  
	bf3a03_set_close_mode(chrdev , 1 , true);

    if (chrdev)
    {
        free(chrdev);
    }
    file->f_priv = NULL;
    g_bf3a03_chardev = NULL;
    printk("bf3a03_close end\n");
    close(chrdev->pwm_fd);

    return 0;
}

static int bf3a03_ioctl(struct file *file , int cmd , unsigned long arg)
{
    int ret = -1;

    struct bf3a03_chrdev *chrdev = file->f_priv;

    switch (cmd)
    {
        case DVPDEVICE_SET_INPUT_TVTYPE:
        {
			bf3a03_set_sleep_mode(chrdev , chrdev->port , true);
            //彩色
            if (chrdev->port == 0)
            {
                printf("SELECT COLOR SENSOR\n");
                bf3a03_select_i2c(chrdev , 0);
                bf3a03_init_config(bf3a03_init_cfg);
                dvp_i2c_close();
            }
            else
            {
                printf("SELECT BLANCK AND WHITE SENSOR\n");
                bf3a03_select_i2c(chrdev , 1);
                bf3a03_init_config(bf3a03_init_cfg_black);
                dvp_i2c_close();
            }
            chrdev->b_init = true;
            ret = 0;
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
            uint8_t port = (uint32_t)arg;
            chrdev->port = port;
            if (chrdev->b_init == true)
            {
                bf3a03_set_sleep_mode(chrdev , 0 , 1);
                bf3a03_set_sleep_mode(chrdev , 1 , 1);
                if (chrdev->port == 0)
                {
                    printf("SELECT COLOR SENSOR\n");
                    bf3a03_select_i2c(chrdev , 0);
                    bf3a03_init_config(bf3a03_init_cfg);
                    dvp_i2c_close();
                }
                else
                {
                    printf("SELECT BLANCK AND WHITE SENSOR\n");
                    bf3a03_select_i2c(chrdev , 1);
                    bf3a03_init_config(bf3a03_init_cfg_black);
                    dvp_i2c_close();
                }
            }
            printf("set input port = %d\n" , port);
            ret = 0;
            break;
        }
        default:
            ret = -1;
            break;
    }

    return ret;
}

int bf3a03_read(int port ,uint8_t cmd , uint8_t *p_data , uint32_t len)
{
    int ret = 0;

    if (g_bf3a03_chardev!=NULL)
    {
        bf3a03_select_i2c(g_bf3a03_chardev , port);
        ret = bf3a03_read_reg(cmd ,p_data , len);
        dvp_i2c_close();
        return ret;
    }
    else
    {
        return -1;
    }

}

int bf3a03_write(int port , uint8_t cmd , uint8_t *p_data , uint32_t len)
{
    int ret = 0;

    if (g_bf3a03_chardev != NULL)
    {
        bf3a03_select_i2c(g_bf3a03_chardev , port);
        ret = bf3a03_write_reg(cmd , p_data , len);
        dvp_i2c_close();
        return ret;
    }
    else
    {
        return -1;
    }
}


static const struct file_operations bf3a03_fops = {
    .open = bf3a03_open,
    .close = bf3a03_close,
    .read = dummy_read,
    .write = dummy_write,
    .ioctl = bf3a03_ioctl,
};

static int bf3a03_init(void)
{
    int np;
    const char *status;

    np = fdt_node_probe_by_path("/hcrtos/dvp-bf3a03");
    if (np < 0)
    {
        log_w("cannot found efsue in DTS\n");
        return 0;
    }

    if (!fdt_get_property_string_index(np , "status" , 0 , &status) &&
        !strcmp(status , "disabled"))
        return 0;

    register_driver("/dev/dvp-bf3a03" , &bf3a03_fops , 0666 , NULL);

    return 0;
}

module_driver(bf3a03 , bf3a03_init , NULL , 0)
