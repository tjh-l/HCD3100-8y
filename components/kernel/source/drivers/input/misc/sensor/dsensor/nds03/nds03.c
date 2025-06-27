/**
 * @file nds03.c
 * @author linsen.chen
 * @brief NDS03 device registration
 * @version 1.x.2
 * @date 2024-07-17
 * 
 * @copyright ZHUHAI HI CHIP SEMICONDUCTOR CO.LTD.
 * @note dts config

	gpio-i2c@0 {
		sda-pinmux = <PINPAD_B03>;
		scl-pinmux = <PINPAD_B02>;
		status = "okay";
		simulate;
	};
	nds03@5c {
		xshut-pin = <PINPAD_B05>;
		xshut-pin-polarity = <0>;
		reg = <0x5c>;
		target-num = <1>;//1 ~ 4
		// 0 : NDS03_WORK_SINGINGRANGING
		// 1 : NDS03_WORK_CONTIUNOUSRANGING
		// 2 : NDS03_WORK_CALIBRATION
		mode = <1>;
		// 0 : NDS03_INIT_WORK_NORMAL
		// 1 : NDS03_INIT_WORK_CAN_SLEEP
		init-work = <0>;
		// int-pin = <PINPAD_B06>;//Gpio interrupt mode
		i2c-devpath = "/dev/gpio-i2c0";
		status = "okay";
	};
 */
#define LOG_TAG "dds03dev"
#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR

#include <kernel/elog.h>
#include <stdio.h>
#include <linux/mutex.h>
#include <kernel/lib/console.h>
#include <kernel/module.h>
#include <sys/unistd.h>
#include <kernel/lib/fdt_api.h>
#include <errno.h>
#include <kernel/delay.h>
#include <hcuapi/i2c-master.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>
#include <kernel/io.h>
#include <stdlib.h>
#include <string.h>
#include <hcuapi/pinpad.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/workqueue.h>
#include "nds03_comm.h"
#include "nds03_dev.h"
#include "nds03_data.h"
#include "nds03_calib.h"
#include "nds03.h"

#define NDS03_DEBUG 0
#if NDS03_DEBUG
#define nds03_debuf_printf printf
#else
#define nds03_debuf_printf
#endif

#define XSHUT_PIN_POLARITY_HIGH 0
#define XSHUT_PIN_POLARITY_LOW 1

typedef enum NDS03_INIT_WORK_MODE_E {
	NDS03_INIT_WORK_NORMAL = 0,
	NDS03_INIT_WORK_CAN_SLEEP,
} ndso3_init_work_mode_e;

struct nds03_data
{
	NDS03_Dev_t *pNxDevice;
	struct work_struct work;
	struct workqueue_struct *nds03_wq;
	const char *i2c_devpath;
	struct input_dev *input_dev;
	struct timer_list timer_deel_data;
	uint8_t target_num;
	nds03_wrok_mode_e mode;
	ndso3_init_work_mode_e init_work;
	int calibration_distance;
	unsigned int xshut_pin_num;
	unsigned int int_pin_num;
	int cref;
	struct mutex sf_lock;
	bool is_work;
};

static struct nds03_data *nds03prv = NULL;

static void nds03dev_lock_async(void)
{
	mutex_lock(&nds03prv->sf_lock);
}

static void nds03dev_unlock_async(void)
{
	mutex_unlock(&nds03prv->sf_lock);
}

/**
 * @brief NDS03使用平台初始化
 * 
 * @param   pDev        平台设备指针
 * @param   arg         参数指针
 * @return  int8_t   
 * @retval  0:成功, 其他:失败
 */
int8_t nds03_platform_init(NDS03_Platform_t *pdev, void *arg){
    return 0;
}

/**
 * @brief NDS03使用平台释放
 * 
 * @param   pDev        平台设备指针
 * @param   arg         参数指针
 * @return  int8_t   
 * @retval  0:成功, 其他:失败
 */
