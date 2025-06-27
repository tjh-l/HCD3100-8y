#include "bl_ts.h"
#include "bl_test.h"

#define SLEEP_TIME(x)	usleep(x * 1000)

struct btl_ts_data *g_btl_ts = NULL;
#if defined(BTL_DEBUG_SUPPORT)
int LogEn = 1;
#endif
#if defined(BTL_FACTORY_TEST_EN)
char fwname[FILE_NAME_LENGTH] = { 0 };
struct timeval rawdata_begin_time;
int rawdata_tested_flag = -1;
int btl_log_level = 0;
#endif

int btl_i2c_write_read(struct btl_ts_data *ts, unsigned char addr,
		       unsigned char *writebuf, int writelen,
		       unsigned char *readbuf, int readlen)
{
	int fd = 0;
	int ret = 0;
	int retry = 3;

	fd = open(ts->i2c_devpath, O_RDWR);

	while (retry--) {
		if (readlen > 0) {
			if (writelen > 0) {
				struct i2c_transfer_s xfer;
				struct i2c_msg_s msgs[] = {
					{
						.addr = (uint8_t)ts->addr,
						.flags = 0,
						.length = writelen,
						.buffer = writebuf,

					},
					{
						.addr = (uint8_t)ts->addr,
						.flags = I2C_M_RD,
						.length = readlen,
						.buffer = readbuf,

					},
				};

				xfer.msgv = msgs;
				xfer.msgc = 2;
				ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
				if (ret < 0) {
					BTL_DEBUG("error.retry = %d", retry);
					continue;
				}

			} else {
				struct i2c_transfer_s xfer;
				struct i2c_msg_s msgs[] = {
					{
						.addr = (uint8_t)ts->addr,
						.flags = I2C_M_RD,
						.length = readlen,
						.buffer = readbuf,

					},
				};

				xfer.msgv = msgs;
				xfer.msgc = 1;
				ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
				if (ret < 0) {
					BTL_DEBUG("error.retry = %d", retry);
					continue;
				}
			}
		}
		break;
	}

	close(fd);

	return ret;
}

int btl_i2c_write(struct btl_ts_data *ts, unsigned char addr,
		  unsigned char *writebuf, int writelen)
{
	int fd = 0;
	int ret = 0;
	int retry = 3;
	struct i2c_transfer_s xfer;
	struct i2c_msg_s msgs[] = {
		{
			.addr = (uint8_t)ts->addr,
			.flags = 0,
			.length = writelen,
			.buffer = writebuf,
		},
	};

	xfer.msgv = msgs;
	xfer.msgc = 1;

	fd = open(ts->i2c_devpath, O_RDWR);

	while (retry--) {
		if (writelen > 0) {
			ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
			if (ret < 0) {
				BTL_DEBUG("error.retry = %d\n", retry);
				continue;
			}
		}
		break;
	}

	close(fd);

	return ret;
}

int btl_i2c_read(struct btl_ts_data *ts, unsigned char addr,
		 unsigned char *readbuf, int readlen)
{
	int fd = 0;
	int ret = 0;
	int retry = 3;

	struct i2c_transfer_s xfer;
	struct i2c_msg_s msgs[] = {
		{
			.addr = (uint8_t)ts->addr,
			.flags = I2C_M_RD,
			.length = readlen,
			.buffer = readbuf,
		},
	};

	xfer.msgv = msgs;
	xfer.msgc = 1;

	fd = open(ts->i2c_devpath, O_RDWR);

	while (retry--) {
		ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
		if (ret < 0) {
			BTL_DEBUG("error.retry = %d\n", retry);
			continue;
		}
		break;
	}

	close(fd);

	return ret;
}

