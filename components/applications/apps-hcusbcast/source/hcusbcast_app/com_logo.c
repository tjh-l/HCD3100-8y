#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <hcuapi/viddec.h>
#include <hcuapi/vidmp.h>
#include <hcuapi/codec_id.h>
#include <hcuapi/common.h>
#include "com_logo.h"

static int g_logo_vfd = -1;
int com_logo_h264_video_open()
{
    struct video_config mvcfg;
    
    memset(&mvcfg, 0, sizeof(struct video_config));
    
    mvcfg.codec_id = HC_AVCODEC_ID_H264;
    mvcfg.sync_mode = 0;//AVSYNC_TYPE_UPDATESTC;
    mvcfg.quick_mode = 3;
    mvcfg.decode_mode = VDEC_WORK_MODE_KSHM;
    mvcfg.pic_width = 1920;
    mvcfg.pic_height = 1080;
    mvcfg.frame_rate = 60 * 1000;
    mvcfg.pixel_aspect_x = 1;
    mvcfg.pixel_aspect_y = 1;
    mvcfg.preview = 0;
    mvcfg.extradata_size = 0;
    mvcfg.kshm_size = 0x100000;

    g_logo_vfd = open("/dev/viddec", O_RDWR);
    if (g_logo_vfd < 0)
    {
        printf("%s Open /dev/viddec error!\n", __func__);
        return -1;
    }

    if (ioctl(g_logo_vfd, VIDDEC_INIT, &mvcfg) != 0)
    {
        printf("%s Init viddec error!\n", __func__);
        close(g_logo_vfd);
        g_logo_vfd = -1;
        return -1;
    }
    
    ioctl(g_logo_vfd, VIDDEC_START, 0);
    return 0;
}

int com_logo_h264_video_feed(unsigned char *data, unsigned int len)
{
    AvPktHd pkthd = { 0 };    
    pkthd.pts = 0;
    pkthd.dur = 0;
    pkthd.size = len;
    pkthd.flag = AV_PACKET_ES_DATA;

    if (write(g_logo_vfd, (uint8_t *)&pkthd, sizeof(AvPktHd)) != sizeof(AvPktHd))
    {
        printf("%s Write AvPktHd fail!\n", __func__);
        return -1;
    }

    if (write(g_logo_vfd, data, len) != len)
    {
        printf("%s Write video_frame error fail!\n", __func__);
        return -1;
    }
    
    return 0;      
}

void com_logo_h264_video_close(void)
{
    if (g_logo_vfd > 0)
    {
        close(g_logo_vfd);
        g_logo_vfd = -1;
        printf("=============================close h264 logo: ok!=============================\n");
    }
}

void com_logo_off(void)
{
    com_logo_h264_video_close();
}

int com_logo_show(const char *file_path)
{
    FILE *h264file = NULL;
    int tol_len = 0;
    char *data = NULL;
    int ret = 0;
    int read_len = 0;
    int left_len = 0;
    char nalu_data[5] = {0x0, 0x0, 0x0, 0x1, 0x9};
    char *file = file_path;

    com_logo_off();
    
    if(!file_path)
    {
        file = UM_LOGO;
    }
   
    h264file = fopen(file, "rb");
    if(h264file == NULL)
    {
        printf("[%s] open %s file fail\n", __func__, file);
        return -1;
    }

    fseek(h264file, 0, SEEK_END);
    
    tol_len = ftell(h264file);
    if(tol_len <= 0)
    {
        printf("[%s] file len error: %d\n", __func__, tol_len);
        fclose(h264file);
        return -1;
    }

    fseek(h264file, 0, SEEK_SET);
    
    data = (char*)malloc(tol_len);
    if(data == NULL)
    {
        printf("[%s] data malloc error.\n", __func__);
        fclose(h264file);
        return -1;
    }

    left_len = tol_len;
    while(left_len)
    {
        ret = fread(data+read_len, 1, left_len, h264file);
        if(ret < 0)
        {
            printf("[%s] fread error.\n", __func__);
            fclose(h264file);
            free(data);
            return -1;
        }
        
        left_len -= ret;
        read_len += ret;
    }

    if(com_logo_h264_video_open() < 0)
    {
        fclose(h264file);
        free(data);
        return -1;
    }

    com_logo_h264_video_feed(data, tol_len);
    com_logo_h264_video_feed(nalu_data, sizeof(nalu_data));

    fclose(h264file);
    free(data);

    printf("=============================Show h264 logo: %s ok!=============================\n", file);
    return 0;
}

