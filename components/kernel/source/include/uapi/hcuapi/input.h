/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 1999-2002 Vojtech Pavlik
 */
#ifndef _INPUT_H
#define _INPUT_H

#ifdef __HCRTOS__

#include <hcuapi/iocbase.h>
#include <hcuapi/input-event-codes.h>
#ifndef __KERNEL__
#include <stdint.h>
#endif
#include <sys/time.h>
#include <linux/types.h>

struct input_id {
	uint16_t bustype;
	uint16_t vendor;
	uint16_t product;
	uint16_t version;
};

/**
 * struct input_absinfo - used by EVIOCGABS/EVIOCSABS ioctls
 * @value: latest reported value for the axis.
 * @minimum: specifies minimum value for the axis.
 * @maximum: specifies maximum value for the axis.
 * @fuzz: specifies fuzz value that is used to filter noise from
 *      the event stream.
 * @flat: values that are within this value will be discarded by
 *      joydev interface and reported as 0 instead.
 * @resolution: specifies resolution for the values reported for
 *      the axis.
 *
 * Note that input core does not clamp reported values to the
 * [minimum, maximum] limits, such task is left to userspace.
 *
 * Resolution for main axes (ABS_X, ABS_Y, ABS_Z) is reported in
 * units per millimeter (units/mm), resolution for rotational axes
 * (ABS_RX, ABS_RY, ABS_RZ) is reported in units per radian.
 */
struct input_absinfo {
	__s32 value;
	__s32 minimum;
	__s32 maximum;
	__s32 fuzz;
	__s32 flat;
	__s32 resolution;
};

struct input_event {
	struct timeval time;
	uint16_t type;
	uint16_t code;
	int32_t value;
};

/**
 * struct input_keymap_entry - used by EVIOCGKEYCODE/EVIOCSKEYCODE ioctls
 * @scancode: scancode represented in machine-endian form.
 * @len: length of the scancode that resides in @scancode buffer.
 * @index: index in the keymap, may be used instead of scancode
 * @flags: allows to specify how kernel should handle the request. For
 *	example, setting INPUT_KEYMAP_BY_INDEX flag indicates that kernel
 *	should perform lookup in keymap by @index instead of @scancode
 * @keycode: key code assigned to this scancode
 *
 * The structure is used to retrieve and modify keymap data. Users have
 * option of performing lookup either by @scancode itself or by @index
 * in keymap entry. EVIOCGKEYCODE will also return scancode or index
 * (depending on which element was used to perform lookup).
 */
struct input_keymap_entry {
#define INPUT_KEYMAP_BY_INDEX	(1 << 0)
	uint8_t  flags;
	uint8_t  len;
	uint16_t index;
	uint32_t keycode;
	uint8_t  scancode[32];
};

#define EVIOCGVERSION		_IOR('E', 0x01, int)			/* get driver version */
#define EVIOCGID		_IOR('E', 0x02, struct input_id)	/* get device ID */
#define EVIOCGREP		_IOR('E', 0x03, unsigned int[2])	/* get repeat settings */
#define EVIOCSREP		_IOW('E', 0x03, unsigned int[2])	/* set repeat settings */

#define EVIOCGKEYCODE	_IOR(INPUT_IOCBASE, 0x04, struct input_keymap_entry)
#define EVIOCSKEYCODE	_IOW(INPUT_IOCBASE, 0x04, struct input_keymap_entry)

#define EVIOCGNAME(len)	_IOC(_IOC_READ, 'E', 0x06, len)         /* get device name */
#define EVIOCGPHYS(len)	_IOC(_IOC_READ, 'E', 0x07, len)		/* get physical location */
#define EVIOCGUNIQ(len)	_IOC(_IOC_READ, 'E', 0x08, len)		/* get unique identifier */
#define EVIOCGPROP(len)	_IOC(_IOC_READ, 'E', 0x09, len)		/* get device properties */

/**
 * EVIOCGMTSLOTS(len) - get MT slot values
 * @len: size of the data buffer in bytes
 *
 * The ioctl buffer argument should be binary equivalent to
 *
 * struct input_mt_request_layout {
 *      __u32 code;
 *      __s32 values[num_slots];
 * };
 *
 * where num_slots is the (arbitrary) number of MT slots to extract.
 *
 * The ioctl size argument (len) is the size of the buffer, which
 * should satisfy len = (num_slots + 1) * sizeof(__s32).  If len is
 * too small to fit all available slots, the first num_slots are
 * returned.
 *
 * Before the call, code is set to the wanted ABS_MT event type. On
 * return, values[] is filled with the slot values for the specified
 * ABS_MT code.
 *
 * If the request code is not an ABS_MT value, -EINVAL is returned.
 */
#define EVIOCGBIT(ev,len)       _IOC(_IOC_READ, 'E', 0x20 + (ev), len)  /* get event bits */
#define EVIOCGABS(abs)          _IOR('E', 0x40 + (abs), struct input_absinfo)   /* get abs value/limits */

#define EVIOCGRAB               _IOW('E', 0x90, int)                    /* Grab/Release device */

/*
 * IDs.
 */

#define ID_BUS			0
#define ID_VENDOR		1
#define ID_PRODUCT		2
#define ID_VERSION		3

#define BUS_PCI			0x01
#define BUS_ISAPNP		0x02
#define BUS_USB			0x03
#define BUS_HIL			0x04
#define BUS_BLUETOOTH		0x05
#define BUS_VIRTUAL		0x06

#define BUS_ISA			0x10
#define BUS_I8042		0x11
#define BUS_XTKBD		0x12
#define BUS_RS232		0x13
#define BUS_GAMEPORT		0x14
#define BUS_PARPORT		0x15
#define BUS_AMIGA		0x16
#define BUS_ADB			0x17
#define BUS_I2C			0x18
#define BUS_HOST		0x19
#define BUS_GSC			0x1A
#define BUS_ATARI		0x1B
#define BUS_SPI			0x1C

/*
 * MT_TOOL types
 */
#define MT_TOOL_FINGER		0
#define MT_TOOL_PEN		1
#define MT_TOOL_PALM		2
#define MT_TOOL_MAX		2

/*
 * Values describing the status of a force-feedback effect
 */
#define FF_STATUS_STOPPED	0x00
#define FF_STATUS_PLAYING	0x01
#define FF_STATUS_MAX		0x01


#endif

#endif
