#include "screen.h"
#include "setup.h"
#include <hcuapi/snd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
#include "factory_setting.h"
#include "mul_lang_text.h"
#include "com_api.h"
#include "osd_com.h"

#define SOUND_MODE_STANDARD 0
#define SOUND_MODE_MUSIC 1
#define SOUND_MODE_MOVIE 2
#define SOUND_MODE_SPORT 3
#define SOUND_MODE_USER 4
#define EQ_GAIN_STEP 4
#define EQ_GAIN_MAX_V 120
#define EQ_GAIN_MIN_V (-120)


static struct snd_eq_band_setting eq_setting[EQ_BAND_LEN] = {
                    {0, 32, 14, 0},{1,64,14,0},{2,128, 14, 0},{3,250, 14, 0},
                    {4,500, 14, 0},{5,1000, 14, 0},{6,2000, 14, 0},
                    {7,4000, 14, 0},{8,8000, 14, 0},{9,16000, 14, 0}
                };
static int sound_mode_pre_text[2];
static lv_obj_t* eq_mode, *eq_table;

int sound_mode_vec[] = {STR_STANDARD, LINE_BREAK_STR, STR_MUSIC, LINE_BREAK_STR, STR_MOVIE, LINE_BREAK_STR, STR_SPORTS, LINE_BREAK_STR,
STR_USER, LINE_BREAK_STR, BLANK_SPACE_STR, BTNS_VEC_END};

int sound_output_vec[] = {STR_SOUND_SPEAKER, LINE_BREAK_STR, STR_SOUND_OUTPUT_SPDFI, LINE_BREAK_STR, BLANK_SPACE_STR, BTNS_VEC_END};
int sound_digital_vec[] = {STR_SOUND_RAW, LINE_BREAK_STR, STR_SOUND_PCM, LINE_BREAK_STR, BLANK_SPACE_STR, BTNS_VEC_END};
int eq_mode_vec[] = {STR_EQ_NORMAL,LINE_BREAK_STR, STR_EQ_ROCK,LINE_BREAK_STR, STR_EQ_POP,LINE_BREAK_STR, STR_EQ_JAZZ,LINE_BREAK_STR, 
STR_EQ_CLASSIC,LINE_BREAK_STR, STR_EQ_VOCAL,LINE_BREAK_STR, STR_EQ_CUSTOME,BTNS_VEC_END};
extern mode_items sound_mode_items[2];
extern lv_timer_t *timer_setting;


static void sound_mode_btnmatrix_event(lv_event_t* e);
static void sound_mode_output_btnmatrix_event(lv_event_t* e);
static void sound_mode_spdif_btnmatrix_event(lv_event_t* e);
static int change_soundMode_event_(int k);
static void eq_table_reset();
static void create_eq_table(lv_obj_t* parent);
static void eq_table_save();
static void eq_modeitch_event_handle(lv_event_t* e);
static void eq_table_set_by_id(int id, int value);
static int change_sound_mode_output(int mode);
static int change_sound_spdif_output(int mode);
int set_twotone(int mode, int treble, int bass);
int set_balance(int v);
void sound_mode_widget(lv_obj_t* btn);

extern lv_obj_t *new_widget_(lv_obj_t*, int title, int*,uint32_t index, int len, int w, int h);
extern void btnmatrix_event(lv_event_t* e, btn_matrix_func f);
static lv_obj_t *m_sound_output_obj = NULL;
extern void label_set_text_color(lv_obj_t* label,const char* text, char* color);
void sound_mode_widget(lv_obj_t* btn)
{
    lv_obj_t * obj = new_widget_(btn, STR_SOUND_MODE, sound_mode_vec,projector_get_some_sys_param(P_SOUND_MODE), 
        HC_ARRAY_SIZE(sound_mode_vec),0,0);
    if(projector_get_some_sys_param(P_SOUND_MODE) ==  SND_TWOTONE_MODE_USER){
        for(int i=0; i<2; i++){
            sound_mode_pre_text[i] = projector_get_some_sys_param(sound_mode_items[i].index);
        }
    }
    lv_obj_add_event_cb(lv_obj_get_child(obj, 1), sound_mode_btnmatrix_event, LV_EVENT_ALL, btn);
}

