#ifndef _HCUAPI_JPEG_ENC_H_
#define _HCUAPI_JPEG_ENC_H_

typedef struct jpeg_enc_quant {
	uint8_t dec_quant_y[64];
	uint8_t dec_quant_c[64];
	uint16_t enc_quant_y[64];
	uint16_t enc_quant_c[64];
} jpeg_enc_quant_t;

typedef enum JPEG_ENC_QUALITY_TYPE {
	JPEG_ENC_QUALITY_TYPE_NORMAL,
	JPEG_ENC_QUALITY_TYPE_LOW_BITRATE,
	JPEG_ENC_QUALITY_TYPE_HIGH_QUALITY,
	JPEG_ENC_QUALITY_TYPE_ULTRA_HIGH_QUALITY,
	JPEG_ENC_QUALITY_TYPE_USER_DEFINE,
	JPEG_ENC_QUALITY_TYPE_NUM,
} jpeg_enc_quality_type_e;


typedef enum WM_DATE_FMT {
	DATE_FMT_YMD_DASH,	//<! YYYY-MM-DD
	DATE_FMT_YMD_SLASH,	//<! YYYY/MM/DD
	DATE_FMT_MDY_DASH,	//<! MM-DD-YYYY
	DATE_FMT_MDY_SLASH,	//<! MM/DD/YYYY
	DATE_FMT_DMY_DASH,	//<! DD-MM-YYYY
	DATE_FMT_DMY_SLASH,	//<! DD/MM/YYYY
} wm_date_fmt_e;

/*      white       red         blue        green       black
 * Y    0xEB        0x5A        0x28        0x90        0x10
 * U    0x80        0x51        0xF0        0x35        0x80
 * V    0x80        0xF0        0x6d        0x22        0x80
 */
typedef struct wm_param {
	uint8_t  enable;        //<! watermark enable
	uint32_t x;             //<! display pos: x
	uint32_t y;             //<! display pos: y
	uint32_t color_y;       //<! font color y
	uint32_t color_u;       //<! font color u
	uint32_t color_v;       //<! font color v
	wm_date_fmt_e date_fmt; //<! wm_date_fmt_e
} wm_param_t;
#endif
