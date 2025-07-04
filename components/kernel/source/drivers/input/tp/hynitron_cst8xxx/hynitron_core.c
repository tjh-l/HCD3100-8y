#include <kernel/module.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <linux/jiffies.h>
#include <nuttx/fs/fs.h>
#include <errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <kernel/lib/fdt_api.h>
#include <hcuapi/gpio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <hcuapi/i2c-master.h>
#include <freertos/semphr.h>
#include <linux/kthread.h>

#include "hynitron_core.h"
#include "hynitron_update_firmware.h"

#include <sys/unistd.h>
#include <unistd.h>

static void cst3xx_touch_report(void);
static int cst8xx_touch_report(void);

#ifndef CONFIG_GTP_USE_PINCTRL
//#define CONFIG_GTP_USE_PINCTRL
#endif
static DECLARE_WAIT_QUEUE_HEAD(waiter);
static struct task_struct *thread = NULL;
static int tpd_load_status = 0;
int tpd_flag = 0;
struct hynitron_ts_data *hyn_ts_data = NULL;
extern struct hynitron_chip_type_array hynitron_chip_type_grp[50];
extern struct hynitron_fw_array hynitron_fw_grp[20];

#define HYN_DRIVER_NAME                     "hyn_ts"

#include "hynitron_update_firmware.h"

void hyn_reset_watchdog_proc(void)
{
#if HYN_RESET_SOFTWARE
	unsigned char buf[4];
	buf[0] = 0xD1;
	buf[1] = 0x0e;
	if (hyn_ts_data->chip_ic_main_addr != hyn_ts_data->client->addr) {
		hyn_ts_data->client->addr = hyn_ts_data->chip_ic_main_addr;
	}
	cst3xx_i2c_write(hyn_ts_data, buf, 2);
#endif
}

/*****************************************************************************
*  Name: hyn_reset_proc
*  Brief: Execute reset operation
*  Input: hdelayms - delay time unit:ms
*  Output:
*  Return: 0 - Get correct Device ID
*****************************************************************************/
int hyn_reset_proc(int hdelayms)
{
	HYN_FUNC_ENTER();
#ifdef CONFIG_GTP_USE_PINCTRL
	tpd_gpio_output(GTP_RST_PORT, 0);
	mdelay(20);
	hyn_reset_watchdog_proc();
	tpd_gpio_output(GTP_RST_PORT, 1);
#else
	gpio_set_output(hyn_ts_data->pdata->reset_gpio, 0);
	mdelay(20);
	hyn_reset_watchdog_proc();
	gpio_set_output(hyn_ts_data->pdata->reset_gpio, 1);
#endif
	if (hdelayms) {
		mdelay(hdelayms);
	}
	return 0;
}

/*****************************************************************************
*  Name: hyn_irq_disable
*  Brief: disable irq
*  Input:
*   sync: 1 - call disable_irq(); Can't used in interrup handler
*         0 - call disable_irq_nosync(); Usually used in interrup handler
*  Output:
*  Return:
*****************************************************************************/
void hyn_irq_disable(struct hynitron_ts_data *ts_data)
{
	// unsigned long irqflags;
	if (!hyn_ts_data->irq_is_disable) {
		//spin_lock_irqsave(&hyn_ts_data->irq_lock, irqflags);
	//	disable_irq(hyn_ts_data->use_irq);
		gpio_irq_disable(ts_data->pdata->irq_gpio);
		hyn_ts_data->irq_is_disable = 1;
		//spin_unlock_irqrestore(&hyn_ts_data->irq_lock, irqflags);
	}
}

/*****************************************************************************
*  Name: hyn_irq_enable
*  Brief: enable irq
*  Input:
*  Output:
*  Return:
*****************************************************************************/
void hyn_irq_enable(struct hynitron_ts_data *ts_data)
{
	//unsigned long irqflags = 0;

	if (hyn_ts_data->irq_is_disable) {
		//spin_lock_irqsave(&hyn_ts_data->irq_lock, irqflags);
	//	enable_irq(hyn_ts_data->use_irq);
		gpio_irq_enable(ts_data->pdata->irq_gpio);
		hyn_ts_data->irq_is_disable = 0;
		//spin_unlock_irqrestore(&hyn_ts_data->irq_lock, irqflags);
	}
}

/*****************************************************************************
*  Reprot related
*****************************************************************************/
/*****************************************************************************
*  Name: hyn_release_all_finger
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void hyn_release_all_finger(void)
{
#if HYN_MT_PROTOCOL_B_EN
	unsigned int finger_count = 0;
#endif

	HYN_FUNC_ENTER();

#if (!HYN_MT_PROTOCOL_B_EN)
	input_report_key(hyn_ts_data->input_dev, BTN_TOUCH, 0);
	input_mt_sync(hyn_ts_data->input_dev);
#else
	for (finger_count = 0; finger_count < hyn_ts_data->pdata->max_touch_num; finger_count++) {
		input_mt_slot(hyn_ts_data->input_dev, finger_count);
		input_report_abs(hyn_ts_data->input_dev, ABS_MT_TRACKING_ID, -1);
		input_mt_report_slot_state(hyn_ts_data->input_dev, MT_TOOL_FINGER, false);
	}
#endif
	input_sync(hyn_ts_data->input_dev);

	HYN_FUNC_EXIT();
}

#if (HYN_DEBUG_EN && (HYN_DEBUG_LEVEL == 2))
char g_sz_debug[1024] = { 0 };

#define HYN_ONE_TCH_LEN HYN_TOUCH_STEP
static void hyn_show_touch_buffer(u8 *buf, int point_num)
{
	int len = point_num * HYN_ONE_TCH_LEN;
	int count = 0;
	int i;

	memset(g_sz_debug, 0, 1024);

	if (hyn_ts_data->config_chip_product_line == HYN_CHIP_PRODUCT_LINE_MUT_CAP) {
		//mutcap
		len = 7 * point_num;
	} else if (hyn_ts_data->config_chip_product_line == HYN_CHIP_PRODUCT_LINE_SEL_CAP) {
		//selfcap

		if (len > (POINT_READ_BUF - 3)) {
			len = POINT_READ_BUF - 3;
		} else if (len == 0) {
			len += HYN_ONE_TCH_LEN;
		}
	}
	count += sprintf(g_sz_debug, "%02X,%02X,%02X", buf[0], buf[1], buf[2]);
	for (i = 0; i < len; i++) {
		count += sprintf(g_sz_debug + count, ",%02X", buf[i + 3]);
	}
	HYN_DEBUG("buffer: %s", g_sz_debug);
}
#endif

#if (!HYN_MT_PROTOCOL_B_EN)
static void tpd_down(int x, int y, int p, int id)
{
	if ((id < 0) || (id > 9))
		return;

	input_report_key(hyn_ts_data->input_dev, BTN_TOUCH, 1);

#if (HYN_REPORT_PRESSURE_EN)
	p = 0x3f;
	input_report_abs(hyn_ts_data->input_dev, ABS_MT_PRESSURE, p);
#endif

	input_report_abs(hyn_ts_data->input_dev, ABS_MT_TRACKING_ID, id);
	input_report_abs(hyn_ts_data->input_dev, ABS_MT_TOUCH_MAJOR, 1);
	input_report_abs(hyn_ts_data->input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(hyn_ts_data->input_dev, ABS_MT_POSITION_Y, y);
	input_mt_sync(hyn_ts_data->input_dev);

#if HYN_REPORT_PRESSURE_EN
	HYN_DEBUG("[A]P%d(%4d,%4d)[p:%d] DOWN!", id, x, y, p);
#else
	HYN_DEBUG("[A]P%d(%4d,%4d) DOWN!", id, x, y);
	printf("[A]P%d(%4d,%4d) DOWN!\n", id, x, y);
#endif
}

static void tpd_up(int x, int y)
{
	HYN_DEBUG("[A]All Up!");
	input_report_key(hyn_ts_data->input_dev, BTN_TOUCH, 0);
	input_mt_sync(hyn_ts_data->input_dev);
}

static int tpd_touchinfo(struct touch_info *cinfo, struct touch_info *pinfo)
{
	u8 i = 0;
	int ret;
	u8 data[POINT_READ_BUF] = { 0 };
	u16 high_byte, low_byte;
	//char writebuf[10]= { 0 };

	HYN_DEBUG("HYN read tpd_touchinfo.");

	for (i = 0; i < 15; i += 5) {
		ret = hyn_i2c_read(hyn_ts_data, &i, 1, (u8 *)data + i, 5);
		if (ret < 0) {
			HYN_ERROR("[B]Read touchdata failed, ret: %d", ret);
			HYN_FUNC_EXIT();
			return ret;
		}
	}

	if ((data[0] & 0x70) != 0)
		return false;

	memcpy(pinfo, cinfo, sizeof(struct touch_info));
	memset(cinfo, 0, sizeof(struct touch_info));
	for (i = 0; i < hyn_ts_data->pdata->max_touch_num; i++)
		cinfo->p[i] = 1; /* Put up */

	/*get the number of the touch points*/
	cinfo->count = data[2] & 0x0f;
	HYN_DEBUG("Number of touch points = %d", cinfo->count);

