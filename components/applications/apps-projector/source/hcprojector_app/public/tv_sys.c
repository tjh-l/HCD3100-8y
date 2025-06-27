/*
tv_sys.c: process the tv system/lcd, scale, rotation, etc
 */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <hcuapi/dis.h>
#include <hcuapi/hdmi_tx.h>
#include <sys/ioctl.h>
#ifdef __linux__
#include <linux/fb.h>
#else
#include <kernel/fb.h>
#include <kernel/lib/fdt_api.h>
#include "lv_drivers/display/fbdev.h"
#endif
#include <hcuapi/fb.h>

#include "com_api.h"
#include "osd_com.h"
//#include "data_mgr.h"
//
#include "factory_setting.h"
#include "tv_sys.h"
#include "setup.h"

#define DEFAULT_TV_SYS	TV_LINE_1080_60
static int m_best_tv_type = DEFAULT_TV_SYS;
#define     DE_DTS_PATH "/proc/device-tree/hcrtos/de-engine"

static int _tvsys_get_vga_dac_enable(void)
{
    int np;
    static int vga_enable = 0;
    static int is_checked = 0;
    const char *st = NULL;

    if (is_checked == 0)
    {
#ifdef __HCRTOS__    
        np = fdt_node_probe_by_path("/hcrtos/de-engine");
        if (np >= 0)
        {
            fdt_get_property_string_index(np, "status", 0, &st);
            if (!strcmp(st, "okay"))
            {
                fdt_get_property_u_32_index(np, "vga-output", 0, &vga_enable);
            }
        }  
#else
        char status[16] = {0};
        api_dts_string_get(DE_DTS_PATH "/status", status, sizeof(status));
        if(!strcmp(status, "okay"))
        {
            vga_enable = api_dts_uint32_get(DE_DTS_PATH "/vga-output");      
        }
#endif

        is_checked = 1;
    }    
    
    return vga_enable;
}

static int _reg_dac(int dac_tpye, int fd)
{
	struct dis_dac_param uvhance = { 0 };

	uvhance.distype = DIS_TYPE_SD;
	uvhance.info.type = dac_tpye;
	uvhance.info.dac.dacidx.cvbs.cv = (1 << 0);
	uvhance.info.dac.enable = 1;
	uvhance.info.dac.progressive = false;

	ioctl(fd, DIS_REGISTER_DAC, &uvhance);
	return 0;
}

static int _unreg_dac(int dac_tpye, int fd)
{
	struct dis_dac_param vhance = { 0 };
	vhance.distype = DIS_TYPE_SD;
	vhance.info.type = dac_tpye;
	ioctl(fd, DIS_UNREGISTER_DAC, &vhance);
	return 0;
}

static int _reg_vga_dac(void)
{
    struct dis_dac_param dac_param = { 0 };
    int fd;

    if (_tvsys_get_vga_dac_enable() == 0)
    {
        return -1;
    }

    dac_param.distype = DIS_TYPE_HD;
    dac_param.info.type = DIS_DAC_RGB;
    dac_param.info.dac.enable = true;
    dac_param.info.dac.progressive = true;
    dac_param.info.dac.dacidx.rgb.r = 4;
    dac_param.info.dac.dacidx.rgb.g = 2;
    dac_param.info.dac.dacidx.rgb.b = 1;

    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0)
    {
        return -1;
    }
    
    ioctl(fd , DIS_REGISTER_DAC , &dac_param);
    close(fd);
    printf("reg vga dac done.\n");
    
    return 0;
}

static int _unreg_vga_dac(void)
{
    struct dis_dac_param dac_param = { 0 };
    int fd;

    if (_tvsys_get_vga_dac_enable() == 0)
    {
        return -1;
    }

    dac_param.distype = DIS_TYPE_HD;
    dac_param.info.type = DIS_DAC_RGB;

    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0)
    {
        return -1;
    }
    
    ioctl(fd , DIS_UNREGISTER_DAC , &dac_param);
    close(fd);
    printf("unreg vga dac done.\n");

    return 0;
}

