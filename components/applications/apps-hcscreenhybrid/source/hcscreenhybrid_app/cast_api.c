/**
 * @file cast_api.c
 * @author your name (you@domain.com)
 * @brief hichip cast api
 * @version 0.1
 * @date 2022-01-21
 *
 * @copyright Copyright (c) 2022
 *
 */


#include <stdbool.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <time.h>
#include <pthread.h>

#include <hccast/hccast_scene.h>
#include <hcuapi/dis.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <hcuapi/snd.h>
#include <hccast/hccast_net.h>
#ifdef SUPPORT_UM_SLAVE
#include <kernel/drivers/hcusb.h>
#endif
#ifdef __HCRTOS__
#include <kernel/lib/fdt_api.h>
#include <kernel/drivers/avinit.h>
#endif

#include "com_api.h"
#include "osd_com.h"
#include "cast_api.h"
#include "data_mgr.h"
#include "app_log.h"
#include "network_api.h"

#define CAST_SERVICE_NAME               "HCcast"
#define CAST_AIRCAST_SERVICE_NAME       "HCcast"
#define CAST_DLNA_FIRENDLY_NAME         "HCcast"
#define CAST_MIRACAST_NAME              "HCcast"

#define UUID_HEADER "HCcast"
//#define CAST_PHOTO_DETECT

static bool m_air_is_demo = false;
static bool m_dial_is_demo = false;
static bool m_um_is_demo = false;

static cast_dial_conn_e m_dial_conn_state = CAST_DIAL_CONN_NONE;

void cast_api_set_dis_zoom(av_area_t * src_rect, av_area_t * dst_rect, dis_scale_avtive_mode_e active_mode);

#ifdef __HCRTOS__
__attribute__((weak)) void set_eswin_drop_out_of_order_packet_flag(int enable) {}
#endif

#ifndef DLNA_SUPPORT
//implement fake functions
int hccast_dlna_service_uninit(void)
{
    return 0;
}

int hccast_dlna_service_start(void)
{
    return 0;
}

int hccast_dlna_service_stop(void)
{
    return 0;
}

#endif


#ifndef AIRCAST_SUPPORT
//implement fake functions
int hccast_air_audio_state_get(void)
{
    return 0;
}
int hccast_air_service_start(void)
{
    return 0;
}
int hccast_air_service_stop(void)
{
    return 0;
}
int hccast_air_mdnssd_start(void)
{
}
int hccast_air_mdnssd_stop(void)
{
}
int hccast_air_service_is_start(void)
{
    return 0;
}

#endif

#ifndef MIRACAST_SUPPORT
//implement fake functions


int hccast_mira_service_start(void)
{
    return 0;
}

int hccast_mira_service_stop(void)
{
    return 0;
}
int hccast_mira_player_init(void)
{
    return 0;
}

int hccast_mira_get_stat(void)
{
    return 0;
}

int hccast_mira_service_uninit(void)
{
    return 0;
}

#endif

void cast_api_set_volume(int vol)
{
    int snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0)
    {
        printf("Open /dev/sndC0i2so fail.\n");
        return;
    }
    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &vol);
    close(snd_fd);
}

int cast_api_get_volume(void)
{
    int snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    uint8_t vol = 0;
    if (snd_fd < 0)
    {
        return 0;
    }
    ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &vol);
    //printf("get vol: %d\n", vol);
    close(snd_fd);

    return vol;
}

void cast_api_set_dis_zoom(av_area_t *src_rect,
                         av_area_t *dst_rect,
                         dis_scale_avtive_mode_e active_mode)
{
    int dis_fd = open("/dev/dis" , O_WRONLY);

    if (dis_fd >= 0) 
    {
        struct dis_zoom dz;
        dz.distype = DIS_TYPE_HD;
        dz.layer = DIS_LAYER_MAIN;
        dz.active_mode = active_mode;
        memcpy(&dz.src_area, src_rect, sizeof(struct av_area));
        memcpy(&dz.dst_area, dst_rect, sizeof(struct av_area));
        ioctl(dis_fd, DIS_SET_ZOOM, &dz);
        close(dis_fd);
    }
}

void cast_api_set_aspect_mode(dis_tv_mode_e ratio, 
                            dis_mode_e dis_mode, 
                            dis_scale_avtive_mode_e active_mode)
{
    int fd = open("/dev/dis", O_RDWR);
    if ( fd < 0)
        return;
    dis_aspect_mode_t aspect = {0};
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    aspect.dis_mode = dis_mode;
    aspect.active_mode = active_mode;
    ioctl(fd, DIS_SET_ASPECT_MODE, &aspect);
    close(fd);
}

int cast_get_service_name(cast_type_t cast_type, char *service_name, int length)
{
    unsigned char mac_addr[6] = {0};
    char service_prefix[32] = CAST_SERVICE_NAME;

    snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_SERVICE_NAME);
    if (0 != api_get_mac_addr((char*)mac_addr))
        memset(mac_addr, 0xff, sizeof(mac_addr));

    if (CAST_TYPE_AIRCAST == cast_type)
        snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_AIRCAST_SERVICE_NAME);
    else if (CAST_TYPE_DLNA == cast_type)
        snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_DLNA_FIRENDLY_NAME);
    else if (CAST_TYPE_MIRACAST == cast_type)
        snprintf(service_prefix, sizeof(service_prefix)-1, "%s", CAST_MIRACAST_NAME);

    snprintf(service_name, length, "%s-%02x%02x%02x",
             service_prefix, mac_addr[3]&0xFF, mac_addr[4]&0xFF, mac_addr[5]&0xFF);

    return 0;
}


#ifdef USBMIRROR_SUPPORT
#define DEFAULT_FRM_SIZE    (0x100000)

static char m_ium_uuid[40] = {0};
static char *g_um_upgrade_buf = NULL;
static unsigned int g_um_upgrade_size = 0;
static char *g_um_frame_buf = NULL;
static unsigned int g_um_frame_size = 0;
static hccast_ium_upg_bo_t ium_upg_buf_info;
static hccast_aum_upg_bo_t aum_upg_buf_info;
static int um_start_upgrade = 0;//0--idel, 1--download, 2--burning.
static unsigned char g_slave_port = 0xFF;
static int g_ium_vd_dis_mode = DIS_PILLBOX;
static int g_ium_full_screen = 0;
int g_um_device_connected = 0;

static void ium_event_process_cb(int event, void *param1, void *param2);

int cast_um_dev_is_connected(void)
{
    return g_um_device_connected;
}

void cast_um_free_upg_buf(void)
{   
    if (g_um_upgrade_buf)
    {
        free(g_um_upgrade_buf);
        g_um_upgrade_buf = NULL;
        g_um_upgrade_size = 0;
    }
}

char *cast_um_alloc_upg_buf(int buf_len)
{  
    char *upg_buf = NULL;
    
    if (um_start_upgrade == 0)
    {
        cast_um_free_upg_buf();  
        g_um_upgrade_buf = malloc(buf_len);
        g_um_upgrade_size = buf_len;
        upg_buf = g_um_upgrade_buf;
        printf("um upg fw size: %d\n", buf_len);
    }

    return upg_buf;
}

static void *ium_burning_process(void *arg)
{
    sys_upg_flash_burn(ium_upg_buf_info.buf, ium_upg_buf_info.len);

    um_start_upgrade = 0;
    cast_um_free_upg_buf();

    return NULL;
}

static void *aum_burning_process(void *arg)
{
    sys_upg_flash_burn(aum_upg_buf_info.buf, aum_upg_buf_info.len);

    um_start_upgrade = 0;
    cast_um_free_upg_buf();

    return NULL;
}

void cast_um_set_dis_zoom(hccast_um_zoom_info_t *um_zoom_info)
{
    av_area_t src_rect;
    av_area_t dst_rect;

    memcpy(&src_rect, &um_zoom_info->src_rect, sizeof(av_area_t));
    memcpy(&dst_rect, &um_zoom_info->dst_rect, sizeof(av_area_t));

    cast_api_set_dis_zoom(&src_rect, &dst_rect, DIS_SCALE_ACTIVE_IMMEDIATELY);
}

static int cast_api_um_rotate_angle_convert(int rotate_mode)
{
    rotate_type_e rotate_angle = 0;
    
    switch (rotate_mode)
    {
        case HCCAST_UM_SCREEN_ROTATE_0:
            rotate_angle = ROTATE_TYPE_0;
            break;
        case HCCAST_UM_SCREEN_ROTATE_90:
            rotate_angle = ROTATE_TYPE_90;
            break;
        case HCCAST_UM_SCREEN_ROTATE_180:
            rotate_angle = ROTATE_TYPE_180;
            break;
        case HCCAST_UM_SCREEN_ROTATE_270:
            rotate_angle = ROTATE_TYPE_270;
            break;
        default:
            rotate_angle = ROTATE_TYPE_0;
            break;
    }

    return rotate_angle;
}

int cast_api_ium_get_rotate_info(hccast_ium_screen_mode_t *ium_screen_mode, hccast_um_rotate_info_t *rotate_info)
{
    rotate_type_e final_rotate_angle = 0;
    rotate_type_e ium_rotate_mode_angle = 0;
    rotate_type_e rotate_seting_angle = 0;
    rotate_type_e display_rotate = 0;
    int expect_vd_dis_mode = 0;
    hccast_um_param_t um_param;
    int video_width;
    int video_height;
    int ium_rotate_mode;
    int flip_rotate = 0;
    int flip_mirror = 0;
    int vscreen = 0;
    
    if (!ium_screen_mode || !rotate_info)
    {
        return -1;
    }

    hccast_um_param_get(&um_param);
    video_width = ium_screen_mode->video_width;
    video_height = ium_screen_mode->video_height;
    ium_rotate_mode = ium_screen_mode->rotate_mode;
    api_get_flip_info(&flip_rotate, &flip_mirror);
    
    if (!um_param.screen_rotate_en)
    {
        ium_rotate_mode_angle = cast_api_um_rotate_angle_convert(ium_rotate_mode);
        rotate_seting_angle = cast_api_um_rotate_angle_convert(um_param.screen_rotate_en);
        display_rotate = (flip_rotate + rotate_seting_angle) % 4;//flip_rotate;
        final_rotate_angle = (ium_rotate_mode_angle + flip_rotate) % 4;//Angle superposition calculation.
             
        if ((video_width > video_height) || (ium_rotate_mode == HCCAST_UM_SCREEN_ROTATE_270) || (ium_rotate_mode == HCCAST_UM_SCREEN_ROTATE_90))
        {
            if (um_param.full_screen_en && ((rotate_seting_angle == ROTATE_TYPE_0) || (rotate_seting_angle == ROTATE_TYPE_180)))
            {
                expect_vd_dis_mode = DIS_NORMAL_SCALE;
            }
            else
            {
                expect_vd_dis_mode = DIS_PILLBOX;
            }   
        }
        else
        {
            expect_vd_dis_mode = DIS_PILLBOX;
            vscreen = 1;
        }
    }
    else
    {
        ium_rotate_mode_angle = cast_api_um_rotate_angle_convert(ium_rotate_mode);
        rotate_seting_angle = cast_api_um_rotate_angle_convert(um_param.screen_rotate_en);
        display_rotate = (flip_rotate + rotate_seting_angle) % 4;
        final_rotate_angle = (ium_rotate_mode_angle + flip_rotate + rotate_seting_angle) % 4;//Angle superposition calculation.

        if (ium_rotate_mode == HCCAST_UM_SCREEN_ROTATE_0)
        {
            if ((video_height > video_width))
            {
                if ((rotate_seting_angle == ROTATE_TYPE_270) || (rotate_seting_angle == ROTATE_TYPE_90))
                {
                    if (um_param.full_screen_en)
                    {
                        expect_vd_dis_mode = DIS_NORMAL_SCALE; 
                    }
                    else
                    {
                        expect_vd_dis_mode = DIS_PILLBOX;
                    }
                } 
                else
                {
                    expect_vd_dis_mode = DIS_PILLBOX;
                }

                vscreen = 1;
            }
            else 
            {
                //HSCR.
                if ((rotate_seting_angle == ROTATE_TYPE_270) || (rotate_seting_angle == ROTATE_TYPE_90))
                {
                    if (um_param.screen_rotate_auto)
                    {
                        final_rotate_angle = (ROTATE_TYPE_0 + flip_rotate) % 4;
                        display_rotate = flip_rotate % 4;
                        if (um_param.full_screen_en)
                        {
                            expect_vd_dis_mode = DIS_NORMAL_SCALE;
                        }
                        else
                        {
                            expect_vd_dis_mode = DIS_PILLBOX;
                        }
                    }
                    else
                    {
                        expect_vd_dis_mode = DIS_PILLBOX;
                    }
                }   
                else
                {
                    if (um_param.full_screen_en)
                    {
                        expect_vd_dis_mode = DIS_NORMAL_SCALE;
                    }
                    else
                    {
                        expect_vd_dis_mode = DIS_PILLBOX;
                    }
                }
            }
        }
        else if ((ium_rotate_mode == HCCAST_UM_SCREEN_ROTATE_270) || (ium_rotate_mode == HCCAST_UM_SCREEN_ROTATE_90))
        {
            //The case actually is a vertical screen stream but see as HSCR.
            if ((rotate_seting_angle == ROTATE_TYPE_270) || (rotate_seting_angle == ROTATE_TYPE_90))
            {
                if (um_param.screen_rotate_auto)
                {
                    final_rotate_angle = (ium_rotate_mode_angle + flip_rotate) % 4;
                    display_rotate = flip_rotate;
                    if (um_param.full_screen_en)
                    {
                        expect_vd_dis_mode = DIS_NORMAL_SCALE; 
                    }
                    else
                    {
                        expect_vd_dis_mode = DIS_PILLBOX;
                    }
                }
                else
                {
                    expect_vd_dis_mode = DIS_PILLBOX;
                }
            }
            else
            {
                if (um_param.full_screen_en)
                {
                    expect_vd_dis_mode = DIS_NORMAL_SCALE; 
                }
                else
                {
                    expect_vd_dis_mode = DIS_PILLBOX;
                }
            }
        }
        else if (ium_rotate_mode == HCCAST_UM_SCREEN_ROTATE_180)
        {
            //The case actually is a vertical screen stream but see as VSCR.
            if ((rotate_seting_angle == ROTATE_TYPE_270) || (rotate_seting_angle == ROTATE_TYPE_90))
            {
                if (um_param.full_screen_en)
                {
                    expect_vd_dis_mode = DIS_NORMAL_SCALE; 
                }
                else
                {
                    expect_vd_dis_mode = DIS_PILLBOX;
                }
            }
            else
            {
                expect_vd_dis_mode = DIS_PILLBOX;
            }  

            vscreen = 1;
        }
    }

    if (expect_vd_dis_mode !=  g_ium_vd_dis_mode)
    {
        if (um_param.full_screen_en != g_ium_full_screen)
            cast_api_set_aspect_mode(DIS_TV_16_9, expect_vd_dis_mode, DIS_SCALE_ACTIVE_IMMEDIATELY);
        else
            cast_api_set_aspect_mode(DIS_TV_16_9, expect_vd_dis_mode, DIS_SCALE_ACTIVE_NEXTFRAME);
        g_ium_vd_dis_mode = expect_vd_dis_mode;
        g_ium_full_screen = um_param.full_screen_en;
    }

    rotate_info->rotate_angle = final_rotate_angle;
    rotate_info->flip_mode = flip_mirror;
    
    return 0;
}