void sound_output_mode_widget(lv_obj_t* btn)
{
    lv_obj_t * obj = new_widget_(btn, STR_SOUND_OUTPUT_MODE, sound_output_vec,projector_get_some_sys_param(P_SOUND_OUT_MODE), 
        HC_ARRAY_SIZE(sound_output_vec),0,0);
    lv_obj_add_event_cb(lv_obj_get_child(obj, 1), sound_mode_output_btnmatrix_event, LV_EVENT_ALL, btn);
}

void sound_spdif_mode_widget(lv_obj_t* btn)
{
    lv_obj_t * obj = new_widget_(btn, STR_SOUND_DIGITAL_OUTPUT, sound_digital_vec,projector_get_some_sys_param(P_SOUND_SPDIF_MODE), 
        HC_ARRAY_SIZE(sound_digital_vec),0,0);
    lv_obj_add_event_cb(lv_obj_get_child(obj, 1), sound_mode_spdif_btnmatrix_event, LV_EVENT_ALL, btn);
}

static void sound_mode_map_get(int key, int value[2]){
    switch (key) {
        case SOUND_MODE_STANDARD:
            value[0] = 0;
            value[1] = 0;
            break;
        case SOUND_MODE_MUSIC:
            value[0] = 5;
            value[1] = 5;
            break;
        case SOUND_MODE_MOVIE:
            value[0] = 6;
            value[1] = 5;
            break;
        case SOUND_MODE_SPORT:
            value[0] = -3;
            value[1] = -3;
            break;
        default:
            value[0] = projector_get_some_sys_param(P_BASS);
            value[1] = projector_get_some_sys_param(P_TREBLE);
            break;
    }
}

static void sound_mode_btnmatrix_event(lv_event_t* e){
    btnmatrix_event(e, change_soundMode_event_);
}

static void sound_mode_output_btnmatrix_event(lv_event_t* e){
    btnmatrix_event(e, change_sound_mode_output);
}

static void sound_mode_spdif_btnmatrix_event(lv_event_t* e){
    btnmatrix_event(e, change_sound_spdif_output);
}

static int change_sound_mode_output(int mode)
{
    printf("%s(), set mode >> %d \n", __func__, mode);
    projector_set_some_sys_param(P_SOUND_OUT_MODE, mode);

    if (SOUND_OUTPUT_SPEAKER == (sound_output_mode_e)mode){
        if (m_sound_output_obj && (!lv_obj_has_state(m_sound_output_obj, LV_STATE_DISABLED))){
            lv_obj_add_state(m_sound_output_obj, LV_STATE_DISABLED);  
        }
    } else if (SOUND_OUTPUT_SPDIF == (sound_output_mode_e)mode) {
        if (m_sound_output_obj && (lv_obj_has_state(m_sound_output_obj, LV_STATE_DISABLED))){
            lv_obj_clear_state(m_sound_output_obj, LV_STATE_DISABLED);    
        }
    }

    return 0;
}

static int change_sound_spdif_output(int mode)
{
    projector_set_some_sys_param(P_SOUND_SPDIF_MODE, mode);
    return 0;
}

void sound_set_outupt_obj(void *obj)
{
    m_sound_output_obj = obj;
}

static int change_soundMode_event_(int k)
{
    lv_obj_t* obj;

    int values[2] = {0};
    sound_mode_map_get(k, values);

    switch (k){
    case SND_TWOTONE_MODE_STANDARD:
    case SND_TWOTONE_MODE_MUSIC:
    case SND_TWOTONE_MODE_MOVIE:
    case SND_TWOTONE_MODE_SPORT:
        set_twotone(k, values[0], values[1]);
         for(int i=0; i< 2; i++){
            obj = sound_mode_items[i].obj;
            if (!lv_obj_has_state(obj, LV_STATE_DISABLED)){
                lv_obj_add_state(obj, LV_STATE_DISABLED);
            }
            lv_obj_set_style_text_color(obj, lv_color_make(125,125,125), 0);
            lv_label_set_text_fmt(lv_obj_get_child(obj, 1), "%d", values[i]);
        }
        break;
    case SND_TWOTONE_MODE_USER:
        set_twotone(k, sound_mode_pre_text[0], sound_mode_pre_text[1]);
        for (int i = 0; i < 2; i++) {
            obj = sound_mode_items[i].obj;
            
            if(lv_obj_has_state(obj, LV_STATE_DISABLED)){
                lv_obj_clear_state(obj, LV_STATE_DISABLED);
            }
            lv_obj_set_style_text_color(obj, lv_color_white(), 0);
            lv_label_set_text_fmt(lv_obj_get_child(obj, 1),"%d",sound_mode_pre_text[i]);
        }
        break;   
    
    default:
        break;
    }
   
 return 0;
}

