#include <stdio.h>
#include "hudi/hudi_cec.h"
#ifdef __linux__
#include <termios.h>
#include <signal.h>
#include "../console.h"
#else
#include <kernel/lib/console.h>
#endif
static int hdmi_cec_test_start(void)
{
    hudi_handle                  handle = NULL;
    hudi_cec_config_t            config = {};
    hudi_cec_logical_addresses_t laes   = {};
    config.dev_path                     = "/dev/hdmi_rx";
    config.msgid_action                 = HUDI_CEC_MSGID_ACTION_CREATE;
    hudi_cec_cmd_t cmd                  = {};

    // 1.open conn
    printf("api test1:%d\n", hudi_cec_open(&handle, &config));

    // 2.use conn(test2 ~ test 6)
    printf("api test2:%d\n",
           hudi_cec_standby_device(handle, HUDI_CEC_DEVICE_BROADCAST));

    printf("api test3:%d\n", hudi_cec_scan_devices(handle, &laes));
    for (int i = 0; i < 15; i++) {
        if (laes.addresses[i] > 0) {
            printf("scan device:%d\n", i);
        }
    }

    printf("api test4:%d\n",
           hudi_cec_poweron_device(handle, HUDI_CEC_DEVICE_BROADCAST));

    printf("api test5:%d\n", hudi_cec_get_active_devices(handle, &laes, 500));
    for (int i = 0; i < 15; i++) {
        if (laes.addresses[i] > 1) {
            printf("active device:%d\n", i);
        }
    }

    printf("api test6:%d\n", hudi_cec_msg_receive(handle, &cmd, true));
    printf("get cec_cmd(i,d,o,p):(%d,%d,%02x,%02x)\n", cmd.initiator,
           cmd.destination, cmd.opcode, cmd.parameters.data[0]);

    // close conn
    printf("api test7:%d\n", hudi_cec_close(handle));

    return 0;
}

static int cec_exam_test(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    hdmi_cec_test_start();

    return 0;
}

#ifdef __linux__
static struct termios stored_settings;
static void exit_console(int signo)
{
	(void)signo;
	tcsetattr (0, TCSANOW, &stored_settings);
	exit(0);
}

int main (int argc, char *argv[]) 
{
	struct termios new_settings;

	tcgetattr(0, &stored_settings);
	new_settings = stored_settings;
	new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);

	signal(SIGTERM, exit_console);
	signal(SIGINT, exit_console);
	signal(SIGSEGV, exit_console);
	signal(SIGBUS, exit_console);
	console_init("cec_exam:");
	
	console_register_cmd(NULL, "run", cec_exam_test, CONSOLE_CMD_MODE_SELF, "run cec test");
	
	console_start();
	exit_console(0);
	(void)argc;
	(void)argv;
	return 0;
}
#else
static int cec_exam_entry(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("cec example entry!\n");

    return 0;
}

CONSOLE_CMD(cec_exam, NULL, cec_exam_entry, CONSOLE_CMD_MODE_SELF,
            "enter cec test");
CONSOLE_CMD(run, "cec_exam", cec_exam_test, CONSOLE_CMD_MODE_SELF,
            "run cec test");
#endif

