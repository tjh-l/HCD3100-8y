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
#include <freertos/event_groups.h>
#include <freertos/semphr.h>

/* ---------------------  ---------------------  ---------------------  --------------------- */
/* ---------------------  Marco Define           ---------------------  --------------------- */
/* ---------------------  ---------------------  ---------------------  --------------------- */

// #define UVC_ROTATE_SUPPORT // support rotate

/* Color Sensor Resolution*/
#define ROTATE_WIDHT 640 //480
#define ROTATE_HEIGHT 480 //640

/* Black/White Sensor Resolution*/
#define CAPTURE_ROTATE_WIDHT 640 // 480
#define CAPTURE_ROTATE_HEIGHT 480 //640

enum dvp_camera_mode {
    CAM_COLOR,
    CAM_BLACK_WHITE,
    CAM_COLOR_TOTAL,
};

#define EVENT_CAM_COLOR (1 << CAM_COLOR)
#define EVENT_CAM_BLACK_WHITE (1 << CAM_BLACK_WHITE)

extern int bf3a03_read(int port, uint8_t cmd, uint8_t *p_data, uint32_t len);
extern int bf3a03_write(int port, uint8_t cmd, uint8_t *p_data, uint32_t len);

/* ---------------------  ---------------------  ---------------------  --------------------- */
/* ---------------------  Golbal Vaule           ---------------------  --------------------- */
/* ---------------------  ---------------------  ---------------------  --------------------- */

/* used for UVC statistics */
static struct timeval cur, start;
static int __frames = 0;
static uint32_t __frames_size_avg = 0;

struct sensor_work {
    uint8_t *buf_yuv;
    uint8_t *buf_mjpg;
    SemaphoreHandle_t mutex;
    EventGroupHandle_t eventgp;
    enum dvp_camera_mode mode;

    int fd_dvp;
    struct kshm_info video_hld;
    AvPktHd hdr;
    struct vindvp_to_kshm_setting to_kshm_setting;

    bool is_uvc_on;
};

struct sensor_work *g_sensor_work;

/* ---------------------  ---------------------  ---------------------  --------------------- */
/* ---------------------  static functions       ---------------------  --------------------- */
/* ---------------------  ---------------------  ---------------------  --------------------- */

static long diff_timeval_in_ms(struct timeval *new, struct timeval *start)
{
    if ((new->tv_sec < start->tv_sec) ||
        ((new->tv_sec == start->tv_sec) && (new->tv_usec < start->tv_usec)))
        return -1;

    return (new->tv_sec * 1000 + new->tv_usec / 1000) -
           (start->tv_sec * 1000 + start->tv_usec / 1000);
}

static struct sensor_work *alloc_sensor_work(void)
{
    struct sensor_work *work;

    work = malloc(sizeof(struct sensor_work));
    if (!work) {
        printf(" [Error] cannot malloc enough buffer!!\n");
        return NULL;
    }
    memset(work, 0, sizeof(struct sensor_work));

    work->buf_yuv = malloc(CAPTURE_ROTATE_WIDHT * CAPTURE_ROTATE_HEIGHT);
    work->buf_mjpg = malloc(512 * 1024);
    if (!work->buf_yuv || !work->buf_mjpg) {
        printf(" [Error] cannot malloc enough buffer!!\n");
        goto error;
    }

    work->fd_dvp = -1;
    work->mode = CAM_COLOR_TOTAL;
    work->mutex = xSemaphoreCreateMutex();
    if (!work->mutex) {
        printf("[Error] Cannot create mutex\n");
        goto error;
    }

    work->eventgp = xEventGroupCreate();
    if (!work->eventgp) {
        printf(" [Error] Canot Create eventgroup\n");
        goto error;
    }
    return work;

error:
    if (work->eventgp)
        vEventGroupDelete(work->eventgp);
    if (work->mutex)
        vSemaphoreDelete(work->mutex);
    if (work->buf_yuv)
        free(work->buf_yuv);
    if (work->buf_mjpg)
        free(work->buf_mjpg);
    free(work);
    return NULL;
}

