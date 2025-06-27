#include "app_config.h"
#include "osd_com.h"
#include "com_api.h"
#ifdef SYS_ZOOM_SUPPORT

#include "setup.h"
#include "../../channel/local_mp/mp_fspage.h"
#include "../../channel/local_mp/media_player.h"
#include "../../channel/local_mp/win_media_list.h"
#include "../../channel/local_mp/mp_playlist.h"
#include "../../channel/local_mp/mp_zoom.h"
#include "../../channel/local_mp/mp_ctrlbarpage.h"
#include "../../channel/cvbs_in/cvbs_rx.h"
#include "../../channel/hdmi_in/hdmi_rx.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/fb.h>
#include <hcuapi/viddec.h>
// #ifdef __linux__
    
// #else
//      #include <kernel/fb.h>
// #endif

#include "factory_setting.h"
#include "screen.h"


#define ZOOM_OUT_COUNT_MAX 50
#define SYS_SCALE_MIN_RATIO 3
#define DIS_ZOOM_FULL_W 1920
#define DIS_ZOOM_FULL_H 1080

// static hcfb_scale_t scale_param = { OSD_MAX_WIDTH, OSD_MAX_HEIGHT, 1920, 1080 };
// static hcfb_lefttop_pos_t start_pos ={0};
static dis_tv_mode_e zoom_mode = DIS_TV_16_9;//16:9或4:3
static dis_tv_mode_e dis_mode = DIS_TV_AUTO;//16:9或4:3或auto
static dis_screen_info_t lcd_area={0};
//static int scale_max_hor = 0;
static double ui_adjust_w_v = 0;
static double ui_adjust_h_v = 0;
static int zoom_out_count = 0;
extern lv_timer_t *timer_setting;
static bool is_vertical_screen = false;
static mainlayer_scale_t mainlayer_scale_type = MAINLAYER_SCALE_TYPE_NORMAL;
static bool has_zoom_operation = false;

static int get_display_h_();
static int get_display_w_();
static void get_scr_h_v_by_ratio(int *w, int *h, int ratio);

typedef struct scale_param_{
    int x;
    int y;
    int w;
    int h;
} scale_param_t;

static scale_param_t scale_param;
static scale_param_t old_scale_area = {0};
static int get_display_w_();

typedef enum scale_type {
    SCALE_ZOOM_IN,
    SCALE_ZOOM_OUT,
    SCALE_ZOOM_RECOVERY,
    SCALE_4_3,
    SCALE_16_9
} scale_type_t;

static void set_sys_scale(void);


static int get_display_x_(int w){
    if(dis_mode == DIS_TV_AUTO){
        return 0;
    }
    return scale_param.x*DIS_ZOOM_FULL_W/((double)lcd_area.area.w)+X_AMENDMENT;;
}

static int get_display_y_(int h){
    if(dis_mode == DIS_TV_AUTO){
        return 0;
    }
    return scale_param.y*DIS_ZOOM_FULL_H/((double)lcd_area.area.h)+Y_AMENDMENT;
}

static int get_display_w_(int w){
    if(dis_mode == DIS_TV_AUTO){
        return DIS_ZOOM_FULL_W;
    }
    return w * DIS_ZOOM_FULL_W / ((double)lcd_area.area.w) - X_AMENDMENT;
}

static int get_display_h_(int h){
    if(dis_mode == DIS_TV_AUTO){
        return DIS_ZOOM_FULL_H;
    }
    return h * DIS_ZOOM_FULL_H / ((double)lcd_area.area.h) - Y_AMENDMENT;
}


int get_display_x(){
    return get_display_x_(scale_param.w);
}

int get_display_y(){
    return get_display_y_(scale_param.h);
}

int get_display_v(){
    return get_display_h_(scale_param.h);
}

int get_display_h(){
    return get_display_w_(scale_param.w);
}

bool get_screen_is_vertical(void){
    return is_vertical_screen;
}

int get_cur_osd_x(void){
    return scale_param.x;
}

int get_cur_osd_y(void){
    return scale_param.y;
}

int get_cur_osd_h(void){
    return scale_param.w;
}

