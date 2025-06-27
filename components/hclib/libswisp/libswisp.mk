################################################################################
#
# libswisp
#
################################################################################

LIBSWISP_VERSION = cd4fbda5cb24bd26dc2b952eff3353dc653b5f5a
LIBSWISP_SITE_METHOD = git
LIBSWISP_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libswisp.git
LIBSWISP_DEPENDENCIES = kernel

LIBSWISP_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

LIBSWISP_INSTALL_STAGING = YES

LIBSWISP_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBSWISP_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBSWISP_VERSION)

LIBSWISP_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(LIBSWISP_MAKE_ENV) $(MAKE) $(LIBSWISP_MAKE_FLAGS) -C $(@D) clean
LIBSWISP_BUILD_CMDS = $(TARGET_MAKE_ENV) $(LIBSWISP_MAKE_ENV) $(MAKE) $(LIBSWISP_MAKE_FLAGS) -C $(@D) all
LIBSWISP_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(LIBSWISP_MAKE_ENV) $(MAKE) $(LIBSWISP_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
