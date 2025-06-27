#include "app_config.h"

#ifdef UIBC_SUPPORT

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <hccast/hccast_mira.h>
#include <hccast/hccast_wifi_mgr.h>

#include "usb_hid.h"

static char g_kbd_report_desc[]  =
{
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x03,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, 0x65,        //   Usage Maximum (0x65)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    // 63 bytes
};

static char g_mouse_report_desc[] =
{
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x03,        //     Usage Maximum (0x03)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x03,        //     Report Count (3)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x05,        //     Report Size (5)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection
    // 52 bytes
};

typedef struct
{
    char *data;
    size_t data_len;
    bool data_valid;
} uibc_message_t;

static int g_uibc_sockfd = -1;
hccast_mira_cat_t g_uibc_enable = HCCAST_MIRA_CAT_NONE;

static void dump_data(char *data, size_t len)
{
    if (!data || len == 0)
        return;

    printf("dump:");
    for (int i = 0; i < len; i++)
    {
        printf(" %2.2x", (uint8_t)data[i]);
    }
    printf("\n");
}

static int send_uibc_message(uibc_message_t *msg, int sockfd)
{
    size_t n = 0;
    ssize_t r = 0;
    int retries = 0;
    int max_retries = 10;
    int backoff_ms = 100;
    int max_backoff_ms = 10000;

    fd_set write_fds;
    struct timeval timeout = {0, 0};

    if (msg == NULL)
    {
        printf("uibc message is NULL\n");
        return -1;
    }

    do
    {
        FD_ZERO(&write_fds);
        FD_SET(sockfd, &write_fds);

        timeout.tv_sec = (backoff_ms * (1 << retries) * 1000) / 1000000;
        timeout.tv_usec = (backoff_ms * (1 << retries) * 1000) % 1000000;

        if (timeout.tv_usec > max_backoff_ms)
        {
            timeout.tv_usec = max_backoff_ms;
        }

        r = select(sockfd + 1, NULL, &write_fds, NULL, &timeout);
        if (r < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                printf("select SOCKET ERROR(%d)\n", errno);
                return -1;
            }
        }
        else if (r == 0)
        {
            if (retries > max_retries)
            {
                printf("write max retries reached)\n");
                break;
            }

            retries++;
            continue;
        }

        r = write(sockfd, msg->data + n, msg->data_len - n);
        if (r < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                printf("write SOCKET ERROR(%d/%d)\n", r, errno);
                return -1;
            }
        }
        else if (r == 0)
        {
            return 0;
        }
        else
        {
            n += r;
        }
    }
    while (n < msg->data_len);

    return 0;
}

static void build_hidc_packet(hid_type_e type, char usage, const char *pack, int pack_len, uibc_message_t *msg)
{
    size_t out_len = pack_len + 9;   // 2 + 2 + 1 + 1 + 1 + 2 + (HIDC value)
    char *out = (char *)malloc(out_len);
    if (out == NULL)
    {
        printf("malloc error\n");
        return;
    }

    memset(out, 0, out_len);

    int offset = 0;
    // UIBC header Octets
    //Version (3 bits),T (1 bit),Reserved(8 bits),Input Category (4 bits)
    out[offset++] = 0x00; // 000 0 0000
    out[offset++] = 0x01; // 0000 0001 HIDC

    // Total Length 2 octetss
    out[offset++] = (out_len >> 8) & 0xFF;   // first 8 bytes
    out[offset++] = out_len & 0xFF;          // last 8 bytes

    // HIDC Input Body Format
    out[offset++] = 1;       // InputPath,  USB
    out[offset++] = type;    // Type, mouse
    out[offset++] = usage;   // Usage, input report (0x0) or input report descriptor (0x1)

    // HIDC value Length, 2 octetss
    out[offset++] = (pack_len >> 8) & 0xFF;    // first 8 bytes
    out[offset++] = pack_len & 0xFF;           //last 8 bytes

    // HIDC value
    for (int i = 0; i < pack_len;)
        out[offset++] = pack[i++];

    msg->data = out;
    msg->data_valid = true;
    msg->data_len = out_len;
}

static uibc_message_t build_uibc_hidc_message(hid_type_e type, char usage, const char *pBuf, int buf_len)
{
    uibc_message_t msg;
    msg.data = NULL;
    msg.data_len = 0;
    msg.data_valid = false;

    if (type >= HID_KEYBOARD && type <= HID_REMOTE_CONTROL)
        build_hidc_packet(type, usage, pBuf, buf_len, &msg);

    return msg;
}