int set_twotone(int mode, int treble, int bass){
    
	int snd_fd = -1;

	struct snd_twotone tt = {0};
	snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("twotone open snd_fd %d failed\n", snd_fd);
		return -1;
	}
    tt.tt_mode = mode;
    tt.onoff = 1;

    if(bass >= -10 && bass <= 10){
        tt.bass_index = bass;
        projector_set_some_sys_param(P_BASS, bass);
    }
    if(treble >= -10 && treble <= 10){
        tt.treble_index = treble;
        projector_set_some_sys_param(P_TREBLE, treble);
    }
    projector_set_some_sys_param(P_SOUND_MODE, mode);

	ioctl(snd_fd, SND_IOCTL_SET_TWOTONE, &tt);
	close(snd_fd);
	return 0;
}

int set_balance(int v){
    int snd_fd = -1;

    struct snd_lr_balance lr = {0};

    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
	if (snd_fd < 0) {
		printf ("lr_balance open snd_fd %d failed\n", snd_fd);
		return -1;
	}

    lr.lr_balance_index = v;
    lr.onoff = 1;

    projector_set_some_sys_param(P_BALANCE, v);

    ioctl(snd_fd, SND_IOCTL_SET_LR_BALANCE, &lr);
	close(snd_fd);
	return 0;
}

int mp_set_eq()
{
    int snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0) {
        printf ("lr_balance open snd_fd %d failed\n", snd_fd);
        return -1;
    }

    for(int i=0; i < EQ_BAND_LEN; i++){
        ioctl(snd_fd, SND_IOCTL_SET_EQ_BAND, &eq_setting[i]);
    }
    close(snd_fd);
    return 0;
}

int mp_set_eq_enable(int en)
{
    int snd_fd = -1;
    opterr = 0;
    optind = 0;
    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0) {
        printf ("eq_enable open snd_fd %d failed\n", snd_fd);
        return -1;
    }
    ioctl(snd_fd, SND_IOCTL_SET_EQ_ONOFF, !!en);
    close(snd_fd);
    return 0;
}

void mp_eq_init()
{
    eq_table_reset();
    mp_set_eq();
    if(projector_get_some_sys_param(P_SOUND_EQ)){
        mp_set_eq_enable(true);
    }else{
        mp_set_eq_enable(false);
    }
}

static void eq_table_set(int* nums)
{
    for(int i = 0; i < EQ_BAND_LEN; i++){
        eq_setting[i].gain = nums[i];
    }    
}

static void eq_table_reset()
{
    int nums[EQ_BAND_LEN] = {0};
    sysdata_eq_gain_get(projector_get_some_sys_param(P_EQ_MODE) ,nums);
    eq_table_set(nums);
}

static void eq_mode_set(eq_mode_e e)
{
    if(e >= EQ_MODE_NORMAL && e <= EQ_MODE_USER){
        projector_set_some_sys_param(P_EQ_MODE, e);
    }
    switch (e){
        case EQ_MODE_NORMAL:
        case EQ_MODE_ROCK:
        case EQ_MODE_POP:
        case EQ_MODE_JAZZ:
        case EQ_MODE_CLASSIC:
        case EQ_MODE_VOICE:
        case EQ_MODE_USER:
            eq_table_reset();            
            break;
        default:
            break;
    }
}

