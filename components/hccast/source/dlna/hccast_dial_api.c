#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#include <hccast_log.h>
#include <hccast_dial_api.h>

#define DIAL_LIBRARY   "/usr/lib/libdial.so"

#ifdef HC_RTOS
#else
#include <dlfcn.h>
#endif

struct dial_api_st
{
    bool inited;
    char res[7];
    void *handle;
    dial_api_service_init init;
    dial_api_service_start start;
    dial_api_service_stop stop;
    dial_api_get_version get_version;
    dial_api_set_log_level set_log_level;
    dial_api_get_log_level get_log_level;
};

struct dial_api_st g_dial_api = {0};
static pthread_mutex_t g_dial_api_mutex = PTHREAD_MUTEX_INITIALIZER;

int hccast_dial_api_init()
{
    int ret = -1;
    pthread_mutex_lock(&g_dial_api_mutex);
    if(g_dial_api.inited)
    {
        goto EXIT;
    }

    memset(&g_dial_api, 0, sizeof(g_dial_api));

#ifdef HC_RTOS
    g_dial_api.init = dial_service_init;
    g_dial_api.start = dial_service_start;
    g_dial_api.stop = dial_service_stop;
    g_dial_api.get_version = dial_get_version;
    g_dial_api.set_log_level = dial_set_log_level;
    //g_dial_api_st.get_log_level = dial_get_log_level;
#else
    g_dial_api.handle = dlopen(DIAL_LIBRARY, RTLD_LAZY);
    if (!g_dial_api.handle)
    {
        hccast_log(LL_ERROR, "dlopen %s failed (%s)!\n", DIAL_LIBRARY, dlerror());
        goto EXIT;

    }

    g_dial_api.init = dlsym(g_dial_api.handle, "dial_service_init");
    g_dial_api.start = dlsym(g_dial_api.handle, "dial_service_start");
    g_dial_api.stop = dlsym(g_dial_api.handle, "dial_service_stop");
    g_dial_api.get_version = dlsym(g_dial_api.handle, "dial_get_version");
    g_dial_api.set_log_level = dlsym(g_dial_api.handle, "dial_set_log_level");
    //g_dial_api_st.get_log_level = dlsym(g_dial_api.handle, "aircast_get_log_level");
#endif

    g_dial_api.inited = true;
    ret = 0;

EXIT:
    pthread_mutex_unlock(&g_dial_api_mutex);
    return ret;
}

int hccast_dial_api_deinit()
{
    pthread_mutex_lock(&g_dial_api_mutex);
    if(g_dial_api.inited)
    {
#ifndef HC_RTOS
        if (g_dial_api.handle)
        {
            dlclose(g_dial_api.handle);
        }
#endif
    }

    memset(&g_dial_api, 0, sizeof(g_dial_api));

    pthread_mutex_unlock(&g_dial_api_mutex);
    return 0;
}

int hccast_dial_api_service_init(dial_fn_event fn)
{
    if (g_dial_api.inited && g_dial_api.init)
    {
        return g_dial_api.init(fn);
    }
    else
    {
        hccast_log(LL_WARNING, "api: dial init is NULL\n");
        return -1;
    }
}

int hccast_dial_api_service_start(struct dial_svr_param *param)
{
    if (g_dial_api.inited && g_dial_api.start)
    {
        return g_dial_api.start(param);
    }
    else
    {
        hccast_log(LL_WARNING, "api: dial start is NULL\n");
        return -1;
    }
}

int hccast_dial_api_service_stop(void)
{
    if (g_dial_api.inited && g_dial_api.stop)
    {
        return g_dial_api.stop();
    }
    else
    {
        hccast_log(LL_WARNING, "api: dial stop is NULL\n");
        return -1;
    }
}

char *hccast_dial_api_service_get_version(void)
{
    if (g_dial_api.inited && g_dial_api.get_version)
    {
        return g_dial_api.get_version();
    }
    else
    {
        hccast_log(LL_WARNING, "api: dial get version is NULL\n");
        return "";
    }
}

int hccast_dial_api_service_set_log_level(int level)
{
    if (g_dial_api.inited && g_dial_api.get_version)
    {
        return g_dial_api.set_log_level(level);
    }
    else
    {
        hccast_log(LL_WARNING, "ali: dial set_log_level is NULL\n");
        return -1;
    }
}