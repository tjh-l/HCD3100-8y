#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#ifdef __linux__
#include <termios.h>
#include <signal.h>
#include "console.h"
#include <autoconf.h>
#else
#include <generated/br2_autoconf.h>
#include <kernel/io.h>
#include <kernel/lib/console.h>
#include <kernel/completion.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#include <kernel/lib/crc32.h>
#include <kernel/module.h>
#include <kernel/vfs.h>
#endif

#include <stdio.h>
#include <getopt.h>
#include <malloc.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/hdmi_rx.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/dis.h>
#include <hcuapi/viddec.h>
#include <hcuapi/hdmi_cec.h>
#include <stdlib.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/snd.h>
#include <hcuapi/audsink.h>
#include <stdlib.h>
#include <hcuapi/gpio.h>
#include <sys/mount.h>
#include "hdmi_cec.h"
#include "../wav.h"

//#define HDMI_RX_TEST_DUMP_I2SO_DATA

static pthread_t m_cec_thread_id = 0;
static pthread_t m_audio_thread_id = 0;
static pthread_t m_video_thread_id = 0;

static int vkshm_fd = -1;
static int akshm_fd = -1;

static bool audio_stop_read = 0;
static bool video_stop_read = 0;
static bool cec_stop_read = 0;

static struct kshm_info audio_read_hdl = { 0 };
//static struct kshm_info video_read_hdl = { 0 };
static enum HDMI_RX_VIDEO_DATA_PATH vpath = HDMI_RX_VIDEO_TO_OSD;
static enum HDMI_RX_AUDIO_DATA_PATH apath = HDMI_RX_AUDIO_BYPASS_TO_HDMI_TX;
static int rx_fd = -1;
static unsigned int rotate_mode = ROTATE_TYPE_0;
static unsigned int mirror_mode = MIRROR_TYPE_NONE;
static int project_mode = 0; //0: PROJECT_REAR; 1: PROJECT_CEILING_REAR; 2: PROJECT_FRONT; 3: PROJECT_CEILING_FRONT
static unsigned int stop_mode = 0;
static unsigned int set_edid = false;
static unsigned char *buf_yuv444 = NULL;
static hdmi_rx_edid_data_t hdmi_rx_edid;
//hdmi_rx_hdcp_key_t hdmi_rx_hdcp;
static jpeg_enc_quality_type_e hdmi_rx_quality_type = JPEG_ENC_QUALITY_TYPE_NORMAL;
static enum DIS_LAYER hrx_dis_layer = DIS_LAYER_MAIN;
static dis_type_e hrx_dis_type = DIS_TYPE_HD;
static int hdmi_rx_set_enc_table(void);
static vdec_dis_rect_t g_dis_rect = { {0,0,1920,1080},{0,0,1920,1080} };
static unsigned int g_preview = false;
static video_pbp_mode_e g_pbp_mode = 0;
static uint8_t g_cec_logic_addr = 0x0;
static bool g_b_start = false;

static uint8_t edid_data[] =
{
#if 1 //nativ 1080P
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0d, 0xc3, 0x20, 0x03, 0x0a, 0x1a, 0x00, 0x00,
    0x18, 0x1a, 0x01, 0x04, 0xa2, 0x46, 0x27, 0x78, 0xfa, 0xd6, 0xa5, 0xa2, 0x59, 0x4a, 0x95, 0x24,
    0x14, 0x50, 0x54, 0xaf, 0xcf, 0x00, 0x81, 0x00, 0x81, 0x40, 0x71, 0x40, 0xa9, 0xc0, 0x81, 0x80,
    0x81, 0xc0, 0x95, 0x00, 0xb3, 0x00, 0xab, 0x22, 0xa0, 0xa0, 0x50, 0x84, 0x1a, 0x30, 0x30, 0x20,
    0x36, 0x00, 0xb0, 0x0e, 0x11, 0x00, 0x00, 0x1e, 0x66, 0x21, 0x50, 0xb0, 0x51, 0x00, 0x1a, 0x30,
    0x40, 0x70, 0x36, 0x00, 0xb0, 0xf3, 0x10, 0x00, 0x00, 0x1e, 0x66, 0x21, 0x56, 0xaa, 0x51, 0x00,
    0x1e, 0x30, 0x46, 0x8f, 0x33, 0x00, 0xaa, 0xef, 0x10, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xfc,
    0x00, 0x48, 0x44, 0x4d, 0x49, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x0d,
    0x02, 0x03, 0x23, 0x70, 0x4d, 0x05, 0x03, 0x04, 0x07, 0x90, 0x01, 0x20, 0x12, 0x13, 0x14, 0x22,
    0x16, 0x20, 0x23, 0x09, 0x07, 0x07, 0x83, 0x4f, 0x00, 0x00, 0x68, 0x03, 0x0c, 0x00, 0x10, 0x00,
    0x80, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x95,
#else //nativ 720p
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x0d, 0xc3, 0x20, 0x03, 0x0a, 0x1a, 0x00, 0x00,
    0x18, 0x1a, 0x01, 0x04, 0xa2, 0x46, 0x27, 0x78, 0xfa, 0xd6, 0xa5, 0xa2, 0x59, 0x4a, 0x95, 0x24,
    0x14, 0x50, 0x54, 0xaf, 0xcf, 0x00, 0x81, 0x00, 0x81, 0x40, 0x71, 0x40, 0xa9, 0xc0, 0x81, 0x80,
    0x81, 0xc0, 0x95, 0x00, 0xb3, 0x00, 0xab, 0x22, 0xa0, 0xa0, 0x50, 0x84, 0x1a, 0x30, 0x30, 0x20,
    0x36, 0x00, 0xb0, 0x0e, 0x11, 0x00, 0x00, 0x1e, 0x66, 0x21, 0x50, 0xb0, 0x51, 0x00, 0x1a, 0x30,
    0x40, 0x70, 0x36, 0x00, 0xb0, 0xf3, 0x10, 0x00, 0x00, 0x1e, 0x66, 0x21, 0x56, 0xaa, 0x51, 0x00,
    0x1e, 0x30, 0x46, 0x8f, 0x33, 0x00, 0xaa, 0xef, 0x10, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xfc,
    0x00, 0x48, 0x44, 0x4d, 0x49, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x0d,
    0x02, 0x03, 0x23, 0x70, 0x4d, 0x05, 0x03, 0x84, 0x07, 0x10, 0x01, 0x20, 0x12, 0x13, 0x14, 0x22,
    0x16, 0x20, 0x23, 0x09, 0x07, 0x07, 0x83, 0x4f, 0x00, 0x00, 0x68, 0x03, 0x0c, 0x00, 0x10, 0x00,
    0x80, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x95,
#endif
};

static uint8_t decoder_quant_table_y[64] =
{
    6       ,3      ,3      ,6      ,9      ,15 ,18 ,21,
    3       ,3      ,6      ,6      ,9      ,21 ,21 ,21,
    6       ,6      ,6      ,9      ,15     ,21 ,24 ,21,
    6       ,6      ,9      ,9      ,18     ,30 ,30 ,21,
    6       ,9      ,12     ,21     ,24     ,39 ,36 ,27,
    9       ,12     ,21     ,24     ,30     ,36 ,42 ,33,
    18      ,24     ,27     ,30     ,36     ,45 ,42 ,36,
    27      ,33     ,33     ,36     ,39     ,36 ,36 ,36,
};

static uint8_t decoder_quant_table_c[64] =
{
6       ,6      ,9      ,18 ,36 ,36 ,36 ,36,
6       ,9      ,9      ,24 ,36 ,36 ,36 ,36,
9       ,9      ,21     ,36 ,36 ,36 ,36 ,36,
18      ,24     ,36     ,36 ,36 ,36 ,36 ,36,
36      ,36     ,36     ,36 ,36 ,36 ,36 ,36,
36      ,36     ,36     ,36 ,36 ,36 ,36 ,36,
36      ,36     ,36     ,36 ,36 ,36 ,36 ,36,
36      ,36     ,36     ,36 ,36 ,36 ,36 ,36,
};

