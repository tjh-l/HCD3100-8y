################################################################################
#
# libswtwill
#
################################################################################

LIBSWTWILL_VERSION = 066127780c3a339ca9aa5b68cc5b643b06e57465
LIBSWTWILL_SITE_METHOD = git
LIBSWTWILL_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libswtwill.git
LIBSWTWILL_DEPENDENCIES = kernel

LIBSWTWILL_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

LIBSWTWILL_INSTALL_STAGING = YES

LIBSWTWILL_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBSWTWILL_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBSWTWILL_VERSION)

LIBSWTWILL_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(LIBSWTWILL_MAKE_ENV) $(MAKE) $(LIBSWTWILL_MAKE_FLAGS) -C $(@D) clean
LIBSWTWILL_BUILD_CMDS = $(TARGET_MAKE_ENV) $(LIBSWTWILL_MAKE_ENV) $(MAKE) $(LIBSWTWILL_MAKE_FLAGS) -C $(@D) all
LIBSWTWILL_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(LIBSWTWILL_MAKE_ENV) $(MAKE) $(LIBSWTWILL_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
