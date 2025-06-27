/**
 *
 * 
 * media_player.c. The file is for media player, the play action include
 * stop/play/pause/seek/fast forward/fast backward/slow forward/slow backward/step/
 */
#include "app_config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ffplayer.h>
#include <hcuapi/dis.h>
#include <hcuapi/avsync.h>
#include <hcuapi/snd.h>
#include "com_api.h"
#include "media_player.h"
#include <sys/ioctl.h>
#include "os_api.h"
#include "key_event.h"
#include "factory_setting.h"
#include "screen.h"
#include "mp_fspage.h"
#include "local_mp_ui.h"
#include "mp_ctrlbarpage.h" 
#include "setup.h"
#ifdef BLUETOOTH_SUPPORT
#include <bluetooth.h>
#endif
#include <hcuapi/audsink.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/lvds.h>
#ifdef RTOS_SUBTITLE_SUPPORT
#include <hcuapi/vidsink.h>
#endif
#include "mp_playlist.h"
#include "ffplayer_manager.h"

static int m_ff_speed[] = {1, 2, 4, 8, 16, 24, 32};
static int m_fb_speed[] = {1, -2, -4, -8, -16, -24, -32};
static float m_sf_speed[] = {1.0, 1.0/2.0, 1.0/4.0, 1.0/8.0, 1.0/16.0, 1.0/24.0};
static float m_sb_speed[] = {1, -1/2, -1/4, -1/8, -1/16, -1/24};
static bool m_closevp = false,  m_fillblack = false;
static media_play_mode_t preview_enable = MEDIA_PLAY_NORMAL;
static vdec_dis_rect_t m_dis_rect;// used for preview/normal
static pthread_t thread_id = 0;
typedef struct{
	volatile img_dis_mode_e dis_mode;
}play_cfg_s;

static play_cfg_s m_play_config = {
	.dis_mode = IMG_DIS_AUTO,
};
void media_set_play_mode(media_play_mode_t mode, vdec_dis_rect_t *rect)
{
	preview_enable = mode;
	memcpy(&m_dis_rect,rect, sizeof(vdec_dis_rect_t));
}
static void *media_monitor_task(void *arg);
static int media_monitor_deinit(media_handle_t *media_hld);
static int media_monitor_init(media_handle_t *media_hld);
static void media_msg_proc(media_handle_t *media_hld, HCPlayerMsg *msg);

static void media_msg_proc(media_handle_t *media_hld, HCPlayerMsg *msg)
{
	if (!media_hld || !msg)
		return;
	if(media_hld->play_id!=(uint32_t)msg->user_data){
		/*do cmp can avoid lot of msg send after it do next*/
		return;
	}
	uint32_t ctlmsg_code=msg->type <<16 | (uint32_t)msg->user_data;
	switch (msg->type)
	{
	case HCPLAYER_MSG_STATE_EOS:
		printf (">> app get eos, normal play end!\n");
		media_hld->state=MEDIA_PLAY_END;
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_STATE_TRICK_EOS:
		printf (">> app get trick eos, fast play to end\n");
		media_hld->state= MEDIA_PLAY_END;
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_STATE_TRICK_BOS:
		printf (">> app get trick bos, fast back play to begining!\n");
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_OPEN_FILE_FAILED:
	case HCPLAYER_MSG_ERR_UNDEFINED:
	case HCPLAYER_MSG_UNSUPPORT_FORMAT:
		printf (">> open file fail\n");
		if(m_closevp == false&&media_hld->type!=MEDIA_TYPE_MUSIC){
			api_dis_show_onoff(false);
			//Beacause of video/pic frame backup,therefore close last frame if next frame can not play
		}
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_BUFFERING:
		printf(">> buffering %d\n", msg->val);
		ctlmsg_code=(HCPLAYER_MSG_BUFFING_FLAG|msg->val)<<16 | (uint32_t)msg->user_data;
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_STATE_PLAYING:
		printf(">> player playing\n");
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_STATE_PAUSED:
		printf(">> player paused\n");
		break;
	case HCPLAYER_MSG_STATE_READY:
		printf(">> player ready\n");
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_READ_TIMEOUT:
		printf(">> player read timeout\n");
		break;
	case HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO:
		printf(">>Audio Track Unsupport\n");
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO:
		printf(">>Video Track Unsupport\n");
		if(m_closevp == false&&media_hld->type!=MEDIA_TYPE_MUSIC){
			api_dis_show_onoff(false);
			//Beacause of video frame backup,therefore close last frame if next frame can not play
		}
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_UNSUPPORT_VIDEO_TYPE:
		{
			//this case mean player has not this type's decode and player will
			//change to next one.if player's all type decode not support send unspport all video 
			HCPlayerVideoInfo video_info;
			char *video_type = "unknow";
			if (media_hld->player && !hcplayer_get_nth_video_stream_info (media_hld->player, msg->val, &video_info)) {
				/* only a simple sample, app developers use a static struct to mapping them. */
				if (video_info.codec_id == HC_AVCODEC_ID_HEVC) {
					video_type = "h265";
				} 
			}
			printf(">>Unsupport Video Type:%s\n", video_type);
		}
		break;
	case HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE:
		{
			HCPlayerAudioInfo audio_info;
			char *audio_type = "unknow";
			//msg->val mean audio track here  
			if (media_hld->player && !hcplayer_get_nth_audio_stream_info (media_hld->player, msg->val, &audio_info)) {
				/* only a simple sample, app developers use a static struct to mapping them. */
				if (audio_info.codec_id < 0x11000) {
					audio_type = "pcm";
				} else if (audio_info.codec_id < 0x12000) {
					audio_type = "adpcm";
				} else if (audio_info.codec_id == HC_AVCODEC_ID_DTS) {
					audio_type = "dts";
				} else if (audio_info.codec_id == HC_AVCODEC_ID_EAC3) {
					audio_type = "eac3";
				} else if (audio_info.codec_id == HC_AVCODEC_ID_APE) {
					audio_type = "ape";
				} 
			}
			printf(">>Unsupport Audio Type:%s\n", audio_type);
		}
		break;
	case HCPLAYER_MSG_AUDIO_DECODE_ERR:
		{
			//this case mean player has this decode but decode err
			printf(">>audio dec err, audio idx %d\n", msg->val);
			/* check if it is the last audio track, if not, then change to next one. */
			if (media_hld->player) {
				int total_audio_num = -1;
				total_audio_num = hcplayer_get_audio_streams_count(media_hld->player);
				if (msg->val >= 0 && total_audio_num > (msg->val + 1)) {
					HCPlayerAudioInfo audio_info;
					if (!hcplayer_get_cur_audio_stream_info(media_hld->player, &audio_info)) {
						if (audio_info.index == msg->val) {
							int idx = audio_info.index + 1;
							while (hcplayer_change_audio_track(media_hld->player, idx)) {
								idx++;
								if (idx >= total_audio_num) {
									api_control_send_media_message(ctlmsg_code);
									break;
								}
							}
						}
					}
				}else{
					hcplayer_change_audio_track(media_hld->player, -1);
					api_control_send_media_message(ctlmsg_code);
				}
			}
		}
		break;
	case HCPLAYER_MSG_VIDEO_DECODE_ERR:
	{
		//this case mean player has this decode but decode err
		printf("video dec err, video idx %d\n", msg->val);
		/* check if it is the last video track, if not, then change to next one. */
		if (media_hld->player) {
			int total_video_num = -1;
			total_video_num = hcplayer_get_video_streams_count(media_hld->player);
			if (msg->val >= 0 && total_video_num > (msg->val + 1)) {
				HCPlayerVideoInfo video_info;
				if (!hcplayer_get_cur_video_stream_info(media_hld->player, &video_info)) {
					if (video_info.index == msg->val) {
						int idx = video_info.index + 1;
							while (hcplayer_change_video_track(media_hld->player, idx)) {
								idx++;
								if (idx >= total_video_num) {
									if(m_closevp == false&&media_hld->type!=MEDIA_TYPE_MUSIC){
										api_dis_show_onoff(false);
										/*Beacause of video frame backup,therefore close last frame 
										  if next frame can not play*/
									}
									api_control_send_media_message(ctlmsg_code);
									break;
								}
							}
						}
					}
				}else{
					if(m_closevp == false&&media_hld->type!=MEDIA_TYPE_MUSIC){
						api_dis_show_onoff(false);
						/*Beacause of video frame backup,therefore close last frame 
							if next frame can not play*/
					}
					hcplayer_change_video_track(media_hld->player, -1);
					api_control_send_media_message(ctlmsg_code);
				}
			}
		}
		break;
	case HCPLAYER_MSG_HTTP_FORBIDDEN:
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_FIRST_VIDEO_FRAME_DECODED:
		media_hld->frame_state |= FIRST_VIDEO_FRAME_DECODED;
		printf(">> FIRST_VIDEO_FRAME_DECODED\n");
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_FIRST_VIDEO_FRAME_SHOWED:
		media_hld->frame_state |= FIRST_VIDEO_FRAME_SHOWED;
		printf(">> FIRST_VIDEO_FRAME_SHOWED\n");
		api_control_send_media_message(ctlmsg_code);
		break;
	case HCPLAYER_MSG_FIRST_VIDEO_FRAME_TRANSCODED:
		media_hld->frame_state |= FIRST_VIDEO_FRAME_TRANSCODED;
		printf(">> FIRST_VIDEO_FRAME_TRANSCODED\n");
		api_control_send_media_message(ctlmsg_code);
		break;
	default:
		break;
	}
}

