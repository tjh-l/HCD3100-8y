#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/jiffies.h>
#include <nuttx/fs/fs.h>

#include <errno.h>
#include <kernel/drivers/spi.h>
#include <nuttx/fs/fs.h>
#include <hcuapi/spidev.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <kernel/lib/fdt_api.h>

#include <hcuapi/gpio.h>

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <hcuapi/i2c-master.h>
#include <freertos/semphr.h>

#include "sis_i2c.h"

/* Addresses to scan */
static struct workqueue_struct *sis_wq;
struct sis_ts_data *ts_bak;/* must 0 */
struct sisTP_driver_data *TPInfo;/* must NULL */
static void sis_tpinfo_clear(struct sisTP_driver_data *TPInfo, int max);

void PrintBuffer(int start, int length, char *buf)
{
	int i;
	for (i = start; i < length; i++) {
		pr_info("%02x ", buf[i]);
		if (i != 0 && i % 30 == 0)
			pr_info("\n");
	}
	pr_info("\n");
}

static int sis_command_for_read(struct sis_ts_data *ts, int rlength,
				unsigned char *rdata)
{
	int fd = 0;
	int ret = 0;
	int retry = 3;

	struct i2c_transfer_s xfer;
	struct i2c_msg_s msgs[] = {
		{
			.addr = (uint8_t)ts->addr,
			.flags = I2C_M_RD,
			.length = rlength,
			.buffer = rdata,
		},
	};

	xfer.msgv = msgs;
	xfer.msgc = 1;

	fd = open(ts->i2c_devpath, O_RDWR);

	while (retry--) {
		ret = ioctl(fd, I2CIOC_TRANSFER, &xfer);
		if (ret < 0) {
			pr_err("error.retry = %d\n", retry);
			continue;
		}
		break;
	}

	close(fd);

	return ret;
}

static int sis_cul_unit(uint8_t report_id)
{
	int ret = NORMAL_LEN_PER_POINT;

	if (report_id != ALL_IN_ONE_PACKAGE) {
		if (IS_AREA(report_id) /*&& IS_TOUCH(report_id)*/)
			ret += AREA_LEN_PER_POINT;
		if (IS_PRESSURE(report_id))
			ret += PRESSURE_LEN_PER_POINT;
	}

	return ret;
}

