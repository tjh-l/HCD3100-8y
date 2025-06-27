#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <kernel/io.h>
#include <kernel/module.h>
#include <kernel/wait.h>
#include <kernel/lib/console.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <hcuapi/tvtype.h>
#include <kernel/lib/fdt_api.h>
#include <nuttx/wqueue.h>
#include <hcuapi/fb.h>
#include <kernel/fb.h>
#include <hcuapi/dis.h>
#include <hcuapi/mmz.h>
#include <nuttx/fs/fs.h>
#include "hc16xx_fb_filter.h"
#include <asm-generic/page.h>
#include <kernel/io.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include "hcfb.h"

#define HC_GMA_CTL		0x300
#define HC_GMA_CTL_HW		(0x300 + 0x800)
#define HC_GMA_DMBA		0x304
#define HC_GMA_K		0x308
#define HC_GMA_MASK		0x350
#define HC_GMA_LINEBUF		0x3b8

#define HC_ADDR_DIFF	(0x80)

#define SCALE_DIR_OFF		(0)
#define SCALE_DIR_UP		BIT(0)
#define SCALE_DIR_DOWN		BIT(1)

#ifndef max
#define max(a, b) (a > b ? a : b)
#endif
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

static const hcfb_palette_t default_palette = {
	.palette_type = HCFB_PALETTE_YCBCR,
	.rgb_order = HCFB_RGB_ORDER_ACrCbY,
	.alpha_range = HCFB_ALPHA_RANGE_15,
	.alpha_polarity = HCFB_ALPHA_POLARITY_0,
	.ncolors = 256,
	.palette = NULL,
};

static bool fb0_valid = false, fb1_valid = false;
static int gma_dmba_update(struct hcfb_device *fbdev);

static void hcfb_update(struct hcfb_device *fbdev,struct fb_var_screeninfo *var);

static enum DIS_DEV_TYPE get_current_de(void)
{
	/* default */
	int fd = -1;
	dis_dev_type cur_dev_type = DE_TYPE_2K;

	fd = open("/dev/dis", O_WRONLY);
	if (fd < 0) {
		return -1;
	}

	ioctl(fd, DIS_GET_CUR_DEV_TYPE, &cur_dev_type);

	close(fd);
	fd = -1;
	return cur_dev_type;
}

static void get_current_viewport(struct fb_info *info, hcfb_viewport_t *viewport)
{
	int disfd = -1;
	struct hcfb_device *fbdev = (struct hcfb_device *)info->par;
	struct dis_screen_info screen = { 0 };
	disfd = open("/dev/dis", O_RDWR);
	if (disfd < 0) {
		return;
	}

	screen.distype = DIS_TYPE_HD;
	if (ioctl(disfd, DIS_GET_SCREEN_INFO, &screen)) {
		close(disfd);
		return;
	}
	viewport->height = screen.area.h;
	viewport->width = screen.area.w;
	close(disfd);
	hcfb_update(fbdev, &info->var);
	gma_dmba_update(fbdev);
}

static int get_dual_de_enable(void)
{
	int np;
	int dual_de_enable = 0;

	np = fdt_node_probe_by_path("/hcrtos/de-engine");
	if (np < 0)
		return 0;
	fdt_get_property_u_32_index(np, "dual-de-enable", 0, &dual_de_enable);
	return dual_de_enable;
}

static void gma_set_mask(struct hcfb_device *fbdev)
{
	REG32_SET_FIELD2(fbdev->iobase + HC_GMA_MASK, 0, 1, 1);
}

static void gma_clear_mask(struct hcfb_device *fbdev)
{
	REG32_SET_FIELD2(fbdev->iobase + HC_GMA_MASK, 0, 1, 0);
}

static void gma_show_onoff(struct hcfb_device *fbdev, bool on)
{
	gma_set_mask(fbdev);
	REG32_SET_FIELD2(fbdev->iobase + HC_GMA_CTL, 0, 1, !!on);
	gma_clear_mask(fbdev);
	fbdev->gma_onoff = !!on;
}

static void gma_show_onoff_sync(struct hcfb_device *fbdev, bool on)
{
	uint32_t val = REG32_READ(fbdev->iobase + HC_GMA_CTL);

	val &= ~BIT(0);
	val |= on;
	gma_set_mask(fbdev);
	REG32_WRITE(fbdev->iobase + HC_GMA_CTL, val);
	gma_clear_mask(fbdev);

	while ((REG32_READ(fbdev->iobase + HC_GMA_CTL_HW) & BIT(0)) != on)
		usleep(5000);
	fbdev->gma_onoff = !!on;
}

static void gma_set_line_buf_combination(struct hcfb_device *fbdev, int val)
{
	gma_set_mask(fbdev);
	REG32_SET_FIELD2(fbdev->iobase + HC_GMA_LINEBUF, 0, 5, val);
	gma_clear_mask(fbdev);
}

static void gma_set_clut_assign(struct hcfb_device *fbdev, int val)
{
	gma_set_mask(fbdev);
	if (fbdev->id == 1) {
		REG32_SET_FIELD2(fbdev->iobase + HC_GMA_LINEBUF-HC_ADDR_DIFF, 16, 2, val);
	} else {
		REG32_SET_FIELD2(fbdev->iobase + HC_GMA_LINEBUF, 16, 2, val);
	}
	gma_clear_mask(fbdev);
}


static void gma_update_screen(struct hcfb_device *fbdev, uint32_t dmba_addr)
{
	gma_set_mask(fbdev);
	REG32_WRITE(fbdev->iobase + HC_GMA_DMBA, PHY_ADDR(dmba_addr));
	gma_clear_mask(fbdev);
}

static void set_csc_bypass(struct hcfb_device *fbdev, int bypass)
{
	if (bypass)
		REG32_SET_FIELD2(fbdev->iobase + HC_GMA_CTL, 19, 1, 1);
	else
		REG32_SET_FIELD2(fbdev->iobase + HC_GMA_CTL, 19, 1, 0);
}

static void set_enhance_sharpness(struct hcfb_device *fbdev, int enable)
{
	if (enable)
		REG32_SET_FIELD2(fbdev->iobase + HC_GMA_CTL, 18, 1, 1);
	else
		REG32_SET_FIELD2(fbdev->iobase + HC_GMA_CTL, 18, 1, 0);
}

static void set_global_alpha(struct hcfb_device *fbdev, uint8_t alpha)
{
	uint32_t val = REG32_READ(fbdev->iobase + HC_GMA_K);

	val &= ~(NBITS_M(16, 1) | NBITS_M(0, 8));
	val |= alpha;

	if (alpha < 0xff) {
		val |= NBITS_M(16, 1);
		fbdev->global_alpha_on = true;
	} else {
		fbdev->global_alpha_on = false;
	}
	REG32_WRITE(fbdev->iobase + HC_GMA_K, val);
	fbdev->global_alpha = alpha;
}

