#ifndef __HCCAST_DIAL_API_H__
#define __HCCAST_DIAL_API_H__

#ifdef HC_RTOS
#include <dlna/dial_api.h>
#else
#include <hccast/dial_api.h>
#endif


typedef int (*dial_api_service_init)(dial_fn_event func);
typedef int (*dial_api_service_start)(struct dial_svr_param *param);
typedef int (*dial_api_service_stop)(void);
typedef char *(*dial_api_get_version)(void);
typedef int (*dial_api_set_log_level)(int level);
typedef int (*dial_api_get_log_level)(void);

int hccast_dial_api_init();
int hccast_dial_api_service_init(dial_fn_event func);
int hccast_dial_api_service_start(struct dial_svr_param *param);
int hccast_dial_api_service_stop(void);
char *hccast_dial_api_service_get_version(void);
int hccast_dial_api_service_set_log_level(int level);

#endif