int cast_api_aum_get_rotate_info(hccast_aum_screen_mode_t *aum_screen_mode, hccast_um_rotate_info_t *rotate_info)
{
    rotate_type_e final_rotate_angle = 0;
    rotate_type_e rotate_seting_angle = 0;
    hccast_um_param_t um_param;
    int flip_rotate = 0;
    int flip_mirror = 0;
    
    if (!aum_screen_mode || !rotate_info)
    {
        return -1;
    }

    hccast_um_param_get(&um_param);
    api_get_flip_info(&flip_rotate, &flip_mirror);
    
    if (!um_param.screen_rotate_en)
    {
        final_rotate_angle = (ROTATE_TYPE_0 + flip_rotate) % 4;//Angle superposition calculation.
    }
    else
    {
        if ((um_param.screen_rotate_en == HCCAST_UM_SCREEN_ROTATE_90 \
            || um_param.screen_rotate_en == HCCAST_UM_SCREEN_ROTATE_270) \
            && um_param.screen_rotate_auto && aum_screen_mode->mode)
        {
            final_rotate_angle = (ROTATE_TYPE_0 + flip_rotate) % 4;//ROTATE_TYPE_0
        }
        else
        { 
            rotate_seting_angle = cast_api_um_rotate_angle_convert(um_param.screen_rotate_en);
            final_rotate_angle = (flip_rotate + rotate_seting_angle) % 4;//Angle superposition calculation.
        }
    }

    rotate_info->rotate_angle = final_rotate_angle;
    rotate_info->flip_mode = flip_mirror;

    return 0;
}

int cast_api_ium_set_dis_aspect_mode(void)
{
    hccast_um_param_t um_param;
    hccast_um_zoom_info_t um_zoom_info = {{0, 0, 1920, 1080}, {0, 0, 1920, 1080}};

    hccast_um_param_get(&um_param);

    if (um_param.full_screen_en)
    {
        cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
        cast_um_set_dis_zoom(&um_zoom_info);
        g_ium_vd_dis_mode = DIS_NORMAL_SCALE;
    }
    else
    {
        cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
        cast_um_set_dis_zoom(&um_zoom_info);
        g_ium_vd_dis_mode = DIS_PILLBOX;
    }
            
    return 0;
}

int cast_api_aum_set_dis_aspect_mode(hccast_aum_screen_mode_t *screen_mode)
{
    hccast_um_param_t um_param;
    hccast_um_zoom_info_t um_zoom_info = {{0, 0, 1920, 1080}, {0, 0, 1920, 1080}};
    rotate_type_e rotate_seting_angle;
    rotate_type_e display_rotate = 0;
    int full_screen = 0;
    int flip_rotate = 0;
    int flip_mirror = 0;
    float ratio;

    if (!screen_mode)
    {
        return -1;
    }
    
    hccast_um_param_get(&um_param);
    api_get_flip_info(&flip_rotate, &flip_mirror);
    rotate_seting_angle = cast_api_um_rotate_angle_convert(um_param.screen_rotate_en);

    if (um_param.full_screen_en)
    {
        if (!screen_mode->mode)//VSCR
        {
            ratio = (float)screen_mode->screen_height / (float)screen_mode->screen_width;
            if ((rotate_seting_angle == ROTATE_TYPE_270) || (rotate_seting_angle == ROTATE_TYPE_90))
            {
                if (screen_mode->screen_height > 1920)
                {
                    if (ratio > 1.8)
                    {
                        if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                        {
                            um_zoom_info.src_rect.h = (screen_mode->screen_width * 1920) / screen_mode->screen_height;
                            um_zoom_info.src_rect.y = (1080 - um_zoom_info.src_rect.h) / 2;
                            
                        }
                        else
                        {
                            int tmp_h = (screen_mode->screen_width * 1920) / screen_mode->screen_height;
                            um_zoom_info.src_rect.w = tmp_h * 1920 / 1080; //when lcd is vscreen, height is trans to width.
                            um_zoom_info.src_rect.x = (1920 - um_zoom_info.src_rect.w) / 2;
                            um_zoom_info.src_rect.h = 1080;
                            um_zoom_info.src_rect.y = 0;
                        }
                    }
                    else
                    {
                        if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                        {
                            um_zoom_info.src_rect.w = (screen_mode->screen_height * 1080) / screen_mode->screen_width;
                            um_zoom_info.src_rect.x = (1920 - um_zoom_info.src_rect.w) / 2;
                        }
                        else
                        {
                            int tmp_w = (screen_mode->screen_height * 1080) / screen_mode->screen_width;
                            um_zoom_info.src_rect.h = tmp_w * 1080 / 1920;
                            um_zoom_info.src_rect.y = (1080 - um_zoom_info.src_rect.h) / 2;
                            um_zoom_info.src_rect.w = 1920;
                            um_zoom_info.src_rect.x = 0;
                        }
                    }

                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    cast_um_set_dis_zoom(&um_zoom_info);
                }
                else
                {
                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    cast_um_set_dis_zoom(&um_zoom_info);
                }
                full_screen = 1;
            }
            else
            {
                cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                cast_um_set_dis_zoom(&um_zoom_info);
                full_screen = 0;
            }
        }
        else if(screen_mode->mode)
        {   
            ratio = (float)screen_mode->screen_width / (float)screen_mode->screen_height;
            if ((rotate_seting_angle == ROTATE_TYPE_180) || (rotate_seting_angle == ROTATE_TYPE_0) || um_param.screen_rotate_auto)
            {
                if (screen_mode->screen_width > 1920)
                {
                    if (ratio > 1.8)
                    {		
                        if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                        {
                            um_zoom_info.src_rect.h = (screen_mode->screen_height * 1920) / screen_mode->screen_width;
                            um_zoom_info.src_rect.y = (1080 - um_zoom_info.src_rect.h) / 2;
                        }
                        else
                        {
                            int tmp_h = (screen_mode->screen_height * 1920) / screen_mode->screen_width;
                            um_zoom_info.src_rect.w = tmp_h * 1920 / 1080; //when lcd is vscreen, height is trans to width.
                            um_zoom_info.src_rect.x = (1920 - um_zoom_info.src_rect.w) / 2;
                            um_zoom_info.src_rect.h = 1080;
                            um_zoom_info.src_rect.y = 0;
                        }
                    }
                    else
                    {
                        if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                        {
                            um_zoom_info.src_rect.w = (screen_mode->screen_width * 1080) / screen_mode->screen_height;
                            um_zoom_info.src_rect.x = (1920 - um_zoom_info.src_rect.w) / 2;
                        }
                        else
                        {
                            int tmp_w = (screen_mode->screen_width * 1080) / screen_mode->screen_height;
                            um_zoom_info.src_rect.h = tmp_w * 1080 / 1920;
                            um_zoom_info.src_rect.y = (1080 - um_zoom_info.src_rect.h) / 2;
                            um_zoom_info.src_rect.w = 1920;
                            um_zoom_info.src_rect.x = 0;
                        }
                    }

                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    cast_um_set_dis_zoom(&um_zoom_info);
                } 
                else
                {
                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    cast_um_set_dis_zoom(&um_zoom_info);
                }
                full_screen = 1;
            }
            else
            {
                cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                cast_um_set_dis_zoom(&um_zoom_info);
                full_screen = 0;
            }
        }
    }
    else
    {
        cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
        cast_um_set_dis_zoom(&um_zoom_info);
        full_screen = 0;
    }

    if (!um_param.screen_rotate_en)
    {
        display_rotate = (ROTATE_TYPE_0 + flip_rotate) % 4;
    }
    else
    {
        if (um_param.screen_rotate_auto && screen_mode->mode)
        {
            display_rotate = (ROTATE_TYPE_0 + flip_rotate) % 4;
        }
        else
        {
            display_rotate = (flip_rotate + rotate_seting_angle) % 4;
        }
    }

    return 0;
}

