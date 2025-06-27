/*
win_um_fast.c
 */

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <semaphore.h>

//#include <lvgl/lvgl.h>
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"

#include <hcuapi/input.h>
#include <hcuapi/input-event-codes.h>


//#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"
#include "osd_com.h"
//#include "network_api.h"
#include "screen.h"
#include "mul_lang_text.h"
#include "hcstring_id.h"
#include "setup.h"
#include "factory_setting.h"
#include "win_cast_root.h"


#ifdef USB_MIRROR_FAST_SUPPORT

typedef enum{
    UM_TYPE_NONE,
    UM_YTPE_I, //apple device    
    UM_YTPE_A, //android devide
}UM_TYPE_E;

lv_obj_t *ui_um_fast = NULL;
static volatile bool m_close_by_key = false;
lv_group_t *m_um_fast_group = NULL;
static sem_t m_um_fast_wait_sem;
static volatile bool m_um_service_onoff = false;
static volatile bool m_um_seriver_start = false;

static pthread_mutex_t m_um_fast_stop_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t m_um_fast_stop_cond = PTHREAD_COND_INITIALIZER;

static char m_um_type = UM_TYPE_NONE;

static void event_handler(lv_event_t * e);


static uint32_t m_hotkey[] = {KEY_POWER, KEY_VOLUMEUP, \
                    KEY_VOLUMEDOWN, KEY_MUTE, KEY_ROTATE_DISPLAY, KEY_FLIP,KEY_CAMERA_FOCUS,KEY_FORWARD,KEY_BACK,KEY_HOME};

static int win_um_fast_open(void *arg)
{
    m_um_fast_group = lv_group_create();
    key_set_group(m_um_fast_group);
    lv_group_add_obj(m_um_fast_group, ui_um_fast);
    lv_group_focus_obj(ui_um_fast);

    set_display_zoom_when_sys_scale();
    api_set_display_aspect(DIS_TV_16_9, DIS_PILLBOX);    

    api_hotkey_enable_set(m_hotkey, sizeof(m_hotkey)/sizeof(m_hotkey[0]));

    //extern void api_set_i2so_gpio_mute_auto(void);
    api_set_i2so_gpio_mute_auto();
    m_close_by_key = false;

    return API_SUCCESS;
}

static int win_um_fast_close(void)
{
    win_msgbox_msg_close();
    if (m_close_by_key)
    {
        if (UM_YTPE_I == m_um_type){
            hccast_ium_stop_mirroring();
        }
        else if (UM_YTPE_A == m_um_type){
            hccast_aum_stop_mirroring();
        }
    }

    if (m_um_fast_group){
        lv_group_remove_all_objs(m_um_fast_group);
        lv_group_del(m_um_fast_group);
        lv_group_set_default(NULL);
    }

    api_hotkey_disable_clear();
    //recover the dispaly aspect.
    api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE);


    return API_SUCCESS;
}

static void event_handler(lv_event_t * e)
{
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if (ta == ui_um_fast){
        if(event == LV_EVENT_SCREEN_LOAD_START) {
            win_um_fast_open(NULL);
        } else if (event == LV_EVENT_SCREEN_UNLOAD_START) {
            win_um_fast_close();
        } 
        else if (event == LV_EVENT_KEY){
            lv_indev_t *key_indev = lv_indev_get_act();
            if (key_indev->proc.state != LV_INDEV_STATE_PRESSED)
                return;

            uint32_t value = lv_indev_get_key(key_indev);
            if (value == LV_KEY_ESC){
                //exit current menu and back to last menu.
                m_close_by_key = true;
                api_scr_go_back();

            }else if(value == FUNC_KEY_SCREEN_ROTATE){
                win_cast_mirror_rotate_switch();
            }
        }
    }
}


static void win_um_fast_control(void *arg1, void *arg2)
{
    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;

    if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_STOP ||
        ctl_msg->msg_type == MSG_TYPE_CAST_AUSB_STOP){
        api_scr_go_back();
    }else if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_START ||
        ctl_msg->msg_type == MSG_TYPE_CAST_AUSB_START){

    }else if(ctl_msg->msg_type == MSG_TYPE_AUM_DEV_ADD ||
        ctl_msg->msg_type == MSG_TYPE_IUM_DEV_ADD){


    #ifdef MIRROR_ES_DUMP_SUPPORT
        extern bool api_mirror_dump_enable_get(char* folder);
        char dump_folder[64];

        if (USB_STAT_MOUNT != mmp_get_usb_stat())
        {
            printf("%s(), line: %d. No disk, disable dump!\n", __func__, __LINE__);
            hccast_um_es_dump_stop();
            return;
        }
        printf("%s(), line: %d. Statr USB mirror ES dump!\n", __func__, __LINE__);
        if (api_mirror_dump_enable_get(dump_folder)){
            hccast_um_es_dump_start(dump_folder);
        } else {
            hccast_um_es_dump_stop();
        }
    #endif    
    }else if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_NO_DATA) {
        win_msgbox_msg_open(STR_IUSB_NO_DATA, 0, NULL, NULL);
    }else if(ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_DEVICE_REMOVE) {
        win_msgbox_msg_close();
    }

}

