include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

obj-$(CONFIG_CMDS_OS) += os/
obj-$(CONFIG_CMDS_PTHREAD) += pthread/
obj-$(CONFIG_CMDS_NSH) += third_party/nuttx/apps/nshlib/
obj-$(CONFIG_CMDS_LSMOD) += lsmod/
obj-$(CONFIG_CMDS_SOUND_TEST) += sound_test/
obj-$(CONFIG_CMDS_INPUT) += input_event/
obj-$(CONFIG_CMDS_MTD) += mtd/
obj-$(CONFIG_CMDS_SPI) += spi/
obj-$(CONFIG_CMDS_PERSISTENTMEM) += persistentmem/
obj-$(CONFIG_CMDS_POK_TEST) += pok/
obj-$(CONFIG_CMDS_WATCHDOG_TEST) += watchdog/
obj-$(CONFIG_CMDS_EFUSE_DUMP) += efuse/
obj-$(CONFIG_CMDS_ADC_TEST) += saradc/
obj-$(CONFIG_CMDS_PQ) += pq/
obj-$(CONFIG_CMDS_PQ) += dis/
obj-$(CONFIG_CMDS_HCFOTA) += hcfota/
obj-$(CONFIG_CMDS_STANDBY) += standby/
obj-$(CONFIG_CMDS_WS2811) += ws2811/
obj-$(CONFIG_CMDS_SORTING) += sorting/
obj-$(CONFIG_CMDS_VIN_DVP) += vin_dvp/
obj-$(CONFIG_CMDS_USB) += usb/
obj-$(CONFIG_CMDS_PWM) += pwm/
obj-$(CONFIG_CMDS_FB_TEST) += fb/
obj-$(CONFIG_CMDS_WIFI) += wifi/
obj-$(CONFIG_CMD_NETWORK) += network/
obj-$(CONFIG_CMD_UNITY) += unity/
obj-$(CONFIG_CMD_UART) += uart/
obj-$(CONFIG_CMD_UPGRADE_5100) += upgrade_5100/
obj-$(CONFIG_CMDS_RTWPRIV) += rtwpriv/
obj-$(CONFIG_CMDS_IPERF3) += iperf3/
obj-$(CONFIG_CMDS_RTC_AT8536) += rtc/
obj-$(CONFIG_CMD_LCD_TEST) += lcd/
obj-$(CONFIG_CMD_MKGPT) += mkgpt/
obj-$(CONFIG_CMD_DDR_MONITORING) += ddr/
obj-$(CONFIG_CMDS_SQLITE) += sqlite/

TARGETS := libcmds.a

all : $(TARGETS) FORCE

$(TARGETS): built-in.o
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -d -m 0775 $(STAGING_DIR)/usr/lib
	install -m 0664 libcmds.a $(STAGING_DIR)/usr/lib/libcmds.a

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	rm -f $(TARGETS)

.PHONY: FORCE

FORCE:

endif	# skip-makefile
