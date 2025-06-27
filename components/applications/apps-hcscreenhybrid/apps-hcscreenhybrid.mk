APPS_HCSCREENHYBRID_SUPPORT_SEPARATE_OUTPUT = YES
APPS_HCSCREENHYBRID_DEPENDENCIES += $(apps_deps-y)
APPS_HCSCREENHYBRID_ALWAYS_BUILD = yes

APPS_HCSCREENHYBRID_DTS_NAME = $(basename $(filter %.dts,$(notdir $(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)))))

APPS_HCSCREENHYBRID_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(APPS_HCSCREENHYBRID_MAKE_ENV) $(MAKE) $(APPS_HCSCREENHYBRID_MAKE_FLAGS) -C $(@D) clean
APPS_HCSCREENHYBRID_BUILD_CMDS = rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/applications/apps-hcscreenhybrid/source/ $(@D) && \
		     if [ -f $(@D)/Makefile ];then \
		     mv $(@D)/Makefile $(@D)/Makefile.old; fi && \
		     cp $(@D)/Makefile.rtos $(@D)/Makefile;\
		     cp $(@D)/main.rtos.c $(@D)/main.c;\
			 cp $(@D)/hcscreenhybrid_app/HCCAST_1920x1080.264 ${IMAGES_DIR}/fs-partition1-root/HCCAST_1920x1080.264;\
			 cp $(@D)/hcscreenhybrid_app/music_bg_logo_19KB.264 ${IMAGES_DIR}/fs-partition1-root/music_bg_logo.264;\
			 mkdir -p $(@D)/src;\
			 ln -sf $(@D)/Makefile.src.rtos $(@D)/src/Makefile;\
			 ln -sf $(@D)/hcscreenhybrid_app $(@D)/src/;\
		     $(TARGET_MAKE_ENV) $(APPS_HCSCREENHYBRID_MAKE_ENV) $(MAKE) $(APPS_HCSCREENHYBRID_MAKE_FLAGS) -C $(@D) all

APPS_HCSCREENHYBRID_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(APPS_HCSCREENHYBRID_MAKE_ENV) $(MAKE) $(APPS_HCSCREENHYBRID_MAKE_FLAGS) -C $(@D) install

ifneq ($(CONFIG_CUSTOM_DTS_PATH),)
define APPS_HCSCREENHYBRID_ENTRY_ADDR_FROM_DTS
	-rm -rf $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr
	mkdir -p $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr
	$(foreach dts,$(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)),
		cp -f $(dts) $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/
	)

	echo "" >> $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREENHYBRID_DTS_NAME)).dts
	echo "unsigned int avp_load_addr = (HCRTOS_SYSMEM_OFFSET + 0x1000);" >> $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREENHYBRID_DTS_NAME)).dts
	gcc -O2 -I$(KERNEL_PKGD)/include -E -Wp,-MMD,$(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREENHYBRID_DTS_NAME)).dtb.d.pre.tmp -nostdinc -undef -D__DTS__ -x assembler-with-cpp -o $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREENHYBRID_DTS_NAME)).dtb.dts.tmp $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREENHYBRID_DTS_NAME)).dts
	cat $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/$(strip $(APPS_HCSCREENHYBRID_DTS_NAME)).dtb.dts.tmp | grep avp_load_addr > $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/main.c
	echo -e " \
#include <stdio.h>\n \
int main(int argc, char **argv) \
{ \
	printf(\"0x%08x\", avp_load_addr | 0x80000000); \
	return 0; \
}" >> $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/main.c
	gcc -o $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/a.out $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/main.c
	$(TOPDIR)/build/scripts/update_entry_addr.sh \
		--genaddrout $(APPS_HCSCREENHYBRID_DIR)/.tmp-for-load-addr/a.out \
		--entryld $(APPS_HCSCREENHYBRID_PKGD)/entry.ld

endef
else ifeq ($(BR2_PACKAGE_APPS_HCSCREENHYBRID),y)
$(info "CONFIG_CUSTOM_DTS_PATH is not defined, not able to update entry address of avp application")
endif

APPS_HCSCREENHYBRID_PRE_BUILD_HOOKS += APPS_HCSCREENHYBRID_ENTRY_ADDR_FROM_DTS

$(eval $(generic-package))

