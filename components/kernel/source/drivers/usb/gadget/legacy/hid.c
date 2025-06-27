#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/usb/g_hid.h>

struct hidg_func_descriptor us_keyboard_data = {
    .subclass        = 0, /* No subclass */
    .protocol        = 1, /* Keyboard */
    .report_length        = 8,
    .report_desc_length    = 63,
    .report_desc        = {
        0x05, 0x01,    /* USAGE_PAGE (Generic Desktop)     */
        0x09, 0x06,    /* USAGE (Keyboard) */
        0xa1, 0x01,    /* COLLECTION (Application) */
        0x05, 0x07,    /* USAGE_PAGE (Keyboard) */
        0x19, 0xe0,    /* USAGE_MINIMUM (Keyboard LeftControl) */
        0x29, 0xe7,    /* USAGE_MAXIMUM (Keyboard Right GUI) */
        0x15, 0x00,    /* LOGICAL_MINIMUM (0) */
        0x25, 0x01,    /* LOGICAL_MAXIMUM (1) */
        0x75, 0x01,    /* REPORT_SIZE (1) */
        0x95, 0x08,    /* REPORT_COUNT (8) */
        0x81, 0x02,    /* INPUT (Data,Var,Abs) */

        0x95, 0x01,    /* REPORT_COUNT (1) */
        0x75, 0x08,    /* REPORT_SIZE (8) */
        0x81, 0x03,    /* INPUT (Cnst,Var,Abs) */

        0x95, 0x05,    /* REPORT_COUNT (5) */
        0x75, 0x01,    /* REPORT_SIZE (1) */
        0x05, 0x08,    /* USAGE_PAGE (LEDs) */
        0x19, 0x01,    /* USAGE_MINIMUM (Num Lock) */
        0x29, 0x05,    /* USAGE_MAXIMUM (Kana) */
        0x91, 0x02,    /* OUTPUT (Data,Var,Abs) */

        0x95, 0x01,    /* REPORT_COUNT (1) */
        0x75, 0x03,    /* REPORT_SIZE (3) */
        0x91, 0x03,    /* OUTPUT (Cnst,Var,Abs) */

        0x95, 0x06,    /* REPORT_COUNT (6) */
        0x75, 0x08,    /* REPORT_SIZE (8) */
        0x15, 0x00,    /* LOGICAL_MINIMUM (0) */
        0x25, 0x65,    /* LOGICAL_MAXIMUM (101) */
        0x05, 0x07,    /* USAGE_PAGE (Keyboard) */
        0x19, 0x00,    /* USAGE_MINIMUM (Reserved) */
        0x29, 0x65,    /* USAGE_MAXIMUM (Keyboard Application) */
        0x81, 0x00,    /* INPUT (Data,Ary,Abs) */

        0xc0        /* END_COLLECTION */
    }
};


/*hid descriptor for a mouse*/
struct hidg_func_descriptor us_mouse_data = {
	.subclass = 0,	/*NO SubClass*/
	.protocol = 2,	/*Mouse*/
	.report_length = 4,
	.report_desc_length = 52,
	.report_desc={
		0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
		0x09, 0x02,        // Usage (Mouse)
		0xA1, 0x01,        // Collection (Application)
		0x09, 0x01,        //   Usage (Pointer)
		0xA1, 0x00,        //   Collection (Physical)
		0x05, 0x09,        //     Usage Page (Button)
		0x19, 0x01,        //     Usage Minimum (0x01)
		0x29, 0x03,        //     Usage Maximum (0x03)
		0x15, 0x00,        //     Logical Minimum (0)
		0x25, 0x01,        //     Logical Maximum (1)
		0x95, 0x03,        //     Report Count (3)
		0x75, 0x01,        //     Report Size (1)
		0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

		0x95, 0x01,        //     Report Count (1)
		0x75, 0x05,        //     Report Size (5)
		0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

		0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
		0x09, 0x30,        //     Usage (X)
		0x09, 0x31,        //     Usage (Y)
		0x09, 0x38,        //     Usage (Wheel)
		0x15, 0x81,        //     Logical Minimum (-127)
		0x25, 0x7F,        //     Logical Maximum (127)
		0x75, 0x08,        //     Report Size (8)
		0x95, 0x03,        //     Report Count (3)
		0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)

		0xC0,              //   End Collection
		0xC0,              // End Collection

		// 52 bytes

	}
};