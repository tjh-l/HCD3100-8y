#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <kernel/lib/console.h>
#include <linux/bitops.h>
#include <linux/bits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <nuttx/fs/dirent.h>
#include <nuttx/fs/fs.h>
#include <nuttx/mtd/mtd.h>
#include <string.h>
#include <kernel/elog.h>
#include <kernel/drivers/hcusb.h>
#include <linux/mutex.h>
#include <sys/mman.h>

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/vindvp.h>
#include <hcuapi/tvtype.h>
#include <hcuapi/vidmp.h>
#include <linux/delay.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

/* 
 ** golbal value
 */
static AvPktHd hdr = { 0 };
static int g_vin_dvp_fd = -1;
static struct kshm_info g_video_read_hdl = { 0 };
static struct vindvp_to_kshm_setting to_kshm_setting = { 0 };

static uint8_t mjpg_data[512 * 1024]; // for mjpg image buffer

#define UVC_ROTATE_SUPPORT
#define ROTATE_WIDHT 480
#define ROTATE_HEIGHT 640

enum dvp_camera_mode {
	CAM_COLOR,
	CAM_BLACK_WHITE,
};

#define EVENT_CAM_COLOR (1 << CAM_COLOR)
#define EVENT_CAM_BLACK_WHITE (1 << CAM_BLACK_WHITE)

static SemaphoreHandle_t g_cam_mutex = NULL;


enum cam_state {
    CAM_STA_NOT_INIT,
    CAM_STA_COLOR,
    CAM_STA_BLACK_WHITE,
} g_cam_state;


static long diff_timeval_in_ms(struct timeval *new, struct timeval *start)
{
    if((new->tv_sec < start->tv_sec) ||
            ((new->tv_sec == start->tv_sec) &&
                (new->tv_usec < start->tv_usec)))
        return -1;

    return (new->tv_sec * 1000 + new->tv_usec / 1000) -
        (start->tv_sec * 1000 + start->tv_usec / 1000);
}


static int vin_dvp_set_to_kshm_setting(bool rotate_enable, bool scale_enable,
				       enum ROTATE_TYPE rotate_mode,
				       int scaled_width, int scaled_height)
{
	enum VINDVP_TO_KSHM_SCALE_MODE scale_mode =
		VIN_DVP_TO_KSHM_SCALE_BY_SIZE;
	enum VIDDEC_SCALE_FACTOR scale_factor = 0;

	to_kshm_setting.rotate_enable = rotate_enable;
	to_kshm_setting.scale_enable = scale_enable;
	to_kshm_setting.rotate_mode = rotate_mode;
	to_kshm_setting.scale_mode = scale_mode;
	if (scale_mode == VIN_DVP_TO_KSHM_SCALE_BY_FACTOR) {
		to_kshm_setting.scale_factor = scale_factor;
	} else {
		to_kshm_setting.scaled_width = scaled_width;
		to_kshm_setting.scaled_height = scaled_height;
	}
	printf("to_kshm_setting rotate_enable:%d scale_enable:%d rotate_mode=%d scale_mode=%d\n",
	       rotate_enable, scale_enable, rotate_mode, scale_mode);
	return 0;
}

static int dvp_camera_open(enum dvp_camera_mode cam_mode)
{
	enum VINDVP_VIDEO_DATA_PATH vpath = VINDVP_VIDEO_TO_KSHM;
	unsigned int combine_mode = VINDVP_COMBINED_MODE_DISABLE;
	enum VINDVP_BG_COLOR bg_color = VINDVP_BG_COLOR_BLACK;
	int stop_mode = VINDVP_BLACK_SRCREEN_ANYWAY;
	enum TVTYPE tv_sys = TV_NTSC;

	if (g_vin_dvp_fd >= 0)
		return 0;

	g_vin_dvp_fd = open("/dev/vindvp", O_RDWR);
	if (g_vin_dvp_fd < 0) {
		printf("[Error] Cannot open /dev/vindvp\n");
		return -1;
	}

	xSemaphoreTake(g_cam_mutex, portMAX_DELAY);

	if (cam_mode == CAM_COLOR) {
		vpath = VINDVP_VIDEO_TO_KSHM;
#ifdef UVC_ROTATE_SUPPORT
		vin_dvp_set_to_kshm_setting(true, true, ROTATE_TYPE_90,
					    ROTATE_WIDHT, ROTATE_HEIGHT);
		ioctl(g_vin_dvp_fd, VINDVP_SET_TO_KSHM_SETTING,
		      &to_kshm_setting);
#endif
	} else {
		vpath = VINDVP_VIDEO_TO_DE;
	}
	printf("\nvpath %d\n", vpath);
	ioctl(g_vin_dvp_fd, VINDVP_SET_VIDEO_DATA_PATH, vpath);
	ioctl(g_vin_dvp_fd, VINDVP_SET_COMBINED_MODE, combine_mode);
	ioctl(g_vin_dvp_fd, VINDVP_SET_BG_COLOR, bg_color);
	ioctl(g_vin_dvp_fd, VINDVP_SET_VIDEO_STOP_MODE, stop_mode);
	ioctl(g_vin_dvp_fd, VINDVP_SET_EXT_DEV_INPUT_PORT_NUM,
	      cam_mode); //FOR BR3A03
	if (vpath == VINDVP_VIDEO_TO_KSHM) {
		ioctl(g_vin_dvp_fd, VINDVP_VIDEO_KSHM_ACCESS,
		      &g_video_read_hdl);
	}
	ioctl(g_vin_dvp_fd, VINDVP_START, tv_sys);
	printf("exit dvp_camera_open\n");

	xSemaphoreGive(g_cam_mutex);

	return 0;
}

