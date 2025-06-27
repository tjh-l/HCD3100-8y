/**
 * @file fbdev.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_drivers/display/fbdev.h"
#if USE_FBDEV

#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#if defined(__HCRTOS__)
#include <kernel/io.h>
#include <kernel/fb.h>
#include <hcuapi/fb.h>
#include <hcuapi/dis.h>
#include <hc-porting/keystone_sw_fix.h>
#include <kernel/lib/fdt_api.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netdb.h>

#include <linux/fb.h>
#include <hcuapi/fb.h>
#include <hcuapi/dis.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/common.h>
#include <hcuapi/kumsgq.h>
#include <sys/epoll.h>
#include <sys/cachectl.h>
#include <hc-porting/keystone_sw_fix.h>
#endif
#include <errno.h>
#include <pthread.h>

#include "hc_lvgl_init.h"

/*********************
 *      DEFINES
 *********************/
#ifndef FBDEV_PATH
#define FBDEV_PATH  "/dev/fb0"
#endif
#if 0
#define FBDEV_DBG printf
#else
#define FBDEV_DBG(...) do{}while(0)
#endif

enum {
	FB_OPT_NONE = 0,
	FB_ROTATE_90_270 = 1<<0,
	FB_FLIP = 1<<1,
	FB_ROTATE_FLIP = (FB_FLIP | FB_ROTATE_90_270),
};

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      STRUCTURES
 **********************/

struct bsd_fb_var_info {
	uint32_t xoffset;
	uint32_t yoffset;
	uint32_t xres;
	uint32_t yres;
	int bits_per_pixel;
};

struct bsd_fb_fix_info {
	long int line_length;
	long int smem_len;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void fbdev_buffer_init(uint32_t virt_addr, uint32_t phy_addr, uint32_t size);

/**********************
 *  STATIC VARIABLES
 **********************/
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
uint8_t *fbp0 = NULL;
uint8_t *fbp1 = NULL;
#if CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN
static void *rotate_bf_buf = NULL;
#endif
static void *draw_fb = NULL;
static uint8_t *fbp_swap_buf = NULL;
static uint8_t *kaf_buf = NULL;
static bool kaf_enable = false;
static HCGERectangle kaf_rect = {0};
static HCGERectangle kaf_rect_rcu = {0};
static bool kaf_rect_update = {0};
static pthread_mutex_t kaf_rec_mutex = PTHREAD_MUTEX_INITIALIZER;

uint8_t *virt_disp = NULL;
static lv_coord_t stretch_dx = 0;
static lv_coord_t stretch_dy = 0;

static long int screensize = 0;
static int fbfd = 0;
static int screen_size =  0;
static int pixel_bytes =  0;
static uint32_t disp_rotate = 0;
static uint32_t flip_support = 0;
static uint32_t h_flip = 0;
static uint32_t v_flip = 0;
static int fb_onoff = 0;
static uint32_t screen_dir = 0;
static HCGESurfacePixelFormat fb_pixel_fmt = HCGE_DSPF_ARGB;
static int draw_to_swap_buf = 0;
static int hor_screen_opt = FB_OPT_NONE;

static int hor_screen_opt_bg = FB_OPT_NONE;
static uint32_t disp_rotate_bg = 0;
static uint32_t h_flip_bg = 0;
static uint32_t v_flip_bg = 0;
static lv_coord_t stretch_dx_bg = 0;
static lv_coord_t stretch_dy_bg = 0;

#if CONFIG_LV_HC_KEYSTONE_EQUAL_SCALE
static hcfb_viewport_t kaf_viewport;
static hcfb_viewport_t kaf_viewport_bg;
#endif

/**********************
 *      MACROS
 **********************/
#define ASIGN_PRECT(rect, _x1, _y1, _x2, _y2) \
	do{\
		(rect)->x1 = _x1;\
		(rect)->y1 = _y1;\
		(rect)->x2 = _x2;\
		(rect)->y2 = _y2;\
	}while(0)

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
#if defined(__linux__)

#define MSG_TASK_STACK_DEPTH (0x2000)
#define MX_EVNTS (10)
#define EPL_TOUT (1000)

struct tvsys_switch {
	int dis_fd;
	struct dis_tvsys distvsys;
	enum TVSYS last_tvsys;
};

struct tvsys_switch g_switch;
static int epoll_fd = -1;

enum EPOLL_EVENT_TYPE {
	EPOLL_EVENT_TYPE_KUMSG = 0,
	EPOLL_EVENT_TYPE_HOTPLUG_CONNECT,
	EPOLL_EVENT_TYPE_HOTPLUG_MSG,
};

struct epoll_event_data {
	int fd;
	enum EPOLL_EVENT_TYPE type;
};



static struct epoll_event_data kumsg_data = { 0 };
static struct epoll_event_data hotplg_data = { 0 };
static struct epoll_event_data hotplg_msg_data = { 0 };

static void fbdev_calc_scale(void);

static int tvsys_to_tvtype(enum TVSYS tv_sys, struct dis_tvsys *tvsys)
{
	switch(tv_sys) {
	case TVSYS_480I:
		tvsys->tvtype = TV_NTSC;
		tvsys->progressive = 0;
		break;

	case TVSYS_480P:
		tvsys->tvtype = TV_NTSC;
		tvsys->progressive = 1;
		break;

	case TVSYS_576I :
		tvsys->tvtype = TV_PAL;
		tvsys->progressive = 0;
		break;

	case TVSYS_576P:
		tvsys->tvtype = TV_PAL;
		tvsys->progressive = 1;
		break;

	case TVSYS_720P_50:
		tvsys->tvtype = TV_LINE_720_30;
		tvsys->progressive = 1;
		break;

	case TVSYS_720P_60:
		tvsys->tvtype = TV_LINE_720_30;
		tvsys->progressive = 1;
		break;

	case TVSYS_1080I_25:
		tvsys->tvtype = TV_LINE_1080_25;
		tvsys->progressive = 0;
		break;

	case TVSYS_1080I_30:
		tvsys->tvtype = TV_LINE_1080_30;
		tvsys->progressive = 0;
		break;

	case TVSYS_1080P_24:
		tvsys->tvtype = TV_LINE_1080_24;
		tvsys->progressive = 1;
		break;

	case TVSYS_1080P_25:
		tvsys->tvtype = TV_LINE_1080_25;
		tvsys->progressive = 1;
		break;

	case TVSYS_1080P_30:
		tvsys->tvtype = TV_LINE_1080_30;
		tvsys->progressive = 1;
		break;

	case TVSYS_1080P_50:
		tvsys->tvtype = TV_LINE_1080_50;
		tvsys->progressive = 1;
		break;

	case TVSYS_1080P_60:
		tvsys->tvtype = TV_LINE_1080_60;
		tvsys->progressive = 1;
		break;

	case TVSYS_3840X2160P_30:
		tvsys->tvtype = TV_LINE_3840X2160_30;
		tvsys->progressive = 1;
		break;

	case TVSYS_4096X2160P_30:
		//tvsys->tvtype = TV_LINE_4096X2160_30;
		tvsys->tvtype = TV_LINE_3840X2160_30;
		tvsys->progressive = 1;
		break;

	default:
		tvsys->tvtype = TV_LINE_720_30;
		tvsys->progressive = 1;
	}

	printf("%s:%d: tvtype=%d\n", __func__, __LINE__, tvsys->tvtype);
	printf("%s:%d: progressive=%d\n", __func__, __LINE__, tvsys->progressive);

	return 0;
}

static uint32_t fbdev_get_param_from_dts(const char *path)
{
	int fd = open(path, O_RDONLY);
	int value = 0;;
	if(fd >= 0) {
		uint8_t buf[4];
		if(read(fd, buf, 4) != 4) {
			close(fd);
			return value;
		}
		close(fd);
		value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
	}
	return value;
}

static void fbdev_get_string_from_dts(const char *path, char *status, int size)
{
	int fd = open(path, O_RDONLY);
	if(fd >= 0) {
		read(fd, status, size);
		close(fd);
	}
	printf("status: %s\n", status);
}


static int __set_hdmi_out_tvsys(struct dis_tvsys *tvsys)
{
	int ret = 0;

	if( g_switch.dis_fd < 0) {
		printf("g_switch.dis_fd is invalue(%d)\n", g_switch.dis_fd);
		return -1;
	}

	ret = ioctl(g_switch.dis_fd, DIS_SET_TVSYS, tvsys);
	if(ret < 0) {
		printf("DIS_SET_TVSYS failed, ret=%d\n", ret);
		return -1;
	}

	fbdev_calc_scale();

	return 0;
}

static int set_hdmi_out_tvsys(enum TVSYS tv_sys)
{
	struct dis_tvsys distvsys = {0};

	printf("%s:%d: tv_sys= %d\n", __func__, __LINE__, tv_sys);

	tvsys_to_tvtype(tv_sys, &distvsys);
	if(g_switch.distvsys.tvtype != distvsys.tvtype ||
	   g_switch.distvsys.progressive != distvsys.progressive) {

		printf("%s:%d: set new display resolution, old(%d, %d), new(%d, %d)\n", __func__, __LINE__,
		       g_switch.distvsys.tvtype, g_switch.distvsys.progressive, distvsys.tvtype,
		       distvsys.progressive);

		g_switch.distvsys.distype     = DIS_TYPE_HD;
		g_switch.distvsys.layer       = DIS_LAYER_MAIN;
		g_switch.distvsys.tvtype      = distvsys.tvtype;
		g_switch.distvsys.progressive = distvsys.progressive;
		g_switch.last_tvsys                = tv_sys;
		__set_hdmi_out_tvsys(&g_switch.distvsys);
	}

	return 0;
}

static void do_tvsys(enum TVSYS tvsys)
{
	if(g_switch.last_tvsys != tvsys) {
		set_hdmi_out_tvsys(tvsys);
	} else {
		printf("%s:%d: last_tvsys(%u) == tv_sys(%u), not set resolution\n",
		       __func__, __LINE__, g_switch.last_tvsys, tvsys);
	}
}

static void do_kumsg(KuMsgDH *msg)
{
	switch (msg->type) {
	case HDMI_TX_NOTIFY_CONNECT:
		printf("%s(), line:%d. hdmi tx connect\n", __func__, __LINE__);
		break;
	case HDMI_TX_NOTIFY_DISCONNECT:
		printf("%s(), line:%d. hdmi tx disconnect\n", __func__, __LINE__);
		break;
	case HDMI_TX_NOTIFY_EDIDREADY: {
		struct hdmi_edidinfo *edid = (struct hdmi_edidinfo *)&msg->params;
		printf("%s(), line:%d. best_tvsys: %d\n", __func__, __LINE__, edid->best_tvsys);
		do_tvsys(edid->best_tvsys);
		break;
	}
	default:
		break;
	}
}

static void *receive_event_func(void *arg)
{
	struct epoll_event events[MX_EVNTS];
	int n = -1;
	int i;
	struct sockaddr_in client;
	socklen_t len = sizeof(client);

	while(1) {
		n = epoll_wait(epoll_fd, events, MX_EVNTS, EPL_TOUT);
		if(n == -1) {
			if(EINTR == errno) {
				continue;
			}
			usleep(100 * 1000);
			continue;
		} else if(n == 0) {
			continue;
		}

		for(i = 0; i < n; i++) {
			struct epoll_event_data *d = (struct epoll_event_data *)events[i].data.ptr;
			int fd = (int)d->fd;
			enum EPOLL_EVENT_TYPE type = d->type;

			switch (type) {
			case EPOLL_EVENT_TYPE_KUMSG: {
				unsigned char msg[MAX_KUMSG_SIZE] = {0};
				int l = 0;
				l = read(fd, msg, MAX_KUMSG_SIZE);
				if(l > 0) {
					do_kumsg((KuMsgDH *)msg);
				}
				break;
			}
			case EPOLL_EVENT_TYPE_HOTPLUG_CONNECT: {
				printf("%s(), line: %d. get hotplug connect...\n", __func__, __LINE__);
				struct epoll_event ev;
				int new_sock = accept(fd, (struct sockaddr *)&client, &len);
				if (new_sock < 0)
					break;

				hotplg_msg_data.fd = new_sock;
				hotplg_msg_data.type = EPOLL_EVENT_TYPE_HOTPLUG_MSG;
				ev.events = EPOLLIN;
				ev.data.ptr = (void *)&hotplg_msg_data;
				epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_sock, &ev);
				printf("get a new client...\n");
				break;
			}
			case EPOLL_EVENT_TYPE_HOTPLUG_MSG: {
				printf("%s(), line: %d. get hotplug msg...\n", __func__, __LINE__);
				unsigned char msg[128] = {0};
				int l = 0;
				l = read(fd, msg, sizeof(msg) - 1);
				if (l > 0) {
					printf("%s\n", msg);
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
					printf("close client\n");
				} else {
					printf("read hotplug msg fail\n");
				}
				break;
			}

			default:
				break;
			}
		}
	}

