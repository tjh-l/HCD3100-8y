#ifndef _HCUAPI_DIS_H_
#define _HCUAPI_DIS_H_

#include <hcuapi/iocbase.h>
#include <hcuapi/tvtype.h>
#include <hcuapi/pixfmt.h>
#include <hcuapi/vidmp.h>

#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#endif

typedef enum DIS_DEV_TYPE {
	DE_TYPE_2K,
	DE_TYPE_4K,
} dis_dev_type;

typedef enum DIS_TYPE {
	DIS_TYPE_SD = 0,
	DIS_TYPE_HD,
	DIS_TYPE_UHD,
} dis_type_e;

typedef enum DIS_PLAYMODE {
	DIS_NORMAL_PLAY,
	DIS_NORMAL_2_ABNORMAL,
	DIS_ABNORMAL_2_NORMAL
} dis_playmode_e;

typedef struct dis_area {
	uint16_t x;		//!< x-offset of the area
	uint16_t y;		//!< y-offset of the area
	uint16_t w;		//!< Width of the area
	uint16_t h;		//!< Height of the area
} dis_area_t;

typedef enum DIS_LAYER {
	DIS_LAYER_MAIN = 0x1,
	DIS_LAYER_AUXP = 0x10,
	DIS_LAYER_GMAS,
	DIS_LAYER_GMAF,
} dis_layer_e;

struct dis_src_dst_rect {
	struct dis_area src_rect;
	struct dis_area dst_rect;
};

typedef struct dis_layer_blend_order {
	enum DIS_TYPE distype;
	/* 0 <= layer <=3 */
	int main_layer;
	int auxp_layer;
	int gmas_layer;
	int gmaf_layer;
} dis_layer_blend_order_t;

typedef enum DIS_TV_MODE {
	DIS_TV_4_3 = 0,
	DIS_TV_16_9,
	DIS_TV_AUTO
} dis_tv_mode_e;

typedef enum DIS_RGB_CLOCK {
	DIS_RGB_CLOCK_27M = 1,
	DIS_RGB_CLOCK_33M = 2,
	DIS_RGB_CLOCK_49M = 3,
	DIS_RGB_CLOCK_66M = 4,
	DIS_RGB_CLOCK_74M = 5,
	DIS_RGB_CLOCK_85M = 6,
	DIS_RGB_CLOCK_108M = 7,
	DIS_RGB_CLOCK_6_6M = 8,
	DIS_RGB_CLOCK_9M = 9,
	DIS_RGB_CLOCK_39_6M = 10,
	DIS_RGB_CLOCK_74_25M = 11,
	DIS_RGB_CLOCK_148_5M = 12,
	DIS_RGB_CLOCK_54M = 13,
	DIS_RGB_CLOCK_132M = 14,
} dis_rgb_clock_e;

typedef enum DIS_MODE {
	DIS_PANSCAN = 0,		//!< default panscan is 16:9 source on 4:3 TV.
	DIS_PANSCAN_NOLINEAR,		//!< non-linear pan&scan
	DIS_LETTERBOX,
	DIS_PILLBOX,
	DIS_VERTICALCUT,
	DIS_NORMAL_SCALE,
	DIS_LETTERBOX149,
	DIS_AFDZOOM,
	DIS_PANSCAN43ON169,		//!< 4:3 source panscan on 16:9 TV.
	DIS_COMBINED_SCALE,
	DIS_VERTICALCUT_149,
	DIS_POINT2POINT,
} dis_mode_e;

typedef enum DIS_SCALE_ACTIVE_MODE {
	DIS_SCALE_ACTIVE_IMMEDIATELY,
	DIS_SCALE_ACTIVE_NEXTFRAME,
} dis_scale_avtive_mode_e;

typedef struct dis_tvsys {
	enum DIS_TYPE distype;
	enum DIS_LAYER layer;
	enum TVTYPE tvtype;
	bool progressive;
} dis_tvsys_t;

