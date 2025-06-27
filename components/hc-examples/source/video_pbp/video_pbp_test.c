#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#ifdef __linux__
#include <termios.h>
#include <signal.h>
#include "console.h"
#include <linux/fb.h>
#else
#include <kernel/lib/console.h>
#include <kernel/fb.h>
#endif

#include <hcuapi/dis.h>
#include "pbp_slot_mgr.h"

static void set_osd_onoff(bool on)
{
	int fd = -1;
    uint32_t blank_mode;

	fd = open("/dev/fb0", O_RDWR);
	if(fd < 0) {
		return;
	}

	blank_mode = on ? FB_BLANK_UNBLANK : FB_BLANK_NORMAL;
    ioctl(fd, FBIOBLANK, blank_mode);
    close(fd);
}

static int pbp_dis_onoff(dis_type_e dis_type , dis_layer_e dis_layer, int on_off)
{
	int fd = -1;
	struct dis_win_onoff winon = { 0 };

	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	winon.distype = dis_type;
	winon.layer = dis_layer;
	winon.on = on_off ? 1 : 0;
	ioctl(fd , DIS_SET_WIN_ONOFF , &winon);

	close(fd);
	return 0;
}

static void set_pic_layer_order(dis_type_e dis_type)
{
	int fd = -1;
	struct dis_layer_blend_order vhance = { 0 };
	
	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return;
	}

	vhance.distype = dis_type;
	vhance.auxp_layer = 3;
	vhance.main_layer = 2;	
	vhance.gmas_layer = 1;
	vhance.gmaf_layer = 0;

	ioctl(fd , DIS_SET_LAYER_ORDER , &vhance);
	close(fd);
}

static int video_pbp_init(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	pbp_slot_busy_init();
	pbp_dis_onoff(DIS_TYPE_HD, DIS_LAYER_MAIN, 0);
	pbp_dis_onoff(DIS_TYPE_HD, DIS_LAYER_AUXP, 0);
	pbp_dis_onoff(DIS_TYPE_UHD, DIS_LAYER_MAIN, 0);
	pbp_dis_onoff(DIS_TYPE_UHD, DIS_LAYER_AUXP, 0);
	set_pic_layer_order(DIS_TYPE_HD);
	set_pic_layer_order(DIS_TYPE_UHD);

	return 0;
}


static int pbp_slot_set(int argc, char *argv[])
{
	int opt;
	opterr = 0;
	optind = 0;

	pbp_dis_src_e dis_src = PBP_DIS_SRC_MP;
	pbp_dis_type_e dis_type = PBP_DIS_TYPE_HD;
	pbp_dis_layer_e dis_layer = PBP_DIS_LAYER_MAIN;

	if (argc < 4) {
		printf("%s(). parameter is error!\n", __func__);
		return -1;
	}

	optind = 1;
	while ((opt = getopt(argc, &argv[0], "d:l:s:")) != EOF) {
		switch (opt) {
		case 'd':
			dis_type = (pbp_dis_type_e)atoi(optarg);
			printf("%s(), dis type: %d\n", __func__, (int)dis_type);
			break;
		case 'l':
			dis_layer = (pbp_dis_layer_e)atoi(optarg);
			printf("%s(), dis layer: %d\n", __func__, (int)dis_layer);
			break;
		case 's':
			dis_src = (pbp_dis_src_e)atoi(optarg);
			printf("%s(), pbp source: %d\n", __func__, (int)dis_src);
			break;
		default:
			break;
		}
	}

	pbp_slot_busy_set(dis_type, dis_layer, dis_src);
	return 0;

}

static int pbp_slot_get(int argc, char *argv[])
{
	int opt;
	opterr = 0;
	optind = 0;

	pbp_dis_type_e dis_type = PBP_DIS_TYPE_HD;
	pbp_dis_layer_e dis_layer = PBP_DIS_LAYER_MAIN;

	if (argc < 3) {
		printf("%s(). parameter is error!\n", __func__);
		return -1;
	}

	while ((opt = getopt(argc, &argv[0], "d:l:")) != EOF) {
		switch (opt) {
		case 'd':
			dis_type = (pbp_dis_type_e)atoi(optarg);
			break;
		case 'l':
			dis_layer = (pbp_dis_layer_e)atoi(optarg);
			break;
		default:
			break;
		}
	}

	pbp_slot_busy_get(dis_type, dis_layer);
	return 0;

}

static int pbp_slot_clear(int argc, char *argv[])
{
	int opt;
	opterr = 0;
	optind = 0;

	pbp_dis_type_e dis_type = PBP_DIS_TYPE_HD;
	pbp_dis_layer_e dis_layer = PBP_DIS_LAYER_MAIN;

	if (argc < 3) {
		printf("%s(). parameter is error!\n", __func__);
		return -1;
	}

	while ((opt = getopt(argc, &argv[0], "d:l:")) != EOF) {
		switch (opt) {
		case 'd':
			dis_type = (pbp_dis_type_e)atoi(optarg);
			break;
		case 'l':
			dis_layer = (pbp_dis_layer_e)atoi(optarg);
			break;
		default:
			break;
		}
	}

	pbp_slot_busy_clear(dis_type, dis_layer);
	return 0;

}

