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

#include "adc_15xx_reg_struct.h"
#include "hc_15xx_key_adc.h"
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include "hc_adc_get_calibration.h"

#define TIME_TO_WORK		50
#define TIME_TO_TIMER		1000
#define REPEAT_NUM		0X0f
#define CLK_DIV_NUM		0Xff
#define DEBOUNCE_VALUE		0X05
#define ADJUST_SLEEP_TIME	1*1000

//#define ADC_DEBUG_MSG
#ifdef  ADC_DEBUG_MSG
#define ADC_DBG(format, ...) printf("ADC DBG: " format, ##__VA_ARGS__)
#else
#define ADC_DBG(format, ...)
#endif

static void hc_key_adc_up(struct timer_list *param)
{
	struct adc_priv_1512 *priv = from_timer(priv, param, timer_keyup);
	input_report_key(priv->input, priv->key_code, 0);
	input_sync(priv->input);
	priv->key_state = 0;

	priv->finsh = true;
	return;
}

static void hc_key_code_clr(struct timer_list *param)
{
	struct adc_priv_1512 *priv = from_timer(priv, param, timer_clr_cnt);
	priv->down_val_st = 0;

	return;
}

static void hc_key_adc_down(struct adc_priv_1512 *priv)
{
	priv->timeout = 200000;
	if (priv->key_state == 0) {
//		input_event(priv->input, EV_MSC, MSC_SCAN, priv->key_code);
		input_report_key(priv->input, priv->key_code, 1);
		input_sync(priv->input);
		priv->key_state = 1;
	}
	if (priv->key_state == 1) {
		input_event(priv->input, EV_MSC, MSC_SCAN, priv->key_code);
		input_sync(priv->input);
	}

	priv->key_up_val = priv->key_down_val;
	priv->keyup_jiffies = jiffies + usecs_to_jiffies(priv->timeout);
	mod_timer(&priv->timer_keyup, priv->keyup_jiffies);
	return;
}

static int hc_key_adc_val_detection(struct adc_priv_1512 *priv)
{
	int i, ret;
	uint32_t down_val;
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	if (reg->sar_ctrl_reg2.default_value == 0xff)
		down_val = priv->key_down_val * priv->dts_refer_value / 260;
	else
		down_val = priv->key_down_val * priv->dts_refer_value / priv->dyn_adjust;

	ADC_DBG("%s:%d priv->key_down_val %d mV==\n", __func__, __LINE__, priv->key_down_val);
	ADC_DBG("%s:%d priv->dyn_adjust %d mV==\n", __func__, __LINE__, priv->dyn_adjust);
	ADC_DBG("%s:%d priv->dts_refer_value %d mV==\n", __func__, __LINE__, priv->dts_refer_value);
	ADC_DBG("%s:%d down_val %ld mV==\n", __func__, __LINE__, down_val);

	for (i = 0; i < priv->keymap_len; i++) {
		if (down_val >= priv->key_map[i].key_min_val &&
		    down_val <= priv->key_map[i].key_max_val) {
			priv->val_cmp = 0;
			if (i == priv->down_code_st)
				priv->down_val_st++;
			else {
				priv->down_code_st = i;
				priv->down_val_st = 0;
			}
			break;
		}
	}

	if (i >= priv->keymap_len)
		priv->val_cmp++;

	if (priv->val_cmp > 10) {
		priv->val_cmp = 0;
		priv->dyn_adjust = reg->sar_ctrl_reg2.default_value = reg->sar_ctrl_reg0.sar_dout;
		if (priv->dyn_adjust == 0 && priv->flash_adjust == 0)
			priv->dyn_adjust = 1;
		priv->down_val_st = 0;
	}

	if (priv->down_val_st > 1) {
		priv->down_val_st = 0;
		ret = priv->down_code_st;
		priv->down_code_st = -1;
		if (priv->finsh == true)
			priv->key_code = priv->key_map[i].key_code;

		return ret;
	} else {
		return -1;
	}

	return ret;
}