typedef struct dis_zoom {
	enum DIS_TYPE distype;
	enum DIS_LAYER layer;
	struct dis_area src_area;
	struct dis_area dst_area;
	enum DIS_SCALE_ACTIVE_MODE active_mode;
} dis_zoom_t;

typedef struct dis_aspect_mode {
	enum DIS_TYPE distype;
	enum DIS_TV_MODE tv_mode;
	enum DIS_MODE dis_mode;
	enum DIS_SCALE_ACTIVE_MODE active_mode;
	enum DIS_LAYER layer;
} dis_aspect_mode_t;

typedef struct dis_win_onoff {
	enum DIS_TYPE distype;
	enum DIS_LAYER layer;
	bool on;
} dis_win_onoff_t;

typedef enum DIS_VIDEO_ENHANCE {
	DIS_VIDEO_ENHANCE_BRIGHTNESS = (1 << 0),    // Value[0, 100], default 50
	DIS_VIDEO_ENHANCE_CONTRAST   = (1 << 1),    // Value[0, 100], default 50
	DIS_VIDEO_ENHANCE_SATURATION = (1 << 2),    // Value[0, 100], default 50
	DIS_VIDEO_ENHANCE_SHARPNESS  = (1 << 3),    // Value[0, 10 ], default 5
	DIS_VIDEO_ENHANCE_HUE        = (1 << 4),    // Value[0, 100], default 50
	DIS_VIDEO_ENHANCE_GAIN_R     = (1 << 5),    // Value[0, 100], default 50
	DIS_VIDEO_ENHANCE_GAIN_G     = (1 << 6),    // Value[0, 100], default 50
	DIS_VIDEO_ENHANCE_GAIN_B     = (1 << 7),    // Value[0, 100], default 50
	// Pivot value[0, 255], default 0, as for luma and R/G/B full-range value
	// actual pivot is derived as
	//   pivot_act = pivot / 255 * (act_range_high - act_range_low)
	// contrast is adjusted base on pivot
	//   channel_out = (channel_in - pivot_act) * contrast + pivot_act
	DIS_VIDEO_ENHANCE_CONT_PIVOT = (1 << 8),
	DIS_VIDEO_ENHANCE_CH_PIVOT_R = (1 << 9),
	DIS_VIDEO_ENHANCE_CH_PIVOT_G = (1 << 10),
	DIS_VIDEO_ENHANCE_CH_PIVOT_B = (1 << 11),
	// Channel black level, value[0, 255], default 0, as for R/G/B full-range value
	DIS_VIDEO_ENHANCE_CH_BLK_LVL_R = (1 << 12),
	DIS_VIDEO_ENHANCE_CH_BLK_LVL_G = (1 << 13),
	DIS_VIDEO_ENHANCE_CH_BLK_LVL_B = (1 << 14),
	DIS_VIDEO_EHNANCE_FACTORY    = (1 << 15),   // set: factory (consumer invisible) setting,
	                                            // unset: consumer visible (UI) setting
} dis_video_enhance_e;

typedef struct video_enhance_param {
	uint16_t enhance_type;	//!< Multiple bits of DIS_VIDEO_ENHANCE_*
	uint16_t grade;		//!< The enhancement value. Range of 0 ~ 100, default 50 means no enhancement.
	bool enable;		//!< 0: disable video enhancement, 1: enable video enhancement
} video_enhance_param_t;

typedef struct dis_video_enhance {
	enum DIS_TYPE distype;
	struct video_enhance_param enhance;
} dis_video_enhance_t;

typedef struct dis_hdmi_output_pix_fmt {
	enum DIS_TYPE distype;
	enum HC_PIXELFORMAT pixfmt;
} dis_hdmi_output_pix_fmt_t;