static int pbp_slot_info(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	pbp_slot_busy_status();
	return 0;
}

static const char help_init[] = "enter and init video pbp mode testing...\n";

static const char help_set[] = 
		"Set DE pbp infor to pbp slot\n\t\t\t"
		"set -s 1 -d 1 -l 1\n\t\t\t"
		"-s set pbp mode source: 0, media player; 1. hdmi in; 2. cvbs in; 3. Miracast\n\t\t\t"
		"                        4, air cast; 5. usb aum; 6. usb ium; 7. dlna ...\n\t\t\t"
		"-d set dis type: 0, DIS HD; 1. DIS UHD\n\t\t\t"
		"-l set dis layer: 0, DIS main layer; 1. DIS auxp layer\n\t\t\t"
;

static const char help_get[] = 
		"Get DE pbp infor from pbp slot\n\t\t\t"
		"get -d 1 -l 1\n\t\t\t"
		"-d dis type: 0, DIS HD; 1. DIS UHD\n\t\t\t"
		"-l dis layer: 0, DIS main layer; 1. DIS auxp layer\n\t\t\t"
;

static const char help_clear[] = 
		"Clear pbp infor of pbp slot\n\t\t\t"
		"clear -d 1 -l 1\n\t\t\t"
		"-d dis type: 0, DIS HD; 1. DIS UHD\n\t\t\t"
		"-l dis layer: 0, DIS main layer; 1. DIS auxp layer\n\t\t\t"
;

static const char help_info[] = 
		"Print pbp slot information \n\t\t\t"
		"info \n\t\t\t"
;


#ifdef __linux__

int video_pbp_enter(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	return 0;
}

#include "../hccast_test/hccast_test.h"

static struct termios stored_settings;

void pbp_test_cmds_register(struct console_cmd *cmd)
{
	console_register_cmd(cmd, "init",  video_pbp_init, CONSOLE_CMD_MODE_SELF, help_init);
	console_register_cmd(cmd, "set",  pbp_slot_set, CONSOLE_CMD_MODE_SELF, help_set);
	console_register_cmd(cmd, "get",  pbp_slot_get, CONSOLE_CMD_MODE_SELF, help_get);
	console_register_cmd(cmd, "clear",  pbp_slot_clear, CONSOLE_CMD_MODE_SELF, help_clear);
	console_register_cmd(cmd, "info",  pbp_slot_info, CONSOLE_CMD_MODE_SELF, help_info);
}


#ifndef BR2_PACKAGE_VIDEO_PBP_EXAMPLES

static void pbp_test_exit_console(int signo)
{
    printf("%s(), signo: %d, error: %s\n", __FUNCTION__, signo, strerror(errno));

	tcsetattr (0, TCSANOW, &stored_settings);
    exit(signo);
}

static void pbp_test_signal_normal(int signo)
{
    printf("%s(), signo: %d, error: %s\n", __FUNCTION__, signo, strerror(errno));

    //reset the SIGPIPE, so that it can be catched again
    signal(signo, pbp_test_signal_normal); 
}

int main(int argc , char *argv[])
{
	struct console_cmd *cmd = NULL;
	video_pbp_init(argc, argv);

	struct termios new_settings;
	tcgetattr(0, &stored_settings);
	new_settings = stored_settings;
	//new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
	new_settings.c_lflag &= ~(ICANON | ECHO);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);

    signal(SIGTERM, pbp_test_exit_console); //kill signal
    signal(SIGINT, pbp_test_exit_console); //Ctrl+C signal
    signal(SIGSEGV, pbp_test_exit_console); //segmentation fault, invalid memory
    signal(SIGBUS, pbp_test_exit_console);  //bus error, memory addr is not aligned.
    signal(SIGPIPE, pbp_test_signal_normal);  //SIGPIPE is a disconnect message(TCP), do not need exit app.

	console_init("pbp:");
	pbp_test_cmds_register(NULL);
	console_start();

	pbp_test_exit_console(0);

	return 0;
}
#endif

#else

CONSOLE_CMD(pbp, NULL, video_pbp_init, CONSOLE_CMD_MODE_SELF ,
            "enter and init video pbp mode testing...\n")

CONSOLE_CMD(init, "pbp", video_pbp_init, CONSOLE_CMD_MODE_SELF, help_init);

CONSOLE_CMD(set, "pbp", pbp_slot_set, CONSOLE_CMD_MODE_SELF, help_set);

CONSOLE_CMD(get, "pbp", pbp_slot_get, CONSOLE_CMD_MODE_SELF, help_get);

CONSOLE_CMD(clear, "pbp", pbp_slot_clear, CONSOLE_CMD_MODE_SELF, help_clear);

CONSOLE_CMD(info, "pbp", pbp_slot_info, CONSOLE_CMD_MODE_SELF, help_info);

#endif


