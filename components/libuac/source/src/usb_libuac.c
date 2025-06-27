#define LOG_TAG "libuac"
#define ELOG_OUTPUT_LVL ELOG_LVL_INFO
#include <kernel/elog.h>

#include <kernel/module.h>

#include <stdlib.h>
#include <stdio.h>
#include <libusb.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <hcuapi/kshm.h>
#include <hcuapi/common.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/fs/fs.h>
#include <kernel/delay.h>

#include <hcuapi/usbuac.h>
#include "libuac_internal.h"

//TODO: Optimize code structure later
static struct libusb_context *uac_ctx = NULL;
static struct libusb_device *uac_dev;
static struct libusb_device_handle *uac_devh;
static struct libusb_config_descriptor *uac_config;
static libusb_hotplug_callback_handle uac_hotplug_hdl;
static struct libusb_transfer *transfers[NUM_TRANSFERS];
static unsigned char *transfer_bufs[NUM_TRANSFERS];
static struct audio_streaming_descriptor *audio_stream_desc;
static pthread_t uac_pthread_hdl = 0;
static bool is_run = false;
static int kill_handler_thread = 0;
static uint8_t channels = DEF_CHANNELS;
static uint32_t sample_rate = DEF_SRATE;
static uint32_t bits_per_coded_sample = DEF_BITS;
static int mMaxPacketSize = 0;
static int _controlInterface = 0;
static int _microphone_inf = 0;
static int _alternateSetting = 0;
static uint8_t _microphoneEndpoint = 0;
static kshm_handle_t kshm_mic = NULL;
static uac_stream_handler_t __uac_handler;

static int _uac_device_init(void);
static void _uac_device_deinit(void);

static void _uac_callback(uint8_t *packet, uint32_t packet_size)
{
    AvPktHd pkthd = { 0 };
    int ret;

    if (!kshm_mic || (packet_size == 0))
        return;

    pkthd.size = packet_size;
    ret = kshm_write(kshm_mic, &pkthd, sizeof(AvPktHd));
    if (ret <= 0) {
        log_w("kshm buffer full... (%d)\n", ret);
        return;
    }

    ret = kshm_write(kshm_mic, (void *)packet, packet_size);
    if (ret <= 0) {
        log_w("kshm buffer full... (%d)\n", ret);
        return;
    }
}

static void _uac_wakeup_waitlist(void)
{
    struct uac_instance *ins;
    struct list_head *curr, *tmp;

    list_for_each_safe (curr, tmp, &__uac_handler.instance_list) {
        ins = (struct uac_instance *)curr;
        assert(ins->closed == false);
        if (ins && !list_empty(&ins->wait.task_list))
            wake_up(&ins->wait);
    }
}

static void _uac_process_payload_iso(struct libusb_transfer *transfer, uac_stream_handler_t *uac_handler)
{
    int packet_id;
    unsigned int len = 0;

    /* This is an isochronous mode transfer, so each packet has a payload transfer */
    for (packet_id = 0; packet_id < transfer->num_iso_packets; ++packet_id) {
        uint8_t *pktbuf;
        struct libusb_iso_packet_descriptor *pkt;
        pkt = transfer->iso_packet_desc + packet_id;
        if (pkt->status != 0) {
            log_w("bad packet (isochronous transfer); status: %d", pkt->status);
            continue;
        }
        pktbuf = libusb_get_iso_packet_buffer_simple(transfer, packet_id);
        len += pkt->actual_length;

        _uac_callback(pktbuf, pkt->actual_length);
    }

    if (len)
        _uac_wakeup_waitlist();

    if (++(uac_handler->frames) % 100 == 0) {
        log_d(" uac frames %d (len:%u)\n", uac_handler->frames, len);
    }

    if (libusb_submit_transfer(transfer) < 0) {
        log_e("error re-submitting URB\n");
        return;
    }
}