static int btl_i2c_transfer(unsigned char i2c_addr, unsigned char *buf, int len,
			    unsigned char rw)
{
	int ret;
	switch (rw) {
	case I2C_WRITE:
		ret = btl_i2c_write(g_btl_ts, i2c_addr, buf, len);
		break;
	case I2C_READ:
		ret = btl_i2c_read(g_btl_ts, i2c_addr, buf, len);
		break;
	}
	if (ret < 0) {
		BTL_DEBUG("btl_i2c_transfer:i2c transfer error___\n");
		return -1;
	}

	return 0;
}

void btl_i2c_lock(void)
{
	mutex_lock(&g_btl_ts->i2c_lock);
}

void btl_i2c_unlock(void)
{
	mutex_unlock(&g_btl_ts->i2c_lock);
}

#if defined(BTL_VIRTRUAL_KEY_SUPPORT)
void btl_ts_report_virtrual_key_data(struct btl_ts_data *ts, s32 x, s32 y)
{
	int keyCode = KEY_CNT;
	unsigned char i;
	BTL_DEBUG_FUNC();
	BTL_DEBUG("x=%d;y=%d\n", x, y);
	for (i = 0; i < BTL_VIRTRUAL_KEY_NUM; i++) {
		if ((x == ts->btl_key_data[i].x) ||
		    (y == ts->btl_key_data[i].y)) {
			keyCode = ts->btl_key_data[i].code;
		}
		BTL_DEBUG("xKey=%d;yKey=%d;key=%x;keyCode=%x\n",
			  ts->btl_key_data[i].x, ts->btl_key_data[i].y,
			  ts->btl_key_data[i].code, keyCode);
	}
	input_report_key(ts->input_dev, keyCode, 1);
	input_sync(ts->input_dev);
	input_report_key(ts->input_dev, keyCode, 0);
	input_sync(ts->input_dev);
}

static int btl_ts_virtrual_key_init(struct btl_ts_data *ts)
{
	int ret = 0;
	int i = 0;
	int data[BTL_VIRTRUAL_KEY_NUM][3] = BTL_VIRTRUAL_KEY_DATA;
	for (i = 0; i < BTL_VIRTRUAL_KEY_NUM; i++) {
		ts->btl_key_data[i].code = data[i][0];
		ts->btl_key_data[i].x = data[i][1];
		ts->btl_key_data[i].y = data[i][2];
	}
	return ret;
}
#endif

#if defined(RESET_PIN_WAKEUP)
void btl_ts_reset_wakeup(void)
{
	struct btl_ts_data *ts = g_btl_ts;
	BTL_GPIO_OUTPUT(ts->reset_gpio_number, 1);
	ts->reset_gpio_level = 1;
	SLEEP_TIME(20);
	BTL_GPIO_OUTPUT(ts->reset_gpio_number, 0);
	ts->reset_gpio_level = 0;
	SLEEP_TIME(20);
	BTL_GPIO_OUTPUT(ts->reset_gpio_number, 1);
	ts->reset_gpio_level = 1;
	SLEEP_TIME(20);
}
#endif

