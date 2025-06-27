#include <string.h>
#include <kernel/module.h>
#include <kernel/vfs.h>
#include <kernel/ld.h>
#include <hcuapi/standby.h>
#include "standby_priv.h"
#include <stdio.h>
#include <kernel/io.h>

int standby_get_bootup_mode(enum standby_bootup_mode *mode)
{
	if (REG8_READ(0xb8818a70) == STANDBY_FLAG_WARM_BOOT)
		*mode = STANDBY_BOOTUP_WARM_BOOT;
	else
		*mode = STANDBY_BOOTUP_COLD_BOOT;
	return 0;
}

int standby_get_bootup_slot(unsigned long *slot)
{
	*slot = REG8_READ(0xb8818a71);
	return 0;
}

int standby_set_bootup_slot(unsigned long slot)
{
	REG8_WRITE(0xb8818a71, slot);
	return 0;
}

int __attribute__((weak)) standby_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	printf("ERR: Failed to call standby ioctl, cmd:0x%x\nThe Standby driver is not selected in menuconfig\n", cmd);
	return -1;
}

static int standby_common_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	int rc = 0;

	switch (cmd) {
	case STANDBY_GET_BOOTUP_MODE:
		rc = standby_get_bootup_mode((enum standby_bootup_mode *)arg);
		break;
	case STANDBY_GET_BOOTUP_SLOT:
		rc = standby_get_bootup_slot((unsigned long *)arg);
		break;
	case STANDBY_SET_BOOTUP_SLOT:
		rc = standby_set_bootup_slot((unsigned long)arg);
		break;
	default:
		rc = standby_ioctl(filep, cmd, arg);
		break;
	}
	return rc;
}

static const struct file_operations g_standbyops = {
	.open = dummy_open, /* open */
	.close = dummy_close, /* close */
	.read = dummy_read, /* read */
	.write = dummy_write, /* write */
	.seek = NULL, /* seek */
	.ioctl = standby_common_ioctl, /* ioctl */
	.poll = NULL /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	,
	.unlink = NULL /* unlink */
#endif
};

int __attribute__((weak)) standby_init(void)
{
	return 0;
}

static int standby_common_init(void)
{
	standby_init();
	register_driver("/dev/standby", &g_standbyops , 0666, NULL);
	return 0;
}
module_driver(standby, standby_common_init, NULL, 0)
