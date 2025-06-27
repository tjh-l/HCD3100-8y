#define LOG_TAG "boot_standby"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/standby.h>
#include <common.h>
#include <kernel/elog.h>
#include <kernel/lib/console.h>
#include <hcuapi/mipi.h>
#include <hcuapi/lvds.h>
#include <kernel/delay.h>
#include <hcuapi/lcd.h>
#include <hcuapi/backlight.h>
#include <hudi/hudi_screen.h>

#ifndef CONFIG_BOOT_STANDBY_DELAY_TIME
#define CONFIG_BOOT_STANDBY_DELAY_TIME 0
#endif

static void boot_standby_pre_process(void)
{
	int fd = -1;
	hudi_handle backlight_hdl = NULL;
	hudi_backlight_info_t bl_dev_info = {0};

	/*close backlight*/
	hudi_backlight_open(&backlight_hdl);
	if (backlight_hdl) {
		hudi_backlight_info_get(backlight_hdl, &bl_dev_info);
		bl_dev_info.brightness_value = 0;
		hudi_backlight_info_set(backlight_hdl, &bl_dev_info);
		hudi_backlight_close(backlight_hdl);
	}

	/*close power pin*/
	fd = open("/dev/lvds", O_RDWR);
	if (fd) {
		ioctl(fd, LVDS_SET_PWM_BACKLIGHT, 0); //lvds set pwm default
		ioctl(fd, LVDS_SET_GPIO_BACKLIGHT, 0); //lvds gpio backlight close
		ioctl(fd, LVDS_SET_GPIO_POWER, 0); //lvds gpio power close
		close(fd);
	}

	fd = open("/dev/mipi", O_RDWR);
	if(fd){
		ioctl(fd, MIPI_DSI_GPIO_ENABLE, 0); //mipi close gpio enable
		close(fd);
	}

	fd = open("/dev/lcddev", O_RDWR); //lcddev close gpio enable
	if (fd) {
		ioctl(fd, LCD_SET_POWER_GPIO, 0);
		ioctl(fd, LCD_SET_PWM_VCOM, 0);
		close(fd);
	}
}

int boot_enter_standby(int argc, char *argv[])
{
	int i;
	int fd_standby;
	int lvds_fd;
	standby_bootup_mode_e temp = 0;

	log_d("enter standby! 1\n");

	fd_standby = open("/dev/standby", O_RDWR);
	if (fd_standby < 0) {
		log_e("Open /dev/standby failed!\n");
		return -1;
	}

	log_d("enter standby! 2\n");

	ioctl(fd_standby, STANDBY_GET_BOOTUP_MODE, &temp);
	if (temp == STANDBY_BOOTUP_COLD_BOOT) {
		boot_standby_pre_process();
		msleep(CONFIG_BOOT_STANDBY_DELAY_TIME);
#if defined(CONFIG_WDT_AUTO_FEED)
		close_watchdog();
#endif
		printf("enter standby!\n");
		ioctl(fd_standby, STANDBY_ENTER, 0);
	}

	close(fd_standby);
	return 0;
}

CONSOLE_CMD(standby, NULL, boot_enter_standby, CONSOLE_CMD_MODE_SELF,
		"boot enter standby")
