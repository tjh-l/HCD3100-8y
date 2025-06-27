#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <fcntl.h>
#include <debug.h>
#include <kernel/ld.h>
#include <nuttx/pwm/pwm.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/module.h>
#include <hcuapi/pinmux.h>

#include "pwm_reg_struct.h"

struct hc_pwm_priv {
	const struct	pwm_ops_s *ops;
	int		id;
	const char 	*path;
	pwm_reg_t	*base;
	int		polarity;
	uint16_t 	pwm_clk_div;
};

static int hc_pwm_setup(struct pwm_lowerhalf_s *dev)
{
	struct hc_pwm_priv *priv = (struct hc_pwm_priv *)dev;
	pwm_reg_t *reg = (pwm_reg_t *)priv->base;

#ifdef CONFIG_SOC_HC16XX
	reg->channel_ctrl[priv->id].polarity = priv->polarity;
#elif defined(CONFIG_SOC_HC15XX)
	reg->divder_ctrl[priv->id].polarity = priv->polarity;
#endif

	return 0;	
}

#ifdef CONFIG_SOC_HC16XX
static int hc_pwm_start(struct pwm_lowerhalf_s *dev,
			const struct pwm_info_s *info)
{
	struct hc_pwm_priv *priv = (struct hc_pwm_priv *)dev;
	pwm_reg_t *reg = (pwm_reg_t *)priv->base;
	uint32_t period_ns, duty_ns;
	bool polarity;
	uint16_t h_val, l_val;

	period_ns = info->period_ns;
	duty_ns = info->duty_ns;
	polarity = info->polarity;

	if (period_ns < 37)
		return 0;

#ifdef CONFIG_RANGE_AUTO_SET
	priv->pwm_clk_div = 1;
	while (1) {
		if (((uint32_t)priv->pwm_clk_div * 37 * 65535) > period_ns)
			break;
		priv->pwm_clk_div++;
	}
	reg->clk_ctrl.pwm_clksel = priv->pwm_clk_div;
#endif

	if (duty_ns > period_ns)
		duty_ns = period_ns;

	h_val = duty_ns / priv->pwm_clk_div / 37;
	l_val = (period_ns - duty_ns) / priv->pwm_clk_div / 37;

	reg->channel_ctrl[priv->id].channel_en = 0x00;

	reg->channel_ctrl[priv->id].polarity = polarity;
	reg->divder_ctrl[priv->id].low_level_counter = l_val;
	reg->divder_ctrl[priv->id].high_level_counter = h_val;

	reg->channel_ctrl[priv->id].channel_en = 0x01;

	return 0;
}

static int hc_pwm_stop(struct pwm_lowerhalf_s *dev)
{
	struct hc_pwm_priv *priv = (struct hc_pwm_priv *)dev;
	
	pwm_reg_t *reg = (pwm_reg_t *)priv->base;

	reg->channel_ctrl[priv->id].channel_en = 0x00;

	return 0;
}

#elif defined(CONFIG_SOC_HC15XX)
static int hc_pwm_start(struct pwm_lowerhalf_s *dev,
			const struct pwm_info_s *info)
{
	struct hc_pwm_priv *priv = (struct hc_pwm_priv *)dev;
	pwm_reg_t *reg = (pwm_reg_t *)priv->base;
	uint32_t period_ns, duty_ns;
	bool polarity;
	uint16_t h_val, l_val ;

	period_ns = info->period_ns;
	duty_ns = info->duty_ns;
	polarity = info->polarity;
	

	if (period_ns < 37)
		return 0;

#ifdef CONFIG_RANGE_AUTO_SET
	priv->pwm_clk_div = 1;
	while (1) {
		if (((uint32_t)priv->pwm_clk_div * 37 * 65535) > period_ns)
			break;
		priv->pwm_clk_div++;
	}
	reg->clk_ctrl.pwm_clksel = priv->pwm_clk_div;
#endif
	if (duty_ns > period_ns)
		duty_ns = period_ns;

	h_val = duty_ns / priv->pwm_clk_div / 37;
	l_val = (period_ns - duty_ns) / priv->pwm_clk_div / 37;

	reg->divder_ctrl[priv->id].channel_en = 0x00;

	reg->divder_ctrl[priv->id].polarity = polarity;
	reg->divder_ctrl[priv->id].low_level_counter = l_val;
	reg->divder_ctrl[priv->id].high_level_counter = h_val;

	/* use other channel must enable ch0 */
	reg->divder_ctrl[0].channel_en = 0x01;
	reg->divder_ctrl[priv->id].channel_en = 0x01;

	return 0;
}

