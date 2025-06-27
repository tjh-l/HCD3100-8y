#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
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
#include <sys/stat.h>
#include <fcntl.h>
#include <kernel/elog.h>
#include <kernel/drivers/hcusb.h>
#include <sys/ioctl.h>

#include <hcuapi/common.h>
#include <hcuapi/kshm.h>
#include <hcuapi/auddec.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/snd.h>

struct pcm_decoder {
	struct audio_config cfg;
	int fd;
};

static struct __uac_demo_st {
    /* mircophone */
    int fd_mircophone;
    uint8_t *buf_mircophone;

    /* speaker */
    // int fd_speaker;
    void *pcm_hdl;
    xQueueHandle xQueue;
    uint8_t *buf_speaker;

} g_uac_demo;

static void *pcm_decoder_init(int bits, int channels, int samplerate)
{
	struct pcm_decoder *p = (struct pcm_decoder *)malloc(sizeof(struct pcm_decoder));
	memset(&p->cfg, 0, sizeof(struct audio_config));
	p->cfg.bits_per_coded_sample = bits;
	p->cfg.channels = channels;
	p->cfg.sample_rate = samplerate;
	p->cfg.codec_id = HC_AVCODEC_ID_PCM_S16LE;

	p->fd = open("/dev/auddec", O_RDWR);
	printf("p->fd = %x\n", p->fd);
	if (p->fd < 0) {
		printf("Open /dev/auddec error.");
		free(p);
		return NULL;
	}

	if (ioctl(p->fd, AUDDEC_INIT, &p->cfg) != 0) {
		printf("Init auddec error.");
		close(p->fd);
		free(p);
		return NULL;
	}
	printf("AUDDEC_START\n");
	ioctl(p->fd, AUDDEC_START, 0);
	return p;
}

static int pcm_decode(void *phandle, uint8_t *audio_frame, size_t packet_size)
{
	struct pcm_decoder *p = (struct pcm_decoder *)phandle;
	AvPktHd pkthd = { 0 };

	pkthd.dur = 0;
	pkthd.size = packet_size;
	pkthd.flag = AV_PACKET_ES_DATA;
	pkthd.pts = -1;

	while (1) {
		if (write(p->fd, (uint8_t *)&pkthd, sizeof(AvPktHd)) !=
		    sizeof(AvPktHd)) {
			usleep(20 * 1000);
			continue;
		}
		break;
	}
	while (1) {
		if (write(p->fd, audio_frame, packet_size) != (int)packet_size) {
			usleep(20 * 1000);
			continue;
		}
		break;
	}

	return 0;
}



static int uac_mircophone_hook(uint8_t **frame, int *frame_sz)
{
    static int fd_mic = -1;
    uint8_t *buf = g_uac_demo.buf_mircophone;
    int rd;

    if(fd_mic < 0)
        fd_mic = open("/media/sda1/sine.pcm", O_RDWR);

    if(fd_mic < 0) {
        printf(" ==> Cannot open /media/sda1/sine.pcm \n");
        *frame = NULL;
        *frame_sz = 0;
        return -1;
    }

    rd = read(fd_mic, buf, 192);
    if(rd < 192) {
        printf(" [warn] Roll back to the beginning of the file\n");
        lseek(fd_mic, 0, SEEK_SET);
        rd = read(fd_mic, buf, 192);
    }

    *frame = buf;
    *frame_sz = rd;
    return 0;
}


static void uac_speaker_daemon(void *__parm)
{
    void *hdl = g_uac_demo.pcm_hdl;
    xQueueHandle xQueue = g_uac_demo.xQueue;
    portBASE_TYPE xStatus;
    uint32_t *buf = (uint32_t *)g_uac_demo.buf_speaker;
    int cnt, index, wr, queue_total;
    uint8_t *buf_temp;
    // int fd = g_uac_demo.fd_speaker;

    for (;;) {
        usleep(10 * 1000); // delay 10 ms

        queue_total = uxQueueMessagesWaiting(xQueue);
        for (index = 0; index < queue_total; index++) {
            xStatus = xQueueReceive(xQueue, buf + index,
                        10); // timeout 10ms
            if (xStatus != pdPASS)
                break;
        }

        if (index == 0 || hdl == NULL)
            continue;

        cnt = index * 4;  // queue item unit is word(4 bytes)

        if(pcm_decode(hdl, (uint8_t *)buf, cnt))
            printf(" [warn] PCM decoder error\n");

        // write(fd, buf, cnt); // write PCM data into u-disk
        // printf(" ==> dec:%d\n", cnt);
    }
}


