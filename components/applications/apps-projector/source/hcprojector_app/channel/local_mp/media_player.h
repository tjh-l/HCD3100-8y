/**
 * media_player.h
 */

#ifndef __MEDIA_PLAYER_H__
#define __MEDIA_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "app_config.h"

#include <pthread.h>
#include <ffplayer.h>
#include <hcuapi/dis.h>
#include <sys/stat.h>
#include <hcuapi/vidmp.h>
#include "mp_zoom.h"
//#define  RTOS_SUBTITLE_SUPPORT
#define HCPLAYER_MSG_BUFFING_FLAG 0X1000
typedef enum{
	MEDIA_PLAY_NORMAL,
	MEDIA_PLAY_PREVIEW,
}media_play_mode_t;

typedef enum{
	MEDIA_TYPE_VIDEO,
	MEDIA_TYPE_MUSIC,
	MEDIA_TYPE_PHOTO,	
	MEDIA_TYPE_TXT,
	MEDIA_TYPE_COUNT,
}media_type_t;

typedef enum    
{
	MEDIA_STOP		= 0, 			//stop
	MEDIA_PLAY  		= 1, 		//normal playback
	MEDIA_PAUSE 		= 2, 		//pause
	MEDIA_FB	 		= 3,		//fast backward	
	MEDIA_FF	 		= 4,		//fast forword
	MEDIA_SB			= 5,		//slow backward
	MEDIA_SF			= 6,		//slow forward
	MEDIA_STEP			= 7,		//step video
	MEDIA_RESUME_STOP	= 8,		//
	MEDIA_PLAY_END		= 9,		//play in the end
} media_state_t;


//used for play list to manage the play mode
typedef enum
{
	PlAY_LIST_SEQUENCE, //seqeunce play media
	PLAY_LIST_REPEAT,  // Juset like sequnce
	PLAY_LIST_RAND,   //rand play media list
	PLAY_LIST_ONE,   // only play current media.
	PLAY_LIST_NONE,  //Just like sequnce
}PlayListLoopType;

typedef enum {
	FIRST_VIDEO_FRAME_INVALID = 0,
	FIRST_VIDEO_FRAME_DECODED = (1 << 0),
	FIRST_VIDEO_FRAME_SHOWED = (1 << 1),
	FIRST_VIDEO_FRAME_TRANSCODED = (1 << 2),
}media_frame_state_e;
/*def a user func callback to do something*/ 
typedef void (*mp_func_callback)(void* arg1,void* arg2);

//define media_do_opt ,//record the action in player,like rotate / zoom /....
#define DO_ROTATE_OPT		(1<<1)
#define DO_ZOOM_OPT			(1<<2)

typedef struct{
	media_type_t 		type;
	media_state_t 		state;	
	uint8_t 			speed;	//only used for video
	rotate_type_e 		rotate; //closewise
	Zoom_Param_t* 		zoom_param; // record the zoom opt param in media player:
	uint32_t 			media_do_opt; //record the action in player,like rotate / zoom /....
	dis_tv_mode_e 		ratio_mode; //video ratio mode attributes ,only use for video
	uint32_t 			play_time; //not used for photo
	uint32_t 			total_time; //not used for photo
	int64_t				last_seek_op_time;
	uint32_t			jump_interval;
	uint32_t 			time_gap; //only used for photo, the slide show play interval, unit is ms.
	PlayListLoopType	loop_type;
	int32_t				seek_step;// use for music manual seek to simulate music ff/fb
	char 				play_name[1024];
	void 				*player;
	int 				msg_id;
	uint16_t  			play_id;//user for msg_proc
	mp_func_callback	mp_func_cb;//use for iptv 
	uint8_t 			exit;
	uint8_t 			is_whth_album;	//used for Music album cover 
	bool 				is_double_urls; //use for double urls like iptv app
	int 				pic_effect_mode;
	media_frame_state_e frame_state;
	pthread_mutex_t 	api_lock;
}media_handle_t;

