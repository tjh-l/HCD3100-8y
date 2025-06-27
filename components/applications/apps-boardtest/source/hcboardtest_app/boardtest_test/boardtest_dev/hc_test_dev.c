#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h> 

#include <kernel/io.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/sysdata.h>
#include "hc_test_dev.h"
#include "../boardtest_module.h"
#include <generated/br2_autoconf.h>

#ifdef CONFIG_NET
	#include <sys/socket.h>
	#include <net/if.h>
#endif

struct sysdata sys_data;

//init eth_mac
static int hc_test_init_eth_MAC(char *mac)
{
	#ifndef CONFIG_NET
		return -6;

	#else 
		struct ifreq ifr;
		int skfd;
		if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
		{
			printf("socket error\n");
			return -4;
		}
		strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
		if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0)
		{
			printf( "%s net_get_hwaddr: ioctl SIOCGIFHWADDR\n",__func__);
			close(skfd);
			return -5;
		}
		close(skfd);
		memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
		return 0;
	#endif
}

//init wifi_mac
static int hc_test_init_Wifi_MAC(char *mac)
{
	#ifndef CONFIG_NET
		return -6;

	#else
		struct ifreq ifr;
		int skfd;
		if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
		{
			printf("socket error\n");
			return -4;
		}
		strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ);
		if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0)
		{
			printf( "%s net_get_hwaddr: ioctl SIOCGIFHWADDR\n",__func__);
			close(skfd);
			return -5;
		}
		close(skfd);
		memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
		return 0;
	#endif
}

//initializes a lot of system information include firmware version and product name
static int hc_test_init_id_version(struct sysdata *sys_data)
{
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
	int fd;

	fd = open("/dev/persistentmem", O_RDWR);
	if (fd < 0)
	 {
		printf("Open /dev/persistentmem failed (%d)\n", fd);
		return -1;
	}
    node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
    node.offset = 0;
    node.size = sizeof(struct sysdata);
	node.buf = sys_data;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) 
	{
        new_node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
        new_node.size = sizeof(struct sysdata);
        if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) 
		{
            printf("create sys_data failed\n");
            close(fd);
            return -2;
        }
		node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
		node.offset = 0;
		new_node.size = sizeof(struct sysdata);
		node.buf = sys_data;
		if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) 
		{
			printf("Create&Store app_data failed\n");
			close(fd);
			return -3;
		}
		close(fd);
		return 0;
	}
	close(fd);
	return 0;
}

 /* 
 	@brief get product name
 	@return test finish status, pass or wrong
 */ 
int hc_test_get_product_id(void)
{
	static char buffer [128]={0};
	int get_init_sta=hc_test_init_id_version(&sys_data);
	if(!get_init_sta)
	{
		snprintf(buffer, sizeof(buffer), "%s", sys_data.product_id);
		write_boardtest_detail(BOARDTEST_PRODUTC_ID,buffer);
		return BOARDTEST_PASS;
	}
	else if(-1==get_init_sta)	
		return BOARDTEST_ERROR_OPEN_FILE;
	else if(-2==get_init_sta)
		return BOARDTEST_ERROR_CREATE_DATA;
	else if(-3==get_init_sta)
		return BOARDTEST_ERROR_STORE_DATA;	
}
 /*
 	@brief get firmware_version
 	@return test finish status, pass or wrong
 */ 
int hc_test_get_firmware_version(void)
{
	static char buffer [128];
	int get_init_sta=hc_test_init_id_version(&sys_data);
	if(!get_init_sta)
	{
		snprintf(buffer, sizeof(buffer), "%u",(unsigned int)sys_data.firmware_version);
		write_boardtest_detail(BOARDTEST_FW_VERSION,buffer);
		return BOARDTEST_PASS;
	}
	else if(-1==get_init_sta)	
		return BOARDTEST_ERROR_OPEN_FILE;
	else if(-2==get_init_sta)
		return BOARDTEST_ERROR_CREATE_DATA;
	else if(-3==get_init_sta)
		return BOARDTEST_ERROR_STORE_DATA;
}
 /*
 	@brief get ddr information
 	@return test finish status, pass or wrong
 */ 