static uint16_t encoder_q_table_y[64] =
{
    0x1555, 0x0d55, 0x0d55, 0x1555, 0x0d55, 0x0d55, 0x1555, 0x1555,
    0x1555, 0x1555, 0x1555, 0x1555, 0x1555, 0x1555, 0x1f1c, 0x1c44,
    0x1f1c, 0x1f1c, 0x1f1c, 0x1f1c, 0x1f1c, 0x271c, 0x1d55, 0x1d55,
    0x1f1c, 0x1c44, 0x2618, 0x271c, 0x2618, 0x2618, 0x2618, 0x271c,
    0x2618, 0x2618, 0x2555, 0x24be, 0x2fc2, 0x24be, 0x2555, 0x2555,
    0x2444, 0x2555, 0x2618, 0x2618, 0x2444, 0x2e90, 0x2444, 0x2444,
    0x2fc2, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2618, 0x24be, 0x2e18,
    0x2db0, 0x2e90, 0x2f1c, 0x2e18, 0x2fc2, 0x2f1c, 0x2f1c, 0x2f1c,
};

static uint16_t encoder_q_table_c[64] =
{
    0x1555, 0x1555, 0x1555, 0x1f1c, 0x1f1c, 0x1f1c, 0x271c, 0x1f1c,
    0x1f1c, 0x271c, 0x2f1c, 0x2555, 0x2618, 0x2555, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
    0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c, 0x2f1c,
};

//////////////////////////////////////////////////////////////
//following apis is for transfering project mode to ration and flip
#if 1

typedef enum{
    HRXTEST_PROJECT_REAR = 0,
    HRXTEST_PROJECT_CEILING_REAR,
    HRXTEST_PROJECT_FRONT,
    HRXTEST_PROJECT_CEILING_FRONT,

    HRXTEST_PROJECT_MODE_MAX
}hrxtest_project_mode_e;

typedef struct{
    uint16_t screen_init_rotate;
    uint16_t screen_init_h_flip;
    uint16_t screen_init_v_flip;
}hrx_rotate_cfg_t;

static hrx_rotate_cfg_t hrx_rotate_info[2];

#ifdef __linux__
static void hrxtest_dts_string_get(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);
    if(fd >= 0){
        read(fd, string, size);
        close(fd);
    }
}

static int hrxtest_dts_uint32_get(const char *path)
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

static void hrxtest_screen_rotate_info(void)
{
    static int init_flag = 0;
    if (init_flag)
        return;

    unsigned int rotate=0,h_flip=0,v_flip=0;
#ifdef __HCRTOS__
    int np;

    memset(hrx_rotate_info, 0, sizeof(hrx_rotate_info));
    np = fdt_node_probe_by_path("/hcrtos/rotate");
    if(np>=0)
    {
        fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
        fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
        fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
        hrx_rotate_info[0].screen_init_rotate = rotate;
        hrx_rotate_info[0].screen_init_h_flip = h_flip;
        hrx_rotate_info[0].screen_init_v_flip = v_flip;
    }

    np = fdt_node_probe_by_path("/hcrtos/rotate_4k");
    if(np>=0)
    {
        fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
        fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
        fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
        hrx_rotate_info[1].screen_init_rotate = rotate;
        hrx_rotate_info[1].screen_init_h_flip = h_flip;
        hrx_rotate_info[1].screen_init_v_flip = v_flip;
    }

#else
#define ROTATE_CONFIG_PATH "/proc/device-tree/hcrtos/rotate"
#define ROTATE_4K_CONFIG_PATH "/proc/device-tree/hcrtos/rotate_4k"
    char status[16] = {0};
    memset(hrx_rotate_info, 0, sizeof(hrx_rotate_info));

    hrxtest_dts_string_get(ROTATE_CONFIG_PATH "/status", status, sizeof(status));
    if(!strcmp(status, "okay")){
        rotate = hrxtest_dts_uint32_get(ROTATE_CONFIG_PATH "/rotate");
        h_flip = hrxtest_dts_uint32_get(ROTATE_CONFIG_PATH "/h_flip");
        v_flip = hrxtest_dts_uint32_get(ROTATE_CONFIG_PATH "/v_flip");
        hrx_rotate_info[0].screen_init_rotate = rotate;
        hrx_rotate_info[0].screen_init_h_flip = h_flip;
        hrx_rotate_info[0].screen_init_v_flip = v_flip;
    }

    hrxtest_dts_string_get(ROTATE_4K_CONFIG_PATH "/status", status, sizeof(status));
    if(!strcmp(status, "okay")){
        rotate = hrxtest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/rotate");
        h_flip = hrxtest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/h_flip");
        v_flip = hrxtest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/v_flip");
        hrx_rotate_info[1].screen_init_rotate = rotate;
        hrx_rotate_info[1].screen_init_h_flip = h_flip;
        hrx_rotate_info[1].screen_init_v_flip = v_flip;
    }

#endif
    init_flag = 1;
    printf("%s()->>> 2k init_rotate = %u h_flip %u v_flip = %u\n", __func__,
        hrx_rotate_info[0].screen_init_rotate,
        hrx_rotate_info[0].screen_init_h_flip,
        hrx_rotate_info[0].screen_init_v_flip);
    printf("%s()->>> 4k init_rotate = %u h_flip %u v_flip = %u\n", __func__, 
        hrx_rotate_info[1].screen_init_rotate,
        hrx_rotate_info[1].screen_init_h_flip,
        hrx_rotate_info[1].screen_init_v_flip);

}


static uint16_t hrxtest_get_screen_init_rotate(int dis_tpye)
{
    if (DIS_TYPE_HD == dis_tpye)
        return hrx_rotate_info[0].screen_init_rotate;
    else
        return hrx_rotate_info[1].screen_init_rotate;
}

static uint16_t hrxtest_get_screen_init_h_flip(int dis_tpye)
{
    if (DIS_TYPE_HD == dis_tpye)
        return hrx_rotate_info[0].screen_init_h_flip;
    else
        return hrx_rotate_info[1].screen_init_h_flip;
}

static uint16_t hrxtest_get_screen_init_v_flip(int dis_tpye)
{
    if (DIS_TYPE_HD == dis_tpye)
        return hrx_rotate_info[0].screen_init_v_flip;
    else
        return hrx_rotate_info[1].screen_init_v_flip;
}

