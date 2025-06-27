#ifndef _H_HC_LVGL_INIT_H
#define _H_HC_LVGL_INIT_H
#include <stdint.h>
#include <stdbool.h>

#define LV_HC_DEFCONFIG 	0

#define LV_HC_FB_COLOR_DEPTH_32_ARGB 1
#define LV_HC_FB_COLOR_DEPTH_16_ARGB1555 2
#define LV_HC_FB_COLOR_DEPTH_16_ARGB4444 3

#define LV_HC_SINGLE_BUFFER 	1
#define LV_HC_DUAL_BUFFER 	2

#define LV_HC_HEAP_BUFFER_FROM_FB 	1
#define LV_HC_HEAP_BUFFER_FROM_MALLOC 	2

#define LV_HC_GPU_SW 1
#define LV_HC_GPU_GE 2

typedef struct lv_hc_conf {
	/* /dev/fb0 or /dev/fb1 */
	char fbdev_path[64];
	uint16_t hor_res;
	uint16_t ver_res;
	uint8_t gpu;
	uint32_t buf1_size;
	uint32_t buf2_size;
	uint32_t heap_size;
	/* 0: use lvgl default config, 0: from framebuffer, 1: from system malloc */
	uint8_t buf_from;
	char dts_cfg_path[64];
	/* support 0, 90, 180, 270 */
	uint16_t sw_rotated;
	int color_depth;
	bool hotplug_support;
	bool no_cached;
	uint8_t fb_buf_num;
}lv_hc_conf_t;

uint32_t custom_tick_get(void);
int hc_lvgl_init(void);
int lv_hc_init_ex(lv_hc_conf_t *conf);
uint32_t hc_cpu_usage_get(void);
typedef int (*hc_lvgl_loop_cb)(void *data);
//wait_time in millisecone
int hc_lvgl_loop(int wait_time, hc_lvgl_loop_cb cb, void *data);
#endif
