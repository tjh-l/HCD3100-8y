#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <string.h>
#include <hcuapi/sci.h>
#include <time.h>
#include <bluetooth.h>
#include <hcuapi/lvds.h>

#ifdef __HCRTOS__
#include <kernel/lib/console.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/completion.h>
#include <hcuapi/gpio.h>
#include <generated/br2_autoconf.h>
#else
#include <termios.h>
#include <signal.h>
#include "console.h"
#include <autoconf.h>
#endif

typedef enum _E_BT_SCAN_STATUS_
{
	BT_SCAN_STATUS_DEFAULT=0,
	BT_SCAN_STATUS_IS_SEARCHING,
	BT_SCAN_STATUS_GET_DATA_SEARCHED,
	BT_SCAN_STATUS_GET_DATA_FINISHED,
}bt_scan_status;

typedef enum _E_BT_CONNECT_STATUS_
{
	BT_CONNECT_STATUS_DEFAULT=0,
	BT_CONNECT_STATUS_DISCONNECTED,
	BT_CONNECT_STATUS_TIMEOUT,
	BT_CONNECT_STATUS_RECONNECTING,
	BT_CONNECT_STATUS_CONNECTED,
	BT_CONNECT_STATUS_GET_CONNECTED_INFO,
}bt_connect_status_e;

static bt_scan_status scan_status;
static bt_connect_status_e connet_status;
unsigned char bluetooth_connect_mac[6]={0x88,0xC4,0x3F,0x89,0x9C,0x52};
void bluetooth_inquiry_printf(struct bluetooth_slave_dev *data)
{
    printf("dev mac : ");
    for(int i=0; i<6; i++)
    printf("%02x ", data->mac[i]);
    printf("\n");
    printf("dev name %s\n",data->name);
}

int bluetooth_callback_test(unsigned long event, unsigned long param)
{
	struct bluetooth_slave_dev dev_info={0};
	struct bluetooth_slave_dev *dev_info_t=NULL;
	struct bluetooth_slave_dev bluetooth_connect={0};
	switch (event)
	{
		case BLUETOOTH_EVENT_SLAVE_DEV_SCANNED:
			printf("BLUETOOTH_EVENT_SLAVE_DEV_SCANNED\n");
			scan_status=BT_SCAN_STATUS_GET_DATA_SEARCHED;
			if(param==0)break;
			dev_info_t=(struct bluetooth_slave_dev*)param;
			memcpy(&dev_info, dev_info_t,sizeof(struct bluetooth_slave_dev));
			bluetooth_inquiry_printf(&dev_info);
			break;
		case BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED:
			printf("BLUETOOTH_EVENT_SLAVE_DEV_SCAN_FINISHED\n");
			scan_status=BT_SCAN_STATUS_GET_DATA_FINISHED;
			break;
		case BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED:
			printf("BLUETOOTH_EVENT_SLAVE_DEV_DISCONNECTED\n");
			connet_status = BT_CONNECT_STATUS_DISCONNECTED;
			break;
		case BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED:
			printf("BLUETOOTH_EVENT_SLAVE_DEV_CONNECTED\n");
			connet_status = BT_CONNECT_STATUS_CONNECTED;
			break;
		case BLUETOOTH_EVENT_SLAVE_DEV_GET_CONNECTED_INFO:
			printf("BLUETOOTH_EVENT_SLAVE_DEV_GET_CONNECTED_INFO\n");
			if(param==0)break;
			memcpy(&bluetooth_connect, (struct bluetooth_slave_dev*)param,sizeof(struct bluetooth_slave_dev));
			bluetooth_inquiry_printf(&bluetooth_connect);
			connet_status = BT_CONNECT_STATUS_GET_CONNECTED_INFO;
			break;
		case BLUETOOTH_EVENT_SLAVE_DEV_CONNECTION_TIMED_OUT:
			connet_status = BT_CONNECT_STATUS_DISCONNECTED;	
			printf("BLUETOOTH_EVENT_SLAVE_DEV_CONNECTION_TIMED_OUT\n");
			break;
		case BLUETOOTH_EVENT_SLAVE_DEV_IS_RECONNECTING:
			connet_status = BT_CONNECT_STATUS_RECONNECTING;
			printf("BLUETOOTH_EVENT_SLAVE_DEV_IS_RECONNECTING\n");
			if(param==0)break;
			dev_info_t=(struct bluetooth_slave_dev*)param;
			memcpy(&dev_info, dev_info_t,sizeof(struct bluetooth_slave_dev));
			bluetooth_inquiry_printf(&dev_info);
			break;
		case BLUETOOTH_EVENT_SLAVE_DEV_IS_SEARCHING:
			scan_status = BT_SCAN_STATUS_IS_SEARCHING;
			printf("BLUETOOTH_EVENT_SLAVE_DEV_IS_SEARCHING\n");
			break;
		default:
			break;
	}
	// printf("bluetooth_callback_test exit\n");
    
    return 0;
}


