/* serverpacket.c
 *
 * Constuct and send DHCP server packets
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "packet.h"
#include "debug.h"
#include "dhcpd.h"
#include "options.h"
#include "leases.h"

extern struct dhcpOfferedAddr *find_lease_by_chaddr(u_int8_t *chaddr, struct server_config_t *server_config);
extern struct dhcpOfferedAddr *find_lease_by_yiaddr(u_int32_t yiaddr, struct server_config_t *server_config);
extern u_int32_t find_address(int check_expired, struct server_config_t *server_config);
extern struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, unsigned long lease, struct server_config_t *server_config);

/* send a packet to giaddr using the kernel ip stack */
static int send_packet_to_relay(struct dhcpMessage *payload, struct server_config_t *server_config)
{
    DEBUG(LOG_INFO, "Forwarding packet to relay");

    return kernel_packet(payload, server_config->server, SERVER_PORT,
                         payload->giaddr, SERVER_PORT);
}


/* send a packet to a specific arp address and ip address by creating our own ip packet */
static int send_packet_to_client(struct dhcpMessage *payload, int force_broadcast, struct server_config_t *server_config)
{
    unsigned char *chaddr;
    u_int32_t ciaddr;

    if (force_broadcast)
    {
        DEBUG(LOG_INFO, "broadcasting packet to client (NAK)");
        ciaddr = INADDR_BROADCAST;
        chaddr = MAC_BCAST_ADDR;
    }
    else if (payload->ciaddr)
    {
        DEBUG(LOG_INFO, "unicasting packet to client ciaddr");
        ciaddr = payload->ciaddr;
        chaddr = payload->chaddr;
    }
    else if (ntohs(payload->flags) & BROADCAST_FLAG)
    {
        DEBUG(LOG_INFO, "broadcasting packet to client (requested)");
        ciaddr = INADDR_BROADCAST;
        chaddr = MAC_BCAST_ADDR;
    }
    else
    {
        DEBUG(LOG_INFO, "unicasting packet to client yiaddr");
        ciaddr = payload->yiaddr;
        chaddr = payload->chaddr;
    }
    return raw_packet(payload, server_config->server, SERVER_PORT,
                      ciaddr, CLIENT_PORT, chaddr, server_config->ifindex);
}


/* send a dhcp packet, if force broadcast is set, the packet will be broadcast to the client */
static int send_packet(struct dhcpMessage *payload, int force_broadcast, struct server_config_t *server_config)
{
    int ret;

    if (payload->giaddr)
        ret = send_packet_to_relay(payload, server_config);
    else ret = send_packet_to_client(payload, force_broadcast, server_config);
    return ret;
}


static void init_packet(struct dhcpMessage *packet, struct dhcpMessage *oldpacket, char type, struct server_config_t *server_config)
{
    init_header(packet, type);
    packet->xid = oldpacket->xid;
    memcpy(packet->chaddr, oldpacket->chaddr, 16);
    packet->flags = oldpacket->flags;
    packet->giaddr = oldpacket->giaddr;
    packet->ciaddr = oldpacket->ciaddr;
    add_simple_option(packet->options, DHCP_SERVER_ID, server_config->server);
}


/* add in the bootp options */
static void add_bootp_options(struct dhcpMessage *packet, struct server_config_t *server_config)
{
    packet->siaddr = server_config->siaddr;
    if (server_config->sname)
        strncpy(packet->sname, server_config->sname, sizeof(packet->sname) - 1);
    if (server_config->boot_file)
        strncpy(packet->file, server_config->boot_file, sizeof(packet->file) - 1);
}


