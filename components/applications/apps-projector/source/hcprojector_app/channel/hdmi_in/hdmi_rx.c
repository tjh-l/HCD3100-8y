#include "app_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <malloc.h>

#ifdef __HCRTOS__
#include <kernel/io.h>
#include <kernel/lib/console.h>
#include <nuttx/wqueue.h>
#include <kernel/lib/fdt_api.h>
#include <cpu_func.h>
#include <hcuapi/audsink.h>
#include <kernel/completion.h>
#include <kernel/lib/console.h>
#include <nuttx/wqueue.h>
#include <cpu_func.h>
#include <hcuapi/gpio.h>
#include <nuttx/mtd/mtd.h>

#else
//#include <console.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/epoll.h>
#include <hcuapi/kumsgq.h>
#include <mtd/mtd-user.h>
#include "gpio_ctrl.h"
#endif
#include <errno.h>    

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/hdmi_rx.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/dis.h>
#include <hcuapi/viddec.h>
#include <hcuapi/fb.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/snd.h>
#include <hcuapi/audsink.h>
#ifdef HDMI_SWITCH_SUPPORT
#include <hcuapi/ms9601.h>
#endif
#include "hdmi_rx.h"
#include "com_api.h"
#include "factory_setting.h"
#include "screen.h"
#include "setup.h"
#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif

#if PROJECTER_C2_VERSION
#define HDMI_RX_SWITCH_GPIO 2
#else
#define HDMI_RX_SWITCH_GPIO INVALID_VALUE_32
#endif
#include "dummy_api.h"

static struct hdmi_switch g_switch_rx;
static int hdcpkey_set = 0;

#define QT_TEST_EN  1// just for qt hrx test

#ifndef HDMI_SWITCH_HDMI_RX
#define HDMI_SWITCH_HDMI_RX
#endif

int hdmi_rx_leave(void);
int hdmi_rx_enter(void);
static int projector_hdcpkey_load(uint8_t *key);
static int load_hdcpkey_from_udisk(uint8_t *key);


#ifdef __linux__
static pthread_t rx_audio_read_thread_id = 0;
static int akshm_fd = -1;

#define MSG_TASK_STACK_DEPTH (0x2000)
#define MX_EVENTS (10)
#define EPL_TOUT (1000)

#define HDCP_KEY_FILE "/hcdemo_files/hdmirxkey.bin"

#endif


#ifdef CONFIG_APPS_PROJECTOR_SPDIF_OUT
	static int apath = HDMI_RX_AUDIO_BYPASS_TO_SPDIF_OUT;
#else 
	static int apath = HDMI_RX_AUDIO_TO_SPDIF_IN_AND_I2SO;
#endif


