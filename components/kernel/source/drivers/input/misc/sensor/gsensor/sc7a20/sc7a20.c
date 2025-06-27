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
#include "sc7a20.h"

#if 1
#define mmaprintk(x...) printk(x)
#else
#define mmaprintk(x...)
#endif

#if 0
#define mmaprintkd(x...) printk(x)
#else
#define mmaprintkd(x...)
#endif

#if 1
#define mmaprintkf(x...) printk(x)
#else
#define mmaprintkf(x...)
#endif

#define SC7A20_SPEED 200 * 1000
#define SC7A20_DEVID 0x11
typedef char status_t;
/*status*/
#define SC7A20_OPEN 1
#define SC7A20_CLOSE 0

struct sc7a20_data {
	status_t status;
	char curr_tate;
	struct input_dev *input_dev;
	struct work_struct work;
	struct delayed_work delaywork; /*report second event*/

	struct sc7a20_axis sense_data;
	struct mutex sense_data_mutex;

	char rate;
	u16 swap_xy;
	u16 swap_xyz;
	signed char orientation[9];
	int int_gpio;
	int pwren_gpio;
	int gpio_rstn;
	int power_enabled;
	struct task_struct *task_kthread_p;

	struct timer_list		timer_deel_data;	
	const char 			*i2c_devpath;
	unsigned int 			addr;
};

static char devid;

static int I2C_RxData(struct sc7a20_data *sc7a20, uint8_t *rxData, int length) {
	int fd, ret;

	fd = open(sc7a20->i2c_devpath, O_RDWR);
	if (fd < 0) {
		printf("open %s fail\n", sc7a20->i2c_devpath);
		return -ENODEV;
	}

	struct i2c_transfer_s xfer;
	struct i2c_msg_s msgs[] = {
		{
			.addr = (uint8_t)sc7a20->addr,
			.flags = 0,
			.length = 1,
			.buffer = rxData,
		},
		{
			.addr = (uint8_t)sc7a20->addr,
			.flags = 1,
			.length = length,
			.buffer = rxData,
		},
	};
	xfer.msgv = msgs;
	xfer.msgc = 2;

	ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
	if (ret < 0) {
		mmaprintk("transfer failed.");
		goto err;
	} else if (ret != ARRAY_SIZE(msgs)) {
		mmaprintk("transfer failed(size error).");
		ret = -ENXIO;
		goto err;
	}

	close(fd);
	return 0;
err:
	close(fd);
	return ret;
}

static int I2C_TxData(struct sc7a20_data *sc7a20, uint8_t *txData, int length) {
	int fd, ret;

	fd = open(sc7a20->i2c_devpath, O_RDWR);
	if (fd < 0) {
		printf("open %s fail\n", sc7a20->i2c_devpath);
		return -ENODEV;
	}

	struct i2c_transfer_s xfer;
	struct i2c_msg_s msg[] = {
		{
			.addr = (uint8_t)sc7a20->addr,
			.flags = 0,
			.length = length,
			.buffer = txData,
		},
	};

	xfer.msgv = msg;
	xfer.msgc = 1;

	ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
	if (ret < 0) {
		mmaprintk("transfer failed.");
		goto err;
	} else if (ret != ARRAY_SIZE(msg)) {
		mmaprintk("transfer failed(size error).");
		ret = -ENXIO;
		goto err;
	}

	close(fd);
	return 0;
err:
	close(fd);
	return ret;

}

static char sc7a20_read_reg(struct sc7a20_data *sc7a20, int addr) {
	char tmp;
	int ret = 0;
	tmp = addr;
	ret = I2C_RxData(sc7a20, &tmp, 1);
	return tmp;
}

static int sc7a20_write_reg(struct sc7a20_data *sc7a20, int addr, int value) {
	char buffer[3];
	int ret = 0;

	buffer[0] = addr;
	buffer[1] = value;
	ret = I2C_TxData(sc7a20, &buffer[0], 2);
	return ret;
}

static char sc7a20_get_devid(struct sc7a20_data *sc7a20) {
	return sc7a20_read_reg(sc7a20, SC7A20_REG_WHO_AM_I);
}

