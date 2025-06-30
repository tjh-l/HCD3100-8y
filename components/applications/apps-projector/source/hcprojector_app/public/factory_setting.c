
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h> //uint32_t
#include <stdlib.h>
#include <pthread.h>
#include "app_config.h"
//#include <hcuapi/sysdata.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/dis.h>
#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif
#include "dummy_api.h"
#include "screen.h"
#include "factory_setting.h"
#include "com_api.h"
#include "tv_sys.h"
#include "osd_com.h"
#include "os_api.h"
#include <semaphore.h>

#ifdef __HCRTOS__
#include <kernel/lib/fdt_api.h>
#endif

#define NODE_ID_PROJECTOR  PERSISTENTMEM_NODE_ID_CASTAPP
#define MEMORY_CTL_MSG_COUNT   100

static sys_param_t sys_param;
static sys_param_t sys_param_bk;
static play_info_t	save_txt_info_tmp;

sem_t sem_projector_memory_save;
static uint32_t m_memory_save_msg_id = INVALID_ID;

static pthread_mutex_t m_sys_rw_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile bool m_sys_rw_enable = true;

static int projector_memory_sys_send_msg(memory_msg_t *control_msg);
static int projector_memory_sys_receive_msg(memory_msg_t *control_msg);
static int projector_memory_save(struct persistentmem_node *node);

extern void api_enter_flash_rw(void);
extern void api_leave_flash_rw(void);

static int _sys_get_sysdata(struct sysdata *sysdata)
{
	int fd;
	struct persistentmem_node node;

	fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
	if (fd < 0) {
		printf("open /dev/persistentmem failed\n");
		return -1;
	}

	node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
	node.offset = 0;
	node.size = sizeof(struct sysdata);
	node.buf = sysdata;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

void projector_factory_init(void)
{
    int level;
    struct sysdata sysdata; // backup

    memcpy(&sysdata, &sys_param.sys_data, sizeof(struct sysdata));

#ifdef BLUETOOTH_SUPPORT		
	struct bluetooth_slave_dev dev;
	memcpy(&dev,&sys_param.app_data.soundset.bt_dev,  sizeof(struct bluetooth_slave_dev));
#endif	
    memset(&sys_param, 0, sizeof(sys_param));
    memcpy(&sys_param.sys_data, &sysdata,  sizeof(struct sysdata));
#ifdef BLUETOOTH_SUPPORT			
	memcpy(&sys_param.app_data.soundset.bt_dev, &dev, sizeof(struct bluetooth_slave_dev));
#endif

    /* Boot parameters init */
    //sys_param.sys_data.volume = 50;
    sys_param.sys_data.tvtype = TV_LINE_800X480_60;
    sys_param.sys_data.ota_detect_modes = HCFOTA_REBOOT_OTA_DETECT_NONE;
	sys_param.app_data.vmotor_count = 0;
#ifdef VMOTOR_LIMIT_ON
	sys_param.app_data.vmotor_limit_single = -1;
#endif
    /* Projector parameters init */
	sys_param.app_data.volume = 50;
	sys_param.app_data.cvbs_gain = 50;
#ifdef  MAIN_PAGE_SUPPORT	
	sys_param.app_data.cur_channel = SCREEN_CHANNEL_MAIN_PAGE;
#else
    sys_param.app_data.cur_channel = SCREEN_CHANNEL_MP;
#endif	
    sys_param.sys_data.flip_mode = FLIP_MODE_REAR;
    sys_param.app_data.pictureset.picture_mode = PICTURE_MODE_STANDARD;
	sys_param.app_data.pictureset.contrast = 50;
	sys_param.app_data.pictureset.brightness = 50;
	sys_param.app_data.pictureset.sharpness = 5;
	sys_param.app_data.pictureset.color = 50;
	sys_param.app_data.pictureset.hue = 50;
	sys_param.app_data.pictureset.color_temp = COLOR_TEMP_STANDARD;
	sys_param.app_data.pictureset.noise_redu = NOISE_REDU_OFF;

	sys_param.app_data.soundset.sound_mode = SOUND_MODE_STANDARD;
	sys_param.app_data.soundset.balance = 0;
	sys_param.app_data.soundset.bt_setting = BLUETOOTH_OFF;

#ifdef SUPPORT_STARTUP_PAGE_LANGUAGE
	sys_param.app_data.is_startup = 1;
#endif
	sys_param.app_data.soundset.treble = 0;
	sys_param.app_data.soundset.bass = 0;
	int eq_gains[EQ_MODE_MAX][EQ_BAND_LEN] = {
		{24,0,0,0,0,0,0,0,0,24},
		{40,36,32,24,8,-8,8,24,32,40},
		{-8,0,16,24,32,40,24,8,0,-16},
		{32,24,16,8,0,-16,0,16,32,40},
		{40,32,24,8,-16,8,24,32,32,40},
		{0,-16,-12,-8,8,32,28,24,20,16},
		{0,0,0,0,0,0,0,0,0,0}
	};
	for(int i = 0; i < EQ_MODE_MAX; i++){
		for(int j= 0; j < EQ_BAND_LEN; j++){
			sys_param.app_data.soundset.eq_gain[i][j] = eq_gains[i][j];
		}
		//memset(sys_param.app_data.soundset.eq_gain[i], 0, sizeof(int8_t)*EQ_BAND_LEN);
	}
	sys_param.app_data.soundset.eq_enable = 0;
	sys_param.app_data.soundset.eq_mode = EQ_MODE_NORMAL;
#ifdef BLUETOOTH_SUPPORT	
	
	memset(&sys_param.app_data.soundset.bt_dev, 0 , sizeof(struct bluetooth_slave_dev));
#endif	
#ifdef DEFAULT_OSD_ENGLISH	
	sys_param.app_data.optset.osd_language = LANGUAGE_ENGLISH;	
#else
    //sys_param.app_data.optset.osd_language = LANGUAGE_CHINESE;
#endif 		
	sys_param.app_data.optset.aspect_ratio = DIS_TV_AUTO;
	sys_param.app_data.optset.auto_sleep = AUTO_SLEEP_OFF;
	sys_param.app_data.optset.osd_time = OSD_TIME_15S;
	sys_param.app_data.optset.video_delay = 500;
#ifdef AUTOKEYSTONE_SWITCH
	sys_param.app_data.auto_keystone = 0;
	#if 0//def ATK_CALIBRATION
	sys_param.app_data.atk_calibration = 0;
	#endif
#endif

    //sys_param.sys_data.tvtype = TV_LINE_1080_60;
    //sys_param.sys_data.volume = 70;
	//cast setting
#ifdef WIFI_SUPPORT	
	memset(sys_param.app_data.cast_setting.wifi_ap, 0, sizeof(hccast_wifi_ap_info_t)*MAX_WIFI_SAVE);
	sys_param.app_data.cast_setting.wifi_onoff = 0;
	sys_param.app_data.cast_setting.wifi_auto_conn = 0;
#endif	
#ifdef DEFAULT_OSD_ENGLISH	
	sys_param.app_data.cast_setting.browserlang = 1;//1-englist;2-chn;3-traditional chn
#else
	sys_param.app_data.cast_setting.browserlang = 2;//1-englist;2-chn;3-traditional chn
	//sys_param.app_data.cast_setting.browserlang = 3;//1-englist;2-chn;3-traditional chn
#endif
	sys_param.app_data.cast_setting.mirror_frame = 1;
	sys_param.app_data.cast_setting.mirror_mode = 1;
	sys_param.app_data.cast_setting.aircast_mode = 1;//mirror-only.
	sys_param.app_data.resolution = APP_TV_SYS_AUTO;//APP_TV_SYS_1080P;
	sys_param.app_data.cast_setting.mirror_rotation = 0;//default disable.
	sys_param.app_data.cast_setting.mirror_vscreen_auto_rotation = 1;//default enable.
	sys_param.app_data.cast_setting.mirror_full_vscreen = 1;//default enable.
	sys_param.app_data.cast_setting.um_full_screen = 1;//default enable.

    sys_param.app_data.cast_setting.wifi_mode = 2; // 1: 2.4G, 2: 5G, 3: 60G (res)
    sys_param.app_data.cast_setting.wifi_ch   = HOSTAP_CHANNEL_24G;
    sys_param.app_data.cast_setting.wifi_ch5g = HOSTAP_CHANNEL_5G;
    sys_param.app_data.cast_setting.airp2p_ch = 149;

	sys_param.app_data.cast_setting.cast_dev_name_changed = 0;
	memset(sys_param.app_data.cast_setting.cast_dev_name, 0,MAX_DEV_NAME );
	snprintf(sys_param.app_data.cast_setting.cast_dev_psk,MAX_DEV_PSK,"%s",DEVICE_PSK);
#ifdef SYS_ZOOM_SUPPORT
	sys_param.app_data.scale_setting.dis_mode = DIS_TV_AUTO;
	sys_param.app_data.scale_setting.zoom_out_count = 0;
#endif

#ifdef HC_FACTORY_TEST

	for(int i=0;i<MAX_FACTORYSET_CHANNEL;i++){
		sys_param.app_data.factory_setting.picset[i].picture_mode = PICTURE_MODE_STANDARD;
		sys_param.app_data.factory_setting.picset[i].contrast = 50;
		sys_param.app_data.factory_setting.picset[i].brightness = 50;
		sys_param.app_data.factory_setting.picset[i].sharpness = 5;
		sys_param.app_data.factory_setting.picset[i].color = 50;
		sys_param.app_data.factory_setting.picset[i].hue = 50;
		sys_param.app_data.factory_setting.picset[i].color_temp = COLOR_TEMP_STANDARD;
		sys_param.app_data.factory_setting.picset[i].noise_redu = NOISE_REDU_OFF;
	}
	sys_param.app_data.factory_setting.channel_select=SCREEN_CHANNEL_HDMI;
	sys_param.app_data.factory_setting.pq_onoff=1;
	sys_param.app_data.factory_setting.dyn_contrast_onoff=1;
	sys_param.app_data.factory_setting.cvbs_aef=128;
#endif		
#ifdef HC_MEDIA_MEMMORY_PLAY
	memset((uint8_t*)&sys_param.app_data.save_video_info,0x00,sizeof(play_info_data));
	memset((uint8_t*)&sys_param.app_data.save_txt_info,0x00,sizeof(play_info_data));
	memset((uint8_t*)&sys_param.app_data.save_music_info,0x00,sizeof(play_info_t));
#endif

#ifdef HCIPTV_YTB_SUPPORT
	snprintf(sys_param.app_data.ytb_app_config.iptv_config.region, sizeof(sys_param.app_data.ytb_app_config.iptv_config.region), "US");
	snprintf(sys_param.app_data.ytb_app_config.iptv_config.page_max, sizeof(sys_param.app_data.ytb_app_config.iptv_config.page_max), "%d", YTB_PAGE_CACHA_NUM);
	sys_param.app_data.ytb_app_config.iptv_config.quality_option = HCCAST_IPTV_VIDEO_1080P;
	sys_param.app_data.ytb_app_config.iptv_config.search_option = HCCAST_IPTV_SEARCH_ORDER_RELEVANCE;
	// snprintf(sys_param.app_data.ytb_app_config.iptv_config.service_addr, sizeof(sys_param.app_data.ytb_app_config.iptv_config.service_addr), "%s", "");
#endif

	sys_param.app_data.mira_param.continue_on_error = 2; // 2: show masaic, 1: no show
	
#ifdef HDMI_RX_CEC_SUPPORT
	sys_param.app_data.cec_info.on = 1;	
	sys_param.app_data.cec_info.dev_la = 15;
#endif

    if(api_get_backlight_default_brightness_level((uint8_t *)&level))
        level = 100;
    sys_param.sys_data.lcd_pwm_backlight = level;
}

/* factory reset  */
void projector_factory_reset(void)
{
    int fd;
    int level;
    int node_id;
    struct persistentmem_node node;

    printf("%s(), line:%d.\n", __func__, __LINE__);
    fd = open("/dev/persistentmem", O_SYNC | O_RDWR);
    if (fd < 0) {
        printf("Open /dev/persistentmem failed (%d)\n", fd);
        return;
    }

    node_id = NODE_ID_PROJECTOR;
    if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_DELETE, node_id) < 0) {
        printf("%s(), line:%d. delete app data failed\n", __func__, __LINE__);
    }
    close(fd);