static void *media_monitor_task(void *arg)
{
    HCPlayerMsg msg;
    media_handle_t *media_hld = (media_handle_t *)arg;
    while(!media_hld->exit) {
        do{
        #ifdef __linux__
        	#include <sys/msg.h>
        	//IPC_NOWAIT, no block but when when hcplayer send message and hcplayer send messaga
            if (msgrcv(media_hld->msg_id, (void *)&msg, sizeof(HCPlayerMsg) - sizeof(msg.type), 0, IPC_NOWAIT) == -1)
        #else
            if (xQueueReceive((QueueHandle_t)media_hld->msg_id, (void *)&msg, -1) != pdPASS)
        #endif
            {
                break;
            }
			pthread_mutex_lock(&media_hld->api_lock);
			if (media_hld->player)
				media_msg_proc(media_hld, &msg);
			pthread_mutex_unlock(&media_hld->api_lock);
        }while(0);
        api_sleep_ms(10);
    }
    printf("exit media_monitor_task()\n");
    return NULL;
}

static int media_monitor_init(media_handle_t *media_hld)
{
    pthread_attr_t attr;

    if (INVALID_ID != media_hld->msg_id)
        return API_SUCCESS;

    media_hld->msg_id = api_message_create(CTL_MSG_COUNT, sizeof(HCPlayerMsg));
    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    if (pthread_create(&thread_id, &attr, media_monitor_task, (void*)media_hld))
    {
        pthread_attr_destroy(&attr);
        return API_FAILURE;
    }  
    
    pthread_attr_destroy(&attr);
    return API_SUCCESS;
}


static int media_monitor_deinit(media_handle_t *media_hld)
{
    if (INVALID_ID == media_hld->msg_id)
        return API_SUCCESS;

#ifndef __linux__
    HCPlayerMsg msg;
    msg.type = HCPLAYER_MSG_ERR_UNDEFINED;
    xQueueSend((QueueHandle_t)media_hld->msg_id, ( void * ) &msg, ( TickType_t ) 0 );
#endif
    media_hld->exit = 1; 
    if(thread_id){
        pthread_join(thread_id,NULL);
    }
    api_message_delete(media_hld->msg_id);
    media_hld->msg_id = INVALID_ID;
    return API_SUCCESS;
}

#ifdef RTOS_SUBTITLE_SUPPORT
static lv_subtitle_t lv_subtitle={0};
static uint16_t last_buf_size = 0;

