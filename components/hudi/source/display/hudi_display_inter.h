#ifndef __HUDI_DISPLAY_INTER_H__
#define __HUDI_DISPLAY_INTER_H__

#define HUDI_DISPLAY_DEV    "/dev/dis"

typedef struct
{
    int on;
    int worker_key;
    int event_type;
    hudi_handle handle;
    hudi_display_cb notifier;
    void *user_data;
} hudi_display_event_t;

typedef struct
{
    int inited;
    int fd;
    unsigned int event_polling;
    unsigned int event_num;
    hudi_list_t * event_list;
    int fd_epoll;
    int fd_kumsg;
    int polling_stop;
    pthread_t polling_tid;
} hudi_display_instance_t;

#endif
