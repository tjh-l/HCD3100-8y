#ifndef __LIBUAC_INTERNAL_H__
#define __LIBUAC_INTERNAL_H__

#include <stdint.h>
#include <kernel/wait.h>
#include <kernel/list.h>
#include <linux/mutex.h>
#include <kernel/completion.h>

/* AS_GENERAL descriptor subtype */
struct audio_streaming_format_descriptor {
    uint8_t bLength;   //该结构体的字节度长，
    uint8_t bDescriptorType; //描述符类型CS_INTERFACE，值为0x24
    uint8_t bDescriptorSubtype;  //描述符子类型 AS_GENERAL
    uint8_t bTerminalLink;
    uint8_t bDelay;
    uint16_t wFormatTag;
}__attribute__ ((packed));

/* FORMAT_TYPE descriptor subtype */
struct audio_streaming_descriptor {
    uint8_t bLength; //该结构体的字节度长，
    uint8_t bDescriptorType; //描述符类型CS_INTERFACE，值为0x24
    uint8_t bDescriptorSubtype;  //描述符子类型 FORMAT_TYPE
    uint8_t bFormatType; //音频数据格式，这里为FORMAT_TYPE_I
    uint8_t bNrChannels; //音频数据的通道数
    uint8_t bSubframeSize; //每通道数据的字节数，可以1，2，3，4
    uint8_t bBitResolution; //bSubframeSize中的有效位数。
    uint8_t bSamFreqType; //在当前interface下有多少种采样率
    uint8_t tSamFreq[0];
}__attribute__ ((packed));


/* default audio microphone paramters */
#define DEF_CHANNELS     1        // microphone channel
#define DEF_BITS    16       // microphone bits
#define DEF_SRATE   44100    // microphone sample rate

/* libusb transfer paramters */
#define NUM_PACKETS   32
#define NUM_TRANSFERS LIBUAC_NUM_TRANSFER_BUFS

#define KSHM_BUF_SIZE LIBUAC_MIC_RINGBUF_SIZE   // unit: KBytes

// 0x22
#define USB_REQ_CS_ENDPOINT_SET \
                (LIBUSB_ENDPOINT_OUT | \
                LIBUSB_REQUEST_TYPE_CLASS | \
                LIBUSB_RECIPIENT_ENDPOINT)

// 0xa2
#define USB_REQ_CS_ENDPOINT_GET \
                (LIBUSB_ENDPOINT_IN | \
                LIBUSB_REQUEST_TYPE_CLASS | \
                LIBUSB_RECIPIENT_ENDPOINT)

/** UAC request code (A.8) */
enum uac_req_code {
    UAC_RC_UNDEFINED = 0x00,
    UAC_SET_CUR = 0x01,
    UAC_GET_CUR = 0x81
};

struct uac_stream_handler {
    int running;
    int frames;

    /* wait list and mutex*/
    struct completion completion;
    struct list_head instance_list;
    struct mutex mutex;
};

struct uac_instance {
    struct list_head list;
    wait_queue_head_t wait;
    struct completion completion;
    bool closed;
};

typedef struct uac_stream_handler uac_stream_handler_t;


#endif // __LIBUAC_INTERNAL_H__
