#ifndef __BOOTINFO_H
#define __BOOTINFO_H

/* Using 0xA0000000 ~ 0xA0000020 for bootinfo */
struct bootinfo {
	unsigned char hcprogrammer_usb0_en : 1;
	unsigned char hcprogrammer_usb1_en : 1;
	unsigned char hcprogrammer_winusb_en : 1;
	unsigned char reserved0;
	unsigned short hcprogrammer_irq_detect_timeout;
	unsigned short hcprogrammer_sync_detect_timeout;
	unsigned short preboot_load_time;
	unsigned short adc_calibration;
	unsigned char reserved1[22];
};

#endif
