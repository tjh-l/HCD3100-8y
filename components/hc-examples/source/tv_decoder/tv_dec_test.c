#include <fcntl.h>
#include <unistd.h>

#ifdef __linux__
#include <pthread.h>
#include <termios.h>
#include <signal.h>
#include "console.h"
#include <autoconf.h>
#else
#include <kernel/vfs.h>
#include <kernel/io.h>
#include <kernel/completion.h>
#include <kernel/lib/console.h>
#include <kernel/lib/fdt_api.h>
#include <generated/br2_autoconf.h>
#endif

#include <stdio.h>
#include <getopt.h>
#include <malloc.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/tvdec.h>
#include <hcuapi/tvtype.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/viddec.h>
#include <stdlib.h>
#include <string.h>


static int tv_dec_fd = -1;
static bool video_stop_read = 0;
static enum TVDEC_VIDEO_DATA_PATH vpath = TVDEC_VIDEO_TO_DE;
static unsigned int rotate_mode = ROTATE_TYPE_0;
static unsigned int mirror_mode = MIRROR_TYPE_NONE;
static int project_mode = 0; //0: PROJECT_REAR; 1: PROJECT_CEILING_REAR; 2: PROJECT_FRONT; 3: PROJECT_CEILING_FRONT
static enum TVTYPE tv_sys = TV_NTSC;
static bool tv_dec_started = false;
static unsigned int stop_mode = 0;
static enum DIS_LAYER tv_dis_layer = DIS_LAYER_MAIN;
static dis_type_e tv_dis_type = DIS_TYPE_HD;
static vdec_dis_rect_t g_dis_rect = { {0, 0, 1920, 1080}, {0, 0, 1920, 1080} };
static unsigned int g_preview = false;
static bool tv_dec_traning = false;
static uint8_t brightness = 50;
static uint8_t dc_offset = 0x80;
static video_pbp_mode_e g_pbp_mode = 0;
static struct wm_param wm = {0};


//////////////////////////////////////////////////////////////
//following apis is for transfering project mode to ration and flip
#if 1

typedef enum{
    TVTEST_PROJECT_REAR = 0,
    TVTEST_PROJECT_CEILING_REAR,
    TVTEST_PROJECT_FRONT,
    TVTEST_PROJECT_CEILING_FRONT,

    TVTEST_PROJECT_MODE_MAX
}tvtest_project_mode_e;


typedef struct{
    uint16_t screen_init_rotate;
    uint16_t screen_init_h_flip;
    uint16_t screen_init_v_flip;
}tv_rotate_cfg_t;

static tv_rotate_cfg_t tv_rotate_info[2];

#ifdef __linux__
static void tvtest_dts_string_get(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);
    if(fd >= 0){
        read(fd, string, size);
        close(fd);
    }
}

static int tvtest_dts_uint32_get(const char *path)
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

static void tvtest_screen_rotate_info(void)
{
   static int init_flag = 0;
    if (init_flag)
        return;

    unsigned int rotate=0,h_flip=0,v_flip=0;
#ifdef __HCRTOS__
    int np;

    memset(tv_rotate_info, 0, sizeof(tv_rotate_info));
    np = fdt_node_probe_by_path("/hcrtos/rotate");
    if(np>=0)
    {
        fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
        fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
        fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
        tv_rotate_info[0].screen_init_rotate = rotate;
        tv_rotate_info[0].screen_init_h_flip = h_flip;
        tv_rotate_info[0].screen_init_v_flip = v_flip;
    }

    np = fdt_node_probe_by_path("/hcrtos/rotate_4k");
    if(np>=0)
    {
        fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
        fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
        fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
        tv_rotate_info[1].screen_init_rotate = rotate;
        tv_rotate_info[1].screen_init_h_flip = h_flip;
        tv_rotate_info[1].screen_init_v_flip = v_flip;
    }

#else
#define ROTATE_CONFIG_PATH "/proc/device-tree/hcrtos/rotate"
#define ROTATE_4K_CONFIG_PATH "/proc/device-tree/hcrtos/rotate_4k"
    char status[16] = {0};
    memset(tv_rotate_info, 0, sizeof(tv_rotate_info));

    tvtest_dts_string_get(ROTATE_CONFIG_PATH "/status", status, sizeof(status));
    if(!strcmp(status, "okay")){
        rotate = tvtest_dts_uint32_get(ROTATE_CONFIG_PATH "/rotate");
        h_flip = tvtest_dts_uint32_get(ROTATE_CONFIG_PATH "/h_flip");
        v_flip = tvtest_dts_uint32_get(ROTATE_CONFIG_PATH "/v_flip");
        tv_rotate_info[0].screen_init_rotate = rotate;
        tv_rotate_info[0].screen_init_h_flip = h_flip;
        tv_rotate_info[0].screen_init_v_flip = v_flip;
    }

    tvtest_dts_string_get(ROTATE_4K_CONFIG_PATH "/status", status, sizeof(status));
    if(!strcmp(status, "okay")){
        rotate = tvtest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/rotate");
        h_flip = tvtest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/h_flip");
        v_flip = tvtest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/v_flip");
        tv_rotate_info[1].screen_init_rotate = rotate;
        tv_rotate_info[1].screen_init_h_flip = h_flip;
        tv_rotate_info[1].screen_init_v_flip = v_flip;
    }

#endif
    init_flag = 1;
    printf("%s()->>> 2k init_rotate = %u h_flip %u v_flip = %u\n", __func__,
        tv_rotate_info[0].screen_init_rotate,
        tv_rotate_info[0].screen_init_h_flip,
        tv_rotate_info[0].screen_init_v_flip);
    printf("%s()->>> 4k init_rotate = %u h_flip %u v_flip = %u\n", __func__, 
        tv_rotate_info[1].screen_init_rotate,
        tv_rotate_info[1].screen_init_h_flip,
        tv_rotate_info[1].screen_init_v_flip);


}


