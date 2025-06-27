#ifndef __CAST_HID_H__
#define __CAST_HID_H__

#define CAST_HID_TYPE_TOUCHPAD      (1)

#define CAST_HID_CHANNEL_USB        (1)
#define CAST_HID_CHANNEL_AUM        (2)
#define CAST_HID_CHANNEL_BT         (3)

int cast_hid_probe(void);
int cast_hid_release(void);
int cast_hid_start(unsigned int channel);
int cast_hid_stop(void);

#endif
