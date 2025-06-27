#ifndef __HUDI_AVSYNC_INTER_H__
#define __HUDI_AVSYNC_INTER_H__

#define HUDI_AVSYNC_DEV    "/dev/avsync0"

typedef struct
{
    int inited;
    int fd;
} hudi_avsync_instance_t;

#endif
