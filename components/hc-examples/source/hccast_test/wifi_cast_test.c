#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>


#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <termios.h>
#include <signal.h>
#include "console.h"
#else
#include <kernel/lib/console.h>
#endif

#include "hccast_test.h"

#include <hcuapi/dis.h>
#include <ffplayer.h>

#include <hccast/hccast_scene.h>
#include <hccast/hccast_net.h>
#include <hccast/hccast_com.h>
#include <hccast/hccast_wifi_mgr.h>

#ifdef DLNA_SUPPORT
#include <hccast/hccast_dlna.h>
#include <hccast/hccast_media.h>
#endif

#ifdef DIAL_SUPPORT
#include <hccast/hccast_dial.h>
#endif

#ifdef AIRCAST_SUPPORT
#include <hccast/hccast_air.h>
#endif

#ifdef MIRACAST_SUPPORT
#include <hccast/hccast_mira.h>
#endif

#include <hudi/hudi_audsink.h>

#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <arpa/inet.h>

static hccast_test_state_t m_wifi_cast_state[HCCAST_TEST_WIFI_TYPE_MAX];

#ifdef MIRACAST_SUPPORT

static int g_mira_enable_vrotation = 0;//v_screen enable
static int g_mira_v_screen = 0;
static av_area_t g_mira_picture_info = { 0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT};
static int g_mira_pic_vdec_w;
static int g_mira_pic_vdec_h;
static int g_mira_full_vscreen = 0;

static int hctest_mira_auto_get_rotation_info(hccast_mira_rotation_t* rotate_info);
static int hctest_mira_process_rotation_change(void);
static void hctest_mira_vscreen_detect_enable(int enable);
static void hctest_mira_set_dis_zoom(hccast_mira_zoom_info_t *mira_zoom_info);
static int hctest_mira_reset_vscreen_zoom(void);
static char m_wifi_cast_mac[64] = {0,};

static volatile bool m_cast_wifi_start = false;

static char *get_cast_name(void)
{
    struct ifreq ifr;
    int skfd;
    char *mac_str = NULL;
    char mac[6];

    do {
        if (strlen(m_wifi_cast_mac) && strcmp(m_wifi_cast_mac, HCCAST_DEF_CAST_NAME)){
            mac_str = m_wifi_cast_mac;
            break;
        }

        memset(m_wifi_cast_mac, 0, sizeof(m_wifi_cast_mac));

        if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            printf("socket error\n");
            break;
        }

        strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ);
        if (ioctl(skfd, SIOCGIFHWADDR, &ifr) != 0) {
            printf( "%s net_get_hwaddr: ioctl SIOCGIFHWADDR\n",__func__);
            break;
        }
        close(skfd);
        memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);

        snprintf(m_wifi_cast_mac, 64, "%s-%02X%02X%02X", 
            HCCAST_DEF_CAST_NAME, mac[3]&0xff, mac[4]&0xff, mac[5]&0xff);   

        mac_str = m_wifi_cast_mac;
    }while (0);

    if (NULL == mac_str)
        mac_str = HCCAST_DEF_CAST_NAME;

    printf("*** cast name: %s ***\n", mac_str);
    return mac_str;
}

static void hctest_mira_set_dis_zoom(hccast_mira_zoom_info_t *mira_zoom_info)
{
    hccast_test_rect_t src_rect;
    hccast_test_rect_t dst_rect;

    int dis_active_mode;
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];    

    dis_active_mode = mira_zoom_info->dis_active_mode;

    src_rect.x = mira_zoom_info->src_rect.x;
    src_rect.y = mira_zoom_info->src_rect.y;
    src_rect.w = mira_zoom_info->src_rect.w;
    src_rect.h = mira_zoom_info->src_rect.h;

    dst_rect.x = cast_state->dst_rect.x;
    dst_rect.y = cast_state->dst_rect.y;
    dst_rect.w = cast_state->dst_rect.w;
    dst_rect.h = cast_state->dst_rect.h;

//    cast_api_set_dis_zoom(&src_rect, &dst_rect, dis_active_mode);
    hccast_test_display_zoom(cast_state->dis_type, cast_state->dis_layer, 
        &src_rect, &dst_rect, dis_active_mode);

}


static void hctest_mira_vscreen_detect_enable(int enable)
{
    int fd = -1;
    struct dis_miracast_vscreen_detect_param mpara = { 0 };
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];

    fd = open("/dev/dis", O_RDWR);
    if (fd < 0)
    {
        return ;
    }

    if (HCCAST_TEST_DIS_TYPE_HD == cast_state->dis_type)
    {
        mpara.distype =  DIS_TYPE_HD;
    }
    else 
    {    
        mpara.distype =  DIS_TYPE_UHD;
    }

	if (HCCAST_TEST_DIS_LAYER_MAIN == cast_state->dis_layer)
        mpara.layer = DIS_LAYER_MAIN;
    else
        mpara.layer = DIS_LAYER_AUXP;

    if (enable)
    {
        mpara.on = 1;
    }
    else
    {
        mpara.on = 0;
    }

    ioctl(fd, DIS_SET_MIRACAST_VSRCEEN_DETECT, &mpara);

    close(fd);
}


static int hctest_mira_reset_vscreen_zoom(void)
{
    int width_ori = g_mira_pic_vdec_w;
    int flip_rotate = 0;
    int flip_mirror = 0;
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];    

    hctest_get_flip_info(cast_state, &flip_rotate, &flip_mirror);
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT }, { 0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT }, DIS_SCALE_ACTIVE_IMMEDIATELY};

    hccast_test_set_aspect_mode(cast_state, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
    
    if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
    {
        mira_zoom_info.src_rect.x = 0;
        mira_zoom_info.src_rect.y = (g_mira_picture_info.x * HCCAST_TEST_SCREEN_HEIGHT) / width_ori;
        mira_zoom_info.src_rect.w = HCCAST_TEST_SCREEN_WIDTH;
        mira_zoom_info.src_rect.h = HCCAST_TEST_SCREEN_HEIGHT - 2 * mira_zoom_info.src_rect.y;
        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
        hctest_mira_set_dis_zoom(&mira_zoom_info);
    }
    else
    {
        mira_zoom_info.src_rect.x = (g_mira_picture_info.x * HCCAST_TEST_SCREEN_WIDTH) / width_ori;
        mira_zoom_info.src_rect.y = 0;
        mira_zoom_info.src_rect.w = HCCAST_TEST_SCREEN_WIDTH - 2 * mira_zoom_info.src_rect.x;
        mira_zoom_info.src_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
        hctest_mira_set_dis_zoom(&mira_zoom_info);
    }
    
    printf("%s\n", __func__);
    
    return 0;   
}

static int hctest_mira_process_rotation_change(void)
{
    int temp = 0;
    int rotate = 0;
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT }, { 0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT }, DIS_SCALE_ACTIVE_NEXTFRAME};
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];    
    int full_vscreen = cast_state->mirror_full_vscreen;

    //rotate = projector_get_some_sys_param(P_MIRROR_ROTATION);
    rotate = cast_state->rotate_mode;
    if ((rotate == HCTEST_ROTATE_90) || (rotate == HCTEST_ROTATE_270))
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
            hctest_mira_vscreen_detect_enable(1);
            g_mira_enable_vrotation = 1;
        }
        else
        {
            // if (!projector_get_some_sys_param(P_MIRROR_VSCREEN_AUTO_ROTATION) || !full_vscreen)
            // {
            //     hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
            // }
            if (!cast_state->mirror_vscreen_auto_rotation || !full_vscreen)
            {
                hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
            }
            
            hctest_mira_vscreen_detect_enable(0);
            hctest_mira_set_dis_zoom(&mira_zoom_info);
            g_mira_enable_vrotation = 0;
            g_mira_v_screen = 0;
        }
    }

    if (g_mira_full_vscreen != full_vscreen && g_mira_enable_vrotation)
    {
        if (g_mira_v_screen && full_vscreen)
        {
            hctest_mira_reset_vscreen_zoom();
        }
        else if(g_mira_v_screen && !full_vscreen)
        {
            //cast_api_set_aspect_mode(g_cast_dis_mode , DIS_VERTICALCUT , DIS_SCALE_ACTIVE_IMMEDIATELY);
            hccast_test_set_aspect_mode(cast_state, DIS_VERTICALCUT, DIS_SCALE_ACTIVE_IMMEDIATELY);
        }
        
        g_mira_full_vscreen = full_vscreen;
    }

    return rotate;
}

static int hctest_mira_auto_get_rotation_info(hccast_mira_rotation_t* rotate_info)
{
    int seting_rotate;
    int rotation_angle = HCTEST_ROTATE_0;
    int flip_mode;
    int flip_rotate;
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];

    if (!rotate_info)
    {
        return -1;
    }
      
    seting_rotate = hctest_mira_process_rotation_change();
    hctest_get_flip_info(cast_state, &flip_rotate, &flip_mode);

    if (g_mira_enable_vrotation)
    {
        if (g_mira_v_screen)
        {
            rotation_angle = hccast_test_rotate_convert(flip_rotate, seting_rotate);
        }
        else
        {
            if (cast_state->mirror_vscreen_auto_rotation)
            {
                 rotation_angle = flip_rotate;
            }
            else
            {
                rotation_angle = hccast_test_rotate_convert(flip_rotate, seting_rotate);
            }
        }
    }
    else
    {
        rotation_angle = hccast_test_rotate_convert(flip_rotate, seting_rotate);
    }

    rotate_info->rotate_angle = rotation_angle;
    rotate_info->flip_mode = flip_mode;

    return 0;
}

