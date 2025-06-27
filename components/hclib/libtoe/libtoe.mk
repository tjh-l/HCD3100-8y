################################################################################
#
# libtoe
#
################################################################################
LIBTOE_VERSION = 7b4e1b770ff97ab6ab131578b946a2c24cbe2c01
LIBTOE_SITE_METHOD = git
LIBTOE_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libtoe.git
LIBTOE_DEPENDENCIES = kernel

LIBTOE_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

LIBTOE_INSTALL_STAGING = YES

LIBTOE_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBTOE_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBTOE_VERSION)


LIBTOE_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(LIBTOE_MAKE_ENV) $(MAKE) $(LIBTOE_MAKE_FLAGS) -C $(@D) clean
LIBTOE_BUILD_CMDS = $(TARGET_MAKE_ENV) $(LIBTOE_MAKE_ENV) $(MAKE) $(LIBTOE_MAKE_FLAGS) -C $(@D) all
LIBTOE_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(LIBTOE_MAKE_ENV) $(MAKE) $(LIBTOE_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