static int vin_dvp_set_to_kshm_setting(struct sensor_work *work,
                       bool rotate_enable, bool scale_enable,
                       enum ROTATE_TYPE rotate_mode,
                       int scaled_width, int scaled_height)
{
    enum VINDVP_TO_KSHM_SCALE_MODE scale_mode =
        VIN_DVP_TO_KSHM_SCALE_BY_SIZE;
    enum VIDDEC_SCALE_FACTOR scale_factor = 0;

    if (!work)
        return -1;

    work->to_kshm_setting.rotate_enable = rotate_enable;
    work->to_kshm_setting.scale_enable = scale_enable;
    work->to_kshm_setting.rotate_mode = rotate_mode;
    work->to_kshm_setting.scale_mode = scale_mode;
    if (scale_mode == VIN_DVP_TO_KSHM_SCALE_BY_FACTOR) {
        work->to_kshm_setting.scale_factor = scale_factor;
    } else {
        work->to_kshm_setting.scaled_width = scaled_width;
        work->to_kshm_setting.scaled_height = scaled_height;
    }
    printf("to_kshm_setting rotate_enable:%d scale_enable:%d rotate_mode=%d scale_mode=%d\n",
           rotate_enable, scale_enable, rotate_mode, scale_mode);
    return 0;
}

static int dvp_camera_open(struct sensor_work *work,
               enum dvp_camera_mode cam_mode)
{
    enum VINDVP_VIDEO_DATA_PATH vpath = VINDVP_VIDEO_TO_KSHM;
    unsigned int combine_mode = VINDVP_COMBINED_MODE_DISABLE;
    enum VINDVP_BG_COLOR bg_color = VINDVP_BG_COLOR_BLACK;
    int stop_mode = VINDVP_BLACK_SRCREEN_ANYWAY;
    enum TVTYPE tv_sys = TV_NTSC;
    int fd;

    if (!work || cam_mode >= CAM_COLOR_TOTAL || work->fd_dvp > 0) {
        printf(" [Error] open sensor fail\n");
        return -1;
    }

    work->fd_dvp = open("/dev/vindvp", O_RDWR);
    if (work->fd_dvp < 0) {
        printf(" [Error] Cannot open /dev/vindvp\n");
        return -1;
    }
    fd = work->fd_dvp;

    xSemaphoreTake(work->mutex, portMAX_DELAY);

    if (cam_mode == CAM_COLOR) {
        vpath = VINDVP_VIDEO_TO_KSHM;
#ifdef UVC_ROTATE_SUPPORT
        vin_dvp_set_to_kshm_setting(work, true, true, ROTATE_TYPE_90,
                        ROTATE_WIDHT, ROTATE_HEIGHT);
        ioctl(fd, VINDVP_SET_TO_KSHM_SETTING, &work->to_kshm_setting);
#endif
    } else {
        vpath = VINDVP_VIDEO_TO_DE;
    }

    ioctl(fd, VINDVP_SET_VIDEO_DATA_PATH, vpath);
    ioctl(fd, VINDVP_SET_COMBINED_MODE, combine_mode);
    ioctl(fd, VINDVP_SET_BG_COLOR, bg_color);
    ioctl(fd, VINDVP_SET_VIDEO_STOP_MODE, stop_mode);
    ioctl(fd, VINDVP_SET_EXT_DEV_INPUT_PORT_NUM, cam_mode);
    if (vpath == VINDVP_VIDEO_TO_KSHM)
        ioctl(fd, VINDVP_VIDEO_KSHM_ACCESS, &work->video_hld);
    ioctl(fd, VINDVP_START, tv_sys);

    work->mode = cam_mode;
    if(work->mode == CAM_COLOR) {
        xEventGroupSetBits(work->eventgp, EVENT_CAM_COLOR);
        printf(" [uvc] open color sensor\n");
    }else if(work->mode == CAM_BLACK_WHITE) {
        xEventGroupSetBits(work->eventgp, EVENT_CAM_BLACK_WHITE);
        printf(" [uvc] open black/white sensor\n");
    }
    xSemaphoreGive(work->mutex);
    return 0;
}

static int dvp_camera_close(struct sensor_work *work)
{
    if (!work || work->fd_dvp < 0)
        return -1;

    xSemaphoreTake(work->mutex, portMAX_DELAY);
    ioctl(work->fd_dvp, VINDVP_STOP);
    close(work->fd_dvp);
    work->fd_dvp = -1;
    if(work->mode == CAM_COLOR) {
        printf(" [uvc] close color sensor\n");
        xEventGroupClearBits(work->eventgp, EVENT_CAM_COLOR);
    }else if(work->mode == CAM_BLACK_WHITE) {
        printf(" [uvc] close black/white sensor\n");
        xEventGroupClearBits(work->eventgp, EVENT_CAM_BLACK_WHITE);
    }
    xSemaphoreGive(work->mutex);
    return 0;
}