static void set_scale_coef(struct hcfb_device *fbdev)
{
	short tmp0, tmp1, tmp2, tmp3;
	short coef[4 * 16 + 1];
	short coef_v[4 * 16 + 1];
	uint32_t i;
	short *pcoef = fbdev->filter_coef;
	int ret;

	ret = generate_scale_coef(coef, fbdev->h_div, fbdev->h_mul, 4, 16, false);
	if (ret)
		return;

	ret = generate_scale_coef(coef_v, fbdev->v_div, fbdev->v_mul, 3, 16, false);
	if (ret)
		return;

	for (i = 0; i < 4 * 64; i++)
		pcoef[i] = 0;

	/*
	 * fill the block header one depth(128bit,2Qword) after depth,
	 * total 16 depth
	 */
	for (i = 0; i < 16; i++) {
		// hor, tap = 4
		tmp3 = coef[16 * (3 - 3) + i];
		tmp2 = coef[16 * (3 - 2) + i];
		tmp1 = coef[16 * (3 - 1) + i];
		tmp0 = coef[16 * (3 - 0) + i];

		*pcoef = *pcoef | (tmp0 & 0x0FF);
		pcoef++;
		*pcoef = *pcoef | (tmp1 & 0x1FF);
		pcoef++;
		*pcoef = *pcoef | (tmp2 & 0x1FF);
		pcoef++;
		*pcoef = *pcoef | (tmp3 & 0x0FF);
		pcoef += 5;
	}

	/*
	 * fill the block header one depth(128bit,2Qword) after depth,
	 * total 16 depth
	 */
	for (i = 0; i < 16; i++) {
		// hor, tap = 3
		tmp2 = coef_v[16 * (2 - 2) + i];
		tmp1 = coef_v[16 * (2 - 1) + i];
		tmp0 = coef_v[16 * (2 - 0) + i];

		*pcoef = *pcoef | (tmp0 & 0x1FF);
		pcoef++;
		*pcoef = *pcoef | (tmp1 & 0x1FF);
		pcoef++;
		*pcoef = *pcoef | (tmp2 & 0x1FF);
		pcoef += 6;
	}
	return;
}

static inline uint32_t aligned_div_log2(uint32_t numerator, uint32_t den_log2)
{
	return (numerator + ((1 << den_log2) - 1)) >> den_log2;
}

static inline uint32_t eval_pitch_byte(uint32_t npixel, uint32_t bpp)
{
	const uint32_t bit_alignment_log2 = 3; // 1-byte align
	const uint32_t padding = (1 << bit_alignment_log2) - 1;
	const uint32_t to_byte_lsh = (bit_alignment_log2 - 3);
	return ((npixel * bpp + padding) >> bit_alignment_log2) << to_byte_lsh;
}

static void hcfb_update(struct hcfb_device *fbdev,
			  struct fb_var_screeninfo *var)
{
	uint32_t x, y, w, h, h_mul, h_div, v_mul, v_div;

	hcfb_viewport_t *vp = &fbdev->view_port;
	fbdev->bits_per_pixel = var->bits_per_pixel;
	fbdev->green_length = var->green.length;
	fbdev->transp_length = var->transp.length;

	fbdev->pitch = eval_pitch_byte(var->xres_virtual, var->bits_per_pixel);

	fbdev->src_w = var->xres;
	if (var->yres_virtual - var->yoffset >= var->yres)
		fbdev->src_h = var->yres;
	else
		fbdev->src_h = var->yres_virtual - var->yoffset;

	x = fbdev->left;
	y = fbdev->top;
	w = fbdev->src_w;
	h = fbdev->src_h;
	h_mul = fbdev->h_mul;
	h_div = fbdev->h_div;
	v_mul = fbdev->v_mul;
	v_div = fbdev->v_div;
	if (vp->enable) {
		fbdev->sx = max(x * h_mul / h_div + vp->x, 0);
		fbdev->sy = max(y * v_mul / v_div + vp->y, 0);
		fbdev->ex = min((x + w) * h_mul / h_div + vp->x, vp->x + vp->width) - 1;
		fbdev->ey = min((y + h) * v_mul / v_div + vp->y, vp->y + vp->height) - 1;
	} else {
		fbdev->sx = x * h_mul / h_div;
		fbdev->sy = y * v_mul / v_div;
		fbdev->ex = (uint16_t)((x + w) * h_mul / h_div) - 1;
		fbdev->ey = (uint16_t)((y + h) * v_mul / v_div) - 1;
	}
}

static void hcfb_set_left_top(struct fb_info *info,hcfb_lefttop_pos_t *pos)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)info->par;
	if (fbdev && pos) {
		fbdev->left = pos->left;
		fbdev->top = pos->top;
	}
}

static void hcfb_set_viewport(struct fb_info *info, hcfb_viewport_t *viewport)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)info->par;
	if (fbdev && viewport) {
		memcpy(&fbdev->view_port, viewport, sizeof(hcfb_viewport_t));
		if (fbdev->view_port.apply_now) {
			hcfb_update(fbdev, &info->var);
			gma_dmba_update(fbdev);
		}
	}

}

static void hcfb_set_scale(struct fb_info *info, hcfb_scale_t *scale)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)info->par;
	uint32_t h_div, v_div, h_mul, v_mul;
	uint8_t h_scale_dir, v_scale_dir;
	uint32_t h_scale, v_scale;
	uint8_t scale_ep_on = SCALE_EP_OFF;

	h_div = scale->h_div;
	v_div = scale->v_div;
	h_mul = scale->h_mul;
	v_mul = scale->v_mul;

	if (h_mul == h_div) {
		h_mul = 1;
		h_div = 1;
		h_scale_dir = SCALE_DIR_OFF;
	} else if (h_mul > h_div) {
		h_scale_dir = SCALE_DIR_UP;
	} else {
		h_scale_dir = SCALE_DIR_DOWN;
	}

	if (v_mul == v_div) {
		v_mul = 1;
		v_div = 1;
		v_scale_dir = SCALE_DIR_OFF;
	} else if (v_mul > v_div) {
		v_scale_dir = SCALE_DIR_UP;
	} else {
		v_scale_dir = SCALE_DIR_DOWN;
	}

	h_scale_dir |= v_scale_dir;

	if (h_scale_dir == SCALE_DIR_OFF)
		fbdev->scale_dir = SCALE_DIR_OFF;
	else if (h_scale_dir == SCALE_DIR_DOWN) {
		fbdev->scale_dir = SCALE_DIR_DOWN;
	} else {
		scale_ep_on = SCALE_EP_ON;
		fbdev->scale_dir = SCALE_DIR_UP;
	}

	fbdev->scale_ep_on = scale_ep_on;
	fbdev->h_div = h_div;
	fbdev->h_mul = h_mul;
	fbdev->v_div = v_div;
	fbdev->v_mul = v_mul;

	h_scale = h_div * 4096 / h_mul;
	v_scale = v_div * 4096 / v_mul;
	fbdev->h_scale = h_scale;
	fbdev->v_scale = v_scale;

	if (fbdev->scale_dir != SCALE_DIR_OFF) {
		set_scale_coef(fbdev);
	}

	hcfb_update(fbdev, &info->var);
	gma_dmba_update(fbdev);
}

