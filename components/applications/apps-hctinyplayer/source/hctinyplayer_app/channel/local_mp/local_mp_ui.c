#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>
#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"
#include "media_player.h"
#include <sys/stat.h>
#include "win_media_list.h"
#include <hcuapi/vidmp.h>
#include "local_mp_ui.h"
#include "mp_mainpage.h"
#include "mp_fspage.h"
#include "mp_playerpage.h"
#include "screen.h"

///////////////////// VARIABLES ////////////////////
lv_obj_t *ui_mainpage = NULL;
lv_obj_t *ui_fspage = NULL;
lv_obj_t *ui_player = NULL;

lv_group_t *main_group = NULL;
lv_group_t *fs_group = NULL;
lv_group_t *player_group = NULL;

///////////////////// FUNCTIONS ////////////////////
static void ui_event_mainpage(lv_event_t *e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START)
    {
        mainpage_open();
    }
    if (event == LV_EVENT_SCREEN_UNLOAD_START)
    {
        mainpage_close();
    }
}

static void ui_event_fspage(lv_event_t *e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START)
    {
        media_fslist_open();
    }
    if (event == LV_EVENT_SCREEN_UNLOAD_START)
    {
        media_fslist_close();
    }
}

static void ui_event_ctrl_bar(lv_event_t *e)
{
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_KEY)
    {
        uint32_t lv_key = lv_indev_get_key(lv_indev_get_act());
        if (lv_key == LV_KEY_ESC)
        {
            _ui_screen_change(ui_fspage, 0, 0);
        }
    }

    if (event == LV_EVENT_SCREEN_LOAD_START)
    {
        ctrlbarpage_open();
    }
    if (event == LV_EVENT_SCREEN_UNLOAD_START)
    {
        ctrlbarpage_close();
    }
}

