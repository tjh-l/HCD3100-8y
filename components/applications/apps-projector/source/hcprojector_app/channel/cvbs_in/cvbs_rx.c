
#include "app_config.h"
#include <fcntl.h>
#include <unistd.h>

#ifdef __HCRTOS__
#include <kernel/vfs.h>
#include <kernel/completion.h>
#include <kernel/io.h>
#include <kernel/lib/console.h>
#include <kernel/elog.h>
#else
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

#include <sys/epoll.h>
#include <hcuapi/kumsgq.h>
#endif
#include <errno.h>    

#include <stdio.h>
#include <getopt.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/tvdec.h>
#include <hcuapi/tvtype.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/snd.h>
#include <hcuapi/avsync.h>
#include <hcuapi/auddec.h>

#if defined(CONFIG_APPS_PROJECTOR_HDMI_SWITCH_SUPPORT) || defined(CONFIG_APPS_PROJECTOR_HDMIIN)    
#include "../hdmi_in/hdmi_rx.h"
#endif

#include <stdlib.h>
#include "factory_setting.h"
#include "screen.h"
#include "setup.h"
#include "com_api.h"
#include "hcstring_id.h"
#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif
#include "dummy_api.h"
#include "cvbs_rx.h"
#ifdef HUDI_FLASH_SUPPORT
#include "flash_otp.h"
#endif
#include "app_log.h"

#ifdef CVBSIN_SUPPORT
static struct cvbs_info m_cvbs_in;
static int tv_dec_fd = -1;
#ifdef CVBS_AUDIO_I2SI_I2SO
static int i2so_fd = -1;
static int i2si_fd = -1;
#endif
static enum TVTYPE tv_sys = TV_NTSC;
static bool tv_dec_started = false;

int cvbs_rx_start(void);
int cvbs_rx_stop(void);

static int cvbs_hotplug_rx_enable(void);
static int cbvs_in_hotplug_disable(void);
static int cvbs_audio_open(void);
static void cvbs_audio_close(void);

bool cvbs_is_playing(void)
{
	return tv_dec_started;
}
static void notifier_cvbs_in_plugin(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);
    m_cvbs_in.plug_status = CVBS_SWITCH_HDMI_STATUS_PLUGIN;
#ifdef CVBS_AUDIO_I2SI_I2SO
    cvbs_audio_open();
#endif  
    return ;
}


static void notifier_cvbs_in_plugout(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);
    m_cvbs_in.plug_status = CVBS_SWITCH_HDMI_STATUS_PLUGOUT;
#ifdef CVBS_AUDIO_I2SI_I2SO
    cvbs_audio_close();
#endif
    return ;
}

static void notifier_cvbs_training_done(void *arg, unsigned long param)
{
//    printf("%s:%d: training done\n", __func__, __LINE__);
    m_cvbs_in.plug_status = CVBS_RX_STATUS_TRAINNING_FINISH;

    return ;
}

static int cvbs_in_plug_status_set(int status)
{
    m_cvbs_in.plug_status = status;
    return 0;
}

#ifdef __HCRTOS__
static int cvbs_hotplug_rx_enable(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    m_cvbs_in.notify_plugin.evtype   = TVDEC_NOTIFY_CONNECT;
   m_cvbs_in.notify_plugin.qid          = LPWORK;
    m_cvbs_in.notify_plugin.remote   = false;
   m_cvbs_in.notify_plugin.oneshot  = false;
    m_cvbs_in.notify_plugin.qualifier  = NULL;
    m_cvbs_in.notify_plugin.arg          = (void *)&m_cvbs_in;
   m_cvbs_in.notify_plugin.worker2  = notifier_cvbs_in_plugin;
    m_cvbs_in.in_ntkey = work_notifier_setup(&m_cvbs_in.notify_plugin);

    m_cvbs_in.notify_plugout.evtype   = TVDEC_NOTIFY_DISCONNECT;
    m_cvbs_in.notify_plugout.qid          = LPWORK;
    m_cvbs_in.notify_plugout.remote   = false;
    m_cvbs_in.notify_plugout.oneshot  = false;
    m_cvbs_in.notify_plugout.qualifier  = NULL;
    m_cvbs_in.notify_plugout.arg          = (void *)&m_cvbs_in;
    m_cvbs_in.notify_plugout.worker2  = notifier_cvbs_in_plugout;
    m_cvbs_in.out_ntkey = work_notifier_setup(& m_cvbs_in.notify_plugout);

    m_cvbs_in.notify_trainning_finish.evtype   = TVDEC_NOTIFY_TRAINING_FINISH;
    m_cvbs_in.notify_trainning_finish.qid          = LPWORK;
    m_cvbs_in.notify_trainning_finish.remote   = false;
    m_cvbs_in.notify_trainning_finish.oneshot  = false;
    m_cvbs_in.notify_trainning_finish.qualifier  = NULL;
    m_cvbs_in.notify_trainning_finish.arg          = (void *)&m_cvbs_in;
    m_cvbs_in.notify_trainning_finish.worker2  = notifier_cvbs_training_done;
    m_cvbs_in.finish_ntkey = work_notifier_setup(&m_cvbs_in.notify_trainning_finish);
    return 0;
}

