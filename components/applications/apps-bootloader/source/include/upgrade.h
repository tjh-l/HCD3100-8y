#ifndef _BOOT_UPGRADE_H
#define _BOOT_UPGRADE_H

#include <generated/br2_autoconf.h>

#ifdef CONFIG_BOOT_UPGRADE
#include <hcfota.h>
int upgrade_detect(int *execute_from_ram);
int upgrade_force(void);
int upgrade_progress_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata);
void upgrade_progress_init(void);
void upgrade_progress_exit(int is_success);
#else
static int upgrade_detect(int *execute_from_ram)
{
	*execute_from_ram = 0;
	return -1;
}
static int upgrade_force(void)
{
	return -1;
}
#endif

int upgrade_from_usb_device(void);

#endif
