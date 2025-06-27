NEWLIB_VERSION = 3.0.0
NEWLIB_SOURCE = newlib-$(NEWLIB_VERSION).tar.gz
NEWLIB_SITE = ftp://sourceware.org/pub/newlib

NEWLIB_CONF_OPTS = --with-newlib --disable-multilib --disable-libgloss \
	CFLAGS_FOR_TARGET="$(TARGET_CFLAGS) -DABORT_PROVIDED -DHAVE_BLKSIZE -DMALLOC_PROVIDED -DREENTRANT_SYSCALLS_PROVIDED -DMISSING_SYSCALL_NAMES -DHAVE_ASSERT_FUNC" \
	CXXFLAGS_FOR_TARGET="$(TARGET_CXXFLAGS) -DABORT_PROVIDED -DHAVE_BLKSIZE -DMALLOC_PROVIDED -DREENTRANT_SYSCALLS_PROVIDED -DMISSING_SYSCALL_NAMES -DHAVE_ASSERT_FUNC"

NEWLIB_DEPENDENCIES = libssp

define NEWLIB_FIX_INCLUDE_LIB_DIR
	mkdir -p $(STAGING_DIR)/usr/include/
	mkdir -p $(STAGING_DIR)/usr/lib/
	find $(STAGING_DIR)/usr/$(GNU_TARGET_NAME)/include/* -maxdepth 0 -not -wholename '$(STAGING_DIR)/usr/$(GNU_TARGET_NAME)/include/pthread.h' | xargs -i cp -rf {} $(STAGING_DIR)/usr/include/
	cp -rf $(STAGING_DIR)/usr/$(GNU_TARGET_NAME)/lib/* $(STAGING_DIR)/usr/lib/
	rm -rf $(STAGING_DIR)/usr/$(GNU_TARGET_NAME)
	mkdir -p $(STAGING_DIR)/usr/include_fix/
	cp -rvf $(NEWLIB_PKGDIR)/porting/include/* $(STAGING_DIR)/usr/include_fix/
endef
NEWLIB_POST_INSTALL_STAGING_HOOKS += NEWLIB_FIX_INCLUDE_LIB_DIR

define NEWLIB_REMOVE_STDATOMIC_H
	if test -f $(@D)/newlib/libc/include/stdatomic.h; then \
		mv $(@D)/newlib/libc/include/stdatomic.h $(@D)/newlib/libc/include/stdatomic.h.bak; \
	fi;
endef
NEWLIB_PRE_CONFIGURE_HOOKS += NEWLIB_REMOVE_STDATOMIC_H

$(eval $(autotools-package))