#define __clip(x, lower, upper) (((x) < (lower)) ? (lower) : ((x) > (upper)) ? (upper) : (x))
static void palette_yuv2rgb(uint8_t *bgra, uint8_t *yuva, bool is_video_matrix,
			     bool is_Bt709)
{
	int R, G, B;
	uint8_t Y, U, V, k;

	k = yuva[3];
	Y = yuva[2];
	U = yuva[1];
	V = yuva[0];

	if (!is_video_matrix) {
		Y = __clip(Y, 16, 235);
		U = __clip(U, 16, 240);
		V = __clip(V, 16, 240);
		if (!is_Bt709) {
			R = (298 * (Y - 16) + 0   * (U - 128) + 409 * (V - 128) + 128) / 256;
			G = (298 * (Y - 16) - 100 * (U - 128) - 208 * (V - 128) + 128) / 256;
			B = (298 * (Y - 16) + 517 * (U - 128) + 0   * (V - 128) + 128) / 256;
		} else {
			R = (298 * (Y - 16) + 0   * (U - 128) + 459 * (V - 128) + 128) / 256;
			G = (298 * (Y - 16) - 54  * (U - 128) - 136 * (V - 128) + 128) / 256;
			B = (298 * (Y - 16) + 541 * (U - 128) + 0   * (V - 128) + 128) / 256;
		}
	} else {
		if (!is_Bt709) {
			R = (256 * (Y) + 0   * (U - 128) + 351 * (V - 128) + 128) / 256;
			G = (256 * (Y) - 86  * (U - 128) - 179 * (V - 128) + 128) / 256;
			B = (256 * (Y) + 444 * (U - 128) + 0   * (V - 128) + 128) / 256;
		} else {
			R = (256 * (Y) + 0   * (U - 128) + 394 * (V - 128) + 128) / 256;
			G = (256 * (Y) - 47  * (U - 128) - 117 * (V - 128) + 128) / 256;
			B = (256 * (Y) + 464 * (U - 128) + 0   * (V - 128) + 128) / 256;
		}
	}

	bgra[2] = __clip(R, 0, 255);
	bgra[1] = __clip(G, 0, 255);
	bgra[0] = __clip(B, 0, 255);
	bgra[3] = (k << 4) + k;
}

static void alloc_palette(struct hcfb_device *fbdev)
{
	if (fbdev->palette_alloc)
		return;

	if (fbdev->use_static_header) {
		unsigned long addr = (unsigned long)fbdev->static_header + fbdev->static_header_offset;
		fbdev->palette_alloc = (uint8_t *)addr;
		memset(fbdev->palette_alloc, 0, 2048);
		fbdev->palette_hw = MIPS_UNCACHED_ADDR(fbdev->palette_alloc);
		fbdev->palette_sw = fbdev->palette_hw + 1024;
		fbdev->static_header_offset += 2048;
	} else {
		fbdev->palette_alloc = memalign(32, 2048);
		assert(fbdev->palette_alloc != NULL);
		memset(fbdev->palette_alloc, 0, 2048);
		fbdev->palette_hw = MIPS_UNCACHED_ADDR(fbdev->palette_alloc);
		fbdev->palette_sw = fbdev->palette_hw + 1024;
	}
}

static void free_palette(struct hcfb_device *fbdev)
{
	if (!fbdev->palette_alloc)
		return;

	if (fbdev->use_static_header) {
		/* always keep the palette_alloc buffer since it can only be allocated from fbdev->static_header once */
		memset(fbdev->palette_alloc, 0, 2048);
	} else {
		free(fbdev->palette_alloc);
		fbdev->palette_alloc = NULL;
		fbdev->palette_hw = NULL;
		fbdev->palette_sw = NULL;
	}
}

static void set_palette(struct hcfb_device *fbdev, hcfb_palette_t *palette)
{
	uint8_t type = palette->palette_type;
	uint8_t alpha_range = palette->alpha_range;
	uint8_t rgb_order = palette->rgb_order;
	uint8_t alpha = 0, R = 0, G = 0, B = 0;
	uint8_t *src, *dst;
	int i;

	dst = fbdev->palette_sw;
	src = palette->palette;
	for (i = 0; i < 256; i++, src += 4, dst += 4) {
		if (i < palette->ncolors) {
			switch (rgb_order) {
			case HCFB_RGB_ORDER_ABGR:
				alpha = src[3];
				B = src[2];
				G = src[1];
				R = src[0];
				break;
			case HCFB_RGB_ORDER_RGBA:
				R = src[3];
				G = src[2];
				B = src[1];
				alpha = src[0];
				break;
			case HCFB_RGB_ORDER_BGRA:
				B = src[3];
				G = src[2];
				R = src[1];
				alpha = src[0];
				break;
			case HCFB_RGB_ORDER_ARGB:
			default:
				alpha = src[3];
				R = src[2];
				G = src[1];
				B = src[0];
				break;
			}
		} else {
			alpha = 0;
			if (type == HCFB_PALETTE_YCBCR) {
				R = 0x10;
				B = 0x80;
				G = 0x80;
			} else {
				R = 0;
				B = 0;
				G = 0;
			}
		}

		dst[3] = alpha;
		dst[2] = R;
		dst[1] = G;
		dst[0] = B;
	}

	if (alpha_range != HCFB_ALPHA_RANGE_255) {
		dst = fbdev->palette_sw;
		for (i = 0; i < 256; i++, dst += 4) {
			alpha = dst[3];
			if (alpha_range == HCFB_ALPHA_RANGE_127) {
				alpha <<= 1;
				alpha |= (dst[3] >> 6);
			} else {
				/* HCFB_ALPHA_RANGE_127 */
				alpha <<= 4;
				alpha |= dst[3];
			}
			dst[3] = alpha;
		}
	}

	fbdev->palette_type_sw = type;
	fbdev->palette_rgb_order_sw = rgb_order;
	fbdev->palette_alpha_range_sw = alpha_range;
	fbdev->palette_ncolors_sw = 256;

	if (fbdev->palette_type_sw == HCFB_PALETTE_YCBCR) {
		for (i = 0; i < fbdev->palette_ncolors_sw; i++)
			palette_yuv2rgb(fbdev->palette_hw + i * 4,
					 fbdev->palette_sw + i * 4, true, false);
	} else {
		memcpy(fbdev->palette_hw, fbdev->palette_sw, 1024);
	}
}

static int eval_filter_select(const struct hcfb_device *fbdev)
{
	int sel = fbdev->filter_select;
	if (sel == -1) {
		switch (fbdev->filter_preference) {
			case FILT_PREF_DEFAULT:
			case FILT_PREF_FORCE_BL:
				sel = FILTER_SELECT_TAP_2;
				break;
			case FILT_PREF_FORCE_4X3:
				sel = FILTER_SELECT_TAP_3;
				break;
			case FILT_PREF_AUTO: {
				const int in_res = fbdev->h_div;
				const int out_res = fbdev->h_mul;
				if (in_res <= out_res) {
					// scale up, to save memory bandwidth
					sel = FILTER_SELECT_TAP_2;
				} else if (in_res * 3 < out_res * 4) {
					// decent scaling down ratio
					sel = FILTER_SELECT_TAP_2;
				} else {
					// large ratio scaling down
					sel = FILTER_SELECT_TAP_3;
				}
				break;
			}
			default:
				sel = FILTER_SELECT_TAP_2;
				break;
		}
	}
	// printf("fb eval_filter_select=%d\n", sel);
	return sel;
}

