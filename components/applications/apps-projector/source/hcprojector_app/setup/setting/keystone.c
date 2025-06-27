#include "app_config.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <hcuapi/dis.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/input-event-codes.h>
#include <stdlib.h>
#include <unistd.h>
#include "hcuapi/fb.h"
#include "setup.h"
#include "screen.h"
#include "factory_setting.h"
#include "../../mul_lang_text.h"
#include "../../app_config.h"
#include "com_api.h"
#include "osd_com.h"
#include "lv_drivers/display/fbdev.h"

#ifdef PROJECTOR_VMOTOR_SUPPORT
 #define KEYSTONE_ADJUST_BAR_LEN 33 
 #else 
  #define KEYSTONE_ADJUST_BAR_LEN 11
 #endif

#ifdef LVGL_RESOLUTION_240P_SUPPORT
    #define KEYSTONE_FONT lv_font_montserrat_10
    #define KEYSTONE_LEFT_WIDTH_PCT LV_SIZE_CONTENT
#else
    #define KEYSTONE_FONT lv_font_montserrat_40
    #define KEYSTONE_LEFT_WIDTH_PCT 23
#endif

#define KEYSTONE_TOP          0
#define KEYSTONE_BOTTOM  1
#define KEYSTONE_STEP    8 // adjust step of display area_width
#define KEYSTONE_MIN_WIDTH   (DIS_SOURCE_FULL_W*1/4)//400



lv_obj_t* keystone_scr;
//lv_group_t * keysone_g;
lv_obj_t* keystone_list;
#ifdef AUTOKEYSTONE_SWITCH
lv_obj_t *autokeystone_label_state = NULL;
#endif

static int m_fd_dis = -1;
static int16_t  width_top = 0, width_bottom = 0,max_width = 0;

extern lv_timer_t *timer_setting;
extern lv_obj_t *slave_scr_obj;
extern lv_obj_t *tab_btns;
extern uint16_t tabv_act_id;
extern SCREEN_TYPE_E cur_scr;
static bool keystone_is_forbid = false;

#ifdef NEW_SETUP_ITEM_CTRL
int keystone_set(int dir, uint16_t step);
#else
static int keystone_set(int dir, uint16_t step);
#endif
static void keystone_stop(void);

#ifdef NEW_SETUP_ITEM_CTRL  //ZHP
void keystone_init(void)
#else
static void keystone_init(void)
#endif
{	
	struct dis_screen_info dis_info;
	struct dis_keystone_param vhance;
	memset(&vhance, 0, sizeof(vhance));
	if(m_fd_dis < 0) {
    	m_fd_dis = open("/dev/dis" , O_RDWR);
    	if(m_fd_dis < 0) {
    		return ;
    	}
    }

    dis_info.distype = DIS_TYPE_HD;
    if(ioctl(m_fd_dis, DIS_GET_SCREEN_INFO, &dis_info)!=0){
    	perror("ioctl(get screeninfo)");
    }
    
    max_width = dis_info.area.w;
    width_bottom = width_top = dis_info.area.w;  
	if(0== projector_get_some_sys_param(P_KEYSTONE_TOP) || 0xff == projector_get_some_sys_param(P_KEYSTONE_TOP)){
        projector_set_some_sys_param(P_KEYSTONE_TOP, width_top);
        projector_set_some_sys_param(P_KEYSTONE_BOTTOM, width_bottom);
        projector_sys_param_save();
	}

}


#ifdef KEYSTONE_STRETCH_SUPPORT

#define MAX_KEYSTONE_WIDTH   1920
#define MAX_KEYSTONE_HEIGHT  1080

