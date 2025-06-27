#include <fcntl.h>
#include <hudi_com.h>
#include <hudi_flash.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/unistd.h>
#include <sys/stdio.h>
#include <stdio.h>
#include <string.h>
#include <hcuapi/efuse.h>
#include <kernel/bootinfo.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include "hc_adc_get_calibration.h"
#include <generated/br2_autoconf.h>

int __attribute__((weak)) sys_is_adc_calibration_valid(void) { return 0; }
int __attribute__((weak)) hudi_flash_open(hudi_handle *handle, hudi_flash_type_e type) { return -1; }
int __attribute__((weak)) hudi_flash_close(hudi_handle handle) { return -1; }
int __attribute__((weak)) hudi_flash_otp_read(hudi_handle handle, hudi_flash_otp_reg_e bank, unsigned int offset, unsigned char *data, unsigned int len) { return -1; }
int __attribute__((weak)) hudi_flash_otp_write(hudi_handle handle, hudi_flash_otp_reg_e bank, unsigned int offset, unsigned char *data, unsigned int len) { return -1; }

#define CALIBRATION_VALID_OFFSET 1
#define CALIBRATION_VALUE_OFFSET 2

int __flash_otp_write_adc_calibration(uint8_t val)
{
	void *hdl = NULL;
	hudi_flash_type_e type = 0;
	unsigned char tmp[256];

#ifdef CONFIG_KEY_ADC_SAVE_STORAGE_NOR_OTP
	type = HUDI_FLASH_TYPE_NOR;
#elif defined(CONFIG_KEY_ADC_SAVE_STORAGE_NAND_OTP)
	type = HUDI_FLASH_TYPE_NAND;
#endif
	if (0 != hudi_flash_open(&hdl, type)) {
		printf("hudi flash open fail\n");
		return -1;
	}

	hudi_flash_otp_read(hdl, HUDI_FLASH_OTP_REG3, 0, tmp, 256);
	tmp[CALIBRATION_VALID_OFFSET] = 1;
	tmp[CALIBRATION_VALUE_OFFSET] = val;
	hudi_flash_otp_write(hdl, HUDI_FLASH_OTP_REG3, 0, tmp, 256);

	hudi_flash_close(hdl);
	return 0;
}

int __flash_otp_read_adc_calibration(uint8_t *val)
{
	void *hdl = NULL;
	hudi_flash_type_e type = 0;
	unsigned char tmp[256] = { 0 };

#ifdef CONFIG_KEY_ADC_SAVE_STORAGE_NOR_OTP
	type = HUDI_FLASH_TYPE_NOR;
#elif defined(CONFIG_KEY_ADC_SAVE_STORAGE_NAND_OTP)
	type = HUDI_FLASH_TYPE_NAND;
#endif
	if (0 != hudi_flash_open(&hdl, type)) {
		printf("hudi flash open fail\n");
		return -1;
	}

	hudi_flash_otp_read(hdl, HUDI_FLASH_OTP_REG3, 0, tmp, 256);
	hudi_flash_close(hdl);

	if (tmp[CALIBRATION_VALID_OFFSET] == 1 && tmp[CALIBRATION_VALUE_OFFSET] != 0) {
		*val = tmp[CALIBRATION_VALUE_OFFSET];
		return 0;
	}

	return -1;
}

int __bootinfo_write_adc_calibration(uint8_t val)
{
	struct bootinfo *pbootinfo = (struct bootinfo *)0xA0000000;
	pbootinfo->adc_calibration = val;
	return 0;
}

int __sysdata_read_adc_calibration(uint8_t *val)
{
	uint8_t tmp = 0;

	if (!sys_get_sysdata_adc_adjust_value(&tmp) && tmp != 0) {
		*val = tmp;
		return 0;
	}

	return -1;
}

int __efuse_read_adc_calibration(uint8_t *val)
{
	struct hc_efuse_bit_map bitmap;

	if (sys_is_adc_calibration_valid() == 0)
		return -1;

	int fd = open("/dev/efuse", O_RDWR);
	if (fd < 0) {
		printf("can't find /dev/efuse\n");
		return -1;
	}

	memset(&bitmap, 0, sizeof(struct hc_efuse_bit_map));
	ioctl(fd, EFUSE_DUMP, (uint32_t)&bitmap);

	close(fd);

	if ((bitmap.customer.content0 & 0xff) != 0) {
		*val = bitmap.customer.content0 & 0xff;
		return 0;
	}

	return -1;
}

