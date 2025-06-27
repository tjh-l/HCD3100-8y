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

#include "app_config.h"

#ifdef CAST_SUPPORT


#include <ffplayer.h>
#include <sys/ioctl.h>


#include <sys/socket.h>

#include <netinet/in.h>
#include <net/if.h>
#include <time.h>

#include <hccast/hccast_scene.h>
#include <hcuapi/dis.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <hcuapi/snd.h>
#ifdef USBMIRROR_SUPPORT
#include <hccast/hccast_um.h>
#endif
#ifdef __HCRTOS__
#include <kernel/drivers/avinit.h>
#endif

#include <hccast/hccast_net.h>
#include <hccast/hccast_com.h>

#include "network_api.h"
#include "com_api.h"
#include "osd_com.h"
#include "setup.h"
#include "cast_api.h"
//#include "data_mgr.h"
#include "factory_setting.h"
#include "app_log.h"
#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif

#ifdef UIBC_SUPPORT
#include "mira_uibc.h"
#include "usb_hid.h"
#endif

#ifdef SUPPORT_HID
#include "cast_hid.h"
#include <hidalgo/hccast_hid.h>
#endif

#define CAST_SERVICE_NAME               "HCcast"
#define CAST_AIRCAST_SERVICE_NAME       "HCcast"
#define CAST_DLNA_FIRENDLY_NAME         "HCcast"
#define CAST_MIRACAST_NAME              "HCcast"

#define UUID_HEADER "HCcast"
//#define CAST_PHOTO_DETECT

static bool m_is_demo = false;
static bool m_air_is_demo = false;
static bool m_dial_is_demo = false;
static bool m_um_is_demo = false;
static bool m_airp2p_is_demo = false;
static int m_airp2p_en = false;
static dis_tv_mode_e g_cast_dis_mode = DIS_TV_16_9;//DIS_TV_4_3 or DIS_TV_16_9.

static cast_dial_conn_e m_dial_conn_state = CAST_DIAL_CONN_NONE;

cast_ui_wait_ready_func dlna_ui_wait_ready = NULL;
cast_ui_wait_ready_func mira_ui_wait_ready = NULL;
cast_ui_wait_ready_func air_ui_wait_ready = NULL;
cast_ui_wait_ready_func cast_main_ui_wait_ready = NULL;

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
{}
int hccast_air_mdnssd_stop(void)
{}
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


//#ifndef USBMIRROR_SUPPORT
#if 0
int hccast_um_init(void)
{
    return 0;
}

int hccast_um_deinit(void)
{
    return 0;
}

int hccast_um_param_set(hccast_um_param_t *param)
{
    (void)param;
    return 0;
}

int hccast_ium_start(char *uuid, hccast_um_cb event_cb)
{
    (void)uuid;
    (void)event_cb;
    return 0;
}

int hccast_ium_stop(void)
{
    return 0;
}

int hccast_aum_start(hccast_aum_param_t *param, hccast_um_cb event_cb)
{
    (void)param;
    (void)event_cb;
    return 0;
}

int hccast_aum_stop(void)
{
    return 0;
}

#endif

void cast_api_set_volume(int vol)
{
#ifdef BT_AC6956C_GX
    // ctrl bt vol and do not ctrl soc snddev vol
    bluetooth_ioctl(BLUETOOTH_SET_MUSIC_VOL_VALUE, vol);
    return;
#endif
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
    int dis_fd = open("/dev/dis", O_WRONLY);

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

int cast_api_check_dis_mode(void)
{
#ifdef SYS_ZOOM_SUPPORT
    if (projector_get_some_sys_param(P_SYS_ZOOM_DIS_MODE) == DIS_TV_4_3)
    {
        if (get_screen_is_vertical())
        {
            g_cast_dis_mode = DIS_TV_16_9;
        }
        else
        {
            g_cast_dis_mode = DIS_TV_4_3;
        }
    }
    else
    {
        g_cast_dis_mode = DIS_TV_16_9;
    }
#endif

    return 0;
}

int cast_get_service_name(cast_type_t cast_type, char *service_name, int length)
{
    unsigned char mac_addr[6] = {0};
    char service_prefix[32] = CAST_SERVICE_NAME;

    snprintf(service_prefix, sizeof(service_prefix) - 1, "%s", CAST_SERVICE_NAME);
    if (0 != api_get_mac_addr((char*)mac_addr))
        memset(mac_addr, 0xff, sizeof(mac_addr));

    if (CAST_TYPE_AIRCAST == cast_type)
        snprintf(service_prefix, sizeof(service_prefix) - 1, "%s", CAST_AIRCAST_SERVICE_NAME);
    else if (CAST_TYPE_DLNA == cast_type)
        snprintf(service_prefix, sizeof(service_prefix) - 1, "%s", CAST_DLNA_FIRENDLY_NAME);
    else if (CAST_TYPE_MIRACAST == cast_type)
        snprintf(service_prefix, sizeof(service_prefix) - 1, "%s", CAST_MIRACAST_NAME);

    snprintf(service_name, length, "%s-%02x%02x%02x",
             service_prefix, mac_addr[3] & 0xFF, mac_addr[4] & 0xFF, mac_addr[5] & 0xFF);

    return 0;
}


#ifdef USBMIRROR_SUPPORT
static char *g_um_upgrade_buf = NULL;
static unsigned int g_um_upgrade_size = 0;
static hccast_aum_upg_bo_t aum_upg_buf_info;
static int um_start_upgrade = 0;//0--idel, 1--download, 2--burning.
static int g_ium_vd_dis_mode = DIS_PILLBOX;
static av_area_t g_um_src_rect = {0, 0, 1920, 1080};
static int g_ium_full_screen = 0;

void cast_um_free_upg_buf(void)
{
    if (g_um_upgrade_buf)
    {
        api_upgrade_buffer_free(g_um_upgrade_buf);
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
        g_um_upgrade_buf = api_upgrade_buffer_alloc(buf_len);
        g_um_upgrade_size = buf_len;
        upg_buf = g_um_upgrade_buf;
        printf("um upg fw size: %d\n", buf_len);
    }

    return upg_buf;
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

#ifdef SYS_ZOOM_SUPPORT
    {
        dst_rect.x = get_display_x();
        dst_rect.y = get_display_y();
        dst_rect.h = get_display_v();
        dst_rect.w = get_display_h();
    }
#endif

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
        display_rotate = flip_rotate % 4;//flip_rotate;
        final_rotate_angle = (ium_rotate_mode_angle + flip_rotate) % 4;//Angle superposition calculation.

        if (um_param.full_screen_en)
        {
            if ((video_width > video_height) || (ium_rotate_mode == HCCAST_UM_SCREEN_ROTATE_270) || (ium_rotate_mode == HCCAST_UM_SCREEN_ROTATE_90))
            {
                if ((rotate_seting_angle == ROTATE_TYPE_0) || (rotate_seting_angle == ROTATE_TYPE_180))
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
            expect_vd_dis_mode = DIS_PILLBOX;
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
        }
    }

    if (expect_vd_dis_mode !=  g_ium_vd_dis_mode)
    {
        if (um_param.full_screen_en != g_ium_full_screen)
            cast_api_set_aspect_mode(g_cast_dis_mode, expect_vd_dis_mode, DIS_SCALE_ACTIVE_IMMEDIATELY);
        else
            cast_api_set_aspect_mode(g_cast_dis_mode, expect_vd_dis_mode, DIS_SCALE_ACTIVE_NEXTFRAME);

        g_ium_vd_dis_mode = expect_vd_dis_mode;
        g_ium_full_screen = um_param.full_screen_en;
    }

    rotate_info->rotate_angle = final_rotate_angle;
    rotate_info->flip_mode = flip_mirror;

#ifdef SUPPORT_HID
    hccast_hid_video_info_t hid_video_info;
    if ((ium_rotate_mode == HCCAST_UM_SCREEN_ROTATE_270) || (ium_rotate_mode == HCCAST_UM_SCREEN_ROTATE_90))
    {
        hid_video_info.w = video_height;
        hid_video_info.h = video_width;
    }
    else
    {
        hid_video_info.w = video_width;
        hid_video_info.h = video_height;
    }
    hid_video_info.cast_type = HCCAST_HID_TYPE_IUM;
    hid_video_info.full_screen = (expect_vd_dis_mode == DIS_NORMAL_SCALE) ? 1 : 0;
    hid_video_info.rotate = display_rotate;
    hid_video_info.flip = flip_mirror;
    hccast_hid_video_info_set(&hid_video_info);
#endif

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
        cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
        cast_um_set_dis_zoom(&um_zoom_info);
        g_ium_vd_dis_mode = DIS_NORMAL_SCALE;
    }
    else
    {
        cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
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

                    cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    cast_um_set_dis_zoom(&um_zoom_info);
                }
                else
                {

                    cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    cast_um_set_dis_zoom(&um_zoom_info);
                }
                full_screen = 1;
            }
            else
            {
                cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                cast_um_set_dis_zoom(&um_zoom_info);
                full_screen = 0;
            }
        }
        else if (screen_mode->mode)
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

                    cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    cast_um_set_dis_zoom(&um_zoom_info);
                }
                else
                {
                    cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    cast_um_set_dis_zoom(&um_zoom_info);
                }
                full_screen = 1;
            }
            else
            {
                cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                cast_um_set_dis_zoom(&um_zoom_info);
                full_screen = 0;
            }
        }
    }
    else
    {
        cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
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

#ifdef SUPPORT_HID
    hccast_hid_video_info_t hid_video_info;
    hid_video_info.w = screen_mode->screen_width;
    hid_video_info.h = screen_mode->screen_height;
    hid_video_info.cast_type = HCCAST_HID_TYPE_AUM;
    hid_video_info.full_screen = full_screen ? 1 : 0;
    hid_video_info.rotate = display_rotate;
    hid_video_info.flip = flip_mirror;
    hccast_hid_video_info_set(&hid_video_info);
#endif

    return 0;
}

