#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <sys/epoll.h>
#include <sys/mman.h>
//#include <linux/ioctl.h>
#include <hcuapi/viddec.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/avsync.h>
#include <hcuapi/common.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <kernel/lib/console.h>
#include <pthread.h>

struct video_decoder {
	struct video_config cfg;
	int fd;
};

static int video_abort = 0;

static void *video_init(int width, int height,
	uint8_t *extra_data, int extra_data_size, 
	int rotate_enable, rotate_type_e rotate, mirror_type_e mirror,
	int transcode_enable, enum IMG_TRANSCODE_FORMAT transcode_format,
	int do_scale, int scale_w, int scale_h, viddec_scale_factor_e scale_factor, enum AVVideoRenderMode scale_mode)
{
	struct video_decoder *p = malloc(sizeof(struct video_decoder));
	memset(&p->cfg , 0 , sizeof(struct video_config));

	p->cfg.codec_id = HC_AVCODEC_ID_MJPEG;
	p->cfg.sync_mode = AVSYNC_TYPE_SYNCSTC_NODROP;
	p->cfg.decode_mode = VDEC_WORK_MODE_KSHM;

	p->cfg.pic_width = width;
	p->cfg.pic_height = height;
	p->cfg.frame_rate = 60 * 1000;

	p->cfg.pixel_aspect_x = 1;
	p->cfg.pixel_aspect_y = 1;
	p->cfg.preview = 0;

	if (extra_data && extra_data_size && extra_data_size < 512) {
		p->cfg.extradata_size = extra_data_size;
		memcpy(p->cfg.extra_data, extra_data, extra_data_size);
	}

	if (rotate_enable) {
		p->cfg.rotate_enable = 1;
		p->cfg.rotate_by_cfg = 1;
		p->cfg.rotate_type = rotate;
		p->cfg.mirror_type = mirror;
	}

	if (transcode_enable) {
		p->cfg.transcode_config.b_enable = 1;
		p->cfg.transcode_config.b_capture_one = 0;
		p->cfg.transcode_config.transcode_format = transcode_format;
		if (do_scale) {
			p->cfg.transcode_config.b_scale = 1;
			p->cfg.transcode_config.transcode_mode = scale_mode;
			if (scale_factor) {
				p->cfg.transcode_config.scale_factor = scale_factor;
			} else {
				p->cfg.transcode_config.transcode_width = scale_w;
				p->cfg.transcode_config.transcode_height = scale_h;
			}
		}
	}

	p->fd = open("/dev/viddec", O_RDWR);
	if(p->fd < 0) {
		printf("Open /dev/viddec error.\n");
		return NULL;
	}

	if(ioctl(p->fd , VIDDEC_INIT , &( p->cfg )) != 0) {
		printf("Init viddec error.\n");
		close(p->fd);
		free(p);
		return NULL;
	}

	ioctl(p->fd , VIDDEC_START , 0);

	//printf("fd: %d\n" , p->fd);
	return p;
}

static int video_write(void *phandle, uint8_t *video_frame, int frame_size)
{
	struct video_decoder *p = (struct video_decoder *)phandle;
	AvPktHd pkthd = {0};
	pkthd.pts = -1;
	pkthd.size = frame_size;
	pkthd.flag = AV_PACKET_ES_DATA;

	//printf("video_frame: %p, frame_size: %d\n", video_frame, frame_size);
	while(write(p->fd , (uint8_t *)&pkthd , sizeof(AvPktHd)) != sizeof(AvPktHd)) {
		printf("Write AvPktHd fail\n");
		return -1;
	}

	while(write(p->fd , video_frame , frame_size) != (int)frame_size) {
		float rate = 1;
		ioctl(p->fd , VIDDEC_FLUSH , &rate);
		printf("Write video_frame error fail\n");
		return -1;
	}

	return 0;
}