int get_cur_osd_v(void){
    return scale_param.h;
}

int get_screen_mode(void){
    return zoom_mode;
}

void mainlayer_scale_type_set(mainlayer_scale_t type)
{
    mainlayer_scale_type = type;
}


static void app_get_pic_zoom_src_area(struct dis_area* src_area)
{
    dis_screen_info_t lcd_area_copy = {0};
    memcpy(&lcd_area_copy, &lcd_area, sizeof(dis_screen_info_t));
    dis_screen_info_t pic_inlcd_area={0};
    api_get_display_area(&pic_inlcd_area);
    if(is_vertical_screen){
        api_swap_value(&pic_inlcd_area.area.w, &pic_inlcd_area.area.h, sizeof(uint16_t));
        api_swap_value(&pic_inlcd_area.area.x, &pic_inlcd_area.area.y, sizeof(uint16_t));
        api_swap_value(&lcd_area_copy.area.w, &lcd_area_copy.area.h, sizeof(uint16_t));
        api_swap_value(&lcd_area_copy.area.x, &lcd_area_copy.area.y, sizeof(uint16_t));
        api_swap_value(&old_scale_area.w, &old_scale_area.h, sizeof(int));
        api_swap_value(&old_scale_area.x, &old_scale_area.y, sizeof(int));
    }
    //Convert to the coordinate of pic_inlcd_area full screen
    pic_inlcd_area.area.x = (pic_inlcd_area.area.x - old_scale_area.x)*lcd_area_copy.area.w / old_scale_area.w;
    pic_inlcd_area.area.y = (pic_inlcd_area.area.y - old_scale_area.y)*lcd_area_copy.area.h / old_scale_area.h;
    pic_inlcd_area.area.w = pic_inlcd_area.area.w * lcd_area_copy.area.w / old_scale_area.w;
    pic_inlcd_area.area.h = pic_inlcd_area.area.h * lcd_area_copy.area.h / old_scale_area.h;
    
    //Convert to the coordinate of DE full screen
    src_area->x=(pic_inlcd_area.area.x*DIS_ZOOM_FULL_W)/lcd_area_copy.area.w;
    src_area->y=(pic_inlcd_area.area.y*DIS_ZOOM_FULL_H)/lcd_area_copy.area.h;
    src_area->w=(pic_inlcd_area.area.w*DIS_ZOOM_FULL_W)/lcd_area_copy.area.w;
    src_area->h=(pic_inlcd_area.area.h*DIS_ZOOM_FULL_H)/lcd_area_copy.area.h;
}

