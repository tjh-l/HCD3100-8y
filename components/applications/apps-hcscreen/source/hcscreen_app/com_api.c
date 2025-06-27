#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <hcuapi/dis.h>
#if defined(SUPPORT_FFPLAYER) || defined(__linux__)
#include <ffplayer.h>
#endif
#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <linux/watchdog.h>
#include <linux/fb.h>
#include <sys/mman.h>
#else
#include <kernel/drivers/hcusb.h>
#include <hcuapi/watchdog.h>
#include <kernel/fb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <kernel/io.h>
#include <kernel/notify.h>
#include <linux/notifier.h>
#include <hcuapi/sys-blocking-notify.h>
#include <freertos/FreeRTOS.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/fb.h>
#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#include <sys/mount.h>
#endif

#endif
#include <hcuapi/fb.h>
#include <hcuapi/standby.h>
#include <dirent.h>

#include "lvgl/lvgl.h"
#include "lv_drivers/display/fbdev.h"
#include <hccast/hccast_wifi_mgr.h>
#include "gpio_ctrl.h"
#include "com_api.h"
#include "cast_api.h"
#include "os_api.h"
#include "data_mgr.h"
#include "com_logo.h"


static uint32_t m_control_msg_id = INVALID_ID;
static cast_play_state_t m_cast_play_state = CAST_STATE_IDLE;
static bool m_ffplay_init = false;
static int m_key_rotate_en = 1;
static int m_upgrade_state = 0;
static int m_key_switch_en = false;


static uint16_t screen_init_rotate,screen_init_h_flip,screen_init_v_flip;
static void api_get_screen_rotate_info(void)
{
    unsigned int rotate=0,h_flip=0,v_flip=0;
#ifdef __HCRTOS__
	int np;
	np = fdt_node_probe_by_path("/hcrtos/rotate");
	if(np>=0)
	{
		fdt_get_property_u_32_index(np, "rotate", 0, &rotate);
		fdt_get_property_u_32_index(np, "h_flip", 0, &h_flip);
		fdt_get_property_u_32_index(np, "v_flip", 0, &v_flip);
		screen_init_rotate = rotate;
		screen_init_h_flip = h_flip;
		screen_init_v_flip = v_flip;
	}
	else
	{
		screen_init_rotate = 0;
		screen_init_h_flip = 0;
		screen_init_v_flip = 0;
	}
#else
#define ROTATE_CONFIG_PATH "/proc/device-tree/hcrtos/rotate"
	char status[16] = {0};
	api_dts_string_get(ROTATE_CONFIG_PATH "/status", status, sizeof(status));
	if(!strcmp(status, "okay")){
		rotate = api_dts_uint32_get(ROTATE_CONFIG_PATH "/rotate");
		h_flip = api_dts_uint32_get(ROTATE_CONFIG_PATH "/h_flip");
		v_flip = api_dts_uint32_get(ROTATE_CONFIG_PATH "/v_flip");
	}else{
		rotate = 0;
		h_flip = 0;
		v_flip = 0;
	}
	screen_init_rotate = rotate;
	screen_init_h_flip = h_flip;
	screen_init_v_flip = v_flip;
#endif
	printf("->>> init_rotate = %u h_flip %u v_flip = %u\n",rotate,h_flip,v_flip);
}

uint16_t api_get_screen_init_rotate(void)
{
    return screen_init_rotate;
}

uint16_t api_get_screen_init_h_flip(void)
{
    return screen_init_h_flip;
}

uint16_t api_get_screen_init_v_flip(void)
{
    return screen_init_v_flip;
}
int api_system_init()
{
    if (!api_watchdog_init()){
        api_watchdog_timeout_set(WATCHDOG_TIMEOUT);
        api_watchdog_start();
        uint32_t timeout_ms = 0;;
        api_watchdog_timeout_get(&timeout_ms);
        printf("%s(), line:%d. set watchdog timeout: %d ms\n", __func__, __LINE__, timeout_ms);
    }

    api_get_screen_rotate_info();

    return 0;
}

