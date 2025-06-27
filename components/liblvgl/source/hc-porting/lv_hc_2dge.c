/**
 * @file lv_gpu_hichip.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../core/lv_refr.h"
#include <hcge/ge_api.h>
#include <stdio.h>
#include <pthread.h>
#ifdef __HCRTOS__
#include <cpu_func.h>
#else
#include <asm/cachectl.h>
#endif
#include "lv_drivers/display/fbdev.h"
#include "lv_hc_2dge.h"

#if LV_USE_GPU_HICHIP

static bool ge_rect_fill(lv_color_t *dest_buf, lv_coord_t dest_stride,
                         const lv_area_t *fill_area, lv_color_t color,
                         unsigned char opa, int set);
static void lv_draw_hc_img_decoded(lv_draw_ctx_t *draw, const lv_draw_img_dsc_t *dsc,
                                   const lv_area_t *coords, const uint8_t *map_p,
                                   lv_img_cf_t color_format);

hcge_context *hcge_ctx = NULL;

#define HC_2DGE_IMG_DBG 0
#define HC_2DGE_MUTEX_ENABLE 0

#if HC_2DGE_MUTEX_ENABLE != 0
static pthread_mutex_t hcge_mutex;
#define LV_HCGE_LOCK() pthread_mutex_lock(&hcge_mutex)
#define LV_HCGE_UNLOCK() pthread_mutex_unlock(&hcge_mutex)
#else
#define LV_HCGE_LOCK()
#define LV_HCGE_UNLOCK()
#endif
#if 0
#define GE_WAIT() do {	\
	lv_ge_lock(); \
	hcge_engine_sync(hcge_ctx); \
	lv_ge_unlock(); \
} while(0)
#else
#define GE_WAIT() do { }while(0)
#endif

void lv_ge_lock(void)
{
	LV_HCGE_LOCK();
}

void lv_ge_unlock(void)
{
	LV_HCGE_UNLOCK();
}

static void lv_draw_hc_deinit(void)
{
	if (hcge_ctx) {
		hcge_close(hcge_ctx);
		hcge_ctx = NULL;
	}

#if HC_2DGE_MUTEX_ENABLE != 0
	pthread_mutex_destroy(&hcge_mutex);
#endif
}

static int bytes_per_pixel(HCGESurfacePixelFormat fmt)
{
	int bytes = 0;
	switch(fmt) {
	case HCGE_DSPF_ARGB1555:
	case HCGE_DSPF_RGB16:
	case HCGE_DSPF_ARGB4444:
		bytes = 2;
		break;
	case HCGE_DSPF_ARGB:
		bytes = 4;
		break;
	default:
		printf("%s:Don't support color format.\n", __func__);
		break;
	}
	return bytes;

}

/**
 * Turn on the peripheral and set output color mode, this only needs to be done once
 */
int lv_draw_hc_init(void)
{
	if (!hcge_ctx && hcge_open(&hcge_ctx) != 0) {
		printf("Init hcge error.\n");
		return -1;
	}

#if HC_2DGE_MUTEX_ENABLE != 0
	pthread_mutex_init(&hcge_mutex, NULL);
#endif

	/*hcge_ctx->log_en = true;*/
	return 0;
}

void lv_draw_hc_ctx_init(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
	lv_draw_sw_init_ctx(drv, draw_ctx);
	if(lv_draw_hc_init() != 0) {
		return;
	}

#if (LV_HC_DRAW_BUF_SIZE >= (LV_HC_SCREEN_HOR_RES * LV_HC_SCREEN_VER_RES))
	lv_draw_hc_ctx_t *hc_draw_ctx = (lv_draw_sw_ctx_t *)draw_ctx;
	hc_draw_ctx->blend = lv_draw_hc_blend;
	hc_draw_ctx->base_draw.draw_img_decoded = lv_draw_hc_img_decoded;
	hc_draw_ctx->base_draw.wait_for_finish = lv_draw_hc_wait_cb;
	hc_draw_ctx->base_draw.buffer_copy = lv_draw_hc_buffer_copy;
#endif
}