static void subtitle_callback(HCPlayerCallbackType type,
	void *user_data, void *val)
{
	HCSubtitle *sub;
	uint32_t buf_size = 0;
	(void)user_data;
	sub = (HCSubtitle *)val;

	if (type == HCPLAYER_CALLBACK_SUBTITLE_OFF) {
		printf("HCPLAYER_CALLBACK_SUBTITLE_OFF\n");
		if(lv_subtitle.type == 0) {		
			subtitles_event_send(SUBTITLES_EVENT_HIDDEN, NULL);
		} else {
			subtitles_event_send(SUBTITLES_EVENT_HIDDEN, NULL);
			printf("HCPLAYER_CALLBACK_SUBTITLE_OFF: sub format %d\n", lv_subtitle.type);
		}
	}
	if (type == HCPLAYER_CALLBACK_SUBTITLE_ON) {
		lv_subtitle.type = sub->format; //lv_subtitle.format = sub->format;
		if (sub->format == 0) { // subtitle pic show     
			/*AV_PIX_FMT_PAL8 ----> AV_PIX_FMT_ARGB*/
			for(int i= 0;i < sub->num_rects; i++) {//
				buf_size = sub->rects[i]->linesize[0] * sub->rects[i]->h;
				if(last_buf_size < buf_size) {
					last_buf_size = buf_size;
					lv_subtitle.tar_size=buf_size;
					if (!lv_subtitle.data) {
						lv_subtitle.data = malloc(buf_size);
					} else {
						lv_subtitle.data = realloc(lv_subtitle.data, buf_size);
					}
					if (!lv_subtitle.data) {
						printf( "no memory %s:%d\n", __func__ ,__LINE__);
						return ;
					}
					memset(lv_subtitle.data, 0,buf_size);
				}else{
					memset(lv_subtitle.data, 0, last_buf_size);
				}

				if (lv_subtitle.data) {
					memcpy(lv_subtitle.data, sub->rects[i]->data[0], buf_size);
					lv_subtitle.w=sub->rects[i]->w;
					lv_subtitle.h=sub->rects[i]->h;
					printf("w %d, h %d, linesize %d, tar_size %ld\n",
						sub->rects[i]->w, sub->rects[i]->h, sub->rects[i]->linesize[0], buf_size);
					subtitles_event_send(SUBTITLES_EVENT_SHOW, &lv_subtitle);
				}else {
					lv_subtitle.tar_size = 0;
				}
			}
		}else if(sub->format == 1){
			HCSubtitleRect  *subRects = *sub->rects;
			if(subRects->type == HC_SUBTITLE_TEXT ){
				if(subRects->text != NULL){
					buf_size = strlen(subRects->text) + 1;
					if(last_buf_size < buf_size) {
						last_buf_size = buf_size;
						if (!lv_subtitle.data) {
							lv_subtitle.data = malloc(buf_size);
						} else {
							lv_subtitle.data = realloc(lv_subtitle.data, buf_size);
						}
						if (!lv_subtitle.data) {
							printf( "Error: no memory %s:%d\n", __func__ ,__LINE__);
							return ;
						}
						memset(lv_subtitle.data, 0, buf_size);
					}else{
						memset(lv_subtitle.data, 0, last_buf_size);
					}

					memcpy(lv_subtitle.data, subRects->text, buf_size);
					subtitles_event_send(SUBTITLES_EVENT_SHOW, &lv_subtitle);
					printf("text[%ld]: %s\n", buf_size, subRects->text );
				}
			}
			if(subRects->type == HC_SUBTITLE_ASS){
				if(subRects->ass != NULL){
					buf_size = strlen(subRects->ass) + 1;
					if(last_buf_size < buf_size) {
						last_buf_size = buf_size;
						if (!lv_subtitle.data) {
							lv_subtitle.data = malloc(buf_size);
						} else {
							lv_subtitle.data = realloc(lv_subtitle.data, buf_size);
						}
						if (!lv_subtitle.data) {
							printf( "Error: no memory %s:%d\n", __func__ ,__LINE__);
							return ;
						}
						memset(lv_subtitle.data, 0, buf_size);
					}else{
						memset(lv_subtitle.data, 0, last_buf_size);
					}

					memcpy(lv_subtitle.data, subRects->ass, buf_size);
					subtitles_event_send(SUBTITLES_EVENT_SHOW, &lv_subtitle);
					printf("ass[%ld]: %s\n", buf_size, subRects->ass); 
				}
			}
		}
		else {
			printf("HCPLAYER_CALLBACK_SUBTITLE_ON: sub format %d\n", sub->format);
		}
	}
}

#endif

media_handle_t *media_open(media_type_t type)
{
	media_handle_t *media_hld = (media_handle_t*)malloc(sizeof(media_handle_t));

	memset(media_hld, 0, sizeof(media_handle_t));
	media_hld->type = type;
	media_hld->state = MEDIA_STOP;
	media_hld->msg_id = INVALID_ID;
	/*reload some def param (static param) in media_hld form  presenten sys param*/
	if (MEDIA_TYPE_PHOTO == type){
		media_hld->time_gap = 3000; //3 seconds interval for next slide show
	}
	media_hld->loop_type = PlAY_LIST_SEQUENCE;
	media_hld->ratio_mode = projector_get_some_sys_param(P_ASPECT_RATIO);
	media_monitor_init(media_hld);
	pthread_mutex_init(&media_hld->api_lock, NULL);
	return media_hld;
}

void media_close(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	if(m_closevp==false){
		if(media_hld->type==MEDIA_TYPE_VIDEO||media_hld->type==MEDIA_TYPE_PHOTO){
			api_dis_show_onoff(false);
		}
		if(api_media_pic_is_backup()){
			api_media_pic_backup_free();
		}
	}
	projector_set_some_sys_param(P_ASPECT_RATIO, media_hld->ratio_mode);
	media_monitor_deinit(media_hld);
	pthread_mutex_destroy(&media_hld->api_lock);
	media_hld->player = NULL;
	free((void*)media_hld);
}

