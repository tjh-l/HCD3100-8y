/* hid_gadget_test */
#define LOG_TAG "hid-demo"
#define ELOG_OUTPUT_LVL ELOG_LVL_DEBUG
#include <kernel/elog.h>

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <hcuapi/input.h>
#include <poll.h>
#include <kernel/io.h>
#include <kernel/drivers/hcusb.h>

#define REPORT_BUF_LEN 8
#define INPUT_EVENT_NUM 32

/* Used to determine if this key is continually pressed */
static bool btn_left_hold = false;
static bool btn_right_hold = false;
static bool btn_mid_hold = false;

int __mouse_fill_report(char *report, struct input_event *in, int nums)
{
    int index;
    bool to_send = false;

    memset(report, 0, REPORT_BUF_LEN);
    for (index = 0; index < nums; index++, in++) {
        if (in->type != EV_REL && in->type != EV_KEY &&
                in->type == EV_SYN)
            continue;

        if (in->type == EV_SYN && in->code == SYN_REPORT &&
                in->value == 0)
            break;

        to_send = true;

        if (in->type == EV_REL) {
            switch (in->code) {
            case REL_X:
                report[1] = (char)in->value;
                break;
            case REL_Y:
                report[2] = (char)in->value;
                break;
            case REL_WHEEL:
                report[3] = (char)in->value;
                break;
            default:
                break;
            }
        } else if (in->type == EV_KEY) {
            switch (in->code) {
            case BTN_LEFT:
                (in->value == 0) ? (report[0] &= (~0x1)) :
                             (report[0] |= 0x1);
                btn_left_hold = (in->value == 0) ? false : true;
                break;
            case BTN_RIGHT:
                (in->value == 0) ? (report[0] &= (~0x2)) :
                             (report[0] |= 0x2);
                btn_right_hold =
                    (in->value == 0) ? false : true;
                break;
            case BTN_MIDDLE:
                (in->value == 0) ? (report[0] &= (~0x4)) :
                             (report[0] |= 0x4);
                btn_mid_hold = (in->value == 0) ? false : true;
                break;
            default:
                break;
            }
        }
    }

    (!btn_left_hold) ?: (report[0] |= 0x1);
    (!btn_right_hold) ?: (report[0] |= 0x2);
    (!btn_mid_hold) ?: (report[0] |= 0x4);

    return to_send ? 4 : 0;
}


int hid_gadget_mouse_demo(int argc, const char *argv[])
{
    const char *filename = NULL;
    const char *hidg_name = NULL;
    int fd, fdg, ret = 0, in_count = 0;
    struct input_event in_array[INPUT_EVENT_NUM], *in;
    static struct pollfd pfd = { 0 };
    char report[REPORT_BUF_LEN];
    int to_send;

    btn_left_hold = false;
    btn_right_hold = false;
    btn_mid_hold = false;

    if (argc != 2) {
        log_e("Usage: Error. eg. %s /dev/input/mouse0\n", argv[0]);
        return -1;
    }

    filename = argv[1];
    hidg_name = "/dev/hidg-mouse";

    hcusb_set_mode(USB_PORT_0, MUSB_PERIPHERAL);
    hcusb_gadget_hidg_specified_init(get_udc_name(USB_PORT_0));

    if ((fd = open(filename, O_RDWR)) == -1) {
        log_e("Cannot open %s\n", filename);
        return -1;
    }
    if ((fdg = open(hidg_name, O_RDWR)) == -1) {
        log_e("Cannot open %s\n", hidg_name);
        return -1;
    }

    pfd.fd = fd;
    pfd.events = POLLIN | POLLRDNORM;
    in = &in_array[0];

    while (1) {
        /* wait for input event */
        if (poll(&pfd, 1, -1) < 0) {
            log_e("%s poll error.\n", filename);
            ret = -1;
            goto __mouse_exit;
        }

        /* read all input events */
        in_count = 0;
        do {
            if (in_count >= INPUT_EVENT_NUM)
                break;

            in = &in_array[in_count];
            ret = read(fd, in, sizeof(struct input_event));
            if (ret != sizeof(struct input_event)) {
                log_e("%s poll error.\n", filename);
                ret = -1;
                goto __mouse_exit;
            }

            // printf("\t [%d] %x %x %ld\n", in_count, in->type,
            //        in->code, in->value);

            in_count++;

            /* input sync event */
            if (in->type == EV_SYN && in->code == SYN_REPORT &&
                in->value == 0)
                break;

            /* poll for receiving next input event */
            ret = poll(&pfd, 1, 100);
            if (ret == 0) // timeout
                break;
            else if (ret < 0) {
                log_e("%s poll error.\n", filename);
                ret = -1;
                goto __mouse_exit;
            }
        } while (1);

        /* fill up usb hid report buffer */
        to_send = __mouse_fill_report(report, &in_array[0], in_count);
        if (to_send == 0)
            continue;
        else if (to_send == -1)
            break;

        // {
        //     int i;
        //     printf("\t -- :");
        //     for (i = 0; i < to_send; i++)
        //         printf(" %2.2x", (uint8_t)report[i]);
        //     printf("\n");
        // }

        /* send usb report buffer */
        if (write(fdg, report, to_send) != to_send) {
            log_e("Write %s error\n", hidg_name);
            ret = -1;
            goto __mouse_exit;
        }
    }

__mouse_exit:
    close(fd);
    close(fdg);
    hcusb_gadget_hidg_deinit();
    return ret;
}