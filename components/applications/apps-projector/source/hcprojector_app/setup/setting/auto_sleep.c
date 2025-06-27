#include "screen.h"
#include "setup.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include "factory_setting.h"
#include "mul_lang_text.h"
#include "com_api.h"
#include "osd_com.h"

//define for QT test is to shorten the test time.
//#define FOR_QT_TEST

#ifdef FOR_QT_TEST
#define AUTO_SLEEP_UNIT  (70000) //70 seconds
#else
#define AUTO_SLEEP_UNIT  (3600000) // 1 hour
#endif

#define COUNT_DOWN 60
int auto_sleep_vec[] = {STR_OFF, LINE_BREAK_STR, STR_AUTO_SLEEP_1H, LINE_BREAK_STR, STR_AUTO_SLEEP_2H, LINE_BREAK_STR, STR_AUTO_SLEEP_3H,
LINE_BREAK_STR, BLANK_SPACE_STR, LINE_BREAK_STR, BLANK_SPACE_STR, BTNS_VEC_END};

static lv_obj_t *sleep_obj = NULL;
static lv_obj_t *auto_sleep_prev_obj = NULL;
static lv_timer_t *auto_sleep_sure_timer = NULL;
static lv_timer_t *auto_sleep_timer = NULL;
static int countdown = COUNT_DOWN;

static void auto_sleep_timer_handle(lv_timer_t *tiemr_);
static void auto_sleep_btnsmatrix_event(lv_event_t *e);
void auto_sleep_widget(lv_obj_t *btn);
static bool autosleep_is_valid(void);

extern void btnmatrix_event(lv_event_t* e, btn_matrix_func f);
extern lv_obj_t *new_widget_(lv_obj_t*, int title, int *,uint32_t index, int len, int w, int h);

void auto_sleep_widget(lv_obj_t *btn){
    lv_obj_t * obj = new_widget_(btn, STR_AUTO_SLEEP, auto_sleep_vec,projector_get_some_sys_param(P_AUTOSLEEP), 
        HC_ARRAY_SIZE(auto_sleep_vec),0,0);
    lv_obj_add_event_cb(lv_obj_get_child(obj, 1), auto_sleep_btnsmatrix_event, LV_EVENT_ALL, btn);
}

static void auto_sleep_btnsmatrix_event(lv_event_t *e){
    btnmatrix_event(e, set_auto_sleep);
}

static void sure_sleep_timer_exit(void)
{
    if (auto_sleep_sure_timer)
        lv_timer_del(auto_sleep_sure_timer);
    auto_sleep_sure_timer = NULL;

    if (sleep_obj)
        lv_obj_del(sleep_obj);
    sleep_obj = NULL;

    if (auto_sleep_prev_obj)
        lv_group_focus_obj(auto_sleep_prev_obj);
    auto_sleep_prev_obj = NULL;

}

static void auto_sleep_sure_timer_handle(lv_timer_t *timer_){
    
    if(countdown==0){
        printf("Enter %ld(S) auto sleep!\n", auto_sleep_timer->period/1000);

        if (auto_sleep_timer)
             lv_timer_del(auto_sleep_timer);
        auto_sleep_timer = NULL;

        sure_sleep_timer_exit();

        enter_standby();
        return;
    }
    lv_obj_t *label = (lv_obj_t*)timer_->user_data;
    lv_label_set_text_fmt(label, "%d seconds to shutdown,\n press any key to cancel", countdown--);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_26, 0);
}

static void auto_sleep_sure_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_KEY){
        sure_sleep_timer_exit();
    }

}

static void auto_sleep_timer_handle(lv_timer_t *timer_){//关机前一分钟弹出倒数计时，从60到0，按任何键结束计时
    
    if(autosleep_is_valid() == false)
    {
        return;
    }
    countdown = COUNT_DOWN;

    sleep_obj = lv_obj_create(lv_layer_top());
    lv_obj_set_style_text_color(sleep_obj, lv_color_white(),0);
    lv_obj_set_size(sleep_obj, LV_PCT(33), LV_PCT(33));
    lv_obj_center(sleep_obj);
    set_pad_and_border_and_outline(sleep_obj);
    lv_obj_set_style_pad_ver(sleep_obj, 0, 0);
    lv_obj_set_style_bg_color(sleep_obj, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_bg_opa(sleep_obj, LV_OPA_90, 0);
   
    lv_obj_t* label = lv_label_create(sleep_obj);
    lv_obj_center(label);
    if (!auto_sleep_sure_timer)
        auto_sleep_sure_timer = lv_timer_create(auto_sleep_sure_timer_handle, 1000, label);
    lv_timer_set_repeat_count(auto_sleep_sure_timer, COUNT_DOWN+1);
    lv_timer_ready(auto_sleep_sure_timer);
   
    lv_group_add_obj(lv_group_get_default(), sleep_obj);
    auto_sleep_prev_obj = lv_group_get_focused(lv_group_get_default());
    lv_group_focus_obj(sleep_obj);
    lv_obj_add_event_cb(sleep_obj, auto_sleep_sure_event_handle, LV_EVENT_ALL, NULL);
    

}

void autosleep_reset_timer(void)
{
    if(auto_sleep_timer)
    {
        lv_timer_reset(auto_sleep_timer);
        sure_sleep_timer_exit();
    }
}

static bool autosleep_is_valid(void)
{
    lv_obj_t *screen;
    bool valid = true;

    if (sys_media_playing())
        valid = false;
    else
        valid = true;

    if (false == valid)
        autosleep_reset_timer();

    return valid;
}

int set_auto_sleep(int mode){
    printf("set auto >> %d \n", mode);
    projector_set_some_sys_param(P_AUTOSLEEP,mode);

    if(mode == AUTO_SLEEP_OFF){
        if(auto_sleep_timer){
            lv_timer_del(auto_sleep_timer);
            auto_sleep_timer = NULL;
        }
        return 0;
    }
    if(!auto_sleep_timer){
        auto_sleep_timer = lv_timer_create(auto_sleep_timer_handle, 1*AUTO_SLEEP_UNIT, NULL);//3600000
        lv_timer_set_repeat_count(auto_sleep_timer, -1);
    }

    switch (mode)
    {
    case AUTO_SLEEP_ONE_HOUR:
        lv_timer_set_period(auto_sleep_timer, 1*AUTO_SLEEP_UNIT);
        break;
    case AUTO_SLEEP_TWO_HOURS:
        lv_timer_set_period(auto_sleep_timer, 2*AUTO_SLEEP_UNIT);
        break;
    case AUTO_SLEEP_THREE_HOURS:
    default:
        lv_timer_set_period(auto_sleep_timer, 3*AUTO_SLEEP_UNIT);
        break;
    }
    lv_timer_reset(auto_sleep_timer);
    return 1;
}
