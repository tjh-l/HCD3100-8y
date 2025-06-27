#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <kernel/lib/console.h>
#include <hcuapi/viddec.h>
#include <hcuapi/auddec.h>
#include <hcuapi/common.h>
#include <hcuapi/avsync.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <pthread.h>
#include <ffplayer.h>
#include <glist.h>

typedef struct mediaplayer {
	void *player;
	char *uri;
} mediaplayer;

static mediaplayer *g_mp = 0;
static glist *g_plist = NULL;//for recode multi play
static QueueHandle_t g_msgid = NULL;
static bool g_mpabort = false;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t msg_rcv_thread_id = 0;
static int g_cur_ply_idx = 0;
static int g_rotate = 0;
static int g_mirror = 0;

static inline int find_player_from_list_by_uri (
	void *a, void *b, void *user_data)
{
	(void)user_data;
	return (strcmp(((mediaplayer *)a)->uri, b));
}

static inline int find_player_from_list_by_mediaplayer_handle (
	void *a, void *b, void *user_data)
{
	(void)user_data;
	return ((mediaplayer *)(a) != b);
}

static int mp_stop(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc <= 1) {
		while (g_plist) {
			list = glist_last(g_plist);
			if (list) {
				mp = (mediaplayer *)list->data;
				if (mp->player) {
					hcplayer_stop (mp->player);
					mp->player = NULL;
				}
				if (mp->uri) {
					free(mp->uri);
					mp->uri = NULL;
				}
				free(mp);
				g_plist = glist_delete_link(g_plist, list);
			}
		}
	} else if (argc > 1){
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			if (mp->player) {
				hcplayer_stop (mp->player);
				mp->player = NULL;
			}
			if (mp->uri) {
				free(mp->uri);
				mp->uri = NULL;
			}
			free(mp);
			g_plist = glist_delete_link(g_plist, list);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static void *msg_recv_thread(void *arg)
{
	HCPlayerMsg msg;
	glist *list = NULL;
	mediaplayer *mp = NULL;
	(void)arg;

	while(!g_mpabort) {
		//printf("g_msgid 0x%x\n", g_msgid);
		if (xQueueReceive((QueueHandle_t)g_msgid, (void *)&msg, -1) != pdPASS)
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
		//printf("msg.type %d\n", msg.type);

		pthread_mutex_lock(&g_mutex);

		mp = msg.user_data;
		list = glist_find_custom(g_plist, mp, find_player_from_list_by_mediaplayer_handle);
		if (!list) {
			usleep(1000);
			pthread_mutex_unlock(&g_mutex);
			continue;
		}

		if (msg.type == HCPLAYER_MSG_STATE_EOS)
		{
			printf ("app get eos\n");
			if (mp->player) {
				hcplayer_stop (mp->player);
				mp->player = NULL;
			}
			if (mp->uri) {
				free(mp->uri);
				mp->uri = NULL;
			}
			free(mp);
			g_plist = glist_delete_link(g_plist, list);
		} else if (msg.type == HCPLAYER_MSG_OPEN_FILE_FAILED
			|| msg.type == HCPLAYER_MSG_UNSUPPORT_FORMAT
			|| msg.type == HCPLAYER_MSG_ERR_UNDEFINED) {
			printf ("err happend, stop it\n");
			if (mp->player) {
				hcplayer_stop (mp->player);
				mp->player = NULL;
			}
			if (mp->uri) {
				free(mp->uri);
				mp->uri = NULL;
			}
			free(mp);
			g_plist = glist_delete_link(g_plist, list);
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
			printf("unsupport video\n");
		} else if (msg.type == HCPLAYER_MSG_UNSUPPORT_AUDIO_TYPE) {
			printf("unsupport audio\n");
		} else if (msg.type == HCPLAYER_MSG_AUDIO_DECODE_ERR) {
			printf("audio dec err, audio idx %d\n", msg.val);
		} else if (msg.type == HCPLAYER_MSG_VIDEO_DECODE_ERR) {
			printf("video dec err, video idx %d\n", msg.val);
		} else {
			printf("unknow msg %d\n", (int)msg.type);
		}

		pthread_mutex_unlock(&g_mutex);
	}

	return NULL;
}

static int mp_deinit(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	g_mpabort = 1;

	mp_stop(0, NULL);

	hcplayer_deinit();

	if (msg_rcv_thread_id){
		HCPlayerMsg msg;
		msg.type = HCPLAYER_MSG_UNDEFINED;
		if (g_msgid) {
			xQueueSendToBack((QueueHandle_t)g_msgid, &msg, 0);
		}
		pthread_join(msg_rcv_thread_id, NULL);
		msg_rcv_thread_id = 0;
	}

	if (g_msgid) {
		vQueueDelete(g_msgid);
		g_msgid = NULL;
	}

	if (g_mp) {
		free(g_mp);
		g_mp = NULL;
	}

	*((uint32_t *)0xb8808300) |= 0x1;

	return 0;
}

static int mp_init(void)
{
	if (!g_mp) {
		g_mp = malloc(sizeof(mediaplayer));
		if (!g_mp) {
			return -1;
		}
		memset(g_mp, 0, sizeof(mediaplayer));
	}

	if (!g_msgid) {
		g_msgid = xQueueCreate(( UBaseType_t )configPLAYER_QUEUE_LENGTH,
			sizeof(HCPlayerMsg));
		if (!g_msgid) {
			printf ("create msg queue failed\n");
			mp_deinit(0, NULL);
			return -1;
		}
	}

	g_mpabort = 0;

	if (!msg_rcv_thread_id)
	{
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, 0x2000);
		if(pthread_create(&msg_rcv_thread_id, &attr, msg_recv_thread, NULL)) {
			mp_deinit(0, NULL);
			return -1;
		}
	}

	*((uint32_t *)0xb8808300) &= 0xfffffffe;
	return 0;
}

