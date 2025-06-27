/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2023-08-08 15:05:47
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


#include <fcntl.h>
#include <hcuapi/pq.h>
#include <sys/ioctl.h>

#include "app_config.h"
#include "screen.h"
#include "com_api.h"
#include "lvgl/lvgl.h"
#include "ui_factory_test.h"
#include "osd_com.h"
#include "factory_setting.h"
#include "setup.h"
#include "../cvbs_in/cvbs_rx.h"
#ifdef CVBSIN_TRAINING_SUPPORT
#include "flash_otp.h"
#endif
#ifdef HC_FACTORY_TEST

#ifdef __linux__
#include "io.h"
#include <linux/fb.h>
#else
#include <kernel/io.h>
#include <kernel/vfs.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/fb.h>
#include <hcuapi/fb.h>
#include <hcuapi/dis.h>
#endif

#define DEBUG_PRINT(fmt, ...)	printf("%s(), line:%d. "fmt"\n", __func__, __LINE__,##__VA_ARGS__)


#define DDR_REG1  0xb8800070
#define DDR_REG2  0xb8801000
#define MAIN_CPU_REG1 0xb8800070
#define MAIN_CPU_REG2 0xb880007c
#define MAIN_CPU_REG3 0xb8800380


#define UI_OPA_PERCENT	LV_OPA_60
//#define UI_OPA_PERCENT	LV_OPA_COVER

#define INFO_LEN_MAX   256
static void win_factory_setings_create(lv_obj_t* p);
static void imageset_event_handler(lv_event_t* e, void* argv,int argc);
typedef enum{
	ID_FULL_COLOR_DISPLAY= 0,
	ID_IMAGE_SET,
	ID_ENABLE_PQ,
	ID_DYN_CONTRAST,
	ID_CVBS,
	ID_HW_INFO,
#ifdef CVBSIN_TRAINING_SUPPORT
	ID_CVBS_TRAINING,
#endif	
	ID_MAX
}FSET_ID;


lv_obj_t *factory_settings_scr = NULL;	
lv_obj_t *factSet_list = NULL;	
lv_group_t *facty_g = NULL;

static int imageset_idx=0;
static char cvbs_num[4] = "128";
static const char * cvbs_mmap[] = {LV_SYMBOL_LEFT, cvbs_num, LV_SYMBOL_RIGHT, ""};
static uint32_t fc_item_label[]={
	STR_FULL_COLOR_DIS,
	STR_IMAGE_SET,
	STR_ENABLE_PQ,
	STR_DYNAMIC_CTR,
	STR_CVBS_AFE,
	STR_HW_INFO,
#ifdef CVBSIN_TRAINING_SUPPORT
	STR_CVBS_TRAINING,
#endif	
};
#define MAX_FC_ITEM_NUM  ID_MAX

static int pq_enable(bool en)
{
	int pq_fd = -1;
	pq_fd = open("/dev/pq" , O_WRONLY);
    if(pq_fd < 0)
    {
        return -1;
    }

    if(en)
    	ioctl(pq_fd , PQ_START);
    else
    	ioctl(pq_fd , PQ_STOP);
    close(pq_fd);

    return 0;
}

static int dyn_contrast_enable(bool en)
{
	int fd = -1;
	struct dis_dyn_enh_onoff dyn_enh = { 0 };
	fd = open("/dev/dis" , O_WRONLY);
	if(fd < 0)
	{
		 return -1;
	}
	dyn_enh.onoff = en?1:0;
	dyn_enh.distype = DIS_TYPE_HD;
	ioctl(fd , DIS_DENH_SET_ONOFF, &dyn_enh);
	close(fd);
	return 0;
}

static void meminfo(char *pbuf)
{
	uint32_t size = 0, type = 0, frequency = 0, ic;

	type = REG32_GET_FIELD2(DDR_REG2, 23, 1);
	size = REG32_GET_FIELD2(DDR_REG2, 0, 3);
	frequency = REG32_GET_FIELD2(DDR_REG1, 4, 3);

	type = type + 2;

	size = 16 << (size);

	switch (frequency) {
	case 0:
		frequency = 800;
		break;
	case 1:
		frequency = 1066;
		break;
	case 2:
		frequency = 1333;
		break;
	case 3:
		frequency = 1600;
		break;
	case 4:
		frequency = 600;
		break;
	default:
		break;
	}

	ic = REG8_READ(0xb8800003);
	if (REG16_GET_BIT(0xb880048a, BIT15)) {
		if (ic == 0x15) 
			frequency = REG16_GET_FIELD2(0xb880048a, 0, 15) * 12;
		else if (ic == 0x16)
			frequency = REG16_GET_FIELD2(0xb880048a, 0, 15) * 24;
	}

	snprintf(pbuf, 80,"\nDDR Info:\n\tType:\tDDR%d\n\tSize:\t%dM\n\tClock:\t%dMHz\n\n",
				 type, size, frequency);

}

