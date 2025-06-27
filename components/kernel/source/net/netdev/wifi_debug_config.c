#include <string.h>
#include <generated/br2_autoconf.h>
#include <kernel/module.h>
#include <freertos/FreeRTOS.h>

int get_config_wifi_debug_dev_deinit(void) 
{
	#ifdef CONFIG_WIFI_DEV_DEINIT_DEBUG
	return 1;
	#else
	return 0;
	#endif
}

int get_config_wifi_debug_adapter_status(void) 
{
	#ifdef CONFIG_WIFI_ADAPTER_CHK_STATUS
	return 1;
	#else
	return 0;
	#endif
}

int get_config_wifi_debug_recv(void) 
{
	#ifdef CONFIG_WIFI_ADAPTER_RECV_DEBUG
	return 1;
	#else
	return 0;
	#endif
}

int get_config_wifi_debug_rx_mirror_prio(void)
{
	#ifdef CONFIG_WIFI_RX_MIRROR_PRIO
	return CONFIG_WIFI_RX_MIRROR_PRIO;
	#else
	return 15;
	#endif
}

int get_config_wifi_debug_rx_air_prio(void)
{
	#ifdef CONFIG_WIFI_RX_AIR_PRIO
	return CONFIG_WIFI_RX_AIR_PRIO;
	#else
	return 14;
	#endif
}

int get_config_wifi_debug_memlist_num_rx(void)
{
	#ifdef CONFIG_WIFI_MEMLIST_NUM_RX
	return CONFIG_WIFI_MEMLIST_NUM_RX;
	#else
	return 1536;
	#endif
}

int get_config_wifi_debug_memlist_num_tx(void)
{
	#ifdef CONFIG_WIFI_MEMLIST_NUM_TX
	return CONFIG_WIFI_MEMLIST_NUM_TX;
	#else
	return 512;
	#endif
}
