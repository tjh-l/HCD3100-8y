#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/input.h>
#include <kernel/lib/console.h>

#define BUF_SIZE 1024

static inline int atest_bit(int nr, const unsigned long *addr) {
    return !!(addr[nr / (sizeof(unsigned long) * 8)] & (1UL << (nr % (sizeof(unsigned long) * 8))));
}

// 辅助函数，将事件类型编号转换为字符串
const char *ev_to_str(int ev_type) {
    switch (ev_type) {
        case EV_SYN:  return "SYN";
        case EV_KEY:  return "KEY";
        case EV_REL:  return "REL";
        case EV_ABS:  return "ABS";
        case EV_MSC:  return "MSC";
        case EV_SW:   return "SW";
        case EV_LED:  return "LED";
        case EV_SND:  return "SND";
        case EV_REP:  return "REP";
        case EV_FF:   return "FF";
        case EV_PWR:  return "PWR";
        case EV_FF_STATUS: return "FF_STATUS";
        default:     return "UNKNOWN";
    }
}

static void print_help(void) {
	printf("***********************************\n");
	printf("input test cmds help\n");
	printf("\tfor example : input_ioctl -i 1\n");
	printf("\t'i'	1 means event1\n");
	printf("***********************************\n");
}

static int input_ioctl_test(int argc, char *argv[])
{
	int fd;
	char input_buf[128];

	long tmp;
	int x = 0, y = 0;
	int event_num = -1;
	char ch;
	opterr = 0;
	optind = 0;

	while ((ch = getopt(argc, argv, "hi:")) != EOF) {
		switch (ch) {
		case 'h':
			print_help();
			return 0;
		case 'i':
			tmp = strtoll(optarg, NULL, 10);
			event_num = tmp;
			break;
		default:
			printf("Invalid parameter %c\r\n", ch);
			print_help();
			return -1;
		}
	}
	if (event_num == -1) {
		print_help();
		return -1;
	}

	sprintf(input_buf, "/dev/input/event%d", event_num);

	unsigned long *evbits; // 动态分配足够存储所有事件类型位字段的内存
	size_t evbits_size = (EV_MAX + 1) / 8; // 计算所需字节数
	evbits = malloc(evbits_size);
	if (!evbits) {
		perror("Failed to allocate memory for evbits");
		return EXIT_FAILURE;
	}
	memset(evbits, 0, evbits_size); // 初始化位字段为0

	struct input_absinfo absinfo;
	int grab_status;

	// 尝试打开输入设备节点，例如 "/dev/input/event0"
	fd = open(input_buf, O_RDONLY);
	if (fd < 0) {
		perror("Cannot open input device");
		free(evbits);
		return EXIT_FAILURE;
	}

	//获取设备名字
	char name[256] = "Unknown";
	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	printf("\nInput Device Name:[%s]. \n", name);

	// 获取事件类型位字段
	if (ioctl(fd, EVIOCGBIT(0, evbits_size),evbits) < 0) {
		perror("Error getting event bits");
		close(fd);
		free(evbits);
		return EXIT_FAILURE;
	}

	printf("Supported events:\n");
	for (int i = 0; i <= EV_MAX; ++i) {
		if (atest_bit(i, evbits)) {
			printf("%s\n", ev_to_str(i));
		}
	}

	// 检查设备是否支持绝对轴事件，并尝试获取相关信息
	if (atest_bit(EV_ABS, evbits)) {
		// 假设存在一个绝对轴，编号为ABS_MT_POSITION_X，实际使用时需要根据设备情况进行调整
		if (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo) < 0) {
			perror("Error getting absolute axis info");
		} else {
			printf("Absolute axis info: Value = %d, Minimum = %d, Maximum = %d\n",
					absinfo.value, absinfo.minimum, absinfo.maximum);
		}
		// 假设存在一个绝对轴，编号为ABS_MT_POSITION_Y，实际使用时需要根据设备情况进行调整
		if (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo) < 0) {
			perror("Error getting absolute axis info");
		} else {
			printf("Absolute axis info: Value = %d, Minimum = %d, Maximum = %d\n",
					absinfo.value, absinfo.minimum, absinfo.maximum);
		}
	}

	// 尝试抓取设备
	grab_status = ioctl(fd, EVIOCGRAB, 1);
	if (grab_status < 0) {
		perror("Error grabbing device");
	} else {
		printf("Device grabbed successfully.\n");

		// ... 在这里执行需要独占设备的操作 ...

		// 释放设备
		if (ioctl(fd, EVIOCGRAB, 0) < 0) {
			perror("Error releasing device");
		}
		printf("Device released successfully.\n");
	}

	close(fd);
	free(evbits); // 释放之前分配的内存
	return EXIT_SUCCESS;
}

CONSOLE_CMD(input_ioctl, NULL, input_ioctl_test, CONSOLE_CMD_MODE_SELF, "input test some ioctl")