static void cpuinfo(char *buf)
{
#ifdef CONFIG_SOC_HC15XX	//hc15XX
	int M15_CPU_frequency;
	int cpu0_flag1=(REG32_READ(MAIN_CPU_REG1)>>8 & 0x07);

	if(cpu0_flag1==7)//digital clock
	{
		int cpu0_flag2=(REG32_READ(MAIN_CPU_REG2)>>7 & 0x01);
		if(cpu0_flag2==0) M15_CPU_frequency=594;
		else
		{
			int cpu0_flag3=(REG32_READ(MAIN_CPU_REG3));	
			int MCPU_DIG_PLL_M=cpu0_flag3>>16 & 0X3FF;
			int MCPU_DIG_PLL_N=cpu0_flag3>>8  & 0X3F;
			int MCPU_DIG_PLL_L=cpu0_flag3  & 0X3F;
			M15_CPU_frequency=24*(MCPU_DIG_PLL_M+1)/(MCPU_DIG_PLL_N+1)*(MCPU_DIG_PLL_L+1);
		}
	}
	else//simulation clock
	{
		switch (cpu0_flag1)
		{
			case 0:
				M15_CPU_frequency=594;
				break;
			case 1:
				M15_CPU_frequency=396;
				break;
			case 2:
				M15_CPU_frequency=297;
				break;
			case 3:
				M15_CPU_frequency=198;
				break;	
			case 4:
				M15_CPU_frequency=198;
				break;
			case 5:
				M15_CPU_frequency=198;
				break;
			default :
				M15_CPU_frequency=198;
				break;					
		}		
	}
	snprintf(buf, 32, "CPU clk: %dMHz\n", M15_CPU_frequency);
#else	
	int Main_CPU_frequency;	

	int cpu1_flag1=(REG32_READ(MAIN_CPU_REG1)>>8 & 0x07);
	if(cpu1_flag1==7)//digital clock
	{
		int cpu1_flag2=(REG32_READ(MAIN_CPU_REG2)>>7 & 0x01);
		if(cpu1_flag2==0) Main_CPU_frequency=594;
		else
		{
			int cpu1_flag3=(REG32_READ(MAIN_CPU_REG3));	
			int MCPU_DIG_PLL_M=cpu1_flag3>>16 & 0X3FF;
			int MCPU_DIG_PLL_N=cpu1_flag3>>8  & 0X3F;
			int MCPU_DIG_PLL_L=cpu1_flag3  & 0X3F;
			Main_CPU_frequency=24*(MCPU_DIG_PLL_M+1)/(MCPU_DIG_PLL_N+1)*(MCPU_DIG_PLL_L+1);
		}
	}

	else//simulation clock
	{
		switch (cpu1_flag1)
		{
			case 0:
				Main_CPU_frequency=594;
				break;
			case 1:
				Main_CPU_frequency=396;
				break;
			case 2:
				Main_CPU_frequency=297;
				break;
			case 3:
				Main_CPU_frequency=198;
				break;	
			case 4:
				Main_CPU_frequency=900;
				break;
			case 5:
				Main_CPU_frequency=1118;
				break;
			default :
				Main_CPU_frequency=24;
				break;					
		}		
	}
	snprintf(buf, 32,"CPU clk: %dMHz\n", Main_CPU_frequency);
#endif

}

static int ft_get_lcd_info(char *buf)
{
	struct fb_var_screeninfo vinfo;
	int disfd = -1;
	int fbfd = -1;
	struct dis_screen_info screen = { 0 };
	int rotate = -1;
	int flip = -1;
	
	// Get variable screen information
	fbfd = open("/dev/fb0", O_RDWR);
	if (fbfd < 0) {
		return -1;
	}
	if(ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("Error reading variable information");
		return -1;
	}
	close(fbfd);

	disfd = open("/dev/dis", O_RDWR);
	if (disfd < 0) {
		return -1;
	}

	screen.distype = DIS_TYPE_HD;
	if(ioctl(disfd, DIS_GET_SCREEN_INFO, &screen)) {
		close(disfd);
		return -1;
	}
	close(disfd);
	api_get_flip_info(&rotate, &flip);
	snprintf(buf,64,"FB0:%ux%u\tLCD:%ux%u\nRotate:%d\n",(unsigned int)vinfo.xres, (unsigned int)vinfo.yres, screen.area.w, screen.area.h,rotate);
	return 0;
}

static void ft_get_hw_info(char *info)
{
	char buf[256];
	int len = 0;

	//OS
	memset(buf, 0, 256);
#ifdef __linux__	
	sprintf(buf, "OS: linux\n");
#else
	sprintf(buf, "OS: rtos\n");
#endif	
	memcpy(info+len, buf, strlen(buf));
	len += strlen(buf);

	memset(buf, 0, 256);
	cpuinfo(buf);
	memcpy(info+len, buf, strlen(buf));
	len += strlen(buf);
	
	//mem info
	memset(buf, 0, 256);
	meminfo(buf);
	memcpy(info+len, buf, strlen(buf));
	len += strlen(buf);
	
	memset(buf, 0, 256);
	ft_get_lcd_info(buf);
	memcpy(info+len, buf, strlen(buf));
	len += strlen(buf);
	
	uint32_t dv = REG32_READ(0xB880817C);
	snprintf(info+len,32, "Dview: %ux%u\n", (unsigned int)dv&0xffff, (unsigned int)(dv>>16)&0xffff);	
//	memcpy(info+len, buf, len);
}


#if 1
lv_obj_t * cvbs_btnm = NULL;
static lv_obj_t *info_scr = NULL;
//static lv_group_t *di_g=NULL;


static void ft_display_event_handle(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    /*if(code== LV_EVENT_SCREEN_LOAD_START){
		key_set_group(di_g);
	}	
	else if (code == LV_EVENT_SCREEN_UNLOAD_START) {
		//lv_group_del(facty_g);
		lv_group_set_default(NULL);
	}
	else */if(code== LV_EVENT_KEY){
		printf("%s,  LV_EVENT_KEY\n",__func__);
		uint32_t lv_key = lv_indev_get_key(lv_indev_get_act());
		if(lv_key== LV_KEY_ESC){
			lv_obj_del(obj);
			win_factory_setings_create(lv_scr_act());
			lv_obj_t *btn=lv_group_get_focused(lv_group_get_default());
			lv_group_focus_obj(lv_obj_get_child(btn->parent,ID_HW_INFO));
		}
	}
}
static void ft_display_info(void *parent, char *info)
{
	info_scr = lv_obj_create(parent);
	lv_obj_set_style_radius(info_scr,0,LV_PART_MAIN);
	lv_obj_set_style_bg_color(info_scr,lv_color_hex(0x303030),LV_PART_MAIN);
	lv_obj_set_style_bg_opa(info_scr, UI_OPA_PERCENT, LV_PART_MAIN);
	lv_obj_set_style_border_width(info_scr,0,LV_PART_MAIN);
	lv_obj_set_pos(info_scr, 0, 139);
	lv_obj_set_size(info_scr, LV_PCT(35), LV_PCT(70));
	lv_group_add_obj(lv_group_get_default(),info_scr);

	lv_obj_t *label = lv_label_create(info_scr);
	lv_label_set_recolor(label, true);
	lv_obj_set_style_text_color(label,lv_color_hex(0xffffff),LV_PART_MAIN);
	lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN);
	lv_obj_set_style_text_font(label,osd_font_get(FONT_MID),0);
	lv_label_set_text(label, info);
	lv_obj_add_event_cb(info_scr,ft_display_event_handle,LV_EVENT_KEY,NULL);
	

}
/* a struct to include obj choose item*/ 
typedef struct {
    uint32_t name;
	int32_t value;
	bool is_number;
    bool is_disabled;
    void (*event_func)(lv_event_t*);
} choose_item_t;