static void mainlayer_zoom(int x, int y, int w, int h){
    int w1 = get_display_w_(w);
    int h1 = get_display_h_(h);
    int x1 = get_display_x_(w);
    int y1 = get_display_y_(h);
    bool de_zoom = true;

    struct vdec_dis_rect rect = {{0,0,1920,1080}, {x1, y1, w1, h1}};
    switch (mainlayer_scale_type){
        case MAINLAYER_SCALE_TYPE_LOCAL:{
            media_handle_t* hdl = (media_handle_t*)mp_get_cur_player_hdl();  
            if(hdl){
                if(hdl->type == MEDIA_TYPE_PHOTO){                 
                    static struct dis_area pic_src_area = {0};
                    if(!has_zoom_operation){
                        app_get_pic_zoom_src_area(&pic_src_area);
                    }
                     if(dis_mode != DIS_TV_AUTO){    
                        if(is_vertical_screen){
                            api_swap_value(&x1, &y1, sizeof(int));
                            api_swap_value(&w1, &h1, sizeof(int));
                        }
                        printf("w:%d, h:%d,x:%d,y:%d\n", pic_src_area.w, pic_src_area.h, pic_src_area.x, pic_src_area.y);
                        x1 = (w1 - pic_src_area.w * w1 / DIS_ZOOM_FULL_W)/2 + x1;
                        y1 = (h1 - pic_src_area.h * h1 / DIS_ZOOM_FULL_H)/2 + y1;
                        w1 = pic_src_area.w * w1 / DIS_ZOOM_FULL_W;
                        h1 = pic_src_area.h * h1 / DIS_ZOOM_FULL_H;               
                        if(is_vertical_screen){
                            api_swap_value(&x1, &y1, sizeof(int));
                            api_swap_value(&w1, &h1, sizeof(int));
                        }  
                    }
                
                    printf("w1:%d, h1:%d,x1:%d,y1:%d\n", w1,h1,x1,y1);
                    api_set_display_zoom(0,0, 1920, 1080, x1, y1, w1, h1);
                }else if(hdl->type == MEDIA_TYPE_MUSIC){
                    int rotate = 0 , h_flip = 0 , v_flip = 0;
                    int fbdev_rotate;
                    int init_rotate = api_get_screen_init_rotate();
                    int init_h_flip = api_get_screen_init_h_flip();
                    int init_v_flip = api_get_screen_init_v_flip();
                    struct dis_area music_area = {
                                                    MUSIC_COVER_ZOOM_X*get_display_h()/DIS_ZOOM_FULL_W+get_display_x(),
                                                    MUSIC_COVER_ZOOM_Y*get_display_v()/DIS_ZOOM_FULL_H+get_display_y(),
                                                    MUSIC_COVER_ZOOM_W*get_display_h()/DIS_ZOOM_FULL_W,
                                                    MUSIC_COVER_ZOOM_H*get_display_v()/DIS_ZOOM_FULL_H
                                                };
                    
                    if (get_screen_is_vertical()){
                         api_swap_value(&music_area.w, &music_area.h, sizeof(uint16_t));
                    }

                    get_rotate_by_flip_mode(projector_get_some_sys_param(P_FLIP_MODE), &rotate , &h_flip , &v_flip);
                    api_transfer_rotate_mode_for_screen(init_rotate, init_h_flip, init_v_flip,
                                                        &rotate, &h_flip, &v_flip, &fbdev_rotate);
                    zoom_transfer_dst_rect_for_screen(rotate, h_flip, &music_area);
                    api_set_display_zoom(0,0,1920,1080,music_area.x, music_area.y, music_area.w, music_area.h);
                }else if(hdl->type == MEDIA_TYPE_VIDEO){
                    if(media_get_state(hdl) == MEDIA_PAUSE ||
                       media_get_state(hdl) == MEDIA_STOP ||
                       media_get_state(hdl) == MEDIA_PLAY_END){
                        api_set_display_zoom(0,0, 1920, 1080, x1, y1, w1, h1);
                    }else{
                        media_set_display_rect(hdl, &rect);
                        de_zoom = false;
                    }
                    
                }
            }else{
                    api_set_display_zoom(0,0, 1920, 1080, x1, y1, w1, h1);
            }      
        }
            break;
        case MAINLAYER_SCALE_TYPE_HDMI:
            hdmi_set_display_rect(&rect);        
            de_zoom = false;			
            break;
#ifdef CVBSIN_SUPPORT
        case MAINLAYER_SCALE_TYPE_CVBS:
            cvbs_set_display_rect(&rect);
            de_zoom = false;
            break;
#endif
        case MAINLAYER_SCALE_TYPE_CAST:
            api_set_display_zoom(0,0, 1920, 1080, x1, y1, w1, h1);
            break;
        default:
            api_set_display_zoom(0,0, 1920, 1080, x1, y1, w1, h1);
            break;
    }

    //if reset scale, also reset the DE zoom.
    if (!de_zoom && 0 == projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT))
        api_set_display_zoom(0,0, 1920, 1080, x1, y1, w1, h1);
}

