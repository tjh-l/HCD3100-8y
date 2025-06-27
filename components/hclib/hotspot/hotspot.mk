################################################################################
#
# hotspot
#
################################################################################

HOTSPOT_VERSION = c9f460f1a7a6913ec3fbab20709be5e1b90f0b59
HOTSPOT_SITE_METHOD = git
HOTSPOT_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libhotspot.git
HOTSPOT_LICENSE = LGPL-2.0+
HOTSPOT_LICENSE_FILES = COPYING
HOTSPOT_CONF_OPTS = 

HOTSPOT_DEPENDENCIES += kernel newlib

HOTSPOT_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}" \
		KERNEL_ROOT="$(KERNEL_PKGD)"

HOTSPOT_INSTALL_STAGING = YES

HOTSPOT_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(HOTSPOT_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(HOTSPOT_VERSION)

HOTSPOT_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(HOTSPOT_MAKE_ENV) $(MAKE) $(HOTSPOT_MAKE_FLAGS) -C $(@D) clean
HOTSPOT_BUILD_CMDS = $(TARGET_MAKE_ENV) $(HOTSPOT_MAKE_ENV) $(MAKE) $(HOTSPOT_MAKE_FLAGS) -C $(@D) all
HOTSPOT_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(HOTSPOT_MAKE_ENV) $(MAKE) $(HOTSPOT_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
