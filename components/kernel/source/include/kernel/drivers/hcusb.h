#ifndef __HCUSB_H__
#define __HCUSB_H__

#include <linux/types.h>
#include <linux/usb/musb.h>
#include <string.h>


/* predefined index for usb controller */
enum {
    USB_PORT_0	= 0,
    USB_PORT_1,
    USB_PORT_MAX
};
struct hcusb_info {
    u8			usb_port;
    const char		*s;
};

void hcusb_phy_setting(uint8_t usb_port);
void hcusb_gadget_detect_handler(uint8_t mode);


extern struct hcusb_info hcusb_controller[];

/* get usb device mode contorller name */
static inline const char *get_udc_name(uint8_t usb_port)
{
    if(usb_port >= USB_PORT_MAX)
        return NULL;
    return hcusb_controller[usb_port].s;
}


static inline uint8_t get_usb_port(const char *udc)
{
    uint8_t usb_port;
    for (usb_port = 0; usb_port < USB_PORT_MAX; usb_port++) {
        if (!strncmp(hcusb_controller[usb_port].s, udc,
                 strlen(hcusb_controller[usb_port].s)))
            return usb_port;
    }
    return 0xff;
}

/*************************************************************************/
/**********************  usb gadget API***********************************/
/*************************************************************************/

/*
** brief: setup USB mode as HOST or GADGET
** parm:
**   usb_port -- 0:usb0, 1:usb1
**   mode -- usb mode (MUSB_HOST, MUSB_PERIPHERAL, MUSB_OTG)
** return:
**   0  -- successfully
**   !0 -- failure
*/
int hcusb_set_mode(uint8_t usb_port, enum musb_mode mode);



/*
** brief: return USB mode as HOST or GADGET
** parm:
**   usb_port -- 0:usb0, 1:usb1
** return:
**   mode -- usb mode (MUSB_HOST, MUSB_PERIPHERAL, MUSB_OTG)
*/
enum musb_mode hcusb_get_mode(uint8_t usb_port);


/* ************************************************************************************ */
/* ***************************** usb host : HID class ********************************* */
/* ************************************************************************************ */
/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *      The number of hichip SOC usb endpoints is limited.
 * Return:
 *      Return true :
 *           unnecessary hid interfaces are simply ignored.
 *      Return false :
 *           all hid interfaces will be used.
 */
bool get_config_usb_host_hid_reduced_mode(void);



/*************************************************************************/
/******************  usb gadget : VID/PID      ***************************/
/*************************************************************************/

/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *      config usb device VID (16 bits)
 * return :
 *      return 0xABCD as default
 */
uint16_t get_config_usb_gadget_vid(void);

/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *      config usb device PID (16 bits)
 * return :
 *      return 0x0001 as default
 */
uint16_t get_config_usb_gadget_pid(void);



/*************************************************************************/
/******************  usb gadget : setup usb mode(1.1 or 2.0)      ********/
/*************************************************************************/
/*
* brief :
*       configure uvc gadget speed mode(usb1.1 or usb2.0)
* return :
*       Return 0 :
* 	        Configure uvc driver as usb1.1 (full speed)
*       Return 1 (as default)
* 	        Configure uvc driver as usb2.0 (high speed)
*/
u32 get_config_usb_uvc_gadget_speed_mode(void);




/*************************************************************************/
/******************  usb gadget : mass-storage ***************************/
/*************************************************************************/

/*
** brief: setup USB gadget mode as MASS-STORAGE device
** parm:
**   path -- the array of path name
**   luns -- numbers of logic units
** return:
**   0  -- successfully
**   !0 -- failure
*/
int hcusb_gadget_msg_init(char **path, int luns);
int hcusb_gadget_msg_deinit(void);
int hcusb_gadget_msg_specified_init(const char *udc_name, char **path, int luns);



