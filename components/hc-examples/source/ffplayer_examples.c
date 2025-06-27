#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>

#ifdef __linux__
#include <sys/msg.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include "console.h"
#include "ge_draw_subtitle.h"
#include <linux/fb.h>
#include <hcge/ge_api.h>

#else
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <kernel/lib/console.h>
#include "showlogo.h"
#include <kernel/lib/fdt_api.h>
#endif

#include <libavutil/common.h>
#include <libavutil/avstring.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <hcuapi/common.h>
#include <hcuapi/avsync.h>
#include <hcuapi/snd.h>
#include <hcuapi/dumpstack.h>
#include <hcuapi/dis.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/vidsink.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <ffplayer.h>
#include <ffplayer_manager.h>
#include <glist.h>
#include <getopt.h>
#include "dis_test.h"
#include "vin_dvp_test.h"
#include "hdmi_tx_test.h"
#include "es_decoder.h"

#ifdef __linux__
#include <autoconf.h>
#endif

#define MAX_SCAN_FILE_LIST_LEN (500)
#define SCAN_SUB_DIR 0

typedef struct mediaplayer {
	void *player;
	char *uri;
	char *username;

    dis_type_e dis_type;
    dis_layer_e dis_layer;

} mediaplayer;

static mediaplayer *g_mp = 0;
#ifdef __linux__
static int g_msgid = -1;
static struct HCGeDrawSubtitle ge_draw_subtitle = {0};
#else
static QueueHandle_t g_msgid = NULL;
#endif

static pthread_t msg_rcv_thread_id = 0;
static pthread_t get_info_thread_id = 0;
static glist *g_plist = NULL;//for recode multi play
static glist *g_plist2 = NULL;//for recode multi play
static bool g_mpabort = false;
static bool g_mp_info = false;
static int g_mp_info_interval = 200;//ms
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_multi_ins = false;
static glist *url_list = NULL;
static glist *url_list2 = NULL;
static int g_cur_ply_idx = 0;
static int g_cur_ply_idx2 = 0;

static pthread_t auto_switch_thread_id = 0;
static int g_stop_auto_switch = 0;
static pthread_t auto_seek_thread_id = 0;
static int g_stop_auto_seek = 0;
static int g_sync_mode = 2;
static int g_sync_mode2 = 0;
static int g_loop_play = 1;
/* i2so: 1 << 0, spo:  1 << 2,  i2so |spo: 1 << 0 | 1 <<2 */
static AudioOutDev g_snd_devs = AUDDEV_I2SO;
static double g_time_ms = 0;
static bool g_buffering_enable = 0;
static int g_mix_priority = 0;
static bool g_mix_maximum_weight = 0;
static int g_slave_mode = 0;
static int g_bypass = 0;
static img_dis_mode_e g_img_dis_mode = 0;
static rotate_type_e g_rotate_type = 0;
static mirror_type_e g_mirror_type = 0;
static int g_project_mode = 0; //0: PROJECT_REAR; 1: PROJECT_CEILING_REAR; 2: PROJECT_FRONT; 3: PROJECT_CEILING_FRONT

static int g_audio_flush_thres = 0;
static int g_en_subtitle = 0;
static int g_pic_show_duration = 3000;//all pic play with 3000ms delay
static int g_gif_interval = 200;//gif frame 200ms interval.
static image_effect_t g_img_effect = {0};
static HCAlphaBlendMode g_pic_bg = ALPHA_BLEND_UNIFORM;
static bool g_preview_enable = 0;
static vdec_dis_rect_t g_dis_rect = {{0,0,1920,1080},{0,0,1920,1080}};
static bool g_closevp = true,  g_fillblack = false;
static bool g_smooth_mode = false;
static uint8_t g_volume = 100;
static FILE *g_mem_play_file = NULL;
static struct video_transcode_config g_vtranscode = {0};
void *g_vtranscode_path = NULL;
static void *g_mp2 = NULL;
static bool g_disable_video = false;
static bool g_disable_audio = false;
static char *decryption_key = NULL;
static bool g_b_aux_layer = false;
static video_pbp_mode_e g_pbp_mode = 0;
static dis_type_e g_dis_type = DIS_TYPE_HD;
static dis_layer_e g_dis_layer = DIS_LAYER_MAIN;
static bool g_audio_smooth_switch = false;
static bool g_audio_rate_play = false;

extern int play_h264_es(int argc , char *argv[]);
extern int h264_es_play(int argc , char *argv[]);
extern int h264_es_stop(int argc , char *argv[]);
static int enter_es_play(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	return 0;
}

//////////////////////////////////////////////////////////////
//following apis is for transfering project mode to ration and flip
#if 1

typedef enum{
	MPTEST_PROJECT_REAR = 0,
	MPTEST_PROJECT_CEILING_REAR,
	MPTEST_PROJECT_FRONT,
	MPTEST_PROJECT_CEILING_FRONT,

	MPTEST_PROJECT_MODE_MAX
}mptest_project_mode_e;

typedef struct{
    uint16_t screen_init_rotate;
    uint16_t screen_init_h_flip;
    uint16_t screen_init_v_flip;
}mp_rotate_cfg_t;

static mp_rotate_cfg_t mp_rotate_info[2];

#ifdef __linux__
static void mptest_dts_string_get(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);
    if(fd >= 0){
        read(fd, string, size);
        close(fd);
    }
    //printf("dts string: %s\n", string);
}

static int mptest_dts_uint32_get(const char *path)
{
    int fd = open(path, O_RDONLY);
    int value = -1;
    if(fd >= 0){
        uint8_t buf[4];
        if(read(fd, buf, 4) != 4){
            close(fd);
            return value;
        }
        close(fd);
        value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
    }
    printf("fd:%d ,dts value: %x\n", fd,value);
    return value;
}
#endif

static void mptest_screen_rotate_info(void)
{
	static int init_flag = 0;
	if (init_flag)
		return;

    unsigned int rotate=0,h_flip=0,v_flip=0;
#ifdef __HCRTOS__
    int np;

    memset(mp_rotate_info, 0, sizeof(mp_rotate_info));
    np = fdt_node_probe_by_path("/hcrtos/rotate");
    if(np>=0)
    {
        fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
        fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
        fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
        mp_rotate_info[0].screen_init_rotate = rotate;
        mp_rotate_info[0].screen_init_h_flip = h_flip;
        mp_rotate_info[0].screen_init_v_flip = v_flip;
    }

    np = fdt_node_probe_by_path("/hcrtos/rotate_4k");
    if(np>=0)
    {
        fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
        fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
        fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
        mp_rotate_info[1].screen_init_rotate = rotate;
        mp_rotate_info[1].screen_init_h_flip = h_flip;
        mp_rotate_info[1].screen_init_v_flip = v_flip;
    }

#else
#define ROTATE_CONFIG_PATH "/proc/device-tree/hcrtos/rotate"
#define ROTATE_4K_CONFIG_PATH "/proc/device-tree/hcrtos/rotate_4k"
    char status[16] = {0};
    memset(mp_rotate_info, 0, sizeof(mp_rotate_info));

    mptest_dts_string_get(ROTATE_CONFIG_PATH "/status", status, sizeof(status));
    if(!strcmp(status, "okay")){
        rotate = mptest_dts_uint32_get(ROTATE_CONFIG_PATH "/rotate");
        h_flip = mptest_dts_uint32_get(ROTATE_CONFIG_PATH "/h_flip");
        v_flip = mptest_dts_uint32_get(ROTATE_CONFIG_PATH "/v_flip");
        mp_rotate_info[0].screen_init_rotate = rotate;
        mp_rotate_info[0].screen_init_h_flip = h_flip;
        mp_rotate_info[0].screen_init_v_flip = v_flip;
    }

    mptest_dts_string_get(ROTATE_4K_CONFIG_PATH "/status", status, sizeof(status));
    if(!strcmp(status, "okay")){
        rotate = mptest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/rotate");
        h_flip = mptest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/h_flip");
        v_flip = mptest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/v_flip");
        mp_rotate_info[1].screen_init_rotate = rotate;
        mp_rotate_info[1].screen_init_h_flip = h_flip;
        mp_rotate_info[1].screen_init_v_flip = v_flip;
    }

#endif
    init_flag = 1;
    printf("%s()->>> 2k init_rotate = %u h_flip %u v_flip = %u\n", __func__,
        mp_rotate_info[0].screen_init_rotate,
        mp_rotate_info[0].screen_init_h_flip,
        mp_rotate_info[0].screen_init_v_flip);
    printf("%s()->>> 4k init_rotate = %u h_flip %u v_flip = %u\n", __func__, 
        mp_rotate_info[1].screen_init_rotate,
        mp_rotate_info[1].screen_init_h_flip,
        mp_rotate_info[1].screen_init_v_flip);

}


static uint16_t mptest_get_screen_init_rotate(int dis_tpye)
{
    if (DIS_TYPE_HD == dis_tpye)
        return mp_rotate_info[0].screen_init_rotate;
    else
        return mp_rotate_info[1].screen_init_rotate;
}

static uint16_t mptest_get_screen_init_h_flip(int dis_tpye)
{
    if (DIS_TYPE_HD == dis_tpye)
        return mp_rotate_info[0].screen_init_h_flip;
    else
        return mp_rotate_info[1].screen_init_h_flip;
}

static uint16_t mptest_get_screen_init_v_flip(int dis_tpye)
{
    if (DIS_TYPE_HD == dis_tpye)
        return mp_rotate_info[0].screen_init_v_flip;
    else
        return mp_rotate_info[1].screen_init_v_flip;
}

static  void mptest_get_rotate_by_flip_mode(mptest_project_mode_e mode,
                             int *p_rotate_mode ,
                             int *p_h_flip ,
                             int *p_v_flip)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    
    switch(mode)
    {
        case MPTEST_PROJECT_CEILING_REAR:
        {
            //printf("180\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 0;
            v_flip = 0;
            break;
        }

        case  MPTEST_PROJECT_FRONT:
        {
            //printf("h_mirror\n");
            rotate = ROTATE_TYPE_0;
            h_flip = 1;
            v_flip = 0;
            break;
        }

        case MPTEST_PROJECT_CEILING_FRONT:
        {
           // printf("v_mirror\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 1;
            v_flip = 0;
            break;

        }
        case MPTEST_PROJECT_REAR:
        {
            //printf("normal\n");
            rotate = ROTATE_TYPE_0;
            h_flip = 0;
            v_flip = 0;
            break;
        }
        default:
            break;
    }
#ifdef LCD_ROTATE_SUPPORT
    *p_rotate_mode = ROTATE_TYPE_0;
    *p_h_flip = 0;
    *p_v_flip = 0;
#else
    *p_rotate_mode = rotate;
    *p_h_flip = h_flip;
    *p_v_flip = v_flip;
#endif
}

static void mptest_transfer_rotate_mode_for_screen(
                                        int init_rotate,
                                        int init_h_flip,
                                        int init_v_flip,
                                        int *p_rotate_mode ,
                                        int *p_h_flip ,
                                        int *p_v_flip,
                                        int *p_fbdev_rotate)
{
    int fbdev_rotate[4] = { 0,270,180,90 }; //setting is anticlockwise for fvdev
    int rotate = 0 , h_flip = 0 , v_flip = 0;
	(void)init_v_flip;

    rotate = *p_rotate_mode;

    //if screen is V screen,h_flip and v_flip exchange
    if(init_rotate == 0 || init_rotate == 180)
    {
        h_flip = *p_h_flip;
        v_flip = *p_v_flip;
    }
    else
    {
        h_flip = *p_v_flip;
        v_flip = *p_h_flip;
    }
 
    /*setting in dts is anticlockwise */
    /*calc rotate mode*/
    if(init_rotate == 270)
    {
        rotate = (rotate + 1) & 3;
    }
    else if(init_rotate == 90)
    {
        rotate = (rotate + 3) & 3;
    }
    else if(init_rotate == 180)
    {
        rotate = (rotate + 2) & 3;
    }

    /*transfer v_flip to h_flip with rotate
    *rotate 0 + H
    *rotate 0 + V--> rotate 180 +H
    *rotate 180 + H
    *rotate 180 + V --> rotate 0  + H 
    *rotate 90 + H
    *rotate 90 + V--> rotate 270 +H
    *rotate 270 +H
    *rotate 270 +V--> rotate 90 + H 
    */
    if(v_flip == 1)
    {
        switch(rotate)
        {
            case ROTATE_TYPE_0:
                rotate = ROTATE_TYPE_180;
                break;
            case ROTATE_TYPE_90:
                rotate = ROTATE_TYPE_270;
                break;
            case ROTATE_TYPE_180:
                rotate = ROTATE_TYPE_0;
                break;
            case ROTATE_TYPE_270:
                rotate = ROTATE_TYPE_90;
                break;
            default:
                break;
        }
        v_flip = 0;
        h_flip = 1;
    }

    h_flip = h_flip ^ init_h_flip;

    if(p_rotate_mode != NULL)
    {
        *p_rotate_mode = rotate;
    }
    
    if(p_h_flip != NULL)
    {
        *p_h_flip = h_flip;
    }
    
    if(p_v_flip != NULL)
    {
        *p_v_flip = 0;
    }
    
    if(p_fbdev_rotate !=  NULL)
    {
        *p_fbdev_rotate = fbdev_rotate[rotate];
    }

}

static int mptest_get_flip_info(mptest_project_mode_e project_mode, int dis_type, int *rotate_type, int *flip_type)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;

    mptest_screen_rotate_info();
    int init_rotate = mptest_get_screen_init_rotate(dis_type);
    int init_h_flip = mptest_get_screen_init_h_flip(dis_type);
    int init_v_flip = mptest_get_screen_init_v_flip(dis_type);

    mptest_get_rotate_by_flip_mode(project_mode,
                            &rotate ,
                            &h_flip ,
                            &v_flip);

    mptest_transfer_rotate_mode_for_screen(
        init_rotate,init_h_flip,init_v_flip,
        &rotate , &h_flip , &v_flip , NULL);

    *rotate_type = rotate;
    *flip_type = h_flip;
    return 0;
}

static int mptest_rotate_convert(int rotate_init, int rotate_set)
{
    return ((rotate_init + rotate_set)%4);    
}

static int mptest_flip_convert(int dis_type, int flip_init, int flip_set)
{
    int rotate = 0;
    int swap;
    int flip_ret = flip_set;

    do {
        if (0 == flip_set){
            flip_ret = flip_init;
            break;
        }
        
        rotate = mptest_get_screen_init_rotate(dis_type);
        if (rotate == 90 || rotate == 270){
            swap = 1;
        } else {
            swap = 0;
        }

        if (swap) {
            if (1 == flip_set) {//horizon
                flip_ret = 2;
            }
            else if (2 == flip_set) {//vertical
                flip_ret = 1;
            }
        }
    } while(0);

    return flip_ret;
}

//////////////////////////////////////////////////////////////
#endif


static int pic_backup(void)
{
	int fd;
	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	if (g_smooth_mode) {
		ioctl(fd ,DIS_BACKUP_MP , DIS_TYPE_HD);
	}

	close(fd);
	return 0;
}

static int set_i2si_volume(uint8_t volume)
{
	int snd_fd = -1;

	snd_fd = open("/dev/sndC0i2si", O_WRONLY);
	if (snd_fd < 0) {
		printf ("open i2si %d failed\n", snd_fd);
		return -1;
	}

	ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
	volume = 0;
	ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &g_volume);
	printf("current i2si volume is %d\n", g_volume);

	close(snd_fd);
	return 0;
}

static int set_i2so_volume(uint8_t volume)
{
	int snd_fd = -1;

	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("open i2so %d failed\n", snd_fd);
		return -1;
	}

	ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
	volume = 0;
	ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &g_volume);
	printf("current i2so volume is %d\n", g_volume);

	close(snd_fd);
	return 0;
}

static int set_i2so_mute(bool mute)
{
	int snd_fd = -1;

	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	if (mute) {
		uint8_t volume = 0;
		ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
	}

	if (!mute) {
		ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &g_volume);
		printf("current volume is %d\n", g_volume);
	}

	close(snd_fd);

	printf("mute is %d\n", mute);
	return 0;
}

static int set_i2so_gpio_mute(bool mute)
{
	int snd_fd = -1;

	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	ioctl(snd_fd, SND_IOCTL_SET_MUTE, mute);

	close(snd_fd);

	printf("mute is %d\n", mute);
	return 0;
}

static int set_hdmi_mute(bool mute)
{
	int snd_fd = -1;

	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	ioctl(snd_fd, SND_IOCTL_SET_HDMI_MUTE, mute);

	close(snd_fd);

	printf("mute is %d\n", mute);
	return 0;
}

static inline int find_player_from_list_by_mediaplayer_handle (
	void *a, void *b, void *user_data)
{
	(void)user_data;
	return ((mediaplayer *)(a) != b);
}

static inline int find_player_from_list_by_uri (
	void *a, void *b, void *user_data)
{
	(void)user_data;
	return (strcmp(((mediaplayer *)a)->uri, b));
}

static inline int find_player_from_list_by_name (
	void *a, void *b, void *user_data)
{
	(void)user_data;
	return (strcmp(((mediaplayer *)a)->username, b));
}

