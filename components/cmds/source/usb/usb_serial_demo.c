#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <kernel/vfs.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <kernel/io.h>
#include <getopt.h>
#include <malloc.h>
#include <kernel/lib/console.h>
#include <kernel/completion.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hcuapi/common.h>

/* ************************************************ */
/* usb-serial tty console demo */
/* ************************************************ */
int usb_serial_tty_console(int argc, char **argv)
{
	uint8_t *at_cmd[] = { "AT\r\n",
			      "AT+CPIN?\r\n",
			      "AT+CSQ\r\n",
			      "AT+RNDISCALL=1\r\n",
			      "AT+RNDISCALL?\r\n",
			      NULL };
	uint8_t at_resp[128];
	uint8_t *ptr_res;
	int rd_cnt, fd, index;
	struct pollfd fds[1];

	if (argc != 2) {
		printf("[Error] command error. eg. usb_console /dev/ttyUSB2\n");
		return -1;
	}
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("[Error] Cannot open device(%s)\n", argv[1]);
		return -1;
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN | POLLRDNORM;

	for (index = 0; at_cmd[index] != NULL; index++) {
		printf("\n==> AT command (len:%ld): \n%s\n",
		       strlen(at_cmd[index]), at_cmd[index]);

		/* send AT cammand to console */
		write(fd, at_cmd[index], strlen(at_cmd[index]));

		/* receive response from console */
		rd_cnt = 0;
		ptr_res = &at_resp[0];
		memset(&at_resp[0], 0, 128);
		while (1) {
			if (poll(fds, 1, 200) <= 0) // wait for 200ms
				break;
			rd_cnt += read(fd, ptr_res++, 1);
		}
		printf("==> AT response (len:%d): \n"
		       "%s\n"
		       "====================================\n",
		       rd_cnt, &at_resp[0]);
	}

exit:
	if (fd)
		close(fd);

	printf("exit usb serial console\n");
	return 0;
}

