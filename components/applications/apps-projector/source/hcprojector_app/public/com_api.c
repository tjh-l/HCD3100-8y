#include "app_config.h"
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <hcuapi/dis.h>
#include <errno.h>

#ifdef __HCRTOS__
#include <hcuapi/watchdog.h>

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

#else
#include <sys/epoll.h>
#include <sys/socket.h>
#include <linux/fb.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <linux/watchdog.h>
#include <linux/netlink.h>
#endif


#include <ffplayer.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <hcuapi/snd.h>
#include <hcuapi/vidsink.h>
#include <hcuapi/common.h>
#include <hcuapi/kumsgq.h>
#include <hcuapi/hdmi_tx.h>

#include "network_api.h"
#include <hcuapi/fb.h>
#include <hcuapi/standby.h>
#include <lvgl/hc-porting/hc_lvgl_init.h>
#include <dirent.h>

#ifdef WIFI_SUPPORT
#include <net/if.h>
#include <hccast/hccast_wifi_mgr.h>
#include <hccast/hccast_net.h>
#ifdef AIRP2P_SUPPORT
#include <libusb.h>
#endif
#endif

#include <hcuapi/input.h>
#include <hcuapi/input-event-codes.h>

#include <hcuapi/snd.h>
#include "lvgl/lvgl.h"
#include "lvgl/src/misc/lv_gc.h"
#include "lvgl/src/misc/lv_ll.h"
#include "lv_drivers/display/fbdev.h"
#include "gpio_ctrl.h"

#include "app_config.h"
#include "factory_setting.h"
#include "setup.h"

#include "screen.h"
#include <hudi/hudi_screen.h>

#include "tv_sys.h"
#include "com_api.h"
#include "os_api.h"
#include "glist.h"
#include "hcuapi/lvds.h"
#include "hcuapi/backlight.h"
#ifdef __HCRTOS__
#include <hcuapi/gpio.h>
#include <nuttx/wqueue.h>
#include <kernel/drivers/hcusb.h>
#endif
#include "com_logo.h"

#ifdef UIBC_SUPPORT
#include "usb_hid.h"
#define NETLINK_BUFFER_SIZE 2048
#endif
#ifdef HDMI_RX_CEC_SUPPORT
#include "channel/hdmi_in/hdmi_rx.h"
#endif

static uint32_t m_control_msg_id = INVALID_ID;
static cast_play_state_t m_cast_play_state = CAST_STATE_IDLE;
static bool m_ffplay_init = false;
static int m_usb_state = USB_STAT_INVALID;
static partition_info_t* partition_info=NULL;

static volatile int m_osd_off_time_cnt = 0;
static volatile char m_osd_off_time_flag = 0;
static pthread_mutex_t m_osd_off_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t m_set_filp_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool m_set_flip_en = true;
static int m_wifi_pm_state = WIFI_PM_STATE_NONE;
static pthread_mutex_t m_wifi_pm_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile  bool m_is_upgrade = false;
static pthread_mutex_t m_flash_rw_mutex = PTHREAD_MUTEX_INITIALIZER;

static fb_zoom_t m_fb_zoom;
static int m_fb_rotate;
static int m_fb_hor_flip;
static int m_fb_ver_flip;
static int m_fb_zoom_set = 0;
static int m_filp_set = 0;

int mmp_get_usb_stat(void)
{
    return m_usb_state;
}

static void mmp_set_usb_stat(int state)
{
    m_usb_state = state;
}

//return usb device: /meida/hdd(linux), /meida/sda (rtos)
static char mounted_dev_name[64];
char *mmp_get_usb_dev_name(void)
{
    char *dev_name = glist_nth_data(partition_info->dev_list, 0);
    if (!dev_name)
        return NULL;
    
    memcpy(mounted_dev_name, dev_name, strlen(dev_name));
    return mounted_dev_name;
}

static glist* partition_glist_find(char * string_to_find)
{
    glist* glist_found = NULL;
    glist* temp_glist = (glist*)partition_info->dev_list;
    while(temp_glist){
        if(temp_glist!=NULL&&temp_glist->data!=NULL){
            if(strcmp(string_to_find,temp_glist->data)==0){
                glist_found = temp_glist;
                break;
            }
            else{
                temp_glist=temp_glist->next;
            }
                
        }
    }
    
    return glist_found;
}

static int partition_delnode_form_glist(char * string_to_find)
{
    int ret = 0;
    int freed_flag = 0;
    glist* glist_del=NULL;

    glist_del = partition_glist_find(string_to_find);
    if(glist_del){
        if(glist_del->data){
            if ((void*)string_to_find == (void*)(glist_del->data))
                freed_flag = 1;

            free(glist_del->data);
            glist_del->data=NULL;//free data 
        }
        partition_info->dev_list=glist_delete_link(partition_info->dev_list,glist_del);
        glist_del=NULL;
        ret = 0;
    } else {
        ret = -1;
    }

    if (0 == freed_flag)
        free(string_to_find);

    return ret;
}

int partition_info_update(int usb_state,void* dev)
{
    if(usb_state==MSG_TYPE_USB_MOUNT||usb_state==MSG_TYPE_SD_MOUNT){
        char *dev_name = NULL;
        if (partition_glist_find((char*)dev))
            return 0;

        partition_info->count++;
        dev_name = (char*)dev;
        partition_info->dev_list=glist_append((glist*)partition_info->dev_list, dev_name);
    }else if(usb_state==MSG_TYPE_USB_UNMOUNT||usb_state==MSG_TYPE_SD_UNMOUNT||usb_state==MSG_TYPE_SD_UNMOUNT_FAIL||usb_state==MSG_TYPE_USB_UNMOUNT_FAIL){
        if (!partition_delnode_form_glist(dev)){
            partition_info->count--;
            if (partition_info->count < 0)
                partition_info->count = 0;
        }

    }
    partition_info->m_storage_state=usb_state;
    return 0;
}

static int partition_info_init()
{
    if(partition_info==NULL){
        partition_info=(partition_info_t *)malloc(sizeof(partition_info_t));
        memset(partition_info,0,sizeof(partition_info_t));
    }
    return 0;
}

void* mmp_get_partition_info(void)
{
    return partition_info;
}
static int partition_info_msg_send(int type,uint32_t code)
{
    control_msg_t msg = {0};
    memset(&msg, 0, sizeof(control_msg_t));
    switch(type){
        case USB_STAT_MOUNT:
            msg.msg_type=MSG_TYPE_USB_MOUNT;
            break;
        case USB_STAT_UNMOUNT:
            msg.msg_type=MSG_TYPE_USB_UNMOUNT;
            break;
        case USB_STAT_MOUNT_FAIL:
            msg.msg_type=MSG_TYPE_USB_MOUNT_FAIL;
            break;
        case USB_STAT_UNMOUNT_FAIL:
            msg.msg_type=MSG_TYPE_USB_UNMOUNT_FAIL;
            break;
        case SD_STAT_MOUNT:
            msg.msg_type=MSG_TYPE_SD_MOUNT;
            break;
        case SD_STAT_UNMOUNT:
            msg.msg_type=MSG_TYPE_SD_UNMOUNT;
            break;
        case SD_STAT_MOUNT_FAIL:
            msg.msg_type=MSG_TYPE_SD_MOUNT_FAIL;
            break;
        case SD_STAT_UNMOUNT_FAIL:
            msg.msg_type=MSG_TYPE_SD_UNMOUNT_FAIL;
            break;
    }
    msg.msg_code=code;
    api_control_send_msg(&msg);

    return 0;
}


static void _hotplug_hdmi_tx_tv_sys_set(void)
{
    int ap_tv_sys;
    int ap_tv_sys_ret;
    int last_tv_sys;

    last_tv_sys = projector_get_some_sys_param(P_DE_TV_SYS);
    ap_tv_sys = projector_get_some_sys_param(P_SYS_RESOLUTION);
    ap_tv_sys_ret = tv_sys_app_auto_set(ap_tv_sys, 2000);
    if (ap_tv_sys_ret >= 0)
    {
        if (APP_TV_SYS_AUTO == ap_tv_sys)
            sysdata_app_tv_sys_set(APP_TV_SYS_AUTO);
        else
            sysdata_app_tv_sys_set(ap_tv_sys_ret);
        projector_sys_param_save();
    }

#ifdef CAST_SUPPORT
    if (((last_tv_sys < TV_LINE_4096X2160_30) && (projector_get_some_sys_param(P_DE_TV_SYS) >= TV_LINE_4096X2160_30)) \
            || ((last_tv_sys >= TV_LINE_4096X2160_30) && (projector_get_some_sys_param(P_DE_TV_SYS) < TV_LINE_4096X2160_30)))
    {
        control_msg_t ctl_msg = {0};
        ctl_msg.msg_type = MSG_TYPE_HDMI_TX_CHANGED;
        api_control_send_msg(&ctl_msg);

    }
#endif

}

#ifdef __HCRTOS__

#ifdef WIFI_SUPPORT

typedef struct _wifi_model_st_
{
    char name[16];
    char desc[16];
    int  type;
} wifi_model_st;

wifi_model_st wifi_model_list[] =
{
    {"", "", HCCAST_NET_WIFI_NONE},
#ifdef WIFI_RTL8188FU_SUPPORT
    {"rtl8188fu", "v0BDApF179", HCCAST_NET_WIFI_8188FTV},
#endif
#ifdef WIFI_RTL8188ETV_SUPPORT
    {"rtl8188eu", "v0BDAp0179", HCCAST_NET_WIFI_8188FTV},
    {"rtl8188eu", "v0BDAp8179", HCCAST_NET_WIFI_8188FTV},
#endif
#ifdef WIFI_RTL8733BU_SUPPORT
    {"rtl8733bu", "v0BDApB733", HCCAST_NET_WIFI_8733BU},
    {"rtl8733bu", "v0BDApF72B", HCCAST_NET_WIFI_8733BU},
#endif   
    //{"rtl8811cu", "v0BDApC811", HCCAST_NET_WIFI_8811FTV},
    {"rtl8723as", "v024Cp8723", HCCAST_NET_WIFI_8188FTV},
#ifdef WIFI_RTL8723BS_SUPPORT
    {"rtl8723bs", "v024CpB723", HCCAST_NET_WIFI_8188FTV},
#endif

#ifdef WIFI_ECR6600U_SUPPORT
    {"ecr6600u", "v3452p6600", HCCAST_NET_WIFI_ECR6600U},
#endif

#ifdef WIFI_RTL8733BS_SUPPORT
    {"rtl8733bs", "v024CpB733", HCCAST_NET_WIFI_8188FTV},
#endif
};

wifi_model_st g_hotplug_wifi_model = {0};

#include <linux/usb.h>

static int usb_wifi_notify_check(unsigned long action, void *dev)
{
    unsigned char found = 0;
    control_msg_t ctl_msg = {0};
    char devpath[64] = { 0 };
    struct removable_notify_info *notify_info =
                (struct removable_notify_info *)dev;

    for (int index = 0; \
            index < sizeof(wifi_model_list)/sizeof(wifi_model_list[0]); \
            index++)
    {
        if(strncmp(wifi_model_list[index].desc,
                    notify_info->devname,
                    strlen(notify_info->devname)))
        {
            if(index == sizeof(wifi_model_list)/sizeof(wifi_model_list[0]) - 1)
                return -1;
            continue;
        }
        else
            break;
    }

    switch (action)
    {
        case USB_DEV_NOTIFY_CONNECT:
        case SDIO_DEV_NOTIFY_CONNECT:
        {
            char path[64] = {0};
            FILE* fp;

            printf("Wi-Fi Plug In -> ");

            for (int i = 0; i < sizeof(wifi_model_list)/sizeof(wifi_model_list[0]); i++)
            {
                if (action == USB_DEV_NOTIFY_CONNECT)
                    snprintf(path, sizeof(path) - 1, "/dev/bus/usb/%s", wifi_model_list[i].desc);
                else if (action == SDIO_DEV_NOTIFY_CONNECT)
                    snprintf(path, sizeof(path) - 1, "/dev/bus/sdio/%s", wifi_model_list[i].desc);
                fp = fopen(path, "rb");
                if (fp)
                {
                    fclose(fp);
                    found = 1;
                    printf("Model: %s\n", wifi_model_list[i].name);

                    memcpy(&g_hotplug_wifi_model, &wifi_model_list[i], sizeof(g_hotplug_wifi_model));
                    
                    network_wifi_module_set(g_hotplug_wifi_model.type);
                    ctl_msg.msg_type = MSG_TYPE_USB_WIFI_PLUGIN;
                    api_control_send_msg(&ctl_msg);
                    break;
                }
            }

            if (!found)
            {
                printf ("Unsupport Model\n");
            }
            // snd msg

            break;
        }
        case USB_DEV_NOTIFY_DISCONNECT:
        case SDIO_DEV_NOTIFY_DISCONNECT:
        {
            if (HCCAST_NET_WIFI_NONE != g_hotplug_wifi_model.type){
                printf("Wi-Fi Plug Out -> model: %s\n", g_hotplug_wifi_model.name);
                memset (&g_hotplug_wifi_model, 0, sizeof(g_hotplug_wifi_model));
                network_wifi_module_set(0);
                ctl_msg.msg_type = MSG_TYPE_USB_WIFI_PLUGOUT;
                api_control_send_msg(&ctl_msg);
            }
            break;
        }     
        default:
            break;
    }

    return NOTIFY_OK;
}
#endif


