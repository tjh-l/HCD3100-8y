
//#define SUPPORT_BLUETOOTH 1

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hcuapi/input-event-codes.h>

#include "app_config.h"
#include "screen.h"
#include "factory_setting.h"
#include "setup.h"
#include "mul_lang_text.h"
#include "osd_com.h"
#include "com_api.h"
#include "hcstring_id.h"
#include "app_config.h"
#include "../../channel/main_page/main_page.h"
extern lv_font_t *select_middle_font[3];

#ifdef BLUETOOTH_SUPPORT

#ifdef LVGL_RESOLUTION_240P_SUPPORT
#define BT_LIST_FONT SiYuanHeiTi_Light_3500_12_1b
#else
#define BT_LIST_FONT SIYUANHEITI_LIGHT_3000_28_1B
#endif

#include <bluetooth.h>



//#ifdef SUPPORT_BLUETOOTH
#define BT_DEV_MAX_LEN  32
#define BTSPEAKER_NAME_MAX_SIZE  32
typedef struct {
    char name[BTSPEAKER_NAME_MAX_SIZE];
    uint8_t mode_onoff;
}bt_speaker_info_t;
bt_speaker_info_t bt_speaker_info = {0};

#ifdef BLUETOOTH_SPEAKER_MODE_SUPPORT
typedef enum{
    BT_MODE_ID,
    BT_SPEAKER_ID,
    BT_SEARCH_ID,
    BT_MY_DEV_ID,
    BT_LIST_MAX_ID,
}bt_list_id;
typedef enum{
    BT_SPEAKER_NAME_ID,
    BT_SPEAKER_MODE_ID,
    BT_SPEAKER_MAX_ID,
}bt_speaker_list_id;

static  lv_obj_t* bt_slave_panel_widget;
static lv_obj_t *bt_local_name_widget;
static lv_obj_t *bt_name_set_widget;
static char set_bt_name[BTSPEAKER_NAME_MAX_SIZE];
static uint32_t m_hotkey[] = {KEY_POWER, KEY_VOLUMEUP, KEY_VOLUMEDOWN, KEY_EXIT,\
                            KEY_BACK, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT};

static void bluetooth_rx_mode_preproc(void);
static void win_bluetooth_speaker_create(void);
static void win_btspeaker_name_prompt_create(lv_obj_t* p);
static void open_bt_slave_widget(void) ;
static void keypad_widget_event_handle(lv_event_t* e);
static void win_keyboard_widget_create(lv_obj_t* p, lv_obj_t* ta, lv_event_cb_t event_cb);

#else

typedef enum{
    BT_MODE_ID,
    BT_SEARCH_ID,
    BT_MY_DEV_ID,
    BT_LIST_MAX_ID,
}bt_list_id;

#endif 

typedef enum wait_type_{
    SCAN_WAIT,
    CONN_WAIT,
    POWER_ON_WAIT
} wait_type;

typedef enum conn_type_{
    CONN_TYPE_IS_SAVED,
    CONN_TYPE_NORMAL,
    CONN_TYPE_POWER_ON
} conn_type; //bluetooth_is_connected(),bluetooth_connect(), power_on()都有可能触发BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED事件

enum bt_list_refresh_event{
    BT_LIST_EVENT_ADD_CONN,
    BT_LIST_EVENT_REMOVE_CONN,
    BT_LIST_EVENT_CREATE_NEW_BTN,
    BT_LIST_EVENT_REFRESH_SAVED,
    BT_LIST_EVENT_DEL_BTN,
};

typedef struct{
    int id;
    void* param1;
    void* param2;
} bt_refresh_event_param;

bt_refresh_event_param bt_refresh_param;
static bt_gpio_set_t * lineout_det = NULL;

extern lv_timer_t *timer_setting;
extern lv_font_t* select_font_normal[3];

extern lv_obj_t* slave_scr_obj;
extern lv_obj_t *tab_btns;
extern char* bt_v;
extern SCREEN_TYPE_E cur_scr;

static int sel_id = -1;
static int connected_bt_id = -2;
static bool active_disconn = false;
static wait_type bt_wait_type = -1;
static conn_type bt_conn_type = -1;
static int bt_first_poweron_flag = 0;
/*record bt first power on to do something*/
struct bluetooth_slave_dev devs_info[BT_DEV_MAX_LEN]={0};


lv_obj_t *bluetooth_obj = NULL;
lv_obj_t *wait_anim = NULL;

lv_obj_t* bt_list_obj = NULL;
lv_obj_t* my_dev = NULL;
lv_obj_t* other_dev = NULL;
static bool is_connected = false;
static bool reset_timer = true;//重设置timer_setting

static int found_bt_num=0;
static lv_timer_t * wait_anim_timer = NULL;


static bt_scan_status scan_status = BT_SCAN_STATUS_DEFAULT;
static bt_connect_status_e connet_status = BT_CONNECT_STATUS_DEFAULT;
static bt_connect_status_e mute_connet_status = BT_CONNECT_STATUS_DEFAULT;
static  lv_obj_t* saved_bt_widget;
static lv_obj_t *prev_obj = NULL;

LV_FONT_DECLARE(font_china_22)
LV_IMG_DECLARE(MAINMENU_IMG_BT)

static bool str_is_black(char *str);
static void get_bt_mac(char *name,unsigned char* mac);
static void bluetooth_wait(wait_type type);
static void event_cb(lv_event_t * e);
 
static void remove_bt_dev(int i);
static bool bt_mac_cmp(unsigned char* mac1, unsigned char* mac2);
static void my_bt_dev_event_handle(lv_event_t *e);
static bool bt_mac_invalid(unsigned char* mac);
static void bt_list_change_loc(lv_obj_t *prev, lv_obj_t *next);
static bool bt_has_my_dev();
static   void add_connected(int i);
static void remove_connected(int i);

lv_obj_t* create_bt_list_obj(lv_obj_t *parent, int w, int h);
lv_obj_t* create_list_obj1(lv_obj_t *parent, int w, int h);
static lv_obj_t* create_list_bt_obj(lv_obj_t *parent, int w, int h, lv_obj_t *btn);
static lv_obj_t* create_list_sub_bt_text_obj1(lv_obj_t *parent, int w, int h, int str1);
static lv_obj_t* create_list_sub_obj(lv_obj_t *parent, char *str);
static lv_obj_t* create_list_sub_btn_obj(lv_obj_t *parent);
static lv_obj_t* create_list_sub_btn_obj1(lv_obj_t *parent, int str1, int str2);
static lv_obj_t* create_list_sub_btn_obj2(lv_obj_t *parent, char* str1, int str2);
static void bt_list_sub_obj_name_set(int id, char* name);
static void bt_list_sub_obj_status_set(int id, int);
static void create_list_bt_sub_obj(lv_obj_t *parent, char *str);
static lv_obj_t* create_list_bt_sub_btn_obj(lv_obj_t *parent, list_sub_param, int, int str2);

static void remove_list_sub_obj(lv_obj_t *parent, int id);
static void remove_list_sub_objs(lv_obj_t *parent, int start, int end);
static void hidden_on_list_sub_objs(lv_obj_t *parent, int start, int end);
static void hidden_off_list_sub_obj(lv_obj_t *parent, int start, int end);

static void bt_my_dev_refresh();
extern lv_obj_t* create_page_(lv_obj_t* parent, choose_item * data, int len);
extern void set_remote_control_disable(bool b);

bool app_bt_is_connected(){
    return is_connected;
}

bool app_bt_is_connecting(){
    return connet_status == BT_CONNECT_STATUS_CONNECTING;
}

bool app_bt_is_scanning(){
    return scan_status == BT_SCAN_STATUS_GET_DATA_SEARCHING;
}

void bt_screen_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SCREEN_LOADED){
        if(projector_get_some_sys_param(P_BT_SETTING)){
            bt_my_dev_refresh();
        }
    }
}

static void add_connected(int i){
    lv_obj_t *label = lv_obj_get_child(bt_list_obj, i);
    set_label_text2(lv_obj_get_child(label, 1), STR_BT_CONN, FONT_NORMAL);
}

static void remove_connected(int i){
    lv_obj_t *label = lv_obj_get_child(bt_list_obj, i);
     set_label_text2(lv_obj_get_child(label, 1), STR_BT_DISCONN, FONT_NORMAL);
}

static void remove_bt_dev(int i){
    if(i<0 || i>=BT_DEV_MAX_LEN){
        return;
    }
    for(int j=i; j+1<BT_DEV_MAX_LEN; j++){
        memcpy(&devs_info[j], &devs_info[j+1], sizeof(struct bluetooth_slave_dev));
    }
    found_bt_num-=1;
}

static void add_bt_dev_head(struct bluetooth_slave_dev* dev_p){
    for(int i = BT_DEV_MAX_LEN-2; i >= 0; i--){
        memcpy(&devs_info[i+1], &devs_info[i], sizeof(struct bluetooth_slave_dev));
    }
    memcpy(&devs_info[0], dev_p, sizeof(struct bluetooth_slave_dev));
    if(found_bt_num<BT_DEV_MAX_LEN){
        found_bt_num++;
    }
}