static int gma_dmba_update(struct hcfb_device *fbdev)
{
	gma_dmba_block_t *block = fbdev->block[fbdev->sw_block_id];
	gma_header_t *header = &block->header;

	if (fbdev->fb_mem == NULL)
		return 0;

	taskENTER_CRITICAL();

	header->is_last_block = true;
	header->alpha_closed = false;
	if (fbdev->scale_dir == SCALE_DIR_OFF) {
		header->scale_on = false;
	} else {
		header->inc_h_frag = VALUE_GET_BITS(fbdev->h_scale, 0, 12);
		header->inc_h_int = VALUE_GET_BITS(fbdev->h_scale, 12, 4);
		header->inc_v_frag = VALUE_GET_BITS(fbdev->v_scale, 0, 12);
		header->inc_v_int = VALUE_GET_BITS(fbdev->v_scale, 12, 4);
		if (fbdev->scale_dir == SCALE_DIR_UP) {
			header->scale_on = true;
			header->edge_preserve_on = fbdev->scale_ep_on;
		} else if (fbdev->scale_dir == SCALE_DIR_DOWN) {
			header->scale_on = true;
			header->edge_preserve_on = false;
		}
	}

	header->gma_mode = fbdev->color_format;
	header->color_by_color = !!!fbdev->global_alpha_on;
	header->global_alpha  = fbdev->global_alpha;
	header->clut_mode = fbdev->clut_mode;
	if (fbdev->color_format >= HCFB_FMT_CLUT2 && fbdev->color_format <= HCFB_FMT_ACLUT88) {
		header->clut_base = (uint32_t)PHY_ADDR(fbdev->palette_hw);
		header->clut_segment = fbdev->clut_segment;
		header->clut_update = true;
		header->rgb_order = fbdev->palette_rgb_order_sw;
	} else {
		header->clut_base = 0;
		header->clut_update = false;
		header->rgb_order = HCFB_RGB_ORDER_ARGB;
	}

	header->pre_multiply = fbdev->pre_multiply;
	header->csc_mode = fbdev->enhance.cscmode;
	header->edge_preserve_reduce_thr = 32;
	header->edge_preserve_avg_thr = 170;

	header->sx = fbdev->sx;
	header->ex = fbdev->ex;
	header->sy = fbdev->sy;
	header->ey = fbdev->ey;
	header->src_w = fbdev->src_w;
	header->src_h = fbdev->src_h;
	header->pitch = fbdev->pitch;
	header->next = 0;
	header->bitmap_addr = (uint32_t)PHY_ADDR(fbdev->fb_mem + fbdev->off_pitch);
	header->scale_mode  = fbdev->scale_mode;
	header->filter_select = eval_filter_select(fbdev);

	memcpy((void *)block->buf + 576, fbdev->enhance_coef, sizeof(fbdev->enhance_coef));
	memcpy((void *)block->buf + 64, fbdev->filter_coef, sizeof(fbdev->filter_coef));

	gma_update_screen(fbdev, (uint32_t)header);
	fbdev->sw_block_id = (fbdev->sw_block_id + 1) % 2;

	taskEXIT_CRITICAL();
	return 0;
}

static void hcfb_notify_vblank(void *arg, unsigned long param)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)arg;

	fbdev->vblank_count++;
	wake_up(&fbdev->wait);
}

static int hcfb_set_color_format(struct hcfb_device *fbdev, struct fb_var_screeninfo *var)
{
	const int clut_comb = fbdev->id == 0 ? 1 : 2;
	if (var->bits_per_pixel == 2) {
		fbdev->color_format = HCFB_FMT_CLUT2;
		gma_set_clut_assign(fbdev, clut_comb);
	} else if (var->bits_per_pixel == 4) {
		fbdev->color_format = HCFB_FMT_CLUT4;
		gma_set_clut_assign(fbdev, clut_comb);
	} else if (var->bits_per_pixel == 8) {
		fbdev->color_format = HCFB_FMT_CLUT8;
		gma_set_clut_assign(fbdev, clut_comb);
	} else if (var->bits_per_pixel == 16) {
		if (var->green.length == 6) {
			fbdev->color_format = HCFB_FMT_RGB565;
		} else if (var->green.length == 5) {
			fbdev->color_format = HCFB_FMT_ARGB1555;
		} else {
			if (var->transp.length == 0) {
				fbdev->color_format = HCFB_FMT_RGB444;
			} else if (var->transp.length == 4) {
				fbdev->color_format = HCFB_FMT_ARGB4444;
			} else {
				return -1;
			}
		}
	} else if (var->bits_per_pixel == 32) {
		if (var->transp.length == 8) {
			fbdev->color_format = HCFB_FMT_ARGB8888;
		} else if (var->transp.length == 0) {
			fbdev->color_format = HCFB_FMT_RGB888;
		} else {
			return -1;
		}
	} else {
		return -1;
	}

	return 0;
}

static void hcfb_hwdeinit(struct hcfb_device *fbdev,
			  struct fb_var_screeninfo *var)
{
	if (fbdev->vblank_ntkey != -1) {
		work_notifier_teardown(fbdev->vblank_ntkey);
		fbdev->vblank_ntkey = -1;
	}
	fbdev->block[0] = NULL;
	fbdev->block[1] = NULL;
}

static void hcfb_hwinit(struct hcfb_device *fbdev,
			  struct fb_var_screeninfo *var)
{
	hcfb_palette_t palette = default_palette;
	uint32_t x, y, w, h, h_mul, h_div, v_mul, v_div;
	hcfb_viewport_t *vp = &fbdev->view_port;

	init_waitqueue_head(&fbdev->wait);

	fbdev->vblank_ntkey = -1;

	hcfb_set_color_format(fbdev, var);

	fbdev->bits_per_pixel = var->bits_per_pixel;
	fbdev->green_length = var->green.length;
	fbdev->transp_length = var->transp.length;

	fbdev->filter_select = -1;
	fbdev->clut_mode = CLUT_MODE_DMA;
	fbdev->global_alpha = 0xff;
	fbdev->global_alpha_on = false;
	fbdev->enhance.cscmode = HCFB_ENHANCE_CSCMODE_BT709;
	fbdev->enhance.brightness = 50;
	fbdev->enhance.contrast = 50;
	fbdev->enhance.saturation = 50;
	fbdev->enhance.hue = 50;
	fbdev->enhance.sharpness = 5;

	if (fbdev->use_static_header) {
		if (!fbdev->block[0]) {
			unsigned long addr = (unsigned long)fbdev->static_header + fbdev->static_header_offset;
			fbdev->block[0] = (gma_dmba_block_t *)MIPS_UNCACHED_ADDR(addr);
			fbdev->block[1] = fbdev->block[0] + 1;
			fbdev->static_header_offset += sizeof(gma_dmba_block_t) * 2;
		}
	} else {
		fbdev->block[0] = (gma_dmba_block_t *)MIPS_UNCACHED_ADDR(ALIGN((unsigned long)fbdev->inlineblock, 32));
		fbdev->block[1] = fbdev->block[0] + 1;
	}
	fbdev->palette_type_hw = HCFB_PALETTE_RGB; /* Always RGB for HW */

