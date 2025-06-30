#include "app_config.h"

#include <stdio.h>
#include <hcuapi/sysdata.h>

#ifdef WIFI_SUPPORT
#include <hccast/hccast_wifi_mgr.h>
#endif

#ifdef HCIPTV_YTB_SUPPORT
#include "../channel/webplayer/win_webservice.h"
#endif

#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif
#include "../channel/local_mp/mp_ebook.h"
#include "../channel/local_mp/media_player.h"

//#include "../channel/local_mp/file_mgr.h"


#define BLUETOOTH_MAC_LEN 6
#define BLUETOOTH_NAME_LEN 128

#define MAX_DEV_NAME    32
#define MAX_DEV_PSK 32
#define MAX_WIFI_SAVE   5
#define MAX_BT_SAVE 4
#define SSID_NAME "Hccast"
#define DEVICE_PSK "12345678"
#define EQ_BAND_LEN 10
#define SYS_DATA_DEV    "/dev/persistentmem"
#define HOSTAP_CHANNEL_24G  1
#define HOSTAP_CHANNEL_5G   36
#define HOSTAP_CHANNEL_AUTO 0
#define HCCAST_MEDIA_MAX_NAME	1024
#define MAC_ADDR_LEN    6
#define RECORD_ITEM_NUM_MAX	4

enum{
    FACTORY_CVBS_IDX = 0,
    FACTORY_HDMIRX_IDX,
    FACTORY_MP_IDX,

    FACTORY_MAX_IDX,
};
#define MAX_FACTORYSET_CHANNEL (FACTORY_MAX_IDX+1)


#define APP_HOSTAP_IP_START_ADDR ("192.168.68.10")
#define APP_HOSTAP_IP_END_ADDR   ("192.168.68.100")
#define APP_HOSTAP_LOCAL_IP_ADDR ("192.168.68.1")
#define APP_HOSTAP_MASK_ADDR     ("255.255.255.0")

typedef enum {
    P_PICTURE_MODE,
    P_CONTRAST,
    P_BRIGHTNESS,
    P_SHARPNESS,
    P_COLOR,
    P_HUE,
    P_COLOR_TEMP,
    P_NOISE_REDU,
    P_SOUND_MODE,
    P_SOUND_EQ,
    P_EQ_MODE,
    P_BALANCE,
    P_BT_SETTING,
    P_BT_MAC,
    P_BT_NAME,
    P_TREBLE,
    P_BASS,
    P_OSD_LANGUAGE,
    P_ASPECT_RATIO,
    P_CUR_CHANNEL,
    P_FLIP_MODE,
    P_OSD_TIME,
    P_VOLUME,
    P_RESTORE,
    P_UPDATE,
    P_NETWORK_UPDATE,
    P_AUTOSLEEP,
    P_KEYSTONE,
    P_KEYSTONE_TOP,
    P_KEYSTONE_BOTTOM,
    P_VERSION_INFO,
    P_DEV_PRODUCT_ID,
    P_DEV_VERSION,
    P_MIRROR_MODE,
    P_AIRCAST_MODE,
    P_MIRROR_FRAME,
    P_BROWSER_LANGUAGE,
    P_SYS_RESOLUTION,
    P_DEVICE_NAME,
    P_DEVICE_PSK,
    P_WIFI_MODE,
    P_WIFI_CHANNEL,
    P_MIRROR_ROTATION,
    P_MIRROR_VSCREEN_AUTO_ROTATION,
    P_DE_TV_SYS,
    P_VMOTOR_COUNT,
#ifdef VMOTOR_LIMIT_ON
    P_VMOTOR_LIMIT_SINGLE,
#endif
    P_WIFI_ONOFF,
	P_WIFI_AUTO_CONN,
    P_WIFI_CHANNEL_24G,
    P_WIFI_CHANNEL_5G,
    P_WINDOW_SCALE,
    P_SYS_ZOOM_DIS_MODE,
    P_SYS_ZOOM_OUT_COUNT,
    P_FACTORY_CHANNEL_SELECT,
    P_PQ_ONFF,
    P_DYN_CONTRAST_ONOFF,
    P_CVBS_AEF_OFFSET,    
    P_MEM_PLAY_MEDIA_TYPE,    
    P_VIDEO_DELAY,
    P_SOUND_OUT_MODE,    
    P_SOUND_SPDIF_MODE,
    P_MIRROR_FULL_VSCREEN,
    P_CVBS_GAIN,
    P_MIRA_CONTINUE_ON_ERROR,
    P_UM_FULL_SCREEN,
    P_AIRP2P_CH,
    P_CEC_ONOFF,
    P_CEC_DEVICE_LA,
    P_BACKLIGHT,
#ifdef AUTOKEYSTONE_SWITCH
	P_AUTO_KEYSTONE,
	#if 0//def ATK_CALIBRATION
    P_ATK_CALIBRATION,
	#endif
#endif
#ifdef SUPPORT_STARTUP_PAGE_LANGUAGE
    P_SYS_IS_STARTUP,
#endif
} projector_sys_param;


