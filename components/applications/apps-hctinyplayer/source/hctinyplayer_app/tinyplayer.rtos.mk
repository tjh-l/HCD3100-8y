HCTINYPLAYER_DIR_NAME ?= hctinyplayer_app

CSRCS += $(wildcard $(TINYPLAYER_DIR)/$(HCTINYPLAYER_DIR_NAME)/*.c)
CSRCS += $(shell find -L $(TINYPLAYER_DIR)/$(HCTINYPLAYER_DIR_NAME)/channel -name "*.c")
CSRCS += $(wildcard $(TINYPLAYER_DIR)/$(HCTINYPLAYER_DIR_NAME)/public/*.c)
CSRCS += $(wildcard $(TINYPLAYER_DIR)/$(HCTINYPLAYER_DIR_NAME)/volume/*.c)

CFLAGS += -I$(TINYPLAYER_DIR)