static void timer_adjust_func(struct timer_list *param)
{
	struct adc_priv_1512 *priv = from_timer(priv, param, timer_adjust);
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	int16_t value = 0;

	usleep(ADJUST_SLEEP_TIME);
	reg->sar_ctrl_reg0.sar_int_en = 0x00;
	reg->sar_ctrl_reg1.repeat_num = 0x00;
	reg->sar_ctrl_reg2.debounce_value = 0x00;
	usleep(ADJUST_SLEEP_TIME);

	if (priv->work_first_read == 0) {
		mod_timer(&priv->timer_adjust, jiffies + usecs_to_jiffies(10));
		priv->work_first_read = 1;
		goto out;
	}
	usleep(ADJUST_SLEEP_TIME);
	if (reg->sar_ctrl_reg0.sar_int_st == 0x01 &&
	    reg->sar_ctrl_reg0.sar_int_en == 0x00 &&
	    (priv->work_first_read == 1)) {

		if ((priv->otp_adjust == 0) && (priv->flash_adjust == 0)) {
			priv->tmp_adjust = priv->dyn_adjust = reg->sar_ctrl_reg0.sar_dout;
			printf("%s:%d adc tmp dyn_adjust %d\n", __func__, __LINE__, priv->dyn_adjust);
		}

		value = (int16_t)reg->sar_ctrl_reg0.sar_dout;

		/* Now it is no key pressed at this moment */
		if (priv->dyn_adjust_temp == 0) {
			priv->dyn_adjust_temp = value;
			priv->work_first_read = 0;
			__bootinfo_write_adc_calibration(priv->dyn_adjust);
			sys_set_sysdata_adc_adjust_value(priv->dyn_adjust);
			goto out;
		}
		if (abs(value - priv->dyn_adjust_temp) > 2) {
			priv->dyn_adjust_temp = 0;
			priv->work_first_read = 0;
			goto out;
		}

		/* training finished */
		priv->dyn_adjust = (uint8_t)value;
		if (priv->store_adjust) {
			int16_t tmp = (int16_t)priv->flash_adjust;
			if (abs(value - tmp) > 2) {
				/* new trained adjustment, update to flash */
				__bootinfo_write_adc_calibration(priv->dyn_adjust);
				sys_set_sysdata_adc_adjust_value(priv->dyn_adjust);
				ADC_DBG("%s:%d adc persistentmem last dyn_adjust %d\n", __func__, __LINE__, priv->dyn_adjust);
			}
#ifdef CONFIG_KEY_ADC_SAVE_CALIBRATION_TO_OTP
			if (abs(priv->otp_adjust - priv->flash_adjust) > 2) {
				__flash_otp_write_adc_calibration(priv->dyn_adjust);
				ADC_DBG("%s:%d adc otp last dyn_adjust %d\n", __func__, __LINE__, priv->dyn_adjust);
			}
#endif
		}
		priv->dyn_adjust_trained = 1;
	} else 
		priv->work_first_read = 0;
out:
	reg->sar_ctrl_reg1.repeat_num = REPEAT_NUM;
	reg->sar_ctrl_reg2.debounce_value = DEBOUNCE_VALUE;
	reg->sar_ctrl_reg0.sar_int_en = 0x01;

	return;
}

static void irq_work(void *param)
{
	struct adc_priv_1512 *priv = (struct adc_priv_1512 *)param;
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	priv->key_down_val = reg->sar_ctrl_reg0.sar_dout;

	if (hc_key_adc_val_detection(priv) >= 0) {
		if (priv->otp_adjust != 0 || priv->flash_adjust != 0 || priv->dyn_adjust_trained != 0 ||
		    priv->tmp_adjust != 0) {
			hc_key_adc_down(priv);
			priv->finsh = false;
		}
	}

#ifndef BR2_PACKAGE_APPS_BOOTLOADER
	if (priv->dyn_adjust_trained == 0) {
		mod_timer(&priv->timer_adjust, jiffies + usecs_to_jiffies(TIME_TO_TIMER * 1000));
	}
#endif
	return;
}

static void hc_1512_saradc_interrupt(uint32_t param)
{
	struct adc_priv_1512 *priv = (struct adc_priv_1512 *)param;
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	priv->work_first_read = 0;

	if (work_available(&priv->irq_work)) {
		work_queue(HPWORK, &priv->irq_work, irq_work, (void *)priv, 0);
	}

	reg->sar_ctrl_reg0.sar_int_en = 0x01;
	reg->sar_ctrl_reg0.sar_int_st = 0x01;

}

static int hc_key_adc_open(struct input_dev *dev)
{
	return 0;
}

static void hc_key_adc_close(struct input_dev *dev)
{
	return;
}

static int hc_1512_saradc_init(struct adc_priv_1512 *priv)
{
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	priv->down_code_st = -1;

	priv->input->open = hc_key_adc_open;
	priv->input->close = hc_key_adc_close;

	timer_setup(&priv->timer_keyup, hc_key_adc_up, 0);
	timer_setup(&priv->timer_clr_cnt, hc_key_code_clr, 0);
#ifndef BR2_PACKAGE_APPS_BOOTLOADER
	timer_setup(&priv->timer_adjust, timer_adjust_func, 0);
#endif

	reg->sar_ctrl_reg3.val = 0x00;

	reg->sar_ctrl_reg2.debounce_value = 0x00;
	reg->sar_ctrl_reg2.default_value = 0x00;

	reg->sar_ctrl_reg3.sar_clk_sel = 0x00;

	reg->sar_ctrl_reg1.repeat_num = REPEAT_NUM;
	reg->sar_ctrl_reg1.clk_div_num = CLK_DIV_NUM;
	reg->sar_ctrl_reg2.debounce_value = DEBOUNCE_VALUE;
	reg->sar_ctrl_reg0.sar_int_en = 0x00;
	reg->sar_ctrl_reg1.sar_en = 0x01;

	priv->key_down_val = 0x00;
	priv->key_up_val = 0x00;
	priv->val_cmp = 0x00;

	return 0;
}

