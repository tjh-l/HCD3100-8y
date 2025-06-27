#define LOG_TAG "main"

#include <generated/br2_autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <kernel/elog.h>
#include <sys/poll.h>
#include <kernel/module.h>
#include <kernel/io.h>
#include <kernel/lib/console.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <nuttx/mtd/mtd.h>
#include <bootm.h>
#include <upgrade.h>
#include <cpu_func.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/standby.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <sys/ioctl.h>
#include <common.h>
#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#  include <sys/mount.h>
#endif
#include <hcuapi/gpio.h>

#include <errno.h>
#include <nuttx/fs/fs.h>
#include <kernel/completion.h>
#include <kernel/notify.h>
#include <hcuapi/sys-blocking-notify.h>
#include <hcuapi/watchdog.h>
#include <linux/minmax.h>
#include <kernel/delay.h>
#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBDEVICE)
#include <kernel/drivers/hcusb.h>
#endif
#include <kernel/ld.h>
#include <hcuapi/dis.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/sysdata.h>

#if defined(CONFIG_BOOT_SUPPORT_POPUP)
static void bootup_show_popup(const char *popup);
#endif
#if defined(CONFIG_BOOT_SUPPORT_BATTERY_CHECK)
static int bootup_battery_check(bool quick_check);
#endif
const char *fdt_get_sysmem_path(void)
{
	return "/hcrtos/memory-mapping/bootmem";
}

const char *fdt_get_stdio_path(void)
{
	return "/hcrtos/boot-stdio";
}

const char *fdt_get_fb0_path(void)
{
	return "/hcrtos/boot-fb0";
}

static unsigned long time_to_boot;
static void app_main(void *pvParameters);
int main(void)
{
	time_to_boot = sys_time_from_boot();
	xTaskCreate(app_main, (const char *)"app_main", configTASK_STACK_DEPTH, NULL, portPRI_TASK_NORMAL, NULL);

	vTaskStartScheduler();

	abort();
	return 0;
}

static int execute_from_ram = 0;
#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBDEVICE)
static int get_filesize(const char *file)
{
	struct stat sb;

	if (stat(file, &sb) == -1)
		return -1;
	return (int)sb.st_size;
}

static void remount_delay(int ms)
{
	umount("/mnt/ram0");
	msleep(ms);
	mount("/dev/ram0", "/mnt/ram0", "vfat", 0, NULL);
}

static void check_file_exist(char *filepath, const char *filename)
{
	int size_pre = 0, size = 0;
	while (1) {
		size = get_filesize(filepath);
		if (size <= 0) {
			printf("Booting from USB Device mode[port %d]...\n", CONFIG_BOOT_USBD_UPGRADE_USBD_PORT_NUM);
			printf("%s not found! Please connect PC and Target USB [port %d], then copy ===<%s>=== into U-Disk on PC!\n",
			       filename, CONFIG_BOOT_USBD_UPGRADE_USBD_PORT_NUM, filename);
			remount_delay(1000);
		} else if (size == size_pre) {
			break;
		}

		size_pre = size;
		remount_delay(100);
	}
}

static int get_file_path_in_partition(int np, char *filepath, int len, const char *name)
{
	u32 npart = 0;
	unsigned int i;

	fdt_get_property_u_32_index(np, "part-num", 0, &npart);

	for (i = 1; i <= npart; i++) {
		char strbuf[128];
		const char *partname;
		const char *filename = "null";

		memset(strbuf, 0, sizeof(strbuf));
		snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
		fdt_get_property_string_index(np, strbuf, 0, &partname);
		snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
		fdt_get_property_string_index(np, strbuf, 0, &filename);
		if (!strncmp(partname, name, max(strlen(partname), strlen(name)))) {
			snprintf(filepath, len, "/mnt/ram0/%s", filename);
			check_file_exist(filepath, filename);
			return 0;
		}
	}

	return -1;
}

