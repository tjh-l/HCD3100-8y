#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H
#ifdef __HCRTOS__
#include <kernel/elog.h>
#endif 
#include "pm5310.h"

#define STANDBY_TIMER 300000  //standby after 300 seconds

typedef enum {
    PM_DISABLE_MODE=0,
    PM_ENABLE_MODE,
}pm_write_mode;

/*battery addr */  
typedef enum {
    PM_GET_BATTERY_LEVEL,  	   //Detect battery level
    PM_GET_CHARGING_STATE,  	  //Detect charging state
    PM_GET_FULL_CHARGE_STATE,  //Detect full charge state
    PM_SET_LIGHT_LOAD_MODE,	//Light load mode switch
}pmu_cmds;

/*battery level */  
typedef enum {
    PM_BATTERY_LEVEL_100,  //capacity >= 75%
    PM_BATTERY_LEVEL_75,  //50% < capacity <= 75%
    PM_BATTERY_LEVEL_50,  //25% < capacity <= 50%
    PM_BATTERY_LEVEL_25,  //3% < capacity <= 25%
    PM_BATTERY_LEVEL_3,  //capacity <= 3%
    PM_IN_CHARGING_STATE,   //charging state
    PM_NOT_FULL_CHARGED_STATE, //not fully charged state
}pmu_state;

void lv_battery_screen_show(void);  //power show (LVGL draw)

void battery_bt_control_pwm(int period_ns, int duty_pct);

int pmu_init(void);
int pm_ioctl(pmu_cmds ioctl_cmd, void* arg);

#endif