static int uibc_hid_message2_send(int sock, const char type, char usage, const char *pBuf, int len)
{
    int ret = 0;
    uibc_message_t msg = {0};

    if (sock > 0)
    {
        msg = build_uibc_hidc_message(type, usage, pBuf, len);
        if (msg.data_len)
        {
            //dump_data(msg.data, msg.data_len);
            ret = send_uibc_message(&msg, sock);
            free(msg.data);
        }
    }

    return ret;
}

static int uibc_hid_message_send(hid_type_e hid_type, const char *report, int repoet_len)
{
    return uibc_hid_message2_send(g_uibc_sockfd, hid_type, 0, report, repoet_len);
}

static void *uibc_conn_create(void *arg)
{
    int sockfd = -1;
    int r;
    int opt_sw;
    int timeout = 5;
    hccast_mira_cat_t *cat = (hccast_mira_cat_t *)arg;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("ERROR opening socket\n");
        goto EXIT;
    }

    opt_sw = 1;
    r = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt_sw, sizeof(opt_sw));
    if (r < 0)
    {
        printf("ERROR set socket\n");
        goto EXIT;
    }

#if 0 // lwip dont support TCP_QUICKACK
    r = setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, &opt_sw, sizeof(opt_sw));
    if (r < 0)
    {
        printf("ERROR set socket\n");
        goto EXIT;
    }
#endif

    int tos = (46 << 2);    // DSCP: EF PHB, ECN: Not-ECT
    // int tos = (56 << 2);    // DSCP: CS7, ECN: Not-ECT. CS7 > EF
    setsockopt(sockfd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    //remote.sin_addr.s_addr = inet_addr(ip);
    remote.sin_addr.s_addr = hccast_wifi_mgr_p2p_get_ip();
    remote.sin_port = htons(hccast_mira_uibc_get_port());

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        perror("fcntl get");
        goto EXIT;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl set");
        goto EXIT;
    }

    do
    {
        int ret = connect(sockfd, (struct sockaddr *)&remote, sizeof(remote));
        if (ret < 0)
        {
            if (errno == EINPROGRESS)
            {
                printf("uibc Connection in progress...\n");

                fd_set fdset;
                struct timeval tv;
                FD_ZERO(&fdset);
                FD_SET(sockfd, &fdset);
                tv.tv_sec = 1;
                tv.tv_usec = 0;

                ret = select(sockfd + 1, NULL, &fdset, NULL, &tv);
                if (ret > 0)
                {
                    int so_error;
                    socklen_t len = sizeof(so_error);

                    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0)
                    {
                        perror("getsockopt");
                        goto EXIT;
                    }

                    if (so_error == 0)
                    {
                        printf("uibc Connected successfully\n");
                        break;
                    }
                    else
                    {
                        printf("uibc Connection failed: %s\n", strerror(so_error));
                        //goto EXIT;
                    }
                }
                else if (ret == 0)
                {
                    if (timeout-- > 0)
                    {
                        continue;
                    }

                    printf("uibc Connection timed out\n");
                    goto EXIT;
                }
                else
                {
                    perror("select");
                    goto EXIT;
                }
            }
            else
            {
                perror("connect");
                goto EXIT;
            }
        }
        else
        {
            printf("uibc Connected immediately\n");
            break;
        }
    }
    while (*cat);

    if ((*cat) == HCCAST_MIRA_CAT_HID)
    {
        uibc_hid_message2_send(sockfd, HID_MOUSE, 1, g_mouse_report_desc, sizeof(g_mouse_report_desc));
        usleep(1000u);
        uibc_hid_message2_send(sockfd, HID_KEYBOARD, 1, g_kbd_report_desc, sizeof(g_kbd_report_desc));
        usleep(1000u);
        g_uibc_sockfd = sockfd;
        return NULL;
    }

EXIT:
    if (sockfd > 0) close(sockfd);
    sockfd = -1;

    return NULL;
}

static int uibc_conn_destroy()
{
    if (g_uibc_sockfd > 0) close(g_uibc_sockfd);
    g_uibc_sockfd = -1;

    return 0;
}

int uibc_hid_enable()
{
    g_uibc_enable = HCCAST_MIRA_CAT_HID;

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &attr, uibc_conn_create, &g_uibc_enable) != 0)
    {
        pthread_attr_destroy(&attr);
        return -1;
    }
    pthread_attr_destroy(&attr);

    usb_hid_set_event_cb(uibc_hid_message_send);

    return 0;
}

int uibc_hid_disable()
{
    usb_hid_set_event_cb(NULL);
    g_uibc_enable = HCCAST_MIRA_CAT_NONE;
    uibc_conn_destroy();

    return 0;
}

#endif