static int usbd_notify(struct notifier_block *self,
                        unsigned long action, void* dev)
{
    if (!dev)
        return NOTIFY_OK;

    switch (action) {
    case USB_MSC_NOTIFY_MOUNT:
        if(strstr(dev,"sd")){
            m_usb_state = USB_STAT_MOUNT;
        }else if(strstr(dev,"mmc")){
            m_usb_state = SD_STAT_MOUNT;
        }
        if (dev)
            printf("USB_STAT_MOUNT: %s\n", (char *)dev);
        break;
    case USB_MSC_NOTIFY_UMOUNT:
        if(strstr(dev,"sd")){
            m_usb_state = USB_STAT_UNMOUNT;
        }else if(strstr(dev,"mmc")){
            m_usb_state = SD_STAT_UNMOUNT;
        }        
        break;
    case USB_MSC_NOTIFY_MOUNT_FAIL:
        if(strstr(dev,"sd")){
            m_usb_state = USB_STAT_MOUNT_FAIL;
        }else if(strstr(dev,"mmc")){
            m_usb_state = SD_STAT_MOUNT_FAIL;
        }
        break;
    case USB_MSC_NOTIFY_UMOUNT_FAIL:
        if(strstr(dev,"sd")){
            m_usb_state = USB_STAT_UNMOUNT_FAIL;
        }else if(strstr(dev,"mmc")){
            m_usb_state = SD_STAT_UNMOUNT_FAIL;
        }
        break;

#ifdef UIBC_SUPPORT
    case USB_HID_KBD_NOTIFY_CONNECT:
        printf("#### USB_HID_KBD_NOTIFY_CONNECT: %s\n", (char*)dev);
        usb_hid_register_fd(HID_KEYBOARD, dev);
        break;
    case USB_HID_KBD_NOTIFY_DISCONNECT:
        printf("#### USB_HID_KBD_NOTIFY_DISCONNECT: %s\n", (char*)dev);
        usb_hid_unregister_fd(HID_KEYBOARD, dev);
        break;
    case USB_HID_MOUSE_NOTIFY_CONNECT:
        printf("#### USB_HID_MOUSE_NOTIFY_CONNECT: %s\n", (char*)dev);
        usb_hid_register_fd(HID_MOUSE, dev);
        break;
    case USB_HID_MOUSE_NOTIFY_DISCONNECT:
        printf("#### USB_HID_MOUSE_NOTIFY_DISCONNECT: %s\n", (char*)dev);
        usb_hid_unregister_fd(HID_MOUSE, dev);
        break;
#endif

    default:
    #ifdef WIFI_SUPPORT
        usb_wifi_notify_check(action, dev);
    #endif
        return NOTIFY_OK;
        break;
    }
    int dev_len =  strlen(dev) + 16;
    char * stroage_devname = malloc(dev_len);
    //dev: sda1, etc, not like /media/hdd in linux
    snprintf(stroage_devname, dev_len-1, "%s/%s", MOUNT_ROOT_DIR, (char*)dev);

    /*need to free its memory after rsv message */ 
    partition_info_msg_send(m_usb_state,(uint32_t)stroage_devname);

    // partition_info_update(m_usb_state,dev);

    return NOTIFY_OK;
}

static struct notifier_block usb_switch_nb = {
        .notifier_call = usbd_notify,
};

#ifdef WIFI_SUPPORT
static void* hotplug_first_wifi_detect_probed(void* arg)
{
    FILE* fp;
    control_msg_t ctl_msg = {0};

    do
    {
        char path[64] = {0};
        for (int i = 0; i < sizeof(wifi_model_list)/sizeof(wifi_model_list[0]); i++)
        {
            snprintf(path, sizeof(path) - 1, "/dev/bus/usb/%s", wifi_model_list[i].desc);
            fp = fopen(path, "rb");
            if (fp)
            {
                fclose(fp);
                printf("detect wifi model: %s\n", wifi_model_list[i].name);

                memcpy(&g_hotplug_wifi_model, &wifi_model_list[i], sizeof(g_hotplug_wifi_model));
                network_wifi_module_set(g_hotplug_wifi_model.type);
                ctl_msg.msg_type = MSG_TYPE_USB_WIFI_PLUGIN;
                api_control_send_msg(&ctl_msg);
                
                goto EXIT;
            }

            snprintf(path, sizeof(path) - 1, "/dev/bus/sdio/%s", wifi_model_list[i].desc);
            fp = fopen(path, "rb");
            if (fp)
            {
                fclose(fp);
                printf("detect wifi model: %s\n", wifi_model_list[i].name);

                memcpy(&g_hotplug_wifi_model, &wifi_model_list[i], sizeof(g_hotplug_wifi_model));
                network_wifi_module_set(g_hotplug_wifi_model.type);
                ctl_msg.msg_type = MSG_TYPE_USB_WIFI_PLUGIN;
                api_control_send_msg(&ctl_msg);
                
                goto EXIT;
            }
        }

        usleep(500*1000);
    }
    while (1);

EXIT:
    return NULL;
}
#endif

static void _notifier_hdmi_tx_plugin(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    return ;
}

static void _notifier_hdmi_tx_plugout(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);
}

static void _notifier_hdmi_tx_ready(void *arg, unsigned long param){

    struct hdmi_edidinfo *edid = (struct hdmi_edidinfo *)param;
    printf("%s(), best_tvsys: %d\n", __func__, edid->best_tvsys);
    _hotplug_hdmi_tx_tv_sys_set();
}

static int hotplug_hdmi_tx_init(void)
{
    printf("%s:%d: \n", __func__, __LINE__);

    struct work_notifier_s notify_plugin;
    struct work_notifier_s notify_plugout;
    struct work_notifier_s notify_edid;
    memset(&notify_plugin, 0, sizeof(notify_plugin));
    memset(&notify_plugout, 0, sizeof(notify_plugout));
    memset(&notify_edid, 0, sizeof(notify_edid));

    notify_plugin.evtype   = HDMI_TX_NOTIFY_CONNECT;
    notify_plugin.qid          = LPWORK;
    notify_plugin.remote   = false;
    notify_plugin.oneshot  = false;
    notify_plugin.qualifier  = NULL;
    notify_plugin.arg          = NULL;
    notify_plugin.worker2  = _notifier_hdmi_tx_plugin;
    work_notifier_setup(& notify_plugin);

    notify_plugout.evtype   = HDMI_TX_NOTIFY_DISCONNECT;
    notify_plugout.qid          = LPWORK;
    notify_plugout.remote   = false;
    notify_plugout.oneshot  = false;
    notify_plugout.qualifier  = NULL;
    notify_plugout.arg          = NULL;
    notify_plugout.worker2  = _notifier_hdmi_tx_plugout;
    work_notifier_setup(& notify_plugout);

    notify_edid.evtype = HDMI_TX_NOTIFY_EDIDREADY;
    notify_edid.qid = LPWORK;
    notify_edid.remote = false;
    notify_edid.oneshot = false;
    notify_edid.qualifier = NULL;
    notify_edid.arg = (void*)(&notify_edid);
    notify_edid.worker2 = _notifier_hdmi_tx_ready;
    work_notifier_setup(& notify_edid);

    return 0;
}


static int hotplug_init()
{
    pthread_t pid;
    hotplug_hdmi_tx_init();
#ifdef WIFI_SUPPORT
    pthread_create(&pid,NULL,hotplug_first_wifi_detect_probed,NULL);
#endif

    return 0;
}

#else

#define MX_EVENTS (10)
#define EPL_TOUT (1000)

static int hotplug_init();

static int m_wifi_plugin = 0;

enum EPOLL_EVENT_TYPE
{
    EPOLL_EVENT_TYPE_KUMSG = 0,
    EPOLL_EVENT_TYPE_HOTPLUG_CONNECT,
    EPOLL_EVENT_TYPE_HOTPLUG_MSG,
};

struct epoll_event_data
{
    int fd;
    enum EPOLL_EVENT_TYPE type;
};

typedef struct{
    int epoll_fd;
    int hdmi_tx_fd;
    int kumsg_fd;
    int hotplug_fd;
}hotplug_fd_t;

static hotplug_fd_t m_hotplug_fd;
static struct epoll_event_data hotplg_data = { 0 };
//static struct epoll_event_data hotplg_msg_data = { 0 };
static struct epoll_event_data kumsg_data = { 0 };

static void process_hotplug_msg(char *msg)
{
    //plug-out: ACTION=wifi-remove INFO=v0BDApF179
    //plug-in: ACTION=wifi-add INFO=v0BDApF179

    control_msg_t ctl_msg = {0};
    const char *plug_msg;

    plug_msg = (const char*)msg;
    if(strstr(plug_msg, "ACTION=wifi"))
    {
    #ifdef WIFI_SUPPORT
        //usb wifi plugin/plugout message
        if (strstr(plug_msg, "ACTION=wifi-remove"))
        {
            if (0 == m_wifi_plugin)
                return;
            
            m_wifi_plugin = 0;
            printf("Wi-Fi plug-out\n");
            network_wifi_module_set(0);
            ctl_msg.msg_type = MSG_TYPE_USB_WIFI_PLUGOUT;
            api_control_send_msg(&ctl_msg);
        }
        else if (strstr(plug_msg, "ACTION=wifi-add"))
        {
            m_wifi_plugin = 1;
            if (strstr(plug_msg, "INFO=v0BDApF179"))
            {
                printf("Wi-Fi probed RTL8188_FTV\n");
                network_wifi_module_set(HCCAST_NET_WIFI_8188FTV);
                hccast_wifi_mgr_set_ap_ifname("wlan0");
                hccast_wifi_mgr_set_sta_ifname("wlan0");
                hccast_wifi_mgr_set_p2p_ifname("p2p0");
            }
            else if (strstr(plug_msg, "INFO=v0BDAp0179") ||
                    strstr(plug_msg, "INFO=v0BDAp8179"))
            {
                printf("Wi-Fi probed RTL8188_ETV\n");
                network_wifi_module_set(HCCAST_NET_WIFI_8188FTV);

            }
            else if ( strstr(plug_msg, "INFO=v0BDAp8733") ||
                    strstr(plug_msg, "INFO=v0BDApB733") ||
                    strstr(plug_msg, "INFO=v0BDApF72B"))
            {
                printf("Wi-Fi probed RTL8731BU\n");
                network_wifi_module_set(HCCAST_NET_WIFI_8733BU);

            }
            else if (strstr(plug_msg, "INFO=v0BDAp8731"))
            {
                printf("Wi-Fi probed RTL8731AU\n");
                network_wifi_module_set(HCCAST_NET_WIFI_8811FTV);
            }
            else if (strstr(plug_msg, "INFO=v0BDApC811"))
            {
                printf("Wi-Fi probed RTL8811_FTV\n");
                network_wifi_module_set(HCCAST_NET_WIFI_8811FTV);
            }
            else if (strstr(plug_msg, "INFO=vA69Cp8D81d"))
            {
                printf("Wi-Fi probed aic8800d\n");
                network_wifi_module_set(HCCAST_NET_WIFI_8800D);
                hccast_wifi_mgr_set_ap_ifname("wlan0");
                hccast_wifi_mgr_set_sta_ifname("wlan1");
                hccast_wifi_mgr_set_p2p_ifname("p2p0");
            }
            else if (strstr(plug_msg, "INFO=v3452p6600"))
            {
                printf("Wi-Fi probed ECR6600U\n");
                network_wifi_module_set(HCCAST_NET_WIFI_ECR6600U);
            }
            else
            {
                printf("Unknown Wi-Fi probed: %s!\n", plug_msg);
                return;
            }

            ctl_msg.msg_type = MSG_TYPE_USB_WIFI_PLUGIN;
            api_control_send_msg(&ctl_msg);
        }
    #endif
    }
    else
    {
        //usb disk plugin/plugout message
        char mount_name[32];
        //usb-disk is plug in (SD??)
        if (strstr(plug_msg, "ACTION=mount"))
        {
            sscanf(plug_msg, "ACTION=mount INFO=%s", mount_name);
            printf("U-disk is plug in: %s\n", mount_name);
            if(strstr(mount_name,"sd")|| strstr(mount_name,"hd")||strstr(mount_name,"usb")){
                m_usb_state = USB_STAT_MOUNT;
            }else if(strstr(mount_name,"mmc")){
                m_usb_state= SD_STAT_MOUNT;
            }
            //Enter upgrade window if there is upgraded file in USB-disk(hc_upgradexxxx.bin)
            char * stroage_devname=strdup(mount_name);
            /*need to free its memory after rsv message */ 
            partition_info_msg_send(m_usb_state,(uint32_t)stroage_devname);
        }    
        else if (strstr(plug_msg, "ACTION=umount"))
        {
            sscanf(plug_msg, "ACTION=umount INFO=%s", mount_name);
            printf("U-disk is plug out: %s\n", mount_name);
            if(strstr(mount_name,"sd")|| strstr(mount_name,"hd")||strstr(mount_name,"usb")){
                m_usb_state = USB_STAT_UNMOUNT;
            }else if(strstr(mount_name,"mmc")){
                m_usb_state= SD_STAT_UNMOUNT;
            }    
            char * stroage_devname=strdup(mount_name);
            /*need to free its memory after rsv message */ 
            partition_info_msg_send(m_usb_state,(uint32_t)stroage_devname);
        }
                         
   }
}


