#include "main_page.h"

#include <stdio.h>
#include <stdlib.h>

#include "boardtest_module.h"
#include "com_api.h"
// #include "lvgl/lvgl.h"
#include "osd_com.h"
#include "boardtest_run.h"

#define BOARDTEST_LIST_ROWS 10
#define BOARDTEST_BG_COLOR_BLUE  lv_color_hex(0x2196F3)
#define BOARDTEST_BG_COLOR_RED   lv_color_hex(0xEB3324)
#define BOARDTEST_BG_COLOR_GRAY  lv_color_hex(0x808080)
#define BOARDTEST_BG_COLOR_GREEN lv_color_hex(0x75F94D)
#define BOARDTEST_BG_COLOR_BLACK lv_color_hex(0x000000)

static lv_obj_t *main_page_scr = NULL;
static lv_obj_t *list1;
static lv_group_t *main_page_g;
static lv_obj_t *list_cont[BOARDTEST_NUM];
static lv_obj_t *check_sort[BOARDTEST_NUM];
static lv_obj_t *btn_test[BOARDTEST_NUM];
static lv_obj_t *btn_state[BOARDTEST_NUM];
static lv_obj_t *lab_state[BOARDTEST_NUM];
static lv_obj_t *lab_detail[BOARDTEST_NUM];
static lv_obj_t *result_cont;
static lv_obj_t *btn_enable_num;
static lv_obj_t *lab_enable_num;
static lv_obj_t *btn_fail_num;
static lv_obj_t *lab_fail_num;
static lv_obj_t *btn_pending_num;
static lv_obj_t *lab_pending_num;
static lv_obj_t *btn_set[2];
static lv_obj_t *help_mbox;          /*help mbox*/
static int test_sort[BOARDTEST_NUM]; // index

static void lv_system_layout(void);
static void lv_List_layout(void);
static void event_handler_help(lv_event_t *e);
static void event_handler_mode(lv_event_t *e);
static void event_handler_sort(lv_event_t *e);
static void event_handler_test(lv_event_t *e);
static void event_handler_pass_mbox(lv_event_t *e);

static hc_boardtest_msg_t *boardtest;
void key_set_group(lv_group_t *key_group);

void main_page_init(void)
{
    /*If an object does not have a parent, then it can be considered a child of the screen object, or it can be said that it represents the entire screen*/
    main_page_scr = lv_scr_act();

    /*create the focus group*/
    main_page_g = lv_group_create();

    /*system*/
    lv_system_layout();

    /*lsit*/
    lv_List_layout();

    /*The input device joins the focus group*/
    key_set_group(main_page_g);
}

static void event_handler_test(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    lv_obj_t *obj = lv_event_get_target(e);
    void *user_data;
    int sort;
    user_data = lv_obj_get_user_data(obj);
    sort = *(int *)user_data;

    if (code == LV_EVENT_KEY)
    {
        uint32_t key = lv_indev_get_key(lv_indev_get_act());

        if (key == LV_KEY_DOWN)
        {
            sort = sort + 1;
            lv_group_focus_next(main_page_g);
            lv_group_focus_next(main_page_g);
        }
        else if (key == LV_KEY_UP)
        {
            sort = sort - 1;
            lv_group_focus_prev(main_page_g);
            lv_group_focus_prev(main_page_g);
        }
        else if (key == LV_KEY_LEFT)
        {
            lv_group_focus_prev(main_page_g);
        }
        else if (key == LV_KEY_RIGHT)
        {
            sort = sort + 1;
            lv_group_focus_next(main_page_g);
        }
        else if (key == LV_KEY_ENTER)
        {
            if (lv_obj_get_state(check_sort[sort]) & LV_STATE_CHECKED)
            {
                control_msg_t ctl_msg = {0};
                ctl_msg.msg_type = MSG_TYPE_BOARDTEST_SORT_SEND;
                ctl_msg.msg_code = sort;
                boardtest_run_control_send_msg(&ctl_msg);
            }
        }
        if (sort > 0 && sort < BOARDTEST_NUM)
            lv_obj_scroll_to_view_recursive(btn_test[sort], LV_ANIM_OFF);
    }
}

