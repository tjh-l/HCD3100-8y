#ifdef SUPPORT_HID
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <poll.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <hcuapi/input.h>
#include <hcuapi/usbhid.h>
#include <hcuapi/common.h>
#include <hcuapi/dis.h>
#include <kernel/lib/fdt_api.h>

#include <hccast/hccast_list.h>
#include <hccast/hccast_um.h>
#include <hidalgo/hccast_hid.h>
#include "cast_hid.h"

typedef struct
{
    unsigned char *data;
    unsigned int len;
} event_bo_t;

static pthread_mutex_t g_cast_hid_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_cast_hid_tid = 0;
static enum usbg_tp_type g_cast_hid_gadget_type = 0;
static list_t *g_tp_feed_list = NULL;
static int g_cast_hid_input_fd = -1;
static int g_cast_hid_gadget_fd = -1;
static int g_cast_hid_src_type = 0;
static int g_cast_hid_dst_type = 0;
static int g_cast_hid_stop = 0;

static void _cast_hid_event_bo_free(void *val)
{
    event_bo_t *bo = (event_bo_t *)val;

    if (!val)
    {
        return ;
    }

    if (bo->data)
    {
        free(bo->data);
    }

    free(val);
}

static void *_cast_hid_tp_mouse_event_dequeue(void *arg)
{
    event_bo_t *bo = NULL;
    list_node_t *node = NULL;
    unsigned char *buf = NULL;
    unsigned short x, y;
    int feed;
    int currentPolicy;
    struct sched_param newPriority;

    pthread_getschedparam(pthread_self(), &currentPolicy, &newPriority);
    newPriority.sched_priority = 14;
    pthread_setschedparam(pthread_self(), currentPolicy, &newPriority);

    while (!g_cast_hid_stop)
    {
        pthread_mutex_lock(&g_cast_hid_mutex);

        if (g_cast_hid_gadget_fd >= 0 && g_tp_feed_list && g_tp_feed_list->len > 0)
        {
            node = list_lpop(g_tp_feed_list);
            if (node)
            {
                bo = (event_bo_t *)node->val;
                buf = bo->data;
                write(g_cast_hid_gadget_fd, bo->data, bo->len);
                x = buf[3] << 8 | buf[2];
                y = buf[5] << 8 | buf[4];
#if 0
                printf("%.2x %.2x (%dx%d) (%d)\n", buf[0], buf[1], x, y, last_tick);
#endif
                _cast_hid_event_bo_free(bo);
                feed = 1;
            }
        }

        pthread_mutex_unlock(&g_cast_hid_mutex);

        if (feed)
        {
            usleep(10 * 1000);
            feed = 0;
        }
        else
        {
            usleep(1000);
        }
    }

    return NULL;
}

static int _cast_hid_event_feed(unsigned int type, char *data, unsigned int len)
{
    char *buf = NULL;
    event_bo_t *bo = NULL;
    list_node_t *node = NULL;

    pthread_mutex_lock(&g_cast_hid_mutex);

    if (usbg_touch_mouse == g_cast_hid_gadget_type)
    {
        switch (type)
        {
            case HCCAST_HID_EVT_TP_MOUSE:
                if (g_tp_feed_list)
                {
                    buf = (unsigned char *)malloc(len);
                    memcpy(buf, data, len);
                    bo = malloc(sizeof(event_bo_t));
                    bo->data = buf;
                    bo->len = len;
                    node = list_node_new(bo);
                    list_rpush(g_tp_feed_list, node);
                }
                break;
        }
    }
    else
    {
        if (CAST_HID_CHANNEL_USB == g_cast_hid_dst_type)
        {
            if (g_cast_hid_gadget_fd >= 0)
            {
                write(g_cast_hid_gadget_fd, data, len);
            }
        }
        else if (CAST_HID_CHANNEL_AUM == g_cast_hid_dst_type)
        {
            if (HCCAST_HID_EVT_TP_MOUSE == type || HCCAST_HID_EVT_TP_NORMAL == type)
            {
                hccast_aum_hid_feed(HCCAST_UM_HID_TP, data, len);
            }
            else if (HCCAST_HID_EVT_KEYBOARD == type)
            {
                hccast_aum_hid_feed(HCCAST_UM_HID_KEYBOARD, data, len);
            }
            else if (HCCAST_HID_EVT_MOUSE == type)
            {
                hccast_aum_hid_feed(HCCAST_UM_HID_MOUSE, data, len);
            }
        }
    }

    pthread_mutex_unlock(&g_cast_hid_mutex);

    return 0;
}

