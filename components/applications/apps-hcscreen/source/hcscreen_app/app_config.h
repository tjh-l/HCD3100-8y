/*
app_config.h: the global config header file for application
 */
#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#ifdef __linux__
#include <stdbool.h> //bool
#include <stdint.h> //uint32_t
#include <sys/types.h> //uint
#else
#include <generated/br2_autoconf.h>
#endif

#ifdef CONFIG_HOTSPOT
#define HOTSPOT_SUPPORT
#endif

#ifdef CONFIG_APPS_WATCHDOG_SUPPORT
#define WATCHDOG_SUPPORT
#endif

#ifdef CONFIG_APPS_WATCHDOG_TIMEOUT
    #define WATCHDOG_TIMEOUT CONFIG_APPS_WATCHDOG_TIMEOUT
#else
    #define WATCHDOG_TIMEOUT 30000
#endif

#ifdef __HCRTOS__
    #if CONFIG_WDT_AUTO_FEED 
      #define WATCHDOG_KERNEL_FEED
    #endif
#endif

#ifdef CONFIG_APPS_UIBC_SUPPORT
#define UIBC_SUPPORT
#endif

#ifdef CONFIG_AUTO_HTTP_UPGRADE
  #define AUTO_HTTP_UPGRADE
#endif

#ifdef BR2_PACKAGE_APPMANAGER
#define APPMANAGER_SUPPORT
#endif

#ifdef CONFIG_HCAPP_NAME_SCREEN
#define HC_APP_NAME CONFIG_HCAPP_NAME_SCREEN
#else
#define HC_APP_NAME "hcscreen_demo"
#endif

#ifdef CONFIG_HCAPP_NAME_UPGRADE
#define HC_APP_UPGRADE CONFIG_HCAPP_NAME_UPGRADE
#else
#define HC_APP_UPGRADE "upgradeapp"
#endif

#endif
