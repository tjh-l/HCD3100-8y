################################################################################
#
# freetype
#
################################################################################

FREETYPE_VERSION = 2.13.0
FREETYPE_SOURCE = freetype-$(FREETYPE_VERSION).tar.xz
FREETYPE_SITE = http://download.savannah.gnu.org/releases/freetype
FREETYPE_INSTALL_STAGING = YES
FREETYPE_MAKE_OPTS = CCexe="$(HOSTCC)"
FREETYPE_LICENSE = Dual FTL/GPL-2.0+
FREETYPE_LICENSE_FILES = docs/LICENSE.TXT docs/FTL.TXT docs/GPLv2.TXT
FREETYPE_CPE_ID_VENDOR = freetype
#FREETYPE_CONFIG_SCRIPTS = freetype-config

# harfbuzz already depends on freetype so disable harfbuzz in freetype to avoid
# a circular dependency
FREETYPE_CONF_OPTS = --without-harfbuzz

HOST_FREETYPE_DEPENDENCIES = host-pkgconf
HOST_FREETYPE_CONF_OPTS = \
	--without-brotli \
	--without-bzip2 \
	--without-harfbuzz \
	--without-png \
	--without-zlib

# since 2.9.1 needed for freetype-config install
FREETYPE_CONF_OPTS += --enable-freetype-config
HOST_FREETYPE_CONF_OPTS += --enable-freetype-config

ifeq ($(BR2_PACKAGE_ZLIB),y)
FREETYPE_DEPENDENCIES += zlib
FREETYPE_CONF_OPTS += --with-zlib
FREETYPE_CONF_ENV += ZLIB_LIBS="-lz"
FREETYPE_CONF_ENV += ZLIB_CFLAGS="-I$(STAGING_DIR)/usr/include"
else
FREETYPE_CONF_OPTS += --without-zlib
endif

ifeq ($(BR2_PACKAGE_BROTLI),y)
FREETYPE_DEPENDENCIES += brotli
FREETYPE_CONF_OPTS += --with-brotli
else
FREETYPE_CONF_OPTS += --without-brotli
endif

ifeq ($(BR2_PACKAGE_BZIP2),y)
FREETYPE_DEPENDENCIES += bzip2
FREETYPE_CONF_OPTS += --with-bzip2
else
FREETYPE_CONF_OPTS += --without-bzip2
endif

ifeq ($(BR2_PACKAGE_LIBPNG),y)
FREETYPE_DEPENDENCIES += libpng
FREETYPE_CONF_OPTS += --with-png
else
FREETYPE_CONF_OPTS += --without-png
endif

$(eval $(autotools-package))
