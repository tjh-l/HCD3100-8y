#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR
#define MODULE_NAME "/dev/backlight"

#include <dt-bindings/gpio/gpio.h>
#include <errno.h>
#include <fcntl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <getopt.h>
#include <hcuapi/backlight.h>
#include <hcuapi/gpio.h>
#include <hcuapi/pinmux.h>
#include <hcuapi/pinpad.h>
#include <kernel/delay.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <kernel/elog.h>
#include <kernel/io.h>
#include <kernel/lib/console.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/module.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <nuttx/pwm/pwm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ctype.h>
#include "lcd_main.h"

/**
 * @author linsen.chen
 * @date 2024-08-20
 * @brief hc backlight function
 * @copyright ZHUHAI HI CHIP SEMICONDUCTOR CO.LTD.
 * @note Only GPIO function dts config

	backlight {
		backlight-gpios-rtos = <PINPAD_T04 GPIO_ACTIVE_HIGH PINPAD_T02 GPIO_ACTIVE_HIGH>;
		default-off;
		status = "okay";
	};

 * @note pwm backlight levels dts config

	pwm@0 {
		pinmux-active = <PINPAD_L00 4>;
		devpath = "/dev/pwm0";
		status = "okay";
	};

	backlight {
		backlight-frequency = <10000>;
		backlight-pwm-polarity = <0>;
		backlight-pwmdev = "/dev/pwm0";
		brightness-levels = <0 4 8 16 32 64 128 255>;
		default-brightness-level = <7>;//max
		// backlight-gpios-rtos = <PINPAD_T04 GPIO_ACTIVE_HIGH>;//Can be added
		default-off;
		status = "okay";
	};

 * @note set backlight range dts config

	pwm@0 {
		pinmux-active = <PINPAD_L00 4>;
		devpath = "/dev/pwm0";
		status = "okay";
	};

	backlight {
		backlight-frequency = <10000>;
		backlight-pwm-polarity = <0>;
		backlight-pwmdev = "/dev/pwm0";
		default-brightness-level = <100>;
		scale = <100>;
		range = "20-100";
		// backlight-gpios-rtos = <PINPAD_T04 GPIO_ACTIVE_HIGH>;//Can be added
		default-off;
		status = "okay";
	};
*/

#define BACKLIGHT_PINPAD_GPIO_NUM 5
#define BACKLIGHT_PWM_SET_STOP	0xFFFF
#define BACKLIGHT_GPIO_ENABLE 1

typedef struct pinpad_gpio {
	u32 padctl;
	bool active;
} pinpad_gpio_s;

typedef struct pinpad_gpio_pack {
	pinpad_gpio_s pad[BACKLIGHT_PINPAD_GPIO_NUM];
	u32 num;
} pinpad_gpio_pack_t;

typedef struct backlight_range {
	unsigned int upper_limit;
	unsigned int lower_limit;
} backlight_range_t;

struct pwm_bl_data {
	pinpad_gpio_pack_t gpio_backlight;
	struct backlight_info info;
	u32 dft_brightness_max;
	const char *pwmbl_path;
	unsigned int *dts_levels;
	unsigned int scale;
	backlight_range_t *range;
	bool is_level_pwm_work;
	int start_flag;
	int lcd_default_off;
};

static struct pwm_bl_data *pbldev = NULL;