static void osd_zoom(int x, int y, int w, int h){
    fb_zoom_t fb_zoom = {0,};

    int fd_fb = open(DEV_FB , O_RDWR);
	if (fd_fb < 0) {
		printf("%s(), line:%d. open device: %s error!\n", 
			__func__, __LINE__, DEV_FB);
		return;
	}

    int factor1 = w > h ? OSD_MAX_WIDTH : OSD_MAX_HEIGHT;
    int factor2 = w > h ? OSD_MAX_HEIGHT : OSD_MAX_WIDTH;
    int x1 = x*factor1/(double)scale_param.w;
    int y1 = y*factor2/(double)scale_param.h;

    hcfb_scale_t scale_param1 = { factor1, factor2, w, h};    
    hcfb_lefttop_pos_t start_pos1 ={x1,y1};    
    if(dis_mode == DIS_TV_AUTO){
        start_pos1.left = 0;
        start_pos1.top = 0;
        scale_param1.h_mul = lcd_area.area.w;
        scale_param1.v_mul = lcd_area.area.h;        
    }

    fb_zoom.left = start_pos1.left;
    fb_zoom.top = start_pos1.top;
    fb_zoom.h_div = scale_param1.h_div;
    fb_zoom.v_div = scale_param1.v_div;
    fb_zoom.h_mul = scale_param1.h_mul;
    fb_zoom.v_mul = scale_param1.v_mul;
    api_fb_set_zoom_arg(&fb_zoom);

#ifdef KEYSTONE_STRETCH_SUPPORT
    close(fd_fb);
    keystone_fb_viewport_scale();
#else
    ioctl(fd_fb, HCFBIOSET_SET_LEFTTOP_POS, &start_pos1);
    ioctl(fd_fb, HCFBIOSET_SCALE, &scale_param1); 
    printf("%s(), %d %d %d %d\n", __func__, scale_param1.h_div, 
        scale_param1.v_div, scale_param1.h_mul, scale_param1.v_mul);
    close(fd_fb);
#endif
	
}

bool app_has_zoom_operation_get(void)
{
    if (has_zoom_operation){
        has_zoom_operation = false;
        return true;
    }
    return false;
}

void app_has_zoom_operation_set(bool b)
{
    has_zoom_operation = b;
}
void set_display_zoom_when_sys_scale(void){
    media_handle_t* hdl = (media_handle_t*)mp_get_cur_player_hdl();
    if(dis_mode == DIS_TV_AUTO){
        mainlayer_zoom(0, 0, lcd_area.area.w, lcd_area.area.h);
        return;
    }        


    mainlayer_zoom(scale_param.x, scale_param.y, scale_param.w, scale_param.h);
}

static void save_sys_scale_param(void){
    int dx = X_AMENDMENT;
    int dy = Y_AMENDMENT;
    if(dis_mode != DIS_TV_4_3 && !projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT)){
        dx = 0;
        dy = 0;
    }
    int x = scale_param.x;
    int y = scale_param.y;
    int w = scale_param.w;
    int h = scale_param.h;
    if (DIS_TV_AUTO == dis_mode ){
        //auto mode, set to full screen.
        sysdata_disp_rect_save(0, 0, 1920, 1080);
    } else {
        sysdata_disp_rect_save(x*1920/((double)lcd_area.area.w)+dx, y*1080/((double)lcd_area.area.h)+dy, w*1920/((double)lcd_area.area.w)-dx, h*1080/((double)lcd_area.area.h)-dy);
    }

    projector_set_some_sys_param(P_SYS_ZOOM_DIS_MODE, dis_mode);
    projector_set_some_sys_param(P_SYS_ZOOM_OUT_COUNT, zoom_out_count);
    printf("zoom_out_count: %d\n", zoom_out_count);  

    //Here do not need save, it would result in waste time while continue scaling, 
    //Exit scale via turn_to_setup_root() would save parameters.
    //projector_sys_param_save();
}

//adjust the scale parameters, especial while change tv system,
//the output screen information may be changed!!!
static int sys_scale_param_adjust(void)
{
    int w;
    int h;
    dis_screen_info_t dis_area;

    api_get_screen_info(&dis_area);
    if (dis_area.area.w == lcd_area.area.w && dis_area.area.h == lcd_area.area.h)
    {
        return -1;
    }    

    int  a = dis_area.area.w, b = dis_area.area.h;
    get_scr_h_v_by_ratio(&a, &b, dis_mode);
    ui_adjust_w_v = a/100.0;
    ui_adjust_h_v = b/100.0;    
    scale_param.w = a - (int)(zoom_out_count*ui_adjust_w_v);
    scale_param.h = b - (int)(zoom_out_count*ui_adjust_h_v);    
        
    scale_param.x = (dis_area.area.w-scale_param.w)/2.0;
    scale_param.y = (dis_area.area.h-scale_param.h)/2.0;

    memcpy(&lcd_area, &dis_area, sizeof(dis_screen_info_t));

    return 0;
}

