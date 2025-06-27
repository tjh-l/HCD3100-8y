#include <stdio.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <kernel/fb.h>
#include <kernel/module.h>
#include <lvgl/hc-porting/hc_lvgl_init.h>
#include "screen.h"
#include "./volume/volume.h"
#include "key_event.h"
#include "channel/local_mp/mp_mainpage.h"
#include "channel/local_mp/mp_fspage.h"
#include "channel/local_mp/mp_playerpage.h"
#include "channel/local_mp/local_mp_ui.h"


static int fd_key;
static int fd_adc_key = -1;
lv_indev_t *indev_keypad;
static lv_group_t *g;
static TaskHandle_t tinyplayer_thread = NULL;
static lv_obj_t *m_last_scr = NULL;

static int key_init(void);


void key_set_group(lv_group_t *key_group)
{
    lv_group_set_default(key_group);
    lv_indev_set_group(indev_keypad, key_group);
}

void _ui_screen_change(lv_obj_t *target, int spd, int delay)
{
    m_last_scr = lv_scr_act();

    lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_MOVE_TOP, spd, delay, false);
}

/*Initialize your keypad*/
static void keypad_init(void)
{
    /*Your code comes here*/
    fd_key = open("/dev/input/event0", O_RDONLY);
    fd_adc_key = open("/dev/input/event1", O_RDONLY);
}

/* Description:
*        hotkeys proc, such as POWER/MENU/SETUP keys etc.
 * return 0:  hotkeys consumed by app, don't send to lvgl;  others: lvgl keys
 */
static uint32_t key_preproc(uint32_t act_key)
{
    uint32_t ret = 0;

    if (act_key != 0)
    {
        if (KEY_VOLUMEUP == act_key || KEY_VOLUMEDOWN == act_key)
        {
            if (volume_bar)
            {
                ret = act_key;
            }
            else
            {
                create_volume();
                ret = act_key;
            }
        }
        else
        {
            printf(">>lvgl keys: %d\n", (int)act_key);//map2lvgl_key(act_key);
            ret = act_key;
        }
    }

    return ret;
}

/* ir key code map to lv_keys*/
static uint32_t keypad_key_map2_lvkey(uint32_t act_key)
{
    switch (act_key)
    {
        case KEY_UP:
            act_key = LV_KEY_UP;
            break;
        case KEY_DOWN:
            act_key = LV_KEY_DOWN;
            break;
        case KEY_LEFT:
            act_key = LV_KEY_LEFT;
            break;
        case KEY_RIGHT:
            act_key = LV_KEY_RIGHT;
            break;
        case KEY_OK:
            act_key = LV_KEY_ENTER;
            break;
        case KEY_ENTER:
            act_key = LV_KEY_ENTER;
            break;
        case KEY_NEXT:
            act_key = LV_KEY_NEXT;
            break;
        case KEY_PREVIOUS:
            act_key = LV_KEY_PREV;
            break;
        case KEY_EXIT:
            act_key = LV_KEY_ESC;
            break;
        case KEY_ESC:
            act_key = LV_KEY_ESC;
            break;
        case KEY_EPG:
            act_key = LV_KEY_HOME;
            break;
        default:
            act_key = USER_KEY_FLAG | act_key;
            break;
    }
    return act_key;
}

/*Get the currently being pressed key.  0 if no key is pressed*/
static uint32_t keypad_read_key(lv_indev_data_t *lv_key)
{
    /*Your code comes here*/
    struct input_event *t;
    uint32_t ret = 0;
    KEY_MSG_s *key_msg = NULL;

    key_msg = api_key_msg_get();
    if (!key_msg)
    {
        return 0;
    }
    t = &key_msg->key_event;

    if (t->value == 1) // pressed
    {
        ret = key_preproc(t->code);
        lv_key->state = (ret == 0) ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;
    }
    else if (t->value == 0) // released
    {
        ret = t->code;
        lv_key->state = LV_INDEV_STATE_REL;
    }

    lv_key->key = keypad_key_map2_lvkey(ret);
    return ret;
}

/*Will be called by the library to read the mouse*/
void keypad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    /*Get whether the a key is pressed and save the pressed key*/
    keypad_read_key(data);
}

static int key_init(void)
{
    static lv_indev_drv_t keypad_driver;

    keypad_init();
    lv_indev_drv_init(&keypad_driver);
    keypad_driver.type = LV_INDEV_TYPE_KEYPAD;
    keypad_driver.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&keypad_driver);

    g = lv_group_create();
    lv_group_set_default(g);
    lv_indev_set_group(indev_keypad, g);

    return 0;
}