int hc_test_get_ddr_info(void)
{
	static char buffer[128]={0};
	int ddr_frequency;
	int ddr_size;
	int flag1=(REG32_READ(DDR_REG1)>>4 & 0x07);
	switch (flag1)
	{
		case 0:
			ddr_frequency=800;
			break;
		case 1:
			ddr_frequency=1066;
			break;
		case 2:
			ddr_frequency=1333;
			break;		
	
		case 4:
			ddr_frequency=576;
			break;
		default:
			ddr_frequency=1600;
			break;				
	}
	int flag2=(REG32_READ(DDR_REG2) & 0x03);
	switch (flag2)
	{
		case 0:
			ddr_size=16;
			break;
		case 1:
			ddr_size=32;
			break;
		case 2:
			ddr_size=64;
			break;		
		case 4:
			ddr_size=256;
			break;
		case 5:
			ddr_size=512;
			break;
		default:
			ddr_size=128;
			break;				
	}
	int ddr_type=(REG32_READ(DDR_REG2)>>23 & 0x01);
	if(ddr_type) //ddr2 or ddr3
		sprintf(buffer,"ddr3_%dMB_%dMHz",ddr_size,ddr_frequency);
	else 
		sprintf(buffer,"ddr2_%dMB_%dMHz",ddr_size,ddr_frequency);

	write_boardtest_detail(BOARDTEST_DDR_DATA,buffer);
	return BOARDTEST_PASS;
}
 /*
 	@brief get cpu frequency
 	@return test finish status, pass or wrong
 */ 
int hc_test_get_cpu_frequency(void)
{
	static char buffer[128]={0};
	#ifdef CONFIG_SOC_HC15XX	//hc15XX
		int M15_CPU_frequency;
		int cpu0_flag1=(REG32_READ(MAIN_CPU_REG1)>>8 & 0x07);
		if(cpu0_flag1==7)//digital clock
		{
			int cpu0_flag2=(REG32_READ(MAIN_CPU_REG2)>>7 & 0x01);
			if(cpu0_flag2==0) M15_CPU_frequency=594;
			else
			{
				int cpu0_flag3=(REG32_READ(MAIN_CPU_REG3));	
				int MCPU_DIG_PLL_M=cpu0_flag3>>16 & 0X3FF;
				int MCPU_DIG_PLL_N=cpu0_flag3>>8  & 0X3F;
				int MCPU_DIG_PLL_L=cpu0_flag3  & 0X3F;
				M15_CPU_frequency=24*(MCPU_DIG_PLL_M+1)/(MCPU_DIG_PLL_N+1)*(MCPU_DIG_PLL_L+1);
			}
		}
		else//simulation clock
		{
			switch (cpu0_flag1)
			{
				case 0:
					M15_CPU_frequency=594;
					break;
				case 1:
					M15_CPU_frequency=396;
					break;
				case 2:
					M15_CPU_frequency=297;
					break;
				case 3:
					M15_CPU_frequency=198;
					break;	
				case 4:
					M15_CPU_frequency=198;
					break;
				case 5:
					M15_CPU_frequency=198;
					break;
				default :
					M15_CPU_frequency=198;
					break;					
			}		
		}
		sprintf(buffer,"cpu_frequency:%dMHz",M15_CPU_frequency);
	#endif

	#ifndef CONFIG_SOC_HC15XX//HC16xx
		int Main_CPU_frequency;	

		int cpu1_flag1=(REG32_READ(MAIN_CPU_REG1)>>8 & 0x07);
		if(cpu1_flag1==7)//digital clock
		{
			int cpu1_flag2=(REG32_READ(MAIN_CPU_REG2)>>7 & 0x01);
			if(cpu1_flag2==0) Main_CPU_frequency=594;
			else
			{
				int cpu1_flag3=(REG32_READ(MAIN_CPU_REG3));	
				int MCPU_DIG_PLL_M=cpu1_flag3>>16 & 0X3FF;
				int MCPU_DIG_PLL_N=cpu1_flag3>>8  & 0X3F;
				int MCPU_DIG_PLL_L=cpu1_flag3  & 0X3F;
				Main_CPU_frequency=24*(MCPU_DIG_PLL_M+1)/(MCPU_DIG_PLL_N+1)*(MCPU_DIG_PLL_L+1);
			}
		}

		else//simulation clock
		{
			switch (cpu1_flag1)
			{
				case 0:
					Main_CPU_frequency=594;
					break;
				case 1:
					Main_CPU_frequency=396;
					break;
				case 2:
					Main_CPU_frequency=297;
					break;
				case 3:
					Main_CPU_frequency=198;
					break;	
				case 4:
					Main_CPU_frequency=900;
					break;
				case 5:
					Main_CPU_frequency=1118;
					break;
				default :
					Main_CPU_frequency=24;
					break;					
			}		
		}
		sprintf(buffer,"cpu_frequency:%dMHz,",Main_CPU_frequency);
	#endif
	write_boardtest_detail(BOARDTEST_CPU_DATA,buffer);
	return BOARDTEST_PASS;
}
 /*
 	@brief get dev eth_MAC
 	@return test finish status,pass or wrong
 */ 
