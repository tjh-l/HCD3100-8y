/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2022-11-29 16:07:56
 */
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include <dirent.h>
#include <sys/stat.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/dis.h>
#include <ffplayer.h>
#include "file_mgr.h"
#include "com_api.h"
#include "osd_com.h"
#include "mp_zoom.h"

#include "local_mp_ui.h"
#include "mp_ctrlbarpage.h"
#include "screen.h" 
#include "setup.h"
#include "factory_setting.h"
#include "mp_fspage.h"

//for zoom 
static Zoom_Param_t zoom_param={0};
static dis_screen_info_t m_dis_info={0};


dis_area_t zoomin_src_disarea={0};
dis_area_t zoomin_dst_disarea={0};
static bool first_flag=true;
//for zoom move
zoom_move_t zoom_move={0};
dis_screen_info_t cur_zoomdis_info={0};

typedef enum{
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
}move_direction_e;
// when first into zoom need to restore original data
static int lcd2dis_coord_conv(dis_screen_info_t * dis_info);
static void zoommove_event_handler(lv_event_t *event);

/**
 * @description:reset param ,need to reset param when playing next or replay
 * beacause zoom factor change when press zoom+/-,reset /dev/dis area 
 * @return {*}
 * @author: Yanisin
 */
void app_reset_diszoom_param(void)
{
    // api_set_display_area(projector_get_some_sys_param(P_ASPECT_RATIO));
    /*due to this apis will change display_aspect,so only do zoom opt*/ 
    dis_zoom_t dis_zoom={
        .src_area.h=DIS_ZOOM_FULL_H,
        .src_area.w=DIS_ZOOM_FULL_W,
        .src_area.x=DIS_ZOOM_FULL_X,
        .src_area.y=DIS_ZOOM_FULL_Y,
        .dst_area.h=get_display_v(),
        .dst_area.w=get_display_h(),
        .dst_area.x=get_display_x(),
        .dst_area.y=get_display_y(),
    };
    /*reset zoom area with it sys scale params */ 
    api_set_display_zoom2(&dis_zoom);
    if(zoom_param.zoom_size!=ZOOM_NORMAL){
        memset(&zoom_param,0,sizeof(Zoom_Param_t));
    }
    first_flag=true;
    setup_sys_scale_set_disable(false);
}

void* app_get_zoom_param(void)
{
    return &zoom_param;
}


/**
 * @description: 
 * @param {float} zoom_factor:0< zoom_factor<2
 * @return {*}
 * @author: Yanisin
 */
