APPS_BOARDTEST_SUPPORT_SEPARATE_OUTPUT = YES
APPS_BOARDTEST_DEPENDENCIES += $(apps_deps-y)
APPS_BOARDTEST_ALWAYS_BUILD = yes

APPS_BOARDTEST_DTS_NAME = $(basename $(filter %.dts,$(notdir $(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)))))

APPS_BOARDTEST_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(APPS_BOARDTEST_MAKE_ENV) $(MAKE) $(APPS_BOARDTEST_MAKE_FLAGS) -C $(@D) clean

APPS_BOARDTEST_BUILD_CMDS = rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/applications/apps-boardtest/source/ $(@D) && \
		     if [ -f $(@D)/Makefile ];then \
		     mv $(@D)/Makefile $(@D)/Makefile.linux; fi && \
		     cp $(@D)/Makefile.rtos $(@D)/Makefile;\
		     cp $(@D)/main.rtos.c $(@D)/main.c;\
			 mkdir -p $(@D)/src;\
			 ln -sf $(@D)/Makefile.src.rtos $(@D)/src/Makefile;\
			 ln -sf $(@D)/hcboardtest_app $(@D)/src/;\
		     $(TARGET_MAKE_ENV) $(APPS_BOARDTEST_MAKE_ENV) $(MAKE) $(APPS_BOARDTEST_MAKE_FLAGS) -C $(@D) all

APPS_BOARDTEST_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(APPS_BOARDTEST_MAKE_ENV) $(MAKE) $(APPS_BOARDTEST_MAKE_FLAGS) -C $(@D) install

ifneq ($(CONFIG_CUSTOM_DTS_PATH),)
define APPS_BOARDTEST_ENTRY_ADDR_FROM_DTS
	-rm -rf $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr
	mkdir -p $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr
	$(foreach dts,$(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)),
		cp -f $(dts) $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/
	)
	echo "" >> $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/$(strip $(APPS_BOARDTEST_DTS_NAME)).dts
	echo "unsigned int avp_load_addr = (HCRTOS_SYSMEM_OFFSET + 0x1000);" >> $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/$(strip $(APPS_BOARDTEST_DTS_NAME)).dts
	gcc -O2 -I$(KERNEL_PKGD)/include -E -Wp,-MMD,$(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/$(strip $(APPS_BOARDTEST_DTS_NAME)).dtb.d.pre.tmp -nostdinc -undef -D__DTS__ -x assembler-with-cpp -o $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/$(strip $(APPS_BOARDTEST_DTS_NAME)).dtb.dts.tmp $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/$(strip $(APPS_BOARDTEST_DTS_NAME)).dts
	cat $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/$(strip $(APPS_BOARDTEST_DTS_NAME)).dtb.dts.tmp | grep avp_load_addr > $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/main.c
	echo -e " \
#include <stdio.h>\n \
int main(int argc, char **argv) \
{ \
	printf(\"0x%08x\", avp_load_addr | 0x80000000); \
	return 0; \
}" >> $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/main.c
	gcc -o $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/a.out $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/main.c
	$(TOPDIR)/build/scripts/update_entry_addr.sh \
		--genaddrout $(APPS_BOARDTEST_DIR)/.tmp-for-load-addr/a.out \
		--entryld $(APPS_BOARDTEST_PKGD)/entry.ld
endef
else ifeq ($(BR2_PACKAGE_APPS_BOARDTEST),y)
$(info "CONFIG_CUSTOM_DTS_PATH is not defined, not able to update entry address of avp application")
endif

APPS_BOARDTEST_PRE_BUILD_HOOKS += APPS_BOARDTEST_ENTRY_ADDR_FROM_DTS

$(eval $(generic-package))
