#include "com_api.h"
#include "pmu.h"
#include "pm5310.h"
#include "app_log.h"
//Initialize device
int pmu_init(void)
{
    return pm5310_i2c_probe();
}

/**
 * @brief 
 * 
 * @param ioctl_cmd :pm register address
 * @param arg    :get data from registers,data to replace
 * @return int 
 */
int pm_ioctl(pmu_cmds ioctl_cmd, void* arg)
{
    int ret = 0;
    unsigned char buf_read;
    int* buf=(int*)arg;
    switch (ioctl_cmd) {
    case PM_GET_BATTERY_LEVEL:
        ret=i2c_read_pm5310_state(REG_BATTERY_LEVEL, &buf_read, 0xf0);
        switch (buf_read) {
        case PM5310_BATTERY_LEVEL_100:
            *buf=PM_BATTERY_LEVEL_100;
            break;
        case PM5310_BATTERY_LEVEL_75:
            *buf=PM_BATTERY_LEVEL_75;
            break;
        case PM5310_BATTERY_LEVEL_50:
            *buf=PM_BATTERY_LEVEL_50;
            break;
        case PM5310_BATTERY_LEVEL_25:
            *buf=PM_BATTERY_LEVEL_25;
            break;
        case PM5310_BATTERY_LEVEL_3:
            *buf=PM_BATTERY_LEVEL_3;
            break;
        }
        break;
    case PM_GET_CHARGING_STATE:
        ret=i2c_read_pm5310_state(REG_CHARGING_STATE, &buf_read, 0x08);
        if (buf_read==PM5310_IN_CHARGING_STATE) {
            *buf=PM_IN_CHARGING_STATE;
        }else {
            *buf=API_FAILURE;
        }
        break;
    case PM_GET_FULL_CHARGE_STATE:
        ret=i2c_read_pm5310_state(REG_FULL_CHARGE_STATE, &buf_read, 0x08);
        if (buf_read==PM5310_NOT_FULL_CHARGED_STATE) {
            *buf=PM_NOT_FULL_CHARGED_STATE;
        }else {
            *buf=API_FAILURE;
        }
        break;
    case PM_SET_LIGHT_LOAD_MODE:
        ret=i2c_write_pm5310_state(REG_LIGHT_LOAD_MODE, 0xe0, *buf);
        break;
    default:
        return API_FAILURE;
    }
    return ret;
}