	return NULL;
}

static bool m_fb_hotplug_support = false;
void lv_fb_hotplug_support_set(bool enable)
{
	m_fb_hotplug_support = enable;
}

static void audo_tvsys_init(void)
{
	pthread_attr_t attr;
	pthread_t tid;
	int hdmi_tx_fd = -1;
	int kumsg_fd = -1;
	int hotplug_fd = -1;
	struct epoll_event ev;
	enum TVSYS tvsys;
	int ret = -1;
	struct sockaddr_un serv;
	struct kumsg_event event = { 0 };

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, MSG_TASK_STACK_DEPTH);
	if (pthread_create(&tid, &attr, receive_event_func, (void *)NULL)) {
		printf("pthread_create receive_event_func fail\n");
		goto error_ret;
	}

	g_switch.dis_fd = open("/dev/dis", O_RDWR);
	if( g_switch.dis_fd < 0) {
		printf("open /dev/dis failed, ret=%d\n", g_switch.dis_fd);
		goto error_ret;
	}

	epoll_fd = epoll_create1(0);
	hdmi_tx_fd = open("/dev/hdmi", O_RDWR);
	kumsg_fd = ioctl(hdmi_tx_fd, KUMSGQ_FD_ACCESS, O_CLOEXEC);
	if (!ioctl(hdmi_tx_fd, HDMI_TX_GET_EDID_TVSYS, &tvsys)) {
		printf("setup initial tvsys\n");
		do_tvsys(tvsys);
	}

	kumsg_data.fd = kumsg_fd;
	kumsg_data.type = EPOLL_EVENT_TYPE_KUMSG;
	ev.events = EPOLLIN;
	ev.data.ptr = (void *)&kumsg_data;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, kumsg_fd, &ev) != 0) {
		printf("EPOLL_CTL_ADD fail\n");
		goto error_ret;
	}

	hotplug_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (hotplug_fd < 0) {
		printf("socket error\n");
		goto error_ret;
	} else {
		printf("socket success\n");
	}

	unlink("/tmp/hotplug.socket");
	bzero(&serv, sizeof(serv));
	serv.sun_family = AF_LOCAL;
	strcpy(serv.sun_path, "/tmp/hotplug.socket");
	ret = bind(hotplug_fd, (struct sockaddr *)&serv, sizeof(serv));
	if (ret < 0) {
		printf("bind error\n");
		goto error_ret;
	} else {
		printf("bind success\n");
	}

	ret = listen(hotplug_fd, 1);
	if (ret < 0) {
		printf("listen error\n");
		goto error_ret;
	} else {
		printf("listen success\n");
	}

	hotplg_data.fd = hotplug_fd;
	hotplg_data.type = EPOLL_EVENT_TYPE_HOTPLUG_CONNECT;
	ev.events = EPOLLIN;
	ev.data.ptr = (void *)&hotplg_data;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, hotplug_fd, &ev) != 0) {
		printf("EPOLL_CTL_ADD hotplug fail\n");
		goto error_ret;
	} else {
		printf("EPOLL_CTL_ADD hotplug success\n");
	}

	event.evtype = HDMI_TX_NOTIFY_CONNECT;
	event.arg = 0;
	ret = ioctl(hdmi_tx_fd, KUMSGQ_NOTIFIER_SETUP, &event);
	if (ret) {
		printf("KUMSGQ_NOTIFIER_SETUP 0x%08x fail\n", (uint32_t)event.evtype);
		goto error_ret;
	}

	event.evtype = HDMI_TX_NOTIFY_DISCONNECT;
	event.arg = 0;
	ret = ioctl(hdmi_tx_fd, KUMSGQ_NOTIFIER_SETUP, &event);
	if (ret) {
		printf("KUMSGQ_NOTIFIER_SETUP 0x%08x fail\n", (uint32_t)event.evtype);
		goto error_ret;
	}

	event.evtype = HDMI_TX_NOTIFY_EDIDREADY;
	event.arg = 0;
	ret = ioctl(hdmi_tx_fd, KUMSGQ_NOTIFIER_SETUP, &event);
	if (ret) {
		printf("KUMSGQ_NOTIFIER_SETUP 0x%08x fail\n", (uint32_t)event.evtype);
		goto error_ret;
	}

	return;

error_ret:
	if (epoll_fd >= 0)
		close(epoll_fd);
	if (hdmi_tx_fd >= 0)
		close(hdmi_tx_fd);
	if (kumsg_fd >= 0)
		close(kumsg_fd);
	if (hotplug_fd >= 0)
		close(hotplug_fd);
	return;
}

#endif /* __linux__ */

static int bytes_per_pixel(int bits_per_pixel)
{
	switch(bits_per_pixel) {
	case 32:
	case 24:
		return 4;
	case 16:
	case 15:
		return 2;
	default:
		return 1;
	}
}

static int get_dual_de_enable(void)
{
	int dual_de_enable = 0;
#ifdef __linux__
	dual_de_enable = fbdev_get_param_from_dts("/proc/device-tree/hcrtos/de-engine");
#else
	int np;

	np = fdt_node_probe_by_path("/hcrtos/de-engine");
	if (np < 0)
		return 0;
	fdt_get_property_u_32_index(np, "dual-de-enable", 0, &dual_de_enable);

#endif
	return dual_de_enable;
}

static uint32_t get_fb0_reg(void)
{
	 uint32_t reg = 0;
#ifdef __linux__
	reg = fbdev_get_param_from_dts("/proc/device-tree/hcrtos/fb0/reg");
#else
	int np;

	np = fdt_node_probe_by_path("/hcrtos/fb0/reg");
	if (np < 0)
		return 0;
	fdt_get_property_u_32_index(np, "reg", 0, &reg);

#endif
	return reg;
}

static int get_dis_type(void)
{
	int type = DIS_TYPE_HD;
	uint32_t reg = 0x0;
	if (get_dual_de_enable() != 0) {
		reg = get_fb0_reg();
		if (reg == 0xb883a000)
			type = DIS_TYPE_UHD;
	}
	return type;
}

static void fbdev_calc_scale(void)
{
	int disfd = -1;
	struct dis_screen_info screen = { 0 };
	hcfb_scale_t scale_param = { 1280, 720, 1920, 1080 };// to do later...
	int scale_enable_val = 0;
#ifdef __linux__
	int fd = open("/proc/device-tree/soc/fb0@18808000/scale", O_RDONLY);
	if(fd < 0) {
		return;
	}
	uint8_t buf[8];
	uint8_t *tmp = buf;
	if(read(fd, buf, 8) != 8) {
		close(fd);
		return;
	}
	close(fd);
	scale_param.h_div = (tmp[0] & 0xff) << 24 | (tmp[1] & 0xff) << 16 | (tmp[2] & 0xff) << 8 | (tmp[3] & 0xff);
	tmp = buf + sizeof(uint32_t);
	scale_param.v_div = (tmp[0] & 0xff) << 24 | (tmp[1] & 0xff) << 16 | (tmp[2] & 0xff) << 8 | (tmp[3] & 0xff);
#else
	u32 value = 0;
	int length = 0;
	int np = -1;
	np = fdt_node_probe_by_path("/hcrtos/fb0");
	if (np >= 0) {
		scale_enable_val = fdt_property_read_bool(np, "scale-enable");
		if(scale_enable_val) {
			fdt_get_property_data_by_name(np,"scale",&length);
			if(length == 16) {
				fdt_get_property_u_32_index(np, "scale", 0, &value);
				scale_param.h_div = (uint16_t)value;
				fdt_get_property_u_32_index(np, "scale", 1, &value);
				scale_param.v_div = (uint16_t)value;
				fdt_get_property_u_32_index(np, "scale", 2, &value);
				scale_param.h_mul = (uint16_t)value;
				fdt_get_property_u_32_index(np, "scale", 3, &value);
				scale_param.v_mul = (uint16_t)value;
			} else {
				scale_enable_val = 0;
			}
		}
	}
#endif

	disfd = open("/dev/dis", O_RDWR);
	if (disfd < 0) {
		return;
	}

	screen.distype = get_dis_type();
	printf("screen.distype: %d\n", screen.distype);
	if(ioctl(disfd, DIS_GET_SCREEN_INFO, &screen)) {
		close(disfd);
		return;
	}

	if(scale_enable_val == 0) {
		scale_param.h_div = vinfo.xres;
		scale_param.v_div = vinfo.yres;
		scale_param.h_mul = screen.area.w;
		scale_param.v_mul = screen.area.h;
	}
	printf("scale_param.h_div: %d, scale_param.v_div: %d, scale_param.h_mul: %d, scale_param.v_mul:%d scale_enable_val:%d\n",
	       scale_param.h_div, scale_param.v_div, scale_param.h_mul, scale_param.v_mul,scale_enable_val);

	close(disfd);
	if (ioctl(fbfd, HCFBIOSET_SCALE, &scale_param) != 0) {
		perror("ioctl(set scale param)");
	}
}

