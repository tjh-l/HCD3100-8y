cflags-y	+= -I$(src)
cflags-y	+= -I$(src)/../
cflags-y	+= -I$(src)/../function/

g_zero-y			:= zero.o ## g_zero for usb gadget demo
g_hid-y 			:= hid.o
g_winusb-y 			:= winusb.o

obj-$(CONFIG_USB_ZERO)		+= g_zero.o
obj-$(CONFIG_USB_GADGET_WINUSB_VENDOR) += g_winusb.o