static int sc7a20_active(struct sc7a20_data *sc7a20, int enable) {
	int tmp;
	int ret = 0;

	tmp = sc7a20_read_reg(sc7a20, SC7A20_REG_CTRL_REG1);
	if (enable)
		tmp = (sc7a20->rate << SC7A20_RATE_SHIFT) | 0x07;
	else
		tmp = 0x08;
	mmaprintkd("sc7a20_active %s (0x%x)\n", enable ? "active" : "standby", tmp);
	ret = sc7a20_write_reg(sc7a20, SC7A20_REG_CTRL_REG1, tmp);
	return ret;
}

static int sc7a20_start_dev(struct sc7a20_data *sc7a20, char rate) {
	int ret = 0;
	sc7a20->rate = rate;
        mmaprintkd(
            "-------------------------sc7a20 start ------------------------\n");

	/* standby */
	sc7a20_active(sc7a20, 1);
        mmaprintkd("sc7a20 SC7A20_REG_CTRL_REG1(0x%x):%x\n",
                   SC7A20_REG_CTRL_REG1,
                   sc7a20_read_reg(sc7a20, SC7A20_REG_CTRL_REG1));

        ret = sc7a20_write_reg(sc7a20, SC7A20_REG_CTRL_REG2, 0x00); // 0x00
        mmaprintkd("sc7a20 SC7A20_REG_CTRL_REG2(0x%x):%x\n",
                   SC7A20_REG_CTRL_REG2,
                   sc7a20_read_reg(sc7a20, SC7A20_REG_CTRL_REG2));

        ret = sc7a20_write_reg(sc7a20, SC7A20_REG_CTRL_REG3, 0x10);
        mmaprintkd("sc7a20 SC7A20_REG_CTRL_REG3(0x%x):%x\n",
                   SC7A20_REG_CTRL_REG3,
                   sc7a20_read_reg(sc7a20, SC7A20_REG_CTRL_REG3));

        ret = sc7a20_write_reg(sc7a20, SC7A20_REG_CTRL_REG4, 0x88); // 0x10
        mmaprintkd("sc7a20 SC7A20_REG_CTRL_REG4(0x%x):%x\n",
                   SC7A20_REG_CTRL_REG4,
                   sc7a20_read_reg(sc7a20, SC7A20_REG_CTRL_REG4));

        ret = sc7a20_write_reg(sc7a20, SC7A20_REG_CTRL_REG5, 0x00); // 0x00
        mmaprintkd("sc7a20 SC7A20_REG_CTRL_REG5(0x%x):%x\n",
                   SC7A20_REG_CTRL_REG5,
                   sc7a20_read_reg(sc7a20, SC7A20_REG_CTRL_REG5));

        ret = sc7a20_write_reg(sc7a20, SC7A20_REG_CTRL_REG6, 0x02); // 0x02
        mmaprintkd("sc7a20 SC7A20_REG_CTRL_REG6(0x%x):%x\n",
                   SC7A20_REG_CTRL_REG6,
                   sc7a20_read_reg(sc7a20, SC7A20_REG_CTRL_REG6));

        ret = sc7a20_write_reg(sc7a20, SC7A20_REG_INT1_CFG, 0x3F);
        mmaprintkd("sc7a20 SC7A20_REG_INT1_CFG(0x%x):%x\n", SC7A20_REG_INT1_CFG,
                   sc7a20_read_reg(sc7a20, SC7A20_REG_INT1_CFG));

        ret = sc7a20_write_reg(sc7a20, SC7A20_REG_HP_MODE, 0x03);//HP MODE SET
        mmaprintkd("sc7a20 SC7A20_REG_HP_MODE(0x%x):%x\n",
                   SC7A20_REG_HP_MODE,
                   sc7a20_read_reg(sc7a20, SC7A20_REG_HP_MODE));

        sc7a20_active(sc7a20, 1);
        mmaprintkd("sc7a20 SC7A20_REG_CTRL_REG1(0x%x):%x\n",
                   SC7A20_REG_CTRL_REG1,
                   sc7a20_read_reg(sc7a20, SC7A20_REG_CTRL_REG1));

        usleep(50 * 1000);

	return ret;
}
static int sc7a20_soft_reset(struct sc7a20_data *sc7a20) {

	int ret = 0;
        ret = sc7a20_write_reg(sc7a20, SC7A20_SOFE_RESET, 0xa5); // 0x00
        mmaprintkd("sc7a20 SC7A20_SOFE_RESET(0x%x):%x\n", SC7A20_SOFE_RESET,
                   sc7a20_read_reg(sc7a20, SC7A20_SOFE_RESET));
	return ret;
}