// typedef enum flip_type{
//     FLIP_NORMAL,
//     FLIP_WH_REVERSE,
//     FLIP_MIRROR,
//     FLIP_180,
// }flip_type_e;



typedef enum picture_mode{
    PICTURE_MODE_STANDARD,
    PICTURE_MODE_DYNAMIC,

    PICTURE_MODE_MILD,
    PICTURE_MODE_USER,
}picture_mode_e;

typedef enum color_temp_{
    COLOR_TEMP_COLD,
    COLOR_TEMP_STANDARD,
    COLOR_TEMP_WARM,
}color_temp_e;

typedef enum noise_redu_{
    NOISE_REDU_OFF,
    NOISE_REDU_LOWER,
    NOISE_REDU_MEDI,
    MOISE_REDU_HIGH,
}noise_redu_e;

typedef enum sound_mode{
    SOUND_MODE_STANDARD,
    SOUND_MODE_MOVIE,
    SOUND_MODE_MUSIC,
    SOUND_MODE_SPORTS,
    SOUND_MODE_USER
} sound_mode_e;

typedef enum bluetooth_onoff_{
    BLUETOOTH_OFF,
    BLUETOOTH_ON
} bluetooth_onoff;

typedef enum auto_sleep{
    AUTO_SLEEP_OFF,
    AUTO_SLEEP_ONE_HOUR,
    AUTO_SLEEP_TWO_HOURS,
    AUTO_SLEEP_THREE_HOURS
} auto_sleep_e;

typedef enum aspect_ratio_{
    ASPECT_RATIO_AUTO,
    ASPECT_RATIO_4_3,
    ASPECT_RATIO_16_9,
    ZOOM_IN,
    ZOOM_OUT
}aspect_ratio_e;

typedef enum osd_time_{
    OSD_TIME_OFF,
    OSD_TIME_5S,
    OSD_TIEM_10S,
    OSD_TIME_15S,
    OSD_TIEM_20S,
    OSD_TIME_25S,
    OSD_TIME_30S
} osd_time_e;

typedef enum sound_output_mode_{
    SOUND_OUTPUT_SPEAKER = 0,
    SOUND_OUTPUT_SPDIF,
} sound_output_mode_e;

typedef enum sound_spdif_mode_{
    SOUND_SPDIF_RAW = 0,
    SOUND_SPDIF_PCM,
} sound_spdif_mode_e;

typedef enum eq_mode_{
    EQ_MODE_NORMAL,
    EQ_MODE_ROCK,
    EQ_MODE_POP,
    EQ_MODE_JAZZ,
    EQ_MODE_CLASSIC,
    EQ_MODE_VOICE,
    EQ_MODE_USER,
    EQ_MODE_MAX,
} eq_mode_e;

typedef struct pictureset{
    uint8_t picture_mode;
    uint8_t contrast;
    uint8_t brightness;
    uint8_t sharpness;
    uint8_t color;
    uint8_t hue;
    uint8_t color_temp;
    uint8_t noise_redu;
}pictureset_t;

typedef struct soundset{
    uint8_t sound_mode;
    int8_t balance;
    uint8_t bt_setting;
#ifdef BLUETOOTH_SUPPORT    
    struct bluetooth_slave_dev bt_dev[MAX_BT_SAVE];
#endif    
    int8_t treble;
    int8_t bass;
    int8_t eq_gain[EQ_MODE_MAX][EQ_BAND_LEN];
    int8_t eq_enable;
    int8_t eq_mode;
    int8_t output_mode; //0: speaker; 1: spdif
    int8_t spdif_mode; //0: spdif-raw; 1: spdif-pcm
}soundset_t;