static int hdmirx_audio_start(void)
{
    int ret = 0;

#ifdef HDMI_SWITCH_HDMI_RX_TO_HDMI_TX
    ret = ioctl(g_switch_rx.in.fd , HDMI_RX_SET_AUDIO_DATA_PATH , HDMI_RX_AUDIO_BYPASS_TO_HDMI_TX);
    if(ret != 0){
        printf("HDMI_RX_SET_AUDIO_DATA_PATH failed\n");
        return -1;
    }
#else
    ret = ioctl(g_switch_rx.in.fd , HDMI_RX_SET_AUDIO_DATA_PATH , apath);
    if(ret != 0){
        printf("HDMI_RX_SET_AUDIO_DATA_PATH failed\n");
        return -1;
    }
#endif
}
static int hdmirx_audio_stop(void)
{
	return 0;
}
static int hdmirx_start(void)
{
    int ret = 0;
    sys_param_t * psys_param;
    psys_param = projector_get_sys_param();
    rotate_type_e rotate_type=ROTATE_TYPE_0;
    mirror_type_e mirror_type=MIRROR_TYPE_NONE;
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    printf("%s:%d: \n", __func__, __LINE__);
    api_get_flip_info(&rotate , &h_flip);
    rotate_type = rotate;
    mirror_type = h_flip;
	hdmirx_audio_start();

    ret = ioctl(g_switch_rx.in.fd , HDMI_RX_SET_VIDEO_STOP_MODE , 0);
    if(ret != 0){
        printf("HDMI_RX_SET_VIDEO_STOP_MODE failed\n");
        return -1;
    }
    
    ret = ioctl(g_switch_rx.in.fd , HDMI_RX_SET_VIDEO_DATA_PATH , HDMI_RX_VIDEO_TO_DE_ROTATE);
    if(ret != 0){
        printf("HDMI_RX_SET_VIDEO_STOP_MODE failed\n");
        return -1;
    }
    ioctl(g_switch_rx.in.fd, HDMI_RX_SET_VIDEO_ROTATE_MODE , rotate_type);
    ioctl(g_switch_rx.in.fd, HDMI_RX_SET_VIDEO_MIRROR_MODE , mirror_type);
    ioctl(g_switch_rx.in.fd , HDMI_RX_SET_VIDEO_ENC_QUALITY , JPEG_ENC_QUALITY_TYPE_ULTRA_HIGH_QUALITY);

    if (api_video_pbp_get_support()){
        struct hdmi_rx_display_info dis_info = { 0 };

        if (APP_DIS_TYPE_HD == api_video_pbp_get_dis_type(VIDEO_PBP_SRC_CVBS_IN))
            dis_info.dis_type = DIS_TYPE_HD;
        else
            dis_info.dis_type = DIS_TYPE_UHD;

        if (APP_DIS_LAYER_MAIN == api_video_pbp_get_dis_layer(VIDEO_PBP_SRC_CVBS_IN))
            dis_info.dis_layer = DIS_LAYER_MAIN;
        else
            dis_info.dis_layer = DIS_LAYER_AUXP;

        ioctl(g_switch_rx.in.fd, HDMI_RX_SET_DISPLAY_INFO, &dis_info);
        ioctl(g_switch_rx.in.fd, HDMI_RX_SET_PBP_MODE , VIDEO_PBP_2P_ON);
    }

    if(hdcpkey_set == 0)
    {
        struct hdmi_rx_hdcp_key hdcpk;
		/* To do... by Custmors */
		hdcpk.b_encrypted = true;
		hdcpk.seed = 0; // the encryption seed set by the Customer, 16bit		
        if(0==projector_hdcpkey_load(hdcpk.hdcp_key)){
            if((hdcpk.hdcp_key[0]==0xff)&& (hdcpk.hdcp_key[1]==0xff)){
			#if  QT_TEST_EN 
                ret = load_hdcpkey_from_udisk(hdcpk.hdcp_key);
                if(ret <0)
                    printf("No hdcp rx key\n");
                else
                    ret = ioctl(g_switch_rx.in.fd, HDMI_RX_SET_HDCP_KEY , &hdcpk);
			#else			
				printf("No hdcp rx key\n");
				ret = -1;
			#endif         
            }
            else{
                ret = ioctl(g_switch_rx.in.fd, HDMI_RX_SET_HDCP_KEY , &hdcpk);
	            if(ret != 0){
	                printf("Error: HDMI_RX_SET_HDCP_KEY failed\n");
	                //return -1;
	            }
				else
					printf(" HDMI_RX_SET_HDCP_KEY Success!\n");
           	}
            hdcpkey_set = 1;
        }
        else{
            printf("Error: load hdcpkey fail\n");
        }
    }
    api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE);
    set_display_zoom_when_sys_scale();

#ifdef SUPPORT_INPUT_BLUE_SPDIF_IN  
    //api_set_i2so_gpio_mute(false);  
 	bluetooth_channel_slect_support(0x01);
#else
    api_set_i2so_gpio_mute_auto();