static int hc_pwm_stop(struct pwm_lowerhalf_s *dev)
{
	struct hc_pwm_priv *priv = (struct hc_pwm_priv *)dev;
	
	pwm_reg_t *reg = (pwm_reg_t *)priv->base;

	reg->divder_ctrl[priv->id].channel_en = 0x00;

	return 0;
}
#endif

static void hc_pwm_config(struct hc_pwm_priv *priv)
{
	pwm_reg_t *reg = (pwm_reg_t *)priv->base;

#ifdef CONFIG_RANGE_206Hz_135KHz
	priv->pwm_clk_div = 2;
#elif defined(CONFIG_RANGE_103Hz_67KHz)
	priv->pwm_clk_div = 4;
#elif defined(CONFIG_RANGE_50Hz_33KHz)
	priv->pwm_clk_div = 8;
#else
	priv->pwm_clk_div = 1;
#endif

#ifdef CONFIG_SOC_HC16XX
	if ((reg->channel_ctrl[priv->id].channel_en == 0x01) &&
	    (reg->clk_ctrl.pwm_enable == 0x01))
		return;
	reg->channel_ctrl[priv->id].polarity = priv->polarity;
	reg->channel_ctrl[priv->id].channel_en = 0x01;

#elif defined(CONFIG_SOC_HC15XX)
	if ((reg->divder_ctrl[priv->id].channel_en == 0x01) &&
	    (reg->clk_ctrl.pwm_enable == 0x01))
		return;

	reg->divder_ctrl[priv->id].polarity = priv->polarity;
	reg->divder_ctrl[priv->id].channel_en = 0x01;
#endif

	if (reg->clk_ctrl.pwm_enable != 0x01) {
		reg->clk_ctrl.pwm_enable = 0x01;
		reg->clk_ctrl.pwm_clken = 0x01;
		reg->clk_ctrl.pwm_clksel = priv->pwm_clk_div;
	}

	return;
}

static int hc_pwm_ioctl(struct pwm_lowerhalf_s *dev, int cmd,
                    unsigned long arg)
{
	return -ENOTTY;
}

static int hc_pwm_shutdown(struct pwm_lowerhalf_s *dev)
{
	return 0;
}

static const struct pwm_ops_s hc_pwm_ops = {
	.setup		= hc_pwm_setup,
	.start		= hc_pwm_start,
	.stop		= hc_pwm_stop,
	.shutdown       = hc_pwm_shutdown,
	.ioctl		= hc_pwm_ioctl,
};

static int hc_pwminitialize(const char *node, int id,
					    struct hc_pwm_priv *priv)
{
	int np;
	struct pinmux_setting *active_state;

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return -ENODEV;
	
	priv->id = id;
	priv->ops = &hc_pwm_ops;
	priv->base = (pwm_reg_t *)&PWMCTRL;
	priv->polarity = 0;

	if (fdt_get_property_string_index(np, "devpath", 0, &priv->path)) {
		pwmerr("ERROR: init fail\n");
		return -1;
	}

	active_state = fdt_get_property_pinmux(np, "active");
	if (active_state) {
		pinmux_select_setting(active_state);
		free(active_state);
	}

	fdt_get_property_u_32_index(np, "polarity", 0, &priv->polarity);

	hc_pwm_config(priv);

	return 0;
}

static int hc_pwm_probe(const char *node, int id)
{
	int ret = 0;
	struct pwm_lowerhalf_s *pwm;
	struct hc_pwm_priv *priv;

	pwm = (struct pwm_lowerhalf_s *)malloc(sizeof(struct pwm_lowerhalf_s));
	priv = (struct hc_pwm_priv *)malloc(sizeof(struct hc_pwm_priv));
	if (!priv) {
		pwmerr("ERROR: init %s fail\n", node);
		return -1;
	}
	memset(priv, 0, sizeof(struct hc_pwm_priv));

	ret = hc_pwminitialize(node, id, priv);
	if (ret == -ENODEV)
		return 0;

	pwm->info.polarity = priv->polarity;

	pwm->priv = (void *)priv;
	ret = pwm_register(priv->path, pwm);
	if (ret < 0)
		pwmerr("ERROR: pwm@%d register failed: %d\n", id, ret);

	return ret;
}

static int hc_pwm_init(void)
{
	int rc = 0;

	rc |= hc_pwm_probe("/hcrtos/pwm@0", 0);
	rc |= hc_pwm_probe("/hcrtos/pwm@1", 1);
	rc |= hc_pwm_probe("/hcrtos/pwm@2", 2);
	rc |= hc_pwm_probe("/hcrtos/pwm@3", 3);
	rc |= hc_pwm_probe("/hcrtos/pwm@4", 4);
	rc |= hc_pwm_probe("/hcrtos/pwm@5", 5);

	return rc;
}

module_driver(pwm, hc_pwm_init, NULL, 0);
