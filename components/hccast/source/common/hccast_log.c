#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "hccast_log.h"

int g_hccast_log_level = LL_NOTICE;

void hccast_log_level_set(int level)
{
    g_hccast_log_level = level;
}