static void channel_switch_event(lv_event_t* e){
	imageset_event_handler(e, NULL, P_CUR_CHANNEL);
}
static void picture_mode_event(lv_event_t* e) {
    imageset_event_handler(e, NULL, P_PICTURE_MODE);
}
static void contrast_event(lv_event_t* e){
    imageset_event_handler(e,  NULL, P_CONTRAST);
}
static void brightness_event(lv_event_t* e){
    imageset_event_handler(e,  NULL, P_BRIGHTNESS);
}
static void color_event(lv_event_t* e){
    imageset_event_handler(e,NULL, P_COLOR);
}
static void sharpness_event(lv_event_t* e){
    imageset_event_handler(e, NULL, P_SHARPNESS);
}
static void color_temperature_event(lv_event_t* e){
    imageset_event_handler(e, NULL, P_COLOR_TEMP);
}

static void win_choose_item_create(lv_obj_t* p,choose_item_t* imageset_menu)
{
	lv_obj_t* cont = lv_btn_create(p);
	lv_obj_set_size(cont,LV_PCT(100),40);
	// lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_FULL, 0);
	lv_obj_set_style_radius(cont, 0, LV_PART_MAIN);
	lv_obj_set_style_shadow_color(cont, lv_color_hex(0xffff00), LV_PART_MAIN);
	lv_obj_set_style_shadow_width(cont,0,LV_PART_MAIN);
	//lv_obj_set_style_bg_color(cont, lv_color_hex(0x292929), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);

	lv_obj_set_style_border_side(cont,LV_BORDER_SIDE_FULL,0);
	lv_obj_set_style_border_width(cont,0,0);
	lv_obj_add_event_cb(cont, imageset_menu->event_func, LV_EVENT_ALL, NULL);
	

	lv_obj_t* label = lv_label_create(cont);
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);	
	if (!imageset_menu->is_disabled){
		lv_obj_set_style_text_color(cont, lv_color_white(), 0);
		set_label_text2(label, imageset_menu->name,  FONT_MID);
	}else{
		lv_obj_set_style_text_color(cont, lv_color_make(125,125,125), 0);
		set_label_text2(label, imageset_menu->name, FONT_MID);
		lv_obj_add_state(cont, LV_STATE_DISABLED);
	}

	label = lv_label_create(cont);
	lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_label_set_recolor(label, true);		
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	
	if(!imageset_menu->is_number){
		if (!imageset_menu->is_disabled){
			set_label_text2(label, imageset_menu->value, FONT_MID);
		}else{
			set_label_text2(label, imageset_menu->value, FONT_MID);
		}
	}else if(imageset_menu->is_number){
		lv_label_set_text_fmt(label,"%d", (int)imageset_menu->value);
		lv_obj_set_style_text_font(label,osd_font_get(FONT_MID), 0);
	}else{
		lv_label_set_text(label, " ");
	}
	return ;
}

/*The order of the elements of the array must be the same as 
the order defined by the structure,like picture_mode_e*/ 

static int channel_title_vec[]={
	STR_AV_TITLE,STR_HDMI_TITLE,STR_MEDIA_TITLE,
};
static int pic_mode_vec[]={
	STR_STANDARD, STR_DYNAMIC,STR_MILD, STR_USER
};

static int color_temp_vec[] = {STR_COLD, STR_STANDARD, STR_WARM};

