APPS_PROJECTOR_HUDI_VERSION = 4292148fcde64a6114e67b05a71a898c81cabf10
APPS_PROJECTOR_HUDI_SITE_METHOD = git
APPS_PROJECTOR_HUDI_SITE = ssh://git@hichiptech.gitlab.com:33888/hcrtos_sdk/apps-projector-hudi.git
APPS_PROJECTOR_HUDI_DEPENDENCIES += $(apps_deps-y)

APPS_PROJECTOR_HUDI_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(APPS_PROJECTOR_HUDI_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(APPS_PROJECTOR_HUDI_VERSION)

APPS_PROJECTOR_HUDI_MAKE_FLAGS ?= \
	CROSS_COMPILE=$(TARGET_CROSS) CXXFLAGS="$(TARGET_CXXFLAGS)" CFLAGS="$(TARGET_CFLAGS)" LDFLAGS="$(TARGET_LDFLAGS)"

APPS_PROJECTOR_HUDI_DTS_NAME = $(basename $(filter %.dts,$(notdir $(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)))))

APPS_PROJECTOR_HUDI_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(APPS_PROJECTOR_HUDI_MAKE_ENV) $(MAKE) $(APPS_PROJECTOR_HUDI_MAKE_FLAGS) -C $(@D) clean

ifeq ($(BR2_PACKAGE_HCCAST_WIRELESS),y)
APPS_PROJECTOR_HUDI_BUILD_CMDS = \
		     if [ -f $(@D)/Makefile ];then \
		     mv $(@D)/Makefile $(@D)/Makefile.linux; fi && \
		     cp $(@D)/Makefile.rtos $(@D)/Makefile;\
		     cp $(@D)/main.rtos.c $(@D)/main.c;\
			 cp $(@D)/hcprojector_app/HCCAST_1920x1080.264 ${IMAGES_DIR}/fs-partition1-root/HCCAST_1920x1080.264;\
			 cp $(@D)/hcprojector_app/music_bg_logo_41KB.264 ${IMAGES_DIR}/fs-partition1-root/music_bg_logo.264;\
			 mkdir -p $(@D)/src;\
			 ln -sf $(@D)/Makefile.src.rtos $(@D)/src/Makefile;\
			 ln -sf $(@D)/hcprojector_app $(@D)/src/;\
		     $(TARGET_MAKE_ENV) $(APPS_PROJECTOR_HUDI_MAKE_ENV) $(MAKE) $(APPS_PROJECTOR_HUDI_MAKE_FLAGS) -C $(@D) all
else			 
APPS_PROJECTOR_HUDI_BUILD_CMDS = \
		     if [ -f $(@D)/Makefile ];then \
		     mv $(@D)/Makefile $(@D)/Makefile.old; fi && \
		     cp $(@D)/Makefile.rtos $(@D)/Makefile;\
		     cp $(@D)/main.rtos.c $(@D)/main.c;\
			 mkdir -p $(@D)/src;\
			 ln -sf $(@D)/Makefile.src.rtos $(@D)/src/Makefile;\
			 ln -sf $(@D)/hcprojector_app $(@D)/src/;\
		     $(TARGET_MAKE_ENV) $(APPS_PROJECTOR_HUDI_MAKE_ENV) $(MAKE) $(APPS_PROJECTOR_HUDI_MAKE_FLAGS) -C $(@D) all
endif

APPS_PROJECTOR_HUDI_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(APPS_PROJECTOR_HUDI_MAKE_ENV) $(MAKE) $(APPS_PROJECTOR_HUDI_MAKE_FLAGS) -C $(@D) install


ifneq ($(CONFIG_CUSTOM_DTS_PATH),)
define APPS_PROJECTOR_HUDI_ENTRY_ADDR_FROM_DTS
	-rm -rf $(@D)/.tmp-for-load-addr
	mkdir -p $(@D)/.tmp-for-load-addr
	$(foreach dts,$(call qstrip,$(CONFIG_CUSTOM_DTS_PATH)),
		cp -f $(dts) $(@D)/.tmp-for-load-addr/
	)
	echo "" >> $(@D)/.tmp-for-load-addr/$(strip $(APPS_PROJECTOR_HUDI_DTS_NAME)).dts
	echo "unsigned int avp_load_addr = (HCRTOS_SYSMEM_OFFSET + 0x1000);" >> $(@D)/.tmp-for-load-addr/$(strip $(APPS_PROJECTOR_HUDI_DTS_NAME)).dts
	gcc -O2 -I$(KERNEL_PKGD)/include -E -Wp,-MMD,$(@D)/.tmp-for-load-addr/$(strip $(APPS_PROJECTOR_HUDI_DTS_NAME)).dtb.d.pre.tmp -nostdinc -undef -D__DTS__ -x assembler-with-cpp -o $(@D)/.tmp-for-load-addr/$(strip $(APPS_PROJECTOR_HUDI_DTS_NAME)).dtb.dts.tmp $(@D)/.tmp-for-load-addr/$(strip $(APPS_PROJECTOR_HUDI_DTS_NAME)).dts
	cat $(@D)/.tmp-for-load-addr/$(strip $(APPS_PROJECTOR_HUDI_DTS_NAME)).dtb.dts.tmp | grep avp_load_addr > $(@D)/.tmp-for-load-addr/main.c
	echo -e " \
#include <stdio.h>\n \
int main(int argc, char **argv) \
{ \
	printf(\"0x%08x\", avp_load_addr | 0x80000000); \
	return 0; \
}" >> $(@D)/.tmp-for-load-addr/main.c
	gcc -o $(@D)/.tmp-for-load-addr/a.out $(@D)/.tmp-for-load-addr/main.c
	$(TOPDIR)/build/scripts/update_entry_addr.sh \
		--genaddrout $(@D)/.tmp-for-load-addr/a.out \
		--entryld $(@D)/entry.ld
endef
else ifeq ($(BR2_PACKAGE_APPS_PROJECTOR_HUDI),y)
$(info "CONFIG_CUSTOM_DTS_PATH is not defined, not able to update entry address of avp application")
endif

APPS_PROJECTOR_HUDI_PRE_BUILD_HOOKS += APPS_PROJECTOR_HUDI_ENTRY_ADDR_FROM_DTS

$(eval $(generic-package))
