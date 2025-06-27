################################################################################
#
# rtl8723as
#
################################################################################
RTL8723AS_VERSION = 09339cc60625424c38c7e52c52899cf362d574ce
RTL8723AS_SITE_METHOD = git
RTL8723AS_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/rtl8723as.git
RTL8723AS_DEPENDENCIES = kernel

RTL8723AS_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

RTL8723AS_INSTALL_STAGING = YES

RTL8723AS_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(RTL8723AS_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(RTL8723AS_VERSION)


RTL8723AS_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(RTL8723AS_MAKE_ENV) $(MAKE) $(RTL8723AS_MAKE_FLAGS) -C $(@D) clean
RTL8723AS_BUILD_CMDS = $(TARGET_MAKE_ENV) $(RTL8723AS_MAKE_ENV) $(MAKE) $(RTL8723AS_MAKE_FLAGS) -C $(@D) all
RTL8723AS_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(RTL8723AS_MAKE_ENV) $(MAKE) $(RTL8723AS_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