#if (HYN_DEBUG_EN && (HYN_DEBUG_LEVEL == 2))
	hyn_show_touch_buffer(data, cinfo->count);
#endif
	for (i = 0; i < cinfo->count; i++) {
		cinfo->p[i] = (data[3 + 6 * i] >> 6) & 0x0003; /* event flag */
		cinfo->id[i] = data[3 + 6 * i + 2] >> 4; // touch id

		/*get the X coordinate, 2 bytes*/
		high_byte = data[3 + 6 * i];
		high_byte <<= 8;
		high_byte &= 0x0F00;

		low_byte = data[3 + 6 * i + 1];
		low_byte &= 0x00FF;
		cinfo->x[i] = high_byte | low_byte;

		/*get the Y coordinate, 2 bytes*/
		high_byte = data[3 + 6 * i + 2];
		high_byte <<= 8;
		high_byte &= 0x0F00;

		low_byte = data[3 + 6 * i + 3];
		low_byte &= 0x00FF;
		cinfo->y[i] = high_byte | low_byte;

		HYN_DEBUG(" cinfo->x[%d] = %d, cinfo->y[%d] = %d, cinfo->p[%d] = %d", i, cinfo->x[i], i, cinfo->y[i], i,
			  cinfo->p[i]);
	}

	return true;
};
#else
/************************************************************************
* Name: hyn_read_touchdata
* Brief: report the point information
* Input: event info
* Output: get touch data in pinfo
* Return: success is zero
***********************************************************************/
static int hyn_read_touchdata(struct ts_event *data)
{
	u8 buf[POINT_READ_BUF + 2] = { 0 };
	u8 temp_buf[8] = { 0 };
	int ret = -1;
	u8 i = 0;
	u8 pointid = 0;

	HYN_FUNC_ENTER();

	memset((u8 *)temp_buf, 0, 8);
	memset((u8 *)buf, 0, POINT_READ_BUF + 2);

	for (i = 0; i < 15; i += 5) {
		ret = hyn_i2c_read(hyn_ts_data, &i, 1, (u8 *)temp_buf, 5);
		if (ret < 0) {
			HYN_ERROR("[B]Read touchdata failed, ret: %d", ret);
			HYN_FUNC_EXIT();
			return ret;
		} else {
			memcpy((u8 *)buf + i, (u8 *)temp_buf, 5);
		}
	}

	/*
    ret = hyn_i2c_read(hyn_ts_data, buf, 1, buf, POINT_READ_BUF);
    if (ret < 0)
    {
        HYN_ERROR("[B]Read touchdata failed, ret: %d", ret);
        HYN_FUNC_EXIT();
        return ret;
    }
    */
	memset(data, 0, sizeof(struct ts_event));
	data->touch_point_num = buf[HYN_TOUCH_POINT_NUM] & 0x0F;
	data->touch_point = 0;
	if (data->touch_point_num > 2) {
		data->touch_point_num = 2;
	}

#if (HYN_DEBUG_EN && (HYN_DEBUG_LEVEL == 2))
	hyn_show_touch_buffer(buf, data->touch_point_num);
#endif

	for (i = 0; i < data->touch_point_num; i++) {
		pointid = (buf[HYN_TOUCH_ID_POS + HYN_TOUCH_STEP * i]) >> 4;
		if (pointid >= HYN_MAX_SELFCAP_ID)
			break;
		else
			data->touch_point++;

		data->au8_finger_id[i] = pointid;

		data->au16_x[i] = ((unsigned short)(buf[HYN_TOUCH_X_H_POS + HYN_TOUCH_STEP * i] & 0x0F) << 8) |
				  (unsigned short)buf[HYN_TOUCH_X_L_POS + HYN_TOUCH_STEP * i];
		data->au16_y[i] = ((unsigned short)(buf[HYN_TOUCH_Y_H_POS + HYN_TOUCH_STEP * i] & 0x0F) << 8) |
				  (unsigned short)buf[HYN_TOUCH_Y_L_POS + HYN_TOUCH_STEP * i];

		data->au8_touch_event[i] = buf[HYN_TOUCH_EVENT_POS + HYN_TOUCH_STEP * i] >> 6;

		data->pressure[i] = (buf[HYN_TOUCH_XY_POS + HYN_TOUCH_STEP * i]); //cannot constant value
		data->area[i] = (buf[HYN_TOUCH_MISC + HYN_TOUCH_STEP * i]) >> 4;
		if ((data->au8_touch_event[i] == 0 || data->au8_touch_event[i] == 2) && (data->touch_point_num == 0))
			break;
	}
	HYN_FUNC_EXIT();
	return 0;
}
/************************************************************************
* Name: hyn_report_key
* Brief: report key event
* Input: event info
* Output: no
* Return: 0: is key event, -1: isn't key event
***********************************************************************/
static int hyn_report_key(struct ts_event *data)
{
	int i = 0;

	if (1 != data->touch_point)
		return -1;

	for (i = 0; i < hyn_ts_data->pdata->max_touch_num; i++) {
		if (data->au16_y[i] <= hyn_ts_data->pdata->y_resolution) {
			return -1;
		}
	}
	if (data->au8_touch_event[i] == 0 || data->au8_touch_event[i] == 2) {
		// tpd_button(data->au16_x[0], data->au16_y[0], 1);
		HYN_DEBUG("[B]Key(%d, %d) DOWN!", data->au16_x[0], data->au16_y[0]);
	} else {
		//  tpd_button(data->au16_x[0], data->au16_y[0], 0);
		HYN_DEBUG("[B]Key(%d, %d) UP!", data->au16_x[0], data->au16_y[0]);
	}

	input_sync(hyn_ts_data->input_dev);

	return 0;
}

