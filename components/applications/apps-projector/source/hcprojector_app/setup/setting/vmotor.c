#include "app_config.h"
#ifdef __HCRTOS__
#ifdef PROJECTOR_VMOTOR_SUPPORT
#include <string.h>
#include <hcuapi/gpio.h>
#include <sys/ioctl.h>
#include <hcuapi/lvds.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vmotor.h"
#include "factory_setting.h"
#include "screen.h"
#include <sys/poll.h>
#include <hcuapi/input-event-codes.h>
#include <hcuapi/input.h>
#include "com_api.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <kernel/delay.h>
#include <kernel/completion.h>
#include <kernel/lib/fdt_api.h>
#include "key_event.h"
/*********************************
 * @file vmotor.c
 * @author linsen.chen
 * @brief vMotor function enable
 * @version 1.0.1
 * @date 2024-05-29
 * @copyright HUHAI HI CHIP SEMICONDUCTOR CO.LTD.
 * @note dts config
	vmotor{
		padout-in-1 = <PINPAD_LVDS_DP7>;
		padout-in-2 = <PINPAD_LVDS_DN7>;
		padout-in-3 = <PINPAD_LVDS_CP1>;
		padout-in-4 = <PINPAD_LVDS_CN1>;
		padin-critical-point = <PINPAD_T00>;
		status = "okay";
	};
*/
#ifdef VMOTOR_LIMIT_ON
int vMotor_detection_limit(void);
#endif

#define VMOTOR_TEST
#define ANSI_COLOR_RESET   "\033[0m"
#define ANSI_COLOR_RED     "\033[1;31m"
#define ANSI_COLOR_GREEN   "\033[1;32m"
#define ANSI_COLOR_YELLOW  "\033[1;33m"
#define ANSI_COLOR_BLUE    "\033[1;34m"
#define ANSI_COLOR_MAGENTA "\033[1;35m"
#define ANSI_COLOR_CYAN    "\033[1;36m"

#ifdef VMOTOR_TEST
#define vmotor_log_d(fmt, ...) printf(ANSI_COLOR_GREEN "[WARNING] " fmt ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#define vmotor_log_e(fmt, ...) printf(ANSI_COLOR_RED "[ERROR] " fmt ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#define vmotor_printf printf
#else
#define vmotor_log_d
#define vmotor_log_e(fmt, ...) printf(ANSI_COLOR_RED "[ERROR] " fmt ANSI_COLOR_RESET "\n", ##__VA_ARGS__)
#define vmotor_printf
#endif

#define VMOTOR_COUNT_SETP_FORWARD_MAX 321
struct vmotor_module_priv
{
	int lvdsfd;
	int cref;
	int step_count;
	int current_step_count;
	unsigned int step_time;
	unsigned int padout_in_1;
	unsigned int padout_in_2;
	unsigned int padout_in_3;
	unsigned int padout_in_4;
	unsigned int padin_lens_l;
	unsigned int padin_lens_r;
	unsigned int padin_critical_point;
	unsigned char roll_cocus_flag;//0 disabled 1 enabled
	vmotor_direction_e direction;//0 Forward 1 reverse
	unsigned char set_beat;
	char ready_to_exit_flag;
	char vmotor_status;
};

static struct vmotor_module_priv *gvmotor = NULL;
static struct completion vmotor_task_completion;
static int vmotor_task_start_flag = 0;

static void vmotor_read_thread(void *args)
{
#ifdef VMOTOR_LIMIT_ON
    gpio_configure(PINPAD_T00, GPIO_DIR_INPUT);
    if(projector_get_some_sys_param(P_VMOTOR_LIMIT_SINGLE) == -1 && !gpio_get_input(gvmotor->padin_critical_point))
        vMotor_detection_limit();
#endif
	while(!vmotor_task_start_flag)
	{
		while(vMotor_get_step_count()>0)
			vMotor_Roll();

		msleep(100);
	}

	usleep(1000);
	complete(&vmotor_task_completion);
	vTaskDelete(NULL);
}

