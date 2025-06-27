#include <generated/br2_autoconf.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <kernel/module.h>
#include <kernel/lib/console.h>
#include <hcuapi/sys-blocking-notify.h>
#include <hcfota.h>
#include <kernel/completion.h>
#include <kernel/notify.h>
#include <linux/notifier.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/input.h>
#include <poll.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <nuttx/drivers/ramdisk.h>
#include <fsutils/mkfatfs.h>
#include <kernel/drivers/hcusb.h>
#include <sys/mount.h>
#include <hcuapi/mmz.h>
#include <upgrade.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <kernel/elog.h>
#include <hcuapi/standby.h>

#define max(a, b) ({\
		typeof(a) _a = a;\
		typeof(b) _b = b;\
		_a > _b ? _a : _b; })

#define min(a, b) ({\
		typeof(a) _a = a;\
		typeof(b) _b = b;\
		_a < _b ? _a : _b; })

static struct completion hcfota_ready;
static struct completion udisk_connect_ready;
static struct completion upgrade_done;
static struct completion mmc_ready;
static unsigned int g_upgrade_mode;
static int do_hcfota_upgrade(unsigned int ota_mode);
static int prepare_ramdisk(void);
static void free_ramdisk(void);

static int boot_is_startup_from_external_partitions(void)
{
	int np = -1;
	u32 npart = 0;
	u32 i = 1;

	np = fdt_get_node_offset_by_path("/hcrtos/external_partitions");
	if (np < 0)
		return 0;

	fdt_get_property_u_32_index(np, "part-num", 0, &npart);
	for (i = 1; i <= npart; i++) {
		char strbuf[128];
		const char *partname = NULL;

		memset(strbuf, 0, sizeof(strbuf));
		snprintf(strbuf, sizeof(strbuf), "part%d-label", i);
		fdt_get_property_string_index(np, strbuf, 0, &partname);
		if (partname == NULL)
			continue;
		if (!strncmp(partname, "romfs", max(strlen(partname), strlen("romfs"))))
			return 1;
		else if (!strncmp(partname, "firmware", max(strlen(partname), strlen("firmware"))))
			return 1;
		else if (!strncmp(partname, "dtb", max(strlen(partname), strlen("dtb"))))
			return 1;
		else if (!strncmp(partname, "avp", max(strlen(partname), strlen("avp"))))
			return 1;
		else if (!strncmp(partname, "linux", max(strlen(partname), strlen("linux"))))
			return 1;
	}

	return 0;
}

static char wait_any_key_pressed(char *tip)
{
#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBDEVICE)
	char buf = '\0';
	struct pollfd pfd = { 0 };
	int wait_timeout = CONFIG_BOOT_USBD_UPGRADE_REQUEST_TIME;

	pfd.fd = STDIN_FILENO;
	pfd.events = POLLIN | POLLRDNORM;

	do {
		printf("%s.....%d\n",tip, wait_timeout);
		wait_timeout--;
		if (wait_timeout >= 0) {
			if (poll(&pfd, 1, 1000) > 0) {
				read(STDIN_FILENO, &buf, 1);
				return buf;
			}
		} else {
			if (poll(&pfd, 1, 0) > 0) {
				read(STDIN_FILENO, &buf, 1);
				return buf;
			}
		}
	} while (wait_timeout >= 0);
#endif
	return (char)0;
}

