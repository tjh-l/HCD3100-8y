#ifndef __HUDI_CVBS_RX_INTER_H__
#define __HUDI_CVBS_RX_INTER_H__

#include "hudi_com.h"
#include "hudi_list.h"
#include "hudi_cvbsrx.h"

#define HUDI_CVBS_RX_DEV    "/dev/tv_decoder"


typedef struct
{
    int on;
    int worker_key;
    int event_type;
    void *user_data;
    hudi_handle handle;
    hudi_cvbsrx_event_cb notifier;
} hudi_cvbsrx_event_t;

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
} hudi_cvbsrx_instance_t;

#endif
