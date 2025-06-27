include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

include $(BR2_MKLINK)

cflags-y += -I$(srctree)include/

# Strip quotes and then whitespaces
qstrip=$(strip $(subst ",,$(1)))
#"))

ifeq ($(CONFIG_SOC_HC15XX),y)
target-y := hc15xx_jtag_updater
else ifeq ($(CONFIG_SOC_HC16XX),y)
target-y := hc16xx_jtag_updater
endif

export target-y

all : $(target-y).bin FORCE

ifeq ($(CONFIG_SPINANDWR_SELFCOMPRESSED),y)
all : $(target-y).z.bin
endif

main-y = main.o libjtag_printf.o usb_upgrade.o uart_upgrade.o

obj-y += main.o

$(target-y).bin: $(target-y).out
	$(OBJCOPY) $(objcopy-flags-y) -O binary $< $@

$(target-y).out: built-in.o
	$(Q)$(CC) -E $(overlay-ldscript-flags-y) $(dynload-ldscript-flags-y) -P - < $(TOPDIR)/components/ld/ldscript-overlay.ld -o ldscript-overlay.ld
	$(Q)$(LD) $(ldflags-y) -o $@ -Map $@.map --start-group $^ $(ldlibs-y) --whole-archive $(wholearchivelibs-y) --no-whole-archive --end-group

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

$(target-y).z.bin: $(target-y).z.out FORCE
	$(OBJCOPY) -O binary $< $@

$(target-y).z.out: $(target-y).bin.z calc_vmlinuz_load_addr FORCE
	$(Q)$(MAKE) $(build)=boot-compressed

$(target-y).bin.z: $(target-y).bin FORCE
ifeq ($(CONFIG_SPINANDWR_SELFCOMPRESSED_LZMA),y)
	lzma -zkf -c $< > $@
else ifeq ($(CONFIG_SPINANDWR_SELFCOMPRESSED_LZO1X),y)
	$(HOST_DIR)/bin/hcprecomp2 $< $@
endif

calc_vmlinuz_load_addr: $(srctree)/boot-compressed/calc_vmlinuz_load_addr.c
	gcc -o $@ $^

install: FORCE
ifeq ($(CONFIG_SPINANDWR_SELFCOMPRESSED),y)
	install -m 0664 $(target-y).z.out $(IMAGES_DIR)/$(target-y).out
	install -m 0664 $(target-y).z.bin $(IMAGES_DIR)/$(target-y).bin
	install -m 0664 $(target-y).out   $(IMAGES_DIR)/$(target-y).uncompress.out
	install -m 0664 $(target-y).bin   $(IMAGES_DIR)/$(target-y).uncompress.bin
	install -m 0664 $(target-y).z.out $(TOPDIR)/build/tools/$(target-y).out
	install -m 0664 $(target-y).z.bin $(TOPDIR)/build/tools/$(target-y).bin
else
	install -m 0664 $(target-y).out $(IMAGES_DIR)/
	install -m 0664 $(target-y).bin $(IMAGES_DIR)/
	install -m 0664 $(target-y).out $(TOPDIR)/build/tools/
	install -m 0664 $(target-y).bin $(TOPDIR)/build/tools/
endif

clean: FORCE
	$(Q)$(MAKE) $(clean)=.
	$(Q)$(MAKE) $(clean)=boot-compressed
	-rm -f $(target-y).out
	-rm -f $(target-y).out.map
	-rm -f $(target-y).bin
	-rm -f $(target-y).z.out
	-rm -f $(target-y).z.out.map
	-rm -f $(target-y).z.bin
	-rm -f calc_vmlinuz_load_addr

.PHONY: FORCE

FORCE:

endif
