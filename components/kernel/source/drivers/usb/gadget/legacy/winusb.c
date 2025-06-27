#define LOG_TAG "usbd-winusb"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/err.h>


/* 
 * PID/VID/product name
 */
#define DRIVER_DESC "HiChip WCID"
#define WINUSB_DEVICE_BCD 0x0003


/* 
 * 响应ID为0xEE的字符描述符请求时, 定义对应的vendor code (bVendorCode) 
 */
uint8_t config_usb_gadget_wcid_vendor_code(void)
{
	return 0x17;
}

/* 
 * Devcie Descripotor 
 */
struct usb_device_descriptor device_desc = {
	.bLength = sizeof device_desc,
	.bDescriptorType = USB_DT_DEVICE,

	.bcdUSB = cpu_to_le16(0x0200),

	.bDeviceClass = USB_CLASS_PER_INTERFACE, //USB_CLASS_COMM,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	/* .bMaxPacketSize0 = f(hardware) */

	/* Vendor and product id defaults change according to what configs
     * we support.  (As does bNumConfigurations.)  These values can
     * also be overridden by module parameters.
     */
    .idVendor = 0,  /* dynamic */
    .idProduct = 0, /* dynamic */	
	.bcdDevice = cpu_to_le16(WINUSB_DEVICE_BCD),

	/* .iManufacturer = DYNAMIC */
	/* .iProduct = DYNAMIC */
	/* .iSerialNumber = DYNAMIC */
	.bNumConfigurations = 1,
};


/* 
 * String Descriptors
 */
struct usb_string strings_dev[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "Hichip Inc.",
	[USB_GADGET_PRODUCT_IDX].s = DRIVER_DESC,
	[USB_GADGET_SERIAL_IDX].s = "123456789abc",
	{} /* end of list */
};


/* 
 * Interface Descriptor
 */
__attribute__((weak)) struct usb_interface_descriptor winusb_intf = {
	.bLength = sizeof(struct usb_interface_descriptor),
	.bDescriptorType = USB_DT_INTERFACE,

	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass = USB_SUBCLASS_VENDOR_SPEC,
	.bInterfaceProtocol = USB_CLASS_PER_INTERFACE,
	/* .iInterface = DYNAMIC */
};


/* 
 * Endpoint Descriptors
 */
/* full speed support: */
struct usb_endpoint_descriptor fs_winusb_ep_in_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
};
struct usb_endpoint_descriptor fs_winusb_ep_out_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
};

/* high speed support: */
struct usb_endpoint_descriptor hs_winusb_ep_in_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(1024),
};
struct usb_endpoint_descriptor hs_winusb_ep_out_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,

	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = cpu_to_le16(1024),
};



/* 
 * 响应请求号为vendor code (bVendorCode), 并且index为5的厂商自定义请求
 */
char *g_ext_prop_name = "DeviceInterfaceGUID";
char g_ext_prop_data[] = "{1D4B2365-4749-48EA-B38A-7C6FDDDD7E26}";
