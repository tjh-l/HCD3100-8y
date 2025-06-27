HCBOARDTEST_DIR_NAME ?= hcboardtest_app

CSRCS += $(wildcard $(BOARDTEST_DIR)/$(HCBOARDTEST_DIR_NAME)/*.c)
CSRCS += $(shell find -L $(BOARDTEST_DIR)/$(HCBOARDTEST_DIR_NAME)/channel -name "*.c")
CSRCS += $(wildcard $(BOARDTEST_DIR)/$(HCBOARDTEST_DIR_NAME)/public/*.c)
CSRCS += $(wildcard $(BOARDTEST_DIR)/$(HCBOARDTEST_DIR_NAME)/boardtest_test/*.c)
CSRCS += $(wildcard $(BOARDTEST_DIR)/$(HCBOARDTEST_DIR_NAME)/boardtest_test/*/*.c)
