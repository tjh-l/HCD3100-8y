#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#ifdef __HCRTOS__
#include <kernel/fb.h>
#include <hcuapi/fb.h>
#else
#include <linux/fb.h>
#endif
#include <stdint.h>
#include <stdlib.h>
#include <getopt.h>
#include <hcge/ge_api.h>

#pragma pack(push, 1)  // 确保结构按1字节对齐
typedef struct tagBITMAPFILEHEADER {
	unsigned short bfType;
	unsigned long bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned long bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
	unsigned long biSize;
	long biWidth;
	long biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long biCompression;
	unsigned long biSizeImage;
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	unsigned long biClrUsed;
	unsigned long biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)  // 恢复默认的结构对齐方式

static int fbdev = -1;
static struct fb_fix_screeninfo fix; /* Current fix */
static struct fb_var_screeninfo var; /* Current var */
static uint8_t *fb_base;
static uint8_t *fb_src;
static int fb_src_w;
static int fb_src_h;
#define HCGE_RED 0
#define HCGE_GREEN 1
#define HCGE_BLUE 2
#define HCGE_BLACK 3
#define HCGE_WHITE 4
static unsigned int default_colors[] = { 0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFF << 24, 0xFFFFFFFF, 0x0 }; /* 0x00RRGGBB */
static hcge_context *ctx = NULL;

static int hcge_openfb_main(int argc, char *argv[])
{
	if (fbdev < 0) {
		fbdev = open("/dev/fb0", O_RDWR);
	}

	if (fbdev < 0) {
		printf("open /dev/fb0 failed\r\n");
		return -1;
	}

	if (hcge_open(&ctx) != 0) {
		printf("Init hcge error.\n");
		close(fbdev);
		fbdev = -1;
		return -1;
	}
	ctx->log_en = true;

	ioctl(fbdev, FBIOGET_FSCREENINFO, &fix);
	ioctl(fbdev, FBIOGET_VSCREENINFO, &var);
	fb_base = (unsigned char *)mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0);
	memset(fb_base, 0, fix.smem_len);
	ioctl(fbdev, FBIOBLANK, FB_BLANK_UNBLANK);
	fb_src = fb_base + var.xres * var.yres * 4;
	return 0;
}

static int hcge_closefb_main(int argc, char *argv[])
{
	if (fbdev < 0)
		return -1;

	if (fb_base) {
		munmap(fb_base, fix.smem_len);
		fb_base = NULL;
	}

	if (ctx) {
		hcge_close(ctx);
		ctx = NULL;
	}

	close(fbdev);
	fbdev = -1;
	return 0;
}

static int hcge_fill_bmp(void *bmp, void *fb_buf, int xres, int yres, int x, int y, int *w, int *h)
{
	BITMAPFILEHEADER fileHeader;
	memcpy(&fileHeader, bmp, sizeof(fileHeader));

	if (fileHeader.bfType != 0x4D42) {  // 'BM' in little-endian
		printf("Not a BMP file.\n");
		return 1;
	}

	BITMAPINFOHEADER infoHeader;
	memcpy(&infoHeader, bmp + sizeof(BITMAPFILEHEADER), sizeof(infoHeader));

	printf("Width: %ld pixels\n", infoHeader.biWidth);
	printf("Height: %ld pixels\n", infoHeader.biHeight);

	int bytesPerPixel;
	switch (infoHeader.biBitCount) {
		case 24:
			bytesPerPixel = 3; // RGB
			break;
		case 32:
			bytesPerPixel = 4; // RGBA
			break;
		default:
			printf("Unsupported bit depth: %u\n", infoHeader.biBitCount);
			return 1;
	}

	if (bytesPerPixel != 4)
		return 1;

	long biHeight = infoHeader.biHeight;
	if (biHeight < 0)
		infoHeader.biHeight = -biHeight;
	size_t imageDataSize = infoHeader.biWidth * infoHeader.biHeight * bytesPerPixel;
	unsigned char *imageData;

	imageData = bmp + fileHeader.bfOffBits;

	uint32_t *fbp = (uint32_t *)fb_buf;
	if (xres == -1) {
		xres = infoHeader.biWidth;
		if (w)
			*w = infoHeader.biWidth;
	}
	if (yres == -1) {
		yres = infoHeader.biHeight;
		if (h)
			*h = infoHeader.biHeight;
	}

	if (biHeight > 0) {
		int linesize = infoHeader.biWidth * bytesPerPixel;
		void *ptr = imageData + (infoHeader.biHeight - 1) * linesize;
		for(int j = 0, i = infoHeader.biHeight - 1; i >= 0; i--, j++) {
			fbp = (uint32_t *)fb_buf + xres * (y + j) + x;
			memcpy(fbp, ptr, linesize);
			ptr -= linesize;
		}
	} else {
		int linesize = infoHeader.biWidth * bytesPerPixel;
		void *ptr = imageData;
		for(int i = 0; i < infoHeader.biHeight; i++) {
			fbp = (uint32_t *)fb_buf + xres * (y + i) + x;
			memcpy(fbp, ptr, linesize);
			ptr += linesize;
		}
	}

	return 0;
}