void btl_irq_disable(struct btl_ts_data *ts)
{
	unsigned long irqflags;

	BTL_DEBUG_FUNC();

	spin_lock_irqsave(&ts->irq_lock, irqflags);
	if (!ts->irq_is_disable) {
		ts->irq_is_disable = 1;
		gpio_irq_disable(ts->irq_gpio_number);
	}
	spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

void btl_irq_enable(struct btl_ts_data *ts)
{
	unsigned long irqflags = 0;

	BTL_DEBUG_FUNC();

	spin_lock_irqsave(&ts->irq_lock, irqflags);
	if (ts->irq_is_disable) {
		gpio_irq_enable(ts->irq_gpio_number);
		ts->irq_is_disable = 0;
	}
	spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

static int btl_request_platform_resource(struct btl_ts_data *ts)
{
	return 0;
}

static int btl_request_input_dev(struct btl_ts_data *ts)
{
	int ret = -1;

	BTL_DEBUG_FUNC();

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		BTL_ERROR("Failed to allocate input device.");
		return -ENOMEM;
	}

	ts->input_dev->name = "blt1680_tp";
	ts->input_dev->evbit[0] =
		BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	__set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
#if defined(BTL_VIRTRUAL_KEY_SUPPORT) || defined(BTL_SYSFS_VIRTRUAL_KEY_SUPPORT)
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_HOMEPAGE, ts->input_dev->keybit);
	set_bit(KEY_BACK, ts->input_dev->keybit);
#endif
#if defined(BTL_CTP_SUPPORT_TYPEB_PROTOCOL)
	input_mt_init_slots(ts->input_dev, MAX_POINT_NUM, INPUT_MT_DIRECT);
#endif
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->TP_MAX_X, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->TP_MAX_Y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, MAX_POINT_NUM, 0, 0);
#if defined(BTL_CTP_PRESSURE)
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
#endif
//	ts->input_dev->name = btl_ts_name;
//	ts->input_dev->phys = btl_input_phys;
//	ts->input_dev->id.bustype = BUS_I2C;
//	ts->input_dev->id.vendor = 0xDEAD;
//	ts->input_dev->id.product = 0xBEEF;
//	ts->input_dev->id.version = 10427;

	ret = input_register_device(ts->input_dev);
	if (ret) {
		BTL_ERROR("Register blt1680 input device failed");
		input_free_device(ts->input_dev);
		return -ENODEV;
	}
	return 0;
}

#if defined(BTL_TOUCHPAD_SUPPORT)
static int btl_request_touchpad_dev(struct btl_ts_data *ts)
{
	int ret = -1;

	BTL_DEBUG_FUNC();

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		BTL_ERROR("Failed to allocate input device.");
		return -ENOMEM;
	}

	ts->input_dev->name = "blt1680_touchpad_tp";
	ts->input_dev->evbit[0] =
		BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	__set_bit(BTN_LEFT, ts->input_dev->keybit);
	__set_bit(BTN_MIDDLE, ts->input_dev->keybit);
	__set_bit(BTN_RIGHT, ts->input_dev->keybit);
	__set_bit(REL_X, ts->input_dev->relbit);
	__set_bit(REL_Y, ts->input_dev->relbit);
	__set_bit(REL_Z, ts->input_dev->relbit);
	__set_bit(REL_WHEEL, ts->input_dev->relbit);
	__set_bit(REL_HWHEEL, ts->input_dev->relbit);
	__set_bit(INPUT_PROP_POINTER, ts->input_dev->propbit);
	__set_bit(INPUT_PROP_POINTING_STICK, ts->input_dev->propbit);

//	ts->input_dev->name = btl_ts_name;
//	ts->input_dev->phys = btl_input_phys;
//	ts->input_dev->id.bustype = BUS_I2C;
//	ts->input_dev->id.vendor = 0xDEAD;
//	ts->input_dev->id.product = 0xBEEF;
//	ts->input_dev->id.version = 10427;

	ret = input_register_device(ts->input_dev);
	if (ret) {
		BTL_ERROR("Register %s input device failed",
			  ts->input_dev->name);
		input_free_device(ts->input_dev);
		return -ENODEV;
	}
	return 0;
}
#endif

static void btl_touch_down(struct btl_ts_data *ts, s32 id, s32 x, s32 y, s32 w,
			   s8 pressure)
{
	BTL_DEBUG("X_origin:%d, Y_origin:%d", x, y);

#if defined(BTL_CTP_SUPPORT_TYPEB_PROTOCOL)
	input_mt_slot(ts->input_dev, id);
	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
	input_report_key(ts->input_dev, BTN_TOUCH, 1);
#else
	input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
	input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
	input_report_key(ts->input_dev, BTN_TOUCH, 1);
	input_mt_sync(ts->input_dev);
#endif
	BTL_DEBUG("ID:%d, X:%d, Y:%d, W:%d\n", id, x, y, w);
}

