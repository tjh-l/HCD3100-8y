#include <string.h>
#include <kernel/module.h>
#include <kernel/vfs.h>
#include <kernel/ld.h>
#include <hcuapi/standby.h>
#include "standby_priv.h"
#include <stdio.h>
#include <kernel/io.h>

int standby_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	int rc = 0;

	switch (cmd) {
	case STANDBY_SET_WAKEUP_BY_IR:
		rc = standby_set_wakeup_by_ir((struct standby_ir_setting *)arg);
		break;

	case STANDBY_SET_WAKEUP_BY_GPIO:
		rc = standby_set_wakeup_by_gpio((struct standby_gpio_setting *)arg);
		break;

	case STANDBY_SET_WAKEUP_BY_SARADC:
		rc = standby_set_wakeup_by_saradc((struct standby_saradc_setting *)arg);
		break;

	case STANDBY_DDR_SCAN:
		standby_ddr_scan();
		break;

	case STANDBY_ENTER:
		standby_enter();
		break;

	case STANDBY_SET_PWROFF_DDR:
		rc = standby_set_ddr((struct standby_pwroff_ddr_setting *)arg);
		break;

	case STANDBY_LOCKER_REQUEST:
		rc = standby_request((struct standby_locker *)arg);
		break;

	case STANDBY_LOCKER_RELEASE:
		rc = standby_release((struct standby_locker *)arg);
		break;
	case STANDBY_SET_WAKEUP_TIME:
		standby_set_wake_up_time((uint32_t )arg);
		break;
	default:
		break;
	}
	return rc;
}

int standby_init(void)
{
	size_t bss_sz = (size_t)((void *)&__STANDBY_BSS_START -
				 (void *)&__STANDBY_BSS_END);

	memset((void *)&__STANDBY_BSS_START, 0, bss_sz);
	standby_get_dts_param();
	standby_lock_list_init();

	return 0;
}