void ui_mainpage_screen_init(void)
{
    // just create a screen and add event ,weight display in event cb
    ui_mainpage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_mainpage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_mainpage, ui_event_mainpage, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(ui_mainpage, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_mainpage, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_fspage_screen_init(void)
{
    ui_fspage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_fspage, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(ui_fspage, ui_event_fspage, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(ui_fspage, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fspage, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    screen_entry_t fspage_entry;
    fspage_entry.screen = ui_fspage;
}

void ui_player_screen_init(void)
{
    ui_player = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_player, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(ui_player, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_player, ui_event_ctrl_bar, LV_EVENT_ALL, NULL);

    screen_entry_t ctrl_bar_entry;
    ctrl_bar_entry.screen = ui_player;
}


///////////////////// SCREENS ////////////////////

static lv_obj_t *create_flex_objcont(lv_obj_t *p)
{

    lv_obj_t *obj_cont = lv_obj_create(p);
    lv_obj_set_layout(obj_cont, LV_LAYOUT_FLEX);
    lv_obj_set_size(obj_cont, LV_PCT(100), LV_PCT(70));
    lv_obj_set_align(obj_cont, LV_ALIGN_BOTTOM_MID);
    lv_obj_add_flag(obj_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj_cont, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj_cont, 0, 0);
    lv_obj_set_scrollbar_mode(obj_cont, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_flex_flow(obj_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    return obj_cont;
}

lv_obj_t *create_storage_subicon(lv_obj_t  *obj_cont, int count)
{
    for (int i = 0; i < count; i++)
    {
        lv_obj_t  *obj = lv_obj_create(obj_cont);
        lv_obj_set_size(obj, LV_PCT(23), LV_PCT(100));
        lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, 0);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    }

    for (int i = 0; i < count; i++)
    {
        lv_obj_t *partition_icon = lv_btn_create(lv_obj_get_child(obj_cont, i));
        lv_obj_set_size(partition_icon, LV_PCT(100), LV_PCT(22));
        lv_obj_set_align(partition_icon, LV_ALIGN_CENTER);
        lv_obj_add_flag(partition_icon, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_clear_flag(partition_icon, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(partition_icon, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(partition_icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_shadow_width(partition_icon, 0, 0);
        lv_obj_set_style_text_color(partition_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(partition_icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_align(partition_icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(partition_icon, lv_color_hex(0x031FFF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_bg_opa(partition_icon, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_color(partition_icon, lv_color_hex(0xFAD665), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_opa(partition_icon, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(partition_icon, 2, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_pad(partition_icon, 0, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_text_color(partition_icon, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        lv_obj_set_style_text_opa(partition_icon, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

        lv_obj_add_event_cb(partition_icon, main_page_keyinput_event_cb, LV_EVENT_ALL, 0);

        lv_obj_t *ui_labrootdir = lv_label_create(partition_icon);
        lv_obj_set_width(ui_labrootdir, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_labrootdir, LV_SIZE_CONTENT);
        lv_obj_set_align(ui_labrootdir, LV_ALIGN_CENTER);
        int partition_letter = 67;  //mean 'C' in ASCII
        partition_letter = partition_letter + i;
        char  partition_letter2[2];
        sprintf(partition_letter2, "%c", partition_letter);
        lv_obj_set_style_text_font(ui_labrootdir, &lv_font_montserrat_18, 0);
        lv_label_set_text(ui_labrootdir, partition_letter2);
    }
    return obj_cont;

}

lv_obj_t *mp_partition = NULL;

void partition_obj_refresh_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t  *target = lv_event_get_target(event);
    void *user_data = lv_event_get_user_data(event);
    if (code == LV_EVENT_REFRESH)
    {
        partition_info_t *part_info = mmp_get_partition_info();
        lv_obj_clean(mp_partition);

        if (part_info != NULL && part_info->count > 0)
        {
            create_storage_subicon(mp_partition, part_info->count);
        }
        else
        {
            lv_obj_t *icon = lv_label_create(mp_partition);
            lv_label_set_text(icon, "  No Device");
            lv_obj_set_style_text_font(icon, &lv_font_montserrat_18, 0);
            lv_obj_set_style_bg_color(icon, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_opa(icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

int create_mainpage_scr(void)
{
    partition_info_t *part_info = mmp_get_partition_info();
    lv_obj_t *ui_titie_ = lv_label_create(ui_mainpage);
    lv_obj_set_width(ui_titie_, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_titie_, lv_pct(8));
    lv_obj_align(ui_titie_, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_color(ui_titie_, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_titie_, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_titie_, "Tiny Player");
    lv_obj_set_style_text_font(ui_titie_, &lv_font_montserrat_18, 0);

    mp_partition = create_flex_objcont(ui_mainpage);
    lv_obj_add_event_cb(mp_partition, partition_obj_refresh_cb, LV_EVENT_REFRESH, NULL);

    if (part_info != NULL && part_info->count > 0)
    {
        create_storage_subicon(mp_partition, part_info->count);
    }
    else
    {
        lv_obj_t *icon = lv_label_create(mp_partition);
        lv_label_set_text(icon, "  No Device");
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_18, 0);
        lv_obj_set_style_bg_color(icon, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(icon, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    return 0;
}

int clear_mainpage_scr(void)
{
    lv_obj_clean(ui_mainpage);
    return 0;
}


lv_obj_t *obj_item[CONT_WINDOW_CNT] = {NULL};
lv_obj_t *obj_labelitem[CONT_WINDOW_CNT] = {NULL};

lv_obj_t *objitem_style_init(lv_obj_t *scr, int index)
{
    // obj style
    lv_obj_t *obj = lv_obj_create(scr);
    lv_obj_set_width(obj, lv_pct(100));
    lv_obj_set_height(obj, lv_pct(9));
    lv_obj_set_y(obj, lv_pct(17 + 9 * index));

    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    obj_labelitem[index] = lv_label_create(obj);
    lv_obj_set_width(obj_labelitem[index], lv_pct(90));
    lv_obj_align(obj_labelitem[index], LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(obj_labelitem[index], "");
    lv_obj_set_style_text_font(obj_labelitem[index], &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(obj_labelitem[index], lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_align(obj_labelitem[index], LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_long_mode(obj_labelitem[index], LV_LABEL_LONG_DOT);

    return obj;
}

int create_objcont(lv_obj_t *p)
{
    int k = 0;
    for (int k = 0; k < CONT_WINDOW_CNT; k++)
    {
        obj_item[k] = objitem_style_init(p, k);
        lv_obj_add_event_cb(obj_item[k], fs_page_keyinput_event_cb, LV_EVENT_ALL, NULL);
        lv_group_add_obj(lv_group_get_default(), obj_item[k]);
    }
}

int create_fspage_scr(void)
{
    lv_obj_t *ui_fstitle = lv_label_create(ui_fspage);
    lv_obj_set_width(ui_fstitle, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_fstitle, lv_pct(8));
    lv_obj_align(ui_fstitle, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_color(ui_fstitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_fstitle, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_fstitle, "Tiny Player");
    lv_obj_set_style_text_font(ui_fstitle, &lv_font_montserrat_18, 0);

    lv_obj_t *ui_fsbarimg = lv_label_create(ui_fspage);
    lv_obj_set_width(ui_fsbarimg, lv_pct(100));
    lv_obj_set_height(ui_fsbarimg, lv_pct(8));
    lv_obj_set_y(ui_fsbarimg, lv_pct(8));
    lv_obj_set_style_radius(ui_fsbarimg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_fsbarimg, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_fsbarimg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_fsbarimg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ui_fspath = lv_label_create(ui_fsbarimg);
    lv_obj_set_width(ui_fspath, lv_pct(85));
    lv_obj_set_height(ui_fspath, LV_SIZE_CONTENT);
    lv_obj_align(ui_fspath, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(ui_fspath, "");
    lv_label_set_long_mode(ui_fspath, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_color(ui_fspath, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_fspath, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_fspath, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_fspath, &lv_font_montserrat_18, 0);

    lv_obj_t *ui_fscount = lv_label_create(ui_fsbarimg);
    lv_obj_set_width(ui_fscount, lv_pct(10));
    lv_obj_set_height(ui_fscount, LV_SIZE_CONTENT);
    lv_obj_align(ui_fscount, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_label_set_text(ui_fscount, "");
    lv_label_set_long_mode(ui_fscount, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_color(ui_fscount, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_fscount, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_fscount, &lv_font_montserrat_18, 0);

    create_objcont(ui_fspage);
    return 0 ;
}

int clear_fapage_scr(void)
{
    lv_obj_clean(ui_fspage);
    for (int i = 0; i < CONT_WINDOW_CNT; i++)
    {
        if (lv_obj_is_valid(obj_labelitem[i]) == false)
            obj_labelitem[i] = NULL;
    }
    return 0;
}
