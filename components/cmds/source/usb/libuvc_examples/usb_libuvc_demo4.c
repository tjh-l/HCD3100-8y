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
#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/auddec.h>
#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>
#include <sys/ioctl.h>
#include <hcuapi/snd.h>
#include <hcuapi/dis.h>
#include <errno.h>
#include <kernel/delay.h>

#include <libuvc/libuvc.h>

#define ALOGI log_i
#define ALOGE log_e
#define ALOGD log_d

#ifndef MKTAG
#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#endif

struct mjpg_decoder {
    struct video_config cfg;
    int fd;
};

void *decode_hld;

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
    ALOGI("video sync_mode: %d\n", p->cfg.sync_mode);
    p->cfg.decode_mode = VDEC_WORK_MODE_KSHM;

    p->cfg.pic_width = width;
    p->cfg.pic_height = height;
    p->cfg.frame_rate = fps * 1000;
    ALOGE("frame_rate: %d\n", (int)p->cfg.frame_rate);

    p->cfg.pixel_aspect_x = 1;
    p->cfg.pixel_aspect_y = 1;
    p->cfg.preview = 0;
    p->cfg.extradata_size = size;
    memcpy(p->cfg.extra_data, extradata, size);

    p->cfg.rotate_enable = rotate_enable;
    ALOGI("video rotate_enable: %d\n", p->cfg.rotate_enable);

    p->fd = open("/dev/viddec", O_RDWR);
    if (p->fd < 0) {
        ALOGE("Open /dev/viddec error.");
        free(p);
        return NULL;
    }

    if (ioctl(p->fd, VIDDEC_INIT, &(p->cfg)) != 0) {
        ALOGE("Init viddec error.");
        close(p->fd);
        free(p);
        return NULL;
    }

    ioctl(p->fd, VIDDEC_START, 0);
    ALOGI("fd: %d\n", p->fd);
    return p;
}

static int mjpg_decode(void *phandle, uint8_t *video_frame, size_t packet_size,
        uint32_t rotate_mode)
{
    struct mjpg_decoder *p = (struct mjpg_decoder *)phandle;
    AvPktHd pkthd = { 0 };
    pkthd.pts = -1;
    pkthd.dur = 0;
    pkthd.size = packet_size;
    pkthd.flag = AV_PACKET_ES_DATA;
    pkthd.video_rotate_mode = rotate_mode;

    // ALOGI("video_frame: %p, packet_size: %d", video_frame, packet_size);
    if (write(p->fd, (uint8_t *)&pkthd, sizeof(AvPktHd)) !=
        sizeof(AvPktHd)) {
        ALOGE("Write AvPktHd fail\n");
        return -1;
    }

    int ret = write(p->fd, video_frame, packet_size);
    if(ret != (int)packet_size)
    {
        ALOGE("Write video_frame error fail (%d/%d)\n", ret, (int)packet_size);
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
        mjpg_decode(decode_hld, frame->data, frame->data_bytes, 0);
        break;
    case UVC_COLOR_FORMAT_YUYV:
        break;
    default:
        break;
    }

    if (frame->sequence % 100 == 0)
        printf(" * got image %lu --> %ld\n", frame->sequence, frame->data_bytes);
}


static uvc_context_t *ctx = NULL;
static uvc_device_t *dev = NULL;
static uvc_device_handle_t *devh = NULL;
static uvc_stream_ctrl_t ctrl;
static bool uvc_is_running = false;