static int _sd_set_tvsys(struct dis_tvsys* tvsys, int fd)
{
	tvsys->distype = DIS_TYPE_SD;
	if(tvsys->tvtype == TV_LINE_720_25 || tvsys->tvtype == TV_LINE_1080_25 ||
			tvsys->tvtype == TV_LINE_1080_50 || tvsys->tvtype == TV_PAL) {
		tvsys->tvtype = TV_PAL;
	} else if (tvsys->tvtype == TV_LINE_720_30 || tvsys->tvtype == TV_LINE_1080_30 ||
			tvsys->tvtype == TV_LINE_1080_60 || tvsys->tvtype == TV_LINE_4096X2160_30 ||
			tvsys->tvtype == TV_LINE_3840X2160_30 || tvsys->tvtype == TV_NTSC) {
		tvsys->tvtype = TV_NTSC;
	} else {
		printf("tvtype not common value, please do other choice again\n");
		return 0;
	}
	tvsys->progressive = false;
	printf("sd tvsys->tvtype = %d\n",tvsys->tvtype);
	ioctl(fd , DIS_SET_TVSYS , tvsys); //SD
	return 0;
}

static int _tvsys_to_tvtype(enum TVSYS tv_sys, enum TVTYPE *tvtype)
{
	switch(tv_sys){
	case TVSYS_480I:
		*tvtype = TV_NTSC;
		break;
	case TVSYS_480P:
		*tvtype = TV_NTSC;
		break;
	case TVSYS_576I :
		*tvtype = TV_PAL;
		break;
	case TVSYS_576P:
		*tvtype = TV_PAL;
		break;
	case TVSYS_720P_50:
		*tvtype = TV_LINE_720_30;
		break;
	case TVSYS_720P_60:
		*tvtype = TV_LINE_720_30;
		break;
	case TVSYS_1080I_25:
		*tvtype = TV_LINE_1080_25;
		break;
	case TVSYS_1080I_30:
		*tvtype = TV_LINE_1080_30;
		break;
	case TVSYS_1080P_24:
		*tvtype = TV_LINE_1080_24;
		break;
	case TVSYS_1080P_25:
		*tvtype = TV_LINE_1080_25;
		break;
	case TVSYS_1080P_30:
		*tvtype = TV_LINE_1080_30;
		break;
	case TVSYS_1080P_50:
		*tvtype = TV_LINE_1080_50;
		break;
	case TVSYS_1080P_60:
		*tvtype = TV_LINE_1080_60;
		break;
	case TVSYS_3840X2160P_30:
		*tvtype = TV_LINE_3840X2160_30;
		break;
	case TVSYS_4096X2160P_30:
		//*tvtype = TV_LINE_4096X2160_30;
		*tvtype = TV_LINE_3840X2160_30;
		break;
	default:
		*tvtype = TV_LINE_720_30;
		break;
	}

	printf("%s:%d: tvtype=%d\n", __func__, __LINE__, *tvtype);

	return 0;
}


static hcfb_scale_t scale_param = { OSD_MAX_WIDTH, OSD_MAX_HEIGHT, 1920, 1080 };// to do later...
static void _tv_sys_fb_scale_out(int fd_dis)
{
	int fd_fb = -1;
	struct dis_screen_info screen = { 0 };

	screen.distype = DIS_TYPE_HD;
	if(ioctl(fd_dis, DIS_GET_SCREEN_INFO, &screen)) {
		return;
	}

	fd_fb = open(DEV_FB , O_RDWR);
	if (fd_fb < 0) {
		printf("%s(), line:%d. open device: %s error!\n", 
			__func__, __LINE__, DEV_FB);
		return;
	}

#if 0
	hcfb_viewport_t viewport = { 0 };
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = screen.area.w;
	viewport.height = screen.area.h;
	ioctl(fd_fb, HCFBIOSET_SET_VIEWPORT, &viewport);	
#endif	

#ifdef SYS_ZOOM_SUPPORT
	//tv system is changed, it may adjust the
	//sys scale parameters
	close(fd_fb);
	sys_scale_fb_adjust();
#else	
	scale_param.h_mul = screen.area.w;
	scale_param.v_mul = screen.area.h;
	ioctl(fd_fb, HCFBIOSET_SCALE, &scale_param);	
	close(fd_fb);
#endif	
}


static void _tv_sys_best_tv_type_set(int tv_type)
{
	m_best_tv_type = tv_type;
}

int tv_sys_best_tv_type_get(void)
{
	return m_best_tv_type;
}

