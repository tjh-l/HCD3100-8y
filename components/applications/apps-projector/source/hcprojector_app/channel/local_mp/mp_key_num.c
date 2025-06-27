#include <sys/ioctl.h>
#include <hcuapi/snd.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"
//#include "../screen.h"
#include "mp_key_num.h"
#include "mp_mainpage.h"
#include "local_mp_ui.h"
#include "local_mp_ui_helpers.h"
#include "key_event.h"
#include "com_api.h"
extern uint16_t xpt2046_get_adc(void);
static lv_obj_t *digital_label = NULL;
static lv_timer_t *key_num_timer;
static lv_timer_t *msg_timer = NULL;
static lv_group_t* group_key_num;
#define ADC_TIMER	30000
static int cur_num=0;

static int warning_box = 0;
extern void _ui_screen_change(lv_obj_t * target,  int spd, int delay);
static lv_group_t * tmp_group=NULL;
static void key_num_timer_handler(lv_timer_t *timer_)
{
	printf("%s,%d\n",__func__,__LINE__);
    close_digital_num(digital_label,(lv_group_t *)timer_->user_data);
}
int get_key_num(void)
{
	return cur_num;
}

void key_num_keyinput_event_cb(lv_event_t *event)
{
	char tmp_num[16]={0};
	lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * parent_target = lv_event_get_target(event);
    lv_group_t * parent_group = lv_event_get_user_data(event);
	//printf("%s,%d,code:%d\n",__func__,__LINE__,code);
	lv_timer_reset(key_num_timer);
    if(code == LV_EVENT_KEY)
    {
		uint32_t value = lv_indev_get_key(lv_indev_get_act());
		int vkey = key_convert2_vkey(value);
		//printf("key code: %d\n", vkey);
		if(value== LV_KEY_ENTER)
		{
		printf("%s,%d\n",__func__,__LINE__);
			lv_timer_pause(key_num_timer);
			close_digital_num(digital_label,tmp_group);
		}
		else if(V_KEY_0 <= vkey && vkey <= V_KEY_9)
		{
			if(cur_num > 99999)
				return;
			cur_num = cur_num*10+vkey;
			sprintf(tmp_num,"%d",cur_num);
			lv_label_set_text(lv_obj_get_child(digital_label,0),tmp_num);
		}
		else if(value == LV_KEY_ESC)
		{
			cur_num = -1;
			lv_timer_pause(key_num_timer);
			close_digital_num(digital_label,tmp_group);
		}
		/*
		if(key_num_timer != NULL)
		{
		    lv_timer_pause(key_num_timer);
		    lv_timer_del(key_num_timer);
			key_num_timer = NULL;
		}
		key_num_timer = lv_timer_create(key_num_timer_handler, 5000, parent_group);
		*/
	}
}

void open_digital_num(lv_obj_t *parent,int value,lv_group_t *group)
{
	char tmp_num[16]={0};
	group_key_num = lv_group_create();
    set_key_group(group_key_num);
	tmp_group = group;
#if 1
	printf("%d\n",__LINE__);
	digital_label = lv_obj_create(parent);
	lv_obj_set_pos(digital_label,LV_PCT(12),LV_PCT(6));
	//lv_obj_set_align(digital_label,LV_ALIGN_CENTER);
	lv_obj_set_style_text_color(digital_label, lv_color_black(), 0);
    lv_obj_set_size(digital_label, LV_PCT(15), LV_PCT(10));
	lv_obj_set_style_border_width(digital_label, 3, 0);
    lv_obj_set_style_border_color(digital_label, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_outline_width(digital_label, 5, 0);
	lv_obj_set_style_outline_color(digital_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_radius(digital_label, 5, 0);
	lv_obj_set_style_text_font(digital_label,osd_font_get(FONT_MID),0);
	//lv_obj_set_align(digital_label, LV_ALIGN_CENTER);
	lv_group_add_obj(group_key_num,digital_label);
	cur_num = value;
	lv_obj_t *digital_num = lv_label_create(digital_label);
	sprintf(tmp_num,"%d",cur_num);
	lv_label_set_text(digital_num,tmp_num);
    //lv_obj_add_style(digital_label, &style_pr, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_event_cb(digital_label,key_num_keyinput_event_cb,LV_EVENT_KEY,NULL);

	key_num_timer = lv_timer_create(key_num_timer_handler, 5000, group);
	
#endif
}

void close_digital_num(lv_obj_t *parent,lv_group_t *group)
{
	lv_group_remove_all_objs(group_key_num);
    lv_group_del(group_key_num);
	control_msg_t objfresh_msg;
	if(digital_label!=NULL)
	{
		lv_obj_add_flag(digital_label,LV_OBJ_FLAG_HIDDEN);
		lv_obj_clean(digital_label);
	}
	if(key_num_timer != NULL)
	{
	    lv_timer_pause(key_num_timer);
	    lv_timer_del(key_num_timer);
		key_num_timer = NULL;
	}
	
	objfresh_msg.msg_type = MSG_TYPE_REMOTE;
    objfresh_msg.msg_code = MSG_TYPE_KEY_NUM;
    api_control_send_msg(&objfresh_msg);
	printf("%s,%d\n",__func__,__LINE__);
	set_key_group(group);
	//_ui_screen_change(parent,0,0);
}

