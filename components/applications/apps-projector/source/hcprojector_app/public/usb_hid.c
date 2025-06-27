#include "app_config.h"

#ifdef UIBC_SUPPORT

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef __HCRTOS__
#include <hcuapi/input.h>
#else
#include <linux/input.h>
#endif

#include "usb_hid.h"

#define REPORT_BUF_LEN      (8)
#define INPUT_EVENT_NUM     (8)
#define INPUT_DEVICE_NUM    (5)

/* Used to determine if this key is continually pressed */
struct mouse_btn_st
{
    bool left;
    bool right;
    bool middle;
    // bool Side1;
    // bool Side2;
};

static hid_event_cb g_hid_event_cb = NULL;
static pthread_mutex_t g_hid_mutex = PTHREAD_MUTEX_INITIALIZER;

struct usb_hid_dev_prop
{
    int fd;
    hid_type_e type;
    bool valid;
    char res[3];
    char path[32];
    pthread_t tid;
};

struct usb_hid_dev
{
    int total;
    struct usb_hid_dev_prop dev[INPUT_DEVICE_NUM];
};

struct usb_hid_dev g_hid_dev = {0};

struct kbd_report_mapping
{
    struct kbd_report_mapping *next;
    const uint8_t report_val;
};

struct kbd_sepc_report_mapping
{
    const uint8_t input_code;
    const uint8_t report_val;
    bool is_hold;
};

struct kbd_report_mapping g_normal_head;

