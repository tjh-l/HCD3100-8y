#ifndef _HC_TEST_USBD_H_
#define _HC_TEST_USBD_H_
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <dirent.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 

#include <linux/kernel.h>

#include <kernel/notify.h>
#include <hcuapi/sys-blocking-notify.h>
#include "../boardtest_module.h"

#define TEST_CLOCKS_RER_SEC 8 //The time, in seconds, waiting to receive an event

typedef enum
{
    USB_STAT_MOUNT,
    USB_STAT_UNMOUNT,
    USB_STAT_MOUNT_FAIL,
    USB_STAT_UNMOUNT_FAIL,
    USB_STAT_INVALID,
    SD_STAT_MOUNT,
    SD_STAT_UNMOUNT,
    SD_STAT_MOUNT_FAIL,
    SD_STAT_UNMOUNT_FAIL,
}USB_MMC_STATE;  //USB, MMC mounting status

typedef struct{
    int write_speed;
    int read_speed;
}wr_speed;  //read or write speed

/**
 * @brief Get the write read speed object
 * 
 * @param path Read/Write File Path
 * @param per_bytes Size of single read/write
 * @param total_bytes Total file size read/write
 * @param arg get read or write speed
 * @return int 
 */
int get_write_read_speed(char *path, size_t per_bytes, size_t total_bytes, wr_speed *arg);

#endif