int vMotor_init(void)
{
	int ret = VMOTOR_RET_SUCCESS;
	int np = 0;
	u32 tmpVal = 0;
	if (gvmotor == NULL) {
		gvmotor = (struct vmotor_module_priv *)malloc(sizeof(struct vmotor_module_priv));
		if (gvmotor == NULL)
			goto error;
		memset(gvmotor, 0, sizeof(struct vmotor_module_priv));
	}

	if (gvmotor->cref == 0) {
		np = fdt_node_probe_by_path("/hcrtos/vmotor");
		if (np) {
			gvmotor->padout_in_1 = INVALID_VALUE_8; //PINPAD_LVDS_DP7;
			gvmotor->padout_in_2 = INVALID_VALUE_8; //PINPAD_LVDS_DN7;
			gvmotor->padout_in_3 = INVALID_VALUE_8; //PINPAD_LVDS_CP1;
			gvmotor->padout_in_4 = INVALID_VALUE_8; //PINPAD_LVDS_CN1;
			gvmotor->padin_lens_l = (unsigned char)INVALID_VALUE_8; //PINPAD_T01;
			gvmotor->padin_lens_r = (unsigned char)INVALID_VALUE_8; //PINPAD_T00;
			gvmotor->padin_critical_point = (unsigned char)INVALID_VALUE_8;
			fdt_get_property_u_32_index(np, "padout-in-1", 0, &gvmotor->padout_in_1);
			fdt_get_property_u_32_index(np, "padout-in-2", 0, &gvmotor->padout_in_2);
			fdt_get_property_u_32_index(np, "padout-in-3", 0, &gvmotor->padout_in_3);
			fdt_get_property_u_32_index(np, "padout-in-4", 0, &gvmotor->padout_in_4);
			fdt_get_property_u_32_index(np, "padout-lens-l", 0, &gvmotor->padin_lens_l);
			fdt_get_property_u_32_index(np, "padout-lens-r", 0, &gvmotor->padin_lens_r);
			fdt_get_property_u_32_index(np, "padin-critical-point", 0, &gvmotor->padin_critical_point);
		} else {
			vmotor_log_e("No devices found /hcrtos/vmotor\n");
			goto error;
		}

		gvmotor->set_beat = 8;
		gvmotor->vmotor_status = 0;
		gvmotor->step_count = 0; //24 * VMOTOR_COUNT_SETP_FORWARD_MAX;
		gvmotor->step_time = 2000;
		gvmotor->direction = BMOTOR_STEP_FORWARD;
		if (gvmotor->padout_in_1 > PINPAD_MAX || gvmotor->padout_in_2 > PINPAD_MAX || gvmotor->padout_in_3 > PINPAD_MAX || gvmotor->padout_in_4 > PINPAD_MAX) {
			gvmotor->lvdsfd = open("/dev/lvds", O_RDWR);
			if (gvmotor->lvdsfd < 0) {
				vmotor_log_e("%s %d lvds open error\n", __FUNCTION__, __LINE__);
				goto lvds_error;
			}
		}
		gpio_configure(gvmotor->padout_in_1, GPIO_DIR_OUTPUT); //in 1
		gpio_configure(gvmotor->padout_in_2, GPIO_DIR_OUTPUT); //in 2
		gpio_configure(gvmotor->padout_in_3, GPIO_DIR_OUTPUT); //in 3
		gpio_configure(gvmotor->padout_in_4, GPIO_DIR_OUTPUT); //in 4
		gpio_configure(gvmotor->padin_lens_l, GPIO_DIR_INPUT); //LENS-L
		gpio_configure(gvmotor->padin_lens_r, GPIO_DIR_INPUT); //LENS-R
		gpio_configure(gvmotor->padin_critical_point, GPIO_DIR_INPUT); //LENS-R
		gvmotor->cref++;
		vmotor_log_d("%s %d in_1 :%din_2: %din_3: %din_4: %dlens_l: %dlens_r: %dcritical_point:%d\n",
		       __func__, __LINE__, gvmotor->padout_in_1,
		       gvmotor->padout_in_2, gvmotor->padout_in_3,
		       gvmotor->padout_in_4, gvmotor->padin_lens_l,
		       gvmotor->padin_lens_r, gvmotor->padin_critical_point);

		vmotor_task_start_flag = 0;
		init_completion(&vmotor_task_completion);
		ret = xTaskCreate(vmotor_read_thread, "vmotor_read_thread", 0x1000, &gvmotor, portPRI_TASK_NORMAL, NULL);
		if (ret != pdTRUE) {
			vmotor_log_e("kshm recv thread create failed\n");
			goto taskcreate_error;
		}
	}
	return VMOTOR_RET_SUCCESS;

autofocus_error:
	vmotor_task_start_flag = 1;
	wait_for_completion_timeout(&vmotor_task_completion, 3000);
taskcreate_error:
lvds_error:
	free(gvmotor);
error:
	gvmotor = NULL;
	vmotor_log_e("%s %d init error\n", __FUNCTION__, __LINE__);
	return VMOTOR_RET_ERROR;
}

