include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

cflags-y += -I$(srctree)
cflags-y += -I$(srctree)/src/

target-y += libuac.a

obj-$(BR2_PACKAGE_LIBUAC) += src/

all : $(target-y)

libuac.a: built-in.o
	$(Q)rm -f $@
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -d -m 0775 $(STAGING_DIR)/usr/include/libuac/
	install -d -m 0775 $(STAGING_DIR)/usr/lib
	install -m 0664 $(target-y) $(STAGING_DIR)/usr/lib/

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	-rm -f libuac.a

.PHONY: FORCE

FORCE:

endif	# skip-makefile
