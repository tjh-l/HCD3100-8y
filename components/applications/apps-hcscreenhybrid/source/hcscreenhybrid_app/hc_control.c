//The control center, process the message and key

#include <getopt.h>
#include <pthread.h>
#include <hcuapi/gpio.h>
#if defined(SUPPORT_FFPLAYER) || defined(__linux__)
#include <ffplayer.h>
#endif
#ifdef __linux__
#include <linux/input.h>
#else
#include <hcuapi/input.h>
#endif
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>

#include "com_api.h"
#include "menu_mgr.h"
#include "network_api.h"
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_media.h>
#include "cast_api.h"
#include "lvgl/lvgl.h"
#include "../lvgl/src/misc/lv_types.h"
#include "lv_drivers/display/fbdev.h"
#include "gpio_ctrl.h"
#include "data_mgr.h"
#include "osd_com.h"
#include "tv_sys.h"

#ifdef __HCRTOS__
#include <kernel/lib/fdt_api.h>
#endif

#define KEY_CHECK_SUPPORT
#define KEY_MODE_SWITCH_TIME (60*1000)

static volatile int m_sys_reset = 0;

#ifdef AIRP2P_SUPPORT
int airp2p_mode_switch_status_check(void)
{
    app_data_t  *app_data = data_mgr_app_get();
    int connected = 1;
    int current_scene = hccast_get_current_scene();

    if (app_data->airp2p_en)
    {
        if ((current_scene == HCCAST_SCENE_NONE) \
            && !hccast_air_connect_state_get() \
            && !api_get_upgrade_state())
        {
            connected = 0;
        }
    }
    else
    {   
        //mean current is sta mode.        
        if (!hccast_wifi_mgr_get_hostap_status())
        {
            connected = 1;
        }
        else
        {
            if (((current_scene == HCCAST_SCENE_NONE) \
                || (((current_scene == HCCAST_SCENE_DLNA_PLAY) \
                || (current_scene == HCCAST_SCENE_AIRCAST_PLAY)) \
                && (hccast_media_get_status() == HCCAST_MEDIA_STATUS_STOP))) \
                && !hccast_air_connect_state_get() \
                && !hostap_get_connect_count() \
                && !api_get_upgrade_state())
            {
                connected = 0;
            }   
        }
    }

    return connected;
}

static void *airp2p_mode_switch_task(void *arg)
{
    control_msg_t msg = {0};
    app_data_t  *app_data = data_mgr_app_get();
    static unsigned long m_mode_switch_tick = 0;
    
    while(1)
    {
        if (app_data->airp2p_mode_switch == 2)
        {
            if (airp2p_mode_switch_status_check() == 0)
            {
                if (m_mode_switch_tick == 0)
                {
                    m_mode_switch_tick = api_get_time_ms();
                    printf("Airp2p mode will switch after %dms\n", KEY_MODE_SWITCH_TIME);
                }

                if ((api_get_time_ms() - m_mode_switch_tick) > KEY_MODE_SWITCH_TIME)
                {
                    printf("\n\nBegin to switch mode.\n");
                    if (data_mgr_cast_airp2p_get())
                    {
                        data_mgr_cast_airp2p_set(0);
                        msg.msg_type = MSG_TYPE_KEY_TRIGER_REBOOT;
                        api_control_send_msg(&msg);
                    }
                    else
                    {
                        data_mgr_cast_airp2p_set(1);
                        msg.msg_type = MSG_TYPE_KEY_TRIGER_REBOOT;
                        api_control_send_msg(&msg);
                    } 

                    break;
                }
            }
            else
            {
                m_mode_switch_tick = 0;
            }
        }    
        else
        {
            m_mode_switch_tick = 0;
        }

        usleep(1000*1000);
    }
    
    return NULL;
}

void airp2p_mode_switch_detect(void)
{
    pthread_t tid;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &attr, airp2p_mode_switch_task, NULL) < 0)
    {
        printf("Create airp2p mode switch task error.\n");
    }
    
    pthread_attr_destroy(&attr);
}
#endif

