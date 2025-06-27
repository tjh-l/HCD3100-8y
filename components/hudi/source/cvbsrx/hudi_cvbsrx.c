/**
* @file
* @brief                hudi cvbsrx interface
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
#include <hudi_log.h>
#include <hudi_cvbsrx.h>
#include "hudi_cvbsrx_inter.h"



static pthread_mutex_t g_hudi_cvbsrx_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_cvbsrx_mutex_lock(void)
{
    pthread_mutex_lock(&g_hudi_cvbsrx_mutex);
    return 0;
}

static int _hudi_cvbsrx_mutex_unlock(void)
{
    pthread_mutex_unlock(&g_hudi_cvbsrx_mutex);
    return 0;
}

static void _hudi_cvbsrx_event_free(void *val)
{
    hudi_cvbsrx_event_t *event = (hudi_cvbsrx_event_t *)val;

    if (!event)
    {
        return ;
    }

    free(event);
}

#ifdef __HCRTOS__
#include <kernel/vfs.h>
#include <kernel/completion.h>
#include <kernel/io.h>
#include <nuttx/wqueue.h>

static int _hudi_cvbsrx_polling_start(hudi_cvbsrx_instance_t *inst)
{
    inst->event_polling = 1;
    return 0;
}

static void _hudi_cvbsrx_event_notifier(void *arg, unsigned long param)
{
    hudi_cvbsrx_event_t *event = (hudi_cvbsrx_event_t *)arg;

    if (event->on && event->notifier)
    {
        event->notifier(event->handle, event->event_type, (void *)param, event->user_data);
    }
}

static int _hudi_cvbsrx_event_add(hudi_cvbsrx_instance_t *inst, hudi_cvbsrx_event_t *event)
{
    struct work_notifier_s work_queue = {0};

    work_queue.evtype    = event->event_type;
    work_queue.qid       = LPWORK;
    work_queue.remote    = false;
    work_queue.oneshot   = false;
    work_queue.qualifier = NULL;
    work_queue.arg       = (void *)event;
    work_queue.worker2   = _hudi_cvbsrx_event_notifier;

    event->worker_key = work_notifier_setup(&work_queue);
    if (event->worker_key < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs event add error\n");
        return -1;
    }

    event->on = 1;
    return 0;
}

static int _hudi_cvbsrx_polling_stop(hudi_cvbsrx_instance_t *inst)
{
    inst->event_polling = 0;
    inst->polling_stop = 1;
    return 0;
}

static int _hudi_cvbsrx_event_del(hudi_cvbsrx_instance_t *inst, hudi_cvbsrx_event_t *event)
{
    work_notifier_teardown(event->worker_key);
    event->worker_key = 0;
    event->on = 0;

    return 0;
}


#else
#include <sys/epoll.h>
#include <hcuapi/kumsgq.h>
#include <hcuapi/common.h>

static void *_hudi_cvbsrx_polling_thread(void *arg)
{
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)arg;
    struct epoll_event events[10];
    KuMsgDH *kumsg = NULL;
    unsigned char msg[MAX_KUMSG_SIZE] = {0};
    hudi_cvbsrx_event_t *pev = NULL;
    int n = 0;

    while (!inst->polling_stop)
    {
        n = epoll_wait(inst->fd_epoll, events, 10, 0);
        if (n <= 0)
        {
            usleep(100 * 1000);
            continue;
        }

        for (int i = 0; i < n; i ++)
        {
            if (read(inst->fd_kumsg, msg, sizeof(msg)) > 0)
            {
                kumsg = (KuMsgDH *)msg;
                switch (kumsg->type)
                {
                    case TVDEC_NOTIFY_CONNECT:
                    case TVDEC_NOTIFY_DISCONNECT:
                    case TVDEC_NOTIFY_TRAINING_FINISH:
                        pev = (hudi_cvbsrx_event_t *)kumsg->user_args;
                        if (pev->on && pev->notifier)
                        {
                            pev->notifier(inst, pev->event_type, (void *)&kumsg->params, pev->user_data);
                        }

                        break;
                    default:
                        hudi_log(HUDI_LL_WARNING, "CVBS RX unhandle event %d\n", kumsg->type);
                        break;
                }
            }
        }
    }

    inst->polling_stop = 0;

    return NULL;

}

static int _hudi_cvbsrx_polling_start(hudi_cvbsrx_instance_t *inst)
{
    struct epoll_event ev;

    inst->fd_kumsg = ioctl(inst->fd, KUMSGQ_FD_ACCESS, O_CLOEXEC);
    if (inst->fd_kumsg < 0)
    {
        hudi_log(HUDI_LL_ERROR, "CVBS RX kumsgq access fail\n");
        return -1;
    }

    inst->fd_epoll = epoll_create1(0);

    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.ptr = (void *)inst;
    if (epoll_ctl(inst->fd_epoll, EPOLL_CTL_ADD, inst->fd_kumsg, &ev) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "CVBS RX epoll control add fail\n");
        close(inst->fd_epoll);
        close(inst->fd_kumsg);
        inst->fd_epoll = -1;
        inst->fd_kumsg = -1;
        return -1;
    }

    pthread_create(&inst->polling_tid, NULL, _hudi_cvbsrx_polling_thread, (void *)inst);

    inst->event_polling = 1;

    return 0;

}

static int _hudi_cvbsrx_event_add(hudi_cvbsrx_instance_t *inst, hudi_cvbsrx_event_t *event)
{
    int ret = -1;
    struct kumsg_event k_event = {0};

    k_event.evtype = event->event_type;
    k_event.arg = (unsigned long)event;
    ret = ioctl(inst->fd, KUMSGQ_NOTIFIER_SETUP, &k_event);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs event add error \n");
        return ret ;
    }

    event->on = 1;
    return ret;
}

static int _hudi_cvbsrx_polling_stop(hudi_cvbsrx_instance_t *inst)
{
    inst->polling_stop = 1;
    pthread_join(inst->polling_tid, NULL);
    inst->event_polling = 0;
    return 0;
}

static int _hudi_cvbsrx_event_del(hudi_cvbsrx_instance_t *inst, hudi_cvbsrx_event_t *event)
{
    if (!event->on)
    {
        return -1;
    }

    event->on = 0;

    return 0;
}

#endif


int hudi_cvbsrx_open(hudi_handle *handle)
{
    hudi_cvbsrx_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid cvbs rx parameters\n");
        return -1;
    }

    _hudi_cvbsrx_mutex_lock();

    inst = (hudi_cvbsrx_instance_t *)malloc(sizeof(hudi_cvbsrx_instance_t));
    memset(inst, 0, sizeof(hudi_cvbsrx_instance_t));

    inst->fd = open(HUDI_CVBS_RX_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open cvbs rx fail\n");
        free(inst);
        _hudi_cvbsrx_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    * handle = inst;

    _hudi_cvbsrx_mutex_unlock();

    return 0;
}

int hudi_cvbsrx_close(hudi_handle handle)
{
    int ret = -1;
    hudi_list_node_t *node = NULL;
    hudi_cvbsrx_event_t *event = NULL;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open cvbs rx\n");
        return ret;
    }


    _hudi_cvbsrx_mutex_lock();

    if (inst->event_polling)
    {
        _hudi_cvbsrx_polling_stop(inst);
    }

    for (int i = 0; i < inst->event_num; i++)
    {
        node = hudi_list_at(inst->event_list, i);
        event = (hudi_cvbsrx_event_t *)node->val;
        _hudi_cvbsrx_event_del(inst, event);
    }

    if (inst->event_list)
    {
        hudi_list_destroy(inst->event_list);
        inst->event_list = NULL;
    }

    close(inst->fd);
    memset(inst, 0, sizeof(hudi_cvbsrx_instance_t));
    free(inst);

    _hudi_cvbsrx_mutex_unlock();

    return 0;
}

int hudi_cvbsrx_event_register(hudi_handle handle, hudi_cvbsrx_event_cb func, int event_type, void *user_data)
{
    int ret = -1;
    hudi_list_node_t *node = NULL;
    hudi_cvbsrx_event_t *event = NULL;
    hudi_cvbsrx_event_t *new_event = NULL;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open error\n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    if (inst->event_list == NULL)
    {
        inst->event_list = hudi_list_new();
        inst->event_list->free = _hudi_cvbsrx_event_free;
    }

    if (!inst->event_polling)
    {
        _hudi_cvbsrx_polling_start(inst);
    }

    for (int i = 0; i < inst->event_list->len; i++)
    {
        node = hudi_list_at(inst->event_list, i);
        event = (hudi_cvbsrx_event_t *)node->val;

        if (event->event_type == event_type)
        {
            if (event->on)
            {
                _hudi_cvbsrx_mutex_unlock();
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
        new_event = calloc(sizeof(hudi_cvbsrx_event_t), 1);
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

    _hudi_cvbsrx_event_add(inst, new_event);
    inst->event_num ++;

    _hudi_cvbsrx_mutex_unlock();

    return 0;
}

int hudi_cvbsrx_event_unregister(hudi_handle handle, int event_type)
{
    int found = -1;
    hudi_list_node_t *node = NULL;
    hudi_cvbsrx_event_t *event = NULL;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open error\n");
        return -1;
    }

    _hudi_cvbsrx_mutex_lock();

    for (int i = 0; i < inst->event_list->len; i++)
    {
        node = hudi_list_at(inst->event_list, i);
        event = (hudi_cvbsrx_event_t *)node->val;

        if (event->on && event->event_type == event_type)
        {
            found = 1;
            break;
        }
    }

    if (found)
    {
        _hudi_cvbsrx_event_del(inst, event);
        inst->event_num --;
        event->notifier = NULL;
        event->handle = NULL;
        event->user_data = NULL;
    }

    if (inst->event_num == 0)
    {
        _hudi_cvbsrx_polling_stop(inst);
    }

    _hudi_cvbsrx_mutex_unlock();

    return 0;
}

int hudi_cvbsrx_start(hudi_handle handle, tvtype_e type)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_START, type);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_stop(hudi_handle handle)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_STOP);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_stop_mode_set(hudi_handle handle, int value)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_SET_VIDEO_STOP_MODE, value);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_data_path_set(hudi_handle handle, tvdec_video_data_path_e path)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_SET_VIDEO_DATA_PATH, path);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_rotate_mode_set(hudi_handle handle, rotate_type_e type)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_SET_VIDEO_ROTATE_MODE, type);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_mirror_mode_set(hudi_handle handle, mirror_type_e type)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_SET_VIDEO_MIRROR_MODE, type);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_info_get(hudi_handle handle, struct tvdec_video_info *info)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited || !info)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_GET_VIDEO_INFO, info);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_training_start(hudi_handle handle, tvtype_e tvtype)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_SET_TRAINING_START, tvtype);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_training_param_set(hudi_handle handle, struct tvdec_training_params *params)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited || !params)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_SET_TRAINING_PARAMS, params);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_training_result_get(hudi_handle handle, struct tvdec_training_result *res)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited || !res)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_GET_TRAINING_RESULT, res);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_training_dc_offset_set(hudi_handle handle, int value)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_SET_DC_OFFSET, value);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_brightness_set(hudi_handle handle, int value)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_SET_BRIGHTNESS, value);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_display_info_set(hudi_handle handle, struct tvdec_display_info *info)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited || !info)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_SET_DISPLAY_INFO, info);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_display_rect_set(hudi_handle handle, struct vdec_dis_rect *rect)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited || !rect)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_SET_DISPLAY_RECT, rect);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}

int hudi_cvbsrx_pbp_mode_set(hudi_handle handle, video_pbp_mode_e mode)
{
    int ret = -1;
    hudi_cvbsrx_instance_t *inst = (hudi_cvbsrx_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Hudi cvbs rx not open \n");
        return ret;
    }

    _hudi_cvbsrx_mutex_lock();

    ret = ioctl(inst->fd, TVDEC_SET_PBP_MODE, mode);

    _hudi_cvbsrx_mutex_unlock();

    return ret;
}
