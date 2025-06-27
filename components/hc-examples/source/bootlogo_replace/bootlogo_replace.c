#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <malloc.h>
#include <signal.h>
#include <errno.h>

#ifdef __HCRTOS__
#include <nuttx/mtd/mtd.h>
#else
#include <mtd/mtd-user.h>
#endif

#define BOOTLOGO_NAME "romfs.img"

#ifdef __HCRTOS__
static int api_get_mtdblock_devpath(char *devpath, int len, const char *partname)
{
    static int np = -1;
    static u32 part_num = 0;
    u32 i = 1;
    const char *label;
    char property[32];

    if (np < 0) {
        np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
    }

    if (np < 0)
        return -1;

    if (part_num == 0)
        fdt_get_property_u_32_index(np, "part-num", 0, &part_num);

    for (i = 1; i <= part_num; i++) {
        snprintf(property, sizeof(property), "part%d-label", i);
        if (!fdt_get_property_string_index(np, property, 0, &label) &&
            !strcmp(label, partname)) {
            memset(devpath, 0, len);
            snprintf(devpath, len, "/dev/mtdblock%d", i);
            return i;
        }
    }

    return -1;
}
#else
static int api_dts_uint32_get(const char *path)
{
    int fd = open(path, O_RDONLY);
    int value = -1;
    if(fd >= 0){
        uint8_t buf[4];
        if(read(fd, buf, 4) != 4){
            close(fd);
            return value;
        }
        close(fd);
        value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
    }
    printf("fd:%d ,dts value: %x\n", fd,value);
    return value;
}

static void api_dts_string_get(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);
    if(fd >= 0){
        read(fd, string, size);
        close(fd);
    }
    //printf("dts string: %s\n", string);
}

static int api_get_mtdblock_devpath(char *devpath, int len, const char *partname)
{
    uint32_t i = 1;
    uint32_t part_num = 0;
    char label[32] = {0};
    char property[128] = {0};

    if (part_num == 0)
        part_num = api_dts_uint32_get("/proc/device-tree/hcrtos/sfspi/spi_nor_flash/partitions/part-num");

    for (i = 1; i <= part_num; i++) {
        snprintf(property, sizeof(property), "/proc/device-tree/hcrtos/sfspi/spi_nor_flash/partitions/part%d-label", i);
        api_dts_string_get(property, label, sizeof(label));
        if (!strcmp(label, partname)){
            memset(devpath, 0, len);
            //snprintf(devpath, len, "/dev/mtdblock%d", i);
            snprintf(devpath, len, "/dev/mtd%d", i);

            printf("%s(), line:%d. devpath:%s\n", __func__, __LINE__, devpath);
            return i;
        }
    }
    printf("%s(), line:%d. cannot find mtd dev path!\n", __func__, __LINE__);
    return -1;
}

#endif

static int get_eromfs_part_erase_size(int dev_fd, uint32_t *erase_size, uint32_t *part_size)
{
#ifdef __linux__
    struct mtd_info_user mtd = {0};
    ioctl(dev_fd, MEMGETINFO, &mtd);

    *erase_size = mtd.erasesize;
    *part_size = mtd.size;
#else
    //in fact, write mtd is OK without erasing.
    //but we do erase to ensure it reased to 0xff.
    struct mtd_geometry_s geo = {0};
    ioctl(dev_fd, MTDIOC_GEOMETRY, &geo);

    *erase_size = geo.erasesize;
    *part_size = geo.size;
#endif

    return 0;
}

static int erase_eromfs_partition(int dev_fd, uint32_t erase_size, uint32_t part_size)
{
    uint32_t left = part_size;
    uint32_t offset = 0;

    lseek(dev_fd, 0, SEEK_SET);
    while(left > 0){
#ifdef __linux__
        struct erase_info_user erase = {0};

        erase.start = offset;
        erase.length = erase_size;
        if (ioctl(dev_fd, MEMERASE, &erase) < 0) {
            printf("Error: erase_eromfs_partition failed \n");
            return -1;
        }
#else
        //in fact, write mtd is OK without erasing.
        //but we do erase to ensure it reased to 0xff.
        struct mtd_eraseinfo_s eraseinfo = {0};

        eraseinfo.start = offset;
        eraseinfo.length = erase_size;
        if (ioctl(dev_fd, MTDIOC_MEMERASE, &eraseinfo) < 0) {
            printf("Error: erase_eromfs_partition failed \n");
            return -1;
        }
#endif

        left -= erase_size;
        if(left < erase_size){
            break;
        }
        offset += erase_size;
        lseek(dev_fd, offset, SEEK_SET);
    }

    return 0;
}

int bootlogo_replace(char *filepath)
{
    FILE *file_fp = NULL;
    long int file_size;
    char devpath[128];
    int dev_fd = -1;
    int ret = -1;
    int once_len = 0, write_size = 0;
    uint32_t erase_size = 0,  part_size = 0;
    char *buf[65536] = {0};  /* max erase size is 64K */

    printf("%s:%d: filepath=%s\n", __func__, __LINE__, filepath);

    /* 1. is file exist? */

    file_fp = fopen(filepath, "rb");
    if(file_fp == NULL){
        printf("%s:%d: %s is not exist\n", __func__, __LINE__, filepath);
        goto end;
    }

    fseek(file_fp, 0, SEEK_END);
    file_size = ftell(file_fp);

    if(file_size == 0){
        printf("%s:%d: file_size can be zero\n", __func__, __LINE__);
        ret = -ENOENT;
        goto end;
    }

    printf("%s:%d: file_size=%lu\n", __func__, __LINE__,  file_size);

    /* get romfs partition */
    ret = api_get_mtdblock_devpath(devpath, sizeof(devpath), "eromfs");
    if(ret < 0){
        printf("get_mtdblock_devpath failed\n");
        goto end;
    }

    dev_fd = open(devpath, O_RDWR);
    if (dev_fd <0){
        printf("Error: open %s failed \n", devpath);
        goto end;
    }

    get_eromfs_part_erase_size(dev_fd, &erase_size, &part_size);
    printf("%s:%d: erase_size=%u, part_size=%u\n", __func__, __LINE__, erase_size,  part_size);

    if(file_size > part_size){
        printf("%s:%d: file is to big, file_size=%lu, part_size=%u\n", __func__, __LINE__, file_size,  part_size);
        goto end;
    }

    /* erase romfs partition */
    ret = erase_eromfs_partition(dev_fd, erase_size, part_size);
    if(ret != 0){
        printf("erase_eromfs_partition failed\n");
        goto end;
    }

    fseek(file_fp, 0, SEEK_SET);
    lseek(dev_fd, 0, SEEK_SET);
    once_len = fread(buf, 1, erase_size, file_fp);
    while(once_len != 0){
        /* write partition */
        write_size = write(dev_fd, buf, once_len);
        if(write_size != once_len){
            printf("Error: write %s failed \n", devpath);
            ret = -1;
            goto end;
        }
        //printf("%s:%d: once_len=%u, write_size=%u\n", __func__, __LINE__, once_len, write_size);

        /* read file */
        once_len = fread(buf, 1, erase_size, file_fp);
    }

    return 0;

end:
    if(file_fp){
        fclose(file_fp);
        file_fp = NULL;
    }

    if(dev_fd >= 0){
        close(dev_fd);
        dev_fd  = -1;
    }

    return ret;
}

int main(int argc, char *argv[])
{
    bootlogo_replace("/media/hdd/romfs.img");

    return 0;
}

