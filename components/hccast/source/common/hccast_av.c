#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/poll.h>
#include <sys/ioctl.h>

#include <hudi/hudi_com.h>
#include <hudi/hudi_snd.h>
#include <hudi/hudi_vdec.h>
#include <hudi/hudi_display.h>

#include <hccast_log.h>
#include <hccast_com.h>
#include <hccast_av.h>

static unsigned int g_audio_sink = AUDSINK_SND_DEVBIT_I2SO;
static unsigned int g_audio_disable = 0;
static unsigned int g_audio_dec_force = 0;

int hccast_com_audsink_get(unsigned int *audsink)
{
    if (!audsink)
    {
        return -1;
    }

    *audsink = g_audio_sink;

    return 0;
}

int hccast_com_audsink_set(unsigned int audsink)
{
    g_audio_sink = audsink;

    return 0;
}

int hccast_com_video_pbp_mode_translate(unsigned int mode)
{
    video_pbp_mode_e pbp_mode = VIDEO_PBP_OFF;

    switch (mode)
    {
        case HCCAST_COM_VIDEO_PBP_OFF:
            pbp_mode = VIDEO_PBP_OFF;
            break;
        case HCCAST_COM_VIDEO_PBP_2P_ON:
            pbp_mode = VIDEO_PBP_2P_ON;
            break;
    }

    return pbp_mode;
}

int hccast_com_video_dis_type_translate(unsigned int mode)
{
    dis_type_e dis_type = DIS_TYPE_HD;

    switch (mode)
    {
        case HCCAST_COM_VIDEO_DIS_TYPE_HD:
            dis_type = DIS_TYPE_HD;
            break;
        case HCCAST_COM_VIDEO_DIS_TYPE_UHD:
            dis_type = DIS_TYPE_UHD;
            break;
    }

    return dis_type;
}

int hccast_com_video_dis_layer_translate(unsigned int mode)
{
    dis_layer_e dis_layer = DIS_LAYER_MAIN;

    switch (mode)
    {
        case HCCAST_COM_VIDEO_DIS_LAYER_MAIN:
            dis_layer = DIS_LAYER_MAIN;
            break;
        case HCCAST_COM_VIDEO_DIS_LAYER_AUXP:
            dis_layer = DIS_LAYER_AUXP;
            break;
    }

    return dis_layer;
}

int hccast_com_audio_disable_set(unsigned int disable)
{
    g_audio_disable = disable;
    return 0;
}

int hccast_com_audio_disable_get(unsigned int *disable)
{
    if (!disable)
    {
        return -1;
    }

    *disable = g_audio_disable;

    return 0;
}

int hccast_com_force_auddec_set(unsigned int enable)
{
    g_audio_dec_force = enable;
    return 0;
}

int hccast_com_force_auddec_get(unsigned int *enable)
{
    if (!enable)
    {
        return -1;
    }

    *enable = g_audio_dec_force;

    return 0;
}