#ifdef SYS_ZOOM_SUPPORT
    sysdata_disp_rect_save(0, 0, 1920, 1080);
#endif
    if(api_get_backlight_default_brightness_level((uint8_t *)&level))
        level = 100;
    sys_param.sys_data.lcd_pwm_backlight = level;
    sys_param.sys_data.flip_mode = FLIP_MODE_REAR;
    node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
    node.offset = 0;
    node.size = sizeof(struct sysdata);
    node.buf = &sys_param.sys_data;
    projector_memory_save(&node);
    usleep(100*1000);
}

static int sys_get_sys_data(struct sysdata *sys_data)
{
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
	int fd;

	fd = open("/dev/persistentmem", O_RDWR);
	if (fd < 0) {
		printf("Open /dev/persistentmem failed (%d)\n", fd);
		return -1;
	}

    node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
    node.offset = 0;
    node.size = sizeof(struct sysdata);
	node.buf = sys_data;

	api_enter_flash_rw();

	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
        new_node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
        new_node.size = sizeof(struct sysdata);
        if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
			api_leave_flash_rw();
            printf("create sys_data failed\n");
            close(fd);
            return -1;
        }
        api_leave_flash_rw();
        close(fd);

        node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
        node.offset = 0;
        new_node.size = sizeof(struct sysdata);
        node.buf = sys_data;
        projector_memory_save(&node);
        
        return 0;
    }
	api_leave_flash_rw();
	close(fd);
	return 0;

}


/* Load factory setting of system */
int projector_sys_param_load(void)
{
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
    int fd;

    sys_get_sys_data(&sys_param.sys_data);
    memcpy(&sys_param_bk.sys_data, &sys_param.sys_data, sizeof(struct sysdata));

	fd = open("/dev/persistentmem", O_RDWR);
	if (fd < 0) {
		printf("Open /dev/persistentmem failed (%d)\n", fd);
		return -1;
	}

	node.id = NODE_ID_PROJECTOR;
	node.offset = 0;
	node.size = sizeof(struct appdata);
	node.buf = &sys_param.app_data;
	api_enter_flash_rw();

	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
        new_node.id = NODE_ID_PROJECTOR;
        new_node.size = sizeof(struct appdata);
        if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
            printf("get sys_data failed\n");
            api_leave_flash_rw();
            close(fd);
            return -1;
        }
        close(fd);
        api_leave_flash_rw();

        node.id = NODE_ID_PROJECTOR;
        node.offset = 0;
        node.size = sizeof(struct appdata);
        node.buf = &sys_param.app_data;
        projector_memory_save(&node);
        memcpy(&sys_param_bk.app_data, &sys_param.app_data, sizeof(struct appdata));
        
        return 0;
    }
    api_leave_flash_rw();

    memcpy(&sys_param_bk.app_data, &sys_param.app_data, sizeof(struct appdata));
	close(fd);
	return 0;
}

/* store projector system parameters to flash */
int projector_sys_param_save(void)
{
    struct persistentmem_node node;
    
    node.id = PERSISTENTMEM_NODE_ID_SYSDATA;
    node.offset = 0;
    node.size = sizeof(struct sysdata);
    node.buf = &sys_param.sys_data;
    projector_memory_save(&node);
    node.id = NODE_ID_PROJECTOR;
    node.offset = 0;
    node.size = sizeof(struct appdata);
    node.buf = &sys_param.app_data;
    projector_memory_save(&node);

    return 0;
}

sys_param_t * projector_get_sys_param(void)
{
    return &sys_param;
}


#ifdef BLUETOOTH_SUPPORT
unsigned char* projector_get_bt_mac(int i){
	if(i>=0 && i<MAX_BT_SAVE){
		return sys_param.app_data.soundset.bt_dev[i].mac;
	}

	return NULL;
}

char* projector_get_bt_name(int i){
	if(i>=0 && i < MAX_BT_SAVE){
		return sys_param.app_data.soundset.bt_dev[i].name;
	}
	return NULL;
}