static void hctest_get_mira_picture_area(av_area_t *src_rect)
{
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];
    int fd = open("/dev/dis" , O_RDWR);
    if(fd < 0)
        return;

    dis_screen_info_t picture_info = { 0 };

    if (HCCAST_TEST_DIS_TYPE_HD == cast_state->dis_type)
        picture_info.distype = DIS_TYPE_HD;
    else
        picture_info.distype = DIS_TYPE_UHD;
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

static int hctest_mira_get_current_pic_info(struct dis_display_info *mpinfo)
{
    int fd = -1;
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];

    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0)
    {
        return -1;
    }
    if (HCCAST_TEST_DIS_TYPE_HD == cast_state->dis_type)
        mpinfo->distype = DIS_TYPE_HD;
    else
        mpinfo->distype = DIS_TYPE_UHD;

    if (HCCAST_TEST_DIS_LAYER_MAIN == cast_state->dis_layer)
        mpinfo->info.layer = DIS_PIC_LAYER_MAIN;
    else
        mpinfo->info.layer = DIS_PIC_LAYER_AUX;

    ioctl(fd , DIS_GET_DISPLAY_INFO , (uint32_t)mpinfo);
    close(fd);
    return 0;
}

static int hctest_mira_get_video_info(int *width, int *heigth)
{
    struct dis_display_info mpinfo = {0};

    hctest_mira_get_current_pic_info(&mpinfo);

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

    return 0;
}

static void hctest_mira_reset_aspect_mode(void)
{
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];    
    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT }, { 0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT }, DIS_SCALE_ACTIVE_IMMEDIATELY};

    hctest_mira_set_dis_zoom(&mira_zoom_info);
    //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
    hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
}


static int hctest_mira_screen_detect_handle(int is_v_screen)
{
    int width_ori = HCCAST_TEST_SCREEN_WIDTH;       //MIRACAST CAST SEND
    int flip_rotate = 0;
    int flip_mirror = 0;
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];    

    //int full_vscreen = projector_get_some_sys_param(P_MIRROR_FULL_VSCREEN);
    int full_vscreen = cast_state->mirror_full_vscreen;

    hccast_mira_zoom_info_t mira_zoom_info = {{ 0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT }, { 0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT }, DIS_SCALE_ACTIVE_IMMEDIATELY};
    int vscreen_auto_rotation = cast_state->mirror_vscreen_auto_rotation;
    hctest_get_flip_info(cast_state, &flip_rotate, &flip_mirror);

    if (g_mira_enable_vrotation)    
    {
        if (is_v_screen)
        {
            printf("V_SCR\n");
            g_mira_v_screen = 1;
            
            if (!full_vscreen)
            {
                hctest_get_mira_picture_area(&g_mira_picture_info);
                hctest_mira_get_video_info(&g_mira_pic_vdec_w,  &g_mira_pic_vdec_h);
                //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_VERTICALCUT, DIS_SCALE_ACTIVE_NEXTFRAME);
                hccast_test_set_aspect_mode(cast_state, DIS_VERTICALCUT, DIS_SCALE_ACTIVE_NEXTFRAME);
            }
            else
            {
                hctest_get_mira_picture_area(&g_mira_picture_info);

                if (hctest_mira_get_video_info(&g_mira_pic_vdec_w,  &g_mira_pic_vdec_h) < 0)
                {
                    printf("get video info fail.\n");  
                    return 0;
                }
                else
                {
                    width_ori = g_mira_pic_vdec_w;
                }

                if (vscreen_auto_rotation)
                {
                    //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_NEXTFRAME);
                    hccast_test_set_aspect_mode(cast_state, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_NEXTFRAME);
                    if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                    {
                        mira_zoom_info.src_rect.x = 0;
                        mira_zoom_info.src_rect.y = (g_mira_picture_info.x * HCCAST_TEST_SCREEN_HEIGHT) / width_ori;
                        mira_zoom_info.src_rect.w = HCCAST_TEST_SCREEN_WIDTH;
                        mira_zoom_info.src_rect.h = HCCAST_TEST_SCREEN_HEIGHT - 2 * mira_zoom_info.src_rect.y;
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                        hctest_mira_set_dis_zoom(&mira_zoom_info);
                    }
                    else
                    {
                        mira_zoom_info.src_rect.x = (g_mira_picture_info.x * HCCAST_TEST_SCREEN_WIDTH) / width_ori;
                        mira_zoom_info.src_rect.y = 0;
                        mira_zoom_info.src_rect.w = HCCAST_TEST_SCREEN_WIDTH - 2 * mira_zoom_info.src_rect.x;
                        mira_zoom_info.src_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                        hctest_mira_set_dis_zoom(&mira_zoom_info);
                    }
                }
                else
                {
                    //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    hccast_test_set_aspect_mode(cast_state, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);

                    if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                    {
                        mira_zoom_info.src_rect.x = 0;
                        mira_zoom_info.src_rect.y = (g_mira_picture_info.x * HCCAST_TEST_SCREEN_HEIGHT) / width_ori;
                        mira_zoom_info.src_rect.w = HCCAST_TEST_SCREEN_WIDTH;
                        mira_zoom_info.src_rect.h = HCCAST_TEST_SCREEN_HEIGHT - 2 * mira_zoom_info.src_rect.y;
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                        hctest_mira_set_dis_zoom(&mira_zoom_info);
                    }
                    else
                    {
                        mira_zoom_info.src_rect.x = (g_mira_picture_info.x * HCCAST_TEST_SCREEN_WIDTH) / width_ori;
                        mira_zoom_info.src_rect.y = 0;
                        mira_zoom_info.src_rect.w = HCCAST_TEST_SCREEN_WIDTH - 2 * mira_zoom_info.src_rect.x;
                        mira_zoom_info.src_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_IMMEDIATELY;
                        hctest_mira_set_dis_zoom(&mira_zoom_info);
                    }
                }
            }
        }
        else
        {
            if (g_mira_enable_vrotation)
            {
                printf("H_SCR\n");
                g_mira_v_screen = 0;

                if (!full_vscreen)
                {
                    if (vscreen_auto_rotation)
                    {
                        //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
                        hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
                    }
                    else
                    {
                        //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                        hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    }        
                }
                else
                {
                    if (vscreen_auto_rotation)
                    {
                        //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
                        hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_NEXTFRAME);
                        mira_zoom_info.dis_active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
                        hctest_mira_set_dis_zoom(&mira_zoom_info);
                    }
                    else
                    {
                        hctest_mira_reset_aspect_mode();
                    }
                }
            }
        }
    }

    return 0;
}

