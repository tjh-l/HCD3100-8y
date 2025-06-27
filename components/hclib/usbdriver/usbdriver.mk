################################################################################
#
# usbdriver
#
################################################################################
USBDRIVER_VERSION = 14a9417c6a751c85280a0d35b51c84909ae0cde6 
USBDRIVER_SITE_METHOD = git
USBDRIVER_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libusb.git
USBDRIVER_DEPENDENCIES = kernel libusb

USBDRIVER_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

USBDRIVER_INSTALL_STAGING = YES

USBDRIVER_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(USBDRIVER_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(USBDRIVER_VERSION)


USBDRIVER_CONFIGURE_CMDS = $(TARGET_MAKE_ENV) $(USBDRIVER_MAKE_ENV) $(MAKE) $(USBDRIVER_MAKE_FLAGS) -C $(@D) default_defconfig
USBDRIVER_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(USBDRIVER_MAKE_ENV) $(MAKE) $(USBDRIVER_MAKE_FLAGS) -C $(@D) clean
USBDRIVER_BUILD_CMDS = $(TARGET_MAKE_ENV) $(USBDRIVER_MAKE_ENV) $(MAKE) $(USBDRIVER_MAKE_FLAGS) -C $(@D) all
USBDRIVER_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(USBDRIVER_MAKE_ENV) $(MAKE) $(USBDRIVER_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