static void eq_table_set_by_id(int id, int value)
{
    if(id < 0 || id >= EQ_BAND_LEN){
        return;
    }
    eq_setting[id].gain = value;
    printf("i: %d, v: %d\n", id, eq_setting[id].gain);
}

static void eq_table_save()
{
    for(int i = 0; i < EQ_BAND_LEN; i++){
        sysdata_eq_gain_set(projector_get_some_sys_param(P_EQ_MODE), i, eq_setting[i].gain);
    }    
    projector_sys_param_save();
}

static void eq_item_event_handle(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    lv_obj_t* parent = obj->parent;
    lv_obj_t* bar = lv_obj_get_child(obj, 0);

    if(code == LV_EVENT_KEY){
        if(timer_setting){
            lv_timer_pause(timer_setting);
        }
        uint16_t key = lv_indev_get_key(lv_indev_get_act());

        if(key == LV_KEY_RIGHT){
            uint32_t idx = lv_obj_get_index(obj);
            idx = idx == lv_obj_get_child_cnt(parent) - 1 ? 0 : idx + 1;
            lv_group_focus_obj(lv_obj_get_child(parent, idx));
        } else if(key == LV_KEY_LEFT){
            uint32_t idx = lv_obj_get_index(obj);
            if(idx == 0){
                lv_group_focus_obj(eq_mode);
                eq_table_save();
            } else{
                idx =  idx - 1;
                lv_group_focus_obj(lv_obj_get_child(parent, idx));
            }

        } else if(key == LV_KEY_ENTER){
            //lv_group_focus_obj(lv_obj_get_child(obj, 1));
        } else if(key == LV_KEY_UP){
            int cur_v = lv_bar_get_value(bar);
            if(cur_v + EQ_GAIN_STEP <= EQ_GAIN_MAX_V) {
                cur_v += EQ_GAIN_STEP;
                lv_bar_set_value(bar, cur_v, LV_ANIM_OFF);
                lv_label_set_text_fmt(lv_obj_get_child(bar, 0), "%d", cur_v);
            }

            uint32_t idx = lv_obj_get_index(obj);
            eq_table_set_by_id(idx, cur_v);
            mp_set_eq();
        } else if(key == LV_KEY_DOWN){
            int cur_v = lv_bar_get_value(bar);
            if(cur_v - EQ_GAIN_STEP >= EQ_GAIN_MIN_V) {
                cur_v -= EQ_GAIN_STEP;
                lv_bar_set_value(bar, cur_v, LV_ANIM_OFF);
                lv_label_set_text_fmt(lv_obj_get_child(bar, 0), "%d", cur_v);
            }

            uint32_t idx = lv_obj_get_index(obj);
            eq_table_set_by_id(idx, cur_v);
            mp_set_eq();
        }else if(key == LV_KEY_ESC || key == LV_KEY_HOME){
            lv_obj_del(obj->parent->parent);
            slave_scr_obj_set(NULL);
            turn_to_setup_root();
        }
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
    } else if(code == LV_EVENT_DRAW_PART_BEGIN){

    }else if(code == LV_EVENT_DELETE){
        eq_table_save();
    }
}

