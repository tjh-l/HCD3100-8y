#include <stdio.h>
#include <kernel/drivers/hcusb.h>


#ifdef BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER

uint16_t get_config_usb_gadget_vid(void)
{
#ifdef BR2_USBGADGETDRIVER_VID
    return BR2_USBGADGETDRIVER_VID;
#else
    return 0xABCD;
#endif
}

uint16_t get_config_usb_gadget_pid(void)
{
#ifdef BR2_USBGADGETDRIVER_PID
    return BR2_USBGADGETDRIVER_PID;
#else
    return 0x1234;
#endif
}


/*
 * brief : 
 *      The number of hichip SOC usb endpoints is limited.
 * Return:
 *      Return true :
 *           all hid interfaces will be used.
 *      Return false :
 *           unnecessary hid interfaces are simply ignored. 
 */ 
bool get_config_usb_host_hid_reduced_mode(void)
{
#ifdef BR2_PACKAGE_PREBUILTS_USBDRIVER_HID_REDUCED_MODE
    return false;
#else
    return true;
#endif
}


/* 
* brief :
*       configure uvc gadget speed mode(usb1.1 or usb2.0)
* return :
*       Return 0 : 
* 	        Configure uvc driver as usb1.1 (full speed)
*       Return 1 (as default)
* 	        Configure uvc driver as usb2.0 (high speed)
*/
u32 get_config_usb_uvc_gadget_speed_mode(void)
{
#ifdef BR2_USBGADGETDRIVER_USB_MODE_HIGHSPEED
	return 0x1;
#elif BR2_USBGADGETDRIVER_USB_MODE_FULLSPEED
	return 0x0;
#else    
	return 0x1;
#endif
}


#ifdef BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_MSC
u32 get_config_usb_gadget_msg_buflen(void)
{
#ifdef BR2_USBGADGETDRIVER_MSC_BUFLEN
    return BR2_USBGADGETDRIVER_MSC_BUFLEN << 10;
#else
    return 16384; // 16KB
#endif
}
#endif /* BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_MSC */



#ifdef BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_UVC
u32 get_config_usb_uvc_gadget_streaming_maxpacket(void)
{
#ifdef BR2_USBGADGETDRIVER_UVC_MAXPACKET
    return BR2_USBGADGETDRIVER_UVC_MAXPACKET;
#else
    return 1024;
#endif
}


/* 
 * note : 
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uvc gadget endpoint address
 * return :
 *       Return 0 (as default) :
 * 	        Configure the video CONTROL endpoint address first,
 * 	        then the video stream endpoint address;
 *       Return 1 : 
 * 	        Configure the video stream endpoint address first,
 * 	        then the video CONTROL endpoint address;
 */
u32 get_config_usb_uvc_gadget_ep_address(void)
{
#ifdef BR2_USBGADGETDRIVER_UVC_EP_ADDRESS_MODE_0
    return 0;
#elif BR2_USBGADGETDRIVER_UVC_EP_ADDRESS_MODE_1
    return 1;
#else
    return 0;
#endif
}


#endif /* BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_UVC */



#ifdef BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_UAC1
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
int get_config_usb_uac_gadget_speaker_channel(void)
{
#ifdef BR2_USBGADGETDRIVER_UAC1_SPEAKER_DISABLE
    return 0;
#elif BR2_USBGADGETDRIVER_UAC1_SPEAKER_MONO
    return 0x1;
#elif BR2_USBGADGETDRIVER_UAC1_SPEAKER_STEREO
    return 0x3;
#else
    return 0x3;
#endif
}


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
int get_config_usb_uac_gadget_microphone_channel(void)
{
#ifdef BR2_USBGADGETDRIVER_UAC1_MICROPHONE_DISABLE
    return 0;
#elif BR2_USBGADGETDRIVER_UAC1_MICROPHONE_MONO
    return 0x1;
#elif BR2_USBGADGETDRIVER_UAC1_MICROPHONE_STEREO
    return 0x3;
#else
    return 0x3;
#endif
}


/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uac gadget speaker sample rate
 * return :
 *       default return 48000 (48 Khz)
 */
