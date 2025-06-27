#ifdef CVBSIN_SUPPORT

#include <hcuapi/kshm.h>
#include <hcuapi/hdmi_rx.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/dis.h>
#include <hcuapi/viddec.h>
#include <hcuapi/fb.h>

#ifdef __HCRTOS__
#include <kernel/lib/console.h>
#include <nuttx/wqueue.h>
#else
//#include <console.h>
#endif

#define CVBS_SWITCH_HDMI_STATUS_PLUGIN 1
#define CVBS_SWITCH_HDMI_STATUS_PLUGOUT 0
#define CVBS_RX_STATUS_TRAINNING_FINISH 2

struct cvbs_info{
    int fd;

#ifdef __HCRTOS__
    struct work_notifier_s notify_plugin;
    struct work_notifier_s notify_plugout;
    struct work_notifier_s notify_edid;
    struct work_notifier_s notify_trainning_finish;
	int in_ntkey;
	int out_ntkey;
    int finish_ntkey;
#endif    

    char plug_status;
    char enable;
};

void cvbs_set_display_rect(struct vdec_dis_rect* rect);
int cvbs_rx_start(void);
int cvbs_rx_stop(void);
bool cvbs_is_playing(void);
void cvbs_rx_training();
void cvbs_rx_set_gain(int v);
#endif