static  void hrxtest_get_rotate_by_flip_mode(hrxtest_project_mode_e mode,
                             int *p_rotate_mode ,
                             int *p_h_flip ,
                             int *p_v_flip)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    
    switch(mode)
    {
        case HRXTEST_PROJECT_CEILING_REAR:
        {
            //printf("180\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 0;
            v_flip = 0;
            break;
        }

        case  HRXTEST_PROJECT_FRONT:
        {
            //printf("h_mirror\n");
            rotate = ROTATE_TYPE_0;
            h_flip = 1;
            v_flip = 0;
            break;
        }

        case HRXTEST_PROJECT_CEILING_FRONT:
        {
           // printf("v_mirror\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 1;
            v_flip = 0;
            break;

        }
        case HRXTEST_PROJECT_REAR:
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

static void hrxtest_transfer_rotate_mode_for_screen(
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

static int hrxtest_get_flip_info(hrxtest_project_mode_e project_mode, int dis_type, int *rotate_type, int *flip_type)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    
    hrxtest_screen_rotate_info();
    int init_rotate = hrxtest_get_screen_init_rotate(dis_type);
    int init_h_flip = hrxtest_get_screen_init_h_flip(dis_type);
    int init_v_flip = hrxtest_get_screen_init_v_flip(dis_type);

    hrxtest_get_rotate_by_flip_mode(project_mode,
                            &rotate ,
                            &h_flip ,
                            &v_flip);

    hrxtest_transfer_rotate_mode_for_screen(
        init_rotate,init_h_flip,init_v_flip,
        &rotate , &h_flip , &v_flip , NULL);

    *rotate_type = rotate;
    *flip_type = h_flip;
    return 0;
}

static int hrxtest_rotate_convert(int rotate_init, int rotate_set)
{
    return ((rotate_init + rotate_set)%4);    
}

static int hrxtest_flip_convert(int dis_type, int flip_init, int flip_set)
{
    int rotate = 0;
    int swap;
    int flip_ret = flip_set;

    do {
        if (0 == flip_set){
            flip_ret = flip_init;
            break;
        }
        
        rotate = (int)hrxtest_get_screen_init_rotate(dis_type);
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

#ifdef __HCRTOS__

static void *audio_read_thread(void *args)
{
    struct kshm_info *hdl = (struct kshm_info *)args;
    AvPktHd hdr = { 0 };
    uint8_t *data = malloc(1024 * 1024);

#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
    FILE *recfile = fopen("/media/sda1/arec_file.wav" , "wb+");
    printf("recfile %p\n" , recfile);
    struct wave_header header = { 0 };
    generate_wave_header(&header , 44100 , 16 , 2);
    if (recfile)
        fwrite(&header , sizeof(struct wave_header) , 1 , recfile);
#endif

    printf("audio_read_thread run```\n");
    while (!audio_stop_read) {
        while (kshm_read(hdl , &hdr , sizeof(AvPktHd)) != sizeof(AvPktHd) && !audio_stop_read) {
            usleep(20 * 1000);
        }

        data = realloc(data , hdr.size);
        while (kshm_read(hdl , data , hdr.size) != hdr.size && !audio_stop_read) {
            usleep(20 * 1000);
        }

#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
        if (recfile)
            fwrite(data , hdr.size , 1 , recfile);
#endif
    }
    usleep(1000);

    if (data)
        free(data);

#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
    if (recfile)
        fclose(recfile);
#endif

}

static void *video_read_thread(void *args)
{
    struct kshm_info *hdl;
    struct kshm_info kshm_hdl;
    AvPktHd hdr = { 0 };
    uint32_t crc = 0;
    uint8_t flag0 = 0 , flag1 = 0;
    uint8_t *data;

    ioctl(rx_fd , HDMI_RX_VIDEO_KSHM_ACCESS , &kshm_hdl);
    hdl = &kshm_hdl;

    data = malloc(1024 * 1024);

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

#ifdef CONFIG_CMDS_HDMI_RX_SORTING
        crc = crc32(0 , data , 0x9a21);
        printf("crc = 0x%lx\n" , crc);

        if (crc == 0x2b0f0f40) {
            flag0++;
            flag1 = 0;
        } else {
            flag1++;
            flag0 = 0;
        }

        if (flag0 >= 3) {
            gpio_set_output(PINPAD_L01 , 1);
            gpio_set_output(PINPAD_L02 , 1);
            printf("test pass\n");
            while (1);
        }

        if (flag1 >= 3) {
            gpio_set_output(PINPAD_L01 , 0);
            gpio_set_output(PINPAD_L02 , 1);
            printf("test fail\n");
            while (1);
        }
#endif
    }
    usleep(1000);

    if (data)
        free(data);

}


#else //else of __HCRTOS__

static void *audio_read_thread(void *args)
{
    (void)args;
    AvPktHd hdr = { 0 };
    uint8_t *data = malloc(1024 * 1024);

#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
    FILE *recfile = fopen("/media/hdd/arec_file.wav" , "wb+");
    printf("arecfile %p\n" , recfile);
    struct wave_header header = { 0 };
    generate_wave_header(&header , 44100 , 16 , 2);
    if (recfile)
        fwrite(&header , sizeof(struct wave_header) , 1 , recfile);
#endif

    printf("audio_read_thread run```\n");
    while (!audio_stop_read) 
    {
        while(read(akshm_fd , &hdr , sizeof(AvPktHd)) != sizeof(AvPktHd) && !audio_stop_read)
        {
            usleep(20 * 1000);
        }

        data = realloc(data , hdr.size);
        while(read(akshm_fd , data , hdr.size) != hdr.size && !audio_stop_read)
        {
            usleep(20 * 1000);
        }

#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
        if (recfile)
            fwrite(data , hdr.size , 1 , recfile);
#endif
    }
    usleep(1000);

    if (data)
        free(data);

#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
    if (recfile)
        fclose(recfile);
#endif

    return NULL;
}

static void *video_read_thread(void *args)
{
    (void)args;
    struct kshm_info kshm_hdl;
    AvPktHd hdr = { 0 };
    uint8_t *data;

#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
    FILE *recfile = fopen("/media/hdd/vrec_file" , "wb+");
    printf("vrecfile %p\n" , recfile);
#endif

    vkshm_fd = open("/dev/kshmdev" , O_RDONLY);
    if(vkshm_fd < 0)
    {
        return NULL;
    }

    ioctl(rx_fd , HDMI_RX_VIDEO_KSHM_ACCESS , &kshm_hdl);
    ioctl(vkshm_fd , KSHM_HDL_SET , &kshm_hdl);

    data = malloc(1024 * 1024);

    printf("video_read_thread run```\n");
    while (!video_stop_read) 
    {

        while(read(vkshm_fd, &hdr, sizeof(AvPktHd)) != sizeof(AvPktHd) && !video_stop_read)
        {
            //printf("read audio hdr from kshm err\n");
            usleep(20 * 1000);
        }
        printf("pkt size 0x%x, flag %d\n" , (int)hdr.size , (int)hdr.flag);

        data = realloc(data , hdr.size);
        while (read(vkshm_fd , data , hdr.size) != hdr.size && !video_stop_read) {
            usleep(20 * 1000);
        }

        //printf("vdata: 0x%x, 0x%x, 0x%x, 0x%x\n", data[0], data[1], data[2], data[3]);
    #ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
        if(recfile)
        {
            fwrite(data , hdr.size , 1 , vrecfile);
        }
    #endif
        usleep(1000);
    }

    if (data)
        free(data);

#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
    if (recfile)
        fclose(recfile);
#endif

    return NULL;
}


#endif

static void hdmi_rx_start_audio_thread(void)
{
    if (apath >= HDMI_RX_AUDIO_TO_I2SI_AND_KSHM) {

        struct kshm_info rx_audio_read_hdl = { 0 };
        int ret = -1;
        if (apath == HDMI_RX_AUDIO_TO_SPDIF_IN_AND_I2SO ||
            apath == HDMI_RX_AUDIO_TO_I2SI_AND_I2SO) {
#ifdef HDMI_RX_TEST_DUMP_I2SO_DATA
            int i2so_fd = open("/dev/sndC0i2so" , O_WRONLY);
            if (i2so_fd >= 0) {
                ret = ioctl(i2so_fd , SND_IOCTL_SET_RECORD , 300 * 1024);
                printf("SND_IOCTL_SET_RECORD ret %d\n" , ret);
                ret |= ioctl(i2so_fd , KSHM_HDL_ACCESS , &audio_read_hdl);
                printf("KSHM_HDL_ACCESS ret %d\n" , ret);
                close(i2so_fd);
            }
#endif
        } else {
        #ifdef __linux__
            akshm_fd = open("/dev/kshmdev" , O_RDONLY);
            if(akshm_fd < 0)
                return;
        #endif
            ret = ioctl(rx_fd , HDMI_RX_AUDIO_KSHM_ACCESS , &audio_read_hdl);

        #ifdef __linux__
            ioctl(akshm_fd , KSHM_HDL_SET , &rx_audio_read_hdl);
        #endif
        }

        if (ret < 0) {
            printf("get audio kshm handle failed\n");
            return;
        }
        audio_stop_read = 0;

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x1000);
        pthread_create(&m_audio_thread_id, &attr, audio_read_thread, (void*)&audio_read_hdl);
        pthread_attr_destroy(&attr);
    }
}


static void hdmi_rx_start_video_thread(void)
{
    if (vpath == HDMI_RX_VIDEO_TO_KSHM ||
        vpath == HDMI_RX_VIDEO_TO_DE_AND_KSHM ||
        vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM) 
    {

        video_stop_read = 0;

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x1000);
        pthread_create(&m_video_thread_id, &attr, video_read_thread, NULL);
        pthread_attr_destroy(&attr);
    }
}


