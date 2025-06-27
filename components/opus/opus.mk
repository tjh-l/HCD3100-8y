################################################################################
#
# opus
#
################################################################################

OPUS_VERSION = 1.5
OPUS_SOURCE = opus-$(OPUS_VERSION).tar.gz
OPUS_SITE = https://downloads.xiph.org/releases/opus
OPUS_LICENSE = BSD-3-Clause
OPUS_LICENSE_FILES = COPYING
OPUS_INSTALL_STAGING = YES

OPUS_CFLAGS = $(TARGET_CFLAGS)

OPUS_DEPENDENCIES += kernel

OPUS_CONF_ENV = CFLAGS="$(OPUS_CFLAGS)"

OPUS_INSTALL_STAGING = YES
OPUS_CONF_OPTS += --enable-fixed-point
OPUS_CONF_OPTS += --disable-extra-programs

$(eval $(autotools-package))
