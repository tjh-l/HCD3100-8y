#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>		/* for memcpy() et al */
#include <unistd.h>		/* for close() */
#include <sys/socket.h>		/* for "struct sockaddr" et al  */
#include <sys/ioctl.h>		/* for ioctl() */
#include <stddef.h>
#include <fcntl.h>
#include <kernel/lib/console.h>
#include <uapi/linux/wireless.h>

#define RTW_IOCTL_MP SIOCDEVPRIVATE

#define RTL8733_EFUSE_MAC_ADDR 0xd7
#define EFUSE_MAC_ADDR RTL8733_EFUSE_MAC_ADDR

#define BUF_SIZE 0x800

static int wlan_ioctl_mp(
    int skfd,
    char *ifname,
    void *pBuffer,
    unsigned int BufferSize)
{
	int err;
	struct iwreq iwr;

	err = 0;

	memset(&iwr, 0, sizeof(struct iwreq));
	strncpy(iwr.ifr_ifrn.ifrn_name, ifname, strlen(ifname));

	iwr.u.data.pointer = pBuffer;
	iwr.u.data.length = (unsigned short)BufferSize;

	err = ioctl(skfd, RTW_IOCTL_MP, &iwr);

	if (iwr.u.data.length == 0)
		*(char*)pBuffer = 0;

	return err;
}

static int execute_cmd(char *ifname, char *cmd)
{
	int sock;
	char input[BUF_SIZE];
	int err = 0;
	printf("ifname = %s\n", ifname);
	printf("cmd: %s\n", cmd);

	if(!ifname || !cmd)
		return -EINVAL;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0) {
		printf("open socket error.\n");
		return -1;
	}
	err = wlan_ioctl_mp(sock, ifname, cmd, strlen(cmd)+1);

	close(sock);

	if (err < 0) {
		fprintf(stderr, "Interface doesn't accept private ioctl...\n");
		fprintf(stderr, "%s: %s\n", cmd, strerror(errno));
	}

	return err;
}

static int get_mac(int argc, char *argv[])
{
	char *cmd;
	if(argc != 2) {
		printf("usage: %s <ifname>\n", argv[0]);
		return 0;
	}
	//这个大小要足够大，因为这驱动会用这个buffer存返回值
	cmd = malloc(BUF_SIZE);
	if(!cmd)
		return -ENOMEM;
	strcpy(cmd, "efuse_get rmap,d7,6");
	if(execute_cmd(argv[1], cmd) >= 0 && strlen(cmd) != 0) {
		/*
		 * save mac addr string to cmd, format is like "0x00 0xE0 0x4C 0x81 0x89 0xDE"
		 */
		printf("%s:%s\n", argv[1], cmd);
	}
	free(cmd);

	return 0;
}

static void get_random(uint32_t *data, uint32_t len, uint32_t seed)
{
	uint32_t v;
	uint32_t i;
	srandom((int)seed);
	for(i = 0; i < len % 4; i++) {
		v = random() ^ seed + xTaskGetTickCount();
		((uint32_t *)data)[i] = v;
	}
}

static int random_update_mac(int argc, char *argv[])
{
	char mac[32];
	uint32_t seed = xTaskGetTickCount();
	uint32_t data1;
	uint32_t data2;
	uint32_t data3;
	uint32_t data4;
	uint32_t data5;
	uint32_t data6;

	char *cmd;

	if(argc != 2)
		return -EINVAL;

	//这个大小要足够大，因为这驱动会用这个buffer存返回值
	cmd = malloc(BUF_SIZE);
	if(!cmd)
		return -ENOMEM;

	/*
	 * 下面这段代码是为了生成一个随机的mac地址，之所以做这么多次，是因为
	 * hcrtos的随机数是软件生成的，在相同的情况下大概率会生成相似或相同的值
	 * 所以多运行几次
	 */
	usleep(seed % 100);
	get_random(&data1, 1, xTaskGetTickCount());
	usleep(seed % 100);

	seed = data1;
	get_random(&data2, 1, xTaskGetTickCount() + seed);
	usleep(seed % 100);

	seed = data2;
	get_random(&data3, 1, xTaskGetTickCount() + seed);
	usleep(seed % 100);

	seed = data3;
	get_random(&data4, 1, xTaskGetTickCount() + seed);
	usleep(seed % 100);

	seed = data4;
	get_random(&data5, 1, xTaskGetTickCount() + seed);
	usleep(seed % 100);

	seed = data5;
	get_random(&data6, 1, xTaskGetTickCount() + seed);
	usleep(seed % 100);

	sprintf(mac, "%02lx%02lx%02lx%02lx%02lx%02lx",
	        data1&0xFC,
	        data2 & 0xFF,
	        data3 & 0xFF,
	        data4 & 0xFF,
	        data5 & 0xFF,
	        data6 & 0xFF);
	printf("cmd: %s\n", mac);

	sprintf(cmd, "efuse_set wmap,d7,%s", mac);

	if(execute_cmd(argv[1], cmd) >= 0 && strlen(cmd) != 0) {
		/*
		 * save mac addr string to cmd, format is like "0x00 0xE0 0x4C 0x81 0x89 0xDE"
		 */
		printf("%s:%s\n", argv[1], cmd);
	}

	free(cmd);

	return 0;
}

static int set_mac(int argc, char *argv[])
{
	char *cmd;
	if(argc != 3) {
		printf("usage: %s <ifname> <mac addr>\n", argv[0]);
		printf("       for example: %s wlan0 00E04C8189DE\n", argv[0]);
		return 0;
	}
	//这个大小要足够大，因为这驱动会用这个buffer存返回值
	cmd = malloc(BUF_SIZE);
	if(!cmd)
		return -ENOMEM;
	sprintf(cmd, "efuse_set wmap,d7,%s", argv[2]);
	if(execute_cmd(argv[1], cmd) >= 0 && strlen(cmd) != 0) {
		/*
		 * save mac addr string to cmd, format is like "0x00 0xE0 0x4C 0x81 0x89 0xDE"
		 */
		printf("%s:%s\n", argv[1], cmd);
	}
	free(cmd);

	return 0;
}

CONSOLE_CMD(get_mac, NULL, get_mac, CONSOLE_CMD_MODE_SELF, "Get mac address")
CONSOLE_CMD(set_mac, NULL, set_mac, CONSOLE_CMD_MODE_SELF, "Set mac address")
CONSOLE_CMD(mac_random, NULL, random_update_mac, CONSOLE_CMD_MODE_SELF, "By randomly seting mac address")
