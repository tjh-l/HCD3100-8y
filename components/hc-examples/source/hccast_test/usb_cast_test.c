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
#include <linux/fb.h>
#include <libusb-1.0/libusb.h>
#else
#include <kernel/lib/console.h>
#include <kernel/fb.h>
#endif

#include "hccast_test.h"

#include <hcuapi/dis.h>
#include <ffplayer.h>

#include <hccast/hccast_scene.h>
#include <hccast/hccast_net.h>
#include <hccast/hccast_com.h>

#ifdef USBMIRROR_SUPPORT
#include <hccast/hccast_um.h>
#endif

#include <hudi/hudi_audsink.h>

static bool m_um_test_start = false;
static hccast_test_state_t m_usb_cast_state[HCCAST_TEST_USB_TYPE_MAX];

static int g_ium_vd_dis_mode = DIS_PILLBOX;
static char m_ium_uuid[40] = {0};

static void hctest_um_set_dis_zoom(hccast_um_zoom_info_t *um_zoom_info)
{
    hccast_test_rect_t src_rect;
    hccast_test_rect_t dst_rect;

    hccast_test_state_t *cast_state = &m_usb_cast_state[HCCAST_TEST_TYPE_IUM];    

    src_rect.x = um_zoom_info->src_rect.x;
    src_rect.y = um_zoom_info->src_rect.y;
    src_rect.w = um_zoom_info->src_rect.w;
    src_rect.h = um_zoom_info->src_rect.h;

    dst_rect.x = cast_state->dst_rect.x;
    dst_rect.y = cast_state->dst_rect.y;
    dst_rect.w = cast_state->dst_rect.w;
    dst_rect.h = cast_state->dst_rect.h;

//    cast_api_set_dis_zoom(&src_rect, &dst_rect, dis_active_mode);
    hccast_test_display_zoom(cast_state->dis_type, cast_state->dis_layer, 
        &src_rect, &dst_rect, DIS_SCALE_ACTIVE_IMMEDIATELY);

}

