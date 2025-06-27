#include "app_log.h"

int g_app_log_level = LL_NOTICE;

int app_log_level_get()
{
    return g_app_log_level;
}

void app_log_level_set(int level)
{
    g_app_log_level = level;
}
