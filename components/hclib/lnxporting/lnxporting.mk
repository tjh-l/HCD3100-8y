################################################################################
#
# lnxporting
#
################################################################################
LNXPORTING_VERSION = 75acb4af24e83be106590df907da40f846558b83
LNXPORTING_SITE_METHOD = git
LNXPORTING_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/linux-porting.git
LNXPORTING_DEPENDENCIES = kernel

LNXPORTING_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

LNXPORTING_INSTALL_STAGING = YES

LNXPORTING_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LNXPORTING_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LNXPORTING_VERSION)


LNXPORTING_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(LNXPORTING_MAKE_ENV) $(MAKE) $(LNXPORTING_MAKE_FLAGS) -C $(@D) clean
LNXPORTING_BUILD_CMDS = $(TARGET_MAKE_ENV) $(LNXPORTING_MAKE_ENV) $(MAKE) $(LNXPORTING_MAKE_FLAGS) -C $(@D) all
LNXPORTING_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(LNXPORTING_MAKE_ENV) $(MAKE) $(LNXPORTING_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
