#include <stdio.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "hc_lvgl_init.h"
#include "key.h"
#include "lv_hc_2dge.h"
#include <stdlib.h>

#ifdef __HCRTOS__
#include <kernel/types.h>
#include <freertos/FreeRTOS.h>
#endif
extern void hc_mouse_init(void);

static void my_monitor_cb(lv_disp_drv_t * disp_drv, uint32_t time, uint32_t px)
{
	/*printf("%d px refreshed in %d ms\n", px, time);*/
}

void *lv_mem_adr;
int lv_mem_size;

static void hc_clear_cb(struct _lv_disp_drv_t * disp_drv, uint8_t * buf, uint32_t size)
{
	/*
	 *  The memset here lead to 20fps decrease in *transparent screen* mode
	 *  No side-effect was found if removed the memset here
	 *  REVISIT: More test is required!!!
	 */
	/*lv_memset_00(buf, size * LV_IMG_PX_SIZE_ALPHA_BYTE);*/
}

extern void fbdev_wait_cb(struct _lv_disp_drv_t * disp_drv);

#ifndef CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN
static void hc_rounder_cb(struct _lv_disp_drv_t * disp_drv, lv_area_t * area)
{
	int w = lv_area_get_width(area);
	int h = lv_area_get_height(area);
	if(w != LV_HOR_RES && w % 2 == 0) {
		if(area->x1 > 0)
			area->x1 -= 1;
		else
			area->x2 += 1;
	}
	if(h != LV_VER_RES && h % 2 == 0) {
		if(area->y1 > 0)
			area->y1 -= 1;
		else
			area->y2 += 1;
	}
}
#endif

static void lv_draw_hc_ctx_init_sw(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
	lv_draw_sw_init_ctx(drv, draw_ctx);
}

int hc_lvgl_init(void)
{
	/*Linux frame buffer device init*/
	fbdev_init();

	lv_color_t *buf1 = NULL;
	lv_color_t *buf2 = NULL;

#ifdef CONFIG_LV_HC_MEM_FROM_MALLOC
	buf1 = malloc(LV_HC_DRAW_BUF_SIZE * sizeof(lv_color_t));
#else
	buf1 = fbdev_static_malloc_virt(LV_HC_DRAW_BUF_SIZE * sizeof(lv_color_t));
#endif
	if(!buf1) {
		LV_LOG_ERROR("malloc buf1 fail!Not enough memory.\n");
		return -1;
	}

#if LV_HC_DRAW_BUF_COUNT == 2
#ifdef CONFIG_LV_HC_MEM_FROM_MALLOC
	buf2 = malloc(LV_HC_DRAW_BUF_SIZE * sizeof(lv_color_t));
#else
	buf2 = fbdev_static_malloc_virt(LV_HC_DRAW_BUF_SIZE * sizeof(lv_color_t));
#endif
	if(!buf2) {
		LV_LOG_ERROR("malloc buf2 fail!Not enough memory.\n");
		return -1;
	}
#endif


#ifdef CONFIG_LV_HC_MEM_FROM_MALLOC
	lv_mem_size = CONFIG_LV_MEM_SIZE_KILOBYTES * 1024;
	lv_mem_adr = malloc(lv_mem_size);
#else
	/* Init lvgl alloc memory */
	lv_mem_size = fbdev_get_buffer_size() & ~0x1F;
	lv_mem_adr = fbdev_static_malloc_virt(lv_mem_size);
#endif
	printf("lv_mem_adr: %p, lv_mem_size: %d\n", lv_mem_adr, lv_mem_size);

	/*LittlevGL init*/
	lv_init();
	printf("%s:%d\n", __func__, __LINE__);

	/*Initialize a descriptor for the buffer*/
	static lv_disp_draw_buf_t disp_buf;

	if(!buf1 && !buf2) {
		LV_LOG_ERROR("Need configure HC_LV_DRAW_BUF1 and HC_LV_DRAW_BUF2\n");
		return -1;
	}

	lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LV_HC_DRAW_BUF_SIZE);
	printf("buf1 = %p, buf2: = %p, size = %d\n", buf1, buf2, LV_HC_DRAW_BUF_SIZE);

	/*Initialize and register a display driver*/
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.draw_buf   = &disp_buf;
	disp_drv.flush_cb   = fbdev_flush;
	disp_drv.hor_res    = LV_HC_SCREEN_HOR_RES;//1920;
	disp_drv.ver_res    = LV_HC_SCREEN_VER_RES;//1080;
	disp_drv.full_refresh = 0;
	disp_drv.direct_mode = 0;
	disp_drv.screen_transp = 1;
#if LV_USE_GPU_HICHIP
#ifndef CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN
	disp_drv.rounder_cb = hc_rounder_cb;
#endif
	disp_drv.wait_cb = fbdev_wait_cb;
	disp_drv.clear_cb = hc_clear_cb;
	disp_drv.draw_ctx_init = lv_draw_hc_ctx_init;
	disp_drv.draw_ctx_deinit = lv_draw_hc_ctx_deinit;
	disp_drv.draw_ctx_size = sizeof(lv_draw_hc_ctx_t);
