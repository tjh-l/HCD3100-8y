#include <kernel/module.h>
#include <sys/unistd.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/jiffies.h>
#include <stdio.h>
#include <nuttx/fs/fs.h>

#include <errno.h>
#include <nuttx/fs/fs.h>
#include <hcuapi/spidev.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/printk.h>

#include <hcuapi/gpio.h>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <stdint.h>
#include <hcuapi/i2c-master.h>
#include <freertos/semphr.h>

#include "qma6100p.h"

//#define QMA6100P_DEBUG_MSG
#ifdef  QMA6100P_DEBUG_MSG
#define ALOGI(format, ...) printf("ADC DBG: " format, ##__VA_ARGS__)
#define ALOGD(format, ...) printf("ADC DBG: " format, ##__VA_ARGS__)
#define ALOGD(format, ...) 
#else
#define ALOGI(format, ...)
#define ALOGD(format, ...) 
#endif

#define M_G			9.80665f

typedef struct
{
	qu8				slave;
	qu8				chip_id;
	qs32				lsb_1g;
	qma6100p_fifo_mode		fifo_mode;
	qs32				fifo_len;
	qs16				raw[3];
	//float				acc[3];
	const char 			*i2c_devpath;
	unsigned int 			addr;
	struct input_dev		*input_dev;
	struct timer_list               timer_deel_data;
} qma6100p_data;

static qma6100p_data g_qma6100p;

static int qma6100p_i2c_write(unsigned char *write_reg,
			      unsigned char *write_data)
{
        int fd = 0;
        int ret = 0;
        int retry = 3;
	unsigned char writebuf[2];
	writebuf[0] = *write_reg;
	writebuf[1] = *write_data;

	struct i2c_transfer_s xfer;
        struct i2c_msg_s msgs[] = {
                {
                        .addr = (uint8_t)g_qma6100p.addr,
                        .flags = 0,
                        .length = 2,
                        .buffer = writebuf,
                },
        };

        xfer.msgv = msgs;
        xfer.msgc = 1;

        fd = open(g_qma6100p.i2c_devpath, O_RDWR);
	if (fd < 0) {
		printf("%s open i2c dev error\n", __func__);
		return -ENODEV;
	}

        while (retry--) {
                        ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
                        if (ret < 0) {
                                ALOGI("error.retry = %d\n", retry);
                                continue;
                        }
                break;
        }

        close(fd);

        return ret;                                                    
}

static int qma6100p_i2c_read(qu8 *reg_add, qu8 *buf, qu16 num)
{
        int fd = 0;
        int ret = 0;
        int retry = 3;

        struct i2c_transfer_s xfer;
        struct i2c_msg_s msgs[] = {
                {
                        .addr = (uint8_t)g_qma6100p.addr,
                        .flags = 0,
                        .length = 1,
                        .buffer = reg_add,
                },
                {
                        .addr = (uint8_t)g_qma6100p.addr,
                        .flags = 1,
                        .length = num,
                        .buffer = buf,
                },
        };

        xfer.msgv = msgs;
        xfer.msgc = 2;

	fd = open(g_qma6100p.i2c_devpath, O_RDWR);
	if (fd < 0) {
		printf("%s open i2c dev error\n", __func__);
		return -ENODEV;
	}

	while (retry--) {
                ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
                if (ret < 0) {
                        ALOGI("error.retry = %d\n", retry);
                        continue;
                }
                break;
        }

        close(fd);

        return ret;
}

qs32 qma6100p_writereg(qu8 reg_add, qu8 reg_dat)
{
	qs32 ret = QMA6100P_FAIL;

	ret = qma6100p_i2c_write(&reg_add, &reg_dat);

	ALOGD("qma6100p write reg result = %d\n", ret);

	return ret;
}

qs32 qma6100p_readreg(qu8 reg_add, qu8 *buf, qu16 num)
{
	qs32 ret = QMA6100P_FAIL;

	ret = qma6100p_i2c_read(&reg_add, buf, num);

	ALOGD("qma6100p read reg result = %d\n", ret);

	return ret;
}

