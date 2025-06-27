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
#ifdef SUPPORT_UM_SLAVE
#include <kernel/drivers/hcusb.h>
#endif
#ifdef __HCRTOS__
#include <kernel/lib/fdt_api.h>
#endif

#include "com_api.h"
#include "osd_com.h"
#include "cast_api.h"
#include "data_mgr.h"
#include "cast_hid.h"
#include <hidalgo/hccast_hid.h>

#define DEFAULT_FRM_SIZE    (0x100000)

static bool m_is_demo = false;

#ifdef USBMIRROR_SUPPORT
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

static void ium_event_process_cb(int event, void *param1, void *param2);

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
                if (um_param.screen_rotate_auto)
                {
                    final_rotate_angle = (ROTATE_TYPE_0 + flip_rotate) % 4;
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

static void ium_event_process_cb(int event, void *param1, void *param2)
{
    control_msg_t ctl_msg = {0};
    int got_data_len;
    int total_data_len ;
    app_data_t* app_data = data_mgr_app_get();

    if ((event != HCCAST_IUM_EVT_GET_FLIP_MODE) \
        && (event != HCCAST_IUM_EVT_UPG_DOWNLOAD_PROGRESS) \
        && (event != HCCAST_IUM_EVT_GET_ROTATION_INFO))
        printf("ium event: %d\n", event);

    switch (event)
    {
        case HCCAST_IUM_EVT_DEVICE_ADD:
            um_start_upgrade = 0;
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
            api_osd_off_time(1000);

            cast_api_set_volume(100);
            printf("%s set vol to 100\n", __func__);

            break;
        case HCCAST_IUM_EVT_MIRROR_STOP:
            printf("%s(), line:%d. HCCAST_IUM_EVT_MIRROR_STOP\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_CAST_IUSB_STOP;
#ifdef SUPPORT_HID
            cast_hid_stop();
#endif
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
                ctl_msg.msg_type = MSG_TYPE_IUM_START_UPGRADE;
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
            ctl_msg.msg_type = MSG_TYPE_UM_INVALID_CERT;
            m_is_demo = true;
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
}

static void aum_event_process_cb(int event, void *param1, void *param2)
{
    control_msg_t ctl_msg = {0};
    int got_data_len;
    int total_data_len ;
    app_data_t* app_data = data_mgr_app_get();

    if ((HCCAST_AUM_EVT_GET_FLIP_MODE != event) \
        && (HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS != event) \
        && (HCCAST_AUM_EVT_GET_ROTATION_INFO != event))
        printf("aum event: %d\n", event);

    switch (event)
    {
        case HCCAST_AUM_EVT_DEVICE_ADD:
            printf("%s(), line:%d. HCCAST_AUM_EVT_DEVICE_ADD\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_AUM_DEV_ADD;
            um_start_upgrade = 0;

            break;
        case HCCAST_AUM_EVT_DEVICE_REMOVE:
            break;
        case HCCAST_AUM_EVT_MIRROR_START:
            printf("%s(), line:%d. HCCAST_AUM_EVT_MIRROR_START\n", __func__, __LINE__);
            //hccast_scene_switch(HCCAST_SCENE_AUMIRROR);
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_START;
#ifdef SUPPORT_HID
#ifdef SUPPORT_UM_SLAVE        
            cast_hid_start(CAST_HID_CHANNEL_USB);
#else
            cast_hid_start(CAST_HID_CHANNEL_AUM);
#endif
#endif
            api_osd_off_time(1000);

            cast_api_set_volume(100);
            printf("%s set vol to 100\n", __func__);

            break;
        case HCCAST_AUM_EVT_MIRROR_STOP:
            ctl_msg.msg_type = MSG_TYPE_CAST_AUSB_STOP;
#ifdef SUPPORT_HID
            cast_hid_stop();
#endif
            break;
        case HCCAST_AUM_EVT_IGNORE_NEW_DEVICE:
            break;
        case HCCAST_AUM_EVT_SERVER_MSG:
            break;
        case HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS:

            if (um_start_upgrade == 0)
            {
                ctl_msg.msg_type = MSG_TYPE_IUM_START_UPGRADE;
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
            data_mgr_cast_full_screen_set((int)param1);
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
        case HCCAST_AUM_EVT_CERT_INVALID:
            printf("[%s],line:%d. HCCAST_AUM_EVT_CERT_INVALID\n", __func__, __LINE__);
            ctl_msg.msg_type = MSG_TYPE_UM_INVALID_CERT;
            m_is_demo = true;
            break;
        default:
            break;
    }
    if (0 != ctl_msg.msg_type)
    {
        api_control_send_msg(&ctl_msg);
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
    um_param.full_screen_en = data_mgr_cast_full_screen_get();
    hccast_um_param_set(&um_param);

#ifdef SUPPORT_IUM
    hccast_ium_init(ium_event_process_cb);
#endif

#if defined(SUPPORT_AUM) && defined(SUPPORT_UM_SLAVE)
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

int cast_usb_mirror_start(void)
{
    int ret;
    hccast_aum_param_t aum_param = {0};
    sys_data_t* sys_data = data_mgr_sys_get();
    app_data_t* app_data = data_mgr_app_get();
    char *udc_name = NULL;
    hccast_um_dev_type_e gadget_type = HCCAST_UM_DEV_IUM;
    int hid_type = 0;

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

#ifdef SUPPORT_IUM
    hccast_ium_set_frm_buf(g_um_frame_buf, g_um_frame_size);

    ret = hccast_ium_start(app_data->ium_uuid, ium_event_process_cb);
    if (ret)
    {
        printf("%s(), line:%d. ret = %d\n", __func__, __LINE__, ret);
        return API_FAILURE;
    }
#endif

#ifdef SUPPORT_AUM
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
#endif

#ifdef SUPPORT_HID
    hid_type = cast_hid_probe();
    if (hid_type > 0)
    {
        gadget_type = HCCAST_UM_DEV_IUM_HID;
        hccast_ium_set_hid_onoff(1);
        hccast_aum_hid_enable(1);
    }
#endif

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

#ifdef SUPPORT_AUM
    if (hid_type > 0)
    {
        gadget_type = HCCAST_UM_DEV_IUM_UAC_HID;
    }
    else
    {
        gadget_type = HCCAST_UM_DEV_IUM_UAC;
    }
    hccast_aum_slave_start(g_slave_port, udc_name, gadget_type);
#endif

#ifdef SUPPORT_IUM
    hccast_ium_slave_start(g_slave_port, udc_name, gadget_type);
#endif
#endif

    return API_SUCCESS;
}

int cast_usb_mirror_stop(void)
{
#ifdef SUPPORT_HID
    cast_hid_release();
#endif

#ifdef SUPPORT_IUM
    hccast_ium_stop();
#ifdef SUPPORT_UM_SLAVE
    if ((0 == g_slave_port) || (1 == g_slave_port))
    {
        hccast_ium_slave_stop(g_slave_port);
    }
#endif
    hccast_ium_set_frm_buf(NULL, 0);
#endif

#ifdef SUPPORT_AUM
#ifdef SUPPORT_UM_SLAVE
    if ((0 == g_slave_port) || (1 == g_slave_port))
    {
        hccast_aum_slave_stop(g_slave_port);
    }
#endif
    hccast_aum_stop();
    hccast_aum_set_frm_buf(NULL, 0);

#endif

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

int cast_init(void)
{
    hccast_scene_init();
    api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
    return API_SUCCESS;
}

int cast_deinit(void)
{
#ifdef USBMIRROR_SUPPORT
    cast_usb_mirror_stop();
    cast_usb_mirror_deinit();
#endif

    return API_SUCCESS;
}

bool cast_is_demo(void)
{
    return m_is_demo;
}