/************************************************************************
* Name: hyn_report_value
* Brief: report the point information
* Input: event info
* Output: no
* Return: success is zero
***********************************************************************/
static int hyn_report_value(struct ts_event *data)
{
	int i = 0;
	int up_point = 0;
	int touchs = 0;
	int finger_count;

	if (data->touch_point == 0) {
		for (finger_count = 0; finger_count < hyn_ts_data->pdata->max_touch_num; finger_count++) {
			input_mt_slot(hyn_ts_data->input_dev, finger_count);
			//input_report_abs(hyn_ts_data->input_dev, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(hyn_ts_data->input_dev, MT_TOOL_FINGER, false);
		}

		input_report_key(hyn_ts_data->input_dev, BTN_TOOL_FINGER, 0);
		input_sync(hyn_ts_data->input_dev);
		return 0;
	}

	for (i = 0; i < data->touch_point; i++) {
		if (data->au8_touch_event[i] == 0 || data->au8_touch_event[i] == 2) //touch down
		{
			input_report_key(hyn_ts_data->input_dev, BTN_TOUCH, 1);
			input_mt_slot(hyn_ts_data->input_dev, data->au8_finger_id[i]);
			input_mt_report_slot_state(hyn_ts_data->input_dev, MT_TOOL_FINGER, true);

			input_report_abs(hyn_ts_data->input_dev, ABS_MT_POSITION_X, data->au16_x[i]);
			input_report_abs(hyn_ts_data->input_dev, ABS_MT_POSITION_Y, data->au16_y[i]);
#if HYN_REPORT_PRESSURE_EN
			data->pressure[i] = 1;
			input_report_abs(hyn_ts_data->input_dev, ABS_MT_PRESSURE, data->pressure[i]);
#endif
			//input_report_abs(hyn_ts_data->input_dev, ABS_MT_TRACKING_ID,data->au8_finger_id[i]);
			input_report_key(hyn_ts_data->input_dev, BTN_TOOL_FINGER, 1);

			data->area[i] = (data->au16_y[i] & 0x07);

			//input_report_abs(hyn_ts_data->input_dev, ABS_MT_TOUCH_MAJOR, data->area[i]);

			touchs |= BIT(data->au8_finger_id[i]);
			data->touchs |= BIT(data->au8_finger_id[i]);

#if HYN_REPORT_PRESSURE_EN
			HYN_DEBUG("[B]P%d(%4d,%4d)[p:%d,tm:%d] DOWN!", data->au8_finger_id[i], data->au16_x[i],
				  data->au16_y[i], data->pressure[i], data->area[i]);
#else
			HYN_DEBUG("[B]P%d(%4d,%4d)[tm:%d] DOWN!", data->au8_finger_id[i], data->au16_x[i],
				  data->au16_y[i], data->area[i]);
#endif
		} else //touch up
		{
			up_point++;

			input_mt_slot(hyn_ts_data->input_dev, data->au8_finger_id[i]);
			input_mt_report_slot_state(hyn_ts_data->input_dev, MT_TOOL_FINGER, false);
#if HYN_REPORT_PRESSURE_EN
			input_report_abs(hyn_ts_data->input_dev, ABS_MT_PRESSURE, 0);
#endif
			data->touchs &= ~BIT(data->au8_finger_id[i]);
			HYN_DEBUG("[B]P%d UP!", data->au8_finger_id[i]);
		}
	}

	input_sync(hyn_ts_data->input_dev);
	return 0;
}
#endif

static int hyn_touch_handler(void *unused)
{
	/* need modify for kernel-4.14 */
	HYN_FUNC_ENTER();

	do {
		set_current_state(TASK_INTERRUPTIBLE);
		wait_event(waiter, tpd_flag != 0);
		if (hyn_ts_data->work_mode > 0) {
			continue;
		}
		tpd_flag = 0;

		set_current_state(TASK_RUNNING);

		if (hyn_ts_data->config_chip_product_line == HYN_CHIP_PRODUCT_LINE_MUT_CAP) {
			//mutcap
			if (hyn_ts_data->apk_rdwr_flag == 0)
				cst3xx_touch_report();
		} else if (hyn_ts_data->config_chip_product_line == HYN_CHIP_PRODUCT_LINE_SEL_CAP) {
			//selfcap
			cst8xx_touch_report();
		}
	} while (!kthread_should_stop());

	return 0;
}