int vMotor_deinit(void)
{
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;

	if (gvmotor->cref == 0)
		return VMOTOR_RET_ERROR;

	gvmotor->cref--;
	if (gvmotor->cref == 0) {
		vmotor_task_start_flag = 1;
		wait_for_completion_timeout(&vmotor_task_completion, 3000);
		close(gvmotor->lvdsfd);
		free(gvmotor);
		gvmotor = NULL;
	}
	return VMOTOR_RET_SUCCESS;
}

void vMotor_set_lvds_in_out(unsigned char padctl, bool value)
{
	struct lvds_set_gpio pad;

	if (padctl == INVALID_VALUE_8)
		return;

	if (padctl > PINPAD_MAX) {
		if (gvmotor->lvdsfd < 0) {
			vmotor_log_e("open error%s %d\n", __FUNCTION__, __LINE__);
			return;
		}
		pad.padctl = padctl;
		pad.value = value;
		ioctl(gvmotor->lvdsfd, LVDS_SET_GPIO_OUT, &pad);
	} else {
		gpio_set_output(padctl, value);
	}
}

int vMotor_set_direction(vmotor_direction_e val)
{
	int vmotor_count = 0;
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;
	vmotor_count = projector_get_some_sys_param(P_VMOTOR_COUNT);
	if (val == BMOTOR_STEP_FORWARD) {
		if (vmotor_count > VMOTOR_COUNT_SETP_FORWARD_MAX)
			gvmotor->step_count = 0;
	}

	projector_set_some_sys_param(P_VMOTOR_COUNT, vmotor_count);
	gvmotor->direction = val;
	gvmotor->ready_to_exit_flag = 0;
	return VMOTOR_RET_SUCCESS;
}

int vMotor_set_step_time(unsigned int val)
{
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;

	gvmotor->step_time = val;
	return VMOTOR_RET_SUCCESS;
}

int vMotor_set_step_count(int val)
{
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;

	if(val > VMOTOR_COUNT_SETP_FORWARD_MAX)
		gvmotor->step_count = VMOTOR_COUNT_SETP_FORWARD_MAX;
	else
		gvmotor->step_count = val;

	return VMOTOR_RET_SUCCESS;
}

int vMotor_get_step_count(void)
{
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;

	return gvmotor->step_count;
}

int vMotor_Roll_set_cocus_flag(unsigned char val)
{
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;
	gvmotor->roll_cocus_flag = val;
	return VMOTOR_RET_SUCCESS;
}

int vMotor_Roll_get_cocus_flag(void)
{
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;
	return gvmotor->roll_cocus_flag;
}

int vMotor_Standby(void)
{
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;
	vMotor_set_lvds_in_out(gvmotor->padout_in_1, 0); //IN_1
	vMotor_set_lvds_in_out(gvmotor->padout_in_2, 0); //IN_2
	vMotor_set_lvds_in_out(gvmotor->padout_in_3, 0); //IN_3
	vMotor_set_lvds_in_out(gvmotor->padout_in_4, 0); //IN_4
	return VMOTOR_RET_SUCCESS;
}

int vMotor_break(void)
{
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;
	vMotor_set_lvds_in_out(gvmotor->padout_in_1, 1); //IN_1
	vMotor_set_lvds_in_out(gvmotor->padout_in_2, 1); //IN_2
	vMotor_set_lvds_in_out(gvmotor->padout_in_3, 1); //IN_3
	vMotor_set_lvds_in_out(gvmotor->padout_in_4, 1); //IN_4
	return VMOTOR_RET_SUCCESS;
}