#endif      

    ret = ioctl(g_switch_rx.in.fd, HDMI_RX_START);
    if(ret != 0){
        printf("HDMI_RX_START failed\n");
        return -1;
    }

    g_switch_rx.in.enable = 1;
    g_switch_rx.in.play_status = HDMI_RX_STATUS_PLAYING;

    return 0;
}

static int hdmirx_stop(void)
{
    if(g_switch_rx.in.fd > 0){
		hdmirx_audio_stop();
        ioctl(g_switch_rx.in.fd , HDMI_RX_STOP);
        close(g_switch_rx.in.fd);
        g_switch_rx.in.fd = -1;
		g_switch_rx.in.enable = 0;
		
		g_switch_rx.in.play_status = HDMI_RX_STATUS_STOP;
    }

#ifdef __linux__
    if (akshm_fd > 0)
        close(akshm_fd);
    akshm_fd = -1;
#endif

    return 0;
}

void hdmi_rx_set_flip_mode(int rotate_type , int mirror_type)
{        
    if(g_switch_rx.in.fd>0){
        ioctl(g_switch_rx.in.fd , HDMI_RX_SET_VIDEO_DATA_PATH , HDMI_RX_VIDEO_TO_DE_ROTATE);
        ioctl(g_switch_rx.in.fd , HDMI_RX_SET_VIDEO_ROTATE_MODE , rotate_type);
        ioctl(g_switch_rx.in.fd , HDMI_RX_SET_VIDEO_MIRROR_MODE , mirror_type);
    }
}
#ifdef HDMI_SWITCH_SUPPORT
void hdmi_port_sel(uint8_t ch)
{
	int fd = -1;
	fd = open("/dev/ms9601a",O_RDWR);
	if(fd < 0){
		printf("open /dev/ms9601a fail\n");
		return ;
	}
	ioctl(fd, MS9601A_SET_CH_SEL, ch);
	close(fd);
}
#endif

#ifdef HDMI_RX_CEC_SUPPORT 
static uint32_t m_hotkey[] = {KEY_POWER, \
                    KEY_MENU, KEY_HOME,KEY_FORWARD,KEY_BACK, KEY_FLIP,KEY_EPG/*projector-flip*/};

static pthread_mutex_t g_hdmirx_cec_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hdmirx_cec_mutex_lock(void)
{
    pthread_mutex_lock(&g_hdmirx_cec_mutex);
    return 0;
}

static int _hdmirx_cec_mutex_unlock(void)
{
    pthread_mutex_unlock(&g_hdmirx_cec_mutex);
    return 0;
}

static int _hdmirx_cec_device_start(void)
{
    int ret = 0;
    hudi_cec_logical_addresses_t laes = {0};
    hdmirx_cec_device_res_t res = {0};
    hudi_handle hdmirx_handle = hdmirx_cec_handle_get();

    _hdmirx_cec_mutex_lock();

    if(projector_get_some_sys_param(P_CEC_ONOFF) && hdmirx_cec_dev_active_status_get() == 0){
        
        ret = hdmirx_cec_device_scan(hdmirx_handle,&res);
        if(ret < 0 || res.count <= 0)
        {
            _hdmirx_cec_mutex_unlock();
            return -1;
        }

        hdmirx_cec_scan_device_save(&res);

        ret = hdmirx_cec_poweron_device(hdmirx_handle, res);
        if(ret < 0)
        {
            _hdmirx_cec_mutex_unlock();
            return ret;
        }

        ret = hudi_cec_get_active_devices(hdmirx_handle, &laes, 200);
        if(ret < 0)
        {
            _hdmirx_cec_mutex_unlock();
            return ret;
        }

        ret = hdmirx_cec_active_device_sys_param_save(&laes);
        if(ret < 0)
        {
            hdmirx_cec_boardcast_en_set(1);
            printf("hdmirx cec boardcast\n");
        }

        hdmirx_cec_dev_active_status_set(1);
    }

    _hdmirx_cec_mutex_unlock();
    return 0;
}

