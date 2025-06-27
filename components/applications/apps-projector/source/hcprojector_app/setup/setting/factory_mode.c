#include "lvgl/lvgl.h"
#include "factory_setting.h"
#include "hcstring_id.h"
#include "setup.h"
#include "com_api.h"
#include "../../channel/cvbs_in/cvbs_rx.h"
#include "mul_lang_text.h"

typedef void (*factoryitemfun)();

static lv_group_t* factorymenu_g;

extern lv_obj_t* create_item(lv_obj_t* page, choose_item * chooseItem);
static void factorymenu_items_event(lv_event_t* e, factoryitemfun fun, int item);
static void cvbs_training_event(lv_event_t* e);
static void factorymenu_close(lv_obj_t* obj);
static void factoryitems_obj_event_handle(lv_event_t* e);
void factorymenu_open(lv_obj_t* e);
void factorymenu_init();

void factorymenu_init()
{
	factorymenu_g = lv_group_create();
}

void factorymenu_open(lv_obj_t* btn)
{
	key_set_group(factorymenu_g);
	lv_obj_t* factorymenu = create_new_widget(60, 50);
	create_widget_head(factorymenu, STR_VERSION_INFO, 15);

	choose_item factoryitems[] = {
		#ifdef CVBSIN_TRAINING_SUPPORT
		{.name = STR_CVBS_TRAINING, .value.v1=BLANK_SPACE_STR, .is_number = false, .is_disabled = false, .event_func = cvbs_training_event},
		#endif
	};

	lv_obj_t* factoryitems_obj = lv_obj_create(factorymenu);
	lv_obj_set_size(factoryitems_obj, LV_PCT(100), lv_pct(85));
	lv_obj_set_flex_flow(factoryitems_obj, LV_FLEX_FLOW_COLUMN_WRAP);
	lv_obj_set_style_bg_color(factoryitems_obj, lv_color_make(101,101,177), 0);	
	lv_obj_t* item;
	int size = sizeof(factoryitems)/sizeof(factoryitems[0]);

	for(int i=0; i < size; i++){
        	item = create_item(factoryitems_obj, factoryitems+i);
        	lv_obj_set_style_text_font(lv_obj_get_child(item, 0), osd_font_get(FONT_MID), 0);
        	lv_obj_set_size(item, lv_pct(100), lv_pct(20));
   	}		
}

static void factorymenu_close(lv_obj_t* obj)
{
	key_set_group(setup_g);
	lv_obj_del(obj->parent->parent);
	turn_to_setup_root();
}

static void factorymenu_items_event(lv_event_t* e, factoryitemfun fun, int item)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_user_data(e);

    if(code == LV_EVENT_KEY){
    	uint16_t key = lv_indev_get_key(lv_indev_get_act());
    	if(key == LV_KEY_DOWN){
    		lv_group_focus_next(factorymenu_g);
    	}else if(key == LV_KEY_UP){
    		lv_group_focus_prev(factorymenu_g);
    	}else if(key == LV_KEY_ENTER){
    		if(fun){
    			fun();
    		}
    	}else if(key == LV_KEY_ESC){
    		factorymenu_close(btn);
    	}
    }
}

static void factoryitems_obj_event_handle(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    lv_obj_t* btn = (lv_obj_t*)lv_event_get_user_data(e);
    if(code == LV_EVENT_KEY){
    	uint16_t key = lv_indev_get_key(lv_indev_get_act());
	if(key == LV_KEY_ESC){
    		factorymenu_close(btn);
    	}
    }
}

#ifdef CVBSIN_TRAINING_SUPPORT
static void cvbs_training_event(lv_event_t* e)
{
	factorymenu_items_event(e, cvbs_rx_training, STR_CVBS_TRAINING);
}
#endif