static int _tv_sys_edid_get(uint32_t timeout)
{
	int fd_hdmi = -1;
	enum TVSYS tv_sys = 0;
	enum TVTYPE tv_type = DEFAULT_TV_SYS;
	uint32_t loop_cnt = timeout/100;
	int ret = -1;

	fd_hdmi = open(DEV_HDMI , O_RDWR);
	if (fd_hdmi < 0) {
		printf("%s(), line:%d. open device: %s error!\n", 
			__func__, __LINE__, DEV_HDMI);
		return -1;
	} else {
		do{
			ret = ioctl(fd_hdmi, HDMI_TX_GET_EDID_TVSYS, &tv_sys);
			if (ret >= 0)
				break;
			api_sleep_ms(100);	

		}while(loop_cnt --);

		if (ret < 0) {
			//return default tv sys if get edid fail!
			tv_type = DEFAULT_TV_SYS;
			printf("%s(), line:%d, HDMI_TX_GET_EDID_TVSYS error\n", __func__, __LINE__);
		} else {
            _tvsys_to_tvtype(tv_sys, &tv_type);
		}
	}
    _tv_sys_best_tv_type_set(tv_type);

	close(fd_hdmi);
	// if (ret < 0)
	// 	return -1;

	return tv_type;
}



int tv_sys_app_sys_to_de_sys(int app_tv_sys)
{
	int tv_type = TV_LINE_1080_60;

	switch(app_tv_sys)
	{
	case APP_TV_SYS_480P:
		tv_type = TV_NTSC;
		break;
	case APP_TV_SYS_576P:
		tv_type = TV_PAL;
		break;
	case APP_TV_SYS_720P:
		tv_type = TV_LINE_720_30;
		break;
	case APP_TV_SYS_1080P:
		tv_type = TV_LINE_1080_60;
		break;
	case APP_TV_SYS_4K:
		// tv_type = TV_LINE_4096X2160_30;
		tv_type = TV_LINE_3840X2160_30;
		break;
	default:
		break;
	}

	return tv_type;
}

static int tv_sys_de_sys_to_app_sys(int tv_type)
{
	int app_tv_sys = APP_TV_SYS_1080P;

	switch(tv_type)
	{
	case TV_NTSC:
		app_tv_sys = APP_TV_SYS_480P;
		break;
	case TV_PAL:
		app_tv_sys = APP_TV_SYS_576P;
		break;
	case TV_LINE_720_30:
		app_tv_sys = APP_TV_SYS_720P;
		break;
	case TV_LINE_1080_60:
		app_tv_sys = APP_TV_SYS_1080P;
		break;
	case TV_LINE_3840X2160_30:
		app_tv_sys = APP_TV_SYS_4K;
		break;
	default:
		break;
	}
	return app_tv_sys;
}

static int _tv_sys_get_dual_output_enable(void)
{
    int np;
    static int dual_enable = 0;
    static int is_checked = 0;
    const char *st = NULL;

    if (is_checked == 0)
    {
#ifdef __HCRTOS__    
        np = fdt_node_probe_by_path("/hcrtos/de-engine");
        if (np >= 0)
        {
            fdt_get_property_string_index(np, "status", 0, &st);
            if (!strcmp(st, "okay"))
            {
                fdt_get_property_u_32_index(np, "dual-output", 0, &dual_enable);
            }
        }  
#else
        char status[16] = {0};
        api_dts_string_get(DE_DTS_PATH "/status", status, sizeof(status));
        if(!strcmp(status, "okay"))
        {
            dual_enable = api_dts_uint32_get(DE_DTS_PATH "/dual-output");      
        }
#endif

        is_checked = 1;
    }    
    
    return dual_enable;
}


static int _tv_sys_check_resolution_support(int check_tv_type)
{
	int fd_hdmi = -1; 
    struct hdmi_edidinfo edidinfo = { 0 };
    enum TVTYPE tv_type = TV_LINE_1080_60;
    int i = 0;
    int ret = -1;

    fd_hdmi = open("/dev/hdmi" , O_RDWR);
    if (fd_hdmi < 0)
    	return -1;

    if(ioctl(fd_hdmi , HDMI_TX_GET_EDIDINFO , &edidinfo) < 0)
    {
        printf("Get edid info error\n");
        close(fd_hdmi);
        return -1;
    }
    
    for(i = 0; i < edidinfo.num_tvsys; i++)
    {
        _tvsys_to_tvtype(edidinfo.tvsys[i], &tv_type);
        if (check_tv_type == tv_type)
        {
            printf("Check tvtype support ok.\n");
            ret = 0;
            break;
        }
    }
    close(fd_hdmi);
    return ret;
}


