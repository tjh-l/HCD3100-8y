define gui_guider_template
UI_INC :=-I$$(LVGL_DIR)/hc_examples/$(1)/custom \
	-I$$(LVGL_DIR)/hc_examples/$(1)/generated

DEPPATH += --dep-path $$(LVGL_DIR)/hc_examples/$(1)/generated \
	   --dep-path $$(LVGL_DIR)/hc_examples/$(1)/custom

VPATH += :$$(LVGL_DIR)/hc_examples/$(1)/generated
VPATH += :$$(LVGL_DIR)/hc_examples/$(1)/custom

CSRCS += $(shell find -L $(LVGL_DIR)/hc_examples/$(1) -name "*.c")
CFLAGS += $$(UI_INC)
endef #gui_guider_template

ifeq ($(CONFIG_LV_HC_UISLIDE_1024x600),y)
$(eval $(call gui_guider_template,UISlide_1024x600))
endif

ifeq ($(CONFIG_LV_HC_UISLIDE_1280x720),y)
$(eval $(call gui_guider_template,UISlide_1280x720))
endif

#ifneq ($(UI_NAME),y)
#UI_INC :=-I$(LVGL_DIR)/hc_examples/$/custom \
	#-I$(LVGL_DIR)/hc_examples/UISlide_1024x600/generated

#DEPPATH += --dep-path $(LVGL_DIR)/hc_examples/UISlide_1024x600/generated \
	   #--dep-path $(LVGL_DIR)/hc_examples/UISlide_1024x600/custom

#VPATH += :$(LVGL_DIR)/hc_examples/UISlide_1024x600/generated
#VPATH += :$(LVGL_DIR)/hc_examples/UISlide_1024x600/custom

#CSRCS += $(shell find -L $(LVGL_DIR)/hc_examples/UISlide_1024x600 -name "*.c")
#CFLAGS += $(UI_INC)
#endif

#ifeq ($(CONFIG_LV_HC_UISLIDE_1280x720),y)
#UI_INC = -I$(LVGL_DIR)/hc_examples/UISlide_1280x720/custom \
	   #-I$(LVGL_DIR)/hc_examples/UISlide_1280x720/generated

#DEPPATH += --dep-path $(LVGL_DIR)/hc_examples/UISlide_1024x600
#VPATH += :$(LVGL_DIR)/hc_examples/UISlide_1024x600
#CFLAGS += "$(UI_INC)"

#CSRCS += $(shell find -L $(LVGL_DIR)/hc_examples/UISlide_1280x720 -name "*.c")
#endif