static void LIBUSB_CALL _uac_stream_callback(struct libusb_transfer *transfer)
{
    uac_stream_handler_t *uac_handler = transfer->user_data;
    int i = 0;

    switch (transfer->status) {
    case LIBUSB_TRANSFER_COMPLETED:
        if (transfer->num_iso_packets) {
            if (is_run) {
                /* This is an isochronous mode transfer, so each packet has a payload transfer */
                _uac_process_payload_iso(transfer, uac_handler);
            } else {
                /* Mark transfer as deleted. */
                for (i = 0; i < NUM_TRANSFERS; ++i) {
                    if (transfers[i] == transfer) {
                        free(transfer->buffer);
                        libusb_free_transfer(transfers[i]);
                        transfers[i] = NULL;
                        break;
                    }
                }

                /* If all libusb transfer is free, kill libuac pthread */
                for (i = 0; i < NUM_TRANSFERS; ++i) {
                    if (transfers[i] != NULL)
                        break;
                }
            }
        }
        break;

    case LIBUSB_TRANSFER_NO_DEVICE:
    case LIBUSB_TRANSFER_CANCELLED:
    case LIBUSB_TRANSFER_ERROR:
        /* Mark transfer as deleted. */
        for (i = 0; i < NUM_TRANSFERS; ++i) {
            if (transfers[i] == transfer) {
                free(transfer->buffer);
                libusb_free_transfer(transfers[i]);
                transfers[i] = NULL;
                break;
            }
        }

        /* If all libusb transfer is free, kill libuac pthread */
        for (i = 0; i < NUM_TRANSFERS; ++i) {
            if (transfers[i] != NULL)
                break;
        }
        break;

    case LIBUSB_TRANSFER_TIMED_OUT:
    case LIBUSB_TRANSFER_STALL:
    case LIBUSB_TRANSFER_OVERFLOW:
    default:
        log_e(" _uac_stream_callback fail (%d)\n", transfer->status);
        break;
    }
}

static void cancel_all_libusb_transfer(void)
{
    int i = 0;

    /* Attempt to cancel any running transfers, we can't free them just yet because they aren't
     *   necessarily completed but they will be free'd in _uac_stream_callback().
     */
    for (i = 0; i < NUM_TRANSFERS; i++) {
        if (transfers[i] != NULL)
            libusb_cancel_transfer(transfers[i]);
    }
}

static int cancel_iso_transfer(void)
{
    int i = 0, timeout = 0;

    cancel_all_libusb_transfer();

    /* Wait for transfers to be complete/cancel */
    do {
        for (i = 0; i < NUM_TRANSFERS; i++) {
            if (transfers[i] != NULL)
                break;
        }
        if (i == NUM_TRANSFERS)
            break;

        if(++timeout > 300) {
            cancel_all_libusb_transfer();
            timeout = 0;
        }
        msleep(1);
    } while (1);
}

static int fill_iso_transfer(void)
{
    int r = 0;
    int endpoint_bytes_per_packet = mMaxPacketSize;
    int packets_per_transfer = NUM_PACKETS;
    int total_transfer_size = endpoint_bytes_per_packet * NUM_PACKETS;
    int transfer_id = 0;
    struct libusb_transfer *transfer;

    __uac_handler.frames = 0;

    log_i("Mircophone: EndpointAddress:0x%x, per_packet:%d, packets:%d, total_transfer_size:%d\n",
          _microphoneEndpoint, endpoint_bytes_per_packet, packets_per_transfer, total_transfer_size);

    for (transfer_id = 0; transfer_id < NUM_TRANSFERS; ++transfer_id) {
        if (transfers[transfer_id] != NULL)
            continue;

        transfer = (struct libusb_transfer *)libusb_alloc_transfer(packets_per_transfer);
        transfers[transfer_id] = transfer;
        transfer_bufs[transfer_id] = (unsigned char *)malloc(total_transfer_size);
        if (transfer_bufs[transfer_id] == NULL) {
            log_e(" Cannot malloc transfer buffer for USB UAC\n");
            assert(0);
        }
        memset(transfer_bufs[transfer_id], 0, total_transfer_size);

        libusb_fill_iso_transfer(transfer, uac_devh, _microphoneEndpoint, transfer_bufs[transfer_id],
                     total_transfer_size, packets_per_transfer, _uac_stream_callback,
                     (void *)&__uac_handler, 5000);
        libusb_set_iso_packet_lengths(transfer, endpoint_bytes_per_packet);
    }

    for (transfer_id = 0; transfer_id < NUM_TRANSFERS; transfer_id++) {
        r = libusb_submit_transfer(transfers[transfer_id]);
        if (r != 0) {
            log_e("libusb_submit_transfer failed: %s\n", libusb_error_name(r));
            break;
        }
    }
    return r;
}

