#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <stdint.h>
#include <hcuapi/persistentmem.h>
#include <semaphore.h>
#include "screen.h"
#include "factory_setting.h"
#include "setup.h"
#include "com_api.h"
#include "mul_lang_text.h"
#include <hcfota.h>
#include "app_config.h"
#include <sys/stat.h>
#include "osd_com.h"
#include <pthread.h>
#include <hcuapi/sysdata.h>
#ifdef LVGL_RESOLUTION_240P_SUPPORT
#define UPGRADE_WIDGET_WIDTH_PCT 45
#define UPGRADE_WIDGET_HEIGHT_PCT 33
#else
#define UPGRADE_WIDGET_WIDTH_PCT 40
#define UPGRADE_WIDGET_HEIGHT_PCT 25
#endif
#ifdef MANUAL_HTTP_UPGRADE
#include "network_api.h"
#include "../../channel/cast/network_upg.h"
void software_network_update_widget(lv_obj_t* btn);
#endif

enum{
    CHECK_OK = 0,
    CHECK_NO_FILE,
    CHECK_FILE_ERR,
    CHECK_PRODUCT_ERR,
    CHECK_VERSION_ERR,
};

lv_timer_t *timer_update;
static long progress = -1;
lv_obj_t *prompt_label = NULL;
lv_obj_t *progress_bar = NULL;

static uint8_t adc_adjust_val=0;

#ifdef USB_AUTO_UPGRADE
volatile int m_usb_upgrade = 0;
static sem_t sys_upg_usb_check_sem;
static sem_t find_software_sem;

static void upgrade_prompt_msg_box(void);
void del_upgrade_prompt(void);
#endif

extern void set_remote_control_disable(bool b);
extern int mmp_get_usb_stat();

static int find_software(char str[160], bool auto_upgrade);
static int hcfota_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata);
static void *software_update_handler(void *target_);
static void timer_update_handle(lv_timer_t *t);
static void software_update_event_handle(lv_event_t *e);
void software_update_widget(lv_obj_t* btn);

static void software_update_widget_(char* urls);
void upgrade_entry_init(void);

static bool m_manual_upgrade = false;
static char* updata_url;
screen_ctrl prev_ctrl = NULL;
static char m_urls[160]={0};

extern lv_timer_t *timer_setting, *timer_setting_clone;
extern lv_font_t* select_font_normal[3];
extern lv_obj_t* slave_scr_obj;

static bool if_can_upgrade(void);

static int version_check(char* file_name)
{
    int ret = CHECK_NO_FILE;
    struct hcfota_header fw_header = { 0 };
    FILE *fp = NULL;
    int header_len = sizeof(struct hcfota_header);
    int file_len;
    int len;

    fp = fopen(file_name, "rb+");
    if (fp == NULL) {
        printf("%s fopen:%s error.\n", __func__, file_name);
        return CHECK_FILE_ERR;
    }

    fseek(fp, 0, SEEK_END);
    file_len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_len > header_len) {
        int read_len = header_len;
        int red_cnt = 0;
        while (read_len) {
            len = fread(&fw_header + red_cnt, 1, read_len, fp);
            if (len <= 0) {
                break;
            }

            red_cnt += len;
            read_len -= len;
        }

        if (red_cnt != header_len) {
            printf("%s fw header len not match.\n", __func__);
            fclose(fp);
            return CHECK_FILE_ERR;
        }

    } else {
        printf("%s fw header len error.\n", __func__);
        fclose(fp);
        return CHECK_FILE_ERR;
    }

    fclose(fp);

    printf("local product:%s, fw product:%s\n", (char *)projector_get_some_sys_param(P_DEV_PRODUCT_ID), (char *)fw_header.board);
    printf("local version:%u, fw version:%u\n", projector_get_some_sys_param(P_DEV_VERSION), (unsigned int)fw_header.version);

    char product_id[16] = { 0 };
    snprintf(product_id, sizeof(product_id), "%s", (char *)projector_get_some_sys_param(P_DEV_PRODUCT_ID));
    if (memcmp(product_id, fw_header.board, sizeof(fw_header.board)) == 0) {
        if (
            fw_header.ignore_version_check || 
            (fw_header.version != projector_get_some_sys_param(P_DEV_VERSION))
            ) {
            printf("%s(), line: %d. found upg file:%s!\n", __func__, __LINE__, file_name);
            ret = CHECK_OK;
        }else{
            ret = CHECK_VERSION_ERR;
        }

    }else{
        ret = CHECK_PRODUCT_ERR;
    }
    return ret;
}