int8_t nds03_platform_uninit(NDS03_Platform_t *pdev, void *arg){
    return 0;
}

int8_t nds03_i2c_read_nbytes(NDS03_Platform_t *pDev,
				   uint8_t i2c_reg_addr,
				   uint8_t *i2c_read_data, uint16_t len)
{
	uint8_t i2c_addr = pDev->i2c_dev_addr;
	int fd = 0;
	int ret = 0;
	fd = open(nds03prv->i2c_devpath, O_RDWR);
	if(fd < 0)
	{
		log_e("open error %s error\n", nds03prv->i2c_devpath);
		goto error;
	}

	struct i2c_transfer_s xfer;
	struct i2c_msg_s msgs[] = {
		{
			.addr = i2c_addr,
			.flags = 0,
			.length = 1,
			.buffer = &i2c_reg_addr,

		},
		{
			.addr = i2c_addr,
			.flags = I2C_M_RD,
			.length = len,
			.buffer = i2c_read_data,

		},
	};

	xfer.msgv = msgs;
	xfer.msgc = 2;
	ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
	if (ret < 0) {
		log_e("error.retry\n");
	}

	close(fd);
error:
	return NDS03_ERROR_NONE;
}

int8_t nds03_i2c_write_nbytes(NDS03_Platform_t *pDev,  uint8_t reg_addr, uint8_t *i2c_write_data, uint16_t len)
{
	uint8_t i2c_addr = pDev->i2c_dev_addr;
	int ret = 0;
	struct i2c_transfer_s xfer_write;
	struct i2c_msg_s i2c_msg_write;
	int fd = 0;

	fd = open(nds03prv->i2c_devpath, O_RDWR);
	if (fd < 0) {
		log_e("i2c open error %s\n", "/dev/gpio-i2c0");
		return -1;
	}

	uint8_t *buf = (uint8_t *)malloc(len + 1);
	buf[0] = reg_addr;
	if (len > 0)
		memcpy(&buf[1], i2c_write_data, len);

	i2c_msg_write.addr = (i2c_addr);
	i2c_msg_write.flags = 0x0;
	i2c_msg_write.buffer = buf;
	i2c_msg_write.length = len + 1;

	xfer_write.msgv = &i2c_msg_write;
	xfer_write.msgc = 1;

	ret = ioctl(fd, I2CIOC_TRANSFER, &xfer_write);
	if (ret < 0) {
		log_e("%s:i2c read error.\n", __func__);
	}
	free(buf);

	close(fd);
	return NDS03_ERROR_NONE;
}


int8_t nds03_delay_1ms(NDS03_Platform_t *pDev, uint32_t wait_ms)
{
	//For user implement
	usleep(1000 * wait_ms);
	return NDS03_ERROR_NONE;
}

int8_t nds03_delay_10us(NDS03_Platform_t *pDev, uint32_t wait_10us)
{
	//For user implement
	usleep(10 * wait_10us);
	return NDS03_ERROR_NONE;
}

int8_t nds03_set_xshut_pin_level(NDS03_Platform_t *pDev, int8_t level)
{

	switch (pDev->xshut_pin) {
	case 0:
		//For user implement
		gpio_set_output(nds03prv->xshut_pin_num, level);
		gpio_configure(nds03prv->xshut_pin_num, GPIO_DIR_OUTPUT);

		break;
	case 1:
		//For user implement
		gpio_set_output(nds03prv->xshut_pin_num, !level);
		gpio_configure(nds03prv->xshut_pin_num, GPIO_DIR_OUTPUT);
		break;
	default:
		break;
	}
	return NDS03_ERROR_NONE;
}

NDS03_Dev_t d_nds03_device = {
                .platform.i2c_dev_addr = NDS03_DEFAULT_SLAVE_ADDR, 
                .platform.xshut_pin = 0,
            };

