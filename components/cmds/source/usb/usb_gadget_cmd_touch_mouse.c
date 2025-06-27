#define LOG_TAG "hidg-tp"
#define ELOG_OUTPUT_LVL ELOG_LVL_INFO
#include <kernel/elog.h>

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <hcuapi/input.h>
#include <poll.h>
#include <kernel/io.h>
#include <kernel/drivers/hcusb.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/list.h>
#include <hcuapi/sys-blocking-notify.h>
#include <kernel/notify.h>
#include <pthread.h>
#include <sys/time.h>

#define TP_NAME "ft5x06"

#define CONTACT_TOTAL_CNT 5
#define REPORT_BUF_LEN 6
#define INPUT_EVENT_NUM 256
#define TP_REPORT_ID 0x03

static bool g_force_exit = false;

struct tp_mouse_report_st {
    uint8_t report_id;
    union {
        struct {
            uint8_t button : 2;
        };
        uint8_t dummy;
    };
    uint16_t x;
    uint16_t y;
} __attribute__((packed));

struct contact_handle {
    uint16_t x;
    uint16_t y;
    int8_t slot; // input type ABS_MT_SLOT(0x2f)
    int32_t id; // input type ABS_MT_TRACKING_ID(0x39)
    bool down;
};

struct contact_ctl {
    struct contact_handle contact_pool[CONTACT_TOTAL_CNT];
    uint8_t contact_count; // number of valid contact
    int8_t cur_slot; // current valid contact slot
};

static uint32_t get_time_in_us(void)
{
    static struct timeval cur;
    gettimeofday(&cur, NULL);
    return (uint32_t)(cur.tv_sec) * 10000 + (uint32_t)cur.tv_usec / 100;
}

static int init_contact_control(struct contact_ctl *p_contact)
{
    struct contact_handle *p_handle;
    int i;
    for (i = 0; i < CONTACT_TOTAL_CNT; i++) {
        p_handle = &p_contact->contact_pool[i];
        p_handle->id = -1;
        p_handle->slot = -1;
    }
    return 0;
}

static struct contact_handle *get_contact_handle(struct contact_ctl *p_contact,
                         int slot)
{
    if (slot >= CONTACT_TOTAL_CNT || slot < 0)
        return NULL;
    else
        return &p_contact->contact_pool[slot];
}

static int get_contact_handle_valid_num(struct contact_ctl *p_contact)
{
    int i, count;
    for (i = count = 0; i < CONTACT_TOTAL_CNT; i++)
        if (p_contact->contact_pool[i].slot >= 0)
            count++;
    return count;
}

static bool is_none_valid_contact(struct contact_ctl *p_contact)
{
    if ((p_contact->contact_pool[0].slot < 0) &&
        (p_contact->contact_pool[1].slot < 0) &&
        (p_contact->contact_pool[2].slot < 0) &&
        (p_contact->contact_pool[3].slot < 0) &&
        (p_contact->contact_pool[4].slot < 0))
        return true;
    else
        return false;
}

static int open_tp_device(void)
{
    DIR *dir;
    struct dirent *dp;
    char *path;
    char devname[64];
    int fd, ret = -1;

    dir = opendir("/dev/input");
    if (dir == NULL) {
        printf("Error: Cannot open /dev/input\n");
        return -1;
    }
    path = malloc(512);
    if (!path) {
        printf("cannot malloc 512 bytes!!\n");
        closedir(dir);
        return -1;
    }

    while ((dp = readdir(dir)) != NULL) {
        sprintf(path, "/dev/input/%s", dp->d_name);
        fd = open(path, O_RDWR);
        if (fd < 0)
            continue;
        ioctl(fd, EVIOCGNAME(sizeof(devname)), devname);
        if (strstr(devname, TP_NAME)) {
            log_i("touch panel device found (%s) !!\n", TP_NAME);
            ret = fd;
            break;
        }
        close(fd);
    }
    closedir(dir);
    free(path);
    return ret;
}

static void dump_hex(uint8_t *data, size_t size)
{
    size_t i = 0;
    for (i = 0; i < size; i++)
        printf("%02x ", data[i]);
    printf("\n");
}

static void dump_contact_info(struct contact_ctl *p_contact)
{
    struct contact_handle *p_handle = NULL;
    int i;
    printf("\n");
    for (i = 0; i < 5; i++) {
        p_handle = &p_contact->contact_pool[i];
        printf("[%d] %4d:%4d %2d:%4ld %s\n", i, p_handle->x,
               p_handle->y, p_handle->slot, p_handle->id,
               (p_handle->down) ? "down" : "up");
    }
}

static void update_contact_status(struct contact_ctl *p_contact,
                  struct input_event *events, int event_cnt)
{
    struct contact_handle *p_handle = NULL;
    struct input_event *event;
    int event_idx;
    int32_t id;