static int hctest_ium_set_dis_aspect_mode(void)
{
    hccast_um_param_t um_param;
    hccast_um_zoom_info_t um_zoom_info = {{0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT}, 
        {0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT}};
    hccast_test_state_t *cast_state = &m_usb_cast_state[HCCAST_TEST_TYPE_IUM];

    hccast_um_param_get(&um_param);

    if (um_param.full_screen_en)
    {
        //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
        hccast_test_set_aspect_mode(cast_state, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
        hctest_um_set_dis_zoom(&um_zoom_info);
        g_ium_vd_dis_mode = DIS_NORMAL_SCALE;
    }
    else
    {
        //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
        hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
        hctest_um_set_dis_zoom(&um_zoom_info);
        g_ium_vd_dis_mode = DIS_PILLBOX;
    }
            
    return 0;
}

static int hctest_um_rotate_angle_convert(int rotate_mode)
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


static int hctest_ium_get_rotate_info(hccast_ium_screen_mode_t *ium_screen_mode, hccast_um_rotate_info_t *rotate_info)
{
    rotate_type_e final_rotate_angle = 0;
    rotate_type_e ium_rotate_mode_angle = 0;
    rotate_type_e rotate_seting_angle = 0;
    //rotate_type_e rotate_mode = 0;
    int expect_vd_dis_mode = 0;
    hccast_um_param_t um_param;
    int video_width;
    int video_height;
    int ium_rotate_mode;
    int flip_rotate = 0;
    int flip_mirror = 0;
    hccast_test_state_t *cast_state = &m_usb_cast_state[HCCAST_TEST_TYPE_IUM];
    
    if (!ium_screen_mode || !rotate_info)
    {
        return -1;
    }

    hccast_um_param_get(&um_param);
    video_width = ium_screen_mode->video_width;
    video_height = ium_screen_mode->video_height;
    ium_rotate_mode = ium_screen_mode->rotate_mode;
    //api_get_flip_info(&flip_rotate, &flip_mirror);
    hctest_get_flip_info(cast_state, &flip_rotate, &flip_mirror);
    
    if (!um_param.screen_rotate_en)
    {

        ium_rotate_mode_angle = hctest_um_rotate_angle_convert(ium_rotate_mode);
        rotate_seting_angle = hctest_um_rotate_angle_convert(um_param.screen_rotate_en);
        //rotate_mode = (flip_rotate + rotate_seting_angle) % 4;//flip_rotate;
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
        ium_rotate_mode_angle = hctest_um_rotate_angle_convert(ium_rotate_mode);
        rotate_seting_angle = hctest_um_rotate_angle_convert(um_param.screen_rotate_en);
        //rotate_mode = (flip_rotate + rotate_seting_angle) % 4;
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
                expect_vd_dis_mode = DIS_PILLBOX;
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
        g_ium_vd_dis_mode = expect_vd_dis_mode;
        //cast_api_set_aspect_mode(g_cast_dis_mode, expect_vd_dis_mode, DIS_SCALE_ACTIVE_NEXTFRAME);
        hccast_test_set_aspect_mode(cast_state, expect_vd_dis_mode, DIS_SCALE_ACTIVE_NEXTFRAME);
    }

    rotate_info->rotate_angle = final_rotate_angle;
    rotate_info->flip_mode = flip_mirror;
    
    return 0;
}


static void ium_event_process_cb(int event, void *param1, void *param2)
{

    if (event != HCCAST_IUM_EVT_GET_FLIP_MODE && event != HCCAST_IUM_EVT_GET_ROTATION_INFO)
        printf("ium event: %d\n", event);

    hccast_test_state_t *cast_state = &m_usb_cast_state[HCCAST_TEST_TYPE_IUM];
    switch (event)
    {
    case HCCAST_IUM_EVT_DEVICE_ADD:
        break;
    case HCCAST_IUM_EVT_DEVICE_REMOVE:
        break;
    case HCCAST_IUM_EVT_MIRROR_START:
		printf("[%s] HCCAST_IUM_EVT_MIRROR_START\n", __func__);        
        break;
    case HCCAST_IUM_EVT_MIRROR_STOP:
		printf("[%s] HCCAST_IUM_EVT_MIRROR_STOP\n", __func__);        
        break;
    case HCCAST_IUM_EVT_SAVE_PAIR_DATA: //param1: buf; param2: length
        break;
    case HCCAST_IUM_EVT_GET_PAIR_DATA: //param1: buf; param2: length
        break;
    case HCCAST_IUM_EVT_NEED_USR_TRUST:
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
        printf("[%s],line:%d. HCCAST_IUM_EVT_CERT_INVALID\n",__func__, __LINE__);
	    break;
    case HCCAST_IUM_EVT_NO_DATA:
        printf("%s(), HCCAST_IUM_EVT_NO_DATA\n", __func__);
        break;
    case HCCAST_IUM_EVT_COPYRIGHT_PROTECTION:
        break;
    case HCCAST_IUM_EVT_SET_DIS_ASPECT:
    {    //cast_api_ium_set_dis_aspect_mode();
        if (cast_state->auto_rotate_disable){
            hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);    
        } else {
            hctest_ium_set_dis_aspect_mode();
        }
        break;
    }
    case HCCAST_IUM_EVT_GET_ROTATION_INFO:
    {
        if (cast_state->auto_rotate_disable){
            int rotate_type;
            int flip_type;
            int flip;
            int rotate;
            hccast_um_rotate_info_t *rotate_info = (hccast_um_rotate_info_t*)param2;

            hctest_get_flip_info(cast_state, &rotate_type, &flip_type);
            //overlay the adding rotation/flip.
            rotate = hccast_test_rotate_convert(rotate_type, cast_state->rotate_mode);
            flip = hccast_test_flip_convert(cast_state->dis_type, flip_type, cast_state->flip_mode);

            rotate_info->rotate_angle = rotate;
            rotate_info->flip_mode = flip;
        } else {
            hctest_ium_get_rotate_info((hccast_ium_screen_mode_t*)param1, (hccast_um_rotate_info_t*)param2);    
        }

        break;
    }    

    case HCCAST_IUM_EVT_GET_PREVIEW_INFO:
    {
    	printf("[%s] HCCAST_IUM_EVT_GET_PREVIEW_INFO\n", __func__);

        hccast_um_preview_info_t preview_info = {{0,0,HCCAST_TEST_SCREEN_WIDTH,HCCAST_TEST_SCREEN_HEIGHT}, 
            {0,0,HCCAST_TEST_SCREEN_WIDTH,HCCAST_TEST_SCREEN_HEIGHT}, 0};

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

        memcpy(param1, &preview_info, sizeof(hccast_um_preview_info_t));
        break;        
    }

    case HCCAST_IUM_EVT_GET_VIDEO_CONFIG:
    {    
        printf("[%s] HCCAST_IUM_EVT_GET_VIDEO_CONFIG\n", __func__);         

        hccast_com_video_config_t *video_config = (hccast_com_video_config_t*)param1;
        video_config->video_pbp_mode = cast_state->pbp_on ? 
                            HCCAST_COM_VIDEO_PBP_2P_ON : HCCAST_COM_VIDEO_PBP_OFF;
        video_config->video_dis_type = cast_state->dis_type ? 
                            HCCAST_COM_VIDEO_DIS_TYPE_UHD : HCCAST_COM_VIDEO_DIS_TYPE_HD;
        video_config->video_dis_layer = cast_state->dis_layer ? 
                            HCCAST_TEST_DIS_LAYER_AUXP : HCCAST_COM_VIDEO_DIS_LAYER_MAIN;

        printf("ium: pbp_mode:%d, dis_type:%d, dis_layer:%d\n",  
        	video_config->video_pbp_mode, video_config->video_dis_type, video_config->video_dis_layer);

        break;
    }

    default:
        break;
    }

}

