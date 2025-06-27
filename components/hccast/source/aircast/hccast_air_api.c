#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#ifdef HC_RTOS
#include <aircast/aircast_api.h>
#include <aircast/aircast_mdns.h>
#include <aircast/aircast_urlplay.h>
#else
#include <hccast/aircast_api.h>
#include <hccast/aircast_mdns.h>
#include <hccast/aircast_urlplay.h>
#include <dlfcn.h>
#endif
#include <hccast_log.h>
#include "hccast_air_api.h"

#define AIRCAST_LIBRARY   "/usr/lib/libaircast.so"

static air_api_service_init g_air_api_service_init = NULL;
static air_api_service_start g_air_api_service_start = NULL;
static air_api_service_stop g_air_api_service_stop = NULL;
static air_api_set_event_callback g_air_api_set_event_callback = NULL;
static air_api_set_resolution g_air_api_set_resolution = NULL;
static air_api_event_notify g_air_api_event_notify = NULL;
static air_api_ioctl g_air_api_ioctl = NULL;
static air_p2p_start g_air_api_p2p_start = NULL;
static air_p2p_stop g_air_api_p2p_stop = NULL;
static air_p2p_set_channel g_air_api_p2p_set_channel = NULL;
static air_p2p_service_init g_air_api_p2p_service_init = NULL;
static int g_air_api_inited = 0;
static pthread_mutex_t g_air_api_mutex = PTHREAD_MUTEX_INITIALIZER;

int hccast_air_api_service_init()
{
    if (g_air_api_service_init)
    {
        return g_air_api_service_init();
    }
    else
    {
        hccast_log(LL_WARNING, "error: air_api_service_init is NULL\n");
        return -1;
    }
}

int hccast_air_api_service_start(char *name, char* ifname)
{
    if (g_air_api_service_start)
    {
        return g_air_api_service_start(name, ifname);
    }
    else
    {
        hccast_log(LL_WARNING, "error: air_api_service_start is NULL\n");
        return -1;
    }
}

int hccast_air_api_service_stop(void)
{
    if (g_air_api_service_stop)
    {
        return g_air_api_service_stop();
    }
    else
    {
        hccast_log(LL_WARNING, "error: air_api_service_stop is NULL\n");
        return -1;
    }
}

int hccast_air_api_set_notifier(evt_cb event_cb)
{
    if (g_air_api_set_event_callback)
    {
        g_air_api_set_event_callback(event_cb);
    }
    else
    {
        hccast_log(LL_WARNING, "error: air_api_set_event_callback is NULL\n");
    }

    return 0;
}

int hccast_air_api_set_resolution(int width, int height, int fps)
{
    if (g_air_api_set_resolution)
    {
        return g_air_api_set_resolution(width, height, fps);
    }
    else
    {
        hccast_log(LL_WARNING, "error: air_api_set_resolution is NULL\n");
        return -1;
    }
}

int hccast_air_api_event_notify(int event_type, void *param)
{
    if (g_air_api_event_notify)
    {
        g_air_api_event_notify(event_type, param);
    }
    else
    {
        hccast_log(LL_WARNING, "error: air_api_event_notify is NULL\n");
    }

    return 0;
}

int hccast_air_api_ioctl(int req_cmd, void *param1, void *param2)
{
    if (g_air_api_ioctl)
    {
        return g_air_api_ioctl(req_cmd, param1, param2);
    }
    else
    {
        hccast_log(LL_WARNING, "error: air_api_ioctl is NULL\n");
        return -1;
    }
}

int hccast_air_api_p2p_start(char *if_name, int ch)
{
    if (g_air_api_p2p_start)
    {
        return g_air_api_p2p_start(if_name, ch);
    }
    else
    {
        hccast_log(LL_WARNING, "error: hccast_air_api_p2p_start is NULL\n");
        return -1;
    }
}

int hccast_air_api_p2p_stop(void)
{
    if (g_air_api_p2p_stop)
    {
        return g_air_api_p2p_stop();
    }
    else
    {
        hccast_log(LL_WARNING, "error: hccast_air_api_p2p_stop is NULL\n");
        return -1;
    }
}

int hccast_air_api_p2p_set_channel(int channel)
{
    if (g_air_api_p2p_set_channel)
    {
        return g_air_api_p2p_set_channel(channel);
    }
    else
    {
        hccast_log(LL_WARNING, "error: hccast_air_api_p2p_set_channel is NULL\n");
        return -1;
    }
}

int hccast_air_api_p2p_service_init(void)
{
    if (g_air_api_p2p_service_init)
    {
        return g_air_api_p2p_service_init();
    }
    else
    {
        hccast_log(LL_WARNING, "error: %s is NULL\n", __func__);
        return -1;
    }
}

