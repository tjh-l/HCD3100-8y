#include <stdbool.h>
#include <stdio.h>

#include <hccast_iptv.h>
#include <hccast_log.h>
#include <string.h>

#ifndef HC_RTOS
#define IPTV_YT_LIBRARY   "/usr/lib/libiptv-yt.so"
#include <dlfcn.h>
#endif

struct hccast_iptv_api_st
{
    bool initialized;
    char res[7];
    void *handle;
    int (*attach)(void);
    int (*detach)(void);
};

static struct hccast_iptv_api_st g_iptv_api[HCCAST_IPTV_APP_MAX] = {0};

#ifdef HC_RTOS
/* weak functions for external sdk without specified library */
int __attribute__((weak)) hccast_iptv_attach_yt(void)
{
    hccast_log(LL_NOTICE, "[YT] Weak\n");
    return -1;
}
#endif

int hccast_iptv_api_init(int app_id)
{
    int ret = 0;

    if (g_iptv_api[app_id].initialized)
    {
        ret = -1;
        goto EXIT;
    }

    memset(&g_iptv_api[app_id], 0, sizeof(g_iptv_api[app_id]));

    switch (app_id)
    {
        case HCCAST_IPTV_APP_YT:
        {
#ifdef HC_RTOS
            g_iptv_api[app_id].attach = hccast_iptv_attach_yt;
#else
            g_iptv_api[app_id].handle = dlopen(IPTV_YT_LIBRARY, RTLD_LAZY);
            if (g_iptv_api[app_id].handle)
            {
                g_iptv_api[app_id].attach = dlsym(g_iptv_api[app_id].handle, "hccast_iptv_attach_yt");
            }
            else
            {
                hccast_log(LL_NOTICE, "api: iptv[%d] dlopen %s failed\n", app_id, IPTV_YT_LIBRARY);
            }
#endif
            break;
        }

        case HCCAST_IPTV_APP_YP:
        case HCCAST_IPTV_APP_MAX:
        default:
            break;

    }

    g_iptv_api[app_id].initialized = true;

EXIT:

    return ret;
}

int hccast_iptv_api_deinit(int app_id)
{
    if (g_iptv_api[app_id].initialized)
    {
        switch (app_id)
        {
            case HCCAST_IPTV_APP_YT:
            {
#ifndef HC_RTOS
                if (g_iptv_api[app_id].handle)
                {
                    dlclose(g_iptv_api[app_id].handle);
                }
#endif
                break;
            }

            case HCCAST_IPTV_APP_YP:
            case HCCAST_IPTV_APP_MAX:
            default:
                break;

        }

        memset(&g_iptv_api[app_id], 0, sizeof(g_iptv_api[app_id]));
    }

    return 0;
}

int hccast_iptv_api_attach(int app_id)
{
    int ret = -1;

    if (g_iptv_api[app_id].initialized && g_iptv_api[app_id].attach)
        ret = g_iptv_api[app_id].attach();
    else
        hccast_log(LL_NOTICE, "api: iptv[%d] attach is null!\n", app_id);

    return ret;
}

int hccast_iptv_api_detach(int app_id)
{
    int ret = -1;

    if (g_iptv_api[app_id].initialized && g_iptv_api[app_id].detach)
        ret = g_iptv_api[app_id].detach();
    else
        hccast_log(LL_NOTICE, "api: iptv[%d] detach is null!\n", app_id);

    return ret;
}