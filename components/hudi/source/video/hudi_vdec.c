/**
* @file
* @brief                hudi video decoder interface
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_list.h>
#include <hudi_vdec.h>

#include "hudi_video_inter.h"

static pthread_mutex_t g_hudi_vdec_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_vdec_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_vdec_mutex);

    return 0;
}

static int _hudi_vdec_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_vdec_mutex);

    return 0;
}

static void _hudi_vdec_event_free(void *val)
{
    hudi_vdec_event_t *event = (hudi_vdec_event_t *)val;

    if (!event)
    {
        return ;
    }

    free(event);
}

#ifdef __HCRTOS__
#include <nuttx/wqueue.h>

static void _hudi_vdec_event_notifier(void *arg, unsigned long param)
{
    hudi_vdec_event_t *event = (hudi_vdec_event_t *)arg;

    if (event->on && event->notifier)
    {
        event->notifier(event->handle, event->event_type, (void *)param, event->user_data);
    }
}

static int _hudi_vdec_polling_start(hudi_vdec_instance_t *inst)
{
    inst->event_polling = 1;

    return 0;
}

static int _hudi_vdec_polling_stop(hudi_vdec_instance_t *inst)
{
    inst->event_polling = 0;

    return 0;
}

static int _hudi_vdec_event_add(hudi_vdec_instance_t *inst, hudi_vdec_event_t *event, int event_type)
{
    struct work_notifier_s notifier = {0};

    notifier.evtype = event_type;
    notifier.qid = HPWORK;
    notifier.remote = 0;
    notifier.oneshot = 0;
    notifier.qualifier = (void *)inst->fd;
    notifier.arg = event;
    notifier.worker2 = _hudi_vdec_event_notifier;

    event->worker_key = work_notifier_setup(&notifier);
    if (event->worker_key < 0)
    {
        hudi_log(HUDI_LL_WARNING, "VDEC notifier(%d) setup fail\n", notifier.evtype);
        return -1;
    }

    event->on = 1;

    return 0;
}

static int _hudi_vdec_event_delete(hudi_vdec_instance_t *inst, hudi_vdec_event_t *event)
{
    if (event->worker_key > 0)
    {
        work_notifier_teardown(event->worker_key);
        event->worker_key = 0;
        event->on = 0;
    }
}
#else
#include <sys/epoll.h>
#include <hcuapi/kumsgq.h>

static void *_hudi_vdec_polling_thread(void *arg)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)arg;
    struct epoll_event events[10];
    KuMsgDH kumsg;
    hudi_vdec_event_t *pev = NULL;
    int n = 0;
    int i;

    while (!inst->polling_stop)
    {
        n = epoll_wait(inst->fd_epoll, events, 10, 0);
        if (n <= 0)
        {
            usleep(100 * 1000);
            continue;
        }

        for (i = 0; i < n; i ++)
        {
            if (sizeof(KuMsgDH) == read(inst->fd_kumsg, &kumsg, sizeof(KuMsgDH)))
            {
                switch (kumsg.type)
                {
                    case AVEVENT_VIDDEC_FIRST_FRAME_DECODED:
                    case AVEVENT_VIDDEC_FIRST_FRAME_SHOWED:
                    case AVEVENT_VIDDEC_FRAME_DECODED:
                        pev = (hudi_vdec_event_t *)kumsg.user_args;
                        if (pev->on && pev->notifier)
                        {
                            pev->notifier(inst, pev->event_type, (void *)kumsg.params, pev->user_data);
                        }

                        break;
                    default:
                        hudi_log(HUDI_LL_WARNING, "VDEC unhandle event %d\n", kumsg.type);
                        break;
                }
            }
        }
    }

    inst->polling_stop = 0;

    return NULL;
}

static int _hudi_vdec_polling_start(hudi_vdec_instance_t *inst)
{
    struct epoll_event ev;

    inst->fd_kumsg = ioctl(inst->fd, KUMSGQ_FD_ACCESS, O_CLOEXEC);
    if (inst->fd_kumsg < 0)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC kumsgq access fail\n");
        return -1;
    }

    inst->fd_epoll = epoll_create1(0);

    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.ptr = (void *)inst;
    if (epoll_ctl(inst->fd_epoll, EPOLL_CTL_ADD, inst->fd_kumsg, &ev) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC epoll control add fail\n");
        close(inst->fd_epoll);
        close(inst->fd_kumsg);
        inst->fd_epoll = -1;
        inst->fd_kumsg = -1;
        return -1;
    }

    pthread_create(&inst->polling_tid, NULL, _hudi_vdec_polling_thread, (void *)inst);

    inst->event_polling = 1;

    return 0;
}

static int _hudi_vdec_polling_stop(hudi_vdec_instance_t *inst)
{
    if (!inst->event_polling)
    {
        return -1;
    }

    inst->polling_stop = 1;
    pthread_join(inst->polling_tid, NULL);

    close(inst->fd_epoll);
    close(inst->fd_kumsg);

    inst->event_polling = 0;

    return 0;
}

static int _hudi_vdec_event_add(hudi_vdec_instance_t *inst, hudi_vdec_event_t *event, int event_type)
{
    struct kumsg_event ev = {0};

    ev.evtype = event_type;
    ev.arg = (unsigned long)event;
    if (ioctl(inst->fd, KUMSGQ_NOTIFIER_SETUP, &ev) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC event add fail\n");
        return -1;
    }

    event->on = 1;

    return 0;
}

static int _hudi_vdec_event_delete(hudi_vdec_instance_t *inst, hudi_vdec_event_t *event)
{
    if (!event->on)
    {
        return -1;
    }

    event->on = 0;

    return 0;
}
#endif

int hudi_vdec_open(hudi_handle *handle, struct video_config *config)
{
    hudi_vdec_instance_t *inst = NULL;

    if (!handle || !config)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid vdec parameters\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    inst = (hudi_vdec_instance_t *)malloc(sizeof(hudi_vdec_instance_t));
    memset(inst, 0, sizeof(hudi_vdec_instance_t));
    inst->fd_epoll = -1;
    inst->fd_kumsg = -1;
    inst->event_list = NULL;

    inst->fd = open(HUDI_VDEC_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open vdec fail\n");
        free(inst);
        _hudi_vdec_mutex_unlock();
        return -1;
    }

    if (0 != ioctl(inst->fd, VIDDEC_INIT, config))
    {
        hudi_log(HUDI_LL_ERROR, "Init vdec fail\n");
        close(inst->fd);
        free(inst);
        _hudi_vdec_mutex_unlock();
        return -1;
    }
    
    inst->inited = 1;
    *handle = inst;

    _hudi_vdec_mutex_unlock();

    return 0;
}

int hudi_vdec_close(hudi_handle handle, int frame_backup)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)handle;
    struct vdec_rls_param rls_param = {0, 0};
    hudi_list_node_t *node = NULL;
    hudi_vdec_event_t *event = NULL;
    int i;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    if (inst->event_num > 0)
    {
        for (i = 0; i < inst->event_list->len; i ++)
        {
            node = hudi_list_at(inst->event_list, i);
            event = (hudi_vdec_event_t *)node->val;
            if (event->on)
            {
                _hudi_vdec_event_delete(inst, event);
                inst->event_num --;
            }
        }
    }

    if (inst->event_polling)
    {
        _hudi_vdec_polling_stop(inst);
    }

    if (inst->event_list)
    {
        hudi_list_destroy(inst->event_list);
        inst->event_list = NULL;       
    }    

    if (frame_backup)
    {
        ioctl(inst->fd, VIDDEC_RLS, &rls_param);
    }
    close(inst->fd);

    memset(inst, 0, sizeof(hudi_vdec_instance_t));
    free(inst);

    _hudi_vdec_mutex_unlock();

    return 0;
}

int hudi_vdec_start(hudi_handle handle)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    ret = ioctl(inst->fd, VIDDEC_START, 0);

    _hudi_vdec_mutex_unlock();

    return ret;
}

int hudi_vdec_pause(hudi_handle handle)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    ret = ioctl(inst->fd, VIDDEC_PAUSE, 0);

    _hudi_vdec_mutex_unlock();

    return ret;
}

int hudi_vdec_feed(hudi_handle handle, char *data, AvPktHd *pkt)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)handle;
    unsigned char data_aux[8] = {0};
    float rate = 1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    if (write(inst->fd, (uint8_t *)pkt, sizeof(AvPktHd)) != sizeof(AvPktHd))
    {
        hudi_log(HUDI_LL_ERROR, "Write vpkt fail\n");
        ioctl(inst->fd, VIDDEC_FLUSH, &rate);
        _hudi_vdec_mutex_unlock();
        return -1;
    }
    
    if (write(inst->fd, data, pkt->size) != pkt->size)
    {
        hudi_log(HUDI_LL_ERROR, "Write vframe fail\n");
        ioctl(inst->fd, VIDDEC_FLUSH, &rate);
        _hudi_vdec_mutex_unlock();
        return -1;
    }

    _hudi_vdec_mutex_unlock();

    return 0;
}

int hudi_vdec_event_register(hudi_handle handle, hudi_vdec_cb func, int event_type, void *user_data)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)handle;
    hudi_list_node_t *node = NULL;
    hudi_vdec_event_t *event = NULL;
    hudi_vdec_event_t *new_event = NULL;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    if (inst->event_list == NULL)
    {
        inst->event_list = hudi_list_new();
        inst->event_list->free = _hudi_vdec_event_free;
    }   
    
    if (!inst->event_polling)
    {
        _hudi_vdec_polling_start(inst);
    }

    for (int i = 0; i < inst->event_list->len; i++)
    {
        node = hudi_list_at(inst->event_list, i);
        event = (hudi_vdec_event_t *)node->val;
        
        if (event->event_type == event_type)
        {
            if (event->on)
            {
                _hudi_vdec_mutex_unlock();
                return 0;
            }
            else
            {
                new_event = event;
                break;
            }
        }
    }

    if (new_event == NULL)
    {
        new_event = calloc(sizeof(hudi_vdec_event_t), 1);
        new_event->event_type = event_type;
        new_event->handle = handle;
        new_event->notifier = func;
        new_event->user_data = user_data;
        node = hudi_list_node_new(new_event);
        hudi_list_rpush(inst->event_list, node);
    }
    else
    {   
        new_event->event_type = event_type;
        new_event->handle = handle;
        new_event->notifier = func;
        new_event->user_data = user_data;
    }
    
    _hudi_vdec_event_add(inst, new_event, event_type);
    inst->event_num ++;

    _hudi_vdec_mutex_unlock();

    return 0;
}

int hudi_vdec_event_unregister(hudi_handle handle, int event_type)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)handle;
    hudi_list_node_t *node = NULL;
    hudi_vdec_event_t *event = NULL;
    int found = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    for (int i = 0; i < inst->event_list->len; i++)
    {
        node = hudi_list_at(inst->event_list, i);
        event = (hudi_vdec_event_t *)node->val;

        if (event->on && event->event_type == event_type)
        {
            found = 1;
            break;
        }
    }

    if (found)
    {
        _hudi_vdec_event_delete(inst, event);
        inst->event_num --;
        event->handle = NULL;
        event->notifier = NULL;
        event->user_data = NULL;
    }

    if (!inst->event_num)
    {
        _hudi_vdec_polling_stop(inst);
    }

    _hudi_vdec_mutex_unlock();

    return 0;
}

int hudi_vdec_stat_get(hudi_handle handle, struct vdec_decore_status *stat)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)handle;

    if (!inst || !inst->inited || !stat)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    if (0 != ioctl(inst->fd, VIDDEC_GET_STATUS, stat))
    {
        hudi_log(HUDI_LL_ERROR, "Get status fail\n");
        _hudi_vdec_mutex_unlock();
        return -1;
    }

    _hudi_vdec_mutex_unlock();

    return 0;
}

int hudi_vdec_pts_get(hudi_handle handle, long long *pts)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    ret = ioctl(inst->fd, VIDDEC_GET_CUR_TIME, pts);

    _hudi_vdec_mutex_unlock();

    return ret;
}

int hudi_vdec_fps_set(hudi_handle handle, unsigned int fps)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    ret = ioctl(inst->fd, VIDDEC_SET_FRAMERATE, fps);

    _hudi_vdec_mutex_unlock();

    return ret;
}

int hudi_vdec_waterline_get(hudi_handle handle, unsigned int *percent)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)handle;
    unsigned int value = 0;
    int ret = -1;

    if (!inst || !inst->inited || !percent)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    ret = ioctl(inst->fd, GET_AV_BUFFERING_PERCENT, &value);
    *percent = value;

    _hudi_vdec_mutex_unlock();

    return ret;
}

int hudi_vdec_masaic_mode_set(hudi_handle handle, unsigned int mode)
{
    hudi_vdec_instance_t *inst = (hudi_vdec_instance_t *)handle;
    int ret = -1;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "VDEC not open\n");
        return -1;
    }

    _hudi_vdec_mutex_lock();

    ret = ioctl(inst->fd, VIDDEC_SET_SHOW_MASAIC_ON_ERR, mode);

    _hudi_vdec_mutex_unlock();

    return ret;
}
