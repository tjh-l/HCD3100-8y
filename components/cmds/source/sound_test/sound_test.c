#include <sys/types.h>
#include <kernel/lib/console.h>

static int sound_test(int argc, char *argv[]){
	return 0;
}

CONSOLE_CMD(snd, NULL, sound_test, CONSOLE_CMD_MODE_SELF, "enter sound test")