void fbdev_init(void)
{
	uint32_t keystone_conf = 0;
#ifdef __linux__
	//HDMI TV sys done in hotplug_mgr.c
	if (m_fb_hotplug_support)
		audo_tvsys_init();

#define ROTATE_CONFIG_PATH "/proc/device-tree/hcrtos/rotate"
	char status[16] = {0};
	fbdev_get_string_from_dts(ROTATE_CONFIG_PATH "/status", status, sizeof(status));
	if(!strcmp(status, "okay")) {
		disp_rotate = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/rotate");
		flip_support = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/flip_support");
		h_flip = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/h_flip");
		v_flip = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/v_flip");
		screen_dir = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/screen_dir");
		stretch_dx = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/stretch_dx");
		stretch_dy = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/stretch_dy");
		keystone_conf = fbdev_get_param_from_dts(ROTATE_CONFIG_PATH "/keystone_conf");
	}
#else
	int np = -1;
	u32 val = 0;
	np = fdt_node_probe_by_path("/hcrtos/rotate");
	if (np >= 0) {
		fdt_get_property_u_32_index(np, "rotate", 0, (u32 *)&disp_rotate);
		fdt_get_property_u_32_index(np, "flip_support", 0, (u32 *)&flip_support);
		fdt_get_property_u_32_index(np, "h_flip", 0, (u32 *)&h_flip);
		fdt_get_property_u_32_index(np, "v_flip", 0, (u32 *)&v_flip);
		fdt_get_property_u_32_index(np, "screen_dir", 0, (u32 *)&screen_dir);
		val = 0;
		fdt_get_property_u_32_index(np, "stretch_dx", 0, (u32 *)&val);
		stretch_dx = (lv_coord_t)val;
		val = 0;
		fdt_get_property_u_32_index(np, "stretch_dy", 0, (u32 *)&val);
		stretch_dy = (lv_coord_t)val;

		fdt_get_property_u_32_index(np, "keystone_conf", 0, (u32 *)&keystone_conf);

	}
#endif
	FBDEV_DBG("%s:disp_rotate: %d, h_flip: %d, v_flip: %d\n", __func__, disp_rotate, h_flip, v_flip);

	// Open the file for reading and writing
	fbfd = open(CONFIG_LV_HC_FBDEV_PATH, O_RDWR);
	if(fbfd == -1) {
		perror("Error: cannot open framebuffer device");
		return;
	}
	FBDEV_DBG("The framebuffer device was opened successfully.\n");

	// Get variable screen information
	if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		return;
	}

	vinfo.xoffset = 0;
	vinfo.yoffset = 0;
#if CONFIG_LV_HC_FB_COLOR_DEPTH == 32
	vinfo.bits_per_pixel = 32;
	vinfo.red.length = 8;
	vinfo.green.length = 8;
	vinfo.blue.length = 8;
	vinfo.transp.length = 8;
	fb_pixel_fmt = HCGE_DSPF_ARGB;
#elif CONFIG_LV_HC_FB_COLOR_DEPTH_16_ARGB1555
	vinfo.bits_per_pixel = 16;
	vinfo.red.length = 5;
	vinfo.green.length = 5;
	vinfo.blue.length = 5;
	vinfo.transp.length = 1;
	fb_pixel_fmt = HCGE_DSPF_ARGB1555;
#else
	//ARGB4444
	vinfo.bits_per_pixel = 16;
	vinfo.red.length = 4;
	vinfo.green.length = 4;
	vinfo.blue.length = 4;
	vinfo.transp.length = 4;
	fb_pixel_fmt = HCGE_DSPF_ARGB4444;
#endif

#if LV_USE_DUAL_FRAMEBUFFER
	vinfo.yres_virtual = 2 * vinfo.yres;
#else
	vinfo.yres_virtual = vinfo.yres;
#endif
	printf("bits_per_pixel: %d,red.length: %d, green.length: %d, blue.length: %d\n",
	       (int)vinfo.bits_per_pixel, (int)vinfo.red.length, (int)vinfo.green.length,
	       (int)vinfo.blue.length);

#ifdef __linux__
	vinfo.activate |= FB_ACTIVATE_FORCE;
#endif

	// Get variable screen information
	if(ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		return;
	}

	// Get fixed screen information
	if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		perror("Error reading fixed information");
		return;
	}

	printf(" variable screen: %dx%d, %dbpp\n", (int)vinfo.xres, (int)vinfo.yres, (int)vinfo.bits_per_pixel);

	// Gget tvsys, DE info & build scale_param here...
	fbdev_calc_scale();

	// Figure out the size of the screen in bytes
	screensize =  finfo.smem_len; //finfo.line_length * vinfo.yres;

	ioctl(fbfd, HCFBIOSET_MMAP_CACHE, HCFB_MMAP_CACHE);

	// Map the device to memory
	fbp0 = (uint8_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if((intptr_t)fbp0 == -1) {
		perror("Error: failed to map framebuffer device to memory");
		return;
	}
	memset(fbp0, 0, screensize);
	cacheflush(fbp0, screensize, DCACHE);

	pixel_bytes = bytes_per_pixel(vinfo.bits_per_pixel);
	screen_size = vinfo.xres * vinfo.yres * pixel_bytes;
	draw_fb = fbp0;
	printf("pixel_bytes: %d, screen_size: %08x\n", pixel_bytes, screen_size);
	printf("The framebuffer device was mapped to memory successfully.\n");

	int buf_count = 1;
#if LV_USE_DUAL_FRAMEBUFFER
	fbp1 = fbp0 + screen_size;
	buf_count++;
#endif
	if(screen_dir) {
		virt_disp = fbp0 + screen_size * buf_count;
		buf_count++;
	}

#if CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN
	if(!screen_dir) {
		//only screen_dir = 0, draw lvgl buffer to fb_showdoer
		if(disp_rotate) {
			rotate_bf_buf = fbp0 + screen_size * buf_count;
			buf_count++;
			printf("rotate_bf_buf: %p\n", rotate_bf_buf);
		}
	}
#endif

	if (flip_support) {
		fbp_swap_buf = fbp0 + screen_size * buf_count;
		if(screen_size * (buf_count + 1) > finfo.smem_len) {
			printf("error: framebuffer is not enough memory\n");
			assert(false);
			return;
		}
		buf_count++;
	}

	//keystone correction buffer
	if (keystone_conf) {
		kaf_buf = fbp0 + screen_size * buf_count;
		if(screen_size * (buf_count + 1) > finfo.smem_len) {
			printf("error: framebuffer is not enough memory\n");
			assert(false);
			return;
		}
		buf_count++;
		printf("kaf_buf: %p\n", kaf_buf);
	}

#ifndef CONFIG_LV_HC_MEM_FROM_MALLOC
	fbdev_buffer_init(((uint32_t)(fbp0 + screen_size * buf_count) + HC_MEM_ALIGN_MASK)&(~HC_MEM_ALIGN_MASK),
	                  ((uint32_t)(finfo.smem_start + screen_size * buf_count) + HC_MEM_ALIGN_MASK)&(~HC_MEM_ALIGN_MASK),
	                  finfo.smem_len - screen_size * buf_count - HC_MEM_ALIGN_MASK);
#endif

#if CONFIG_LV_HC_KEYSTONE_EQUAL_SCALE
    kaf_viewport.x = 0;
    kaf_viewport.y = 0;
    kaf_viewport.width = vinfo.xres;
    kaf_viewport.height = vinfo.yres;
    kaf_viewport_bg = kaf_viewport;
#endif
}

void fbdev_init_ext(struct lv_hc_conf *conf)
{
	uint32_t keystone_conf = 0;
#ifdef __linux__
#define ROTATE_CONFIG_PATH "/proc/device-tree/hcrtos/rotate"
	if (!conf->dts_cfg_path[0])
		strcpy(conf->dts_cfg_path, ROTATE_CONFIG_PATH);
	if (conf->hotplug_support)
		m_fb_hotplug_support = true;

	//HDMI TV sys done in hotplug_mgr.c
	if (m_fb_hotplug_support)
		audo_tvsys_init();

	char status[16] = {0};
	char path[128];
	sprintf(path, "%s/%s\n", conf->dts_cfg_path, "status");
	fbdev_get_string_from_dts(path, status, sizeof(status));
	if(!strcmp(status, "okay")) {
		sprintf(path, "%s/%s\n", conf->dts_cfg_path, "rotate");
		disp_rotate = fbdev_get_param_from_dts(path);
		sprintf(path, "%s/%s\n", conf->dts_cfg_path, "flip_support");
		flip_support = fbdev_get_param_from_dts(path);
		sprintf(path, "%s/%s\n", conf->dts_cfg_path, "h_flip");
		h_flip = fbdev_get_param_from_dts(path);
		sprintf(path, "%s/%s\n", conf->dts_cfg_path, "v_flip");
		v_flip = fbdev_get_param_from_dts(path);
		sprintf(path, "%s/%s\n", conf->dts_cfg_path, "screen_dir");
		screen_dir = fbdev_get_param_from_dts(path);
		sprintf(path, "%s/%s\n", conf->dts_cfg_path, "stretch_dx");
		stretch_dx = fbdev_get_param_from_dts(path);
		sprintf(path, "%s/%s\n", conf->dts_cfg_path, "stretch_dy");
		stretch_dy = fbdev_get_param_from_dts(path);
		sprintf(path, "%s/%s\n", conf->dts_cfg_path, "keystone_conf");
		keystone_conf = fbdev_get_param_from_dts(path);

	}
#else
#define ROTATE_CONFIG_PATH "/hcrtos/rotate"
	int np = -1;
	u32 val = 0;
	if (!conf->dts_cfg_path[0])
		strcpy(conf->dts_cfg_path, ROTATE_CONFIG_PATH);

	np = fdt_node_probe_by_path(conf->dts_cfg_path);
	if (np >= 0) {
		fdt_get_property_u_32_index(np, "rotate", 0, (u32 *)&disp_rotate);
		fdt_get_property_u_32_index(np, "flip_support", 0, (u32 *)&flip_support);
		fdt_get_property_u_32_index(np, "h_flip", 0, (u32 *)&h_flip);
		fdt_get_property_u_32_index(np, "v_flip", 0, (u32 *)&v_flip);
		fdt_get_property_u_32_index(np, "screen_dir", 0, (u32 *)&screen_dir);
		val = 0;
		fdt_get_property_u_32_index(np, "stretch_dx", 0, (u32 *)&val);
		stretch_dx = (lv_coord_t)val;
		val = 0;
		fdt_get_property_u_32_index(np, "stretch_dy", 0, (u32 *)&val);
		stretch_dy = (lv_coord_t)val;
		fdt_get_property_u_32_index(np, "keystone_conf", 0, (u32 *)&keystone_conf);
	}
#endif
	FBDEV_DBG("%s:disp_rotate: %d, h_flip: %d, v_flip: %d\n", __func__, disp_rotate, h_flip, v_flip);

	// Open the file for reading and writing
	if (conf->fbdev_path[0])
		fbfd = open(conf->fbdev_path, O_RDWR);
	else
		fbfd = open(CONFIG_LV_HC_FBDEV_PATH, O_RDWR);
	if(fbfd == -1) {
		perror("Error: cannot open framebuffer device");
		return;
	}
	FBDEV_DBG("The framebuffer device was opened successfully.\n");

	// Get variable screen information
	if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		return;
	}

	vinfo.xoffset = 0;
	vinfo.yoffset = 0;
	if (conf->color_depth > 0) {
		switch (conf->color_depth) {
			case LV_HC_FB_COLOR_DEPTH_32_ARGB:
				vinfo.bits_per_pixel = 32;
				vinfo.red.length = 8;
				vinfo.green.length = 8;
				vinfo.blue.length = 8;
				vinfo.transp.length = 8;
				fb_pixel_fmt = HCGE_DSPF_ARGB;
				break;
			case LV_HC_FB_COLOR_DEPTH_16_ARGB1555:
				vinfo.bits_per_pixel = 16;
				vinfo.red.length = 5;
				vinfo.green.length = 5;
				vinfo.blue.length = 5;
				vinfo.transp.length = 1;
				fb_pixel_fmt = HCGE_DSPF_ARGB1555;
				break;
			case LV_HC_FB_COLOR_DEPTH_16_ARGB4444:
			default:
				//ARGB4444
				vinfo.bits_per_pixel = 16;
				vinfo.red.length = 4;
				vinfo.green.length = 4;
				vinfo.blue.length = 4;
				vinfo.transp.length = 4;
				fb_pixel_fmt = HCGE_DSPF_ARGB4444;
				break;

		}
	} else {
#if CONFIG_LV_HC_FB_COLOR_DEPTH == 32
		vinfo.bits_per_pixel = 32;
		vinfo.red.length = 8;
		vinfo.green.length = 8;
		vinfo.blue.length = 8;
		vinfo.transp.length = 8;
		fb_pixel_fmt = HCGE_DSPF_ARGB;
#elif CONFIG_LV_HC_FB_COLOR_DEPTH_16_ARGB1555
		vinfo.bits_per_pixel = 16;
		vinfo.red.length = 5;
		vinfo.green.length = 5;
		vinfo.blue.length = 5;
		vinfo.transp.length = 1;
		fb_pixel_fmt = HCGE_DSPF_ARGB1555;
#else
		//ARGB4444
		vinfo.bits_per_pixel = 16;
		vinfo.red.length = 4;
		vinfo.green.length = 4;
		vinfo.blue.length = 4;
		vinfo.transp.length = 4;
		fb_pixel_fmt = HCGE_DSPF_ARGB4444;
#endif
	}

	if (conf->fb_buf_num == 0) {
#if LV_USE_DUAL_FRAMEBUFFER
		vinfo.yres_virtual = 2 * vinfo.yres;
#else
		vinfo.yres_virtual = vinfo.yres;
#endif
	} else {
		if (conf->fb_buf_num == LV_HC_DUAL_BUFFER)
			vinfo.yres_virtual = 2 * vinfo.yres;
		else
			vinfo.yres_virtual = vinfo.yres;

	}
	printf("bits_per_pixel: %d,red.length: %d, green.length: %d, blue.length: %d\n",
			(int)vinfo.bits_per_pixel, (int)vinfo.red.length, (int)vinfo.green.length,
			(int)vinfo.blue.length);