    /* update contact info */
    for (event_idx = 0; event_idx < event_cnt; event_idx++) {
        event = events + event_idx;
        if (event->type == EV_SYN) {
            switch (event->code) {
            case SYN_REPORT:
                if (event->value == 0)
                    goto __update_exit; // return when EV_SYN
            }
        } else if (event->type == EV_ABS) {
            switch (event->code) {
            case ABS_MT_SLOT:
                if ((event->value < 0) || (event->value > 5)) {
                    struct input_event *__event;
                    int index;
                    log_e("Error slot  : %ld\n",
                          event->value);
                    for (index = 0; index < event_cnt;
                         index++) {
                        __event = events + index;
                        printf("\t [%d] %x %3x %3ld\n",
                               index, __event->type,
                               __event->code,
                               __event->value);
                    }
                }

                p_contact->cur_slot = (int8_t)event->value;
                p_handle = &p_contact->contact_pool
                            [p_contact->cur_slot];
                break;

            case ABS_MT_TRACKING_ID: // need to create new contact
                id = event->value;
                p_handle = &p_contact->contact_pool
                            [p_contact->cur_slot];
                p_handle->id = id;
                p_handle->slot =
                    (id != -1) ? p_contact->cur_slot : -1;
                p_handle->down = (id != -1) ? 1 : 0;
                p_handle->x = (id != -1) ? p_handle->x : 0;
                p_handle->y = (id != -1) ? p_handle->y : 0;
                break;
            }
        }

        p_handle = (p_handle) ? (p_handle) :
                (&p_contact->contact_pool[p_contact->cur_slot]);

        /*
         * 只需要处理 EV_ABS
         */
        if (event->type == EV_ABS) {
            switch (event->code) {
            case ABS_MT_POSITION_X:
                p_handle->x = (uint16_t)event->value;
                break;

            case ABS_MT_POSITION_Y:
                p_handle->y = (uint16_t)event->value;
                break;
            }
        }
    }

__update_exit:
    return;
}

static void cleanup_contact_status(struct contact_ctl *p_contact)
{
    int i;
    struct contact_handle *p_handle;
    for (i = 0; i < CONTACT_TOTAL_CNT; i++) {
        p_handle = &p_contact->contact_pool[i];
        if ((p_handle->slot >= 0) && (p_handle->id == -1))
            p_handle->slot = -1;
    }
}

static int fill_hid_report(struct contact_ctl *p_contact,
               struct tp_mouse_report_st *report)
{
    struct contact_handle *p_handle;

    p_handle = &p_contact->contact_pool[0];
    report->report_id = TP_REPORT_ID;
    report->button = (p_handle->slot >= 0) ? 0x1 : 0x0;
    report->x = (uint16_t)(p_handle->x * 1);
    report->y = (uint16_t)(p_handle->y * 1);
    return 0;
}

static void *tp_thread(void *args)
{
    const char *filename = NULL;
    const char *hidg_name = NULL;
    int fd = -1, fdg = -1, ret = 0, in_count = 0;
    struct input_event *in_array, *in;
    static struct pollfd pfd = { 0 };
    struct tp_mouse_report_st report;
    uint8_t report_buf_bak[REPORT_BUF_LEN];
    int to_send;
    struct contact_ctl *p_contact = NULL;

    /* debug log level and tag */
    elog_set_filter_tag_lvl("hidg-tp", ELOG_LVL_INFO);
    elog_set_filter_tag_lvl("hcusb", ELOG_LVL_INFO);
    // elog_set_filter_tag_lvl("composite", ELOG_LVL_WARN);

    printf(" ==> touch panel pthread start ...\n");

    in_array = malloc(INPUT_EVENT_NUM * sizeof(struct input_event));
    if (!in_array) {
        log_e(" [Error] Cannot malloc enough memory (%ld bytes)\n",
              INPUT_EVENT_NUM * sizeof(struct input_event));
        goto __tp_exit;
    }
    memset(in_array, 0, INPUT_EVENT_NUM * sizeof(struct input_event));

    p_contact = malloc(sizeof(struct contact_ctl));
    if (!p_contact) {
        log_e(" [Error] Cannot malloc enough memory (%ld bytes)\n",
              sizeof(struct contact_ctl));
        goto __tp_exit;
    }
    memset(p_contact, 0, sizeof(struct contact_ctl));

    memset(report_buf_bak, 0, REPORT_BUF_LEN);

    /* try to open touch panel device file */
    fd = open_tp_device();
    if (fd < 0) {
        log_e(" [Error] cannot find touch panel device\n");
        goto __tp_exit;
    }

    hidg_name = "/dev/hidg-tp";

    if ((fdg = open(hidg_name, O_RDWR)) == -1) {
        log_e("Cannot open %s\n", hidg_name);
        goto __tp_exit;
    }

    /* init contact control object */
    ret = init_contact_control(p_contact);
    if (ret) {
        log_e(" [Error] init contact control fail\n");
        goto __tp_exit;
    }

    pfd.fd = fd;
    pfd.events = POLLIN | POLLRDNORM;
    in = &in_array[0];

    while (1) {
        /*
        * wait for input event
        */
        do {
            ret = poll(&pfd, 1, 100);
            if (ret == 0) { // timeout
                if (g_force_exit == true)
                    goto __tp_exit;
                else
                    continue;
            } else if (ret < 0) {
                log_e("%s poll error.\n", filename);
                ret = -1;
                goto __tp_exit;
            } else
                break;
        } while (1);

        /*
         * read all input events
         */
        in_count = 0;
        do {
            if (in_count >= INPUT_EVENT_NUM)
                break;

            in = &in_array[in_count];
            ret = read(fd, in, sizeof(struct input_event));
            if (ret != sizeof(struct input_event)) {
                log_e("%s read error.\n", filename);
                ret = -1;
                goto __tp_exit;
            }

            // printf("\t [%d] %x %3x %3ld\n", in_count,
            //             in->type, in->code, in->value);

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
                goto __tp_exit;
            }
        } while (1);

        /*
         * fill up usb hid report buffer
         */
        update_contact_status(p_contact, &in_array[0], in_count);
        // dump_contact_info(p_contact);

        fill_hid_report(p_contact, &report);

        cleanup_contact_status(p_contact);

        if (memcmp(report_buf_bak, (uint8_t *)&report, REPORT_BUF_LEN))
            memcpy(report_buf_bak, (uint8_t *)&report, REPORT_BUF_LEN);
        else
            continue;
        // dump_hex((uint8_t *)&report, REPORT_BUF_LEN);

        /*
         * send usb report buffer
         */
        if (write(fdg, (uint8_t *)&report,
                 REPORT_BUF_LEN) != REPORT_BUF_LEN) {
            log_e("Write %s error\n", hidg_name);
            ret = -1;
            goto __tp_exit;
        }
    }