static void key_cast_rotate(bool is_active)
{
    int rotate_type = ROTATE_TYPE_0;
    control_msg_t msg = {0};

    printf("%s(), line:%d. switch rotate!\n", __func__, __LINE__);

 #ifdef AIRP2P_SUPPORT   
    app_data_t  *app_data = data_mgr_app_get();
    
    if(!api_get_key_rotate_enable())
    {
        if (data_mgr_cast_airp2p_get())
        {
            data_mgr_cast_airp2p_set(0);
            msg.msg_type = MSG_TYPE_KEY_TRIGER_REBOOT;
            api_control_send_msg(&msg);
            return ;
        }
        else
        {
            data_mgr_cast_airp2p_set(1);
            msg.msg_type = MSG_TYPE_KEY_TRIGER_REBOOT;
            api_control_send_msg(&msg);
            return ;
        }        
    }
    else
#endif    
    {
        if (data_mgr_cast_rotation_get()){
            rotate_type = ROTATE_TYPE_0;
            data_mgr_cast_rotation_set(0);
            // fbdev_set_rotate(0, 0, 0);
        }
        else{
            rotate_type = ROTATE_TYPE_270;
            data_mgr_cast_rotation_set(1);
            // fbdev_set_rotate(90, 0, 0);
        }
    }    
#if defined(SUPPORT_FFPLAYER) || defined(__linux__)
    //api_logo_reshow();
    void *player = api_ffmpeg_player_get();
    if(player !=NULL && !is_active){
        hcplayer_change_rotate_type(player, rotate_type);
        hcplayer_change_mirror_type(player, MIRROR_TYPE_NONE);
    }
#endif

#ifdef USBMIRROR_SUPPORT 
    hccast_um_param_t um_param = {0};
    hccast_um_param_get(&um_param);
    if (data_mgr_cast_rotation_get())
        um_param.screen_rotate_en = 1;
    else
        um_param.screen_rotate_en = 0;
    hccast_um_param_set(&um_param);
    hccast_um_reset_video();
#endif

}

#define INPUT_DEV_MAX	10
typedef struct
{
    int first_press;
    int pressed;
    int tick_last;
    int key_code;
}gpio_key_info_t;

static int set_fd_nonblock(int fd)
{
    int flag = fcntl(fd,F_GETFL,0);
    flag |= O_NONBLOCK;
    return fcntl(fd,F_SETFL,flag);
}

static int set_fd_block(int fd)
{
    int flag = fcntl(fd,F_GETFL,0);
    flag |= O_NONBLOCK;
    return fcntl(fd,F_SETFL,flag);
}

static void *key_monitor_task(void *arg)
{
    control_msg_t msg = {0};
    struct pollfd *pfd;
    int cnt = 0;
    struct input_event *t;
    struct input_event key_event;
    int input_cnt = 0;
    uint8_t input_name[32];
    int key_fds[INPUT_DEV_MAX] = {0,};
    int i;
    gpio_key_info_t key_info[INPUT_DEV_MAX] = {0};
    char name[256] = "Unkown";
    int fd = -1;
    

#ifdef __linux__

    for (i = 0; i < INPUT_DEV_MAX; i++)
    {
        sprintf(input_name, "/dev/input/event%d", i);
        fd = open(input_name, O_RDONLY);
        if (fd < 0)
        {
            break;
        }

        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        if (strstr(name, "gpio-keys"))
        {
            key_fds[0] = fd;
            input_cnt = 1;//just only one input device.
            break;
        }

        close(fd);
    }
    
#else

    sprintf(input_name, "/dev/input/event0");
    fd = open(input_name, O_RDONLY);
    if (fd > 0)
    {
        key_fds[0] = fd;
        input_cnt = 1;//just only one input device.
    }
    
#endif
    
    if (0 == input_cnt)
    {
    	printf("%s(), line:%d. No Input device!!!\n", __func__, __LINE__);
    	return NULL;
    }

    pfd = (struct pollfd*)malloc(sizeof(struct pollfd)*input_cnt);
    memset(pfd, 0, sizeof(struct pollfd)*input_cnt);
    t = &key_event;
    
    for (i = 0; i< input_cnt; i ++)
    {
        pfd[i].fd = key_fds[i];
        pfd[i].events = POLLIN | POLLRDNORM;

        //Get key bootup state.
        set_fd_nonblock(key_fds[i]);
        cnt = read(key_fds[i], t, sizeof(struct input_event));
        if (cnt == sizeof(struct input_event))
        {
            if ((t->type == EV_KEY) && (t->value == 1))
            {
                key_info[i].first_press = 1;
                printf("event[%d] first_press :%d\n", i, key_info[i].first_press);
            }
        }
        set_fd_block(key_fds[i]);
    }

    printf("%s(), input device num: %d.\n", __func__, input_cnt);

    while (1) 
    {
        if (poll(pfd, input_cnt, -1) <= 0)
        {
            continue;
        }

        for (i = 0; i < input_cnt; i ++)
        {
            if (pfd[i].revents != 0)
            {
                cnt = read(key_fds[i], t, sizeof(struct input_event));
                if (cnt != sizeof(struct input_event))
                {
                	printf("cnt = %d, err(%d):%s\n", cnt, errno, strerror(errno));
                	api_sleep_ms(10);
                }

                //printf("t->type :%d, t->value %d, t->code :%d\n", t->type, t->value, t->code);
                
                if (t->type == EV_KEY)
                {
                    if (t->value == 1)// pressed
                    {
                        if (key_info[i].tick_last == 0)
                        {
                            key_info[i].tick_last = api_get_time_ms();
                        }
                
                        key_info[i].pressed = 1;
                    }
                    else if (t->value == 0) //released
                    {  
                        if (key_info[i].pressed)
                        {
                            key_cast_rotate(0);
                        }   
                        
                        key_info[i].pressed = 0;
                        key_info[i].tick_last = 0;
                    }
                    else if (t->value == 2) //repeat
                    {
                        if (key_info[i].pressed)
                        {
                            if ((api_get_time_ms() - key_info[i].tick_last) > 2000)
                            {
                                msg.msg_type = MSG_TYPE_KEY_TRIGER_RESET;
                                api_control_send_msg(&msg); 
                                goto EXIT;
                            }
                        }      
                    }
                }
            }
        }
        	
        api_sleep_ms(5);
    }

EXIT:

    for (i = 0; i < input_cnt; i ++)
    {
        if (key_fds[i] > 0)
        close(key_fds[i]);
    }

    free(pfd);
    
    return NULL;
}