#if 0
//manual caculate the keystone height ratio by top_with and bottom_width
//the return ratio is 0 < ratio <= 100. <=0 is fail
static int m_height_ratio = -1;
static int keystone_height_ratio_cal(int top_w, int bottom_w)
{
    int ratio = -1;
    do {
        if (top_w == bottom_w){
            ratio = 100;
            break;
        }
        else if ((top_w < MAX_KEYSTONE_WIDTH && bottom_w < MAX_KEYSTONE_WIDTH) || 
                (top_w > MAX_KEYSTONE_WIDTH) || 
                (bottom_w > MAX_KEYSTONE_WIDTH)){
            ratio = -1;
            break;
        }

        if (top_w < MAX_KEYSTONE_WIDTH)
            ratio = (top_w * 100 / MAX_KEYSTONE_WIDTH);
        else if (bottom_w < MAX_KEYSTONE_WIDTH)
            ratio = (bottom_w * 100 / MAX_KEYSTONE_WIDTH);
        else 
            ratio = -1;
    } while (0);

    m_height_ratio = ratio;
    return ratio;
}

static int keystone_height_ratio_get(void)
{
    return m_height_ratio;
}

static int keystone_dis_height_get(int ori_height)
{
    //The customer can return/set the keystone height themselves
    if (-1 == keystone_height_ratio_get())
        return ori_height;

    return ori_height*keystone_height_ratio_get() / 100;
}

static int keystone_fb_height_get(int ori_height)
{
    //The customer can return/set the keystone height themselves
    if (-1 == keystone_height_ratio_get())
        return ori_height;

    return ori_height*keystone_height_ratio_get() / 100;
}

//Input the x,y,w,h of keystone, calculate and output the position and height(shorten) 
// of keystone, so that avoid the output image being draw high.
static int keystone_rect_transform(int *x, int *y, int *w, int *h)
{
    (void)x;
    (void)w;
    dis_screen_info_t screen_info = {0};

    api_get_screen_info(&screen_info);
    *h = keystone_fb_height_get(*h);
    *y = (screen_info.area.h - *h)/2;

    return 0;
}
#endif

//Input the x,y,w,h of keystone, calculate and output the position and height(shorten) 
// of keystone, so that avoid the output image being draw high.
static int keystone_rect_transform(int *x, int *y, int *w, int *h)
{
    struct dis_keystone_rect keystone_rect = { 0 };
    if (m_fd_dis < 0) {
        m_fd_dis = open("/dev/dis" , O_RDWR);
        if (m_fd_dis < 0) {
            return -1;
        }
    }

    keystone_rect.distype = DIS_TYPE_HD;
    keystone_rect.ori_area.x = *x;
    keystone_rect.ori_area.y = *y;
    keystone_rect.ori_area.w = *w;
    keystone_rect.ori_area.h = *h;

    printf("%s(), before: %d %d %d %d\n", __func__, *x, *y, *w, *h);

    //Input the x,y,w,h of keystone, calculate the position and height(shorten) of keystone, so that
    //avoid the output image being draw high.
    if (ioctl(m_fd_dis , DIS_GET_DST_RECT_OF_KEYSTONE , &keystone_rect) != 0) {
        perror("ioctl(get key stone rect)");
        return -1;
    }

    *x = keystone_rect.dst_area.x;
    *y = keystone_rect.dst_area.y;
    *w = keystone_rect.dst_area.w;
    *h = keystone_rect.dst_area.h;

    printf("%s(), after: %d %d %d %d\n", __func__, *x, *y, *w, *h);

    return 0;
}