int cast_api_aum_get_preveiw_info(hccast_um_preview_info_t *preview_info, hccast_aum_screen_mode_t *screen_mode)
{
    hccast_um_param_t um_param;
    rotate_type_e rotate_seting_angle;
    int flip_rotate = 0;
    int flip_mirror = 0;
    float ratio;

    if (!screen_mode || !preview_info)
    {
        printf("preview_info: %p, screen_mode: %p error.\n", preview_info, screen_mode);
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
                            preview_info->src_rect.h = (screen_mode->screen_width * 1920) / screen_mode->screen_height;
                            preview_info->src_rect.y = (1080 - preview_info->src_rect.h) / 2;
                            
                        }
                        else
                        {
                            int tmp_h = (screen_mode->screen_width * 1920) / screen_mode->screen_height;
                            preview_info->src_rect.w = tmp_h * 1920 / 1080; //when lcd is vscreen, height is trans to width.
                            preview_info->src_rect.x = (1920 - preview_info->src_rect.w) / 2;
                            preview_info->src_rect.h = 1080;
                            preview_info->src_rect.y = 0;
                        }
                    }
                    else
                    {
                        if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                        {
                            preview_info->src_rect.w = (screen_mode->screen_height * 1080) / screen_mode->screen_width;
                            preview_info->src_rect.x = (1920 - preview_info->src_rect.w) / 2;
                        }
                        else
                        {
                            int tmp_w = (screen_mode->screen_height * 1080) / screen_mode->screen_width;
                            preview_info->src_rect.h = tmp_w * 1080 / 1920;
                            preview_info->src_rect.y = (1080 - preview_info->src_rect.h) / 2;
                            preview_info->src_rect.w = 1920;
                            preview_info->src_rect.x = 0;
                        }
                    }
                }
            }
        }
        else if (screen_mode->mode)
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
                            preview_info->src_rect.h = (screen_mode->screen_height * 1920) / screen_mode->screen_width;
                            preview_info->src_rect.y = (1080 - preview_info->src_rect.h) / 2;
                        }
                        else
                        {
                            int tmp_h = (screen_mode->screen_height * 1920) / screen_mode->screen_width;
                            preview_info->src_rect.w = tmp_h * 1920 / 1080; //when lcd is vscreen, height is trans to width.
                            preview_info->src_rect.x = (1920 - preview_info->src_rect.w) / 2;
                            preview_info->src_rect.h = 1080;
                            preview_info->src_rect.y = 0;
                        }
                    }
                    else
                    {
                        if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                        {
                            preview_info->src_rect.w = (screen_mode->screen_width * 1080) / screen_mode->screen_height;
                            preview_info->src_rect.x = (1920 - preview_info->src_rect.w) / 2;
                        }
                        else
                        {
                            int tmp_w = (screen_mode->screen_width * 1080) / screen_mode->screen_height;
                            preview_info->src_rect.h = tmp_w * 1080 / 1920;
                            preview_info->src_rect.y = (1080 - preview_info->src_rect.h) / 2;
                            preview_info->src_rect.w = 1920;
                            preview_info->src_rect.x = 0;
                        }
                    }
                }
            }
        }
    }

    return 0;
}

