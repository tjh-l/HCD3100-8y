#ifndef __HCRTOS__
#include "app_config.h"

#ifdef UIBC_SUPPORT
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/input.h>

#include "usb_hid.h"

#define INPUT_DEV_MAX   10
static void *_key_task(void *arg)
{
    uint8_t input_name[32];
    int i = 0, fd = -1;

    for (i = 0; i < INPUT_DEV_MAX; i++){
        sprintf(input_name, "/dev/input/event%d", i);
        fd= open(input_name, O_RDONLY);
        if (fd < 0)
            break;

        hid_type_e type = usb_hid_get_device_type(fd);
        close(fd);
        if (type == HID_KEYBOARD || type == HID_MOUSE) {
            usb_hid_register_fd(type, input_name);
        }
    }

    return NULL;
}

void api_key_get_init()
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    if(pthread_create(&thread_id, &attr, _key_task, NULL)) {
        pthread_attr_destroy(&attr);
        return;
    }

    pthread_attr_destroy(&attr);
}
#endif
#endif