static int sis_ReadPacket(struct sis_ts_data *ts, uint8_t cmd, uint8_t *buf)
{
	uint8_t tmpbuf[MAX_BYTE] = {0};	/*MAX_BYTE = 64;*/
#ifdef _CHECK_CRC
	uint16_t buf_crc = 0;
	uint16_t package_crc = 0;
	int l_package_crc = 0;
	int crc_end = 0;
#endif
	int ret = SIS_ERR;
	int touchnum = 0;
	int p_count = 0;
	int touc_formate_id = 0;
	int locate = 0;
	bool read_first = true;
	/*
	* New i2c format
	* buf[0] = Low 8 bits of byte count value
	* buf[1] = High 8 bits of byte counte value
	* buf[2] = Report ID
	* buf[touch num * 6 + 2 ] = Touch informations;
	* 1 touch point has 6 bytes, it could be none if no touch
	* buf[touch num * 6 + 3] = Touch numbers
	*
	* One touch point information include 6 bytes, the order is
	*
	* 1. status = touch down or touch up
	* 2. id = finger id
	* 3. x axis low 8 bits
	* 4. x axis high 8 bits
	* 5. y axis low 8 bits
	* 6. y axis high 8 bits
	* */
	do {
		if (locate >= PACKET_BUFFER_SIZE) {
			pr_err("sis_ReadPacket: Buf Overflow\n");
			return SIS_ERR;
		}
		ret = sis_command_for_read(ts, MAX_BYTE, tmpbuf);

#ifdef _DEBUG_PACKAGE
		pr_info("sis_ReadPacket: Buf_Data [0~63]\n");
		PrintBuffer(0, 0x3d, tmpbuf);
#endif

		if (ret < 0) {
			pr_err("sis_ReadPacket: i2c transfer error\n");
			return SIS_ERR_TRANSMIT_I2C;
		}
		/*error package length of receiving data*/
		else if (tmpbuf[P_BYTECOUNT] > MAX_BYTE) {
#ifdef _DEBUG_REPORT
			pr_err("sis_ReadPacket: Error Bytecount\n");
			PrintBuffer(0, 64, tmpbuf);
#endif
			return SIS_ERR;
		}
		if (read_first) {
			/* access NO TOUCH event unless BUTTON NO TOUCH event*/
			if (tmpbuf[P_BYTECOUNT] == 0/*NO_TOUCH_BYTECOUNT*/)
				return touchnum;/*touchnum is 0*/
			if (tmpbuf[P_BYTECOUNT] == 3/*EMPRY PACKET*/) {
#ifdef _DEBUG_REPORT
				pr_err("sis_ReadPacket: Empty packet");
				PrintBuffer(0, 64, tmpbuf);
#endif
				return SIS_ERR_EMPTY_PACKET;
			}
		}
		/*skip parsing data when two devices are registered
		 * at the same slave address*/
		/*parsing data when P_REPORT_ID && 0xf is TOUCH_FORMAT
		 * or P_REPORT_ID is ALL_IN_ONE_PACKAGE*/
		touc_formate_id = tmpbuf[P_REPORT_ID] & 0xf;

		if ((touc_formate_id != HIDI2C_FORMAT)
#ifndef CONFIG_TOUCHSCREEN_SIS_I2C_95XX
		&& (touc_formate_id != TOUCH_FORMAT)
#endif
		&& (touc_formate_id != BUTTON_FORMAT)
		&& (tmpbuf[P_REPORT_ID] != ALL_IN_ONE_PACKAGE)) {
			pr_err("sis_ReadPacket: Error Report_ID 0x%x\n",
				tmpbuf[P_REPORT_ID]);
			return SIS_ERR;
		}
		p_count = (int) tmpbuf[P_BYTECOUNT] - 1;	/*start from 0*/
		if (tmpbuf[P_REPORT_ID] != ALL_IN_ONE_PACKAGE) {
			if (touc_formate_id == HIDI2C_FORMAT) {
				p_count -= BYTE_CRC_HIDI2C;
#ifndef CONFIG_TOUCHSCREEN_SIS_I2C_95XX
			} else if (touc_formate_id == TOUCH_FORMAT) {
				p_count -= BYTE_CRC_I2C;/*delete 2 byte crc*/
#endif
			} else if (touc_formate_id == BUTTON_FORMAT) {
				p_count -= BYTE_CRC_BTN;
			} else {	/*should not be happen*/
				pr_err("sis_ReadPacket: delete crc error\n");
				return SIS_ERR;
			}
			if (IS_SCANTIME(tmpbuf[P_REPORT_ID])) {
				p_count -= BYTE_SCANTIME;
#ifdef CONFIG_TOUCHSCREEN_SIS_I2C_95XX
			} else {
				pr_err("sis_ReadPacket: Error Report_ID 0x%x, no scan time\n",
					tmpbuf[P_REPORT_ID]);
				return SIS_ERR;	
#endif
			}
		}
		/*else {}*/ /*For ALL_IN_ONE_PACKAGE*/
		if (read_first)
			touchnum = tmpbuf[p_count];
		else {
			if (tmpbuf[p_count] != 0) {
				pr_err("sis_ReadPacket: get error package\n");
				return SIS_ERR;
			}
		}

#ifdef _CHECK_CRC
		crc_end = p_count + (IS_SCANTIME(tmpbuf[P_REPORT_ID]) * 2);
		buf_crc = cal_crc(tmpbuf, 2, crc_end);
		/*sub bytecount (2 byte)*/
		l_package_crc = p_count + 1
		+ (IS_SCANTIME(tmpbuf[P_REPORT_ID]) * 2);
		package_crc = ((tmpbuf[l_package_crc] & 0xff)
		| ((tmpbuf[l_package_crc + 1] & 0xff) << 8));

		if (buf_crc != package_crc)	{
			pr_err("sis_ReadPacket: CRC Error\n");
			return SIS_ERR;
		}
#endif
		memcpy(&buf[locate], &tmpbuf[0], 64);
		/*Buf_Data [0~63] [64~128]*/
		locate += 64;
		read_first = false;
	} while (tmpbuf[P_REPORT_ID] != ALL_IN_ONE_PACKAGE &&
			tmpbuf[p_count] > 5);
	return touchnum;
}