static void eq_mode_event_handle(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    lv_obj_t* sw = lv_obj_get_child(obj, 0);
    lv_obj_t* btm = lv_obj_get_child(obj, 1);
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_user_data(e);

    static int idx = 0;
    if(code == LV_EVENT_KEY){
        if(timer_setting){
            lv_timer_pause(timer_setting);
        }
        uint16_t key = lv_indev_get_key(lv_indev_get_act());

        if(key == LV_KEY_DOWN || key == LV_KEY_UP){
            if(idx >= 0 && idx < EQ_MODE_MAX){
                lv_event_send(btm, LV_EVENT_KEY, &key);
                idx += key == LV_KEY_DOWN ? 1 : -1;
                idx = idx < 0 ? 0 : idx;
                idx = idx >= EQ_MODE_MAX ? EQ_MODE_MAX - 1 : idx;
                eq_mode_set(lv_btnmatrix_get_selected_btn(btm));
                mp_set_eq();
                
                for(int i = 0; i < lv_obj_get_child_cnt(eq_table); i++){
                    lv_obj_t* child = lv_obj_get_child(lv_obj_get_child(eq_table, i), 0);
                    lv_bar_set_value(child, eq_setting[i].gain, LV_ANIM_OFF);
                    lv_label_set_text_fmt(lv_obj_get_child(child, 0), "%d", eq_setting[i].gain);
                }                
            }
        } else if(key == LV_KEY_RIGHT){
            if(lv_obj_has_state(sw, LV_STATE_CHECKED)){
                if(idx >= EQ_MODE_MAX){
                    idx -= EQ_MODE_MAX;
                    lv_btnmatrix_set_btn_ctrl(btm, idx, LV_BTNMATRIX_CTRL_CHECKED);
                    lv_btnmatrix_set_selected_btn(btm, idx);

                    lv_obj_set_style_outline_width(sw, 0, 0);                   
                }else{
                    lv_group_focus_obj(lv_obj_get_child(eq_table, 0));
                }
                
            }

        }else if(key == LV_KEY_LEFT){
           if(idx >= 0 && idx < EQ_MODE_MAX){
                lv_btnmatrix_clear_btn_ctrl(btm, idx, LV_BTNMATRIX_CTRL_CHECKED);
                lv_btnmatrix_set_selected_btn(btm, LV_BTNMATRIX_BTN_NONE);
                idx += EQ_MODE_MAX;
                lv_obj_set_style_outline_width(sw, 2, 0);
           }
        }
        else if(key == LV_KEY_ESC){
            eq_table = NULL;
            eq_mode = NULL;
            lv_obj_del(obj->parent);

            slave_scr_obj_set(NULL);
            turn_to_setup_root();
        }else if(key == LV_KEY_ENTER){
            if(idx >= EQ_MODE_MAX){
                if(lv_obj_has_state(sw, LV_STATE_CHECKED)){
                    lv_obj_clear_state(sw, LV_STATE_CHECKED);
                    lv_obj_set_style_outline_color(sw, lv_palette_main(LV_PALETTE_BLUE), 0);
                    create_message_box(api_rsc_string_get(STR_EQ_CLOSE_MSG));
                    projector_set_some_sys_param(P_SOUND_EQ, 0);
                    mp_set_eq_enable(false);
                    lv_obj_clear_state(lv_obj_get_child(btn->parent, 0), LV_STATE_DISABLED);
                    lv_obj_set_style_text_color(lv_obj_get_child(btn->parent, 0), lv_color_white(), 0);
                    if(projector_get_some_sys_param(P_SOUND_MODE) == SND_TWOTONE_MODE_USER){
                        lv_obj_clear_state(lv_obj_get_child(btn->parent, 1), LV_STATE_DISABLED);
                        lv_obj_clear_state(lv_obj_get_child(btn->parent, 2), LV_STATE_DISABLED);
                        lv_obj_set_style_text_color(lv_obj_get_child(btn->parent, 1), lv_color_white(), 0);
                        lv_obj_set_style_text_color(lv_obj_get_child(btn->parent, 2), lv_color_white(), 0);                    
                    }
                    int nums[2] = {0};
                    sound_mode_map_get(projector_get_some_sys_param(P_SOUND_MODE), nums);
                    set_twotone(projector_get_some_sys_param(P_SOUND_MODE), nums[0], nums[1]);                     
                } else{
                    lv_obj_add_state(sw, LV_STATE_CHECKED);
                    lv_obj_set_style_outline_color(sw, lv_color_white(), 0);
                    create_message_box(api_rsc_string_get(STR_EQ_OPEN_MSG));
                    mp_set_eq_enable(true);
                    projector_set_some_sys_param(P_SOUND_EQ, 1);

                    lv_obj_add_state(lv_obj_get_child(btn->parent, 0), LV_STATE_DISABLED);
                    lv_obj_add_state(lv_obj_get_child(btn->parent, 1), LV_STATE_DISABLED);
                    lv_obj_add_state(lv_obj_get_child(btn->parent, 2), LV_STATE_DISABLED);
                    lv_obj_set_style_text_color(lv_obj_get_child(btn->parent, 0), lv_color_make(125,125,125), 0);
                    lv_obj_set_style_text_color(lv_obj_get_child(btn->parent, 1), lv_color_make(125,125,125), 0);
                    lv_obj_set_style_text_color(lv_obj_get_child(btn->parent, 2), lv_color_make(125,125,125), 0);

                    int mode = projector_get_some_sys_param(P_SOUND_MODE);
                    int nums[2] = {0};
                    sound_mode_map_get(projector_get_some_sys_param(P_SOUND_MODE), nums);                
                    set_twotone(SOUND_MODE_STANDARD,0,0);
                    projector_set_some_sys_param(P_SOUND_MODE, mode);
                    projector_set_some_sys_param(P_BASS, nums[1]);
                    projector_set_some_sys_param(P_TREBLE, nums[0]);                    
                }
            }else{
                eq_table = NULL;
                eq_mode = NULL;
                lv_obj_del(obj->parent);

                slave_scr_obj_set(NULL);
                turn_to_setup_root();
            }
        }
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
    }else if(code == LV_EVENT_FOCUSED){
        if(projector_get_some_sys_param(P_SOUND_EQ)){
            idx = projector_get_some_sys_param(P_EQ_MODE);
        }else{
            idx = projector_get_some_sys_param(P_EQ_MODE) + EQ_MODE_MAX;
        }
    }else if(code == LV_EVENT_DEFOCUSED){

    }else if(code == LV_EVENT_DELETE){
        lv_event_send(btn, LV_EVENT_REFRESH, NULL);
    }
}