struct bluetooth_slave_dev * projector_get_bt_by_mac(char* mac){
	int i = 0;
	for(; i< MAX_BT_SAVE; i++){
		if(memcmp(mac, sys_param.app_data.soundset.bt_dev[i].mac, 6) == 0){
			return sys_param.app_data.soundset.bt_dev + i;
		}
	}
	return NULL;
}

struct bluetooth_slave_dev * projector_get_bt_by_name(char* name){
	int i = 0;
	for(; i< MAX_BT_SAVE; i++){
		if(strncmp(name, sys_param.app_data.soundset.bt_dev[i].name, BLUETOOTH_NAME_LEN) == 0){
			return sys_param.app_data.soundset.bt_dev + i;
		}
	}
	return NULL;
}

struct bluetooth_slave_dev *projector_get_bt_dev(int i){
	if(i>=0 && i < MAX_BT_SAVE && strlen(sys_param.app_data.soundset.bt_dev[i].name) > 0){
		return sys_param.app_data.soundset.bt_dev+i;
	}
	return NULL;
}

void projector_save_bt_dev(struct bluetooth_slave_dev *dev){
	if(!dev){
		return;
	}
    int8_t i = 0;
	for(; i< MAX_BT_SAVE; i++){
		if(memcmp(dev->mac, sys_param.app_data.soundset.bt_dev[i].mac, 6) == 0){
			projector_bt_dev_delete(i);
		}
	}
	i = 0;
    for(i = MAX_BT_SAVE-2; i >= 0; i--){
        memcpy(sys_param.app_data.soundset.bt_dev+i+1, sys_param.app_data.soundset.bt_dev+i, sizeof(struct bluetooth_slave_dev));
    }
	memcpy(sys_param.app_data.soundset.bt_dev, dev, sizeof(struct bluetooth_slave_dev));
}

void projector_bt_dev_delete(int index){
    int i = 0;

    if(index > MAX_BT_SAVE-1)
    {
        return;
    }

    for(i = index; i < MAX_BT_SAVE-1; i++)
    {
        memcpy(sys_param.app_data.soundset.bt_dev + i, sys_param.app_data.soundset.bt_dev+i+1, sizeof(struct bluetooth_slave_dev));
    }
    memset(sys_param.app_data.soundset.bt_dev+MAX_BT_SAVE-1,0x00,sizeof(struct bluetooth_slave_dev));	
}

int projector_get_saved_bt_count(){
	int i=0;
	for(; MAX_BT_SAVE>i && strlen(sys_param.app_data.soundset.bt_dev[i].name)>0;i++);
	return i;
}
#endif

char* projector_get_version_info(void){
	static char version_info_v[32] = {0};
	#ifdef LVGL_RESOLUTION_240P_SUPPORT
		unsigned int ver = (unsigned int)sys_param.sys_data.firmware_version;
		snprintf(version_info_v, sizeof(version_info_v), "%u.%u.%u", ver/100000000, (ver%100000000)/1000000, (ver%1000000)/10000);
	#else
		snprintf(version_info_v, sizeof(version_info_v), "%s-%u", sys_param.sys_data.product_id, (unsigned int)sys_param.sys_data.firmware_version);
	#endif

	return version_info_v;
}

 int projector_get_some_sys_param(projector_sys_param param){
	switch (param){
		case P_PICTURE_MODE:
			return sys_param.app_data.pictureset.picture_mode;
		case P_CONTRAST:
			return sys_param.app_data.pictureset.contrast;
		case P_BRIGHTNESS:
			return sys_param.app_data.pictureset.brightness;
		case P_SHARPNESS:
			return sys_param.app_data.pictureset.sharpness;
		case P_COLOR:
			return sys_param.app_data.pictureset.color;
		case P_HUE:
			return sys_param.app_data.pictureset.hue;
		case P_COLOR_TEMP:
			return sys_param.app_data.pictureset.color_temp;
		case P_NOISE_REDU:
			return sys_param.app_data.pictureset.noise_redu;
		case P_BACKLIGHT:
			return sys_param.sys_data.lcd_pwm_backlight;
		case P_SOUND_MODE:
			return sys_param.app_data.soundset.sound_mode;
		case P_BALANCE:
			return sys_param.app_data.soundset.balance;
		case P_BT_SETTING:
			return sys_param.app_data.soundset.bt_setting;
		case P_TREBLE:
			return sys_param.app_data.soundset.treble;
		case P_BASS:
			return sys_param.app_data.soundset.bass;
		case P_OSD_LANGUAGE:
			return sys_param.app_data.optset.osd_language;
		case P_ASPECT_RATIO:
			return sys_param.app_data.optset.aspect_ratio;
		case P_CUR_CHANNEL:
			return sys_param.app_data.cur_channel;
		case P_FLIP_MODE:
			return sys_param.sys_data.flip_mode;
		case P_VOLUME:
			return sys_param.app_data.volume;
		case P_KEYSTONE_TOP:
			return sys_param.sys_data.keystone_top_w;
		case P_KEYSTONE_BOTTOM:
			return sys_param.sys_data.keystone_bottom_w;
		case P_AUTOSLEEP:
			return sys_param.app_data.optset.auto_sleep;
		case P_OSD_TIME:
			return sys_param.app_data.optset.osd_time;
			break;
		case P_DEV_PRODUCT_ID:
			return (int)(sys_param.sys_data.product_id);
			break;
		case P_DEV_VERSION:
			return (int)(sys_param.sys_data.firmware_version);
			break;
		case P_MIRROR_MODE:
			return sys_param.app_data.cast_setting.mirror_mode;
			break;
		case P_AIRCAST_MODE:
			return sys_param.app_data.cast_setting.aircast_mode;
			break;
		case P_MIRROR_FRAME:
			return sys_param.app_data.cast_setting.mirror_frame;
			break;
		case P_BROWSER_LANGUAGE:
			return sys_param.app_data.cast_setting.browserlang;
			break;
		case P_SYS_RESOLUTION:
			return sys_param.app_data.resolution;
#ifdef WIFI_SUPPORT
		case P_DEVICE_NAME:
		    if (sys_param.app_data.cast_setting.cast_dev_name[0] == 0 && 
		    	sys_param.app_data.cast_setting.cast_dev_name[1] == 0)
		    {
		        sysdata_init_device_name();
		    }
			return (int)sys_param.app_data.cast_setting.cast_dev_name;
			break;
		case P_WIFI_ONOFF:
			return (int)sys_param.app_data.cast_setting.wifi_onoff;
			break;
#endif			
		case P_DEVICE_PSK:
			return (int)sys_param.app_data.cast_setting.cast_dev_psk;
			break;
		case P_WIFI_MODE:
			return sys_param.app_data.cast_setting.wifi_mode;
			break;
		case P_WIFI_CHANNEL:
            if (1 == sys_param.app_data.cast_setting.wifi_mode)
            {
                return sys_param.app_data.cast_setting.wifi_ch;
            }
            else if (2 == sys_param.app_data.cast_setting.wifi_mode)
            {
                return sys_param.app_data.cast_setting.wifi_ch5g;
            }
            return sys_param.app_data.cast_setting.wifi_ch;
            break;
        case P_WIFI_CHANNEL_24G:
            return sys_param.app_data.cast_setting.wifi_ch;
        case P_WIFI_CHANNEL_5G:
            return sys_param.app_data.cast_setting.wifi_ch5g;
		case P_MIRROR_ROTATION:
			return sys_param.app_data.cast_setting.mirror_rotation;
			break;
		case P_MIRROR_VSCREEN_AUTO_ROTATION:
			return sys_param.app_data.cast_setting.mirror_vscreen_auto_rotation;
			break;	
		case P_DE_TV_SYS:
			return sys_param.sys_data.tvtype;
			break;
		case P_VMOTOR_COUNT:
			return sys_param.app_data.vmotor_count;
			break;
#ifdef VMOTOR_LIMIT_ON
		case P_VMOTOR_LIMIT_SINGLE:
			return sys_param.app_data.vmotor_limit_single;
			break;
#endif
#ifdef SYS_ZOOM_SUPPORT
		case P_SYS_ZOOM_DIS_MODE:
			return sys_param.app_data.scale_setting.dis_mode;
			break;
		case P_SYS_ZOOM_OUT_COUNT:
			return sys_param.app_data.scale_setting.zoom_out_count;
			break;
#endif
		case P_SOUND_EQ:
			return sys_param.app_data.soundset.eq_enable;
			break;
		case P_EQ_MODE:
			return sys_param.app_data.soundset.eq_mode;
			break;
#ifdef HC_MEDIA_MEMMORY_PLAY			
		case P_MEM_PLAY_MEDIA_TYPE:
			return sys_param.app_data.save_media_type;
			break;
#endif
		case P_VIDEO_DELAY:
			return sys_param.app_data.optset.video_delay;
			break;
		case P_SOUND_OUT_MODE:
			return sys_param.app_data.soundset.output_mode;
			break;
		case P_SOUND_SPDIF_MODE:
			return sys_param.app_data.soundset.spdif_mode;
			break;
		case P_MIRROR_FULL_VSCREEN:
			return sys_param.app_data.cast_setting.mirror_full_vscreen;
			break;
		case P_CVBS_GAIN:
			return sys_param.app_data.cvbs_gain;
			break;
		case P_MIRA_CONTINUE_ON_ERROR:
			return sys_param.app_data.mira_param.continue_on_error;
			break;
		case P_UM_FULL_SCREEN:
			return sys_param.app_data.cast_setting.um_full_screen;
			break;
		case P_AIRP2P_CH:
			return sys_param.app_data.cast_setting.airp2p_ch;
			break;
#ifdef HDMI_RX_CEC_SUPPORT
		case P_CEC_ONOFF:
			return sys_param.app_data.cec_info.on;
			break;
		case P_CEC_DEVICE_LA:
			return sys_param.app_data.cec_info.dev_la;
			break;
#endif
#ifdef AUTOKEYSTONE_SWITCH
		case P_AUTO_KEYSTONE:
	        return sys_param.app_data.auto_keystone;
		 break;			
	#if 0//def ATK_CALIBRATION
	case P_ATK_CALIBRATION:
	        return sys_param.app_data.atk_calibration;
		 break;
	#endif
#endif
#ifdef SUPPORT_STARTUP_PAGE_LANGUAGE
		case P_SYS_IS_STARTUP:
	        return sys_param.app_data.is_startup;
		 break;	
#endif
		default:
			break;
	}
	return -1;
}