int hdmirx_cec_process_reload(void)
{
    int ret = _hdmirx_cec_device_start();
    if(ret < 0)
    {
        api_hotkey_enable_set(m_hotkey, sizeof(m_hotkey)/sizeof(m_hotkey[0]));
    }
    return ret; 
}

#endif


/* stop show bootlogo, end show hdmi in picture */
static void notifier_hdmi_in_plugin(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch_rx.in.plug_status = HDMI_RX_STATUS_PLUGIN;

#ifdef HDMI_SWITCH_BACK_BOOTLOGO
    onoff_show_bootlogo(0);
#endif

#ifdef HDMI_RX_CEC_SUPPORT
    int ret = _hdmirx_cec_device_start();
    if(ret >= 0)
    {
        api_hotkey_enable_set(m_hotkey, sizeof(m_hotkey)/sizeof(m_hotkey[0]));
    }
#endif
    
    return ;
}

/* start show bootlogo */
static void notifier_hdmi_in_plugout(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch_rx.in.plug_status = HDMI_RX_STATUS_PLUGOUT;

#ifdef HDMI_SWITCH_BACK_BOOTLOGO
    onoff_show_bootlogo(1);
#endif

#ifdef HDMI_RX_CEC_SUPPORT
    hdmirx_cec_dev_active_status_set(0);
    hdmirx_cec_boardcast_en_set(0);
    api_hotkey_disable_clear();
#endif
    return ;
}


static void notifier_hdmi_in_err(void *arg, unsigned long param){
    printf("%s:%d: \n", __func__, __LINE__);
    g_switch_rx.in.plug_status = HDMI_RX_STATUS_ERR_INPUT;

#ifdef HDMI_SWITCH_BACK_BOOTLOGO
    onoff_show_bootlogo(1);
#endif
    return ;

}

#ifdef __HCRTOS__
static int hdmi_hotplug_rx_enable(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    g_switch_rx.in.notify_plugin.evtype   = HDMI_RX_NOTIFY_CONNECT;
    g_switch_rx.in.notify_plugin.qid          = LPWORK;
    g_switch_rx.in.notify_plugin.remote   = false;
    g_switch_rx.in.notify_plugin.oneshot  = false;
    g_switch_rx.in.notify_plugin.qualifier  = NULL;
    g_switch_rx.in.notify_plugin.arg          = (void *)&g_switch_rx;
    g_switch_rx.in.notify_plugin.worker2  = notifier_hdmi_in_plugin;
    g_switch_rx.in.in_ntkey = work_notifier_setup(&g_switch_rx.in.notify_plugin);

    g_switch_rx.in.notify_plugout.evtype   = HDMI_RX_NOTIFY_DISCONNECT;
    g_switch_rx.in.notify_plugout.qid          = LPWORK;
    g_switch_rx.in.notify_plugout.remote   = false;
    g_switch_rx.in.notify_plugout.oneshot  = false;
    g_switch_rx.in.notify_plugout.qualifier  = NULL;
    g_switch_rx.in.notify_plugout.arg          = (void *)&g_switch_rx;
    g_switch_rx.in.notify_plugout.worker2  = notifier_hdmi_in_plugout;
    g_switch_rx.in.out_ntkey = work_notifier_setup(&g_switch_rx.in.notify_plugout);

    g_switch_rx.in.notify_err_input.evtype = HDMI_RX_NOTIFY_ERR_INPUT;
    g_switch_rx.in.notify_err_input.qid = LPWORK;
    g_switch_rx.in.notify_err_input.remote = false;
    g_switch_rx.in.notify_err_input.oneshot = false;
    g_switch_rx.in.notify_err_input.qualifier = NULL;
    g_switch_rx.in.notify_err_input.arg = (void *)&g_switch_rx;
    g_switch_rx.in.notify_err_input.worker2 = notifier_hdmi_in_err;
    g_switch_rx.in.err_ntkey = work_notifier_setup(&g_switch_rx.in.notify_err_input);

    g_switch_rx.in.plug_status = HDMI_RX_STATUS_PLUGOUT;
    g_switch_rx.in.play_status = HDMI_RX_STATUS_STOP;

    return 0;
}
#else
//linux