static int cbvs_in_hotplug_disable(void)
{
    printf("%s:%d: \n", __func__, __LINE__);
	if(m_cvbs_in.in_ntkey > 0){
		work_notifier_teardown(m_cvbs_in.in_ntkey);
		m_cvbs_in.in_ntkey = -1;
	}
	if(m_cvbs_in.out_ntkey > 0){
		work_notifier_teardown(m_cvbs_in.out_ntkey);
		m_cvbs_in.out_ntkey = -1;
	}
    return 0;
}

#else
#define MSG_TASK_STACK_DEPTH (0x2000)
#define MX_EVENTS (10)
#define EPL_TOUT (1000)

typedef struct cvbs_in_info{
    int fd;

    pthread_t pid;
    int thread_run;
    int epoll_fd;
    int kumsg_id;
    char plug_status;
    char enable;
}cvbs_in_info_t;

static cvbs_in_info_t m_cvbs_in_info;


static int cvbs_in_hotplug_disable(void);

static void *receive_cvbs_in_event_func(void *arg)
{
    struct epoll_event events[MX_EVENTS];
    int n = -1;
    int i;

    while(m_cvbs_in_info.thread_run) {
        n = epoll_wait(m_cvbs_in_info.epoll_fd, events, MX_EVENTS, EPL_TOUT);
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
                    case TVDEC_NOTIFY_CONNECT:
                        app_log(LL_DEBUG, "cvbs in connect\n");
                        notifier_cvbs_in_plugin(NULL, 0);
                        break;
                    case TVDEC_NOTIFY_DISCONNECT:
                        app_log(LL_DEBUG, "cbvs in disconnect\n");
                        notifier_cvbs_in_plugout(NULL, 0);
                        break;
                    case TVDEC_NOTIFY_TRAINING_FINISH:
                        app_log(LL_DEBUG, "cbvs training finish\n");
                        notifier_cvbs_training_done(NULL, 0);
                        break;    
                    default:
                        app_log(LL_DEBUG, "%s:%d: unkown message type(%d)\n", __func__, __LINE__, kmsg->type);
                    break;
                }
            }
        }
    }

    printf("%s:%d: \n", __func__, __LINE__);

    return NULL;

}

static int cvbs_hotplug_rx_enable(void)
{
    int ret = 0;
    struct epoll_event ev = {0};
    struct kumsg_event event = {0};
    pthread_attr_t attr;

    m_cvbs_in_info.thread_run = 1;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, MSG_TASK_STACK_DEPTH);

    //if use pthread_join() to wait and recovery the thread resource, DO NOT set the thread to detached state.
    //pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    
    if (pthread_create(&m_cvbs_in_info.pid, &attr, receive_cvbs_in_event_func, (void *)NULL)) {
        printf("pthread_create receive_cvbs_in_event_func fail\n");
        pthread_attr_destroy(&attr);
        goto err;
    }
    pthread_attr_destroy(&attr);
    
    m_cvbs_in_info.epoll_fd = epoll_create1(0);
    if(m_cvbs_in_info.epoll_fd < 0){
        printf("%s:%d:err: epoll_create1 failed\n", __func__, __LINE__);
        goto err;
    }

    m_cvbs_in_info.kumsg_id = ioctl(tv_dec_fd, KUMSGQ_FD_ACCESS, O_CLOEXEC);
    if(m_cvbs_in_info.kumsg_id < 0){
        printf("%s:%d:err: KUMSGQ_FD_ACCESS failed\n", __func__, __LINE__);
        goto err;
    }


    ev.events = EPOLLIN;
    ev.data.ptr = (void *)m_cvbs_in_info.kumsg_id;
    ret = epoll_ctl(m_cvbs_in_info.epoll_fd, EPOLL_CTL_ADD, m_cvbs_in_info.kumsg_id, &ev);
    if(ret != 0){
        printf("%s:%d:err: EPOLL_CTL_ADD failed\n", __func__, __LINE__);
        goto err;
    }

    event.evtype = TVDEC_NOTIFY_CONNECT;
    event.arg = 0;
    ret = ioctl(tv_dec_fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret) {
        printf("KUMSGQ_NOTIFIER_SETUP TVDEC_NOTIFY_CONNECT fail\n");
        goto err;
    }

    event.evtype = TVDEC_NOTIFY_DISCONNECT;
    event.arg = 0;
    ret = ioctl(tv_dec_fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret) {
        printf("KUMSGQ_NOTIFIER_SETUP TVDEC_NOTIFY_DISCONNECT fail\n");
        goto err;
    }

    event.evtype = TVDEC_NOTIFY_TRAINING_FINISH;
    event.arg = 0;
    ret = ioctl(tv_dec_fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret) {
        printf("KUMSGQ_NOTIFIER_SETUP TVDEC_NOTIFY_TRAINING_FINISH fail\n");
        goto err;
    }    

    return 0;