#ifdef __linux__
	vinfo.activate |= FB_ACTIVATE_FORCE;
#endif

	// Get variable screen information
	if(ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		return;
	}

	// Get fixed screen information
	if(ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
		perror("Error reading fixed information");
		return;
	}

	printf(" variable screen: %dx%d, %dbpp\n", (int)vinfo.xres, (int)vinfo.yres, (int)vinfo.bits_per_pixel);

	// Gget tvsys, DE info & build scale_param here...
	fbdev_calc_scale();

	// Figure out the size of the screen in bytes
	screensize =  finfo.smem_len; //finfo.line_length * vinfo.yres;

	if (conf->no_cached)
		ioctl(fbfd, HCFBIOSET_MMAP_CACHE, HCFB_MMAP_NO_CACHE);
	else
		ioctl(fbfd, HCFBIOSET_MMAP_CACHE, HCFB_MMAP_CACHE);

	// Map the device to memory
	fbp0 = (uint8_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	if((intptr_t)fbp0 == -1) {
		perror("Error: failed to map framebuffer device to memory");
		return;
	}
	memset(fbp0, 0, screensize);
	cacheflush(fbp0, screensize, DCACHE);

	pixel_bytes = bytes_per_pixel(vinfo.bits_per_pixel);
	screen_size = vinfo.xres * vinfo.yres * pixel_bytes;
	draw_fb = fbp0;
	printf("pixel_bytes: %d, screen_size: %08x\n", pixel_bytes, screen_size);
	printf("The framebuffer device was mapped to memory successfully.\n");

	int buf_count = 1;
	if (conf->fb_buf_num == 0) {
#if LV_USE_DUAL_FRAMEBUFFER
		fbp1 = fbp0 + screen_size;
		buf_count++;
#endif
	} else {
		if (conf->fb_buf_num == LV_HC_DUAL_BUFFER) {
			fbp1 = fbp0 + screen_size;
			buf_count++;
		}
	}

	if(screen_dir && (conf->gpu == LV_HC_GPU_GE)) {
		virt_disp = fbp0 + screen_size * buf_count;
		buf_count++;
	}

#if CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN
	if(!screen_dir && (conf->gpu == LV_HC_GPU_GE)) {
		//only screen_dir = 0, draw lvgl buffer to fb_showdoer
		if(disp_rotate) {
			rotate_bf_buf = fbp0 + screen_size * buf_count;
			buf_count++;
			printf("rotate_bf_buf: %p\n", rotate_bf_buf);
		}
	}
#endif

	if (flip_support && (conf->gpu == LV_HC_GPU_GE)) {
		fbp_swap_buf = fbp0 + screen_size * buf_count;
		if(screen_size * (buf_count + 1) > finfo.smem_len) {
			printf("error: framebuffer is not enough memory\n");
			assert(false);
			return;
		}
		buf_count++;
	}

	if (!conf->buf_from) {
#ifndef CONFIG_LV_HC_MEM_FROM_MALLOC
		fbdev_buffer_init(((uint32_t)(fbp0 + screen_size * buf_count) + HC_MEM_ALIGN_MASK)&(~HC_MEM_ALIGN_MASK),
				((uint32_t)(finfo.smem_start + screen_size * buf_count) + HC_MEM_ALIGN_MASK)&(~HC_MEM_ALIGN_MASK),
				finfo.smem_len - screen_size * buf_count - HC_MEM_ALIGN_MASK);
#endif
	} else if (conf->buf_from == LV_HC_HEAP_BUFFER_FROM_FB) {
		fbdev_buffer_init(((uint32_t)(fbp0 + screen_size * buf_count) + HC_MEM_ALIGN_MASK)&(~HC_MEM_ALIGN_MASK),
				((uint32_t)(finfo.smem_start + screen_size * buf_count) + HC_MEM_ALIGN_MASK)&(~HC_MEM_ALIGN_MASK),
				finfo.smem_len - screen_size * buf_count - HC_MEM_ALIGN_MASK);
	}
}


void fbdev_get_fb_buf(uint8_t **buf1, uint8_t **buf2)
{
	*buf1 = (uint8_t *)fbp0;
#if LV_USE_DUAL_FRAMEBUFFER == 1
	*buf2 = (uint8_t *)fbp0 + screen_size;
#else
	*buf2 = NULL;
#endif
}

void fbdev_exit(void)
{
	close(fbfd);
}

/**
 * Flush a buffer to the marked area
 * @param drv pointer to driver where this function belongs
 * @param area an area where to copy `color_p`
 * @param color_p an array of pixels to copy to the `area` part of the screen
 */
void fbdev_flush_sw(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
	uint8_t *fbp = fbp0;
#if 0
	if(fbp == NULL ||
	   area->x2 < 0 ||
	   area->y2 < 0 ||
	   area->x1 > (int32_t)vinfo.xres - 1 ||
	   area->y1 > (int32_t)vinfo.yres - 1) {
		lv_disp_flush_ready(drv);
		return;
	}
#endif

	/*Truncate the area to the screen*/
	int32_t act_x1 = area->x1 < 0 ? 0 : area->x1;
	int32_t act_y1 = area->y1 < 0 ? 0 : area->y1;
	int32_t act_x2 = area->x2 > (int32_t)vinfo.xres - 1 ? (int32_t)vinfo.xres - 1 : area->x2;
	int32_t act_y2 = area->y2 > (int32_t)vinfo.yres - 1 ? (int32_t)vinfo.yres - 1 : area->y2;

	lv_coord_t w = (act_x2 - act_x1 + 1);
	long int location = 0;
	long int byte_location = 0;
	unsigned char bit_location = 0;

	/*32 or 24 bit per pixel*/
	if(vinfo.bits_per_pixel == 32 || vinfo.bits_per_pixel == 24) {
		uint32_t * fbp32 = (uint32_t *)fbp;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 4;
			memcpy(&fbp32[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1) * 4);
			color_p += w;
		}
	}
	/*16 bit per pixel*/
	else if(vinfo.bits_per_pixel == 16) {
		uint16_t * fbp16 = (uint16_t *)fbp;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length / 2;
			memcpy(&fbp16[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1) * 2);
			color_p += w;
		}
	}
	/*8 bit per pixel*/
	else if(vinfo.bits_per_pixel == 8) {
		uint8_t * fbp8 = (uint8_t *)fbp;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			location = (act_x1 + vinfo.xoffset) + (y + vinfo.yoffset) * finfo.line_length;
			memcpy(&fbp8[location], (uint32_t *)color_p, (act_x2 - act_x1 + 1));
			color_p += w;
		}
	}
	/*1 bit per pixel*/
	else if(vinfo.bits_per_pixel == 1) {
		uint8_t * fbp8 = (uint8_t *)fbp;
		int32_t x;
		int32_t y;
		for(y = act_y1; y <= act_y2; y++) {
			for(x = act_x1; x <= act_x2; x++) {
				location = (x + vinfo.xoffset) + (y + vinfo.yoffset) * vinfo.xres;
				byte_location = location / 8; /* find the byte we need to change */
				bit_location = location % 8; /* inside the byte found, find the bit we need to change */
				fbp8[byte_location] &= ~(((uint8_t)(1)) << bit_location);
				fbp8[byte_location] |= ((uint8_t)(color_p->full)) << bit_location;
				color_p++;
			}

			color_p += area->x2 - act_x2;
		}
	} else {
		/*Not supported bit per pixel*/
	}

	if(fb_onoff==0) {
		if (ioctl(fbfd, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
			perror("ioctl(FBIOBLANK)");
			return;
		}
		fb_onoff = 1;
	}

	lv_disp_flush_ready(drv);
}

#if LV_USE_GPU_HICHIP

