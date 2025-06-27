/**
 * media_player.c. The file is for media player, the play action include
 * stop/play/
 */
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
#include "screen.h"
#include "mp_fspage.h"
#include "local_mp_ui.h"
#include "mp_playerpage.h"
#include <hcuapi/audsink.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/lvds.h>
#include "ffplayer_manager.h"

static bool m_closevp = false, m_fillblack = false;
static vdec_dis_rect_t m_dis_rect;// used for preview/normal
typedef struct
{
    volatile img_dis_mode_e dis_mode;
} play_cfg_s;

static play_cfg_s m_play_config =
{
    .dis_mode = IMG_DIS_AUTO,
};

static void *media_monitor_task(void *arg);
static int media_monitor_deinit(media_handle_t *media_hld);
static int media_monitor_init(media_handle_t *media_hld);
static void media_msg_proc(media_handle_t *media_hld, HCPlayerMsg *msg);

static void media_msg_proc(media_handle_t *media_hld, HCPlayerMsg *msg)
{
    if (!media_hld || !msg)
        return;
    if (media_hld->play_id != (uint32_t)msg->user_data)
    {
        /*do cmp can avoid lot of msg send after it do next*/
        return;
    }
    uint32_t ctlmsg_code = msg->type << 16 | (uint32_t)msg->user_data;
    switch (msg->type)
    {
        case HCPLAYER_MSG_STATE_EOS:
            printf(">> app get eos, normal play end!\n");
            media_hld->state = MEDIA_PLAY_END;
            break;
        case HCPLAYER_MSG_STATE_TRICK_EOS:
            printf(">> app get trick eos, fast play to end\n");
            media_hld->state = MEDIA_PLAY_END;
            break;
        case HCPLAYER_MSG_STATE_TRICK_BOS:
            printf(">> app get trick bos, fast back play to begining!\n");
            break;
        case HCPLAYER_MSG_OPEN_FILE_FAILED:
        case HCPLAYER_MSG_ERR_UNDEFINED:
        case HCPLAYER_MSG_UNSUPPORT_FORMAT:
            printf(">> open file fail\n");
            if (m_closevp == false && media_hld->type != MEDIA_TYPE_MUSIC)
            {
                api_dis_show_onoff(false);
                //Beacause of video/pic frame backup,therefore close last frame if next frame can not play
            }
            break;
        case HCPLAYER_MSG_BUFFERING:
            printf(">> buffering %d\n", msg->val);
            ctlmsg_code = (HCPLAYER_MSG_BUFFING_FLAG | msg->val) << 16 | (uint32_t)msg->user_data;
            break;
        case HCPLAYER_MSG_STATE_PLAYING:
            printf(">> player playing\n");
            break;
        case HCPLAYER_MSG_STATE_PAUSED:
            printf(">> player paused\n");
            break;
        case HCPLAYER_MSG_STATE_READY:
            printf(">> player ready\n");
            break;
        case HCPLAYER_MSG_READ_TIMEOUT:
            printf(">> player read timeout\n");
            break;
        case HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO:
            printf(">>Audio Track Unsupport\n");
            break;
        case HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO:
            printf(">>Video Track Unsupport\n");
            if (m_closevp == false && media_hld->type != MEDIA_TYPE_MUSIC)
            {
                api_dis_show_onoff(false);
                //Beacause of video frame backup,therefore close last frame if next frame can not play
            }
            break;
        case HCPLAYER_MSG_UNSUPPORT_VIDEO_TYPE:
        {
            //this case mean player has not this type's decode and player will
            //change to next one.if player's all type decode not support send unspport all video
            HCPlayerVideoInfo video_info;
            char *video_type = "unknow";
            if (media_hld->player && !hcplayer_get_nth_video_stream_info(media_hld->player, msg->val, &video_info))
            {
                /* only a simple sample, app developers use a static struct to mapping them. */
                if (video_info.codec_id == HC_AVCODEC_ID_HEVC)
                {
                    video_type = "h265";
                }
            }
            printf(">>Unsupport Video Type:%s\n", video_type);
            break;
        }
        case HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE:
        {
            HCPlayerAudioInfo audio_info;
            char *audio_type = "unknow";
            //msg->val mean audio track here
            if (media_hld->player && !hcplayer_get_nth_audio_stream_info(media_hld->player, msg->val, &audio_info))
            {
                /* only a simple sample, app developers use a static struct to mapping them. */
                if (audio_info.codec_id < 0x11000)
                {
                    audio_type = "pcm";
                }
                else if (audio_info.codec_id < 0x12000)
                {
                    audio_type = "adpcm";
                }
                else if (audio_info.codec_id == HC_AVCODEC_ID_DTS)
                {
                    audio_type = "dts";
                }
                else if (audio_info.codec_id == HC_AVCODEC_ID_EAC3)
                {
                    audio_type = "eac3";
                }
                else if (audio_info.codec_id == HC_AVCODEC_ID_APE)
                {
                    audio_type = "ape";
                }
            }
            printf(">>Unsupport Audio Type:%s\n", audio_type);
            break;
        }
        case HCPLAYER_MSG_AUDIO_DECODE_ERR:
            //this case mean player has this decode but decode err
            printf(">>audio dec err, audio idx %d\n", msg->val);
            /* check if it is the last audio track, if not, then change to next one. */
            if (media_hld->player)
            {
                int total_audio_num = -1;
                total_audio_num = hcplayer_get_audio_streams_count(media_hld->player);
                if (msg->val >= 0 && total_audio_num > (msg->val + 1))
                {
                    HCPlayerAudioInfo audio_info;
                    if (!hcplayer_get_cur_audio_stream_info(media_hld->player, &audio_info))
                    {
                        if (audio_info.index == msg->val)
                        {
                            int idx = audio_info.index + 1;
                            while (hcplayer_change_audio_track(media_hld->player, idx))
                            {
                                idx++;
                                if (idx >= total_audio_num)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    hcplayer_change_audio_track(media_hld->player, -1);
                }
            }
            break;
        case HCPLAYER_MSG_VIDEO_DECODE_ERR:
        {
            //this case mean player has this decode but decode err
            printf("video dec err, video idx %d\n", msg->val);
            /* check if it is the last video track, if not, then change to next one. */
            if (media_hld->player)
            {
                int total_video_num = -1;
                total_video_num = hcplayer_get_video_streams_count(media_hld->player);
                if (msg->val >= 0 && total_video_num > (msg->val + 1))
                {
                    HCPlayerVideoInfo video_info;
                    if (!hcplayer_get_cur_video_stream_info(media_hld->player, &video_info))
                    {
                        if (video_info.index == msg->val)
                        {
                            int idx = video_info.index + 1;
                            while (hcplayer_change_video_track(media_hld->player, idx))
                            {
                                idx++;
                                if (idx >= total_video_num)
                                {
                                    if (m_closevp == false && media_hld->type != MEDIA_TYPE_MUSIC)
                                    {
                                        api_dis_show_onoff(false);
                                        /*Beacause of video frame backup,therefore close last frame
                                          if next frame can not play*/
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    if (m_closevp == false && media_hld->type != MEDIA_TYPE_MUSIC)
                    {
                        api_dis_show_onoff(false);
                        /*Beacause of video frame backup,therefore close last frame
                            if next frame can not play*/
                    }
                    hcplayer_change_video_track(media_hld->player, -1);
                }
            }
            break;
        }
        default:
            break;
    }
}

static void *media_monitor_task(void *arg)
{
    HCPlayerMsg msg;
    media_handle_t *media_hld = (media_handle_t *)arg;
    while (!media_hld->exit)
    {
        do
        {
            if (xQueueReceive((QueueHandle_t)media_hld->msg_id, (void *)&msg, -1) != pdPASS)
            {
                break;
            }
            pthread_mutex_lock(&media_hld->api_lock);
            if (media_hld->player)
                media_msg_proc(media_hld, &msg);
            pthread_mutex_unlock(&media_hld->api_lock);
        }
        while (0);
        api_sleep_ms(10);
    }
    pthread_mutex_lock(&media_hld->msg_task_mutex);
    pthread_cond_signal(&media_hld->msg_task_cond);
    pthread_mutex_unlock(&media_hld->msg_task_mutex);

    printf("exit media_monitor_task()\n");
    return NULL;
}

static int media_monitor_init(media_handle_t *media_hld)
{
    pthread_t thread_id = 0;
    pthread_attr_t attr;

    if (INVALID_ID != media_hld->msg_id)
        return API_SUCCESS;

    media_hld->msg_id = api_message_create(CTL_MSG_COUNT, sizeof(HCPlayerMsg));
    pthread_mutex_init(&media_hld->msg_task_mutex, NULL);
    pthread_cond_init(&media_hld->msg_task_cond, NULL);

    //create the message task
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 0x2000);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); //release task resource itself
    if (pthread_create(&thread_id, &attr, media_monitor_task, (void *)media_hld))
        return API_FAILURE;
    pthread_attr_destroy(&attr);
    return API_SUCCESS;
}


static int media_monitor_deinit(media_handle_t *media_hld)
{
    if (INVALID_ID == media_hld->msg_id)
        return API_SUCCESS;

    HCPlayerMsg msg;
    msg.type = HCPLAYER_MSG_ERR_UNDEFINED;
    xQueueSend((QueueHandle_t)media_hld->msg_id, (void *) &msg, (TickType_t) 0);

    media_hld->exit = 1;
    pthread_mutex_lock(&media_hld->msg_task_mutex);
    pthread_cond_wait(&media_hld->msg_task_cond, &media_hld->msg_task_mutex);
    pthread_mutex_unlock(&media_hld->msg_task_mutex);
    api_message_delete(media_hld->msg_id);
    media_hld->msg_id = INVALID_ID;
    pthread_mutex_destroy(&media_hld->msg_task_mutex);
    pthread_cond_destroy(&media_hld->msg_task_cond);
    return API_SUCCESS;
}

media_handle_t *media_open(media_type_t type)
{
    media_handle_t *media_hld = (media_handle_t *)malloc(sizeof(media_handle_t));

    memset(media_hld, 0, sizeof(media_handle_t));
    media_hld->type = type;
    media_hld->state = MEDIA_STOP;
    media_hld->msg_id = INVALID_ID;

    media_hld->loop_type = PLAY_LIST_ONE;
    media_monitor_init(media_hld);
    pthread_mutex_init(&media_hld->api_lock, NULL);
    return media_hld;
}

void media_close(media_handle_t *media_hld)
{
    ASSERT_API(media_hld);
    if (m_closevp == false)
    {
        if (media_hld->type == MEDIA_TYPE_VIDEO || media_hld->type == MEDIA_TYPE_PHOTO)
        {
            api_dis_show_onoff(false);
        }
        if (api_media_pic_is_backup())
        {
            api_media_pic_backup_free();
        }
    }
    media_monitor_deinit(media_hld);
    pthread_mutex_destroy(&media_hld->api_lock);
    media_hld->player = NULL;
    free((void *)media_hld);
}

int media_play(media_handle_t *media_hld, const char *media_src)
{
    ASSERT_API(media_hld && media_src);
    image_effect_t *img_effect;
    dis_zoom_t musiccover_args = {0};
    HCPlayerInitArgs player_args;

    pthread_mutex_lock(&media_hld->api_lock);

    if (media_hld->state == MEDIA_PLAY)
    {
        pthread_mutex_unlock(&media_hld->api_lock);
        return API_SUCCESS;
    }

    strncpy(media_hld->play_name, media_src, MAX_FILE_NAME - 1);
    media_play_id_update(media_hld);
    /*use user_data to link with player msgs */
    printf("%s(), line:%d. play: %s.\n", __FUNCTION__, __LINE__, media_src);
    memset(&player_args, 0, sizeof(player_args));
    player_args.uri = media_src;
    player_args.msg_id = media_hld->msg_id;
    player_args.user_data = (void *)((uint32_t)media_hld->play_id);
    player_args.sync_type = 2;
    player_args.rotate_enable = 0;

    switch (media_hld->type)
    {
        case MEDIA_TYPE_VIDEO:
            player_args.play_attached_file = 1;
            /*some special video make by image and music,so show its musiccover */
            player_args.preview_enable = 0;
            break;
        case MEDIA_TYPE_MUSIC:
            player_args.play_attached_file = 1;
            player_args.preview_enable = 1;
            media_hld->is_whth_album = 1;
            memcpy(&player_args.src_area, &musiccover_args.src_area, sizeof(av_area_t));
            memcpy(&player_args.dst_area, &musiccover_args.dst_area, sizeof(av_area_t));
            break;
        case MEDIA_TYPE_PHOTO:
            player_args.img_dis_mode = m_play_config.dis_mode;//IMG_DIS_AUTO;
            player_args.preview_enable = 0;
            player_args.img_dis_hold_time = media_hld->time_gap;
            player_args.gif_dis_interval = 50;
            player_args.img_alpha_mode = 0;
            break;
        default :
            break;
    }

    hcplayer_init(LOG_WARNING);
    media_hld->player = hcplayer_create(&player_args);
    if (!media_hld->player)
    {
        printf("hcplayer_create() fail!\n");
        pthread_mutex_unlock(&media_hld->api_lock);
        return API_FAILURE;
    }
    hcplayer_play(media_hld->player);

    media_hld->state = MEDIA_PLAY;
    media_hld->speed = 0;
    media_hld->is_double_urls = false;
    // mainlayer rotate is closewise
    pthread_mutex_unlock(&media_hld->api_lock);
    return API_SUCCESS;
}

int media_stop(media_handle_t *media_hld)
{
    ASSERT_API(media_hld);
    pthread_mutex_lock(&media_hld->api_lock);

    if (!media_hld->player || media_hld->state == MEDIA_STOP)
    {
        pthread_mutex_unlock(&media_hld->api_lock);
        return API_FAILURE;
    }
    hcplayer_stop2(media_hld->player, m_closevp, m_fillblack);
    media_hld->player = NULL;

    media_hld->state = MEDIA_STOP;
    media_hld->speed = 0;
    media_hld->seek_step = 0;

    pthread_mutex_unlock(&media_hld->api_lock);
    return API_SUCCESS;
}


int media_play_id_update(media_handle_t *media_hld)
{
    ASSERT_API(media_hld);
    media_hld->play_id++;
    return API_SUCCESS;
}

