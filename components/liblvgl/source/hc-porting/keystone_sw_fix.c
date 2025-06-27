#include "keystone_sw_fix.h"
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <hcuapi/dis.h>
#include <hcuapi/pq.h>
#include <sys/ioctl.h>
#if LV_HC_KEYSTONE_AA_SW_FIX_KAF_CMD
#include <kernel/lib/console.h>
#endif
#include "lv_hichip_conf.h"

#if LV_HC_KEYSTONE_AA_SW_FIX || CONFIG_LV_HC_KEYSTONE_EQUAL_SCALE

#define NBIT_FRAC_KS_SUBPIX 18

// left edge:
// 		LEFT_DEAD_SPAN | LEFT_SRC_SPAN | non-fixing pixels...
#define LEFT_SRC_SPAN 2
#define LEFT_DEAD_SPAN 1
#define LEFT_FIX_SPAN (LEFT_DEAD_SPAN + LEFT_SRC_SPAN)

// right edge:
//		non-fixing pixels... | RIGHT_SRC_SPAN | RIGHT_DEAD_SPAN
#define RIGHT_SRC_SPAN 2
#define RIGHT_DEAD_SPAN 2
#define RIGHT_FIX_SPAN (RIGHT_SRC_SPAN + RIGHT_DEAD_SPAN)

// #define KAF_DEBUG

#ifdef KAF_DEBUG
#define KAF_PRINT printf
#else
#define KAF_PRINT(fmt, ...) do {} while (0)
#endif

#define ALWAYS_INLINE __attribute__((always_inline)) inline

struct kaf_ctx_t {
	int16_t width;
	int16_t height;
	int16_t top_width;
	int16_t bottom_width;
	int16_t knob;

	int16_t last_top_width;
	int16_t last_bottom_width;
	union {
		struct {
			int16_t active : 1; // set by start_frame, indicate if keystone fix is active
			int16_t in_frame : 1; // set by start_frame, clear by end_frame
			int16_t left_dirty : 1;
			int16_t right_dirty : 1;
			int16_t forced_redraw : 1;
		};
		int16_t flags;
	};
	void *src_left;
	void *src_right;
	void *fixed_left;
	void *fixed_right;
};

static ALWAYS_INLINE int calc_delta(int offset_diff, int nline) {
	return ((offset_diff << (NBIT_FRAC_KS_SUBPIX + 1)) / nline) >> 1;
}

/**
 * @brief Calculate side spans of keystone corrections
 *
 * @param top_width The top base length of trapezoid (pixels)
 * @param bottom_width The bottom base length of trapezoid (pixels)
 * @param nline_osd Number of lines in OSD layer. It might be scaled to display size.
 * @param [out] ctx Keystone edge fix context
 *
 * @remarks As only to blend active video with background color, sub-pixel
 *          position is not important. Delta could be non-directional.
 */
static
void calc_keystone_side_delta(int top_width, int bottom_width, int nline_osd,
							  int *left_delta, int *right_delta) {
	/* left & right span of keystone correction
			|<->|___________|<->|
				/			\
			   /			 \
			  / 			  \
			 /_________________\
	 */
	int init_offset, last_offset;
	if (top_width < bottom_width) {
		init_offset = (bottom_width - top_width) >> 1;
		last_offset = 0;
	} else {
		init_offset = 0;
		last_offset = (top_width - bottom_width) >> 1;
	}
	*left_delta = -calc_delta(init_offset - last_offset, nline_osd);
	*right_delta = calc_delta(init_offset - last_offset, nline_osd);
}

/// @param c_osd pre-multiplied OSD pixel channel
static ALWAYS_INLINE
uint32_t mix_premult(uint32_t c_osd, uint32_t beta_q8) {
	uint32_t c = c_osd * (256 - beta_q8);
	return c >> 8;
}

static inline
uint32_t compose_edge_no_corr(uint32_t c_osd, uint32_t beta) {
	const uint32_t c0_osd =  c_osd        & 0xff;
	const uint32_t c1_osd = (c_osd >>  8) & 0xff;
	const uint32_t c2_osd = (c_osd >> 16) & 0xff;
	const uint32_t c0 = mix_premult(c0_osd, beta);
	const uint32_t c1 = mix_premult(c1_osd, beta);
	const uint32_t c2 = mix_premult(c2_osd, beta);
	return (c2 << 16) | (c1 << 8) | c0;
}