void projector_set_some_sys_param(projector_sys_param param, int v){
	switch (param){
		case P_PICTURE_MODE:
			sys_param.app_data.pictureset.picture_mode = v;
			break;
		case P_CONTRAST:
			sys_param.app_data.pictureset.contrast = v;
			break;
		case P_BRIGHTNESS:
			sys_param.app_data.pictureset.brightness = v;
			break;
		case P_SHARPNESS:
			sys_param.app_data.pictureset.sharpness = v;
			break;
		case P_COLOR:
			sys_param.app_data.pictureset.color = v;
			break;
		case P_HUE:
			sys_param.app_data.pictureset.hue = v;
			break;
		case P_COLOR_TEMP:
			sys_param.app_data.pictureset.color_temp = v;
			break;
		case P_NOISE_REDU:
			sys_param.app_data.pictureset.noise_redu = v;
			break;
		case P_BACKLIGHT:
			sys_param.sys_data.lcd_pwm_backlight = v;
			break;
		case P_SOUND_MODE:
			sys_param.app_data.soundset.sound_mode = v;
			break;
		case P_BALANCE:
			sys_param.app_data.soundset.balance = v;
			break;
		case P_BT_SETTING:
			sys_param.app_data.soundset.bt_setting = v;
			break;
		case P_TREBLE:
			sys_param.app_data.soundset.treble = v;
			break;
		case P_BASS:
			sys_param.app_data.soundset.bass = v;
			break;
		case P_OSD_LANGUAGE:
			sys_param.app_data.optset.osd_language = v;
			break;
		case P_ASPECT_RATIO:
			sys_param.app_data.optset.aspect_ratio = v;
			break;
		case P_CUR_CHANNEL:
			sys_param.app_data.cur_channel = v;
			break;
		case P_FLIP_MODE:
			sys_param.sys_data.flip_mode = v;
			break;
		case P_VOLUME:
			sys_param.app_data.volume = v;
			break;
		case P_KEYSTONE_TOP:
			sys_param.sys_data.keystone_top_w = v;
			break;
		case P_KEYSTONE_BOTTOM:
			sys_param.sys_data.keystone_bottom_w = v;
			break;
		case P_AUTOSLEEP:
			sys_param.app_data.optset.auto_sleep = v;
			break;
		case P_OSD_TIME:
			sys_param.app_data.optset.osd_time = v;
			break;
		case P_MIRROR_MODE:
			sys_param.app_data.cast_setting.mirror_mode = v;
			break;
		case P_AIRCAST_MODE:
			sys_param.app_data.cast_setting.aircast_mode = v;
			break;
		case P_MIRROR_FRAME:
			sys_param.app_data.cast_setting.mirror_frame = v;
			break;
		case P_BROWSER_LANGUAGE:
			sys_param.app_data.cast_setting.browserlang = v;
			break;
		case P_SYS_RESOLUTION:
			sys_param.app_data.resolution = v;
			break;
		case P_DEVICE_NAME:
			sys_param.app_data.cast_setting.cast_dev_name_changed = 1;
			strncpy(sys_param.app_data.cast_setting.cast_dev_name, (char*)v, MAX_DEV_NAME);
			break;
		case P_DEVICE_PSK:
			strncpy(sys_param.app_data.cast_setting.cast_dev_psk, (char*)v, MAX_DEV_PSK);
			break;
		case P_WIFI_MODE:
			sys_param.app_data.cast_setting.wifi_mode = v;
			break;
		case P_WIFI_CHANNEL:
            if (v >= 34) // 5G
            {
                sys_param.app_data.cast_setting.wifi_ch5g = v;
            }
            else
            {
                sys_param.app_data.cast_setting.wifi_ch = v;
            }
			break;
		case P_MIRROR_ROTATION:
			sys_param.app_data.cast_setting.mirror_rotation = v;
			break;
		case P_MIRROR_VSCREEN_AUTO_ROTATION:
			sys_param.app_data.cast_setting.mirror_vscreen_auto_rotation = v;
			break;
		case P_VMOTOR_COUNT:
			sys_param.app_data.vmotor_count = v;
			break;
#ifdef VMOTOR_LIMIT_ON
		case P_VMOTOR_LIMIT_SINGLE:
			sys_param.app_data.vmotor_limit_single = v;
			break;
#endif
#ifdef WIFI_SUPPORT
		case P_WIFI_ONOFF:
			if(v!=0 && v!= 1){
				break;
			}
			sys_param.app_data.cast_setting.wifi_onoff = v;
			break;
#endif
#ifdef SYS_ZOOM_SUPPORT
		case P_SYS_ZOOM_DIS_MODE:
		 	sys_param.app_data.scale_setting.dis_mode = v;
			break;
		case P_SYS_ZOOM_OUT_COUNT:
			sys_param.app_data.scale_setting.zoom_out_count = v;
			break;
#endif
		case P_SOUND_EQ:
			sys_param.app_data.soundset.eq_enable = v;
			break;
		case P_EQ_MODE:
			sys_param.app_data.soundset.eq_mode = v;
			break;
#ifdef HC_MEDIA_MEMMORY_PLAY			
		case P_MEM_PLAY_MEDIA_TYPE:
			sys_param.app_data.save_media_type = v;
			break;
#endif
        	case P_VIDEO_DELAY:
        		sys_param.app_data.optset.video_delay = (uint16_t)v;
        		break;
		case P_SOUND_OUT_MODE:
			sys_param.app_data.soundset.output_mode = v;
			break;
		case P_SOUND_SPDIF_MODE:
			sys_param.app_data.soundset.spdif_mode = v;
			break;
		case P_MIRROR_FULL_VSCREEN:
			sys_param.app_data.cast_setting.mirror_full_vscreen = v;
			break;
		case P_CVBS_GAIN:
			sys_param.app_data.cvbs_gain = v;
			break;
		case P_UM_FULL_SCREEN:
			sys_param.app_data.cast_setting.um_full_screen = v;
			break;	
		case P_AIRP2P_CH:
			sys_param.app_data.cast_setting.airp2p_ch = v;
			break;
#ifdef HDMI_RX_CEC_SUPPORT
		case P_CEC_ONOFF:
			sys_param.app_data.cec_info.on= v;
			break;
		case P_CEC_DEVICE_LA:
			sys_param.app_data.cec_info.dev_la = v;
			break;
#endif
#ifdef AUTOKEYSTONE_SWITCH
		case P_AUTO_KEYSTONE:
			sys_param.app_data.auto_keystone = v;
		 	break;
	#if 0//def ATK_CALIBRATION
        case P_ATK_CALIBRATION:
             sys_param.app_data.atk_calibration = v;
             break;
	#endif
#endif
#ifdef SUPPORT_STARTUP_PAGE_LANGUAGE
		case P_SYS_IS_STARTUP:
	        sys_param.app_data.is_startup = v;
		 break;		
#endif
		default:
			break;
	}
}

