/*
 * @Description: 
 * @Autor: Yanisin.chen
 * @Date: 2023-08-14 20:25:22
 */
#ifndef __UI__FACTORY_TEST_H__
#define __UI__FACTORY_TEST_H__

#ifdef HC_FACTORY_TEST

extern lv_group_t *facty_g;
extern lv_obj_t * full_color_panel;

/*ITEM INDEX FOR IMAGE SETTING MENU*/ 
typedef enum{
	// ITEM_CHANNEL=0,
	ITEM_IMAGE_MODE,
	ITEM_CONTRAST,
	ITEM_BRIGHTNESS,
	ITEM_COLOR,
	ITEM_SHARPNESS,
	ITEM_COLORTEMP,
	IAMGE_SET_ITEM_MAX
}IAMGE_SET_ITEM;


void full_color_page_test(void *p);
void factory_test_enter_check(uint16_t key);
void ui_factory_settings_init(void);
extern void factory_venhance_set(uint8_t op,int value);
extern int factory_venhance_reload(int channel);


#endif
#endif 
