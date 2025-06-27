LIBHIDALGO_VERSION = afc48a3efc9a11ccf8472420cf650b03d1b6ca87
LIBHIDALGO_SITE_METHOD = git
LIBHIDALGO_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libhidalgo.git
LIBHIDALGO_DEPENDENCIES = kernel newlib pthread

LIBHIDALGO_CONF_OPTS += -DHCRTOS=ON
LIBHIDALGO_INSTALL_STAGING = YES

LIBHIDALGO_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBHIDALGO_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBHIDALGO_VERSION)

define LIBHIDALGO_INSTALL_HEADERS
	$(INSTALL) -d $(PREBUILT_DIR)/usr/include/hidalgo
	$(INSTALL) $(@D)/inc/* -m 0666 $(PREBUILT_DIR)/usr/include/hidalgo
endef
LIBHIDALGO_POST_INSTALL_TARGET_HOOKS += LIBHIDALGO_INSTALL_HEADERS

$(eval $(cmake-package))
