/**
 * @file
 * @brief                hudi cec module log.
 * @par Copyright(c):    Hichip Semiconductor (c) 2024
 */
#ifndef __HUDI_CEC_LOG_H__
#define __HUDI_CEC_LOG_H__
#include <hudi_com.h>
#include <hudi_log.h>

extern hudi_log_level g_hudi_cec_log_level;

#define hudi_cec_pr(level, fmt, ...) \
    do { \
        if (level <= g_hudi_cec_log_level ||level <= g_hudi_log_level) printf(fmt, ##__VA_ARGS__); \
    } while(0);

#define hudi_cec(level, fmt, ...) \
    do { \
        if (level <= g_hudi_cec_log_level ||level <= g_hudi_log_level) printf("[hudi]"fmt, ##__VA_ARGS__); \
    } while(0);

#endif
