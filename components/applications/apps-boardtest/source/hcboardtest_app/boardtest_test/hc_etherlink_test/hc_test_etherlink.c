#define LOG_TAG "hc_test_etherlink"
#define ELOG_OUTPUT_LVL ELOG_LVL_ALL

#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "hc_test_etherlink.h"
#include "app_config.h"



#ifdef BOARDTEST_NET_SUPPORT
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <net/net_device.h>
#endif

#ifdef __HCRTOS__
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <linux/skbuff.h>
#else
#define log_e printf
#define log_w printf
#define log_i printf
#endif

bool test_eth_module = false;
bool test_eth_over = false;
bool test_eth_flag = false;
static char eth_name[16] = {0};

extern uint32_t OsShellDhclient2(int argc, const char **argv);
extern uint32_t osShellPing2(int argc, const char **argv);

#ifdef BOARDTEST_NET_SUPPORT
struct icmp_packet
{
    struct icmphdr hdr;
    char msg[PACKET_SIZE - sizeof(struct icmphdr)];
};

static void set_eth_flags(int s, struct ifreq *ifr)
{
    int ret, saved_errno;
    saved_errno = errno;
    ret = ioctl(s, SIOCSIFFLAGS, ifr);
    if (ret == -1)
    {
        log_e("Interface %s : %s\n", ifr->ifr_name, strerror(errno));
    }
    errno = saved_errno;
}

/**
 * @description: Set eth0 to enable up Or down
 * @return {*}
 */
