#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "key_event.h"
#include "media_player.h"
#include "local_mp_ui.h"
#include "mp_mainpage.h"
#include "mp_fspage.h"
#include "screen.h"

media_type_t media_type;
partition_info_t *cur_temp_partition = NULL;
#define MAX_DEVNAME_LEN 20
static char last_devname[MAX_DEVNAME_LEN] = {0};

void set_key_group(lv_group_t *group)
{
    lv_indev_set_group(indev_keypad, group);
    lv_group_set_default(group);
}

void main_page_keyinput_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *target = lv_event_get_target(event);
    if (code == LV_EVENT_KEY)
    {
        int keypad_value = lv_indev_get_key(lv_indev_get_act());
        switch (keypad_value)
        {
            case LV_KEY_UP:
                lv_group_focus_next(lv_group_get_default());
                break;
            case LV_KEY_DOWN:
                lv_group_focus_prev(lv_group_get_default());
                break;
            case LV_KEY_LEFT:
                lv_group_focus_prev(lv_group_get_default());
                break;
            case LV_KEY_RIGHT:
                lv_group_focus_next(lv_group_get_default());
                break;
            case LV_KEY_ENTER:
            {
                int obj_cont_index = lv_obj_get_index(lv_obj_get_parent(target));
                api_set_partition_info(obj_cont_index);
                if (strcmp(last_devname, cur_temp_partition->used_dev) != 0)
                {
                    app_media_list_all_free();
                    memset(last_devname, 0, sizeof(last_devname));
                    strncpy(last_devname, cur_temp_partition->used_dev, strlen(cur_temp_partition->used_dev));
                }
                _ui_screen_change(ui_fspage, 0, 0);
                break;
            }
            default :
                break;
        }
    }
}


int mainpage_open(void)
{
    main_group = lv_group_create();
    set_key_group(main_group);
    create_mainpage_scr();
    cur_temp_partition = mmp_get_partition_info();
    return 0;
}

int mainpage_close(void)
{
    lv_group_remove_all_objs(main_group);
    lv_group_del(main_group);
    clear_mainpage_scr();
    return 0;
}