static int hccast_test_mira_callback_func(hccast_mira_event_e event, void* in, void* out)
{
    static int vdec_first_show = 0;
    static int ui_logo_close = 0;
    
    //printf("[%s] event: %d\n", __func__, event);
	
    switch (event)
    {
        case HCCAST_MIRA_GET_DEVICE_NAME:
        {
            if (in)
            {
                sprintf((char *)in, "%s_mira", get_cast_name());
            }
            break;
        }
        case HCCAST_MIRA_GET_DEVICE_PARAM:
        {
            hccast_wifi_p2p_param_t *p2p_param = (hccast_wifi_p2p_param_t*)in;
            sprintf((char *)p2p_param->device_name, "%s_mira", get_cast_name());
            strcpy(p2p_param->p2p_ifname, "p2p0");
            p2p_param->listen_channel = 0;
            break;
        }
        case HCCAST_MIRA_SSID_DONE:
        {
            //printf("[%s]HCCAST_MIRA_SSID_DONE\n",__func__);
            break;
        }
        case HCCAST_MIRA_GET_CUR_WIFI_INFO:
        {
            char ssid_str[64] = "can not get ssid!";
            int ret = hccast_wifi_mgr_get_connect_ssid(ssid_str, sizeof(ssid_str));
            printf("[%s]HCCAST_MIRA_GET_CUR_WIFI_INFO, ssid(ret:%d): %s\n",__func__, ret, ssid_str);
            snprintf(((hccast_wifi_ap_info_t*)in)->ssid, WIFI_MAX_SSID_LEN, "%s", ssid_str);
            break;
        }
        case HCCAST_MIRA_CONNECT:
        {
            //miracast connect start
            printf("[%s]HCCAST_MIRA_CONNECT\n",__func__);
            break;
        }
        case HCCAST_MIRA_CONNECTED:
        {
            //miracast connect success
            printf("[%s]HCCAST_MIRA_CONNECTED\n",__func__);
            break;
        }
        case HCCAST_MIRA_DISCONNECT:
        {
            //miracast disconnect
            printf("[%s]HCCAST_MIRA_DISCONNECT\n",__func__);
            break;
        }

        case HCCAST_MIRA_START_DISP:
        {
            //miracast start
            printf("[%s] HCCAST_MIRA_START_DISP [%d:%d]\n",__func__, vdec_first_show, ui_logo_close);
            hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];

            int rotate = cast_state->rotate_mode;
            if ((rotate == HCTEST_ROTATE_90) || (rotate == HCTEST_ROTATE_270))
            {
                hctest_mira_vscreen_detect_enable(1);
                g_mira_enable_vrotation = 1;
            }
            else
            {
                hctest_mira_vscreen_detect_enable(0);
                g_mira_enable_vrotation = 0;
            }

            g_mira_full_vscreen = cast_state->mirror_full_vscreen;
            //cast_api_check_dis_mode();

            hccast_test_set_aspect_mode(cast_state, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);

            break;
        }
        case HCCAST_MIRA_START_FIRST_FRAME_DISP:
        {
            printf("[%s] HCCAST_MIRA_START_FIRST_FRAME_DISP [%d:%d]\n",__func__, vdec_first_show, ui_logo_close);
            break;
        }        
        case HCCAST_MIRA_STOP_DISP:
        {
            //miracast stop
            printf("[%s] HCCAST_MIRA_STOP_DISP [%d:%d]\n",__func__, vdec_first_show, ui_logo_close);
            g_mira_enable_vrotation = 0;
            g_mira_v_screen = 0;
            hctest_mira_reset_aspect_mode();
            hctest_mira_vscreen_detect_enable(0);

            break;
        }
        case HCCAST_MIRA_RESET:
        {
            printf("[%s] HCCAST_MIRA_RESET\n", __func__);
            bool *valid = (bool*) out;
            if (valid)
            {
                //false: hccast middle layer would stop miracast and restart miracast, reconnet wifi if need.
                //true: stop miracast and restart miracast, reconnet wifi if need should be done by application
                *valid = true;

                if (true == *valid){
                    //hccast_mira_service_stop();
                }
            }
            break;
        }
        case HCCAST_MIRA_GET_MIRROR_ROTATION_INFO:
        {    
            //cast_api_mira_get_rotation_info((hccast_mira_rotation_t*)out);
            
            hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];
            if (cast_state->auto_rotate_disable){
                int rotation_angle;
                int rotate_type;
                int flip_type;
                hccast_mira_rotation_t *rotate_info = (hccast_mira_rotation_t*)out;

                hctest_get_flip_info(cast_state, &rotate_type, &flip_type);

                //overlay the adding rotation/flip.
                rotation_angle = hccast_test_rotate_convert(rotate_type, cast_state->rotate_mode);
                rotate_info->rotate_angle = rotation_angle;
                rotate_info->flip_mode = hccast_test_flip_convert(cast_state->dis_type, flip_type, cast_state->flip_mode);
            } else {
                hctest_mira_auto_get_rotation_info((hccast_mira_rotation_t*)out);    
            }
            
            break;
        }
        case HCCAST_MIRA_MIRROR_SCREEN_DETECT_NOTIFY:
        {
            printf("[%s] HCCAST_MIRA_MIRROR_SCREEN_DETECT_NOTIFY\n", __func__);
            int v_screen;
            if (in)
            {
                v_screen = *(unsigned long*)in;
                hctest_mira_screen_detect_handle(v_screen);   
            }    
            break;
        }

        case HCCAST_MIRA_GOT_IP:
            printf("[%s] HCCAST_MIRA_GOT_IP\n", __func__);
            break;

        case HCCAST_MIRA_GET_CONTINUE_ON_ERROR:
            printf("[%s] HCCAST_MIRA_GOT_IP\n", __func__);
            if (out)
            {
                //*(uint8_t *)out = projector_get_some_sys_param(P_MIRA_CONTINUE_ON_ERROR);
            }
            break;

        case HCCAST_MIRA_GET_PREVIEW_INFO:
        {
            printf("[%s] HCCAST_MIRA_GET_PREVIEW_INFO\n", __func__);

            hccast_mira_preview_info_t preview_info = {0};
            hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];
            if (memcmp(&cast_state->src_rect, &cast_state->dst_rect, sizeof(hccast_test_rect_t))){
                preview_info.preview_en = 1;
            } else {
                preview_info.preview_en = 0;

                if (HCCAST_TEST_SCREEN_WIDTH == cast_state->dst_rect.w && 
                    HCCAST_TEST_SCREEN_HEIGHT == cast_state->dst_rect.h){
                    //revert to full screen, use DE zoom
                    hccast_test_display_zoom(cast_state->dis_type, cast_state->dis_layer, 
                        &cast_state->src_rect, &cast_state->dst_rect, DIS_SCALE_ACTIVE_IMMEDIATELY);

                    usleep(100*1000);
                }
            }

            preview_info.src_rect.x = cast_state->src_rect.x;
            preview_info.src_rect.y = cast_state->src_rect.y;
            preview_info.src_rect.w = cast_state->src_rect.w;
            preview_info.src_rect.h = cast_state->src_rect.h;
            preview_info.dst_rect.x = cast_state->dst_rect.x;
            preview_info.dst_rect.y = cast_state->dst_rect.y;
            preview_info.dst_rect.w = cast_state->dst_rect.w;
            preview_info.dst_rect.h = cast_state->dst_rect.h;
            memcpy(in, &preview_info, sizeof(hccast_mira_preview_info_t));  
            break;
        }

        case HCCAST_MIRA_GET_VIDEO_CONFIG:
        {
            printf("[%s] HCCAST_MIRA_GET_VIDEO_CONFIG\n", __func__);

            hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST];
            
            hccast_com_video_config_t *video_config = (hccast_com_video_config_t*)in;
            video_config->video_pbp_mode = cast_state->pbp_on ? 
            					HCCAST_COM_VIDEO_PBP_2P_ON : HCCAST_COM_VIDEO_PBP_OFF;
            video_config->video_dis_type = cast_state->dis_type ? 
            					HCCAST_COM_VIDEO_DIS_TYPE_UHD : HCCAST_COM_VIDEO_DIS_TYPE_HD;
            video_config->video_dis_layer = cast_state->dis_layer ? 
            					HCCAST_TEST_DIS_LAYER_AUXP : HCCAST_COM_VIDEO_DIS_LAYER_MAIN;

            printf("miracast: pbp_mode:%d, dis_type:%d, dis_layer:%d\n",  
            	video_config->video_pbp_mode, video_config->video_dis_type, video_config->video_dis_layer);

            break;
        }
        default:
            break;

    }
    return 0;
}
#endif

#ifdef AIRCAST_SUPPORT
static int g_air_vd_dis_mode = DIS_PILLBOX;

static void hctest_air_dis_mode_set(int rotate, unsigned int width, unsigned int height)
{
    int full_vscreen;
    int dis_scale_active;
    int dis_mode;
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_AIRCAST];
    
    full_vscreen = cast_state->mirror_full_vscreen;
    if (!rotate)
    {
        dis_mode = DIS_PILLBOX;
        dis_scale_active = DIS_SCALE_ACTIVE_IMMEDIATELY;
    }
    else
    {
        if ((rotate == HCTEST_ROTATE_270) || (rotate == HCTEST_ROTATE_90))
        {
            if ((height > width) && full_vscreen)
            {
                dis_mode = DIS_NORMAL_SCALE;
                dis_scale_active = DIS_SCALE_ACTIVE_IMMEDIATELY;
            }
            else
            {
                dis_mode = DIS_PILLBOX;
                dis_scale_active = DIS_SCALE_ACTIVE_IMMEDIATELY;
            }
        }
        else
        {
            dis_mode = DIS_PILLBOX;
            dis_scale_active = DIS_SCALE_ACTIVE_IMMEDIATELY;
        }
    }

    if (dis_mode != g_air_vd_dis_mode)
    {
        //cast_api_set_aspect_mode(DIS_TV_16_9, dis_mode, dis_scale_active);
        hccast_test_set_aspect_mode(cast_state, dis_mode, dis_scale_active);
        g_air_vd_dis_mode = dis_mode;
    }

}

static int hctest_air_get_rotation_info(hccast_air_rotation_t *rotate_info)
{
    int seting_rotate;
    int rotation_angle = HCTEST_ROTATE_0;
    int flip_mode;
    int flip_rotate;
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_AIRCAST];
    
    if (!rotate_info)
    {
        return -1;
    }

    //seting_rotate = projector_get_some_sys_param(P_MIRROR_ROTATION);
    seting_rotate = cast_state->rotate_mode;
    if((seting_rotate == HCTEST_ROTATE_90) || (seting_rotate == HCTEST_ROTATE_270))
    {
        if (rotate_info->src_w > rotate_info->src_h) 
        {
            if (cast_state->mirror_vscreen_auto_rotation)
            {
                seting_rotate = HCTEST_ROTATE_0;
            }  
        }
    }

    hctest_air_dis_mode_set(seting_rotate, rotate_info->src_w, rotate_info->src_h);
    
    //overlay the adding rotation/flip.
    hctest_get_flip_info(cast_state, &flip_rotate, &flip_mode);
    rotation_angle = hccast_test_rotate_convert(flip_rotate, seting_rotate);
    rotate_info->rotate_angle = rotation_angle;
    rotate_info->flip_mode = hccast_test_flip_convert(cast_state->dis_type, flip_mode, cast_state->flip_mode);
    
    return 0;
}

