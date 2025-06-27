/**
* @file
* @brief                hudi power interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_POWER_INTER_H__
#define __HUDI_POWER_INTER_H__

#define HUDI_WATCHDOG_DEV    "/dev/watchdog"
#define HUDI_STANDBY_DEV     "/dev/standby"

typedef struct
{
    int inited;
    int fd;
} hudi_watchdog_instance_t;

typedef struct
{
    int inited;
    int fd;
} hudi_standby_instance_t;

#endif