//while the tv system is changed, it should adjust
//scale parameters, and re-scale again.
void sys_scale_fb_adjust(void)
{
    if (0 == sys_scale_param_adjust())
        set_sys_scale();
}

void sys_scala_init(void){
    dis_mode = projector_get_some_sys_param(P_SYS_ZOOM_DIS_MODE);
    zoom_mode = dis_mode == DIS_TV_AUTO ? DIS_TV_16_9 : dis_mode;
    zoom_out_count = projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT); 
    printf("zoom_out_count: %d\n", zoom_out_count);  

    api_get_screen_info(&lcd_area);//ui宽和高
    printf("lcd.area.w:%d, lcd.area.h:%d\n",lcd_area.area.w, lcd_area.area.h);
    if(lcd_area.area.w < lcd_area.area.h){
        is_vertical_screen = true;
    }

    int a=lcd_area.area.w, b=lcd_area.area.h;
    get_scr_h_v_by_ratio(&a, &b, dis_mode);
    scale_param.w = a;
    scale_param.h = b;
    printf("scale_param.h_mul:%d, scale_param.v_mul:%d\n", scale_param.w, scale_param.h);
    ui_adjust_w_v = scale_param.w/100.0;
    ui_adjust_h_v = scale_param.h/100.0;  

    if(zoom_out_count>0 && zoom_out_count<=ZOOM_OUT_COUNT_MAX){
        scale_param.w -= (int)(zoom_out_count*ui_adjust_w_v);
        scale_param.h -= (int)(zoom_out_count*ui_adjust_h_v);
    }
    scale_param.x = (lcd_area.area.w-scale_param.w)/2.0;
    scale_param.y = (lcd_area.area.h-scale_param.h)/2.0;

    set_sys_scale();
}


static void set_sys_scale(){
    osd_zoom(scale_param.x, scale_param.y, scale_param.w, scale_param.h);
}