err:
    cbvs_in_hotplug_disable();

    return -1;
}

static int cbvs_in_hotplug_disable(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    m_cvbs_in_info.thread_run = 0;
    if (pthread_join(m_cvbs_in_info.pid, NULL)) {
        printf("thread is not exit...\n");
    }

    if(m_cvbs_in_info.epoll_fd >= 0){
        close(m_cvbs_in_info.epoll_fd);
        m_cvbs_in_info.epoll_fd = -1;
    }

    if(m_cvbs_in_info.kumsg_id >= 0){
        close(m_cvbs_in_info.kumsg_id);
        m_cvbs_in_info.kumsg_id = -1;
    }

    return 0;
}

#endif


int cvbs_get_plug_status(void)
{
    return m_cvbs_in.plug_status;
}
#ifdef CVBS_AUDIO_I2SI_I2SO
/* audio path: cvbs audio--> ADC --> i2si-->i2so 
*	i2si : L01;
*   start i2si
*/
static int cvbs_audio_open(void)
{
	struct snd_pcm_params i2si_params = {0};
	int channels = 2;

	int rate = 44100;
	int periods = 16;
	int bitdepth = 16;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	int align = SND_PCM_ALIGN_RIGHT;
	snd_pcm_access_t access = SND_PCM_ACCESS_RW_INTERLEAVED;
	int period_size = 1536;

	int read_size = period_size;
	int ret = 0;
	snd_pcm_uframes_t poll_size = period_size;
	struct snd_pcm_params params = {0};

	if(	(i2si_fd >0) && (i2so_fd > 0))
		return 0;

	i2si_fd = open("/dev/sndC0i2si", O_WRONLY);
	if(i2si_fd < 0){
		printf("Open /dev/sndC0i2si fail\n");
		return -1;
	}
	api_set_i2so_gpio_mute(true);

	i2si_params.access = access;
	i2si_params.format = format;
	i2si_params.sync_mode = AVSYNC_TYPE_FREERUN;
	i2si_params.align = align;
	i2si_params.rate = rate;
	i2si_params.channels = channels;
	i2si_params.period_size = period_size;
	i2si_params.periods = periods;
	i2si_params.bitdepth = bitdepth;
	i2si_params.start_threshold = 1;
	i2si_params.pcm_source = SND_PCM_SOURCE_AUDPAD;
	i2si_params.pcm_dest = SND_PCM_DEST_BYPASS;

	ret = ioctl(i2si_fd, SND_IOCTL_HW_PARAMS, &i2si_params);
	if (ret < 0) {
		printf("SND_IOCTL_HW_PARAMS error \n");
		close(i2si_fd);
		i2si_fd = -1;
		return -1;
	}

	i2so_fd = open("/dev/sndC0i2so", O_WRONLY);
	if(i2so_fd < 0){
		printf("Open /dev/sndC0i2so error \n");
		close(i2si_fd);
		i2si_fd = -1;
		return -1;
	}

	params.access = SND_PCM_ACCESS_RW_INTERLEAVED;
	params.format = SND_PCM_FORMAT_S16_LE;
	params.sync_mode = AVSYNC_TYPE_FREERUN;
	params.align = align;
	params.rate = rate;

	params.channels = channels;
	params.period_size = read_size;
	params.periods = periods;
	params.bitdepth = bitdepth;
	params.start_threshold = 2;
	ioctl(i2so_fd, SND_IOCTL_HW_PARAMS, &params);
	printf ("SND_IOCTL_HW_PARAMS done\n");

	ioctl(i2so_fd, SND_IOCTL_AVAIL_MIN, &poll_size);

	ioctl(i2si_fd, SND_IOCTL_START, 0);
	printf ("i2si_fd SND_IOCTL_START done\n");

	ioctl(i2so_fd, SND_IOCTL_START, 0);
	printf ("i2so_fd SND_IOCTL_START done\n");
	
	api_set_i2so_gpio_mute(false);
	//sleep(3);
	return 0;
}

