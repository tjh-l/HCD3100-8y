#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <hcuapi/dis.h>
#include <kernel/io.h>
#include <kernel/notify.h>
#include <linux/notifier.h>
#include <hcuapi/sys-blocking-notify.h>
#include <freertos/FreeRTOS.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/fb.h>
#include <sys/mount.h>
#include <ffplayer.h>
#include <glist.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/wqueue.h>
#include <hcuapi/hdmi_tx.h>
#include "com_api.h"
#include "os_api.h"

static uint32_t m_control_msg_id = INVALID_ID;
static bool m_ffplay_init = false;
static int m_usb_state = USB_STAT_INVALID;
static partition_info_t *partition_info = NULL;


void cast_api_set_aspect_mode(dis_tv_mode_e ratio,
                              dis_mode_e dis_mode,
                              dis_scale_avtive_mode_e active_mode)
{
    int fd = open("/dev/dis", O_RDWR);
    if (fd < 0)
        return;
    dis_aspect_mode_t aspect = {0};
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = ratio;
    aspect.dis_mode = dis_mode;
    aspect.active_mode = active_mode;
    ioctl(fd, DIS_SET_ASPECT_MODE, &aspect);
    close(fd);
}

static glist *partition_glist_find(char *string_to_find)
{
    glist *glist_found = NULL;
    glist *temp_glist = (glist *)partition_info->dev_list;
    while (temp_glist)
    {
        if (temp_glist != NULL && temp_glist->data != NULL)
        {
            if (strcmp(string_to_find, temp_glist->data) == 0)
            {
                glist_found = temp_glist;
                break;
            }
            else
            {
                temp_glist = temp_glist->next;
            }
        }
    }

    return glist_found;
}

static int partition_delnode_form_glist(char *string_to_find)
{
    int ret = 0;
    glist *glist_del = NULL;

    glist_del = partition_glist_find(string_to_find);
    if (glist_del)
    {
        if (glist_del->data)
        {
            free(glist_del->data);
            glist_del->data = NULL; //free data
        }
        partition_info->dev_list = glist_delete_link(partition_info->dev_list, glist_del);
        glist_del = NULL;
        ret = 0;
    }
    else
    {
        ret = -1;
    }
    return ret;
}
int partition_info_update(int usb_state, void *dev)
{
    if (usb_state == MSG_TYPE_USB_MOUNT || usb_state == MSG_TYPE_SD_MOUNT)
    {
        char *dev_name = NULL;
        if (partition_glist_find((char *)dev))
            return 0;

        partition_info->count++;
        dev_name = (char *)dev;
        partition_info->dev_list = glist_append((glist *)partition_info->dev_list, dev_name);
    }
    else if (usb_state == MSG_TYPE_USB_UNMOUNT || usb_state == MSG_TYPE_SD_UNMOUNT || usb_state == MSG_TYPE_SD_UNMOUNT_FAIL || usb_state == MSG_TYPE_USB_UNMOUNT_FAIL)
    {
        if (!partition_delnode_form_glist(dev))
        {
            partition_info->count--;
            if (partition_info->count < 0)
                partition_info->count = 0;
        }
    }
    partition_info->m_storage_state = usb_state;
    return 0;
}

static int partition_info_init()
{
    if (partition_info == NULL)
    {
        partition_info = (partition_info_t *)malloc(sizeof(partition_info_t));
        memset(partition_info, 0, sizeof(partition_info_t));
    }
    return 0;
}

void *mmp_get_partition_info()
{
    return partition_info;
}
static int partition_info_msg_send(int type, uint32_t code)
{
    control_msg_t msg = {0};
    memset(&msg, 0, sizeof(control_msg_t));
    switch (type)
    {
        case USB_STAT_MOUNT:
            msg.msg_type = MSG_TYPE_USB_MOUNT;
            break;
        case USB_STAT_UNMOUNT:
            msg.msg_type = MSG_TYPE_USB_UNMOUNT;
            break;
        case USB_STAT_MOUNT_FAIL:
            msg.msg_type = MSG_TYPE_USB_MOUNT_FAIL;
            break;
        case USB_STAT_UNMOUNT_FAIL:
            msg.msg_type = MSG_TYPE_USB_UNMOUNT_FAIL;
            break;
        case SD_STAT_MOUNT:
            msg.msg_type = MSG_TYPE_SD_MOUNT;
            break;
        case SD_STAT_UNMOUNT:
            msg.msg_type = MSG_TYPE_SD_UNMOUNT;
            break;
        case SD_STAT_MOUNT_FAIL:
            msg.msg_type = MSG_TYPE_SD_MOUNT_FAIL;
            break;
        case SD_STAT_UNMOUNT_FAIL:
            msg.msg_type = MSG_TYPE_SD_UNMOUNT_FAIL;
            break;
    }
    msg.msg_code = code;
    api_control_send_msg(&msg);
}