#ifdef __linux__
static int bt_test_dts_string_get(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);
    if(fd >= 0){
        read(fd, string, size);
        close(fd);
        return 0;
    }
    return -1;
}
#endif

static int fdt_bt_devpath_get(char *devpath, int length)
{
    if (!devpath)
        return -1;

#ifdef __linux__
    int ret = -1;
    char status[16] = {0};

    ret = bt_test_dts_string_get("/proc/device-tree/bluetooth/status", status, sizeof(status));
    if(ret || strcmp(status, "okay"))
    {
        printf("%s(), line:%d. get status fail!\n", __func__, __LINE__);
        return -1;
    }
    ret = bt_test_dts_string_get("/proc/device-tree/bluetooth/devpath", devpath, length);
    if (ret)
    {
        printf("%s(), line:%d. get devpath fail!\n", __func__, __LINE__);
        return -1;
    }
    else
    {
        return 0;
    }

#else
    (void)length;
    const char *dts_path = NULL;

    int np=fdt_node_probe_by_path("/hcrtos/bluetooth");

    if(np>0)
    {
        if(!fdt_get_property_string_index(np, "devpath", 0, (const char**)(&dts_path)))
        {
            strcpy(devpath, dts_path);
            return 0;
        }
    }
    return -1;
#endif    
}

static int bluetooth_init_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    char devpath[32] = {0,};

    if (fdt_bt_devpath_get(devpath, sizeof(devpath)))
        return -1;

    if(bluetooth_init(devpath, bluetooth_callback_test) == 0){
        printf("%s %d bluetooth_init ok\n",__FUNCTION__,__LINE__);
    }else{
        printf("%s %d bluetooth_init error\n",__FUNCTION__,__LINE__);
    }

    bluetooth_ioctl(BLUETOOTH_SET_DEFAULT_CONFIG,NULL);
    bluetooth_set_music_vol(100);
    return 0;
}

static int bluetooth_deinit_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if(bluetooth_deinit()==0)
    {
        printf("deinit success\n");
    }
    else
    {
        printf("deinit error\n");
    }
	return 0;
}

static int bluetooth_power_on_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    connet_status = BT_CONNECT_STATUS_DEFAULT;
    if(bluetooth_poweron()==0)
    {
        printf("Device exists\n");
    }
    else
    {
        printf("Device does not exist\n");
    }
	return 0;
}



static int bluetooth_scan_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    scan_status=BT_SCAN_STATUS_DEFAULT;
    bluetooth_scan();
	return 0;
}

static int bluetooth_bt_stop_scan_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    bluetooth_stop_scan();
	return 0;
}

static int bluetooth_connect_cb(int argc, char *argv[])
{
    char buff[7];
    char temp[3];
    char *ptr;
    int argc_t=argc-1;
    int i=0;
    connet_status = BT_CONNECT_STATUS_DEFAULT;
    if(argc_t == 0)
    {
        bluetooth_connect(bluetooth_connect_mac);
    }
    else if(argc_t==6)
    {
        while(argc_t--)
        {
            memcpy(temp,argv[i+1],2);
            buff[i++] = (char)strtol(temp, &ptr, 16);
        }
        bluetooth_connect((unsigned char*)buff);
    }
	return 0;
}