static int contain_bt_dev(char *name){
    for(int i=0; i<found_bt_num; i++){
        if(strcmp(name, devs_info[i].name) == 0){
            return i;
        }
    }
    return -1;
}

static bool bt_mac_cmp(unsigned char* mac1, unsigned char* mac2){
    for(int i=0; i<6; i++){
        if(mac1[i] != mac2[i]){
            return false;
        }
    }
    return true;
}

static void printf_bt_mac(char* mac){
    printf("\n*mac addr: %02x, %02x, %02x, %02x, %02x, %02x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

static void my_bt_dev_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
     
    if(code == LV_EVENT_KEY){
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            int sel = lv_obj_has_state(lv_obj_get_child(target, 0), LV_STATE_CHECKED) ? 0 : 1;
            if(sel == 0 ){              
                lv_obj_del(lv_obj_get_child(bt_list_obj, connected_bt_id));
                projector_bt_dev_delete(connected_bt_id - (int)lv_obj_get_index(my_dev) - 1);
                projector_sys_param_save();
                if(is_connected && connected_bt_id - lv_obj_get_index(my_dev) == 1){
                    bluetooth_disconnect();
                    usleep(1000);
                    bluetooth_del_all_device();
                    is_connected = false;
                }else if(is_connected == false){
                    bluetooth_del_all_device();
                }
                sel_id=BT_SEARCH_ID;                    
            }else{
                if(is_connected && connected_bt_id == (int)lv_obj_get_index(my_dev) + 1){
                    connet_status = BT_CONNECT_STATUS_DISCONNECTING;
                    bluetooth_disconnect();
                    sel_id=connected_bt_id;
                }else{
                    active_disconn = true;
                    bt_conn_type = CONN_TYPE_IS_SAVED;
                    char* mac = projector_get_bt_mac(connected_bt_id - (int)lv_obj_get_index(my_dev) - 1);;
                    connet_status = BT_CONNECT_STATUS_CONNECTING;
                    if(bluetooth_connect(mac)==0){
                        if(bt_mac_invalid(mac)){
                            bluetooth_stop_scan();
                            // bluetooth_disconnect();
                            printf("invalid mac address\n"); 
                            connet_status = BT_CONNECT_STATUS_DEFAULT;   
                            sel_id = connected_bt_id;                       
                        }else{
                            api_set_bt_connet_status(BT_CONNECT_STATUS_CONNECTING);
                            bluetooth_wait(CONN_WAIT);
                            reset_timer = false;                            
                        }
                    }else{
                        connet_status = BT_CONNECT_STATUS_DEFAULT; 
                        bt_conn_type = -1;
                        sel_id = connected_bt_id;
                        create_message_box(api_rsc_string_get(STR_BT_CONN_FAILED)); 
                    }
                }
               
            }
            if(sel_id>-1){
                lv_group_focus_obj(bt_list_obj);
            }
            lv_obj_del(target);
            slave_scr_obj = NULL;
        }else if(key == LV_KEY_DOWN){
            lv_obj_clear_state(lv_obj_get_child(target, 0), LV_STATE_CHECKED);
            lv_obj_add_state(lv_obj_get_child(target, 1), LV_STATE_CHECKED);
        }else if(key == LV_KEY_UP){
            lv_obj_clear_state(lv_obj_get_child(target, 1), LV_STATE_CHECKED);
            lv_obj_add_state(lv_obj_get_child(target, 0), LV_STATE_CHECKED);           
        }else if(key == LV_KEY_ESC){
            sel_id = connected_bt_id;
            lv_group_focus_obj(bt_list_obj);
            lv_obj_del(target);
            slave_scr_obj = NULL;
        }
    }else if(code == LV_EVENT_FOCUSED){
        lv_obj_add_state(lv_obj_get_child(target, 0), LV_STATE_CHECKED);  
    }
}

static int last_focused_id = -1;

