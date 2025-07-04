#ifndef __HCUAPI_HDMI_TX_H_
#define __HCUAPI_HDMI_TX_H_

#include <hcuapi/iocbase.h>
#include <hcuapi/tvtype.h>
#include <hcuapi/hdmi_cec.h>

#define HDMI_TX_GET_EDID_TVSYS			_IOR(HDMI_TX_IOCBASE, 0, enum TVSYS)
#define HDMI_TX_GET_EDIDINFO			_IOR(HDMI_TX_IOCBASE, 1, struct hdmi_edidinfo)
#define HDMI_TX_GET_HOTPLUG_STATUS		_IOR(HDMI_TX_IOCBASE, 2, uint32_t)
#define HDMI_TX_SET_PHY_ONOFF			_IO (HDMI_TX_IOCBASE, 7) //<! param: 0 off; 1 on

#define HDMI_TX_NOTIFY_CONNECT			_IO (HDMI_TX_IOCBASE, 8)
#define HDMI_TX_NOTIFY_DISCONNECT		_IO (HDMI_TX_IOCBASE, 9)
#define HDMI_TX_NOTIFY_EDIDREADY		_IOR(HDMI_TX_IOCBASE, 10, struct hdmi_edidinfo)

#define HDMI_TX_MAX_EDID_TVSYS_NUM		16

typedef int hdmi_edid_audio_format_t;

typedef enum HDMI_EDID_AUDIO_FMTBIT {
	HDMI_EDID_AUDIO_FMTBIT_LPCM = 0x0001,
	HDMI_EDID_AUDIO_FMTBIT_AC3 = 0x0002,
	HDMI_EDID_AUDIO_FMTBIT_MPEG1 = 0x0004,
	HDMI_EDID_AUDIO_FMTBIT_MP3 = 0x0008,
	HDMI_EDID_AUDIO_FMTBIT_MPEG2 = 0x0010,
	HDMI_EDID_AUDIO_FMTBIT_AAC = 0x0020,
	HDMI_EDID_AUDIO_FMTBIT_DTS = 0x0040,
	HDMI_EDID_AUDIO_FMTBIT_ATRAC = 0x0080,
	HDMI_EDID_AUDIO_FMTBIT_ONEBITAUDIO = 0x0100,
	HDMI_EDID_AUDIO_FMTBIT_DD_PLUS = 0x0200,
	HDMI_EDID_AUDIO_FMTBIT_DTS_HD = 0x0400,
	HDMI_EDID_AUDIO_FMTBIT_MAT_MLP = 0x0800,
	HDMI_EDID_AUDIO_FMTBIT_DST = 0x1000,
	HDMI_EDID_AUDIO_FMTBIT_BYE1PRO = 0x2000,
} hdmi_edid_audio_fmtbit_e;

typedef struct hdmi_edid_production_info {
	char manufacturer_name[4];
	char monitor_name[13];
} hdmi_edid_production_info_t;

typedef struct hdmi_edidinfo {
	enum TVSYS tvsys[HDMI_TX_MAX_EDID_TVSYS_NUM];
	int num_tvsys;
	hdmi_edid_audio_format_t audfmt;

	enum TVSYS best_tvsys;
	hdmi_edid_production_info_t prod_info;
} hdmi_edidinfo_t;

#endif /* __HCUAPI_HDMI_TX_H_ */
