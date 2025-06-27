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
#include <linux/printk.h>
#include <linux/delay.h>
#include <stdbool.h>
#include "../dvp_i2c.h"

#include "ms7200.h"

extern VOID ms7200_init(VOID);

struct ms7200_chrdev
{
    const char *i2c_devpath;
    uint32_t    i2c_addr;
    uint32_t    b_support_input_detect;
    uint32_t    reset_gpio;
    uint32_t    reset_active;
};

static void ms7200_dts_init(struct ms7200_chrdev *chrdev)
{
    int np;

    np = fdt_get_node_offset_by_path("/hcrtos/dvp-ms7200");

    chrdev->reset_gpio = 0xffffffff;
    chrdev->i2c_addr = 0xffffffff;
    chrdev->reset_active = 0xffffffff;

    if(np >= 0)
    {
        if(chrdev != NULL)
        {
            fdt_get_property_string_index(np, "i2c-devpath", 0, &chrdev->i2c_devpath);
            fdt_get_property_u_32_index(np, "i2c-addr", 0, (u32 *)(&chrdev->i2c_addr));
            fdt_get_property_u_32_index(np, "reset-gpio", 0, (u32 *)(&chrdev->reset_gpio));
            fdt_get_property_u_32_index(np, "reset-gpio", 1, (u32 *)(&chrdev->reset_active));
            if(chrdev->reset_active == GPIO_ACTIVE_LOW)
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

static void ms7200_hw_reset(struct ms7200_chrdev *chrdev)
{
    if(chrdev->reset_gpio != 0xffffffff)
    {
        printf("chrdev->reset_gpio=%ld chrdev->reset_active=%ld\n",
               chrdev->reset_gpio, chrdev->reset_active);
        gpio_configure(chrdev->reset_gpio, GPIO_DIR_OUTPUT);
        gpio_set_output(chrdev->reset_gpio, chrdev->reset_active);
        msleep(5);
        gpio_set_output(chrdev->reset_gpio, 1 - chrdev->reset_active);
        msleep(100);
    }
}


static bool ms7200_get_input_signal_format(tvtype_e *tv_sys, bool *bProgressive)
{
    bool input_on = false;
 
    input_on = ms7200_hdmirx_source_connect_detect();
    //printk("%s %d input_on=%s\n", __FUNCTION__, __LINE__, input_on ? "true" : "false");
    if(input_on == true)
    {
        if(tv_sys == NULL || bProgressive == NULL)
        {
            return false;
        }
        msleep(300);
        HDMI_CONFIG_T t_hdmi_rx;
        ms7200_hdmirx_input_infoframe_get(&t_hdmi_rx);
        //printk("%s %d t_hdmi_rx.u8_video_format=0x%x\n", __FUNCTION__, __LINE__, t_hdmi_rx.u8_video_format);
        printk("%s %d t_hdmi_rx.u8_vic=0x%x\n", __FUNCTION__, __LINE__, t_hdmi_rx.u8_vic);
        switch (t_hdmi_rx.u8_vic)
        {
            case 4:
            {
                *tv_sys = TV_LINE_720_30;
                *bProgressive = true;
                break;
            }
            case 19:
            {
                *tv_sys = TV_LINE_720_25;
                *bProgressive = true;
                break;
            }
            case 2:
            case 3:
            {
                *tv_sys = TV_NTSC;
                *bProgressive = true;
                break;
            }
            case 17:
            case 18:
            {
                *tv_sys = TV_PAL;
                *bProgressive = true;
                break;
            }
            case 20:
            {
                *tv_sys = TV_LINE_1080_25;
                *bProgressive = false;
                break;
            }
            case 5:
            {
                *tv_sys = TV_LINE_1080_30;
                *bProgressive = false;
                break;
            }
            case 16:
            {
                *tv_sys = TV_LINE_1080_60;
                *bProgressive = true;
                break;
            }
            case 31:
            {
                *tv_sys = TV_LINE_1080_50;
                *bProgressive = true;
                break;
            }
                
        }
        
        return true;
    }
    else
    {
        return false;
    }
}


static int ms7200_open(struct file *file)
{
    struct ms7200_chrdev *chrdev = NULL;

    chrdev = malloc(sizeof(struct ms7200_chrdev));
    memset(chrdev, 0, sizeof(struct ms7200_chrdev));
    file->f_priv = chrdev;

    ms7200_dts_init(chrdev);
    ms7200_hw_reset(chrdev);
    dvp_i2c_open(chrdev->i2c_devpath);
    dvp_i2c_set_slv_addr(chrdev->i2c_addr);
    chrdev->b_support_input_detect = true;

    ms7200_init();

    return 0;
}

static int ms7200_close(struct file *file)
{
    struct ms7200_chrdev *chrdev = file->f_priv;

    printk("ms7200_close\n");
    dvp_i2c_close();
    if (chrdev->reset_gpio != 0xffffffff)
    {
        gpio_configure(chrdev->reset_gpio , GPIO_DIR_OUTPUT);
        gpio_set_output(chrdev->reset_gpio , chrdev->reset_active);
    }
    if(chrdev)
    {
        free(chrdev);
    }
    file->f_priv = NULL;
    printk("ms7200_close end\n");

    return 0;
}

static int ms7200_ioctl(struct file *file, int cmd, unsigned long arg)
{
    int ret = -1;

    struct ms7200_chrdev *chrdev = file->f_priv;

    //printk("%s %d cmd=%d\n", __FUNCTION__, __LINE__, cmd);
    switch(cmd)
    {
        case DVPDEVICE_SET_INPUT_TVTYPE:
        {
            struct dvp_device_tvtype *device_tvtype = (struct dvp_device_tvtype *)arg;
            printf("%s %d DVPDEVICE_SET_INPUT_TVTYPE device_tvtype->tv_sys=%d\n", __FUNCTION__, __LINE__, device_tvtype->tv_sys);
            printf("%s %d DVPDEVICE_SET_INPUT_TVTYPE device_tvtype->b_progressive=%d\n", __FUNCTION__, __LINE__, device_tvtype->b_progressive);
            ret = 0;
            break;
        }
        case DVPDEVICE_GET_INPUT_DETECT_SUPPORT:
        {
            uint32_t *b_support_input_detect;
            b_support_input_detect = (uint32_t *)arg;
            *b_support_input_detect = chrdev->b_support_input_detect;
            printf("%s %d DVPDEVICE_GET_INPUT_DETECT_SUPPORT chrdev->b_support_input_detect=%lu\n", __FUNCTION__, __LINE__, chrdev->b_support_input_detect);
            ret = 0;
            break;
        }
        case DVPDEVICE_GET_INPUT_TVTYPE:
        {
            extern bool ms7200_media_service(VOID);
            ret = -1;
            if (ms7200_media_service() == true)
            {
                struct dvp_device_tvtype *device_tvtype = (struct dvp_device_tvtype *)arg;
                if (ms7200_get_input_signal_format(&device_tvtype->tv_sys , &device_tvtype->b_progressive) == true)
                {
                    printf("%s %d DVPDEVICE_GET_INPUT_TVTYPE device_tvtype->tv_sys=%d\n" , __FUNCTION__ , __LINE__ , device_tvtype->tv_sys);
                    ret = 0;
                }
            }
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

static const struct file_operations ms7200_fops =
{
    .open = ms7200_open,
    .close = ms7200_close,
    .read = dummy_read,
    .write = dummy_write,
    .ioctl = ms7200_ioctl,
};

static int ms7200_drv_init(void)
{
    int np;
    const char *status;
    uint32_t    reset_gpio = 0xFFFFFFFF;
    uint32_t    reset_active = 0xFFFFFFFF;

    np = fdt_node_probe_by_path("/hcrtos/dvp-ms7200");
    if(np < 0)
    {
        log_w("cannot found efsue in DTS\n");
        return 0;
    }

    if(!fdt_get_property_string_index(np, "status", 0, &status) &&
            !strcmp(status, "disabled"))
    { return 0; }

    
    fdt_get_property_u_32_index(np , "reset-gpio" , 0 , (u32 *)(&reset_gpio));
    fdt_get_property_u_32_index(np , "reset-gpio" , 1 , (u32 *)(&reset_active));
    if (reset_gpio != 0xFFFFFFFF)
    {
        if (reset_active == GPIO_ACTIVE_LOW)
        {
            reset_active = 0;
        }
        else
        {
            reset_active = 1;
        }
        gpio_configure(reset_gpio , GPIO_DIR_OUTPUT);
        gpio_set_output(reset_gpio , reset_active);
    }

    register_driver("/dev/dvp-ms7200" , &ms7200_fops , 0666 , NULL);
    return 0;
}

module_driver(ms7200, ms7200_drv_init , NULL, 0)