static int find_software(char str[160], bool auto_upgrade)
{
    struct stat sd;
    int ret = CHECK_NO_FILE;
    char file_name[160] = { 0 };
    partition_info_t *partinfo = NULL;
    char  *dev_name = NULL;
    int index;

    partinfo = mmp_get_partition_info();
    if (!partinfo)
        return CHECK_NO_FILE;

    if (auto_upgrade){
    /* auto usb upgrade, check the last plug in device */
        index = partinfo->count-1 > 0 ? partinfo->count-1 : 0;
        dev_name = api_get_partition_info_by_index(index);
        if (!dev_name)
            return CHECK_NO_FILE;

        memset(file_name, 0, sizeof(file_name));
        snprintf(file_name, sizeof(file_name), "%s/%s", dev_name, BR2_EXTERNAL_HCFOTA_FILENAME);
        if (!stat(file_name, &sd)) 
            ret = CHECK_OK;
        else
            return CHECK_NO_FILE;
            
    } else {
    /* usb upgrade from UI menu, check devices one by one */
        for (index = 0; index < partinfo->count; index ++){
            dev_name = api_get_partition_info_by_index(index);
            if (!dev_name)
                return CHECK_NO_FILE;

            memset(file_name, 0, sizeof(file_name));
            snprintf(file_name, sizeof(file_name), "%s/%s", dev_name, BR2_EXTERNAL_HCFOTA_FILENAME);
            if (!stat(file_name, &sd)) {
                ret = CHECK_OK; 
                break;
            }
        }
        if (index == partinfo->count)
            ret = CHECK_NO_FILE; 
    }

    if (CHECK_OK == ret) {
        ret = version_check(file_name);
        if (CHECK_OK == ret){
            memcpy(str, file_name, 160);
            printf("update file: %s\n", str);
        }
    }else{
        ret = CHECK_NO_FILE;
    }

    return ret;

}

static int hcfota_report(hcfota_report_event_e event, unsigned long param, unsigned long usrdata)
{
    if (event == HCFOTA_REPORT_EVENT_UPGRADE_PROGRESS) {
        progress = param;
        control_msg_t msg = { 0 };
        msg.msg_type = MSG_TYPE_UPG_BURN_PROGRESS;
        api_control_send_msg(&msg);

    } else if (event == HCFOTA_REPORT_EVENT_DOWNLOAD_PROGRESS) {
        progress = param;
        control_msg_t msg = { 0 };
        msg.msg_type = MSG_TYPE_UPG_DOWNLOAD_PROGRESS;
        api_control_send_msg(&msg);
    }
    return 0;
}

#define REBOOT_LATER 4

static void update_failed_timer_handle(lv_timer_t *t)
{
	if (prompt_label && lv_obj_is_valid(prompt_label->parent)) {
		lv_obj_del(prompt_label->parent);
		turn_to_setup_root();
	}
}

static int set_ota_detect_mode(unsigned long mode)
{
    int fd;
    struct persistentmem_node node;
    struct sysdata sysdata = { 0 };

    fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
    if (fd < 0) {
        printf("open /dev/persistentmem failed\n");
        return -1;
    }

    sysdata.ota_detect_modes = mode;
    node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
    node.offset = offsetof(struct sysdata, ota_detect_modes);
    node.size = sizeof(sysdata.ota_detect_modes);
    node.buf = &sysdata.ota_detect_modes;
    if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
        printf("put sysdata failed\n");
        close(fd);
        return -1;
    }

    return 0;
}

