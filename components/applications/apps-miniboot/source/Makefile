include $(BR2_MKENTRY)
ifeq ($(skip-makefile),)

include $(srctree)/Makefile.link

# Strip quotes and then whitespaces
qstrip=$(strip $(subst ",,$(1)))
#"))

DTS:=$(basename $(call qstrip,$(CONFIG_MINIBOOT_DEFAULT_DEVICE_TREE)))
DTS_SOURCE:=$(call qstrip,$(CONFIG_MINIBOOT_CUSTOM_DTS_PATH))
export DTS DTS_SOURCE

cflags-y += -I$(srctree)include/
cflags-y += -I$(srctree)../../../kernel/source/include
cflags-y += -I$(srctree)../../../kernel/source/include/uapi/
cflags-y += -I$(srctree)../../../kernel/source/arch/mips/include/
cflags-y += -I$(srctree)../../../kernel/source/arch/mips/include/uapi/
cflags-y += -I$(srctree)../../../kernel/source/freertos/include
cflags-y += -I$(srctree)../../../kernel/source/arch/mips/include
cflags-y += -I$(srctree)../../../kernel/source/third_party/linux/include
cflags-y += -nostdlib

ifeq ($(CONFIG_MINIBOOT_LIB_LZMA),y)
cflags-y += -DCONFIG_LIB_LZMA
endif

ifeq ($(CONFIG_MINIBOOT_LIB_GZIP),y)
cflags-y += -DCONFIG_LIB_GZIP
endif

target-y := $(call qstrip,$(CONFIG_APPS_NAME))
export target-y

all : $(target-y).bin FORCE

ifeq ($(CONFIG_MINIBOOT_SELFCOMPRESSED),y)
all : $(target-y).z.bin
endif

main-y = main.o

obj-y += main.o
obj-y += cmd/
obj-y += common/
obj-y += lib/

$(target-y).bin: $(target-y).out
	$(OBJCOPY) -O binary $< $@

$(target-y).out: built-in.o
	$(Q)$(CC) -E $(overlay-ldscript-flags-y) $(dynload-ldscript-flags-y) -P - < $(TOPDIR)/components/ld/ldscript-overlay.ld -o ldscript-overlay.ld
	$(Q)$(LD) $(ldflags-y) -o $@ -Map $@.map --start-group $^ $(ldlibs-y) --whole-archive $(wholearchivelibs-y) --no-whole-archive --end-group

built-in.o: $(DTS).dtb

built-in.o: FORCE
	$(Q)$(MAKE) $(build)=.

$(target-y).z.bin: $(target-y).z.out FORCE
	$(OBJCOPY) -O binary $< $@

$(target-y).z.out: $(target-y).bin.z calc_vmlinuz_load_addr FORCE
	$(Q)$(MAKE) $(build)=boot-compressed

$(target-y).bin.z: $(target-y).bin FORCE
ifeq ($(CONFIG_MINIBOOT_SELFCOMPRESSED_LZMA),y)
	lzma -zkf -c $< > $<.lzma
	cp $<.lzma $@
else ifeq ($(CONFIG_MINIBOOT_SELFCOMPRESSED_LZO1X),y)
	${TOPDIR}/build/tools/hcprecomp2 $< $<.lzo1x
	cp $<.lzo1x $@
else ifeq ($(CONFIG_MINIBOOT_SELFCOMPRESSED_GZIP),y)
	gzip -kf9 $<
	cp $<.gz $@
endif

calc_vmlinuz_load_addr: $(srctree)/boot-compressed/calc_vmlinuz_load_addr.c
	gcc -o $@ $^

install: FORCE
	install -d -m 0775 $(STAGING_DIR)/usr/lib
ifeq ($(CONFIG_MINIBOOT_SELFCOMPRESSED),y)
	install -m 0664 $(target-y).z.out $(IMAGES_DIR)/$(target-y).out
	install -m 0664 $(target-y).z.out.map $(IMAGES_DIR)/$(target-y).out.map
	install -m 0664 $(target-y).z.bin $(IMAGES_DIR)/$(target-y).bin
	install -m 0664 $(target-y).out $(IMAGES_DIR)/$(target-y).uncompress.out
	install -m 0664 $(target-y).out.map $(IMAGES_DIR)/$(target-y).uncompress.out.map
	install -m 0664 $(target-y).bin $(IMAGES_DIR)/$(target-y).uncompress.bin
else
	install -m 0664 $(target-y).out $(IMAGES_DIR)/
	install -m 0664 $(target-y).out.map $(IMAGES_DIR)/
	install -m 0664 $(target-y).bin $(IMAGES_DIR)/
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
	-rm -f $(DTS).dtb

$(DTS).dtb: $(DTS_SOURCE)
	if test -z "$(DTS_SOURCE)"; then \
		echo "No device tree defined in CONFIG_MINIBOOT_CUSTOM_DTS_PATH, Please check make menuconfig"; \
		exit 1; \
	fi
	cp -vf $(DTS_SOURCE) $(@D)
	$(Q)cat $(@D)/$(DTS).dts > $(@D)/.$(DTS).dtb.pre.tmp;
	$(Q)$(CC) -I$(srctree)/include -E -Wp,-MD,$(@D)/.$(DTS).dtb.d.pre.tmp \
		-nostdinc $(cflags-y) $(ccflags-y) $(cflags) \
		-D__ASSEMBLY__ -undef -D__DTS__ -x assembler-with-cpp \
		-o $(@D)/.$(DTS).dtb.dts.tmp \
		$(@D)/.$(DTS).dtb.pre.tmp
	$(Q)$(TOPDIR)/build/tools/dtc -O dtb -o $(@D)/$(DTS).dtb -b 0 \
		-Wno-unit_address_vs_reg -Wno-unit_address_format \
		-Wno-avoid_unnecessary_addr_size -Wno-alias_paths \
		-Wno-graph_child_address -Wno-graph_port -Wno-unique_unit_address \
		-Wno-simple_bus_reg -Wno-pci_device_reg -Wno-pci_bridge \
		-Wno-pci_device_bus_num -Wno-unit_address_vs_reg \
		-Wno-unit_address_format -Wno-avoid_unnecessary_addr_size \
		-Wno-alias_paths -Wno-graph_child_address -Wno-graph_port \
		-Wno-unique_unit_address -Wno-simple_bus_reg -Wno-pci_device_reg \
		-Wno-pci_bridge -Wno-pci_device_bus_num \
		-d $(@D)/.$(DTS).dtb.d.dtc.tmp \
		$(@D)/.$(DTS).dtb.dts.tmp || \
		(echo "Check $(@D)/.$(DTS).dtb.pre.tmp for errors" && false)

.PHONY: FORCE

FORCE:

endif