static void btl_touch_up(struct btl_ts_data *ts, s32 id, s32 x, s32 y)
{
#if defined(BTL_CTP_SUPPORT_TYPEB_PROTOCOL)
	input_mt_slot(ts->input_dev, id);
	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
#endif
	BTL_DEBUG("ID:%d\n", id);
}

static void btl_release_all_points(struct btl_ts_data *ts)
{
#if defined(BTL_CTP_SUPPORT_TYPEB_PROTOCOL)
	unsigned char i;
#endif

#if defined(BTL_TOUCHPAD_SUPPORT)
	return;
#endif

#if defined(BTL_CTP_SUPPORT_TYPEB_PROTOCOL)
	for (i = 0; i < MAX_POINT_NUM; i++) {
		btl_touch_up(ts, i, 0, 0);
	}
#endif
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
#ifndef BTL_CTP_SUPPORT_TYPEB_PROTOCOL
	input_mt_sync(ts->input_dev);
#endif

	input_sync(ts->input_dev);
}

static void btl_ts_update_data(struct btl_ts_data *ts)
{
	BTL_DEBUG_FUNC();
	u8 cmd = TS_DATA_REG;
	u8 point_data[2 + 6 * MAX_POINT_NUM] = { 0 };
	u8 touch_num = 0;
	u8 pressure = 0x0;
	s32 input_x = 0;
	s32 input_y = 0;
	s32 id = 0;
	u8 eventFlag = 0;
	s32 i = 0;
	s32 ret = 0;
#if defined(BTL_PROXIMITY_SUPPORT)
	u8 proxValue = BTL_CTP_PROXIMITY_LEAVE;
	u8 proxReg = BTL_CTP_PROXIMITY_FLAG_REG;
#endif
	BTL_DEBUG_FUNC();
#if defined(BTL_ESD_PROTECT_SUPPORT)
	if (ts->esd_need_block == 0) {
		ts->esd_need_block = 1;
	}
#endif
#if defined(BTL_CHARGE_PROTECT_SUPPORT)
	if (ts->charge_need_block == 0) {
		ts->charge_need_block = 1;
	}
#endif

	if (ts->enter_update) {
		goto exit_work_func;
	}

	ret = btl_i2c_write_read(ts, CTP_SLAVE_ADDR, &cmd, 1,
				 point_data, 2 + 6 * MAX_POINT_NUM);
	if (ret < 0) {
		BTL_ERROR("I2C transfer error. errno:%d\n ", ret);
		goto exit_work_func;
	}
#if defined(BTL_GESTURE_SUPPORT)
#endif

#if defined(BTL_PROXIMITY_SUPPORT)
	if (ts->proximity_enable) {
		btl_i2c_write_read(ts, CTP_SLAVE_ADDR, &proxReg, 1,
				   &proxValue, 1);
		if ((proxValue == BTL_CTP_PROXIMITY_NEAR) &&
		    (ts->proximity_state != proxValue)) {
			input_report_abs(ts->ps_input_dev, ABS_DISTANCE, 0);
			input_mt_sync(ts->ps_input_dev);
			input_sync(ts->ps_input_dev);
			ts->proximity_state = proxValue;
			goto exit_work_func;
		} else if ((proxValue == BTL_CTP_PROXIMITY_LEAVE) &&
			   (ts->proximity_state != proxValue)) {
			input_report_abs(ts->ps_input_dev, ABS_DISTANCE, 1);
			input_mt_sync(ts->ps_input_dev);
			input_sync(ts->ps_input_dev);
			ts->proximity_state = proxValue;
		}
	}
#endif

	touch_num = point_data[1] & 0x0f;
	if ((touch_num > MAX_POINT_NUM) || (touch_num == 0)) {
		goto exit_work_func;
	}

#if defined(BTL_CTP_PRESSURE)
	pressure = point_data[7];
#endif

#if defined(BTL_VIRTRUAL_KEY_SUPPORT)
	input_x = ((point_data[2] & 0x0f) << 8) | point_data[3];
	input_y = ((point_data[4] & 0x0f) << 8) | point_data[5];
	if (input_y > ts->TP_MAX_Y) {
		btl_ts_report_virtrual_key_data(ts, input_x, input_y);
		goto exit_work_func;
	}
#endif

	for (i = 0; i < MAX_POINT_NUM; i++) {
		eventFlag = point_data[2 + 6 * i] >> 6;
		id = point_data[4 + 6 * i] >> 4;
		input_x = ((point_data[2 + 6 * i] & 0x0f) << 8) |
			  point_data[3 + 6 * i];
		input_y = ((point_data[4 + 6 * i] & 0x0f) << 8) |
			  point_data[5 + 6 * i];
		BTL_DEBUG("eventFlag:%d, id:%d, x:%d, y:%d", eventFlag, id,
			  input_x, input_y);
		if ((id != 0x0f) &&
		    ((eventFlag == CTP_MOVE) || (eventFlag == CTP_DOWN))) {
			btl_touch_down(ts, id, input_x, input_y, 1, pressure);
		} else if (eventFlag == CTP_UP) {
			btl_touch_up(ts, id, input_x, input_y);
		}
	}

	input_sync(ts->input_dev);

	return;

exit_work_func:
#if defined(BTL_ESD_PROTECT_SUPPORT)
	if (ts->esd_need_block == 1) {
		ts->esd_need_block = 0;
	}
#endif
#if defined(BTL_CHARGE_PROTECT_SUPPORT)
	if (ts->charge_need_block == 1) {
		ts->charge_need_block = 0;
	}
#endif
	btl_release_all_points(ts);
	return;
}