int  hc_test_get_eth_MAC(void)
{
	char eth_mac[6] = {0};
	static char buffer[128]={0};
	int get_init_sta=hc_test_init_eth_MAC(eth_mac);
	if(!get_init_sta)
	{
		sprintf(buffer,"eth_MAC:%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)eth_mac[0], (unsigned char)eth_mac[1],(unsigned char)eth_mac[2],(unsigned char)eth_mac[3], 
		(unsigned char)eth_mac[4],(unsigned char)eth_mac[5]);
		write_boardtest_detail(BOARDTEST_ETH_MAC,buffer);	
		return BOARDTEST_PASS;
	}
	switch(get_init_sta)
	{
		case -1:
			return BOARDTEST_ERROR_PRAR_DATA;
			break;
		case -2:
			return BOARDTEST_ERROR_OPEN_FILE;
			break;
		case -3:
			return BOARDTEST_ERROR_CHECK_DATA;
			break;
		case -4:
			return BOARDTEST_ERROR_SOCKET_DATA;
			break;
		case -5:
			return BOARDTEST_ERROR_IOCTL_DEVICE;
			break;
		case -6:
			sprintf(buffer,"not open network");
			write_boardtest_detail(BOARDTEST_ETH_MAC,buffer);
			return BOARDTEST_FAIL;
			break;
		default :
			break;
	}
}
 /*
 	@brief get device Wifi_MAC
 	@return test finish status,pass or wrong
 */ 
int hc_test_get_Wifi_MAC(void)
{
	char wifi_mac[6] = {0};
	static char buffer[128]={0};
	int get_init_sta=hc_test_init_Wifi_MAC(wifi_mac);
	if(!get_init_sta)
	{
		sprintf(buffer,"eth_MAC:%02x:%02x:%02x:%02x:%02x:%02x", (unsigned char)wifi_mac[0], (unsigned char)wifi_mac[1],(unsigned char)wifi_mac[2],(unsigned char)wifi_mac[3], 
		(unsigned char)wifi_mac[4],(unsigned char)wifi_mac[5]);
		write_boardtest_detail(BOARDTEST_WIFI_MAC,buffer);	
		return BOARDTEST_PASS;
	}
	switch(get_init_sta)
	{
		case -1:
			return BOARDTEST_ERROR_PRAR_DATA;
			break;
		case -2:
			return BOARDTEST_ERROR_OPEN_FILE;
			break;
		case -3:
			return BOARDTEST_ERROR_CHECK_DATA;
			break;
		case -4:
			return BOARDTEST_ERROR_SOCKET_DATA;
			break;
		case -5:
			return BOARDTEST_ERROR_IOCTL_DEVICE;
			break;	
		case -6:
			sprintf(buffer,"not open network");
			write_boardtest_detail(BOARDTEST_WIFI_MAC,buffer);
			return BOARDTEST_FAIL;
			break;
		default :
			break;		
	}
}

/*Invoke the template*/

