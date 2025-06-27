#define ELOG_OUTPUT_LVL ELOG_LVL_ERROR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <kernel/io.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/module.h>

#include "lvds.h"
#include "lvds_lld.h"
#include "lvds_hal.h"
#include <hcuapi/lvds.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>
#include <hcuapi/pinpad.h>
#include <hcuapi/pinmux.h>
#include <kernel/drivers/hc_clk_gate.h>
// #include "display/lcd_spi.h"
#define MODULE_NAME "/dev/lvds"
#include <nuttx/pwm/pwm.h>
#include <hcuapi/standby.h>

typedef struct pinpad_gpio
{
	pinpad_e padctl;
	bool active;
}pinpad_gpio_s;
#define LCD_BACKLIGHT_NUM_MAX 5
#define LCD_POWER_NUM_MAX 10
#define LVDS_GPIO_LOW 0
#define LVDS_GPIO_HIGH 1
#define LVDS_RET_ERROR (-EINVAL)
#define LVDS_RET_SUCCESS 0
struct hichip_lvds {
	void *reg_base;
	void *sys_base;
	int vcom_fd;
	int backlight_fd;
	struct pwm_info_s pwm_backlight;
	struct pwm_info_s pwm_vcom;
	pinpad_gpio_s rgb_backlight;
	pinpad_gpio_s lcd_backlight[LCD_BACKLIGHT_NUM_MAX];
	pinpad_gpio_s lcd_power[LCD_POWER_NUM_MAX];
	u32 lcd_backlight_num;
	u32 lcd_power_num;
	lvds_info_t info;
	u32 lvds_src_sel;
	bool default_off;
};

static struct hichip_lvds lvds_misc;

static void config_lvds_ttl(void);
static int lvds_set_pwm_vcom_duty(unsigned long duty)
{
	struct pwm_info_s info = lvds_misc.pwm_vcom;
	if (lvds_misc.vcom_fd > 0) {
		if (duty <= 100) {
			info.duty_ns = info.period_ns / 100 * duty;
			ioctl(lvds_misc.vcom_fd, PWMIOC_SETCHARACTERISTICS, &info);
			ioctl(lvds_misc.vcom_fd, PWMIOC_START);
		} else {
			ioctl(lvds_misc.vcom_fd, PWMIOC_SETCHARACTERISTICS,	&info);
			ioctl(lvds_misc.vcom_fd, PWMIOC_START);
		}
	} else
		return LVDS_RET_ERROR;

	return LVDS_RET_SUCCESS;
}

static int lvds_set_pwm_backlight_duty(unsigned long duty)
{
	struct pwm_info_s info = lvds_misc.pwm_backlight;
	if (lvds_misc.backlight_fd > 0) {
		if (duty <= 100) {
			info.duty_ns=info.period_ns/100*duty;
			ioctl(lvds_misc.backlight_fd, PWMIOC_SETCHARACTERISTICS, &info);
			ioctl(lvds_misc.backlight_fd, PWMIOC_START);
		} else {
			ioctl(lvds_misc.backlight_fd, PWMIOC_SETCHARACTERISTICS, &info);
			ioctl(lvds_misc.backlight_fd, PWMIOC_START);
		}
	} else
		return LVDS_RET_ERROR;
	return LVDS_RET_SUCCESS;
}

static void lvds_gpio_set_output(int padctl, bool val)
{
	struct lvds_set_gpio pad;
	if (padctl >= PINPAD_LVDS_DP0 && padctl < PINPAD_LVDS_MAX) {
		pad.padctl = padctl;
		pad.value = val;
		lvds_set_gpio_output(&pad);
	} else
		gpio_set_output(padctl, val);
}

static int lvds_set_gpio_backlight(unsigned long value)
{
	u32 i=0;
	if(lvds_misc.lcd_backlight_num == 0)
		return LVDS_RET_ERROR;

	for(i=0;i<lvds_misc.lcd_backlight_num;i++)
	{
		if(value==LVDS_GPIO_LOW)
			lvds_gpio_set_output(lvds_misc.lcd_backlight[i].padctl, lvds_misc.lcd_backlight[i].active);
		else
			lvds_gpio_set_output(lvds_misc.lcd_backlight[i].padctl, !lvds_misc.lcd_backlight[i].active);
		gpio_configure(lvds_misc.lcd_backlight[i].padctl, GPIO_DIR_OUTPUT);
	}
	return LVDS_RET_SUCCESS;
}

