/**
* @file
* @brief                hudi persistentmem interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_PERSISTENTMEM_INTER_H__
#define __HUDI_PERSISTENTMEM_INTER_H__

#define HUDI_PERSISTENTMEM_DEV    "/dev/persistentmem"

typedef struct
{
    int inited;
    int fd;
} hudi_persistentmem_instance_t;

#endif