int hc_test_eth_set_link(char *state)
{
    int sfd;
    short flags;
    struct ifreq ifr;

    if ( (sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        log_e("%s: socket error!\n", __func__);
        return BOARDTEST_FAIL;
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, eth_name, IFNAMSIZ);
    flags = hc_test_eth_get_flags();

    ifr.ifr_flags = flags;

    /* set IFF_UP if cleared */
    if (!(flags & IFF_UP) && !strcmp(state, "up"))
    {
        ifr.ifr_flags |= IFF_UP;
        set_eth_flags(sfd, &ifr);
        log_i("Interface %s : UP set.\n", "eth0");
    }
    else if ((flags & IFF_UP) && !strcmp(state, "down"))
    {
        ifr.ifr_flags &= ~IFF_UP;
        set_eth_flags(sfd, &ifr);
        log_i("Interface %s : DOWN set.\n", "eth0");
    }
    else
    {
        log_e("parameter error\n");
        return BOARDTEST_FAIL;
    }

    flags = ifr.ifr_flags;

    close(sfd);

    return 1;
}

/**
 * @description: Get the name of the Ethernet such as eth0
 * @return {*} Success 1, failure 0
 */
static int hc_test_eth_get_name(void)
{
    int sfd, if_count, i;
    int ret;
    struct ifconf ifc;
    struct ifreq ifr[10];
    char ipaddr[INET_ADDRSTRLEN] = {'\0'};

    memset(&ifc, 0, sizeof(struct ifconf));

    /**/
    if ( (sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        log_e("%s: socket error!\n", __func__);
        return 0;
    }

    ifc.ifc_len = 10 * sizeof(struct ifreq);
    ifc.ifc_buf = (char *)ifr;

    ret = ioctl(sfd, SIOCGIFCONF, (char *)&ifc);
    if (ret == -1)
    {
        log_e("ioctl erron\n");
        return 0;

    }

    if_count = ifc.ifc_len / (sizeof(struct ifreq));
    for (i = 0; i < if_count; i++)
    {
        if (strstr(ifr[i].ifr_name, "eth") != NULL)
        {
            strcpy(eth_name, ifr[i].ifr_name);
            log_i("Interface: %s  \n", ifr[i].ifr_name);
            close(sfd);
            return 1;
        }
    }
    close(sfd);
    return 0;
}

/**
 * @description: Get the status of the Ethernet
 * @param {eth_info} *eth A struct that stores Ethernet information
 * @return {*}Success 1, failure 0
 */
static int hc_test_eth_get_status(eth_info *eth)
{
    int i = 0;
    short flags;
    char eth_status[MAX_STASUS_LENGTH] = {0};

    if (NULL == eth)
    {
        log_e("eth is a null pointer\n");
        return 0;
    }

    if ((flags = hc_test_eth_get_flags()) == 0)
    {
        log_e("flags fail to get\n");
        return 0;
    }
#ifdef __HCRTOS__
    if (flags & IFF_UP)
        strcat(eth_status, "LINK UP ");
    else
        strcat(eth_status, "LINK DOWM ");
#else
    if (flags & IFF_UP)
        strcat(eth_status, "UP ");
    else
        strcat(eth_status, "DOWM ");
#endif
    if (flags & IFF_RUNNING)
        strcat(eth_status, "RUNNING ");
    else
        strcat(eth_status, "STOP ");

    if (flags & IFF_LOOPBACK)
        strcat(eth_status, "LOOPBACK ");

    if (flags & IFF_BROADCAST)
        strcat(eth_status, "BROADCAST ");

    if (flags & IFF_MULTICAST)
        strcat(eth_status, "MULTICAST ");

    if (flags & IFF_PROMISC)
        strcat(eth_status, "PROMISC ");

#ifndef IFF_LOWER_UP
#define IFF_LOWER_UP 0x10000
    if (flags & IFF_LOWER_UP)
        strcat(eth_status, "LOWER_UP ");
#endif
    strncpy(eth->ipstatus, eth_status, sizeof(eth_status));
    log_i("eth status : %s\n", eth->ipstatus);

    return 1;
}

/**
 * @description: Get the flag, as needed in the hc_test_eth_get_status() function
 * @return {*} flags 0 is returned on success
 */
static short hc_test_eth_get_flags(void)
{
    int saved_errno, ret;
    int sfd;
    short if_flags;
    struct ifreq ifr;

    if ( (sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        log_e("%s: socket error!\n", __func__);
        return 0;
    }
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, eth_name, IFNAMSIZ);
    saved_errno = errno;

    ret = ioctl(sfd, SIOCGIFFLAGS, &ifr);
    if (ret == -1 && errno == 19)
    {
        log_e("Interface %s : No such device.\n", eth_name);
        return 0;
    }
    errno = saved_errno;
    if_flags = ifr.ifr_flags;
    close(sfd);
    return if_flags;
}


/**
 * @description: Get Ethernet ip address A structure that stores Ethernet information
 * @param {eth_info} *eth A struct that stores Ethernet information
 * @return {*} Success 1 failure 0
 */
static int hc_test_eth_get_ipaddr(eth_info *eth)
{
    int sfd, saved_errno, ret;
    struct ifreq ifr;
    static char ipaddr[INET_ADDRSTRLEN];

    if (NULL == eth)
    {
        log_e("eth is a null pointer\n");
        return 0;
    }
    memset(eth, 0, sizeof(eth));

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, eth_name, IFNAMSIZ);

    if ( (sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        log_e("%s: socket error!\n", __func__);
        return 0;
    }

    saved_errno = errno;
    ret = ioctl(sfd, SIOCGIFADDR, &ifr);
    if (ret == -1)
    {
        if (errno == 19)
        {
            log_e("Interface %s : No such device.\n", eth_name);
            return 0;
        }
        if (errno == 99)
        {
            log_e("Interface %s : No IPv4 address assigned.\n", eth_name);
            return 0;
        }
    }
    errno = saved_errno;

    inet_ntop(AF_INET, &(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), ipaddr, INET_ADDRSTRLEN);
    strncpy(eth->ipaddr, ipaddr, sizeof(ipaddr));

    log_i("ipaddr = %s\n", eth->ipaddr);
    close(sfd);
    return 1;
}

/**
 * @description: Get the Ethernet mask
 * @param {eth_info} *eth A struct that stores Ethernet information
 * @return {*} Success 1 failure 0
 */
static int hc_test_eth_get_ipmask(eth_info *eth)
{
    int sfd, saved_errno, ret;
    struct ifreq ifr;
    static char ipmask[INET_ADDRSTRLEN];

    if (NULL == eth)
    {
        log_e("eth is a null pointer\n");
        return 0;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, eth_name, IFNAMSIZ);

    if ( (sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        log_e("%s: socket error!\n", __func__);
        return 0;
    }

    saved_errno = errno;
    ret = ioctl(sfd, SIOCGIFNETMASK, &ifr);
    if (ret == -1)
    {
        if (errno == 19)
        {
            log_e("Interface %s : No such device.\n", eth_name);
            return 0;
        }
        if (errno == 99)
        {
            log_e("Interface %s : No IPv4 address assigned.\n", eth_name);
            return 0;
        }
    }
    errno = saved_errno;

    inet_ntop(AF_INET, &(((struct sockaddr_in *)&ifr.ifr_netmask)->sin_addr), ipmask, INET_ADDRSTRLEN);
    strncpy(eth->ipmask, ipmask, sizeof(ipmask));

    close(sfd);
    return 1;
}

/**
 * @description: Get Ethernet other
 * @param {eth_info} *eth A struct that stores Ethernet information
 * @return {*} Success 1 failure 0
 */
static int hc_test_eth_get_other(eth_info *eth)
{
    struct in_addr addr = {0};
    struct ifreq ifr;
    int fd;
    int ret = 0;

    if (NULL == eth)
    {
        log_e("eth is a null pointer\n");
        return 0;
    }
#ifdef __HCRTOS__
    NetIfGetGateway(eth_name, &addr.s_addr);
    strncpy(eth->ipother, inet_ntoa(addr), sizeof(eth->ipother));
#else

    fd =  socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        perror("socket");
        return 0;
    }
    sprintf(ifr.ifr_ifrn.ifrn_name, "%s", "eth0");
    if ((ret = ioctl(fd, SIOCGIFBRDADDR, &ifr)) < 0)
    {
        perror("ioctl");
        close(fd);
        return 0;
    }

    sprintf(eth->ipother, "%d.%d.%d.%d", (uint8_t)ifr.ifr_ifru.ifru_broadaddr.sa_data[2], (uint8_t)ifr.ifr_ifru.ifru_broadaddr.sa_data[3], \
            (uint8_t)ifr.ifr_ifru.ifru_broadaddr.sa_data[4], (uint8_t)ifr.ifr_ifru.ifru_broadaddr.sa_data[5]);

    close(fd);
#endif
    return 1;
}

/**
 * @description: Gets the mac of the Ethernet
 * @param {eth_info} *eth A struct that stores Ethernet information
 * @return {*} Success 1 failure 0

 */
static int hc_test_eth_get_ipmac(eth_info *eth)
{
    int sfd, ret, saved_errno, i;
    unsigned char *mac_addr;
    char temp[34];
    struct ifreq ifr;

    if (NULL == eth)
    {
        log_e("eth is a null pointer\n");
        return 0;
    }

    mac_addr = (unsigned char *)malloc(ETH_ALEN);

    if (NULL == mac_addr)
    {
        log_e("mac_addr malloc null pointer\n");
        return 0;
    }

    if ( (sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        log_e("%s: socket error!\n", __func__);
        return 0;
    }
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, eth_name, IFNAMSIZ);

    saved_errno = errno;
    ret = ioctl(sfd, SIOCGIFHWADDR, &ifr);
    if (ret == -1 && errno == 19)
    {
        log_e("Interface %s : No such device.\n", eth_name);
        return 0;
    }
    errno = saved_errno;

    if (ifr.ifr_addr.sa_family == ARPHRD_LOOPBACK)
    {
        log_e("Interface %s : A Loopback device.\n", eth_name);
        return 0;
    }
    memcpy(mac_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

    sprintf(temp, "%.2x", *mac_addr);
    strcat(eth->ipmac, temp);
    for (i = 1; i < 6; i++)
    {
        sprintf(temp, "%.2x", *(mac_addr + i));
        strcat(eth->ipmac, ":");
        strcat(eth->ipmac, temp);

    }
    free(mac_addr);
    return 1;
}

unsigned short test_checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;

    if (len == 1)
        sum += *(unsigned char *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;

    return result;
}

/**
 * @description: A simple ping command to check connectivity
 * @param {char} *ip_addr ip address to ping
 * @return {*} Success 1 failure 0
 */
static int hc_test_eth_ping(const char *ip_addr)
{
    int sockfd;
    struct sockaddr_in addr;
    struct icmp_packet packet;
    struct icmp_packet response;
    struct timeval start_time, end_time, tv_out;
    double total_time = 0.0;
    int packets_sent = 0, packets_received = 0;
    int addr_len, bytes, msg_count = 255;
    unsigned int i;
    double time_diff;
    tv_out.tv_sec = 5;
    tv_out.tv_usec = 0;

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket error");
        return 0;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = 0;
    addr_len = sizeof(addr);

    memset(&packet, 0, sizeof(packet));
    packet.hdr.type = ICMP_ECHO;
    packet.hdr.un.echo.id = getpid();
    for (i = 0; i < sizeof(packet.msg) - 1; i++)
        packet.msg[i] = i + '0';
    packet.msg[i] = 0;
    packet.hdr.un.echo.sequence = msg_count--;
    packet.hdr.checksum = test_checksum(&packet, sizeof(packet));

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));

    while (packets_sent < MAX_NO_PACKETS)
    {
        if (test_eth_flag == true)
        {
            return 0;
        }

        if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&addr, sizeof(addr)) <= 0)
        {
            perror("sendto error");
            break;
        }
        packets_sent++;
        gettimeofday(&start_time, NULL);
        if ((bytes = recvfrom(sockfd, &response, sizeof(response), 0, (struct sockaddr *)&addr, &addr_len)) <= 0)
        {
            perror("recvfrom error");
            continue;
        }
        packets_received++;

        gettimeofday(&end_time, NULL);
        time_diff = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 + (double)(end_time.tv_usec - start_time.tv_usec) / 1000.0;
        total_time += time_diff;

        printf("Reply from %s: bytes=%d time=%.3fms\n", ip_addr, bytes, time_diff);

        sleep(1);
    }

    log_i("\nPing statistics for %s:\n", ip_addr);
    log_i("    Packets: Sent = %d, Received = %d, Lost = %d (%.1f%% loss),\n", packets_sent, packets_received, \
          packets_sent - packets_received, ((packets_sent - packets_received) / (float)packets_sent) * 100);
    log_i("Approximate round trip times in milli-seconds:\n");
    log_i("    Minimum = %.3fms, Maximum = %.3fms, Average = %.3fms\n", total_time / packets_received, total_time\
          / packets_received, total_time / packets_received);

    close(sockfd);

    if (packets_sent != MAX_NO_PACKETS && packets_received != MAX_NO_PACKETS)
    {
        return 0;
    }

    return 1;
}