static dis_zoom_t app_set_disdev_param(float zoom_factor)
{
    dis_zoom_t diszoom_param;
    diszoom_param.layer= DIS_LAYER_MAIN;
    diszoom_param.distype=DIS_TYPE_HD;
    setup_sys_scale_set_disable(false);

    if(zoom_factor==(float)1.0){
        // 缩放的是整个屏幕，要以屏幕为对象
        diszoom_param.src_area.x=DIS_ZOOM_FULL_X;
        diszoom_param.src_area.y=DIS_ZOOM_FULL_Y;
        diszoom_param.src_area.w=DIS_ZOOM_FULL_W;
        diszoom_param.src_area.h=DIS_ZOOM_FULL_H;

        diszoom_param.dst_area.x=m_dis_info.area.x;
        diszoom_param.dst_area.y=m_dis_info.area.y;
        diszoom_param.dst_area.w=m_dis_info.area.w;
        diszoom_param.dst_area.h=m_dis_info.area.h;

    }else if(zoom_factor<(float)1.0){
        //zoom -

        // 缩放整个屏幕
        diszoom_param.src_area.x=DIS_ZOOM_FULL_X;
        diszoom_param.src_area.y=DIS_ZOOM_FULL_Y;
        diszoom_param.src_area.w=DIS_ZOOM_FULL_W;
        diszoom_param.src_area.h=DIS_ZOOM_FULL_H;

        diszoom_param.dst_area.x=(int)m_dis_info.area.x+(m_dis_info.area.w-m_dis_info.area.w*zoom_factor)/2;
        diszoom_param.dst_area.y=(int)m_dis_info.area.y+(m_dis_info.area.h-m_dis_info.area.h*zoom_factor)/2;
        diszoom_param.dst_area.w=(int)m_dis_info.area.w*zoom_factor;
        diszoom_param.dst_area.h=(int)m_dis_info.area.h*zoom_factor;
        
        //Sys scale parameters do not depend on media player zoom parameters, if player scale down(zoom out),
        //and then sys scale down, may the video scales up, it is strange, because sys scale paramers is larger player 
        //scale parameters. So while in player zoom out, disable sys scale function to avoid the conflict.
        setup_sys_scale_set_disable(true);
    }else if(zoom_factor>(float)1.0){

        // dst_area
        int zoomview_w=get_display_h();
        diszoom_param.dst_area.w=m_dis_info.area.w*zoom_factor>zoomview_w ? zoomview_w:m_dis_info.area.w*zoom_factor;
        int zoomview_h=get_display_v();
        diszoom_param.dst_area.h=m_dis_info.area.h*zoom_factor>zoomview_h ? zoomview_h:m_dis_info.area.h*zoom_factor;
        int zoomview_x=get_display_x();
        diszoom_param.dst_area.x=diszoom_param.dst_area.w==zoomview_w ? zoomview_x:zoomview_x+(zoomview_w-diszoom_param.dst_area.w)/2;
        int zoomview_y=get_display_y();
        diszoom_param.dst_area.y=diszoom_param.dst_area.h==zoomview_h ?zoomview_y:zoomview_y+(zoomview_h-diszoom_param.dst_area.h)/2;

        // src_area
        dis_screen_info_t current_dis={0};
        lcd2dis_coord_conv(&current_dis);
        if(current_dis.area.w==zoomview_w&&current_dis.area.h==zoomview_h){
            //full screen size display
            //due to its type is float so turn it to int 
            diszoom_param.src_area.w=zoomview_w*10/(zoom_factor*10);
            diszoom_param.src_area.h=zoomview_h*10/(zoom_factor*10);
            diszoom_param.src_area.x=zoomview_x+(zoomview_w-diszoom_param.src_area.w)/2;
            diszoom_param.src_area.y=zoomview_y+(zoomview_h-diszoom_param.src_area.h)/2;
        }else if(current_dis.area.w==zoomview_w){
            diszoom_param.src_area.h=zoomview_h;
            diszoom_param.src_area.w=zoomview_w*10/(zoom_factor*10);//等比例取原图区域
            diszoom_param.src_area.x=zoomview_x+(zoomview_w-diszoom_param.src_area.w)/2;
            diszoom_param.src_area.y=zoomview_y;
        }else if(current_dis.area.h==zoomview_h){
            diszoom_param.src_area.h=zoomview_h*10/(zoom_factor*10);//等比例取原图区域
            diszoom_param.src_area.w=zoomview_w;
            diszoom_param.src_area.x=zoomview_x;
            diszoom_param.src_area.y=zoomview_y+(zoomview_h-diszoom_param.src_area.h)/2;
        }else {
            diszoom_param.src_area.w=zoomview_w*10/(zoom_factor*10);
            diszoom_param.src_area.h=zoomview_h*10/(zoom_factor*10);
            diszoom_param.src_area.x=zoomview_x+(zoomview_w-diszoom_param.src_area.w)/2;
            diszoom_param.src_area.y=zoomview_y+(zoomview_h-diszoom_param.src_area.h)/2;
        }
        // memcpy zoom param use for zoom move
        memcpy(&zoomin_src_disarea,&diszoom_param.src_area,sizeof(dis_area_t));
        memcpy(&zoomin_dst_disarea,&diszoom_param.dst_area,sizeof(dis_area_t));

    }

    return diszoom_param;
    
}