static void update_control(void *arg1, void *arg2)
{
    (void)arg2;
    if (prev_ctrl) {
        prev_ctrl(arg1, arg2);
    }
    control_msg_t *ctl_msg = (control_msg_t *)arg1;
    switch (ctl_msg->msg_type) {
        case MSG_TYPE_UPG_STATUS:
            switch (ctl_msg->msg_code) {
                case UPG_STATUS_BURN_FAIL:
                    lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_ERR));
                    break;
                case UPG_STATUS_VERSION_IS_OLD:
                    lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_VERSION_ERR));
                    break;
                case UPG_STATUS_USB_READ_ERR:
                    lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_LOAD_ERR));
                    break;
                case UPG_STATUS_FILE_CRC_ERROR:
                    lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_ERR));
                    break;
                case UPG_STATUS_FILE_UNZIP_ERROR:
                    lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_DECO_ERR));
                    break;
                case UPG_STATUS_BURN_OK:
                    lv_label_set_text_fmt(prompt_label, "%s %d%s", api_rsc_string_get(STR_UPGRADE_SUCCESS_MSG1), REBOOT_LATER, api_rsc_string_get(STR_UPGRADE_SUCCESS_MSG2));
                default:
                    break;
            }

            lv_timer_t *timer = lv_timer_create(update_failed_timer_handle, 3000, NULL);
            lv_timer_set_repeat_count(timer, 1);
            lv_timer_reset(timer);
            break;
        case MSG_TYPE_UPG_DOWNLOAD_PROGRESS:
        case MSG_TYPE_UPG_BURN_PROGRESS:
            if (ctl_msg->msg_type == MSG_TYPE_UPG_DOWNLOAD_PROGRESS) {
                lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_DOWNLOADING));
            } else {
                lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADEING));
            }

            static unsigned long prev_progress = 0;
            if (prev_progress != progress) {
                lv_bar_set_value(progress_bar, progress, LV_ANIM_OFF);
                lv_label_set_text_fmt(lv_obj_get_child(progress_bar, 0), "%ld%%", progress);
                prev_progress = progress;
            }
            break;
        case MSG_TYPE_USB_UPGRADE:
#ifdef USB_AUTO_UPGRADE
            upgrade_prompt_msg_box();
#else
            software_update_widget_(m_urls);
#endif
            break;
        default:
            break;
    }
}

static void reboot_timer_handle(lv_timer_t *t)
{
    static int count = REBOOT_LATER - 1;
    if (count--) {
        lv_label_set_text_fmt(prompt_label, "%s %d%s", api_rsc_string_get(STR_UPGRADE_SUCCESS_MSG1), count, api_rsc_string_get(STR_UPGRADE_SUCCESS_MSG2));
    } else {
        if (set_ota_detect_mode(HCFOTA_REBOOT_OTA_DETECT_NONE) < 0) {
            printf("set ota detect mode failed\n");
        }

        api_system_reboot();
    }
}

