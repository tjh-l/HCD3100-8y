#ifndef __HYNITRON_UPDATE_FIRMWARE_H__
#define __HYNITRON_UPDATE_FIRMWARE_H__

//#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
//#include <mach/irqs.h>

//#include <linux/syscalls.h>
//#include <asm/unistd.h>
//#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>

//#include <linux/file.h>
//#include <linux/fs.h>
//#include <linux/proc_fs.h>
//

int cst3xx_firmware_info(struct hynitron_ts_data *client);
int hyn_firmware_info(struct hynitron_ts_data *client);
int hyn_boot_update_fw(struct hynitron_ts_data *client);
int hyn_detect_main_iic(struct hynitron_ts_data *client);
int hyn_detect_bootloader(struct hynitron_ts_data *client);
void hyn_init_factory_test_init(struct hynitron_ts_data *client);
#if HYN_AUTO_FACTORY_TEST_EN
int hyn_factory_touch_test(void);
#endif
#if HYN_SYS_AUTO_SEARCH_FIRMWARE
int hyn_sys_auto_search_firmware(void);
#endif
#endif

