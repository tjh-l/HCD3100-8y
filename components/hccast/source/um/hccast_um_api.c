#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#ifdef HC_RTOS
#include <um/iumirror_api.h>
#include <um/aumirror_api.h>
#else
#include <dlfcn.h>
#include <hccast/iumirror_api.h>
#include <hccast/aumirror_api.h>
#endif
#include <hccast_um.h>
#include <hccast_log.h>
#include "hccast_um_api.h"

#define USBMIRROR_LIBRARY   "/usr/lib/libusbmirror.so"

static int g_um_inited = 0;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static void *g_dl_handle = NULL;
static hccast_um_param_t g_um_param = {0, 0, 0};

um_ioctl aum_api_ioctl = NULL;
um_ioctl aum_api_ioctl_slave = NULL;
um_api aum_api_start = NULL;
um_api aum_api_stop = NULL;

um_ioctl ium_api_ioctl = NULL;
um_ioctl ium_api_ioctl_slave = NULL;
um_api ium_api_start = NULL;
um_api ium_api_stop = NULL;

#ifdef HC_RTOS
static void um_start_dummy()
{
}

static void um_stop_dummy()
{
}

#ifdef SUPPORT_UM_SLAVE
void setup_iap_function(void *c)
{
}
#endif

int hccast_um_init()
{
    aum_api_ioctl = aum_ioctl;
    ium_api_ioctl = ium_ioctl;

#ifdef SUPPORT_UM_SLAVE
    ium_api_ioctl_slave = ium_ioctl_slave;
    aum_api_ioctl_slave = aum_ioctl_slave;
    aum_api_start = um_start_dummy;
    aum_api_stop = um_stop_dummy;
    ium_api_start = um_start_dummy;
    ium_api_stop = um_stop_dummy;
#else
    aum_api_start = aum_start;
    aum_api_stop = aum_stop;
    ium_api_start = ium_start;
    ium_api_stop = ium_stop;
#endif

    return 0;
}

int hccast_um_deinit()
{
    return 0;
}
#else
int hccast_um_init()
{
    pthread_mutex_lock(&g_mutex);

    if (g_um_inited)
    {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }

    g_dl_handle = dlopen(USBMIRROR_LIBRARY, RTLD_LAZY);
    if (!g_dl_handle)
    {
        hccast_log(LL_ERROR, "dlopen %s\n", dlerror());
        pthread_mutex_unlock(&g_mutex);
        return -1;

    }

    aum_api_ioctl = dlsym(g_dl_handle, "aum_ioctl");
    if (!aum_api_ioctl)
    {
        hccast_log(LL_ERROR, "dlsym aum_ioctl %s\n", dlerror());
    }

    aum_api_start = dlsym(g_dl_handle, "aum_start");
    if (!aum_api_start)
    {
        hccast_log(LL_ERROR, "dlsym aum_start %s\n", dlerror());
    }

    aum_api_stop = dlsym(g_dl_handle, "aum_stop");
    if (!aum_api_stop)
    {
        hccast_log(LL_ERROR, "dlsym aum_stop %s\n", dlerror());
    }

    ium_api_ioctl = dlsym(g_dl_handle, "ium_ioctl");
    if (!aum_api_ioctl)
    {
        hccast_log(LL_ERROR, "dlsym ium_ioctl %s\n", dlerror());
    }

    ium_api_start = dlsym(g_dl_handle, "ium_start");
    if (!ium_api_start)
    {
        hccast_log(LL_ERROR, "dlsym ium_start %s\n", dlerror());
    }

    ium_api_stop = dlsym(g_dl_handle, "ium_stop");
    if (!ium_api_stop)
    {
        hccast_log(LL_ERROR, "dlsym ium_close %s\n", dlerror());
    }

    g_um_inited = 1;

    pthread_mutex_unlock(&g_mutex);

    return 0;
}

int hccast_um_deinit()
{
    pthread_mutex_lock(&g_mutex);

    if (g_um_inited && g_dl_handle)
    {
        dlclose(g_dl_handle);
        g_dl_handle = NULL;
    }

    g_um_inited = 0;

    pthread_mutex_unlock(&g_mutex);

    return 0;
}
#endif

int hccast_um_param_get(hccast_um_param_t *param)
{
    if (!param)
    {
        return -1;
    }

    memcpy(param, &g_um_param, sizeof(hccast_um_param_t));

    return 0;
}

int hccast_um_param_set(hccast_um_param_t *param)
{
    if (!param)
    {
        return -1;
    }

    memcpy(&g_um_param, param, sizeof(hccast_um_param_t));

    return 0;
}

hccast_um_param_t *hccast_um_inter_param_get()
{
    return &g_um_param;
}

#ifdef HC_RTOS
void __attribute__((weak)) ium_ioctl(int cmd, void *param1, void *param2)
{
    if(cmd == IUM_CMD_SET_EVENT_CB)
    {
        if (param1)
        {
            ium_evt_cb ium_fake_cb = param1;
            ium_fake_cb(IUM_EVT_FAKE_LIB, NULL, NULL);
        }    
    }

    printf("[WEAK] %s\n", __func__);
}

void __attribute__((weak)) ium_start(void)
{
    printf("[WEAK] %s\n", __func__);
}

void __attribute__((weak)) ium_stop(void)
{
    printf("[WEAK] %s\n", __func__);
}

void __attribute__((weak)) aum_ioctl(int cmd, void *param1, void *param2)
{
    printf("[WEAK] %s\n", __func__);
}

void __attribute__((weak)) aum_start(void)
{
    printf("[WEAK] %s\n", __func__);
}

void __attribute__((weak)) aum_stop(void)
{
    printf("[WEAK] %s\n", __func__);
}
#endif
