/*
win_um_play.c
 */
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
//#include <lvgl/lvgl.h>
#include "lvgl/lvgl.h"
#include "../lvgl/src/font/lv_font.h"

#include "menu_mgr.h"
#include "com_api.h"
#include "cast_api.h"
#include "osd_com.h"
#include "network_api.h"


static volatile bool m_win_um_play_open = false;
static lv_obj_t *m_obj_label_msg = NULL;


static void win_lable_pop_msg_open(char* lable_msg)
{
    if(m_obj_label_msg)
    {       
        lv_obj_del(m_obj_label_msg);
        m_obj_label_msg = NULL;
    }
    
    m_obj_label_msg = lv_label_create(lv_scr_act());
    lv_obj_align(m_obj_label_msg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(m_obj_label_msg, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(m_obj_label_msg, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(m_obj_label_msg, &lv_font_montserrat_22, 0);
    lv_label_set_text(m_obj_label_msg, lable_msg);
}

static void win_lable_pop_msg_close(void)
{
    if(m_obj_label_msg)
    {       
        lv_obj_del(m_obj_label_msg);
        m_obj_label_msg = NULL;
    }
}

bool win_um_play_wait_open(uint32_t timeout)
{
    uint32_t count;
    count = timeout/20;

    while(count--){
        if (m_win_um_play_open)
            break;
        api_sleep_ms(20);
    }
    printf("%s(), m_win_um_play_open(%d):%d\n", __func__, m_win_um_play_open, count);
    return m_win_um_play_open; 
}

static int win_um_play_open(void *arg)
{
    printf("%s()\n", __func__);
    m_win_um_play_open = true;
    network_stop_services();

    return API_SUCCESS;
}

static int win_um_play_close(void *arg)
{
    m_win_um_play_open = false;
    win_lable_pop_msg_close();
    win_msgbox_msg_close();
    network_start_services();
   
    return API_SUCCESS;
}

static win_ctl_result_t win_um_play_control(void *arg1, void *arg2)
{

    (void)arg2;
    control_msg_t *ctl_msg = (control_msg_t*)arg1;
    win_ctl_result_t ret = WIN_CTL_SKIP;

    if (ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_STOP || ctl_msg->msg_type == MSG_TYPE_CAST_AUSB_STOP)
    {
        ret = WIN_CTL_POPUP_CLOSE;
    }
    else if (ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_NO_DATA)
    {
        win_lable_pop_msg_open("Connect fail, reboot the phone and try again.");
    }
    else if (ctl_msg->msg_type == MSG_TYPE_CAST_IUSB_COPYRIGHT_PROTECTION)
    {
        win_msgbox_msg_open("Copyright limited", 3000, NULL, NULL);
    }
    else if (ctl_msg->msg_type == MSG_TYPE_CAST_AUSB_START_UPGRADE)
    {
        win_des_t *cur_win =  &g_win_upgrade;
        cur_win->param = (void*)(MSG_TYPE_USB_UPGRADE);
        menu_mgr_push(cur_win);
        ret = WIN_CTL_PUSH_CLOSE;
    }
    
    return ret;
}

win_des_t g_win_um_play = 
{
    .open = win_um_play_open,
    .close = win_um_play_close,
    .control = win_um_play_control,
};