#ifdef __linux__
 static int draw_subtitle(struct HCGeDrawSubtitle *ge_info, int size)
{
	char *buf_next;

	if(init_fb_device(ge_info) != 0) {
		av_log(NULL, AV_LOG_ERROR, "Init framebuffer error.\n");
		return -1;
	}

	memcpy((void *)ge_info->bg_picture, (void *)ge_info->tar_buf, size);
	buf_next = (char *)ge_info->screen_buffer[0];
	draw_background((uint8_t *)buf_next, ge_info);

	return 0;
}

 static void subtitle_callback(HCPlayerCallbackType type,
	void *user_data, void *val)
{
	HCSubtitle *sub;
	unsigned i = 0;
	int size;
	(void)user_data;

	sub = (HCSubtitle *)val;

	if (type == HCPLAYER_CALLBACK_SUBTITLE_OFF) {
		if(ge_draw_subtitle.format == 0) {
			ge_stop_draw_subtitle(&ge_draw_subtitle);
		} else {
			av_log(NULL, AV_LOG_INFO, "HCPLAYER_CALLBACK_SUBTITLE_OFF: sub format %d\n",
				ge_draw_subtitle.format);
		}
	}

	if (type == HCPLAYER_CALLBACK_SUBTITLE_ON) {
		ge_draw_subtitle.format = sub->format;
		if (sub->format == 0) {
			/*AV_PIX_FMT_PAL8 ----> AV_PIX_FMT_ARGB*/
			for(i = 0;i < sub->num_rects; i++) {
				ge_draw_subtitle.x = sub->rects[i]->x;
				ge_draw_subtitle.y = sub->rects[i]->y;
				ge_draw_subtitle.w = sub->rects[i]->w;
				ge_draw_subtitle.h = sub->rects[i]->h;
				size = sub->rects[i]->linesize[0] * sub->rects[i]->h;
				if ((int)ge_draw_subtitle.tar_size < size) {
					ge_draw_subtitle.tar_size = sub->rects[i]->linesize[0] * sub->rects[i]->h;
					if (!ge_draw_subtitle.tar_buf) {
						ge_draw_subtitle.tar_buf = malloc(ge_draw_subtitle.tar_size);
					} else {
						void *tmp = realloc(ge_draw_subtitle.tar_buf, ge_draw_subtitle.tar_size);
						if (tmp) {
							ge_draw_subtitle.tar_buf = tmp;
						} else {
							free(ge_draw_subtitle.tar_buf);
							ge_draw_subtitle.tar_buf = NULL;
						}
					}
				}
				if (ge_draw_subtitle.tar_buf) {
					av_log(NULL, AV_LOG_VERBOSE, "w %d, h %d, linesize %d, tar_size %d\n",
						sub->rects[i]->w, sub->rects[i]->h, sub->rects[i]->linesize[0], size);
					memcpy(ge_draw_subtitle.tar_buf, sub->rects[i]->data[0], size);
					draw_subtitle(&ge_draw_subtitle, size);
				} else {
					ge_draw_subtitle.tar_size = 0;
				}
			}
		} else {
			for(i = 0;i < sub->num_rects; i++) {
				if (sub->rects[i]->type == HC_SUBTITLE_TEXT) {
					av_log(NULL, AV_LOG_VERBOSE, "```%s```\n", sub->rects[i]->text);
				} else if (sub->rects[i]->type == HC_SUBTITLE_ASS) {
					av_log(NULL, AV_LOG_VERBOSE, "```%s```\n", sub->rects[i]->ass);
				}
			}
		}
	}
}
#endif

static int play_uri2(char *uri)
{
	HCPlayerInitArgs init_args = {0};
	if (!g_mp) {
		return -1;
	}

	init_args.uri = uri;
	init_args.mix_priority = g_mix_priority;
	init_args.mix_maximum_weight = g_mix_priority;
	init_args.slave_mode = g_slave_mode;
	init_args.snd_devs = g_snd_devs;
	init_args.sync_type = g_sync_mode2;
	init_args.user_data = g_mp;
	init_args.start_time = g_time_ms;
	init_args.buffering_enable = g_buffering_enable;
	init_args.play_attached_file = 0;
	init_args.msg_id = (int)g_msgid;
	init_args.img_dis_mode = g_img_dis_mode;

    g_mp->dis_type = g_dis_type;
    g_mp->dis_layer = g_dis_layer;
#if 1
	//get rotate and flip by project mode and dts, and overlay
	//the -r -m setting for rotation and filp.
	int rotate_type;
	int flip_type;
	int rotate;
    int flip;

	mptest_get_flip_info(g_project_mode, g_dis_type, &rotate_type, &flip_type);
	rotate = mptest_rotate_convert(rotate_type, g_rotate_type);
    flip = mptest_flip_convert(g_dis_type, flip_type, g_mirror_type);
    init_args.rotate_type = rotate % 4;
	init_args.mirror_type = flip;
#else
	init_args.mirror_type = g_mirror_type;
	init_args.rotate_type = g_rotate_type % 4;
#endif

	init_args.callback = NULL;
	init_args.disable_audio = g_disable_audio;
	init_args.disable_video = 1;//g_disable_video;
	if (g_en_subtitle) {
#ifdef __linux__
		init_args.callback = subtitle_callback;
#endif
	}
	//if (g_rotate_type || g_mirror_type) 
	{
		init_args.rotate_enable = 1;
	}
	init_args.audio_flush_thres = g_audio_flush_thres;
	init_args.bypass = g_bypass;

	init_args.img_dis_hold_time = g_pic_show_duration;
	init_args.gif_dis_interval = g_gif_interval;
	init_args.img_alpha_mode = g_pic_bg;
    init_args.dis_layer = g_dis_layer;
    init_args.pbp_mode = g_pbp_mode;
    init_args.dis_type = g_dis_type;


	init_args.decryption_key = decryption_key;

	if (g_img_effect.mode != IMG_SHOW_NULL) {
		memcpy(&init_args.img_effect, &g_img_effect, sizeof(image_effect_t));
	}

	if (g_vtranscode.b_enable) {
		memcpy(&init_args.transcode_config, &g_vtranscode, sizeof(struct video_transcode_config));
	}

	if (g_preview_enable) {
		g_preview_enable = 0;
		init_args.preview_enable = 1;
		memcpy(&init_args.src_area, &g_dis_rect.src_rect, sizeof(struct av_area));
		memcpy(&init_args.dst_area, &g_dis_rect.dst_rect, sizeof(struct av_area));
	} else {
		init_args.preview_enable = 0;
	}

	g_mp->player = hcplayer_create(&init_args);
	if (!g_mp->player) {
		return -1;
	}

	g_mp->uri = strdup(uri);

	g_plist2 = glist_append(g_plist2, g_mp);
	hcplayer_play(g_mp->player);

	g_mp = malloc(sizeof(mediaplayer));
	if (!g_mp) {
		printf("malloc g_mp err\n");
	}
	memset(g_mp, 0, sizeof(mediaplayer));

	return 0;
}

static int play_uri(char *uri, char* username)
{
	HCPlayerInitArgs init_args = {0};
	if (!g_mp) {
		return -1;
	}

	init_args.uri = uri;
	init_args.mix_priority = g_mix_priority;
	init_args.mix_maximum_weight = g_mix_priority;
	init_args.slave_mode = g_slave_mode;
	init_args.snd_devs = g_snd_devs;
	init_args.sync_type = g_sync_mode;
	init_args.user_data = g_mp;
	init_args.start_time = g_time_ms;
	init_args.buffering_enable = g_buffering_enable;
	init_args.play_attached_file = 1;
	init_args.msg_id = (int)g_msgid;
	init_args.img_dis_mode = g_img_dis_mode;

    g_mp->dis_type = g_dis_type;
    g_mp->dis_layer = g_dis_layer;

#if 1
	//get rotate and flip by project mode and dts, and overlay
	//the -r -m setting for rotation and filp.
	int rotate_type;
	int flip_type;
	int rotate;
    int flip;
    mptest_get_flip_info(g_project_mode, g_dis_type, &rotate_type, &flip_type);
	rotate = mptest_rotate_convert(rotate_type, g_rotate_type);
    flip = mptest_flip_convert(g_dis_type, flip_type, g_mirror_type);
    init_args.rotate_type = rotate % 4;
    init_args.mirror_type = flip;
#else
	init_args.mirror_type = g_mirror_type;
	init_args.rotate_type = g_rotate_type % 4;
#endif
	init_args.callback = NULL;
	init_args.disable_audio = g_disable_audio;
	init_args.disable_video = g_disable_video;
    init_args.b_aux_layer = g_b_aux_layer;
    init_args.dis_layer = g_dis_layer;
    init_args.pbp_mode = g_pbp_mode;
    init_args.dis_type = g_dis_type;
	init_args.enable_audio_rate_play = g_audio_rate_play;
	init_args.switch_audio_smooth = g_audio_smooth_switch;

	if (g_en_subtitle) {
#ifdef __linux__
		init_args.callback = subtitle_callback;
#endif
	}
	//if (g_rotate_type || g_mirror_type) 
	{
		init_args.rotate_enable = 1;
	}
	init_args.audio_flush_thres = g_audio_flush_thres;
	init_args.bypass = g_bypass;

	init_args.img_dis_hold_time = g_pic_show_duration;
	init_args.gif_dis_interval = g_gif_interval;
	init_args.img_alpha_mode = g_pic_bg;

	init_args.decryption_key = decryption_key;

	if (g_img_effect.mode != IMG_SHOW_NULL) {
		memcpy(&init_args.img_effect, &g_img_effect, sizeof(image_effect_t));
	}

	if (g_vtranscode.b_enable) {
		memcpy(&init_args.transcode_config, &g_vtranscode, sizeof(struct video_transcode_config));
	}

	if (g_preview_enable) {
		init_args.preview_enable = 1;
		memcpy(&init_args.src_area, &g_dis_rect.src_rect, sizeof(struct av_area));
		memcpy(&init_args.dst_area, &g_dis_rect.dst_rect, sizeof(struct av_area));
	} else {
		init_args.preview_enable = 0;
	}

	g_mp->player = hcplayer_create(&init_args);
	if (!g_mp->player) {
		return -1;
	}

	g_mp->uri = strdup(uri);
	if (username)
		g_mp->username = strdup(username);
	else
		g_mp->username = strdup(uri);

	g_plist = glist_append(g_plist, g_mp);
	hcplayer_play(g_mp->player);

	g_mp = malloc(sizeof(mediaplayer));
	if (!g_mp) {
		printf("malloc g_mp err\n");
	}
	memset(g_mp, 0, sizeof(mediaplayer));

	return 0;
}

static void play_next_uri(void)
{
	printf("play_next_uri\n");
	if (url_list) {
		char *uri;

		g_cur_ply_idx++;
		if (g_cur_ply_idx >= (int)glist_length(url_list)) {
			g_cur_ply_idx = 0;
		}

		uri = (char *)glist_nth_data(url_list, g_cur_ply_idx);
		if (uri) {
			printf("uri %s sync_type %d\n", uri,g_sync_mode);
			play_uri(uri, NULL);
		}
	}
}

static void play_next_uri2(void)
{
	printf("play_next_uri\n");
	if (url_list2) {
		char *uri;

		g_cur_ply_idx2++;
		if (g_cur_ply_idx2 >= (int)glist_length(url_list2)) {
			g_cur_ply_idx2 = 0;
		}

		uri = (char *)glist_nth_data(url_list2, g_cur_ply_idx2);
		if (uri) {
			printf("uri %s sync_type %d\n", uri, g_sync_mode2);
			play_uri2(uri);
		}
	}
}

static void play_prev_uri(void)
{
	printf("play_prev_uri\n");

	if (url_list) {
		char *uri;

		g_cur_ply_idx--;
		if (g_cur_ply_idx < 0) {
			g_cur_ply_idx = glist_length(url_list) - 1;
		}

		uri = (char *)glist_nth_data(url_list, g_cur_ply_idx);
		if (uri) {
			printf("uri %s sync_type %d\n", uri, g_sync_mode);
			play_uri(uri, NULL);
		}
	}
}

static void play_prev_uri2(void)
{
	printf("play_prev_uri\n");

	if (url_list2) {
		char *uri;

		g_cur_ply_idx2--;
		if (g_cur_ply_idx2 < 0) {
			g_cur_ply_idx2 = glist_length(url_list2) - 1;
		}

		uri = (char *)glist_nth_data(url_list2, g_cur_ply_idx2);
		if (uri) {
			printf("uri %s sync_type %d\n", uri, g_sync_mode2);
			play_uri2(uri);
		}
	}
}

static void free_mp(mediaplayer* mp)
{
	if (mp->player) {
		hcplayer_stop2 (mp->player, g_closevp, g_fillblack);
		pic_backup();
		mp->player = NULL;
	}
	if (mp->uri) {
		free(mp->uri);
		mp->uri = NULL;
	}
	if (mp->username){
		free(mp->username);
		mp->username = NULL;
	}
	free(mp);
}

static void *msg_recv_thread(void *arg)
{
	HCPlayerMsg msg;
	glist *list = NULL;
	mediaplayer *mp = NULL;
	(void)arg;
	int nth_list = 0;

	while(!g_mpabort) {
#ifdef __linux__
		if (msgrcv(g_msgid, (void *)&msg, sizeof(HCPlayerMsg) - sizeof(msg.type), 0, 0) == -1)
#else
		if (xQueueReceive((QueueHandle_t)g_msgid, (void *)&msg, -1) != pdPASS)
#endif
		{
			if (!g_mpabort) {
				printf("msg_recv_thread err\n");
				usleep(5000);
			}
			continue;
		}

		if (g_mpabort) {
			break;
		}

		if (g_mp2 && msg.type == HCPLAYER_MSG_STATE_EOS) {
			hcplayer_multi_destroy(g_mp2);
			g_mp2 = NULL;
			continue;
		}

		pthread_mutex_lock(&g_mutex);

		mp = msg.user_data;
		list = glist_find_custom(g_plist, mp,
			find_player_from_list_by_mediaplayer_handle);
		if (!list) {
			list = glist_find_custom(g_plist2, mp,
				find_player_from_list_by_mediaplayer_handle);
			nth_list = 1;
		} else {
			nth_list = 0;
		}
		if (!list) {
			usleep(1000);
			pthread_mutex_unlock(&g_mutex);
			continue;
		}

		if (msg.type == HCPLAYER_MSG_STATE_EOS)
		{
			if (g_multi_ins) {
				printf("multi mode: loop the same file.\n");
				hcplayer_seek(mp->player, 0);
			} else {
				printf ("app get eos\n");
				free_mp(mp);
				if (nth_list == 0) {
					g_plist = glist_delete_link(g_plist, list);
					play_next_uri();
				} else {
					g_plist2 = glist_delete_link(g_plist2, list);
					play_next_uri2();
				}
			}
		} else if (msg.type == HCPLAYER_MSG_STATE_TRICK_BOS) {
			printf ("app get trick bos\n");
			if (mp->player) {
				hcplayer_resume(mp->player);
			}
		} else if (msg.type == HCPLAYER_MSG_STATE_TRICK_EOS) {
			printf ("app get trick eos\n");
			free_mp(mp);
			if (nth_list == 0) {
				g_plist = glist_delete_link(g_plist, list);
				play_next_uri();
			} else {
				g_plist2 = glist_delete_link(g_plist2, list);
				play_next_uri2();
			}
		} else if (msg.type == HCPLAYER_MSG_OPEN_FILE_FAILED
			|| msg.type == HCPLAYER_MSG_UNSUPPORT_FORMAT
			|| msg.type == HCPLAYER_MSG_ERR_UNDEFINED) {
			printf ("err happend, stop it\n");
			free_mp(mp);
			if (nth_list == 0) {
				g_plist = glist_delete_link(g_plist, list);
			} else {
				g_plist2 = glist_delete_link(g_plist2, list);
			}

			if (nth_list == 0) {
				list = glist_nth(url_list, g_cur_ply_idx);
				url_list = glist_delete_link(url_list, list);
				g_cur_ply_idx--;
				play_next_uri();
			} else {
				list = glist_nth(url_list2, g_cur_ply_idx2);
				url_list2 = glist_delete_link(url_list2, list);
				g_cur_ply_idx2--;
				play_next_uri2();
			}

		} else if (msg.type == HCPLAYER_MSG_BUFFERING) {
			printf("buffering %d\n", msg.val);
		} else if (msg.type == HCPLAYER_MSG_STATE_PLAYING) {
			printf("player playing\n");
		} else if (msg.type == HCPLAYER_MSG_STATE_PAUSED) {
			printf("player paused\n");
		} else if (msg.type == HCPLAYER_MSG_STATE_READY) {
			printf("player ready\n");
		} else if (msg.type == HCPLAYER_MSG_READ_TIMEOUT) {
			printf("player read timeout\n");
		} else if (msg.type == HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO) {
			printf("no audio track or no supported audio track\n");
		} else if (msg.type == HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO) {
			printf("no video track or no supported video track\n");
		} else if (msg.type == HCPLAYER_MSG_UNSUPPORT_VIDEO_TYPE) {
			HCPlayerVideoInfo video_info;
			char *video_type = "unknow";
			if (!hcplayer_get_nth_video_stream_info (mp->player, msg.val, &video_info)) {
				/* only a simple sample, app developers use a static struct to mapping them. */
				if (video_info.codec_id == HC_AVCODEC_ID_HEVC) {
					video_type = "h265";
				} else if (video_info.codec_id == HC_AVCODEC_ID_VP9) {
					video_type = "vp9";
				} else if (video_info.codec_id == HC_AVCODEC_ID_AMV) {
					video_type = "amv";
				}
			}
			printf("unsupport video type %s, codec id %d\n", video_type, video_info.codec_id);
		} else if (msg.type == HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE) {
			HCPlayerAudioInfo audio_info;
			char *audio_type = "unknow";
			if (!hcplayer_get_nth_audio_stream_info (mp->player, msg.val, &audio_info)) {
				/* only a simple sample, app developers use a static struct to mapping them. */
				if (audio_info.codec_id < 0x11000) {
					audio_type = "pcm";
				} else if (audio_info.codec_id < 0x12000) {
					audio_type = "adpcm";
				} else if (audio_info.codec_id == HC_AVCODEC_ID_DTS) {
					audio_type = "dts";
				} else if (audio_info.codec_id == HC_AVCODEC_ID_EAC3) {
					audio_type = "eac3";
				} else if (audio_info.codec_id == HC_AVCODEC_ID_APE) {
					audio_type = "ape";
				}
			}
			printf("unsupport audio type %s\n", audio_type);
		} else if (msg.type == HCPLAYER_MSG_AUDIO_DECODE_ERR) {
			printf("audio dec err, audio idx %d\n", msg.val);
			/* check if it is the last audio track, if not, then change to next one. */
			if (mp->player) {
				int total_audio_num = -1;
				total_audio_num = hcplayer_get_audio_streams_count(mp->player);
				if (msg.val >= 0 && total_audio_num > (msg.val + 1)) {
					HCPlayerAudioInfo audio_info;
					if (!hcplayer_get_cur_audio_stream_info(mp->player, &audio_info)) {
						if (audio_info.index == msg.val) {
							int idx = audio_info.index + 1;
							while (hcplayer_change_audio_track(mp->player, idx)) {
								idx++;
								if (idx >= total_audio_num) {
									break;
								}
							}
						}
					}
				} else {
					hcplayer_change_audio_track(mp->player, -1);
				}
			}
		} else if (msg.type == HCPLAYER_MSG_VIDEO_DECODE_ERR) {
			printf("video dec err, video idx %d\n", msg.val);
			/* check if it is the last video track, if not, then change to next one. */
			if (mp->player) {
				int total_video_num = -1;
				total_video_num = hcplayer_get_video_streams_count(mp->player);
				if (msg.val >= 0 && total_video_num > (msg.val + 1)) {
					HCPlayerVideoInfo video_info;
					if (!hcplayer_get_cur_video_stream_info(mp->player, &video_info)) {
						if (video_info.index == msg.val) {
							int idx = video_info.index + 1;
							while (hcplayer_change_video_track(mp->player, idx)) {
								idx++;
								if (idx >= total_video_num) {
									break;
								}
							}
						}
					}
				} else {
					hcplayer_change_video_track(mp->player, -1);
				}
			}
		}  else if (msg.type == HCPLAYER_MSG_FIRST_VIDEO_FRAME_TRANSCODED) {
			printf("first video frame transcoded!\n");
			AvPktHd hdr = { 0 };
			if (hcplayer_read_transcoded_picture(mp->player, &hdr, sizeof(AvPktHd))
				!= sizeof(AvPktHd)) {
				printf("read header err\n");
			} else {
				void *data = malloc(hdr.size);
				if (data) {
					if (hcplayer_read_transcoded_picture(mp->player , data , hdr.size) != hdr.size) {
						printf ("read data err\n");
					} else {
						printf("get a picture, size %d\n", hdr.size);
						if (g_vtranscode_path) {
							FILE *rec = fopen(g_vtranscode_path, "wb");
							if (rec) {
								fwrite(data, hdr.size, 1, rec);
								fclose(rec);
								printf("write pic success\n");
							} else {
								printf("write pic failed\n");
							}
						}
					}
					free(data);
				} else {
					printf("no memory");
				}
			}
		} else {
			printf("unknow msg %d\n", (int)msg.type);
		}

		pthread_mutex_unlock(&g_mutex);
	}

	return NULL;
}