static void ts_work(void *param)
{
	struct btl_ts_data *ts = (struct btl_ts_data *)param;

#ifdef BTL_THREADED_IRQ
	btl_i2c_lock();
	btl_ts_update_data(ts);
	btl_i2c_unlock();
#endif
	return;
}

static void ts_irq(uint32_t param)
{
	struct btl_ts_data *ts = (struct btl_ts_data *)param;

	if (work_available(&ts->work)) {
		work_queue(HPWORK, &ts->work, ts_work, (void *)ts, 0);
	}
}

static int btl_request_irq(struct btl_ts_data *ts)
{
	int ret = -1;

	BTL_DEBUG_FUNC();
	BTL_GPIO_AS_INT(ts->irq_gpio_number);
	ts->irq_gpio_dir = 1;
	ts->int_trigger_type = 0;
	BTL_DEBUG("INT trigger type:%x", ts->int_trigger_type);
#ifdef BTL_THREADED_IRQ
	gpio_configure(ts->irq_gpio_number, GPIO_DIR_INPUT | GPIO_IRQ_FALLING);
	ret = gpio_irq_request(ts->irq_gpio_number, ts_irq, (uint32_t)ts);
	if (ret < 0) {
		printf("ERROR: gpio_irq_request() failed: %d\n", ret);
	}
#endif
	if (ret) {
		BTL_ERROR("Request IRQ failed!ERRNO:%d.", ret);
		BTL_GPIO_AS_INPUT(ts->irq_gpio_number);
		ts->irq_gpio_dir = 1;
//		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
//		ts->timer.function = btl_ts_timer_handler;
//		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
		return -1;
	} else {
		btl_irq_disable(ts);
		ts->use_irq = 1;
		return 0;
	}
}