static int backlight_set_pwm_duty(u32 duty_cycle)
{
	struct pwm_info_s info = { 0 };
	int fd = 0;
	if (pbldev->pwmbl_path == NULL)
		return 0;

	fd = open(pbldev->pwmbl_path, O_RDWR);
	if (fd <= 0) {
		log_e("open backlight error %s\n", pbldev->pwmbl_path);
		return -1;
	}

	info.period_ns = 1000000000 / pbldev->info.pwm_frequency;
	info.polarity = pbldev->info.pwm_polarity;
	if (duty_cycle == BACKLIGHT_PWM_SET_STOP) {
		ioctl(fd, PWMIOC_STOP, 0);
	} else if (pbldev->is_level_pwm_work) {
		if (duty_cycle >= pbldev->info.levels_count) {
			duty_cycle = pbldev->dft_brightness_max;
		}
		info.duty_ns = info.period_ns * pbldev->dts_levels[duty_cycle] / pbldev->scale;
		ioctl(fd, PWMIOC_SETCHARACTERISTICS, &info);
		ioctl(fd, PWMIOC_START);
	} else {
		if(duty_cycle > pbldev->scale)
			duty_cycle = pbldev->scale;

		if(pbldev->range) {
			if((pbldev->info.levels_count - 1) < duty_cycle) {
				duty_cycle = pbldev->range->upper_limit;
			} else {
				duty_cycle = pbldev->range->lower_limit + duty_cycle;
			}
		}

		info.duty_ns = info.period_ns * duty_cycle / pbldev->scale;
		ioctl(fd, PWMIOC_SETCHARACTERISTICS, &info);
		ioctl(fd, PWMIOC_START);
	}
	close(fd);
	log_d("%s %d info.duty_ns =%ld info.period_ns = %ld duty_cycle = %d\n", __func__, __LINE__, info.duty_ns, info.period_ns, duty_cycle);

	return 0;
}

static void backlight_set_gpio_status(char value)
{
	u32 i = 0;
	for (i = 0; i < pbldev->gpio_backlight.num; i++) {
		gpio_configure(pbldev->gpio_backlight.pad[i].padctl, GPIO_DIR_OUTPUT);
		if (value != 0)
			lcd_gpio_set_output(pbldev->gpio_backlight.pad[i].padctl, !pbldev->gpio_backlight.pad[i].active);
		else
			lcd_gpio_set_output(pbldev->gpio_backlight.pad[i].padctl, pbldev->gpio_backlight.pad[i].active);
	}
	log_d("pbldev->gpio_backlight.pad[i].padctl = %d value = %d\n", pbldev->gpio_backlight.pad[i].padctl, value);
}

static int backlight_io_get_info(struct backlight_info *info)
{
	if (info == NULL)
		return -EFAULT;

	memcpy(info, &pbldev->info, sizeof(struct backlight_info));
	return 0;
}

static int backlight_io_set_info(struct backlight_info *info)
{
	if(info == NULL)
		return -EFAULT;

	pbldev->info.pwm_frequency = info->pwm_frequency;
	pbldev->info.brightness_value = info->brightness_value;
	pbldev->info.pwm_polarity = info->pwm_polarity;
	return 0;
}

static int backlight_io_start(unsigned int val)
{
	pbldev->info.brightness_value = val;
	backlight_set_pwm_duty(pbldev->info.brightness_value);
	backlight_set_gpio_status(pbldev->info.brightness_value);
	pbldev->start_flag = 1;
	return 0;
}

static int backlight_io_stop(void)
{
	backlight_set_pwm_duty(BACKLIGHT_PWM_SET_STOP);
	backlight_set_gpio_status(0);
	return 0;
}