static int set_sample_rate(int rate)
{
    unsigned char data[3];
    int ret, crate;

    data[0] = (rate & 0xff);
    data[1] = (rate >> 8);
    data[2] = (rate >> 16);
    ret = libusb_control_transfer(uac_devh, USB_REQ_CS_ENDPOINT_SET, UAC_SET_CUR, 0x0100, _microphoneEndpoint, data,
                      sizeof(data), 1000);
    if (ret < 0) {
        log_e("%d:%d: cannot set freq %d to ep %#x (error:%s)\n", _microphone_inf, _alternateSetting, rate,
              _microphoneEndpoint, libusb_error_name(ret));
        return ret;
    }
    printf(" usb microphone sample rate is %d\n", rate);
    return 0;
}

static bool check_audio_stream_desc(const struct libusb_interface_descriptor *desc, uint8_t channels, uint8_t bits,
                    uint32_t sam_rate)
{
    uint8_t *buf = (uint8_t *)desc->extra;
    int offset = 0;
    int size = desc->extra_length;
    struct audio_streaming_format_descriptor *format_desc;
    struct audio_streaming_descriptor *type_desc;
    uint32_t srate = 0;

    while (offset < size) {
        buf = (uint8_t *)desc->extra + offset;
        if (buf[1] != 0x24) {
            log_e("error audio streaming descs, please check\n");
            break;
        }

        if (buf[2] == 0x1) { /* AS_GENERAL descriptor subtype */
            format_desc = (struct audio_streaming_format_descriptor *)buf;
            log_i("\t format:%x (0x1:PCM)\n", format_desc->wFormatTag);
        } else if (buf[2] == 0x2) { /* FORMAT_TYPE descriptor subtype */
            uint8_t *tmp;
            audio_stream_desc = (struct audio_streaming_descriptor *)buf;
            tmp = buf + 8;
            srate = tmp[0] | (tmp[1] << 8) | (tmp[2] << 16);
            log_i("\t ch:%u, frame_sz:%u, bits:%u, srate_cnt:%u srate:%lu\n",
                  audio_stream_desc->bNrChannels, audio_stream_desc->bSubframeSize,
                  audio_stream_desc->bBitResolution, audio_stream_desc->bSamFreqType, srate);
        }
        offset += buf[0];
    }

    if ((channels == audio_stream_desc->bNrChannels) && (bits == audio_stream_desc->bBitResolution) &&
        (sam_rate == srate))
        return true;
    else
        return false;
}

static bool check_uac_device(struct libusb_context *ctx, struct libusb_device *usb_dev)
{
    int interface_idx;
    int altsetting_idx;
    bool got_interface = false;
    struct libusb_device_descriptor desc;
    const struct libusb_interface_descriptor *if_desc = NULL;
    const struct libusb_interface *interface;
    struct libusb_config_descriptor *config;

    if (libusb_get_config_descriptor(usb_dev, 0, &config) != 0)
        return false;
    if (libusb_get_device_descriptor(usb_dev, &desc) != LIBUSB_SUCCESS)
        return false;

    if (desc.idVendor == 0x1d6b && desc.idProduct == 0x2) //!note: hichip usb root
        goto __check_exit;

    for (interface_idx = 0; interface_idx < config->bNumInterfaces; interface_idx++) {
        interface = &config->interface[interface_idx];
        for (altsetting_idx = 0; altsetting_idx < interface->num_altsetting; altsetting_idx++) {
            if_desc = &interface->altsetting[altsetting_idx];
            log_d(" interface#%d, alt#%d, bInterfaceClass:%d, bInterfaceSubClass:%d\n", interface_idx,
                  altsetting_idx, if_desc->bInterfaceClass, if_desc->bInterfaceSubClass);
        }

        // found audio steam interface
        if (if_desc && if_desc->bInterfaceClass == LIBUSB_CLASS_AUDIO &&\
             if_desc->bInterfaceSubClass == 0x2) {
            got_interface = true;
            uac_dev = usb_dev;
        }
    }

__check_exit:
    libusb_free_config_descriptor(config);
    return got_interface;
}

