#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "com_api.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/font/lv_font.h"
#include "osd_com.h"

static lv_obj_t *m_msgbox_obj = NULL;
static user_msgbox_cb msgbox_func = NULL;
static void *msgbox_user_data = NULL;
static void win_msgbox_msg_btn_event_cb(lv_event_t *e)
{

    control_msg_t ctl_msg = {0};
    int btn_sel = -1;

    lv_obj_t *obj = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);

    if (code == LV_EVENT_KEY)
    {

        printf("obj:0x%x, target:0x%x\n", (unsigned int)obj, (unsigned int)target);

        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_ENTER)
        {
            btn_sel = lv_msgbox_get_active_btn(obj);
        }

        if (btn_sel != -1)
        {
            lv_obj_del(target->parent);
            if (msgbox_func)
                msgbox_func(btn_sel, msgbox_user_data); // msgbox_user_data don't used
            m_msgbox_obj = NULL;
        }
    }
}

/*user_msgbox_cb Handling message functions Send a message (which button is pressed Pop-up window closed)*/
void win_msgbox_passfail_open(lv_obj_t *parent, char *str_msg, user_msgbox_cb cb, void *user_data)
{
    lv_obj_t *obj_root;

    if (m_msgbox_obj)
        lv_obj_del(m_msgbox_obj);
    static const char *btns[] = {"pass", "fail", ""};

    obj_root = parent ? parent : lv_layer_top();

    m_msgbox_obj = lv_msgbox_create(obj_root, "", str_msg, btns, false);
    lv_obj_center(m_msgbox_obj);

    if (cb)
    {
        msgbox_func = cb;
        if (user_data)
            msgbox_user_data = user_data;
        else
            msgbox_user_data = NULL;
    }
    else
    {
        msgbox_func = NULL;
        msgbox_user_data = NULL;
    }

    lv_obj_add_event_cb(m_msgbox_obj, win_msgbox_msg_btn_event_cb, LV_EVENT_ALL, NULL);

    lv_group_focus_obj(lv_msgbox_get_btns(m_msgbox_obj));
}

void win_msgbox_ok_open(lv_obj_t *parent, char *str_msg, user_msgbox_cb cb, void *user_data)
{
    lv_obj_t *obj_root;

    if (m_msgbox_obj)
        lv_obj_del(m_msgbox_obj);
    static const char *btns[] = {"ok", ""};
    obj_root = parent ? parent : lv_layer_top();

    m_msgbox_obj = lv_msgbox_create(obj_root, "", str_msg, btns, false);
    lv_obj_center(m_msgbox_obj);

    if (cb)
    {
        msgbox_func = cb;
        if (user_data)
            msgbox_user_data = user_data;
        else
            msgbox_user_data = NULL;
    }
    else
    {
        msgbox_func = NULL;
        msgbox_user_data = NULL;
    }

    lv_obj_add_event_cb(m_msgbox_obj, win_msgbox_msg_btn_event_cb, LV_EVENT_ALL, NULL);

    lv_group_focus_obj(lv_msgbox_get_btns(m_msgbox_obj));
}

void win_msgbox_btn_close(void)
{
    if (m_msgbox_obj)
    {
        lv_msgbox_close_async(m_msgbox_obj);
        m_msgbox_obj = NULL;
    }
}
