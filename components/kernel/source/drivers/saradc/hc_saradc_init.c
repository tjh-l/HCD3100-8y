#include <kernel/module.h>
#include <stdio.h>
#include <stdlib.h>
#include "hc_adc_get_calibration.h"
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include <kernel/drivers/hc_clk_gate.h>
#include <generated/br2_autoconf.h>

static int hc_saradc_clk_enable(void)
{
	int ret = 0;

	hc_clk_enable(SAR_ADC_CLK);

	return ret;
}

static void hc_saradc_save_adjust_to_otp(void)
{
	uint8_t persistentmem_adjust = 0;
	uint8_t otp_adjust = 0;

	__bootinfo_write_adc_calibration(0);

	if (!__sysdata_read_adc_calibration(&persistentmem_adjust)) {
		__flash_otp_read_adc_calibration(&otp_adjust);
		__bootinfo_write_adc_calibration(persistentmem_adjust);

		if (abs(otp_adjust - persistentmem_adjust) > 2) {
			__flash_otp_write_adc_calibration(persistentmem_adjust);
		}
		return;
	}

	if (!__flash_otp_read_adc_calibration(&otp_adjust)) {

		__bootinfo_write_adc_calibration(otp_adjust);

		if (abs(otp_adjust - persistentmem_adjust) > 2) {
			sys_set_sysdata_adc_adjust_value(otp_adjust);
		}
		return;
	}

}

static int hc_saradc_init(void)
{
	hc_saradc_clk_enable();

#ifdef CONFIG_KEY_ADC_SAVE_CALIBRATION_TO_OTP
	hc_saradc_save_adjust_to_otp();
#endif

	return 0;
}

module_system(hc_saradc_clk_init, hc_saradc_init, NULL, 3)