void ts_report_key(struct sis_ts_data *ts, uint8_t keybit_state)
{
	int i = 0;
	/*check keybit_state is difference with pre_keybit_state*/
	uint8_t diff_keybit_state = 0x0;
	/*button location for binary*/
	uint8_t key_value = 0x0;
	/*button is up or down*/
	uint8_t  key_pressed = 0x0;

	if (!ts) {
		pr_err("%s error: Missing Platform Data!\n", __func__);
		return;
	}

	diff_keybit_state = TPInfo->pre_keybit_state ^ keybit_state;

	if (diff_keybit_state) {
		for (i = 0; i < BUTTON_KEY_COUNT; i++) {
			if ((diff_keybit_state >> i) & 0x01) {
				key_value = diff_keybit_state & (0x01 << i);
				key_pressed = (keybit_state >> i) & 0x01;
				switch (key_value) {
				case MSK_COMP:
				input_report_key
				(ts->input_dev, KEY_COMPOSE, key_pressed);
				pr_err("%s : MSK_COMP %d\n"
					, __func__ , key_pressed);
					break;
				case MSK_BACK:
				input_report_key
					(ts->input_dev, KEY_BACK, key_pressed);
				pr_err("%s : MSK_BACK %d\n"
					, __func__ , key_pressed);
					break;
				case MSK_MENU:
				input_report_key
					(ts->input_dev, KEY_MENU, key_pressed);
				pr_err("%s : MSK_MENU %d\n"
					, __func__ , key_pressed);
					break;
				case MSK_HOME:
				input_report_key
					(ts->input_dev, KEY_HOME, key_pressed);
				pr_err("%s : MSK_HOME %d\n"
					, __func__ , key_pressed);
					break;
				case MSK_NOBTN:

				default:
					break;
				}
			}
		}
		TPInfo->pre_keybit_state = keybit_state;
	}

	
}