static int _tv_sys_driver_set(enum TVTYPE real_tv_type)
{
	int fd_dis = -1;
	struct dis_tvsys tvsys = { 0 };	
	int dac_tpye = DIS_DAC_CVBS;
	int dual_out;

	fd_dis = open(DEV_DIS , O_WRONLY);
	if(fd_dis < 0) {
		printf("%s(), line:%d, open device: %s error!\n", 
			__func__, __LINE__, DEV_DIS);
		return -1;
	}

	struct dis_tvsys get_tvsys = { 0 };	
	get_tvsys.distype = DIS_TYPE_HD;
	get_tvsys.layer = DIS_LAYER_MAIN;
	ioctl(fd_dis , DIS_GET_TVSYS, &get_tvsys); //HD
	printf("%s(), line:%d, current true TV sys: %d!\n", 
		__FUNCTION__, __LINE__, get_tvsys.tvtype);
	if (get_tvsys.tvtype == real_tv_type)
	{
		printf("%s(), line:%d, real_tv_type: %d. same tv sys, not need set!!!\n", 
			__FUNCTION__, __LINE__, real_tv_type);
		return 0;	
	}

	tvsys.distype = DIS_TYPE_HD;
	tvsys.layer = DIS_LAYER_MAIN;//1;
	tvsys.tvtype = real_tv_type;//TV_LINE_1080_25;
	switch (real_tv_type)
	{
	case TV_LINE_720_25:
	case TV_LINE_720_30:
	case TV_LINE_1080_50:
	case TV_LINE_1080_60:
	case TV_LINE_4096X2160_30:
	case TV_LINE_3840X2160_30:
		tvsys.progressive = 1;
		break;
	default: 
		break;
	}

	dual_out = _tv_sys_get_dual_output_enable();
	if (dual_out) {
        _unreg_dac(dac_tpye, fd_dis);
		ioctl(fd_dis , DIS_SET_TVSYS, &tvsys); //HD
        _sd_set_tvsys(&tvsys, fd_dis); //SD
        _reg_dac(dac_tpye, fd_dis);

	} else {
        _unreg_vga_dac();
		printf("%s(), line: %d, single output set TV sys: %d\n", __FUNCTION__, __LINE__, tvsys.tvtype);
		ioctl(fd_dis , DIS_SET_TVSYS, &tvsys);
        _reg_vga_dac();
	}

#ifdef KEYSTONE_SUPPORT
	//tv system is changed, it may adjust the
	//keystone parametes
	keystone_adjust();
#endif	

    _tv_sys_fb_scale_out(fd_dis);
	close(fd_dis);

	return 0;
}

/**
* @brief       set tv system 
* @param[in]   set_tv_type: tv system to set
* @param[in]   auto_set: true, set tv system by EDID
* @param[in]   match_set: true, only set tv system that hdmi tv support
* @param[in]   timeout: get edid timeout
* @retval      < 0       fail
* @retval      other   set real tv system
*/
static int _tv_sys_set(int set_tv_type, bool auto_set, bool match_set, uint32_t timeout)
{
	int ret;
	enum TVTYPE tv_type = TV_LINE_1080_60;
	enum TVTYPE real_tv_type = 0;
	printf("%s(), line:%d, set_tv_type: %d, match_set: %d!\n", 
		__FUNCTION__, __LINE__, set_tv_type, match_set);

    tv_type = _tv_sys_edid_get(timeout);
	if ((int)tv_type < 0){
		return -1;
	}

	if (auto_set){
		real_tv_type = tv_type;
	} else {
	    if (_tv_sys_check_resolution_support(set_tv_type) < 0){
	    	if (match_set)
	    		return -1;

	        real_tv_type = tv_type; 
	    } else{
	        real_tv_type = set_tv_type;
	    }
	}

    ret = _tv_sys_driver_set(real_tv_type);
    if (ret)
    	return -1;

	return real_tv_type;
}



//Set TV system, if TV can not support the tv system, return API_FAILURE.
//return API_SUCCESS if set/change TV sys OK
int tv_sys_app_set(int app_tv_sys)
{
	int ret;
	int convert_tv_type = TV_LINE_1080_60;
	bool auto_set = false;

	if (APP_TV_SYS_AUTO == app_tv_sys)
		auto_set = true;
	convert_tv_type = tv_sys_app_sys_to_de_sys(app_tv_sys);

	ret = _tv_sys_set(convert_tv_type, auto_set, true, 2000);
	if (ret < 0)
		return API_FAILURE;
	else
		return API_SUCCESS;
}