static lv_obj_t* create_eq_item(lv_obj_t* parent, struct snd_eq_band_setting* eq0)
{
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_set_height(obj, lv_pct(100));
    lv_obj_set_style_pad_hor(obj, 4, 0);
    lv_obj_set_style_pad_ver(obj, 5, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_border_color(obj, lv_color_white(), LV_STATE_FOCUSED);
    lv_obj_add_event_cb(obj, eq_item_event_handle, LV_EVENT_ALL, 0);
    lv_group_add_obj(lv_group_get_default(), obj);
    lv_obj_set_style_bg_color(obj, lv_color_make(64,64,128), 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 2, LV_STATE_FOCUSED);

    lv_obj_set_style_text_color(obj , lv_color_white(), 0);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* child = lv_bar_create(obj);
    lv_bar_set_range(child, -120, 120);
    lv_bar_set_value(child, eq0->gain, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(child, lv_palette_lighten(LV_PALETTE_GREY, 1), 0);
    lv_obj_set_style_bg_grad_dir(child, LV_GRAD_DIR_VER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(child, lv_color_make(0, 255, 0), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(child, lv_color_make(0, 128, 255), LV_PART_INDICATOR);
    lv_obj_set_flex_grow(child, 15);
    lv_obj_set_width(child, lv_pct(80));

    lv_obj_t *bar_v = lv_label_create(child);
    lv_obj_center(bar_v);
    lv_obj_set_style_text_color(bar_v, lv_color_white(), 0);
    lv_label_set_text_fmt(bar_v, "%d", eq0->gain);

    child = lv_label_create(obj);
    lv_obj_set_width(child, lv_pct(100));
    lv_obj_set_flex_grow(child, 1);
    lv_obj_set_style_border_color(child, lv_color_make(0,0,128), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(child, 1, LV_STATE_FOCUSED);
    lv_obj_set_style_text_align(child, LV_TEXT_ALIGN_CENTER, 0);
    if(eq0->cutoff < 1000){
        lv_label_set_text_fmt(child, "%d\nHz", eq0->cutoff);
    } else{
        lv_label_set_text_fmt(child, "%dk\nHz", eq0->cutoff/1000);
    }
    return obj;
}

static void create_eq_table(lv_obj_t* parent)
{
    lv_obj_t *obj = lv_obj_create(parent);
    eq_table = obj;
    lv_obj_center(obj);
    lv_obj_set_size(obj, lv_pct(80), lv_pct(100));
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(obj, lv_color_make(32,32,64), 0);
    lv_obj_set_style_pad_gap(obj, 17, 0);
    lv_obj_set_style_border_width(obj, 0, 0);

    eq_table_reset();

    lv_obj_t *child = NULL;
    for(int i=0; i<EQ_BAND_LEN; i++){
        child = create_eq_item(obj, &eq_setting[i]);
        lv_obj_set_flex_grow(child, 1);
    }
}

static void eq_mode_table_event_handle(lv_event_t* e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_btnmatrix_set_btn_ctrl(obj, lv_btnmatrix_get_selected_btn(obj), LV_BTNMATRIX_CTRL_CHECKED);
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        if(dsc->part == LV_PART_ITEMS){
            dsc->label_dsc->color = lv_color_white();
        }
    }
}

static lv_obj_t *create_eq_mode_table(lv_obj_t* parent)
{
    static const char * mode_name[EQ_MODE_MAX*2-1] = {0};
    
    for(int i = 0; i < EQ_MODE_MAX*2; i++){
        if(i%2 == 0){
            mode_name[i] = api_rsc_string_get_by_langid(projector_get_some_sys_param(P_OSD_LANGUAGE), eq_mode_vec[i/2]);
        }else{
            mode_name[i] = "\n";
        }
    }
    mode_name[EQ_MODE_MAX*2-2] = "";
    lv_obj_t *obj = lv_btnmatrix_create(parent);
    lv_obj_add_event_cb(obj, eq_mode_table_event_handle, LV_EVENT_ALL, 0);
    lv_obj_set_size(obj, lv_pct(60), lv_pct(60));
    lv_obj_align(obj, LV_ALIGN_RIGHT_MID, 0, 0);
    set_btns_lang2(obj, HC_ARRAY_SIZE(eq_mode_vec), FONT_SMALL, eq_mode_vec);
    lv_btnmatrix_set_btn_ctrl_all(obj,  LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(obj, true);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, LV_PART_MAIN );
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, LV_PART_ITEMS );
    lv_obj_set_style_text_color(obj, lv_color_white(), 0);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 5, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(obj, 0, LV_PART_ITEMS);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
    if(projector_get_some_sys_param(P_SOUND_EQ)){
        lv_btnmatrix_set_selected_btn(obj, projector_get_some_sys_param(P_EQ_MODE));
        lv_btnmatrix_set_btn_ctrl(obj, projector_get_some_sys_param(P_EQ_MODE), LV_BTNMATRIX_CTRL_CHECKED);
    }
    return obj;
}

void create_eq_widget(lv_obj_t* btn)
{
    lv_obj_t *obj = lv_obj_create(setup_slave_root);
    lv_obj_center(obj);
    lv_obj_set_size(obj, lv_pct(70), lv_pct(70));
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(obj, lv_color_make(32,32,64), 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *child = lv_obj_create(obj);
    lv_obj_set_scrollbar_mode(child, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(child, LV_OPA_0, 0);
    lv_obj_set_style_border_width(child, 0, 0);
    lv_obj_set_style_pad_all(child, 0, 0);
    lv_obj_set_style_outline_width(child, 2, 0);
    lv_obj_set_style_outline_color(child, lv_color_white(), 0);
    lv_obj_set_size(child, lv_pct(20), lv_pct(100));
    lv_group_add_obj(lv_group_get_default(), child);
    lv_obj_add_event_cb(child, eq_mode_event_handle, LV_EVENT_ALL, (void*)btn);
    eq_mode = child;

    lv_obj_t *switch_obj= lv_switch_create(child);
    lv_obj_align(switch_obj, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_outline_color(switch_obj, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_outline_width(switch_obj, 2, 0);
    lv_obj_set_style_outline_pad(switch_obj, 1, 0);
    lv_obj_set_width(switch_obj, lv_pct(40));
    if(projector_get_some_sys_param(P_SOUND_EQ)){
        lv_obj_add_state(switch_obj, LV_STATE_CHECKED);
        lv_obj_set_style_outline_color(switch_obj, lv_color_white(), 0);
        lv_obj_set_style_outline_width(switch_obj, 0, 0);
    }

    create_eq_mode_table(child);
    create_eq_table(obj);
    slave_scr_obj_set(obj);
    lv_group_focus_obj(child);
}