int media_play(media_handle_t *media_hld, const char *media_src)
{
	ASSERT_API(media_hld && media_src);
	image_effect_t* img_effect;
	dis_zoom_t musiccover_args={0};
    HCPlayerInitArgs player_args;
    int rotate = 0 , h_flip = 0;
    rotate_type_e rotate_type = ROTATE_TYPE_0;
    mirror_type_e mirror_type = MIRROR_TYPE_NONE;

    api_get_flip_info(&rotate , &h_flip);
    rotate_type = rotate;
    mirror_type = h_flip;

	pthread_mutex_lock(&media_hld->api_lock);

	if (media_hld->state == MEDIA_PLAY){
		pthread_mutex_unlock(&media_hld->api_lock);
		return API_SUCCESS;
	}

	strncpy(media_hld->play_name, media_src, MAX_FILE_NAME-1);
	media_play_id_update(media_hld);
	/*use user_data to link with player msgs */
	printf("%s(), line:%d. play: %s.\n", __FUNCTION__, __LINE__, media_src);
	memset(&player_args, 0, sizeof(player_args));
	player_args.uri = media_src;
	player_args.msg_id = media_hld->msg_id;
	player_args.user_data =  (void*)((uint32_t)media_hld->play_id);
	player_args.sync_type = 2;
#ifdef RTOS_SUBTITLE_SUPPORT
	memset(&lv_subtitle, 0, sizeof(lv_subtitle));
	player_args.callback = subtitle_callback;
	last_buf_size = 0;
    subtitles_event_send(SUBTITLES_EVENT_HIDDEN, NULL);
	ext_subtitles_init(app_get_playlist_t());	//scan subtitle file form filelist
	ext_subtitle_t *m_subtitle =ext_subtitle_data_get();
	if(m_subtitle->ext_subs_count!=0){
		player_args.ext_subtitle_stream_num=m_subtitle->ext_subs_count;
		player_args.ext_sub_uris=m_subtitle->uris;
	}
#endif    

#ifdef CONFIG_APPS_PROJECTOR_SPDIF_OUT
	player_args.snd_devs = AUDSINK_SND_DEVBIT_SPO | AUDSINK_SND_DEVBIT_I2SO;
#endif

    if (SOUND_OUTPUT_SPEAKER == projector_get_some_sys_param(P_SOUND_OUT_MODE)){
        player_args.bypass = SPDIF_BYPASS_OFF;
    }else if (SOUND_OUTPUT_SPDIF == projector_get_some_sys_param(P_SOUND_OUT_MODE)){
        sound_spdif_mode_e spdif_mode = projector_get_some_sys_param(P_SOUND_SPDIF_MODE);
        if (SOUND_SPDIF_RAW == spdif_mode){
            player_args.bypass = SPDIF_BYPASS_RAW;
        } else if (SOUND_SPDIF_PCM == spdif_mode){
            player_args.bypass = SPDIF_BYPASS_PCM;
        } else {
        	player_args.bypass = SPDIF_BYPASS_OFF;
        }
    }

    player_args.rotate_enable = 1;
    player_args.rotate_type = rotate_type;
    player_args.mirror_type = mirror_type;
    if (api_video_pbp_get_support()){
	    player_args.pbp_mode = VIDEO_PBP_2P_ON;

        if (APP_DIS_TYPE_HD == api_video_pbp_get_dis_type(VIDEO_PBP_SRC_MP))
            player_args.dis_type = DIS_TYPE_HD;
        else
            player_args.dis_type = DIS_TYPE_UHD;

        if (APP_DIS_LAYER_MAIN == api_video_pbp_get_dis_layer(VIDEO_PBP_SRC_MP))
            player_args.dis_layer = DIS_LAYER_MAIN;
        else
            player_args.dis_layer = DIS_LAYER_AUXP;

    }

    musiccover_args = app_reset_mainlayer_param(rotate_type, mirror_type);

   if(preview_enable == MEDIA_PLAY_PREVIEW){
		if(media_hld->type==MEDIA_TYPE_PHOTO){
			player_args.img_dis_mode = IMG_DIS_THUMBNAIL;
		}else if(media_hld->type==MEDIA_TYPE_MUSIC){
			player_args.play_attached_file = 1;
		}
		player_args.preview_enable = 1;
   		memcpy(&player_args.src_area, &m_dis_rect.src_rect, sizeof(struct av_area));
		memcpy(&player_args.dst_area, &m_dis_rect.dst_rect, sizeof(struct av_area));
		media_switch_blink(true);
   	}else {
		switch(media_hld->type){
			case MEDIA_TYPE_VIDEO:
				#ifdef SYS_ZOOM_SUPPORT
				if(projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT)==0){
					player_args.preview_enable=0;
				}else{
					player_args.preview_enable=1;
					struct vdec_dis_rect new_rect = {0};
					api_get_ratio_rect(&new_rect, projector_get_some_sys_param(P_ASPECT_RATIO));
					player_args.dst_area.x = new_rect.dst_rect.x;
					player_args.dst_area.y = new_rect.dst_rect.y;
					player_args.dst_area.h = new_rect.dst_rect.h;
					player_args.dst_area.w = new_rect.dst_rect.w;
					memcpy(&player_args.src_area,&musiccover_args.src_area,sizeof(av_area_t));						
				}
				#else
					player_args.preview_enable=0;
				#endif
				player_args.play_attached_file = 1;
				/*some special video make by image and music,so show its musiccover */
				media_switch_blink(false);
				
				break;
			case MEDIA_TYPE_MUSIC:
				player_args.play_attached_file = 1;
				player_args.preview_enable=1;
				media_hld->is_whth_album=1;
				memcpy(&player_args.src_area,&musiccover_args.src_area,sizeof(av_area_t));
				memcpy(&player_args.dst_area,&musiccover_args.dst_area,sizeof(av_area_t));
				media_switch_blink(true);
				break;
			case MEDIA_TYPE_PHOTO:
				media_switch_blink(false);
				player_args.img_dis_mode = m_play_config.dis_mode;//IMG_DIS_AUTO;

			#ifdef SYS_ZOOM_SUPPORT
				if(projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT)==0 && 
				 	projector_get_some_sys_param(P_SYS_ZOOM_DIS_MODE) == DIS_TV_AUTO){
				 	player_args.preview_enable=0;
				}else{
				 	player_args.preview_enable=1;
				}
				player_args.dst_area.x = get_display_x();
				player_args.dst_area.y = get_display_y();
				player_args.dst_area.h = get_display_v();
				player_args.dst_area.w = get_display_h();
				memcpy(&player_args.src_area,&musiccover_args.src_area,sizeof(av_area_t));	
			#else
				player_args.preview_enable=0;
			#endif

				player_args.img_dis_hold_time = media_hld->time_gap;
				player_args.gif_dis_interval = 50;
				player_args.img_alpha_mode = 0;	
				
				img_effect=mpimage_effect_param_get();
				media_hld->pic_effect_mode=img_effect->mode;
				if (img_effect->mode != IMG_SHOW_NULL) {
					memcpy(&player_args.img_effect, img_effect, sizeof(image_effect_t));
				}
				
				break;
			default :
				break;
		} 
   	}
	hcplayer_init(LOG_WARNING);
   	media_hld->player = hcplayer_create(&player_args);
    if (!media_hld->player){
        printf("hcplayer_create() fail!\n");
        pthread_mutex_unlock(&media_hld->api_lock);
        return API_FAILURE;
    }
    hcplayer_play(media_hld->player);

    media_hld->state = MEDIA_PLAY;
    media_hld->speed = 0;
    media_hld->rotate = rotate_type;
    media_hld->is_double_urls = false;
    // mainlayer rotate is closewise
    pthread_mutex_unlock(&media_hld->api_lock);
    return API_SUCCESS;
}

int media_stop(media_handle_t *media_hld)
{
	int ret = -1;
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);

	if (!media_hld->player || media_hld->state == MEDIA_STOP){
		pthread_mutex_unlock(&media_hld->api_lock);
		return API_FAILURE;
	}
	hcplayer_stop2(media_hld->player, m_closevp, m_fillblack);
	media_hld->player = NULL;

	if(m_closevp==false){
		if(media_hld->type==MEDIA_TYPE_VIDEO){
			api_pic_effect_enable(false);
			/*if the video consists of pics,so do it like pic*/ 
			ret = api_media_pic_backup(60);
			if(ret < 0){
				api_dis_show_onoff(false);
			}
		}else if(media_hld->type==MEDIA_TYPE_PHOTO){
			if(media_hld->pic_effect_mode == IMG_SHOW_NULL){
				api_pic_effect_enable(false);
				ret = api_media_pic_backup(60);
				if(ret < 0){
					api_dis_show_onoff(false);
					/* dis backup pic fail, close de output to avoid mosaic */
				}
			}else if(media_hld->pic_effect_mode!=IMG_SHOW_NULL&& ((media_hld->media_do_opt&DO_ROTATE_OPT)==DO_ROTATE_OPT)){
				// pic effect do when rotate need to backup,
				ret = api_media_pic_backup(60);
				if(ret < 0){
					api_dis_show_onoff(false);
				}
				media_hld->media_do_opt&=~(DO_ROTATE_OPT);
			}
		}
	}
	/*if it has zoom size do reset /dev/dis zoom size
		and then reset some handle params  */ 
	if((media_hld->media_do_opt&DO_ZOOM_OPT)==DO_ZOOM_OPT){
		/*when do zoom opt after Vertical diagram , it willstretch diagram 
		* then reset display zoom reset , so do not show it process */
		api_dis_show_onoff(false);
		media_display_zoom_reset(media_hld);
	} else {
		if(media_hld->type==MEDIA_TYPE_PHOTO){
		    //reset dis zoom, for the image decoder may change the dis zoom.
		    int ratio = projector_get_some_sys_param(P_ASPECT_RATIO);
		    api_set_display_area(ratio);
		}
	}
	/*due to music cover play in /dev/dis mainlayer and 
	set preview area. so reset zoom area  */ 
	if(media_hld->type==MEDIA_TYPE_MUSIC &&media_hld->is_whth_album==1){
		media_display_zoom_reset(media_hld);
		media_hld->is_whth_album=0;
	}
	media_hld->state = MEDIA_STOP;
	media_hld->speed = 0;
	media_hld->seek_step=0;
	media_hld->frame_state = FIRST_VIDEO_FRAME_INVALID;