static void cvbs_audio_close(void)
{
	printf(">>> cvbs_audio_close\n");	
	api_set_i2so_gpio_mute(true);
	if(i2so_fd > 0)
	{
		printf ("cvbs_audio_close --i2so_fd, SND_IOCTL_HW_FREE \n");
		ioctl(i2so_fd, SND_IOCTL_DROP, 0);
		ioctl(i2so_fd, SND_IOCTL_HW_FREE, 0);
		close(i2so_fd);
		i2so_fd = -1;
	}
	if(i2si_fd > 0){
		printf ("cvbs_audio_close --i2si_fd, SND_IOCTL_HW_FREE \n");
		ioctl(i2si_fd, SND_IOCTL_DROP, 0);
		ioctl(i2si_fd, SND_IOCTL_HW_FREE, 0);
		close(i2si_fd);
		i2si_fd = -1;
	}
	
	api_set_i2so_gpio_mute(false);
}
#endif


static void cbvs_set_gpio_mute(bool mute)//true: mute; false: no mute
{
    int snd_fd = -1;
    uint8_t vol = 0;

    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0) {
        printf ("open snd_fd %d failed\n", snd_fd);
        return;
    }
    //If mute by volume 0, do not need to set gpio mute.
    //because it is auto set to mute in CVBS in.
    ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &vol);
    if (0 == vol){
        close(snd_fd);
        return;
    }

    if (mute){
        //Revert enable sound auto mute. Then sound would auto
        //set mute if no audio data.
        ioctl(snd_fd, SND_IOCTL_SET_AUTO_MUTE_ONOFF, 1);
    } else {
        //Disable sound auto mute first if set unmute.
        ioctl(snd_fd, SND_IOCTL_SET_AUTO_MUTE_ONOFF, 0);
    }
    close(snd_fd);
}

