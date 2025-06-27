LIBYOUTUBE_VERSION = 3224248eb10060e447df6624afc14afa772ded94
LIBYOUTUBE_SITE_METHOD = git
LIBYOUTUBE_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/libyoutube.git
LIBYOUTUBE_DEPENDENCIES = kernel newlib pthread wolfssl libcurl cjson pcre hccast

LIBYOUTUBE_CONF_OPTS += -DHCRTOS=ON

LIBYOUTUBE_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(LIBYOUTUBE_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(LIBYOUTUBE_VERSION)

define LIBYOUTUBE_INSTALL_PREBUILT
	#$(INSTALL) -D -m 0664 $(@D)/include/yt/iptv_inter_yt_api.h $(PREBUILT_DIR)/usr/include/yt/iptv_inter_yt_api.h
	$(INSTALL) -D -m 0664 $(@D)/libiptv-yt.a $(PREBUILT_DIR)/usr/lib/libiptv-yt.a
endef

define LIBYOUTUBE_POST_BUILD
	$(TARGET_CROSS)strip -g -S -d $(@D)/libiptv-yt.a
endef

LIBYOUTUBE_POST_BUILD_HOOKS += LIBYOUTUBE_POST_BUILD

LIBYOUTUBE_POST_INSTALL_TARGET_HOOKS += LIBYOUTUBE_INSTALL_PREBUILT

LIBYOUTUBE_INSTALL_STAGING = YES

$(eval $(cmake-package))