/**
 * @description: 将LCD上图像显示的坐标转换到de 设备显示的坐标上
 * de 设备的坐标默认是1920 *1080 ，而不同LCD上获取到的信息通过
 * api_get_display_area 
 * @return {*}
 * @author: Yanisin
 */
static int lcd2dis_coord_conv(dis_screen_info_t * dis_info)
{
    dis_screen_info_t lcd_area={0};
    dis_screen_info_t pic_inlcd_area={0};
    api_get_screen_info(&lcd_area);
    api_get_display_area(&pic_inlcd_area);
    dis_info->area.x=(pic_inlcd_area.area.x*DIS_ZOOM_FULL_W)/lcd_area.area.w + X_AMENDMENT;
    dis_info->area.y=(pic_inlcd_area.area.y*DIS_ZOOM_FULL_H)/lcd_area.area.h + Y_AMENDMENT;
    dis_info->area.w=(pic_inlcd_area.area.w*DIS_ZOOM_FULL_W)/lcd_area.area.w - X_AMENDMENT;
    dis_info->area.h=(pic_inlcd_area.area.h*DIS_ZOOM_FULL_H)/lcd_area.area.h - Y_AMENDMENT;
    return 0;
}

int app_set_diszoom(Zoom_mode_e Zoom_mode, bool isplay)
{
    dis_zoom_t zoom_argv={0};
    media_handle_t* hdl = mp_get_cur_player_hdl();
	int do_zoom = 1;
	
    if((zoom_param.zoom_size==ZOOM_NORMAL && first_flag)
#ifdef SYS_ZOOM_SUPPORT
        || app_has_zoom_operation_get()
#endif
        ){
        first_flag = false;
#ifdef SYS_ZOOM_SUPPORT
    app_has_zoom_operation_set(false);
#endif
        // api_get_display_area(&m_dis_info);
        lcd2dis_coord_conv(&m_dis_info);
    }
    if(Zoom_mode==MPZOOM_IN){
        zoom_param.zoom_state=zoom_param.zoom_state<3?zoom_param.zoom_state+1:0;
    }else{
        zoom_param.zoom_state=zoom_param.zoom_state>-3?zoom_param.zoom_state-1:0;
    }
    switch (zoom_param.zoom_state){
        case ZOOM_OUTSIZE_3:
            zoom_argv=app_set_disdev_param(0.5);
            zoom_param.zoom_size=ZOOM_OUTSIZE_3;
            break;
        case ZOOM_OUTSIZE_2:
            zoom_argv=app_set_disdev_param(0.66);
            zoom_param.zoom_size=ZOOM_OUTSIZE_2;
            break;
        case ZOOM_OUTSIZE_1://--
            zoom_argv=app_set_disdev_param(0.83);
            zoom_param.zoom_size=ZOOM_OUTSIZE_1;
            break;
        case ZOOM_NORMAL:
            zoom_argv=app_set_disdev_param(1.0);
            zoom_param.zoom_size=ZOOM_NORMAL;
            break;
        case ZOOM_INSIZE_1://zoom +
            zoom_argv=app_set_disdev_param(1.2);
            zoom_param.zoom_size=ZOOM_INSIZE_1;
            break;
        case ZOOM_INSIZE_2:
            zoom_argv=app_set_disdev_param(1.6);
            zoom_param.zoom_size=ZOOM_INSIZE_2;
            break;
        case ZOOM_INSIZE_3:
            zoom_argv=app_set_disdev_param(1.8);
            zoom_param.zoom_size=ZOOM_INSIZE_3;
            break;
        default :
			do_zoom = 0;
            break;
    }
	
	if (do_zoom){
        if(hdl->type == MEDIA_TYPE_VIDEO && isplay){
            struct vdec_dis_rect rect = {{zoom_argv.src_area.x, zoom_argv.src_area.y, zoom_argv.src_area.w, zoom_argv.src_area.h}, 
                                         {zoom_argv.dst_area.x, zoom_argv.dst_area.y, zoom_argv.dst_area.w, zoom_argv.dst_area.h}};
            media_set_display_rect(hdl, &rect);
        }else{
            api_set_display_zoom2(&zoom_argv);
        }
	}

    return 0;

}

