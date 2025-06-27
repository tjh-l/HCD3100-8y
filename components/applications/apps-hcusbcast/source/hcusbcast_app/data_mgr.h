/*
data_mgr.h
used for save the config data, and read config data from nonvolatile memory
(nor/nand flash)

 */

#ifndef __DATA_MGR_H__
#define __DATA_MGR_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <hcuapi/dis.h>
#include <hcuapi/hdmi_tx.h>
#include <hcuapi/sysdata.h>


typedef enum{
	APP_TV_SYS_480P = 1,
	APP_TV_SYS_576P,
	APP_TV_SYS_720P,
	APP_TV_SYS_1080P,
	APP_TV_SYS_4K,

	APP_TV_SYS_AUTO,

}app_tv_sys_t;

typedef enum
{
    MIRROR_ROTATE_0,
    MIRROR_ROTATE_270,
    MIRROR_ROTATE_90,
    MIRROR_ROTATE_180,
} mirror_rotate_e;

typedef struct app_data{
    uint32_t data_crc32; //do not modify.

    app_tv_sys_t resolution;
    int mirror_rotation;    // 0-disable, 1-enable.
    int mirror_vscreen_auto_rotation;//0-disable, 1-enable.
    int um_full_screen;          // 0-disable, 1-enable.

    unsigned int ium_pdata_len;
    char ium_pair_data[20*1024];
    char ium_uuid[40];
}app_data_t;

typedef struct sysdata sys_data_t;

int data_mgr_load(void);
app_data_t *data_mgr_app_get(void);
sys_data_t *data_mgr_sys_get(void);
int data_mgr_save(void);
void data_mgr_app_tv_sys_set(app_tv_sys_t app_tv_sys);
void data_mgr_de_tv_sys_set(int tv_type);
enum TVTYPE data_mgr_tv_type_get(void);
uint8_t data_mgr_volume_get(void);
void data_mgr_volume_set(uint8_t vol);
void data_mgr_factory_reset(void);
app_tv_sys_t data_mgr_app_tv_sys_get(void);
int data_mgr_de_tv_sys_get(void);
void data_mgr_cast_rotation_set(int rotate);
int data_mgr_cast_rotation_get(void);
void data_mgr_flip_mode_set(int flip_mode);
int data_mgr_flip_mode_get(void);
void data_mgr_ium_uuid_set(char* uuid);
void data_mgr_ium_pair_data_set(char* ium_pair_data);
void data_mgr_ium_pdata_len_set(int ium_pdata_len);
void data_mgr_cast_full_screen_set(int enable);
int data_mgr_cast_full_screen_get(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif



#endif