static int backlight_ioctl(FAR struct file *file, int cmd, unsigned long arg)
{
	int ret = 0;
	switch (cmd) {
	case BACKLIGHT_GET_INFO:
		ret = backlight_io_get_info((struct backlight_info *)arg);
		break;
	case BACKLIGHT_SET_INFO:
		ret = backlight_io_set_info((struct backlight_info *)arg);
		break;
	case BACKLIGHT_START:
		backlight_io_start(pbldev->info.brightness_value);
		break;
	case BACKLIGHT_STOP:
		backlight_io_stop();
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int backlight_close(struct file *filep)
{
	return 0;
}

static int backlight_open(struct file *filep)
{
	return 0;
}

static ssize_t backlight_write(struct file *filep, const char *buffer, size_t buflen)
{
	unsigned int buffer_val = 0;
	if (buffer == NULL)
		return -EFAULT;

	log_d("%d %ld\n",(unsigned char) buffer[0], buflen);
	if(buflen > 4)
		buflen = 4;

	for(unsigned int i = buflen; i > 0; i--){
		buffer_val <<= 8;
		buffer_val |= (unsigned char)buffer[i - 1];
	}
	backlight_io_start(buffer_val);
	return buflen;
}

static ssize_t backlight_read(struct file *filep, char *buffer, size_t buflen)
{
	if (buffer == NULL)
		return -EFAULT;

	if(buflen > 4)
		buflen = 4;

	for(unsigned int i = buflen; i > 0; i--) {
		buffer[i - 1] = pbldev->info.brightness_value >> ((i - 1) * 8);
	}

	return buflen;
}

static const struct file_operations backlight_fops = {
	.open = backlight_open,
	.close = backlight_close,
	.read = backlight_read,
	.write = backlight_write,
	.ioctl = backlight_ioctl,
};

static int backlight_probe(const char *node)
{
	int ret;
	u32 tmpVal = 0;
	u32 i = 0;
	const char *path = NULL;
	const char *str_range = NULL;
	int np = fdt_node_probe_by_path(node);

	if (np < 0) {
		goto error;
	}

	if (pbldev != NULL)
		return 0;

	pbldev = (struct pwm_bl_data *)malloc(sizeof(struct pwm_bl_data));
	if (pbldev == NULL) {
		log_e("malloc error\n");
		goto malloc_error;
	}

	memset(pbldev, 0, sizeof(struct pwm_bl_data));

	if (fdt_get_property_data_by_name(np, "backlight-gpios-rtos", &pbldev->gpio_backlight.num) == NULL) {
		pbldev->gpio_backlight.num = 0;
	} else {
		pbldev->info.brightness_value = 1;
	}

	pbldev->gpio_backlight.num >>= 3;

	if (pbldev->gpio_backlight.num > BACKLIGHT_PINPAD_GPIO_NUM)
		pbldev->gpio_backlight.num = BACKLIGHT_PINPAD_GPIO_NUM;

	for (i = 0; i < pbldev->gpio_backlight.num; i++) {
		pbldev->gpio_backlight.pad[i].padctl = PINPAD_INVALID;
		pbldev->gpio_backlight.pad[i].active = GPIO_ACTIVE_HIGH;

		if (fdt_get_property_u_32_index(np, "backlight-gpios-rtos", i * 2, &tmpVal) == 0)
			pbldev->gpio_backlight.pad[i].padctl = tmpVal;
		if (fdt_get_property_u_32_index(np, "backlight-gpios-rtos", i * 2 + 1, &tmpVal) == 0)
			pbldev->gpio_backlight.pad[i].active = tmpVal;
		log_d("%s %d pad = %d active = %d\n", __func__, __LINE__, pbldev->gpio_backlight.pad[i].padctl, pbldev->gpio_backlight.pad[i].active);
	}

	pbldev->dft_brightness_max = 1;
	pbldev->scale = 100;
	pbldev->info.levels_count = 0;
	pbldev->pwmbl_path = NULL;
	pbldev->is_level_pwm_work = 0;
	if (fdt_get_property_string_index(np, "backlight-pwmdev", 0, &path) == 0) {
		pbldev->pwmbl_path = path;
		if (fdt_get_property_u_32_index(np, "brightness-levels", 0, &tmpVal) == 0){
			fdt_get_property_data_by_name(np, "brightness-levels", &pbldev->info.levels_count);
			pbldev->info.levels_count >>= 2;
		}

		if (pbldev->info.levels_count > 0) {
			pbldev->scale = 0;
			pbldev->dts_levels = (unsigned int *)malloc(sizeof(unsigned int) * pbldev->info.levels_count);

			if(!pbldev->dts_levels)
			{
				log_e("malloc error\n");
				goto malloc_free;
			}

			memset(pbldev->dts_levels, 0, sizeof(unsigned int) * pbldev->info.levels_count);
			for (i = 0; i < pbldev->info.levels_count; i++) {
				fdt_get_property_u_32_index(np, "brightness-levels", i, &pbldev->dts_levels[i]);
				log_d("brightness-levels = %d\n", pbldev->dts_levels[i]);
				if (pbldev->dts_levels[i] > pbldev->scale) {
					pbldev->dft_brightness_max = i;
					pbldev->scale = pbldev->dts_levels[i];
				}
			}
			if(pbldev->info.levels_count < BACKLIGHT_LEVEL_SIZE)
				memcpy(pbldev->info.levels, pbldev->dts_levels, pbldev->info.levels_count * sizeof(unsigned int));
			else
				memcpy(pbldev->info.levels, pbldev->dts_levels, BACKLIGHT_LEVEL_SIZE * sizeof(unsigned int));
			pbldev->is_level_pwm_work = 1;
		} else {
			if (fdt_get_property_u_32_index(np, "scale", 0, &tmpVal) == 0) {
				pbldev->scale = tmpVal;
			}
			pbldev->info.levels_count = pbldev->scale + 1;
			fdt_get_property_string_index(np, "range", 0, &str_range);
			if (str_range != NULL) {
				pbldev->range = malloc(sizeof(backlight_range_t));
				if (pbldev->range) {
					unsigned int number[2] = {0};
					int isNumber = 0;
					int number_count = 0;
					for (int i = 0; str_range[i] != '\0'; i++) {
						if (isspace((unsigned char)str_range[i])) {
							continue;
						}
						if (isdigit((unsigned char)str_range[i])) {
							isNumber = 1;
							number[number_count] = number[number_count] * 10 + (str_range[i] - '0');
						} else if (isNumber) {
							isNumber = 0;
							number_count++;
							if(number_count > 1)
								break;
						}
					}
					if(number[0] > number[1])
					{
						pbldev->range->upper_limit = number[0];
						pbldev->range->lower_limit = number[1];
					} else {
						pbldev->range->lower_limit = number[0];
						pbldev->range->upper_limit = number[1];
					}

					log_d("upper_limit: %d lower_limit: %d\n", pbldev->range->upper_limit, pbldev->range->lower_limit);
					if(pbldev->range->upper_limit == 0) {
						log_e("upper_limit and lower_limit = 0\n");
						free(pbldev->range);
						pbldev->range = NULL;
					} else if(pbldev->range->upper_limit > pbldev->scale) {
						log_e("upper_limit > scale\n");
						free(pbldev->range);
						pbldev->range = NULL;
					} else {
						pbldev->info.levels_count = pbldev->range->upper_limit - pbldev->range->lower_limit + 1;
					}
				}
			}
			pbldev->dft_brightness_max = pbldev->info.levels_count - 1;
		}

		tmpVal = pbldev->dft_brightness_max;
		ret = fdt_get_property_u_32_index(np, "default-brightness-level", 0, &tmpVal);
		pbldev->info.default_brightness_level = pbldev->info.brightness_value = tmpVal;

		fdt_get_property_u_32_index(np, "backlight-frequency", 0, &pbldev->info.pwm_frequency);

		tmpVal = 0;
		fdt_get_property_u_32_index(np, "backlight-pwm-polarity", 0, &tmpVal);
		pbldev->info.pwm_polarity = tmpVal;

		log_d("pbldev->pwmbl_path = %s  pbldev->info.levels_count = %d pbldev->info.brightness_value =%d pbldev->scale =%d pbldev->dft_brightness_max = %d\n",
				pbldev->pwmbl_path, pbldev->info.levels_count, pbldev->info.brightness_value, pbldev->scale, pbldev->dft_brightness_max);
	}

	if (pbldev->scale == 0) {
		log_e("pbldev->scale == 0\n");
		goto malloc_free;
	}

	pbldev->lcd_default_off = fdt_property_read_bool(np, "default-off");
	if (pbldev->lcd_default_off)
		goto backlight_register;

	backlight_io_start(pbldev->info.brightness_value);

backlight_register:
	return register_driver(MODULE_NAME, &backlight_fops, 0666, NULL);
malloc_free:
	free(pbldev);
malloc_error:
error:
	pbldev = NULL;

	return 0;
}

static int backlight_init(void)
{
	backlight_probe("/hcrtos/backlight");
	return 0;
}

static int backlight_exit(void)
{
	if (pbldev != NULL)
		free(pbldev);
	unregister_driver(MODULE_NAME);
}

module_driver(backlight, backlight_init, backlight_exit, 2)
