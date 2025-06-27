#define LOG_TAG "usb_cam"
#define ELOG_OUTPUT_LVL ELOG_LVL_DEBUG
#include <kernel/elog.h>

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/auddec.h>
#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/dis.h>
#include <kernel/delay.h>

#include <libusb.h>
#include <libuvc/libuvc.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <nuttx/wqueue.h>

#ifndef MKTAG
#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#endif

/* ***************************************************************************************
 *  global value
 * ***************************************************************************************
 */

struct mjpg_decoder {
    struct video_config cfg;
    int fd;
};


struct hcusb_camera {
    /* for hcrtos display */
    void *decode_hld;

    /* for libuvc */
    uvc_context_t *uvc_ctx;
    uvc_device_t *dev;
    uvc_device_handle_t *devh;
    uvc_stream_ctrl_t ctrl;

    /* for libusb */
    struct libusb_context *usb_ctx;

    /* for libuvc demo */
    bool is_connected;
	struct work_s work;
};


static struct hcusb_camera *pcam_hld = NULL;
static void hcusb_camera_switch_on(void *parameter);
static void hcusb_camera_switch_off(void *parameter);

/* ***************************************************************************************
 *  decoder
 * ***************************************************************************************
 */
__attribute__((weak)) void set_aspect_mode(dis_tv_mode_e ratio,
                       dis_mode_e dis_mode)
{
}

static void *mjpg_decode_init(int width, int height, int fps, uint8_t *extradata, int size,
               int8_t rotate_enable)
{
    struct mjpg_decoder *p = malloc(sizeof(struct mjpg_decoder));
    memset(&p->cfg, 0, sizeof(struct video_config));

    set_aspect_mode(DIS_TV_16_9, DIS_PILLBOX);

    p->cfg.codec_id = HC_AVCODEC_ID_MJPEG;
    p->cfg.codec_tag = MKTAG('m', 'j', 'p', 'g');
    p->cfg.sync_mode = 0;
    log_i("video sync_mode: %d\n", p->cfg.sync_mode);
    p->cfg.decode_mode = VDEC_WORK_MODE_KSHM;

    p->cfg.pic_width = width;
    p->cfg.pic_height = height;
    p->cfg.frame_rate = fps * 1000;
    log_i("frame_rate: %d\n", (int)p->cfg.frame_rate);

    p->cfg.pixel_aspect_x = 1;
    p->cfg.pixel_aspect_y = 1;
    p->cfg.preview = 0;
    p->cfg.extradata_size = size;
    memcpy(p->cfg.extra_data, extradata, size);

    p->cfg.rotate_enable = rotate_enable;
    log_i("video rotate_enable: %d\n", p->cfg.rotate_enable);

    p->fd = open("/dev/viddec", O_RDWR);
    if (p->fd < 0) {
        log_e("Open /dev/viddec error.");
        return NULL;
    }

    if (ioctl(p->fd, VIDDEC_INIT, &(p->cfg)) != 0) {
        log_e("Init viddec error.");
        close(p->fd);
        free(p);
        return NULL;
    }

    ioctl(p->fd, VIDDEC_START, 0);
    log_i("fd: %d\n", p->fd);
    return p;
}

static int mjpg_decode(void *phandle, uint8_t *video_frame, size_t packet_size,
        uint32_t rotate_mode)
{
    struct mjpg_decoder *p = (struct mjpg_decoder *)phandle;
    AvPktHd pkthd = { 0 };
    int ret;

    pkthd.pts = -1;
    pkthd.dur = 0;
    pkthd.size = packet_size;
    pkthd.flag = AV_PACKET_ES_DATA;
    pkthd.video_rotate_mode = rotate_mode;

    // log_i("video_frame: %p, packet_size: %d", video_frame, packet_size);
    if (write(p->fd, (uint8_t *)&pkthd, sizeof(AvPktHd)) !=
        sizeof(AvPktHd)) {
        log_e("Write AvPktHd fail\n");
        return -1;
    }

    ret = write(p->fd, video_frame, packet_size);
    if (ret != (int)packet_size) {
        log_e("Write video_frame error fail (%d/%d)\n", ret, (int)packet_size);
        float tmp = 0;
        ioctl(p->fd, VIDDEC_FLUSH, &tmp);
        return -1;
    }
    return 0;
}

static void mjpg_decoder_flush(void *phandle)
{
    struct mjpg_decoder *p = (struct mjpg_decoder *)phandle;
    float tmp = 0;
    ioctl(p->fd, VIDDEC_FLUSH, &tmp);
}