static void bt_setting_event_handle1(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        if(timer_setting)
            lv_timer_pause(timer_setting);
        reset_timer = true;
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            if(sel_id == BT_MODE_ID){
                if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_ON){
                    video_delay_ms_zero();
                #if PROJECTER_C2_D3000_VERSION
                    bluetooth_disconnect();
                #else
                    bluetooth_disconnect();
                    bluetooth_poweroff();
                #endif
                    found_bt_num = 0;
                    is_connected = false;
                    api_set_bt_connet_status(BT_CONNECT_STATUS_DEFAULT);
                    api_set_i2so_gpio_mute(false);
                    bluetooth_ioctl(BLUETOOTH_SET_CTRL_CMD,CMD_VALUE_UNMUTE);
                    /* do opt to decide which audio device output */
                    if(lineout_det && lineout_det->value == 0){
                        /*it mean lineout had insert,so mute local speaker output*/
                        api_set_i2so_gpio_mute(true);
                    }
                    connet_status = BT_CONNECT_STATUS_DEFAULT;
                    scan_status = BT_SCAN_STATUS_DEFAULT;
                    projector_set_some_sys_param(P_BT_SETTING, BLUETOOTH_OFF);
                    // update win_bt_list ui 
                    remove_list_sub_objs(bt_list_obj, lv_obj_get_index(other_dev)+1, lv_obj_get_child_cnt(bt_list_obj));
                    bt_my_dev_refresh();    
                    hidden_on_list_sub_objs(bt_list_obj, BT_MODE_ID+1, lv_obj_get_child_cnt(bt_list_obj));                    
                    set_label_text2(lv_obj_get_child(lv_obj_get_child(bt_list_obj, BT_MODE_ID), 1), STR_OFF, FONT_NORMAL);
                }else{
                    api_set_bt_connet_status(BT_CONNECT_STATUS_DEFAULT);
                    if(bluetooth_poweron() == 0){
                        projector_set_some_sys_param(P_BT_SETTING, BLUETOOTH_ON);
                        printf("Device exists\n");
                        hidden_off_list_sub_obj(bt_list_obj, BT_MODE_ID+1,lv_obj_get_child_cnt(bt_list_obj));
                        set_label_text2(lv_obj_get_child(lv_obj_get_child(bt_list_obj, BT_MODE_ID), 1), STR_ON, FONT_NORMAL);
                    #if PROJECTER_C2_D3000_VERSION
                        bluetooth_scan();
                    #endif
                        scan_status = BT_SCAN_STATUS_GET_DATA_SEARCHING;
                        connet_status = BT_CONNECT_STATUS_CONNECTING;
                        bt_conn_type = CONN_TYPE_POWER_ON;
                        active_disconn = true;
                        printf("scan bluettoth!");
                        bluetooth_wait(POWER_ON_WAIT);
                        reset_timer = false;
                    }else{
                        set_label_text2(lv_obj_get_child(lv_obj_get_child(bt_list_obj, BT_MODE_ID), 1), STR_NO_BT, FONT_NORMAL);
                    }
                }
            }
            #ifdef BLUETOOTH_SPEAKER_MODE_SUPPORT
            else if(sel_id == BT_SPEAKER_ID){
                /*check bt_onoff  on->disconect device ,remove ui then create new ui content*/
                if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_ON){
                    bluetooth_rx_mode_preproc();
                    bluetooth_ioctl(BLUETOOTH_GET_LOCAL_NAME,0);
                    api_sleep_ms(200);
                    /*add animation for 200ms so that it can sync for local name */
                    api_hotkey_enable_set(m_hotkey, sizeof(m_hotkey)/sizeof(m_hotkey[0]));
                    bt_my_dev_refresh();
                    win_bluetooth_speaker_create();
                    return;
                }
            }
            #endif 
            else if(sel_id == BT_SEARCH_ID){
                if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_ON){
                    remove_list_sub_objs(bt_list_obj, lv_obj_get_index(other_dev)+1, lv_obj_get_child_cnt(bt_list_obj));
                    found_bt_num = 0;      
                    if(bt_first_poweron_flag){
                        bluetooth_stop_scan();
                        api_sleep_ms(100);
                        bt_first_poweron_flag = 0;
                    }
                    if(bluetooth_scan() == 0){
                        scan_status = BT_SCAN_STATUS_GET_DATA_SEARCHING;
                        active_disconn = true;
                       
                        printf("scan bluettoth!");
                        bluetooth_wait(SCAN_WAIT);
                        reset_timer = false;
                    }
                }
                
            }else if(sel_id>(int)lv_obj_get_index(my_dev) && sel_id < (int)lv_obj_get_index(other_dev)){
                if(connet_status == BT_CONNECT_STATUS_DISCONNECTING){
                    return;
                }
              
                if(projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_OFF){
                    printf("bt off\n");
                    return;
                } 
                saved_bt_widget = create_list_obj1(setup_slave_root, 18, 10);
                lv_obj_set_style_bg_color(saved_bt_widget, lv_palette_darken(LV_PALETTE_GREY, 1),0);
                lv_obj_set_style_bg_opa(saved_bt_widget, LV_OPA_70, 0);   
                lv_obj_t *sub_obj;                  
                if(is_connected && sel_id == (int)lv_obj_get_index(my_dev) + 1){
                    sub_obj = create_list_sub_text_obj1(saved_bt_widget, 100, 50, STR_BT_DELETE, FONT_NORMAL);
                    lv_obj_set_style_text_color(sub_obj, lv_color_white(), 0);
                    sub_obj = create_list_sub_text_obj1(saved_bt_widget, 100, 50, STR_BT_MAKE_DISCONN, FONT_NORMAL);
                    lv_obj_set_style_text_color(sub_obj, lv_color_white(), 0);
                }else{
                    sub_obj = create_list_sub_text_obj1(saved_bt_widget, 100, 50, STR_BT_DELETE, FONT_NORMAL);
                    lv_obj_set_style_text_color(sub_obj, lv_color_white(), 0);
                    sub_obj = create_list_sub_text_obj1(saved_bt_widget, 100, 50,STR_BT_MAKE_CONN, FONT_NORMAL);
                    lv_obj_set_style_text_color(sub_obj, lv_color_white(), 0);
                }
                connected_bt_id = sel_id;
           
                lv_obj_add_event_cb(saved_bt_widget, my_bt_dev_event_handle, LV_EVENT_ALL, 0);
                lv_group_focus_obj(saved_bt_widget); 
                slave_scr_obj = saved_bt_widget;    
                if(timer_setting){
                    lv_timer_reset(timer_setting);
                    lv_timer_resume(timer_setting);
                }              
                    
                
            }else if(sel_id > (int)lv_obj_get_index(other_dev)){
                char *text = lv_label_get_text(lv_obj_get_child(lv_obj_get_child(bt_list_obj, sel_id), 0));
                unsigned char mac[6] = {0};
               
                get_bt_mac(text, mac);

                if(bt_mac_invalid(mac)){
                    printf("invalid mac address!\n");
                    remove_bt_dev(sel_id-(int)lv_obj_get_index(other_dev)-1); 
                    remove_list_sub_obj(bt_list_obj, sel_id); 
                    sel_id = 1;
                    lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
                    return;
                }
                
                active_disconn = true;
                bt_conn_type = CONN_TYPE_NORMAL;
                connet_status = BT_CONNECT_STATUS_CONNECTING;
                if(bluetooth_connect(mac)==0){
                    printf_bt_mac(mac);
                    printf("\n");
                    printf("connect %s\n", text);
                    connected_bt_id = sel_id;
                    // api_set_bt_connet_status(BT_CONNECT_STATUS_CONNECTING);
                    bluetooth_wait(CONN_WAIT);
                    reset_timer = false;
                }else{
                    connet_status == BT_CONNECT_STATUS_DEFAULT;
                    bt_conn_type = -1;
                }

            }
        }else if(key == LV_KEY_DOWN || key == LV_KEY_RIGHT){
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            if(sel_id>BT_MY_DEV_ID){
                lv_label_set_long_mode(lv_obj_get_child(lv_obj_get_child(target, sel_id), 0), LV_LABEL_LONG_DOT);
            }           
            sel_id = sel_id+1;
            while (sel_id<(int)lv_obj_get_child_cnt(target) && (lv_obj_get_child(target, sel_id)->class_p == &lv_list_text_class ||
                    lv_obj_has_flag(lv_obj_get_child(target, sel_id), LV_OBJ_FLAG_HIDDEN))){
                sel_id +=1;
            }
  
            if(sel_id == (int)lv_obj_get_child_cnt(target)){
                lv_group_focus_obj(tab_btns);
                return;
            }   
            if(sel_id>BT_MY_DEV_ID){
                lv_label_set_long_mode(lv_obj_get_child(lv_obj_get_child(target, sel_id), 0), LV_LABEL_LONG_SCROLL_CIRCULAR);
            }                      
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            lv_obj_scroll_to_view(lv_obj_get_child(target, sel_id), false);
        }else if(key == LV_KEY_UP || key == LV_KEY_LEFT){

            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            if(sel_id>BT_MY_DEV_ID){
                lv_label_set_long_mode(lv_obj_get_child(lv_obj_get_child(target, sel_id), 0), LV_LABEL_LONG_DOT);
            } 
            sel_id-=1;
            while (sel_id>-1 && (lv_obj_get_child(target, sel_id)->class_p == &lv_list_text_class ||
            lv_obj_has_flag(lv_obj_get_child(target, sel_id), LV_OBJ_FLAG_HIDDEN))){
                sel_id -=1;
            }
            if(sel_id == -1){
               lv_group_focus_obj(tab_btns);
                return;
            }
            if(sel_id>BT_MY_DEV_ID){
                lv_label_set_long_mode(lv_obj_get_child(lv_obj_get_child(target, sel_id), 0), LV_LABEL_LONG_SCROLL_CIRCULAR);
            }     
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            lv_obj_scroll_to_view(lv_obj_get_child(target, sel_id), false);

        }else if(key == LV_KEY_HOME){
            turn_to_main_scr();
            return;

        }else if(key == LV_KEY_ESC){
            turn_to_main_scr();
            return;
        }
        if(timer_setting && reset_timer){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
    }else if(code == LV_EVENT_FOCUSED){
        if(act_key_code == KEY_UP){
            sel_id = (int)lv_obj_get_child_cnt(target)-1;
            lv_obj_scroll_to_view(lv_obj_get_child(target, sel_id), false);
            while (sel_id > -1 && (lv_obj_get_child(target, sel_id)->class_p == &lv_list_text_class ||
                lv_obj_has_flag(lv_obj_get_child(target, sel_id), LV_OBJ_FLAG_HIDDEN))){
                sel_id-=1;
            }
            if(sel_id>1){
                lv_label_set_long_mode(lv_obj_get_child(lv_obj_get_child(target, sel_id), 0), LV_LABEL_LONG_SCROLL_CIRCULAR);               
            }
        }else if(act_key_code == KEY_DOWN || act_key_code == KEY_OK || act_key_code == KEY_EXIT || act_key_code == KEY_EPG){
            if(sel_id == -1){//没有修改默认值时置为0
                sel_id = 0;
            }
            #ifdef BLUETOOTH_SPEAKER_MODE_SUPPORT
            if(act_key_code == KEY_EXIT && last_focused_id == BT_SPEAKER_ID){
                last_focused_id = -1;
                sel_id = BT_SPEAKER_ID;
            }
            #endif
        }        
        if(sel_id>-1)
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
    }else if(code == LV_EVENT_DEFOCUSED){
        if(sel_id<(int)lv_obj_get_child_cnt(target)){
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
        }
        last_focused_id = sel_id;
        sel_id = -1;
        lv_obj_scroll_to_view(lv_obj_get_child(target, 0), false);

    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
      
    }
}





static bool str_is_black(char *str){
    for(int i=0; i<strlen(str); i++){
        int c = str[i];
        printf("%02x\n", c);
        if( c != 1 && !isspace(c)){
            return false;
        }
    }
    return true;
}

static bool bt_mac_invalid(unsigned char* mac){
    for(int i=0; i<6; i++){
        printf("%2x ", mac[i]);
        if(mac[i] != 0){
            return false;
        }
    }
    printf("\n");
    return true;
}

static bool bt_dev_contained(char *mac){
    for(int i=0; i<found_bt_num; i++){
        if(strncmp(mac, devs_info[i].mac, 6) == 0){
            return true;
        }
    }
    return false;
}

static void bt_list_change_loc(lv_obj_t *prev, lv_obj_t *next){
    int begin_id = lv_obj_get_index(prev);
    int end_id = lv_obj_get_index(next);
    for(int i=end_id; i>=0 && i<lv_obj_get_child_cnt(bt_list_obj) && i>begin_id; i--){
        lv_obj_swap(lv_obj_get_child(bt_list_obj, i), lv_obj_get_child(bt_list_obj, i-1));
    }
}

static bool bt_has_my_dev(){
    return lv_obj_get_index(my_dev)+1 != lv_obj_get_index(other_dev);
}


static void bt_my_dev_refresh(){
    int id = lv_obj_get_index(my_dev)+1;
    int size = lv_obj_get_index(other_dev);
    for(int i = 0; i < MAX_BT_SAVE; i++){
        struct bluetooth_slave_dev* dev_p = projector_get_bt_dev(i);
        if(!dev_p || strlen(dev_p->name) == 0){
            for(int j = id; id < size; j++){
                lv_obj_del(lv_obj_get_child(bt_list_obj, j));
            }
            return;
        }
        int bt_status = i == 0 && is_connected ? STR_BT_CONN : STR_BT_DISCONN;
        if(id < size){
            bt_list_sub_obj_name_set(id, dev_p->name);
            bt_list_sub_obj_status_set(id, bt_status);
            id++;
        }else{
            list_sub_param pa;
            pa.str = dev_p->name;
            
            lv_obj_t * obj = create_list_bt_sub_btn_obj(bt_list_obj, pa, LIST_PARAM_TYPE_STR, bt_status);
            lv_obj_move_to_index(obj, lv_obj_get_index(other_dev));        
        }

    }
}

static void bt_other_dev_refresh(){
    for(int i = lv_obj_get_index(other_dev) + 1; i < lv_obj_get_child_cnt(bt_list_obj);){
        lv_obj_del(lv_obj_get_child(bt_list_obj, i));
    }

    for(int i = 0; i < found_bt_num; i++){
        list_sub_param pa;
        pa.str = devs_info[i].name;
        lv_obj_t * obj = create_list_bt_sub_btn_obj(bt_list_obj, pa, LIST_PARAM_TYPE_STR, STR_NONE);
    }
}

