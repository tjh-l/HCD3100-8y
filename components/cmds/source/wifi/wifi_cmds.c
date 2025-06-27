#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <kernel/lib/console.h>
#include <pthread.h>
#include <string.h>
#include <uapi/linux/wireless.h>
#include <wpa_ctrl.h>
#include <errno.h>

struct wpa_params {
	int argc;
	char **argv;
};

static bool g_p2p_thread_running = false;

extern int wpa_supplicant_main(int argc, char *argv[]);
extern int wpa_cli_main(int argc, char *argv[]);
extern int hostapd_main(int argc, char *argv[]);

extern int hostapd_cli_main(int argc, char *argv[]);
extern int iwpriv_main(int argc, char *argv[]);
extern int iwconfig_main(int argc, char *argv[]);
extern int iwlist_main(int argc, char *argv[]);

static pthread_t wpa_supplicant_tid;
static pthread_t hostapd_tid;

static int wifi_entry(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	return 0;
}

static void wpa_params_destroy(struct wpa_params *params)
{
	int i = 0;
	if (params) {
		if (params->argv) {
			for (i = 0; i < params->argc; i++)
				free(params->argv[i]);
			free(params->argv);
		}
		free(params);
	}
}

static struct wpa_params *wpa_params_create(int argc, char **argv)
{
	struct wpa_params *params = (struct wpa_params *)calloc(1, sizeof(struct wpa_params));
	int i = 0;
	if (!params) {
		printf("%s:%d,Not enough memory", __func__, __LINE__);
		return NULL;
	}

	params->argc = argc;
	params->argv = (char **)calloc(argc + 1, sizeof(char *));
	if (!params->argv) {
		printf("%s:%d,Not enough memory", __func__, __LINE__);
		goto fail;
	}
	for (i = 0; i < argc; i++) {
		params->argv[i] = (char *)strdup(argv[i]);
		if (!params->argv[i]) {
			printf("%s:%d,Not enough memory", __func__, __LINE__);
			goto fail;
		}
	}
	params->argv[argc] = NULL;

	return params;
fail:

	wpa_params_destroy(params);
	return NULL;
}

static void *wpa_supplicant_thread(void *args)
{
	struct wpa_params *params = (struct wpa_params *)args;
	wpa_supplicant_main(params->argc, params->argv);
	wpa_params_destroy(params);
	printf("%s:exit\n", __func__);
	return NULL;
}

static void *hostapd_thread(void *args)
{
#ifndef BR2_PACKAGE_PREBUILTS_ECR6600U
	struct wpa_params *params = (struct wpa_params *)args;
	hostapd_main(params->argc, params->argv);
	wpa_params_destroy(params);
#endif
	return NULL;
}

static int wpa_start_helper(int argc, char **argv, pthread_t *tid, void *(*entry)(void *))
{
	pthread_attr_t attr;
	int stack_size = 128 * 1024;
	int ret = 0;
	struct wpa_params *params;
	params = wpa_params_create(argc, argv);
	if (!params) {
		return -1;
	}
	ret = pthread_attr_init(&attr);
	if (ret) {
		printf("Init pthread_attr_t error.\n");
		wpa_params_destroy(params);
		return -1;
	}

	ret = pthread_attr_setstacksize(&attr, stack_size);
	if (ret != 0) {
		printf("Set stack size error.\n");
		wpa_params_destroy(params);
		return -1;
	}

	ret = pthread_create(tid, &attr, entry, params);
	if (ret != 0) {
		printf("Create wpa_supplicant error.\n");
		wpa_params_destroy(params);
		return -1;
	}

	printf("Create thread success.\n");
	return 0;
}

static int wpa_supplicant_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;
	return wpa_start_helper(argc, argv, &wpa_supplicant_tid, wpa_supplicant_thread);
}

static int hostapd_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;
	return wpa_start_helper(argc, argv, &hostapd_tid, hostapd_thread);
}

static int wpa_cli_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;

	return wpa_cli_main(argc, argv);
}