static void mjpg_decoder_destroy(void *phandle)
{
    struct mjpg_decoder *p = (struct mjpg_decoder *)phandle;
    set_aspect_mode(DIS_TV_AUTO, DIS_PILLBOX);
    if (!p)
        return;

    if (p->fd > 0) {
        close(p->fd);
    }
    free(p);
}


/* ***************************************************************************************
 *  libuvc porting
 * ***************************************************************************************
 */

/* This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames.
 */
static void uvc_cb(uvc_frame_t *frame, void *ptr)
{
    uvc_error_t ret;

    //   printf("callback! frame_format = %d, width = %ld, height = %ld, length = %lu, ptr = %p\n",
    //     frame->frame_format, frame->width, frame->height, frame->data_bytes, ptr);

    switch (frame->frame_format) {
    case UVC_FRAME_FORMAT_H264:
        break;
    case UVC_COLOR_FORMAT_MJPEG:
        mjpg_decode(pcam_hld->decode_hld, frame->data, frame->data_bytes, 0);
        break;
    case UVC_COLOR_FORMAT_YUYV:
        break;
    default:
        break;
    }

    if (frame->sequence % 30 == 0)
        printf(" * got image %lu\n", frame->sequence);
}


static int camera_init(struct hcusb_camera *pcam_hld)
{
    uvc_context_t *ctx;
    uvc_device_t *dev;
    uvc_device_handle_t *devh;
    uvc_stream_ctrl_t *ctrl = &pcam_hld->ctrl;
    uvc_error_t res;


    /* Initialize a UVC service context. Libuvc will set up its own libusb
   * context. Replace NULL with a libusb_context pointer to run libuvc
   * from an existing libusb context. */
    res = uvc_init(&ctx, pcam_hld->usb_ctx);
    if (res < 0) {
        uvc_perror(res, "uvc_init");
        return res;
    }

    puts("UVC initialized");

    /* Locates the first attached UVC device, stores in dev */
    res = uvc_find_device(
        ctx, &dev, 0, 0,
        NULL); /* filter devices: vendor_id, product_id, "serial_num" */
    if (res < 0) {
        uvc_perror(res, "uvc_find_device"); /* no devices found */
        return -1;
    }

    puts("Device found");

    /* Try to open the device: requires exclusive access */
    res = uvc_open(dev, &devh);
    if (res < 0) {
        uvc_perror(res, "uvc_open"); /* unable to open device */
        return -1;
    }

    puts("Device opened");
    pcam_hld->uvc_ctx = ctx;
    pcam_hld->devh = devh;
    pcam_hld->dev = dev;

    /* Print out a message containing all the information that libuvc
        * knows about the device */
    uvc_print_diag(devh, stderr);

    const uvc_format_desc_t *format_desc =
        uvc_get_format_descs(devh);
    const uvc_frame_desc_t *frame_desc =
        format_desc->frame_descs;
    enum uvc_frame_format frame_format;
    int width = 800;
    int height = 600;
    int fps = 60;

    switch (format_desc->bDescriptorSubtype) {
    case UVC_VS_FORMAT_MJPEG:
        frame_format = UVC_COLOR_FORMAT_MJPEG;
        break;
    case UVC_VS_FORMAT_FRAME_BASED:
        frame_format = UVC_FRAME_FORMAT_H264;
        break;
    default:
        frame_format = UVC_FRAME_FORMAT_YUYV;
        break;
    }

    if (frame_desc) {
        width = frame_desc->wWidth;
        height = frame_desc->wHeight;
        fps = 10000000 /
                frame_desc->dwDefaultFrameInterval;
    }

    log_i("First format: (%4s) %dx%d %dfps\n",
            format_desc->fourccFormat, width, height, fps);

    /* Try to negotiate first stream profile */
    res = uvc_get_stream_ctrl_format_size(
        devh, ctrl, /* result stored in ctrl */
        frame_format, width, height,
        fps /* width, height, fps */
    );

    /* Print out the result */
    // uvc_print_stream_ctrl(ctrl, stderr);

    if (res < 0) {
        uvc_perror(res, "get_mode"); /* device doesn't provide a matching stream */
    } else {
        pcam_hld->decode_hld = mjpg_decode_init(width, height, fps, NULL, 0, 0); // init mjpg decoder
        if (!pcam_hld->decode_hld) {
            printf("cannot init mjpg decode\n");
            return -1;
        }
        puts("mjpg display decoder init");

        /* Start the video stream. The library will call user function cb:
            *   cb(frame, (void *) 12345)
            */
        res = uvc_start_streaming(devh, ctrl, uvc_cb, NULL, 0);
        if (res < 0) {
            uvc_perror(
                res,
                "start_streaming"); /* unable to start stream */
        } else {
            puts("Streaming...");

            /* enable auto exposure - see uvc_set_ae_mode documentation */
            puts("Enabling auto exposure ...");
            const uint8_t UVC_AUTO_EXPOSURE_MODE_AUTO = 2;
            res = uvc_set_ae_mode(
                devh,
                UVC_AUTO_EXPOSURE_MODE_AUTO);
            if (res == UVC_SUCCESS) {
                puts(" ... enabled auto exposure");
            } else if (res == UVC_ERROR_PIPE) {
                /* this error indicates that the camera does not support the full AE mode;
                    * try again, using aperture priority mode (fixed aperture, variable exposure time) */
                puts(" ... full AE not supported, trying aperture priority mode");
                const uint8_t
                    UVC_AUTO_EXPOSURE_MODE_APERTURE_PRIORITY =
                        8;
                res = uvc_set_ae_mode(
                    devh,
                    UVC_AUTO_EXPOSURE_MODE_APERTURE_PRIORITY);
                if (res < 0) {
                    uvc_perror(
                        res,
                        " ... uvc_set_ae_mode failed to enable aperture priority mode");
                } else {
                    puts(" ... enabled aperture priority auto exposure mode");
                }
            } else {
                uvc_perror(
                    res,
                    " ... uvc_set_ae_mode failed to enable auto exposure mode");
            }
        }
    }

    return 0;
}

