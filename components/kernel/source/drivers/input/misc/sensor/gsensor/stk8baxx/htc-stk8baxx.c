#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/jiffies.h>
#include <stdio.h>
#include <kernel/drivers/spi.h>
#include <nuttx/fs/fs.h>
#include <linux/mutex.h>
#include <hcuapi/i2c-master.h>
#include <linux/workqueue.h>
#include <linux/printk.h>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <fcntl.h>
#include <stdint.h>

/*##################################################################*/

#define I2C_DRIVERID_BA50       134
#define I2C_DRIVERID_BA53       135 //a53 /a58
#define I2C_DRIVERID_BMA250     250
#define I2C_DRIVERID_BMA253     253

/* STK8BA53/8 Register Map  (Please refer to STK8BA53/8 Specifications) */
#define STK8BA5X_REG_DEVID                  0x00
#define STK8BA5X_REG_BW_RATE                0x10
#define STK8BA5X_BW_200HZ                   0x0d
#define STK8BA5X_BW_100HZ                   0x0c
#define STK8BA5X_BW_50HZ                    0x0b
#define STK8BA5X_BW_25HZ                    0x0a
#define STK8BA5X_BW_15HZ                    0x09
#define STK8BA5X_REG_RANGE                  0x0f
#define STK8BA5X_RANGE_2G                   0x03
#define STK8BA5X_RANGE_4G                   0x05
#define STK8BA5X_RANGE_8G                   0x08
#define STK8BA5XREG_DATAXLOW                0x02

/*##################################################################*/

#define HTC_STK8BAXX_DEBUG	0

struct stk8baxx_data {
	s32				x;
	s32				y;
	s32				z;
	unsigned int 			addr;
	const char 			*i2c_devpath;
	struct input_dev 		*input_dev;
	struct timer_list		timer_deel_data;	
	struct mutex 			sense_data_mutex;
};

static int i2c_read_data(struct stk8baxx_data *priv, uint8_t *rxData, int length) {
	int fd, ret;

	fd = open(priv->i2c_devpath, O_RDWR);
	if (fd < 0) {
		printf("open %s fail\n", priv->i2c_devpath);
		return -ENODEV;
	}

	struct i2c_transfer_s xfer;
	struct i2c_msg_s msgs[] = {
		{
			.addr = (uint8_t)priv->addr,
			.flags = 0,
			.length = 1,
			.buffer = rxData,
		},
		{
			.addr = (uint8_t)priv->addr,
			.flags = 1,
			.length = length,
			.buffer = rxData,
		},
	};
	xfer.msgv = msgs;
	xfer.msgc = 2;

	ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
	if (ret < 0) {
		printk("transfer failed.");
		goto err;
	} else if (ret != ARRAY_SIZE(msgs)) {
		printk("transfer failed(size error).");
		ret = -ENXIO;
		goto err;
	}

	close(fd);
	return 0;
err:
	close(fd);
	return ret;
}

static int i2c_write_data(struct stk8baxx_data *priv, uint8_t reg,
			  uint8_t *txData, int length)
{
	int fd, ret;

	fd = open(priv->i2c_devpath, O_RDWR);
	if (fd < 0) {
		printf("open %s fail\n", priv->i2c_devpath);
		return -ENODEV;
	}

	uint8_t txbuf[1 + length];
	txbuf[0] = reg;

	for (int i = 0; i < length; i++)
		txbuf[i + 1] = txData[i];

	struct i2c_transfer_s xfer;
	struct i2c_msg_s msg[] = {
		{
			.addr = (uint8_t)priv->addr,
			.flags = 0,
			.length = length,
			.buffer = txbuf,
		},
	};

	xfer.msgv = msg;
	xfer.msgc = 1;

	ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
	if (ret < 0) {
		printk("transfer failed.");
		goto err;
	} else if (ret != ARRAY_SIZE(msg)) {
		printk("transfer failed(size error).");
		ret = -ENXIO;
		goto err;
	}

	close(fd);
	return 0;
err:
	close(fd);
	return ret;

}

