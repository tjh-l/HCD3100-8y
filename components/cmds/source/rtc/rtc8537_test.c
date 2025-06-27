#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/delay.h>
#include <kernel/lib/console.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>

#include <hcuapi/iocbase.h>
#include <uapi/linux/rtc.h>

static const char *device = "/dev/at8563";
static int i =0;

int at8563_test(int argc, char * argv[])
{
    int fd;
    int ret;
    struct rtc_time rtc_tm = { 0 };
    struct rtc_time rtc_tm_temp = { 0 };

    //闰月
    rtc_tm.tm_year = 2024 - 1900; /* 需要设置的年份，需要减1900 */
    rtc_tm.tm_mon = 12 - 1; /* 需要设置的月份,需要确保在0-11范围*/
    rtc_tm.tm_mday = 31; /* 需要设置的日期*/
    rtc_tm.tm_hour = 23; /* 需要设置的时间*/
    rtc_tm.tm_min = 59; /* 需要设置的分钟时间*/
    rtc_tm.tm_sec = 55; /* 需要设置的秒数*/

    fd = open(device, O_RDWR);
    if (fd < 0) {
	    printf("can't open device\n");
	    return -1;
    }

    struct rtc_time rtc_alarm_tm = { 0 };
    struct rtc_time rtc_alarm_tm_temp = { 0 };

    if (i == 0) {
	    /* set rtc time*/
	    if (ioctl(fd, RTC_SET_TIME, &rtc_tm) < 0) {
		    printf("RTC_SET_TIME failed\n");
		    close(fd);
		    return -1;
	    }
	    i++;
    }

    rtc_alarm_tm.tm_year = 0; /* 闹钟忽略年设置*/
    rtc_alarm_tm.tm_mon = 0; /* 闹钟忽略月设置*/
    rtc_alarm_tm.tm_mday = 1; /* 闹钟忽略日期设置*/
    rtc_alarm_tm.tm_hour = 0; /* 需要设置的时间*/
    rtc_alarm_tm.tm_min = 0; /* 需要设置的分钟时间*/
    rtc_alarm_tm.tm_sec = 0; /* 需要设置的秒数,没有按秒触发alam,大于0，会在min+1*/

    /* set alarm time and will enabled alarm  */
    if (ioctl(fd, RTC_ALM_SET, &rtc_alarm_tm) < 0) {
	    printf("RTC_ALM_SET failed\n");
    }
    /* get alarm time */
    if (ioctl(fd, RTC_ALM_READ, &rtc_alarm_tm_temp) < 0) {
	    printf("RTC_ALM_READ failed\n");
    }
    printf("RTC_ALM_READ return (day:%02d) (hour:%02d):(min%02d)\n",
	   rtc_alarm_tm_temp.tm_mday, rtc_alarm_tm_temp.tm_hour,
	   rtc_alarm_tm_temp.tm_min);

#if 0
    /* enable alarm time */
    if (ioctl(fd, RTC_AIE_ON, 1) < 0) {
	    printf("RTC_AIE_ON failed\n");
    }

    /* disable alarm time */
    if (ioctl(fd, RTC_AIE_OFF, 0) < 0) {
	    printf("RTC_AIE_OFF failed\n");
    }
#endif

    /* read rtc time */
    if (ioctl(fd, RTC_RD_TIME, &rtc_tm_temp) < 0) {
	    printf("RTC_RD_TIME failed\n");
	    close(fd);
	    return -1;
    }
    printf("RTC_RD_TIME return %04d-%02d-%02d %02d:%02d:%02d\n",
	   rtc_tm_temp.tm_year + 1900, rtc_tm_temp.tm_mon + 1,
	   rtc_tm_temp.tm_mday, rtc_tm_temp.tm_hour, rtc_tm_temp.tm_min,
	   rtc_tm_temp.tm_sec);

    close(fd);

    return ret;
}

CONSOLE_CMD(at8563_test,NULL,at8563_test,CONSOLE_CMD_MODE_SELF,"rtc at8563")