static void reset_scale_w_h(dis_tv_mode_e mode)
{
    int w;
    int h;

    scale_param.w = lcd_area.area.w;
    scale_param.h = lcd_area.area.h;
    w = scale_param.w;
    h = scale_param.h;    
    get_scr_h_v_by_ratio(&w, &h, mode);
    scale_param.w = w;
    scale_param.h = h;
    ui_adjust_w_v = scale_param.w/100.0;
    ui_adjust_h_v = scale_param.h/100.0;    
}
#ifdef NEW_SETUP_ITEM_CTRL
int do_sys_scale(scale_type_t scale_type_v){
#else
static int do_sys_scale(scale_type_t scale_type_v){
#endif

    /* reset display zoom area and reset media_hld member 
     * when media_player change display zoom before do sys scale.
     * */
    media_handle_t * media_hld = mp_get_cur_player_hdl();
    if(media_hld && (media_hld->media_do_opt&DO_ZOOM_OPT)==DO_ZOOM_OPT){
        media_display_sys_scale_reset(media_hld);
    }
    
    //sys_scale_param_adjust();
    memcpy(&old_scale_area, &scale_param, sizeof(scale_param_t));
    switch (scale_type_v)
    {
    case SCALE_ZOOM_OUT:
        if(zoom_out_count<ZOOM_OUT_COUNT_MAX){  
            zoom_out_count++;
        }else{
            return 0;
        }
        if(dis_mode == DIS_TV_AUTO){
            dis_mode = DIS_TV_16_9;
            reset_scale_w_h(DIS_TV_16_9);
        }       
        break;
    case SCALE_ZOOM_IN:
        if(zoom_out_count>0){
            zoom_out_count--; 
        }else{
            return 0;
        }
        if(dis_mode == DIS_TV_AUTO){
            dis_mode = DIS_TV_16_9;
            reset_scale_w_h(DIS_TV_16_9);
        }
        break;
    case SCALE_ZOOM_RECOVERY:
        dis_mode = DIS_TV_AUTO;
        zoom_mode = DIS_TV_16_9;       
        zoom_out_count = 0;
        break; 
    case SCALE_4_3:        
        if(dis_mode == DIS_TV_16_9 || dis_mode == DIS_TV_AUTO){
            if(dis_mode == DIS_TV_AUTO){
                reset_scale_w_h(DIS_TV_4_3);
            }else if(zoom_mode == DIS_TV_16_9){
                if(is_vertical_screen){
                    ui_adjust_h_v = ui_adjust_h_v*3/4;
                }else{
                    ui_adjust_w_v = ui_adjust_w_v*3/4;
                }
            }            
        }else{
            return 0;
        }
        dis_mode = DIS_TV_4_3;
        zoom_mode = dis_mode;
        break;
    case SCALE_16_9:
        if(dis_mode == DIS_TV_4_3 || dis_mode == DIS_TV_AUTO){
            if(dis_mode == DIS_TV_AUTO){
                 reset_scale_w_h(DIS_TV_16_9);
             }else if(zoom_mode == DIS_TV_4_3){
                if(is_vertical_screen){
                    ui_adjust_h_v = ui_adjust_h_v*4/3;
                }else{
                    ui_adjust_w_v = ui_adjust_w_v*4/3;
                }
            }            
        }else {
            return 0;
        }
        dis_mode = DIS_TV_16_9;
        zoom_mode = dis_mode;
        break;
    default:
        break;
    }
    int a=lcd_area.area.w, b=lcd_area.area.h;
    get_scr_h_v_by_ratio(&a, &b, dis_mode);
    printf("old_scale_area(x:%d,y:%d,w:%d,h:%d)\n", old_scale_area.x,old_scale_area.y,old_scale_area.w,old_scale_area.h);
    scale_param.w = a;
    scale_param.h = b;
    scale_param.w -= (int)(zoom_out_count*ui_adjust_w_v);
    scale_param.h -= (int)(zoom_out_count*ui_adjust_h_v);    
        
    scale_param.x = (lcd_area.area.w-scale_param.w)/2.0;
    scale_param.y = (lcd_area.area.h-scale_param.h)/2.0;
    save_sys_scale_param();
    if(scale_type_v == SCALE_ZOOM_OUT || scale_type_v == SCALE_4_3){
        set_display_zoom_when_sys_scale();
        set_sys_scale();        
    }else{
        set_sys_scale();
        set_display_zoom_when_sys_scale();
    }

	has_zoom_operation = true;
    return 0;
}



static void scale_widget_event_cb(lv_event_t* e){
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_btnmatrix_t *btnms = (lv_btnmatrix_t*)obj;

    if(code == LV_EVENT_PRESSED){
        if(timer_setting){
            lv_timer_pause(timer_setting);
        }
        switch (btnms->btn_id_sel)
        {
        case 1:
            do_sys_scale(SCALE_4_3);
            break;
        case 3:
            do_sys_scale(SCALE_ZOOM_OUT);
            break;
        case 4:
            do_sys_scale(SCALE_ZOOM_RECOVERY);
            break;
        case 5:
            do_sys_scale(SCALE_ZOOM_IN);
            break;
        case 7:
            do_sys_scale(SCALE_16_9);
            break;  
        default:
            break;
        }
        if(timer_setting){
            lv_timer_reset(timer_setting);
            lv_timer_resume(timer_setting);
        }
    }else if(code == LV_EVENT_KEY){
        uint16_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ESC){
            lv_obj_del(obj);
            turn_to_setup_root();
        }
    }else if(code == LV_EVENT_DRAW_PART_BEGIN){
        lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
        if(dsc->id == btnms->btn_id_sel){
            dsc->rect_dsc->bg_color = lv_color_make(0,0,255);
            dsc->label_dsc->color = lv_color_white();
        }else{
            if(!lv_btnmatrix_has_btn_ctrl(obj, dsc->id, LV_BTNMATRIX_CTRL_HIDDEN)){
                dsc->rect_dsc->bg_color = lv_color_white();
                dsc->label_dsc->color = lv_color_black();
            }

        }
    }else if(code == LV_EVENT_DRAW_MAIN_BEGIN){

    }
}