static int hdmi_hotplug_disable(void);

static void *receive_hdmi_rx_event_func(void *arg)
{
    struct epoll_event events[MX_EVENTS];
    int n = -1;
    int i;

    while(g_switch_rx.in.thread_run) {
        n = epoll_wait(g_switch_rx.in.epoll_fd, events, MX_EVENTS, EPL_TOUT);
        if(n == -1) {
            if(EINTR == errno) {
                continue;
            }
            usleep(100 * 1000);
            continue;
        } else if(n == 0) {
            continue;
        }

        for(i = 0; i < n; i++) {
            unsigned char msg[MAX_KUMSG_SIZE] = {0};
            int len = 0;
            int kumsg_id = (int)events[i].data.ptr;
            len = read(kumsg_id, msg, MAX_KUMSG_SIZE);
            if(len > 0) {
                KuMsgDH *kmsg = (KuMsgDH *)msg;
                switch (kmsg->type) {
                    case HDMI_RX_NOTIFY_CONNECT:
                        printf("hdmi rx connect\n");
                        notifier_hdmi_in_plugin(NULL, 0);
                        break;
                    case HDMI_RX_NOTIFY_DISCONNECT:
                        printf("hdmi rx disconnect\n");
                        notifier_hdmi_in_plugout(NULL, 0);
                        break;
                    default:
                        printf("%s:%d: unkown message type(%d)\n", __func__, __LINE__, kmsg->type);
                    break;
                }
            }
        }
    }

    printf("%s:%d: \n", __func__, __LINE__);

    return NULL;
}

static int hdmi_hotplug_rx_enable(void)
{
    int ret = 0;
    struct epoll_event ev = {0};
    struct kumsg_event event = {0};
    pthread_attr_t attr;

    /* hdmi in hotplug event */
    g_switch_rx.in.thread_run = 1;
    g_switch_rx.in.plug_status = HDMI_RX_STATUS_PLUGOUT;
    g_switch_rx.in.play_status = HDMI_RX_STATUS_STOP;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, MSG_TASK_STACK_DEPTH);

    //if use pthread_join() to wait and recovery the thread resource, DO NOT set the thread to detached state.
    //pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself

    if (pthread_create(&g_switch_rx.in.pid, &attr, receive_hdmi_rx_event_func, (void *)NULL)) {
        printf("pthread_create receive_hdmi_rx_event_func fail\n");
        pthread_attr_destroy(&attr);
        goto err;
    }
    pthread_attr_destroy(&attr);

    g_switch_rx.in.epoll_fd = epoll_create1(0);
    if(g_switch_rx.in.epoll_fd < 0){
        printf("%s:%d:err: epoll_create1 failed\n", __func__, __LINE__);
        goto err;
    }

    g_switch_rx.in.kumsg_id = ioctl(g_switch_rx.in.fd, KUMSGQ_FD_ACCESS, O_CLOEXEC);
    if(g_switch_rx.in.kumsg_id < 0){
        printf("%s:%d:err: KUMSGQ_FD_ACCESS failed\n", __func__, __LINE__);
        goto err;
    }

    ev.events = EPOLLIN;
    ev.data.ptr = (void *)g_switch_rx.in.kumsg_id;
    ret = epoll_ctl(g_switch_rx.in.epoll_fd, EPOLL_CTL_ADD, g_switch_rx.in.kumsg_id, &ev);
    if(ret != 0){
        printf("%s:%d:err: EPOLL_CTL_ADD failed\n", __func__, __LINE__);
        goto err;
    }

    event.evtype = HDMI_RX_NOTIFY_CONNECT;
    event.arg = 0;
    ret = ioctl(g_switch_rx.in.fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret) {
        printf("KUMSGQ_NOTIFIER_SETUP HDMI_RX_NOTIFY_CONNECT fail\n");
        goto err;
    }

    event.evtype = HDMI_RX_NOTIFY_DISCONNECT;
    event.arg = 0;
    ret = ioctl(g_switch_rx.in.fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret) {
        printf("KUMSGQ_NOTIFIER_SETUP HDMI_RX_NOTIFY_DISCONNECT fail\n");
        goto err;
    }

    return 0;