static void ium_event_process_cb(int event, void *param1, void *param2)
{
    control_msg_t ctl_msg = {0};
    int got_data_len;
    int total_data_len ;
    app_data_t* app_data = data_mgr_app_get();
    static int g_vol = 0;

    if ((event != HCCAST_IUM_EVT_GET_FLIP_MODE) \
        && (event != HCCAST_IUM_EVT_UPG_DOWNLOAD_PROGRESS) \
        && (event != HCCAST_IUM_EVT_GET_ROTATION_INFO))
        printf("ium event: %d\n", event);

    switch (event)
    {
        case HCCAST_IUM_EVT_DEVICE_ADD:
            um_start_upgrade = 0;
            g_um_device_connected = 1;
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_DEV_ADD;
            break;
        case HCCAST_IUM_EVT_DEVICE_REMOVE:
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_DEV_REMOVE;
            g_um_device_connected = 0;
            break;
        case HCCAST_IUM_EVT_MIRROR_START:
            printf("%s(), line:%d. HCCAST_IUM_EVT_MIRROR_START\n", __func__, __LINE__);
            //hccast_scene_switch(HCCAST_SCENE_IUMIRROR);
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_START;
            g_um_device_connected = 1;
            network_service_enable_set(false);
            hccast_scene_set_mira_restart_flag(0);
            api_set_wireless_playback_enable(false);

            g_vol = cast_api_get_volume();
            cast_api_set_volume(100);
            g_ium_full_screen = app_data->um_full_screen;
            printf("%s set vol %d->100\n", __func__, g_vol);

            break;
        case HCCAST_IUM_EVT_MIRROR_STOP:
            printf("%s(), line:%d. HCCAST_IUM_EVT_MIRROR_STOP\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_STOP;
            g_um_device_connected = 0;
            network_service_enable_set(true);
            api_set_wireless_playback_enable(true);
            cast_api_set_volume(g_vol);
            cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);

            break;
        case HCCAST_IUM_EVT_SAVE_PAIR_DATA: //param1: buf; param2: length
        {
            char *pair_buf = (char *)param1;
            int len = (int)param2;
            if (len > 20 * 1024)
            {
                printf("%s %d: error \n", __func__, __LINE__);
                if (pair_buf)
                {
                    free(pair_buf);
                }

                return ;
            }

            data_mgr_ium_pdata_len_set(len);
            data_mgr_ium_pair_data_set(pair_buf);
            free(pair_buf);
            break;
        }
        case HCCAST_IUM_EVT_GET_PAIR_DATA: //param1: buf; param2: length
        {
            if (app_data->ium_pdata_len > 0)
            {
                char **pair_buf = (char **)param1;
                int *len = (int*)param2;
                if (pair_buf)
                {
                    char *buf = malloc(app_data->ium_pdata_len);
                    memcpy(buf, app_data->ium_pair_data, app_data->ium_pdata_len);
                    *pair_buf = buf;
                    if (len) *len = app_data->ium_pdata_len;
                }
            }

            break;
        }
        case HCCAST_IUM_EVT_NEED_USR_TRUST:
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_NEED_TRUST;
            break;
        case HCCAST_IUM_EVT_USR_TRUST_DEVICE:
            break;
        case HCCAST_IUM_EVT_CREATE_CONN_FAILED:
            break;
        case HCCAST_IUM_EVT_CANNOT_GET_AV_DATA:
            break;
        case HCCAST_IUM_EVT_UPG_DOWNLOAD_PROGRESS: //param1: data len; param2: file len

            if (um_start_upgrade == 0)
            {
                ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_START_UPGRADE;
                um_start_upgrade = 1;
            }
            else if (um_start_upgrade == 1)
            {
                got_data_len = (int)param1;
                total_data_len = (int)param2;
                ctl_msg.msg_type = MSG_TYPE_UPG_DOWNLOAD_PROGRESS;
                ctl_msg.msg_code = got_data_len * 100 / total_data_len;

                printf("HCCAST_IUM_EVT_UPG_DOWNLOAD_PROGRESS progress: %d\n", ctl_msg.msg_code);
            }

            break;
        case HCCAST_IUM_EVT_GET_UPGRADE_DATA: //param1: hccast_ium_upg_bo_t

            if (um_start_upgrade == 1)
            {
                pthread_t pid;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setstacksize(&attr, 0x2000);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                memcpy(&ium_upg_buf_info, param1, sizeof(ium_upg_buf_info));
                um_start_upgrade = 2;
                if (pthread_create(&pid, &attr, ium_burning_process, NULL) != 0)
                {
                    um_start_upgrade = 0;
                    cast_um_free_upg_buf();
                }   

                pthread_attr_destroy(&attr);
            }

            break;
        case HCCAST_IUM_EVT_SAVE_UUID:
            memcpy(app_data->ium_uuid, param1, 37);
            data_mgr_ium_uuid_set(app_data->ium_uuid);
            break;
        case HCCAST_IUM_EVT_CERT_INVALID:
            printf("[%s],line:%d. HCCAST_IUM_EVT_CERT_INVALID\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_AIR_INVALID_CERT;
            m_um_is_demo = true;
            break;
        case HCCAST_IUM_EVT_NO_DATA:
        {
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_NO_DATA;
            break;
        }
        case HCCAST_IUM_EVT_COPYRIGHT_PROTECTION:
        {
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_COPYRIGHT_PROTECTION;
            break;
        }
        case HCCAST_IUM_EVT_GET_UPGRADE_BUF:
        {
            hccast_ium_upg_bi_t *bi = (hccast_ium_upg_bi_t *)param1;
            if (bi)
            {
                bi->buf = cast_um_alloc_upg_buf(bi->len);
            }
            break;
        }    
        case HCCAST_IUM_EVT_SET_DIS_ASPECT:
            cast_api_ium_set_dis_aspect_mode();
            break;
        case HCCAST_IUM_EVT_GET_ROTATION_INFO:
            cast_api_ium_get_rotate_info((hccast_ium_screen_mode_t*)param1, (hccast_um_rotate_info_t*)param2);
            break;          
        default:
            break;
    }

    if (0 != ctl_msg.msg_type)
    {
        api_control_send_msg(&ctl_msg);
    }

    if (MSG_TYPE_CAST_IUSB_STOP == ctl_msg.msg_type)
    {
        //sometime it is still in cast play UI(not exit to win root UI),
        //the next dlna url play is starting, then the UI/logo may block the dlna playing.
        //So here exit callback function wait for win cast root UI opening
        printf("[%s] wait cast root start tick: %d\n",__func__,(int)time(NULL));
        bool win_cast_root_wait_open(uint32_t timeout);
        win_cast_root_wait_open(20000);
        printf("[%s] wait cast root end tick: %d\n",__func__,(int)time(NULL));
    }

    if (MSG_TYPE_CAST_IUSB_START == ctl_msg.msg_type)
    {
        printf("[%s] wait um play start tick: %d\n",__func__,(int)time(NULL));
        bool win_um_play_wait_open(uint32_t timeout);
        win_um_play_wait_open(20000);
        printf("[%s] wait um play end tick: %d\n",__func__,(int)time(NULL));
#if CASTING_CLOSE_FB_SUPPORT	
        api_osd_show_onoff(false);
#endif
    }
}

static void aum_event_process_cb(int event, void *param1, void *param2)
{
    control_msg_t ctl_msg = {0};
    int got_data_len;
    int total_data_len ;
    app_data_t* app_data = data_mgr_app_get();

    if ((HCCAST_AUM_EVT_GET_FLIP_MODE != event) \
        && (HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS != event) \
        &&(HCCAST_AUM_EVT_GET_ROTATION_INFO != event))
        printf("aum event: %d\n", event);

    switch (event)
    {
        case HCCAST_AUM_EVT_DEVICE_ADD:
            printf("%s(), line:%d. HCCAST_AUM_EVT_DEVICE_ADD\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_DEV_ADD;
            um_start_upgrade = 0;
            g_um_device_connected = 1;
            
            network_service_enable_set(false);
            hccast_scene_set_mira_restart_flag(0);
            api_set_wireless_playback_enable(false);
            network_stop_services();
            api_logo_off2(0, 0);
            break;
        case HCCAST_AUM_EVT_DEVICE_REMOVE:
            printf("%s(), line:%d. HCCAST_AUM_EVT_DEVICE_REMOVE\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_DEV_REMOVE;
            g_um_device_connected = 0;
 
            network_service_enable_set(true);
            api_set_wireless_playback_enable(true);
            network_start_services();
            break;
        case HCCAST_AUM_EVT_MIRROR_START:
            printf("%s(), line:%d. HCCAST_AUM_EVT_MIRROR_START\n", __func__, __LINE__);
            //hccast_scene_switch(HCCAST_SCENE_AUMIRROR);
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_START;
            api_osd_off_time(1000);
            break;
        case HCCAST_AUM_EVT_MIRROR_STOP:
        {
            printf("%s(), line:%d. HCCAST_AUM_EVT_MIRROR_STOP\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_STOP;
            hccast_um_zoom_info_t um_zoom_info = {{0, 0, 1920, 1080}, {0, 0, 1920, 1080}};
            cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
            cast_um_set_dis_zoom(&um_zoom_info);
            break;
        }    
        case HCCAST_AUM_EVT_IGNORE_NEW_DEVICE:
            break;
        case HCCAST_AUM_EVT_SERVER_MSG:
            break;
        case HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS:

            if (um_start_upgrade == 0)
            {
                ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_START_UPGRADE;
                um_start_upgrade = 1;
            }
            else if (um_start_upgrade == 1)
            {
                got_data_len = (int)param1;
                total_data_len = (int)param2;
                ctl_msg.msg_type = MSG_TYPE_UPG_DOWNLOAD_PROGRESS;
                ctl_msg.msg_code = got_data_len * 100 / total_data_len;

                printf("HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS progress: %d\n", ctl_msg.msg_code);
            }

            break;
        case HCCAST_AUM_EVT_GET_UPGRADE_DATA:

            if (um_start_upgrade == 1)
            {
                pthread_t pid;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setstacksize(&attr, 0x2000);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                memcpy(&aum_upg_buf_info, param1, sizeof(aum_upg_buf_info));
                um_start_upgrade = 2;
                if (pthread_create(&pid, &attr, aum_burning_process, NULL) != 0)
                {
                    um_start_upgrade = 0;
                    cast_um_free_upg_buf();
                }

                pthread_attr_destroy(&attr);
            }

            break;
        case HCCAST_AUM_EVT_SET_SCREEN_ROTATE:
            data_mgr_cast_rotation_set((int)param1);
            break;
        case HCCAST_AUM_EVT_SET_AUTO_ROTATE:
            break;
        case HCCAST_AUM_EVT_SET_FULL_SCREEN:
            data_mgr_cast_um_full_screen_set((int)param1);
            break;
        case HCCAST_AUM_EVT_GET_UPGRADE_BUF:
        {
            hccast_aum_upg_bi_t *bi = (hccast_aum_upg_bi_t *)param1;
            if (bi)
            {
                bi->buf = cast_um_alloc_upg_buf(bi->len);
            }
            break;
        }    
        case HCCAST_AUM_EVT_SET_DIS_ASPECT:
            cast_api_aum_set_dis_aspect_mode((hccast_aum_screen_mode_t*)param1);
            break;
        case HCCAST_AUM_EVT_GET_ROTATION_INFO:
            cast_api_aum_get_rotate_info((hccast_aum_screen_mode_t*)param1, (hccast_um_rotate_info_t*)param2);
            break;    
        default:
            break;
    }
    if (0 != ctl_msg.msg_type)
    {
        api_control_send_msg(&ctl_msg);
    }
    
    if (MSG_TYPE_CAST_AUSB_STOP == ctl_msg.msg_type)
    {
        //sometime it is still in cast play UI(not exit to win root UI),
        //the next dlna url play is starting, then the UI/logo may block the dlna playing.
        //So here exit callback function wait for win cast root UI opening
        printf("[%s] wait cast root start tick: %d\n",__func__,(int)time(NULL));
        bool win_cast_root_wait_open(uint32_t timeout);
        win_cast_root_wait_open(20000);
        printf("[%s] wait cast root end tick: %d\n",__func__,(int)time(NULL));
    }

    if (MSG_TYPE_CAST_AUSB_START == ctl_msg.msg_type)
    {
        printf("[%s] wait um play start tick: %d\n",__func__,(int)time(NULL));
        bool win_um_play_wait_open(uint32_t timeout);
        win_um_play_wait_open(20000);
        printf("[%s] wait um play end tick: %d\n",__func__,(int)time(NULL));
#if CASTING_CLOSE_FB_SUPPORT	
        api_osd_show_onoff(false);
#endif
    }
    
}

int cast_usb_mirror_init(void)
{
    hccast_um_param_t um_param = {0};

    if (hccast_um_init() < 0)
    {
        printf("%s(), line:%d. hccast_um_init() fail!\n", __func__, __LINE__);
        return API_FAILURE;
    }

    if (data_mgr_cast_rotation_get())
        um_param.screen_rotate_en = 1;
    else
        um_param.screen_rotate_en = 0;

    um_param.screen_rotate_auto = 1;
    um_param.full_screen_en = data_mgr_cast_um_full_screen_get();
    hccast_um_param_set(&um_param);

    hccast_ium_init(ium_event_process_cb);
    
#if defined(SUPPORT_UM_SLAVE)
    hccast_aum_set_resolution(HCCAST_AUM_RES_AUTO);
    hccast_aum_init(aum_event_process_cb);
#endif

    return API_SUCCESS;
}

int cast_usb_mirror_deinit(void)
{
    if (hccast_um_deinit() < 0)
        return API_FAILURE;
    else
        return API_SUCCESS;
}

#ifdef __linux__
#define AUM_UPG_URL "http://172.16.12.81/hccast/linux/%s/hcscreenhybrid/HCFOTA.json"
#else
#define AUM_UPG_URL "http://172.16.12.81/hccast/rtos/%s/hcscreenhybrid/HCFOTA.json"
#endif
#define AUM_APK_URL "http://119.3.89.190:8080/apk/hccast_3.2.2.apk"

int cast_usb_mirror_start(void)
{
    int ret;
    hccast_aum_param_t aum_param = {0};
    sys_data_t* sys_data = data_mgr_sys_get();
    app_data_t* app_data = data_mgr_app_get();
    char *udc_name = NULL;
    hccast_um_dev_type_e gadget_type = HCCAST_UM_DEV_IUM;

    if (!g_um_frame_buf)
    {
        g_um_frame_buf = malloc(DEFAULT_FRM_SIZE);
        g_um_frame_size = DEFAULT_FRM_SIZE;
    }

    int np, port_num = -1;
    np = fdt_node_probe_by_path("/hcrtos/usb_slave_port");
    if (np >= 0)
    {
        fdt_get_property_u_32_index(np, "port_num", 0, &port_num);
    }

    hccast_ium_set_frm_buf(g_um_frame_buf, g_um_frame_size);

    ret = hccast_ium_start(app_data->ium_uuid, ium_event_process_cb);
    if (ret)
    {
        printf("%s(), line:%d. ret = %d\n", __func__, __LINE__, ret);
        return API_FAILURE;
    }

    strcat(aum_param.product_id, (char*)sys_data->product_id);//HCT-AT01
    sprintf(aum_param.fw_url, AUM_UPG_URL, (char*)sys_data->product_id);
    strcat(aum_param.apk_url, AUM_APK_URL);
    strcat(aum_param.aoa_desc, "ElfCast-Screen_Mirror");
    aum_param.fw_version = (unsigned int)sys_data->firmware_version;

    hccast_aum_set_frm_buf(g_um_frame_buf, g_um_frame_size);
    ret = hccast_aum_start(&aum_param, aum_event_process_cb);
    if (ret)
    {
        printf("%s(), line:%d. ret = %d\n", __func__, __LINE__, ret);
        return API_FAILURE;
    }

#ifdef SUPPORT_UM_SLAVE
    if ((0 == port_num) || (1 == port_num))
    {
        g_slave_port = port_num;
    }
    else
    {
        g_slave_port = 0;
    }
    
    udc_name = get_udc_name(g_slave_port);

#if defined(SUPPORT_UAC_GADGET)
    gadget_type = HCCAST_UM_DEV_IUM_UAC;
    hccast_aum_slave_start(g_slave_port, udc_name, gadget_type);
#endif
    hccast_ium_slave_start(g_slave_port, udc_name, gadget_type);
#endif

    return API_SUCCESS;
}

int cast_usb_mirror_stop(void)
{
    hccast_ium_stop();
#ifdef SUPPORT_UM_SLAVE
    if ((0 == g_slave_port) || (1 == g_slave_port))
    {
        hccast_ium_slave_stop(g_slave_port);
    }
#endif
    hccast_ium_set_frm_buf(NULL, 0);

#ifdef SUPPORT_UM_SLAVE
    if ((0 == g_slave_port) || (1 == g_slave_port))
    {
        hccast_aum_slave_stop(g_slave_port);
    }
#endif

    hccast_aum_stop();
    hccast_aum_set_frm_buf(NULL, 0);

    cast_um_free_upg_buf();

    if (g_um_frame_buf)
    {
        free(g_um_frame_buf);
        g_um_frame_buf = NULL;
        g_um_frame_size = 0;
    }

    return API_SUCCESS;
}
#endif

int cast_api_mira_set_default_res(void)
{
#ifdef MIRACAST_SUPPORT
#ifdef SOC_HC15XX
    hccast_mira_res_e res = HCCAST_MIRA_RES_1080P30; // HCCAST_MIRA_RES_720P30
#else
    hccast_mira_res_e res = HCCAST_MIRA_RES_1080P60; // HCCAST_MIRA_RES_720P30
#endif

    hccast_mira_service_set_resolution(res);
#endif

    return 0;
}

int cast_init(void)
{
#ifdef DLNA_SUPPORT
    hccast_dlna_service_init(hccast_dlna_callback_func);
#ifdef DIAL_SUPPORT
    hccast_dial_service_init(hccast_dial_callback_func);
#endif
#endif

#ifdef MIRACAST_SUPPORT
    hccast_mira_service_init(hccast_mira_callback_func);
    if (data_mgr_de_tv_sys_get() >= TV_LINE_4096X2160_30)
    {
        hccast_mira_service_set_resolution(HCCAST_MIRA_RES_1080F30);
    }
    else
    {
        cast_api_mira_set_default_res();
    }
#endif

#ifdef CAST_PHOTO_DETECT
    hccast_air_set_resolution(1280, 960, 60);
#endif
    hccast_scene_init();
    api_set_aspect_mode(1, 3,DIS_SCALE_ACTIVE_IMMEDIATELY);//16:9 as default.

    return API_SUCCESS;
}

int cast_deinit(void)
{
#ifdef DLNA_SUPPORT
    hccast_dlna_service_stop();
    hccast_dlna_service_uninit();
#ifdef DIAL_SUPPORT
    hccast_dial_service_stop();
    hccast_dial_service_uninit();
#endif
#endif

#ifdef MIRACAST_SUPPORT
    hccast_mira_service_stop();
    hccast_mira_service_uninit();
#endif

#ifdef AIRCAST_SUPPORT
    hccast_air_service_stop();
#endif

#ifdef USBMIRROR_SUPPORT
    cast_usb_mirror_stop();
    cast_usb_mirror_deinit();
#endif

    return API_SUCCESS;
}

/*
static int cast_get_wifi_mac(unsigned char *mac)
{
    int ret = 0;
    int sock, if_count, i;
    struct ifconf ifc;
    struct ifreq ifr[10];

    if (!mac)
    {
        return 0;
    }

    memset(&ifc, 0, sizeof(struct ifconf));

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    ifc.ifc_len = 10 * sizeof(struct ifreq);
    ifc.ifc_buf = ifr;
    ioctl(sock, SIOCGIFCONF, &ifc);

    if_count = ifc.ifc_len / sizeof(struct ifreq);

    for (i = 0; i < if_count; i ++)
    {
        if (ioctl(sock, SIOCGIFHWADDR, &ifr[i]) == 0)
        {
            memcpy(mac, ifr[i].ifr_hwaddr.sa_data, 6);
            if (!strcmp(ifr[i].ifr_name, "wlan0"))
            {
                return 1;
            }
        }
    }

    return 0;
}
*/

#ifdef DLNA_SUPPORT
int hccast_dlna_callback_func(hccast_dlna_event_e event, void* in, void* out)
{
    app_log(LL_INFO, "[%s] event: %d", __func__,event);
    char *str_tmp = NULL;
    control_msg_t ctl_msg = {0};
    sys_data_t* sys_data = data_mgr_sys_get();

    switch (event)
    {
        case HCCAST_DLNA_GET_DEVICE_NAME:
        {
            printf("[%s]HCCAST_DLNA_GET_DEVICE_NAME\n",__func__);
            if (in)
            {
                str_tmp = data_mgr_get_device_name();
                if (str_tmp)
                {
                    sprintf((char *)in, "%s_dlna", str_tmp);
                    printf("[%s]HCCAST_DLNA_GET_DEVICE_NAME:%s\n",__func__, str_tmp);
                }
            }
            break;
        }
        case HCCAST_DLNA_GET_DEVICE_PARAM:
        {
            hccast_dlna_param *dlna = (hccast_dlna_param*)in;
            if (dlna)
            {
                static char service_name[DLNA_SERVICE_NAME_LEN] = "hccast_dlna";
                str_tmp = data_mgr_get_device_name();
                if (str_tmp)
                {
                    sprintf((char *)service_name, "%s_dlna", str_tmp);
                    printf("[%s]HCCAST_DLNA_GET_DEVICE_PARAM:%s\n",__func__, str_tmp);
                }

                dlna->svrname = service_name;
                dlna->svrport = DLNA_UPNP_PORT; // need > 49152
                dlna->ifname  = DLNA_BIND_IFNAME;
            }
            break;
        }
        case HCCAST_DLNA_GET_HOSTAP_STATE:
#ifdef NETWORK_SUPPORT
#ifdef WIFI_SUPPORT
            *(int*)in = hccast_wifi_mgr_get_hostap_status();
#endif
#endif
            break;
        case HCCAST_DLNA_HOSTAP_MODE_SKIP_URL:
            ctl_msg.msg_type = MSG_TYPE_DLNA_HOSTAP_SKIP_URL;
            break;
        case HCCAST_DLNA_GET_SAVE_AUDIO_VOL:
            *(int*)in = sys_data->volume;
            break;
        default:
            break;
    }

    if (0 != ctl_msg.msg_type)
        api_control_send_msg(&ctl_msg);

    return 0;
}
#endif

#ifdef DIAL_SUPPORT
int hccast_dial_callback_func(hccast_dial_event_e event, void *in, void *out)
{
    app_log(LL_WARNING, "event: %d", event);
    char *str_tmp = NULL;
    control_msg_t ctl_msg = {0};

    switch (event)
    {
        case HCCAST_DIAL_GET_SVR_NAME:
        {
            if (in)
            {
                str_tmp = data_mgr_get_device_name();
                if (str_tmp)
                {
                    sprintf((char *)in, "%s_dial", str_tmp);
                    app_log(LL_INFO, "HCCAST_DIAL_GET_DEVICE_NAME:%s\n", str_tmp);
                }
            }

            break;
        }

        case HCCAST_DIAL_CONN_CONNECTING:
        {
            app_log(LL_INFO, "[%s] HCCAST_DIAL_CONN_CONNECTING", __func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_DIAL_CONNECTING;
            m_dial_conn_state = CAST_DIAL_CONN_CONNECTING;
            break;
        }

        case HCCAST_DIAL_CONN_CONNECTED:
        {
            app_log(LL_DEBUG, "[%s] HCCAST_DIAL_CONN_CONNECTED", __func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_DIAL_CONNECTED;
            m_dial_conn_state = CAST_DIAL_CONN_CONNECTED;
            break;
        }

        case HCCAST_DIAL_CONN_CONNECTED_FAILED:
        {
            app_log(LL_DEBUG, "[%s] HCCAST_DIAL_CONN_CONNECTED_FAILED", __func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_DIAL_CONNECTED_FAILED;
            break;
        }

        case HCCAST_DIAL_CONN_DISCONNECTED:
        {
            app_log(LL_DEBUG, "[%s] HCCAST_DIAL_CONN_DISCONNECTED", __func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_DIAL_DISCONNECTED;
            break;
        }

        case HCCAST_DIAL_CONN_DISCONNECTED_ALL:
        {
            app_log(LL_DEBUG, "[%s] HCCAST_DIAL_CONN_DISCONNECTED_ALL", __func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_DIAL_DISCONNECTED_ALL;
            m_dial_conn_state = CAST_DIAL_CONN_NONE;
            break;
        }

        case HCCAST_DIAL_CTRL_INVALID_CERT:
        {
            app_log(LL_WARNING, "[%s] HCCAST_DIAL_CTRL_INVALID_CERT", __func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_DIAL_INVALID_CERT;
            m_dial_is_demo = true;
            break;
        }

        default:
            break;
    }

    if (0 != ctl_msg.msg_type)
    {
        api_control_send_msg(&ctl_msg);
    }

    return 0;
}
#endif

#ifdef MIRACAST_SUPPORT
static int g_mira_enable_vrotation = 0;//v_screen enable
static int g_mira_v_screen = 0;
static av_area_t g_mira_picture_info = { 0, 0, 1920, 1080};
static int g_mira_pic_vdec_w;
static int g_mira_pic_vdec_h;
static int g_mira_full_vscreen = 0;
static int g_mira_force_detect_en = 0;

void cast_mira_set_dis_zoom(hccast_mira_zoom_info_t *mira_zoom_info)
{
    av_area_t src_rect;
    av_area_t dst_rect;
    int dis_active_mode;

    memcpy(&src_rect, &mira_zoom_info->src_rect, sizeof(av_area_t));
    memcpy(&dst_rect, &mira_zoom_info->dst_rect, sizeof(av_area_t));
    dis_active_mode = mira_zoom_info->dis_active_mode;

    cast_api_set_dis_zoom(&src_rect, &dst_rect, dis_active_mode);
}

void cast_api_mira_reset_aspect_mode()
{
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};

    cast_mira_set_dis_zoom(&mira_zoom_info);
    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
}

static int cast_api_mira_flip_rotate_covert(int flip_mode, int rotation)
{
    int flip_mode_0[4] = {ROTATE_TYPE_0, ROTATE_TYPE_270, ROTATE_TYPE_90, ROTATE_TYPE_180};
    int flip_mode_90[4] = {ROTATE_TYPE_90, ROTATE_TYPE_0, ROTATE_TYPE_180, ROTATE_TYPE_270};
    int flip_mode_180[4] = {ROTATE_TYPE_180, ROTATE_TYPE_90, ROTATE_TYPE_270, ROTATE_TYPE_0};
    int flip_mode_270[4] = {ROTATE_TYPE_270, ROTATE_TYPE_180, ROTATE_TYPE_0, ROTATE_TYPE_90};

    if (ROTATE_TYPE_0 == flip_mode)
    {
        return flip_mode_0[rotation];
    }
    else if (ROTATE_TYPE_90 == flip_mode)
    {
        return flip_mode_90[rotation];
    }
    else if (ROTATE_TYPE_180 == flip_mode)
    {
        return flip_mode_180[rotation];
    }
    else if (ROTATE_TYPE_270 == flip_mode)
    {
        return flip_mode_270[rotation];
    }

    return 0;
}

void cast_api_mira_vscreen_detect_enable(int enable)
{
    struct dis_miracast_vscreen_detect_param mpara = { 0 };
    int fd = -1;

    fd = open("/dev/dis", O_RDWR);
    if (fd < 0)
    {
        return ;
    }

    mpara.distype = DIS_TYPE_HD;
    if (enable)
    {
        mpara.on = 1;
    }
    else
    {
        mpara.on = 0;
    }

#ifdef CAST_PHOTO_DETECT
    mpara.cast_photo_detect = true;
#endif

    ioctl(fd, DIS_SET_MIRACAST_VSRCEEN_DETECT, &mpara);

    close(fd);
}

void cast_api_get_mira_picture_area(av_area_t *src_rect)
{
    int fd = open("/dev/dis" , O_RDWR);
    if(fd < 0)
        return;

    dis_screen_info_t picture_info = { 0 };

    picture_info.distype = DIS_TYPE_HD;
    ioctl(fd , DIS_GET_MIRACAST_PICTURE_ARER , &picture_info);
    src_rect->x = picture_info.area.x;
    src_rect->y = picture_info.area.y;
    src_rect->w = picture_info.area.w;
    src_rect->h = picture_info.area.h;

    printf("%s %d %d %d %d\n",__FUNCTION__, 
           src_rect->x , 
           src_rect->y, 
           src_rect->w, 
           src_rect->h);
    close(fd);
}

int cast_api_mira_get_current_pic_info(struct dis_display_info *mpinfo)
{
    int fd = -1;

    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0)
    {
        return -1;
    }

    mpinfo->distype = DIS_TYPE_HD;
    mpinfo->info.layer = DIS_PIC_LAYER_MAIN;
    ioctl(fd , DIS_GET_DISPLAY_INFO , (uint32_t)mpinfo);
    close(fd);
    return 0;
}

int cast_api_mira_get_video_info(int *width, int *heigth)
{
    struct dis_display_info mpinfo = {0};

    cast_api_mira_get_current_pic_info(&mpinfo);

    if ((!mpinfo.info.pic_height) || (!mpinfo.info.pic_width))
    {
        printf("mpinfo param error.\n");
        return -1;
    }


    if (mpinfo.info.rotate_mode == ROTATE_TYPE_90 ||
            mpinfo.info.rotate_mode == ROTATE_TYPE_270)
    {
        *width = mpinfo.info.pic_height;//mpinfo.info.pic_dis_area.h;
        *heigth = mpinfo.info.pic_width;//mpinfo.info.pic_dis_area.w;
    }
    else
    {
        *width = mpinfo.info.pic_width;//mpinfo.info.pic_dis_area.w;
        *heigth = mpinfo.info.pic_height;//mpinfo.info.pic_dis_area.h;
    }

    printf("video info: rotate=%d, w:%ld, h:%ld\n",
           mpinfo.info.rotate_mode, *width, *heigth);

    return 0;
}

static int cast_mira_get_dis_rotate(int *dis_rotate)
{
    int fd = -1;
    int ret = 0;
    struct dis_display_info dis_info;
    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0)
    {
        return -1;
    }
    dis_info.distype = DIS_TYPE_HD;
    dis_info.info.layer = DIS_PIC_LAYER_MAIN;
    ret = ioctl(fd, DIS_GET_DISPLAY_INFO, &dis_info);
    *dis_rotate = dis_info.info.rotate_mode;
    close(fd);
    return ret;
}
int cast_api_mira_reset_vscreen_zoom()
{
    int width_ori = g_mira_pic_vdec_w;
    int flip_rotate = 0;
    int flip_mirror = 0;
    api_get_flip_info(&flip_rotate, &flip_mirror);
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};

    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
    
    if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
    {
        mira_zoom_info.src_rect.x = 0;
        mira_zoom_info.src_rect.y = (g_mira_picture_info.x * 1080) / width_ori;
        mira_zoom_info.src_rect.w = 1920;
        mira_zoom_info.src_rect.h = 1080 - 2 * mira_zoom_info.src_rect.y;
        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
        cast_mira_set_dis_zoom(&mira_zoom_info);
    }
    else
    {
        mira_zoom_info.src_rect.x = (g_mira_picture_info.x * 1920) / width_ori;
        mira_zoom_info.src_rect.y = 0;
        mira_zoom_info.src_rect.w = 1920 - 2 * mira_zoom_info.src_rect.x;
        mira_zoom_info.src_rect.h = 1080;
        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
        cast_mira_set_dis_zoom(&mira_zoom_info);
    }
    
    printf("%s\n", __func__);
    
    return 0;   
}

int cast_api_mira_process_rotation_change()
{
    int temp = 0;
    int rotate = 0;
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};
    app_data_t * app_data = data_mgr_app_get();
    int full_vscreen = app_data->mirror_full_vscreen;

    rotate = app_data->mirror_rotation;
    if ((rotate == MIRROR_ROTATE_90) || (rotate == MIRROR_ROTATE_270))
    {
        temp = 1;
    }
    else
    {
        temp = 0;
    }

    //for avoid every time will call back the hccast_mira_vscreen_detect_enable.
    if (temp != g_mira_enable_vrotation)
    {
        if (temp)
        {
            g_mira_enable_vrotation = 1;

            if (g_mira_force_detect_en)
            {
                if (g_mira_v_screen)
                {
                    if (full_vscreen)
                    {
                        cast_api_mira_reset_vscreen_zoom();
                    }
                    else
                    {
                        cast_api_set_aspect_mode(DIS_TV_16_9 , DIS_VERTICALCUT , DIS_SCALE_ACTIVE_IMMEDIATELY);
                    }      
                }
            }
            else
            {
                cast_api_mira_vscreen_detect_enable(1);
            }
        }
        else
        {
            if (!app_data->mirror_vscreen_auto_rotation || !full_vscreen)
            {
                cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
            }

            if (!g_mira_force_detect_en)      
            {
                cast_api_mira_vscreen_detect_enable(0);
                g_mira_v_screen = 0;
            }
            
            cast_mira_set_dis_zoom(&mira_zoom_info);
            g_mira_enable_vrotation = 0;
        }
    }

    if (g_mira_full_vscreen != full_vscreen && g_mira_enable_vrotation)
    {
        if (g_mira_v_screen && full_vscreen)
        {
            cast_api_mira_reset_vscreen_zoom();
        }
        else if(g_mira_v_screen && !full_vscreen)
        {
            cast_mira_set_dis_zoom(&mira_zoom_info);
            cast_api_set_aspect_mode(DIS_TV_16_9 , DIS_VERTICALCUT , DIS_SCALE_ACTIVE_IMMEDIATELY);
        }
        
        g_mira_full_vscreen = full_vscreen;
    }

    return rotate;
}

int cast_api_mira_get_rotation_info(hccast_mira_rotation_t* rotate_info)
{
    int seting_rotate;
    int rotation_angle = MIRROR_ROTATE_0;
    int flip_mode;
    int flip_rotate;
    app_data_t * app_data = data_mgr_app_get();

    if (!rotate_info)
    {
        return -1;
    }
      
    seting_rotate = cast_api_mira_process_rotation_change();
    api_get_flip_info(&flip_rotate, &flip_mode);

    if (g_mira_enable_vrotation)
    {
        if (g_mira_v_screen)
        {
            rotation_angle = cast_api_mira_flip_rotate_covert(flip_rotate, seting_rotate);
        }
        else
        {
            if (app_data->mirror_vscreen_auto_rotation)
            {
                rotation_angle = flip_rotate;
            }
            else
                {
                rotation_angle = cast_api_mira_flip_rotate_covert(flip_rotate, seting_rotate);
            }
        }
    }
    else
    {
        rotation_angle = cast_api_mira_flip_rotate_covert(flip_rotate, seting_rotate);
    }

    rotate_info->rotate_angle = rotation_angle;
    rotate_info->flip_mode = flip_mode;

    return 0;
}

int cast_api_mira_screen_detect_handle(int is_v_screen)
{
    int width_ori = 1920;       //MIRACAST CAST SEND
    int width_height = 1080;    //MIRACAST CAST SEND
    int flip_rotate = 0;
    int flip_mirror = 0;
    av_area_t m_mira_picture_info = {0};
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};  
    app_data_t * app_data = data_mgr_app_get();
    int full_vscreen = app_data->mirror_full_vscreen;
    int vscreen_auto_rotation = app_data->mirror_vscreen_auto_rotation;
    api_get_flip_info(&flip_rotate, &flip_mirror);

    if(cast_api_mira_get_video_info(&g_mira_pic_vdec_w,  &g_mira_pic_vdec_h) < 0)
    {
        printf("get video info fail.\n"); 
        return 0;
    }

    memcpy(&m_mira_picture_info, &g_mira_picture_info, sizeof(g_mira_picture_info));
    cast_api_get_mira_picture_area(&g_mira_picture_info);

#ifndef CAST_PHOTO_DETECT
    if (g_mira_picture_info.x >= ((((g_mira_pic_vdec_w + 15) >> 4) / 3) << 4))
    {
        is_v_screen = 1;
    }
    else
    {
        is_v_screen = 0;
    }
#else
    if (abs(g_mira_picture_info.w - g_mira_picture_info.h) <= 8)
    {
        printf("1:1 picture.\n");
        is_v_screen = 0;
    }
#endif

    if (is_v_screen)
    {
        printf("V_SCR\n");
        int last_vscreen = g_mira_v_screen;
        g_mira_v_screen = 1;

        if (g_mira_enable_vrotation)
        {
            if (!full_vscreen)
            {
                cast_mira_set_dis_zoom(&mira_zoom_info);
                cast_api_set_aspect_mode(DIS_TV_16_9, DIS_VERTICALCUT, DIS_SCALE_ACTIVE_NEXTFRAME);
            }
            else
            {
                width_ori = g_mira_pic_vdec_w;
                if (vscreen_auto_rotation)
                {
                    if (last_vscreen)
                    {
                        int dis_rotate;
                        int expect_rotate = cast_api_mira_flip_rotate_covert(flip_rotate, app_data->mirror_rotation);
                        if (cast_mira_get_dis_rotate(&dis_rotate) < 0)
                        {
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                        }
                        else
                        {
                            if (dis_rotate == expect_rotate)
                            {
                                mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                            }
                            else
                            {
                                mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                            }
                        }
                    }
                    else
                    {
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                    }

                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_NEXTFRAME);
                }
                else
                {
                    mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                }

                if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                {
                    mira_zoom_info.src_rect.x = 0;
                    mira_zoom_info.src_rect.y = (g_mira_picture_info.x * 1080) / width_ori;
                    mira_zoom_info.src_rect.w = 1920;
                    mira_zoom_info.src_rect.h = 1080 - 2 * mira_zoom_info.src_rect.y;
                    cast_mira_set_dis_zoom(&mira_zoom_info);
                }
                else
                {
                    mira_zoom_info.src_rect.x = (g_mira_picture_info.x * 1920) / width_ori;
                    mira_zoom_info.src_rect.y = 0;
                    mira_zoom_info.src_rect.w = 1920 - 2 * mira_zoom_info.src_rect.x;
                    mira_zoom_info.src_rect.h = 1080;
                    cast_mira_set_dis_zoom(&mira_zoom_info);
                }
            }
        }
        else
        {
            cast_mira_set_dis_zoom(&mira_zoom_info);
        }
    }
    else
    {
        printf("H_SCR\n");
        int last_vscreen = g_mira_v_screen;
        g_mira_v_screen = 0;
        
        if (g_mira_enable_vrotation)
        {
            if (!full_vscreen)
            {
                if (vscreen_auto_rotation)
                {
#ifdef CAST_PHOTO_DETECT                
                    if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                    {
                        mira_zoom_info.src_rect.x = (g_mira_picture_info.x * 1920) / g_mira_pic_vdec_w;
                        mira_zoom_info.src_rect.y = (g_mira_picture_info.y * 1080) / g_mira_pic_vdec_h;
                        mira_zoom_info.src_rect.w = 1920 - (2 * mira_zoom_info.src_rect.x);
                        mira_zoom_info.src_rect.h = 1080 - (2 * mira_zoom_info.src_rect.y);

                        float w_ratio = 1920/(float)mira_zoom_info.src_rect.w;
                        float h_ratio = 1080/(float)mira_zoom_info.src_rect.h;

                        if (w_ratio > h_ratio)
                        {   
                            mira_zoom_info.dst_rect.w = mira_zoom_info.src_rect.w*h_ratio;
                            mira_zoom_info.dst_rect.x = (1920 - mira_zoom_info.dst_rect.w)/2;
                        }
                        else
                        {
                            mira_zoom_info.dst_rect.h = mira_zoom_info.src_rect.h*w_ratio;
                            mira_zoom_info.dst_rect.y = (1080 - mira_zoom_info.dst_rect.h)/2;
                        }
                    }
                    else
                    {
                        mira_zoom_info.src_rect.x = (1920 * g_mira_picture_info.y) / 1080;
                        mira_zoom_info.src_rect.w = 1920 - mira_zoom_info.src_rect.x * 2;
                        mira_zoom_info.src_rect.y = 1080 * g_mira_picture_info.x / 1920;
                        mira_zoom_info.src_rect.h = 1080 - mira_zoom_info.src_rect.y * 2;
                        float h_ratio = 1920/(float)mira_zoom_info.src_rect.w;  //H
                        float w_ratio = 1080/(float)mira_zoom_info.src_rect.h;  //W

                        if (w_ratio > h_ratio)
                        {
                            mira_zoom_info.dst_rect.h = mira_zoom_info.src_rect.h*h_ratio;
                            mira_zoom_info.dst_rect.y = (1080 - mira_zoom_info.dst_rect.h)/2;
                        }
                        else
                        {
                            mira_zoom_info.dst_rect.w = mira_zoom_info.src_rect.w*w_ratio;
                            mira_zoom_info.dst_rect.x = (1920 - mira_zoom_info.dst_rect.w)/2;
                        }
                    }
                    
                    cast_mira_set_dis_zoom(&mira_zoom_info);
                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
#else
                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
#endif                    
                }
                else
                {
                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                }        
            }
            else
            {
                if (vscreen_auto_rotation)
                {
#ifdef CAST_PHOTO_DETECT 
                    if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                    {
                        mira_zoom_info.src_rect.x = (g_mira_picture_info.x * 1920) / g_mira_pic_vdec_w;
                        mira_zoom_info.src_rect.y = (g_mira_picture_info.y * 1080) / g_mira_pic_vdec_h;
                        mira_zoom_info.src_rect.w = 1920 - (2 * mira_zoom_info.src_rect.x);
                        mira_zoom_info.src_rect.h = 1080 - (2 * mira_zoom_info.src_rect.y);
                    }
                    else
                    {
                        mira_zoom_info.src_rect.x = (1920 * g_mira_picture_info.y) / 1080;
                        mira_zoom_info.src_rect.w = 1920 - mira_zoom_info.src_rect.x * 2;
                        mira_zoom_info.src_rect.y = 1080 * g_mira_picture_info.x / 1920;
                        mira_zoom_info.src_rect.h = 1080 - mira_zoom_info.src_rect.y * 2;
                    }

                    if (last_vscreen)
                    {
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                    }
                    else
                    {
                        int dis_rotate;
                        int expect_rotate = flip_rotate;
                        if (cast_mira_get_dis_rotate(&dis_rotate) < 0)
                        {
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                        }
                        else
                        {
                            if (dis_rotate == expect_rotate)
                            {
                                mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                            }
                            else
                            {
                                mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                            }
                        }
                    }

                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, mira_zoom_info.dis_active_mode);
                    cast_mira_set_dis_zoom(&mira_zoom_info);
#else       
                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
                    mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                    cast_mira_set_dis_zoom(&mira_zoom_info);
#endif     
                }
                else
                {
#ifdef CAST_PHOTO_DETECT
                    if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                    {
                        mira_zoom_info.src_rect.x = 0;
                        mira_zoom_info.src_rect.y = (g_mira_picture_info.x * 1080) / g_mira_pic_vdec_w;
                        mira_zoom_info.src_rect.w = 1920;
                        mira_zoom_info.src_rect.h = 1080 - 2 * mira_zoom_info.src_rect.y;
                    }
                    else
                    {
                        mira_zoom_info.src_rect.x = (g_mira_picture_info.x * 1920) / g_mira_pic_vdec_w;
                        mira_zoom_info.src_rect.y = 0;
                        mira_zoom_info.src_rect.w = 1920 - 2 * mira_zoom_info.src_rect.x;
                        mira_zoom_info.src_rect.h = 1080;
                    }
                    
                    mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, mira_zoom_info.dis_active_mode);
                    cast_mira_set_dis_zoom(&mira_zoom_info);
#else
                    cast_mira_set_dis_zoom(&mira_zoom_info);
                    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
#endif    
                }
            }
        }
        else
        {
            cast_mira_set_dis_zoom(&mira_zoom_info);
        }
    }

    return 0;
}

int hccast_mira_callback_func(hccast_mira_event_e event, void* in, void* out)
{
    control_msg_t ctl_msg = {0};
    char *str_tmp = NULL;
    app_data_t * app_data = data_mgr_app_get();
    static int vdec_first_show = 0;
    static int ui_logo_close = 0;
    static uint8_t g_vol = 100;

    switch (event)
    {
        case HCCAST_MIRA_GET_DEVICE_NAME:
        {
            if (in)
            {
                str_tmp = data_mgr_get_device_name();
                if (str_tmp)
                {
                    sprintf((char *)in, "%s_mira", str_tmp);
                    app_log(LL_INFO, "HCCAST_MIRA_GET_DEVICE_NAME:%s\n", str_tmp);
                }
            }
            break;
        }
        case HCCAST_MIRA_GET_DEVICE_PARAM:
        {
            hccast_wifi_p2p_param_t *p2p_param = (hccast_wifi_p2p_param_t*)in;
            if (p2p_param)
            {
                str_tmp = data_mgr_get_device_name();
                if (str_tmp)
                {
                    sprintf((char *)p2p_param->device_name, "%s_mira", str_tmp);
                }

                char p2p_ifname[32] = {0};
                hccast_wifi_mgr_get_p2p_ifname(p2p_ifname, sizeof(p2p_ifname));
                snprintf(p2p_param->p2p_ifname, sizeof(p2p_param->p2p_ifname), "%s", p2p_ifname);

                int ch = hccast_wifi_mgr_get_current_freq();
                if (ch > 0)
                {
                    if (hccast_wifi_mgr_get_hostap_status())
                    { // AP
                        // Some Special models P2P listen channel must be consistent with AP
                        if (HCCAST_NET_WIFI_8800D == network_wifi_module_get())
                        {
                            p2p_param->listen_channel = ch;
                        }
                        else if (1 == ch || 6 == ch || 11 == ch)
                        {
                            p2p_param->listen_channel = ch;
                        }
                        else
                        {
                            p2p_param->listen_channel = HCCAST_P2P_LISTEN_CH_DEFAULT;
                        }
                    }
                    else if (ch >= 36)
                    { // STA 5G
                        p2p_param->listen_channel = HCCAST_P2P_LISTEN_CH_DEFAULT;
                    }
                    else if ((ch == 1) || (ch >= 11))
                    { // STA 2.4G
                        p2p_param->listen_channel = 6;
                    }
                    else
                    {
                        p2p_param->listen_channel = 1;
                    }
                    if (HCCAST_NET_WIFI_ECR6600U == network_wifi_module_get())
                    {
                        p2p_param->listen_channel = ch;
                    }
                }
                else
                {
                    p2p_param->listen_channel = HCCAST_P2P_LISTEN_CH_DEFAULT;
                }
            }
            break;
        }
        case HCCAST_MIRA_SSID_DONE:
        {
            app_log(LL_DEBUG, "[%s]HCCAST_MIRA_SSID_DONE\n",__func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_SSID_DONE;
            break;
        }
        case HCCAST_MIRA_GET_CUR_WIFI_INFO:
        {
            app_log(LL_DEBUG, "[%s]HCCAST_MIRA_GET_CUR_WIFI_INFO\n",__func__);
            hccast_wifi_ap_info_t *cur_ap;
            char* cur_ssid = app_get_connecting_ssid();
            cur_ap = data_mgr_get_wifi_info(cur_ssid);
            if(cur_ap)
            {
                memcpy(in, cur_ap, sizeof(hccast_wifi_ap_info_t));
            }
            if (HCCAST_NET_WIFI_ECR6600U == hccast_wifi_mgr_get_wifi_model() \
                && ((hccast_wifi_ap_info_t*)in)->encryptMode != HCCAST_WIFI_ENCRYPT_MODE_OPEN_WEP \
                && ((hccast_wifi_ap_info_t*)in)->encryptMode != HCCAST_WIFI_ENCRYPT_MODE_SHARED_WEP \
                )
                ((hccast_wifi_ap_info_t*)in)->encryptMode = HCCAST_WIFI_ENCRYPT_MODE_NOT_HANDLE;
            break;
        }
        case HCCAST_MIRA_CONNECT:
        {
            //miracast connect start
            app_log(LL_DEBUG, "[%s]HCCAST_MIRA_CONNECT\n",__func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_CONNECTING;
            break;
        }
        case HCCAST_MIRA_CONNECTED:
        {
            //miracast connect success
            app_log(LL_DEBUG, "[%s]HCCAST_MIRA_CONNECTED\n",__func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_CONNECTED;
            break;
        }
        case HCCAST_MIRA_DISCONNECT:
        {
            //miracast disconnect
            app_log(LL_DEBUG, "[%s]HCCAST_MIRA_DISCONNECT\n",__func__);
            break;
        }

        case HCCAST_MIRA_START_DISP:
        {
            //miracast start
            printf("[%s] HCCAST_MIRA_START_DISP [%d:%d]\n",__func__, vdec_first_show, ui_logo_close);
            g_vol = cast_api_get_volume();
            cast_api_set_volume(100);
            printf("%s set vol %d->100\n", __func__, g_vol);
            cast_usb_mirror_stop();
            ui_logo_close = 1;
            api_logo_off2(0,0);
            cast_api_set_volume(0);//set to mute until recv HCCAST_MIRA_START_FIRST_FRAME_DISP event.
            g_mira_full_vscreen = app_data->mirror_full_vscreen;
            
            int rotate = app_data->mirror_rotation;
            if ((rotate == MIRROR_ROTATE_90) || (rotate == MIRROR_ROTATE_270))
            {
                cast_api_mira_vscreen_detect_enable(1);
                g_mira_enable_vrotation = 1;
            }
            else
            {
                cast_api_mira_vscreen_detect_enable(0);
                g_mira_enable_vrotation = 0;
            }

            if (g_mira_force_detect_en)
            {
                cast_api_mira_vscreen_detect_enable(1);
            }
            
            cast_api_set_aspect_mode(DIS_TV_16_9, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_NEXTFRAME);
            cast_set_drv_hccast_type(CAST_TYPE_MIRACAST);
#ifdef __HCRTOS__
            set_eswin_drop_out_of_order_packet_flag(1);
#endif
            break;
        }
        case HCCAST_MIRA_START_FIRST_FRAME_DISP:
        {
            printf("[%s] HCCAST_MIRA_START_FIRST_FRAME_DISP [%d:%d]\n",__func__, vdec_first_show, ui_logo_close);

            vdec_first_show = 1;
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_START;
            cast_api_set_volume(100);
            
            break;
        }
        case HCCAST_MIRA_STOP_DISP:
        {
            //miracast stop
            printf("[%s] HCCAST_MIRA_STOP_DISP [%d:%d]\n",__func__, vdec_first_show, ui_logo_close);
#ifdef __HCRTOS__
            set_eswin_drop_out_of_order_packet_flag(0);
#endif
            g_mira_enable_vrotation = 0;
            g_mira_v_screen = 0;
            cast_api_mira_reset_aspect_mode();
            cast_api_set_volume(g_vol);
            cast_api_mira_vscreen_detect_enable(0);
            cast_set_drv_hccast_type(CAST_TYPE_NONE);
            printf("%s reset vol 100->%d\n", __func__, g_vol);
            
            if((vdec_first_show == 0) && (ui_logo_close == 1))
            {   
                api_logo_show(NULL);
                ui_logo_close = 0;
                cast_usb_mirror_start();
                return 0;
            }
            vdec_first_show = 0;
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_STOP;
            cast_usb_mirror_start();
            break;
        }
        case HCCAST_MIRA_GET_MIRROR_QUICK_MODE_NUM:
        {
            if(app_data->mirror_mode == 1)
            {
                *(int*)in = 2;
            }
            else if(app_data->mirror_mode == 2)
            {
                *(int*)in = 6;       
            }
            else
            {
                printf("[%s] unkown mirror mode.\n", __func__);
            }

            break;
        }
        case HCCAST_MIRA_GET_MIRROR_ROTATION_INFO:
            cast_api_mira_get_rotation_info((hccast_mira_rotation_t*)out);
            break;
        case HCCAST_MIRA_MIRROR_SCREEN_DETECT_NOTIFY:
        {
            int v_screen;
            if (in)
            {
                v_screen = *(unsigned long*)in;
                cast_api_mira_screen_detect_handle(v_screen);   
            }    
            break;
        }    

        default:
            break;

    }

    if (0 != ctl_msg.msg_type)
    {
        api_control_send_msg(&ctl_msg);
    }

    if (MSG_TYPE_CAST_MIRACAST_STOP == ctl_msg.msg_type)
    {
        //printf("[%s] wait cast root start tick: %d\n",__func__,(int)time(NULL));
        bool win_cast_root_wait_open(uint32_t timeout);
        win_cast_root_wait_open(20000);
        //printf("[%s] wait cast root end tick: %d\n",__func__,(int)time(NULL));
    }

    if (MSG_TYPE_CAST_MIRACAST_START == ctl_msg.msg_type)
    {
        //printf("[%s] wait cast play start tick: %d\n",__func__,(int)time(NULL));
        bool win_cast_play_wait_open(uint32_t timeout);
        win_cast_play_wait_open(20000);
        //printf("[%s] wait cast play end tick: %d\n",__func__,(int)time(NULL));
#if CASTING_CLOSE_FB_SUPPORT	
        api_osd_show_onoff(false);
#endif
    }

    return 0;
}
#endif

#ifdef AIRCAST_SUPPORT
static int g_air_vd_dis_mode = DIS_PILLBOX;
static int g_air_black_detect = 0;
static int g_air_photo_vscreen = 0;
static int g_air_video_width = 0;
static int g_air_video_height = 0;
static int g_air_detect_zoomed = 0;
static int g_air_feed_data_tick = -1;
static pthread_t g_air_rotate_tid = 0;
static int g_air_rotate_run = 0;
static int g_air_dis_rotate = -1;
static int g_air_full_vscreen = 0;
static pthread_mutex_t m_air_rotate_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
    av_area_t src_rect;
    av_area_t dst_rect;
    int dis_active_mode;
} cast_air_zoom_info_t;

static void cast_air_set_dis_zoom(cast_air_zoom_info_t *air_zoom_info)
{
    av_area_t src_rect;
    av_area_t dst_rect;
    int dis_active_mode;

    memcpy(&src_rect, &air_zoom_info->src_rect, sizeof(av_area_t));
    memcpy(&dst_rect, &air_zoom_info->dst_rect, sizeof(av_area_t));
    dis_active_mode = air_zoom_info->dis_active_mode;

    cast_api_set_dis_zoom(&src_rect, &dst_rect, dis_active_mode);
}

static void cast_air_vscreen_detect_enable(int enable)
{
    struct dis_miracast_vscreen_detect_param mpara = { 0 };
    int fd = -1;

    fd = open("/dev/dis", O_RDWR);
    if (fd < 0)
    {
        return ;
    }

    mpara.distype = DIS_TYPE_HD;
    if (enable)
    {
        mpara.on = 1;
    }
    else
    {
        mpara.on = 0;
    }

#ifdef CAST_PHOTO_DETECT
    mpara.cast_photo_detect = true;
#endif

    ioctl(fd, DIS_SET_MIRACAST_VSRCEEN_DETECT, &mpara);

    close(fd);
}

static void cast_air_get_picture_area(av_area_t *src_rect)
{
    int fd = open("/dev/dis" , O_RDWR);
    if(fd < 0)
        return;

    dis_screen_info_t picture_info = { 0 };

    picture_info.distype = DIS_TYPE_HD;
    ioctl(fd , DIS_GET_MIRACAST_PICTURE_ARER , &picture_info);
    src_rect->x = picture_info.area.x;
    src_rect->y = picture_info.area.y;
    src_rect->w = picture_info.area.w;
    src_rect->h = picture_info.area.h;

    printf("%s %d %d %d %d\n",__FUNCTION__, 
           src_rect->x , 
           src_rect->y, 
           src_rect->w, 
           src_rect->h);
           
    close(fd);
}

static int cast_air_get_current_pic_info(struct dis_display_info *mpinfo)
{
    int fd = -1;

    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0)
    {
        return -1;
    }

    mpinfo->distype = DIS_TYPE_HD;
    mpinfo->info.layer = DIS_PIC_LAYER_MAIN;
    ioctl(fd, DIS_GET_DISPLAY_INFO, (uint32_t)mpinfo);
    close(fd);
    return 0;
}

static int cast_air_get_video_info(int *width, int *heigth)
{
    struct dis_display_info mpinfo = {0};

    cast_air_get_current_pic_info(&mpinfo);
    
    if ((!mpinfo.info.pic_height) || (!mpinfo.info.pic_width))
    {
        printf("mpinfo param error.\n");
        return -1;
    }


    if (mpinfo.info.rotate_mode == ROTATE_TYPE_90 ||
        mpinfo.info.rotate_mode == ROTATE_TYPE_270)
    {
        *width = mpinfo.info.pic_height;//mpinfo.info.pic_dis_area.h;
        *heigth = mpinfo.info.pic_width;//mpinfo.info.pic_dis_area.w;
    }
    else
    {
        *width = mpinfo.info.pic_width;//mpinfo.info.pic_dis_area.w;
        *heigth = mpinfo.info.pic_height;//mpinfo.info.pic_dis_area.h;
    }

    printf("video info: rotate=%d, w:%ld, h:%ld\n",
           mpinfo.info.rotate_mode, *width, *heigth);

    return 0;
}

static void cast_air_dis_rotate_set(int rotate, int enable)
{
    int fd = -1;
    dis_rotate_t dis_rotate;

    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0)
    {
        printf("%s open dis error.\n", __func__);
        return ;
    }

    dis_rotate.distype = DIS_TYPE_HD;
    dis_rotate.enable = enable;
    dis_rotate.mirror_enable = 0;
    dis_rotate.angle = rotate;
    dis_rotate.layer = DIS_LAYER_MAIN;
    
    ioctl(fd, DIS_SET_ROTATE, &dis_rotate);
    printf("set dis rotate done\n");
    close(fd);
}

static int cast_air_get_dis_rotate(int *dis_rotate)
{
    int fd = -1;
    int ret = 0;
    struct dis_display_info dis_info;

    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0)
    {
        return -1;
    }

    dis_info.distype = DIS_TYPE_HD;
    dis_info.info.layer = DIS_PIC_LAYER_MAIN;
    ret = ioctl(fd, DIS_GET_DISPLAY_INFO, &dis_info);
    
    *dis_rotate = dis_info.info.rotate_mode;
    
    close(fd);
    return ret;
}