static int hccast_test_air_callback_event(hccast_air_event_e event, void* in, void* out)
{
    (void)out;
    int ret = 0;
    //int mode_set, 
    int air_mode;

    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_AIRCAST];
    switch (event)
    {
        case HCCAST_AIR_GET_SERVICE_NAME:
            //sprintf((char *)in, "%s_itv", (char*)projector_get_some_sys_param(P_DEVICE_NAME));
            sprintf((char *)in, "%s_air", (char*)get_cast_name());
            break;
        case HCCAST_AIR_GET_NETWORK_DEVICE:
            sprintf((char *)in, "%s", "wlan0");
            break;
        case HCCAST_AIR_GET_MIRROR_MODE:
            //mode_set = projector_get_some_sys_param(P_AIRCAST_MODE);
           /*
            mode_set = 2;
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
                if (hccast_wifi_mgr_get_hostap_status())
                {
                    air_mode = HCCAST_AIR_MODE_MIRROR_ONLY;
                }
                else
                {
                    air_mode = HCCAST_AIR_MODE_MIRROR_STREAM;
                }
            }
            *(int*)in = air_mode;
           */
            air_mode = HCCAST_AIR_MODE_MIRROR_STREAM;
            *(int*)in = air_mode;
            break;
		case HCCAST_AIR_GET_NETWORK_STATUS:
            printf("[%s],line:%d. ****** HCCAST_AIR_GET_NETWORK_STATUS *********\n",__func__, __LINE__);
            //*(int*)in = hccast_wifi_mgr_get_hostap_status();
            *(int*)in = 0; //0, station mode; 1, host mode. host mode only support mirror(not support stream)
            break;
        case HCCAST_AIR_MIRROR_START:
            printf("[%s],line:%d. HCCAST_AIR_MIRROR_START\n",__func__, __LINE__);
            hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
            g_air_vd_dis_mode = DIS_PILLBOX;
            break;
        case HCCAST_AIR_MIRROR_STOP:
            printf("[%s],line:%d. HCCAST_AIR_MIRROR_STOP\n",__func__, __LINE__);
            break;
        case HCCAST_AIR_AUDIO_START:
            break;
        case HCCAST_AIR_AUDIO_STOP:
            break;
        case HCCAST_AIR_INVALID_CERT:
            printf("[%s],line:%d. HCCAST_AIR_INVALID_CERT\n",__func__, __LINE__);
            break;
        case HCCAST_AIR_GET_4K_MODE:
        /*
            if(projector_get_some_sys_param(P_DE_TV_SYS) < TV_LINE_4096X2160_30)
            {
                *(int*)in = 0;
                printf("[%s] NOT 4K MODE, tv_sys:%d\n",__func__,projector_get_some_sys_param(P_DE_TV_SYS));

            }
            else
            {
                *(int*)in = 1;
                printf("[%s] NOW IS 4K MODE, tv_sys:%d\n",__func__,projector_get_some_sys_param(P_DE_TV_SYS));
            }
		*/
			*(int*)in = 0;
            break;
        case HCCAST_AIR_HOSTAP_MODE_SKIP_URL:
            printf("[%s]HCCAST_AIR_HOSTAP_MODE_SKIP_URL\n",__func__);
            break;	
		case HCCAST_AIR_BAD_NETWORK:
			printf("[%s]HCCAST_AIR_BAD_NETWORK\n",__func__);
			break;
        case HCCAST_AIR_URL_ENABLE_SET_DEFAULT_VOL:
            *(int*)in = 1; //0, defaut set volume 0; 1, defaut set volume 80
            break;
        case HCCAST_AIR_SET_AUDIO_VOL:
            hccast_test_set_vol((int)in);
            printf("%s set vol:%d\n", __func__, (int)in);
            break;
        case HCCAST_AIR_GET_MIRROR_ROTATION_INFO:
        {    
            hctest_air_get_rotation_info((hccast_air_rotation_t*)in);
            break;
        }
        case HCCAST_AIR_GET_ABDISCONNECT_STOP_PLAY_EN:
            if (in)
            {
                *(int*)in = 0;
            }
            break;
        case HCCAST_AIR_GET_AIRP2P_PIN:
            sprintf((char *)in, "%s", "1234");
            break;
        case HCCAST_AIR_GET_MIRROR_QUICK_MODE_NUM:
            //if (network_get_airp2p_state())
            {
                *(int*)in = 7;//set to support the max level.
            }
            break;

        case HCCAST_AIR_GET_PREVIEW_INFO:
        {
            hccast_air_deview_t preview_info = {{0,0,HCCAST_TEST_SCREEN_WIDTH,HCCAST_TEST_SCREEN_HEIGHT}, \
				{0,0,HCCAST_TEST_SCREEN_WIDTH,HCCAST_TEST_SCREEN_HEIGHT}, 0};
            hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_AIRCAST];
            if (memcmp(&cast_state->src_rect, &cast_state->dst_rect, sizeof(hccast_test_rect_t))){
                preview_info.preview_en = 1;
            } else {
                preview_info.preview_en = 0;

                if (HCCAST_TEST_SCREEN_WIDTH == cast_state->dst_rect.w && 
                    HCCAST_TEST_SCREEN_HEIGHT == cast_state->dst_rect.h){
                    //revert to full screen, use DE zoom
                    hccast_test_display_zoom(cast_state->dis_type, cast_state->dis_layer, 
                        &cast_state->src_rect, &cast_state->dst_rect, DIS_SCALE_ACTIVE_IMMEDIATELY);

                    usleep(100*1000);
                }

            }

            preview_info.src_rect.x = cast_state->src_rect.x;
            preview_info.src_rect.y = cast_state->src_rect.y;
            preview_info.src_rect.w = cast_state->src_rect.w;
            preview_info.src_rect.h = cast_state->src_rect.h;
            preview_info.dst_rect.x = cast_state->dst_rect.x;
            preview_info.dst_rect.y = cast_state->dst_rect.y;
            preview_info.dst_rect.w = cast_state->dst_rect.w;
            preview_info.dst_rect.h = cast_state->dst_rect.h;
            memcpy(in, &preview_info, sizeof(hccast_air_deview_t));
            break;              
        }

        case HCCAST_AIR_GET_VIDEO_CONFIG:
        {
            printf("[%s] HCCAST_AIR_GET_VIDEO_CONFIG\n", __func__);         

            hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_AIRCAST];
            
            hccast_com_video_config_t *video_config = (hccast_com_video_config_t*)in;
            video_config->video_pbp_mode = cast_state->pbp_on ? 
                                HCCAST_COM_VIDEO_PBP_2P_ON : HCCAST_COM_VIDEO_PBP_OFF;
            video_config->video_dis_type = cast_state->dis_type ? 
                                HCCAST_COM_VIDEO_DIS_TYPE_UHD : HCCAST_COM_VIDEO_DIS_TYPE_HD;
            video_config->video_dis_layer = cast_state->dis_layer ? 
                                HCCAST_TEST_DIS_LAYER_AUXP : HCCAST_COM_VIDEO_DIS_LAYER_MAIN;

            printf("aircast: pbp_mode:%d, dis_type:%d, dis_layer:%d\n",  
            	video_config->video_pbp_mode, video_config->video_dis_type, video_config->video_dis_layer);

            break;            
        }

        default:
            break;
    }

    return ret;
}
#endif


#ifdef DLNA_SUPPORT
static int hccast_test_dlna_callback_func(hccast_dlna_event_e event, void* in, void* out)
{
    (void)out;
    static char service_name[DLNA_SERVICE_NAME_LEN] = "hccast_dlna";
    static char service_ifname[DLNA_SERVICE_NAME_LEN] = DLNA_BIND_IFNAME;

    switch (event)
    {
        case HCCAST_DLNA_GET_DEVICE_NAME:
        {
            printf("[%s]HCCAST_DLNA_GET_DEVICE_NAME\n",__func__);
            if (in)
                sprintf((char *)in, "%s_dlna", get_cast_name());
            break;
        }
        case HCCAST_DLNA_GET_DEVICE_PARAM:
        {
            hccast_dlna_param *dlna = (hccast_dlna_param *)in;
            if (dlna)
            {
                sprintf(service_name, "%s_dlna", get_cast_name());

                /*
                if (WIFI_MODE_AP == app_wifi_get_work_mode())
                {
                    hccast_wifi_mgr_get_ap_ifname(service_ifname, sizeof(service_ifname));
                }
                else if (WIFI_MODE_STATION == app_wifi_get_work_mode())
                {
                    hccast_wifi_mgr_get_sta_ifname(service_ifname, sizeof(service_ifname));
                }
				*/
                dlna->svrname = service_name;
                dlna->svrport = DLNA_UPNP_PORT; // need > 49152
                dlna->ifname = service_ifname;
            }
            break;
        }
        case HCCAST_DLNA_GET_HOSTAP_STATE:
            //*(int*)in = hccast_wifi_mgr_get_hostap_status();
            break;
        case HCCAST_DLNA_HOSTAP_MODE_SKIP_URL:
            break;
        case HCCAST_DLNA_GET_SAVE_AUDIO_VOL:
            *(int*)in = 80;
            break;
        default:
            break;
    }

    return 0;
}

