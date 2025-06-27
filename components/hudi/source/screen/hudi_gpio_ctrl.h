/**
* @file
* @brief                hudi get/set the GPIO value
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_GPIO_CTRL_H__
#define __HUDI_GPIO_CTRL_H__

#include <hcuapi/pinpad.h>
#include <stdint.h> //uint8_t

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t hudi_gpio_pinset_t;

#define HUDI_GPIO_DIR_INPUT         (0 << 0)
#define HUDI_GPIO_DIR_OUTPUT        (1 << 0)

#define HUDI_INVALID_VALUE_32       (0xFFFFFFFF)

int hudi_gpio_configure(pinpad_e padctl, hudi_gpio_pinset_t pinset);
void hudi_gpio_output_set(pinpad_e padctl, bool val);
int hudi_gpio_input_get(pinpad_e padctl);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //end of __HUDI_GPIO_CTRL_H__