__tp_exit:
    if (in_array)
        free(in_array);
    if (p_contact)
        free(p_contact);
    if (fd >= 0)
        close(fd);
    if (fdg >= 0)
        close(fdg);

    printf(" ==> touch panel pthread exit ...\n");

    return NULL;
}

static int usb_tp_notify(struct notifier_block *self, unsigned long action,
             void *param)
{
    pthread_attr_t attr;
    pthread_t threadid;

    switch (action) {
    case USB_GADGET_NOTIFY_CONNECT:
        printf(" ==> usb gadget touch panel connect ...\n");

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setstacksize(&attr, 128 * 1024);
        pthread_create(&threadid, &attr, tp_thread, NULL);
        g_force_exit = false;
        break;

    case USB_GADGET_NOTIFY_DISCONNECT:
        printf(" ==> usb gadget touch panel disconnect ...\n");
        g_force_exit = true;
        break;
    }
    return NOTIFY_OK;
}

static struct notifier_block usb_tp_nb = {
    .notifier_call = usb_tp_notify,
};

int hid_gadget_tp_mouse_demo(int argc, char **argv)
{
    const char *udc_name = get_udc_name(USB_PORT_1);
    char ch;
    int usb_port = 1;
    bool is_deinit = false;

    opterr = 0;
    optind = 0;

    while ((ch = getopt(argc, argv, "hHsSp:P:")) != EOF) {
        switch (ch) {
        case 'p':
        case 'P':
            usb_port = atoi(optarg);
            udc_name = get_udc_name(usb_port);
            if (udc_name == NULL) {
                printf("[error] parameter(-p {usb_port}) error\n");
                return -1;
            }
            break;

        case 's':
        case 'S':
            is_deinit = true;
            break;

        default:
            break;
        }
    }

    if (is_deinit) {
        printf(" deinit usb#%d gadget touch panel driver\n", usb_port);

        /*
        * deinit usb as usb gadget mode
        */
        hcusb_gadget_hidg_tp_deinit();
        hcusb_set_mode(usb_port, MUSB_HOST);

        /*
         * unregister hotplug notify
         */
        sys_unregister_notify(&usb_tp_nb);
    } else {
        printf(" init usb#%d gadget touch panel driver\n", usb_port);

        /*
        * config usb as usb gadget mode (touch panel)
        */
        hcusb_set_mode(usb_port, MUSB_PERIPHERAL);
        hcusb_gadget_hidg_tp_specified_init(get_udc_name(usb_port));

        /*
         * register hotplug notify
         */
        sys_register_notify(&usb_tp_nb);
    }
    return 0;
}

#include <kernel/lib/console.h>
CONSOLE_CMD(usbg_tp_mouse, NULL, hid_gadget_tp_mouse_demo,
        CONSOLE_CMD_MODE_SELF, "usb gadget touch panel as mouse device")
