/**
* @file
* @brief                hudi bluetooth interface
* @par Copyright(c):    Hichip Semiconductor (c) 2024
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <hcuapi/sci.h>

#include <hudi_com.h>
#include <hudi_log.h>
#include <hudi_bluetooth.h>
#include "hudi_bluetooth_inter.h"


static hudi_bluetooth_input_evt_cb g_hudi_input_event_func = NULL;

#ifdef __HCRTOS__

#include <kernel/lib/fdt_api.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <kernel/lib/fdt_api.h>
#include <kernel/completion.h>
#include <kernel/delay.h>


static QueueHandle_t g_hudi_bluetooth_mutex = {0};
static int g_mutex_inited = 0;

static int _hudi_bluetooth_mutex_lock(void)
{
    if (g_mutex_inited == 0)
    {
        g_hudi_bluetooth_mutex = xSemaphoreCreateMutex();
        g_mutex_inited = 1;
    }
    xSemaphoreTake(g_hudi_bluetooth_mutex, portMAX_DELAY);
    return 0;
}

static int _hudi_bluetooth_mutex_unlock(void)
{
    xSemaphoreGive(g_hudi_bluetooth_mutex);
    return 0;
}

static int _hudi_bluetooth_dts_resource_parse(hudi_bluetooth_instance_t *inst)
{
    int ret = -1;
    const char *status = NULL;
    const char *path = NULL;
    int np = fdt_node_probe_by_path("/hcrtos/bluetooth");
    if (np < 0)
    {
        return ret;
    }
    fdt_get_property_string_index(np, "status", 0, &status);
    fdt_get_property_string_index(np, "devpath", 0, &path);
    strcpy(inst->dts_res.status, status);
    strcpy(inst->dts_res.dev_path, path);
    if (!strcmp(status, "okay"))
    {
        ret = 0;
    }

    return ret;
}


static void _hudi_bluetooth_polling_thread(void *arg)
{
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)arg;
    inst->polling_sem = (struct completion *)malloc(sizeof(struct completion));
    char rx_buf[UART_RX_BUFFER_SIZE] = {0};
    memset(rx_buf, 0, sizeof(rx_buf));
    struct pollfd fds[1];
    nfds_t nfds = 1;
    fds[0].fd = inst->fd;
    fds[0].events  = POLLIN | POLLRDNORM;
    fds[0].revents = 0;
    int ret  = -1;
    int poll_ret = -1;
    int byte_count = 0;
    char byte;

    init_completion(inst->polling_sem);
    while (!inst->polling_stop)
    {
        poll_ret = poll(fds, nfds, 0);
        if (poll_ret > 0)
        {
            if (fds[0].revents & (POLLRDNORM | POLLIN))
            {
                byte_count = read(inst->fd, rx_buf, sizeof(rx_buf));
                hudi_log(HUDI_LL_DEBUG, "%s,%d,%d\n", __func__, __LINE__, byte_count);
            }

            if (byte_count > 0 && byte_count <= UART_RX_BUFFER_SIZE)
            {
                if (!inst->module || !inst->module->proto_parse)
                {
                    hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error\n");
                    break;
                }

                ret = inst->module->proto_parse((hudi_handle)inst, rx_buf, byte_count);
                if (ret < 0)
                {
                    hudi_log(HUDI_LL_DEBUG, "Bluetooth protocal parse buffer date error \n");
                }
                byte_count = 0;
                memset(rx_buf, 0, sizeof(rx_buf));
            }
        }

        usleep(50 * 1000);
    }

    complete(inst->polling_sem);
    vTaskDelete(NULL);
    usleep(1000);

    return;
}

static int _hudi_bluetooth_polling_start(hudi_bluetooth_instance_t *inst)
{
    int ret = -1;
    ret = xTaskCreate(_hudi_bluetooth_polling_thread, "_hudi_bluetooth_polling_thread",
                      0x1000, (void *)inst, portPRI_TASK_NORMAL, NULL);
    inst->event_polling = 1;
    hudi_log(HUDI_LL_ERROR, "%s,%d\n", __func__, __LINE__);

    return ret;
}

static int _hudi_bluetooth_polling_stop(hudi_bluetooth_instance_t *inst)
{
    int ret = -1;
    inst->polling_stop = 1 ;
    ret = wait_for_completion_timeout(inst->polling_sem, 3000);
    if (ret <= 0)
    {
        hudi_log(HUDI_LL_ERROR, "polling stop timeout \n");
    }
    inst->event_polling = 0;
    if (inst->polling_sem)
    {
        free(inst->polling_sem);
    }
    return ret;
}

int hudi_bluetooth_uart_configure(int fd, int baudrate)
{
    struct sci_setting bt_sci;
    bt_sci.parity_mode = PARITY_NONE;
    bt_sci.bits_mode = bits_mode_default;
    switch (baudrate)
    {
        case 9600:
            ioctl(fd, SCIIOC_SET_BAUD_RATE_9600, NULL);
            break;
        case 115200:
            ioctl(fd, SCIIOC_SET_BAUD_RATE_115200, NULL);
            break;
        default:
            ioctl(fd, SCIIOC_SET_BAUD_RATE_115200, NULL);
            break;
    }
    ioctl(fd, SCIIOC_SET_SETTING, &bt_sci);
    return 0;
}

#else
#include <termios.h>

static pthread_mutex_t g_hudi_bluetooth_mutex = PTHREAD_MUTEX_INITIALIZER;

static int _hudi_bluetooth_mutex_lock(void)
{
    pthread_mutex_lock(&g_hudi_bluetooth_mutex);
    return 0;
}

static int _hudi_bluetooth_mutex_unlock(void)
{
    pthread_mutex_unlock(&g_hudi_bluetooth_mutex);
    return 0;
}

static int _hudi_bluetooth_dts_string_get(const char *path, char *string, int size)
{
    int ret = -1;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return ret;
    }
    ret = read(fd, string, size);
    close(fd);
    return ret;
}

static int _hudi_bluetooth_dts_resource_parse(hudi_bluetooth_instance_t *inst)
{
    int ret = -1;
    _hudi_bluetooth_dts_string_get(BLUETOOTH_DEVICE_TREE_PATH "status", inst->dts_res.status, sizeof(inst->dts_res.status));
    if (!strcmp(inst->dts_res.status, "okay"))
    {
        _hudi_bluetooth_dts_string_get(BLUETOOTH_DEVICE_TREE_PATH "devpath", inst->dts_res.dev_path, sizeof(inst->dts_res.dev_path));
        ret = 0;
    }

    return ret;
}

static void *_hudi_bluetooth_polling_thread(void *arg)
{
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)arg;
    char rx_buf[UART_RX_BUFFER_SIZE] = {0};
    struct pollfd fds[1];
    nfds_t nfds = 1;
    fds[0].fd = inst->fd;
    fds[0].events  = POLLIN | POLLRDNORM;
    fds[0].revents = 0;
    int ret = -1;
    int poll_ret = -1;
    int byte_count = 0;
    char byte;

    while (!inst->polling_stop)
    {
        poll_ret = poll(fds, nfds, 0);
        if (poll_ret > 0)
        {
            if (fds[0].revents & (POLLRDNORM | POLLIN))
            {
                byte_count = read(inst->fd, rx_buf, sizeof(rx_buf));
                hudi_log(HUDI_LL_DEBUG, "%s,%d,%d\n", __func__, __LINE__, byte_count);
            }
            if (byte_count > 0 && byte_count <= UART_RX_BUFFER_SIZE)
            {
                if (!inst->module || !inst->module->proto_parse)
                {
                    hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error\n");
                    break;
                }

                ret = inst->module->proto_parse((hudi_handle)inst, rx_buf, byte_count);
                if (ret < 0)
                {
                    hudi_log(HUDI_LL_ERROR, "Bluetooth protocal parse buffer date error \n");
                }
                byte_count = 0;
                memset(rx_buf, 0, sizeof(rx_buf));
            }
        }
        usleep(50 * 1000);
    }

    usleep(1000);
    return NULL;
}

static int _hudi_bluetooth_polling_start(hudi_bluetooth_instance_t *inst)
{
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_create(&inst->polling_tid, &thread_attr, _hudi_bluetooth_polling_thread, (void *)inst);
    pthread_attr_destroy(&thread_attr);

    inst->event_polling = 1;

    return 0;
}

static int _hudi_bluetooth_polling_stop(hudi_bluetooth_instance_t *inst)
{
    inst->polling_stop = 1;
    pthread_join(inst->polling_tid, NULL);
    inst->event_polling = 0;
    return 0;
}


int hudi_bluetooth_uart_configure(int fd, int baudrate)
{
    struct termios newtio, oldtio;

    if ( tcgetattr( fd, &oldtio) != 0)
    {
        return -1;
    }

    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag |= CLOCAL | CREAD;
    newtio.c_cflag &= ~CSIZE;

    newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
    newtio.c_oflag  &= ~OPOST;   /*Output*/

    newtio.c_cflag |= CS8;

    newtio.c_cflag &= ~PARENB;

    switch (baudrate)
    {
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
            break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
            break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
            break;
        default:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
            break;
    }

    newtio.c_cflag &= ~CSTOPB;

    newtio.c_cc[VMIN]  = 1;
    newtio.c_cc[VTIME] = 0;

    tcflush(fd, TCIFLUSH);

    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0)
    {
        return -1;
    }

    return 0;
}


