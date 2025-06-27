/**
* @file
* @brief                hudi screen interface.
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#ifndef __HUDI_SCREEN_INTER_H__
#define __HUDI_SCREEN_INTER_H__

#define HUDI_PQ_DEV            "/dev/pq"
#define HUDI_LCD_DEV           "/dev/lcddev"
#define HUDI_MIPI_DEV          "/dev/mipi"
#define HUDI_LVDS_DEV          "/dev/lvds"

#define HUDI_BACKLIGHT_PINPAD_GPIO_NUM 5

typedef struct{
    bool active;
    unsigned int padctl;
} hudi_pinpad_gpio_t;

typedef struct
{
    unsigned int num;
    hudi_pinpad_gpio_t pad[HUDI_BACKLIGHT_PINPAD_GPIO_NUM];
} hudi_pinpad_gpio_info_t;

typedef struct
{
    unsigned int lower_limit;
    unsigned int upper_limit;
} hudi_backlight_range_t;

typedef struct
{
    int start_flag;
    int lcd_default_off;
    unsigned int scale;
    unsigned int *dts_levels;
    unsigned int dft_brightness_max;
    char pwmbl_path[32];
    bool is_level_pwm_work;
    hudi_backlight_info_t info;
    hudi_backlight_range_t range;
    hudi_pinpad_gpio_info_t gpio_backlight;
} hudi_backlight_pwm_t;

typedef struct
{
    int inited;
    int fd;
} hudi_pq_instance_t;

typedef struct
{
    int inited;
    int fd;
} hudi_lcd_instance_t;

typedef struct
{
    int inited;
    int fd;
} hudi_mipi_instance_t;

typedef struct
{
    int inited;
    int fd;
} hudi_lvds_instance_t;

typedef struct
{
    int inited;
    int fd;
} hudi_backlight_instance_t;

#endif
