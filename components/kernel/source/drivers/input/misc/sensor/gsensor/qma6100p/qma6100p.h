#ifndef __QMA6100P_H
#define __QMA6100P_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <string.h>


#ifndef QST_BASE_TYPE
#define QST_BASE_TYPE
typedef		signed char					qs8;
typedef		unsigned char				qu8;
typedef		short						qs16;
typedef		unsigned short				qu16;
typedef		int							qs32;
typedef		unsigned int				qu32;
typedef		long long					qs64;
typedef		unsigned long long			qu64;
typedef		float						qf32;
typedef		double						qd64;
#endif

#define QMA6100P_DEVICE_ID				0x09
#define QMA6100P_I2C_SLAVE_ADDR			0x12
#define QMA6100P_I2C_SLAVE_ADDR2		0x13
#define QMA6100P_FAIL					0
#define QMA6100P_SUCCESS				1

#define GRAVITY_EARTH_1000 				9807	// about (9.80665f)*1000   mm/s2
#define QMA6100P_ABS(X) 				((X) < 0 ? (-1 * (X)) : (X))

#define QMA6100P_DELAY				0xff
/*Register Map*/
#define QMA6100P_CHIP_ID		    0x00
#define QMA6100P_XOUTL				0x01
#define QMA6100P_XOUTH				0x02
#define QMA6100P_YOUTL				0x03
#define QMA6100P_YOUTH				0x04
#define QMA6100P_ZOUTL				0x05
#define QMA6100P_ZOUTH				0x06
#define QMA6100P_STEP_CNT_L			0x07
#define QMA6100P_STEP_CNT_M			0x08
#define QMA6100P_INT_STATUS_0		0x09
#define QMA6100P_INT_STATUS_1		0x0a
#define QMA6100P_INT_STATUS_2		0x0b
#define QMA6100P_INT_STATUS_3		0x0c
#define QMA6100P_STEP_CNT_H			0x0d
#define QMA6100P_FIFO_STATE			0x0e
#define QMA6100P_REG_RANGE			0x0f
#define QMA6100P_REG_BW_ODR			0x10
#define QMA6100P_REG_POWER_MANAGE	0x11
#define QMA6100P_STEP_SAMPLE_CNT	0x12
#define QMA6100P_STEP_PRECISION		0x13
#define QMA6100P_STEP_TIME_LOW		0x14
#define QMA6100P_STEP_TIME_UP		0x15
#define QMA6100P_INT_EN_0			0x16
#define QMA6100P_INT_EN_1			0x17
#define QMA6100P_INT_EN_2			0x18
#define QMA6100P_INT1_MAP_0			0x19
#define QMA6100P_INT1_MAP_1			0x1a
#define QMA6100P_INT2_MAP_0			0x1b
#define QMA6100P_INT2_MAP_1			0x1c

#define QMA6100P_INTPIN_CFG			0x20
#define QMA6100P_INT_CFG			0x21
#define QMA6100P_OS_CUST_X		    0x27
#define QMA6100P_OS_CUST_Y			0x28
#define QMA6100P_OS_CUST_Z			0x29

#define QMA6100P_REG_NVM			0x33
#define QMA6100P_REG_RESET			0x36


#define QMA6100P_DRDY_BIT			0x10	// enable 1

#define QMA6100P_AMD_X_BIT			0x01
#define QMA6100P_AMD_Y_BIT			0x02
#define QMA6100P_AMD_Z_BIT			0x04


typedef enum
{
	QMA6100P_BW_100 = 0,
	QMA6100P_BW_200 = 1,
	QMA6100P_BW_400 = 2,
	QMA6100P_BW_800 = 3,
	QMA6100P_BW_1600 = 4,
	QMA6100P_BW_50 = 5,
	QMA6100P_BW_25 = 6,
	QMA6100P_BW_12_5 = 7,
	QMA6100P_BW_OTHER = 8
}qma6100p_bw;

typedef enum
{
	QMA6100P_RANGE_2G = 0x01,
	QMA6100P_RANGE_4G = 0x02,
	QMA6100P_RANGE_8G = 0x04,
	QMA6100P_RANGE_16G = 0x08,
	QMA6100P_RANGE_32G = 0x0f
}qma6100p_range;

typedef enum
{
	QMA6100P_LPF_OFF = (0x00<<5),
	QMA6100P_LPF_1 = (0x04<<5),
	QMA6100P_LPF_2 = (0x01<<5),
	QMA6100P_LPF_4 = (0x02<<5),
	QMA6100P_LPF_8 = (0x03<<5),
	QMA6100P_LPF_RESERVED = 0xff
}qma6100p_nlpf;


typedef enum
{
	QMA6100P_MODE_STANDBY = 0,
	QMA6100P_MODE_ACTIVE = 1,
	QMA6100P_MODE_MAX
}qma6100p_mode;

typedef enum
{
	QMA6100P_MCLK_102_4K = 0x03,
	QMA6100P_MCLK_51_2K = 0x04,
	QMA6100P_MCLK_25_6K = 0x05,
	QMA6100P_MCLK_12_8K = 0x06,
	QMA6100P_MCLK_6_4K = 0x07,
	QMA6100P_MCLK_RESERVED = 0xff
}qma6100p_mclk;

typedef enum
{
	QMA6100P_FIFO_MODE_NONE,
	QMA6100P_FIFO_MODE_FIFO,
	QMA6100P_FIFO_MODE_STREAM,
	QMA6100P_FIFO_MODE_BYPASS,
	QMA6100P_FIFO_MODE_MAX
}qma6100p_fifo_mode;


extern qs32 qma6100p_writereg(qu8 reg_add,qu8 reg_dat);
extern qs32 qma6100p_readreg(qu8 reg_add,qu8 *buf,qu16 num);
extern qu8 qma6100p_chip_id(void);
extern qs32 qma6100p_init(void);
extern qs32 qma6100p_set_range(qs32 range);
extern qs32 qma6100p_read_raw_xyz(qs16 *data);
extern qs32 qma6100p_read_acc_xyz(float *accData);

extern void qma6100p_irq_hdlr(void);
extern qs8 GetCustomerGsensorPosition(void);

#ifdef __cplusplus
}
#endif
#endif  /*QMA6100P*/
