#ifndef _NET_IF_H
#define _NET_IF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/features.h>

#define IF_NAMESIZE 16

struct if_nameindex {
	unsigned int if_index;
	char *if_name;
};

unsigned int if_nametoindex (const char *);
char *if_indextoname (unsigned int, char *);
struct if_nameindex *if_nameindex (void);
void if_freenameindex (struct if_nameindex *);




#if defined(_GNU_SOURCE) || defined(_BSD_SOURCE)

#include <sys/socket.h>

#define IFF_UP	0x1
#define IFF_BROADCAST 0x2
#define IFF_DEBUG 0x4
#define IFF_LOOPBACK 0x8
#define IFF_POINTOPOINT 0x10
#define IFF_NOTRAILERS 0x20
#define IFF_RUNNING 0x40
#define IFF_NOARP 0x80
#define IFF_PROMISC 0x100
#define IFF_ALLMULTI 0x200
#define IFF_MASTER 0x400
#define IFF_SLAVE 0x800
#define IFF_MULTICAST 0x1000
#define IFF_PORTSEL 0x2000
#define IFF_AUTOMEDIA 0x4000
#define IFF_DYNAMIC 0x8000
#define IFF_LOWER_UP 0x10000
#define IFF_DORMANT 0x20000
#define IFF_ECHO 0x40000
#define IFF_VOLATILE (IFF_LOOPBACK|IFF_POINTOPOINT|IFF_BROADCAST| \
        IFF_ECHO|IFF_MASTER|IFF_SLAVE|IFF_RUNNING|IFF_LOWER_UP|IFF_DORMANT)

struct ifaddr {
	struct sockaddr ifa_addr;
	union {
		struct sockaddr	ifu_broadaddr;
		struct sockaddr	ifu_dstaddr;
	} ifa_ifu;
	struct iface *ifa_ifp;
	struct ifaddr *ifa_next;
};

#define ifa_broadaddr	ifa_ifu.ifu_broadaddr
#define ifa_dstaddr	ifa_ifu.ifu_dstaddr

struct ifmap {
	unsigned long int mem_start;
	unsigned long int mem_end;
	unsigned short int base_addr;
	unsigned char irq;
	unsigned char dma;
	unsigned char port;
};

#define IFHWADDRLEN	6
#define IFNAMSIZ	IF_NAMESIZE

struct ifreq {
	union {
		char ifrn_name[IFNAMSIZ];
	} ifr_ifrn;
	union {
		struct sockaddr ifru_addr;
		struct sockaddr ifru_dstaddr;
		struct sockaddr ifru_broadaddr;
		struct sockaddr ifru_netmask;
		struct sockaddr ifru_hwaddr;
		short int ifru_flags;
		int ifru_ivalue;
		int ifru_mtu;
		struct ifmap ifru_map;
		char ifru_slave[IFNAMSIZ];
		char ifru_newname[IFNAMSIZ];
		char *ifru_data;
		struct sockaddr_storage    lifru_addr6;        /* for ipv6 */
	} ifr_ifru;
};

#define ifr_name	ifr_ifrn.ifrn_name
#define ifr_hwaddr	ifr_ifru.ifru_hwaddr
#define ifr_addr	ifr_ifru.ifru_addr
#define ifr_dstaddr	ifr_ifru.ifru_dstaddr
#define ifr_broadaddr	ifr_ifru.ifru_broadaddr
#define ifr_netmask	ifr_ifru.ifru_netmask
#define ifr_flags	ifr_ifru.ifru_flags
#define ifr_metric	ifr_ifru.ifru_ivalue
#define ifr_mtu		ifr_ifru.ifru_mtu
#define ifr_map		ifr_ifru.ifru_map
#define ifr_slave	ifr_ifru.ifru_slave
#define ifr_data	ifr_ifru.ifru_data
#define ifr_ifindex	ifr_ifru.ifru_ivalue
#define ifr_bandwidth	ifr_ifru.ifru_ivalue
#define ifr_qlen	ifr_ifru.ifru_ivalue
#define ifr_newname	ifr_ifru.ifru_newname
#define _IOT_ifreq	_IOT(_IOTS(char),IFNAMSIZ,_IOTS(char),16,0,0)
#define _IOT_ifreq_short _IOT(_IOTS(char),IFNAMSIZ,_IOTS(short),1,0,0)
#define _IOT_ifreq_int	_IOT(_IOTS(char),IFNAMSIZ,_IOTS(int),1,0,0)

struct ifconf {
	int ifc_len;		
	union {
		char *ifcu_buf;
		struct ifreq *ifcu_req;
	} ifc_ifcu;
};

#define ifc_buf		ifc_ifcu.ifcu_buf
#define ifc_req		ifc_ifcu.ifcu_req
#define _IOT_ifconf _IOT(_IOTS(struct ifconf),1,0,0,0,0)

#define __UAPI_DEF_IF_IFCONF                                    0
#define __UAPI_DEF_IF_IFMAP                                     0
#define __UAPI_DEF_IF_IFNAMSIZ                                  0
#define __UAPI_DEF_IF_IFREQ                                     0
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS                          0
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO    0

/* There are two forms of the I/F request structure.
 * One for IPv6 and one for IPv4.
 * Notice that they are (and must be) cast compatible and really different
 * only in the size of the structure allocation.
 *
 * This is the I/F request that should be used with IPv6.
 */

struct lifreq
{
  char                         lifr_name[IFNAMSIZ];  /* Network device name (e.g. "eth0") */
  int16_t                      lifr_ifindex;         /* Interface index */
  union
  {
    struct sockaddr_storage    lifru_addr;           /* IP Address */
    struct sockaddr_storage    lifru_dstaddr;        /* P-to-P Address */
    struct sockaddr_storage    lifru_broadaddr;      /* Broadcast address */
    struct sockaddr_storage    lifru_netmask;        /* Netmask */
    struct sockaddr            lifru_hwaddr;         /* MAC address */
    int                        lifru_count;          /* Number of devices */
    int                        lifru_mtu;            /* MTU size */
    uint32_t                   lifru_flags;          /* Interface flags */
  } lifr_ifru;
};

#define lifr_addr             lifr_ifru.lifru_addr             /* IP address */
#define lifr_dstaddr          lifr_ifru.lifru_dstaddr          /* P-to-P Address */
#define lifr_broadaddr        lifr_ifru.lifru_broadaddr        /* Broadcast address */
#define lifr_netmask          lifr_ifru.lifru_netmask          /* Interface net mask */
#define lifr_hwaddr           lifr_ifru.lifru_hwaddr           /* MAC address */
#define lifr_mtu              lifr_ifru.lifru_mtu              /* MTU */
#define lifr_count            lifr_ifru.lifru_count            /* Number of devices */
#define lifr_flags            lifr_ifru.lifru_flags            /* interface flags */

/* Used only with the SIOCGLIFCONF IOCTL command */

struct lifconf
{
  size_t                      lifc_len;              /* Size of buffer */
  union
  {
#ifndef FAR
#define FAR
#endif
    FAR char                 *lifcu_buf;             /* Buffer address */
    FAR struct lifreq        *lifcu_req;             /* Array of ifreq structures */
  } lifc_ifcu;
};

#define lifc_buf              lifc_ifcu.lifcu_buf    /* Buffer address */
#define lifc_req              lifc_ifcu.lifcu_req    /* Array of ifreq structures */

#endif

#ifdef __cplusplus
}
#endif

#endif
