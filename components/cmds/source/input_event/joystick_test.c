
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
#include <hcuapi/joystick.h>
#include <hcuapi/sys-blocking-notify.h>
#include <kernel/notify.h>
#include <kernel/module.h>


#define BUF_SIZE 1024
#define MAX_EVENT 16
#define INPUT_BUF_SZ 256

struct fds_st {
	int fd;
	char path[64];
};

static bool g_js_force_exit = false;


static int joystick_test(int argc, char *argv[])
{
	struct pollfd events[MAX_EVENT];
	struct fds_st fds[MAX_EVENT];
	DIR *dir;
	struct dirent *dp;
	int event_total, rd, ret = 0;
	int index = 0;
	struct js_event *temp;
    struct input_id id;
    char input_buf[INPUT_BUF_SZ];
	char file_name[512] = { 0 };

    // elog_set_filter_tag_lvl("hid", ELOG_LVL_INFO);

	dir = opendir("/dev/input");
	if (dir == NULL) {
		printf("Error: Cannot open /dev/input\n");
		return -1;
	}

	memset(&fds[0], 0, MAX_EVENT * sizeof(struct fds_st));
	do {
		if(index >= MAX_EVENT)
			break;

		if((dp = readdir(dir)) == NULL)
			break;

		if(!strstr(dp->d_name, "js"))
			continue;

		sprintf(fds[index].path, "/dev/input/%s", dp->d_name);
		fds[index].fd = open(fds[index].path, O_RDWR);
		if(fds[index].fd < 0) {
			log_e("[Error] cannot open %s\n", fds[index].path);
			continue;
		}

		events[index].fd = fds[index].fd;
		events[index].events = POLLIN | POLLRDNORM;

        memset(&id, 0, sizeof(struct input_id));
        ioctl(fds[index].fd, EVIOCGID, &id);		

		printf("[%d] fd:%d path:%s (%4.4x-%4.4x-%4.4x)\n", 
			index, fds[index].fd, fds[index].path,
            id.vendor, id.product, id.version);	
			
		index++;
	} while(1);

	printf(" ========================================== \n");

	event_total = index;
	printf(" ==> poll total numbers : %d\n", event_total);

	while(1) {

        /* wait for input event */
		ret = poll(&events[0], event_total, 300);
		if(ret == 0) {
			if(g_js_force_exit)
				break;
		}
		else if(ret < 0) {
            log_e("[Error] poll error\n");
            goto __force_exit;
		}

		for(index = 0; index < event_total; index++) {
			if(events[index].revents & (POLLIN | POLLRDNORM)) {

				rd = read(fds[index].fd, &input_buf,  sizeof(struct js_event));
				if(rd != sizeof(struct js_event)) {
					printf("[%s] : read error, return len %d\n", 
						fds[index].path, rd);
					continue;
				}

				temp = (struct js_event *)&input_buf[0];
				printf("[%s] : %d %x %x %x\n", 
					fds[index].path,  temp->time, 
					temp->type, temp->number, temp->value);
			}
		}
	}

__force_exit:

	for(index = 0; index < event_total; index++)
		close(fds[index].fd);
	
	closedir(dir);
	return ret;
}

static int input_dev_notify(struct notifier_block *self,
			       unsigned long action, void *param)
{
	struct removable_notify_info *info = 
		(struct removable_notify_info *)param;
	switch (action) {
	case INPUT_DEV_NOTIFY_CONNECT:
		printf(" ==> input dev connect : %s\n", info->devname);
		g_js_force_exit = false;		
		break;
	case INPUT_DEV_NOTIFY_DISCONNECT:
		printf(" ==> input dev disconnect : %s\n", info->devname);
		g_js_force_exit = true;
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block input_dev_nb = {
	.notifier_call = input_dev_notify,
};

static int input_dev_init(void)
{
	printf(" ===> input_dev_init \n");
	sys_register_notify(&input_dev_nb);
	return 0;
}


module_system(input_dev, input_dev_init, NULL, 4)


CONSOLE_CMD(joystick, NULL, joystick_test, CONSOLE_CMD_MODE_SELF, "joystick test")