static int get_file_path(char *filepath, int len, const char *name)
{
	int np;
	const char *status;
	do {
		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash");
		if (np < 0)
			break;

		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			break;
		}

		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
		if (get_file_path_in_partition(np, filepath, len, name) == 0)
			return 0;
	} while (0);

	do {
		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nand_flash");
		if (np < 0)
			break;

		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			break;
		}

		np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nand_flash/partitions");
		if (get_file_path_in_partition(np, filepath, len, name) == 0)
			return 0;
	} while (0);

	do {
		np = fdt_get_node_offset_by_path("/hcrtos/nand");
		if (np < 0)

		if (!fdt_get_property_string_index(np, "status", 0, &status) &&
		    !strcmp(status, "disabled")) {
			break;
		}

		np = fdt_get_node_offset_by_path("/hcrtos/nand/partitions");
		if (get_file_path_in_partition(np, filepath, len, name) == 0)
			return 0;
	} while (0);

	/* Then try external eMMC/SD-card devices */
	np = fdt_get_node_offset_by_path("/hcrtos/external_partitions");
	if (np < 0)
		return -1;

	if (get_file_path_in_partition(np, filepath, len, name) == 0)
		return 0;

	return -1;
}
#endif


struct usbsd_notifier {
	struct notifier_block notifier;
	int nb_partitions;
	const char *dev[30];
};

static int usbsd_notify(struct notifier_block *self, unsigned long action, void *dev)
{
	struct usbsd_notifier *nt = (struct usbsd_notifier *)self;

	if (action == SDMMC_NOTIFY_MOUNT || action == USB_MSC_NOTIFY_MOUNT) {
		if (nt->nb_partitions < 30)
			nt->dev[nt->nb_partitions++] = strdup(dev);
	}

	return NOTIFY_OK;
}

static struct usbsd_notifier usbsd_nt = {
	.notifier =
		{
			.notifier_call = usbsd_notify,
		},
	.nb_partitions = 0,
};

static int check_dev_or_file(char *devpath, int len, int is_file, int blk_index,
			     const char *prefix, const char *filename)
{
	struct stat sb;
	char strbuf[128];
	int i;

	if (!is_file) {
		snprintf(devpath, len, "/dev/mmcblk0p%d", blk_index);
		if (!stat(devpath, &sb))
			return 0;
	} else {
		for (i = 0; i < usbsd_nt.nb_partitions; i++) {
			memset(strbuf, 0, sizeof(strbuf));
			if (prefix)
				snprintf(strbuf, sizeof(strbuf), "/media/%s/%s/%s", usbsd_nt.dev[i], prefix, filename);
			else
				snprintf(strbuf, sizeof(strbuf), "/media/%s/%s", usbsd_nt.dev[i], filename);
			if (!stat(strbuf, &sb)) {
				snprintf(devpath, len, "%s", strbuf);
				return 0;
			}
		}
	}

	return -1;
}