#ifdef SYS_ZOOM_SUPPORT
void sysdata_disp_rect_save(int x, int y, int w, int h)
{
	if(x < 0 || x > 1920 &&
	   y < 0 || y > 1080 &&
	   w < 0 || w > 1920 &&
	   h < 0 || h > 1080){
		return;
	}
	sys_param.sys_data.disp_rect_x = x;
	sys_param.sys_data.disp_rect_y = y;
	sys_param.sys_data.disp_rect_w = w;
	sys_param.sys_data.disp_rect_h = h;
}
#endif

#ifdef HC_FACTORY_TEST

static int sys_channel_2_factory_idx(int sys_channel)
{
	int factory_idx = FACTORY_CVBS_IDX;

	switch (sys_channel){
	case SCREEN_CHANNEL_CVBS:
		factory_idx = FACTORY_CVBS_IDX;
		break;
	case SCREEN_CHANNEL_HDMI:
#ifdef HDMI_SWITCH_SUPPORT
	case SCREEN_CHANNEL_HDMI2:
#endif	
		factory_idx = FACTORY_HDMIRX_IDX;
		break;
	case SCREEN_CHANNEL_MP:
		factory_idx = FACTORY_MP_IDX;
		break;
	default:
		factory_idx = FACTORY_CVBS_IDX;
		break;
	}

	return factory_idx;
}


void factory_set_some_sys_param(projector_sys_param param, int v)
{
	int idx = 0;

	idx = sys_channel_2_factory_idx(sys_param.app_data.cur_channel);

	if(idx >= MAX_FACTORYSET_CHANNEL){
		idx = MAX_FACTORYSET_CHANNEL - 1;
		/*limit it to avoid segment fault*/ 
	}
	switch (param){
		case P_PICTURE_MODE:
			sys_param.app_data.factory_setting.picset[idx].picture_mode = v;
			break;
		case P_CONTRAST:
			sys_param.app_data.factory_setting.picset[idx].contrast = v;
			break;
		case P_BRIGHTNESS:
			sys_param.app_data.factory_setting.picset[idx].brightness = v;
			break;
		case P_SHARPNESS:
			sys_param.app_data.factory_setting.picset[idx].sharpness = v;
			break;
		case P_COLOR:
			sys_param.app_data.factory_setting.picset[idx].color = v;
			break;
		case P_HUE:
			sys_param.app_data.factory_setting.picset[idx].hue = v;
			break;
		case P_COLOR_TEMP:
			sys_param.app_data.factory_setting.picset[idx].color_temp = v;
			break;
		case P_NOISE_REDU:
			sys_param.app_data.factory_setting.picset[idx].noise_redu = v;
			break;
		case P_CUR_CHANNEL:
			if(v>MAX_FACTORYSET_CHANNEL)
				v=MAX_FACTORYSET_CHANNEL;
			sys_param.app_data.cur_channel=v;
			break;
		case P_PQ_ONFF:
			sys_param.app_data.factory_setting.pq_onoff=v;
			break;
		case P_DYN_CONTRAST_ONOFF:
			sys_param.app_data.factory_setting.dyn_contrast_onoff=v;
			break;
		case P_CVBS_AEF_OFFSET:
			sys_param.app_data.factory_setting.cvbs_aef=v;
			break;
		default: 
			break;
	}
}

/* get factory sys param by cur channel*/ 
 int factory_get_some_sys_param(projector_sys_param param)
 {
	int idx = 0;

	idx = sys_channel_2_factory_idx(sys_param.app_data.cur_channel);

	if(idx >= MAX_FACTORYSET_CHANNEL){
		idx = MAX_FACTORYSET_CHANNEL - 1;
		/*limit it to avoid segment fault*/ 
	}

	switch (param){
		case P_PICTURE_MODE:
			return sys_param.app_data.factory_setting.picset[idx].picture_mode;
		case P_CONTRAST:
			return sys_param.app_data.factory_setting.picset[idx].contrast;
		case P_BRIGHTNESS:
			return sys_param.app_data.factory_setting.picset[idx].brightness;
		case P_SHARPNESS:
			return sys_param.app_data.factory_setting.picset[idx].sharpness;
		case P_COLOR:
			return sys_param.app_data.factory_setting.picset[idx].color ;
		case P_HUE:
			return sys_param.app_data.factory_setting.picset[idx].hue ;
		case P_COLOR_TEMP:
			return sys_param.app_data.factory_setting.picset[idx].color_temp ;
		case P_NOISE_REDU:
			return sys_param.app_data.factory_setting.picset[idx].noise_redu;
		case P_CUR_CHANNEL:
			if(sys_param.app_data.cur_channel>MAX_FACTORYSET_CHANNEL){
				return MAX_FACTORYSET_CHANNEL;
			}else {
				return sys_param.app_data.cur_channel;
			}
		case P_PQ_ONFF:
			return sys_param.app_data.factory_setting.pq_onoff;
		case P_DYN_CONTRAST_ONOFF:
			return sys_param.app_data.factory_setting.dyn_contrast_onoff;
		case P_CVBS_AEF_OFFSET:
			return sys_param.app_data.factory_setting.cvbs_aef;
		default: 
			return 0;
	}
 }

#endif

#ifdef WIFI_SUPPORT

static pthread_mutex_t wifi_mutex;
static pthread_mutex_t *wifi_mutex_p=NULL;

static int8_t save_flag = 0;// save_flag == 1, mean called func sysdata_wifi_ap_save(hccast_wifi_ap_info_t *)

void set_save_wifi_flag_zero(void){
	save_flag = 0;
}

int8_t get_save_wifi_flag(void){
	return save_flag;
}

void wifi_mutex_init(void){
	if(!wifi_mutex_p){
		pthread_mutex_init(&wifi_mutex, NULL);
		wifi_mutex_p = &wifi_mutex;
	}
}


int sysdata_check_ap_saved(hccast_wifi_ap_info_t* check_wifi)
{
	int i = 0;
	int index = -1;
	
	for(i = 0; i < MAX_WIFI_SAVE; i++)
	{
		if(strlen(sys_param.app_data.cast_setting.wifi_ap[i].ssid) && strlen(check_wifi->ssid))
		{
			if(strcmp(check_wifi->ssid, sys_param.app_data.cast_setting.wifi_ap[i].ssid) == 0)
			{
				index = i;
				return index;
			}
		}	
	}
	
	return index;
}


void sysdata_wifi_ap_save(hccast_wifi_ap_info_t *wifi_ap)
{
    int8_t i = 0;

    for(i = MAX_WIFI_SAVE-2; i >= 0; i--)
    {
        memcpy(&(sys_param.app_data.cast_setting.wifi_ap[i+1]), &(sys_param.app_data.cast_setting.wifi_ap[i]), \
        	sizeof(hccast_wifi_ap_info_t));
    }
	sys_param.app_data.cast_setting.wifi_auto_conn = sys_param.app_data.cast_setting.wifi_auto_conn>>1;
	sys_param.app_data.cast_setting.wifi_auto_conn = sys_param.app_data.cast_setting.wifi_auto_conn | (0x8000);//
    memcpy(sys_param.app_data.cast_setting.wifi_ap, wifi_ap, sizeof(hccast_wifi_ap_info_t));
	save_flag = 1;
	printf("auto_conn save: %x\n", sys_param.app_data.cast_setting.wifi_auto_conn);
}