/*****************************************************************************
*  Name: hyn_eint_interrupt_handler
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static void hyn_eint_interrupt_handler(uint32_t param)
{
	tpd_flag = 1;
	wake_up(&waiter);
	return;
}

/*****************************************************************************
*  Name: hyn_irq_registration
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int hyn_irq_registration(struct hynitron_ts_data *client)
{
	int ret = 0;
	u32 ints[2] = { 0, 0 };

	HYN_FUNC_ENTER();
//	tpd_gpio_as_int(GTP_INT_PORT);

	ret = of_property_read_u32_array(hyn_ts_data->np, "debounce", ints, ARRAY_SIZE(ints));
	if (!ret) {
		gpio_set_debounce(ints[0], ints[1]); //gpio_set_debounce(unsigned gpio, unsigned debounce)
	} else {
		HYN_ERROR("debounce not exist");
	}

#ifdef CONFIG_GTP_USE_PINCTRL
	hyn_ts_data->use_irq = irq_of_parse_and_map(node, 0);
#endif
	HYN_INFO("hyn_ts_data->use_irq%d.\n", hyn_ts_data->use_irq);

	if (hyn_ts_data->pdata->irq_gpio == PINPAD_INVALID) {
		HYN_ERROR("Unable to irq_of_parse_and_map().\n");
		return -1;
	} else {
		gpio_configure(hyn_ts_data->pdata->irq_gpio, GPIO_DIR_INPUT | GPIO_IRQ_FALLING);
		ret = gpio_irq_request(hyn_ts_data->pdata->irq_gpio, hyn_eint_interrupt_handler, (uint32_t)hyn_ts_data);
		if (ret == 0) {
			HYN_INFO("IRQ request succussfully, irq=%d ,trigger_rising:%d .", hyn_ts_data->use_irq,
					HYN_IRQ_TRIGGER_RISING_CONFIG);
			//	client->irq = hyn_ts_data->use_irq;
#if HYN_GESTURE_EN
			enable_irq_wake(hyn_ts_data->use_irq);
#endif
		} else {
			HYN_ERROR("request_irq IRQ LINE NOT AVAILABLE!.");
			return -1;
		}
	}

	HYN_FUNC_EXIT();
	return 0;
}

static void cst3xx_touch_down(struct input_dev *input_dev, s32 id, s32 x, s32 y, s32 w)
{
	s32 temp_w = (w >> 3);

#if HYN_MT_PROTOCOL_B_EN
	input_mt_slot(input_dev, id);
	input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, 1);
	input_report_abs(input_dev, ABS_MT_TRACKING_ID, id);
	input_report_abs(input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
	input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, temp_w);
	input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, temp_w);
#if HYN_REPORT_PRESSURE_EN
	input_report_abs(input_dev, ABS_MT_PRESSURE, temp_w);
#endif
#else
	input_report_key(input_dev, BTN_TOUCH, 1);
	input_report_abs(input_dev, ABS_MT_POSITION_X, x);
	input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
	input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, temp_w);
	input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, temp_w);
	input_report_abs(input_dev, ABS_MT_TRACKING_ID, id);
	input_mt_sync(input_dev);
#endif
}

static void cst3xx_touch_up(struct input_dev *input_dev, int id)
{
#if HYN_MT_PROTOCOL_B_EN
	input_mt_slot(input_dev, id);
	input_report_abs(input_dev, ABS_MT_TRACKING_ID, -1);
	input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, 0);
#else
	input_report_key(input_dev, BTN_TOUCH, 0);
	input_mt_sync(input_dev);
#endif
}

static void cst3xx_touch_report(void)
{
	unsigned char buf[70];
	unsigned char i2c_buf[8];
	unsigned char key_status, key_id = 0, finger_id, sw;
	unsigned int input_x = 0;
	unsigned int input_xy_temp = 0;
	unsigned int input_y = 0;
	unsigned int input_w = 0;
	unsigned char cnt_up = 0, cnt_down = 0;
	int i, ret, idx;
	int cnt, i2c_len;
#if HYN_IIC_TRANSFER_LIMIT
	int len_1, len_2;
#endif

#if HYN_ESDCHECK_EN
	if (hyn_ts_data->hyn_esd_i2c_lock_flag > 0) {
		HYN_ERROR("  hyn_esd_i2c_lock_flag is running .\n");
		goto END;
	}
#endif

#if HYN_EN_AUTO_UPDATE
	if (hyn_ts_data->fw_updating > 0) {
		HYN_ERROR("  fw_updating is running .\n");
		goto END;
	}
#endif

#if HYN_PSENSOR_EN
	hyn_proximity_readdata();
#endif

#if HYN_GESTURE_EN
	if (hyn_gesture_readdata() == 0)
		goto END;
#endif

#if 0
{

   	unsigned char value_buf[28];
	value_buf[0] = 0xD0;
	value_buf[1] = 0xFF;
	ret = cst3xx_i2c_read_register(hyn_ts_data, value_buf, 28);
	if(ret < 0) {
		HYN_ERROR("  iic read touch point data failed.\n");
	}

	hyn_save_log((unsigned char *)(value_buf));
}
#endif
	key_status = 0;
	buf[0] = 0xD0;
	buf[1] = 0x00;
	ret = cst3xx_i2c_read_register(hyn_ts_data, buf, 7);
	if (ret < 0) {
		HYN_ERROR("  iic read touch point data failed.\n");
		goto OUT_PROCESS;
	}

	if (buf[6] != 0xAB) {
		HYN_ERROR("  buf[6] != 0xAB data is not valid..\r\n");
		goto OUT_PROCESS;
	}
	if (buf[0] == 0xAB) {
		HYN_ERROR("  buf[0]=0xAB,data is not valid..\r\n");
		goto OUT_PROCESS;
	}

	if (buf[5] == 0x80) {
		key_status = buf[0];
		key_id = buf[1];
		goto KEY_PROCESS;
	}

	cnt = buf[5] & 0x7F;
	if (cnt > HYN_MAX_POINTS)
		goto OUT_PROCESS;
	else if (cnt == 0)
		goto CLR_POINT;

	if (cnt == 0x01) {
		goto FINGER_PROCESS;
	} else {
#if HYN_IIC_TRANSFER_LIMIT
		if ((buf[5] & 0x80) == 0x80) //key
		{
			i2c_len = (cnt - 1) * 5 + 3;
			len_1 = i2c_len;
			for (idx = 0; idx < i2c_len; idx += 6) {
				i2c_buf[0] = 0xD0;
				i2c_buf[1] = 0x07 + idx;

				if (len_1 >= 6) {
					len_2 = 6;
					len_1 -= 6;
				} else {
					len_2 = len_1;
					len_1 = 0;
				}

				ret = cst3xx_i2c_read_register(hyn_ts_data, i2c_buf, len_2);
				if (ret < 0)
					goto OUT_PROCESS;

				for (i = 0; i < len_2; i++) {
					buf[5 + idx + i] = i2c_buf[i];
				}
			}

			i2c_len += 5;
			key_status = buf[i2c_len - 3];
			key_id = buf[i2c_len - 2];
		} else {
			i2c_len = (cnt - 1) * 5 + 1;
			len_1 = i2c_len;

			for (idx = 0; idx < i2c_len; idx += 6) {
				i2c_buf[0] = 0xD0;
				i2c_buf[1] = 0x07 + idx;

				if (len_1 >= 6) {
					len_2 = 6;
					len_1 -= 6;
				} else {
					len_2 = len_1;
					len_1 = 0;
				}

				ret = cst3xx_i2c_read_register(hyn_ts_data, i2c_buf, len_2);
				if (ret < 0)
					goto OUT_PROCESS;

				for (i = 0; i < len_2; i++) {
					buf[5 + idx + i] = i2c_buf[i];
				}
			}
			i2c_len += 5;
		}
#else
		if ((buf[5] & 0x80) == 0x80) {
			buf[5] = 0xD0;
			buf[6] = 0x07;
			i2c_len = (cnt - 1) * 5 + 3;
			ret = cst3xx_i2c_read_register(hyn_ts_data, &buf[5], i2c_len);
			if (ret < 0)
				goto OUT_PROCESS;
			i2c_len += 5;
			key_status = buf[i2c_len - 3];
			key_id = buf[i2c_len - 2];
		} else {
			buf[5] = 0xD0;
			buf[6] = 0x07;
			i2c_len = (cnt - 1) * 5 + 1;
			ret = cst3xx_i2c_read_register(hyn_ts_data, &buf[5], i2c_len);
			if (ret < 0)
				goto OUT_PROCESS;
			i2c_len += 5;
		}
#endif

		if (buf[i2c_len - 1] != 0xAB) {
			goto OUT_PROCESS;
		}
	}

	//both key and point
	if ((cnt > 0) && (key_status & 0x80)) {
		if (hyn_ts_data->report_flag == 0xA5)
			goto KEY_PROCESS;
	}

FINGER_PROCESS:

	i2c_buf[0] = 0xD0;
	i2c_buf[1] = 0x00;
	i2c_buf[2] = 0xAB;
	ret = cst3xx_i2c_write(hyn_ts_data, i2c_buf, 3);
	if (ret < 0) {
		HYN_ERROR(" cst3xx hyn send read touch info ending failed.\r\n");
		goto OUT_PROCESS;
	}
#if (HYN_DEBUG_EN && (HYN_DEBUG_LEVEL == 2))
	hyn_show_touch_buffer(buf, cnt);
#endif
	idx = 0;
	cnt_up = 0;
	cnt_down = 0;
	for (i = 0; i < cnt; i++) {
		//u16 checksum;
		input_x = (unsigned int)((buf[idx + 1] << 4) | ((buf[idx + 3] >> 4) & 0x0F));
		input_y = (unsigned int)((buf[idx + 2] << 4) | (buf[idx + 3] & 0x0F));
		input_w = (unsigned int)(buf[idx + 4]);
		sw = (buf[idx] & 0x0F) >> 1;
		finger_id = (buf[idx] >> 4) & 0x0F;

#if 0
	    checksum=0;
		checksum+=buf[idx + 0];
		checksum+=buf[idx + 1];
		checksum+=buf[idx + 2];
		checksum+=buf[idx + 3];
		checksum+=0xA8;


		if((report_check_flag==0)&&((checksum&0xff)==input_w)){
			report_check_flag=1;
		}else if((report_check_flag==1)&&((checksum&0xff)!=input_w)){	
			HYN_ERROR(" cst3xx hyn read touch info checksum failed:0x%x .\r\n",checksum); 
			continue;		
		}
#endif

		if (hyn_ts_data->pdata->xy_exchange) {
			input_xy_temp = input_x;
			input_x = input_y;
			input_y = input_xy_temp;
		}

		if (hyn_ts_data->pdata->x_overturn) {
			input_x = hyn_ts_data->pdata->x_resolution - input_x;
		}
		if (hyn_ts_data->pdata->y_overturn) {
			input_y = hyn_ts_data->pdata->y_resolution - input_y;
		}
#if (HYN_DEBUG_EN && (HYN_DEBUG_LEVEL == 2))
		HYN_INFO(" cst3xx Point x:%d, y:%d, id:%d, sw:%d, press:%d. \n", input_x, input_y, finger_id, sw,
			 input_w);
#endif
		if ((hyn_ts_data->pdata->max_touch_num < finger_id) || ((sw != 0x03) && (sw != 0x0))) {
			HYN_INFO(" cst3xx error Point x:%d, y:%d, id:%d, sw:%d, press:%d. \n", input_x, input_y,
				 finger_id, sw, input_w);
			break;
		}
		if (sw == 0x03) {
			cst3xx_touch_down(hyn_ts_data->input_dev, finger_id, input_x, input_y, input_w);
			cnt_down++;
		} else {
			cnt_up++;
#if HYN_MT_PROTOCOL_B_EN
			cst3xx_touch_up(hyn_ts_data->input_dev, finger_id);
#endif
		}
		idx += 5;
	}
#if HYN_MT_PROTOCOL_B_EN
	if ((cnt_up > 0) && (cnt_down == 0))
		input_report_key(hyn_ts_data->input_dev, BTN_TOUCH, 0);
	else
		input_report_key(hyn_ts_data->input_dev, BTN_TOUCH, 1);
#endif

#if (!HYN_MT_PROTOCOL_B_EN)
	if ((cnt_up > 0) && (cnt_down == 0))
		cst3xx_touch_up(hyn_ts_data->input_dev, 0);
#endif

	if (cnt_down == 0)
		hyn_ts_data->report_flag = 0;
	else
		hyn_ts_data->report_flag = 0xCA;

	input_sync(hyn_ts_data->input_dev);
	goto END;

KEY_PROCESS:

	i2c_buf[0] = 0xD0;
	i2c_buf[1] = 0x00;
	i2c_buf[2] = 0xAB;
	ret = cst3xx_i2c_write(hyn_ts_data, i2c_buf, 3);
	if (ret < 0) {
		HYN_ERROR(" cst3xx hyn send read touch info ending failed.\r\n");
		hyn_reset_proc(20);
	}

	if (!hyn_ts_data->pdata->report_key_position) {
		if (key_status & 0x80) {
			i = (key_id >> 4) - 1;
			if ((key_status & 0x7F) == 0x03) {
				if ((i == hyn_ts_data->key_index) || (hyn_ts_data->key_index == 0xFF)) {
					HYN_INFO(" cst3xx key down code:%d,key id:%d. \n",
						 hyn_ts_data->pdata->key_code[i], i);
					input_report_key(hyn_ts_data->input_dev, hyn_ts_data->pdata->key_code[i], 1);
					hyn_ts_data->report_flag = 0xA5;
					hyn_ts_data->key_index = i;
				} else {
					HYN_INFO(" cst3xx key up code:%d,key id:%d. \n",
						 hyn_ts_data->pdata->key_code[hyn_ts_data->key_index],
						 hyn_ts_data->key_index);
					input_report_key(hyn_ts_data->input_dev,
							 hyn_ts_data->pdata->key_code[hyn_ts_data->key_index], 0);
					hyn_ts_data->key_index = 0xFF;
				}
			} else {
				HYN_INFO(" cst3xx key up code:%d,key id:%d. \n", hyn_ts_data->pdata->key_code[i], i);
				input_report_key(hyn_ts_data->input_dev, hyn_ts_data->pdata->key_code[i], 0);
				cst3xx_touch_up(hyn_ts_data->input_dev, 0);
				hyn_ts_data->report_flag = 0;
				hyn_ts_data->key_index = 0xFF;
			}
		}
	} else {
		if (key_status & 0x80) {
			i = (key_id >> 4) - 1;
			if ((key_status & 0x7F) == 0x03) {
				if ((i == hyn_ts_data->key_index) || (hyn_ts_data->key_index == 0xFF)) {
					HYN_INFO(" cst3xx key down x:%d,y id:%d. \n",
						 hyn_ts_data->pdata->key_x_coords[i],
						 hyn_ts_data->pdata->key_y_coords[i]);
					cst3xx_touch_down(hyn_ts_data->input_dev, 0,
							  hyn_ts_data->pdata->key_x_coords[i],
							  hyn_ts_data->pdata->key_y_coords[i], 50);
					hyn_ts_data->report_flag = 0xA5;
					hyn_ts_data->key_index = i;
				} else {
					hyn_ts_data->key_index = 0xFF;
				}
			} else {
				HYN_INFO(" cst3xx key up. \n");
				cst3xx_touch_up(hyn_ts_data->input_dev, 0);
				hyn_ts_data->report_flag = 0;
				hyn_ts_data->key_index = 0xFF;
			}
		}
	}
	input_sync(hyn_ts_data->input_dev);
	goto END;

CLR_POINT:
	HYN_ERROR(" CLEAR POINT.\n");
#if HYN_MT_PROTOCOL_B_EN
	for (i = 0; i <= 10; i++) {
		input_mt_slot(hyn_ts_data->input_dev, i);
		input_report_abs(hyn_ts_data->input_dev, ABS_MT_TRACKING_ID, -1);
		input_mt_report_slot_state(hyn_ts_data->input_dev, MT_TOOL_FINGER, false);
	}
	input_report_key(hyn_ts_data->input_dev, BTN_TOUCH, 0);
#else
	input_report_key(hyn_ts_data->input_dev, BTN_TOUCH, 0);
	input_mt_sync(hyn_ts_data->input_dev);
#endif
	input_sync(hyn_ts_data->input_dev);

OUT_PROCESS:
	buf[0] = 0xD0;
	buf[1] = 0x00;
	buf[2] = 0xAB;
	ret = cst3xx_i2c_write(hyn_ts_data, buf, 3);
	if (ret < 0) {
		HYN_ERROR("  send read touch info ending failed.\n");
		//hyn_reset_proc(20);
	}

END:
	cnt_up = 0;
	cnt_down = 0;
	return;
}

/*****************************************************************************
*  Name: cst8xx_touch_report
*  Brief:
*  Input:
*  Output:
*  Return:
*****************************************************************************/
static int cst8xx_touch_report(void)
{
	int i = 0;
	int ret;

#if HYN_MT_PROTOCOL_B_EN
	struct ts_event pevent;
#else
	struct touch_info cinfo, pinfo;
#endif

	struct touch_info finfo;
	if (hyn_ts_data->pdata->have_key) {
		memset(&finfo, 0, sizeof(struct touch_info));
		for (i = 0; i < HYN_MAX_POINTS; i++)
			finfo.p[i] = 1;
	}
#if !HYN_MT_PROTOCOL_B_EN
	memset(&cinfo, 0, sizeof(struct touch_info));
	memset(&pinfo, 0, sizeof(struct touch_info));
#endif

	HYN_DEBUG("cst8xx_touch_report start");

	{
#if HYN_GESTURE_EN

		hyn_gesture_readdata();
#endif

#if HYN_PSENSOR_EN
		hyn_proximity_readdata();
#endif

#if HYN_MT_PROTOCOL_B_EN
		{
			ret = hyn_read_touchdata(&pevent);
			if (ret == 0) {
				if (hyn_ts_data->pdata->have_key) {
					ret = !hyn_report_key(&pevent);
				}
				if (ret == 0) {
					hyn_report_value(&pevent);
				}
			}
		}
#else //HYN_MT_PROTOCOL_A_EN
		{
			ret = tpd_touchinfo(&cinfo, &pinfo);
			if (ret > 0) {
				if (hyn_ts_data->pdata->have_key) {
					if (cinfo.p[0] == 0)
						memcpy(&finfo, &cinfo, sizeof(struct touch_info));
				}

				if ((cinfo.y[0] >= hyn_ts_data->pdata->y_resolution) &&
				    (pinfo.y[0] < hyn_ts_data->pdata->x_resolution) &&
				    ((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
					HYN_DEBUG("All up");
					tpd_up(pinfo.x[0], pinfo.y[0]);
					input_sync(hyn_ts_data->input_dev);
				}

				if (hyn_ts_data->pdata->have_key) {
					if ((cinfo.y[0] <= hyn_ts_data->pdata->y_resolution && cinfo.y[0] != 0) &&
					    (pinfo.y[0] > hyn_ts_data->pdata->y_resolution) &&
					    ((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
						HYN_DEBUG("All key up");
						tpd_up(pinfo.x[0], pinfo.y[0]);
						input_sync(hyn_ts_data->input_dev);
					}

					if ((cinfo.y[0] > hyn_ts_data->pdata->y_resolution) ||
					    (pinfo.y[0] > hyn_ts_data->pdata->y_resolution)) {
						if (finfo.y[0] > hyn_ts_data->pdata->y_resolution) {
							if ((cinfo.p[0] == 0) || (cinfo.p[0] == 2)) {
								HYN_DEBUG("Key(%d,%d) Down", pinfo.x[0], pinfo.y[0]);
								tpd_down(pinfo.x[0], pinfo.y[0], 1, 0);
							} else if ((cinfo.p[0] == 1) &&
								   ((pinfo.p[0] == 0) || (pinfo.p[0] == 2))) {
								HYN_DEBUG("Key(%d,%d) Up!", pinfo.x[0], pinfo.y[0]);
								tpd_up(pinfo.x[0], pinfo.y[0]);
							}
							input_sync(hyn_ts_data->input_dev);
						}
					}
				}
				if (cinfo.count > 0) {
					for (i = 0; i < cinfo.count; i++)
						tpd_down(cinfo.x[i], cinfo.y[i], i + 1, cinfo.id[i]);
				} else {
					tpd_up(cinfo.x[0], cinfo.y[0]);
				}
				input_sync(hyn_ts_data->input_dev);
			}
		}
#endif
	}

	return 0;
}

int hyn_update_firmware_init(struct hynitron_ts_data *client)
{
	int ret = -1;
	unsigned char scan_id;

	HYN_FUNC_ENTER();
	ret = hyn_firmware_info(client);
	if (ret < 0) {
		HYN_ERROR("hyn_firmware_info failed.\n");
	}

	for (scan_id = 0; scan_id < ARRAY_SIZE(hynitron_chip_type_grp); scan_id++) {
		if (hyn_ts_data->config_chip_type == hynitron_chip_type_grp[scan_id].chip_type) {
			hyn_ts_data->config_chip_series = hynitron_chip_type_grp[scan_id].chip_series;
			hyn_ts_data->config_chip_product_line = hynitron_chip_type_grp[scan_id].chip_product_line;
			hyn_ts_data->chip_ic_bootloader_addr = hynitron_chip_type_grp[scan_id].chip_boot_addr;
			HYN_DEBUG("config [hyn] hyn_ts_data->config_chip_type = %d\n", hyn_ts_data->config_chip_type);
			HYN_DEBUG("config [hyn] scan_id = %d\n", scan_id);
			break;
		}
	}
	if (scan_id == ARRAY_SIZE(hynitron_chip_type_grp)) {
		HYN_ERROR("chip type error,please check firmware.\r\n");
		return 0;
	}

#if HYN_SYS_AUTO_SEARCH_FIRMWARE
	ret = hyn_sys_auto_search_firmware();
	if (ret > 0) {
		HYN_INFO("hyn_sys_auto_search_firmware find firmware,update first !\n");
	}

#endif

#if HYN_EN_AUTO_UPDATE
	ret = hyn_boot_update_fw(client);
	if (ret < 0) {
		HYN_ERROR("hyn_boot_update_fw failed.\n");
	}
#endif

	HYN_FUNC_EXIT();

	return ret;
}
int hyn_input_dev_int(struct hynitron_ts_data *ts_data)
{
	int ret = 0;
	int key_num = 0;
	struct hynitron_ts_platform_data *pdata = ts_data->pdata;
	struct input_dev *input_dev;

	HYN_FUNC_ENTER();

	ts_data->input_dev = input_allocate_device();
	input_dev = ts_data->input_dev;

	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	if (pdata->have_key) {
		HYN_INFO("set key capabilities");
		for (key_num = 0; key_num < HYN_MAX_KEYS; key_num++)
			input_set_capability(input_dev, EV_KEY, pdata->key_code[key_num]);
	}

#if HYN_MT_PROTOCOL_B_EN
	//clear_bit(BTN_TOUCH, ts_data->input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 7, 0))
	input_mt_init_slots(input_dev, ts_data->pdata->max_touch_num, INPUT_MT_DIRECT);
#else
	input_mt_init_slots(input_dev, ts_data->pdata->max_touch_num);
#endif

#else
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif

	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0, ts_data->pdata->max_touch_num, 0, 0);

#ifdef CONFIG_GTP_USE_PINCTRL
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ts_data->pdata->x_resolution, 0, 0); // fuzz --flat
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ts_data->pdata->y_resolution, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);

#if HYN_REPORT_PRESSURE_EN
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 0xFF, 0, 0);
#endif

#endif
	input_dev->name = "hynitron_cst8xxx_tp";

	ret = input_register_device(input_dev);
	if (ret) {
		HYN_ERROR("Register %s input device failed", input_dev->name);
		input_free_device(input_dev);
		return -ENODEV;
	}

	thread = kthread_run(hyn_touch_handler, (void *)ts_data, HYN_DRIVER_NAME, 0);
	if (IS_ERR(thread)) {
		ret = PTR_ERR(thread);
		HYN_ERROR("hyn create touch event handler thread failed: %d.\n", ret);
	}
	HYN_FUNC_EXIT();

	return ret;
}


