#include "src/core/lv_obj.h"
#include "src/font/lv_symbol_def.h"
#include "src/misc/lv_anim.h"
#include "src/misc/lv_area.h"
#include "src/misc/lv_color.h"
#include <unistd.h>
#include "sys/unistd.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../screen.h" 
#include "com_api.h"
#include "osd_com.h"
#include "pmu.h"
#include "app_log.h"

lv_obj_t *power_label=NULL;
static lv_obj_t *charge_label=NULL;
static lv_style_t style;
static lv_style_t charge_style;
static lv_anim_t power_anim;

static lv_timer_t *timer1=NULL;
static int state=0;

static void msg_timer_handle(lv_timer_t *timer_){
    lv_obj_t *obj = (lv_obj_t*)timer_->user_data;
    lv_obj_del(obj);
    state=0;
}

lv_obj_t* lvgl_msgbox_charger(int color, char* str){
    lv_obj_t *con = lv_obj_create(lv_layer_top());
    // lv_obj_set_style_text_color(con, lv_color_white(), 0);
    lv_obj_set_style_text_color(con, lv_palette_main(color), 0);
    lv_obj_set_size(con, LV_SIZE_CONTENT, LV_PCT(10));
    lv_obj_center(con);
    lv_obj_set_style_bg_color(con, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
    lv_obj_set_scrollbar_mode(con, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(con, LV_OPA_100, 0);
    lv_obj_set_style_border_width(con, 1, 0);
    lv_obj_set_style_border_color(con, lv_palette_main(color), 0);
    lv_obj_set_style_outline_width(con, 0, 0);
    lv_obj_set_style_radius(con, 10, 0);

    lv_obj_t *label = lv_label_create(con);
    lv_obj_center(label);
    lv_label_set_text(label, str);
    lv_obj_set_style_text_font(label, &LISTFONT_3000, 0);

    lv_timer_t *timer = lv_timer_create(msg_timer_handle, 3000, con);
    lv_timer_set_repeat_count(timer, 1);
    lv_timer_reset(timer);
    return con;
}

static void msg_timer_standby_handle(){
    enter_standby();
}

lv_obj_t* lvgl_msgbox_standby_charger(int color, char* str){
    lv_obj_t *con = lv_obj_create(lv_layer_top());
    // lv_obj_set_style_text_color(con, lv_color_white(), 0);
    lv_obj_set_style_text_color(con, lv_palette_main(color), 0);
    lv_obj_set_size(con, LV_SIZE_CONTENT, LV_PCT(10));
    lv_obj_center(con);
    lv_obj_set_style_bg_color(con, lv_palette_darken(LV_PALETTE_GREY, 1), 0);
    lv_obj_set_scrollbar_mode(con, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(con, LV_OPA_100, 0);
    lv_obj_set_style_border_width(con, 1, 0);
    lv_obj_set_style_border_color(con, lv_palette_main(color), 0);
    lv_obj_set_style_outline_width(con, 0, 0);
    lv_obj_set_style_radius(con, 10, 0);

    lv_obj_t *label = lv_label_create(con);
    lv_obj_center(label);
    lv_label_set_text(label, str);
    lv_obj_set_style_text_font(label, &LISTFONT_3000, 0);

    lv_timer_t *timer = lv_timer_create(msg_timer_standby_handle, 3000, con);
    lv_timer_set_repeat_count(timer, 1);
    lv_timer_reset(timer);
    return con;
}

void msg_timer_handle1(){
    lvgl_msgbox_standby_charger(LV_PALETTE_RED, (char*)api_rsc_string_get(STR_BATTERY_SHUTDOWN));
}

int lvgl_msgbox_standby()
{
    timer1 = lv_timer_create(msg_timer_handle1, STANDBY_TIMER, NULL);
    lv_timer_set_repeat_count(timer1, 1);
    lv_timer_reset(timer1);
    return 0;
}

//Anim callback function
void anim_size_cb(void * var, int32_t v)
{
    char power[][20] = {LV_SYMBOL_BATTERY_EMPTY,LV_SYMBOL_BATTERY_1,
                        LV_SYMBOL_BATTERY_2,LV_SYMBOL_BATTERY_3,LV_SYMBOL_BATTERY_FULL};
    lv_label_set_text((lv_obj_t *)var,power[v]);
}

//Add power_label event callback
static void power_label_event_cb(lv_event_t* event)
{
    int battery_level;
    int charging;
    int full_charge;
    pm_ioctl(PM_GET_BATTERY_LEVEL, &battery_level);
    pm_ioctl(PM_GET_CHARGING_STATE, &charging);
    pm_ioctl(PM_GET_FULL_CHARGE_STATE, &full_charge);

    if (charging==PM_IN_CHARGING_STATE) {  //Charging state
        battery_bt_control_pwm(10, 0);
        if (!(timer1==NULL)) {
            lv_timer_reset(timer1);
            lv_timer_pause(timer1);
            lv_timer_del(timer1);
            timer1=NULL;
        }

        lv_label_set_text(charge_label, LV_SYMBOL_CHARGE);
        
        if (full_charge==PM_NOT_FULL_CHARGED_STATE) {  //Not fully charged state
            lv_anim_t *power_anim_0=&power_anim;
            if(power_anim_0==NULL){
                lv_anim_t power_anim;
                // lv_anim_init(&power_anim);
            }
            lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));
            lv_anim_start(&power_anim);
        }else {  //Fully charged state
            lv_anim_t *power_anim_1=&power_anim;
            if(!(power_anim_1==NULL)){
                lv_anim_del(&power_anim,anim_size_cb);
                lv_anim_del_all();
            }
            lv_label_set_text(power_label, LV_SYMBOL_BATTERY_FULL);
            lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));
        }
    }else {  //Not in charging state
        lv_anim_t *power_anim_2=&power_anim;
        if(!(power_anim_2==NULL)){
            lv_anim_del(&power_anim,anim_size_cb);
            lv_anim_del_all();
        }

        lv_label_set_text(charge_label, "");

        switch (battery_level) {  //power show
        case PM_BATTERY_LEVEL_100:
            lv_label_set_text(power_label, LV_SYMBOL_BATTERY_FULL);
            lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));
            break;
        case PM_BATTERY_LEVEL_75:
            lv_label_set_text(power_label, LV_SYMBOL_BATTERY_3);
            lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));
            break;
        case PM_BATTERY_LEVEL_50:
            lv_label_set_text(power_label, LV_SYMBOL_BATTERY_2);
            lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_ORANGE));
            battery_bt_control_pwm(10, 30);
            break;
        case PM_BATTERY_LEVEL_25:
            lv_label_set_text(power_label, LV_SYMBOL_BATTERY_1);
            lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_RED));
            battery_bt_control_pwm(10, 30);

            if (state==0) {
                state=1;
                lvgl_msgbox_charger(LV_PALETTE_RED, (char*)api_rsc_string_get(STR_LOW_BATTERY));
            }

            if (timer1==NULL) {
                lvgl_msgbox_standby();
            }
            break;
        case PM_BATTERY_LEVEL_3:
            lv_label_set_text(power_label, LV_SYMBOL_BATTERY_EMPTY);
            lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_RED));
            lvgl_msgbox_standby_charger(LV_PALETTE_RED, (char*)api_rsc_string_get(STR_BATTERY_SHUTDOWN));
            break;
        default:
            lv_label_set_text(power_label, "");
            break;
        }
    }
}

