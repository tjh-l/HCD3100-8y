/* hccast_test_com.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/ioctl.h>

#include <hcuapi/snd.h>

#ifdef __linux__
#include <termios.h>
#include <signal.h>
#include "console.h"
#include <linux/fb.h>
#else
#include <kernel/lib/console.h>
#include <kernel/fb.h>
#include <kernel/lib/fdt_api.h>

#endif

#include <hcuapi/dis.h>
#include <ffplayer.h>

#include <hccast/hccast_scene.h>
#include <hccast/hccast_net.h>
#include <hccast/hccast_com.h>
#include <hccast/hccast_wifi_mgr.h>

#include "hccast_test.h"

typedef struct{
    uint16_t screen_init_rotate;
    uint16_t screen_init_h_flip;
    uint16_t screen_init_v_flip;
}rotate_cfg_t;

static rotate_cfg_t m_rotate_info[HCCAST_TEST_DIS_TYPE_MAX];

static void hctest_transfer_rotate_mode_for_screen(
                                        int init_rotate,
                                        int init_h_flip,
                                        int init_v_flip,
                                        int *p_rotate_mode ,
                                        int *p_h_flip ,
                                        int *p_v_flip,
                                        int *p_fbdev_rotate);

#ifdef __linux__
void hctest_dts_string_get(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);
    if(fd >= 0){
        read(fd, string, size);
        close(fd);
    }
}

int hctest_dts_uint32_get(const char *path)
{
    int fd = open(path, O_RDONLY);
    int value = -1;
    if(fd >= 0){
        uint8_t buf[4];
        if(read(fd, buf, 4) != 4){
            close(fd);
            return value;
        }
        close(fd);
        value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
    }
    printf("fd:%d ,dts value: %x\n", fd,value);
    return value;
}
#endif

void hctest_screen_rotate_info(void)
{
    static int init_flag = 0;
    if (init_flag)
        return;

    unsigned int rotate=0,h_flip=0,v_flip=0;
#ifdef __HCRTOS__
    int np;

    memset(m_rotate_info, 0, sizeof(m_rotate_info));
    np = fdt_node_probe_by_path("/hcrtos/rotate");
    if(np>=0)
    {
        fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
        fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
        fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
        m_rotate_info[HCCAST_TEST_DIS_TYPE_HD].screen_init_rotate = rotate;
        m_rotate_info[HCCAST_TEST_DIS_TYPE_HD].screen_init_h_flip = h_flip;
        m_rotate_info[HCCAST_TEST_DIS_TYPE_HD].screen_init_v_flip = v_flip;
    }

    np = fdt_node_probe_by_path("/hcrtos/rotate_4k");
    if(np>=0)
    {
        fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
        fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
        fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
        m_rotate_info[HCCAST_TEST_DIS_TYPE_UHD].screen_init_rotate = rotate;
        m_rotate_info[HCCAST_TEST_DIS_TYPE_UHD].screen_init_h_flip = h_flip;
        m_rotate_info[HCCAST_TEST_DIS_TYPE_UHD].screen_init_v_flip = v_flip;
    }

#else
#define ROTATE_CONFIG_PATH "/proc/device-tree/hcrtos/rotate"
#define ROTATE_4K_CONFIG_PATH "/proc/device-tree/hcrtos/rotate_4k"
    char status[16] = {0};
    memset(m_rotate_info, 0, sizeof(m_rotate_info));

    hctest_dts_string_get(ROTATE_CONFIG_PATH "/status", status, sizeof(status));
    if(!strcmp(status, "okay")){
        rotate = hctest_dts_uint32_get(ROTATE_CONFIG_PATH "/rotate");
        h_flip = hctest_dts_uint32_get(ROTATE_CONFIG_PATH "/h_flip");
        v_flip = hctest_dts_uint32_get(ROTATE_CONFIG_PATH "/v_flip");
        m_rotate_info[HCCAST_TEST_DIS_TYPE_HD].screen_init_rotate = rotate;
        m_rotate_info[HCCAST_TEST_DIS_TYPE_HD].screen_init_h_flip = h_flip;
        m_rotate_info[HCCAST_TEST_DIS_TYPE_HD].screen_init_v_flip = v_flip;
    }

    hctest_dts_string_get(ROTATE_4K_CONFIG_PATH "/status", status, sizeof(status));
    if(!strcmp(status, "okay")){
        rotate = hctest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/rotate");
        h_flip = hctest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/h_flip");
        v_flip = hctest_dts_uint32_get(ROTATE_4K_CONFIG_PATH "/v_flip");
        m_rotate_info[HCCAST_TEST_DIS_TYPE_UHD].screen_init_rotate = rotate;
        m_rotate_info[HCCAST_TEST_DIS_TYPE_UHD].screen_init_h_flip = h_flip;
        m_rotate_info[HCCAST_TEST_DIS_TYPE_UHD].screen_init_v_flip = v_flip;
    }

#endif
    init_flag = 1;
    printf("%s()->>> 2k init_rotate = %u h_flip %u v_flip = %u\n", __func__,
        m_rotate_info[HCCAST_TEST_DIS_TYPE_HD].screen_init_rotate,
        m_rotate_info[HCCAST_TEST_DIS_TYPE_HD].screen_init_h_flip,
        m_rotate_info[HCCAST_TEST_DIS_TYPE_HD].screen_init_v_flip);
    printf("%s()->>> 4k init_rotate = %u h_flip %u v_flip = %u\n", __func__,
        m_rotate_info[HCCAST_TEST_DIS_TYPE_UHD].screen_init_rotate,
        m_rotate_info[HCCAST_TEST_DIS_TYPE_UHD].screen_init_h_flip,
        m_rotate_info[HCCAST_TEST_DIS_TYPE_UHD].screen_init_v_flip);
}


static uint16_t hctest_get_screen_init_rotate(int dis_tpye)
{
    if (dis_tpye != HCCAST_TEST_DIS_TYPE_HD && dis_tpye != HCCAST_TEST_DIS_TYPE_UHD)
        return 0;
    return m_rotate_info[dis_tpye].screen_init_rotate;
}

static uint16_t hctest_get_screen_init_h_flip(int dis_tpye)
{
    if (dis_tpye != HCCAST_TEST_DIS_TYPE_HD && dis_tpye != HCCAST_TEST_DIS_TYPE_UHD)
        return 0;

    return m_rotate_info[dis_tpye].screen_init_h_flip;
}

static uint16_t hctest_get_screen_init_v_flip(int dis_tpye)
{
    if (dis_tpye != HCCAST_TEST_DIS_TYPE_HD && dis_tpye != HCCAST_TEST_DIS_TYPE_UHD)
        return 0;

    return m_rotate_info[dis_tpye].screen_init_v_flip;   
}

static void hctest_get_rotate_by_flip_mode(hctest_project_mode_e mode,
                             int *p_rotate_mode ,
                             int *p_h_flip ,
                             int *p_v_flip)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    
    switch(mode)
    {
        case HCTEST_PROJECT_CEILING_REAR:
        {
            //printf("180\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 0;
            v_flip = 0;
            break;
        }

        case  HCTEST_PROJECT_FRONT:
        {
            //printf("h_mirror\n");
            rotate = ROTATE_TYPE_0;
            h_flip = 1;
            v_flip = 0;
            break;
        }

        case HCTEST_PROJECT_CEILING_FRONT:
        {
           // printf("v_mirror\n");
            rotate = ROTATE_TYPE_180;
            h_flip = 1;
            v_flip = 0;
            break;

        }
        case HCTEST_PROJECT_REAR:
        {
            //printf("normal\n");
            rotate = ROTATE_TYPE_0;
            h_flip = 0;
            v_flip = 0;
            break;
        }
        default:
            break;
    }
#ifdef LCD_ROTATE_SUPPORT
    *p_rotate_mode = ROTATE_TYPE_0;
    *p_h_flip = 0;
    *p_v_flip = 0;
#else
    *p_rotate_mode = rotate;
    *p_h_flip = h_flip;
    *p_v_flip = v_flip;
#endif
}

static void hctest_transfer_rotate_mode_for_screen(
                                        int init_rotate,
                                        int init_h_flip,
                                        int init_v_flip,
                                        int *p_rotate_mode ,
                                        int *p_h_flip ,
                                        int *p_v_flip,
                                        int *p_fbdev_rotate)
{
    (void)init_v_flip;
    int fbdev_rotate[4] = { 0,270,180,90 }; //setting is anticlockwise for fvdev
    int rotate = 0 , h_flip = 0 , v_flip = 0;

    rotate = *p_rotate_mode;

    //if screen is V screen,h_flip and v_flip exchange
    if(init_rotate == 0 || init_rotate == 180)
    {
        h_flip = *p_h_flip;
        v_flip = *p_v_flip;
    }
    else
    {
        h_flip = *p_v_flip;
        v_flip = *p_h_flip;
    }
 
    /*setting in dts is anticlockwise */
    /*calc rotate mode*/
    if(init_rotate == 270)
    {
        rotate = (rotate + 1) & 3;
    }
    else if(init_rotate == 90)
    {
        rotate = (rotate + 3) & 3;
    }
    else if(init_rotate == 180)
    {
        rotate = (rotate + 2) & 3;
    }

    /*transfer v_flip to h_flip with rotate
    *rotate 0 + H
    *rotate 0 + V--> rotate 180 +H
    *rotate 180 + H
    *rotate 180 + V --> rotate 0  + H 
    *rotate 90 + H
    *rotate 90 + V--> rotate 270 +H
    *rotate 270 +H
    *rotate 270 +V--> rotate 90 + H 
    */
    if(v_flip == 1)
    {
        switch(rotate)
        {
            case ROTATE_TYPE_0:
                rotate = ROTATE_TYPE_180;
                break;
            case ROTATE_TYPE_90:
                rotate = ROTATE_TYPE_270;
                break;
            case ROTATE_TYPE_180:
                rotate = ROTATE_TYPE_0;
                break;
            case ROTATE_TYPE_270:
                rotate = ROTATE_TYPE_90;
                break;
            default:
                break;
        }
        v_flip = 0;
        h_flip = 1;
    }

    h_flip = h_flip ^ init_h_flip;

    if(p_rotate_mode != NULL)
    {
        *p_rotate_mode = rotate;
    }
    
    if(p_h_flip != NULL)
    {
        *p_h_flip = h_flip;
    }
    
    if(p_v_flip != NULL)
    {
        *p_v_flip = 0;
    }
    
    if(p_fbdev_rotate !=  NULL)
    {
        *p_fbdev_rotate = fbdev_rotate[rotate];
    }

}