int hyn_irq_init(struct hynitron_ts_data *client)
{
	int ret = -1;
	//spin_lock_init(&hyn_ts_data->irq_lock);

	hyn_ts_data->int_trigger_type = HYN_IRQ_TRIGGER_RISING_CONFIG;
	hyn_ts_data->pdata->irq_gpio_flags =
		((HYN_IRQ_TRIGGER_RISING_CONFIG) ? IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING);
	hyn_ts_data->pdata->irq_gpio_flags |= IRQF_ONESHOT;

	/* Configure gpio to irq and request irq */
#ifndef CONFIG_GTP_USE_PINCTRL
//	hyn_ts_data->use_irq = gpio_to_irq(hyn_ts_data->pdata->irq_gpio);
#endif
	ret = hyn_irq_registration(client);
	return ret;
}

static int hyn_gpio_configure(struct hynitron_ts_data *data)
{
#ifdef CONFIG_GTP_USE_PINCTRL

	gpio_set_output(GTP_RST_PORT, 0);
	mdelay(20);
	gpio_set_output(GTP_RST_PORT, 1);
	gpio_set_output(GTP_INT_PORT);

	return 0;

#else
	int ret = -1;

	HYN_FUNC_ENTER();

	/* request irq gpio */
	if (gpio_is_valid(data->pdata->irq_gpio)) {
		ret = gpio_configure(data->pdata->irq_gpio, GPIO_DIR_INPUT);
		if (ret < 0) {
			HYN_ERROR("[GPIO]set_direction for irq gpio failed");
			goto err_irq_gpio_dir;
		}
	}

	/* request reset gpio */
	if (gpio_is_valid(data->pdata->reset_gpio)) {
		ret = gpio_configure(data->pdata->reset_gpio, GPIO_DIR_OUTPUT);
		if (ret < 0) {
			HYN_ERROR("[GPIO]set_direction for reset gpio failed");
			goto err_reset_gpio_dir;
		}
	}
	gpio_set_output(hyn_ts_data->pdata->reset_gpio, 0);
	mdelay(20);
	gpio_set_output(hyn_ts_data->pdata->reset_gpio, 1);

	HYN_FUNC_EXIT();

	return 0;

err_reset_gpio_req:
err_reset_gpio_dir:
err_irq_gpio_req:
err_irq_gpio_dir:

	HYN_FUNC_EXIT();

	return ret;
#endif
}