static uint16_t tvtest_get_screen_init_rotate(int dis_tpye)
{
    if (DIS_TYPE_HD == dis_tpye)
        return tv_rotate_info[0].screen_init_rotate;
    else
        return tv_rotate_info[1].screen_init_rotate;
}

static uint16_t tvtest_get_screen_init_h_flip(int dis_tpye)
{
    if (DIS_TYPE_HD == dis_tpye)
        return tv_rotate_info[0].screen_init_h_flip;
    else
        return tv_rotate_info[1].screen_init_h_flip;
}

static uint16_t tvtest_get_screen_init_v_flip(int dis_tpye)
{
    if (DIS_TYPE_HD == dis_tpye)
        return tv_rotate_info[0].screen_init_v_flip;
    else
        return tv_rotate_info[1].screen_init_v_flip;
}

static  void tvtest_get_rotate_by_flip_mode(tvtest_project_mode_e mode,
                             int *p_rotate_mode ,
                             int *p_h_flip ,
                             int *p_v_flip)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    
    switch(mode)
    {
        case TVTEST_PROJECT_CEILING_REAR:
        {
            //printf("180\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 0;
            v_flip = 0;
            break;
        }

        case  TVTEST_PROJECT_FRONT:
        {
            //printf("h_mirror\n");
            rotate = ROTATE_TYPE_0;
            h_flip = 1;
            v_flip = 0;
            break;
        }

        case TVTEST_PROJECT_CEILING_FRONT:
        {
           // printf("v_mirror\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 1;
            v_flip = 0;
            break;

        }
        case TVTEST_PROJECT_REAR:
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

static void tvtest_transfer_rotate_mode_for_screen(
                                        int init_rotate,
                                        int init_h_flip,
                                        int init_v_flip,
                                        int *p_rotate_mode ,
                                        int *p_h_flip ,
                                        int *p_v_flip,
                                        int *p_fbdev_rotate)
{
    (void)init_v_flip;
    int fbdev_rotate[4] = { 0,270,180,90 }; //setting is anticlockwise for fvdev
    int rotate = 0 , h_flip = 0 , v_flip = 0;

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

static int tvtest_get_flip_info(tvtest_project_mode_e project_mode, int dis_type, int *rotate_type, int *flip_type)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    
    tvtest_screen_rotate_info();
    int init_rotate = tvtest_get_screen_init_rotate(dis_type);
    int init_h_flip = tvtest_get_screen_init_h_flip(dis_type);
    int init_v_flip = tvtest_get_screen_init_v_flip(dis_type);

    tvtest_get_rotate_by_flip_mode(project_mode,
                            &rotate ,
                            &h_flip ,
                            &v_flip);

    tvtest_transfer_rotate_mode_for_screen(
        init_rotate,init_h_flip,init_v_flip,
        &rotate , &h_flip , &v_flip , NULL);

    *rotate_type = rotate;
    *flip_type = h_flip;
    return 0;
}

static int tvtest_rotate_convert(int rotate_init, int rotate_set)
{
    return ((rotate_init + rotate_set)%4); 
}

static int tvtest_flip_convert(int dis_type, int flip_init, int flip_set)
{
    int rotate = 0;
    int swap;
    int flip_ret = flip_set;

    do {
        if (0 == flip_set){
            flip_ret = flip_init;
            break;
        }
        
        rotate = tvtest_get_screen_init_rotate(dis_type);
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


#ifdef __linux__

static int kshm_fd = -1;
static pthread_t tv_dec_video_read_thread_id = 0;

static void *tv_dec_video_read_thread(void *args)
{
    (void)args;
    int data_size = 0;
    uint8_t *data = NULL;

    //printf("rx_audio_read_thread run\n");
    while (!video_stop_read && kshm_fd >= 0) {
        AvPktHd hdr = {0};
        while (read(kshm_fd, &hdr, sizeof(AvPktHd)) != sizeof(AvPktHd)){
            //printf("read audio hdr from kshm err\n");
            usleep(20*1000);
            if (video_stop_read) {
                goto end;
            }
        }
        printf("pkt size 0x%x, flag %d\n" , (int)hdr.size , (int)hdr.flag);

        if (data_size < hdr.size) {
            data_size = hdr.size;
            if (data) {
                data = realloc(data, data_size);
            } else {
                data = malloc(data_size);
            }
            if (!data) {
                printf("no memory\n");
                goto end;
            }
        }

        while (read(kshm_fd, data, hdr.size) != hdr.size){
            //printf("read audio data from kshm err\n");
            usleep(20*1000);
            if (video_stop_read) {
                goto end;
            }
        }

        printf("adata: 0x%x, 0x%x, 0x%x, 0x%x\n", data[0], data[1], data[2], data[3]);
        usleep(1000);
    }

end:
    if(data)
        free(data);

    (void)args;
    
    return NULL;    
}

static void tv_dec_video_kshm_rec(int fd)
{
    struct kshm_info kshm_hdl = { 0 };

    kshm_fd = open("/dev/kshmdev", O_RDONLY);
    if (kshm_fd < 0) {
        return;
    }

    ioctl(fd , TVDEC_VIDEO_KSHM_ACCESS , &kshm_hdl);
    ioctl(kshm_fd, KSHM_HDL_SET, &kshm_hdl);

    video_stop_read = 0;
    if (pthread_create(&tv_dec_video_read_thread_id, NULL, 
        tv_dec_video_read_thread, NULL)) {
        printf("tv dec kshm recv thread create failed\n");
    }
}

static void tv_dec_video_kshm_wait_stop(void)
{
    if (tv_dec_video_read_thread_id)
        pthread_join(tv_dec_video_read_thread_id, NULL);
    tv_dec_video_read_thread_id = 0;

    if (kshm_fd >= 0)
        close (kshm_fd);
    kshm_fd = -1;
}

#else //else of _linux_

static struct kshm_info video_read_hdl = { 0 };
static struct completion video_read_task_completion;

static void tv_dec_video_read_thread(void *args)
{
    struct kshm_info *hdl = (struct kshm_info *)args;
    AvPktHd hdr = { 0 };
    uint8_t *data = malloc(1024 * 1024);

    printf("video_read_thread run```\n");
    while (!video_stop_read) {
        while (kshm_read(hdl , &hdr , sizeof(AvPktHd)) != sizeof(AvPktHd) && !video_stop_read) {
            //printf("read audio hdr from kshm err\n");
            usleep(20 * 1000);
        }
        printf("pkt size 0x%x, flag %d\n" , (int)hdr.size , (int)hdr.flag);

        data = realloc(data , hdr.size);
        while (kshm_read(hdl , data , hdr.size) != hdr.size && !video_stop_read) {
            //printf("read audio data from kshm err\n");
            usleep(20 * 1000);
        }
        printf("data: 0x%x\n" , (unsigned int)data);
    }
    usleep(1000);

    if (data) {
        free(data);
        data = NULL;
    }
    complete(&video_read_task_completion);
    vTaskSuspend(NULL);
}

static void tv_dec_video_kshm_rec(int fd)
{
    int ret;
    struct kshm_info kshm_hdl;

    ioctl(fd , TVDEC_VIDEO_KSHM_ACCESS , &video_read_hdl);

    init_completion(&video_read_task_completion);
    video_stop_read = 0;
    ret = xTaskCreate(tv_dec_video_read_thread , "video_read_thread" ,
                      0x1000 , &video_read_hdl , portPRI_TASK_HIGH , NULL);
    if (ret != pdTRUE) {
        printf("kshm recv thread create failed\n");
    }

}

static void tv_dec_video_kshm_wait_stop(void)
{
    wait_for_completion_timeout(&video_read_task_completion , 3000);
}

#endif

static int tv_dec_start(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    int real_rotate;
    int real_mirror;
    struct tvdec_display_info dis_info = { 0 };
    struct tvdec_training_params training_params = { 0 };
    int training_times = 10;
    int color_diff_value = 40;
    int dcoffset_min = 0x01;
    int dcoffset_max = 0xfe;

    if (tv_dec_started == true) {
        return 0;
    }

    vpath = TVDEC_VIDEO_TO_DE;
    rotate_mode = ROTATE_TYPE_0;
    mirror_mode = MIRROR_TYPE_NONE;
    tv_dec_traning = 0;

	if (tv_dec_fd < 0)
	{
		tv_dec_fd = open("/dev/tv_decoder" , O_WRONLY);
		if (tv_dec_fd < 0) {
			return -1;
		}
	}

    while((opt = getopt(argc , argv , "v:r:t:m:f:s:d:p:b:l:y:e:c:i:a:")) != EOF)
    {
        switch(opt)
        {
            case 'v':
                vpath = atoi(optarg);
                break;
            case 'r':
                rotate_mode = atoi(optarg);
                break;
            case 'm':
                mirror_mode = atoi(optarg);
                break;
            case 'f':
            {
                project_mode = atoi(optarg);
                if (project_mode < 0 || project_mode > 3){
                    printf("project_mode must be in 0 - 3!\n");
                    return -1;
                }
                break;
            }
            case 't':
                tv_dec_traning = atoi(optarg);
                break;
            case 's':
                stop_mode = atoi(optarg);
                break;
            case 'd':
                dc_offset = atoi(optarg);
                break;
            case 'b':
                brightness = atoi(optarg);
                break;
			case 'l':
               
                if (atoi(optarg) == 0)
                {
                    tv_dis_layer = DIS_LAYER_MAIN;
                    printf("DISPLAY ON MP\n");
                }
                else
                {
                    tv_dis_layer = DIS_LAYER_AUXP;
                    printf("DISPLAY ON AUXP\n");
                }
                break;
            case 'p':
                g_pbp_mode = atoi(optarg);
                g_pbp_mode = g_pbp_mode ? 1 : 0;
                break;
            case 'y':
                if (atoi(optarg) == 0) {
                    tv_dis_type = DIS_TYPE_HD;
                } else {
                    tv_dis_type = DIS_TYPE_UHD;
                }
                break;
           case 'e':
                training_times = atoi(optarg);
                break;
            case 'c': 
                color_diff_value = atoi(optarg);
                break;
            case 'i':
                dcoffset_min = atoi(optarg);
                break;
            case 'a':
                dcoffset_max = atoi(optarg);
                break;
            default:
                break;
        }
    }

    dis_info.dis_type = tv_dis_type;
    dis_info.dis_layer = tv_dis_layer;

    printf("%s(). vpath %d, dis_type: %d, dis_layer: %d\n", __func__,
        vpath, dis_info.dis_type, dis_info.dis_layer);
    ioctl(tv_dec_fd , TVDEC_SET_VIDEO_DATA_PATH , vpath);

    if (vpath == TVDEC_VIDEO_TO_KSHM ||
        vpath == TVDEC_VIDEO_TO_DE_AND_KSHM ||
        vpath == TVDEC_VIDEO_TO_DE_ROTATE_AND_KSHM) {
        tv_dec_video_kshm_rec(tv_dec_fd);
    }

    if (vpath == TVDEC_VIDEO_TO_DE_ROTATE ||
        vpath == TVDEC_VIDEO_TO_DE_ROTATE_AND_KSHM) {

    #if 1
        //get rotate and mirror type by project mode and dts, and overlay
        //the -r -m setting for rotation and mirror type.
        int rotate_type;
        int mirror_type;
        int rotate;
        tvtest_get_flip_info(project_mode, tv_dis_type, &rotate_type, &mirror_type);
        rotate = tvtest_rotate_convert(rotate_type, rotate_mode);
        real_mirror = tvtest_flip_convert(tv_dis_type, mirror_type, mirror_mode);
        real_rotate = rotate % 4;
    #else
        real_mirror = mirror_mode;
        real_rotate = rotate_mode;
    #endif

        printf("rotate_mode = 0x%x\n" , real_rotate);
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_ROTATE_MODE , real_rotate);

        printf("mirror_mode = 0x%x\n" , real_mirror);
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_MIRROR_MODE , real_mirror);

    }
    printf("stop_mode = 0x%x\n" , stop_mode);
    printf("dc_offset = %d\n" , dc_offset);
    printf("brightness = %d\n" , brightness);

    ioctl(tv_dec_fd , TVDEC_SET_VIDEO_STOP_MODE , stop_mode);
    ioctl(tv_dec_fd , TVDEC_SET_DC_OFFSET , dc_offset);
    ioctl(tv_dec_fd , TVDEC_SET_BRIGHTNESS , brightness);
    ioctl(tv_dec_fd , TVDEC_SET_DISPLAY_INFO , &dis_info);
    ioctl(tv_dec_fd , TVDEC_SET_PBP_MODE , g_pbp_mode);
    if (g_preview == true) {
        ioctl(tv_dec_fd , TVDEC_SET_DISPLAY_RECT , &g_dis_rect);
    }

	if(vpath>TVDEC_VIDEO_TO_DE)
	{
		ioctl(tv_dec_fd , TVDEC_SET_WM_PARAM , &wm);
	}

    if (tv_dec_traning == false) {
        ioctl(tv_dec_fd , TVDEC_START , tv_sys);
    } else {
        training_params.training_times = training_times;
        training_params.color_diff_value = color_diff_value;
        training_params.dcoffset_min = dcoffset_min;
        training_params.dcoffset_max = dcoffset_max;
        ioctl(tv_dec_fd , TVDEC_SET_TRAINING_PARAMS , &training_params);
        ioctl(tv_dec_fd , TVDEC_SET_TRAINING_START , tv_sys);

    }
    printf("tv_dec start ok\n");
    tv_dec_started = true;
    return 0;
}


static int tv_dec_stop(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    if (tv_dec_fd >= 0) {
        video_stop_read = 1;
        tv_dec_started = false;

        if (vpath == TVDEC_VIDEO_TO_KSHM ||
            vpath == TVDEC_VIDEO_TO_DE_AND_KSHM ||
            vpath == TVDEC_VIDEO_TO_DE_ROTATE_AND_KSHM) {
            tv_dec_video_kshm_wait_stop();
        }

        ioctl(tv_dec_fd , TVDEC_STOP);
        close(tv_dec_fd);
        tv_dec_fd = -1;
        return 0;
    } else {
        return -1;
    }

}

static int tv_dec_get_video_info(int argc , char *argv[])
{
    (void)argc;
    (void)argv;
    struct tvdec_video_info video_info = { 0 };

    if (tv_dec_fd >= 0) {
        ioctl(tv_dec_fd , TVDEC_GET_VIDEO_INFO , &video_info);
        printf("video_info.tv_sys =%d  width =%d height =%d framerate = %d progressive = %d\n" ,
               video_info.tv_sys ,
               video_info.width ,
               video_info.height ,
               video_info.frame_rate ,
               video_info.b_progressive
              );
        return 0;
    } else {
        return -1;
    }
}

static int tv_dec_get_traning_result(int argc , char *argv[])
{
    (void)argc;
    (void)argv;
    struct tvdec_training_result training_result = { 0 };

    if (tv_dec_fd >= 0) {
        ioctl(tv_dec_fd , TVDEC_GET_TRAINING_RESULT , &training_result);
        printf("training_result.status =%d raining_result.dc_offset_val = 0x%x\n" ,
               training_result.status ,
               training_result.dc_offset_val);
        return 0;
    } else {
        return -1;
    }
}

static int tv_dec_set_brightness(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    int value = 0;

    while ((opt = getopt(argc , argv , "b:")) != EOF) {
        switch (opt) {
            case 'b':
                value = atoi(optarg);
                if (value < 0 || value > 100) {
                    printf("tv_dec_set_brightness parm err %d\n", value);
                    return -1;
                }
                brightness = value;
                printf("brightness = %d\n", brightness);
                break;

            default:
                break;
        }
    }
    if (tv_dec_fd >= 0) {
        ioctl(tv_dec_fd , TVDEC_SET_BRIGHTNESS , brightness);
        return 0;
    } else {
        return -1;
    }

}

static int tv_dec_set_preview(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    if (atoi(argv[1]) < 0 || atoi(argv[1]) > 1920 ||
        atoi(argv[2]) < 0 || atoi(argv[2]) > 1080 ||
        atoi(argv[3]) <= 0 || atoi(argv[3]) + atoi(argv[1]) > 1920 ||
        atoi(argv[4]) <= 0 || atoi(argv[4]) + atoi(argv[2]) > 1080 ||
        atoi(argv[5]) < 0 || atoi(argv[5]) > 1920 ||
        atoi(argv[6]) < 0 || atoi(argv[6]) > 1080 ||
        atoi(argv[7]) <= 0 || atoi(argv[7]) + atoi(argv[5]) > 1920 ||
        atoi(argv[8]) <= 0 || atoi(argv[8]) + atoi(argv[6]) > 1080) {
        printf("invalid args\n");
        printf("0 <= x <= 1920, 0 <= y <= 1080, 0 < x + w <= 1920, 0 < y + h <= 1080\n");
        return -1;
    }
    g_dis_rect.src_rect.x = atoi(argv[1]);
    g_dis_rect.src_rect.y = atoi(argv[2]);
    g_dis_rect.src_rect.w = atoi(argv[3]);
    g_dis_rect.src_rect.h = atoi(argv[4]);
    g_dis_rect.dst_rect.x = atoi(argv[5]);
    g_dis_rect.dst_rect.y = atoi(argv[6]);
    g_dis_rect.dst_rect.w = atoi(argv[7]);
    g_dis_rect.dst_rect.h = atoi(argv[8]);
    g_preview = true;
    if (tv_dec_fd > 0) {
        ioctl(tv_dec_fd , TVDEC_SET_DISPLAY_RECT , &g_dis_rect);
    }
    return 0;
}

static int tv_dec_set_rotate(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;

    if (argc < 3){
        printf("%s(), argc is error!\n", __func__);
        return 0;
    }

    while ((opt = getopt(argc , argv , "r:m:f:")) != EOF) {
        switch (opt) {
            case 'r':
                rotate_mode = atoi(optarg);
                break;
            case 'm':
                mirror_mode = atoi(optarg);
                mirror_mode = mirror_mode ? 1 : 0;
                break;
            case 'f':
            {
                project_mode = atoi(optarg);
                if (project_mode < 0 || project_mode > 3){
                    printf("project_mode must be in 0 - 3!\n");
                    return -1;
                }
                break;
            }

            default:
                break;
        }
    }

    if (tv_dec_fd > 0) {

        int real_rotate;
        int real_mirror;

    #if 1
        //get rotate and mirror type by project mode and dts, and overlay
        //the -r -m setting for rotation and mirror type.
        int rotate_type;
        int mirror_type;
        int rotate;
        tvtest_get_flip_info(project_mode, tv_dis_type, &rotate_type, &mirror_type);
        rotate = tvtest_rotate_convert(rotate_type, rotate_mode);
        real_mirror = tvtest_flip_convert(tv_dis_type, mirror_type, mirror_mode);
        real_rotate = rotate % 4;
    #else
        real_rotate = rotate_mode;
        real_mirror = mirror_mode;
    #endif        
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_DATA_PATH , TVDEC_VIDEO_TO_DE_ROTATE);
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_ROTATE_MODE , real_rotate);
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_MIRROR_MODE , real_mirror);
    }

    return 0;
}