/*************************************************************************/
/******************  usb gadget : CDC (serial) ***************************/
/*************************************************************************/
int hcusb_gadget_serial_init(void);
void hcusb_gadget_serial_deinit(void);
int hcusb_gadget_serial_specified_init(const char *udc_name);



/*************************************************************************/
/******************  usb gadget : CDC (NCM)  *****************************/
/*************************************************************************/
int hcusb_gadget_ncm_init(void);
void hcusb_gadget_ncm_deinit(void);
int hcusb_gadget_ncm_specified_init(const char *udc_name);


/*************************************************************************/
/******************  usb gadget : HID      *******************************/
/*************************************************************************/
int hcusb_gadget_hidg_init(void);
void hcusb_gadget_hidg_deinit(void);
int hcusb_gadget_hidg_specified_init(const char *udc_name);


/*************************************************************************/
/******************  usb gadget : uvc      *******************************/
/*************************************************************************/
typedef int (*uvc_get_frame_cb_t)(uint8_t **frame, int *frame_sz);
typedef int (*uvc_switch_cb_t)(bool in_on);
int hcusb_gadget_uvc_init(uvc_switch_cb_t switch_hook,
                    uvc_get_frame_cb_t get_frame_hook);
int hcusb_gadget_uvc_resolution_setting(uint16_t wWidth,
                                        uint16_t wHeight);
void hcusb_gadget_uvc_deinit(void);
int hcusb_gadget_uvc_specified_init(const char *udc_name,
                        uvc_switch_cb_t switch_hook,
                        uvc_get_frame_cb_t get_frame_hook);


/*
 * deprecated, just keep it for backward compatible,
 *   should use get_config_usb_uvc_gadget_streaming_maxpacket() instead
 *
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *      config video stream endpoint maxpacket size, default 1024 bytes
 * return :
 *      maxpacket size
 *      return 1024 as default
 */
u32 config_usb_uvc_gadget_streaming_maxpacket(void);
u32 get_config_usb_uvc_gadget_streaming_maxpacket(void);


/*
 * deprecated, just keep it for backward compatible,
 *   should using get_config_usb_uvc_gadget_ep_address() instead
 *
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uvc gadget endpoint address
 * return :
 *       Return 1 :
 * 	        Configure the video stream endpoint address first,
 * 	        then the video CONTROL endpoint address;
 *       Return 0 (as default) :
 * 	        Configure the video CONTROL endpoint address first,
 * 	        then the video stream endpoint address;
 */
u32 config_usb_uvc_gadget_ep_address(void);
u32 get_config_usb_uvc_gadget_ep_address(void);





/*************************************************************************/
/******************  usb gadget : uac (uac1)  ****************************/
/*************************************************************************/
typedef enum {
    uvc_cam_dev = 0,     // video
    uac_speaker_dev, // audio: speaker
    uac_mic_dev      // audio: microphone
}usb_av_dev_t;

typedef int (*av_switch_cb_t)(usb_av_dev_t usb_av, bool in_on);
typedef int (*uac_microphone_cb_t)(uint8_t **frame, int *frame_sz);
typedef int (*uac_speaker_cb_t)(uint8_t *frame, int frame_sz);
int hcusb_gadget_uac_init( av_switch_cb_t switch_hook,
                           uac_microphone_cb_t mic_hook,
                           uac_speaker_cb_t speaker_hook);
void hcusb_gadget_uac_deinit(void);
int hcusb_gadget_uac_specified_init(const char *udc_name,
                                    av_switch_cb_t switch_hook,
                                    uac_microphone_cb_t mic_hook,
                                    uac_speaker_cb_t speaker_hook);



/*************************************************************************/
/******************  usb gadget : audio vedio  ***************************/
/*************************************************************************/
int hcusb_gadget_av_init(av_switch_cb_t switch_hook,
                        uvc_get_frame_cb_t get_frame_hook,
                        uac_microphone_cb_t mic_hook,
                        uac_speaker_cb_t speaker_hook);