#if LV_HC_KEYSTONE_AA_SW_FIX_GC

static ALWAYS_INLINE int round_8b(int value) {
	return (value + 127) >> 8;
}

static ALWAYS_INLINE int clip_8b(int value) {
	if (value < 0) {
		return 0;
	} else if (value > 255) {
		return 255;
	} else {
		return value;
	}
}

static ALWAYS_INLINE int clip_10b(int value) {
	if (value < 0) {
		return 0;
	} else if (value > 1023) {
		return 1023;
	} else {
		return value;
	}
}

static unsigned degamma_lookup(const int *lut, int v) {
	static const int16_t interp_coef[][4] = {
		{  0, 256,   0,   0},
		{-14, 210,  70, -10},
		{-16, 144, 144, -16},
		{-10,  70, 210, -14},
	};
	unsigned index = (v >> 2)  + 1;
	unsigned phase = v & 3;
	const int16_t *coef = interp_coef[phase];
	int sum = lut[index - 1] * coef[0] +
			  lut[index    ] * coef[1] +
			  lut[index + 1] * coef[2] +
			  lut[index + 2] * coef[3];
	return round_8b(sum);
}

static int gamma_lookup(const int *lut, int value) {
	static const int16_t interp_coef[][4] = {
		{  0, 256,   0,   0},
		{ -9, 236,  34,  -5},
		{-14, 210,  70, -10},
		{-16, 179, 107, -14},
		{-16, 144, 144, -16},
		{-14, 107, 179, -16},
		{-10,  70, 210, -14},
		{ -5,  34, 236,  -9},
	};
	unsigned index = (value >> 3) + 1;
	unsigned phase = value & 7;
	const int16_t *coef = interp_coef[phase];
	int sum = lut[index - 1] * coef[0] +
			  lut[index    ] * coef[1] +
			  lut[index + 1] * coef[2] +
			  lut[index + 2] * coef[3];
	return clip_8b(round_8b(sum));
}

static void generate_gamma_lut(const int *degamma_lut, const int *gamma_lut,
							   uint8_t *gamma_corr) {
	memset(gamma_corr, 0, sizeof(*gamma_corr) * 256);
	for (int vi = 0; vi < 256; ++vi) {
		int li = degamma_lookup(degamma_lut, vi);
		int vo = gamma_lookup(gamma_lut, li);
		gamma_corr[vo] = vi;
	}
	// fill gaps
	int last_value = 0;
	for (int i = 1; i < 256; ++i) {
		if (gamma_corr[i] == 0) {
			gamma_corr[i] = last_value;
		} else {
			last_value = gamma_corr[i];
		}
	}
}

struct kaf_gamma_corr {
	bool enabled;
	bool lut_valid;
	uint8_t lut[3][256];
};

static struct kaf_gamma_corr *gamma_corr_inst() {
	static struct kaf_gamma_corr gamma_corr = {
		.enabled = LV_HC_KEYSTONE_AA_SW_FIX_GC,
		.lut_valid = false,
	};
	return &gamma_corr;
}

void kaf_set_pq_param(const struct pq_settings *pq) {
	struct kaf_gamma_corr *gc = gamma_corr_inst();
	if (pq) {
		generate_gamma_lut(pq->invgamma_lut_r, pq->gamma_lut_r, gc->lut[0]);
		generate_gamma_lut(pq->invgamma_lut_g, pq->gamma_lut_g, gc->lut[1]);
		generate_gamma_lut(pq->invgamma_lut_b, pq->gamma_lut_b, gc->lut[2]);
		gc->lut_valid = true;
	} else {
		gc->lut_valid = false;
	}
}

