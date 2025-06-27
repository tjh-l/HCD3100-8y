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
int get_cur_playing_from_disk(file_list_t * handle,char *file_name,psave_node_t save_node,int type);//��ȡ��ǰ���ŵ��ļ����ϴμ�¼��ʱ��

int auto_playing_from_disk(file_list_t * handle,char *file_name,psave_node_t save_node,int type);//������豸�Զ�����
int udisk_test(void);
void memmory_play_callback(void *data);
int memmory_play_init(void);

int memmory_play_set_state(int playing);
int memmory_play_get_state(void);

#endif //__MEMMROY_PLAY_H__