static void *software_update_handler(void *target_)
{
    (void)target_;
    int rc;

    set_remote_control_disable(true);
    if (set_ota_detect_mode(HCFOTA_REBOOT_OTA_DETECT_USB_DEVICE) < 0) {
        printf("set ota detect mode failed\n");
    }
    #ifdef __linux__
        api_hw_watchdog_mmap_open();
    #endif    

    api_upgrade_buffer_sync();
    rc = hcfota_url_checkonly(updata_url, 0);
    if (0 == rc)
    {
    #ifdef APPMANAGER_SUPPORT //only support in linux
        rc = api_app_command_upgrade(updata_url);
        if (0 == rc){
            //send upgrade command OK, entering upgrade app.
            while(1)
                sleep(1);
        }
        
    #else
        api_enter_flash_rw();
        rc = hcfota_url(updata_url, hcfota_report, 0);
        api_leave_flash_rw();
    #endif
    }

    control_msg_t msg = { 0 };
    msg.msg_type = MSG_TYPE_UPG_STATUS;
    if (rc == 0) {
        msg.msg_code = UPG_STATUS_BURN_OK;
        lv_timer_t *reboot_timer = lv_timer_create(reboot_timer_handle, 1000, NULL);
        lv_timer_reset(reboot_timer);
        lv_timer_set_repeat_count(reboot_timer, REBOOT_LATER);

    } else {
    #ifdef __linux__
        api_hw_watchdog_mmap_close();
    #endif    
        
        lv_mem_free(updata_url);
        set_remote_control_disable(false);

        switch (rc) {
            case HCFOTA_ERR_LOADFOTA:
                printf("load file err!");
                msg.msg_code = UPG_STATUS_USB_READ_ERR;
                break;
            case HCFOTA_ERR_HEADER_CRC:
                printf("header crc err!");
                msg.msg_code = UPG_STATUS_FILE_CRC_ERROR;
                break;
            case HCFOTA_ERR_VERSION:
                printf("version err!");
                msg.msg_code = UPG_STATUS_VERSION_IS_OLD;
                break;
            case HCFOTA_ERR_DECOMPRESSS:
                printf("decompress err!");
                msg.msg_code = UPG_STATUS_FILE_UNZIP_ERROR;
                break;
            case HCFOTA_ERR_UPGRADE:
                printf("upgrade err");
                msg.msg_code = UPG_STATUS_BURN_FAIL;
            default:
                break;
        }
        api_control_send_msg(&msg);
        if (timer_setting_clone) {
            lv_timer_resume(timer_setting_clone);
            lv_timer_reset(timer_setting_clone);
            timer_setting = timer_setting_clone;
            timer_setting_clone = NULL;
        }
    }
    if (timer_update) {
        lv_timer_pause(timer_update);
        lv_timer_del(timer_update);
        timer_update = NULL;
    }
    api_is_upgrade_set(false);

    return NULL;
}

static void software_update_event_handle(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *target = lv_event_get_target(e);
	if (code == LV_EVENT_KEY) {
		uint16_t key = lv_indev_get_key(lv_indev_get_act());
		if (key == LV_KEY_ESC) {
			lv_obj_del(target);
			turn_to_setup_root();

		} else if (key == LV_KEY_HOME) {
			lv_obj_del(target);
			turn_to_setup_root();
		}
	}
}

void software_update_widget(lv_obj_t *btn)
{
    int check_ret;
    char *err_str = NULL;
	lv_obj_t *obj = lv_obj_create(setup_slave_root);

	slave_scr_obj = obj;
	lv_obj_set_style_radius(obj, 20, 0);
	lv_obj_set_style_border_width(obj, 0, 0);
	lv_obj_set_style_outline_width(obj, 0, 0);
	lv_group_add_obj(setup_g, obj);
	lv_group_focus_obj(obj);
	lv_obj_add_event_cb(obj, software_update_event_handle, LV_EVENT_ALL, NULL);
	lv_obj_set_style_bg_color(obj, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
	lv_obj_set_size(obj, LV_PCT(25), LV_PCT(25));
	lv_obj_center(obj);
	lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
	lv_obj_t *label;

#if 0
	int usb_status = mmp_get_usb_stat();
	if (usb_status == USB_STAT_UNMOUNT ||
        usb_status == USB_STAT_INVALID) {
		label = lv_label_create(obj);
		lv_obj_center(label);
		lv_label_set_recolor(label, true);
		lv_label_set_text(label, api_rsc_string_get(STR_UPGRADE_NO_DEV));
		lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);
		if (timer_setting) {
			lv_timer_reset(timer_setting);
			lv_timer_resume(timer_setting);
		}
		return;
	}
#endif

    check_ret = find_software(m_urls, false);
	if (CHECK_OK != check_ret) {
		label = lv_label_create(obj);
        if (CHECK_FILE_ERR == check_ret)
            err_str = api_rsc_string_get(STR_UPGRADE_LOAD_ERR);        
        else if (CHECK_PRODUCT_ERR == check_ret)
            err_str = api_rsc_string_get(STR_UPGRADE_PRODUCT_ERR);        
        else if (CHECK_VERSION_ERR == check_ret)
            err_str = api_rsc_string_get(STR_UPGRADE_VERSION_ERR);        
        else {
            int usb_status = mmp_get_usb_stat();
            if (usb_status == USB_STAT_UNMOUNT ||
                usb_status == USB_STAT_INVALID) {
                err_str = api_rsc_string_get(STR_UPGRADE_NO_DEV);    
            } else {
                err_str = api_rsc_string_get(STR_UPGRADE_NO_SOFTWATE);          
            }
        }

		lv_obj_center(label);
        lv_obj_set_size(label, LV_PCT(100), LV_PCT(40));
		lv_label_set_recolor(label, true);
		lv_label_set_text(label, err_str);
		lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);
		if (timer_setting) {
			lv_timer_reset(timer_setting);
			lv_timer_resume(timer_setting);
		}
		return;
	}

    m_manual_upgrade = true;
	lv_obj_del(obj);
	upgrade_entry_init();
	control_msg_t msg = { 0 };
	msg.msg_type = MSG_TYPE_USB_UPGRADE;
	api_control_send_msg(&msg);
}