static int sc7a20_start(struct sc7a20_data *sc7a20, char rate) {

	mmaprintkf("%s::enter\n", __FUNCTION__);
	if (sc7a20->status == SC7A20_OPEN) {
		return 0;
	}
	sc7a20->status = SC7A20_OPEN;
	return sc7a20_start_dev(sc7a20, rate);
}

static void sc7a20_report_value(struct sc7a20_data *sc7a20,
		struct sc7a20_axis *axis) {

	/* Report acceleration sensor information */
	input_report_abs(sc7a20->input_dev, ABS_X,
			 (int)(axis->x));
	input_report_abs(sc7a20->input_dev, ABS_Y,
			 (int)(axis->y));
	input_report_abs(sc7a20->input_dev, ABS_Z,
			 (int)(axis->z));
	input_sync(sc7a20->input_dev);
	mmaprintkd("Gsensor x==%d  y==%d z==%d\n", axis->x, axis->y, axis->z);
}

static inline int sc7a20_convert_to_int(const char high_byte,
		const char low_byte) {
	s64 result;

	switch (devid) {
		case SC7A20_DEVID:
			result = (((s16)((high_byte << 8) + low_byte)) >> 4);
			if (result < SC7A20_BOUNDARY)
				result = result * SC7A20_GRAVITY_STEP;
			else
				result = ~(((~result & (0x7ffff >> (20 - SC7A20_PRECISION))) + 1) *
						SC7A20_GRAVITY_STEP) + 1;
			break;

		default:
			mmaprintk(KERN_ERR "sc7a20_convert_to_int: devid wasn't set correctly\n");
			return -EFAULT;
	}

	return (int)result;
}

static int sc7a20_get_data(struct sc7a20_data *sc7a20)
{
	int ret;
	int x, y, z;
	struct sc7a20_axis axis;
	char buffer[6];

	memset(buffer, 0, 6);
	buffer[0] = SC7A20_REG_STATUS;
	ret = I2C_RxData(sc7a20, &buffer[0], 1);
	if (ret < 0) {
		printk("error\n");
		return ret;
	}

	mmaprintkd("SC7A20_REG_STATUS(0x27) = 0x%x \n", buffer[0]);
	if (!(buffer[0] & 0x08)) {
		mmaprintkd("data no ready\n");
		return ret;
	}

	memset(buffer, 0, 6);
	buffer[0] = SC7A20_REG_X_OUT_LSB | 0x80;
	ret = I2C_RxData(sc7a20, &buffer[0], 1);
	buffer[1] = SC7A20_REG_X_OUT_MSB | 0x80;
	ret = I2C_RxData(sc7a20, &buffer[1], 1);
	buffer[2] = SC7A20_REG_Y_OUT_LSB | 0x80;
	ret = I2C_RxData(sc7a20, &buffer[2], 1);
	buffer[3] = SC7A20_REG_Y_OUT_MSB | 0x80;
	ret = I2C_RxData(sc7a20, &buffer[3], 1);
	buffer[4] = SC7A20_REG_Z_OUT_LSB | 0x80;
	ret = I2C_RxData(sc7a20, &buffer[4], 1);
	buffer[5] = SC7A20_REG_Z_OUT_MSB | 0x80;
	ret = I2C_RxData(sc7a20, &buffer[5], 1);
	if (ret < 0)
		return ret;
	mmaprintkd("%s: --sc7a20_buffer= %d  %d  %d %d  %d  %d-----\n",
		   __func__, buffer[0], buffer[1], buffer[2], buffer[3],
		   buffer[4], buffer[5]);
	x = sc7a20_convert_to_int(buffer[1], buffer[0]);
	y = sc7a20_convert_to_int(buffer[3], buffer[2]);
	z = sc7a20_convert_to_int(buffer[5], buffer[4]);

	if (sc7a20->swap_xyz) {
		axis.x = (sc7a20->orientation[0]) * x +
			 (sc7a20->orientation[1]) * y +
			 (sc7a20->orientation[2]) * z;
		axis.y = (sc7a20->orientation[3]) * x +
			 (sc7a20->orientation[4]) * y +
			 (sc7a20->orientation[5]) * z;
		axis.z = (sc7a20->orientation[6]) * x +
			 (sc7a20->orientation[7]) * y +
			 (sc7a20->orientation[8]) * z;
	} else {
		axis.x = x;
		axis.y = y;
		axis.z = z;
	}

	if (sc7a20->swap_xy) {
		axis.x = -axis.x;
		swap(axis.x, axis.y);
	}

	mmaprintkd("%s: ------------------sc7a20_GetData axis = %d  %d  "
		   "%d--------------\n",
		   __func__, axis.x, axis.y, axis.z);

	sc7a20_report_value(sc7a20, &axis);

	return 0;
}