/**
 * @description: Get all the data in eth (summary)
 * @param {eth_info} *eth A struct that stores Ethernet information
 * @return {*} Success 1 failure 0
 */
static int hc_test_eth_all(eth_info *eth)
{

    if (NULL == eth)
    {
        log_e("eth is a null pointer\n");
        return 0;
    }

    if (hc_test_eth_get_ipaddr(eth) == 0)
        return 0;

    if (hc_test_eth_get_ipmac(eth) == 0)
        return 0;

    if (hc_test_eth_get_ipmask(eth) == 0)
        return 0;

    if (hc_test_eth_get_other(eth) == 0)
        return 0;

    return 1;
}

#ifdef __linux__
static int castapp_system_cmd(const char *cmd)
{

    pid_t pid;
    if (-1 == (pid = vfork()))
    {
        return 1;
    }
    if (0 == pid)
    {
        execl("/bin/sh", "sh", "-c", cmd, (char *)0);
        return  0;
    }
    else
    {
        wait(&pid);
    }
    return 0;
}
#endif

#endif

/**
 * @description: Ethernet test initialization
 * @return {*}
 */
int hc_test_eth_init(void)
{
    const char *ip_init[20];
    ip_init[0] = "eth0";

    const char *ifconfig_up[20];
    ifconfig_up[0] = "eth0";
    ifconfig_up[1] = "up";

    test_eth_flag = false;

#ifdef BOARDTEST_NET_SUPPORT

#ifdef __HCRTOS__
    if (hc_test_eth_get_name() == 0)
    {
        log_e("No Ethernet device\n");
        return BOARDTEST_FAIL;
    }

    hc_test_eth_set_link("up");
    usleep(1000 * 1000 * 2.5);
    OsShellDhclient2(1, ip_init);

#else
    castapp_system_cmd("udhcpc -t 3 -n");
    if (hc_test_eth_get_name() == 0)
    {
        log_e("No Ethernet device\n");
        write_boardtest_detail(BOARDTEST_ETHERNET_LINK_STATUS, "No Ethernet device");
        return BOARDTEST_FAIL;
    }
#endif

    return BOARDTEST_PASS;

#else
    return BOARDTEST_FAIL;
#endif
}