#if LV_HC_KEYSTONE_AA_SW_FIX || CONFIG_LV_HC_KEYSTONE_EQUAL_SCALE
static void swblit_ks_fix(struct kaf_ctx_t *ctx, int fix_flag,
                          int dst_width, int dst_height,
                          int dst_stride_px, uint32_t *dst_buf)
{
	int span;
	const uint32_t *p;
	uint32_t *dst;
	if (fix_flag & KAF_EDGE_LEFT) {
		p = kaf_get_left_edge(ctx, &span, NULL);
		dst = dst_buf;
		for (int y = 0; y < dst_height; ++y) {
			for (int x = 0; x < span; ++x) {
				// dst[x] = 0xff00ff00;
				dst[x] = p[x];
			}
			dst += dst_stride_px;
			p += span;
		}
	}
	if (fix_flag & KAF_EDGE_RIGHT) {
		p = kaf_get_right_edge(ctx, &span, NULL);
		dst = dst_buf + dst_width - span;
		for (int y = 0; y < dst_height; ++y) {
			for (int x = 0; x < span; ++x) {
				// dst[x] = 0xffff0000;
				dst[x] = p[x];
			}
			dst += dst_stride_px;
			p += span;
		}
	}
	cacheflush(dst_buf, dst_stride_px * dst_height * sizeof(*dst_buf), DCACHE);
}

static void hwblit(int format, int width, int height,
                   const void *src, int src_pitch_u8,
                   void *dst, int dst_pitch_u8)
{
	struct lv_blit blit = { 0 };
	lv_area_t src_area;

	ASIGN_PRECT(&src_area, 0, 0, width - 1, height - 1);

	blit.dst_buf = dst;
	blit.dst_area = &src_area;
	blit.dst_stride = dst_pitch_u8;
	blit.dst_fmt = blit.src_fmt = format;
	blit.src_buf = src;
	blit.src_area = &src_area;
	blit.dst_stride = src_pitch_u8;
	blit.crop = &src_area;
	lv_ge_blit(&blit, false);
}

static void hwblit_ks_fix(struct kaf_ctx_t *ctx, int fix_flag,
                          int dst_width, int dst_height,
                          int dst_stride, uint32_t *dst_buf)
{
	int span, height;
	do { // just to create a scope to make variables local
		const uint32_t *left_edge = kaf_get_left_edge(ctx, &span, &height);
		cacheflush(left_edge, span*height, DCACHE);
		hwblit(HCGE_DSPF_ARGB, span, dst_height,
		       left_edge, span,
		       dst_buf, dst_stride);
	} while (0);
	do { // just to create a scope to make variables local
		const uint32_t *right_edge = kaf_get_right_edge(ctx, &span, &height);
		cacheflush(right_edge, span*height, DCACHE);
		hwblit(HCGE_DSPF_ARGB, span, dst_height,
		       right_edge, span,
		       dst_buf + dst_width - span, dst_stride);
	} while (0);
}

static void (*blit_ks_fix)(struct kaf_ctx_t*, int, int, int, int, uint32_t*) = swblit_ks_fix;

#if LV_HC_MODIFY_EDGE_FOR_DEBUG
void modify_for_debug(uint32_t *color_p, const lv_area_t *area)
{
	const int left_span = 10;
	const int stride = area->x2 - area->x1 + 1;
	if (area->x1 < left_span) {
		uint32_t *p = color_p;
		for (int y = area->y1; y <= area->y2; ++y) {
			uint32_t *t = p;
			uint32_t c = ((y * 255 + (vinfo.yres >> 1)) / (vinfo.yres - 1)) & 0xff;
			uint32_t ic = 255 - c;
			for (int x = area->x1; x < left_span && x <= area->x2; ++x) {
				*t++ = 0xff000000 | (c << 8) | (ic << 16) | ic;
			}
			p += stride;
		}
	}
	const int right_span = 10;
	const int right_start = vinfo.xres - right_span;
	if (area->x2 >= right_start) {
		uint32_t *p = color_p;
		for (int y = area->y1; y <= area->y2; ++y) {
			uint32_t *t = p + (right_start - area->x1);
			uint32_t c = ((y * 255 + (vinfo.yres >> 1)) / (vinfo.yres - 1)) & 0xff;
			uint32_t ic = 255 - c;
			for (int x = right_start; x < vinfo.xres && x <= area->x2; ++x) {
				*t++ = 0xff000000 | (c << 16) | (ic << 8) | ic;
			}
			p += stride;
		}
	}
	cacheflush(color_p, (area->y2 - area->y1) * (area->x2 - area->x1) * sizeof(uint32_t), DCACHE);
}
#endif // LV_HC_MODIFY_EDGE_FOR_DEBUG

static void set_rect_per_chopping(const lv_area_t *area, const lv_area_t *chopped,
                                  lv_area_t *srect)
{
	srect->x1 = chopped->x1 - area->x1;
	srect->y1 = chopped->y1 - area->y1;
	srect->x2 = srect->x1 + chopped->x2 - chopped->x1;
	srect->y2 = srect->y1 + chopped->y2 - chopped->y1;
}
#endif // LV_HC_KEYSTONE_AA_SW_FIX

static void update_canvas(lv_color_t *src_buf,
                          lv_area_t *src_area,
                          lv_coord_t src_stride,
                          HCGESurfacePixelFormat src_fmt,
                          lv_color_t *dst_buf,
                          lv_coord_t dst_stride,
                          HCGESurfacePixelFormat dst_fmt,
                          lv_coord_t xres,
                          lv_coord_t yres,
                          bool backup
                         )
{
	const int32_t act_x1 = src_area->x1;
	const int32_t act_y1 = src_area->y1;
	lv_coord_t sw = lv_area_get_width(src_area);
	lv_coord_t sh = lv_area_get_height(src_area);

	struct lv_blit blit = { 0 };
	lv_area_t crop, srect, drect;
	lv_point_t pivot = { 0 };

	blit.dst_buf = dst_buf;
	blit.dst_stride = dst_stride;
	blit.dst_fmt = dst_fmt;
	blit.src_buf = src_buf;
	blit.src_stride = src_stride;
	blit.src_fmt = src_fmt;
	blit.opa = LV_OPA_COVER;

	int dx;
	int dy;
	if((disp_rotate == 270)) {
		dx = xres - act_y1 - sh;
		if((dx + sh) > xres)
			sh = xres - dx;
		dy = act_x1;
	} else if (disp_rotate == 90)  {
		dy = yres - act_x1 - sw;
		if((dy + sw) > yres)
			sw = yres - dy;
		dx = act_y1;
	} else if(disp_rotate == 180) {
		dx = xres  - act_x1 - sw;
		dy = yres  - act_y1 - sh;
	} else {
		dx = act_x1;
		dy = act_y1;
	}

	ASIGN_PRECT(&srect, 0, 0, sw - 1, sh - 1);
	ASIGN_PRECT(&crop, 0, 0, xres - 1, yres - 1);
	if(disp_rotate == 270 || disp_rotate == 90) {
		ASIGN_PRECT(&drect, dx, dy, dx + sh - 1, dy + sw - 1);
	} else {
		ASIGN_PRECT(&drect, dx, dy, dx + sw - 1, dy + sh - 1);
	}

	if(backup) {
		blit.src_area = &drect;
		blit.dst_area = &drect;
		blit.angle = 0;
	} else {
		blit.src_area = &srect;
		blit.dst_area = &drect;
		blit.angle = disp_rotate;
		pivot.x = sw/2;
		pivot.y = sh/2;
		blit.pivot = &pivot;
	}
	blit.crop = &crop;

#if LV_HC_KEYSTONE_AA_SW_FIX
#if LV_HC_MODIFY_EDGE_FOR_DEBUG
	modify_for_debug(src_buf, src_area);
#endif
	int kaf_enabled = 0;
	if((xres > yres) && (disp_rotate == 0 || disp_rotate == 180) && vinfo.bits_per_pixel == 32) {
		unsigned long onoff = 0;

		ioctl(fbfd, HCFBIOGET_SCALE_ONOFF, &onoff);
		kaf_enabled = !onoff;
	}
	struct kaf_ctx_t *kaf_ctx = kaf_get_ctx();

	if (kaf_enabled) {
		static int kaf_initialized = 0;
		if (!kaf_initialized) {
			unsigned knob = KAF_KNOB_NONE;
			if (LV_HC_KEYSTONE_AA_SW_FIX_ALWAYS_CHOP_BAD_EDGE) {
				knob |= KAF_KNOB_ALWAYS_CHOP_EDGE_DEAD_ZONE;
			}
			kaf_init(kaf_ctx, xres, yres, knob);
			kaf_initialized = 1;
		}
		kaf_start_frame(kaf_ctx);
		if (!draw_to_swap_buf) {
			kaf_catch_edge(kaf_ctx, disp_rotate, h_flip, v_flip, src_area, src_buf, sw);
			lv_area_t chopped;
			kaf_chop_edge(kaf_ctx, src_area, disp_rotate, h_flip, v_flip, &chopped);
			set_rect_per_chopping(src_area, &chopped, &srect);
			drect.x1 += srect.x1;// h_flip is ignored here
		}
	}
#endif
	lv_ge_blit(&blit, false);

#if LV_HC_DRAW_BUF_COUNT < 2
	lv_ge_lock();
	hcge_engine_sync(hcge_ctx);
	lv_ge_unlock();
#endif
}

#if CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN
static void fix_rotate_blit(lv_color_t *src_buf,
                      lv_area_t *src_area,
                      lv_coord_t src_stride,
                      HCGESurfacePixelFormat src_fmt,
                      lv_color_t *dst_buf,
                      lv_coord_t dst_stride,
                      HCGESurfacePixelFormat dst_fmt,
                      lv_coord_t xres,
                      lv_coord_t yres,
                      uint32_t rotate
                     )
{
	const int32_t act_x1 = src_area->x1;
	const int32_t act_y1 = src_area->y1;
	lv_coord_t sw = lv_area_get_width(src_area);
	lv_coord_t sh = lv_area_get_height(src_area);

	struct lv_blit blit = { 0 };
	lv_area_t crop, srect, drect;
	lv_point_t pivot = { 0 };

	blit.dst_buf = dst_buf;
	blit.dst_stride = dst_stride;
	blit.dst_fmt = dst_fmt;
	blit.src_buf = src_buf;
	blit.src_stride = src_stride;
	blit.src_fmt = src_fmt;
	blit.opa = LV_OPA_COVER;

	int dx;
	int dy;

#if ROTATE_DEBUG
	if(rotate) {
		int x, y;
		for(x = src_area->x1; x <=src_area->x2; x++) {
			src_buf[src_area->y1 * src_stride + x].full = 0xFFFF0000;
			src_buf[src_area->y2 * src_stride + x].full = 0xFFFF0000;
		}
		for(y = src_area->y1; y <=src_area->y2; y++) {
			src_buf[y * src_stride + src_area->x1].full = 0xFFFF0000;
			src_buf[y * src_stride + src_area->x2].full = 0xFFFF0000;
		}
		cacheflush(src_buf, screen_size, DCACHE);

	}
#endif

	if(rotate == 270) {
		dx = xres - act_y1 - sh;
		if((dx + sh) > xres)
			sh = xres - dx;
		dy = act_x1;
	} else if (rotate == 90)  {
		dy = yres - act_x1 - sw;
		if((dy + sw) > yres)
			sw = yres - dy;
		dx = act_y1;
	} else if(rotate == 180) {
		dx = xres  - act_x1 - sw;
		dy = yres  - act_y1 - sh;
	} else {
		dx = act_x1;
		dy = act_y1;
	}

	ASIGN_PRECT(&srect, 0, 0, sw - 1, sh - 1);
	ASIGN_PRECT(&crop, 0, 0, xres - 1, yres - 1);
	if(rotate == 270 || rotate  == 90) {
		ASIGN_PRECT(&drect, dx, dy, dx + sh - 1, dy + sw - 1);
	} else {
		ASIGN_PRECT(&drect, dx, dy, dx + sw - 1, dy + sh - 1);
	}

	blit.src_area = &srect;
	blit.dst_area = &drect;
	blit.angle = rotate;
	pivot.x = sw/2;
	pivot.y = sh/2;
	blit.pivot = &pivot;

	blit.crop = &crop;

	lv_ge_blit(&blit, false);
}
#endif