void hcusb_gadget_av_deinit(void);
int hcusb_gadget_av_specified_init(const char *udc_name,
                    av_switch_cb_t switch_hook,
                    uvc_get_frame_cb_t get_frame_hook,
                    uac_microphone_cb_t mic_hook,
                    uac_speaker_cb_t speaker_hook);

/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uac gadget spaker channel
 * return :
 *       Return 0x0 : disable
 *       Return 0x1 :  Mono
 *       Return 0x3 (as default) :  Stereo
 */
int get_config_usb_uac_gadget_speaker_channel(void);


/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uac gadget microphone channel
 * return :
 *       Return 0x0 : disable
 *       Return 0x1 :  Mono
 *       Return 0x3 (as default) :  Stereo
 */
int get_config_usb_uac_gadget_microphone_channel(void);


/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uac gadget speaker sample rate
 * return :
 *       default return 48000 (48 Khz)
 */
int get_config_usb_uac_gadget_speaker_srate(void);


/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uac gadget microphone sample rate
 * return :
 *       default return 48000 (48 Khz)
 */
int get_config_usb_uac_gadget_microphone_srate(void);


/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uac gadget speaker bit depth (unit:byte)
 * return:
 *       default return 2 (16 bits)
 */
int get_config_usb_uac_gadget_speaker_bitdepth(void);


/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uac gadget microphone bit depth (unit:byte)
 * return:
 *       default return 2 (16 bits)
 */
int get_config_usb_uac_gadget_microphone_bitdepth(void);




/***************************************************/
/*************  usb gadget vendor : ium+audio ******/
/***************************************************/
int hcusb_gadget_ium_audio_init(uac_microphone_cb_t mic_hook,
                                uac_speaker_cb_t speaker_hook);
void hcusb_gadget_ium_audio_deinit(void);
int hcusb_gadget_ium_audio_specified_init(const char *udc_name,
                                uac_microphone_cb_t mic_hook,
                                uac_speaker_cb_t speaker_hook);

// with hid gadget driver
int hcusb_gadget_ium_audio_hid_specified_init(const char *udc_name, uac_microphone_cb_t func_microphone,
					  uac_speaker_cb_t func_speaker);
void hcusb_gadget_ium_audio_hid_deinit(void);

/***************************************************/
/*************  usb gadget vendor : zero ***********/
/***************************************************/
int hcusb_gadget_zero_init(void);
void hcusb_gadget_zero_deinit(void);
int hcusb_gadget_zero_specified_init(const char *udc_name);


/***************************************************/
/*************  usb gadget vendor : ium ***********/
/***************************************************/
int hcusb_gadget_ium_init(void);
void hcusb_gadget_ium_deinit(void);
int hcusb_gadget_ium_specified_init(const char *udc_name);

// with hid gadget driver
int hcusb_gadget_ium_hid_specified_init(const char *udc_name);
int hcusb_gadget_ium_hid_deinit(void);

/***************************************************/
/*************  usb gadget vendor : winusb *********/
/***************************************************/
int hcusb_gadget_winusb_init(void);
void hcusb_gadget_winusb_deinit(void);
int hcusb_gadget_winusb_specified_init(const char *udc_name);

/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure size of winusb host out buffer
 */
int get_config_usb_winusb_gadget_hostout_buf_sz(void);

/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure number of winusb host out buffers
 */
int get_config_usb_winusb_gadget_hostout_buf_cnt(void);

/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure size of winusb host in buffer
 */
int get_config_usb_winusb_gadget_hostin_buf_sz(void);





/***************************************************/
/********  usb gadget vendor : hid (touch panel) ***/
/***************************************************/
int hcusb_gadget_hidg_tp_specified_init(const char *udc_name);
int hcusb_gadget_hidg_tp_deinit(void);

uint16_t get_config_usb_gadget_hid_tp_width(void);
uint16_t get_config_usb_gadget_hid_tp_height(void);
#endif /* __HCUSB_H__ */