void sysdata_wifi_ap_resave(hccast_wifi_ap_info_t *wifi_ap){
    pthread_mutex_lock(wifi_mutex_p);
    int index = sysdata_check_ap_saved(wifi_ap);
    printf("ssid index: %d\n",index);
    if(index >= 0)//set the index ap to first.
    {
        sysdata_wifi_ap_delete(index);
    }

    sysdata_wifi_ap_save(wifi_ap);
    projector_sys_param_save();
    pthread_mutex_unlock(wifi_mutex_p);
}

void sysdata_wifi_ap_swap(int index1,int index2)
{
    if(index1<0 && index1>=MAX_WIFI_SAVE && index2<0 && index2>=MAX_WIFI_SAVE){
        return;
    }
    if(sysdata_wifi_ap_get_auto(index1) != sysdata_wifi_ap_get_auto(index2))
    {
        sys_param.app_data.cast_setting.wifi_auto_conn = sys_param.app_data.cast_setting.wifi_auto_conn ^ (0x8000 >> index1);
        sys_param.app_data.cast_setting.wifi_auto_conn = sys_param.app_data.cast_setting.wifi_auto_conn ^ (0x8000 >> index2);
    }
    hccast_wifi_ap_info_t *wifi_ap = malloc(sizeof(hccast_wifi_ap_info_t));
    if (wifi_ap != NULL) {
        memcpy(wifi_ap, &(sys_param.app_data.cast_setting.wifi_ap[index1]), sizeof(hccast_wifi_ap_info_t));
        memcpy(&(sys_param.app_data.cast_setting.wifi_ap[index1]), &(sys_param.app_data.cast_setting.wifi_ap[index2]), \
            sizeof(hccast_wifi_ap_info_t));
        memcpy(&(sys_param.app_data.cast_setting.wifi_ap[index2]), wifi_ap, sizeof(hccast_wifi_ap_info_t));
        free(wifi_ap);
    }
}

void sysdata_wifi_ap_sort(bool wifi_connect) /*big to small*/
{
    pthread_mutex_lock(wifi_mutex_p);
    int i, j, wifi_max;
    i = (wifi_connect) ? 1 : 0;
    int wifi_num = sysdata_get_saved_wifi_count();
    for (; i < wifi_num; i++)
    {
        wifi_max = i;
        for (j = i + 1; j < wifi_num; j++)
        {
            if(sys_param.app_data.cast_setting.wifi_ap[wifi_max].quality<sys_param.app_data.cast_setting.wifi_ap[j].quality)
                wifi_max = j;
        }
        if(wifi_max != i)
        {
            sysdata_wifi_ap_swap(i, wifi_max);
        }
    }
    pthread_mutex_unlock(wifi_mutex_p);
}

void sysdata_wifi_ap_set_auto(int index){
	if(index<0 && index>=MAX_WIFI_SAVE){
		return;
	}
	sys_param.app_data.cast_setting.wifi_auto_conn = sys_param.app_data.cast_setting.wifi_auto_conn | (0x8000>>index);
}

void sysdata_wifi_ap_set_nonauto(int index){
	if(index<0 && index>=MAX_WIFI_SAVE){
		return;
	}
	sys_param.app_data.cast_setting.wifi_auto_conn = sys_param.app_data.cast_setting.wifi_auto_conn & ((0x8000>>index)^0xffff);
}

bool sysdata_wifi_ap_get_auto(int index){
	if(index<0 && index>=MAX_WIFI_SAVE){
		return false;
	}
	if (sys_param.app_data.cast_setting.wifi_auto_conn & (0x8000>>index))
		return true;
	else
		return false;
}

void sysdata_wifi_ap_delete(int index)
{
    int i = 0;

    if(index > MAX_WIFI_SAVE-1)
    {
        return;
    }

    for(i = index; i < MAX_WIFI_SAVE-1; i++)
    {
        memcpy(&(sys_param.app_data.cast_setting.wifi_ap[i]), &(sys_param.app_data.cast_setting.wifi_ap[i+1]), \
        	sizeof(hccast_wifi_ap_info_t));
    }
	sys_param.app_data.cast_setting.wifi_auto_conn = (((0xffff>>(index+1))&sys_param.app_data.cast_setting.wifi_auto_conn)<<1)
	 | ((~(0xffff>>index)) & sys_param.app_data.cast_setting.wifi_auto_conn);//去除被删除wifi对应的自动连接位
    memset(&(sys_param.app_data.cast_setting.wifi_ap[MAX_WIFI_SAVE-1]),0x00,sizeof(hccast_wifi_ap_info_t));
	printf("auto_conn delete: %x\n", sys_param.app_data.cast_setting.wifi_auto_conn);
}

hccast_wifi_ap_info_t *sysdata_get_wifi_info(char* ssid)
{
	pthread_mutex_lock(wifi_mutex_p);
	int i = 0;
	for(i = 0; i < MAX_WIFI_SAVE; i++)
	{
		if(strcmp(ssid, sys_param.app_data.cast_setting.wifi_ap[i].ssid) == 0)
		{
			pthread_mutex_unlock(wifi_mutex_p);
			return &sys_param.app_data.cast_setting.wifi_ap[i];
		}
	}
	pthread_mutex_unlock(wifi_mutex_p);
	return NULL;
}

hccast_wifi_ap_info_t *sysdata_get_wifi_info_by_index(int i){

    if(i > MAX_WIFI_SAVE-1 || strlen(sys_param.app_data.cast_setting.wifi_ap[i].ssid) == 0)
    {
        return NULL;
    }

    return &sys_param.app_data.cast_setting.wifi_ap[i];
}

int sysdata_get_saved_wifi_count(void){
	int i=0;
	for(; MAX_WIFI_SAVE>i && strlen(sys_param.app_data.cast_setting.wifi_ap[i].ssid)>0;i++);
	return i;
}

int sysdata_get_wifi_index_by_ssid(char *ssid){
	for(int i = 0; i < MAX_WIFI_SAVE; i++){
		if(ssid && strcmp(ssid,sys_param.app_data.cast_setting.wifi_ap[i].ssid) == 0){
			return i;
		}
	}
	return -1;
}

bool sysdata_wifi_ap_get(hccast_wifi_ap_info_t *wifi_ap)
{
	for(int i= 0; i<MAX_WIFI_SAVE;i++){
		if(!sysdata_wifi_ap_get_auto(i)){
			continue;
		}
		if (strlen(sys_param.app_data.cast_setting.wifi_ap[i].ssid)){
			memcpy(wifi_ap, sys_param.app_data.cast_setting.wifi_ap+i, sizeof(hccast_wifi_ap_info_t));
			if(i>0){
				sysdata_wifi_ap_delete(i);
				sysdata_wifi_ap_save(wifi_ap);//移到第0个
				//sys_param.app_data.cast_setting.wifi_auto_conn = j | (sys_param.app_data.cast_setting.wifi_auto_conn & (0xffff>>1));//交换wifi_auto_conn的第0位和第i位
				projector_sys_param_save();				
			}

			return true;
		}
		else{
			return false;
		}
	}
	return false;
        
}

#endif



void sysdata_eq_gain_get(int eq_mode,int* nums){
	if(eq_mode < 0 || eq_mode >= EQ_MODE_MAX){
		return;
	}
	for(int i=0; i < EQ_BAND_LEN; i++){
		nums[i] = sys_param.app_data.soundset.eq_gain[eq_mode][i];
	}
}

void sysdata_eq_gain_set(int eq_mode, int id, int gain){
	if(eq_mode < 0 || eq_mode >= EQ_MODE_MAX){
		return;
	}	
	if(id < 0 || id >= EQ_BAND_LEN){
		return;
	}
	sys_param.app_data.soundset.eq_gain[eq_mode][id] = gain;
}


void sysdata_app_tv_sys_set(app_tv_sys_t app_tv_sys)
{
    int tv_sys;

    sys_param.app_data.resolution = app_tv_sys;

    if (APP_TV_SYS_AUTO == app_tv_sys)
        tv_sys = tv_sys_best_tv_type_get();    
    else
        tv_sys = tv_sys_app_sys_to_de_sys(app_tv_sys);
    sys_param.sys_data.tvtype = tv_sys;
}