#endif


int hudi_bluetooth_open(hudi_handle *handle)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = NULL;

    if (!handle)
    {
        hudi_log(HUDI_LL_ERROR, "Invaild bluetooth pararmeters\n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    inst = (hudi_bluetooth_instance_t *)malloc(sizeof(hudi_bluetooth_instance_t));
    memset(inst, 0, sizeof(hudi_bluetooth_instance_t));

    ret = _hudi_bluetooth_dts_resource_parse(inst);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth dts resource configuration failure\n");
        free(inst);
        _hudi_bluetooth_mutex_unlock();
        return ret;
    }

    inst->fd = open(inst->dts_res.dev_path, O_RDWR | O_NONBLOCK);
    if (inst->fd < 0)
    {
        hudi_log(HUDI_LL_ERROR, "open uart fail\n");
        free(inst);
        _hudi_bluetooth_mutex_unlock();
        return -1;
    }

    inst->inited = 1;
    *handle = inst;

    _hudi_bluetooth_mutex_unlock();

    return ret;
}

int hudi_bluetooth_close(hudi_handle handle)
{
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "bluetooth_inst not open \n");
        return -1;
    }

    _hudi_bluetooth_mutex_lock();

    if (inst->event_polling)
    {
        _hudi_bluetooth_polling_stop(inst);
    }
    memset(inst, 0, sizeof(hudi_bluetooth_instance_t));
    free(inst);

    _hudi_bluetooth_mutex_unlock();
    return 0;

}

