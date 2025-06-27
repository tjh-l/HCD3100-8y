#ifndef __KERNEL__
#include <stdint.h>       
#endif
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <malloc.h>
#include <string.h>
#include <sys/ioctl.h>  
#include <sys/types.h>  
#include <hcuapi/common.h>
#include <hcuapi/hdmi_rx.h>  
#include <hcuapi/dis.h>      
#include <stdlib.h>
#include <stdlib.h>
#include <errno.h>  
#include "hc_test_hdmi_in.h"
#include "boardtest_module.h"
 

#ifdef __HCRTOS__
#include <nuttx/wqueue.h>  
#else
#include <pthread.h>        
#include <sys/epoll.h>    
#include <hcuapi/kumsgq.h>   
#endif

static enum HDMI_RX_VIDEO_DATA_PATH vpath = HDMI_RX_VIDEO_TO_OSD;          
static enum HDMI_RX_AUDIO_DATA_PATH apath = HDMI_RX_AUDIO_BYPASS_TO_HDMI_TX;    
static int rx_fd = -1;
static unsigned int rotate_mode = ROTATE_TYPE_0;
static unsigned int mirror_mode = MIRROR_TYPE_NONE;
static unsigned int stop_mode = 0;
static unsigned int set_edid = false;
static unsigned char *buf_yuv444 = NULL;
static jpeg_enc_quality_type_e hdmi_rx_quality_type = JPEG_ENC_QUALITY_TYPE_NORMAL;
static int hdmi_in_insert_status;
static char *error_message =NULL;  //出错信息

#define HDMI_RX_STATUS_PLUGOUT      0 //  plugout, disconnect
#define HDMI_RX_STATUS_PLUGIN       1 // plugin , connect, playing
#define HDMI_RX_STATUS_ERR_INPUT    2  // this hrx device not support
#define HDMI_RX_STATUS_STOP   		3

struct hdmi_info{
    int fd;
    #ifdef __HCRTOS__
    struct work_notifier_s notify_plugin;
    struct work_notifier_s notify_plugout;
    struct work_notifier_s notify_err_input;
    struct work_notifier_s notify_edid;
    int in_ntkey; //plugin ntkey
    int out_ntkey; //plugin ntkey
    int err_ntkey; //err ntkey
    #else
    pthread_t pid;
    int thread_run;
    int epoll_fd;
    int kumsg_id;
    #endif
    char plug_status;
    char enable;
};
struct hdmi_switch{
    struct hdmi_info in;
    struct hdmi_info out;

    int dis_fd;
    struct dis_display_info bootlogo;
    struct dis_display_info hdmi_rx;
    struct dis_tvsys hdmi_out_tvsys;
    enum TVSYS last_tv_sys;
};
static struct hdmi_switch g_switch_rx;

static int hdmi_rx_stop_test(void)
{
    if(rx_fd >= 0)
    {
        ioctl(rx_fd , HDMI_RX_STOP);
        close(rx_fd);
        rx_fd = -1;
        if(buf_yuv444 != NULL)
        {
            free(buf_yuv444);
            buf_yuv444 = NULL;
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

static int hdmi_rx_start_test(void)
{
    int opt;
      opterr = 0;
    optind = 0;
    if(rx_fd >= 0)
    {
        return 0;
    }

    rx_fd = open("/dev/hdmi_rx" , O_RDWR);//打开"/dev/hdmi_rx"设备文件
    if(rx_fd < 0)
    {
        return -1;
    }

    int argc=3;
    char * argv[]={"start","-v","1",NULL};
    while((opt = getopt(argc , argv , "a:v:r:s:m:e:q:")) != EOF)
    {
        switch(opt)
        {
            case 'a':
                apath = atoi(optarg);
                break;
            case 'v':
                vpath = atoi(optarg);
                break;
            case 'r':
                rotate_mode = atoi(optarg);
                break;
            case 's':
                stop_mode = atoi(optarg);
                break;
            case 'm':
                mirror_mode = atoi(optarg);
                break;
            case 'e':
                set_edid = atoi(optarg);
                break;
            case 'q':
                hdmi_rx_quality_type = atoi(optarg);
                break;
            default:
                break;
        }
    }
    printf("apath %d, vpath %d stop_mode %d hdmi_rx_quality_type %d\n" , apath , vpath , stop_mode, hdmi_rx_quality_type);
    printf("set_edid = %d\n", set_edid);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_DATA_PATH , vpath);
    ioctl(rx_fd , HDMI_RX_SET_AUDIO_DATA_PATH , apath);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_STOP_MODE , stop_mode);
    ioctl(rx_fd , HDMI_RX_SET_VIDEO_ENC_QUALITY , hdmi_rx_quality_type);
    ioctl(rx_fd , HDMI_RX_START);
    printf("hdmi_rx start ok```\n");
    return 0;
}

static void notifier_hdmi_in_plugin(void *arg, unsigned long param)
{
    printf("\n\n %s:%d: \n\n", __func__, __LINE__);
    hdmi_in_insert_status=0;
    g_switch_rx.in.plug_status = HDMI_RX_STATUS_PLUGIN;

    #ifdef HDMI_SWITCH_BACK_BOOTLOGO
    onoff_show_bootlogo(0);
    #endif

    return ;
}

/* start show bootlogo */
static void notifier_hdmi_in_plugout(void *arg, unsigned long param)
{
    printf("\n\n %s:%d: \n\n", __func__, __LINE__);
    hdmi_in_insert_status=-1;

    g_switch_rx.in.plug_status = HDMI_RX_STATUS_PLUGOUT;

    #ifdef HDMI_SWITCH_BACK_BOOTLOGO
    onoff_show_bootlogo(1);
    #endif
    return ;
}


static void notifier_hdmi_in_err(void *arg, unsigned long param){
    printf("\n\n %s:%d: \n\n", __func__, __LINE__);
    g_switch_rx.in.plug_status = HDMI_RX_STATUS_ERR_INPUT;

    #ifdef HDMI_SWITCH_BACK_BOOTLOGO
    onoff_show_bootlogo(1);
    #endif
    return ;

}

static int hdmi_hotplug_rx_enable(void)	//启用 HDMI 热插拔检测功能
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
    return 0;
}