typedef struct optionset{
    uint8_t osd_language;
    uint8_t aspect_ratio;
//    uint keystone_top_w;
//   uint keystone_bottom_w;
    uint resved_uint[2];//8 bytes
    uint8_t auto_sleep;
    uint8_t osd_time;
    uint16_t video_delay;    
    uint8_t resved[2];
}optionset_t;

typedef enum{
    APP_TV_SYS_480P = 1,
    APP_TV_SYS_576P,
    APP_TV_SYS_720P,
    APP_TV_SYS_1080P,
    APP_TV_SYS_4K,

    APP_TV_SYS_AUTO,

}app_tv_sys_t;

typedef enum
{
    MIRROR_ROTATE_0,
    MIRROR_ROTATE_270,
    MIRROR_ROTATE_90,
    MIRROR_ROTATE_180,
} mirror_rotate_e;

typedef enum{
    CVBS_CHANNEL=0,
    HDMI_CHANNEL,
    MUTL_MEDIA_CHANNEL,
    MAX_CHANNEL,
}factory_channel_e;
typedef struct play_info{
    int type;
    uint32_t current_time;
	uint32_t current_offset;
	int last_page;
	char path[HCCAST_MEDIA_MAX_NAME];
}play_info_t;

typedef struct{
	play_info_t	play_node[RECORD_ITEM_NUM_MAX];
}play_info_data;

typedef struct wifi_cast_setting{
    char cast_dev_name[MAX_DEV_NAME];
    char cast_dev_psk[MAX_DEV_PSK];
    char mac_addr[MAC_ADDR_LEN];
    char cast_dev_name_changed; //if the flag is set, used the cast_dev_name for cast name
#ifdef WIFI_SUPPORT    
    hccast_wifi_ap_info_t wifi_ap[MAX_WIFI_SAVE];
    uint16_t wifi_auto_conn;
    uint8_t wifi_onoff;
#endif    
    int browserlang;//1-englist;2-chn;3-traditional chn
    int ratio;
    int mirror_mode;//1-standard.
    int mirror_frame;//0-30FPS,1-60FPS
    int aircast_mode;//0-mirror-stream, 1-mirror-only, 2-Auto
    int wifi_mode;          // 1: 24G, 2: 5G, 3: 60G (res)
    int wifi_ch;            // 24G hostap channel
    int mirror_rotation;    //mirror_rotate_e
    int mirror_vscreen_auto_rotation;//0-disable, 1-enable.
    int wifi_ch5g;          // 5G hostap wifi channel
    int mirror_full_vscreen; //0-disable, 1-enable.
    int um_full_screen; //0-disable, 1-enable.
    int airp2p_ch; 
}wifi_cast_setting_t;

typedef struct sys_scale_setting{
    // int left;
    // int top;
    // int h_mul;
    // int v_mul;
    // int main_layer_h;
    // int main_layer_v;
    int dis_mode;
    int zoom_out_count;
} sys_scale_setting_t;

typedef struct {
    uint8_t  channel_select;
    pictureset_t picset[MAX_FACTORYSET_CHANNEL];
    uint8_t pq_onoff;
    uint8_t dyn_contrast_onoff;
    uint32_t cvbs_aef;
}factory_settiing_t ;

typedef struct {
    uint8_t continue_on_error;
} mir_param_t;

typedef struct{
	uint8_t on;
	uint8_t dev_la;
}cec_info_t;

typedef struct appdata{
    uint8_t cur_channel;
    uint8_t flip_mode;
    uint8_t volume;
    int vmotor_count;
#ifdef VMOTOR_LIMIT_ON
    int8_t vmotor_limit_single;
#endif
    app_tv_sys_t resolution;
    uint8_t cvbs_gain;
    pictureset_t pictureset; 
    soundset_t soundset;
    optionset_t optset;
    wifi_cast_setting_t cast_setting;
#ifdef SYS_ZOOM_SUPPORT
    sys_scale_setting_t scale_setting;
#endif
#ifdef SUPPORT_STARTUP_PAGE_LANGUAGE
    bool is_startup;
#endif
#ifdef HC_FACTORY_TEST
    factory_settiing_t factory_setting;
#endif
#ifdef HC_MEDIA_MEMMORY_PLAY
    media_type_t    save_media_type;
	play_info_data	save_video_info;
	play_info_data	save_txt_info;
	play_info_t		save_music_info;
#endif

#ifdef HCIPTV_YTB_SUPPORT
    hciptv_ytb_app_config_t ytb_app_config;
#endif

    mir_param_t mira_param;
#ifdef HDMI_RX_CEC_SUPPORT
    cec_info_t  cec_info;
#endif
#ifdef AUTOKEYSTONE_SWITCH
    int auto_keystone;
	#if 0//def ATK_CALIBRATION
    int atk_calibration;
	#endif
#endif
}app_data_t;


