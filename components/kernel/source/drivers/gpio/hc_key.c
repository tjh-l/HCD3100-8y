#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <kernel/lib/console.h>
#include <kernel/module.h>

#include <freertos/FreeRTOS.h>
#include <kernel/lib/console.h>
#include <sys/mman.h>
#include <hcuapi/pinpad.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>

#include <kernel/lib/libfdt/libfdt.h>
#include <kernel/lib/fdt_api.h>
#include <linux/slab.h>
#include <kernel/io.h>
#include <kernel/ld.h>

#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

#define KEY_TIME_OUT  		20*100

typedef struct {
	uint32_t  padctl;
	uint32_t  value_status;
	uint32_t  key_code;
} hc_gpiokey_param_s;

typedef struct hc_key_priv
{
	uint32_t			debounce;
	unsigned long			key_jiffies;
	struct timer_list		timer_key;	
	struct input_dev 		*input;
	hc_gpiokey_param_s *key_map;
}hc_key_priv_s;

static void hc_timer_key(struct timer_list *param)
{
	struct hc_key_priv *priv = from_timer(priv, param, timer_key);

	if (priv->key_map->value_status == GPIO_ACTIVE_LOW) {
		if(!gpio_get_input(priv->key_map->padctl)) {
			input_report_key(priv->input, priv->key_map->key_code, 1);
			input_sync(priv->input);
		} else  {
			input_report_key(priv->input, priv->key_map->key_code, 0);
			input_sync(priv->input);
		}
	} else {
		if(gpio_get_input(priv->key_map->padctl)) {
			input_report_key(priv->input, priv->key_map->key_code, 1);
			input_sync(priv->input);
		} else  {
			input_report_key(priv->input, priv->key_map->key_code, 0);
			input_sync(priv->input);
		}
	}

	return;
}

static void key_gpio_irq(uint32_t param)
{
	struct hc_key_priv *priv = (struct hc_key_priv *)param;
	priv->key_jiffies = jiffies + usecs_to_jiffies(priv->debounce * 1000);

	mod_timer(&priv->timer_key, priv->key_jiffies);

	return;
}

static int key_probe(const char *path)
{
	const char *status;
	int ret = 0;
	gpio_pinset_t int_pinset = GPIO_DIR_INPUT;
	u32 key_num;
	u32 debounce = 20;
	struct pinmux_setting *active_state;
	char name[32];

	int np = fdt_node_probe_by_path(path);
	if (np < 0)
		return -1;

	ret = fdt_get_property_string_index(np, "status", 0, &status);
	if(ret != 0)
		return -1;

	if(strcmp(status, "okay"))
		return -1;

	active_state = fdt_get_property_pinmux(np, "active");
	if (active_state != NULL)
		pinmux_select_setting(active_state);
	else
		return -1;

	ret = fdt_get_property_u_32_index(np, "key-num", 0, &key_num);
	ret = fdt_get_property_u_32_index(np, "debounce", 0, &debounce);

	hc_gpiokey_param_s *key_map = kzalloc((sizeof( hc_gpiokey_param_s) * key_num ), GFP_KERNEL);

	fdt_get_property_u_32_array(np, "gpio-key-map",(u32 *)key_map, (key_num) * 3);

	struct hc_key_priv *priv[key_num];

	for (u32 i = 0; i < key_num; i++) {
		priv[i] = kzalloc(sizeof(struct hc_key_priv), GFP_KERNEL);

		priv[i]->key_map = (key_map + i);

		priv[i]->input = input_allocate_device();

		memset(name, 0, sizeof(name));
		snprintf(name, sizeof(name), "gpio%d_key",i);
		priv[i]->input->name = strdup(name);

		input_set_drvdata(priv[i]->input, priv[i]);
		__set_bit(EV_KEY, priv[i]->input->evbit);
		__set_bit(EV_REP, priv[i]->input->evbit);
		set_bit(priv[i]->key_map->key_code, priv[i]->input->keybit);
		ret = input_register_device(priv[i]->input);

		if (priv[i]->key_map->padctl != PINPAD_INVALID) {
			gpio_configure(priv[i]->key_map->padctl,
				       int_pinset | GPIO_IRQ_RISING |
					       GPIO_IRQ_FALLING);
		}
		priv[i]->debounce = debounce;
		ret = gpio_irq_request(priv[i]->key_map->padctl, key_gpio_irq, (uint32_t)priv[i]);
		if (ret < 0)
			return -1;

		timer_setup(&priv[i]->timer_key, hc_timer_key, 0);

		key_gpio_irq((uint32_t)priv[i]);
	}

	return 0;
}

static int hc_key_init(void)
{
	int ret = 0;

	ret |= key_probe("/hcrtos/gpio_key");

	return ret;
}

module_system(hc_key,hc_key_init,NULL,4)