static int hctest_aum_set_dis_aspect_mode(hccast_aum_screen_mode_t *screen_mode)
{
    hccast_um_param_t um_param;
    hccast_um_zoom_info_t um_zoom_info = {{0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT}, 
        {0, 0, HCCAST_TEST_SCREEN_WIDTH, HCCAST_TEST_SCREEN_HEIGHT}};
    rotate_type_e rotate_seting_angle;
    //rotate_type_e rotate_angle;
    int flip_rotate = 0;
    int flip_mirror = 0;
    float ratio;
    hccast_test_state_t *cast_state = &m_usb_cast_state[HCCAST_TEST_TYPE_AUM];

    if (!screen_mode)
    {
        return -1;
    }
    
    hccast_um_param_get(&um_param);
    //api_get_flip_info(&flip_rotate, &flip_mirror);
    hctest_get_flip_info(cast_state, &flip_rotate, &flip_mirror);
    rotate_seting_angle = hctest_um_rotate_angle_convert(um_param.screen_rotate_en);
    //rotate_angle = (rotate_seting_angle + flip_rotate) % 4;


    if (um_param.full_screen_en)
    {
        if (!screen_mode->mode)//VSCR
        {
            ratio = (float)screen_mode->screen_height / (float)screen_mode->screen_width;
            if ((rotate_seting_angle == ROTATE_TYPE_270) || (rotate_seting_angle == ROTATE_TYPE_90))
            {
                if ((ratio > 1.8) && (screen_mode->screen_height > HCCAST_TEST_SCREEN_WIDTH))
                {
                    if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                    {
                        um_zoom_info.src_rect.h = (screen_mode->screen_width * HCCAST_TEST_SCREEN_WIDTH) / 
                            screen_mode->screen_height;
                        um_zoom_info.src_rect.y = (HCCAST_TEST_SCREEN_HEIGHT - um_zoom_info.src_rect.h) / 2;
                        //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                        hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    }
                    else
                    {
                        int tmp_h = (screen_mode->screen_width * HCCAST_TEST_SCREEN_WIDTH) / screen_mode->screen_height;
                        um_zoom_info.src_rect.w = tmp_h*HCCAST_TEST_SCREEN_WIDTH/HCCAST_TEST_SCREEN_HEIGHT; //when lcd is vscreen, height is trans to width.
                        um_zoom_info.src_rect.x = (HCCAST_TEST_SCREEN_WIDTH - um_zoom_info.src_rect.w)/2;
                        um_zoom_info.src_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
                        um_zoom_info.src_rect.y = 0;
                        //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                        hccast_test_set_aspect_mode(cast_state, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    }
                    hctest_um_set_dis_zoom(&um_zoom_info);
                }
                else
                {
                    
                    //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    hccast_test_set_aspect_mode(cast_state, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    hctest_um_set_dis_zoom(&um_zoom_info);
                }
            }
            else
            {
                //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                hctest_um_set_dis_zoom(&um_zoom_info);
            }
        }
        else if(screen_mode->mode)
        {   
            ratio = (float)screen_mode->screen_width / (float)screen_mode->screen_height;
            if ((rotate_seting_angle == ROTATE_TYPE_180) || (rotate_seting_angle == ROTATE_TYPE_0) || um_param.screen_rotate_auto)
            {
                if ((ratio > 1.8) && (screen_mode->screen_width > HCCAST_TEST_SCREEN_WIDTH))
                {
                    if (flip_rotate == ROTATE_TYPE_0 || flip_rotate == ROTATE_TYPE_180)
                    {
                        um_zoom_info.src_rect.h = (screen_mode->screen_height * HCCAST_TEST_SCREEN_WIDTH) / 
                            screen_mode->screen_width;
                        um_zoom_info.src_rect.y = (HCCAST_TEST_SCREEN_HEIGHT - um_zoom_info.src_rect.h) / 2;
                        //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                        hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    }
                    else
                    {
                        int tmp_h = (screen_mode->screen_height * HCCAST_TEST_SCREEN_WIDTH) / screen_mode->screen_width;
                        um_zoom_info.src_rect.w = tmp_h*HCCAST_TEST_SCREEN_WIDTH/HCCAST_TEST_SCREEN_HEIGHT; //when lcd is vscreen, height is trans to width.
                        um_zoom_info.src_rect.x = (HCCAST_TEST_SCREEN_WIDTH - um_zoom_info.src_rect.w)/2;
                        um_zoom_info.src_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
                        um_zoom_info.src_rect.y = 0;
                        //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                        hccast_test_set_aspect_mode(cast_state, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    }
                    
                    hctest_um_set_dis_zoom(&um_zoom_info);
                } 
                else
                {
                    //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    hccast_test_set_aspect_mode(cast_state, DIS_NORMAL_SCALE, DIS_SCALE_ACTIVE_IMMEDIATELY);
                    hctest_um_set_dis_zoom(&um_zoom_info);
                }
            }
            else
            {
                //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
                hctest_um_set_dis_zoom(&um_zoom_info);
            }
        }
    }
    else
    {
        //cast_api_set_aspect_mode(g_cast_dis_mode, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
        hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);
        hctest_um_set_dis_zoom(&um_zoom_info);
    }

    return 0;
}

int hctest_aum_get_rotate_info(hccast_aum_screen_mode_t *aum_screen_mode, hccast_um_rotate_info_t *rotate_info)
{
    rotate_type_e final_rotate_angle = 0;
    rotate_type_e rotate_seting_angle = 0;
    hccast_um_param_t um_param;
    int flip_rotate = 0;
    int flip_mirror = 0;
    hccast_test_state_t *cast_state = &m_usb_cast_state[HCCAST_TEST_TYPE_AUM];    

    if (!aum_screen_mode || !rotate_info)
    {
        return -1;
    }

    hccast_um_param_get(&um_param);
    //api_get_flip_info(&flip_rotate, &flip_mirror);
    hctest_get_flip_info(cast_state, &flip_rotate, &flip_mirror);
    
    if (!um_param.screen_rotate_en)
    {
        final_rotate_angle = (ROTATE_TYPE_0 + flip_rotate) % 4;//Angle superposition calculation.
    }
    else
    {
        if (um_param.screen_rotate_auto && aum_screen_mode->mode)
        {
            final_rotate_angle = (ROTATE_TYPE_0 + flip_rotate) % 4;//ROTATE_TYPE_0
        }
        else
        { 
            rotate_seting_angle = hctest_um_rotate_angle_convert(um_param.screen_rotate_en);
            final_rotate_angle = (flip_rotate + rotate_seting_angle) % 4;//Angle superposition calculation.
        }
    }

    rotate_info->rotate_angle = final_rotate_angle;
    rotate_info->flip_mode = flip_mirror;

    return 0;
}

static void aum_event_process_cb(int event, void *param1, void *param2)
{
    if (HCCAST_AUM_EVT_GET_FLIP_MODE != event && HCCAST_AUM_EVT_GET_ROTATION_INFO != event)
        printf("aum event: %d\n", event);
    
    hccast_test_state_t *cast_state = &m_usb_cast_state[HCCAST_TEST_TYPE_AUM];
    switch (event)
    {
    case HCCAST_AUM_EVT_DEVICE_ADD:
    	printf("[%s] HCCAST_AUM_EVT_DEVICE_ADD\n", __func__);         
        break;
    case HCCAST_AUM_EVT_DEVICE_REMOVE:
        break;
    case HCCAST_AUM_EVT_MIRROR_START:
    	printf("[%s] HCCAST_AUM_EVT_MIRROR_START\n", __func__);         
        break;
    case HCCAST_AUM_EVT_MIRROR_STOP:
    	printf("[%s] HCCAST_AUM_EVT_MIRROR_STOP\n", __func__);         
        break;
    case HCCAST_AUM_EVT_IGNORE_NEW_DEVICE:
        break;
    case HCCAST_AUM_EVT_SERVER_MSG:
        break;
    case HCCAST_AUM_EVT_UPG_DOWNLOAD_PROGRESS:
        break;
    case HCCAST_AUM_EVT_GET_UPGRADE_DATA:
        break;
    case HCCAST_AUM_EVT_SET_SCREEN_ROTATE:
        // printf("aum set screen rotate: %d\n", (int)param1);
        // projector_set_some_sys_param(P_MIRROR_ROTATION, (int)param1);
        // projector_sys_param_save();
        cast_state->rotate_mode = (int)param1;
        break;
    case HCCAST_AUM_EVT_SET_AUTO_ROTATE:
        break;
    case HCCAST_AUM_EVT_SET_FULL_SCREEN:
       //*(int*)param1 = projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT) == 0 ? 1 : 0;
        // printf("aum set full screen: %d\n", (int)param1);
        // projector_set_some_sys_param(P_UM_FULL_SCREEN, (int)param1);
        // projector_sys_param_save();
        cast_state->um_full_screen = (int)param1;
        break;
    case HCCAST_AUM_EVT_GET_UPGRADE_BUF:
    {
        // hccast_aum_upg_bi_t *bi = (hccast_aum_upg_bi_t *)param1;
        // if (bi)
        // {
        //     bi->buf = cast_um_alloc_upg_buf(bi->len);
        // }
        break;
    }
    case HCCAST_AUM_EVT_SET_DIS_ASPECT:
    {    
        if (cast_state->auto_rotate_disable){
            hccast_test_set_aspect_mode(cast_state, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);    
        } else {
            hctest_aum_set_dis_aspect_mode((hccast_aum_screen_mode_t*)param1);    
        }
        break;
    }
    case HCCAST_AUM_EVT_GET_ROTATION_INFO:
    {
        //cast_api_aum_get_rotate_info((hccast_aum_screen_mode_t*)param1, (hccast_um_rotate_info_t*)param2);
        if (cast_state->auto_rotate_disable){
            int rotate_type;
            int flip_type;
            int flip;
            int rotate;
            hccast_um_rotate_info_t *rotate_info = (hccast_um_rotate_info_t*)param2;

            hctest_get_flip_info(cast_state, &rotate_type, &flip_type);
            //overlay the adding rotation/flip.
            rotate = hccast_test_rotate_convert(rotate_type, cast_state->rotate_mode);
            flip = hccast_test_flip_convert(cast_state->dis_type, flip_type, cast_state->flip_mode);

            rotate_info->rotate_angle = rotate;
            rotate_info->flip_mode = flip;

        } else {
            hctest_aum_get_rotate_info((hccast_aum_screen_mode_t*)param1, (hccast_um_rotate_info_t*)param2);
        }

        break;
	}

    case HCCAST_AUM_EVT_GET_PREVIEW_INFO:
    {
    	printf("[%s] HCCAST_AUM_EVT_GET_PREVIEW_INFO\n", __func__);

        hccast_um_preview_info_t preview_info = {{0,0,HCCAST_TEST_SCREEN_WIDTH,HCCAST_TEST_SCREEN_HEIGHT}, 
            {0,0,HCCAST_TEST_SCREEN_WIDTH,HCCAST_TEST_SCREEN_HEIGHT}, 0};

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

        memcpy(param1, &preview_info, sizeof(hccast_um_preview_info_t));
        break;        

    }
    case HCCAST_AUM_EVT_GET_VIDEO_CONFIG:
    {    
        printf("[%s] HCCAST_AUM_EVT_GET_VIDEO_CONFIG\n", __func__);         

        hccast_com_video_config_t *video_config = (hccast_com_video_config_t*)param1;
        video_config->video_pbp_mode = cast_state->pbp_on ? 
                            HCCAST_COM_VIDEO_PBP_2P_ON : HCCAST_COM_VIDEO_PBP_OFF;
        video_config->video_dis_type = cast_state->dis_type ? 
                            HCCAST_COM_VIDEO_DIS_TYPE_UHD : HCCAST_COM_VIDEO_DIS_TYPE_HD;
        video_config->video_dis_layer = cast_state->dis_layer ? 
                            HCCAST_TEST_DIS_LAYER_AUXP : HCCAST_COM_VIDEO_DIS_LAYER_MAIN;

        printf("aum: pbp_mode:%d, dis_type:%d, dis_layer:%d\n",  
        	video_config->video_pbp_mode, video_config->video_dis_type, video_config->video_dis_layer);

        break;
    }
    default:
        break;
    }    
}

static void usb_mirror_rotate_init(void)
{
    hccast_um_param_t um_param = {0};
    int full_screen_en = 0;

    hccast_test_state_t *cast_state = &m_usb_cast_state[HCCAST_TEST_TYPE_AUM];

    if (cast_state->rotate_mode)
        um_param.screen_rotate_en = 1;
    else
        um_param.screen_rotate_en = 0;

    if (cast_state->mirror_vscreen_auto_rotation)
        um_param.screen_rotate_auto = 1;
    else
        um_param.screen_rotate_auto = 0;

    full_screen_en = cast_state->um_full_screen;

    hccast_um_param_t param;
    hccast_um_param_get(&param);
    um_param.full_screen_en = full_screen_en;

    hccast_um_param_set(&um_param);
}


int cast_usb_init(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

#ifdef __linux__
    /*
    In the meson.build compiling, the application calls libraries in the following order: 
    libhccast-um.so -> libusbmirror.so -> libusb.so. libhccast-um.so uses dlopen() to call library functions 
    of libusbmirror.so, and does not directly call libusb.so. This may result in the application 
    not being linked to libusb.so. Here, it is necessary to show a call to 
    a function from libusb.so to force the application to link to libusb.so.    
     */
    const struct libusb_version *version;
    libusb_init(NULL);
    version = libusb_get_version();
    printf("%s() libusb version: libusb-%hu-%hu.\n", __func__, version->major, version->minor);
#endif    

	static bool cast_usb_init_flag = false;
    int i;
	if (cast_usb_init_flag)
		return 0;

    memset(m_usb_cast_state, 0, sizeof(hccast_test_state_t));
    for (i = 0; i < HCCAST_TEST_USB_TYPE_MAX; i ++){
        switch (i)
        {
        case HCCAST_TEST_TYPE_AUM:
            sprintf(m_usb_cast_state[i].name, "aum");
            break;
        case HCCAST_TEST_TYPE_IUM:
            sprintf(m_usb_cast_state[i].name, "ium");
            break;
        default:
            break;
        }

        m_usb_cast_state[i].rotate_mode = 0;
        m_usb_cast_state[i].flip_mode = 0;;
        m_usb_cast_state[i].tv_mode = DIS_TV_16_9;
        m_usb_cast_state[i].project_mode = HCTEST_PROJECT_REAR;        

        m_usb_cast_state[i].mirror_vscreen_auto_rotation = 1;
        m_usb_cast_state[i].mirror_full_vscreen = 1;
        m_usb_cast_state[i].um_full_screen = 1;
        m_usb_cast_state[i].audio_path = AUDSINK_SND_DEVBIT_I2SO;

        m_usb_cast_state[i].pbp_on = 0;
        m_usb_cast_state[i].dis_type = HCCAST_TEST_DIS_TYPE_HD;
        m_usb_cast_state[i].dis_layer = HCCAST_TEST_DIS_LAYER_MAIN;
        m_usb_cast_state[i].src_rect.x = 0;
        m_usb_cast_state[i].src_rect.y = 0;
        m_usb_cast_state[i].src_rect.w = HCCAST_TEST_SCREEN_WIDTH;
        m_usb_cast_state[i].src_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
        m_usb_cast_state[i].dst_rect.x = 0;
        m_usb_cast_state[i].dst_rect.y = 0;
        m_usb_cast_state[i].dst_rect.w = HCCAST_TEST_SCREEN_WIDTH;
        m_usb_cast_state[i].dst_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
        m_usb_cast_state[i].state = 0;

    }

    hccast_um_init();
    if (hccast_um_init() < 0){
        printf("%s(), line:%d. hccast_um_init() fail!\n", __func__, __LINE__);
        return -1;
    }
    hccast_ium_init(ium_event_process_cb);

    usb_mirror_rotate_init();

	cast_usb_init_flag = true;
	return 0;
}

static int cast_usb_start(int argc, char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
	int dis_type = -1;
	int dis_layer = -1;
	int pbp_on = -1;
    int rotate = -1;
    int h_flip = -1;
    int project_mode = -1;
    int um_full_screen = -1;
    int mirror_vscreen_auto_rotation = -1;
    int auto_rotate_disable = -1;
    int audio_path = -1;
    int audio_dec_enable = -1;
    int audio_disable = -1;

    int ret;
    int i;
    hccast_aum_param_t aum_param = {0};

    if (m_um_test_start)
    {    
        return 0;
    }

	cast_usb_init(argc, argv);

	while ((opt = getopt(argc, &argv[0], "p:d:l:r:m:f:k:j:a:o:g:h:")) != EOF) {
		switch (opt) {
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
			printf("%s(), dis layer: %d\n", __func__, (int)dis_layer);
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
            um_full_screen = atoi(optarg);
            um_full_screen = um_full_screen ? 1 : 0;
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

    for (i = 0; i < HCCAST_TEST_USB_TYPE_MAX; i ++){
        m_usb_cast_state[i].state = 1;
        if (-1 != pbp_on)
            m_usb_cast_state[i].pbp_on = pbp_on;
        if (-1 != dis_type)
            m_usb_cast_state[i].dis_type = dis_type;
        if (-1 != dis_layer)
            m_usb_cast_state[i].dis_layer = dis_layer;
        if (-1 != rotate)
            m_usb_cast_state[i].rotate_mode = rotate;
        if (-1 != h_flip)
            m_usb_cast_state[i].flip_mode = h_flip;
        if (-1 != um_full_screen)
            m_usb_cast_state[i].um_full_screen = um_full_screen;
        if (-1 != mirror_vscreen_auto_rotation)
            m_usb_cast_state[i].mirror_vscreen_auto_rotation = mirror_vscreen_auto_rotation;
        if (-1 != auto_rotate_disable)
            m_usb_cast_state[i].auto_rotate_disable = auto_rotate_disable;
        if (-1 != audio_path)
            m_usb_cast_state[i].audio_path = audio_path;
        if (-1 != audio_dec_enable)
            m_usb_cast_state[i].audio_dec_enable = audio_dec_enable;
        if (-1 != audio_disable)
            m_usb_cast_state[i].audio_disable = audio_disable;
    }
    hccast_com_audsink_set(m_usb_cast_state[0].audio_path);        
    hccast_com_force_auddec_set(m_usb_cast_state[0].audio_dec_enable);        
    hccast_com_audio_disable_set(m_usb_cast_state[0].audio_disable);

    ret = hccast_ium_start(m_ium_uuid, ium_event_process_cb);
    if (ret){
        printf("%s(), line:%d. ret = %d\n", __func__, __LINE__, ret);    
        return -1;
    }

    strcat(aum_param.product_id, "HCT-AT01");
    snprintf(aum_param.fw_url, sizeof(aum_param.fw_url), "%s", "hccast_test_url");
    strcat(aum_param.apk_url, "hccast_test_url");
    strcat(aum_param.aoa_desc, "ElfCast-Screen_Mirror");
    aum_param.fw_version = 12345678;
    
    ret = hccast_aum_start(&aum_param, aum_event_process_cb);
    if (ret){
        printf("%s(), line:%d. ret = %d\n", __func__, __LINE__, ret);    
        return m_um_test_start;
    }

    m_um_test_start = true;
    return 0;

}


static int cast_usb_rotate(int argc, char *argv[])
{
    int opt;
    opterr = 0;
    optind = 0;
	int dis_type = -1;
	int dis_layer = -1;
    int rotate = -1;
    int h_flip = -1;
    int project_mode = -1;
    int auto_rotate_disable = -1;
    int i;

    while ((opt = getopt(argc, &argv[0], "r:m:f:a:")) != EOF) {
        switch (opt) {
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
            auto_rotate_disable = auto_rotate_disable ? 1 : 0;
            printf("%s(), auto_rotate_disable: %d\n", __func__, (int)auto_rotate_disable);
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

    for (i = 0; i < HCCAST_TEST_USB_TYPE_MAX; i ++){
        if (-1 != rotate)
            m_usb_cast_state[i].rotate_mode = rotate;
        if (-1 != h_flip)
            m_usb_cast_state[i].flip_mode = h_flip;            
        if (-1 != project_mode)
            m_usb_cast_state[i].project_mode = project_mode;
        if (-1 != auto_rotate_disable)
            m_usb_cast_state[i].auto_rotate_disable = auto_rotate_disable;            
    }

    usb_mirror_rotate_init();
    hccast_um_reset_video();


	return 0;
}


static int cast_usb_stop(int argc, char *argv[])
{
	int i;
	(void)argc;
	(void)argv;

    if (!m_um_test_start)
        return 0;


    for (i = 0; i < HCCAST_TEST_USB_TYPE_MAX; i ++){
        m_usb_cast_state[i].state = 0;
    }

    
    hccast_aum_stop();
    hccast_ium_stop();

    m_um_test_start = false;

	return 0;
}

static int cast_usb_preview(int argc, char *argv[])
{
    int i;
    int arg_base;
    int dst_x;
    int dst_y;
    int dst_w;
    int dst_h;

    //cmd: preview 0 0 960 540
    if (argc < 5) {
        printf("%s(), too few args: preview 0 0 960 540\n", __func__);
        return -1;
    }

    cast_usb_init(argc, argv);

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

    for (i = 0; i < HCCAST_TEST_USB_TYPE_MAX; i ++){
        m_usb_cast_state[i].src_rect.x = 0;
        m_usb_cast_state[i].src_rect.y = 0;
        m_usb_cast_state[i].src_rect.w = HCCAST_TEST_SCREEN_WIDTH;
        m_usb_cast_state[i].src_rect.h = HCCAST_TEST_SCREEN_HEIGHT;
        m_usb_cast_state[i].dst_rect.x = dst_x;
        m_usb_cast_state[i].dst_rect.y = dst_y;
        m_usb_cast_state[i].dst_rect.w = dst_w;
        m_usb_cast_state[i].dst_rect.h = dst_h;
    }
    
    return 0;
}

static int cast_usb_disable_audio(int argc, char *argv[])
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
        m_usb_cast_state[cast_type].audio_disable = audio_disable;
    } else {
        for (i = 0; i < HCCAST_TEST_USB_TYPE_MAX; i ++)
            m_usb_cast_state[i].audio_disable = audio_disable;
    }
    hccast_com_audio_disable_set(audio_disable);
    return 0;
}

static int cast_usb_info(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    int i;
    printf("\ncast usb test information:\n");    
    char *rotate_str[] = {
        "0",
        "90",
        "180",
        "270",
    };

    for (i = 0; i < HCCAST_TEST_USB_TYPE_MAX; i ++){

        if (m_usb_cast_state[i].state){
            printf("\tcast: %s. [pbp:%d][%s][%s]\n", m_usb_cast_state[i].name, 
                m_usb_cast_state[i].pbp_on, 
                m_usb_cast_state[i].dis_type == HCCAST_TEST_DIS_TYPE_HD ? "HD" : "UHD",
                m_usb_cast_state[i].dis_layer == HCCAST_TEST_DIS_LAYER_MAIN ? "main layer" : "auxp layer");

            printf("\t\trotate:[%s], flip: [%s]\n", rotate_str[m_usb_cast_state[i].rotate_mode], 
                m_usb_cast_state[i].flip_mode ? "Horizon" : "No");

            printf("\t\tsrc_rect:{%d,%d,%d,%d}, dst_rect:{%d,%d,%d,%d}\n", 
                m_usb_cast_state[i].src_rect.x, m_usb_cast_state[i].src_rect.y,
                m_usb_cast_state[i].src_rect.w, m_usb_cast_state[i].src_rect.h,
                m_usb_cast_state[i].dst_rect.x, m_usb_cast_state[i].dst_rect.y,
                m_usb_cast_state[i].dst_rect.w, m_usb_cast_state[i].dst_rect.h
                );
        }
    }

    return 0;
}



static const char help_start[] = 
    "Start USB cast\n\t\t\t"
    "start -p 1 -d 1 -l 1 -r 0 -m 0\n\t\t\t"
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

static const char help_stop[] = "Stop USB cast\n\t\t\t";

static const char help_rotate[] = 
    "Set USB cast rotate\n\t\t\t"
    "rotate -r 1 -m 1 -f 0\n\t\t\t"
    "-r set rotate angle: 0, 0 degree; 1, 90 degree; 2, 180 degree, 3. 270 degree\n\t\t\t"
    "-m enable horizon flip: 0, disable horizontal flip; 1. enable horizontal flip\n\t\t\t"
    "-f project mode: 0, Front; 1, Ceiling Front; 2, Rear; 3, Ceiling Rear\n\t\t\t"
    "-a disable auto rotate and zoom: 0, enable auto roate; 1. enable auto rotate\n\t"
;

static const char help_preview[] = 
    "Set USB cast preview(miracast/aircast/dlna..)\n\t\t\t"
    "preview {dst_rect}\n\t\t\t"
    "   {dst_rect} base on 1920*1080\n\t\t\t"
    " for example: \n\t\t\t"
    "  preview 0 0 960 540\n\t\t\t"
;

static const char help_disa[] = 
    "disable audio\n\t\t\t"
    "disa -a 0\n\t\t\t"
    "-s set cast type: \n\t\t\t"
    "   0: set miracast; 1: set aircast; 2: set dlna; No -s: set all casts\n\t\t\t"
    "-a : 1, disable; 0, enable\n\t"
;

static const char help_info[] = 
"Show USB cast information(andriod/apple)\n\t\t\t"
;

#ifdef __linux__

void usb_cast_cmds_register(struct console_cmd *cmd)
{
    console_register_cmd(cmd, "start",  cast_usb_start, CONSOLE_CMD_MODE_SELF, help_start);

    console_register_cmd(cmd, "stop",  cast_usb_stop, CONSOLE_CMD_MODE_SELF, help_stop);

    console_register_cmd(cmd, "rotate",  cast_usb_rotate, CONSOLE_CMD_MODE_SELF, help_rotate);

    console_register_cmd(cmd, "preview",  cast_usb_preview, CONSOLE_CMD_MODE_SELF, help_preview);

    console_register_cmd(cmd, "disa",  cast_usb_disable_audio, CONSOLE_CMD_MODE_SELF, help_disa);

    console_register_cmd(cmd, "info",  cast_usb_info, CONSOLE_CMD_MODE_SELF, help_info);
}

#else

CONSOLE_CMD(cast_usb, NULL, cast_usb_init, CONSOLE_CMD_MODE_SELF ,
            "enter and init hccast usb cast testing...")

CONSOLE_CMD(start, "cast_usb", cast_usb_start, CONSOLE_CMD_MODE_SELF, help_start)

CONSOLE_CMD(stop, "cast_usb", cast_usb_stop, CONSOLE_CMD_MODE_SELF, help_stop)

CONSOLE_CMD(rotate, "cast_usb", cast_usb_rotate, CONSOLE_CMD_MODE_SELF, help_rotate)

CONSOLE_CMD(preview, "cast_usb", cast_usb_preview, CONSOLE_CMD_MODE_SELF, help_preview)

CONSOLE_CMD(disa, "cast_wifi", cast_usb_disable_audio, CONSOLE_CMD_MODE_SELF, help_disa)

CONSOLE_CMD(info, "cast_usb", cast_usb_info, CONSOLE_CMD_MODE_SELF, help_info)

#endif