void create_scale_widget(lv_obj_t* btn)
{
    static const char* scale_map[] = {" ", "4:3", " ","\n","zoom out" , "recovery", "zoom in","\n", " ", "16:9", " ", ""};
    scale_map[4] = api_rsc_string_get(STR_ZOOMOUT);
    scale_map[5] = api_rsc_string_get(STR_RESET);
    scale_map[6] = api_rsc_string_get(STR_ZOOMIN);
    lv_obj_t *btnms = lv_btnmatrix_create(setup_scr);
    extern lv_obj_t* slave_scr_obj;
    slave_scr_obj = btnms;
    lv_obj_center(btnms);
    lv_obj_set_size(btnms, lv_pct(30), lv_pct(50));
    lv_btnmatrix_set_map(btnms, scale_map);
    lv_obj_set_style_text_font(btnms, osd_font_get(FONT_NORMAL),0);
    lv_btnmatrix_set_btn_ctrl(btnms, 0, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(btnms, 2, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(btnms, 6, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(btnms, 8, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_obj_set_style_bg_opa(btnms, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_border_width(btnms, 0, 0 | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btnms, 0, 0 | LV_STATE_FOCUSED);
    lv_obj_add_event_cb(btnms, scale_widget_event_cb, LV_EVENT_ALL, (void*)btn);
    lv_group_focus_obj(btnms);
}

#else //else of SYS_ZOOM_SUPPORT

#include <hcuapi/dis.h>
#define DIS_ZOOM_FULL_X  0
#define DIS_ZOOM_FULL_Y  0
#define DIS_ZOOM_FULL_H  1080
#define DIS_ZOOM_FULL_W  1920
static void get_scr_h_v_by_ratio(int *w, int *h, int ratio);

void set_display_zoom_when_sys_scale(void){
    return;
}

int get_display_x(void){
    return DIS_ZOOM_FULL_X;
}

int get_display_y(void){
    return DIS_ZOOM_FULL_Y;
}

int get_display_h(void){
    return DIS_ZOOM_FULL_W;
}
int get_display_v(void){
    return DIS_ZOOM_FULL_H;
}

int get_cur_osd_x(void){
    return DIS_ZOOM_FULL_X;
}

int get_cur_osd_y(void){
    return DIS_ZOOM_FULL_Y;
}

int get_cur_osd_h(void){
    dis_screen_info_t lcd_area = {0};
    api_get_screen_info(&lcd_area);
    return lcd_area.area.w;
}

int get_cur_osd_v(void){
    dis_screen_info_t lcd_area = {0};
    api_get_screen_info(&lcd_area);
    return lcd_area.area.h;
}

bool get_screen_is_vertical(){
    dis_screen_info_t lcd_area = {0};
    api_get_screen_info(&lcd_area);
    return lcd_area.area.w  < lcd_area.area.h;
}

#endif

static void get_scr_h_v_by_ratio(int *w, int *h, int ratio){
    if(ratio == DIS_TV_AUTO){
        return;
    }
    int ratio_h = 16;
    int ratio_v = 9;
    if(ratio == DIS_TV_4_3){
        ratio_h = 4;
        ratio_v = 3;
    }
    if(*w < *h){
        api_swap_value(&ratio_h, &ratio_v, sizeof(int));
    }

    if((*w)*ratio_v > (*h)*ratio_h){
        *w = (*h)*ratio_h/ratio_v;
    }else if((*w)*ratio_v < (*h)*ratio_h){
        *h = (*w)*ratio_v/ratio_h;
    }
}

void auto_mode_get_rect_by_ratio(int *h, int *v, int ratio){
    dis_screen_info_t lcd_area = {0};
    api_get_screen_info(&lcd_area);
    int a = lcd_area.area.w, b = lcd_area.area.h;
    get_scr_h_v_by_ratio(&a, &b, ratio);

    *h = a*DIS_ZOOM_FULL_W/lcd_area.area.w;
    *v = b*DIS_ZOOM_FULL_H/lcd_area.area.h;
}
