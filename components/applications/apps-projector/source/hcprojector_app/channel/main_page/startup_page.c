#include "screen.h"
#include "osd_com.h"
#include "factory_setting.h"
#include "com_api.h"
#include "setup.h"
// #include "mul_lang_text.h"

extern void change_language(void);
lv_obj_t* startup_scr;
lv_group_t *startup_g;

static char* text_ids[2][3] ={{"Thanks for",
                         "using this projector!",
                         "Please press OK to set language"},
                         {"��ӭ",
                         "ʹ�ñ���Ʒ",
                         "��ѡ�������õ�����"}};


lv_obj_t* startu_label0 = NULL;
lv_obj_t *startup_btncontain = NULL;
static int cur_id = 0;
static void startup_btn_event_handle(lv_event_t *e)
{
	lv_event_code_t event = lv_event_get_code(e);
	lv_obj_t *ta = lv_event_get_target(e);

    if (event == LV_EVENT_KEY) {
        printf("================%s(), line:%d. \n", __func__, __LINE__);  
		int key = lv_indev_get_key(lv_indev_get_act());
        // int index = lv_obj_get_index(ta);
		if (key == LV_KEY_ESC || key == LV_KEY_HOME)
		{
			//change_screen(main_page_scr);
		}
		else if(key == LV_KEY_ENTER){
            projector_set_some_sys_param(P_SYS_IS_STARTUP,0);
            projector_set_some_sys_param(P_OSD_LANGUAGE, cur_id);
            projector_sys_param_save();
            change_language();
            change_screen(SCREEN_CHANNEL_MAIN_PAGE);
		}
		else if( key == LV_KEY_LEFT || key == LV_KEY_UP){
            cur_id = (--cur_id + 25)%25;
            for(int i=0; i<5; i++){
                lv_label_set_text(lv_obj_get_child(lv_obj_get_child(startup_btncontain,i),0), api_rsc_string_get_by_langid((i+cur_id+23)%LANGUAGE_MAX, STR_LANG));
                lv_obj_set_style_text_font(lv_obj_get_child(lv_obj_get_child(startup_btncontain,i),0), osd_font_get_by_langid((i+cur_id+23)%LANGUAGE_MAX,FONT_MID), 0);
            }
            
            projector_set_some_sys_param(P_OSD_LANGUAGE, cur_id);
            lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(startup_scr, 0), 2);
            set_label_text2(label, GET_LABEL_ID((int)label->user_data), GET_FONT_ID((int)label->user_data)); 

            label = lv_obj_get_child(lv_obj_get_child(startup_scr, 0), 1);
            set_label_text2(label, GET_LABEL_ID((int)label->user_data),GET_FONT_ID((int)label->user_data));

            label = lv_obj_get_child(lv_obj_get_child(startup_scr, 0), 3);
            set_label_text2(label, GET_LABEL_ID((int)label->user_data), GET_FONT_ID((int)label->user_data));
		}
		else if( key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
		    cur_id = (++cur_id)%25 ;
            for(int i=0; i<5; i++){
                lv_label_set_text(lv_obj_get_child(lv_obj_get_child(startup_btncontain,i),0), api_rsc_string_get_by_langid((i+cur_id+23)%LANGUAGE_MAX, STR_LANG));
                lv_obj_set_style_text_font(lv_obj_get_child(lv_obj_get_child(startup_btncontain,i),0), osd_font_get_by_langid((i+cur_id+23)%LANGUAGE_MAX,FONT_MID), 0);
            }
        
            projector_set_some_sys_param(P_OSD_LANGUAGE, cur_id);
            lv_obj_t *label = lv_obj_get_child(lv_obj_get_child(startup_scr, 0), 2);
            set_label_text2(label, GET_LABEL_ID((int)label->user_data), GET_FONT_ID((int)label->user_data)); 

            label = lv_obj_get_child(lv_obj_get_child(startup_scr, 0), 1);
            set_label_text2(label, GET_LABEL_ID((int)label->user_data),GET_FONT_ID((int)label->user_data));

            label = lv_obj_get_child(lv_obj_get_child(startup_scr, 0), 3);
            set_label_text2(label, GET_LABEL_ID((int)label->user_data), GET_FONT_ID((int)label->user_data));
		}
	}
    // else if(event == LV_EVENT_DRAW_PART_BEGIN) {
    //     // lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
    //     // // if (lv_btnmatrix_get_selected_btn(target) == dsc->id){
    //     // //     dsc->rect_dsc->radius = 0;
    //     // //     dsc->rect_dsc->outline_width = 0;
    //     // // }
    //     // // if(dsc->id == 0){//第一个为创建时默认的字体
    //     // //     return;
    //     // // }
	// 	// dsc->label_dsc->font = osd_font_get_by_langid(dsc->id, FONT_NORMAL);
    // }
}

