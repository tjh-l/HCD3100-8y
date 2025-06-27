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
#define INPUT_EVENT_NUM 64

struct kbd_report_mapping {
    struct kbd_report_mapping *next;
    const uint8_t report_val;
};

struct kbd_sepc_report_mapping {
    const uint8_t input_code;
    const uint8_t report_val;
    bool is_hold;
};

struct kbd_report_mapping g_normal_head;

/* special key */
struct kbd_sepc_report_mapping kbd_sepc_map[] = {
    { .input_code = 29, .report_val = 0x01 }, /* KEY_LEFTCTRL */
    { .input_code = 42, .report_val = 0x02 }, /* KEY_LEFTSHIFT */
    { .input_code = 56, .report_val = 0x04 }, /* KEY_LEFTALT */
    { .input_code = 125, .report_val = 0x08 }, /* KEY_LEFTMETA */
    { .input_code = 97, .report_val = 0x10 }, /* KEY_RIGHTCTRL */
    { .input_code = 54, .report_val = 0x20 }, /* KEY_RIGHTSHIFT */
    { .input_code = 100, .report_val = 0x40 }, /* KEY_RIGHTALT */
    { .input_code = 126, .report_val = 0x80 }, /* KEY_RIGHTMETA */
};

#define is_special_key(code)                                                  \
    ((code == KEY_LEFTCTRL) || (code == KEY_LEFTSHIFT) ||                \
     (code == KEY_LEFTALT) || (code == KEY_LEFTMETA) ||                  \
     (code == KEY_RIGHTCTRL) || (code == KEY_RIGHTSHIFT) ||              \
     (code == KEY_RIGHTALT) || (code == KEY_RIGHTMETA))

