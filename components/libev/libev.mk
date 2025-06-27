################################################################################
#
# libev
#
################################################################################

LIBEV_VERSION = 4.27
LIBEV_SITE = http://dist.schmorp.de/libev/Attic
LIBEV_SOURCE = libev-$(LIBEV_VERSION).tar.gz
LIBEV_INSTALL_STAGING = YES
LIBEV_LICENSE = BSD-2-Clause or GPL-2.0+
LIBEV_LICENSE_FILES = LICENSE
LIBEV_CONF_OPTS = 
LIBEV_DEPENDENCIES +=  kernel

LIBEV_INSTALL_STAGING = YES

LIBEV_INSTALL_STAGING_OPTS = DESTDIR=$(STAGING_DIR) install


LIBEV_CONF_OPTS += -D__HCRTOS__=ON

# The 'compatibility' event.h header conflicts with libevent
# It's completely unnecessary for BR packages so remove it
define LIBEV_DISABLE_EVENT_H_INSTALL
	$(SED) 's/ event.h//' $(@D)/Makefile.in
endef

LIBEV_POST_PATCH_HOOKS += LIBEV_DISABLE_EVENT_H_INSTALL

LIBEV_POST_INSTALL_STAGING_HOOKS += LIBEV_INSTALL_PREBUILTS

define LIBEV_FIX_HAVE_STRUCT_ADDRINFO
	sed -i 's/#  define EV_CHILD_ENABLE EV_FEATURE_WATCHERS/#  define EV_CHILD_ENABLE 0/g' $(@D)/ev.h
	sed -i 's/# define EV_SIGNAL_ENABLE EV_FEATURE_WATCHERS/# define EV_SIGNAL_ENABLE 0/g' $(@D)/ev.h
	sed -i 's/# define EV_ASYNC_ENABLE EV_FEATURE_WATCHERS/# define EV_ASYNC_ENABLE 0/g' $(@D)/ev.h
	sed -i 's/.* #undef HAVE_POLL .*/#define HAVE_POLL 1/g' $(@D)/config.h
	sed -i 's/.* #undef HAVE_CLOCK_GETTIME .*/#define HAVE_CLOCK_GETTIME 1/g' $(@D)/config.h
	sed -i 's/#   define EV_USE_REALTIME  0/#   define EV_USE_REALTIME  1/g' $(@D)/ev.c
	sed -i 's/#define ecb_attribute(attrlist)        __attribute__ (attrlist)/#define ecb_attribute(attrlist)/g' $(@D)/ev.c
	sed -i 's/return getuid () != geteuid ()//g' $(@D)/ev.c
	sed -i 's/|| getgid () != getegid ();/return 0;/g' $(@D)/ev.c
endef

LIBEV_PRE_BUILD_HOOKS += LIBEV_FIX_HAVE_STRUCT_ADDRINFO

$(eval $(autotools-package))
$(eval $(host-autotools-package))
