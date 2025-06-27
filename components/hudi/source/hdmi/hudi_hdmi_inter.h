#ifndef __HUDI_HDMI_INTER_H__
#define __HUDI_HDMI_INTER_H__

#define HUDI_HDMI_DEV   "/dev/hdmi"

typedef struct
{
    int on;
    int worker_key;
    int event_type;
    hudi_handle handle;
    hudi_hdmi_cb notifier;
    void *user_data;
} hudi_hdmi_event_t;

typedef struct
{
    int inited;
    int fd;
    unsigned int event_polling;
    unsigned int event_num;
    hudi_list_t *event_list;
    int fd_epoll;
    int fd_kumsg;
    int polling_stop;
    pthread_t polling_tid;
} hudi_hdmi_instance_t;

#endif