int hctest_get_flip_info(hccast_test_state_t *cast_state, int *rotate_type, int *flip_type)
{
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    
    hctest_screen_rotate_info();
    int init_rotate = hctest_get_screen_init_rotate(cast_state->dis_type);
    int init_h_flip = hctest_get_screen_init_h_flip(cast_state->dis_type);
    int init_v_flip = hctest_get_screen_init_v_flip(cast_state->dis_type);

    hctest_get_rotate_by_flip_mode(cast_state->project_mode,
                            &rotate ,
                            &h_flip ,
                            &v_flip);

    hctest_transfer_rotate_mode_for_screen(
        init_rotate,init_h_flip,init_v_flip,
        &rotate , &h_flip , &v_flip , NULL);

    *rotate_type = rotate;
    *flip_type = h_flip;
    return 0;
}


void hccast_test_display_zoom(hccast_test_dis_type_e dis_type, hccast_test_dis_layer_e dis_layer, 
    hccast_test_rect_t *src_rect, hccast_test_rect_t *dst_rect, dis_scale_avtive_mode_e active_mode)
{
    int fd = -1;
    struct dis_zoom dz = { 0 };

    printf("%s(). src_rect:%d %d %d %d, dst_rect:%d %d %d %d\n", __func__,
        src_rect->x, src_rect->y, src_rect->w, src_rect->h,
        dst_rect->x, dst_rect->y, dst_rect->w, dst_rect->h);

    fd = open("/dev/dis", O_WRONLY);
    if(fd < 0){
        return;
    }

    dz.active_mode = active_mode;
    if (HCCAST_TEST_DIS_TYPE_HD == dis_type)
        dz.distype = DIS_TYPE_HD;
    else
        dz.distype = DIS_TYPE_UHD;

    if (HCCAST_TEST_DIS_LAYER_MAIN == dis_layer)
        dz.layer = DIS_LAYER_MAIN;
    else
        dz.layer = DIS_LAYER_AUXP;

    printf("%s(), dis_type[%s], dis_layer[%s]\n", __func__,
        dz.distype==DIS_TYPE_HD ? "2K" : "4K", dz.layer==DIS_LAYER_MAIN ? "Main" : "Auxp");

    dz.src_area.x = src_rect->x;
    dz.src_area.y = src_rect->y;
    dz.src_area.w = src_rect->w;
    dz.src_area.h = src_rect->h;
    dz.dst_area.x = dst_rect->x;
    dz.dst_area.y = dst_rect->y;
    dz.dst_area.w = dst_rect->w;
    dz.dst_area.h = dst_rect->h;

    ioctl(fd , DIS_SET_ZOOM , &dz);
    close(fd);
}