int api_video_init()
{
    return 0;
}

int api_audio_init()
{
    return 0;
}

int api_get_flip_info(int *rotate_type, int *flip_type)
{
   
    int rotate = 0 , h_flip = 0 , v_flip = 0;
    int init_rotate = api_get_screen_init_rotate();
    int init_h_flip = api_get_screen_init_h_flip();
    int init_v_flip = api_get_screen_init_v_flip();

    api_get_rotate_by_flip_mode(data_mgr_flip_mode_get() ,
                            &rotate ,
                            &h_flip ,
                            &v_flip);

    api_transfer_rotate_mode_for_screen(
        init_rotate,init_h_flip,init_v_flip,
        &rotate , &h_flip , &v_flip , NULL);

    *rotate_type = rotate;
    *flip_type = h_flip;
    return 0;
}

int api_lvgl_init(int width, int height)
{
    extern int hc_lvgl_init(void);
    return hc_lvgl_init();
}

#ifdef __HCRTOS__
#include <kernel/lib/fdt_api.h>
#include <kernel/lib/libfdt/libfdt.h>
#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#include <sys/mount.h>
#endif

static int get_mtdblock_devpath(char *devpath, int len, const char *partname)
{
    static int np = -1;
    static u32 part_num = 0;
    u32 i = 1;
    const char *label;
    char property[32];

    if (np < 0)
    {
        np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
    }

    if (np < 0)
        return -1;

    if (part_num == 0)
        fdt_get_property_u_32_index(np, "part-num", 0, &part_num);

    for (i = 1; i <= part_num; i++)
    {
        snprintf(property, sizeof(property), "part%d-label", i);
        if (!fdt_get_property_string_index(np, property, 0, &label) &&
            !strcmp(label, partname))
        {
            memset(devpath, 0, len);
            snprintf(devpath, len, "/dev/mtdblock%d", i);
            return i;
        }
    }

    return -1;
}

int api_romfs_resources_mount()
{
    static char m_mount = 0;
    char devpath[64];
    int ret = 0;

    if (m_mount)
    {
        printf("%s: resources alread mount!\n", __func__);
        return -1;
    }

    ret = get_mtdblock_devpath(devpath, sizeof(devpath), "eromfs");
    if (ret >= 0)
        ret = mount(devpath, "/hccast_files", "romfs", MS_RDONLY, NULL);

    if (ret >= 0)
    {
        printf("mount ok!\n");
        m_mount = 1;
    }

    return 0;
}
#endif



static int m_logo_player = 0;
static char m_logo_file[128] = {0};

static int api_logo_off_no_black()
{
#if defined(SUPPORT_FFPLAYER) || defined(__linux__)
    if (m_logo_player){
        com_logo_off(0, 0);
    }
#endif   
    m_logo_player = 0;
    return 0;

}

int api_logo_reshow(void)
{
    if (m_logo_player && strlen(m_logo_file))
    {
        api_logo_off_no_black();
        api_logo_show(m_logo_file);
    }

    return 0;
}
int api_logo_show(const char *file)
{
#if !defined(BR2_PACKAGE_PREBUILTS_FFPLAYER) && !defined(__linux)
	return 0;
#else

    char *file_path = file;
    int rotate_type = ROTATE_TYPE_0;
    int flip_type = MIRROR_TYPE_NONE;
    
    api_logo_off();

    if (!file)
        file_path = BACK_LOGO;

    strcpy(m_logo_file, file_path);

    if (com_logo_show(file_path, rotate_type, flip_type) < 0)
    {
       printf("com_logo_show() fail!\n");
       return -1;
    }
    
    m_logo_player = 1;

    printf("=============================Show logo: %s ok!=============================\n", file_path);

    return 0;
#endif
}