static void software_update_widget_(char *urls)
{
	updata_url = lv_mem_alloc(strlen(urls) + 1);
	memcpy(updata_url, urls, strlen(urls) + 1);

    api_is_upgrade_set(true);

	printf("%s\n", updata_url);
	lv_obj_t *obj1 = NULL;
	if (lv_scr_act() == setup_scr) {
		obj1 = lv_obj_create(setup_slave_root);
	} else {
		obj1 = lv_obj_create(lv_layer_top());
	}

	lv_obj_set_style_text_color(obj1, lv_color_white(), 0);
	lv_obj_set_style_outline_width(obj1, 0, 0);
	lv_obj_set_style_border_width(obj1, 0, 0);

	lv_obj_set_style_bg_color(obj1, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
	lv_obj_set_scrollbar_mode(obj1, LV_SCROLLBAR_MODE_OFF);
	lv_obj_set_style_radius(obj1, 20, 0);
	lv_obj_set_size(obj1, LV_PCT(UPGRADE_WIDGET_WIDTH_PCT), LV_PCT(UPGRADE_WIDGET_HEIGHT_PCT));
	lv_obj_center(obj1);

	lv_obj_t *bar = lv_bar_create(obj1);
	lv_obj_align(bar, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_set_size(bar, LV_PCT(100), LV_PCT(20));
	lv_bar_set_range(bar, 0, 100);
	lv_bar_set_value(bar, 0, LV_ANIM_OFF);
	lv_obj_center(bar);
	progress_bar = bar;

#ifdef APPMANAGER_SUPPORT //only support in linux
    //Do not need to show bar while upgraded by upgrade app
    lv_obj_add_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
#endif    

	lv_obj_t *label = lv_label_create(bar);
	lv_label_set_text(label, "0%");
	lv_obj_center(label);
	lv_obj_set_style_text_color(label, lv_color_white(), 0);

	prompt_label = lv_label_create(obj1);
	lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPGRADE_DOWNLOADING));
	lv_obj_set_width(prompt_label, LV_PCT(100));
	lv_obj_set_style_text_color(prompt_label, lv_color_make(255, 0, 0), 0);
	lv_obj_set_style_text_font(prompt_label, osd_font_get(FONT_MID), 0);
	lv_obj_align_to(prompt_label, bar, LV_ALIGN_OUT_TOP_MID, 0, -3);
	lv_obj_set_style_text_align(prompt_label, LV_TEXT_ALIGN_CENTER, 0);

	label = lv_label_create(obj1);
	lv_label_set_text(label, api_rsc_string_get(STR_UPGRADE_NOT_POWER_OFF));
	lv_obj_set_style_text_font(label, osd_font_get(FONT_MID), 0);
	lv_obj_set_style_text_color(label, lv_color_make(255, 0, 0), 0);
	lv_obj_align_to(label, bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

	if (timer_setting) {
		lv_timer_pause(timer_setting);
		timer_setting_clone = timer_setting;
		timer_setting = NULL;
	}

	pthread_t thread_id = 0;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 0x1000);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, software_update_handler, obj1);
	pthread_attr_destroy(&attr);
}