	fbdev->pitch = eval_pitch_byte(var->xres_virtual, var->bits_per_pixel);
	fbdev->src_w = var->xres;
	fbdev->src_h = var->yres;
	fbdev->view_port.x = 0;
	fbdev->view_port.fit_mode = 0;
	fbdev->view_port.y = 0;

	generate_enhance_coef(fbdev->enhance_coef, &fbdev->enhance);

	set_csc_bypass(fbdev, fbdev->csc_bypass);
	set_enhance_sharpness(fbdev, 1);
	set_global_alpha(fbdev, 0xff);

	x = 0;
	y = 0;
	w = var->xres;
	h = var->yres;
	h_mul = fbdev->h_mul;
	h_div = fbdev->h_div;
	v_mul = fbdev->v_mul;
	v_div = fbdev->v_div;

	fbdev->sx = max(x * h_mul / h_div + vp->x, 0);
	fbdev->sy = max(y * v_mul / v_div + vp->y, 0);
	fbdev->ex = min((x + w) * h_mul / h_div + vp->x, vp->x + vp->width) - 1;
	fbdev->ey = min((y + h) * v_mul / v_div + vp->y, vp->y + vp->height) - 1;

	if(fbdev->src_w == 1920 && fbdev->color_format == HCFB_FMT_ARGB8888){
		gma_set_line_buf_combination(fbdev, 0xb);
		printf("Error: to do ...\n");// need fine tune, pls ask window pic for help.
	}
	else if(fbdev->src_w <=1280){
		if(fb0_valid && fb1_valid)
			gma_set_line_buf_combination(fbdev, 0x2);
		else if(fb0_valid)
			gma_set_line_buf_combination(fbdev, 0xa);
		else if(fb1_valid)
			gma_set_line_buf_combination(fbdev, 0x12);
	}
	else
		gma_set_line_buf_combination(fbdev, 0x2);

	palette.palette = malloc(1024);
	assert(palette.palette != NULL);
	memset(palette.palette, 0, 1024);
	set_palette(fbdev, &palette);
	free(palette.palette);
}

static void hcfb_free_mem(struct fb_info *info)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)(info->par);

	free_palette(fbdev);

	if (fbdev->buffer_source == HCFB_BUFFER_SOURCE_NONE)
		return;

	if (fbdev->buffer_source == HCFB_BUFFER_SOURCE_MMZ0)
		mmz_free(0, fbdev->fb_mem);
	else if (fbdev->buffer_source == HCFB_BUFFER_SOURCE_STATIC_ALLOC){
		//nothing to do
	}else
		free(fbdev->fb_mem);

	fbdev->fb_mem = NULL;
	fbdev->fb_phys = 0;
	fbdev->fb_len = 0;
	info->fix.smem_start = 0;
	info->fix.smem_len = 0;
	info->screen_base = NULL;
	info->screen_size = 0;
}

static int hcfb_put_fix(struct fb_info *info, struct fb_fix_screeninfo *fix)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)(info->par);
	int layer_size;

	if (fbdev->buffer_source != HCFB_BUFFER_SOURCE_NONE)
		return 0;

	layer_size = eval_pitch_byte(info->var.xres_virtual, info->var.bits_per_pixel) * info->var.yres_virtual;

	if (fix->smem_len != layer_size)
		return -EINVAL;

	fbdev->fb_mem = (unsigned char *)MIPS_CACHED_ADDR(fix->smem_start);
	fbdev->fb_phys = (unsigned long)PHY_ADDR(fix->smem_start);
	fbdev->fb_len = layer_size;

	info->fix.smem_start = fbdev->fb_phys;
	info->fix.smem_len   = layer_size;
	info->screen_base = fbdev->fb_mem;
	info->screen_size = layer_size;

	gma_dmba_update(fbdev);

	return 0;
}

static int hcfb_alloc_mem(struct fb_info *info)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)(info->par);
	int layer_size;

	fbdev->pitch = eval_pitch_byte(info->var.xres_virtual, info->var.bits_per_pixel);
	info->fix.line_length = fbdev->pitch;

	if (fbdev->buffer_source == HCFB_BUFFER_SOURCE_NONE)
		return 0;

	layer_size = eval_pitch_byte(info->var.xres_virtual, info->var.bits_per_pixel) * info->var.yres_virtual;

	if (fbdev->fb_mem) {
		if (layer_size <= fbdev->fb_len)
			return 0;

		if (fbdev->buffer_source == HCFB_BUFFER_SOURCE_MMZ0 || fbdev->buffer_source == HCFB_BUFFER_SOURCE_STATIC_ALLOC) {
			printf("fail to allocate framebuffer (size: %dK))", layer_size / 1024);
			return -ENOMEM;
		} else {
			printf("layer_size (%dK) is larger than fb_len (%ldK)", layer_size / 1024, fbdev->fb_len / 1024);
			free(fbdev->fb_mem);
			fbdev->fb_mem = NULL;
			return hcfb_alloc_mem(info);
		}
	} else {
		layer_size += fbdev->extra_fb_len;
		if (fbdev->buffer_source == HCFB_BUFFER_SOURCE_MMZ0) {
			fbdev->fb_mem = mmz_malloc(0, layer_size);
		} else if (fbdev->buffer_source == HCFB_BUFFER_SOURCE_STATIC_ALLOC) {
			if (fbdev->use_static_header) {
				if (!fbdev->static_header) {
					fbdev->static_header = (void *)MIPS_CACHED_ADDR(fbdev->fb_static_phys);
					memset(fbdev->static_header, 0, fbdev->static_header_size);
				}
				fbdev->fb_mem = fbdev->static_header + fbdev->static_header_size;
				fbdev->fb_phys = fbdev->fb_static_phys + fbdev->static_header_size;
				layer_size = fbdev->fb_static_len - fbdev->static_header_size;
			} else {
				fbdev->fb_mem = (void *)MIPS_CACHED_ADDR(fbdev->fb_static_phys);
				fbdev->fb_phys = fbdev->fb_static_phys;
				layer_size = fbdev->fb_static_len;
			}
			printf("fbdev->static_header: %p\n", fbdev->static_header);
			printf("fbdev->fb_mem: %p\n", fbdev->fb_mem);
			printf("fbdev->fb_phys: %08lx\n", fbdev->fb_phys);
			printf("layer_size: %08x\n", (unsigned int)layer_size);
		} else {
			fbdev->fb_mem = malloc(layer_size);
		}

		if (!fbdev->fb_mem) {
			printf("fail to allocate framebuffer (size: %dK))", layer_size / 1024);
			return -ENOMEM;
		}

		fbdev->fb_len = layer_size;

		fbdev->fb_phys = (unsigned long)PHY_ADDR(fbdev->fb_mem);

		info->fix.smem_start = fbdev->fb_phys;
		info->fix.smem_len   = layer_size;
		info->screen_base = fbdev->fb_mem;
		info->screen_size = layer_size;
	}

	alloc_palette(fbdev);

	/*memset(fbdev->fb_mem, 0, layer_size);*/

	return 0;
}

static void hcfb_set_default_fix(struct fb_fix_screeninfo *fix)
{
	fix->xpanstep = 1;		/* zero if no hardware panning  */
	fix->ypanstep = 1;		/* zero if no hardware panning  */
	fix->ywrapstep = 1;		/* zero if no hardware ywrap    */
}

