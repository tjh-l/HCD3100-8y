#include <generated/br2_autoconf.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/lib/console.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/module.h>
#include <hcuapi/gpio.h>
#include <string.h>

int platform_wifi_power_on(void)
{
	int np;
	int pw_gpio = 0xffffffff;
	int pw_active = 0;

	np = fdt_node_probe_by_path("/hcrtos/wifi_pw_enable");
	if (np < 0)
		return 0;

	fdt_get_property_u_32_index(np, "gpio_num", 0, &pw_gpio);
	fdt_get_property_u_32_index(np, "gpio_active", 0, &pw_active);
	if (0xffffffff != (unsigned)pw_gpio) {
		gpio_configure(pw_gpio, GPIO_DIR_OUTPUT);
		gpio_set_output(pw_gpio, pw_active);
	}
	return 0;
}

void platform_wifi_power_off(void)
{
	int np;
	int pw_gpio = 0xffffffff;
	int pw_active = 0;

	np = fdt_node_probe_by_path("/hcrtos/wifi_pw_enable");
	if (np < 0)
		return;

	fdt_get_property_u_32_index(np, "gpio_num", 0, &pw_gpio);
	fdt_get_property_u_32_index(np, "gpio_active", 0, &pw_active);
	
	if (0xffffffff != (unsigned)pw_gpio) {
		gpio_configure(pw_gpio, GPIO_DIR_OUTPUT);
		gpio_set_output(pw_gpio, !pw_active);
	}
}

void __attribute__((weak)) platform_wifi_reset(void)
{
}

__initcall(platform_wifi_power_on);
#if 0
static int wifi_power_on(int argc, char **argv) 
{
	platform_wifi_power_on();
	return 0;
}

static int wifi_power_off(int argc, char **argv) 
{
	platform_wifi_power_off();
	return 0;
}


CONSOLE_CMD(wifi_on, NULL, wifi_power_on_cmd, CONSOLE_CMD_MODE_SELF, "wifi power on")
CONSOLE_CMD(wifi_off, NULL, wifi_power_off_cmd, CONSOLE_CMD_MODE_SELF, "wifi power off")
#endif
