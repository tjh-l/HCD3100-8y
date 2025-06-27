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
    custom_init(&guider_ui);
#endif

}
