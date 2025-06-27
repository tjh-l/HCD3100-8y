
#include <kernel/elog.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/input.h>
#include <kernel/lib/console.h>
#include <nuttx/fs/fs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#define BUF_SIZE 1024
#define MAX_EVENT 16

struct fds_st {
	int fd;
	char path[64];
};

static int input_poll_test(int argc, char *argv[])
{
	struct pollfd events[MAX_EVENT];
	struct fds_st fds[MAX_EVENT];
	DIR *dir;
	struct dirent *dp;
	int index, event_total, rd, ret = 0;
	struct input_event input_buf;
	// TimerHandle_t xTimer;

	char file_name[512] = { 0 };

    // elog_set_filter_tag_lvl("hid", ELOG_LVL_INFO);

	dir = opendir("/dev/input");
	if (dir == NULL) {
		printf("Error: Cannot open /dev/input\n");
		return -1;
	}

	memset(&fds[0], 0, MAX_EVENT * sizeof(struct fds_st));
	for(index = 0; (index < MAX_EVENT) && ((dp = readdir(dir)) != NULL); index++) {
		sprintf(fds[index].path, "/dev/input/%s", dp->d_name);
		fds[index].fd = open(fds[index].path, O_RDWR);
		if(fds[index].fd < 0) {
			index--;
			continue;
		}

		events[index].fd = fds[index].fd;
		events[index].events = POLLIN | POLLRDNORM;

		printf("[%d] fd:%d path:%s \n", 
			index, fds[index].fd, fds[index].path);
	}

	event_total = index;

	while(1) {
        /* wait for input event */
        if (poll(&events[0], event_total, -1) < 0) {
            log_e("[Error] poll error\n");
            ret = -1;
            goto __force_exit;
        }

		for(index = 0; index < event_total; index++) {
			if(events[index].revents & (POLLIN | POLLRDNORM)) {
				rd = read(fds[index].fd, &input_buf, sizeof(struct input_event));
				if(rd != sizeof(struct input_event)) {
					log_e("[Error] Read input event (%s) error\n", 
							fds[index].path);
					continue;
				}
				printf("[%s] : %x %x %ld\n", 
					fds[index].path,  input_buf.type, 
					input_buf.code, input_buf.value);
			}
		}
	}

__force_exit:
	for(index = 0; index < event_total; index++)
		close(fds[index].fd);
	
	closedir(dir);
	return ret;
}

CONSOLE_CMD(input_poll, NULL, input_poll_test, CONSOLE_CMD_MODE_SELF, "input poll test")