int api_logo_off()
{
#if !defined(BR2_PACKAGE_PREBUILTS_FFPLAYER) && !defined(__linux)
	return 0;
#else

    if (m_logo_player)
    {
        com_logo_off(1,1);
        m_logo_player = 0;
        printf("%s =============================Close logo: ok!=============================\n", __func__);
    }
    return 0;
	#endif
}

int api_logo_off2(int closevp, int fillblack)
{
#if !defined(BR2_PACKAGE_PREBUILTS_FFPLAYER) && !defined(__linux)
	return 0;
#else

    if (m_logo_player)
    {
        com_logo_off(closevp,fillblack);
        m_logo_player = 0;
        printf("%s =============================Close logo: ok!=============================\n", __func__);
    }
    return 0;
	#endif
}

int api_logo_dis_backup_free(void)
{
    com_logo_dis_backup_free();
    
    return 0;
}

int api_logo_dis_backup(void)
{
    com_logo_dis_backup();
    
    return 0;
}

int api_logo_state_get(void)
{
    return m_logo_player;
}

/**
 * @brief turn on/off the video frame output
 *
 * @param on_off
 * @return int
 */
int api_dis_show_onoff(bool on_off)
{
    int fd = -1;
    struct dis_win_onoff winon = { 0 };

    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0)
    {
        return -1;
    }

    winon.distype = DIS_TYPE_HD;
    winon.layer =  DIS_LAYER_MAIN;
    winon.on = on_off ? 1 : 0;

    ioctl(fd, DIS_SET_WIN_ONOFF, &winon);
    close(fd);

    return 0;
}

int api_control_send_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == m_control_msg_id)
    {
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id)
        {
            return -1;
        }
    }
    return api_message_send(m_control_msg_id, control_msg, sizeof(control_msg_t));
}

int api_control_receive_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == m_control_msg_id)
    {
        return -1;
    }
    return api_message_receive_tm(m_control_msg_id, control_msg, sizeof(control_msg_t), 5);
}

int api_control_send_key(uint32_t key)
{
    control_msg_t control_msg;
    control_msg.msg_type = MSG_TYPE_KEY;
    control_msg.msg_code = key;

    if (INVALID_ID == m_control_msg_id)
    {
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id)
        {
            return -1;
        }
    }
    return api_message_send(m_control_msg_id, &control_msg, sizeof(control_msg_t));
}

cast_play_state_t api_cast_play_state_get(void)
{
    return m_cast_play_state;
}
void api_cast_play_state_set(cast_play_state_t state)
{
    m_cast_play_state = state;
}

/** check the string if it is IP address
 * @param
 * @return
 */
bool api_is_ip_addr(char *ip_buff)
{
    int ip1, ip2, ip3, ip4;
    char temp[64];
    if((sscanf(ip_buff,"%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4))!=4)
        return false;
    sprintf(temp,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);
    if(strcmp(temp, ip_buff) != 0)
        return false;
    if(!((ip1 <= 255 && ip1 >= 0)&&(ip2 <= 255 && ip2 >= 0)&&(ip3 <= 255 && ip1 >= 0)))
        return false;
    else
        return true;
}


#define ETHE_MAC_PATH "/sys/class/net/eth0/address"
#define WIFI_MAC_PATH "/sys/class/net/wlan0/address"

#define MAC_ADDRESS_PATH   WIFI_MAC_PATH// ETHE_MAC_PATH
/**get the mac address, after modprobe cmmand, the ip address of wlan0
 * should be saved to "/sys/class/net/wlan0/address"
 * @param
 * @return
 */
int api_get_mac_addr(char *mac)
{
#ifdef __linux__
    int ret;
    char buffer[32] = {0};
    if (mac == NULL)
        return -1;

    FILE *fp = fopen(MAC_ADDRESS_PATH, "r");
    if (fp == NULL)
        return -1;

    fread(buffer, 1, 32, fp);
    fclose(fp);
    buffer[strlen(buffer)-1] = '\0';
    printf(MAC_ADDRESS_PATH "=%s\n", buffer);
    ret = sscanf(buffer, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
                 &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    if(ret == 6)
        return 0;
    else
        return -1;
#else
	struct ifreq ifr;
    int skfd;

    if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        printf("socket error\n");
        return -1;
    }

    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ);
    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0)
    {
        printf( "%s net_get_hwaddr: ioctl SIOCGIFHWADDR\n",__func__);
        close(skfd);
        return -1;
    }
    close(skfd);
    memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, 6);
	return 0;
#endif
}