#ifndef __linux__
static FILE *avfile = NULL;
static int hcread(void *buf, int size, void *file)
{
	FILE *file_to_read = (FILE *)file;
	fread(buf, 1, size, file_to_read);
}

static int showlogo(int argc, char *argv[])
{
	if (argc < 2) {
		return -1;
	}

	avfile = fopen(argv[1], "rb");

	if (!avfile) {
		return -1;
	}

	return start_show_logo((void *)avfile, hcread);
}

static int stop_showlogo(int argc, char *argv[])
{
	(void)argc;
	(void)(**argv);
	if (avfile) {
		stop_show_logo();
		fclose(avfile);
		avfile = NULL;
	}

	return 0;
}

static int wait_showlogo(int argc, char *argv[])
{
	(void)argc;
	(void)(**argv);
	if (avfile) {
		wait_show_logo();
		fclose(avfile);
		avfile = NULL;
	}

	return 0;
}
#endif

static int mp_stop2(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc <= 1) {
		while (g_plist2) {
			list = glist_last(g_plist2);
			if (list) {
				mp = (mediaplayer *)list->data;
				free_mp(mp);
				g_plist2 = glist_delete_link(g_plist2, list);
			}
		}

		if (argc != 0) {
			while (url_list2) {
				if (url_list2->data)
					free(url_list2->data);
				url_list2 = glist_delete_link(url_list2, url_list2);
			}
		}
	} else if (argc > 1){
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_name);
		if (list) {
			mp = (mediaplayer *)list->data;
			free_mp(mp);
			g_plist2 = glist_delete_link(g_plist2, list);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_stop(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

#ifndef __linux__
	stop_showlogo(0, NULL);
#endif

	pthread_mutex_lock(&g_mutex);

	if (argc <= 1) {
		while (g_plist) {
			list = glist_last(g_plist);
			if (list) {
				mp = (mediaplayer *)list->data;
				free_mp(mp);
				g_plist = glist_delete_link(g_plist, list);
			}
		}

		if (argc != 0) {
			while (url_list) {
				if (url_list->data)
					free(url_list->data);
				url_list = glist_delete_link(url_list, url_list);
			}
		}
	} else if (argc > 1){
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_name);
		if (list) {
			mp = (mediaplayer *)list->data;
			free_mp(mp);
			g_plist = glist_delete_link(g_plist, list);
		}
	}

	if (g_mem_play_file) {
		fclose(g_mem_play_file);
		g_mem_play_file = NULL;
	}

#ifdef __linux__
	if (ge_draw_subtitle.fbdev > 0) {
		if (ge_draw_subtitle.ctx != NULL) {
			free(ge_draw_subtitle.ctx);
			ge_draw_subtitle.ctx = NULL;
		}
		if (ge_draw_subtitle.tar_buf != NULL) {
			free(ge_draw_subtitle.tar_buf);
			ge_draw_subtitle.tar_buf = NULL;
		}
		deinit_fb_device(&ge_draw_subtitle);
		memset(&ge_draw_subtitle, 0, sizeof(struct HCGeDrawSubtitle));
	}
#endif
	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_smooth(int argc, char *argv[])
{
	if (argc < 2)
		return -1;
	int distype = DIS_TYPE_HD;
	int fd;
	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0) {
		return -1;
	}

	g_smooth_mode = atoi(argv[1]);
	if (g_smooth_mode == 0) {
		g_closevp = 1;
		g_fillblack = 0;
		ioctl(fd ,DIS_FREE_BACKUP_MP , distype);
	} else {
		g_closevp = 0;
		g_fillblack = 0;
	}

	close(fd);
	return 0;

}
static int mp_play(int argc, char *argv[])
{
	int opt;
	char *uri = NULL;
	char *username = NULL;
	opterr = 0;
	optind = 0;

	if (argc < 2) {
		return -1;
	}

	uri = argv[1];
	username = argv[1];
	if (!uri)
		return -1;

	g_time_ms = 0;
	g_sync_mode = 2;
	g_mix_priority = 0;
	g_mix_maximum_weight = 0;
	g_slave_mode = 0;
	g_b_aux_layer = false;
    g_pbp_mode = 0;
    g_dis_type = DIS_TYPE_HD;
    g_dis_layer = DIS_LAYER_MAIN;

	while ((opt = getopt(argc-1, &argv[1], "s:b:d:t:r:m:e:i:c:f:v:a:p:o:x:z:y:l:w:u:")) != EOF) {
		switch (opt) {
		case 'c':
			g_slave_mode = atoi(optarg);
			printf("g_slave_mode %d\n", g_slave_mode);
			break;
		case 'b':
			g_buffering_enable = atoi(optarg);
			printf("buffering_enable %d\n", g_buffering_enable);
			break;
		case 'i':
			g_mix_priority = atoi(optarg);
			printf("mix_priority %d\n", g_mix_priority);
			break;
		case 'v':
			g_mix_maximum_weight = atoi(optarg);
			printf("mix_maximum_weight %d\n", g_mix_maximum_weight);
			break;
		case 't':
			g_time_ms = atof(optarg);
			printf("time_ms %f\n", g_time_ms);
			break;
		case 's':
			g_sync_mode = atoi(optarg);
			printf("uri_info.sync_type %d\n", g_sync_mode);
			break;
		case 'd':
			g_img_dis_mode = atoi(optarg);
			printf("img_dis_mode %d\n", g_img_dis_mode);
			break;
		case 'p':
			g_bypass = atoi(optarg);
			printf("bypass %d\n", g_bypass);
			break;
		case 'r':
			g_rotate_type = atoi(optarg);
			if (g_rotate_type > 4)
				g_rotate_type = 4;
			printf("rotate_type %d\n", g_rotate_type);
			break;
		case 'm':
			g_mirror_type = atoi(optarg);
			printf("mirror_type %d\n", g_mirror_type);
			break;
		case 'f':
		{
			g_project_mode = atoi(optarg);
			if (g_project_mode < 0 || g_project_mode > 3){
				printf("g_project_mode must be in 0 - 3!\n");
				return -1;
			}
			break;
		}
		case 'a':
			g_audio_flush_thres = atoi(optarg);
			printf("audio_flush_thres %d\n", g_audio_flush_thres);
			break;
		case 'e':
			g_en_subtitle = atoi(optarg);
			printf("en_subtitle %d\n", g_en_subtitle);
			break;
		case 'o':
			g_snd_devs = atoi(optarg);
			printf("g_snd_devs %ld\n", g_snd_devs);
			break;
        case 'x':
            g_b_aux_layer = atoi(optarg);
            printf("g_b_aux_layer %d\n" , g_b_aux_layer);
            break;
        case 'z':
            g_pbp_mode = atoi(optarg);
            printf("g_pbp_mode %d\n" , g_pbp_mode);
			break;
        case 'y':
            if (atoi(optarg) == 0) {
                g_dis_type = DIS_TYPE_HD;
            } else {
                g_dis_type = DIS_TYPE_UHD;
            }
            break;
        case 'l':
            if (atoi(optarg) == 0) {
                g_dis_layer = DIS_LAYER_MAIN;
            }
            else{
                g_dis_layer = DIS_LAYER_AUXP;
                g_b_aux_layer = 1;
            }
            break;
		case 'w':
			g_preview_enable = 1;
			sscanf(optarg,"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",
				   &g_dis_rect.src_rect.x, &g_dis_rect.src_rect.y,
				   &g_dis_rect.src_rect.w, &g_dis_rect.src_rect.h,
				   &g_dis_rect.dst_rect.x, &g_dis_rect.dst_rect.y,
				   &g_dis_rect.dst_rect.w, &g_dis_rect.dst_rect.h);
			break;
		case 'u':
			username = optarg;
			printf("user name: %s\n", username);
			break;
		default:
			break;
		}
	}

	pthread_mutex_lock(&g_mutex);
	while (url_list) {
		if (url_list->data)
			free(url_list->data);
		url_list = glist_delete_link(url_list, url_list);
	}
	pthread_mutex_unlock(&g_mutex);

	if (!g_multi_ins) {
		mp_stop(0, NULL);
	}

	pthread_mutex_lock(&g_mutex);

	if (g_time_ms >= 1) {
		g_time_ms *= 1000;
	}
	play_uri(uri, username);
	if (g_loop_play) {
		url_list = glist_append(url_list, strdup(uri));
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int memory_read(void * opaque, uint8_t *buf, int bufsize)
{
	int fd, ret;
	fd = fileno((FILE *)opaque);
	ret = read(fd, buf, bufsize);
	if (ret == 0) {
		return AVERROR_EOF;
	}

	return (ret == -1) ? AVERROR(errno) : ret;
}

static int64_t memory_seek(void *opaque, int64_t offset, int whence)
{
	int64_t ret;
	int fd = fileno((FILE *)opaque);

#ifndef AVSEEK_SIZE
#define AVSEEK_SIZE 0x10000
#endif

	if (whence == AVSEEK_SIZE) {
		/**
		* ORing this as the "whence" parameter to a seek function causes it to
		* return the filesize without seeking anywhere. Supporting this is optional.
		* If it is not supported then the seek function will return 0.
		*/
		struct stat st;
		if(fstat(fd, &st))
			return 0;
		else
			return st.st_size;
	}

	ret = lseek(fd, offset, whence);

	return ret < 0 ? AVERROR(errno) : ret;
}

static int play_uri_memory(char *uri)
{
	HCPlayerInitArgs init_args = {0};

	if (!g_mp) {
		return -1;
	}

	g_mem_play_file = fopen(uri, "r");
	if (!g_mem_play_file) {
		return -1;
	}

	//printf("g_mem_play_file %p\n", g_mem_play_file);
	init_args.readdata_opaque = g_mem_play_file;
	init_args.readdata_callback = memory_read;
	init_args.seekdata_callback = memory_seek;
	init_args.user_data = g_mp;
	init_args.play_attached_file = 1;
	init_args.msg_id = (int)g_msgid;
	init_args.sync_type = g_sync_mode;

	g_mp->player = hcplayer_create(&init_args);
	if (!g_mp->player) {
		return -1;
	}

	g_plist = glist_append(g_plist, g_mp);
	hcplayer_play(g_mp->player);

	g_mp = malloc(sizeof(mediaplayer));
	if (!g_mp) {
		printf("malloc g_mp err\n");
	}
	memset(g_mp, 0, sizeof(mediaplayer));

	return 0;
}

static int mp_memory_play(int argc, char *argv[])
{
	char *uri;
	int ret = 0;

	if (argc < 2)
		return -1;

	uri = argv[1];
	if (!uri)
		return -1;

	pthread_mutex_lock(&g_mutex);
	while (url_list) {
		if (url_list->data)
			free(url_list->data);
		url_list = glist_delete_link(url_list, url_list);
	}
	pthread_mutex_unlock(&g_mutex);

	if (!g_multi_ins) {
		mp_stop(0, NULL);
	}

	pthread_mutex_lock(&g_mutex);
	ret = play_uri_memory(uri);
	pthread_mutex_unlock(&g_mutex);

	return ret;
}

static int mp_pause(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc == 1) {
		list = g_plist;
		while (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_pause(mp->player);
			list = glist_next(list);
		}
	} else {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_name);
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_pause(mp->player);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_resume(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc == 1) {
		list = g_plist;
		while (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_resume(mp->player);
			list = glist_next(list);
		}
	} else {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_name);;
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_resume(mp->player);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}


static int mp_seek(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc >= 3) {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_name);
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_seek(mp->player, atoi(argv[2]) * 1000);
		}
	} else if (glist_length(g_plist) == 1 && argc == 2) {
		mp = glist_first(g_plist)->data;
		hcplayer_seek(mp->player, atoi(argv[1]) * 1000);
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static void print_media_info(mediaplayer *mp)
{
	int64_t filesize;
	HCPlayerAudioInfo audio_info = {0};
	HCPlayerVideoInfo video_info = {0};
	HCPlayerSubtitleInfo subtitle_info = {0};
	HCPlayerMediaInfo media_info = {0};

	if (mp->uri) {
		printf("uri: %s:\n", mp->uri);
	}
	if (mp->username) {
		printf("username: %s:\n", mp->username);
	}

	if (mp->player) {
		filesize = hcplayer_get_filesize(mp->player);
		printf ("filesize "LONG_INT_FORMAT"\n", filesize);

		printf ("number of audio tracks %d\n",
			hcplayer_get_audio_streams_count(mp->player));
		printf ("number of video tracks %d\n",
			hcplayer_get_video_streams_count(mp->player));
		printf ("number of subtitle tracks %d\n",
			hcplayer_get_subtitle_streams_count(mp->player));
		printf ("\n");

		hcplayer_get_cur_audio_stream_info(mp->player, &audio_info);
		printf ("audio info:\n");
		printf ("index:		 %d\n", audio_info.index);
		printf ("codec_id:		%d\n", audio_info.codec_id);
		printf ("lang_code:	 %s\n", audio_info.lang_code);
		printf ("channels:		%d\n", audio_info.channels);
		printf ("samplerate:	%d\n", audio_info.sample_rate);
		printf ("depth:		 %d\n", audio_info.depth);
		printf ("bitrate:		"LONG_INT_FORMAT"\n", audio_info.bit_rate);
		printf ("\n");

		hcplayer_get_cur_video_stream_info(mp->player, &video_info);
		printf ("video info:\n");
		printf ("index:		 %d\n", video_info.index);
		printf ("codec_id:		%d\n", video_info.codec_id);
		printf ("lang_code:	 %s\n", video_info.lang_code);
		printf ("width:		 %d\n", video_info.width);
		printf ("height:		%d\n", video_info.height);
		printf ("frame_rate:	%f\n", video_info.frame_rate);
		printf ("bitrate:		"LONG_INT_FORMAT"\n", video_info.bit_rate);
		printf ("\n");

		hcplayer_get_cur_subtitle_stream_info(mp->player, &subtitle_info);
		printf ("subtitle info:\n");
		printf ("index:		 %d\n", subtitle_info.index);
		printf ("codec_id:		%d\n", subtitle_info.codec_id);
		printf ("lang_code:	 %s\n", subtitle_info.lang_code);
		printf ("\n");

		hcplayer_get_media_info(mp->player, &media_info);
		printf ("media info:\n");
		if (media_info.artist)
			printf ("artist:		%s\n", media_info.artist);
		if (media_info.album)
			printf ("album:		%s\n", media_info.album);
		if (media_info.title)
			printf ("title:		%s\n", media_info.title);
		if (media_info.TYER)
			printf ("TYER:		%s\n", media_info.TYER);
		if (media_info.datetime)
			printf ("datetime:	%s\n", media_info.datetime);
		if (media_info.orientation)
			printf ("orientation:	%s\n", media_info.orientation);
		if (media_info.gpslatitude)
			printf ("GPS:	%s\n", media_info.gpslatitude);
		if (media_info.make)
			printf ("make:	%s\n", media_info.make);

		printf ("\nbitrate:		"LONG_INT_FORMAT"\n", media_info.bit_rate);
	}
}

static int mp_info(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc > 1) {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_name);
		if (list) {
			mp = (mediaplayer *)list->data;
			print_media_info(mp);
		}
	} else {
		list = g_plist;
		while (list) {
			print_media_info((mediaplayer *)list->data);
			list = glist_next(list);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_time(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;
	int64_t position = 0;
	int64_t duration = 0;

	pthread_mutex_lock(&g_mutex);

	if (argc > 1) {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_name);
		if (list) {
			mp = (mediaplayer *)list->data;
			position = hcplayer_get_position(mp->player);
			duration = hcplayer_get_duration(mp->player);
			printf("uri: %s:\n", mp->uri);
			printf("curtime/duration %lld ms/%lld ms\n",
				position, duration);
		}
	} else {
		list = g_plist;
		while (list) {
			mp = (mediaplayer *)list->data;
			position = hcplayer_get_position(mp->player);
			duration = hcplayer_get_duration(mp->player);
			//printf("uri: %s:\n", mp->uri);
			printf("\033[1A");
			fflush(stdout);
			printf("\033[K");
			fflush(stdout);
			printf("pos/dur %8lld.%03llds/%8lld.%03llds\n",
				position/1000, position%1000, duration/1000, duration%1000);
			fflush(stdout);
			list = glist_next(list);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_rate(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc >= 3) {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_name);
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_set_speed_rate(mp->player, atof(argv[2]));
		}
	} else if (glist_length(g_plist) == 1 && argc == 2) {
		mp = glist_first(g_plist)->data;
		hcplayer_set_speed_rate(mp->player, atof(argv[1]));
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_log(int argc, char *argv[])
{
	if (argc < 2) {
		return -1;
	}

	hcplayer_change_log_level(atoi(argv[1]));

	return 0;
}

static void mp_preview_deinit(void)
{
	g_preview_enable = 0;
	g_dis_rect.src_rect.x = 0;
	g_dis_rect.src_rect.y = 0;
	g_dis_rect.src_rect.w = 1920;
	g_dis_rect.src_rect.h = 1080;
	g_dis_rect.dst_rect.x = 0;
	g_dis_rect.dst_rect.y = 0;
	g_dis_rect.dst_rect.w = 1920;
	g_dis_rect.dst_rect.h = 1080;
}

static int mp_deinit(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	g_mpabort = 1;
	g_mp_info = 0;

	mp_stop(0, NULL);

	mp_preview_deinit();

	hcplayer_deinit();

	if (msg_rcv_thread_id){
		HCPlayerMsg msg;
		msg.type = HCPLAYER_MSG_UNDEFINED;
#ifdef __linux__
		if (g_msgid >= 0) {
			msgsnd(g_msgid, (void *)&msg, sizeof(HCPlayerMsg) - sizeof(msg.type), 0);
		}
#else
		if (g_msgid) {
			xQueueSendToBack((QueueHandle_t)g_msgid, &msg, 0);
		}
#endif
		pthread_join(msg_rcv_thread_id, NULL);
		msg_rcv_thread_id = 0;
	}

#ifdef __linux__
	if (g_msgid >= 0) {
		msgctl(g_msgid,IPC_RMID,NULL);
		g_msgid = -1;
	}
#else
	if (g_msgid) {
		vQueueDelete(g_msgid);
		g_msgid = NULL;
	}
#endif

	if (get_info_thread_id){
		pthread_join(get_info_thread_id, NULL);
		get_info_thread_id = 0;
	}

	if (g_mp) {
		free(g_mp);
		g_mp = NULL;
	}

	if (g_vtranscode_path) {
		free(g_vtranscode_path);
	}

#ifndef __linux__
	*((uint32_t *)0xb8808300) |= 0x1;
#endif

	return 0;
}

int mp_set_dis_onoff(bool on_off)
{
    int fd = -1;
    struct dis_win_onoff winon = { 0 };
    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0)
    {
        return -1;
    }
    winon.distype = DIS_TYPE_HD;
    winon.layer = DIS_LAYER_MAIN;
    winon.on = on_off ? 1 : 0;
    ioctl(fd , DIS_SET_WIN_ONOFF , &winon);
    close(fd);
    return 0;
}

static int mp_debug(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	ffplayer_debug();
	return 0;
}

static int mp_init(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (!g_mp) {
		g_mp = malloc(sizeof(mediaplayer));
		if (!g_mp) {
			return -1;
		}
		memset(g_mp, 0, sizeof(mediaplayer));
		set_i2so_volume(40);
#ifndef BR2_PACKAGE_VIDEO_PBP_EXAMPLES
		mp_set_dis_onoff(false);
#endif		
	}

#ifdef __linux__
	if (g_msgid < 0) {
		g_msgid = msgget(MKTAG('h','c','p','l'), 0666 | IPC_CREAT);
		if (g_msgid < 0) {
			printf ("create msg queue failed\n");
			mp_deinit(0, NULL);
			return -1;
		}
	}
#else
	if (!g_msgid) {
		g_msgid = xQueueCreate(( UBaseType_t )configPLAYER_QUEUE_LENGTH,
			sizeof(HCPlayerMsg));
		if (!g_msgid) {
			printf ("create msg queue failed\n");
			mp_deinit(0, NULL);
			return -1;
		}
	}
#endif

	g_multi_ins = 0;

	g_mpabort = 0;

	if (!msg_rcv_thread_id)
	{
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 0x2000);
		if(pthread_create(&msg_rcv_thread_id, &attr, msg_recv_thread, NULL)) {
			mp_deinit(0, NULL);
			return -1;
		}
	}

	hcplayer_init(LOG_WARNING);

#ifndef __linux__
#ifndef BR2_PACKAGE_VIDEO_PBP_EXAMPLES
	*((uint32_t *)0xb8808300) &= 0xfffffffe;
#endif	
	//console_run_cmd("nsh mw 0xb8808300=0x00040000 4");
#endif
	return 0;
}

static int mp_multi(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	g_loop_play =0;
	g_multi_ins = true;
	printf("enter multi-instance mode\n");
	return 0;
}

static int mp_single(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	g_loop_play = 1;
	g_multi_ins = false;
	printf("exit multi-instance mode\n");
	return 0;
}

static int mp_pic_mode(int argc, char *argv[])
{
	int opt;
	opterr = 0;
	optind = 0;

	while ((opt = getopt(argc, &argv[0], "e:i:d:b:")) != EOF) {
		switch (opt) {
		case 'i':
			g_gif_interval = atoi(optarg);
			printf("g_gif_interval %d\n", g_gif_interval);
			break;
		case 'd':
			g_pic_show_duration = atoi(optarg);
			printf("g_pic_show_duration %d\n", g_pic_show_duration);
			break;
		case 'b':
			g_pic_bg = atoi(optarg);
			printf("g_pic_bg %d\n", g_pic_bg);
			break;

		default:
			printf("pic_mode -i 100 -d 3000 -b 1\n"
				"-i int,ms,gif frame interval\n"
				"-d int,ms,pic stay time\n"
				"-b alpha blend mode\n");
			break;
		}
	}

	return 0;
}

static int mp_pic_effect(int argc, char *argv[])
{
	if (argc < 4) {
		printf("need 4 args\n");
		return -1;
	}

	g_img_effect.mode = atoi(argv[1]);
	if (g_img_effect.mode >= IMG_SHOW_MODE_MAX) {
		return -1;
	}
	switch (g_img_effect.mode) {
		case IMG_SHOW_NULL:
		case IMG_SHOW_NORMAL:
			printf("no effect.\n");
			break;
		case IMG_SHOW_SHUTTERS:
			g_img_effect.mode_param.shuttles_param.direction = atoi(argv[2]);
			g_img_effect.mode_param.shuttles_param.type = atoi(argv[3]);
			g_img_effect.mode_param.shuttles_param.time = atoi(argv[4]);
			printf("effect: IMG_SHOW_SHUTTERS\n");
			break;
		case IMG_SHOW_BRUSH:
			g_img_effect.mode_param.brush_param.direction = atoi(argv[2]);
			g_img_effect.mode_param.brush_param.type = atoi(argv[3]);
			g_img_effect.mode_param.brush_param.time = atoi(argv[4]);
			printf("effect: IMG_SHOW_BRUSH\n");
			break;
		case IMG_SHOW_SLIDE:
		case IMG_SHOW_SLIDE2:
			g_img_effect.mode_param.slide_param.direction = atoi(argv[2]);
			g_img_effect.mode_param.slide_param.type = atoi(argv[3]);
			g_img_effect.mode_param.slide_param.time = atoi(argv[4]);
			printf("effect: IMG_SHOW_SLIDE\n");
			break;
		case IMG_SHOW_RANDOM:
			g_img_effect.mode_param.random_param.type = atoi(argv[2]);
			g_img_effect.mode_param.random_param.time = atoi(argv[3]);
			printf("effect: IMG_SHOW_RANDOM\n");
			break;
		case IMG_SHOW_FADE:
			g_img_effect.mode_param.fade_param.type = atoi(argv[2]);
			g_img_effect.mode_param.fade_param.time = atoi(argv[3]);
			printf("effect: IMG_SHOW_FADE\n");
			break;
		default:
			printf("pleast enter the correct effect mode\n");
			break;
	}

	return 0;
}

static int mp_pic_effect_enable(int argc, char *argv[])
{
	int opt;
    opterr = 0;
    optind = 0;

	int vidsink_fd;
	int enable;

	struct vframe_display_info dis_info = {0};

	if (argc >= 2) {
		vidsink_fd = open("/dev/vidsink", O_WRONLY);
		if (vidsink_fd < 0) {
			return -1;
		}

		while ((opt = getopt(argc, &argv[0], "l:y:")) != EOF) {
			switch (opt) {
			case 'l':
				if (atoi(optarg) == 1) {
					dis_info.dis_layer = DIS_LAYER_AUXP;
				} else {
					dis_info.dis_layer = DIS_LAYER_MAIN;
				}
				break;
			case 'y':
				if(atoi(optarg))
					dis_info.dis_type = DIS_TYPE_UHD;
				else
				 	dis_info.dis_type = DIS_TYPE_HD;
				break;
			default:
				break;
			}
		}

		enable = atoi(argv[1]);
		if (enable) {
			ioctl(vidsink_fd, VIDSINK_ENABLE_IMG_EFFECT, 0);
		} else {
			printf("layer=%d type=%d\n",dis_info.dis_layer,dis_info.dis_type);
			ioctl(vidsink_fd, VIDSINK_DISABLE_IMG_EFFECT2, &dis_info);
		}

		close(vidsink_fd);

		return 0;
	}

	return -1;
}

static int scan_dir(char *path)
{
	int ret = 0;
	DIR *dirp;
	struct dirent *entry;
	char item_path[512];
	char *uri;
	int uri_len;
	int len = strlen(path);

	//printf("scan %s\n", path);
	if ((dirp = opendir(path)) == NULL) {
		ret = -1;
		return ret;
	}

	while (1) {
		entry = readdir(dirp);
		if (!entry)
			break;
		//printf("entry->d_name %s, entry->d_type %d\n", entry->d_name, entry->d_type);

		if(entry->d_name[0] == '.'){
			continue;
		}

		if (SCAN_SUB_DIR && entry->d_type == 4){//S_ISDIR(entry->d_type)){
			memset(item_path, 0, 512);
			strcpy(item_path, path);
			if (item_path[len - 1] != '/') {
				item_path[len] = '/';
			}
			strcpy(item_path + strlen(item_path), entry->d_name);
			scan_dir(item_path);
		} else if (entry->d_type != 4) {
			uri_len = len + strlen(entry->d_name) + 1;
			if(path[len - 1] != '/'){
				uri_len++;
			}

			uri = malloc(uri_len);
			if(!uri){
				ret = 0;
				goto end;
			}
			memset(uri, 0, uri_len);
			strcpy(uri, path);
			if (uri[len - 1] != '/') {
				uri[len] = '/';
			}
			strcpy(uri + strlen(uri), entry->d_name);
			printf("add %s\n", uri);
			url_list = glist_append(url_list, (void *)uri);
			if (glist_length(url_list) >= MAX_SCAN_FILE_LIST_LEN) {
				break;
			}
		}
	}

end:
	closedir(dirp);

	return ret;
}

static int mp_scan(int argc, char *argv[])
{
	DIR *dirp = NULL;

	if (argc < 2) {
		return -1;
	}

	printf("try open dir %s\n", argv[1]);
	dirp = opendir(argv[1]);
	if (argc >= 3) {
		g_sync_mode = atoi(argv[2]);
	}

	if (dirp) {
		g_multi_ins = false;
		mp_stop(0, NULL);
		closedir(dirp);

		pthread_mutex_lock(&g_mutex);

		while (url_list) {
			if (url_list->data)
				free(url_list->data);
			url_list = glist_delete_link(url_list, url_list);
		}

		scan_dir(argv[1]);
		if (url_list) {
			char *uri = NULL;

			g_cur_ply_idx = 0;
			uri = (char *)glist_nth_data(url_list, g_cur_ply_idx);
			if (uri) {
				printf("uri %s sync_type %d\n",uri, g_sync_mode);
				play_uri(uri, NULL);
			}
		}

		printf("scan done, url_list len %d\n", (int)glist_length(url_list));
		pthread_mutex_unlock(&g_mutex);
	} else {
		printf("open dir %s failed\n", argv[1]);
	}

	return 0;
}

static int scan_dir2(char *path)
{
	int ret = 0;
	DIR *dirp;
	struct dirent *entry;
	char item_path[512];
	char *uri;
	int uri_len;
	int len = strlen(path);

	//printf("scan %s\n", path);
	if ((dirp = opendir(path)) == NULL) {
		ret = -1;
		return ret;
	}

	while (1) {
		entry = readdir(dirp);
		if (!entry)
			break;
		//printf("entry->d_name %s, entry->d_type %d\n", entry->d_name, entry->d_type);

		if(entry->d_name[0] == '.'){
			continue;
		}

		if (SCAN_SUB_DIR && entry->d_type == 4){//S_ISDIR(entry->d_type)){
			memset(item_path, 0, 512);
			strcpy(item_path, path);
			if (item_path[len - 1] != '/') {
				item_path[len] = '/';
			}
			strcpy(item_path + strlen(item_path), entry->d_name);
			scan_dir(item_path);
		} else if (entry->d_type != 4) {
			uri_len = len + strlen(entry->d_name) + 1;
			if(path[len - 1] != '/'){
				uri_len++;
			}

			uri = malloc(uri_len);
			if(!uri){
				ret = 0;
				goto end;
			}
			memset(uri, 0, uri_len);
			strcpy(uri, path);
			if (uri[len - 1] != '/') {
				uri[len] = '/';
			}
			strcpy(uri + strlen(uri), entry->d_name);
			printf("add %s\n", uri);
			url_list2 = glist_append(url_list2, (void *)uri);
			if (glist_length(url_list2) >= MAX_SCAN_FILE_LIST_LEN) {
				break;
			}
		}
	}

end:
	closedir(dirp);

	return ret;
}

static int mp_scan2(int argc, char *argv[])
{
	DIR *dirp = NULL;

	if (argc < 2) {
		return -1;
	}

	printf("try open dir %s\n", argv[1]);
	dirp = opendir(argv[1]);
	if (argc >= 3) {
		g_sync_mode2 = atoi(argv[2]);
	}

	if (dirp) {
		mp_stop2(0, NULL);
		closedir(dirp);

		pthread_mutex_lock(&g_mutex);

		while (url_list2) {
			if (url_list2->data)
				free(url_list2->data);
			url_list2 = glist_delete_link(url_list2, url_list2);
		}

		scan_dir2(argv[1]);
		if (url_list2) {
			char *uri = NULL;

			g_cur_ply_idx2 = 0;
			uri = (char *)glist_nth_data(url_list2, g_cur_ply_idx2);
			if (uri) {
				printf("uri %s sync_type %d\n",uri, g_sync_mode2);
				play_uri2(uri);
			}
		}

		printf("scan done, url_list len %d\n", (int)glist_length(url_list2));
		pthread_mutex_unlock(&g_mutex);
	} else {
		printf("open dir %s failed\n", argv[1]);
	}

	return 0;
}

static int mp_next(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	mp_stop(0, NULL);

	pthread_mutex_lock(&g_mutex);
	play_next_uri();
	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_next2(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	mp_stop2(0, NULL);

	pthread_mutex_lock(&g_mutex);
	play_next_uri2();
	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_prev(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	mp_stop(0, NULL);

	pthread_mutex_lock(&g_mutex);
	play_prev_uri();
	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_prev2(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	mp_stop2(0, NULL);

	pthread_mutex_lock(&g_mutex);
	play_prev_uri2();
	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_volume(int argc, char *argv[])
{
	uint8_t volume = 0;

	if (argc < 2) {
		return -1;
	}

	volume = atoi(argv[1]);
	set_i2so_volume(volume);
	set_i2si_volume(volume);
	return 0;
}

static int mp_i2so_mute(int argc, char *argv[])
{
	bool mute = 0;

	if (argc < 2) {
		return -1;
	}

	mute = atoi(argv[1]);

	return set_i2so_mute(mute);
}

static int mp_i2so_gpio_mute(int argc, char *argv[])
{
	bool mute = 0;

	if (argc < 2) {
		return -1;
	}

	mute = atoi(argv[1]);

	return set_i2so_gpio_mute(mute);
}

static int mp_hdmi_mute(int argc, char *argv[])
{
	bool mute = 0;

	if (argc < 2) {
		return -1;
	}

	mute = atoi(argv[1]);

	return set_hdmi_mute(mute);
}


static int mp_set_twotone(int argc, char *argv[])
{
	int opt;
	int snd_fd = -1;
	opterr = 0;
	optind = 0;
	struct snd_twotone tt = {0};
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("twotone open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	while ((opt = getopt(argc, &argv[0], "b:t:o:m:")) != EOF) {
		switch (opt) {
		case 'b':
			tt.bass_index = atoi(optarg);
			printf("twotone bass_index %d\n", tt.bass_index);
			break;
		case 't':
			tt.treble_index = atoi(optarg);
			printf("twotone treble_index %d\n", tt.treble_index);
			break;
		case 'o':
			tt.onoff = atoi(optarg);
			printf("twotone onoff %d\n", tt.onoff);
			break;
		case 'm':
			tt.tt_mode = atoi(optarg);
			printf("twotone mode %d\n", tt.tt_mode);
			break;

		default:
			break;
		}
	}

	ioctl(snd_fd, SND_IOCTL_SET_TWOTONE, &tt);
	close(snd_fd);
	return 0;
}

static int mp_set_lr_balance(int argc, char *argv[])
{
	int opt;
	int snd_fd = -1;
	opterr = 0;
	optind = 0;
	struct snd_lr_balance lr = {0};
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("lr_balance open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	while ((opt = getopt(argc, &argv[0], "i:o:")) != EOF) {
		switch (opt) {
		case 'i':
			lr.lr_balance_index = atoi(optarg);
			printf("lr balance index %d\n", lr.lr_balance_index);
			break;
		case 'o':
			lr.onoff = atoi(optarg);
			printf("lr balance onoff %d\n", lr.onoff);
			break;
		default:
			break;
		}
	}
	ioctl(snd_fd, SND_IOCTL_SET_LR_BALANCE, &lr);
	close(snd_fd);
	return 0;
}

static int mp_set_eq_enable(int argc, char *argv[])
{
	int snd_fd = -1;
	int en = 0;
	opterr = 0;
	optind = 0;
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("eq_enable open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	if (argc != 2) {
		printf("eq_enable <0 | 1>");
		return -1;
	}

	en = atoi(argv[1]);

	ioctl(snd_fd, SND_IOCTL_SET_EQ_ONOFF, !!en);
	close(snd_fd);
	return 0;
}

static int mp_set_eq(int argc, char *argv[])
{
	int opt;
    opterr = 0;
    optind = 0;
    
	int snd_fd = -1;
	struct snd_eq_band_setting setting = { 0 };


	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("lr_balance open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	setting.band = -1;

	while ((opt = getopt(argc, &argv[0], "b:c:q:g:")) != EOF) {
		switch (opt) {
		case 'b':
			setting.band = atoi(optarg);
			break;
		case 'c':
			setting.cutoff = atoi(optarg);
			break;
		case 'q':
			setting.q = atoi(optarg);
			break;
		case 'g':
			setting.gain = atoi(optarg);
			break;
		default:
			printf("eq -b <band> -c <cutoff> -q <q> -g <gain>");
			return -1;
		}
	}

	ioctl(snd_fd, SND_IOCTL_SET_EQ_BAND, &setting);
	close(snd_fd);

	return 0;
}

static int mp_set_pbe(int argc, char *argv[])
{
	int snd_fd = -1;
	int val = 0;
	opterr = 0;
	optind = 0;
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("eq_enable open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	if (argc != 2) {
		printf("pbe <0-100>");
		return -1;
	}

	val = atoi(argv[1]);

	ioctl(snd_fd, SND_IOCTL_SET_PBE, val);
	close(snd_fd);
	return 0;
}

static int mp_set_pbe_precut(int argc, char *argv[])
{
	int snd_fd = -1;
	int val = 0;
	opterr = 0;
	optind = 0;
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("eq_enable open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	if (argc != 2) {
		printf("pbe_precut <-45 ~ 0>");
		return -1;
	}

	val = atoi(argv[1]);

	ioctl(snd_fd, SND_IOCTL_SET_PBE_PRECUT, val);
	close(snd_fd);
	return 0;
}

static int mp_set_audio_eq6(int argc, char *argv[])
{
	int opt;
    opterr = 0;
    optind = 0;
	int snd_fd = -1;
	struct snd_audio_eq6 eq6 = {0};
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("audio eq6 open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	while ((opt = getopt(argc, &argv[0], "o:m:")) != EOF) {
		switch (opt) {
		case 'o':
			eq6.onoff = atoi(optarg);
			printf("audio eq6 onoff %d\n", eq6.onoff);
			break;
		case 'm':
			eq6.mode = atoi(optarg);
			printf("audio eq6 mode %d\n", eq6.mode);
			break;
		default:
			break;
		}
	}
	ioctl(snd_fd, SND_IOCTL_SET_EQ6, &eq6);
	close(snd_fd);
	return 0;
}

static void *get_info_thread(void *arg)
{
	(void)arg;

	while(g_mp_info) {
		mp_time(0, 0);
		usleep(g_mp_info_interval*1000);
		//mp_info(0, 0);
		//usleep(g_mp_info_interval*1000);
	}
	return NULL;
}

static int mp_loop_info(int argc, char *argv[])
{
	if (argc > 1) {
		g_mp_info_interval = atoi(argv[1]);//ms
	}
	if (!get_info_thread_id) {
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 0x2000);
		g_mp_info = true;
		if(pthread_create(&get_info_thread_id, &attr, get_info_thread, NULL)) {
			mp_deinit(0, NULL);
			return -1;
		}
	}

	return 0;
}

static int mp_remove_loop_info(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	g_mp_info = false;
	pthread_join(get_info_thread_id, NULL);
	get_info_thread_id = 0;
	return 0;
}

static void *auto_test_thread(void *arg)
{
	struct timeval tv;
	(void)arg;

	while(!g_stop_auto_switch) {
		gettimeofday(&tv, NULL);
		int ms = ((uint32_t)tv.tv_usec/1000) % 40;
		usleep(ms * 100 * 1000);
		mp_next(0, NULL);
	}

	//auto_switch_thread_id = 0;
	//pthread_detach(pthread_self ());
	//pthread_exit(NULL);
	return NULL;
}

static int mp_auto_switch(int argc, char *argv[])
{
	if (argc < 2)
		return -1;

	if (atoi(argv[1]) == 1) {
		if (!auto_switch_thread_id) {
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setstacksize(&attr, 0x2000);
			g_stop_auto_switch = 0;
			if(pthread_create(&auto_switch_thread_id, &attr, auto_test_thread, NULL)) {
				mp_deinit(0, NULL);
				return -1;
			}
		}
	} else {
		g_stop_auto_switch = 1;
		if (auto_switch_thread_id) {
			pthread_join(auto_switch_thread_id, NULL);
		}
		auto_switch_thread_id = 0;
	}

	return 0;
}

static void *auto_seek_thread(void *arg)
{
	struct timeval tv;
	mediaplayer *mp = NULL;
	(void)arg;

	while(!g_stop_auto_seek) {
		gettimeofday(&tv, NULL);
		int ms = ((uint32_t)tv.tv_usec/1000) % 40;
		usleep(ms * 100 * 1000);
		pthread_mutex_lock(&g_mutex);
		if (glist_first(g_plist) && glist_first(g_plist)->data) {
			int64_t duration;
			mp = glist_first(g_plist)->data;
			duration = hcplayer_get_duration(mp->player);
			gettimeofday(&tv, NULL);
			ms = ((uint32_t)tv.tv_usec/1000);
			hcplayer_seek(mp->player, ms * (duration / 1000));
		} else {
			//printf("mp %p\n", mp);
		}
		pthread_mutex_unlock(&g_mutex);
	}

	//auto_seek_thread_id = 0;
	//pthread_detach(pthread_self ());
	//pthread_exit(NULL);
	return NULL;
}

static int mp_auto_seek(int argc, char *argv[])
{
	if (argc < 2) {
		return -1;
	}

	if (atoi(argv[1]) == 1) {
			if (!auto_seek_thread_id) {
				pthread_attr_t attr;
				pthread_attr_init(&attr);
				pthread_attr_setstacksize(&attr, 0x2000);
				g_stop_auto_seek = 0;
				if(pthread_create(&auto_seek_thread_id, &attr, auto_seek_thread, NULL)) {
					mp_deinit(0, NULL);
					return -1;
				}
			}
	} else {
			g_stop_auto_seek = 1;
			if (auto_seek_thread_id) {
				pthread_join(auto_seek_thread_id, NULL);
			}
			auto_seek_thread_id = 0;
	}

	return 0;
}

static void change_display_rect(void * player,
	struct vdec_dis_rect *old_rect, struct vdec_dis_rect *new_rect)
{
	int n;
	int times = 50;
	struct vdec_dis_rect rect;
	struct av_area *ns = &new_rect->src_rect;
	struct av_area *nd = &new_rect->dst_rect;
	struct av_area *os = &old_rect->src_rect;
	struct av_area *od = &old_rect->dst_rect;

	for(n = 0 ; n < times ; n++) {
		rect.src_rect.x = (int)os->x + ((int)ns->x - (int)os->x) * n / times;
		rect.src_rect.y = (int)os->y + ((int)ns->y - (int)os->y) * n / times;
		rect.src_rect.w = (int)os->w + ((int)ns->w - (int)os->w) * n / times;
		rect.src_rect.h = (int)os->h + ((int)ns->h - (int)os->h) * n / times;
		rect.dst_rect.x = (int)od->x + ((int)nd->x - (int)od->x) * n / times;
		rect.dst_rect.y = (int)od->y + ((int)nd->y - (int)od->y) * n / times;
		rect.dst_rect.w = (int)od->w + ((int)nd->w - (int)od->w) * n / times;
		rect.dst_rect.h = (int)od->h + ((int)nd->h - (int)od->h) * n / times;
		hcplayer_set_display_rect(player, &rect);
		usleep(20 * 1000);
	}

	hcplayer_set_display_rect(player, new_rect);
}

static int mp_preview(int argc, char *argv[])
{
	(void)argc;
	(void)(**argv);
	glist *list = NULL;
	struct vdec_dis_rect old_rect;
	struct vdec_dis_rect new_rect;
	int single_media = 1;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);


	list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_name);
	if (list) {
		mp = (mediaplayer *)list->data;
		single_media = 1;
	} else {
		single_media = 0;
	}
	memcpy(&old_rect, &g_dis_rect, sizeof(old_rect));

	if (single_media){
		if (sscanf(argv[2],"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",
			   &new_rect.src_rect.x, &new_rect.src_rect.y,
			   &new_rect.src_rect.w, &new_rect.src_rect.h,
			   &new_rect.dst_rect.x, &new_rect.dst_rect.y,
			   &new_rect.dst_rect.w, &new_rect.dst_rect.h) == 8){
			g_preview_enable = 1;
			memcpy(&g_dis_rect, &new_rect, sizeof(new_rect));
			if (mp->player) 
				change_display_rect(mp->player, &old_rect, &new_rect);
		} else {
			printf("invalid args\n");
		}
	} else {
		if (sscanf(argv[1],"%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",
			   &new_rect.src_rect.x, &new_rect.src_rect.y,
			   &new_rect.src_rect.w, &new_rect.src_rect.h,
			   &new_rect.dst_rect.x, &new_rect.dst_rect.y,
			   &new_rect.dst_rect.w, &new_rect.dst_rect.h) == 8){

			g_preview_enable = 1;
			memcpy(&g_dis_rect, &new_rect, sizeof(new_rect));
			list = g_plist;
			while (list) {
				mp = (mediaplayer *)list->data;
				if (mp->player) 
					change_display_rect(mp->player, &old_rect, &new_rect);
				
				list = list->next;
			}
		} else {
			printf("invalid args\n");
		}
	}


	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_rm_preview(int argc, char *argv[])
{
	int dis_fd = -1;
	vdec_dis_rect_t dis_rect = {{0, 0, 1920, 1080},{0, 0, 1920, 1080}};
	(void)argc;
	(void)argv;

	pthread_mutex_lock(&g_mutex);
	memcpy(&g_dis_rect, &dis_rect, sizeof(vdec_dis_rect_t));
	g_preview_enable = 0;
	dis_fd = open("/dev/dis" , O_WRONLY);
	if (dis_fd >= 0) {
		struct dis_zoom dz;
		dz.distype = DIS_TYPE_HD;
		dz.layer = DIS_LAYER_MAIN;
		memcpy(&dz.src_area, &g_dis_rect.src_rect, sizeof(struct av_area));
		memcpy(&dz.dst_area, &g_dis_rect.dst_rect, sizeof(struct av_area));
		ioctl(dis_fd, DIS_SET_ZOOM, &dz);
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_full_screen(int argc, char *argv[])
{
	vdec_dis_rect_t dis_rect = {{0, 0, 1920, 1080}, {0, 0, 1920, 1080}};
	mediaplayer *mp = NULL;
	(void)argc;
	(void)argv;

	pthread_mutex_lock(&g_mutex);

	memcpy(&g_dis_rect, &dis_rect, sizeof(vdec_dis_rect_t));
	if (g_plist && g_plist->data) {
		mp = (mediaplayer *)g_plist->data;
		change_display_rect(mp->player, &g_dis_rect, &dis_rect);
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_disable_av(int argc, char *argv[])
{
	int opt;
	opterr = 0;
	optind = 0;

	while ((opt = getopt(argc, &argv[0], "a:v:")) != EOF) {
		switch (opt) {
		case 'a':
			g_disable_audio = atoi(optarg);
			break;
		case 'v':
			g_disable_video = atoi(optarg);
			break;

		default:
			break;
		}
	}

	return 0;
}

static int mp_change_rotate(int argc, char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;

	glist *list = NULL;
	mediaplayer *mp = NULL;
	int argc_base = 0;
    int single_media = 1;
	int real_rotate;
	int real_flip;


	pthread_mutex_lock(&g_mutex);

	list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_name);
	if (list) {
		mp = (mediaplayer *)list->data;
		argc_base = 1;
		single_media = 1;
	} else {
		argc_base = 0;
		single_media = 0;
	}
    while ((opt = getopt(argc-argc_base , &argv[argc_base] , "r:m:f:")) != EOF) {
        switch (opt) {
            case 'r':
                g_rotate_type = atoi(optarg);
                break;
            case 'm':
                g_mirror_type = atoi(optarg);
                break;
            case 'f':
			{
				g_project_mode = atoi(optarg);
                g_project_mode = g_project_mode % 4;
				break;
			}
            default:
                break;
        }
    }


	//get rotate and flip by project mode and dts, and overlay
	//the -r -m setting for rotation and filp.
	int rotate_type;
	int flip_type;
	int rotate;

	if (1 == single_media){
        mptest_get_flip_info(g_project_mode, mp->dis_type, &rotate_type, &flip_type);
        rotate = mptest_rotate_convert(rotate_type, g_rotate_type);
        real_rotate = rotate % 4;
        real_flip = mptest_flip_convert(mp->dis_type, flip_type, g_mirror_type);
		hcplayer_change_rotate_mirror_type(mp->player, real_rotate, real_flip);		
	} else {
		list = g_plist;
		while (list) {
			mp = (mediaplayer *)list->data;
			if (mp->player) {
                mptest_get_flip_info(g_project_mode, mp->dis_type, &rotate_type, &flip_type);
                rotate = mptest_rotate_convert(rotate_type, g_rotate_type);
                real_rotate = rotate % 4;
                real_flip = mptest_flip_convert(mp->dis_type, flip_type, g_mirror_type);
				hcplayer_change_rotate_mirror_type(mp->player, real_rotate, real_flip);		
			}
			list = list->next;
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_stop_mode(int argc, char *argv[])
{
	if (argc < 3) {
		printf("please enter: stop_mode closevp(bool) fillblack(bool);\nfor example stop_mode 0 0\n");
		return -1;
	}

	g_closevp = atoi(argv[1]);
	g_fillblack = atoi(argv[2]);

	return 0;
}

static int mp_set_decryption_key(int argc, char *argv[])
{

	if (decryption_key)
		free(decryption_key);
	decryption_key = NULL;

	if (argc < 2) {
		printf("current decryption_key removed, if need set, please enter: decryption_key c7e16c4403654b85847037383f0c2db3\n");
		return 0;
	}

	decryption_key = strdup(argv[1]);

	return 0;
}

static int mp_set_audio_track(int argc, char *argv[])
{
	int index = 0;
	mediaplayer *mp = NULL;
	int ret = 0;
	if (argc < 2) {
		printf("please enter: atrack index\n");
		return -1;
	}

	index = atoi(argv[1]);

	mp = (mediaplayer *)g_plist->data;
	int total_audio_num = -1;
	total_audio_num = hcplayer_get_audio_streams_count(mp->player);
	if (index >= 0 && total_audio_num > index) {
		HCPlayerAudioInfo audio_info = {0};
		hcplayer_get_cur_audio_stream_info(mp->player, &audio_info);
		if(index != audio_info.index) {
			int count = 0;
			while ((ret = hcplayer_change_audio_track(mp->player, index)) != 0) {
				index++;
				count++;
				if (index >= total_audio_num) {
					index = 0;
				}
				if (count > total_audio_num) {
					break;
				}
			}
			if (ret != 0) {
				hcplayer_change_audio_track(mp->player, -1);
			}
		} else {
			return 0;
		}
	} else {
		hcplayer_change_audio_track(mp->player, -1);
	}


	return 0;
}

static int mp_set_video_track(int argc, char *argv[])
{
	int index = 0;
	int ret = 0;
	mediaplayer *mp = NULL;

	if (argc < 2) {
		printf("please enter: vtrack index\n");
		return -1;
	}

	index = atoi(argv[1]);
	mp = (mediaplayer *)g_plist->data;
	int total_video_num = -1;
	total_video_num = hcplayer_get_video_streams_count(mp->player);
	if (index >= 0 && total_video_num > index) {
		HCPlayerVideoInfo video_info = {0};
		hcplayer_get_cur_video_stream_info(mp->player, &video_info);
		if(index != video_info.index) {
			int count = 0;
			while ((ret = hcplayer_change_video_track(mp->player, index)) != 0) {
				index++;
				count++;
				if (index >= total_video_num) {
					index = 0;
				}
				if (count > total_video_num) {
					break;
				}
			}
			if (ret != 0) {
				hcplayer_change_video_track(mp->player, -1);
			}
		} else {
			return 0;
		}
	} else {
		hcplayer_change_video_track(mp->player, -1);
	}

	return 0;
}

static int mp_set_subtitle_track(int argc, char *argv[])
{
	int index = 0;
	mediaplayer *mp = NULL;
	int ret = 0;
	if (argc < 2) {
		printf("please enter: strack index\n");
		return -1;
	}

	index = atoi(argv[1]);

	mp = (mediaplayer *)g_plist->data;
	int total_subtitle_num = -1;
	total_subtitle_num = hcplayer_get_subtitle_streams_count(mp->player);
	if (index >= 0 && total_subtitle_num > index) {
		HCPlayerSubtitleInfo subtitle_info = {0};
		hcplayer_get_cur_subtitle_stream_info(mp->player, &subtitle_info);
		if(index != subtitle_info.index) {
			int count = 0;
			while ((ret = hcplayer_change_subtitle_track(mp->player, index)) != 0) {
				index++;
				count++;
				if (index >= total_subtitle_num) {
					index = 0;
				}
				if (count > total_subtitle_num) {
					break;
				}
			}
			if (ret != 0) {
				hcplayer_change_subtitle_track(mp->player, -1);
			}
		} else {
			return 0;
		}
	} else {
		hcplayer_change_subtitle_track(mp->player, -1);
	}

	return 0;
}

static int mp_set_video_transcode(int argc, char *argv[])
{
	if (argc < 2) {
		printf("transcode_set b_enable b_show b_capture_one b_scale scale_factor store_path\n"
			"for_example: transcode_set 1 0 0 0 0 /media/hdd/v.mjpg\n"
			"or 'transcode_set 0'\n");
		return -1;
	}

	g_vtranscode.b_enable = atoi(argv[1]);

	if (!g_vtranscode.b_enable) {
		printf ("transcode disabled\n");
		return 0;
	} else if (g_vtranscode.b_enable && argc < 7) {
		printf("transcode_set b_enable b_show b_capture_one b_scale scale_factor store_path\n"
			"for_example: transcode_set 1 0 0 0 0 /media/hdd/v.mjpg\n"
			"or 'transcode_set 0'\n");
		g_vtranscode.b_enable = 0;
		return -1;
	}

	g_vtranscode.b_show = atoi(argv[2]);
	g_vtranscode.b_capture_one = atoi(argv[3]);
	g_vtranscode.b_scale = atoi(argv[4]);
	g_vtranscode.scale_factor = atoi(argv[5]);
	if (g_vtranscode_path)
		free(g_vtranscode_path);
	g_vtranscode_path = strdup(argv[6]);

	return 0;
}

static int mp_play_two_uri(int argc, char *argv[])
{
	HCPlayerInitArgs audio_initargs = {0}, video_initargs = {0};

	if (argc < 3) {
		return -1;
	}

	if (g_mp2) {
		hcplayer_multi_destroy(g_mp2);
	}

	audio_initargs.uri = strdup(argv[1]);
	video_initargs.uri = strdup(argv[2]);

	audio_initargs.sync_type = video_initargs.sync_type = HCPLAYER_AUDIO_MASTER;
	audio_initargs.msg_id = video_initargs.msg_id = (int)g_msgid;

	g_mp2 = hcplayer_multi_create(&audio_initargs, &video_initargs);
	if (!g_mp2) {
		return -1;
	}

	hcplayer_multi_play(g_mp2);

	return 0;
}

static int mp_stop_two_uri(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (g_mp2) {
		hcplayer_multi_destroy(g_mp2);
		g_mp2 = NULL;
		return 0;
	}

	return -1;
}

static int mp_time_two_uri(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (g_mp2) {
		printf("pos/dur: %d, %d\n", hcplayer_multi_position(g_mp2),
			hcplayer_multi_duration(g_mp2));
		return 0;
	}

	return -1;
}

static int mp_pause_two_uri(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (g_mp2) {
		return hcplayer_multi_pause(g_mp2);
	}

	return -1;
}

static int mp_resume_two_uri(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	if (g_mp2) {
		return hcplayer_multi_play(g_mp2);
	}

	return -1;
}

static int mp_seek_two_uri(int argc, char *argv[])
{
	if (g_mp2 && argc > 1) {
		return hcplayer_multi_seek(g_mp2, atoi(argv[1]) * 1000);
	}

	return -1;
}

static int mp_audio_smooth_switch(int argc, char *argv[])
{
	if (argc >= 2) {
		g_audio_smooth_switch = atoi(argv[1]);
		return 0;
	}

	return -1;
}

static int mp_audio_rate_play(int argc, char *argv[])
{
	if (argc >= 2) {
		g_audio_rate_play = atoi(argv[1]);
		return 0;
	}

	return -1;
}

static int mp_audio_pitch_shift(int argc, char *argv[])
{
	if (!g_plist) {
		return -1;
	}
	if (argc >= 2) {
		mediaplayer *mp = g_plist->data;
		return hcplayer_set_audio_pitch_shift(mp->player, atoi(argv[1]));
	}

	return -1;
}

static int transcode(int argc, char *argv[])
{
	uint8_t *jpg_data = NULL;
	int jpg_size = 0;
	int ret;

	if (argc < 8) {
		printf("please enter: transcode /media/sda1/in.jpg /media/sda1/out.jpg width height starttime mode format auxp_layer memory_mode\n"
			"\tstarttime: float, 0~1 meam percent of the file duration; > 1 means seconds\n"
			"\tmode: 0,长宽保持比例，全部缩到边框内, 1,长宽裁剪到边框尺寸\n"
			"\tformat: 0,jpg，1,bgra，2,yuv420p，3,gray8\n"
			"\tauxp_layer: 0,diable pbp mode， 1,enable pbp and use main layer， 16,enable pbp and use auxp layer\n"
			"\tmemory_mode: 0,send file path to transcode function， 1,read file to a data addr, and send data addr to transcode function\n");
		return -1;
	} else {
		HCPlayerTranscodeArgs transcode_args = {0};

		if (argc >= 10 && atoi(argv[9])) {
			int fd;
			struct stat st;
			FILE *file = fopen(argv[1], "r");
			if (!file)
				return -1;
			fd = fileno(file);
			fstat(fd, &st);
			transcode_args.read_size = st.st_size;
			printf("memory mode, File size: %d bytes\n", transcode_args.read_size);
			transcode_args.read_data = malloc(transcode_args.read_size);
			if (!transcode_args.read_data) {
				fclose(file);
				transcode_args.read_size = 0;
				return -1;
			}
			fread(transcode_args.read_data, 1, transcode_args.read_size, file);
			fclose(file);
		} else {
			transcode_args.url = argv[1];
		}
		transcode_args.render_width = atoi(argv[3]);
		transcode_args.render_height = atoi(argv[4]);
		transcode_args.start_time = atof(argv[5]);
		transcode_args.transcode_mode = atoi(argv[6]);
		transcode_args.transcode_format = atoi(argv[7]);
		if (argc >= 9)
			transcode_args.dis_layer = atoi(argv[8]);
		ret = hcplayer_pic_transcode2(&transcode_args);
		if (ret < 0) {
			return ret;
		}

		if (transcode_args.read_data) {
			free(transcode_args.read_data);
			transcode_args.read_data = NULL;
		}

		jpg_data = transcode_args.out;
		jpg_size = transcode_args.out_size;
		printf("width %d, height %d\n", transcode_args.out_width, transcode_args.out_height);
	}

	if (jpg_data && jpg_size && argv[2]) {
		FILE *rec = fopen(argv[2], "wb");
		if (rec) {
			fwrite(jpg_data, jpg_size, 1, rec);
			fclose(rec);
			printf("write pic success\n");
		} else {
			printf("write pic failed\n");
		}
	}

	if (jpg_data) {
		free(jpg_data);
	}

	return 0;
}

#ifdef __linux__
static struct termios stored_settings;

static int dumpstack_cmd(int argc, char *argv[])
{
	long long val;
	unsigned long xHandle = 0;
	int fd;

	if (argc != 2)
		return 0;

	val = strtoll(argv[1], NULL, 16);
	xHandle = (unsigned long)val;

	fd = open("/dev/dumpstack", O_RDWR);
	if (fd < 0)
		return 0;

	ioctl(fd, DUMPSTACK_DUMP, xHandle);

	close(fd);

	return 0;
}

static int avp_cmd(int argc, char *argv[])
{
	char buf[512];
	int fd, ret, i;

	fd = open("/dev/virtuart", O_RDWR);
	if (fd < 0) {
		printf("open /dev/virtuart fail!\n");
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	for (i = 1; i < argc; i++) {
		strcat(buf, " ");
		strcat(buf, argv[i]);
	}

	strcat(buf, "\n");

	write(fd, buf, strlen(buf));
	usleep(5000);

	while ((ret = read(fd, buf, sizeof(buf))) > 0) {
		printf("%s", buf);
		memset(buf, 0, sizeof(buf));
	}
	printf("\n");

	close(fd);

	return 0;
}

static int mp_system(int argc, char *argv[])
{
	if (argc >= 2) {
		system(argv[1]);
	}
	return 0;
}

static void exit_console(int signo)
{
	(void)signo;
	mp_deinit(0, NULL);
	tcsetattr (0, TCSANOW, &stored_settings);
	exit(0);
}

int spdif_open(int rate, int channels, int bitdepth, int format)
{
	struct snd_pcm_params params = {0};
	int spo_fd;

	spo_fd = open("/dev/sndC0spo", O_WRONLY);
	if (spo_fd < 0) {
		printf("open spo failed\n");
		return spo_fd;
	}

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.rate = rate;//i2si 多少就填多少
	params.channels = channels;//i2si 多少就填多少
	params.bitdepth = bitdepth;//i2si 多少就填多少
	params.format = format;//i2si 多少就填多少
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = SND_PCM_ALIGN_RIGHT;
	params.period_size = 1536;
	params.periods = 16;
	params.start_threshold = 1;
	params.pcm_source = SND_SPO_SOURCE_I2SODMA;
	ioctl(spo_fd, SND_IOCTL_HW_PARAMS, &params);
	ioctl(spo_fd, SND_IOCTL_START, 0);
	printf("spo start\n");
	return spo_fd;
}


/* audio path: cvbs audio--> ADC --> i2si --> i2so -->DAC --> speaker
 *	i2si : L01;
 *   start i2si
 */
int i2si_fd = -1, i2so_fd = -1;
static int cvbs_audio_open(int argc, char *argv[])
{
	struct snd_pcm_params i2si_params = {0};
	int channels = 1;

	int rate = 44100;
	int periods = 16;
	int bitdepth = 16;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	int align = SND_PCM_ALIGN_RIGHT;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	int period_size = 1536;

	int read_size = period_size;
	int ret = 0;
	snd_pcm_uframes_t poll_size = period_size;
	struct snd_pcm_params params = {0};

	if((i2si_fd >= 0) && (i2so_fd >= 0))
		return 0;

	i2si_fd = open("/dev/sndC0i2si", O_WRONLY);
	if(i2si_fd < 0){
		printf("Open /dev/sndC0i2si fail\n");
		return -1;
	}

	printf("ioctl````\n");
	struct snd_cjc8988_input src_mode = {"linein"};
	ioctl(i2si_fd, SND_IOCTL_SET_CJC8988_INPUT, &src_mode);

	i2si_params.access = access;
	i2si_params.format = format;
	i2si_params.sync_mode = 0;
	i2si_params.align = align;
	i2si_params.rate = rate;
	i2si_params.channels = channels;
	i2si_params.period_size = period_size;
	i2si_params.periods = periods;
	i2si_params.bitdepth = bitdepth;
	i2si_params.start_threshold = 1;
	i2si_params.pcm_source = SND_PCM_SOURCE_AUDPAD;
	i2si_params.pcm_dest = SND_PCM_DEST_BYPASS;
	ret = ioctl(i2si_fd, SND_IOCTL_HW_PARAMS, &i2si_params);
	if (ret < 0) {
		printf("SND_IOCTL_HW_PARAMS error \n");
		close(i2si_fd);
		i2si_fd = -1;
		return -1;
	}

	i2so_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (i2so_fd < 0) {
		printf("Open /dev/sndC0i2so error \n");
		close(i2si_fd);
		i2si_fd = -1;
		return -1;
	}

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S16_LE;
	params.sync_mode = 0;
	params.align = align;
	params.rate = rate;

	params.channels = channels;
	params.period_size = read_size;
	params.periods = periods;
	params.bitdepth = bitdepth;
	params.start_threshold = 2;
	ioctl(i2so_fd, SND_IOCTL_HW_PARAMS, &params);
	printf ("SND_IOCTL_HW_PARAMS done\n");

	ioctl(i2so_fd, SND_IOCTL_AVAIL_MIN, &poll_size);

	ioctl(i2si_fd, SND_IOCTL_START, 0);
	printf ("i2si_fd SND_IOCTL_START done\n");

	ioctl(i2so_fd, SND_IOCTL_START, 0);
	printf ("i2so_fd SND_IOCTL_START done\n");

	spdif_open(rate, channels, bitdepth, SND_PCM_FORMAT_S16_LE);

	(void)argc;
	(void)argv;
	return 0;
}

static int cvbs_audio_close(int argc, char *argv[])
{
	printf(">>> cvbs_audio_close\n");
	if(i2so_fd >= 0)
	{
		printf ("cvbs_audio_close --i2so_fd, SND_IOCTL_HW_FREE \n");
		ioctl(i2so_fd, SND_IOCTL_DROP, 0);
		ioctl(i2so_fd, SND_IOCTL_HW_FREE, 0);
		close(i2so_fd);
		i2so_fd = -1;
	}
	if(i2si_fd >= 0){
		printf ("cvbs_audio_close --i2si_fd, SND_IOCTL_HW_FREE \n");
		ioctl(i2si_fd, SND_IOCTL_DROP, 0);
		ioctl(i2si_fd, SND_IOCTL_HW_FREE, 0);
		close(i2si_fd);
		i2si_fd = -1;
	}

	(void)argc;
	(void)argv;
	return 0;
}


void mplayer_cmds_register(struct console_cmd *cmd_root)
{
    struct console_cmd *cmd = NULL;

    console_register_cmd(cmd_root, "transcode",  transcode, CONSOLE_CMD_MODE_SELF, "transcode ipath opath");

    console_register_cmd(cmd_root, "cvbs_on", cvbs_audio_open, CONSOLE_CMD_MODE_SELF,
        "cvbs audio on");
    console_register_cmd(cmd_root, "cvbs_off", cvbs_audio_close, CONSOLE_CMD_MODE_SELF,
        "cvbs audio off");
    console_register_cmd(cmd_root, "dbg", mp_debug, CONSOLE_CMD_MODE_SELF,
        "init: debug mp");
    console_register_cmd(cmd_root, "init", mp_init, CONSOLE_CMD_MODE_SELF,
        "init: init mp");
    console_register_cmd(cmd_root, "deinit", mp_deinit, CONSOLE_CMD_MODE_SELF,
        "deinit: deinit mp");
    console_register_cmd(cmd_root, "multi", mp_multi, CONSOLE_CMD_MODE_SELF,
        "enter multi-instance mode");
    console_register_cmd(cmd_root, "single", mp_single, CONSOLE_CMD_MODE_SELF,
        "exit multi-instance mode");

    console_register_cmd(cmd_root, "play", mp_play, CONSOLE_CMD_MODE_SELF,
        "play file_path -s sync -b buffering -d img_dis_mode -t start_time -r rotate -a athresh -e play_subtitle\n\t\t\t"
        "-s sync: 0, free run; 1, sync to stc(not support yet); 2, audio master, 3, video_master(not support yet)\n\t\t\t"
        "-b buffering: 0/1\n\t\t\t"
        "-d img_dis_mode: 1, full screen; 2, realsize; 4, auto(pillbox or letterbox)\n\t\t\t"
        "-t time: 0< time < 1, start_time = total_duration * time; time >= 1, start_time = time(seconds)\n\t\t\t"
        "-r rotate: 1, 90; 2, 180; 3, 270; 4, only enable rotate\n\t\t\t"
        "-m mirror: 1, left-right mirror; 2, up-down mirror\n\t\t\t"
        "-f project mode: 0, Rear; 1, Ceiling Rear; 2, Front; 3, Ceiling Front\n\t\t\t"
        "-e play_subtitle: 1, show subtitle; 0, do not show subtitle\n\t\t\t"
        "-i set mix volume priority: value is [0,1,2],and 2 is the highest level\n\t\t\t"
        "-c set slave mode,0 is master_mode,1 is slave_mode\n\t\t\t"
        "-v set mix volume maximum_weight:value is 0 or 1,1 is 100% volume,and 0 is not\n\t\t\t"
        "-a athresh: i2so will only keep max athresh data, unit is ms\n\t\t\t"
        "-p bypass mode: 0, bypass off; 1, bypass raw; 2, bypass PCM; 3, bypass raw no preamble;\n\t\t\t"
        "-o sound dev path: 0, I2SO; 1, PCM output; 2, spdif output; 3, DDP spdif output\n\t\t\t"
        "-z video PBP mode enable: 0, disable PBP mode; 1, enable PBP mode\n\t\t\t"
        "-y PBP mode play on DE: 0, 2K DE; 1, 4K DE. only valid while enable PBP mode\n\t\t\t"
        "-l PBP mode play on DE layer: 0, main layer; 1, AUXP layer. only valid while enable PBP mode\n\n"
        );
    console_register_cmd(cmd_root, "memory_play", mp_memory_play, CONSOLE_CMD_MODE_SELF,
        "memory_play local_file_path\n\t\t\t"
        "simulate memory play, will load file to memory, and then play memory with ffplayer");
    console_register_cmd(cmd_root, "stop_mode", mp_stop_mode, CONSOLE_CMD_MODE_SELF,
        "stop_mode closevp(bool) fillblack(bool, not support yet);\n\t\t\t"
        "for example stop_mode 0 0\n");
    console_register_cmd(cmd_root, "stop", mp_stop, CONSOLE_CMD_MODE_SELF,
        "stop: stop all player. \n\t\t\t"
        "If in multi mode, use 'stop /mnt/1.mkv'");
    console_register_cmd(cmd_root, "stop2", mp_stop2, CONSOLE_CMD_MODE_SELF,
        "stop: stop all player. \n\t\t\t"
        "If in multi mode, use 'stop /mnt/1.mkv'");
    console_register_cmd(cmd_root, "pause", mp_pause, CONSOLE_CMD_MODE_SELF,
        "pause: pause all player. \n\t\t\t"
        "If in multi mode, use 'pause /mnt/1.mkv'");
    console_register_cmd(cmd_root, "resume", mp_resume, CONSOLE_CMD_MODE_SELF,
        "resume: resume all player. \n\t\t\t"
        "If in multi mode, use 'resume /mnt/1.mkv'");
    console_register_cmd(cmd_root, "seek", mp_seek, CONSOLE_CMD_MODE_SELF,
        "seek time: seek to time, unit is second. 0 < time < duration \n\t\t\t"
        "If in multi mode, use 'seek /mnt/1.mkv time'");
    console_register_cmd(cmd_root, "rate", mp_rate, CONSOLE_CMD_MODE_SELF,
        "rate n: change play rate of the player, unit is float, n > 0 or n < 0 is both ok.\n\t\t\t"
        "If in multi mode, we can use 'rate /mnt/1.mkv n'");
    console_register_cmd(cmd_root, "info", mp_info, CONSOLE_CMD_MODE_SELF,
        "info: print media info of all player. \n\t\t\t"
        "If in multi mode, use 'info /mnt/1.mkv'");
    console_register_cmd(cmd_root, "time", mp_time, CONSOLE_CMD_MODE_SELF,
        "time: print position & duration of all player. \n\t\t\t"
        "If in multi mode, use 'time /mnt/1.mkv'");
    console_register_cmd(cmd_root, "rotate", mp_change_rotate, CONSOLE_CMD_MODE_SELF,
        "rotate <file_name> -r 1 -m 1 \n\t\t\t"
        " -r: 1, 90; 2, 180; 3, 270\n\t\t\t"
        " -m: 0, no flip; 1, left-right flip; 2, up-down flip\n\t\t\t"
        " -f project mode: 0, Rear; 1, Ceiling Rear; 2, Front; 3, Ceiling Front\n\t"        
        );
    console_register_cmd(cmd_root, "preview", mp_preview, CONSOLE_CMD_MODE_SELF,
        "preview src dst, base on 1920*1080\n\t\t\t"
        "for example: preview 0 0 1920 1080  800 450 320 180");
    console_register_cmd(cmd_root, "rm_preview", mp_rm_preview, CONSOLE_CMD_MODE_SELF,
        "remove preview flag");
    console_register_cmd(cmd_root, "full_screen", mp_full_screen, CONSOLE_CMD_MODE_SELF,
        "resume to full screen play from preview");
    console_register_cmd(cmd_root, "disav", mp_disable_av, CONSOLE_CMD_MODE_SELF,
        "disav -v 0 -a 1");
    console_register_cmd(cmd_root, "smooth", mp_smooth, CONSOLE_CMD_MODE_SELF,
        "smooth 1/0 1: smooth mode; 0:normal mode\n\t\t\t");

    console_register_cmd(cmd_root, "volume", mp_volume, CONSOLE_CMD_MODE_SELF,
        "volume 100 -> set volume 100\n\t\t\t");
    console_register_cmd(cmd_root, "mute", mp_i2so_mute, CONSOLE_CMD_MODE_SELF,
        "mute 1/0 -> 1/0 to mute i2so /unmute i2so\n\t\t\t");
    console_register_cmd(cmd_root, "gpio_mute", mp_i2so_gpio_mute, CONSOLE_CMD_MODE_SELF,
        "gpio mute 1/0 -> 1/0 to gpio mute i2so /unmute i2so\n\t\t\t");
    console_register_cmd(cmd_root, "hdmi_mute", mp_hdmi_mute, CONSOLE_CMD_MODE_SELF,
        "mute 1/0 -> enable/disable hdmi out\n\t\t\t");

    console_register_cmd(cmd_root, "twotone", mp_set_twotone, CONSOLE_CMD_MODE_SELF,
            "twotone -o (1/0) -m 1 -> set twodone on/off & music mode\n\t\t\t");
    console_register_cmd(cmd_root, "lr_balance", mp_set_lr_balance, CONSOLE_CMD_MODE_SELF,
            "lr_balance -o (1/0) -i 1 -> set lr_balance on/off & index 1 \n\t\t\t");

    console_register_cmd(cmd_root, "eq6", mp_set_audio_eq6, CONSOLE_CMD_MODE_SELF,
            "eq6 -o (1/0) -m 1 -> set audio eq6 on/off & mode  1 \n\t\t\t");

    console_register_cmd(cmd_root, "eq_enqble", mp_set_eq_enable, CONSOLE_CMD_MODE_SELF,
            "eq_enable <0/1>\n");
    console_register_cmd(cmd_root, "eq", mp_set_eq, CONSOLE_CMD_MODE_SELF,
            "eq -b <band 0~9> -c <cutoff> -q <q> -g <gain>\n");

    console_register_cmd(cmd_root, "pbe", mp_set_pbe, CONSOLE_CMD_MODE_SELF, "pbe <0~100>\n");
    console_register_cmd(cmd_root, "pbe_precut", mp_set_pbe_precut, CONSOLE_CMD_MODE_SELF, "pbe_precut <-45 ~ 0>\n");

    console_register_cmd(cmd_root, "pic_mode", mp_pic_mode, CONSOLE_CMD_MODE_SELF,
        "picture_mode -i 50 -> set gif interval to 50ms\n\t\t\t"
        "picture_mode -d 3000 -> set picture show duration to 3000 ms\n\t\t\t"
        "picture_mode -b 1 -> set png backgroud, 1: white, 2: black, 3: chess\n\n");
    console_register_cmd(cmd_root, "pic_effect", mp_pic_effect, CONSOLE_CMD_MODE_SELF,
        "pic_effect  mode  direction  type  time\n\t\t\t"
        "mode: 0, normal; 1, shuttle; 2, brush; 3, slide; 4, ramdom; 5, fade\n\t\t\t"
        "direction: 0, up; 1, left; 2 down; 3, right\n\t\t\t"
        "time: step interval, unit is ms"
        "for example: pic_effect 1 0 0 50");
    console_register_cmd(cmd_root, "pic_effect_enable", mp_pic_effect_enable, CONSOLE_CMD_MODE_SELF,
        "pic_effect_en 1/pic_effect_en 0 to alloc/free pic_effect buffer");


    console_register_cmd(cmd_root, "scan", mp_scan, CONSOLE_CMD_MODE_SELF,
        "scan directory_path default sync mode is 2\n"
        "for example: 'scan /mnt/usb 2' will exit multi-instance mode, \n\t\t\tand scan /mnt/usb to create a list and then play list one by one\n\n");
    console_register_cmd(cmd_root, "n", mp_next, CONSOLE_CMD_MODE_SELF,
        "play next url in list");
    console_register_cmd(cmd_root, "p", mp_prev, CONSOLE_CMD_MODE_SELF,
        "play previous url in list");
    console_register_cmd(cmd_root, "scan2", mp_scan2, CONSOLE_CMD_MODE_SELF,
        "scan directory_path, only play audio, default sync mode is 0\n"
        "for example: 'scan /mnt/usb 0' will exit multi-instance mode, \n\t\t\tand scan /mnt/usb to create a list and then play list one by one\n\n");
    console_register_cmd(cmd_root, "n2", mp_next2, CONSOLE_CMD_MODE_SELF,
        "play next url in list");
    console_register_cmd(cmd_root, "p2", mp_prev2, CONSOLE_CMD_MODE_SELF,
        "play previous url in list");

    console_register_cmd(cmd_root, "log", mp_log, CONSOLE_CMD_MODE_SELF,
        "log level: 0, panic; 1, fatal; 2, err, 3, warn; 4, info; 5, verbose(print main flow); 6, debug(print pkt info); 7, trace(print all)\n\n");
    console_register_cmd(cmd_root, "loopinfo", mp_loop_info, CONSOLE_CMD_MODE_SELF,
        "loopinfo 200 -> will loop get info per 200ms");
    console_register_cmd(cmd_root, "rm_loopinfo", mp_remove_loop_info, CONSOLE_CMD_MODE_SELF,
        "rm_loopinfo -> will stop get info loop");
    console_register_cmd(cmd_root, "auto_switch", mp_auto_switch, CONSOLE_CMD_MODE_SELF,
        "auto_switch 1/0 -> 1/0 to start/stop auto change stream test");
    console_register_cmd(cmd_root, "auto_seek", mp_auto_seek, CONSOLE_CMD_MODE_SELF,
        "auto_seek 1/0 -> 1/0 to start/stop auto seek test");

    console_register_cmd(cmd_root, "atrack", mp_set_audio_track, CONSOLE_CMD_MODE_SELF,
        "atrack index;\n\t\t\t"
        "set audio play track, index range(0 ~ (hcplayer_get_audio_streams_count() - 1) \n\t\t\t"
        "info cmd can get number of audio tracks\n\n");
    console_register_cmd(cmd_root, "vtrack", mp_set_video_track, CONSOLE_CMD_MODE_SELF,
        "vtrack index");
    console_register_cmd(cmd_root, "strack", mp_set_subtitle_track, CONSOLE_CMD_MODE_SELF,
        "strack index");

    console_register_cmd(cmd_root, "transcode_set", mp_set_video_transcode, CONSOLE_CMD_MODE_SELF,
        "set video transcode config");

    console_register_cmd(cmd_root, "two_play", mp_play_two_uri, CONSOLE_CMD_MODE_SELF,
        "two_play a_uri v_uri\n");
    console_register_cmd(cmd_root, "two_stop", mp_stop_two_uri, CONSOLE_CMD_MODE_SELF,
        "two_stop");
    console_register_cmd(cmd_root, "two_time", mp_time_two_uri, CONSOLE_CMD_MODE_SELF,
        "two_time");
    console_register_cmd(cmd_root, "two_seek", mp_seek_two_uri, CONSOLE_CMD_MODE_SELF,
        "two_seek");
    console_register_cmd(cmd_root, "two_pause", mp_pause_two_uri, CONSOLE_CMD_MODE_SELF,
        "two_pause");
    console_register_cmd(cmd_root, "two_resume", mp_resume_two_uri, CONSOLE_CMD_MODE_SELF,
        "two_resume");
    console_register_cmd(cmd_root, "decryption_key", mp_set_decryption_key, CONSOLE_CMD_MODE_SELF,
        "set decryption_key, for example: decryption_key c7e16c4403654b85847037383f0c2db3");

    console_register_cmd(cmd_root, "audio_smooth_switch", mp_audio_smooth_switch, CONSOLE_CMD_MODE_SELF,
        "audio smooth switch");
    console_register_cmd(cmd_root, "audio_rate_play", mp_audio_rate_play, CONSOLE_CMD_MODE_SELF,
        "audio rate play");
	console_register_cmd(cmd_root, "audio_pitch_shift", mp_audio_pitch_shift, CONSOLE_CMD_MODE_SELF,
		"audio pitch shift");

    /* dis test */
    cmd = console_register_cmd(cmd_root, "dis_test", enter_dis_test, CONSOLE_CMD_MODE_SELF,
        "enter dis test");
    console_register_cmd(cmd, "aspect_ratio", aspect_test, CONSOLE_CMD_MODE_SELF,
        "aspect_ratio -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -t TV_MODE -m DISPLAY_MODE");
    console_register_cmd(cmd, "disrotate", rotate_test, CONSOLE_CMD_MODE_SELF,
        "disrotate -r rotate(1 90, 2 180,3 270) -m mirror -d DIS_TYPE");
    console_register_cmd(cmd, "tvsys" , tvsys_test , CONSOLE_CMD_MODE_SELF ,
        "tvsys -c cmd(0:DIS_SET_TVSYS  1:DIS_GET_TVSYS) -l DIS_LAYER -d DIS_TYPE -t TVTYPE -p progressive -s 1:dual out 0:single out -a dac type");
    console_register_cmd(cmd, "zoom" , zoom_test , CONSOLE_CMD_MODE_SELF ,
        "tvsys -c cmd -l DIS_LAYER -d DIS_TYPE source area:(-x -y -w -h )  dst area:(-o x-offset of the area -k y-offset of the area -j Width of the area -g Height of the area");
    console_register_cmd(cmd, "winon", winon_test , CONSOLE_CMD_MODE_SELF ,
        "winon -l DIS_LAYER(0x1 main, 0x10 auxp) -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -o ON(1 on, 0 off)");
    console_register_cmd(cmd, "venhance", venhance_test , CONSOLE_CMD_MODE_SELF ,
        "venhance -c Picture-enhancement-type(0x01 BRIGHTNESS,0x02 CONTRAST,0x04 SATURATION,0x08 SHARPNESS,0x10 HUE)-d DIS_TYPE(0 sd, 1 hd, 2 uhd) -g picture enhancement value(0-100,default 50)");
    console_register_cmd(cmd, "hanen", enhance_enable_test , CONSOLE_CMD_MODE_SELF ,
        "hanen -e ENHANCE EN(0 disable, 1 enable) -d DIS_TYPE(0 sd, 1 hd, 2 uhd)");
    console_register_cmd(cmd, "outfmt", hdmi_out_fmt_test , CONSOLE_CMD_MODE_SELF ,
        "outfmt -f HDMI_OUT_FORMT(1 YCBCR_420,2 YCBCR_422,3 YCBCR_444,4 RGB_MODE1,5 RGB_MODE2) -d DIS_TYPE(0 sd, 1 hd, 2 uhd)");
    console_register_cmd(cmd, "layerorder", layerorder_test , CONSOLE_CMD_MODE_SELF ,
        "layerorder -d 1 -o 0 -l 1 -r 2 -g 3 ->set layer order: main_layer 0, auxp_layer 1, gmas_layer 2, gmaf_layer 3");
    console_register_cmd(cmd, "bppic", backup_pic_test , CONSOLE_CMD_MODE_SELF ,"bppic  -d DIS_TYPE(0 sd, 1 hd, 2 uhd)");
    console_register_cmd(cmd, "cutoff", cutoff_test , CONSOLE_CMD_MODE_SELF ,
        "cutoff -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -c cutoff");
    console_register_cmd(cmd, "awonoff", auto_win_onoff_test , CONSOLE_CMD_MODE_SELF ,
        "awonoff -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -w WinOnOff");
    console_register_cmd(cmd, "singleout", single_output_test , CONSOLE_CMD_MODE_SELF ,
        "singleout -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -o single output");
    console_register_cmd(cmd, "keystone", keystone_param_test , CONSOLE_CMD_MODE_SELF ,
        "keystone -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -c bg color enable(0 disable 1 enable) -e keystone(0 disable 1 enable) -w up width -i down width -y bg y -b bg cb -r bg cr");
    console_register_cmd(cmd, "get_keystone", get_keystone_param_test , CONSOLE_CMD_MODE_SELF ,
        "get keystone -d DIS_TYPE(0 sd, 1 hd, 2 uhd)");
    console_register_cmd(cmd, "bgcolor", bg_color_test , CONSOLE_CMD_MODE_SELF ,
        "bgcolor -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -y color uY -b color uCb -r color uCr");
    console_register_cmd(cmd, "regdac", reg_dac_test , CONSOLE_CMD_MODE_SELF ,
        "regdac -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -t eDacType -e enable -f uDacFirst -s uDacSecond -h uDacThird -p bpregressive(1 DVI_SCAN_PROGRESSIVE,2 DVI_SCAN_INTERLACE) -v tvtype");
    console_register_cmd(cmd, "underdac", under_dac_test , CONSOLE_CMD_MODE_SELF ,
        "underdac -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -u type");

    cmd = console_register_cmd(cmd_root , "es_play" , enter_es_play, CONSOLE_CMD_MODE_SELF , "enter H264_es_test");
    console_register_cmd(cmd, "h264_es_test" , play_h264_es , CONSOLE_CMD_MODE_SELF , "enter H264_es_test");
    console_register_cmd(cmd, "h264_es_play" , h264_es_play , CONSOLE_CMD_MODE_SELF , "play h264 format raw data");
    console_register_cmd(cmd, "h264_es_stop" , h264_es_stop , CONSOLE_CMD_MODE_SELF , "stop h264 format raw data");

    /* vin dvp test */
    cmd = console_register_cmd(cmd_root, "vin_dvp", vindvp_enter, CONSOLE_CMD_MODE_SELF,
        "enter vin dvp test");
    console_register_cmd(cmd, "start", vindvp_start, CONSOLE_CMD_MODE_SELF, "vin dvp_start");
    console_register_cmd(cmd, "stop", vindvp_stop, CONSOLE_CMD_MODE_SELF, "vin dvp stop");
    console_register_cmd(cmd, "enable", vindvp_enable, CONSOLE_CMD_MODE_SELF, "vin_dvp_enable");
    console_register_cmd(cmd, "disable", vindvp_disable, CONSOLE_CMD_MODE_SELF, "vin_dvp_disable");
    console_register_cmd(cmd, "set_region", vindvp_set_combine_regin_status, CONSOLE_CMD_MODE_SELF, "vin_set_set_region");
    console_register_cmd(cmd, "capture", vindvp_capture_pictrue, CONSOLE_CMD_MODE_SELF, "vin_dvp_capture_pictrue");

    /* linux only */
    console_register_cmd(cmd_root, "avp", avp_cmd, CONSOLE_CMD_MODE_SELF,
        "send avp command to avp. e.g: avp | avp help | avp os top");
    console_register_cmd(cmd_root, "dumpstack", dumpstack_cmd, CONSOLE_CMD_MODE_SELF,
        "send avp command to avp. e.g: avp | avp help | avp os top");
    console_register_cmd(cmd_root, "system", mp_system, CONSOLE_CMD_MODE_SELF , "system argv[1]");

    
}


#ifdef BR2_PACKAGE_VIDEO_PBP_EXAMPLES
static int enter_mp_test(int argc , char *argv[])
{
    (void)argc;
    (void)argv;
    return 0;
}
#endif

int main (int argc, char *argv[])
{
	struct termios new_settings;
	FILE *fp = NULL;
	char buf[CONSOLE_MAX_CMD_BUFFER_LEN];

	mp_init(0, NULL);

	tcgetattr(0, &stored_settings);
	new_settings = stored_settings;
	//new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_lflag &= ~(ICANON | ECHO);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);

	signal(SIGTERM, exit_console);
	signal(SIGINT, exit_console);
	signal(SIGSEGV, exit_console);
	signal(SIGBUS, exit_console);

#ifdef BR2_PACKAGE_VIDEO_PBP_EXAMPLES
    console_init("multi_dis:");

    struct console_cmd *cmd;

    extern void pbp_test_cmds_register(struct console_cmd *cmd);
    extern int video_pbp_enter(int argc, char *argv[]);
    cmd = console_register_cmd(NULL, "pbp", video_pbp_enter, CONSOLE_CMD_MODE_SELF, "Entering pbp testing");
    pbp_test_cmds_register(cmd);

    cmd = console_register_cmd(NULL, "mplayer", enter_mp_test, CONSOLE_CMD_MODE_SELF, "Entering multi display for MP");
    mplayer_cmds_register(cmd);

  #ifdef BR2_PACKAGE_HCCAST_WIFI_CAST_EXAMPLES
    #include "hccast_test/hccast_test.h"
    cmd = console_register_cmd(NULL, "cast_wifi",  cast_wifi_init, CONSOLE_CMD_MODE_SELF, "Enter wifi cast multi display testing");
    wifi_cast_cmds_register(cmd);
  #endif

  #ifdef BR2_PACKAGE_HCCAST_USB_CAST_EXAMPLES
    #include "hccast_test/hccast_test.h"
    cmd = console_register_cmd(NULL, "cast_usb",  cast_usb_init, CONSOLE_CMD_MODE_SELF, "Enter usb cast multi display testing");
    usb_cast_cmds_register(cmd);
  #endif

  #ifdef BR2_PACKAGE_HC_TVDEC_TEST
    extern void tv_dec_cmds_register(struct console_cmd *cmd);
    extern int tv_dec_enter(int argc , char *argv[]);
    cmd = console_register_cmd(NULL, "tv_dec",  tv_dec_enter, CONSOLE_CMD_MODE_SELF, "Enter tv decoder(CVBS rx) testing");
    tv_dec_cmds_register(cmd);
  #endif

  #ifdef BR2_PACKAGE_HC_HDMIRX_TEST
    extern int hdmi_test_rx_enter(int argc , char *argv[]);
    extern void hdmi_rx_cmds_register(struct console_cmd *cmd);
    cmd = console_register_cmd(NULL, "hdmi_rx",  hdmi_test_rx_enter, CONSOLE_CMD_MODE_SELF, "Enter HMDI in testing");
    hdmi_rx_cmds_register(cmd);
  #endif

#else    
    system("cat /dev/zero > /dev/fb0");
    console_init("mp:");
    mplayer_cmds_register(NULL);
#endif    

	if (argc == 2) {
		fp = fopen(argv[1], "r");
	}

	if (fp) {
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			printf("[console_run_cmd] %s", buf);
			console_run_cmd(buf);
		}
		fclose(fp);
	}

	console_start();

	exit_console(0);
	return 0;
}

#else //else of _linux_

CONSOLE_CMD(mp, NULL, mp_init, CONSOLE_CMD_MODE_SELF,
	"enter & init mplayer")
CONSOLE_CMD(init,  "mp", mp_init, CONSOLE_CMD_MODE_SELF,
	"init mplayer")
CONSOLE_CMD(deinit, "mp", mp_deinit, CONSOLE_CMD_MODE_SELF,
	"deinit mplayer")
CONSOLE_CMD(multi, "mp", mp_multi, CONSOLE_CMD_MODE_SELF,
	"enter multi-instance mode")
CONSOLE_CMD(single, "mp", mp_single, CONSOLE_CMD_MODE_SELF,
	"exit multi-instance mode")
CONSOLE_CMD(smooth, "mp", mp_smooth, CONSOLE_CMD_MODE_SELF,
	"smooth 1/0 1: smooth mode; 0:normal mode")

CONSOLE_CMD(play, "mp", mp_play, CONSOLE_CMD_MODE_SELF,
	"play file_path -s sync -b buffering -d img_dis_mode -t start_time -r rotate -a athresh -e play_subtitle\n\t\t\t"
	"-s sync: 0, free run; 1, sync to stc(not support yet); 2, audio master, 3, video_master(not support yet)\n\t\t\t"
	"-b buffering: 0/1\n\t\t\t"
	"-d img_dis_mode: 1, full screen; 2, realsize; 4, auto(pillbox or letterbox)\n\t\t\t"
	"-t time: 0< time < 1, start_time = total_duration * time; time >= 1, start_time = time(seconds)\n\t\t\t"
	"-r rotate: 1, 90; 2, 180; 3, 270; 4, only enable rotate\n\t\t\t"
	"-m mirror:(not support yet)\n\t\t\t"
	"-e play_subtitle: 1, show subtitle; 0, do not show subtitle\n\t\t\t"
	"-a athresh: i2so will only keep max athresh data, unit is ms\n\t\t\t"
	"-p bypass mode: 0, bypass off; 1, bypass raw; 2, bypass PCM; 3, bypass raw no preamble;\n\t\t\t"
	"-o sound dev path: 1, I2SO; 2, PCM output; 4, spdif output; 8, DDP spdif output. can overlay\n\t\t\t"
	"-z multi display enable: 0, disable; 1, enable\n\t\t\t"
	"-y multi display on DE type: 0, 2K DE; 1, 4K DE. only valid while enable PBP mode\n\t\t\t"
	"-l multi display on DE layer: 0, main layer; 1, AUXP layer. only valid while multi display enable\n\t\t\t"
	"-w Preview's coordinates and sizes:src.x,src.y,src.width,src.height,dst.x,dst.y,dst.width,dst.height\n\t\t\t"
	"-u user specified name, for example: -u 111\n\t\t\t"
)

CONSOLE_CMD(memory_play, "mp", mp_memory_play, CONSOLE_CMD_MODE_SELF,
	"memory_play local_file_path\n\t\t\t"
	"simulate memory play, will load file to memory, and then play memory with ffplayer")
CONSOLE_CMD(decryption_key, "mp", mp_set_decryption_key, CONSOLE_CMD_MODE_SELF,
	"set decryption_key;\n\t\t\t"
	"for example: decryption_key c7e16c4403654b85847037383f0c2db3\n")
CONSOLE_CMD(stop_mode, "mp", mp_stop_mode, CONSOLE_CMD_MODE_SELF,
	"stop_mode closevp(bool) fillblack(bool, not support yet);\n\t\t\t"
	"for example stop_mode 0 0\n")
CONSOLE_CMD(stop, "mp", mp_stop, CONSOLE_CMD_MODE_SELF,
	"stop: stop all player. \n\t\t\t"
	"If in multi mode, we can use 'stop /mnt/1.mkv'\n\t\t\t"
	"If in multi mode and set username, we can use 'stop username'")
CONSOLE_CMD(stop2, "mp", mp_stop2, CONSOLE_CMD_MODE_SELF,
	"stop: stop all player. \n\t\t\t"
	"If in multi mode, we can use 'stop /mnt/1.mkv'")
CONSOLE_CMD(pause, "mp", mp_pause, CONSOLE_CMD_MODE_SELF,
	"pause: pause all player. \n\t\t\t"
	"If in multi mode, we can use 'pause /mnt/1.mkv'")
CONSOLE_CMD(resume, "mp", mp_resume, CONSOLE_CMD_MODE_SELF,
	"resume: resume all player. \n\t\t\t"
	"If in multi mode, we can use 'resume /mnt/1.mkv'")
CONSOLE_CMD(seek, "mp", mp_seek, CONSOLE_CMD_MODE_SELF,
	"seek time: seek to time, unit is second. 0 < time < duration \n\t\t\t"
	"If in multi mode, we can use 'seek /mnt/1.mkv time'")
CONSOLE_CMD(rate, "mp", mp_rate, CONSOLE_CMD_MODE_SELF,
	"rate n: change play rate of the player, unit is float, n > 0 or n < 0 is both ok.\n\t\t\t"
	"If in multi mode, we can use 'rate /mnt/1.mkv n'")
CONSOLE_CMD(info, "mp", mp_info, CONSOLE_CMD_MODE_SELF,
	"info: print media info of all player. \n\t\t\t"
	"If in multi mode, we can use 'info /mnt/1.mkv'")
CONSOLE_CMD(time, "mp", mp_time, CONSOLE_CMD_MODE_SELF,
	"time: print position & duration of all player. \n\t\t\t"
	"If in multi mode, we can use 'time /mnt/1.mkv'")
CONSOLE_CMD(rotate, "mp", mp_change_rotate, CONSOLE_CMD_MODE_SELF,
	"rotate n: 1, 90; 2, 180; 3, 270")
CONSOLE_CMD(preview, "mp", mp_preview, CONSOLE_CMD_MODE_SELF,
	"preview src dst, base on 1920*1080\n\t\t\t"
	"If in multi mode and set username for example: preview username 0,0,1920,1080,800,450,320,180\n\t\t\t"
	"If in multi mode and set filepath for example: preview filepath 0,0,1920,1080,800,450,320,180\n\t\t\t"
	"If in normal mode for example: preview 0,0,1920,1080,800,450,320,180\n\t\t\t")
CONSOLE_CMD(rm_preview, "mp", mp_rm_preview, CONSOLE_CMD_MODE_SELF,
	"remove preview flag")
CONSOLE_CMD(full_screen, "mp", mp_full_screen, CONSOLE_CMD_MODE_SELF,
	"resume to full screen play from preview")


CONSOLE_CMD(volume, "mp", mp_volume, CONSOLE_CMD_MODE_SELF,
	"volume 100 -> set volume to 100\n\n")
CONSOLE_CMD(mute, "mp", mp_i2so_mute, CONSOLE_CMD_MODE_SELF,
	"mute 1/0 -> enable/disable snd out")
CONSOLE_CMD(hdmi_mute, "mp", mp_hdmi_mute, CONSOLE_CMD_MODE_SELF,
		"mute 1/0 -> enable/disable hdmi out")
CONSOLE_CMD(gpio_mute, "mp", mp_i2so_gpio_mute, CONSOLE_CMD_MODE_SELF,
	"gpio_mute 1/0 -> gpio mute enable/disable snd out")

CONSOLE_CMD(twotone, "mp", mp_set_twotone, CONSOLE_CMD_MODE_SELF,
	"twotone -o (1/0) -m 1 -> set twodone on/off & music mode\n")
CONSOLE_CMD(lr_balance, "mp", mp_set_lr_balance, CONSOLE_CMD_MODE_SELF,
	"lr_balance -o (1/0) -i 1 -> set lr_balance on/off & index 1\n")
CONSOLE_CMD(eq6, "mp", mp_set_audio_eq6, CONSOLE_CMD_MODE_SELF,
	"eq6 -o (1/0) -m 1 -> set audio eq6 on/off & mode 1\n")

CONSOLE_CMD(eq_enqble, "mp", mp_set_eq_enable, CONSOLE_CMD_MODE_SELF,
	"eq_enable <0/1>\n")
CONSOLE_CMD(eq, "mp", mp_set_eq, CONSOLE_CMD_MODE_SELF,
	"eq -b <band 0~9> -c <cutoff> -q <q> -g <gain>\n")

CONSOLE_CMD(pbe, "mp", mp_set_pbe, CONSOLE_CMD_MODE_SELF, "pbe <0~100>\n")
CONSOLE_CMD(pbe_precut, "mp", mp_set_pbe_precut, CONSOLE_CMD_MODE_SELF, "pbe_precut <-45 ~ 0>\n")


CONSOLE_CMD(pic_mode, "mp", mp_pic_mode, CONSOLE_CMD_MODE_SELF,
	"pic_mode -i 50 -> set gif interval to 50ms\n\t\t\t"
	"pic_mode -d 3000 -> set picture show duration to 3000 ms\n\t\t\t"
	"pic_mode -b 1 -> set png backgroud, 1: white, 2: black, 3: chess\n\n")
CONSOLE_CMD(pic_effect, "mp", mp_pic_effect, CONSOLE_CMD_MODE_SELF,
	"pic_effect mode direction type time\n\t\t\t"
	"mode: 0, normal; 1, shuttle; 2, brush; 3, slide; 4, ramdom; 5, fade\n\t\t\t"
	"direction: 0, up; 1, left; 2 down; 3, right\n\t\t\t"
	"time: step interval, unit is ms\n\t\t\t"
	"for example: pic_effect 1 0 0 50")
CONSOLE_CMD(pic_effect_enable, "mp", mp_pic_effect_enable, CONSOLE_CMD_MODE_SELF,
	"pic_effect_en 1/pic_effect_en 0 to alloc/free buffer for pic_effect")


CONSOLE_CMD(scan, "mp", mp_scan, CONSOLE_CMD_MODE_SELF,
	"scan directory_path"
	"for example: 'scan /mnt/usb' will exit multi-instance mode, scan /mnt/usb to create a list and then play list one by one\n\n")
CONSOLE_CMD(scan2, "mp", mp_scan2, CONSOLE_CMD_MODE_SELF,
	"scan directory_path"
	"for example: 'scan2 /mnt/usb' will exit multi-instance mode, scan /mnt/usb to create a list and then play list one by one\n\n")
CONSOLE_CMD(n, "mp", mp_next, CONSOLE_CMD_MODE_SELF,
	"play next stream in scaned list")
CONSOLE_CMD(n2, "mp", mp_next2, CONSOLE_CMD_MODE_SELF,
	"play next stream in scaned list")
CONSOLE_CMD(p, "mp", mp_prev, CONSOLE_CMD_MODE_SELF,
	"play previous stream in scaned list")
CONSOLE_CMD(p2, "mp", mp_prev2, CONSOLE_CMD_MODE_SELF,
	"play previous stream in scaned list")


CONSOLE_CMD(log, "mp", mp_log, CONSOLE_CMD_MODE_SELF,
	"log level: 0, panic; 1, fatal; 2, err, 3, warn; 4, info; 5, verbose(print main flow); 6, debug(print pkt info); 7, trace(print all)\n\n")
CONSOLE_CMD(loopinfo, "mp", mp_loop_info, CONSOLE_CMD_MODE_SELF,
	"loopinfo 200 -> will loop get info per 200ms")
CONSOLE_CMD(rm_loopinfo, "mp", mp_remove_loop_info, CONSOLE_CMD_MODE_SELF,
	"rm_loopinfo -> will stop get info loop")
CONSOLE_CMD(auto_switch, "mp", mp_auto_switch, CONSOLE_CMD_MODE_SELF,
	"auto_switch 1/0 -> 1/0 to start/stop auto change stream test")
CONSOLE_CMD(auto_seek, "mp", mp_auto_seek, CONSOLE_CMD_MODE_SELF,
	"auto_seek  1/0 -> 1/0 to start/stop auto seek test")

CONSOLE_CMD(atrack, "mp", mp_set_audio_track, CONSOLE_CMD_MODE_SELF,
	"atrack index;\n\t\t\t"
	"set audio play track, index range(0 ~ (hcplayer_get_audio_streams_count() - 1) \n\t\t\t"
	"info cmd can get number of audio tracks\n\n")
CONSOLE_CMD(vtrack, "mp", mp_set_video_track, CONSOLE_CMD_MODE_SELF,
	"vtrack index")
CONSOLE_CMD(strack, "mp", mp_set_subtitle_track, CONSOLE_CMD_MODE_SELF,
	"strack index")

CONSOLE_CMD(transcode_set, "mp", mp_set_video_transcode, CONSOLE_CMD_MODE_SELF,
	"set video transcode config")

CONSOLE_CMD(two_play, "mp", mp_play_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_play a_uri v_uri\n")
CONSOLE_CMD(two_stop, "mp", mp_stop_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_stop")
CONSOLE_CMD(two_time, "mp", mp_time_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_time")
CONSOLE_CMD(two_seek, "mp", mp_seek_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_seek")
CONSOLE_CMD(two_pause, "mp", mp_pause_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_pause")
CONSOLE_CMD(two_resume, "mp", mp_resume_two_uri, CONSOLE_CMD_MODE_SELF,
	"two_resume")

CONSOLE_CMD(disav, "mp", mp_disable_av, CONSOLE_CMD_MODE_SELF,
	"disav -v 0 -a 1")

CONSOLE_CMD(showav, "mp", showlogo, CONSOLE_CMD_MODE_SELF,
	"start show av\n\n")
CONSOLE_CMD(stop_showav, "mp", stop_showlogo, CONSOLE_CMD_MODE_SELF,
	"stop show av")
CONSOLE_CMD(wait_showav, "mp", wait_showlogo, CONSOLE_CMD_MODE_SELF,
	"wait show av done and then destroy it")
CONSOLE_CMD(h264_es_test, "mp", play_h264_es, CONSOLE_CMD_MODE_SELF,
	"enter H264_es_test");

CONSOLE_CMD(audio_smooth_switch, "mp", mp_audio_smooth_switch, CONSOLE_CMD_MODE_SELF,
	"audio smooth switch")
CONSOLE_CMD(audio_rate_play, "mp", mp_audio_rate_play, CONSOLE_CMD_MODE_SELF,
	"audio rate play")
CONSOLE_CMD(audio_pitch_shift, "mp", mp_audio_pitch_shift, CONSOLE_CMD_MODE_SELF,
	"audio pitch shift")

CONSOLE_CMD(dbg , "mp" , mp_debug , CONSOLE_CMD_MODE_SELF ,
            "show mp info")

CONSOLE_CMD(dis , NULL , enter_dis_test , CONSOLE_CMD_MODE_SELF , "enter dis test")
CONSOLE_CMD(tvsys , "dis" , tvsys_test , CONSOLE_CMD_MODE_SELF ,
		"tvsys -c cmd(0:DIS_SET_TVSYS  1:DIS_GET_TVSYS) -l DIS_LAYER -d DIS_TYPE -t TVTYPE -p progressive")
CONSOLE_CMD(zoom , "dis" , zoom_test , CONSOLE_CMD_MODE_SELF ,
		"zoom -l DIS_LAYER -d DIS_TYPE source area:(-x -y -w -h )  dst area:(-o x-offset of the area -k y-offset of the area -j Width of the area -g Height of the area)")
CONSOLE_CMD(aspect , "dis" , aspect_test , CONSOLE_CMD_MODE_SELF ,
		 "aspect -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -t TV_MODE -m DISPLAY_MODE")
CONSOLE_CMD(disrotate , "dis" , rotate_test , CONSOLE_CMD_MODE_SELF ,
		 "disrotate -r rotate(1 90, 2 180,3 270)  -m mirror -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(winon , "dis" , winon_test , CONSOLE_CMD_MODE_SELF ,
		 "winon -l DIS_LAYER(0x1 main, 0x10 auxp) -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -o ON(1 on, 0 off)")
CONSOLE_CMD(venhance , "dis" , venhance_test , CONSOLE_CMD_MODE_SELF ,
	"venhance -c Picture enhancement type(0x01 BRIGHTNESS,0x02 CONTRAST,0x04 SATURATION,0x08 SHARPNESS,0x10 HUE)-d DIS_TYPE(0 sd, 1 hd, 2 uhd) -g picture enhancement value(0-100,default 50)")
CONSOLE_CMD(hanen , "dis" , enhance_enable_test , CONSOLE_CMD_MODE_SELF ,
	"hanen -e ENHANCE EN(0 disable, 1 enable) -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(outfmt , "dis" , hdmi_out_fmt_test , CONSOLE_CMD_MODE_SELF ,
	"outfmt -f HDMI OUT FORMT(1 YCBCR_420,2 YCBCR_422,3 YCBCR_444,4 RGB_MODE1,5 RGB_MODE2) -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(layerorder , "dis" , layerorder_test , CONSOLE_CMD_MODE_SELF ,
	"layerorder -d 1 -o 0 -l 1 -r 2 -g 3 ->set layer order: main_layer 0, auxp_layer 1, gmas_layer 2, gmaf_layer 3")
CONSOLE_CMD(bppic , "dis" , backup_pic_test , CONSOLE_CMD_MODE_SELF ,"bppic  -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(cutoff , "dis" , cutoff_test , CONSOLE_CMD_MODE_SELF ,
	"cutoff -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -c cutoff")
CONSOLE_CMD(awonoff , "dis" , auto_win_onoff_test , CONSOLE_CMD_MODE_SELF ,
	"awonoff -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -w WinOnOff")
CONSOLE_CMD(singleout , "dis" , single_output_test , CONSOLE_CMD_MODE_SELF ,
	"singleout -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -o single output")
CONSOLE_CMD(keystone, "dis" , keystone_param_test , CONSOLE_CMD_MODE_SELF ,
	"keystone -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -c bg color enable(0 disable 1 enable) -e keystone(0 disable 1 enable) -w up width -i down width -y bg y -b bg cb -r bg cr")
CONSOLE_CMD(get_keystone, "dis" , get_keystone_param_test , CONSOLE_CMD_MODE_SELF, "get keystone -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(bgcolor , "dis" , bg_color_test , CONSOLE_CMD_MODE_SELF ,
	"bgcolor -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -y color uY -b color uCb -r color uCr")
CONSOLE_CMD(regdac , "dis" , reg_dac_test , CONSOLE_CMD_MODE_SELF ,
	"regdac -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -t eDacType -e enable -f uDacFirst -s uDacSecond -h uDacThird -p bpregressive(1 DVI_SCAN_PROGRESSIVE,2 DVI_SCAN_INTERLACE) -v tvtype")
CONSOLE_CMD(underdac , "dis" , under_dac_test , CONSOLE_CMD_MODE_SELF ,
	"underdac -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -u type")
CONSOLE_CMD(cb , "dis" , miracast_vscreen_cb_test , CONSOLE_CMD_MODE_SELF ,
	"cb (miracast vscreen cb) -d DIS_TYPE(0 sd, 1 hd, 2 uhd) -o 1:on 0:off")
CONSOLE_CMD(getdisplayinfo , "dis" , dis_get_display_info_test , CONSOLE_CMD_MODE_SELF ,
                "getdisplayinfo -d DIS_TYPE(0 sd, 1 hd, 2 uhd)")
CONSOLE_CMD(getbuf , "dis" , dis_get_video_buf_test , CONSOLE_CMD_MODE_SELF ,
                "get display buf in rgb")
CONSOLE_CMD(dump , "dis" , dis_dump , CONSOLE_CMD_MODE_SELF ,
                "y:buf_y c:buf_c s:size)")
CONSOLE_CMD(denh , "dis" , denh_enable_test , CONSOLE_CMD_MODE_SELF ,
                "dynamic enh -d DIS_TYPE(0 sd, 1 hd, 2 uhd),-o :onoff(0:off,1:on))")
CONSOLE_CMD(get_edid_all_res , "dis" , hdmi_get_edid_all_video_res , CONSOLE_CMD_MODE_SELF ,
            "get_edid_all_video_res")
CONSOLE_CMD(set_cec_onff , "dis" , hdmi_set_cec_onoff , CONSOLE_CMD_MODE_SELF ,
            "-e 0:off,1:on")
CONSOLE_CMD(get_cec_onff , "dis" , hdmi_get_cec_onoff , CONSOLE_CMD_MODE_SELF ,
            "get_cec_onff")
CONSOLE_CMD(send_cec_cmd , "dis" , hdmi_send_cec_cmd , CONSOLE_CMD_MODE_SELF ,
            "-c 0:standby")
CONSOLE_CMD(set_cec_logical_addr , "dis" , hdmi_send_logical_addr , CONSOLE_CMD_MODE_SELF ,
            "-c 0:standby")

CONSOLE_CMD(transcode, NULL,  transcode, CONSOLE_CMD_MODE_SELF, "transcode ipath opath")
CONSOLE_CMD(get_blend_order , "dis"  , dis_get_blend_order , CONSOLE_CMD_MODE_SELF , "get blend order")
CONSOLE_CMD(set_cvbs_src_layer , "dis" , dis_set_cvbs_source_layer , CONSOLE_CMD_MODE_SELF , "set cvbs src layer")
CONSOLE_CMD(set_global_alpha , "dis" , global_alpha_test , CONSOLE_CMD_MODE_SELF , 
            "set global alpha,-d DIS_TYPE(0 sd, 1 hd, 2 uhd) -l DIS_LAYER(0x1 main, 0x10 auxp) -e enable -a alpha")
#endif