static int dvp_cam_capture_yuv(struct sensor_work *work,
                    unsigned char *image_buf, int len,
                    struct vindvp_video_info *video_info,
                    int rotate_mode)
{
    int buf_size = 0, ret = -1;
    void *buf = NULL;

    if (!work || work->fd_dvp < 0)
        return -1;

    xSemaphoreTake(work->mutex, portMAX_DELAY);
    buf_size = video_info->width * video_info->height * 4;
    buf = mmap(0, buf_size, PROT_READ | PROT_WRITE, MAP_SHARED,
           work->fd_dvp, 0);
    if (buf) {
        memcpy((char *)image_buf, (char *)buf, len);
        ret = 0;
    }
    xSemaphoreGive(work->mutex);
    return ret;
}

static int dvp_cam_mjpg_hook(uint8_t **frame, int *frame_sz)
{
    struct sensor_work *work = g_sensor_work;
    int index = 0;
    uint8_t *data;
    int interval = 0;
    EventBits_t eventgp_bits;
    int ret = 0;

    if (!work || !work->is_uvc_on) {
        *frame = NULL;
        *frame_sz = 0;
        ret = -1;
        goto _exit_hook;
    }

    /* wait for color sensor ready */
    while(1) {
        eventgp_bits = xEventGroupWaitBits(work->eventgp,
                        EVENT_CAM_COLOR | EVENT_CAM_BLACK_WHITE,
                        pdFALSE, pdFALSE, 10);
        if(eventgp_bits == 0) {
            // continue;
            *frame = NULL;
            *frame_sz = 0;
            ret = 0;
            goto _exit_hook;
        } else if (eventgp_bits == (EVENT_CAM_COLOR | EVENT_CAM_BLACK_WHITE)) {
            printf(" [Error] error case: color and black/white sensor "
             "should not ready at the same time\n");
            ret = -1;
            goto _exit_hook;
        }
        else if (eventgp_bits == EVENT_CAM_BLACK_WHITE) {
            *frame = NULL;
            *frame_sz = 0;
            ret = 0;
            goto _exit_hook;
        } 
        else if (eventgp_bits == EVENT_CAM_COLOR) 
            break; // color sensor is ready

        if(++index > 50) {
            printf(" [demo] get frame timeout\n");
            *frame = NULL;
            *frame_sz = 0;
            ret = 0;
            goto _exit_hook;
        }
    }

    // xSemaphoreTake(work->mutex, portMAX_DELAY);

    data = work->buf_mjpg;
    index = 0;

    while ((kshm_read(&work->video_hld, &work->hdr, sizeof(AvPktHd)) !=
        sizeof(AvPktHd))) {
        msleep(10);
        if(++index > 30) {
            printf(" [lock] kshm read headler info timeout\n");
            ret = -1;
            goto _exit_hook;            
        }
    }

    index = 0;
    while ((kshm_read(&work->video_hld, data, work->hdr.size) !=
        work->hdr.size)) {
        msleep(1);
        if(++index > 100) {
            printf(" [lock] cannot get correct frame data\n");
            ret = -1;
            goto _exit_hook;  
        }        
    }

    /* add jpg ending content */
    if ((data[work->hdr.size - 2] != 0xff) &&
        (data[work->hdr.size - 1] != 0xd9)) {
        data[work->hdr.size + 0] = 0xff;
        data[work->hdr.size + 1] = 0xd9;
        work->hdr.size += 2;
        printf(" [lock] add jpg ending content (frame size: %d bytes)\n",
               work->hdr.size);
    }

    *frame = data;
    *frame_sz = work->hdr.size;

    __frames++;
    __frames_size_avg += work->hdr.size;
    gettimeofday(&cur, NULL);
    interval = diff_timeval_in_ms(&cur, &start);
    if (interval >= 5000) {
        printf("%d -- %d (%ld)(%ld)\n", interval, __frames,
               kshm_get_valid_size(&work->video_hld),
               __frames_size_avg /  __frames);
        gettimeofday(&start, NULL);
        __frames = 0;
        __frames_size_avg = 0;
    }

    // xSemaphoreGive(work->mutex);

_exit_hook:
    return ret;
}

