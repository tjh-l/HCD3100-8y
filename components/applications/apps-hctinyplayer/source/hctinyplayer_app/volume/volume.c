#include <sys/ioctl.h>
#include <hcuapi/snd.h>
#include <hcuapi/input-event-codes.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include "key_event.h"
#include "volume.h"
#include "screen.h"

lv_obj_t *volume_scr = NULL;

lv_group_t *volume_g = NULL;
lv_obj_t *volume_bar;
static lv_timer_t *volume_timer = NULL, *timer_mute = NULL;
lv_group_t *pre_group = NULL;
lv_obj_t *prev_obj = NULL;
lv_obj_t *icon = NULL;
static int volume_num = 3;

static void timer_handler(lv_timer_t *timer1);
static int key_pre_proc(int key);
static void event_handler(lv_event_t *e);

void create_balance_ball(lv_obj_t *parent, lv_coord_t radius, lv_coord_t width)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_size(obj, LV_PCT(width), LV_PCT(100));
    lv_obj_set_style_bg_color(obj, lv_palette_lighten(LV_PALETTE_GREY, 4), 0);
}

lv_obj_t *create_display_bar_widget(lv_obj_t *parent, int w, int h)
{
    lv_obj_t *balance = lv_obj_create(parent);

    lv_obj_set_style_radius(balance, 0, 0);
    lv_obj_set_size(balance, LV_PCT(w), LV_PCT(h));
    lv_obj_set_style_outline_width(balance, 0, 0);
    lv_obj_align(balance, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(balance, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_style_bg_opa(balance, LV_OPA_50, 0);
    lv_obj_set_flex_flow(balance, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(balance, 0, 0);
    lv_obj_set_style_pad_gap(balance, 0, 0);
    lv_obj_set_flex_align(balance, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    return balance;
}

lv_obj_t *create_display_bar_name_part(lv_obj_t *parent, char *name, int w, int h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    set_pad_and_border_and_outline(obj);
    lv_obj_align(obj, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_size(obj, LV_PCT(w), LV_PCT(h));
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_center(label);

    return obj;
}

lv_obj_t *create_display_bar_main(lv_obj_t *parent, int w, int h, int ball_count, int width)
{
    lv_obj_t *container = lv_obj_create(parent);
    for (int i = 0; i < ball_count; i++)
    {
        create_balance_ball(container, 8, width);
    }

    lv_obj_set_style_bg_color(container, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_size(container, LV_PCT(w), LV_PCT(h));
    lv_obj_set_style_outline_width(container, 0, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_ver(container, 0, 0);
    lv_obj_set_style_pad_gap(container, 1, 0);
    lv_obj_set_style_pad_hor(container, 1, 0);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);

    lv_group_add_obj(lv_group_get_default(), container);
    lv_group_focus_obj(container);
    return container;
}

lv_obj_t *create_display_bar_show(lv_obj_t *parent, int w, int h, int num)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(obj, LV_PCT(w), LV_PCT(h));
    set_pad_and_border_and_outline(obj);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);

    lv_obj_t *label = lv_label_create(obj);
    lv_label_set_text_fmt(label, "%d", num);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_center(label);

    return obj;
}

/**
 * set i2so volume.
 * set bluetooth volume on C2 board.(special case)
 * set i2si volume in cvbs-in that using i2si->i2so apath.
*/
void set_volume1(uint8_t vol)
{
    int snd_fd = -1;

    //set i2so
    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0)
    {
        printf ("open snd_fd %d failed\n", snd_fd);
        return ;
    }
    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &vol);
    vol = 0;
    ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &vol);
    printf("%s volume is %d\n", __func__, vol);
    close(snd_fd);
}

static void timer_handler(lv_timer_t *timer1)
{
    if (volume_bar)
    {
        lv_obj_del(volume_bar);
        volume_bar = NULL;
    }

    if (lv_obj_is_valid(prev_obj))
    {
        //If backup key group is changed or curren key group is volume key group,
        //do not recover backup key group. So that avoid recover the deleted group.
        if (lv_group_get_default() != volume_g && lv_group_get_default() != pre_group)
        {
            volume_timer = NULL;
            return;
        }

        lv_group_set_default(pre_group);
        lv_indev_set_group(indev_keypad, pre_group);
        lv_group_focus_obj(prev_obj);
    }

    volume_timer = NULL;
}

static int key_pre_proc(int key)
{
    int key1 = USER_KEY_FLAG ^ key;
    if (key1 == KEY_VOLUMEUP)
    {
        return LV_KEY_UP;
    }
    else if (key1 == KEY_VOLUMEDOWN)
    {
        return LV_KEY_DOWN;
    }
    return key;
}

static void event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if (code == LV_EVENT_KEY)
    {
        lv_timer_pause(volume_timer);
        uint32_t key = key_pre_proc(lv_indev_get_key(lv_indev_get_act())) ;
        if (key == LV_KEY_UP || key == LV_KEY_DOWN)
        {
            int8_t count = lv_obj_get_child_cnt(target);
            if (key == LV_KEY_UP && count < 33)
            {
                count++;
                create_balance_ball(target, 8, 3);
            }
            if (key == LV_KEY_DOWN && (--count) >= 0)
            {
                lv_obj_del(lv_obj_get_child(target, count));
            }
            volume_num = count > 0 ? count * 3 : 0;
            if (volume_num > -1 && volume_num < 101)
            {
                lv_obj_t *sub = lv_obj_get_child(target->parent, 2);
                lv_obj_t *label = lv_obj_get_child(sub, 0);
                lv_label_set_text_fmt(label, "%d", volume_num / 3);
                set_volume1(volume_num);
            }
        }
        if (key == LV_KEY_ESC || key == LV_KEY_ENTER)
        {

            lv_timer_ready(volume_timer);
            lv_timer_resume(volume_timer);
            return;
        }
        lv_timer_resume(volume_timer);
        lv_timer_reset(volume_timer);
    }

}

void volume_screen_init(void)
{
    volume_scr = lv_layer_top();
    set_volume1(volume_num);
}

void del_volume()
{
    if (volume_timer)
    {
        lv_timer_del(volume_timer);
        volume_timer = NULL;
    }
    if (volume_bar)
    {
        lv_obj_del(volume_bar);
        volume_bar = NULL;
        lv_group_del(volume_g);
        volume_g = NULL;
    }
}

void create_volume()
{
    if (!volume_bar)
    {
        pre_group = lv_group_get_default();
        prev_obj = lv_group_get_focused(pre_group);
        volume_g = lv_group_create();
        lv_group_set_default(volume_g);
        lv_indev_set_group(indev_keypad, volume_g);
        volume_bar = create_display_bar_widget(volume_scr, 70, 9);
        lv_obj_set_style_bg_opa(volume_bar, LV_OPA_60, 0);
        lv_obj_set_style_outline_width(volume_bar, 0, 0);
        create_display_bar_name_part(volume_bar, "volume", 22, 100);
        lv_obj_t *container = create_display_bar_main(volume_bar, 70, 44,  volume_num / 3, 3);
        lv_obj_add_event_cb(container, event_handler, LV_EVENT_ALL, 0);
        create_display_bar_show(volume_bar, 8, 100, volume_num / 3);
        if (!volume_timer)
        {
            volume_timer = lv_timer_create(timer_handler, 3000, container);
            lv_timer_set_repeat_count(volume_timer, 1);
            lv_timer_reset(volume_timer);
        }
    }
    else
    {
        if (volume_timer)
        {
            lv_timer_reset(volume_timer);
        }
    }
}
