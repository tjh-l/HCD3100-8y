################################################################################
#
# mmcdriver
#
################################################################################
MMCDRIVER_VERSION = 509a810a3c0f50fdaf6db9e467150d3164119dec
MMCDRIVER_SITE_METHOD = git
MMCDRIVER_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libmmc.git
MMCDRIVER_DEPENDENCIES = kernel

MMCDRIVER_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

MMCDRIVER_INSTALL_STAGING = YES

MMCDRIVER_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(MMCDRIVER_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(MMCDRIVER_VERSION)


MMCDRIVER_CONFIGURE_CMDS = $(TARGET_MAKE_ENV) $(MMCDRIVER_MAKE_ENV) $(MAKE) $(MMCDRIVER_MAKE_FLAGS) -C $(@D) default_defconfig
MMCDRIVER_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(MMCDRIVER_MAKE_ENV) $(MAKE) $(MMCDRIVER_MAKE_FLAGS) -C $(@D) clean
MMCDRIVER_BUILD_CMDS = $(TARGET_MAKE_ENV) $(MMCDRIVER_MAKE_ENV) $(MAKE) $(MMCDRIVER_MAKE_FLAGS) -C $(@D) all
MMCDRIVER_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(MMCDRIVER_MAKE_ENV) $(MAKE) $(MMCDRIVER_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
