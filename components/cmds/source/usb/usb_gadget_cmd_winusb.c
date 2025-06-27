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

static void usbd_winusb_help_info(void)
{
    printf("usb device mode : winusb wcid\n");
    printf("\tusage:\n");
    printf("\tg_winusb -p 0 ## usb port 0\n");
    printf("\tg_winusb -p 1 ## usb port 1\n");
}


int setup_usbd_winusb(int argc, char **argv)
{
    char ch;
    int i;
    const char *udc_name = NULL;
    int usb_port = 0;

    opterr = 0;
    optind = 0;

    while ((ch = getopt(argc, argv, "hHsSp:P:")) != EOF) {
        switch(ch) {
            case 'h' :
            case 'H' :
                usbd_winusb_help_info();
                return 0;
            case 'p' :
            case 'P' :
                usb_port = atoi(optarg);
                udc_name = get_udc_name(usb_port);
                if(udc_name == NULL){
                    printf("[error] parameter(-p {usb_port}) error,"
                        "please check for help information(cmd: g_winusb -h)\n");
                    return -1;
                }
                printf("==> set usb#%u as winusb(wcid) demo gadget\n", usb_port);
                break;
            case 's' :
            case 'S' :
                hcusb_gadget_winusb_deinit();
                return 0;
            default:
                break;
        }
    }

    hcusb_set_mode(usb_port, MUSB_PERIPHERAL);
    if(!udc_name)
        hcusb_gadget_winusb_init();
    else 
        hcusb_gadget_winusb_specified_init(udc_name);
    return 0;
}



int setup_usbd_winusb_demo(int argc, char **argv)
{
#define WINUSB_BUF_LEN	(1024 * 4)
	int fd, rd, wr, index;
	struct pollfd pfd;
	unsigned char buf[WINUSB_BUF_LEN];

	fd = open("/dev/winusb", O_RDWR);
	if(fd < 0) {
		printf("Cannot open /dev/winusb\n");
		return -1;
	}
	pfd.fd = fd;
	pfd.events = POLLIN | POLLRDNORM;

	while(1) {
		printf("\n\n======== wait for receiving data from PC host ============\n");

		memset(buf, 0, WINUSB_BUF_LEN);
		if (poll(&pfd, 1, -1) <= 0) {
            printf(" ==> poll abort ...\n");
        	break;
        }

		rd = read(fd, buf, WINUSB_BUF_LEN);
        if(rd < 0) {
            printf(" ==> read error ..\n");
            break;
        }

		printf("read len: %d\n", rd);
		for(index = 0; index < rd; index++) {
            if(index % 16 == 0)
                printf("%.4xh: ", index);
			
            printf("%2.2x ", buf[index]);

			if(index % 16 == 15)
				printf("\n");
		}
		printf("\n");

		wr = write(fd, buf, rd);
        if(wr < 0) {
            printf(" ==> read error ..\n");
            break;
        }
		printf("write len: %d\n", wr);
	}

    close(fd);
    printf("exit...\n");
    return 0;
}


CONSOLE_CMD(g_winusb, "usb", setup_usbd_winusb, CONSOLE_CMD_MODE_SELF,
	    "setup USB as WINUSB device demo. eg. g_winusb -p 0")

CONSOLE_CMD(g_winusb_demo, "usb", setup_usbd_winusb_demo, CONSOLE_CMD_MODE_SELF,
	    "test WINUSB device demo, loopback mode")
