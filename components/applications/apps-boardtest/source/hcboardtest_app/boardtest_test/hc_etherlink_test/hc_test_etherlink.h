/*
 * @Author: qiulu.xie qiulu.xie@hichiptech.com
 * @Date: 2023-09-09 14:16:16
 * @LastEditors: qiulu.xie qiulu.xie@hichiptech.com
 * @LastEditTime: 2023-10-12 11:06:00
 * @FilePath: /out/components/applications/apps-boardtest/source/hcboardtest_app/boardtest_test/hc_test_etherlink.h
 * @Description: 
 * 
 * Copyright (c) 2023 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef __HC_TEST_ETHERLINK_H__
#define __HC_TEST_ETHERLINK_H__

//#ifdef WIFI_SUPPORT
#include "boardtest_module.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_STASUS_LENGTH 64
#define PACKET_SIZE 64
#define MAX_WAIT_TIME 5
#define MAX_NO_PACKETS 2

#define MAX_LENGT 16
#define MAC_LENGTH 32

typedef struct etherlink_info
{
    char ipstatus[MAX_STASUS_LENGTH];
    char ipaddr[MAX_LENGT];
    char ipmask[MAX_LENGT];
    char ipother[MAX_LENGT];
    char ipmac[MAC_LENGTH];
}eth_info;

//#define IFNAMSIZ 16
static int hc_test_eth_get_status(eth_info *eth);
static short hc_test_eth_get_flags(void);
static int hc_test_eth_get_name(void);
static int hc_test_eth_get_ipaddr(eth_info *eth);
static int hc_test_eth_get_other(eth_info *eth);
static int hc_test_eth_get_ipmac(eth_info *eth);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