static int hostapd_cli_cmd(int argc, char **argv)
{
#ifdef BR2_PACKAGE_PREBUILTS_ECR6600U
	return 0;
#else
	optind = 0;
	opterr = 0;
	optopt = 0;

	return hostapd_cli_main(argc, argv);
#endif
}

static int iwpriv_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;

	return iwpriv_main(argc, argv);
}

static int iwconfig_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;

	return iwconfig_main(argc, argv);
}

static int iwlist_cmd(int argc, char **argv)
{
	optind = 0;
	opterr = 0;
	optopt = 0;

	return iwlist_main(argc, argv);
}

static int log_level(int argc, char **argv)
{
	extern int wpa_debug_level;
	if (argc < 2) {
		printf("parameters error.\n");
		return -1;
	}
	wpa_debug_level = atoi(argv[1]);

	return 0;
}

static int get_best_channel(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	uint32_t val = 0;
	struct iwreq iwr;
	int ioctl_sock = -1;
	memset(&iwr, 0, sizeof(iwr));
	ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (ioctl_sock < 0) {
		printf("socket[PF_INET,SOCK_DGRAM]");
		return -1;
	}

	strlcpy(iwr.ifr_name, "wlan0", IFNAMSIZ);
	iwr.u.data.pointer = &val;
	iwr.u.data.length = sizeof(val);
	if (ioctl(ioctl_sock, IW_PRIV_IOCTL_BEST_CHANNEL, &iwr) < 0) {
		printf("ioctl[IW_PRIV_IOCTL_BEST_CHANNEL]");
		close(ioctl_sock);
		return -1;
	}
	printf("%s:%d,2.4G: %lu, 5G: %lu\n", __func__, __LINE__, val & 0xFFFF, (val >> 16) & 0xFFFF);
	close(ioctl_sock);

	return 0;
}
static pthread_t g_p2p_thread = 0;
static struct wpa_ctrl *g_cmd_p2p_ifrecv = NULL;
static int64_t cmd_timestamp_listen = 0;

int cmd_p2p_ctrl_wpas_init(void)
{
	printf("Enter %s!\n", __func__);
	char ctrl_iface[64];
	int err = 0;

	if (g_cmd_p2p_ifrecv) {
		return 0;
	}

	snprintf(ctrl_iface, sizeof(ctrl_iface), "%s/%s", "/var/run/wpa_supplicant", "p2p0");

	printf("ctrl_iface: %s\n", ctrl_iface);

	g_cmd_p2p_ifrecv = wpa_ctrl_open(ctrl_iface, 9890);

	if (!g_cmd_p2p_ifrecv) {
		printf("g_p2p_ifrecv open error!\n");
		return -1;
	}

	err = wpa_ctrl_attach(g_cmd_p2p_ifrecv);

	if (err) {
		printf("g_p2p_ifrecv attach error!\n");
		wpa_ctrl_close(g_cmd_p2p_ifrecv);
		g_cmd_p2p_ifrecv = NULL;
		return -1;
	}

	return wpa_ctrl_get_fd(g_cmd_p2p_ifrecv);
}