static void event_handler_sort(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    lv_obj_t *obj = lv_event_get_target(e);
    const char *check_text;
    int sort;
    check_text = lv_checkbox_get_text(obj);
    sort = atoi(check_text);   // char to int
    sort--;

    if (code == LV_EVENT_KEY)
    {
        uint32_t key = lv_indev_get_key(lv_indev_get_act());

        if (key == LV_KEY_DOWN)
        {
            sort = sort + 1;
            lv_group_focus_next(main_page_g);
            lv_group_focus_next(main_page_g);
        }
        else if (key == LV_KEY_UP)
        {
            sort = sort - 1;
            lv_group_focus_prev(main_page_g);
            lv_group_focus_prev(main_page_g);
        }
        else if (key == LV_KEY_LEFT)
        {
            sort = sort - 1;
            lv_group_focus_prev(main_page_g);
        }
        else if (key == LV_KEY_RIGHT)
        {
            lv_group_focus_next(main_page_g);
        }
        else if (key == LV_KEY_ENTER)
        {
            boardtest = hc_boardtest_msg_get(sort);
            /*If check is not checked, the entire row cannot be selected*/
            if (lv_obj_get_state(obj) & LV_STATE_CHECKED)
            {
                /*The current check state is checked, and it is toggled to Unchecked*/
                lv_obj_clear_state(obj, LV_STATE_CHECKED);
                lv_obj_set_style_bg_color(btn_test[sort], BOARDTEST_BG_COLOR_GRAY, LV_STATE_DEFAULT);     // gray
                boardtest->isabled = BOARDTEST_DISABLE;
            }
            else
            {
                lv_obj_add_state(obj, LV_STATE_CHECKED);
                lv_obj_set_style_bg_color(btn_test[sort], BOARDTEST_BG_COLOR_BLUE, LV_STATE_DEFAULT);     /*blue*/
                boardtest->isabled = BOARDTEST_ENABLE;
            }
        }
        if (sort > 0 && sort < BOARDTEST_NUM)
            lv_obj_scroll_to_view_recursive(btn_test[sort], LV_ANIM_OFF);
    }
}

static void event_handler_help_mbox(int btn_sel, void *user_data)
{
    if (btn_sel == 0)
        lv_group_focus_obj(btn_set[0]); //help btn
}

static void event_handler_help(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_KEY)
    {
        uint32_t key = lv_indev_get_key(lv_indev_get_act());

        if (key == LV_KEY_DOWN)
        {
            lv_group_focus_next(main_page_g);
            lv_group_focus_next(main_page_g);
            lv_obj_scroll_to_view_recursive(btn_test[0], LV_ANIM_OFF);
        }
        else if (key == LV_KEY_UP)
        {
            lv_group_focus_prev(main_page_g);
            lv_group_focus_prev(main_page_g);
            lv_obj_scroll_to_view_recursive(btn_test[BOARDTEST_NUM - 1], LV_ANIM_OFF);
        }
        else if (key == LV_KEY_LEFT)
        {
            lv_group_focus_prev(main_page_g);
            lv_obj_scroll_to_view_recursive(btn_test[BOARDTEST_NUM - 1], LV_ANIM_OFF);
        }
        else if (key == LV_KEY_RIGHT)
        {
            lv_group_focus_next(main_page_g);
        }
        else if (key == LV_KEY_ENTER)
        {
            main_page_total_result_update();
            win_msgbox_ok_open(main_page_scr, "Welcome to Hichip world!", event_handler_help_mbox, NULL);
        }
    }
}