static void hccast_media_callback_func(hccast_media_event_e msg_type, void* param)
{
    hccast_test_state_t *cast_state = &m_wifi_cast_state[HCCAST_TEST_TYPE_DLNA];

    switch (msg_type)
    {
        case HCCAST_MEDIA_EVENT_PARSE_END:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PARSE_END\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_PLAYING:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PLAYING\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_PAUSE:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PAUSE\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_BUFFERING:
            //printf("[%s] %d   HCCAST_MEDIA_EVENT_BUFFERING, %d%%\n", __func__, __LINE__, (int)param);
            break;
        case HCCAST_MEDIA_EVENT_PLAYBACK_END:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_PLAYBACK_END\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_VIDEO_DECODER_ERROR:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_VIDEO_DECODER_ERROR\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_AUDIO_DECODER_ERROR:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_AUDIO_DECODER_ERROR\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_VIDEO_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_VIDEO_NOT_SUPPORT\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_AUDIO_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_AUDIO_NOT_SUPPORT\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_NOT_SUPPORT:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_NOT_SUPPORT\n", __func__, __LINE__);
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_DLNA:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_DLNA, media_type: %d\n", __func__, __LINE__,(hccast_media_type_e)param);
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_DIAL:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_DIAL, media_type: %d\n", __func__, __LINE__, (hccast_media_type_e)param);
            break;
        case HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_FROM_AIRCAST, media_type: %d\n", __func__, __LINE__,(hccast_media_type_e)param);
            break;
        case HCCAST_MEDIA_EVENT_URL_SEEK:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_URL_SEEK  position: %ds\n", __func__, __LINE__, (int)param / 1000);
            break;
        case HCCAST_MEDIA_EVENT_SET_VOLUME:
            printf("[%s] %d   HCCAST_MEDIA_EVENT_SET_VOLUME  volume: %d\n", __func__, __LINE__, (int)param);
            hccast_test_set_vol((int)param);
            break;
        case HCCAST_MEDIA_EVENT_GET_MIRROR_ROTATION:
            //*(int*)param = projector_get_some_sys_param(P_MIRROR_ROTATION);           
            *(int*)param = cast_state->rotate_mode;
            break;  
        case HCCAST_MEDIA_EVENT_GET_FLIP_MODE:
        {
        	/*
            int flip_mode;
            int rotate;     
            int flip;
            api_get_flip_info(&rotate, &flip);
            flip_mode = (rotate & 0xffff) << 16 | (flip & 0xffff);
            *(int*)param = flip_mode;
            */
           
            int flip_mode;
            int rotate;     
            int flip;

            int rotate_type;
            int flip_type;

            hctest_get_flip_info(cast_state, &rotate_type, &flip_type);

            //overlay the adding rotation/flip.
            rotate = hccast_test_rotate_convert(rotate_type, cast_state->rotate_mode);
            flip = hccast_test_flip_convert(cast_state->dis_type, flip_type, cast_state->flip_mode);

            flip_mode = (rotate & 0xffff) << 16 | (flip & 0xffff);
            *(int*)param = flip_mode;

            break;
        }
        case HCCAST_MEDIA_EVENT_URL_START_PLAY:
        {   
            break;
        }
        case HCCAST_MEDIA_EVENT_GET_ZOOM_INFO:
        {   
            printf("[%s] HCCAST_MEDIA_EVENT_GET_ZOOM_INFO\n", __func__);            
            hccast_media_zoom_info_t *zoom_info = (hccast_media_zoom_info_t*)param;

            if (memcmp(&cast_state->src_rect, &cast_state->dst_rect, sizeof(hccast_test_rect_t))){
                zoom_info->enable = 1;
            }
            else {
                zoom_info->enable = 0;

                if (HCCAST_TEST_SCREEN_WIDTH == cast_state->dst_rect.w && 
                    HCCAST_TEST_SCREEN_HEIGHT == cast_state->dst_rect.h){
                    //revert to full screen, use DE zoom
                    hccast_test_display_zoom(cast_state->dis_type, cast_state->dis_layer, 
                        &cast_state->src_rect, &cast_state->dst_rect, DIS_SCALE_ACTIVE_IMMEDIATELY);

                    usleep(100*1000);
                }
            }

            zoom_info->src_rect.x = cast_state->src_rect.x;
            zoom_info->src_rect.y = cast_state->src_rect.y;
            zoom_info->src_rect.w = cast_state->src_rect.w;
            zoom_info->src_rect.h = cast_state->src_rect.h;
            zoom_info->dst_rect.x = cast_state->dst_rect.x;
            zoom_info->dst_rect.y = cast_state->dst_rect.y;
            zoom_info->dst_rect.w = cast_state->dst_rect.w;
            zoom_info->dst_rect.h = cast_state->dst_rect.h;

            break;
        }

        case HCCAST_MEDIA_EVENT_GET_VIDEO_CONFIG:
        {   
            printf("[%s] HCCAST_MEDIA_EVENT_GET_VIDEO_CONFIG\n", __func__);
            hccast_com_video_config_t *video_config = (hccast_com_video_config_t*)param;

            video_config->video_pbp_mode = cast_state->pbp_on ? 
            					HCCAST_COM_VIDEO_PBP_2P_ON : HCCAST_COM_VIDEO_PBP_OFF;
            video_config->video_dis_type = cast_state->dis_type ? 
            					HCCAST_COM_VIDEO_DIS_TYPE_UHD : HCCAST_COM_VIDEO_DIS_TYPE_HD;
            video_config->video_dis_layer = cast_state->dis_layer ? 
            					HCCAST_TEST_DIS_LAYER_AUXP : HCCAST_COM_VIDEO_DIS_LAYER_MAIN;

            printf("DLNA media: pbp_mode:%d, dis_type:%d, dis_layer:%d\n",  
            	video_config->video_pbp_mode, video_config->video_dis_type, video_config->video_dis_layer);

            break;
        }        
        default:
            break;
    }

}

#endif

int cast_wifi_init(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
	static bool cast_wifi_init_flag = false;
    int i;
	if (cast_wifi_init_flag)
		return 0;

    //default set wifi type to 8188FTV, other wise p2p int fail:
    //log: [23-15:55:53:473][hccast]hc_wifi_service_p2p_init failed!
    hccast_wifi_mgr_set_wifi_model(HCCAST_NET_WIFI_8188FTV);

    memset(m_wifi_cast_state, 0, sizeof(hccast_test_state_t));
    for (i = 0; i < HCCAST_TEST_WIFI_TYPE_MAX; i ++){
        switch (i)
        {
        case HCCAST_TEST_TYPE_MIRACAST:
            sprintf(m_wifi_cast_state[i].name, "miracast");
            break;
        case HCCAST_TEST_TYPE_AIRCAST:
            sprintf(m_wifi_cast_state[i].name, "aircast");
            break;
        case HCCAST_TEST_TYPE_DLNA:
            sprintf(m_wifi_cast_state[i].name, "dlna");
            break;
        default:
            break;
        }

        m_wifi_cast_state[i].rotate_mode = 0;
        m_wifi_cast_state[i].flip_mode = 0;
        m_wifi_cast_state[i].tv_mode = DIS_TV_16_9;
        m_wifi_cast_state[i].project_mode = HCTEST_PROJECT_REAR;

        m_wifi_cast_state[i].mirror_vscreen_auto_rotation = 1;
        m_wifi_cast_state[i].mirror_full_vscreen = 1;
        //m_wifi_cast_state[i].um_full_screen = 1;
        m_wifi_cast_state[i].audio_path = AUDSINK_SND_DEVBIT_I2SO;
        

        m_wifi_cast_state[i].pbp_on = 0;
        m_wifi_cast_state[i].dis_type = HCCAST_TEST_DIS_TYPE_HD;
        m_wifi_cast_state[i].dis_layer = HCCAST_TEST_DIS_LAYER_MAIN;
        m_wifi_cast_state[i].src_rect.x = 0;
        m_wifi_cast_state[i].src_rect.y = 0;
        m_wifi_cast_state[i].src_rect.w = HCCAST_TEST_SCREEN_WIDTH;
        m_wifi_cast_state[i].src_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
        m_wifi_cast_state[i].dst_rect.x = 0;
        m_wifi_cast_state[i].dst_rect.y = 0;
        m_wifi_cast_state[i].dst_rect.w = HCCAST_TEST_SCREEN_WIDTH;
        m_wifi_cast_state[i].dst_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
        m_wifi_cast_state[i].state = 0;

    }


#ifdef MIRACAST_SUPPORT
    hccast_mira_service_init(hccast_test_mira_callback_func);
    hccast_mira_service_set_resolution(HCCAST_MIRA_RES_1080P30);
#endif

#ifdef AIRCAST_SUPPORT    
    hccast_air_service_init(hccast_test_air_callback_event);
    hccast_air_set_resolution(1280, 720, 30);
#endif    

#ifdef DLNA_SUPPORT
    hccast_dlna_service_init(hccast_test_dlna_callback_func);
    hccast_media_init(hccast_media_callback_func);
  #ifdef DIAL_SUPPORT
    //hccast_dial_service_init(hccast_dial_callback_func);
  #endif
#endif

    hccast_scene_init();
    hctest_screen_rotate_info();

	cast_wifi_init_flag = true;
	return 0;
}