int api_get_wifi_freq_mode(void)
{
    return (int)hccast_wifi_mgr_freq_support_mode();
}

void app_ffplay_init(void)
{
#if !defined(BR2_PACKAGE_PREBUILTS_FFPLAYER) && !defined(__linux)
	return;
#else

    if (m_ffplay_init) return;

    hcplayer_init(1);
    m_ffplay_init = true;
#endif
}

void app_ffplay_deinit(void)
{
#if !defined(BR2_PACKAGE_PREBUILTS_FFPLAYER) && !defined(__linux)
	return;
#else
    hcplayer_deinit();
    m_ffplay_init = false;
#endif
}

/**
 * @brief linux system will send the exit signal, then release system resource here
 *
 */
void app_exit(void)
{
    if (INVALID_ID != m_control_msg_id)
        api_message_delete(m_control_msg_id);

    app_ffplay_deinit();
}

void api_sleep_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

#ifdef __linux__
int api_shell_exe_result(char *cmd)
{
    int ret = API_FAILURE;
    char result_buf[32];
    int result_val = -1;
    FILE *fp = NULL;

    //step1: excute the shell command
    system(cmd);

    //step2: get the excuting result
    fp = popen("echo $?", "r");
    if (fgets(result_buf, sizeof(result_buf), fp) != NULL)
    {
        if('\n' == result_buf[strlen(result_buf)-1])
        {
            result_buf[strlen(result_buf)-1] = '\0';
        }

        sscanf(result_buf,"%d", &result_val);
    }
    pclose(fp);
    if (0 == result_val)
    {
        printf("Excute cmd: [%s] OK!\n", cmd);
        ret = API_SUCCESS;
    }
    else
    {
        printf("Excute cmd: [%s] fail, ret = %d!\n", cmd, result_val);
        ret = API_FAILURE;
    }

    return ret;
}
#endif

