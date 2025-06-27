
#ifndef _HDMI_RX_H_
#define _HDMI_RX_H_

#include <hcuapi/kshm.h>
#include <hcuapi/hdmi_rx.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/dis.h>
#include <hcuapi/viddec.h>
#include <hcuapi/fb.h>
#include <hudi/hudi_cec.h>

#ifdef __HCRTOS__
#include <kernel/lib/console.h>
#include <nuttx/wqueue.h>
#endif


#define HDMI_RX_STATUS_PLUGOUT		0 //  plugout, disconnect
#define HDMI_RX_STATUS_PLUGIN		1 // plugin , connect, playing
#define HDMI_RX_STATUS_ERR_INPUT    2  // this hrx device not support

#define HDMI_RX_STATUS_STOP   		3
#define HDMI_RX_STATUS_START   		4
#define HDMI_RX_STATUS_PAUSE 		5
#define HDMI_RX_STATUS_PLAYING   	6

#define HDMI_SWITCH_BOOTLOGO_LAYER			DIS_LAYER_MAIN
#define HDMI_SWITCH_HDMI_RX_LAYER           DIS_LAYER_AUXP

#define HDMI_SWITCH_DEFAULT_DISP_RESOLUTION TVSYS_1080P_60
#define HDMI_SWITCH_GET_EDID_TRY_TIME		500




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
    char play_status;
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


int hdmirx_get_plug_status(void);
int hdmirx_get_play_status(void);
void hdmi_set_display_rect(struct vdec_dis_rect* rect);

#ifdef HDMI_RX_CEC_SUPPORT

typedef struct {
    int device_la[16];
    int count;
}hdmirx_cec_device_res_t;


/**
 * @brief       Get cec device active status  
 * @retval      0       cec device not as active source 
 * @retval      other   cec device as active source 
 */
int hdmirx_cec_dev_active_status_get(void);

/**
 * @brief       Set cec device active status
 * @param[in]   status  Cec active status
 * @retval      0       cec device not as active source 
 * @retval      other   cec device as active source
 */
int hdmirx_cec_dev_active_status_set(int status);

/**
 * @brief       Get hdmirx  hudi cec handle
 * @retval      handle  hudi cec handle
 */
void * hdmirx_cec_handle_get(void);

/**
 * @brief       Get hdmirx  hudi cec scan res
 * @retval      handle  hudi cec handle
 */
void * hdmirx_cec_device_res_get(void);

/**
 * @brief       Set hdmirx  hudi boardcast en
 * @retval      handle  hudi cec handle
 */
int hdmirx_cec_boardcast_en_set(int en);

/**
 * @brief       Get hdmirx  hudi boardcast en flag
 * @retval      handle  hudi cec handle
 */
int hdmirx_cec_boardcast_en_get(void);

/**
 * @brief       Scan cec device and save scan res in globle var
 * @param[in]   handle  hudi cec handle 
 * @retval      0       not in standby
 * @retval      other   in standby
 */
int hdmirx_cec_device_scan(hudi_handle handle, hdmirx_cec_device_res_t *res);

/**
 * @brief       Poweron all playback devices through HDMI_CEC
 * @retval      0       Success
 * @retval      other   Error
 */
int hdmirx_cec_poweron_device(hudi_handle handle, hdmirx_cec_device_res_t res);

/**
 * @brief       Notify cec device whether the cec device is as active source 
 *              (TV stick prvivate protocal cmd)
 * @param[in]   bactive cec device active status
 *              (ture: TV stick responses bluetooth romote)
 *              (false: TV stick NOT responses bluetooth romote)
 * @retval      0       Success
 * @retval      other   Error
 */
int hdmirx_cec_device_active_status_report(bool bactive);

/**
 * @brief       Save the hudi cec address into sys param  
 * @param[in]   laes    cec address array
 * @retval      0       Success
 * @retval      other   Error
 */
int hdmirx_cec_active_device_sys_param_save(hudi_cec_logical_addresses_t *laes);

void hdmirx_cec_scan_device_save(hdmirx_cec_device_res_t *res);

/**
 * @brief       Open hdmirx hudi cec and create huid cec resv msg task   
 * @param[in]   laes    cec address array
 * @retval      0       Success
 * @retval      other   Error
 */
int hdmirx_cec_init(void);

/**
 * @brief       Close hdmirx hudi cec and del huid cec resv msg task
 * @param[in]   laes    cec address array
 * @retval      0       Success
 * @retval      other   Error
 */
int hdmirx_cec_deinit(void);

int hdmirx_cec_process_reload(void);

#endif



#endif
