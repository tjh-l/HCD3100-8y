################################################################################
#
# hctinyplayer
#
################################################################################

HCTINYPLAYER_VERSION = 59ae0947ed254d411ca41739873542457e75ec58
HCTINYPLAYER_SITE_METHOD = git
HCTINYPLAYER_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/hctinyplayer.git
HCTINYPLAYER_DEPENDENCIES += kernel pthread

HCTINYPLAYER_CONF_OPTS += -DHCRTOS=ON

HCTINYPLAYER_INSTALL_STAGING = YES

HCTINYPLAYER_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(HCTINYPLAYER_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(HCTINYPLAYER_VERSION)

HCTINYPLAYER_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(HCTINYPLAYER_MAKE_ENV) $(MAKE) $(HCTINYPLAYER_MAKE_FLAGS) -C $(@D) clean
HCTINYPLAYER_BUILD_CMDS = $(TARGET_MAKE_ENV) $(HCTINYPLAYER_MAKE_ENV) $(MAKE) $(HCTINYPLAYER_MAKE_FLAGS) -C $(@D) all

define HCTINYPLAYER_INSTALL_PREBUILTS
	mkdir -p $(PREBUILT_DIR)/usr/include/hctinyplayer
	cp -arf $(@D)/registry.h $(PREBUILT_DIR)/usr/include/hctinyplayer/
	mkdir -p $(PREBUILT_DIR)/usr/lib
	cp -arf $(@D)/libhctinyplayer.a $(PREBUILT_DIR)/usr/lib/
	mkdir -p $(PREBUILT_DIR)/usr/lib/hctinyplugins
	cp -arf $(@D)/libhctinyplugin*.a $(PREBUILT_DIR)/usr/lib/hctinyplugins/
endef
HCTINYPLAYER_POST_INSTALL_STAGING_HOOKS += HCTINYPLAYER_INSTALL_PREBUILTS

$(eval $(cmake-package))