void keystone_fb_viewport_scale(void)
{
    int fd = open(DEV_FB , O_RDWR);
    if (fd < 0) {
        printf("%s(), line:%d. open device: %s error!\n", 
            __func__, __LINE__, DEV_FB);
        return;
    }

    int x;
    int y;
    int w;
    int h;

    x = get_cur_osd_x();
    y = get_cur_osd_y();
    w = get_cur_osd_h();
    h = get_cur_osd_v();

    keystone_rect_transform(&x, &y, &w, &h);

    hcfb_viewport_t viewport = { 0 };
    hcfb_scale_t fb_scale;  
    dis_screen_info_t screen_info = {0};

    viewport.x = x;
    viewport.y = y;
    viewport.width = w;
    viewport.height = h;
    viewport.apply_now = true;
    viewport.enable = true;

#ifdef CONFIG_LV_HC_KEYSTONE_EQUAL_SCALE
    //support anti-aliasing of bevel edge while keystone enable, but should
    //add more one screen frame buffer for fb (extra-buffer-size).
    //and the xres/yres for fb0 DTS must be screen really size.
    fbdev_set_viewport(&viewport);
#else
    api_get_screen_info(&screen_info);
    fb_scale.h_div = screen_info.area.w > screen_info.area.h ?  OSD_MAX_WIDTH : OSD_MAX_HEIGHT; 
    fb_scale.v_div = screen_info.area.w > screen_info.area.h ?  OSD_MAX_HEIGHT : OSD_MAX_WIDTH; 
    fb_scale.h_mul = w;
    fb_scale.v_mul = h;
    ioctl(fd, HCFBIOSET_SCALE, &fb_scale); 
    ioctl(fd, HCFBIOSET_SET_VIEWPORT, &viewport);
#endif

    printf("viewport: %d %d %d %d\n", 
        viewport.x, viewport.y, viewport.width, viewport.height);

    close(fd);
}

#endif //endif of KEYSTONE_STRETCH_SUPPORT


//Set the mininal threshold width of keystone to avoid
//the OSD/fb to be distorted.
#define KEYSTONE_WIDTH_MIN_PER  60
void set_keystone(int top_w, int bottom_w)
{
    if(m_fd_dis == -1)
    {
    	m_fd_dis = open("/dev/dis" , O_RDWR);
    	if(m_fd_dis < 0) {
    		return;
    	}
	}

    if (0 == max_width) {
        struct dis_screen_info dis_info = { 0, };
        dis_info.distype = DIS_TYPE_HD;
        if (!ioctl(m_fd_dis, DIS_GET_SCREEN_INFO, &dis_info)) 
            max_width = dis_info.area.w;
    }
    if (max_width) {
		int width_min = max_width*KEYSTONE_WIDTH_MIN_PER/100;
        if(top_w < width_min || bottom_w < width_min){
            return;
        }
    }

    struct dis_keystone_param vhance;
    memset(&vhance, 0, sizeof(vhance));
    vhance.distype = DIS_TYPE_HD;
    vhance.info.enable = 1;
    vhance.info.bg_enable = 0;
    vhance.info.width_up = top_w;
    vhance.info.width_down = bottom_w;

#ifdef KEYSTONE_STRETCH_SUPPORT
    static int top_w_bak = -1;
    static int bottom_w_bak = -1;

  #if 0
    //manual set the height of dis keystone
    int height_ratio = keystone_height_ratio_cal(top_w, bottom_w);
    if (height_ratio > 0){
        vhance.info.height = keystone_dis_height_get(MAX_KEYSTONE_HEIGHT);
        vhance.info.offset = (MAX_KEYSTONE_HEIGHT-vhance.info.height)/2;
    }

  #endif

    int set_dis_first = 1;
    if (top_w_bak != -1){
        if (top_w > top_w_bak || bottom_w > bottom_w_bak)
            set_dis_first = 0;
        else
            set_dis_first = 1;
    }

    if (set_dis_first){
        ioctl(m_fd_dis , DIS_SET_KEYSTONE_PARAM , &vhance);
        keystone_fb_viewport_scale();    
    } else {
        //Our UI is full screen, when enlarge keystone, we should enlarge fb keystone first
        // to mask the video edge, because video keytone is also enlarging.
        
        //DE keystone do not take effect, only calcute the keystone parameters for
        //fb/OSD, fb set the ketstone first, then DE keysonte take effcet.
        vhance.info.enable = 0;
        ioctl(m_fd_dis , DIS_SET_KEYSTONE_PARAM , &vhance);
        keystone_fb_viewport_scale();   
        lv_refr_now(lv_disp_get_default()); 

        //DE set keystone, take effect at once.
        vhance.info.enable = 1;
        ioctl(m_fd_dis , DIS_SET_KEYSTONE_PARAM , &vhance);

    }
    top_w_bak = top_w;
    bottom_w_bak = bottom_w;
#else
    ioctl(m_fd_dis , DIS_SET_KEYSTONE_PARAM , &vhance);
#endif    


    projector_set_some_sys_param(P_KEYSTONE_TOP, top_w);
    projector_set_some_sys_param(P_KEYSTONE_BOTTOM, bottom_w);
    //projector_sys_param_save();
    printf("%s(), top width:  %d, bottom width: %d\n", __func__, 
        (int)vhance.info.width_up, (int)vhance.info.width_down);
}
#ifdef NEW_SETUP_ITEM_CTRL
int keystone_set(int dir, uint16_t step)
#else
static int keystone_set(int dir, uint16_t step)
#endif
{
    width_top = projector_get_some_sys_param(P_KEYSTONE_TOP);
    width_bottom = projector_get_some_sys_param(P_KEYSTONE_BOTTOM);
    max_width = width_top > width_bottom ? width_top : width_bottom;

    if(dir == KEYSTONE_TOP)
    {
        if(width_bottom  < max_width)
        {
            width_bottom  += step;
            width_bottom = width_bottom>max_width?max_width:width_bottom;
            width_top = max_width;
        }
        else
        {
            width_top -= step;  
            if(width_top < KEYSTONE_MIN_WIDTH)
                width_top  = KEYSTONE_MIN_WIDTH;
            width_bottom = max_width;
        }
    }
    else if(dir == KEYSTONE_BOTTOM)
    {
        if(width_top  < max_width)
        {
            width_top  += step;
            width_top = width_top>max_width?max_width:width_top;
            width_bottom = max_width;
        }
        else
        {
            width_bottom -= step;                  
            if(width_bottom < KEYSTONE_MIN_WIDTH)
                width_bottom = KEYSTONE_MIN_WIDTH;
            width_top = max_width;
        }
    }

    set_keystone(width_top, width_bottom);

	return 0;
}
static void keystone_stop(void)
{
    if(m_fd_dis >0)
    {
        max_width = 0;
        width_top = 0;
        width_bottom = 0;
        close(m_fd_dis);
        m_fd_dis = -1;
    }
}

