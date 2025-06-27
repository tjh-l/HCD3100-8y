include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

cflags-y += -I$(srctree)
cflags-y += -I$(srctree)/src/
cflags-y += -I$(srctree)/include/

target-y += libuvc.a

obj-$(BR2_PACKAGE_LIBUVC) += src/

all : $(target-y)

libuvc.a: built-in.o
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -d -m 0775 $(STAGING_DIR)/usr/include/libuvc/
	cp $(srctree)/include/libuvc/libuvc.h $(STAGING_DIR)/usr/include/libuvc/
	cp $(srctree)/include/libuvc/libuvc_config.h $(STAGING_DIR)/usr/include/libuvc/
	install -d -m 0775 $(STAGING_DIR)/usr/lib
	install -m 0664 $(target-y) $(STAGING_DIR)/usr/lib/

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	-rm -f libuvc.a

.PHONY: FORCE

FORCE:

endif	# skip-makefile