void upgrade_entry_init(void)
{
	printf("upgrade entry init\n");

	screen_entry_t *update_entry = api_screen_get_ctrl_entry(lv_scr_act());
	bool new_entry = false;
	screen_entry_t entry_temp;
	if (!update_entry) {
		new_entry = true;
		update_entry = &entry_temp;
		update_entry->screen = lv_scr_act();
	} else {
        if (update_entry->control != update_control)
		    prev_ctrl = update_entry->control;
	}

	update_entry->control = update_control;

	if (new_entry) {
		api_screen_regist_ctrl_handle(update_entry);
	}
}


static bool if_can_upgrade(void)
{
    if (api_is_upgrade_get() || 
    #ifdef CAST_SUPPORT
        (ui_wifi_cast_root==lv_scr_act()) ||
    #endif
    #ifdef USBMIRROR_SUPPORT
        (ui_um_play==lv_scr_act()) ||
    #endif
    #ifdef USB_MIRROR_FAST_SUPPORT
        (ui_um_fast==lv_scr_act()) ||
    #endif
        sys_media_playing() 
        ){
        return false;
    } else {
        return true;
    }


}

#ifdef USB_AUTO_UPGRADE
static lv_group_t *prev_g = NULL;
static lv_obj_t *prev_obj = NULL;
static lv_obj_t* upgrade_msg_box = NULL;
static void upgrade_prompt_msg_box_event_handle(lv_event_t *e)
{
	lv_obj_t *target = lv_event_get_current_target(e);
	lv_event_code_t code = lv_event_get_code(e);
	bool end_handle = false;
	if (code == LV_EVENT_PRESSED) {
		if (lv_msgbox_get_active_btn(target) == 0) {
			lv_obj_del(target);
            del_upgrade_prompt();
			software_update_widget_(m_urls);

            m_manual_upgrade = false;
		} else if (lv_msgbox_get_active_btn(target) == 1) {
			end_handle = true;
		}
	} else if (code == LV_EVENT_KEY) {
		uint16_t key = lv_indev_get_key(lv_indev_get_act());
		if (key == LV_KEY_ESC) {
			end_handle = true;
		}
	}

	if (end_handle) {
		if (api_group_is_valid(prev_g) && api_group_contain_obj(prev_g, prev_obj)) {
			key_set_group(prev_g);
            lv_group_focus_obj(prev_obj);
		} else {
			lv_group_t *cur_g = api_get_group_by_scr(lv_scr_act());
			if (cur_g) {
				key_set_group(cur_g);
			}
		}
        del_upgrade_prompt();

		screen_entry_t *update_entry = api_screen_get_ctrl_entry(lv_scr_act());
		if (update_entry && prev_ctrl != update_entry->control) {
			update_entry->control = prev_ctrl;
		}
		prev_ctrl = NULL;

        if (m_manual_upgrade)
            turn_to_setup_root();

        m_manual_upgrade = false;

	}
}

void del_upgrade_prompt(){
    if(upgrade_msg_box && lv_obj_is_valid(upgrade_msg_box)){
        delete_message_box(upgrade_msg_box);
        prev_g = NULL;
        prev_obj = NULL;
    }
    upgrade_msg_box = NULL;        

}

//auto press OK to comfirm USB auto upgrade, just for overnight USB upgrade test
//do not enable for release....
//#define AUTO_TEST_PRESS_OK
#ifdef AUTO_TEST_PRESS_OK
#define AUTO_TEST_DELAY    5000
static lv_timer_t *m_timer_auto_ok = NULL;

static void auto_click_ok(lv_timer_t *t)
{
    if (upgrade_msg_box && lv_obj_is_valid(upgrade_msg_box))
    {
        lv_obj_t * btnmatrix = lv_msgbox_get_btns(upgrade_msg_box);
        //cancel the repeat press
        lv_btnmatrix_set_btn_ctrl(btnmatrix, 0, LV_BTNMATRIX_CTRL_NO_REPEAT); 
        //chose the first button(OK)
        lv_btnmatrix_set_selected_btn(btnmatrix, 0); 
        //send press event
        lv_event_send(upgrade_msg_box, LV_EVENT_PRESSED, NULL);
    }
}
#endif

