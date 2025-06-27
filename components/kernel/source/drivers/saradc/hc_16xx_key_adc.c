#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <kernel/drivers/input.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include <hcuapi/efuse.h>
#include <linux/jiffies.h>
#include "adc_16xx_reg_struct.h"
#include "hc_16xx_key_adc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include "hc_adc_get_calibration.h"

#define MS_TO_US(msec)          	((msec) * 1000)
#define ADC_DEFAULT_TIMEOUT      	MS_TO_US(200)
#define SAMPL_VAL               	5

#define IDMAP(id) ((id) > 3 ? ((id) + 2) : (id))

//#define ADC_DEBUG_MSG
#ifdef  ADC_DEBUG_MSG
#define ADC_DBG(format, ...) printf("ADC DBG: " format, ##__VA_ARGS__)
#else
#define ADC_DBG(format, ...)
#endif

static void hc_key_adc_up(struct timer_list *param)
{
	struct hc_key_adc_priv *priv = from_timer(priv, param, timer_keyup);
	input_report_key(priv->input, priv->key_code, 0);
	input_sync(priv->input);
	priv->key_state = 0;

	priv->finsh = true;

	return;
}

static void hc_key_code_clr(struct timer_list *param)
{
	struct hc_key_adc_priv *priv = from_timer(priv, param, timer_clr_cnt);
	priv->down_val_st = 0;

	return;
}