static void *cec_read_thread(void *args)
{
    (void)args;
    unsigned int i = 0;

    while (!cec_stop_read) {
        hdmi_cec_info_t cec_info = { 0 };
        if (rx_fd >= 0) {
            if (ioctl(rx_fd , HDMI_CEC_GET_CMD , &cec_info) == 0) {
                printf("GET CEC CMD\n");
                for (i = 0; i < cec_info.data_len; i++) {
                    printf("0x%x " , cec_info.data[i]);
                }
                printf("\n");
            }
        }
        usleep(1000 * 10);
    }

    printf("exit cec_read_thread 1\n");
    return NULL;
}

static void hdmi_rx_start_cec_thread(void)
{
    printf("hdmi_rx_start_cec_thread\n");

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x1000);
    pthread_create(&m_cec_thread_id, &attr, cec_read_thread, NULL);
    pthread_attr_destroy(&attr);

}


int sorting_rx_test(int argc , char **argv)
{
    (void)argc;
    (void)argv;
    if (g_b_start == true) {
        return 0;
    }

    if (rx_fd < 0) {
        rx_fd = open("/dev/hdmi_rx" , O_RDWR);
        if (rx_fd < 0) {
            return -1;
        }
    }
    apath = 0;
    vpath = 3;
    stop_mode = 1;
    hdmi_rx_quality_type = 0;
    printf("apath %d, vpath %d stop_mode %d hdmi_rx_quality_type %d\n" ,
           apath , vpath , stop_mode , hdmi_rx_quality_type);
    printf("set_edid = %d\n" , set_edid);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_DATA_PATH , vpath);
    ioctl(rx_fd , HDMI_RX_SET_AUDIO_DATA_PATH , apath);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_STOP_MODE , stop_mode);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY , hdmi_rx_quality_type);
    hdmi_rx_set_enc_table();
#if 0
    memcpy(&(hdmi_rx_hdcp.hdcp_key[0]) , g_hdmi_rx_hdcp_cstm_key , HDCP_KEY_LEN);
    hdmi_rx_hdcp.b_encrypted = TRUE;
    ioctl(rx_fd , HDMI_RX_SET_HDCP_KEY , &hdmi_rx_hdcp);
#endif
    if (set_edid == true) {
        memcpy(&(hdmi_rx_edid.edid_data[0]) , edid_data , EDID_DATA_LEN);
        ioctl(rx_fd , HDMI_RX_SET_EDID , &hdmi_rx_edid);
    }

    hdmi_rx_start_audio_thread();
    hdmi_rx_start_video_thread();

    if (vpath == HDMI_RX_VIDEO_TO_DE_ROTATE ||
        vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM) {
        printf("rotate_mode = 0x%x\n" , rotate_mode);
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_ROTATE_MODE , rotate_mode);
        printf("mirror_mode = 0x%x\n" , mirror_mode);
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_MIRROR_MODE , mirror_mode);
    }

    ioctl(rx_fd , HDMI_RX_START);
    g_b_start = true;
    printf("hdmi_rx start ok```\n");
    return 0;
}


static int hdmi_rx_start_test(int argc , char *argv[])
{
    (void)argc;
    (void)argv;
    int opt;
    opterr = 0;
    optind = 0;
    struct hdmi_rx_display_info dis_info = { 0 };
    int real_rotate;
    int real_mirror;

    if (g_b_start == true) {
        return 0;
    }

    if (rx_fd < 0) {
        rx_fd = open("/dev/hdmi_rx" , O_RDWR);
        if (rx_fd < 0) {
            return -1;
        }
    }

    while ((opt = getopt(argc , argv , "a:v:r:s:m:f:e:q:l:p:t:")) != EOF) {
        switch (opt) {
            case 'a':
                apath = atoi(optarg);
                break;
            case 'v':
                vpath = atoi(optarg);
                break;
            case 'r':
                rotate_mode = atoi(optarg);
                break;
            case 's':
                stop_mode = atoi(optarg);
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

            case 'e':
                set_edid = atoi(optarg);
                break;
            case 'q':
                hdmi_rx_quality_type = atoi(optarg);
                break;
            case 'l':
                if (atoi(optarg) == 0) {
                    hrx_dis_layer = DIS_LAYER_MAIN;
                } else {
                    hrx_dis_layer = DIS_LAYER_AUXP;
                }
                break;
            case 't':
                if (atoi(optarg) == 0) {
                    hrx_dis_type = DIS_TYPE_HD;
                } else {
                    hrx_dis_type = DIS_TYPE_UHD;
                }
                break;
            case 'p':
                g_pbp_mode = atoi(optarg);
                g_pbp_mode = g_pbp_mode ? 1 : 0;
                break;
            default:
                break;
        }
    }

    dis_info.dis_type = hrx_dis_type;
    dis_info.dis_layer = hrx_dis_layer;

    printf("apath %d, vpath %d stop_mode %d hdmi_rx_quality_type %d\n" ,
           apath , vpath , stop_mode , hdmi_rx_quality_type);
    printf("dis_type %d dis_layer %d\n" , dis_info.dis_type , dis_info.dis_layer);
    printf("set_edid = %d\n" , set_edid);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_DATA_PATH , vpath);
    ioctl(rx_fd , HDMI_RX_SET_AUDIO_DATA_PATH , apath);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_STOP_MODE , stop_mode);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY , hdmi_rx_quality_type);
    ioctl(rx_fd , HDMI_RX_SET_DISPLAY_INFO , &dis_info);
    ioctl(rx_fd , HDMI_RX_SET_PBP_MODE , g_pbp_mode);
    hdmi_rx_set_enc_table();

#if 0
    memcpy(&(hdmi_rx_hdcp.hdcp_key[0]) , g_hdmi_rx_hdcp_cstm_key , HDCP_KEY_LEN);
    hdmi_rx_hdcp.b_encrypted = TRUE;
    ioctl(rx_fd , HDMI_RX_SET_HDCP_KEY , &hdmi_rx_hdcp);
#endif
    if (set_edid == true) {
        memcpy(&(hdmi_rx_edid.edid_data[0]) , edid_data , EDID_DATA_LEN);
        ioctl(rx_fd , HDMI_RX_SET_EDID , &hdmi_rx_edid);
    }
    if (g_preview == true) {
        ioctl(rx_fd , HDMI_RX_SET_DISPLAY_RECT , &g_dis_rect);
    }

    hdmi_rx_start_audio_thread();
    hdmi_rx_start_video_thread();

    if (vpath == HDMI_RX_VIDEO_TO_DE_ROTATE ||
        vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM) {

    #if 1
        //get rotate and mirror type by project mode and dts, and overlay
        //the -r -m setting for rotation and mirror type.
        int rotate_type;
        int mirror_type;
        int rotate;
        hrxtest_get_flip_info(project_mode, hrx_dis_type, &rotate_type, &mirror_type);
        rotate = hrxtest_rotate_convert(rotate_type, rotate_mode);
        real_mirror = hrxtest_flip_convert(hrx_dis_type, mirror_type, mirror_mode);
        real_rotate = rotate % 4;
    #else
        real_mirror = mirror_mode;
        real_rotate = rotate_mode;
    #endif

        printf("rotate_mode = 0x%x\n" , real_rotate);
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_ROTATE_MODE , real_rotate);
        printf("mirror_mode = 0x%x\n" , real_mirror);
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_MIRROR_MODE , real_mirror);
    }

    ioctl(rx_fd , HDMI_RX_START);
    g_b_start = true;
    printf("hdmi_rx start ok```\n");
    return 0;
}