static void d_nds03_device_get_data(void)
{
	NDS03_Error ret = NDS03_ERROR_NONE;
	NDS03_Dev_t *pNxDevice = nds03prv->pNxDevice;
	int i = 0;
	NDS03_Error error = NDS03_ERROR_NONE;
	uint8_t targetNum = 0;


	if (nds03prv->mode == NDS03_WORK_SINGINGRANGING) {
		error = NDS03_GetTargetNum(pNxDevice, &targetNum);
		if (error != NDS03_ERROR_NONE) {
			NX_PRINTF("Failed to get number of targets, error code: %d\r\n", error);
			return;
		}
		ret = NDS03_GetSingleRangingData(pNxDevice);
	} else if(nds03prv->mode == NDS03_WORK_CONTIUNOUSRANGING) {
		ret = NDS03_GetContinuousRangingData(pNxDevice);
	}
	else if (nds03prv->mode == NDS03_WORK_CALIBRATION) {
		ret = NDS03_GetSingleRangingData(pNxDevice);
	}

	if (ret == NDS03_ERROR_NONE) {
		/* REPORT XYZ DATA */
		for (i = 0; i < nds03prv->target_num; i++) {
			input_report_rel(nds03prv->input_dev, REL_X, (int)(pNxDevice->ranging_data[i].depth));
			input_report_rel(nds03prv->input_dev, REL_Y, (int)(pNxDevice->ranging_data[i].confi));
			input_report_rel(nds03prv->input_dev, REL_Z, (int)(pNxDevice->ranging_data[i].count));
			input_report_rel(nds03prv->input_dev, REL_RESERVED, i);
			input_sync(nds03prv->input_dev);
		}
		input_sync(nds03prv->input_dev);
	}
	nds03_debuf_printf("depth:%d mm, confi:%d, count:%d\r\n", pNxDevice->ranging_data[0].depth, pNxDevice->ranging_data[0].confi, pNxDevice->ranging_data[0].count);
}

static void d_nds03_device_callback(struct timer_list *param)
{
	d_nds03_device_get_data();
	if(nds03prv->is_work == true)
		mod_timer(&nds03prv->timer_deel_data, jiffies + usecs_to_jiffies(100 * 1000));

	return;
}

static int d_nds03_device_init_config(void)
{
	NDS03_Dev_t *pNxDevice = nds03prv->pNxDevice;
	NDS03_Error ret = NDS03_ERROR_NONE; 
	if(nds03prv->mode == NDS03_WORK_SINGINGRANGING || nds03prv->mode ==  NDS03_WORK_CONTIUNOUSRANGING) {
		if(nds03prv->mode == NDS03_WORK_SINGINGRANGING) {
			ret = NDS03_SetTargetNum(pNxDevice, nds03prv->target_num);
			if(ret != NDS03_ERROR_NONE) {
				log_e("NDS03_InitDevice error\n");
				return ret;
			}
		}

		if(nds03prv->int_pin_num != PINPAD_INVALID)
				NDS03_SetGpio1Config(pNxDevice, NDS03_GPIO1_NEW_MEASURE_READY, NDS03_GPIO1_POLARITY_LOW);

		if(nds03prv->mode == NDS03_WORK_CONTIUNOUSRANGING)
			NDS03_StartContinuousMeasurement(pNxDevice);

		if (nds03prv->int_pin_num == PINPAD_INVALID) {
			timer_setup(&nds03prv->timer_deel_data, d_nds03_device_callback, 0);
			mod_timer(&nds03prv->timer_deel_data, jiffies + usecs_to_jiffies(100 * 1000));
		} else {
			gpio_irq_enable(nds03prv->int_pin_num);
			if(nds03prv->mode == NDS03_WORK_SINGINGRANGING)
				NDS03_StartSingleMeasurement(pNxDevice);
		}
	} else if(nds03prv->mode == NDS03_WORK_CALIBRATION) {
		ret = NDS03_XtalkCalibration(pNxDevice);
		if(NDS03_ERROR_NONE != ret) {
			printf("NDS03_XtalkCalibration error!!\r\n");
			return ret;
		}

		ret = NDS03_OffsetCalibrationAtDepth(pNxDevice, nds03prv->calibration_distance);
		if(NDS03_ERROR_NONE != ret) {
			printf("NDS03_OffsetCalibrationAtDepth error!! %d \r\n", ret);
			return ret;
		}
		timer_setup(&nds03prv->timer_deel_data, d_nds03_device_callback, 0);
		mod_timer(&nds03prv->timer_deel_data, jiffies + usecs_to_jiffies(100 * 1000));
	}
	nds03prv->is_work = true;
}