static int libuvc_setup(void)
{
    // uvc_context_t *ctx;
    // uvc_device_t *dev;
    // uvc_device_handle_t *devh;
    // uvc_stream_ctrl_t ctrl;
    uvc_error_t res;

    /* Initialize a UVC service context. Libuvc will set up its own libusb
   * context. Replace NULL with a libusb_context pointer to run libuvc
   * from an existing libusb context. */
    res = uvc_init(&ctx, NULL);

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
    } else {
        puts("Device found");

        /* Try to open the device: requires exclusive access */
        res = uvc_open(dev, &devh);

        if (res < 0) {
            uvc_perror(res, "uvc_open"); /* unable to open device */
        } else {
            puts("Device opened");

            /* Print out a message containing all the information that libuvc
             * knows about the device */
            uvc_print_diag(devh, stderr);

            const uvc_format_desc_t *format_desc =
                uvc_get_format_descs(devh);
            const uvc_frame_desc_t *frame_desc =
                format_desc->frame_descs;
            enum uvc_frame_format frame_format;
            int width = 1280;
            int height = 720;
            int fps = 30;

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

            printf("\nFirst format: (%4s) %dx%d %dfps\n",
                   format_desc->fourccFormat, width, height, fps);

			width = 1280;
			height = 720;
			// width = 640;
			// height = 480;
            printf("\ncurrent format: (%4s) %dx%d %dfps\n",
                   format_desc->fourccFormat, width, height, fps);

            /* Try to negotiate first stream profile */
            res = uvc_get_stream_ctrl_format_size(
                devh, &ctrl, /* result stored in ctrl */
                frame_format, width, height,
                fps /* width, height, fps */
            );

            /* Print out the result */
            uvc_print_stream_ctrl(&ctrl, stderr);

            if (res < 0) {
                uvc_perror(res, "get_mode"); /* device doesn't provide a matching stream */
            } else {
                decode_hld = mjpg_decode_init(width, height, fps, NULL, 0, 0); // init mjpg decoder
                if (!decode_hld) {
                    printf("cannot init mjpg decode\n");
                    return -1;
                }
                puts("mjpg display decoder init");

                /* Start the video stream. The library will call user function cb:
                 *   cb(frame, (void *) 12345)
                 */
                res = uvc_start_streaming(devh, &ctrl, uvc_cb, NULL, 0);
                // res = uvc_start_streaming(devh, &ctrl, NULL, NULL, 0);
                if (res < 0) {
                    uvc_perror(
                        res,
                        "start_streaming"); /* unable to start stream */
                } else {
                    puts("Streaming...");

                    uvc_is_running = true;

                    /* enable auto exposure - see uvc_set_ae_mode documentation */
                    // puts("Enabling auto exposure ...");
                    // const uint8_t
                    //     UVC_AUTO_EXPOSURE_MODE_AUTO = 2;
                    // res = uvc_set_ae_mode(
                    //     devh,
                    //     UVC_AUTO_EXPOSURE_MODE_AUTO);
                    // if (res == UVC_SUCCESS) {
                    //     puts(" ... enabled auto exposure");
                    // } else if (res == UVC_ERROR_PIPE) {
                    //     /* this error indicates that the camera does not support the full AE mode;
                    //      * try again, using aperture priority mode (fixed aperture, variable exposure time) */
                    //     puts(" ... full AE not supported, trying aperture priority mode");
                    //     const uint8_t
                    //         UVC_AUTO_EXPOSURE_MODE_APERTURE_PRIORITY =
                    //             8;
                    //     res = uvc_set_ae_mode(
                    //         devh,
                    //         UVC_AUTO_EXPOSURE_MODE_APERTURE_PRIORITY);
                    //     if (res < 0) {
                    //         uvc_perror(
                    //             res,
                    //             " ... uvc_set_ae_mode failed to enable aperture priority mode");
                    //     } else {
                    //         puts(" ... enabled aperture priority auto exposure mode");
                    //     }
                    // } else {
                    //     uvc_perror(
                    //         res,
                    //         " ... uvc_set_ae_mode failed to enable auto exposure mode");
                    // }

                    // sleep(10); /* stream for 10 seconds */

                    /* End the stream. Blocks until last callback is serviced */
                    // uvc_stop_streaming(devh);
                    // puts("Done streaming.");
                }
            }

            /* Release our handle on the device */
            // uvc_close(devh);
            // puts("Device closed");
        }

        /* Release the device descriptor */
        // uvc_unref_device(dev);
    }

    /* Close the UVC context. This closes and cleans up any existing device handles,
   * and it closes the libusb context if one was not provided. */
    // uvc_exit(ctx);
    // puts("UVC exited");

    // mjpg_decoder_destroy(decode_hld);
    // puts("mjpg display decoder destroy");

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

static int usb_cam_player(int argc, char **argv)
{
    if(uvc_is_running == true) {
        printf(" usb camera is working now !!!\n");
        return -1;
    }

    osd_onoff(false);  /* close OSD */
    libuvc_setup();
    return 0;
}

static int usb_cam_player_stop(int argc, char **argv)
{
    if(uvc_is_running == false) {
        printf(" usb camera is not working now !!!\n");
        return -1;
    }

    /* End the stream. Blocks until last callback is serviced */
    uvc_stop_streaming(devh);

    /* Release our handle on the device */
    uvc_close(devh);
    
    /* Release the device descriptor */
    uvc_unref_device(dev);

    /* Close the UVC context. This closes and cleans up any existing device handles,
   * and it closes the libusb context if one was not provided. */
    uvc_exit(ctx);

    mjpg_decoder_destroy(decode_hld);
    
    /* open OSD */
    osd_onoff(true);

    uvc_is_running = false;

    return 0;
}


#include <kernel/lib/console.h>
CONSOLE_CMD(uvc, NULL, NULL, CONSOLE_CMD_MODE_SELF, "uvc demo")
CONSOLE_CMD(start, "uvc", usb_cam_player, CONSOLE_CMD_MODE_SELF, "libuvc example 2")
CONSOLE_CMD(stop, "uvc", usb_cam_player_stop, CONSOLE_CMD_MODE_SELF, "libuvc example 2")
CONSOLE_CMD(uvc_demo2, NULL, usb_cam_player, CONSOLE_CMD_MODE_SELF,
        "libuvc example 2")
