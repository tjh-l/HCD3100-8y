#ifndef __HC_TEST_DEV__
#define __HC_TEST_DEV__

#define ETHE_MAC_PATH "/sys/class/net/eth0/address"
#define WIFI_MAC_PATH "/sys/class/net/wlan0/address"
#define DDR_REG1  0xb8800070
#define DDR_REG2  0xb8801000
#define MAIN_CPU_REG1 0xb8800070
#define MAIN_CPU_REG2 0xb880007c
#define MAIN_CPU_REG3 0xb8800380
#define SLAVE_CPU_REG1 0xb880009c
#define SLAVE_CPU_REG2 0xb88003b0

//所有测试数据都会以字符串形式返回，如：DDR信息显示正常,ddr:ddr3_128MB_1066MHz
int hc_test_get_product_id(void);
int hc_test_get_firmware_version(void);
int hc_test_get_ddr_info(void);
int hc_test_get_cpu_frequency(void);
int hc_test_get_eth_MAC(void);
int hc_test_get_Wifi_MAC(void);

#endif