#else
	disp_drv.draw_ctx_init = lv_draw_hc_ctx_init;
	disp_drv.draw_ctx_size = sizeof(lv_draw_sw_ctx_t);
#endif
	lv_disp_drv_register(&disp_drv);
	printf("lv_disp_register: w-%d,h-%d\n",disp_drv.hor_res,disp_drv.ver_res);

#if USE_EVDEV
	hc_mouse_init();
#endif

#if LV_HC_IR != 0
	key_init();
#endif

#if CONFIG_LV_USE_HC_TP_MONKEY != 0 && LV_USE_MONKEY != 0
	/*Create pointer monkey test*/
	lv_monkey_config_t config;
	lv_monkey_config_init(&config);
	config.type = LV_INDEV_TYPE_POINTER;
	config.period_range.min = 10;
	config.period_range.max = 100;
	lv_monkey_t * monkey = lv_monkey_create(&config);

	/*Start monkey test*/
	lv_monkey_set_enable(monkey, true);
#endif

	return 0;
}

int lv_hc_init_ex(lv_hc_conf_t *conf)
{
	/*Linux frame buffer device init*/
	fbdev_init_ext(conf);

	lv_color_t *buf1 = NULL;
	lv_color_t *buf2 = NULL;

	if (!conf->buf_from) {
#ifdef CONFIG_LV_HC_MEM_FROM_MALLOC
		buf1 = malloc(LV_HC_DRAW_BUF_SIZE * sizeof(lv_color_t));
#else
		buf1 = fbdev_static_malloc_virt(LV_HC_DRAW_BUF_SIZE * sizeof(lv_color_t));
#endif

		if(!buf1) {
			LV_LOG_ERROR("malloc buf1 fail!Not enough memory.\n");
			return -1;
		}

#if LV_HC_DRAW_BUF_COUNT == 2
#ifdef CONFIG_LV_HC_MEM_FROM_MALLOC
		buf2 = malloc(LV_HC_DRAW_BUF_SIZE * sizeof(lv_color_t));
#else
		buf2 = fbdev_static_malloc_virt(LV_HC_DRAW_BUF_SIZE * sizeof(lv_color_t));
#endif
		if(!buf2) {
			LV_LOG_ERROR("malloc buf2 fail!Not enough memory.\n");
			return -1;
		}
#endif

#ifdef CONFIG_LV_HC_MEM_FROM_MALLOC
		lv_mem_size = CONFIG_LV_MEM_SIZE_KILOBYTES * 1024;
		lv_mem_adr = malloc(lv_mem_size);
#else
		/* Init lvgl alloc memory */
		lv_mem_size = fbdev_get_buffer_size() & ~0x1F;
		lv_mem_adr = fbdev_static_malloc_virt(lv_mem_size);
#endif
	} else {
		if (conf->buf_from == LV_HC_HEAP_BUFFER_FROM_FB)
			buf1 = fbdev_static_malloc_virt(conf->buf1_size > 0 ? conf->buf1_size: conf->hor_res * conf->ver_res * sizeof(lv_color_t));
		else
			buf1 = malloc(conf->buf1_size > 0 ? conf->buf1_size: conf->hor_res * conf->ver_res * sizeof(lv_color_t));
		if(!buf1) {
			LV_LOG_ERROR("malloc buf1 fail!Not enough memory.\n");
			return -1;
		}

		if (conf->buf2_size > 0) {
			if (conf->buf_from == LV_HC_HEAP_BUFFER_FROM_FB)
				buf2 = fbdev_static_malloc_virt(conf->buf2_size > 0 ? conf->buf2_size: conf->hor_res * conf->ver_res * sizeof(lv_color_t));
			else
				buf2 = malloc(conf->buf2_size > 0 ? conf->buf2_size: conf->hor_res * conf->ver_res * sizeof(lv_color_t));
			if(!buf2) {
				LV_LOG_ERROR("malloc buf2 fail!Not enough memory.\n");
				return -1;
			}
		}

		if (conf->buf_from == LV_HC_HEAP_BUFFER_FROM_FB) {
			/* Init lvgl alloc memory */
			lv_mem_size = fbdev_get_buffer_size() & ~0x1F;
			lv_mem_adr = fbdev_static_malloc_virt(lv_mem_size);
		} else {
			lv_mem_size = (conf->heap_size > 0)? conf->heap_size: CONFIG_LV_MEM_SIZE_KILOBYTES * 1024;
			lv_mem_adr = malloc(lv_mem_size);
		}
	}

	printf("lv_mem_adr: %p, lv_mem_size: %d\n", lv_mem_adr, lv_mem_size);

	/*LittlevGL init*/
	lv_init();
	printf("%s:%d\n", __func__, __LINE__);

	/*Initialize a descriptor for the buffer*/
	static lv_disp_draw_buf_t disp_buf;

	if(!buf1 && !buf2) {
		LV_LOG_ERROR("Need configure HC_LV_DRAW_BUF1 and HC_LV_DRAW_BUF2\n");
		return -1;
	}

	lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LV_HC_DRAW_BUF_SIZE);
	printf("buf1 = %p, buf2: = %p, size = %d\n", buf1, buf2, LV_HC_DRAW_BUF_SIZE);

	/*Initialize and register a display driver*/
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.draw_buf   = &disp_buf;
	disp_drv.flush_cb   = fbdev_flush;
	disp_drv.full_refresh = 0;
	disp_drv.direct_mode = 0;
	disp_drv.screen_transp = 1;
	if (conf->hor_res == 0)
		disp_drv.hor_res = LV_HC_SCREEN_HOR_RES;
	else
		disp_drv.hor_res = conf->hor_res;
	if (conf->ver_res == 0)
		disp_drv.ver_res    = LV_HC_SCREEN_VER_RES;
	else
		disp_drv.ver_res = conf->ver_res;
	if (conf->sw_rotated > 0) {
		disp_drv.sw_rotate = 1;
		disp_drv.rotated = conf->sw_rotated / 90;
	}

	if (!conf->gpu) {
#if LV_USE_GPU_HICHIP
#ifndef CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN
		disp_drv.rounder_cb = hc_rounder_cb;
#endif
		disp_drv.wait_cb = fbdev_wait_cb;
		disp_drv.clear_cb = hc_clear_cb;
		disp_drv.draw_ctx_init = lv_draw_hc_ctx_init;
		disp_drv.draw_ctx_deinit = lv_draw_hc_ctx_deinit;
		disp_drv.draw_ctx_size = sizeof(lv_draw_hc_ctx_t);
#else

		disp_drv.flush_cb   = fbdev_flush_sw;
		disp_drv.draw_ctx_init = lv_draw_hc_ctx_init_sw;
		disp_drv.draw_ctx_size = sizeof(lv_draw_sw_ctx_t);
#endif
	} else {
		if (conf->gpu == LV_HC_GPU_GE) {
#ifndef CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN
			disp_drv.rounder_cb = hc_rounder_cb;
#endif
			disp_drv.wait_cb = fbdev_wait_cb;
			disp_drv.clear_cb = hc_clear_cb;
			disp_drv.draw_ctx_init = lv_draw_hc_ctx_init;
			disp_drv.draw_ctx_deinit = lv_draw_hc_ctx_deinit;
			disp_drv.draw_ctx_size = sizeof(lv_draw_hc_ctx_t);
		} else {
			disp_drv.flush_cb   = fbdev_flush_sw;
			disp_drv.draw_ctx_init = lv_draw_hc_ctx_init_sw;
			disp_drv.draw_ctx_size = sizeof(lv_draw_sw_ctx_t);
		}
	}

	lv_disp_drv_register(&disp_drv);
	printf("lv_disp_register: w-%d,h-%d\n",disp_drv.hor_res,disp_drv.ver_res);