static int play_uri(char *uri)
{
	HCPlayerInitArgs init_args = {0};
	if (!g_mp) {
		return -1;
	}

	init_args.uri = uri;
	init_args.msg_id = (int)g_msgid;
	init_args.user_data = g_mp;
	init_args.sync_type = HCPLAYER_AUDIO_MASTER;
	init_args.img_dis_hold_time = 5000;
	if (g_rotate != 0 || g_mirror != 0) {
		init_args.rotate_enable = 1;
		init_args.rotate_type = g_rotate % 4;
		init_args.mirror_type = g_mirror % 3;
	}

	g_mp->player = hcplayer_create(&init_args);
	if (!g_mp->player) {
		return -1;
	}

	g_mp->uri = strdup(uri);
	g_plist = glist_append(g_plist, g_mp);
	hcplayer_play(g_mp->player);

	g_mp = malloc(sizeof(mediaplayer));
	if (!g_mp) {
		printf("malloc g_mp err\n");
	}
	memset(g_mp, 0, sizeof(mediaplayer));

	return 0;
}

static int mp_play(int argc, char *argv[])
{
	if (argc < 2)
		return -1;
	mp_init();

	mp_stop(0, NULL);
	return play_uri(argv[1]);
}

static int mp_seek(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc >= 3) {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_seek(mp->player, atoi(argv[2]) * 1000);
		}
	} else if (glist_length(g_plist) == 1 && argc == 2) {
		mp = glist_first(g_plist)->data;
		hcplayer_seek(mp->player, atoi(argv[1]) * 1000);
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_pause(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc == 1) {
		list = g_plist;
		while (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_pause(mp->player);
			list = glist_next(list);
		}
	} else {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_pause(mp->player);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_resume(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;

	pthread_mutex_lock(&g_mutex);

	if (argc == 1) {
		list = g_plist;
		while (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_resume(mp->player);
			list = glist_next(list);
		}
	} else {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			hcplayer_resume(mp->player);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_time(int argc, char *argv[])
{
	glist *list = NULL;
	mediaplayer *mp = NULL;
	int64_t position = 0;
	int64_t duration = 0;

	pthread_mutex_lock(&g_mutex);

	if (argc > 1) {
		list = glist_find_custom(g_plist, argv[1], find_player_from_list_by_uri);
		if (list) {
			mp = (mediaplayer *)list->data;
			position = hcplayer_get_position(mp->player);
			duration = hcplayer_get_duration(mp->player);
			printf("uri: %s:\n", mp->uri);
			printf("curtime/duration %lld ms/%lld ms\n",
				position, duration);
		}
	} else {
		list = g_plist;
		while (list) {
			mp = (mediaplayer *)list->data;
			position = hcplayer_get_position(mp->player);
			duration = hcplayer_get_duration(mp->player);
			//printf("uri: %s:\n", mp->uri);
			printf("\033[1A");
			fflush(stdout);
			printf("\033[K");
			fflush(stdout);
			printf("pos/dur %8lld.%03llds/%8lld.%03llds\n",
				position/1000, position%1000, duration/1000, duration%1000);
			fflush(stdout);
			list = glist_next(list);
		}
	}

	pthread_mutex_unlock(&g_mutex);

	return 0;
}

static int mp_default_rotate(int argc, char *argv[])
{
	if (argc < 2) {
		return -1;
	}

	g_rotate = atoi(argv[1]);
	if (argc >= 3) {
		g_mirror = atoi(argv[2]);
	}

	return 0;
}

static int mp_rotate(int argc, char *argv[])
{
	rotate_type_e rotate_type = ROTATE_TYPE_0;
	mirror_type_e mirror_type = MIRROR_TYPE_NONE;
	glist *list = NULL;
	mediaplayer *mp = NULL;

	if (argc < 2) {
		return -1;
	}

	rotate_type = atoi(argv[1]);
	if (argc >= 3) {
		mirror_type = atoi(argv[2]);
	}

	list = g_plist;
	while (list) {
		mp = (mediaplayer *)list->data;
		hcplayer_change_rotate_mirror_type(mp->player, rotate_type, mirror_type);
		list = glist_next(list);
	}

	return 0;
}

CONSOLE_CMD(mp, NULL, NULL, CONSOLE_CMD_MODE_SELF, "tinymp cmds")
CONSOLE_CMD(play, "mp",  mp_play, CONSOLE_CMD_MODE_SELF, "play uri")
CONSOLE_CMD(stop, "mp",  mp_stop, CONSOLE_CMD_MODE_SELF, "stop")
CONSOLE_CMD(seek, "mp",  mp_seek, CONSOLE_CMD_MODE_SELF, "seek time_s")
CONSOLE_CMD(pause, "mp",  mp_pause, CONSOLE_CMD_MODE_SELF, "pause")
CONSOLE_CMD(resume, "mp",  mp_resume, CONSOLE_CMD_MODE_SELF, "resume")
CONSOLE_CMD(time, "mp",  mp_time, CONSOLE_CMD_MODE_SELF, "time")
CONSOLE_CMD(default_rotate, "mp",  mp_default_rotate, CONSOLE_CMD_MODE_SELF, "default_rotate rotate_type mirror_type")
CONSOLE_CMD(rotate, "mp",  mp_rotate, CONSOLE_CMD_MODE_SELF, "rotate rotate_type mirror_type")