#ifdef RTOS_SUBTITLE_SUPPORT	
	if(lv_subtitle.data){
	    free(lv_subtitle.data);
	    lv_subtitle.data = NULL;
    }
	printf("media stop111\n");
	subtitles_event_send(SUBTITLES_EVENT_CLOSE, NULL);
	ext_subtitle_deinit();
#endif    
	pthread_mutex_unlock(&media_hld->api_lock);
	return API_SUCCESS;
}

int media_pause(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);

	pthread_mutex_lock(&media_hld->api_lock);
	if (!media_hld->player){
		pthread_mutex_unlock(&media_hld->api_lock);
		return API_FAILURE;
	}
	if(media_hld->is_double_urls){
		hcplayer_multi_pause(media_hld->player);
	}else{
		hcplayer_pause(media_hld->player);
	}
	media_hld->state = MEDIA_PAUSE;
	media_hld->speed = 0;
	media_hld->seek_step=0;
	#ifdef RTOS_SUBTITLE_SUPPORT
		subtitles_event_send(SUBTITLES_EVENT_PAUSE, NULL);
	#endif
	pthread_mutex_unlock(&media_hld->api_lock);
	return API_SUCCESS;
}

int media_resume(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);
	if (!media_hld->player){
		pthread_mutex_unlock(&media_hld->api_lock);
		return API_FAILURE;
	}
	if(media_hld->is_double_urls){
		hcplayer_multi_play(media_hld->player);
	}else{
		hcplayer_resume(media_hld->player);
	}
	media_hld->state = MEDIA_PLAY;
	media_hld->speed = 0;
	media_hld->seek_step=0;
	#ifdef RTOS_SUBTITLE_SUPPORT
		subtitles_event_send(SUBTITLES_EVENT_RESUME, NULL);
	#endif
	pthread_mutex_unlock(&media_hld->api_lock);
	return API_SUCCESS;
}

int media_seek(media_handle_t *media_hld, int time_sec)
{
	ASSERT_API(media_hld);

	pthread_mutex_lock(&media_hld->api_lock);
	if (!media_hld->player){
		pthread_mutex_unlock(&media_hld->api_lock);
		return API_FAILURE;
	}
	if(media_hld->is_double_urls){
		hcplayer_multi_seek(media_hld->player, time_sec * 1000);
	}else{
		hcplayer_seek(media_hld->player, time_sec * 1000);
	}
	media_hld->state = MEDIA_PLAY;
	media_hld->speed = 0;
	pthread_mutex_unlock(&media_hld->api_lock);
	return API_SUCCESS;
}

//1x, 2x, 4x, 8x, 16x, 24x, 32x
int media_fastforward(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	int speed;
	int speed_cnt;

	pthread_mutex_lock(&media_hld->api_lock);

	if (!media_hld->player){
		pthread_mutex_unlock(&media_hld->api_lock);
		return API_FAILURE;
	}

	speed_cnt = sizeof(m_ff_speed)/sizeof(m_ff_speed[0]);

	if (media_hld->state != MEDIA_FF){
		speed = 1;
	}else {
		speed = media_hld->speed;
		speed += 1;
		speed = speed % speed_cnt;
	}
	printf("%s(), line:%d. speed: %d\n", __FUNCTION__, __LINE__, m_ff_speed[speed]);
	hcplayer_set_speed_rate(media_hld->player, m_ff_speed[speed]);
	media_hld->speed = speed;
	if (0 == speed) //normal play
		media_hld->state = MEDIA_PLAY;
	else
		media_hld->state = MEDIA_FF;

	pthread_mutex_unlock(&media_hld->api_lock);

	return API_SUCCESS;
}

//1x, -2x, -4x, -8x, -16x, -24x, -32x
int media_fastbackward(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	int speed;
	int speed_cnt;

	pthread_mutex_lock(&media_hld->api_lock);
	if (!media_hld->player){
		pthread_mutex_unlock(&media_hld->api_lock);
		return API_FAILURE;
	}

	speed_cnt = sizeof(m_fb_speed)/sizeof(m_fb_speed[0]);

	if (media_hld->state != MEDIA_FB){
		speed = 1;	
	}else{
		speed = media_hld->speed;
		speed += 1;
		speed = speed % speed_cnt;
	}
	printf("%s(), line:%d. speed: %d\n", __FUNCTION__, __LINE__, m_fb_speed[speed]);
	hcplayer_set_speed_rate(media_hld->player, m_fb_speed[speed]);
	media_hld->speed = speed;
	if (0 == speed) //normal play
		media_hld->state = MEDIA_PLAY;
	else
		media_hld->state = MEDIA_FB;

	pthread_mutex_unlock(&media_hld->api_lock);

	return API_SUCCESS;
}

//1x, 1/2, 1/4, 1/8, 1/16, 1/24
int media_slowforward(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	int speed;
	int speed_cnt;

	pthread_mutex_lock(&media_hld->api_lock);

	if (!media_hld->player){
		pthread_mutex_unlock(&media_hld->api_lock);
		return API_FAILURE;
	}

	speed_cnt = sizeof(m_sf_speed)/sizeof(m_sf_speed[0]);

	if (media_hld->state != MEDIA_SF){
		speed = 1;
	} else {
		speed = media_hld->speed;
		speed += 1;
		speed = speed % speed_cnt;
	}
	printf("%s(), line:%d. speed: %f\n", __FUNCTION__, __LINE__, (double)m_sf_speed[speed]);
	hcplayer_set_speed_rate(media_hld->player, m_sf_speed[speed]);
	media_hld->speed = speed;
	if (0 == speed) //normal play
		media_hld->state = MEDIA_PLAY;
	else
		media_hld->state = MEDIA_SF;


	pthread_mutex_unlock(&media_hld->api_lock);

	return API_SUCCESS;
}

