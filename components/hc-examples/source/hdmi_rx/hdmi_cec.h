#ifndef _HDMI_CEC_H_
#define _HDMI_CEC_H_

#define INVALID_CEC_PHYSICAL_ADDRESS		0xFFFF

typedef enum HDMI_CEC_CMD {
    HDMI_CEC_CMD_SYSTEM_STANDBY ,
    HDMI_CEC_CMD_IMAGE_VIEW_ON ,
    HDMI_CEC_CMD_PHYSICAL_ADDR ,
    HDMI_CEC_CMD_GIVE_POWER ,
    HDMI_CEC_CMD_USER_CTRL_PRESSED ,
    HDMI_CEC_CMD_USER_CTRL_RELEASED ,
} hdmi_cec_cmd_e;



/************************************************************************************************************
* 	CEC Related Defined
************************************************************************************************************/
typedef enum {
    // One Touch Play
    OPCODE_ACTIVE_SOURCE = 0x82 ,
    OPCODE_IMAGE_VIEW_ON = 0x04 ,
    OPCODE_TEXT_VIEW_ON = 0x0D ,

    // Routing Control Feature
    OPCODE_INACTIVE_SOURCE = 0x9D ,
    OPCODE_REQUEST_ACTIVE_SOURCE = 0x85 ,
    OPCODE_ROUTING_CHANGE = 0x80 ,
    OPCODE_ROUTING_INFORMATION = 0x81 ,
    OPCODE_SET_STREAM_PATH = 0x86 ,

    // Standby Feature
    OPCODE_SYSTEM_STANDBY = 0x36 ,

    // One Touch Record
    OPCODE_RECORD_OFF = 0x0B ,
    OPCODE_RECORD_ON = 0x09 ,
    OPCODE_RECORD_STATUS = 0x0A ,
    OPCODE_RECORD_TV_SCREEN = 0x0F ,

    // Timer Programming
    OPCODE_CLEAR_ANALOG_TIMER = 0x33 ,
    OPCODE_CLEAR_DIGITAL_TIMER = 0x99 ,
    OPCODE_CLEAR_EXTERNAL_TIMER = 0xA1 ,
    OPCODE_SET_ANALOG_TIMER = 0x34 ,
    OPCODE_SET_DIGITAL_TIMER = 0x97 ,
    OPCODE_SET_EXTERNAL_TIMER = 0xA2 ,
    OPCODE_SET_TIMER_PROGRAM_TITLE = 0x67 ,
    OPCODE_TIMER_CLEARED_STATUS = 0x43 ,
    OPCODE_TIMER_STATUS = 0x35 ,

    // System Information
    OPCODE_CEC_VERSION = 0x9E ,
    OPCODE_GET_CEC_VERSION = 0x9F ,
    OPCODE_GIVE_PHYSICAL_ADDR = 0x83 ,
    OPCODE_GET_MENU_LANGUAGE = 0x91 ,
    OPCODE_REPORT_PHYSICAL_ADDR = 0x84 ,
    OPCODE_SET_MENU_LANGUAGE = 0x32 ,

    // Deck Control
    OPCODE_DECK_CONTROL = 0x42 ,
    OPCODE_DECK_STATUS = 0x1B ,
    OPCODE_GIVE_DECK_STATUS = 0x1A ,
    OPCODE_PLAY = 0x41 ,

    OPCODE_SELECT_ANALOG_SERVICE = 0x92 ,
    OPCODE_SELECT_DIGITAL_SERVICE = 0x93 ,

    // Vendor Specific Commands
    OPCODE_DEVICE_VENDOR_ID = 0x87 ,
    OPCODE_GIVE_DEVICE_VENDOR_ID = 0x8C ,
    OPCODE_VENDOR_COMMAND = 0x89 ,
    OPCODE_VENDOR_COMMAND_WITH_ID = 0xA0 ,
    OPCODE_VENDOR_REOMTE_BUTTON_DOWN = 0x8A ,
    OPCODE_VENDOR_REOMTE_BUTTON_UP = 0x8B ,

    // OSD Status Display
    OPCODE_SET_OSD_STRING = 0x64 ,

    // Device OSD Transfer
    OPCODE_GIVE_OSD_NAME = 0x46 ,
    OPCODE_SET_OSD_NAME = 0x47 ,

    // Device Menu Control
    OPCODE_MENU_REQUEST = 0x8D ,
    OPCODE_MENU_STATUS = 0x8E ,

    // Remote Control Passthrough
    OPCODE_USER_CTRL_PRESSED = 0x44 ,
    OPCODE_USER_CTRL_RELEASED = 0x45 ,

    // Power STATUS
    OPCODE_GIVE_POWER_STATUS = 0x8F ,
    OPCODE_REPORT_POWER_STATUS = 0x90 ,

    // General Protocol Message
    OPCODE_FEATURE_ABORT = 0x00 ,
    OPCODE_ABORT = 0xFF ,

    // System Audio Control
    OPCODE_GIVE_AUDIO_STATUS = 0x71 ,
    OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS = 0x7D ,
    OPCODE_REPORT_AUDIO_STATUS = 0x7A ,
    OPCODE_REPORT_SHORT_AUDIO_DESCRIPTOR = 0xA3 ,	// CEC V1.4 New MSG
    OPCODE_REQUEST_SHORT_AUDIO_DESCRIPTOR = 0xA4 ,	// CEC V1.4 New MSG
    OPCODE_SET_SYSTEM_AUDIO_MODE = 0x72 ,
    OPCODE_SYSTEM_AUDIO_MODE_REQUEST = 0x70 ,
    OPCODE_SYSTEM_AUDIO_MODE_STATUS = 0x7E ,

    // Audio Rate Control
    OPCODE_SET_AUDIO_RATE = 0x9A ,

    // Audio Return Channel Control (CEC V1.4 New MSG)
    OPCODE_INITIATE_ARC = 0xC0 ,
    OPCODE_REPORT_ARC_INITIATED = 0xC1 ,
    OPCODE_REPORT_ARC_TERMINATED = 0xC2 ,
    OPCODE_REQUEST_ARC_INITIATION = 0xC3 ,
    OPCODE_REQUEST_ARC_TERMINATION = 0xC4 ,
    OPCODE_TERMINAE_ARC = 0xC5 ,

    // Capability Discovery and Control Feature (CEC V1.4 New MSG)
    OPCODE_CDC_MESSAGE = 0xF8 ,
}	E_CEC_OPCODE;

typedef enum {
    CEC_LA_TV = 0x0 ,
    CEC_LA_RECORD_1 = 0x1 ,
    CEC_LA_RECORD_2 = 0x2 ,
    CEC_LA_TUNER_1 = 0x3 ,
    CEC_LA_PLAYBACK_1 = 0x4 ,
    CEC_LA_AUDIO_SYSTEM = 0x5 ,
    CEC_LA_TUNER_2 = 0x6 ,
    CEC_LA_TUNER_3 = 0x7 ,
    CEC_LA_PLAYBACK_2 = 0x8 ,
    CEC_LA_RECORD_3 = 0x9 ,
    CEC_LA_TUNER_4 = 0xA ,
    CEC_LA_PLAYBACK_3 = 0xB ,
    CEC_LA_RESERVED_1 = 0xC ,
    CEC_LA_RESERVED_2 = 0xD ,
    CEC_LA_FREE_USE = 0xE ,
    CEC_LA_BROADCAST = 0xF ,
}E_CEC_LOGIC_ADDR;


typedef struct hdmi_tx_cec_config {
    E_CEC_LOGIC_ADDR 		logical_address;
    uint16_t				physical_address;
} hdmi_tx_cec_config_t;


#endif