static int bluetooth_disconnect_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    bluetooth_disconnect();
    connet_status = BT_CONNECT_STATUS_DISCONNECTED;
	return 0;
}

static int bluetooth_bt_power_off_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    bluetooth_poweroff();
    usleep(2000* 1000);
	return 0;
}

static int bluetooth_bt_get_connect_state_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if(bluetooth_is_connected()==0)
    {
        printf("The device is connected to a Bluetooth speaker \n");
    }
    else
    {
        printf("The device is not connected to a Bluetooth speaker\n");
    }

	return 0;
}

static int bluetooth_bt_set_gpio_power_cb(int argc, char *argv[])
{
    char val;
    char temp[3];
    char *ptr;

    if(argc > 1)
    {
        memcpy(temp,argv[1],1);
        val = (char)strtol(temp, &ptr, 16);
        bluetooth_set_gpio_backlight(val);
        printf("val : %d\n",val);
    }
    return 0;
}

static int bluetooth_bt_set_mute_cb(int argc, char *argv[])
{
    char val;
    char temp[3];
    char *ptr;

    if(argc > 1)
    {
        memcpy(temp,argv[1],1);
        val = (char)strtol(temp, &ptr, 16);
        bluetooth_set_gpio_mutu(val);
        printf("val : %d\n", (int)val);
    }
    return 0;
}

static int bluetooth_set_cvbs_aux_mode_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    return bluetooth_set_cvbs_aux_mode();
}

static int bluetooth_set_cvbs_fiber_mode_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    return bluetooth_set_cvbs_fiber_mode();
}

static int bluetooth_set_music_vol_cb(int argc, char *argv[])
{
    /*
    unsigned char val;
    unsigned char temp[5]={0};
    char *ptr;
    int argc_t=argc-1;
    if(argc > 1)
    {
        memcpy(temp,argv[1],strlen(argv[1]));
        val = (unsigned)strtol(temp, &ptr, 16);
        bluetooth_set_music_vol(val);
        printf("val : %d\n",val);
    }
    */
    int val;

    if(argc > 1)
    {
        val = atoi(argv[1]);
        bluetooth_set_music_vol(val);
        printf("val : %d\n",val);
    }

    return 0;
}

static int bluetooth_set_connection_cvbs_aux_mode_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    return bluetooth_set_connection_cvbs_aux_mode();
}

static int bluetooth_set_connection_cvbs_fiber_mode_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    return bluetooth_set_connection_cvbs_fiber_mode();
}

/*
 * configrue bluetooth pinpad function
 * useage: pinmux  22  0  0/1 
 * */
static int bluetooth_set_pinmux_cb(int argc, char *argv[])
{
    if(argc < 4){
        printf("invalid param. usage: pinmux <pinpad> <function> <value> ... \n");
        return -1;
    }
    int function = atoi(argv[2]);
    switch(function){
        case PINMUX_BT_GPIO_INPUT :
        {
            bt_pinmux_set_t pinmux = {0};
            pinmux.pinpad = atoi(argv[1]);
            pinmux.pinset = atoi(argv[2]);
            bluetooth_ioctl(BLUETOOTH_SET_PINMUX,&pinmux);
            break;
        }
        case PINMUX_BT_GPIO_OUT :
        {
            bt_gpio_set_t gpioset = {0};
            gpioset.pinpad = atoi(argv[1]);
            gpioset.value = atoi(argv[3]);
            bluetooth_ioctl(BLUETOOTH_SET_GPIO_OUT,&gpioset);
            break;
        }
        case PINMUX_BT_PWM :
        {
            bt_pinmux_set_t pinmux = {0};
            pinmux.pinpad = atoi(argv[1]);
            pinmux.pinset = atoi(argv[2]);
            bluetooth_ioctl(BLUETOOTH_SET_PINMUX,&pinmux);
            if(argc < 5){
                printf("pwm function invaled param. usage : pinmux <pinpad> <function> <pwm_frequency> <pwm_duty>\n");
                return -1;
            }
            /* set pwm frquency param */
            bt_pwm_param_t pwm_param1 = {0};
            pwm_param1.pinpad = atoi(argv[1]);
            pwm_param1.type = 0 ;
            pwm_param1.value =atoi(argv[3]);
            bluetooth_ioctl(BLUETOOTH_SET_PWM_PARAM,&pwm_param1);
            
            /* set pwm duty param */
            bt_pwm_param_t pwm_param2 = {0};
            pwm_param2.pinpad = atoi(argv[1]);
            pwm_param2.type = 1 ;
            pwm_param2.value =atoi(argv[4]);
            bluetooth_ioctl(BLUETOOTH_SET_PWM_PARAM,&pwm_param2);
            break;
        }
    }
    return 0;
}

