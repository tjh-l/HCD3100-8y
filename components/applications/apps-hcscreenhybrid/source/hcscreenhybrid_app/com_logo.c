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
#include <hcuapi/dis.h>
#include "com_logo.h"

static int g_logo_vfd = -1;
static int g_logo_backup = 0;

int com_logo_dis_backup(void)
{
    int fd = -1;
    int distype = DIS_TYPE_HD;

    if(g_logo_backup == 0)
    {
        fd = open("/dev/dis", O_WRONLY);
        if (fd < 0)
        {
            printf("[%s] open dis error\n", __func__);
            return -1;    
        }
        usleep(100 * 1000);
        if (ioctl(fd, DIS_BACKUP_MP, distype) != 0)
        {
            printf("dis backup mp fail.\n");
            close(fd);
            return -1;
        }
        usleep(100 * 1000);
        close(fd);
        g_logo_backup = 1;
        printf("[%s] backup logo dis.\n", __func__);
    }

    return 0;
}

int com_logo_dis_backup_free(void)
{
    int fd = -1;
    int distype = DIS_TYPE_HD;

    if(g_logo_backup)
    {
        fd = open("/dev/dis", O_WRONLY);
        if (fd < 0)
        {
            printf("[%s] open dis error\n", __func__);
            return -1;    
        }
        
        if (ioctl(fd, DIS_FREE_BACKUP_MP, distype) != 0)
        {
            printf("dis free backup mp fail.\n");
            close(fd);
            return -1;
        }
        
        close(fd);     
        g_logo_backup = 0;
        printf("[%s] free backup logo dis.\n", __func__);
    }
    
    return 0;
}

int com_logo_h264_video_open(void)
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
    mvcfg.rotate_enable = 1;//default enable.
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

int com_logo_h264_video_feed(unsigned char *data, unsigned int len, int rotate_type, int mirror_type)
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

void com_logo_h264_video_close(int closevp, int fillblack)
{
    struct vdec_rls_param rls_param = {0, 0};
    int fd = -1;
    int distype = DIS_TYPE_HD;

    rls_param.closevp = closevp;
    rls_param.fillblack = fillblack;

    if (g_logo_vfd > 0)
    {
        if(!fillblack && !closevp)
        {
            ioctl(g_logo_vfd, VIDDEC_RLS, &rls_param);
            com_logo_dis_backup();
        }    
        
        close(g_logo_vfd);
        g_logo_vfd = -1;
    }
}

void com_logo_off(int closevp, int fillblack)
{
    com_logo_h264_video_close(closevp, fillblack);
}

int com_logo_show(const char *file_path, int rotate_type, int mirror_type)
{
    FILE *h264file = NULL;
    int tol_len = 0;
    char *data = NULL;
    int ret = 0;
    int read_len = 0;
    int left_len = 0;
    char nalu_data[5] = {0x0, 0x0, 0x0, 0x1, 0x9};
    char *file = file_path;

    com_logo_dis_backup_free();
    
    if(!file_path)
    {
        printf("[%s] invalid file path param\n", __func__);
        return -1;
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

    com_logo_h264_video_feed(data, tol_len, rotate_type, mirror_type);
    com_logo_h264_video_feed(nalu_data, sizeof(nalu_data), rotate_type, mirror_type);

    fclose(h264file);
    free(data);
    
    return 0;
}