static void canvas_flip(lv_coord_t xres,
                        lv_coord_t yres,
                        lv_color_t *src,
                        lv_color_t *dst,
                        HCGESurfacePixelFormat pixel_fmt)
{
	struct lv_blit blit = { 0 };
	lv_area_t drect = { 0 };
	lv_area_t srect = { 0 };
	lv_area_t crop = { 0 };

	ASIGN_PRECT(&srect, 0, 0, xres - 1, yres - 1);
	ASIGN_PRECT(&drect, 0, 0, xres - 1, yres - 1);
	ASIGN_PRECT(&crop, 0, 0, xres - 1, yres - 1);
	blit.dst_stride = blit.src_stride = xres;
	blit.dst_buf = dst;
	blit.dst_area = &drect;
	blit.src_buf = src;
	blit.src_fmt = blit.dst_fmt = pixel_fmt;
	blit.src_area = &srect;
	blit.crop = &crop;
	blit.opa = LV_OPA_COVER;
	blit.v_flip = v_flip;
	blit.h_flip = h_flip;
	blit.angle = 0;
#if LV_HC_KEYSTONE_AA_SW_FIX
	int dx = 0;
	int kaf_enabled = 0;

	if((xres > yres) && (disp_rotate == 0 || disp_rotate == 180) && vinfo.bits_per_pixel == 32) {
		unsigned long onoff = 0;

		ioctl(fbfd, HCFBIOGET_SCALE_ONOFF, &onoff);
		kaf_enabled = !onoff;
	}
	struct kaf_ctx_t *kaf_ctx = kaf_get_ctx();


	if (kaf_enabled) {
		lv_area_t src_area = {0, 0, xres - 1, yres - 1};
		kaf_catch_edge(kaf_ctx, 0, h_flip, v_flip, &src_area, fbp_swap_buf, xres);
		lv_area_t chopped;
		kaf_chop_edge(kaf_ctx, &src_area, 0, h_flip, v_flip, &chopped);
		set_rect_per_chopping(&src_area, &chopped, &srect);
		dx += h_flip ? src_area.x2 - chopped.x2 : chopped.x1 - src_area.x1;
		drect.x1 = dx;
	}
#endif
	lv_ge_blit(&blit, false);
}

static void canvas_stretch(lv_color_t *src,
                           lv_area_t *src_area,
                           lv_color_t *dst,
                           lv_area_t *dst_area,
                           HCGESurfacePixelFormat pixel_fmt)
{
	struct lv_blit blit = { 0 };
	lv_area_t crop = { 0, 0, vinfo.xres - 1, vinfo.yres - 1};

	blit.dst_buf = dst;
	blit.dst_area = dst_area;
	blit.dst_stride = vinfo.xres;
	blit.src_buf = src;
	blit.src_fmt = blit.dst_fmt = pixel_fmt;
	blit.src_area = src_area;
	blit.src_stride = lv_area_get_width(src_area);
	blit.opa = LV_OPA_COVER;
	blit.crop = &crop;
	blit.v_flip = 0;
	blit.h_flip = 0;
	blit.angle = 0;
	blit.stretch = true;
	blit.cover = true;
	lv_ge_blit(&blit, false);
}

static void coord_convert(lv_area_t *src_area, lv_area_t *dst_area, int xres, int yres, int rotate)
{
	lv_coord_t sw = lv_area_get_width(src_area);
	lv_coord_t sh = lv_area_get_height(src_area);

	if(rotate == 270) {
		dst_area->x1 = xres - src_area->y1 - sh;
		dst_area->y1 = src_area->x1;
		dst_area->x2 = dst_area->x1 + sh - 1;
		dst_area->y2 = dst_area->x1 + sw - 1;
	} else if (rotate == 90)  {
		dst_area->x1 = src_area->y1;
		dst_area->y1 = yres - src_area->x1 - sw;
		dst_area->x2 = dst_area->x1 + sh - 1;
		dst_area->y2 = dst_area->y1 + sw - 1;
	} else if(rotate == 180) {
		dst_area->x1 = xres - src_area->x1 - sw;
		dst_area->y1 = yres - src_area->y1 - sh;
		dst_area->x2 = dst_area->x1 + sw - 1;
		dst_area->y2 = dst_area->y1 + sh - 1;
	} else {
		dst_area->x1 = src_area->x1;
		dst_area->y1 = src_area->y1;
		dst_area->x2 = src_area->x2;
		dst_area->y2 = src_area->y2;
	}

#if KW_DEBUG
	printf("src_area: %d, %d, %d, %d, rotate: %d\n", src_area->x1, src_area->x2, src_area->y1, src_area->y2, rotate);
	printf("dst_area: %d, %d, %d, %d, rotate: %d\n", dst_area->x1, dst_area->x2, dst_area->y1, dst_area->y2);
	printf("xres: %d, yres: %d\n", xres, yres);
#endif
}

#if LV_USE_DUAL_FRAMEBUFFER == 1
static void fb_swap(lv_disp_drv_t *drv, HCGESurfacePixelFormat pixel_fmt)
{
	int ret;
	struct lv_blit blit = { 0 };
	lv_area_t drect = { 0 };
	lv_area_t crop = { 0, 0, vinfo.xres - 1, vinfo.yres - 1};

	if(draw_fb == fbp0)
		vinfo.yoffset = 0;
	else
		vinfo.yoffset = vinfo.yres;
	ret = ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo);
	if (ret < 0) {
		perror("ioctl() / FBIOPAN_DISPLAY");
	}

	ret = 0;
	ioctl(fbfd, FBIO_WAITFORVSYNC, &ret);
	if (ret < 0) {
		perror("ioctl() / FBIO_WAITFORVSYNC");
	}

#if LV_HC_KEYSTONE_AA_SW_FIX
	if (0) {
#else
	if (disp_rotate == 0 && !h_flip && !v_flip) {
#endif
		lv_disp_t *disp = _lv_refr_get_disp_refreshing();
		for (int i = 0; i < disp->inv_p; i++) {
			if (disp->inv_area_joined[i] == 0) {
				lv_color_t *src_buf, *dst_buf;
				if (draw_fb == fbp0) {
					src_buf = (lv_color_t *)fbp0;
					dst_buf = (lv_color_t *)fbp1;
				} else {
					src_buf = (lv_color_t *)fbp1;
					dst_buf = (lv_color_t *)fbp0;
				}
				update_canvas(src_buf, &disp->inv_areas[i], vinfo.xres, pixel_fmt, dst_buf,
				              vinfo.xres, pixel_fmt, vinfo.xres, vinfo.yres, true);
			}
		}
	} else {
		ASIGN_PRECT(&drect, 0, 0, vinfo.xres - 1, vinfo.yres - 1);
		blit.dst_stride = blit.src_stride = vinfo.xres;
		blit.dst_buf = (lv_color_t *)((draw_fb == fbp0)? fbp1: fbp0);
		blit.dst_area = blit.src_area = &drect;
		blit.crop = &crop;
		blit.src_buf = (lv_color_t *)draw_fb;
		blit.src_fmt = blit.dst_fmt = pixel_fmt;
		blit.opa = LV_OPA_COVER;
		blit.v_flip = 0;
		blit.h_flip = 0;
		blit.angle = 0;
		lv_ge_blit(&blit, false);
	}

	draw_fb = (draw_fb == fbp0)?fbp1:fbp0;
	FBDEV_DBG("flip: vinfo.yoffset: %03d, draw_fb:%p, fbp0: %p, fbp1: %p\n", vinfo.yoffset,
	          draw_fb, fbp0, fbp1);
}
#endif

void fbdev_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
	int32_t x_limit = (int32_t)vinfo.xres;
	int32_t y_limit = (int32_t)vinfo.yres;
	bool fb_init = false;

	if((disp_rotate == 90 || disp_rotate == 270) && !screen_dir) {
		x_limit = (int32_t)vinfo.yres;
		y_limit = (int32_t)vinfo.xres;
	}

	if(draw_fb == NULL ||
	   area->x2 < 0 ||
	   area->y2 < 0 ||
	   area->x1 > x_limit - 1 ||
	   area->y1 > y_limit - 1) {
		lv_disp_flush_ready(drv);
		return;
	}

	lv_color_t *dst_buf;
	HCGESurfacePixelFormat dst_fmt;
	lv_coord_t dst_stride;
	lv_coord_t src_stride;
	HCGESurfacePixelFormat src_fmt;
	lv_coord_t sw = lv_area_get_width(area);
	lv_coord_t sh = lv_area_get_height(area);

	if (kaf_enable && kaf_rect_update) {
		pthread_mutex_lock(&kaf_rec_mutex);
		memcpy(&kaf_rect, &kaf_rect_rcu, sizeof(kaf_rect));
		kaf_rect_update = false;
		fb_init = true;
		pthread_mutex_unlock(&kaf_rec_mutex);
	}

	draw_to_swap_buf = !screen_dir && fbp_swap_buf && (h_flip || v_flip);

	hor_screen_opt = FB_OPT_NONE;
	if(screen_dir && (h_flip ||v_flip) && fbp_swap_buf)
		hor_screen_opt |= FB_FLIP;
	if(screen_dir && (disp_rotate == 90 || disp_rotate == 270) && virt_disp)
		hor_screen_opt |= FB_ROTATE_90_270;

#if CONFIG_LV_HC_KEYSTONE_EQUAL_SCALE
    if (!virt_disp) {
        virt_disp = lv_mem_alloc(drv->hor_res * drv->ver_res * 4);
    }
    assert(virt_disp != NULL);
