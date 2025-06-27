#include <generated/br2_autoconf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <hcuapi/fb.h>
#include <kernel/delay.h>
#include <kernel/types.h>
#include <kernel/fb.h>
#include <hcfota.h>
#include <hcuapi/pinpad.h>
#include <hcuapi/gpio.h>
#include <dt-bindings/gpio/gpio.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <common.h>

#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_SCREEN
static uint32_t y_location;
static int fd = 0;
static uint8_t *fb_base = NULL;
static struct fb_fix_screeninfo fix; /* Current fix */
static struct fb_var_screeninfo var; /* Current var */
static int color_bit = 32;
static uint32_t line_width;
static uint32_t pixel_width;
static uint32_t screen_size;
static hcfb_scale_t scale_param = { 1280, 720, 1920, 800 }; // to do later...
static int bar_init_rotate = 180;

static struct dis_rect {
	uint32_t x_start;
	uint32_t x_width;
	uint32_t y_start;
	uint32_t y_hight;
} dis_rect;

static void ap_fill_color(struct dis_rect *dis_rect, uint32_t color)
{
	uint32_t i = 0;
	uint32_t j = 0;
	int16_t *fb_dis_16b = NULL;
	int32_t *fb_dis_32b = NULL;
	int8_t *fb_dis_8b = NULL;
	int ret = 0;
	uint32_t x_end = 0;
	uint32_t y_end = 0;

	fb_dis_16b = (int16_t *)fb_base;
	fb_dis_8b = (int8_t *)fb_base;
	fb_dis_32b = (int32_t *)fb_base;
	x_end = dis_rect->x_start + dis_rect->x_width;
	if (x_end > var.xres) {
		x_end = var.xres;
	}

	y_end = dis_rect->y_start + dis_rect->y_hight;
	if (y_end > var.yres) {
		y_end = var.yres;
	}

	for (j = dis_rect->y_start; j < y_end; j++) {
		for (i = dis_rect->x_start; i < x_end; i++) {
			if (color_bit == 32) {
				fb_dis_32b[j * var.xres + i] = color;
			} else if (color_bit == 16) {
				fb_dis_16b[j * var.xres + i] = color;
			} else if (color_bit == 8) {
				fb_dis_8b[j * var.xres + i] = color;
			}
		}
	}
}

static int get_video_rotate(void)
{
	static int np = -1;
	const char *status;
	int ret = 0;
	int init_rotate = 180;

	np = fdt_get_node_offset_by_path("/hcrtos/rotate");
	if(np < 0){
		printf("%s:%d: fdt_get_node_offset_by_path failed\n", __func__, __LINE__);
		return -1;
	}

	ret = fdt_get_property_string_index(np, "status", 0, &status);
	if(ret != 0){
		printf("%s:%d: fdt_get_property_u_32_index status failed\n", __func__, __LINE__);
	}

	if(strcmp(status, "okay")){
		printf("%s:%d: rotate is disable\n", __func__, __LINE__);
		return -1;
	}

	ret = fdt_get_property_u_32_index(np, "rotate", 0, &init_rotate);
	if(ret != 0){
		printf("%s:%d: fdt_get_property_u_32_index rotate failed\n", __func__, __LINE__);
	}

	bar_init_rotate = init_rotate;

	printf("%s:%d: bar_init_rotate=%d\n", __func__, __LINE__, bar_init_rotate);

	return 0;
}

static int get_scale_param(void)
{
	static int np = -1;
	const char *status;
	u32 value = 0;
	int ret = 0;

	np = fdt_get_node_offset_by_path("/hcrtos/boot-fb0");
	if(np < 0){
		printf("%s:%d: fdt_get_node_offset_by_path failed\n", __func__, __LINE__);
		return -1;
	}

	ret = fdt_get_property_string_index(np, "status", 0, &status);
	if(ret != 0){
		printf("%s:%d: fdt_get_property_u_32_index status failed\n", __func__, __LINE__);
	}

	if(strcmp(status, "okay")){
		printf("%s:%d: rotate is disable\n", __func__, __LINE__);
		return -1;
	}

	ret = fdt_get_property_u_32_index(np, "scale", 0, &value);
	if(ret != 0){
		printf("%s:%d: fdt_get_property_u_32_index scale failed\n", __func__, __LINE__);
	}
	scale_param.h_div = (uint16_t)value;

	ret = fdt_get_property_u_32_index(np, "scale", 1, &value);
	if(ret != 0){
		printf("%s:%d: fdt_get_property_u_32_index scale failed\n", __func__, __LINE__);
	}
	scale_param.v_div = (uint16_t)value;

	ret = fdt_get_property_u_32_index(np, "scale", 2, &value);
	if(ret != 0){
		printf("%s:%d: fdt_get_property_u_32_index scale failed\n", __func__, __LINE__);
	}
	scale_param.h_mul = (uint16_t)value;

	ret = fdt_get_property_u_32_index(np, "scale", 3, &value);
	if(ret != 0){
		printf("%s:%d: fdt_get_property_u_32_index scale failed\n", __func__, __LINE__);
	}
	scale_param.v_mul = (uint16_t)value;

	printf("scale_param(%u, %u, %u, %u,)\n", scale_param.h_div, scale_param.v_div, scale_param.h_mul, scale_param.v_mul);

	return 0;
}