#if USE_EVDEV
	hc_mouse_init();
#endif

#if LV_HC_IR != 0
	key_init();
#endif

#if CONFIG_LV_USE_HC_TP_MONKEY != 0 && LV_USE_MONKEY != 0
	/*Create pointer monkey test*/
	lv_monkey_config_t config;
	lv_monkey_config_init(&config);
	config.type = LV_INDEV_TYPE_POINTER;
	config.period_range.min = 10;
	config.period_range.max = 100;
	lv_monkey_t * monkey = lv_monkey_create(&config);

	/*Start monkey test*/
	lv_monkey_set_enable(monkey, true);
#endif

	return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
#ifdef __HCRTOS__
	return xTaskGetTickCount()/portTICK_PERIOD_MS;
#else
	struct timespec tv_now;
	assert(clock_gettime(CLOCK_BOOTTIME, &tv_now) == 0);
	return tv_now.tv_sec * 1000 + tv_now.tv_nsec / 1000000;
#endif
}

#if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )
uint32_t hc_cpu_usage_get(void)
{
	static uint32_t nowtime      = 0;
	static uint32_t oldtime      = 0;
	static uint32_t oldtotaltime = 0;
	static uint32_t nowtotaltime = 0;
	static uint32_t cpuusage     = 0;

	oldtotaltime = nowtotaltime;
	nowtotaltime = portGET_RUN_TIME_COUNTER_VALUE();
	oldtime      = nowtime;
	nowtime      = xTaskGetIdleRunTimeCounter();
	cpuusage = 100 - (((nowtime - oldtime) * 100) / (nowtotaltime - oldtotaltime));
	return cpuusage;
}
#else
uint32_t hc_cpu_usage_get(void)
{
	uint32_t cpu = 100 - lv_timer_get_idle();
	return cpu;
}
#endif

int hc_lvgl_loop(int wait_time, hc_lvgl_loop_cb cb, void *data)
{
	if(wait_time == 0)
		wait_time = 1*1000;
	else
		wait_time *= 1000;
	while(1) {
		if(cb && cb(data) != 0)
			break;
		lv_task_handler();
		usleep(wait_time);
	}

	return 0;
}
