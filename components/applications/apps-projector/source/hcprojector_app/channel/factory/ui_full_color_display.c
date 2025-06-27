#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "screen.h"
#include "osd_com.h"
#include "lvgl/lvgl.h"
#include "ui_factory_test.h"

#ifdef HC_FACTORY_TEST

#ifdef __linux__
#include "io.h"
#else
#include <kernel/io.h>
#endif

#define HW_BASE1 0xb8808000 //2K DE
#define HW_BASE2 0xb883a000 //4K DE

typedef struct {
	uint32_t color_value;
	char    name[8];
}full_color_t;

typedef struct{
	int8_t inited;
	int8_t resved[3];
	uint32_t dw00;
	uint32_t dw80;
}fcdtest_t;

static fcdtest_t g_fcdtest;
static full_color_t full_color_array[]={
	0xffabcdef, "OFF",
	0x0000ff, "Red",
	0xff0000, "Green",
	0x00ff00, "Blue",
	0x000000, "Black",
	0xffffff, "White",
	0xff00ff, "Yellow",
	0xffff00, "Cyan",
	0x00ffff, "Magenta",
};
#define FULL_COLOR_NUM_MAX  (sizeof(full_color_array)/sizeof(full_color_array[0]))

static void factory_color_test(uint32_t idx);
static int32_t value = 0;
static lv_timer_t* autochange_tim=NULL;
static void auto_change_time_cb(lv_timer_t * t)
{
	if (value < FULL_COLOR_NUM_MAX-1) 
		value++; 
	else 
		value = 0;
	factory_color_test(value);
}


static uint32_t m_de_base_addr = HW_BASE1;

static void hw_full_scr_color_open(void)
{
	if(g_fcdtest.inited==1)
		return;

    uint32_t reg_value = REG32_READ(HW_BASE1);

	if (reg_value == 0x14){
		m_de_base_addr = HW_BASE2;
    }else if (reg_value == 0x15){
		m_de_base_addr = HW_BASE1;
    }
	g_fcdtest.dw80= REG32_READ(m_de_base_addr+0x80);
	g_fcdtest.dw00 = REG32_READ(m_de_base_addr+0x00);
	REG32_SET_FIELD2(m_de_base_addr+0x80,21,1,1);
	REG32_SET_FIELD2(m_de_base_addr+0x80,20,1,0);
	REG32_SET_FIELD2(m_de_base_addr+0x80,16,1,0);
	REG32_SET_FIELD2(m_de_base_addr+0x00,2,1,0);
	REG32_SET_FIELD2(m_de_base_addr+0x00,2,1,1);
			printf("full color display LV_KEY_RIGHT:%s,%d\n",__func__,__LINE__);

	g_fcdtest.inited = 1;
}

static void hw_full_scr_color_close(void)
{
	if(g_fcdtest.inited != 1)
		return;

	REG32_WRITE(m_de_base_addr+0x80,g_fcdtest.dw80);
	REG32_WRITE(m_de_base_addr+0x00,g_fcdtest.dw00);

	REG32_SET_FIELD2(m_de_base_addr+0x00,2,1,0);
	REG32_SET_FIELD2(m_de_base_addr+0x00,2,1,1);
	g_fcdtest.inited = 0;
}

static void change_color(uint32_t color)
{
	if(g_fcdtest.inited == 1){
		REG32_SET_FIELD2(m_de_base_addr+0x88,0,24,color);
	}
}

static void factory_color_test(uint32_t idx)
{
			printf("full color display LV_KEY_RIGHT:%d,%d\n",idx,__LINE__);

	if(idx == 0)
		hw_full_scr_color_close();
	else
	{
			printf("full color display LV_KEY_RIGHT:%s,%d\n",__func__,__LINE__);

		hw_full_scr_color_open();
			printf("full color display LV_KEY_RIGHT:%s,%d\n",__func__,__LINE__);

		change_color(full_color_array[idx].color_value);
			printf("full color display LV_KEY_RIGHT:%d,%d\n",idx,__LINE__);

	}
	// hw_full_scr_color_close();
}

#ifdef MSG
/*Define a message ID*/
#define MSG_INC             1
#define MSG_DEC             2
//#define MSG_SET             3
#define MSG_UPDATE          4
#define MSG_UPDATE_REQUEST  5
#endif

lv_obj_t * full_color_panel = NULL;

static lv_group_t *fcd_g = NULL;
static lv_obj_t *btn_l = NULL;
static lv_obj_t *btn_r = NULL;
static void btn_event_cb(lv_event_t * e);
#ifdef MSG	
static void value_handler(void * s, lv_msg_t * m);
static void label_event_cb(lv_event_t * e);
#endif

/**
 * Show how an increment button, a decrement button, as slider can set a value
 * and a label display it.
 * The current value (i.e. the system's state) is stored only in one static variable in a function
 * and no global variables are required.
 */