static void *_cast_hid_input_thread(void *arg)
{
    struct pollfd pfd = {0};
    struct input_event evt;
    struct sched_param newPriority;
    pthread_t tid;
    int currentPolicy;
    int ret;
    int np;
    unsigned int rotate = 0;
    hccast_hid_args_t args;

    if (g_cast_hid_input_fd < 0)
    {
        return NULL;
    }

    pthread_getschedparam(pthread_self(), &currentPolicy, &newPriority);
    newPriority.sched_priority = 14;
    pthread_setschedparam(pthread_self(), currentPolicy, &newPriority);

    if (usbg_touch_mouse == g_cast_hid_gadget_type)
    {
        g_tp_feed_list = list_new();
        g_tp_feed_list->free = _cast_hid_event_bo_free;
        pthread_create(&tid, NULL, _cast_hid_tp_mouse_event_dequeue, NULL);
    }

    memset(&args, 0, sizeof(args));

    np = fdt_node_probe_by_path("/hcrtos/tp-info");
    if (np >= 0)
    {
        fdt_get_property_u_32_index(np, "width", 0, &args.tp_width);
        fdt_get_property_u_32_index(np, "height", 0, &args.tp_height);
        fdt_get_property_u_32_index(np, "h-invert", 0, &args.tp_h_inv);
        fdt_get_property_u_32_index(np, "v-invert", 0, &args.tp_v_inv);
        printf("TP info (%dx%d) %d %d\n", args.tp_width, args.tp_height, args.tp_h_inv, args.tp_v_inv);
    }

    np = fdt_node_probe_by_path("/hcrtos/rotate");
    if (np >= 0)
    {
        fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
        args.scr_rotate = rotate / 90;
    }

    if (usbg_touch_mouse == g_cast_hid_gadget_type)
    {
        args.tp_type = HCCAST_HID_TP_MOUSE;
    }
    else
    {
        args.tp_type = HCCAST_HID_TP_NORMAL;
    }
    hccast_hid_start(&args, _cast_hid_event_feed);

    pfd.fd = g_cast_hid_input_fd;
    pfd.events = POLLIN | POLLRDNORM;

    while (!g_cast_hid_stop)
    {
        if (poll(&pfd, 1, 10) <= 0)
        {
            usleep(5 * 1000);
            continue;
        }

        while (read(g_cast_hid_input_fd, &evt, sizeof(struct input_event)) == sizeof(struct input_event))
        {
#if 0
            printf("\t [type]%.4x [code]%.4x [value]%.8x\n", evt.type, evt.code, evt.value);
#endif
            hccast_hid_event_feed(&evt);
        }
    }

    if (g_tp_feed_list)
    {
        pthread_join(tid, NULL);
        list_destroy(g_tp_feed_list);
        g_tp_feed_list = NULL;
    }

    return NULL;
}

int cast_hid_probe(void)
{
    DIR *dir = NULL;
    struct dirent *dp = NULL;
    char path[512] = {0};
    char devname[128] = {0};
    int fd = -1;

    pthread_mutex_lock(&g_cast_hid_mutex);

    dir = opendir("/dev/input");
    if (!dir)
    {
        pthread_mutex_unlock(&g_cast_hid_mutex);
        return -1;
    }

    while ((dp = readdir(dir)) != NULL)
    {
        if (DT_DIR == dp->d_type)
        {
            continue;
        }

        memset(path, 0, 512);
        snprintf(path, 512, "/dev/input/%s", dp->d_name);
        fd = open(path, O_RDWR);
        if (fd < 0)
        {
            continue;
        }

        memset(devname, 0, 128);
        ioctl(fd, EVIOCGNAME(sizeof(devname)), devname);

        printf("HID dev (%s)\n", devname);
        if (strstr(devname, "gt911"))
        {
            g_cast_hid_input_fd = fd;
            g_cast_hid_src_type = CAST_HID_TYPE_TOUCHPAD;
            g_cast_hid_stop = 0;
            break;
        }

        close(fd);
    }

    closedir(dir);

    pthread_mutex_unlock(&g_cast_hid_mutex);

    return g_cast_hid_src_type;
}

int cast_hid_release(void)
{
    pthread_mutex_lock(&g_cast_hid_mutex);

    if (g_cast_hid_input_fd >= 0)
    {
        close(g_cast_hid_input_fd);
        g_cast_hid_input_fd = -1;
    }

    g_cast_hid_src_type = 0;

    pthread_mutex_unlock(&g_cast_hid_mutex);

    return 0;
}

int cast_hid_start(unsigned int channel)
{
    int ret = 0;

    pthread_mutex_lock(&g_cast_hid_mutex);

    if (g_cast_hid_src_type <= 0)
    {
        pthread_mutex_unlock(&g_cast_hid_mutex);
        return 0;
    }

    if (CAST_HID_CHANNEL_USB == channel)
    {
        g_cast_hid_gadget_fd = open("/dev/hidg-tp", O_RDWR);
        if (g_cast_hid_gadget_fd < 0)
        {
            printf("Open /dev/hidg-tp fail\n");
            pthread_mutex_unlock(&g_cast_hid_mutex);
            return -1;
        }
        ioctl(g_cast_hid_gadget_fd, USBHID_GADGET_GET_TP_TYPE, &g_cast_hid_gadget_type);
    }
    else if (CAST_HID_CHANNEL_AUM == channel)
    {
        g_cast_hid_gadget_type = usbg_touch_panel;
        g_cast_hid_dst_type = channel;
    }

    printf("HID gadget type %d\n", g_cast_hid_gadget_type);

    pthread_create(&g_cast_hid_tid, NULL, _cast_hid_input_thread, NULL);

    pthread_mutex_unlock(&g_cast_hid_mutex);

    return 0;
}

int cast_hid_stop(void)
{
    pthread_mutex_lock(&g_cast_hid_mutex);

    if (g_cast_hid_tid)
    {
        g_cast_hid_stop = 1;
        pthread_join(g_cast_hid_tid, NULL);
        g_cast_hid_tid = 0;
        g_cast_hid_stop = 0;
    }

    if (g_cast_hid_gadget_fd > 0)
    {
        close(g_cast_hid_gadget_fd);
        g_cast_hid_gadget_fd = -1;
    }

    hccast_hid_stop();

    pthread_mutex_unlock(&g_cast_hid_mutex);

    return 0;
}
#endif