static struct kbd_sepc_report_mapping kbd_sepc_map[] =
{
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

static struct kbd_report_mapping kbd_normal_map[] =
{
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

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof (a) / sizeof *(a))
#endif

#ifndef __HCRTOS__
#ifndef BITS_PER_LONG
#define BITS_PER_LONG        (sizeof(long) * 8)
#endif
#define NBITS(x)             ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)               ((x)%BITS_PER_LONG)
#ifndef BIT
#define BIT(x)               (1UL<<OFF(x))
#endif
#define LONG(x)              ((x)/BITS_PER_LONG)
#undef test_bit
#define test_bit(bit, array) ((array[LONG(bit)] >> OFF(bit)) & 1)

/*
 * Type of input device for basic classification.
 * Values may be or'ed together.
 */
typedef enum
{
    DIDTF_NONE          = 0x00000000,  /* Unclassified, no specific type. */

    DIDTF_KEYBOARD      = 0x00000001,  /* Can act as a keyboard. */
    DIDTF_MOUSE         = 0x00000002,  /* Can be used as a mouse. */
    DIDTF_JOYSTICK      = 0x00000004,  /* Can be used as a joystick. */
    DIDTF_REMOTE        = 0x00000008,  /* Is a remote control. */
    DIDTF_VIRTUAL       = 0x00000010,  /* Is a virtual input device. */
    DIDTF_REMOTE_CONTROL = 0x00000020,  /* Is a remote control. */
    DIDTF_ALL           = 0x0000001F   /* All type flags set. */
} DFBInputDeviceTypeFlags; // #include <directfb.h>

/*
 * Fill device information.
 * Queries the input device and tries to classify it.
 */
static hid_type_e get_device_type(int fd)
{
    int           i;
    unsigned int  num_keys     = 0;
    unsigned int  num_ext_keys = 0;
    unsigned int  num_buttons  = 0;
    unsigned int  num_rels     = 0;
    unsigned int  num_abs      = 0;
    unsigned long evbit[NBITS(EV_CNT)];
    unsigned long keybit[NBITS(KEY_CNT)];
    unsigned long relbit[NBITS(REL_CNT)];
    unsigned long absbit[NBITS(ABS_CNT)];
    int type = DIDTF_NONE;

    /* get event type bits */
    ioctl( fd, EVIOCGBIT(0, sizeof(evbit)), evbit ); //! hcrtos doesn't have this Ioctl EVIOCGBIT

    if (test_bit( EV_KEY, evbit ))
    {
        /* get keyboard bits */
        ioctl( fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit );

        /* count typical keyboard keys only */
        for (i = KEY_Q; i <= KEY_M; i++)
        {
            if (test_bit( i, keybit ))
                num_keys++;
        }

        /* this might be a keyboard with just cursor keys, typically found
            on front panels - handle as remote control and make sure not to
            treat normal (full key) keyboards likewise */
        if (!num_keys)
        {
            for (i = KEY_HOME; i <= KEY_PAGEDOWN; i++)
            {
                if (test_bit( i, keybit ))
                    num_ext_keys++;
            }
        }

        for (i = KEY_OK; i < KEY_CNT; i++)
        {
            if (test_bit( i, keybit ))
                num_ext_keys++;
        }

        for (i = BTN_MOUSE; i < BTN_JOYSTICK; i++)
        {
            if (test_bit( i, keybit ))
                num_buttons++;
        }
    }

    if (test_bit( EV_REL, evbit ))
    {
        /* get bits for relative axes */
        ioctl( fd, EVIOCGBIT(EV_REL, sizeof(relbit)), relbit );

        for (i = 0; i < REL_CNT; i++)
        {
            if (test_bit( i, relbit ))
                num_rels++;
        }
    }

    if (test_bit( EV_ABS, evbit ))
    {
        /* get bits for absolute axes */
        ioctl( fd, EVIOCGBIT(EV_ABS, sizeof(absbit)), absbit );

        for (i = 0; i < ABS_PRESSURE; i++)
        {
            if (test_bit( i, absbit ))
                num_abs++;
        }
    }

    /* Mouse, Touchscreen or Joystick */
    if ((test_bit( EV_KEY, evbit ) && (test_bit( BTN_TOUCH, keybit ) || test_bit( BTN_TOOL_FINGER, keybit ))) ||
        ((num_rels >= 2 && num_buttons) || (num_abs == 2 && num_buttons == 1)))
    {
        type |= DIDTF_MOUSE;
    }
    else if (num_abs && num_buttons)
    {
        type |= DIDTF_JOYSTICK;
    }

    /* Keyboard */
    if (num_keys > 20)
    {
        type |= DIDTF_KEYBOARD;
    }

    /* Remote Control */
    if (num_ext_keys)
    {
        type |= DIDTF_REMOTE_CONTROL;
    }

    /* Primary input device */
    if (type & DIDTF_KEYBOARD)
    {
        printf("  -> keyboard\n");
        return HID_KEYBOARD;
    }
    /* modify Composite devices mainly support mouse devices. maybe type include DIDTF_MOUSE | HID_REMOTE_CONTROL*/
    else if (type & DIDTF_MOUSE)
    {
        printf("  -> mouse\n");
        return HID_MOUSE;
    }
    else if (type & DIDTF_REMOTE_CONTROL)
    {
        printf("  -> remote control\n");
        return HID_REMOTE_CONTROL;
    }
    else if (type & DIDTF_JOYSTICK)
    {
        printf("  -> joystick\n");
        return HID_JOYSTICK;
    }
    else
    {
        printf("  --> any\n");
        return HID_NONE;
    }
}
#endif

hid_type_e usb_hid_get_device_type(int fd)
{
#ifdef __HCRTOS__
    return HID_NONE;
#else
    return get_device_type(fd);
#endif
}

static int __keyboard_fill_report(char *report, struct input_event *in, int nums)
{
    int index, j;
    bool to_send = false;
    struct kbd_report_mapping *cur;

    memset(report, 0, REPORT_BUF_LEN);

    for (index = 0; index < nums; index++, in++)
    {

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
        if (is_special_key(in->code))
        {
            for (index = 0; index < ARRAY_SIZE(kbd_sepc_map);
                 index++)
            {
                if (in->code == kbd_sepc_map[index].input_code)
                    kbd_sepc_map[index].is_hold = (in->value) ?
                                                  true : false;
            }
            continue;
        }

        if (in->code >= ARRAY_SIZE(kbd_normal_map))
        {
            printf("Error input event, input code is 0x%x\n", in->code);
            continue;
        }

        /* check normal keyboard key */
        j = 0;
        cur = &g_normal_head;
        if (in->value == 1)   // press
        {
            while (cur->next && (++j < 6))
                cur = cur->next;
            if (j >= 6) // no more than 6 keys can be sent
                break;
            cur->next = &kbd_normal_map[in->code];
        }
        else if (in->value == 0)     // release
        {
            while (cur->next && (++j <= 6))
            {
                if (cur->next == &kbd_normal_map[in->code])
                {
                    cur->next = kbd_normal_map[in->code].next;
                    kbd_normal_map[in->code].next = NULL;
                    break;
                }
                cur = cur->next;
            }
        }
    }

    if (to_send)
    {
        /* for special keyboard key */
        for (index = 0; index < ARRAY_SIZE(kbd_sepc_map); index++)
        {
            if (kbd_sepc_map[index].is_hold)
                report[0] |= kbd_sepc_map[index].report_val;
            else
                report[0] &= ~kbd_sepc_map[index].report_val;
        }

        /* for normal keyboard key */
        cur = g_normal_head.next;
        for (index = 2; index < 8; index++)
        {
            if (!cur)
                break;

            report[index] = cur->report_val;
            // printf("\t  ==> report[%d] = 0x%2.2x\n", index, report[index]);
            cur = cur->next;
        }
    }

    return (to_send) ? 8 : 0;
}

static int __mouse_fill_report(char *report, struct input_event *in, int nums, struct mouse_btn_st *btn)
{
    int index;
    bool to_send = false;

    memset(report, 0, REPORT_BUF_LEN);
    for (index = 0; index < nums; index++, in++)
    {
        if (in->type != EV_REL && in->type != EV_KEY &&
            in->type == EV_SYN)
            continue;

        if (in->type == EV_SYN && in->code == SYN_REPORT &&
            in->value == 0)
            break;

        to_send = true;

        if (in->type == EV_REL)
        {
            switch (in->code)
            {
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
        }
        else if (in->type == EV_KEY)
        {
            switch (in->code)
            {
                case BTN_LEFT:
                    (in->value == 0) ? (report[0] &= (~0x1)) :
                    (report[0] |= 0x1);
                    btn->left = (in->value == 0) ? false : true;
                    break;
                case BTN_RIGHT:
                    (in->value == 0) ? (report[0] &= (~0x2)) :
                    (report[0] |= 0x2);
                    btn->right = (in->value == 0) ? false : true;
                    break;
                case BTN_MIDDLE:
                    (in->value == 0) ? (report[0] &= (~0x4)) :
                    (report[0] |= 0x4);
                    btn->middle = (in->value == 0) ? false : true;
                    break;
                default:
                    break;
            }
        }
    }

    (!btn->left) ? : (report[0] |= 0x1);
    (!btn->right) ? : (report[0] |= 0x2);
    (!btn->middle) ? : (report[0] |= 0x4);

    return to_send ? 4 : 0;
}

static void *usb_hid_event(void *arg)
{
    int ret = 0, in_count = 0;

    struct input_event in_array[INPUT_EVENT_NUM], *in;
    struct pollfd fds = {0};

    char report[REPORT_BUF_LEN];
    int to_send = 0;

    int index = (int)arg;
    int hid_type = g_hid_dev.dev[index].type;
    int hid_fd = g_hid_dev.dev[index].fd;

    struct mouse_btn_st btn = {0};

    fds.fd = hid_fd;
    fds.events = POLLIN | POLLRDNORM;

    in = &in_array[0];

    while (g_hid_dev.dev[index].valid)
    {
    BEGIN:
        /* wait for input event */
        ret = poll(&fds, 1, 200);
        if (ret < 0)
        {
            printf("%s %d: poll error.\n", __func__, __LINE__);
            goto EXIT;
        }
        else if (ret == 0)
        {
            continue;
        }

        /* read all input events */
        in_count = 0;

        do
        {
            if (in_count >= INPUT_EVENT_NUM)
                break;

            in = &in_array[in_count];

            ret = read(hid_fd, in, sizeof(struct input_event));
            if (ret != sizeof(struct input_event))
            {
                printf("read error.\n");
                goto EXIT;
            }

            //printf("\t [%d] %x %x %ld\n", in_count, in->type, in->code, in->value);

            in_count++;

            /* input sync event */
            if (in->type == EV_SYN && in->code == SYN_REPORT && in->value == 0)
                break;

            /* poll for receiving next input event */
            ret = poll(&fds, 1, 100);
            if (ret == 0) // timeout
                break;
            else if (ret < 0)
            {
                printf("%s %d: poll error.\n", __func__, __LINE__);
                goto EXIT;
            }
        }
        while (1);

        switch (hid_type)
        {
            case HID_KEYBOARD:
            {
                /* fill up usb hid report buffer */
                to_send = __keyboard_fill_report(report, &in_array[0], in_count);
                if (to_send == 0)
                    continue;
                else if (to_send == -1)
                    break;

                break;
            }

            case HID_MOUSE:
            {
                /* fill up usb hid report buffer */
                to_send = __mouse_fill_report(report, &in_array[0], in_count, &btn);
                if (to_send == 0)
                    continue;
                else if (to_send == -1)
                    break;

                break;
            }
        }

        if (to_send > 0)
        {
            /* send usb report buffer */
            pthread_mutex_lock(&g_hid_mutex);
            if (g_hid_event_cb)
                g_hid_event_cb(hid_type, report, to_send);
            pthread_mutex_unlock(&g_hid_mutex);
        }
    }

EXIT:

    return NULL;
}

int usb_hid_register_fd(hid_type_e type, const char *str)
{
    int index = -1;
    pthread_mutex_lock(&g_hid_mutex);
    if (str)
    {
        for (int i = 0; i < INPUT_DEVICE_NUM; i++)
        {
            if (!g_hid_dev.dev[i].valid)
            {
                snprintf(g_hid_dev.dev[i].path, sizeof(g_hid_dev.dev[i].path), "%s", str);
                g_hid_dev.dev[i].type = type;
                g_hid_dev.dev[i].fd = open(g_hid_dev.dev[i].path, O_RDWR);
                if (g_hid_dev.dev[i].fd < 0)
                {
                    printf("Cannot open %s\n", g_hid_dev.dev[i].path);
                    break;
                }

                index = i;
                g_hid_dev.dev[i].valid = true;
                g_hid_dev.total++;
                break;
            }
        }

        if (index > -1)
        {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, 0x2000);

            if (pthread_create(&g_hid_dev.dev[index].tid, &attr, usb_hid_event, (void *)index) != 0)
            {
                printf("crate usb_hid_event thread fail.\n");
                close(g_hid_dev.dev[index].fd);
                g_hid_dev.dev[index].fd = -1;
                g_hid_dev.dev[index].valid = false;
                g_hid_dev.total--;
            }

            pthread_attr_destroy(&attr);
        }
    }
    pthread_mutex_unlock(&g_hid_mutex);

    return 0;
}

int usb_hid_unregister_fd(hid_type_e type, const char *str)
{
    (void)type;
    int index = -1;
    pthread_mutex_lock(&g_hid_mutex);
    if (str)
    {
        for (int i = 0; i < INPUT_DEVICE_NUM; i++)
        {
            if (g_hid_dev.dev[i].valid \
                && !strncmp(g_hid_dev.dev[i].path, str, strlen(g_hid_dev.dev[i].path)))
            {
                g_hid_dev.dev[i].valid = false;
                g_hid_dev.total--;
                index = i;
                break;
            }
        }
    }

    if (index > -1)
    {
        pthread_join(g_hid_dev.dev[index].tid, NULL);
        g_hid_dev.dev[index].tid = 0;
        close(g_hid_dev.dev[index].fd);
        g_hid_dev.dev[index].fd = -1;
    }

    pthread_mutex_unlock(&g_hid_mutex);

    return 0;
}

int usb_hid_set_event_cb(hid_event_cb cb)
{
    pthread_mutex_lock(&g_hid_mutex);
    g_hid_event_cb = cb;
    pthread_mutex_unlock(&g_hid_mutex);
}

int usb_hid_get_total()
{
    int total = 0;
    pthread_mutex_lock(&g_hid_mutex);
    total = g_hid_dev.total;
    pthread_mutex_unlock(&g_hid_mutex);

    return total;
}

#endif