LV_IMG_DECLARE(sw_startup_up);
LV_IMG_DECLARE(sw_startup_down);
static void startup_scr_open(void)
{
	startup_g = lv_group_create();
	lv_group_t* g = lv_group_get_default();
	key_set_group(startup_g);

    lv_obj_t *startup_left = lv_obj_create(startup_scr);
    lv_obj_clear_flag(startup_left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(startup_left,LV_PCT(50),LV_PCT(100));
    lv_obj_set_style_bg_opa(startup_left, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(startup_left, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(startup_left, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t *line = lv_obj_create(startup_left);
        lv_obj_set_style_bg_color(line, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(line, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(line, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_size(line,240,5);
        lv_obj_center(line);

        lv_obj_t* startu_label1=lv_label_create(startup_left);
        lv_obj_set_size(startu_label1,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
        //lv_obj_set_style_text_font(startu_label1, &lv_font_montserrat_36, 0);//,osd_font_get_by_langid(0,FONT_LARGE)
        //lv_label_set_text(startu_label1, text_ids[0][1]);
        // set_label_text2(startu_label1, STR_WIFI_TITLE, FONT_LARGE);
        set_label_text2(startu_label1, STR_CHOOSE_LANG2, FONT_LARGE);


        lv_obj_align_to(startu_label1, line, LV_ALIGN_OUT_TOP_LEFT, 0, -10);
        lv_obj_set_style_text_color(startu_label1, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);


        startu_label0=lv_label_create(startup_left);
        lv_obj_set_size(startu_label0,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
        // lv_obj_set_style_text_font(startu_label0, &lv_font_montserrat_36, 0);//,osd_font_get_by_langid(0,FONT_LARGE)
        // lv_label_set_text(startu_label0, text_ids[0][0]);
        //lv_obj_set_style_text_align(startu_label0, LV_TEXT_ALIGN_CENTER, 0);
        // set_label_text2(startu_label0, STR_WIFI_TITLE, FONT_LARGE);
        set_label_text2(startu_label0, STR_CHOOSE_LANG1, FONT_LARGE);
        lv_obj_align_to(startu_label0, startu_label1, LV_ALIGN_OUT_TOP_LEFT, 0, 0);
        lv_obj_set_style_text_color(startu_label0, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);


        lv_obj_t* startu_label2=lv_label_create(startup_left);
        lv_obj_set_size(startu_label2,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
        //lv_obj_set_style_text_font(startu_label2, &lv_font_montserrat_26, 0);//,osd_font_get_by_langid(0,FONT_NORMAL)
        //lv_label_set_text(startu_label2, text_ids[0][2]);
        // set_label_text2(startu_label2, STR_WIFI_TITLE, FONT_NORMAL);
        set_label_text2(startu_label2, STR_CHOOSE_LANG3, FONT_NORMAL);
        lv_obj_align_to(startu_label2, line, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
        lv_obj_set_style_text_color(startu_label2, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);


    lv_obj_t *startup_right = lv_obj_create(startup_scr);
    lv_obj_clear_flag(startup_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(startup_right,LV_PCT(50),LV_PCT(100));
    lv_obj_align(startup_right, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(startup_right, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(startup_right, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(startup_right, LV_OBJ_FLAG_SCROLLABLE);

        startup_btncontain = lv_obj_create(startup_right);
        lv_obj_set_size(startup_btncontain,LV_PCT(90),LV_PCT(80));
        lv_obj_set_style_bg_opa(startup_btncontain, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(startup_btncontain, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_flex_flow(startup_btncontain,LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(startup_btncontain, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(startup_btncontain, 50, 0);
        lv_obj_align(startup_btncontain, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_scrollbar_mode(startup_btncontain, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t *startup_up = lv_obj_create(startup_right);
        lv_obj_set_size(startup_up,200,LV_PCT(10));
        lv_obj_align_to(startup_up, startup_btncontain, LV_ALIGN_OUT_TOP_MID, 0, 0);
        lv_obj_set_style_bg_opa(startup_up, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(startup_up, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(startup_up, &sw_startup_up, 0);

        lv_obj_t *startup_down = lv_obj_create(startup_right);
        lv_obj_set_size(startup_down,200,LV_PCT(10));
        lv_obj_align_to(startup_down, startup_btncontain, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_opa(startup_down, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(startup_down, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_img_src(startup_down, &sw_startup_down, 0);

            lv_obj_t *startup_btn = NULL;
            lv_obj_t *startup_lab = NULL;
            for(int i=0; i<5; i++){
                startup_btn = lv_btn_create(startup_btncontain);
                lv_obj_set_style_bg_color(startup_btn, lv_color_hex(0xd93a1e), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_opa(startup_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(startup_btn, lv_color_hex(0xd92152), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
                lv_obj_set_style_bg_opa(startup_btn, 255, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
                lv_obj_set_style_border_color(startup_btn, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_opa(startup_btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(startup_btn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
                if(i==2)
                    lv_obj_set_size(startup_btn,LV_PCT(80),LV_PCT(15));
                else
                lv_obj_set_size(startup_btn,LV_PCT(50),LV_PCT(12));
                lv_obj_set_style_shadow_width(startup_btn,0,0);
                //lv_obj_add_event_cb(startup_btn, startup_btn_event_handle, LV_EVENT_ALL, 0);
                    startup_lab= lv_label_create(startup_btn);
                    lv_obj_set_size(startup_lab,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
                    lv_label_set_text(startup_lab, api_rsc_string_get_by_langid((i+cur_id+23)%LANGUAGE_MAX, FONT_MID));
                    //lv_label_set_text_fmt(startup_lab,"%d", (i+cur_id+23)%25);//lv_label_set_text_fmt(startup_lab,"%d", i);
                    lv_obj_set_style_bg_color(startup_lab, lv_color_hex(0xffff00), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(startup_lab, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_align(startup_lab, LV_TEXT_ALIGN_CENTER, 0);
                    lv_obj_set_style_text_font(startup_lab, osd_font_get_by_langid((i+cur_id+23)%LANGUAGE_MAX,FONT_MID), 0);//,osd_font_get_by_langid(0,FONT_LARGE)
                    lv_obj_center(startup_lab);
            }
    lv_group_focus_obj(lv_obj_get_child(startup_btncontain, 2));
    for(int i=0; i<5; i++){
        lv_label_set_text(lv_obj_get_child(lv_obj_get_child(startup_btncontain,i),0), api_rsc_string_get_by_langid((i+cur_id+23)%LANGUAGE_MAX, STR_LANG));
        lv_obj_set_style_text_font(lv_obj_get_child(lv_obj_get_child(startup_btncontain,i),0), osd_font_get_by_langid((i+cur_id+23)%LANGUAGE_MAX,FONT_MID), 0);
        lv_obj_add_event_cb(lv_obj_get_child(startup_btncontain, i), startup_btn_event_handle, LV_EVENT_ALL, 0);
    }
    //lv_group_set_default(g);

}

static void startup_scr_close()
{
    lv_group_remove_all_objs(startup_g);
    lv_group_del(startup_g);
    lv_obj_clean(startup_scr);

}

static void startup_event_handle(lv_event_t *e)
{
	lv_event_code_t event = lv_event_get_code(e);
	lv_obj_t *ta = lv_event_get_target(e);
	if (event == LV_EVENT_SCREEN_LOADED) {
		startup_scr_open();
	}
	if (event == LV_EVENT_SCREEN_UNLOADED) {
		startup_scr_close();
	}
}

void startup_screen_init(void)
{
    startup_scr = lv_obj_create(NULL);
    lv_obj_clear_flag(startup_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(startup_scr, startup_event_handle, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(startup_scr, lv_color_hex(0x0000FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(startup_scr, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}