static int hcge_fill_binary(void *imageData, void *fb_buf, int xres, int yres, int x, int y, int w, int h)
{
	int linesize = w * 4;
	void *ptr = imageData;
	void *fbp;
	for(int i = 0; i < h; i++) {
		fbp = (uint32_t *)fb_buf + xres * (y + i) + x;
		memcpy(fbp, ptr, linesize);
		ptr += linesize;
	}

	return 0;
}

static void hcge_fill_usage(const char *prog)
{
	printf("Usage: %s [-cxywh]\n", prog);
	puts("  -c --color        specify color [red|green|blue|black|white|user defined 32-bit integer]\n"
	     "  -x --xpos         x position\n"
	     "  -y --ypos         y position\n"
	     "  -w --width        width\n"
	     "  -h --height       height\n"
	     "  -f --file         file source\n");
}

static int hcge_fill_main(int argc, char *argv[])
{
	int color = 0;
	int x = 0;
	int y = 0;
	int w = var.xres;
	int h = var.yres;
	const char *filename = NULL;

	while (1) {
		static const struct option lopts[] = {
			{ "color", 1, 0, 'c' },
			{ "xpos", 1, 0, 'x' },
			{ "ypos", 1, 0, 'y' },
			{ "width", 1, 0, 'w' },
			{ "height", 1, 0, 'h' },
			{ "file", 1, 0, 'f' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "c:x:y:w:h:f:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'c':
			if (!strcmp(optarg, "red"))
				color = default_colors[HCGE_RED];
			else if (!strcmp(optarg, "green"))
				color = default_colors[HCGE_GREEN];
			else if (!strcmp(optarg, "blue"))
				color = default_colors[HCGE_BLUE];
			else if (!strcmp(optarg, "black"))
				color = default_colors[HCGE_BLACK];
			else if (!strcmp(optarg, "white"))
				color = default_colors[HCGE_WHITE];
			else
				color = (int)strtoll(optarg, NULL, 0);
			break;
		case 'x':
			x = (int)strtoll(optarg, NULL, 0);
			break;
		case 'y':
			y = (int)strtoll(optarg, NULL, 0);
			break;
		case 'w':
			w = (int)strtoll(optarg, NULL, 0);
			break;
		case 'h':
			h = (int)strtoll(optarg, NULL, 0);
			break;
		case 'f':
			filename = optarg;
			break;
		default:
			hcge_fill_usage(argv[0]);
			return -1;
		}
	}

	if (filename) {
		struct stat sb;
		void *bmp = NULL;
		size_t bytesRead;
		FILE *file = NULL;

		if (stat(filename, &sb) != 0)
			return -1;

		size_t fsize = sb.st_size;
		file = fopen(filename, "rb");
		if (!file) {
			printf("Error opening file");
			return -1;
		} else {
			printf("Open %s success\r\n", filename);
		}
		bmp = malloc(fsize);
		if (!bmp) {
			printf("malloc fail\r\n");
			fclose(file);
			return -1;
		}

		bytesRead = fread(bmp, 1, fsize, file);
		if (bytesRead != fsize) {
			printf("read fail\r\n");
			fclose(file);
			free(bmp);
			return -1;
		}

		if (!strcmp(&filename[strlen(filename) - 4], ".bmp"))
			hcge_fill_bmp(bmp, fb_base, var.xres, var.yres, x, y, NULL, NULL);
		else
			hcge_fill_binary(bmp, fb_base, var.xres, var.yres, x, y, w, h);

		fclose(file);
		free(bmp);
	} else {
		uint32_t *fbp = (uint32_t *)fb_base;
		for (int i = 0; i < h; i++) {
			fbp = (uint32_t *)fb_base + var.xres * (y + i) + x;
			for (int j = 0; j < w; j++) {
				*fbp++ = (uint32_t)color;
			}
		}
	}

	return 0;
}

static void hcge_genimg_usage(const char *prog)
{
	printf("Usage: %s [-cwhmfd]\n", prog);
	puts("  -c --color        specify color [red|green|blue|black|white|user defined 32-bit integer]\n"
	     "  -w --width        image width\n"
	     "  -h --height       image height\n"
	     "  -m --mode         mode: 0\n"
	     "  -f --file         file source\n"
	     "  -d --dump         enable dump to <file-path>\n"
	     "                          AAAAAAA\n"
	     "                          BBBBBBB\n"
	     "                          CCCCCCC\n"
	     "                          DDDDDDD\n"
	     "                    mode: 1\n"
	     "                          ABCD\n"
	     "                          ABCD\n"
	     "                          ABCD\n"
	     "                          ABCD\n"
	     "                    mode: 2\n"
	     "                          ABCD\n"
	     "                          BCDA\n"
	     "                          CDAB\n"
	     "                          DABC\n");
}

static int hcge_genimg_main(int argc, char *argv[])
{
	unsigned int colors[128];
	int n_colors = 0;
	int mode = 0;
	const char *dump_path = NULL;
	const char *filename = NULL;
	FILE *fp;

	while (1) {
		static const struct option lopts[] = {
			{ "color", 1, 0, 'c' },
			{ "width", 1, 0, 'w' },
			{ "height", 1, 0, 'h' },
			{ "mode", 1, 0, 'm' },
			{ "dump", 1, 0, 'd' },
			{ "file", 1, 0, 'f' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "c:w:h:m:d:f:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'c':
			if (!strcmp(optarg, "red"))
				colors[n_colors++] = default_colors[HCGE_RED];
			else if (!strcmp(optarg, "green"))
				colors[n_colors++] = default_colors[HCGE_GREEN];
			else if (!strcmp(optarg, "blue"))
				colors[n_colors++] = default_colors[HCGE_BLUE];
			else if (!strcmp(optarg, "black"))
				colors[n_colors++] = default_colors[HCGE_BLACK];
			else if (!strcmp(optarg, "white"))
				colors[n_colors++] = default_colors[HCGE_WHITE];
			else
				colors[n_colors++] = (unsigned int)strtoul(optarg, NULL, 0);
			break;
		case 'w':
			fb_src_w = (int)strtoll(optarg, NULL, 0);
			break;
		case 'h':
			fb_src_h = (int)strtoll(optarg, NULL, 0);
			break;
		case 'm':
			mode = (int)strtoll(optarg, NULL, 0);
			break;
		case 'd':
			dump_path = optarg;
			break;
		case 'f':
			filename = optarg;
			break;
		default:
			hcge_genimg_usage(argv[0]);
			return -1;
		}
	}

	if (filename) {
		struct stat sb;
		void *bmp = NULL;
		size_t bytesRead;
		FILE *file = NULL;

		if (stat(filename, &sb) != 0)
			return -1;

		size_t fsize = sb.st_size;
		file = fopen(filename, "rb");
		if (!file) {
			printf("Error opening file");
			return -1;
		}
		bmp = malloc(fsize);
		if (!bmp) {
			printf("malloc fail\r\n");
			fclose(file);
			return -1;
		}

		bytesRead = fread(bmp, 1, fsize, file);
		if (bytesRead != fsize) {
			printf("read fail\r\n");
			fclose(file);
			free(bmp);
			return -1;
		}

		if (!strcmp(&filename[strlen(filename) - 4], ".bmp")) {
			hcge_fill_bmp(bmp, fb_src, -1, -1, 0, 0, &fb_src_w, &fb_src_h);
		} else {
			if (fb_src_w != 0 && fb_src_h != 0) {
				hcge_fill_binary(bmp, fb_src, fb_src_w, fb_src_h, 0, 0, fb_src_w, fb_src_h);
			} else {
				printf("please specify width and height for binary image source\r\n");
				hcge_genimg_usage(argv[0]);
			}
		}

		fclose(file);
		free(bmp);
		return 0;
	}

	if (fb_src_w == 0 || fb_src_h == 0 || n_colors == 0 || (mode == 3 && n_colors != 2)) {
		printf("please specify width and height\r\n");
		hcge_genimg_usage(argv[0]);
		return -1;
	}

	if (mode == 0) {
		uint32_t *fbp0 = (uint32_t *)fb_src;
		int color;
		int color_idx = 0;
		for (int i = 0; i < fb_src_h; i++) {
			color = colors[color_idx++]; 
			if (color_idx >= n_colors)
				color_idx = 0;
			for (int j = 0; j < fb_src_w; j++) {
				*fbp0++ = color;
			}
		}
	} else if (mode == 1) {
		uint32_t *fbp0 = (uint32_t *)fb_src;
		int color_idx = 0;
		for (int i = 0; i < fb_src_h; i++) {
			color_idx = 0;
			for (int j = 0; j < fb_src_w; j++) {
				*fbp0++ = colors[color_idx++];
				if (color_idx >= n_colors)
					color_idx = 0;
			}
		}
	} else if (mode == 2) {
		uint32_t *fbp0 = (uint32_t *)fb_src;
		int color_idx = 0;
		for (int i = 0; i < fb_src_h; i++) {
			color_idx = i % n_colors;
			for (int j = 0; j < fb_src_w; j++) {
				*fbp0++ = colors[color_idx++];
				if (color_idx >= n_colors)
					color_idx = 0;
			}
		}
	} else if (mode == 3) {
		uint32_t *fbp0 = (uint32_t *)fb_src;
		for (int i = 0; i < fb_src_h; i++) {
			for (int j = 0; j < fb_src_w; j++) {
				if (i == 0 || i == (fb_src_h - 1) || j == 0 || j == (fb_src_w - 1))
					*fbp0++ = colors[0];
				else 
					*fbp0++ = colors[1];
			}
		}
	} else {
		printf("un-supported mode %d\r\n", mode);
		hcge_genimg_usage(argv[0]);
		return -1;
	}

	if (dump_path) {
		fp = fopen(dump_path, "wb");
		if (fp) {
			fwrite(fb_src, fb_src_w * fb_src_h * 4, 1, fp);
			fclose(fp);
		} else {
			printf("create file %s failed\r\n", dump_path);
		}
	}

	return 0;
}

static void hcge_blitimg_usage(const char *prog)
{
	printf("Usage: %s [-rxyld]\n", prog);
	puts("  -r --rotate        rotage angle [0, 90, 180, 270]\n"
	     "  -x --destx         destination x position\n"
	     "  -y --desty         destination y position\n"
	     "  -l --log           enable HCGE log\n"
	     "  -d --dump          dump dest blit img to file\n");
}

static int hcge_blitimg_main(int argc, char *argv[])
{
	int angle = 0;
	int x = 0, y = 0;
	const char *dump_path = NULL;
	FILE *fp;

	ctx->log_en = false;

	while (1) {
		static const struct option lopts[] = {
			{ "rotage", 1, 0, 'r' },
			{ "destx", 1, 0, 'x' },
			{ "desty", 1, 0, 'y' },
			{ "log", 0, 0, 'l' },
			{ "dump", 1, 0, 'd' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "r:x:y:ld:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'r':
			angle = (int)strtoll(optarg, NULL, 0);
			break;
		case 'x':
			x = (int)strtoll(optarg, NULL, 0);
			break;
		case 'y':
			y = (int)strtoll(optarg, NULL, 0);
			break;
		case 'l':
			ctx->log_en = true;
			break;
		case 'd':
			dump_path  = optarg;
			break;
		default:
			hcge_blitimg_usage(argv[0]);
			return -1;
		}
	}

	if (angle != 0 && angle != 90 && angle != 180 && angle != 270) {
		printf("please specify angle\r\n");
		hcge_blitimg_usage(argv[0]);
		return -1;
	}

	if (angle == 90) {
		angle = HCGE_DSBLIT_ROTATE90;
	} else if (angle == 180) {
		angle = HCGE_DSBLIT_ROTATE180;
	} else if (angle == 270) {
		angle = HCGE_DSBLIT_ROTATE270;
	}

	hcge_state *state = &ctx->state;
	HCGERectangle srect = { 0, 0, fb_src_w, fb_src_h };
	HCGERectangle drect = { 0, 0, var.xres, var.yres };

	state->render_options = HCGE_DSRO_NONE;
	state->drawingflags = HCGE_DSDRAW_NOFX;
	state->blittingflags = HCGE_DSBLIT_NOFX;

	state->src_blend = HCGE_DSBF_SRCALPHA;
	state->dst_blend = HCGE_DSBF_ZERO;

	state->destination.config.size.w = var.xres;
	state->destination.config.size.h = var.yres;
	state->destination.config.format = HCGE_DSPF_ARGB;
	state->dst.phys = (unsigned long)fix.smem_start;
	state->dst.pitch = var.xres * 4;

	//state->mod_hw = HCGE_SMF_CLIP;
	state->clip.x1 = 0;
	state->clip.y1 = 0;
	state->clip.x2 = var.xres - 1;
	state->clip.y2 = var.yres - 1;

	state->source.config.size.w = fb_src_w;
	state->source.config.size.h = fb_src_h;
	state->source.config.format = HCGE_DSPF_ARGB;
	state->src.phys = (unsigned long)((uint8_t *)fix.smem_start + (fb_src - fb_base));
	state->src.pitch = 4 * fb_src_w;

#if 0
	state->dst.phys =(unsigned long)((uint8_t *)fix.smem_start + (bg_picture - fb_base));
	state->dst.pitch = ppm_info.w * 4;

	state->accel = HCGE_DFXL_STRETCHBLIT;
	srect.x = 0;
	srect.y = 0;
	srect.w = ppm_info.w;
	srect.h = 720;
	hcge_set_state(ctx, &ctx->state, state->accel);
	hcge_blit(ctx, &srect, 0, 0);


	state->dst.phys =(unsigned long)((uint8_t *)fix.smem_start + (buf - fb_base));
	state->dst.pitch = 1280*4;
#endif
#if 0
	state->accel = HCGE_DFXL_BLIT;
	hcge_set_state(ctx, &ctx->state, state->accel);
	hcge_blit(ctx, &srect, 0, 0);
	hcge_engine_sync(ctx);
	int ret;
	// 4. 通知驱动切换 buffer
	var.yoffset = var.yres;
	printf("var.yoffset: %d\n", var.yoffset);
	ret = ioctl(fbdev, FBIOPAN_DISPLAY, &var);
	if (ret < 0) {
		perror("ioctl() / FBIOPAN_DISPLAY");
	}

	// 5. 等待帧同步完成
	ret = 0;
	ioctl(fbdev, FBIO_WAITFORVSYNC, &ret);
	if (ret < 0) {
		perror("ioctl() / FBIO_WAITFORVSYNC");
	}
#endif
	/*usleep(4*1000*1000);*/

	state->accel = HCGE_DFXL_BLIT;
	state->blittingflags = angle;
	hcge_set_state(ctx, &ctx->state, state->accel);
	hcge_blit(ctx, &srect, x, y);
	hcge_engine_sync(ctx);

	/*usleep(4*1000*1000);*/

#if 0
	printf("******************test rotate 270******************\n");
	state->accel = HCGE_DFXL_BLIT;
	state->blittingflags = HCGE_DSBLIT_ROTATE270;
	hcge_set_state(ctx, &ctx->state, state->accel);
	hcge_blit(ctx, &srect, 820, 360);
	hcge_engine_sync(ctx);

	/*usleep(4*1000*1000);*/
	printf("**************** test stretch blit *****************\n");
	drect.x = 0;
	drect.y = 400;
	drect.w= 1280;
	drect.h = 720-400;
	state->accel = HCGE_DFXL_STRETCHBLIT;
	state->blittingflags = HCGE_DSBLIT_ROTATE90;;
	hcge_set_state(ctx, &ctx->state, state->accel);
	hcge_stretch_blit(ctx, &srect, &drect);
#endif

	if (!dump_path)
		return 0;

	fp = fopen(dump_path, "wb");
	if (fp) {
		int h = fb_src_h;
		int w = fb_src_w;
		if (angle == HCGE_DSBLIT_ROTATE90 || angle == HCGE_DSBLIT_ROTATE270) {
			h = fb_src_w;
			w = fb_src_h;
		}

		uint32_t *fbp = (uint32_t *)fb_base;
		for (int i = 0; i < h; i++) {
			fbp = (uint32_t *)fb_base + var.xres * (y + i) + x;
			fwrite(fbp, w * 4, 1, fp);
		}

		fclose(fp);
	} else {
		printf("create file %s failed\r\n", dump_path);
	}
	
	return 0;
}

#ifdef __HCRTOS__
#include <kernel/lib/console.h>
CONSOLE_CMD(ge, NULL, NULL, CONSOLE_CMD_MODE_SELF, "hcge test entry")
CONSOLE_CMD(openfb, "ge", hcge_openfb_main, CONSOLE_CMD_MODE_SELF, "open framebuffer")
CONSOLE_CMD(closefb, "ge", hcge_closefb_main, CONSOLE_CMD_MODE_SELF, "close framebuffer")
CONSOLE_CMD(genimg, "ge", hcge_genimg_main, CONSOLE_CMD_MODE_SELF, "generate image")
CONSOLE_CMD(fill, "ge", hcge_fill_main, CONSOLE_CMD_MODE_SELF, "fill color to framebuffer")
CONSOLE_CMD(blitimg, "ge", hcge_blitimg_main, CONSOLE_CMD_MODE_SELF, "blit image")
#endif
