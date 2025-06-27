#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "screen.h"
#include "memmory_play.h"
#include "factory_setting.h"
#include "file_mgr.h"
#include <dirent.h>
#include "mp_ctrlbarpage.h"
#include "mp_fspage.h"

#ifdef HC_MEDIA_MEMMORY_PLAY 
#define MAX_SAVE_MEDIA_TYPE	3
int memmory_play_pthread_stop(void);
volatile int memmory_play_upgrade = 0;

int get_cur_playing_from_disk(file_list_t * handle,char *file_name,psave_node_t save_node,int type)
{
	play_info_t *media_play_info = NULL;
	media_play_info = sys_data_search_media_info(file_name, type);

	if (NULL == media_play_info || media_play_info->path[0] == '\0')
		return -1;
	save_node->save_time = media_play_info->current_time;
	save_node->save_offset= media_play_info->current_offset;
	save_node->idx = media_play_info->last_page;

	return 0;
}

int auto_playing_from_disk(file_list_t * handle,char *file_name,psave_node_t save_node,int type)
{
	char *path_name = NULL;
	play_info_t	*media_play_info  = NULL;

	partition_info_t * m_cur_partinfo=mmp_get_partition_info();
	media_play_info = sys_data_get_media_info(type);

	if (NULL == media_play_info || strlen(media_play_info->path) < 3)
		return -1;

	path_name = m_cur_partinfo->used_dev;
	if(0!=memcmp(media_play_info->path,path_name,strlen(path_name)))
		return -1;

	if(access(media_play_info->path,F_OK)){
		printf("%s(), %d. access %s fail!\n", __func__, __LINE__, media_play_info->path);				
		return -1;
	}

	save_node->save_time = media_play_info->current_time;
	memset(save_node->save_name, 0, MAX_FILE_NAME);
	memcpy(save_node->save_name,media_play_info->path,sizeof(save_node->save_name));
	save_node->save_offset= media_play_info->current_offset;
	save_node->idx = media_play_info->last_page;

	printf("%s(). mem play:%s, time:%d\n", __func__, \
		save_node->save_name, (int)save_node->save_time);	

	return 0;

}


static bool if_can_mem_play(void)
{
    lv_obj_t *screen;

    screen = lv_scr_act();
    if (
    	main_page_scr == screen ||
    	volume_scr == screen ||
    	ui_mainpage == screen ||
    	ui_subpage == screen ||
    	ui_fspage == screen
    	){
    	//auto play support in the screens.
    	return true;
	}
    else{
    	return false;
    }
}

/*
* Do not compare the whole path, we should skip the mount path. 
* Because sometimes plugin/plugout fast, the mount path may be different(/media/sda1->/media/sda2, etc)
* So we may change the root/mount path to the recently mount path, keep sub path.
* 
* for example: in_path="/media/hdd/video/1.mp4", root="/mnt/sda"
*  then out="/mnt/sda/video/1.mp4"
*/
static int change_root_path(char *in_path, char *out_path, char *root) 
{
	char *tmp = in_path;
	char *token = NULL;
	char root_tmp[64] = {'\0'};
	char find = 0;
	int slash_idx = 0;
	int slash_cnt = 0;
	
    if (in_path == NULL || out_path == NULL || root == NULL) {
        printf("Invalid input: path or root is NULL\n");
        return -1;
    }
	
	//backup root path
	strncpy(root_tmp, root, sizeof(root_tmp)-1);

	//how many slashes should be skipped
    token = strtok(root_tmp, "/");
	while(token !=NULL){
		 slash_cnt ++;
		 token = strtok(NULL, "/");
	}
	slash_cnt = slash_cnt + 1;
	
	//skip the original root path according slashes
	while(*tmp != '\0') {
		if (*tmp++ == '/') {
			slash_idx ++;
			//skip 3 slashes
			if (slash_idx >= slash_cnt) {
				find = 1;
				break;
			}
		}
	}

	//copy the change root path to output path
	if (1 == find) {
		strcpy(out_path, root);
		if (root[strlen(root)-1] != '/')
			strcat(out_path, "/");
		strcat(out_path, tmp);
	}
	
	return find ? 0 : -1;
}


static int check_cur_device_has_memmory_play(void)
{
	int i,j;

	play_info_t	*media_play_info = NULL;
	char  *dev_name;
	media_type_t cur_type;

	media_type_t change_type[MAX_SAVE_MEDIA_TYPE] = {
		MEDIA_TYPE_VIDEO,
		MEDIA_TYPE_MUSIC,
		MEDIA_TYPE_TXT,
	};

	if(!if_can_mem_play())
        return -1;

	partition_info_t * m_cur_partinfo = mmp_get_partition_info();
	dev_name = api_get_partition_info_by_index(m_cur_partinfo->count-1);
	if (!dev_name)
		return -1;

	char tmp_path[HCCAST_MEDIA_MAX_NAME] = {'\0'};
	for(i = 0; i < MAX_SAVE_MEDIA_TYPE; i++)
	{
		cur_type = change_type[i];
		media_play_info = sys_data_get_media_info(cur_type);
		if (NULL == media_play_info)
			return -1;

		int record_cnt = RECORD_ITEM_NUM_MAX;
		if(cur_type == MEDIA_TYPE_MUSIC)
			record_cnt = 1;

		for(j = 0;j < record_cnt; j++)
		{	
			if (0 == strncmp((media_play_info+j)->path, dev_name, strlen(dev_name))){
			    //root path is same, do not need changed.

				if (0 == access((media_play_info+j)->path, F_OK)) {
					projector_set_some_sys_param(P_MEM_PLAY_MEDIA_TYPE,cur_type);
					if (j != 0){
						//just move the memory paly file path to the first position.
						media_info_save_memmory(&media_play_info[j], (media_play_info+j)->path,(media_play_info+j)->type);
					}
					return 0;
				}
			} else {
			    //root path is different, change the root path.
			    
				if (!change_root_path((media_play_info+j)->path, tmp_path, dev_name)) {
					if (0 == access(tmp_path, F_OK)) {
						strcpy((media_play_info+j)->path, tmp_path);				
						projector_set_some_sys_param(P_MEM_PLAY_MEDIA_TYPE,cur_type);
						//change the path
						media_info_save_memmory(&media_play_info[j], (media_play_info+j)->path,(media_play_info+j)->type);
						printf("change memory play:%s\n", (media_play_info+j)->path);
						return 0;
					}
				}
			}
		}
	}

	return -1;
}

static int m_mem_playing = 0;
int memmory_play_set_state(int playing)
{
	m_mem_playing = playing;
}

int memmory_play_get_state(void)
{
	return m_mem_playing;
}

int memmory_play_init(void)
{
	if (memmory_play_get_state())
		return 0;

	if(!check_cur_device_has_memmory_play())
    {
    	control_msg_t ctl_msg = {0};
		if (read_file_list())
			return -1;

		memmory_play_set_state(1);

		ctl_msg.msg_type = MSG_TYPE_MP_MEMMORY_PLAY;
		api_control_send_msg(&ctl_msg);
	}
	return 0;
}
#endif

