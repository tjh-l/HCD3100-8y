#ifndef __HCCAST_AV_H__
#define __HCCAST_AV_H__

#ifdef __cplusplus
extern "C" {
#endif

int hccast_com_audsink_get(unsigned int *audsink);
int hccast_com_video_pbp_mode_translate(unsigned int mode);
int hccast_com_video_dis_type_translate(unsigned int mode);
int hccast_com_video_dis_layer_translate(unsigned int mode);
int hccast_com_audio_disable_get(unsigned int *disable);
int hccast_com_force_auddec_get(unsigned int *enable);

#ifdef __cplusplus
}
#endif

#endif