static void sis_ts_work_func(struct work_struct *work)
{
	struct sis_ts_data *ts = container_of(work, struct sis_ts_data, work);
	int ret = SIS_ERR;
	int point_unit;
	uint8_t buf[PACKET_BUFFER_SIZE] = {0};
	uint8_t i = 0, fingers = 0;
	uint8_t px = 0, py = 0, pstatus = 0;
	uint8_t p_area = 0;
	uint8_t p_preasure = 0;
	int button_key;
	int p_button;

	bool all_touch_up = true;


	mutex_lock(&ts->mutex_wq);
	/* I2C or SMBUS block data read */
	ret = sis_ReadPacket(ts, SIS_CMD_NORMAL, buf);
#ifdef _DEBUG_PACKAGE_WORKFUNC
	pr_info("sis_ts_work_func: Buf_Data [0~63]\n");
	PrintBuffer(0, 64, buf);
	if ((buf[P_REPORT_ID] != ALL_IN_ONE_PACKAGE) && (ret > 5)) {
		pr_info("sis_ts_work_func: Buf_Data [64~125]\n");
		PrintBuffer(64, 128, buf);
	}
#endif

	/*Error Number*/
	if (ret < 0) {
		if (ret == -1)
			pr_info("sis_ts_work_func: ret = -1\n");
		goto err_free_allocate;
	}
	/*New button format*/
	else if ((buf[P_REPORT_ID] & 0xf) == BUTTON_FORMAT) {
		p_button = ((int) buf[P_BYTECOUNT] - 1 ) - BYTE_CRC_BTN + 1;
		button_key = ((buf[p_button] & 0xff)
		| ((buf[p_button + 1] & 0xff) << 8));
		ts_report_key(ts, button_key);
	}

	/* access NO TOUCH event unless BUTTON NO TOUCH event */
	else if (ret == 0) {
		fingers = 0;
		sis_tpinfo_clear(TPInfo, MAX_FINGERS);
		goto label_send_report;
	}
	sis_tpinfo_clear(TPInfo, MAX_FINGERS);

	/*Parser and Get the sis9200 data*/
	point_unit = sis_cul_unit(buf[P_REPORT_ID]);
	fingers = ret;

	TPInfo->fingers = fingers = (fingers > MAX_FINGERS ? 0 : fingers);

	/*fingers 10 =  0 ~ 9*/
	for (i = 0; i < fingers; i++) {
		if ((buf[P_REPORT_ID] != ALL_IN_ONE_PACKAGE) && (i >= 5)) {
			/*Calc point status*/
			pstatus = BYTE_BYTECOUNT + BYTE_ReportID
					+ ((i - 5) * point_unit);
			pstatus += 64;
		} else {
			pstatus = BYTE_BYTECOUNT + BYTE_ReportID
					+ (i * point_unit);
					/*Calc point status*/
		}
	    px = pstatus + 2;	/*Calc point x_coord*/
	    py = px + 2;	/*Calc point y_coord*/
		if ((buf[pstatus]) == TOUCHUP) {
			TPInfo->pt[i].Width = 0;
			TPInfo->pt[i].Height = 0;
			TPInfo->pt[i].Pressure = 0;
		} else if (buf[P_REPORT_ID] == ALL_IN_ONE_PACKAGE
					&& (buf[pstatus] & TOUCHDOWN) ) {
			TPInfo->pt[i].Width = 1;
			TPInfo->pt[i].Height = 1;
			TPInfo->pt[i].Pressure = 1;
		} else if ( buf[pstatus] & TOUCHDOWN ) {
#ifdef CONFIG_TOUCHSCREEN_SIS_I2C_95XX
			p_area = py + 3;
			p_preasure = py + 2;
			/*area*/
			if (IS_AREA(buf[P_REPORT_ID])) {
				TPInfo->pt[i].Width = (buf[p_area] & 0xff) 
				| ((buf[p_area+1] & 0xff) << 8);
				TPInfo->pt[i].Height = (buf[p_area+2] & 0xff) 
				| ((buf[p_area+3] & 0xff) << 8);
#else
			p_area = py + 2;
			p_preasure = py + 4;
			/*area*/
			if (IS_AREA(buf[P_REPORT_ID])) {
				TPInfo->pt[i].Width = buf[p_area];
				TPInfo->pt[i].Height = buf[p_area + 1];
#endif
			} else {
				TPInfo->pt[i].Width = 1;
				TPInfo->pt[i].Height = 1;
			}

			/*preasure*/
			if (IS_PRESSURE(buf[P_REPORT_ID]))
				TPInfo->pt[i].Pressure = (buf[p_preasure]);
			else
				TPInfo->pt[i].Pressure = 1;

		} else {
			pr_err("sis_ts_work_func: Error Touch Status\n");
			goto err_free_allocate;
		}
		TPInfo->pt[i].id = (buf[pstatus + 1]);
		TPInfo->pt[i].x = ((buf[px] & 0xff)
		| ((buf[px + 1] & 0xff) << 8));
		TPInfo->pt[i].y = ((buf[py] & 0xff)
		| ((buf[py + 1] & 0xff) << 8));
	}
#ifdef _DEBUG_REPORT
	for (i = 0; i < TPInfo->fingers; i++) {
		pr_info("sis_ts_work_func: i = %d, id = %d, x = %d, y = %d"
		", pstatus = %d, width = %d, height = %d, pressure = %d\n"
		, i, TPInfo->pt[i].id, TPInfo->pt[i].x
		, TPInfo->pt[i].y , buf[pstatus], TPInfo->pt[i].Width
		, TPInfo->pt[i].Height, TPInfo->pt[i].Pressure);
	}
#endif

label_send_report:
/* Report co-ordinates to the multi-touch stack */

	for (i = 0; ((i < TPInfo->fingers) && (i < MAX_FINGERS)); i++) {
		if (TPInfo->pt[i].Pressure) {
			input_report_key(ts->input_dev, BTN_TOUCH, 1);
//			TPInfo->pt[i].Width *= AREA_UNIT;
//			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
//					 TPInfo->pt[i].Width);
//			TPInfo->pt[i].Height *= AREA_UNIT;
//			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR,
//					 TPInfo->pt[i].Height);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE,
					 TPInfo->pt[i].Pressure);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
					 TPInfo->pt[i].x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
					 TPInfo->pt[i].y);
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID,
					 TPInfo->pt[i].id);
			input_mt_sync(ts->input_dev);
			all_touch_up = false;
		}

		if (i == (TPInfo->fingers - 1) && all_touch_up == true) {
			input_report_key(ts->input_dev, BTN_TOUCH, 0);
			input_mt_sync(ts->input_dev);
		}
	}

	if (TPInfo->fingers == 0) {
		input_report_key(ts->input_dev, BTN_TOUCH, 0);
		input_mt_sync(ts->input_dev);
	}

	input_sync(ts->input_dev);