void hccast_test_set_aspect_mode(hccast_test_state_t *cast_state, 
    dis_mode_e dis_mode, dis_scale_avtive_mode_e active_mode)
{
    printf("%s(), line:%d. aspect:%d\n", __func__, __LINE__, dis_mode);                    

    int fd = open("/dev/dis", O_RDWR);
    if ( fd < 0)
        return;
    dis_aspect_mode_t aspect = {0};

    if (HCCAST_TEST_DIS_TYPE_HD == cast_state->dis_type)
    	aspect.distype = DIS_TYPE_HD;
    else
    	aspect.distype = DIS_TYPE_UHD;

    if (HCCAST_TEST_DIS_LAYER_MAIN == cast_state->dis_layer)
        aspect.layer = DIS_LAYER_MAIN;
    else
        aspect.layer = DIS_LAYER_AUXP;

    aspect.tv_mode = cast_state->tv_mode;//16:9
    aspect.dis_mode = dis_mode;
    aspect.active_mode = active_mode;
    ioctl(fd, DIS_SET_ASPECT_MODE, &aspect);
    close(fd);
}


int hccast_test_rotate_convert(int rotate_init, int rotate_set)
{
    return ((rotate_init + rotate_set)%HCTEST_ROTATE_MAX);
}

int hccast_test_flip_convert(int dis_type, int flip_init, int flip_set)
{
    int rotate = 0;
    int swap;
    int flip_ret = flip_set;

    do {
        if (0 == flip_set){
            flip_ret = flip_init;
            break;
        }
        
        rotate = hctest_get_screen_init_rotate(dis_type);
        if (rotate == 90 || rotate == 270){
            swap = 1;
        } else {
            swap = 0;
        }

        if (swap) {
            if (1 == flip_set) {//horizon
                flip_ret = 2;
            }
            else if (2 == flip_set) {//vertical
                flip_ret = 1;
            }
        }
    } while(0);

    return flip_ret;
}