static void d_nds03_device_stop_config(void)
{
	NDS03_Dev_t *pNxDevice = nds03prv->pNxDevice;
	nds03prv->is_work = false;
	usleep(150 * 1000);

	if (nds03prv->mode == NDS03_WORK_CONTIUNOUSRANGING)
		NDS03_StopContinuousMeasurement(pNxDevice);

	if (nds03prv->int_pin_num == PINPAD_INVALID) {
		del_timer(&nds03prv->timer_deel_data);
	} else {
		gpio_irq_disable(nds03prv->int_pin_num);
		NDS03_SetGpio1Config(pNxDevice, NDS03_GPIO1_FUNCTIONALITY_OFF, NDS03_GPIO1_POLARITY_LOW);
	}
}

static int nds03_gpio_int_get_data(void)
{
	NDS03_Error ret = NDS03_ERROR_NONE;
	NDS03_Dev_t *pNxDevice = nds03prv->pNxDevice;
	int i = 0;
	if (nds03prv->mode == NDS03_WORK_SINGINGRANGING)
	{
		NDS03_GetInterruptRangingData(pNxDevice);
		NDS03_StartSingleMeasurement(pNxDevice);
	}
	else if(nds03prv->mode == NDS03_WORK_CONTIUNOUSRANGING) {
		ret = NDS03_GetInterruptRangingData(pNxDevice);
	}

	if (ret == NDS03_ERROR_NONE) {
		/* REPORT XYZ DATA */
		for (i = 0; i < nds03prv->target_num; i++) {
			input_report_rel(nds03prv->input_dev, REL_X, (int)(pNxDevice->ranging_data[i].depth));
			input_report_rel(nds03prv->input_dev, REL_Y, (int)(pNxDevice->ranging_data[i].confi));
			input_report_rel(nds03prv->input_dev, REL_Z, (int)(pNxDevice->ranging_data[i].count));
			input_report_rel(nds03prv->input_dev, REL_RESERVED, i);
			// input_sync(nds03prv->input_dev);
		}
		input_sync(nds03prv->input_dev);
	}
	nds03_debuf_printf("depth:%d mm, confi:%d, count:%d\r\n", pNxDevice->ranging_data[0].depth, pNxDevice->ranging_data[0].confi, pNxDevice->ranging_data[0].count);
}

static void nds03_gpio_work(struct work_struct *work)
{
	nds03_gpio_int_get_data();
	if(nds03prv->is_work == true)
		gpio_irq_enable(nds03prv->int_pin_num);
}

static void nds03_gpio_irq(uint32_t param)
{
	if (nds03prv->work.func != NULL) {
		gpio_irq_disable(nds03prv->int_pin_num);
		queue_work(nds03prv->nds03_wq, &nds03prv->work);
	}
}

static int nds03_gpio_irq_work(void)
{
	int ret = 0;
	gpio_configure(nds03prv->int_pin_num, GPIO_DIR_INPUT | GPIO_IRQ_FALLING);
	ret = gpio_irq_request(nds03prv->int_pin_num, nds03_gpio_irq, 0);
	if (ret < 0) {
		log_e("%s %d error\n", __func__, __LINE__);
		return NDS03_ERROR_API;
	}
	return NDS03_ERROR_NONE;
}