err_free_allocate:
	if (ts->use_irq) {
		gpio_irq_enable(ts->irq);
	}

	mutex_unlock(&ts->mutex_wq);
	return;
}

static void sis_tpinfo_clear(struct sisTP_driver_data *TPInfo, int max)
{
	int i = 0;

	for (i = 0; i < max; i++) {
		TPInfo->pt[i].id = -1;
		TPInfo->pt[i].x = 0;
		TPInfo->pt[i].y = 0;
		TPInfo->pt[i].Pressure = 0;
		TPInfo->pt[i].Width = 0;
	}
	TPInfo->id = 0x0;
	TPInfo->fingers = 0;
}

static void sis_ts_irq_handler(uint32_t param)
{
	struct sis_ts_data *ts = (struct sis_ts_data *)param;

	gpio_irq_disable(ts->irq);
	if(ts_bak->is_cmd_mode == MODE_IS_TOUCH) {
		queue_work(sis_wq, &ts->work);
	}
	else {
#ifdef _DEBUG_REPORT
		pr_err("sis drop the first packet\n");
#endif		
		ts_bak->is_cmd_mode = MODE_IS_TOUCH;
		gpio_irq_enable(ts->irq);
	}
	
	return ;
}

uint16_t cal_crc(char *cmd, int start, int end)
{
	int i = 0;
	uint16_t crc = 0;
	for (i = start; i <= end ; i++)
		crc = (crc<<8) ^ crc16tab[((crc>>8) ^ cmd[i])&0x00FF];
	return crc;
}

uint16_t cal_crc_with_cmd(char *data, int start, int end, uint8_t cmd)
{
	int i = 0;
	uint16_t crc = 0;

	crc = (crc<<8) ^ crc16tab[((crc>>8) ^ cmd)&0x00FF];
	for (i = start; i <= end ; i++)
		crc = (crc<<8) ^ crc16tab[((crc>>8) ^ data[i])&0x00FF];
	return crc;
}

void write_crc(unsigned char *buf, int start, int end)
{
	uint16_t crc = 0;
	crc = cal_crc(buf, start , end);
	buf[end+1] = (crc >> 8) & 0xff;
	buf[end+2] = crc & 0xff;
}