static int sel_id = 0;
#ifdef AUTOKEYSTONE_SWITCH
extern void create_atk_cpage();
static void event_handle(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    //lv_obj_t *btn = lv_event_get_user_data(e);

    if(code == LV_EVENT_PRESSED) {
        if(sel_id == 0) //autokeystone
        {
            if (projector_get_some_sys_param(P_AUTO_KEYSTONE) == 1)
            {
                lv_obj_clear_flag(lv_obj_get_child(keystone_list, 1), LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(lv_obj_get_child(keystone_list, 2), LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(lv_obj_get_child(keystone_list, 3), LV_OBJ_FLAG_HIDDEN);
                //lv_obj_clear_state(lv_obj_get_child(keystone_list, 1), LV_STATE_DISABLED);
                //lv_obj_clear_state(lv_obj_get_child(keystone_list, 2), LV_STATE_DISABLED);
                //lv_obj_clear_state(lv_obj_get_child(keystone_list, 3), LV_STATE_DISABLED);
                set_label_text2(autokeystone_label_state, STR_OFF, FONT_NORMAL);
                projector_set_some_sys_param(P_AUTO_KEYSTONE,0);
#ifdef ATK_CALIBRATION
    lv_obj_add_flag(lv_obj_get_child(keystone_list, 4), LV_OBJ_FLAG_HIDDEN);
#endif
            }else {
                lv_obj_add_flag(lv_obj_get_child(keystone_list, 1), LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(lv_obj_get_child(keystone_list, 2), LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(lv_obj_get_child(keystone_list, 3), LV_OBJ_FLAG_HIDDEN);
                //lv_obj_add_state(lv_obj_get_child(keystone_list, 1), LV_STATE_DISABLED);
                //lv_obj_add_state(lv_obj_get_child(keystone_list, 2), LV_STATE_DISABLED);
                //lv_obj_add_state(lv_obj_get_child(keystone_list, 3), LV_STATE_DISABLED);
                set_label_text2(autokeystone_label_state, STR_ON, FONT_NORMAL);
                projector_set_some_sys_param(P_AUTO_KEYSTONE,1);
#ifdef ATK_CALIBRATION
    lv_obj_clear_flag(lv_obj_get_child(keystone_list, 4), LV_OBJ_FLAG_HIDDEN);
#endif

            }
  
        }
        if(sel_id == 1) //up
        {
            keystone_set(KEYSTONE_TOP, KEYSTONE_STEP);
        }
        else if(sel_id==2){// reset keystone
            set_keystone(max_width, max_width);
            width_top = width_bottom = max_width;
        }
        else if(sel_id==3)
        {
            keystone_set(KEYSTONE_BOTTOM, KEYSTONE_STEP);
        }
#ifdef ATK_CALIBRATION
        else if(sel_id==4) {
            create_atk_cpage();
        }
#endif
		lv_group_focus_obj(lv_group_get_focused(lv_group_get_default()));
    }
    // if(code == LV_EVENT_DRAW_PART_BEGIN)
    // {
    //         lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    //         lv_btnmatrix_t* btnm = (lv_btnmatrix_t*)keystone_list;
    //         if(dsc->class_p == &lv_btnmatrix_class && dsc->type == LV_BTNMATRIX_DRAW_PART_BTN) {
    //             if(dsc->id == btnm->btn_id_sel) {
    //                    if(lv_btnmatrix_get_selected_btn(obj) == dsc->id) 
    //                         dsc->rect_dsc->bg_color = lv_palette_darken(LV_PALETTE_BLUE, 3);
    //                     else 
    //                         dsc->rect_dsc->bg_color = lv_palette_main(LV_PALETTE_BLUE);
    //             }
    //         }

    // }
    else if(code == LV_EVENT_KEY){
        if(timer_setting)
            lv_timer_pause(timer_setting);
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_HOME){
           
            turn_to_main_scr();
            return;
            
        }else if(key == LV_KEY_ESC){
        
            turn_to_main_scr();
        
            return;
        }else if(key == LV_KEY_UP || key == LV_KEY_LEFT){
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            sel_id -= 1;
            if(sel_id<0){
                lv_group_focus_obj(tab_btns);
                return;
            }
#ifdef ATK_CALIBRATION
            if (projector_get_some_sys_param(P_AUTO_KEYSTONE) == 1 && sel_id == 3){
                sel_id = 0;
            }
#endif
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
        }else if(key == LV_KEY_DOWN || key == LV_KEY_RIGHT){
    #ifndef ATK_CALIBRATION
            if (projector_get_some_sys_param(P_AUTO_KEYSTONE) == 1){
                lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
                lv_group_focus_next(lv_group_get_default());
                return;
            }
    #endif

            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            sel_id += 1;
#ifdef ATK_CALIBRATION
            if (projector_get_some_sys_param(P_AUTO_KEYSTONE) == 0 && sel_id == 4){
                sel_id++;
            }
#endif
            if(sel_id>=lv_obj_get_child_cnt(target)){
                lv_group_focus_next(lv_group_get_default());
                return;
            }
#ifdef ATK_CALIBRATION
            if (projector_get_some_sys_param(P_AUTO_KEYSTONE) == 1 && sel_id == 1){
                sel_id = 4;
            }
#endif
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);           
        }
    }else if(code == LV_EVENT_FOCUSED){
         if(act_key_code == KEY_UP){
            if (projector_get_some_sys_param(P_AUTO_KEYSTONE) == 1)
            #ifdef ATK_CALIBRATION
                sel_id = 4;
            #else
                sel_id = 0;
            #endif
            else
        #ifdef ATK_CALIBRATION
            sel_id = lv_obj_get_child_cnt(target)-2;
        #else
            sel_id = lv_obj_get_child_cnt(target)-1;
        #endif
        }else if(act_key_code == KEY_DOWN){
            sel_id = 0;
        }
        if(sel_id>-1)
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
    }else if(code == LV_EVENT_DEFOCUSED){
        
        if(sel_id<lv_obj_get_child_cnt(target))
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
        sel_id = 0;
    }
}
#else
static void event_handle(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    //lv_obj_t *btn = lv_event_get_user_data(e);
    if(code == LV_EVENT_PRESSED) {
        if(sel_id == 0) //up
        {
            keystone_set(KEYSTONE_TOP, KEYSTONE_STEP);
        }
        else if(sel_id==1){// reset keystone
            set_keystone(max_width, max_width);
            width_top = width_bottom = max_width;
        }
        else if(sel_id==2)
        {
            keystone_set(KEYSTONE_BOTTOM, KEYSTONE_STEP);
        }
		lv_group_focus_obj(lv_group_get_focused(lv_group_get_default()));
    }
    // if(code == LV_EVENT_DRAW_PART_BEGIN)
    // {
    //         lv_obj_draw_part_dsc_t * dsc = lv_event_get_draw_part_dsc(e);
    //         lv_btnmatrix_t* btnm = (lv_btnmatrix_t*)keystone_list;
    //         if(dsc->class_p == &lv_btnmatrix_class && dsc->type == LV_BTNMATRIX_DRAW_PART_BTN) {
    //             if(dsc->id == btnm->btn_id_sel) {
    //                    if(lv_btnmatrix_get_selected_btn(obj) == dsc->id) 
    //                         dsc->rect_dsc->bg_color = lv_palette_darken(LV_PALETTE_BLUE, 3);
    //                     else 
    //                         dsc->rect_dsc->bg_color = lv_palette_main(LV_PALETTE_BLUE);
    //             }
    //         }

    // }
    else if(code == LV_EVENT_KEY){
        if(timer_setting)
            lv_timer_pause(timer_setting);
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_HOME){
           
            turn_to_main_scr();
            return;
            
        }else if(key == LV_KEY_ESC){
          
            turn_to_main_scr();
        
            return;
        }else if(key == LV_KEY_UP || key == LV_KEY_LEFT){
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            sel_id -= 1;
            if(sel_id<0){
                lv_group_focus_obj(tab_btns);
                return;
            }
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
        }else if(key == LV_KEY_DOWN || key == LV_KEY_RIGHT){
             lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
            sel_id += 1;
            if(sel_id>=lv_obj_get_child_cnt(target)){
                lv_group_focus_next(lv_group_get_default());
                return;
            }
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);           
        }
    }else if(code == LV_EVENT_FOCUSED){
         if(act_key_code == KEY_UP){
            sel_id = lv_obj_get_child_cnt(target)-1;
        }else if(act_key_code == KEY_DOWN){
            sel_id = 0;
        }
        if(sel_id>-1)
            lv_obj_add_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
    }else if(code == LV_EVENT_DEFOCUSED){
        
        if(sel_id<lv_obj_get_child_cnt(target))
            lv_obj_clear_state(lv_obj_get_child(target, sel_id), LV_STATE_CHECKED);
        sel_id = 0;
    }
}
#endif