static void event_handler_mode(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_KEY)
    {
        uint32_t key = lv_indev_get_key(lv_indev_get_act());

        if (key == LV_KEY_DOWN)
        {
            lv_obj_scroll_to_view_recursive(btn_test[0], LV_ANIM_OFF);
            lv_group_focus_next(main_page_g);
            lv_group_focus_next(main_page_g);
        }
        else if (key == LV_KEY_UP)
        {
            lv_obj_scroll_to_view_recursive(btn_test[BOARDTEST_NUM - 1], LV_ANIM_OFF);
            lv_group_focus_prev(main_page_g);
            lv_group_focus_prev(main_page_g);
        }
        else if (key == LV_KEY_LEFT)
        {
            lv_group_focus_prev(main_page_g);
        }
        else if (key == LV_KEY_RIGHT)
        {
            lv_obj_scroll_to_view_recursive(btn_test[0], LV_ANIM_OFF);
            lv_group_focus_next(main_page_g);
        }
        else if (key == LV_KEY_ENTER)
        {
            if (boardtest_run_get_auto() == 0)
            {
                // Send a message to the queue that starts the automated test process Start the test from the first item
                control_msg_t ctl_msg = {0};
                ctl_msg.msg_type = MSG_TYPE_BOARDTEST_AUTO;
                ctl_msg.msg_code = 0; /*Start testing with this one*/
                boardtest_run_control_send_msg(&ctl_msg);
            }
        }
    }
}

static void lv_system_layout(void)
{
    lv_obj_t *lab;

    static lv_style_t style_test;
    lv_style_init(&style_test);
    lv_style_set_opa(&style_test, LV_OPA_COVER);
    lv_style_set_bg_color(&style_test, lv_palette_lighten(LV_PALETTE_PINK, 1));

    static lv_style_t style_label;
    lv_style_init(&style_label);
    lv_style_set_text_color(&style_label, BOARDTEST_BG_COLOR_BLACK);     /*black*/

    /*title*/
    lab = lv_label_create(main_page_scr);
    lv_label_set_text_static(lab, "boardtest");
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_28, 0);
    lv_obj_align(lab, LV_ALIGN_TOP_MID, 0, 10);   // Head centered

    /*help*/
    btn_set[0] = lv_btn_create(main_page_scr);
    lv_obj_align(btn_set[0], LV_ALIGN_TOP_LEFT, 0, 10);
    lab = lv_label_create(btn_set[0]);
    lv_label_set_text_static(lab, "help");
    lv_group_add_obj(main_page_g, btn_set[0]);
    lv_obj_add_event_cb(btn_set[0], event_handler_help, LV_EVENT_ALL, NULL);
    lv_obj_add_style(btn_set[0], &style_test, LV_STATE_FOCUSED);
    lv_obj_add_style(lab, &style_label, 0);

    /*mode*/
    btn_set[1] = lv_btn_create(main_page_scr);
    lv_obj_align(btn_set[1], LV_ALIGN_TOP_RIGHT, 0, 10);
    lab = lv_label_create(btn_set[1]);
    lv_label_set_text_fmt(lab, "auto");
    lv_group_add_obj(main_page_g, btn_set[1]);
    lv_obj_add_event_cb(btn_set[1], event_handler_mode, LV_EVENT_ALL, NULL);
    lv_obj_add_style(btn_set[1], &style_test, LV_STATE_FOCUSED);
    lv_obj_add_style(lab, &style_label, 0);

    /*list1 create*/
    list1 = lv_list_create(main_page_scr);
    lv_obj_set_size(list1, lv_pct(100), lv_pct(100 - (lv_obj_get_height(btn_set[0]) + 10) / 7.2));           // List percentage
    lv_obj_align(list1, LV_ALIGN_BOTTOM_MID, 0, 0);                                                /*The bottom is center-aligned*/
}

