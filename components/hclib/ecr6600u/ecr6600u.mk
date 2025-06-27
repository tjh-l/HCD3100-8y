################################################################################
#
# ecr6600u
#
################################################################################
ECR6600U_VERSION = 0702e8adb93f462f3feb9c695984922cfc327813
ECR6600U_SITE_METHOD = git
ECR6600U_SITE = ssh://git@hichiptech.gitlab.com:33888/hclib/ecr6600u.git
ECR6600U_DEPENDENCIES = kernel

ECR6600U_MAKE_FLAGS += \
		CROSS_COMPILE=$(TARGET_CROSS) \
		CFLAGS="$(TARGET_CFLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS)" \
		LDFLAGS="${TARGET_LDFLAGS}"

ECR6600U_INSTALL_STAGING = YES

ECR6600U_EXTRACT_CMDS = \
	git -C $(@D) init && \
	git -C $(@D) remote add origin $(ECR6600U_SITE) && \
	git -C $(@D) fetch && \
	git -C $(@D) checkout master && \
	git -C $(@D) checkout $(ECR6600U_VERSION)

ECR6600U_CLEAN_CMDS = $(TARGET_MAKE_ENV) $(ECR6600U_MAKE_ENV) $(MAKE) $(ECR6600U_MAKE_FLAGS) -C $(@D) clean
ECR6600U_BUILD_CMDS = $(TARGET_MAKE_ENV) $(ECR6600U_MAKE_ENV) $(MAKE) $(ECR6600U_MAKE_FLAGS) -C $(@D) all
ECR6600U_INSTALL_STAGING_CMDS = $(TARGET_MAKE_ENV) $(ECR6600U_MAKE_ENV) $(MAKE) $(ECR6600U_MAKE_FLAGS) -C $(@D) install

$(eval $(generic-package))