typedef enum DIS_PIC_LAYER {
	DIS_PIC_LAYER_MAIN,		//!< only can be used to get main pic info, can not use for backup pic
	DIS_PIC_LAYER_CURRENT,		//!< get current pic info or backup current pic info
	DIS_PIC_LAYER_AUX,		//!< get aux pic info or backup aux pic info
} DIS_PIC_LAYER_e;

typedef struct display_info {
	uint8_t   de_index; 		//!< Input parameter. 0: DE_HD; 1: DE_SD.
	enum DIS_PIC_LAYER layer;
	uint32_t mf_buf;		//!< MF buffer address
	uint32_t mf_buf_size;		//!< MF buffer size

	uint32_t y_buf;			//!< Y data  top field buffer address
	uint32_t y_buf_size;		//!< Y data buffer size

	uint32_t c_buf;			//!< C data  top field buffer address
	uint32_t c_buf_size;		//!< C data buffer size

	uint32_t pic_height;		//!< Height of picture
	uint32_t pic_width;		//!< Width of picture
	uint8_t de_map_mode;		//!< Y/C data mapping mode 0 or 1

#define DIS_DISPLAY_INFO_STATUS_NOTIMPLEMENTED	(0)
#define DIS_DISPLAY_INFO_STATUS_FAIL		(1)
#define DIS_DISPLAY_INFO_STATUS_SUCCESS		(2)
	uint8_t status;
	uint8_t sample_format;		//!< sample format
	uint8_t rotate_mode;
	struct dis_area pic_dis_area;
} display_info_t;

typedef struct dis_display_info {
	enum DIS_TYPE distype;
	struct display_info info;
} dis_display_info_t;

typedef struct dis_screen_info {
	enum DIS_TYPE distype;
	struct dis_area area;
} dis_screen_info_t;

typedef struct dis_rotate {
	enum DIS_TYPE distype;
	enum ROTATE_TYPE angle;
	bool mirror_enable;
	bool enable;
	enum DIS_LAYER layer;
} dis_rotate_t;

typedef struct dis_cutoff {
	enum DIS_TYPE distype;
	uint32_t cutoff;		//!< cut-off frequency value, Range of 0 ~ 100
} dis_cutoff_t;

typedef struct dis_rgb_timing_param {
	bool b_enable;

	enum DIS_RGB_CLOCK rgb_clock;

	uint32_t h_total_len;
	uint32_t v_total_len;

	uint32_t h_active_len;
	uint32_t v_active_len;

	uint32_t h_front_len;
	uint32_t h_sync_len;
	uint32_t h_back_len;

	uint32_t v_front_len;
	uint32_t v_sync_len;
	uint32_t v_back_len;
	bool  h_sync_level;
	bool  v_sync_level;
	uint32_t frame_rate;

	uint32_t h_display_len;
	uint32_t v_display_len;
	bool active_polarity;

	uint32_t dpll_clock_reg_value;
} dis_rgb_timing_param_t;

typedef struct dis_rgb_param {
	enum DIS_TYPE distype;
	struct dis_rgb_timing_param timing;
} dis_rgb_param_t;

typedef struct dis_win_autoonoff {
	enum DIS_TYPE distype;
	bool enable;
} dis_win_autoonoff_t;

typedef struct dis_single_output {
	enum DIS_TYPE distype;
	bool enable;
} dis_single_output_t;

typedef struct dis_ycbcr {
	uint8_t y;
	uint8_t	cb;
	uint8_t	cr;
} dis_ycbcr_t;

typedef struct dis_keystone_info {
	uint8_t enable;
	uint32_t width_up;
	uint32_t width_down;

	uint8_t bg_enable;
	struct dis_ycbcr bg_ycbcr;
	uint32_t height;			//!< Deprecated!
	uint32_t offset;			//!< Deprecated!
} dis_keystone_info_t;

typedef struct dis_keystone_param {
	enum DIS_TYPE distype;
	struct dis_keystone_info info;
} dis_keystone_param_t;