void keystone_adjust(void)
{   
    struct dis_screen_info dis_info;
    struct dis_keystone_param vhance;
    memset(&vhance, 0, sizeof(vhance));
    uint16_t keystone_top;
    uint16_t keystone_bottom;
    int fd_dis = -1;

    if (0 == max_width)
        return;

    dis_info.distype = DIS_TYPE_HD;
    if (m_fd_dis < 0){
        fd_dis = open("/dev/dis" , O_RDWR);
        if(fd_dis < 0) {
            return ;
        }
        ioctl(fd_dis, DIS_GET_SCREEN_INFO, &dis_info);
    } else {
        ioctl(m_fd_dis, DIS_GET_SCREEN_INFO, &dis_info);
        close(fd_dis);
    }

    keystone_top = projector_get_some_sys_param(P_KEYSTONE_TOP);
    keystone_bottom = projector_get_some_sys_param(P_KEYSTONE_BOTTOM);
    if (keystone_top == dis_info.area.w || keystone_bottom == dis_info.area.w)
    {
        //Not change screen parameter and Not set keystone. do not need set keystone
        printf("%s(), line:%d. same keystone width:%d, not need set!\n", 
            __func__, __LINE__, dis_info.area.w);
        return;
    }

    if (keystone_top != dis_info.area.w)
        keystone_top = keystone_top*dis_info.area.w/max_width;

    if (keystone_bottom != dis_info.area.w)
        keystone_bottom = keystone_bottom*dis_info.area.w/max_width;

    max_width = dis_info.area.w;
    set_keystone(keystone_top, keystone_bottom);
}

