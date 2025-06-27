################################################################################
#
# rtl8723bs
#
################################################################################
RTL8723BS_VERSION = 18209d7c3a300b3740d2482421cb4e1059ee4e75
RTL8723BS_SITE_METHOD = git
RTL8723BS_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/rtl8723bs.git
RTL8723BS_DEPENDENCIES = kernel

RTL8723BS_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

RTL8723BS_INSTALL_STAGING = YES

RTL8723BS_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(RTL8723BS_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(RTL8723BS_VERSION)

RTL8723BS_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(RTL8723BS_MAKE_ENV) $(MAKE) $(RTL8723BS_MAKE_FLAGS) -C $(@D) clean
RTL8723BS_BUILD_CMDS = $(TARGET_MAKE_ENV) $(RTL8723BS_MAKE_ENV) $(MAKE) $(RTL8723BS_MAKE_FLAGS) -C $(@D) all
RTL8723BS_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(RTL8723BS_MAKE_ENV) $(MAKE) $(RTL8723BS_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
