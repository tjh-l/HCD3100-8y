include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

CFLAGS_ffplayer_examples.o += -Werror=unused-variable -Werror=unused-parameter

TARGETS := libhc-examples.a

all : $(TARGETS) FORCE

hc-examples-y := ffplayer_examples.o showlogo.o dis_test.o hdmi_tx_test.o h264_es_player.o es_decoder.o
hc-examples-$(BR2_PACKAGE_PREBUILTS_AACELDEC) += fdkaac_test.o
hc-examples-$(BR2_PACKAGE_HDMIRX_EXAMPLES) += wav.o

obj-y = hc-examples.o
ifeq ($(BR2_PACKAGE_HDMI_WIREELSS_EXAMPLE), y)
obj-$(BR2_PACKAGE_HDMI_WIRELSS_DISPLAY_RAW) += hdmi_wireless_display.raw/
obj-$(BR2_PACKAGE_HDMI_WIRELSS_DISPLAY_TCP) += hdmi_wireless_display.tcp/
obj-$(BR2_PACKAGE_HDMI_WIRELSS_DISPLAY_UDP) += hdmi_wireless_display.tcp/
endif

obj-$(BR2_PACKAGE_VIDEO_PBP_EXAMPLES) += video_pbp/

obj-$(BR2_PACKAGE_SCREENSHOT_EXAMPLES) += screenshot_test/

obj-$(BR2_PACKAGE_HCCAST_EXAMPLES) += hccast_test/

obj-$(BR2_PACKAGE_BLUETOOTH_EXAMPLES) += bluetooth/

obj-$(BR2_PACKAGE_HDMIRX_EXAMPLES) += hdmi_rx/

obj-$(BR2_PACKAGE_TVDEC_EXAMPLES) += tv_decoder/

obj-$(BR2_PACKAGE_HC_EXAMPLES_HUDI) += hudi/

obj-$(BR2_PACKAGE_SCREENSHOT_TEST) += screenshot_test/

$(TARGETS): built-in.o
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -D -m 0664 $(TARGETS) $(STAGING_DIR)/usr/lib/$(TARGETS)

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	rm -f $(TARGETS)

.PHONY: FORCE

FORCE:

endif	# skip-makefile
