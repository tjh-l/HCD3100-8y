
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <hccast_dlna.h>
#include <hccast_log.h>
#include <pthread.h>

#ifdef HC_RTOS
#include <dlna/dlna_api.h>
#else
#include <hccast/dlna_api.h>
#endif

extern struct output_module output_ffplayer;
hccast_dlna_event_callback dlna_callback = NULL;

static pthread_mutex_t g_dlna_svr_mutex = PTHREAD_MUTEX_INITIALIZER;

static int g_dlna_started = 0;

int hccast_dlna_service_is_start()
{
    return g_dlna_started;
}

int hccast_dlna_service_start()
{
    char service_name[DLNA_SERVICE_NAME_LEN] = "hccast_dlna";

    pthread_mutex_lock(&g_dlna_svr_mutex);
    if (g_dlna_started)
    {
        hccast_log(LL_WARNING,"[%s]: dlna service has been start\n", __func__);
        pthread_mutex_unlock(&g_dlna_svr_mutex);
        return 0;
    }

    hccast_dlna_param service_params = {0};
    struct dlna_svr_param svr_param = {0};

    if (dlna_callback)
    {
        dlna_callback (HCCAST_DLNA_GET_DEVICE_NAME, &service_name, NULL);
        dlna_callback (HCCAST_DLNA_GET_DEVICE_PARAM, &service_params, NULL);
    }

    svr_param.output  = service_params.output;
    svr_param.ifname  = service_params.ifname;
    svr_param.svrname = service_params.svrname;
    svr_param.svrport = service_params.svrport;

    if (!svr_param.output)
    {
        svr_param.output = &output_ffplayer;
    }

    if (!svr_param.ifname)
    {
        svr_param.ifname = DLNA_BIND_IFNAME;
    }

    if (!svr_param.svrname)
    {
        svr_param.svrname = service_name;
    }

    if (svr_param.svrport <= 0)
    {
        svr_param.svrport = DLNA_UPNP_PORT;
    }

    dlna_service_start_ex(&svr_param);
    g_dlna_started = 1;
    pthread_mutex_unlock(&g_dlna_svr_mutex);

    hccast_log(LL_NOTICE,"dlna service start\n");
    return 0;
}

int hccast_dlna_service_stop()
{
    pthread_mutex_lock(&g_dlna_svr_mutex);
    if (g_dlna_started == 0)
    {
        hccast_log(LL_WARNING,"[%s]: dlna service has been stop\n", __func__);
        pthread_mutex_unlock(&g_dlna_svr_mutex);
        return 0;
    }

    dlna_sevice_stop();
    g_dlna_started = 0;
    pthread_mutex_unlock(&g_dlna_svr_mutex);

    hccast_log(LL_NOTICE, "dlna service stop\n");
    return 0;
}

static void hccast_dlna_event_process(int event_type, void *param)
{
    switch (event_type)
    {
        case DLNA_EVENT_INVALID_CERT:
            dlna_callback(HCCAST_DLNA_INVALID_CERT, NULL, NULL);
            break;
        default:
            break;
    }
}

int hccast_dlna_service_init(hccast_dlna_event_callback func)
{
    hccast_log(LL_NOTICE, "%s\n", dlna_get_version());

    dlna_callback = func;

    dlna_service_init(hccast_dlna_event_process);
    
    return 0;
}

int hccast_dlna_service_uninit()
{
	dlna_callback = NULL;
	return 0;
}