static int usbd_notify(struct notifier_block *self,
                       unsigned long action, void *dev)
{
    if (!dev)
        return NOTIFY_OK;

    switch (action)
    {
        case USB_MSC_NOTIFY_MOUNT:
            if (strstr(dev, "sd"))
            {
                m_usb_state = USB_STAT_MOUNT;
            }
            else if (strstr(dev, "mmc"))
            {
                m_usb_state = SD_STAT_MOUNT;
            }
            if (dev)
                printf("USB_STAT_MOUNT: %s\n", (char *)dev);
            break;
        case USB_MSC_NOTIFY_UMOUNT:
            if (strstr(dev, "sd"))
            {
                m_usb_state = USB_STAT_UNMOUNT;
            }
            else if (strstr(dev, "mmc"))
            {
                m_usb_state = SD_STAT_UNMOUNT;
            }
            break;
        case USB_MSC_NOTIFY_MOUNT_FAIL:
            if (strstr(dev, "sd"))
            {
                m_usb_state = USB_STAT_MOUNT_FAIL;
            }
            else if (strstr(dev, "mmc"))
            {
                m_usb_state = SD_STAT_MOUNT_FAIL;
            }
            break;
        case USB_MSC_NOTIFY_UMOUNT_FAIL:
            if (strstr(dev, "sd"))
            {
                m_usb_state = USB_STAT_UNMOUNT_FAIL;
            }
            else if (strstr(dev, "mmc"))
            {
                m_usb_state = SD_STAT_UNMOUNT_FAIL;
            }
            break;
        default:
            return NOTIFY_OK;
            break;
    }
    int dev_len = strlen(dev) + 16;
    char *stroage_devname = malloc(dev_len);
    //dev: sda1, etc, not like /media/hdd in linux
    snprintf(stroage_devname, dev_len - 1, "%s/%s", MOUNT_ROOT_DIR, (char *)dev);

    /*need to free its memory after rsv message */
    partition_info_msg_send(m_usb_state, (uint32_t)stroage_devname);

    return NOTIFY_OK;
}

static struct notifier_block usb_switch_nb =
{
    .notifier_call = usbd_notify,
};

static void _notifier_hdmi_tx_plugin(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);

    return ;
}

static void _notifier_hdmi_tx_plugout(void *arg, unsigned long param)
{
    printf("%s:%d: \n", __func__, __LINE__);
}

static void _notifier_hdmi_tx_ready(void *arg, unsigned long param)
{

    struct hdmi_edidinfo *edid = (struct hdmi_edidinfo *)param;
    printf("%s(), best_tvsys: %d\n", __func__, edid->best_tvsys);
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

    notify_plugin.evtype = HDMI_TX_NOTIFY_CONNECT;
    notify_plugin.qid = LPWORK;
    notify_plugin.remote = false;
    notify_plugin.oneshot = false;
    notify_plugin.qualifier = NULL;
    notify_plugin.arg = NULL;
    notify_plugin.worker2 = _notifier_hdmi_tx_plugin;
    work_notifier_setup(&notify_plugin);

    notify_plugout.evtype = HDMI_TX_NOTIFY_DISCONNECT;
    notify_plugout.qid = LPWORK;
    notify_plugout.remote = false;
    notify_plugout.oneshot = false;
    notify_plugout.qualifier = NULL;
    notify_plugout.arg = NULL;
    notify_plugout.worker2 = _notifier_hdmi_tx_plugout;
    work_notifier_setup(&notify_plugout);

    notify_edid.evtype = HDMI_TX_NOTIFY_EDIDREADY;
    notify_edid.qid = LPWORK;
    notify_edid.remote = false;
    notify_edid.oneshot = false;
    notify_edid.qualifier = NULL;
    notify_edid.arg = (void *)(&notify_edid);
    notify_edid.worker2 = _notifier_hdmi_tx_ready;
    work_notifier_setup(& notify_edid);

    return 0;
}


