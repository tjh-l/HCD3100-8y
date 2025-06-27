#include <stdio.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "hc-porting/hc_lvgl_init.h"
#include "hc_examples/lv_100ask_demo_init_icon.h"
#ifdef __HCRTOS__
#include <kernel/module.h>
#endif

#ifdef __linux__
int main(int argc, char *argv[])
#else
static void lvgl_demo_main(void *args)
#endif
{
	printf("date:%s,time:%s\n", __DATE__, __TIME__);
	hc_lvgl_init();

	/*Create a Demo*/
#if LV_USE_DEMO_WIDGETS == 1
	lv_demo_widgets();
#elif LV_USE_DEMO_STRESS == 1
	lv_demo_stress();
#elif LV_USE_DEMO_MUSIC == 1
	lv_demo_music();
#elif LV_USE_DEMO_BENCHMARK == 1
	lv_demo_benchmark();
#elif CONFIG_LV_HC_UISLIDE_1024x600 == 1 || CONFIG_LV_HC_UISLIDE_1280x720 == 1
	extern void UISlide_init(void);
	UISlide_init();
#else
#error "Havn't configure demo app"
#endif

	printf("%s:%d\n", __func__, __LINE__);

	hc_lvgl_loop(0, NULL, NULL);

	/*lv_example_label_1();*/
	/*Handle LitlevGL tasks (tickless mode)*/
}
#ifdef __HCRTOS__
static int lvgl_demo_start(void)
{
	xTaskCreate(lvgl_demo_main, (const char *)"lvgl_main", configTASK_STACK_DEPTH,
		    NULL, portPRI_TASK_NORMAL, NULL);
	return 0;
}
#if CONFIG_LV_HC_DEMO_AUTO_START
__initcall(lvgl_demo_start)
#endif
#endif
