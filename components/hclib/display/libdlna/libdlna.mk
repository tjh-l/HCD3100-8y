LIBDLNA_VERSION = 3908ab1ee6c8d46a3a1808f7782c6c55fc0e746a
LIBDLNA_SITE_METHOD = git
LIBDLNA_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libdlna.git
LIBDLNA_DEPENDENCIES = kernel newlib pthread wolfssl libcurl cjson

LIBDLNA_CONF_OPTS += -DHCRTOS=ON


LIBDLNA_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBDLNA_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBDLNA_VERSION)

define LIBDLNA_POST_BUILD
	$(TARGET_CROSS)strip -g -S -d $(@D)/libdlna.a
	$(TARGET_CROSS)strip -g -S -d $(@D)/libdial.a
endef

LIBDLNA_POST_BUILD_HOOKS += LIBDLNA_POST_BUILD

define LIBDLNA_INSTALL_PREBUILT
	$(INSTALL) -D -m 0664 $(@D)/include/hccast/dlna_api.h $(PREBUILT_DIR)/usr/include/dlna/dlna_api.h
	$(INSTALL) -D -m 0664 $(@D)/include/hccast/dial_api.h $(PREBUILT_DIR)/usr/include/dlna/dial_api.h
	$(INSTALL) -D -m 0664 $(@D)/libdlna.a $(PREBUILT_DIR)/usr/lib/libdlna.a
	$(INSTALL) -D -m 0664 $(@D)/libdial.a $(PREBUILT_DIR)/usr/lib/libdial.a
endef

LIBDLNA_POST_INSTALL_TARGET_HOOKS += LIBDLNA_INSTALL_PREBUILT

LIBDLNA_INSTALL_STAGING = YES

$(eval $(cmake-package))