static void timer_callback(struct timer_list *param)
{
	struct sc7a20_data *sc7a20 = from_timer(sc7a20, param, timer_deel_data);

	mutex_lock(&sc7a20->sense_data_mutex);

	if (sc7a20_get_data(sc7a20) < 0) {
		mmaprintkd(KERN_ERR "SC7A20 timer: Get data failed\n");
		return;
	}
	mutex_unlock(&sc7a20->sense_data_mutex);

	mod_timer(&sc7a20->timer_deel_data,
		  jiffies + usecs_to_jiffies(100 * 1000));
}

static int hc_sc7a20_probe(void)
{
	struct sc7a20_data *sc7a20;

	int np;
	int err = -1;

	sc7a20 = kzalloc(sizeof(struct sc7a20_data), GFP_KERNEL);
	if (!sc7a20) {
		mmaprintk("[sc7a20]:alloc data failed.\n");
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	memset(&(sc7a20->sense_data), 0, sizeof(struct sc7a20_axis));

	np = fdt_get_node_offset_by_path("/hcrtos/sc7a20@19");
	if (np < 0) {
		printf("no find sc7a20@19\n");
		err = -ENODEV;
		return err;
	}

	if (fdt_get_property_u_32_index(np, "reg", 0, (u32 *)&sc7a20->addr))
		return -1;
	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &sc7a20->i2c_devpath))
		return -1;

	mutex_init(&(sc7a20->sense_data_mutex));

        devid = sc7a20_get_devid(sc7a20);
        if ((SC7A20_DEVID != devid)) {
		pr_info("sc7a20: invalid devid\n");
		goto exit_invalid_devid;
	}

	sc7a20->input_dev = input_allocate_device();
	if (!sc7a20->input_dev) {
		err = -ENOMEM;
		mmaprintk(KERN_ERR "sc7a20_probe: Failed to allocate input device\n");
		goto exit_input_allocate_device_failed;
	}

	sc7a20->input_dev->name = "sc7a20_gsensor";
	set_bit(EV_ABS, sc7a20->input_dev->evbit);

	/* x-axis acceleration */
	input_set_abs_params(sc7a20->input_dev, ABS_X, -0x80000000, 0x80000000, 0,
			0); // 2g full scale range
	/* y-axis acceleration */
	input_set_abs_params(sc7a20->input_dev, ABS_Y, -0x80000000, 0x80000000, 0,
			0); // 2g full scale range
	/* z-axis acceleration */
	input_set_abs_params(sc7a20->input_dev, ABS_Z, -0x80000000, 0x80000000, 0,
			0); // 2g full scale range

	err = input_register_device(sc7a20->input_dev);
	if (err < 0) {
		mmaprintk(KERN_ERR "sc7a20_probe: Unable to register input device.\n");
		goto exit_input_register_device_failed;
	}

	sc7a20_soft_reset(sc7a20);
	sc7a20_start(sc7a20, SC7A20_RATE_50);
        timer_setup(&sc7a20->timer_deel_data, timer_callback, 0);

	mod_timer(&sc7a20->timer_deel_data,
		  jiffies + usecs_to_jiffies(500 * 1000));

	mmaprintkf(KERN_INFO "sc7a20 probe ok\n");
	return err;

exit_input_register_device_failed:
	input_free_device(sc7a20->input_dev);
exit_input_allocate_device_failed:
exit_invalid_devid:
	kfree(sc7a20);
exit_alloc_data_failed:
	mmaprintk("%s error\n", __FUNCTION__);
	return -1;
}

static int hc_sc7a20_init(void)
{
	int ret = 0;

	ret = hc_sc7a20_probe();

	return ret;
}

module_driver(hc_sc7a20, hc_sc7a20_init, NULL, 1)
