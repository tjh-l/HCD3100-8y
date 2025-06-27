#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include "com_api.h"
#include "../screen.h" 
#include "app_log.h"

#include "pmu.h"

typedef struct {
    int battery_level;
    int charging;
    int full_charge;
}find_pm_state;

static volatile bool m_power_check_init = false;

/* *hc_power_check_task
*read pm battery Register Status,Send refresh LVGL event information
*/
static void *hc_battery_check_task()
{
    int count=0;
    control_msg_t ctl_msg = {0};
    find_pm_state pmu;
    int battery_level_cmp=-1;
    int charging_cmp=-1;
    int full_charge_cmp=-1;
    int ret = -1;

    pm_write_mode enable_mode={PM_ENABLE_MODE};
    pm_ioctl(PM_SET_LIGHT_LOAD_MODE, &enable_mode);

    while (1) {
        ret = pm_ioctl(PM_GET_CHARGING_STATE, &pmu.charging);
        if(ret < 0){
            app_log(LL_ERROR,"pm_ioctl error");
            usleep(300 * 1000);
            continue;
        }
        if (PM_IN_CHARGING_STATE==pmu.charging) {
            ret = pm_ioctl(PM_GET_FULL_CHARGE_STATE, &pmu.full_charge);
            if(ret < 0){
                app_log(LL_ERROR,"pm_ioctl error");
                usleep(300 * 1000);
                continue;
            }
        }else {
            ret = pm_ioctl(PM_GET_BATTERY_LEVEL, &pmu.battery_level);
            if(ret < 0){
                app_log(LL_ERROR,"pm_ioctl error");
                usleep(300 * 1000);
                continue;
            }
        }

        if (!(pmu.battery_level==battery_level_cmp)) {
            count=0;
            battery_level_cmp=pmu.battery_level;
            ctl_msg.msg_type = MSG_TYPE_PM_BATTERY_MONITOR;
        }else if (!(pmu.charging==charging_cmp)) {
            count=0;
            charging_cmp=pmu.charging;
            ctl_msg.msg_type = MSG_TYPE_PM_BATTERY_MONITOR;
        }else if (!(pmu.full_charge==full_charge_cmp)) {
            count=0;
            full_charge_cmp=pmu.full_charge;
            ctl_msg.msg_type = MSG_TYPE_PM_BATTERY_MONITOR;
        }else {
            count++;
        }
        
        if ((0 != ctl_msg.msg_type) && (count>=4)){
            api_control_send_msg(&ctl_msg);
            ctl_msg.msg_type = 0;
        }
        if (count>4) {
            count = 0;
        }
        usleep(300000);
    }
}

int battery_screen_init(void)
{
    int ret=0;
    ret = pmu_init();
    if (ret<0) {
        app_log(LL_DEBUG,"pm_charger_read error!!\n");
        return API_FAILURE;
    }

    lv_battery_screen_show();  //power show (LVGL draw)

    printf("Entering %s()!\n", __FUNCTION__);
    if (true == m_power_check_init){
        return API_SUCCESS;
    }
    pthread_t thread_id = 0;
    pthread_attr_t attr;    
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x800);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if(pthread_create(&thread_id, &attr, hc_battery_check_task, NULL)) {
        pthread_attr_destroy(&attr);
        return API_FAILURE;
    }

    pthread_attr_destroy(&attr);
    m_power_check_init = true;
    return API_SUCCESS;
}