static void do_kumsg(KuMsgDH *msg)
{
    switch (msg->type)
    {
    case HDMI_TX_NOTIFY_CONNECT:
        //m_hdmi_tx_plugin = 1;
        printf("%s(), line: %d. hdmi tx connect\n", __func__, __LINE__);
        break;
    case HDMI_TX_NOTIFY_DISCONNECT:
        //m_hdmi_tx_plugin = 0;
        printf("%s(), line: %d. hdmi tx disconnect\n", __func__, __LINE__);
        break;
    case HDMI_TX_NOTIFY_EDIDREADY:
    {
        struct hdmi_edidinfo *edid = (struct hdmi_edidinfo *)&msg->params;
        printf("%s(), best_tvsys: %d\n", __func__, edid->best_tvsys);
        _hotplug_hdmi_tx_tv_sys_set();
        break;
    }
    default:
        break;
    }
}


static void *hotplug_receive_event_func(void *arg)
{
    struct epoll_event events[MX_EVENTS];
    int n = -1;
    int i;
    struct sockaddr_in client;
    socklen_t sock_len = sizeof(client);
    int len;

    while (1)
    {
        n = epoll_wait(m_hotplug_fd.epoll_fd, events, MX_EVENTS, EPL_TOUT);
        if (n == -1)
        {
            if (EINTR == errno)
            {
                continue;
            }
            usleep(100 * 1000);
            continue;
        }
        else if (n == 0)
        {
            continue;
        }

        for (i = 0; i < n; i++)
        {
            struct epoll_event_data *d = (struct epoll_event_data *)events[i].data.ptr;
            int fd = (int)d->fd;
            enum EPOLL_EVENT_TYPE type = d->type;

            switch (type)
            {
                case EPOLL_EVENT_TYPE_KUMSG:
                {
                    unsigned char msg[MAX_KUMSG_SIZE] = {0};
                    len = read(fd, (void *)msg, MAX_KUMSG_SIZE);
                    if (len > 0)
                    {
                        do_kumsg((KuMsgDH *)msg);
                    }
                    break;
                }
                case EPOLL_EVENT_TYPE_HOTPLUG_CONNECT:
                {
                    printf("%s(), line: %d. get hotplug connect...\n", __func__, __LINE__);
                    struct epoll_event ev;
                    int new_sock = accept(fd, (struct sockaddr *)&client, &sock_len);
                    if (new_sock < 0)
                        break;

                    struct epoll_event_data *epoll_event = (struct epoll_event_data *)malloc(sizeof(struct epoll_event_data));
                    epoll_event->fd = new_sock;
                    epoll_event->type = EPOLL_EVENT_TYPE_HOTPLUG_MSG;
                    ev.events = EPOLLIN;
                    ev.data.ptr = (void *)epoll_event;
                    if (epoll_ctl(m_hotplug_fd.epoll_fd, EPOLL_CTL_ADD, new_sock, &ev) == -1)
                    {
                        perror("epoll_ctl: EPOLL_CTL_ADD");
                        close(new_sock);
                        free(epoll_event);
                    }
                    break;
                }
                case EPOLL_EVENT_TYPE_HOTPLUG_MSG:
                {
                    printf("%s(), line: %d. get hotplug msg...\n", __func__, __LINE__);
                    char msg[128] = {0};
                    len = read(fd, (void*)msg, sizeof(msg) - 1);
                    if (len > 0)
                    {
                        printf("%s\n", msg);
                        if (strstr(msg, "ACTION="))
                        {
                            process_hotplug_msg(msg);
                        }
                    }
                    else if (len == 0)
                    {
                        printf("Connection closed by peer\n");
                    }
                    else if (len < 0)
                    {
                        if (errno != EINTR && errno != EAGAIN)
                        {
                            perror("read error");
                        }
                        else
                        {
                            printf("read hotplug msg fail: %s\n", strerror(errno));
                        }
                    }

                    if (epoll_ctl(m_hotplug_fd.epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
                    {
                        perror("epoll_ctl: EPOLL_CTL_DEL");
                    }
                    close(fd);
                    free(d);

                    break;
                }
                default:
                    break;
            }
        }

        usleep(10 * 1000);

    }

    return NULL;
}

#ifdef UIBC_SUPPORT
static void process_uevent_message(const char* msg)
{
    if (!memcmp(msg, "add@", 4))
    {
        if (!memcmp(&msg[strlen(msg) - 7], "/event", 6))
        {
            printf("Found event: ");
            printf("%s\n", &msg[strlen(msg) - 6]);

            char path[256] = "";
            snprintf(path, sizeof(path), "/dev/input/%s", &msg[strlen(msg) - 6]);

            int fd = open(path, O_RDONLY);
            if (fd == -1)
            {
                perror("open");
                return;
            }

            hid_type_e type = usb_hid_get_device_type(fd);

            close(fd);

            if (type == HID_KEYBOARD || type == HID_MOUSE)
            {
                usb_hid_register_fd(type, path);
            }
        }
    }
    else if (!memcmp(msg,"remove@",7))
    {
        if (!memcmp(&msg[strlen(msg) - 7],"/event",6))
        {
            printf("remove event: ");
            printf("%s\n", &msg[strlen(msg) - 6]);

            char path[256] = "";
            snprintf(path, sizeof(path), "/dev/input/%s", &msg[strlen(msg) - 6]);
            usb_hid_unregister_fd(0, path);
        }
    }
}

static void *hotplug_receive_uevent_func(void *arg)
{
    struct sockaddr_nl sa;
    int sock_fd, len;
    char buf[NETLINK_BUFFER_SIZE];
    struct iovec iov = { .iov_base = buf, .iov_len = NETLINK_BUFFER_SIZE };
    struct msghdr msg = { .msg_name = &sa, .msg_namelen = sizeof(sa), .msg_iov = &iov, .msg_iovlen = 1 };

    sock_fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (sock_fd == -1)
    {
        perror("Socket creation failed");
        return NULL;
    }

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_pid = getpid();
    sa.nl_groups = 0xFFFFFFFF;

    if (bind(sock_fd, (struct sockaddr*)&sa, sizeof(sa)) == -1)
    {
        perror("Binding failed");
        close(sock_fd);
        return NULL;
    }

    while (1)
    {
        len = recvmsg(sock_fd, &msg, 0);
        if (len <= 0)
        {
            if (errno == EINTR)
                continue;
            perror("recvmsg");
            break;
        }

        for (char *p = buf; p < buf + len; )
        {
            char *end = strchr(p, '\n');
            if (end == NULL)
                end = buf + len;
            *end = '\0';
            p = end + 1;
        }

        process_uevent_message(buf);
    }

    close(sock_fd);
    return 0;
}
#endif

static int hotplug_init()
{
    pthread_attr_t attr;
    pthread_t tid;
    struct sockaddr_un serv;
    struct epoll_event ev;
    struct kumsg_event event = { 0 };
    
    int ret;

    // start hot-plug detect
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&tid, &attr, hotplug_receive_event_func, (void *)NULL))
    {
        printf("pthread_create receive_event_func fail\n");
        pthread_attr_destroy(&attr);
        goto out;
    }
    pthread_attr_destroy(&attr);

#ifdef UIBC_SUPPORT
    // start hot-plug hid uevent detect
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&tid, &attr, hotplug_receive_uevent_func, (void *)NULL))
    {
        printf("pthread_create hotplug_receive_uevent_func fail\n");
        pthread_attr_destroy(&attr);
        goto out;
    }
    pthread_attr_destroy(&attr);
#endif

    m_hotplug_fd.hotplug_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (m_hotplug_fd.hotplug_fd < 0)
    {
        printf("socket error\n");
        goto out;
    }
    else
    {
        printf("socket success\n");
    }

    unlink("/tmp/hotplug.socket");
    bzero(&serv, sizeof(serv));
    serv.sun_family = AF_LOCAL;
    snprintf(serv.sun_path, sizeof(serv.sun_path),"/tmp/hotplug.socket");//strcpy(serv.sun_path, "/tmp/hotplug.socket");
    ret = bind(m_hotplug_fd.hotplug_fd, (struct sockaddr *)&serv, sizeof(serv));
    if (ret < 0)
    {
        printf("bind error\n");
        goto out;
    }
    else
    {
        printf("bind success\n");
    }

    ret = listen(m_hotplug_fd.hotplug_fd, 1);
    if (ret < 0)
    {
        printf("listen error\n");
        goto out;
    }
    else
    {
        printf("listen success\n");
    }

    m_hotplug_fd.epoll_fd = epoll_create1(0);

    hotplg_data.fd = m_hotplug_fd.hotplug_fd;
    hotplg_data.type = EPOLL_EVENT_TYPE_HOTPLUG_CONNECT;
    ev.events = EPOLLIN;
    ev.data.ptr = (void *)&hotplg_data;
    if (epoll_ctl(m_hotplug_fd.epoll_fd, EPOLL_CTL_ADD, m_hotplug_fd.hotplug_fd, &ev) != 0)
    {
        printf("EPOLL_CTL_ADD hotplug fail\n");
        goto out;
    }
    else
    {
        printf("EPOLL_CTL_ADD hotplug success\n");
    }

    //hdmi hotplug message
    m_hotplug_fd.hdmi_tx_fd = open(DEV_HDMI, O_RDWR);
    if (m_hotplug_fd.hdmi_tx_fd < 0)
    {
        printf("%s(), line:%d. open device: %s error!\n",
                __func__, __LINE__, DEV_HDMI);
        goto out;
    }
    m_hotplug_fd.kumsg_fd = ioctl(m_hotplug_fd.hdmi_tx_fd, KUMSGQ_FD_ACCESS, O_CLOEXEC);
    kumsg_data.fd = m_hotplug_fd.kumsg_fd;
    kumsg_data.type = EPOLL_EVENT_TYPE_KUMSG;
    ev.events = EPOLLIN;
    ev.data.ptr = (void *)&kumsg_data;
    if (epoll_ctl(m_hotplug_fd.epoll_fd, EPOLL_CTL_ADD, m_hotplug_fd.kumsg_fd, &ev) != 0)
    {
        printf("EPOLL_CTL_ADD fail\n");
        goto out;
    }

    event.evtype = HDMI_TX_NOTIFY_CONNECT;
    event.arg = 0;
    ret = ioctl(m_hotplug_fd.hdmi_tx_fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret)
    {
        printf("KUMSGQ_NOTIFIER_SETUP 0x%08x fail\n", (int)event.evtype);
        goto out;
    }

    event.evtype = HDMI_TX_NOTIFY_DISCONNECT;
    event.arg = 0;
    ret = ioctl(m_hotplug_fd.hdmi_tx_fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret)
    {
        printf("KUMSGQ_NOTIFIER_SETUP 0x%08x fail\n", (int)event.evtype);
        goto out;
    }
    event.evtype = HDMI_TX_NOTIFY_EDIDREADY;
    event.arg = 0;
    ret = ioctl(m_hotplug_fd.hdmi_tx_fd, KUMSGQ_NOTIFIER_SETUP, &event);
    if (ret)
    {
        printf("KUMSGQ_NOTIFIER_SETUP 0x%08x fail\n", (int)event.evtype);
        goto out;
    }


out:
    return 0;

}
#endif


static uint16_t screen_init_rotate,screen_init_h_flip,screen_init_v_flip;
void api_get_screen_rotate_info(void)
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

#ifdef __linux__
static void exit_console(int signo)
{
    printf("%s(), signo: %d, error: %s\n", __FUNCTION__, signo, strerror(errno));

    api_watchdog_stop();

    //exit(): exit app after the resource is released!!
    //_exit(): exit at once
    exit(signo);
}

static void signal_normal(int signo)
{
    printf("%s(), signo: %d, error: %s\n", __FUNCTION__, signo, strerror(errno));

    //reset the SIGPIPE, so that it can be catched again
    signal(signo, signal_normal); 
}

#endif


void api_system_pre_init(void)
{
    partition_info_init();
#ifdef __HCRTOS__    
    //usb mount/umount notify must registed in advance,
    //otherwise when bootup, App may not receive the notifications from driver.
    sys_register_notify(&usb_switch_nb);
#endif    
}

