input-y := input.o input-mt.o
input-$(CONFIG_INPUT_JOYDEV) += joydev.o

cflags-y += -Wno-sign-compare

obj-y += input.o

obj-$(CONFIG_RC_CORE) += rc/
obj-$(CONFIG_TP) += tp/
obj-$(CONFIG_ROTARY_KEY) += rotary_key/
obj-$(CONFIG_MISC) += misc/
obj-$(CONFIG_INPUT_JOYSTICK) += joystick/

obj-y += ../saradc/