qs32 qma6100p_set_range(qs32 range)
{
	qs32 ret = 0;
	qu8 reg_data = (qu8)(range);

	if (range == QMA6100P_RANGE_4G) {
		g_qma6100p.lsb_1g = 2048;
	} else if (range == QMA6100P_RANGE_8G) {
		g_qma6100p.lsb_1g = 1024;
	} else if (range == QMA6100P_RANGE_16G) {
		g_qma6100p.lsb_1g = 512;
	} else if (range == QMA6100P_RANGE_32G) {
		g_qma6100p.lsb_1g = 256;
	} else {
		g_qma6100p.lsb_1g = 4096;
	}

	ret = qma6100p_writereg(QMA6100P_REG_RANGE, reg_data);

	return ret;
}

qs32 qma6100p_set_bw(qs32 bw)
{
	qs32 ret = 0;
	qu8 reg_data = (qu8)(bw | QMA6100P_LPF_OFF);

	ret = qma6100p_writereg(QMA6100P_REG_BW_ODR, reg_data);
	return ret;
}

qs32 qma6100p_set_mode(qs32 mode)
{
	qs32 ret = 0;
	qu8 reg_data = (qu8)QMA6100P_MCLK_51_2K | 0x80;

	if (mode >= QMA6100P_MODE_ACTIVE) {
		ret = qma6100p_writereg(QMA6100P_REG_POWER_MANAGE, reg_data);
	} else {
		ret = qma6100p_writereg(QMA6100P_REG_POWER_MANAGE, 0x00);
	}

	return ret;
}

void qma6100p_dump_reg(void)
{
	qu8 reg_data = 0;
	qu32 i = 0;
	qu8 reg_map[] = { 0x0f, 0x10, 0x11, 0x17, 0x18, 0x1a, 0x1c,
			  0x20, 0x43, 0x45, 0x4a, 0x50, 0x56, 0x57 };
	ALOGI("qma6100p_dump_reg\n");
	for (i = 0; i < sizeof(reg_map) / sizeof(reg_map[0]); i++) {
		qma6100p_readreg(reg_map[i], &reg_data, 1);
		ALOGI("0x%x = 0x%x\n", reg_map[i], reg_data);
	}
	ALOGI("\n");
}

void qma6100p_irq_hdlr(void)
{
	qu8 ret = QMA6100P_FAIL;
	qu8 databuf[4];
	qs32 retry = 0;

	while ((ret == QMA6100P_FAIL) && (retry++ < 10)) {
		ret = qma6100p_readreg(QMA6100P_INT_STATUS_0, databuf, 4);
		if (ret == QMA6100P_SUCCESS) {
			break;
		}
	}
	if (ret == QMA6100P_FAIL) {
		ALOGI("qma6100p_irq_hdlr read status fail!\n");
		return;
	} else {
		ALOGI("irq [0x%x 0x%x 0x%x 0x%x]\n", databuf[0], databuf[1],
		      databuf[2], databuf[3]);
	}
}

void qma6100p_axis_convert(short data_a[3], int layout)
{
	short raw[3];

	raw[0] = data_a[0];
	raw[1] = data_a[1];
	//raw[2] = data[2];

	if (layout >= 4 && layout <= 7) {
		data_a[2] = -data_a[2];
	}

	if (layout % 2) {
		data_a[0] = raw[1];
		data_a[1] = raw[0];
	} else {
		data_a[0] = raw[0];
		data_a[1] = raw[1];
	}

	if ((layout == 1) || (layout == 2) || (layout == 4) || (layout == 7)) {
		data_a[0] = -data_a[0];
	}
	if ((layout == 2) || (layout == 3) || (layout == 6) || (layout == 7)) {
		data_a[1] = -data_a[1];
	}
}

qs32 qma6100p_read_raw_xyz(qs16 data[3])
{
	qu8 databuf[6] = { 0 };
	qs16 raw_data[3];
	qu8 drdy = 0;
	qs32 ret = 0;

	//drdy = (databuf[0]&0x01) + (databuf[2]&0x01) + (databuf[4]&0x01);
	drdy = 0x10;

	if (drdy & 0x10) {
		ret = qma6100p_readreg(QMA6100P_XOUTL, databuf, 6);
		if (ret == QMA6100P_FAIL) {
			ALOGI("read xyz read reg error!!!\n");
			return QMA6100P_FAIL;
		}
		raw_data[0] = (qs16)(((databuf[1] << 8)) | (databuf[0]));
		raw_data[1] = (qs16)(((databuf[3] << 8)) | (databuf[2]));
		raw_data[2] = (qs16)(((databuf[5] << 8)) | (databuf[4]));
		data[0] = raw_data[0] >> 2;
		data[1] = raw_data[1] >> 2;
		data[2] = raw_data[2] >> 2;
		//ALOGI("1--%d	%d	%d\n",raw_data[0],raw_data[1],raw_data[2]);
		//ALOGI("2--%d	%d	%d\n",data[0],data[1],data[2]);

		return QMA6100P_SUCCESS;
	} else {
		ALOGI("read xyz data ready error!!!\n");
		return QMA6100P_FAIL;
	}
}

