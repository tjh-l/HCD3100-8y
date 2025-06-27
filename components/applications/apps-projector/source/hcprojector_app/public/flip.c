#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <hcuapi/dis.h>
#include <hcuapi/vidmp.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ffplayer.h>
#include "com_api.h"

#include "lv_drivers/display/fbdev.h"
#include "screen.h"
#include "factory_setting.h"
#include "../channel/local_mp/mp_zoom.h"
#include "../channel/local_mp/media_player.h"
#include <hcuapi/lcd.h>
extern void *mp_get_cur_player_hdl(void);

#ifdef LCD_ROTATE_SUPPORT
static void lcd_set_flip_mode(flip_mode_e mode)
{
	int fd = -1;
	fd = open("/dev/lcddev",O_RDWR);
	if(fd > 0) {
		ioctl(fd, LCD_SET_ROTATE, mode);
		close(fd);
	}
}
#endif

void get_rotate_by_flip_mode(flip_mode_e mode,
                             int *p_rotate_mode ,
                             int *p_h_flip ,
                             int *p_v_flip)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    
    switch(mode)
    {
        case FLIP_MODE_CEILING_REAR:
        {
            //printf("180\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 0;
            v_flip = 0;
            break;
        }

        case  FLIP_MODE_FRONT:
        {
            //printf("h_mirror\n");
            rotate = ROTATE_TYPE_0;
            h_flip = 1;
            v_flip = 0;
            break;
        }

        case FLIP_MODE_CEILING_FRONT:
        {
           // printf("v_mirror\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 1;
            v_flip = 0;
            break;

        }
        case FLIP_MODE_REAR:
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

int set_flip_mode(flip_mode_e mode)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    void *player = api_ffmpeg_player_get();
    /*it will register hcplayer in media_play/dina/iptv case*/
    media_handle_t *mp_hdl=mp_get_cur_player_hdl();
    int fbdev_rotate;

    int init_rotate = api_get_screen_init_rotate();
    int init_h_flip = api_get_screen_init_h_flip();
    int init_v_flip = api_get_screen_init_v_flip();
#ifdef LCD_ROTATE_SUPPORT
    lcd_set_flip_mode(mode);
#ifdef MULTI_OS_SUPPORT
    avparam_set_rotate_param(mode , 0 , 0);
#endif	
    return 0;
#endif
	
    get_rotate_by_flip_mode(mode, &rotate , &h_flip , &v_flip);
    api_transfer_rotate_mode_for_screen(init_rotate , init_h_flip , init_v_flip ,
                                        &rotate , &h_flip , &v_flip , &fbdev_rotate);
    fbdev_set_rotate(fbdev_rotate, h_flip, v_flip);
    api_set_flip_arg(fbdev_rotate, h_flip, v_flip);

#ifdef MULTI_OS_SUPPORT
    avparam_set_rotate_param(fbdev_rotate , h_flip , v_flip);
#endif
    // due to fbdev_flush do not work in lvgl top layer,so invalidate it by user
    lv_obj_invalidate(lv_layer_top());
    if (player != NULL)
    {
        if(mp_hdl)
        {
            media_change_rotate_mirror_type(mp_hdl,rotate,h_flip);
            /*local player need to set mp_hld member when change rotate*/
        }
        else
        {
            hcplayer_change_rotate_mirror_type(player, rotate, h_flip);
        }
    }

    if (NULL != mp_hdl && mp_hdl->type == MEDIA_TYPE_MUSIC) {
        app_reset_mainlayer_pos(rotate, h_flip);
    }

    printf("rotate = %u mode =%d h_flip =%d\n",rotate,mode, h_flip);
     //set hdmi rx/cvbs rx flip mode
#ifdef HDMIIN_SUPPORT     
#ifdef HDMI_SWITCH_SUPPORT
    if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI||projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI2)
#else
    if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI)
#endif
    {
        hdmi_rx_set_flip_mode(rotate, h_flip);
    }
    else 
#endif		
	if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_CVBS){
        #ifdef CVBSIN_SUPPORT
        cvbs_rx_set_flip_mode(rotate , h_flip);
        #endif
        //cvbs_rx_stop();
        //cvbs_rx_start();
    }
    return 0;
}