static int scan_audio_interface(libusb_device *usbDev)
{
    const struct libusb_interface_descriptor *if_desc;
    const struct libusb_interface *i_face;
    const struct libusb_endpoint_descriptor *endpoint;
    int r = 0;

    r = libusb_get_config_descriptor(usbDev, 0, &uac_config);
    log_d("scan_audio_interface\n");
    for (int interface_idx = 0; interface_idx < uac_config->bNumInterfaces; interface_idx++) {
        i_face = &uac_config->interface[interface_idx];
        if (i_face->altsetting->bInterfaceClass != LIBUSB_CLASS_AUDIO /*1*/) { // Audio, Control
            continue;
        }
        log_d("scan_audio_interface :%d\n", i_face->num_altsetting);

        for (int i = 0; i < i_face->num_altsetting; ++i) {
            if_desc = &i_face->altsetting[i];

            log_d("\n\talt %d\n", i);

            switch (if_desc->bInterfaceSubClass) {
            case 1:
                _controlInterface = if_desc->bInterfaceNumber;
                log_d("\t control_int:%d\n", _controlInterface);
                break;
            case 2:
                if (if_desc->bNumEndpoints) {
                    endpoint = if_desc->endpoint;

                    /* MICROPHONE (host-in) */
                    if ((endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN) == LIBUSB_ENDPOINT_IN) {
                        /* get FORMAT_TYPE_I data */
                        if (true == check_audio_stream_desc(if_desc, channels,
                                            bits_per_coded_sample,
                                            sample_rate)) {
                            _microphone_inf = if_desc->bInterfaceNumber;
                            _alternateSetting = if_desc->bAlternateSetting;
                            _microphoneEndpoint = endpoint->bEndpointAddress;
                            mMaxPacketSize = endpoint->wMaxPacketSize;
                        }
                        log_d("\t _microphone_inf:%d, _alternateSetting:%d, ep_addr:0x%x, mMaxPacketSize:%d, \n",
                              if_desc->bInterfaceNumber, if_desc->bAlternateSetting,
                              endpoint->bEndpointAddress, endpoint->wMaxPacketSize);
                    }
                }
                break;
            }
        }
    }
    libusb_free_config_descriptor(uac_config);

    log_i("found correct interface for MICROPHONE\n");
    log_i("\tcontrol_intf:%d, stream_intf:%d, stream_intf_alt:%d, stream_ep:0x%x, stream_ep_maxpkgsz:%d\n",
          _controlInterface, _microphone_inf, _alternateSetting, _microphoneEndpoint, mMaxPacketSize);
    return r;
}

static int operate_interface(libusb_device_handle *devh, int interface_number)
{
    int r = 0;

    //detach_kernel_driver
    r = libusb_kernel_driver_active(devh, interface_number);
    if (r == 1) { //find out if kernel driver is attached
        log_d("Kernel Driver Active\n");
        if (libusb_detach_kernel_driver(devh, interface_number) == 0) //detach it
            log_d("Kernel Driver Detached!\n");
    }
    log_d("kernel detach interface_number:%d\n", interface_number);

    //claim_interface
    r = libusb_claim_interface(devh, interface_number);
    log_d("claim_interface r:%s\n", libusb_error_name(r));
    if (r != 0) {
        log_e("Error claiming interface: %s\n", libusb_error_name(r));
        return r;
    }
    log_d("claim_interface r:%d\n", r);
    return r;
}

static int interface_claim_if(libusb_device_handle *devh)
{
    int r = 0;

    r = operate_interface(devh, _microphone_inf);
    if (r < 0)
        return r;

    r = libusb_set_interface_alt_setting(devh, _microphone_inf, _alternateSetting);
    if (r != 0)
        return r;
    return 0;
}