/* send a DHCP OFFER to a DHCP DISCOVER */
int sendOffer(struct dhcpMessage *oldpacket, struct server_config_t *server_config)
{
    struct dhcpMessage packet;
    struct dhcpOfferedAddr *lease = NULL;
    u_int32_t req_align, lease_time_align = server_config->lease;
    unsigned char *req, *lease_time;
    struct option_set *curr;
    struct in_addr addr;

    init_packet(&packet, oldpacket, DHCPOFFER, server_config);

    /* ADDME: if static, short circuit */
    /* the client is in our lease/offered table */
    if ((lease = find_lease_by_chaddr(oldpacket->chaddr, server_config)))
    {
        if (!lease_expired(lease))
            lease_time_align = lease->expires - time(0);
        packet.yiaddr = lease->yiaddr;

        /* Or the client has a requested ip */
    }
    else if ((req = get_option(oldpacket, DHCP_REQUESTED_IP)) &&

             /* Don't look here (ugly hackish thing to do) */
             memcpy(&req_align, req, 4) &&

             /* and the ip is in the lease range */
             ntohl(req_align) >= ntohl(server_config->start) &&
             ntohl(req_align) <= ntohl(server_config->end) &&

             /* and its not already taken/offered */ /* ADDME: check that its not a static lease */
             ((!(lease = find_lease_by_yiaddr(req_align, server_config)) ||

               /* or its taken, but expired */ /* ADDME: or maybe in here */
               lease_expired(lease))))
    {
        packet.yiaddr = req_align; /* FIXME: oh my, is there a host using this IP? */

        /* otherwise, find a free IP */ /*ADDME: is it a static lease? */
    }
    else
    {
        packet.yiaddr = find_address(0, server_config);

        /* try for an expired lease */
        if (!packet.yiaddr) packet.yiaddr = find_address(1, server_config);
    }

    if (!packet.yiaddr)
    {
        LOG(LOG_WARNING, "no IP addresses to give -- OFFER abandoned");
        return -1;
    }

    if (!add_lease(packet.chaddr, packet.yiaddr, server_config->offer_time, server_config))
    {
        LOG(LOG_WARNING, "lease pool is full -- OFFER abandoned");
        return -1;
    }

    if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME)))
    {
        memcpy(&lease_time_align, lease_time, 4);
        lease_time_align = ntohl(lease_time_align);
        if (lease_time_align > server_config->lease)
            lease_time_align = server_config->lease;
    }

    /* Make sure we aren't just using the lease time from the previous offer */
    if (lease_time_align < server_config->min_lease)
        lease_time_align = server_config->lease;
    /* ADDME: end of short circuit */
    add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));

    curr = server_config->options;
    while (curr)
    {
        if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
            add_option_string(packet.options, curr->data);
        curr = curr->next;
    }

    add_bootp_options(&packet, server_config);

    addr.s_addr = packet.yiaddr;
    LOG(LOG_INFO, "sending OFFER of %s", inet_ntoa(addr));
    return send_packet(&packet, 0, server_config);
}


int sendNAK(struct dhcpMessage *oldpacket, struct server_config_t *server_config)
{
    struct dhcpMessage packet;

    init_packet(&packet, oldpacket, DHCPNAK, server_config);

    LOG(LOG_INFO, "sending NAK");
    return send_packet(&packet, 1, server_config);
}

#include "hccast_dhcpd.h"

int sendACK2(struct dhcpMessage *oldpacket, u_int32_t yiaddr, struct server_config_t *server_config, bool en)
{
    struct dhcpMessage packet;
    struct option_set *curr;
    unsigned char *lease_time;
    u_int32_t lease_time_align = server_config->lease;
    struct in_addr addr;

    init_packet(&packet, oldpacket, DHCPACK, server_config);
    packet.yiaddr = yiaddr;

    if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME)))
    {
        memcpy(&lease_time_align, lease_time, 4);
        lease_time_align = ntohl(lease_time_align);
        if (lease_time_align > server_config->lease)
            lease_time_align = server_config->lease;
        else if (lease_time_align < server_config->min_lease)
            lease_time_align = server_config->lease;
    }

    add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));

    if (server_config->subnet_mask)
    {
        add_simple_option(packet.options, DHCP_SUBNET, server_config->subnet_mask);
    }
    
    curr = server_config->options;
    while (curr)
    {
        if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
            add_option_string(packet.options, curr->data);
        curr = curr->next;
    }

    if (en)
    {
        packet.flags |= 0x80;
        add_dns_option(packet.options, server_config->dns, server_config->dns[DNS_NUM]);
        add_simple_option(packet.options, DHCP_ROUTER, server_config->server);
    }

    add_bootp_options(&packet, server_config);

    addr.s_addr = packet.yiaddr;
    LOG(LOG_INFO, "sending ACK to %s", inet_ntoa(addr));

    if (send_packet(&packet, 0, server_config) < 0)
        return -1;

    add_lease(packet.chaddr, packet.yiaddr, lease_time_align, server_config);

    return 0;
}

int sendACK(struct dhcpMessage *oldpacket, u_int32_t yiaddr, struct server_config_t *server_config)
{
    return sendACK2(oldpacket, yiaddr, server_config, false);
}

int send_inform(struct dhcpMessage *oldpacket, struct server_config_t *server_config)
{
    struct dhcpMessage packet;
    struct option_set *curr;

    init_packet(&packet, oldpacket, DHCPACK, server_config);

    curr = server_config->options;
    while (curr)
    {
        if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
            add_option_string(packet.options, curr->data);
        curr = curr->next;
    }

    add_bootp_options(&packet, server_config);

    return send_packet(&packet, 0, server_config);
}



