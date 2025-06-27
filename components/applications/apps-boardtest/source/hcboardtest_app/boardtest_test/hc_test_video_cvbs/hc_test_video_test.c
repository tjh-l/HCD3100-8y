/**
 * @file hc_test_video_test.c
 * @author arthur (arthur.wen@hichiptech.com)
 * @brief cvbs以及video的相关test
 *        1.接入前需要检查disk
 *        2.linux尚未支持，
 *        3.添加了参数检查
 *        检查优盘，添加多种路径
 *
 *
 * @version 0.1
 * @date 2023-10-28
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "zconf.h"
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef __linux__
#else
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#endif

#include <hcuapi/common.h>
#include <hcuapi/dis.h>
#include <hcuapi/snd.h>

#include "boardtest_module.h"
#include <ffplayer.h>
#include <ffplayer_manager.h>
#include <glist.h>

#define _MALLOC_ADD(x, s)                                                      \
    do {                                                                       \
    } while (0);
#define MESSAGE(message, assertion)                                            \
    do {                                                                       \
    } while (0);
typedef struct mediaplayer {
    void *player;
    char *uri;
} mediaplayer;
static mediaplayer *g_mp = 0;

#ifdef __linux__
static int g_msgid = -1;
static struct HCGeDrawSubtitle ge_draw_subtitle = { 0 };
#else
static QueueHandle_t g_msgid = NULL;
#endif
static bool g_multi_ins;
static bool g_mpabort;
static pthread_t msg_rcv_thread_id;
static uint8_t g_volume = 100;
static bool g_mp_info;
static void *g_mp2;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static glist *g_plist;
static bool g_closevp = true, g_fillblack;
static glist *url_list;
static int g_cur_ply_idx;
static FILE *g_mem_play_file;
static bool g_smooth_mode;
static int g_sync_mode = 2;
static AudioOutDev g_snd_devs;
static double g_time_ms;
static bool g_buffering_enable;
static img_dis_mode_e g_img_dis_mode;
static mirror_type_e g_mirror_type;
static rotate_type_e g_rotate_type;
static bool g_disable_video;
static bool g_disable_audio;
static int g_en_subtitle;
static int g_audio_flush_thres;
static int g_pic_show_duration; // all pic play with 3000ms delay
static int g_gif_interval;
static bool g_bypass;
static HCAlphaBlendMode g_pic_bg = ALPHA_BLEND_CHECKERBOARD;
static char *decryption_key;

static vdec_dis_rect_t g_dis_rect = { { 0, 0, 1920, 1080 },
                                      { 0, 0, 1920, 1080 } };
static int g_loop_play = 1;

#define MAX_ABSOLUTE_PATH_LENGTH 160
static char video_play_file_path[MAX_ABSOLUTE_PATH_LENGTH];
static char video_play_dir_path[MAX_ABSOLUTE_PATH_LENGTH];
static char cvbs_video_display_file_path[MAX_ABSOLUTE_PATH_LENGTH];
static char cvbs_lvideo_play_file_path[MAX_ABSOLUTE_PATH_LENGTH];
static char cvbs_rvideo_play_file_path[MAX_ABSOLUTE_PATH_LENGTH];
#ifdef __linux__
#define MAX_UDISK_PATH_ARRAY_LENGTH 1
static char *udisk_dir[MAX_UDISK_PATH_ARRAY_LENGTH] = { "/media/hdd/hichip_boardtest/" };
#else
#define MAX_UDISK_PATH_ARRAY_LENGTH 2
static char *udisk_dir[MAX_UDISK_PATH_ARRAY_LENGTH] = { "/media/sda/hichip_boardtest/",
                                                        "/media/sda1/hichip_boardtest/" };
#endif

static int hc_test_video_test_mp_init(void);
static int set_i2so_volume(uint8_t volume);

static int mp_set_dis_onoff(bool on_off);
static int mp_deinit(int argc, char *argv[]);

static void *hc_test_video_test_msg_recv_thread(void *arg);
static int mp_stop(int argc, char *argv[]);
static inline int find_player_from_list_by_mediaplayer_handle(void *a, void *b,
                                                              void *user_data);
static int pic_backup(void);
static int stop_showlogo(int argc, char *argv[]);
static inline int find_player_from_list_by_uri(void *a, void *b,
                                               void *user_data);
static int play_uri(char *uri);
static int hc_test_video_test_play_next_uri(void);
static int hc_test_video_test_set_screen_info(void);

static int hc_test_video_test_mp_play(char *uri);

static void waiting_disk(void);
static int check_dir_path(const char *path);
static int get_disk_path(char *path[], const int path_array_length,
                         char **disk_root);
static int get_abs_path(const char *dir_path, const char *rela_path,
                        char *abs_path);
static int hc_test_video_test_list_files(char *dir_path);

static int _hc_test_video_test_play(char *path);
static int _hc_test_video_test_stress(char *path);
static int _hc_test_video_test_exit(void);

static int hc_test_cvbs_test_display(void);
static int hc_test_cvbs_test_lvideo_play(void);
static int hc_test_cvbs_test_rvideo_play(void);

static int hc_test_video_test_play(void);
static int hc_test_video_test_stress(void);
static int hc_test_video_test_exit(void);

static int hc_test_cvbs_test_display(void)
{
    int ret = 0;
    char *disk_root = NULL;
    char *cvbs_video_display_file_rela_path = "qt/cvbs/cvbs.ts";
    ret = get_disk_path(udisk_dir, MAX_UDISK_PATH_ARRAY_LENGTH, &disk_root);
    if (ret != 0) {
        write_boardtest_detail(BOARDTEST_CVBS_STATUS,
                               "cvbs display play error");
        printf("get_disk_path error:%d", ret);
        return BOARDTEST_FAIL;
    }
    get_abs_path(disk_root, cvbs_video_display_file_rela_path,
                 cvbs_video_display_file_path);

    ret = _hc_test_video_test_play(cvbs_video_display_file_path);
    if (ret != 0) {
        write_boardtest_detail(BOARDTEST_CVBS_STATUS,
                               "cvbs display play error");
        printf("hc_test_cvbs_test_display error:%d", ret);
        return BOARDTEST_FAIL;
    }

    create_boardtest_passfail_mbox(BOARDTEST_CVBS_STATUS);
    return BOARDTEST_CALL_PASS;
}

static int hc_test_cvbs_test_lvideo_play(void)
{
    int ret = 0;
    char *disk_root = NULL;
    char *cvbs_lvideo_play_file_rela_path = "qt/cvbs/cvbsl.ts";
    ret = get_disk_path(udisk_dir, MAX_UDISK_PATH_ARRAY_LENGTH, &disk_root);
    if (ret != 0) {
        write_boardtest_detail(BOARDTEST_CVBS_LEFT_CHANNEL,
                               "cvbs lvideo play error");
        printf("get_disk_path error:%d", ret);
        return BOARDTEST_FAIL;
    }
    get_abs_path(disk_root, cvbs_lvideo_play_file_rela_path,
                 cvbs_lvideo_play_file_path);

    ret = _hc_test_video_test_play(cvbs_lvideo_play_file_path);
    if (ret != 0) {
        write_boardtest_detail(BOARDTEST_CVBS_LEFT_CHANNEL,
                               "cvbs lvideo play error");
        printf("hc_test_cvbs_test_lvideo_play error:%d", ret);
        return BOARDTEST_FAIL;
    }

    create_boardtest_passfail_mbox(BOARDTEST_CVBS_LEFT_CHANNEL);
    return BOARDTEST_CALL_PASS;
}

static int hc_test_cvbs_test_rvideo_play(void)
{
    int ret = 0;
    char *disk_root = NULL;
    char *cvbs_rvideo_play_file_rela_path = "qt/cvbs/cvbsr.ts";
    ret = get_disk_path(udisk_dir, MAX_UDISK_PATH_ARRAY_LENGTH, &disk_root);
    if (ret != 0) {
        write_boardtest_detail(BOARDTEST_CVBS_RIGHT_CHANNEL,
                               "right video play error");
        printf("get_disk_path error:%d", ret);
        return BOARDTEST_FAIL;
    }
    get_abs_path(disk_root, cvbs_rvideo_play_file_rela_path,
                 cvbs_rvideo_play_file_path);

    ret = _hc_test_video_test_play(cvbs_rvideo_play_file_path);
    if (ret != 0) {
        write_boardtest_detail(BOARDTEST_CVBS_RIGHT_CHANNEL,
                               "right video play error");
        printf("hc_test_cvbs_test_rvideo_play error:%d", ret);
        return BOARDTEST_FAIL;
    }
    create_boardtest_passfail_mbox(BOARDTEST_CVBS_RIGHT_CHANNEL);
    return BOARDTEST_CALL_PASS;
}

static int hc_test_video_test_play(void)
{
    int ret = 0;
    char *disk_root = NULL;
    char *video_test_play_rela_path = "qt/APinkPart001.mp4";
    ret = get_disk_path(udisk_dir, MAX_UDISK_PATH_ARRAY_LENGTH, &disk_root);
    if (ret != 0) {
        write_boardtest_detail(BOARDTEST_VIDEO_TEST, "video test play error");
        printf("get_disk_path error:%d", ret);
        return BOARDTEST_FAIL;
    }
    get_abs_path(disk_root, video_test_play_rela_path, video_play_file_path);

    ret = _hc_test_video_test_play(video_play_file_path);
    if (ret != 0) {
        write_boardtest_detail(BOARDTEST_VIDEO_TEST, "video test play error");
        printf("hc_test_video_test_play error:%d", ret);
        return BOARDTEST_FAIL;
    }
    create_boardtest_passfail_mbox(BOARDTEST_VIDEO_TEST);
    return BOARDTEST_CALL_PASS;
}

static int hc_test_video_test_stress(void)
{
    int ret = 0;

    char *disk_root = NULL;
    char *stress_rela_path = "qt";
    ret = get_disk_path(udisk_dir, MAX_UDISK_PATH_ARRAY_LENGTH, &disk_root);
    if (ret != 0) {
        write_boardtest_detail(BOARDTEST_VIDEO_IMAGE_LOOP,
                               "video stress test play error");
        printf("get_disk_path error:%d", ret);
        return BOARDTEST_FAIL;
    }
    get_abs_path(disk_root, stress_rela_path, video_play_dir_path);

    ret = _hc_test_video_test_stress(video_play_dir_path);
    if (ret != 0) {
        write_boardtest_detail(BOARDTEST_VIDEO_IMAGE_LOOP,
                               "video stress test play error");
        printf("hc_test_video_test_stress error:%d", ret);
        return BOARDTEST_FAIL;
    }
    return BOARDTEST_CALL_PASS;
}

static int hc_test_video_test_exit(void)
{
    int ret = 0;
    ret = _hc_test_video_test_exit();
    if (ret != 0) {
        printf("hc_test_video_test_exit error:%d", ret);
        return BOARDTEST_FAIL;
    }
    return BOARDTEST_CALL_PASS;
}

static int hc_test_video_test_stress_exit(void)
{
    int ret = 0;
    ret = _hc_test_video_test_exit();
    if (ret != 0) {
        printf("hc_test_video_test_stress_exit error:%d",
               ret); // fail callback
        return BOARDTEST_FAIL;
    }
    return BOARDTEST_CALL_PASS;
}

/**
 * @brief 测试视频文件的是否能正常播放，有无画面，有无声音
 *
 * @param path 文件绝对路径
 * @return int
 */