typedef struct dis_bg_color {
	enum DIS_TYPE distype;
	struct dis_ycbcr ycbcr;
} dis_bg_color_t;

typedef enum dis_dac_type {
	DIS_DAC_CVBS,
	DIS_DAC_SVIDEO,
	DIS_DAC_YUV,
	DIS_DAC_RGB,
} dis_dac_type_t;

typedef enum DIS_DAC_IDX {
	DIS_DAC_0 = (1 << 0),
	DIS_DAC_1 = (1 << 1),
	DIS_DAC_2 = (1 << 2),
} dis_dac_idx_e;

typedef struct dis_dac_info {
	bool enable;
	uint8_t progressive;
	union {
		struct {
			enum DIS_DAC_IDX cv;
		} cvbs;
		struct {
			enum DIS_DAC_IDX sv1;
			enum DIS_DAC_IDX sv2;
		} svideo;
		struct {
			enum DIS_DAC_IDX r;
			enum DIS_DAC_IDX g;
			enum DIS_DAC_IDX b;
		} rgb;
		struct {
			enum DIS_DAC_IDX y;
			enum DIS_DAC_IDX u;
			enum DIS_DAC_IDX v;
		} yuv;
	} dacidx;
} dis_dac_info_t;

typedef struct dis_dac_reginfo {
	enum dis_dac_type type;
	struct dis_dac_info dac;		//!< Not used for DIS_UNREGISTER_DAC
} dis_dac_reginfo_t;

typedef struct dis_dac_param {
	enum DIS_TYPE distype;
	struct dis_dac_reginfo info;
} dis_dac_param_t;

typedef struct dis_play_mode {
	enum DIS_TYPE distype;
	enum DIS_PLAYMODE playmode;
} dis_play_mode_t;

typedef struct dis_miracast_vscreen_detect_param {
	enum DIS_TYPE distype;
	bool on;
	enum DIS_LAYER layer;
	bool cast_photo_detect;
	bool skip_first_frame;
} dis_miracast_vscreen_detect_param_t;

typedef struct dis_suspend_resume {
	enum DIS_TYPE distype;
} dis_suspend_resume_t;

#define DIS_HIST_VEC_N  5
#define DIS_Q_FROM_FLOAT(x, qp) (int32_t)((x) * (1 << (qp)) + .5)
#define DIS_DENH_HIST_VEC_QP	8
#define DIS_DENH_HIST_VEC_FROM_FLOAT(a, b, c, d, e) \
	{ DIS_Q_FROM_FLOAT(a, DIS_DENH_HIST_VEC_QP), \
	  DIS_Q_FROM_FLOAT(b, DIS_DENH_HIST_VEC_QP), \
	  DIS_Q_FROM_FLOAT(c, DIS_DENH_HIST_VEC_QP), \
	  DIS_Q_FROM_FLOAT(d, DIS_DENH_HIST_VEC_QP), \
	  DIS_Q_FROM_FLOAT(e, DIS_DENH_HIST_VEC_QP), }
#define DIS_Q8P8_FROM_FLOAT(x) (int16_t)((x) * (1 << 8) + .5)
#define DIS_DENH_CONTRAST(x) DIS_Q8P8_FROM_FLOAT(x)
#define DIS_DENH_CONT_SCALE(x) DIS_Q8P8_FROM_FLOAT(x)

/// @brief user scene table entry, associate contrast and others with histogram vector
typedef struct dis_denh_user_scene {
	// Histogram vector, descreibes how histogram looks like.
	// Each component is <= 1.0, while total sum should equal to 1. Q8.8 format
	int16_t vec[DIS_HIST_VEC_N];
	// Associated contrast adjustment, Q8.8 format
	int16_t contrast;
	// Pivot for associated contrast adjustment
	int16_t pivot;
	// Associated black level adjustment
	int16_t blk_level;
	// Scene description, a short string for human reader
	char desc[32];
} dis_denh_user_scene_t;