/*
 * configure bluetooth dac output function
 * cmdvalue : 
 * CMD_VALUE_PLAYORPAUSE = 0,
 * CMD_VALUE_PREV = 1,
 * CMD_VALUE_NEXT = 2,
 * CMD_VALUE_STOP = 3,
 * CMD_VALUE_VOL_UP = 4,
 * CMD_VALUE_VOL_DOWN = 5,
 * CMD_VALUE_MUTE = 6,
 * CMD_VALUE_UNMUTE = 7,
 * usage : ctrl 6 
 * */
static int bluetooth_set_ctrl_cb(int argc, char *argv[])
{
    if(argc < 2){
        printf("invalid param. usage: dac_ctrl <value> \n");
        return -1;
    }
    int ctrl_value = atoi(argv[1]);
    bluetooth_ioctl(BLUETOOTH_SET_CTRL_CMD,ctrl_value);
    return 0;
}
/*
 * audio channel output set 
 * usage: audio_channel 1/0 .
 * input argv  0 : linein  1: spdif
 * */
static int bluetooth_set_audio_channel_cb(int argc, char *argv[])
{
    if(argc < 2){
        printf("invalid param. usage: audio_channel <value> \n");
        return -1;
    }
    int audio_channel= atoi(argv[1]);
    bluetooth_ioctl(BLUETOOTH_SET_AUDIO_CHANNEL_INPUT,audio_channel);
    return 0;
}

/*
 * blutoooth switch to rx_mode(bluetooth speaker mode)
 * before enter this mode, power on first
 * usege : rx_mode 
 * */
static int bluetooth_power_on_rxmode_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    bluetooth_ioctl(BLUETOOTH_SET_BT_POWER_ON_TO_RX,NULL);
    return 0;
}

/*
 * bluetooth set speaker local name(bluetooth speaker name) 
 * before set bluetooth speaker name, power on first, then
 * enter bluetooth speaker mode 
 * usage : local_name  bt_speaker
 * */
static int bluetooth_set_local_name_cb(int argc, char *argv[])
{
    if(argc <2){
        printf("invalid param usage : local_name <usr_define>\n");
    }
    if(strlen(argv[1]) > 32){
        printf("local_name too long,please input character < 32\n");
    }
    bluetooth_ioctl(BLUETOOTH_SET_LOCAL_NAME,argv[1]);
    return 0;
}

/*
 * bluetooth reboot itself 
 * usage : reboot
 * */
static int bluetooth_reboot_cb(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    bluetooth_ioctl(BLUETOOTH_SET_RESET,NULL);
    return 0 ;
}

/*
 * bluetooth close the channel which apponit,bluetooth must
 * work on some frequence channels,do not let bluetooth
 * only work on a few channel,just close the channel which
 * you want to close. this function need to reboot bluetooth
 * itself to take effect
 * usage : signal_channel 1 
 * */