#if defined(BTL_CONFIG_OF)
static int btl_parse_dt(struct btl_ts_data *ts)
{
	int np = 0;
#if defined(BTL_SYSFS_VIRTRUAL_KEY_SUPPORT)
	int i = 0;
	u32 data[12] = { 0 };
#endif
	u32 tmp_val_u32;

	BTL_DEBUG_FUNC();
	np = fdt_node_probe_by_path("/hcrtos/betterlife_ts@2c");
	if (np < 0) {
		printf("Not find betterlife_ts in dts\n");
		goto fail;
	}

#if defined(RESET_PIN_WAKEUP)
	if (fdt_get_property_u_32_index(np, "reset_gpio", 0,
					(u32 *)&ts->reset_gpio_number)) {
		BTL_ERROR("fail to get reset_gpio_number\n");
		goto fail;
	}
#endif

	if (fdt_get_property_u_32_index(np, "irq_gpio", 0,
					(u32 *)&ts->irq_gpio_number)) {
		BTL_ERROR("fail to get irq_gpio_number\n");
		goto fail;
	}

	/* get i2c addr and i2c devpath */
	if (fdt_get_property_u_32_index(np, "i2c_addr", 0, (u32 *)&ts->addr)) {
		BTL_ERROR("fail to get i2c-addr\n");
		goto fail;
	}
	BTL_DEBUG("BTL I2C Address: 0x%02x", ts->addr);
	if (fdt_get_property_string_index(np, "i2c_devpath", 0, &ts->i2c_devpath)) {
		BTL_ERROR("fail to get i2c-devpath\n");
		goto fail;
	}

#if defined(BTL_VCC_LDO_SUPPORT)
	if (fdt_get_property_string_index(np, "vcc_name", 0, &ts->vcc_name)) {
		BTL_ERROR("fail to get ldo for vcc_name\n");
		goto fail;
	}
#endif

#if defined(BTL_CUSTOM_VCC_LDO_SUPPORT)
	if (fdt_get_property_u_32_index(np, "vcc_gpio", 0,
					(u32 *)&ts->vcc_gpio_number)) {
		BTL_ERROR("fail to get vcc_gpio_number\n");
		goto fail;
	}
#endif

#if defined(BTL_IOVCC_LDO_SUPPORT)
	if (fdt_get_property_string_index(np, "iovcc_name", 0, &ts->iovcc_name)) {
		BTL_ERROR("fail to get ldo for iovcc_name\n");
		goto fail;
	}
#endif
#if defined(BTL_CUSTOM_IOVCC_LDO_SUPPORT)
	if (fdt_get_property_u_32_index(np, "iovcc_gpio", 0,
					(u32 *)&ts->iovcc_gpio_number)) {
		BTL_ERROR("fail to get iovcc_gpio_number\n");
		goto fail;
	}
#endif

#if defined(BTL_SYSFS_VIRTRUAL_KEY_SUPPORT)
	if (fdt_get_property_u_32_array(np, "virtualkeys", (u32 *)data, 12)) {
		BTL_ERROR("fail to get virtualkeys\n");
		goto fail;
	}
	for (i = 0; i < 12; i++) {
		g_btl_ts->virtualkeys[i] = data[i];
	}
#endif

	if (fdt_get_property_u_32_index(np, "TP_MAX_X", 0,
					(u32 *)&tmp_val_u32)) {
		BTL_ERROR("fail to get TP_MAX_X\n");
		goto fail;
	}
	g_btl_ts->TP_MAX_X = tmp_val_u32;
	if (fdt_get_property_u_32_index(np, "TP_MAX_Y", 0,
					(u32 *)&tmp_val_u32)) {
		BTL_ERROR("fail to get TP_MAX_Y\n");
		goto fail;
	}
	g_btl_ts->TP_MAX_Y = tmp_val_u32;

	return 0;
fail:
	return -1;
}
#endif

