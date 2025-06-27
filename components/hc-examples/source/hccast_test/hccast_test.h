#ifndef __HCCAST_TEST_H__
#define __HCCAST_TEST_H__

#include <hcuapi/dis.h>
#include <hcuapi/viddec.h>

#include <hccast/hccast_mira.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __HCRTOS__
#include <generated/br2_autoconf.h>
#else
#include <autoconf.h>
#endif

typedef enum{
	HCTEST_PROJECT_REAR = 0,
	HCTEST_PROJECT_CEILING_REAR,
	HCTEST_PROJECT_FRONT,
	HCTEST_PROJECT_CEILING_FRONT,

	HCTEST_PROJECT_MODE_MAX
}hctest_project_mode_e;

enum{
	HCTEST_ROTATE_0 = 0,
	HCTEST_ROTATE_90,
	HCTEST_ROTATE_180,
	HCTEST_ROTATE_270,

	HCTEST_ROTATE_MAX
};


typedef enum{
	HCCAST_TEST_DIS_TYPE_HD = 0,
	HCCAST_TEST_DIS_TYPE_UHD,

	HCCAST_TEST_DIS_TYPE_MAX

}hccast_test_dis_type_e;

typedef enum{
	HCCAST_TEST_DIS_LAYER_MAIN = 0,
	HCCAST_TEST_DIS_LAYER_AUXP,

	HCCAST_TEST_DIS_LAYER_MAX
}hccast_test_dis_layer_e;


enum{
	HCCAST_TEST_TYPE_MIRACAST,
	HCCAST_TEST_TYPE_AIRCAST,
	HCCAST_TEST_TYPE_DLNA,

	HCCAST_TEST_WIFI_TYPE_MAX
};

enum{
	HCCAST_TEST_TYPE_AUM,
	HCCAST_TEST_TYPE_IUM,

	HCCAST_TEST_USB_TYPE_MAX
};

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} hccast_test_rect_t;

typedef struct{
    int pbp_on;
    hccast_test_dis_type_e dis_type;
    hccast_test_dis_layer_e dis_layer;

    dis_tv_mode_e tv_mode;
    int rotate_mode; //0: 0; 1: 90 degree; 2: 180 degree; 3, 270 degree. ROTATE_TYPE_0
    int flip_mode;  //0: no flip; 1: horizontal filp
    int auto_rotate_disable;  //0: auto rotate by dts; 1: disable auto rotate
    hctest_project_mode_e project_mode; 
    hccast_test_rect_t src_rect;
    hccast_test_rect_t dst_rect;

    int mirror_full_vscreen; //default enable, 1.
    int mirror_vscreen_auto_rotation; //default enable, 1.
    int um_full_screen; //default enable, 1.
    int audio_path; 
    int audio_disable; 
    int audio_dec_enable; //use audio dec to decoding audio, can mix audio with media player

    char name[32];
    int state; //0: stop; 1: start
}hccast_test_state_t;

#define HCCAST_DEF_CAST_NAME	"my-hccast-test"

#define HCCAST_TEST_SCREEN_WIDTH	1920
#define HCCAST_TEST_SCREEN_HEIGHT	1080

#ifdef BR2_PACKAGE_HCCAST_DLNA
#define DLNA_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_DIAL
#define DIAL_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_AIRCAST
#define AIRCAST_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_MIRACAST
#define MIRACAST_SUPPORT
#endif

#ifdef BR2_PACKAGE_HCCAST_USBMIRROR
#define USBMIRROR_SUPPORT
#endif

#ifdef __linux__

void wifi_cast_cmds_register(struct console_cmd *cmd);
void usb_cast_cmds_register(struct console_cmd *cmd);

void hctest_dts_string_get(const char *path, char *string, int size);
int hctest_dts_uint32_get(const char *path);

void cast_test_exit_console(int signo);
void cast_console_init(void);
#endif

int cast_wifi_init(int argc, char *argv[]);
int cast_usb_init(int argc, char *argv[]);

void hctest_screen_rotate_info(void);
int hctest_get_flip_info(hccast_test_state_t *cast_state, int *rotate_type, int *flip_type);

void hccast_test_display_zoom(hccast_test_dis_type_e dis_type, hccast_test_dis_layer_e dis_layer, 
    hccast_test_rect_t *src_rect, hccast_test_rect_t *dst_rect, dis_scale_avtive_mode_e active_mode);

void hccast_test_set_aspect_mode(hccast_test_state_t *cast_state, 
	dis_mode_e dis_mode, dis_scale_avtive_mode_e active_mode);

int hccast_test_rotate_convert(int rotate_init, int rotate_set);
int hccast_test_flip_convert(int dis_type, int flip_init, int flip_set);
int hccast_test_set_vol(int volume);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //end of __HCCAST_TEST_H__