static int _hc_test_video_test_play(char *path)
{
    int ret = 0;
    char *uri;
    uri = strdup(path);
    _MALLOC_ADD(1, RUNNING);

    printf("video path:%s\n", uri);
    // waiting_disk ();

    ret = hc_test_video_test_mp_init();
    if (ret != 0)
        return 200 + ret;
    ret = hc_test_video_test_mp_play(uri);
    if (ret != 0)
        return 300 + ret;

    return 0;
}
/**
 * @brief 对指定文件夹内的视频文件循环播放，压力测试
 *
 * @param path 指定目录绝对路径
 * @return int
 */
static int _hc_test_video_test_stress(char *path)
{
    int ret = 0;
    printf("video test dir:%s\n", path);
    // waiting_disk ();
    ret = hc_test_video_test_mp_init();
    if (ret != 0)
        return 200 + ret;
    ret = hc_test_video_test_list_files(path);
    if (ret != 0)
        return 300 + ret;
    return 0;
}
/**
 * @brief exit video test
 *
 * @return int
 */
static int _hc_test_video_test_exit(void)
{
    mp_stop(1, NULL);
    mp_deinit(1, NULL);
    return 0;
}

static int hc_test_video_test_mp_init(void)
{
    // init
    // i2s0 获取音量，设置音量10
    set_i2so_volume(5);
    hc_test_video_test_set_screen_info();
    // mp_set_dis_onoff (false);
    // osd request
    close_lvgl_osd();

#ifdef __linux__
    if (g_msgid < 0) {
        g_msgid = msgget(MKTAG('h', 'c', 'p', 'l'), 0666 | IPC_CREAT);
        if (g_msgid < 0) {
            printf("create msg queue failed\n");
            mp_deinit(0, NULL);
            return -1;
        }
    }
#else
    //创建消息队列
    if (!g_msgid) {
        g_msgid = xQueueCreate((UBaseType_t)configPLAYER_QUEUE_LENGTH,
                               sizeof(HCPlayerMsg));
        if (!g_msgid) {
            printf("create msg queue failed\n");
            mp_deinit(0, NULL);
            return -1;
        }
    }
#endif

    g_multi_ins = 0;

    g_mpabort = 0;

    // msg接受线程
    if (!msg_rcv_thread_id) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x2000);
        if (pthread_create(&msg_rcv_thread_id, &attr,
                           hc_test_video_test_msg_recv_thread, NULL)) {
            mp_deinit(0, NULL);
            return -1;
        }
    }
    // hcplayer初始化
    hcplayer_init(LOG_WARNING);

    return 0;
}