int hudi_bluetooth_module_register(hudi_handle handle, hudi_bluetooth_module_st *module)
{
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "bluetooth_inst not open \n");
        return -1;
    }

    inst->module = module;

    return 0;
}

int hudi_bluetooth_write(hudi_handle handle, char *buf, int len)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "bluetooth_inst not open \n");
        return ret;
    }

    ret = write(inst->fd, buf, len);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "bluetooth write error\n");
        return ret;
    }

    return 0;

}

int hudi_bluetooth_read(hudi_handle handle, char *buf, int len)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited)
    {
        hudi_log(HUDI_LL_ERROR, "bluetooth_inst not open \n");
        return ret;
    }

    ret = read(inst->fd, buf, len);
    if (ret < 0)
    {
        hudi_log(HUDI_LL_ERROR, "bluetooth read error\n");
        return ret;
    }

    return 0;

}

int hudi_bluetooth_input_event_register(hudi_bluetooth_input_evt_cb func)
{
    if (func == NULL)
    {
        hudi_log(HUDI_LL_ERROR, "Invaild input parameters\n");
        return -1;
    }
    g_hudi_input_event_func = func;
    return 0;
}

int hudi_bluetooth_input_event_send(hudi_bluetooth_input_event_t event)
{
    if (g_hudi_input_event_func == NULL)
    {
        hudi_log(HUDI_LL_ERROR, "Input event send error\n");
        return -1;
    }
    g_hudi_input_event_func(event);
    return 0;
}

