/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Definitions for sc7a20 compass chip.
 */
#ifndef SC7A20_H
#define SC7A20_H

#include <linux/kernel.h>

//#define IRQ_MODE 1

#define DRV_VERSION		"1.0.0.0"

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2
#define AXES_NUM 3

/* Default register settings */
#define RBUFF_SIZE		12	/* Rx buffer size */

#define SC7A20_REG_OUT_TEMP_L      		0x0C
#define SC7A20_REG_OUT_TEMP_H      		0x0D
#define SC7A20_REG_WHO_AM_I      		0x0F //RO
#define SC7A20_REG_NVM_WR	      		0x1E
#define SC7A20_REG_HP_MODE	      		0x1F
#define SC7A20_REG_CTRL_REG1			0x20 //RW
#define SC7A20_REG_CTRL_REG2			0x21 //RW
#define SC7A20_REG_CTRL_REG3			0x22 //RW
#define SC7A20_REG_CTRL_REG4			0x23 //RW
#define SC7A20_REG_CTRL_REG5			0x24 //RW
#define SC7A20_REG_CTRL_REG6			0x25 //RW

#define SC7A20_REG_STATUS   	    	0x27 //RO
#define SC7A20_REG_X_OUT_LSB			0x28 //RO
#define SC7A20_REG_X_OUT_MSB			0x29 //RO
#define SC7A20_REG_Y_OUT_LSB			0x2a //RO
#define SC7A20_REG_Y_OUT_MSB			0x2b //RO
#define SC7A20_REG_Z_OUT_LSB			0x2c //RO
#define SC7A20_REG_Z_OUT_MSB			0x2d //RO

#define SC7A20_REG_INT1_CFG				0x30
#define SC7A20_REG_INT1_SOURCE			0x31
#define SC7A20_REG_INT1_THS				0x32
#define SC7A20_REG_INT1_DURATION	    0x33
#define SC7A20_REG_INT2_CFG				0x34
#define SC7A20_REG_INT2_SOURCE			0x35
#define SC7A20_REG_INT2_THS				0x36
#define SC7A20_REG_INT2_DURATION		0x37
#define SC7A20_REG_CLICK_CFG	       	0x38
#define SC7A20_REG_CLICK_SRC		    0x39
#define SC7A20_REG_CLICK_THS		    0x3A
#define SC7A20_REG_TIME_LIMIT		    0x3B
#define SC7A20_REG_TIME_LATEMCY			0x3c
#define SC7A20_REG_TIME_WINDOW			0x3d
#define SC7A20_REG_ACT_THS				0x3E
#define SC7A20_REG_DURATION				0x3F


#define SC7A20_REG_F_SETUP		       	0x9 //RW
#define SC7A20_REG_SYSMOD				0xB //RO
#define SC7A20_REG_INTSRC	    		0xC //RO
#define SC7A20_REG_XYZ_DATA_CFG			0xE //RW
#define SC7A20_REG_HP_FILTER_CUTOFF		0xF //RW
#define SC7A20_REG_PL_STATUS			0x10 //RO
#define SC7A20_REG_PL_CFG				0x11 //RW
#define SC7A20_REG_PL_COUNT				0x12 //RW
#define SC7A20_REG_PL_BF_ZCOMP			0x13 //RW
#define SC7A20_REG_P_L_THS_REG			0x14 //RW
#define SC7A20_REG_FF_MT_CFG			0x15 //RW
#define SC7A20_REG_FF_MT_SRC			0x16 //RO
#define SC7A20_REG_FF_MT_THS			0x17 //RW
#define SC7A20_REG_FF_MT_COUNT			0x18 //RW
#define SC7A20_REG_TRANSIENT_CFG		0x1D //RW
#define SC7A20_REG_TRANSIENT_SRC		0x1E //RO
#define SC7A20_REG_TRANSIENT_THS		0x1F //RW
#define SC7A20_REG_TRANSIENT_COUNT		0x20 //RW
#define SC7A20_REG_PULSE_CFG			0x21 //RW
#define SC7A20_REG_PULSE_SRC			0x22 //RO
#define SC7A20_REG_PULSE_THSX			0x23 //RW
#define SC7A20_REG_PULSE_THSY			0x24 //RW
#define SC7A20_REG_PULSE_THSZ			0x25 //RW
#define SC7A20_REG_PULSE_TMLT			0x26 //RW
#define SC7A20_REG_PULSE_LTCY			0x27 //RW
#define SC7A20_REG_PULSE_WIND			0x28 //RW
#define SC7A20_REG_ASLP_COUNT			0x29 //RW

#define SC7A20_REG_OFF_X				0x2F //RW
#define SC7A20_REG_OFF_Y				0x30 //RW
#define SC7A20_REG_OFF_Z				0x31 //RW
#define SC7A20_SOFE_RESET				0x68

#define MMAIO				0xA1

/* IOCTLs for SC7A20 library */
#define MMA_IOCTL_INIT                  _IO(MMAIO, 0x01)
#define MMA_IOCTL_RESET      	          _IO(MMAIO, 0x04)
#define MMA_IOCTL_CLOSE		           _IO(MMAIO, 0x02)
#define MMA_IOCTL_START		             _IO(MMAIO, 0x03)
#define MMA_IOCTL_GETDATA               _IOR(MMAIO, 0x08, char[RBUFF_SIZE+1])

/* IOCTLs for APPs */
#define MMA_IOCTL_APP_SET_RATE		_IOW(MMAIO, 0x10, char)


/*rate*/
#define SC7A20_RATE_0	        0
#define SC7A20_RATE_1          	1
#define SC7A20_RATE_10          2
#define SC7A20_RATE_25          3
#define SC7A20_RATE_50        	4
#define SC7A20_RATE_100         5
#define SC7A20_RATE_200         6
#define SC7A20_RATE_400         7
#define SC7A20_RATE_1600        8
#define SC7A20_RATE_5000        9
#define SC7A20_RATE_SHIFT		4


#define SC7A20_ASLP_RATE_50          0
#define SC7A20_ASLP_RATE_12P5        1
#define SC7A20_ASLP_RATE_6P25        2
#define SC7A20_ASLP_RATE_1P56        3
#define SC7A20_ASLP_RATE_SHIFT		  6

#define FREAD_MASK				0 /* enabled(1<<1) only if reading MSB 8bits*/
#define SC7A20_RANGE			(2*16384)//780

/* sc7a20 */
#define SC7A20_PRECISION       12
#define SC7A20_BOUNDARY        (0x1 << (SC7A20_PRECISION - 1))
#define SC7A20_GRAVITY_STEP    SC7A20_RANGE / SC7A20_BOUNDARY


/*End of precision adaption*/

#define SC7A20_TOTAL_TIME      10

#define ACTIVE_MASK				1

/*status*/
#define SC7A20_SUSPEND           2
#define SC7A20_OPEN           1
#define SC7A20_CLOSE          0

#define SC7A20_REG_LEN         11

struct sc7a20_axis {
	int x;
	int y;
	int z;
};

struct gsensor_platform_data {
	u16 model;
	u16 swap_xy;
	u16 swap_xyz;
	signed char orientation[9];
	/*int (*get_pendown_state)(void);
	int (*init_platform_hw)(void);
	int (*gsensor_platform_sleep)(void);
	int (*gsensor_platform_wakeup)(void);
	void (*exit_platform_hw)(void);*/
};

#define  GSENSOR_DEV_PATH    "/dev/sc7a20_daemon"

#endif