static int get_block_devpath(char *devpath, int len, const char *name)
{
	int np = -1;
	u32 npart = 0;
	u32 i = 1;
	int rc;
	int is_file = 0;

#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBDEVICE)
	if (execute_from_ram)
		return get_file_path(devpath, len, name);
#endif

	/* First try mtd devices */
	if ((rc = get_mtd_device_index_nm(name)) >= 0) {
		snprintf(devpath, len, "/dev/mtdblock%d", rc);
		return rc;
	}

	/* Then try external eMMC/SD-card devices */
	np = fdt_get_node_offset_by_path("/hcrtos/external_partitions");
	if (np < 0) {
		np = fdt_get_node_offset_by_path("/hcrtos/external_files");
		if (np < 0)
			return -1;
		is_file = 1;
		sys_register_notify((struct notifier_block *)&usbsd_nt);
	}

	fdt_get_property_u_32_index(np, "part-num", 0, &npart);
	for (i = 1; i <= npart; i++) {
		char strbuf[128];
		const char *prop = NULL;
		const char *path_prefix = NULL;

		memset(strbuf, 0, sizeof(strbuf));
		snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
		fdt_get_property_string_index(np, strbuf, 0, &prop);
		if (prop == NULL)
			continue;
		if (strncmp(prop, name, max(strlen(prop), strlen(name))))
			continue;

		if (is_file) {
			fdt_get_property_string_index(np, "path-prefix", 0, &path_prefix);
			memset(strbuf, 0, sizeof(strbuf));
			snprintf(strbuf, sizeof(strbuf), "part%d-filename", i);
			fdt_get_property_string_index(np, strbuf, 0, &prop);
			if (prop == NULL)
				continue;
		}

		if (!check_dev_or_file(devpath, len, is_file, i, path_prefix, prop))
			return 0;

		module_init("hc15_mmc_device");
		module_init("hcmmc_device");
		if (is_file) {
			module_init("all");
		}
#if defined(CONFIG_BOOT_SUPPORT_POPUP)
		bool popup_showed = 0;
		TickType_t xTicksToWaitPopup = 2000;
		TimeOut_t xTimeOutPopup;
		vTaskSetTimeOutState(&xTimeOutPopup);
#endif
		while (1) {
#if defined(CONFIG_BOOT_SUPPORT_BATTERY_CHECK)
			if (bootup_battery_check(true)) {
#if defined(CONFIG_BOOT_SUPPORT_POPUP)
				popup_showed = 0;
				xTicksToWaitPopup = 2000;
				vTaskSetTimeOutState(&xTimeOutPopup);
#endif
			}
#endif
#if defined(CONFIG_BOOT_SUPPORT_POPUP)
			if (!popup_showed) {
				if (xTaskCheckForTimeOut(&xTimeOutPopup, &xTicksToWaitPopup) != pdFALSE) {
					printf("show popup\r\n");
					bootup_show_popup(NULL);
#if defined(CONFIG_BOOT_BACKLIGHT)
					open_lcd_backlight(1,((char *[]){"backlight"}));
#endif
					popup_showed = true;
				}
			}
#endif
			if (!check_dev_or_file(devpath, len, is_file, i, path_prefix, prop))
				break;
			msleep(1);
		}

		return 0;
	}

	return -1;
}
#ifdef CONFIG_BOOT_UPDATE_SCREEN_PARAMS
static int boot_update_dt(void *dtb)
{
	int boot_np;
	int dtb_np;
	unsigned int screen_w = 0, screen_h = 0;
	unsigned int temp = 0;
	int ret = 1;

	boot_np = fdt_get_node_offset_by_path("/hcrtos/de-engine/VPInitInfo/rgb-cfg/timing-para");
	dtb_np = fdt_path_offset(dtb, "/hcrtos/de-engine/VPInitInfo/rgb-cfg/timing-para");
	do {
		if (boot_np && dtb_np) {
			fdt_get_property_u_32_index(boot_np, "h-active-len", 0, (u32 *)&screen_w);
			fdt_get_property_u_32_index(boot_np, "v-active-len", 0, (u32 *)&screen_h);
			if (!screen_w || !screen_h)
				break;

			fdt_setprop_u32(dtb, dtb_np, "h-active-len", screen_w);
			fdt_setprop_u32(dtb, dtb_np, "v-active-len", screen_h);
			ret = 0;
			if (fdt_get_property_u_32_index(boot_np, "output-clock", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "output-clock", temp);
			if (fdt_get_property_u_32_index(boot_np, "h-total-len", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "h-total-len", temp);
			if (fdt_get_property_u_32_index(boot_np, "v-total-len", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "v-total-len", temp);
			if (fdt_get_property_u_32_index(boot_np, "h-front-len", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "h-front-len", temp);
			if (fdt_get_property_u_32_index(boot_np, "h-sync-len", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "h-sync-len", temp);
			if (fdt_get_property_u_32_index(boot_np, "h-back-len", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "h-back-len", temp);
			if (fdt_get_property_u_32_index(boot_np, "v-front-len", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "v-front-len", temp);
			if (fdt_get_property_u_32_index(boot_np, "v-sync-len", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "v-sync-len", temp);
			if (fdt_get_property_u_32_index(boot_np, "v-back-len", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "v-back-len", temp);
			if (fdt_get_property_u_32_index(boot_np, "h-sync-level", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "h-sync-level", temp);
			if (fdt_get_property_u_32_index(boot_np, "v-sync-level", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "v-sync-levell", temp);
			if (fdt_get_property_u_32_index(boot_np, "active-polarity", 0, (u32 *)&temp))
				break;
			fdt_setprop_u32(dtb, dtb_np, "active-polarity", temp);
		}
	} while (0);

	if(ret)
		printf("warning: screen_w ==0 or screen_h == 0\n");

	return ret;
}
#endif

static int sysdata_update_dt(void *dtb)
{
	struct sysdata sysdata = { 0 };
	int np;

	if (sys_get_sysdata(&sysdata)) {
		return -1;
	}

	np = fdt_path_offset(dtb, "/hcrtos/de-engine");
	if (np >= 0) {
		u32 tvtype = sysdata.tvtype;
		if ((int)tvtype >= 0)
			fdt_setprop_u32(dtb, np, "tvtype", tvtype);
	}

	np = fdt_path_offset(dtb, "/hcrtos/i2so");
	if (np >= 0) {
		u32 volume = sysdata.volume;
		if (volume <= 100)
			fdt_setprop_u32(dtb, np, "volume", volume);
	}

	return 0;
}

static int initramdisk_update_dt(void *dtb, unsigned long rd_start, unsigned long rd_size)
{
	int np;

	np = fdt_path_offset(dtb, "/chosen");
	if (np >= 0) {
		const char *bootargs = NULL;
		char *p;
		int lenp;
		bootargs = fdt_getprop(dtb, np, "bootargs", &lenp);
		if (bootargs == NULL)
			return 0;

		p = strstr(bootargs, "rd_start=");
		if (!p)
			return 0;
		sprintf(p, "rd_start=0x%08lX", rd_start);
		p[19] = ' ';
		p = strstr(bootargs, "rd_size=");
		if (!p)
			return 0;
		sprintf(p, "rd_size=0x%08lX", rd_size);
		p[18] = ' ';
	}

	return 0;
}

int close_watchdog(void)
{
	int ret = -1;
	int wdt_fd = -1;

	wdt_fd = open("/dev/watchdog", O_RDWR);
	if (wdt_fd < 0) {
		printf("can't open %s\n", "/dev/watchdog");
		return -1;
	}

	ret = ioctl(wdt_fd, WDIOC_STOP, 0);
	if (!ret) {
		return -1;
	}

	close(wdt_fd);
}

static void set_gpio_def_st(void)
{
	int np, num_pins, i, pin, pin_val;
	bool val;

	np = fdt_node_probe_by_path("/hcrtos/gpio-out-def");
	if (np < 0)
		return;

	num_pins = 0;
	if (fdt_get_property_data_by_name(np, "gpio-group", &num_pins) == NULL)
		num_pins = 0;

	num_pins >>= 3;

	if (num_pins == 0)
		return;

	for (i = 0; i < num_pins; i++) {
		fdt_get_property_u_32_index(np, "gpio-group", i * 2, &pin);
		fdt_get_property_u_32_index(np, "gpio-group", i * 2 + 1, &pin_val);

		val = !pin_val;
		gpio_configure(pin, GPIO_DIR_OUTPUT);
		gpio_set_output(pin, val);
	}

	return;
}

static void boot_mode_setup(void)
{
	/*
	 * Using South Bridge Timer7 register to store the boot mode flag
	 * The South Bridge Timer7 register will only be cleared by power-off
	 */
	unsigned char flag = REG8_READ(0xb8818a70);
	if (flag != STANDBY_FLAG_COLD_BOOT && flag != STANDBY_FLAG_WARM_BOOT) {
		REG8_WRITE(0xb8818a70, STANDBY_FLAG_COLD_BOOT);
		REG8_WRITE(0xb8818a71, STANDBY_BOOTUP_SLOT_UNDEF);
	} else {
		REG8_WRITE(0xb8818a70, STANDBY_FLAG_WARM_BOOT);
	}
}

static void reset_usb_mode(void)
{
#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBDEVICE)
	int usb_port = CONFIG_BOOT_USBD_UPGRADE_USBD_PORT_NUM;
	if (execute_from_ram != 0) {
		hcusb_gadget_msg_deinit();
		/* reset usb mode usbdevice to usbhost */
		hcusb_set_mode(usb_port, MUSB_HOST);
	}
#endif
}

#if defined(CONFIG_BOOT_HCRTOS_OR_HCRTOS) ||                                   \
	defined(CONFIG_BOOT_HCRTOS_OR_HCLINUX_DUALCORE) ||                     \
	defined(CONFIG_BOOT_HCLINUX_DUALCORE_OR_HCRTOS) ||                     \
	defined(CONFIG_BOOT_HCLINUX_DUALCORE_OR_HCLINUX_DUALCORE) ||           \
	defined(CONFIG_BOOT_SHOWLOGO) || defined(CONFIG_BOOT_OSDLOGO)
static unsigned long get_bootup_slot(void)
{
	unsigned long boot_slot = STANDBY_BOOTUP_SLOT_UNDEF;
	int fd = open("/dev/standby", O_RDWR);
	if (fd >= 0) {
		ioctl(fd, STANDBY_GET_BOOTUP_SLOT, &boot_slot);
		close(fd);
	}

	return boot_slot;
}
#else
static unsigned long get_bootup_slot(void)
{
	return 0;
}
#endif

#if defined(CONFIG_BOOT_SUPPORT_POPUP)
static int bootup_show_popup_from_part(const char *part, const char *popup)
{
	char devpath[64];
	int ret;

	ret = get_block_devpath(devpath, sizeof(devpath), part);
	if (ret >= 0)
		ret = mount(devpath, "/eromfs", "romfs", MS_RDONLY, NULL);

	if (ret >= 0) {
		do {
#if defined(CONFIG_BOOT_OSDLOGO)
			if (popup) {
				ret = osdlogo(2, ((char *[]){ "showlogo", (char *)popup }));
				if (ret == 0)
					break;
			} else {
				ret = osdlogo(2, ((char *[]){ "showlogo", "/eromfs/popup.bmp" }));
				if (ret != 0)
					ret = osdlogo(2, ((char *[]){ "showlogo", "/eromfs/popup.bmp.gz" }));
				if (ret == 0)
					break;
			}
#endif

#if defined(CONFIG_BOOT_SHOWLOGO)
			if (popup)
				ret = showlogo(2, ((char *[]){ "showlogo", (char *)popup }));
			else
				ret = showlogo(2, ((char *[]){ "showlogo", "/eromfs/popup.hc" }));
			if (ret == 0) {
				wait_show_logo_finish_feed();
				break;
			}
#endif
		} while (0);
		umount("/eromfs");
	}

	return ret;
}

static void bootup_show_popup(const char *popup)
{
	int ret;

	ret = bootup_show_popup_from_part("eromfs", popup);
	if (ret == 0)
		return;

	ret = bootup_show_popup_from_part("eromfs2", popup);
	if (ret == 0)
		return;

	ret = bootup_show_popup_from_part("eromfs3", popup);
	if (ret == 0)
		return;
}
#endif

#if defined(CONFIG_BOOT_SUPPORT_BATTERY_CHECK)
static int bootup_battery_check(bool quick_check)
{
	int low_battery_showed = 0;

	while (1) {
		if (!is_low_battery(quick_check))
			return low_battery_showed;

		if (!low_battery_showed) {
			low_battery_showed = 1;
			bootup_show_popup(get_low_battery_popup_fpath());
#if defined(CONFIG_BOOT_BACKLIGHT)
			open_lcd_backlight(1, ((char *[]){ "backlight" }));
#endif
		}
	}
	return low_battery_showed;
}
#endif

static void bootup_show_bootmedia(void)
{
	unsigned long feature = STANDBY_BOOTUP_SLOT_FEATURE(get_bootup_slot());
	char devpath[64];
	int ret;

	(void)feature;
	(void)ret;
	(void)devpath;

#if defined(CONFIG_BOOT_SHOWLOGO) && !defined(CONFIG_DISABLE_MOUNTPOINT)
	do {
		if (feature != STANDBY_BOOTUP_SLOT_FEATURE_LOGO_ON && feature != STANDBY_BOOTUP_SLOT_FEATURE_UNDEF)
			break;

		if (execute_from_ram != 0)
			break;

		ret = get_block_devpath(devpath, sizeof(devpath), "eromfs");
		if (ret >= 0)
			ret = mount(devpath, "/etc", "romfs", MS_RDONLY, NULL);

		if (ret >= 0) {
			showlogo(2, ((char *[]){ "showlogo", "/etc/logo.hc" }));
			wait_show_logo_finish_feed();
		}
	} while (0);
#endif

#if defined(CONFIG_BOOT_OSDLOGO) && !defined(CONFIG_DISABLE_MOUNTPOINT)
	do {

#if defined(CONFIG_BOOT_SHOWLOGO)
		if (feature != STANDBY_BOOTUP_SLOT_FEATURE_OSDLOGO_ON)
			break;
#else
		if (feature != STANDBY_BOOTUP_SLOT_FEATURE_OSDLOGO_ON && feature != STANDBY_BOOTUP_SLOT_FEATURE_UNDEF)
			break;
#endif

		if (execute_from_ram != 0)
			break;

		ret = get_block_devpath(devpath, sizeof(devpath), "logo");
		if (ret >= 0) {
			osdlogo(2, ((char *[]){ "showlogo", devpath }));
		}
	} while (0);
#endif
}

#if defined(CONFIG_BOOT_HCRTOS) || \
	defined(CONFIG_BOOT_HCRTOS_OR_HCRTOS) ||  \
	defined(CONFIG_BOOT_HCRTOS_OR_HCLINUX_DUALCORE) || \
	defined(CONFIG_BOOT_HCLINUX_DUALCORE_OR_HCRTOS)
static void bootup_hcrtos(const char *partname)
{
	char devpath[64];
	int ret = get_block_devpath(devpath, sizeof(devpath), partname);
	printf("booting hcrtos %s\r\n", partname);
	if (ret >= 0) {
		reset_usb_mode();
		mtdloaduImage(2, ((char *[]){ "mtdloaduImage", devpath }));
		if (bootm(NULL, 0, 1, ((char *[]){ "bootm" })))
			upgrade_force();
	}
}
#endif

#if defined(CONFIG_BOOT_HCLINUX_DUALCORE) || \
	defined(CONFIG_BOOT_HCRTOS_OR_HCLINUX_DUALCORE) || \
	defined(CONFIG_BOOT_HCLINUX_DUALCORE_OR_HCRTOS) || \
	defined(CONFIG_BOOT_HCLINUX_DUALCORE_OR_HCLINUX_DUALCORE)
static void bootup_hclinux_dualcore(void)
{
	char devpath[64];
	void *dtb = malloc(0x10000);
	char loadaddr[16] = { 0 };
	char dtbaddr[16] = { 0 };
	int ret;

	printf("booting hclinux dualcore firmware\r\n");

	sprintf(dtbaddr, "0x%08x", (unsigned int)dtb);
	REG8_WRITE(0xb880006b, 0x1);
	REG32_WRITE(0xb8800004, (uint32_t)dtb);

	ret = get_block_devpath(devpath, sizeof(devpath), "dtb");
	if (ret >= 0) {
		mtdloadraw(4, ((char *[]){ "mtdloadraw", dtbaddr, devpath, "0x10000" }));
		sysdata_update_dt(dtb);
#ifdef CONFIG_BOOT_UPDATE_SCREEN_PARAMS
		boot_update_dt(dtb);
#endif
		cache_flush(dtb, fdt_totalsize(dtb));
	}

	ret = get_block_devpath(devpath, sizeof(devpath), "initramdisk");
	if (ret >= 0) {
		mtdloadraw(4, ((char *[]){ "mtdloadraw", "-1", devpath, "-1" }));
		initramdisk_update_dt(dtb, raw_load_addr, raw_load_size);
		cache_flush(dtb, fdt_totalsize(dtb));
	}

	ret = get_block_devpath(devpath, sizeof(devpath), "avp");
	if (ret >= 0) {
		reset_usb_mode();
		mtdloaduImage(2, ((char *[]){ "mtdloaduImage", devpath }));
		if (bootm(NULL, 0, 1, ((char *[]){ "bootm" })))
			upgrade_force();

		if (REG8_READ(0xb880006b) != 0x2) {
			/* scpu boot fail */
			reset();
		}
	}

	ret = get_block_devpath(devpath, sizeof(devpath), "linux");
	if (ret >= 0) {
		mtdloaduImage(2, ((char *[]){ "mtdloaduImage", devpath }));
		sprintf(loadaddr, "0x%08lx", image_load_addr);
		if (bootm(NULL, 0, 4, ((char *[]){ "bootm", loadaddr, "-", dtbaddr })))
			upgrade_force();
	}
}
#endif

#if defined(CONFIG_BOOT_HCLINUX_SINGLECORE)
static void bootup_hclinux_singlecore(void)
{
	char devpath[64];
	void *dtb = malloc(0x10000);
	char loadaddr[16] = { 0 };
	char dtbaddr[16] = { 0 };

	sprintf(dtbaddr, "0x%08x", (unsigned int)dtb);
	ret = get_block_devpath(devpath, sizeof(devpath), "dtb");
	if (ret >= 0) {
		mtdloadraw(4, ((char *[]){ "mtdloadraw", dtbaddr, devpath, "0x10000" }));
		sysdata_update_dt(dtb);
		cache_flush(dtb, fdt_totalsize(dtb));
	}

	ret = get_block_devpath(devpath, sizeof(devpath), "linux");
	if (ret >= 0) {
		reset_usb_mode();
		mtdloaduImage(2, ((char *[]){ "mtdloaduImage", devpath }));
		sprintf(loadaddr, "0x%08lx", image_load_addr);
		if (bootm(NULL, 0, 4, ((char *[]){ "bootm", loadaddr, "-", dtbaddr })))
			upgrade_force();
	}
}
#endif


#if defined(CONFIG_BOOT_KEYSTONE)
static void boot_keystone_set(void)
{
	int fd_sys = -1;
	int fd_dis = -1;
	uint16_t top_w = 0;
	uint16_t bottom_w = 0;
	struct sysdata sys_data = { 0 };
	struct persistentmem_node node;
    struct dis_keystone_param vhance;

	do {

		fd_sys = open("/dev/persistentmem", O_RDWR);
		if (fd_sys < 0) {
			printf("%s(). Open /dev/persistentmem failed (%d)\n", __func__, fd_sys);
			break;
		}
		node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
		node.offset = 0;
		node.size = sizeof(struct sysdata);
		node.buf = &sys_data;
		if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
			break;
		}

		top_w = sys_data.keystone_top_w;
		bottom_w = sys_data.keystone_bottom_w;
		if ((!top_w) || (!bottom_w) || (top_w == bottom_w))
			break;

		fd_dis = open("/dev/dis" , O_RDWR);
		if(fd_dis < 0) {
			printf("%s(). Open /dev/dis failed (%d)\n", __func__, fd_dis);
			break;
		}

	    memset(&vhance, 0, sizeof(vhance));
	    vhance.distype = DIS_TYPE_HD;
	    vhance.info.enable = 1;
	    vhance.info.bg_enable = 0;
	    vhance.info.width_up = top_w;
	    vhance.info.width_down = bottom_w;
	    ioctl(fd_dis , DIS_SET_KEYSTONE_PARAM , &vhance);

	} while (0);

	if (fd_sys >= 0)
		close(fd_sys);
	if (fd_dis >= 0)
		close(fd_dis);
}
#endif

static void app_main(void *pvParameters)
{
	int ret;
	bool is_mmc = 0;
	char devpath[64];
	char *excludes[] = {
		/* mmc/sd support */
		"hc15_mmc_device",
		"hcmmc_device",
		/* usb support */
		"usb",
		"musb_driver",
		"hc16xx_driver",
		"hcdisk_driver",
		"usb_storage_driver",
		/* usb gadget support */
		"mass_storage",
		/* usb host support */
		"usb_core",
		};

	boot_mode_setup();

	hc_clk_disable_all();

	set_gpio_def_st();

	assert(module_init2("all", 9, excludes) == 0);
	printf("time to boot: %ld\r\n", time_to_boot);

	/* Set default time zone is GMT+8 */
	setenv("TZ", CONFIG_APP_TIMEZONE, 1);
	tzset();

	upgrade_detect(&execute_from_ram);

#if defined(CONFIG_BOOT_STANDBY)
	boot_enter_standby(1, ((char *[]){ "standby" }));
#endif

#if defined(CONFIG_BOOT_PQ_START)
	open_pq_start(1, ((char *[]){ "pq_start" }));
#endif

#if defined(CONFIG_BOOT_LCD)
	open_boot_lcd_init(1,((char *[]){"boot_lcd"}));
#endif

#if defined(CONFIG_BOOT_SUPPORT_BATTERY_CHECK)
	bootup_battery_check(false);
#endif

#if defined(CONFIG_BOOT_KEYSTONE)
	boot_keystone_set();
#endif	

	bootup_show_bootmedia();

#if defined(CONFIG_BOOT_BACKLIGHT)
	open_lcd_backlight(1,((char *[]){"backlight"}));
#endif

#if defined(CONFIG_BOOT_HCRTOS)
	bootup_hcrtos("firmware");
#elif defined(CONFIG_BOOT_HCLINUX_DUALCORE)
	bootup_hclinux_dualcore();
#elif defined(CONFIG_BOOT_HCLINUX_SINGLECORE)
	bootup_hclinux_singlecore();
#elif defined(CONFIG_BOOT_HCRTOS_OR_HCRTOS)
	unsigned long slot = STANDBY_BOOTUP_SLOT_NR(get_bootup_slot());
	if (slot == STANDBY_BOOTUP_SLOT_NR_SECONDARY)
		bootup_hcrtos("firmware2");
	else
		bootup_hcrtos("firmware");
#elif defined(CONFIG_BOOT_HCRTOS_OR_HCLINUX_DUALCORE)
	unsigned long slot = STANDBY_BOOTUP_SLOT_NR(get_bootup_slot());
	if (slot == STANDBY_BOOTUP_SLOT_NR_SECONDARY)
		bootup_hclinux_dualcore();
	else
		bootup_hcrtos("firmware");
#elif defined(CONFIG_BOOT_HCLINUX_DUALCORE_OR_HCRTOS)
	unsigned long slot = STANDBY_BOOTUP_SLOT_NR(get_bootup_slot());
	if (slot == STANDBY_BOOTUP_SLOT_NR_SECONDARY)
		bootup_hcrtos("firmware");
	else
		bootup_hclinux_dualcore();
#elif defined(CONFIG_BOOT_HCLINUX_DUALCORE_OR_HCLINUX_DUALCORE)
#error "Not support yet"
#endif

	assert(module_init("all") == 0);

	console_init();
	/* Console loop */
	console_start();

	/* Program should not run to here. */
	for (;;);

	/* Delete current thread. */
	vTaskDelete(NULL);
}
