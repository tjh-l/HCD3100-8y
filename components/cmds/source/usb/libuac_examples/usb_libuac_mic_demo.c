#define LOG_TAG "libuac"
#define ELOG_OUTPUT_LVL ELOG_LVL_INFO
#include <kernel/elog.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <kernel/delay.h>
#include <nuttx/fs/fs.h>
#include <hcuapi/usbuac.h>

static pthread_t uac_test_pthread_hdl;
static int fd_usbmic = -1;

static void *uac_test_pthread(void *arg)
{
    #define TEST_PCM_FILE   "/media/sda1/tt.pcm"

    AvPktHd pkthd = { 0 };
    int index, ret;
    int fd = (int)arg;
    uint8_t *pkg = NULL;
    uint32_t pkg_maxlen = 300 * 1024;
    uint32_t pkg_len;
    uint32_t total_len;
    struct pollfd events;

    FILE *fp = fopen(TEST_PCM_FILE, "w+");

    pkg = malloc(pkg_maxlen);
    if(!pkg) {
        log_e("uac_test_pthread: Failed to malloc %lu memory !!\n", pkg_maxlen);
        goto __uac_pthread_exit;
    }

    index = 0;
    total_len = 0;

    events.fd = fd;
    events.events = POLLIN | POLLRDNORM | POLLHUP;

    while(1) {
        if(poll(&events, 1, -1) < 0) {
            log_e("[Error] poll timeout !!\n");
            goto __uac_pthread_exit;
        }

        if(events.revents & POLLHUP) {
            log_i(" shut down /dev/usbmic0 \n");
            goto __uac_pthread_exit;
        }

        ret = read(fd, &pkthd, sizeof(AvPktHd));
        if(ret != sizeof(AvPktHd)) {
            log_e(" read kshm header error (%d/%lu)\n", ret, sizeof(AvPktHd));
            continue;
        }

        ret = read(fd, pkg + total_len, pkthd.size);
        if(ret != pkthd.size) {
            log_e(" read kshm data error (%d/%d)\n", ret, pkthd.size);
            continue;
        }

        total_len += pkthd.size;
        if(total_len > pkg_maxlen * 0.8) {
            if(fp) {
                printf(" write %s %lu\n", TEST_PCM_FILE, total_len);
                fwrite(pkg, total_len, 1, fp);
                fflush(fp);
                fclose(fp);
                fp = fopen(TEST_PCM_FILE, "a+");
            }
            total_len = 0;
        }
    }

__uac_pthread_exit:
    if(pkg)
        free(pkg);
    if(fp)
        fclose(fp);
    return NULL;
}

int uac_test_start(int argc, char **argv)
{
    struct usbuac_config config;
    int fd, ret = 0;
    
    fd = open("/dev/usbmic0", O_RDWR);
    if(fd < 0) {
        log_e("Cannot open /dev/usbmic0\n");
        ret = -1;
        goto __uac_exit;
    }

    config.channels = 1;
    config.bits_per_coded_sample = 16;
    config.sample_rate = 44100;
    ret = ioctl(fd, USBUAC_MIC_SET_CONFIG, &config);
    if(ret) {
        log_e("Fail to access ioctl cmd USBUAC_MIC_GET_CONFIG\n");
        goto __uac_exit;
    }
    
    memset(&config, 0, sizeof(config));
    ret = ioctl(fd, USBUAC_MIC_GET_CONFIG, &config);
    if(ret) {
        log_e("Fail to access ioctl cmd USBUAC_MIC_GET_CONFIG\n");
        goto __uac_exit;
    }
    printf("usb microphone -- rate:%lu, channels:%u, bits:%lu \n",
        config.sample_rate, config.channels, config.bits_per_coded_sample);

    
    ret = ioctl(fd, USBUAC_MIC_START, NULL);
    if(ret) {
        log_e("Fail to start usb microphone\n");
        goto __uac_exit;
    }

    printf("usb microphone pthread create\n");
    if (pthread_create(&uac_test_pthread_hdl, NULL, uac_test_pthread, (void *)fd) != 0) {
        log_e("Failed to create uac libusb pthread\n");
        goto __uac_exit;
    }

    printf("usb microphone start\n");
    
    fd_usbmic = fd;
    return 0;

__uac_exit:
    if(fd > 0)
        close(fd);
    return ret;
}

int uac_test_stop(int argc, char **argv)
{
    int ret;

    if(fd_usbmic < 0)
        return 0;

    printf(" stop usb microphone\n");
    ret = ioctl(fd_usbmic, USBUAC_MIC_STOP, NULL);
    if(ret) {
        log_e("Fail to start usb microphone\n");
        return -1;
    }

    printf(" wait for usb mic pthread exit\n");
    pthread_join(uac_test_pthread_hdl, NULL);
    
    printf(" close /dev/usbmic0\n");
    close(fd_usbmic);
    fd_usbmic = -1;
    return 0;
}

#include <kernel/lib/console.h>
CONSOLE_CMD(uac_start, NULL, uac_test_start, CONSOLE_CMD_MODE_SELF, "libuac example")
CONSOLE_CMD(uac_stop, NULL, uac_test_stop, CONSOLE_CMD_MODE_SELF, "libuac example")