static int camera_deinit(struct hcusb_camera *pcam_hld)
{
    /* End the stream. Blocks until last callback is serviced */
    // uvc_stream_stop();
    if(pcam_hld->devh) {
        uvc_stop_streaming(pcam_hld->devh);
        puts("Done streaming.");

        /* Release our handle on the device */
        uvc_close(pcam_hld->devh);
        puts("Device closed");
    }

    /* Release the device descriptor */
    if(pcam_hld->dev)
        uvc_unref_device(pcam_hld->dev);

    /* Close the UVC context. This closes and cleans up any existing device handles,
   * and it closes the libusb context if one was not provided. */
    if(pcam_hld->uvc_ctx)
        uvc_exit(pcam_hld->uvc_ctx);
    puts("UVC exited");

    if(pcam_hld->decode_hld) {
        mjpg_decoder_destroy(pcam_hld->decode_hld);
        puts("mjpg display decoder destroy");
    }

    pcam_hld->devh = NULL;
    pcam_hld->dev = NULL;
    pcam_hld->uvc_ctx = NULL;
    pcam_hld->decode_hld = NULL;

    return 0;
}



/* ***************************************************************************************
 *  libusb porting
 * ***************************************************************************************
 */

static int camera_hotplug_func(libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *user_data)
{
    struct libusb_device_descriptor desc;
    struct libusb_config_descriptor *config;
    int ret;

    if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event){
        ret = libusb_get_device_descriptor(device, &desc);
        if (LIBUSB_SUCCESS != ret)
            log_e("Error getting device descriptor\n");

        log_i("USB plug-in %p: %.4x:%.2x\n", device, desc.idVendor, desc.bDeviceClass);

#if 0
        if(0 != libusb_get_active_config_descriptor(device, &config)){
            log_e("Cannot get active config descriptor\n");
        }else{
            log_i("Get active config descriptor : \n");
            log_i("bLength:0x%x\n", config->bLength);
            log_i("bDescriptorType:0x%x\n", config->bDescriptorType);
            log_i("wTotalLength:0x%x\n", config->wTotalLength);
            log_i("bNumInterfaces:0x%x\n", config->bNumInterfaces);
            log_i("bConfigurationValue:0x%x\n", config->bConfigurationValue);
            log_i("iConfiguration:0x%x\n", config->iConfiguration);
            log_i("bmAttributes:0x%x\n", config->bmAttributes);
            log_i("MaxPower:0x%x\n\n", config->MaxPower);
        }
#endif        

        work_queue(HPWORK, &pcam_hld->work, hcusb_camera_switch_on, (void *)pcam_hld, 0);

    } else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
        log_i("USB plug-out %p\n", device);

        work_queue(HPWORK, &pcam_hld->work, hcusb_camera_switch_off, (void *)pcam_hld, 0);
    }

    return 0;
}


