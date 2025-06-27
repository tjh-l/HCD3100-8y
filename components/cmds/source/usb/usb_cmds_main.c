#include "usb_cmds_main.h"
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/fs/fs.h>
#include <kernel/drivers/hcusb.h>

int usb_get_cap_cmds(int argc, char **argv)
{
	struct stat info;
	int ret = -1;
	if (argc != 2) {
		printf("=---> Error command\n");
		return -1;
	}
	ret = stat(argv[1], &info);
	if (ret < 0) {
		printf("[%s][%d] err\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (S_ISDIR(info.st_mode)) {
		printf("[error] Cannot get capacity for dirent file (%s)\n",
		       argv[1]);
		return -1;
	} else if (S_ISCHR(info.st_mode)) {
		printf("==> char file:%s, size:%llu MB (%llu bytes)\n", argv[1],
		       info.st_size >> 20, info.st_size);
		return 0;
	} else if (S_ISBLK(info.st_mode)) {
		struct geometry mem;
		int fd = open(argv[1], O_RDONLY, 0);
		if (ioctl(fd, BIOC_GEOMETRY, &mem) < 0) {
			printf("[error] ioctrl error -- path(%s), please check\n",
			       argv[1]);
			return -1;
		}
		if (fd < 0) {
			printf("[error] cannot open path(%s), please check\n",
			       argv[1]);
			return -1;
		}
		printf("==> block file:%s, size: %llu MB (%llu * %lu)\n",
		       argv[1], mem.geo_nsectors >> 11, mem.geo_nsectors,
		       mem.geo_sectorsize);
		close(fd);
		return 0;
	} else {
		printf("[error] Error path(%s), please check\n", argv[1]);
		return -1;
	}
	return 0;
}

int usb_debug_host_cmds(int argc, char **argv)
{
	printf("--> usb#0 debug otg test : host\n");
	// console_run_cmd("usb g_mass_storage -p 0 -s");
	hcusb_set_mode(0, MUSB_HOST);
	hcusb_set_mode(1, MUSB_HOST);
	return 0;
}

CONSOLE_CMD(host, "usb", usb_debug_host_cmds, CONSOLE_CMD_MODE_SELF,
	    "usb debug test for switch to host")

int usb_debug_gadget_cmds(int argc, char **argv)
{
	printf("--> usb#0 debug otg test: gadget\n");
	// console_run_cmd("usb g_mass_storage -p 0 /dev/ram0");
	hcusb_set_mode(0, MUSB_PERIPHERAL);
	hcusb_set_mode(1, MUSB_PERIPHERAL);
	return 0;
}

CONSOLE_CMD(gadget, "usb", usb_debug_gadget_cmds, CONSOLE_CMD_MODE_SELF,
	    "usb debug test for OTG switch")

#ifdef CONFIG_CMDS_USB_EYE_PATTERN

int usb_1_debug_eye_test(int argc, char **argv)
{
	printf("--> usb#1 eye test\n");
	*((uint8_t *)0xB8850001) = 0x23;
	*((uint8_t *)0xB8850380) = 0x01;
	usleep(10);
	*((uint8_t *)0xB8850001) = 0x20;
	usleep(1000);
	*((uint8_t *)0xB8850380) = 0x00;
	usleep(1);
	*((uint8_t *)0xB8800087) = 0x20;
	usleep(1);
	*((uint8_t *)0xB8800087) = 0x00;
	usleep(1);
	*((uint8_t *)0xB8850380) = 0x40; // ok, set usb1 iddig = 1;
	*((uint8_t *)0xB8850001) = 0x60;
	// rick pattern
	usleep(100000); //delay 100ms for usb init
	//config usb phy

	*((uint8_t *)0xB8845000) = 0x1f;
	usleep(1); // [2:0] always enable pre-emphasis
	*((uint8_t *)0xB8845002) = 0x64;
	usleep(1); // [6:4] HS eye tuning
	*((uint8_t *)0xB8845003) = 0xfc;
	usleep(1); //[6:2] odt, default:0xd4,0xfc:fastest rise time; 0xc0:slowest rise time. bit7 default is 1
	*((uint8_t *)0xB8845005) = 0xbc;
	usleep(1); // [4:2] TX HS pre_emphasize stength

	*((uint8_t *)0xB885000F) = 0x01;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0x0000;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0x0000;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0x0000;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0x0000;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xAA00;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xAAAA;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xAAAA;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xAAAA;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xEEAA;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xEEEE;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xEEEE;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xEEEE;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xFEEE;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xFFFF;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xFFFF;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xFFFF;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xFFFF;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xFFFF;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0x7FFF;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xDFBF;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xF7EF;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xFDFB;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0x7EFC;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xDFBF;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xF7EF;
	usleep(1);
	*((uint16_t *)0xB8850020) = 0xFDFB;
	usleep(1);
	*((uint8_t *)0xB8850020) = 0x7e;
	usleep(1);

	*((uint8_t *)0xB885000F) = 0x08;
	usleep(1);
	*((uint8_t *)0xB8850012) = 0x02;
	usleep(1);
	printf("--> usb#1 eye test exit .....\n");
	return 0;
}

CONSOLE_CMD(eye1, "usb", usb_1_debug_eye_test, CONSOLE_CMD_MODE_SELF,
	    "usb#1 eye patern for oscilloscope")

int usb_0_debug_eye_test(int argc, char **argv)
{
	printf("--> usb#0 eye test\n");
	*((uint8_t *)0xB8844001) = 0x23;
	*((uint8_t *)0xB8844380) = 0x01;
	usleep(10);
	*((uint8_t *)0xB8844001) = 0x20;
	usleep(1000);
	*((uint8_t *)0xB8844380) = 0x00;
	usleep(1);
	*((uint8_t *)0xB8800083) = 0x10;
	usleep(1);
	*((uint8_t *)0xB8800083) = 0x00;
	usleep(1);
	*((uint8_t *)0xb8845020) = 0xc0;
	*((uint8_t *)0xb8845021) = 0x10;
	*((uint8_t *)0xB8844380) =
		0x00; //ok    otg_dis = 0, use default iddig = 1, Èç¹û²åÉÏ×ª½ÓÏßÀ­µÍid£¬Ôòfail
	*((uint8_t *)0xB8844001) = 0x60;
	// rick pattern
	usleep(100000); //delay 100ms for usb init
	//config usb phy
	/*
    *((uint8_t*)0xB8845100) = 0x1f;usleep(1); // [2:0] always enable pre-emphasis
    *((uint8_t*)0xB8845102) = 0x64;usleep(1);// [6:4] HS eye tuning 400/362.5/350/387.5 /412.5/425/475/450
    *((uint8_t*)0xB8845103) = 0xfc;usleep(1);//[6:2] odt, default:0xd4,0xfc:fastest rise time; 0xc0:slowest rise time. bit7 default is 1
    *((uint8_t*)0xB8845105) = 0xbc;usleep(1);// [4:2] TX HS pre_emphasize stength 111 is the strongest
    */

	*((uint8_t *)0xB884400F) = 0x01;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0x0000;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0x0000;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0x0000;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0x0000;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xAA00;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xAAAA;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xAAAA;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xAAAA;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xEEAA;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xEEEE;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xEEEE;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xEEEE;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xFEEE;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xFFFF;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xFFFF;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xFFFF;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xFFFF;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xFFFF;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0x7FFF;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xDFBF;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xF7EF;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xFDFB;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0x7EFC;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xDFBF;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xF7EF;
	usleep(1);
	*((uint16_t *)0xB8844020) = 0xFDFB;
	usleep(1);
	*((uint8_t *)0xB8844020) = 0x7e;
	usleep(1);

	*((uint8_t *)0xB884400F) = 0x08; // set test mode = 3
	usleep(1);
	*((uint8_t *)0xB8844012) = 0x02; // set tx packet ready
	usleep(100);
	printf("--> usb#0 eye test exit ...\n");
	return 0;
}

CONSOLE_CMD(eye0, "usb", usb_0_debug_eye_test, CONSOLE_CMD_MODE_SELF,
	    "usb#0 eye patern for oscilloscope")

#endif /* CONFIG_CMDS_USB_EYE_PATTERN */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>

int __storage_device_speed_test(char *path, size_t per_bytes,
				size_t total_bytes)
{
	char *buf;
	struct timeval tv1, tv2;
	int cost_ms, rc, cnt = 0;
	FILE *fp;

	buf = malloc(per_bytes);
	if (buf == NULL) {
		printf("Error: Cannot malloc 64KB buffer\n");
		return -1;
	}
	memset(buf, 'k', per_bytes);

	fp = fopen(path, "w+");
	if (fp == NULL) {
		printf("Error: Cannot create %s\n", path);
		return -1;
	}

	printf("=---> Try to write file(%s) %ldMB test data...\n", path,
	       total_bytes / (1000 * 1000));
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = fwrite(buf, per_bytes, 1, fp);
		cnt += rc;
		if ((cnt >= total_bytes / per_bytes) || (rc != 1))
			break;
	}
	gettimeofday(&tv2, NULL);
	printf("=---> Write file(%s) successfully (offset: %d)...\n", path,
	       cnt);
	cnt *= (per_bytes);

	cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) -
		  (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);

	printf("========================================================\n");
	printf("File: %s\n", path);
	printf("Length per fread/fwrite: %ld (%ld KB)\n", per_bytes,
	       per_bytes / 1024);
	printf("fwrite: total bytes %d KB, %d MB\n", cnt / 1000, cnt / 1000000);
	printf("fwrite: duration: %d ms\n", cost_ms);
	printf("fwrite: speed: %d.%d MB/s, %d KB/s\n", (cnt / 1000) / cost_ms,
	       ((cnt) / cost_ms) % 1000, (cnt) / cost_ms);
	printf("========================================================\n\n");
	fclose(fp);

	/* ***************************************************** */
	cnt = 0;
	fp = fopen(path, "r");
	if (fp == NULL) {
		printf("Error: Cannot open %s\n", path);
		return -1;
	}
	printf("=---> Try to read file(%s) %ldMB test data...\n", path,
	       total_bytes / (1000 * 1000));
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = fread(buf, per_bytes, 1, fp);
		cnt += rc;
		if ((cnt >= total_bytes / per_bytes) || (rc != 1))
			break;
	}
	gettimeofday(&tv2, NULL);
	printf("=---> Read file(%s) successfully (offset: %d)...\n", path, cnt);
	cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) -
		  (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);

	printf("========================================================\n");
	printf("File: %s\n", path);
	printf("Length per fread/fwrite: %ld (%ld KB)\n", per_bytes,
	       per_bytes / 1024);
	printf("fread: total bytes %d KB, %d MB\n", cnt / 1000, cnt / 1000000);
	printf("fread: duration: %d ms\n", cost_ms);
	printf("fread: speed: %d.%d MB/s, %d KB/s\n", (cnt / 1000) / cost_ms,
	       ((cnt) / cost_ms) % 1000, (cnt) / cost_ms);
	printf("========================================================\n\n");
	fclose(fp);

	free(buf);
	return 0;
}

