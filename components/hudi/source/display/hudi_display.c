/**
* @file
* @brief                hudi display engine interface
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
#include <hudi_display.h>

#include "hudi_display_inter.h"

static pthread_mutex_t g_hudi_display_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_display_mutex_lock()
{
    pthread_mutex_lock(&g_hudi_display_mutex);

    return 0;
}

static int _hudi_display_mutex_unlock()
{
    pthread_mutex_unlock(&g_hudi_display_mutex);

    return 0;
}

static void _hudi_display_event_free(void *val)
{
    hudi_display_event_t *event = (hudi_display_event_t *)val;

    if (!event)
    {
        return ;
    }

    free(event);
}

#ifdef __HCRTOS__
#include <nuttx/wqueue.h>

static void _hudi_display_event_notifier(void *arg, unsigned long param)
{
    hudi_display_event_t *event = (hudi_display_event_t *)arg;

    if (event->on && event->notifier)
    {
        event->notifier(event->handle, event->event_type, (void *)param, event->user_data);
    }
}

static int _hudi_display_polling_start(hudi_display_instance_t *inst)
{
    inst->event_polling = 1;

    return 0;
}

static int _hudi_display_polling_stop(hudi_display_instance_t *inst)
{
    inst->event_polling = 0;

    return 0;
}

static int _hudi_display_event_add(hudi_display_instance_t *inst, hudi_display_event_t *event, int event_type)
{
    struct work_notifier_s notifier = {0};

    notifier.evtype = event_type;
    notifier.qid = HPWORK;
    notifier.remote = 0;
    notifier.oneshot = 0;
    notifier.qualifier = NULL;
    notifier.arg = event;
    notifier.worker2 = _hudi_display_event_notifier;

    event->worker_key = work_notifier_setup(&notifier);
    if (event->worker_key < 0)
    {
        hudi_log(HUDI_LL_WARNING, "Display notifier(%d) setup fail\n", notifier.evtype);
        return -1;
    }

    event->on = 1;

    return 0;
}

static int _hudi_display_event_delete(hudi_display_instance_t *inst, hudi_display_event_t *event)
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

static void *_hudi_display_polling_thread(void *arg)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)arg;
    struct epoll_event events[10];
    KuMsgDH kumsg;
    hudi_display_event_t *pev = NULL;
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
                    case DIS_NOTIFY_VBLANK:
                    case DIS_NOTIFY_MIRACAST_VSRCEEN:
                    case DIS_NOTIFY_DE_TYPE_CHANGE:
                        pev = (hudi_display_event_t *)kumsg.user_args;
                        if (pev->on && pev->notifier)
                        {
                            pev->notifier(inst, pev->event_type, (void *)kumsg.params, pev->user_data);
                        }
                        break;
                    default:
                        hudi_log(HUDI_LL_WARNING, "Display unhandle event %d\n", kumsg.type);
                        break;
                }
            }
        }
    }

    inst->polling_stop = 0;

    return NULL;
}

static int _hudi_display_polling_start(hudi_display_instance_t *inst)
{
    struct epoll_event ev;

    inst->fd_kumsg = ioctl(inst->fd, KUMSGQ_FD_ACCESS, O_CLOEXEC);
    if (inst->fd_kumsg < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Display kumsgq access fail\n");
        return -1;
    }

    inst->fd_epoll = epoll_create1(0);

    memset(&ev, 0, sizeof(struct epoll_event));
    ev.events = EPOLLIN;
    ev.data.ptr = (void *)inst;
    if (epoll_ctl(inst->fd_epoll, EPOLL_CTL_ADD, inst->fd_kumsg, &ev) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "Display epoll control add fail\n");
        close(inst->fd_epoll);
        close(inst->fd_kumsg);
        inst->fd_epoll = -1;
        inst->fd_kumsg = -1;
        return -1;
    }

    pthread_create(&inst->polling_tid, NULL, _hudi_display_polling_thread, (void *)inst);

    inst->event_polling = 1;

    return 0;
}

static int _hudi_display_polling_stop(hudi_display_instance_t *inst)
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

static int _hudi_display_event_add(hudi_display_instance_t *inst, hudi_display_event_t *event, int event_type)
{
    struct kumsg_event ev = {0};

    ev.evtype = event_type;
    ev.arg = (unsigned long)event;
    if (ioctl(inst->fd, KUMSGQ_NOTIFIER_SETUP, &ev) != 0)
    {
        hudi_log(HUDI_LL_ERROR, "Display event add fail\n");
        return -1;
    }

    event->on = 1;

    return 0;
}

static int _hudi_display_event_delete(hudi_display_instance_t *inst, hudi_display_event_t *event)
{
    if (!event->on)
    {
        return -1;
    }

    event->on = 0;

    return 0;
}
#endif

int hudi_display_open(hudi_handle *handle)
{
    hudi_display_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid display parameters\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    inst = (hudi_display_instance_t *)malloc(sizeof(hudi_display_instance_t));
    memset(inst, 0, sizeof(hudi_display_instance_t));
    inst->fd_epoll = -1;
    inst->fd_kumsg = -1;
    inst->event_list = NULL;

    inst->fd = open(HUDI_DISPLAY_DEV, O_RDWR);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Open display fail\n");
        free(inst);
        _hudi_display_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_display_mutex_unlock();

    return 0;
}

int hudi_display_close(hudi_handle handle)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    hudi_list_node_t *node = NULL;
    hudi_display_event_t *event = NULL;
    int i;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    if (inst->event_num > 0)
    {
        for (i = 0; i < inst->event_list->len; i ++)
        {
            node = hudi_list_at(inst->event_list, i);
            event = (hudi_display_event_t *)node->val;
            if (event->on)
            {
                _hudi_display_event_delete(inst, event);
                inst->event_num --;
            }
        }
    }    

    if (inst->event_list)
    {
        hudi_list_destroy(inst->event_list);
        inst->event_list = NULL;
    }

    if (inst->event_polling)
    {
        _hudi_display_polling_stop(inst);
    }

    close(inst->fd);
    memset(inst, 0, sizeof(hudi_display_instance_t));
    free(inst);

    _hudi_display_mutex_unlock();

    return 0;
}

int hudi_display_aspect_set(hudi_handle handle, dis_aspect_mode_t *aspect_args)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    dis_aspect_mode_t aspect = {0};
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    ret = ioctl(inst->fd, DIS_SET_ASPECT_MODE, aspect_args);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_zoom_set(hudi_handle handle, dis_zoom_t *zoom_args)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    ioctl(inst->fd, DIS_SET_ZOOM, zoom_args);

    _hudi_display_mutex_unlock();

    return 0;
}

int hudi_display_pic_backup(hudi_handle handle, dis_type_e dis_type)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    ret = ioctl(inst->fd, DIS_BACKUP_MP, dis_type);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_pic_free(hudi_handle handle, dis_type_e dis_type)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    ret = ioctl(inst->fd, DIS_FREE_BACKUP_MP, dis_type);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_event_register(hudi_handle handle, hudi_display_cb func, int event_type, void *user_data)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    hudi_list_node_t *node = NULL;
    hudi_display_event_t *event = NULL;
    hudi_display_event_t *new_event = NULL;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    if (inst->event_list == NULL)
    {
        inst->event_list = hudi_list_new();
        inst->event_list->free = _hudi_display_event_free;
    }  

    if (!inst->event_polling)
    {
        _hudi_display_polling_start(inst);
    }

    for (int i = 0; i < inst->event_list->len; i++)
    {
        node = hudi_list_at(inst->event_list, i);
        event = (hudi_display_event_t *)node->val;
        
        if (event->event_type == event_type)
        {
            if (event->on)
            {
                _hudi_display_mutex_unlock();
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
        new_event = calloc(sizeof(hudi_display_event_t), 1);
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

    _hudi_display_event_add(inst, new_event, event_type);
    inst->event_num ++;

    _hudi_display_mutex_unlock();

    return 0;
}

int hudi_display_event_unregister(hudi_handle handle, int event_type)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    hudi_list_node_t *node = NULL;
    hudi_display_event_t *event = NULL;
    int found = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    for (int i = 0; i < inst->event_list->len; i++)
    {
        node = hudi_list_at(inst->event_list, i);
        event = (hudi_display_event_t *)node->val;

        if (event->on && event->event_type == event_type)
        {
            found = 1;
            break;
        }
    }

    if (found)
    {
        _hudi_display_event_delete(inst, event);
        inst->event_num --;
        event->handle = NULL;
        event->notifier = NULL;
        event->user_data = NULL;
    }

    if (!inst->event_num)
    {
        _hudi_display_polling_stop(inst);
    }

    _hudi_display_mutex_unlock();

    return 0;
}

int hudi_display_screen_info_get(hudi_handle handle, dis_type_e distype, dis_area_t *screen_info)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    dis_screen_info_t dis_screen_info = { 0 };
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    if (!screen_info)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid display parameters\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    dis_screen_info.distype = distype;
    ret = ioctl(inst->fd, DIS_GET_SCREEN_INFO, &dis_screen_info);

    screen_info->x = dis_screen_info.area.x;
    screen_info->y = dis_screen_info.area.y;
    screen_info->w = dis_screen_info.area.w;
    screen_info->h = dis_screen_info.area.h;

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_area_info_get(hudi_handle handle, dis_type_e distype, dis_area_t *area_info)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    dis_screen_info_t screen_info = { 0 };
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    if (!area_info)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid display parameters\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    screen_info.distype = distype;
    ret = ioctl(inst->fd, DIS_GET_MP_AREA_INFO, &screen_info);

    area_info->x = screen_info.area.x;
    area_info->y = screen_info.area.y;
    area_info->w = screen_info.area.w;
    area_info->h = screen_info.area.h;

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_picture_area_get(hudi_handle handle, dis_type_e distype, dis_area_t *picture_area)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    dis_screen_info_t picture_info = { 0 };
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    if (!picture_area)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid display parameters\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    picture_info.distype = distype;
    ret = ioctl(inst->fd, DIS_GET_MIRACAST_PICTURE_ARER, &picture_info);

    picture_area->x = picture_info.area.x;
    picture_area->y = picture_info.area.y;
    picture_area->w = picture_info.area.w;
    picture_area->h = picture_info.area.h;

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_info_get(hudi_handle handle, dis_type_e distype, dis_layer_e layer, dis_display_info_t *dis_info)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    if (!dis_info)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid display parameters\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    dis_info->distype = distype;
    dis_info->info.layer = layer;
    ret = ioctl(inst->fd, DIS_GET_DISPLAY_INFO, dis_info);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_dynamic_enhance_set(hudi_handle handle, dis_type_e distype, unsigned int on)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    struct dis_dyn_enh_onoff dyn_enh = {0};
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    dyn_enh.distype = distype;
    dyn_enh.onoff = on;

    ret = ioctl(inst->fd, DIS_DENH_SET_ONOFF, &dyn_enh);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_layer_order_set(hudi_handle handle, dis_layer_blend_order_t *layer_args)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    ret = ioctl(inst->fd, DIS_SET_LAYER_ORDER, layer_args);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_enhance_set(hudi_handle handle, dis_video_enhance_t *enhance_args)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    ret = ioctl(inst->fd, DIS_SET_VIDEO_ENHANCE, enhance_args);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_vscreen_detect_set(hudi_handle handle, dis_type_e distype, dis_layer_e layer, unsigned char on)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    dis_miracast_vscreen_detect_param_t vscreen_detect = {0};

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    vscreen_detect.distype = distype;
    vscreen_detect.layer = layer;
    vscreen_detect.on = on;

    ioctl(inst->fd, DIS_SET_MIRACAST_VSRCEEN_DETECT, &vscreen_detect);

    _hudi_display_mutex_unlock();

    return 0;
}

int hudi_display_suspend(hudi_handle handle, dis_type_e distype)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    dis_suspend_resume_t suspend_param = {0};
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    suspend_param.distype = distype;

    ret = ioctl(inst->fd, DIS_SET_SUSPEND, &suspend_param);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_onoff_set(hudi_handle handle, dis_type_e distype, dis_layer_e layer, unsigned int on)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    dis_win_onoff_t onoff_param = {0};
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    onoff_param.distype = distype;
    onoff_param.layer = layer;
    onoff_param.on = on;

    ret = ioctl(inst->fd, DIS_SET_WIN_ONOFF, &onoff_param);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_keystone_get(hudi_handle handle, dis_type_e distype, dis_keystone_param_t *keystone_info)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    if (!keystone_info)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid display parameters\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    keystone_info->distype = distype;
    ret = ioctl(inst->fd, DIS_GET_KEYSTONE_PARAM, keystone_info);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_keystone_set(hudi_handle handle, dis_type_e distype, dis_keystone_param_t *keystone_args)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    keystone_args->distype = distype;
    ret = ioctl(inst->fd, DIS_SET_KEYSTONE_PARAM, keystone_args);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_tvsys_get(hudi_handle handle, dis_tvsys_t *tvsys_info)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    if (!tvsys_info)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid display parameters\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    ret = ioctl(inst->fd, DIS_GET_TVSYS, tvsys_info);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_tvsys_set(hudi_handle handle, dis_tvsys_t *tvsys_args)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    ret = ioctl(inst->fd, DIS_SET_TVSYS, tvsys_args);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_dac_register(hudi_handle handle, dis_dac_param_t *dac_args)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    ret = ioctl(inst->fd, DIS_REGISTER_DAC, dac_args);

    _hudi_display_mutex_unlock();

    return ret;
}

int hudi_display_dac_unregister(hudi_handle handle, dis_dac_param_t *dac_args)
{
    hudi_display_instance_t *inst = (hudi_display_instance_t *)handle;
    int ret = 0;

    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "Display not open\n");
        return -1;
    }

    _hudi_display_mutex_lock();

    ret = ioctl(inst->fd, DIS_UNREGISTER_DAC, dac_args);

    _hudi_display_mutex_unlock();

    return ret;
}
