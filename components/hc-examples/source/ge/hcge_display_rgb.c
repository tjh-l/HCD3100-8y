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
#include <linux/fb.h>
#include <stdint.h>
#include <stdlib.h>
#include <hcge/ge_api.h>

static int fbdev;
static struct fb_fix_screeninfo fix;
static struct fb_var_screeninfo var;
static int fb_bits_per_pixel = 32;
static int fb_color_format = HCGE_DSPF_ARGB;
static uint32_t screen_size;
static uint8_t *fb_base;
static uint8_t *screen_buffer[2];
static uint8_t *free_ptr;
static uint32_t free_size;
static uint8_t *bg_picture;
static int bg_picture_size = 1280*720*4;
int buffer_num  = 4;
static hcge_context *ctx = NULL;

static void draw_background(int src_width, int src_height, int per_pixel_bytes, int src_color_format,
                            uint8_t *src, uint8_t *buf)
{
	hcge_state *state = &ctx->state;
	HCGERectangle srect = {0, 0, src_width, src_height};
	HCGERectangle drect = {0, 0, 1280, 720};

	state->render_options = HCGE_DSRO_NONE;
	state->drawingflags = HCGE_DSDRAW_NOFX;
	state->blittingflags = HCGE_DSBLIT_NOFX;

	state->src_blend = HCGE_DSBF_SRCALPHA;
	state->dst_blend = HCGE_DSBF_ZERO;

	state->destination.config.size.w = 1280;
	state->destination.config.size.h = 720;
	state->destination.config.format = fb_color_format;
	state->dst.phys =(unsigned long)((uint8_t *)fix.smem_start + (buf - fb_base));
	state->dst.pitch = 1280*fb_bits_per_pixel/8;

	state->mod_hw = HCGE_SMF_CLIP;
	state->clip.x1 = 0;
	state->clip.y1 = 0;
	state->clip.x2 = 1280 - 1;
	state->clip.y2 = 720 - 1;

	state->source.config.size.w = src_width;
	state->source.config.size.h = src_height;
	state->source.config.format = src_color_format;
	state->src.phys =(unsigned long)((uint8_t *)fix.smem_start + (src - fb_base));
	state->src.pitch = src_width * per_pixel_bytes;

	state->accel = HCGE_DFXL_STRETCHBLIT;
	state->blittingflags = HCGE_DSBLIT_ROTATE90;;
	hcge_set_state(ctx, &ctx->state, state->accel);
	hcge_stretch_blit(ctx, &srect, &drect);
}

static int init_fb_device(int bits_per_pixel)
{
	int ret;
	if(hcge_open(&ctx) != 0) {
		printf("Init hcge error.\n");
		return -1;
	}
	fbdev = open("/dev/fb0", O_RDWR);

	ioctl(fbdev, FBIOGET_FSCREENINFO, &fix);
	ioctl(fbdev, FBIOGET_VSCREENINFO, &var);
	if (bits_per_pixel == 16) {
		var.bits_per_pixel = 16;
		var.red.length = 5;
		var.green.length = 6;
		var.blue.length = 5;
		var.transp.length = 0;
		fb_color_format = HCGE_DSPF_RGB16;
	} else if (bits_per_pixel == 32) {
		var.bits_per_pixel = 32;
		var.red.length = 8;
		var.green.length = 8;
		var.blue.length = 8;
		var.transp.length = 8;
		fb_color_format = HCGE_DSPF_ARGB;
	} else {
		printf("not support bits_per_pixel %d\n", bits_per_pixel);
		return -1;
	}

	screen_size = var.xres * var.yres * var.bits_per_pixel / 8;

	buffer_num = fix.smem_len / screen_size;

	//Make sure that the display is on.
	if (ioctl(fbdev, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
		printf("%s:%d\n", __func__, __LINE__);
	}

	var.activate = FB_ACTIVATE_NOW;
	var.yoffset = 0;
	var.xoffset = 0;
	var.transp.length = 8;
	var.yres_virtual = buffer_num * var.yres;
	printf("var.yres_virtual: %d\n", var.yres_virtual);

	//set variable information
	if(ioctl(fbdev, FBIOPUT_VSCREENINFO, &var) == -1) {
		perror("Error setting variable information");
		return -1;
	}

	fb_base = (unsigned char *)mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0);
	if (fb_base == MAP_FAILED) {
		printf("can't mmap\n");
		return -1;
	}
	memset(fb_base, 0, fix.smem_len);

	// 4. 通知驱动切换 buffer
	ret = ioctl(fbdev, FBIOPAN_DISPLAY, &var);
	if(ret < 0)
		printf("FBIOPAN_DISPLAY error. ret: %d\n", ret);

	ioctl(fbdev, FBIO_WAITFORVSYNC, &ret);
	if (ret < 0) {
		perror("ioctl() / FBIO_WAITFORVSYNC");
	}

	screen_buffer[0] = fb_base;

	if(buffer_num > 2) {
		screen_buffer[1] = fb_base + screen_size;
		free_ptr = fb_base + 2*screen_size;
	}

	bg_picture = free_ptr;
	free_ptr += 1280*720*4;
	free_size -= 1280*720*4;
	return 0;
}

