################################################################################
#
# libswxf
#
################################################################################

LIBSWXF_VERSION = 3a79d1bc1a18bce39c68cbe4899f1a7ca6169e42
LIBSWXF_SITE_METHOD = git
LIBSWXF_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libswxf.git
LIBSWXF_DEPENDENCIES = kernel

LIBSWXF_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

LIBSWXF_INSTALL_STAGING = YES

LIBSWXF_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBSWXF_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBSWXF_VERSION)

LIBSWXF_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(LIBSWXF_MAKE_ENV) $(MAKE) $(LIBSWXF_MAKE_FLAGS) -C $(@D) clean
LIBSWXF_BUILD_CMDS = $(TARGET_MAKE_ENV) $(LIBSWXF_MAKE_ENV) $(MAKE) $(LIBSWXF_MAKE_FLAGS) -C $(@D) all
LIBSWXF_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(LIBSWXF_MAKE_ENV) $(MAKE) $(LIBSWXF_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
