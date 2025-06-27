/**
* @file
* @brief                hudi standby interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_POWER_H__
#define __HUDI_POWER_H__

#include <hcuapi/standby.h>
#ifdef __HCRTOS__
#include <hcuapi/watchdog.h>
#else
#include <linux/watchdog.h>
#endif
#include "hudi_com.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief       Open a hudi standby module instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_standby_open(hudi_handle *handle);

/**
* @brief       Close a hudi standby module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_standby_close(hudi_handle handle);

/**
* @brief       Set bootup feature for multi-OS, like system, logo etc
* @param[in]   handle  Handle of the instance
* @param[in]   slot    Slot value, refer to the macro in hcuapi/standby.h
* @retval      0       Success
* @retval      other   Error
*/
int hudi_standby_bootup_slot_set(hudi_handle handle, unsigned long slot);

/**
* @brief       Set wakeup by ir
* @param[in]   handle  Handle of the instance
* @param[in]   ir      Pointer to ir setting structure
* @retval      0       Success
* @retval      other   Error
*/
int hudi_standby_ir_wakeup_set(hudi_handle handle, struct standby_ir_setting *ir);

/**
* @brief       Set wakeup by gpio
* @param[in]   handle    Handle of the instance
* @param[in]   pin       GPIO pin to configure for wakeup
* @param[in]   polarity  Low is active
* @retval      0         Success
* @retval      other     Error
*/
int hudi_standby_gpio_wakeup_set(hudi_handle handle, pinpad_e pin, int polarity);

/**
* @brief       Set wakeup by saradc
* @param[in]   handle    Handle of the instance
* @param[in]   channel   Saradc channel to configure for wakeup
* @param[in]   min       Minimum threshold value for wakeup
* @param[in]   max       Maximum threshold value for wakeup
* @retval      0         Success
* @retval      other     Error
*/
int hudi_standby_saradc_wakeup_set(hudi_handle handle, int channel, int min, int max);

/**
* @brief       Set pwroff ddr
* @param[in]   handle    Handle of the instance
* @param[in]   pin       GPIO pin to configure for wakeup
* @param[in]   polarity  Low is active
* @retval      0         Success
* @retval      other     Error
*/
int hudi_standby_ddr_pwroff_set(hudi_handle handle, pinpad_e pin, int polarity);

/**
* @brief       Standby enter
* @param[in]   handle    Handle of the instance
* @retval      0         Success
* @retval      other     Error
*/
int hudi_standby_enter(hudi_handle handle);


/**
* @brief       Open a hudi watchdog module instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_watchdog_open(hudi_handle *handle);

/**
* @brief       Close a hudi watchdog module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_watchdog_close(hudi_handle handle);

/**
* @brief       Set the watchdog mode, only support hcrtos
* @param[in]   handle  Output the handle of the instance
* @param[in]   mode    Watchdog mode, refer to the macro in hcuapi/watchdog.h
* @retval      0       Success
* @retval      other   Error
*/
int hudi_watchdog_mode_set(hudi_handle handle, unsigned int mode);

/**
* @brief       Set the watchdog timeout
* @param[in]   handle   Handle of the instance to be closed
* @param[in]   timeout  Watchdog timeout, in milliseconds
* @retval      0        Success
* @retval      other    Error
*/
int hudi_watchdog_timeout_set(hudi_handle handle, unsigned int timeout);

/**
* @brief       Get the remaining watchdog timeout
* @param[in]   handle   Handle of the instance to be closed
* @param[out]  timeout  Output the remaining watchdog timeout, in milliseconds
* @retval      0        Success
* @retval      other    Error
*/
int hudi_watchdog_timeout_get(hudi_handle handle, unsigned int *timeout);

/**
* @brief       Start the watchdog
* @param[in]   handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_watchdog_start(hudi_handle handle);

/**
* @brief       Stop the watchdog
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_watchdog_stop(hudi_handle handle);

/**
* @brief       Keep alive the watchdog
* @param[in]   handle   Handle of the instance to be closed
* @retval      0        Success
* @retval      other    Error
*/
int hudi_watchdog_alive_keep(hudi_handle handle);

/**
* @brief       Get the watchdog status, only support hclinux
* @param[in]   handle   Handle of the instance to be closed
* @param[out]  status   Output the watchdog status, 0--stop, 1--running
* @retval      0        Success
* @retval      other    Error
*/
int hudi_watchdog_status_get(hudi_handle handle, unsigned int *status);

#ifdef __cplusplus
}
#endif

#endif