#ifdef WIFI_SUPPORT
extern int api_get_mac_addr(char *mac);
int sysdata_init_device_name(void)
{
    unsigned char mac[6] = {0};
    int rand_num = 0;

    wifi_cast_setting_t *cast_set = &(sys_param.app_data.cast_setting);

    if (api_get_mac_addr((char*)mac) == 0){
        if (!cast_set->cast_dev_name_changed && memcmp(mac, cast_set->mac_addr, MAC_ADDR_LEN)){
            snprintf(cast_set->cast_dev_name, MAX_DEV_NAME, "%s-%02X%02X%02X", 
                SSID_NAME, mac[3]&0xff, mac[4]&0xff, mac[5]&0xff);   
            memcpy(cast_set->mac_addr, mac, MAC_ADDR_LEN);
            projector_sys_param_save();
        }
    } else {
        if (cast_set->cast_dev_name[0] == 0 && cast_set->cast_dev_name[1] == 0){
            printf("%s get netif addr failed!Use rand value.\n", __FUNCTION__);
            srand(time(NULL));
            rand_num = rand();
            snprintf(cast_set->cast_dev_name, MAX_DEV_NAME, "%s_%06X", SSID_NAME, rand_num&0xffffff);
            projector_sys_param_save();
        }
    }
    printf("%s device_name=%s\n", __func__, cast_set->cast_dev_name);
    return 0;
}
#endif

//#include "../channel/local_mp/file_mgr.h"

play_info_t * sys_data_search_media_info(char *file_name, media_type_t type)
{
#ifndef HC_MEDIA_MEMMORY_PLAY
    if(type == MEDIA_TYPE_TXT)
	{
		return &save_txt_info_tmp;
    }
#else
    if(type == MEDIA_TYPE_MUSIC)
    {
    	if (0 == strncmp(sys_param.app_data.save_music_info.path, file_name, HCCAST_MEDIA_MAX_NAME-1)){
    		return &sys_param.app_data.save_music_info;
    	}
    }
    else
    {
		if(type == MEDIA_TYPE_VIDEO){
			for (int i = 0; i < RECORD_ITEM_NUM_MAX; i ++){
		    	if (0 == strncmp(sys_param.app_data.save_video_info.play_node[i].path, file_name, HCCAST_MEDIA_MAX_NAME-1)){
		    		return &(sys_param.app_data.save_video_info.play_node[i]);
		    	}
			}
		}
		else if(type == MEDIA_TYPE_TXT){
			for (int i = 0; i < RECORD_ITEM_NUM_MAX; i ++){
		    	if (0 == strncmp(sys_param.app_data.save_txt_info.play_node[i].path, file_name, HCCAST_MEDIA_MAX_NAME-1)){
		    		return &(sys_param.app_data.save_txt_info.play_node[i]);
		    	}
			}
	    }
	}

#endif

	return NULL;
}

play_info_t * sys_data_get_media_info(media_type_t type)
{
#ifndef HC_MEDIA_MEMMORY_PLAY
    if(type == MEDIA_TYPE_TXT)
	{
		return &save_txt_info_tmp;
    }
#else

    if(type == MEDIA_TYPE_MUSIC)
    {
		return &sys_param.app_data.save_music_info;
    }
    else
    {
		if(type == MEDIA_TYPE_VIDEO)
			return &sys_param.app_data.save_video_info.play_node[0];
		else if(type == MEDIA_TYPE_TXT)
			return &sys_param.app_data.save_txt_info.play_node[0];
    }
#endif
    return NULL;
}
void sys_data_set_media_info(play_info_t *media_info_data)
{
#ifndef HC_MEDIA_MEMMORY_PLAY
    if(media_info_data->type == MEDIA_TYPE_TXT)
    {
        if(memcmp(save_txt_info_tmp.path,media_info_data->path,strlen(media_info_data->path)+1) != 0)
        {
            memset(save_txt_info_tmp.path,0,HCCAST_MEDIA_MAX_NAME);
            strcpy(save_txt_info_tmp.path,media_info_data->path);
            save_txt_info_tmp.current_time = media_info_data->current_time;
            save_txt_info_tmp.current_offset= media_info_data->current_offset;
            save_txt_info_tmp.type = media_info_data->type;
            save_txt_info_tmp.last_page= media_info_data->last_page;
        } 
    }
#else
    int offset=0;
    sys_param.app_data.save_media_type = media_info_data->type;
    offset = offsetof(app_data_t, save_media_type);
    sys_data_item_save(offset, sizeof(media_type_t),NODE_ID_PROJECTOR);

    int i = 0;
    int file_index = 0;
    bool has_same_file = false;
    play_info_data *p = NULL;
    if(media_info_data->type == MEDIA_TYPE_VIDEO)
    {
        p = &sys_param.app_data.save_video_info;
        offset =offsetof(app_data_t, save_video_info);
    }
    else if(media_info_data->type == MEDIA_TYPE_TXT)
    {
        p = &sys_param.app_data.save_txt_info;
        offset =offsetof(app_data_t, save_txt_info);
    }
    if(media_info_data->type == MEDIA_TYPE_VIDEO || media_info_data->type == MEDIA_TYPE_TXT)
    {
        for(i = 0;i < RECORD_ITEM_NUM_MAX;i++)
        {
            if(memcmp(p->play_node[i].path,media_info_data->path,strlen(media_info_data->path)+1) == 0)
            {
                has_same_file= true;
                file_index = i;
                break;
            }
        }	
        if(!has_same_file)
            file_index = RECORD_ITEM_NUM_MAX-1;

        for(i = file_index; i > 0; i--)
        {
            if(strlen(p->play_node[i-1].path)>0)
            {
                memset(&p->play_node[i],0,sizeof(play_info_t));
                p->play_node[i].type = p->play_node[i-1].type;
                p->play_node[i].current_time = p->play_node[i-1].current_time;
                p->play_node[i].current_offset = p->play_node[i-1].current_offset;
                p->play_node[i].last_page = p->play_node[i-1].last_page;
                strncpy(p->play_node[i].path,p->play_node[i-1].path,\
                	strlen(p->play_node[i-1].path)+1);
            }
        }
        memset(&p->play_node[0],0,sizeof(play_info_t));
        p->play_node[0].type = media_info_data->type;
        p->play_node[0].current_time = media_info_data->current_time;
        p->play_node[0].current_offset = media_info_data->current_offset;
        p->play_node[0].last_page = media_info_data->last_page;
        strncpy(p->play_node[0].path,media_info_data->path,\
        	strlen(media_info_data->path)+1);

        sys_data_item_save(offset, sizeof(play_info_data),NODE_ID_PROJECTOR);

    //     for(i = 0; i < RECORD_ITEM_NUM_MAX; i++)
    //     {
    //         if(strlen(p->play_node[i].path)>0)
    //         {
				// printf("%s().%d(%d): %s:%d\n", __func__, __LINE__, i, p->play_node[i].path, p->play_node[i].current_time);                
    //         }
    //     }
    }
    else if(media_info_data->type == MEDIA_TYPE_MUSIC)
    {
    	play_info_t *p_music = &sys_param.app_data.save_music_info;

    	//only update the different played file name.
        if(memcmp(p_music->path, media_info_data->path,strlen(media_info_data->path)+1))
        {
	        offset =offsetof(app_data_t, save_music_info);
	        strcpy(p_music->path,media_info_data->path);
	        p_music->current_time = media_info_data->current_time;
	        p_music->current_offset= media_info_data->current_offset;
	        p_music->type = media_info_data->type;
	        p_music->last_page= media_info_data->last_page;
	        sys_data_item_save(offset, sizeof(play_info_t),NODE_ID_PROJECTOR);
        }

    }
#endif
}