/**
 * @brief Set the i2so volume object设置音量大小
 *
 * @param volume
 * @return int
 */
static int set_i2so_volume(uint8_t volume)
{
    int snd_fd = -1;

    snd_fd = open("/dev/sndC0i2so", O_WRONLY);
    if (snd_fd < 0) {
        printf("open i2so %d failed\n", snd_fd);
        return -1;
    }

    ioctl(snd_fd, SND_IOCTL_SET_VOLUME, &volume);
    volume = 0;
    ioctl(snd_fd, SND_IOCTL_GET_VOLUME, &g_volume);
    printf("current i2so volume is %d\n", g_volume);

    close(snd_fd);
    return 0;
}

/**
 * @brief 设置显示的开关
 *
 * @param on_off
 * @return int
 */
static int mp_set_dis_onoff(bool on_off)
{
    int fd = -1;
    struct dis_win_onoff winon = { 0 };
    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0) {
        return -1;
    }
    winon.distype = DIS_TYPE_HD;
    winon.layer = DIS_LAYER_MAIN;
    winon.on = on_off ? 1 : 0;
    ioctl(fd, DIS_SET_WIN_ONOFF, &winon);
    close(fd);
    return 0;
}

static int mp_deinit(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    g_mpabort = 1;
    g_mp_info = 0;

    // mp_stop (0, NULL);

    hcplayer_deinit();

    if (msg_rcv_thread_id) {
        HCPlayerMsg msg;
        msg.type = HCPLAYER_MSG_UNDEFINED;
#ifdef __linux__
        if (g_msgid >= 0) {
            msgsnd(g_msgid, (void *)&msg,
                   sizeof(HCPlayerMsg) - sizeof(msg.type), 0);
        }
#else
        if (g_msgid) {
            xQueueSendToBack((QueueHandle_t)g_msgid, &msg, 0);
        }
#endif
        pthread_join(msg_rcv_thread_id, NULL);
        msg_rcv_thread_id = 0;
    }

#ifdef __linux__
    if (g_msgid >= 0) {
        msgctl(g_msgid, IPC_RMID, NULL);
        g_msgid = -1;
    }
#else
    if (g_msgid) {
        vQueueDelete(g_msgid);
        g_msgid = NULL;
    }
#endif

    _MALLOC_ADD(0, STOP);

    // osd release
    open_lvgl_osd();
    return 0;
}