static void hc_key_adc_down(struct hc_key_adc_priv *priv)
{
	priv->timeout = ADC_DEFAULT_TIMEOUT;

	if (priv->key_state == 0) {
		//input_event(priv->input, EV_MSC, MSC_SCAN, priv->key_code);
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

static int hc_key_adc_val_detection(struct hc_key_adc_priv *priv)
{
	int i,id, ret;
	adc_reg_t *reg = (adc_reg_t *)priv->base;
	uint32_t down_val;

	if (priv->efuse_adjust && priv->dyn_adjust_trained == 0)
		down_val = priv->key_down_val * 1860 / priv->efuse_adjust;
	else
		down_val = priv->key_down_val * priv->dts_refer_value / priv->dyn_adjust;

	ADC_DBG("%s:%d down_val %ld mV==\n", __func__, __LINE__, down_val);

	id = IDMAP(priv->ch_id);

	for (i = 0; i < priv->keymap_len; i++) {
		if (down_val >= priv->key_map[i].key_min_val && down_val <= priv->key_map[i].key_max_val) {
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
		priv->dyn_adjust = reg->def_val[priv->ch_id].ch =
			reg->read_data_2[id].ch;
		if (priv->dyn_adjust == 0 && priv->flash_adjust == 0)
			priv->dyn_adjust = 1;
		priv->down_val_st = 0;
	}

#ifdef BR2_PACKAGE_APPS_BOOTLOADER
	if (priv->down_val_st > 1) {
#else
	if (priv->down_val_st > SAMPL_VAL) {
#endif
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

static void dyn_adjust_train_work(void *param)
{
	struct hc_key_adc_priv *priv = (struct hc_key_adc_priv *)param;
	adc_reg_t *reg = (adc_reg_t *)priv->base;
	int id = IDMAP(priv->ch_id);
	uint16_t status= 0;
	int16_t value = 0;

	status = reg->status_ctl.val;
	if ((((status >> 6) & 0x01) == 0) && (status >> (8 + priv->ch_id)) &&
	    ((status >> priv->ch_id) & 0x01) == 1) {

		if ((priv->otp_adjust == 0) && (priv->flash_adjust == 0)) {
			priv->tmp_adjust = priv->dyn_adjust = reg->read_data_2[id].ch;
			ADC_DBG("%s:%d priv->dyn_adjust %d==\n", __func__, __LINE__, priv->dyn_adjust);
		}

		value = (int16_t)reg->read_data_2[id].ch;

		/* Now it is no key pressed at this moment */
		if (priv->dyn_adjust_temp == 0) {
			priv->dyn_adjust_temp = value;
			/* save adjust to flash */
			__bootinfo_write_adc_calibration(priv->dyn_adjust);
			sys_set_sysdata_adc_adjust_value(priv->dyn_adjust);
			ADC_DBG("%s:%d priv->dyn_adjust %d==\n", __func__, __LINE__, priv->dyn_adjust);
			return;
		}

		if (abs(value - priv->dyn_adjust_temp) > 2) {
			priv->dyn_adjust_temp = value;
			ADC_DBG("%s:%d priv->dyn_adjust %d==\n", __func__, __LINE__, priv->dyn_adjust);
			return;
		}

		/* training finished */
		priv->dyn_adjust = (uint8_t)value;

		if (priv->store_adjust) {
			ADC_DBG("%s:%d priv->dyn_adjust %d==\n", __func__, __LINE__, priv->dyn_adjust);
			int16_t tmp = (int16_t)priv->flash_adjust;
			if (abs(value - tmp) > 2) {
				/* new trained adjustment, update to flash */
				/* save adjust to flash */
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
	}
	return;
}

static void hc_key_irq_work(void *param)
{
	struct hc_key_adc_priv *priv = (struct hc_key_adc_priv *)param;
	adc_reg_t *reg = (adc_reg_t *)priv->base;
	int id = IDMAP(priv->ch_id);

	priv->key_down_val = reg->read_data_2[id].ch;

	if (hc_key_adc_val_detection(priv) >= 0) {
		if (priv->otp_adjust != 0 || priv->efuse_adjust != 0 || priv->flash_adjust != 0 ||
		    priv->dyn_adjust_trained != 0 || priv->tmp_adjust != 0) {
			hc_key_adc_down(priv);
			priv->finsh = false;
		}
	}

	if (priv->dyn_adjust_trained == 0) {
		work_queue(HPWORK, &priv->work, dyn_adjust_train_work, (void *)priv, 50);
	}

	return;
}

static void hc_key_adc_interrupt(uint32_t param)
{
	struct hc_key_adc_priv *priv = (struct hc_key_adc_priv *)param;
	adc_reg_t *reg = (adc_reg_t *)priv->base;
	int status;

	status = reg->status_ctl.val;

	work_queue(HPWORK, &priv->irq_work, hc_key_irq_work, (void *)priv, 0);
	mod_timer(&priv->timer_clr_cnt, jiffies + usecs_to_jiffies(150 * 1000));

	reg->status_ctl.val = status;

	return;
}

static int hc_key_adc_open(struct input_dev *dev)
{
	return 0;
}

static void hc_key_adc_close(struct input_dev *dev)
{
	return;
}

static void hc_key_adc_map_register(struct hc_key_adc_priv *priv)
{
	int i = 0;

	for (i = 0; i < priv->keymap_len; i++)
		set_bit(priv->key_map[i].key_code, priv->input->keybit);
	return;
}

static void hc_key_adc_init(struct hc_key_adc_priv *priv)
{
	adc_reg_t *reg = (adc_reg_t *)priv->base;
	adc_wait_reg_t *reg_wait = (adc_wait_reg_t *)priv->base_wait;

	priv->input->open = hc_key_adc_open;
	priv->input->close = hc_key_adc_close;

        timer_setup(&priv->timer_keyup, hc_key_adc_up, 0);
        timer_setup(&priv->timer_clr_cnt, hc_key_code_clr, 0);

	reg->saradc_ctl.sar_clksel_core 	= 0x04;
	reg->saradc_ctl.sar_clksel_core_div 	= 0x10;
	reg->ctrl_reg.touch_panel_mode_sel	= 0x00;
	reg->count_end_thr[priv->ch_id].ch	= 0x09;
	reg->ave_thr[priv->ch_id].ch		= 0x1e;
	reg->cmp_def_val[priv->ch_id].ch	= 0x08;
	reg->count_thr[priv->ch_id].ch		= 0x20;
	reg->ctrl_reg.val			|= 0x01 << (16 + priv->ch_id);

	reg->enable_ctl.out_wait_end_int_en	= 0x01;
	reg->ctrl_reg.new_arc_mode_sel		= 0x01;
	reg->old_read_ch.val			&= 0x00ffffff;
	reg->saradc_ctl.saradc_pwd		= 0x00;
	reg->old_read_ch.Average_sel		= 0x01;
	reg->def_val[priv->ch_id].ch		= 0;

	reg_wait->new_saradc_ch_ctrl[priv->ch_id].wait_ch_ave_counter = 0xf8;
	reg_wait->new_saradc_wait_ch_counter_threshold[priv->ch_id].wait_ch_counter_threshold = 0xff;

	priv->key_down_val			= 0x00;
	priv->key_up_val			= 0x00;

	reg->saradc_en.sar_en = 0x01;

	return;
}

static void hc_key_adc_adjust_init(struct hc_key_adc_priv *priv)
{
	int id;
	adc_reg_t *reg = (adc_reg_t *)priv->base;

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

	if ((priv->otp_adjust == 0) && (priv->flash_adjust == 0)) {
		if (!__efuse_read_adc_calibration(&priv->efuse_adjust))
			__bootinfo_write_adc_calibration(priv->efuse_adjust);
	}

	ADC_DBG("%s:%d priv->otp_adjust %d==\n", __func__, __LINE__, priv->otp_adjust);
	ADC_DBG("%s:%d priv->flash_adjust %d==\n", __func__, __LINE__, priv->flash_adjust);
	ADC_DBG("%s:%d priv->efuse_adjust %d==\n", __func__, __LINE__, priv->efuse_adjust);

	if (priv->efuse_adjust != 0) {
		reg->def_val[priv->ch_id].ch = priv->dts_refer_value * priv->efuse_adjust / 1860;
	} else if (priv->dyn_adjust != 0) {
		reg->def_val[priv->ch_id].ch = priv->dyn_adjust;
	} else  {
		id = IDMAP(priv->ch_id);
		usleep(1000);
		reg->def_val[priv->ch_id].ch = reg->read_data_2[id].ch;
		reg->def_val[priv->ch_id].ch = reg->read_data_2[id].ch;
		priv->dyn_adjust = reg->def_val[priv->ch_id].ch;
		usleep(1000);
		ADC_DBG("%s:%d read_data %d==\n", __func__, __LINE__, reg->read_data_2[id].ch);
		if (priv->dyn_adjust == 0)
			priv->dyn_adjust = 1;
	}

	int irq_status;

	irq_status = reg->status_ctl.val;
	reg->status_ctl.val = irq_status;
}

static void set_mult_channel(struct hc_key_adc_priv *priv)
{
	int id = 5;

	adc_reg_t *reg = (adc_reg_t *)priv->base;
	adc_wait_reg_t *reg_wait = (adc_wait_reg_t *)priv->base_wait;

	reg->saradc_ctl.sar_clksel_core 	= 0x04;
	reg->saradc_ctl.sar_clksel_core_div 	= 0x10;
	reg->ctrl_reg.touch_panel_mode_sel	= 0x00;
	reg->count_end_thr[id].ch		= 0x09;
	reg->ave_thr[id].ch			= 0x02;
	reg->cmp_def_val[id].ch			= 0xff;
	reg->count_thr[id].ch			= 0x04;
	reg->ctrl_reg.val			|= 0x01 << (16 + id);
	reg->enable_ctl.out_wait_end_int_en	= 0x01;
	reg->ctrl_reg.new_arc_mode_sel		= 0x01;
	reg->old_read_ch.val			&= 0x00ffffff;
	reg->saradc_ctl.saradc_pwd		= 0x00;
	reg->old_read_ch.Average_sel		= 0x01;
	reg->def_val[id].ch 			= 0xff;

	reg_wait->new_saradc_ch_ctrl[id].wait_ch_ave_counter = 0x04;
	reg_wait->new_saradc_wait_ch_counter_threshold[id].wait_ch_counter_threshold = 0x08;
}

static int hc_key_adc_probe(char *node, int id)
{
	int np;
	int ret = -EINVAL;
	const char *status;
	struct hc_key_adc_priv *priv;

	ADC_DBG("=== start probe id %d==\n", id);
	np = fdt_get_node_offset_by_path(node);
	if (np < 0) {
		return 0;
	}

	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
	    !strcmp(status, "disabled"))
		return 0;

	priv = kzalloc(sizeof(struct hc_key_adc_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->input = input_allocate_device();
	if (!priv->input)
		goto err;

	priv->input->name = "adc_key";
	__set_bit(EV_KEY, priv->input->evbit);
	__set_bit(EV_REP, priv->input->evbit);
	__set_bit(EV_MSC, priv->input->evbit);
	__set_bit(EV_MSC, priv->input->mscbit);

	priv->irq = (int)&SAR_ADC_INTR;
	if (priv->irq < 0) {
		ret = priv->irq;
		goto err;
	}

	priv->finsh		= true;
	priv->ch_id 		= id;
	priv->base 		= (void *)&ADCCTRL;
	priv->base_wait 	= (void *)(0xb8800160);

	ret = fdt_get_property_u_32_index(np, "key-num", 0, &priv->keymap_len);
	priv->key_map = (hc_adckey_map_s *)malloc(sizeof(hc_adckey_map_s) * priv->keymap_len);
	fdt_get_property_u_32_array(np, "key-map", (u32 *)priv->key_map, priv->keymap_len * 3);

	ret = fdt_get_property_u_32_index(np, "adc_ref_voltage", 0, &priv->dts_refer_value);
	if (ret < 0) {
		printf("can't get property refer_value\n");
		return -1;
	}
	if (priv->dts_refer_value > 2000) {
		printf("refer_value to big , must small 2000mV\n");
		return -1;
	}

	set_mult_channel(priv);
	hc_key_adc_map_register(priv);
	hc_key_adc_init(priv);
	hc_key_adc_adjust_init(priv);

	input_set_drvdata(priv->input, priv);
	ret = input_register_device(priv->input);
	xPortInterruptInstallISR(priv->irq, hc_key_adc_interrupt, (uint32_t)priv);

	ADC_DBG("=== finsh probe id %d==\n", id);
	return 0;

err:
	input_free_device(priv->input);
	kfree(priv);

	return ret;
}

static int hc_adc_key_init(void)
{
	int ret = 0;
	ret |= hc_key_adc_probe("/hcrtos/key-adc@0", 0);
	ret |= hc_key_adc_probe("/hcrtos/key-adc@1", 1);
	ret |= hc_key_adc_probe("/hcrtos/key-adc@2", 2);
	ret |= hc_key_adc_probe("/hcrtos/key-adc@3", 3);
	ret |= hc_key_adc_probe("/hcrtos/key-adc@4", 4);
	ret |= hc_key_adc_probe("/hcrtos/key-adc@5", 5);
	return ret;
}

module_system(hc_key_adc, hc_adc_key_init, NULL, 4)