static void key_monitor()
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, key_monitor_task, NULL)) {
        pthread_attr_destroy(&attr);
        return;
    }
}

static void sys_reboot_func(void *user_data)
{
    (void)user_data;
    api_system_reboot();
    m_sys_reset = 0;
}

static void sys_standby_func(void *user_data)
{
    (void)user_data;

    api_system_standby();
}

static bool m_wifi_plug_out = false;
static win_ctl_result_t common_msg_proc(control_msg_t *msg)
{
    win_ctl_result_t ctl_result = WIN_CTL_NONE;

    switch(msg->msg_type)
    {
        case MSG_TYPE_KEY_TRIGER_RESET:
            m_sys_reset = 1;
            printf("%s(): factory reset!!\n", __func__);
            data_mgr_factory_reset();
            win_msgbox_msg_open("System reset, then reboot ...", 2000, sys_reboot_func, NULL);
            break;
        case MSG_TYPE_ENTER_STANDBY:
            win_msgbox_msg_open("Entering system standby ...", 2000, sys_standby_func, NULL);
            break;
        case MSG_TYPE_CAST_IUSB_NEED_TRUST:
            win_msgbox_msg_open("Please select \"Trust\" to \nstart mirror cast", 5000, NULL, NULL);
            break;
        case MSG_TYPE_CAST_IUSB_DEV_REMOVE:
            win_msgbox_msg_close();
            break;
        case MSG_TYPE_USB_WIFI_PLUGIN:
            if (!m_wifi_plug_out){
                network_connect();
                m_wifi_plug_out = false;
            }
            break;
        case MSG_TYPE_USB_WIFI_PLUGOUT:
        #ifdef __HCRTOS__
            //reboot...
            network_plugout_reboot();
            hccast_stop_services();
            win_msgbox_msg_open("WiFi plug out, reboot now ...", 2000, sys_reboot_func, NULL);
        #else
            hccast_stop_services();
        #endif
            m_wifi_plug_out = true;
            break;

        case MSG_TYPE_KEY_TRIGER_REBOOT:
            win_msgbox_msg_open("Switch mode, reboot now ...", 2000, sys_reboot_func, NULL);
            break;
        case MSG_TYPE_NETWORK_WIFI_MAY_LIMITED:
            app_wifi_set_limited_internet(true);
            printf("%s %d: MSG_TYPE_NETWORK_WIFI_MAY_LIMITED\n",__func__,__LINE__);
            break;

        default:
            break;
    }

    return ctl_result;
}