static int hc_boardtest_FW_VERSION_auto_register(void)
{
	hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));
	if (test == NULL) 
	{
		printf("malloc failed\n");
		return 0;
	}
	/*If it is not needed, please assign a value to NULL*/
	test->english_name = "FW_VERSION"; /*It needs to be consistent with the .ini profile*/
	test->sort_name = BOARDTEST_FW_VERSION;      /*Please go to the header file to find the corresponding name*/
	test->init = NULL;
	test->run = hc_test_get_firmware_version;
	test->exit = NULL;
	test->tips = NULL;
	// test->tips = "Please selsect whether the test item passed or not."; /*mbox tips*/
	hc_boardtest_module_register(test);
	return 0;
}
static int hc_boardtest_FW_PRODUCT_ID_auto_register(void)
{
	hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));
	if (test == NULL) 
	{
		printf("malloc failed\n");
		return 0;
	}
	test->english_name = "PRODUCT_ID"; /*It needs to be consistent with the .ini profile*/
	test->sort_name = BOARDTEST_PRODUTC_ID;      /*Please go to the header file to find the corresponding name*/
	test->init = NULL;
	test->run = hc_test_get_product_id;
	test->exit = NULL;
	test->tips = NULL;
	// test->tips = "Please selsect whether the test item passed or not."; /*mbox tips*/
	hc_boardtest_module_register(test);
	return 0;
}
static int hc_boardtest_FW_WIFI_MAC_auto_register(void)
{
	hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));
	if (test == NULL) 
	{
		printf("malloc failed\n");
		return 0;
	}
	test->english_name = "WIFI_MAC"; /*It needs to be consistent with the .ini profile*/
	test->sort_name = BOARDTEST_WIFI_MAC;      /*Please go to the header file to find the corresponding name*/
	test->init = NULL;
	test->run = hc_test_get_Wifi_MAC;
	test->exit = NULL;
	test->tips = NULL;
	// test->tips = "Please selsect whether the test item passed or not."; /*mbox tips*/
	hc_boardtest_module_register(test);
	return 0;
}
static int hc_boardtest_FW_ETH_MAC_auto_register(void)
{
	hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));
	if (test == NULL) 
	{
		printf("malloc failed\n");
		return 0;
	}
	test->english_name = "ETH_MAC"; /*It needs to be consistent with the .ini profile*/
	test->sort_name = BOARDTEST_ETH_MAC;      /*Please go to the header file to find the corresponding name*/
	test->init = NULL;
	test->run = hc_test_get_eth_MAC;
	test->exit = NULL;
	test->tips = NULL;
	// test->tips = "Please selsect whether the test item passed or not."; /*mbox tips*/
	hc_boardtest_module_register(test);
	return 0;
}
static int hc_boardtest_FW_DDR_auto_register(void)
{
	hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));
	if (test == NULL) 
	{
		printf("malloc failed\n");
		return 0;
	}
	test->english_name = "DDR_DATA"; /*It needs to be consistent with the .ini profile*/
	test->sort_name = BOARDTEST_DDR_DATA;      /*Please go to the header file to find the corresponding name*/
	test->init = NULL;
	test->run = hc_test_get_ddr_info;
	test->exit = NULL;
	test->tips = NULL;
	// test->tips = "Please selsect whether the test item passed or not."; /*mbox tips*/
	hc_boardtest_module_register(test);
	return 0;
}
static int hc_boardtest_FW_CPU_auto_register(void)
{
	hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));
	if (test == NULL) 
	{
		printf("malloc failed\n");
		return 0;
	}
	test->english_name = "CPU_DATA"; /*It needs to be consistent with the .ini profile*/
	test->sort_name = BOARDTEST_CPU_DATA;      /*Please go to the header file to find the corresponding name*/
	test->init = NULL;
	test->run = hc_test_get_cpu_frequency;
	test->exit = NULL;
	test->tips = NULL;
	// test->tips = "Please selsect whether the test item passed or not."; /*mbox tips*/
	hc_boardtest_module_register(test);

}

/*Automatic enrollment*/
__initcall(hc_boardtest_FW_VERSION_auto_register);
__initcall(hc_boardtest_FW_PRODUCT_ID_auto_register);
__initcall(hc_boardtest_FW_WIFI_MAC_auto_register);
__initcall(hc_boardtest_FW_ETH_MAC_auto_register);
__initcall(hc_boardtest_FW_DDR_auto_register);
__initcall(hc_boardtest_FW_CPU_auto_register);
/*----------------------------------------------------------------------------------*/

