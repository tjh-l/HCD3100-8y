#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <termios.h>
#include <signal.h>
#include "console.h"
#include <linux/fb.h>
#else
#include <kernel/lib/console.h>
#include <kernel/fb.h>
#endif

#include <hcuapi/dis.h>
#include "pbp_slot_mgr.h"

static char *m_dis_type_str[] = {
	"HD display",
	"UHD display",
};

static char *m_dis_layer_str[] = {
	"main layer",
	"auxp layer",
};

/*
static char *m_dis_src_str[] = {
	"media player",
	"HDMI in",
	"CVBS in",
	"miracast",
	"air cast",
	"dlna",
	"usb android mirror",
	"usb ios mirror",
};
*/

#define DIS_ENTRY(src_id, str)    str,
static const char *m_dis_src_str[] = {
	DIS_ALL_ENTRIES()
};
#undef DIS_ENTRY


static int m_dis_src[PBP_DIS_TYPE_MAX][PBP_DIS_LAYER_MAX] = {0,};

void pbp_slot_busy_set(pbp_dis_type_e dis_type, pbp_dis_layer_e dis_layer, pbp_dis_src_e src_type)
{
	if (dis_type >= PBP_DIS_TYPE_MAX || dis_layer >= PBP_DIS_LAYER_MAX){
		printf("%s(), dis_type:%d, dis_layer:%d. parameter error!\n", __func__, dis_type, dis_layer);
		return;
	}
	if (src_type < PBP_DIS_SRC_MP || src_type >= PBP_DIS_SRC_MAX ){
		printf("%s(), src_type:%d. parameter error!\n", __func__, src_type);
		return;
	}

	printf("%s(). [%s][%s]: %s!\n", __func__,\
		m_dis_type_str[dis_type], m_dis_layer_str[dis_layer], m_dis_src_str[src_type]);

	m_dis_src[dis_type][dis_layer] = src_type;
}

bool pbp_slot_busy_get(pbp_dis_type_e dis_type, pbp_dis_layer_e dis_layer)
{
	if (dis_type >= PBP_DIS_TYPE_MAX || dis_layer >= PBP_DIS_LAYER_MAX){
		printf("%s(), dis_type:%d, dis_layer:%d. parameter error!\n", __func__, dis_type, dis_layer);
		return true;
	}

	int dis_src = m_dis_src[dis_type][dis_layer];
	if (dis_src >= 0){
		printf("%s(), [%s][%s]:%s is busy!\n", __func__, m_dis_type_str[dis_type], \
			m_dis_layer_str[dis_layer], m_dis_src_str[dis_src]);

		return true;
	} else {
		printf("%s(), [%s][%s] is free!\n", __func__, m_dis_type_str[dis_type], \
			m_dis_layer_str[dis_layer]);

		return false;
	}
}

void pbp_slot_busy_clear(pbp_dis_type_e dis_type, pbp_dis_layer_e dis_layer)
{
	if (dis_type >= PBP_DIS_TYPE_MAX || dis_layer >= PBP_DIS_LAYER_MAX){
		printf("%s(), dis_type:%d, dis_layer:%d. parameter error!\n", __func__, dis_type, dis_layer);
		return;
	}

	printf("%s(), [%s][%s] is clear!\n", __func__, m_dis_type_str[dis_type], \
		m_dis_layer_str[dis_layer]);

	m_dis_src[dis_type][dis_layer] = -1;
}

void pbp_slot_busy_status(void)
{
	int i = 0;
	int j = 0;
	int count = 0;

	printf("\npbp busy status:\n");
	for (i = 0; i < PBP_DIS_TYPE_MAX; i ++){
		for (j = 0; j < PBP_DIS_LAYER_MAX; j ++){
			if (m_dis_src[i][j] >= 0)
			{
				int src_str_id = m_dis_src[i][j];
				printf("\t\t [%s][%s]: %s\n", \
					m_dis_type_str[i], m_dis_layer_str[j], m_dis_src_str[src_str_id]);

				count ++;
			}
		}
	}

	if (!count){
		printf("\nAll pbp slots is free!\n");
	}
}


void pbp_slot_busy_init(void)
{
	memset(m_dis_src, -1, sizeof(m_dis_src));
}