static int cast_wifi_start(int argc, char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
	int cast_type = -1;
	int dis_type = -1;
	int dis_layer = -1;
	int pbp_on = -1;
    int rotate = -1;
    int h_flip = -1;
    int project_mode = -1;
    int auto_rotate_disable = -1;
    int mirror_full_vscreen = -1;
    int mirror_vscreen_auto_rotation = -1;
    int audio_path = -1;
    int audio_dec_enable = -1;
    int audio_disable = -1;

    if (true == m_cast_wifi_start)
        return 0;

    cast_wifi_init(0, NULL);

	while ((opt = getopt(argc, &argv[0], "s:p:d:l:r:m:f:k:j:a:o:g:h:")) != EOF) {
		switch (opt) {
		case 's':
			cast_type = atoi(optarg);
			printf("%s(), cast type: %d\n", __func__, (int)cast_type);
            if (cast_type < 0 || cast_type >= HCCAST_TEST_WIFI_TYPE_MAX){
                printf("%s(), line: %d. args err: cast_type %d\n", 
                    __func__, __LINE__, (int)cast_type);
                return -1;
            }

			break;
		case 'p':
			pbp_on = atoi(optarg);
            pbp_on = pbp_on ? 1 : 0;
			printf("%s(), pbp on: %d\n", __func__, pbp_on);
			break;
		case 'd':
			dis_type = atoi(optarg);
            dis_type = dis_type ? 1 : 0;
			printf("%s(), dis type: %d\n", __func__, (int)dis_type);
			break;
        case 'l':
            dis_layer = atoi(optarg);
            dis_layer = dis_layer ? 1 : 0;
            printf("%s(), dis_layer: %d\n", __func__, (int)dis_layer);
            break;
        case 'r':
            rotate = atoi(optarg);
            printf("%s(), rotate: %d\n", __func__, (int)rotate);
            break;
        case 'm':
            h_flip = atoi(optarg);
            h_flip = h_flip ? 1 : 0;
            printf("%s(), h_flip: %d\n", __func__, (int)h_flip);
            break;
        case 'f':
        {    
            project_mode = atoi(optarg);
            if (project_mode < 0 || project_mode > 3){
                printf("%s(), project_mode: %d err, must be in 0 - 3\n", __func__, (int)project_mode);
                return -1;
            }
            break;
        }
        case 'k':
            mirror_full_vscreen = atoi(optarg);
            mirror_full_vscreen = mirror_full_vscreen ? 1 : 0;
            break;
        case 'j':
            mirror_vscreen_auto_rotation = atoi(optarg);
            mirror_vscreen_auto_rotation = mirror_vscreen_auto_rotation ? 1 : 0;
            break;
        case 'a':
            auto_rotate_disable = atoi(optarg);
            auto_rotate_disable = auto_rotate_disable ? 1 : 0;
            break;
        case 'o':
            audio_path = atoi(optarg);
            break;
        case 'g':
            audio_dec_enable = atoi(optarg);
            audio_dec_enable = audio_dec_enable ? 1 : 0;
            break;
        case 'h':
            audio_disable = atoi(optarg);
            audio_disable = audio_disable ? 1 : 0;
            break;
		default:
			break;
		}
	}
	if (dis_type >= HCCAST_TEST_DIS_TYPE_MAX ||
		dis_layer >= HCCAST_TEST_DIS_LAYER_MAX
		){
		printf("%s(), args err: dis_type:%d, dis_layer:%d\n", \
			__func__, (int)dis_type, (int)dis_layer);
		return -1;
	}

    if (hccast_get_current_scene() != HCCAST_SCENE_NONE)
    {
        hccast_scene_switch(HCCAST_SCENE_NONE);
    }

    if (cast_type != -1){
        m_wifi_cast_state[cast_type].state = 1;
        if (-1 != pbp_on)
            m_wifi_cast_state[cast_type].pbp_on = pbp_on;
        if (-1 != dis_type)
            m_wifi_cast_state[cast_type].dis_type = dis_type;
        if (-1 != dis_layer)
            m_wifi_cast_state[cast_type].dis_layer = dis_layer;
        if (-1 != rotate)
            m_wifi_cast_state[cast_type].rotate_mode = rotate;
        if (-1 != h_flip)
            m_wifi_cast_state[cast_type].flip_mode = h_flip;
        if (-1 != project_mode)
            m_wifi_cast_state[cast_type].project_mode = project_mode;
        if (-1 != mirror_full_vscreen)
            m_wifi_cast_state[cast_type].mirror_full_vscreen = mirror_full_vscreen;
        if (-1 != mirror_vscreen_auto_rotation)
            m_wifi_cast_state[cast_type].mirror_vscreen_auto_rotation = mirror_vscreen_auto_rotation;
        if (-1 != auto_rotate_disable)
            m_wifi_cast_state[cast_type].auto_rotate_disable = auto_rotate_disable;
        if (-1 != audio_path)
            m_wifi_cast_state[cast_type].audio_path = audio_path;
        if (-1 != audio_dec_enable)
            m_wifi_cast_state[cast_type].audio_dec_enable = audio_dec_enable;
        if (-1 != audio_disable)
            m_wifi_cast_state[cast_type].audio_disable = audio_disable;

        hccast_com_audsink_set(m_wifi_cast_state[cast_type].audio_path);        
        hccast_com_force_auddec_set(m_wifi_cast_state[cast_type].audio_dec_enable);    
        hccast_com_audio_disable_set(m_wifi_cast_state[cast_type].audio_disable);
    } else {
        int i;
        for (i = 0; i < HCCAST_TEST_WIFI_TYPE_MAX; i ++){
            m_wifi_cast_state[i].state = 1;
            if (-1 != pbp_on)
                m_wifi_cast_state[i].pbp_on = pbp_on;
            if (-1 != dis_type)
                m_wifi_cast_state[i].dis_type = dis_type;
            if (-1 != dis_layer)
                m_wifi_cast_state[i].dis_layer = dis_layer;
            if (-1 != rotate)
                m_wifi_cast_state[i].rotate_mode = rotate;
            if (-1 != h_flip)
                m_wifi_cast_state[i].flip_mode = h_flip;
            if (-1 != project_mode)
                m_wifi_cast_state[i].project_mode = project_mode;
            if (-1 != mirror_full_vscreen)
                m_wifi_cast_state[i].mirror_full_vscreen = mirror_full_vscreen;
            if (-1 != mirror_vscreen_auto_rotation)
                m_wifi_cast_state[i].mirror_vscreen_auto_rotation = mirror_vscreen_auto_rotation;
            if (-1 != auto_rotate_disable)
                m_wifi_cast_state[i].auto_rotate_disable = auto_rotate_disable;
            if (-1 != audio_path)
                m_wifi_cast_state[i].audio_path = audio_path;
            if (-1 != audio_dec_enable)
                m_wifi_cast_state[i].audio_dec_enable = audio_dec_enable;
            if (-1 != audio_disable)
                m_wifi_cast_state[i].audio_disable = audio_disable;
        }
        hccast_com_audsink_set(m_wifi_cast_state[0].audio_path);        
        hccast_com_force_auddec_set(m_wifi_cast_state[0].audio_dec_enable);        
        hccast_com_audio_disable_set(m_wifi_cast_state[0].audio_disable);

    }

    if (m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST].state)
    {
    #ifdef MIRACAST_SUPPORT   
        hccast_mira_service_start();
    #endif    
    }

    if (m_wifi_cast_state[HCCAST_TEST_TYPE_AIRCAST].state)
    {
    #ifdef AIRCAST_SUPPORT    
        hccast_air_service_stop();
        hccast_air_service_start();
    #endif

    }

    if (m_wifi_cast_state[HCCAST_TEST_TYPE_DLNA].state)
    {
    #ifdef DLNA_SUPPORT
        hccast_dlna_service_stop();
        hccast_dlna_service_start();

      #ifdef DIAL_SUPPORT
        hccast_dial_service_stop();
        hccast_dial_service_start();
      #endif
    #endif
    }
    m_cast_wifi_start = true;

	return 0;
}