//draw battery logo (LVGL)
void lv_battery_screen_show(void)
{
    int I2C_ret = 0;
    I2C_ret = pmu_init();
    if (I2C_ret < 0) {
        app_log(LL_ERROR,"I2C_ret = %d ,NO find node pm or gpio-i2c in dts",I2C_ret);
        return;
    }

    power_label = lv_label_create(lv_layer_top());  //create power label(global)
    lv_label_set_text(power_label, LV_SYMBOL_BATTERY_FULL);
    lv_obj_align(power_label, LV_ALIGN_TOP_RIGHT, 0, -5);

    lv_style_init(&style);  //power label style
    lv_obj_add_style(power_label, &style, 0);
    lv_style_set_text_font(&style,&lv_font_montserrat_40);

    lv_style_init(&charge_style);   //Charging logo style
    charge_label=lv_label_create(power_label);
    lv_obj_align_to(charge_label,power_label,LV_ALIGN_OUT_RIGHT_MID,-30,10);
    lv_style_set_text_font(&charge_style,&lv_font_montserrat_22);
    lv_style_set_text_color(&charge_style, lv_palette_main(LV_PALETTE_LIME));
    lv_obj_add_style(charge_label, &charge_style, 0);
    lv_label_set_text(charge_label, "");

    lv_anim_init(&power_anim);  //Initialize Anim
    lv_anim_set_var(&power_anim, power_label);
    lv_anim_set_values(&power_anim, 0, 4);
    lv_anim_set_time(&power_anim, 2000);
    lv_anim_set_repeat_delay(&power_anim, 1000);
    lv_anim_set_repeat_count(&power_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&power_anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&power_anim, anim_size_cb);

    //Add power_label event callback
    lv_obj_add_event_cb(power_label, power_label_event_cb, LV_EVENT_REFRESH, NULL);
    lv_event_send(power_label, LV_EVENT_REFRESH, NULL);
}
