#include "lv_drivers/display/fbdev.h"
#include "lvgl/demos/lv_demos.h"
#include "lvgl/lvgl.h"
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "boardtest_module.h"
#include "boardtest_read_ini.h"
#include "boardtest_run.h"
#include "channel/main_page/main_page.h"
#include "com_api.h"
#include "key_event.h"
#include "osd_com.h"
#include <lvgl/hc-porting/hc_lvgl_init.h>
#ifdef BLUETOOTH_SUPPORT
extern int hc_test_bluetooth_init();
#endif

static int fd_key;
static int fd_adc_key = -1;
static lv_group_t *g;
static hc_boardtest_msg_t *boardtest;

lv_indev_t *indev_keypad;

static int key_init(void);                                          // Key Initialization, Register the key
void keypad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);   // Gets the currently pressed key
static uint32_t keypad_read_key(lv_indev_data_t *lv_key);           // Notifies LVGL of key press events
static uint32_t keypad_key_map2_lvkey(uint32_t act_key);            // The key codes of the IR remote control are mapped to the keys in LVGL

static void message_ctrl_process(void);

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
#ifdef PROJECTOR_VMOTOR_SUPPORT
        case KEY_CAMERA_FOCUS:
            ctrlbar_reset_mpbacklight(); // reset backlight with key
            vMotor_Roll_cocus();
            act_key = 0;
            break;
        case KEY_FORWARD:
        case KEY_BACK:
            ctrlbar_reset_mpbacklight(); // reset backlight with key
            vMotor_set_step_count(192);
            if (act_key == KEY_FORWARD)
            {
                vMotor_set_direction(BMOTOR_STEP_FORWARD);
            }
            else
            {
                vMotor_set_direction(BMOTOR_STEP_BACKWARD);
            }
            act_key = 0;
            break;
#endif
        default:
            act_key = USER_KEY_FLAG | act_key;
            break;
    }
    return act_key;
}

/* Description:
 *        hotkeys proc, such as EXIT keys etc.
 * return 0:  hotkeys consumed by app, don't send to lvgl;  others: lvgl keys
 */
static uint32_t key_preproc(uint32_t act_key)
{
    if (act_key != 0)
    {
        if (KEY_EXIT == act_key)
        {
            control_msg_t ctl_msg = {0};
            ctl_msg.msg_type = MSG_TYPE_BOARDTEST_STOP;
            ctl_msg.msg_code = BOARDTEST_FAIL;
            boardtest_exit_control_send_msg(&ctl_msg);
            return 0;
        }
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

    if (!key_get_user_input_flag())
        key_msg = api_key_msg_get();
    if (!key_msg)
    {
        return 0;
    }
    t = &key_msg->key_event;

    if (t->value == 1)   // pressed
    {
        ret = key_preproc(t->code);
        lv_key->state = (ret == 0) ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;
    }
    else if (t->value == 0)   // released
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

/*Initialize your keypad*/
static void keypad_init(void)
{
    /*Your code comes here*/
    fd_key = open("/dev/input/event0", O_RDONLY);
    fd_adc_key = open("/dev/input/event1", O_RDONLY);
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

static void *boardtest_task(void *pvParameters)
{
    printf("\n\n ********* Welcome to Hichip world! *********\n\n");
    printf("\n\n ******************* over *******************\n\n");

    hc_lvgl_init();

    key_init();

    hc_boardtest_module_all_register();

    main_page_init();

    api_key_get_init();

    boardtest_run_init();
    boardtest_exit_init();

    boardtest_read_ini_init();

#ifdef BLUETOOTH_SUPPORT
    hc_test_bluetooth_init();
#endif

    while (1)
    {
        message_ctrl_process();

        lv_task_handler();

        usleep(10000);   // 1000 1ms 10000 10ms
    }
}

static int sort_cur;

static void mbox_msg_send(int btn_sel, void *user_data)   /*btn_sel  0->pass 1->fail*/
{
    control_msg_t ctl_msg = {0};
    ctl_msg.msg_type = MSG_TYPE_MBOX_RESULT;
    if (btn_sel == 1)
        ctl_msg.msg_code = BOARDTEST_FAIL;
    else if (btn_sel == 0)
        ctl_msg.msg_code = BOARDTEST_PASS;
    boardtest_exit_control_send_msg(&ctl_msg);
    main_page_g_back(sort_cur);
}

static void com_message_process(control_msg_t *ctl_msg)
{
    if (!ctl_msg)
        return;

    switch (ctl_msg->msg_type)
    {
        case MSG_TYPE_PASSFAIL_MBOX_CREATE:
        {
            char buf[100];
            boardtest = hc_boardtest_msg_get(ctl_msg->msg_code);
            if (boardtest->boardtest_msg_reg->tips)
                snprintf(buf, sizeof(buf), "%s", boardtest->boardtest_msg_reg->tips);
            else
                snprintf(buf, sizeof(buf), "%s", "Please select whether the test item passed or not.");
            win_msgbox_passfail_open(NULL, buf, mbox_msg_send, NULL);
            sort_cur = ctl_msg->msg_code;
            break;
        }
        case MSG_TYPE_OK_MBOX_CREATE:
        {
            char buf[100];
            boardtest = hc_boardtest_msg_get(ctl_msg->msg_code);
            snprintf(buf, sizeof(buf), "%s", boardtest->boardtest_msg_reg->tips);
            win_msgbox_ok_open(NULL, buf, mbox_msg_send, NULL);
            sort_cur = ctl_msg->msg_code;
            break;
        }
        case MSG_TYPE_OSD_CLOSE:
        {
            lvgl_osd_close();
            break;
        }
        case MSG_TYPE_OSD_OPEN:
        {
            lvgl_osd_open();
            break;
        }
        case MSG_TYPE_BOARDTEST_EXIT:
        {
            main_page_state_detail_update(ctl_msg->msg_code);
            break;
        }
        case MSG_TYPE_USB_MOUNT:
        {
            char *mount_add;
            mount_add = api_get_ad_mount_add();
            if (boardtest_read_ini(mount_add) == 0)     /*only run once*/
            {
                boardtest_read_ini_exit();
                main_page_ini_init();
            }
            break;
        }
        case MSG_TYPE_MBOX_CLOSE:
        {
            win_msgbox_btn_close();
            main_page_g_back(ctl_msg->msg_code);
            break;
        }
        case MSG_TYPE_BOARDTEST_AUTO_OVER:
        {
            main_page_total_result_update();
            break;
        }
        case MSG_TYPE_DISPLAY_DETAIL:
        {
            main_page_run_detail_update(ctl_msg->msg_code);
            break;
        }

        default:
            break;
    }
}

static void message_ctrl_process(void)
{
    control_msg_t ctl_msg = {0};
    int ret = -1;

    ret = api_control_receive_msg(&ctl_msg);   // ret == 0 Information is received normally
    if (0 == ret)
        com_message_process(&ctl_msg);
}

static void boardtest_start(void)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // release task resource itself

    if (pthread_create(&thread_id, &attr, boardtest_task, NULL))
    {
        return;
    }

    pthread_attr_destroy(&attr);
}

static int boardtest_auto_start(void)
{
    boardtest_start();
    return 0;
}

__initcall(boardtest_auto_start)