static void hotplug_event_daemon(void *arg)
{
    struct libusb_context *ctx = (struct libusb_context *)arg;
    while(1)
        libusb_handle_events_completed(ctx, NULL);

    log_w("hotplug_event_daemon exit .....\n");
    libusb_hotplug_deregister_callback(ctx, 0);
    libusb_exit(ctx);
    vTaskDelete(NULL);
}


static int camera_hotplug(struct libusb_context *ctx)
{
    int res;

    if(!ctx)
        return -1;

    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

    if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)){
        log_e("libusb do not support hotplug\n");
        return -1;
    }

    log_i("Registering for libusb hotplug events\n");
    res = libusb_hotplug_register_callback(ctx,
                                        LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | \
                                                    LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                                        LIBUSB_HOTPLUG_ENUMERATE,
                                        LIBUSB_HOTPLUG_MATCH_ANY,
                                        LIBUSB_HOTPLUG_MATCH_ANY,
                                        LIBUSB_HOTPLUG_MATCH_ANY,
                                        camera_hotplug_func,
                                        NULL,
                                        NULL);
    if (res == LIBUSB_SUCCESS)
        log_i("Register libusb hotplug success\n");
    else
        log_e("ERROR: Could not register for libusb hotplug events. %sc", libusb_error_name(res));

    xTaskCreate(hotplug_event_daemon, 
                (const char *)"libusb_daemon",
                configTASK_STACK_DEPTH,
                ctx, portPRI_TASK_NORMAL,
                NULL);
    return 0;
}

#include <hcuapi/dis.h>
#include <hcuapi/fb.h>
#include <kernel/fb.h>
static void osd_onoff(bool is_on)
{
    int fd;
    fd = open("/dev/fb0", O_RDWR);
    if(fd < 0) {
        printf("Cannot open /dev/fb0\n");
        return;
    }

    if(is_on == true) {
        if (ioctl(fd, FBIOBLANK, FB_BLANK_UNBLANK) != 0) {
            printf("fb0 ioctrl FBIOBLANK error\n");
            goto __osd_exit;
        }
    } else {
        if (ioctl(fd, FBIOBLANK, FB_BLANK_NORMAL) != 0) {
            printf("fb0 ioctrl FBIOBLANK error\n");
            goto __osd_exit;
        }
    }
__osd_exit:
    close(fd);
    return;
}

/* ***************************************************************************************
 *  libuvc demo
 * ***************************************************************************************
 */

static void hcusb_camera_switch_on(void *parameter)
{
    struct hcusb_camera *pcam_hld = 
        (struct hcusb_camera *)parameter;
    int ret;

    if(pcam_hld->is_connected == false) {
        ret = camera_init(pcam_hld);
        if(!ret) {
            osd_onoff(false);  /* close OSD */
            pcam_hld->is_connected = true;
            log_w(" ==> camera_init finish\n");
        }
    }
}

static void hcusb_camera_switch_off(void *parameter)
{
    struct hcusb_camera *pcam_hld = 
        (struct hcusb_camera *)parameter;
    int ret;

    if(pcam_hld->is_connected == true) {
        ret = camera_deinit(pcam_hld);
        if(!ret){
            osd_onoff(true);  /* open OSD */
            pcam_hld->is_connected = false;
            log_w(" ==> camera_deinit finish\n");
        }
    }
}


int hcusb_camera_probe(int argc, char** argv)
{
    struct libusb_context *usb_ctx = NULL;
    uvc_context_t *uvc_ctx;
    int ret;

    elog_set_filter_tag_lvl("usb_cam", ELOG_LVL_DEBUG);

    if(pcam_hld) {
        log_e("hcrtos usb camera has been probe already\n");
        return -1;
    }

    pcam_hld = malloc(sizeof(struct hcusb_camera));
    if(!pcam_hld) {
        log_e("not enough memory for malloc\n");
        return -1;
    }
    memset(pcam_hld, 0, sizeof(struct hcusb_camera));

    ret = libusb_init(&usb_ctx);
    if(ret) {
        log_e("libusb init error : errno:%d\n", ret);
        return -1;
    }
    pcam_hld->usb_ctx = usb_ctx;

    ret = camera_hotplug(usb_ctx);
    if(ret) {
        log_e("camera_hotplug error : errno:%d\n", ret);
        return -1;
    }

    log_i(" ==> hcusb_camera_probe exit...\n");
    return 0;
}


/* ***************************************************************************** */

#include <kernel/lib/console.h>
CONSOLE_CMD(uvc_demo3, NULL, hcusb_camera_probe, CONSOLE_CMD_MODE_SELF,
        "libuvc example 3")
