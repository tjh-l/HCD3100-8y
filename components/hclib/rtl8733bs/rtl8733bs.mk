################################################################################
#
# rtl8733bs
#
################################################################################
RTL8733BS_VERSION = 6aa5c14dacb614500a213b741951bed3ff0e9b26
RTL8733BS_SITE_METHOD = git
RTL8733BS_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/rtl8733bs.git
RTL8733BS_DEPENDENCIES = kernel

RTL8733BS_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

RTL8733BS_INSTALL_STAGING = YES

RTL8733BS_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(RTL8733BS_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(RTL8733BS_VERSION)


RTL8733BS_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(RTL8733BS_MAKE_ENV) $(MAKE) $(RTL8733BS_MAKE_FLAGS) -C $(@D) clean
RTL8733BS_BUILD_CMDS = $(TARGET_MAKE_ENV) $(RTL8733BS_MAKE_ENV) $(MAKE) $(RTL8733BS_MAKE_FLAGS) -C $(@D) all
RTL8733BS_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(RTL8733BS_MAKE_ENV) $(MAKE) $(RTL8733BS_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