static int lvds_set_gpio_power(unsigned long value)
{
	u32 i=0;
	if(lvds_misc.lcd_power_num == 0)
		return LVDS_RET_ERROR;

	for(i=0;i<lvds_misc.lcd_power_num;i++)
	{
		if(value==LVDS_GPIO_LOW)
			lvds_gpio_set_output(lvds_misc.lcd_power[i].padctl, lvds_misc.lcd_power[i].active);
		else
			lvds_gpio_set_output(lvds_misc.lcd_power[i].padctl, !lvds_misc.lcd_power[i].active);
		gpio_configure(lvds_misc.lcd_power[i].padctl, GPIO_DIR_OUTPUT);
	}
	return LVDS_RET_SUCCESS;
}

static int lvds_get_channel_mode(lvds_channel_mode_e *val)
{
	if(val==NULL)
		return -1;

	*val = lvds_hal_get_channel_mode();
	log_d("val =%d\n",*val);
	return LVDS_RET_SUCCESS;
}
static int lvds_ioctl_get_info(lvds_info_t* info)
{
	if(info == NULL)
		return LVDS_RET_ERROR;

	memcpy(info, &lvds_misc.info, sizeof(lvds_info_t));
	return LVDS_RET_SUCCESS;
}

static bool is_lvds_working(void)
{
	u8 lvds_reg = 0;
	lvds_reg = lvds_hal_get_lane_clk_bias_en_reserve(1);
	log_d("%s %d lvds_reg = %d\n", __func__, __LINE__, lvds_reg);
	if (lvds_reg == 0x0)
		return 0; // no working
	else
		return 1; // working
}

static void lvds_set_wrok_status(bool val)
{
	if(val)
		lvds_hal_set_lane_clk_bias_en_reserve(1, 0x2);
	else
		lvds_hal_set_lane_clk_bias_en_reserve(1, 0x0);
}