static unsigned int crc_table[256];
static volatile bool m_crc_init = false;
static void api_crc_table_init(void)
{
    unsigned int c;
    unsigned int i, j;

    if (m_crc_init)
        return;

    for (i = 0; i < 256; i++)
    {
        c = (unsigned int)i;
        for (j = 0; j < 8; j++)
        {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[i] = c;
    }

    m_crc_init = true;
}

unsigned int api_crc32(unsigned int crc,unsigned char *buffer, unsigned int size)
{
    unsigned int i;

    api_crc_table_init();

    for (i = 0; i < size; i++)
    {
        crc = crc_table[(crc ^ buffer[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ;
}

int api_osd_show_onoff(bool on_off)
{
    // Open the file for reading and writing
    int fbfd = open("/dev/fb0", O_RDWR);
    uint32_t blank_mode;

    if(fbfd == -1)
    {
        printf("%s(), line: %d. Error: cannot open framebuffer device", __func__, __LINE__);
        return API_FAILURE;
    }

    if (on_off)
        blank_mode = FB_BLANK_UNBLANK;
    else
        blank_mode = FB_BLANK_NORMAL;

    if (ioctl(fbfd, FBIOBLANK, blank_mode) != 0)
    {
        printf("%s(), line: %d. Error: FBIOBLANK", __func__, __LINE__);
    }

    close(fbfd);
    return API_SUCCESS;
}

static int api_wifi_disconnect(void)
{
#ifdef __HCRTOS__
#ifdef WIFI_SUPPORT
    printf("%s()\n", __func__);
    extern void platform_wifi_reset(void);
    platform_wifi_reset();
    hcusb_set_mode(0, MUSB_PERIPHERAL);
    hcusb_set_mode(1, MUSB_PERIPHERAL);
    usleep(100*1000);
#endif
#endif
    return 0;
}

void api_system_reboot(void)
{
    printf("%s(): reboot now!!\n", __func__);
    api_wifi_disconnect();
    api_osd_show_onoff(false);
    api_logo_off();
    api_dis_show_onoff(false);
#ifdef __linux__
    system("reboot -f");
#else
    extern int reset(void);
    reset();
#endif

    while(1);
}

/*
we can wake up by 3 ways: ir, gpio and sacadc key.
 */
void api_system_standby(void)
{
    int fd = -1;

    fd = open("/dev/standby", O_RDWR);
    if (fd < 0)
    {
        printf("%s(), line:%d. open standby device error!\n", __func__, __LINE__);
        return;
    }

    printf("%s(), line:%d.\n", __func__, __LINE__);

    //step 1: off the display
    api_osd_show_onoff(false);
    api_logo_off();
    api_dis_show_onoff(false);
    //sleep for a while so that hardware display is really off.
    api_sleep_ms(100);

    //Step 2: off other devices

    //Step 3: config the standby wake up methods

    api_watchdog_stop();


    //config wake up ir scancode(so far, default is power key:28)
    //check hclinux\SOURCE\linux-drivers\drivers\hcdrivers\rc\keymaps\rc-hcdemo.c
    //for scan code.
    struct standby_ir_setting ir = { 0 };
    ir.num_of_scancode = 1;
    ir.scancode[0] = 28;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_IR, (unsigned long)&ir);

    //config wake up GPIO
    struct standby_gpio_setting gpio = { 0 };
    gpio.pin = PINPAD_L08;
    gpio.polarity = 0;//low is active;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_GPIO, (unsigned long)&gpio);

#if 0
    //config wake up adc key
    struct standby_saradc_setting adc = { 0 };
    adc.channel = 1;
    adc.min = 1000;
    adc.max = 1500;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_SARADC, (unsigned long)&adc);
#endif

#if 0
    //lower the volatage of ddr via the GPIO
    struct standby_pwroff_ddr_setting ddr = { 0 };
    ddr.pin = PINPAD_L09;
    ddr.polarity = 0;//low is active;
    ioctl(fd, STANDBY_SET_PWROFF_DDR, (unsigned long)&ddr);
#endif

    //Step 4: entering system standby
    ioctl(fd, STANDBY_ENTER, 0);
    close(fd);
    while(1);
}

static volatile int m_osd_off_time_cnt = 0;
#define ONE_COUNT_TIME  200
static void *osd_off_for_time(void *arg)
{
    uint32_t timeout = (uint32_t)arg;
    m_osd_off_time_cnt = timeout/ONE_COUNT_TIME;
    do
    {
        api_sleep_ms(ONE_COUNT_TIME);
    }
    while(m_osd_off_time_cnt --);

    m_osd_off_time_cnt = 0;

    api_osd_show_onoff(true);

    return NULL;
}

//Turn off OSD for a time, then turn on OSD.
//because sometimes enter dlna music play, the OSD is still show
//but the video screen is black(BUG #2848), so we turn off OSD for some time.
void api_osd_off_time(uint32_t timeout)
{

    //update the wait time
    if (m_osd_off_time_cnt)
    {
        int timeout_cnt = timeout/ONE_COUNT_TIME;
        if (timeout_cnt > m_osd_off_time_cnt)
            m_osd_off_time_cnt = timeout_cnt;

        return;
    }

    api_osd_show_onoff(false);
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x1000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&thread_id, &attr, osd_off_for_time, (void*)timeout))
    {
        pthread_attr_destroy(&attr);
        return;
    }

    pthread_attr_destroy(&attr);
}

void api_set_aspect_mode(dis_tv_mode_e ratio, 
                            dis_mode_e dis_mode, 
                            dis_scale_avtive_mode_e active_mode)
{
    int fd = open("/dev/dis", O_RDWR);
    if ( fd < 0)
        return;
    dis_aspect_mode_t aspect = {0};
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    aspect.dis_mode = dis_mode;
    aspect.active_mode = dis_mode;
    ioctl(fd, DIS_SET_ASPECT_MODE, &aspect);
    close(fd);
}


typedef void *(player_get_func)(void);
static player_get_func *ffmpeg_player_get = NULL;
void *api_ffmpeg_player_get(void)
{
    if (ffmpeg_player_get)
        return ffmpeg_player_get();
    else
        return NULL;
}


void api_ffmpeg_player_get_regist(void *(func)(void))
{
    ffmpeg_player_get = func;
}

void api_transfer_rotate_mode_for_screen(
                                        int init_rotate,
                                        int init_h_flip,
                                        int init_v_flip,
                                        int *p_rotate_mode ,
                                        int *p_h_flip ,
                                        int *p_v_flip,
                                        int *p_fbdev_rotate)
{
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

#ifdef __linux__
int api_dts_uint32_get(const char *path)
{
    int fd = open(path, O_RDONLY);
    int value = 0;;
    if(fd >= 0){
        uint8_t buf[4];
        if(read(fd, buf, 4) != 4){
            close(fd);
            return value;
        }
        close(fd);
        value = (buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | (buf[2] & 0xff) << 8 | (buf[3] & 0xff);
    }
    printf("dts value: %d\n", value);
    return value;
}


void api_dts_string_get(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);
    if(fd >= 0){
        read(fd, string, size);
        close(fd);
    }
    printf("dts string: %s\n", string);
}
#endif


static const char *m_dev_dog = "/dev/watchdog";
static int m_dog_fd = -1;

int api_watchdog_init(void)
{
#ifndef WATCHDOG_SUPPORT
    return -1;
#endif

    if (m_dog_fd >= 0)
        return 0;

    m_dog_fd = open(m_dev_dog, O_RDWR);
    if (m_dog_fd < 0) {
        printf("%s(), line:%d. No watchdog!!!\n", __func__, __LINE__);
        return -1;
    }

    return 0;
}


//Set the timeout of watchdog , system would reboot if watchdog not feed
//within the timeout.
int api_watchdog_timeout_set(uint32_t timeout_ms)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__    
    ioctl(m_dog_fd, WDIOC_SETMODE, WDT_MODE_WATCHDOG);
    return ioctl(m_dog_fd, WDIOC_SETTIMEOUT, timeout_ms*1000);
#else
    uint32_t timeout_second = timeout_ms/1000;
    return ioctl(m_dog_fd, WDIOC_SETTIMEOUT, &timeout_second);
#endif    
}

int api_watchdog_timeout_get(uint32_t *timeout_ms)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__    
    uint32_t timeout_us = 0;
    if (ioctl(m_dog_fd, WDIOC_GETTIMEOUT, &timeout_us))
        return -1;
    *timeout_ms = timeout_us/1000;
#else
    uint32_t timeout_second;
    if (ioctl(m_dog_fd, WDIOC_GETTIMEOUT, &timeout_second))
        return -1;

    *timeout_ms = timeout_second*1000;

#endif    

    return 0;
}