static int zoom_move_init(void* argc)
{
    //cur dis info is after zoom 
    // api_get_display_area(&cur_zoomdis_info);
    memcpy(&cur_zoomdis_info.area,&zoomin_src_disarea,sizeof(dis_area_t));
    zoom_move.move_step=ZOOM_MOVE_STEP;//move ZOOM_MOVE_STEP unit when press key
    zoom_move.zoom_area.up_range=cur_zoomdis_info.area.y;
    zoom_move.zoom_area.down_range=DIS_ZOOM_FULL_H-cur_zoomdis_info.area.y-cur_zoomdis_info.area.h;
    // zoom_move.zoom_area.left_range=cur_zoomdis_info.area.x-m_dis_info.area.x;
    // zoom_move.zoom_area.right_range=(m_dis_info.area.x+m_dis_info.area.w)-(cur_zoomdis_info.area.x+cur_zoomdis_info.area.w);
    zoom_move.zoom_area.left_range=cur_zoomdis_info.area.x;
    zoom_move.zoom_area.right_range=DIS_ZOOM_FULL_W-(cur_zoomdis_info.area.x+cur_zoomdis_info.area.w);
    return 0;
}

static dis_area_t zoom_move_set_param(move_direction_e move_direction)
{
    static dis_area_t dst_area_aftermove={0};

    //边界处理,还要处理当0<range<step情况
    switch (move_direction){
        case MOVE_UP:
            if(zoom_move.zoom_area.up_range>0&&zoom_move.zoom_area.up_range<zoom_move.move_step){
                zoom_move.move_step=zoom_move.zoom_area.up_range;
            }else{
                zoom_move.move_step=ZOOM_MOVE_STEP;
                if(zoom_move.zoom_area.up_range==0)
                    break;
            }
            zoom_move.zoom_area.up_range=zoom_move.zoom_area.up_range-zoom_move.move_step;
            zoom_move.zoom_area.down_range=zoom_move.zoom_area.down_range+zoom_move.move_step;
            break;
        case MOVE_DOWN:
            if(zoom_move.zoom_area.down_range>0&&zoom_move.zoom_area.down_range<zoom_move.move_step){
                zoom_move.move_step=zoom_move.zoom_area.down_range;
            }else{
                zoom_move.move_step=ZOOM_MOVE_STEP;
                if(zoom_move.zoom_area.down_range==0)
                    break;
            }
            zoom_move.zoom_area.down_range=zoom_move.zoom_area.down_range-zoom_move.move_step;
            zoom_move.zoom_area.up_range=zoom_move.zoom_area.up_range+zoom_move.move_step;
            break;
        case MOVE_LEFT:
            if(zoom_move.zoom_area.left_range>0&&zoom_move.zoom_area.left_range<zoom_move.move_step){
                zoom_move.move_step=zoom_move.zoom_area.left_range;
            }else{
                zoom_move.move_step=ZOOM_MOVE_STEP;                   
                if(zoom_move.zoom_area.left_range==0)
                    break;
            }
            zoom_move.zoom_area.left_range=zoom_move.zoom_area.left_range-zoom_move.move_step;
            zoom_move.zoom_area.right_range=zoom_move.zoom_area.right_range+zoom_move.move_step;
            break;
        case MOVE_RIGHT:
            if(zoom_move.zoom_area.right_range>0&&zoom_move.zoom_area.right_range<zoom_move.move_step){
                zoom_move.move_step=zoom_move.zoom_area.right_range;
            }else{
                zoom_move.move_step=ZOOM_MOVE_STEP;                   
                if(zoom_move.zoom_area.right_range==0)
                    break;
            }
            zoom_move.zoom_area.right_range=zoom_move.zoom_area.right_range-zoom_move.move_step;
            zoom_move.zoom_area.left_range=zoom_move.zoom_area.left_range+zoom_move.move_step;
            break;
        default :
            break;
    }
    
    // dst_area_aftermove.x=m_dis_info.area.x+zoom_move.zoom_area.left_range;
    dst_area_aftermove.x=zoom_move.zoom_area.left_range;
    dst_area_aftermove.y=zoom_move.zoom_area.up_range;
    dst_area_aftermove.w=cur_zoomdis_info.area.w;
    dst_area_aftermove.h=cur_zoomdis_info.area.h;



    return dst_area_aftermove;
}