static int lvds_ioctl_set_info(lvds_info_t* info)
{
	bool lvds_status = 0;
	if(info == NULL)
		return LVDS_RET_ERROR;

	memcpy(&lvds_misc.info, info, sizeof(lvds_info_t));
	lvds_status = is_lvds_working();
	lvds_hal_reset();

	if(lvds_status)
		lvds_set_wrok_status(1);

	config_lvds_ttl();

	if(!lvds_status)
		lvds_set_wrok_status(1);

	return LVDS_RET_SUCCESS;
}
static int lvds_ioctl(FAR struct file *file, int cmd, unsigned long arg)
{
	u32 val = (u32)arg;
	int ret = 0;
	switch (cmd){
	case LVDS_SET_CHANNEL_MODE:
		lvds_hal_set_channel_mode((lvds_channel_mode_e)val);
		break;
	case LVDS_SET_MAP_MODE:
		lvds_hal_set_data_map_format(val);
		break;
	case LVDS_SET_CH0_SRC_SEL:
		lvds_hal_set_ch0_src_sel(val);
		break;
	case LVDS_SET_CH1_SRC_SEL:
		lvds_hal_set_ch1_src_sel(val);
		break;
	case LVDS_SET_CH0_INVERT_CLK_SEL:
		lvds_hal_set_ch0_invert_clk_sel(val);
		break;
	case LVDS_SET_CH1_INVERT_CLK_SEL:
		lvds_hal_set_ch1_invert_clk_sel(val);
		break;
	case LVDS_SET_CH0_CLK_GATE:
		lvds_hal_set_ch0_clk_gate(val);
		break;
	case LVDS_SET_CH1_CLK_GATE:
		lvds_hal_set_ch1_clk_gate(val);
		break;
	case LVDS_SET_HSYNC_POLARITY:
		lvds_hal_set_hsync_polarity(val);
		break;
	case LVDS_SET_VSYNC_POLARITY:
		lvds_hal_set_vsync_polarity(val);
		break;
	case LVDS_SET_EVEN_ODD_ADJUST_MODE:
		lvds_hal_set_even_odd_adjust_mode(val);
		break;
	case LVDS_SET_EVEN_ODD_INIT_VALUE:
		lvds_hal_set_even_odd_init_value(val);
		break;
	case LVDS_SET_SRC_SEL:
		lvds_hal_set_src_sel(val, val);
		break;
	case LVDS_SET_RESET:
		lvds_hal_reset();
		break;
	case LVDS_SET_POWER_TRIGGER:
		lvds_lld_phy_mode_init(0, 0, 0);
		lvds_lld_phy_mode_init(1, 0, 0);
		break;
	case LVDS_SET_GPIO_OUT:
		lvds_set_gpio_output((struct lvds_set_gpio *)val);
		break;
	case LVDS_SET_PWM_BACKLIGHT:
		ret = lvds_set_pwm_backlight_duty(val);
		break;
	case LVDS_SET_GPIO_BACKLIGHT:
		ret = lvds_set_gpio_backlight(val);
		break;
	case LVDS_SET_PWM_VCOM:
		ret = lvds_set_pwm_vcom_duty(val);
		break;
	case LVDS_SET_GPIO_POWER:
		ret = lvds_set_gpio_power(val);
		break;
	case LVDS_SET_TRIGGER_EN:
		lvds_hal_set_trigger_en();
		break;
	case LVDS_GET_CHANNEL_MODE:
		lvds_get_channel_mode((lvds_channel_mode_e*)val);
		break;
	case LVDS_GET_CHANNEL_INFO:
		ret = lvds_ioctl_get_info((lvds_info_t *)val);
		break;
	case LVDS_SET_CHANNEL_INFO:
		ret = lvds_ioctl_set_info((lvds_info_t *)val);
		break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int lvds_close(struct file *filep)
{
	return 0;
}

static int lvds_open(struct file *filep)
{
	return 0;
}

static const struct file_operations lvds_fops = {
	.ioctl = lvds_ioctl,
	.open = lvds_open,
	.close = lvds_close,
	.read = dummy_read,
	.write = dummy_write,
};

static void ttl_pinmux_select_setting(int ttl_mode)
{
	int np = 0;
	struct pinmux_setting *active_state;
	for(int i = 0; i < 2; i++)
	{
		if(i == 0)
			np = fdt_node_probe_by_path("/hcrtos/lvds@0xb8860000");
		else
			np = fdt_node_probe_by_path("/soc/lvds@0xb8860000");

		if(np > 0)
		{
			if(ttl_mode == LVDS_CHANNEL_SEL_RGB888)
			{
				active_state = fdt_get_property_pinmux(np, "rgb888");
				if (active_state) {
					pinmux_select_setting(active_state);
					free(active_state);
				}
			}
			else if(ttl_mode == LVDS_CHANNEL_SEL_RGB666)
			{
				active_state = fdt_get_property_pinmux(np, "rgb666");
				if (active_state) {
					pinmux_select_setting(active_state);
					free(active_state);
				}
			}
		}
	}
}

static void config_lvds(uint8_t ch)
{
	int lvds_reset = 0;

	lvds_hal_set_src_sel(lvds_misc.info.lvds_cfg.src_sel, lvds_misc.info.lvds_cfg.src1_sel);

	lvds_lld_set_cfg(&lvds_misc.info.lvds_cfg,ch);
	if(lvds_misc.info.channel_type[1] == LVDS_CHANNEL_SEL_LVDS && lvds_misc.info.channel_type[0] != LVDS_CHANNEL_SEL_LVDS)
		lvds_reset = 1;

	lvds_lld_phy_mode_init(ch, lvds_misc.info.lvds_cfg.channel_mode, lvds_reset);
	lvds_lld_phy_enhance_driving_ability(ch, (lvds_drive_strength_e)lvds_misc.info.lvds_cfg.drive_strength);
}

static void config_ttl(uint8_t ch)
{
	hc_clk_enable(RGB_CLK);
	ttl_pinmux_select_setting(lvds_misc.info.channel_type[ch]);
	lvds_hal_rgb_clk_inv_sel(lvds_misc.info.ttl_cfg.rgb_clk_inv);

	rgb_hal_set_src_video_sel(lvds_misc.info.ttl_cfg.src_sel);

	rgb_hal_set_src_sel2(lvds_misc.info.ttl_cfg.rgb_src_sel[0], lvds_misc.info.ttl_cfg.rgb_src_sel[1], lvds_misc.info.ttl_cfg.rgb_src_sel[2]);
	lvds_io_ttl_sel_set(lvds_misc.info.channel_type[ch]);
	lvds_lld_phy_ttl_mode_init(ch);
	lvds_ttl_enhance_driving_ability(ch, (lvds_ttl_drive_strength_e)lvds_misc.info.ttl_cfg.drive_strength);
}


static int lvds_get_info(int np, int ch)
{
	u32 temp = 0;
	fdt_get_property_u_32_index(np, "channel-mode", 0, (u32 *)&lvds_misc.info.lvds_cfg.channel_mode);
	fdt_get_property_u_32_index(np, "map-mode", 0, (u32 *)&lvds_misc.info.lvds_cfg.map_mode);
	fdt_get_property_u_32_index(np, "ch1-src-sel", 0, (u32 *)&lvds_misc.info.lvds_cfg.ch1_src_sel);
	fdt_get_property_u_32_index(np, "ch1-invert-clk-sel", 0, (u32 *)&lvds_misc.info.lvds_cfg.ch1_invert_clk_sel);
	fdt_get_property_u_32_index(np, "ch0-src-sel", 0, (u32 *)&lvds_misc.info.lvds_cfg.ch0_src_sel);
	fdt_get_property_u_32_index(np, "ch0-invert-clk-sel", 0, (u32 *)&lvds_misc.info.lvds_cfg.ch0_invert_clk_sel);
	fdt_get_property_u_32_index(np, "hsync-polarity", 0, (u32 *)&lvds_misc.info.lvds_cfg.hsync_polarity);
	fdt_get_property_u_32_index(np, "vsync-polarity", 0, (u32 *)&lvds_misc.info.lvds_cfg.vsync_polarity);
	fdt_get_property_u_32_index(np, "even-odd-adjust-mode", 0, (u32 *)&lvds_misc.info.lvds_cfg.even_odd_adjust_mode);
	fdt_get_property_u_32_index(np, "even-odd-init-value", 0, (u32 *)&lvds_misc.info.lvds_cfg.even_odd_init_value);
	temp = LVDS_VIDEO_SRC_SEL_FXDE;
	fdt_get_property_u_32_index(np, "src-sel", 0, (u32 *)&temp);
	lvds_misc.info.lvds_cfg.src_sel = temp;
    fdt_get_property_u_32_index(np , "src1-sel" , 0 , (u32 *)&temp);
    lvds_misc.info.lvds_cfg.src1_sel = temp;
	fdt_get_property_u_32_index(np, "ch0-clk-gate", 0, (u32 *)&lvds_misc.info.lvds_cfg.ch0_clk_gate);
	fdt_get_property_u_32_index(np, "ch1-clk-gate", 0, (u32 *)&lvds_misc.info.lvds_cfg.ch1_clk_gate);
	temp = LVDS_LLD_PHY_DRIVE_STRENGTH_NORMAL;
	fdt_get_property_u_32_index(np, "lvds-drive-strength", 0, (u32 *)&temp);
	lvds_misc.info.lvds_cfg.drive_strength = temp;
	fdt_get_property_u_32_index(np, "chx-swap-ctrl", 0, (u32 *)&lvds_misc.info.lvds_cfg.chx_swap_ctrl);
	return 0;
}

static int ttl_get_dts_info(int np, const char *pin_type,uint8_t ch)
{
	int ret;
	u32 tmpVal = 0;
	const char *index_temp = pin_type;

	u32 i=0;
	lvds_misc.info.channel_type[ch] = LVDS_CHANNEL_SEL_RGB888;
	lvds_misc.info.ttl_cfg.src_sel = LVDS_RGB_SRC_SEL_FXDE;
	lvds_misc.info.ttl_cfg.rgb_src_sel[0] = LVDS_RGB_SRC_SHOW_R;
	lvds_misc.info.ttl_cfg.rgb_src_sel[1] = LVDS_RGB_SRC_SHOW_G;
	lvds_misc.info.ttl_cfg.rgb_src_sel[2] = LVDS_RGB_SRC_SHOW_B;

	if(index_temp != NULL && strcmp("rgb888", index_temp) == 0)
		lvds_misc.info.channel_type[ch] = LVDS_CHANNEL_SEL_RGB888;
	else if(index_temp != NULL && strcmp("rgb666", index_temp) == 0)
		lvds_misc.info.channel_type[ch] = LVDS_CHANNEL_SEL_RGB666;
	else if(index_temp != NULL && strcmp("rgb565", index_temp) == 0)
		lvds_misc.info.channel_type[ch] = LVDS_CHANNEL_SEL_RGB565;
	else if(index_temp != NULL && strcmp("i2so", index_temp) == 0)
		lvds_misc.info.channel_type[ch] = LVDS_CHANNEL_SEL_I2SO;
	else if(index_temp != NULL && strcmp("gpio", index_temp) == 0)
		lvds_misc.info.channel_type[ch] = LVDS_CHANNEL_SEL_LVDS_GPIO1;
	else 
		lvds_misc.info.channel_type[ch] = LVDS_CHANNEL_SEL_LVDS_GPIO1;

	if(lvds_misc.info.channel_type[ch] <= LVDS_CHANNEL_SEL_RGB565)
	{
		if(fdt_get_property_u_32_index(np, "rgb-clk-inv", 0, &tmpVal) == 0)
			lvds_misc.info.ttl_cfg.rgb_clk_inv = tmpVal;

		ret = fdt_get_property_u_32_index(np, "src-sel", 0, &tmpVal);
		if (ret == 0) {
			lvds_misc.info.ttl_cfg.src_sel = tmpVal;
		}

		lvds_misc.rgb_backlight.padctl = PINPAD_INVALID;
		lvds_misc.rgb_backlight.active = GPIO_ACTIVE_HIGH;
		ret = fdt_get_property_u_32_index(np, "rgb-backlight-gpios-rtos", 0, &tmpVal);
		if (ret == 0)
			lvds_misc.rgb_backlight.padctl = (pinpad_e)tmpVal;

		ret = fdt_get_property_u_32_index(np, "rgb-backlight-gpios-rtos", 1, &tmpVal);
		if (ret == 0)
			lvds_misc.rgb_backlight.active = (pinpad_e)tmpVal;

		gpio_configure(lvds_misc.rgb_backlight.padctl, GPIO_DIR_OUTPUT);
		if(lvds_misc.rgb_backlight.active == GPIO_ACTIVE_LOW)
			lvds_gpio_set_output(lvds_misc.rgb_backlight.padctl, 0);
		else
			lvds_gpio_set_output(lvds_misc.rgb_backlight.padctl, 1);

		fdt_get_property_string_index(np, "rgb-src-sel", 0, &index_temp);
		if(index_temp!=NULL&&strlen(index_temp)==3)
		{
			tmpVal=0;
			for(i=0;i<3;i++)
			{
				if(index_temp[i]=='r'){
					lvds_misc.info.ttl_cfg.rgb_src_sel[i] = LVDS_RGB_SRC_SHOW_R;
				}
				else if(index_temp[i]=='g'){
					lvds_misc.info.ttl_cfg.rgb_src_sel[i] = LVDS_RGB_SRC_SHOW_G;
				}
				else if(index_temp[i]=='b'){
					lvds_misc.info.ttl_cfg.rgb_src_sel[i] = LVDS_RGB_SRC_SHOW_B;
				}
			}
		}
	}

	tmpVal = LVDS_TTL_DRIVE_STRENGTH_NORMAL;
	fdt_get_property_u_32_index(np, "ttl-drive-strength", 0, (u32 *)&tmpVal);
	lvds_misc.info.ttl_cfg.drive_strength = tmpVal;
	return 0;
}

static int lvds_get_dts_info(int np)
{
	const char *screen_type = NULL;
	char st_buf[20]={0};
	u8 i;
	for(i=0;i<2;i++)
	{
		sprintf(st_buf,"lvds_ch%d-type",i);
		fdt_get_property_string_index(np, st_buf, 0, &screen_type);
		if(screen_type != NULL && strcmp("lvds", screen_type) == 0)
		{
			lvds_misc.info.channel_type[i] = LVDS_CHANNEL_SEL_LVDS;
			lvds_get_info(np, i);
		}
		else
			ttl_get_dts_info(np,screen_type,i);
	}
}

static int lvds_get_lcd_dts_info(int np)
{
	int ret;
	u32 tmpVal = 0;
	u32 i = 0;
	const char *path=NULL;

	if (fdt_get_property_data_by_name(np, "lcd-power-gpios-rtos", &lvds_misc.lcd_power_num) == NULL)
		lvds_misc.lcd_power_num = 0;

	lvds_misc.lcd_power_num >>= 3;

	if(lvds_misc.lcd_power_num > LCD_POWER_NUM_MAX)
		lvds_misc.lcd_power_num = LCD_POWER_NUM_MAX;

	for(i=0;i<lvds_misc.lcd_power_num;i++){
		lvds_misc.lcd_power[i].padctl = PINPAD_INVALID;
		lvds_misc.lcd_power[i].active = GPIO_ACTIVE_HIGH;

		if(fdt_get_property_u_32_index(np, "lcd-power-gpios-rtos", i * 2, &tmpVal)==0)
			lvds_misc.lcd_power[i].padctl = tmpVal;
		if(fdt_get_property_u_32_index(np, "lcd-power-gpios-rtos", i * 2 + 1, &tmpVal)==0)
			lvds_misc.lcd_power[i].active = tmpVal;
	}

	if (fdt_get_property_data_by_name(np, "lcd-backlight-gpios-rtos", &lvds_misc.lcd_backlight_num) == NULL)
		lvds_misc.lcd_backlight_num = 0;

	lvds_misc.lcd_backlight_num >>= 3;

	if(lvds_misc.lcd_backlight_num > LCD_BACKLIGHT_NUM_MAX)
		lvds_misc.lcd_backlight_num = LCD_BACKLIGHT_NUM_MAX;

	for(i=0;i<lvds_misc.lcd_backlight_num;i++){
		lvds_misc.lcd_backlight[i].padctl = PINPAD_INVALID;
		lvds_misc.lcd_backlight[i].active = GPIO_ACTIVE_HIGH;

		if(fdt_get_property_u_32_index(np, "lcd-backlight-gpios-rtos", i * 2, &tmpVal)==0)
			lvds_misc.lcd_backlight[i].padctl = tmpVal;
		if(fdt_get_property_u_32_index(np, "lcd-backlight-gpios-rtos", i * 2 + 1, &tmpVal)==0)
			lvds_misc.lcd_backlight[i].active = tmpVal;
	}

	if (fdt_get_property_string_index(np, "vcom-pwmdev", 0, &path)==0)
	{
		ret = fdt_get_property_u_32_index(np, "vcom-frequency", 0, &tmpVal);
		if(ret == 0)
			lvds_misc.pwm_vcom.period_ns = tmpVal;

		ret = fdt_get_property_u_32_index(np, "vcom-duty", 0, &tmpVal);
		if (ret == 0)
			lvds_misc.pwm_vcom.duty_ns = tmpVal;

		lvds_misc.vcom_fd = open(path, O_RDWR);
		if (lvds_misc.vcom_fd >= 0) {
			lvds_misc.pwm_vcom.polarity = 0;
			ioctl(lvds_misc.vcom_fd, PWMIOC_SETCHARACTERISTICS, &lvds_misc.pwm_vcom);
			ioctl(lvds_misc.vcom_fd, PWMIOC_START);
		}
		else
			lvds_misc.vcom_fd = -1;
	}
	else
		lvds_misc.vcom_fd = -1;

	if (fdt_get_property_string_index(np, "backlight-pwmdev", 0, &path)==0)
	{
		ret = fdt_get_property_u_32_index(np, "backlight-frequency", 0, &tmpVal);
		if(ret == 0)
			lvds_misc.pwm_backlight.period_ns = tmpVal;

		ret = fdt_get_property_u_32_index(np, "backlight-duty", 0, &tmpVal);
		if (ret == 0)
			lvds_misc.pwm_backlight.duty_ns = tmpVal;

		lvds_misc.backlight_fd = open(path, O_RDWR);
	}
	else
		lvds_misc.backlight_fd = -1;

	lvds_misc.default_off = fdt_property_read_bool(np, "default-off");
}

static void config_lvds_ttl(void)
{
	u8 i;
	for (i = 0; i < 2; i++) {
		if (lvds_misc.info.channel_type[i] == LVDS_CHANNEL_SEL_LVDS)
			config_lvds(i);
		else if (lvds_misc.info.channel_type[0] == LVDS_CHANNEL_SEL_LVDS &&
			 lvds_misc.info.channel_type[1] == LVDS_CHANNEL_SEL_LVDS &&
			 lvds_misc.info.lvds_cfg.channel_mode) {
			config_lvds(0);
			break;
		} else
			config_ttl(i);
	}
}

static int lvds_probe(const char *node)
{
	int np = fdt_node_probe_by_path(node);
	if(np < 0){
		return 0;
	}

	memset(&lvds_misc, 0, sizeof(struct hichip_lvds));

	lvds_misc.reg_base = (void *)((u32)0xB8860000);
	lvds_misc.sys_base = (void *)((u32)0xB8800000);
	fdt_get_property_u_32_index(np, "reg", 0, (u32 *)&lvds_misc.reg_base);
	fdt_get_property_u_32_index(np, "reg", 2, (u32 *)&lvds_misc.sys_base);

	lvds_misc.reg_base = (void *)((u32)lvds_misc.reg_base | 0xa0000000);
	lvds_misc.sys_base = (void *)((u32)lvds_misc.sys_base | 0xa0000000);
	// lvds_spi_display_init(NULL);
	lvds_lld_init(lvds_misc.reg_base, lvds_misc.sys_base, false);

	lvds_get_lcd_dts_info(np);
	lvds_get_dts_info(np);
	hc_clk_enable(LVDS_CH1_PIXEL_CLK);
	hc_clk_enable(LVDS_CLK);
	lvds_open_phy_clk();
	if (!is_lvds_working() && !lvds_misc.default_off)
	{
		/*reg init*/
		lvds_hal_reset();
		config_lvds_ttl();
		lvds_set_wrok_status(1);

		/*Turn off the backlight and turn it on after the logo is displayed*/
		for (u32 i = 0; i < lvds_misc.lcd_power_num; i++) {
			lvds_gpio_set_output(lvds_misc.lcd_power[i].padctl, !lvds_misc.lcd_power[i].active);
			gpio_configure(lvds_misc.lcd_power[i].padctl, GPIO_DIR_OUTPUT);
		}

		// if(lvds_misc.lcd_power_num>0)
		// 	usleep(100*1000);

		/*spi lcd init*/
		// lvds_display_init();
	}

	/*LVDS outputs high level in GPIO mode*/
	/*
	struct lvds_set_gpio pad={0,1};
	for(int i=0;i<32;i++)
	{
		lvds_set_gpio_output(&pad);
		pad.padctl++;
	}
	*/
lvds_register:
	return register_driver(MODULE_NAME, &lvds_fops, 0666, NULL);
}

static int lvds_init(void)
{
	lvds_probe("/hcrtos/lvds@0xb8860000");
	lvds_probe("/soc/lvds@0xb8860000");

	return 0;
}

static int lvds_exit(void)
{
	unregister_driver(MODULE_NAME);
}

module_driver(lvds, lvds_init, lvds_exit, 1)