//1x, -1/2, -1/4, -1/8, -1/16, -1/24
int media_slowbackward(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	int speed;
	int speed_cnt;

	pthread_mutex_lock(&media_hld->api_lock);

	if (!media_hld->player){
		pthread_mutex_unlock(&media_hld->api_lock);
		return API_FAILURE;
	}

	speed_cnt = sizeof(m_sb_speed)/sizeof(m_sb_speed[0]);

	if (media_hld->state != MEDIA_SB){
		speed = 1;	
	}else{
		speed = media_hld->speed;
		speed += 1;
		speed = speed % speed_cnt;
	}
	printf("%s(), line:%d. speed: %f\n", __FUNCTION__, __LINE__, (double)m_sb_speed[speed]);
	hcplayer_set_speed_rate(media_hld->player, m_sb_speed[speed]);
	media_hld->speed = speed;
	if (0 == speed) //normal play
		media_hld->state = MEDIA_PLAY;
	else
		media_hld->state = MEDIA_SB;


	pthread_mutex_unlock(&media_hld->api_lock);

	return API_SUCCESS;
}

uint32_t media_get_playtime(media_handle_t *media_hld)
{
	uint32_t play_time;
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);
	if (!media_hld->player){
		pthread_mutex_unlock(&media_hld->api_lock);
		return 0;
	}
	if(media_hld->is_double_urls){
		play_time = (uint32_t)hcplayer_multi_position(media_hld->player);
	}else{
		play_time = (uint32_t)hcplayer_get_position(media_hld->player);
	}
	if ((int)play_time < 0){
		play_time = media_hld->play_time;
		pthread_mutex_unlock(&media_hld->api_lock);
		return play_time;
	}
	play_time = play_time/1000;
	media_hld->play_time = play_time;
	// printf("play time %ld\n", play_time);
	pthread_mutex_unlock(&media_hld->api_lock);
	return play_time;
}

uint32_t media_get_totaltime(media_handle_t *media_hld)
{
	uint32_t total_time;
	ASSERT_API(media_hld);
	if (!media_hld->player)
		return media_hld->total_time;
	if(media_hld->is_double_urls){
		total_time=(uint32_t)(hcplayer_multi_duration(media_hld->player)/1000);
	}else{
		total_time = (uint32_t)(hcplayer_get_duration(media_hld->player)/1000);
	}
	media_hld->total_time = total_time;
	return total_time;
}


media_state_t media_get_state(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	media_state_t temp_state;
	pthread_mutex_lock(&media_hld->api_lock);
	temp_state=media_hld->state;
	pthread_mutex_unlock(&media_hld->api_lock);
	return temp_state;
}

uint8_t media_get_speed(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	return media_hld->speed;
}

char *media_get_cur_play_file(media_handle_t *media_hld)
{
	ASSERT_API(media_hld);
	return media_hld->play_name;
}

int media_get_info(media_handle_t *media_hld,mp_info_t* mp_info)
{
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);
	if (media_hld->player) 
	{
		mp_info->filesize = (float)hcplayer_get_filesize(media_hld->player)*1.0/1024.0/1024.0; // size unit MB 
		hcplayer_get_cur_audio_stream_info(media_hld->player, &mp_info->audio_info);
		hcplayer_get_cur_video_stream_info(media_hld->player, &mp_info->video_info);
		hcplayer_get_cur_subtitle_stream_info(media_hld->player, &mp_info->subtitle_info);
		hcplayer_get_media_info(media_hld->player, &mp_info->media_info);
		mp_info->audio_tracks_count=hcplayer_get_audio_streams_count(media_hld->player);
		mp_info->subtitles_count=hcplayer_get_subtitle_streams_count(media_hld->player);
		stat(media_hld->play_name,&mp_info->stat_buf);//get date form vfs, "stat" is LINUX C FUNC 
	}
	pthread_mutex_unlock(&media_hld->api_lock);
	return 0;
}


/**
 * @description: set bcaklight 
 * @param {bool} val 1 -> backlight is on  ,0 -> backlight is off 
 * @return {*}
 */
int app_set_blacklight(bool val)
{
	int ret = 0;
	if(val == 1){
#ifdef BACKLIGHT_MONITOR_SUPPORT
		api_pwm_backlight_monitor_update_status();
#else		
		ret = api_set_backlight_brightness(projector_get_some_sys_param(P_BACKLIGHT));
#endif		
		api_hotkey_disable_clear();
	}
	else{
		ret = api_set_backlight_brightness(0);
		api_hotkey_disable_all();
	}


	return ret;
}
/**
 * @description: use to blink or not when media_player to switch pic or video 
 * @param {bool} blink-> true  not blink ->false
 * @return {*}
 * @author: Yanisin
 */
int media_switch_blink(bool blink){
	m_closevp=	blink;
	return 0;
}
/*change pic rotate ,input pararm is rotate dire*/
int media_change_rotate_type(media_handle_t* media_hld,rotate_direction_e type)
{
	ASSERT_API(media_hld);
	api_media_pic_backup(300);
	// do backup when rotate can avoid mosaic
	if((media_hld->zoom_param && media_hld->zoom_param->zoom_size != ZOOM_NORMAL) 
	#ifdef SYS_ZOOM_SUPPORT
		|| app_has_zoom_operation_get()
	#endif
		)
	{
		/* reset media_display zoom when do rotate,onoff dis_show
		* then it can aviod Show the stretching process */
		api_dis_show_onoff(false);
		media_display_zoom_reset(media_hld);
	}
	pthread_mutex_lock(&media_hld->api_lock);
	if (!media_hld->player){
		pthread_mutex_unlock(&media_hld->api_lock);
		return 0;
	}
	if(type==CLOCKWISE){
		switch (media_hld->rotate){
			case ROTATE_TYPE_0:
				media_hld->rotate=ROTATE_TYPE_90;
				break;
			case ROTATE_TYPE_90:
				media_hld->rotate=ROTATE_TYPE_180;
				break;
			case ROTATE_TYPE_180:
				media_hld->rotate=ROTATE_TYPE_270;
				break;
			case ROTATE_TYPE_270:
				media_hld->rotate=ROTATE_TYPE_0;
				break;
		}
	}else{
		switch (media_hld->rotate){
			case ROTATE_TYPE_0:
				media_hld->rotate=ROTATE_TYPE_270;
				break;
			case ROTATE_TYPE_90:
				media_hld->rotate=ROTATE_TYPE_0;
				break;
			case ROTATE_TYPE_180:
				media_hld->rotate=ROTATE_TYPE_90;
				break;
			case ROTATE_TYPE_270:
				media_hld->rotate=ROTATE_TYPE_180;
				break;
		}
	}
	media_hld->media_do_opt|=DO_ROTATE_OPT;
	hcplayer_change_rotate_type(media_hld->player,media_hld->rotate);
	pthread_mutex_unlock(&media_hld->api_lock);
	return 0;
}
/* different parameter for api media_change_rotate_type */
int media_change_rotate_mirror_type(media_handle_t* media_hld,rotate_type_e rotate_type,mirror_type_e mirror_type)
{
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);
	if (!media_hld->player){
		pthread_mutex_unlock(&media_hld->api_lock);
		return 0;
	}
	media_hld->media_do_opt|=DO_ROTATE_OPT;
	media_hld->rotate=rotate_type;
	
	if (hcplayer_change_rotate_mirror_type(media_hld->player,rotate_type,mirror_type) < 0){
		uint32_t msg_type = HCPLAYER_MSG_VIDEO_DECODE_ERR;
		uint32_t ctlmsg_code = msg_type <<16 | (uint32_t)media_hld->play_id;
		api_control_send_media_message(ctlmsg_code);
	}

	pthread_mutex_unlock(&media_hld->api_lock);
	return 0;
}