static inline
uint32_t compose_edge_corr_ARGB(uint32_t c_osd, uint32_t beta) {
	struct kaf_gamma_corr *gc = gamma_corr_inst();
	// 32-bit: (msb) A8 R8 G8 B8 (lsb)
	const uint32_t c0_osd =  c_osd        & 0xff;
	const uint32_t c1_osd = (c_osd >>  8) & 0xff;
	const uint32_t c2_osd = (c_osd >> 16) & 0xff;
	const uint32_t c0 = gc->lut[2][mix_premult(c0_osd, beta)];
	const uint32_t c1 = gc->lut[1][mix_premult(c1_osd, beta)];
	const uint32_t c2 = gc->lut[0][mix_premult(c2_osd, beta)];
	return (c2 << 16) | (c1 << 8) | c0;
}

typedef uint32_t (*compose_edge_func_t)(uint32_t, uint32_t);

static
compose_edge_func_t pick_compose_edge_func(const struct kaf_gamma_corr *gc) {
	if (gc->enabled && gc->lut_valid) {
		return compose_edge_corr_ARGB;
	} else {
		return compose_edge_no_corr;
	}
}

#endif // LV_HC_KEYSTONE_AA_SW_FIX_GC

static ALWAYS_INLINE uint32_t decode_alpha8(uint32_t alpha) {
	return alpha + (alpha != 0);
}

static ALWAYS_INLINE uint32_t encode_alpha8(uint32_t alpha) {
	return alpha - (alpha != 0);
}

/**
 * @brief Fix keystone left jaggy edge by overlaying a smooth version.
 * @remark Fix up to three pixels each line.
 *
 * @param y_start start of line number, inclusive
 * @param y_end end of line, exclusive
 * @param delta left edge delta, from calc_keystone_side_delta()
 * @param bg_color keystone background color
 * @param last_edge external backup of source edge pixel, a walk-around solution, to be removed. TODO REMOVE
 * @param src_stride stride of OSD buffer
 * @param src pointer to source buffer (left most pixel)
 * @param dst pointer to destination buffer (left most pixel)
 */
static
void fix_left_edge_a8c24(int delta, int y_start, int y_end,
		int src_stride, const uint32_t *src, int dst_stride, uint32_t *dst) {
	const int nbit_shift = NBIT_FRAC_KS_SUBPIX - 8; // 8-bit beta
#if LV_HC_KEYSTONE_AA_SW_FIX_GC
	compose_edge_func_t compose_edge = pick_compose_edge_func(gamma_corr_inst());
#endif
	// assuming A[31:24] | COLOR[23:0]
	int x = delta * y_start;
	const uint32_t *src_pixel = src;
	uint32_t *dst_pixel = dst;
	for (int y = y_start; y < y_end; ++y) {
		const uint32_t c_osd = src_pixel[0]; // 1 in original plane
		const uint32_t a_osd = decode_alpha8((c_osd >> 24) & 0xff);
		const uint32_t beta = (x >> nbit_shift) & 255;
		const uint32_t a1_q16 = 65536 - (256 - a_osd) * (256 - beta);
#if LV_HC_KEYSTONE_AA_SW_FIX_GC
		const uint32_t new_color = compose_edge(c_osd, beta);
#else
		const uint32_t new_color = compose_edge_no_corr(c_osd, beta);
#endif
		const uint32_t alpha = encode_alpha8(a1_q16 >> 8);
		const uint32_t new_pixel = (alpha << 24) | new_color;
		dst_pixel[0] = 0xff << 24;
		if (delta > 0 && beta == 0) {
			dst_pixel[1] = 0xff << 24;
			dst_pixel[2] = new_pixel;
		} else {
			dst_pixel[1] = new_pixel;
			dst_pixel[2] = src_pixel[1]; // 2 in original plane
		}
		x += delta;
		src_pixel += src_stride;
		dst_pixel += dst_stride;
	}
}

/**
 * @brief Fix keystone right jaggy edge by overlaying a smooth version.
 * @remark Fix up to four pixels each line.
 *
 * @param y_start start of line number, inclusive
 * @param y_end end of line, exclusive
 * @param delta right edge delta, from calc_keystone_side_delta()
 * @param last_edge external backup of source edge pixel, a walk-around solution, to be removed
 * @param src_stride stride of source buffer
 * @param src points to right most source pixel
 * @param dst points to right most destination pixel
 */