#ifndef CONFIG_GTP_USE_PINCTRL
static int hyn_get_dt_coords(char *name, struct hynitron_ts_data *ts_data)
{
	int ret = 0;
	u32 coords[2] = { 0 };
	int coords_size = 2;
	struct hynitron_ts_platform_data *pdata = ts_data->pdata;

	ret = of_property_read_u32_array(ts_data->np, name, coords, coords_size);
	if (ret < 0) {
		HYN_ERROR("Unable to read %s, please check dts", name);
		pdata->x_resolution = HYN_X_DISPLAY_DEFAULT;
		pdata->y_resolution = HYN_Y_DISPLAY_DEFAULT;
		return -ENODATA;
	}
	pdata->x_resolution = coords[0];
	pdata->y_resolution = coords[1];

	HYN_INFO("display x(%d ) y(%d )", pdata->x_resolution, pdata->y_resolution);

	return 0;
}

static int hyn_parse_dt(struct hynitron_ts_data *ts_data)
{
	int ret = -1;
	u32 temp_val = 0;
	int i = 0;

	struct hynitron_ts_platform_data *pdata = ts_data->pdata;

	HYN_FUNC_ENTER();

	if (fdt_get_property_string_index(ts_data->np, "i2c-devpath", 0, &ts_data->i2c_devpath))
		return -1;

	if (fdt_get_property_u_32_index(ts_data->np, "reg", 0, (u32 *)&ts_data->addr))
		return -1;

	ret = hyn_get_dt_coords("hynitron,display-coords", ts_data);
	if (ret < 0) {
		HYN_ERROR("DTS Unable to get display-coords");
		return -1;
	}

	/* key */
	pdata->have_key = of_property_read_bool(ts_data->np, "hynitron,have-key");
	if (pdata->have_key) {
		ret = of_property_read_u32(ts_data->np, "hynitron,key-number", &pdata->max_key_num);
		if (ret < 0)
			HYN_ERROR("Key number undefined!");
		else if (pdata->max_key_num > HYN_MAX_KEYS)
			pdata->max_key_num = HYN_MAX_KEYS;

		ret = of_property_read_u32_array(ts_data->np, "hynitron,key-code", pdata->key_code, HYN_MAX_KEYS);
		if (ret < 0) {
			HYN_ERROR("Key code undefined!");
		}

		ret = of_property_read_u32_array(ts_data->np, "hynitron,key-x-coord", pdata->key_x_coords,
						 HYN_MAX_KEYS);
		if (ret < 0) {
			HYN_ERROR("Key X Coords undefined!");
		}

		ret = of_property_read_u32_array(ts_data->np, "hynitron,key-y-coord", pdata->key_y_coords,
						 HYN_MAX_KEYS);
		if (ret < 0) {
			HYN_ERROR("Key Y Coords undefined!");
		}

		for (i = 0; i < pdata->max_key_num; i++) {
			HYN_DEBUG("KEY Number:%d, key_x_coords:%d, key_y_coords:%d, key_code:%d.\n", pdata->max_key_num,
				  pdata->key_x_coords[i], pdata->key_y_coords[i], pdata->key_code[i]);
		}
	}
	/* reset, irq gpio info */
	//pdata->reset_gpio = of_get_named_gpio(dev->of_node, "hynitron,reset-gpio", 0);
	//pdata->irq_gpio = of_get_named_gpio(dev->of_node, "hynitron,irq-gpio", 0);

	if (fdt_get_property_u_32_index(ts_data->np, "hynitron,reset-gpio", 0, (u32 *)&pdata->reset_gpio)) {
		HYN_ERROR("DTS Unable to get reset_gpio");
		return -1;
	}

	if (fdt_get_property_u_32_index(ts_data->np, "hynitron,irq-gpio", 0, (u32 *)&pdata->irq_gpio)) {
		HYN_ERROR("DTS Unable to get irq_gpio");
		return -1;
	}

	ret = of_property_read_u32(ts_data->np, "hynitron,max-touch-number", &temp_val);
	if (ret < 0) {
		HYN_ERROR("DTS Unable to get max-touch-number, please check dts");
		pdata->max_touch_num = HYN_MAX_POINTS;
		return -1;
	} else {
		if (temp_val < 2)
			pdata->max_touch_num = 2; /* max_touch_number must >= 2 */
		else if (temp_val > HYN_MAX_POINTS)
			pdata->max_touch_num = HYN_MAX_POINTS;
		else
			pdata->max_touch_num = temp_val;
	}

	HYN_DEBUG("DTS max touch number:%d, irq gpio:%d, reset gpio:%d", pdata->max_touch_num, pdata->irq_gpio,
		  pdata->reset_gpio);

	HYN_FUNC_EXIT();

	return ret;
}
#endif /* ifndef CONFIG_GTP_USE_PINCTRL */