int api_system_init(void)
{

#ifdef __HCRTOS__    
    api_romfs_resources_mount();
#else
    //start up wifi/network service script
    system("/etc/wifiprobe.sh &");

#endif    
    hotplug_init();
#ifdef BACKLIGHT_MONITOR_SUPPORT
    api_pwm_backlight_monitor_init();
#else
    api_set_backlight_brightness(projector_get_some_sys_param(P_BACKLIGHT));
#endif
    api_set_i2so_gpio_mute(false);

    if (!api_watchdog_init()){
        api_watchdog_timeout_set(WATCHDOG_TIMEOUT);
        api_watchdog_start();
        uint32_t timeout_ms = 0;;
        api_watchdog_timeout_get(&timeout_ms);
        printf("%s(), line:%d. set watchdog timeout: %u ms\n", __func__, __LINE__, (unsigned int)timeout_ms);
    }

#ifdef __linux__
    signal(SIGTERM, exit_console); //kill signal
    signal(SIGINT, exit_console); //Ctrl+C signal
    signal(SIGSEGV, exit_console); //segmentation fault, invalid memory
    signal(SIGBUS, exit_console);  //bus error, memory addr is not aligned.
    signal(SIGPIPE, signal_normal);  //SIGPIPE is a disconnect message(TCP), do not need exit app.
#endif

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

    get_rotate_by_flip_mode(projector_get_some_sys_param(P_FLIP_MODE) ,
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

void api_get_ratio_rect(struct vdec_dis_rect *new_rect, dis_tv_mode_e ratio)
{
    int h = get_display_h();
    int v = get_display_v();

    printf("h1: %d, v1: %d\n", h, v);
#ifdef SYS_ZOOM_SUPPORT
    int zoom_mode = projector_get_some_sys_param(P_SYS_ZOOM_DIS_MODE);
    zoom_mode = zoom_mode == DIS_TV_AUTO ? get_screen_mode() : zoom_mode;
#else
    int zoom_mode = DIS_TV_AUTO;
#endif
    if(zoom_mode == DIS_TV_4_3)//ui为4:3缩放
    {
        if(ratio == DIS_TV_16_9)
        {    
            if(get_screen_is_vertical())
            {
                h = h*3/4;//已验证
            }
            else
            {
                v = v*3/4;                    
            }
        }
    }
    else if(zoom_mode == DIS_TV_16_9)
    {
        if(ratio == DIS_TV_4_3)
        {
            if(get_screen_is_vertical())
            {
                v = v*3/4;//已验证

            }else{
                 h = h*3/4;
            }
        }
    }else if(zoom_mode == DIS_TV_AUTO){
        auto_mode_get_rect_by_ratio(&h, &v, ratio);
    }
    
    int x = get_display_x()+(get_display_h()-h)/2;
    int y = get_display_y()+(get_display_v()-v)/2;
    printf("h2: %d, v2: %d, x: %d, y: %d\n", h, v, x, y);
    new_rect->dst_rect.x =  x;
    new_rect->dst_rect.y =  y;
    new_rect->dst_rect.w =  h;
    new_rect->dst_rect.h =  v;
    new_rect->src_rect.x = 0;
    new_rect->src_rect.y = 0;
    new_rect->src_rect.w = 1920;
    new_rect->src_rect.h = 1080;
}

static int m_logo_player = 0;
static char m_logo_file[128] = {0};

int api_logo_off_no_black(void)
{
    if (m_logo_player){
        com_logo_off(0, 0);
    }
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
    char *file_path = file;
    int rotate_type = ROTATE_TYPE_0;
    int flip_type = MIRROR_TYPE_NONE;
    
    api_logo_off();

    if (!file)
        file_path = BACK_LOGO;

    strcpy(m_logo_file, file_path);
    
    api_get_flip_info(&rotate_type, &flip_type);
    
    if (com_logo_show(file_path, rotate_type, flip_type) < 0)
    {
        printf("com_logo_show() fail!\n");
        return -1;
    }

    m_logo_player = 1;
    printf("Show logo: %s ok!\n", file_path);
    return 0;
}

int api_logo_off()
{
    if (m_logo_player){
        com_logo_off(1, 0);
    }
    m_logo_player = 0;
   
    return 0;
}

int api_logo_off2(int closevp, int fillblack)
{
    com_logo_off(closevp, fillblack);

    return 0;
}

int api_logo_dis_backup_free(void)
{
    com_logo_dis_backup_free();
    
    return 0;
}

//Suspend dis before entering standby
int api_dis_suspend(void)
{
    int fd = -1;
    struct dis_suspend_resume   suspend_resume = { 0 };

    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0)
    {
        printf("%s(), line:%d, open dis error!\n" , __func__ , __LINE__);
        return -1;
    }

    suspend_resume.distype = DIS_TYPE_HD;

    ioctl(fd , DIS_SET_SUSPEND , &suspend_resume);
    close(fd);

    return 0;
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
    if (fd < 0) {
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
    if (INVALID_ID == m_control_msg_id){
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id){
            return -1;
        }
    }
    return api_message_send(m_control_msg_id, control_msg, sizeof(control_msg_t));
}

int api_control_receive_msg(control_msg_t *control_msg)
{
    if (INVALID_ID == m_control_msg_id){
        return -1;
    }
    return api_message_receive_tm(m_control_msg_id, control_msg, sizeof(control_msg_t), 5);
}

void api_control_clear_msg(void)
{
    control_msg_t msg_buffer;
    while(1){
        if(api_control_receive_msg(&msg_buffer)){
            break;
        }
    }
    return;
}

int api_control_send_key(uint32_t key)
{
    control_msg_t control_msg;
    control_msg.msg_type = MSG_TYPE_KEY;
    control_msg.msg_code = key;

    if (INVALID_ID == m_control_msg_id){
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id){
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
 * should be save tho "/sys/class/net/wlan0/address"
 * @param
 * @return
 */
 #ifdef WIFI_SUPPORT
int api_get_mac_addr(char *mac)
{
#ifdef __linux__
    int ret = 0;
    char buffer[32] = {0};
    int fd = 0;
    int err = 0;

    if(mac == NULL){
        printf("%s:%d: the parameter is invalid\n", __func__, __LINE__);
        return -1;
    }

    fd = open(MAC_ADDRESS_PATH, O_RDONLY);
    if(fd < 0) {
        printf("err: api_get_mac_addr: could not open %s, %s", MAC_ADDRESS_PATH, strerror(errno));
        return -1;
    }

    ret = read(fd, (void *)&buffer, sizeof(buffer));
    if(ret <= 0) {
        printf("api_get_mac_addr: read failed, %s", strerror(errno));
        err = -2;
        goto end;
    }

    printf("%s mac: %s\n", MAC_ADDRESS_PATH, buffer);

    sscanf(buffer, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
            &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

end:
    close(fd);

    return err;
#else
    struct ifreq ifr;
    int skfd;

    if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        printf("socket error\n");
        return -1;
    }

    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ);
    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) != 0)
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

#endif

void app_ffplay_init(void)
{
    if (m_ffplay_init) return;

    hcplayer_init(1);
    m_ffplay_init = true;
}

void app_ffplay_deinit(void)
{
    hcplayer_deinit();
    m_ffplay_init = false;
}

/**
 * @brief linux system will send the exit signal, then release system resource here
 * 
 */
void app_exit(void)
{
    if (INVALID_ID != m_control_msg_id){
        api_message_delete(m_control_msg_id);
        m_control_msg_id = INVALID_ID;
    }

    app_ffplay_deinit();
}

void api_sleep_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

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
    if (fgets(result_buf, sizeof(result_buf), fp) != NULL){
        if('\n' == result_buf[strlen(result_buf)-1]){
            result_buf[strlen(result_buf)-1] = '\0';
        }

        sscanf(result_buf,"%d", &result_val);
    }
    pclose(fp);
    if (0 == result_val){
        printf("Excute cmd: [%s] OK!\n", cmd);
        ret = API_SUCCESS;
    }else{
        printf("Excute cmd: [%s] fail, ret = %d!\n", cmd, result_val);
        ret = API_FAILURE;
    }

    return ret;

}

int api_osd_show_onoff(bool on_off)
{
    pthread_mutex_lock(&m_osd_off_mutex);    
    // Open the file for reading and writing
    int fbfd = open("/dev/fb0", O_RDWR);
    uint32_t blank_mode;

    if(fbfd == -1) {
        printf("%s(), line: %d. Error: cannot open framebuffer device", __func__, __LINE__);
        pthread_mutex_unlock(&m_osd_off_mutex);
        return API_FAILURE;
    }

    m_osd_off_time_flag = 0;

    if (on_off)
        blank_mode = FB_BLANK_UNBLANK;
    else
        blank_mode = FB_BLANK_NORMAL;

    if (ioctl(fbfd, FBIOBLANK, blank_mode) != 0) {
        printf("%s(), line: %d. Error: FBIOBLANK", __func__, __LINE__);
    }

    close(fbfd);
    pthread_mutex_unlock(&m_osd_off_mutex);
    return API_SUCCESS;
}


//send message to other task
int api_control_send_media_message(uint32_t mediamsg_type)
{
    control_msg_t control_msg;
    control_msg.msg_type = MSG_TYPE_MSG;
    control_msg.msg_code = mediamsg_type;

    if (INVALID_ID == m_control_msg_id){
        m_control_msg_id = api_message_create(CTL_MSG_COUNT, sizeof(control_msg_t));
        if (INVALID_ID == m_control_msg_id){
            return -1;
        }
    }
    return api_message_send(m_control_msg_id, &control_msg, sizeof(control_msg_t));

}

#ifdef __linux__
int api_dts_uint32_get(const char *path)
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
    //printf("fd:%d ,dts value: %x\n", fd,value);
    return value;
}

void api_dts_string_get(const char *path, char *string, int size)
{
    int fd = open(path, O_RDONLY);

    if(fd >= 0){
        read(fd, string, size);
        close(fd);
    }
    //printf("dts string: %s\n", string);
}

#include <sys/mman.h>
#define SYSTEM_WDT_ADDR (0xb8818500 & 0x1fffffff)
#define PAGE_SIZE 4096
#define PAGE_ADDR_ALIGN(addr)  (addr & ~(uint32_t)(PAGE_SIZE - 1))
#define PAGE_ADDR_OFFSET(addr)  (addr & (PAGE_SIZE - 1))

typedef struct WDT_MMAP_INFO{
    void *virt_addr;
    int wdt_fd;
    void *map_addr;
}WDT_MMAP_INFO_t;
static WDT_MMAP_INFO_t m_wdt_mmap = {0};

void api_hw_watchdog_mmap_open(void)
{
    uint32_t addr_align = 0;
    uint32_t addr_offset = 0;
    void *map_addr = 0;
    void *virt_addr = 0;

    if (m_wdt_mmap.map_addr)
        return;

    int fd =  open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
        return;

    addr_align = PAGE_ADDR_ALIGN(SYSTEM_WDT_ADDR);
    addr_offset = PAGE_ADDR_OFFSET(SYSTEM_WDT_ADDR);

    printf("%s(), addr_align=0x%x, addr_offse=0x%x!\n", __func__, addr_align,addr_offset);

    //size and address should be aligned page size(4096)
    map_addr = (void*)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, addr_align);
    printf("%s(), return map_addr=0x%x!\n", __func__, (unsigned int)map_addr);
    if (0 == (uint32_t)map_addr || 0xFFFFFFFF == (uint32_t)map_addr)
        return;

    virt_addr = (char*)map_addr + addr_offset;
    //printf("%s(), return val: 0x%x!\n", __func__, *(uint32_t*)virt_addr);

    m_wdt_mmap.map_addr = map_addr;
    m_wdt_mmap.wdt_fd = fd;
    m_wdt_mmap.virt_addr = virt_addr;

}

void api_hw_watchdog_mmap_close(void)
{
    if (m_wdt_mmap.map_addr){
        munmap(m_wdt_mmap.map_addr, PAGE_SIZE);
    }

    if (m_wdt_mmap.wdt_fd > 0){
        close(m_wdt_mmap.wdt_fd);
    }

    memset(&m_wdt_mmap, 0, sizeof(m_wdt_mmap));
}

static void api_hw_watchdog_reboot(void)
{
    printf("%s()!\n", __func__);    
    volatile uint32_t base_addr = 0;
    if (!m_wdt_mmap.map_addr){
        printf("%s(), No mmap!", __func__);
        return;
    }

    base_addr = (uint32_t)(m_wdt_mmap.virt_addr);
    *(uint32_t*)base_addr = 0xfffffffa;
    *(uint32_t*)(base_addr+4) = 0x26;
}

#endif

int api_wifi_disconnect(void)
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
#ifdef __linux__
    //system("reboot -f");
    api_hw_watchdog_mmap_open();
    api_hw_watchdog_reboot();
#else
    extern int reset(void);
    reset();
#endif
    while(1);
}

static glist *m_screen_list = NULL;
void api_screen_regist_ctrl_handle(screen_entry_t *entry)
{
    screen_entry_t *entry_tmp;
    glist *glist_tmp = NULL;

    glist_tmp = m_screen_list;
    while(glist_tmp){
        entry_tmp = glist_tmp->data;
        if (entry_tmp->screen == entry->screen){
            entry_tmp->control = entry->control;
            return;
        }
        glist_tmp = glist_tmp->next;
    }

    entry_tmp = (screen_entry_t*)malloc(sizeof(screen_entry_t));
    memcpy(entry_tmp, entry, sizeof(screen_entry_t));
    //printf("%s(), screen:%x, ctrl:%x\n", __func__, entry_tmp->screen, entry_tmp->control);
    m_screen_list = glist_append(m_screen_list, (void*)entry_tmp);
}