void set_keystone_disable(bool en){
    keystone_is_forbid = en;
}

void keystone_screen_init(lv_obj_t *parent)
{
#if 0//def PROJECTOR_VMOTOR_SUPPORT
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_set_size(obj, LV_PCT(85), LV_PCT(40));
    lv_obj_set_style_pad_left(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, 0);

    lv_obj_t *label = lv_label_create(obj);
    lv_obj_set_size(label, LV_PCT(KEYSTONE_LEFT_WIDTH_PCT), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(label, 0, 0);
    // language_choose_add_label1(label,);
    // set_label_text1(label, projector_get_some_sys_param(P_OSD_LANGUAGE), NULL);
    set_label_text2(label,  STR_KEYSTONE, FONT_NORMAL);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);


    keystone_list = create_list_obj1(obj, 77, 100);
#else
    keystone_list = create_list_obj1(parent, 77, 120);
    lv_obj_t *label;
#endif

#ifdef AUTOKEYSTONE_SWITCH
    create_list_sub_text_obj4(keystone_list, 100,  KEYSTONE_ADJUST_BAR_LEN, STR_ATKEYSTONE);
    lv_obj_set_align(lv_obj_get_child(lv_obj_get_child(keystone_list,0),0),LV_ALIGN_LEFT_MID);
    autokeystone_label_state = lv_label_create(lv_obj_get_child(keystone_list,0));
    lv_obj_set_align(autokeystone_label_state, LV_ALIGN_RIGHT_MID);
    if( projector_get_some_sys_param(P_AUTO_KEYSTONE) == 1)
        set_label_text2(autokeystone_label_state, STR_ON, FONT_NORMAL);
    else
        set_label_text2(autokeystone_label_state, STR_OFF, FONT_NORMAL);
    
    //lv_obj_set_style_text_color(autokeystone_label_state,lv_color_hex(0xffffff),0);
#endif
    create_list_sub_text_obj3(keystone_list,100, KEYSTONE_ADJUST_BAR_LEN, "+");
    lv_obj_set_style_text_font(lv_obj_get_child(keystone_list, 0), &KEYSTONE_FONT,0);
    label = lv_label_create(lv_obj_get_child(keystone_list, 0));
    
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //lv_obj_set_style_border_width(label, 2, 0);
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, "<");
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    label = lv_label_create(lv_obj_get_child(keystone_list, 0));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, ">");
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);



    create_list_sub_text_obj4(keystone_list, 100,  KEYSTONE_ADJUST_BAR_LEN, STR_RESET);
    //lv_obj_set_style_text_font(lv_obj_get_child(keystone_list, 1), &lv_font_montserrat_40,0);

    label = lv_label_create(lv_obj_get_child(keystone_list, 1));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, "<");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    label = lv_label_create(lv_obj_get_child(keystone_list, 1));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, ">");
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);



    create_list_sub_text_obj3(keystone_list,100,KEYSTONE_ADJUST_BAR_LEN , "-");
    lv_obj_set_style_text_font(lv_obj_get_child(keystone_list, 2), &KEYSTONE_FONT,0);

    label = lv_label_create(lv_obj_get_child(keystone_list, 2));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, "<");
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);

    label = lv_label_create(lv_obj_get_child(keystone_list, 2));
    lv_obj_set_size(label, LV_PCT(15), LV_PCT(100));
    //set_pad_and_border_and_outline(label);
    lv_obj_set_style_bg_opa(label, LV_OPA_0, 0);
    lv_label_set_text(label, ">");
    lv_obj_set_style_text_font(label, &KEYSTONE_FONT,0);
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_font(lv_obj_get_child(keystone_list, 2), &KEYSTONE_FONT,0);


    #ifdef ATK_CALIBRATION
    create_list_sub_text_obj4(keystone_list, 100,  KEYSTONE_ADJUST_BAR_LEN,STR_HC);
    lv_obj_set_align(lv_obj_get_child(lv_obj_get_child(keystone_list,4),0),LV_ALIGN_LEFT_MID);
    lv_obj_set_style_text_color(lv_obj_get_child(keystone_list, 4),lv_color_hex(0xffffff),0);
    //lv_obj_set_style_text_color(lv_obj_get_child(lv_obj_get_child(keystone_list,4),0),lv_color_hex(0xffffff),0);
    //lv_obj_add_flag(lv_obj_get_child(keystone_list, 4), LV_OBJ_FLAG_HIDDEN);
    #endif