static int video_set_eof(void *phandle)
{
	struct video_decoder *p = (struct video_decoder *)phandle;
	AvPktHd pkthd = {0};

	if (!p || p->fd < 0)
		return -1;

	pkthd.flag = AV_PACKET_EOS;
	if(write(p->fd , (uint8_t *)&pkthd , sizeof(AvPktHd)) != sizeof(AvPktHd)) {
		printf("Write AvPktHd fail\n");
		return -1;
	}

	return 0;
}

static int video_eos_check(void *phandle)
{
	struct video_decoder *p = (struct video_decoder *)phandle;
	int eos = 0;

	if (!p || p->fd < 0)
		return -1;

	ioctl(p->fd, VIDDEC_CHECK_EOS, &eos); //end of stream check

	return eos;
}

static int video_read(void *phandle, void **buf, int *len)
{
	struct video_decoder *p = (struct video_decoder *)phandle;
	AvPktHd hdr = {0};

	while(read(p->fd, &hdr, sizeof(AvPktHd)) != sizeof(AvPktHd) && !video_abort) {
		usleep(15 * 1000);
	}

	*buf = malloc(hdr.size);
	if (!(*buf)) {
		return -1;
	}
	*len = hdr.size;
	
	while (read(p->fd, *buf, *len) != *len && !video_abort) {
		usleep(1 * 1000);
	}

	return 0;
}

static int video_get_transcode_image(void *phandle, void **y, int *y_len, void **u, int *u_len, void **v, int *v_len)
{
	struct video_decoder *p = (struct video_decoder *)phandle;

	if (p->cfg.transcode_config.transcode_format == IMG_TRANSCODE_YUV420P) {
		if (video_read(phandle, y, y_len) < 0) {
			return -1;
		}
		if (video_read(phandle, u, u_len) < 0) {
			free(*y);
			*y = NULL;
			return -1;
		}
		if (video_read(phandle, v, v_len) < 0) {
			free(*y);
			*y = NULL;
			y_len = 0;
			free(*u);
			*u = NULL;
			u_len = 0;
			return -1;
		}
	} else if (p->cfg.transcode_config.transcode_format == IMG_TRANSCODE_JPG ||
		p->cfg.transcode_config.transcode_format == IMG_TRANSCODE_YUV_Y_ONLY) {
		if (video_read(phandle, y, y_len) < 0) {
			return -1;
		}
	} else {
		printf("unsupport transcode format\n");
		return -1;
	}

	return 0;
}

static void video_destroy(void *phandle)
{
	struct video_decoder *p = (struct video_decoder *)phandle;

	if(!p)
		return;

	if(p->fd > 0) {
		close(p->fd);
	}

	free(p);
}

#ifndef AV_RB16
#define AV_RB16(x)						   \
	((((const uint8_t*)(x))[0] << 8) |		  \
	  ((const uint8_t*)(x))[1])
#endif

static int get_jpg_size(uint8_t *buf, int size, int *width, int *height)
{
	uint8_t marker;
	uint16_t len;

	if (size < 2)
		return -1;
	marker = *buf;
	buf++;size--;
	while (marker == 0xff) {
		marker = *buf;
		buf++;size--;
		switch(marker) {
			case 0xda://SOS
				return -1;
			case 0xd8:
				break;
			case 0xc0:
				if (size < 16)
					return -1;
				len = AV_RB16(buf);
				if (len < 12) {
					return -1;
				}
				if (height)
					*height = AV_RB16(buf + 3);
				if (width)
					*width = AV_RB16(buf + 5);
				return 0;
			default:
				if (size < 2) {
					return -1;
				}
				len = AV_RB16(buf);
				if (size < len) {
					return -1;
				}
				buf += len; size -= len;
				break;
		}
		if (size < 2)
			return -1;
		marker = *buf;
		buf++;size--;
	}

	return -1;
}

static int get_file_size(FILE* file)
{
	int fd, ret;
	struct stat st;

	//fseek(file, 0, SEEK_END);
	//int size = ftell(file);
	//fseek(file, 0, SEEK_SET);

	fd = fileno(file);
	ret = fstat(fd, &st);
	return ret < 0 ? AVERROR(errno) : (S_ISFIFO(st.st_mode) ? 0 : st.st_size);
}

