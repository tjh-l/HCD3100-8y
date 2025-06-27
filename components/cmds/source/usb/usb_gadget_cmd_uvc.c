#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <kernel/lib/console.h>
#include <linux/bitops.h>
#include <linux/bits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <nuttx/fs/dirent.h>
#include <nuttx/fs/fs.h>
#include <nuttx/mtd/mtd.h>
#include <string.h>
#include <kernel/elog.h>
#include <kernel/drivers/hcusb.h>
#include <linux/mutex.h>
#include <sys/mman.h>
static void usbd_uvc_help_info(void)
{
    printf("usb device mode : uvc\n");
}

#define FB_BUF_SIZE (300 * 1024)


/* ---------------------  ---------------------  ---------------------  --------------------- */
/* ---------------------  ---------------------  ---------------------  --------------------- */
/* ---------------------  ---------------------  ---------------------  --------------------- */

static uint8_t *g_frame_buf = NULL;

static int __install_frame(uint8_t **frame , int *frame_sz)
{
    int fd;
    int file_size;
    int i;
    int ret;

    fd = open("/media/sda1/tt.jpg" , O_RDONLY);
    if (fd < 0)
    {
        log_e("Cannot open /media/sda1/tt.jpg\n");
        return -1;
    }

    lseek(fd , 0 , SEEK_SET);
    file_size = lseek(fd , 0 , SEEK_END);
    if (file_size > FB_BUF_SIZE)
    {
        log_e("Buffer isnot enough to save one frame data\n");
        close(fd);
        return -1;
    }

    lseek(fd , 0 , SEEK_SET);
    ret = read(fd , g_frame_buf , file_size);
    if (ret != file_size)
    {
        log_e("Read error. return value is %d, but file size is %d\n" ,
              ret , file_size);
        return -1;
    }

    close(fd);

    *frame = g_frame_buf;
    *frame_sz = file_size;

    return 0;
}


static int __switch_camera(bool is_turn_on)
{

    printf("usb camera gadget power %s\n", is_turn_on ? "on" : "off");

    if(!is_turn_on) {
        free(g_frame_buf);
        g_frame_buf = NULL;
    } else {
        if(g_frame_buf) {
            printf("malloc before .......\n");
            return 0;
        }
        g_frame_buf = malloc(FB_BUF_SIZE);
        if (!g_frame_buf)
        {
            log_e("Cannot malloc enough memory\n");
            return -1;
        }
    }

    return 0;
}

static int setup_usbd_uvc(int argc , char **argv)
{
    char ch;
    int i, usb_port = 0;
    const char *udc_name = NULL;
    int ret = 0;

    opterr = 0;
    optind = 0;

    while ((ch = getopt(argc , argv , "hHsSp:P:")) != EOF)
    {
        switch (ch)
        {
            case 'h':
            case 'H':
                usbd_uvc_help_info();
                return 0;
            case 'p':
            case 'P':
                usb_port = atoi(optarg);
                udc_name = get_udc_name(usb_port);
                if (udc_name == NULL)
                {
                    printf("[error] parameter(-p {usb_port}) error,"
                           "please check for help information(cmd: g_uvc -h)\n");
                    return -1;
                }
                printf("==> set usb#%u as uvc demo gadget\n" , usb_port);
                break;
            case 's':
            case 'S':
                hcusb_set_mode(usb_port , MUSB_HOST);
                hcusb_gadget_uvc_deinit();
                return 0;
            default:
                break;
        }
    }

    if (!udc_name)
        hcusb_gadget_uvc_init(__switch_camera , __install_frame);
    else
        hcusb_gadget_uvc_specified_init(udc_name , __switch_camera ,
                                        __install_frame);
    hcusb_set_mode(usb_port , MUSB_PERIPHERAL);

    return 0;
}

CONSOLE_CMD(g_uvc, "usb", setup_usbd_uvc, CONSOLE_CMD_MODE_SELF,
	    "setup USB as UVC device demo")