int stk8baxx_read_data(struct stk8baxx_data *priv, u8 reg, u8 *buf, int len)
{
	int ret = 0;

	buf[0] = reg;
	ret = i2c_read_data(priv, buf, len);
	if (ret < 0) {
		printk("stk8baxx_read_data failed !\n");
	}

	return ret;
}

int stk8baxx_write_data(struct stk8baxx_data *priv, u8 reg, u8 *buf, int len)
{

    int ret = 0;

    ret = i2c_write_data(priv, reg, buf, len);
    if(ret < 0)
    {
        printk( "stk8baxx_write_data failed !\n");
    }

    return ret;
}

static int stk8baxx_CheckDeviceID(struct stk8baxx_data *priv)
{
	u8 CheckDeviceID[1];
	int res = 0;

	res = stk8baxx_read_data(priv, STK8BA5X_REG_DEVID, CheckDeviceID, 1);

	if (res < 0) {
		printk("CheckDeviceID Read Fail ! \n");
		return -1;
	}

	printk("CheckDeviceID is %d pass!\n ", CheckDeviceID[0]);

	return 0;
}

static int stk8baxx_SetBWRate(struct stk8baxx_data *priv)
{
	u8 databuf = 0;
	int res = 0;

	res = stk8baxx_read_data(priv, STK8BA5X_REG_BW_RATE, &databuf, 1);
	if (res < 0) {
		printk("STK8BA5X Rate Read Fail ! \n");
		return -1;
	}
	printk("STK8BA5X def rate [%d]\n", databuf);

	databuf = STK8BA5X_BW_15HZ;
	res = stk8baxx_write_data(priv, STK8BA5X_REG_BW_RATE, &databuf, 1);
	if (res < 0) {
		printk("STK8BA5X Rate Write Fail ! \n");
		return -1;
	}

	return 0;
}

static int stk8baxx_SetRange(struct stk8baxx_data *priv)
{
	u8 databuf = 0;
	int res = 0;

	res = stk8baxx_read_data(priv, STK8BA5X_REG_RANGE, &databuf, 1);
	if (res < 0) {
		printk("STK8BA5X Rate Read Fail ! \n");
		return -1;
	}
	printk("STK8BA5X def range [%d]\n", databuf);

	databuf = STK8BA5X_RANGE_2G;
	res = stk8baxx_write_data(priv, STK8BA5X_REG_RANGE, &databuf, 1);
	if (res < 0) {
		printk("STK8BA5X Rate Write Fail ! \n");
		return -1;
	}

	return 0;
}

static s32 Hotack_SensorConvertData(u8 high_byte, u8 low_byte)
{
	s32 resultVal = 0;

	resultVal = ((((high_byte & 0x00FF) << 8) | (low_byte & 0x00FF)) >> 4);

	//判断是正整数还是负整数
	if ((high_byte & 0x00FF) >> 7) {
		//负数
		resultVal = -(((~resultVal) & 0x0FFF) + 1);
	}

	//更新数据检测
	if (!(low_byte & 0x01)) {
		if (HTC_STK8BAXX_DEBUG)
			printk("Sensor Data not updated !\n");
	}

	return resultVal;
}

static int Hotack_SensorReportValue(struct stk8baxx_data *priv, s32 *out_x,
				    s32 *out_y, s32 *out_z)
{
	int ret = 0;
	u8 Index = 0;
	u8 *buff;
	s32 x, y, z;

	buff = malloc(6 * sizeof(u8));
	//清空buff
	memset(buff, 0, sizeof(buff));

	//Read [x y z] data from sensor
	for (Index = 0; Index < 6; Index++) {
		ret = stk8baxx_read_data(priv, STK8BA5XREG_DATAXLOW + Index,
					 &buff[Index], 1);
		if (ret == -1) {
			printk("Hotack_SensorReportValue Read i2c error!\n");
			return -1;
		}
	}

	if (HTC_STK8BAXX_DEBUG)
		printk("Sensor Buff[x] = 0x%x%x, Buff[y] = 0x%x%x, Buff[z]=0x%x%x\n",
		       buff[1], buff[0], buff[3], buff[2], buff[5], buff[4]);

	//this gsensor need 6 bytes buffer [note: buff[1][3][5] is hight bit, buff[0][2][4] is low bit]
	x = Hotack_SensorConvertData(buff[1], buff[0]);
	y = Hotack_SensorConvertData(buff[3], buff[2]);
	z = Hotack_SensorConvertData(buff[5], buff[4]);

	if (HTC_STK8BAXX_DEBUG)
		printk("Gsensor -> x = %d, y = %d, z = %d\n", x, y, z);

	*out_x = x;
	*out_y = y;
	*out_z = z;

	return ret;
}