int __storage_device_speed_test2(char *path, size_t per_bytes,
				 size_t total_bytes)
{
	char *buf;
	struct timeval tv1, tv2;
	int cost_ms, rc, cnt = 0;
	int fd;

	buf = malloc(per_bytes);
	if (buf == NULL) {
		printf("Error: Cannot malloc 64KB buffer\n");
		return -1;
	}
	memset(buf, 'k', per_bytes);

	fd = open(path, O_RDWR | O_CREAT | O_TRUNC);
	if (fd < 0) {
		printf("Error: Cannot create %s\n", path);
		return -1;
	}

	printf("\n=---> Try to write file(%s) %ldMB test data...\n", path,
	       total_bytes / (1000 * 1000));
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = write(fd, buf, per_bytes);
		cnt += rc;
		if (rc != per_bytes || cnt > total_bytes)
			break;
	}
	gettimeofday(&tv2, NULL);

	cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) -
		  (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);

	printf("========================================================\n");
	printf("File: %s\n", path);
	printf("Length per read/write: %ld (%ld KB)\n", per_bytes,
	       per_bytes / 1024);
	printf("Write: total bytes %d MB, %d KB\n", cnt / 1000000, cnt / 1000);
	printf("Write: duration: %d ms\n", cost_ms);
	printf("Write: speed: %d.%d MB/s, %d KB/s\n", (cnt / 1000) / cost_ms,
	       ((cnt) / cost_ms) % 1000, (cnt) / cost_ms);
	printf("========================================================\n\n");
	close(fd);

	/* ***************************************************** */
	cnt = 0;
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("Error: Cannot open %s\n", path);
		return -1;
	}
	lseek(fd, 0, SEEK_SET);
	printf("=---> Try to read file(%s) %ldMB test data...\n", path,
	       total_bytes / (1000 * 1000));
	gettimeofday(&tv1, NULL);
	for (;;) {
		rc = read(fd, buf, per_bytes);
		cnt += rc;
		if (rc != per_bytes || cnt > total_bytes)
			break;
	}
	gettimeofday(&tv2, NULL);
	cost_ms = (tv2.tv_sec * 1000 + tv2.tv_usec / 1000) -
		  (tv1.tv_sec * 1000 + tv1.tv_usec / 1000);

	printf("========================================================\n");
	printf("File: %s\n", path);
	printf("Length per read/write: %ld (%ld KB)\n", per_bytes,
	       per_bytes / 1024);
	printf("Read: total bytes %d MB, %d KB\n", cnt / 1000000, cnt / 1000);
	printf("Read: duration: %d ms\n", cost_ms);
	printf("Read: speed: %d.%d MB/s, %d KB/s\n", (cnt / 1000) / cost_ms,
	       ((cnt) / cost_ms) % 1000, (cnt) / cost_ms);
	printf("========================================================\n\n");
	close(fd);

	free(buf);
	return 0;
}

