#ifndef __HC_ADC_GET_ADJUST_H
#define __HC_ADC_GET_ADJUST_H

int __flash_otp_write_adc_calibration(uint8_t val);
int __bootinfo_write_adc_calibration(uint8_t val);
int __flash_otp_read_adc_calibration(uint8_t *val);
int __sysdata_read_adc_calibration(uint8_t *val);
int __efuse_read_adc_calibration(uint8_t *val);

#endif
