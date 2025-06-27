################################################################################
#
# hudi
#
################################################################################

HUDI_VERSION =
HUDI_SITE_METHOD = UNDEFINED
HUDI_SITE = UNDEFINED
HUDI_LICENSE = LGPL-2.0+
HUDI_LICENSE_FILES = COPYING
HUDI_INSTALL_STAGING = YES

HUDI_BUILD_CMDS = rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/hudi/source/ $(@D)
HUDI_EXTRACT_CMDS = rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/hudi/source/ $(@D)

HUDI_DEPENDENCIES += kernel pthread
HUDI_CONF_OPTS += -DHCRTOS=ON

ifeq ($(BR2_PACKAGE_HUDI_FLASH),y)
HUDI_CONF_OPTS += -DFLASH_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_VIDEO),y)
HUDI_CONF_OPTS += -DVIDEO_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_AUDIO),y)
HUDI_CONF_OPTS += -DAUDIO_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_DISPLAY),y)
HUDI_CONF_OPTS += -DDISPLAY_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_AVSYNC),y)
HUDI_CONF_OPTS += -DAVSYNC_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_CEC),y)
HUDI_CONF_OPTS += -DCEC_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_DSC),y)
HUDI_CONF_OPTS += -DDSC_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_PERSISTENTMEM),y)
HUDI_CONF_OPTS += -DPERSISTENTMEM_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_BLUETOOTH),y)
HUDI_CONF_OPTS += -DBLUETOOTH_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_HDMI),y)
HUDI_CONF_OPTS += -DHDMI_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_POWER),y)
HUDI_CONF_OPTS += -DPOWER_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_CVBS_RX),y)
HUDI_CONF_OPTS += -DCVBSRX_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_SCREEN),y)
HUDI_CONF_OPTS += -DSCREEN_API=ON
endif

ifeq ($(BR2_PACKAGE_HUDI_HDMIRX),y)
HUDI_CONF_OPTS += -DHDMIRX_API=ON
endif

define HUDI_INSTALL_HEADERS
	$(INSTALL) -d $(STAGING_DIR)/usr/include/hudi
	$(INSTALL) $(@D)/include/* -m 0666 $(STAGING_DIR)/usr/include/hudi
endef

HUDI_POST_INSTALL_TARGET_HOOKS += HUDI_INSTALL_HEADERS

$(eval $(cmake-package))