static int btl_get_basic_chip_info(struct btl_ts_data *ts)
{
	int ret = 0;
#if (CTP_TYPE == SELF_CTP)
	unsigned char cmdChannel = BTL_CHANNEL_RX_REG;
#endif
#if (CTP_TYPE == SELF_INTERACTIVE_CTP)
	unsigned char cmdChannel = BTL_CHANNEL_RX_REG;
#endif
#if (CTP_TYPE == COMPATIBLE_CTP)
	return 0;
#endif
	BTL_DEBUG_FUNC();
	btl_i2c_lock();
	ret = btl_i2c_write_read(ts, CTP_SLAVE_ADDR, &cmdChannel, 1,
				 &ts->chipInfo.rxChannel, 1);
	if (ret < 0) {
		goto err;
	}
#if (CTP_TYPE == SELF_CTP)
	cmdChannel = BTL_CHANNEL_KEY_REG;
	ret = btl_i2c_write_read(ts, CTP_SLAVE_ADDR, &cmdChannel, 1,
				 &ts->chipInfo.keyChannel, 1);
	if (ret < 0) {
		goto err;
	}
#endif
#if (CTP_TYPE == SELF_INTERACTIVE_CTP)
	cmdChannel = BTL_CHANNEL_TX_REG;
	ret = btl_i2c_write_read(ts, CTP_SLAVE_ADDR, &cmdChannel, 1,
				 &ts->chipInfo.txChannel, 1);
	if (ret < 0) {
		goto err;
	}
#endif

err:
	btl_i2c_unlock();
#if (CTP_TYPE == SELF_CTP)
	BTL_DEBUG("rx_channel:%d,key_channel=%d\n", ts->chipInfo.rxChannel,
		  ts->chipInfo.keyChannel);
#endif
#if (CTP_TYPE == SELF_INTERACTIVE_CTP)
	BTL_DEBUG("tx_channel:%d,rx_channel=%d\n", ts->chipInfo.txChannel,
		  ts->chipInfo.rxChannel);
#endif

	return ret;
}

static int hc_blt1680_ts_probe(void)
{
	s32 ret = -1;
	struct btl_ts_data *ts;
	BTL_DEBUG_FUNC();
	BTL_DEBUG("BTL Driver Version: %s", BTL_DRIVER_VERSION);

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		BTL_ERROR("Alloc GFP_KERNEL memory failed.");
		return -ENOMEM;
	}
	g_btl_ts = ts;
#if defined(BTL_CONFIG_OF)
	ret = btl_parse_dt(ts);
	if (ret < 0) {
		BTL_ERROR("BTL parse dt failed!\n");
		goto ERROR_PARSE_DT;
	}
#else
	ts->reset_gpio_number = BTL_RST_PORT;
	ts->irq_gpio_number = BTL_INT_PORT;
	ts->TP_MAX_X = TPD_RES_X;
	ts->TP_MAX_Y = TPD_RES_Y;
#if defined(BTL_VCC_LDO_SUPPORT)
	ts->vcc_name = "vdd28";
#endif
#if defined(BTL_CUSTOM_VCC_LDO_SUPPORT)
	ts->vcc_gpio_number = BTL_VCC_PORT;
#endif
#if defined(BTL_IOVCC_LDO_SUPPORT)
	ts->iovcc_name = "vio18";
#endif
#if defined(BTL_CUSTOM_IOVCC_LDO_SUPPORT)
	ts->iovcc_gpio_number = BTL_IOVCC_PORT;
#endif
#endif
	BTL_DEBUG("TP_MAX_X = %d TP_MAX_Y= %d rst = %d int = %d\n",
		  ts->TP_MAX_X, ts->TP_MAX_Y, ts->reset_gpio_number,
		  ts->irq_gpio_number);

	spin_lock_init(&ts->irq_lock);
	spin_lock_init(&ts->poll_lock);
	mutex_init(&ts->i2c_lock);

	ret = btl_request_platform_resource(ts);
	if (ret < 0) {
		BTL_ERROR("BTL request platform_resource failed.");
		goto ERR_REQUEST_PLT_RC;
	}

#if defined(BTL_POWER_CONTROL_SUPPORT)
#endif

#if defined(RESET_PIN_WAKEUP)
	btl_ts_reset_wakeup();
