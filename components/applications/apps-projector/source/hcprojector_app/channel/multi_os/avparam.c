#include "app_config.h"

#ifdef  MULTI_OS_SUPPORT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <hcuapi/audsink.h>

#include <hcuapi/dis.h>
#include <hcuapi/pq.h>
#include <hcuapi/persistentmem.h>
#include <hcuapi/standby.h>
#ifdef __HCRTOS__
#include <hcuapi/gpio.h>
#include <hcuapi/fb.h>
#endif

#include "avparam.h"

/* 
*  init_ppios: gpios to be set when enter multi-os 
*  exit_gpios: gpios to be set when exit multi-os
*/
//#define PROJECTOR_C3_Q6   1

static avparam_t *m_avparam = NULL;

static gpio_def_t init_gpios[]={
#ifdef  PROJECTOR_C3_Q6
	{PINPAD_LVDS_DP7, GPIO_DIR_OUTPUT, 1},
	{PINPAD_LVDS_DN6, GPIO_DIR_OUTPUT, 1},
	{PINPAD_L02, GPIO_DIR_OUTPUT, 1},
	{PINPAD_L04, GPIO_DIR_OUTPUT, 1},
#endif	
};
#define INIT_GPIO_NUMS  (sizeof(init_gpios)/sizeof(init_gpios[0]))

static gpio_def_t exit_gpios[]={
#ifdef  PROJECTOR_C3_Q6
	{PINPAD_LVDS_DN6, GPIO_DIR_OUTPUT, 0},
#endif	
};
#define EXIT_GPIO_NUMS  (sizeof(exit_gpios)/sizeof(exit_gpios[0]))

// avparam data struct init
int avparam_data_init(void)
{
	m_avparam = malloc(sizeof(avparam_t));
	if(m_avparam == NULL)
		return -1;
	memset(m_avparam, 0, sizeof(avparam_t));
	memset(&m_avparam->init_gpio_def, -1, GPIO_NUM_MAX*sizeof(gpio_def_t));
	memset(&m_avparam->exit_gpio_def, -1, GPIO_NUM_MAX*sizeof(gpio_def_t));
	return 0;
}

int avparam_data_deinit(void)
{
	if(m_avparam != NULL){
		//if(m_avparam->pq_setting.name != NULL)
		//	free(m_avparam->pq_setting.name);
		free(m_avparam);
	}
	return 0;
}
int avparam_set_rotate_param(int rotate, int h_flip, int v_flip)
{
	if(m_avparam){
		m_avparam->rotate = rotate;
		m_avparam->h_flip = h_flip;
		m_avparam->v_flip = v_flip;
		printf("rotate %d, h_flip:%d, v_flip: %d\n", rotate,h_flip,v_flip);
	}
}
// to do ...
int avparam_set_color_temp_param(struct pq_settings *pq_setting)
{
	if(m_avparam){
		//m_avparam->pq_setting = pq_setting;
		//printf("color temp:  %p %s \n",m_avparam->pq_setting->name,m_avparam->pq_setting->name);
	}
}

int avparam_set_gpio_def_param(void)
{
	if(m_avparam){
		memcpy(&m_avparam->init_gpio_def, &init_gpios, sizeof(init_gpios));
		memcpy(&m_avparam->exit_gpio_def, &exit_gpios, sizeof(exit_gpios));
	}
}

static int avparam_get_volume(uint8_t *volume)
{
	int snd_fd = -1;

	snd_fd = open(SOUND_DEV, O_WRONLY);
	if (snd_fd < 0) {
		printf ("open snd_fd %d failed\n", snd_fd);
		return -1;
	}

	ioctl(snd_fd, SND_IOCTL_GET_VOLUME, volume);
	//printf("Current volume is %d\n", *volume);
	close(snd_fd);
	return 0;
}

static int avparam_get_fb_enhance(hcfb_enhance_t *eh){
	int fd = -1;
	int ret = 0;

	fd = open(FB_DEV, O_RDWR);
	if( fd < 0){
		return -1;
	}

	ret = ioctl(fd, HCFBIOGET_ENHANCE, eh);
	if( ret != 0 ){
		printf("HCFBIOGET_ENHANCE failed\n");
		close(fd);
		fd = -1;
		return -1;
	}
	close(fd);
	printf("b: %d\n c: %d\n h:%d\n s:%d\n sharp: %d\n cscmode:%d\n",
			eh->brightness,eh->contrast,eh->hue,eh->saturation,eh->sharpness,eh->cscmode);
	return 0;
}

static int avparam_get_keystone_param(struct dis_keystone_param *pparm)
{
	int fd = open("/dev/dis" , O_RDWR);
	if (fd < 0) {
		return -1;
	}
	pparm->distype = DIS_TYPE_HD;
	if(ioctl(fd , DIS_GET_KEYSTONE_PARAM , pparm) !=0){
		close(fd);
		return -1;
	}
	printf(">> T: %d , B: %d , enable: %d\n", (int)pparm->info.width_up, (int)pparm->info.width_down, pparm->info.enable);
	close(fd);
	return 0;
}

/* store projector avparam parameters to flash */
static int api_set_avparam_node(avparam_t *m_avparam)
{
	struct persistentmem_node_create new_node;
	struct persistentmem_node node;
	avparam_t avparam_tmp;
	int fd = -1;

	fd = open("/dev/persistentmem", O_RDWR);
	if (fd < 0) {
		printf("Open /dev/persistentmem failed (%d)\n", fd);
		return -1;
	}

	node.id = PERSISTENTMEM_NODE_ID_AVPARAM;
	node.offset = 0;
	node.size = sizeof(avparam_t);
	node.buf = &avparam_tmp;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_GET, &node) < 0) {
		new_node.id = PERSISTENTMEM_NODE_ID_AVPARAM;
		new_node.size = sizeof(avparam_t);
		if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_CREATE, &new_node) < 0) {
			printf("create avparam node failed\n");
			close(fd);
			return -1;
		}
	}

	node.id = PERSISTENTMEM_NODE_ID_AVPARAM;
	node.offset = 0;
	node.size = sizeof(avparam_t);
	node.buf = m_avparam;
	if (ioctl(fd, PERSISTENTMEM_IOCTL_NODE_PUT, &node) < 0) {
		printf("Store avparam node failed\n");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

static int set_bootup_slot(unsigned long slot)
{
	int fd = open("/dev/standby", O_RDWR);
	if (fd < 0) {
		return -1;
	}
	ioctl(fd, STANDBY_SET_BOOTUP_SLOT, slot);
	close(fd);
	return 0;
}

int store_avparam(void)
{	
	unsigned long slot = 0;
	
	avparam_set_gpio_def_param();
	avparam_get_volume(&m_avparam->volume);
	avparam_get_keystone_param(&m_avparam->ks_param);
	avparam_get_fb_enhance(&m_avparam->fb_eh);
	api_set_avparam_node(m_avparam);

	slot = STANDBY_BOOTUP_SLOT(STANDBY_BOOTUP_SLOT_NR_SECONDARY, STANDBY_BOOTUP_SLOT_FEATURE_OSDLOGO_ON);
	set_bootup_slot(slot);
}

#endif

