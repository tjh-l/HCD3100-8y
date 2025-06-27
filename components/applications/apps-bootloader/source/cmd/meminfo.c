#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <kernel/module.h>
#include <kernel/lib/console.h>
#include <kernel/io.h>

static int meminfo(int argc, char **argv)
{
	uint32_t size = 0, type = 0, frequency = 0, ic;

	type = REG32_GET_FIELD2(0xb8801000, 23, 1);
	size = REG32_GET_FIELD2(0xb8801000, 0, 3);
	frequency = REG32_GET_FIELD2(0xb8800070, 4, 3);

	type = type + 2;

	size = 16 << (size);

	switch (frequency) {
	case 0:
		frequency = 800;
		break;
	case 1:
		frequency = 1066;
		break;
	case 2:
		frequency = 1333;
		break;
	case 3:
		frequency = 1600;
		break;
	case 4:
		frequency = 600;
		break;
	default:
		break;
	}

	ic = REG8_READ(0xb8800003);
	if (REG16_GET_BIT(0xb880048a, BIT15)) {
		if (ic == 0x15) 
			frequency = REG16_GET_FIELD2(0xb880048a, 0, 15) * 12;
		else if (ic == 0x16)
			frequency = REG16_GET_FIELD2(0xb880048a, 0, 15) * 24;
	}

	printf("\nDDR Info:\n");
	printf("\ttype:DDR%ld\n", type);
	printf("\tsize:%ldM\n", size);
	printf("\tfreq:%ldMHz\n\n", frequency);

	return 0;
}

static int show_meminfo(void)
{
	return meminfo(0, NULL);
}

__initcall(show_meminfo);

CONSOLE_CMD(meminfo, NULL, meminfo, CONSOLE_CMD_MODE_SELF, "show memory info")