static int tv_dec_set_wm(int argc , char *argv[])
{
    int ret = -1;
    int opt;
    opterr = 0;
    optind = 0;

	
	if (tv_dec_fd < 0)
	{
		tv_dec_fd = open("/dev/tv_decoder" , O_WRONLY);
		if (tv_dec_fd < 0) {
			return -1;
		}
	}

    while ((opt = getopt(argc , argv , "e:x:y:a:b:c:")) != EOF) {
        switch (opt) {
            case 'e':
                wm.enable = atoi(optarg);
                break;
			case 'x':
				wm.x = atoi(optarg);
				break;
			case 'y':
				wm.y = atoi(optarg);
				break;
			case 'a':
				wm.color_y =  strtoll(optarg , NULL , 16);
				break;
			case 'b':
				wm.color_u =  strtoll(optarg , NULL , 16);
				break;
			case 'c':
				wm.color_v =  strtoll(optarg , NULL , 16);
				break;
			case 'f':
				wm.date_fmt = atoi(optarg);
				break;
            default:
                break;
        }
    }
    if (tv_dec_fd >= 0) {
        ret = ioctl(tv_dec_fd , TVDEC_SET_WM_PARAM , &wm);
        if (ret < 0) {
            printf("hdmi_rx_set_wm\n");
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}

int tv_dec_enter(int argc , char *argv[])
{
    (void)argc;
    (void)argv;


    return 0;
}

#ifdef __HCRTOS__
int tv_dec_dump_rawdata(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    long buf_addr_y = 0;
    long buf_addr_c = 0;
    int len = 736 * 576;
    int cap_times = 0;

    char m_y_file_name[256] = {0};
    char m_c_file_name[256] = {0};
    FILE *y_fd = 0;
    FILE *c_fd = 0;
    long long tmp = 0;
    char path[32] = "/media/sda1/", *filename_mark = "cvbs";

    struct tvdec_video_info video_info = { 0 };
    if (tv_dec_fd >= 0) {
        ioctl(tv_dec_fd , TVDEC_GET_VIDEO_INFO , &video_info);
        printf("dump vidinfo %d w:%d h:%d fr:%d prog:%d\n",
               video_info.tv_sys ,
               video_info.width ,
               video_info.height ,
               video_info.frame_rate ,
               video_info.b_progressive
              );
        len = video_info.width * video_info.height;
    }

    while ((opt = getopt(argc , argv , "f:s:t:")) != EOF) {
        switch (opt) {
            case 'f':
                filename_mark = optarg;
                break;
            case 's':
                tmp = strtoll(optarg , NULL , 16);
                len = (unsigned long)tmp;
                break;
            case 't':
                tmp = strtoll(optarg , NULL , 10);
                cap_times = (unsigned long)tmp;
                break;
            default:
                break;
        }
    }
    printf("cap times:%d,len:%d\n", cap_times, len);
    for (int i = 0; i < cap_times; i++) {
        buf_addr_y = (*(volatile unsigned long *)(0xb8864050) | 0xa0000000);
        buf_addr_c = (*(volatile unsigned long *)(0xb8864054) | 0xa0000000);

        memset(m_y_file_name, 0, sizeof(m_y_file_name));
        memset(m_c_file_name, 0, sizeof(m_c_file_name));

        sprintf(m_y_file_name, "%s%s_y%d.bin", path, filename_mark, i);
        sprintf(m_c_file_name, "%s%s_c%d.bin", path, filename_mark, i);

        y_fd = fopen(m_y_file_name , "wb");
        c_fd = fopen(m_c_file_name , "wb");

        if (y_fd == NULL || c_fd == NULL) {
            printf("file null:%lx,%lx,%s,%s\n", (long)y_fd, (long)c_fd, m_y_file_name, m_c_file_name);
            continue;
        }

        printf("addr_y = 0x%lx len = %d %s\n" , buf_addr_y , len, m_y_file_name);
        if (buf_addr_y != 0) {
            fwrite((char *)buf_addr_y , 1 , len , y_fd);
            fsync((int)y_fd);
            fclose(y_fd);
        }

        printf("addr_c = 0x%x len = %d %s\n" , (int)buf_addr_c , len, m_c_file_name);
        if (buf_addr_c != 0) {
            fwrite((char *)buf_addr_c , 1 , len , c_fd);
            fsync((int)c_fd);
            fclose(c_fd);
        }
    }

    return 0;
}
#endif

static int tv_dec_set_training_params(int argc , char *argv[])
{
    struct tvdec_training_params training_params = { 0 };
    int opt;
    opterr = 0;
    optind = 0;

    int training_times = 10;
    int color_diff_value = 40;
    int dcoffset_min = 0x01;
    int dcoffset_max = 0xfe;

    while ((opt = getopt(argc , argv , "e:c:i:a:")) != EOF) {
        switch (opt) {
            case 'e':
                training_times = atoi(optarg);
                break;
            case 'c': 
                color_diff_value = atoi(optarg);
                break;
            case 'i':
                dcoffset_min = atoi(optarg);
                break;
            case 'a':
                dcoffset_max = atoi(optarg);
                break;
        }
    }
    if (tv_dec_fd >= 0) {
        training_params.training_times = training_times;
        training_params.color_diff_value = color_diff_value;
        training_params.dcoffset_min = dcoffset_min;
        training_params.dcoffset_max = dcoffset_max;
        ioctl(tv_dec_fd , TVDEC_SET_TRAINING_PARAMS , &training_params);
        return 0;
    } else {
        printf("cvbs dec not open!\n");
        return -1;
    }
}

const char help_start[] = 
    "start CVBS in test\n\t\t\t"
    "start -v 1 -p 1 -y 0 -l 0 -f 1\n\t\t\t"
    "-v path: TVDEC_VIDEO_DATA_PATH. 0, no rotate; 1, support rotate\n\t\t\t"
    "-p multi display enable: 0, disable; 1, enable\n\t\t\t"
    "-y set dis type: 0, DIS HD; 1. DIS UHD\n\t\t\t"
    "-l set dis layer: 0, DIS main layer; 1. DIS auxp layer\n\t\t\t"
    "-r set rotate mode: 0, degree; 1, 90 degree; 2, 180 degree; 3, 180 degree\n\t\t\t"
    "-m set mirror mode: 0, no flip; 1, horizonal filp\n\t\t\t"
    "-f set project mode: 0, Front; 1, Ceiling Front; 2, Rear; 3, Ceiling Rear\n\t"
    "-e training times -c color diff vale -i dcoffsetmin -a dcoffsetmax\n\t"
;

const char help_preview[] = 
    "preview src dst, base on 1920*1080\n\t\t\t"
    "for example: preview 0 0 1920 1080  800 450 320 180\n\t"
;

const char help_rotate[] = 
    "rotate -r <rotage_angle> -m <horizonal> -f <project mode>\n\t\t\t"
    "-r set rotate mode: 0, degree; 1, 90 degree; 2, 180 degree; 3, 180 degree\n\t\t\t"
    "-m set mirror mode: 0, no flip; 1, horizonal filp\n\t\t\t"
    "-f set project mode: 0, Front; 1, Ceiling Front; 2, Rear; 3, Ceiling Rear\n\t"
;

const char help_dump[] = "dump_rawdata -f filename_mark -s size -t times";

const char help_training[] = 
    "set_training_params -e training times -c color diff vale -i dcoffsetmin -a dcoffsetmax";

static const char help_wm[] = 
	"wm -e <enable> -x <h start pos> -y <h start pos> -a color_y -b color_u -c color_v -f <date format>\n\t\t\t"
;

#ifdef __linux__

void tv_dec_cmds_register(struct console_cmd *cmd)
{
    console_register_cmd(cmd, "start", tv_dec_start, CONSOLE_CMD_MODE_SELF, help_start);
    console_register_cmd(cmd, "stop", tv_dec_stop, CONSOLE_CMD_MODE_SELF, "stop tv_dec");
    console_register_cmd(cmd, "get_video_info", tv_dec_get_video_info, CONSOLE_CMD_MODE_SELF, "get_video_info");
    console_register_cmd(cmd, "get_traning_result", tv_dec_get_traning_result, CONSOLE_CMD_MODE_SELF, "traning_result");
    console_register_cmd(cmd, "set_brighness", tv_dec_set_brightness, CONSOLE_CMD_MODE_SELF, "set_brighness");
    console_register_cmd(cmd, "preview", tv_dec_set_preview, CONSOLE_CMD_MODE_SELF, help_preview);
    //console_register_cmd(cmd, "dump_rawdata", tv_dec_dump_rawdata, CONSOLE_CMD_MODE_SELF, help_dump);
    console_register_cmd(cmd, "rotate", tv_dec_set_rotate, CONSOLE_CMD_MODE_SELF, help_rotate);
    console_register_cmd(cmd, "set_training_params", tv_dec_set_training_params, CONSOLE_CMD_MODE_SELF, help_training);
}


#ifndef BR2_PACKAGE_VIDEO_PBP_EXAMPLES
static struct termios stored_settings;
static void exit_console(int signo)
{
    (void)signo;
    tv_dec_stop(0, 0);
    tcsetattr (0, TCSANOW, &stored_settings);
    exit(0);
}


int main(int argc , char *argv[])
{
    (void)argc;
    (void)argv;
    struct termios new_settings;

    tcgetattr(0, &stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &new_settings);

    signal(SIGTERM, exit_console);
    signal(SIGINT, exit_console);
    signal(SIGSEGV, exit_console);
    signal(SIGBUS, exit_console);
    console_init("tv_dec:");
    
    tv_dec_cmds_register(NULL);

    console_start();
    exit_console(0);
    return 0;

}
#endif

#else

CONSOLE_CMD(tv_dec , NULL , tv_dec_enter , CONSOLE_CMD_MODE_SELF , "enter tv dec")
CONSOLE_CMD(start , "tv_dec" , tv_dec_start , CONSOLE_CMD_MODE_SELF, help_start )
CONSOLE_CMD(stop , "tv_dec" , tv_dec_stop , CONSOLE_CMD_MODE_SELF , "stop tv_dec")
CONSOLE_CMD(get_video_info , "tv_dec" , tv_dec_get_video_info , CONSOLE_CMD_MODE_SELF , "get_video_info")
CONSOLE_CMD(get_traning_result , "tv_dec" , tv_dec_get_traning_result , CONSOLE_CMD_MODE_SELF , "traning_result")
CONSOLE_CMD(set_brighness , "tv_dec" , tv_dec_set_brightness , CONSOLE_CMD_MODE_SELF , "set_brighness")
CONSOLE_CMD(preview , "tv_dec" , tv_dec_set_preview , CONSOLE_CMD_MODE_SELF, help_preview)
CONSOLE_CMD(dump_rawdata , "tv_dec" , tv_dec_dump_rawdata, CONSOLE_CMD_MODE_SELF, help_dump)
CONSOLE_CMD(rotate , "tv_dec" , tv_dec_set_rotate , CONSOLE_CMD_MODE_SELF , help_rotate)
CONSOLE_CMD(set_training_params, "tv_dec" , tv_dec_set_training_params , CONSOLE_CMD_MODE_SELF , help_training)
CONSOLE_CMD(wm , "tv_dec" , tv_dec_set_wm , CONSOLE_CMD_MODE_SELF , help_wm)
#endif