static int dvp_camera_switch(bool is_turn_on)
{
    struct sensor_work *work = g_sensor_work;

    printf(" [uvc] dvp camera : %s\n", is_turn_on ? "on" : "off");

    if (!work) {
        printf(" [Error] Please check door lock init\n");
        return -1;
    }
    work->is_uvc_on = is_turn_on;

    if (is_turn_on == true) {
        dvp_camera_close(work);
        dvp_camera_open(work, CAM_COLOR);
        gettimeofday(&start, NULL);
    } else {
        dvp_camera_close(work);
        dvp_camera_open(work, CAM_BLACK_WHITE);
    }
    return 0;
}


static void yuv_sensor_thread(void *args)
{
    struct vindvp_video_info video_info = { 0 };
    uint8_t reg_value = 0;
    struct sensor_work *work = (struct sensor_work *)args;
    EventBits_t eventgp_bits;

    if(!work) {
        printf(" [Error] Please check demo init\n");
        vTaskSuspend(NULL);
        return;
    }

    for(;;) {
        xEventGroupWaitBits(work->eventgp,
                        EVENT_CAM_BLACK_WHITE,
                        pdFALSE, pdFALSE, portMAX_DELAY);

        xSemaphoreTake(work->mutex, portMAX_DELAY);
        if(work->fd_dvp < 0) {
            xSemaphoreGive(work->mutex);
            continue;
        }
        ioctl(work->fd_dvp, VINDVP_GET_VIDEO_INFO, &video_info);
        ioctl(work->fd_dvp, VINDVP_SET_YUV_BUF_CAPTURE_FORMAT,
            CAPTURE_FORMAT_Y_ONLY);
        #ifdef UVC_ROTATE_SUPPORT
            ioctl(work->fd_dvp, VINDVP_SET_YUV_BUF_CAPTURE_ROTATE_MODE,
                ROTATE_TYPE_90);
        #endif
        xSemaphoreGive(work->mutex);

        while(1) {
            eventgp_bits = xEventGroupWaitBits(work->eventgp,
                        EVENT_CAM_BLACK_WHITE,
                        pdFALSE, pdFALSE, 0);
            if(eventgp_bits != EVENT_CAM_BLACK_WHITE)
                break;
            if(work->is_uvc_on)  // !note: 只有在uvc不工作时候才采集yuv图像
                break;

            #if 0 // !note: 只是用于调试, 不去做yuv数据的获取
            xEventGroupWaitBits(work->eventgp,
                            EVENT_CAM_COLOR,
                            pdFALSE, pdFALSE, portMAX_DELAY);
            #else
            dvp_cam_capture_yuv(work, work->buf_yuv,
                        CAPTURE_ROTATE_WIDHT * CAPTURE_ROTATE_HEIGHT,
                        &video_info, ROTATE_TYPE_90);
            #endif
        }
    }

    /* Delete current thread. */
    vTaskSuspend(NULL);
}


/* ---------------------  ---------------------  ---------------------  --------------------- */
/* ---------------------  ---------------------  ---------------------  --------------------- */
/* ---------------------  ---------------------  ---------------------  --------------------- */

int door_lock_demo(int argc, char **argv)
{
    elog_set_filter_tag_lvl("composite", ELOG_LVL_WARN);

    g_sensor_work = alloc_sensor_work();

#ifdef UVC_ROTATE_SUPPORT
    hcusb_gadget_uvc_resolution_setting(ROTATE_WIDHT, ROTATE_HEIGHT);
#endif

    hcusb_gadget_uvc_specified_init(get_udc_name(USB_PORT_1),
                    dvp_camera_switch,
                    dvp_cam_mjpg_hook);
    hcusb_set_mode(USB_PORT_1, MUSB_PERIPHERAL);

    xTaskCreate(yuv_sensor_thread, "yuv_sensor_thread", 0x1000, g_sensor_work,
            portPRI_TASK_NORMAL, NULL);

    /* default using black/white sensor */
    dvp_camera_open(g_sensor_work, CAM_BLACK_WHITE);
  
    printf(" ==> door lock demo\n");

    return 0;
}

CONSOLE_CMD(lock, NULL, door_lock_demo, CONSOLE_CMD_MODE_SELF,
        "door lock demo (video only)")