static int transcode_test (int argc, char *argv[])
{
	int ret = 0;
	FILE *in = NULL;
	FILE *out = NULL;
	void *in_buf = NULL;
	int in_buf_size;
	int width, height;
	void *dec = NULL;
	int eos = 0;
	void *y = NULL;
	void *u = NULL;
	void *v = NULL;
	int y_len = 0, u_len = 0, v_len = 0;

	if (argc < 4) {
		printf("please enter: transcode /media/sda1/in.jpg /media/sda1/out.yuv420p 3\n");
		return -1;
	}

	in = fopen(argv[1], "rb");
	if (!in) {
		printf ("open input file failed\n");
		ret = -1;
		goto fail;
	}

	out = fopen(argv[2], "wb");
	if (!out) {
		printf ("open input file failed\n");
		ret = -1;
		goto fail;
	}

	in_buf_size = get_file_size(in);
	if (in_buf_size < 0) {
		printf ("get size of input file failed\n");
		ret = -1;
		goto fail;
	}

	in_buf = malloc(in_buf_size);
	if (!in_buf) {
		printf ("no memory\n");
		ret = -1;
		goto fail;
	}

	ret = fread(in_buf, 1, in_buf_size, in);
	if (ret != in_buf_size){
		printf ("read file err\n");
		ret = -1;
		goto fail;
	}

	ret = get_jpg_size(in_buf, in_buf_size, &width, &height);
	if (ret < 0){
		printf ("get jpg width & height err\n");
		goto fail;
	}

	dec = video_init(width, height,
					  NULL, 0,
					  0, 0, 0,
					  1, atoi(argv[3]),
					  0, 0, 0, 0, 0);
					  //1, width, height, 0, AV_VIDEO_RENDER_MODE_CENTER);
	if (!dec) {
		printf ("init viddec err\n");
		ret = -1;
		goto fail;
	}

	ret = video_write(dec, in_buf, in_buf_size);
	if (ret < 0) {
		printf ("write viddec err\n");
		goto fail;
	}

	ret = video_set_eof(dec);
	if (ret < 0) {
		printf ("write eof flag err\n");
		goto fail;
	}

	while (!eos) {
		eos = video_eos_check(dec);
		usleep(2 * 1000);
	}

	ret = video_get_transcode_image(dec, &y, &y_len, &u, &u_len, &v, &v_len);
	if (ret < 0) {
		printf("get transcode image failed\n");
		goto fail;
	}

	if (y_len && y)
		fwrite(y, 1, y_len, out);
	if (u_len && u)
		fwrite(u, 1, u_len, out);
	if (v_len && v)
		fwrite(v, 1, v_len, out);
	printf("write picture len %d\n", y_len + u_len + v_len);

fail:
	if (in) {
		fclose(in);
	}
	if (out) {
		fclose(out);
	}
	if (in_buf) {
		free(in_buf);
	}
	if (dec) {
		video_destroy(dec);
	}
	if (y) {
		free(y);
	}
	if (u) {
		free(u);
	}
	if (v) {
		free (v);
	}
	return ret;
}


/************************************test2********************************************/
static pthread_t video_read_thread_id = 0;
static FILE *in_file = NULL;
static FILE *out_file = NULL;
static enum IMG_TRANSCODE_FORMAT format = IMG_TRANSCODE_YUV420P;