int vMotor_Roll(void)
{
#ifdef VMOTOR_LIMIT_ON
static int limit_single = -1;
static bool is_first_pass = 0;
#endif
	int i = 0, j = 0;
	if (gvmotor == NULL)
		goto error;

	if (gvmotor->step_count <= 0) {
		goto vmotor_end;
	}

	gvmotor->vmotor_status = 1;
	gvmotor->step_count--;


	
	
#ifdef VMOTOR_LIMIT_ON
	    limit_single = projector_get_some_sys_param(P_VMOTOR_LIMIT_SINGLE);

    #if LIMIT_LEVEL_SIGNAL == 1
    if(gpio_get_input(gvmotor->padin_critical_point)){
    #else
    if(!gpio_get_input(gvmotor->padin_critical_point)){
    #endif
        if (limit_single == -1){
            if(gvmotor->direction==BMOTOR_STEP_FORWARD)
                limit_single = 0;
            else if (gvmotor->direction==BMOTOR_STEP_BACKWARD)
                limit_single = 1;
        }
    }else
        limit_single = -1;

    projector_set_some_sys_param(P_VMOTOR_LIMIT_SINGLE, limit_single);


	//printf("---------------->direction: %d padin_critical_point:%d ->step_count: %d limt:%d\n", \
        gvmotor->direction , gpio_get_input(gvmotor->padin_critical_point), gvmotor->step_count, limit_single);


    if(gvmotor->direction==BMOTOR_STEP_FORWARD)
    {
        if(limit_single == 0){
            projector_sys_param_save();
            goto vmotor_end;
        }
		gvmotor->current_step_count++;
    }
    else
    {
        if(limit_single == 1){
            projector_sys_param_save();
            goto vmotor_end;
        }
		gvmotor->current_step_count--;
    }
	
#else

	if (gvmotor->ready_to_exit_flag == 0) {
		if (gvmotor->padin_critical_point != INVALID_VALUE_8) {
			if (gpio_get_input(gvmotor->padin_critical_point) && gvmotor->direction == BMOTOR_STEP_BACKWARD) {
				gvmotor->ready_to_exit_flag = 1;
				vmotor_log_d("%s %d %d %d\n", __func__, __LINE__, gvmotor->step_count, gvmotor->current_step_count);
				gvmotor->step_count = 24 - 1;
				gvmotor->current_step_count = 0;
				gvmotor->direction = BMOTOR_STEP_FORWARD;
			}
		}
	}
	
	if (gvmotor->direction == BMOTOR_STEP_FORWARD)
		gvmotor->current_step_count++;
	else
		gvmotor->current_step_count--;

#endif

	for (i = 0; i < gvmotor->set_beat; i++) {
		if (gvmotor->direction == BMOTOR_STEP_FORWARD) {
			j = gvmotor->set_beat - 1 - i;
		} else {
			j = i;
		}

		switch (j) {
		case 0:
			//OUT1--1     OUT2--1     OUT3--1     OUT4--0
			vMotor_set_lvds_in_out(gvmotor->padout_in_1, 1); //IN_1
			vMotor_set_lvds_in_out(gvmotor->padout_in_2, 0); //IN_2
			vMotor_set_lvds_in_out(gvmotor->padout_in_3, 0); //IN_3
			vMotor_set_lvds_in_out(gvmotor->padout_in_4, 0); //IN_4
			break;
		//OUT1--1     OUT2--1     OUT3--0     OUT4--0
		case 1:
			vMotor_set_lvds_in_out(gvmotor->padout_in_1, 1); //IN_1
			vMotor_set_lvds_in_out(gvmotor->padout_in_2, 1); //IN_2
			vMotor_set_lvds_in_out(gvmotor->padout_in_3, 0); //IN_3
			vMotor_set_lvds_in_out(gvmotor->padout_in_4, 0); //IN_4
			break;
		//OUT1--1     OUT2--1     OUT3--0     OUT4--1
		case 2:
			vMotor_set_lvds_in_out(gvmotor->padout_in_1, 0); //IN_1
			vMotor_set_lvds_in_out(gvmotor->padout_in_2, 1); //IN_2
			vMotor_set_lvds_in_out(gvmotor->padout_in_3, 0); //IN_3
			vMotor_set_lvds_in_out(gvmotor->padout_in_4, 0); //IN_4
			break;
		//OUT1--1     OUT2--0     OUT3--0     OUT4--1
		case 3:
			vMotor_set_lvds_in_out(gvmotor->padout_in_1, 0); //IN_1
			vMotor_set_lvds_in_out(gvmotor->padout_in_2, 1); //IN_2
			vMotor_set_lvds_in_out(gvmotor->padout_in_3, 1); //IN_3
			vMotor_set_lvds_in_out(gvmotor->padout_in_4, 0); //IN_4
			break;
		//OUT1--1     OUT2--0     OUT3--1     OUT4--1
		case 4:
			vMotor_set_lvds_in_out(gvmotor->padout_in_1, 0); //IN_1
			vMotor_set_lvds_in_out(gvmotor->padout_in_2, 0); //IN_2
			vMotor_set_lvds_in_out(gvmotor->padout_in_3, 1); //IN_3
			vMotor_set_lvds_in_out(gvmotor->padout_in_4, 0); //IN_4
			break;
		//OUT1--0     OUT2--0     OUT3--1     OUT4--1
		case 5:
			vMotor_set_lvds_in_out(gvmotor->padout_in_1, 0); //IN_1
			vMotor_set_lvds_in_out(gvmotor->padout_in_2, 0); //IN_2
			vMotor_set_lvds_in_out(gvmotor->padout_in_3, 1); //IN_3
			vMotor_set_lvds_in_out(gvmotor->padout_in_4, 1); //IN_4
			break;
		//OUT1--0     OUT2--1     OUT3--1     OUT4--1
		case 6:
			vMotor_set_lvds_in_out(gvmotor->padout_in_1, 0); //IN_1
			vMotor_set_lvds_in_out(gvmotor->padout_in_2, 0); //IN_2
			vMotor_set_lvds_in_out(gvmotor->padout_in_3, 0); //IN_3
			vMotor_set_lvds_in_out(gvmotor->padout_in_4, 1); //IN_4
			break;
		//OUT1--0     OUT2--1     OUT3--1     OUT4--0
		case 7:
			vMotor_set_lvds_in_out(gvmotor->padout_in_1, 1); //IN_1
			vMotor_set_lvds_in_out(gvmotor->padout_in_2, 0); //IN_2
			vMotor_set_lvds_in_out(gvmotor->padout_in_3, 0); //IN_3
			vMotor_set_lvds_in_out(gvmotor->padout_in_4, 1); //IN_4
			break;
		default:
			vMotor_break();
			break;
		}
		usleep(gvmotor->step_time);
	}

	if (gvmotor->step_count <= 0) {
		goto vmotor_end;
	}

	return VMOTOR_RET_SUCCESS;

vmotor_end:
	vMotor_Standby();
	vMotor_Roll_set_cocus_flag(0);
	gvmotor->ready_to_exit_flag = 0;
	gvmotor->step_count = 0;
	gvmotor->vmotor_status = 0;
	vmotor_log_d("%s %d current_step: %d direction: %d\n", __func__, __LINE__, gvmotor->current_step_count, gvmotor->direction);
	if (gvmotor->current_step_count > VMOTOR_COUNT_SETP_FORWARD_MAX)
		gvmotor->current_step_count = VMOTOR_COUNT_SETP_FORWARD_MAX;
	else if(gvmotor->current_step_count < 0)
		gvmotor->current_step_count = 0;

	projector_set_some_sys_param(P_VMOTOR_COUNT, gvmotor->current_step_count);

	return VMOTOR_RET_SUCCESS;
error:
	return VMOTOR_RET_ERROR;
}