static int hdmi_hotplug_disable(void)	//禁用 hdmi 的热插拔功能
{
    printf("%s:%d: \n", __func__, __LINE__);
    if(g_switch_rx.in.in_ntkey > 0){
        work_notifier_teardown(g_switch_rx.in.in_ntkey);
        g_switch_rx.in.in_ntkey = -1;
    }
    if(g_switch_rx.in.out_ntkey > 0){
        work_notifier_teardown(g_switch_rx.in.out_ntkey);
        g_switch_rx.in.out_ntkey = -1;
    }
    return 0;
}

/**
 * @brief hdmi_in接入状态初始化
 *
 * @param 无参
 * @return  BOARDTEST_FAIL：hdmi_in接入状态初始化失败
 *          BOARDTEST_PASS: hdmi_in接入状态初始化成功
 */
int hc_test_hdmi_in_init(void)
{   
    hdmi_in_insert_status=1;
    int result=0;

    result=hdmi_rx_start_test(); //打开hdmi_rx设备
    if(result != 0){
        error_message="hdmi_rx_start_test is fail";
        write_boardtest_detail(BOARDTEST_HDMI_IN, error_message);//显示出错信息
        return BOARDTEST_FAIL;
    }

    result=hdmi_hotplug_rx_enable(); //开启热拔插
    if(result != 0){
        error_message="hdmi_hotplug_rx_enable is fail";
        write_boardtest_detail(BOARDTEST_HDMI_IN, error_message);//显示出错信息
        return BOARDTEST_FAIL;
    }

    usleep(4*1000*1000);//延迟一段时间，让notifier反应。 经过数百次实验，4秒的延迟较好，成功率 100/100 次

    return BOARDTEST_PASS;
}

/**
 * @brief hdmi_in测试开始
 *
 * @param 无参
 * @return   BOARDTEST_PASS：获取hdmi_in测试运行成功
 *           BOARDTEST_FAIL: 获取hdmi_in测试运行失败
 */
int hc_test_hdmi_in_start(void)
{
    if (hdmi_in_insert_status == 0){
        puts(" \n\n\n hdmi_in_insert_status_plugin \n\n\n");
        error_message="hdmi_in_insert_status_plugin";
        write_boardtest_detail(BOARDTEST_HDMI_IN, error_message);//插入
    }
    else{
        puts("\n\n\n hdmi_in_insert_status_err\n\n\n");
        error_message="hdmi_in_insert_status_err";
        write_boardtest_detail(BOARDTEST_HDMI_IN, error_message);//拔出
        return BOARDTEST_FAIL;
    }

    close_lvgl_osd();//关闭osd层显示
    create_boardtest_passfail_mbox(BOARDTEST_HDMI_IN); //创建一个弹窗
    return BOARDTEST_PASS;
}

/**
 * @brief hdmi_in测试退出
 *
 * @param 无参
 * @return   BOARDTEST_PASS：hdmi_in测试退出函数运行成功
 *           BOARDTEST_FAIL: hdmi_in测试退出函数运行失败
 */
int hc_test_hdmi_in_stop(void)
{    
    int result=0;

    open_lvgl_osd();//打开osd层显示

    result=hdmi_hotplug_disable(); //关闭热拔插
    if(result != 0){
        error_message="hdmi_hotplug_disable is fail";
        write_boardtest_detail(BOARDTEST_HDMI_IN, error_message);//显示出错信息
        return BOARDTEST_FAIL; 
    }

    result=hdmi_rx_stop_test(); //关闭hdmi_rx设备
    if(result != 0){
        error_message="hdmi_rx_stop_test is fail";
        write_boardtest_detail(BOARDTEST_HDMI_IN, error_message);//显示出错信息
        return BOARDTEST_FAIL; 
    }

    return BOARDTEST_PASS;
}

/**
 * @brief hdmi test  register func
 *
 * @return int
 */
static int hc_boardtest_hdmi_in_register(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "HDMI_IN"; 
    test->sort_name = BOARDTEST_HDMI_IN;     
    test->init = hc_test_hdmi_in_init;
    test->run = hc_test_hdmi_in_start;
    test->exit =hc_test_hdmi_in_stop;
    test->tips = "Please select whether the test item passed or not."; 

    hc_boardtest_module_register(test);

    return 0;
}

/**
 * @brief register
 *
 */
__initcall(hc_boardtest_hdmi_in_register);
