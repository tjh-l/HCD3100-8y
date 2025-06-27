cflags-y += -I$(src)/../linux-porting/include -Wno-sign-compare

__usb_cmds-y = usb_cmds_main.o

### usb host
__usb_cmds-$(CONFIG_CMDS_USB_HID) += usb_cmd_hid.o
__usb_cmds-$(CONFIG_CMDS_USB_4G_RNDIS_NET_DEVICE) += usb_serial_demo.o

### usb gadget
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_MSC) += usb_gadget_cmd_msc.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_NCM) += usb_gadget_cmd_ncm.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_SERIAL) += usb_gadget_cmd_serial.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_ZERO) += usb_gadget_cmd_zero.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_HID) += usb_gadget_cmd_kbd_demo.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_HID) += usb_gadget_cmd_mouse_demo.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_HID) += usb_gadget_cmd_hid_test.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_HID) += usb_gadget_cmd_hid.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_UAC1) += usb_gadget_cmd_uac1.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_UVC) += usb_gadget_cmd_uvc.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_WINUSB) += usb_gadget_cmd_winusb.o

__usb_cmds-$(CONFIG_CMDS_USB_GADGET_LOCK) += usb_gadget_cmd_doorlock_av.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_LOCK) += usb_gadget_cmd_doorlock_performance.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_LOCK) += usb_gadget_cmd_doorlock.o

__usb_cmds-$(CONFIG_CMDS_USB_GADGET_HID_TP) += usb_gadget_cmd_touch_panel.o
__usb_cmds-$(CONFIG_CMDS_USB_GADGET_HID_TP) += usb_gadget_cmd_touch_mouse.o

__usb_cmds-$(CONFIG_CMDS_FILE_TEST) += file_chmod_test.o

obj-y += __usb_cmds.o

### libusb
obj-$(CONFIG_CMDS_LIBUSB_EXAMPLES) += libusb_examples/

### libuvc
obj-$(CONFIG_CMDS_LIBUVC_EXAMPLES) += libuvc_examples/

### libuac
obj-$(CONFIG_CMDS_LIBUAC_EXAMPLES) += libuac_examples/