screen_ctrl api_screen_get_ctrl(void *screen)
{
    screen_entry_t *entry_tmp;
    glist *glist_tmp = NULL;

    glist_tmp = m_screen_list;
    while(glist_tmp){
        entry_tmp = glist_tmp->data;
        if (entry_tmp->screen == screen){
            return entry_tmp->control;
        }
        glist_tmp = glist_tmp->next;
    }

    return NULL;
}

screen_entry_t* api_screen_get_ctrl_entry(void *screen){
    screen_entry_t *entry_tmp;
    glist *glist_tmp = NULL;

    glist_tmp = m_screen_list;
    while(glist_tmp){
        entry_tmp = glist_tmp->data;
        if (entry_tmp->screen == screen){
            return entry_tmp;
        }
        glist_tmp = glist_tmp->next;
    }

    return NULL;
}



/*
we can wake up by 3 ways: ir, gpio and sacadc key. 
 */
void api_system_standby(void)
{
    int fd = -1;

    fd = open("/dev/standby", O_RDWR);
    if (fd < 0) {
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


#if 0
    //Do not need config wakeup mode here. standby driver would get
    //parameters from dts node while initializes: standby {...}. Of cause, the customer
    //can change wakeup mode again before enter standby here if necessary


    //Step 3: config the standby wake up methods
        
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

    //config wake up adc key
    struct standby_saradc_setting adc = { 0 };
    adc.channel = 1;
    adc.min = 1000;
    adc.max = 1500;
    ioctl(fd, STANDBY_SET_WAKEUP_BY_SARADC, (unsigned long)&adc);



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

#define ONE_COUNT_TIME  100
static void *osd_off_for_time(void *arg)
{
    uint32_t timeout = (uint32_t)arg;
    char time_flag;
    m_osd_off_time_cnt = timeout/ONE_COUNT_TIME;
    do{
        api_sleep_ms(ONE_COUNT_TIME);
    }while(m_osd_off_time_cnt --);

    m_osd_off_time_cnt = 0;
    pthread_mutex_lock(&m_osd_off_mutex);
    time_flag = m_osd_off_time_flag;
    pthread_mutex_unlock(&m_osd_off_mutex);
    if (time_flag)
        api_osd_show_onoff(true);

    return NULL;
}

//Turn off OSD for a time, then turn on OSD.
//because sometimes enter dlna music play, the OSD is still show
//but the video screen is black(BUG #2848), so we turn off OSD for some time.
void api_osd_off_time(uint32_t timeout)
{

    //update the wait time
    if (m_osd_off_time_cnt){
        int timeout_cnt = timeout/ONE_COUNT_TIME;
        if (timeout_cnt > m_osd_off_time_cnt)
            m_osd_off_time_cnt = timeout_cnt;

        return;
    }

    api_osd_show_onoff(false);
    pthread_mutex_lock(&m_osd_off_mutex);
    m_osd_off_time_flag = 1;
    pthread_mutex_unlock(&m_osd_off_mutex);
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x1000);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&thread_id, &attr, osd_off_for_time, (void*)timeout)) {
        pthread_attr_destroy(&attr);
        return;
    }
    pthread_attr_destroy(&attr);    
}


static bool m_hotkey_all_enable = true;
#define ENBALE_KEY_MAX 10
static uint32_t m_hotkey_enable_key[ENBALE_KEY_MAX] = {0,};
static uint32_t m_enable_key_cnt = 0;
void api_hotkey_enable_set(uint32_t *enable_key, uint32_t key_cnt)
{
    m_enable_key_cnt = key_cnt > ENBALE_KEY_MAX ? ENBALE_KEY_MAX :  key_cnt;
    if (!enable_key)
        m_enable_key_cnt = 0;

    if (enable_key && key_cnt)
        memcpy(m_hotkey_enable_key, enable_key, m_enable_key_cnt*sizeof(uint32_t));

    m_hotkey_all_enable = false;
}

void api_hotkey_disable_clear(void)
{
    m_enable_key_cnt = 0;
    m_hotkey_all_enable = true;
}

void api_hotkey_disable_all(void)
{
    m_enable_key_cnt = 0;
    m_hotkey_all_enable = false;
}

bool api_hotkey_enable_get(uint32_t key)
{
    int i;
    if (m_hotkey_all_enable)
        return true;

    for (i = 0; i < m_enable_key_cnt; i ++){
        if (key == m_hotkey_enable_key[i])
            return true;
    }
    return false;
}
static bt_connect_status_e bt_connet_status = BT_CONNECT_STATUS_DEFAULT;
void api_set_bt_connet_status(bt_connect_status_e val)
{
    bt_connet_status = val;
}

bt_connect_status_e api_get_bt_connet_status(void)
{
    return bt_connet_status;
}

#define SOUND_DEV   "/dev/sndC0i2so"
#define DEFAULT_VOL 50
static uint8_t m_vol_back = DEFAULT_VOL;
// vol and mute ctrl i2si when cvbs in,else ctrl i2so
static int64_t last_op_time = 0;
#define OPT_TIME 200 //uint ms 
static int api_mute_ctrl(bool mute,int snd_fd)
{
    if(snd_fd<0){
        return API_FAILURE;
    }
    uint8_t volume = 0;
    if (mute){
#ifdef SUPPORT_INPUT_BLUE_SPDIF_IN               
       api_set_i2so_gpio_mute(1);
#endif         
    #ifdef BT_AC6956C_GX
        /* reduce command send when long press cause bluetooth module crash  */ 
        if (last_op_time != 0 && (api_get_sys_clock_time() - last_op_time) < OPT_TIME) {
            return API_FAILURE;
        }
        last_op_time = api_get_sys_clock_time();
        bluetooth_ioctl(BLUETOOTH_GET_MUSIC_VOL_RESULTS,&m_vol_back);
        bluetooth_ioctl(BLUETOOTH_SET_MUSIC_VOL_VALUE,volume);
        return API_SUCCESS;
    #endif 
        ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &m_vol_back);
        ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);

        //Set muet GPIO, may be used in CVBS in while 
        //CVBS in is link to line out directly.
        if(api_get_bt_connet_status()){
            ioctl(snd_fd, SND_IOCTL_SET_MUTE, 1);
        }

    } else {
        volume = m_vol_back;
    #ifdef BT_AC6956C_GX
        /* reduce command send when long press cause bluetooth module crash  */ 
        if (last_op_time != 0 && (api_get_sys_clock_time() - last_op_time) < OPT_TIME) {
            return API_FAILURE;
        }
        last_op_time = api_get_sys_clock_time();
        bluetooth_ioctl(BLUETOOTH_SET_MUSIC_VOL_VALUE,m_vol_back);
        return API_SUCCESS;
    #endif 
#ifdef SUPPORT_INPUT_BLUE_SPDIF_IN        
        api_set_i2so_gpio_mute(0);
#endif  
        ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);

        //Set muet GPIO, may be used in CVBS in while 
        //CVBS in is link to line out directly.
        if(!api_get_bt_connet_status()){
            ioctl(snd_fd, SND_IOCTL_SET_MUTE, 0);        
        }
    }

    printf("mute is %d, vol: %d\n", mute, volume);
    return API_SUCCESS;
}
// #include 
int api_media_mute(bool mute)
{
    int snd_fd_i2so=-1;
    int ret = API_SUCCESS;
    snd_fd_i2so = open("/dev/sndC0i2so", O_WRONLY);
    if(snd_fd_i2so<0){
        printf("open snd_fd_i2so fail\n");
        return API_FAILURE;
    }else{
        ret = api_mute_ctrl(mute,snd_fd_i2so);
        close(snd_fd_i2so);
    }
#ifdef CVBS_AUDIO_I2SI_I2SO
    int snd_fd_i2si=-1;
    snd_fd_i2si = open("/dev/sndC0i2si", O_WRONLY);
    if(snd_fd_i2si<0){
        printf("open snd_fd_i2si fail\n");
        return API_FAILURE;
    }else{
        ret = api_mute_ctrl(mute,snd_fd_i2si);
        close(snd_fd_i2si);
    }
#endif
    return ret;

}

int api_media_set_vol(uint8_t volume)
{
    int snd_fd = -1;
#ifdef BT_AC6956C_GX
    // ctrl bt vol and do not ctrl soc snddev vol
    bluetooth_ioctl(BLUETOOTH_SET_MUSIC_VOL_VALUE,volume);
    return API_SUCCESS;
#endif

    snd_fd = open(SOUND_DEV, O_WRONLY);
    if (snd_fd < 0) {
        printf ("open snd_fd %d failed\n", snd_fd);
        return API_FAILURE;
    }

    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
    volume = 0;
    close(snd_fd);
    return API_SUCCESS;
}

// gpio mute
void api_set_i2so_gpio_mute(bool val)//1 mute false no mute
{
    int snd_fd = -1;
#if defined(SUPPORT_INPUT_BLUE_SPDIF_IN)

#ifdef BLUETOOTH_SUPPORT        
    bluetooth_set_gpio_mutu(val);    
    if (api_get_bt_connet_status() == BT_CONNECT_STATUS_CONNECTED)
        SET_MUTE
    else    
#endif     
    if(val) SET_MUTE
    else SET_UNMUTE
   //  printf("bt_get_connet_state = %d  val = %d\n",api_get_bt_connet_status(),val);


#else
#ifdef BLUETOOTH_SUPPORT
    /*mute pin is link to bt pin in some moudle*/ 
    bluetooth_set_gpio_mutu(val);
    if(api_get_bt_connet_status() == BT_CONNECT_STATUS_CONNECTED && !val)// snd --x-->i2so, but snd ->bt
        return;
#endif
    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0) {
        printf ("open snd_fd %d failed\n", snd_fd);
        return;
    }

#ifdef DRC_GAIN_SUPPORT
    struct snd_drc_setting setting = { 0 };
    ioctl(snd_fd, SND_IOCTL_GET_DRC_PARAM, &setting);
    setting.gain_dBFS = CONFIG_APP_DRC_GAIN_LEVEL;  //Gain to be set
    setting.peak_dBFS = -12;                          //Peak value to be set
    ioctl(snd_fd, SND_IOCTL_SET_DRC_PARAM, &setting);
#endif

    ioctl(snd_fd, SND_IOCTL_SET_MUTE, val);
    close(snd_fd);
#endif     
}

// this api would be deprecated.
void api_set_i2so_gpio_mute_auto(void)
{
    return ;
    int snd_fd = -1;
    uint8_t vol_back = 0;
    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0) {
        printf ("open snd_fd %d failed\n", snd_fd);
        return;
    }
#ifdef BLUETOOTH_SUPPORT
    if(api_get_bt_connet_status() == BT_CONNECT_STATUS_CONNECTED)
        ioctl(snd_fd, SND_IOCTL_SET_MUTE, 1);
    else
#endif
    {
        ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &vol_back);
        printf("vol_back =%d\n", vol_back);
        if(vol_back==0)
            ioctl(snd_fd, SND_IOCTL_SET_MUTE, 1);
        else
            ioctl(snd_fd, SND_IOCTL_SET_MUTE, 0);
    }
    close(snd_fd);
    printf("bt_get_connet_state = %d\n",api_get_bt_connet_status());
}

#ifdef __HCRTOS__
/* Load HDCP key*/
int api_get_mtdblock_devpath(char *devpath, int len, const char *partname)
{
    int np = -1;
    static u32 part_num = 0;
    u32 i = 1;
    const char *label;
    char property[32];

    if (np < 0) {
        np = fdt_get_node_offset_by_path("/hcrtos/sfspi/spi_nor_flash/partitions");
    }

    if (np < 0)
        return -1;

    if (part_num == 0)
        fdt_get_property_u_32_index(np, "part-num", 0, &part_num);

    for (i = 1; i <= part_num; i++) {
        snprintf(property, sizeof(property), "part%d-label", i);
        if (!fdt_get_property_string_index(np, property, 0, &label) &&
            !strcmp(label, partname)) {
            memset(devpath, 0, len);
            snprintf(devpath, len, "/dev/mtdblock%d", i);
            return i;
        }
    }

    return -1;
}
#else
#define DTS_FLASH_CONFIG_PATH "/proc/device-tree/hcrtos/sfspi/spi_nor_flash/partitions"
int api_get_mtdblock_devpath(char *devpath, int len, const char *partname)
{
    uint32_t i = 1;
    uint32_t part_num = 0;
    const char label[32];
    char property[128];

    if (part_num == 0)
        part_num = api_dts_uint32_get(DTS_FLASH_CONFIG_PATH "/part-num");

    for (i = 1; i <= part_num; i++) {
        snprintf(property, sizeof(property), DTS_FLASH_CONFIG_PATH "/part%d-label", i);
        api_dts_string_get(property, label, sizeof(label));
        if (!strcmp(label, partname)){
            memset(devpath, 0, len);
            //snprintf(devpath, len, "/dev/mtdblock%d", i);
            snprintf(devpath, len, "/dev/mtd%d", i);

            printf("%s(), line:%d. devpath:%s\n", __func__, __LINE__, devpath);
            return i;
        }
    }
    printf("%s(), line:%d. cannot find mtd dev path!\n", __func__, __LINE__);
    return -1;
}