void full_color_page_test(void * parent)
{
#ifdef MSG	
    lv_msg_subsribe(MSG_INC, value_handler, NULL);
    lv_msg_subsribe(MSG_DEC, value_handler, NULL);
    lv_msg_subsribe(MSG_UPDATE, value_handler, NULL);
    lv_msg_subsribe(MSG_UPDATE_REQUEST, value_handler, NULL);
#endif

    //lv_obj_t * 
    full_color_panel = lv_obj_create(parent);
    
    lv_obj_set_size(full_color_panel, 250, 40);
    lv_obj_set_align(full_color_panel, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_flex_flow(full_color_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(full_color_panel, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_color(full_color_panel, lv_color_hex(0x404040), 0);
    
    lv_obj_add_event_cb(full_color_panel, btn_event_cb, LV_EVENT_ALL, NULL);
    //lv_obj_t * btn;
    lv_obj_t * label;
	fcd_g = lv_group_create();

    /*Up button*/
    btn_l = lv_btn_create(full_color_panel);
    lv_obj_set_flex_grow(btn_l, 1);
    lv_obj_add_event_cb(btn_l, btn_event_cb, LV_EVENT_ALL, NULL);
    label = lv_label_create(btn_l);
    lv_label_set_text(label, LV_SYMBOL_LEFT);
    lv_obj_center(label);

	
    /*Current value*/
    label = lv_label_create(full_color_panel);
    lv_obj_set_flex_grow(label, 2);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label, "Off");
#ifdef MSG	    
    lv_msg_subsribe_obj(MSG_UPDATE, label, NULL);
    lv_obj_add_event_cb(label, label_event_cb, LV_EVENT_MSG_RECEIVED, NULL);
#endif
    /*Down button*/
    btn_r = lv_btn_create(full_color_panel);
    lv_obj_set_flex_grow(btn_r, 1);
    lv_obj_add_event_cb(btn_r, btn_event_cb, LV_EVENT_ALL, NULL);
    label = lv_label_create(btn_r);
    lv_label_set_text(label, LV_SYMBOL_RIGHT);
    lv_obj_center(label);
	lv_group_add_obj(fcd_g,btn_r);
	lv_group_add_obj(fcd_g,btn_l);
	lv_group_focus_obj(btn_l);
	lv_group_set_default(fcd_g);
#ifdef MSG		
    /* As there are new UI elements that don't know the system's state
     * send an UPDATE REQUEST message which will trigger an UPDATE message with the current value*/
    lv_msg_send(MSG_UPDATE_REQUEST, NULL);
#endif
	//extern void key_set_group(lv_group_t *key_group);
	key_set_group(fcd_g);
	if(!autochange_tim){
		autochange_tim=lv_timer_create(auto_change_time_cb,2000,NULL);
	}else{
		lv_timer_resume(autochange_tim);
	}
}

#ifdef MSG	
static void value_handler(void * s, lv_msg_t * m)
{
    LV_UNUSED(s);

    static int32_t value = 0;
    int32_t old_value = value;
    switch(lv_msg_get_id(m)) {
        case MSG_INC:
            if (value < FULL_COLOR_NUM_MAX-1) value++; 
			else value = 0;
            break;
        case MSG_DEC:
            if (value > 0) value--;            
			else value = FULL_COLOR_NUM_MAX-1;
            break;
        case MSG_UPDATE_REQUEST:
            lv_msg_send(MSG_UPDATE, &value);
            break;
        default:
            break;
    }

    if(value != old_value) {
        lv_msg_send(MSG_UPDATE, &value);
    }
}
#endif

static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    static int32_t value = 0;

    if(code== LV_EVENT_SCREEN_LOAD_START){
		key_set_group(fcd_g);
	}	
	else if (code == LV_EVENT_SCREEN_UNLOAD_START) {
		lv_group_del(fcd_g);
		lv_group_set_default(facty_g);
	}
	else if(code == LV_EVENT_KEY){
		hw_full_scr_color_close();
		uint8_t key = lv_indev_get_key(lv_indev_get_act());		
    	if(key == LV_KEY_LEFT){
		#ifdef MSG	
            lv_msg_send(MSG_DEC, NULL);
		#else
			if (value > 0) value--;            
			else value = FULL_COLOR_NUM_MAX-1;
			printf("full color display LV_KEY_RIGHT:%d\n",value);
			factory_color_test(value);
			printf("full color display LV_KEY_RIGHT:%d\n",value);

		#endif
		}
		else if(key == LV_KEY_RIGHT){
		printf("full color display LV_KEY_RIGHT:%d,%d\n",value,__LINE__);

		#ifdef 	MSG
			lv_msg_send(MSG_INC, NULL);
		#else
			if (value < FULL_COLOR_NUM_MAX-1) value++; 
			else value = 0;
			printf("full color display LV_KEY_RIGHT:%d,%d\n",value,__LINE__);
			factory_color_test(value);
			printf("full color display LV_KEY_RIGHT:%d,%d\n",value,__LINE__);

		#endif		
		}
		else if(key == LV_KEY_ESC){
			//printf("full color display LV_KEY_ESC\n");
			if(autochange_tim)
				lv_timer_pause(autochange_tim);			
			lv_obj_clear_flag(factory_settings_scr,LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(full_color_panel,LV_OBJ_FLAG_HIDDEN);
			change_screen(SCREEN_CHANNEL_MAIN_PAGE);//
			lv_group_set_default(facty_g);
		}
	}
}

#ifdef MSG	
static void label_event_cb(lv_event_t * e)
{
    lv_obj_t * label = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_MSG_RECEIVED) {
        lv_msg_t * m = lv_event_get_msg(e);
        if(lv_msg_get_id(m) == MSG_UPDATE) {
            const int32_t * v = lv_msg_get_payload(m);
           // printf("%s  %d \n", __func__,*v);
           // lv_label_set_text_fmt(label, "%d %%", *v);  
     
	       lv_label_set_text(label, full_color_array[*v].name);//plabel_txt[*v]);
	       printf("%d %s--0x%06x\n", *v, full_color_array[*v].name, full_color_array[*v].color_value);
		   //change_color(*v);
		   factory_color_test(*v);
        }
    }
}
#endif
#endif