static bool bt_device_name_is_same(char* dev_name)
{
    if(dev_name){
        for(int i = 0; i < found_bt_num; i++){
            if(devs_info[i].name){
                if(strcmp(devs_info[i].name, dev_name) == 0){
                    return true;
                }
            }
        }
    }
    return false;
}


int bt_event(unsigned long event, unsigned long param){
    int bt_device_state=-1;
    static bool get_connected_msg = false;
    control_msg_t ctl_msg = {0};
     switch (event){
        case BLUETOOTH_EVENT_SLAVE_DEV_SCANNED:
            printf("BT_AD6956F_EVENT_SLAVE_DEV_SCANNED\n");
            scan_status=BT_SCAN_STATUS_GET_DATA_SEARCHED;
            connet_status=BT_CONNECT_STATUS_DEFAULT;
            struct bluetooth_slave_dev* dev_p = (struct bluetooth_slave_dev*)param;
            printf_bt_mac(dev_p->mac);
            if(param==0)break;
            if(found_bt_num>=BT_DEV_MAX_LEN){
                break;
            }
            if(strlen(dev_p->name) == 0 || str_is_black(dev_p->name)){
                break;
            }
            if(!wait_anim_timer){
                break;
            }
            if(bt_dev_contained(dev_p->mac)){
                break;
            }
            if(projector_get_bt_by_mac(dev_p->mac)){
                break;
            }

            memcpy(devs_info+found_bt_num, dev_p,sizeof(struct bluetooth_slave_dev));
            printf("dev %d: %s\n",found_bt_num, dev_p->name);
            if(bt_list_obj){
                ctl_msg.msg_type = MSG_TYPE_BT_SCANED;
                api_control_send_msg(&ctl_msg);
            }
               found_bt_num += 1;
            scan_status = BT_SCAN_STATUS_GET_DATA_SEARCHING;
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED\n");
            scan_status = BT_SCAN_STATUS_GET_DATA_FINISHED;
            connet_status=BT_CONNECT_STATUS_DEFAULT;
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED 1\n");
            
            api_set_bt_connet_status(BT_CONNECT_STATUS_DISCONNECTED);
            api_set_i2so_gpio_mute(false);//api_set_i2so_gpio_mute_auto();
            bluetooth_ioctl(BLUETOOTH_SET_CTRL_CMD,CMD_VALUE_UNMUTE);
            video_delay_ms_zero();
            /* do opt to decide which audio device output */
            if(lineout_det && lineout_det->value == 0){
                /*it mean lineout had insert,so mute local speaker output*/
                api_set_i2so_gpio_mute(true);
            }
            
            if(is_connected){
                is_connected = false;
                ctl_msg.msg_type = MSG_TYPE_BT_DISCONNECTED;
                api_control_send_msg(&ctl_msg);                

            }
            if(connet_status != BT_CONNECT_STATUS_CONNECTING){
                connet_status = BT_CONNECT_STATUS_DISCONNECTED;
            }
            break;
        case BLUETOOTH_EVENT_SLAVE_REPORT_LOCALNAME:
            strcpy(bt_speaker_info.name , (char*)param);
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED:
            printf("BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED \n");
            get_connected_msg = true;
            break;
        case BLUETOOTH_EVENT_SLAVE_DEV_GET_CONNECTED_INFO:
            bluetooth_ioctl(BLUETOOTH_GET_DEVICE_STATUS, &bt_device_state);
            if (bt_device_state==EBT_DEVICE_STATUS_WORKING_ON_RX_MODE) {
                return 0;
            }
            active_disconn = false;          
            printf("%s connected\n",((struct bluetooth_slave_dev*)param)->name);
            printf_bt_mac(((struct bluetooth_slave_dev*)param)->mac);
            if(!is_connected && get_connected_msg){
                is_connected = true;
                get_connected_msg = false;

                if(bt_conn_type == CONN_TYPE_NORMAL){
                    remove_bt_dev(connected_bt_id-(int)lv_obj_get_index(other_dev)-1);
                    sel_id = BT_MY_DEV_ID+1;
                    if(projector_get_saved_bt_count() >= MAX_BT_SAVE){
                        add_bt_dev_head(projector_get_bt_dev(MAX_BT_SAVE-1));
                    }
                }else{
                    int id = contain_bt_dev(((struct bluetooth_slave_dev*)param)->name);
                    if(id >= 0){
                        remove_bt_dev(id);
                    }
                }
                projector_save_bt_dev((struct bluetooth_slave_dev*)param);                
                ctl_msg.msg_type = MSG_TYPE_BT_CONNECTED;
                api_control_send_msg(&ctl_msg);                
            }
            api_set_bt_connet_status(BT_CONNECT_STATUS_CONNECTED);
            api_set_i2so_gpio_mute(true);//api_set_i2so_gpio_mute_auto();
            bluetooth_ioctl(BLUETOOTH_SET_CTRL_CMD,CMD_VALUE_MUTE);
            video_delay_ms_set();
            
            bt_conn_type = -1;
            connected_bt_id = -1;
            connet_status = BT_CONNECT_STATUS_CONNECTED;
            break;
        case BLUETOOTH_EVENT_REPORT_GPI_STAT:
            {
                lineout_det=(bt_gpio_set_t*)param;
                if(is_connected==false){
                    // mean it has not connet to bt dev
                    if(lineout_det->pinpad==LINEOUTDET_PINPAD && lineout_det->value==0){
                        api_set_i2so_gpio_mute(true);
                        // let line out product sound,mute the local speaker
                    }else if(lineout_det->pinpad==LINEOUTDET_PINPAD && lineout_det->value==1){
                        api_set_i2so_gpio_mute(false);
                        // let the local speaker product sound,mute the line out 
                    }
                }
                break;
            }
        }
    return 0;
}
    

void setup_bt_control(void *arg1, void *arg2){
    (void)arg2;
     control_msg_t *ctl_msg = (control_msg_t*)arg1;

    switch (ctl_msg->msg_type){
        case MSG_TYPE_BT_SCANED:
            bt_other_dev_refresh();
            break;
        case MSG_TYPE_BT_CONNECTED:
            bt_my_dev_refresh();
            bt_other_dev_refresh();
            break;
        case MSG_TYPE_BT_DISCONNECTED:
            bt_my_dev_refresh();
            if(bt_conn_type == CONN_TYPE_POWER_ON){
                create_message_box(api_rsc_string_get(STR_BT_CONN_FAILED));
            }
            break;
        case MSG_TYPE_BT_SCAN_FINISH:
            break;
    }
}



static lv_obj_t* create_list_bt_sub_btn_obj(lv_obj_t *parent,list_sub_param param1,int type1,  int str2){
    
    if(type1 == LIST_PARAM_TYPE_INT){
        return create_list_sub_btn_obj1(parent, param1.str_id, str2);
    }else if(type1 == LIST_PARAM_TYPE_STR){
        return create_list_sub_btn_obj2(parent, param1.str, str2);
    }
    
}