/**
 * @description: Start of test program
 * @return {*}
 */
int hc_test_eth_start(void)
{
    int ret;
    int i = 0;
    eth_info test_eth = {0};
    const char *ping_ip[20];

#ifdef BOARDTEST_NET_SUPPORT
    ret = hc_test_eth_get_status(&test_eth);


    if (strstr(test_eth.ipstatus, "STOP") != NULL)
    {
        log_e("Status acquisition failure\n");
        test_eth_module = false;
        write_boardtest_detail(BOARDTEST_ETHERNET_LINK_STATUS, "Ethernet is not enabled");
        return BOARDTEST_FAIL;
    }
    test_eth_module = true;

    ret = hc_test_eth_get_ipaddr(&test_eth);
    while (strcmp(test_eth.ipaddr, "0.0.0.0") == 0)
    {
        sleep(1);
        hc_test_eth_get_ipaddr(&test_eth);
        if (i == 4 || test_eth_flag == true)
        {
            write_boardtest_detail(BOARDTEST_ETHERNET_LINK_STATUS, "Allocate IP timeout");
            return BOARDTEST_FAIL;
        }
        i++;
    }

    ret = hc_test_eth_ping(test_eth.ipaddr);

    if (ret <= 0)
    {
        test_eth_over = false;
        write_boardtest_detail(BOARDTEST_ETHERNET_LINK_STATUS, "ping it is timeout");
        return BOARDTEST_FAIL;
    }
    test_eth_over = true;

    log_i("test_eth_module : %d  test_eth_over : %d \n", test_eth_module, test_eth_over);
    return BOARDTEST_PASS;

#endif
}

