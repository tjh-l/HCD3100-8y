#ifndef __MEDIA_PLAYER_H__
#define __MEDIA_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <ffplayer.h>
#include <hcuapi/dis.h>
#include <sys/stat.h>
#include <hcuapi/vidmp.h>

#define HCPLAYER_MSG_BUFFING_FLAG 0X1000
typedef enum
{
    MEDIA_PLAY_NORMAL,
    MEDIA_PLAY_PREVIEW,
} media_play_mode_t;

typedef enum
{
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_MUSIC,
    MEDIA_TYPE_PHOTO,
    MEDIA_TYPE_TXT,
    MEDIA_TYPE_COUNT,
} media_type_t;

typedef enum
{
    MEDIA_STOP          = 0,        //stop
    MEDIA_PLAY          = 1,        //normal playback
    MEDIA_PAUSE         = 2,        //pause
    MEDIA_FB            = 3,        //fast backward
    MEDIA_FF            = 4,        //fast forword
    MEDIA_SB            = 5,        //slow backward
    MEDIA_SF            = 6,        //slow forward
    MEDIA_STEP          = 7,        //step video
    MEDIA_RESUME_STOP   = 8,        //
    MEDIA_PLAY_END      = 9,        //play in the end
} media_state_t;


//used for play list to manage the play mode
typedef enum
{
    PlAY_LIST_SEQUENCE,  //seqeunce play media
    PLAY_LIST_REPEAT,    // Juset like sequnce
    PLAY_LIST_RAND,      //rand play media list
    PLAY_LIST_ONE,       // only play current media.
    PLAY_LIST_NONE,      //Just like sequnce
} PlayListLoopType;

/*def a user func callback to do something*/
typedef void (*mp_func_callback)(void *arg1, void *arg2);

//define media_do_opt ,//record the action in player,like rotate / zoom /....
#define DO_ROTATE_OPT       (1<<1)

typedef struct
{
    media_type_t        type;
    media_state_t       state;
    uint8_t             speed;  //only used for video
    rotate_type_e       rotate; //closewise
    uint32_t            media_do_opt; //record the action in player,like rotate / zoom /....
    dis_tv_mode_e       ratio_mode; //video ratio mode attributes ,only use for video
    uint32_t            play_time; //not used for photo
    uint32_t            total_time; //not used for photo
    int64_t             last_seek_op_time;
    uint32_t            jump_interval;
    uint32_t            time_gap; //only used for photo, the slide show play interval, unit is ms.
    PlayListLoopType    loop_type;
    int32_t             seek_step;// use for music manual seek to simulate music ff/fb
    char                play_name[1024];
    void                *player;
    int                 msg_id;
    uint16_t            play_id;//user for msg_proc
    mp_func_callback    mp_func_cb;//use for iptv
    uint8_t             exit;
    uint8_t             is_whth_album;  //used for Music album cover
    bool                is_double_urls; //use for double urls like iptv app
    int                 pic_effect_mode;
    pthread_mutex_t     api_lock;
    pthread_mutex_t     msg_task_mutex;
    pthread_cond_t      msg_task_cond;
} media_handle_t;

media_handle_t *media_open(media_type_t type);
void media_close(media_handle_t *media_hld);
int media_play(media_handle_t *media_hld, const char *media_src);
int media_stop(media_handle_t *media_hld);
int media_play_id_update(media_handle_t *media_hld);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __MEDIA_PLAYER_H__
