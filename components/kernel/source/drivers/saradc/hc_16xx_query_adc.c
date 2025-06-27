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
#include <stdio.h>
#include <nuttx/fs/fs.h>
#include <linux/slab.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include "hc_adc_get_calibration.h"

#define QUERY_ADC_ADJUST_WAY_NONE               0
#define QUERY_ADC_ADJUST_WAY_EFUSE              1
#define QUERY_ADC_ADJUST_WAY_FLASH              2
#define QUERY_ADC_ADJUST_WAY_CHANNEL            3

#define IDMAP(id) ((id) > 3 ? ((id) + 2) : (id))

struct adc_priv_16xx
{
	int 			ch_id;
	struct 			device *dev;
	void 			__iomem *base;
	void 			__iomem *base_wait;
	int			adjust_channel; /* -1 : invalid, other : valid */
	uint8_t			adjust_way;  /* adjust way : channel > flash > efuse > none */
	uint8_t			adjust_value;
};

static void hc_key_adc_adjust_init(struct adc_priv_16xx *priv);

static int queryadc_open(struct file *filep)
{
	return 0;
}

static int queryadc_close(struct file *filep)
{
	return 0;
}

static ssize_t queryadc_read(struct file *filep, char *buffer, size_t buflen)
{
	struct inode *inode = filep->f_inode;
	struct adc_priv_16xx *priv = inode->i_private;
	adc_reg_t *reg = (adc_reg_t *)priv->base;
	uint8_t id = 0, adjust_id = 0;
	uint8_t sar_dout = 0;

	id = IDMAP(priv->ch_id);
	adjust_id = IDMAP(priv->adjust_channel);

	if (priv->adjust_value == 0)
		hc_key_adc_adjust_init(priv);

	if (priv->adjust_way == QUERY_ADC_ADJUST_WAY_NONE)
		return -1;

	if (priv->adjust_way == QUERY_ADC_ADJUST_WAY_CHANNEL)
		priv->adjust_value = reg->read_data_2[adjust_id].ch;

	if (priv->adjust_way != QUERY_ADC_ADJUST_WAY_NONE) {
		if ((reg->read_data_2[id].ch * 241 / priv->adjust_value) > 0xff )
			sar_dout = 0xff;
		else
			sar_dout = reg->read_data_2[id].ch * 241 / priv->adjust_value;
	}
	/*	else
		sar_dout = reg->read_data_2[id].ch;
	*/

	*buffer = sar_dout;

	return buflen;

}

static const struct file_operations query_fops = {
	.open  = queryadc_open, /* open */
	.close = queryadc_close, /* close */
	.read  = queryadc_read, /* read */
};

static void set_mult_channel(struct adc_priv_16xx *priv)
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

static void hc_key_adc_init(struct adc_priv_16xx *priv, int ch_id)
{
	adc_reg_t *reg = (adc_reg_t *)priv->base;
	adc_wait_reg_t *reg_wait = (adc_wait_reg_t *)priv->base_wait;

	reg->saradc_ctl.sar_clksel_core 	= 0x04;
	reg->saradc_ctl.sar_clksel_core_div 	= 0x10;
	reg->ctrl_reg.touch_panel_mode_sel	= 0x00;
	reg->count_end_thr[ch_id].ch		= 0x09;
	reg->ave_thr[ch_id].ch			= 0x1e;
	reg->cmp_def_val[ch_id].ch		= 0xff;
	reg->count_thr[ch_id].ch		= 0x20;
	reg->ctrl_reg.val			|= 0x01 << (16 + ch_id);

	reg->enable_ctl.out_wait_end_int_en	= 0x01;
	reg->ctrl_reg.new_arc_mode_sel		= 0x01;
	reg->old_read_ch.val			&= 0x00ffffff;
	reg->saradc_ctl.saradc_pwd		= 0x00;
	reg->old_read_ch.Average_sel		= 0x01;
	reg->def_val[ch_id].ch			= 0;

	reg_wait->new_saradc_ch_ctrl[ch_id].wait_ch_ave_counter = 0x29;
	reg_wait->new_saradc_wait_ch_counter_threshold[ch_id].wait_ch_counter_threshold = 0x2a;

	reg->saradc_ctl.saradc_pwd 		= 0x00;
	reg->saradc_en.sar_en 			= 0x01;

	return;
}

static void hc_key_adc_adjust_init(struct adc_priv_16xx *priv)
{
	/* channel adjust */
	if (priv->adjust_way == QUERY_ADC_ADJUST_WAY_CHANNEL) {
		return;
	}
	/* flash adjust */
#ifdef CONFIG_KEY_ADC_SAVE_CALIBRATION_TO_OTP
	if (!__flash_otp_read_adc_calibration(&priv->adjust_value)) {
		priv->adjust_way = QUERY_ADC_ADJUST_WAY_FLASH;
		return;
	}
#endif
	if (!__sysdata_read_adc_calibration(&priv->adjust_value)) {
		priv->adjust_way = QUERY_ADC_ADJUST_WAY_FLASH;
		return;
	}
	/* efuse adjust */
	if (!__efuse_read_adc_calibration(&priv->adjust_value)) {
		priv->adjust_way = QUERY_ADC_ADJUST_WAY_EFUSE;
		return;
	}
	/* no adjust way */
	priv->adjust_way = QUERY_ADC_ADJUST_WAY_NONE;

}

static int hc_16xx_queryadc_probe(char *node, int id)
{
	int np;
	int ret = -EINVAL;
	const char * path;
	const char *status;
	struct adc_priv_16xx *priv;

	np = fdt_get_node_offset_by_path(node);
	if (np < 0) {
		return 0;
	}

	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
	    !strcmp(status, "disabled"))
		return 0;

	if (fdt_get_property_string_index(np, "devpath", 0, &path)) {
		return -ENOENT;
	}

	priv = kzalloc(sizeof(struct adc_priv_16xx), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->base = (void *)&ADCCTRL;
	priv->base_wait 	= (void *)(0xb8800160);
	priv->ch_id = id;

	priv->adjust_channel = -1;
	fdt_get_property_u_32_index(np, "adjust_channel", 0, &priv->adjust_channel);
	if (priv->adjust_channel != -1) {
		hc_key_adc_init(priv, priv->adjust_channel);
		priv->adjust_way = QUERY_ADC_ADJUST_WAY_CHANNEL;
	}

	set_mult_channel(priv);
	hc_key_adc_adjust_init(priv);
	hc_key_adc_init(priv, priv->ch_id);

	register_driver(path, &query_fops, 0666, priv);

	return 0;

err:	
	kfree(priv);

	return ret;
}

static int hc_16xx_queryadc_init(void)
{
    int ret = 0;

    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc0", 0);
    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc1", 1);
    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc2", 2);
    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc3", 3);
    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc4", 4);
    ret |= hc_16xx_queryadc_probe("/hcrtos/queryadc5", 5);

    return ret;
}

module_driver(saradc, hc_16xx_queryadc_init, NULL, 0)