static int uac_speaker_hook(uint8_t *frame, int frame_sz)
{
    int index;
    portBASE_TYPE xStatus;
    xQueueHandle xQueue = g_uac_demo.xQueue;
    uint32_t *w_frame = (uint32_t *)frame;

    for(index = 0; index < frame_sz / 4; index++) {
        xStatus = xQueueSendToBack(xQueue, w_frame + index, 0);
        if (xStatus != pdPASS) {
            printf(" ==> [warn] speaker daemon : cannot write audio data into queue\n");
            return -1;
        }
    }

    return 0;
}


static int av_switch_hook(usb_av_dev_t dev, bool is_on)
{
    switch(dev) {
        case uvc_cam_dev:
            printf("[av demo] camera %s\n", is_on ? "power on" : "power off");
            break;
        case uac_speaker_dev:
            printf("[av demo] speaker %s\n", is_on ? "power on" : "power off");
            break;
        case uac_mic_dev:
            printf("[av demo] microphone %s\n", is_on ? "power on" : "power off");
            break;
    }
    return 0;
}


static void usbd_uac_help_info(void)
{
    printf("usb device mode : uac\n");
}


int setup_usbd_uac1(int argc, char **argv)
{
    char ch;
    int i;
    const char *udc_name = NULL;
    int usb_port = 0;
    int fd;
    xQueueHandle xQueue;
    void *hdl;

    opterr = 0;
    optind = 0;

    elog_set_filter_tag_lvl("uac", ELOG_LVL_ALL);
    elog_set_filter_tag_lvl("usbd", ELOG_LVL_ALL);

    while ((ch = getopt(argc, argv, "hHsSp:P:")) != EOF) {
        switch(ch) {
            case 'h' :
            case 'H' :
                usbd_uac_help_info();
                return 0;
            case 'p' :
            case 'P' :
                usb_port = atoi(optarg);
                udc_name = get_udc_name(usb_port);
                if(udc_name == NULL){
                    printf("[error] parameter(-p {usb_port}) error,"
                        "please check for help information(cmd: g_mass_storage -h)\n");
                    return -1;
                }
                printf("==> set usb#%u as uac1 demo gadget\n", usb_port);
                break;
            case 's' :
            case 'S' :
                hcusb_gadget_uac_deinit();
                if(g_uac_demo.xQueue)
                    vQueueDelete(g_uac_demo.xQueue);
                printf(" exit uac gadget demo\n");
                return 0;
            default:
                break;
        }
    }

    // hdl = pcm_decoder_init(16, 1, 8000);   // 16 bits, 1 channel, sample rate 8K
    hdl = pcm_decoder_init(16, 2, 48000);   // 16 bits, 2 channel, sample rate 48K
    if(hdl == NULL) {
        printf(" ==> cannot init pcm decoder\n");
        return -1;
    }
    g_uac_demo.pcm_hdl = hdl;

    xQueue = xQueueCreate(8 * 1024, 4); // total size: 8K * 4 = 32KB
    if(!xQueue) {
        printf(" ==> Cannot create queue for mircophone \n");

        return -1;
    }
    g_uac_demo.xQueue = xQueue;
    g_uac_demo.buf_mircophone = malloc(512);
    g_uac_demo.buf_speaker = malloc(64 * 1024);
    if(!g_uac_demo.buf_speaker) {
        printf(" ==> Cannot malloc enough memory \n");
        return -1;
    }

    xTaskCreate(uac_speaker_daemon , "uac speaker daemon" ,
                        0x1000 , NULL, portPRI_TASK_NORMAL , NULL);

    if(!udc_name)
        hcusb_gadget_uac_init(av_switch_hook, 
                            uac_mircophone_hook, 
                            uac_speaker_hook);
    else
        hcusb_gadget_uac_specified_init(udc_name, 
                            av_switch_hook,
                            uac_mircophone_hook,
                            uac_speaker_hook);

    hcusb_set_mode(usb_port, MUSB_PERIPHERAL);

    return 0;
}

CONSOLE_CMD(g_uac, "usb", setup_usbd_uac1, CONSOLE_CMD_MODE_SELF,
	    "setup USB as UAC device demo, including mircophone/speaker")