static
void fix_right_edge_a8c24(int delta, int y_start, int y_end,
		int src_stride, const uint32_t *src, int dst_stride, uint32_t *dst) {
	const int nbit_shift = NBIT_FRAC_KS_SUBPIX - 8; // 8-bit beta
#if LV_HC_KEYSTONE_AA_SW_FIX_GC
	compose_edge_func_t compose_edge = pick_compose_edge_func(gamma_corr_inst());
#endif
	// assuming A[31:24] | COLOR[23:0]
	int x = delta * y_start;
	const uint32_t *src_pixel = src;
	uint32_t *dst_pixel = dst;
	for (int y = y_start; y < y_end; ++y) {
		const uint32_t c_osd = src_pixel[0]; // -2 in original plane
		const uint32_t a_osd = decode_alpha8((c_osd >> 24) & 0xff);
		const uint32_t beta = 256 - ((x >> nbit_shift) & 255);
		const uint32_t a1_q16 = 65536 - (256 - a_osd) * (256 - beta);
#if LV_HC_KEYSTONE_AA_SW_FIX_GC
		const uint32_t new_color = compose_edge(c_osd, beta);
#else
		const uint32_t new_color = compose_edge_no_corr(c_osd, beta);
#endif
		const uint32_t alpha = encode_alpha8(a1_q16 >> 8);
		const uint32_t new_pixel = (alpha << 24) | new_color;
		if (delta < 0 && beta == 1) {
			dst_pixel[-3] = new_pixel;
			dst_pixel[-2] = 0xff << 24;
		} else {
			dst_pixel[-3] = src_pixel[-1]; // -3 in original plane
			dst_pixel[-2] = new_pixel;
		}
		// to avoid keystone HW interp artifact, last two pixels are set to BG
		dst_pixel[-1] = 0xff << 24;
		dst_pixel[ 0] = 0xff << 24;
		x += delta;
		src_pixel += src_stride;
		dst_pixel += dst_stride;
	}
}

static int get_keystone_setup(int16_t *top_w, int16_t *bot_w, uint8_t *enable)
{
	struct dis_keystone_param pparm = { 0 };
	int fd = open("/dev/dis" , O_RDWR);
	if (fd < 0) {
		return -1;
	}
	pparm.distype = DIS_TYPE_HD;
	ioctl(fd , DIS_GET_KEYSTONE_PARAM , &pparm);
	if (pparm.info.enable) {
		*top_w = pparm.info.width_up;
		*bot_w = pparm.info.width_down;
		*enable = pparm.info.enable;
	}
	//printf(">> T: %d , B: %d , enable: %d\n", pparm.info.width_up, pparm.info.width_down, pparm.info.enable);
	close(fd);
	return 0;
}

struct kaf_ctx_t *kaf_get_ctx(void) {
	static struct kaf_ctx_t _ctx = {};
	return &_ctx;
}

static int init_32bpp(struct kaf_ctx_t *ctx, int height) {
	ctx->src_left = malloc(height * LEFT_FIX_SPAN * sizeof(uint32_t));
	ctx->src_right = malloc(height * RIGHT_FIX_SPAN * sizeof(uint32_t));
	ctx->fixed_left = malloc(height * LEFT_FIX_SPAN * sizeof(uint32_t));
	ctx->fixed_right = malloc(height * RIGHT_FIX_SPAN * sizeof(uint32_t));
	if (ctx->src_left == NULL || ctx->src_right == NULL ||
			ctx->fixed_left == NULL || ctx->fixed_right == NULL) {
		goto err_exit;
	}
	return 1;
err_exit:
	free(ctx->src_left);
	free(ctx->src_right);
	free(ctx->fixed_left);
	free(ctx->fixed_right);
	return 0;
}

void kaf_init(struct kaf_ctx_t *ctx, int width, int height, unsigned knob) {
	ctx->width = width;
	ctx->height = height;
	ctx->knob = knob;
	kaf_reset(ctx);
	KAF_PRINT("[kaf] init size=%dx%d\n", width, height);
	init_32bpp(ctx, height);
}