int storage_device_speed_test(int argc, char **argv)
{
	DIR *dir;
	struct dirent *dp;
	char file_name[512] = { 0 };

	dir = opendir("/media");
	if (dir == NULL) {
		printf("Error: please plugin U-disk or SD/TF card\n");
		return -1;
	}

	while ((dp = readdir(dir)) != NULL) {
		printf("\n\t/media/%s: \n", dp->d_name);
		sprintf(&file_name[0], "/media/%s/hichip-test.bin", dp->d_name);

		__storage_device_speed_test2(file_name, 512 * 1024,
					     64 * 1024 * 1024);
		__storage_device_speed_test2(file_name, 256 * 1024,
					     64 * 1024 * 1024);
		__storage_device_speed_test2(file_name, 128 * 1024,
					     64 * 1024 * 1024);
		__storage_device_speed_test2(file_name, 64 * 1024,
					     64 * 1024 * 1024);
		__storage_device_speed_test2(file_name, 32 * 1024,
					     64 * 1024 * 1024);
		__storage_device_speed_test2(file_name, 16 * 1024,
					     64 * 1024 * 1024);
		__storage_device_speed_test2(file_name, 4 * 1024,
					     64 * 1024 * 1024);
	}

	closedir(dir);
	return 0;
}

CONSOLE_CMD(usb, NULL, NULL, CONSOLE_CMD_MODE_SELF, "usb configuration")