static void *video_read_thread(void *arg)
{
    void *dec = NULL;
	struct AVCodecParserContext *s = av_parser_init(AV_CODEC_ID_MJPEG);
	struct AVCodecContext avctx = {0};
	int ret;
	void *in_buf = NULL;
	int in_buf_size = 4096;
	uint8_t *outbuf = NULL;
	int outbuf_size = 0;
	int width = 0, height = 0;
	void *y = NULL;
	void *u = NULL;
	void *v = NULL;
	int y_len = 0, u_len = 0, v_len = 0;

	in_buf = malloc(in_buf_size);
	if (!in_buf) {
		printf ("no memory\n");
		ret = -1;
		goto fail;
	}

	while (1) {
		int len, remain;
		uint8_t *data;
		remain = fread(in_buf, 1, in_buf_size, in_file);
		if (remain != in_buf_size){
			ret = -1;
			break;
		}
		data = in_buf;
		do {
			len = s->parser->parser_parse(s, &avctx, (const uint8_t **)&outbuf, &outbuf_size, data, remain);
			if (len < 0)
				len = 0;
			data = len ? data + len : data;
			remain -= len;
			if (outbuf && outbuf_size) {
				//printf("outbuf_size %d, ret %d, len %d, 0x%x, 0x%x, 0x%x, 0x%x\n",
				//	outbuf_size, ret, len, outbuf[0], outbuf[1], outbuf[2], outbuf[3]);
				if (width == 0) {
					ret = get_jpg_size(outbuf, outbuf_size, &width, &height);
					if (ret < 0){
						printf ("get jpg width & height err\n");
						goto fail;
					}
					printf("width %d, height %d\n", width, height);
					dec = video_init(width, height,
									NULL, 0,
									0, 0, 0,
									1, format,
									0, 0, 0, 0, 0);
									//1, width, height, 0, AV_VIDEO_RENDER_MODE_CENTER);
					if (!dec) {
						printf ("init viddec err\n");
						ret = -1;
						goto fail;
					}
				}
				ret = video_write(dec, outbuf, outbuf_size);
				outbuf = NULL;
				outbuf_size = 0;

				ret = video_get_transcode_image(dec, &y, &y_len, &u, &u_len, &v, &v_len);
				if (ret < 0) {
					printf("get transcode image failed\n");
					goto fail;
				}
				if (y) {
					fwrite(y, 1, y_len, out_file);
					free(y);
					y = NULL;
				}
				if (u_len && u) {
					fwrite(u, 1, u_len, out_file);
					free(u);
					u = NULL;
				}
				if (v_len && v) {
					fwrite(v, 1, v_len, out_file);
					free (v);
					v = NULL;
				}
			}
		} while (remain > 0 && !video_abort);
	}

fail:

	if (in_buf) {
		free(in_buf);
	}

	if (s) {
		av_parser_close(s);
	}

	if (dec)
		video_destroy(dec);
	dec = NULL;

	return NULL;
}

static int transcode_test2_stop (int argc, char *argv[])
{
	video_abort = 1;
	if (video_read_thread_id)
		pthread_join(video_read_thread_id, NULL);
	video_read_thread_id = 0;
	if (in_file)
		fclose(in_file);
	in_file = NULL;
	if (out_file)
		fclose(out_file);
	out_file = NULL;

	return 0;
}

static int transcode_test2 (int argc, char *argv[])
{
	if (argc < 4) {
		printf("please enter: rec_more /media/sda1/in.mjpg /media/sda1/out.yuv420p transcode_format\n");
		return -1;
	}

	if (video_read_thread_id) {
		printf("please stop first\n");
		return -1;
	}
		
	video_abort = 0;

	in_file = fopen((char *)argv[1], "rb");
	if (!in_file) {
		printf ("open input file failed\n");
		transcode_test2_stop(0, NULL);
		return -1;
	}

	out_file = fopen((char *)argv[2], "wb");
	if (!out_file) {
		printf ("open input file failed\n");
		transcode_test2_stop(0, NULL);
		return -1;
	}

	format = atoi(argv[3]);
	
	if (!video_read_thread_id) {
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 0x2000);
		if(pthread_create(&video_read_thread_id, &attr, video_read_thread, NULL)) {
			transcode_test2_stop(0, NULL);
			return -1;
		}
	}

	return 0;
}


CONSOLE_CMD(rec_one, NULL, transcode_test, CONSOLE_CMD_MODE_SELF, "transcode in out format")
CONSOLE_CMD(rec_more, NULL, transcode_test2, CONSOLE_CMD_MODE_SELF, "transcode in out format")
CONSOLE_CMD(stop_rec_more, NULL, transcode_test2_stop, CONSOLE_CMD_MODE_SELF, "transcode in out format")