/* normal key */
struct kbd_report_mapping kbd_normal_map[] = {
    { .report_val = 0x00 }, { .report_val = 0x29 }, { .report_val = 0x1e },
    { .report_val = 0x1f }, { .report_val = 0x20 }, { .report_val = 0x21 },
    { .report_val = 0x22 }, { .report_val = 0x23 }, { .report_val = 0x24 },
    { .report_val = 0x25 }, { .report_val = 0x26 }, { .report_val = 0x27 },
    { .report_val = 0x2d }, { .report_val = 0x2e }, { .report_val = 0x2a },
    { .report_val = 0x2b }, { .report_val = 0x14 }, { .report_val = 0x1a },
    { .report_val = 0x08 }, { .report_val = 0x15 }, { .report_val = 0x17 },
    { .report_val = 0x1c }, { .report_val = 0x18 }, { .report_val = 0x0c },
    { .report_val = 0x12 }, { .report_val = 0x13 }, { .report_val = 0x2f },
    { .report_val = 0x30 }, { .report_val = 0x28 }, { .report_val = 0x00 },
    { .report_val = 0x04 }, { .report_val = 0x16 }, { .report_val = 0x07 },
    { .report_val = 0x09 }, { .report_val = 0x0a }, { .report_val = 0x0b },
    { .report_val = 0x0d }, { .report_val = 0x0e }, { .report_val = 0x0f },
    { .report_val = 0x33 }, { .report_val = 0x34 }, { .report_val = 0x35 },
    { .report_val = 0x00 }, { .report_val = 0x32 }, { .report_val = 0x1d },
    { .report_val = 0x1b }, { .report_val = 0x06 }, { .report_val = 0x19 },
    { .report_val = 0x05 }, { .report_val = 0x11 }, { .report_val = 0x10 },
    { .report_val = 0x36 }, { .report_val = 0x37 }, { .report_val = 0x38 },
    { .report_val = 0x00 }, { .report_val = 0x55 }, { .report_val = 0x00 },
    { .report_val = 0x2c }, { .report_val = 0x39 }, { .report_val = 0x3a },
    { .report_val = 0x3b }, { .report_val = 0x3c }, { .report_val = 0x3d },
    { .report_val = 0x3e }, { .report_val = 0x3f }, { .report_val = 0x40 },
    { .report_val = 0x41 }, { .report_val = 0x42 }, { .report_val = 0x43 },
    { .report_val = 0x53 }, { .report_val = 0x47 }, { .report_val = 0x5f },
    { .report_val = 0x60 }, { .report_val = 0x61 }, { .report_val = 0x56 },
    { .report_val = 0x5c }, { .report_val = 0x5d }, { .report_val = 0x5e },
    { .report_val = 0x57 }, { .report_val = 0x59 }, { .report_val = 0x5a },
    { .report_val = 0x5b }, { .report_val = 0x62 }, { .report_val = 0x63 },
    { .report_val = 0x00 }, { .report_val = 0x94 }, { .report_val = 0x64 },
    { .report_val = 0x44 }, { .report_val = 0x45 }, { .report_val = 0x87 },
    { .report_val = 0x92 }, { .report_val = 0x93 }, { .report_val = 0x8a },
    { .report_val = 0x88 }, { .report_val = 0x8b }, { .report_val = 0x8c },
    { .report_val = 0x58 }, { .report_val = 0x00 }, { .report_val = 0x54 },
    { .report_val = 0x46 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x4a }, { .report_val = 0x52 }, { .report_val = 0x4b },
    { .report_val = 0x50 }, { .report_val = 0x4f }, { .report_val = 0x4d },
    { .report_val = 0x51 }, { .report_val = 0x4e }, { .report_val = 0x49 },
    { .report_val = 0x4c }, { .report_val = 0x00 }, { .report_val = 0x7f },
    { .report_val = 0x81 }, { .report_val = 0x80 }, { .report_val = 0x66 },
    { .report_val = 0x67 }, { .report_val = 0x00 }, { .report_val = 0x48 },
    { .report_val = 0x00 }, { .report_val = 0x85 }, { .report_val = 0x90 },
    { .report_val = 0x91 }, { .report_val = 0x89 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x65 }, { .report_val = 0x78 },
    { .report_val = 0x79 }, { .report_val = 0x76 }, { .report_val = 0x7a },
    { .report_val = 0x77 }, { .report_val = 0x7c }, { .report_val = 0x74 },
    { .report_val = 0x7d }, { .report_val = 0x7e }, { .report_val = 0x7b },
    { .report_val = 0x75 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x00 }, { .report_val = 0x00 }, { .report_val = 0x00 },
    { .report_val = 0x68 }, { .report_val = 0x69 }, { .report_val = 0x6a },
    { .report_val = 0x6b }, { .report_val = 0x6c }, { .report_val = 0x6d },
    { .report_val = 0x6e }, { .report_val = 0x6f }, { .report_val = 0x70 },
    { .report_val = 0x71 }, { .report_val = 0x72 }, { .report_val = 0x73 }
};

