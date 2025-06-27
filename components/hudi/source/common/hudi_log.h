/**
* @file         
* @brief		        hudi log interface
* @par Copyright(c): 	Hichip Semiconductor (c) 2023
*/

#ifndef __HUDI_LOG_H__
#define __HUDI_LOG_H__

#include <hudi_com.h>

extern hudi_log_level g_hudi_log_level;

#define hudi_log(level, fmt, ...) \
    do { \
        if (level <= g_hudi_log_level) printf("[hudi]"fmt, ##__VA_ARGS__); \
    } while(0)

#endif