int vMotor_Roll_test(void)
{
	vMotor_set_step_count(24);
	vMotor_set_direction(BMOTOR_STEP_FORWARD);
	while (vMotor_get_step_count() > 0)
		vMotor_Roll();

	return VMOTOR_RET_SUCCESS;
}

int vMotor_Roll_cocus(void)
{
	vMotor_set_step_count(VMOTOR_COUNT_SETP_FORWARD_MAX);
	vMotor_set_direction(BMOTOR_STEP_BACKWARD);
	vMotor_Roll_set_cocus_flag(1);

	return VMOTOR_RET_SUCCESS;
}

int vMotor_Roll_stop(void)
{
	if(vMotor_Roll_get_cocus_flag()) {
		vMotor_set_step_count(0);
	}

	return VMOTOR_RET_SUCCESS;
}

int vMotor_get_current_step_count(void)
{
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;

	return gvmotor->current_step_count;
}

int vMotor_get_work_status(void)
{
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;

	return gvmotor->vmotor_status;
}

int vMotor_set_work_status(int status)
{
	if (gvmotor == NULL)
		return VMOTOR_RET_ERROR;

	gvmotor->vmotor_status = status;
	return VMOTOR_RET_SUCCESS;
}


#ifdef VMOTOR_LIMIT_ON
int vMotor_detection_limit(void)
{
    uint32_t detection_time = lv_tick_get();
    if (gpio_get_input(gvmotor->padin_critical_point))
        return 1;
    if(gvmotor==NULL) return -1;

    int bM_Step_test = 0;
    int old_test_Step = 16;
    int test_Step = 8;
    bool test_direction = 0;

retest:

for(int i = 0; i < test_Step; i++){
    if(test_direction==BMOTOR_STEP_FORWARD)
    {
        bM_Step_test--;
        if(bM_Step_test < 0)
            bM_Step_test=7;
    }
    else
    {
        bM_Step_test++;
        if(bM_Step_test > 7)
            bM_Step_test=0;
    }

    switch(bM_Step_test)
    {
        case 0:
            //OUT1--1     OUT2--1     OUT3--1     OUT4--0
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,0);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,0);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,1);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,0);//IN_4
            break;
        //OUT1--1     OUT2--1     OUT3--0     OUT4--0
        case 1:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,0);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,0);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,1);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,1);//IN_4
            break;
        //OUT1--1     OUT2--1     OUT3--0     OUT4--1
        case 2:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,0);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,0);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,0);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,1);//IN_4
            break;
        //OUT1--1     OUT2--0     OUT3--0     OUT4--1
        case 3:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,1);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,0);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,0);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,1);//IN_4
            break;
        //OUT1--1     OUT2--0     OUT3--1     OUT4--1
        case 4:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,1);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,0);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,0);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,0);//IN_4
            break;
        //OUT1--0     OUT2--0     OUT3--1     OUT4--1
        case 5:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,1);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,1);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,0);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,0);//IN_4
            break;
        //OUT1--0     OUT2--1     OUT3--1     OUT4--1
        case 6:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,0);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,1);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,0);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,0);//IN_4
            break;
        //OUT1--0     OUT2--1     OUT3--1     OUT4--0
        case 7:
            vMotor_set_lvds_in_out(gvmotor->padout_in_1,0);//IN_1
            vMotor_set_lvds_in_out(gvmotor->padout_in_2,1);//IN_2
            vMotor_set_lvds_in_out(gvmotor->padout_in_3,1);//IN_3
            vMotor_set_lvds_in_out(gvmotor->padout_in_4,0);//IN_4
            break;
        default:
            vMotor_break();
            break;
    }
    usleep(gvmotor->step_time);//gvmotor->step_time

//printf("\n========================test_Step:%d old_test_Step:%d bM_Step_test:%d test_direction: %d padin_critical_point: %d\n", test_Step, old_test_Step, bM_Step_test, test_direction, gpio_get_input(gvmotor->padin_critical_point));
    
    
    if(gpio_get_input(gvmotor->padin_critical_point))
        goto detection_finish;
#ifdef FOUCS_POPUP
    if (lv_tick_get() - detection_time > 9000){
        battery_msgbox_msg_open(999, 2000, NULL, NULL);
        return -1; 
    }
#endif

    if(i==test_Step-1)
    {
        old_test_Step+=8;
        test_Step = old_test_Step;
        test_direction = test_direction==BMOTOR_STEP_FORWARD ? BMOTOR_STEP_BACKWARD : BMOTOR_STEP_FORWARD;
        msleep(100);
        goto retest;
    }

}

detection_finish:
    vMotor_set_step_count(192);
    vMotor_set_direction(test_direction);

    return 0;
}
#endif

#endif
#endif
