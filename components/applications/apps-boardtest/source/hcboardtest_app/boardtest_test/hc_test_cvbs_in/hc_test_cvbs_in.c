#include <fcntl.h>    
#include <unistd.h>
#include <errno.h>    
#include <stdio.h>
#include <getopt.h>
#include <malloc.h>
#include <sys/ioctl.h>       
#include <sys/types.h>    
#include <hcuapi/tvdec.h>       
#include <hcuapi/vidmp.h>     
#include <stdlib.h> 
#include "hc_test_cvbs_in.h"
#include "boardtest_module.h"

#ifdef __HCRTOS__
#include <nuttx/wqueue.h>
#else
#include <pthread.h>       
#include <sys/epoll.h>  
#include <hcuapi/kumsgq.h>    
#endif

#define CVBS_SWITCH_HDMI_STATUS_PLUGIN 1
#define CVBS_SWITCH_HDMI_STATUS_PLUGOUT 0

struct cvbs_info{
    int fd;
    #ifdef __HCRTOS__
    struct work_notifier_s notify_plugin;
    struct work_notifier_s notify_plugout;
    struct work_notifier_s notify_edid;
    int in_ntkey;
    int out_ntkey;
    #endif    

    char plug_status;
    char enable;
};

static int cvbs_in_insert_status;	//cvbs_in的接入状态
static struct cvbs_info m_cvbs_in;
static bool tv_dec_started = false;
static enum TVDEC_VIDEO_DATA_PATH vpath = TVDEC_VIDEO_TO_DE;
static unsigned int rotate_mode = ROTATE_TYPE_0;
static unsigned int mirror_mode = MIRROR_TYPE_NONE;
static int tv_dec_fd = -1;
static enum TVTYPE tv_sys = TV_NTSC;
static unsigned int stop_mode = 0;
static char *error_message =NULL;  //出错信息

static void notifier_cvbs_in_plugin(void *arg, unsigned long param) // CVBS 插入事件通知的处理函数
{
    printf("\n\n%s:%d: \n\n", __func__, __LINE__);
    cvbs_in_insert_status=0;

    m_cvbs_in.plug_status = CVBS_SWITCH_HDMI_STATUS_PLUGIN;
    #ifdef CVBS_AUDIO_I2SI_I2SO
    cvbs_audio_open();
    #endif  
    return ;
}

static void notifier_cvbs_in_plugout(void *arg, unsigned long param)//用于响应 CVBS 拔出事件，并进行相应的设置和处理。
{
    printf("\n\n %s:%d: \n\n", __func__, __LINE__);   
    cvbs_in_insert_status=-1;

    m_cvbs_in.plug_status = CVBS_SWITCH_HDMI_STATUS_PLUGOUT;
    #ifdef CVBS_AUDIO_I2SI_I2SO
    cvbs_audio_close();
    #endif
    return ;
}

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
    return 0;
}

static int cbvs_in_hotplug_disable(void)	//禁用 CVBS 的热插拔功能
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

static int tv_dec_stop(void)
{
    if(tv_dec_fd >= 0)
    {
        tv_dec_started = false;
        ioctl(tv_dec_fd , TVDEC_STOP);
        close(tv_dec_fd);
        tv_dec_fd = -1;
        return 0;
    }
    else
    {
        return -1;
    }
   
}

static int tv_dec_start(void)
{
    int opt;
    int value = 0;

    if(tv_dec_started == true)
    {
        return 0;
    }

    vpath = TVDEC_VIDEO_TO_DE;
    rotate_mode = ROTATE_TYPE_0;
    mirror_mode = MIRROR_TYPE_NONE;

    tv_dec_fd = open("/dev/tv_decoder" , O_WRONLY);
    if(tv_dec_fd < 0)
    {
        return -1;
    }

    int argc=1;
    char * argv[]={"start",NULL};
    while((opt = getopt(argc , argv , "v:r:t:m:s:")) != EOF)
    {
        switch(opt)
        {
            case 'v':
                vpath = atoi(optarg);
                break;
            case 'r':
                rotate_mode = atoi(optarg);
                break;
            case 'm':
                mirror_mode = atoi(optarg);
                break;
            case 't':
                value = atoi(optarg);
                if(value == 0)
                {
                    tv_sys = TV_PAL;
                }
                else
                {
                    tv_sys = TV_NTSC;
                }
                break;
            case 's':
                stop_mode = atoi(optarg);
                break;
            default:
                break;
        }
    }

    printf("vpath %d\n" , vpath);
    ioctl(tv_dec_fd , TVDEC_SET_VIDEO_DATA_PATH , vpath);
    printf("stop_mode = 0x%x\n" , stop_mode);
    ioctl(tv_dec_fd , TVDEC_SET_VIDEO_STOP_MODE , stop_mode);
    ioctl(tv_dec_fd , TVDEC_START , tv_sys);
    printf("tv_dec start ok\n");
    tv_dec_started = true;
    return 0;
}