static int bluetooth_close_signal_channel_map_cb(int argc, char *argv[])
{
    (void)argc;
	struct bluetooth_channel_map map;
    int channel = atoi(argv[1]);
    switch(channel) 
    {
        case 1:
            map.x = 1;
            map.y = 23;
            break;
        case 2:
            map.x = 6;
            map.y = 28;
            break;
        case 3:
            map.x = 11;
            map.y = 33;
            break;
        case 4:
            map.x = 16;
            map.y = 38;
            break;
        case 5:
            map.x = 21;
            map.y = 43;
            break;
        case 6:
            map.x = 26;
            map.y = 48;
            break;
        case 7:
            map.x = 31;
            map.y = 53;
            break;
        case 8:
            map.x = 36;
            map.y = 58;
            break;
        case 9:
            map.x = 41;
            map.y = 63;
            break;
        case 10:
            map.x = 46;
            map.y = 68;
            break;
        case 11:
            map.x = 51;
            map.y = 73;
            break;
        case 12:
            map.x = 56;
            map.y = 78;
            break;
        case 13:
            map.x = 61;
            map.y = 83;
            break;
        case 14:
            map.x = 61;
            map.y = 83;
            break;
        case 255://close all
            map.x = 0;
            map.y = 83;
            break;
        default://open all
            map.x = 128;
            map.y = 128;
            break;
    }
    /*appoint channel map to frequence start and end */
	bluetooth_ioctl(BLUETOOTH_SET_CLOSE_CHANNEL_MAP, &map);
    return 0;
}

static int bluetooth_get_signal_channel_map_cb(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    uint8_t signal_channel[10] = {0};
    bluetooth_ioctl(BLUETOOTH_GET_CHANNEL_MAP,signal_channel);
    printf("bluetooth channel map:");
    for(int i =0; i< 10; i++){
        printf("%x",signal_channel[i]);
    }
    printf("\n");
    return 0;
}

static const char help_init[] = "Bluetooth serial port and task initialization";
static const char help_deinit[] = "Bluetooth serial port and task de-initialization";
static const char help_on[] = "Bluetooth sending power on";
static const char help_scan[] = "Bluetooth search device";
static const char help_stop_scan[] ="Bluetooth stop scan";
static const char help_connet[] = "Bluetooth connected devices, connet [mac addr], for example: connet 88 C4 3F 89 9C 52";
static const char help_disconnet[] = "Bluetooth disconnect_cb";
static const char help_off[] = "Bluetooth power off";
static const char help_get_state[] = "Get Bluetooth state";
static const char help_backlight[] = "Bluetooth settings backlight,for example: backlight 0";
static const char help_mute[] = "Bluetooth settings gpio mute,for example: mute 0";
static const char help_aux[] = "Bluetooth enters the aux mode";
static const char help_fiber[] = "Bluetooth enters fiber mode";
static const char help_c_aux[] = "Bluetooth enters the connected aux mode";
static const char help_c_fiber[] = "Bluetooth enters the connected fiber mode";
static const char help_vol[] = "bluetooth_set_music_vol_cb";
static const char help_pinmux[] = "bluetooth_pinmux_set";
static const char help_ctrl[] = "bluetooth_ctrl_cmd ,such as : mute unmute ... ";
static const char help_audio_channel[] = "bluetooth_audio_channel_input select";
static const char help_rx_mode[] = "bluetooth work on bluetooth speaker mode";
static const char help_local_name[] = "bluetooth set bluetooth speaker local name";
static const char help_reboot[] = "bluetooth reboot itself";
static const char help_close_signal_channel[] = "bluetooth close appoint channel";
static const char help_get_signal_channel[] = "bluetooth get signal channel";

#ifdef __linux__

static struct termios stored_settings;
static void exit_console(int signo)
{
    printf("%s(), signo:%d\n", __func__, signo);
    tcsetattr (0, TCSANOW, &stored_settings);
    exit(0);
}