int get_config_usb_uac_gadget_speaker_srate(void)
{
#ifdef BR2_USBGADGETDRIVER_UAC1_SPEAKER_SRATE
    return BR2_USBGADGETDRIVER_UAC1_SPEAKER_SRATE;
#else
    return 48000;
#endif
}


/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uac gadget microphone sample rate
 * return :
 *       default return 48000 (48 Khz)
 */
int get_config_usb_uac_gadget_microphone_srate(void)
{
#ifdef BR2_USBGADGETDRIVER_UAC1_MICROPHONE_SRATE
    return BR2_USBGADGETDRIVER_UAC1_MICROPHONE_SRATE;
#else
    return 48000;
#endif
}


/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uac gadget speaker bit depth (unit:byte)
 * return:
 *       default return 2 (16 bits)
 */
int get_config_usb_uac_gadget_speaker_bitdepth(void)
{
#ifdef BR2_USBGADGETDRIVER_UAC1_SPEAKER_BITDEPTH_8_BITS
    return 8;
#elif BR2_USBGADGETDRIVER_UAC1_SPEAKER_BITDEPTH_16_BITS
    return 16;
#elif BR2_USBGADGETDRIVER_UAC1_SPEAKER_BITDEPTH_24_BITS
    return 24;
#elif BR2_USBGADGETDRIVER_UAC1_SPEAKER_BITDEPTH_32_BITS
    return 32;
#else
    return 16;
#endif
}


/*
 * note :
 *      This is a weak function that can be implemented at the user level.
 * brief :
 *       configure uac gadget microphone bit depth (unit:byte)
 * return:
 *       default return 2 (16 bits)
 */
int get_config_usb_uac_gadget_microphone_bitdepth(void)
{
#ifdef BR2_USBGADGETDRIVER_UAC1_MICROPHONE_BITDEPTH_8_BITS
    return 8;
#elif BR2_USBGADGETDRIVER_UAC1_MICROPHONE_BITDEPTH_16_BITS
    return 16;
#elif BR2_USBGADGETDRIVER_UAC1_MICROPHONE_BITDEPTH_24_BITS
    return 24;
#elif BR2_USBGADGETDRIVER_UAC1_MICROPHONE_BITDEPTH_32_BITS
    return 32;
#else
    return 16;
#endif
}

#endif /* BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_UAC1 */


#ifdef BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_WINUSB

int get_config_usb_winusb_gadget_hostout_buf_sz(void)
{
#ifdef BR2_USBGADGETDRIVER_WINUSB_HOST_OUT_BUF_SZ
    return (BR2_USBGADGETDRIVER_WINUSB_HOST_OUT_BUF_SZ * 1024);
#else
	return 32 * 1024;
#endif
}

int get_config_usb_winusb_gadget_hostout_buf_cnt(void)
{
#ifdef BR2_USBGADGETDRIVER_WINUSB_HOST_OUT_BUF_CNT
    return BR2_USBGADGETDRIVER_WINUSB_HOST_OUT_BUF_CNT;
#else
	return 6;
#endif
}

int get_config_usb_winusb_gadget_hostin_buf_sz(void)
{
#ifdef BR2_USBGADGETDRIVER_WINUSB_HOST_IN_BUF_SZ
	return (BR2_USBGADGETDRIVER_WINUSB_HOST_IN_BUF_SZ * 1024);
#else
    return 4096;
#endif
}

#endif /* BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_WINUSB */

#ifdef BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_HID_TP

uint16_t get_config_usb_gadget_hid_tp_width(void)
{
#ifdef BR2_USBGADGETDRIVER_HID_TP_WIDTH
    return BR2_USBGADGETDRIVER_HID_TP_WIDTH;
#else
    return 1920;
#endif
}

uint16_t get_config_usb_gadget_hid_tp_height(void)
{
#ifdef BR2_USBGADGETDRIVER_HID_TP_HEIGHT
    return BR2_USBGADGETDRIVER_HID_TP_HEIGHT;
#else
    return 1200;
#endif
}

#endif /* BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER_HID_TP */


#endif /* BR2_PACKAGE_PREBUILTS_USBGADGETDRIVER */
