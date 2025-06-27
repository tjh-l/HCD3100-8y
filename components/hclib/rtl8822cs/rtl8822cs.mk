################################################################################
#
# rtl8822cs
#
################################################################################
RTL8822CS_VERSION = 7e70dce1f1dd68d28f4bfbfc5a2db1cc117abd1e
RTL8822CS_SITE_METHOD = git
RTL8822CS_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/rtl8822cs.git
RTL8822CS_DEPENDENCIES = kernel

RTL8822CS_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

RTL8822CS_INSTALL_STAGING = YES

RTL8822CS_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(RTL8822CS_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(RTL8822CS_VERSION)


RTL8822CS_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(RTL8822CS_MAKE_ENV) $(MAKE) $(RTL8822CS_MAKE_FLAGS) -C $(@D) clean
RTL8822CS_BUILD_CMDS = $(TARGET_MAKE_ENV) $(RTL8822CS_MAKE_ENV) $(MAKE) $(RTL8822CS_MAKE_FLAGS) -C $(@D) all
RTL8822CS_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(RTL8822CS_MAKE_ENV) $(MAKE) $(RTL8822CS_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