static void hcfb_set_default_var(struct fb_var_screeninfo *var)
{
	var->xres = 1280;
	var->yres = 720;
	var->xres_virtual = 1280;
	var->yres_virtual = 720;
	var->xoffset = 0;
	var->yoffset = 0;
	var->bits_per_pixel = 32;

	var->red = (struct fb_bitfield){ 16, 8, 0 };
	var->green = (struct fb_bitfield){ 8, 8, 0 };
	var->blue = (struct fb_bitfield){ 0, 8, 0 };
	var->transp = (struct fb_bitfield){ 24, 8, 0 };
}

static void hcfb_set_default_par(struct hcfb_device *fbdev)
{
	fbdev->scale_mode = SCALE_MODE_FILTER;
	fbdev->buffer_source = HCFB_BUFFER_SOURCE_SYSTEM;
}

static int hcfb_blank(int blank, struct fb_info *info)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)info->par;
	if (blank == FB_BLANK_UNBLANK) {
		gma_show_onoff(fbdev, 1);
	} else if (blank == FB_BLANK_NORMAL) {
		gma_show_onoff(fbdev, 0);
	} else {
		return -1;
	}

	return 0;
}

static int hcfb_waitforvblank(struct hcfb_device *fbdev)
{
	unsigned int count = fbdev->vblank_count;
	int ret;
	struct work_notifier_s info = { 0 };

	info.evtype = DIS_NOTIFY_VBLANK;
	info.qid = HPWORK;
	info.remote = false;
	info.oneshot = true;
	info.qualifier = NULL;
	info.arg = fbdev;
	info.worker2 = hcfb_notify_vblank;
	fbdev->vblank_ntkey = work_notifier_setup(&info);
	ret = wait_event_timeout(fbdev->wait, count != fbdev->vblank_count, 33);
	if (ret == 0) {
		ret = -1;
		work_notifier_teardown(fbdev->vblank_ntkey);
		fbdev->vblank_ntkey = -1;
	} else {
		ret = 0;
	}

	return ret;
}

static void hcfb_notify_dechange(void *arg, unsigned long param)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)arg;
	enum DIS_DEV_TYPE de = get_current_de();
	uint32_t old_iobase = (uint32_t)fbdev->iobase;
	uint32_t new_iobase = (uint32_t)fbdev->iobase;

	if (de == DE_TYPE_4K) {
		if ((old_iobase & 0xff000) == 0x08000)
			new_iobase = (old_iobase & ~0xff000) | 0x3a000;
	} else {
		if ((old_iobase & 0xff000) == 0x3a000)
			new_iobase = (old_iobase & ~0xff000) | 0x08000;
	}

	if (old_iobase != new_iobase) {
		taskENTER_CRITICAL();
		fbdev->iobase = (void *)new_iobase;
		gma_set_mask(fbdev);
		REG32_WRITE(fbdev->iobase + HC_GMA_K,
			    REG32_READ(old_iobase + HC_GMA_K));
		REG32_WRITE(fbdev->iobase + HC_GMA_LINEBUF,
			    REG32_READ(old_iobase + HC_GMA_LINEBUF));
		REG32_WRITE(fbdev->iobase + HC_GMA_DMBA,
			    REG32_READ(old_iobase + HC_GMA_DMBA));
		REG32_WRITE(fbdev->iobase + HC_GMA_CTL,
			    REG32_READ(old_iobase + HC_GMA_CTL));
		gma_clear_mask(fbdev);

		taskEXIT_CRITICAL();
	}
}

static void hcfb_monitor_dechange(struct hcfb_device *fbdev)
{
	struct work_notifier_s info = { 0 };

	info.evtype = DIS_NOTIFY_DE_TYPE_CHANGE;
	info.qid = HPWORK;
	info.remote = false;
	info.oneshot = false;
	info.qualifier = NULL;
	info.arg = fbdev;
	info.worker2 = hcfb_notify_dechange;
	fbdev->vblank_ntkey = work_notifier_setup(&info);
}

static int hcfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)info->par;

	fbdev->off_pitch = info->fix.line_length * var->yoffset +
			     aligned_div_log2(var->xoffset * fbdev->bits_per_pixel, 3); // TODO have problem in CLUT2/CLUT4

	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;

	hcfb_update(fbdev, &info->var);
	gma_dmba_update(fbdev);

	return 0;
}

static int hcfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	if (((var->xoffset + var->xres) > var->xres_virtual) ||
	    ((var->yoffset + var->yres) > var->yres_virtual)) {
		return -1;
	}

	if (var->bits_per_pixel <= 2) {
		var->bits_per_pixel = 2;
	} else if (var->bits_per_pixel <= 4) {
		var->bits_per_pixel = 4;
	} else if (var->bits_per_pixel <= 8) {
		var->bits_per_pixel = 8;
	} else if (var->bits_per_pixel <= 16) {
		var->bits_per_pixel = 16;
	} else if (var->bits_per_pixel <= 32) {
		var->bits_per_pixel = 32;
	}

	switch (var->bits_per_pixel) {
	case 2:
		var->red.offset = 0;
		var->red.length = 2;
		var->red.msb_right = 0;
		var->green.offset = 0;
		var->green.length = 0;
		var->green.msb_right = 0;
		var->blue.offset = 0;
		var->blue.length = 0;
		var->blue.msb_right = 0;
		var->transp.offset = 0;
		var->transp.length = 0;
		var->transp.msb_right = 0;
		break;

	case 4:
		var->red.offset = 0;
		var->red.length = 4;
		var->red.msb_right = 0;
		var->green.offset = 0;
		var->green.length = 0;
		var->green.msb_right = 0;
		var->blue.offset = 0;
		var->blue.length = 0;
		var->blue.msb_right = 0;
		var->transp.offset = 0;
		var->transp.length = 0;
		var->transp.msb_right = 0;
		break;

	case 8:
		var->red.offset = 0;
		var->red.length = 8;
		var->red.msb_right = 0;
		var->green.offset = 0;
		var->green.length = 0;
		var->green.msb_right = 0;
		var->blue.offset = 0;
		var->blue.length = 0;
		var->blue.msb_right = 0;
		var->transp.offset = 0;
		var->transp.length = 0;
		var->transp.msb_right = 0;
		break;

	case 16:
		if (var->xres_virtual > 2560) {
			fb_err(info,
			       "don't support > 2560 x resolution when 16bit output\n");
			return -1;
		}

		if (var->green.length <= 4) {
			var->green.length = 4;
		} else if (var->green.length <= 5) {
			var->green.length = 5;
		} else {
			var->green.length = 6;
		}

		var->blue.offset = 0;
		var->blue.length =
			(var->green.length == 6) ? 5 : var->green.length;
		var->blue.msb_right = 0;
		var->green.offset = var->blue.offset + var->blue.length;
		var->green.msb_right = 0;
		var->red.offset = var->green.offset + var->green.length;
		var->red.length =
			(var->green.length == 6) ? 5 : var->green.length;
		var->red.msb_right = 0;
		if (var->transp.length == 0) {
			var->transp.offset = 0;
			var->transp.length = 0;
		} else {
			var->transp.offset =
				(var->red.offset + var->red.length) & 0xF;
			var->transp.length = 16 - var->transp.offset;
		}
		var->transp.msb_right = 0;
		break;

	case 32:
		var->red.offset = 16;
		var->red.length = 8;
		var->red.msb_right = 0;
		var->green.offset = 8;
		var->green.length = 8;
		var->green.msb_right = 0;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->blue.msb_right = 0;
		var->transp.offset = 24;
		if (var->transp.length) {
			var->transp.length = 8;
		}
		var->transp.msb_right = 0;
		break;

	default:
		return -1;
	}

	return 0;
}