static char m_ium_uuid[40] = {0};
static void ium_event_process_cb(int event, void *param1, void *param2)
{
    control_msg_t ctl_msg = {0};

    if (event != HCCAST_IUM_EVT_GET_FLIP_MODE && event != HCCAST_IUM_EVT_GET_ROTATION_INFO)
        printf("ium event: %d\n", event);

    switch (event)
    {
        case HCCAST_IUM_EVT_DEVICE_ADD:
            ctl_msg.msg_type = MSG_TYPE_IUM_DEV_ADD;
            break;
        case HCCAST_IUM_EVT_DEVICE_REMOVE:
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_DEVICE_REMOVE;
            break;
        case HCCAST_IUM_EVT_MIRROR_START:
            printf("%s(), line:%d. HCCAST_IUM_EVT_MIRROR_START\n", __func__, __LINE__);
            //hccast_scene_switch(HCCAST_SCENE_IUMIRROR);
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_START;
#ifdef SUPPORT_HID
            cast_hid_start(CAST_HID_CHANNEL_USB);
#endif
            // api_osd_off_time(2000);
            api_logo_off();
            cast_api_check_dis_mode();
            g_ium_full_screen = projector_get_some_sys_param(P_UM_FULL_SCREEN);
            break;
        case HCCAST_IUM_EVT_MIRROR_STOP:
            printf("%s(), line:%d. HCCAST_IUM_EVT_MIRROR_STOP\n", __func__, __LINE__);
            //if(hccast_get_current_scene() == HCCAST_SCENE_IUMIRROR)
            {
                //    hccast_scene_reset(HCCAST_SCENE_IUMIRROR,HCCAST_SCENE_NONE);
            }
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_STOP;
#ifdef SUPPORT_HID
            cast_hid_stop();
#endif
            break;
        case HCCAST_IUM_EVT_SAVE_PAIR_DATA: //param1: buf; param2: length
            break;
        case HCCAST_IUM_EVT_GET_PAIR_DATA: //param1: buf; param2: length
            break;
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
            break;
        case HCCAST_IUM_EVT_GET_UPGRADE_DATA: //param1: hccast_ium_upg_bo_t
            break;
        case HCCAST_IUM_EVT_SAVE_UUID:
            break;
        case HCCAST_IUM_EVT_CERT_INVALID:
            ctl_msg.msg_type = MSG_TYPE_AIR_INVALID_CERT;
            m_is_demo = true;
            m_um_is_demo = true;
            printf("[%s],line:%d. HCCAST_IUM_EVT_CERT_INVALID\n", __func__, __LINE__);
            break;
        case HCCAST_IUM_EVT_NO_DATA:
        {
            printf("%s(), HCCAST_IUM_EVT_NO_DATA\n", __func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_NO_DATA;
            break;
        }
        case HCCAST_IUM_EVT_COPYRIGHT_PROTECTION:
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_COPYRIGHT_PROTECTION;
            break;
        case HCCAST_IUM_EVT_SET_DIS_ASPECT:
            cast_api_ium_set_dis_aspect_mode();
            break;
        case HCCAST_IUM_EVT_GET_ROTATION_INFO:
            cast_api_ium_get_rotate_info((hccast_ium_screen_mode_t*)param1, (hccast_um_rotate_info_t*)param2);
            break;
#ifdef SYS_ZOOM_SUPPORT
        case HCCAST_IUM_EVT_GET_PREVIEW_INFO:
        {
            hccast_um_preview_info_t preview_info = {{0, 0, 1920, 1080}, {0}, 1};

            if (projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT) == 0)
            {
                preview_info.preview_en = 0;
            }
            else
            {
                preview_info.preview_en = 1;
            }

            preview_info.dst_rect.x = get_display_x();
            preview_info.dst_rect.y = get_display_y();
            preview_info.dst_rect.w = get_display_h();
            preview_info.dst_rect.h = get_display_v();
            memcpy(param1, &preview_info, sizeof(hccast_um_preview_info_t));
            break;
        }
#endif
        case HCCAST_IUM_EVT_GET_VIDEO_CONFIG:
        {
            hccast_com_video_config_t *video_config = (hccast_com_video_config_t*)param1;
            if (api_video_pbp_get_support())
            {
                video_config->video_pbp_mode = HCCAST_COM_VIDEO_PBP_2P_ON;
                if (APP_DIS_TYPE_HD == api_video_pbp_get_dis_type(VIDEO_PBP_SRC_USB_MIRROR))
                    video_config->video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_HD;
                else
                    video_config->video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_UHD;

                if (APP_DIS_LAYER_MAIN == api_video_pbp_get_dis_layer(VIDEO_PBP_SRC_USB_MIRROR))
                    video_config->video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_MAIN;
                else
                    video_config->video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_AUXP;
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

}

static void aum_event_process_cb(int event, void *param1, void *param2)
{
    control_msg_t ctl_msg = {0};

    if (HCCAST_AUM_EVT_GET_FLIP_MODE != event && HCCAST_AUM_EVT_GET_ROTATION_INFO != event)
        printf("aum event: %d\n", event);

    switch (event)
    {
        case HCCAST_AUM_EVT_DEVICE_ADD:
            printf("%s(), line:%d. HCCAST_AUM_EVT_DEVICE_ADD\n", __func__, __LINE__);
            //hccast_scene_switch(HCCAST_SCENE_AUMIRROR);
            ctl_msg.msg_type = MSG_TYPE_AUM_DEV_ADD;
            um_start_upgrade = 0;
            break;
        case HCCAST_AUM_EVT_DEVICE_REMOVE:
            break;
        case HCCAST_AUM_EVT_MIRROR_START:
            printf("%s(), line:%d. HCCAST_AUM_EVT_MIRROR_START\n", __func__, __LINE__);
            //hccast_scene_switch(HCCAST_SCENE_AUMIRROR);
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_START;

            //api_osd_off_time(2000);
            api_logo_off();
            cast_api_check_dis_mode();
#ifdef SUPPORT_HID
            cast_hid_start(CAST_HID_CHANNEL_AUM);
#endif
            break;
        case HCCAST_AUM_EVT_MIRROR_STOP:
            printf("%s(), line:%d. HCCAST_AUM_EVT_MIRROR_STOP\n", __func__, __LINE__);
            //if(hccast_get_current_scene() == HCCAST_SCENE_AUMIRROR)
            {
                //    hccast_scene_reset(HCCAST_SCENE_AUMIRROR,HCCAST_SCENE_NONE);
            }
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_STOP;
            break;
        case HCCAST_AUM_EVT_IGNORE_NEW_DEVICE:
            break;
        case HCCAST_AUM_EVT_SERVER_MSG:
            break;
        case HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS:
        {
            int got_data_len;
            int total_data_len ;

            if (um_start_upgrade == 0)
            {
                ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_START_UPGRADE;
                ctl_msg.msg_code = MSG_TYPE_USB_UPGRADE;
                um_start_upgrade = 1;
            }
            else if (um_start_upgrade == 1)
            {
                got_data_len = (int)param1;
                total_data_len = (int)param2;
                ctl_msg.msg_type = MSG_TYPE_UPG_DOWNLOAD_PROGRESS;
                ctl_msg.msg_code = got_data_len * 100 / total_data_len;

                printf("HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS progress: %d\n", (int)ctl_msg.msg_code);
            }

            break;
        }
        case HCCAST_AUM_EVT_GET_UPGRADE_DATA:

            if (um_start_upgrade == 1)
            {
                pthread_t tid;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_attr_setstacksize(&attr, 0x2000);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                memcpy(&aum_upg_buf_info, param1, sizeof(aum_upg_buf_info));
                um_start_upgrade = 2;

                if (pthread_create(&tid, &attr, aum_burning_process, NULL) != 0)
                {
                    um_start_upgrade = 0;
                    cast_um_free_upg_buf();
                    pthread_attr_destroy(&attr);
                    return ;
                }

                pthread_attr_destroy(&attr);
            }

            break;
        case HCCAST_AUM_EVT_SET_SCREEN_ROTATE:
            printf("aum set screen rotate: %d\n", (int)param1);
            projector_set_some_sys_param(P_MIRROR_ROTATION, (int)param1);
            projector_sys_param_save();
            break;
        case HCCAST_AUM_EVT_SET_AUTO_ROTATE:
            break;
        case HCCAST_AUM_EVT_SET_FULL_SCREEN:
            //*(int*)param1 = projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT) == 0 ? 1 : 0;
            printf("aum set full screen: %d\n", (int)param1);
            projector_set_some_sys_param(P_UM_FULL_SCREEN, (int)param1);
            projector_sys_param_save();
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
#ifdef SYS_ZOOM_SUPPORT
        case HCCAST_AUM_EVT_GET_PREVIEW_INFO:
        {
            hccast_um_preview_info_t preview_info = {{0, 0, 1920, 1080}, {0}, 1};
            cast_api_aum_get_preveiw_info(&preview_info, param2);

            if (projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT) == 0)
            {
                preview_info.preview_en = 0;
            }
            else
            {
                preview_info.preview_en = 1;
            }

            preview_info.dst_rect.x = get_display_x();
            preview_info.dst_rect.y = get_display_y();
            preview_info.dst_rect.w = get_display_h();
            preview_info.dst_rect.h = get_display_v();

            memcpy(param1, &preview_info, sizeof(hccast_um_preview_info_t));
            break;
        }
#endif
        case HCCAST_AUM_EVT_GET_VIDEO_CONFIG:
        {
            hccast_com_video_config_t *video_config = (hccast_com_video_config_t*)param1;
            if (api_video_pbp_get_support())
            {
                video_config->video_pbp_mode = HCCAST_COM_VIDEO_PBP_2P_ON;
                if (APP_DIS_TYPE_HD == api_video_pbp_get_dis_type(VIDEO_PBP_SRC_USB_MIRROR))
                    video_config->video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_HD;
                else
                    video_config->video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_UHD;

                if (APP_DIS_LAYER_MAIN == api_video_pbp_get_dis_layer(VIDEO_PBP_SRC_USB_MIRROR))
                    video_config->video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_MAIN;
                else
                    video_config->video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_AUXP;
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
}

void cast_usb_mirror_rotate_init(void)
{
    hccast_um_param_t um_param = {0};
    int full_screen_en = 0;


    if (projector_get_some_sys_param(P_MIRROR_ROTATION))
        um_param.screen_rotate_en = 1;
    else
        um_param.screen_rotate_en = 0;

    if (projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION))
        um_param.screen_rotate_auto = 1;
    else
        um_param.screen_rotate_auto = 0;

    full_screen_en = projector_get_some_sys_param(P_UM_FULL_SCREEN);

    hccast_um_param_t param;
    hccast_um_param_get(&param);
    um_param.full_screen_en = full_screen_en;

    hccast_um_param_set(&um_param);
}

int cast_usb_mirror_init(void)
{
    hccast_um_init();
    if (hccast_um_init() < 0)
    {
        printf("%s(), line:%d. hccast_um_init() fail!\n", __func__, __LINE__);
        return API_FAILURE;
    }
    hccast_ium_init(ium_event_process_cb);
    cast_usb_mirror_rotate_init();

    return API_SUCCESS;
}

int cast_usb_mirror_deinit(void)
{
    if (hccast_um_deinit() < 0)
        return API_FAILURE;
    else
        return API_SUCCESS;
}

static volatile bool m_um_start = false;
int cast_usb_mirror_start(void)
{
    int ret;
    hccast_aum_param_t aum_param = {0};
    int hid_type = 0;

    if (m_um_start)
        return API_SUCCESS;

#ifdef SUPPORT_HID
    hid_type = cast_hid_probe();
    if (hid_type > 0)
    {
        hccast_ium_set_hid_onoff(1);
        hccast_aum_hid_enable(1);
    }
#endif

    ret = hccast_ium_start(m_ium_uuid, ium_event_process_cb);
    if (ret)
    {
        printf("%s(), line:%d. ret = %d\n", __func__, __LINE__, ret);
        return API_FAILURE;
    }

    strcat(aum_param.product_id, "HCT-AT01");
    snprintf(aum_param.fw_url, sizeof(aum_param.fw_url), HCCAST_APK_CONIG, (char*)projector_get_some_sys_param(P_DEV_PRODUCT_ID));
    strcat(aum_param.apk_url, HCCAST_APK_PACKAGE);
    strcat(aum_param.aoa_desc, "ElfCast-Screen_Mirror");
    aum_param.fw_version = (uint32_t)projector_get_some_sys_param(P_DEV_VERSION);

    ret = hccast_aum_start(&aum_param, aum_event_process_cb);
    if (ret)
    {
        printf("%s(), line:%d. ret = %d\n", __func__, __LINE__, ret);
        return API_FAILURE;
    }

    m_um_start = true;
    return API_SUCCESS;
}

int cast_usb_mirror_stop(void)
{
#ifdef SUPPORT_HID
    cast_hid_release();
#endif

    if (!m_um_start)
        return API_SUCCESS;

    hccast_aum_stop();
    hccast_ium_stop();

    cast_um_free_upg_buf();

    m_um_start = false;
    return API_SUCCESS;
}

bool cast_usb_mirror_start_get(void)
{
    return m_um_start;
}

#endif

int cast_air_set_default_res(void)
{
#ifdef AIRCAST_SUPPORT
#ifdef CAST_PHOTO_DETECT
    hccast_air_set_resolution(1280, 960, 60);
#else
#if CAST_720P_SUPPORT
    hccast_air_set_resolution(1280, 1280, 30);
#else
#ifdef CONFIG_SOC_HC15XX
    hccast_air_set_resolution(1920, 1080, 30);
#else
    hccast_air_set_resolution(1920, 1080, 60);
#endif
#endif
#endif
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
    hccast_mira_res_e res = HCCAST_MIRA_RES_1080P30;

    hccast_mira_service_init(hccast_mira_callback_func);

#if CAST_720P_SUPPORT
    res = HCCAST_MIRA_RES_720P30;
#else
#ifdef CONFIG_SOC_HC15XX
    res = HCCAST_MIRA_RES_1080P30;
#else
    res = HCCAST_MIRA_RES_1080P60;
#endif
#endif

    hccast_mira_service_set_resolution(res);
#endif

    cast_air_set_default_res();

#ifdef CAST_SUPPORT
    hccast_scene_init();
#endif

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
    app_log(LL_INFO, "[%s] event: %d", __func__, event);
    char *str_tmp = NULL;
    control_msg_t ctl_msg = {0};

    static char service_name[DLNA_SERVICE_NAME_LEN] = "hccast_dlna";
    static char service_ifname[DLNA_SERVICE_NAME_LEN] = DLNA_BIND_IFNAME;

    switch (event)
    {
        case HCCAST_DLNA_GET_DEVICE_NAME:
        {
            printf("[%s]HCCAST_DLNA_GET_DEVICE_NAME\n", __func__);
            if (in)
            {
                str_tmp = (char*)projector_get_some_sys_param(P_DEVICE_NAME);
                if (str_tmp)
                {
                    sprintf((char *)in, "%s_dlna", str_tmp);
                    printf("[%s]HCCAST_DLNA_GET_DEVICE_NAME:%s\n", __func__, str_tmp);
                }
            }
            break;
        }
        case HCCAST_DLNA_GET_DEVICE_PARAM:
        {
            hccast_dlna_param *dlna = (hccast_dlna_param *)in;
            if (dlna)
            {
                str_tmp = (char*)projector_get_some_sys_param(P_DEVICE_NAME);
                if (str_tmp)
                {
                    snprintf(service_name, sizeof(service_name), "%s_dlna", str_tmp);
                }

                if (WIFI_MODE_AP == app_wifi_get_work_mode())
                {
                    hccast_wifi_mgr_get_ap_ifname(service_ifname, sizeof(service_ifname));
                }
                else if (WIFI_MODE_STATION == app_wifi_get_work_mode())
                {
                    hccast_wifi_mgr_get_sta_ifname(service_ifname, sizeof(service_ifname));
                }

                dlna->svrname = service_name;
                dlna->svrport = DLNA_UPNP_PORT; // need > 49152
                dlna->ifname = service_ifname;
            }
            break;
        }
        case HCCAST_DLNA_GET_HOSTAP_STATE:
#ifdef WIFI_SUPPORT
            *(int*)in = hccast_wifi_mgr_get_hostap_status();
#endif
            break;
        case HCCAST_DLNA_HOSTAP_MODE_SKIP_URL:
            ctl_msg.msg_type = MSG_TYPE_DLNA_HOSTAP_SKIP_URL;
            break;
        case HCCAST_DLNA_GET_SAVE_AUDIO_VOL:
            *(int*)in = projector_get_some_sys_param(P_VOLUME);
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
    app_log(LL_NOTICE, "event: %d", event);
    char *str_tmp = NULL;
    control_msg_t ctl_msg = {0};

    switch (event)
    {
        case HCCAST_DIAL_GET_SVR_NAME:
        {
            if (in)
            {
                str_tmp = (char*)projector_get_some_sys_param(P_DEVICE_NAME);
                if (str_tmp)
                {
                    sprintf((char *)in, "%s_dial", str_tmp);
                    app_log(LL_INFO, "HCCAST_DIAL_GET_DEVICE_NAME:%s", str_tmp);
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
            m_is_demo = true;
            m_dial_is_demo = true;
            break;
        }

        case HCCAST_DIAL_CTRL_GET_VOL:
        {
            //app_log(LL_WARNING, "[%s] MSG_TYPE_CAST_DIAL_GET_VOL", __func__);
            if (out)
            {
                *(int*)out = projector_get_some_sys_param(P_VOLUME);
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

    return 0;
}
#endif

#define SWITCH_TIMEOUT (1000)
static pthread_mutex_t g_p2p_switch_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_p2p_switch_thread_run = 0;
static pthread_t g_p2p_tid = 0;
static int g_airp2p_thread_run = 0;
static int g_airp2p_connected = 0;
static int g_mira_connecting = 0;
static int g_airp2p_enable = 0;
static int g_mira_p2p_effected = 0;
static int g_p2p_switch_en = 0;

int cast_get_p2p_switch_enable(void)
{
    return g_p2p_switch_en;
}

int cast_set_p2p_switch_enable(int enable)
{
    g_p2p_switch_en = enable;
    return 0;
}

int cast_detect_p2p_exception(void)
{
    int ret = 0;
    
#if defined(AIRP2P_SUPPORT) && defined(__linux__)
    int fd = -1;
    char tmp[20] = {0};
    int len = 0;
    
    fd = open("/proc/net/hc/airp2p_exception", O_RDWR);
    if (fd < 0)
    {
        printf("%s open error.\n", __func__);
        return 0;
    }

    len = read(fd, tmp, sizeof(tmp));
    if (len > 0) 
    {
        if (!strncmp("1", tmp, 1)) 
        {
           printf("Detect wifi exception, restore wifi device\n");
           write(fd, "0", 1);
           ret = 1;
        }
    }
    
    close(fd);
#else
    struct ifreq ifr;
    int skfd;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        printf("%s socket error.\n", __func__);
        return 0;
    }

    strncpy(ifr.ifr_name, AIRP2P_INTF, IFNAMSIZ);
    ifr.ifr_metric = 0;
    if (ioctl(skfd, SIOCDEVPRIVATE+6, &ifr) < 0)
    {
        printf("%s Detect wifi exception fail.\n", __func__);
        close(skfd);
        return 0;
    }
    
    if (ifr.ifr_metric)
    {
        printf("Detect wifi exception, restore wifi device\n");
        ifr.ifr_metric = 0;
        ret = 1;
        if (ioctl(skfd, SIOCDEVPRIVATE+7, &ifr) < 0)
        {
            printf("%s Reset wifi exception fail.\n", __func__);
        }
    }

    close(skfd);
#endif
    return ret;
}

int cast_reset_p2p_exception(void)
{
    int ret = 0;

#if defined(AIRP2P_SUPPORT) && defined(__linux__)
    int fd = -1;

    fd = open("/proc/net/hc/airp2p_exception", O_RDWR);
    if (fd < 0)
    {
        printf("%s open error.\n", __func__);
        return 0;
    }

    write(fd, "0", 1);
    close(fd);
#else
    struct ifreq ifr;
    int skfd;

    skfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (skfd < 0)
    {
        printf("%s socket error.\n", __func__);
        return 0;
    }

    strncpy(ifr.ifr_name, AIRP2P_INTF, IFNAMSIZ);
    ifr.ifr_metric = 0;
    if (ioctl(skfd, SIOCDEVPRIVATE+7, &ifr) < 0)
    {
        printf("%s Reset wifi exception fail.\n", __func__);
        close(skfd);
        return 0;
    }
    
    close(skfd);
#endif
    return ret;
}

int cast_set_wifi_p2p_state(int enable)
{
#if defined(AIRP2P_SUPPORT) && defined(__linux__)
    int fd = -1;

    fd = open("/proc/net/hc/airp2p_enable", O_RDWR);
    if (fd < 0)
    {
        printf("%s open error.\n", __func__);
        return -1;
    }

    if (enable)
    {
        write(fd, "1", 1);
    }
    else
    {
        write(fd, "0", 1);
    }

    close(fd);
#endif
    return 0;
}

static void *cast_p2p_switch_thread(void *args)
{
#if defined(AIRP2P_SUPPORT) && defined(__linux__)
    printf("p2p switch thread is running.\n");
    unsigned long switch_time = 0;
    unsigned long time_now = 0;

    while (g_p2p_switch_thread_run)
    {
        pthread_mutex_lock(&g_p2p_switch_mutex);
        if (g_p2p_switch_thread_run == 0)
        {
            pthread_mutex_unlock(&g_p2p_switch_mutex);
            break;
        }

        if (hccast_mira_get_stat())
        {
            time_now = api_get_sys_clock_time();
            if (!g_airp2p_connected && !g_mira_connecting && ((time_now - switch_time) > SWITCH_TIMEOUT))
            {
                if (g_airp2p_enable == 0)
                {
                    hccast_wifi_mgr_p2p_send_cmd("P2P_STOP_FIND");
                    cast_set_wifi_p2p_state(1);
                    hccast_air_p2p_channel_set(projector_get_some_sys_param(P_AIRP2P_CH));
                    g_airp2p_enable = 1;
                }
                else
                {
                    hccast_wifi_mgr_p2p_send_cmd("P2P_LISTEN");
                    cast_set_wifi_p2p_state(0);
                    g_airp2p_enable = 0;
                }

                switch_time = time_now;
            }
        }

        pthread_mutex_unlock(&g_p2p_switch_mutex);
        usleep(100 * 1000);
    }

    printf("p2p switch thread exit.\n");
#endif
    return NULL;
}

void cast_p2p_switch_thread_start(void)
{
#if defined(AIRP2P_SUPPORT) && defined(__linux__)
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);

    if (g_p2p_tid > 0)
    {
        printf("p2p switch thread has run.\n");
        return ;
    }

    g_p2p_switch_thread_run = 1;
    g_airp2p_enable = 1;
    if (pthread_create(&g_p2p_tid, &attr, cast_p2p_switch_thread, NULL) < 0)
    {
        printf("Create p2p switch thread error.\n");
    }

    pthread_attr_destroy(&attr);
#endif
}

void cast_p2p_switch_thread_stop(void)
{
#if defined(AIRP2P_SUPPORT) && defined(__linux__)
    if (g_p2p_tid > 0)
    {
        g_p2p_switch_thread_run = 0;
        pthread_join(g_p2p_tid, NULL);
        g_p2p_tid = 0;
    }
#endif
}

int cast_air_set_p2p_switch(void)
{
#if defined(AIRP2P_SUPPORT) && defined(__linux__)
    pthread_mutex_lock(&g_p2p_switch_mutex);
    if (g_p2p_switch_en)
    {
        g_airp2p_connected = 1;
        g_airp2p_enable = 1;
        cast_set_wifi_p2p_state(0);
        cast_set_wifi_p2p_state(1);
        hccast_air_p2p_channel_set(projector_get_some_sys_param(P_AIRP2P_CH));
    }
    pthread_mutex_unlock(&g_p2p_switch_mutex);
#endif
    return 0;
}

int cast_mira_set_p2p_switch(void)
{
#ifdef AIRP2P_SUPPORT
    pthread_mutex_lock(&g_p2p_switch_mutex);
    if (g_p2p_switch_en)
    {
        if (g_mira_p2p_effected == 0)
        {
            g_mira_connecting = 1;
            g_airp2p_enable = 0;
            g_mira_p2p_effected = 1;
            //cast_set_wifi_p2p_state(0);
            hccast_air_service_stop();
            hccast_air_p2p_stop();
        }
    }
    pthread_mutex_unlock(&g_p2p_switch_mutex);
#endif
    return 0;
}

int cast_reset_p2p_switch_state(void)
{
#ifdef AIRP2P_SUPPORT
    pthread_mutex_lock(&g_p2p_switch_mutex);
    if (g_p2p_switch_en)
    {
        g_mira_p2p_effected = 0;
        g_mira_connecting = 0;
        g_airp2p_connected = 0;
        g_airp2p_enable = 1;
        hccast_air_p2p_start(AIRP2P_INTF, projector_get_some_sys_param(P_AIRP2P_CH));
        hccast_air_service_start();
    }
    pthread_mutex_unlock(&g_p2p_switch_mutex);
#endif
    return 0;
}

#ifdef MIRACAST_SUPPORT
static int g_mira_enable_vrotation = 0;//v_screen enable
static int g_mira_v_screen = 0;
static av_area_t g_mira_picture_info = { 0, 0, 1920, 1080};
static int g_mira_pic_vdec_w = 1920;
static int g_mira_pic_vdec_h = 1080;
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

#ifdef SYS_ZOOM_SUPPORT
    {
        dst_rect.x = get_display_x();
        dst_rect.y = get_display_y();
        dst_rect.h = get_display_v();
        dst_rect.w = get_display_h();
    }
#endif

    cast_api_set_dis_zoom(&src_rect, &dst_rect, dis_active_mode);
}

void cast_api_mira_reset_aspect_mode()
{
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};

    cast_mira_set_dis_zoom(&mira_zoom_info);
    cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
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
    int fd = open("/dev/dis", O_RDWR);
    if (fd < 0)
        return;

    dis_screen_info_t picture_info = { 0 };

    picture_info.distype = DIS_TYPE_HD;
    ioctl(fd, DIS_GET_MIRACAST_PICTURE_ARER, &picture_info);
    src_rect->x = picture_info.area.x;
    src_rect->y = picture_info.area.y;
    src_rect->w = picture_info.area.w;
    src_rect->h = picture_info.area.h;

    printf("%s %d %d %d %d\n", __FUNCTION__,
           src_rect->x,
           src_rect->y,
           src_rect->w,
           src_rect->h);
    close(fd);
}

int cast_api_mira_get_current_pic_info(struct dis_display_info *mpinfo)
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

    cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);

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

int cast_api_mira_process_rotation_change()
{
    int temp = 0;
    int rotate = 0;
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};
    int full_vscreen = projector_get_some_sys_param(P_MIRROR_FULL_VSCREEN);

    rotate = projector_get_some_sys_param(P_MIRROR_ROTATION);
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
                        cast_api_set_aspect_mode(g_cast_dis_mode , DIS_VERTICALCUT , DIS_SCALE_ACTIVE_IMMEDIATELY);
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
            if (!projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION) || !full_vscreen)
            {
                cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
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
        else if (g_mira_v_screen && !full_vscreen)
        {
            cast_mira_set_dis_zoom(&mira_zoom_info);
            cast_api_set_aspect_mode(g_cast_dis_mode, DIS_VERTICALCUT, DIS_SCALE_ACTIVE_IMMEDIATELY);
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
            if (projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION))
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
    int full_vscreen = projector_get_some_sys_param(P_MIRROR_FULL_VSCREEN);
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};
    int vscreen_auto_rotation = projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION);
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
                cast_api_set_aspect_mode(g_cast_dis_mode, DIS_VERTICALCUT, DIS_SCALE_ACTIVE_NEXTFRAME);
            }
            else
            {
                width_ori = g_mira_pic_vdec_w;
                if (g_mira_picture_info.w > g_mira_picture_info.h) //skip bad info.
                {
                    return 0;
                }

                if (vscreen_auto_rotation)
                {
                    if (last_vscreen)
                    {
                        int dis_rotate;
                        int expect_rotate = cast_api_mira_flip_rotate_covert(flip_rotate, projector_get_some_sys_param(P_MIRROR_ROTATION));
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
                    
                    cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_NEXTFRAME);
                }
                else
                {
                    mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                    cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
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
                    cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
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

                    cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, mira_zoom_info.dis_active_mode);
                    cast_mira_set_dis_zoom(&mira_zoom_info);
#else
                    cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
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
    static int vdec_first_show = 0;
    static int ui_logo_close = 0;
    int ret = 0;

    app_log(LL_INFO, "[%s] event: %d", __func__, event);

    switch (event)
    {
        case HCCAST_MIRA_GET_DEVICE_NAME:
        {
            if (in)
            {
                str_tmp = (char*)projector_get_some_sys_param(P_DEVICE_NAME);;
                if (str_tmp)
                {
                    if (m_airp2p_en)
                    {   
                        sprintf((char *)in, "%s_%s", str_tmp, AIRP2P_NAME);
                    }
                    else
                    {
                        sprintf((char *)in, "%s_mira", str_tmp);
                    }     
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
                str_tmp = (char*)projector_get_some_sys_param(P_DEVICE_NAME);;
                if (str_tmp)
                {
                    if (m_airp2p_en)
                    {   
                        sprintf((char *)p2p_param->device_name, "%s_%s", str_tmp, AIRP2P_NAME);
                    }
                    else
                    {
                        sprintf((char *)p2p_param->device_name, "%s_mira", str_tmp);
                    }
                }

                char p2p_ifname[32] = {0};
                hccast_wifi_mgr_get_p2p_ifname(p2p_ifname, sizeof(p2p_ifname));
                snprintf(p2p_param->p2p_ifname, sizeof(p2p_param->p2p_ifname), "%s", p2p_ifname);

                if (cast_get_p2p_switch_enable())
                {
                    p2p_param->listen_channel = HCCAST_P2P_LISTEN_CH_DEFAULT;
                }
                else
                {
                    int ch = hccast_wifi_mgr_get_current_freq();
                    if (ch > 0)
                    {
                        if (hccast_wifi_mgr_get_hostap_status())
                        {
                            // AP
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
                        {
                            // STA 5G
                            p2p_param->listen_channel = HCCAST_P2P_LISTEN_CH_DEFAULT;
                        }
                        else if (ch >= 1 && ch < 6)
                        {
                            // STA 2.4G
                            p2p_param->listen_channel = 6;
                        }
                        else if (ch >= 6 && ch < 11)
                        {
                            p2p_param->listen_channel = 11;
                        }
                        else if (ch >= 11 && ch <= 14)
                        {
                            p2p_param->listen_channel = 6;
                        }
                        else
                        {
                            p2p_param->listen_channel = HCCAST_P2P_LISTEN_CH_DEFAULT;
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
            }
            break;
        }
        case HCCAST_MIRA_SSID_DONE:
        {
            app_log(LL_DEBUG, "[%s]HCCAST_MIRA_SSID_DONE\n", __func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_SSID_DONE;
            break;
        }
        case HCCAST_MIRA_GET_CUR_WIFI_INFO:
        {
            app_log(LL_DEBUG, "[%s]HCCAST_MIRA_GET_CUR_WIFI_INFO\n", __func__);
            hccast_wifi_ap_info_t *cur_ap;
            char* cur_ssid = app_get_connecting_ssid();
            cur_ap = sysdata_get_wifi_info(cur_ssid);
            if (cur_ap)
            {
                memcpy(in, cur_ap, sizeof(hccast_wifi_ap_info_t));
            }
            else
            {
                snprintf(((hccast_wifi_ap_info_t*)in)->ssid, WIFI_MAX_SSID_LEN, "%s", cur_ssid);
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
#ifdef UIBC_SUPPORT
            if (usb_hid_get_total() <= 0)
            {
                int en = 1;
                hccast_mira_uibc_disable_set(&en);
            }
            else
            {
                int en = 0;
                hccast_mira_uibc_disable_set(&en);

                //hccast_mira_dev_t dev = {0};
                //dev.cat     = HCCAST_MIRA_CAT_HID;
                //dev.path    = 1; // 1=USB
                //dev.type    = 3; // 3=HID_MULTI_TOUCH
                //dev.valid   = 1;
                //hccast_mira_uibc_add_device(&dev);
            }
#endif

            //miracast connect start
            app_log(LL_DEBUG, "[%s]HCCAST_MIRA_CONNECT\n", __func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_CONNECTING;
            cast_mira_set_p2p_switch();
            break;
        }
        case HCCAST_MIRA_CONNECTED:
        {
            //miracast connect success
            app_log(LL_DEBUG, "[%s]HCCAST_MIRA_CONNECTED\n", __func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_CONNECTED;
            break;
        }
        case HCCAST_MIRA_DISCONNECT:
        {
            //miracast disconnect
            app_log(LL_DEBUG, "[%s]HCCAST_MIRA_DISCONNECT\n", __func__);
            break;
        }

        case HCCAST_MIRA_START_DISP:
        {
            //miracast start
            printf("[%s] HCCAST_MIRA_START_DISP [%d:%d]\n", __func__, vdec_first_show, ui_logo_close);

            api_set_flip_mode_enable(false);
            ui_logo_close = 1;
            api_logo_off2(0, 0);

            int rotate = projector_get_some_sys_param(P_MIRROR_ROTATION);
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
                cast_api_mira_vscreen_detect_enable(1);
                
            g_mira_full_vscreen = projector_get_some_sys_param(P_MIRROR_FULL_VSCREEN);

            cast_api_check_dis_mode();
            cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_NEXTFRAME);//DIS_PILLBOX
            cast_set_drv_hccast_type(CAST_TYPE_MIRACAST);
#ifdef __HCRTOS__
            set_eswin_drop_out_of_order_packet_flag(1);
#endif
            break;
        }
        case HCCAST_MIRA_START_FIRST_FRAME_DISP:
        {
            printf("[%s] HCCAST_MIRA_START_FIRST_FRAME_DISP [%d:%d]\n", __func__, vdec_first_show, ui_logo_close);

            vdec_first_show = 1;
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_START;
            api_set_flip_mode_enable(true);
            break;
        }
        case HCCAST_MIRA_STOP_DISP:
        {
            //miracast stop
            printf("[%s] HCCAST_MIRA_STOP_DISP [%d:%d]\n", __func__, vdec_first_show, ui_logo_close);
#ifdef __HCRTOS__
            set_eswin_drop_out_of_order_packet_flag(0);
#endif
            g_mira_enable_vrotation = 0;
            g_mira_v_screen = 0;
            cast_api_mira_reset_aspect_mode();
            cast_api_mira_vscreen_detect_enable(0);
            cast_set_drv_hccast_type(CAST_TYPE_NONE);

            if ((vdec_first_show == 0) && (ui_logo_close == 1))
            {
                api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE); //set same as win_cast_root.c
                //If there is a logo on the ui menu, please display the corresponding ui menu logo again.
                if (m_airp2p_en)
                {
                    //mean p2p mode.
                    //api_logo_show("P2P.logo");
                }
                else
                {
                    //mean classics mode.
                    api_logo_show(NULL);
                }
                ui_logo_close = 0;
                return 0;
            }

#ifdef UIBC_SUPPORT
            hccast_mira_cat_t cat = HCCAST_MIRA_CAT_NONE;
            hccast_mira_uibc_get_supported(&cat);

            if (cat & HCCAST_MIRA_CAT_HID)
            {
                uibc_hid_disable();
            }
#endif

            api_set_flip_mode_enable(true);
            vdec_first_show = 0;
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_STOP;
            break;
        }
        case HCCAST_MIRA_RESET:
        {
            printf("[%s] HCCAST_MIRA_RESET\n", __func__);
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_RESET;
            ctl_msg.msg_code = *(uint32_t*) in;
            bool *valid = (bool*) out;
            if (valid)
            {
                *valid = true;
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

        case HCCAST_MIRA_GOT_IP:
            ctl_msg.msg_type = MSG_TYPE_CAST_MIRACAST_GOT_IP;
            break;

        case HCCAST_MIRA_GET_CONTINUE_ON_ERROR:
            if (out)
            {
                *(uint8_t *)out = projector_get_some_sys_param(P_MIRA_CONTINUE_ON_ERROR);
            }
            break;
#ifdef SYS_ZOOM_SUPPORT
        case HCCAST_MIRA_GET_PREVIEW_INFO:
        {
            hccast_mira_preview_info_t preview_info = {{0, 0, 1920, 1080}, {0}, 1};

            if (projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT) == 0)
            {
                preview_info.preview_en = 0;
            }
            else
            {
                preview_info.preview_en = 1;
            }

            preview_info.dst_rect.x = get_display_x();
            preview_info.dst_rect.y = get_display_y();
            preview_info.dst_rect.w = get_display_h();
            preview_info.dst_rect.h = get_display_v();
            memcpy(in, &preview_info, sizeof(hccast_mira_preview_info_t));
            break;
        }
#endif
        case HCCAST_MIRA_GET_VIDEO_CONFIG:
        {
            hccast_com_video_config_t *video_config = (hccast_com_video_config_t*)in;
            if (api_video_pbp_get_support())
            {
                video_config->video_pbp_mode = HCCAST_COM_VIDEO_PBP_2P_ON;

                if (APP_DIS_TYPE_HD == api_video_pbp_get_dis_type(VIDEO_PBP_SRC_MIRACAST))
                    video_config->video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_HD;
                else
                    video_config->video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_UHD;

                if (APP_DIS_LAYER_MAIN == api_video_pbp_get_dis_layer(VIDEO_PBP_SRC_MIRACAST))
                    video_config->video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_MAIN;
                else
                    video_config->video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_AUXP;

            }
            break;
        }

#ifdef UIBC_SUPPORT
        case HCCAST_MIRA_UIBC_ENABLE:
        {
            hccast_mira_cat_t cat = HCCAST_MIRA_CAT_NONE;
            hccast_mira_uibc_get_supported(&cat);

            if (cat & HCCAST_MIRA_CAT_HID)
            {
                uibc_hid_enable();
            }
            break;
        }
        case HCCAST_MIRA_UIBC_DISABLE:
        {
            hccast_mira_cat_t cat = HCCAST_MIRA_CAT_NONE;
            hccast_mira_uibc_get_supported(&cat);

            if (cat & HCCAST_MIRA_CAT_HID)
            {
                uibc_hid_disable();
            }
            break;
        }
#endif

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
        if (cast_main_ui_wait_ready)
            cast_main_ui_wait_ready(20000);
        //printf("[%s] wait cast root end tick: %d\n",__func__,(int)time(NULL));
    }

    if (MSG_TYPE_CAST_MIRACAST_START == ctl_msg.msg_type)
    {
        //printf("[%s] wait cast play start tick: %d\n",__func__,(int)time(NULL));
        if (mira_ui_wait_ready)
            mira_ui_wait_ready(20000);
        //printf("[%s] wait cast play end tick: %d\n",__func__,(int)time(NULL));
#if CASTING_CLOSE_FB_SUPPORT
        api_osd_show_onoff(false);
#endif
    }

    return ret;
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

    int rotate = projector_get_some_sys_param(P_MIRROR_ROTATION);
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
    g_air_feed_data_tick = api_get_sys_clock_time();
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
        if (g_air_dis_rotate!= -1 && (api_get_sys_clock_time() - g_air_feed_data_tick >= 200))
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

                if (projector_get_some_sys_param(P_MIRROR_FULL_VSCREEN) == 0)
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

                if (projector_get_some_sys_param(P_MIRROR_FULL_VSCREEN) == 0)
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

            g_air_dis_rotate = cast_air_flip_rotate_covert(flip_rotate, projector_get_some_sys_param(P_MIRROR_ROTATION));
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
            g_air_photo_vscreen = 1;
            g_air_detect_zoomed = 1;
        }
        else
        {
            if (projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION))
                g_air_dis_rotate = cast_air_flip_rotate_covert(flip_rotate, ROTATE_TYPE_0); 

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
        int expect_dis_rotate = cast_air_flip_rotate_covert(flip_rotate, projector_get_some_sys_param(P_MIRROR_ROTATION));
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
    int rotate = projector_get_some_sys_param(P_MIRROR_ROTATION);
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
    int full_vscreen = projector_get_some_sys_param(P_MIRROR_FULL_VSCREEN);
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
    seting_rotate = projector_get_some_sys_param(P_MIRROR_ROTATION);
    
    if(((seting_rotate == MIRROR_ROTATE_90) || (seting_rotate == MIRROR_ROTATE_270)) && !g_air_photo_vscreen)
    {
        if (rotate_info->src_w >= rotate_info->src_h)
        {
            if (projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION))
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
    int flip_rotate = 0;
    int flip_mirror = 0;
    av_area_t m_air_picture_info = { 0, 0, 1920, 1080};
    int m_air_pic_vdec_w = 1920;
    int m_air_pic_vdec_h = 1080;
    cast_air_zoom_info_t air_zoom_info = {{ 0, 0, 1920, 1080 }, { 0, 0, 1920, 1080 }, DIS_SCALE_ACTIVE_IMMEDIATELY};
    api_get_flip_info(&flip_rotate, &flip_mirror);

    printf("%s %d.\n", __func__, __LINE__);

    pthread_mutex_lock(&m_air_rotate_mutex);
    
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

                if (projector_get_some_sys_param(P_MIRROR_FULL_VSCREEN) == 0)
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

                if (projector_get_some_sys_param(P_MIRROR_FULL_VSCREEN) == 0)
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

    pthread_mutex_unlock(&m_air_rotate_mutex);
}

void cast_air_reset_video(void)
{
    int flip_rotate = 0;
    int flip_mode = 0;
    int rotation_angle;

    pthread_mutex_lock(&m_air_rotate_mutex);
    int seting_rotate = projector_get_some_sys_param(P_MIRROR_ROTATION);

    if (g_air_photo_vscreen && ((seting_rotate == MIRROR_ROTATE_90) || (seting_rotate == MIRROR_ROTATE_270)))
    {
        cast_air_reset_zoom();
    }
    else if (((seting_rotate == MIRROR_ROTATE_90) || (seting_rotate == MIRROR_ROTATE_270)))
    {
        if (g_air_video_width >= g_air_video_height) 
        {
            if(projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION))
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
    int ret = 0;
    control_msg_t ctl_msg = {0};
    int menu_status = 1;//0 -- win_cast_play ui menu not open, 1 -- open.
    int mode_set, air_mode;
    static int airmirror_started = 0;

    switch (event)
    {
        case HCCAST_AIR_GET_SERVICE_NAME:
            if (m_airp2p_en)
            {
                sprintf((char *)in, "%s_%s", (char*)projector_get_some_sys_param(P_DEVICE_NAME), AIRP2P_NAME);
            }
            else
            {
                sprintf((char *)in, "%s_%s", (char*)projector_get_some_sys_param(P_DEVICE_NAME), AIRCAST_NAME);
            }
            break;
        case HCCAST_AIR_GET_NETWORK_DEVICE:
            sprintf((char *)in, "%s", "wlan0");
            break;
        case HCCAST_AIR_GET_MIRROR_MODE:
            mode_set = projector_get_some_sys_param(P_AIRCAST_MODE);
            air_mode = HCCAST_AIR_MODE_MIRROR_ONLY;
            if (0 == mode_set)
            {
                air_mode = HCCAST_AIR_MODE_MIRROR_STREAM;
            }
            else if (1 == mode_set)
            {
                air_mode = HCCAST_AIR_MODE_MIRROR_ONLY;
            }
            else
            {
#ifdef WIFI_SUPPORT
                if (hccast_wifi_mgr_get_hostap_status())
                {
                    air_mode = HCCAST_AIR_MODE_MIRROR_ONLY;
                }
                else
                {
                    air_mode = HCCAST_AIR_MODE_MIRROR_STREAM;
                }
#endif
            }
            *(int*)in = air_mode;
            break;
        case HCCAST_AIR_GET_NETWORK_STATUS:
#ifdef WIFI_SUPPORT
            *(int*)in = hccast_wifi_mgr_get_hostap_status();
#endif
            break;
        case HCCAST_AIR_MIRROR_START:
#ifdef CAST_PHOTO_DETECT            
            cast_air_vscreen_detect_start();
#endif            
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRMIRROR_START;
            cast_api_check_dis_mode();
            g_air_vd_dis_mode = -1;
            g_air_full_vscreen = projector_get_some_sys_param(P_MIRROR_FULL_VSCREEN);
            cast_set_drv_hccast_type(CAST_TYPE_AIRCAST);
#ifdef __HCRTOS__
            set_eswin_drop_out_of_order_packet_flag(1);
#endif
            break;
        case HCCAST_AIR_MIRROR_STOP:
#ifdef CAST_PHOTO_DETECT            
            cast_air_vscreen_detect_stop();
#endif            
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRMIRROR_STOP;
            cast_set_drv_hccast_type(CAST_TYPE_NONE);
#ifdef __HCRTOS__
            set_eswin_drop_out_of_order_packet_flag(0);
#endif
            g_airp2p_connected = 0;
            break;
        case HCCAST_AIR_AUDIO_START:
            if (!airmirror_started)
            {
                cast_air_set_p2p_switch();
            }
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRCAST_AUDIO_START;
            break;
        case HCCAST_AIR_AUDIO_STOP:
            if (!airmirror_started)
            {
                g_airp2p_connected = 0;
            }
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRCAST_AUDIO_STOP;
            break;
        case HCCAST_AIR_INVALID_CERT:
            ctl_msg.msg_type = MSG_TYPE_AIR_INVALID_CERT;
            m_is_demo = true;
            m_air_is_demo = true;
            printf("[%s],line:%d. HCCAST_AIR_INVALID_CERT\n", __func__, __LINE__);
            break;
        case HCCAST_AIR_P2P_INVALID_CERT:
            ctl_msg.msg_type = MSG_TYPE_AIR_INVALID_CERT;
            m_is_demo = true;
            m_airp2p_is_demo = true;
            printf("[%s],line:%d. HCCAST_AIR_P2P_INVALID_CERT\n",__func__, __LINE__);
            break;        
        case HCCAST_AIR_GET_4K_MODE:
            if (projector_get_some_sys_param(P_DE_TV_SYS) < TV_LINE_4096X2160_30)
            {
                *(int*)in = 0;
                printf("[%s] NOT 4K MODE, tv_sys:%d\n", __func__, projector_get_some_sys_param(P_DE_TV_SYS));

            }
            else
            {
                *(int*)in = 1;
                printf("[%s] NOW IS 4K MODE, tv_sys:%d\n", __func__, projector_get_some_sys_param(P_DE_TV_SYS));
            }

            break;
        case HCCAST_AIR_HOSTAP_MODE_SKIP_URL:
            ctl_msg.msg_type = MSG_TYPE_AIR_HOSTAP_SKIP_URL;
            printf("[%s]HCCAST_AIR_HOSTAP_MODE_SKIP_URL\n", __func__);
            break;
        case HCCAST_AIR_BAD_NETWORK:
            ctl_msg.msg_type = MSG_TYPE_AIR_MIRROR_BAD_NETWORK;
            printf("[%s]HCCAST_AIR_BAD_NETWORK\n", __func__);
            break;
        case HCCAST_AIR_URL_ENABLE_SET_DEFAULT_VOL:
            *(int*)in = 0;
            break;
        case HCCAST_AIR_SET_AUDIO_VOL:
            printf("%s set vol:%d\n", __func__, (int)in);
            /*
            cast_api_set_volume((int)in);
            projector_set_some_sys_param(P_VOLUME, (int)in);
            projector_sys_param_save();
            */
            ctl_msg.msg_type = MSG_TYPE_CAST_AIRCAST_VOL_SET;
            ctl_msg.msg_code = (uint32_t)in;
            break;
        case HCCAST_AIR_GET_MIRROR_ROTATION_INFO:
            cast_air_get_rotation_info((hccast_air_rotation_t*)in);
            break;
        case HCCAST_AIR_GET_ABDISCONNECT_STOP_PLAY_EN:
            if (in)
            {
                *(int*)in = 0;
            }
            break;
#ifdef AIRP2P_SUPPORT
        case HCCAST_AIR_GET_AIRP2P_PIN:
            sprintf((char *)in, "%s", "1234");
            break;
        case HCCAST_AIR_GET_MIRROR_QUICK_MODE_NUM:
            if (network_get_airp2p_state())
            {
                *(int*)in = 7;//set to support the max level.
            }
            break;
#endif
#ifdef SYS_ZOOM_SUPPORT
        case HCCAST_AIR_GET_PREVIEW_INFO:
        {
            hccast_air_deview_t preview_info = {{0, 0, 1920, 1080}, {0}, 1};

            if (projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT) == 0)
            {
                preview_info.preview_en = 0;
            }
            else
            {
                preview_info.preview_en = 1;
            }

            preview_info.dst_rect.x = get_display_x();
            preview_info.dst_rect.y = get_display_y();
            preview_info.dst_rect.w = get_display_h();
            preview_info.dst_rect.h = get_display_v();
            memcpy(in, &preview_info, sizeof(hccast_air_deview_t));
            break;
        }
#endif
        case HCCAST_AIR_GET_VIDEO_CONFIG:
        {
            hccast_com_video_config_t *video_config = (hccast_com_video_config_t*)in;
            if (api_video_pbp_get_support())
            {
                video_config->video_pbp_mode = HCCAST_COM_VIDEO_PBP_2P_ON;

                if (APP_DIS_TYPE_HD == api_video_pbp_get_dis_type(VIDEO_PBP_SRC_AIRCAST))
                    video_config->video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_HD;
                else
                    video_config->video_dis_type = HCCAST_COM_VIDEO_DIS_TYPE_UHD;

                if (APP_DIS_LAYER_MAIN == api_video_pbp_get_dis_layer(VIDEO_PBP_SRC_AIRCAST))
                    video_config->video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_MAIN;
                else
                    video_config->video_dis_layer = HCCAST_COM_VIDEO_DIS_LAYER_AUXP;
            }
            break;
        }
        case HCCAST_AIR_PHONE_CONNECT:
            cast_air_set_p2p_switch();
            break;
        case HCCAST_AIR_PHONE_DISCONNECT:
            if (g_p2p_switch_en)
            {
                printf("[%s]Airp2p device disconnected.\n", __func__);
                g_airp2p_connected = 0;
            }      
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
        printf("[%s] wait cast root start tick: %d\n", __func__, (int)time(NULL));
        if (cast_main_ui_wait_ready)
            cast_main_ui_wait_ready(20000);
        printf("[%s] wait cast root end tick: %d\n", __func__, (int)time(NULL));
    }

    if (MSG_TYPE_CAST_AIRMIRROR_START == ctl_msg.msg_type)
    {
        printf("[%s] wait cast play start tick: %d\n", __func__, (int)time(NULL));
        if (air_ui_wait_ready)
        {
            menu_status = air_ui_wait_ready(20000);
        }

        printf("[%s] wait cast play end tick: %d\n", __func__, (int)time(NULL));

        if (menu_status == 0)
        {
            if (in)
            {
                *(int *)in = 0;//in: 0 -- mean not open mirror video, 1 -- mean open mirror video.
            }
            printf("[%s] win cast play menu not open: %d\n", __func__, menu_status);
            return 0;
        }

#if CASTING_CLOSE_FB_SUPPORT
        api_osd_show_onoff(false);
#endif
    }

    return ret;
}
#endif

void cast_restart_services()
{
    if (hccast_get_current_scene() != HCCAST_SCENE_NONE)
    {
        //hccast_scene_switch(HCCAST_SCENE_NONE);
    }

    printf("[%s]  begin restart services.\n", __func__);
#ifdef DLNA_SUPPORT
    hccast_dlna_service_stop();
    hccast_dlna_service_start();
#ifdef DIAL_SUPPORT
    hccast_dial_service_stop();
    hccast_dial_service_start();
#endif
#endif

#ifdef MIRACAST_SUPPORT
    hccast_air_service_stop();
    hccast_air_service_start();
#endif

}

void restart_air_service_by_hdmi_change(void)
{
#ifdef AIRCAST_SUPPORT
    int cur_scene = 0;
    cur_scene = hccast_get_current_scene();
    if ((cur_scene != HCCAST_SCENE_AIRCAST_PLAY) && (cur_scene != HCCAST_SCENE_AIRCAST_MIRROR))
    {
        if (hccast_air_service_is_start())
        {
            hccast_air_service_stop();
            hccast_air_service_start();
        }
    }
#endif
}

bool cast_is_demo(void)
{
    return m_is_demo;
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

bool cast_airp2p_is_demo(void)
{
    return m_airp2p_is_demo;
}

void cast_dlna_ui_wait_init(cast_ui_wait_ready_func ready_func)
{
    dlna_ui_wait_ready = ready_func;
}
void cast_mira_ui_wait_init(cast_ui_wait_ready_func ready_func)
{
    mira_ui_wait_ready = ready_func;
}
void cast_air_ui_wait_init(cast_ui_wait_ready_func ready_func)
{
    air_ui_wait_ready = ready_func;
}
void cast_main_ui_wait_init(cast_ui_wait_ready_func ready_func)
{
    cast_main_ui_wait_ready = ready_func;
}

cast_dial_conn_e cast_dial_connect_state(void)
{
    return m_dial_conn_state;
}

void cast_airp2p_enable(int enable)
{
    m_airp2p_en = enable;
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
#endif