static void deinit_fb_device(void)
{
	if(fbdev > 0) {
		if(fb_base) {
			munmap(fb_base, fix.smem_len);
			fb_base = NULL;
		}
		close(fbdev);
		fbdev = -1;
	}
}

static uint8_t *load_data(char *path, int *size, int *color_format)
{
	FILE *fp = fopen(path, "r");
	char *buf = bg_picture;
	printf("fp: %p\n", fp);
	if(!fp) {
		perror("Open error.\n");
		return NULL;
	}

	fseek(fp, 0L, SEEK_END);
	*size = ftell(fp);
	rewind(fp);
	if(*size <= 0 || *size > bg_picture_size) {
		printf("size error: %d\n", *size);
		fclose(fp);
		return NULL;
	}
	if(*color_format == HCGE_DSPF_RGB24) {
		buf = malloc(*size);
		if(!buf) {
			perror("Not enough memory\n");
			fclose(fp);
			return NULL;
		}
	}

	if(*size != (int)fread(buf, 1, *size, fp)) {
		perror("read data error.\n");
		fclose(fp);
		return NULL;
	}
	if(*color_format == HCGE_DSPF_RGB24) {
		for(int i = 0; i < *size/3; i++) {
			memcpy(bg_picture + i*4, buf + i*3, 3);
		}
		*color_format = HCGE_DSPF_RGB32;
	}
	return bg_picture;
}

void show_usage(void)
{
	printf("hcge_display_rgb -d <src_width> -h <src_height>  -f <src_color_format> "\
			"-b <framebuffer bit per pixel <file path>\n");
	printf("    src_color_format: rgb555, argb1555, rgb24, rgb32, argb32\n");
}

int main(int argc, char **argv)
{
	int ret;
	char *buf_next;
	int width = 0, height = 0, src_color_format = 0;
	uint8_t *src_buf;
	int src_size;
	int per_pixel_bytes = 0;
	char *path = NULL;
	int opt;

	opterr = 0;
	optind = 0;

	while((opt = getopt(argc, argv, "d:h:f:b:")) != EOF) {
		switch(opt) {
		case 'd':
			width = atoi(optarg);
			break;
		case 'h':
			height = atoi(optarg);
			break;
		case 'f':
			printf("optarg: %s\n", optarg);
			if(!strcmp(optarg, "rgb555")) {
				src_color_format = HCGE_DSPF_RGB555;
				per_pixel_bytes = 2;
			} else if(!strcmp(optarg, "rgb24")) {
				src_color_format = HCGE_DSPF_RGB24;
				per_pixel_bytes = 4;//software convert to 4 bytes
			} else if(!strcmp(optarg, "rgb32")) {
				src_color_format = HCGE_DSPF_RGB32;
				per_pixel_bytes = 4;
			} else if(!strcmp(optarg, "argb32")) {
				src_color_format = HCGE_DSPF_ARGB;
				per_pixel_bytes = 4;
			} else if(!strcmp(optarg, "argb1555")) {
				src_color_format = HCGE_DSPF_ARGB1555;
				per_pixel_bytes = 2;
			} else {
				printf("Don't support color format, %s\n", optarg);
			}
			break;
		case 'b':
			fb_bits_per_pixel = atoi(optarg);
			break;
		default:
			break;
		}
	}
	if(optind == argc) {
		printf("Not file path\n");
		return -1;
	}

	path = argv[optind++];
	printf("path: %s\n", path);
	printf("fb_bits_per_pixel: %d\n", fb_bits_per_pixel);

	if(init_fb_device(fb_bits_per_pixel) != 0) {
		perror("Init framebuffer error.\n");
		exit(-1);
	}

	printf("width: %d, height: %d, src_color_format: %d\n", width, height, src_color_format);
	src_buf = load_data(path, &src_size, &src_color_format);
	if(!src_buf) {
		return -1;
	}

	//draw background
	buf_next = (char *)screen_buffer[1];
	draw_background(width, height, per_pixel_bytes, src_color_format, src_buf, (uint8_t *)buf_next);

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

	printf("Press any key exit.\n");
	getchar();

	deinit_fb_device();
	return 0;
}