static int hcfb_set_par(struct fb_info *info)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)info->par;
	struct fb_var_screeninfo *var = &info->var;
	int ret = 0;
	bool onoff = fbdev->gma_onoff;

	info->fix.line_length = eval_pitch_byte(var->xres_virtual, var->bits_per_pixel);

	if (fbdev->hwinited) {
		if (fbdev->bits_per_pixel == var->bits_per_pixel &&
		    fbdev->src_w == var->xres && fbdev->src_h == var->yres &&
		    fbdev->green_length == var->green.length &&
		    fbdev->transp_length == var->transp.length) {
			if (onoff&&!(var->vmode & FB_VMODE_YWRAP)) {
				gma_show_onoff(fbdev, 1);
			}
			return 0;
		}
		if(onoff)
			gma_show_onoff_sync(fbdev, 0);
	}

	hcfb_set_color_format(fbdev, var);

	ret = hcfb_alloc_mem(info);
	if (ret) {
		fb_err(info,
		       "Failed to alloc memory for framebuffer device: %d\n",
		       ret);
		return ret;
	}

	hcfb_update(fbdev, var);
	gma_dmba_update(fbdev);

	if (onoff&&(!(var->vmode & FB_VMODE_YWRAP))) {
		gma_show_onoff(fbdev, 1);
	}

	return 0;
}

static int hcfb_ioctl(struct fb_info *info, u32 cmd, unsigned long arg)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)info->par;
	int rc = 0;
	unsigned long on = 0;

	switch (cmd) {
	case HCFBIOSET_SCALE:
		assert((void *)arg != NULL);
		hcfb_set_scale(info, (hcfb_scale_t *)arg);
		rc = 0;
		break;

	case HCFBIOGET_SCALE_ONOFF:
		if(fbdev->scale_dir != SCALE_DIR_OFF)
			on = 1;
		*(unsigned long *)arg = on;
		break;

	case HCFBIOGET_ENHANCE:
		assert((void *)arg != NULL);
		memcpy((void *)arg, &fbdev->enhance, sizeof(hcfb_enhance_t));
		rc = 0;
		break;

	case HCFBIOSET_ENHANCE:
		assert((void *)arg != NULL);
		memcpy(&fbdev->enhance, (void *)arg, sizeof(hcfb_enhance_t));
		generate_enhance_coef(fbdev->enhance_coef, &fbdev->enhance);
		gma_dmba_update(fbdev);
		rc = 0;
		break;

	case HCFBIOSET_ENHANCE_COEF:
		memcpy(fbdev->enhance_coef, (void *)arg, 12*4);
		gma_dmba_update(fbdev);
		rc = 0;
		break;

	case HCFBIOPUT_FSCREENINFO:
		hcfb_put_fix(info, (struct fb_fix_screeninfo *)arg);
		rc = 0;
		break;

	case FBIO_WAITFORVSYNC:
		rc = hcfb_waitforvblank(fbdev);
		break;

	case HCFBIOSET_MMAP_CACHE:
		if ((hcfb_mmap_cache_e)arg == HCFB_MMAP_CACHE)
			fbdev->mmap_cache = HCFB_MMAP_CACHE;
		else
			fbdev->mmap_cache = HCFB_MMAP_NO_CACHE;
		break;
	case HCFBIOSET_SET_LEFTTOP_POS:
		assert((void *)arg != NULL);
		hcfb_set_left_top(info,(hcfb_lefttop_pos_t *)arg);
		rc = 0;
		break;
	case HCFBIOSET_SET_VIEWPORT:
		assert((void *)arg != NULL);
		hcfb_set_viewport(info,(hcfb_viewport_t *)arg);
		rc = 0;
		break;
	default:
		break;
	}

	return 0;
}

static int hcfb_open(struct fb_info *info, int user)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)info->par;
	int ret;

	if (info == NULL) {
		fb_err(info, " info is null, 0x%x\n", (unsigned int)info);
		return -1;
	}

	if (fbdev == NULL) {
		fb_err(info, "fbdev is %x\n", (unsigned int)fbdev);
		return -1;
	}

	ret = hcfb_alloc_mem(info);
	if (ret) {
		fb_err(info,
		       "Failed to alloc memory for framebuffer device: %d\n",
		       ret);
		return ret;
	}

	if (fbdev->hwinited == 0) {
		hcfb_hwinit(fbdev, &info->var);
		fbdev->hwinited = 1;
	}

	gma_dmba_update(fbdev);

	if (user) {
		fbdev->open_cnt++;
	}

	return 0;
}

static int hcfb_release(struct fb_info *info, int user)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)info->par;

	if (user) {
		fbdev->open_cnt--;
	}

	if (fbdev->open_cnt == 0 && fbdev->hwinited == 1) {
		gma_show_onoff(fbdev, 0);
		hcfb_hwdeinit(fbdev, &info->var);
		//hcfb_free_mem(info);
		fbdev->hwinited = 0;
	}

	return 0;
}

static int hcfb_setcolreg(unsigned regno, unsigned red, unsigned green,
                           unsigned blue, unsigned transp, struct fb_info *info)
{
	return 0;
}

static int hcfb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	struct hcfb_device *fbdev = (struct hcfb_device *)info->par;
	int i = 0;
	unsigned short *red = NULL;
	unsigned short *green = NULL;
	unsigned short *blue = NULL;
	unsigned short *transp = NULL;
	unsigned char *palette = NULL;

	red = cmap->red;
	green = cmap->green;
	blue = cmap->blue;
	transp = cmap->transp;
	palette = fbdev->palette_sw;

	for (i = 0; i < cmap->len; i++) {
		*palette++ = *(u8 *)blue;
		blue++;
		*palette++ = *(u8 *)green;
		green++;
		*palette++ = *(u8 *)red;
		red++;
		if (transp) {
			*palette++ = *(u8 *)transp;
			transp++;
		} else {
			*palette++ = 0xff;
		}
	}

	fbdev->palette_ncolors_sw = cmap->len;
	fbdev->palette_type_sw = HCFB_PALETTE_RGB;
	fbdev->palette_alpha_range_sw = HCFB_ALPHA_RANGE_255;
	fbdev->palette_rgb_order_sw = HCFB_RGB_ORDER_ARGB;

	memcpy(fbdev->palette_hw, fbdev->palette_sw, 1024);
	gma_dmba_update(fbdev);
	return 0;
}

