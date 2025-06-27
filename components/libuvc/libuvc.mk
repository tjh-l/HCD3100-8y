LIBUVC_SUPPORT_SEPARATE_OUTPUT = YES

LIBUVC_DEPENDENCIES += kernel pthread libusb

$(eval $(generic-package))