static int nds03_dev_open(struct input_dev *dev)
{
	NDS03_Dev_t *pNxDevice = nds03prv->pNxDevice;
	nds03dev_lock_async();

	if(nds03prv->init_work == NDS03_INIT_WORK_CAN_SLEEP && (nds03prv->cref == 0)) {
		if (NDS03_ERROR_NONE != NDS03_WaitDeviceBootUp(pNxDevice)) {
			log_e("NDS03_WaitDeviceBootUp error\r\n");
			return 0;
		}

		d_nds03_device_init_config();
	}
	nds03prv->cref++;
	nds03dev_unlock_async();

	return 0;
}

static void nds03_dev_close(struct input_dev *dev)
{
	--nds03prv->cref;
	if (nds03prv->init_work == NDS03_INIT_WORK_CAN_SLEEP && (nds03prv->cref == 0)) {
		d_nds03_device_stop_config();
	}
}

#define NDS03_IOCTL_SET_MODE 1
#define NDS03_IOCTL_SET_CALIBRATION_DISTANCE 2

static int nds03_dev_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	u32 val = (u32)arg;
	int ret = 0;
	static int mode = 0, calibration = 0, enter_io_flag = 0;
	switch (cmd){
	case NDS03_SET_WORK_MODE:
		enter_io_flag |= NDS03_IOCTL_SET_MODE;
		mode = val;
		break;
	case NDS03_SET_CALIBRATION_DISTANCE:
		enter_io_flag |= NDS03_IOCTL_SET_CALIBRATION_DISTANCE;
		calibration = val;
		break;
	case NDS03_START:
		d_nds03_device_stop_config();
		if(enter_io_flag) {
			if(enter_io_flag & NDS03_IOCTL_SET_MODE) {
				nds03prv->mode = mode;
			}

			if(enter_io_flag & NDS03_IOCTL_SET_CALIBRATION_DISTANCE) {
				nds03prv->calibration_distance = calibration;
			}

			enter_io_flag = 0;
		}
		ret = d_nds03_device_init_config();
		if(ret != NDS03_ERROR_NONE)
			return -EINVAL;
		break;
	case NDS03_STOP:
		d_nds03_device_stop_config();
		break;
	default:
		log_e("%s %d ioctl error%d\n", __func__, __LINE__, cmd);
		return -EINVAL;
	}
}