static int cast_air_flip_rotate_covert(int flip_mode, int rotation)
{
    int flip_mode_0[4] = {ROTATE_TYPE_0, ROTATE_TYPE_270, ROTATE_TYPE_90, ROTATE_TYPE_180};
    int flip_mode_90[4] = {ROTATE_TYPE_90, ROTATE_TYPE_0, ROTATE_TYPE_180, ROTATE_TYPE_270};
    int flip_mode_180[4] = {ROTATE_TYPE_180, ROTATE_TYPE_90, ROTATE_TYPE_270, ROTATE_TYPE_0};
    int flip_mode_270[4] = {ROTATE_TYPE_270, ROTATE_TYPE_180, ROTATE_TYPE_0, ROTATE_TYPE_90};
    
    if (ROTATE_TYPE_0 == flip_mode)
    {
        return flip_mode_0[rotation];
    }
    else if (ROTATE_TYPE_90 == flip_mode)
    {
        return flip_mode_90[rotation];
    }
    else if (ROTATE_TYPE_180 == flip_mode)
    {
        return flip_mode_180[rotation];
    }
    else if (ROTATE_TYPE_270 == flip_mode)
    {
        return flip_mode_270[rotation];
    }
    
    return 0;
}

static int cast_air_rotation_setting_handle(int width, int height)
{
    int temp = 0;
    cast_air_zoom_info_t air_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_NEXTFRAME};

    app_data_t * app_data = data_mgr_app_get();
    int rotate = app_data->mirror_rotation;
    if ((rotate == MIRROR_ROTATE_90) || (rotate == MIRROR_ROTATE_270))
    {
        temp = 1;
    }
    else
    {
        temp = 0;
    }

    g_air_video_width = width;
    g_air_video_height = height;
    g_air_feed_data_tick = api_get_time_ms();
    g_air_dis_rotate = -1;
    
    //for avoid every time will call back the vscreen_detect_enable.
    if (temp != g_air_black_detect)
    {
        if (temp)
        {
            cast_air_vscreen_detect_enable(1);
            g_air_black_detect = 1;
        }
        else
        {
            cast_air_vscreen_detect_enable(0);
            cast_air_set_dis_zoom(&air_zoom_info);
            cast_air_dis_rotate_set(ROTATE_TYPE_0, 0);
            g_air_detect_zoomed = 0;
            g_air_photo_vscreen = 0;
            g_air_black_detect = 0;
        }
    }

    return 0;
}