int api_watchdog_start(void)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__        
    return ioctl(m_dog_fd, WDIOC_START, 0);
#else
    uint32_t val = WDIOS_ENABLECARD;
    return ioctl(m_dog_fd, WDIOC_SETOPTIONS, &val);
#endif
}

int api_watchdog_stop(void)
{
    if (m_dog_fd < 0)
        return -1;

#ifdef __HCRTOS__        

    ioctl(m_dog_fd, WDIOC_STOP, 0);
#else    

    uint32_t val;

    ioctl(m_dog_fd, WDIOC_GETSTATUS, &val);
    printf("%s(), line: %d: watchdog status = %s \n", __func__, __LINE__,
        val ? "running" : "stop");

    uint32_t timeout_second;
    ioctl(m_dog_fd, WDIOC_GETTIMEOUT, &timeout_second);
    printf("%s(), line: %d: watchdog timeout = %d \n", __func__, __LINE__, timeout_second);


    write(m_dog_fd, "V", 1);
    val = WDIOS_DISABLECARD;
    ioctl(m_dog_fd, WDIOC_SETOPTIONS, &val);

    ioctl(m_dog_fd, WDIOC_GETSTATUS, &val);
    printf("%s(), line: %d: watchdog status = %s \n", __func__, __LINE__,
           val ? "running" : "stop");


#endif
    return 0;
}