static void *hc_test_video_test_msg_recv_thread(void *arg)
{
    (void)arg;
    HCPlayerMsg msg;
    glist *list = NULL;
    mediaplayer *mp = NULL;

    while (!g_mpabort) {
#ifdef __linux__
        if (msgrcv(g_msgid, (void *)&msg,
                   sizeof(HCPlayerMsg) - sizeof(msg.type), 0, 0) == -1)
#else
        if (xQueueReceive((QueueHandle_t)g_msgid, (void *)&msg, -1) != pdPASS)
#endif
        {
            if (!g_mpabort) {
                printf("msg_recv_thread err\n");
                usleep(5000);
            }
            continue;
        }

        if (g_mpabort) {
            break;
        }

        if (g_mp2 && msg.type == HCPLAYER_MSG_STATE_EOS) {
            hcplayer_multi_destroy(g_mp2);
            g_mp2 = NULL;
            continue;
        }

        pthread_mutex_lock(&g_mutex);

        mp = msg.user_data;
        list = glist_find_custom(g_plist, mp,
                                 find_player_from_list_by_mediaplayer_handle);
        if (!list) {
            usleep(1000);
            pthread_mutex_unlock(&g_mutex);
            continue;
        }

        if (msg.type == HCPLAYER_MSG_STATE_EOS) {
            printf("app get eos\n");
            if (mp->player) {
                hcplayer_stop2(mp->player, g_closevp, g_fillblack);
                pic_backup();
                mp->player = NULL;
            }

            hc_test_video_test_play_next_uri();
        } else if (msg.type == HCPLAYER_MSG_STATE_TRICK_BOS) {
            printf("app get trick bos\n");
            if (mp->player) {
                hcplayer_resume(mp->player);
            }
        } else if (msg.type == HCPLAYER_MSG_STATE_TRICK_EOS) {
            printf("app get trick eos\n");
            if (mp->player) {
                hcplayer_stop2(mp->player, g_closevp, g_fillblack);
                mp->player = NULL;
                pic_backup();
            }

            hc_test_video_test_play_next_uri();
        } else if (msg.type == HCPLAYER_MSG_OPEN_FILE_FAILED ||
                   msg.type == HCPLAYER_MSG_UNSUPPORT_FORMAT ||
                   msg.type == HCPLAYER_MSG_ERR_UNDEFINED) {
            printf("err happend, stop it\n");
            if (mp->player) {
                hcplayer_stop2(mp->player, g_closevp, g_fillblack);
                mp->player = NULL;
                pic_backup();
            }
            hc_test_video_test_play_next_uri();
        } else if (msg.type == HCPLAYER_MSG_BUFFERING) {
            printf("buffering %d\n", msg.val);
        } else if (msg.type == HCPLAYER_MSG_STATE_PLAYING) {
            printf("player playing\n");
        } else if (msg.type == HCPLAYER_MSG_STATE_PAUSED) {
            printf("player paused\n");
        } else if (msg.type == HCPLAYER_MSG_STATE_READY) {
            printf("player ready\n");
        } else if (msg.type == HCPLAYER_MSG_READ_TIMEOUT) {
            printf("player read timeout\n");
        } else if (msg.type == HCPLAYER_MSG_UNSUPPORT_ALL_AUDIO) {
            printf("no audio track or no supported audio track\n");
        } else if (msg.type == HCPLAYER_MSG_UNSUPPORT_ALL_VIDEO) {
            printf("no video track or no supported video track\n");
        } else if (msg.type == HCPLAYER_MSG_UNSUPPORT_VIDEO_TYPE) {
            HCPlayerVideoInfo video_info;
            char *video_type = "unknow";
            if (!hcplayer_get_nth_video_stream_info(mp->player, msg.val,
                                                    &video_info)) {
                /* only a simple sample, app developers use a static struct to
               * mapping them. */
                if (video_info.codec_id == HC_AVCODEC_ID_HEVC) {
                    video_type = "h265";
                } else if (video_info.codec_id == HC_AVCODEC_ID_VP9) {
                    video_type = "vp9";
                } else if (video_info.codec_id == HC_AVCODEC_ID_AMV) {
                    video_type = "amv";
                }
            }
            printf("unsupport video type %s, codec id %d\n", video_type,
                   video_info.codec_id);
        } else if (msg.type == HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE) {
            HCPlayerAudioInfo audio_info;
            char *audio_type = "unknow";
            if (!hcplayer_get_nth_audio_stream_info(mp->player, msg.val,
                                                    &audio_info)) {
                /* only a simple sample, app developers use a static struct to
               * mapping them. */
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
            printf("unsupport audio type %s\n", audio_type);
        } else if (msg.type == HCPLAYER_MSG_AUDIO_DECODE_ERR) {
            printf("audio dec err, audio idx %d\n", msg.val);
            /* check if it is the last audio track, if not, then change to next
           * one. */
            if (mp->player) {
                int total_audio_num = -1;
                total_audio_num = hcplayer_get_audio_streams_count(mp->player);
                if (msg.val >= 0 && total_audio_num > (msg.val + 1)) {
                    HCPlayerAudioInfo audio_info;
                    if (!hcplayer_get_cur_audio_stream_info(mp->player,
                                                            &audio_info)) {
                        if (audio_info.index == msg.val) {
                            int idx = audio_info.index + 1;
                            while (hcplayer_change_audio_track(mp->player,
                                                               idx)) {
                                idx++;
                                if (idx >= total_audio_num) {
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    hcplayer_change_audio_track(mp->player, -1);
                }
            }
        } else if (msg.type == HCPLAYER_MSG_VIDEO_DECODE_ERR) {
            printf("video dec err, video idx %d\n", msg.val);
            /* check if it is the last video track, if not, then change to next
           * one. */
            if (mp->player) {
                int total_video_num = -1;
                total_video_num = hcplayer_get_video_streams_count(mp->player);
                if (msg.val >= 0 && total_video_num > (msg.val + 1)) {
                    HCPlayerVideoInfo video_info;
                    if (!hcplayer_get_cur_video_stream_info(mp->player,
                                                            &video_info)) {
                        if (video_info.index == msg.val) {
                            int idx = video_info.index + 1;
                            while (hcplayer_change_video_track(mp->player,
                                                               idx)) {
                                idx++;
                                if (idx >= total_video_num) {
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    hcplayer_change_video_track(mp->player, -1);
                }
            }
        } else if (msg.type == HCPLAYER_MSG_FIRST_VIDEO_FRAME_TRANSCODED) {
            printf("unknow msg %d\n", (int)msg.type);
        } else {
            printf("unknow msg %d\n", (int)msg.type);
        }

        pthread_mutex_unlock(&g_mutex);
    }

    return NULL;
}

static int mp_stop(int argc, char *argv[])
{
    glist *list = NULL;
    mediaplayer *mp = NULL;

#ifndef __linux__
    stop_showlogo(0, NULL);
#endif

    pthread_mutex_lock(&g_mutex);

    if (argc <= 1) {
        while (g_plist) {
            list = glist_last(g_plist);
            if (list) {
                mp = (mediaplayer *)list->data;
                if (mp->player) {
                    hcplayer_stop2(mp->player, g_closevp, g_fillblack);
                    mp->player = NULL;
                    pic_backup();
                }
                free(mp);
                _MALLOC_ADD(-1, RUNNING);
                g_plist = glist_delete_link(g_plist, list);
            }
        }
        g_mp = NULL;

        if (argc != 0) {
            while (url_list) {
                if (url_list->data) {
                    free(url_list->data);
                    _MALLOC_ADD(-1, RUNNING);
                }
                url_list = glist_delete_link(url_list, url_list);
            }
        }
    }

    if (g_mem_play_file) {
        fclose(g_mem_play_file);
        g_mem_play_file = NULL;
    }

#ifdef __linux__
    if (ge_draw_subtitle.fbdev > 0) {
        if (ge_draw_subtitle.ctx != NULL) {
            free(ge_draw_subtitle.ctx);
            ge_draw_subtitle.ctx = NULL;
        }
        if (ge_draw_subtitle.tar_buf != NULL) {
            free(ge_draw_subtitle.tar_buf);
            ge_draw_subtitle.tar_buf = NULL;
        }
        deinit_fb_device(&ge_draw_subtitle);
        memset(&ge_draw_subtitle, 0, sizeof(struct HCGeDrawSubtitle));
    }
#endif
    pthread_mutex_unlock(&g_mutex);

    return 0;
}
static inline int find_player_from_list_by_mediaplayer_handle(void *a, void *b,
                                                              void *user_data)
{
    (void)user_data;
    return ((mediaplayer *)(a) != b);
}

static int pic_backup(void)
{
    int fd;
    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0) {
        return -1;
    }

    if (g_smooth_mode) {
        ioctl(fd, DIS_BACKUP_MP, DIS_TYPE_HD);
    }

    close(fd);
    return 0;
}

static int hc_test_video_test_play_next_uri(void)
{
    printf("play_next_uri\n");
    if (url_list) {
        char *uri;

        g_cur_ply_idx++;
        if (g_cur_ply_idx >= (int)glist_length(url_list)) {
            g_cur_ply_idx = 0;
        }
        uri = (char *)glist_nth_data(url_list, g_cur_ply_idx);
        if (uri) {
            printf("uri %s sync_type %d\n", uri, g_sync_mode);
            play_uri(uri);
        }
    }
    return 0;
}

static int stop_showlogo(int argc, char *argv[])
{
    // Todo
    return 0;
}
static inline int find_player_from_list_by_uri(void *a, void *b,
                                               void *user_data)
{
    (void)user_data;
    return (strcmp(((mediaplayer *)a)->uri, b));
}
static int play_uri(char *uri)
{
    HCPlayerInitArgs init_args = { 0 };
    if (!g_mp) {
        g_mp = malloc(sizeof(mediaplayer));
        if (!g_mp) {
            return -1;
        }
        _MALLOC_ADD(1, RUNNING);
        memset(g_mp, 0, sizeof(mediaplayer));
        // put into list without args init.
        g_plist = glist_append(g_plist, g_mp);
    }

    init_args.uri = uri;
    init_args.snd_devs = g_snd_devs;
    init_args.sync_type = g_sync_mode;
    init_args.user_data = g_mp;
    init_args.start_time = g_time_ms;
    init_args.buffering_enable = g_buffering_enable;
    init_args.play_attached_file = 1;
    init_args.msg_id = (int)g_msgid;
    init_args.img_dis_mode = g_img_dis_mode;
    init_args.mirror_type = g_mirror_type;
    init_args.rotate_type = g_rotate_type % 4;
    init_args.callback = NULL;
    init_args.disable_audio = g_disable_audio;
    init_args.disable_video = g_disable_video;
    if (g_en_subtitle) {
#ifdef __linux__
        init_args.callback = subtitle_callback;
#endif
    }
    if (g_rotate_type || g_mirror_type) {
        init_args.rotate_enable = 1;
    }
    init_args.audio_flush_thres = g_audio_flush_thres;
    init_args.bypass = g_bypass;

    init_args.img_dis_hold_time = g_pic_show_duration;
    init_args.gif_dis_interval = g_gif_interval;
    init_args.img_alpha_mode = g_pic_bg;

    init_args.decryption_key = decryption_key;

    MESSAGE("testing", g_mp->player == NULL);
    g_mp->player = hcplayer_create(&init_args);
    if (!g_mp->player) {
        return -1;
    }
    g_mp->uri = uri;
    hcplayer_play(g_mp->player);

    return 0;
}

static int hc_test_video_test_mp_play(char *uri)
{
    if (!uri)
        return -1;

    g_time_ms = 0;
    g_sync_mode = 2;

    printf("g_multi_ins:%d\n", g_multi_ins);
    if (!g_multi_ins) {
        mp_stop(0, NULL);
    }

    pthread_mutex_lock(&g_mutex);

    if (g_time_ms >= 1) {
        g_time_ms *= 1000;
    }

    if (g_loop_play) {
        url_list = glist_append(url_list, uri);
    }
    play_uri(url_list->data);

    pthread_mutex_unlock(&g_mutex);

    return 0;
}
//for test
static void waiting_disk(void)
{
    int print_tic = 0;
    // video img get
    char *flag_path = "/media/sda1/flag.txt";
    FILE *rec = NULL;
    for (; rec == NULL;) {
        rec = fopen(flag_path, "r");
        if (rec) {
            fclose(rec);
            printf("wait pic success\n");
        }
        if ((print_tic++) % 1000 == 0) {
            printf("\n\n ********* waiting disk! *********\n\n");
        }
        usleep(10000);
    }
}

static int check_dir_path(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir) {
        printf("open dir error!\n");
        return 1;
    }
    closedir(dir);

    return 0;
}

// get_disk_path(path,&pointer);
static int get_disk_path(char *path[], const int path_array_length,
                         char **disk_root)
{
    for (int i = 0; i < path_array_length; i++) {
        if (check_dir_path(path[i]) == 0) {
            *disk_root = path[i];
            return 0;
        }
    }

    return 1;
}

static int get_abs_path(const char *dir_path, const char *rela_path,
                        char *abs_path)
{
    // todo parse path

    // 	 dir_path, rela_path);
    if (strlen(abs_path) == 0) {
        strcat(abs_path, dir_path);
        strcat(abs_path, rela_path);
    }

    return 0;
}

static int hc_test_video_test_set_screen_info(void)
{
    dis_screen_info_t dis_area;
    int fd;
    fd = open("/dev/dis", O_WRONLY);
    if (fd < 0) {
        return -1;
    }
    dis_area.distype = DIS_TYPE_HD;
    ioctl(fd, DIS_GET_SCREEN_INFO, &dis_area);
    close(fd);
    g_dis_rect.src_rect.x = dis_area.area.x;
    g_dis_rect.src_rect.y = dis_area.area.y;
    g_dis_rect.src_rect.w = dis_area.area.w;
    g_dis_rect.src_rect.h = dis_area.area.h;

    g_dis_rect.dst_rect = g_dis_rect.src_rect;

    return 0;
}

static int hc_test_video_test_list_files(char *dir_path)
{
    DIR *dir = opendir(dir_path);
    if (!dir) {
        printf("open dir error!\n");
        return -1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char *file_path =
                (char *)malloc(strlen(dir_path) + strlen(entry->d_name) + 2);
        if (file_path == NULL) {
            printf("memory alloc error!\n");
            closedir(dir);
            return -1;
        }
        _MALLOC_ADD(1, RUNNING);
        snprintf(file_path, strlen(dir_path) + strlen(entry->d_name) + 2,
                 "%s/%s", dir_path, entry->d_name);
        DIR *sub_dir = opendir(file_path);
        if (sub_dir) {
            closedir(sub_dir);
        } else {
            char *ext = strrchr(entry->d_name, '.');
            if (ext && (strcmp(ext, ".mp4") == 0 || strcmp(ext, ".jpg") == 0)) {
                printf("%s\n", file_path);
                if (g_loop_play) {
                    if (!url_list) {
                        hc_test_video_test_mp_play(file_path);
                    } else {
                        url_list = glist_append(url_list, file_path);
                    }
                }
            }
        }
    }

    closedir(dir);
    return 0;
}

/**
 * @brief video play test register func
 *
 * @return int
 */
static int hc_boardtest_cvbs_test_display(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "CVBS_STATUS";
    test->sort_name = BOARDTEST_CVBS_STATUS;
    test->init = NULL;
    test->run = hc_test_cvbs_test_display;
    test->exit = hc_test_video_test_exit;
    test->tips = "dispaly.Please select whether the test item passed or not.";

    hc_boardtest_module_register(test);

    return 0;
}

/**
 * @brief video play test register func
 *
 * @return int
 */
static int hc_boardtest_cvbs_test_lvideo_play(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "CVBS_LEFT_CHANNEL";
    test->sort_name = BOARDTEST_CVBS_LEFT_CHANNEL;
    test->init = NULL;
    test->run = hc_test_cvbs_test_lvideo_play;
    test->exit = hc_test_video_test_exit;
    test->tips =
            "Woman's voice.Please select whether the test item passed or not.";

    hc_boardtest_module_register(test);

    return 0;
}

/**
 * @brief video play test register func
 *
 * @return int
 */
static int hc_boardtest_cvbs_test_rvideo_play(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "CVBS_RIGHT_CHANNEL";
    test->sort_name = BOARDTEST_CVBS_RIGHT_CHANNEL;
    test->init = NULL;
    test->run = hc_test_cvbs_test_rvideo_play;
    test->exit = hc_test_video_test_exit;
    test->tips =
            "Man's voice.Please select whether the test item passed or not.";

    hc_boardtest_module_register(test);

    return 0;
}

/**
 * @brief video play test register func
 *
 * @return int
 */
static int hc_boardtest_video_test(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "VIDEO_TEST";
    test->sort_name = BOARDTEST_VIDEO_TEST;
    test->init = NULL;
    test->run = hc_test_video_test_play;
    test->exit = hc_test_video_test_exit;
    test->tips = "Please select whether the test item passed or not.";

    hc_boardtest_module_register(test);

    return 0;
}

/**
 * @brief video test stress register func
 *
 * @return int
 */
static int hc_boardtest_video_test_stress(void)
{
    hc_boardtest_msg_reg_t *test = malloc(sizeof(hc_boardtest_msg_reg_t));

    test->english_name = "VIDEO_IMAGE_LOOP";
    test->sort_name = BOARDTEST_VIDEO_IMAGE_LOOP;
    test->init = NULL;
    test->run = hc_test_video_test_stress;
    test->exit = hc_test_video_test_exit;
    test->tips = "Please select whether the test item passed or not.";

    hc_boardtest_module_register(test);

    return 0;
}

/**
 * @brief register
 *
 */
__initcall(hc_boardtest_cvbs_test_display);
__initcall(hc_boardtest_cvbs_test_lvideo_play);
__initcall(hc_boardtest_cvbs_test_rvideo_play);
__initcall(hc_boardtest_video_test);
__initcall(hc_boardtest_video_test_stress);
