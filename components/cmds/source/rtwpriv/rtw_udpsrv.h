/******************************************************************************
 *
 * Copyright(c) 2007 - 2021 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#ifndef __RTW_UDPSRV_H__
#define __RTW_UDPSRV_H__
	
#ifdef __cplusplus  
	extern "C"{   
#endif

#include "rtwpriv.h"
#include "rtw_api.h"

#define udpSrv_version "rtw_5.2_20190329"
#define CONFIG_DBG 1

#define MYPORT 9034                    // the port users will be connecting to
#define BUFLEN   85535                   // length of the buffer
#define BUFFLEN_MAX		85535

#define FLASH_DEVICE_NAME		("/dev/mtd")
#define FLASH_DEVICE_NAME1		("/dev/mtdblock1")
#define HW_SETTING_HEADER_TAG		((char *)"hs")
#define HW_SETTING_OFFSET		0x6000
#define DEFAULT_SETTING_OFFSET		0x8000
#define CURRENT_SETTING_OFFSET		0xc000
//#define IF_NAMING    // if wlan interface is not "wlan0" or "wlan1", turn it on and use str_replace()

#define RTW_IOCTL_MP SIOCDEVPRIVATE

int rtw_udpsrv_start(char* wlanname);
#ifdef __cplusplus  
}  
#endif


#endif