err:
    hdmi_hotplug_disable();

    return -1;
}

static int hdmi_hotplug_disable(void)
{
    g_switch_rx.in.thread_run = 0;
    if (pthread_join(g_switch_rx.in.pid, NULL)) {
        printf("thread is not exit...\n");
    }

    if(g_switch_rx.in.epoll_fd >= 0){
        close(g_switch_rx.in.epoll_fd);
        g_switch_rx.in.epoll_fd = -1;
    }

    if(g_switch_rx.in.kumsg_id >= 0){
        close(g_switch_rx.in.kumsg_id);
        g_switch_rx.in.kumsg_id = -1;
    }

    return 0;
}
#endif

static int hdmi_hotplug_rx_disable(void)
{
    //printf("%s:%d: \n", __func__, __LINE__);
#ifdef __HCRTOS__
	if(g_switch_rx.in.in_ntkey> 0){
		work_notifier_teardown(g_switch_rx.in.in_ntkey);
		g_switch_rx.in.in_ntkey = -1;
	}
	if(g_switch_rx.in.out_ntkey> 0){
		work_notifier_teardown(g_switch_rx.in.out_ntkey);
		g_switch_rx.in.out_ntkey = -1;
	}
	if(g_switch_rx.in.err_ntkey > 0){
		work_notifier_teardown(g_switch_rx.in.err_ntkey);
		g_switch_rx.in.err_ntkey = -1;
	}
#endif
    return 0;
}
int hdmirx_get_plug_status(void)
{
    return g_switch_rx.in.plug_status;
}
int hdmirx_get_play_status(void)
{
    return g_switch_rx.in.play_status;
}

