#ifndef __AVPARAM_NODE_H_
#define __AVPARAM_NODE_H_

#include "app_config.h"

#ifdef  MULTI_OS_SUPPORT
#include <hcuapi/dis.h>
#include <hcuapi/fb.h>
#include <hcuapi/pq.h>

#define SOUND_DEV   "/dev/sndC0i2so"
#define FB_DEV      "/dev/fb0"

#define GPIO_NUM_MAX   6

typedef enum app_type{
	APP_USBMIRROR,
	APP_VIDEO,
	APP_MUSIC,
	APP_PHOTO,
	APP_ETEXT,
	APP_WIRELESS_CAST,
	APP_AIRP2P,
	APP_MIRRCAST,
	APP_AIRCAST,
	APP_DLNA,
	APP_Y2B,
	APP_IPTV,
	APP_HDMIRX,
	APP_AVIN,
	APP_USBMIRROR_DEVICE,
	APP_RESVED1,
	APP_RESVED2,
	APP_TYPE_MAX,
}app_type_e;

typedef struct gpio_group{
	uint8_t pinpad;
	uint8_t dir; //GPIO_DIR_INPUT, GPIO_DIR_OUTPUT
	uint8_t def_v;
}gpio_def_t;

typedef struct avparam_node{
	uint8_t volume; //0-255
	uint8_t rotate_mode; // 0: using fb do rotate/flip; 1: using lcd do rotate/flip; 
	uint16_t rotate; // rotate_mode=0:0/90/180/270; rotate_mode=1: 0,1,2,3
	uint8_t h_flip; // 0,1
	uint8_t v_flip; // 0,1
	uint8_t pq_en;
	hcfb_lefttop_pos_t start_pos; // 4B
	hcfb_scale_t scale_param; //8B
	hcfb_enhance_t fb_eh;  //24B
	struct dis_keystone_param ks_param;//20B

	//custom config
	gpio_def_t init_gpio_def[GPIO_NUM_MAX];
	gpio_def_t exit_gpio_def[GPIO_NUM_MAX];
	app_type_e cur_app_type;
	uint8_t language;//0~24
	uint8_t priv[64];

	uint8_t reservd[82];
	//struct pq_settings *pq_setting;
}avparam_t;


#endif
#endif//__AVPARAM_NODE_H_