static int hc_sis9509_tp_probe(void)
{
	int np;
	int ret;
	const char *status;
	struct sis_ts_data *ts = NULL;

	np = fdt_get_node_offset_by_path("/hcrtos/sis_touchscreen@5c");
	if (np < 0) {
		printf("sis9509 touchscreen no find node in dts\n");
		return -ENODEV;
	}

	fdt_get_property_string_index(np, "status", 0, &status);

	if (strcmp(status, "disabled") == 0)
		return -ENODEV;

	pr_info("sis_ts_probe\n");
	TPInfo = kzalloc(sizeof(struct sisTP_driver_data), GFP_KERNEL);
	if (TPInfo == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	ts = kzalloc(sizeof(struct sis_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	ts_bak = ts;

	if (fdt_get_property_u_32_index(np, "touch-gpio", 0, (u32 *)&ts->irq))
		return -ENODEV;
	if (fdt_get_property_u_32_index(np, "reg", 0, (u32 *)&ts->addr))
		return -ENODEV;
	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &ts->i2c_devpath))
		return -ENODEV;

	mutex_init(&ts->mutex_wq);
	/*1. Init Work queue and necessary buffers*/
	sis_wq = create_singlethread_workqueue("sis_wq");
	if (!sis_wq)
		return -ENOMEM;

	INIT_WORK(&ts->work, sis_ts_work_func);

	/*2. Allocate input device*/
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		pr_err("sis_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	/*This input device name should be the same to IDC file name.*/
	/*"SiS9200-i2c-touchscreen"*/
	ts->input_dev->name = "sis9509_tp";

	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(EV_SYN, ts->input_dev->evbit); 
	set_bit(ABS_MT_POSITION_X, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts->input_dev->absbit);
	set_bit(ABS_MT_TRACKING_ID, ts->input_dev->absbit);

	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
	set_bit(ABS_MT_PRESSURE, ts->input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MINOR, ts->input_dev->absbit);


	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE,
						0, PRESSURE_MAX, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR,
						0, AREA_LENGTH_LONGER, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MINOR,
						0, AREA_LENGTH_SHORT, 0, 0);

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
						0, SIS_MAX_X, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
						0, SIS_MAX_Y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID,
						0, 15, 0, 0);

	/* add for touch keys */
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(KEY_COMPOSE, ts->input_dev->keybit);
	set_bit(KEY_BACK, ts->input_dev->keybit);
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_HOME, ts->input_dev->keybit);

	ts->is_cmd_mode = MODE_IS_TOUCH;

	/*3. Register input device to core*/
	ret = input_register_device(ts->input_dev);
	if (ret) {
		pr_err("sis_ts_probe: Unable to register %s input device\n",
				ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	/*4. irq or timer setup*/
#ifdef _I2C_INT_ENABLE
	gpio_configure(ts->irq, GPIO_DIR_INPUT | GPIO_IRQ_FALLING);
	ret = gpio_irq_request(ts->irq, sis_ts_irq_handler, (uint32_t)ts);
	if (ret == 0)
		ts->use_irq = 1;
	else
		dev_err(&client->dev, "request_irq failed\n");
#endif

	pr_info("sis_ts_probe: Start touchscreen %s in %s mode\n",
			ts->input_dev->name,
			ts->use_irq ? "interrupt" : "polling");

	pr_info("sis SIS_SLAVE_ADDR: %d\n", SIS_SLAVE_ADDR);

	return 0;
err_input_register_device_failed:
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
err_power_failed:
	kfree(ts);
err_alloc_data_failed:
	return ret;
}

static int hc_sis9509_tp_init(void)
{
	int ret = 0;

	ret = hc_sis9509_tp_probe();

	return ret;
}

module_driver(hc_sis9509_driver, hc_sis9509_tp_init, NULL, 1)