#define DIS_DENH_MAX_N_SCENES  32

typedef struct dis_dyn_enh_user_scenes {
	enum DIS_TYPE distype;
	// number of scenes
	unsigned n_scenes;
	// contrast global scale, Q8.8 format, value 0 as not-applying global scale
	// scene actual contrast = scene scale * global scale
	int16_t contrast_scale;
	// boundaries to cluster histogram, as following shows
	//     ----- b0 ----- b1 ----- b2 ----- b3 ----- b4
	int16_t hist_bounds[DIS_HIST_VEC_N];
	// enhancement scenes
	struct dis_denh_user_scene scenes[DIS_DENH_MAX_N_SCENES];
} dis_dyn_enh_user_scenes_t;

typedef struct dis_dyn_enh_onoff {
	enum DIS_TYPE distype;
	// 0: off, 1: on
	int onoff;
} dis_dyn_enh_onoff_t;

#ifdef __HCRTOS__
typedef void (*callback_dis_vblank_t)(unsigned long param);
typedef struct dis_vblank_cb_param {
	enum DIS_TYPE distype;
	unsigned long param;
	callback_dis_vblank_t cb;
} dis_vblank_cb_param_t;
#endif

typedef struct dis_alpha_param {
	enum DIS_TYPE distype;
	enum DIS_LAYER layer;
	bool enable;
	uint8_t alpha; //!< min:0 max:0xFF
} dis_alpha_param_t;

typedef struct dis_keystone_rect {
	enum DIS_TYPE distype;
	struct dis_area ori_area;			//!< INPUT: original area
	struct dis_area dst_area;			//!< OUTPUT：destination area
} dis_keystone_rect_t;

#define DIS_SET_TVSYS			_IOW(DIS_IOCBASE, 0, struct dis_tvsys)
#define DIS_GET_TVSYS			_IOWR(DIS_IOCBASE, 1, struct dis_tvsys)
#define DIS_SET_ZOOM			_IOW(DIS_IOCBASE, 2, struct dis_zoom)
#define DIS_SET_ASPECT_MODE		_IOW(DIS_IOCBASE, 3, struct dis_aspect_mode)
#define DIS_SET_WIN_ONOFF		_IOW(DIS_IOCBASE, 4, struct dis_win_onoff)
#define DIS_SET_WIN_AUTOONOFF		_IOW(DIS_IOCBASE, 5, struct dis_win_autoonoff)

#define DIS_SET_VIDEO_ENHANCE		_IOW(DIS_IOCBASE, 6, struct dis_video_enhance)
#define DIS_SET_VIDEO_ENHANCE_ENABLE	_IOW(DIS_IOCBASE, 7, struct dis_video_enhance)
#define DIS_SET_HDMI_OUTPUT_PIXFMT	_IOW(DIS_IOCBASE, 8, struct dis_hdmi_output_pix_fmt)
#define DIS_SET_LAYER_ORDER		_IOW(DIS_IOCBASE, 9, struct dis_layer_blend_order)
#define DIS_GET_DISPLAY_INFO		_IOWR(DIS_IOCBASE, 10, struct dis_display_info)
#define DIS_SET_DISPLAY_INFO		_IOW (DIS_IOCBASE, 11, struct dis_display_info)
#define DIS_GET_SCREEN_INFO		_IOWR(DIS_IOCBASE, 12, struct dis_screen_info)

#define DIS_SET_CUTOFF			_IOW(DIS_IOCBASE, 13, struct dis_cutoff)
#define DIS_SET_LCD_PARAM		_IOW(DIS_IOCBASE, 14, struct dis_rgb_param)

