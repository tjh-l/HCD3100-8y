APPS_HDMISWITCH_SUPPORT_SEPARATE_OUTPUT = YES
APPS_HDMISWITCH_DEPENDENCIES += $(apps_deps-y)
APPS_HDMISWITCH_ALWAYS_BUILD = yes

APPS_HDMISWITCH_DTS_NAME = $(basename $(filter %.dts,$(notdir $(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)))))

ifneq ($(CONFIG_CUSTOM_DTS_PATH),)
define APPS_HDMISWITCH_ENTRY_ADDR_FROM_DTS
	-rm -rf $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr
	mkdir -p $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr
	$(foreach dts,$(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)),
		cp -f $(dts) $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/
	)
	echo "" >> $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/$(strip $(APPS_HDMISWITCH_DTS_NAME)).dts
	echo "unsigned int hdmiswitch_load_addr = (HCRTOS_SYSMEM_OFFSET + 0x1000);" >> $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/$(strip $(APPS_HDMISWITCH_DTS_NAME)).dts
	gcc -O2 -I$(KERNEL_PKGD)/include -E -Wp,-MMD,$(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/$(strip $(APPS_HDMISWITCH_DTS_NAME)).dtb.d.pre.tmp -nostdinc -undef -D__DTS__ -x assembler-with-cpp -o $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/$(strip $(APPS_HDMISWITCH_DTS_NAME)).dtb.dts.tmp $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/$(strip $(APPS_HDMISWITCH_DTS_NAME)).dts
	cat $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/$(strip $(APPS_HDMISWITCH_DTS_NAME)).dtb.dts.tmp | grep hdmiswitch_load_addr > $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/main.c
	echo -e " \
#include <stdio.h>\n \
int main(int argc, char **argv) \
{ \
	printf(\"0x%08x\", hdmiswitch_load_addr | 0x80000000); \
	return 0; \
}" >> $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/main.c
	gcc -o $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/a.out $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/main.c
	$(TOPDIR)/build/scripts/update_entry_addr.sh \
		--genaddrout $(APPS_HDMISWITCH_DIR)/.tmp-for-load-addr/a.out \
		--entryld $(APPS_HDMISWITCH_PKGD)/entry.ld
endef
else ifeq ($(BR2_PACKAGE_APPS_HDMISWITCH),y)
$(info "CONFIG_CUSTOM_DTS_PATH is not defined, not able to update entry address of hdmiswitch application")
endif

APPS_HDMISWITCH_PRE_BUILD_HOOKS += APPS_HDMISWITCH_ENTRY_ADDR_FROM_DTS

$(eval $(generic-package))
