APPS_MINIBOOT_SUPPORT_SEPARATE_OUTPUT = YES
APPS_MINIBOOT_DEPENDENCIES += $(apps_deps-y)
APPS_MINIBOOT_ALWAYS_BUILD = yes

APPS_MINIBOOT_DTS_NAME = $(basename $(filter %.dts,$(notdir $(call qstrip,$(CONFIG_MINIBOOT_CUSTOM_DTS_PATH)))))

ifneq ($(CONFIG_MINIBOOT_CUSTOM_DTS_PATH),)
define APPS_MINIBOOT_ENTRY_ADDR_FROM_DTS
	-rm -rf $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr
	mkdir -p $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr
	$(foreach dts,$(call qstrip,$(CONFIG_MINIBOOT_CUSTOM_DTS_PATH)),
		cp -f $(dts) $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/
	)
	echo "" >> $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/$(strip $(APPS_MINIBOOT_DTS_NAME)).dts
	echo "unsigned int bootloader_load_addr = (HCRTOS_BOOTMEM_OFFSET);" >> $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/$(strip $(APPS_MINIBOOT_DTS_NAME)).dts
	gcc -O2 -I$(KERNEL_PKGD)/include -E -Wp,-MMD,$(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/$(strip $(APPS_MINIBOOT_DTS_NAME)).dtb.d.pre.tmp -nostdinc -undef -D__DTS__ -x assembler-with-cpp -o $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/$(strip $(APPS_MINIBOOT_DTS_NAME)).dtb.dts.tmp $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/$(strip $(APPS_MINIBOOT_DTS_NAME)).dts
	cat $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/$(strip $(APPS_MINIBOOT_DTS_NAME)).dtb.dts.tmp | grep bootloader_load_addr > $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/main.c
	echo -e " \
#include <stdio.h>\n \
int main(int argc, char **argv) \
{ \
	printf(\"0x%08x\", bootloader_load_addr | 0x80000000); \
	return 0; \
}" >> $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/main.c
	gcc -o $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/a.out $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/main.c
	$(TOPDIR)/build/scripts/update_entry_addr.sh \
		--genaddrout $(APPS_MINIBOOT_DIR)/.tmp-for-load-addr/a.out \
		--entryld $(APPS_MINIBOOT_PKGD)/entry.ld
endef
else ifeq ($(BR2_PACKAGE_APPS_MINIBOOT),y)
$(info "CONFIG_CUSTOM_DTS_PATH is not defined, not able to update entry address of bootloader application")
endif

APPS_MINIBOOT_PRE_BUILD_HOOKS += APPS_MINIBOOT_ENTRY_ADDR_FROM_DTS

$(eval $(generic-package))