int hccast_air_api_init()
{
    void *air_dl_handle = NULL;

    pthread_mutex_lock(&g_air_api_mutex);
    if (g_air_api_inited)
    {
        pthread_mutex_unlock(&g_air_api_mutex);
        return 0;
    }

#ifdef HC_RTOS

    g_air_api_service_init = aircast_service_init;
    g_air_api_service_start = aircast_service_start;
    g_air_api_service_stop = aircast_service_stop;
    g_air_api_set_event_callback = aircast_set_event_callback;
    g_air_api_set_resolution = aircast_set_resolution;
    g_air_api_event_notify = aircast_event_notify;
    g_air_api_ioctl = aircast_ioctl;

#ifdef SUPPORT_AIRP2P
    g_air_api_p2p_start = aircast_p2p_start;
    g_air_api_p2p_stop = aircast_p2p_stop;
    g_air_api_p2p_set_channel = aircast_p2p_set_channel;
    g_air_api_p2p_service_init = aircast_p2p_service_init;
#endif

#else

    air_dl_handle = dlopen(AIRCAST_LIBRARY, RTLD_LAZY);
    if (!air_dl_handle)
    {
        hccast_log(LL_ERROR, "%s:dlopen %s\n", __func__, dlerror());
        pthread_mutex_unlock(&g_air_api_mutex);
        return -1;

    }

    g_air_api_service_init = dlsym(air_dl_handle, "aircast_service_init");
    if (!g_air_api_service_init)
    {
        hccast_log(LL_ERROR, "dlsym aircast_service_init %s\n", dlerror());
    }

    g_air_api_service_start = dlsym(air_dl_handle, "aircast_service_start");
    if (!g_air_api_service_start)
    {
        hccast_log(LL_ERROR, "dlsym aircast_service_start %s\n", dlerror());
    }

    g_air_api_service_stop = dlsym(air_dl_handle, "aircast_service_stop");
    if (!g_air_api_service_stop)
    {
        hccast_log(LL_ERROR, "dlsym aircast_service_stop %s\n", dlerror());
    }

    g_air_api_set_event_callback = dlsym(air_dl_handle, "aircast_set_event_callback");
    if (!g_air_api_set_event_callback)
    {
        hccast_log(LL_ERROR, "dlsym aircast_set_event_callback %s\n", dlerror());
    }

    g_air_api_set_resolution = dlsym(air_dl_handle, "aircast_set_resolution");
    if (!g_air_api_set_resolution)
    {
        hccast_log(LL_ERROR, "dlsym aircast_set_resolution %s\n", dlerror());
    }

    g_air_api_event_notify = dlsym(air_dl_handle, "aircast_event_notify");
    if (!g_air_api_event_notify)
    {
        hccast_log(LL_ERROR, "dlsym aircast_event_notify %s\n", dlerror());
    }

    g_air_api_ioctl = dlsym(air_dl_handle, "aircast_ioctl");
    if (!g_air_api_ioctl)
    {
        hccast_log(LL_ERROR, "dlsym aircast_ioctl %s\n", dlerror());
    }
    
#ifdef SUPPORT_AIRP2P
    g_air_api_p2p_start = dlsym(air_dl_handle, "aircast_p2p_start");
    if (!g_air_api_p2p_start)
    {
        hccast_log(LL_ERROR, "dlsym aircast_p2p_start %s\n", dlerror());
    }

    g_air_api_p2p_stop = dlsym(air_dl_handle, "aircast_p2p_stop");
    if (!g_air_api_p2p_stop)
    {
        hccast_log(LL_ERROR, "dlsym aircast_p2p_stop %s\n", dlerror());
    }   
    
    g_air_api_p2p_set_channel = dlsym(air_dl_handle, "aircast_p2p_set_channel");
    if (!g_air_api_p2p_set_channel)
    {
        hccast_log(LL_ERROR, "dlsym g_air_api_p2p_set_channel %s\n", dlerror());
    }
    
    g_air_api_p2p_service_init = dlsym(air_dl_handle, "aircast_p2p_service_init");
    if (!g_air_api_p2p_service_init)
    {
        hccast_log(LL_ERROR, "dlsym g_air_api_p2p_service_init %s\n", dlerror());
    }  
#endif
#endif
    g_air_api_inited = 1;
    pthread_mutex_unlock(&g_air_api_mutex);
    return 0;
}

#ifdef HC_RTOS
int __attribute__((weak)) aircast_service_init(void)
{
    printf("[WEAK] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) aircast_service_start(char *name, char *ifname)
{
    printf("[WEAK] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) aircast_service_stop(void)
{
    printf("[WEAK] %s\n", __func__);
    return 0;
}
void __attribute__((weak)) aircast_set_event_callback(evt_cb event_cb)
{
    evt_cb air_fake_ecb = NULL;
    
    if (event_cb)
    {
        air_fake_ecb = event_cb;
        air_fake_ecb(AIRCAST_FAKE_LIB, 0);
    }

    printf("[WEAK] %s\n", __func__);
}

int __attribute__((weak)) aircast_set_resolution(int width, int height, int refreshRate)
{
    printf("[WEAK] %s\n", __func__);
    return 0;
}

void __attribute__((weak)) aircast_event_notify(int event_type, void *param)
{
    printf("[WEAK] %s\n", __func__);
}

int __attribute__((weak)) aircast_ioctl(int req_cmd, void *param1, void *param2)
{
    printf("[WEAK] %s\n", __func__);
    return 0;
}

void __attribute__((weak)) aircast_log_level_set(int level)
{
    printf("[WEAK] %s\n", __func__);
}

int __attribute__((weak)) aircast_p2p_start(char *intf_name, int chan_num)
{
    printf("[WEAK] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) aircast_p2p_stop(void)
{
    printf("[WEAK] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) aircast_p2p_set_channel(int channel)
{
    printf("[WEAK] %s\n", __func__);
    return 0;
}

int __attribute__((weak)) aircast_p2p_service_init(void)
{
    printf("[WEAK] %s\n", __func__);
    return 0;
}
#endif