static void hdmi_rx_stop_audio_thread(void)
{
    audio_stop_read = 1;
    if (apath >= HDMI_RX_AUDIO_TO_I2SI_AND_KSHM && audio_read_hdl.desc) {
        if (m_audio_thread_id)
            pthread_join(m_audio_thread_id, NULL);
        m_audio_thread_id = 0;
    }
    audio_read_hdl.desc = NULL;
}

static void hdmi_rx_stop_video_thread(void)
{
    video_stop_read = 1;

    if (vpath == HDMI_RX_VIDEO_TO_KSHM ||
        vpath == HDMI_RX_VIDEO_TO_DE_AND_KSHM ||
        vpath == HDMI_RX_VIDEO_TO_DE_ROTATE_AND_KSHM) {
        if (m_video_thread_id)
            pthread_join(m_video_thread_id, NULL);
        m_video_thread_id = 0;
    }
}

static int hdmi_rx_stop_test(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    if (rx_fd >= 0) {
        hdmi_rx_stop_audio_thread();
        hdmi_rx_stop_video_thread();

        ioctl(rx_fd , HDMI_RX_STOP);
        //close(rx_fd);
        //rx_fd = -1;
        g_b_start = false;
        if (buf_yuv444 != NULL) {
            free(buf_yuv444);
            buf_yuv444 = NULL;
        }
        return 0;
    } else {
        return -1;
    }

    if(akshm_fd >= 0)
        close(akshm_fd);
    if(vkshm_fd >= 0)
        close(vkshm_fd);
    akshm_fd = vkshm_fd = -1;
}

static int hdmi_rx_get_video_info(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    if (rx_fd >= 0) {
        hdmi_rx_video_info_t hdmi_rx_video_info = { 0 };
        ioctl(rx_fd , HDMI_RX_GET_VIDEO_INFO , &hdmi_rx_video_info);
        printf("w:%d h:%d format:%d range:%d: frame_rate:%d b_progressive:%d\n" ,
               hdmi_rx_video_info.width ,
               hdmi_rx_video_info.height ,
               hdmi_rx_video_info.color_format ,
               hdmi_rx_video_info.range ,
               hdmi_rx_video_info.frame_rate ,
               hdmi_rx_video_info.b_progressive);

        return 0;
    } else {
        return -1;
    }
}

#include <sys/mman.h>
#define HDMI_RX_INTER_FRAME_SIZE_Y           (1920 * 1088)

#if 0


enum VideoColorSpace {
    ITU_601 ,
    ITU_709 ,
    SMPTE_240M ,
};
static int Trunc8(int v)
{
    return v >> 8;
}

static int Clamp8(int v)
{
    if (v < 0) {
        return 0;
    } else if (v > 255) {
        return 255;
    } else {
        return v;
    }
}

static void pixel_ycbcrL_to_argbF(unsigned char y , unsigned char cb , unsigned char cr ,
                                  unsigned char *r , unsigned char *g , unsigned char *b ,
                                  int c[3][3])
{
    int  red = 0 , green = 0 , blue = 0;

    const int y1 = (y - 16) * c[0][0];
    const int pr = cr - 128;
    const int pb = cb - 128;
    red = Clamp8(Trunc8(y1 + (pr * c[0][2])));
    green = Clamp8(Trunc8(y1 + (pb * c[1][1] + pr * c[1][2])));
    blue = Clamp8(Trunc8(y1 + (pb * c[2][1])));

    *r = red;
    *g = green;
    *b = blue;
}

static void pixel_ycbcrF_to_argbF(unsigned char y , unsigned char cb , unsigned char cr ,
                                  unsigned char *r , unsigned char *g , unsigned char *b ,
                                  int c[3][3])
{
    int  y_2 = 0 , cr_r = 0 , cr_g = 0 , cb_g = 0 , cb_b = 0;
    int  red = 0 , green = 0 , blue = 0;

    const int y1 = (y)*c[0][0];
    const int pr = cr - 128;
    const int pb = cb - 128;
    red = Clamp8(Trunc8(y1 + (pr * c[0][2])));
    green = Clamp8(Trunc8(y1 + (pb * c[1][1] + pr * c[1][2])));
    blue = Clamp8(Trunc8(y1 + (pb * c[2][1])));

    *r = red;
    *g = green;
    *b = blue;
}

static void YUV422_YUV444ORRGB(char *p_buf_y ,
                               char *p_buf_c ,
                               char *p_buf_yuv ,
                               int ori_width ,
                               int ori_height ,
                               enum VideoColorSpace color_space ,
                               bool b_yuv2rgb
)
{
    int i = 0 , j = 0 , k = 0;
    int width = (ori_width + 31) & 0xFFFFFFE0;
    int height = (ori_height + 15) & 0xFFFFFFF0;
    int h_block = (width & 0xFFFFFFE0) >> 5;
    int v_block = (height & 0xFFFFFFF0) >> 4;
    char *p_y_block = NULL;
    char *p_c_block = NULL;
    char *p_y_line = NULL;
    char *p_c_line = NULL;
    char *p_y = NULL;
    char *p_c = NULL;
    char *p_out = NULL;
    unsigned char y = 0 , u = 0 , v = 0;
    unsigned char r , g , b;
    unsigned char cur_y , cur_u , cur_v;
    int block_size = 32 * 16;
    int h_block_size = h_block * block_size;

    int c_bt601_yuvL2rgbF[3][3] =
    { {298,0 ,  409},
      {298,-100 ,-208},
      {298,516,0} };

    int c_bt709_yuvL2rgbF[3][3] =
    { {298,0 ,  459},
      {298,-55 ,-136},
      {298,541,0} };

    p_y = p_buf_y;
    p_c = p_buf_c;
    p_out = p_buf_yuv;
    p_y_block = p_buf_y;
    p_c_block = p_buf_c;
    p_y_line = p_y_block;
    p_c_line = p_c_block;
    for (i = 0; i < v_block; i++) {
        for (j = 0; j < 16; j++) {
            p_y = p_y_line;
            p_c = p_c_line;

            for (k = 0; k < width; k++) {
                y = *p_y++;
                if ((k & 0x1) == 0) {
                    u = *p_c++;
                    v = *p_c++;
                }
                cur_v = v;
                cur_u = u;
                cur_y = y;

                if (b_yuv2rgb == true) {
                    if (k < ori_width) {
                        if (color_space == ITU_601) {
                            pixel_ycbcrL_to_argbF(y , u , v , &r , &g , &b , c_bt601_yuvL2rgbF);
                        } else {
                            pixel_ycbcrL_to_argbF(y , u , v , &r , &g , &b , c_bt709_yuvL2rgbF);
                        }

                        *p_out++ = b;
                        *p_out++ = g;
                        *p_out++ = r;
                        *p_out++ = 0xFF;
                    }
                } else {
                    if (k < ori_width) {
                        *p_out++ = cur_v;
                        *p_out++ = cur_u;
                        *p_out++ = cur_y;
                        *p_out++ = 0xFF;
                    }
                }

                if (k >= 31 && (((k + 1) & 0x1F) == 0)) {
                    p_y += block_size - 32;
                    p_c += block_size - 32;
                }
            }
            p_y_line += 32;
            p_c_line += 32;
        }
        p_y_block += h_block_size;
        p_c_block += h_block_size;
        p_y_line = p_y_block;
        p_c_line = p_c_block;
    }
}
#endif