static int cast_wifi_rotate(int argc, char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    int cast_type = -1;
    int dis_type = -1;
    int dis_layer = -1;
    int rotate = -1;
    int h_flip = -1;
    int project_mode = -1;
    int auto_rotate_disable = -1;

    while ((opt = getopt(argc, &argv[0], "s:r:m:f:a:")) != EOF) {
        switch (opt) {
        case 's':
            cast_type = atoi(optarg);
            printf("%s(), cast type: %d\n", __func__, (int)cast_type);
            if (cast_type < 0 || cast_type >= HCCAST_TEST_WIFI_TYPE_MAX){
                printf("%s(), line: %d. args err: cast_type %d\n", 
                    __func__, __LINE__, (int)cast_type);
                return -1;
            }

            break;
        case 'r':
            rotate = atoi(optarg);
            printf("%s(), rotate: %d\n", __func__, (int)rotate);
            break;
        case 'm':
            h_flip = atoi(optarg);
            h_flip = h_flip ? 1 : 0;
            printf("%s(), h_flip: %d\n", __func__, (int)h_flip);
            break;
        case 'f':
        {    
            project_mode = atoi(optarg);
            if (project_mode < 0 || project_mode > 3){
                printf("%s(), project_mode: %d err, must be in 0 - 3\n", __func__, (int)project_mode);
                return -1;
            }
            break;
        }
        case 'a':
            auto_rotate_disable = atoi(optarg);
            break;

        default:
            break;
        }
    }

    if (dis_type >= HCCAST_TEST_DIS_TYPE_MAX ||
        dis_layer >= HCCAST_TEST_DIS_LAYER_MAX
        ){
        printf("%s(), args err: dis_type:%d, dis_layer:%d\n", \
            __func__, (int)dis_type, (int)dis_layer);
        return -1;
    }

    if (cast_type != -1){
        if (-1 != rotate)
            m_wifi_cast_state[cast_type].rotate_mode = rotate;
        if (-1 != h_flip)
            m_wifi_cast_state[cast_type].flip_mode = h_flip;       
        if (-1 != project_mode)
            m_wifi_cast_state[cast_type].project_mode = project_mode;        
        if (-1 != h_flip)
            m_wifi_cast_state[cast_type].auto_rotate_disable = auto_rotate_disable;        
    } else {
        int i;
        for (i = 0; i < HCCAST_TEST_WIFI_TYPE_MAX; i ++){
            if (-1 != rotate)
                m_wifi_cast_state[i].rotate_mode = rotate;
            if (-1 != h_flip)
                m_wifi_cast_state[i].flip_mode = h_flip;       
            if (-1 != project_mode)
                m_wifi_cast_state[i].project_mode = project_mode;        
            if (-1 != h_flip)
                m_wifi_cast_state[i].auto_rotate_disable = auto_rotate_disable;        

        }
    }

    if (m_wifi_cast_state[HCCAST_TEST_TYPE_DLNA].state){
        void *player = hccast_media_player_get();
        if (player){
            int rotate_type;
            int flip_type;
            int flip;
            int rotate;
            hctest_get_flip_info(&m_wifi_cast_state[HCCAST_TEST_TYPE_DLNA], 
                &rotate_type, &flip_type);

            //overlay the adding rotation/flip.
            rotate = hccast_test_rotate_convert(rotate_type,
                m_wifi_cast_state[HCCAST_TEST_TYPE_DLNA].rotate_mode);
            flip = hccast_test_flip_convert(m_wifi_cast_state[HCCAST_TEST_TYPE_DLNA].dis_type, flip_type, 
                m_wifi_cast_state[HCCAST_TEST_TYPE_DLNA].flip_mode);

            hcplayer_change_rotate_mirror_type(player, rotate, flip);   
        }
    }
    

    return 0;
}


static int cast_wifi_stop(int argc, char *argv[])
{
	int opt;
    opterr = 0;
    optind = 0;
	int cast_type = -1;

    if (false == m_cast_wifi_start)
    {    
        return 0;
    }

	while ((opt = getopt(argc, &argv[0], "s:")) != EOF) {
		switch (opt) {
		case 's':
			cast_type = atoi(optarg);
            printf("%s(), cast type: %d\n", __func__, (int)cast_type);
            if (cast_type < 0 || cast_type >= HCCAST_TEST_WIFI_TYPE_MAX){
                printf("%s(), line: %d. args err: cast_type %d\n", 
                    __func__, __LINE__, (int)cast_type);
                return -1;
            }
			break;
		default:
			break;
		}
	}

    if (cast_type != -1){
        m_wifi_cast_state[cast_type].state = 0;
    } else {
        int i;
        for (i = 0; i < HCCAST_TEST_WIFI_TYPE_MAX; i ++){
            m_wifi_cast_state[i].state = 0;
        }
    }

	if (0 == m_wifi_cast_state[HCCAST_TEST_TYPE_MIRACAST].state)
	{
	#ifdef MIRACAST_SUPPORT    
	    hccast_mira_service_stop();
	#endif    
	}

	if (0 == m_wifi_cast_state[HCCAST_TEST_TYPE_AIRCAST].state)
	{
	#ifdef AIRCAST_SUPPORT        
	    hccast_air_service_stop();
	#endif

	}

	if (0 == m_wifi_cast_state[HCCAST_TEST_TYPE_DLNA].state)
	{
	#ifdef DLNA_SUPPORT
		hccast_media_stop_by_key();
	    hccast_dlna_service_stop();
	  #ifdef DIAL_SUPPORT
	    hccast_dial_service_stop();
	  #endif
	#endif
	}

    m_cast_wifi_start = false;
	return 0;
}


static int cast_wifi_preview(int argc, char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    int cast_type = -1;
    int arg_base;
    int dst_x;
    int dst_y;
    int dst_w;
    int dst_h;

    if (argc < 5) {
        printf("too few args\n");
        return -1;
    }

    cast_wifi_init(0, NULL);

    while ((opt = getopt(argc, &argv[0], "s:")) != EOF) {
        switch (opt) {
        case 's':
            cast_type = atoi(optarg);
            printf("%s(), cast type: %d\n", __func__, (int)cast_type);
            if (cast_type < 0 || cast_type >= HCCAST_TEST_WIFI_TYPE_MAX){
                printf("%s(), line: %d. args err: cast_type %d\n", 
                    __func__, __LINE__, (int)cast_type);
                return -1;
            }
            break;
        default:
            break;
        }
    }

    if (cast_type != -1){
        //cmd: preview -s 1 0 0 960 540
        
        if (argc < 7) {
            printf("%s(), too few args: preview -s 1 0 0 960 540\n", __func__);
            return -1;
        }
        arg_base = 3;
        dst_x = atoi(argv[arg_base+0]);
        dst_y = atoi(argv[arg_base+1]);
        dst_w = atoi(argv[arg_base+2]);
        dst_h = atoi(argv[arg_base+3]);
        if (
            dst_x < 0 || dst_x > HCCAST_TEST_SCREEN_WIDTH ||
            dst_y < 0 || dst_y > HCCAST_TEST_SCREEN_HEIGHT ||
            dst_w <= 0 || dst_w + dst_x > HCCAST_TEST_SCREEN_WIDTH ||
            dst_h <= 0 || dst_h + dst_y > HCCAST_TEST_SCREEN_HEIGHT
        ) {
            printf("%s(), invalid args\n", __func__);
            printf("%s(), line: %d. 0 <= x <= 1920, 0 <= y <= 1080, 0 < x + w <= 1920, 0 < y + h <= 1080\n",
                __func__, __LINE__);
            return -1;
        }

        m_wifi_cast_state[cast_type].src_rect.x = 0;
        m_wifi_cast_state[cast_type].src_rect.y = 0;
        m_wifi_cast_state[cast_type].src_rect.w = HCCAST_TEST_SCREEN_WIDTH;
        m_wifi_cast_state[cast_type].src_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
        m_wifi_cast_state[cast_type].dst_rect.x = dst_x;
        m_wifi_cast_state[cast_type].dst_rect.y = dst_y;
        m_wifi_cast_state[cast_type].dst_rect.w = dst_w;
        m_wifi_cast_state[cast_type].dst_rect.h = dst_h;
    } else {
        //cmd: preview 0 0 960 540
        if (argc < 5) {
            printf("%s(), too few args: preview 0 0 960 540\n", __func__);
            return -1;
        }

        int i;
        arg_base = 1;
        dst_x = atoi(argv[arg_base+0]);
        dst_y = atoi(argv[arg_base+1]);
        dst_w = atoi(argv[arg_base+2]);
        dst_h = atoi(argv[arg_base+3]);
        if (
            dst_x < 0 || dst_x > HCCAST_TEST_SCREEN_WIDTH ||
            dst_y < 0 || dst_y > HCCAST_TEST_SCREEN_HEIGHT ||
            dst_w <= 0 || dst_w + dst_x > HCCAST_TEST_SCREEN_WIDTH ||
            dst_h <= 0 || dst_h + dst_y > HCCAST_TEST_SCREEN_HEIGHT
        ) {
            printf("%s(), invalid args\n", __func__);
            printf("%s(), line: %d. 0 <= x <= 1920, 0 <= y <= 1080, 0 < x + w <= 1920, 0 < y + h <= 1080\n",
                __func__, __LINE__);
            return -1;
        }

        for (i = 0; i < HCCAST_TEST_WIFI_TYPE_MAX; i ++){
            m_wifi_cast_state[i].src_rect.x = 0;
            m_wifi_cast_state[i].src_rect.y = 0;
            m_wifi_cast_state[i].src_rect.w = HCCAST_TEST_SCREEN_WIDTH;
            m_wifi_cast_state[i].src_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
            m_wifi_cast_state[i].dst_rect.x = dst_x;
            m_wifi_cast_state[i].dst_rect.y = dst_y;
            m_wifi_cast_state[i].dst_rect.w = dst_w;
            m_wifi_cast_state[i].dst_rect.h = dst_h;
        }
    }

    return 0;
}