void hyn_ts_data_init(void)
{
	unsigned char scan_id;
	HYN_FUNC_ENTER();
	hyn_ts_data->config_chip_type = HYN_CHIP_TYPE_CONFIG;
	hyn_ts_data->config_chip_product_line = HYN_CHIP_PRODUCT_LINE_MUT_CAP;

	hyn_ts_data->chip_ic_main_addr = HYN_MAIN_IIC_ADDR_CONFIG;
	hyn_ts_data->pdata->x_overturn = HYN_X_REVERT;
	hyn_ts_data->pdata->y_overturn = HYN_Y_REVERT;
	hyn_ts_data->pdata->xy_exchange = HYN_XY_EXCHANGE;
	hyn_ts_data->work_mode = HYN_WORK_MODE_NORMAL;
	hyn_ts_data->pdata->max_touch_num = HYN_MAX_POINTS;

	for (scan_id = 0; scan_id < ARRAY_SIZE(hynitron_chip_type_grp); scan_id++) {
		if (hyn_ts_data->config_chip_type == hynitron_chip_type_grp[scan_id].chip_type) {
			hyn_ts_data->config_chip_series = hynitron_chip_type_grp[scan_id].chip_series;
			hyn_ts_data->config_chip_product_line = hynitron_chip_type_grp[scan_id].chip_product_line;
			hyn_ts_data->chip_ic_bootloader_addr = hynitron_chip_type_grp[scan_id].chip_boot_addr;
			HYN_DEBUG("config [hyn] scan_id = %d,config_chip_type:0x%x.\n", scan_id,
				  hyn_ts_data->config_chip_type);
			break;
		}
	}
	if (scan_id == ARRAY_SIZE(hynitron_chip_type_grp)) {
		HYN_ERROR("chip type error,please check firmware.\r\n");
		return;
	}

	if (hyn_ts_data->chip_ic_main_addr != hyn_ts_data->addr) {
		hyn_ts_data->chip_ic_main_addr = HYN_MAIN_IIC_ADDR_CONFIG;
		HYN_DEBUG(
			"client addr is different,now use default addr,please confirm. client->addr=0x%x, default iic addr=0x%x.\r\n",
			hyn_ts_data->addr, HYN_MAIN_IIC_ADDR_CONFIG);
	}

	HYN_DEBUG("config_chip_type:0x%x,main_addr:0x%x,x_overturn=%d,y_overturn=%d,xy_exchange=%d,.\r\n",
		  hyn_ts_data->config_chip_type, hyn_ts_data->chip_ic_main_addr, hyn_ts_data->pdata->x_overturn,
		  hyn_ts_data->pdata->y_overturn, hyn_ts_data->pdata->xy_exchange);
	HYN_FUNC_EXIT();
}