static void get_yuv(unsigned char *p_buf_y ,
                    unsigned char *p_buf_c ,
                    int ori_width ,
                    int ori_height ,
                    int x ,
                    int y ,
                    unsigned char *y_value ,
                    unsigned char *u ,
                    unsigned char *v)
{
    (void)ori_height;

    int mb_x = 0;
    int mb_y = 0;
    int x1 , y1;
    int pos;
    int width1 = (ori_width + 31) & 0xFFFFFFE0;
    int mb_width = width1 >> 5;
    int pos_mb;

    mb_x = x >> 5;
    mb_y = y >> 4;
    x1 = x & 0x1F;
    y1 = y & 0xF;

    pos_mb = ((mb_width * mb_y + mb_x) << 9) + (y1 << 5);
    pos = pos_mb + x1;
    *y_value = p_buf_y[pos];

    pos = pos_mb + (x1 >> 1 << 1);
    *u = p_buf_c[pos];
    *v = p_buf_c[pos + 1];
}

static void YUV422_YUV444(unsigned char *p_buf_y ,
                          unsigned char *p_buf_c ,
                          unsigned char *p_buf_yuv ,
                          int ori_width ,
                          int ori_height)
{
    int i = 0 , j = 0;
    char *p_out = NULL;
    unsigned char cur_y , cur_u , cur_v;

    p_out = (char*)p_buf_yuv;

    for (i = 0; i < ori_height; i++) {
        for (j = 0;j < ori_width;j++) {
            get_yuv((unsigned char *)p_buf_y ,
                    (unsigned char *)p_buf_c ,
                    ori_width ,
                    ori_height ,
                    j ,
                    i ,
                    &cur_y , &cur_u , &cur_v);
            *p_out++ = cur_v;
            *p_out++ = cur_u;
            *p_out++ = cur_y;
            *p_out++ = 0xFF;
        }
    }
}

static void hdmi_rx_YUV422_YUV44(unsigned char *p_buf ,
                                 unsigned char *p_buf_yuv ,
                                 int ori_width ,
                                 int ori_height)
{
    unsigned char *p_y = p_buf;
    unsigned char *p_c = p_buf + HDMI_RX_INTER_FRAME_SIZE_Y;

    YUV422_YUV444(p_y ,
                  p_c ,
                  p_buf_yuv ,
                  ori_width ,
                  ori_height);
}
static int hdmi_rx_get_video_buf(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    int buf_size = 0;
    void *buf = NULL;

    if (rx_fd >= 0) {
        hdmi_rx_video_info_t hdmi_rx_video_info = { 0 };
        ioctl(rx_fd , HDMI_RX_GET_VIDEO_INFO , &hdmi_rx_video_info);
        printf("w:%d h:%d format:%d range:%d\n" ,
               hdmi_rx_video_info.width ,
               hdmi_rx_video_info.height ,
               hdmi_rx_video_info.color_format ,
               hdmi_rx_video_info.range);
        buf_size = hdmi_rx_video_info.width * hdmi_rx_video_info.height * 4;
        buf = mmap(0 , buf_size , PROT_READ | PROT_WRITE , MAP_SHARED , rx_fd , 0);
        printf("buf = 0x%x\n" , (int)buf);
        if (vpath == HDMI_RX_VIDEO_TO_DE) {
            buf_yuv444 = realloc(buf_yuv444 , hdmi_rx_video_info.width * hdmi_rx_video_info.height * 4);
            printf("buf_yuv444 = 0x%x\n" , (int)buf_yuv444);
            hdmi_rx_YUV422_YUV44((unsigned char *)buf ,
                                 buf_yuv444 ,
                                 hdmi_rx_video_info.width ,
                                 hdmi_rx_video_info.height);
        }
        return 0;
    } else {
        return -1;
    }
}

static int hdmi_rx_set_yuv2rgb_en(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    int yuv2rgb_en = 0;

    while ((opt = getopt(argc , argv , "e:")) != EOF) {
        switch (opt) {
            case 'e':
                yuv2rgb_en = atoi(optarg);
                break;
            default:
                break;
        }
    }
    if (rx_fd >= 0) {
        ioctl(rx_fd , HDMI_RX_SET_BUF_YUV2RGB_ONOFF , yuv2rgb_en);
        return 0;
    } else {
        return -1;
    }
}

static int hdmi_rx_dump(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    long buf_addr = 0;
    int len = 0;
    FILE *osd_fd = 0;
    long long tmp = 0;

    char *m_video_file_name = "/media/sda1/OSD.bin";
    osd_fd = fopen(m_video_file_name , "w");

    while ((opt = getopt(argc , argv , "b:l:")) != EOF) {
        switch (opt) {
            case 'b':
                tmp = strtoll(optarg , NULL , 16);
                buf_addr = (unsigned long)tmp;
                break;
            case 'l':
                len = atoi(optarg);
                break;
            default:
                break;
        }
    }

    // buf = (char *)0x876bc000;
     //len = 8355840;
    printf("buf_addr = 0x%lx len = %d\n" , buf_addr , len);
    fwrite((char *)buf_addr , 1 , len , osd_fd);
    fsync((int)osd_fd);
    fclose(osd_fd);

    return 0;
}

static int hdmi_rx_set_enc_table(void)
{
    struct jpeg_enc_quant enc_table;

    if (rx_fd >= 0) {
        memcpy(&enc_table.dec_quant_y , decoder_quant_table_y , 64);
        memcpy(&enc_table.dec_quant_c , decoder_quant_table_c , 64);
        memcpy(&enc_table.enc_quant_y , encoder_q_table_y , 128);
        memcpy(&enc_table.enc_quant_c , encoder_q_table_c , 128);

        ioctl(rx_fd , HDMI_RX_SET_ENC_QUANT , &enc_table);
        return 0;
    } else {
        return -1;
    }
}

static int hdmi_rx_set_quality(int argc , char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;

    while ((opt = getopt(argc , argv , "q:")) != EOF) {
        switch (opt) {
            case 'q':
                hdmi_rx_quality_type = atoi(optarg);
                break;
            default:
                break;
        }
    }

    if (rx_fd >= 0) {
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY , hdmi_rx_quality_type);
        return 0;
    } else {
        return -1;
    }
}

static int hdmi_rx_pause(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    if (rx_fd >= 0) {
        hdmi_rx_stop_audio_thread();
        hdmi_rx_stop_video_thread();
        ioctl(rx_fd , HDMI_RX_PAUSE);
    }
    return 0;
}
static int hdmi_rx_resume(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    if (rx_fd >= 0) {
        ioctl(rx_fd , HDMI_RX_RESUME);

        hdmi_rx_start_audio_thread();
        hdmi_rx_start_video_thread();
    }
    return 0;
}

static int hdmi_rx_get_enc_framerate(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    uint32_t enc_frame_rate = 0;
    if (rx_fd >= 0) {
        ioctl(rx_fd , HDMI_RX_GET_ENC_FRAMERATE , &enc_frame_rate);
        printf("enc_frame_rate = %ld\n" , enc_frame_rate);
    }
    return 0;
}

static int hdmi_rx_set_preview(int argc , char *argv[])
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
    if (rx_fd > 0) {
        ioctl(rx_fd , HDMI_RX_SET_DISPLAY_RECT , &g_dis_rect);
    }
    return 0;
}