static void lv_List_layout(void)
{
    lv_obj_t *lab;

    int i;
    char buf[100]; // test name
    static lv_style_t style_test;
    lv_style_init(&style_test);
    lv_style_set_opa(&style_test, LV_OPA_COVER);   // Sets the style background transparency
    lv_style_set_bg_color(&style_test, lv_palette_lighten(LV_PALETTE_PINK, 1));

    static lv_style_t style_label;
    lv_style_init(&style_label);
    lv_style_set_text_color(&style_label, BOARDTEST_BG_COLOR_BLACK);     /*black*/

    int ten_list_cont_sort;
    lv_obj_t *ten_list_cont[((BOARDTEST_NUM + 1) / BOARDTEST_LIST_ROWS) + 1];

    for (ten_list_cont_sort = 0; ten_list_cont_sort < ((BOARDTEST_NUM + 1) / BOARDTEST_LIST_ROWS) + 1; ten_list_cont_sort++)
    {
        ten_list_cont[ten_list_cont_sort] = lv_obj_create(list1);
        lv_obj_set_width(ten_list_cont[ten_list_cont_sort], lv_pct(100));
        lv_obj_set_height(ten_list_cont[ten_list_cont_sort], lv_pct(100));
        lv_obj_clear_flag(ten_list_cont[ten_list_cont_sort], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_flex_flow(ten_list_cont[ten_list_cont_sort], LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_top(ten_list_cont[ten_list_cont_sort], 10, 0);

        /*When the last page cannot fill the entire space, uniform distribution is not applied*/
        if (ten_list_cont_sort == ((BOARDTEST_NUM + 1) / BOARDTEST_LIST_ROWS) && (BOARDTEST_NUM + 1) % BOARDTEST_LIST_ROWS != 0)
            lv_obj_set_flex_align(ten_list_cont[ten_list_cont_sort], LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        else
            lv_obj_set_flex_align(ten_list_cont[ten_list_cont_sort], LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    }
    printf("ten_list_cont_sort = %d\n", ten_list_cont_sort);

    lv_obj_set_scroll_snap_y(list1, LV_SCROLL_SNAP_START);

    for (i = 0; i < BOARDTEST_NUM; i++)
    {
        ten_list_cont_sort = i / BOARDTEST_LIST_ROWS;

        list_cont[i] = lv_obj_create(ten_list_cont[ten_list_cont_sort]);
        lv_obj_set_width(list_cont[i], lv_pct(100));
        lv_obj_set_height(list_cont[i], lv_pct(100 / BOARDTEST_LIST_ROWS));
        lv_obj_clear_flag(list_cont[i], LV_OBJ_FLAG_SCROLLABLE);   // Disable scroll bars
        lv_obj_set_flex_flow(list_cont[i], LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(list_cont[i], LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        /*sort*/
        check_sort[i] = lv_checkbox_create(list_cont[i]);
        lv_obj_set_width(check_sort[i], lv_pct(5));
        sprintf(buf, "%d", i + 1);   // int to char, tester's perspective test ranking starts at 1
        lv_checkbox_set_text(check_sort[i], buf);
        lv_obj_set_align(check_sort[i], LV_ALIGN_CENTER);
        lv_group_add_obj(main_page_g, check_sort[i]);
        lv_obj_clear_flag(check_sort[i], LV_OBJ_FLAG_CHECKABLE);   // Set to uncheckable, otherwise the arrow keys will also cause a toggle
        lv_obj_add_event_cb(check_sort[i], event_handler_sort, LV_EVENT_ALL, NULL);

        /*test name*/
        btn_test[i] = lv_btn_create(list_cont[i]);
        lv_obj_set_width(btn_test[i], lv_pct(20));
        lv_obj_add_style(btn_test[i], &style_test, LV_STATE_FOCUSED);
        lv_obj_add_event_cb(btn_test[i], event_handler_test, LV_EVENT_ALL, NULL);
        lab = lv_label_create(btn_test[i]);
        lv_obj_set_width(lab, lv_pct(100));
        lv_label_set_long_mode(lab, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_label_set_text_fmt(lab, "test%d", i);
        lv_obj_set_align(lab, LV_ALIGN_CENTER);

        boardtest = hc_boardtest_msg_get(i);
        /*If the english_name is initialized*/
        if (boardtest->boardtest_msg_reg->english_name)
        {
            snprintf(buf, sizeof(buf), "%s", boardtest->boardtest_msg_reg->english_name);
            lv_label_set_text(lab, buf);
            lv_label_set_long_mode(lab, LV_LABEL_LONG_SCROLL_CIRCULAR);
        }

        lv_obj_set_style_bg_color(btn_test[i], BOARDTEST_BG_COLOR_GRAY, LV_STATE_DEFAULT);     // gray
        lv_obj_add_style(lab, &style_label, 0);
        lv_obj_clear_state(btn_test[i], LV_STATE_CHECKED);
        lv_group_add_obj(main_page_g, btn_test[i]);
        test_sort[i] = i;
        lv_obj_set_user_data(btn_test[i], &test_sort[i]);

        /*state : pass or fail or going or pending*/
        btn_state[i] = lv_btn_create(list_cont[i]);
        lab_state[i] = lv_label_create(btn_state[i]);
        lv_label_set_text_fmt(lab_state[i], "pending");
        lv_obj_set_width(btn_state[i], lv_pct(7));
        lv_obj_set_style_bg_color(btn_state[i], BOARDTEST_BG_COLOR_GRAY, LV_STATE_DEFAULT);
        lv_obj_set_align(lab_state[i], LV_ALIGN_CENTER);
        lv_obj_add_style(lab_state[i], &style_label, 0);
        lv_group_remove_obj(btn_state[i]); /*Avoid generating focus*/

        /*detail*/
        lab_detail[i] = lv_label_create(list_cont[i]);
        lv_obj_set_width(lab_detail[i], lv_pct(40));
        lv_label_set_text_fmt(lab_detail[i], "detail");
        lv_label_set_long_mode(lab_detail[i], LV_LABEL_LONG_SCROLL_CIRCULAR);   /*Loop scrolling*/
        lv_obj_set_align(lab_detail[i], LV_ALIGN_CENTER);                     /*Horizontal and vertical centered display*/
    }

    /*total result*/
    ten_list_cont_sort = BOARDTEST_NUM / BOARDTEST_LIST_ROWS;
    printf("ten_list_cont_sort = %d\n", ten_list_cont_sort);
    result_cont = lv_obj_create(ten_list_cont[ten_list_cont_sort]);
    lv_obj_set_width(result_cont, lv_pct(100));
    lv_obj_set_height(result_cont, lv_pct(100 / BOARDTEST_LIST_ROWS));
    lv_obj_clear_flag(result_cont, LV_OBJ_FLAG_SCROLLABLE);   // Disable scroll bars
    lv_obj_set_flex_flow(result_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(result_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /*enable*/
    btn_enable_num = lv_btn_create(result_cont);
    lab_enable_num = lv_label_create(btn_enable_num);
    lv_label_set_text_fmt(lab_enable_num, "enable");
    lv_obj_set_width(btn_enable_num, lv_pct(15));
    lv_obj_set_style_bg_color(btn_enable_num, BOARDTEST_BG_COLOR_BLUE, LV_STATE_DEFAULT); /*blue*/
    lv_obj_set_align(lab_enable_num, LV_ALIGN_CENTER);
    lv_obj_add_style(lab_enable_num, &style_label, 0);
    lv_group_remove_obj(btn_enable_num); /*Avoid generating focus*/

    /*fail*/
    btn_fail_num = lv_btn_create(result_cont);
    lab_fail_num = lv_label_create(btn_fail_num);
    lv_label_set_text_fmt(lab_fail_num, "fail");
    lv_obj_set_width(btn_fail_num, lv_pct(15));
    lv_obj_set_style_bg_color(btn_fail_num, BOARDTEST_BG_COLOR_BLUE, LV_STATE_DEFAULT); /*blue*/
    lv_obj_set_align(lab_fail_num, LV_ALIGN_CENTER);
    lv_obj_add_style(lab_fail_num, &style_label, 0);
    lv_group_remove_obj(btn_fail_num); /*Avoid generating focus*/

    /*pending*/
    btn_pending_num = lv_btn_create(result_cont);
    lab_pending_num = lv_label_create(btn_pending_num);
    lv_label_set_text_fmt(lab_pending_num, "pending");
    lv_obj_set_width(btn_pending_num, lv_pct(15));
    lv_obj_set_style_bg_color(btn_pending_num, BOARDTEST_BG_COLOR_GRAY, LV_STATE_DEFAULT); /*blue*/
    lv_obj_set_align(lab_pending_num, LV_ALIGN_CENTER);
    lv_obj_add_style(lab_pending_num, &style_label, 0);
    lv_group_remove_obj(btn_pending_num); /*Avoid generating focus*/

    lv_group_focus_obj(check_sort[0]);   // Set the focus
}

void key_set_group(lv_group_t *key_group)
{
    lv_group_set_default(key_group);
    lv_indev_set_group(indev_keypad, key_group);
}

void main_page_run_detail_update(int sort)
{
    boardtest = hc_boardtest_msg_get(sort);
    if (boardtest->detail)
    {
        lv_label_set_text_fmt(lab_detail[sort], "%s", boardtest->detail);
        free(boardtest->detail);
        boardtest->detail = NULL;
    }
}

static int last_state = -1;

void main_page_state_detail_update(int sort)
{
    char detail_all[100];

    boardtest = hc_boardtest_msg_get(sort);

    /*When the execution time of the test item is <= 50ms,
    only the final result information is updated to avoid the detail being overwritten*/
    if (last_state == boardtest->state)
    {
        last_state = -1;
        return;
    }

    switch (boardtest->state)
    {
        case BOARDTEST_FAIL:
        {
            lv_label_set_text_fmt(lab_state[sort], "fail");
            lv_obj_set_style_bg_color(btn_state[sort], BOARDTEST_BG_COLOR_RED, LV_STATE_DEFAULT);     // red
            /*update detail*/
            snprintf(detail_all, sizeof(detail_all), "Run time: %d ms\n", boardtest->run_time);

            if (boardtest->detail)
            {
                strncat(detail_all, boardtest->detail, sizeof(detail_all) - strlen(detail_all));
                free(boardtest->detail);
                boardtest->detail = NULL;
            }
            else if (boardtest->boardtest_msg_reg->tips)
                strncat(detail_all, boardtest->boardtest_msg_reg->tips, sizeof(detail_all) - strlen(detail_all));

            lv_label_set_text_fmt(lab_detail[sort], "%s", detail_all);
            break;
        }
        case BOARDTEST_PASS:
        {
            lv_label_set_text_fmt(lab_state[sort], "pass");
            lv_obj_set_style_bg_color(btn_state[sort], BOARDTEST_BG_COLOR_GREEN, LV_STATE_DEFAULT);     // green
            /*update detail*/
            snprintf(detail_all, sizeof(detail_all), "Run time: %d ms\n", boardtest->run_time);

            if (boardtest->detail)
            {
                strncat(detail_all, boardtest->detail, sizeof(detail_all) - strlen(detail_all));
                free(boardtest->detail);
                boardtest->detail = NULL;
            }

            lv_label_set_text_fmt(lab_detail[sort], "%s", detail_all);
            break;
        }
        case BOARDTEST_GOING:
        {
            lv_label_set_text_fmt(lab_state[sort], "going");
            lv_obj_set_style_bg_color(btn_state[sort], lv_color_hex(0xFFFD55), LV_STATE_DEFAULT);     // yellow
            break;
        }
        case BOARDTEST_PENDING:
        {
            lv_label_set_text_fmt(lab_state[sort], "pending");
            lv_obj_set_style_bg_color(btn_state[sort], BOARDTEST_BG_COLOR_GRAY, LV_STATE_DEFAULT);     // gray
            break;
        }
        case BOARDTEST_CALL_PASS: /*the enumeration type is not received*/
            break;
        default: /*error_code*/
        {
            snprintf(detail_all, sizeof(detail_all), "Run time: %d ms  \n", boardtest->run_time);

            if (boardtest->detail)
            {
                strncat(detail_all, boardtest->detail, sizeof(detail_all) - strlen(detail_all));
                free(boardtest->detail);
                boardtest->detail = NULL;
            }
            else
            {
                const char *error_message = hc_boardtest_error_msg_get(boardtest->state);
                strncat(detail_all, error_message, sizeof(detail_all) - strlen(detail_all));
            }

            lv_label_set_text_fmt(lab_detail[sort], "%s", detail_all);
            lv_label_set_text_fmt(lab_state[sort], "fail");
            lv_obj_set_style_bg_color(btn_state[sort], BOARDTEST_BG_COLOR_RED, LV_STATE_DEFAULT);     // red
            break;
        }
    }
    last_state = boardtest->state;
    lv_obj_scroll_to_view_recursive(btn_test[sort], LV_ANIM_OFF);
}

void main_page_g_back(int sort)
{
    lv_group_focus_obj(btn_test[sort]);
}

void main_page_ini_init(void)
{
    for (int i = 0; i < BOARDTEST_NUM; i++)
    {
        boardtest = hc_boardtest_msg_get(i);
        if (boardtest->isabled == BOARDTEST_ENABLE)
        {
            lv_obj_add_state(check_sort[i], LV_STATE_CHECKED);                                /*Unselected by default*/
            lv_obj_set_style_bg_color(btn_test[i], BOARDTEST_BG_COLOR_BLUE, LV_STATE_DEFAULT); /*blue*/
        }
        else
        {
            lv_obj_clear_state(check_sort[i], LV_STATE_CHECKED);
            lv_obj_set_style_bg_color(btn_test[i], BOARDTEST_BG_COLOR_GRAY, LV_STATE_DEFAULT); // gray
        }
    }
}

void lvgl_osd_close(void)
{
    lv_obj_add_flag(main_page_scr, LV_OBJ_FLAG_HIDDEN);
    api_dis_show_onoff(1);
}

void lvgl_osd_open(void)
{
    lv_obj_clear_flag(main_page_scr, LV_OBJ_FLAG_HIDDEN);
    api_dis_show_onoff(0);
}

void main_page_total_result_update(void)
{
    int i;
    int total_enable = 0;
    int total_fail = 0;
    int total_pending = 0;

    for (i = 0; i < BOARDTEST_NUM; i++)
    {
        boardtest = hc_boardtest_msg_get(i);
        if (boardtest->isabled == BOARDTEST_ENABLE)
        {
            total_enable++;
            if (boardtest->state == BOARDTEST_PENDING)
                total_pending++;
            else if (boardtest->state != BOARDTEST_PASS)
                total_fail++;
        }
    }

    lv_label_set_text_fmt(lab_enable_num, "enable  :  %d", total_enable);
    lv_label_set_text_fmt(lab_pending_num, "pending  :  %d", total_pending);
    if (total_fail == 0)
    {
        lv_obj_set_style_bg_color(btn_fail_num, BOARDTEST_BG_COLOR_GREEN, LV_STATE_DEFAULT);     // green
        lv_label_set_text_fmt(lab_fail_num, "pass");
    }
    else
    {
        lv_obj_set_style_bg_color(btn_fail_num, BOARDTEST_BG_COLOR_RED, LV_STATE_DEFAULT);     // red
        lv_label_set_text_fmt(lab_fail_num, "fail  :  %d", total_fail);
    }
    lv_obj_scroll_to_view_recursive(btn_fail_num, LV_ANIM_OFF);
}