int hudi_bluetooth_event_register(hudi_handle handle, hudi_bluetooth_cb func, void *user_data)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !func)
    {
        hudi_log(HUDI_LL_ERROR, "Invalid param \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    inst->notifier = func;
    inst->user_data = user_data;
    if (!inst->event_polling)
    {
        _hudi_bluetooth_polling_start(inst);
    }

    _hudi_bluetooth_mutex_unlock();
    return ret;

}

int hudi_bluetooth_rf_on(hudi_handle handle)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->rf_on)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->rf_on(handle);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_rf_off(hudi_handle handle)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->rf_off)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->rf_off(handle);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_scan_async(hudi_handle handle)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->scan_async)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->scan_async(handle);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_scan_abort(hudi_handle handle)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->scan_abort)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->scan_abort(handle);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_connect(hudi_handle handle, char *device_mac)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->connect)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->connect(handle, device_mac);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_disconnect(hudi_handle handle)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->disconnect)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->disconnect(handle);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_ignore(hudi_handle handle)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->ignore)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->ignore(handle);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}


int hudi_bluetooth_volume_set(hudi_handle handle, int vol)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->volume_set)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->volume_set(handle, vol);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_volume_get(hudi_handle handle, int *vol)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->volume_get)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->volume_get(handle, vol);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_audio_input_ch_set(hudi_handle handle, hudi_bluetooth_audio_ch_e  ch)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->audio_input_ch_set)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->audio_input_ch_set(handle, ch);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_audio_input_ch_get(hudi_handle handle, hudi_bluetooth_audio_ch_e *ch)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->audio_input_ch_get)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->audio_input_ch_get(handle, ch);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_audio_output_ctrl(hudi_handle handle, hudi_bluetooth_audio_ctrl_e ctrl)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->audio_output_ctrl)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->audio_output_ctrl(handle, ctrl);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_mode_switch(hudi_handle handle, hudi_bluetooth_mode_e mode)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->mode_switch)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->mode_switch(handle, mode);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_mode_get(hudi_handle handle, hudi_bluetooth_mode_e *mode)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->mode_get)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->mode_get(handle, mode);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_status_get(hudi_handle handle, hudi_bluetooth_status_e *status)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->status_get)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->status_get(handle, status);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_localname_set(hudi_handle handle, char *name)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->localname_set)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->localname_set(handle, name);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_localname_get(hudi_handle handle, char *name)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->localname_get)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->localname_get(handle, name);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_firmware_ver_get(hudi_handle handle, char *version)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->firmware_ver_get)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->firmware_ver_get(handle, version);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_extio_func_set(hudi_handle handle, int pinpad, hudi_bluetooth_extio_func_e func)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->extio_func_set)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->extio_func_set(handle, pinpad, func);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_extio_input(hudi_handle handle, int pinpad, int *value)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->extio_input)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->extio_input(handle, pinpad, value);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_extio_output(hudi_handle handle, int pinpad, int value)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->extio_output)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->extio_output(handle, pinpad, value);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_extio_pwm_set(hudi_handle handle, int pinpad, int freq, int duty)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->extio_pwm_set)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->extio_pwm_set(handle, pinpad, freq, duty);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_standby_key_set(hudi_handle handle, int standby_key)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->standby_key_set)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->standby_key_set(handle, standby_key);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_standby_enter(hudi_handle handle)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->standby_enter)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->standby_enter(handle);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_reboot(hudi_handle handle)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->reboot)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->reboot(handle);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_ir_usercode_set(hudi_handle handle, int ir_usercode)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->ir_usercode_set)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->ir_usercode_set(handle, ir_usercode);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_signal_channel_set(hudi_handle handle, char *bitmap)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->signal_ch_set)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->signal_ch_set(handle, bitmap);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_signal_channel_get(hudi_handle handle, char *bitmap)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->signal_ch_get)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->signal_ch_get(handle, bitmap);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}

int hudi_bluetooth_extio_standby_status_set(hudi_handle handle, int pinpad, int value)
{
    int ret = -1;
    hudi_bluetooth_instance_t *inst = (hudi_bluetooth_instance_t *)handle;
    if (!inst || !inst->inited || !inst->module || !inst->module->extio_standby_status_set)
    {
        hudi_log(HUDI_LL_ERROR, "Bluetooth inst operation error \n");
        return ret;
    }

    _hudi_bluetooth_mutex_lock();

    ret = inst->module->extio_standby_status_set(handle, pinpad, value);

    _hudi_bluetooth_mutex_unlock();
    return ret;
}