static struct fb_ops hcfb_ops = {
	.fb_open = hcfb_open,
	.fb_release = hcfb_release,
	.fb_ioctl = hcfb_ioctl,
	.fb_check_var = hcfb_check_var,
	.fb_set_par = hcfb_set_par,
	.fb_setcolreg = hcfb_setcolreg,
	.fb_setcmap = hcfb_setcmap,
	.fb_pan_display = hcfb_pan_display,
	.fb_blank = hcfb_blank,
};

static
int get_property_u8(int offset, const char *name, u8 default_val, u8 *out)
{
	u32 val;
	if (fdt_get_property_u_32_index(offset, name, 0, &val) == 0) {
		*out = (uint8_t)val;
	} else {
		*out = default_val;
	}
}

static int hcfb_probe(const char *node, int id)
{
	struct fb_info *info = NULL;
	struct hcfb_device *fbdev = NULL;
	const char *buffer_source = NULL;
	u32 iobase = 0;
	hcfb_scale_t scale;
	u32 h_div = 1, v_div = 1, h_mul = 1, v_mul = 1;
	int ret, np;
	u32 fb_static_phys = 0;
	u32 val;
	enum DIS_DEV_TYPE de;
	int dual_de_enable = get_dual_de_enable();

	np = fdt_node_probe_by_path(node);
	if (np < 0)
		return 0;

	if (fdt_get_property_u_32_index(np, "reg", 0, &iobase))
		return 0;
	if (dual_de_enable == 0) {
		de = get_current_de();
		if (de == DE_TYPE_4K) {
			if ((iobase & 0xff000) == 0x08000)
				iobase = (iobase & ~0xff000) | 0x3a000;
		} else {
			if ((iobase & 0xff000) == 0x3a000)
				iobase = (iobase & ~0xff000) | 0x08000;
		}
	}
	info = framebuffer_alloc(sizeof(struct hcfb_device));
	if (!info) {
		return -ENOMEM;
	}

	ret = fb_alloc_cmap(&info->cmap, 256, 1);
	if (ret < 0) {
		printf("alloc cmap fail\n");
		goto err_fb;
	}

	info->fbops = &hcfb_ops;
	info->node = id;

	fbdev = (struct hcfb_device *)info->par;
	memset((void *)fbdev, 0, sizeof(struct hcfb_device));
	fbdev->id = info->node;
	fbdev->iobase = (void *)(iobase | 0xa0000000);
	hcfb_set_default_var(&info->var);
	hcfb_set_default_fix(&info->fix);
	hcfb_set_default_par(fbdev);
	fdt_get_property_u_32_index(np, "bits_per_pixel", 0, (u32 *)&info->var.bits_per_pixel);
	fdt_get_property_u_32_index(np, "xres", 0, (u32 *)&info->var.xres);
	fdt_get_property_u_32_index(np, "yres", 0, (u32 *)&info->var.yres);
	fdt_get_property_u_32_index(np, "xres_virtual", 0, (u32 *)&info->var.xres_virtual);
	fdt_get_property_u_32_index(np, "yres_virtual", 0, (u32 *)&info->var.yres_virtual);
	fdt_get_property_u_32_index(np, "xoffset", 0, (u32 *)&info->var.xoffset);
	fdt_get_property_u_32_index(np, "yoffset", 0, (u32 *)&info->var.yoffset);
	fdt_get_property_u_32_index(np, "extra-buffer-size", 0, (u32 *)&fbdev->extra_fb_len);
	ret = hcfb_check_var(&info->var, info);
	if (ret) {
		goto err_fb;
	}

	get_property_u8(np, "pre-multiply", true, &fbdev->pre_multiply);
	get_property_u8(np, "csc-bypass", false, &fbdev->csc_bypass);

	fdt_get_property_u_32_index(np, "scale", 0, (u32 *)&h_div);
	fdt_get_property_u_32_index(np, "scale", 1, (u32 *)&v_div);
	fdt_get_property_u_32_index(np, "scale", 2, (u32 *)&h_mul);
	fdt_get_property_u_32_index(np, "scale", 3, (u32 *)&v_mul);
	scale.h_div = h_div;
	scale.v_div = v_div;
	scale.h_mul = h_mul;
	scale.v_mul = v_mul;
	fbdev->left = 0;
	fbdev->top = 0;
	fbdev->view_port.x = 0;
	fbdev->view_port.fit_mode = 0;
	fbdev->view_port.y = 0;
	fbdev->view_port.width = 0;
	fbdev->view_port.height = 0;
	fbdev->view_port.enable = 0;
	fbdev->view_port.apply_now = 0;

	hcfb_set_scale(info, &scale);
	
	if (fdt_get_property_u_32_index(np, "filt-pref", 0, &val) == 0) {
		fbdev->filter_preference = val;
	} else {
		fbdev->filter_preference = FILT_PREF_DEFAULT;
	}

	fdt_get_property_u_32_index(np, "buffer-phy-static", 0, (u32 *)&fb_static_phys);
	fdt_get_property_u_32_index(np, "buffer-phy-static", 1, (u32 *)&fbdev->fb_static_len);

	fbdev->fb_static_phys = (fb_static_phys + PAGE_SIZE - 1) & PAGE_MASK;
	fbdev->fb_static_len -= fbdev->fb_static_phys - fb_static_phys;

	if (!fdt_get_property_string_index(np, "buffer-source", 0, &buffer_source)) {
		if (!strcmp(buffer_source, "system"))
			fbdev->buffer_source = HCFB_BUFFER_SOURCE_SYSTEM;
		else if (!strcmp(buffer_source, "mmz0"))
			fbdev->buffer_source = HCFB_BUFFER_SOURCE_MMZ0;
		else if (!strcmp(buffer_source, "static"))
			fbdev->buffer_source = HCFB_BUFFER_SOURCE_STATIC_ALLOC;
		else if (!strcmp(buffer_source, "none"))
			fbdev->buffer_source = HCFB_BUFFER_SOURCE_NONE;
	}

	if (fdt_property_read_bool(np, "use-static-header")) {
		fdt_get_property_u_32_index(np, "buffer-header-size", 0, (u32 *)&fbdev->static_header_size);
		if (fbdev->static_header_size >= (2 * sizeof(gma_dmba_block_t) + 2048))
			fbdev->use_static_header = 1;
	}

	ret = hcfb_alloc_mem(info);
	if (ret) {
		goto err_fb;
	}

	ret = register_framebuffer(info);
	if (ret < 0) {
		fb_err(info, "Failed to register framebuffer device: %d\n", ret);
		goto err_hcfb;
	}

	if (fdt_property_read_bool(np, "default-on"))
		gma_show_onoff(fbdev, true);
	else
		gma_show_onoff(fbdev, false);
	if(id == 0)
		fb0_valid = true;
	else
		fb1_valid = true;

	hcfb_monitor_dechange(fbdev);
	return 0;

err_hcfb:
	hcfb_free_mem(info);
err_fb:
	fb_dealloc_cmap(&info->cmap);
	framebuffer_release(info);

	return -1;
}

const char * __attribute__((weak)) fdt_get_fb0_path(void)
{
	return "/hcrtos/fb0";
}

static int fb_init(void)
{
	int ret = 0;

	ret = hcfb_probe(fdt_get_fb0_path(),0);
	ret |= hcfb_probe("/hcrtos/fb1", 1);

	return ret;
}

module_driver(fb, fb_init, NULL, 1)
