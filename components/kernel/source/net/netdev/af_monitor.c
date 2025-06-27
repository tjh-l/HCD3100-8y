#include <lwip/sockets.h>

#if LWIP_MONITOR 
#include <net/net_device.h>
#include <linux/mutex.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/netdevice.h>
#include <lwip/netif.h>

static DEFINE_MUTEX(__monitor_mutex);

#define __MONITOR_LOCK()          portENTER_CRITICAL() 
#define __MONITOR_UNLOCK()        portEXIT_CRITICAL()

struct monitor_node {					/*  一个消息节点                */
	struct list_head	head;			/*  待接收数据链                */
	char *if_name;
	/* callback */
	NetifLLMonitorCallback  rcv_cb;
	NetifLLMonitorCallback  send_cb;
	struct net_device *dev;
	struct netif *netif;
	void *opaque;
};

struct list_head __monitor = LIST_HEAD_INIT(__monitor);

int netif_monitor_receive_hook(struct sk_buff *skb)
{
	int ret = MONITOR_SKB_NOT_HANDLE;
	struct list_head *pos;
	struct monitor_node *node;
	__MONITOR_LOCK();
	list_for_each(pos, &__monitor){
		node = (struct monitor_node *)pos;
		if(node->rcv_cb){
			ret = node->rcv_cb(skb, node->opaque);
			if(MONITOR_SKB_EAT == ret){
				ret = MONITOR_SKB_EAT;
				break;
			}
		}
	}
	__MONITOR_UNLOCK();
	return ret;
}


int netif_monitor_send_hook(struct sk_buff *skb)
{
	int ret = MONITOR_SKB_NOT_HANDLE;
	struct list_head *pos;
	struct monitor_node *node;
	__MONITOR_LOCK();
	list_for_each(pos, &__monitor){
		struct net_device *ndev = skb->dev;
		if(!ndev){
			printf("ndev is null\n");
			break;
		}
		node = (struct monitor_node *)pos;
		if(node->send_cb && !strcmp(ndev->name, node->if_name)){
			ret = node->send_cb(skb, node->opaque);
			if(MONITOR_SKB_EAT == ret){
				while(node->dev->netdev_ops->ndo_start_xmit(skb, node->dev) == NETDEV_TX_BUSY){
					usleep(1000*10);
				}
				ret = MONITOR_SKB_EAT;
				break;
			}
		}
	}
	__MONITOR_UNLOCK();
	return ret;
}

void *NetIfLLMonitorConfig(char *if_name, NetifLLMonitorCallback monitor_receive_hook, 
		NetifLLMonitorCallback monitor_send_hook,void *opaque)
{
	struct monitor_node *node;
	struct netif *netiftmp = NULL;
	if(!if_name || !monitor_receive_hook){
		return NULL;
	}
	netiftmp = netifapi_netif_find_by_name(if_name);
	if(!netiftmp){
		printf("ifname %s isn't found\n", if_name);
		return NULL;
	}

	node = (struct monitor_node *)calloc(1, sizeof(struct monitor_node));
	if(!node)
		return NULL;
	node->if_name = strdup(if_name);
	node->rcv_cb = monitor_receive_hook;
	node->send_cb = monitor_send_hook;
	node->dev = (struct net_device *)netiftmp->state;
	node->opaque = opaque;
	node->netif = netiftmp;
	__MONITOR_LOCK();
	list_add_tail(&node->head, &__monitor);
	__MONITOR_UNLOCK();
	return (void *)node;
}

int32_t NetIfLLMonitorDelete(void *config)
{
	struct monitor_node *node = (struct monitor_node *)config;
	if(!node)
		return -EINVAL;
	__MONITOR_LOCK();
	list_del(&node->head);
	if(node->if_name) {
		free(node->if_name);
		node->if_name = NULL;
	}
	__MONITOR_UNLOCK();
	return 0;
}

int32_t NetIfLLMonitorSend(void *config, struct sk_buff *skb)
{
	struct monitor_node *node = (struct monitor_node *)config;
	int ret = 0;
	if(!node)
		return -EINVAL;
	__MONITOR_LOCK();
	if(!node->dev){
		__MONITOR_UNLOCK();
		return -ENODEV;
	}

	//not return util sending success
again:
	ret = node->dev->netdev_ops->ndo_start_xmit(skb, node->dev);
	if(ret == NETDEV_TX_BUSY){
		usleep(1000*10);
		goto again;
	}   
	__MONITOR_UNLOCK();
	return (ret != NETDEV_TX_OK)? -EIO: 0;
}

void *NetifLLMonitorGetDev(void *config)
{
	return ((struct monitor_node *)config)->dev;
}
#endif
