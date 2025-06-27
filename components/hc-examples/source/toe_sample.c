#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <pthread.h>

#ifdef __linux__
#include <termios.h>
#include <signal.h>
#include "console.h"
#else
#include <kernel/lib/console.h>
#endif



#define MAX_STASUS_LENGTH 128


static struct termios stored_settings;

static pthread_t toe_st_listen_thread_id = 0;
static char toe_st_listen_running = 0;
static char ifname[IFNAMSIZ] = {'\0'};


static short get_eth_if_flags(char *eth_if)
{
    int saved_errno, ret;
    short if_flags;
    struct ifreq ifr;
	int sfd;
    short flags;
	
	if (!eth_if) {
		return -1;
	} 
	printf("[%s][%d]eth_if=%s\n",__FUNCTION__,__LINE__,eth_if);
    sfd = socket(AF_INET, SOCK_DGRAM, 0);
	
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

    saved_errno = errno;
    ret = ioctl(sfd, SIOCGIFFLAGS, &ifr);
    if (ret == -1 && errno == 19) {
        printf("Interface %s : No such device.\n", eth_if);
        return ret;
    }
    errno = saved_errno;
    if_flags = ifr.ifr_flags;
	close(sfd);
    return if_flags;
}


/* @param {eth_info} *eth 存储以太网信息的结构体
 * @return {*}成功 1 失败 0
 */
int hc_test_eth_get_status(char *eth_if)
{
    int i = 0;
    short flags;
    char eth_status[MAX_STASUS_LENGTH] = {0};

	printf("[%s][%d]eth_if=%s\n",__FUNCTION__,__LINE__,eth_if);
	
    flags = get_eth_if_flags(eth_if);

    if (flags & IFF_UP)
        strcat(eth_status,"LINK UP ");
    else
        strcat(eth_status,"LINK DOWM ");

    if (flags & IFF_RUNNING)
        strcat(eth_status,"RUNNING ");
    else
        strcat(eth_status,"STOP ");

    if (flags & IFF_LOOPBACK)
        strcat(eth_status,"LOOPBACK ");

    if (flags & IFF_BROADCAST)
        strcat(eth_status,"BROADCAST ");

    if (flags & IFF_MULTICAST)
        strcat(eth_status,"MULTICAST ");

    if (flags & IFF_PROMISC)
        strcat(eth_status,"PROMISC ");

#ifndef IFF_LOWER_UP
#define IFF_LOWER_UP 0x10000
    if (flags & IFF_LOWER_UP)
        strcat(eth_status,"LOWER_UP ");
#endif
    printf("eth status : %s\n",eth_status);

    return 0;
}

static void *toe_status_listen_task(void *arg) {
	int ret = 0;
	int i =0;
	toe_st_listen_running = 1;
	while (toe_st_listen_running) {
		hc_test_eth_get_status(ifname);
		usleep(1000*2);
	}
	return NULL;
}

int toe_status_listen_demo(int argc, char **argv) {
	toe_st_listen_running = 0;
	if (argv[1] == NULL) {
		return 0;
	}
	usleep(1000*3);
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 0x2000);
	strncpy(ifname, argv[1], IFNAMSIZ-1);
	if(pthread_create(&toe_st_listen_thread_id, &attr, toe_status_listen_task, NULL)) {
		printf("[%s][%d]pthread_create fail\n",__FUNCTION__,__LINE__);
		return -1;
	}
	return 0;
}

int toe_status_listen_stop(int argc, char **argv) {
	printf("enter toe_status_listen_stop\n");
	toe_st_listen_running = 0;
	return 0;
}

static void exit_console (int signo) {
    (void)signo;
    tcsetattr(0 , TCSANOW , &stored_settings);
    exit(0);
}


int main (int argc , char *argv[]) {
	struct termios new_settings;

    tcgetattr(0 , &stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0 , TCSANOW , &new_settings);

    signal(SIGTERM , exit_console);
    signal(SIGINT , exit_console);
    signal(SIGSEGV , exit_console);
    signal(SIGBUS , exit_console);
    console_init("usb_cmds:");

    console_register_cmd(NULL , "toe_start" , toe_status_listen_demo , CONSOLE_CMD_MODE_SELF , "enter toe status listen test utilities ");
    console_register_cmd(NULL , "toe_stop" , toe_status_listen_stop , CONSOLE_CMD_MODE_SELF , "stop toe status listen test utilities");
	
    console_start();
    exit_console(0);
    (void)argc;
    (void)argv;
    return 0;
}