static void hc_key_adc_adjust_init(struct adc_priv_1512 *priv)
{
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	priv->store_adjust = 1;

	__bootinfo_write_adc_calibration(0);
#ifdef CONFIG_KEY_ADC_SAVE_CALIBRATION_TO_OTP
	if (!__flash_otp_read_adc_calibration(&priv->otp_adjust)) {
		priv->dyn_adjust = priv->otp_adjust;
		__bootinfo_write_adc_calibration(priv->dyn_adjust);
	}
#endif

	if (priv->otp_adjust == 0 && !__sysdata_read_adc_calibration(&priv->flash_adjust)) {
		priv->dyn_adjust = priv->flash_adjust;
		__bootinfo_write_adc_calibration(priv->dyn_adjust);
	}

	ADC_DBG("%s:%d priv->otp_adjust %d==\n", __func__, __LINE__, priv->otp_adjust);
	ADC_DBG("%s:%d priv->flash_adjust %d==\n", __func__, __LINE__, priv->flash_adjust);

	if (priv->dyn_adjust != 0) {
		reg->sar_ctrl_reg2.default_value = priv->dyn_adjust;
	} else  {
		usleep(100 * 1000);	//wait data change
		reg->sar_ctrl_reg2.default_value = reg->sar_ctrl_reg0.sar_dout;
		priv->dyn_adjust = reg->sar_ctrl_reg0.sar_dout;
		ADC_DBG("%s:%d priv->default_value %d==\n", __func__, __LINE__, reg->sar_ctrl_reg2.default_value);
		if (priv->dyn_adjust == 0)
			priv->dyn_adjust = 1;
	}

	reg->sar_ctrl_reg0.sar_int_en = 0x01;
	reg->sar_ctrl_reg1.sar_en = 0x01;

	reg->sar_ctrl_reg0.sar_int_st = 1;
}

static void hc_key_adc_map_register(struct adc_priv_1512 *priv)
{
	int i = 0;

	for (i = 0; i < priv->keymap_len; i++)
		set_bit(priv->key_map[i].key_code, priv->input->keybit);
	return;
}

static int hc_1512_saradc_probe(char *node)
{
	int np;
	int ret = -EINVAL;
	struct adc_priv_1512 *priv;

	np = fdt_get_node_offset_by_path(node);
	if (np < 0) {
		printf("no found %s", node);
		return 0;
	}

	priv = kzalloc(sizeof(struct adc_priv_1512), GFP_KERNEL);
	if (!priv) {
		return -ENOMEM;
	}

	priv->input = input_allocate_device();
	if (!priv->input)
		goto err;

	priv->input->name = "adc_key";
	__set_bit(EV_KEY, priv->input->evbit);
	__set_bit(EV_REP, priv->input->evbit);
	__set_bit(EV_MSC, priv->input->evbit);
//	__set_bit(EV_MSC, priv->input->mscbit);

	priv->irq = (int)&SAR_ADC_INTR;
	if (priv->irq < 0) {
		ret = priv->irq;
		goto err;
	}

	priv->finsh		= true;
	priv->base = (void *)&ADCCTRL;

	ret = fdt_get_property_u_32_index(np, "key-num", 0, &priv->keymap_len);
	if (ret < 0) {
		printf("can't get property key-num\n");
	}
	priv->key_map = (hc_adckey_map_s *)malloc(sizeof(hc_adckey_map_s) *
						  priv->keymap_len);
	ret = fdt_get_property_u_32_array(np, "key-map", (u32 *)priv->key_map,
					  priv->keymap_len * 3);
	if (ret < 0) {
		printf("\n\ncan't get ke-map\n\n");
	}

	ret = fdt_get_property_u_32_index(np, "adc_ref_voltage", 0, &priv->dts_refer_value);
	if (ret < 0) {
		printf("can't get property adc_ref_voltage\n");
	}
	if (priv->dts_refer_value > 2000) /*mv*/
		priv->dts_refer_value = 2000;
	hc_key_adc_map_register(priv);

	ret = hc_1512_saradc_init(priv);
	if (ret < 0) {
		goto err;
	}
	hc_key_adc_adjust_init(priv);

	input_set_drvdata(priv->input, priv);
	ret = input_register_device(priv->input);
	xPortInterruptInstallISR(priv->irq, hc_1512_saradc_interrupt,
				 (uint32_t)priv);
	return 0;

err:
	input_free_device(priv->input);
	kfree(priv);

	return ret;
}

static int hc_1512_saradc_driver_init(void)
{
	int ret = 0;

	ret = hc_1512_saradc_probe("/hcrtos/adc");

	return ret;
}

module_system(saradc_module, hc_1512_saradc_driver_init, NULL, 4)
