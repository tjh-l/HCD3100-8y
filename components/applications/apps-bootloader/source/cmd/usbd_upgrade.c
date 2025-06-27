#include <generated/br2_autoconf.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/mount.h>
#include <errno.h>
#include <kernel/delay.h>
#include "hcfota.h"
#include <upgrade.h>

static char *ram_path = "/dev/ram0";
static char *mnt_path = "/mnt/ram0";

static int do_usb_device_upgrade(int do_upgrade)
{
	char fota_path[512];
	int ret = -ENOENT;
	struct stat st;

	snprintf(fota_path, sizeof(fota_path), "%s/%s", mnt_path, BR2_EXTERNAL_HCFOTA_FILENAME);
	if (stat(fota_path, &st) == -1)
		return ret;

	if (do_upgrade) {
		printf("==> upgrade from %s\n", fota_path);
		upgrade_progress_init();
		ret = hcfota_url(fota_path, upgrade_progress_report, 0);
		upgrade_progress_exit(ret);
	} else {
		ret = (int)st.st_size;
	}

	return ret;
}

static void remount_delay(int ms)
{
	umount(mnt_path);
	msleep(ms);
	mount(ram_path, mnt_path, "vfat", 0, NULL);
}

int upgrade_from_usb_device(void)
{
#define MAX_FOTA_DETECT_TIMEOUT (3 * 60 * 1000)
	TickType_t xTicksToWait = MAX_FOTA_DETECT_TIMEOUT;
	TimeOut_t xTimeOut;
	int ret = -1;
	int fota_size_pre = 0, fota_size = 0;

	printf("\n\t### USB-Device mode UPGRADE ###\n\n");

	vTaskSetTimeOutState(&xTimeOut);

	while (1) {
		if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) != pdFALSE) {
			printf("HCFOTA not found! Time out in %d seconds!\n", MAX_FOTA_DETECT_TIMEOUT / 1000);
			break;
		}

		fota_size = do_usb_device_upgrade(0);
		if (fota_size > 0) {
			if (fota_size != fota_size_pre) {
				fota_size_pre = fota_size;
				remount_delay(100);
				continue;
			}

			if (do_usb_device_upgrade(1) == 0) {
				ret = 0;
				break;
			} else {
				printf("Upgrade failed!\n");
				fota_size = fota_size_pre = 0;
			}
		}

		printf("HCFOTA not found! Please connect PC and Target USB [port %d], then copy ===<%s>=== into U-Disk on PC!\n",
		       CONFIG_BOOT_USBD_UPGRADE_USBD_PORT_NUM, BR2_EXTERNAL_HCFOTA_FILENAME);
		remount_delay(1000);
	}

	return ret;
}