int cvbs_rx_start(void)
{
    int ret = -1;
    sys_param_t * psys_param;
    rotate_type_e rotate_type=ROTATE_TYPE_0;
    mirror_type_e mirror_type=MIRROR_TYPE_NONE;
    int rotate = 0 , h_flip = 0 , v_flip = 0;

    psys_param = projector_get_sys_param();

    if(tv_dec_started == true)
    {
        return 0;
    }

    tv_dec_fd = open("/dev/tv_decoder" , O_WRONLY);
    if(tv_dec_fd < 0)
    {
        return -1;
    }

    api_get_flip_info(&rotate,&h_flip);
    rotate_type = rotate;
    mirror_type = h_flip;

#ifdef BLUETOOTH_SUPPORT
#if PROJECTER_C2_D3000_VERSION
    bluetooth_set_connection_cvbs_aux_mode();
#endif
    bluetooth_ioctl(BLUETOOTH_SET_AUDIO_CHANNEL_INPUT,0);
#endif
    cvbs_hotplug_rx_enable();
	api_set_display_aspect(DIS_TV_16_9, DIS_NORMAL_SCALE);
    set_display_zoom_when_sys_scale();
    
    printf("%s %d rotate_type =%d mirror_type =%d\n",__FUNCTION__,__LINE__,rotate_type,mirror_type);
    ioctl(tv_dec_fd , TVDEC_SET_VIDEO_DATA_PATH , TVDEC_VIDEO_TO_DE_ROTATE);
    ret = ioctl(tv_dec_fd, TVDEC_SET_VIDEO_ROTATE_MODE , rotate_type);
    ioctl(tv_dec_fd, TVDEC_SET_VIDEO_MIRROR_MODE , mirror_type);
    if(ret != 0){
        printf("CVBS_SET_VIDEO_STOP_MODE failed\n");
        return -1;
    }

    if (api_video_pbp_get_support()){
        struct tvdec_display_info dis_info = { 0 };

        if (APP_DIS_TYPE_HD == api_video_pbp_get_dis_type(VIDEO_PBP_SRC_CVBS_IN))
            dis_info.dis_type = DIS_TYPE_HD;
        else
            dis_info.dis_type = DIS_TYPE_UHD;

        if (APP_DIS_LAYER_MAIN == api_video_pbp_get_dis_layer(VIDEO_PBP_SRC_CVBS_IN))
            dis_info.dis_layer = DIS_LAYER_MAIN;
        else
            dis_info.dis_layer = DIS_LAYER_AUXP;

        ioctl(tv_dec_fd , TVDEC_SET_DISPLAY_INFO , &dis_info);
        ioctl(tv_dec_fd , TVDEC_SET_PBP_MODE , VIDEO_PBP_2P_ON);
    }


    cbvs_set_gpio_mute(false);

#ifdef SUPPORT_INPUT_BLUE_SPDIF_IN
    //api_set_i2so_gpio_mute(false);
    bluetooth_channel_slect_support(0x00);//zhp add supprot  bl channel select  spdif   
    //bluetooth_set_volume_level(projector_get_some_sys_param(P_VOLUME));    
#endif     
#ifdef CVBSIN_TRAINING_SUPPORT
    flash_otp_data_init();
    int dc_offset_val = cvbs_training_value_get();

    printf("dc offset va; : %d\n", dc_offset_val);
    if(dc_offset_val > 0 && dc_offset_val < 0xff){
        ioctl(tv_dec_fd, TVDEC_SET_DC_OFFSET, dc_offset_val);
    }else{
        ioctl(tv_dec_fd, TVDEC_SET_DC_OFFSET, 0x80);
    }
#endif
    ioctl(tv_dec_fd, TVDEC_SET_BRIGHTNESS , projector_get_some_sys_param(P_CVBS_GAIN));
    ioctl(tv_dec_fd, TVDEC_START, tv_sys);
    app_log(LL_DEBUG, "tv_dec start ok\n");
    tv_dec_started = true;
    
   
    return 0;
}

void cvbs_set_display_rect(struct vdec_dis_rect* rect)
{
    if(tv_dec_fd > 0){
        ioctl(tv_dec_fd , TVDEC_SET_DISPLAY_RECT , rect);
    }
}

int cvbs_rx_stop(void)
{
#ifdef SYS_ZOOM_SUPPORT
    mainlayer_scale_type_set(MAINLAYER_SCALE_TYPE_NORMAL);
#endif

    if(tv_dec_fd > 0)
    {
#ifdef CVBS_AUDIO_I2SI_I2SO
        cvbs_audio_close();
#endif  
        tv_dec_started = false;

        ioctl(tv_dec_fd , TVDEC_STOP);
        close(tv_dec_fd);
        tv_dec_fd = -1;
#ifdef BLUETOOTH_SUPPORT
        bluetooth_set_connection_cvbs_fiber_mode();
        bluetooth_ioctl(BLUETOOTH_SET_AUDIO_CHANNEL_INPUT,1);
        bluetooth_ioctl(BLUETOOTH_SET_MUSIC_VOL_VALUE,projector_get_some_sys_param(P_VOLUME));
#endif
#ifdef SUPPORT_INPUT_BLUE_SPDIF_IN
        bluetooth_channel_slect_support(0x01);//zhp add supprot  bl channel select  spdif     
        //bluetooth_set_volume_level(100);
#endif  
        cbvs_in_hotplug_disable();
        cbvs_set_gpio_mute(true);
        cvbs_in_plug_status_set(CVBS_SWITCH_HDMI_STATUS_PLUGOUT);

        return 0;
    }
    else
    {
        return -1;
    }
   
}

void cvbs_rx_set_flip_mode(int rotate_type , int mirror_type)
{        
    if(tv_dec_fd>0){
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_DATA_PATH , TVDEC_VIDEO_TO_DE_ROTATE);
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_ROTATE_MODE , rotate_type);
        ioctl(tv_dec_fd , TVDEC_SET_VIDEO_MIRROR_MODE , mirror_type);
 
    }
}