static void *cast_air_dis_rotate_task(void *arg)
{
    while(g_air_rotate_run)
    {
        pthread_mutex_lock(&m_air_rotate_mutex);
        if (g_air_dis_rotate!= -1 && (api_get_time_ms() - g_air_feed_data_tick >= 200))
        {
            cast_air_dis_rotate_set(g_air_dis_rotate, 1);
            g_air_dis_rotate = -1;
        }
        pthread_mutex_unlock(&m_air_rotate_mutex);
        usleep(20*1000);
    }

    printf("%s exit!\n", __func__);

    return NULL;
}

static int cast_air_screen_detect_handle(int is_v_screen)
{
    app_data_t * app_data = data_mgr_app_get();
    int flip_rotate = 0;
    int flip_mirror = 0;
    int cur_tick = 0;
    av_area_t m_air_picture_info = { 0, 0, 1920, 1080};
    int m_air_pic_vdec_w = 1920;
    int m_air_pic_vdec_h = 1080;
    cast_air_zoom_info_t air_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_NEXTFRAME};
    api_get_flip_info(&flip_rotate, &flip_mirror);

    pthread_mutex_lock(&m_air_rotate_mutex);
    printf("Air detect %s\n", (is_v_screen ? "V_SCR" : "H_SCR"));
    
    if (g_air_video_width >= g_air_video_height)
    {
        cast_air_get_picture_area(&m_air_picture_info);
        if (cast_air_get_video_info(&m_air_pic_vdec_w, &m_air_pic_vdec_h) < 0)
        {
            m_air_pic_vdec_w = g_air_video_width;
        }

        if (m_air_picture_info.h > (m_air_picture_info.w+8))
        {
            float w_ratio = 1080/(float)m_air_picture_info.w;
            float h_ratio = 1920/(float)m_air_picture_info.h;

            if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)      
            {
                air_zoom_info.src_rect.x = 0;
                air_zoom_info.src_rect.y = (m_air_picture_info.x * 1080) / m_air_pic_vdec_w;
                air_zoom_info.src_rect.w = 1920;
                air_zoom_info.src_rect.h = 1080 - 2 * air_zoom_info.src_rect.y;

                if (app_data->mirror_full_vscreen == 0)
                {
                    if (w_ratio >= h_ratio)
                    {
                        air_zoom_info.dst_rect.h = m_air_picture_info.w*h_ratio;
                        air_zoom_info.dst_rect.y = (1080 - air_zoom_info.dst_rect.h)/2;
                    }
                    else
                    {
                        air_zoom_info.dst_rect.w = m_air_picture_info.h*w_ratio;
                        air_zoom_info.dst_rect.x = (1920 - air_zoom_info.dst_rect.w)/2;
                    }
                }
            }
            else
            {
                air_zoom_info.src_rect.x = (m_air_picture_info.x * 1920) / m_air_pic_vdec_w;
                air_zoom_info.src_rect.y = 0;
                air_zoom_info.src_rect.w = 1920 - 2 * air_zoom_info.src_rect.x;
                air_zoom_info.src_rect.h = 1080;

                if (app_data->mirror_full_vscreen == 0)
                {
                    if (w_ratio >= h_ratio)
                    {
                        air_zoom_info.dst_rect.w = m_air_picture_info.w*h_ratio*1920/1080;
                        air_zoom_info.dst_rect.x = (1920 - air_zoom_info.dst_rect.w)/2;
                    }
                    else
                    {
                        air_zoom_info.dst_rect.h = m_air_picture_info.h*w_ratio*1080/1920;
                        air_zoom_info.dst_rect.y = (1080 - air_zoom_info.dst_rect.h)/2;
                    }
                }
            }

            g_air_dis_rotate = cast_air_flip_rotate_covert(flip_rotate, app_data->mirror_rotation);
            g_air_vd_dis_mode = DIS_NORMAL_SCALE;
            int dis_rotate;
            if (cast_air_get_dis_rotate(&dis_rotate) < 0)
            {
                printf("air get dis rotate error.\n");
                cast_api_set_aspect_mode(DIS_TV_16_9, g_air_vd_dis_mode, DIS_SCALE_ACTIVE_IMMEDIATELY);
                air_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
            }
            else
            {
                if (dis_rotate == g_air_dis_rotate)
                {
                    cast_api_set_aspect_mode(DIS_TV_16_9, g_air_vd_dis_mode, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    air_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                }    
                else
                {
                    cast_api_set_aspect_mode(DIS_TV_16_9, g_air_vd_dis_mode, DIS_SCALE_ACTIVE_NEXTFRAME);
                    air_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                }
            }
            
            cast_air_set_dis_zoom(&air_zoom_info);
            g_air_detect_zoomed = 1;
            g_air_photo_vscreen = 1;
        }
        else
        {
            if (app_data->mirror_vscreen_auto_rotation)
            {
                g_air_dis_rotate = cast_air_flip_rotate_covert(flip_rotate, ROTATE_TYPE_0);
            }

            g_air_vd_dis_mode = DIS_PILLBOX;
            int dis_rotate;
            if (cast_air_get_dis_rotate(&dis_rotate) < 0)
            {
                printf("air get dis rotate error.\n");
                cast_api_set_aspect_mode(DIS_TV_16_9, g_air_vd_dis_mode, DIS_SCALE_ACTIVE_IMMEDIATELY);
                air_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
            }
            else
            {
                if (dis_rotate == g_air_dis_rotate || g_air_dis_rotate == -1)
                {
                    cast_api_set_aspect_mode(DIS_TV_16_9, g_air_vd_dis_mode, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    air_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                }    
                else
                {
            cast_api_set_aspect_mode(DIS_TV_16_9, g_air_vd_dis_mode, DIS_SCALE_ACTIVE_NEXTFRAME);
                }
            }
            cast_air_set_dis_zoom(&air_zoom_info);
            g_air_detect_zoomed = 0;
            g_air_photo_vscreen = 0;
        }
    }
    else
    {
        int expect_dis_rotate = cast_air_flip_rotate_covert(flip_rotate, app_data->mirror_rotation);
        int dis_rotate;
        if (cast_air_get_dis_rotate(&dis_rotate) < 0)
        {
            printf("air get dis rotate error.\n");
            air_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
        }
        else
        {
            if (dis_rotate == expect_dis_rotate)
            {
                air_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
            }    
            else
            {
                air_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
            }
        }
        cast_air_set_dis_zoom(&air_zoom_info);
        g_air_detect_zoomed = 0;
        g_air_photo_vscreen = 0;
    }

    pthread_mutex_unlock(&m_air_rotate_mutex);

    return 0;
}