#define DIS_SET_SINGLE_OUTPUT		_IOW(DIS_IOCBASE, 15, struct dis_single_output)
#define DIS_SET_KEYSTONE_PARAM		_IOW(DIS_IOCBASE, 16, struct dis_keystone_param)
#define DIS_SET_BG_COLOR		_IOW(DIS_IOCBASE, 17, struct dis_bg_color)
#define DIS_REGISTER_DAC		_IOW(DIS_IOCBASE, 18, struct dis_dac_param)
#define DIS_UNREGISTER_DAC		_IOW(DIS_IOCBASE, 19, struct dis_dac_param)
#define DIS_SET_MIRACAST_VSRCEEN_DETECT	_IOW(DIS_IOCBASE, 20, struct dis_miracast_vscreen_detect_param)
#define DIS_GET_MP_AREA_INFO		_IOWR(DIS_IOCBASE, 21, struct dis_screen_info)

#define DIS_BACKUP_MP			_IO (DIS_IOCBASE, 22) //<! enum DIS_TYPE for the argument
#define DIS_FREE_BACKUP_MP		_IO (DIS_IOCBASE, 23) //<! enum DIS_TYPE for the argument
#define DIS_PLAYMODE_CHANGE		_IOW (DIS_IOCBASE, 24, struct dis_play_mode)
#define DIS_SET_ROTATE			_IOW (DIS_IOCBASE, 25, struct dis_rotate)
#define DIS_GET_MIRACAST_PICTURE_ARER	_IOWR(DIS_IOCBASE, 26, struct dis_screen_info)
#define DIS_SET_SUSPEND			_IOW (DIS_IOCBASE, 27, struct dis_suspend_resume)
#define DIS_SET_RESUME			_IOW (DIS_IOCBASE, 28, struct dis_suspend_resume)
#define DIS_GET_KEYSTONE_PARAM		_IOWR(DIS_IOCBASE, 29, struct dis_keystone_param)

#define DIS_DENH_SET_USER_SCENES	_IOW (DIS_IOCBASE, 30, struct dis_dyn_enh_user_scenes)
#define DIS_DENH_RESTART		_IO (DIS_IOCBASE, 31) //<! enum DIS_TYPE for the argument
#define DIS_DENH_SET_ONOFF		_IOW (DIS_IOCBASE, 32, struct dis_dyn_enh_onoff)
#define DIS_DENH_GET_ONOFF		_IOR (DIS_IOCBASE, 33, struct dis_dyn_enh_onoff)
#ifdef __HCRTOS__
#define DIS_SET_VBLANK_CB		_IOW (DIS_IOCBASE, 34, struct dis_vblank_cb_param)
#endif

#define DIS_BACKUP_AUXP			_IO (DIS_IOCBASE, 35) //<! enum DIS_TYPE for the argument
#define DIS_FREE_BACKUP_AUXP		_IO (DIS_IOCBASE, 36) //<! enum DIS_TYPE for the argument
#define DIS_GET_CUR_DEV_TYPE		_IOR (DIS_IOCBASE, 37, enum DIS_DEV_TYPE)
#define DIS_GET_LAYER_ORDER		_IOR(DIS_IOCBASE, 38, struct dis_layer_blend_order)
#define DIS_SET_CVBS_SOURCE_LAYER	_IO (DIS_IOCBASE, 39) //<! param: layer must be <= 3, the LAYERS equal-and-less than the specific layer will be displayed
#define DIS_SET_GLOBAL_ALPHA		_IOW (DIS_IOCBASE, 40, struct dis_alpha_param)
#define DIS_GET_DST_RECT_OF_KEYSTONE	_IOWR(DIS_IOCBASE, 41, struct dis_keystone_rect)

#define DIS_NOTIFY_VBLANK		_IO (DIS_IOCBASE, 50)
#define DIS_NOTIFY_MIRACAST_VSRCEEN	_IO (DIS_IOCBASE, 51)
#define DIS_NOTIFY_DE_TYPE_CHANGE	_IO (DIS_IOCBASE, 52)

#endif /* _HCUAPI_DIS_H_ */