#endif


#ifdef __HCRTOS__
int api_romfs_resources_mount(void)
{
    static char m_mount = 0;
    char devpath[64];
    int ret = 0;

    if (m_mount)
    {
        printf("%s: resources alread mount!\n", __func__);
        return -1;
    }

    ret = api_get_mtdblock_devpath(devpath, sizeof(devpath), "eromfs");
    if (ret >= 0)
        ret = mount(devpath, "/hcdemo_files", "romfs", MS_RDONLY, NULL);

    if (ret >= 0)
    {
        printf("mount ok!\n");
        m_mount = 1;
    }

    return 0;
}
#endif

// set display area in projector
void api_set_display_area(dis_tv_mode_e ratio)//void set_aspect_ratio(dis_tv_mode_e ratio)
{
    struct dis_aspect_mode aspect = { 0 };
    struct dis_zoom dz;
    int fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0) {
        return;
    }
#if 0
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    if(ratio == DIS_TV_4_3)
        aspect.dis_mode = DIS_LETTERBOX; //DIS_PANSCAN
    else if(ratio == DIS_TV_16_9)
        aspect.dis_mode = DIS_PILLBOX; //DIS_PILLBOX
    else if(ratio == DIS_TV_AUTO){
        aspect.dis_mode = DIS_NORMAL_SCALE;
    }
    ioctl(fd , DIS_SET_ASPECT_MODE , &aspect);
#else
    dz.layer = DIS_LAYER_MAIN;
    dz.distype = DIS_TYPE_HD;
    int h = get_display_h();
    int v = get_display_v();

    printf("h1: %d, v1: %d\n", h, v);

    #ifdef SYS_ZOOM_SUPPORT
    int zoom_mode = projector_get_some_sys_param(P_SYS_ZOOM_DIS_MODE);
    #else
    int zoom_mode = DIS_TV_AUTO;
    #endif
    if(zoom_mode == DIS_TV_4_3)//ui为4:3缩放
    {
        if(ratio == DIS_TV_16_9)
        {    
            if(get_screen_is_vertical())
            {
                h = h*3/4;
            }
            else
            {
                v = v*3/4;                    
            }
        }
    }
    else if(zoom_mode == DIS_TV_16_9)
    {
        if(ratio == DIS_TV_4_3)
        {
            if(get_screen_is_vertical())
            {
                v = v*3/4;
                
            }else{
                h = h*3/4;
            }
        }
    }else if(zoom_mode == DIS_TV_AUTO){
        auto_mode_get_rect_by_ratio(&h, &v, ratio);
    }
    
    int x = get_display_x()+(get_display_h()-h)/2;
    int y = get_display_y()+(get_display_v()-v)/2;
    printf("h2: %d, v2: %d, x: %d, y: %d\n", h, v, x, y);
    dz.dst_area.x =  x;//get_display_x()+(get_display_h()-h)/2;
    dz.dst_area.y =  y;//get_display_y()+(get_display_v()-v)/2;
    dz.dst_area.w =  h;
    dz.dst_area.h =  v;
    dz.src_area.x = 0;
    dz.src_area.y = 0;
    dz.src_area.w = 1920;
    dz.src_area.h = 1080;
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = DIS_TV_16_9;
    aspect.dis_mode = DIS_NORMAL_SCALE;
    if(ratio == DIS_TV_AUTO){
        aspect.dis_mode = DIS_PILLBOX;
    }
    ioctl(fd , DIS_SET_ZOOM, &dz);
    usleep(30*1000);
    ioctl(fd , DIS_SET_ASPECT_MODE, &aspect);
#endif
    close(fd);
}

void api_set_display_zoom(int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h){
    struct dis_zoom dz = { 0 };
    dz.layer = DIS_LAYER_MAIN;
    dz.distype = DIS_TYPE_HD;
    dz.src_area.x = s_x;
    dz.src_area.y = s_y;
    dz.src_area.w = s_w;
    dz.src_area.h = s_h;
    dz.dst_area.x = d_x;
    dz.dst_area.y = d_y;
    dz.dst_area.w = d_w;
    dz.dst_area.h = d_h;

    int fd = -1;

    fd = open("/dev/dis", O_WRONLY);
    if(fd < 0){
        return;
    }
    ioctl(fd , DIS_SET_ZOOM , &dz);
    close(fd);
}
 
 /**
  * @description: 视频比例设置相关接口
  * dis_tv_mode_e ratio = DIS_TV_AUTO ,dis_mode_e dis_mode = xxx   视频全屏显示
  * dis_tv_mode_e ratio = DIS_TV_16_9 / DIS_TV_4_3 ,dis_mode_e dis_mode =DIS_PILLBOX 视频等比例显示
  * dis_tv_mode_e ratio = DIS_TV_16_9 / DIS_TV_4_3 ,dis_mode_e dis_mode =DIS_NORMAL_SCALE 视频全屏显示
  * @return {*}
  */
void api_set_display_aspect(dis_tv_mode_e ratio , dis_mode_e dis_mode)
{
    int ret = 0;
    dis_aspect_mode_t aspect = { 0 };

    printf("ratio: %d, dis_mode: %d\n" , ratio , dis_mode);
    int fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0) {
        return;
    }

    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio == DIS_TV_AUTO ? DIS_TV_16_9 : ratio;
    aspect.dis_mode = dis_mode;
    //take affect next frame to avoid influence current aspect
    aspect.active_mode = DIS_SCALE_ACTIVE_NEXTFRAME;
    ret = ioctl(fd, DIS_SET_ASPECT_MODE , &aspect);
    if( ret != 0 ){
        printf("%s:%d: err: DIS_SET_ASPECT_MODE failed\n", __func__, __LINE__);
        close(fd);
        return;
    }
    close(fd);
    return;
}

/**
 * @description: 获取的是图像在实际LCD屏幕显示的区域
 * @param {dis_screen_info_t *} dis_area
 * @return {*}
 * @author: Yanisin
 */
int api_get_display_area(dis_screen_info_t * dis_area)
{
    int fd;
    fd = open("/dev/dis", O_WRONLY);
    if(fd < 0){
        return -1 ;
    }
    dis_area->distype=DIS_TYPE_HD;
    ioctl(fd , DIS_GET_MP_AREA_INFO, dis_area);
    close(fd);
    return 0;
}
/**
 * @description: 获取的是实际输出LCD屏幕的信息，类似通过fb获取实际LCD信息
 * @param {dis_screen_info_t *} dis_area
 * @return {*}
 * @author: Yanisin
 */
int api_get_screen_info(dis_screen_info_t * dis_area)
{
    int fd;
    fd = open("/dev/dis", O_WRONLY);
    if(fd < 0){
        return -1 ;
    }
    dis_area->distype=DIS_TYPE_HD;
    ioctl(fd , DIS_GET_SCREEN_INFO, dis_area);
    close(fd);
    return 0;
}

