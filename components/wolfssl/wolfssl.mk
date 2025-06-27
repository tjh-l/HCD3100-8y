################################################################################
#
# wolfssl
#
################################################################################

WOLFSSL_VERSION = 5.6.2-stable
WOLFSSL_SOURCE = v$(WOLFSSL_VERSION).tar.gz
WOLFSSL_SITE = https://github.com/wolfSSL/wolfssl/archive/refs/tags
WOLFSSL_INSTALL_STAGING = YES

WOLFSSL_LICENSE = GPL-2.0+
WOLFSSL_LICENSE_FILES = COPYING LICENSING
WOLFSSL_CPE_ID_VENDOR = wolfssl

WOLFSSL_DEPENDENCIES += kernel newlib pthread

# wolfssl's source code is released without a configure
# script, so we need autoreconf
WOLFSSL_AUTORECONF = YES

WOLFSSL_CONF_OPTS = --disable-examples --disable-crypttests
WOLFSSL_CONF_OPTS += --disable-shared --enable-static
WOLFSSL_CONF_OPTS += --enable-fastmath
WOLFSSL_CONF_OPTS += --disable-singlethreaded
WOLFSSL_CONF_OPTS += --enable-ffmpeg
WOLFSSL_CONF_OPTS += --enable-opensslall
WOLFSSL_CONF_OPTS += --enable-opensslextra
WOLFSSL_CONF_OPTS += --enable-aescbc
WOLFSSL_CONF_OPTS += --enable-oldtls
WOLFSSL_CONF_OPTS += --enable-tlsv10
WOLFSSL_CONF_OPTS += --enable-ocsp
WOLFSSL_CONF_OPTS += --enable-ocspstapling
WOLFSSL_CONF_OPTS += --enable-ocspstapling2
WOLFSSL_CONF_OPTS += --enable-certgen
WOLFSSL_CONF_OPTS += --enable-certreq
WOLFSSL_CONF_OPTS += --enable-keygen
WOLFSSL_CONF_OPTS += --enable-sha224
WOLFSSL_CONF_OPTS += --disable-errorqueue
WOLFSSL_CONF_OPTS += --disable-error-queue-per-thread
WOLFSSL_CONF_OPTS += --enable-sni
WOLFSSL_CONF_OPTS += --enable-alpn
WOLFSSL_CONF_OPTS += --disable-chacha
WOLFSSL_CONF_OPTS += --disable-poly1305
WOLFSSL_CONF_OPTS += --disable-tls13
WOLFSSL_CONF_OPTS += --enable-des3
WOLFSSL_CONF_OPTS += --enable-curl
WOLFSSL_CONF_OPTS += --enable-bind
WOLFSSL_CONF_OPTS += --enable-smallstack
WOLFSSL_CONF_OPTS += --enable-aesgcm-stream

WOLFSSL_CONF_ENV += CPPFLAGS="$(TARGET_CPPFLAGS) -DWOLFSSL_HCOS -DHC_RTOS"
WOLFSSL_CONF_ENV += CFLAGS="$(TARGET_CFLAGS) -DWOLFSSL_HCOS -DHC_RTOS"

define WOLFSSL_CONFIGURE_CMDS
	(cd $(@D) && rm -rf config.cache && \
	$(TARGET_CONFIGURE_OPTS) \
	$(TARGET_CONFIGURE_ARGS) \
	$(WOLFSSL_CONF_ENV) \
	./configure \
		--target=mips-mti-elf \
		--host=mips-mti-elf \
		--build=x86_64 \
		--prefix=/usr \
		$(SHARED_STATIC_LIBS_OPTS) \
		$(WOLFSSL_CONF_OPTS) \
	)
endef

define WOLFSSL_CREATE_OPENSSL_LINK
	if test -d $(STAGING_DIR)/usr/include/openssl; then \
		echo "openssl exist"; \
	else \
		ln -s $(STAGING_DIR)/usr/include/wolfssl/openssl $(STAGING_DIR)/usr/include/; \
	fi;
endef

define WOLFSSL_SED_SETTINGS_HEADER
	sed -i 's/#ifdef EXTERNAL_OPTS_OPENVPN/#if 1/g' $(STAGING_DIR)/usr/include/wolfssl/wolfcrypt/settings.h
endef

WOLFSSL_POST_INSTALL_TARGET_HOOKS += WOLFSSL_CREATE_OPENSSL_LINK WOLFSSL_SED_SETTINGS_HEADER

$(eval $(autotools-package))