int api_watchdog_feed(void)
{
    if (m_dog_fd < 0)
        return -1;

    return ioctl(m_dog_fd, WDIOC_KEEPALIVE, 0);    
}

int api_set_key_rotate_enable(int enable)
{
    m_key_rotate_en = enable;
    return 0;
}

int api_get_key_rotate_enable(void)
{
    return m_key_rotate_en;
}

unsigned long api_get_time_ms(void)
{
    struct timeval time_t;
    gettimeofday(&time_t, NULL);
    return (time_t.tv_sec * 1000 + time_t.tv_usec / 1000);
}

int api_get_upgrade_state(void)
{
    return m_upgrade_state;
}

void api_set_upgrade_state(int flag)
{
    m_upgrade_state = flag;
}

int api_get_key_switch_enable(void)
{
    return m_key_switch_en;
}

void api_set_key_switch_enable(int enable)
{
    m_key_switch_en = enable;
}

#ifdef APPMANAGER_SUPPORT
#define APP_IPC_MSG

#define APP_NAME "hcscreen_demo"

//use message queue for PIC communication
#ifdef APP_IPC_MSG

#ifdef CONFIG_APPS_MGR_MSG_KEY
  #define APP_QUEUE_KEY   CONFIG_APPS_MGR_MSG_KEY
#else
  #define APP_QUEUE_KEY   12345678
#endif

//use message for PIC communication

int api_app_command_send(const char *cmd)
{
    static volatile int app_msg_id = 0;

    if (app_msg_id <= 0){
        app_msg_id = api_message_create_by_key(APP_QUEUE_KEY);   
        if (app_msg_id < 0){
            return -1;
        }
    }
    
    return api_message_send((uint32_t)app_msg_id, cmd, strlen(cmd));
}

#else
//use named pipe for PIC communication

#define APP_FIFO_NAME   "/tmp/appmgr.fifo"
//send command to application manager via named pipe
int api_app_command_send(const char *cmd)
{
    static int fifo_open = 0;

    if (access(APP_FIFO_NAME, F_OK))
    {
        //no APP_FIFO_NAME
        return -1;
    }

    if (!fifo_open)
    {
        umask(0);
        int ret = mkfifo(APP_FIFO_NAME, 0664);
        if (ret < 0 && errno != EEXIST)
        {
            perror("mkfifo error");
            return -1;
        }
        fifo_open = 1;
    }

    int pipe_fd = open(APP_FIFO_NAME, O_WRONLY);
    if (pipe_fd < 0)
    {
        perror("open fifo error");
        return -1;
    }
    printf("open fifo success\n");

    printf("%s().cmd:%s\n", __func__, cmd);
    write(pipe_fd, cmd, strlen(cmd));

    close(pipe_fd);
    return 0;

}
#endif