#if 0
static void fill_progress_bar(uint32_t len, uint32_t color)
{
	struct dis_rect show_rect;
	uint32_t dis_color = color;

	show_rect.x_start = 0;
	show_rect.x_width = len;
	show_rect.y_start = y_location / 2;
	show_rect.y_hight = y_location / 10;

	//printf("x(%lu, %lu), y(%lu, %lu)\n", show_rect.x_start, show_rect.x_width, show_rect.y_start, show_rect.y_hight);

	ap_fill_color(&show_rect, dis_color);
}
#else
static void fill_progress_bar(uint32_t len, uint32_t color)
{
	struct dis_rect show_rect = {0};
	uint32_t dis_color = color;

	switch(bar_init_rotate){
		case 0:
			show_rect.x_start = 0;
			show_rect.x_width = len;
			show_rect.y_start = var.yres / 2;
			show_rect.y_hight = var.yres / 10;
			break;

		case 90:
			show_rect.x_width = var.xres/10;
			show_rect.x_start = var.xres / 2 - (show_rect.x_width / 2);
			show_rect.y_start = var.yres-len;
			show_rect.y_hight = len;
			break;

		case 180:
			show_rect.x_start = var.xres-len;
			show_rect.x_width = len;
			show_rect.y_start = var.yres / 2;
			show_rect.y_hight = var.yres / 10;
			break;

		case 270:
			show_rect.x_width = var.xres / 10;
			show_rect.x_start = var.xres / 2 - (show_rect.x_width / 2);
			show_rect.y_start = 0;
			show_rect.y_hight = len;
			break;

		default:
			printf("%s:%d: unkow value, bar_init_rotate=%d\n", __func__, __LINE__, bar_init_rotate);
	}

	//printf("x(%lu, %lu), y(%lu, %lu)\n", show_rect.x_start, show_rect.x_width, show_rect.y_start, show_rect.y_hight);

	ap_fill_color(&show_rect,dis_color);
}
#endif