#ifdef DRAW_UI_SYNC
void hc_control_process(void)
{
    control_msg_t ctl_msg;
    win_des_t *cur_win = NULL;
    win_ctl_result_t ctl_result = WIN_CTL_NONE;
    int ret = -1;

    do
    {
        //get message
        ret = api_control_receive_msg(&ctl_msg);
        if (0 != ret)
        {
#if 0
            //get key
            ret = api_get_key();
#endif
            if (0 != ret)
                break;
        }

        cur_win = menu_mgr_get_top();
        if (cur_win)
        {
            ctl_result = cur_win->control((void*)(&ctl_msg), NULL);

            if (ctl_result == WIN_CTL_PUSH_CLOSE ||
                ctl_result == WIN_CTL_POPUP_CLOSE )
            {
                cur_win->close((void*)(ctl_result));

                if (ctl_result == WIN_CTL_POPUP_CLOSE)
                {
                    //popup current menu, back to parent menu.
                    menu_mgr_pop();
                }

                cur_win = menu_mgr_get_top();
                if (cur_win)
                {
                    printf("open next win!\n");
                    cur_win->open(cur_win->param);
                }
            }
            else if (ctl_result == WIN_CTL_SKIP)
            {
                //menu do not process the message or key, may process here
            }
        }

        ctl_result = common_msg_proc(&ctl_msg);

    }
    while(0);

}
#else
static void *hc_control_task(void *arg)
{
    control_msg_t ctl_msg;
    win_des_t *cur_win = NULL;
    win_ctl_result_t ctl_result = WIN_CTL_NONE;
    int ret = -1;

    while(1)
    {
        do
        {
            //get message
            ret = api_control_receive_msg(&ctl_msg);
            if (0 != ret)
            {
#if 0
                //get key
                ret = api_get_key();
#endif
                if (0 != ret)
                    break;
            }

            cur_win = menu_mgr_get_top();
            if (cur_win)
            {
                ctl_result = cur_win->control((void*)(&ctl_msg), NULL);

                if (ctl_result == WIN_CTL_PUSH_CLOSE ||
                    ctl_result == WIN_CTL_POPUP_CLOSE )
                {
                    cur_win->close((void*)(ctl_result));

                    if (ctl_result == WIN_CTL_POPUP_CLOSE)
                    {
                        //popup current menu, back to parent menu.
                        menu_mgr_pop();
                    }

                    cur_win = menu_mgr_get_top();
                    if (cur_win)
                    {
                        printf("open next win!\n");
                        cur_win->open(cur_win->param);
                    }
                }
                else if (ctl_result == WIN_CTL_SKIP)
                {
                    //menu do not process the message or key, may process here
                }
            }
        }
        while(0);
        api_sleep_ms(10);//sleep 10ms
    }
    return NULL;
}
#endif // DRAW_UI_SYNC

void hc_control_init()
{
    win_des_t *cur_win;

#ifdef NETWORK_SUPPORT
    //network_connect();
#endif

    menu_mgr_init();
#ifdef KEY_CHECK_SUPPORT    
    key_monitor();
#endif    

#ifdef AIRP2P_SUPPORT
    airp2p_mode_switch_detect();
#endif

    //Set the screen to transprent
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(lv_scr_act(), LV_OPA_TRANSP, 0);

    osd_obj_com_set();

    cur_win = &g_win_cast_root;
    //cur_win = &g_win_dlna_play;

#ifdef AIRCAST_SUPPORT
    hccast_air_service_init(hccast_air_callback_event);
#ifdef SOC_HC15XX
    hccast_air_set_resolution(1920, 1080, 30);
#else
    hccast_air_set_resolution(1920, 1080, 60);
#endif    
#endif

#ifdef USBMIRROR_SUPPORT
    cast_usb_mirror_init();
    cast_usb_mirror_start();
#endif

    cur_win->open(cur_win->param);
    menu_mgr_push(cur_win);

    //Enter upgrade window if there is upgraded file in USB-disk(hc_upgradexxxx.bin)
    sys_upg_usb_check(5000);

#if 0
    //create the netwrok init task
    memset(&attr, 0, sizeof(pthread_attr_t));
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, hc_network_init_task, NULL))
    {
        return;
    }
    hc_network_monitor_start();
#endif

#ifndef DRAW_UI_SYNC
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, hc_control_task, NULL))
    {
        pthread_attr_destroy(&attr);
        return;
    }

    pthread_attr_destroy(&attr);
#endif

}