typedef struct{
	float filesize;
	int audio_tracks_count; 
	int subtitles_count;
	struct stat stat_buf ;
	HCPlayerAudioInfo audio_info;
	HCPlayerVideoInfo video_info;
	HCPlayerSubtitleInfo subtitle_info;
	HCPlayerMediaInfo media_info;
}mp_info_t ;

typedef struct{
	char* url1;     //meida_urls
	char* url2;     //audio_urls
	bool is_double_urls; //
}media_urls_t ;

typedef enum{
	CLOCKWISE=0,
	ANTICLOCKWISE,
}rotate_direction_e;

typedef enum {
	EFFECT_NULL=0,
	EFFECT_SHUTTERS,
	EFFECT_BRUSH,
	EFFECT_SLIDE,
	EFFECT_MOSAIC,
	EFFECT_FADE,
	EFFECT_RANDOM,
}mpimage_effect_type_e;

typedef struct{
	mpimage_effect_type_e type;
	image_effect_t param;
}mpimage_effect_t;




#ifdef RTOS_SUBTITLE_SUPPORT
typedef struct {
	uint16_t type; // 0: pic ;  others: text or ass or ....
	int w;
	int h;
	int tar_size;
	uint8_t *data;
}lv_subtitle_t;

#endif

media_handle_t *media_open(media_type_t type);
void media_close(media_handle_t *media_hld);
int media_play(media_handle_t *media_hld, const char *media_src);
int media_stop(media_handle_t *media_hld);
int media_pause(media_handle_t *media_hld);
int media_resume(media_handle_t *media_hld);
int media_seek(media_handle_t *media_hld, int time_sec);
media_state_t media_get_state(media_handle_t *media_hld);
int media_fastforward(media_handle_t *media_hld);
int media_fastbackward(media_handle_t *media_hld);
int media_slowforward(media_handle_t *media_hld);
int media_slowbackward(media_handle_t *media_hld);
uint32_t media_get_playtime(media_handle_t *media_hld);
uint32_t media_get_totaltime(media_handle_t *media_hld);
uint8_t media_get_speed(media_handle_t *media_hld);
char *media_get_cur_play_file(media_handle_t *media_hld);
int media_get_info(media_handle_t *media_hld,mp_info_t* mp_info);
// int media_set_previwe(media_handle_t *media_hld);
int set_img_dis_modeparam(image_effect_t *  g_img_effect);
int app_set_blacklight(bool val);
void media_set_play_mode(media_play_mode_t mode, vdec_dis_rect_t *rect);
int media_switch_blink(bool blink);
int media_change_rotate_type(media_handle_t* media_hld,rotate_direction_e type);
int media_play_with_double_url(media_handle_t* media_hld,media_urls_t* media_urls);
int media_stop_with_double_url(media_handle_t* media_hld);
int media_func_callback_register(media_handle_t* media_hld,mp_func_callback mp_func);
int media_play_next_program(media_handle_t* media_hld);
int media_play_prev_program(media_handle_t * media_hld);
int media_play_set_img_dis_mode(img_dis_mode_e dis_mode);

int media_manual_ffsetting(media_handle_t * media_hld);
int media_manual_fbsetting(media_handle_t * media_hld);
void media_manual_seek_end(media_handle_t * media_hld);
int media_manual_seekopt(media_handle_t * media_hld);
int meida_display_zoom(media_handle_t * media_hld,Zoom_mode_e zoom_mode);
void media_display_zoom_reset(media_handle_t * media_hld);
int media_play_id_update(media_handle_t* media_hld);
int media_change_rotate_mirror_type(media_handle_t* media_hld,rotate_type_e rotate_type,mirror_type_e mirror_type);
void media_display_sys_scale_reset(media_handle_t* media_hld);
int media_looptype_state_set(media_handle_t* media_hld,int looptype);
int media_display_ratio_set(media_handle_t* media_hld, int tv_mode);
void media_set_display_rect(media_handle_t *media_hld , struct vdec_dis_rect *new_rect);
void media_set_display_rect_by_ratio(media_handle_t* media_hld, dis_tv_mode_e ratio);
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