static lv_obj_t* create_list_sub_obj(lv_obj_t *parent, char *str){
    lv_obj_t *list_btn;
    list_btn = lv_list_add_text(parent, str);


    lv_obj_set_size(list_btn,100,17);
    lv_obj_set_style_pad_top(list_btn, 5, 0);
    lv_obj_set_style_text_align(list_btn, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_set_style_bg_color(list_btn, lv_color_make(101,101,177), 0);
    lv_obj_set_style_border_width(list_btn, 3, 0);
    lv_obj_set_style_border_color(list_btn, lv_color_make(101,101,177), 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_50, 0);
    lv_obj_set_style_bg_color(list_btn, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);
    lv_obj_set_style_text_color(list_btn, lv_color_white(), 0);
    lv_obj_set_style_text_color(list_btn, lv_color_black(), LV_STATE_CHECKED);
 
    return list_btn;
}


static void bt_list_sub_obj_name_set(int id, char* name){
    lv_label_set_text(lv_obj_get_child(lv_obj_get_child(bt_list_obj, id), 0), name);
}

static void bt_list_sub_obj_status_set(int id, int str_id){
    set_label_text2(lv_obj_get_child(lv_obj_get_child(bt_list_obj, id), 1), str_id, FONT_NORMAL);  
}


static lv_obj_t* create_list_sub_btn_obj(lv_obj_t *parent){
    lv_obj_t *list_btn;
    list_btn = lv_list_add_btn(parent, NULL, " ");
    lv_group_remove_obj(list_btn);

    lv_obj_set_size(list_btn,LV_PCT(100),LV_PCT(11));
    lv_obj_set_style_border_side(list_btn, LV_BORDER_SIDE_FULL, 0);
    lv_obj_set_style_border_width(list_btn, 2, 0);
    lv_obj_set_style_border_color(list_btn, lv_color_white(), 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(list_btn, LV_OPA_100, LV_STATE_CHECKED);

    lv_obj_set_style_bg_opa(list_btn, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(list_btn, LV_OPA_100, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(list_btn, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);

    lv_obj_set_style_text_color(list_btn, lv_color_white(), 0);
    lv_obj_set_style_text_color(list_btn, lv_color_black(), LV_STATE_CHECKED);

    lv_obj_t* label = lv_obj_get_child(list_btn, 0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_flex_grow(label, 9);   

    label = lv_label_create(list_btn);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_flex_grow(label, 5);  
    lv_obj_set_width(label, LV_SIZE_CONTENT);   
    return list_btn;
}

static lv_obj_t* create_list_sub_btn_obj1(lv_obj_t *parent, int str1, int str2){

    lv_obj_t *list_btn = create_list_sub_btn_obj(parent);


    lv_obj_t *label = lv_obj_get_child(list_btn, 0);
    if(str1>=0){  
        set_label_text2(label, str1, FONT_NORMAL);     
    }else{
        lv_label_set_text(label, " ");
    }

    label = lv_obj_get_child(list_btn, 1);
    if(str2>=0){       
        set_label_text2(label, str2, FONT_NORMAL);
    }else{
        lv_label_set_text(label, " ");
    }

    return list_btn;
}

static lv_obj_t* create_list_sub_btn_obj2(lv_obj_t *parent, char* str1, int str2){
    lv_obj_t *list_btn = create_list_sub_btn_obj(parent);

    lv_obj_t *label = lv_obj_get_child(list_btn, 0);
    lv_label_set_text(label, str1);
    lv_obj_set_style_text_font(label,&LISTFONT_3000, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    
    label = lv_obj_get_child(list_btn, 1);

    if(str2>=0){      
         set_label_text2(label, str2, FONT_NORMAL);
    }else{
        lv_label_set_text(label, " ");
    }

    return list_btn;
}

static void remove_list_sub_obj(lv_obj_t *parent, int id){  
    if(id >= lv_obj_get_child_cnt(parent)){
        return;
    }
    lv_obj_del(lv_obj_get_child(parent, id));
}

static void remove_list_sub_objs(lv_obj_t *parent, int start, int end){
    if(start>=lv_obj_get_child_cnt(parent)){
        return;
    }
    for(int i=end-1; i>=start; i--){
        remove_list_sub_obj(parent, i);
    }
}

static void hidden_off_list_sub_obj(lv_obj_t *parent, int start, int end){
    if(start>=lv_obj_get_child_cnt(parent)){
        return;
    }
    for(int i=end-1; i>=start; i--){
        lv_obj_clear_flag(lv_obj_get_child(parent, i), LV_OBJ_FLAG_HIDDEN);
    }
}

static void hidden_on_list_sub_objs(lv_obj_t *parent, int start, int end){
    if(start>=lv_obj_get_child_cnt(parent)){
        return;
    }
    for(int i=end-1; i>=start; i--){
        lv_obj_add_flag(lv_obj_get_child(parent, i), LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t* create_bt_list_obj(lv_obj_t *parent, int w, int h){
    lv_obj_t *obj = lv_list_create(parent);
    lv_obj_set_style_radius(obj, 0, 0);
    set_pad_and_border_and_outline(obj);
    lv_obj_set_style_pad_ver(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_scroll_to_view(obj, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(obj, lv_color_make(101,101,177), 0);
    lv_obj_set_size(obj, LV_PCT(w), LV_PCT(h));
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_group_add_obj(lv_group_get_default(), obj);
    lv_group_focus_obj( obj);
    return obj;
}




lv_obj_t* create_list_bt_obj1(lv_obj_t *parent, int w, int h){
    lv_obj_t* obj = create_list_obj1(parent, w, h);
    lv_obj_add_event_cb(obj, bt_setting_event_handle1, LV_EVENT_ALL, NULL);
    return obj;
}

static lv_obj_t* create_list_sub_bt_text_obj1(lv_obj_t *parent, int w, int h, int str1){
    lv_obj_t *obj = create_list_sub_text_obj1(parent, w, h, str1, FONT_NORMAL);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT,0);
    lv_obj_set_size(obj, LV_PCT(w), LV_PCT(h));
    lv_obj_set_style_pad_left(obj, 10, 0);
    return obj;
}



static void get_bt_mac(char *name,unsigned char* mac){
    for(int i=0; i<found_bt_num; i++){
        if(strcmp(name, devs_info[i].name) == 0){
            memcpy(mac,devs_info[i].mac, 6);
        }
    }
}

static bool del_wait = false;
static uint total_wait_time = 0;
static void bluetooth_wait_timer_handle(lv_timer_t *timer_){
    static uint8_t i = 0;
    
   
    int radius1 = lv_disp_get_hor_res(lv_disp_get_default())/100*1.6;
    int radius2 = lv_disp_get_hor_res(lv_disp_get_default())/50;
    total_wait_time += 500;
    if(timer_setting){
        lv_timer_pause(timer_setting);
    }
    
    if( ( bt_wait_type == CONN_WAIT ? total_wait_time > 10000 : total_wait_time > 20000 )||
        (bt_wait_type == POWER_ON_WAIT && (scan_status == BT_SCAN_STATUS_GET_DATA_FINISHED || connet_status == BT_CONNECT_STATUS_CONNECTED)) ||
        (bt_wait_type == SCAN_WAIT && (scan_status == BT_SCAN_STATUS_GET_DATA_FINISHED)) ||
        (bt_wait_type == CONN_WAIT && connet_status == BT_CONNECT_STATUS_CONNECTED) ){
        i=0;
        printf("total_wait_timer: %d", total_wait_time);
        if(bt_wait_type == CONN_WAIT ? total_wait_time > 10000 : total_wait_time > 20000){
            //is_connected = false;
            sel_id=0;
            
            if(scan_status == BT_SCAN_STATUS_GET_DATA_SEARCHING || scan_status == BT_SCAN_STATUS_GET_DATA_SEARCHED){
                printf("scan_status: %d\n", scan_status); 
                bluetooth_stop_scan();
            }
            else if(connet_status == BT_CONNECT_STATUS_CONNECTING){
                printf("connet_status: %d\n", connet_status);
                create_message_box(api_rsc_string_get(STR_BT_CONN_FAILED));
                if(connected_bt_id>(int)lv_obj_get_index(other_dev)){
                    connected_bt_id = -1;
                  
                }
           }
        }
        total_wait_time = 0;
        bt_conn_type = -1;
        if(timer_setting){
            lv_timer_resume(timer_setting);
            lv_timer_reset(timer_setting); 
        }   
        scan_status = BT_SCAN_STATUS_DEFAULT;
        if(connet_status != BT_CONNECT_STATUS_CONNECTED){
            connet_status = BT_CONNECT_STATUS_DISCONNECTED;
        }
        
        
        if(wait_anim_timer){
            lv_timer_pause(wait_anim_timer);
            lv_timer_del(wait_anim_timer);
            wait_anim_timer = NULL;
        }
        if(wait_anim){
            lv_obj_add_flag(wait_anim, LV_OBJ_FLAG_HIDDEN);
        }
        lv_group_focus_obj(bt_list_obj);
        set_remote_control_disable(false);
        return;
    }
    
    if(wait_anim){
        lv_obj_set_style_radius(lv_obj_get_child(wait_anim, i), radius1, 0);
        lv_obj_set_size(lv_obj_get_child(wait_anim, i), LV_PCT(20), LV_PCT(40));
        lv_obj_set_style_bg_color(lv_obj_get_child(wait_anim, i), lv_palette_lighten(LV_PALETTE_GREY, 2), 0);
    

        i = (i+1)%3;
        lv_obj_set_style_radius(lv_obj_get_child(wait_anim, i), radius2, 0);
        lv_obj_set_size(lv_obj_get_child(wait_anim, i), LV_PCT(26), LV_PCT(50));
        lv_obj_set_style_bg_color(lv_obj_get_child(wait_anim, i), lv_palette_main(LV_PALETTE_BLUE), 0);        
    }

    if(bt_wait_type == SCAN_WAIT){
        scan_status = BT_SCAN_STATUS_GET_DATA_SEARCHING;
   }else if(bt_wait_type == POWER_ON_WAIT){

   }
}

void del_bt_wait_anim(){
    if(wait_anim_timer){
        lv_timer_pause(wait_anim_timer);
        lv_timer_del(wait_anim_timer);
        wait_anim_timer = NULL;
    }       
    if(wait_anim ){
        lv_obj_add_flag(wait_anim, LV_OBJ_FLAG_HIDDEN);
    }
    
    if(bt_wait_type == SCAN_WAIT && cur_scr != SCREEN_SETUP){
        bluetooth_stop_scan(); 
    }

}

static void bluetooth_wait_handle(lv_event_t *e){
    lv_obj_t *target = lv_event_get_current_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
           if(key == LV_KEY_ESC && (bt_wait_type == POWER_ON_WAIT || bt_wait_type == SCAN_WAIT ) && scan_status != BT_SCAN_STATUS_GET_DATA_SEARCHED){
            printf("stop scan\n");
            bluetooth_stop_scan();  
            scan_status = BT_SCAN_STATUS_DEFAULT;
            sel_id = 0;
            total_wait_time = 0;           
            lv_group_focus_obj(bt_list_obj);
            if(wait_anim_timer){
                lv_timer_pause(wait_anim_timer);
                lv_timer_del(wait_anim_timer);
                wait_anim_timer = NULL;
            }
            if(wait_anim){
                lv_obj_add_flag(wait_anim, LV_OBJ_FLAG_HIDDEN);
            } 
        }
    }
}

void BT_first_power_on(){
    api_set_bt_connet_status(BT_CONNECT_STATUS_DEFAULT);
        bt_get_local_name();//zhp test
    if(projector_get_some_sys_param(P_BT_SETTING)){
        bt_conn_type = CONN_TYPE_POWER_ON;
        if(bluetooth_poweron() == 0){
            printf("Device exist\n");
            unsigned char *mac = projector_get_bt_mac(0);
            printf("bt name is : %s\n", projector_get_bt_name(0));            
            for(int i=0; i<6; i++){
                printf("%02x,", mac[i]);
            }            
            #if PROJECTER_C2_D3000_VERSION
            if(bluetooth_connect(mac) == 0){
                if(bt_mac_invalid(mac)){
                    bluetooth_stop_scan();
                    printf("invalid mac address\n");
                    return;
                }
                    printf("mac\n");
                    connet_status = BT_CONNECT_STATUS_CONNECTING;
            }else{
                bluetooth_stop_scan();
                bt_conn_type = -1;
            }
            #else
                connet_status = BT_CONNECT_STATUS_CONNECTING;
            #endif
        }else{
            bt_conn_type = -1;
        }
    }else{
    #if PROJECTER_C2_D3000_VERSION
        bluetooth_poweron();
    #else
        bluetooth_poweroff();
    #endif
        hidden_on_list_sub_objs(bt_list_obj, BT_MODE_ID+1, lv_obj_get_child_cnt(bt_list_obj));
		api_set_i2so_gpio_mute(false);
    }
    bt_first_poweron_flag = 1;
}



static void bluetooth_wait(wait_type type){
    if(wait_anim){
        lv_obj_clear_flag(wait_anim, LV_OBJ_FLAG_HIDDEN);
    }else{
        wait_anim = lv_obj_create(other_dev);
        lv_obj_set_size(wait_anim, LV_PCT(16), LV_PCT(100));
        lv_obj_align(wait_anim, LV_ALIGN_TOP_MID, 0,0);
        lv_obj_set_style_bg_opa(wait_anim, LV_OPA_0, 0);
        lv_obj_set_style_pad_all(wait_anim, 0, 0);
        lv_obj_set_style_border_width(wait_anim, 0, 0);
        lv_obj_set_style_outline_width(wait_anim, 0, 0);
        lv_obj_set_scrollbar_mode(wait_anim, LV_SCROLLBAR_MODE_OFF);
        // prev_obj = lv_group_get_focused(lv_group_get_default());
        lv_group_add_obj(lv_group_get_default(), wait_anim);
        
        lv_obj_add_event_cb(wait_anim, bluetooth_wait_handle, LV_EVENT_ALL, 0);


        lv_obj_t *ball = lv_obj_create(wait_anim);
        lv_obj_set_scrollbar_mode(ball, LV_SCROLLBAR_MODE_OFF);
        lv_obj_align(ball, LV_ALIGN_LEFT_MID,0, 0);
        int radius = lv_disp_get_hor_res(lv_disp_get_default())/100*1.6;
        lv_obj_set_style_radius(ball, radius, 0);
        lv_obj_set_size(ball, LV_PCT(20), LV_PCT(40));

        lv_obj_set_style_bg_color(ball, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);

        ball = lv_obj_create(wait_anim);
        lv_obj_set_scrollbar_mode(ball, LV_SCROLLBAR_MODE_OFF);
        lv_obj_align(ball, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_radius(ball, radius, 0);
        lv_obj_set_size(ball, LV_PCT(20), LV_PCT(40));
        lv_obj_set_style_bg_color(ball, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);

        ball = lv_obj_create(wait_anim);
        lv_obj_set_scrollbar_mode(ball, LV_SCROLLBAR_MODE_OFF);
        lv_obj_align(ball, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_style_radius(ball, radius, 0);
        lv_obj_set_size(ball, LV_PCT(20), LV_PCT(40));
        lv_obj_set_style_bg_color(ball, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);          
    }
    lv_group_focus_obj(wait_anim);
    bt_wait_type = type;
    
    if (wait_anim_timer == NULL)
    {
        wait_anim_timer = lv_timer_create(bluetooth_wait_timer_handle, 500, (void*)wait_anim);
        lv_timer_reset(wait_anim_timer);       
    }else{
        lv_timer_resume(wait_anim_timer);
        lv_timer_reset(wait_anim_timer);
    }
    

 
    if(timer_setting){
        lv_timer_pause(timer_setting);
        printf("pause timer\n");
    }
        
    if(bt_wait_type == CONN_WAIT){
        set_remote_control_disable(true);
    }
}





void bt_init(){
#ifdef __linux__
    #define BLUETOOTH_CONFIG_PATH   "/proc/device-tree/bluetooth"
    char devpath[16]={0};
    char status[16]={0};
    api_dts_string_get(BLUETOOTH_CONFIG_PATH "/status", status, sizeof(status));
    if(!strcmp(status,"okay")){
        api_dts_string_get(BLUETOOTH_CONFIG_PATH "/devpath",devpath,sizeof(devpath));
    }
    if(bluetooth_init(devpath, bt_event) == 0){
        printf("%s %d bluetooth_init ok\n",__FUNCTION__,__LINE__);
    }else{
        printf("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);
    }
#else
    const char *devpath=NULL;
    int np = fdt_node_probe_by_path("/hcrtos/bluetooth");
    if(np>0)
    {
        if(!fdt_get_property_string_index(np, "devpath", 0, &devpath))
        {
            if(bluetooth_init(devpath, bt_event) == 0){
                printf("%s %d bluetooth_init ok\n",__FUNCTION__,__LINE__);
            }else{
                printf("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);
            }
        }
    }
    else
        printf("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);
#endif 

    bluetooth_ioctl(BLUETOOTH_SET_DEFAULT_CONFIG,NULL);
}

lv_obj_t* create_bt_page(lv_obj_t* parent){
    lv_obj_t *obj = create_page_(parent, NULL, 0);    
    bt_list_obj = create_list_bt_obj1(obj, 82, 100);
    lv_obj_set_style_pad_all(bt_list_obj, 0, 0);
    int bt_model_str2 = projector_get_some_sys_param(P_BT_SETTING) == BLUETOOTH_ON ?
       STR_ON : STR_OFF;
    list_sub_param param;
    param.str_id = STR_BT_SETTING;
    create_list_bt_sub_btn_obj(bt_list_obj, param, LIST_PARAM_TYPE_INT,bt_model_str2);
    #ifdef BLUETOOTH_SPEAKER_MODE_SUPPORT
    param.str_id = STR_BT_SPEAKER_SET;
    create_list_bt_sub_btn_obj(bt_list_obj, param, LIST_PARAM_TYPE_INT,-1);
    #endif
    param.str_id = STR_SEARCH_BT;
    create_list_bt_sub_btn_obj(bt_list_obj, param, LIST_PARAM_TYPE_INT,BLANK_SPACE_STR);
    my_dev = create_list_sub_bt_text_obj1(bt_list_obj,100,11, STR_BT_MY_DEV);
    char* mac = projector_get_bt_mac(0);
    if(!bt_mac_invalid(mac)){
        param.str = projector_get_bt_name(0);
        create_list_bt_sub_btn_obj(bt_list_obj, param, LIST_PARAM_TYPE_STR, STR_BT_DISCONN); 
    }
    other_dev = create_list_sub_bt_text_obj1(bt_list_obj,100,11, STR_BT_OTHER_DEV);
    lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(parent, 0), 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    set_label_text2(label, STR_BT_DEVICE, FONT_NORMAL);
    lv_obj_t *icon = lv_img_create(lv_obj_get_child(parent, 1));
    lv_img_set_src(icon, &MAINMENU_IMG_BT);
    lv_obj_center(icon);
}


#ifdef BLUETOOTH_SPEAKER_MODE_SUPPORT

static void bluetooth_rx_mode_preproc(void)
{
    if(is_connected){
        bluetooth_disconnect();
        bluetooth_del_all_device();
        /*do not reconnet it*/
        found_bt_num = 0;
        is_connected = false;
        api_set_bt_connet_status(BT_CONNECT_STATUS_DEFAULT);
        api_set_i2so_gpio_mute(false);
        connet_status = BT_CONNECT_STATUS_DEFAULT;
        scan_status = BT_SCAN_STATUS_DEFAULT;
    }
}

LV_IMG_DECLARE(Bluetooth_Speaker);
//static int bt_slave_sel_id = 0;
static void win_bluetooth_speaker_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    static int bt_slave_sel_id = 0;
    if(code == LV_EVENT_KEY){
        /* do logic into bt_rx mode ui connent */
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER){
            if (bt_slave_sel_id == BT_SPEAKER_NAME_ID) {
                /*keyboard input and keyboard create,to do*/
                win_btspeaker_name_prompt_create(setup_slave_root);
            }else if (bt_slave_sel_id == BT_SPEAKER_MODE_ID) {
                /* close backlight ,power on to rx mode  then create new ui connent */
                if(!bluetooth_ioctl(BLUETOOTH_SET_BT_POWER_ON_TO_RX,0)){
                    bluetooth_set_gpio_backlight(0); 
                    bt_speaker_info.mode_onoff == 1 ;
                } 
                lv_label_set_text(lv_obj_get_child(lv_obj_get_child(bt_slave_panel_widget, BT_SPEAKER_MODE_ID), 1), (char*)api_rsc_string_get(STR_ON));
                open_bt_slave_widget();
            }
        }else if(key == LV_KEY_DOWN){
            lv_obj_t* label = lv_obj_get_child(lv_obj_get_child(target, BT_SPEAKER_NAME_ID),1);
            if(bt_slave_sel_id == BT_SPEAKER_MODE_ID ){
                lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);   
            }else{
                lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
            }
            lv_obj_clear_state(lv_obj_get_child(target, bt_slave_sel_id), LV_STATE_CHECKED);
            bt_slave_sel_id = bt_slave_sel_id >= BT_SPEAKER_MODE_ID ? BT_SPEAKER_NAME_ID: bt_slave_sel_id+1;
            lv_obj_add_state(lv_obj_get_child(target, bt_slave_sel_id), LV_STATE_CHECKED);
        }else if(key == LV_KEY_UP){
            lv_obj_t* label = lv_obj_get_child(lv_obj_get_child(target, BT_SPEAKER_NAME_ID),1);
            if(bt_slave_sel_id == BT_SPEAKER_MODE_ID ){
                lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);   
            }else{
                lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
            }
            lv_obj_clear_state(lv_obj_get_child(target, bt_slave_sel_id), LV_STATE_CHECKED);
            bt_slave_sel_id = bt_slave_sel_id <= BT_SPEAKER_NAME_ID ? BT_SPEAKER_MODE_ID : bt_slave_sel_id-1;
            lv_obj_add_state(lv_obj_get_child(target, bt_slave_sel_id), LV_STATE_CHECKED);
        }else if(key == LV_KEY_ESC|| key == LV_KEY_HOME){
            lv_obj_clear_flag(setup_root, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clean(setup_slave_root);
            lv_group_focus_obj(bt_list_obj);
            if(timer_setting && reset_timer){
                lv_timer_reset(timer_setting);
                lv_timer_resume(timer_setting);
            }
            api_hotkey_disable_clear();
        }
    }else if(code == LV_EVENT_FOCUSED){
        if (bt_slave_sel_id != 1) {
            bt_slave_sel_id=0;
        }
        lv_obj_t* label = lv_obj_get_child(lv_obj_get_child(target, BT_SPEAKER_NAME_ID),1);
        if(bt_slave_sel_id == BT_SPEAKER_MODE_ID ){
            lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);   
        }else{
            lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        }
        lv_obj_add_state(lv_obj_get_child(target, bt_slave_sel_id), LV_STATE_CHECKED); 
    }
}

static void win_bluetooth_speaker_create(void)
{
    lv_obj_add_flag(setup_root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t* obj = lv_obj_create(setup_slave_root);
    lv_obj_set_size(obj,lv_pct(100),lv_pct(100));
    lv_obj_set_style_border_width(obj,0,0);
    lv_obj_set_style_bg_color(obj, lv_color_make(101,101,177), 0);
    lv_obj_t* speaker_img = lv_img_create(obj);
    lv_img_set_src(speaker_img,&Bluetooth_Speaker);
    lv_obj_align(speaker_img,LV_ALIGN_CENTER,0,-100);

    bt_slave_panel_widget = create_list_obj1(obj, 50, 18);
    lv_obj_align_to(bt_slave_panel_widget,speaker_img,LV_ALIGN_OUT_BOTTOM_MID,0,20);
    lv_obj_set_style_bg_color(bt_slave_panel_widget, lv_color_make(101,101,177), 0);    
    list_sub_param param;
    param.str_id = STR_BT_SPEAKER_NAME;
    lv_obj_t* list_btn = create_list_bt_sub_btn_obj(bt_slave_panel_widget, param, LIST_PARAM_TYPE_INT,-1);
    lv_obj_set_size(list_btn,lv_pct(100),lv_pct(50));
    bt_local_name_widget=lv_obj_get_child(lv_obj_get_child(bt_slave_panel_widget, BT_SPEAKER_NAME_ID), 1);
    lv_label_set_text(bt_local_name_widget, bt_speaker_info.name);
    lv_obj_set_size(bt_local_name_widget, lv_pct(60),lv_pct(100));
    lv_label_set_long_mode(bt_local_name_widget,LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_obj_set_style_text_font(bt_local_name_widget, &LISTFONT_3000, 0);
    param.str_id = STR_BT_SPEAKER_MODE;
    lv_obj_t* list_btn2 = create_list_bt_sub_btn_obj(bt_slave_panel_widget, param, LIST_PARAM_TYPE_INT,STR_OFF);
    lv_obj_set_size(list_btn2,lv_pct(100),lv_pct(50));   
    lv_obj_add_event_cb(bt_slave_panel_widget, win_bluetooth_speaker_event_cb, LV_EVENT_ALL, 0);
    lv_group_focus_obj(bt_slave_panel_widget);
}


static lv_obj_t* contain = NULL;
static lv_obj_t* kb = NULL;
static lv_obj_t* kb_textarea = NULL;

static void keypad_widget_event_handle(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    static uint sel_id = 0;
    if(code == LV_EVENT_KEY) {
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            lv_obj_del(kb);
            kb = NULL;
            lv_obj_center(contain);
            lv_group_focus_obj(lv_obj_get_child(contain, 1));
        }else if(key == LV_KEY_ENTER){
            const char * txt = lv_btnmatrix_get_btn_text(obj, lv_btnmatrix_get_selected_btn(obj));
            if (strcmp(txt, LV_SYMBOL_OK) == 0){
                char* bt_name=lv_textarea_get_text(((lv_keyboard_t*)obj)->ta);
                if (strlen(bt_name) > 0 && strlen(bt_name) < BTSPEAKER_NAME_MAX_SIZE) {
                    if (strcmp(bt_name, bt_speaker_info.name)) {
                        memset(bt_speaker_info.name,0, BTSPEAKER_NAME_MAX_SIZE);
                        strcpy(bt_speaker_info.name , bt_name);
                        if(!bluetooth_ioctl(BLUETOOTH_SET_LOCAL_NAME, bt_speaker_info.name)){
                            lv_obj_del(kb);
                            kb = NULL;
                            lv_obj_del(contain);
                            lv_label_set_text(bt_local_name_widget,bt_speaker_info.name);
                        }
                    }
                }else{
                    lv_obj_del(kb);
                    kb = NULL;
                    lv_obj_del(contain);
                    /* strlen too long */
                    create_message_box(api_rsc_string_get(STR_INPUT_NEW_BTNAME));
                }
            }
        }else if(key==LV_KEY_UP){
            lv_btnmatrix_t *btnm = &((lv_keyboard_t*)obj)->btnm;
            if(btnm->button_areas[sel_id].y1 == btnm->button_areas[0].y1){
                lv_group_focus_obj(lv_obj_get_child(contain, 1));
            }
        }
        if(obj)
            sel_id = ((lv_keyboard_t*)obj)->btnm.btn_id_sel;
    }
}

static void keypad_textarea_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_textarea_t * ta = (lv_textarea_t *)obj;
    if(code == LV_EVENT_KEY){
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_UP) {

        }else if(key == LV_KEY_DOWN){
            lv_group_focus_obj(lv_obj_get_child(obj->parent->parent, 1));
        }else if(key == LV_KEY_ENTER){
            if(kb && lv_obj_is_valid(kb)){
                return;
            }else{
                win_keyboard_widget_create(setup_slave_root,kb_textarea,keypad_widget_event_handle); 
                lv_obj_align(contain, LV_ALIGN_TOP_MID, 0,LV_PCT(22));
            }
        }
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        if(dsc && lv_group_get_focused(lv_group_get_default()) == obj){
            dsc->rect_dsc->bg_opa = LV_OPA_10;
            dsc->rect_dsc->outline_width = 0;
        }
    }else if(code == LV_EVENT_DEFOCUSED){
        lv_obj_add_state(obj,LV_STATE_FOCUSED);
    }
        
}

static void new_speakername_btns_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_btnmatrix_t *btnms = (lv_btnmatrix_t*)obj;

    if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t* dsc = lv_event_get_draw_part_dsc(e);
        dsc->rect_dsc->border_side = LV_BORDER_SIDE_TOP;
        dsc->rect_dsc->border_width = 1;
        dsc->rect_dsc->outline_width = 0;
    }else if(code == LV_EVENT_KEY){
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_RIGHT || key == LV_KEY_LEFT){
            lv_btnmatrix_set_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
        }else if(key == LV_KEY_ENTER){
             if(btnms->btn_id_sel == 0 || btnms->btn_id_sel == 1){    
                if(btnms->btn_id_sel == 1){
                    char* bt_name=lv_textarea_get_text(kb_textarea);
                    if (strlen(bt_name) > 0 && strlen(bt_name) < BTSPEAKER_NAME_MAX_SIZE) {
                        if (strcmp(bt_name, bt_speaker_info.name)) {
                            memset(bt_speaker_info.name, 0 , BTSPEAKER_NAME_MAX_SIZE);
                            strcpy(bt_speaker_info.name , bt_name);
                            if(!bluetooth_ioctl(BLUETOOTH_SET_LOCAL_NAME, bt_speaker_info.name)){
                                if(kb){
                                    lv_obj_del(kb);
                                    kb = NULL;
                                }
                                lv_obj_del(contain);
                                contain = NULL;
                                lv_label_set_text(bt_local_name_widget,bt_speaker_info.name);
                            }
                        }
                    }else{
                        if(kb){
                            lv_obj_del(kb);
                            kb = NULL;
                        }
                        lv_obj_del(contain);
                        contain = NULL;
                        /* strlen too long or too short*/
                        create_message_box(api_rsc_string_get(STR_INPUT_NEW_BTNAME));
                    }
                }else if(btnms->btn_id_sel == 0){
                    lv_obj_del(contain);
                    contain = NULL;
                    if(kb && lv_obj_is_valid(kb)){
                        lv_obj_del(kb);
                        kb = NULL;
                    }
                    lv_group_focus_obj(bt_slave_panel_widget);
                }
             }
        }else if(key == LV_KEY_UP){
            lv_btnmatrix_clear_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
            lv_group_focus_prev(lv_group_get_default());
        }else if(key == LV_KEY_DOWN){
            if(kb &&lv_obj_is_valid(kb)){
                lv_btnmatrix_clear_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
                lv_group_focus_obj(kb);
            }
        }else if(key == LV_KEY_ESC){
            lv_obj_del(contain);
            contain = NULL;
            if(kb && lv_obj_is_valid(kb)){
                lv_obj_del(kb);
                kb = NULL;
            }
            lv_group_focus_obj(bt_slave_panel_widget);
        }
    }else if(code == LV_EVENT_FOCUSED){
        btnms->btn_id_sel = 1;
        lv_btnmatrix_set_btn_ctrl(obj, 1, LV_BTNMATRIX_CTRL_CHECKED);
    }
}

static void win_keyboard_widget_create(lv_obj_t* p, lv_obj_t* ta, lv_event_cb_t event_cb)
{
    kb = lv_keyboard_create(p);
    lv_obj_set_style_text_font(kb, osd_font_get_by_langid(0, FONT_MID), 0);
    lv_obj_set_style_bg_color(kb, lv_color_make(81, 100, 117), 0);
    lv_obj_set_size(kb,LV_PCT(100),LV_PCT(46));
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(kb, event_cb, LV_EVENT_ALL, NULL);
    
    lv_keyboard_t *keyb = (lv_keyboard_t *)kb;
    lv_obj_t *btns = (lv_obj_t*)(&keyb->btnm);
    lv_obj_set_style_bg_opa(btns, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(btns, lv_palette_darken(LV_PALETTE_GREY, 1), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(btns, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(btns, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(btns, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(btns, lv_color_white(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_pad_hor(btns, lv_disp_get_hor_res(lv_disp_get_default())/10, 0);
    lv_keyboard_set_textarea(kb,ta);
    lv_group_focus_obj(kb);
    
}

static void win_btspeaker_name_prompt_create(lv_obj_t* p){
    contain = lv_obj_create(p);
    lv_obj_set_style_text_color(contain, lv_color_white(), 0);
    lv_obj_set_style_text_font(contain, osd_font_get(FONT_MID), 0);
    lv_obj_align(contain, LV_ALIGN_TOP_MID, 0,LV_PCT(22));
    lv_obj_set_size(contain,LV_PCT(40),LV_PCT(28));
    lv_obj_set_style_pad_hor(contain, 0, 0);
    lv_obj_set_style_pad_bottom(contain, 0, 0);
    lv_obj_set_style_bg_color(contain, lv_color_make(81, 100, 117), 0);
    lv_obj_set_style_border_width(contain,2,0);
    lv_obj_set_scrollbar_mode(contain, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(contain, 5, 0);
    lv_obj_set_flex_flow(contain, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(contain, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* panel_obj = lv_obj_create(contain);
    //lv_obj_set_flex_grow(panel_obj, 9);
    lv_obj_set_style_text_color(panel_obj, lv_color_white(), 0);
    lv_obj_set_style_border_width(panel_obj,0,0);
    lv_obj_set_scrollbar_mode(panel_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(panel_obj, 0, 0); 
    lv_obj_set_size(panel_obj,lv_pct(100), lv_pct(55));
    lv_obj_set_flex_flow(panel_obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_bg_opa(panel_obj, LV_OPA_0, 0);
    lv_obj_set_style_pad_gap(panel_obj, 0, 0);
    lv_obj_set_flex_align(panel_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t *obj = lv_label_create(panel_obj);
    lv_label_set_text_fmt(obj,"%s:",api_rsc_string_get(STR_BT_SPEAKER_NAME));
    lv_obj_set_style_text_font(obj, osd_font_get(FONT_MID),0);
    lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_size(obj,lv_pct(40),LV_SIZE_CONTENT);

    kb_textarea = lv_textarea_create(panel_obj);
    //lv_obj_set_flex_grow(ta, 9);
    lv_obj_set_size(kb_textarea,lv_pct(60),LV_SIZE_CONTENT);
    lv_textarea_set_one_line(kb_textarea, true);
    lv_textarea_set_max_length(kb_textarea, BTSPEAKER_NAME_MAX_SIZE);
    lv_textarea_set_placeholder_text(kb_textarea,bt_speaker_info.name);
    lv_obj_set_style_border_side(kb_textarea, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_radius(kb_textarea, 0, 0);
    lv_obj_set_style_text_color(kb_textarea, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(kb_textarea, LV_OPA_10, 0);
    lv_obj_set_style_radius(kb_textarea, 5, 0);
    lv_obj_add_event_cb(kb_textarea, keypad_textarea_event_handle, LV_EVENT_ALL, 0);
    lv_obj_add_state(kb_textarea,LV_STATE_FOCUSED);

    static const char* strs[3];
    strs[0] = api_rsc_string_get(STR_WIFI_CANCEL);
    strs[1] = api_rsc_string_get(STR_FOOT_SURE);
    strs[2] = "";
    lv_obj_t* btm_obj = lv_btnmatrix_create(contain);
    //lv_obj_set_flex_grow(btm_obj, 10);
    lv_obj_set_style_text_font(btm_obj, osd_font_get(FONT_MID),0);
    lv_obj_set_style_bg_opa(btm_obj, LV_OPA_0, 0);
    lv_obj_set_style_bg_opa(btm_obj, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_text_color(btm_obj, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_text_color(btm_obj, lv_color_black(), LV_STATE_CHECKED | LV_PART_ITEMS);
    lv_obj_set_size(btm_obj,LV_PCT(103),LV_PCT(45));
    lv_btnmatrix_set_map(btm_obj, strs);
    lv_obj_set_style_pad_all(btm_obj, 0, 0);
    lv_obj_set_style_pad_bottom(btm_obj, -6, 0);
    lv_obj_set_style_pad_gap(btm_obj, 0, 0);
    lv_obj_set_style_radius(btm_obj, 0, 0);
    lv_obj_set_style_shadow_width(btm_obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(btm_obj, 0, LV_PART_ITEMS);

    lv_btnmatrix_set_btn_ctrl_all(btm_obj, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(btm_obj, true);
    lv_obj_add_event_cb(btm_obj, new_speakername_btns_event_handle, LV_EVENT_ALL, contain);
    win_keyboard_widget_create(p, kb_textarea, keypad_widget_event_handle);
}


static void open_bt_slave_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
    if(code == LV_EVENT_KEY){
        /* input handle after into bt_rx mode  */
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC || key == LV_KEY_ENTER){
            lv_label_set_text(lv_obj_get_child(lv_obj_get_child(bt_slave_panel_widget, BT_SPEAKER_MODE_ID), 1), (char*)api_rsc_string_get(STR_OFF));
            lv_group_focus_obj(lv_obj_get_parent(target));
            lv_obj_del(target);
            if(!bluetooth_ioctl(BLUETOOTH_SET_BT_POWER_ON_TO_TX,0)){
                bt_speaker_info.mode_onoff = 0 ;
            }
            bluetooth_set_gpio_backlight(1);
        }else if(key == LV_KEY_HOME){
            /* to do here */
        }
    }
}

static void open_bt_slave_widget(void)  
{
    lv_obj_t *obj = create_list_obj1(bt_slave_panel_widget,100, 100);
    lv_obj_align(obj,LV_ALIGN_CENTER,0,0);
    lv_obj_set_style_bg_color(obj, lv_palette_darken(LV_PALETTE_GREY, 1),0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_100, 0);     
    lv_obj_add_event_cb(obj, open_bt_slave_event_handle, LV_EVENT_ALL, 0);
    lv_group_focus_obj(obj);
}

#endif 

#endif
