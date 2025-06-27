#ifndef __HUDI_VDEC_INTER_H__
#define __HUDI_VDEC_INTER_H__

#include <hudi_list.h>

#define HUDI_VDEC_DEV   "/dev/viddec"
#define HUDI_VIDSINK_DEV   "/dev/vidsink"

typedef struct
{
    int on;
    int worker_key;
    int event_type;
    hudi_handle handle;
    hudi_vdec_cb notifier;
    void *user_data;
} hudi_vdec_event_t;

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
} hudi_vdec_instance_t;

typedef struct
{
    int inited;
    int fd;
} hudi_vidsink_instance_t;

#endif