static move_direction_e zoom_move_direction_remap(uint16_t lv_key)
{
    move_direction_e ret_dir=0;
    // do handle for rotate
    int rotate = 0 , h_flip = 0;
    api_get_flip_info(&rotate , &h_flip);
    switch(rotate){
        case ROTATE_TYPE_0:
        {
            if(lv_key==LV_KEY_UP){
                ret_dir=MOVE_UP;
            }else if(lv_key==LV_KEY_DOWN){
                ret_dir=MOVE_DOWN;
            }else if(lv_key==LV_KEY_LEFT){
                ret_dir=MOVE_LEFT;
            }else if(lv_key==LV_KEY_RIGHT){
                ret_dir=MOVE_RIGHT;
            }
            break;
        }
        case ROTATE_TYPE_90:
        {
            if(lv_key==LV_KEY_UP){
                ret_dir=MOVE_RIGHT;
            }else if(lv_key==LV_KEY_DOWN){
                ret_dir=MOVE_LEFT;
            }else if(lv_key==LV_KEY_LEFT){
                ret_dir=MOVE_UP;
            }else if(lv_key==LV_KEY_RIGHT){
                ret_dir=MOVE_DOWN;
            }
            break;
        }
        case ROTATE_TYPE_180:
        {
            if(lv_key==LV_KEY_UP){
                ret_dir=MOVE_DOWN;
            }else if(lv_key==LV_KEY_DOWN){
                ret_dir=MOVE_UP;
            }else if(lv_key==LV_KEY_LEFT){
                ret_dir=MOVE_RIGHT;
            }else if(lv_key==LV_KEY_RIGHT){
                ret_dir=MOVE_LEFT;
            }
            break;
        }
            
        case ROTATE_TYPE_270:
        {
            if(lv_key==LV_KEY_UP){
                ret_dir=MOVE_LEFT;
            }else if(lv_key==LV_KEY_DOWN){
                ret_dir=MOVE_RIGHT;
            }else if(lv_key==LV_KEY_LEFT){
                ret_dir=MOVE_DOWN;
            }else if(lv_key==LV_KEY_RIGHT){
                ret_dir=MOVE_UP;
            }
            break;
        }
            
    }


    // superpose the mirror after do rotate
    if(h_flip==1){
        switch(ret_dir){
            case MOVE_LEFT:
                ret_dir=MOVE_RIGHT;
                break;
            case MOVE_RIGHT:
                ret_dir=MOVE_LEFT;
                break;
            default:
                break;
        }
    }

    return ret_dir;

}

static int zoom_move_operation(uint32_t lv_key)
{
    dis_area_t cur_zoomdis_area={0};
    dis_zoom_t move_zoom={0};
    // do a remap key 
    move_direction_e ret_dir=zoom_move_direction_remap(lv_key);
    cur_zoomdis_area=zoom_move_set_param(ret_dir);
    move_zoom.layer = DIS_LAYER_MAIN;
    move_zoom.distype = DIS_TYPE_HD;
    move_zoom.active_mode=DIS_SCALE_ACTIVE_IMMEDIATELY;
    memcpy(&move_zoom.dst_area,&zoomin_dst_disarea,sizeof(dis_area_t));
    memcpy(&move_zoom.src_area,&cur_zoomdis_area,sizeof(dis_area_t));
    api_set_display_zoom2(&move_zoom);
    return 0;
}