static int cast_wifi_info(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    int i;
    printf("\ncast wifi test information:\n");    
    char *rotate_str[] = {
        "0",
        "90",
        "180",
        "270",
    };

    for (i = 0; i < HCCAST_TEST_WIFI_TYPE_MAX; i ++){
    
        printf("\tcast: %s[%s]. [pbp:%d][%s][%s]\n", m_wifi_cast_state[i].name, 
            m_wifi_cast_state[i].state ? "On" : "Off",
            m_wifi_cast_state[i].pbp_on, 
            m_wifi_cast_state[i].dis_type == HCCAST_TEST_DIS_TYPE_HD ? "HD" : "UHD",
            m_wifi_cast_state[i].dis_layer == HCCAST_TEST_DIS_LAYER_MAIN ? "main layer" : "auxp layer");

        printf("\t\trotate:[%s], flip: [%s]\n", rotate_str[m_wifi_cast_state[i].rotate_mode], 
            m_wifi_cast_state[i].flip_mode ? "Horizon" : "No");

        printf("\t\tsrc_rect:{%d,%d,%d,%d}, dst_rect:{%d,%d,%d,%d}\n", 
            m_wifi_cast_state[i].src_rect.x, m_wifi_cast_state[i].src_rect.y,
            m_wifi_cast_state[i].src_rect.w, m_wifi_cast_state[i].src_rect.h,
            m_wifi_cast_state[i].dst_rect.x, m_wifi_cast_state[i].dst_rect.y,
            m_wifi_cast_state[i].dst_rect.w, m_wifi_cast_state[i].dst_rect.h
            );

    }

    return 0;
}


static int cast_wifi_disable_audio(int argc, char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
    int cast_type = -1;
    int audio_disable = -1;
    int i;

    if (argc < 3) {
        printf("too few args\n");
        return -1;
    }

    while ((opt = getopt(argc, &argv[0], "s:a:")) != EOF) {
        switch (opt) {
        case 's':
            cast_type = atoi(optarg);
            printf("%s(), cast type: %d\n", __func__, (int)cast_type);
            if (cast_type < 0 || cast_type >= HCCAST_TEST_WIFI_TYPE_MAX){
                printf("%s(), line: %d. args err: cast_type %d\n", 
                    __func__, __LINE__, (int)cast_type);
                return -1;
            }
            break;
        case 'a':
            audio_disable = atoi(optarg);
            audio_disable = audio_disable ? 1 : 0;
            break;
        default:
            break;
        }
    }

    if (-1 == audio_disable)
        return -1;

    if (cast_type != -1){
        m_wifi_cast_state[cast_type].audio_disable = audio_disable;
    } else {
        for (i = 0; i < HCCAST_TEST_WIFI_TYPE_MAX; i ++)
            m_wifi_cast_state[i].audio_disable = audio_disable;
    }
    hccast_com_audio_disable_set(audio_disable);
    return 0;
}

static const char help_start[] = 
        "Start WiFi cast(miracast/aircast/dlna..)\n\t\t\t"
        "start -s 1 -p 1 -d 1 -l 1 -r 1 -m 0 -f 0\n\t\t\t"
        "-s start cast type: \n\t\t\t"
        "   0: start miracast; 1: start aircast; 2: start dlna; No -s: start all casts\n\t\t\t"
        "-p multi display enable: 0, disable; 1, enable\n\t\t\t"
        "-d set dis type: 0, DIS HD; 1. DIS UHD\n\t\t\t"
        "-l set dis layer: 0, DIS main layer; 1. DIS auxp layer\n\t\t\t"
        "-r set rotate angle: 0, 0 degree; 1, 90 degree; 2, 180 degree, 3. 270 degree\n\t\t\t"
        "-m enable horizon flip: 0, disable horizontal flip; 1. enable horizontal flip\n\t\t\t"
        "-f project mode: 0, Front; 1, Ceiling Front; 2, Rear; 3, Ceiling Rear\n\t\t\t"
        "-k enable mirror full screen: 0, disable; 1. enable\n\t\t\t"
        "-j enable vscreen auto rotation: 0, disable; 1. enable\n\t\t\t"
        "-o sound dev path: 1, I2SO; 2, PCM output; 4, spdif output; 8, DDP spdif output. can overlay\n\t\t\t"
        "-g enable use audio dec: 1, enable; 0, disable. enable can mix audio with media player\n\t\t\t"
        "-h disable audio: 1, disable; 0, enable.\n\t\t\t"
        "-a disable auto rotate and zoom: 0, enable auto roate; 1. disable auto rotate\n\t"
        ;

static const char help_stop[] = 
        "Stop WiFi cast(miracast/aircast/dlna..)\n\t\t\t"
        "stop -s 1\n\t\t\t"
        "-s stop cast type: \n\t\t\t"
        "   0: stop miracast; 1: stop aircast; 2: stop dlna; No -s: stop all casts\n\t\t\t"
        ;

static const char help_rotate[] = 
        "Set WiFi cast rotate(miracast/aircast/dlna..)\n\t\t\t"
        "rotate -s 1 -r 1 -m 1 -f 0\n\t\t\t"
        "-s set cast type: \n\t\t\t"
        "   0: set miracast; 1: set aircast; 2: set dlna; No -s: set all casts\n\t\t\t"
        "-r set rotate angle: 0, 0 degree; 1, 90 degree; 2, 180 degree, 3. 270 degree\n\t\t\t"
        "-m enable horizon flip: 0, disable horizontal flip; 1. enable horizontal flip\n\t\t\t"
        "-f project mode: 0, Front; 1, Ceiling Front; 2, Rear; 3, Ceiling Rear\n\t\t\t"
        "-a disable auto rotate and zoom: 0, enable auto roate; 1. enable auto rotate\n\t"
        ;

static const char help_preview[] = 
        "Set WiFi cast from full screen to preview(miracast/aircast/dlna..)\n\t\t\t"
        "preview -s 1 {dst_rect}\n\t\t\t"
        "   {dst_rect} base on 1920*1080\n\t\t\t"
        "-s set preview for cast type: \n\t\t\t"
        "   0: miracast preview; 1. aircast preview; 2. dlna preview; No -s: all cast preview\n\t\t\t"
        " for example: \n\t\t\t"
        "  preview -s 1 0 0 960 540\n\t\t\t"
        ;

static const char help_disa[] = 
        "disable audio\n\t\t\t"
        "disa -a 0\n\t\t\t"
        "-s set cast type: \n\t\t\t"
        "   0: set miracast; 1: set aircast; 2: set dlna; No -s: set all casts\n\t\t\t"
        "-a : 1, disable; 0, enable\n\t"
        ;

static const char help_info[] = "Show WiFi cast information(miracast/aircast/dlna..)\n\t";


#ifdef __linux__

void wifi_cast_cmds_register(struct console_cmd *cmd)
{
    console_register_cmd(cmd, "start",  cast_wifi_start, CONSOLE_CMD_MODE_SELF, help_start);

    console_register_cmd(cmd, "stop",  cast_wifi_stop, CONSOLE_CMD_MODE_SELF, help_stop);

    console_register_cmd(cmd, "rotate",  cast_wifi_rotate, CONSOLE_CMD_MODE_SELF, help_rotate);

    console_register_cmd(cmd, "preview",  cast_wifi_preview, CONSOLE_CMD_MODE_SELF, help_preview);

    console_register_cmd(cmd, "disa",  cast_wifi_disable_audio, CONSOLE_CMD_MODE_SELF, help_disa);

    console_register_cmd(cmd, "info",  cast_wifi_info, CONSOLE_CMD_MODE_SELF, help_info);
}

#ifndef BR2_PACKAGE_VIDEO_PBP_EXAMPLES
int main(int argc , char *argv[])
{
    (void)argc;
    (void)argv;

    cast_console_init();

    cast_wifi_init(0, NULL);

    console_init("cast_wifi:");

    wifi_cast_cmds_register(NULL);

    console_start();

    cast_test_exit_console(0);

    return 0;
}
#endif


#else

static int hccast_sleep(int argc, char *argv[])
{
    int sleep_ms;

    if (argc != 2)
        return 0;

    sleep_ms = atoi(argv[1]);
    usleep(sleep_ms*1000);
    return 0;
}

CONSOLE_CMD(sleep, NULL, hccast_sleep, CONSOLE_CMD_MODE_SELF,
            "sleep milliseconds")

CONSOLE_CMD(cast_wifi, NULL, cast_wifi_init, CONSOLE_CMD_MODE_SELF,
            "enter and init hccast wifi cast testing...")

CONSOLE_CMD(start, "cast_wifi", cast_wifi_start, CONSOLE_CMD_MODE_SELF, help_start)

CONSOLE_CMD(stop, "cast_wifi", cast_wifi_stop, CONSOLE_CMD_MODE_SELF, help_stop)

CONSOLE_CMD(rotate, "cast_wifi", cast_wifi_rotate, CONSOLE_CMD_MODE_SELF, help_rotate)

CONSOLE_CMD(preview, "cast_wifi", cast_wifi_preview, CONSOLE_CMD_MODE_SELF, help_preview)

CONSOLE_CMD(disa, "cast_wifi", cast_wifi_disable_audio, CONSOLE_CMD_MODE_SELF, help_disa)

CONSOLE_CMD(info, "cast_wifi", cast_wifi_info, CONSOLE_CMD_MODE_SELF, help_info)


#endif