static void upgrade_prompt_msg_box(void)
{
    if (NULL != upgrade_msg_box)
        return;

#ifdef AUTO_TEST_PRESS_OK
    lv_timer_t *m_timer_auto_ok = lv_timer_create(auto_click_ok, AUTO_TEST_DELAY, NULL);
    lv_timer_reset(m_timer_auto_ok);
    lv_timer_set_repeat_count(m_timer_auto_ok, 1);
#endif    

    prev_g = lv_group_get_default();
    prev_obj = lv_group_get_focused(prev_g);

    upgrade_msg_box = create_message_box1(STR_UPGRADE_PROMPT, STR_PROMPT_YES, STR_PROMPT_NO, upgrade_prompt_msg_box_event_handle, 21, 33);
//	lv_obj_align(upgrade_msg_box, LV_ALIGN_TOP_MID, LV_PCT(29), LV_PCT(10));
    lv_obj_set_flex_align(upgrade_msg_box, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
}

static int find_soft = 0;
static bool sys_upg_usb_task_running = 0;
static void *sys_upg_usb_task(void *arg)
{
	sys_upg_usb_task_running = 1;
	while (1) {
		sem_wait(&sys_upg_usb_check_sem);
		if (CHECK_OK == find_software(m_urls, true)
            && if_can_upgrade()) {
			find_soft = 1;
			upgrade_entry_init();
			control_msg_t msg = { 0 };
			msg.msg_type = MSG_TYPE_USB_UPGRADE;
			api_control_send_msg(&msg);
		}else{
            find_soft = 0;
        }
		sem_post(&find_software_sem);
	}
    return NULL;
}

int sys_upg_usb_check_notify(void)
{
	while (!sys_upg_usb_task_running) {
		usleep(1000);
	}
    
	sem_post(&sys_upg_usb_check_sem);
	sem_wait(&find_software_sem);
	return find_soft;
}

int sys_upg_usb_check_init(void)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    if (m_usb_upgrade) {
        printf("%s(), line: %d. usb uprade task is running!\n", __func__, __LINE__);
        return API_SUCCESS;
    }
    sem_init(&sys_upg_usb_check_sem, 0, 0);
    sem_init(&find_software_sem, 0, 0);

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread_id, &attr, sys_upg_usb_task, NULL)) {
        pthread_attr_destroy(&attr);
        return API_FAILURE;
    }
    m_usb_upgrade = 1;
    return API_SUCCESS;
}

#endif

#ifdef MANUAL_HTTP_UPGRADE

static void network_update_failed_timer_handle(lv_timer_t *t)
{
    if (timer_update)
    {
        lv_timer_del(timer_update);
        timer_update = NULL;
    }
    if (prompt_label && lv_obj_is_valid(prompt_label->parent)) {
        lv_obj_del(prompt_label->parent);
        turn_to_setup_root();
    }
}

static void network_update_control(void *arg1, void *arg2)
{
    (void)arg2;
    if (prev_ctrl) {
        prev_ctrl(arg1, arg2);
    }
    control_msg_t *ctl_msg = (control_msg_t *)arg1;
    screen_entry_t *update_entry;
    if (!(prompt_label && lv_obj_is_valid(prompt_label->parent)))
    {
        printf("[%s] line :%d\n",__func__,__LINE__);
        return;
    }
    switch (ctl_msg->msg_type) {
        case MSG_TYPE_UPG_STATUS:
            switch (ctl_msg->msg_code) {
                case UPG_STATUS_VERSION_IS_OLD:
                    lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPG_SW_VER_NOT_UPG));
                    break;
                case UPG_STATUS_PRODUCT_ID_MISMATCH:
                    lv_label_set_text(prompt_label, api_rsc_string_get(STR_UPG_PRODUCT_ID_MISMATCH));
                    break;
                case UPG_STATUS_CONFIG_ERROR:
                    lv_label_set_text(prompt_label, api_rsc_string_get(STR_CONFIG_ERROR));
                    break;
                case UPG_STATUS_NETWORK_ERROR:
                    lv_label_set_text(prompt_label, api_rsc_string_get(STR_NETWORK_ERR));
                    break;
                default:
                    break;
            }
            printf("[%s]  type : %d  code : %ld\n", __func__, ctl_msg->msg_type, ctl_msg->msg_code);
            timer_update = lv_timer_create(network_update_failed_timer_handle, 3000, NULL);
            lv_timer_set_repeat_count(timer_update, 1);
            lv_timer_reset(timer_update);
            
            update_entry = api_screen_get_ctrl_entry(lv_scr_act());
            if (update_entry && prev_ctrl != update_entry->control) {
                update_entry->control = prev_ctrl;
            }
            prev_ctrl = NULL;
            break;
        case MSG_TYPE_NET_UPGRADE:
            printf("[%s]  type : %d  code : %ld\n", __func__, ctl_msg->msg_type, ctl_msg->msg_code);
            lv_obj_del(prompt_label->parent);
            
            update_entry = api_screen_get_ctrl_entry(lv_scr_act());
            if (update_entry && prev_ctrl != update_entry->control) {
                update_entry->control = prev_ctrl;
            }
            prev_ctrl = NULL;
            break;
        default:
            break;
    }
}