#endif

#if defined(BTL_CHECK_CHIPID)
	ret = btl_get_chip_id(&ts->chipInfo.chipID);
	if (ret < 0) {
		BTL_ERROR("I2C communication ERROR!");

		goto ERR_CHECK_ID;
	} else if (ts->chipInfo.chipID != BTL_FLASH_ID) {
		BTL_ERROR("Please specify the IC model:0x%x!",
			  ts->chipInfo.chipID);
		ret = -1;
		goto ERR_CHECK_ID;
	} else {
		BTL_DEBUG("I2C communication success:chipID = %x!",
			  ts->chipInfo.chipID);
	}
#endif

#if defined(BTL_AUTO_UPDATE_FARMWARE)
	btl_i2c_lock();
	ret = btl_auto_update_fw();
	btl_i2c_unlock();
#endif

#if defined(BTL_SYSFS_VIRTRUAL_KEY_SUPPORT) || defined(BTL_VIRTRUAL_KEY_SUPPORT)
	ret = btl_ts_virtrual_key_init(ts);
	if (ret < 0) {
		BTL_ERROR("BTL request input dev failed");
		goto ERR_KEY_INIT;
	}
#endif

#if defined(BTL_TOUCHPAD_SUPPORT)
	ret = btl_request_touchpad_dev(ts);
	if (ret < 0) {
		BTL_ERROR("BTL request input dev failed");
		goto ERR_REGISTER_INPUT;
	}
#else
	ret = btl_request_input_dev(ts);
	if (ret < 0) {
		BTL_ERROR("BTL request input dev failed");
		goto ERR_REGISTER_INPUT;
	}
#endif

#if defined(BTL_PROXIMITY_SUPPORT)
#endif

#if defined(BTL_GESTURE_SUPPORT)
#endif

#if defined(BTL_FACTORY_TEST_EN)
	ret = btl_test_init();
	if (ret) {
		BTL_DEBUG("btl_test_init fail");
	}
#endif

#if defined(BTL_ESD_PROTECT_SUPPORT)
#endif

#if defined(BTL_CHARGE_PROTECT_SUPPORT)
#endif

#if defined(BTL_WAIT_QUEUE)
#endif

	ret = btl_request_irq(ts);
	if (ret < 0) {
		BTL_DEBUG("BTL works in polling mode.");
	} else {
		BTL_DEBUG("BTL works in interrupt mode.");
	}

	if (ts->use_irq) {
		btl_irq_enable(ts);
	}

#if defined(BTL_ESD_PROTECT_SUPPORT)
#endif

#if defined(BTL_CHARGE_PROTECT_SUPPORT)
#endif

#if defined(BTL_SUSPEND_MODE)
#endif

	ret = btl_get_basic_chip_info(ts);
	if (ret < 0) {
		BTL_ERROR("get basic chip information error!");
	}

#if defined(BTL_DEBUGFS_SUPPORT)
#endif

#if defined(BTL_APK_SUPPORT)
#endif

	return 0;

ERR_REGISTER_INPUT:
#if defined(BTL_SYSFS_VIRTRUAL_KEY_SUPPORT)
#endif

#if defined(BTL_SYSFS_VIRTRUAL_KEY_SUPPORT) || defined(BTL_VIRTRUAL_KEY_SUPPORT)
ERR_KEY_INIT:
#endif

#if defined(BTL_CHECK_CHIPID)
ERR_CHECK_ID:
#endif

ERR_REQUEST_PLT_RC:

#ifdef BTL_CONFIG_OF
ERROR_PARSE_DT:
#endif
	g_btl_ts = NULL;
	kfree(ts);

	return ret;
}

static int hc_blt1680_ts_init(void)
{
	int ret = 0;

	ret = hc_blt1680_ts_probe();

	return ret;
}

module_driver(hc_blt1680_driver, hc_blt1680_ts_init, NULL, 1)