int hccast_test_set_vol(int volume)
{
    int snd_fd = -1;

    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0) {
        printf ("open snd_fd %d failed\n", snd_fd);
        return -1;
    }

    volume = volume > 100 ? 100 : volume;
    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
    close(snd_fd);
    return 0;
}

#ifdef __linux__
static struct termios stored_settings;

void cast_test_exit_console(int signo)
{
    printf("%s(), signo: %d, error: %s\n", __FUNCTION__, signo, strerror(errno));

    tcsetattr (0, TCSANOW, &stored_settings);
    exit(signo);
}

static void cast_test_signal_normal(int signo)
{
    printf("%s(), signo: %d, error: %s\n", __FUNCTION__, signo, strerror(errno));

    //reset the SIGPIPE, so that it can be catched again
    signal(signo, cast_test_signal_normal); 
}


#if 0
static void handle_sigtstp(int sig) {
    printf("Received SIGTSTP, process sent to the background.\n");

    console_tty_onoff(0);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGTSTP);
    signal(SIGTSTP, SIG_DFL);
    sigprocmask(SIG_UNBLOCK,&mask,NULL);

    signal(SIGTSTP, SIG_DFL);
    kill(getpid(),SIGTSTP);
    signal(SIGTSTP,handle_sigtstp);
}

static void handle_sigcont(int sig) {
    printf("Received SIGCONT, process continued.\n");

    struct termios new_settings;
    new_settings = stored_settings;
    //new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_lflag &= ~(ICANON | ECHO);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &new_settings);


    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGCONT);
    signal(SIGCONT, SIG_DFL);
    sigprocmask(SIG_UNBLOCK,&mask,NULL);

    signal(SIGCONT, SIG_DFL);
    kill(getpid(),SIGCONT);
    signal(SIGCONT,handle_sigcont);
}
#endif

void cast_console_init(void)
{
    struct termios new_settings;
    tcgetattr(0, &stored_settings);
    new_settings = stored_settings;
    //new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_settings.c_lflag &= ~(ICANON | ECHO);
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &new_settings);

    signal(SIGTERM, cast_test_exit_console); //kill signal
    signal(SIGINT, cast_test_exit_console); //Ctrl+C signal
    signal(SIGSEGV, cast_test_exit_console); //segmentation fault, invalid memory
    signal(SIGBUS, cast_test_exit_console);  //bus error, memory addr is not aligned.
    signal(SIGPIPE, cast_test_signal_normal);  //SIGPIPE is a disconnect message(TCP), do not need exit app.


#if 0
    if (signal(SIGTSTP, SIG_IGN) == SIG_DFL)
        signal(SIGTSTP, handle_sigtstp);
    if (signal(SIGCONT, SIG_IGN) == SIG_DFL)
        signal(SIGCONT, handle_sigcont);
#endif

}


#endif