void network_upgrade_entry_init(void)
{
    printf("network upgrade entry init\n");
    screen_entry_t *network_update_entry = api_screen_get_ctrl_entry(lv_scr_act());
    bool new_entry = false;
    screen_entry_t entry_temp;
    if (!network_update_entry) {
        new_entry = true;
        network_update_entry = &entry_temp;
        network_update_entry->screen = lv_scr_act();
    } else {
        if (network_update_entry->control != network_update_control)
            prev_ctrl = network_update_entry->control;
    }

    network_update_entry->control = network_update_control;

    if (new_entry) {
        api_screen_regist_ctrl_handle(network_update_entry);
    }
}

static void software_network_update_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if (code == LV_EVENT_KEY) {
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if (key == LV_KEY_ESC || key == LV_KEY_HOME) {
            lv_obj_del(target);
            turn_to_setup_root();
            
            network_upg_set_user_abort(true);

            if (timer_update)
            {
                lv_timer_pause(timer_update);
                lv_timer_del(timer_update);
                timer_update = NULL;
            }

            screen_entry_t *update_entry = api_screen_get_ctrl_entry(lv_scr_act());
            if (update_entry->control == network_update_control) /*network update is running*/
            {
                if (update_entry && prev_ctrl != update_entry->control) {
                    update_entry->control = prev_ctrl;
                }
                prev_ctrl = NULL;
            }
        }
    }
}

void software_network_update_widget(lv_obj_t *btn)
{
    int check_ret;
    char *err_str = NULL;
    lv_obj_t *obj = lv_obj_create(setup_slave_root);

    slave_scr_obj = obj;
    lv_obj_set_style_radius(obj, 20, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_group_add_obj(setup_g, obj);
    lv_group_focus_obj(obj);
    lv_obj_add_event_cb(obj, software_network_update_event_handle, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(obj, lv_palette_darken(LV_PALETTE_GREY, 2), 0);
    lv_obj_set_size(obj, LV_PCT(25), LV_PCT(25));
    lv_obj_center(obj);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    
    prompt_label = lv_label_create(obj);
    lv_obj_center(prompt_label);
    lv_obj_set_size(prompt_label, LV_PCT(100), LV_PCT(40));
    lv_label_set_recolor(prompt_label, true);    
    lv_obj_set_style_text_font(prompt_label, osd_font_get(FONT_MID), 0);

    if (network_upgrade_flag_get())
    {
        network_upg_set_user_abort(false);
        err_str = api_rsc_string_get(STR_UPGRADEING);
        lv_label_set_text(prompt_label, err_str);

        network_upgrade_entry_init();
        return;
    }

    if (!(network_wifi_module_get() && (hccast_wifi_mgr_get_connect_status() == 1))) {
        err_str = api_rsc_string_get(STR_WIFI_NOT_CONNECT);

        lv_label_set_text(prompt_label, err_str);
        if (timer_setting) {
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
        return;
    }

    err_str = api_rsc_string_get(STR_NET_UPG_REQUEST);
    lv_label_set_text(prompt_label, err_str);

    network_upgrade_entry_init();
    network_upgrade_start();
}

#endif