qs32 qma6100p_read_acc_xyz(float accData[3])
{
	qs32 ret;
	qs16 rawData[3];

	ret = qma6100p_read_raw_xyz(rawData);
	if (ret == QMA6100P_SUCCESS) {
		//qma6100p_axis_convert(rawData, 0);
		g_qma6100p.raw[0] = rawData[0];
		g_qma6100p.raw[1] = rawData[1];
		g_qma6100p.raw[2] = rawData[2];
	}
	accData[0] = (float)(g_qma6100p.raw[0] * M_G) / (g_qma6100p.lsb_1g);
	accData[1] = (float)(g_qma6100p.raw[1] * M_G) / (g_qma6100p.lsb_1g);
	accData[2] = (float)(g_qma6100p.raw[2] * M_G) / (g_qma6100p.lsb_1g);
	ALOGD("accData[0] = %f, accData[1] = %f, accData[2] = %f\n",
	       accData[0], accData[1], accData[2]);

	return QMA6100P_SUCCESS;
}

qs32 qma6100p_soft_reset(void)
{
	qu8 reg_0x33 = 0;
	qu32 retry = 0;

	ALOGI("qma6100p_soft_reset\n");
	qma6100p_writereg(QMA6100P_REG_RESET, 0xb6);
	usleep(5 * 1000);
	qma6100p_writereg(QMA6100P_REG_RESET, 0x00);
	usleep(10 * 1000);

	// check otp
	retry = 0;
	while (retry++ < 100) {
		qma6100p_readreg(QMA6100P_REG_NVM, &reg_0x33, 1);
		ALOGI("confirm-%d read 0x33 = 0x%x\n", retry, reg_0x33);
		if ((reg_0x33 & 0x01) && (reg_0x33 & 0x04)) {
			break;
		}
		usleep(2 * 1000);
	}

	return QMA6100P_SUCCESS;
}

static qs32 qma6100p_initialize(void)
{
	ALOGI("qma6100p_initialize\n");
	qma6100p_soft_reset();
	qma6100p_writereg(0x11, 0x80);
	qma6100p_writereg(0x11, 0x84);
	qma6100p_writereg(0x4a, 0x20);
	qma6100p_writereg(0x56, 0x01);
	qma6100p_writereg(0x5f, 0x80);
	usleep(2 * 1000);
	qma6100p_writereg(0x5f, 0x00);
	usleep(10 * 1000);

	//qma6100p_writereg(QMA6100P_INT_EN_0, 0x00);
	//qma6100p_writereg(QMA6100P_INT_EN_1, 0x00);
	//qma6100p_writereg(QMA6100P_INT_EN_2, 0x00);
	//qma6100p_writereg(QMA6100P_INT1_MAP_0, 0x00);
	//qma6100p_writereg(QMA6100P_INT1_MAP_1, 0x00);
	//qma6100p_writereg(QMA6100P_INT2_MAP_0, 0x00);
	//qma6100p_writereg(QMA6100P_INT2_MAP_1, 0x00);
	//qma6100p_writereg(0x3e, 0x07);

	qma6100p_set_range(QMA6100P_RANGE_4G);
	qma6100p_set_bw(QMA6100P_BW_100);
	qma6100p_set_mode(QMA6100P_MODE_ACTIVE);

#if 0 // MCLK to int1, 52.25K Hz
	qma6100p_writereg(0x49, 0x01);
	qma6100p_writereg(0x56, 0x10);
#endif // MCLK to int1

	unsigned char reg_data[4];
	qma6100p_readreg(0x09, reg_data, 4);
	ALOGI("read status=[0x%x 0x%x 0x%x 0x%x] \n", reg_data[0], reg_data[1],
	      reg_data[2], reg_data[3]);

	qma6100p_dump_reg();

	return QMA6100P_SUCCESS;
}

