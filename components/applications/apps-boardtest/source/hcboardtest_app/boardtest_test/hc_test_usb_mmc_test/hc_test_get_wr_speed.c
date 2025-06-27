#include <stdio.h>
#include <time.h>
#include "hc_test_usb_mmc.h"

//Obtain USB and SD read and write speeds
int get_write_read_speed(char *path, size_t per_bytes, size_t total_bytes, wr_speed *arg)
{
    char *buf;
    static struct timespec start_time, end_time;
    int elapsed_time, single, total = 0;
    int fd;  //cost_ms is the total time, single is a single write, total is the total write

    buf = malloc(per_bytes);
    if (buf == NULL)
    {
        printf("Error: Cannot malloc 64KB buffer\n");
        return -1;
    }
    memset(buf, 'k', per_bytes);

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC);
    if (fd < 0)
    {
        printf("Error: Cannot create %s\n", path);
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (;;)
    {
        single = write(fd, buf, per_bytes);
        total += single;
        if ((size_t) single != per_bytes || (size_t)total > total_bytes)
        {
            break;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_time = (end_time.tv_sec * 1000.0 + end_time.tv_nsec / 1000000.0) - 
                    (start_time.tv_sec * 1000.0 + start_time.tv_nsec / 1000000.0);

    arg->write_speed = total/elapsed_time; //Write Speed
    printf("Write: speed: %d.%d MB/s, %d KB/s\n", 
            (total/1000)/elapsed_time, (total/elapsed_time)%1000, total/elapsed_time);
    close(fd);
    /*----------------------------------------------------------------------------------*/

    total = 0;
    lseek(fd, 0, SEEK_SET);
    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        printf("Error: Cannot open %s\n", path);
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (;;)
    {
        single = read(fd, buf, per_bytes);
        total += single;
        if ((size_t)single != per_bytes || (size_t)total > total_bytes)
        {
            break;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_time = (end_time.tv_sec * 1000.0 + end_time.tv_nsec / 1000000.0) - 
                    (start_time.tv_sec * 1000.0 + start_time.tv_nsec / 1000000.0);

    close(fd);
    free(buf);
    arg->read_speed = (total) / elapsed_time; //Read speed
    printf("Read: speed: %d.%d MB/s, %d KB/s\n", 
            (total/1000)/elapsed_time, (total/elapsed_time)%1000, total/elapsed_time);
    return 0;
}