static int nds03_probe(void)
{
	int np;
	int ret = -1;
	u32 tmp = 0;
	NDS03_Dev_t *pNxDevice = &d_nds03_device;

	np = fdt_node_probe_by_path("/hcrtos/nds03@5c");
	if (np < 0) {
		log_e("Not found /hcrtos/nds03@5c\n");
		return 0;
	}

	nds03prv = kzalloc(sizeof(struct nds03_data), GFP_KERNEL);
	if (!nds03prv) {
		log_e("kzalloc error\n");
		return 0;
	}
	memset(nds03prv, 0, sizeof(struct nds03_data));

	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &nds03prv->i2c_devpath)) {
		log_e("i2c-devpath error\n");
		ret = -ENOMEM;
		goto nds03_free;
	}

	tmp = PINPAD_INVALID;
	fdt_get_property_u_32_index(np, "xshut-pin", 0, (u32 *)&tmp);
	nds03prv->xshut_pin_num = tmp;

	tmp = XSHUT_PIN_POLARITY_HIGH;
	fdt_get_property_u_32_index(np, "xshut-pin-polarity", 0, (u32 *)&tmp);
	pNxDevice->platform.xshut_pin = tmp;

	tmp = NDS03_DEFAULT_SLAVE_ADDR;
	fdt_get_property_u_32_index(np, "reg", 0, (u32 *)&tmp);
	pNxDevice->platform.i2c_dev_addr = tmp;

	tmp = 1;
	fdt_get_property_u_32_index(np, "target-num", 0, (u32 *)&tmp);
	nds03prv->target_num = tmp;

	tmp = 0;
	fdt_get_property_u_32_index(np, "mode", 0, (u32 *)&tmp);
	nds03prv->mode = tmp;

	tmp = PINPAD_INVALID;
	fdt_get_property_u_32_index(np, "int-pin", 0, (u32 *)&tmp);
	nds03prv->int_pin_num = tmp;

	tmp = 500;
	fdt_get_property_u_32_index(np, "calibration-distance", 0, (u32 *)&tmp);
	nds03prv->calibration_distance = tmp;

	if (nds03prv->int_pin_num != PINPAD_INVALID) {
		ret = nds03_gpio_irq_work();
		if (ret != NDS03_ERROR_NONE) {
			ret = -ENOMEM;
			goto nds03_free;
		}
		gpio_irq_disable(nds03prv->int_pin_num);

		nds03prv->nds03_wq = create_singlethread_workqueue("nds03_wq");
		if (!nds03prv->nds03_wq) {
			log_e("create error\n");
			ret = -ENOMEM;
			goto nds03_free;
		}

		INIT_WORK(&nds03prv->work, nds03_gpio_work);
	}

	tmp = NDS03_INIT_WORK_NORMAL;
	fdt_get_property_u_32_index(np, "init-work", 0, (u32 *)&tmp);
	nds03prv->init_work = tmp;
	nds03prv->pNxDevice = pNxDevice;

	log_d("%s %d init work = %d\n", __func__, __LINE__, nds03prv->init_work);

	if(nds03prv->target_num > 4)
		nds03prv->target_num = 4;

	if (nds03prv->target_num < 1) {
		log_e("target_num < 1\n");
		ret = -ENOMEM;
		goto nds03_free;
	}
	if(NDS03_ERROR_NONE != nds03_platform_init(&pNxDevice->platform, NULL))
	{
		log_e("nds03_platform_init error\n");
		ret = -ENOMEM;
		goto nds03_free;
	}

	if (NDS03_ERROR_NONE != NDS03_WaitDeviceBootUp(pNxDevice)) {
		log_e("NDS03_WaitDeviceBootUp error\n");
		ret = -ENOMEM;
		goto nds03_free;
	}
	/* 初始化模组设备 */
	if (NDS03_ERROR_NONE != NDS03_InitDevice(pNxDevice)) {
		log_e("NDS03_InitDevice error\n");
		ret = -ENOMEM;
		goto nds03_free;
	}
	printf("nds03 fw_version %lx\n", pNxDevice->chip_info.fw_version);

	nds03prv->input_dev = input_allocate_device();
	if (!nds03prv->input_dev) {
		ret = -ENOMEM;
		log_e("d_nds03_device: Failed to allocate input device\n");
		goto nds03_free;
	}

	nds03prv->cref = 0;
	mutex_init(&nds03prv->sf_lock);

	nds03prv->input_dev->name = "nds03_dsensor";
	nds03prv->input_dev->open = nds03_dev_open;
	nds03prv->input_dev->close = nds03_dev_close;
	nds03prv->input_dev->ioctl = nds03_dev_ioctl;

	set_bit(EV_REL, nds03prv->input_dev->evbit);

	/* x-axis acceleration */
	input_set_abs_params(nds03prv->input_dev, ABS_X, 0, 65535, 0, 0);
	/* y-axis acceleration */
	input_set_abs_params(nds03prv->input_dev, ABS_Y, 0, 65535, 0, 0);
	/* z-axis acceleration */
	input_set_abs_params(nds03prv->input_dev, ABS_Z, 0, 65535, 0, 0);

	input_set_abs_params(nds03prv->input_dev, REL_RESERVED, 0, 4, 0, 0);

	ret = input_register_device(nds03prv->input_dev);
	if (ret < 0) {
		log_e("d_nds03_device: Unable to register input device.\n");
		goto nds03_free;
	}

	if (nds03prv->init_work == NDS03_INIT_WORK_NORMAL) {
		d_nds03_device_init_config();
	}

	return 0;

nds03_free:
	free(nds03prv);
	return ret;
}

static int nds03_init(void)
{
	int ret = 0;

	ret = nds03_probe();

	return ret;
}


module_driver(nds03_driver, nds03_init, NULL, 2)