#endif

	/*
	 *
	 * horizontal screen flow:
	 *                              flip           keystone correction 
	 * color_p ------> fbp_swap_buf ------> kaf_buf----------------------> fbp0/fbp1
	 *
	 *
	 * horizontal screen rotate flow:
	 *          rotate             flip                 stretch
	 * color_p -------> virt_disp ------> fbp_swap_buf ----------> fbp0/fbp1
	 *                       |  stretch
	 *                       |-----------fbp0/fbp1
	 *
	 *
	 * vertical screen rotate flow:
	 *          rotate                flip
	 * color_p -------> fbp_swap_buf ---------->fbp0/fbp1
	 *
	 *
	 * CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN == 1 && disp_rotate > 0,vertical screen rotate flow:
	 *           blit                rotate                flip
	 * color_p ------->rotate_bf_buf-------> fbp_swap_buf -------->fbp0/fbp1
	 *                           | rotate
	 *                           |-------->fbp0/fbp1
	 *
	 * CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN == 1 && disp_rotate > 0 && keystone correction, vertical screen rotate flow:
	 *           blit                rotate                flip            keystone correction 
	 * color_p ------->rotate_bf_buf-------> fbp_swap_buf -------->kt_buf---------------------fbp0/fbp1
	 *                           | rotate         keystone correction
	 *                           |-------->kt_buf--------------------->fbp0/fbp1

	 * */
	if(draw_to_swap_buf || hor_screen_opt == FB_FLIP) {
		dst_buf = (lv_color_t *)fbp_swap_buf;
    } else if(hor_screen_opt == FB_ROTATE_90_270 || hor_screen_opt == FB_ROTATE_FLIP) {
		dst_buf = (lv_color_t *)virt_disp;
    } else {
#if CONFIG_LV_HC_KEYSTONE_EQUAL_SCALE
        dst_buf = (lv_color_t *)virt_disp;
#else
        dst_buf = draw_fb;
#endif
    }

	dst_stride = vinfo.xres;
	dst_fmt = fb_pixel_fmt;

	src_stride = sw;
#if LV_COLOR_DEPTH == 16
	src_fmt = HCGE_DSPF_RGB16;
#elif LV_COLOR_DEPTH == 32
	src_fmt = HCGE_DSPF_ARGB;
#else
#error "Don't support color format"
#endif
	cacheflush(draw_fb, screen_size, DCACHE);
	cacheflush(color_p, (sw * sh) * sizeof(lv_color_t), DCACHE);
#if CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN
	if (!screen_dir && rotate_bf_buf && disp_rotate) {
		dst_buf = rotate_bf_buf;
		fix_rotate_blit(color_p, area, src_stride, src_fmt, dst_buf, LV_HOR_RES, dst_fmt,
		          LV_HOR_RES, LV_VER_RES, 0);
	} else {
		if(hor_screen_opt == FB_ROTATE_90_270 || hor_screen_opt == FB_ROTATE_FLIP)
			update_canvas(color_p, area, src_stride, src_fmt, dst_buf, vinfo.yres, dst_fmt,
			              vinfo.yres, vinfo.xres, false);
		else
			update_canvas(color_p, area, src_stride, src_fmt, dst_buf, dst_stride, dst_fmt,
			              vinfo.xres, vinfo.yres, false);
	}
#else
	if(hor_screen_opt == FB_ROTATE_90_270 || hor_screen_opt == FB_ROTATE_FLIP) {
		update_canvas(color_p, area, src_stride, src_fmt, dst_buf, vinfo.yres, dst_fmt,
				vinfo.yres, vinfo.xres, false);
	} else {
		if (kaf_enable && !draw_to_swap_buf && disp_rotate == 0) {
			dst_buf = (lv_color_t *)kaf_buf;
		}
#if CONFIG_LV_HC_KEYSTONE_EQUAL_SCALE
        update_canvas(color_p, area, src_stride, src_fmt, dst_buf, drv->hor_res, dst_fmt,
                drv->hor_res, drv->ver_res, false);
#else
        update_canvas(color_p, area, src_stride, src_fmt, dst_buf, dst_stride, dst_fmt,
                vinfo.xres, vinfo.yres, false);
#endif
	}
#endif

	if(lv_disp_flush_is_last(drv)) {
#if CONFIG_LV_HC_ROTATE_BY_FULL_SCREEN
		if (rotate_bf_buf && disp_rotate) {
			lv_area_t rotate_area = {
				0,
				0,
				LV_HOR_RES - 1,
				LV_VER_RES - 1
			};
			if (kaf_enable) {
				if (draw_to_swap_buf) {
					fix_rotate_blit(rotate_bf_buf, &rotate_area, LV_HOR_RES, src_fmt, fbp_swap_buf, vinfo.xres, dst_fmt,
							vinfo.xres, vinfo.yres, disp_rotate);
				} else {
					lv_area_t src_area;
					lv_area_t dst_area;
					lv_area_t kaf_area = {kaf_rect.x, kaf_rect.y,
						kaf_rect.x + kaf_rect.w - 1,
						kaf_rect.y + kaf_rect.h - 1};
					coord_convert(&rotate_area, &src_area, vinfo.xres, vinfo.yres, disp_rotate);
					coord_convert(&kaf_area, &dst_area, vinfo.xres, vinfo.yres, disp_rotate);

					fix_rotate_blit(rotate_bf_buf, &rotate_area, LV_HOR_RES, src_fmt, kaf_buf, vinfo.xres, dst_fmt,
							vinfo.xres, vinfo.yres, disp_rotate);
					if (fb_init) {
						lv_memset(draw_fb, 0, screen_size);
						fb_init = false;
					}
					canvas_stretch((lv_color_t *)kaf_buf, &src_area, draw_fb, &dst_area, dst_fmt);
				}

			} else {
				fix_rotate_blit(rotate_bf_buf, &rotate_area, LV_HOR_RES, src_fmt, (draw_to_swap_buf) ? fbp_swap_buf: draw_fb, vinfo.xres, dst_fmt,
						vinfo.xres, vinfo.yres, disp_rotate);
			}
		}
#endif

		if(draw_to_swap_buf || hor_screen_opt == FB_FLIP) {
			lv_area_t rotate_area = {
				0,
				0,
				LV_HOR_RES - 1,
				LV_VER_RES - 1
			};

			if (kaf_enable) {
				lv_area_t src_area;
				lv_area_t dst_area;
				lv_area_t kaf_area = {kaf_rect.x, kaf_rect.y,
					kaf_rect.x + kaf_rect.w - 1,
					kaf_rect.y + kaf_rect.h - 1};
				coord_convert(&rotate_area, &src_area, vinfo.xres, vinfo.yres, disp_rotate);
				coord_convert(&kaf_area, &dst_area, vinfo.xres, vinfo.yres, disp_rotate);

				canvas_flip(vinfo.xres, vinfo.yres, (lv_color_t *)fbp_swap_buf,
						(lv_color_t *)kaf_buf, dst_fmt);
				if (fb_init) {
					lv_memset(draw_fb, 0, screen_size);
					fb_init = false;
				}
				canvas_stretch((lv_color_t *)kaf_buf, &src_area, draw_fb, &dst_area, dst_fmt);
			} else {
#if CONFIG_LV_HC_KEYSTONE_EQUAL_SCALE
				canvas_flip(drv->hor_res, drv->ver_res, (lv_color_t *)fbp_swap_buf,
						(lv_color_t *)virt_disp, dst_fmt);
#else
				canvas_flip(vinfo.xres, vinfo.yres, (lv_color_t *)fbp_swap_buf,
						(lv_color_t *)draw_fb, dst_fmt);
#endif
			}
		} else if(hor_screen_opt == FB_ROTATE_90_270 || hor_screen_opt == FB_ROTATE_FLIP) {
			lv_area_t src_area = {0, 0, vinfo.yres - 1, vinfo.xres - 1};
			lv_area_t dst_area = {
				stretch_dx,
				stretch_dy,
				stretch_dx + vinfo.yres*vinfo.yres/vinfo.xres - 1,
				stretch_dy + vinfo.yres - 1,
			};
			if(hor_screen_opt_bg != hor_screen_opt
					|| v_flip_bg != v_flip
					|| h_flip_bg != h_flip
					|| stretch_dx_bg != stretch_dx
					|| stretch_dy_bg != stretch_dy
					|| disp_rotate_bg != disp_rotate) {
				lv_memset(draw_fb, 0, screen_size);
				hor_screen_opt_bg = hor_screen_opt;
				v_flip_bg = v_flip;
				h_flip_bg = h_flip;
				stretch_dx_bg = stretch_dx;
				stretch_dy_bg = stretch_dy;
				disp_rotate_bg = disp_rotate;
			}
			if(hor_screen_opt == FB_ROTATE_FLIP) {
				canvas_flip(vinfo.yres, vinfo.xres, (lv_color_t *)virt_disp,
						(lv_color_t *)fbp_swap_buf, dst_fmt);
				lv_ge_lock();
				hcge_engine_sync(hcge_ctx);
				lv_ge_unlock();
				hcge_reset(hcge_ctx);
				canvas_stretch((lv_color_t *)fbp_swap_buf, &src_area, draw_fb, &dst_area, dst_fmt);
			} else
				canvas_stretch((lv_color_t *)virt_disp, &src_area, draw_fb, &dst_area, dst_fmt);
		} else {

			if (kaf_enable) {
				lv_area_t src_area = {0, 0, vinfo.xres - 1, vinfo.yres - 1};
				lv_area_t dst_area = {kaf_rect.x, kaf_rect.y,
					kaf_rect.x + kaf_rect.w - 1,
					kaf_rect.y + kaf_rect.h - 1};

				dst_fmt = fb_pixel_fmt;
				if (fb_init) {
					lv_memset(draw_fb, 0, screen_size);
					fb_init = false;
				}
				canvas_stretch((lv_color_t *)kaf_buf, &src_area, draw_fb, &dst_area, dst_fmt);
			}
		}

#if CONFIG_LV_HC_KEYSTONE_EQUAL_SCALE
            lv_area_t src_area = {0, 0, drv->hor_res - 1, drv->ver_res - 1};
            lv_area_t dst_area = {
                kaf_viewport.x,
                kaf_viewport.y,
                kaf_viewport.x + kaf_viewport.width - 1,
                kaf_viewport.y + kaf_viewport.height - 1,
            };
            lv_area_t dst_area1 = {
                0, 0, vinfo.xres - 1, vinfo.yres - 1
            };

            FBDEV_DBG("dst_area->x1: %d, dst_area->y1: %d, dst_area->x2: %d, dst_area->y2: %d\n", dst_area.x1, dst_area.y1,
                    dst_area.x2, dst_area.y2);
            if (kaf_viewport_bg.y < kaf_viewport.y
                    || (kaf_viewport_bg.y + kaf_viewport_bg.height) > (kaf_viewport.y + kaf_viewport.height)) {
                if(kaf_viewport_bg.y < kaf_viewport.y){
                    lv_memset(draw_fb + kaf_viewport_bg.y*vinfo.xres*pixel_bytes, 0,
                            (kaf_viewport.y - kaf_viewport_bg.y)*vinfo.xres*pixel_bytes);
                }

                if((kaf_viewport_bg.y + kaf_viewport_bg.height) > (kaf_viewport.y + kaf_viewport.height)) {
                    lv_memset(draw_fb + (kaf_viewport.y + kaf_viewport.height)*vinfo.xres*pixel_bytes, 0,
                            ((kaf_viewport_bg.y + kaf_viewport_bg.height) - (kaf_viewport.y + kaf_viewport.height))*vinfo.xres*pixel_bytes);
                }
            }
            kaf_viewport_bg = kaf_viewport;

            canvas_stretch((lv_color_t *)virt_disp, &src_area, draw_fb, &dst_area, dst_fmt);

            hcge_engine_sync(hcge_ctx);
            static struct kaf_ctx_t *af = NULL;
            if(!af) {
                af = kaf_get_ctx();
                kaf_init(af, vinfo.xres, vinfo.yres, 0);
            }
            kaf_start_frame(af);
            kaf_catch_edge(af, 0, 0, 0, &dst_area1, draw_fb, vinfo.xres);
            const int fix_flag = kaf_fix(af);
            blit_ks_fix(af, fix_flag, vinfo.xres, vinfo.yres, vinfo.xres, draw_fb);
            kaf_end_frame(af);
#endif

#if LV_HC_KEYSTONE_AA_SW_FIX
		int kaf_enabled = 0;
		if((vinfo.xres > vinfo.yres) && (disp_rotate == 0 || disp_rotate == 180) && vinfo.bits_per_pixel == 32) {
			unsigned long onoff = 0;

			ioctl(fbfd, HCFBIOGET_SCALE_ONOFF, &onoff);
			kaf_enabled = !onoff;
		}
		struct kaf_ctx_t *kaf_ctx = kaf_get_ctx();

		if (kaf_enabled) {
			const int fix_flag = kaf_fix(kaf_ctx);
			if (fix_flag) {
				blit_ks_fix(kaf_ctx, fix_flag, vinfo.xres, vinfo.yres, vinfo.xres, draw_fb);
			}
			kaf_end_frame(kaf_ctx);
		}
#endif

#if LV_USE_DUAL_FRAMEBUFFER == 1
		fb_swap(drv, dst_fmt);
#endif
		if(fb_onoff==0) {
			if (ioctl(fbfd, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
				perror("ioctl(FBIOBLANK)");
				lv_ge_unlock();
				return;
			}
			fb_onoff = 1;
		}
	}

	lv_disp_flush_ready(drv);
}
#endif