static int hotplug_init()
{
    pthread_t pid;
    hotplug_hdmi_tx_init();
    return 0;
}


void api_system_pre_init(void)
{
    partition_info_init();
    //usb mount/umount notify must registed in advance,
    //otherwise when bootup, App may not receive the notifications from driver.
    sys_register_notify(&usb_switch_nb);
}

int api_system_init(void)
{
    api_romfs_resources_mount();
    hotplug_init();
    cast_api_set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX, DIS_SCALE_ACTIVE_IMMEDIATELY);

    return 0;
}

// turn on/off the video frame output
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
    winon.layer = DIS_LAYER_MAIN;
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

void api_control_clear_msg(msg_type_t msg_type)
{
    control_msg_t msg_buffer;
    while (1)
    {
        if (api_control_receive_msg(&msg_buffer))
        {
            break;
        }
    }
    return;
}

void app_ffplay_init(void)
{
    if (m_ffplay_init) return;

    hcplayer_init(1);
    m_ffplay_init = true;
}

void api_sleep_ms(uint32_t ms)
{
    usleep(ms * 1000);
}

static glist *m_screen_list = NULL;
void api_screen_regist_ctrl_handle(screen_entry_t *entry)
{
    screen_entry_t *entry_tmp;
    glist *glist_tmp = NULL;

    glist_tmp = m_screen_list;
    while (glist_tmp)
    {
        entry_tmp = glist_tmp->data;
        if (entry_tmp->screen == entry->screen)
        {
            entry_tmp->control = entry->control;
            return;
        }
        glist_tmp = glist_tmp->next;
    }

    entry_tmp = (screen_entry_t *)malloc(sizeof(screen_entry_t));
    memcpy(entry_tmp, entry, sizeof(screen_entry_t));
    m_screen_list = glist_append(m_screen_list, (void *)entry_tmp);
}

screen_ctrl api_screen_get_ctrl(void *screen)
{
    screen_entry_t *entry_tmp;
    glist *glist_tmp = NULL;

    glist_tmp = m_screen_list;
    while (glist_tmp)
    {
        entry_tmp = glist_tmp->data;
        if (entry_tmp->screen == screen)
        {
            return entry_tmp->control;
        }
        glist_tmp = glist_tmp->next;
    }

    return NULL;
}


/* Load HDCP key*/
int api_get_mtdblock_devpath(char *devpath, int len, const char *partname)
{
    int np = -1;
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


int api_set_partition_info(int index)
{
    if (partition_info != NULL && partition_info->count > 0)
    {
        partition_info->used_dev = glist_nth_data((glist *)partition_info->dev_list, index);
    }
    return 0;
}

char *api_get_partition_info_by_index(int index)
{
    if (partition_info != NULL && partition_info->count > 0 && index <= partition_info->count)
    {
        return glist_nth_data((glist *)partition_info->dev_list, index);
    }
    return NULL;
}

/**
 * @description: check used_dev had be hotplug(umount)
 * @return {*} true is unoumt
 * @author: Yanisin
 */
bool api_check_partition_used_dev_ishotplug(void)
{
    char *dev_name = NULL;
    if (partition_info != NULL && partition_info->used_dev != NULL)
    {
        for (int i = 0; i < partition_info->count; i++)
        {
            dev_name = (char *)glist_nth_data((glist *)partition_info->dev_list, i);
            if (strcmp(dev_name, partition_info->used_dev) == 0)
            {
                return false;
            }
        }
        return true ;
    }
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
static bool is_backup = false;
int api_media_pic_backup(void)
{
    int distype = DIS_TYPE_HD;
    int fd;
    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0)
    {
        return -1;
    }
    usleep(60 * 1000);
    ioctl(fd, DIS_BACKUP_MP, distype);
    usleep(60 * 1000);
    close(fd);
    is_backup = true;
    return 0;
}

/*need to free backup buffer if you not need to play
driver support multiple call for this IO*/
int api_media_pic_backup_free(void)
{
    int distype = DIS_TYPE_HD;
    int fd;
    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0)
    {
        return -1;
    }
    ioctl(fd, DIS_FREE_BACKUP_MP, distype);
    close(fd);
    is_backup = false;
    return 0;
}
bool api_media_pic_is_backup(void)
{
    return is_backup;
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