#if 0
static int upgrade_detect_key(void)
{
#define MAX_UPGRADE_KEY_NUMBER 10
#define MAX_INPUT_EVENTS 5
#define UPGRADE_KEY_SAMPLE_TIMES 5
	int fds[MAX_INPUT_EVENTS] = { 0 };
	int key_value[MAX_UPGRADE_KEY_NUMBER];
	struct input_event t = {0};
	u32 nkeys = 0;
	int i, np, nfd = 0, detect = 0;
	int retry_ms = 300;

	nkeys = 0;
	for (i = 0; i < MAX_UPGRADE_KEY_NUMBER; i++)
		key_value[i] = -1;

	do {
		np = fdt_get_node_offset_by_path("/hcrtos/hcfota-upgrade");
		if (np < 0)
			break;
		const char *status = NULL;
		if (!fdt_get_property_string_index(np, "status", 0, &status) && !strcmp(status, "disabled"))
			break;
		if (fdt_get_property_data_by_name(np, "key", &nkeys) == NULL)
			nkeys = 0;
		nkeys >>= 2;
		if (nkeys == 0)
			break;
		nkeys = min((int)nkeys, MAX_UPGRADE_KEY_NUMBER);
		for (i = 0; i < (int)nkeys; i++) {
			fdt_get_property_u_32_index(np, "key", i, &key_value[i]);
		}
	} while (0);

	if (nkeys == 0)
		return 0;

	for (i = 0; i < MAX_INPUT_EVENTS; i++) {
		char path[64];
		int fd;
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "/dev/input/event%d", i);
		fd = open(path, O_RDONLY);
		if (fd < 0)
			continue;
		fds[nfd++] = fd;
	}

	if (nfd == 0)
		return 0;

	for (i = 0; i < nfd; i++) {
		if (read(fds[i], &t, sizeof(t)) != sizeof(t))
			continue;
		if (t.type != EV_KEY || !t.value)
			continue;

		for (int j = 0; j < (int)nkeys; j++) {
			if (t.code == key_value[j]) {
				detect++;
				break;
			}
		}
	}

	if (detect == 0)
		return 0;

	while (retry_ms-- && detect != UPGRADE_KEY_SAMPLE_TIMES) {
		for (i = 0; i < nfd; i++) {
			if (read(fds[i], &t, sizeof(t)) != sizeof(t))
				continue;
			if (t.type != EV_KEY || !t.value)
				continue;

			for (int j = 0; j < (int)nkeys; j++) {
				if (t.code == key_value[j]) {
					detect++;
					break;
				}
			}
			if (detect == UPGRADE_KEY_SAMPLE_TIMES)
				break;
		}

		usleep(1000);
	}

	for (i = 0; i < nfd; i++)
		close(fds[i]);

	return detect == UPGRADE_KEY_SAMPLE_TIMES ? 1 : 0;
}
#else
#define UPGRADE_KEY_MAX_NUMBER 10
#define UPGRADE_KEY_MAX_INPUT_EVENTS 5
#define UPGRADE_KEY_REPEAT_LONG_KEY_TIME 5

/*
 * if no key press is detected, then exit
 * if upgrade key is detected, press and hold the key for 5 seconds
 */