/*lvgl show&operation*/
static lv_obj_t*  create_zoommove_cont(lv_obj_t *p)
{
    lv_obj_t* obj = lv_obj_create(p);
    lv_obj_set_size(obj,ZOOMMOVE_CONT_SIZE,ZOOMMOVE_CONT_SIZE);
    lv_obj_set_align(obj, LV_ALIGN_TOP_RIGHT);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x323232), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(obj, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(obj, ZOOMMOVE_LABSIZE, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t* sublabel = lv_label_create(obj);
    lv_obj_set_width(sublabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(sublabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(sublabel, LV_ALIGN_TOP_MID);
    lv_label_set_text(sublabel,LV_SYMBOL_UP);
    lv_obj_set_style_text_color(sublabel,lv_palette_main(LV_PALETTE_BLUE),LV_STATE_FOCUS_KEY);

    lv_obj_t* sublabel1 = lv_label_create(obj);
    lv_obj_set_width(sublabel1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(sublabel1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(sublabel1, LV_ALIGN_LEFT_MID);
    lv_label_set_text(sublabel1,LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(sublabel1,lv_palette_main(LV_PALETTE_BLUE),LV_STATE_FOCUS_KEY);


    lv_obj_t* sublabel2 = lv_label_create(obj);
    lv_obj_set_width(sublabel2, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(sublabel2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(sublabel2, LV_ALIGN_BOTTOM_MID);
    lv_label_set_text(sublabel2,LV_SYMBOL_DOWN);
    lv_obj_set_style_text_color(sublabel2,lv_palette_main(LV_PALETTE_BLUE),LV_STATE_FOCUS_KEY);


    lv_obj_t* sublabel3 = lv_label_create(obj);
    lv_obj_set_width(sublabel3, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(sublabel3, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(sublabel3, LV_ALIGN_RIGHT_MID);
    lv_label_set_text(sublabel3,LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(sublabel3,lv_palette_main(LV_PALETTE_BLUE),LV_STATE_FOCUS_KEY);

    return obj;
}

int create_zoommoove_win(lv_obj_t* p,lv_obj_t * subbtn)
{
    lv_obj_t* inst_cont=create_zoommove_cont(p);
    lv_obj_add_event_cb(inst_cont,zoommove_event_handler,LV_EVENT_ALL,subbtn);
    lv_group_add_obj(lv_group_get_default(),inst_cont);
    lv_group_focus_obj(inst_cont);
    //init param
    zoom_move_init(NULL);
    /* while do zoom move opt do not let it change zoom_move state machine*/
    setup_sys_scale_set_disable(true);
    return 0;
}

static void key_operation_show_reset(lv_timer_t* t)
{
    lv_obj_t  * user_data = t->user_data;
    for(int i=0;i<lv_obj_get_child_cnt(user_data);i++)
        lv_obj_clear_state(lv_obj_get_child(user_data,i), LV_STATE_FOCUS_KEY);
}

static lv_timer_t* timer_handler=NULL;
static int key_operation_show(lv_obj_t* t,uint32_t key)
{
    switch(key)
    {
        case LV_KEY_UP:
            lv_obj_add_state(lv_obj_get_child(t,0), LV_STATE_FOCUS_KEY);
            for(int i=1;i<lv_obj_get_child_cnt(t);i++)
                lv_obj_clear_state(lv_obj_get_child(t,i), LV_STATE_FOCUS_KEY);
            break;
        case LV_KEY_LEFT:
            lv_obj_add_state(lv_obj_get_child(t,1), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,0), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,2), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,3), LV_STATE_FOCUS_KEY);
            break;
        case LV_KEY_DOWN:
            lv_obj_add_state(lv_obj_get_child(t,2), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,0), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,1), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,3), LV_STATE_FOCUS_KEY);
            break;
        case LV_KEY_RIGHT:
            lv_obj_add_state(lv_obj_get_child(t,3), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,0), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,1), LV_STATE_FOCUS_KEY);
            lv_obj_clear_state(lv_obj_get_child(t,2), LV_STATE_FOCUS_KEY);
            break;
        case LV_KEY_ESC:
            if(timer_handler!=NULL){
                lv_timer_del(timer_handler);
                timer_handler=NULL;
            }
            return 0;
        default : 
            break;
    }
    if(timer_handler==NULL){
        timer_handler=lv_timer_create(key_operation_show_reset,500,t);
    }else{
        lv_timer_pause(timer_handler);
        lv_timer_reset(timer_handler);
        lv_timer_resume(timer_handler);
    }
    return 0;
}

static void zoommove_event_handler(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * target =lv_event_get_target(event);
    lv_obj_t *user_data=lv_event_get_user_data(event);
    if(code==LV_EVENT_PRESSED){

    }else if(code==LV_EVENT_KEY){
        int key_val = lv_indev_get_key(lv_indev_get_act());
        show_play_bar(true);
        lv_timer_reset(bar_show_timer);
        zoom_move_operation(key_val);
        key_operation_show(target,key_val);
        if(key_val==LV_KEY_ESC){
            lv_group_focus_obj(user_data);
            lv_obj_del(target);
            setup_sys_scale_set_disable(false);
        }
    }else if(code == LV_EVENT_DELETE){
        if(timer_handler!=NULL){
            lv_timer_del(timer_handler);
            timer_handler=NULL;
            setup_sys_scale_set_disable(false);
        }
    }

}

void zoom_transfer_dst_rect_for_screen(int rotate,int h_flip,struct dis_area *p_dst_area)
{
    int x = 0 , y = 0 , w = 0 , h = 0;

    x = p_dst_area->x;
    y = p_dst_area->y;
    w = p_dst_area->w;
    h = p_dst_area->h;

    printf("%s rotate:%d,h_flip:%d\n",__FUNCTION__, rotate, h_flip);
    switch(rotate)
    {
        case ROTATE_TYPE_90:
        {
            x = (DIS_ZOOM_FULL_H - y - h) * DIS_ZOOM_FULL_W / DIS_ZOOM_FULL_H;
            y = p_dst_area->x * DIS_ZOOM_FULL_H / DIS_ZOOM_FULL_W;
            w = p_dst_area->h * DIS_ZOOM_FULL_W / DIS_ZOOM_FULL_H;
            h = p_dst_area->w * DIS_ZOOM_FULL_H / DIS_ZOOM_FULL_W;
            break;
        }
        case ROTATE_TYPE_180:
        {
            x = (DIS_ZOOM_FULL_W - x - w);
            break;
        }
        case ROTATE_TYPE_270:
        {
            x = y * DIS_ZOOM_FULL_W / DIS_ZOOM_FULL_H;
            y = (DIS_ZOOM_FULL_W - p_dst_area->x - w) * DIS_ZOOM_FULL_H / DIS_ZOOM_FULL_W;
            w = p_dst_area->h * DIS_ZOOM_FULL_W / DIS_ZOOM_FULL_H;
            h = p_dst_area->w * DIS_ZOOM_FULL_H / DIS_ZOOM_FULL_W;
            break;
        }
        default:
            break;
    }

    if(h_flip == 1)
    {
        x = (DIS_ZOOM_FULL_W - x - w);
    }

    p_dst_area->x = x;
    p_dst_area->y = y;
    p_dst_area->w = w;
    p_dst_area->h = h;
}

/**
 * @description: reset zoom args when filp so that use this param to set zoom pos
 * @param {flip_mode_e} flip_mode
 * @return {*}
 * @author: Yanisin
 */
dis_zoom_t app_reset_mainlayer_param(int rotate , int h_flip)
{
    dis_zoom_t musiccover_param={0};
    musiccover_param.active_mode=DIS_SCALE_ACTIVE_IMMEDIATELY;
    musiccover_param.distype= DIS_TYPE_HD;
    musiccover_param.layer= DIS_LAYER_MAIN;
    musiccover_param.src_area.x=DIS_ZOOM_FULL_X;
    musiccover_param.src_area.y=DIS_ZOOM_FULL_Y;
    musiccover_param.src_area.w=DIS_ZOOM_FULL_W;
    musiccover_param.src_area.h=DIS_ZOOM_FULL_H;

    musiccover_param.dst_area.x = MUSIC_COVER_ZOOM_X*get_display_h()/DIS_ZOOM_FULL_W+get_display_x();
    musiccover_param.dst_area.y = MUSIC_COVER_ZOOM_Y*get_display_v()/DIS_ZOOM_FULL_H+get_display_y();
    musiccover_param.dst_area.w = MUSIC_COVER_ZOOM_W*get_display_h()/DIS_ZOOM_FULL_W;
    musiccover_param.dst_area.h = MUSIC_COVER_ZOOM_H*get_display_v()/DIS_ZOOM_FULL_H;

    if (get_screen_is_vertical()){
        api_swap_value(&musiccover_param.dst_area.w, &musiccover_param.dst_area.h, sizeof(uint16_t));
    }

    zoom_transfer_dst_rect_for_screen(rotate, h_flip,&(musiccover_param.dst_area));
    return musiccover_param;
}

/**
 * @description: because rotate do not change the origin of coordinates in DE dev.
 * so had to reset main layer zoom pos when play music to show music cover 
 * only use for music cover when playing music.
 * @return {*}
 * @author: Yanisin
 */
int app_reset_mainlayer_pos(int rotate , int h_flip)
{
    if(lv_scr_act() == ui_fspage){
        /* do not change the preview win pos when doing preview*/
        return 0;
    }else if(lv_scr_act() == ui_ctrl_bar || lv_scr_act() == setup_scr){
        dis_zoom_t musiccover_param2={0};
        musiccover_param2=app_reset_mainlayer_param(rotate , h_flip);
        api_set_display_zoom2(&musiccover_param2);
    }
    return 0;
}

void app_reset_diszoom_param_by_ratio(int tv_mode)
{
    /*reset zoom area with it sys scale params and video ratio set */ 
    api_set_display_area(tv_mode);
    if(zoom_param.zoom_size!=ZOOM_NORMAL){
        memset(&zoom_param,0,sizeof(Zoom_Param_t));
    }
    first_flag=true;
    setup_sys_scale_set_disable(false);
}

/*
 * reset default diszoom param (The initial display area)
 * */
void app_reset_diszoom_default_param_by_sysscale(void)
{
    if(zoom_param.zoom_size!=ZOOM_NORMAL){
        dis_zoom_t dis_zoom={
            .src_area.h=DIS_ZOOM_FULL_H,
            .src_area.w=DIS_ZOOM_FULL_W,
            .src_area.x=DIS_ZOOM_FULL_X,
            .src_area.y=DIS_ZOOM_FULL_Y,
            .dst_area.h=m_dis_info.area.h,
            .dst_area.w=m_dis_info.area.w,
            .dst_area.x=m_dis_info.area.x,
            .dst_area.y=m_dis_info.area.y,
        };
        api_set_display_zoom2(&dis_zoom);
        memset(&zoom_param,0,sizeof(Zoom_Param_t));
    }
    first_flag=true;
    setup_sys_scale_set_disable(false);
}