void kaf_free(struct kaf_ctx_t *ctx) {
	free(ctx->src_left);
	free(ctx->src_right);
	free(ctx->fixed_left);
	free(ctx->fixed_right);
	ctx->src_left = NULL;
	ctx->src_right = NULL;
	ctx->fixed_left = NULL;
	ctx->fixed_right = NULL;
}

#define SCAN_H_BIT 0
#define SCAN_V_BIT 1
#define SCAN_H_MASK (1u << SCAN_H_BIT)
#define SCAN_V_MASK (1u << SCAN_V_BIT)

static uint32_t eval_scan_order(int rotate, int h_flip, int v_flip) {
	uint32_t scan = 0;
	if (rotate == 180) {
		scan ^= SCAN_H_MASK;
		scan ^= SCAN_V_MASK;
	}
	if (h_flip) {
		scan ^= SCAN_H_MASK;
	}
	if (v_flip) {
		scan ^= SCAN_V_MASK;
	}
	return scan;
}

static
void blit_u32(const lv_area_t *area,
		uint32_t *src, int src_stride, uint32_t *dst, int dst_stride) {
	KAF_PRINT("[kaf] blit_u32 w=%d h=%d\n", area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
	for (int y = area->y1; y <= area->y2; ++y) {
		const uint32_t *s = src;
		uint32_t *t = dst;
		for (int x = area->x1; x <= area->x2; ++x) {
			*t++ = *s++;
		}
		src += src_stride;
		dst += dst_stride;
	}
}

static
void blit_r_u32(const lv_area_t *area,
		uint32_t *src, int src_stride, uint32_t *dst, int dst_stride) {
	for (int y = area->y1; y <= area->y2; ++y) {
		const uint32_t *s = src;
		uint32_t *t = dst;
		for (int x = area->x1; x <= area->x2; ++x) {
			*t++ = *s--; // reverse source
		}
		src += src_stride;
		dst += dst_stride;
	}
}

static
void blit_uint32(const lv_area_t *area, uint32_t scan,
		const uint32_t *src, int src_stride, uint32_t *dst, int dst_stride) {
	// point to last line and negate stride if vertical scan is reversed
	if (scan & SCAN_V_MASK) {
		src += src_stride * (area->y2 - area->y1); // as y2 is inclusive
		src_stride = -src_stride;
	}
	if (scan & SCAN_H_MASK) {
		// point to last pixel if horizontal scan is reversed
		src += area->x2 - area->x1; // as x2 is inclusive
		blit_r_u32(area, src, src_stride, dst, dst_stride);
	} else {
		blit_u32(area, src, src_stride, dst, dst_stride);
	}
}

// left edge: | DEAD_SPAN | SRC_SPAN | ... no-fixing-part ...
// right edge: ... no-fixing-part ... | SRC_SPAN | DEAD_SPAN |

/// @brief evaluate intersection with left edge
/// @param [out] sect intersection between incoming area and left edge
/// @param [out] src_offset offset to adjust source pointer
/// @param [out] dst_offset offset to adjust destination pointer
/// @return 1 if two intersects, otherwise 0
static int intersect_left_edge(struct kaf_ctx_t *ctx, const lv_area_t *area,
		uint32_t scan, lv_area_t *sect, int *src_offset, int *dst_offset) {
	const lv_area_t left_edge_area = {
		.x1 = 0,
		.y1 = 0,
		.x2 = LEFT_FIX_SPAN - 1,
		.y2 = ctx->height - 1,
	};
	if (_lv_area_intersect(sect, area, &left_edge_area)) {
		if (scan & SCAN_H_MASK) {
			/*                   |<- offset >|
				+----------------+--- ... ---o <- src buffer
				|/ intersection /|           |
				|////////////////|           |
				|<------    stride    ------>|
			*/
			*src_offset = area->x2 - sect->x2;
		} else {
			/*
				src buffer -> o----------------+--- ... ---+
				              |/ intersection /|           |
				              |////////////////|           |
				              |<------    stride    ------>|
			*/
			*src_offset = sect->x1 - area->x1; // should be always 0
		}
		*dst_offset = LEFT_FIX_SPAN * sect->y1 + sect->x1;
		return 1;
	} else {
		return 0;
	}
}

/// @brief evaluate intersection with right edge
/// @param [out] sect intersection between incoming area and right edge
/// @param [out] src_offset offset to adjust source pointer
/// @param [out] dst_offset offset to adjust destination pointer
/// @return 1 if two intersects, otherwise 0
static int intersect_right_edge(struct kaf_ctx_t *ctx, const lv_area_t *area,
		uint32_t scan, lv_area_t *sect, int *src_offset, int *dst_offset) {
	const lv_area_t right_edge_area = {
		.x1 = ctx->width - RIGHT_FIX_SPAN,
		.y1 = 0,
		.x2 = ctx->width - 1, // inclusive
		.y2 = ctx->height - 1,
	};
	if (_lv_area_intersect(sect, area, &right_edge_area)) {
		if (scan & SCAN_H_MASK) {
			/*
				+--- ... ---+----------------o <- src buffer
				|           |/ intersection /|
				|           |////////////////|
				|<------    stride    ------>|
			*/
			*src_offset = area->x2 - sect->x2; // should be always 0
		} else {
			/*                |<- offset >|
				src buffer -> o--- ... ---+----------------+
				              |           |/ intersection /|
				              |           |////////////////|
				              |<------    stride    ------>|
			*/
			*src_offset = sect->x1 - area->x1;
		}
		*dst_offset = RIGHT_FIX_SPAN * sect->y1 + (sect->x1 - right_edge_area.x1);
		return 1;
	} else {
		return 0;
	}
}

static void kaf_update_a8c24(struct kaf_ctx_t *ctx,
		const lv_area_t *area, uint32_t scan, uint32_t *src, int src_stride) {
	lv_area_t sect;
	int src_offset, dst_offset;
	if (intersect_left_edge(ctx, area, scan, &sect, &src_offset, &dst_offset)) {
		const uint32_t *s = src + src_offset;
		uint32_t *t = (uint32_t*)ctx->src_left + dst_offset;
		blit_uint32(&sect, scan, s, src_stride, t, LEFT_FIX_SPAN);
		ctx->left_dirty = 1;
	}
	if (intersect_right_edge(ctx, area, scan, &sect, &src_offset, &dst_offset)) {
		const uint32_t *s = src + src_offset;
		uint32_t *t = (uint32_t*)ctx->src_right + dst_offset;
		blit_uint32(&sect, scan, s, src_stride, t, RIGHT_FIX_SPAN);
		ctx->right_dirty = 1;
	}
}

enum kaf_status_t kaf_catch_edge(struct kaf_ctx_t *ctx,
		int rotate, int h_flip, int v_flip,
		const lv_area_t *src_area, const void *src, int src_stride) {
	if (rotate != 0 && rotate != 180) {
		return KAF_STATUS_INVALID_TRANSFORM;
	}
	const uint32_t scan = eval_scan_order(rotate, h_flip, v_flip);
	lv_area_t area = *src_area;
	if (scan & SCAN_H_MASK) {
		area.x1 = ctx->width - 1 - src_area->x2;
		area.x2 = ctx->width - 1 - src_area->x1;
	}
	if (scan & SCAN_V_MASK) {
		area.y1 = ctx->height - 1 - src_area->y2;
		area.y2 = ctx->height - 1 - src_area->y1;
	}
	KAF_PRINT("[kaf] catch edge %c%c (%d, %d)-(%d, %d) -> (%d, %d)-(%d, %d)\n",
		(scan & SCAN_H_MASK) ? 'h' : '-',
		(scan & SCAN_V_MASK) ? 'v' : '-',
		src_area->x1, src_area->y1, src_area->x2, src_area->y2,
		area.x1, area.y1, area.x2, area.y2);
	kaf_update_a8c24(ctx, &area, scan, src, src_stride);
	return KAF_STATUS_NO_ERR;
}

void kaf_reset(struct kaf_ctx_t *ctx) {
	ctx->last_top_width = 0;
	ctx->last_bottom_width = 0;
	ctx->flags = 0;
}

enum kaf_status_t kaf_start_frame(struct kaf_ctx_t *ctx) {
	if (ctx->in_frame) {
		return KAF_STATUS_FRAME_IN_FRAME;
	}
	ctx->in_frame = 1;
	// detect if keystone fix should be active
	uint8_t ks_enable = 0;
	if (get_keystone_setup(&ctx->top_width, &ctx->bottom_width, &ks_enable) != 0) {
		KAF_PRINT("[kaf] failed to get keystone setup\n");
		return KAF_STATUS_FRAME_INACTIVE;
	}
	ctx->active = ks_enable && ctx->top_width != ctx->bottom_width;
	KAF_PRINT("[kaf] start_frame : ks %d / %d, en=%d, active=%d\n",
			ctx->top_width, ctx->bottom_width, ks_enable, ctx->active);
	return ctx->active ? KAF_STATUS_FRAME_ACTIVE : KAF_STATUS_FRAME_INACTIVE;
}

void kaf_end_frame(struct kaf_ctx_t *ctx) {
	ctx->active = 0;
	ctx->in_frame = 0;
}

int kaf_is_active(struct kaf_ctx_t *ctx) {
	return ctx->active;
}

static inline void refresh_last(struct kaf_ctx_t *ctx) {
	ctx->last_top_width = ctx->top_width;
	ctx->last_bottom_width = ctx->bottom_width;
	ctx->left_dirty = 0;
	ctx->right_dirty = 0;
	ctx->forced_redraw = 0;
}

static ALWAYS_INLINE
bool is_ks_setup_changed(const struct kaf_ctx_t *ctx) {
	return ctx->top_width != ctx->last_top_width ||
		   ctx->bottom_width != ctx->last_bottom_width;
}

static int fix_edge(struct kaf_ctx_t *ctx) {
	const bool redraw = ctx->forced_redraw || is_ks_setup_changed(ctx);
	if (!(redraw || ctx->left_dirty || ctx->right_dirty))
		return 0;
	int flag = 0;
	int left_delta, right_delta;
	calc_keystone_side_delta(ctx->top_width, ctx->bottom_width, ctx->height,
							 &left_delta, &right_delta);
	if (ctx->left_dirty || redraw) {
		fix_left_edge_a8c24(
			left_delta, 0, ctx->height,
			LEFT_FIX_SPAN, (uint32_t*)ctx->src_left + LEFT_DEAD_SPAN,
			LEFT_FIX_SPAN, (uint32_t*)ctx->fixed_left);
		flag |= KAF_EDGE_LEFT;
	}
	if (ctx->right_dirty || redraw) {
		fix_right_edge_a8c24(
			right_delta, 0, ctx->height,
			RIGHT_FIX_SPAN, (uint32_t*)ctx->src_right + RIGHT_SRC_SPAN - 1,
			RIGHT_FIX_SPAN, (uint32_t*)ctx->fixed_right + RIGHT_FIX_SPAN - 1);
		flag |= KAF_EDGE_RIGHT;
	}
	return flag;
}

static int restore_edge(struct kaf_ctx_t *ctx) {
	const bool chop_dead = ctx->knob & KAF_KNOB_ALWAYS_CHOP_EDGE_DEAD_ZONE;
	lv_area_t left_area = {
		.x1 = chop_dead ? LEFT_DEAD_SPAN : 0,
		.y1 = 0,
		.x2 = LEFT_FIX_SPAN - 1,
		.y2 = ctx->height - 1,
	};
	blit_u32(&left_area,
		(uint32_t*)ctx->src_left + (chop_dead ? LEFT_DEAD_SPAN : 0), LEFT_FIX_SPAN,
		ctx->fixed_left, LEFT_FIX_SPAN);

	lv_area_t right_area = {
		.x1 = 0,
		.y1 = 0,
		.x2 = chop_dead ? RIGHT_SRC_SPAN - 1 : RIGHT_FIX_SPAN - 1,
		.y2 = ctx->height - 1,
	};
	blit_u32(&right_area,
		ctx->src_right, RIGHT_FIX_SPAN, ctx->fixed_right, RIGHT_FIX_SPAN);
	return KAF_EDGE_LEFT | KAF_EDGE_RIGHT;
}

int kaf_fix(struct kaf_ctx_t *ctx) {
	if (!ctx->in_frame)
		return 0;
	int flag = 0;
	if (!ctx->active) {
		if (ctx->last_top_width != ctx->last_bottom_width) {
			// restore edges when transit to inactive from active
			KAF_PRINT("[kaf] restore edge\n");
			flag = restore_edge(ctx);
		}
	} else {
		flag = fix_edge(ctx);
	}
	refresh_last(ctx);
	KAF_PRINT("[kaf] kaf_fix return %d\n", flag);
	return flag;
}

void *kaf_get_left_edge(struct kaf_ctx_t *ctx, int *width, int *height) {
	if (width) {
		*width = LEFT_FIX_SPAN;
	}
	if (height) {
		*height = ctx->height;
	}
	return ctx->fixed_left;
}

void *kaf_get_right_edge(struct kaf_ctx_t *ctx, int *width, int *height) {
	if (width) {
		*width = RIGHT_FIX_SPAN;
	}
	if (height) {
		*height = ctx->height;
	}
	return ctx->fixed_right;
}

static ALWAYS_INLINE int min_int(int a, int b) {
	return a < b ? a : b;
}

static ALWAYS_INLINE int max_int(int a, int b) {
	return a >= b ? a : b;
}

static void chop(struct kaf_ctx_t *ctx, const lv_area_t *area, uint32_t scan,
				 int left_to_chop, int right_to_chop,
				 lv_area_t *chopped) {
	if (scan & SCAN_H_MASK) {
		chopped->x1 = max_int(area->x1, right_to_chop);
		chopped->x2 = min_int(area->x2, ctx->width - left_to_chop - 1);
	} else {
		chopped->x1 = max_int(area->x1, left_to_chop);
		chopped->x2 = min_int(area->x2, ctx->width - right_to_chop - 1);
	}
	chopped->y1 = area->y1;
	chopped->y2 = area->y2;
}

void kaf_chop_edge(struct kaf_ctx_t *ctx, const lv_area_t *area,
				   int rotate, int h_flip, int v_flip, lv_area_t *chopped) {
	if (ctx->active) {
		const uint32_t scan = eval_scan_order(rotate, h_flip, v_flip);
		chop(ctx, area, scan, LEFT_FIX_SPAN, RIGHT_FIX_SPAN, chopped);
	} else {
		if (ctx->knob & KAF_KNOB_ALWAYS_CHOP_EDGE_DEAD_ZONE) {
			const uint32_t scan = eval_scan_order(rotate, h_flip, v_flip);
			chop(ctx, area, scan, LEFT_DEAD_SPAN, RIGHT_DEAD_SPAN, chopped);
		} else {
			*chopped = *area;
		}
	}
}

#if LV_HC_KEYSTONE_AA_SW_FIX_KAF_CMD

static int kaf_main(int argc, char *argv[]) {
	struct kaf_ctx_t *ctx = kaf_get_ctx();
#if LV_HC_KEYSTONE_AA_SW_FIX_GC
	struct kaf_gamma_corr *gc = gamma_corr_inst();
	if (argc > 1) {
		gc->enabled = !!strtol(argv[1], NULL, 0);
		ctx->forced_redraw = 1;
	}
#endif
	printf("keystone: %d x %d, %d / %d, knob=%d, gc en=%d vld=%d\n",
		ctx->width, ctx->height, ctx->top_width, ctx->bottom_width, ctx->knob,
#if LV_HC_KEYSTONE_AA_SW_FIX_GC
		gc->enabled, gc->lut_valid
#else
		-1, -1
#endif
		);
	return 0;
}

CONSOLE_CMD(kaf, NULL, kaf_main, CONSOLE_CMD_MODE_SELF, "keystone")
#endif

#endif // LV_HC_KEYSTONE_AA_SW_FIX

#if (LV_HC_KEYSTONE_AA_SW_FIX == 0) || (LV_HC_KEYSTONE_AA_SW_FIX_GC == 0)

void kaf_set_pq_param(const struct pq_settings *pq) {
	(void)pq;
}

#endif // LV_HC_KEYSTONE_AA_SW_FIX
