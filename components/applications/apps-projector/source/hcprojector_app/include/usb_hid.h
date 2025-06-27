
#ifndef __USB_HID_H__
#define __USB_HID_H__

typedef enum 
{
    HID_NONE = -1,
    HID_KEYBOARD = 0,
    HID_MOUSE,
    HID_SIGLE_TOUCH,
    HID_MULTI_TOUCH,
    HID_JOYSTICK,
    HID_CAMERA,
    HID_GESTURE,
    HID_REMOTE_CONTROL,
} hid_type_e;

typedef int (*hid_event_cb)(hid_type_e type, const char*report, int report_len);

hid_type_e usb_hid_get_device_type(int fd);

int usb_hid_register_fd(hid_type_e type, const char *str);
int usb_hid_unregister_fd(hid_type_e type, const char *str);

int usb_hid_set_event_cb(hid_event_cb cb);
int usb_hid_get_total(void);

#endif