/* test command for usb gadget driver */
CONSOLE_CMD(g_mass_storage, "usb", setup_usbd_mass_storage,
	    CONSOLE_CMD_MODE_SELF, "setup USB as mass-storage device")
CONSOLE_CMD(g_serial, "usb", setup_usbd_serial, CONSOLE_CMD_MODE_SELF,
	    "setup USB as serial console")
CONSOLE_CMD(g_serial_test, "usb", setup_usbd_serial_testing,
	    CONSOLE_CMD_MODE_SELF, "setup USB as serial console test demo")
CONSOLE_CMD(g_ncm, "usb", setup_usbd_ncm, CONSOLE_CMD_MODE_SELF,
	    "setup USB as NCM device demo")
CONSOLE_CMD(g_zero, "usb", setup_usbd_zero, CONSOLE_CMD_MODE_SELF,
	    "setup USB as zero device demo")


CONSOLE_CMD(hid_gadget_test, "usb", hid_gadget_test, CONSOLE_CMD_MODE_SELF,
	    "hid gadget test")

/* test command for usb host driver */
CONSOLE_CMD(hid, "usb", hid_test_main, CONSOLE_CMD_MODE_SELF,
	    "usb hid: get input event from USB keyboard")
CONSOLE_CMD(
	hid_kbd_demo, "usb", hid_gadget_kbd_demo, CONSOLE_CMD_MODE_SELF,
	"usb hid: get input event from USB keyboard, and send these data to usb hid gadget")
CONSOLE_CMD(
	hid_mouse_demo, "usb", hid_gadget_mouse_demo, CONSOLE_CMD_MODE_SELF,
	"usb hid: get input event from USB Mouse, and send these data to usb hid gadget")

CONSOLE_CMD(at_cmd, "usb", usb_serial_tty_console, CONSOLE_CMD_MODE_SELF,
	    "send AT command to 4G net device")

/* test command for libusb demo */
#ifdef CONFIG_CMDS_LIBUSB_EXAMPLES
CONSOLE_CMD(hello, "usb", libusb_helloworld_demo, CONSOLE_CMD_MODE_SELF,
	    "libusb examples: hello world demo")
CONSOLE_CMD(testlibusb, "usb", testlibusb, CONSOLE_CMD_MODE_SELF,
	    "libusb examples: test libusb")
CONSOLE_CMD(xusb, "usb", xusb, CONSOLE_CMD_MODE_SELF, "libusb examples: xusb")
CONSOLE_CMD(hotplug, "usb", hotplug, CONSOLE_CMD_MODE_SELF,
	    "libusb examples: hotplug")
#endif

/* commands for getting storage infomation */
CONSOLE_CMD(g_get_cap, "usb", usb_get_cap_cmds, CONSOLE_CMD_MODE_SELF,
	    "usb/sd get cap")
CONSOLE_CMD(speed, NULL, storage_device_speed_test, CONSOLE_CMD_MODE_SELF,
	    "speed test for storage devcie")
