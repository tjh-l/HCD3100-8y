#include "app_config.h"

#ifdef NETWORK_SUPPORT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdbool.h>

#include "com_api.h"
#include "network_ping.h"

#define PACKET_SIZE 64
#define MAX_WAIT_TIME 2
#define MAX_NO_PACKETS 4

typedef struct _app_ping_thread_param_
{
    pthread_t tid;
    bool is_stop;
    bool is_limited;
} app_ping_thread_param;

app_ping_thread_param m_app_ping_parm =
{
    .tid = 0,
    .is_stop = true,
    .is_limited = false,
};

static unsigned short calculateChecksum(unsigned short *addr, int len)
{
    unsigned int sum = 0;
    unsigned short answer = 0;
    unsigned short *w = addr;
    int nleft = len;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return answer;
}

static int sendPingRequest(int sockfd, struct sockaddr_in *dest_addr, int seq)
{
    char buffer[PACKET_SIZE];
    struct icmp *icmp_packet;
    struct timeval start_time;
    int icmp_packet_len;

    icmp_packet = (struct icmp *)buffer;
    memset(icmp_packet, 0, sizeof(struct icmp));

    icmp_packet->icmp_type = ICMP_ECHO;
    icmp_packet->icmp_code = 0;
    icmp_packet->icmp_id = getpid();
    icmp_packet->icmp_seq = seq;
    gettimeofday(&start_time, NULL);
    memcpy(buffer + sizeof(struct icmp), &start_time, sizeof(struct timeval));
    icmp_packet_len = sizeof(struct icmp) + sizeof(struct timeval);

    icmp_packet->icmp_cksum = calculateChecksum((unsigned short *)icmp_packet, icmp_packet_len);

    if (sendto(sockfd, buffer, icmp_packet_len, 0, (struct sockaddr *)dest_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("sendto");
        return -1;
    }

    return 0;
}

static int receivePingReply(int sockfd, struct sockaddr_in *src_addr, int seq, long *rtt)
{
    char buffer[PACKET_SIZE];
    struct ip *ip_packet = (struct ip *)buffer;
    struct icmp *icmp_packet;
    struct timeval start_time, end_time;
    int ip_header_len, /*ip_packet_len,*/ icmp_packet_len;

    gettimeofday(&start_time, NULL);

    while (1)
    {
        fd_set readfds;
        struct timeval timeout;
        int select_result;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 100 * 1000;

        select_result = select(sockfd + 1, &readfds, NULL, NULL, &timeout);

        if (select_result == -1)
        {
            perror("ping select");
            return -1;
        }
        else if (select_result == 0)
        {
            if (m_app_ping_parm.is_stop)
            {
                return 2;
            }

            gettimeofday(&end_time, NULL);

            if (end_time.tv_sec - start_time.tv_sec >= MAX_WAIT_TIME)
            {
                return 1;
            }

            continue;
        }

        if (FD_ISSET(sockfd, &readfds))
        {
            int recv_len = recvfrom(sockfd, buffer, PACKET_SIZE, 0, NULL, NULL);
            gettimeofday(&end_time, NULL);

            ip_header_len = ip_packet->ip_hl << 2;
            //ip_packet_len = ntohs(ip_packet->ip_len);
            icmp_packet = (struct icmp *)(buffer + ip_header_len);
            icmp_packet_len = recv_len - ip_header_len;

            if (icmp_packet->icmp_type  == ICMP_ECHOREPLY && icmp_packet->icmp_id == getpid() && icmp_packet->icmp_seq  == seq)
            {
                *rtt = (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;
                printf("[%d]from %s Reply: packet=%d time=%ldms ttl=%d\n", seq, inet_ntoa(src_addr->sin_addr), icmp_packet_len, *rtt, ip_packet->ip_ttl);
                return 0;
            }
        }
    }
}

/**
 * @brief network ping
 *
 * @param arg: ping addr
 * @return < 0: error; 0: normal; 1: timeout; 2: stop
 */
static int ping(void *arg)
{
    int ret = -1;
    char *addr = (char *)arg;

    if (addr == NULL)
    {
        return ret;
    }

    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;

    hints.ai_socktype = SOCK_RAW;

    hints.ai_protocol = IPPROTO_ICMP;

    int status = getaddrinfo(addr, NULL, &hints, &res);

    if (status != 0)
    {
        printf("Failed to resolve hostname or IP address。\n");
        return ret;
    }

    struct sockaddr_in dest_addr;

    memset(&dest_addr, 0, sizeof(dest_addr));

    dest_addr.sin_family = AF_INET;

    memcpy(&dest_addr, res->ai_addr, sizeof(struct sockaddr_in));

    freeaddrinfo(res);

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sockfd < 0)
    {
        perror("socket");
        return ret;
    }

    int seq = 0, cnt = 0;
    bool fist = true;
    long rtt;

    while (seq < MAX_NO_PACKETS)
    {
        if (cnt++ >= 10 || fist)
        {
            fist = false;
            sendPingRequest(sockfd, &dest_addr, seq);
            ret = receivePingReply(sockfd, &dest_addr, seq, &rtt);

            if (2 == ret || 0 == ret)
            {
                break;
            }

            seq++;
        }

        usleep(100 * 1000);
    }

    close(sockfd);
    return ret;
}

static void *app_ping_thread(void *arg)
{
    m_app_ping_parm.is_stop = false;
    int ret = ping(arg);

    if (1 == ret)
    {
        control_msg_t ctl_msg;
        ctl_msg.msg_type = MSG_TYPE_NETWORK_WIFI_MAY_LIMITED;
        api_control_send_msg(&ctl_msg);
    }

    //m_app_ping_parm.is_stop = true;
    return NULL;
}

void app_ping_stop_thread(void)
{
    if (!m_app_ping_parm.is_stop)
    {
        m_app_ping_parm.is_stop = true;
        pthread_join(m_app_ping_parm.tid, NULL);
    }

    return;
}

int app_ping_start_thread(char *addr)
{
    if (!m_app_ping_parm.is_stop)
    {
        printf("ping thread already running. wait for stop.\n");
        app_ping_stop_thread();
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);

    if (pthread_create(&m_app_ping_parm.tid, &attr, app_ping_thread, addr) != 0)
    {
        printf("crate wifi ping thread fail.\n");
    }

    pthread_attr_destroy(&attr);

    return 0;
}
#endif