int upgrade_detect_key(void)
{
    int fds[UPGRADE_KEY_MAX_INPUT_EVENTS] = { 0 };
    int key_value[UPGRADE_KEY_MAX_NUMBER] = { 0 };
    int key_code = 0;
    struct input_event t = {0};
    u32 nkeys = 0;
    int i, np, nfd = 0, detect = 0;
    int retry_ms = 300;
    struct timeval time_now = {0};
    double start_time = 0;
    double cur_time = 0;
    int res = 0;

    /* key init */
    nkeys = 0;
    for (i = 0; i < UPGRADE_KEY_MAX_NUMBER; i++)
        key_value[i] = -1;

    do {
        np = fdt_get_node_offset_by_path("/hcrtos/hcfota-upgrade");
        if (np < 0)
            break;
        const char *status = NULL;
        if (!fdt_get_property_string_index(np, "status", 0, &status) && !strcmp(status, "disabled"))
            break;
        if (fdt_get_property_data_by_name(np, "key", &nkeys) == NULL)
            nkeys = 0;
        nkeys >>= 2;
        if (nkeys == 0)
            break;
        nkeys = min((int)nkeys, UPGRADE_KEY_MAX_NUMBER);
        for (i = 0; i < (int)nkeys; i++) {
            fdt_get_property_u_32_index(np, "key", i, &key_value[i]);
        }
    } while (0);

    if (nkeys == 0)
        return 0;

    for (i = 0; i < UPGRADE_KEY_MAX_INPUT_EVENTS; i++) {
        char path[64];
        int fd;
        memset(path, 0, sizeof(path));
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        fd = open(path, O_RDONLY);
        if (fd < 0)
            continue;
        fds[nfd++] = fd;
    }

	if (nfd == 0)
		return 0;

    /* try to read upgrade key */
    for (i = 0; i < nfd; i++) {
        if (read(fds[i], &t, sizeof(t)) != sizeof(t))
            continue;

        if (t.type == EV_SYN || !t.value){
            continue;
        }

        /* get key_code, long key's code in t.value */
        if(t.type == EV_KEY){
            key_code = t.code;
        }else if(t.type == EV_MSC){
            key_code = t.value;
        }else{
            key_code = -2;  /* key_value defaut is -1 */
        }

        for (int j = 0; j < (int)nkeys; j++) {
            if (key_code == key_value[j]) {
                detect = 1;

                gettimeofday(&time_now, NULL);
                start_time = time_now.tv_sec;

                printf("t.code=%u, t.type=%u, t.value=%ld, start_time(%llu, %lu)\n",
                        t.code, t.type, t.value, time_now.tv_sec, time_now.tv_usec);

                break;
            }
        }
	}

    /* if no key press is detected, then return */
    if (detect == 0){
        return 0;
    }else{
    /* check is long key? */
        detect = 0;
    }

    /* if upgrade key is detected, press and hold the key for 5 seconds */
    while (1) {
        for (i = 0; i < nfd; i++) {
            if (read(fds[i], &t, sizeof(t)) != sizeof(t)){
                gettimeofday(&time_now, NULL);
                cur_time = time_now.tv_sec;
                if((cur_time - start_time) >= UPGRADE_KEY_REPEAT_LONG_KEY_TIME){
                    printf("wait upgrade key timeout\n");
                    detect = 0;
                    goto end;
                }

                /* the minimum time interval between button sending is 33ms */
                usleep(10000);
                continue;
            }

            if (t.type == EV_SYN){
                continue;
            }

            /* get key_code */
            if(t.type == EV_KEY){
                key_code = t.code;
            }else if(t.type == EV_MSC){
                key_code = t.value;
            }else{
                key_code = -2;  /* key_value defaut is -1 */
            }

            for (int j = 0; j < (int)nkeys; j++) {
                if (key_code == key_value[j]) {
                    //printf("t.code=%u, t.type=%u, t.value=%ld, time(%llu, %lu)\n",
                    //        t.code, t.type, t.value, time_now.tv_sec, time_now.tv_usec);

                    gettimeofday(&time_now, NULL);
                    cur_time = time_now.tv_sec;
                    if((cur_time - start_time) >= (UPGRADE_KEY_REPEAT_LONG_KEY_TIME - 1)){
                        detect = 1;
                        printf("t.code=%u, t.type=%u, t.value=%ld, end_time(%llu, %lu)\n",
                                t.code, t.type, t.value, time_now.tv_sec, time_now.tv_usec);
                        printf("Upgrade button detected Press %ds, enter the upgrade mode\n", UPGRADE_KEY_REPEAT_LONG_KEY_TIME);
                        goto end;
                    }else{
                        /* detected key up, then exit */
                        if(t.value == 0){
                            detect = 0;
                            printf("t.code=%u, t.type=%u, t.value=%ld, end_time(%llu, %lu)\n",
                                    t.code, t.type, t.value, time_now.tv_sec, time_now.tv_usec);
                            printf("detected key up, exit detection, time is %fs\n", (cur_time - start_time));
                            goto end;
                        }
                    }

                    break;
                }
            }
        }

        /* the minimum time interval between button sending is 33ms */
        usleep(10000);
    }

end:
    for (i = 0; i < nfd; i++)
        close(fds[i]);

    return detect;
}

#endif

unsigned long force_ota_mode = 0;

static unsigned long upgrade_all_modes(void)
{
	unsigned long ota_mode = 0;

	ota_mode |= hcfota_reboot_ota_detect_mode_priority(HCFOTA_REBOOT_OTA_DETECT_USB_HOST, 0);
	ota_mode |= hcfota_reboot_ota_detect_mode_priority(HCFOTA_REBOOT_OTA_DETECT_SD, 1);
	ota_mode |= hcfota_reboot_ota_detect_mode_priority(HCFOTA_REBOOT_OTA_DETECT_NETWORK, 2);
	ota_mode |= hcfota_reboot_ota_detect_mode_priority(HCFOTA_REBOOT_OTA_DETECT_USB_DEVICE, 3);

	return ota_mode;
}

int upgrade_force(void)
{
	return do_hcfota_upgrade(upgrade_all_modes());
}