int __keyboard_fill_report(char *report, struct input_event *in, int nums)
{
    int index, j;
    bool to_send = false;
    struct kbd_report_mapping *cur;

    memset(report, 0, REPORT_BUF_LEN);

    for (index = 0; index < nums; index++, in++) {

        if (in->type == EV_SYN && in->code == SYN_REPORT &&
            in->value == 0)
            break;

        if ((in->type != EV_KEY))
            continue;

        // printf("\t  @@ %2.2x %2.2x %ld\n", in->type, in->code, in->value);

        /* note: only send usb report when key is pressed or released */
        if (in->value == 1 || in->value == 0)
            to_send = true;
        else
            continue;

        /* check special key */
        if (is_special_key(in->code)) {
            for (index = 0; index < ARRAY_SIZE(kbd_sepc_map);
                 index++) {
                if (in->code == kbd_sepc_map[index].input_code)
                    kbd_sepc_map[index].is_hold = (in->value) ?
                                             true : false;
            }
            continue;
        }

        if (in->code >= ARRAY_SIZE(kbd_normal_map)) {
            log_e("Error input event, input code is 0x%x\n",
                  in->code);
            continue;
        }

        /* check normal keyboard key */
        j = 0;
        cur = &g_normal_head;
        if (in->value == 1) { // press
            while (cur->next && (++j < 6))
                cur = cur->next;
            if (j >= 6) // no more than 6 keys can be sent
                break;
            cur->next = &kbd_normal_map[in->code];
        } else if (in->value == 0) { // release
            while (cur->next && (++j <= 6)) {
                if (cur->next == &kbd_normal_map[in->code]) {
                    cur->next = kbd_normal_map[in->code].next;
                    kbd_normal_map[in->code].next = NULL;
                    break;
                }
                cur = cur->next;
            }
        }
    }

    if (to_send) {
        /* for special keyboard key */
        for (index = 0; index < ARRAY_SIZE(kbd_sepc_map); index++){
            if (kbd_sepc_map[index].is_hold)
                report[0] |= kbd_sepc_map[index].report_val;
            else
                report[0] &= ~kbd_sepc_map[index].report_val;
        }

        /* for normall keyboard key */
        cur = g_normal_head.next;
        for (index = 2; index < 8; index++) {
            if (!cur)
                break;

            report[index] = cur->report_val;
            // printf("\t  ==> report[%d] = 0x%2.2x\n", index, report[index]);
            cur = cur->next;
        }
    }

    return (to_send) ? 8 : 0;
}

int hid_gadget_kbd_demo(int argc, const char *argv[])
{
    const char *filename = NULL;
    const char *hidg_name = NULL;
    int fd, fdg, ret = 0, in_count = 0;
    struct input_event in_array[INPUT_EVENT_NUM], *in;
    static struct pollfd pfd = { 0 };
    char report[REPORT_BUF_LEN];
    int to_send, index;

    if (argc != 2) {
        log_e("Usage: Error. eg. %s /dev/input/kbd0\n", argv[0]);
        return -1;
    }

    filename = argv[1];
    hidg_name = "/dev/hidg-kbd";

    if ((fd = open(filename, O_RDWR)) == -1) {
        log_e("Cannot open %s\n", filename);
        return -1;
    }
    if ((fdg = open(hidg_name, O_RDWR)) == -1) {
        log_e("Cannot open %s\n", hidg_name);
        return -1;
    }

    hcusb_set_mode(USB_PORT_0, MUSB_PERIPHERAL);
    hcusb_gadget_hidg_specified_init(get_udc_name(USB_PORT_0));

    pfd.fd = fd;
    pfd.events = POLLIN | POLLRDNORM;
    in = &in_array[0];

    for (index = 0; index < ARRAY_SIZE(kbd_normal_map); index++)
        kbd_normal_map[index].next = NULL;
    g_normal_head.next = NULL;

    while (1) {
        /* wait for input event */
        if (poll(&pfd, 1, -1) < 0) {
            log_e("%s poll error.\n", filename);
            ret = -1;
            goto __kbd_exit;
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
                goto __kbd_exit;
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
                goto __kbd_exit;
            }
        } while (1);

        /* fill up usb hid report buffer */
        to_send =
            __keyboard_fill_report(report, &in_array[0], in_count);
        if (to_send == 0)
            continue;
        else if (to_send == -1)
            break;

        // {
        // 	int i;
        // 	printf("\t -- :");
        // 	for (i = 0; i < to_send; i++)
        // 		printf(" %2.2x", (uint8_t)report[i]);
        // 	printf("\n");
        // }

        /* send usb report buffer */
        if (write(fdg, report, to_send) != to_send) {
            log_e("Write %s error\n", hidg_name);
            ret = -1;
            goto __kbd_exit;
        }
    }

__kbd_exit:
    close(fd);
    close(fdg);
    hcusb_gadget_hidg_deinit();
    return ret;
}
