#include "usb_cmds_main.h"
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/fs/fs.h>
#include <kernel/drivers/hcusb.h>

char file_path_tmp[512] = {0};

int change_file_attr_test(int argc, char **argv)
{
	struct stat *file_stat;
	int attr_mod = 0;
	int fd;
	int cmd = 0;
	int ret = 0;
	DIR *dir;
	struct stat buf;
	struct tm timeinfo;
	time_t t ;
	struct timeval tv;
	int hid_attr = 0;
	struct timeval times_tmp[2];
	if (argc < 3) {
		printf("input param error\n");
		return -1;
	}
	strcpy(file_path_tmp,argv[1]);
	dir = opendir("/media/sda1/tt");
	if (dir == NULL) {
		printf("Error: please plugin U-disk or SD/TF card\n");
		return -1;
	}
	
	attr_mod = atoi(argv[2]);
	switch(attr_mod) {
		#if 0 //for api test
		case 1:
			ret = chmod_fs(file_path_tmp,AM_ARC,AM_RDO);
			printf("chmod fs ret=%d\n",ret);
			break;
		case 2:
			timeinfo.tm_mday = 21;
			timeinfo.tm_mon = 11;
			timeinfo.tm_year = 2023;
			timeinfo.tm_hour = 12;
			timeinfo.tm_min = 6;
			timeinfo.tm_sec = 10;
			buf.st_ctime = mktime(&timeinfo);
			buf.st_mtime = mktime(&timeinfo);
			ret = chtime_fs(file_path_tmp,&buf);
			printf("chmod fs ret=%d\n",ret);
			break;
		#endif
		case 3:
			chmod(file_path_tmp,0777);
			break;
		case 4:
			timeinfo.tm_mday = 21;
			timeinfo.tm_mon = 11-1;
			timeinfo.tm_year = 2022-1900;
			timeinfo.tm_hour = 12;
			timeinfo.tm_min = 6;
			timeinfo.tm_sec = 10;
			times_tmp[0].tv_sec = mktime(&timeinfo);
			times_tmp[0].tv_usec = 0;
			times_tmp[1].tv_sec = mktime(&timeinfo);
			times_tmp[1].tv_usec = 0;
			utimes(file_path_tmp,times_tmp);
			break;
		case 5:
			chmod(file_path_tmp,0444);
			break;
		#if 0 //for api test
		case 6:
			ret = chmod_fs(file_path_tmp,AM_RDO,AM_ARC|AM_RDO);
			printf("chmod fs ret=%d\n",ret);
			break;
		#endif
		case 7:
			timeinfo.tm_mday = 21;
			timeinfo.tm_mon = 11-1;
			timeinfo.tm_year = 2022-1900;
			timeinfo.tm_hour = 12;
			timeinfo.tm_min = 6;
			timeinfo.tm_sec = 10;
			tv.tv_sec = mktime(&timeinfo);
			tv.tv_usec = 0;
			settimeofday(&tv,NULL);
			t = time(NULL);
			struct tm tmr;
			localtime_r(&t, &tmr);
			break;
		case 8:
			chattr(file_path_tmp,"+e");
			printf("[%s][%d]\n",__FUNCTION__,__LINE__);
			break;
		case 9:
			chattr(file_path_tmp,"-e");
			printf("[%s][%d]\n",__FUNCTION__,__LINE__);
			break;
		case 10:
			lsattr(file_path_tmp,&hid_attr);
			printf("[%s][%d]hid_attr=%d\n",__FUNCTION__,__LINE__,hid_attr);
			break;
		default:
			break;
	}
	return 0;
}

CONSOLE_CMD(chmod_demo, NULL, change_file_attr_test, CONSOLE_CMD_MODE_SELF,
		"chmod_demo /media/sda1/xx/xx.txt 3")


