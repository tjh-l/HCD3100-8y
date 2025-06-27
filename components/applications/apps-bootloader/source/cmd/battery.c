#include <generated/br2_autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <kernel/delay.h>
#include <common.h>
#include <kernel/lib/console.h>

#if defined(CONFIG_BOOT_SUPPORT_BATTERY_CHECK_C6P2)
static int __is_low_battery(void)
{
	unsigned char adc0_val = 0;
	unsigned char adc1_val = 1;
	unsigned int bat_check_val = 0;
	static int adc_fd0 = -1, adc_fd1 = -1;

	if(adc_fd0 < 0 && adc_fd1 < 0)
	{
		adc_fd0 = open("/dev/queryadc0", O_RDONLY);
		if (adc_fd0 < 0) {
			printf("can't open device %s %d\n", __func__, __LINE__);
			return -1;
		}
		adc_fd1 = open("/dev/queryadc1", O_RDONLY);
		if (adc_fd1 < 0) {
			printf("can't open device %s %d\n", __func__, __LINE__);
			return -1;
		}
	}
	read(adc_fd0, &adc0_val, 1);
	read(adc_fd1, &adc1_val, 1);

	bat_check_val = adc1_val * 2000 / 255;
	// printf("bat_check_val %d adc0_val %d\n", bat_check_val, adc0_val);

	if(adc0_val < 220)
		return 1;

	if(bat_check_val < 1000)
		return 1;

	return 0;
}

int is_low_battery(int quick_check)
{
	int count = 0;
	int rc = 0;

	if (quick_check)
		return __is_low_battery();

	while (count++ < 200) {
		rc |= __is_low_battery();
		msleep(1);
	}

	return rc;
}

const char *get_low_battery_popup_fpath(void)
{
	return "/eromfs/batterylow.bmp.gz";
}
#endif