static void *cmd_p2p_ctrl_thread(void *arg)
{
	// wext no support p2p go mode.
	//udhcpd_start(&g_p2p_udhcpd_conf);
	int fd = (int)arg;
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	char aSend[256] = { 0 };
	char aResp[1024] = { 0 };
	size_t aRespN = 0;
	bool wps_flag = false;
	bool group_started = false;
	struct timeval tv = { 0, 0 };
	int err = 0;
	(void)arg;

	int p2p_state_curr = 0;
	int p2p_state_last = 0;

	while (g_p2p_thread_running) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_usec = 100 * 1000;
		err = select(fd + 1, &fds, NULL, NULL, &tv);

		if (err < 0) {
			if (errno == EINTR) {
				continue;
			}
			printf("select error!\n");
			break;
		}

		if (FD_ISSET(fd, &fds)) {
			aRespN = sizeof aResp - 1;
			err = wpa_ctrl_recv(g_cmd_p2p_ifrecv, aResp, &aRespN);
			if (err) {
				printf("wpa recv error!\n");
				break;
			}

			aResp[aRespN] = '\0';
			printf("[%s][%d] %s\n", __FUNCTION__, __LINE__, aResp);

			if (!strncmp(aResp + 3, P2P_EVENT_GO_NEG_REQUEST, strlen(P2P_EVENT_GO_NEG_REQUEST))) {
				// <3>P2P-GO-NEG-REQUEST 9a:ac:cc:96:2d:6b dev_passwd_id=4 go_intent=15
				printf("EVENT: P2P_EVENT_GO_NEG_REQUEST!\n");
			} else if (!strncmp(aResp + 3, P2P_EVENT_PROV_DISC_PBC_REQ,
					    strlen(P2P_EVENT_PROV_DISC_PBC_REQ))) {
				printf("Event: DISC PBC REQ!\n");
			} else if (!strncmp(aResp + 3, P2P_EVENT_GO_NEG_SUCCESS, strlen(P2P_EVENT_GO_NEG_SUCCESS))) {
				// P2P-GO-NEG-SUCCESS role=client freq=2437 ht40=1 peer_dev=9a:ac:cc:96:2d:6b peer_iface=9a:ac:cc:96:2d:6b wps_method=PBC
				printf("EVENT: P2P_EVENT_GO_NEG_SUCCESS!\n");
			} else if (!strncmp(aResp + 3, P2P_EVENT_INVITATION_ACCEPTED,
					    strlen(P2P_EVENT_INVITATION_ACCEPTED))) {
				// <3>P2P-INVITATION-ACCEPTED sa=9a:ac:cc:96:2d:6b persistent=1
				printf("Event: INVITATION ACCEPTED!\n");
			} else if (strstr(aResp + 3, "Trying to associate")) {
				printf("Event: TRYING TO ASSOCIATE!\n");
			} else if (!strncmp(aResp + 3, P2P_EVENT_GROUP_STARTED, strlen(P2P_EVENT_GROUP_STARTED))) {
				//<3>P2P-GROUP-STARTED p2p0 client ssid="DIRECT-XXX" freq=2462
				// psk=0d8a759a26a6cbe8c6ae7735ba39b4f3ff35b6c6002f3549e94003c34f88ffbe go_dev_addr=02:2e:2d:9d:78:58
				// [PERSISTENT] ip_addr=192.168.137.247 ip_mask=255.255.255.0 go_ip_addr=192.168.137.1
				printf("Event: GROUP STARTED!\n");
			} else if (!strncmp(aResp + 3, WPA_EVENT_CONNECTED, strlen(WPA_EVENT_CONNECTED))) {
				printf("Event: CONNECTED!\n");
			} else if (!strncmp(aResp + 3, WPA_EVENT_DISCONNECTED, strlen(WPA_EVENT_DISCONNECTED))) {
				printf("EVENT: DISCONNECT!\n");
			} else if (!strncmp(aResp + 3, AP_STA_CONNECTED, strlen(AP_STA_CONNECTED))) {
				printf("EVENT: AP-STA-CONNECTED!\n");
			} else if (!strncmp(aResp + 3, AP_STA_DISCONNECTED, strlen(AP_STA_DISCONNECTED))) {
				printf("EVENT: AP-STA-DISCONNECTED!\n");
			} else if (!strncmp(aResp + 3, P2P_EVENT_GROUP_FORMATION_FAILURE,
					    strlen(P2P_EVENT_GROUP_FORMATION_FAILURE))) {
				printf("EVENT: GROUP FORMATION FAILURE!\n");
			} else if (!strncmp(aResp + 3, P2P_EVENT_DEVICE_LOST, strlen(P2P_EVENT_DEVICE_LOST))) {
				printf("EVENT: DEVICE LOST!\n");
			} else if (!strncmp(aResp + 3, WPA_EVENT_TERMINATING, strlen(WPA_EVENT_TERMINATING))) {
				printf("WPA_EVENT_TERMINATING\n");
			} else if (!strncmp(aResp + 3, WPA_EVENT_SCAN_RESULTS, strlen(WPA_EVENT_SCAN_RESULTS))) {
				printf("EVENT: SCAN RESULTS!\n");
			}
		}
	}

	err = wpa_ctrl_detach(g_cmd_p2p_ifrecv);
	wpa_ctrl_close(g_cmd_p2p_ifrecv);
	g_cmd_p2p_ifrecv = NULL;
	g_p2p_thread_running = false;
}

