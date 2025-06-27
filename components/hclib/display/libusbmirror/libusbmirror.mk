LIBUSBMIRROR_SUPPORT_SEPARATE_OUTPUT = YES

LIBUSBMIRROR_DEPENDENCIES = kernel newlib pthread libusb
LIBUSBMIRROR_INSTALL_STAGING = YES
LIBUSBMIRROR_EXTRACT_CMDS = \
	rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/hclib/display/libusbmirror/source/ $(@D)

ifeq ($(BR2_PACKAGE_WOLFSSL),y)
LIBUSBMIRROR_DEPENDENCIES += wolfssl
endif

ifeq ($(BR2_PACKAGE_LIBOPENSSL),y)
LIBUSBMIRROR_DEPENDENCIES += libopenssl
endif

LIBUSBMIRROR_CONF_OPTS += -DHCRTOS=ON

define LIBUSBMIRROR_INSTALL_HEADERS
	$(INSTALL) -d $(STAGING_DIR)/usr/include/um
	$(INSTALL) $(@D)/inc/iumirror_api.h -m 0666 $(STAGING_DIR)/usr/include/um
	$(INSTALL) $(@D)/inc/aumirror_api.h -m 0666 $(STAGING_DIR)/usr/include/um
	$(INSTALL) $(@D)/inc/um_api.h -m 0666 $(STAGING_DIR)/usr/include/um
endef

define LIBUSBMIRROR_POST_BUILD
	$(TARGET_CROSS)strip -g -S -d $(@D)/libusbmirror.a
endef

LIBUSBMIRROR_POST_BUILD_HOOKS += LIBUSBMIRROR_POST_BUILD

LIBUSBMIRROR_POST_INSTALL_TARGET_HOOKS += LIBUSBMIRROR_INSTALL_HEADERS

$(eval $(cmake-package))
