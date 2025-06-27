LIBAIRCAST_VERSION = c11493d1cdfb27f529d1efc9b12473bf89cab09a
LIBAIRCAST_SITE_METHOD = git
LIBAIRCAST_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libaircast.git
LIBAIRCAST_DEPENDENCIES = kernel newlib pthread

ifeq ($(BR2_PACKAGE_WOLFSSL),y)
LIBAIRCAST_DEPENDENCIES += wolfssl
endif

ifeq ($(BR2_PACKAGE_LIBOPENSSL),y)
LIBAIRCAST_DEPENDENCIES += libopenssl
endif

LIBAIRCAST_CONF_OPTS += -DHCRTOS=ON


LIBAIRCAST_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBAIRCAST_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBAIRCAST_VERSION)

define LIBAIRCAST_POST_BUILD
	$(TARGET_CROSS)strip -g -S -d $(@D)/libaircast/libaircast.a
endef

LIBAIRCAST_POST_BUILD_HOOKS += LIBAIRCAST_POST_BUILD

LIBAIRCAST_INSTALL_STAGING = YES

$(eval $(cmake-package))