static unsigned long get_ota_detect_mode(void)
{
	int fd;
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		printf("open /dev/persistentmem failed\n");
		return HCFOTA_REBOOT_OTA_DETECT_NONE;
	}

	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = 0;
	node.size = sizeof(struct sysdata);
	node.buf = &sysdata;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
		printf("No systemdata found in /dev/persistentmem\n");
		close(fd);
		return upgrade_all_modes();
	}

	close(fd);

	if (sysdata.firmware_version == 0 || sysdata.ota_doing)
		return upgrade_all_modes();

	return sysdata.ota_detect_modes;
}

static int set_ota_detect_mode(unsigned long mode)
{
	int fd;
	struct persistentmem_node node;
	struct sysdata sysdata = { 0 };

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		printf("open /dev/persistentmem failed\n");
		return -1;
	}

	sysdata.ota_detect_modes = mode;
	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = offsetof(struct sysdata, ota_detect_modes);
	node.size = sizeof(sysdata.ota_detect_modes);
	node.buf = &sysdata.ota_detect_modes;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
		printf("put sysdata failed\n");
		close(fd);
		return -1;
	}

	return 0;
}

#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBHOST) || defined(CONFIG_BOOT_UPGRADE_SUPPORT_SD)
static int usb_mmc_upgrade_entry(void *dev)
{
	char fota_path[512] = { 0 };
	struct stat st;
	int ret = -ENOENT;

	snprintf(fota_path, sizeof(fota_path), "/media/%s/%s", (char *)dev, BR2_EXTERNAL_HCFOTA_FILENAME);
	if (stat(fota_path, &st) == -1)
		return ret;

	complete(&hcfota_ready);
	printf("==> upgrade from = %s\n", fota_path);
	ret = hcfota_url(fota_path, upgrade_progress_report, 0);
	upgrade_progress_exit(ret);

	return ret;
}

static int mmc_connect_notify(struct notifier_block *self, unsigned long action, void *dev)
{
    //printf("%s:%d: action=%lu, dev=%s\n", __func__, __LINE__, action, (char *)dev);

	if (action == SDMMC_NOTIFY_CONNECT && !strncmp(dev, "mmcblk0boot0", max(strlen(dev), strlen("mmcblk0boot0")))) {
		complete(&mmc_ready);
	}

	return NOTIFY_OK;
}

static int fs_mount_notify(struct notifier_block *self, unsigned long action,
			   void *dev)
{
	char system_data_path[64] = { 0 };
	int upgrade = -1;

	if (action != USB_MSC_NOTIFY_MOUNT)
		return NOTIFY_OK;

	printf("%s:%d ota mode = %d\n", __func__, __LINE__, g_upgrade_mode);
	if (g_upgrade_mode == HCFOTA_REBOOT_OTA_DETECT_USB_HOST && (strncmp((void *)dev, "sd", 2) == 0)) {
		/* U-disk devices found, e.g sda1/sdb1/etc */
		upgrade = usb_mmc_upgrade_entry(dev);
	} else if (g_upgrade_mode == HCFOTA_REBOOT_OTA_DETECT_SD && (strncmp((void *)dev, "mmc", 3) == 0)) {
		/* eMMC/SD-card devices found, e.g mmcblk0 */
		upgrade = usb_mmc_upgrade_entry(dev);
	}

	set_ota_detect_mode(HCFOTA_REBOOT_OTA_DETECT_NONE);

	if (upgrade == 0) {
		printf("%s:%d: upgrade success, just reboot to launch new firmware\n", __func__, __LINE__);
		/* upgrade success, just reboot to launch new firmware */
		reset();
	} else if (upgrade == -ENOENT) {
		/* No firmware to upgrade */
		printf("%s:%d:%s No firmware found\n", __func__, __LINE__, (char *)dev);
	} else if (upgrade == HCFOTA_ERR_VERSION) {
		/* Firmware version check fail */
		printf("%s:%d:%s version check fail, just launch from old firmware\n", __func__, __LINE__, (char *)dev);
		complete(&upgrade_done);
	} else if (upgrade < 0) {
		/* upgrade fail */
		printf("%s:%d: upgrade fail, please retry plug-out-in U disk/sd\n", __func__, __LINE__);
	}

	return NOTIFY_OK;
}
static struct notifier_block fs_mount = {
       .notifier_call = fs_mount_notify,
       .priority = 5,
};
static struct notifier_block mmc_connect = {
       .notifier_call = mmc_connect_notify,
       .priority = 5,
};

