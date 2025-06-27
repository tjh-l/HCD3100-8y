LIBMIRACAST_VERSION = 65ae3f12469a44b49af77ddbf038ca6679a812ed
LIBMIRACAST_SITE_METHOD = git
LIBMIRACAST_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libmiracast.git
LIBMIRACAST_DEPENDENCIES = kernel newlib pthread wolfssl

LIBMIRACAST_CONF_OPTS += -DBUILD_LIBRARY_TYPE="STATIC" -DBUILD_OS_TARGET="HCRTOS"

LIBMIRACAST_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBMIRACAST_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBMIRACAST_VERSION)

define LIBMIRACAST_POST_BUILD
	$(TARGET_CROSS)strip -g -S -d $(@D)/libmiracast.a
endef

LIBMIRACAST_POST_BUILD_HOOKS += LIBMIRACAST_POST_BUILD

define LIBMIRACAST_INSTALL_PREBUILT
	$(INSTALL) -D -m 0664 $(@D)/libmiracast.a $(PREBUILT_DIR)/usr/lib/libmiracast.a
	$(INSTALL) -D -m 0664 $(@D)/include/hccast/miracast_api.h $(PREBUILT_DIR)/usr/include/miracast/miracast_api.h
endef

LIBMIRACAST_POST_INSTALL_TARGET_HOOKS += LIBMIRACAST_INSTALL_PREBUILT

LIBMIRACAST_INSTALL_STAGING = YES

$(eval $(cmake-package))