void lv_draw_hc_ctx_deinit(lv_disp_drv_t *drv, lv_draw_ctx_t *draw_ctx)
{
	LV_UNUSED(drv);
	LV_UNUSED(draw_ctx);
	lv_draw_hc_deinit();
}

void lv_draw_hc_blend(lv_draw_ctx_t *draw_ctx, const lv_draw_sw_blend_dsc_t *dsc)
{
	lv_area_t blend_area;
	bool done = false;

	if(dsc->mask_buf && dsc->mask_res == LV_DRAW_MASK_RES_TRANSP) return;
	if(!_lv_area_intersect(&blend_area, dsc->blend_area, draw_ctx->clip_area)) return;

	if (dsc->mask_buf == NULL && dsc->blend_mode == LV_BLEND_MODE_NORMAL && dsc->opa >= LV_OPA_MAX) {
		lv_coord_t dest_stride = lv_area_get_width(draw_ctx->buf_area);
		lv_color_t *dest_buf = draw_ctx->buf;
		const lv_color_t *src_buf = dsc->src_buf;

		if (src_buf) {
			lv_coord_t src_stride;
			lv_area_t src_area;
			struct lv_blit blit = { 0 };
			src_stride = lv_area_get_width(dsc->blend_area);
			lv_area_copy(&src_area, &blend_area);
			lv_area_move(&src_area, -dsc->blend_area->x1, -dsc->blend_area->y1);
			lv_area_move(&blend_area, -draw_ctx->buf_area->x1,-draw_ctx->buf_area->y1);
			blit.dst_buf = dest_buf;
			blit.dst_area = &blend_area;
			blit.dst_stride = dest_stride;
			blit.src_buf = src_buf;
			blit.src_stride = src_stride;
			blit.src_area = &src_area;
			blit.opa = dsc->opa;

#if LV_COLOR_DEPTH == 16
			blit.dst_fmt = HCGE_DSPF_RGB16;
			blit.src_fmt = HCGE_DSPF_RGB16;
#elif LV_COLOR_DEPTH == 32
			blit.dst_fmt = HCGE_DSPF_ARGB;
			blit.src_fmt = HCGE_DSPF_ARGB;
#else
#error "Don't support color format"
#endif

			if (lv_ge_blit(&blit, true))
				done = true;
		} else {
			lv_area_move(&blend_area, -draw_ctx->buf_area->x1,
			             -draw_ctx->buf_area->y1);
			if (ge_rect_fill(dest_buf, dest_stride, &blend_area,
			                 dsc->color, dsc->opa, 0)) {
				done = true;
			}
		}
	}

	if (!done)
		lv_draw_sw_blend_basic(draw_ctx, dsc);
}

void lv_draw_hc_buffer_copy(lv_draw_ctx_t *draw_ctx, void *dest_buf,
                            lv_coord_t dest_stride,
                            const lv_area_t *dest_area, void *src_buf,
                            lv_coord_t src_stride,
                            const lv_area_t *src_area)
{
	struct lv_blit blit = { 0 };
	blit.dst_buf = dest_buf;
	blit.dst_area = dest_area;
	blit.dst_stride = dest_stride;
	blit.src_buf = src_buf;
	blit.src_stride = src_stride;
	blit.src_area = src_area;
	blit.opa = LV_OPA_MAX;

#if LV_COLOR_DEPTH == 16
	blit.dst_fmt = HCGE_DSPF_RGB16;
	blit.src_fmt = HCGE_DSPF_RGB16;
#elif LV_COLOR_DEPTH == 32
	blit.dst_fmt = HCGE_DSPF_ARGB;
	blit.src_fmt = HCGE_DSPF_ARGB;
#else
#error "Don't support color format"
#endif

	if (!lv_ge_blit(&blit, true))
		lv_draw_sw_buffer_copy(draw_ctx, dest_buf, dest_stride,
		                       dest_area, src_buf, src_stride,
		                       src_area);
}

