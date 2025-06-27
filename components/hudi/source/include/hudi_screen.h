/**
* @file
* @brief                hudi screen interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_SCREEN_H__
#define __HUDI_SCREEN_H__

#include <hcuapi/pq.h>
#include <hcuapi/lcd.h>
#include <hcuapi/lvds.h>
#include <hcuapi/mipi.h>
#include "hudi_com.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HUDI_BACKLIGHT_LEVEL_SIZE            (40)
#define HUDI_BACKLIGHT_PWM_SET_STOP	         0xFFFF

typedef struct
{
    unsigned int levels_count;
    unsigned int pwm_polarity;
    unsigned int pwm_frequency;
    unsigned int brightness_value;
    unsigned int default_brightness_level;
    unsigned int levels[HUDI_BACKLIGHT_LEVEL_SIZE];
} hudi_backlight_info_t;

/**
* @brief       Open a hudi pq module instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_pq_open(hudi_handle *handle);

/**
* @brief       Close a hudi pq module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_pq_close(hudi_handle handle);

/**
* @brief       Set the pq parameters
* @param[in]   handle    Handle of the instance
* @param[in]   pq_param  pq parameters, refer to the struct in hcuapi/pq.h
* @retval      0         Success
* @retval      other     Error
*/
int hudi_pq_set(hudi_handle handle, struct pq_settings *pq_param);

/**
* @brief       Start the pq
* @param[in]   handle  Handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_pq_start(hudi_handle handle);

/**
* @brief       Stop the pq
* @param[in]   handle   Handle of the instance
* @retval      0        Success
* @retval      other    Error
*/
int hudi_pq_stop(hudi_handle handle);


/**
* @brief       Open a hudi mipi module instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_mipi_open(hudi_handle *handle);

/**
* @brief       Close a hudi mipi module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_mipi_close(hudi_handle handle);

/**
* @brief       Set the mipi dsi gpio value
* @param[in]   handle  Handle of the instance
* @param[in]   val     1--disable, 0--enable
* @retval      0       Success
* @retval      other   Error
*/
int hudi_mipi_dsi_gpio_set(hudi_handle handle, int val);


/**
* @brief       Open a hudi backlight module instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_backlight_open(hudi_handle *handle);

/**
* @brief       Close a hudi backlight module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_backlight_close(hudi_handle handle);

/**
* @brief       Get the backlight info
* @param[in]   handle  Handle of the instance
* @param[out]  info    Out data pointer
* @retval      0       Success
* @retval      other   Error
*/
int hudi_backlight_info_get(hudi_handle handle, hudi_backlight_info_t *info);

/**
* @brief       Set the backlight info
* @param[in]   handle  Handle of the instance
* @param[in]   info    Out data pointer
* @retval      0       Success
* @retval      other   Error
*/
int hudi_backlight_info_set(hudi_handle handle, hudi_backlight_info_t *info);

/**
* @brief       Start the backlight
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_backlight_start(hudi_handle handle);


/**
* @brief       Open a hudi lcd module instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_lcd_open(hudi_handle *handle);

/**
* @brief       Close a hudi lcd module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_lcd_close(hudi_handle handle);

/**
* @brief       Set the lcd gpio power value
* @param[in]   handle  Handle of the instance
* @param[in]   val     Param: 0 or 1 as Power-GPIO value
* @retval      0       Success
* @retval      other   Error
*/
int hudi_lcd_gpio_power_set(hudi_handle handle, int val);

/**
* @brief       Set the lcd pwm vcom value
* @param[in]   handle  Handle of the instance
* @param[in]   val     Param: val cycle: 0~100
* @retval      0       Success
* @retval      other   Error
*/
int hudi_lcd_pwm_vcom_set(hudi_handle handle, int val);

/**
* @brief       Set the lcd rotate mdoe
* @param[in]   handle  Handle of the instance to be closed
* @param[in]   mode    Lcd rotate mode, refer to the emum in hcuapi/lcd.h
* @retval      0       Success
* @retval      other   Error
*/
int hudi_lcd_rotate_set(hudi_handle handle, lcd_rotate_type_e mode);


/**
* @brief       Open a hudi lvds module instance
* @param[out]  handle  Output the handle of the instance
* @retval      0       Success
* @retval      other   Error
*/
int hudi_lvds_open(hudi_handle *handle);

/**
* @brief       Close a hudi lvds module instance
* @param[in]   handle  Handle of the instance to be closed
* @retval      0       Success
* @retval      other   Error
*/
int hudi_lvds_close(hudi_handle handle);

/**
* @brief       Set the lvds gpio power value
* @param[in]   handle  Handle of the instance
* @param[in]   val     Param: 0 or 1 as Power-GPIO value
* @retval      0       Success
* @retval      other   Error
*/
int hudi_lvds_gpio_power_set(hudi_handle handle, int val);

/**
* @brief       Set the lvds pwm backlight value
* @param[in]   handle  Handle of the instance
* @param[in]   val     Param: val cycle: 0~100
* @retval      0       Success
* @retval      other   Error
*/
int hudi_lvds_pwm_backlight_set(hudi_handle handle, int val);

/**
* @brief       Set the lvds gpio backlight value
* @param[in]   handle  Handle of the instance to be closed
* @param[in]   val     Param: 0 or 1 as Backlight-GPIO value
* @retval      0       Success
* @retval      other   Error
*/
int hudi_lvds_gpio_backlight_set(hudi_handle handle, int val);

/**
* @brief       Set the lvds gpio out value
* @param[in]   handle  Handle of the instance to be closed
* @param[in]   padctl  GPIO pin
* @param[in]   value   The pin value, 0 or 1
* @retval      0       Success
* @retval      other   Error
*/
int hudi_lvds_gpio_out_set(hudi_handle handle, unsigned int padctl, unsigned char value);

#ifdef __cplusplus
}
#endif

#endif
