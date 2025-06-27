#ifndef __HUDI_HDMIRX_INTER_H__
#define __HUDI_HDMIRX_INTER_H__

#include "hudi_com.h"
#include "hudi_list.h"
#include "hudi_hdmirx.h"

#define HUDI_HDMI_RX_DEV    "/dev/hdmi_rx"
#define HUDI_HDMI_RX_SWITCH_DEV     "/dev/ms9601a"


typedef struct
{
    int on;
    int worker_key;
    int event_type;
    void *user_data;
    hudi_handle handle;
    hudi_hdmirx_event_cb notifier;
} hudi_hdmirx_event_t;

typedef struct
{
    int inited;
    int fd;
    int fd_epoll;
    int fd_kumsg;
    int polling_stop;
    int event_polling;
    int event_num;
    pthread_t polling_tid;
    hudi_list_t *event_list;
} hudi_hdmirx_instance_t;

typedef struct
{
    int inited;
    int fd;
} hudi_hdmirx_ms9601a_instance_t;

#endif
