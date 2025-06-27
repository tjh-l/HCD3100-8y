/*
hotplug_mgr.c: to manage the hotplug device, such as usb wifi, usb disk etc
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#ifdef __linux__
    #include <sys/epoll.h>
#else
    #include <kernel/notify.h>
    #include <linux/notifier.h>
    #include <hcuapi/sys-blocking-notify.h>
#endif
#include <pthread.h>
#include <netdb.h>
#include <hcuapi/common.h>
#include <hcuapi/kumsgq.h>
#include "com_api.h"
#include "data_mgr.h"
#include "cast_api.h"
#include "tv_sys.h"
#include <linux/usb.h>

static USB_STATE m_usb_state = USB_STAT_INVALID;

static int hotplug_usb_notify(struct notifier_block *self, unsigned long action, void *dev)
{
    switch (action)
    {
    case USB_MSC_NOTIFY_MOUNT:
        printf("USB Plug In!\n");
        m_usb_state = USB_STAT_MOUNT;
        break;
    case USB_MSC_NOTIFY_UMOUNT:
        printf("USB Plug Out!\n");
        m_usb_state= USB_STAT_UNMOUNT;
        break;
    case USB_MSC_NOTIFY_MOUNT_FAIL:
        printf("USB Plug mount fail!\n");
        m_usb_state = USB_STAT_MOUNT_FAIL;
        break;
    case USB_MSC_NOTIFY_UMOUNT_FAIL:
        printf("USB Plug unmount fail!\n");
        m_usb_state = USB_STAT_UNMOUNT_FAIL;
        break;
    default:

        break;
    }

    return NOTIFY_OK;
}

static void _hotplug_hdmi_tx_tv_sys_set(void)
{
    int ap_tv_sys;
    int ap_tv_sys_ret;
    int last_tv_sys;

    last_tv_sys = data_mgr_de_tv_sys_get();
    ap_tv_sys = data_mgr_app_tv_sys_get();
    ap_tv_sys_ret = tv_sys_app_auto_set(ap_tv_sys, 2000);
    if (ap_tv_sys_ret >= 0)
    {
        data_mgr_de_tv_sys_set(ap_tv_sys_ret);
        data_mgr_save();
    }
}

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

static struct notifier_block hotplug_usb_nb =
{
    .notifier_call = hotplug_usb_notify,
};

void hotplug_init(void)
{
    sys_register_notify(&hotplug_usb_nb);
    hotplug_hdmi_tx_init();
}

int hotplug_usb_get(void)
{
    return m_usb_state;
}