// handle for multi urls to hcplayer 
int media_play_with_double_url(media_handle_t* media_hld,media_urls_t* media_urls)
{
    ASSERT_API(media_hld && media_urls);
    HCPlayerInitArgs video_initargs;
    HCPlayerInitArgs audio_initargs;
    media_play_id_update(media_hld);
    memset(&video_initargs, 0, sizeof(HCPlayerInitArgs));
    memset(&audio_initargs, 0, sizeof(HCPlayerInitArgs));
    pthread_mutex_lock(&media_hld->api_lock);
    int rotate = 0 , h_flip = 0;
    rotate_type_e rotate_type = ROTATE_TYPE_0;
    mirror_type_e mirror_type = MIRROR_TYPE_NONE;
    api_get_flip_info(&rotate , &h_flip);
    rotate_type = rotate;
    mirror_type = h_flip;
    video_initargs.uri=media_urls->url1;
    video_initargs.sync_type = 2;
    video_initargs.msg_id=media_hld->msg_id;
    video_initargs.user_data = (void *)((uint32_t)media_hld->play_id);
    video_initargs.rotate_enable = 1;
    video_initargs.rotate_type = rotate_type;
    video_initargs.mirror_type = mirror_type;

#ifdef SYS_ZOOM_SUPPORT
    if(projector_get_some_sys_param(P_SYS_ZOOM_OUT_COUNT)==0 && 
        projector_get_some_sys_param(P_SYS_ZOOM_DIS_MODE) == DIS_TV_AUTO){
        video_initargs.preview_enable=0;
    }else{
        video_initargs.preview_enable=1;
    }
    video_initargs.src_area.x = DIS_ZOOM_FULL_X;
    video_initargs.src_area.y = DIS_ZOOM_FULL_Y;
    video_initargs.src_area.w = DIS_ZOOM_FULL_W;
    video_initargs.src_area.h = DIS_ZOOM_FULL_H;
    video_initargs.dst_area.x = get_display_x();
    video_initargs.dst_area.y = get_display_y();
    video_initargs.dst_area.h = get_display_v();
    video_initargs.dst_area.w = get_display_h();
#else
    video_initargs.preview_enable=0;
#endif

    audio_initargs.uri=media_urls->url2;
    audio_initargs.sync_type = 2;
    audio_initargs.msg_id=media_hld->msg_id;
    audio_initargs.user_data = (void *)((uint32_t)media_hld->play_id);
    media_hld->player=hcplayer_multi_create(&audio_initargs, &video_initargs);
    if (!media_hld->player){
        printf("hcplayer_multi_create() fail!\n");
        pthread_mutex_unlock(&media_hld->api_lock);
        return API_FAILURE;
    }
    hcplayer_multi_play(media_hld->player);

    media_hld->state = MEDIA_PLAY;
    media_hld->speed = 0;
    media_hld->is_double_urls=true;
    pthread_mutex_unlock(&media_hld->api_lock);
    return API_SUCCESS;
}


int media_stop_with_double_url(media_handle_t* media_hld)
{
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);
	hcplayer_multi_destroy(media_hld->player);
	media_hld->player = NULL;
	media_hld->state = MEDIA_STOP;
	media_hld->speed = 0;
	pthread_mutex_unlock(&media_hld->api_lock);
	return API_SUCCESS;
}

int media_func_callback_register(media_handle_t* media_hld,mp_func_callback mp_func)
{
	ASSERT_API(media_hld);
	pthread_mutex_lock(&media_hld->api_lock);
	if(media_hld->mp_func_cb==NULL)
		media_hld->mp_func_cb=mp_func;
	pthread_mutex_unlock(&media_hld->api_lock);
	return API_SUCCESS;
}

/*use a func cb to get next player urls */ 
int media_play_next_program(media_handle_t* media_hld)
{
	ASSERT_API(media_hld);
	if(media_hld->mp_func_cb){
		if(media_hld->player){
			if(media_hld->is_double_urls==true){
				media_stop_with_double_url(media_hld);
			}else{
				media_stop(media_hld);
			}
		}
		media_urls_t media_urls={0};
		media_hld->mp_func_cb("next",(void*)&media_urls);		//form this func to get next urls  
		if(media_urls.url1!=NULL||media_urls.url2!=NULL){
			if(media_urls.is_double_urls==true){
				media_play_with_double_url(media_hld,&media_urls);
				printf(">>! v_url: %s\n",media_urls.url1);
				printf(">>! a_url: %s\n",media_urls.url2);
			}else{
				media_play(media_hld,media_urls.url1);
			}
		}
	}
	return API_SUCCESS;
}

int media_play_prev_program(media_handle_t * media_hld)
{
	ASSERT_API(media_hld);
	if(media_hld->mp_func_cb){
		if(media_hld->player){
			if(media_hld->is_double_urls==true){
				media_stop_with_double_url(media_hld);
			}else{
				media_stop(media_hld);
			}
		}
		media_urls_t media_urls={0};
		media_hld->mp_func_cb("prev",(void*)&media_urls);		//form this func to get next urls  
		if(media_urls.url1!=NULL||media_urls.url2!=NULL){
			if(media_urls.is_double_urls==true){
				media_play_with_double_url(media_hld,&media_urls);
				printf(">>! v_url: %s\n",media_urls.url1);
				printf(">>! a_url: %s\n",media_urls.url2);
			}else{
				media_play(media_hld,media_urls.url1);
			}
		}
	}
	return API_SUCCESS;
}

int media_play_set_img_dis_mode(img_dis_mode_e dis_mode)
{
	printf("%s(), dis_mode: %d\n", __func__, dis_mode);
	m_play_config.dis_mode = dis_mode;
}


/**
 * @description: due to hcplayer has not ff/fb when play music,so auto seek by user
 *  to simulation ff/fb func
 * @param {media_state_t} set_state
 * @return {*}
 * @author: Yanisin
 */
