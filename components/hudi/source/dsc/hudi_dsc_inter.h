#ifndef __HUDI_DSC_INTER_H__
#define __HUDI_DSC_INTER_H__

#include <hudi_dsc.h>

#define HUDI_DSC_DEV       "/dev/dsc"
#define HUDI_DSC_BLOCK_SIZE (32 * 1024)

typedef struct
{
    hudi_dsc_config_t config;
    unsigned char *dsc_buf;
    int inited;
    int fd;
} hudi_dsc_instance_t;

#endif