qs32 qma6100p_init(void)
{
	qs32 ret = QMA6100P_FAIL;
	qu8 slave_addr[2] = { QMA6100P_I2C_SLAVE_ADDR,
			      QMA6100P_I2C_SLAVE_ADDR2 };
	qu8 index = 0;
	qu8 chip_id = 0x00;

	for (index = 0; index < 2; index++) {
		chip_id = 0;
		g_qma6100p.slave = slave_addr[index];
		qma6100p_readreg(QMA6100P_CHIP_ID, &chip_id, 1);
		g_qma6100p.chip_id = (chip_id>>4);
		ALOGI("qma6100p chip_id=0x%x\n", chip_id);
		if (g_qma6100p.chip_id == QMA6100P_DEVICE_ID) {
			ALOGI("qma6100p find slave=0x%x\n",g_qma6100p.slave);
			break;
		}
	}

	if (g_qma6100p.chip_id == QMA6100P_DEVICE_ID) {
		ret = qma6100p_initialize();
	} else {
		ret = QMA6100P_FAIL;
	}

	return ret;
}

qs8 GetCustomerGsensorPosition(void)
{
	qs8 position;
	char strCustomerID[96] = { 0 };

	ALOGD("Customer ID = %s\n", strCustomerID);

	if (strcmp(strCustomerID, "BZX_V1") == 0) {
		ALOGD("it is BZX_V1 Customer\n");
		position = 1;
	} else if (strcmp(strCustomerID, "UB30") == 0) {
		ALOGD("UB30 Customer\n");
		position = 2;

	} else {
		ALOGD("other Customer\n");
		position = 0;
	}
	return position;
}

static void g_qma6100p_callback(struct timer_list *param)
{
	int ret = QMA6100P_FAIL;
	float acc[3];

	ret = qma6100p_read_acc_xyz(acc);
	if (ret == QMA6100P_SUCCESS) {
		/* REPORT XYZ DATA */
		input_report_abs(g_qma6100p.input_dev, ABS_X,
				 (int)(acc[0] * 1000000));
		input_report_abs(g_qma6100p.input_dev, ABS_Y,
				 (int)(acc[1] * 1000000));
		input_report_abs(g_qma6100p.input_dev, ABS_Z,
				 (int)(acc[2] * 1000000));
		input_sync(g_qma6100p.input_dev);
	}

	mod_timer(&g_qma6100p.timer_deel_data, jiffies + usecs_to_jiffies(100 * 1000));

	return;
}

static int hc_qma6100p_probe(void)
{
	int np;
	int ret = -1;

	np = fdt_get_node_offset_by_path("/hcrtos/qma6100p");

	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &g_qma6100p.i2c_devpath))
		return ret;

	if (fdt_get_property_u_32_index(np, "i2c-addr", 0, (u32 *)&g_qma6100p.addr))
		return ret;

	ret = qma6100p_init();
	if (ret < 0) {
		ALOGI("qma6100p init failed\n");
		return ret;
	}

	g_qma6100p.input_dev = input_allocate_device();
	if (!g_qma6100p.input_dev) {                                                      
		ret = -ENOMEM;                                                         
		printf(KERN_ERR "g_qma6100p: Failed to allocate input device\n"); 
		return ret;
	}                                                                              

	g_qma6100p.input_dev->name = "qma6100p_gsensor";
	set_bit(EV_ABS, g_qma6100p.input_dev->evbit);

	/* x-axis acceleration */
	input_set_abs_params(g_qma6100p.input_dev, ABS_X, -0x80000000, 0x80000000, 0,
			     0); // 2g full scale range
	/* y-axis acceleration */
	input_set_abs_params(g_qma6100p.input_dev, ABS_Y, -0x80000000, 0x80000000, 0,
			     0); // 2g full scale range
	/* z-axis acceleration */
	input_set_abs_params(g_qma6100p.input_dev, ABS_Z, -0x80000000, 0x80000000, 0,
			     0); // 2g full scale range

	ret = input_register_device(g_qma6100p.input_dev);
	if (ret < 0) {
		printf(KERN_ERR "g_qma6100p: Unable to register input device.\n");
		return ret;
	}

	timer_setup(&g_qma6100p.timer_deel_data, g_qma6100p_callback, 0);

	mod_timer(&g_qma6100p.timer_deel_data, jiffies + usecs_to_jiffies(500 * 1000));
}

static int hc_qma6100p_init(void)
{
	int ret = 0;

	ret = hc_qma6100p_probe();

	return ret;
}

module_driver(hc_qma6100p_driver, hc_qma6100p_init, NULL, 1)