static int p2p_thread_init(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	int fd = cmd_p2p_ctrl_wpas_init(); // need g_mira_param value.
	if (fd <= 0) {
		printf("P2P init failed!\n");
		return fd;
	}

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 0x4000);
	g_p2p_thread_running = true;
	if (pthread_create(&g_p2p_thread, NULL, cmd_p2p_ctrl_thread, (int *)fd) != 0) {
		printf("create p2p thread error!\n");
		return -1;
	}
	return 0;
}

static int p2p_thread_uninit(int argc, char **argv)
{
	g_p2p_thread_running = false;
}

#ifdef CONFIG_CMDS_RTWPRIV
extern int rtwpriv_main(int argc, char *argv[]);
#endif
#ifdef BR2_PACKAGE_PREBUILTS_ECR6600U
int fhost_reoder_discard_flag = 0;
int fhost_reoder_discard_change(int argc, char *argv[])
{
	fhost_reoder_discard_flag = !fhost_reoder_discard_flag;
	printf("fhost_reoder_discard_flag=%d\n", fhost_reoder_discard_flag);
	return 0;
}
CONSOLE_CMD(reoder, "net", fhost_reoder_discard_change, CONSOLE_CMD_MODE_SELF, "fhost_reoder_discard_change")
#endif

CONSOLE_CMD(wifi, NULL, wifi_entry, CONSOLE_CMD_MODE_SELF, "enter wifi test utilities")
CONSOLE_CMD(wpa_supplicant, "wifi", wpa_supplicant_cmd, CONSOLE_CMD_MODE_SELF, "wpa_supplicant daemon")
CONSOLE_CMD(hostapd, "wifi", hostapd_cmd, CONSOLE_CMD_MODE_SELF, "hostapd daemon")
CONSOLE_CMD(wpa_cli, "wifi", wpa_cli_cmd, CONSOLE_CMD_MODE_SELF, "wpa_cli command")
CONSOLE_CMD(hostapd_cli, "wifi", hostapd_cli_cmd, CONSOLE_CMD_MODE_SELF, "hostpad_cli command")
CONSOLE_CMD(iwpriv, "wifi", iwpriv_cmd, CONSOLE_CMD_MODE_SELF, "iwpriv command")
CONSOLE_CMD(iwconfig, "wifi", iwconfig_cmd, CONSOLE_CMD_MODE_SELF, "iwconfig command")
CONSOLE_CMD(iwlist, "wifi", iwlist_cmd, CONSOLE_CMD_MODE_SELF, "iwconfig command")
CONSOLE_CMD(log_level, "wifi", log_level, CONSOLE_CMD_MODE_SELF, "set log level")
CONSOLE_CMD(get_best_channel, "wifi", get_best_channel, CONSOLE_CMD_MODE_SELF, "set log level")
CONSOLE_CMD(p2p_thread, "wifi", p2p_thread_init, CONSOLE_CMD_MODE_SELF, "p2p recv pthread")
CONSOLE_CMD(wpa_supplicant_event_disassoc, "wifi", p2p_thread_uninit, CONSOLE_CMD_MODE_SELF, "p2p recv pthread exit")
#ifdef CONFIG_CMDS_RTWPRIV
CONSOLE_CMD(rtwpriv, NULL, rtwpriv_main, CONSOLE_CMD_MODE_SELF, "rtwpriv tool")
#endif

