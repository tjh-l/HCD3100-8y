cflags-y += -I$(TOPDIR)/components/hudi/source/include

saradc-$(CONFIG_KEY_ADC) += hc_key_adc.o

saradc-$(CONFIG_TOUCH_ADC) += hc_touch_adc.o

saradc-$(CONFIG_POLL_ADC) += hc_query_adc.o

saradc-y += hc_saradc_init.o hc_adc_get_calibration.o

obj-y += saradc.o