static void *_um_monitor_task(void *arg)
{
    for(;;){
        sem_wait(&m_um_fast_wait_sem);
        if (m_um_service_onoff){
            printf("%s() start 1!\n", __func__);
            cast_usb_mirror_start();
            pthread_mutex_lock(&m_um_fast_stop_mutex);
            m_um_seriver_start = true;
            pthread_mutex_unlock(&m_um_fast_stop_mutex);
            printf("%s(), cast_usb_mirror_start!\n", __func__);
        }else{
            printf("%s() stop 1!\n", __func__);
            pthread_mutex_lock(&m_um_fast_stop_mutex);
            cast_usb_mirror_stop();
            m_um_seriver_start = false;
            pthread_cond_signal(&m_um_fast_stop_cond);
            pthread_mutex_unlock(&m_um_fast_stop_mutex);
            printf("%s(), cast_usb_mirror_stop!\n", __func__);
        }
    }
    return NULL;
}

void ui_um_fast_init(void)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    screen_entry_t um_fast_entry;

    hccast_ium_set_resolution(1280, 720);
    ui_um_fast = lv_obj_create(NULL);
    
    lv_obj_clear_flag(ui_um_fast, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_um_fast, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_opa(ui_um_fast, LV_OPA_TRANSP, 0);


    um_fast_entry.screen = ui_um_fast;
    um_fast_entry.control = win_um_fast_control;
    api_screen_regist_ctrl_handle(&um_fast_entry);

    sem_init(&m_um_fast_wait_sem, 0, 0);
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x1000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, _um_monitor_task, NULL)) 
    {
        pthread_attr_destroy(&attr);
        return;
    }    
    
    pthread_attr_destroy(&attr);
}


static volatile lv_obj_t *m_pre_scr = NULL;

//You can add screen that can run usb mirror menu here.
#define UM_CAN_PLAY(screen) (\
    (screen == ui_um_play   || \
     screen == ui_um_fast  || \
     screen == main_page_scr) \
)       

static void _um_service_notify_onoff(bool on)
{
    m_um_service_onoff = on;
    sem_post(&m_um_fast_wait_sem);
}

//start/stop usb mirror service in sepeical menu automatically
static bool _um_service_onoff_by_menu(lv_obj_t *scr)
{
    if (m_pre_scr == scr)
        return false;

    m_pre_scr = scr;
    if (UM_CAN_PLAY(scr)){
        _um_service_notify_onoff(true);
        return true;
    }else{
        _um_service_notify_onoff(false);
        return false;
    }
}


bool um_service_off_by_menu(lv_obj_t *scr)
{
    if (!UM_CAN_PLAY(scr)){
        _um_service_notify_onoff(false);
        return true;
    }
    return false;
}

bool um_service_on_by_menu(lv_obj_t *scr)
{
    if (UM_CAN_PLAY(scr)){
        _um_service_notify_onoff(true);
        return true;
    }
    return false;
}

void um_service_wait_exit(void)
{
    pthread_mutex_lock(&m_um_fast_stop_mutex);
    while (m_um_seriver_start)
        pthread_cond_wait(&m_um_fast_stop_cond, &m_um_fast_stop_mutex);
    pthread_mutex_unlock(&m_um_fast_stop_mutex);
}

static void _switch_to_fast_um(void)
{
    //Here can add some process before open fast usb mirror. 
    //For example, stop media play, stop wifi mirror, etc.
    
    _ui_screen_change(ui_um_fast,0,0);
}

void ui_um_fast_proc(uint32_t msg_type)
{
    lv_obj_t *screen = NULL;
    screen = lv_scr_act();

    //start usb mirror service if necessary.
    //_um_service_onoff_by_menu(screen);

    //start usb mirror if necessary
    if (msg_type == MSG_TYPE_CAST_IUSB_START)
        m_um_type = UM_YTPE_I;
    else if (msg_type == MSG_TYPE_CAST_AUSB_START)
        m_um_type = UM_YTPE_A;
    else
        return;

    //It the screen is usb mirror screen already, do not enter again.
    if (screen != ui_um_play && 
        screen != ui_um_fast)
        _switch_to_fast_um();
}

#endif