typedef struct sys_param{
    // boot/upgrade parameters, 
    struct sysdata sys_data;  
    //projector prameters
    struct appdata app_data;

}sys_param_t;

typedef struct{
    struct persistentmem_node *msg_code;
}memory_msg_t;

void projector_factory_init(void);
void projector_factory_reset(void);
int projector_sys_param_load(void);
int projector_sys_param_save(void);

extern sys_param_t * projector_get_sys_param(void);
void projector_set_some_sys_param(projector_sys_param param, int v);
int projector_get_some_sys_param(projector_sys_param param);
#ifdef BLUETOOTH_SUPPORT
  unsigned char* projector_get_bt_mac(int );
  char* projector_get_bt_name(int);
  void projector_save_bt_dev(struct bluetooth_slave_dev *dev);
  struct bluetooth_slave_dev *projector_get_bt_dev(int);
  struct bluetooth_slave_dev * projector_get_bt_by_name(char* name);
  struct bluetooth_slave_dev * projector_get_bt_by_mac(char* mac);
  void projector_bt_dev_delete(int index);
  int projector_get_saved_bt_count();
#endif

#ifdef WIFI_SUPPORT
int8_t get_save_wifi_flag(void);
void set_save_wifi_flag_zero(void);
int sysdata_check_ap_saved(hccast_wifi_ap_info_t* check_wifi);
void sysdata_wifi_ap_save(hccast_wifi_ap_info_t *wifi_ap);
hccast_wifi_ap_info_t *sysdata_get_wifi_info(char* ssid);
bool sysdata_wifi_ap_get(hccast_wifi_ap_info_t *wifi_ap);
hccast_wifi_ap_info_t *sysdata_get_wifi_info_by_index(int i);
int sysdata_get_wifi_index_by_ssid(char *ssid);
int sysdata_get_saved_wifi_count(void);
void sysdata_wifi_ap_set_auto(int index);
void sysdata_wifi_ap_set_nonauto(int index);
bool sysdata_wifi_ap_get_auto(int index);
void sysdata_wifi_ap_resave(hccast_wifi_ap_info_t *wifi_ap);
void sysdata_wifi_ap_sort(bool wifi_connect);
void wifi_mutex_init(void);
void sysdata_wifi_ap_delete(int index);
#endif

#ifdef SYS_ZOOM_SUPPORT
void sysdata_disp_rect_save(int x, int y, int w, int h);
#endif
void sysdata_app_tv_sys_set(app_tv_sys_t app_tv_sys);
int sysdata_init_device_name(void);
char* projector_get_version_info(void);
#ifdef HC_FACTORY_TEST
void factory_set_some_sys_param(projector_sys_param param, int v);
int factory_get_some_sys_param(projector_sys_param param);
#endif 

#ifdef HCIPTV_YTB_SUPPORT
hciptv_ytb_app_config_t *sysdata_get_iptv_app_config(void);
uint32_t sysdata_get_iptv_app_search_option(void);
#endif

int sys_data_get_media_play_num(void);
play_info_t * sys_data_get_media_info(media_type_t type);
void sys_data_set_media_info(play_info_t *media_info_data);
play_info_t * sys_data_search_media_info(char *file_name, media_type_t type);
void sys_data_set_media_play_cur_time(char *name,int type,int cut_time);
char *sys_data_get_last_play_name(int type);
void sys_data_set_ebook_offset(char *name, ebook_page_info_t *media_info_data);
void sys_data_set_media_play_num(int index);
void sysdata_eq_gain_get(int eq_mode, int* nums);
void sysdata_eq_gain_set(int eq_mode, int id, int gain);
static int sys_data_item_save(uint16_t offset, uint16_t size, int node_id);
void projector_memory_save_init(void);