int hdmirx_pause(void)
{
    printf("%s:%d: \n", __func__, __LINE__);
#ifdef SYS_ZOOM_SUPPORT
    mainlayer_scale_type_set(MAINLAYER_SCALE_TYPE_NORMAL);
#endif
    if( g_switch_rx.in.fd > 0&& (hdmirx_get_play_status() != HDMI_RX_STATUS_PAUSE)){
		//hdmirx_audio_stop();
        ioctl(g_switch_rx.in.fd , HDMI_RX_PAUSE);
        g_switch_rx.in.play_status = HDMI_RX_STATUS_PAUSE;
    }
	else{
		printf("%s-ed %d \n",__func__,g_switch_rx.in.play_status);
		return -1;
	}
    return 0;
}
int hdmirx_resume(void)
{
    printf("%s:%d: \n", __func__, __LINE__);
    api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE);
    set_display_zoom_when_sys_scale();
    /* reset the hdmi display flip mode when into hdmi twice*/ 
    rotate_type_e rotate_type=ROTATE_TYPE_0;
    mirror_type_e mirror_type=MIRROR_TYPE_NONE;
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    api_get_flip_info(&rotate , &h_flip);
    rotate_type = rotate;
    mirror_type = h_flip;

    if( g_switch_rx.in.fd > 0){		
        ioctl(g_switch_rx.in.fd , HDMI_RX_RESUME);
		//hdmirx_audio_start();
        g_switch_rx.in.play_status = HDMI_RX_STATUS_PLAYING;

        int ret = ioctl(g_switch_rx.in.fd , HDMI_RX_SET_VIDEO_DATA_PATH , HDMI_RX_VIDEO_TO_DE_ROTATE);
        if(ret != 0){
            printf("HDMI_RX_SET_VIDEO_STOP_MODE failed\n");
            return -1;
        }
        ioctl(g_switch_rx.in.fd, HDMI_RX_SET_VIDEO_ROTATE_MODE , rotate_type);
        ioctl(g_switch_rx.in.fd, HDMI_RX_SET_VIDEO_MIRROR_MODE , mirror_type);
        
    }
	else{
		printf("hrx closed\n");
		return -1;
	}
    return 0;
}
int hdmi_rx_leave(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

#ifdef SYS_ZOOM_SUPPORT
    mainlayer_scale_type_set(MAINLAYER_SCALE_TYPE_NORMAL);
#endif

    if( g_switch_rx.in.fd > 0){
        hdmirx_stop();
        hdmi_hotplug_rx_disable();
        close( g_switch_rx.in.fd);
        g_switch_rx.in.fd = -1;
	    g_switch_rx.in.play_status = HDMI_RX_STATUS_STOP;
    }

    return 0;
}
static int cur_port = -1;
int hdmi_rx_enter(void)
{
    printf("%s:%d: \n", __func__, __LINE__);
    if(projector_get_some_sys_param(P_CUR_CHANNEL) == SCREEN_CHANNEL_HDMI)
    {
    #ifdef HDMI_SWITCH_SUPPORT
        if(cur_port != MS9601_PORT_0) {
            hdmi_port_sel(MS9601_PORT_0);
            cur_port = MS9601_PORT_0;
        }
    #else //PROJECTER_C2_VERSION
        gpio_configure(HDMI_RX_SWITCH_GPIO, GPIO_DIR_OUTPUT);
        gpio_set_output(HDMI_RX_SWITCH_GPIO, 1);
    #endif
    }
    else
    {
    #ifdef HDMI_SWITCH_SUPPORT
        if(cur_port != MS9601_PORT_1) {
            hdmi_port_sel(MS9601_PORT_1);
            cur_port = MS9601_PORT_1;
        }
	#else //PROJECTER_C2_VERSION
        gpio_configure(HDMI_RX_SWITCH_GPIO, GPIO_DIR_OUTPUT);
	    gpio_set_output(HDMI_RX_SWITCH_GPIO, 0);
	#endif
    }
    if(g_switch_rx.in.fd  <=0)
    {
        g_switch_rx.in.fd = open("/dev/hdmi_rx" , O_RDWR);
        if( g_switch_rx.in.fd < 0){
            printf("open /dev/hdmi_rx failed, ret=%d\n", g_switch_rx.in.fd);
            return -1;
        }

        g_switch_rx.hdmi_rx.distype = DIS_TYPE_HD;
        g_switch_rx.hdmi_rx.info.layer = HDMI_SWITCH_HDMI_RX_LAYER;

        hdmi_hotplug_rx_enable();

        hdmirx_start();
        printf("%s:%d:  really\n", __func__, __LINE__);
    }
    else if(hdmirx_get_play_status() == HDMI_RX_STATUS_PAUSE)
    {
	 	hdmirx_resume();
    }
#ifdef HDMI_RX_CEC_SUPPORT
    int ret = _hdmirx_cec_device_start();
    if(ret >= 0)
    {
        api_hotkey_enable_set(m_hotkey, sizeof(m_hotkey)/sizeof(m_hotkey[0]));
    }
#endif

    return 0;
}

void hdmi_set_display_rect(struct vdec_dis_rect* rect)
{
    printf("---------------%s:%d----------------------\n", __func__, __LINE__);
    if(g_switch_rx.in.fd > 0){
        printf("-%s:%d. %d %d %d %d\n", __func__, __LINE__, \
            rect->dst_rect.x, rect->dst_rect.y, rect->dst_rect.w, rect->dst_rect.h);
        ioctl(g_switch_rx.in.fd, HDMI_RX_SET_DISPLAY_RECT , rect);  
    }
}

