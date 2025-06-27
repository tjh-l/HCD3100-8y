#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/lib/console.h>

int mtd_test(int argc, char **argv)
{
	printf("WARNNING: flash torture test will destroy existing data on flash and maybe broken you flash!!!\n");
	printf("WARNNING: flash torture test will destroy existing data on flash and maybe broken you flash!!!\n");
	printf("WARNNING: flash torture test will destroy existing data on flash and maybe broken you flash!!!\n");

	return 0;
}
CONSOLE_CMD(mtd_test, NULL, mtd_test, CONSOLE_CMD_MODE_SELF,
	    "mtd test commands")