static int usb_connect_notify(struct notifier_block *self, unsigned long action,
			      void *dev)
{
	if (action == USB_MSC_DEV_NOTIFY_CONNECT)
		complete(&udisk_connect_ready);
	return NOTIFY_OK;
}
static struct notifier_block udisk_connect = {
       .notifier_call = usb_connect_notify,
       .priority = 5,
};
#endif

static int do_hcfota_upgrade(unsigned int ota_mode)
{
	int i = 0;
	unsigned long usbhost_detect_timeout;
	char *execlude_usb[] = {
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

	char *execlude_mmc[] = {
		/* mmc/sd support */
		"hc15_mmc_device",
		"hcmmc_device",
		};

	if (ota_mode == HCFOTA_REBOOT_OTA_DETECT_NONE)
		return 0;

	/* do upgrade */
 	for (i = 0; i < 8; i++) {
		g_upgrade_mode = hcfota_reboot_get_ota_detect_mode_priority(ota_mode, i);

		switch (g_upgrade_mode) {
#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBDEVICE)
		case HCFOTA_REBOOT_OTA_DETECT_USB_DEVICE: {
			if (module_init2("all", 2, execlude_mmc) != 0) {
				break;
			}

			if (prepare_ramdisk() != 0) {
				printf("fail to prepare ramdisk for usb device connection\n");
				break;
			}

			if (upgrade_from_usb_device() == 0) {
				printf("usb device mode upgrade success, reseting!\n");
				reset();
			} else {
				free_ramdisk();
			}

			break;
		}
#endif

#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBHOST)
		case HCFOTA_REBOOT_OTA_DETECT_USB_HOST: {
			if (boot_is_startup_from_external_partitions()) {
				init_completion(&mmc_ready);
				sys_register_notify(&mmc_connect);
				module_init("hc15_mmc_device");
				module_init("hcmmc_device");
				if (wait_for_completion_timeout(&mmc_ready, CONFIG_BOOT_HCFOTA_TIMEOUT) == 0) {
					printf("mmc device not found!\n");
					break;
				}
			}
			init_completion(&hcfota_ready);
			init_completion(&udisk_connect_ready);
			init_completion(&upgrade_done);
			sys_register_notify(&fs_mount);
			sys_register_notify(&udisk_connect);
			if (module_init2("all", 2, execlude_mmc) != 0) {
				break;
			}

			usbhost_detect_timeout = CONFIG_BOOT_UPGRADE_SUPPORT_USBHOST_DETECT_TIMEOUT;
			if (force_ota_mode == ota_mode)
				usbhost_detect_timeout += 10000; //10s: set enough time to detect when force upgrade mode

			if (wait_for_completion_timeout(&udisk_connect_ready, usbhost_detect_timeout) == 0) {
				printf("usbhost check device timeout!\n");
				break;
			}
			if (wait_for_completion_timeout(&hcfota_ready, CONFIG_BOOT_HCFOTA_TIMEOUT) == 0) {
				printf("usbhost upgrade timeout!\n");
				break;
			}
			wait_for_completion(&upgrade_done);
			break;
		}
#endif

#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_SD)
		case HCFOTA_REBOOT_OTA_DETECT_SD: {
			if (boot_is_startup_from_external_partitions()) {
				printf("Do not support upgrade into eMMC/SD-card from eMMC/SD-card");
				printf("Please try to upgrade into eMMC/SD-card from U-Disk");
				break;
			}

			init_completion(&hcfota_ready);
			init_completion(&upgrade_done);
			sys_register_notify(&fs_mount);
			if (module_init2("all", 7, execlude_usb) != 0) {
				break;
			}
			if (wait_for_completion_timeout(&hcfota_ready, CONFIG_BOOT_HCFOTA_TIMEOUT) == 0) {
				printf("sd/emmc upgrade timeout\n");
				break;
			}
			wait_for_completion(&upgrade_done);
			break;
		}
#endif

#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_NETWORK)
		case HCFOTA_REBOOT_OTA_DETECT_NETWORK: {
			printf("Not support upgrade from network!\n");
			break;
		}
#endif
		default:
			break;
		}
	}

	return 0;
}

#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBDEVICE)
static void *buffer = NULL;
static int is_mmz = 0;
static int make_ramdisk(int minor, uint32_t nsectors, uint16_t sectsize, uint8_t rdflags)
{
	int ret = 0;

	buffer = mmz_malloc(0, sectsize * nsectors);
	if (buffer == NULL) {
		printf("Not enough memory from MMZ, try system memory!\n");
		buffer = malloc(sectsize * nsectors);
		if (!buffer) {
			printf("Not enough memory from system, quit!\n");
			return -ENOMEM;
		}
	} else {
		is_mmz = 1;
	}
	memset(buffer, 0, sectsize * nsectors);
	ret = ramdisk_register(minor, buffer, nsectors, sectsize, rdflags);
	if (ret < 0) {
		if (is_mmz)
			mmz_free(0, buffer);
		else
			free(buffer);
		return ret;
	}

	return 0;
}

static void free_ramdisk(void)
{
	if (buffer != NULL) {
		if (is_mmz)
			mmz_free(0, buffer);
		else
			free(buffer);
	}
}

static int prepare_ramdisk(void)
{
	struct fat_format_s fmt = FAT_FORMAT_INITIALIZER;
	int usb_port = CONFIG_BOOT_USBD_UPGRADE_USBD_PORT_NUM;
	int ret = 0;
	char *path[] = { "/dev/ram0", };
	char *execlude_mmc[] = {
		/* mmc/sd support */
		"hc15_mmc_device",
		"hcmmc_device",
		};

	if (module_init2("all", 2, execlude_mmc) != 0) {
		return -1;
	}

	ret = make_ramdisk(0, 36864, 512, RDFLAG_WRENABLED | RDFLAG_FUNLINK);
	if (ret < 0) {
		printf("error make ramdisk\n");
		return ret;
	}

	ret = mkfatfs("/dev/ram0", &fmt);
	if (ret < 0) {
		printf("error mkfatfs\n");
		return ret;
	}

	mount("/dev/ram0", "/mnt/ram0", "vfat", 0, NULL);

	hcusb_set_mode(usb_port, MUSB_PERIPHERAL);
	hcusb_gadget_msg_specified_init(get_udc_name(usb_port), path, 1);

	return 0;
}
#else
static int prepare_ramdisk(void)
{
	return -1;
}
#endif

int upgrade_detect(int *execute_from_ram)
{
	force_ota_mode = upgrade_all_modes();

#if defined(CONFIG_BOOT_AUTO_UPGRADE)
	unsigned int ota_mode = HCFOTA_REBOOT_OTA_DETECT_NONE;

#if defined(CONFIG_BOOT_UPGRADE_SUPPORT_USBHOST)
	ota_mode |= HCFOTA_REBOOT_OTA_DETECT_USB_HOST;
#elif defined(CONFIG_BOOT_UPGRADE_SUPPORT_SD)
	ota_mode |= HCFOTA_REBOOT_OTA_DETECT_SD;
#endif

#ifdef CONFIG_BOOT_STANDBY
	int fd_standby;
	standby_bootup_mode_e temp = 0;
	fd_standby = open("/dev/standby", O_RDWR);
	if (fd_standby < 0) {
		log_e("Open /dev/standby failed!\n");
		return -ENODEV;
	}
	ioctl(fd_standby, STANDBY_GET_BOOTUP_MODE, &temp);
	log_d("STANDBY_GET_BOOTUP_MODE temp =%d\n", temp);
	if (temp == STANDBY_BOOTUP_COLD_BOOT)
		do_hcfota_upgrade(ota_mode);
	close(fd_standby);
#else
	do_hcfota_upgrade(ota_mode);
#endif

#else
	char ch;

	*execute_from_ram = false;

	if (upgrade_detect_key()) {
		return upgrade_force();
	}

	do_hcfota_upgrade(get_ota_detect_mode());

	ch = wait_any_key_pressed("Press 'r' to EXECUTE or 'space/enter key' to UPGRADE from usb device mode");
	if (ch == 0) {
		return 0;
	} else if (ch == 114 || ch == 82) {
		if (prepare_ramdisk() == 0) {
			*execute_from_ram = true;
		}
		return 0;
	} else if (ch == 32 || ch == 13) {
		unsigned long ota_mode = 0;
		ota_mode |= hcfota_reboot_ota_detect_mode_priority(HCFOTA_REBOOT_OTA_DETECT_USB_DEVICE, 0);
		return do_hcfota_upgrade(ota_mode);
	}

#endif
	return 0;
}
