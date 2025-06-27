#ifndef __IP_HOTSPOT_H__
#define __IP_HOTSPOT_H__

struct ip_hdr;
struct pbuf;
struct netif;
	
int hotspot_forward(struct pbuf *p, struct ip_hdr *iphdr, struct netif *inp, struct netif **outp);
void hotspot_recv(struct pbuf *p, struct ip_hdr *iphdr);
void hotspot_init(void);
void hotspot_enable(uint32_t addr, int enable);

int hotspot_start(char *sifname, char *difname);
int hotspot_stop(char *sifname, char *difname);

int hotspot_get_ip4_dns(uint32_t dns[], uint32_t max);
int hotspot_set_ip4_dns(uint32_t dns[], uint32_t max);

#endif