/*
* send upgrade command to appmanager
* cmd: upgrade -k hcscreen_demo -s upgradeapp -u /tmp/HCFOTA.bin -z 0,0,1280,720,1920,1080 -f 180,0,0
*     -k: kill application
*     -u: upgrade file
*     -z: fb zoom
*     -f: rotate, hor_flip, ver_flip
*/
//cmd: upgrade -k hcscreen_demo -s upgradeapp -u /tmp/HCFOTA.bin -z 0,0,1280,720,1920,1080 -f 180,0,0
int api_app_command_upgrade(const char *url)
{
    int rotate = 0;
    int hor_flip = 0;
    int ver_flip = 0;

    char upgrade_cmd[256];

    if (access(url, F_OK))
        return UPG_STATUS_FILE_CRC_ERROR;

    if (!api_get_flip_info(&rotate, &hor_flip)){
        //set fb zoom & file args for upgrade application.
        //cmd: upgrade -k hcscreen_demo -s ./upgradeapp -u /tmp/HCFOTA.bin -z 0,0,1280,720,1920,1080 -f 180,0,0
        snprintf(upgrade_cmd, sizeof(upgrade_cmd), 
            "upgrade -k %s -s %s -u %s -f %d,%d,%d",
            HC_APP_NAME, HC_APP_UPGRADE, url, rotate, hor_flip, ver_flip);

    } else {
        //use full screen for for upgrade application.
        //cmd: upgrade -k hcscreen_demo -s ./upgradeapp
        snprintf(upgrade_cmd, sizeof(upgrade_cmd), 
            "upgrade -k %s -s %s -u %s", HC_APP_NAME, HC_APP_UPGRADE, url);
    
    }
    return api_app_command_send(upgrade_cmd);
}


typedef struct{
    int fd;
    void *buff;
    uint32_t size;
}upgrade_mmap_t;

static upgrade_mmap_t m_upgrade_mmap = 
{
    .fd = -1,
};

void *api_upgrade_buffer_alloc(unsigned long size)
{
    int fd = -1;

    fd = open(UPGRADE_APP_TEMP_OTA_BIN, O_RDWR | O_CREAT | O_TRUNC, 0644);    
    if (fd < 0) {
        printf("%s(), line:%d. open %s fail!\n", __func__, __LINE__, UPGRADE_APP_TEMP_OTA_BIN);
        return NULL;
    }

    if (ftruncate(fd, size) < 0) { // set the create file size
        printf("%s(), line:%d. ftruncate %s fail!\n", __func__, __LINE__, UPGRADE_APP_TEMP_OTA_BIN);
        close(fd);
        return NULL;
    }

    struct stat statbuf;
    if (fstat(fd, &statbuf) == -1) {
        printf("%s(), line:%d. fstat %s fail!\n", __func__, __LINE__, UPGRADE_APP_TEMP_OTA_BIN);
        close(fd);
        return NULL;
    }

    void *mapped_area = mmap(NULL, statbuf.st_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped_area == MAP_FAILED) {
        printf("%s(), line:%d. mmap fail!\n", __func__, __LINE__);
        close(fd);
        return NULL;
    }

    m_upgrade_mmap.fd = fd;
    m_upgrade_mmap.buff = mapped_area;
    m_upgrade_mmap.size = statbuf.st_size;

    return mapped_area;
}

void api_upgrade_buffer_sync(void)
{
    if (m_upgrade_mmap.fd < 0 || !m_upgrade_mmap.buff)
        return;

    if (m_upgrade_mmap.buff && msync(m_upgrade_mmap.buff, m_upgrade_mmap.size, MS_SYNC) == -1) {
        printf("%s(), line:%d. msync fail!\n", __func__, __LINE__);
        return;
    }

    if (munmap(m_upgrade_mmap.buff, m_upgrade_mmap.size) == -1) {
        printf("%s(), line:%d. munmap fail!\n", __func__, __LINE__);
        return;
    }

    close(m_upgrade_mmap.fd);
    m_upgrade_mmap.fd = -1;
    m_upgrade_mmap.buff = NULL;
}

void api_upgrade_buffer_free(void *ptr)
{
    (void)ptr;

    api_upgrade_buffer_sync();
    remove(UPGRADE_APP_TEMP_OTA_BIN);
}

#else

void *api_upgrade_buffer_alloc(unsigned long size)
{
    return malloc(size);
}

void api_upgrade_buffer_sync(void)
{
}

void api_upgrade_buffer_free(void *ptr)
{
    free(ptr);
}

#endif //endif of APPMANAGER_SUPPORT