static void win_factory_imageset_create(lv_obj_t* parent)
{
	lv_obj_t* title = lv_obj_create(parent);
	lv_obj_set_pos(title, 0, 100);
	lv_obj_set_size(title, LV_PCT(35), 40);
	lv_obj_clear_flag(title,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(title, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(title, lv_color_hex(0x505050), LV_PART_MAIN);
	lv_obj_set_style_border_width(title,0,LV_PART_MAIN);
	lv_obj_add_state(title,LV_STATE_DISABLED);

	lv_obj_t * label = lv_label_create(title);
	lv_label_set_recolor(label, true);
	lv_obj_set_style_text_color(label,lv_color_hex(0x000000),0);
	set_label_text2(label,STR_IMAGE_SET,FONT_MID);
	lv_obj_set_align(label,LV_ALIGN_CENTER);
	
	lv_obj_t* cont = lv_obj_create(parent);
	lv_obj_set_style_radius(cont,0,LV_PART_MAIN);
	lv_obj_set_style_bg_color(cont,lv_color_hex(0x292929),LV_PART_MAIN);
	lv_obj_set_style_bg_opa(cont, UI_OPA_PERCENT, LV_PART_MAIN);

	lv_obj_set_style_border_width(cont,0,LV_PART_MAIN);
	lv_obj_set_pos(cont, 0, 139);
	lv_obj_set_size(cont, LV_PCT(35), LV_PCT(70));
	lv_obj_set_flex_flow(cont,LV_FLEX_FLOW_COLUMN);

	/*del group's old obj and add new obj in group when create new obj
	and then image_menu's obj can group focus next by key one by one  */
	int oldobj_cnt=lv_group_get_obj_count(lv_group_get_default());
	if(oldobj_cnt>0){
		lv_group_remove_all_objs(lv_group_get_default());
	}
	choose_item_t imageset_menu[]={
		// {.name=STR_CHANNEL_TITLE,.value=channel_title_vec[factory_get_some_sys_param(P_CUR_CHANNEL)],.is_number = false,.event_func=channel_switch_event},
		{.name=STR_PICTURE_MODE,.value=pic_mode_vec[factory_get_some_sys_param(P_PICTURE_MODE)],.is_number = false,.event_func=picture_mode_event},
		{.name=STR_CONSTRAST, .value= factory_get_some_sys_param(P_CONTRAST),.is_number=true, .event_func=contrast_event},
		{.name=STR_BRIGHTNESS, .value= factory_get_some_sys_param(P_BRIGHTNESS),.is_number=true,.event_func= brightness_event},
		{.name=STR_COLOR, .value= factory_get_some_sys_param(P_COLOR),.is_number=true,  .event_func=color_event},
		{.name=STR_SHARPNESS, .value= factory_get_some_sys_param(P_SHARPNESS),.is_number=true, .event_func=sharpness_event},
		{.name=STR_COLOR_TEMP, .value=color_temp_vec[factory_get_some_sys_param(P_COLOR_TEMP)],.is_number=false,.event_func=color_temperature_event},
	};
	for(int i=0;i<IAMGE_SET_ITEM_MAX;i++){
		win_choose_item_create(cont,imageset_menu+i);
	}
	

}
static lv_obj_t* win_adjustbar_create(lv_obj_t* p,int name_id, int value, int min, int max){
    lv_obj_t *m_bar = lv_obj_create(p);
    lv_obj_align(m_bar, LV_ALIGN_TOP_MID, lv_pct(44), 20);
    lv_obj_set_size(m_bar, lv_pct(5), lv_pct(70));
    lv_obj_set_style_bg_color(m_bar, lv_color_hex(0x303030), 0);
    lv_obj_set_flex_flow(m_bar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(m_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_border_width(m_bar, 0, 0);
    lv_obj_clear_flag(m_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_hor(m_bar, 0, 0);
   
    

    // lv_obj_t *label_name = lv_label_create(m_bar);
    // lv_obj_set_size(label_name, lv_pct(100), LV_SIZE_CONTENT);
    // lv_label_set_text(label_name, api_rsc_string_get(name_id));
    // lv_obj_set_style_text_font(label_name, osd_font_get(FONT_SMALL), 0);
    // lv_obj_set_style_text_color(label_name, lv_color_hex(0x8a86b8), 0);
    // lv_obj_set_style_text_align(label_name, LV_TEXT_ALIGN_CENTER, 0);
    // lv_label_set_long_mode(label_name, LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_obj_t *slider_obj = lv_slider_create(m_bar);
    lv_obj_set_size(slider_obj, lv_pct(60), lv_pct(100));
    //lv_obj_set_style_anim_time(slider_obj, 100, 0);
    lv_slider_set_range(slider_obj, min, max);
    lv_slider_set_value(slider_obj, value, LV_ANIM_OFF);
    lv_obj_set_style_bg_opa(slider_obj, LV_OPA_0, LV_PART_KNOB);
    // lv_obj_set_style_border_width(slider_obj, 0, 0);
    // lv_obj_set_style_outline_width(slider_obj, 0, 0);
    // lv_obj_set_style_shadow_width(slider_obj, 0, 0);
    

    lv_obj_t*  label_name = lv_label_create(slider_obj);
    lv_label_set_text_fmt(label_name,"%d", value);
    lv_obj_set_style_text_color(label_name, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_name, osd_font_get(FONT_MID), 0);
    lv_obj_center(label_name);
    
    return slider_obj;
}


static void adjustbar_event_handle(lv_event_t *e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    uint32_t item = (uint32_t)lv_event_get_user_data(e);
    static int cur_v=0;
    if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_UP || key == LV_KEY_RIGHT){
               cur_v+=1;

            if(item == P_BASS){
                if(cur_v>10){
                    cur_v=10;
                    return;
                }
                // set_twotone(SND_TWOTONE_MODE_USER, factory_get_some_sys_param(P_TREBLE), cur_v);
            }else if (item == P_TREBLE){
                if(cur_v>10){
                    cur_v=10;
                    return;
                }
                // set_twotone(SND_TWOTONE_MODE_USER, cur_v, factory_get_some_sys_param(P_BASS));
            }else if(item == P_CONTRAST || item == P_COLOR ||  item == P_BRIGHTNESS || item == P_HUE){
                if(cur_v>100){
                    cur_v=100;
                    return;
                }
                factory_venhance_set(item,cur_v);
				factory_set_some_sys_param(item,cur_v);
            }else if(item == P_BALANCE){
                if(cur_v>24){
                    cur_v=24;
                    return;
                }
                set_balance(cur_v);
            }else if(item == P_SHARPNESS){
                if(cur_v>10){
                    cur_v=10;
                    return;
                }
                factory_venhance_set(item,cur_v);
				factory_set_some_sys_param(item,cur_v);
            }
            lv_label_set_text_fmt(lv_obj_get_child(obj, 0), "%d", cur_v);
        }else if(key == LV_KEY_DOWN ||key == LV_KEY_LEFT){
           
            cur_v-=1;
            if(item == P_BASS){
                if(cur_v<-10){
                    cur_v=-10;
                    return;
                }
                // set_twotone(SND_TWOTONE_MODE_USER, factory_get_some_sys_param(P_TREBLE), cur_v);
            }else if (item == P_TREBLE){
                if(cur_v<-10){
                    cur_v=-10;
                    return;
                }                
                // set_twotone(SND_TWOTONE_MODE_USER, cur_v, factory_get_some_sys_param(P_BASS));
            }else if(item == P_CONTRAST || item == P_COLOR ||  item == P_BRIGHTNESS || item == P_HUE){
                if(cur_v<0){
                    cur_v=0;
                    return;
                }
                factory_venhance_set(item,cur_v);
				factory_set_some_sys_param(item,cur_v);
            }else if(item == P_BALANCE){
                if(cur_v<-24){
                    cur_v=-24;
                    return;
                }
                set_balance(cur_v);
            }else if(item == P_SHARPNESS){
                if(cur_v<0){
                    cur_v=0;
                    return;
                }
                factory_venhance_set(item,cur_v);
				factory_set_some_sys_param(item,cur_v);
            }
            lv_label_set_text_fmt(lv_obj_get_child(obj, 0), "%d", cur_v);
        }else if(key == LV_KEY_ESC || key==LV_KEY_ENTER ||  key == LV_KEY_HOME){
			projector_sys_param_save();
            lv_obj_del(obj->parent);
			win_factory_imageset_create(lv_scr_act());
			/*reset foucus */
			lv_obj_t *btn=lv_group_get_focused(lv_group_get_default());
			lv_group_focus_obj(lv_obj_get_child(btn->parent,imageset_idx));


        }      
    }else if(code == LV_EVENT_FOCUSED){
        cur_v = factory_get_some_sys_param(item);
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
        if(dsc && dsc->rect_dsc){
            dsc->rect_dsc->outline_width=0;           
        }

    }
}

static void imageset_menu_value_set(uint32_t item){
	lv_obj_t* scr=lv_scr_act();
    lv_obj_t *slider=NULL;

    int v = factory_get_some_sys_param(item);
    switch (item)
    {
    case P_BASS:
        slider=win_adjustbar_create(scr,STR_BASS, v, -10, 10);
        break;
    case P_TREBLE:
        slider=win_adjustbar_create(scr,STR_TREBLE, v, -10, 10);
        break;
    case P_CONTRAST:
        slider=win_adjustbar_create(scr,STR_CONSTRAST, v, 0, 100);
        break;
    case P_COLOR:
        slider=win_adjustbar_create(scr,STR_COLOR, v, 0, 100);
        break;
    case P_BRIGHTNESS:
        slider=win_adjustbar_create(scr,STR_BRIGHTNESS, v, 0, 100);
        break;
    case P_HUE:
        slider=win_adjustbar_create(scr,STR_HUE, v, 0, 100);
        break;
    case P_BALANCE:
        slider=win_adjustbar_create(scr,STR_BALANCE, v, -24, 24);
        break;
    case P_SHARPNESS:
        slider=win_adjustbar_create(scr,STR_SHARPNESS, v, 0, 10);
        break;
    default:
        break;
    }

    if(slider){
        lv_obj_add_event_cb(slider, adjustbar_event_handle, LV_EVENT_ALL, (void*)item);
        lv_group_focus_obj(slider);
    }
}



static int adjustlist_pictureMode_changed(int k){    
    uint8_t ops[4] = {P_CONTRAST, P_BRIGHTNESS, P_COLOR, P_SHARPNESS};
    char* values[4] = { k==0 ? "50" : k==1 ? "60" : "45",
                        k==0 ? "50" : k==1 ? "49" : "50",
                        k==0 ? "50" : k==1 ? "60" : "45",
                        k==0 ? "5" : k==1 ? "5" : "4"};
	int picture_mode_pre_text[4] = {50, 50, 50, 5};
    switch (k) {
        case PICTURE_MODE_STANDARD:
        case PICTURE_MODE_DYNAMIC:
        case PICTURE_MODE_MILD:
            for(int i=0; i< 4; i++){
				factory_venhance_set(ops[i],strtol(values[i], NULL, 10));
				factory_set_some_sys_param(ops[i],strtol(values[i], NULL, 10));
            }
            break;
        case PICTURE_MODE_USER:
            for (int i = 0; i < 4; i++) {
				factory_venhance_set(ops[i],picture_mode_pre_text[i]);
				factory_set_some_sys_param(ops[i],strtol(values[i], NULL, 10));
            }
            break ;
        default:
            break;
    }

    return 0;
}

static void adjustlist_event_handle(lv_event_t * e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = lv_event_get_target(e);
	void*  user_data=lv_event_get_user_data(e);
	if(code==LV_EVENT_KEY){
		uint32_t lv_key = lv_indev_get_key(lv_indev_get_act());
		if(lv_key==LV_KEY_UP){
			lv_group_focus_prev(lv_group_get_default());
		}else if(lv_key==LV_KEY_DOWN){
			lv_group_focus_next(lv_group_get_default());
		}else if(lv_key==LV_KEY_ESC){
			/*del its cont and then create new cont */ 
			lv_obj_del(target->parent);
			win_factory_imageset_create(lv_scr_act());
			/*reset foucus */
			lv_obj_t *btn=lv_group_get_focused(lv_group_get_default());
			lv_group_focus_obj(lv_obj_get_child(btn->parent,imageset_idx));
		}else if(lv_key==LV_KEY_LEFT){

		}
	}else if(code==LV_EVENT_PRESSED){
		int title_id=(int)user_data;
		// printf(">>! title_id:%d ,user_data:%x\n", title_id,user_data);
		uint32_t sel_idx=lv_obj_get_index(target);
		switch(title_id){
			case STR_CHANNEL_TITLE:
				factory_set_some_sys_param(P_CUR_CHANNEL,sel_idx);
				if(sel_idx==CVBS_CHANNEL){
					// change_screen(SCREEN_CHANNEL_CVBS);
					/*need to reset some logic*/ 
				}else if(sel_idx==HDMI_CHANNEL){
					// change_screen(SCREEN_CHANNEL_HDMI);
				}else if(sel_idx==MUTL_MEDIA_CHANNEL){
				}
				/*to switch channel ?*/ 
				break;
			case STR_PICTURE_MODE:
				factory_set_some_sys_param(P_PICTURE_MODE,sel_idx);
				adjustlist_pictureMode_changed(sel_idx);
				/*to set someting */ 
				break;
			case STR_COLOR_TEMP:
				factory_set_some_sys_param(P_COLOR_TEMP,sel_idx);
				set_color_temperature(sel_idx);
				break;
			default:
				break;
		}
		
	}
}
static void win_adjustlist_create(lv_obj_t* p,uint32_t name_id ,int* item_arr,int item_cnt)
{
	lv_obj_t *aplist_title = lv_obj_create(p);
	lv_obj_set_pos(aplist_title, 0, 100);
	lv_obj_set_size(aplist_title, LV_PCT(35), 40);
	lv_obj_clear_flag(aplist_title,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(aplist_title, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(aplist_title, lv_color_hex(0x505050), LV_PART_MAIN);
	lv_obj_set_style_border_width(aplist_title,0,LV_PART_MAIN);
	lv_obj_add_state(aplist_title,LV_STATE_DISABLED);

	lv_obj_t *label = lv_label_create(aplist_title);
	lv_label_set_recolor(label, true);
	lv_obj_set_style_text_color(label,lv_color_hex(0x000000),0);
	set_label_text2(label,name_id,FONT_MID);
	lv_obj_set_align(label,LV_ALIGN_CENTER);

	lv_obj_t* cont = lv_obj_create(p);
	lv_obj_set_style_radius(cont,0,LV_PART_MAIN);
	lv_obj_set_style_bg_color(cont,lv_color_hex(0x303030),LV_PART_MAIN);
	lv_obj_set_style_bg_opa(cont, UI_OPA_PERCENT, LV_PART_MAIN);
	lv_obj_set_style_border_width(cont,0,LV_PART_MAIN);
	lv_obj_set_pos(cont, 0, 139);
	lv_obj_set_size(cont, LV_PCT(35), LV_PCT(70));
	lv_obj_set_flex_flow(cont,LV_FLEX_FLOW_COLUMN);
	
	for(int i=0;i<item_cnt;i++){
		lv_obj_t* btn = lv_btn_create(cont);
		lv_obj_set_size(btn,LV_PCT(100),40);
		lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_FULL, 0);
		lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
		lv_obj_set_style_shadow_color(btn, lv_color_hex(0xffff00), LV_PART_MAIN);
		lv_obj_set_style_shadow_width(btn,0,LV_PART_MAIN);
		//lv_obj_set_style_bg_color(btn, lv_color_hex(0x292929), LV_PART_MAIN);
		lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);

		lv_obj_set_style_border_side(btn,LV_BORDER_SIDE_FULL,0);
		lv_obj_set_style_border_width(btn,0,0);
		lv_obj_add_event_cb(btn, adjustlist_event_handle, LV_EVENT_ALL, (void*)name_id);

		label = lv_label_create(btn);
		lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);	
		lv_obj_set_style_text_color(btn, lv_color_white(), 0);
		set_label_text2(label, item_arr[i],  FONT_MID);
	}

}

static void imageset_menu_item_set(uint32_t item)
{
	int cnt=0;
    switch (item)
    {
    case P_CUR_CHANNEL:
		cnt=sizeof(channel_title_vec)/sizeof(channel_title_vec[0]);
		win_adjustlist_create(lv_scr_act(),STR_CHANNEL_TITLE,channel_title_vec,cnt);
        break;
    case P_PICTURE_MODE:
		cnt=sizeof(pic_mode_vec)/sizeof(pic_mode_vec[0]);
		win_adjustlist_create(lv_scr_act(),STR_PICTURE_MODE,pic_mode_vec,cnt);
        break;
    case P_COLOR_TEMP:
		cnt=sizeof(color_temp_vec)/sizeof(color_temp_vec[0]);
		win_adjustlist_create(lv_scr_act(),STR_COLOR_TEMP,color_temp_vec,cnt);
        break;
    default:
        break;
    }
}
static void imageset_event_handler(lv_event_t* e,void* argv,int argc)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
	if(code == LV_EVENT_PRESSED){
        lv_obj_clear_state(target, LV_STATE_PRESSED);
		imageset_idx=lv_obj_get_index(target);
		if(argc==P_BRIGHTNESS||argc==P_CONTRAST||argc==P_COLOR
			||argc==P_SHARPNESS){
			lv_obj_clean(lv_scr_act());
			imageset_menu_value_set(argc);
		}else if(argc==P_CUR_CHANNEL||argc==P_PICTURE_MODE
			||argc==P_COLOR_TEMP){
			// for test to know if del obj ,its event will abort? NO
			lv_obj_clean(lv_scr_act());
			imageset_menu_item_set(argc);
		}
	}else if(code==LV_EVENT_KEY){
		uint32_t lv_key = lv_indev_get_key(lv_indev_get_act());
		if(lv_key==LV_KEY_UP){
			lv_group_focus_prev(lv_group_get_default());
		}else if(lv_key==LV_KEY_DOWN){
			lv_group_focus_next(lv_group_get_default());
		}else if(lv_key==LV_KEY_ESC){
			/*del its cont and then create new cont */ 
			lv_obj_clean(lv_scr_act());
			win_factory_setings_create(factory_settings_scr);
			lv_obj_t *btn=lv_group_get_focused(lv_group_get_default());
			lv_group_focus_obj(lv_obj_get_child(btn->parent,ID_IMAGE_SET));
		}else if(lv_key==LV_KEY_LEFT){

		}
	}
}

static uint32_t m_hotkey[] = {KEY_POWER};
static void ft_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
	if(obj==factory_settings_scr){
		if(code== LV_EVENT_SCREEN_LOAD_START){
			facty_g = lv_group_create();
			key_set_group(facty_g);
			/*create a new group before create new obj .
				so that it can add into new group */ 

			api_hotkey_enable_set(m_hotkey, sizeof(m_hotkey)/sizeof(m_hotkey[0]));
			win_factory_setings_create(factory_settings_scr);
		}else if (code == LV_EVENT_SCREEN_UNLOAD_START) {
			lv_group_set_default(NULL);
			lv_obj_clean(factory_settings_scr);
			api_hotkey_disable_clear();
		}
	}
	if(code==LV_EVENT_PRESSED){
        lv_obj_clear_state(obj, LV_STATE_PRESSED);
	}else if(code== LV_EVENT_KEY){
        uint8_t key = lv_indev_get_key(lv_indev_get_act());		
        if(key == LV_KEY_ENTER){
			if(lv_obj_get_index(obj) == ID_FULL_COLOR_DISPLAY){
				lv_obj_add_flag(factory_settings_scr,LV_OBJ_FLAG_HIDDEN);
				lv_group_set_default(NULL);
				full_color_page_test(factory_settings_scr);//
				change_screen(SCREEN_FTEST_FULL_COLOR_DIS);
			}else if(lv_obj_get_index(obj)==ID_IMAGE_SET){
				lv_obj_del(obj->parent);
				win_factory_imageset_create(lv_scr_act());
			}else if(lv_obj_get_index(obj) == ID_ENABLE_PQ){
				if(lv_obj_has_state(lv_obj_get_child(obj,1), LV_STATE_CHECKED)){
					lv_obj_clear_state(lv_obj_get_child(obj,1), LV_STATE_CHECKED);
					pq_enable(false);
					factory_set_some_sys_param(P_PQ_ONFF,0);
				}
				else{
					lv_obj_add_state(lv_obj_get_child(obj,1), LV_STATE_CHECKED);					
					pq_enable(true);
					factory_set_some_sys_param(P_PQ_ONFF,1);
				}
				//printf("State: %s\n", lv_obj_has_state(lv_obj_get_child(obj,1), LV_STATE_CHECKED) ? "On" : "Off");
			}
			else if(lv_obj_get_index(obj) == ID_DYN_CONTRAST){
				if(lv_obj_has_state(lv_obj_get_child(obj,1), LV_STATE_CHECKED)){
					lv_obj_clear_state(lv_obj_get_child(obj,1), LV_STATE_CHECKED);
					dyn_contrast_enable(false);
					factory_set_some_sys_param(P_DYN_CONTRAST_ONOFF,0);
				}
				else{
					lv_obj_add_state(lv_obj_get_child(obj,1), LV_STATE_CHECKED);					
					dyn_contrast_enable(true);
					factory_set_some_sys_param(P_DYN_CONTRAST_ONOFF,1);
				}
			}
			else if(lv_obj_get_index(obj) == ID_CVBS){
				printf("%s show cvbs info...\n", __func__);
				//ft_get_hw_info(NULL);
			}
			else if(lv_obj_get_index(obj) == ID_HW_INFO){
				lv_obj_del(obj->parent);
				char info[128];
				memset(info, 0, 128);
				ft_get_hw_info(info);
				ft_display_info(lv_scr_act(), info);
			}
		#ifdef CVBSIN_TRAINING_SUPPORT
			else if(lv_obj_get_index(obj) == ID_CVBS_TRAINING){
				cvbs_rx_training();
			}
		#endif
        }
		else if(key == LV_KEY_UP){
			lv_group_focus_prev(facty_g);
		}
		else if(key == LV_KEY_DOWN){
			lv_group_focus_next(facty_g);
		}		
		else if(key == LV_KEY_LEFT|| key == LV_KEY_RIGHT){
			if(lv_obj_get_index(obj) == ID_CVBS){
				lv_obj_t *btnm = lv_obj_get_child(obj,1);
				printf("CVBS: %s \n",lv_btnmatrix_get_btn_text(btnm, 1));
				const char ** map = lv_btnmatrix_get_map((lv_obj_t *)btnm);
				printf("map: %s, %s\n", *map, *(map+1));
				int x = 0;
				sscanf(lv_btnmatrix_get_btn_text(btnm, 1), "%d",&x);

	
				if(key == LV_KEY_LEFT){
					x-=1;
					if(x < 0)
						x= 0;
				}
				else if(key == LV_KEY_RIGHT){
					x+=1;
					if(x >255)
						x = 255;
				}

				sprintf(cvbs_num, "%d", x);
				//itoa(x, cvbs_mmap[1], 10);
				printf("itoa %s\n",cvbs_num);
				lv_btnmatrix_set_map(btnm, (const char**)cvbs_mmap);
				REG32_SET_FIELD2(0xb8800180,8,8,x);
			#ifdef CVBSIN_TRAINING_SUPPORT
				cvbs_training_value_set(x);
			#endif
				//factory_set_some_sys_param(P_CVBS_AEF_OFFSET,x);
			}
		}
		else if(key == LV_KEY_ESC){
		#ifdef CVBSIN_TRAINING_SUPPORT
			flash_otp_data_saved();
		#endif

			printf("factory settings LV_KEY_ESC\n");	
			// lv_obj_add_flag(info_scr, LV_OBJ_FLAG_HIDDEN);
			projector_sys_param_save();
			lv_obj_clean(lv_scr_act());
			change_screen(SCREEN_CHANNEL);
			
		}
	}
}