int api_set_display_zoom2(dis_zoom_t* diszoom_param)
{
    struct dis_zoom dz = { 0 };
    memcpy(&dz,diszoom_param, sizeof(dis_zoom_t));
    dz.layer = DIS_LAYER_MAIN;
    dz.distype = DIS_TYPE_HD;
    dz.active_mode=DIS_SCALE_ACTIVE_IMMEDIATELY;
    int fd = -1;

    fd = open("/dev/dis", O_WRONLY);
    if(fd < 0){
        return -1;
    }
    ioctl(fd , DIS_SET_ZOOM , &dz);
    close(fd);
    return 0;

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

/*
* Get the mounted usb disk path. save usb dev path to dev_path: /media/hdd, /media/usb1, etc
* return the count of USB disk device path.
* dev_cnt: the count of dev_path[] for saving the device path.
*/
int api_usb_dev_path_get(char dev_path[][MAX_UDISK_DEV_LEN], int dev_cnt)
{
    int real_dev_cnt = 0;
    DIR *dirp = NULL;
    struct dirent *entry = NULL;

    if ((dirp = opendir(MOUNT_ROOT_DIR)) == NULL) {
        printf("%s(), line: %d. open dir:%s error!\n", __func__, __LINE__, MOUNT_ROOT_DIR);
        return 0;
    }

    do{
        entry = readdir(dirp);
        if (!entry)
            break;

        if(!strcmp(entry->d_name, ".") ||
            !strcmp(entry->d_name, "..")){
            //skip the upper dir
            continue;
        }

        if (strlen(entry->d_name) && entry->d_type == 4){ //dir
            sprintf(dev_path[real_dev_cnt], "%s/%s", MOUNT_ROOT_DIR, entry->d_name);
            printf("%s(), line: %d. found USB device: %s!\n", __func__, __LINE__,dev_path[real_dev_cnt]);        
            real_dev_cnt++;
        }
    }while(real_dev_cnt < dev_cnt);

    return real_dev_cnt;
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
#ifdef MIRROR_ES_DUMP_SUPPORT

//#define DUMP_FOLDER   "mirror_dump"
//Dump file to: /mirror_dump/aircast-X.h264, etc

static bool m_mirror_dump_enable = false;
void api_mirror_dump_enable_set(bool enable)
{
    m_mirror_dump_enable = enable;
}

bool api_mirror_dump_enable_get(char* folder)
{
    char usb_dev[1][MAX_UDISK_DEV_LEN];

    if (!m_mirror_dump_enable)
        return false;

    if (!api_usb_dev_path_get(usb_dev, 1)){
        printf("%s(), line: %d!. No dump file path!\n", __func__, __LINE__);
        return false;
    }
    
    snprintf(folder,MAX_UDISK_DEV_LEN,"%s", usb_dev[0]);
    
    printf("%s(), line: %d!. dump:%s\n", __func__, __LINE__, folder);
    return m_mirror_dump_enable;
}

#endif


static void *dev_ready_check(void *param)
{
    DIR *dirp = NULL;
    int usb_dev_found = 0;
    struct dirent *entry = NULL;
    uint32_t timeout = (uint32_t)param;
    char usb_path[512];
    int check_count = timeout/100;

    if (USB_STAT_MOUNT == mmp_get_usb_stat())
        return NULL;

    while(check_count --){
        if (NULL == dirp)
            dirp = opendir(MOUNT_ROOT_DIR);

        while (dirp) {
            entry = readdir(dirp);
            if (!entry)
                break;

            if(!strcmp(entry->d_name, ".") ||
                !strcmp(entry->d_name, "..")){
                //skip the upper dir
                continue;
            }

            if (strlen(entry->d_name) && entry->d_type == 4){ //dir
                snprintf(usb_path, sizeof(usb_path)-1, "%s/%s", MOUNT_ROOT_DIR, entry->d_name);
                usb_dev_found = 1;
                if(strstr(usb_path,"sd")|| strstr(usb_path,"hd")||strstr(usb_path,"usb")){
                    m_usb_state = USB_STAT_MOUNT;
                }else if(strstr(usb_path,"mmc")){
                    m_usb_state= SD_STAT_MOUNT;
                }

                if (!partition_glist_find((char*)usb_path))
                {
                    char * stroage_devname=strdup(usb_path);
                    /*need to free its memory after rsv message */ 
                    partition_info_msg_send(m_usb_state,(uint32_t)stroage_devname);
                    // partition_info_update(USB_STAT_MOUNT, usb_path);
                    printf("%s(), line: %d. found USB device: %s, 0x%x!\n", __func__, __LINE__,usb_path, stroage_devname);        
                    //break;
                }
            }
        }

        if (dirp){
            closedir(dirp);
            dirp = NULL;
        }
        api_sleep_ms(100);
    }
    if (0 == usb_dev_found)
        printf("%s(), line: %d. No USB device!\n", __func__, __LINE__);        

    if (dirp)
        closedir(dirp);

    return NULL;
}

//create a task to check if usb disk if ready.
//sometimes there is no mount message while start up in linux.
void api_usb_dev_check(void)
{
    pthread_t id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); //release task resource itself
    pthread_attr_setstacksize(&attr, 0x1000);
    pthread_create(&id, &attr,dev_ready_check, (void*)6000);
    pthread_attr_destroy(&attr);
}


/*call VIDSINK_DISABLE_IMG_EFFECT command to release
the picture effect buffer or software decoding buffer
for pic. so do not free effect buff when playing 
because the pic is using effect buffer */
int api_pic_effect_enable(bool enable)
{
    int vidsink_fd;

    vidsink_fd = open("/dev/vidsink", O_WRONLY);
    if (vidsink_fd < 0) {
        return -1;
    }

    if (enable) {
        ioctl(vidsink_fd, VIDSINK_ENABLE_IMG_EFFECT, 0);
    } else {
        ioctl(vidsink_fd, VIDSINK_DISABLE_IMG_EFFECT, 0);
    }

    if (vidsink_fd >= 0)
        close(vidsink_fd);

    return 0;
}

/*
 * @brief Get backlight level count
 *
 * @param level_count (2 ~ 101)
 * This parameter is only valid when the value of level_count > 1.
 *
 * @return 0 : success, other : fail
 */
int api_get_backlight_level_count(uint8_t *level_count)
{
    int ret = -1;
    hudi_handle backlight_hdl = NULL;
    hudi_backlight_info_t bl_dev_info = {0};

    hudi_backlight_open(&backlight_hdl);
    if (!backlight_hdl)
    {
        printf ("open backlight failed\n");
        return ret;
    }

    if (backlight_hdl)
    {
        hudi_backlight_info_get(backlight_hdl, &bl_dev_info);
        *level_count = bl_dev_info.levels_count;
        hudi_backlight_close(backlight_hdl);
        if (*level_count > 1)
            ret = 0;
    }

    return ret;
}

/*
 * @brief Get backlight default brightness level
 *
 * @param brightness_level (0 ~ 100)
 * When levels_count is greater than 1, default_brightness_level is valid.
 *
 * @return 0 : success, other : fail
 */
int api_get_backlight_default_brightness_level(uint8_t *brightness_level)
{
    int ret = -1;
    hudi_handle backlight_hdl = NULL;
    hudi_backlight_info_t bl_dev_info = {0};

    hudi_backlight_open(&backlight_hdl);
    if (!backlight_hdl)
    {
        printf ("open backlight failed\n");
        return ret;
    }

    if (backlight_hdl)
    {
        hudi_backlight_info_get(backlight_hdl, &bl_dev_info);
        *brightness_level = bl_dev_info.default_brightness_level;
        if (bl_dev_info.levels_count > 1)
            ret = 0;
        hudi_backlight_close(backlight_hdl);
    }

    return ret;
}

/*
 * @brief Set backlight brightness
 *
 * @param val (0 ~ 100)
 *
 * @return Is there a backlight device present
 *
 * @note 0： Backlight device present， -1： Backlight device not present
 */
int api_set_backlight_brightness(int val)
{
    int lvds_fd;
    int ret = -1;
    hudi_handle backlight_hdl = NULL;
    hudi_backlight_info_t bl_dev_info = {0};

    #ifdef BLUETOOTH_SUPPORT
    bluetooth_set_gpio_backlight(val);
    #endif

    hudi_backlight_open(&backlight_hdl);
    lvds_fd = open("/dev/lvds",O_RDWR);
    if ((backlight_hdl == NULL) && lvds_fd < 0)
    {
        printf ("open backlight failed\n");
        return ret;
    }

    if (backlight_hdl)
    {
        hudi_backlight_info_get(backlight_hdl, &bl_dev_info);
        bl_dev_info.brightness_value = val;
        hudi_backlight_info_set(backlight_hdl, &bl_dev_info);
        hudi_backlight_close(backlight_hdl);
        ret = 0;
    }

    if(lvds_fd)
    {
        if(ioctl(lvds_fd, LVDS_SET_PWM_BACKLIGHT,val) != -1)//lvds set pwm default
        {
            ret = 0;
        }

        if(ioctl(lvds_fd, LVDS_SET_GPIO_BACKLIGHT,val) != -1)//lvds gpio backlight close
        {
            ret = 0;
        }
        close(lvds_fd);
    }

    return ret;
}

#ifdef BACKLIGHT_MONITOR_SUPPORT
typedef enum{
    POWER_SOURCE_DEFAULT,
    POWER_SOURCE_DCPOWER,
    POWER_SOURCE_BATTERY,
}power_source_t;
static pinpad_e backlight_detection_pad = INVALID_VALUE_8;
static power_source_t power_source = POWER_SOURCE_DEFAULT;
static unsigned int pwm_battery_duty = 50;
static unsigned int pwm_power_duty = 100;
void api_pwm_backlight_monitor_init(void)
{
    int lcd_np=fdt_node_probe_by_path("/hcrtos/lcd");
    u32 tmp = INVALID_VALUE_8;

    if(lcd_np > 0)
    {
        fdt_get_property_u_32_index(lcd_np, "backlight-detection", 0, (u32 *)&tmp);
        backlight_detection_pad = (pinpad_e)tmp;
        fdt_get_property_u_32_index(lcd_np, "pwm-batter-duty", 0, (u32 *)&pwm_battery_duty);
        fdt_get_property_u_32_index(lcd_np, "pwm-power-duty", 0, (u32 *)&pwm_power_duty);
    }
    else
    {
        printf("%s %d error\n",__func__,__LINE__);
    }
    gpio_configure(backlight_detection_pad,GPIO_DIR_INPUT);
    printf("%s %d %d\n",__func__,__LINE__,backlight_detection_pad);
}

void api_pwm_backlight_monitor_loop(void)
{
    if(gpio_get_input(backlight_detection_pad))
    {
        if(power_source != POWER_SOURCE_DCPOWER)
        {
            printf("%s %d pwm duty %d\n",__func__,__LINE__,pwm_battery_duty);
            api_set_backlight_brightness(pwm_battery_duty);
            // ioctl(lvds_fd, LVDS_SET_PWM_BACKLIGHT,pwm_battery_duty);//lvds pwm set default value
            power_source = POWER_SOURCE_DCPOWER;
        }
    }
    else
    {
        if(power_source != POWER_SOURCE_BATTERY)
        {
            printf("%s %d pwm duty %d\n",__func__,__LINE__,pwm_power_duty);
            api_set_backlight_brightness(pwm_power_duty);
            // ioctl(lvds_fd, LVDS_SET_PWM_BACKLIGHT,pwm_power_duty);//lvds pwm set default value
            power_source = POWER_SOURCE_BATTERY;
        }
    }
}

void api_pwm_backlight_monitor_update_status(void)
{
    power_source = POWER_SOURCE_DEFAULT;
}
#endif

int api_set_partition_info(int index)
{
    if(partition_info!=NULL&&partition_info->count>0){
        partition_info->used_dev=glist_nth_data((glist*)partition_info->dev_list,index);
    }
    return 0;
}

char *api_get_partition_info_by_index(int index)
{
    if(partition_info!=NULL&&partition_info->count>0&& index<=partition_info->count){
        return glist_nth_data((glist*)partition_info->dev_list,index);
    }
    return NULL;
}

int api_partition_info_used_disk_set(char *disk)
{
    if(!disk)
    {
        return -1;
    }
    glist* node = partition_glist_find(disk);
    if(!node)
    {
        return -1;
    }
    partition_info->used_dev = (char*)node->data;
    return 0;
}

/**
 * @description: check used_dev had be hotplug(umount)
 * @return {*} true is unoumt 
 * @author: Yanisin
 */
bool api_check_partition_used_dev_ishotplug(void)
{
    char *dev_name=NULL;
    if(partition_info!=NULL&&partition_info->used_dev!=NULL){
        for(int i=0;i<partition_info->count;i++){
            dev_name=(char *)glist_nth_data((glist*)partition_info->dev_list,i);
            if(strcmp(dev_name,partition_info->used_dev)==0){
                return false;
            }
        }
        return true;
    }
    return false;
}

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


uint32_t api_sys_tick_get(void)
{
    extern uint32_t custom_tick_get(void);
    return custom_tick_get();
}
/**
 * @description: use this func to switch video and pic without black flash  
 * @return {*}
 * @author: Yanisin
 */
static bool is_backup=false;
int api_media_pic_backup(int time)
{
    int ret = -1;
    int distype = DIS_TYPE_HD;
    int fd;
    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0) {
        return -1;
    }
    usleep(time*1000);
    ret = ioctl(fd ,DIS_BACKUP_MP , distype);
    usleep(time*1000);
    close(fd);
    is_backup=true;
    return ret;
}
/*need to free backup buffer if you not need to play
driver support multiple call for this IO*/
int api_media_pic_backup_free(void)
{
    int distype = DIS_TYPE_HD;
    int fd;
    fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0) {
        return -1;
    }
    ioctl(fd ,DIS_FREE_BACKUP_MP , distype);
    close(fd);
    is_backup=false;
    return 0;
}
bool api_media_pic_is_backup(void)
{
    return is_backup;
}

#ifdef __HCRTOS__

#include <nuttx/drivers/ramdisk.h>
#include <nuttx/fs/fs.h>
#include <fsutils/mkfatfs.h>

static char * m_ram_path = "/dev/ram0";
//static char * m_mnt_path = "/mnt/ram0";
static char * m_mnt_path = "/tmp";

static uint8_t *m_ram_buffer = NULL;
static int _ramdisk_create(int minor, uint32_t nsectors, uint16_t sectsize, uint8_t rdflags)
{
    int ret = 0;

    /* Allocate the memory backing up the ramdisk from the kernel heap */

    m_ram_buffer = (uint8_t *)malloc(sectsize * nsectors);
    if (m_ram_buffer == NULL)
    {
        printf("ERROR: mmz_malloc() failed: %d\n", ret);
        return -ENOMEM;
    }

    memset(m_ram_buffer, 0, sectsize * nsectors);
    ret = ramdisk_register(minor, m_ram_buffer, nsectors, sectsize, rdflags);
    if (ret < 0)
    {
        printf("ERROR: ramdisk_register() failed: %d\n", ret);
        free(m_ram_buffer);
        m_ram_buffer = NULL;
    }

    return ret;
}

//Create a Ramdisk with the size in byte
int api_ramdisk_open(uint32_t size)
{
    struct fat_format_s fmt = FAT_FORMAT_INITIALIZER;
    int ret = 0;
    uint32_t nsectors;
    uint16_t sectsize = 512;

    if (m_ram_buffer)
        return 0;

    nsectors = size/sectsize;

    do{
        ret = _ramdisk_create(0,nsectors,sectsize,RDFLAG_WRENABLED | RDFLAG_FUNLINK);
        if (ret < 0 )
        {
            printf("%s(), error mkrd\n", __func__);
            if (m_ram_buffer)
                free(m_ram_buffer);
            m_ram_buffer = NULL;
            break;
        }

        ret = mkfatfs(m_ram_path, &fmt);
        if (ret < 0)
        {
            printf("%s(), error mkfatfs\n", __func__);
            break;
        }

        ret = mount(m_ram_path, m_mnt_path, "vfat", 0, NULL);
        if (ret < 0)
        {
            printf("%s(), error mount ramdisk\n", __func__);
            break;
        }    

        printf("%s(), mount %s to %s OK!\n", __func__, m_ram_path, m_mnt_path);

    }while(0);

    if (ret)
    {
        unlink(m_ram_path);
        m_ram_path = NULL;
    }
    return ret;
}

//Get the mount path of ramddisl
char *api_ramdisk_path_get()
{
    if (m_ram_buffer)
        return m_mnt_path;
    else
        return NULL;
}

//Close the ramdisk
int api_ramdisk_close()
{
    int ret = 0;
    ret = umount(m_mnt_path);
    if (ret < 0)
        printf("error umount ramdisk\n");

    //unlink will free m_ram_buffer
    unlink(m_ram_path);
    // if (m_ram_buffer)
    //     free(m_ram_buffer);

    m_ram_buffer = NULL;
}

#endif


bool api_group_is_valid(lv_group_t *g){
    lv_group_t* g_;
    _LV_LL_READ(&LV_GC_ROOT(_lv_group_ll), g_){
        if(g_ == g){
            return true;
        }
    }
    return false;
}

bool api_group_contain_obj(lv_group_t *g, lv_obj_t* obj){
    if(g == NULL || obj == NULL){
        return false;
    }
    lv_obj_t** obj_;
    _LV_LL_READ(&g->obj_ll, obj_){
        if((*obj_) == obj){
            return true;
        }
    }
    return false;
}

lv_group_t* api_get_group_by_scr(lv_obj_t* scr){
    int i = 0;
    for(lv_obj_t* child = lv_obj_get_child(scr, i); i < lv_obj_get_child_cnt(scr); i++){
        if(child->spec_attr && child->spec_attr->group_p){
            return child->spec_attr->group_p;
        }else if(lv_obj_get_child_cnt(child)>0){
            lv_group_t* child_g = api_get_group_by_scr(child);
            if(child_g){
                return child_g;
            }
        }
    }
    return NULL;
}
/**
 * @description:  a var to check if the storage device info is available
 * @return {*}  is_device_info, true->available false -> not available
 * @author: Yanisin
 */
static bool is_device_info=true;
bool api_storage_devinfo_state_get(void)
{
    return is_device_info;
}
void api_storage_devinfo_state_set(bool state)
{
    is_device_info=state;
}

 /**
 * @description: check if the storage device info is available,if not 
 * send a (umount)message to lvgl pthread.
 * @return {*}
 * @author: Yanisin
 */
int api_storage_devinfo_check(char* device,char* filename)
{
    int fs_ret=0;
    control_msg_t ctl_msg={0};
    int ret=0;
    //struct stat file_stat={0};
    //fs_ret=stat(filename,&file_stat);
    //it will stat error for large file so use access to check if available
    fs_ret = access(filename,F_OK);
    if(fs_ret==-1){
        ctl_msg.msg_type=MSG_TYPE_USB_UNMOUNT;
        ctl_msg.msg_code=(uint32_t)device;
        api_storage_devinfo_state_set(false);
        api_control_send_msg(&ctl_msg);
        printf(">>!%s ,%d\n",__func__,__LINE__);
        ret=-1;
    }
    return ret;
}


 /**
 * @description: Get system clock time.
 * @return: millisecond(ms)
 */