/**
 * @brief cvbs_in测试初始化
 *
 * @param 无参
 * @return   BOARDTEST_FAIL：cvbs_in初始化失败
 *           BOARDTEST_PASS:cvbs_in初始化成功
 */
int hc_test_cvbs_in_init(void)
{
    cvbs_in_insert_status=1;       
    int result=0;

    result=tv_dec_start(); //打开tv_dec设备
    if(result != 0){
        error_message="tv_dec_start is fail";
        write_boardtest_detail(BOARDTEST_CVBS_IN, error_message);//显示出错信息
        return BOARDTEST_FAIL; 
    }

    

    result=cvbs_hotplug_rx_enable(); //开启热拔插
    if(result != 0){
        error_message="cvbs_hotplug_rx_enable is fail";
        write_boardtest_detail(BOARDTEST_CVBS_IN, error_message);//显示出错信息
        return BOARDTEST_FAIL; 
    }

    usleep(5*1000*1000);  //延迟一段时间，让notifier反应，经过数百次实验，5秒的延迟较好，成功率 100/100 次

    return BOARDTEST_PASS;
}

/**
 * @brief cvbs_in测试开始
 *
 * @param 无参
 * @return   BOARDTEST_FAIL：获取cvbs_in测试失败
 *           BOARDTEST_PASS:获取cvbs_in测试成功
 */
int hc_test_cvbs_in_start(void)
{
    if(cvbs_in_insert_status == 0){
        puts(" \n\n\n notifier_cvbs_in_plugin \n\n\n");
        error_message="notifier_cvbs_in_plugin";
        write_boardtest_detail(BOARDTEST_CVBS_IN, error_message);//插入 
    }
    else{
        puts("\n\n\n cvbs_in_insert_status_err\n\n\n");
        error_message="cvbs_in_insert_status_err";
        write_boardtest_detail(BOARDTEST_CVBS_IN, error_message);//未初始化
        return BOARDTEST_FAIL;  
    }

    close_lvgl_osd();//关闭osd层显示
    create_boardtest_passfail_mbox(BOARDTEST_CVBS_IN); //创建一个弹窗
    return BOARDTEST_PASS;
}

/**
 * @brief cvbs_in接入状态测试退出
 *
 * @param 无参
 * @return   BOARDTEST_PASS：cvbs_in接入状态测试退出函数运行成功
 *           BOARDTEST_FAIL: cvbs_in接入状态测试退出函数运行失败
 */
int hc_test_cvbs_in_stop(void)
{
    int result=0;

    open_lvgl_osd();//打开osd层显示

    result=cbvs_in_hotplug_disable(); //关闭热拔插
    if(result != 0){
        error_message="cbvs_in_hotplug_disable is fail";
        write_boardtest_detail(BOARDTEST_CVBS_IN, error_message);//显示出错信息
        return BOARDTEST_FAIL;
    }

    result=tv_dec_stop();  //关闭tv_dec设备
    if(result != 0){
        error_message="tv_dec_stop is fail";
        write_boardtest_detail(BOARDTEST_CVBS_IN, error_message);//显示出错信息
        return BOARDTEST_FAIL; 
    }

    return BOARDTEST_PASS; //0
}

/**
 * @brief cvbs test  register func
 *
 * @return int
 */
static int hc_boardtest_cvbs_in_register(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "CVBS_IN"; 
    test->sort_name = BOARDTEST_CVBS_IN;      
    test->init = hc_test_cvbs_in_init;
    test->run = hc_test_cvbs_in_start;
    test->exit =hc_test_cvbs_in_stop;
    test->tips = "Please select whether the test item passed or not."; /*mbox tips*/

    hc_boardtest_module_register(test);

    return 0;
}

/**
 * @brief register
 *
 */__initcall(hc_boardtest_cvbs_in_register);