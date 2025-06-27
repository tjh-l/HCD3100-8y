#include <stdio.h>
#include "lvgl.h"
#include "custom.h"
lv_ui guider_ui;


void UISlide_init(void)
{
	/*usleep(5000*1000);*/
#if 0
	lv_obj_t *cont1 = lv_obj_create(lv_scr_act());
	lv_obj_set_style_bg_opa(lv_scr_act(), 100, LV_PART_MAIN|LV_STATE_DEFAULT);

	lv_obj_set_size(cont1, 600, 600);
	lv_obj_set_style_bg_opa(cont1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
#else
#if LV_USE_MONKEY != 0           
	/*Create pointer monkey test*/
	lv_monkey_config_t config;
	lv_monkey_config_init(&config);
	config.type = LV_INDEV_TYPE_POINTER;
	config.period_range.min = 10;
	config.period_range.max = 100;
	lv_monkey_t * monkey = lv_monkey_create(&config);

	/*Start monkey test*/
	lv_monkey_set_enable(monkey, true);
#endif   

    custom_init(&guider_ui);
#endif

}
