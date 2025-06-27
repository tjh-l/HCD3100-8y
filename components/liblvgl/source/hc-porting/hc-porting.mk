DEPPATH += --dep-path $(LVGL_DIR)/hc-porting
VPATH += :$(LVGL_DIR)/hc-porting

CFLAGS += "-I$(LVGL_DIR)/hc-porting"

CSRCS += $(shell find -L $(LVGL_DIR)/hc-porting -name "*.c")
