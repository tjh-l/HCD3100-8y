include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

include $(BR2_MKLINK)

# Strip quotes and then whitespaces
qstrip=$(strip $(subst ",,$(1)))
#"))

target-y := boardtest

all : $(target-y).bin FORCE

CUR_APP_DIR ?= ${shell pwd}

SYS_DIR = -I$(CUR_APP_DIR)/hcboardtest_app \
		-I$(CUR_APP_DIR)/hcboardtest_app/channel/main_page \
		-I$(CUR_APP_DIR)/hcboardtest_app/boardtest_test \
		-I$(STAGING_DIR)/usr/include \
		
LVGL_HEADERS = -I$(STAGING_DIR)/usr/include/lvgl/ \
		-I$(STAGING_DIR)/usr/include/lvgl/lvgl \

CFLAGS ?= -O3 $(SYS_DIR)/ -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare
CFLAGS += $(LVGL_HEADERS) -include $(AUTO_CONFIG)
LDFLAGS ?= -lm -llvgl

# main-y += main.o

# obj-y += main.o

$(target-y).bin: $(target-y).out
	$(OBJCOPY) $(objcopy-flags-y) -O binary $< $@

$(target-y).out: built-in.o
	$(Q)$(CC) -E $(overlay-ldscript-flags-y) $(dynload-ldscript-flags-y) -P - < $(TOPDIR)/components/ld/ldscript-overlay.ld -o ldscript-overlay.ld
	$(Q)$(LD) $(ldflags-y) -o $@ -Map $@.map --start-group $^ $(ldlibs-y) --whole-archive $(wholearchivelibs-y) --no-whole-archive --end-group

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

install: FORCE
	install -m 0664 $(target-y).out $(IMAGES_DIR)/
	install -m 0664 $(target-y).out.map $(IMAGES_DIR)/
	install -m 0664 $(target-y).bin $(IMAGES_DIR)/

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	-rm -f $(target-y).out
	-rm -f $(target-y).out.map
	-rm -f $(target-y).bin

.PHONY: FORCE

FORCE:

endif