/**
 * @description: Acquisition of test results
 * @return {*}
 */
int hc_test_eth_exit(void)
{
    eth_info test_res = {0};
    const char *close_ip[20];
    char test_details[128];
    close_ip[0] = "-x";
    close_ip[1] = "eth0";

#ifdef BOARDTEST_NET_SUPPORT

    test_eth_flag = true;
    hc_test_eth_all(&test_res);
    hc_test_eth_get_status(&test_res);
    hc_test_eth_set_link("down");

    if (test_eth_module != true || test_eth_over != true)
    {
        write_boardtest_detail(BOARDTEST_ETHERNET_LINK_STATUS, "STATUS:LINK DOWN");
        return BOARDTEST_FAIL;
    }

    sprintf(test_details, "IPADDR:%s \nSTATUS:%s", test_res.ipaddr, test_res.ipstatus);
    write_boardtest_detail(BOARDTEST_ETHERNET_LINK_STATUS, test_details);

#ifdef __HCRTOS__
    OsShellDhclient2(2, close_ip);
#endif
    test_eth_module = false;
    test_eth_over = false;

    return BOARDTEST_PASS;
#else
    write_boardtest_detail(BOARDTEST_ETHERNET_LINK_STATUS, "not have this function");
    return BOARDTEST_FAIL;
#endif
}



static int hc_boardtest_ETH_auto_register(void)
{
    hc_boardtest_msg_reg_t *test_eth = malloc(sizeof(hc_boardtest_msg_reg_t));


    test_eth->english_name = "ETHERNET_LINK_STATUS";
    test_eth->sort_name = BOARDTEST_ETHERNET_LINK_STATUS;
    test_eth->init = hc_test_eth_init;
    test_eth->run = hc_test_eth_start;
    test_eth->exit = hc_test_eth_exit;
    test_eth->tips = NULL; /*mbox tips*/

    hc_boardtest_module_register(test_eth);

    return 0;
}

__initcall(hc_boardtest_ETH_auto_register)