static int hotplug_callback(struct libusb_context *ctx, struct libusb_device *device, libusb_hotplug_event event,
                void *user_data)
{
    struct libusb_device_descriptor desc;
    libusb_get_device_descriptor(device, &desc);
    int ret;

    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        log_i("[ctx:%p] Device attached: %04x:%04x\n", ctx, desc.idVendor, desc.idProduct);
        if (check_uac_device(ctx, device)) {
            ret = libusb_open(uac_dev, &uac_devh);
            if (ret < 0) {
                log_e("Failed to open libusb device, errno %d\n", ret);
                return ret;
            }

            ret = scan_audio_interface(uac_dev);
            if (ret < 0) {
                log_e("Failed to scan audio interface, errno %d\n", ret);
                return ret;
            }

            ret = interface_claim_if(uac_devh);
            if (ret < 0) {
                log_e("Failed to claim interface, errno %d\n", ret);
                return ret;
            }

            _uac_device_init();
        }
    } else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
        log_i("[ctx:%p] Device detached: %04x:%04x\n", ctx, desc.idVendor, desc.idProduct);
        if (check_uac_device(ctx, device)) {
            if (is_run) {
                cancel_iso_transfer();

                is_run = false;
                _uac_wakeup_waitlist();

                channels = DEF_CHANNELS;
                sample_rate = DEF_SRATE;
                bits_per_coded_sample = DEF_BITS;

                mMaxPacketSize = 0;
                _controlInterface = 0;
                _microphone_inf = 0;
                _alternateSetting = 0;
                _microphoneEndpoint = 0;
                kshm_mic = NULL;
            }

            libusb_close(uac_devh);
            _uac_device_deinit();
        }
    }
    return 0;
}

static void *uac_pthread(void *arg)
{
    struct libusb_context *uac_ctx = (struct libusb_context *)arg;
    while (!kill_handler_thread)
        libusb_handle_events_completed(uac_ctx, &kill_handler_thread);
    return NULL;
}

int usbmic_open(struct file *file)
{
    struct uac_instance *ins;

    ins = malloc(sizeof(struct uac_instance));
    if (!ins) {
        log_e("[Error] Cannot alloc usb mircophone instance\n");
        return -1;
    }
    memset(ins, 0, sizeof(struct uac_instance));

    file->f_priv = ins;
    ins->closed = false;
    init_waitqueue_head(&ins->wait);
    list_add_tail(&ins->list, &__uac_handler.instance_list);
    init_completion(&ins->completion);
    return 0;
}

int usbmic_close(struct file *file)
{
    struct uac_instance *ins = file->f_priv;

    if (ins) {
        if (!list_empty(&ins->wait.task_list)) {
            ins->closed = true;
            wake_up(&ins->wait);
            while (!list_empty(&ins->wait.task_list)) {
                if (wait_for_completion_timeout(&ins->completion, 2))
                    break;
            }
        }

        if ((ins->list.next != LIST_POISON1) && (ins->list.prev != LIST_POISON2))
            list_del(&ins->list);
        free(ins);
    }
    return 0;
}

ssize_t usbmic_read(FAR struct file *file, FAR char *buffer, size_t buflen)
{
    int ret;
    mutex_lock(&__uac_handler.mutex);
    ret = kshm_read(kshm_mic, buffer, buflen);
    mutex_unlock(&__uac_handler.mutex);
    return ret;
}

ssize_t usbmic_write(FAR struct file *file, FAR const char *buffer, size_t buflen)
{
    return -1;
}