int auto_seek_step[] = {0, 2, 4, 8, 16, 24, 32};
int media_manual_ffsetting(media_handle_t * media_hld)
{
	int speed;
	int speed_cnt; 
    speed_cnt = sizeof(auto_seek_step)/sizeof(auto_seek_step[0]);
    if (media_hld->state != MEDIA_FF){
		speed = 1;
	}else {
		speed = media_hld->speed;
		speed += 1;
		speed = speed % speed_cnt;
	} 
	/*state machine by speed var*/  
	media_hld->seek_step=auto_seek_step[speed];
	media_hld->speed=speed;
	if(media_hld->seek_step){
		media_hld->state = MEDIA_FF;
	}else{
		media_hld->state=MEDIA_PLAY;
	}    return 0;
}

int media_manual_fbsetting(media_handle_t * media_hld)
{
	int speed;
	int speed_cnt; 
    speed_cnt = sizeof(auto_seek_step)/sizeof(auto_seek_step[0]);
    if (media_hld->state != MEDIA_FB){
		speed = 1;
	}else {
		speed = media_hld->speed;
		speed += 1;
		speed = speed % speed_cnt;
	} 
	/*state machine by speed var*/  
	media_hld->seek_step=-auto_seek_step[speed];
	media_hld->speed=speed;
	if(media_hld->seek_step){
		media_hld->state = MEDIA_FB;
	}else{
		media_hld->state=MEDIA_PLAY;
	}
    return 0;
}

void media_manual_seek_end(media_handle_t * media_hld)
{
    media_hld->seek_step=0;
	media_hld->speed=0;
}

int media_manual_seekopt(media_handle_t * media_hld)
{
	if(media_hld->seek_step){
		int32_t m_tims=media_get_playtime(media_hld);//unit s
		int32_t total_tims=media_get_totaltime(media_hld);//unit s
		if(m_tims>=total_tims)
			return 0; 
		int64_t tims_ms=m_tims+media_hld->seek_step;//need seek time
		if(tims_ms<=0){
			tims_ms=0;
			media_manual_seek_end(media_hld);
		}else{
			if(tims_ms>=total_tims)
				tims_ms=total_tims;
			tims_ms=tims_ms*1000;
		}
		hcplayer_seek(media_hld->player,tims_ms); //unit ms
	}
	return 0;
}
/**
 * @description: operate /dev/dis to realize zoom ,use this apis when press zoom btn 
 * on ctrlbar. zoom func depend on mp_zoom.c  
 * @param {media_handle_t *} media_hld
 * @author: Yanisin
 */
int meida_display_zoom(media_handle_t * media_hld,Zoom_mode_e zoom_mode)
{
	int ret = -1 ;
	pthread_mutex_lock(&media_hld->api_lock);
	if((media_hld->frame_state & FIRST_VIDEO_FRAME_SHOWED) == FIRST_VIDEO_FRAME_SHOWED){
		if(media_hld->type==MEDIA_TYPE_VIDEO){
			/* In general,display_aspect setting DIS_TV_16_9,DIS_PILLBOX in video
			setting DIS_TV_AUTO,DIS_NORMAL_SCALE if it has picture, if it do zoom
			opt setting DIS_TV_AUTO DIS_NORMAL_SCALE.							*/
			api_set_display_aspect(DIS_TV_16_9,DIS_NORMAL_SCALE);
		}
		//Use VE view method in play state
		app_set_diszoom(zoom_mode,  (media_hld->state != MEDIA_PAUSE) && (media_hld->state != MEDIA_STOP)); 
		Zoom_Param_t * zoom_args=app_get_zoom_param();
		media_hld->zoom_param = zoom_args;
		media_hld->media_do_opt|=DO_ZOOM_OPT;
		ret = API_SUCCESS;
	}
	pthread_mutex_unlock(&media_hld->api_lock);
	return ret;
}
/*do it after it call meida stop*/ 
void media_display_zoom_reset(media_handle_t * media_hld)
{
	if(media_hld->type==MEDIA_TYPE_VIDEO){
		/*reset display_aspect before it zoom opt*/ 
		//api_set_display_aspect(projector_get_some_sys_param(P_ASPECT_RATIO),DIS_PILLBOX);
		app_reset_diszoom_param_by_ratio(media_hld->ratio_mode);
	}else{
		app_reset_diszoom_param();
	}
	media_hld->zoom_param = NULL;
	/*reset area and zoom param,reset bit */
	media_hld->media_do_opt&=~(DO_ZOOM_OPT);
}

/* sys_scale need set display zoom area and reset media_hld member 
 * when media_player change display zoom. 
 * */
void media_display_sys_scale_reset(media_handle_t* media_hld)
{
	if(media_hld->type==MEDIA_TYPE_VIDEO){
		app_reset_diszoom_param_by_ratio(media_hld->ratio_mode);
	}else{
		app_reset_diszoom_default_param_by_sysscale();
	}
	media_hld->zoom_param = NULL;
	/*reset area and zoom param,reset bit */
	media_hld->media_do_opt&=~(DO_ZOOM_OPT);
}

int media_play_id_update(media_handle_t* media_hld)
{
	ASSERT_API(media_hld);
	media_hld->play_id++;
	return API_SUCCESS;
}

/*
 * Use ve to set display rect_area ,it only work when video playing 
 * it work on next frame play when set param in video pause
 * */
void media_set_display_rect(media_handle_t *media_hld , struct vdec_dis_rect *new_rect)
{
    ASSERT_API(media_hld);
    hcplayer_set_display_rect(media_hld->player, new_rect);
}
int media_looptype_state_set(media_handle_t* media_hld,int looptype)
{
	ASSERT_API(media_hld);
	media_hld->loop_type = looptype; 
	return API_SUCCESS;
}
/*
 * media_display_ratio_set 16:9 /4:3 /auto before ratio set ,reset zoom
 * if it do zoom opt 
 * */
int media_display_ratio_set(media_handle_t* media_hld, int tv_mode)
{
	ASSERT_API(media_hld);
	if(media_hld->zoom_param && media_hld->zoom_param->zoom_size != ZOOM_NORMAL){
		media_display_zoom_reset(media_hld);
	}
	media_hld->ratio_mode = tv_mode;
	api_set_display_area(tv_mode);
	return API_SUCCESS;
}
void media_set_display_rect_by_ratio(media_handle_t* media_hld, dis_tv_mode_e ratio)//void set_aspect_ratio(dis_tv_mode_e ratio)
{
   struct dis_aspect_mode aspect = { 0 };
   struct vdec_dis_rect rect = {0};
   int fd = open("/dev/dis" , O_WRONLY);
    if(fd < 0) {
        return;
    }
 
    aspect.distype = DIS_TYPE_HD;
    aspect.tv_mode = DIS_TV_16_9;
    aspect.dis_mode = DIS_NORMAL_SCALE;
    if(ratio == DIS_TV_AUTO)
    {
        aspect.dis_mode = DIS_PILLBOX;
    }

    if(media_hld){
	    api_get_ratio_rect(&rect, ratio);
	    media_set_display_rect(media_hld, &rect);
	    usleep(30*1000);    	
    }

    ioctl(fd , DIS_SET_ASPECT_MODE, &aspect);
    close(fd);
}

