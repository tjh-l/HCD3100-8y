#ifndef __HCCAST_COM_H__
#define __HCCAST_COM_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
    HCCAST_COM_VIDEO_PBP_OFF,
    HCCAST_COM_VIDEO_PBP_2P_ON,	//!< Two pictures
} hccast_com_video_pbp_mode_e;

typedef enum
{
    HCCAST_COM_VIDEO_DIS_TYPE_HD,
    HCCAST_COM_VIDEO_DIS_TYPE_UHD,
}hccast_com_video_dis_type_e;

typedef enum
{
    HCCAST_COM_VIDEO_DIS_LAYER_MAIN,
    HCCAST_COM_VIDEO_DIS_LAYER_AUXP,
} hccast_com_video_dis_layer_e;

typedef struct
{
    hccast_com_video_pbp_mode_e video_pbp_mode;
    hccast_com_video_dis_type_e video_dis_type;
    hccast_com_video_dis_layer_e video_dis_layer;
} hccast_com_video_config_t;

int hccast_com_audsink_set(unsigned int audsink);
int hccast_com_audio_disable_set(unsigned int disable);
int hccast_com_force_auddec_set(unsigned int enable);

#ifdef __cplusplus
}
#endif


#endif