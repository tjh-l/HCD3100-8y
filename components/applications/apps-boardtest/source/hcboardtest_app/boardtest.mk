HCBOARDTEST_DIR_NAME ?= hcboardtest_app
HCBOARDTEST_SRC += $(wildcard $(CUR_APP_DIR)/$(HCBOARDTEST_DIR_NAME)/*.c)
HCBOARDTEST_SRC += $(shell find -L $(CUR_APP_DIR)/$(HCBOARDTEST_DIR_NAME)/channel -name "*.c")
HCBOARDTEST_SRC += $(wildcard $(CUR_APP_DIR)/$(HCBOARDTEST_DIR_NAME)/public/*.c)
HCBOARDTEST_SRC += $(wildcard $(CUR_APP_DIR)/$(HCBOARDTEST_DIR_NAME)/boardtest_test/*.c)