static void bt_test_cmds_register(struct console_cmd *cmd)
{
    console_register_cmd(cmd, "init", bluetooth_init_cb, CONSOLE_CMD_MODE_SELF, help_init);
    console_register_cmd(cmd, "deinit", bluetooth_deinit_cb, CONSOLE_CMD_MODE_SELF, help_deinit);
    console_register_cmd(cmd, "on", bluetooth_power_on_cb, CONSOLE_CMD_MODE_SELF, help_on);
    console_register_cmd(cmd, "scan", bluetooth_scan_cb, CONSOLE_CMD_MODE_SELF, help_scan);
    console_register_cmd(cmd, "stop_scan", bluetooth_bt_stop_scan_cb, CONSOLE_CMD_MODE_SELF, help_stop_scan);
    console_register_cmd(cmd, "connet", bluetooth_connect_cb, CONSOLE_CMD_MODE_SELF, help_connet);
    console_register_cmd(cmd, "disconnet", bluetooth_disconnect_cb, CONSOLE_CMD_MODE_SELF, help_disconnet);
    console_register_cmd(cmd, "off", bluetooth_bt_power_off_cb, CONSOLE_CMD_MODE_SELF, help_off);
    console_register_cmd(cmd, "get_state", bluetooth_bt_get_connect_state_cb, CONSOLE_CMD_MODE_SELF, help_get_state);
    console_register_cmd(cmd, "backlight", bluetooth_bt_set_gpio_power_cb, CONSOLE_CMD_MODE_SELF, help_backlight);
    console_register_cmd(cmd, "mute", bluetooth_bt_set_mute_cb, CONSOLE_CMD_MODE_SELF, help_mute);
    console_register_cmd(cmd, "aux", bluetooth_set_cvbs_aux_mode_cb, CONSOLE_CMD_MODE_SELF, help_aux);
    console_register_cmd(cmd, "fiber", bluetooth_set_cvbs_fiber_mode_cb, CONSOLE_CMD_MODE_SELF, help_fiber);
    console_register_cmd(cmd, "c_aux", bluetooth_set_connection_cvbs_aux_mode_cb, CONSOLE_CMD_MODE_SELF, help_c_aux);
    console_register_cmd(cmd, "c_fiber", bluetooth_set_connection_cvbs_fiber_mode_cb, CONSOLE_CMD_MODE_SELF, help_c_fiber);
    console_register_cmd(cmd, "vol", bluetooth_set_music_vol_cb, CONSOLE_CMD_MODE_SELF, help_vol);
    console_register_cmd(cmd, "pinmux", bluetooth_set_pinmux_cb, CONSOLE_CMD_MODE_SELF, help_pinmux);
    console_register_cmd(cmd, "ctrl", bluetooth_set_ctrl_cb, CONSOLE_CMD_MODE_SELF, help_ctrl);
    console_register_cmd(cmd, "audio_channel", bluetooth_set_audio_channel_cb, CONSOLE_CMD_MODE_SELF, help_audio_channel);
    console_register_cmd(cmd, "rx_mode", bluetooth_power_on_rxmode_cb, CONSOLE_CMD_MODE_SELF, help_rx_mode);
    console_register_cmd(cmd, "local_name", bluetooth_set_local_name_cb, CONSOLE_CMD_MODE_SELF, help_local_name);
    console_register_cmd(cmd, "reboot", bluetooth_reboot_cb, CONSOLE_CMD_MODE_SELF, help_reboot);
    console_register_cmd(cmd, "close_signal_channel", bluetooth_close_signal_channel_map_cb, CONSOLE_CMD_MODE_SELF, help_close_signal_channel);
    console_register_cmd(cmd, "get_signal_channel", bluetooth_get_signal_channel_map_cb, CONSOLE_CMD_MODE_SELF, help_get_signal_channel);
}

int main(int argc , char *argv[])
{
    (void)argc;
    (void)argv;
    struct termios new_settings;

    tcgetattr(0, &stored_settings);
    new_settings = stored_settings;
    //new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_lflag &= ~(ICANON | ECHO);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &new_settings);

    signal(SIGTERM, exit_console);
    signal(SIGINT, exit_console);
    signal(SIGSEGV, exit_console);
    signal(SIGBUS, exit_console);
    console_init("bluetooth:");
    
    bt_test_cmds_register(NULL);

    console_start();
    exit_console(0);
    return 0;

}


#else

static int bluetooth_test_enter(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return 0;
}