#if 0
static void tv_sys_force_set()
{
	int fd_dis = -1;
	int dac_tpye = DIS_DAC_CVBS;
	struct dis_tvsys tvsys = { 0 };	
	
	fd_dis = open(DEV_DIS , O_WRONLY);
	if (fd_dis < 0)
		return;

	tvsys.distype = DIS_TYPE_HD;
	tvsys.layer = DIS_LAYER_MAIN;//1;
	tvsys.tvtype = TV_LINE_1080_60;
	switch (tvsys.tvtype)
	{
	case TV_LINE_720_25:
	case TV_LINE_720_30:
	case TV_LINE_1080_50:
	case TV_LINE_1080_60:
	case TV_LINE_4096X2160_30:
	case TV_LINE_3840X2160_30:
		tvsys.progressive = 1;
		break;
	default:
		break;
	}

	int dual_out;
	dual_out = _tv_sys_get_dual_output_enable();
	if (dual_out) {
        _unreg_dac(dac_tpye, fd_dis);
		ioctl(fd_dis , DIS_SET_TVSYS, &tvsys); //HD
        _sd_set_tvsys(&tvsys, fd_dis); //SD
        _reg_dac(dac_tpye, fd_dis);

	} else {
        _unreg_vga_dac();
		printf("%s(), line: %d, single output set TV sys: %d\n", __FUNCTION__, __LINE__, tvsys.tvtype);
		ioctl(fd_dis , DIS_SET_TVSYS, &tvsys);
        _reg_vga_dac();
	}

	_tv_sys_fb_scale_out(fd_dis);
	close(fd_dis);
}
#endif


//Auto set TV system, if TV can not support the tv system, auto set the supported TV sys
//(by EDID) return the supported TV sys. In general, the api used in HDMI plug in
int tv_sys_app_auto_set(int app_tv_sys, uint32_t timeout)
{
	int convert_tv_type = TV_LINE_1080_60;
	bool auto_set = false;
	int support_tv_type;

	if (APP_TV_SYS_AUTO == app_tv_sys)
		auto_set = true;
	convert_tv_type = tv_sys_app_sys_to_de_sys(app_tv_sys);

	support_tv_type = _tv_sys_set(convert_tv_type, auto_set, false, timeout);
	if (support_tv_type < 0)
		return -1;
	return tv_sys_de_sys_to_app_sys(support_tv_type);
}


//Auto check if need to reset tv system, it use in bootup application.
//Because the bootloader would not set TV sys by EDID. So application
//must check it, if the TV sys in sys data is less than TV sys in EDID,
//not need to reset TV sys.
int tv_sys_app_start_set(int check)
{
    int ap_tv_sys;
    int ap_tv_sys_ret;

    ap_tv_sys = projector_get_some_sys_param(P_SYS_RESOLUTION);

    if (check){
		enum TVTYPE edid_tv_type = TV_LINE_1080_60;
		enum TVTYPE de_tv_type;
        edid_tv_type = _tv_sys_edid_get(200);
		if ((int)edid_tv_type < 0)
			return -1;

		de_tv_type = projector_get_some_sys_param(P_DE_TV_SYS);

		if (APP_TV_SYS_AUTO == ap_tv_sys){
			if (edid_tv_type == de_tv_type){
				printf("%s(), line: %d, auto mode: de_tv_type=%d, not reset tv sys!\n", 
					__func__, __LINE__, de_tv_type);

				return 0;
			}
		} else {
			if (edid_tv_type >= de_tv_type){
				printf("%s(), line: %d, edid_tv_type=%d, de_tv_type=%d, not reset tv sys!\n", 
					__func__, __LINE__, edid_tv_type, de_tv_type);

				return 0;
			}
		}
	}
	
    ap_tv_sys_ret = tv_sys_app_auto_set(ap_tv_sys, 2000);
    if (ap_tv_sys_ret >= 0){
        if (APP_TV_SYS_AUTO == ap_tv_sys)
            sysdata_app_tv_sys_set(APP_TV_SYS_AUTO);
        else
            sysdata_app_tv_sys_set(ap_tv_sys_ret);

        projector_sys_param_save();
    }

    return 0;
}