static void win_factory_setings_create(lv_obj_t* p)
{
	lv_obj_t * obj, *aplist_title;
	lv_obj_t * label;
	char buf[256];
	lv_obj_t * cont_col;

	//factory title
	aplist_title = lv_obj_create(p);
	lv_obj_set_pos(aplist_title, 0, 100);
	lv_obj_set_size(aplist_title, LV_PCT(35), 40);
	lv_obj_clear_flag(aplist_title,LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(aplist_title, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(aplist_title, lv_color_hex(0x505050), LV_PART_MAIN);
	lv_obj_set_style_border_width(aplist_title,0,LV_PART_MAIN);
	lv_obj_add_state(aplist_title,LV_STATE_DISABLED);

	label = lv_label_create(aplist_title);
	lv_label_set_recolor(label, true);
	snprintf(buf,255, "#000000  Factory Setting#");
	lv_obj_set_style_text_color(label,lv_color_hex(0x000000),0);
	set_label_text2(label,STR_FACTORY_SETTING,FONT_MID);
	// lv_label_set_text(label, buf);
	lv_obj_set_align(label,LV_ALIGN_CENTER);

	/*Create a container with COLUMN flex direction*/
	cont_col = lv_obj_create(p);
	// lv_obj_set_pos(cont_col, 0,139);//560, 500);
	lv_obj_align_to(cont_col,aplist_title,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
	lv_obj_set_size(cont_col, LV_PCT(35), LV_PCT(70));
	lv_obj_set_style_radius(cont_col, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(cont_col, lv_color_hex(0x292929), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(cont_col, UI_OPA_PERCENT, LV_PART_MAIN);

	//lv_obj_align_to(cont_col, cont_row, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
	lv_obj_set_flex_flow(cont_col, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_align(cont_col, LV_ALIGN_TOP_LEFT);
	lv_obj_set_style_border_width(cont_col, 0, LV_PART_MAIN);

	uint32_t i;
	for (i = 0; i < MAX_FC_ITEM_NUM; i++) {
		/*Add items to the column*/
		obj = lv_btn_create(cont_col);
		lv_obj_clear_flag(obj,LV_OBJ_FLAG_CLICK_FOCUSABLE);
		lv_obj_set_size(obj, LV_PCT(100), 40);
		lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
		lv_obj_set_style_shadow_color(obj, lv_color_hex(0xffff00), LV_PART_MAIN);
		lv_obj_set_style_shadow_width(obj,0,LV_PART_MAIN);
		lv_obj_set_style_bg_color(obj, lv_color_hex(0x292929), LV_PART_MAIN);
		lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);

		lv_obj_set_style_border_side(obj,LV_BORDER_SIDE_FULL,0);
		lv_obj_set_style_border_width(obj,0,0);
		lv_group_add_obj(facty_g, obj);
		lv_obj_add_event_cb(obj,ft_event_handler,LV_EVENT_ALL, facty_g);

		// Items
		label = lv_label_create(obj);
		lv_obj_set_align(label,LV_ALIGN_LEFT_MID);
		lv_obj_set_size(label, LV_PCT(80),LV_SIZE_CONTENT);
		lv_obj_set_style_text_align(obj,LV_TEXT_ALIGN_LEFT,0);

		lv_label_set_recolor(label, true);
		set_label_text2(label, fc_item_label[i],FONT_MID);
		if(i==ID_ENABLE_PQ || i==ID_DYN_CONTRAST){
			int pq_val=factory_get_some_sys_param(P_PQ_ONFF);
			int dyn_val=factory_get_some_sys_param(P_DYN_CONTRAST_ONOFF);
			lv_obj_t * sw;
			sw = lv_switch_create(obj);
			/*some obj will add itself into group when create*/ 
			lv_group_remove_obj(sw);
			lv_obj_set_align(sw,LV_ALIGN_RIGHT_MID);
			lv_obj_set_size(sw,50,30);
			if((i==ID_ENABLE_PQ&&pq_val==1)||(i==ID_DYN_CONTRAST&&dyn_val==1))
				lv_obj_add_state(sw, LV_STATE_CHECKED);
		}else if(i== ID_CVBS){
			int aef_val = 128;
		#ifdef CVBSIN_TRAINING_SUPPORT
			aef_val=cvbs_training_value_get();
		#endif
			sprintf(cvbs_num, "%d", aef_val);
			//itoa(aef_val, map[1], 10);
			//sprintf(cvbs_mmap[1], "%d", x);
			cvbs_btnm = lv_btnmatrix_create(obj);
			lv_group_remove_obj(cvbs_btnm);
			lv_obj_set_size(cvbs_btnm,100,35);
			lv_obj_set_align(cvbs_btnm,LV_ALIGN_RIGHT_MID);
			lv_btnmatrix_set_map(cvbs_btnm, cvbs_mmap);
			REG32_SET_FIELD2(0xb8800180,8,8,aef_val);
		}
	}


}

static void factory_settings_scr_control(void *arg1, void *arg2)
{
	(void)arg2;
     control_msg_t *ctl_msg = (control_msg_t*)arg1;
	 switch(ctl_msg->msg_type){
		case MSG_TYPE_CVBS_TRAINING_FINISH:{
			int aef_val = 128;
		#ifdef CVBSIN_TRAINING_SUPPORT
			aef_val=cvbs_training_value_get();
		#endif
			sprintf(cvbs_num, "%d", aef_val);
			lv_btnmatrix_set_map(cvbs_btnm, cvbs_mmap);		
		}

			break;
		default:
			break;
	 }
}

void ui_factory_settings_init(void)
{

	/*Create a container */
	factory_settings_scr = lv_obj_create(NULL);
	//lv_obj_set_style_bg_opa(factory_settings_scr ,LV_OPA_60, LV_PART_MAIN);
	printf("fs_scr: (%d,%d), (%d, %d)\n", lv_obj_get_x(factory_settings_scr),lv_obj_get_y(factory_settings_scr)
						, lv_obj_get_width(factory_settings_scr), lv_obj_get_height(factory_settings_scr));
	//lv_obj_set_pos(factory_settings_scr, 80, 200);
	lv_obj_set_size(factory_settings_scr, LV_PCT(38), LV_PCT(70));
	printf("fs_scr: (%d,%d), (%d, %d)\n", lv_obj_get_x(factory_settings_scr),lv_obj_get_y(factory_settings_scr)
						, lv_obj_get_width(factory_settings_scr), lv_obj_get_height(factory_settings_scr));
	lv_obj_set_style_radius(factory_settings_scr, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(factory_settings_scr, LV_OPA_TRANSP, LV_PART_MAIN);
	lv_obj_set_style_border_width(factory_settings_scr, 0, LV_PART_MAIN);
	lv_obj_set_align(factory_settings_scr, LV_ALIGN_BOTTOM_LEFT);
	
	lv_obj_add_event_cb(factory_settings_scr,ft_event_handler,LV_EVENT_ALL, facty_g);

    screen_entry_t factory_test_entry;
    factory_test_entry.screen = factory_settings_scr;
    factory_test_entry.control = factory_settings_scr_control;
    api_screen_regist_ctrl_handle(&factory_test_entry);

}
#endif


void factory_test_enter_check(uint16_t key)
{
	static uint8_t cur_status = 0;
	switch(cur_status){
		case 0:
			if(key == LV_KEY_UP)
				cur_status = 1;
			break;
		case 1:
			if(key == LV_KEY_DOWN)
				cur_status = 2;
			else
				cur_status = 0;
			break;
		case 2:
			if(key == LV_KEY_LEFT)
				cur_status = 3;
			else
				cur_status = 0;
			break;
		case 3:
			cur_status = 0;
			if(key == LV_KEY_RIGHT){
				printf("\t\tEnter factory test mode...\n");	
				change_screen(SCREEN_FACTORY_TEST);//_ui_screen_change(factSet_list,0,0);
			}
			break;
		default:cur_status = 0;
			printf("reset state...\n");
			break;
	}
}

#endif