static int dvp_camera_close(void)
{
	printf("dvp_camera_close\n");

	if (g_vin_dvp_fd < 0)
		return 0;

	xSemaphoreTake(g_cam_mutex, portMAX_DELAY);

	ioctl(g_vin_dvp_fd, VINDVP_STOP);
	close(g_vin_dvp_fd);

	g_vin_dvp_fd = -1;
	xSemaphoreGive(g_cam_mutex);
}

static void __dvp_camera_capture_yuv_buf(unsigned char *image_buf, int len,
					 struct vindvp_video_info *video_info,
					 int rotate_mode)
{
	//Y 数据存在buf 这个地址里面
	int buf_size = 0;
	void *buf = NULL;

	if (g_vin_dvp_fd >= 0) {
		// printf("yuv: %d*%d:%d\n", video_info->width, video_info->height,
		//        video_info->frame_rate);

		buf_size = video_info->width * video_info->height * 4;

    	xSemaphoreTake(g_cam_mutex, portMAX_DELAY);
		buf = mmap(0, buf_size, PROT_READ | PROT_WRITE, MAP_SHARED,
			   g_vin_dvp_fd, 0);
    	xSemaphoreGive(g_cam_mutex);

		if (buf)
			memcpy((char *)image_buf, (char *)buf, len);
	}
}


static void lock_thread(void *args)
{
	struct vindvp_video_info video_info = { 0 };
    uint8_t reg_value = 0;
	uint8_t *data = &mjpg_data[0];
	struct timeval cur, start;
	int interval = 0;
	int __frames = 0;

	while (dvp_camera_open(CAM_COLOR))
		;

	gettimeofday(&start, NULL);

    while(1)
	{
		while (kshm_read(&g_video_read_hdl, &hdr, sizeof(AvPktHd)) !=
			sizeof(AvPktHd))
		{
			mdelay(1);
			continue;
		}

		while (kshm_read(&g_video_read_hdl, data, hdr.size) !=
				hdr.size)
			mdelay(1);

		gettimeofday(&cur, NULL);

		__frames++;
		gettimeofday(&cur, NULL);
		interval = diff_timeval_in_ms(&cur, &start);
		if(interval >= 5000) {
			printf("%d -- %d (%ld)\n", interval, __frames, 
				kshm_get_valid_size(&g_video_read_hdl));
			gettimeofday(&start, NULL);
			__frames = 0;
		}
	}

	dvp_camera_close();

	/* Delete current thread. */
	vTaskSuspend(NULL);
}

/* ---------------------  ---------------------  ---------------------  --------------------- */
/* ---------------------  ---------------------  ---------------------  --------------------- */
/* ---------------------  ---------------------  ---------------------  --------------------- */

int lock_test(int argc, char **argv)
{
	g_cam_mutex = xSemaphoreCreateMutex();

	if (!g_cam_mutex) {
		printf("[Error] cannot create mutex\n");
		return -1;
	}	

#ifdef UVC_ROTATE_SUPPORT
	hcusb_gadget_uvc_resolution_setting(ROTATE_WIDHT, ROTATE_HEIGHT);
#endif

	xTaskCreate(lock_thread, "lock_thread", 0x1000, NULL,
		    portPRI_TASK_NORMAL, NULL);

	printf(" ==> door lock performance demo\n");

	return 0;
}

CONSOLE_CMD(lock_test, NULL, lock_test, CONSOLE_CMD_MODE_SELF, "door lock demo")