int usbmic_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
    struct usbuac_config *config;
    int ret;

    switch (cmd) {
    case USBUAC_MIC_SET_CONFIG:
        config = (struct usbuac_config *)arg;
        channels = config->channels;
        bits_per_coded_sample = config->bits_per_coded_sample;
        sample_rate = config->sample_rate;

        log_i("scan audio interface (rate:%lu, chs:%u, bits:%lu) \n", sample_rate, channels,
              bits_per_coded_sample);
        ret = scan_audio_interface(uac_dev);
        if (ret < 0) {
            log_e("Failed to scan audio interface, errno %d\n", ret);
            return ret;
        }

        ret = interface_claim_if(uac_devh);
        if (ret < 0) {
            log_e("Failed to claim interface, errno %d\n", ret);
            return ret;
        }

        set_sample_rate(sample_rate);

        break;

    case USBUAC_MIC_GET_CONFIG:
        config = (struct usbuac_config *)arg;
        config->channels = channels;
        config->bits_per_coded_sample = bits_per_coded_sample;
        config->sample_rate = sample_rate;
        break;

    case USBUAC_MIC_START:
        log_i("start usb mic transfer\n");
        if (!is_run) {
            is_run = true;

            set_sample_rate(sample_rate);
            kshm_reset(kshm_mic);
            fill_iso_transfer();
        }
        break;

    case USBUAC_MIC_STOP:
        log_i("stop usb mic transfer\n");
        if (is_run) {
            cancel_iso_transfer();

            is_run = false;
            _uac_wakeup_waitlist();
        }
        break;

    default:
        log_e("Error ioctl command, please check ...\n");
        return -1;
    }

    return 0;
}

int usbmic_poll(FAR struct file *file, poll_table *wait)
{
    struct uac_instance *ins = file->f_priv;
    int mask = 0;

    if (ins->closed) {
        complete(&ins->completion);
        return -1;
    }
    poll_wait(file, &ins->wait, wait);

    if (is_run == false)
        mask = POLLHUP;
    else if (kshm_get_valid_size(kshm_mic))
        mask = POLLIN | POLLRDNORM;
    return mask;
}

const struct file_operations usbmic_fops = {
    .read = usbmic_read,
    .write = usbmic_write,
    .poll = usbmic_poll,
    .ioctl = usbmic_ioctl,
    .open = usbmic_open,
    .close = usbmic_close,
};

static int _uac_device_init(void)
{
    if (_microphoneEndpoint) {
        kshm_mic = kshm_create(KSHM_BUF_SIZE * 1024);
        if (kshm_mic == NULL) {
            log_e(" Cannot create kshm buffer for usb microphone\n");
            return -1;
        }

        /* create device file for usb mircohpone */
        register_driver("/dev/usbmic0", &usbmic_fops, 0666, NULL);
        printf("Create /dev/usbmic0 for usb microphone\n");

        INIT_LIST_HEAD(&__uac_handler.instance_list);
        mutex_init(&__uac_handler.mutex);
    }

    return 0;
}

static void _uac_device_deinit(void)
{
    if (_microphoneEndpoint) {
        if(kshm_mic)
            kshm_destroy(kshm_mic);
        mutex_destroy(&__uac_handler.mutex);
        unregister_driver("/dev/usbmic0");
        printf("Destory /dev/usbmic0 for usb microphone\n");
    }
}

int libuac_probe(void)
{
    int r;

    if (uac_ctx != NULL)
        return 0;

    r = libusb_init(&uac_ctx);
    if (r < 0) {
        log_e("libusb_init fail : %d\n", r);
        return -1;
    }

    if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
        log_e("Hotplug capability is not supported on this platform\n");
        libusb_exit(uac_ctx);
        return -1;
    }

    r = libusb_hotplug_register_callback(uac_ctx,
                         LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                         LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY,
                         LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL,
                         &uac_hotplug_hdl);
    if (r != LIBUSB_SUCCESS) {
        log_e("Error registering hotplug callback: %d\n", r);
        libusb_exit(uac_ctx);
        return -1;
    }

    kill_handler_thread = 0;
    if (pthread_create(&uac_pthread_hdl, NULL, uac_pthread, (void *)uac_ctx) != 0) {
        log_e("Failed to create uac libusb pthread\n");
        return -1;
    }

    log_i("libusb probe\n");
    return 0;
}

int libuac_release(void)
{
    if (uac_ctx) {
        if (is_run) {
            is_run = false;
            kill_handler_thread = 1;
            pthread_join(uac_pthread_hdl, NULL);
        }
        libusb_hotplug_deregister_callback(uac_ctx, uac_hotplug_hdl);
        libusb_exit(uac_ctx);
        log_i("libusb release\n");
    }
    return 0;
}

module_others(libuac, libuac_probe, libuac_release, 1)