void sys_data_set_media_play_cur_time(char *name,int type,int cut_time)
{
#ifndef HC_MEDIA_MEMMORY_PLAY
    if(type == MEDIA_TYPE_TXT)
        save_txt_info_tmp.current_time= cut_time;
#else
    int offset=0;	

    bool volatile new_record = false;
    if(type == MEDIA_TYPE_VIDEO)
    {
        if(memcmp(sys_param.app_data.save_video_info.play_node[0].path,name,strlen(name))!=0)
        	new_record = true;
    }
    else if(type == MEDIA_TYPE_TXT)
    {
        if(memcmp(sys_param.app_data.save_txt_info.play_node[0].path,name,strlen(name))!=0)
        	new_record = true;
    }
    else if(type == MEDIA_TYPE_MUSIC)
    {
        if(memcmp(sys_param.app_data.save_music_info.path,name,strlen(name))!=0)
        	new_record = true;
    }

    if (new_record){
	    play_info_t media_info_data = {0};
	    media_info_data.type = type;
	    media_info_data.current_time = cut_time;
	    memcpy(media_info_data.path,name,sizeof(media_info_data.path));
	    sys_data_set_media_info(&media_info_data);
    } else {
	    if(type == MEDIA_TYPE_VIDEO)
	    {
	        offset =offsetof(app_data_t, save_video_info.play_node[0].current_time);
	        sys_param.app_data.save_video_info.play_node[0].current_time = cut_time;
	    }
	    else if(type == MEDIA_TYPE_TXT)
	    {
	        offset =offsetof(app_data_t, save_txt_info.play_node[0].current_time);
	        sys_param.app_data.save_txt_info.play_node[0].current_time = cut_time;
	    }
	    else if(type == MEDIA_TYPE_MUSIC)
	    {
	        offset =offsetof(app_data_t, save_music_info.current_time);
	        sys_param.app_data.save_music_info.current_time = cut_time;
	    }
	    sys_data_item_save(offset, sizeof(uint32_t),NODE_ID_PROJECTOR);
	}
#endif
}


void sys_data_set_ebook_offset(char *name, ebook_page_info_t *ebook_data)
{
#ifndef HC_MEDIA_MEMMORY_PLAY
	strncpy(save_txt_info_tmp.path, name, HCCAST_MEDIA_MAX_NAME-1);
    save_txt_info_tmp.current_time = ebook_data->e_page_byte;
    save_txt_info_tmp.current_offset= ebook_data->e_page_seek;
    save_txt_info_tmp.last_page= ebook_data->e_page_num;
#else
    if(0 == strncmp(sys_param.app_data.save_txt_info.play_node[0].path, name, HCCAST_MEDIA_MAX_NAME-1)){
	    int offset=0;
	    
	    offset = offsetof(app_data_t, save_txt_info.play_node[0].current_time);
	    sys_param.app_data.save_txt_info.play_node[0].current_time = ebook_data->e_page_byte;
	    sys_param.app_data.save_txt_info.play_node[0].current_offset= ebook_data->e_page_seek;
	    sys_param.app_data.save_txt_info.play_node[0].last_page= ebook_data->e_page_num;
	    sys_data_item_save(offset, sizeof(uint32_t)+sizeof(uint32_t)+sizeof(int),NODE_ID_PROJECTOR);
    }else{
	    play_info_t media_info_data = {0};

	    media_info_data.type = MEDIA_TYPE_TXT;
	    strncpy(media_info_data.path, name, HCCAST_MEDIA_MAX_NAME-1);
	    media_info_data.current_time = ebook_data->e_page_byte;
	    media_info_data.current_offset = ebook_data->e_page_seek;
	    media_info_data.last_page = ebook_data->e_page_num;
	    sys_data_set_media_info(&media_info_data);
    }

#endif
}

static int sys_data_item_save(uint16_t offset, uint16_t size, int node_id)
{
    uint8_t *save_item = NULL;
    if (NODE_ID_PROJECTOR == node_id) {
        save_item = (uint8_t*)(&sys_param.app_data) + offset;
    }else {
        return -1;
    }
    
    struct persistentmem_node node;

    memset(&node, 0, sizeof(struct persistentmem_node));
    node.id = node_id;
    node.offset = offset;
    node.size = size;
    node.buf = save_item;
    projector_memory_save(&node);

    return 0;
}

static void *projector_memory_save_task(void *arg)
{
    memory_msg_t ctl_msg = {0};
    struct persistentmem_node *save_node;
    int ret = -1;
    int fd_sys = -1;

    while(1) {
        sem_wait(&sem_projector_memory_save);
        ret = projector_memory_sys_receive_msg(&ctl_msg);
        if (ret != 0 || !ctl_msg.msg_code)
            continue;

		save_node = (struct persistentmem_node *)ctl_msg.msg_code;
		api_enter_flash_rw();
        fd_sys = open(SYS_DATA_DEV, O_RDWR);
        if (fd_sys < 0) {
	        api_leave_flash_rw();
            printf("%s(), line:%d. Open %s fail!\n", __func__, __LINE__, SYS_DATA_DEV);
            free(save_node->buf);
            free(save_node);
            continue;
        }
        if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_PUT, save_node) < 0) {
            printf("%s(), line:%d. put app data failed\n", __func__, __LINE__);
        }
        api_leave_flash_rw();

        close(fd_sys);
        free(save_node->buf);
        free(save_node);
    }
    return NULL;
}

static int projector_memory_save(struct persistentmem_node *node)
{
	void *changed_start = NULL;
	void *backup_start = NULL;
	uint32_t cmp_length = 0;

    struct persistentmem_node *save_node = NULL;
    uint8_t *save_buf = NULL;

    if (NODE_ID_PROJECTOR == node->id){
    	changed_start = (uint8_t*)(&sys_param.app_data) + node->offset;
    	backup_start = (uint8_t*)(&sys_param_bk.app_data) + node->offset;
    } else if (PERSISTENTMEM_NODE_ID_SYSDATA == node->id) {
    	changed_start = (uint8_t*)(&sys_param.sys_data) + node->offset;
    	backup_start = (uint8_t*)(&sys_param_bk.sys_data) + node->offset;
    }
	cmp_length = node->size;
	//Do not need saving to flash if the config is same!
	if (!memcmp(changed_start, backup_start, cmp_length))
		return 0;

    save_node = (struct persistentmem_node *)malloc(sizeof(struct persistentmem_node));
    save_buf = malloc(node->size);
    if (save_node == NULL || save_buf == NULL) {
        printf("%s(), line:%d. malloc memory fail!\n", __func__, __LINE__);
        return -1;
    }
    save_node->id = node->id;
    save_node->offset = node->offset;
    save_node->size = node->size;
    
    memcpy(save_buf, node->buf, save_node->size);
    save_node->buf = save_buf;

    memcpy(backup_start, changed_start, cmp_length);
#if 0
    int fd_sys = -1;
    fd_sys = open(SYS_DATA_DEV, O_RDWR);
    if (fd_sys < 0) {
        printf("%s(), line:%d. Open %s fail!\n", __func__, __LINE__, SYS_DATA_DEV);
        return -1;
    }
    if (ioctl(fd_sys, PERSISTENTMEM_IOCTL_NODE_PUT, save_node) < 0) {
        printf("%s(), line:%d. put app data failed\n", __func__, __LINE__);
        close(fd_sys);
        return -1;
    }
    close(fd_sys);
#else    
    memory_msg_t control_msg = {0};
    control_msg.msg_code = save_node;
    projector_memory_sys_send_msg(&control_msg);
#endif    

    return 0;
}

static int projector_memory_sys_send_msg(memory_msg_t *control_msg)
{
    if (INVALID_ID == m_memory_save_msg_id){
        m_memory_save_msg_id = api_message_create(MEMORY_CTL_MSG_COUNT, sizeof(memory_msg_t));
        if (INVALID_ID == m_memory_save_msg_id){
            return -1;
        }
    }
    api_message_send(m_memory_save_msg_id, control_msg, sizeof(memory_msg_t));
    sem_post(&sem_projector_memory_save);
    return 0;
}

static int projector_memory_sys_receive_msg(memory_msg_t *control_msg)
{
    if (INVALID_ID == m_memory_save_msg_id){
        return -1;
    }
    return api_message_receive_tm(m_memory_save_msg_id, control_msg, sizeof(memory_msg_t), 5);
}

void projector_memory_save_init(void)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // release task resource itself

    if (sem_init(&sem_projector_memory_save, 0, 0) == -1)
        printf("%s(), line:%d. creat sem fail\n", __func__, __LINE__);
    if (pthread_create(&thread_id, &attr, projector_memory_save_task, NULL)) {
        pthread_attr_destroy(&attr);
        return;
    }

    pthread_attr_destroy(&attr);
}

#ifdef HCIPTV_YTB_SUPPORT

hciptv_ytb_app_config_t *sysdata_get_iptv_app_config(void)
{
    return &sys_param.app_data.ytb_app_config;
}

uint32_t sysdata_get_iptv_app_search_option(void)
{
    return sys_param.app_data.ytb_app_config.iptv_config.search_option;
}

#endif
