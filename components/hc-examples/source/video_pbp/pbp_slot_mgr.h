#ifndef __PBP_SLOT_MGR_H__
#define __PBP_SLOT_MGR_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef enum{
	PBP_DIS_TYPE_HD = 0,
	PBP_DIS_TYPE_UHD,

	PBP_DIS_TYPE_MAX,
}pbp_dis_type_e;

typedef enum{
	PBP_DIS_LAYER_MAIN,
	PBP_DIS_LAYER_AUXP,
	
	PBP_DIS_LAYER_MAX,
}pbp_dis_layer_e;

/*
typedef enum{
	PBP_DIS_SRC_MP,
	PBP_DIS_SRC_HDMI_IN,
	PBP_DIS_SRC_CVBS_IN,
	PBP_DIS_SRC_MIRACAST,
	PBP_DIS_SRC_AIRCAST,
	PBP_DIS_SRC_AUM,
	PBP_DIS_SRC_IUM,
	PBP_DIS_SRC_DLNA,

	PBP_DIS_SRC_MAX,
}pbp_dis_src_e;
*/

#define DIS_ALL_ENTRIES()     \
    DIS_ENTRY(PBP_DIS_SRC_MP, "media player")     \
    DIS_ENTRY(PBP_DIS_SRC_HDMI_IN, "HDMI in")     \
    DIS_ENTRY(PBP_DIS_SRC_CVBS_IN, "CVBS in")     \
    DIS_ENTRY(PBP_DIS_SRC_MIRACAST, "miracast")     \
    DIS_ENTRY(PBP_DIS_SRC_AIRCAST, "air cast")     \
    DIS_ENTRY(PBP_DIS_SRC_DLNA, "dlna")     \
    DIS_ENTRY(PBP_DIS_SRC_AUM, "usb android mirror")     \
    DIS_ENTRY(PBP_DIS_SRC_IUM, "usb ios mirror")     \
    DIS_ENTRY(PBP_DIS_SRC_MAX, "MAX")     


#define DIS_ENTRY(src_id, str)    src_id,
typedef enum{
	DIS_ALL_ENTRIES()
}pbp_dis_src_e;
#undef DIS_ENTRY


void pbp_slot_busy_set(pbp_dis_type_e dis_type, pbp_dis_layer_e dis_layer, pbp_dis_src_e src_type);
bool pbp_slot_busy_get(pbp_dis_type_e dis_type, pbp_dis_layer_e dis_layer);
void pbp_slot_busy_clear(pbp_dis_type_e dis_type, pbp_dis_layer_e dis_layer);
void pbp_slot_busy_status(void);
void pbp_slot_busy_init(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //end of __PBP_SLOT_MGR_H__