void cvbs_rx_set_gain(int v)
{
    bool tv_dec_fd_delete = false;
    if(tv_dec_fd == -1){
        tv_dec_fd = open("/dev/tv_decoder" , O_WRONLY);
        if(tv_dec_fd < 0) {
            return;
        }        
        tv_dec_fd_delete = true;
    }
    if(v < 0){
        v = 0;
    }
    if(v > 100){
        v = 100;
    }
    projector_set_some_sys_param(P_CVBS_GAIN, v);
    ioctl(tv_dec_fd , TVDEC_SET_BRIGHTNESS , v);
    if(tv_dec_fd_delete){
        close(tv_dec_fd);
        tv_dec_fd = -1;        
    }
}

#ifdef CVBSIN_TRAINING_SUPPORT
static bool if_can_cvbs_trainig(void)
{
    if(tv_dec_started == true) {
        create_message_box(api_rsc_string_get(STR_CVBS_RUNNING));
        return false;
    }

#if defined(CONFIG_APPS_PROJECTOR_HDMI_SWITCH_SUPPORT) || defined(CONFIG_APPS_PROJECTOR_HDMIIN)    
    if (HDMI_RX_STATUS_PLAYING == hdmirx_get_play_status()) {
        create_message_box(api_rsc_string_get(STR_EXIT_HDMI_RX_FIRST));
        return false;
    }
#endif

    if (NULL != api_ffmpeg_player_get()) {
        create_message_box(api_rsc_string_get(STR_EXIT_MP_FIRST));
        return false;
    }

    return true;
}

void cvbs_rx_training()
{
    uint32_t addr;
    int ret;
    int num = 0;
    enum TVDEC_VIDEO_DATA_PATH vpath = TVDEC_VIDEO_TO_DE;
    enum TVTYPE tv_sys = TV_NTSC;
    struct tvdec_training_result training_result = { 0 };
        
    if (false == if_can_cvbs_trainig())
        return;

    tv_dec_fd = open("/dev/tv_decoder" , O_WRONLY);
    if(tv_dec_fd < 0) {
        return ;
    }
    m_cvbs_in.plug_status = -1;
    cvbs_hotplug_rx_enable();
    ioctl(tv_dec_fd , TVDEC_SET_VIDEO_DATA_PATH , vpath);
    ioctl(tv_dec_fd , TVDEC_START , tv_sys);
    ioctl(tv_dec_fd, TVDEC_SET_BRIGHTNESS, 50);
    projector_set_some_sys_param(P_CVBS_GAIN, 50);
    ioctl(tv_dec_fd, TVDEC_SET_DC_OFFSET, 0X80);
    create_message_box(api_rsc_string_get(STR_CVBS_TRAINING));
    lv_refr_now(lv_disp_get_default());
    app_log(LL_DEBUG, "training start\n");
    tv_dec_started = true;
    ioctl(tv_dec_fd, TVDEC_SET_TRAINING_START , tv_sys);

    int count = 100;
    do{// timeout proc
        api_sleep_ms(200);
        count--;
    }while(cvbs_get_plug_status()!=CVBS_RX_STATUS_TRAINNING_FINISH && count > 0);
    
    if(cvbs_get_plug_status() == CVBS_RX_STATUS_TRAINNING_FINISH){
        ioctl(tv_dec_fd , TVDEC_GET_TRAINING_RESULT , &training_result);
        if(training_result.status == TVDEC_TRAINING_STATUS_SUCCESS)
        {
            if(training_result.dc_offset_val>0 && training_result.dc_offset_val<0xff){
                cvbs_training_value_set(training_result.dc_offset_val);
                flash_otp_data_saved();
                create_message_box(api_rsc_string_get(STR_CVBS_TRAINING_OK));                
            }
        }
        app_log(LL_DEBUG, "training_result: stauts: %d, dc_offset_val: %d\n", training_result.status,training_result.dc_offset_val);  
        control_msg_t ctl_msg = {0};
        ctl_msg.msg_type = MSG_TYPE_CVBS_TRAINING_FINISH;
        api_control_send_msg(&ctl_msg);            
    }else{
        create_message_box(api_rsc_string_get(STR_CVBS_RUNNING_FAILED));
    }

    ioctl(tv_dec_fd , TVDEC_STOP);
    close(tv_dec_fd);
    tv_dec_fd = -1;
    tv_dec_started = false;
    cbvs_in_hotplug_disable();
    return;
}
#endif
#endif
