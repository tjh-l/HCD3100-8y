#include <kernel/module.h>
#include <sys/unistd.h>
#include <errno.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/ld.h>
#include <stdio.h>
#include <nuttx/fs/fs.h>
#include <linux/slab.h>
#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include "adc_15xx_reg_struct.h"
#include "hc_adc_get_calibration.h"

#define QUERY_ADC_ADJUST_WAY_NONE               0
#define QUERY_ADC_ADJUST_WAY_FLASH              2

struct adc_priv_1512
{
	struct 			device *dev;
	void __iomem 		*base;
	uint8_t			adjust_way;  /* adjust way : flash > none */
	uint8_t			adjust_value;
};

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
	struct adc_priv_1512 *priv = inode->i_private;
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;
	uint8_t sar_dout = 0;

	if (priv->adjust_way == QUERY_ADC_ADJUST_WAY_FLASH)
		if ((reg->sar_ctrl_reg0.sar_dout * 241 / priv->adjust_value) > 0xff )
			sar_dout = 0xff;
		else
			sar_dout = reg->sar_ctrl_reg0.sar_dout * 241 / priv->adjust_value;
	else
		sar_dout = reg->sar_ctrl_reg0.sar_dout;

	*buffer = sar_dout;

	return buflen;
}

static const struct file_operations query_fops = {
	.open  = queryadc_open, /* open */
	.close = queryadc_close, /* close */
	.read  = queryadc_read, /* read */
};

static void hc_key_adc_adjust_init(struct adc_priv_1512 *priv)
{
	/* flash adjust */
	if (!__sysdata_read_adc_calibration(&priv->adjust_value)) {
		priv->adjust_way = QUERY_ADC_ADJUST_WAY_FLASH;
		return;
	}

#ifdef CONFIG_KEY_ADC_SAVE_CALIBRATION_TO_OTP
	if (!__flash_otp_read_adc_calibration(&priv->adjust_value)) {
		priv->adjust_way = QUERY_ADC_ADJUST_WAY_FLASH;
		return;
	}
#endif
	/* no adjust way */
	priv->adjust_way = QUERY_ADC_ADJUST_WAY_NONE;
}

static int hc_1512_adc_reg_init(struct adc_priv_1512 *priv)
{
	saradc_1512_reg_t *reg = (saradc_1512_reg_t *)priv->base;

	reg->sar_ctrl_reg3.val = 0x00;

	reg->sar_ctrl_reg3.sar_clk_sel = 0x01;
	reg->sar_ctrl_reg2.default_value = 0xff;
	reg->sar_ctrl_reg2.debounce_value = 0x00;

	reg->sar_ctrl_reg1.repeat_num = 0x01;
	reg->sar_ctrl_reg1.clk_div_num = 0x0f;
	reg->sar_ctrl_reg1.sar_en = 0x01;

	reg->sar_ctrl_reg1.clk_div_num = 0xff;

	reg->sar_ctrl_reg1.sar_en = 0x01;

	return 0;
}

static int hc_1512_queryadc_probe(char *node)
{
	int np;
	int ret = -EINVAL;
	const char * path;
	const char *status;
	struct adc_priv_1512 *priv;

	np = fdt_get_node_offset_by_path(node);
	if (np < 0) {
		printf("no found %s", node);
		return -ENOENT;
	}

	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
	    !strcmp(status, "disabled"))
		return 0;

	priv = kzalloc(sizeof(struct adc_priv_1512), GFP_KERNEL);
	if (!priv) {
		return -ENOMEM;
	}

	priv->base= (void *)&ADCCTRL;

	if (fdt_get_property_string_index(np, "devpath", 0, &path)) {
		return -ENOENT;
	}

	ret = hc_1512_adc_reg_init(priv);
	if (ret < 0) {
		goto err;
	}
	hc_key_adc_adjust_init(priv);
	register_driver(path, &query_fops, 0666, priv);

	return 0;

err:
	kfree(priv);

	return ret;
}

static int hc_1512_queryadc_init(void)
{
    int ret = 0;

    hc_1512_queryadc_probe("/hcrtos/queryadc0");

    return ret;
}

module_driver(saradc, hc_1512_queryadc_init, NULL, 0)
