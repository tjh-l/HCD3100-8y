#include "pmu.h"
#include <stdio.h>

#include "com_api.h"

#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif

/**
 * @brief 
 * 
 * @param period_ns :PWM frequency
 * @param duty_pct  :PWM duty cycle
 */
void battery_bt_control_pwm(int period_ns, int duty_pct)
{
#ifdef BLUETOOTH_SUPPORT
    #ifdef BT_AC6956C_GX
    bt_pinmux_set_t pwm_pinpad;
    pwm_pinpad.pinpad=PINPAD_BT_PB2;
    pwm_pinpad.pinset=PINMUX_BT_PWM;

    bt_pwm_param_t pwm_fre;
    pwm_fre.pinpad=PINPAD_BT_PB2;
    pwm_fre.type=0;
    pwm_fre.value=period_ns;

    bt_pwm_param_t pwm_duty;
    pwm_duty.pinpad=PINPAD_BT_PB2;
    pwm_duty.type=1;
    pwm_duty.value=duty_pct;
    bluetooth_ioctl(BLUETOOTH_SET_PINMUX,&pwm_pinpad);
    bluetooth_ioctl(BLUETOOTH_SET_PWM_PARAM,&pwm_fre);
    bluetooth_ioctl(BLUETOOTH_SET_PWM_PARAM,&pwm_duty);
    #endif 
#endif
}