static int init_progress_bar(void)
{
	int ret = 0;

	get_video_rotate();
	get_scale_param();

	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0) {
		printf("Couldn't open /dev/fb0\n");
		return -1;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &var) == -1)
		printf("Error reading variable information");

	if (color_bit == 16) {
		var.bits_per_pixel = 16;
		var.red.length = 5;
		var.green.length = 6;
		var.blue.length = 5;
		var.yres_virtual = var.yres;
	} else if (color_bit == 32) {
		var.yoffset = 0;
		var.xoffset = 0;
		var.transp.length = 8;
		var.bits_per_pixel = 32;
		var.red.length = 8;
		var.green.length = 8;
		var.blue.length = 8;
		var.yres_virtual = var.yres;
	}
	if (ioctl(fd, FBIOPUT_VSCREENINFO, &var) == -1) {
		printf("Error reading variable information");
		return -1;
	}

	ioctl(fd, FBIOGET_FSCREENINFO, &fix);
	line_width = var.xres * var.bits_per_pixel / 8;
	pixel_width = var.bits_per_pixel / 8;
	screen_size = var.xres * var.yres * var.bits_per_pixel / 8;
	y_location = var.yres;

	if (ioctl(fd, HCFBIOSET_SCALE, &scale_param) != 0) {
		printf("ioctl(set scale param)");
		return -1;
	}

	ioctl(fd, HCFBIOSET_MMAP_CACHE, HCFB_MMAP_NO_CACHE);
	fb_base = (unsigned char *)mmap(
			NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (fb_base == MAP_FAILED) {
		printf("can't mmap\n");
		return -1;
	}

	memset(fb_base, 0x00, fix.smem_len);
	//Make sure that the display is on.
	if (ioctl(fd, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
		printf("%s:%d\n", __func__, __LINE__);
	}

    printf("var.xres=%lu, var.yres=%lu\n", var.xres, var.yres);

	if(bar_init_rotate == 0 || bar_init_rotate == 180){
		fill_progress_bar(var.xres, 0xffffffff);
	}else{
		fill_progress_bar(var.yres, 0xffffffff);
	}

#if defined(CONFIG_BOOT_LCD)
	open_boot_lcd_init(1,((char *[]){"boot_lcd"}));
#endif

#if defined(CONFIG_BOOT_BACKLIGHT)
	open_lcd_backlight(1,((char *[]){"backlight"}));
#endif

	return 0;
}

static int exit_progress_bar(void)
{
	if (fb_base != NULL)
		munmap(fb_base, fix.smem_len);
}

#endif

#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_LED
#define LED_OFF 0
#define LED_ON 1
#define LED_BLINK 2

typedef struct pinpad_gpio {
	pinpad_e padctl;
	bool active;
} pinpad_gpio_s;

pinpad_gpio_s led_gpio;

static TaskHandle_t show_led_t = { 0 };
static int led_flag = LED_OFF;

static void show_led(uint32_t time)
{
	gpio_set_output(led_gpio.padctl, GPIO_ACTIVE_HIGH);
	msleep(time);
	gpio_set_output(led_gpio.padctl, GPIO_ACTIVE_LOW);
	msleep(time);
}

static void show_led_main(void *pvParameters)
{
	while (1) {
		if (led_flag == LED_BLINK)
			show_led(700);
		else if (led_flag == LED_ON) {
			gpio_set_output(led_gpio.padctl, led_gpio.active);
			vTaskDelete(NULL);
		} else if (led_flag == LED_OFF)
			gpio_set_output(led_gpio.padctl, !led_gpio.active);
		msleep(1);
	}
}

static int led_init(void)
{
	u32 tmpVal = 0;
	int ret = 0;

	int np = fdt_node_probe_by_path("/hcrtos/upgrade_led");
	if (np < 0) {
		printf("no find led gpio\n");
		return -1;
	}

	led_gpio.padctl = PINPAD_INVALID;
	led_gpio.active = GPIO_ACTIVE_LOW;

	ret = fdt_get_property_u_32_index(np, "led-gpio", 0, &tmpVal);
	if (ret == 0)
		led_gpio.padctl = (pinpad_e)tmpVal;

	ret = fdt_get_property_u_32_index(np, "led-gpio", 1, &tmpVal);
	if (ret == 0)
		led_gpio.active = (pinpad_e)tmpVal;

	if (led_gpio.padctl != PINPAD_INVALID) {
		gpio_configure(led_gpio.padctl, GPIO_DIR_OUTPUT);
		gpio_set_output(led_gpio.padctl, GPIO_ACTIVE_HIGH);
	}

	xTaskCreate(show_led_main, (const char *)"show_led_main",
		    configTASK_STACK_DEPTH, NULL, portPRI_TASK_NORMAL,
		    (TaskHandle_t *)&show_led_t);

	return 0;
}
#endif

void upgrade_progress_init(void)
{
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_SCREEN
	init_progress_bar();
#endif
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_LED
	led_init();
#endif
}

void upgrade_progress_exit(int is_err)
{
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_SCREEN
	exit_progress_bar();
#endif
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_LED
	if (is_err)
		led_flag = LED_ON;
	else
		led_flag = LED_OFF;
#endif
}

static void upgrade_progress_report_to_serial(unsigned long param)
{
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_SERIAL
	printf("\n");
	unsigned char progress_sign[100 + 1];
	int per = param;
	int i;

	for (i = 0; i < 100; i++) {
		if (i < per) {
			progress_sign[i] = '=';
		} else if (per == i) {
			progress_sign[i] = '>';
		} else {
			progress_sign[i] = ' ';
		}
	}

	progress_sign[sizeof(progress_sign) - 1] = '\0';

	printf("\033[1A");
	fflush(stdout);
	printf("\033[K");
	fflush(stdout);
	printf("Upgrading: [%s] %3d%%", progress_sign, per);
	fflush(stdout);
#endif
}

static void upgrade_progress_report_to_screen(unsigned long param)
{
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_SCREEN
	uint32_t per = 0;

    if(bar_init_rotate == 0 || bar_init_rotate == 180){
		per = var.xres * param / 100;
    }else{
        per = var.yres * param / 100;
    }

	fill_progress_bar(per, 0xff0000ff);
#endif
}

static void upgrade_progress_report_to_led(unsigned long param)
{
#ifdef CONFIG_BOOT_UPGRADE_SHOW_WITH_LED
	if (param < 100) {
		led_flag = LED_BLINK;
	} else if (param >= 100) {
		led_flag = LED_OFF;
	}
#endif
}

int upgrade_progress_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata)
{
	static int progress_init = 0;

	if (progress_init == 0) {
		progress_init = 1;
		upgrade_progress_init();
	}

	if (event != HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS)
		return 0;

	upgrade_progress_report_to_serial(param);
	upgrade_progress_report_to_screen(param);
	upgrade_progress_report_to_led(param);
	return 0;
}