static int hyn_platform_data_init(struct hynitron_ts_data *ts_data)
{
	int ret = -1;
	int pdata_size = sizeof(struct hynitron_ts_platform_data);

	HYN_FUNC_ENTER();

	ts_data->pdata = kzalloc(pdata_size, GFP_KERNEL);
	if (!ts_data->pdata) {
		HYN_ERROR("allocate memory for platform_data fail");
		return -ENOMEM;
	}

#if 0
	if (tpd_dts_data.use_tpd_button) {
		/*initialize tpd button data*/
		u8 key_num;
		tpd_button_setting(tpd_dts_data.tpd_key_num, tpd_dts_data.tpd_key_local,
				   tpd_dts_data.tpd_key_dim_local);

		ts_data->pdata->have_key = true;
		hyn_ts_data->pdata->report_key_position = 0;
		for (key_num = 0; key_num < tpd_dts_data.tpd_key_num; key_num++) {
			ts_data->pdata->key_code[key_num] = tpd_dts_data.tpd_key_local[key_num];
		}
	}
#endif

#ifdef CONFIG_GTP_USE_PINCTRL
	HYN_DEBUG("USE DEFAULT PINCTRL.");
	ts_data->pdata->max_key_num = HYN_MAX_KEYS;
	ts_data->pdata->max_touch_num = HYN_MAX_POINTS;
	ts_data->pdata->x_resolution = HYN_X_DISPLAY_DEFAULT;
	ts_data->pdata->y_resolution = HYN_Y_DISPLAY_DEFAULT;
	return 0;
#else
	ret = hyn_parse_dt(ts_data);
	if (ret < 0) {
		HYN_ERROR("device-tree parse fail.");
		return -ENODEV;
	}
#endif

	return ret;
}

static int hyn_probe(const char *path)
{
	int ret = -1, np = -1;
	struct hynitron_ts_data *ts_data = NULL;	

	HYN_FUNC_ENTER();

	np = fdt_get_node_offset_by_path(path);
	if (np < 0) {
		HYN_ERROR("no find hynitron cst8xxx\n");
		return -ENODEV;
	}

	/* malloc memory for global struct variable,init 0 */
	ts_data = kzalloc(sizeof(*ts_data), GFP_KERNEL);
	if (!ts_data) {
		HYN_ERROR("allocate memory for ts_data fail");
		return -ENOMEM;
	}

	ts_data->np = np;

	if (tpd_load_status == 1)
		return -1;

	hyn_ts_data = ts_data;

	ret = hyn_platform_data_init(ts_data);
	if (ret < 0) {
		HYN_ERROR("hyn_platform_data_init fail,please check DTS.");
		kfree(ts_data);
		return ret;
	}

	ret = hyn_gpio_configure(ts_data);
	if (ret < 0) {
		HYN_ERROR("hyn_gpio_configure fail");
		return -1;
	}

	hyn_ts_data_init();

	mdelay(60);

        ret = hyn_firmware_info(ts_data);
        if(ret<0){
		HYN_ERROR("hyn_firmware_info failed,start check boot.\n");
		ret = hyn_detect_bootloader(ts_data);
		if (ret<0){
			HYN_ERROR("Touch Probe : detect hynitron ic type fail...");
			goto err_end;
		}
		mdelay(30);
	}else{
		HYN_INFO("hyn_firmware_info success.\n");
	}

	tpd_load_status = 1;

	ret = hyn_input_dev_int(ts_data);
	if (ret < 0) {
		HYN_ERROR("Touch Probe : hyn_input_dev_int  fail...");
		goto err_end;
	}

	ret = hyn_irq_init(ts_data);
	if (ret<0){
		HYN_ERROR("Touch Probe : hyn_irq_init  fail...");
		goto err_end;
	}	  

	hyn_irq_disable(ts_data);

	ret = hyn_update_firmware_init(ts_data);
	if (ret<0){
		HYN_ERROR("Touch Probe : hyn_update_firmware_init  fail...");
		goto err_end;
	}

#if HYN_GESTURE_EN
	hyn_gesture_init(ts_data->input_dev, client);
	if (ret < 0) {
		HYN_ERROR("Touch Probe : hyn_gesture_init  fail...");
	}
#endif

#if HYN_PSENSOR_EN
	hyn_proximity_init();
	if (ret < 0) {
		HYN_ERROR("Touch Probe : hyn_proximiy_init  fail...");
	}
#endif

#if HYN_ESDCHECK_EN
	hyn_esd_init();
#endif

	hyn_reset_proc(40);

	hyn_irq_enable(ts_data);

	HYN_DEBUG("Touch Panel Device Probe %s!", (ret < 0) ? "FAIL" : "PASS");

	HYN_FUNC_EXIT();

	return 0;

err_end:
	return ret;

}

static int hc_hynitron_cst8xxx_init(void)
{
	int ret = -1;

	ret = hyn_probe("/hcrtos/hynitron@1a");

	return ret;
}

module_driver(hc_hynitron_cst8xxx_driver, hc_hynitron_cst8xxx_init, NULL, 1)