int64_t api_get_sys_clock_time (void) //get ms
{
    struct timeval time_t;
    gettimeofday(&time_t, NULL);
    return time_t.tv_sec * 1000 + time_t.tv_usec / 1000;
}


static int64_t m_clock_first = 0;
static int64_t m_clock_last = 0;

 /**
 * @description: Get system clock time for calculating time-consuming, start to record time
 * @return: microsecond(us)
 */
void api_sys_clock_time_check_start(void)
{
    struct timeval time_t;
    gettimeofday(&time_t, NULL);
    m_clock_first = m_clock_last = time_t.tv_sec * 1000000 + time_t.tv_usec;
}

 /**
 * @description: Get system clock time for calculating time-consuming, get the time-consuming
 *     from start.
 * @param out:  last_clock, output time-consuming after last calling.    
 * @return: microsecond(us), output time-consuming from start.  
 */
int64_t api_sys_clock_time_check_get(int64_t *last_clock)
{
    int64_t clock_time = 0;
    struct timeval time_t;
    gettimeofday(&time_t, NULL);
    clock_time = time_t.tv_sec * 1000000 + time_t.tv_usec;

    if (last_clock)
        *last_clock = clock_time - m_clock_last;
    m_clock_last = clock_time;

    return (clock_time - m_clock_first);
}

int api_set_next_flip_mode(void)
{
    pthread_mutex_lock(&m_set_filp_mutex);
    
    if (m_set_flip_en)
    {
        set_next_flip_mode();
        projector_sys_param_save();
        api_logo_reshow();
    }    
    
    pthread_mutex_unlock(&m_set_filp_mutex);

    return 0;
}

int api_set_flip_mode_enable(int enable)
{
    pthread_mutex_lock(&m_set_filp_mutex);
    m_set_flip_en = enable;
    pthread_mutex_unlock(&m_set_filp_mutex);
    
    return 0;
}


int api_set_wifi_pm_state(int state)
{
    pthread_mutex_lock(&m_wifi_pm_mutex);
    m_wifi_pm_state = state;
    pthread_mutex_unlock(&m_wifi_pm_mutex);
    
    return 0;
}

int api_get_wifi_pm_state(void)
{
    int state = WIFI_PM_STATE_NONE;
    
    pthread_mutex_lock(&m_wifi_pm_mutex);
    state = m_wifi_pm_state;
    pthread_mutex_unlock(&m_wifi_pm_mutex);
    
    return state;
}

int api_get_wifi_pm_mode(void)
{
    static int pm_mode = 0;
    
#ifdef WIFI_PM_SUPPORT
    static int get_pm_mode_inited = 0;
    if (get_pm_mode_inited)
    {
        return pm_mode;
    }

    get_pm_mode_inited = 1;
    
  #ifdef __HCRTOS__     
    int np;
    const char *st;
    np = fdt_node_probe_by_path("/hcrtos/wifi_pw_enable");
    if(np>=0)
    {
        fdt_get_property_string_index(np, "status", 0, &st);
        if (!strcmp(st, "okay"))
        {   
            if (fdt_get_property_u_32_index(np, "pm_mode", 0, &pm_mode) == 0)
            {
                printf("get wifi pm mode success [%d].\n", pm_mode);
            }
            else
            {
                pm_mode = 0;
            }
        }
    }
  #endif    
#endif

    return pm_mode;
}

int api_wifi_pm_plugin_handle(void)
{
#ifdef WIFI_PM_SUPPORT    
    return network_wifi_pm_plugin_handle();
#else
    return 0;
#endif    
}

int api_wifi_pm_open(void)
{
#ifdef WIFI_PM_SUPPORT    
    return network_wifi_pm_open();
#else
    return 0;
#endif
}

int api_wifi_pm_stop(void)
{
#ifdef WIFI_PM_SUPPORT 
    return network_wifi_pm_stop();
#else
    return 0;
#endif
}

void api_swap_value(void *a, void *b, int len)
{
    uint8_t* v1 = (uint8_t*)a;
    uint8_t* v2 = (uint8_t*)b;
    for(int i = 0; i < len; i++,v1++, v2++){
        *v1 ^= *v2;
        *v2 ^= *v1;
        *v1 ^= *v2;
    }
}


void api_is_upgrade_set(bool upgrade)
{
    m_is_upgrade = upgrade;
}

bool api_is_upgrade_get(void)
{
    return m_is_upgrade;
}


void api_enter_flash_rw(void)
{
//    printf("enter %s()!\n", __func__);
    pthread_mutex_lock(&m_flash_rw_mutex);  
}

void api_leave_flash_rw(void)
{
//    printf("enter %s()!\n", __func__);
    pthread_mutex_unlock(&m_flash_rw_mutex);  
}


int api_wifi_get_bus_port(void)
{
    int bus_port = -1;
#if defined(AIRP2P_SUPPORT) && defined(__HCRTOS__)
    int res;
    libusb_device **devs;
    struct libusb_device_descriptor desc;
    int cnt;
    int ret;
    char buf[32] = {0};
    
    res = libusb_init(NULL);
    libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

    if (res != 0)
    {
        printf("%s libusb_init failed: %s\n", __func__, libusb_error_name(res));
        return -1;
    }

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt > 0)
    {
        for (int i = 0; i < cnt; i++)
        {
            ret = libusb_get_device_descriptor(devs[i], &desc);
            if (ret != LIBUSB_SUCCESS)
            {
                printf("Error getting enumerate device descriptor\n");
                continue;
            }

            memset(buf, 0, sizeof(buf));
            snprintf(buf, sizeof(buf), "v%04xp%04x", desc.idVendor, desc.idProduct);

            if (!strcasecmp(g_hotplug_wifi_model.desc, buf))
            {
                bus_port = libusb_get_bus_number(devs[i]);
                if (libusb_get_bus_number(devs[i]) == 1)
                {
                    bus_port = 0;
                }
                else if (libusb_get_bus_number(devs[i]) == 2)
                {
                    bus_port = 1;
                }

                printf("get wifi bus port: %d\n", bus_port);
                break;
            }
        }

        libusb_free_device_list(devs, 1);
    }

    libusb_exit(NULL);
#endif    
    return bus_port;
}

#ifdef MULTI_OS_SUPPORT
/**
 * @brief :Detect icube,if exist,enter multi_os
 * @param :void;
 * @return :void
*/
void find_icube_enter_multi_os(void)
{
    int ret=-1;
    DIR * dir;
    struct dirent * ptr;
    char *path = NULL;

    path = (char *)malloc(512);
    if (path == NULL){
        printf("path malloc failed\n");
        return ;
    }
    memset(path, 0, sizeof(char)*512);

    dir = opendir(MOUNT_ROOT_DIR);
    if (dir != NULL) {
        while((ptr = readdir(dir)) != NULL)
        {
            sprintf(path, "/%s/%s/cubegm/icube", MOUNT_ROOT_DIR, ptr->d_name);
            ret = access(path,R_OK);
            if (ret >= 0){
                printf("icube path: %s\n",path);
                break;
            }
        }
        closedir(dir);
        free(path);

        if (ret >= 0) {
            store_avparam();
            api_system_reboot();
        }
    }
}
#endif

//if support picture by picture mode(multi videos) by DTS
bool api_video_pbp_get_support(void)
{
#if 0    
    static u32 dual_de_enable = INVALID_VALUE_32;

    if (INVALID_VALUE_32 != dual_de_enable)
        return dual_de_enable ? true : false;

#ifdef __HCRTOS__
    int np = -1;
    np = fdt_get_node_offset_by_path("/hcrtos/de-engine");
    if (np < 0)
        return false;

    if (fdt_get_property_u_32_index(np, "dual-de-enable", 0, &dual_de_enable))
        return false;
#else
    #define DTS_DE2K_CONFIG_PATH "/proc/device-tree/hcrtos/de-engine"
    if (-1 == api_dts_uint32_get(DTS_DE2K_CONFIG_PATH "/dual-de-enable")){
        return false;
    }
#endif    

    return dual_de_enable ? true : false;
#else
    #ifdef VIDEO_PBP_MODE_SUPPORT
        return true;
    #else
        return false;
    #endif

#endif

}

//get the display engine type according video src
int api_video_pbp_get_dis_type(int src_type)
{
    (void)src_type;
    return APP_DIS_TYPE_HD;
}

//get the display layer according video src
int api_video_pbp_get_dis_layer(int src_type)
{
    (void)src_type;
    return APP_DIS_LAYER_MAIN;
}

void api_video_pbp_set_dis_type(int src_type)
{
    (void)src_type;
}
void api_video_pbp_set_dis_layer(int src_type)
{
    (void)src_type;
}

void api_fb_set_zoom_arg(fb_zoom_t *fb_zoom)
{
    memcpy(&m_fb_zoom, fb_zoom, sizeof(fb_zoom_t));
    m_fb_zoom_set = 1;

    printf("%s(), L:%d T:%d, %d %d %d %d\n", 
        __func__, m_fb_zoom.left, m_fb_zoom.top, m_fb_zoom.h_div,
        m_fb_zoom.v_div, m_fb_zoom.h_mul, m_fb_zoom.v_mul);
}

int api_fb_get_zoom_arg(fb_zoom_t *fb_zoom)
{
    if (!m_fb_zoom_set)
        return -1;

    memcpy(fb_zoom, &m_fb_zoom, sizeof(fb_zoom_t));   

    printf("%s(), L:%d T:%d, %d %d %d %d\n", 
        __func__, fb_zoom->left, fb_zoom->top, fb_zoom->h_div,
        fb_zoom->v_div, fb_zoom->h_mul, fb_zoom->v_mul);
    return 0;
}

void api_set_flip_arg(int rotate, int hor_flip, int ver_flip)
{
    m_fb_rotate = rotate;
    m_fb_hor_flip = hor_flip;
    m_fb_ver_flip = ver_flip;
    m_filp_set = 1;

    printf("%s(), rotate: %d, h_f: %d, v_f: %d\n", __func__,
        m_fb_rotate, m_fb_hor_flip, m_fb_ver_flip);
}

int api_get_flip_arg(int *rotate, int *hor_flip, int *ver_flip)
{
    if (!m_filp_set)
        return -1;

    *rotate = m_fb_rotate;
    *hor_flip = m_fb_hor_flip;
    *ver_flip = m_fb_ver_flip;

    printf("%s(), rotate: %d, h_f: %d, v_f: %d\n", __func__,
        *rotate, *hor_flip, *ver_flip);

    return 0;
}

#ifdef APPMANAGER_SUPPORT
#define APP_IPC_MSG


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
* cmd: upgrade -k hcprojector -s upgradeapp -u /tmp/HCFOTA.bin -z 0,0,1280,720,1920,1080 -f 180,0,0
*     -k: kill application
*     -u: upgrade file
*     -z: fb zoom
*     -f: rotate, hor_flip, ver_flip
*/
//cmd: upgrade -k hcprojector -s upgradeapp -u /tmp/HCFOTA.bin -z 0,0,1280,720,1920,1080 -f 180,0,0
int api_app_command_upgrade(const char *url)
{
    fb_zoom_t fb_zoom;
    int rotate;
    int hor_flip;
    int ver_flip;

    char upgrade_cmd[256];

    if (!api_fb_get_zoom_arg(&fb_zoom) && 
        !api_get_flip_arg(&rotate, &hor_flip, &ver_flip)){
        //set fb zoom & file args for upgrade application.
        //cmd: upgrade -k hcprojector -s ./upgradeapp -u /tmp/HCFOTA.bin -z 0,0,1280,720,1920,1080 -f 180,0,0
        snprintf(upgrade_cmd, sizeof(upgrade_cmd), 
            "upgrade -k %s -s %s -u %s -z %hd,%hd,%hd,%hd,%hd,%hd -f %d,%d,%d",
            HC_APP_NAME, HC_APP_UPGRADE, url, fb_zoom.left, fb_zoom.top, fb_zoom.h_div, fb_zoom.v_div, 
            fb_zoom.h_mul, fb_zoom.v_mul, rotate, hor_flip, ver_flip);

    } else {
        //use full screen for for upgrade application.
        //cmd: upgrade -k hcprojector -s ./upgradeapp
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
{}

void api_upgrade_buffer_free(void *ptr)
{
    free(ptr);
}


#endif //endif of APPMANAGER_SUPPORT

int api_spdif_bypass_support_get(void)
{

#ifdef BLUETOOTH_SUPPORT
    //bluetooth can not decode spdif DTS, so do not support spdif out bypass.
    return 0;
#endif

#ifdef __HCRTOS__
    int np;
    np = fdt_node_probe_by_path("/hcrtos/spdif-out");
    if(np>=0){
        return 1;
    }
#else
    char status[16] = {0};
    api_dts_string_get("/proc/device-tree/hcrtos/spdif-out/status", status, sizeof(status));
    if(!strcmp(status, "okay")){
        return 1;
    }
#endif
    return 0;
}