static int hdmi_rx_proc_cec_cmd(enum HDMI_CEC_CMD cec_cmd , struct hdmi_cec_info *cec_info)
{
    unsigned char *data = cec_info->data;
    int ret = 0;

    printf("cec_cmd = %d\n" , cec_cmd);
    switch (cec_cmd) {
        case HDMI_CEC_CMD_SYSTEM_STANDBY:
        {
            data[0] = ((g_cec_logic_addr < 4) | CEC_LA_BROADCAST);
            data[1] = OPCODE_SYSTEM_STANDBY;
            cec_info->data_len = 2;
            break;
        }
        case HDMI_CEC_CMD_IMAGE_VIEW_ON:
        {
            data[0] = ((g_cec_logic_addr << 4) | CEC_LA_BROADCAST);
            data[1] = OPCODE_IMAGE_VIEW_ON;
            cec_info->data_len = 2;
            break;
        }
        case HDMI_CEC_CMD_PHYSICAL_ADDR:
        {
            data[0] = ((g_cec_logic_addr << 4) | CEC_LA_BROADCAST);
            data[1] = OPCODE_GIVE_PHYSICAL_ADDR;
            cec_info->data_len = 2;
            break;
        }
        case HDMI_CEC_CMD_GIVE_POWER:
        {
            data[0] = ((g_cec_logic_addr << 4) | 4);
            data[1] = OPCODE_GIVE_POWER_STATUS;
            cec_info->data_len = 2;
            break;
        }

        case HDMI_CEC_CMD_USER_CTRL_PRESSED:
        {
            data[0] = ((g_cec_logic_addr << 4) | 4);
            data[1] = OPCODE_USER_CTRL_PRESSED;
            data[2] = 0x40;
            cec_info->data_len = 3;

            break;
        }
        case HDMI_CEC_CMD_USER_CTRL_RELEASED:
        {
            data[0] = ((g_cec_logic_addr << 4) | 4);
            data[1] = OPCODE_USER_CTRL_RELEASED;
            cec_info->data_len = 2;
            break;
        }
        default:
        {
            printf("UN support CEC CMD:0x%x\n" , cec_cmd);
            ret = -1;
            break;
        }
    }

    return ret;
}