void fbdev_get_sizes(uint32_t *width, uint32_t *height)
{
	if (width)
		*width = vinfo.xres;

	if (height)
		*height = vinfo.yres;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static uint32_t fbdev_buffer_start_virt;
static uint32_t fbdev_buffer_start_phy;
static uint32_t fbdev_buffer_off;
static uint32_t fbdev_buffer_size;
#define FBDEV_ALIGN16(addr) ((addr&0xF) ? (addr + 0xF)&0xF : (addr))
#define FBDEV_ALIGN32(addr) ((addr&0x1F) ? (addr + 0x1F)&0x1F : (addr))
static void fbdev_buffer_init(uint32_t virt_addr, uint32_t phy_addr, uint32_t size)
{
	uint32_t addr = FBDEV_ALIGN32(virt_addr);
	int s;
	int round = addr - virt_addr;
	s = size - round;
	if(s < 0) {
		perror("Parameter error.\n");
		return;
	}
	if(addr != virt_addr && (phy_addr + round)&HC_MEM_ALIGN_MASK) {
		perror("physical address is not align with 16bytes\n");
		return;
	}

	fbdev_buffer_size = s;
	fbdev_buffer_start_phy = phy_addr + round;
	fbdev_buffer_start_virt = virt_addr + round;
	fbdev_buffer_off = 0;
	printf("fbdev_buffer_start_phy: 0x%08x, fbdev_buffer_start_virt: 0x%08x\n",
	       (int)fbdev_buffer_start_phy, (int)fbdev_buffer_start_virt);
}

void  *fbdev_virt_to_phy(void *virt_addr)
{
#ifndef __HCRTOS__
	return (void *)(fbdev_buffer_start_phy + ((uint32_t)virt_addr - fbdev_buffer_start_virt));
#else
	return PHY_ADDR(virt_addr);
#endif
}

bool fbdev_check_addr(uint32_t virt_addr, uint32_t size)
{
#ifdef __linux__
	if(virt_addr < (uint32_t)fbp0 || (virt_addr + size) > ((uint32_t)fbp0 + finfo.smem_len)) {
		FBDEV_DBG("virt_addr over range:addr = 0x%08x, size = 0x%08x\n", (int)virt_addr, (int)size);
		return false;
	}
#endif

	if(virt_addr & 0x1F) {
		printf("virt_addr not align: 0x%08x\n", (int)virt_addr);
		return false;
	}

	return true;
}

void *fbdev_static_malloc_virt(int size)
{
	uint32_t addr;
	size = (size + HC_MEM_ALIGN_MASK)&(~HC_MEM_ALIGN_MASK);
	if(size & HC_MEM_ALIGN_MASK) {
		perror("size is not align with 32bytes\n");
		return NULL;
	}

	if(fbdev_buffer_size < size) {
		printf("fbdev:not enough memory, required %d but only %ld available\n", size, fbdev_buffer_size);
		return NULL;
	}

	addr = fbdev_buffer_start_virt + fbdev_buffer_off;
	fbdev_buffer_off += size;
	fbdev_buffer_size -= size;

	FBDEV_DBG("return addr: 0x%08x, fbdev_buffer_off: %u\n", (int)addr, (int)fbdev_buffer_off);

	return (void *)addr;
}

uint32_t fbdev_get_buffer_size(void)
{
	return fbdev_buffer_size;
}

void fbdev_wait_cb(struct _lv_disp_drv_t * disp_drv)
{
#if 0
	int ret = 0;
	ioctl(fbfd, FBIO_WAITFORVSYNC, &ret);
	if (ret < 0) {
		perror("ioctl() / FBIO_WAITFORVSYNC");
	}
	FBDEV_DBG("%s\n", __func__);
#endif
}

void fbdev_set_offset(uint32_t xoffset, uint32_t yoffset)
{
	vinfo.xoffset = xoffset;
	vinfo.yoffset = yoffset;
}

void fbdev_set_rotate(int rotate, int hor_flip, int ver_flip)
{
	lv_disp_t *disp = lv_disp_get_default();

	if(flip_support && !fbp_swap_buf) {
		fbp_swap_buf = lv_mem_alloc(screen_size);
		if(!fbp_swap_buf) {
			printf("Enable flip_support, But not enough memory\n");
			return;
		}
	}

	if(fbp_swap_buf) {
		h_flip = hor_flip;
		v_flip = ver_flip;
	}

	disp_rotate = rotate;

	//Used flip to replace rotate to speed up display
	if(flip_support && disp_rotate == 180) {
		disp_rotate = 0;
		h_flip = (h_flip == 0)?1:0;
		v_flip = (v_flip == 0)?1:0;
	}

	lv_disp_drv_update(disp, disp->driver);
	FBDEV_DBG("%s:disp_rotate: %d, hor_flip: %d, ver_flip: %d\n", __func__, disp_rotate, h_flip, v_flip);
}


/*
 * This api for horizontal screen to rotate and scale.
 * Rotate = {0, 90, 180, 270}, with anti-clockwise,
 * yoff and xoff is valid only with rotate equal to 90 or 270,
 * which display UI with left-top corner offset.
 * hor_flip: 1 is horizontal flip, 0 is nothing.
 * ver_flip: 1 is vertical flip, 0 is nothing.
 */
void fbdev_hor_scr_rotate(int rotate, int hor_flip, int ver_flip,
                          int xoff, int yoff)
{
	lv_disp_t *disp = lv_disp_get_default();

	if(flip_support && !fbp_swap_buf) {
		fbp_swap_buf = lv_mem_alloc(screen_size);
		if(!fbp_swap_buf) {
			printf("Enable flip_support, But not enough memory\n");
			return;
		}
	}

	if(screen_dir && !virt_disp) {
		virt_disp = lv_mem_alloc(screen_size);
		if(!fbp_swap_buf) {
			printf("Not enough memory\n");
			return;
		}
	}


	if(fbp_swap_buf) {
		h_flip = hor_flip;
		v_flip = ver_flip;
	}

	disp_rotate = rotate;

	//Used flip to replace rotate to speed up display
	if(flip_support && disp_rotate == 180) {
		disp_rotate = 0;
		h_flip = (h_flip == 0)?1:0;
		v_flip = (v_flip == 0)?1:0;
	}

	if(stretch_dx >= 0 && stretch_dx < vinfo.xres)
		stretch_dx = xoff;
	if(stretch_dy >= 0 && stretch_dy < vinfo.yres)
		stretch_dy = yoff;

	FBDEV_DBG("%s:disp_rotate: %ld, hor_flip: %ld, ver_flip: %ld, xoff: %d, yoff: %d\n",
	          __func__, disp_rotate, h_flip,
	          v_flip, xoff, yoff);

	lv_disp_drv_update(disp, disp->driver);
}


int fbdev_keystone_config(int x0, int y0, int width, int height)
{
	lv_disp_t *disp = lv_disp_get_default();

	if (screen_dir) {
		printf("horizontal screen isn't supported!\n");
		return -1;
	}

	if (x0 < 0 || y0 < 0 || (x0 + width) > lv_disp_get_hor_res(disp)
			|| (y0 + height) > lv_disp_get_ver_res(disp))
		return -EINVAL;

	if(!kaf_buf) {
		kaf_buf = lv_mem_alloc(screen_size);
		if(!kaf_buf) {
			printf("Enable keystone correction, but not enough memory\n");
			return -ENOMEM;
		}
	}

	if (kaf_enable && kaf_rect.x == x0 && kaf_rect.y == y0
			&& width == kaf_rect.w && height == kaf_rect.h)
		return 0;
	pthread_mutex_lock(&kaf_rec_mutex);
	if (x0 != 0 || y0 != 0 || width != lv_disp_get_hor_res(disp)
			||height != lv_disp_get_ver_res(disp)) {
		kaf_rect_rcu.x = x0;
		kaf_rect_rcu.y = y0;
		kaf_rect_rcu.w = width;
		kaf_rect_rcu.h = height;
		kaf_enable = true;
		kaf_rect_update = true;

	} else {
		kaf_enable = false;
	}
	pthread_mutex_unlock(&kaf_rec_mutex);

	lv_disp_drv_update(disp, disp->driver);
	return 0;
}

void fbdev_set_viewport(hcfb_viewport_t *viewport)
{
#if CONFIG_LV_HC_KEYSTONE_EQUAL_SCALE
	lv_disp_t *disp = lv_disp_get_default();
    kaf_viewport = *viewport;
    FBDEV_DBG("kaf_viewport.x: %d, kaf_viewport.y: %d, kaf_viewport.width: %d, kaf_viewport.height:%d\n",
            kaf_viewport.x, kaf_viewport.y, kaf_viewport.width, kaf_viewport.height);

	lv_disp_drv_update(disp, disp->driver);
#endif
}
#endif