CONSOLE_CMD(bluetooth, NULL, bluetooth_test_enter, CONSOLE_CMD_MODE_SELF, "enter Bluetooth test")
CONSOLE_CMD(init, "bluetooth", bluetooth_init_cb, CONSOLE_CMD_MODE_SELF, help_init)
CONSOLE_CMD(deinit, "bluetooth", bluetooth_deinit_cb, CONSOLE_CMD_MODE_SELF, help_deinit)
CONSOLE_CMD(on, "bluetooth", bluetooth_power_on_cb, CONSOLE_CMD_MODE_SELF, help_on)
CONSOLE_CMD(scan, "bluetooth", bluetooth_scan_cb, CONSOLE_CMD_MODE_SELF, help_scan)
CONSOLE_CMD(stop_scan, "bluetooth", bluetooth_bt_stop_scan_cb, CONSOLE_CMD_MODE_SELF, help_stop_scan)
CONSOLE_CMD(connet, "bluetooth", bluetooth_connect_cb, CONSOLE_CMD_MODE_SELF, help_connet)
CONSOLE_CMD(disconnet, "bluetooth", bluetooth_disconnect_cb, CONSOLE_CMD_MODE_SELF, help_disconnet)
CONSOLE_CMD(off, "bluetooth", bluetooth_bt_power_off_cb, CONSOLE_CMD_MODE_SELF, help_off)
CONSOLE_CMD(get_state, "bluetooth", bluetooth_bt_get_connect_state_cb, CONSOLE_CMD_MODE_SELF, help_get_state)
CONSOLE_CMD(backlight, "bluetooth", bluetooth_bt_set_gpio_power_cb, CONSOLE_CMD_MODE_SELF, help_backlight)
CONSOLE_CMD(mute, "bluetooth", bluetooth_bt_set_mute_cb, CONSOLE_CMD_MODE_SELF, help_mute)
CONSOLE_CMD(aux, "bluetooth", bluetooth_set_cvbs_aux_mode_cb, CONSOLE_CMD_MODE_SELF, help_aux)
CONSOLE_CMD(fiber, "bluetooth", bluetooth_set_cvbs_fiber_mode_cb, CONSOLE_CMD_MODE_SELF, help_fiber)
CONSOLE_CMD(c_aux, "bluetooth", bluetooth_set_connection_cvbs_aux_mode_cb, CONSOLE_CMD_MODE_SELF, help_c_aux)
CONSOLE_CMD(c_fiber, "bluetooth", bluetooth_set_connection_cvbs_fiber_mode_cb, CONSOLE_CMD_MODE_SELF, help_c_fiber)
CONSOLE_CMD(vol, "bluetooth", bluetooth_set_music_vol_cb, CONSOLE_CMD_MODE_SELF, help_vol)
CONSOLE_CMD(pinmux, "bluetooth", bluetooth_set_pinmux_cb, CONSOLE_CMD_MODE_SELF, help_pinmux)
CONSOLE_CMD(ctrl, "bluetooth", bluetooth_set_ctrl_cb, CONSOLE_CMD_MODE_SELF, help_ctrl)
CONSOLE_CMD(audio_channel, "bluetooth", bluetooth_set_audio_channel_cb, CONSOLE_CMD_MODE_SELF, help_audio_channel)
CONSOLE_CMD(rx_mode, "bluetooth", bluetooth_power_on_rxmode_cb, CONSOLE_CMD_MODE_SELF, help_rx_mode)
CONSOLE_CMD(local_name, "bluetooth", bluetooth_set_local_name_cb, CONSOLE_CMD_MODE_SELF, help_local_name)
CONSOLE_CMD(reboot, "bluetooth", bluetooth_reboot_cb, CONSOLE_CMD_MODE_SELF, help_reboot)
CONSOLE_CMD(close_signal_channel, "bluetooth", bluetooth_close_signal_channel_map_cb, CONSOLE_CMD_MODE_SELF, help_close_signal_channel)
CONSOLE_CMD(get_signal_channel, "bluetooth",bluetooth_get_signal_channel_map_cb,CONSOLE_CMD_MODE_SELF, help_get_signal_channel)

#endif
