#ifndef __MEMMROY_PLAY_H__
#define __MEMMROY_PLAY_H__

#include "file_mgr.h"
typedef struct{
	uint32_t idx;
	uint32_t save_time;
	uint32_t save_offset;
	char save_name[1024];
}save_node_t, *psave_node_t;
void memory_play_close(void);
int get_cur_playing_from_disk(file_list_t * handle,char *file_name,psave_node_t save_node,int type);//获取当前播放的文件在上次记录的时间

int auto_playing_from_disk(file_list_t * handle,char *file_name,psave_node_t save_node,int type);//插外接设备自动播放
int udisk_test(void);
void memmory_play_callback(void *data);
int memmory_play_init(void);

int memmory_play_set_state(int playing);
int memmory_play_get_state(void);

#endif //__MEMMROY_PLAY_H__