int stk8baxx_SetInit(struct stk8baxx_data *priv)
{
	stk8baxx_CheckDeviceID(priv);

	stk8baxx_SetBWRate(priv);

	stk8baxx_SetRange(priv);

	return 0;
}

static void timer_callback(struct timer_list *param)
{
	int ret = 0;
	struct stk8baxx_data *priv = from_timer(priv, param, timer_deel_data);
	s32 x, y, z;

	mutex_lock(&priv->sense_data_mutex);

	ret = Hotack_SensorReportValue(priv, &x, &y, &z);
	if (ret < 0) {
		printf("htc stk8baxx timer: Get data failed\n");
		return;
	}

	input_report_abs(priv->input_dev, ABS_X, x);
	input_report_abs(priv->input_dev, ABS_Y, y);
	input_report_abs(priv->input_dev, ABS_Z, z);
	input_sync(priv->input_dev);

	mutex_unlock(&priv->sense_data_mutex);

	mod_timer(&priv->timer_deel_data,
		  jiffies + usecs_to_jiffies(100 * 1000));
}

static int hc_htc_stk8baxx_probe(void)
{
	int np, err;
	struct stk8baxx_data *priv;

	priv = malloc(sizeof(struct stk8baxx_data));
	if (!priv) {
		printf("%s:%d Failed to stk8baxx_data device.\n", __func__, __LINE__);
		return -ENOMEM;
	}
	memset(priv, 0, sizeof(struct stk8baxx_data));

	np = fdt_get_node_offset_by_path("/hcrtos/stk8ba58@18");
	if (np < 0)
		return -ENODEV;

	if (fdt_get_property_u_32_index(np, "i2c-addr", 0, (u32 *)&priv->addr))
		return -ENODEV;
	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &priv->i2c_devpath))
		return -ENODEV;
//	if (fdt_get_property_u_32_index(np, "irq-gpio", 0, (u32 *)&priv->irq))
//		return -ENODEV;

	printf("i2c devpath = %s\n", priv->i2c_devpath);
	printf("i2c addr = %d\n", priv->addr);
	priv->input_dev = input_allocate_device();
	if (!priv->input_dev) {
		printf("%s:%d Failed to allocate input device.\n", __func__, __LINE__);
		return -ENOMEM;
	}

	priv->input_dev->name = "stk8baxx_gsensor";
	set_bit(EV_ABS, priv->input_dev->evbit);

	/* x-axis acceleration */
	input_set_abs_params(priv->input_dev, ABS_X, -2048, 2048, 0,
			0); // 2g full scale range
	/* y-axis acceleration */
	input_set_abs_params(priv->input_dev, ABS_Y, -2048, 2048, 0,
			0); // 2g full scale range
	/* z-axis acceleration */
	input_set_abs_params(priv->input_dev, ABS_Z, -2048, 2048, 0,
			0); // 2g full scale range

	err = input_register_device(priv->input_dev);
	if (err < 0) {
		printk(KERN_ERR "sc7a20_probe: Unable to register input device.\n");
		return -ENODEV;
	}

	stk8baxx_SetInit(priv);

	mutex_init(&(priv->sense_data_mutex));

        timer_setup(&priv->timer_deel_data, timer_callback, 0);

	mod_timer(&priv->timer_deel_data,
		  jiffies + usecs_to_jiffies(500 * 1000));
	return 0;
}

static int hc_htc_stk8baxx_init(void)
{
	int ret = 0;

	ret = hc_htc_stk8baxx_probe();

	return ret;
}

module_driver(hc_htc_stk8baxx, hc_htc_stk8baxx_init, NULL, 1)
