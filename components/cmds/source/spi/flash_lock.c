#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/delay.h>
#include <kernel/lib/console.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>

#include <hcuapi/iocbase.h>
#include <uapi/mtd/mtd-abi.h>

static void print_help(void) {
	printf("***********************************\n");
	printf("flash_lock test cmds help\n");
	printf("\tfor example : flash_lock -i0 -l1\n");
	printf("\t'i'	0 means /dev/mtd0\n");
	printf("\t'l'	1 means lock, 0 means unlock all area\n");
	printf("***********************************\n");
}

int flash_lock_test(int argc, char * argv[])
{
    int ret = 0;
    int fd;
    char *s = "/dev/mtd";
    char devpath[124];
    int mtd_num = -1;
    int lock = -1;

    char ch;
    long tmp;
    opterr = 0;
    optind = 0;

    while ((ch = getopt(argc, argv, "hi:l:")) != EOF) {
	    switch (ch) {
	    case 'h':
		    print_help();
		    return 0;
	    case 'i':
		    tmp = strtoll(optarg, NULL, 10);
		    mtd_num = tmp;
		    break;
	    case 'l':
		    tmp = strtoll(optarg, NULL, 10);
		    lock = tmp;
		    break;
	    default:
		    printf("Invalid parameter %c\r\n", ch);
		    print_help();
		    return -1;
	    }
    }

    if (mtd_num == -1)
	    return -1;

    sprintf(devpath, "/dev/mtd%d", mtd_num);

    fd = open(devpath, O_RDWR);
    if (fd < 0) {
	    printf("can't open device %s\n", devpath);
	    return -1;
    }

    printf("open device %s\n", devpath);

    struct erase_info_user ops_info;
    struct mtd_info_user mtd_info;

    if (ioctl(fd, MEMGETINFO, &mtd_info) != 0) {
	    printf("Error get MTD block");
	    close(fd);
	    return EXIT_FAILURE;
    }

    printf("mtd_info.size = 0x%x\n", mtd_info.size);
    printf("mtd_info.erasesize = 0x%x\n", mtd_info.erasesize);

    memset(&ops_info, 0, sizeof(ops_info));
    ops_info.start = 0; //start from 0x00
    ops_info.length = (mtd_info.size / 4);

    if (lock == 1) {
	    /* lock */
	    if (ioctl(fd, MEMLOCK, &ops_info) != 0) {
		    perror("Error locking MTD block");
		    close(fd);
		    return EXIT_FAILURE;
	    }
    } else if (lock == 0) {
	    ops_info.length = mtd_info.size;
	    /* unlock */
	    if (ioctl(fd, MEMUNLOCK, &ops_info) != 0) {
		    perror("Error unlocking MTD block");
		    close(fd);
		    return EXIT_FAILURE;
	    }
    }

    printf("ops_info.start = 0x%x\n", ops_info.start);
    printf("ops_info.length = 0x%x\n", ops_info.length);


    close(fd);

    return ret;
}

CONSOLE_CMD(flash_lock,NULL,flash_lock_test,CONSOLE_CMD_MODE_SELF,"flash lock sample cmd")