static void cast_air_vscreen_detect_start(void)
{
    app_data_t * app_data = data_mgr_app_get();
    int rotate = app_data->mirror_rotation;
    if ((rotate == MIRROR_ROTATE_90) || (rotate == MIRROR_ROTATE_270))
    {
        cast_air_vscreen_detect_enable(1);
        g_air_black_detect = 1;       
    }
    else
    {
        cast_air_vscreen_detect_enable(0);
        g_air_black_detect = 0;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    g_air_rotate_run = 1;
    if (pthread_create(&g_air_rotate_tid, &attr, cast_air_dis_rotate_task, NULL) < 0)
    {
        printf("Create cast_air_dis_rotate_task error.\n");
    }
}

static void cast_air_vscreen_detect_stop(void)
{
    cast_air_zoom_info_t air_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};
    g_air_rotate_run = 0;
    if (g_air_rotate_tid > 0)
    {
        pthread_join(g_air_rotate_tid, NULL);
        g_air_rotate_tid = 0;
    }
    api_dis_show_onoff(false);
    cast_air_set_dis_zoom(&air_zoom_info);
    cast_air_vscreen_detect_enable(0);
    cast_air_dis_rotate_set(ROTATE_TYPE_0, 0);
    g_air_photo_vscreen = 0;
    g_air_detect_zoomed = 0;
    g_air_feed_data_tick = -1;
    g_air_black_detect = 0;
    g_air_dis_rotate = -1;
}