#ifdef AUTOKEYSTONE_SWITCH
    lv_obj_set_style_text_color(lv_obj_get_child(keystone_list, 0),lv_color_hex(0xffffff),0);
    lv_obj_set_style_text_color(lv_obj_get_child(keystone_list, 1),lv_color_hex(0xffffff),0);
    lv_obj_set_style_text_color(lv_obj_get_child(keystone_list, 2),lv_color_hex(0xffffff),0);
    lv_obj_set_style_text_color(lv_obj_get_child(keystone_list, 3),lv_color_hex(0xffffff),0);

    if( projector_get_some_sys_param(P_AUTO_KEYSTONE) == 1){
        lv_obj_add_flag(lv_obj_get_child(keystone_list, 1), LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lv_obj_get_child(keystone_list, 2), LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lv_obj_get_child(keystone_list, 3), LV_OBJ_FLAG_HIDDEN);
        //lv_obj_add_state(lv_obj_get_child(keystone_list, 1), LV_STATE_DISABLED);
        //lv_obj_add_state(lv_obj_get_child(keystone_list, 2), LV_STATE_DISABLED);
        //lv_obj_add_state(lv_obj_get_child(keystone_list, 3), LV_STATE_DISABLED);
    #ifdef ATK_CALIBRATION
        lv_obj_clear_flag(lv_obj_get_child(keystone_list, 4), LV_OBJ_FLAG_HIDDEN);
    #endif
    }else{
        lv_obj_clear_flag(lv_obj_get_child(keystone_list, 1), LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(lv_obj_get_child(keystone_list, 2), LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(lv_obj_get_child(keystone_list, 3), LV_OBJ_FLAG_HIDDEN);
        //lv_obj_clear_state(lv_obj_get_child(keystone_list, 1), LV_STATE_DISABLED);
        //lv_obj_clear_state(lv_obj_get_child(keystone_list, 2), LV_STATE_DISABLED);
        //lv_obj_clear_state(lv_obj_get_child(keystone_list, 3), LV_STATE_DISABLED);
    #ifdef ATK_CALIBRATION
        lv_obj_add_flag(lv_obj_get_child(keystone_list, 4), LV_OBJ_FLAG_HIDDEN);
    #endif
    }
#endif


    lv_obj_add_event_cb(keystone_list, event_handle, LV_EVENT_ALL, NULL);

	keystone_init();
}

void change_keystone(void){
#ifdef KEYSTONE_SUPPORT
     tabv_act_id = TAB_KEYSTONE;
#endif
    change_screen(SCREEN_SETUP);
}