#if HC_2DGE_IMG_DBG != 0
static void print_area(char *prefix, lv_area_t *area)
{
	if(prefix)
		printf("%s:", prefix);
	printf("x1:%d, y1:%d, x2: %d, y2: %d, w=%d,h=%d\n", area->x1, area->y1, area->x2, area->y2,
	       area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
}
#endif

static bool _ge_blit(lv_draw_ctx_t *draw_ctx,
                     const lv_draw_img_dsc_t *draw_dsc,
                     lv_draw_sw_blend_dsc_t *blend_dsc)
{
	lv_coord_t src_stride;
	lv_coord_t dst_stride;
	lv_area_t src_area;
	lv_area_t blend_area;
	lv_area_t clip_area;
	int32_t w = lv_area_get_width(blend_dsc->blend_area);
	int32_t h = lv_area_get_height(blend_dsc->blend_area);

	/*
	 * drawing area is less than 240 pixel, use sw draw route
	 */
	if(w * h < 240) {
		return 0;
	}

	if(draw_dsc->angle > 0) {
		lv_disp_t * disp = _lv_refr_get_disp_refreshing();
		lv_disp_draw_buf_t * draw_buf = lv_disp_get_draw_buf(disp);
		bool full_sized = draw_buf->size == (uint32_t)disp->driver->hor_res * disp->driver->ver_res;
		if(!full_sized) {
			printf("Unsupported rotate operate. Must configure draw buffer equal to "
			       "%d x %d\n", disp->driver->hor_res, disp->driver->ver_res);
			return false;
		}

		/*
		 * currently, not support no rectangle rotate
		 */
		if(w != h) {
			return false;
		}
	}

	lv_area_copy(&blend_area, blend_dsc->blend_area);
	if(draw_dsc->zoom != LV_IMG_ZOOM_NONE) {
	
		_lv_img_buf_get_transformed_area(&blend_area, w, h, draw_dsc->angle, draw_dsc->zoom, &draw_dsc->pivot);

		blend_area.x1 += blend_dsc->blend_area->x1;
		blend_area.y1 += blend_dsc->blend_area->y1;
		blend_area.x2 += blend_dsc->blend_area->x1;
		blend_area.y2 += blend_dsc->blend_area->y1;
	}

	src_stride = w;
	dst_stride = lv_area_get_width(draw_ctx->buf_area);
	lv_area_copy(&src_area, blend_dsc->blend_area);
	lv_area_move(&src_area, -blend_dsc->blend_area->x1, -blend_dsc->blend_area->y1);
	lv_area_move(&blend_area, -draw_ctx->buf_area->x1,-draw_ctx->buf_area->y1);
	lv_area_copy(&clip_area, draw_ctx->clip_area);
	lv_area_move(&clip_area, -draw_ctx->buf_area->x1, -draw_ctx->buf_area->y1);

#if HC_2DGE_IMG_DBG != 0
	if(true || draw_dsc->angle > 0) {
		printf("angle: %d, pivot_x: %d, pivot_y: %d\n", draw_dsc->angle, draw_dsc->pivot.x, draw_dsc->pivot.y);
		print_area("buf_area:", draw_ctx->buf_area);
		print_area("clip_area:", &clip_area);
		print_area("blend_area:", &blend_area);
		print_area("src_area:", &src_area);
	}
#endif

	struct lv_blit blit = { 0 };
	blit.dst_buf = draw_ctx->buf;
	blit.dst_area = &blend_area;
	blit.dst_stride = dst_stride;
	blit.src_buf = blend_dsc->src_buf;
	blit.src_stride = src_stride;
	blit.src_area = &src_area;
	blit.opa = LV_OPA_MAX;
	if(draw_dsc->angle > 0) {
		blit.angle = (3600 - draw_dsc->angle % 3600) % 3600 / 10.0;
	}
	blit.crop = &clip_area;
	blit.pivot = &draw_dsc->pivot;
	blit.stretch = 1;

#if LV_COLOR_DEPTH == 16
	blit.dst_fmt = HCGE_DSPF_RGB16;
	blit.src_fmt = HCGE_DSPF_RGB16;
#elif LV_COLOR_DEPTH == 32
	blit.dst_fmt = HCGE_DSPF_ARGB;
	blit.src_fmt = HCGE_DSPF_ARGB;
#else
#error "Don't support color format"
#endif

	return lv_ge_blit(&blit, true);
}

LV_ATTRIBUTE_FAST_MEM void lv_draw_hc_img_decoded(struct _lv_draw_ctx_t *draw_ctx,
        const lv_draw_img_dsc_t *draw_dsc,
        const lv_area_t *coords,
        const uint8_t *src_buf,
        lv_img_cf_t cf)
{
	lv_area_t draw_area;
	bool mask_any;
	lv_draw_sw_blend_dsc_t blend_dsc;
	bool done = false;

	lv_area_copy(&draw_area, draw_ctx->clip_area);
	mask_any = lv_draw_mask_is_any(&draw_area);

	if (!mask_any && (cf == LV_IMG_CF_TRUE_COLOR || cf == LV_IMG_CF_TRUE_COLOR_ALPHA)
	    && draw_dsc->recolor_opa == LV_OPA_TRANSP && draw_dsc->opa >= LV_OPA_MAX
	    && draw_dsc->blend_mode == LV_BLEND_MODE_NORMAL) {
		lv_memset_00(&blend_dsc, sizeof(lv_draw_sw_blend_dsc_t));
		blend_dsc.src_buf = (lv_color_t *)src_buf;
		blend_dsc.blend_area = coords;
		blend_dsc.opa = draw_dsc->opa;
		blend_dsc.blend_mode = draw_dsc->blend_mode;

		if (blend_dsc.mask_buf == NULL &&
		    blend_dsc.blend_mode == LV_BLEND_MODE_NORMAL) {
			if ((draw_dsc->angle > 0 &&
			     draw_dsc->zoom == LV_IMG_ZOOM_NONE) ||
			    draw_dsc->angle == 0) {
				done = _ge_blit(draw_ctx, draw_dsc, &blend_dsc);
			}
		}
	}
	if(!done)
		lv_draw_sw_img_decoded(draw_ctx, draw_dsc, coords, src_buf, cf);
}

static bool ge_rect_fill(lv_color_t *dest_buf, lv_coord_t dest_stride,
                         const lv_area_t *fill_area, lv_color_t color,
                         unsigned char opa, int set)
{
	/*Simply fill an area*/
	int32_t area_w = lv_area_get_width(fill_area);
	int32_t area_h = lv_area_get_height(fill_area);
	uint32_t size = (fill_area->x2 + fill_area->y2 * dest_stride) *
	                sizeof(lv_color_t);

	if (!fbdev_check_addr((uint32_t)dest_buf, size))
		return false;

	hcge_state *state = &hcge_ctx->state;
	lv_ge_lock();
	hcge_state_init(state);

	if (opa < LV_OPA_MAX && !set)
		state->blittingflags = HCGE_DSBLIT_BLEND_ALPHACHANNEL;

	state->src_blend = HCGE_DSBF_SRCALPHA;
	state->dst_blend = HCGE_DSBF_ZERO;

	state->destination.config.size.w = dest_stride;
	state->destination.config.size.h = fill_area->y2 + 1;
	state->destination.config.format = HCGE_DSPF_ARGB;
	state->dst.phys =
	    (unsigned long)fbdev_virt_to_phy(dest_buf);
	state->dst.pitch = dest_stride * sizeof(lv_color_t);

	state->color.a = opa;
	state->color.r = color.ch.red;
	state->color.g = color.ch.green;
	state->color.b = color.ch.blue;

	state->mod_hw = HCGE_SMF_CLIP;
	state->clip.x1 = fill_area->x1;
	state->clip.y1 = fill_area->y1;
	state->clip.x2 = fill_area->x2;
	state->clip.y2 = fill_area->y2;

	HCGERectangle drect;
	drect.x = fill_area->x1;
	drect.y = fill_area->y1;
	drect.w = area_w;
	drect.h = area_h;

	state->accel = HCGE_DFXL_FILLRECTANGLE;
	cacheflush(dest_buf, state->dst.pitch * state->destination.config.size.h +
	           (fill_area->x2 + 1) * sizeof(lv_color_t),
	           DCACHE);
	hcge_set_state(hcge_ctx, state, state->accel);
	hcge_fill_rect(hcge_ctx, &drect);
	lv_ge_unlock();

	return true;
}

bool lv_ge_blit(struct lv_blit *blit, bool f_cache)
{
	uint32_t src_size = ((blit->src_area->y2 + 1) * blit->src_stride + blit->src_area->x2 + 1) *
	                    bytes_per_pixel(blit->src_fmt);
	uint32_t dst_size = ((blit->dst_area->y2 + 1) * blit->dst_stride + blit->dst_area->x2 + 1) *
	                    bytes_per_pixel(blit->dst_fmt);

	if (!fbdev_check_addr((uint32_t)blit->dst_buf, dst_size))
		return false;
	if (!fbdev_check_addr((uint32_t)blit->src_buf, src_size))
		return false;

	lv_ge_lock();
	hcge_state *state = &hcge_ctx->state;
	HCGERectangle srect;
	HCGERectangle drect;

	hcge_state_init(state);
#if CONFIG_LV_HC_DITHER == 1
	if(bytes_per_pixel(blit->dst_fmt) < 4 && !blit->h_flip && !blit->v_flip
	   && bytes_per_pixel(blit->src_fmt) == 4)
		state->dither_en = true;
	else
		state->dither_en = false;
#else
	state->dither_en = false;
#endif

	if(blit->h_flip)
		state->blittingflags |= HCGE_DSBLIT_FLIP_HORIZONTAL;
	if(blit->v_flip)
		state->blittingflags |= HCGE_DSBLIT_FLIP_VERTICAL;

	state->mod_hw = HCGE_SMF_CLIP;
	if (blit->crop) {
		state->clip.x1 = blit->crop->x1;
		state->clip.y1 = blit->crop->y1;
		state->clip.x2 = blit->crop->x2;
		state->clip.y2 = blit->crop->y2;
	} else {
		state->clip.x1 = 0;
		state->clip.y1 = 0;
		state->clip.x2 = LV_HOR_RES - 1;
		state->clip.y2 = LV_VER_RES - 1;
	}

	state->color.a = (blit->opa >= LV_OPA_MAX) ? LV_OPA_COVER: blit->opa;

	if (blit->angle > 0) {
		state->blittingflags |= HCGE_DSBLIT_ROTATE;
		hcge_ctx->angle = blit->angle;
		/*
		 * if blit->pivot = NULL, rotate center is (w/2,h/2)
		 */
		if(blit->pivot) {
			hcge_ctx->pivot.x = blit->pivot->x;
			hcge_ctx->pivot.y = blit->pivot->y;
		}
	} else {
		hcge_ctx->angle = 0;
	}

	state->destination.config.size.w = blit->dst_stride;
	state->destination.config.size.h = blit->dst_area->y2 + 1;
	state->destination.config.format = blit->dst_fmt;
	state->dst.phys = (unsigned long)fbdev_virt_to_phy(blit->dst_buf);
	state->dst.pitch = blit->dst_stride * bytes_per_pixel(blit->dst_fmt);

	state->source.config.size.w = blit->src_stride;
	state->source.config.size.h = blit->src_area->y2 + 1;
	state->source.config.format = blit->src_fmt;
	state->src.phys = (unsigned long)fbdev_virt_to_phy(blit->src_buf);
	state->src.pitch = blit->src_stride * bytes_per_pixel(blit->src_fmt);

	drect.x = blit->dst_area->x1;
	drect.y = blit->dst_area->y1;
	drect.w = lv_area_get_width(blit->dst_area);
	drect.h = lv_area_get_height(blit->dst_area);

	srect.x = blit->src_area->x1;
	srect.y = blit->src_area->y1;
	srect.w = lv_area_get_width(blit->src_area);
	srect.h = lv_area_get_height(blit->src_area);
	if(f_cache) {
		cacheflush(blit->dst_buf, dst_size, DCACHE);
		cacheflush(blit->src_buf, src_size, DCACHE);
	}

	if (blit->angle > 0 || blit->h_flip || blit->v_flip || !blit->stretch) {
		if(blit->angle == 0 && !blit->h_flip && !blit->v_flip) {
			state->blend_mode = HCGE_BLEND_PATTERN_BYPASS;
		} else if (blit->angle > 0 && blit->stretch) {
			state->blend_mode = HCGE_BLEND_ALPHA;
		} else {
			state->blend_mode = HCGE_BLEND_DFB;
		}
		state->accel = HCGE_DFXL_BLIT;
		hcge_set_state(hcge_ctx, state, state->accel);
		hcge_blit(hcge_ctx, &srect, drect.x, drect.y);
	} else {
		state->blend_mode = HCGE_BLEND_ALPHA;
		if(blit->cover) {
			/*
			 * used blow code alse get the same function
			 * state->blittingflags |= HCGE_DSBLIT_BLEND_ALPHACHANNEL;
			 * state->blend_operation = HCGE_DSBF_SRC;
			 */
			state->blend_mode = HCGE_BLEND_DFB;
		}
		state->accel = HCGE_DFXL_STRETCHBLIT;
		hcge_set_state(hcge_ctx, state, state->accel);
		hcge_stretch_blit(hcge_ctx, &srect, &drect);
	}
	hcge_engine_sync(hcge_ctx);
	lv_ge_unlock();
	return true;
}

void lv_draw_hc_wait_cb(lv_draw_ctx_t *draw_ctx)
{
	lv_ge_lock();
	hcge_engine_sync(hcge_ctx);
	lv_ge_unlock();
	lv_draw_sw_wait_for_finish(draw_ctx);
}

void lv_ge_memset(void *dst, uint8_t val, size_t len)
{
	uint8_t * d8 = (uint8_t *)dst;
	uintptr_t d_align = (lv_uintptr_t) d8 & HC_MEM_ALIGN_MASK;
	if(!hcge_ctx){
		memset(dst, val, len);
		return;
	}

	if(len > 1024*20) {
		//align header address
		if(d_align) {
			d_align = HC_MEM_ALIGN_MASK + 1 - d_align;
			memset(d8, val, d_align);
			d8 += d_align;
			len -= d_align;
		}

		int w = 1<<10;
		//devided by 1024 * 4
		int h = len >> (sizeof(lv_color_t)/2 + 10);
		int l = 0;
		if(h > 0) {
			lv_area_t area = {0, 0, w - 1, h -1};
			lv_color_t color;
			memset(&color, val, sizeof(lv_color_t));
			ge_rect_fill((lv_color_t *)d8, w, &area, color, val, 1);
			l = w * h * sizeof(lv_color_t);
			d8 += l;
			len -= l;
			/* here add hcge_engine_sync(hcge_ctx), that have image backup
			 * lead to decrease 8-10fps
			 */
		}
	}

	if(len > 0) {
		memset(d8, val, len);
	}
	GE_WAIT();
}

LV_ATTRIBUTE_FAST_MEM void lv_memset_ff(void * dst, size_t len)
{
	lv_ge_memset(dst, 0xff, len);
}

LV_ATTRIBUTE_FAST_MEM void lv_memset_00(void * dst, size_t len)
{
	lv_ge_memset(dst, 0x00, len);
}

LV_ATTRIBUTE_FAST_MEM void lv_memset(void * dst, uint8_t v, size_t len)
{
	lv_ge_memset(dst, v, len);
}

#endif