static void cast_air_dis_mode_set(int rotate, unsigned int width, unsigned int height)
{
    app_data_t * app_data = data_mgr_app_get();
    int full_vscreen = app_data->mirror_full_vscreen;
    int dis_mode;
    
    if (!rotate)
    {
        dis_mode = DIS_PILLBOX;
    }
    else
    {
        if ((rotate == MIRROR_ROTATE_270) || (rotate == MIRROR_ROTATE_90))
        {
            if ((height > width) && full_vscreen)
            {
                dis_mode = DIS_NORMAL_SCALE;
            }
            else
            {
                if (g_air_photo_vscreen)
                {
                    dis_mode = DIS_NORMAL_SCALE;
                }
                else
                {
                    dis_mode = DIS_PILLBOX;
                }
            }
        }
        else
        {
            dis_mode = DIS_PILLBOX;
        }
    }

    if (g_air_detect_zoomed && (g_air_video_height > g_air_video_width))
    {
        g_air_detect_zoomed = 0;
        cast_air_zoom_info_t air_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_NEXTFRAME};
        cast_air_set_dis_zoom(&air_zoom_info);
        printf("[%s] reset zoom.\n", __func__);
    }  

    if (dis_mode != g_air_vd_dis_mode)
    {
        if (full_vscreen != g_air_full_vscreen)
            cast_api_set_aspect_mode(DIS_TV_16_9, dis_mode, DIS_SCALE_ACTIVE_IMMEDIATELY);
        else
            cast_api_set_aspect_mode(DIS_TV_16_9, dis_mode, DIS_SCALE_ACTIVE_NEXTFRAME);     
        g_air_vd_dis_mode = dis_mode;
        g_air_full_vscreen = full_vscreen;
    }
}

static int cast_air_get_rotation_info(hccast_air_rotation_t *rotate_info)
{
    int seting_rotate;
    int rotation_angle = MIRROR_ROTATE_0;
    int flip_mode;
    int flip_rotate;
    app_data_t * app_data = data_mgr_app_get();
    
    if (!rotate_info)
    {
        return -1;
    }

    pthread_mutex_lock(&m_air_rotate_mutex);
#ifdef CAST_PHOTO_DETECT    
    cast_air_rotation_setting_handle(rotate_info->src_w, rotate_info->src_h);
#else
    g_air_video_width = rotate_info->src_w;
    g_air_video_height = rotate_info->src_h;
#endif    
    seting_rotate = app_data->mirror_rotation;

    if(((seting_rotate == MIRROR_ROTATE_90) || (seting_rotate == MIRROR_ROTATE_270)) && !g_air_photo_vscreen)
    {
        if (rotate_info->src_w >= rotate_info->src_h) 
        {
            if(app_data->mirror_vscreen_auto_rotation)
            {
                seting_rotate = MIRROR_ROTATE_0;
            }  
        }
    }
    
    cast_air_dis_mode_set(seting_rotate, rotate_info->src_w, rotate_info->src_h);
    api_get_flip_info(&flip_rotate, &flip_mode);
    rotation_angle = cast_air_flip_rotate_covert(flip_rotate, seting_rotate);

    rotate_info->rotate_angle = rotation_angle;
    rotate_info->flip_mode = flip_mode;
    pthread_mutex_unlock(&m_air_rotate_mutex);
    
    return 0;
}

