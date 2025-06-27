#include "app_config.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <hcuapi/standby.h>
#include <hcuapi/lvds.h>
#include <hcuapi/mipi.h>
#include <hcuapi/lcd.h>
#include "screen.h"
#include "factory_setting.h"
#include "com_api.h"

#ifdef CVBSIN_SUPPORT
#include "./channel/cvbs_in/cvbs_rx.h"
#endif

#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif

#ifdef __HCRTOS__
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <kernel/io.h>
#endif
#include "channel/local_mp/media_player.h"
#include "channel/local_mp/mp_ctrlbarpage.h"
#ifdef HDMI_RX_CEC_SUPPORT
#include "channel/hdmi_in/hdmi_rx.h"
#endif
#ifdef WIFI_SUPPORT
#include "network_api.h"
#endif
#include "./volume/volume.h"
#include <hcuapi/gpio.h>

static void standby_pre_process(void);
// pre process before enter standby mode
static void standby_pre_process(void)
{
    int fd;

    //step 1: close display    
    api_set_backlight_brightness(0);
    api_osd_show_onoff(false);
    api_logo_off();
    api_dis_show_onoff(false);

    set_volume1(0);

    //step 2: stop device & save system data	
    hdmi_rx_leave();

#ifdef HDMI_RX_CEC_SUPPORT
    if(projector_get_some_sys_param(P_CEC_ONOFF))
        hudi_cec_standby_device(hdmirx_cec_handle_get(),HUDI_CEC_DEVICE_BROADCAST);
    hdmirx_cec_deinit();	
#endif

    #ifdef CVBSIN_SUPPORT
    cvbs_rx_stop();
    #endif
    media_player_close();

    projector_sys_param_save();

#if PROJECTER_C2_D3000_VERSION
    api_set_i2so_gpio_mute(true);
#endif

#ifdef  BLUETOOTH_SUPPORT   
    printf("%s %dbluetooth disconnect\n",__func__,__LINE__);
    bluetooth_set_gpio_backlight(0);
    bluetooth_set_gpio_mutu(1);
    bluetooth_poweroff();//stop bluetooth
    //switch off power form bt,so soc can standby.
    bluetooth_ioctl(BLUETOOTH_SET_POWER_PINPAD_OFF,NULL);
    api_sleep_ms(200);
    // bluetooth_deinit();
#endif

#ifdef WIFI_PM_SUPPORT
    api_wifi_pm_open();
#endif

#ifdef WIFI_SUPPORT
    network_wifi_module_set(0);
    hccast_stop_services();
    api_wifi_disconnect();
#endif

#ifdef SUPPORT_INPUT_BLUE_SPDIF_IN
    SET_MUTE;
#endif

    //step 3:  lowpower disaplay: lcd/backlight/light-machine etc.
    printf("close lcd/backlight/ etc.\n");
	fd = open("/dev/lvds", O_RDWR);
	if (fd) {
		ioctl(fd, LVDS_SET_GPIO_POWER, 0); //lvds gpio power close
		close(fd);
	}
	fd = open("/dev/mipi", O_RDWR);
	if (fd) {
		ioctl(fd, MIPI_DSI_GPIO_ENABLE, 0); //mipi close gpio enable
		close(fd);
	}
	fd = open("/dev/lcddev", O_RDWR); //lcddev close gpio enable
	if (fd) {
        ioctl(fd, LCD_SET_POWER_GPIO, 0);
        ioctl(fd, LCD_SET_PWM_VCOM, 0);
		close(fd);
	}

    api_dis_suspend();
    api_sleep_ms(100);


}

// the Keys set in DTS, not here
void enter_standby(void)
{
#ifdef SUPPORT_INPUT_BLUE_SPDIF_IN
    SET_MUTE;
#endif
    int fd_standby;    
    
    standby_pre_process();

    fd_standby = open("/dev/standby", O_RDWR);
    if(fd_standby<0){
        printf("Open /dev/standby failed!\n");
        return;
    }

    printf("enter standby!\n");

    api_watchdog_stop();

#ifdef __HCRTOS__    
    taskDISABLE_INTERRUPTS();
#endif    
    ioctl(fd_standby, STANDBY_ENTER, 0);
    close(fd_standby);
}

