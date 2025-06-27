################################################################################
#
# bluetooth
#
################################################################################

BLUETOOTH_VERSION =
BLUETOOTH_SITE_METHOD = UNDEFINED
BLUETOOTH_SITE = UNDEFINED
BLUETOOTH_LICENSE = LGPL-2.0+
BLUETOOTH_LICENSE_FILES = COPYING
BLUETOOTH_INSTALL_STAGING = YES
BLUETOOTH_DEPENDENCIES += kernel pthread

BLUETOOTH_BUILD_CMDS = rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/bluetooth/source/ $(@D)
BLUETOOTH_EXTRACT_CMDS = rsync -au --chmod=u=rwX,go=rX  --exclude .svn --exclude .git --exclude .hg --exclude .bzr --exclude CVS $(TOPDIR)/components/bluetooth/source/ $(@D)

BLUETOOTH_CONF_OPTS += -DHCRTOS=ON

ifeq ($(BR2_PACKAGE_AD6956F),y)
BLUETOOTH_CONF_OPTS += -DBLUETOOTH_AD6956F=ON
endif

ifeq ($(BR2_PACKAGE_AC6955F_RT),y)
BLUETOOTH_CONF_OPTS += -DBLUETOOTH_AC6955F_RT=ON
endif

ifeq ($(BR2_PACKAGE_AC6955F_GX),y)
BLUETOOTH_CONF_OPTS += -DBLUETOOTH_AC6955F_GX=ON
endif

ifeq ($(BR2_PACKAGE_AC6956C_GX),y)
BLUETOOTH_CONF_OPTS += -DBLUETOOTH_AC6956C_GX=ON
endif

define BLUETOOTH_INSTALL_HEADERS
	$(INSTALL) -d $(STAGING_DIR)/usr/include
	$(INSTALL) $(@D)/inc/* -m 0666 $(STAGING_DIR)/usr/include
endef

BLUETOOTH_POST_INSTALL_TARGET_HOOKS += BLUETOOTH_INSTALL_HEADERS

$(eval $(cmake-package))
