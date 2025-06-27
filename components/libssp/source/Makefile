include $(BR2_MKENTRY)

ifeq ($(skip-makefile),)

ssp-y  = ssp.o

obj-y += ssp.o

TARGETS = libssp.a

all : $(TARGETS) FORCE

libssp.a: built-in.o
	$(Q)$(AR) -rc $@ $^

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -d -m 0775 $(STAGING_DIR)/usr/lib
	install -m 0664 libssp.a $(STAGING_DIR)/usr/lib/libssp.a
	install -m 0664 libssp.a $(STAGING_DIR)/usr/lib/libssp_nonshared.a

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	rm -f $(TARGETS)

.PHONY: FORCE

FORCE:

endif	# skip-makefile
