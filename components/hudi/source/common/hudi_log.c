/**
* @file         
* @brief		        hudi log interface
* @par Copyright(c): 	Hichip Semiconductor (c) 2023
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include <hudi_com.h>
#include "hudi_log.h"

hudi_log_level g_hudi_log_level = HUDI_LL_NOTICE;

void hudi_log_level_set(hudi_log_level level)
{
    g_hudi_log_level = level;
}