// Logic handle for removing and inserting storage devices
void stroage_hotplug_handle(int msg_type)
{
    int updata_state = 0;
    partition_info_t *cur_partition_info = mmp_get_partition_info();
    if (cur_partition_info == NULL)
    {
        return;
    }
    printf(">>!%s ,%d\n", __func__, __LINE__);
    if (msg_type == MSG_TYPE_USB_MOUNT || msg_type == MSG_TYPE_SD_MOUNT)
    {
        if (lv_scr_act() == ui_mainpage)
        {
            lv_event_send(mp_partition, LV_EVENT_REFRESH, 0);
        }
    }
    else if (msg_type == MSG_TYPE_USB_UNMOUNT || msg_type == MSG_TYPE_SD_UNMOUNT || msg_type == MSG_TYPE_SD_UNMOUNT_FAIL || msg_type == MSG_TYPE_USB_UNMOUNT_FAIL)
    {
        if (lv_scr_act() == ui_mainpage)
        {
            lv_event_send(mp_partition, LV_EVENT_REFRESH, 0);
        }
        else if (lv_scr_act() == ui_fspage || lv_scr_act() == ui_player)
        {
            if (api_check_partition_used_dev_ishotplug())
            {
                _ui_screen_change(ui_mainpage, 0, 0);
                app_media_list_all_free();
            }
        }
    }
}

static void com_message_process(control_msg_t *ctl_msg)
{
    if (!ctl_msg)
    {
        return;
    }

    switch (ctl_msg->msg_type)
    {
        // for test usb msg
        case MSG_TYPE_USB_MOUNT:
        case MSG_TYPE_USB_UNMOUNT:
        case MSG_TYPE_USB_UNMOUNT_FAIL:
        case MSG_TYPE_SD_MOUNT:
        case MSG_TYPE_SD_UNMOUNT:
        case MSG_TYPE_SD_UNMOUNT_FAIL:
        {
            partition_info_update(ctl_msg->msg_type, (char *)ctl_msg->msg_code);
            /*due to msg_code is malloc in wq pthread */
            stroage_hotplug_handle(ctl_msg->msg_type);
            break;
        }
        default:
            break;
    }

}

static void message_ctrl_process(void)
{
    control_msg_t ctl_msg = {0,};
    screen_ctrl ctrl_fun = NULL;
    lv_obj_t *screen;
    int ret = -1;

    screen = lv_scr_act();
    do
    {
        ret = api_control_receive_msg(&ctl_msg);
        if (0 != ret)
        {
            if (0 != ret)
                break;
        }
        if (screen)
        {
            ctrl_fun = api_screen_get_ctrl(screen);
            if (ctrl_fun)
                ctrl_fun((void *)&ctl_msg, NULL);
        }
    }
    while (0);

    com_message_process(&ctl_msg);
}

//init UI screens
static void ui_screen_init(void)
{
    if (!volume_scr)
        volume_screen_init();

    //local mp screen
    if (!ui_mainpage)
        ui_mainpage_screen_init();
    if (!ui_fspage)
        ui_fspage_screen_init();
    if (!ui_player)
        ui_player_screen_init();
}

//init some UI/LVGL system.
static void ui_sys_init(void)
{
    lv_disp_t *dispp = NULL;
    lv_theme_t *theme = NULL;

    hc_lvgl_init();
    key_init();
    dispp = lv_disp_get_default();
    theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), \
                                  lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
}

static void *post_sys_init_task(void *arg)
{
    api_system_init();
    app_ffplay_init();

    api_key_get_init();

    return NULL;
}

//Some modules can be initialized late to show UI menu as soon as possible.
static void post_sys_init(void)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); //release task resource itself
    pthread_create(&thread_id, &attr, post_sys_init_task, NULL);
    pthread_attr_destroy(&attr);
}

static void tinyplayer_task(void *pvParameters)
{
    api_sys_clock_time_check_start();

    printf("\n\n ********* Welcome to Hichip world! *********\n\n");

    api_system_pre_init();

    ui_sys_init();

    ui_screen_init();

    api_dis_show_onoff(0);

    _ui_screen_change(ui_mainpage, 0, 0);

    volatile char first_flag = 1;
    while (1)
    {
        message_ctrl_process();

        lv_task_handler();

        if (first_flag)
        {
            //Hide boot logo after UI showed.
            api_dis_show_onoff(0);
            post_sys_init();

            first_flag = 0;
        }

        usleep(10000);
    }

}

static void lvgl_exit(void)
{
    lv_deinit();
}

static int lvgl_stop(int argc, char **argv)
{
    struct fb_var_screeninfo var;
    int fbfd = 0;

    fbfd = open(FBDEV_PATH, O_RDWR);
    if (fbfd == -1)
        return -1;

    ioctl(fbfd, FBIOBLANK, FB_BLANK_NORMAL);
    ioctl(fbfd, FBIOGET_VSCREENINFO, &var);
    var.yoffset = 0;
    var.xoffset = 0;
    ioctl(fbfd, FBIOPUT_VSCREENINFO, &var);
    close(fbfd);

    if (tinyplayer_thread != NULL)
        vTaskDelete(tinyplayer_thread);
    lvgl_exit();

    fbdev_exit();

    return 0;
}

static int tinyplayer_start(int argc, char **argv)
{
    // start tinyplayer main task.
    xTaskCreate(tinyplayer_task, (const char *)"tinyplayer_solution", 0x2000/*configTASK_STACK_DEPTH*/,
                NULL, portPRI_TASK_NORMAL, &tinyplayer_thread);
    return 0;
}

static int tinyplayer_auto_start(void)
{
    tinyplayer_start(0, NULL);
    return 0;
}

__initcall(tinyplayer_auto_start)