#if	QT_TEST_EN 
static void dump_buf(char *tag, uint8_t *buf, int len)
{
     int i;
     
     printf("******%s*******\n", tag);
     for(i = 0; i < len; i++)
    {
         if(i %16==0)
             printf("\n");
           printf("%02x ", buf[i]);
    }
    printf("\n\n");
}

static int projector_hdcpkey_load(uint8_t *key)
{
    int fd = -1;
    int ret = -1;

    char devpath[64] = {0};
    ret = api_get_mtdblock_devpath(devpath, sizeof(devpath), "individual");     
    if (ret < 0) {
        return -1;
    }

    printf("%s devpath:%s\n", __func__, devpath);
    fd = open(devpath, O_RDONLY);
    if (fd < 0) {
        printf("Error:  Open %s failed\n", devpath);
        return -1;
    }

    memset(key, 0, HDCP_KEY_LEN);
    read(fd, key,HDCP_KEY_LEN);
    //dump_buf("read flash/individual", key, HDCP_KEY_LEN);

     close(fd);

	return 0;
}

/* store projector system parameters to flash */
int load_hdcpkey_from_udisk(uint8_t *key)
{
    int fd, ret = -1;    
    uint8_t key_buf[HDCP_KEY_LEN];
    char devpath[64];
    if(mmp_get_usb_stat() == USB_STAT_MOUNT)
    {
        //read from udisk first
		snprintf(devpath,sizeof(devpath),"%s/hdmirxkey.bin",mmp_get_usb_dev_name());
		fd = open(devpath,O_RDWR);
		if(fd < 0){
            printf("open hdmi key file: %s fail!\n", devpath);   
			return -1;
        }
        if(HDCP_KEY_LEN !=read(fd, key_buf, HDCP_KEY_LEN))
        {
            printf("read *.bin fail\n");
            close(fd);
            return -1;
        }
        close(fd);
        fd = -1;
   
        dump_buf("read udisk",  key_buf, HDCP_KEY_LEN);
        //write it to flash/individual
        memset(devpath, 0, sizeof(devpath));
        ret = api_get_mtdblock_devpath(devpath, sizeof(devpath), "individual");     
        if (ret < 0) {
            return -1;
        }
        fd = open(devpath, O_RDWR);
        if (fd <0){
        	printf("Error: open %s failed \n", devpath);
        	return -1;
        }           
    #ifdef __linux__
        //step 1: erase the blcok
        struct mtd_info_user mtd;
        struct erase_info_user erase;
        ioctl(fd, MEMGETINFO, &mtd);
        printf("mtd.erasesize=%d\n", mtd.erasesize);
        erase.start = 0;
        erase.length = mtd.erasesize;
        if (ioctl(fd, MEMERASE, &erase) < 0) {
            printf("Error: erase %s failed \n", devpath);
            close(fd);
            return -1;
        }
    #else
        //in fact, write mtd is OK without erasing.
        //but we do erase to ensure it reased to 0xff.
        struct mtd_geometry_s geo;
        struct mtd_eraseinfo_s eraseinfo;
        ioctl(fd, MTDIOC_GEOMETRY, &geo);
        printf("geo.erasesize=%d\n", (int)geo.erasesize);
        eraseinfo.start = 0;
        eraseinfo.length = geo.erasesize;
        if (ioctl(fd, MTDIOC_MEMERASE, &eraseinfo) < 0) {
            printf("Error: erase %s failed \n", devpath);
            close(fd);
            return -1;
        }
    #endif
        //step 2: write the block
        if(HDCP_KEY_LEN !=write(fd, key_buf, HDCP_KEY_LEN)){
             printf("Error: write %s failed \n", devpath);
             ret = -1;
        }
        else{
            printf("Write key to flash/individual success!\n");    
            fsync(fd);
            close(fd);
            api_system_reboot();
            memcpy(key, key_buf, HDCP_KEY_LEN);
            ret = 0;
        }

        return ret;
    }
    else {
        printf("No usb disk, can not read hdmi key file!\n");  
    }
    return -1;
}
#endif


