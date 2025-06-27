#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/lib/console.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/lcd.h>

int open_boot_lcd_init(int argc, char *argv[])
{
	int fd;
	int tmp = 0;

	fd = open("/dev/lcddev",O_RDWR);
	if(fd > 0)
	{
		/*get lcd init status*/
		ioctl(fd, LCD_GET_INIT_STATUS, &tmp);
		if(tmp == LCD_NOT_INITED)
		{
			ioctl(fd, LCD_INIT);
#ifdef CONFIG_BOOT_LCD_ROTATE
			uint8_t flip_flag = 0;
			if(sys_get_sysdata_flip_mode(&flip_flag)!=0)
			{
				flip_flag=0;
			}
			ioctl(fd, LCD_SET_ROTATE, flip_flag);
#endif
		}
		close(fd);
	}

	return 0;
}

CONSOLE_CMD(boot_lcd, NULL, open_boot_lcd_init, CONSOLE_CMD_MODE_SELF, "boot open lcd init")