static void cast_air_reset_zoom(void)
{
    app_data_t * app_data = data_mgr_app_get();
    int flip_rotate = 0;
    int flip_mirror = 0;
    av_area_t m_air_picture_info = { 0, 0, 1920, 1080};
    int m_air_pic_vdec_w = 1920;
    int m_air_pic_vdec_h = 1080;
    cast_air_zoom_info_t air_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};
    api_get_flip_info(&flip_rotate, &flip_mirror);

    printf("%s %d.\n", __func__, __LINE__);

    if (g_air_video_width >= g_air_video_height)
    {
        cast_air_get_picture_area(&m_air_picture_info);
        if (cast_air_get_video_info(&m_air_pic_vdec_w, &m_air_pic_vdec_h) < 0)
        {
            m_air_pic_vdec_w = g_air_video_width;
        }

        if (m_air_picture_info.h > (m_air_picture_info.w+8))
        {
            float w_ratio = 1080/(float)m_air_picture_info.w;
            float h_ratio = 1920/(float)m_air_picture_info.h;

            if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)      
            {
                air_zoom_info.src_rect.x = 0;
                air_zoom_info.src_rect.y = (m_air_picture_info.x * 1080) / m_air_pic_vdec_w;
                air_zoom_info.src_rect.w = 1920;
                air_zoom_info.src_rect.h = 1080 - 2 * air_zoom_info.src_rect.y;

                if (app_data->mirror_full_vscreen == 0)
                {
                    if (w_ratio >= h_ratio)
                    {
                        air_zoom_info.dst_rect.h = m_air_picture_info.w*h_ratio;
                        air_zoom_info.dst_rect.y = (1080 - air_zoom_info.dst_rect.h)/2;
                    }
                    else
                    {
                        air_zoom_info.dst_rect.w = m_air_picture_info.h*w_ratio;
                        air_zoom_info.dst_rect.x = (1920 - air_zoom_info.dst_rect.w)/2;
                    }
                }
            }
            else
            {
                air_zoom_info.src_rect.x = (m_air_picture_info.x * 1920) / m_air_pic_vdec_w;
                air_zoom_info.src_rect.y = 0;
                air_zoom_info.src_rect.w = 1920 - 2 * air_zoom_info.src_rect.x;
                air_zoom_info.src_rect.h = 1080;

                if (app_data->mirror_full_vscreen == 0)
                {
                    if (w_ratio >= h_ratio)
                    {
                        air_zoom_info.dst_rect.w = m_air_picture_info.w*h_ratio*1920/1080;
                        air_zoom_info.dst_rect.x = (1920 - air_zoom_info.dst_rect.w)/2;
                    }
                    else
                    {
                        air_zoom_info.dst_rect.h = m_air_picture_info.h*w_ratio*1080/1920;
                        air_zoom_info.dst_rect.y = (1080 - air_zoom_info.dst_rect.h)/2;
                    }
                }
            }

            g_air_detect_zoomed = 1;
            g_air_photo_vscreen = 1;
            g_air_vd_dis_mode = DIS_NORMAL_SCALE;
            cast_api_set_aspect_mode(DIS_TV_16_9, g_air_vd_dis_mode, DIS_SCALE_ACTIVE_IMMEDIATELY);
            cast_air_set_dis_zoom(&air_zoom_info);
        }
    }
}

void cast_air_reset_video(void)
{
    app_data_t * app_data = data_mgr_app_get();
    int flip_rotate = 0;
    int flip_mode = 0;
    int rotation_angle;

    pthread_mutex_lock(&m_air_rotate_mutex);
    int seting_rotate = app_data->mirror_rotation;

    if (g_air_photo_vscreen && ((seting_rotate == MIRROR_ROTATE_90) || (seting_rotate == MIRROR_ROTATE_270)))
    {
        cast_air_reset_zoom();
    }
    else if (((seting_rotate == MIRROR_ROTATE_90) || (seting_rotate == MIRROR_ROTATE_270)))
    {
        if (g_air_video_width >= g_air_video_height) 
        {
            if(app_data->mirror_vscreen_auto_rotation)
            {
                seting_rotate = MIRROR_ROTATE_0;
            }  
        }

        cast_air_dis_mode_set(seting_rotate, g_air_video_width, g_air_video_height);
    }
 
    pthread_mutex_unlock(&m_air_rotate_mutex);
}

int hccast_air_callback_event(hccast_air_event_e event, void* in, void* out)
{
    control_msg_t ctl_msg = {0};
    app_data_t * app_data = data_mgr_app_get();
    int vol = 0;
    int menu_status = 1;//0 -- win_cast_play ui menu not open, 1 -- open.

    switch (event)
    {
        case HCCAST_AIR_GET_SERVICE_NAME:
            printf("[%s]HCCAST_AIR_GET_SERVICE_NAME\n",__func__);
#ifdef AIRP2P_SUPPORT            
            if(app_data->airp2p_en)
            {
                sprintf((char *)in, "%s_itv_p2p", data_mgr_get_device_name());
            }
            else
#endif            
            {
                sprintf((char *)in, "%s_itv", data_mgr_get_device_name());
            }    

            break;

        case HCCAST_AIR_GET_NETWORK_DEVICE:
            printf("[%s]HCCAST_AIR_GET_NETWORK_DEVICE\n",__func__);
            sprintf((char *)in, "%s", "wlan0");
            break;
        case HCCAST_AIR_GET_MIRROR_MODE:
            printf("[%s]HCCAST_AIR_GET_MIRROR_MODE\n",__func__);
            hccast_air_mode_e mode = HCCAST_AIR_MODE_MIRROR_ONLY;
            if (0 == app_data->aircast_mode)
            {
                mode = HCCAST_AIR_MODE_MIRROR_STREAM;
            }
            else if (1 == app_data->aircast_mode)
            {
                mode = HCCAST_AIR_MODE_MIRROR_ONLY;
            }
            else
            {
#ifdef WIFI_SUPPORT
                if (hccast_wifi_mgr_get_hostap_status())
                {
                    mode = HCCAST_AIR_MODE_MIRROR_ONLY;
                }
                else
                {
                    mode = HCCAST_AIR_MODE_MIRROR_STREAM;
                }
#endif
            }
            *(int*)in = mode;
            break;
        case HCCAST_AIR_GET_NETWORK_STATUS:
#ifdef WIFI_SUPPORT
            printf("[%s]HCCAST_AIR_GET_NETWORK_STATUS\n",__func__);
            *(int*)in = hccast_wifi_mgr_get_hostap_status();
#endif
            break;
        case HCCAST_AIR_MIRROR_START:
            printf("[%s]HCCAST_AIR_MIRROR_START\n",__func__);
            cast_usb_mirror_stop();
#ifdef CAST_PHOTO_DETECT
            cast_air_vscreen_detect_start();
#endif
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRMIRROR_START;
            g_air_vd_dis_mode = -1;
            g_air_full_vscreen = app_data->mirror_full_vscreen;
            cast_set_drv_hccast_type(CAST_TYPE_AIRCAST);
#ifdef __HCRTOS__
            set_eswin_drop_out_of_order_packet_flag(1);
#endif
            break;
        case HCCAST_AIR_MIRROR_STOP:
#ifdef CAST_PHOTO_DETECT            
            cast_air_vscreen_detect_stop();
#endif            
            printf("[%s]HCCAST_AIR_MIRROR_STOP\n",__func__);
            cast_usb_mirror_start();
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRMIRROR_STOP;
            cast_set_drv_hccast_type(CAST_TYPE_NONE);
#ifdef __HCRTOS__
            set_eswin_drop_out_of_order_packet_flag(0);
#endif
            break;
        case HCCAST_AIR_AUDIO_START:
            printf("[%s]HCCAST_AIR_AUDIO_START\n",__func__);
            cast_usb_mirror_stop();
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRCAST_AUDIO_START;
            break;
        case HCCAST_AIR_AUDIO_STOP:
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRCAST_AUDIO_STOP;
            printf("[%s]HCCAST_AIR_AUDIO_STOP\n",__func__);
            cast_usb_mirror_start();
            break;
        case HCCAST_AIR_INVALID_CERT:
            ctl_msg.msg_type = MSG_TYPE_AIR_INVALID_CERT;
            m_air_is_demo = true;
            printf("[%s],line:%d. HCCAST_AIR_INVALID_CERT\n",__func__, __LINE__);
            break;
        case HCCAST_AIR_GET_4K_MODE:
            if(data_mgr_de_tv_sys_get() < TV_LINE_4096X2160_30)
            {
                *(int*)in = 0;
                printf("[%s] NOT 4K MODE, tv_sys:%d\n",__func__,data_mgr_de_tv_sys_get());

            }
            else
            {
                *(int*)in = 1;
                printf("[%s] NOW IS 4K MODE, tv_sys:%d\n",__func__,data_mgr_de_tv_sys_get());
            }

            break;
        case HCCAST_AIR_HOSTAP_MODE_SKIP_URL:
            ctl_msg.msg_type = MSG_TYPE_AIR_HOSTAP_SKIP_URL;
            printf("[%s]HCCAST_AIR_HOSTAP_MODE_SKIP_URL\n",__func__);
            break;
        case HCCAST_AIR_BAD_NETWORK:
            ctl_msg.msg_type = MSG_TYPE_AIR_MIRROR_BAD_NETWORK;
            printf("[%s]HCCAST_AIR_BAD_NETWORK\n",__func__);
            break;
        case HCCAST_AIR_URL_ENABLE_SET_DEFAULT_VOL:
            *(int*)in = 1;
            break; 
        case HCCAST_AIR_SET_AUDIO_VOL:
            vol = (int)in;
            printf("%s set vol:%d\n", __func__, vol);
            cast_api_set_volume(vol);
            data_mgr_volume_set(vol);     
            break;
#ifdef AIRP2P_SUPPORT            
        case HCCAST_AIR_GET_AIRP2P_PIN:
            sprintf((char *)in, "%s", app_data->airp2p_pin);
            break;
#endif 
        case HCCAST_AIR_GET_MIRROR_QUICK_MODE_NUM:
        {
#ifdef AIRP2P_SUPPORT                  
            if(app_data->airp2p_en)
            {
                *(int*)in = 7;//set to support the max level.
            }
            else
#endif        
            {
                if(app_data->mirror_mode == 1)
                {
                    *(int*)in = 2;
                }
                else if(app_data->mirror_mode == 2)
                {
                    *(int*)in = 6;       
                }
                else
                {
                    printf("[%s] unkown mirror mode.\n", __func__);
                }
            }
        
            break;
        }
        case HCCAST_AIR_GET_MIRROR_ROTATION_INFO:
            cast_air_get_rotation_info((hccast_air_rotation_t*)in);
            break;    
        case HCCAST_AIR_MIRROR_SCREEN_DETECT_NOTIFY:
        {
            int vscreen;
            if (in)
            {
                vscreen = *(unsigned long*)in;
                cast_air_screen_detect_handle(vscreen);
            }
            
            break;
        }    
        default:
            break;
    }

    if (0 != ctl_msg.msg_type)
        api_control_send_msg(&ctl_msg);

    if (MSG_TYPE_CAST_AIRMIRROR_STOP == ctl_msg.msg_type)
    {
        //While dlna preempt air mirror(air mirror->air play), air mirror stop and dlna start,
        //sometime it is still in cast play UI(not exit to win root UI),
        //the next dlna url play is starting, then the UI/logo may block the dlna playing.
        //So here exit callback function wait for win cast root UI opening
        printf("[%s] wait cast root start tick: %d\n",__func__,(int)time(NULL));
        bool win_cast_root_wait_open(uint32_t timeout);
        win_cast_root_wait_open(20000);
        printf("[%s] wait cast root end tick: %d\n",__func__,(int)time(NULL));
    }

    if (MSG_TYPE_CAST_AIRMIRROR_START == ctl_msg.msg_type)
    {
        printf("[%s] wait cast play start tick: %d\n",__func__,(int)time(NULL));
        bool win_cast_play_wait_open(uint32_t timeout);
        menu_status = win_cast_play_wait_open(20000);
        printf("[%s] wait cast play end tick: %d\n",__func__,(int)time(NULL));

        if (menu_status == 0)
        {
            if(in)
            {
                *(int *)in = 0;//in: 0 -- mean not open mirror video, 1 -- mean open mirror video.
            }
            printf("[%s] win cast play menu not open: %d\n",__func__,menu_status);
            return 0;
        }        
        
#if CASTING_CLOSE_FB_SUPPORT	
        api_osd_show_onoff(false);
#endif
    }

    return 0;
}
#endif

void cast_restart_services()
{
    if(hccast_get_current_scene() != HCCAST_SCENE_NONE)
    {
        hccast_scene_switch(HCCAST_SCENE_NONE);
    }

    printf("[%s]  begin restart services.\n",__func__);

#ifdef DLNA_SUPPORT
    hccast_dlna_service_stop();
    hccast_dlna_service_start();
#ifdef DIAL_SUPPORT
    hccast_dial_service_stop();
    hccast_dial_service_start();
#endif
#endif

    hccast_air_service_stop();
    hccast_air_service_start();

}

void restart_air_service_by_hdmi_change(void)
{
#ifdef AIRCAST_SUPPORT
    int cur_scene = 0;
    cur_scene = hccast_get_current_scene();
    if((cur_scene != HCCAST_SCENE_AIRCAST_PLAY) && (cur_scene != HCCAST_SCENE_AIRCAST_MIRROR))
    {
        if(hccast_air_service_is_start())
        {
            hccast_air_service_stop();
            hccast_air_service_start();
        }
    }
#endif
}

bool cast_air_is_demo(void)
{
    return m_air_is_demo;
}

bool cast_dial_is_demo(void)
{
    return m_dial_is_demo;
}

bool cast_um_is_demo(void)
{
    return m_um_is_demo;
}

cast_dial_conn_e cast_dial_connect_state(void)
{
    return m_dial_conn_state;
}

int cast_set_drv_hccast_type(cast_type_t type)
{
#ifdef __HCRTOS__
    switch (type)
    {
        case CAST_TYPE_NONE:
            set_config_hccast_type(HCCAST_TYPE_NONE);
            break;
        case CAST_TYPE_AIRCAST:
            set_config_hccast_type(HCCAST_TYPE_AIR);
            break;
        case CAST_TYPE_DLNA:
            set_config_hccast_type(HCCAST_TYPE_DLNA);
            break; 
        case CAST_TYPE_MIRACAST:
            set_config_hccast_type(HCCAST_TYPE_MIRROR);
            break;
        default:
            printf("Unkown cast type!\n");
            break;
    }
#endif
    return 0;
}