static int hdmi_rx_send_cec_cmd(int argc , char *argv[])
{
    int ret = -1;
    int opt;
    opterr = 0;
    optind = 0;
    enum HDMI_CEC_CMD cec_cmd = 0;
    struct hdmi_cec_info cec_info = { 0 };

    while ((opt = getopt(argc , argv , "c:")) != EOF) {
        switch (opt) {
            case 'c':
                cec_cmd = atoi(optarg);
                break;
            default:
                break;
        }
    }
    hdmi_rx_proc_cec_cmd(cec_cmd , &cec_info);
    if (rx_fd >= 0) {
        ret = ioctl(rx_fd , HDMI_CEC_SEND_CMD , &cec_info);
        if (ret < 0) {
            printf("HDMI_TX_SEND_CEC_CMD error\n");
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}

static int hdmi_rx_set_logic_addr(int argc , char *argv[])
{
    int ret = -1;
    int opt;
    opterr = 0;
    optind = 0;

    while ((opt = getopt(argc , argv , "a:")) != EOF) {
        switch (opt) {
            case 'a':
                g_cec_logic_addr = atoi(optarg);
                break;
            default:
                break;
        }
    }
    if (rx_fd >= 0) {
        ret = ioctl(rx_fd , HDMI_CEC_SET_LOGIC_ADDR , g_cec_logic_addr);
        if (ret < 0) {
            printf("hdmi_rx_set_logic_addr\n");
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}

static int hdmi_rx_set_wm(int argc , char *argv[])
{
    int ret = -1;
    int opt;
    opterr = 0;
    optind = 0;

	struct wm_param wm = {0};
    while ((opt = getopt(argc , argv , "e:x:y:a:b:c:f:")) != EOF) {
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
    if (rx_fd >= 0) {
        ret = ioctl(rx_fd , HDMI_RX_SET_WM_PARAM , &wm);
        if (ret < 0) {
            printf("hdmi_rx_set_wm\n");
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}

int hdmi_test_rx_enter(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    if (rx_fd < 0) {
        printf("open hdmi_rx\n");
        rx_fd = open("/dev/hdmi_rx" , O_RDWR);
        if (rx_fd < 0) {
            return -1;
        }
        cec_stop_read = 0;
        hdmi_rx_start_cec_thread();
    }

    return 0;
}
static int hdmi_rx_quit(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    if (rx_fd >= 0) {
        cec_stop_read = 1;
        if (m_cec_thread_id)
            pthread_join(m_cec_thread_id, NULL);
        m_cec_thread_id = 0;
        close(rx_fd);
        rx_fd = -1;
    }

    return 0;
}

static int hdmi_rx_test_rotate(int argc , char *argv[])
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

    if (rx_fd > 0) {
        int real_rotate;
        int real_mirror;

    #if 1
        //get rotate and mirror type by project mode and dts, and overlay
        //the -r -m setting for rotation and mirror type.
        int rotate_type;
        int mirror_type;
        int rotate;
        hrxtest_get_flip_info(project_mode, hrx_dis_type, &rotate_type, &mirror_type);
        rotate = hrxtest_rotate_convert(rotate_type, rotate_mode);
        real_mirror = hrxtest_flip_convert(hrx_dis_type, mirror_type, mirror_mode);
        real_rotate = rotate % 4;
    #else
        real_rotate = rotate_mode;
        real_mirror = mirror_mode;
    #endif        

        ioctl(rx_fd , HDMI_RX_SET_VIDEO_DATA_PATH , HDMI_RX_VIDEO_TO_DE_ROTATE);
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_ROTATE_MODE , real_rotate);
        ioctl(rx_fd , HDMI_RX_SET_VIDEO_MIRROR_MODE , real_mirror);
    }

    return 0;
}


static const char help_quit[] = "quit hdmi rx";
static const char help_start[] = 
            "hdmi_rx -a apath -v vpath, default video to osd, audio to tx\n\t\t\t"
            "-r : 0, degree; 1, 90 degree; 2, 180 degree; 3, 180 degree\n\t\t\t"
            "-m : 0, no flip; 1, horizonal filp\n\t\t\t"
            "-f set project mode: 0, Front; 1, Ceiling Front; 2, Rear; 3, Ceiling Rear\n\t\t\t"
            "-t set dis type: 0, DIS HD; 1. DIS UHD\n\t\t\t"
            "-l set dis layer: 0, DIS main layer; 1. DIS auxp layer\n\t\t\t"
            "-p multi display enable: 0, disable; 1, enable\n\t"
;
static const char help_stop[] = "stop hdmi_rx";
static const char help_video_info[] = "get hdmi_rx video info";
static const char help_video_buf[] = "get hdmi_rx video buf";
static const char help_yuv2rgb_en[] = "set_yuv2rgb_en";
static const char help_dump[] = "dump data";
static const char help_quality[] = "set_quality";
static const char help_framerate[] = "get_enc_framerate";
static const char help_sorting[] = "soring test lvds to hdmrx";
static const char help_pause[] = "pause";
static const char help_resume[] = "resume";
static const char help_cec_cmd[] = "send CEC cmds: -c 0:standby";
static const char help_logic_addr[] = "Set address: -a :logic addr";
static const char help_preview[] = 
        "preview src dst, base on 1920*1080\n\t\t\t"
        "for example: preview 0 0 1920 1080  800 450 320 180\n\t"
;
static const char help_rotate[] = 
    "rotate -r <rotage_angle> -m <horizonal> -f <project mode>\n\t\t\t"
    "-r : 0, degree; 1, 90 degree; 2, 180 degree; 3, 180 degree\n\t\t\t"
    "-m : 0, no flip; 1, horizonal filp\n\t\t\t"
    "-f set project mode: 0, Front; 1, Ceiling Front; 2, Rear; 3, Ceiling Rear\n\t"
;

static const char help_wm[] = 
    "wm -e <enable> -x <h start pos> -y <h start pos> -a color_y -b color_u -c color_v -f <date format>\n\t\t\t"
;

#ifdef __linux__

void hdmi_rx_cmds_register(struct console_cmd *cmd)
{
    console_register_cmd(cmd, "start", hdmi_rx_start_test, CONSOLE_CMD_MODE_SELF, help_start);
    console_register_cmd(cmd, "quit", hdmi_rx_quit, CONSOLE_CMD_MODE_SELF, help_quit);
    console_register_cmd(cmd, "stop", hdmi_rx_stop_test, CONSOLE_CMD_MODE_SELF, help_stop);
    console_register_cmd(cmd, "get_video_info", hdmi_rx_get_video_info, CONSOLE_CMD_MODE_SELF, help_video_info);
    console_register_cmd(cmd, "get_video_buf", hdmi_rx_get_video_buf, CONSOLE_CMD_MODE_SELF, help_video_buf);
    console_register_cmd(cmd, "set_yuv2rgb_en", hdmi_rx_set_yuv2rgb_en, CONSOLE_CMD_MODE_SELF, help_yuv2rgb_en);
    console_register_cmd(cmd, "dump", hdmi_rx_dump, CONSOLE_CMD_MODE_SELF, help_dump);
    console_register_cmd(cmd, "set_quality", hdmi_rx_set_quality, CONSOLE_CMD_MODE_SELF, help_quality);
    console_register_cmd(cmd, "pause", hdmi_rx_pause, CONSOLE_CMD_MODE_SELF, help_pause);
    console_register_cmd(cmd, "resume", hdmi_rx_resume, CONSOLE_CMD_MODE_SELF, help_resume);
    console_register_cmd(cmd, "get_enc_framerate", hdmi_rx_get_enc_framerate, CONSOLE_CMD_MODE_SELF, help_framerate);
    console_register_cmd(cmd, "preview", hdmi_rx_set_preview, CONSOLE_CMD_MODE_SELF, help_preview);
    console_register_cmd(cmd, "sorting_hdmi_rx_test", sorting_rx_test, CONSOLE_CMD_MODE_SELF, help_sorting);
    console_register_cmd(cmd, "send_cec_cmd", hdmi_rx_send_cec_cmd, CONSOLE_CMD_MODE_SELF, help_cec_cmd);
    console_register_cmd(cmd, "set_logic_addr", hdmi_rx_set_logic_addr, CONSOLE_CMD_MODE_SELF, help_logic_addr);
    console_register_cmd(cmd, "rotate", hdmi_rx_test_rotate, CONSOLE_CMD_MODE_SELF, help_rotate);
	console_register_cmd(cmd, "wm", hdmi_rx_set_wm, CONSOLE_CMD_MODE_SELF, help_wm);
}

#ifndef BR2_PACKAGE_VIDEO_PBP_EXAMPLES

static struct termios stored_settings;
static void exit_console(int signo)
{
    (void)signo;

    hdmi_rx_stop_test(0, NULL);
    tcsetattr(0 , TCSANOW , &stored_settings);
    exit(0);
}

int main(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    struct termios new_settings;

    tcgetattr(0 , &stored_settings);
    new_settings = stored_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0 , TCSANOW , &new_settings);

    signal(SIGTERM , exit_console);
    signal(SIGINT , exit_console);
    signal(SIGSEGV , exit_console);
    signal(SIGBUS , exit_console);
    console_init("hdmi_rx:");

    hdmi_test_rx_enter(argc, argv);

    hdmi_rx_cmds_register(NULL);

    console_start();
    exit_console(0);
    return 0;
}
#endif


#else //else of __linux__

static const char help_enter[] = "enter hdmi rx";

CONSOLE_CMD(hdmi_rx , NULL , hdmi_test_rx_enter , CONSOLE_CMD_MODE_SELF , help_enter)
CONSOLE_CMD(quit , "hdmi_rx" , hdmi_rx_quit , CONSOLE_CMD_MODE_SELF , help_quit)
CONSOLE_CMD(start , "hdmi_rx" , hdmi_rx_start_test , CONSOLE_CMD_MODE_SELF, help_start)
CONSOLE_CMD(stop , "hdmi_rx" , hdmi_rx_stop_test , CONSOLE_CMD_MODE_SELF, help_stop)
CONSOLE_CMD(get_video_info , "hdmi_rx" , hdmi_rx_get_video_info , CONSOLE_CMD_MODE_SELF, help_video_info)
CONSOLE_CMD(get_video_buf , "hdmi_rx" , hdmi_rx_get_video_buf , CONSOLE_CMD_MODE_SELF, help_video_buf)
CONSOLE_CMD(set_yuv2rgb_en , "hdmi_rx" , hdmi_rx_set_yuv2rgb_en , CONSOLE_CMD_MODE_SELF, help_yuv2rgb_en)
CONSOLE_CMD(dump , "hdmi_rx" , hdmi_rx_dump , CONSOLE_CMD_MODE_SELF , help_dump)
CONSOLE_CMD(set_quality , "hdmi_rx" , hdmi_rx_set_quality , CONSOLE_CMD_MODE_SELF, help_quality)
CONSOLE_CMD(pause , "hdmi_rx" , hdmi_rx_pause , CONSOLE_CMD_MODE_SELF , help_pause)
CONSOLE_CMD(resume , "hdmi_rx" , hdmi_rx_resume , CONSOLE_CMD_MODE_SELF , help_resume)
CONSOLE_CMD(get_enc_framerate , "hdmi_rx" , hdmi_rx_get_enc_framerate , CONSOLE_CMD_MODE_SELF, help_framerate)
CONSOLE_CMD(preview , "hdmi_rx" , hdmi_rx_set_preview , CONSOLE_CMD_MODE_SELF, help_preview)
CONSOLE_CMD(sorting_hdmi_rx_test , NULL , sorting_rx_test , CONSOLE_CMD_MODE_SELF, help_sorting)
CONSOLE_CMD(send_cec_cmd , "hdmi_rx" , hdmi_rx_send_cec_cmd , CONSOLE_CMD_MODE_SELF , help_cec_cmd)
CONSOLE_CMD(set_logic_addr , "hdmi_rx" , hdmi_rx_set_logic_addr , CONSOLE_CMD_MODE_SELF , help_logic_addr)
CONSOLE_CMD(rotate , "hdmi_rx" , hdmi_rx_test_rotate , CONSOLE_CMD_MODE_SELF , help_rotate)
CONSOLE_CMD(wm , "hdmi_rx" , hdmi_rx_set_wm , CONSOLE_CMD_MODE_SELF , help_wm)

#ifdef CONFIG_CMDS_HDMI_RX_SORTING
static int get_mtdblock_devpath(char *devpath , int len , const char *partname)
{
    static int np = -1;
    static u32 part_num = 0;
    u32 i = 1;
    const char *label;
    char property[32];

    if (np < 0) {
        np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
    }

    if (np < 0)
        return -1;

    if (part_num == 0)
        fdt_get_property_u_32_index(np , "part-num" , 0 , &part_num);

    for (i = 1; i <= part_num; i++) {
        snprintf(property , sizeof(property) , "part%d-label" , i);
        if (!fdt_get_property_string_index(np , property , 0 , &label) &&
            !strcmp(label , partname)) {
            memset(devpath , 0 , len);
            snprintf(devpath , len , "/dev/mtdblock%d" , i);
            return i;
        }
    }

    return -1;
}

static int auto_mount_eromfs(void)
{
    char devpath[64];
    int ret;
    ret = get_mtdblock_devpath(devpath , sizeof(devpath) , "eromfs");
    if (ret >= 0)
        ret = mount(devpath , "/etc" , "romfs" , MS_RDONLY , NULL);
    return 0;
}
__initcall(auto_mount_eromfs)
#endif


#endif